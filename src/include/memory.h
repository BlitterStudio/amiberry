 /*
  * UAE - The Un*x Amiga Emulator
  *
  * memory management
  *
  * Copyright 1995 Bernd Schmidt
  */

#ifndef UAE_MEMORY_H
#define UAE_MEMORY_H

extern void memory_reset (void);

#ifdef JIT
extern int special_mem;
#define S_READ 1
#define S_WRITE 2

extern void *cache_alloc (int);
extern void cache_free (void*, int);
#endif

#ifdef PANDORA
extern uae_u8* natmem_offset;
extern void free_AmigaMem(void);
extern void alloc_AmigaMem(void);
#endif

#ifdef ADDRESS_SPACE_24BIT
#define MEMORY_BANKS 256
#else
#define MEMORY_BANKS 65536
#endif

typedef uae_u32 (REGPARAM3 *mem_get_func)(uaecptr) REGPARAM;
typedef void (REGPARAM3 *mem_put_func)(uaecptr, uae_u32) REGPARAM;
typedef uae_u8 *(REGPARAM3 *xlate_func)(uaecptr) REGPARAM;
typedef int (REGPARAM3 *check_func)(uaecptr, uae_u32) REGPARAM;

extern uae_u8 *chipmemory;

extern uae_u32 allocated_chipmem;
extern uae_u32 allocated_fastmem;
extern uae_u32 allocated_bogomem;
extern uae_u32 allocated_gfxmem;
extern uae_u32 allocated_z3fastmem;
extern uae_u32 max_z3fastmem;

#undef DIRECT_MEMFUNCS_SUCCESSFUL
#include "md-pandora/maccess.h"

#define chipmem_start 0x00000000
#define bogomem_start 0x00C00000
#define kickmem_start 0x00F80000
extern uaecptr z3fastmem_start;
extern uaecptr p96ram_start;
extern uaecptr fastmem_start;

extern bool cloanto_rom, kickstart_rom;
extern uae_u16 kickstart_version;
extern bool uae_boot_rom;
extern int uae_boot_rom_size;
extern uaecptr rtarea_base;

enum { ABFLAG_UNK = 0, ABFLAG_RAM = 1, ABFLAG_ROM = 2, ABFLAG_ROMIN = 4, ABFLAG_IO = 8, ABFLAG_NONE = 16, ABFLAG_SAFE = 32 };
typedef struct {
  /* These ones should be self-explanatory... */
  mem_get_func lget, wget, bget;
  mem_put_func lput, wput, bput;
  /* Use xlateaddr to translate an Amiga address to a uae_u8 * that can
   * be used to address memory without calling the wget/wput functions.
   * This doesn't work for all memory banks, so this function may call
   * abort(). */
  xlate_func xlateaddr;
  /* To prevent calls to abort(), use check before calling xlateaddr.
   * It checks not only that the memory bank can do xlateaddr, but also
   * that the pointer points to an area of at least the specified size.
   * This is used for example to translate bitplane pointers in custom.c */
  check_func check;
  /* For those banks that refer to real memory, we can save the whole trouble
     of going through function calls, and instead simply grab the memory
     ourselves. This holds the memory address where the start of memory is
     for this particular bank. */
  uae_u8 *baseaddr;
  const TCHAR *name;
  /* for instruction opcode/operand fetches */
  mem_get_func lgeti, wgeti;
  int flags;
} addrbank;

extern uae_u8 *filesysory;
extern uae_u8 *rtarea;

extern addrbank chipmem_bank;
extern addrbank chipmem_agnus_bank;
extern addrbank kickmem_bank;
extern addrbank custom_bank;
extern addrbank clock_bank;
extern addrbank cia_bank;
extern addrbank rtarea_bank;
extern addrbank expamem_bank;
extern addrbank fastmem_bank;
extern addrbank gfxmem_bank;
extern addrbank akiko_bank;

extern void rtarea_init (void);
extern void rtarea_init_mem (void);
extern void rtarea_setup (void);
extern void expamem_init (void);
extern void expamem_reset (void);
extern void expamem_next (void);

extern uae_u32 gfxmem_start;
extern uae_u8 *gfxmemory;
extern uae_u32 gfxmem_mask;

/* Default memory access functions */

extern int REGPARAM3 default_check(uaecptr addr, uae_u32 size) REGPARAM;
extern uae_u8 *REGPARAM3 default_xlate(uaecptr addr) REGPARAM;
/* 680x0 opcode fetches */
extern uae_u32 REGPARAM3 dummy_lgeti (uaecptr addr) REGPARAM;
extern uae_u32 REGPARAM3 dummy_wgeti (uaecptr addr) REGPARAM;

#define bankindex(addr) (((uaecptr)(addr)) >> 16)

extern addrbank *mem_banks[MEMORY_BANKS];

#define get_mem_bank(addr) (*mem_banks[bankindex(addr)])

extern void memory_init (void);
extern void memory_cleanup (void);
extern void map_banks (addrbank *bank, int first, int count, int realsize);
extern void map_overlay (int chip);
extern void memory_hardreset (int);
extern void memory_clear (void);
extern void free_fastmemory (void);

