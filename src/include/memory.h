/*
* UAE - The Un*x Amiga Emulator
*
* memory management
*
* Copyright 1995 Bernd Schmidt
*/

#ifndef UAE_MEMORY_H
#define UAE_MEMORY_H

extern void memory_reset(void);
extern void memory_restore(void);
extern void a1000_reset(void);

#ifdef JIT
extern int special_mem;
extern int special_mem_default;
extern int jit_n_addr_unsafe;
#endif

#define S_READ 1
#define S_WRITE 2
#define S_N_ADDR 4

bool init_shm (void);
void free_shm (void);
bool preinit_shm (void);
extern bool canbang;
extern bool jit_direct_compatible_memory;
extern uaecptr highest_ram;

#define Z3BASE_UAE 0x10000000
#define Z3BASE_REAL 0x40000000

#define AUTOCONFIG_Z2 0x00e80000
#define AUTOCONFIG_Z2_MEM 0x00200000
#define AUTOCONFIG_Z3 0xff000000

#ifdef ADDRESS_SPACE_24BIT
#define MEMORY_BANKS 256
#define MEMORY_BANKS_24 256
#define MEMORY_RANGE_MASK ((1<<24)-1)
#else
#define MEMORY_BANKS 65536
#define MEMORY_BANKS_24 256
#define MEMORY_RANGE_MASK (~0)
#endif

typedef uae_u32 (REGPARAM3 *mem_get_func)(uaecptr) REGPARAM;
typedef void (REGPARAM3 *mem_put_func)(uaecptr, uae_u32) REGPARAM;
typedef uae_u8 *(REGPARAM3 *xlate_func)(uaecptr) REGPARAM;
typedef int (REGPARAM3 *check_func)(uaecptr, uae_u32) REGPARAM;

extern uae_u32 max_z3fastmem, max_physmem;

extern uae_u32 wait_cpu_cycle_read (uaecptr addr, int mode);
extern void wait_cpu_cycle_write (uaecptr addr, int mode, uae_u32 v);
extern uae_u32 wait_cpu_cycle_read_ce020 (uaecptr addr, int mode);
extern void wait_cpu_cycle_write_ce020 (uaecptr addr, int mode, uae_u32 v);

#undef DIRECT_MEMFUNCS_SUCCESSFUL
#include "machdep/maccess.h"

#define chipmem_start_addr 0x00000000
#define bogomem_start_addr 0x00C00000
#define cardmem_start_addr 0x00E00000
#define kickmem_start_addr 0x00F80000

#define ROM_SIZE_512 524288
#define ROM_SIZE_256 262144
#define ROM_SIZE_128 131072

extern bool ersatzkickfile;
extern bool cloanto_rom, kickstart_rom;
extern uae_u16 kickstart_version;
extern int uae_boot_rom_type;
extern int uae_boot_rom_size;
extern uaecptr rtarea_base;
extern uaecptr uaeboard_base;

extern uae_u8* baseaddr[];

#define CACHE_ENABLE_DATA 0x01
#define CACHE_ENABLE_DATA_BURST 0x02
#define CACHE_ENABLE_COPYBACK 0x020
#define CACHE_ENABLE_INS 0x80
#define CACHE_ENABLE_INS_BURST 0x40
#define CACHE_ENABLE_BOTH (CACHE_ENABLE_DATA | CACHE_ENABLE_INS)
#define CACHE_ENABLE_ALL (CACHE_ENABLE_BOTH | CACHE_ENABLE_INS_BURST | CACHE_ENABLE_DATA_BURST)
#define CACHE_DISABLE_ALLOCATE 0x08
#define CACHE_DISABLE_MMU 0x10
extern uae_u8 ce_banktype[65536], ce_cachable[65536];

