/* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Memory access functions
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef MACCESS_UAE_H
#define MACCESS_UAE_H

#ifdef CPU_64_BIT
#define ALIGN_POINTER_TO32(p) ((~(uae_u64)(p)) & 3)
#else
#define ALIGN_POINTER_TO32(p) ((~(uae_u32)(p)) & 3)
#endif

static __inline__ uint32_t do_get_mem_long(uint32_t* a)
{
    uint8_t* b = (uint8_t*)a;

    return (*b << 24) | (*(b + 1) << 16) | (*(b + 2) << 8) | (*(b + 3));
}

static __inline__ uint16_t do_get_mem_word(uint16_t* a)
{
    uint8_t* b = (uint8_t*)a;

    return (*b << 8) | (*(b + 1));
}

static __inline__ uint8_t do_get_mem_byte(uint8_t* a)
{
    return *a;
}

static __inline__ void do_put_mem_long(uint32_t* a, uint32_t v)
{
    uint8_t* b = (uint8_t*)a;

    *b = v >> 24;
    *(b + 1) = v >> 16;
    *(b + 2) = v >> 8;
    *(b + 3) = v;
}

static __inline__ void do_put_mem_word(uint16_t* a, uint16_t v)
{
    uint8_t* b = (uint8_t*)a;

    *b = v >> 8;
    *(b + 1) = v;
}

static __inline__ void do_put_mem_byte(uint8_t* a, uint8_t v)
{
    *a = v;
}

#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))

#undef USE_MAPPED_MEMORY
#undef CAN_MAP_MEMORY
#undef NO_INLINE_MEMORY_ACCESS
#undef MD_HAVE_MEM_1_FUNCS

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

#endif
