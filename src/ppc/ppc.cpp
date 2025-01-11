
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "threaddep/thread.h"
#include "machdep/rpt.h"
#include "memory.h"
#include "cpuboard.h"
#include "debug.h"
#include "custom.h"
#include "uae.h"
#include "gui.h"
#include "autoconf.h"
#include "uae/dlopen.h"
#include "uae/log.h"
#include "uae/ppc.h"
#include "uae/qemu.h"
#include "devices.h"

#define SPINLOCK_DEBUG 0
#define PPC_ACCESS_LOG 0

#define PPC_DEBUG_ADDR_FROM 0x000000
#define PPC_DEBUG_ADDR_TO   0xffffff

#ifdef WITH_PEARPC_CPU
#include "pearpc/cpu/cpu.h"
#include "pearpc/io/io.h"
#include "pearpc/cpu/cpu_generic/ppc_cpu.h"
#endif

#define TRACE(format, ...) write_log(_T("PPC: ") format, ## __VA_ARGS__)

#ifdef WIN32
#define WIN32_SPINLOCK
#endif

#if SPINLOCK_DEBUG
static volatile int spinlock_cnt;
#endif

static volatile bool ppc_spinlock_waiting;

#ifdef WIN32_SPINLOCK
#define CRITICAL_SECTION_SPIN_COUNT 5000
static CRITICAL_SECTION ppc_cs1, ppc_cs2;
static bool ppc_cs_initialized;
#else
static SDL_mutex* ppc_mutex, *ppc_mutex2;
#endif

void uae_ppc_spinlock_get(void)
{
#ifdef WIN32_SPINLOCK
	EnterCriticalSection(&ppc_cs2);
	ppc_spinlock_waiting = true;
	EnterCriticalSection(&ppc_cs1);
	ppc_spinlock_waiting = false;
	LeaveCriticalSection(&ppc_cs2);
#else
	SDL_LockMutex(ppc_mutex2);
	ppc_spinlock_waiting = true;
	SDL_LockMutex(ppc_mutex);
	ppc_spinlock_waiting = false;
	SDL_UnlockMutex(ppc_mutex2);
#endif
#if SPINLOCK_DEBUG
	if (spinlock_cnt != 0)
		write_log(_T("uae_ppc_spinlock_get %d!\n"), spinlock_cnt);
	spinlock_cnt++;
#endif
}

void uae_ppc_spinlock_release(void)
{
#if SPINLOCK_DEBUG
	if (spinlock_cnt != 1)
		write_log(_T("uae_ppc_spinlock_release %d!\n"), spinlock_cnt);
	spinlock_cnt--;
#endif
#ifdef WIN32_SPINLOCK
	LeaveCriticalSection(&ppc_cs1);
#else
	SDL_UnlockMutex(ppc_mutex);
#endif
}

static void uae_ppc_spinlock_create(void)
{
#ifdef WIN32_SPINLOCK
#if SPINLOCK_DEBUG
	write_log(_T("uae_ppc_spinlock_create\n"));
#endif
	if (ppc_cs_initialized) {
		DeleteCriticalSection(&ppc_cs1);
		DeleteCriticalSection(&ppc_cs2);
	}
	InitializeCriticalSectionAndSpinCount(&ppc_cs1, CRITICAL_SECTION_SPIN_COUNT);
	InitializeCriticalSectionAndSpinCount(&ppc_cs2, CRITICAL_SECTION_SPIN_COUNT);
#if SPINLOCK_DEBUG
	spinlock_cnt = 0;
#endif
	ppc_cs_initialized = true;
#endif
}

volatile int ppc_state;
static volatile bool ppc_thread_running;
int ppc_cycle_count;
static volatile bool ppc_access;
static volatile int ppc_cpu_lock_state;
static bool ppc_init_done;
static int ppc_implementation;
static bool ppc_paused;

#define CSPPC_PVR 0x00090204
#define BLIZZPPC_PVR 0x00070101

#define KB * 1024
#define MB * (1024 * 1024)

/* Dummy PPC implementation */