#define ABFLAG_CACHE_SHIFT 24
enum
{
	ABFLAG_UNK = 0, ABFLAG_RAM = 1, ABFLAG_ROM = 2, ABFLAG_ROMIN = 4, ABFLAG_IO = 8,
	ABFLAG_NONE = 16, ABFLAG_SAFE = 32, ABFLAG_INDIRECT = 64, ABFLAG_NOALLOC = 128,
	ABFLAG_RTG = 256, ABFLAG_THREADSAFE = 512, ABFLAG_DIRECTMAP = 1024, ABFLAG_ALLOCINDIRECT = 2048,
	ABFLAG_CHIPRAM = 4096, ABFLAG_CIA = 8192, ABFLAG_PPCIOSPACE = 16384,
	ABFLAG_MAPPED = 32768,
	ABFLAG_DIRECTACCESS = 65536,
	ABFLAG_NODMA = 131072,
	ABFLAG_CACHE_ENABLE_DATA = CACHE_ENABLE_DATA << ABFLAG_CACHE_SHIFT,
	ABFLAG_CACHE_ENABLE_DATA_BURST = CACHE_ENABLE_DATA_BURST << ABFLAG_CACHE_SHIFT,
	ABFLAG_CACHE_ENABLE_INS = CACHE_ENABLE_INS << ABFLAG_CACHE_SHIFT,
	ABFLAG_CACHE_ENABLE_INS_BURST = CACHE_ENABLE_INS_BURST << ABFLAG_CACHE_SHIFT,
};

#define ABFLAG_CACHE_ENABLE_BOTH (ABFLAG_CACHE_ENABLE_DATA | ABFLAG_CACHE_ENABLE_INS)
#define ABFLAG_CACHE_ENABLE_ALL (ABFLAG_CACHE_ENABLE_BOTH | ABFLAG_CACHE_ENABLE_INS_BURST | ABFLAG_CACHE_ENABLE_DATA_BURST)

typedef struct addrbank {
	/* These ones should be self-explanatory...
	 * Do not move. JIT depends on it
	 */
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
	const TCHAR *label;
	const TCHAR *name;
	/* for instruction opcode/operand fetches */
	mem_get_func lgeti, wgeti;
	int flags;
	int jit_read_flag, jit_write_flag;
	struct addrbank_sub *sub_banks;
	uae_u32 mask;
	uae_u32 startmask;
	uae_u32 start;
	// if RAM: size of allocated RAM. Zero if failed.
	uae_u32 allocated_size;
	// size of bank (if IO or before RAM allocation)
	uae_u32 reserved_size;
	/* non-NULL if xget/xput can be bypassed */
	uae_u8 *baseaddr_direct_r;
	uae_u8 *baseaddr_direct_w;
	uae_u32 startaccessmask;
	bool barrier;
	uae_u32 protectmode;
} addrbank;

#define MEMORY_MIN_SUBBANK 1024
struct addrbank_sub
{
	addrbank *bank;
	uae_u32 offset;
	uae_u32 suboffset;
	uae_u32 mask;
	uae_u32 maskval;
};

struct autoconfig_info
{
	struct uae_prefs *prefs;
	bool doinit;
	bool postinit;
	int devnum;
	uae_u8 autoconfig_raw[128];
	uae_u8 autoconfig_bytes[16];
	TCHAR name[128];
	const uae_u8 *autoconfigp;
	bool autoconfig_automatic;
	uae_u32 start;
	uae_u32 size;
	int zorro;
	// never direct maps RAM
	bool indirect;
	const TCHAR *label;
	struct addrbank *addrbank;
	uaecptr write_bank_address;
	struct romconfig *rc;
	uae_u32 last_high_ram;
	const struct cpuboardsubtype *cst;
	const struct expansionromtype *ert;
	struct autoconfig_info *parent;
	const int *parent_romtype;
	bool parent_of_previous;
	bool parent_address_space;
	bool direct_vram;
	const TCHAR *parent_name;
	bool can_sort;
	bool hardwired;
	bool (*get_params)(struct uae_prefs*, struct expansion_params*);
	bool (*set_params)(struct uae_prefs*, struct expansion_params*);
	void *userdata;
};

#define CE_MEMBANK_FAST32 0
#define CE_MEMBANK_CHIP16 1
#define CE_MEMBANK_CHIP32 2
#define CE_MEMBANK_CIA 3
#define CE_MEMBANK_FAST16 4
//#define CE_MEMBANK_FAST16_EXTRA_ACCURACY 5

