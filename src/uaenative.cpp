/*
* UAE - The Un*x Amiga Emulator
*
* UAE Native Interface (UNI) - uaenative.library
*
* Copyright 2013-2014 Frode Solheim. Amiga-side library sample code
* provided by Toni Wilen.
*
* TODO: Handling UAE reset and shutdown better. When resetting the Amiga,
* all opened native libraries should be closed, and all async call threads
* should be stopped.
*/

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef WITH_UAENATIVE

#include <stdlib.h>
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "traps.h"
#include "autoconf.h"
#include "execlib.h"
#include "threaddep/thread.h"
#include "native2amiga.h"
#include "events.h"
#include "uaenative.h"
#include "fsdb.h"


#ifndef _WIN32
#include <dlfcn.h>
#endif

static float syncdivisor;

#define SIGBIT 8 // SIGB_DOS

// the function prototype for the callable native functions
typedef uae_s32 (UNICALL *uae_uni_native_function)(struct uni *uni);

// the function prototype for the callable native functions (old style)
typedef uae_u32 (UNICALL *uae_uni_native_compat_function)(uae_u32, uae_u32,
        uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32,
        uae_u32, uae_u32, uae_u32, void *, uae_u32, void *);

// the function prototype for the native library's uni_init function
typedef int (UNICALL *uni_init_function)(void);

struct library_data {
    void *dl_handle;
    uae_thread_id thread_id;
    uae_sem_t empty_count;
    uae_sem_t full_count;
    int thread_stop_flag;
    struct uni *uni;
};

struct uni_handle {
    struct library_data *library;
    void *function;
};

static uni_handle *g_handles = NULL;
static int g_allocated_handles = 0;
static int g_max_handle = -1;

#ifdef _WIN32
#ifndef OS_NAME
#define OS_NAME _T("windows")
#endif
#else
#ifdef __MACH__
#ifndef OS_NAME
#define OS_NAME _T("macos")
#endif
#else
#ifndef OS_NAME
#define OS_NAME _T("linux")
#endif
#endif
#endif

#if defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
    #define ARCH_NAME _T("x86-64")
#elif defined(__i386__) || defined (_M_IX86)
    #define ARCH_NAME _T("x86")
#elif defined(__ppc__)
    #define ARCH_NAME _T("ppc")
#elif defined(__arm__)
	#define ARCH_NAME _T("arm")
#else
    #define ARCH_NAME _T("unknown")
#endif

#define MODULE_SUFFIX (OS_NAME _T("-") ARCH_NAME LT_MODULE_EXT)

static int UNICALL uni_version(void)
{
    return UNI_VERSION;
}

static void * UNICALL uni_resolve(uae_u32 ptr)
{
    void *result = get_real_address (ptr);
    //printf ("UNI: resolve address 0x%08x -> %p\n", ptr, result);
    return result;
}

static const char * UNICALL uni_uae_version(void)
{
    // A standard GNU macro with a string containing program name and
    // version, e.g. "WinUAE 2.7.0" or "FS-UAE 2.3.16dev".
    return PACKAGE_STRING;
}

#ifdef AMIBERRY
static void * UNICALL uni_get_context(struct uni *uni)
{
	return uni->ctx;
}
#endif

static void free_library_data(struct library_data *library_data) {
    if (library_data->empty_count) {
        uae_sem_destroy(&library_data->empty_count);
    }
    if (library_data->full_count) {
        uae_sem_destroy(&library_data->full_count);
    }
    free(library_data);
}

static uae_u32 register_handle(library_data *library_data, void *function)
{
    int index = -1;
    if (g_max_handle >= g_allocated_handles - 1) {
        // Entries above g_max_handle are assumed to be uninitialized,
        // but unused/"freed" entries below g_max_handle must be zeroed.
        // Try to find a reusable entry before allocating more space.
        for (int i = 0; i < g_max_handle; i++) {
            if (g_handles[i].library == NULL && g_handles[i].function == NULL) {
                index = i;
                break;
            }
        }
        if (index == -1) {
            int new_count = g_allocated_handles * 2;
            if (new_count == 0) {
                new_count = 128;
            }
            write_log("uni: allocating memory for %d handles\n", new_count);
            g_handles = (uni_handle *) realloc(g_handles,
                                new_count * sizeof(struct uni_handle));
            g_allocated_handles = new_count;
        }
    }

    if (index == -1) {
        index = g_max_handle + 1;
        g_max_handle = index;
    }

    g_handles[index].library = library_data;
    g_handles[index].function = function;

    // valid handles start from 0x80000000, anything below is error code
    return (uae_u32) 0x80000000 + (uae_u32) index;
}

