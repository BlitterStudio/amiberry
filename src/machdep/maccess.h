/* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Memory access functions
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef MACCESS_UAE_H
#define MACCESS_UAE_H

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

#define ALIGN_POINTER_TO32(p) ((~(unsigned long)(p)) & 3)

#define call_mem_get_func(func, addr) ((*func)(addr))
#define call_mem_put_func(func, addr, v) ((*func)(addr, v))

#undef USE_MAPPED_MEMORY
#undef CAN_MAP_MEMORY
#undef NO_INLINE_MEMORY_ACCESS
#undef MD_HAVE_MEM_1_FUNCS

#endif