static void PPCCALL dummy_ppc_cpu_close(void) { }
static void PPCCALL dummy_ppc_cpu_stop(void) { }
static void PPCCALL dummy_ppc_cpu_atomic_raise_ext_exception(void) { }
static void PPCCALL dummy_ppc_cpu_atomic_cancel_ext_exception(void) { }
static void PPCCALL dummy_ppc_cpu_map_memory(PPCMemoryRegion *regions, int count) { }
static void PPCCALL dummy_ppc_cpu_set_pc(int cpu, uint32_t value) { }
static void PPCCALL dummy_ppc_cpu_run_continuous(void) { }
static void PPCCALL dummy_ppc_cpu_run_single(int count) { }
//static uint64_t PPCCALL dummy_ppc_cpu_get_dec(void) { return 0; }
//static void PPCCALL dummy_ppc_cpu_do_dec(int value) { }

static void PPCCALL dummy_ppc_cpu_pause(int pause)
{
    //UAE_LOG_STUB("pause=%d\n", pause)
}

/* Functions typedefs for PPC implementation */

typedef bool (PPCCALL *ppc_cpu_init_function)(const char *model, uint32_t hid1);
typedef bool (PPCCALL *ppc_cpu_init_pvr_function)(uint32_t pvr);
typedef void (PPCCALL *ppc_cpu_close_function)(void);
typedef void (PPCCALL *ppc_cpu_stop_function)(void);
typedef void (PPCCALL *ppc_cpu_atomic_raise_ext_exception_function)(void);
typedef void (PPCCALL *ppc_cpu_atomic_cancel_ext_exception_function)(void);
typedef void (PPCCALL *ppc_cpu_map_memory_function)(PPCMemoryRegion *regions, int count);
typedef void (PPCCALL *ppc_cpu_set_pc_function)(int cpu, uint32_t value);
typedef void (PPCCALL *ppc_cpu_run_continuous_function)(void);
typedef void (PPCCALL *ppc_cpu_run_single_function)(int count);
typedef uint64_t (PPCCALL *ppc_cpu_get_dec_function)(void);
typedef void (PPCCALL *ppc_cpu_do_dec_function)(int value);
typedef void (PPCCALL *ppc_cpu_pause_function)(int pause);
typedef bool (PPCCALL *ppc_cpu_check_state_function)(int state);
typedef void (PPCCALL *ppc_cpu_set_state_function)(int state);
typedef void (PPCCALL *ppc_cpu_reset_function)(void);

/* Function pointers to active PPC implementation */

static struct impl {
	/* Common */
	ppc_cpu_atomic_raise_ext_exception_function atomic_raise_ext_exception;
	ppc_cpu_atomic_cancel_ext_exception_function atomic_cancel_ext_exception;

	/* PearPC */
	ppc_cpu_run_continuous_function run_continuous;
	ppc_cpu_init_pvr_function init_pvr;
	ppc_cpu_pause_function pause;
	ppc_cpu_close_function close;
	ppc_cpu_stop_function stop;
	ppc_cpu_set_pc_function set_pc;
	ppc_cpu_run_single_function run_single;
	ppc_cpu_get_dec_function get_dec;
	ppc_cpu_do_dec_function do_dec;

	/* QEMU */
	ppc_cpu_init_function init;
	ppc_cpu_map_memory_function map_memory;
	ppc_cpu_check_state_function check_state;
	ppc_cpu_set_state_function set_state;
	ppc_cpu_reset_function reset;
	qemu_uae_ppc_in_cpu_thread_function in_cpu_thread;
	qemu_uae_ppc_external_interrupt_function external_interrupt;
	qemu_uae_lock_function lock;

} impl;

static void load_dummy_implementation(void)
{
	write_log(_T("PPC: Loading dummy implementation\n"));
	memset(&impl, 0, sizeof(impl));
	impl.close = dummy_ppc_cpu_close;
	impl.stop = dummy_ppc_cpu_stop;
	impl.atomic_raise_ext_exception = dummy_ppc_cpu_atomic_raise_ext_exception;
	impl.atomic_cancel_ext_exception = dummy_ppc_cpu_atomic_cancel_ext_exception;
	impl.map_memory = dummy_ppc_cpu_map_memory;
	impl.set_pc = dummy_ppc_cpu_set_pc;
	impl.run_continuous = dummy_ppc_cpu_run_continuous;
	impl.run_single = dummy_ppc_cpu_run_single;
	impl.pause = dummy_ppc_cpu_pause;
}