static TCHAR *get_native_library_path (const TCHAR *library_name)
{
    write_log (_T("uni: find_native_library %s\n"), library_name);
    TCHAR path[PATH_MAX];
    const TCHAR **library_dirs = uaenative_get_library_dirs ();

    for (const TCHAR **dir = library_dirs; *dir != NULL; dir++) {
        // name must already have been checked to not contain anything
        // to allow access to parent directories.
        _sntprintf (path, PATH_MAX, _T("%s/%s-%s"), *dir, library_name,
                    MODULE_SUFFIX);
        write_log (_T("uni: checking %s\n"), path);
        if (my_existsfile (path)) {
            return my_strdup (path);
        }
#ifdef _WIN32
        // for compatibility with existing WinUAE native interface
        _sntprintf (path, PATH_MAX, _T("%s/%s.dll"), *dir, library_name);
        write_log (_T("uni: checking %s\n"), path);
        if (my_existsfile (path)) {
            return my_strdup (path);
        }
#endif
    }
    return NULL;
}

static void *dl_symbol(void *dl, const char *name)
{
    if (dl == NULL) {
        return NULL;
    }
#ifdef _WIN32
    return (void *) GetProcAddress ((HINSTANCE) dl, name);
#else
    return dlsym (dl, name);
#endif
}

static void dl_close(void *dl)
{
#ifdef _WIN32
    FreeLibrary ((HMODULE) dl);
#else
    dlclose (dl);
#endif
}

static void set_library_globals(void *dl)
{
    void *address;

    address = dl_symbol(dl, "uni_version");
    if (address) *((uni_version_function *) address) = &uni_version;

    address = dl_symbol(dl, "uni_resolve");
    if (address) *((uni_resolve_function *) address) = &uni_resolve;

    address = dl_symbol(dl, "uni_uae_version");
    if (address) *((uni_uae_version_function *) address) = &uni_uae_version;

#ifdef AMIBERRY
    address = dl_symbol(dl, "uni_get_context");
    if (address) *((uni_get_context_function *) address) = &uni_get_context;
    
    address = dl_symbol(dl, "trap_call_add_dreg");
    if (address) *((trap_call_add_dreg_function *) address) = (trap_call_add_dreg_function) &trap_call_add_dreg;
    
    address = dl_symbol(dl, "trap_call_add_areg");
    if (address) *((trap_call_add_areg_function *) address) = (trap_call_add_dreg_function) &trap_call_add_areg;
    
    address = dl_symbol(dl, "trap_call_lib");
    if (address) *((trap_call_lib_function *) address) = (trap_call_lib_function )&trap_call_lib;
    
    address = dl_symbol(dl, "write_log");
    if (address) *((write_log_function *) address) = &write_log;
#endif
}