#define MEMORY_LGETI(name) \
static uae_u32 REGPARAM3 name ## _lgeti (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## _lgeti (uaecptr addr) \
{ \
	uae_u8 *m; \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	m = name ## _bank.baseaddr + addr; \
	return do_get_mem_long ((uae_u32 *)m); \
}
#define MEMORY_WGETI(name) \
static uae_u32 REGPARAM3 name ## _wgeti (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## _wgeti (uaecptr addr) \
{ \
	uae_u8 *m; \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	m = name ## _bank.baseaddr + addr; \
	return do_get_mem_word ((uae_u16 *)m); \
}
#define MEMORY_LGET(name) \
static uae_u32 REGPARAM3 name ## _lget (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## _lget (uaecptr addr) \
{ \
	uae_u8 *m; \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	m = name ## _bank.baseaddr + addr; \
	return do_get_mem_long ((uae_u32 *)m); \
}
#define MEMORY_WGET(name) \
static uae_u32 REGPARAM3 name ## _wget (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## _wget (uaecptr addr) \
{ \
	uae_u8 *m; \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	m = name ## _bank.baseaddr + addr; \
	return do_get_mem_word ((uae_u16 *)m); \
}
#define MEMORY_BGET(name) \
static uae_u32 REGPARAM3 name ## _bget (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## _bget (uaecptr addr) \
{ \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	return name ## _bank.baseaddr[addr]; \
}
#define MEMORY_LPUT(name) \
static void REGPARAM3 name ## _lput (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM2 name ## _lput (uaecptr addr, uae_u32 l) \
{ \
	uae_u8 *m;  \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	m = name ## _bank.baseaddr + addr; \
	do_put_mem_long ((uae_u32 *)m, l); \
}
#define MEMORY_WPUT(name) \
static void REGPARAM3 name ## _wput (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM2 name ## _wput (uaecptr addr, uae_u32 w) \
{ \
	uae_u8 *m;  \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	m = name ## _bank.baseaddr + addr; \
	do_put_mem_word ((uae_u16 *)m, w); \
}
#define MEMORY_BPUT(name) \
static void REGPARAM3 name ## _bput (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM2 name ## _bput (uaecptr addr, uae_u32 b) \
{ \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	name ## _bank.baseaddr[addr] = b; \
}
#define MEMORY_CHECK(name) \
static int REGPARAM3 name ## _check (uaecptr addr, uae_u32 size) REGPARAM; \
static int REGPARAM2 name ## _check (uaecptr addr, uae_u32 size) \
{ \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	return (addr + size) <= name ## _bank.allocated_size; \
}
#define MEMORY_XLATE(name) \
static uae_u8 *REGPARAM3 name ## _xlate (uaecptr addr) REGPARAM; \
static uae_u8 *REGPARAM2 name ## _xlate (uaecptr addr) \
{ \
	addr -= name ## _bank.startaccessmask; \
	addr &= name ## _bank.mask; \
	return name ## _bank.baseaddr + addr; \
}