static void uae_patch_library_ppc(UAE_DLHANDLE handle)
{
	void *ptr;

	ptr = uae_dlsym(handle, "uae_ppc_io_mem_read");
	if (ptr) *((uae_ppc_io_mem_read_function *) ptr) = &uae_ppc_io_mem_read;
	else write_log(_T("WARNING: uae_ppc_io_mem_read not set\n"));

	ptr = uae_dlsym(handle, "uae_ppc_io_mem_write");
	if (ptr) *((uae_ppc_io_mem_write_function *) ptr) = &uae_ppc_io_mem_write;
	else write_log(_T("WARNING: uae_ppc_io_mem_write not set\n"));

	ptr = uae_dlsym(handle, "uae_ppc_io_mem_read64");
	if (ptr) *((uae_ppc_io_mem_read64_function *) ptr) = &uae_ppc_io_mem_read64;
	else write_log(_T("WARNING: uae_ppc_io_mem_read64 not set\n"));

	ptr = uae_dlsym(handle, "uae_ppc_io_mem_write64");
	if (ptr) *((uae_ppc_io_mem_write64_function *) ptr) = &uae_ppc_io_mem_write64;
	else write_log(_T("WARNING: uae_ppc_io_mem_write64 not set\n"));
}

static bool load_qemu_implementation(void)
{
#ifdef WITH_QEMU_CPU
	write_log(_T("PPC: Loading QEmu implementation\n"));
	memset(&impl, 0, sizeof(impl));

	UAE_DLHANDLE handle = uae_qemu_uae_init();
	if (!handle) {
		notify_user (NUMSG_NO_PPC);
		return false;
	}
	write_log(_T("PPC: Loaded qemu-uae library at %p\n"), handle);

	/* Retrieve function pointers from library */

	impl.init = (ppc_cpu_init_function) uae_dlsym(handle, "ppc_cpu_init");
	//impl.free = (ppc_cpu_free_function) uae_dlsym(handle, "ppc_cpu_free");
	//impl.stop = (ppc_cpu_stop_function) uae_dlsym(handle, "ppc_cpu_stop");
	impl.external_interrupt = (qemu_uae_ppc_external_interrupt_function) uae_dlsym(handle, "qemu_uae_ppc_external_interrupt");
	impl.map_memory = (ppc_cpu_map_memory_function) uae_dlsym(handle, "ppc_cpu_map_memory");
	impl.run_continuous = (ppc_cpu_run_continuous_function) uae_dlsym(handle, "ppc_cpu_run_continuous");
	impl.check_state = (ppc_cpu_check_state_function) uae_dlsym(handle, "ppc_cpu_check_state");
	impl.set_state = (ppc_cpu_set_state_function) uae_dlsym(handle, "ppc_cpu_set_state");
	impl.reset = (ppc_cpu_reset_function) uae_dlsym(handle, "ppc_cpu_reset");
	impl.in_cpu_thread = (qemu_uae_ppc_in_cpu_thread_function) uae_dlsym(handle, "qemu_uae_ppc_in_cpu_thread");
	impl.lock = (qemu_uae_lock_function) uae_dlsym(handle, "qemu_uae_lock");

	// FIXME: not needed, handled internally by uae_dlopen_plugin
	// uae_dlopen_patch_common(handle);

	uae_patch_library_ppc(handle);
	return true;
#else
	return false;
#endif
}

static bool load_pearpc_implementation(void)
{
#ifdef WITH_PEARPC_CPU
	write_log(_T("PPC: Loading PearPC implementation\n"));
	memset(&impl, 0, sizeof(impl));

	impl.init_pvr = ppc_cpu_init;
	impl.close = ppc_cpu_close;
	impl.stop = ppc_cpu_stop;
	impl.atomic_raise_ext_exception = ppc_cpu_atomic_raise_ext_exception;
	impl.atomic_cancel_ext_exception = ppc_cpu_atomic_cancel_ext_exception;
	impl.set_pc = ppc_cpu_set_pc;
	impl.run_continuous = ppc_cpu_run_continuous;
	impl.run_single = ppc_cpu_run_single;
	impl.get_dec = ppc_cpu_get_dec;
	impl.do_dec = ppc_cpu_do_dec;
	return true;
#else
	return false;
#endif
}

