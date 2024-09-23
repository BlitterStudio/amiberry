/* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Memory access functions
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef MACCESS_UAE_H
#define MACCESS_UAE_H

#include "uae/byteswap.h"
#include "sysdeps.h"

#ifdef CPU_64_BIT
#define ALIGN_POINTER_TO32(p) ((~(uae_u64)(p)) & 3)
#else
#define ALIGN_POINTER_TO32(p) ((~(uae_u32)(p)) & 3)
#endif

static inline uae_u64 do_get_mem_quad(uae_u64 *a)
{
#ifdef WORDS_BIGENDIAN
	return *a;
#else
	return uae_bswap_64(*a);
#endif
}

static inline uint32_t do_get_mem_long(uint32_t* a)
{
#ifdef WORDS_BIGENDIAN
	return *a;
#else
	return uae_bswap_32(*a);
#endif
}

static inline uint16_t do_get_mem_word(uint16_t* a)
{
#ifdef WORDS_BIGENDIAN
	return *a;
#else
	return uae_bswap_16(*a);
#endif
}

#define do_get_mem_byte(a) ((uae_u32)*(uae_u8 *)(a))

static inline void do_put_mem_quad(uae_u64 *a, uae_u64 v)
{
#ifdef WORDS_BIGENDIAN
	*a = v;
#else
	*a = uae_bswap_64(v);
#endif
}

static inline void do_put_mem_long(uint32_t* a, uint32_t v)
{
#ifdef WORDS_BIGENDIAN
	*a = v;
#else
	*a = uae_bswap_32(v);
#endif
}

static inline void do_put_mem_word(uint16_t* a, uint16_t v)
{
#ifdef WORDS_BIGENDIAN
	*a = v;
#else
	*a = uae_bswap_16(v);
#endif
}

static inline void do_put_mem_byte(uae_u8 *a, uae_u8 v)
{
	*a = v;
}

STATIC_INLINE uae_u64 do_byteswap_64(uae_u64 v)
{
	return uae_bswap_64(v);
}

STATIC_INLINE uae_u32 do_byteswap_32(uae_u32 v)
{
	return uae_bswap_32(v);
}

STATIC_INLINE uae_u16 do_byteswap_16(uae_u16 v)
{
	return uae_bswap_16(v);
}

STATIC_INLINE uae_u32 do_get_mem_word_unswapped(uae_u16* a)
{
	return *a;
}

#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))

#endif
