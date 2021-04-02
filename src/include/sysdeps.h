/*
  * UAE - The Un*x Amiga Emulator
  *
  * Try to include the right system headers and get other system-specific
  * stuff right & other collected kludges.
  *
  * If you think about modifying this, think twice. Some systems rely on
  * the exact order of the #include statements. That's also the reason
  * why everything gets included unconditionally regardless of whether
  * it's actually needed by the .c file.
  *
  * Copyright 1996, 1997 Bernd Schmidt
  */
#ifndef UAE_SYSDEPS_H
#define UAE_SYSDEPS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "sysconfig.h"

#ifndef UAE
#define UAE
#endif

#ifdef __cplusplus
#include <string>
using namespace std;
#else
#include <string.h>
#include <ctype.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

#ifndef UAE
#define UAE
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
#define CPU_x86_64 1
#define CPU_64_BIT 1
#elif defined(__i386__) || defined(_M_IX86) && !defined(__arm__) && !defined(__aarch64__)
#define CPU_i386 1
#elif defined(__arm__) || defined(_M_ARM) || defined(__aarch64__)
#define CPU_arm 1
#elif defined(__powerpc__) || defined(_M_PPC)
#define CPU_powerpc 1
#else
#error unrecognized CPU type
#endif

#ifndef __STDC__
#ifndef _MSC_VER
#error "Your compiler is not ANSI. Get a real one."
#endif
#endif

#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_VALUES_H
#include <values.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_DIRENT_H
#if defined(__PSP2__) // NOT __SWITCH__
# include "psp2-dirent.h"
#else
# include <dirent.h>
#endif
#else
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#ifdef HAVE_SYS_UTIME_H
# include <sys/utime.h>
#endif

#include <errno.h>
#include <assert.h>

#ifdef __NeXT__
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IXUSR S_IEXEC
#define S_ISDIR(val) (S_IFDIR & val)
struct utimbuf
{
    time_t actime;
    time_t modtime;
};
#endif

/* If char has more then 8 bits, good night. */
typedef unsigned char uae_u8;
typedef signed char uae_s8;
typedef char uae_char;

typedef struct { uae_u8 RGB[3]; } RGB;

#include "uae/types.h"

#ifdef HAVE_STRDUP
#define my_strdup _tcsdup
#else
extern TCHAR *my_strdup (const TCHAR*s);
#endif
extern TCHAR *my_strdup_ansi (const char*);
extern void my_trim (TCHAR*);
extern TCHAR *my_strdup_trim (const TCHAR*);
extern TCHAR *au (const char*);
extern char *ua (const TCHAR*);
extern TCHAR *au_fs (const char*);
extern char *ua_fs (const TCHAR*, int);
extern char *ua_copy (char *dst, int maxlen, const TCHAR *src);
extern TCHAR *au_copy (TCHAR *dst, int maxlen, const char *src);
extern char *ua_fs_copy (char *dst, int maxlen, const TCHAR *src, int defchar);
extern TCHAR *au_fs_copy (TCHAR *dst, int maxlen, const char *src);
extern char *uutf8 (const TCHAR *s);
extern TCHAR *utf8u (const char *s);
extern void to_lower (TCHAR *s, int len);
extern void to_upper (TCHAR *s, int len);

/* We can only rely on GNU C getting enums right. Mickeysoft VSC++ is known
 * to have problems, and it's likely that other compilers choke too. */
#ifdef __GNUC__
#define ENUMDECL typedef enum
#define ENUMNAME(name) name

/* While we're here, make abort more useful.  */
#define abort() \
  do { \
    printf ("Internal error; file %s, line %d\n", __FILE__, __LINE__); \
    SDL_Quit(); \
    (abort) (); \
} while (0)
#else
#define ENUMDECL enum
#define ENUMNAME(name) ; typedef int name
#endif

/*
 * Porters to weird systems, look! This is the preferred way to get
 * filesys.c (and other stuff) running on your system. Define the
 * appropriate macros and implement wrappers in a machine-specific file.
 *
 * I guess the Mac port could use this (Ernesto?)
 */

#undef DONT_HAVE_POSIX
#undef DONT_HAVE_REAL_POSIX /* define if open+delete doesn't do what it should */
#undef DONT_HAVE_STDIO
#undef DONT_HAVE_MALLOC

#if defined AMIBERRY

#include <ctype.h>

