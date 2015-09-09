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
extern void a1000_reset (void);

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

extern char *address_space, *good_address_map;
extern uae_u8 *chipmemory;

extern uae_u32 allocated_chipmem;
extern uae_u32 allocated_fastmem;
extern uae_u32 allocated_bogomem;
extern uae_u32 allocated_gfxmem;
extern uae_u32 allocated_z3fastmem, max_z3fastmem;
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
extern uae_u32 allocated_a3000mem;
extern uae_u32 allocated_cardmem;
#endif

extern void wait_cpu_cycle (void);

#undef DIRECT_MEMFUNCS_SUCCESSFUL
#include "md-pandora/maccess.h"

#define chipmem_start 0x00000000
#define bogomem_start 0x00C00000
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
#define cardmem_start 0x00E00000
#endif
#define kickmem_start 0x00F80000
extern uaecptr z3fastmem_start;
extern uaecptr p96ram_start;
extern uaecptr fastmem_start;
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
extern uaecptr a3000lmem_start, a3000hmem_start;
#endif

extern int ersatzkickfile;
extern int cloanto_rom;
extern uae_u16 kickstart_version;
extern int uae_boot_rom, uae_boot_rom_size;
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
    const char *name;
    /* for instruction opcode/operand fetches */
    mem_get_func lgeti, wgeti;
    int flags;
} addrbank;

extern uae_u8 *filesysory;
extern uae_u8 *rtarea;

extern addrbank chipmem_bank;
extern addrbank chipmem_agnus_bank;
extern addrbank chipmem_bank_ce2;
extern addrbank kickmem_bank;
extern addrbank custom_bank;
extern addrbank clock_bank;
extern addrbank cia_bank;
extern addrbank rtarea_bank;
extern addrbank expamem_bank;
extern addrbank fastmem_bank;
extern addrbank gfxmem_bank, gfxmem_bankx;
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
extern addrbank gayle_bank;
extern addrbank gayle2_bank;
extern addrbank gayle_attr_bank;
extern addrbank gayle_common_bank;
extern addrbank mbres_bank;
extern addrbank akiko_bank;
extern addrbank cardmem_bank;
#endif

extern void rtarea_init (void);
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
#ifdef JIT
extern uae_u8 *baseaddr[MEMORY_BANKS];
#endif
#define get_mem_bank(addr) (*mem_banks[bankindex(addr)])

#ifdef JIT
#define put_mem_bank(addr, b, realstart) do { \
    (mem_banks[bankindex(addr)] = (b)); \
    if ((b)->baseaddr) \
	baseaddr[bankindex(addr)] = (b)->baseaddr - (realstart); \
    else \
	baseaddr[bankindex(addr)] = (uae_u8*)(((long)b)+1); \
} while (0)
#else
#define put_mem_bank(addr, b, realstart) \
  (mem_banks[bankindex(addr)] = (b));
#endif

extern void memory_init (void);
extern void memory_cleanup (void);
extern void map_banks (addrbank *bank, int first, int count, int realsize);
extern void map_overlay (int chip);
extern void memory_hardreset (void);
extern void free_fastmemory (void);

#define NONEXISTINGDATA 0

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

extern int addr_valid(char*,uaecptr,uae_u32);

/* For faster access in custom chip emulation.  */
extern void REGPARAM3 chipmem_wput (uaecptr, uae_u32) REGPARAM;
extern uae_u32 REGPARAM3 chipmem_agnus_wget (uaecptr) REGPARAM;

extern uae_u32 chipmem_mask, chipmem_full_mask, kickmem_mask;
extern uae_u8 *kickmemory;
extern addrbank dummy_bank;


static __inline__ uae_u16 CHIPMEM_WGET (uae_u32 PT) {
  return do_get_mem_word((uae_u16 *)&chipmemory[PT & chipmem_mask]);
}

static __inline void CHIPMEM_WPUT (uae_u32 PT, uae_u16 DA) {
  do_put_mem_word((uae_u16 *)&chipmemory[PT & chipmem_mask], DA);
}

static __inline__ uae_u32 CHIPMEM_LGET(uae_u32 PT) {
  return do_get_mem_long((uae_u32 *)&chipmemory[PT & chipmem_mask]);
}

static __inline__ uae_u16 CHIPMEM_WGET_CUSTOM (uae_u32 PT) {
  return do_get_mem_word((uae_u16 *)&chipmemory[PT & chipmem_mask]);
}

static __inline__ uae_u16 CHIPMEM_AGNUS_WGET_CUSTOM (uae_u32 PT) {
  return do_get_mem_word((uae_u16 *)&chipmemory[PT & chipmem_full_mask]);
}