static void load_ppc_implementation(void)
{
	int impl = currprefs.ppc_implementation;
	if (impl == PPC_IMPLEMENTATION_AUTO || impl == PPC_IMPLEMENTATION_QEMU) {
		if (load_qemu_implementation()) {
			ppc_implementation = PPC_IMPLEMENTATION_QEMU;
			return;
		}
	}
	if (impl == PPC_IMPLEMENTATION_AUTO || impl == PPC_IMPLEMENTATION_PEARPC) {
		if (load_pearpc_implementation()) {
			ppc_implementation = PPC_IMPLEMENTATION_PEARPC;
			return;
		}
	}
	load_dummy_implementation();
	ppc_implementation = PPC_IMPLEMENTATION_DUMMY;
}

static bool using_qemu(void)
{
	return ppc_implementation == PPC_IMPLEMENTATION_QEMU;
}

static bool using_pearpc(void)
{
	return ppc_implementation == PPC_IMPLEMENTATION_PEARPC;
}

enum PPCLockMethod {
	PPC_RELEASE_SPINLOCK,
	PPC_KEEP_SPINLOCK,
};

enum PPCLockStatus {
	PPC_NO_LOCK_NEEDED,
	PPC_LOCKED,
	PPC_LOCKED_WITHOUT_SPINLOCK,
};

static PPCLockStatus get_ppc_lock(PPCLockMethod method)
{
	if (impl.in_cpu_thread()) {
		return PPC_NO_LOCK_NEEDED;
	} else if (method == PPC_RELEASE_SPINLOCK) {

		uae_ppc_spinlock_release();
		impl.lock(QEMU_UAE_LOCK_ACQUIRE);
		return PPC_LOCKED_WITHOUT_SPINLOCK;

	} else if (method == PPC_KEEP_SPINLOCK) {

		bool trylock_called = false;
		while (true) {
			if (ppc_spinlock_waiting) {
				/* PPC CPU is waiting for the spinlock and the UAE side
				 * owns the spinlock - no additional locking needed */
				if (trylock_called) {
					impl.lock(QEMU_UAE_LOCK_TRYLOCK_CANCEL);
				}
				return PPC_NO_LOCK_NEEDED;
			}
			int error = impl.lock(QEMU_UAE_LOCK_TRYLOCK);
			if (error == 0) {
				/* Lock succeeded */
				return PPC_LOCKED;
			}
			trylock_called = true;
		}
	} else {
		write_log("?\n");
		return PPC_NO_LOCK_NEEDED;
	}
}

static void release_ppc_lock(PPCLockStatus status)
{
	if (status == PPC_NO_LOCK_NEEDED) {
		return;
	} else if (status == PPC_LOCKED_WITHOUT_SPINLOCK) {
		impl.lock(QEMU_UAE_LOCK_RELEASE);
		uae_ppc_spinlock_get();
	} else if (status == PPC_LOCKED) {
		impl.lock(QEMU_UAE_LOCK_RELEASE);
	}
}

static void initialize(void)
{
	static bool initialized = false;
	if (initialized) {
		return;
	}

	initialized = true;

	load_ppc_implementation();

	uae_ppc_spinlock_create();
	/* Grab the lock for the first time. This lock will be released
	 * by the UAE emulation thread when the PPC CPU can do I/O. */
	uae_ppc_spinlock_get();
}

static void map_banks(void)
{
	if (impl.map_memory == NULL) {
		return;
	}
	/*
	 * Use NULL for memory to get callbacks for read/write. Use real
	 * memory address for direct access to RAM banks (looks like this
	 * is needed by JIT, or at least more work is needed on QEmu Side
	 * to allow all memory access to go via callbacks).
	 */

	PPCMemoryRegion regions[UAE_MEMORY_REGIONS_MAX];
	UaeMemoryMap map;
#ifdef DEBUGGER
	uae_memory_map(&map);
#endif

	for (int i = 0; i < map.num_regions; i++) {
		UaeMemoryRegion *r = &map.regions[i];
		PPCMemoryRegion *pr = &regions[i];
		pr->start = r->start;
		pr->size = r->size;
		pr->name = ua(r->name);
		pr->alias = r->alias;
		pr->memory = r->memory;
	}

	if (impl.in_cpu_thread && impl.in_cpu_thread() == false) {
		uae_ppc_spinlock_release();
	}
	impl.map_memory(regions, map.num_regions);
	if (impl.in_cpu_thread && impl.in_cpu_thread() == false) {
		uae_ppc_spinlock_get();
	}

	for (int i = 0; i < map.num_regions; i++) {
		free(regions[i].name);
	}
}