static uae_u32 open_library (const char *name, uae_u32 min_version)
{
    syncdivisor = (3580000.0f * CYCLE_UNIT) / (float) syncbase;

    for (const char *c = name; *c; c++) {
        if (*c == '/' || *c == '\\' || *c == ':') {
            return UNI_ERROR_ILLEGAL_LIBRARY_NAME;
        }
    }

    TCHAR *tname = au (name);
    write_log (_T("uni: open native library '%s'\n"), tname);
    TCHAR *path = get_native_library_path (tname);
    free (tname);
    if (path == NULL) {
        write_log(_T("uni: library not found\n"));
        return UNI_ERROR_LIBRARY_NOT_FOUND;
    }

    write_log (_T("uni: found library at %s - opening\n"), path);
#ifdef _WIN32
    void *dl = LoadLibrary (path);
#else
    void *dl = dlopen (path, RTLD_NOW);
#endif
    free(path);
    if (dl == NULL) {
        write_log (_T("uni: error opening library errno %d\n"), errno);
        return UNI_ERROR_COULD_NOT_OPEN_LIBRARY;
    }

    // FIXME: check min version

    set_library_globals(dl);

    void *function_address = dl_symbol(dl, "uni_init");
    if (function_address) {
        int error = ((uni_init_function) function_address)();
        if (error) {
            dl_close(dl);
            return error;
        }
    }

    struct library_data *library_data = (struct library_data *) malloc(
            sizeof(struct library_data));
    memset(library_data, 0, sizeof(struct library_data));
    library_data->dl_handle = dl;

    uae_u32 handle = register_handle (library_data, NULL);
    write_log(_T("uni: opened library %08x (%p)\n"), handle, dl);
    return handle;
}

uae_u32 uaenative_open_library (TrapContext *ctx, int flags)
{
    char namebuf[256];
	
	if (!currprefs.native_code) {
        write_log(_T("uni: tried to open native library, but native code ")
                  _T("is not enabled\n"));
        return UNI_ERROR_NOT_ENABLED;
    }

    uaecptr name;
    uae_u32 min_version;
    if (flags & UNI_FLAG_COMPAT) {
        name = trap_get_areg(ctx, 0);
        min_version = 0;
    }
    else {
        name = trap_get_areg(ctx, 1);
        min_version = trap_get_dreg(ctx, 0);
    }

	trap_get_string(ctx, namebuf, name, sizeof namebuf);

    uae_u32 result = open_library(namebuf, min_version);

    if ((flags & UNI_FLAG_COMPAT) && !(result & 0x80000000)) {
        // error opening library, return 0 for error in compatibility mode
        return 0;
    }
    return result;
}

static struct library_data *get_library_data_from_handle (uae_u32 handle)
{
    int index = handle - (uae_u32) 0x80000000;
    //printf("check library index %u\n", index);
    if (index < 0 || index > g_max_handle) {
        //printf("index < 0 || index > g_max_handle\n");
        return NULL;
    }
    if (g_handles[index].function) {
        //printf("- g_handles[index].function\n");
        return NULL;
    }
    return g_handles[index].library;
}

static uae_u32 get_function_handle (uae_u32 handle, const char *name)
{
    struct library_data *library_data = get_library_data_from_handle (handle);
    if (library_data == NULL) {
        write_log(_T("uni: get_function - invalid library (%d)\n"), handle);
        return UNI_ERROR_INVALID_LIBRARY;
    }

    void *function_address = dl_symbol (library_data->dl_handle, name);
    if (!function_address) {
        write_log(_T("uni: get_function - function (%s) not found ")
                _T("in library %d (%p)\n"), name, handle,
                library_data->dl_handle);
        return UNI_ERROR_FUNCTION_NOT_FOUND;
    }
    return register_handle (library_data, function_address);
}

uae_u32 uaenative_get_function (TrapContext *ctx, int flags)
{
	char namebuf[256];
	
	if (!currprefs.native_code) {
        return UNI_ERROR_NOT_ENABLED;
    }

    //m68k_dreg (regs, 1), m68k_areg (regs, 0),
    //m68k_dreg (regs, 0), m68k_areg (regs, 1),

    uaecptr name;
    uae_u32 library;
    if (flags & UNI_FLAG_COMPAT) {
        name = trap_get_areg(ctx, 0);
        library = trap_get_dreg(ctx, 1);
    }
    else {
        library = trap_get_areg(ctx, 0);
        name = trap_get_areg(ctx, 1);
    }

	trap_get_string(ctx, namebuf, name, sizeof namebuf);
	
	uae_u32 result = get_function_handle (library, namebuf);

    if ((flags & UNI_FLAG_COMPAT) && !(result & 0x80000000)) {
        // error getting function, return 0 for error in compatibility mode
        return 0;
    }
    return result;
}

#if defined(X86_MSVC_ASSEMBLY)

