 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Memory access functions
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef MACCESS_UAE_H
#define MACCESS_UAE_H

static uae_u32 do_get_mem_long(uae_u32 *a)
{
	uae_u8 *b = (uae_u8 *)a;

	return (*b << 24) | (*(b + 1) << 16) | (*(b + 2) << 8) | (*(b + 3));
}

static uae_u16 do_get_mem_word(uae_u16 *a)
{
    uae_u8 *b = (uae_u8 *)a;
    
    return (*b << 8) | (*(b+1));
}

static uae_u8 do_get_mem_byte(uae_u8 *a)
{
    return *a;
}

static void do_put_mem_long(uae_u32 *a, uae_u32 v)
{
	uae_u8 *b = (uae_u8 *)a;

	*b = v >> 24;
	*(b + 1) = v >> 16;
	*(b + 2) = v >> 8;
	*(b + 3) = v;
}

static void do_put_mem_word(uae_u16 *a, uae_u16 v)
{
    uae_u8 *b = (uae_u8 *)a;
    
    *b = v >> 8;
    *(b+1) = v;
}

static void do_put_mem_byte(uae_u8 *a, uae_u8 v)
{
    *a = v;
}

#define ALIGN_POINTER_TO32(p) ((~(unsigned long)(p)) & 3)

#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))

#undef USE_MAPPED_MEMORY
#undef CAN_MAP_MEMORY
#undef NO_INLINE_MEMORY_ACCESS
#undef MD_HAVE_MEM_1_FUNCS

#endif