#define FILEFLAG_DIR     0x1
#define FILEFLAG_ARCHIVE 0x2
#define FILEFLAG_WRITE   0x4
#define FILEFLAG_READ    0x8
#define FILEFLAG_EXECUTE 0x10
#define FILEFLAG_SCRIPT  0x20
#define FILEFLAG_PURE    0x40

#define REGPARAM2
#define REGPARAM3 
#define REGPARAM

#define abort() \
  do { \
    printf ("Internal error; file %s, line %d\n", __FILE__, __LINE__); \
    SDL_Quit(); \
    (abort) (); \
} while (0)

#endif

#ifdef DONT_HAVE_POSIX

#define access posixemu_access
extern int posixemu_access (const TCHAR *, int);
#define open posixemu_open
extern int posixemu_open (const TCHAR *, int, int);
#define close posixemu_close
extern void posixemu_close (int);
#define read posixemu_read
extern int posixemu_read (int, TCHAR *, int);
#define write posixemu_write
extern int posixemu_write (int, const TCHAR *, int);
#undef lseek
#define lseek posixemu_seek
extern int posixemu_seek (int, int, int);
#define stat(a,b) posixemu_stat ((a), (b))
extern int posixemu_stat (const TCHAR *, STAT *);
#define mkdir posixemu_mkdir
extern int mkdir (const TCHAR *, int);
#define rmdir posixemu_rmdir
extern int posixemu_rmdir (const TCHAR *);
#define unlink posixemu_unlink
extern int posixemu_unlink (const TCHAR *);
#define truncate posixemu_truncate
extern int posixemu_truncate (const TCHAR *, long int);
#define rename posixemu_rename
extern int posixemu_rename (const TCHAR *, const TCHAR *);
#define chmod posixemu_chmod
extern int posixemu_chmod (const TCHAR *, int);
#define tmpnam posixemu_tmpnam
extern void posixemu_tmpnam (TCHAR *);
#define utime posixemu_utime
extern int posixemu_utime (const TCHAR *, struct utimbuf *);
#define opendir posixemu_opendir
extern DIR * posixemu_opendir (const TCHAR *);
#define readdir posixemu_readdir
extern struct dirent* readdir (DIR *);
#define closedir posixemu_closedir
extern void closedir (DIR *);

/* This isn't the best place for this, but it fits reasonably well. The logic
 * is that you probably don't have POSIX errnos if you don't have the above
 * functions. */
extern long dos_errno (void);

#endif

#ifdef DONT_HAVE_STDIO

extern FILE *stdioemu_fopen (const TCHAR *, const TCHAR *);
#define fopen(a,b) stdioemu_fopen(a, b)
extern int stdioemu_fseek (FILE *, int, int);
#define fseek(a,b,c) stdioemu_fseek(a, b, c)
extern int stdioemu_fread (TCHAR *, int, int, FILE *);
#define fread(a,b,c,d) stdioemu_fread(a, b, c, d)
extern int stdioemu_fwrite (const TCHAR *, int, int, FILE *);
#define fwrite(a,b,c,d) stdioemu_fwrite(a, b, c, d)
extern int stdioemu_ftell (FILE *);
#define ftell(a) stdioemu_ftell(a)
extern int stdioemu_fclose (FILE *);
#define fclose(a) stdioemu_fclose(a)

#endif

#ifdef DONT_HAVE_MALLOC

#define malloc(a) mallocemu_malloc(a)
extern void *mallocemu_malloc (int size);
#define free(a) mallocemu_free(a)
extern void mallocemu_free (void *ptr);

#endif

#ifdef X86_ASSEMBLY
#define ASM_SYM_FOR_FUNC(a) __asm__(a)
#else
#define ASM_SYM_FOR_FUNC(a)
#endif

extern void write_log (const TCHAR *,...);
extern TCHAR console_getch(void);
extern void f_out(FILE*, const TCHAR*, ...);
extern void gui_message (const TCHAR *,...);

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef STATIC_INLINE
#ifdef DEBUG
#define STATIC_INLINE static __attribute__ ((noinline))
#define NOINLINE __attribute__ ((noinline))
#define NORETURN
#elif __GNUC__ - 1 > 1 && __GNUC_MINOR__ - 1 >= 0
#ifdef AMIBERRY
#define STATIC_INLINE static //__inline__
#else
#define STATIC_INLINE static //__inline__ __attribute__ ((always_inline))
#endif
#define NOINLINE __attribute__ ((noinline))
#define NORETURN __attribute__ ((noreturn))
#elif _MSC_VER
#define STATIC_INLINE static __forceinline
#define NOINLINE __declspec(noinline)
#define NORETURN __declspec(noreturn)
#else
#define STATIC_INLINE static
#define NOINLINE
#define NORETURN
#endif
#endif