static uae_u32 do_call_function_compat_asm (struct uni *uni)
{
    unsigned int espstore;
    void *native_func = uni->native_function;
    uae_u32 d1 = uni->d1;
    uae_u32 d2 = uni->d2;
    uae_u32 d3 = uni->d3;
    uae_u32 d4 = uni->d4;
    uae_u32 d5 = uni->d5;
    uae_u32 d6 = uni->d6;
    uae_u32 d7 = uni->d7;
    uae_u32 a1 = uni->a1;
    uae_u32 a2 = uni->a2;
    uae_u32 a3 = uni->a3;
    uae_u32 a4 = uni->a4;
    uae_u32 a5 = uni->a5;
    uae_u32 a7 = uni->a7;
    uae_u32 regs_ = (uae_u32)&regs;
    void *a6 = uni->uaevar_compat;

    __asm
    {   mov espstore,esp
        push regs_
        push a7
        push a6
        push a5
        push a4
        push a3
        push a2
        push a1
        push d7
        push d6
        push d5
        push d4
        push d3
        push d2
        push d1
        call native_func
        mov esp,espstore
    }
}

#endif

static void do_call_function (struct uni *uni)
{
//    printf("uni: calling native function %p\n", uni->native_function);

    frame_time_t start_time;
    const int flags = uni->flags;
    if ((flags & UNI_FLAG_ASYNCHRONOUS) == 0) {
        start_time = read_processor_time ();
    }

    if (uni->flags & UNI_FLAG_COMPAT) {
#if defined(X86_MSVC_ASSEMBLY)
        uni->result = (uae_s32) do_call_function_compat_asm(uni);
#else
        uni->result = ((uae_uni_native_compat_function) uni->native_function) (
                uni->d1, uni->d2, uni->d3, uni->d4, uni->d5, uni->d6,
                uni->d7, uni->a1, uni->a2, uni->a3, uni->a4, uni->a5,
                uni->uaevar_compat, uni->a7, &regs);
#endif
    }
    else {
        uni->result = ((uae_uni_native_function) uni->native_function) (uni);
    }

    if ((flags & UNI_FLAG_ASYNCHRONOUS) == 0) {
        frame_time_t time_diff = read_processor_time () - start_time;
        float v = syncdivisor * time_diff;
        if (v > 0) {
            if (v > 1000000.0f * CYCLE_UNIT) {
                v = 1000000.0f * CYCLE_UNIT;
            }
            // compensate for the time spent in the native function
            do_extra_cycles ((unsigned long) (syncdivisor * time_diff));
        }
    }
}

static int uaenative_thread(void *arg)
{
    struct library_data *library_data = (struct library_data *) arg;

    while (1) {
        uae_sem_wait(&library_data->full_count);
        if (library_data->thread_stop_flag) {
            break;
        }
        if (library_data->uni) {
            do_call_function (library_data->uni);
            uae_Signal (library_data->uni->task, 1 << SIGBIT);
        }
        library_data->uni = NULL;
        uae_sem_post(&library_data->empty_count);
    }

    write_log (_T("uni: uaenative_thread exiting\n"));
    free_library_data(library_data);
    return 0;
}