static void set_and_wait_for_state(int state, int unlock)
{
	if (state == PPC_CPU_STATE_PAUSED) {
		if (ppc_paused)
			return;
		ppc_paused = true;
	} else if (state == PPC_CPU_STATE_RUNNING) {
		if (!ppc_paused)
			return;
		ppc_paused = false;
	} else {
		return;
	}
	if (using_qemu()) {
		if (impl.in_cpu_thread() == false) {
			uae_ppc_spinlock_release();
		}
		impl.set_state(state);
		if (impl.in_cpu_thread() == false) {
			uae_ppc_spinlock_get();
		}
	}
}

bool uae_self_is_ppc(void)
{
	if (ppc_state == PPC_STATE_INACTIVE)
		return false;
	return impl.in_cpu_thread();
}

void uae_ppc_wakeup_main(void)
{
	if (uae_self_is_ppc()) {
		sleep_cpu_wakeup();
	}
}

static void ppc_map_region(PPCMemoryRegion *r, bool dolock)
{
	if (dolock && impl.in_cpu_thread() == false) {
		/* map_memory will acquire the qemu global lock, so we must ensure
		* the PPC CPU can finish any I/O requests and release the lock. */
		uae_ppc_spinlock_release();
	}
	impl.map_memory(r, -1);
	if (dolock && impl.in_cpu_thread() == false) {
		uae_ppc_spinlock_get();
	}
	free((void*)r->name);
}

static void ppc_map_banks2(uae_u32 start, uae_u32 size, const TCHAR *name, void *addr, bool remove, bool direct, bool dolock)
{
	if (ppc_state == PPC_STATE_INACTIVE || !impl.map_memory)
		return;

	PPCMemoryRegion r;
	r.start = start;
	r.size = size;
	r.name = ua(name);
	r.alias = remove ? 0xffffffff : 0;
	r.memory = addr;

	if (r.start == rtarea_base && rtarea_base && direct) {
		// Map first half directly, it contains code only.
		// Second half has dynamic data, it must not be direct mapped.
		r.memory = rtarea_bank.baseaddr;
		r.size = RTAREA_DATAREGION;
		ppc_map_region(&r, dolock);
		r.start = start + RTAREA_DATAREGION;
		r.size = size - RTAREA_DATAREGION;
		r.name = ua(name);
		r.alias = remove ? 0xffffffff : 0;
		r.memory = NULL;
	} else if (r.start == uaeboard_base && uaeboard_base && direct) {
		// Opposite here, first half indirect, second half direct.
		r.memory = NULL;
		r.size = UAEBOARD_DATAREGION_START;
		ppc_map_region(&r, dolock);
		r.start = start + UAEBOARD_DATAREGION_START;
		r.size = UAEBOARD_DATAREGION_SIZE;
		r.name = ua(name);
		r.alias = remove ? 0xffffffff : 0;
		r.memory = uaeboard_bank.baseaddr + UAEBOARD_DATAREGION_START;
	}
	ppc_map_region(&r, dolock);
}
void ppc_map_banks(uae_u32 start, uae_u32 size, const TCHAR *name, void *addr, bool remove)
{
	ppc_map_banks2(start, size, name, addr, remove, true, true);
}

void ppc_remap_bank(uae_u32 start, uae_u32 size, const TCHAR *name, void *addr)
{
	if (ppc_state == PPC_STATE_INACTIVE || !impl.map_memory)
		return;

	if (impl.in_cpu_thread() == false) {
		uae_ppc_spinlock_release();
	}
	ppc_map_banks2(start, size, name, addr, true, false, false);
	ppc_map_banks2(start, size, name, addr, false, true, false);
	if (impl.in_cpu_thread() == false) {
		uae_ppc_spinlock_get();
	}

}

void uae_ppc_get_model(const TCHAR **model, uint32_t *hid1)
{
	if (currprefs.ppc_model[0]) {
		/* Override PPC CPU model. See qemu/target-ppc/cpu-models.c for
		 * a list of valid CPU model identifiers */
		*model = currprefs.ppc_model;
	} else {
		/* Set default CPU model based on accelerator board */
		if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC)) {
			*model = _T("603ev");
		} else {
			*model = _T("604e");
		}
	}
	if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC)) {
		*hid1 = 0xc0000000; // 4x
	} else {
		*hid1 = 0xa0000000; // 4x
	}
}