#include "target.h"

/* Every Amiga hardware clock cycle takes this many "virtual" cycles.  This
   used to be hardcoded as 1, but using higher values allows us to time some
   stuff more precisely.
   512 is the official value from now on - it can't change, unless we want
   _another_ config option "finegrain2_m68k_speed".

   We define this value here rather than in events.h so that gencpu.c sees
   it.  */
#define CYCLE_UNIT 512

/* This one is used by cfgfile.c.  We could reduce the CYCLE_UNIT back to 1,
   I'm not 100% sure this code is bug free yet.  */
#define OFFICIAL_CYCLE_UNIT 512

/*
 * You can specify numbers from 0 to 5 here. It is possible that higher
 * numbers will make the CPU emulation slightly faster, but if the setting
 * is too high, you will run out of memory while compiling.
 * Best to leave this as it is.
 */
#define CPU_EMU_SIZE 0

/*
 * Byte-swapping functions
 */

#ifdef ARMV6_ASSEMBLY

STATIC_INLINE uae_u32 do_byteswap_32(uae_u32 v) {
  __asm__ (
	"rev %0, %0"
    : "=r" (v) : "0" (v) ); return v;
}

STATIC_INLINE uae_u32 do_byteswap_16(uae_u32 v) {
  __asm__ (
    "revsh %0, %0\n\t"
    "uxth %0, %0"
    : "=r" (v) : "0" (v) ); return v;
}
#define bswap_16(x) do_byteswap_16(x)
#define bswap_32(x) do_byteswap_32(x)

#elif defined(CPU_AARCH64)

STATIC_INLINE uae_u32 do_byteswap_32(uae_u32 v) {
  __asm__ (
	"rev %w0, %w0"
    : "=r" (v) : "0" (v) ); return v;
}

STATIC_INLINE uae_u32 do_byteswap_16(uae_u32 v) {
  __asm__ (
    "rev16 %w0, %w0\n\t"
    "uxth %w0, %w0"
    : "=r" (v) : "0" (v) ); return v;
}
#define bswap_16(x) do_byteswap_16(x)
#define bswap_32(x) do_byteswap_32(x)

#else

/* Try to use system bswap_16/bswap_32 functions. */
#if defined HAVE_BSWAP_16 && defined HAVE_BSWAP_32
# include <byteswap.h>
#  ifdef HAVE_BYTESWAP_H
#  include <byteswap.h>
# endif
#else
/* Else, if using SDL, try SDL's endian functions. */
#if defined (USE_SDL)
#include <SDL_endian.h>
#define bswap_16(x) SDL_Swap16(x)
#define bswap_32(x) SDL_Swap32(x)
#else
/* Otherwise, we'll roll our own. */
#define bswap_16(x) (((x) >> 8) | (((x) & 0xFF) << 8))
#define bswap_32(x) (((x) << 24) | (((x) << 8) & 0x00FF0000) | (((x) >> 8) & 0x0000FF00) | ((x) >> 24))
#endif
#endif

#endif /* ARMV6_ASSEMBLY*/

#ifndef __cplusplus

#define xmalloc(T, N) malloc(sizeof (T) * (N))
#define xcalloc(T, N) calloc(sizeof (T), N)
#define xfree(T) free(T)
#define xrealloc(T, TP, N) realloc(TP, sizeof (T) * (N))

#else

#define xmalloc(T, N) (static_cast<T*>(malloc (sizeof (T) * (N))))
#define xcalloc(T, N) (static_cast<T*>(calloc (sizeof (T), N)))
#define xrealloc(T, TP, N) (static_cast<T*>(realloc (TP, sizeof (T) * (N))))
#define xfree(T) free(T)

#endif

#ifdef HAVE_VAR_ATTRIBUTE_UNUSED
#define NOWARN_UNUSED(x) __attribute__((unused)) x
#else
#define NOWARN_UNUSED(x) x
#endif

#endif /* UAE_SYSDEPS_H */
