#ifndef UAE_SYSDEPS_H
#define UAE_SYSDEPS_H

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

using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

#ifdef _GCCRES_
#undef _GCCRES_
#endif

#ifdef UAE4ALL_NO_USE_RESTRICT
#define _GCCRES_
#else
#define _GCCRES_ __restrict__
#endif

#ifndef __STDC__
#error "Your compiler is not ANSI. Get a real one."
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
# include <dirent.h>
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

#if EEXIST == ENOTEMPTY
#define BROKEN_OS_PROBABLY_AIX
#endif

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

#if defined(__GNUC__) && defined(AMIGA)
/* gcc on the amiga need that __attribute((regparm)) must */
/* be defined in function prototypes as well as in        */
/* function definitions !                                 */
#define REGPARAM2 REGPARAM
#else /* not(GCC & AMIGA) */
#define REGPARAM2
#endif

/* sam: some definitions so that SAS/C can compile UAE */
#if defined(__SASC) && defined(AMIGA)
#define REGPARAM2
#define REGPARAM
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IXUSR S_IEXECUTE
#define S_ISDIR(val) (S_IFDIR & val)
#define mkdir(x,y) mkdir(x)
#define truncate(x,y) 0
#define creat(x,y) open("T:creat",O_CREAT|O_TEMP|O_RDWR) /* sam: for zfile.c */
#define strcasecmp stricmp
#define utime(file,time) 0
struct utimbuf
{
    time_t actime;
    time_t modtime;
};
#endif

#ifdef __DOS__
#include <pc.h>
#include <io.h>
#endif

/* Acorn specific stuff */
#ifdef ACORN

#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IXUSR S_IEXEC

#define strcasecmp stricmp

#endif

#ifndef L_tmpnam
#define L_tmpnam 128 /* ought to be safe */
#endif

/* If char has more then 8 bits, good night. */
typedef unsigned char uae_u8;
typedef signed char uae_s8;
typedef char uae_char;

typedef struct { uae_u8 RGB[3]; } RGB;

#if SIZEOF_SHORT == 2
typedef unsigned short uae_u16;
typedef short uae_s16;
#elif SIZEOF_INT == 2
typedef unsigned int uae_u16;
typedef int uae_s16;
#else
#error No 2 byte type, you lose.
#endif

#if SIZEOF_INT == 4
typedef unsigned int uae_u32;
typedef int uae_s32;
#elif SIZEOF_LONG == 4
typedef unsigned long uae_u32;
typedef long uae_s32;
#else
#error No 4 byte type, you lose.
#endif

typedef uae_u32 uaecptr;

#undef uae_s64
#undef uae_u64

#if SIZEOF_LONG_LONG == 8
#define uae_s64 long long
#define uae_u64 unsigned long long
#define VAL64(a) (a ## LL)
#define UVAL64(a) (a ## uLL)
#elif SIZEOF___INT64 == 8
#define uae_s64 __int64
#define uae_u64 unsigned __int64
#define VAL64(a) (a)
#define UVAL64(a) (a)
#elif SIZEOF_LONG == 8
#define uae_s64 long;
#define uae_u64 unsigned long;
#define VAL64(a) (a ## l)
#define UVAL64(a) (a ## ul)
#endif

#ifdef HAVE_STRDUP
#define my_strdup strdup
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

#if defined PANDORA

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

#ifdef UAE_CONSOLE
#undef write_log
#define write_log write_log_standard
#endif

#ifndef WITH_LOGGING
#undef write_log
#define write_log(FORMATO, RESTO...)
#define write_log_standard(FORMATO, RESTO...)
#else
extern void write_log (const TCHAR *format,...);
extern FILE *debugfile;
#endif
extern void console_out (const TCHAR *, ...);
extern void gui_message (const TCHAR *,...);

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef STATIC_INLINE
#if __GNUC__ - 1 > 1 && __GNUC_MINOR__ - 1 >= 0
#ifdef RASPBERRY
#define STATIC_INLINE static __inline__
#else
#define STATIC_INLINE static __inline__ __attribute__ ((always_inline))
#endif
#define NOINLINE __attribute__ ((noinline))
#define NORETURN __attribute__ ((noreturn))
#elif _MSC_VER
#define STATIC_INLINE static __forceinline
#define NOINLINE __declspec(noinline)
#define NORETURN __declspec(noreturn)
#else
#define STATIC_INLINE static __inline__
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

STATIC_INLINE uae_u32 do_byteswap_32(uae_u32 v) {__asm__ (
						"rev %0, %0"
                                                : "=r" (v) : "0" (v) ); return v;}

STATIC_INLINE uae_u32 do_byteswap_16(uae_u32 v) {__asm__ (
  						"revsh %0, %0\n\t"
              "uxth %0, %0"
                                                : "=r" (v) : "0" (v) ); return v;}

#endif

#define bswap_16(x) (((x) >> 8) | (((x) & 0xFF) << 8))

#endif

#ifndef __cplusplus

#define xmalloc(T, N) malloc(sizeof (T) * (N))
#define xcalloc(T, N) calloc(sizeof (T), N)
#define xfree(T) free(T)
#define xrealloc(T, TP, N) realloc(TP, sizeof (T) * (N))

#if 0
extern void *xmalloc (size_t);
extern void *xcalloc (size_t, size_t);
extern void xfree (const void*);
#endif

#else

#define xmalloc(T, N) static_cast<T*>(malloc (sizeof (T) * (N)))
#define xcalloc(T, N) static_cast<T*>(calloc (sizeof (T), N))
#define xrealloc(T, TP, N) static_cast<T*>(realloc (TP, sizeof (T) * (N)))
#define xfree(T) free(T)

#endif
