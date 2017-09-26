/*
 * Common types used throughout the UAE source code
 * Copyright (C) 2014 Frode Solheim
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#ifndef UAE_TYPES_H
#define UAE_TYPES_H

/* This file is intended to be included by external libraries as well,
 * so don't pull in too much UAE-specific stuff. */

/* Define uae_ integer types. Prefer long long int for uae_x64 since we can
 * then use the %lld format specifier for both 32-bit and 64-bit instead of
 * the ugly PRIx64 macros. */

#include <stdint.h>

typedef int8_t uae_s8;
typedef uint8_t uae_u8;

typedef int16_t uae_s16;
typedef uint16_t uae_u16;

typedef int32_t uae_s32;
typedef uint32_t uae_u32;

#ifndef uae_s64
typedef long long int uae_s64;
#endif
#ifndef uae_u64
typedef unsigned long long int uae_u64;
#endif

/* Parts of the UAE/WinUAE code uses the bool type (from C++).
 * Including stdbool.h lets this be valid also when compiling with C. */

#ifndef __cplusplus
#include <stdbool.h>
#endif

/* Use uaecptr to represent 32-bit (or 24-bit) addresses into the Amiga
 * address space. This is a 32-bit unsigned int regarless of host arch. */

typedef uae_u32 uaecptr;

/* Define UAE character types. WinUAE uses TCHAR (typedef for wchar_t) for
 * many strings. FS-UAE always uses char-based strings in UTF-8 format.
 * Defining SIZEOF_TCHAR lets us determine whether to include overloaded
 * functions in some cases or not. */

typedef char uae_char;

#ifdef _WIN32_
#include <tchar.h>
#ifdef UNICODE
#define SIZEOF_TCHAR 2
#else
#define SIZEOF_TCHAR 1
#endif
#else
typedef char TCHAR;
#define SIZEOF_TCHAR 1
#endif

#ifndef _T
#if SIZEOF_TCHAR == 1
#define _T(x) x
#else
#define _T(x) Lx
#endif
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#endif /* UAE_TYPES_H */
