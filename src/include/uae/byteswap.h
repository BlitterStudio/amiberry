/*
 * UAE - The Un*x Amiga Emulator
 *
 * Byte swapping functions
 *
 * Copyright 2019 Frode Solheim
 */

#ifndef UAE_BYTESWAP_H_
#define UAE_BYTESWAP_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "uae/types.h"

#if defined(_WIN32) && !defined(__GNUC__)
#include <stdlib.h>
#define uae_bswap_16 _byteswap_uint16
#define uae_bswap_32 _byteswap_uint32
#define uae_bswap_64 _byteswap_uint64
#elif defined(__GNUC__)
/* GCC/MinGW: use compiler builtins */
#define uae_bswap_16 __builtin_bswap16
#define uae_bswap_32 __builtin_bswap32
#define uae_bswap_64 __builtin_bswap64
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define uae_bswap_16 OSSwapInt16
#define uae_bswap_32 OSSwapInt32
#define uae_bswap_64 OSSwapInt64
#else
#include <byteswap.h>
#define uae_bswap_16 bswap_16
#define uae_bswap_32 bswap_32
#define uae_bswap_64 bswap_64
#endif

// Using builtin byteswap functions where possible. In many cases, the
// compiler may use optimized byteswap builtins anyway, but better to
// not risk using slower function calls.

#ifdef HAVE___BUILTIN_BSWAP16
#undef uae_bswap_16
#define uae_bswap_16 __builtin_bswap16
#endif
#ifdef HAVE___BUILTIN_BSWAP32
#undef uae_bswap_32
#define uae_bswap_32 __builtin_bswap32
#endif
#ifdef HAVE___BUILTIN_BSWAP64
#undef uae_bswap_64
#define uae_bswap_64 __builtin_bswap64
#endif

#endif // UAE_BYTESWAP_H_