static __inline void CHIPMEM_WPUT_CUSTOM (uae_u32 PT, uae_u16 DA) {
  do_put_mem_word((uae_u16 *)&chipmemory[PT & chipmem_mask], DA);
}

static __inline void CHIPMEM_AGNUS_WPUT_CUSTOM (uae_u32 PT, uae_u16 DA) {
  do_put_mem_word((uae_u16 *)&chipmemory[PT & chipmem_full_mask], DA);
}

static __inline__ uae_u32 CHIPMEM_LGET_CUSTOM(uae_u32 PT) {
  return do_get_mem_long((uae_u32 *)&chipmemory[PT & chipmem_mask]);
}

extern uae_u8 *mapped_malloc (size_t, const char *);
extern void mapped_free (uae_u8 *);
extern void clearexec (void);

extern void decode_cloanto_rom_do (uae_u8 *mem, int size, int real_size, uae_u8 *key, int keysize);

#define ROMTYPE_KICK 1
#define ROMTYPE_KICKCD32 2
#define ROMTYPE_EXTCD32 4
#define ROMTYPE_EXTCDTV 8
#define ROMTYPE_A2091BOOT 16
#define ROMTYPE_A4091BOOT 32
#define ROMTYPE_AR 64
#define ROMTYPE_SUPERIV 128
#define ROMTYPE_KEY 256
#define ROMTYPE_ARCADIABIOS 512
#define ROMTYPE_ARCADIAGAME 1024
#define ROMTYPE_HRTMON 2048
#define ROMTYPE_NORDIC 4096
#define ROMTYPE_XPOWER 8192
#define ROMTYPE_CD32CART 16384
#define ROMTYPE_EVEN 131072
#define ROMTYPE_ODD 262144
#define ROMTYPE_BYTESWAP 524288
#define ROMTYPE_SCRAMBLED 1048576

struct romheader {
    char *name;
    int id;
};

struct romdata {
    const char *name;
    int ver, rev;
    int subver, subrev;
    const char *model;
    uae_u32 size;
    int id;
    int cpu;
    int cloanto;
    int type;
    int group;
    int title;
    uae_u32 crc32;
    uae_u32 sha1[5];
    char *configname;
};

struct romlist {
    char *path;
    struct romdata *rd;
};

extern struct romdata *getromdatabypath(char *path);
extern struct romdata *getromdatabycrc (uae_u32 crc32);
extern struct romdata *getromdatabydata (uae_u8 *rom, int size);
extern struct romdata *getromdatabyid (int id);
extern struct romdata *getromdatabyzfile (struct zfile *f);
extern struct romlist **getarcadiaroms (void);
extern struct romdata *getarcadiarombyname (char *name);
extern struct romlist **getromlistbyident(int ver, int rev, int subver, int subrev, char *model, int all);
extern void getromname (struct romdata*, char*);
extern struct romdata *getromdatabyname (char*);
extern struct romlist *getromlistbyids(int *ids);
extern void romwarning(int *ids);
extern struct romlist *getromlistbyromdata(struct romdata *rd);
extern void romlist_add (char *path, struct romdata *rd);
extern char *romlist_get (struct romdata *rd);
extern void romlist_clear (void);

extern uae_u8 *load_keyfile (struct uae_prefs *p, char *path, int *size);
extern void free_keyfile (uae_u8 *key);

uaecptr strcpyha_safe (uaecptr dst, const char *src);
extern char *strcpyah_safe (char *dst, uaecptr src);
void memcpyha_safe (uaecptr dst, const uae_u8 *src, int size);
void memcpyha (uaecptr dst, const uae_u8 *src, int size);
void memcpyah_safe (uae_u8 *dst, uaecptr src, int size);
void memcpyah (uae_u8 *dst, uaecptr src, int size);

#if defined(ARMV6_ASSEMBLY)

extern "C" {
	void *arm_memset(void *s, int c, size_t n);
	void *arm_memcpy(void *dest, const void *src, size_t n);
}

/* 4-byte alignment */
//#define UAE4ALL_ALIGN __attribute__ ((__aligned__ (4)))
#define UAE4ALL_ALIGN __attribute__ ((__aligned__ (16)))
#define uae4all_memclr(p,l) arm_memset(p,0,l)
#define uae4all_memcpy arm_memcpy

#else

/* 4-byte alignment */
#define UAE4ALL_ALIGN __attribute__ ((__aligned__ (4)))
#define uae4all_memcpy memcpy
#define uae4all_memclr(p,l) memset(p, 0, l)

#endif

#endif