uae_u32 uaenative_call_function (TrapContext *ctx, int flags)
{
    if (!currprefs.native_code) {
        return UNI_ERROR_NOT_ENABLED;
    }

    struct uni uni = {0};
#ifdef AMIBERRY
    uni.ctx = ctx;
#endif
    uni.function = trap_get_areg(ctx, 0);
    if (flags & UNI_FLAG_COMPAT) {
        uni.library = 0;
#ifdef AHI
        uni.uaevar_compat = uaenative_get_uaevar();
#else
        uni.uaevar_compat = NULL;
#endif
    }
    else if (flags & UNI_FLAG_NAMED_FUNCTION) {
        uni.library = trap_get_dreg(ctx, 0);
    }
    else {
        uni.library = 0;
    }

    struct library_data *library_data;

    if (uni.library) {
        // library handle given, function is pointer to function name
		uae_u8 function[256];
		trap_get_string(ctx, function, uni.function, sizeof function);

        library_data = get_library_data_from_handle (uni.library);
        if (library_data == NULL) {
            write_log (_T("uni: get_function - invalid library (%d)\n"),
                    uni.library);
            return UNI_ERROR_INVALID_LIBRARY;
        }

        uni.native_function = dl_symbol (library_data->dl_handle, (const char*)function);
        if (uni.native_function == NULL) {
            write_log (_T("uni: get_function - function (%s) not found ")
                       _T("in library %d (%p)\n"), function, uni.library,
                                                   library_data->dl_handle);
            return UNI_ERROR_FUNCTION_NOT_FOUND;
        }
    }
    else {
        // library handle not given, function argument is function handle
        int index = uni.function - (uae_u32) 0x80000000;
        if (index >= 0 && index <= g_max_handle) {
            uni.native_function = g_handles[index].function;
            library_data = g_handles[index].library;
        }
        else {
            uni.native_function = NULL;
        }
        if (uni.native_function == NULL) {
            // printf ("UNI_ERROR_INVALID_FUNCTION\n");
            return UNI_ERROR_INVALID_FUNCTION;
        }
    }

    if (ctx == NULL) {
        // we have no context and cannot call into m68k space
        flags &= ~UNI_FLAG_ASYNCHRONOUS;
    }

    uni.d1 = trap_get_dreg(ctx, 1);
    uni.d2 = trap_get_dreg(ctx, 2);
    uni.d3 = trap_get_dreg(ctx, 3);
    uni.d4 = trap_get_dreg(ctx, 4);
    uni.d5 = trap_get_dreg(ctx, 5);
    uni.d6 = trap_get_dreg(ctx, 6);
    uni.d7 = trap_get_dreg(ctx, 7);
    uni.a1 = trap_get_areg(ctx, 1);
    uni.a2 = trap_get_areg(ctx, 2);
    uni.a3 = trap_get_areg(ctx, 3);
    uni.a4 = trap_get_areg(ctx, 4);
    uni.a5 = trap_get_areg(ctx, 5);
    uni.a7 = trap_get_areg(ctx, 7);

    uni.flags = flags;
    uni.error = 0;

    if (flags & UNI_FLAG_ASYNCHRONOUS) {
        uaecptr sysbase = trap_get_long(ctx, 4);
        uni.task = trap_get_long(ctx, sysbase + 276); // ThisTask

        // make sure signal bit is cleared
		trap_call_add_dreg(ctx, 0, 0);
		trap_call_add_dreg(ctx, 1, 1 << SIGBIT);
		trap_call_lib(ctx, sysbase, -0x132); // SetSignal

        // start thread if necessary
        if (!library_data->thread_id) {
            uae_sem_init (&library_data->full_count, 0, 0);
            // we don't have a queue as such, the thread only processes
            // one item at a time with a "queue size" of 1
            uae_sem_init (&library_data->empty_count, 0, 1);
            uae_start_thread (_T("uaenative"), uaenative_thread,
                              library_data, &library_data->thread_id);
        }

        // signal async thread to process new function call
        uae_sem_wait(&library_data->empty_count);
        library_data->uni = &uni;
        uae_sem_post(&library_data->full_count);

        // wait for signal
		trap_call_add_dreg(ctx, 0, 1 << SIGBIT);
		trap_call_lib(ctx, sysbase, -0x13e); // Wait
        write_log (_T("uni: -- Got async result --\n"));
    }
    else {
        // synchronous mode, just call the function here and now
        do_call_function(&uni);
    }
    return uni.result;
}