#define DECLARE_MEMORY_FUNCTIONS(name) \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _lget) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _lgeti) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _wget) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _wgeti) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _bget) (uaecptr) REGPARAM; \
static void REGPARAM3 NOWARN_UNUSED(name ## _lput) (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM3 NOWARN_UNUSED(name ## _wput) (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM3 NOWARN_UNUSED(name ## _bput) (uaecptr, uae_u32) REGPARAM; \
static int REGPARAM3 NOWARN_UNUSED(name ## _check) (uaecptr addr, uae_u32 size) REGPARAM; \
static uae_u8 *REGPARAM3 NOWARN_UNUSED(name ## _xlate) (uaecptr addr) REGPARAM;

#define DECLARE_MEMORY_FUNCTIONS_WITH_SUFFIX(name, suffix) \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _lget_ ## suffix) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _lgeti_ ## suffix) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _wget_ ## suffix) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _wgeti_ ## suffix) (uaecptr) REGPARAM; \
static uae_u32 REGPARAM3 NOWARN_UNUSED(name ## _bget_ ## suffix) (uaecptr) REGPARAM; \
static void REGPARAM3 NOWARN_UNUSED(name ## _lput_ ## suffix) (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM3 NOWARN_UNUSED(name ## _wput_ ## suffix) (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM3 NOWARN_UNUSED(name ## _bput_ ## suffix) (uaecptr, uae_u32) REGPARAM; \
static int REGPARAM3 NOWARN_UNUSED(name ## _check_ ## suffix) (uaecptr addr, uae_u32 size) REGPARAM; \
static uae_u8 *REGPARAM3 NOWARN_UNUSED(name ## _xlate_ ## suffix) (uaecptr addr) REGPARAM;

#define MEMORY_FUNCTIONS(name) \
MEMORY_LGET(name); \
MEMORY_WGET(name); \
MEMORY_BGET(name); \
MEMORY_LPUT(name); \
MEMORY_WPUT(name); \
MEMORY_BPUT(name); \
MEMORY_CHECK(name); \
MEMORY_XLATE(name);


#define MEMORY_ARRAY_LGET(name, index) \
static uae_u32 REGPARAM3 name ## index ## _lget (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## index ## _lget (uaecptr addr) \
{ \
	uae_u8 *m; \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	m = name ## _bank[index].baseaddr + addr; \
	return do_get_mem_long ((uae_u32 *)m); \
}
#define MEMORY_ARRAY_WGET(name, index) \
static uae_u32 REGPARAM3 name ## index ## _wget (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## index ## _wget (uaecptr addr) \
{ \
	uae_u8 *m; \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	m = name ## _bank[index].baseaddr + addr; \
	return do_get_mem_word ((uae_u16 *)m); \
}
#define MEMORY_ARRAY_BGET(name, index) \
static uae_u32 REGPARAM3 name ## index ## _bget (uaecptr) REGPARAM; \
static uae_u32 REGPARAM2 name ## index ## _bget (uaecptr addr) \
{ \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	return name ## _bank[index].baseaddr[addr]; \
}
#define MEMORY_ARRAY_LPUT(name, index) \
static void REGPARAM3 name ## index ## _lput (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM2 name ## index ## _lput (uaecptr addr, uae_u32 l) \
{ \
	uae_u8 *m;  \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	m = name ## _bank[index].baseaddr + addr; \
	do_put_mem_long ((uae_u32 *)m, l); \
}
#define MEMORY_ARRAY_WPUT(name, index) \
static void REGPARAM3 name ## index ## _wput (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM2 name ## index ## _wput (uaecptr addr, uae_u32 w) \
{ \
	uae_u8 *m;  \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	m = name ## _bank[index].baseaddr + addr; \
	do_put_mem_word ((uae_u16 *)m, w); \
}
#define MEMORY_ARRAY_BPUT(name, index) \
static void REGPARAM3 name ## index ## _bput (uaecptr, uae_u32) REGPARAM; \
static void REGPARAM2 name ## index ## _bput (uaecptr addr, uae_u32 b) \
{ \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	name ## _bank[index].baseaddr[addr] = b; \
}
#define MEMORY_ARRAY_CHECK(name, index) \
static int REGPARAM3 name ## index ## _check (uaecptr addr, uae_u32 size) REGPARAM; \
static int REGPARAM2 name ## index ## _check (uaecptr addr, uae_u32 size) \
{ \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	return (addr + size) <= name ## _bank[index].allocated_size; \
}
#define MEMORY_ARRAY_XLATE(name, index) \
static uae_u8 *REGPARAM3 name ## index ## _xlate (uaecptr addr) REGPARAM; \
static uae_u8 *REGPARAM2 name ## index ## _xlate (uaecptr addr) \
{ \
	addr -= name ## _bank[index].startaccessmask; \
	addr &= name ## _bank[index].mask; \
	return name ## _bank[index].baseaddr + addr; \
}

#define MEMORY_ARRAY_FUNCTIONS(name, index) \
MEMORY_ARRAY_LGET(name, index); \
MEMORY_ARRAY_WGET(name, index); \
MEMORY_ARRAY_BGET(name, index); \
MEMORY_ARRAY_LPUT(name, index); \
MEMORY_ARRAY_WPUT(name, index); \
MEMORY_ARRAY_BPUT(name, index); \
MEMORY_ARRAY_CHECK(name, index); \
MEMORY_ARRAY_XLATE(name, index);

extern addrbank chipmem_bank;
extern addrbank chipmem_agnus_bank;
extern addrbank chipmem_bank_ce2;
extern addrbank kickmem_bank;
extern addrbank custom_bank;
extern addrbank clock_bank;
extern addrbank cia_bank;
extern addrbank rtarea_bank;
extern addrbank filesys_bank;
extern addrbank uaeboard_bank;
extern addrbank expamem_bank;
extern addrbank expamem_null, expamem_none;
extern addrbank fastmem_bank[MAX_RAM_BOARDS];
extern addrbank fastmem_nojit_bank[MAX_RAM_BOARDS];
extern addrbank *gfxmem_banks[MAX_RTG_BOARDS];
extern addrbank gayle_bank;
extern addrbank gayle2_bank;
extern addrbank mbres_bank;
extern addrbank akiko_bank;
extern addrbank cardmem_bank;
extern addrbank bogomem_bank;
extern addrbank z3fastmem_bank[MAX_RAM_BOARDS];
extern addrbank z3chipmem_bank;
extern addrbank mem25bit_bank;
extern addrbank debugmem_bank;
extern addrbank a3000lmem_bank;
extern addrbank a3000hmem_bank;
extern addrbank extendedkickmem_bank;
extern addrbank extendedkickmem2a_bank;
extern addrbank extendedkickmem2b_bank;
extern addrbank custmem1_bank;
extern addrbank custmem2_bank;
extern addrbank romboardmem_bank[MAX_ROM_BOARDS];

extern void rtarea_init(void);
extern void rtarea_free(void);
extern void rtarea_init_mem(void);
extern void rtarea_setup(void);
extern void expamem_reset(int);
extern void expamem_next(addrbank *mapped, addrbank *next);
extern void expamem_shutup(addrbank *mapped);
extern bool expamem_z3hack(struct uae_prefs*);
extern void expansion_cpu_fallback(void);
extern void set_expamem_z3_hack_mode(int);
extern uaecptr expamem_board_pointer, expamem_highmem_pointer;
extern uaecptr expamem_z3_pointer_real, expamem_z3_pointer_uae;
extern uae_u32 expamem_z3_highram_real, expamem_z3_highram_uae;
extern uae_u32 expamem_board_size;

/* Default memory access functions */

extern void dummy_put (uaecptr addr, int size, uae_u32 val);
extern uae_u32 dummy_get (uaecptr addr, int size, bool inst, uae_u32 defvalue);
extern uae_u32 dummy_get_safe(uaecptr addr, int size, bool inst, uae_u32 defvalue);

extern int REGPARAM3 default_check(uaecptr addr, uae_u32 size) REGPARAM;
extern uae_u8 *REGPARAM3 default_xlate(uaecptr addr) REGPARAM;
/* 680x0 opcode fetches */
extern uae_u32 REGPARAM3 dummy_lgeti (uaecptr addr) REGPARAM;
extern uae_u32 REGPARAM3 dummy_wgeti (uaecptr addr) REGPARAM;

/* sub bank support */
extern uae_u32 REGPARAM3 sub_bank_lget (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 sub_bank_wget(uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 sub_bank_bget(uaecptr) REGPARAM;
extern void REGPARAM3 sub_bank_lput(uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 sub_bank_wput(uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 sub_bank_bput(uaecptr, uae_u32) REGPARAM;
extern uae_u32 REGPARAM3 sub_bank_lgeti(uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 sub_bank_wgeti(uaecptr) REGPARAM;
extern int REGPARAM3 sub_bank_check(uaecptr addr, uae_u32 size) REGPARAM;
extern uae_u8 *REGPARAM3 sub_bank_xlate(uaecptr addr) REGPARAM;
extern addrbank *get_sub_bank(uaecptr *addr);

#define bankindex(addr) (((uaecptr)(addr)) >> 16)

extern addrbank *mem_banks[MEMORY_BANKS];

#ifdef JIT
extern uae_u8 *baseaddr[MEMORY_BANKS];
#endif

#define get_mem_bank(addr) (*mem_banks[bankindex(addr)])
extern addrbank *get_mem_bank_real(uaecptr);

#ifdef JIT
#define put_mem_bank(addr, b, realstart) do { \
	(mem_banks[bankindex(addr)] = (b)); \
	if ((b)->baseaddr) \
		baseaddr[bankindex(addr)] = (b)->baseaddr - (realstart); \
	else \
		baseaddr[bankindex(addr)] = (uae_u8*)(((uae_u8*)b)+1); \
} while (0)
#else
#define put_mem_bank(addr, b, realstart) \
	(mem_banks[bankindex(addr)] = (b));
#endif

extern void memory_init (void);
extern void memory_cleanup(void);
extern void restore_banks(void);
extern void map_banks (addrbank *bank, int first, int count, int realsize);
extern void map_banks_z2(addrbank *bank, int first, int count);
extern uae_u32 map_banks_z2_autosize(addrbank *bank, int first);
extern void map_banks_z3(addrbank *bank, int first, int count);
extern bool validate_banks_z2(addrbank *bank, int start, int size);
extern bool validate_banks_z3(addrbank *bank, int start, int size);
extern void map_banks_quick (addrbank *bank, int first, int count, int realsize);
extern void map_banks_nojitdirect (addrbank *bank, int first, int count, int realsize);
extern void map_banks_cond (addrbank *bank, int first, int count, int realsize);
extern void map_overlay (int chip);
extern void memory_hardreset (int);
extern void memory_clear (void);
extern void free_fastmemory (int);
extern void set_roms_modified (void);
extern void reload_roms(void);
extern bool read_kickstart_version(struct uae_prefs *p);
extern void chipmem_setindirect(void);
extern void initramboard(addrbank *ab, struct ramboard *rb);
extern void loadboardfile(addrbank *ab, struct boardloadfile *lf);
extern void mman_set_barriers(bool);

uae_u32 memory_get_long(uaecptr);
uae_u32 memory_get_word(uaecptr);
uae_u32 memory_get_byte(uaecptr);
uae_u32 memory_get_longi(uaecptr);
uae_u32 memory_get_wordi(uaecptr);

STATIC_INLINE uae_u32 get_long(uaecptr addr)
{
	return memory_get_long(addr);
}
STATIC_INLINE uae_u32 get_word (uaecptr addr)
{
	return memory_get_word(addr);
}
STATIC_INLINE uae_u32 get_byte (uaecptr addr)
{
	return memory_get_byte(addr);
}
STATIC_INLINE uae_u32 get_longi(uaecptr addr)
{
	return memory_get_longi(addr);
}
STATIC_INLINE uae_u32 get_wordi(uaecptr addr)
{
	return memory_get_wordi(addr);
}

// do split memory access if it can cross memory banks
STATIC_INLINE uae_u32 get_long_compatible(uaecptr addr)
{
	if ((addr &0xffff) < 0xfffd) {
		return memory_get_long(addr);
	} else if (addr & 1) {
		uae_u8 v0 = memory_get_byte(addr + 0);
		uae_u16 v1 = memory_get_word(addr + 1);
		uae_u8 v3 = memory_get_byte(addr + 3);
		return (v0 << 24) | (v1 << 8) | (v3 << 0);
	} else {
		uae_u16 v0 = memory_get_word(addr + 0);
		uae_u16 v1 = memory_get_word(addr + 2);
		return (v0 << 16) | (v1 << 0);
	}
}
STATIC_INLINE uae_u32 get_word_compatible(uaecptr addr)
{
	if ((addr & 0xffff) < 0xffff) {
		return memory_get_word(addr);
	} else {
		uae_u8 v0 = memory_get_byte(addr + 0);
		uae_u8 v1 = memory_get_byte(addr + 1);
		return (v0 << 8) | (v1 << 0);
	}
}
STATIC_INLINE uae_u32 get_byte_compatible(uaecptr addr)
{
	return memory_get_byte(addr);
}
STATIC_INLINE uae_u32 get_longi_compatible(uaecptr addr)
{
	if ((addr & 0xffff) < 0xfffd) {
		return memory_get_longi(addr);
	} else {
		uae_u16 v0 = memory_get_wordi(addr + 0);
		uae_u16 v1 = memory_get_wordi(addr + 2);
		return (v0 << 16) | (v1 << 0);
	}
}
STATIC_INLINE uae_u32 get_wordi_compatible(uaecptr addr)
{
	return memory_get_wordi(addr);
}


STATIC_INLINE uae_u32 get_long_jit(uaecptr addr)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_read_flag;
#endif
	return memory_get_long(addr);
}
STATIC_INLINE uae_u32 get_word_jit(uaecptr addr)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_read_flag;
#endif
	return memory_get_word(addr);
}
STATIC_INLINE uae_u32 get_byte_jit(uaecptr addr)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_read_flag;
#endif
	return memory_get_byte(addr);
}
STATIC_INLINE uae_u32 get_longi_jit(uaecptr addr)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_read_flag;
#endif
	return memory_get_longi(addr);
}
STATIC_INLINE uae_u32 get_wordi_jit(uaecptr addr)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_read_flag;
#endif
	return memory_get_wordi(addr);
}

/*
* Read a host pointer from addr
*/
#if SIZEOF_VOID_P == 4
# define get_pointer(addr) ((void *)get_long (addr))
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

void dma_put_word(uaecptr addr, uae_u16 v);
uae_u16 dma_get_word(uaecptr addr);
void dma_put_byte(uaecptr addr, uae_u8 v);
uae_u8 dma_get_byte(uaecptr addr);

void memory_put_long(uaecptr, uae_u32);
void memory_put_word(uaecptr, uae_u32);
void memory_put_byte(uaecptr, uae_u32);

STATIC_INLINE void put_long (uaecptr addr, uae_u32 l)
{
	memory_put_long(addr, l);
}
STATIC_INLINE void put_word (uaecptr addr, uae_u32 w)
{
	memory_put_word(addr, w);
}
STATIC_INLINE void put_byte (uaecptr addr, uae_u32 b)
{
	memory_put_byte(addr, b);
}

// do split memory access if it can cross memory banks
STATIC_INLINE void put_long_compatible(uaecptr addr, uae_u32 l)
{
	if ((addr & 0xffff) < 0xfffd) {
		memory_put_long(addr, l);
	} else if (addr & 1) {
		memory_put_byte(addr + 0, l >> 24);
		memory_put_word(addr + 1, l >>  8);
		memory_put_byte(addr + 3, l >>  0);
	} else {
		memory_put_word(addr + 0, l >> 16);
		memory_put_word(addr + 2, l >>  0);
	}
}
STATIC_INLINE void put_word_compatible(uaecptr addr, uae_u32 w)
{
	if ((addr & 0xffff) < 0xffff) {
		memory_put_word(addr, w);
	} else {
		memory_put_byte(addr + 0, w >> 8);
		memory_put_byte(addr + 1, w >> 0);
	}
}
STATIC_INLINE void put_byte_compatible(uaecptr addr, uae_u32 b)
{
	memory_put_byte(addr, b);
}


STATIC_INLINE void put_long_jit(uaecptr addr, uae_u32 l)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_write_flag;
#endif
	memory_put_long(addr, l);
}
STATIC_INLINE void put_word_jit(uaecptr addr, uae_u32 l)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_write_flag;
#endif
	memory_put_word(addr, l);
}
STATIC_INLINE void put_byte_jit(uaecptr addr, uae_u32 l)
{
#ifdef JIT
	addrbank *bank = &get_mem_bank(addr);
	special_mem |= bank->jit_write_flag;
#endif
	memory_put_byte(addr, l);
}

/*
* Store host pointer v at addr
*/
#if SIZEOF_VOID_P == 4
# define put_pointer(addr, p) (put_long ((addr), (uae_u32)(p)))
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

bool real_address_allowed(void);
uae_u8 *memory_get_real_address(uaecptr);
int memory_valid_address(uaecptr, uae_u32);

STATIC_INLINE uae_u8 *get_real_address (uaecptr addr)
{
	return memory_get_real_address(addr);
}

STATIC_INLINE int valid_address (uaecptr addr, uae_u32 size)
{
	return memory_valid_address(addr, size);
}

STATIC_INLINE void put_quad_host(void *addr, uae_u64 v)
{
	do_put_mem_long((uae_u32*)addr, v >> 32);
	do_put_mem_long(((uae_u32*)addr) + 1, (uae_u32)v);
}
STATIC_INLINE void put_long_host(void *addr, uae_u32 v)
{
	do_put_mem_long((uae_u32*)addr, v);
}
STATIC_INLINE void put_word_host(void *addr, uae_u16 v)
{
	do_put_mem_word((uae_u16*)addr, v);
}
STATIC_INLINE void put_byte_host(void *addr, uae_u8 v)
{
	*((uae_u8*)addr) = v;
}
STATIC_INLINE uae_u64 get_quad_host(void *addr)
{
	uae_u64 v = ((uae_u64)do_get_mem_long((uae_u32*)addr)) << 32;
	v |= do_get_mem_long(((uae_u32*)addr) + 1);
	return v;
}
STATIC_INLINE uae_u32 get_long_host(void *addr)
{
	return do_get_mem_long((uae_u32*)addr);
}
STATIC_INLINE uae_u16 get_word_host(void *addr)
{
	return do_get_mem_word((uae_u16*)addr);
}
STATIC_INLINE uae_u32 get_byte_host(void *addr)
{
	return *((uae_u8*)addr);
}

extern int addr_valid (const TCHAR*, uaecptr,uae_u32);

/* For faster access in custom chip emulation.  */
extern void REGPARAM3 chipmem_lput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_wput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_bput (uaecptr, uae_u32) REGPARAM;

extern uae_u32 REGPARAM3 chipmem_agnus_wget (uaecptr) REGPARAM;
extern void REGPARAM3 chipmem_agnus_wput (uaecptr, uae_u32) REGPARAM;

extern addrbank dummy_bank;

/* 68020+ Chip RAM DMA contention emulation */
extern void REGPARAM3 chipmem_bput_c2 (uaecptr, uae_u32) REGPARAM;

extern uae_u32 (REGPARAM3 *chipmem_lget_indirect)(uaecptr) REGPARAM;
extern uae_u32 (REGPARAM3 *chipmem_wget_indirect)(uaecptr) REGPARAM;
extern uae_u32 (REGPARAM3 *chipmem_bget_indirect)(uaecptr) REGPARAM;
extern void (REGPARAM3 *chipmem_lput_indirect)(uaecptr, uae_u32) REGPARAM;
extern void (REGPARAM3 *chipmem_wput_indirect)(uaecptr, uae_u32) REGPARAM;
extern void (REGPARAM3 *chipmem_bput_indirect)(uaecptr, uae_u32) REGPARAM;
extern int (REGPARAM3 *chipmem_check_indirect)(uaecptr, uae_u32) REGPARAM;
extern uae_u8 *(REGPARAM3 *chipmem_xlate_indirect)(uaecptr) REGPARAM;

#ifdef NATMEM_OFFSET

typedef struct shmpiece_reg {
	uae_u8 *native_address;
	int id;
	uae_u32 size;
	const TCHAR *name;
	struct shmpiece_reg *next;
	struct shmpiece_reg *prev;
} shmpiece;

extern shmpiece *shm_start;

extern uae_u8* natmem_offset;
extern uae_u8 *natmem_reserved;
extern uae_u32 natmem_reserved_size;

#endif

extern bool mapped_malloc (addrbank*);
extern void mapped_free (addrbank*);
extern void a3000_fakekick (int);

extern uaecptr strcpyha_safe (uaecptr dst, const uae_char *src);
extern uae_char *strcpyah_safe (uae_char *dst, uaecptr src, int maxsize);
extern void memcpyha_safe (uaecptr dst, const uae_u8 *src, int size);
extern void memcpyha (uaecptr dst, const uae_u8 *src, int size);
extern void memcpyah_safe (uae_u8 *dst, uaecptr src, int size);
extern void memcpyah (uae_u8 *dst, uaecptr src, int size);

#define UAE_MEMORY_REGIONS_MAX 64
#define UAE_MEMORY_REGION_NAME_LENGTH 64

#define UAE_MEMORY_REGION_RAM (1 << 0)
#define UAE_MEMORY_REGION_ALIAS (1 << 1)
#define UAE_MEMORY_REGION_MIRROR (1 << 2)

/* Get a list of memory regions in the Amiga address space */

typedef struct UaeMemoryRegion {
	uaecptr start;
	uae_u32 size;
	TCHAR name[UAE_MEMORY_REGION_NAME_LENGTH];
	TCHAR rom_name[UAE_MEMORY_REGION_NAME_LENGTH];
	uaecptr alias;
	int flags;
	uae_u8 *memory;
} UaeMemoryRegion;

typedef struct UaeMemoryMap {
	UaeMemoryRegion regions[UAE_MEMORY_REGIONS_MAX];
	int num_regions;
} UaeMemoryMap;

void uae_memory_map(UaeMemoryMap *map);

#endif /* UAE_MEMORY_H */