static void cpu_init(void)
{
	if (using_pearpc()) {
		const TCHAR *model;
		uint32_t hid1;
		uae_ppc_get_model(&model, &hid1);
		uint32_t pvr = 0;
		if (_tcsicmp(model, _T("603ev")) == 0) {
			pvr = BLIZZPPC_PVR;
		} else if (_tcsicmp(model, _T("604e")) == 0) {
			pvr = CSPPC_PVR;
		} else {
			pvr = CSPPC_PVR;
			write_log(_T("PPC: Unrecognized model \"%s\", using PVR 0x%08x\n"), model, pvr);
		}
		write_log(_T("PPC: Calling ppc_cpu_init with PVR 0x%08x\n"), pvr);
		impl.init_pvr(pvr);
		return;
	}

	/* QEMU: CPU has already been initialized by qemu_uae_init */
}

static void uae_ppc_cpu_reset(void)
{
	TRACE(_T("uae_ppc_cpu_reset\n"));

	initialize();

	static bool ppc_cpu_init_done;
	if (!ppc_cpu_init_done) {
		write_log(_T("PPC: Hard reset\n"));
		cpu_init();
		ppc_cpu_init_done = true;
	}

	/* Map memory and I/O banks (for QEmu PPC implementation) */
	map_banks();

	if (using_qemu()) {
		impl.reset();
	} else if (using_pearpc()) {
		write_log(_T("PPC: Init\n"));
		impl.set_pc(0, 0xfff00100);
		ppc_cycle_count = 2000;
	}

	ppc_cpu_lock_state = 0;
	ppc_state = PPC_STATE_ACTIVE;
}

static int ppc_thread(void *v)
{
	if (using_qemu()) {
		write_log(_T("PPC: Warning - ppc_thread started with QEMU impl\n"));
	} else {
		uae_ppc_cpu_reset();
		impl.run_continuous();

		if (ppc_state == PPC_STATE_ACTIVE || ppc_state == PPC_STATE_SLEEP)
			ppc_state = PPC_STATE_STOP;
		write_log(_T("ppc_cpu_run() exited.\n"));
		ppc_thread_running = false;
	}
	return 0;
}

void uae_ppc_execute_check(void)
{
	if (ppc_spinlock_waiting) {
		uae_ppc_spinlock_release();
		uae_ppc_spinlock_get();
	}
}

void uae_ppc_execute_quick()
{
	uae_ppc_spinlock_release();
	sleep_millis_main(1);
	uae_ppc_spinlock_get();
}

void uae_ppc_emulate(void)
{
	if (using_pearpc()) {
		ppc_interrupt(intlev());
		if (ppc_state == PPC_STATE_ACTIVE || ppc_state == PPC_STATE_SLEEP)
			impl.run_single(10);
	}
}

bool uae_ppc_direct_physical_memory_handle(uint32_t addr, uint8_t *&ptr)
{
	ptr = get_real_address(addr);
	if (!ptr)
		gui_message(_T("Executing PPC code at IO address %08x!"), addr);
	return true;
}

STATIC_INLINE bool spinlock_pre(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if ((ab->flags & ABFLAG_THREADSAFE) == 0) {
		sleep_cpu_wakeup();
		uae_ppc_spinlock_get();
		return true;
	}
	return false;
}

STATIC_INLINE void spinlock_post(bool locked)
{
	if (locked)
		uae_ppc_spinlock_release();
}

bool UAECALL uae_ppc_io_mem_write(uint32_t addr, uint32_t data, int size)
{
	bool locked = false;

	while (ppc_thread_running && ppc_cpu_lock_state < 0 && ppc_state);

#if PPC_ACCESS_LOG > 0 && PPC_ACCESS_LOG < 2
	if (!valid_address(addr, size)) {
		if (addr >= PPC_DEBUG_ADDR_FROM && addr < PPC_DEBUG_ADDR_TO)
			write_log(_T("PPC io write %08x = %08x %d\n"), addr, data, size);
	}
#endif

	locked = spinlock_pre(addr);
	switch (size)
	{
	case 4:
		put_long(addr, data);
		break;
	case 2:
		put_word(addr, data);
		break;
	case 1:
		put_byte(addr, data);
		break;
	}

	if (addr >= 0xdff000 && addr < 0xe00000) {
		int reg = addr & 0x1fe;
		switch (reg) {
			case 0x09c: // INTREQ
			case 0x09a: // INTENA
			if (data & 0x8000) {
				// possible interrupt change:
				// make sure M68K thread reacts to it ASAP.
				atomic_or(&uae_int_requested, 0x010000);
			}
			break;
		}
	}

	spinlock_post(locked);

#if PPC_ACCESS_LOG >= 2
	write_log(_T("PPC write %08x = %08x %d\n"), addr, data, size);
#endif

	return true;
}