uae_u32 uaenative_close_library(TrapContext *ctx, int flags)
{
    if (!currprefs.native_code) {
        return UNI_ERROR_NOT_ENABLED;
    }

    uae_u32 handle;
    if (flags & UNI_FLAG_COMPAT) {
        handle = trap_get_dreg(ctx, 1);
    }
    else {
        handle = trap_get_areg(ctx, 1);
    }

    struct library_data *library_data = get_library_data_from_handle (handle);
    if (library_data == NULL) {
        return UNI_ERROR_INVALID_LIBRARY;
    }

    dl_close (library_data->dl_handle);

    // We now "free" the library and function entries for this library. This
    // makes the entries available for re-use. The bad thing about this is
    // that it could be possible for a buggy Amiga program to call a
    // mismatching function if a function handle is kept after the library
    // is closed.
    for (int i = 0; i <= g_max_handle; i++) {
        if (g_handles[i].library == library_data) {
            g_handles[i].library = NULL;
            g_handles[i].function = NULL;
        }
    }

    if (library_data->thread_id) {
        write_log (_T("uni: signalling uaenative_thread to stop\n"));
        library_data->thread_stop_flag = 1;
        // wake up thread so it can shut down
        uae_sem_post(&library_data->full_count);
    }
    else {
        free_library_data(library_data);
    }

    return 0;
}

// ----------------------------------------------------------------------------

typedef uae_u32 (REGPARAM2 *uae_library_trap) (TrapContext *context);

struct uae_library_trap_def {
    uae_library_trap function; // native function pointer for trap
    int flags;                 // trap flags, e.g. TRAPFLAG_EXTRA_STACK
    uaecptr aptr;              // address of trap (Amiga-side)
};

struct uae_library {
    // these members must be specified
    const TCHAR *name;
    const TCHAR *id;
    int version;
    int revision;
    uae_library_trap_def *traps;

    // these members can default to 0
    int data_size;

    // these members will be initialized by library functions
    uaecptr aptr_name;
    uaecptr aptr_id;
    uaecptr aptr_init;
    uaecptr aptr_func_table;
    uaecptr aptr_data_table;
};

static void uae_library_install (struct uae_library *library)
{
	library->aptr_name = ds (library->name);
	library->aptr_id = ds (library->id);

    for (uae_library_trap_def *t = library->traps; t->function; t++) {
        t->aptr = here ();
        calltrap (deftrap2 (t->function, t->flags, _T("")));
        dw (RTS);
    }

	library->aptr_func_table = here ();
    for (uae_library_trap_def *t = library->traps + 1; t->function; t++) {
        dl (t->aptr);
    }
	dl (0xFFFFFFFF); // end of table

	library->aptr_data_table = here ();
	dw (0xE000);     // INITBYTE
	dw (0x0008);     // LN_TYPE
	dw (0x0900);     // NT_LIBRARY
	dw (0xE000);     // INITBYTE
	dw (0x0009);     // LN_PRI
	dw (0xCE00);     // -50
	dw (0xC000);     // INITLONG
	dw (0x000A);     // LN_NAME
	dl (library->aptr_name);
	dw (0xE000);     // INITBYTE
	dw (0x000E);     // LIB_FLAGS
	dw (0x0600);     // LIBF_SUMUSED | LIBF_CHANGED
	dw (0xD000);     // INITWORD
	dw (0x0014);     // LIB_VERSION
	dw (library->version);
	dw (0xD000);
	dw (0x0016);     // LIB_REVISION
	dw (library->revision);
	dw (0xC000);
	dw (0x0018);     // LIB_IDSTRING
	dl (library->aptr_id);
	dl (0x00000000); // end of table

	library->aptr_init = here ();
	dl (SIZEOF_LIBRARY + library->data_size);
	dl (library->aptr_func_table);
	dl (library->aptr_data_table);
	dl (library->traps[0].aptr);

    write_log (_T("%s installed (%s)\n"),
               library->name, MODULE_SUFFIX);
}

static uaecptr uae_library_startup (TrapContext *ctx, uaecptr res_addr, struct uae_library *library)
{
	if (library->aptr_name == 0 || !currprefs.native_code) {
		return res_addr;
	}

	trap_put_word(ctx, res_addr + 0x00, 0x4AFC);
	trap_put_long(ctx, res_addr + 0x02, res_addr);
	trap_put_long(ctx, res_addr + 0x06, res_addr + 0x1A); // Continue scan here
	trap_put_word(ctx, res_addr + 0x0A, 0x8004);          // RTF_AUTOINIT, RT_VERSION
	trap_put_word(ctx, res_addr + 0x0C, 0x0905);          // NT_LIBRARY, RT_PRI
	trap_put_long(ctx, res_addr + 0x0E, library->aptr_name);
	trap_put_long(ctx, res_addr + 0x12, library->aptr_id);
	trap_put_long(ctx, res_addr + 0x16, library->aptr_init);

	return res_addr + 0x1A;
}

