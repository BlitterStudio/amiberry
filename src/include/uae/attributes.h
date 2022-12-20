/*
 * Defines useful functions and variable attributes for UAE
 * Copyright (C) 2014 Frode Solheim
 *
 * Licensed under the terms of the GNU General Public License version 2.
 * See the file 'COPYING' for full license text.
 */

#ifndef UAE_ATTRIBUTES_H
#define UAE_ATTRIBUTES_H

/* This file is intended to be included by external libraries as well,
 * so don't pull in too much UAE-specific stuff. */

#ifdef _WIN32
#define uae_cdecl __cdecl
#elif defined(__GNUC__) && defined(__i386__)
#define uae_cdecl __attribute__((cdecl))
#else
#define uae_cdecl
#endif

/* This attribute allows (some) compiles to emit warnings when incorrect
 * arguments are used with the format string. */

#ifdef __GNUC__
#define UAE_PRINTF_FORMAT(f, a) __attribute__((format(printf, f, a)))
#else
#define UAE_PRINTF_FORMAT(f, a)
#endif

#define UAE_WPRINTF_FORMAT(f, a)

#endif /* UAE_ATTRIBUTES_H */
