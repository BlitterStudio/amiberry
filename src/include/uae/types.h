/*
 * UAE core integer and character types.
 *
 * Every translation unit in the emulator gets these either directly or
 * through sysdeps.h. The uae_* names are real typedefs (never macros) and
 * their underlying types are fixed: uae_s64/uae_u64 must be long long so
 * that %lld/%llx formatting matches, and uae_s8/uae_u8/uae_char must be
 * identical to the typedefs sysdeps.h makes before including this file.
 *
 * This header is self-contained: it must compile without sysconfig.h,
 * without UAE defined and with none of the SIZEOF_* macros present, so it
 * never tests SIZEOF_* and never includes emulator headers.
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UAE_TYPES_H
#define UAE_TYPES_H

#include <cstdint>

/* TCHAR model. Amiberry uses narrow (UTF-8) strings everywhere; Windows
 * builds do not define UNICODE, so <tchar.h> provides TCHAR as char. */
#ifdef _WIN32
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

/* _T(x) must stay the single token x while TCHAR is char, so that the
 * redefinitions in sysconfig.h and floppybridge_abstract.h (both guarded
 * only by #ifndef _T or processed later without one) stay token-identical. */
#ifndef _T
#if SIZEOF_TCHAR == 1
#define _T(x) x
#else
#define _T(x) L##x
#endif
#endif

/* Fixed-width emulator types. short/int/long long are 16/32/64 bits on
 * every supported target (ILP32, LP64 and LLP64 alike). */
typedef signed char uae_s8;
typedef unsigned char uae_u8;
typedef signed short uae_s16;
typedef unsigned short uae_u16;
typedef signed int uae_s32;
typedef unsigned int uae_u32;
typedef long long uae_s64;
typedef unsigned long long uae_u64;

typedef char uae_char;

/* Amiga address (32-bit address space, even on 64-bit hosts) */
typedef uae_u32 uaecptr;

/* Emulator cycle counter (events.h) */
typedef signed long long evt_t;

/* Booleans. FALSE must be exactly 0 because mameglue.h redefines it
 * unguarded; TRUE is redefined as 1 by sysconfig.h first. */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* 64-bit literal helpers */
#define VAL64(a) a##LL
#define UVAL64(a) a##ULL

/* Win32-style sized integers used by the MAME-derived sources. MinGW/MSVC
 * already provide this group via <basetsd.h> (pulled in by <windows.h>). */
#ifndef _BASETSD_H_
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
#endif

#ifdef __cplusplus
static_assert(sizeof(uae_s8) == 1 && sizeof(uae_u8) == 1, "uae_s8/uae_u8 must be 8 bit");
static_assert(sizeof(uae_s16) == 2 && sizeof(uae_u16) == 2, "uae_s16/uae_u16 must be 16 bit");
static_assert(sizeof(uae_s32) == 4 && sizeof(uae_u32) == 4, "uae_s32/uae_u32 must be 32 bit");
static_assert(sizeof(uae_s64) == 8 && sizeof(uae_u64) == 8, "uae_s64/uae_u64 must be 64 bit");
static_assert(sizeof(evt_t) == 8, "evt_t must be 64 bit");
static_assert(SIZEOF_TCHAR == 1, "Amiberry builds use narrow TCHAR only");
#endif

#endif /* UAE_TYPES_H */
