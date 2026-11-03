/*
* UAE - The Un*x Amiga Emulator
*
* MSVC support for the imported 86Box sources.
*
* 86Box is written for GCC and Clang. This header is force-included when
* the 86box/ sources are compiled with MSVC and maps the few GNU builtins
* and attributes they use onto MSVC intrinsics, so that the imported files
* don't need to be changed for it.
*/

#ifndef WINUAE_86BOX_MSVC_H
#define WINUAE_86BOX_MSVC_H

#if defined(_MSC_VER) && !defined(__clang__)

/* M_PI and the other math.h constants */
#define _USE_MATH_DEFINES

/* The ARM64 intrinsics define float32_t and float64_t as float types, but
   softfloat3e uses these names for its own structures */
#if defined(_M_ARM64)
#define float32_t uae_neon_float32_t
#define float64_t uae_neon_float64_t
#include <intrin.h>
#undef float32_t
#undef float64_t
#else
#include <intrin.h>
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#define __attribute__(x)
#define __thread __declspec(thread)

/* timer.h does 128-bit arithmetic; MSVC has no __int128 but its C++20
   standard library provides 128-bit integer classes */
#include <__msvc_int128.hpp>
#define int128_t std::_Signed128
#define uint128_t std::_Unsigned128

#define __builtin_expect(x, v) (x)

/* C11's _Static_assert, which GCC and Clang also accept in C++ */
#define _Static_assert static_assert

/* The ARM64 recompiler flushes new code from the data cache with GCC's
   helper; the files using it include windows.h */
#if defined(_M_ARM64)
#define __clear_cache(begin, end) \
	FlushInstructionCache(GetCurrentProcess(), (begin), (SIZE_T)((char *)(end) - (char *)(begin)))
#endif
#define __builtin_bswap16(x) _byteswap_ushort(x)
#define __builtin_bswap32(x) _byteswap_ulong(x)
#define __builtin_bswap64(x) _byteswap_uint64(x)

static __forceinline int __builtin_parity(unsigned int x)
{
	x ^= x >> 16;
	x ^= x >> 8;
	x ^= x >> 4;
	x &= 0xf;
	return (0x6996 >> x) & 1;
}

static __forceinline int __builtin_ctz(unsigned int x)
{
	unsigned long i;
	_BitScanForward(&i, x);
	return (int)i;
}

static __forceinline int __builtin_clz(unsigned int x)
{
	unsigned long i;
	_BitScanReverse(&i, x);
	return 31 - (int)i;
}

#endif

#endif /* WINUAE_86BOX_MSVC_H */
