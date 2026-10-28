/*
 * UAE calling-convention and printf-format attribute macros.
 *
 * Must stay self-contained: no includes, so it can be pulled in by api.h
 * and directly by sources that only need uae_cdecl.
 *
 * Copyright (C) 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef UAE_ATTRIBUTES_H
#define UAE_ATTRIBUTES_H

#if defined(_WIN32)
/* Win32 __cdecl keyword (llvm-mingw / MSYS2 clang) */
#define uae_cdecl __cdecl
#elif defined(__GNUC__) && defined(__i386__)
/* 32-bit x86 GCC/Clang (libretro android-x86): GNU cdecl attribute */
#define uae_cdecl __attribute__((cdecl))
#else
#define uae_cdecl
#endif

#ifdef __GNUC__
/* Archetype must be plain "printf" (not gnu_printf/ms_printf) so that
 * format checking also accepts Amiga-specific %ld-style usage unchanged. */
#define UAE_PRINTF_FORMAT(f, a) __attribute__((format(printf, f, a)))
#else
#define UAE_PRINTF_FORMAT(f, a)
#endif

/* Amiberry never uses wide printf formats */
#define UAE_WPRINTF_FORMAT(f, a)

#endif /* UAE_ATTRIBUTES_H */