bool UAECALL uae_ppc_io_mem_read(uint32_t addr, uint32_t *data, int size)
{
	uint32_t v;
	bool locked = false;

	while (ppc_thread_running && ppc_cpu_lock_state < 0 && ppc_state);

	if (addr >= 0xdff000 && addr < 0xe00000) {
		int reg = addr & 0x1fe;
		// shortcuts for common registers
		switch (reg) {
			case 0x01c: // INTENAR
			*data = intena;
			return true;
			case 0x01e: // INTREQR
			*data = intreq;
			return true;
		}
	}

	locked = spinlock_pre(addr);
	switch (size)
	{
	case 4:
		v = get_long(addr);
		break;
	case 2:
		v = get_word(addr);
		break;
	case 1:
		v = get_byte(addr);
		break;
	}
	*data = v;
	spinlock_post(locked);

#if PPC_ACCESS_LOG > 0 && PPC_ACCESS_LOG < 2
	if (!valid_address(addr, size)) {
		if (addr >= PPC_DEBUG_ADDR_FROM && addr < PPC_DEBUG_ADDR_TO && addr != 0xdff006)
			write_log(_T("PPC io read %08x=%08x %d\n"), addr, v, size);
	}
#endif
#if PPC_ACCESS_LOG >= 2
	if (addr < 0xb00000 || addr > 0xc00000)
		write_log(_T("PPC read %08x=%08x %d\n"), addr, v, size);
#endif
	return true;
}

bool UAECALL uae_ppc_io_mem_write64(uint32_t addr, uint64_t data)
{
	bool locked = false;

	while (ppc_thread_running && ppc_cpu_lock_state < 0 && ppc_state);

	locked = spinlock_pre(addr);
	put_long(addr + 0, data >> 32);
	put_long(addr + 4, data & 0xffffffff);
	spinlock_post(locked);

#if PPC_ACCESS_LOG >= 2
	write_log(_T("PPC mem write64 %08x = %08llx\n"), addr, data);
#endif

	return true;
}

bool UAECALL uae_ppc_io_mem_read64(uint32_t addr, uint64_t *data)
{
	bool locked = false;
	uint32_t v1, v2;

	while (ppc_thread_running && ppc_cpu_lock_state < 0 && ppc_state);

	locked = spinlock_pre(addr);
	v1 = get_long(addr + 0);
	v2 = get_long(addr + 4);
	*data = ((uint64_t)v1 << 32) | v2;
	spinlock_post(locked);

#if PPC_ACCESS_LOG >= 2
	write_log(_T("PPC mem read64 %08x = %08llx\n"), addr, *data);
#endif

	return true;
}

static void uae_ppc_hsync_handler(void)
{
	if (ppc_state == PPC_STATE_INACTIVE)
		return;
	if (using_pearpc()) {
		if (ppc_state != PPC_STATE_SLEEP)
			return;
		if (impl.get_dec() == 0) {
			uae_ppc_wakeup();
		} else {
			impl.do_dec(ppc_cycle_count);
		}
	}
}

void uae_ppc_cpu_stop(void)
{
	if (ppc_state == PPC_STATE_INACTIVE)
		return;
	TRACE(_T("uae_ppc_cpu_stop %d %d\n"), ppc_thread_running, ppc_state);
	if (using_qemu()) {
		write_log(_T("PPC: Stopping...\n"));
		set_and_wait_for_state(PPC_CPU_STATE_PAUSED, 1);
		write_log(_T("PPC: Stopped\n"));
	} else if (using_pearpc()) {
		if (ppc_thread_running && ppc_state) {
			write_log(_T("PPC: Stopping...\n"));
			impl.stop();
			while (ppc_state != PPC_STATE_STOP && ppc_state != PPC_STATE_CRASH) {
				uae_ppc_wakeup();
				uae_ppc_spinlock_release();
				uae_ppc_spinlock_get();
			}
			write_log(_T("PPC: Stopped\n"));
		}
	}
	ppc_state = PPC_STATE_INACTIVE;
}

