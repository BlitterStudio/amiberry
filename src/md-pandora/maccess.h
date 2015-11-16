 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Memory access functions
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef MACCESS_UAE_H
#define MACCESS_UAE_H

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


#ifdef ARMV6_ASSEMBLY
STATIC_INLINE uae_u16 do_get_mem_word(uae_u16 *_GCCRES_ a)
{
  uae_u16 v;
   __asm__ __volatile__ (
						"ldrh %[v], [%[a]] \n\t"
						"rev16 %[v], %[v] \n\t"
           : [v] "=r" (v) : [a] "r" (a) ); 
  return v;
}
#else
STATIC_INLINE uae_u16 do_get_mem_word(uae_u16 *_GCCRES_ a)
{
    uae_u8 *b = (uae_u8 *)a;
    
    return (*b << 8) | (*(b+1));
}
#endif


#ifdef ARMV6_ASSEMBLY
STATIC_INLINE uae_u32 do_get_mem_long(uae_u32 *a) 
{
  uae_u32 v;
   __asm__ __volatile__ (
						"ldr %[v], [%[a]] \n\t"
						"rev %[v], %[v] \n\t"
           : [v] "=r" (v) : [a] "r" (a) ); 
  return v;
}
#else
STATIC_INLINE uae_u32 do_get_mem_long(uae_u32 *_GCCRES_ a)
{
    uae_u8 *b = (uae_u8 *)a;
    
    return (*b << 24) | (*(b+1) << 16) | (*(b+2) << 8) | (*(b+3));
}
#endif


STATIC_INLINE uae_u8 do_get_mem_byte(uae_u8 *_GCCRES_ a)
{
    return *a;
}

#ifdef ARMV6_ASSEMBLY
STATIC_INLINE void do_put_mem_word(uae_u16 *_GCCRES_ a, uae_u16 v)
{
   __asm__ __volatile__ (
						"rev16 r2, %[v] \n\t"
						"strh r2, [%[a]] \n\t"
           : : [v] "r" (v), [a] "r" (a) : "r2", "memory" ); 
}
#else
STATIC_INLINE void do_put_mem_word(uae_u16 *_GCCRES_ a, uae_u16 v)
{
    uae_u8 *b = (uae_u8 *)a;
    
    *b = v >> 8;
    *(b+1) = v;
}
#endif

#ifdef ARMV6_ASSEMBLY
STATIC_INLINE void do_put_mem_long(uae_u32 *_GCCRES_ a, uae_u32 v)
{
   __asm__ __volatile__ (
						"rev r2, %[v] \n\t"
						"str r2, [%[a]] \n\t"
           : : [v] "r" (v), [a] "r" (a) : "r2", "memory" ); 
}
#else
STATIC_INLINE void do_put_mem_long(uae_u32 *_GCCRES_ a, uae_u32 v)
{
    uae_u8 *b = (uae_u8 *)a;
    
    *b = v >> 24;
    *(b+1) = v >> 16;    
    *(b+2) = v >> 8;
    *(b+3) = v;
}
#endif

STATIC_INLINE void do_put_mem_byte(uae_u8 *_GCCRES_ a, uae_u8 v)
{
    *a = v;
}

#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))

#undef MD_HAVE_MEM_1_FUNCS

#define ALIGN_POINTER_TO32(p) ((~(unsigned long)(p)) & 3)

#endif