#define longget(addr) (call_mem_get_func(get_mem_bank(addr).lget, addr))
#define wordget(addr) (call_mem_get_func(get_mem_bank(addr).wget, addr))
#define byteget(addr) (call_mem_get_func(get_mem_bank(addr).bget, addr))
#define longgeti(addr) (call_mem_get_func(get_mem_bank(addr).lgeti, addr))
#define wordgeti(addr) (call_mem_get_func(get_mem_bank(addr).wgeti, addr))
#define longput(addr,l) (call_mem_put_func(get_mem_bank(addr).lput, addr, l))
#define wordput(addr,w) (call_mem_put_func(get_mem_bank(addr).wput, addr, w))
#define byteput(addr,b) (call_mem_put_func(get_mem_bank(addr).bput, addr, b))

STATIC_INLINE uae_u32 get_long(uaecptr addr)
{
  return longget(addr);
}

STATIC_INLINE uae_u32 get_word(uaecptr addr)
{
  return wordget(addr);
}

STATIC_INLINE uae_u32 get_byte(uaecptr addr)
{
  return byteget(addr);
}

STATIC_INLINE uae_u32 get_longi(uaecptr addr)
{
    return longgeti(addr);
}
STATIC_INLINE uae_u32 get_wordi(uaecptr addr)
{
    return wordgeti(addr);
}

/*
 * Read a host pointer from addr
 */
#if SIZEOF_VOID_P == 4
# define get_pointer(addr) ((void *)get_long(addr))
#else
# if SIZEOF_VOID_P == 8
STATIC_INLINE void *get_pointer (uaecptr addr)
{
    const unsigned int n = SIZEOF_VOID_P / 4;
    union {
	void    *ptr;
	uae_u32  longs[SIZEOF_VOID_P / 4];
    } p;
    unsigned int i;

    for (i = 0; i < n; i++) {
#ifdef WORDS_BIGENDIAN
	p.longs[i]     = get_long (addr + i * 4);
#else
	p.longs[n - 1 - i] = get_long (addr + i * 4);
#endif
    }
    return p.ptr;
}
# else
#  error "Unknown or unsupported pointer size."
# endif
#endif

STATIC_INLINE void put_long(uaecptr addr, uae_u32 l)
{
    longput(addr, l);
}
STATIC_INLINE void put_word(uaecptr addr, uae_u32 w)
{
    wordput(addr, w);
}
STATIC_INLINE void put_byte(uaecptr addr, uae_u32 b)
{
    byteput(addr, b);
}

/*
 * Store host pointer v at addr
 */
#if SIZEOF_VOID_P == 4
# define put_pointer(addr, p) (put_long((addr), (uae_u32)(p)))
#else
# if SIZEOF_VOID_P == 8
STATIC_INLINE void put_pointer (uaecptr addr, void *v)
{
    const unsigned int n = SIZEOF_VOID_P / 4;
    union {
	void    *ptr;
	uae_u32  longs[SIZEOF_VOID_P / 4];
    } p;
    unsigned int i;

    p.ptr = v;

    for (i = 0; i < n; i++) {
#ifdef WORDS_BIGENDIAN
	put_long (addr + i * 4, p.longs[i]);
#else
	put_long (addr + i * 4, p.longs[n - 1 - i]);
#endif
    }
}
# endif
#endif

STATIC_INLINE uae_u8 *get_real_address(uaecptr addr)
{
    return get_mem_bank(addr).xlateaddr(addr);
}

STATIC_INLINE int valid_address(uaecptr addr, uae_u32 size)
{
    return get_mem_bank(addr).check(addr, size);
}

extern int addr_valid (const TCHAR*,uaecptr,uae_u32);

/* For faster access in custom chip emulation.  */
extern uae_u32 chipmem_mask, chipmem_full_mask, kickmem_mask;
extern uae_u8 *kickmemory;
extern addrbank dummy_bank;

STATIC_INLINE uae_u32 chipmem_lget_indirect(uae_u32 PT) {
  return do_get_mem_long((uae_u32 *)&chipmemory[PT & chipmem_full_mask]);
}
STATIC_INLINE uae_u32 chipmem_wget_indirect (uae_u32 PT) {
  return do_get_mem_word((uae_u16 *)&chipmemory[PT & chipmem_full_mask]);
}
#define chipmem_wput_indirect  chipmem_agnus_wput
extern void REGPARAM2 chipmem_agnus_wput (uaecptr addr, uae_u32 w);


extern uae_u8 *mapped_malloc (size_t, const TCHAR*);
extern void mapped_free (uae_u8 *);
extern void clearexec (void);

extern uaecptr strcpyha_safe (uaecptr dst, const uae_char *src);
extern uae_char *strcpyah_safe (uae_char *dst, uaecptr src, int maxsize);
extern void memcpyha_safe (uaecptr dst, const uae_u8 *src, int size);
extern void memcpyha (uaecptr dst, const uae_u8 *src, int size);
extern void memcpyah_safe (uae_u8 *dst, uaecptr src, int size);
extern void memcpyah (uae_u8 *dst, uaecptr src, int size);

extern uae_s32 getz2size (struct uae_prefs *p);
extern uae_u32 getz2endaddr (void);

#endif