// ----------------------------------------------------------------------------

static uae_u32 REGPARAM2 lib_init (TrapContext *context)
{
	uaecptr aptr_base = trap_get_dreg(context, 0);
#if 0
	uaecptr aptr_data = aptr_base + SIZEOF_LIBRARY; // sizeof(Library)
	// our library data area, LIB_DATA_SIZE must be at least as big
	put_long (aptr_data + 0, somedata);
#endif
	return aptr_base;
}

static uae_u32 REGPARAM2 lib_open (TrapContext *context)
{
	// we could do some security checks here if only some specific Amiga
	// tasks can call us or something like that
	trap_put_word(context, trap_get_areg(context, 6) + 32,
		trap_get_word(context, trap_get_areg(context, 6) + 32) + 1);
	return trap_get_areg(context, 6);
}

static uae_u32 REGPARAM2 lib_close (TrapContext *context)
{
	trap_put_word(context, trap_get_areg(context, 6) + 32,
	          trap_get_word(context, trap_get_areg(context, 6) + 32) - 1);
	return 0;
}

static uae_u32 REGPARAM2 lib_expunge (TrapContext *context)
{
        return 0;
}

static uae_u32 REGPARAM2 lib_null (TrapContext *context)
{
        return 0;
}

static uae_u32 REGPARAM2 lib_open_library (TrapContext *context)
{
    return uaenative_open_library (context, 0);
}

static uae_u32 REGPARAM2 lib_close_library (TrapContext *context)
{
    return uaenative_close_library (context, 0);
}

static uae_u32 REGPARAM2 lib_get_function (TrapContext *context)
{
    return uaenative_get_function (context, 0);
}

static uae_u32 REGPARAM2 lib_call_function (TrapContext *context)
{
    int flags = 0;
    return uaenative_call_function (context, flags);
}

static uae_u32 REGPARAM2 lib_call_function_async (TrapContext *context)
{
    int flags = UNI_FLAG_ASYNCHRONOUS;
    return uaenative_call_function (context, flags);
}

static uae_u32 REGPARAM2 lib_call_function_by_name (TrapContext *context)
{
    int flags = UNI_FLAG_NAMED_FUNCTION;
    return uaenative_call_function (context, flags);
}

static uae_u32 REGPARAM2 lib_call_function_by_name_async (TrapContext *context)
{
    int flags = UNI_FLAG_ASYNCHRONOUS | UNI_FLAG_NAMED_FUNCTION;
    return uaenative_call_function (context, flags);
}

static uae_library_trap_def uaenative_functions[] = {
    { lib_init },
    { lib_open },
    { lib_close },
    { lib_expunge },
    { lib_null },
    { lib_open_library },
    { lib_close_library },
    { lib_get_function },
    { lib_call_function },
    { lib_call_function_async, TRAPFLAG_EXTRA_STACK },
    { lib_call_function_by_name },
    { lib_call_function_by_name_async, TRAPFLAG_EXTRA_STACK },
#ifdef AMIBERRY
    { lib_call_function, TRAPFLAG_EXTRA_STACK },
    { lib_call_function_by_name, TRAPFLAG_EXTRA_STACK },
#endif
    {nullptr },
};

static struct uae_library uaenative_library = {
    _T("uaenative.library"),
    _T("UAE Native Interface 1.0"),
    1, // version
    0, // revision
    uaenative_functions,
};

void uaenative_install (void)
{
    if (!currprefs.native_code) {
        return;
    }
    uae_library_install (&uaenative_library);
}

uaecptr uaenative_startup(TrapContext *ctx, uaecptr res_addr)
{
    if (!currprefs.native_code) {
        return res_addr;
    }
    return uae_library_startup(ctx, res_addr, &uaenative_library);
}

#endif // WITH_UAENATIVE
