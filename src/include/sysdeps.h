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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


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

#define REGPARAM2
#define REGPARAM3 

#if defined(WARPUP)
#include "devices/timer.h"
#include "osdep/posixemu.h"
#define REGPARAM
#define REGPARAM2
#define RETSIGTYPE
#define USE_ZFILE
#define strcasecmp stricmp
#define memcpy q_memcpy
#define memset q_memset
#define strdup my_strdup
// #define random rand
#define creat(x,y) open("T:creat",O_CREAT|O_RDWR|O_TRUNC,777)
extern void* q_memset(void*,int,size_t);
extern void* q_memcpy(void*,const void*,size_t);
#endif

#ifdef __DOS__
#include <pc.h>
#include <io.h>
#endif

#ifndef L_tmpnam
#define L_tmpnam 128 /* ought to be safe */
#endif


/* If char has more then 8 bits, good night. */
typedef unsigned char uae_u8;
typedef signed char uae_s8;

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
extern char *my_strdup (const char*s);
#endif

extern void *xmalloc(size_t);
extern void *xcalloc(size_t, size_t);
extern void xfree(void*);

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

#if defined(WARPUP)
#define DONT_HAVE_POSIX
#endif

#ifdef DONT_HAVE_POSIX

#define access posixemu_access
extern int posixemu_access (const char *, int);
#define open posixemu_open
extern int posixemu_open (const char *, int, int);
#define close posixemu_close
extern void posixemu_close (int);
#define read posixemu_read
extern int posixemu_read (int, char *, int);
#define write posixemu_write
extern int posixemu_write (int, const char *, int);
#undef lseek
#define lseek posixemu_seek
extern int posixemu_seek (int, int, int);
#define stat(a,b) posixemu_stat ((a), (b))
extern int posixemu_stat (const char *, STAT *);
#define mkdir posixemu_mkdir
extern int mkdir (const char *, int);
#define rmdir posixemu_rmdir
extern int posixemu_rmdir (const char *);
#define unlink posixemu_unlink
extern int posixemu_unlink (const char *);
#define truncate posixemu_truncate
extern int posixemu_truncate (const char *, long int);
#define rename posixemu_rename
extern int posixemu_rename (const char *, const char *);
#define chmod posixemu_chmod
extern int posixemu_chmod (const char *, int);
#define tmpnam posixemu_tmpnam
extern void posixemu_tmpnam (char *);
#define utime posixemu_utime
extern int posixemu_utime (const char *, struct utimbuf *);
#define opendir posixemu_opendir
extern DIR * posixemu_opendir (const char *);
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

extern FILE *stdioemu_fopen (const char *, const char *);
#define fopen(a,b) stdioemu_fopen(a, b)
extern int stdioemu_fseek (FILE *, int, int);
#define fseek(a,b,c) stdioemu_fseek(a, b, c)
extern int stdioemu_fread (char *, int, int, FILE *);
#define fread(a,b,c,d) stdioemu_fread(a, b, c, d)
extern int stdioemu_fwrite (const char *, int, int, FILE *);
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

#include "target.h"

#ifdef UAE_CONSOLE
#undef write_log
#define write_log write_log_standard
#else
#undef write_log
#define write_log(FORMATO, RESTO...)
#endif

#ifdef write_log
#undef write_log
#endif
#ifndef WITH_LOGGING
#define write_log(FORMATO, RESTO...)
#define write_log_standard(FORMATO, RESTO...)
#define console_out(FORMATO, RESTO...)
#define console_get(FORMATO, RESTO...)
#else
extern void write_log (const char *format,...);
extern FILE *debugfile;
#endif
extern void gui_message (const char *,...);

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef STATIC_INLINE
#ifdef RASPBERRY
#define STATIC_INLINE static __inline__
#else
#define STATIC_INLINE static __inline__ __attribute__ ((always_inline))
#endif
#define NOINLINE __attribute__ ((noinline))
#define NORETURN __attribute__ ((noreturn))
#endif

#else
#ifndef STATIC_INLINE
#define STATIC_INLINE static __inline__
#endif
#define ENUMDECL enum
#define ENUMNAME(name) ; typedef int name
#endif

#ifdef ARMV6_ASSEMBLY

static inline uae_u32 do_byteswap_32(uae_u32 v) {__asm__ (
						"rev %0, %0"
                                                : "=r" (v) : "0" (v) ); return v;}

static inline uae_u32 do_byteswap_16(uae_u32 v) {__asm__ (
  						"revsh %0, %0\n\t"
              "uxth %0, %0"
                                                : "=r" (v) : "0" (v) ); return v;}

#endif

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

#undef REGPARAM
#define REGPARAM

#define FILEFLAG_DIR     0x1
#define FILEFLAG_ARCHIVE 0x2
#define FILEFLAG_WRITE   0x4
#define FILEFLAG_READ    0x8
#define FILEFLAG_EXECUTE 0x10
#define FILEFLAG_SCRIPT  0x20
#define FILEFLAG_PURE    0x40

#endif