void uae_ppc_cpu_reboot(void)
{
	TRACE(_T("uae_ppc_cpu_reboot\n"));

	initialize();

	device_add_hsync(uae_ppc_hsync_handler);

	if (!ppc_thread_running) {
		write_log(_T("Starting PPC thread.\n"));
		ppc_thread_running = true;
		if (using_qemu()) {
			uae_ppc_cpu_reset();
			//qemu_uae_ppc_start();
			impl.set_state(PPC_CPU_STATE_RUNNING);
			//set_and_wait_for_state(PPC_CPU_STATE_RUNNING, 0);
		} else {
			uae_start_thread(NULL, ppc_thread, NULL, NULL);
		}
	} else if (using_qemu()) {
		write_log(_T("PPC: Thread already running, resetting\n"));
		uae_ppc_cpu_reset();
		set_and_wait_for_state(PPC_CPU_STATE_RUNNING, 1);
	}
}

void uae_ppc_reset(bool hardreset)
{
	if (!currprefs.ppc_mode)
		return;
	TRACE(_T("uae_ppc_reset hardreset=%d\n"), hardreset);
	if (using_qemu()) {
	    set_and_wait_for_state(PPC_CPU_STATE_PAUSED, 1);
	} else if (using_pearpc()) {
		uae_ppc_cpu_stop();
		if (hardreset) {
			if (ppc_init_done)
				impl.close();
			ppc_init_done = false;
		}
	}
	ppc_state = PPC_STATE_INACTIVE;
}

void uae_ppc_free(void)
{
	bool wasactive = ppc_state != PPC_STATE_INACTIVE;
	uae_ppc_cpu_stop();
	if (wasactive && impl.map_memory)
		impl.map_memory(NULL, 0);
}

void uae_ppc_cpu_lock(void)
{
	// when called, lock was already set by other CPU
	if (ppc_access) {
		// ppc accessing but m68k already locked
		ppc_cpu_lock_state = -1;
	} else {
		// m68k accessing but ppc already locked
		ppc_cpu_lock_state = 1;
	}
}

bool uae_ppc_cpu_unlock(void)
{
	if (!ppc_cpu_lock_state)
		return true;
	ppc_cpu_lock_state = 0;
	return false;
}

void uae_ppc_wakeup(void)
{
	if (ppc_state == PPC_STATE_SLEEP)
		ppc_state = PPC_STATE_ACTIVE;
}

void uae_ppc_interrupt(bool active)
{
	if (using_pearpc()) {
		if (active) {
			impl.atomic_raise_ext_exception();
			uae_ppc_wakeup();
		} else {
			impl.atomic_cancel_ext_exception();
		}
		return;
	}

	PPCLockStatus status = get_ppc_lock(PPC_KEEP_SPINLOCK);
	impl.external_interrupt(active);
	release_ppc_lock(status);
}

// sleep until interrupt (or PPC stopped)
void uae_ppc_doze(void)
{
	//TRACE(_T("uae_ppc_doze\n"));
	if (!ppc_thread_running)
		return;
	ppc_state = PPC_STATE_SLEEP;
	while (ppc_state == PPC_STATE_SLEEP) {
		sleep_millis(2);
	}
}

void uae_ppc_crash(void)
{
	TRACE(_T("uae_ppc_crash\n"));
	ppc_state = PPC_STATE_CRASH;
	if (impl.stop) {
		impl.stop();
	}
}

void uae_ppc_pause(int pause)
{
	if (ppc_state == PPC_STATE_INACTIVE)
		return;
	// FIXME: assert(uae_is_emulation_thread())
	if (using_qemu()) {
		if (pause) {
			set_and_wait_for_state(PPC_CPU_STATE_PAUSED, 1);
		} else {
			set_and_wait_for_state(PPC_CPU_STATE_RUNNING, 1);
		}
	}
#if 0
	else if (impl.pause) {
		impl.pause(pause);
	}
#endif
}
