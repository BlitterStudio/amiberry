/*
* UAE - The Un*x Amiga Emulator
*
* Memory management
*
* (c) 1995 Bernd Schmidt
*/

#define DEBUG_STUPID 0

// don't touch
#define ACA500x_DEVELOPMENT 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "rommgr.h"
#include "zfile.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "savestate.h"
#include "ar.h"
#include "crc32.h"
#include "gui.h"
#include "cdtv.h"
#include "akiko.h"
#ifdef ARCADIA
#include "arcadia.h"
#endif
#ifdef ENFORCER
#include "enforcer.h"
#endif
#include "gayle.h"
#include "debug.h"
#include "debugmem.h"
#include "cpuboard.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "devices.h"
#ifdef WITH_DRACO
#include "draco.h"
#endif

bool canbang;
uaecptr highest_ram;
static bool rom_write_enabled;
#ifdef JIT
/* Set by each memory handler that does not simply access real memory. */
int special_mem, special_mem_default;
/* do not use get_n_addr */
int jit_n_addr_unsafe;
#endif
static int mem_hardreset;
static bool roms_modified;

#define FLASHEMU 0

static bool isdirectjit (void)
{
	return currprefs.cachesize && !currprefs.comptrustbyte;
}

static bool canjit (void)
{
	if (currprefs.cpu_model < 68020 || currprefs.address_space_24)
		return false;
	return true;
}
static bool needmman (void)
{
	if (!jit_direct_compatible_memory)
		return false;
#ifdef _WIN32
	return true;
#endif
	if (canjit ())
		return true;
	return false;
}

static void nocanbang (void)
{
	if (canbang) {
		write_log(_T("Switching JIT direct off!\n"));
	}
	canbang = 0;
}

uae_u8 ce_banktype[65536];
uae_u8 ce_cachable[65536];

static size_t bootrom_filepos, chip_filepos, bogo_filepos, a3000lmem_filepos, a3000hmem_filepos, mem25bit_filepos;

/* Set if we notice during initialization that settings changed,
and we must clear all memory to prevent bogus contents from confusing
the Kickstart.  */
static bool need_hardreset;
static int bogomem_aliasing;

/* The address space setting used during the last reset.  */
static bool last_address_space_24;

addrbank *mem_banks[MEMORY_BANKS];

/* This has two functions. It either holds a host address that, when added
to the 68k address, gives the host address corresponding to that 68k
address (in which case the value in this array is even), OR it holds the
same value as mem_banks, for those banks that have baseaddr==0. In that
case, bit 0 is set (the memory access routines will take care of it).  */

uae_u8 *baseaddr[MEMORY_BANKS];

#ifdef NO_INLINE_MEMORY_ACCESS
__inline__ uae_u32 longget (uaecptr addr)
{
	return call_mem_get_func (get_mem_bank (addr).lget, addr);
}
__inline__ uae_u32 wordget (uaecptr addr)
{
	return call_mem_get_func (get_mem_bank (addr).wget, addr);
}
__inline__ uae_u32 byteget (uaecptr addr)
{
	return call_mem_get_func (get_mem_bank (addr).bget, addr);
}
__inline__ void longput (uaecptr addr, uae_u32 l)
{
	call_mem_put_func (get_mem_bank (addr).lput, addr, l);
}
__inline__ void wordput (uaecptr addr, uae_u32 w)
{
	call_mem_put_func (get_mem_bank (addr).wput, addr, w);
}
__inline__ void byteput (uaecptr addr, uae_u32 b)
{
	call_mem_put_func (get_mem_bank (addr).bput, addr, b);
}
#endif

#ifdef DEBUGGER
extern bool debugmem_initialized;
#endif

bool real_address_allowed(void)
{
#ifdef DEBUGGER
	return debugmem_initialized == false;
#else
	return true;
#endif
}

int addr_valid (const TCHAR *txt, uaecptr addr, uae_u32 len)
{
	addrbank *ab = &get_mem_bank(addr);
	if (ab == 0 || !(ab->flags & (ABFLAG_RAM | ABFLAG_ROM)) || addr < 0x100 || len > 16777215 || !valid_address (addr, len)) {
		write_log (_T("corrupt %s pointer %x (%d) detected!\n"), txt, addr, len);
		return 0;
	}
	return 1;
}

static int illegal_count;
/* A dummy bank that only contains zeros */

static uae_u32 REGPARAM3 dummy_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dummy_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dummy_bget (uaecptr) REGPARAM;
static void REGPARAM3 dummy_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dummy_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dummy_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 dummy_check (uaecptr addr, uae_u32 size) REGPARAM;

/* fake UAE ROM */

extern addrbank fakeuaebootrom_bank;
MEMORY_FUNCTIONS(fakeuaebootrom);

#define	MAX_ILG 1000
#define NONEXISTINGDATA 0
//#define NONEXISTINGDATA 0xffffffff

static bool map_uae_boot_rom_direct(void)
{
	if (!fakeuaebootrom_bank.allocated_size) {
		fakeuaebootrom_bank.start = 0xf00000;
		fakeuaebootrom_bank.reserved_size = 65536;
		fakeuaebootrom_bank.mask = fakeuaebootrom_bank.reserved_size - 1;
		if (!mapped_malloc (&fakeuaebootrom_bank))
			return false;
		// create jump table to real uae boot rom
		for (int i = 0xff00; i < 0xfff8; i += 8) {
			uae_u8 *p = fakeuaebootrom_bank.baseaddr + i;
			p[0] = 0x4e;
			p[1] = 0xf9;
			uaecptr p2 = rtarea_base + i;
			p[2] = p2 >> 24;
			p[3] = p2 >> 16;
			p[4] = p2 >>  8;
			p[5] = p2 >>  0;
		}
	}
	map_banks(&fakeuaebootrom_bank, 0xf0, 1, 1);
	write_log(_T("Mapped fake UAE Boot ROM jump table.\n"));
	return true;
}

// if access looks like old style hook call and f00000 space is unused, redirect to UAE boot ROM.
static bool maybe_map_boot_rom(uaecptr addr)
{
	if (currprefs.uaeboard >= 2 && get_mem_bank_real(0xf00000) == &dummy_bank && !currprefs.mmu_model) {
		uae_u32 pc = M68K_GETPC;
		if (addr >= 0xf0ff00 && addr <= 0xf0fff8 && (((valid_address(pc, 2) && (pc < 0xf00000 || pc >= 0x01000000) && !currprefs.cpu_compatible) || (pc == addr && currprefs.cpu_compatible)))) {
			bool check2 = currprefs.cpu_compatible;
			if (!check2) {
				uae_u32 w = get_word(pc);
				// JSR xxxxxxxx or JSR (an)
				if (w == 0x4eb9 || (w & 0xfff8) == 0x4e90)
					check2 = true;
			}
			if (check2) {
				if (map_uae_boot_rom_direct()) {
					if (get_mem_bank_real(0xf00000) == &fakeuaebootrom_bank) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

static void dummylog(int rw, uaecptr addr, int size, uae_u32 val, int ins)
{
	/* ignore Zorro3 expansion space */
	if (addr >= AUTOCONFIG_Z3 && addr <= AUTOCONFIG_Z3 + 0x200)
		return;
	/* autoconfig and extended rom */
	if (addr >= 0xe00000 && addr <= 0xf7ffff)
		return;
	/* motherboard ram */
	if (addr >= 0x08000000 && addr <= 0x08000007)
		return;
	if (addr >= 0x07f00000 && addr <= 0x07f00007)
		return;
	if (addr >= 0x07f7fff0 && addr <= 0x07ffffff)
		return;
#ifdef DEBUGGER
	if (debugmem_extinvalidmem(addr, val, rw ? (1 << size) : -(1 << size)))
		return;
	if ((illegal_count >= MAX_ILG && MAX_ILG > 0) && !memwatch_access_validator)
		return;
#endif
	if (MAX_ILG >= 0)
		illegal_count++;
	if (ins) {
		write_log (_T("WARNING: Illegal opcode %cget at %08x PC=%x\n"),
			size == sz_word ? 'w' : 'l', addr, M68K_GETPC);
	} else if (rw) {
		write_log (_T("Illegal %cput at %08x=%08x PC=%x\n"),
			size == sz_byte ? 'b' : size == sz_word ? 'w' : 'l', addr, val, M68K_GETPC);
	} else {
		write_log (_T("Illegal %cget at %08x PC=%x\n"),
			size == sz_byte ? 'b' : size == sz_word ? 'w' : 'l', addr, M68K_GETPC);
	}
}

// 250ms delay
static void gary_wait(uaecptr addr, int size, bool write)
{
	static int cnt = 50;

#if 0
	int lines = 313 * 12;
	while (lines-- > 0)
		x_do_cycles(228 * CYCLE_UNIT);
#endif

	if (cnt > 0) {
		write_log (_T("Gary timeout: %08x %d %c PC=%08x\n"), addr, size, write ? 'W' : 'R', M68K_GETPC);
		cnt--;
	}
}

static bool gary_nonrange(uaecptr addr)
{
	if (currprefs.cs_fatgaryrev < 0)
		return false;
	if (addr < 0xb80000)
		return false;
	if (addr >= 0xd00000 && addr < 0xdc0000)
		return true;
	if (addr >= 0xdd0000 && addr < 0xde0000)
		return true;
	if (addr >= 0xdf8000 && addr < 0xe00000)
		return false;
	if (addr >= 0xe80000 && addr < 0xf80000)
		return false;
	if (expansion_get_autoconfig_by_address(&currprefs, addr, 0)) {
		return false;
	}
	return true;
}

void dummy_put (uaecptr addr, int size, uae_u32 val)
{
#if FLASHEMU
	if (addr >= 0xf00000 && addr < 0xf80000 && size < 2)
		flash_write(addr, val);
#endif

	if (gary_nonrange(addr) || (size > sz_byte && gary_nonrange(addr + (1 << size) - 1))) {
		if (gary_timeout)
			gary_wait(addr, size, true);
		if (gary_toenb && currprefs.mmu_model)
			hardware_exception2(addr, val, false, false, size);
	}
}

static uae_u32 nonexistingdata(void)
{
	if (currprefs.cs_unmapped_space == 1)
		return 0x00000000;
	if (currprefs.cs_unmapped_space == 2)
		return 0xffffffff;
	return NONEXISTINGDATA;
}

uae_u32 dummy_get_safe(uaecptr addr, int size, bool inst, uae_u32 defvalue)
{
	uae_u32 v = defvalue;
	uae_u32 mask = size == sz_long ? 0xffffffff : (1 << ((1 << size) * 8)) - 1;
	if (currprefs.cpu_model >= 68040)
		return v & mask;
	if (!currprefs.cpu_compatible)
		return v & mask;
	if (currprefs.address_space_24)
		addr &= 0x00ffffff;
	if (addr >= 0x10000000)
		return v & mask;
	// CD32 and B2000
	if (currprefs.cs_unmapped_space == 1)
		return 0;
	if (currprefs.cs_unmapped_space == 2)
		return 0xffffffff & mask;
	if ((currprefs.cpu_model <= 68010) || (currprefs.cpu_model == 68020 && (currprefs.chipset_mask & CSMASK_AGA) && currprefs.address_space_24)) {
		// if executed from ROM: always return zero
		uaecptr pc = m68k_getpc();
		addrbank *ab = &get_mem_bank(pc);
		if (ab->flags & ABFLAG_ROM) {
			return 0;
		}
		if (size == sz_long) {
			v = regs.irc & 0xffff;
			if (addr & 1)
				v = (v << 8) | (v >> 8);
			v = (v << 16) | v;
		} else if (size == sz_word) {
			v = regs.irc & 0xffff;
			if (addr & 1)
				v = (v << 8) | (v >> 8);
		} else {
			v = regs.irc;
			v = (addr & 1) ? (v & 0xff) : ((v >> 8) & 0xff);
		}
	}
	return v & mask;
}

uae_u32 dummy_get (uaecptr addr, int size, bool inst, uae_u32 defvalue)
{
	uae_u32 v = defvalue;

#if FLASHEMU
	if (addr >= 0xf00000 && addr < 0xf80000 && size < 2) {
		if (addr < 0xf60000)
			return flash_read(addr);
		return 8;
	}
#endif
#if ACA500x_DEVELOPMENT
	if (addr == 0xb03000) {
		return 0xffff;
	}
	if (addr == 0xb07000) {
		return 0x0000;
	}
	if (addr == 0xb2f800) {
		return 0xffff;
	}
	if (addr == 0xb3b800) {
		return 0x0000;
	}
	if (addr == 0xb3f800) {
		return currprefs.cpu_model > 68000 ? 0x0000 : 0xffff;
	}
	if (addr == 0xb0b000) {
		return isideint() ? 0xffff : 0x0000;
	}
#endif

	if ((size == sz_word || size == sz_long) && inst && maybe_map_boot_rom(addr)) {
		if (size == sz_word)
			return get_word(addr);
		return get_long(addr);
	}

	if (gary_nonrange(addr) || (size > sz_byte && gary_nonrange(addr + (1 << size) - 1))) {
		if (gary_timeout)
			gary_wait(addr, size, false);
		if (gary_toenb)
			hardware_exception2(addr, 0, true, false, size);
		return v;
	}

	return dummy_get_safe(addr, size, inst, defvalue);
}

static uae_u32 REGPARAM2 dummy_lget (uaecptr addr)
{
	if (currprefs.illegal_mem)
		dummylog(0, addr, sz_long, 0, 0);
	return dummy_get(addr, sz_long, false, nonexistingdata());
}
uae_u32 REGPARAM2 dummy_lgeti (uaecptr addr)
{
	if (currprefs.illegal_mem)
		dummylog(0, addr, sz_long, 0, 1);
	return dummy_get(addr, sz_long, true, nonexistingdata());
}

static uae_u32 REGPARAM2 dummy_wget (uaecptr addr)
{
	if (currprefs.illegal_mem)
		dummylog(0, addr, sz_word, 0, 0);
	return dummy_get(addr, sz_word, false, nonexistingdata());
}
uae_u32 REGPARAM2 dummy_wgeti (uaecptr addr)
{
	if (currprefs.illegal_mem)
		dummylog(0, addr, sz_word, 0, 1);
	return dummy_get(addr, sz_word, true, nonexistingdata());
}

static uae_u32 REGPARAM2 dummy_bget (uaecptr addr)
{
	if (currprefs.illegal_mem)
		dummylog(0, addr, sz_byte, 0, 0);
	return dummy_get(addr, sz_byte, false, nonexistingdata());
}

static void REGPARAM2 dummy_lput (uaecptr addr, uae_u32 l)
{
	if (currprefs.illegal_mem)
		dummylog(1, addr, sz_long, l, 0);
	dummy_put(addr, sz_long, l);
}
static void REGPARAM2 dummy_wput (uaecptr addr, uae_u32 w)
{
	if (currprefs.illegal_mem)
		dummylog(1, addr, sz_word, w, 0);
	dummy_put(addr, sz_word, w);
}
static void REGPARAM2 dummy_bput (uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		dummylog(1, addr, sz_byte, b, 0);
	dummy_put(addr, sz_byte, b);
}

static int REGPARAM2 dummy_check (uaecptr addr, uae_u32 size)
{
	return 0;
}

static void REGPARAM2 none_put (uaecptr addr, uae_u32 v)
{
}
static uae_u32 REGPARAM2 ones_get (uaecptr addr)
{
	return 0xffffffff;
}

addrbank *get_sub_bank(uaecptr *paddr)
{
	int i;
	uaecptr addr = *paddr;
	addrbank *ab = &get_mem_bank(addr);
	struct addrbank_sub *sb = ab->sub_banks;
	if (!sb)
		return &dummy_bank;
	for (i = 0; sb[i].bank; i++) {
		int offset = addr & 65535;
		if (offset < sb[i + 1].offset) {
			uae_u32 mask = sb[i].mask;
			uae_u32 maskval = sb[i].maskval;
			if ((offset & mask) == maskval) {
				*paddr = addr - sb[i].suboffset;
				return sb[i].bank;
			}
		}
	}
	*paddr = addr - sb[i - 1].suboffset;
	return sb[i - 1].bank;
}
uae_u32 REGPARAM3 sub_bank_lget (uaecptr addr) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	return ab->lget(addr);
}
uae_u32 REGPARAM3 sub_bank_wget(uaecptr addr) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	return ab->wget(addr);
}
uae_u32 REGPARAM3 sub_bank_bget(uaecptr addr) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	return ab->bget(addr);
}
void REGPARAM3 sub_bank_lput(uaecptr addr, uae_u32 v) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	ab->lput(addr, v);
}
void REGPARAM3 sub_bank_wput(uaecptr addr, uae_u32 v) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	ab->wput(addr, v);
}
void REGPARAM3 sub_bank_bput(uaecptr addr, uae_u32 v) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	ab->bput(addr, v);
}
uae_u32 REGPARAM3 sub_bank_lgeti(uaecptr addr) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	return ab->lgeti(addr);
}
uae_u32 REGPARAM3 sub_bank_wgeti(uaecptr addr) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	return ab->wgeti(addr);
}
int REGPARAM3 sub_bank_check(uaecptr addr, uae_u32 size) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	return ab->check(addr, size);
}
uae_u8 *REGPARAM3 sub_bank_xlate(uaecptr addr) REGPARAM
{
	addrbank *ab = get_sub_bank(&addr);
	return ab->xlateaddr(addr);
}


/* Chip memory */

static uae_u32 chipmem_full_mask;
static uae_u32 chipmem_full_size;

static int REGPARAM3 chipmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 chipmem_xlate (uaecptr addr) REGPARAM;

#ifdef AGA

/* AGA ce-chipram access */

static void ce2_timeout (void)
{
#ifdef CPUEMU_13
	wait_cpu_cycle_read (0, -1);
#endif
}

static uae_u32 REGPARAM2 chipmem_lget_ce2 (uaecptr addr)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	ce2_timeout ();
	return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 chipmem_wget_ce2 (uaecptr addr)
{
	uae_u16 *m, v;

	addr &= chipmem_bank.mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	ce2_timeout ();
	v = do_get_mem_word (m);
	return v;
}

static uae_u32 REGPARAM2 chipmem_bget_ce2 (uaecptr addr)
{
	addr &= chipmem_bank.mask;
	ce2_timeout ();
	return chipmem_bank.baseaddr[addr];
}

static void REGPARAM2 chipmem_lput_ce2 (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	ce2_timeout ();
	do_put_mem_long (m, l);
}

static void REGPARAM2 chipmem_wput_ce2 (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	ce2_timeout ();
	do_put_mem_word (m, w);
}

static void REGPARAM2 chipmem_bput_ce2 (uaecptr addr, uae_u32 b)
{
	addr &= chipmem_bank.mask;
	ce2_timeout ();
	chipmem_bank.baseaddr[addr] = b;
}

#endif

static uae_u32 chipmem_noise(uae_u32 addr)
{
	// not yet implemented
	return 0;
}

static uae_u32 REGPARAM2 chipmem_lget_limit(uaecptr addr)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	if (addr >= 0x180000 - 3) {
		return chipmem_noise(addr);
	}
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	return do_get_mem_long(m);
}

static uae_u32 REGPARAM2 chipmem_wget_limit(uaecptr addr)
{
	uae_u16 *m, v;

	addr &= chipmem_bank.mask;
	if (addr >= 0x180000 - 1) {
		return chipmem_noise(addr);
	}
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	v = do_get_mem_word(m);
	return v;
}

static uae_u32 REGPARAM2 chipmem_bget_limit(uaecptr addr)
{
	uae_u8 v;
	addr &= chipmem_bank.mask;
	if (addr >= 0x180000) {
		return chipmem_noise(addr);
	}
	v = chipmem_bank.baseaddr[addr];
	return v;
}

static void REGPARAM2 chipmem_lput_limit(uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	if (addr >= 0x180000) {
		return;
	}
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_long(m, l);
}

static void REGPARAM2 chipmem_wput_limit(uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr &= chipmem_bank.mask;
	if (addr >= 0x180000) {
		return;
	}
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_word(m, w);
}

static void REGPARAM2 chipmem_bput_limit(uaecptr addr, uae_u32 b)
{
	addr &= chipmem_bank.mask;
	if (addr >= 0x180000) {
		return;
	}
	chipmem_bank.baseaddr[addr] = b;
}


static uae_u32 REGPARAM2 chipmem_lget (uaecptr addr)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 chipmem_wget (uaecptr addr)
{
	uae_u16 *m, v;

	addr &= chipmem_bank.mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	v = do_get_mem_word (m);
	return v;
}

static uae_u32 REGPARAM2 chipmem_bget (uaecptr addr)
{
	uae_u8 v;
	addr &= chipmem_bank.mask;
	v = chipmem_bank.baseaddr[addr];
	return v;
}

void REGPARAM2 chipmem_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_long (m, l);
}

void REGPARAM2 chipmem_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_word (m, w);
}

void REGPARAM2 chipmem_bput (uaecptr addr, uae_u32 b)
{
	addr &= chipmem_bank.mask;
	chipmem_bank.baseaddr[addr] = b;
}

/* cpu chipmem access inside agnus addressable ram but no ram available */
static uae_u32 chipmem_dummy (void)
{
	/* not really right but something random that has more ones than zeros.. */
	return 0xffff & ~((1 << (uaerand () & 31)) | (1 << (uaerand () & 31)));
}

static void REGPARAM2 chipmem_dummy_bput (uaecptr addr, uae_u32 b)
{
}
static void REGPARAM2 chipmem_dummy_wput (uaecptr addr, uae_u32 b)
{
}
static void REGPARAM2 chipmem_dummy_lput (uaecptr addr, uae_u32 b)
{
}

static uae_u32 REGPARAM2 chipmem_dummy_bget (uaecptr addr)
{
	return chipmem_dummy ();
}
static uae_u32 REGPARAM2 chipmem_dummy_wget (uaecptr addr)
{
	return chipmem_dummy ();
}
static uae_u32 REGPARAM2 chipmem_dummy_lget (uaecptr addr)
{
	return (chipmem_dummy () << 16) | chipmem_dummy ();
}

static uae_u32 REGPARAM2 chipmem_agnus_lget (uaecptr addr)
{
	uae_u32 *m;

	addr &= chipmem_full_mask;
	if (addr >= chipmem_full_size - 3)
		return chipmem_noise(addr);
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	return do_get_mem_long (m);
}

uae_u32 REGPARAM2 chipmem_agnus_wget (uaecptr addr)
{
	uae_u16 *m;

	addr &= chipmem_full_mask;
	if (addr >= chipmem_full_size - 1)
		return chipmem_noise(addr);
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 chipmem_agnus_bget (uaecptr addr)
{
	addr &= chipmem_full_mask;
	if (addr >= chipmem_full_size)
		return chipmem_noise(addr);
	return chipmem_bank.baseaddr[addr];
}

static void REGPARAM2 chipmem_agnus_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	addr &= chipmem_full_mask;
	if (addr >= chipmem_full_size - 3)
		return;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_long (m, l);
}

void REGPARAM2 chipmem_agnus_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr &= chipmem_full_mask;
	if (addr >= chipmem_full_size - 1)
		return;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_word (m, w);
}

static void REGPARAM2 chipmem_agnus_bput (uaecptr addr, uae_u32 b)
{
	addr &= chipmem_full_mask;
	if (addr >= chipmem_full_size)
		return;
	chipmem_bank.baseaddr[addr] = b;
}

static int REGPARAM2 chipmem_check (uaecptr addr, uae_u32 size)
{
	// Check if chip ram is in two "banks" (ECS 0.5m+0.5m config)
	if (((addr & ~chipmem_bank.mask) & chipmem_full_mask) || (((addr + size) & ~chipmem_bank.mask) & chipmem_full_mask)) {
		return 0;
	}
	addr &= chipmem_bank.mask;
	return (addr + size) <= chipmem_full_size;
}

static uae_u8 *REGPARAM2 chipmem_xlate (uaecptr addr)
{
	addr &= chipmem_bank.mask;
	return chipmem_bank.baseaddr + addr;
}

STATIC_INLINE void REGPARAM2 chipmem_lput_bigmem (uaecptr addr, uae_u32 v)
{
	put_long (addr, v);
}
STATIC_INLINE void REGPARAM2 chipmem_wput_bigmem (uaecptr addr, uae_u32 v)
{
	put_word (addr, v);
}
STATIC_INLINE void REGPARAM2 chipmem_bput_bigmem (uaecptr addr, uae_u32 v)
{
	put_byte (addr, v);
}
STATIC_INLINE uae_u32 REGPARAM2 chipmem_lget_bigmem (uaecptr addr)
{
	return get_long (addr);
}
STATIC_INLINE uae_u32 REGPARAM2 chipmem_wget_bigmem (uaecptr addr)
{
	return get_word (addr);
}
STATIC_INLINE uae_u32 REGPARAM2 chipmem_bget_bigmem (uaecptr addr)
{
	return get_byte (addr);
}
STATIC_INLINE int REGPARAM2 chipmem_check_bigmem (uaecptr addr, uae_u32 size)
{
	return valid_address (addr, size);
}
STATIC_INLINE uae_u8* REGPARAM2 chipmem_xlate_bigmem (uaecptr addr)
{
	return get_real_address (addr);
}

STATIC_INLINE void REGPARAM2 chipmem_lput_debugmem(uaecptr addr, uae_u32 v)
{
#ifdef DEBUGGER
	if (addr < debugmem_chiplimit)
		debugmem_chiphit(addr, v, 4);
#endif
	put_long(addr, v);
}
STATIC_INLINE void REGPARAM2 chipmem_wput_debugmem(uaecptr addr, uae_u32 v)
{
#ifdef DEBUGGER
	if (addr < debugmem_chiplimit)
		debugmem_chiphit(addr, v, 2);
#endif
	put_word(addr, v);
}
STATIC_INLINE void REGPARAM2 chipmem_bput_debugmem(uaecptr addr, uae_u32 v)
{
#ifdef DEBUGGER
	if (addr < debugmem_chiplimit)
		debugmem_chiphit(addr, v, 1);
#endif
	put_byte(addr, v);
}
STATIC_INLINE uae_u32 REGPARAM2 chipmem_lget_debugmem(uaecptr addr)
{
#ifdef DEBUGGER
	if (addr < debugmem_chiplimit)
		return debugmem_chiphit(addr, 0, -4);
#endif
	return get_long(addr);
}
STATIC_INLINE uae_u32 REGPARAM2 chipmem_wget_debugmem(uaecptr addr)
{
#ifdef DEBUGGER
	if (addr < debugmem_chiplimit)
		return debugmem_chiphit(addr, 0, -2);
#endif
	return get_word(addr);
}
STATIC_INLINE uae_u32 REGPARAM2 chipmem_bget_debugmem(uaecptr addr)
{
#ifdef DEBUGGER
	if (addr < debugmem_chiplimit)
		return debugmem_chiphit(addr, 0, -1);
#endif
	return get_byte(addr);
}
STATIC_INLINE int REGPARAM2 chipmem_check_debugmem(uaecptr addr, uae_u32 size)
{
	return valid_address(addr, size);
}
STATIC_INLINE uae_u8* REGPARAM2 chipmem_xlate_debugmem(uaecptr addr)
{
	return get_real_address(addr);
}

uae_u32 (REGPARAM2 *chipmem_lget_indirect)(uaecptr);
uae_u32 (REGPARAM2 *chipmem_wget_indirect)(uaecptr);
uae_u32 (REGPARAM2 *chipmem_bget_indirect)(uaecptr);
void (REGPARAM2 *chipmem_lput_indirect)(uaecptr, uae_u32);
void (REGPARAM2 *chipmem_wput_indirect)(uaecptr, uae_u32);
void (REGPARAM2 *chipmem_bput_indirect)(uaecptr, uae_u32);
int (REGPARAM2 *chipmem_check_indirect)(uaecptr, uae_u32);
uae_u8 *(REGPARAM2 *chipmem_xlate_indirect)(uaecptr);

void chipmem_setindirect(void)
{
#ifdef DEBUGGER
	if (debugmem_bank.baseaddr && debugmem_chiplimit) {
#else
	if (debugmem_bank.baseaddr && 0) {
#endif
		chipmem_lget_indirect = chipmem_lget_debugmem;
		chipmem_wget_indirect = chipmem_wget_debugmem;
		chipmem_bget_indirect = chipmem_bget_debugmem;
		chipmem_lput_indirect = chipmem_lput_debugmem;
		chipmem_wput_indirect = chipmem_wput_debugmem;
		chipmem_bput_indirect = chipmem_bput_debugmem;
		chipmem_check_indirect = chipmem_check_bigmem;
		chipmem_xlate_indirect = chipmem_xlate_bigmem;
	} else if (currprefs.z3chipmem.size) {
		chipmem_lget_indirect = chipmem_lget_bigmem;
		chipmem_wget_indirect = chipmem_wget_bigmem;
		chipmem_bget_indirect = chipmem_bget_bigmem;
		chipmem_lput_indirect = chipmem_lput_bigmem;
		chipmem_wput_indirect = chipmem_wput_bigmem;
		chipmem_bput_indirect = chipmem_bput_bigmem;
		chipmem_check_indirect = chipmem_check_bigmem;
		chipmem_xlate_indirect = chipmem_xlate_bigmem;
	} else {
		chipmem_lget_indirect = chipmem_lget;
		chipmem_wget_indirect = chipmem_agnus_wget;
		chipmem_bget_indirect = chipmem_agnus_bget;
		chipmem_lput_indirect = chipmem_lput;
		chipmem_wput_indirect = chipmem_agnus_wput;
		chipmem_bput_indirect = chipmem_agnus_bput;
		chipmem_check_indirect = chipmem_check;
		chipmem_xlate_indirect = chipmem_xlate;
	}

	if (currprefs.chipmem.size == 0x180000) {
		chipmem_bank.bget = chipmem_bget_limit;
		chipmem_bank.wget = chipmem_wget_limit;
		chipmem_bank.lget = chipmem_lget_limit;
		chipmem_bank.bput = chipmem_bput_limit;
		chipmem_bank.wput = chipmem_wput_limit;
		chipmem_bank.lput = chipmem_lput_limit;
	} else {
		chipmem_bank.bget = chipmem_bget;
		chipmem_bank.wget = chipmem_wget;
		chipmem_bank.lget = chipmem_lget;
		chipmem_bank.bput = chipmem_bput;
		chipmem_bank.wput = chipmem_wput;
		chipmem_bank.lput = chipmem_lput;
	}
}

/* Slow memory */

MEMORY_FUNCTIONS(bogomem);

/* A3000 motherboard fast memory */

MEMORY_FUNCTIONS(a3000lmem);
MEMORY_FUNCTIONS(a3000hmem);

/* 25bit memory (0x01000000) */

MEMORY_FUNCTIONS(mem25bit);

/* debugger memory */

MEMORY_FUNCTIONS(debugmem);
MEMORY_WGETI(debugmem);
MEMORY_LGETI(debugmem);

/* Kick memory */

uae_u16 kickstart_version;

/*
* A1000 kickstart RAM handling
*
* RESET instruction unhides boot ROM and disables write protection
* write access to boot ROM hides boot ROM and enables write protection
*
*/
static int a1000_kickstart_mode;
static uaecptr a1000_kickstart_size;
static uae_u8 *a1000_bootrom;
static void a1000_handle_kickstart (int mode)
{
	if (!a1000_bootrom)
		return;
	protect_roms (false);
	if (mode == 0) {
		a1000_kickstart_mode = 0;
		if (a1000_kickstart_size == ROM_SIZE_256) {
			memcpy (kickmem_bank.baseaddr, kickmem_bank.baseaddr + ROM_SIZE_256, ROM_SIZE_256);
		}
		kickstart_version = (kickmem_bank.baseaddr[a1000_kickstart_size + 12] << 8) | kickmem_bank.baseaddr[a1000_kickstart_size + 13];
		kickmem_bank.baseaddr_direct_r = kickmem_bank.baseaddr;
	} else {
		a1000_kickstart_mode = 1;
		kickstart_version = 0;
		kickmem_bank.baseaddr_direct_r = NULL;
	}
	if (kickstart_version == 0xffff)
		kickstart_version = 0;
}

void a1000_reset (void)
{
	a1000_handle_kickstart (1);
}

static void REGPARAM3 kickmem_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_bput(uaecptr, uae_u32) REGPARAM;
static uae_u32 REGPARAM3 kickmem_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 kickmem_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 kickmem_bget(uaecptr) REGPARAM;

MEMORY_CHECK(kickmem);
MEMORY_XLATE(kickmem);

extern bool get_power_led(void);

static uaecptr get_a1000_addr(uaecptr addr)
{
	bool led = get_power_led();
	addr -= led && a1000_kickstart_size == ROM_SIZE_512 ? ROM_SIZE_256 : 0;
	return addr;
}

static uae_u32 REGPARAM2 kickmem_lget(uaecptr addr)
{
	uae_u32 *p;
	if (a1000_kickstart_mode) {
		if (addr >= 0x1000000 - ROM_SIZE_256) {
			addr = get_a1000_addr(addr);
			addr &= kickmem_bank.mask;
			p = (uae_u32*)(kickmem_bank.baseaddr + addr);
		} else {
			addr &= kickmem_bank.mask;
			p = (uae_u32*)(a1000_bootrom + addr);
		}
	} else {
		addr &= kickmem_bank.mask;
		p = (uae_u32*)(kickmem_bank.baseaddr + addr);
	}
	uae_u32 m = do_get_mem_long(p);
#ifdef DEBUGGER
	if (debug_dma) {
		record_rom_access(kickmem_bank.start + addr, m, 4, false);
	}
#endif
	return m;
}
static uae_u32 REGPARAM2 kickmem_wget(uaecptr addr)
{
	uae_u16 *p;
	if (a1000_kickstart_mode) {
		if (addr >= 0x1000000 - ROM_SIZE_256) {
			addr = get_a1000_addr(addr);
			addr &= kickmem_bank.mask;
			p = (uae_u16*)(kickmem_bank.baseaddr + addr);
		} else {
			addr &= kickmem_bank.mask;
			p = (uae_u16*)(a1000_bootrom + addr);
		}
	} else {
		addr &= kickmem_bank.mask;
		p = (uae_u16*)(kickmem_bank.baseaddr + addr);
	}
	uae_u16 m = do_get_mem_word(p);
#ifdef DEBUGGER
	if (debug_dma) {
		record_rom_access(kickmem_bank.start + addr, m, 2, false);
	}
#endif
	return m;
}
static uae_u32 REGPARAM2 kickmem_bget(uaecptr addr)
{
	uae_u8 *p;
	if (a1000_kickstart_mode) {
		if (addr >= 0x1000000 - ROM_SIZE_256) {
			addr = get_a1000_addr(addr);
			addr &= kickmem_bank.mask;
			p = (uae_u8*)(kickmem_bank.baseaddr + addr);
		} else {
			addr &= kickmem_bank.mask;
			p = (uae_u8*)(a1000_bootrom + addr);
		}
	} else {
		addr &= kickmem_bank.mask;
		p = (uae_u8*)(kickmem_bank.baseaddr + addr);
	}
	uae_u8 m = *p;
#ifdef DEBUGGER
	if (debug_dma) {
		record_rom_access(kickmem_bank.start + addr, m, 1, false);
	}
#endif
	return m;
}

static void REGPARAM2 kickmem_lput (uaecptr addr, uae_u32 b)
{
	uae_u32 *m;
#ifdef DEBUGGER
	if (debug_dma) {
		record_rom_access(kickmem_bank.start + addr, b, 4, true);
	}
#endif
	if (currprefs.rom_readwrite && rom_write_enabled) {
		addr &= kickmem_bank.mask;
		m = (uae_u32*)(kickmem_bank.baseaddr + addr);
		do_put_mem_long (m, b);
#if 0
		if (addr == ROM_SIZE_512-4) {
			rom_write_enabled = false;
			write_log (_T("ROM write disabled\n"));
		}
#endif
	} else if (a1000_kickstart_mode) {
		if (addr >= 0x1000000 - ROM_SIZE_256) {
			addr = get_a1000_addr(addr);
			addr &= kickmem_bank.mask;
			m = (uae_u32*)(kickmem_bank.baseaddr + addr);
			do_put_mem_long(m, b);
			return;
		} else
			a1000_handle_kickstart (0);
	} else if (currprefs.illegal_mem) {
		write_log (_T("Illegal kickmem lput at %08x PC=%08x\n"), addr, M68K_GETPC);
	}
}

static void REGPARAM2 kickmem_wput (uaecptr addr, uae_u32 b)
{
	uae_u16 *m;
#ifdef DEBUGGER
	if (debug_dma) {
		record_rom_access(kickmem_bank.start + addr, b, 2, true);
	}
#endif
	if (currprefs.rom_readwrite && rom_write_enabled) {
		addr &= kickmem_bank.mask;
		m = (uae_u16 *)(kickmem_bank.baseaddr + addr);
		do_put_mem_word (m, b);
	} else if (a1000_kickstart_mode) {
		if (addr >= 0x1000000 - ROM_SIZE_256) {
			addr = get_a1000_addr(addr);
			addr &= kickmem_bank.mask;
			m = (uae_u16*)(kickmem_bank.baseaddr + addr);
			do_put_mem_word (m, b);
			return;
		} else
			a1000_handle_kickstart (0);
	} else if (currprefs.illegal_mem) {
		write_log (_T("Illegal kickmem wput at %08x PC=%08x\n"), addr, M68K_GETPC);
	}
}

static void REGPARAM2 kickmem_bput (uaecptr addr, uae_u32 b)
{
#ifdef DEBUGGER
	if (debug_dma) {
		record_rom_access(kickmem_bank.start + addr, b, 1, true);
	}
#endif
	if (currprefs.rom_readwrite && rom_write_enabled) {
		addr &= kickmem_bank.mask;
		kickmem_bank.baseaddr[addr] = b;
	} else if (a1000_kickstart_mode) {
		if (addr >= 0x1000000 - ROM_SIZE_256) {
			addr = get_a1000_addr(addr);
			addr &= kickmem_bank.mask;
			kickmem_bank.baseaddr[addr] = b;
			return;
		} else {
			a1000_handle_kickstart (0);
		}
	} else if (currprefs.illegal_mem) {
		write_log (_T("Illegal kickmem bput at %08x PC=%08x\n"), addr, M68K_GETPC);
	}
}

static void REGPARAM2 kickmem2_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr &= kickmem_bank.mask;
	m = (uae_u32 *)(kickmem_bank.baseaddr + addr);
	do_put_mem_long (m, l);
}

static void REGPARAM2 kickmem2_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;
	addr &= kickmem_bank.mask;
	m = (uae_u16 *)(kickmem_bank.baseaddr + addr);
	do_put_mem_word (m, w);
}

static void REGPARAM2 kickmem2_bput (uaecptr addr, uae_u32 b)
{
	addr &= kickmem_bank.mask;
	kickmem_bank.baseaddr[addr] = b;
}

/* CD32/CDTV extended kick memory */

static int extendedkickmem_type;

#define EXTENDED_ROM_CD32 1
#define EXTENDED_ROM_CDTV 2
#define EXTENDED_ROM_KS 3
#define EXTENDED_ROM_ARCADIA 4
#define EXTENDED_ROM_ALG 5

static void REGPARAM3 extendedkickmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem_bput (uaecptr, uae_u32) REGPARAM;

MEMORY_BGET(extendedkickmem);
MEMORY_WGET(extendedkickmem);
MEMORY_LGET(extendedkickmem);
MEMORY_CHECK(extendedkickmem);
MEMORY_XLATE(extendedkickmem);

static void REGPARAM2 extendedkickmem_lput (uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log (_T("Illegal extendedkickmem lput at %08x\n"), addr);
}
static void REGPARAM2 extendedkickmem_wput (uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log (_T("Illegal extendedkickmem wput at %08x\n"), addr);
}
static void REGPARAM2 extendedkickmem_bput (uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log (_T("Illegal extendedkickmem lput at %08x\n"), addr);
}


static void REGPARAM3 extendedkickmem2a_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2a_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2a_bput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2b_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2b_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2b_bput(uaecptr, uae_u32) REGPARAM;

MEMORY_BGET(extendedkickmem2a);
MEMORY_WGET(extendedkickmem2a);
MEMORY_LGET(extendedkickmem2a);
MEMORY_CHECK(extendedkickmem2a);
MEMORY_XLATE(extendedkickmem2a);

MEMORY_BGET(extendedkickmem2b);
MEMORY_WGET(extendedkickmem2b);
MEMORY_LGET(extendedkickmem2b);
MEMORY_CHECK(extendedkickmem2b);
MEMORY_XLATE(extendedkickmem2b);

static void REGPARAM2 extendedkickmem2a_lput (uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log (_T("Illegal extendedkickmem2a lput at %08x\n"), addr);
}
static void REGPARAM2 extendedkickmem2a_wput (uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log (_T("Illegal extendedkickmem2a wput at %08x\n"), addr);
}
static void REGPARAM2 extendedkickmem2a_bput (uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log (_T("Illegal extendedkickmem2a lput at %08x\n"), addr);
}

static void REGPARAM2 extendedkickmem2b_lput(uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log(_T("Illegal extendedkickmem2b lput at %08x\n"), addr);
}
static void REGPARAM2 extendedkickmem2b_wput(uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log(_T("Illegal extendedkickmem2b wput at %08x\n"), addr);
}
static void REGPARAM2 extendedkickmem2b_bput(uaecptr addr, uae_u32 b)
{
	if (currprefs.illegal_mem)
		write_log(_T("Illegal extendedkickmem2b lput at %08x\n"), addr);
}


/* Default memory access functions */

int REGPARAM2 default_check (uaecptr a, uae_u32 b)
{
	return 0;
}

static int be_cnt, be_recursive;

uae_u8 *REGPARAM2 default_xlate (uaecptr addr)
{
	if (be_recursive) {
		cpu_halt(CPU_HALT_OPCODE_FETCH_FROM_NON_EXISTING_ADDRESS);
		return kickmem_xlate(2);
	}

	if (maybe_map_boot_rom(addr))
		return get_real_address(addr);

	be_recursive++;
	int size = currprefs.cpu_model >= 68020 ? 4 : 2;
	if (quit_program == 0) {
		/* do this only in 68010+ mode, there are some tricky A500 programs.. */
		if ((currprefs.cpu_model > 68000 || !currprefs.cpu_compatible) && !currprefs.mmu_model) {
#if defined(ENFORCER)
			enforcer_disable ();
#endif
			if (be_cnt < 3) {
				int i, j;
				uaecptr a2 = addr - 32;
				uaecptr a3 = m68k_getpc () - 32;
				write_log (_T("Your Amiga program just did something terribly stupid %08X PC=%08X\n"), addr, M68K_GETPC);
#ifdef DEBUGGER
				if (debugging || DEBUG_STUPID) {
					activate_debugger();
					m68k_dumpstate(NULL, 0xffffffff);
				}
#endif
				for (i = 0; i < 10; i++) {
					write_log (_T("%08X "), i >= 5 ? a3 : a2);
					for (j = 0; j < 16; j += 2) {
						write_log (_T(" %04X"), get_word (i >= 5 ? a3 : a2));
						if (i >= 5) a3 += 2; else a2 += 2;
					}
					write_log (_T("\n"));
				}
#ifdef DEBUGGER
				memory_map_dump();
#endif
			}
			if (0 || (gary_toenb && (gary_nonrange(addr) || (size > 1 && gary_nonrange(addr + size - 1))))) {
				hardware_exception2(addr, 0, true, true, size);
			} else {
				cpu_halt(CPU_HALT_OPCODE_FETCH_FROM_NON_EXISTING_ADDRESS);
			}
		}
	}
	be_recursive--;
	return kickmem_xlate (2); /* So we don't crash. */
}

/* Address banks */

addrbank dummy_bank = {
	dummy_lget, dummy_wget, dummy_bget,
	dummy_lput, dummy_wput, dummy_bput,
	default_xlate, dummy_check, NULL, NULL, NULL,
	dummy_lgeti, dummy_wgeti,
	ABFLAG_NONE, S_READ, S_WRITE
};

addrbank ones_bank = {
	ones_get, ones_get, ones_get,
	none_put, none_put, none_put,
	default_xlate, dummy_check, NULL, NULL, _T("Ones"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_NONE, S_READ, S_WRITE
};

addrbank chipmem_bank = {
	chipmem_lget, chipmem_wget, chipmem_bget,
	chipmem_lput, chipmem_wput, chipmem_bput,
	chipmem_xlate, chipmem_check, NULL, _T("chip"), _T("Chip memory"),
	chipmem_lget, chipmem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CHIPRAM | ABFLAG_CACHE_ENABLE_BOTH, 0, 0
};

addrbank chipmem_dummy_bank = {
	chipmem_dummy_lget, chipmem_dummy_wget, chipmem_dummy_bget,
	chipmem_dummy_lput, chipmem_dummy_wput, chipmem_dummy_bput,
	default_xlate, dummy_check, NULL, NULL, _T("Dummy Chip memory"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_CHIPRAM, S_READ, S_WRITE
};


#ifdef AGA
addrbank chipmem_bank_ce2 = {
	chipmem_lget_ce2, chipmem_wget_ce2, chipmem_bget_ce2,
	chipmem_lput_ce2, chipmem_wput_ce2, chipmem_bput_ce2,
	chipmem_xlate, chipmem_check, NULL, NULL, _T("Chip memory (68020 'ce')"),
	chipmem_lget_ce2, chipmem_wget_ce2,
	ABFLAG_RAM | ABFLAG_CHIPRAM | ABFLAG_CACHE_ENABLE_BOTH, S_READ, S_WRITE
};
#endif

addrbank bogomem_bank = {
	bogomem_lget, bogomem_wget, bogomem_bget,
	bogomem_lput, bogomem_wput, bogomem_bput,
	bogomem_xlate, bogomem_check, NULL, _T("bogo"), _T("Slow memory"),
	bogomem_lget, bogomem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_BOTH, 0, 0
};

addrbank mem25bit_bank = {
	mem25bit_lget, mem25bit_wget, mem25bit_bget,
	mem25bit_lput, mem25bit_wput, mem25bit_bput,
	mem25bit_xlate, mem25bit_check, NULL, _T("25bitmem"), _T("25bit memory"),
	mem25bit_lget, mem25bit_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL, 0, 0
};

addrbank debugmem_bank = {
	debugmem_lget, debugmem_wget, debugmem_bget,
	debugmem_lput, debugmem_wput, debugmem_bput,
	debugmem_xlate, debugmem_check, NULL, _T("debugmem"), _T("debugger memory"),
	debugmem_lgeti, debugmem_wgeti,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL, 0, 0
};

addrbank a3000lmem_bank = {
	a3000lmem_lget, a3000lmem_wget, a3000lmem_bget,
	a3000lmem_lput, a3000lmem_wput, a3000lmem_bput,
	a3000lmem_xlate, a3000lmem_check, NULL, _T("ramsey_low"), _T("RAMSEY memory (low)"),
	a3000lmem_lget, a3000lmem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, 0
};

addrbank a3000hmem_bank = {
	a3000hmem_lget, a3000hmem_wget, a3000hmem_bget,
	a3000hmem_lput, a3000hmem_wput, a3000hmem_bput,
	a3000hmem_xlate, a3000hmem_check, NULL, _T("ramsey_high"), _T("RAMSEY memory (high)"),
	a3000hmem_lget, a3000hmem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, 0
};

addrbank kickmem_bank = {
	kickmem_lget, kickmem_wget, kickmem_bget,
	kickmem_lput, kickmem_wput, kickmem_bput,
	kickmem_xlate, kickmem_check, NULL, _T("kick"), _T("Kickstart ROM"),
	kickmem_lget, kickmem_wget,
	ABFLAG_ROM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, S_WRITE
};

addrbank kickram_bank = {
	kickmem_lget, kickmem_wget, kickmem_bget,
	kickmem2_lput, kickmem2_wput, kickmem2_bput,
	kickmem_xlate, kickmem_check, NULL, NULL, _T("Kickstart Shadow RAM"),
	kickmem_lget, kickmem_wget,
	ABFLAG_UNK | ABFLAG_SAFE | ABFLAG_CACHE_ENABLE_ALL, 0, S_WRITE
};

addrbank extendedkickmem_bank = {
	extendedkickmem_lget, extendedkickmem_wget, extendedkickmem_bget,
	extendedkickmem_lput, extendedkickmem_wput, extendedkickmem_bput,
	extendedkickmem_xlate, extendedkickmem_check, NULL, NULL, _T("Extended Kickstart ROM"),
	extendedkickmem_lget, extendedkickmem_wget,
	ABFLAG_ROM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, S_WRITE
};
addrbank extendedkickmem2a_bank = {
	extendedkickmem2a_lget, extendedkickmem2a_wget, extendedkickmem2a_bget,
	extendedkickmem2a_lput, extendedkickmem2a_wput, extendedkickmem2a_bput,
	extendedkickmem2a_xlate, extendedkickmem2a_check, NULL, _T("rom_a8"), _T("Extended 2nd Kickstart ROM"),
	extendedkickmem2a_lget, extendedkickmem2a_wget,
	ABFLAG_ROM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, S_WRITE
};
addrbank extendedkickmem2b_bank = {
	extendedkickmem2b_lget, extendedkickmem2b_wget, extendedkickmem2b_bget,
	extendedkickmem2b_lput, extendedkickmem2b_wput, extendedkickmem2b_bput,
	extendedkickmem2b_xlate, extendedkickmem2b_check, NULL, _T("rom_b0"), _T("Extended 3nd Kickstart ROM"),
	extendedkickmem2b_lget, extendedkickmem2b_wget,
	ABFLAG_ROM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, S_WRITE
};
addrbank fakeuaebootrom_bank = {
	fakeuaebootrom_lget, fakeuaebootrom_wget, mem25bit_bget,
	fakeuaebootrom_lput, fakeuaebootrom_wput, mem25bit_bput,
	fakeuaebootrom_xlate, fakeuaebootrom_check, NULL, _T("*"), _T("fakeuaerom"),
	fakeuaebootrom_lget, fakeuaebootrom_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

MEMORY_FUNCTIONS(custmem1);
MEMORY_FUNCTIONS(custmem2);

addrbank custmem1_bank = {
	custmem1_lget, custmem1_wget, custmem1_bget,
	custmem1_lput, custmem1_wput, custmem1_bput,
	custmem1_xlate, custmem1_check, NULL, _T("custmem1"), _T("Non-autoconfig RAM #1"),
	custmem1_lget, custmem1_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, 0
};
addrbank custmem2_bank = {
	custmem2_lget, custmem2_wget, custmem2_bget,
	custmem2_lput, custmem2_wput, custmem2_bput,
	custmem2_xlate, custmem2_check, NULL, _T("custmem2"), _T("Non-autoconfig RAM #2"),
	custmem2_lget, custmem2_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CACHE_ENABLE_ALL | ABFLAG_DIRECTACCESS, 0, 0
};

static bool singlebit(uae_u32 v)
{
	while (v && !(v & 1))
		v >>= 1;
	return (v & ~1) == 0;
}

static void allocate_memory_custombanks(void)
{
	if (custmem1_bank.reserved_size != currprefs.custom_memory_sizes[0]) {
		mapped_free(&custmem1_bank);
		custmem1_bank.reserved_size = currprefs.custom_memory_sizes[0];
		// custmem1 and 2 can have non-power of 2 size so only set correct mask if size is power of 2.
		custmem1_bank.mask = singlebit(custmem1_bank.reserved_size) ? custmem1_bank.reserved_size - 1 : -1;
		custmem1_bank.start = currprefs.custom_memory_addrs[0];
		if (custmem1_bank.reserved_size) {
			if (!mapped_malloc(&custmem1_bank))
				custmem1_bank.reserved_size = 0;
		}
	}
	if (custmem2_bank.reserved_size != currprefs.custom_memory_sizes[1]) {
		mapped_free(&custmem2_bank);
		custmem2_bank.reserved_size = currprefs.custom_memory_sizes[1];
		custmem2_bank.mask = singlebit(custmem2_bank.reserved_size) ? custmem2_bank.reserved_size - 1 : -1;
		custmem2_bank.start = currprefs.custom_memory_addrs[1];
		if (custmem2_bank.reserved_size) {
			if (!mapped_malloc(&custmem2_bank))
				custmem2_bank.reserved_size = 0;
		}
	}
}

#define fkickmem_size ROM_SIZE_512
static int a3000_f0;
void a3000_fakekick (int map)
{
	static uae_u8 *kickstore;

	protect_roms (false);
	if (map) {
		uae_u8 *fkickmemory = a3000lmem_bank.baseaddr + a3000lmem_bank.reserved_size - fkickmem_size;
		if (fkickmemory[2] == 0x4e && fkickmemory[3] == 0xf9 && fkickmemory[4] == 0x00) {
			if (!kickstore)
				kickstore = xmalloc (uae_u8, fkickmem_size);
			memcpy (kickstore, kickmem_bank.baseaddr, fkickmem_size);
			if (fkickmemory[5] == 0xfc) {
				memcpy (kickmem_bank.baseaddr, fkickmemory, fkickmem_size / 2);
				memcpy (kickmem_bank.baseaddr + fkickmem_size / 2, fkickmemory, fkickmem_size / 2);
				extendedkickmem_bank.reserved_size = 65536;
				extendedkickmem_bank.label = _T("rom_f0");
				extendedkickmem_bank.mask = extendedkickmem_bank.reserved_size - 1;
				mapped_malloc (&extendedkickmem_bank);
				memcpy (extendedkickmem_bank.baseaddr, fkickmemory + fkickmem_size / 2, 65536);
				map_banks (&extendedkickmem_bank, 0xf0, 1, 1);
				a3000_f0 = 1;
			} else {
				memcpy (kickmem_bank.baseaddr, fkickmemory, fkickmem_size);
			}
		}
	} else {
		if (a3000_f0) {
			map_banks (&dummy_bank, 0xf0, 1, 1);
			mapped_free (&extendedkickmem_bank);
			a3000_f0 = 0;
		}
		if (kickstore)
			memcpy (kickmem_bank.baseaddr, kickstore, fkickmem_size);
		xfree (kickstore);
		kickstore = NULL;
	}
	protect_roms (true);
}

static void descramble_alg(uae_u8 *data, int size)
{
	uae_u8 *tbuf = xmalloc(uae_u8, size);
	memcpy(tbuf, data, size);
	for (int mode = 0; mode < 3; mode++) {
		if ((data[8] == 0x4a && data[9] == 0xfc))
			break;
		for (int s = 0; s < size; s++) {
			int d = s;
			if (mode == 0) {
				if (s & 0x2000)
					d ^= 0x1000;
				if (s & 0x8000)
					d ^= 0x4000;
			} else if (mode == 1) {
				if (s & 0x2000)
					d ^= 0x1000;
			} else {
				if ((~s) & 0x2000)
					d ^= 0x1000;
				if (s & 0x8000)
					d ^= 0x4000;
				d ^= 0x20000;
			}
			data[d] = tbuf[s];
		}
	}
	xfree(tbuf);
}

static const uae_char *kickstring = "exec.library";

static int read_kickstart(struct zfile *f, uae_u8 *mem, int size, int dochecksum, int noalias)
{
	uae_char buffer[11];
	int i, j, oldpos;
	int cr = 0, kickdisk = 0;

	if (size < 0) {
		zfile_fseek(f, 0, SEEK_END);
		size = zfile_ftell32(f) & ~0x3ff;
		zfile_fseek(f, 0, SEEK_SET);
	}
	oldpos = zfile_ftell32(f);
	i = zfile_fread32(buffer, 1, sizeof(buffer), f);
	if (i < sizeof(buffer))
		return 0;
	if (!memcmp(buffer, "KICK", 4)) {
		zfile_fseek(f, 512, SEEK_SET);
		kickdisk = 1;
#if 0
	} else if (size >= ROM_SIZE_512 && !memcmp (buffer, "AMIG", 4)) {
		/* ReKick */
		zfile_fseek (f, oldpos + 0x6c, SEEK_SET);
		cr = 2;
#endif
	} else if (memcmp((uae_char*)buffer, "AMIROMTYPE1", 11) != 0) {
		zfile_fseek(f, oldpos, SEEK_SET);
	} else {
		cloanto_rom = 1;
		cr = 1;
	}

	memset(mem, 0, size);
	if (size >= 131072) {
		for (i = 0; i < 8; i++) {
			mem[size - 16 + i * 2 + 1] = 0x18 + i;
		}
		mem[size - 20] = size >> 24;
		mem[size - 19] = size >> 16;
		mem[size - 18] = size >>  8;
		mem[size - 17] = size >>  0;
	}

	i = zfile_fread32(mem, 1, size, f);

	if (kickdisk && i > ROM_SIZE_256)
		i = ROM_SIZE_256;
#if 0
	if (i >= ROM_SIZE_256 && (i != ROM_SIZE_256 && i != ROM_SIZE_512 && i != ROM_SIZE_512 * 2 && i != ROM_SIZE_512 * 4)) {
		notify_user (NUMSG_KSROMREADERROR);
		return 0;
	}
#endif
	if (i < size - 20)
		kickstart_fix_checksum(mem, size);
	j = 1;
	while (j < i)
		j <<= 1;
	i = j;

	if (!noalias && i == size / 2)
		memcpy(mem + size / 2, mem, size / 2);

	if (cr) {
		if (!decode_rom(mem, size, cr, i))
			return 0;
	}

	if (size <= 256)
		return size;

	if (currprefs.cs_a1000ram && i < ROM_SIZE_256) {
		if (!a1000_bootrom) {
			a1000_kickstart_size = ROM_SIZE_256;
			if (is_board_enabled(&currprefs, ROMTYPE_512KWOM, 0)) {
				a1000_kickstart_size = ROM_SIZE_512;
			}
			a1000_bootrom = xcalloc(uae_u8, ROM_SIZE_256 + 4);
		}
		if (i <= ROM_SIZE_256) {
			int off = 0;
			while (off + i < ROM_SIZE_256) {
				memcpy(a1000_bootrom + off, kickmem_bank.baseaddr, i);
				off += i;
			}
			memset(kickmem_bank.baseaddr, 0, kickmem_bank.allocated_size);
			a1000_handle_kickstart(1);
			dochecksum = 0;
			i = ROM_SIZE_512;
		}
	}

	for (j = 0; j < 256 && i >= ROM_SIZE_256; j++) {
		if (!memcmp(mem + j, kickstring, strlen(kickstring) + 1))
			break;
	}

	if (j == 256 || i < ROM_SIZE_256)
		dochecksum = 0;
	if (dochecksum)
		kickstart_checksum(mem, size);
	return i;
}

static bool load_extendedkickstart (const TCHAR *romextfile, int type)
{
	struct zfile *f;
	int size, off;
	bool ret = false;

	if (romextfile[0] == '\0')
		return false;
#ifdef ARCADIA
	if (is_arcadia_rom (romextfile) == ARCADIA_BIOS) {
		extendedkickmem_type = EXTENDED_ROM_ARCADIA;
		return false;
	}
#endif
	f = read_rom_name (romextfile, false);
	if (!f) {
		notify_user (NUMSG_NOEXTROM);
		return false;
	}
	zfile_fseek (f, 0, SEEK_END);
	size = zfile_ftell32(f);
	extendedkickmem_bank.reserved_size = ROM_SIZE_512;
#ifdef ARCADIA
	struct romdata *rd = get_alg_rom(romextfile);
	if (rd) {
		size = rd->size;
		type = EXTENDED_ROM_ALG;
	}
#endif
	off = 0;
	if (type == 0) {
		if (currprefs.cs_cd32cd) {
			extendedkickmem_type = EXTENDED_ROM_CD32;
		} else if (currprefs.cs_cdtvcd || currprefs.cs_cdtvram) {
			extendedkickmem_type = EXTENDED_ROM_CDTV;
		} else if (size > 300000) {
			uae_u8 data[2] = { 0 };
			zfile_fseek(f, off, SEEK_SET);
			zfile_fread(data, sizeof(data), 1, f);
			if (data[0] == 0x11 && data[1] == 0x11) {
				if (need_uae_boot_rom(&currprefs) != 0xf00000)
					extendedkickmem_type = EXTENDED_ROM_CDTV;
			} else {
				extendedkickmem_type = EXTENDED_ROM_CD32;
			}
		} else if (need_uae_boot_rom (&currprefs) != 0xf00000) {
			extendedkickmem_type = EXTENDED_ROM_CDTV;
		}	
	} else {
		extendedkickmem_type = type;
	}
	if (extendedkickmem_type) {
		zfile_fseek (f, off, SEEK_SET);
		switch (extendedkickmem_type) {
		case EXTENDED_ROM_CDTV:
			extendedkickmem_bank.label = _T("rom_f0");
			mapped_malloc (&extendedkickmem_bank);
			extendedkickmem_bank.start = 0xf00000;
			break;
		case EXTENDED_ROM_CD32:
			extendedkickmem_bank.label = _T("rom_e0");
			mapped_malloc (&extendedkickmem_bank);
			extendedkickmem_bank.start = 0xe00000;
			break;
		case EXTENDED_ROM_ALG:
			extendedkickmem_bank.label = _T("rom_f0");
			mapped_malloc(&extendedkickmem_bank);
			extendedkickmem_bank.start = 0xf00000;
			break;
		}
		if (extendedkickmem_bank.baseaddr) {
			read_kickstart (f, extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated_size, 0, 1);
			if (extendedkickmem_type == EXTENDED_ROM_ALG)
				descramble_alg(extendedkickmem_bank.baseaddr, size);
			extendedkickmem_bank.mask = extendedkickmem_bank.allocated_size - 1;
			ret = true;
		}
	}
	zfile_fclose (f);
	return ret;
}

#ifndef AMIBERRY
extern unsigned char arosrom[];
extern unsigned int arosrom_len;
#endif
extern int seriallog;
static bool load_kickstart_replacement(void)
{
#ifdef AMIBERRY
	int arosrom_len;
	char path[MAX_DPATH];
	get_rom_path(path, MAX_DPATH);
	strcat(path, "aros-ext.bin");
	auto* arosrom = zfile_load_file(path, &arosrom_len);
	if (arosrom == nullptr)
	{
		gui_message("Could not find the 'aros-ext.bin' file in the ROMs directory!");
		return false;
	}
	struct zfile* f = zfile_fopen_data(path, arosrom_len, arosrom);
	if (!f)
	{
		xfree(arosrom);
		return false;
	}
#else
	f = zfile_fopen_data(_T("aros.gz"), arosrom_len, arosrom);
	if (!f)
		return false;
	f = zfile_gunzip(f);
	if (!f)
		return false;
#endif

	extendedkickmem_bank.reserved_size = ROM_SIZE_512;
	extendedkickmem_bank.mask = ROM_SIZE_512 - 1;
	extendedkickmem_bank.label = _T("rom_e0");
	extendedkickmem_type = EXTENDED_ROM_KS;
	mapped_malloc(&extendedkickmem_bank);
	read_kickstart(f, extendedkickmem_bank.baseaddr, ROM_SIZE_512, 0, 1);

#ifdef AMIBERRY
	zfile_fclose(f);
	xfree(arosrom);
	get_rom_path(path, MAX_DPATH);
	strcat(path, "aros-rom.bin");
	arosrom = zfile_load_file(path, &arosrom_len);
	if (arosrom == nullptr)
	{
		gui_message("Could not find the 'aros-rom.bin' file in the ROMs directory!");
		return false;
	}
	f = zfile_fopen_data(path, arosrom_len, arosrom);
	if (!f)
	{
		xfree(arosrom);
		return false;
	}
#endif
	
	kickmem_bank.reserved_size = ROM_SIZE_512;
	kickmem_bank.mask = ROM_SIZE_512 - 1;
	read_kickstart(f, kickmem_bank.baseaddr, ROM_SIZE_512, 1, 0);

	zfile_fclose(f);
	xfree(arosrom);

	seriallog = -1;

	// if 68000-68020 config without any other fast ram with m68k aros: enable special extra RAM.
	if (currprefs.cpu_model <= 68020 &&
		currprefs.cachesize == 0 &&
		currprefs.fastmem[0].size == 0 &&
		currprefs.z3fastmem[0].size == 0 &&
		currprefs.mbresmem_high.size == 0 &&
		currprefs.mbresmem_low.size == 0 &&
		currprefs.cpuboardmem1.size == 0) {

		changed_prefs.custom_memory_addrs[0] = currprefs.custom_memory_addrs[0] = 0xa80000;
		changed_prefs.custom_memory_sizes[0] = currprefs.custom_memory_sizes[0] = 512 * 1024;
		changed_prefs.custom_memory_mask[0] = currprefs.custom_memory_mask[0] = 0;
		changed_prefs.custom_memory_addrs[1] = currprefs.custom_memory_addrs[1] = 0xb00000;
		changed_prefs.custom_memory_sizes[1] = currprefs.custom_memory_sizes[1] = 512 * 1024;
		changed_prefs.custom_memory_mask[1] = currprefs.custom_memory_mask[1] = 0;

		allocate_memory_custombanks();
	}

	return true;
}

static int patch_shapeshifter (uae_u8 *kickmemory)
{
	/* Patch Kickstart ROM for ShapeShifter - from Christian Bauer.
	* Changes 'lea $400,a0' and 'lea $1000,a0' to 'lea $3000,a0' for
	* ShapeShifter compatibility.
	*/
	int i, patched = 0;
	uae_u8 kickshift1[] = { 0x41, 0xf8, 0x04, 0x00 };
	uae_u8 kickshift2[] = { 0x41, 0xf8, 0x10, 0x00 };
	uae_u8 kickshift3[] = { 0x43, 0xf8, 0x04, 0x00 };

	for (i = 0x200; i < 0x300; i++) {
		if (!memcmp (kickmemory + i, kickshift1, sizeof (kickshift1)) ||
			!memcmp (kickmemory + i, kickshift2, sizeof (kickshift2)) ||
			!memcmp (kickmemory + i, kickshift3, sizeof (kickshift3))) {
				kickmemory[i + 2] = 0x30;
				write_log (_T("Kickstart KickShifted @%04X\n"), i);
				patched++;
		}
	}
	return patched;
}

/* disable incompatible drivers */
static int patch_residents (uae_u8 *kickmemory, int size)
{
	int i, j, patched = 0;
	const uae_char *residents[] = { "NCR scsi.device", NULL };
	// "scsi.device", "carddisk.device", "card.resource" };
	uaecptr base = size == ROM_SIZE_512 ? 0xf80000 : 0xfc0000;

	if (is_device_rom(&currprefs, ROMTYPE_SCSI_A4000T, 0) < 0) {
		for (i = 0; i < size - 100; i++) {
			if (kickmemory[i] == 0x4a && kickmemory[i + 1] == 0xfc) {
				uaecptr addr;
				addr = (kickmemory[i + 2] << 24) | (kickmemory[i + 3] << 16) | (kickmemory[i + 4] << 8) | (kickmemory[i + 5] << 0);
				if (addr != i + base)
					continue;
				addr = (kickmemory[i + 14] << 24) | (kickmemory[i + 15] << 16) | (kickmemory[i + 16] << 8) | (kickmemory[i + 17] << 0);
				if (addr >= base && addr < base + size) {
					j = 0;
					while (residents[j]) {
						if (!memcmp (residents[j], kickmemory + addr - base, strlen (residents[j]) + 1)) {
							TCHAR *s = au (residents[j]);
							write_log (_T("KSPatcher: '%s' at %08X disabled\n"), s, i + base);
							xfree (s);
							kickmemory[i] = 0x4b; /* destroy RTC_MATCHWORD */
							patched++;
							break;
						}
						j++;
					}
				}
			}
		}
	}
	return patched;
}

static void patch_kick (void)
{
	int patched = 0;
	if (kickmem_bank.allocated_size >= ROM_SIZE_512 && currprefs.kickshifter)
		patched += patch_shapeshifter (kickmem_bank.baseaddr);
	patched += patch_residents (kickmem_bank.baseaddr, kickmem_bank.allocated_size);
	if (extendedkickmem_bank.baseaddr) {
		patched += patch_residents (extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated_size);
		if (patched)
			kickstart_fix_checksum (extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated_size);
	}
	if (patched)
		kickstart_fix_checksum (kickmem_bank.baseaddr, kickmem_bank.allocated_size);
}

static struct zfile *get_kickstart_filehandle(struct uae_prefs *p)
{
	struct zfile *f;
	TCHAR tmprom[MAX_DPATH], tmprom2[MAX_DPATH];

	f = read_rom_name(p->romfile, false);
	_tcscpy(tmprom, p->romfile);
	_tcscpy(tmprom2, p->romfile);
	if (f == NULL) {
#ifdef AMIBERRY
		// don't check default paths if romfile is empty
		if (p->romfile[0] != 0) {
#endif
			_sntprintf(tmprom2, sizeof tmprom2, _T("%s/%s"), home_dir.c_str(), p->romfile);
			f = rom_fopen(tmprom2, _T("rb"), ZFD_NORMAL);
#ifdef AMIBERRY
		}
#endif
		if (f == NULL) {
			_sntprintf(tmprom2, sizeof tmprom2, _T("%s/roms/kick.rom"), home_dir.c_str());
			f = rom_fopen(tmprom2, _T("rb"), ZFD_NORMAL);
			if (f == NULL) {
				_sntprintf(tmprom2, sizeof tmprom2, _T("%s/kick.rom"), home_dir.c_str());
				f = rom_fopen(tmprom2, _T("rb"), ZFD_NORMAL);
				if (f == NULL) {
					_sntprintf(tmprom2, sizeof tmprom2, _T("%s/../shared/rom/kick.rom"), home_dir.c_str());
					f = rom_fopen(tmprom2, _T("rb"), ZFD_NORMAL);
					if (f == NULL) {
						_sntprintf(tmprom2, sizeof tmprom2, _T("%s/../System/rom/kick.rom"), home_dir.c_str());
						f = rom_fopen(tmprom2, _T("rb"), ZFD_NORMAL);
						if (f == NULL) {
							f = read_rom_name_guess(tmprom, tmprom2);
						}
					}
				}
			}
		}
	}
	if (f) {
		_tcscpy(p->romfile, tmprom2);
	}
	return f;
}

static const uae_u8 romend[20] = {
	0x00, 0x08, 0x00, 0x00,
	0x00, 0x18, 0x00, 0x19, 0x00, 0x1a, 0x00, 0x1b, 0x00, 0x1c, 0x00, 0x1d, 0x00, 0x1e, 0x00, 0x1f
};

static int load_kickstart (void)
{
	TCHAR tmprom[MAX_DPATH];
	cloanto_rom = 0;
	if (!_tcscmp(currprefs.romfile, _T(":AROS"))) {
		return load_kickstart_replacement();
	}
	_tcscpy(tmprom, currprefs.romfile);
	struct zfile *f = get_kickstart_filehandle(&currprefs);
	addkeydir (currprefs.romfile);
	if (f == NULL) /* still no luck */
		goto err;

	if (f != NULL) {
		int filesize, size, maxsize;
		int kspos = ROM_SIZE_512;
		int extpos = 0;
		bool singlebigrom = false;

		uae_u8 tmp[8] = { 0 };
		zfile_fread(tmp, sizeof tmp, 1, f);

		maxsize = ROM_SIZE_512;

		if ((tmp[0] == 0x00 && tmp[1] == 0x00 && tmp[2] == 0x03 && tmp[3] == 0xf3 &&
			tmp[4] == 0x00 && tmp[5] == 0x00 && tmp[6] == 0x00 && tmp[7] == 0x00) ||
			(tmp[0] == 0x7f && tmp[1] == 'E' && tmp[2] == 'L' && tmp[3] == 'F')) {
			struct zfile *zf = read_executable_rom(f, ROM_SIZE_512, 3);
			if (zf) {
				int size = zfile_size32(zf);
				zfile_fclose(f);
				f = zf;
				if (size > ROM_SIZE_512) {
					maxsize = zfile_size32(zf);
					singlebigrom = true;
					extendedkickmem2a_bank.reserved_size = 524288;
					extendedkickmem2a_bank.mask = extendedkickmem2a_bank.allocated_size - 1;
					extendedkickmem2a_bank.start = size > 2 * ROM_SIZE_512 ? 0xa00000 : 0xa80000;
					mapped_malloc(&extendedkickmem2a_bank);
					extendedkickmem2b_bank.reserved_size = 524288;
					extendedkickmem2b_bank.mask = extendedkickmem2a_bank.allocated_size - 1;
					extendedkickmem2b_bank.start = extendedkickmem2a_bank.start + 524288;
					mapped_malloc(&extendedkickmem2b_bank);
					read_kickstart(f, extendedkickmem2a_bank.baseaddr, 524288, 0, 1);
					read_kickstart(f, extendedkickmem2b_bank.baseaddr, 524288, 0, 1);
					memset(kickmem_bank.baseaddr, 0, ROM_SIZE_512);
					memcpy(kickmem_bank.baseaddr, extendedkickmem2a_bank.baseaddr, 0xd0);
					memcpy(kickmem_bank.baseaddr + ROM_SIZE_512 - 20, romend, sizeof(romend));
					kickstart_fix_checksum(kickmem_bank.baseaddr, ROM_SIZE_512);
				}
			}
		}

		if (!singlebigrom) {
			zfile_fseek(f, 0, SEEK_END);
			filesize = zfile_ftell32(f);
			zfile_fseek(f, 0, SEEK_SET);
			if (!singlebigrom) {
				if (filesize == 1760 * 512) {
					filesize = ROM_SIZE_256;
					maxsize = ROM_SIZE_256;
				}
				if (filesize == ROM_SIZE_512 + 8) {
					/* GVP 0xf0 kickstart */
					zfile_fseek(f, 8, SEEK_SET);
				}
				if (filesize >= ROM_SIZE_512 * 2) {
					struct romdata *rd = getromdatabyzfile(f);
					// CD32 with swapper upper and lower 512k?
					if (rd && (rd->type & ROMTYPE_KICKCD32) && rd->size == ROM_SIZE_512) {
						kspos = 0;
						extpos = ROM_SIZE_512;
					}
					zfile_fseek(f, kspos, SEEK_SET);
				}
				if (filesize >= ROM_SIZE_512 * 4) {
					kspos = ROM_SIZE_512 * 3;
					extpos = 0;
					zfile_fseek(f, kspos, SEEK_SET);
				}
			}
			size = read_kickstart(f, kickmem_bank.baseaddr, maxsize, 1, 0);
			if (size == 0)
				goto err;
			kickmem_bank.mask = size - 1;
			kickmem_bank.reserved_size = size;
			if (filesize >= ROM_SIZE_512 * 2 && !extendedkickmem_type) {
				extendedkickmem_bank.reserved_size = ROM_SIZE_512;
				if (currprefs.cs_cdtvcd || currprefs.cs_cdtvram) {
					extendedkickmem_type = EXTENDED_ROM_CDTV;
					extendedkickmem_bank.reserved_size *= 2;
					extendedkickmem_bank.label = _T("rom_f0");
					extendedkickmem_bank.start = 0xf00000;
				} else {
					extendedkickmem_type = EXTENDED_ROM_KS;
					extendedkickmem_bank.label = _T("rom_e0");
					extendedkickmem_bank.start = 0xe00000;
				}
				mapped_malloc(&extendedkickmem_bank);
				zfile_fseek(f, extpos, SEEK_SET);
				read_kickstart(f, extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated_size, 0, 1);
				extendedkickmem_bank.mask = extendedkickmem_bank.allocated_size - 1;
			}
			if (filesize > ROM_SIZE_512 * 2) {
				extendedkickmem2a_bank.reserved_size = ROM_SIZE_512;
				mapped_malloc(&extendedkickmem2a_bank);
				extendedkickmem2b_bank.reserved_size = ROM_SIZE_512;
				mapped_malloc(&extendedkickmem2b_bank);
				zfile_fseek(f, extpos + ROM_SIZE_512, SEEK_SET);
				read_kickstart(f, extendedkickmem2a_bank.baseaddr, ROM_SIZE_512, 0, 1);
				zfile_fseek(f, extpos + ROM_SIZE_512 * 2, SEEK_SET);
				read_kickstart(f, extendedkickmem2b_bank.baseaddr, ROM_SIZE_512, 0, 1);
				extendedkickmem2a_bank.mask = extendedkickmem2a_bank.allocated_size - 1;
				extendedkickmem2a_bank.start = 0xa80000;
				extendedkickmem2b_bank.mask = extendedkickmem2b_bank.allocated_size - 1;
				extendedkickmem2b_bank.start = 0xb00000;
			}
		}
	}

	kickstart_version = (kickmem_bank.baseaddr[12] << 8) | kickmem_bank.baseaddr[13];
	if (kickstart_version == 0xffff) {
		// 1.0-1.1 and older
		kickstart_version = (kickmem_bank.baseaddr[16] << 8) | kickmem_bank.baseaddr[17];
		if (kickstart_version > 33)
			kickstart_version = 0;
	}
	zfile_fclose (f);
	return 1;
err:
	_tcscpy (currprefs.romfile, tmprom);
	zfile_fclose (f);
	return 0;
}

static void set_direct_memory(addrbank *ab)
{
	if (!(ab->flags & ABFLAG_DIRECTACCESS))
		return;
	if (currprefs.cpu_memory_cycle_exact && currprefs.cpu_model < 68020) {
		return;
	}
	ab->baseaddr_direct_r = ab->baseaddr;
	if (!(ab->flags & ABFLAG_ROM))
		ab->baseaddr_direct_w = ab->baseaddr;
}

#ifndef NATMEM_OFFSET

bool mapped_malloc (addrbank *ab)
{
	ab->startmask = ab->start;
	ab->startaccessmask = ab->start & ab->mask;
	ab->baseaddr = xcalloc (uae_u8, ab->reserved_size + 4);
	ab->allocated_size =  ab->baseaddr != NULL ? ab->reserved_size : 0;
	ab->baseaddr_direct_r = NULL;
	ab->baseaddr_direct_w = NULL;
	ab->flags &= ~ABFLAG_MAPPED;
	set_direct_memory(ab);
	return ab->baseaddr != NULL;
}

void mapped_free (addrbank *ab)
{
	xfree(ab->baseaddr);
	ab->flags &= ~ABFLAG_MAPPED;
	ab->allocated_size = 0;
	ab->baseaddr = NULL;
}

#else

#include <uae/mman.h>

shmpiece *shm_start;

static void dumplist (void)
{
	shmpiece *x = shm_start;
	write_log (_T("Start Dump:\n"));
	while (x) {
		write_log (_T("this=%p,Native %p,id %d,prev=%p,next=%p,size=0x%08x\n"),
			x, x->native_address, x->id, x->prev, x->next, x->size);
		x = x->next;
	}
	write_log (_T("End Dump:\n"));
}

static shmpiece *find_shmpiece (uae_u8 *base, bool safe)
{
	shmpiece *x = shm_start;

	while (x && x->native_address != base)
		x = x->next;
	if (!x) {
		if (safe || bogomem_aliasing)
			return 0;
		write_log (_T("NATMEM: Failure to find mapping at %08lx, %p\n"), base - NATMEM_OFFSET, base);
		nocanbang ();
		return 0;
	}
	return x;
}

static void delete_shmmaps (uae_u32 start, uae_u32 size)
{
	if (!needmman ())
		return;

	while (size) {
		uae_u8 *base = mem_banks[bankindex (start)]->baseaddr;
		if (base) {
			shmpiece *x;
			//base = ((uae_u8*)NATMEM_OFFSET)+start;

			x = find_shmpiece (base, true);
			if (!x)
				return;

			if (x->size > size) {
				if (isdirectjit ())
					write_log (_T("NATMEM WARNING: size mismatch mapping at %08x (size %08x, delsize %08x)\n"),start,x->size,size);
				size = x->size;
			}

			uae_shmdt (x->native_address);
			size -= x->size;
			start += x->size;
			if (x->next)
				x->next->prev = x->prev;	/* remove this one from the list */
			if (x->prev)
				x->prev->next = x->next;
			else
				shm_start = x->next;
			xfree (x);
		} else {
			size -= 0x10000;
			start += 0x10000;
		}
	}
}

static void add_shmmaps (uae_u32 start, addrbank *what)
{
	shmpiece *x = shm_start;
	shmpiece *y;
	uae_u8 *base = what->baseaddr;

	if (!needmman ())
		return;

	if (!base)
		return;

	if (what->jit_read_flag && what->jit_write_flag)
		return;

	x = find_shmpiece (base, false);
	if (!x)
		return;

	y = xmalloc (shmpiece, 1);
	*y = *x;
	base = ((uae_u8 *) NATMEM_OFFSET) + start;
	y->native_address = (uae_u8*)uae_shmat (what, y->id, base, 0, NULL);
	if (y->native_address == (void *) -1) {
		write_log (_T("NATMEM: Failure to map existing at %08x (%p)\n"), start, base);
		dumplist ();
		nocanbang ();
		xfree(y);
		return;
	}
	y->next = shm_start;
	y->prev = NULL;
	if (y->next)
		y->next->prev = y;
	shm_start = y;
}

#define MAPPED_MALLOC_DEBUG 1

bool mapped_malloc (addrbank *ab)
{
	int id;
	void *answer;
	bool rtgmem = (ab->flags & ABFLAG_RTG) != 0;
	static int recurse;

	if (ab->allocated_size) {
		write_log(_T("mapped_malloc with memory bank '%s' already allocated!?\n"), ab->name);
	}
	ab->allocated_size = 0;
	ab->baseaddr_direct_r = NULL;
	ab->baseaddr_direct_w = NULL;
	ab->flags &= ~ABFLAG_MAPPED;

	if (ab->label && ab->label[0] == '*') {
		if (ab->start == 0 || ab->start == 0xffffffff) {
			write_log(_T("mapped_malloc(*) without start address!\n"));
			return false;
		}
	}

	struct uae_mman_data md = { 0 };
	uaecptr start = ab->start;
	if (uae_mman_info(ab, &md)) {
		start = md.start;
	}
	ab->startmask = start;
	ab->startaccessmask = start & ab->mask;
	if (!md.directsupport || (ab->flags & ABFLAG_ALLOCINDIRECT)) {
		if (!(ab->flags & ABFLAG_ALLOCINDIRECT)) {
			if (canbang) {
				error_log(_T("JIT direct switched off: %s\n"), ab->name);
			}
			nocanbang();
		}
		ab->flags &= ~ABFLAG_DIRECTMAP;
		if (ab->flags & ABFLAG_NOALLOC) {
			ab->allocated_size = ab->reserved_size;
#if MAPPED_MALLOC_DEBUG
			write_log(_T("mapped_malloc noalloc %s\n"), ab->name);
#endif
			return true;
		}
		ab->baseaddr = xcalloc (uae_u8, ab->reserved_size + 4);
		if (ab->baseaddr) {
			// fill end of ram with ILLEGAL to catch direct PC falling out of RAM.
			put_long_host(ab->baseaddr + ab->reserved_size, 0x4afc4afc);
			ab->allocated_size = ab->reserved_size;
		}
		set_direct_memory(ab);
#if MAPPED_MALLOC_DEBUG
		write_log(_T("mapped_malloc nodirect %s %p\n"), ab->name, ab->baseaddr);
#endif
		return ab->baseaddr != NULL;
	}

	id = uae_shmget (UAE_IPC_PRIVATE, ab, 0x1ff);
	if (id == -1) {
		nocanbang ();
		if (recurse)
			return false;
		recurse++;
		mapped_malloc (ab);
		recurse--;
		return ab->baseaddr != NULL;
	}
	if (!(ab->flags & ABFLAG_NOALLOC)) {
		answer = uae_shmat (ab, id, NULL, 0, &md);
		uae_shmctl (id, UAE_IPC_RMID, NULL);
	} else {
		write_log(_T("MMAN: mapped_malloc using existing baseaddr %p\n"), ab->baseaddr);
		answer = ab->baseaddr;
	}
	if (answer != (void *) -1) {
		shmpiece *xx = shm_start;
		shmpiece *x = NULL;
		// if we already have shmpiece for this address space: reuse it
		while (xx) {
			if (answer == xx->native_address) {
				x = xx;
				break;
			}
			xx = xx->next;
		}
		if (!x) {
			// no old shmpice: allocate new
			x = xcalloc(shmpiece, 1);
			x->next = shm_start;
			x->prev = NULL;
			if (x->next)
				x->next->prev = x;
			shm_start = x;
		}
		x->native_address = (uae_u8*)answer;
		x->id = id;
		x->size = ab->reserved_size;
		x->name = ab->label;
		ab->baseaddr = x->native_address;
		if (ab->baseaddr) {
			if (md.hasbarrier) {
				// fill end of ram with ILLEGAL to catch direct PC falling out of RAM.
				put_long_host(ab->baseaddr + ab->reserved_size, 0x4afc4afc);
				ab->barrier = true;
			}
			ab->allocated_size = ab->reserved_size;
		}
		ab->flags |= ABFLAG_DIRECTMAP;
		set_direct_memory(ab);
#if MAPPED_MALLOC_DEBUG
		write_log(_T("mapped_malloc direct %s %p\n"), ab->name, ab->baseaddr);
#endif
		return ab->baseaddr != NULL;
	}
	if (recurse)
		return false;
	nocanbang ();
	recurse++;
	mapped_malloc (ab);
	recurse--;
	set_direct_memory(ab);
#if MAPPED_MALLOC_DEBUG
	write_log(_T("mapped_malloc indirect %s %p\n"), ab->name, ab->baseaddr);
#endif
	return ab->baseaddr != NULL;
}

#endif

static void init_mem_banks (void)
{
	// unsigned so i << 16 won't overflow to negative when i >= 32768
	for (unsigned int i = 0; i < MEMORY_BANKS; i++)
		put_mem_bank (i << 16, &dummy_bank, 0);
#ifdef NATMEM_OFFSET
	delete_shmmaps (0, 0xFFFF0000);
#endif
}

static void map_banks_set(addrbank *bank, int start, int size, int realsize)
{
	bank->startmask = start << 16;
	map_banks(bank, start, size, realsize);
}

static void allocate_memory (void)
{
	bogomem_aliasing = 0;

	bool bogoreset = (bogomem_bank.flags & ABFLAG_NOALLOC) != 0 &&
		(chipmem_bank.reserved_size != currprefs.chipmem.size || bogomem_bank.reserved_size != currprefs.bogomem.size);

	mapped_free(&fakeuaebootrom_bank);

	if (bogoreset) {
		mapped_free(&chipmem_bank);
		mapped_free(&bogomem_bank);
	}

	/* emulate 0.5M+0.5M with 1M Agnus chip ram aliasing */
	if (currprefs.chipmem.size == 0x80000 && currprefs.bogomem.size >= 0x80000 &&
		(currprefs.chipset_mask & CSMASK_ECS_AGNUS) && !(currprefs.chipset_mask & CSMASK_AGA) && currprefs.cpu_model < 68020) {
			if ((chipmem_bank.reserved_size != currprefs.chipmem.size || bogomem_bank.reserved_size != currprefs.bogomem.size || chipmem_full_size != 0x80000 * 2)) {
				int memsize1, memsize2;
				mapped_free (&chipmem_bank);
				mapped_free (&bogomem_bank);
				bogomem_bank.reserved_size = 0;
				memsize1 = chipmem_bank.reserved_size = currprefs.chipmem.size;
				memsize2 = bogomem_bank.reserved_size = currprefs.bogomem.size;
				chipmem_bank.mask = chipmem_bank.reserved_size - 1;
				chipmem_bank.start = chipmem_start_addr;
				chipmem_full_mask = bogomem_bank.reserved_size * 2 - 1;
				chipmem_full_size = 0x80000 * 2;
				chipmem_bank.reserved_size = memsize1 + memsize2;
				mapped_malloc (&chipmem_bank);
				chipmem_bank.reserved_size = currprefs.chipmem.size;
				chipmem_bank.allocated_size = currprefs.chipmem.size;
				bogomem_bank.baseaddr = chipmem_bank.baseaddr + memsize1;
				bogomem_bank.mask = bogomem_bank.reserved_size - 1;
				bogomem_bank.start = bogomem_start_addr;
				bogomem_bank.flags |= ABFLAG_NOALLOC;
				bogomem_bank.allocated_size = bogomem_bank.reserved_size;
				if (chipmem_bank.baseaddr == 0) {
					write_log (_T("Fatal error: out of memory for chipmem.\n"));
					chipmem_bank.reserved_size = 0;
				} else {
					need_hardreset = true;
				}
			}
			bogomem_aliasing = 1;
	} else if (currprefs.chipmem.size == 0x80000 && currprefs.bogomem.size >= 0x80000 &&
		!(currprefs.chipset_mask & CSMASK_ECS_AGNUS) && currprefs.cs_1mchipjumper && currprefs.cpu_model < 68020) {
			if ((chipmem_bank.reserved_size != currprefs.chipmem.size || bogomem_bank.reserved_size != currprefs.bogomem.size || chipmem_full_size != chipmem_bank.reserved_size)) {
				int memsize1, memsize2;
				mapped_free (&chipmem_bank);
				mapped_free (&bogomem_bank);
				bogomem_bank.reserved_size = 0;
				memsize1 = chipmem_bank.reserved_size = currprefs.chipmem.size;
				memsize2 = bogomem_bank.reserved_size = currprefs.bogomem.size;
				chipmem_bank.mask = chipmem_bank.reserved_size - 1;
				chipmem_bank.start = chipmem_start_addr;
				chipmem_full_mask = chipmem_bank.reserved_size - 1;
				chipmem_full_size = chipmem_bank.reserved_size;
				chipmem_bank.reserved_size = memsize1 + memsize2;
				mapped_malloc (&chipmem_bank);
				chipmem_bank.reserved_size = currprefs.chipmem.size;
				chipmem_bank.allocated_size = currprefs.chipmem.size;
				bogomem_bank.baseaddr = chipmem_bank.baseaddr + memsize1;
				bogomem_bank.mask = bogomem_bank.reserved_size - 1;
				bogomem_bank.start = chipmem_bank.start + currprefs.chipmem.size;
				bogomem_bank.flags |= ABFLAG_NOALLOC;
				if (chipmem_bank.baseaddr == 0) {
					write_log (_T("Fatal error: out of memory for chipmem.\n"));
					chipmem_bank.reserved_size = 0;
				} else {
					need_hardreset = true;
				}
			}
			bogomem_aliasing = 2;
	}

	if (chipmem_bank.reserved_size != currprefs.chipmem.size || bogoreset) {
		int memsize;
		mapped_free (&chipmem_bank);
		chipmem_bank.flags &= ~ABFLAG_NOALLOC;

		memsize = chipmem_bank.reserved_size = chipmem_full_size = currprefs.chipmem.size;
		chipmem_full_mask = chipmem_bank.mask = chipmem_bank.reserved_size - 1;
		chipmem_bank.start = chipmem_start_addr;
		if (!currprefs.cachesize && memsize < 0x100000) {
			memsize = 0x100000;
		}
		if (memsize == 0x180000) {
			memsize = 0x200000;
			chipmem_full_mask = chipmem_bank.mask = memsize - 1;
		}
		chipmem_bank.reserved_size = memsize;
		mapped_malloc (&chipmem_bank);
		chipmem_bank.reserved_size = currprefs.chipmem.size;
		chipmem_bank.allocated_size = currprefs.chipmem.size;
		if (chipmem_bank.baseaddr == 0) {
			write_log (_T("Fatal error: out of memory for chipmem.\n"));
			chipmem_bank.reserved_size = 0;
		} else {
			need_hardreset = true;
			if (memsize > chipmem_bank.allocated_size)
				memset (chipmem_bank.baseaddr + chipmem_bank.allocated_size, 0xff, memsize - chipmem_bank.allocated_size);
		}
		currprefs.chipset_mask = changed_prefs.chipset_mask;
		if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
			if (chipmem_bank.allocated_size < 0x100000)
				chipmem_full_mask = 0x100000 - 1;
		} else {
			chipmem_full_mask = 0x80000 - 1;
		}
	}

	if (currprefs.cs_agnussize > AGNUSSIZE_AUTO) {
		if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
			// if ECS: allow smaller than select chip RAM size DMA range
			if (currprefs.cs_agnussize <= AGNUSSIZE_512) {
				chipmem_full_mask = 0x80000 - 1;
			} else if (currprefs.cs_agnussize == AGNUSSIZE_1M && chipmem_full_mask > 0x100000) {
				chipmem_full_mask = 0x100000 - 1;
			}
		} else {
			// if OCS: allow larger than 512k DMA range
			if (currprefs.cs_agnussize == AGNUSSIZE_1M && chipmem_bank.allocated_size >= 0x100000) {
				chipmem_full_mask = 0x100000 - 1;
			}
			if (currprefs.cs_agnussize == AGNUSSIZE_2M && chipmem_bank.allocated_size >= 0x180000) {
				chipmem_full_mask = 0x200000 - 1;
			}
		}
	}

	if (currprefs.cachesize) {
		chipmem_full_mask = chipmem_bank.mask = chipmem_bank.allocated_size - 1;
	}

	if (bogomem_bank.reserved_size != currprefs.bogomem.size || bogoreset) {
		if (!(bogomem_bank.reserved_size == 0x200000 && currprefs.bogomem.size == 0x180000)) {
			mapped_free (&bogomem_bank);
			bogomem_bank.flags &= ~ABFLAG_NOALLOC;
			bogomem_bank.reserved_size = 0;

			bogomem_bank.reserved_size = currprefs.bogomem.size;
			if (bogomem_bank.reserved_size >= 0x180000)
				bogomem_bank.reserved_size = 0x200000;
			bogomem_bank.mask = bogomem_bank.reserved_size - 1;
			bogomem_bank.start = bogomem_start_addr;

			if (bogomem_bank.reserved_size) {
				if (!mapped_malloc (&bogomem_bank)) {
					write_log (_T("Out of memory for bogomem.\n"));
					bogomem_bank.reserved_size = 0;
				}
			}
			need_hardreset = true;
		}
	}
	if (debugmem_bank.reserved_size != currprefs.debugmem_size) {
		mapped_free(&debugmem_bank);

		debugmem_bank.reserved_size = currprefs.debugmem_size;
		debugmem_bank.mask = debugmem_bank.reserved_size - 1;
		debugmem_bank.start = currprefs.debugmem_start;
		if (debugmem_bank.reserved_size) {
			if (!mapped_malloc(&debugmem_bank)) {
				write_log(_T("Out of memory for debugger memory.\n"));
				debugmem_bank.reserved_size = 0;
			}
		}
	}
	if (mem25bit_bank.reserved_size != currprefs.mem25bit.size) {
		mapped_free(&mem25bit_bank);

		mem25bit_bank.reserved_size = currprefs.mem25bit.size;
		mem25bit_bank.mask = mem25bit_bank.reserved_size - 1;
		mem25bit_bank.start = 0x01000000;
		if (mem25bit_bank.reserved_size) {
			if (!mapped_malloc(&mem25bit_bank)) {
				write_log(_T("Out of memory for 25 bit memory.\n"));
				mem25bit_bank.reserved_size = 0;
			}
		}
		need_hardreset = true;
	}
	if (a3000lmem_bank.reserved_size != currprefs.mbresmem_low.size) {
		mapped_free (&a3000lmem_bank);

		a3000lmem_bank.reserved_size = currprefs.mbresmem_low.size;
		a3000lmem_bank.mask = a3000lmem_bank.reserved_size - 1;
		a3000lmem_bank.start = 0x08000000 - a3000lmem_bank.reserved_size;
		if (a3000lmem_bank.reserved_size) {
			if (!mapped_malloc (&a3000lmem_bank)) {
				write_log (_T("Out of memory for a3000lowmem.\n"));
				a3000lmem_bank.reserved_size = 0;
			}
		}
		need_hardreset = true;
	}
	if (a3000hmem_bank.reserved_size != currprefs.mbresmem_high.size) {
		mapped_free (&a3000hmem_bank);

		a3000hmem_bank.reserved_size = currprefs.mbresmem_high.size;
		a3000hmem_bank.mask = a3000hmem_bank.reserved_size - 1;
		if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
			a3000hmem_bank.start = 0x40000000;
		} else {
			a3000hmem_bank.start = 0x08000000;
		}
		if (a3000hmem_bank.reserved_size) {
			if (!mapped_malloc (&a3000hmem_bank)) {
				write_log (_T("Out of memory for a3000highmem.\n"));
				a3000hmem_bank.reserved_size = 0;
			}
		}
		need_hardreset = true;
	}

	allocate_memory_custombanks();

	if (savestate_state == STATE_RESTORE) {
		if (bootrom_filepos) {
			if (currprefs.uaeboard < 0)
				currprefs.uaeboard = 0;
			protect_roms (false);
			restore_ram (bootrom_filepos, rtarea_bank.baseaddr);
			protect_roms (true);
			if (currprefs.uaeboard >= 2) {
				map_banks_set(&rtarea_bank, rtarea_base >> 16, 1, 0);
			}
		}
		restore_ram (chip_filepos, chipmem_bank.baseaddr);
		if (bogomem_bank.allocated_size > 0)
			restore_ram (bogo_filepos, bogomem_bank.baseaddr);
		if (mem25bit_bank.allocated_size > 0)
			restore_ram(mem25bit_filepos, mem25bit_bank.baseaddr);
		if (a3000lmem_bank.allocated_size > 0)
			restore_ram (a3000lmem_filepos, a3000lmem_bank.baseaddr);
		if (a3000hmem_bank.allocated_size > 0)
			restore_ram (a3000hmem_filepos, a3000hmem_bank.baseaddr);
	} else {
#ifdef ARCADIA
		alg_flag = 0;
#endif
	}
#ifdef AGA
	chipmem_bank_ce2.baseaddr = chipmem_bank.baseaddr;
#endif
	bootrom_filepos = 0;
	chip_filepos = 0;
	bogo_filepos = 0;
	a3000lmem_filepos = 0;
	a3000hmem_filepos = 0;
	cpuboard_init();
}

static void setmemorywidth(struct ramboard *mb, addrbank *ab)
{
	if (!ab || !ab->allocated_size)
		return;
	if (mb->force16bit) {
		for (int i = (ab->start >> 16); i < ((ab->start + ab->allocated_size) >> 16); i++) {
			if (ce_banktype[i] == CE_MEMBANK_FAST32)
				ce_banktype[i] = CE_MEMBANK_FAST16;
			if (ce_banktype[i] == CE_MEMBANK_CHIP32)
				ce_banktype[i] = CE_MEMBANK_CHIP16;
		}
	}
	if (mb->chipramtiming) {
		for (int i = (ab->start >> 16); i < ((ab->start + ab->allocated_size) >> 16); i++) {
			ce_banktype[i] = ce_banktype[0];
		}
	}
	if (mb->fault) {
		ab->baseaddr_direct_w = NULL;
		ab->baseaddr_direct_r = NULL;
		ab->lput = &dummy_lput;
		ab->wput = &dummy_wput;
		ab->bput = &dummy_bput;
	}
}

static void fill_ce_banks (void)
{
	int i;

	if (currprefs.cpu_model <= 68010) {
		memset (ce_banktype, CE_MEMBANK_FAST16, sizeof ce_banktype);
	} else {
		memset (ce_banktype, CE_MEMBANK_FAST32, sizeof ce_banktype);
	}

	addrbank *ab = &get_mem_bank(0);
	if (ab && (ab->flags & ABFLAG_CHIPRAM)) {
		for (i = 0; i < (0x200000 >> 16); i++) {
			ce_banktype[i] = (currprefs.cs_mbdmac || (currprefs.chipset_mask & CSMASK_AGA)) ? CE_MEMBANK_CHIP32 : CE_MEMBANK_CHIP16;
		}
	}
	for (i = (0xd00000 >> 16); i < (0xe00000 >> 16); i++) {
		ce_banktype[i] = CE_MEMBANK_CHIP16;
	}
	for (i = (0xa00000 >> 16); i < (0xc00000 >> 16); i++) {
		addrbank *b;
		ce_banktype[i] = CE_MEMBANK_CIA;
		b = &get_mem_bank (i << 16);
		if (b && !(b->flags & ABFLAG_CIA)) {
			ce_banktype[i] = CE_MEMBANK_FAST32;
		}
	}
	// CD32 ROM is 16-bit
	if (currprefs.cs_cd32cd) {
		for (i = (0xe00000 >> 16); i < (0xe80000 >> 16); i++)
			ce_banktype[i] = CE_MEMBANK_FAST16;
		for (i = (0xf80000 >> 16); i <= (0xff0000 >> 16); i++)
			ce_banktype[i] = CE_MEMBANK_FAST16;
	}

	// A4000T NCR is 32-bit
	if (is_device_rom(&currprefs, ROMTYPE_SCSI_A4000T, 0) >= 0) {
		ce_banktype[0xdd0000 >> 16] = CE_MEMBANK_FAST32;
	}

	if (currprefs.cs_toshibagary) {
		for (i = (0xe80000 >> 16); i < (0xf80000 >> 16); i++)
			ce_banktype[i] = CE_MEMBANK_CHIP16;
	}

	if (currprefs.cs_romisslow) {
		if (aga_mode && !currprefs.cs_cd32cd) {
			for (i = (0xe00000 >> 16); i < (0xe80000 >> 16); i++)
				ce_banktype[i] = CE_MEMBANK_CHIP32;
			for (i = (0xf80000 >> 16); i < (0x1000000 >> 16); i++)
				ce_banktype[i] = CE_MEMBANK_CHIP32;
		} else {
			for (i = (0xe00000 >> 16); i < (0xe80000 >> 16); i++)
				ce_banktype[i] = CE_MEMBANK_CHIP16;
			for (i = (0xf80000 >> 16); i < (0x1000000 >> 16); i++)
				ce_banktype[i] = CE_MEMBANK_CHIP16;
		}
	}

	setmemorywidth(&currprefs.chipmem, &chipmem_bank);
	setmemorywidth(&currprefs.bogomem, &bogomem_bank);
	setmemorywidth(&currprefs.z3chipmem, &z3chipmem_bank);
	setmemorywidth(&currprefs.mbresmem_low, &a3000lmem_bank);
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		setmemorywidth(&currprefs.z3fastmem[i], &z3fastmem_bank[i]);
		setmemorywidth(&currprefs.fastmem[i], &fastmem_bank[i]);
	}

	if (currprefs.address_space_24) {
		for (i = 1; i < MEMORY_BANKS_24; i++)
			memcpy(&ce_banktype[i * 256], &ce_banktype[0], 256);
	}

}

static int overlay_state;

void map_overlay (int chip)
{
	int size;
	addrbank *cb;

	if (chip < 0)
		chip = overlay_state;
#ifdef WITH_DRACO
	if (currprefs.cs_compatible == CP_CASABLANCA) {
		casablanca_map_overlay();
		return;
	} else if (currprefs.cs_compatible == CP_DRACO) {
		draco_map_overlay();
		return;
	}
#endif
	size = chipmem_bank.allocated_size >= 0x180000 ? (chipmem_bank.allocated_size >> 16) : 32;
	if (bogomem_aliasing)
		size = 8;
	cb = &chipmem_bank;
#ifdef AGA
#if 0
	if (currprefs.cpu_cycle_exact && currprefs.cpu_model >= 68020)
		cb = &chipmem_bank_ce2;
#endif
#endif
	if (chip) {
		map_banks(&dummy_bank, 0, size, 0);
		if (!isdirectjit()) {
			if ((currprefs.chipset_mask & CSMASK_ECS_AGNUS) && bogomem_bank.allocated_size == 0) {
				map_banks(cb, 0, size, chipmem_bank.allocated_size);
				int start = chipmem_bank.allocated_size >> 16;
				if (chipmem_bank.allocated_size < 0x100000) {
					if (currprefs.cs_1mchipjumper) {
						int dummy = (0x100000 - chipmem_bank.allocated_size) >> 16;
						map_banks(&chipmem_dummy_bank, start, dummy, 0);
						map_banks(&chipmem_dummy_bank, start + 16, dummy, 0);
					}
				} else if (chipmem_bank.allocated_size < 0x200000 && chipmem_bank.allocated_size > 0x100000) {
					int dummy = (0x200000 - chipmem_bank.allocated_size) >> 16;
					map_banks(&chipmem_dummy_bank, start, dummy, 0);
				}
			} else {
				int mapsize = 32;
				if ((chipmem_bank.allocated_size >> 16) > mapsize)
					mapsize = chipmem_bank.allocated_size >> 16;
				map_banks(cb, 0, mapsize, chipmem_bank.allocated_size);
			}
		} else {
			map_banks(cb, 0, chipmem_bank.allocated_size >> 16, 0);
		}
	} else {
		addrbank *rb = NULL;
		if (size < 32 && bogomem_aliasing == 0)
			size = 32;
		cb = get_mem_bank_real(0xf00000);
		if (!rb && cb && (cb->flags & ABFLAG_ROM) && get_word(0xf00000) == 0x1114)
			rb = cb;
		cb = get_mem_bank_real(0xe00000);
		if (!rb && cb && (cb->flags & ABFLAG_ROM) && get_word(0xe00000) == 0x1114)
			rb = cb;
		if (!rb)
			rb = &kickmem_bank;
		map_banks(rb, 0, size, 0x80000);
	}
	initramboard(&chipmem_bank, &currprefs.chipmem);
	overlay_state = chip;
	fill_ce_banks();
	cpuboard_overlay_override();
	if (!isrestore() && valid_address(regs.pc, 4)) {
		m68k_setpc_normal(m68k_getpc());
	}
}

// Set a default pattern for uninitialized memory after hard reset.
//   0:even 1:odd on columns for even rows,
//   1:even 0:odd on columns for odd rows.
static void fillpattern(addrbank *ab)
{
	if (currprefs.cs_memorypatternfill && aga_mode) {
		uae_u32 fillval = 0;
		for (int fillbank = 0; fillbank < ab->allocated_size / 2048; fillbank++) {
			fillval = ~fillval;
			for (int fillrow = fillbank * 2048; fillrow < (fillbank + 1) * 2048; fillrow += 4) {
				// Chip emulated: NEC PD42S4260 (A1200 R1).  Spec says 512x512x16.
				*((uae_u32 *)(ab->baseaddr + fillrow)) = fillval;
				if ((fillrow & 7) == 4) {
					fillval = ~fillval;
				}
			}
		}
	} else if (currprefs.cs_memorypatternfill && (currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
		uae_u32 fillval = 0;
		for (int fillbank = 0; fillbank < ab->allocated_size / 1024; fillbank++) {
			fillval = ~fillval;
			for (int fillrow = fillbank * 1024; fillrow < (fillbank + 1) * 1024; fillrow += 4) {
				// Chip emulated: Generic 4256.  4 * 512x4.
				*((uae_u32 *)(ab->baseaddr + fillrow)) = fillval;
				if ((fillrow & 7) == 0) {
					fillval = ~fillval;
				}
			}
		}
	} else if (currprefs.cs_memorypatternfill && !agnusa1000) {
		// OCS Agnus has swapped row and column compared to ECS and AGA.
		uae_u16 fillval = 0;
		for (int fillbank = 0; fillbank < ab->allocated_size / 256; fillbank++) {
			fillval = ~fillval;
			for (int fillrow = 0; fillrow < 256; fillrow += 2) {
				// Chip emulated: Generic 4256.  16 * 512x1.
				*((uae_u16 *)(ab->baseaddr + fillbank * 256 + fillrow)) = fillval;
			}
		}
	} else if (currprefs.cs_memorypatternfill) {
		// A1000
		uae_u16 fillval = 0;
		for (int fillbank = 0; fillbank < ab->allocated_size / 512; fillbank++) {
			fillval = ~fillval;
			for (int fillrow = 0; fillrow < 512; fillrow += 2) {
				// Chip emulated: Generic 4256.  16 * 512x1.
				*((uae_u16 *)(ab->baseaddr + fillbank * 512 + fillrow)) = fillval;
				if (((fillrow >> 1) & 15) == 15) {
					fillval = ~fillval;
				}
			}
		}
	} else {
		memset(ab->baseaddr, 0, ab->allocated_size);
	}
}

void memory_clear (void)
{
	mem_hardreset = 0;
	if (savestate_state == STATE_RESTORE)
		return;
	
	if (chipmem_bank.baseaddr) {
		fillpattern(&chipmem_bank);
	}

	if (bogomem_bank.baseaddr) {
		// TODO: slow RAM can have 16x chips even if Agnus is ECS.
		fillpattern(&bogomem_bank);
	}
	
	if (mem25bit_bank.baseaddr)
		memset(mem25bit_bank.baseaddr, 0, mem25bit_bank.allocated_size);
	if (a3000lmem_bank.baseaddr)
		memset(a3000lmem_bank.baseaddr, 0, a3000lmem_bank.allocated_size);
	if (a3000hmem_bank.baseaddr)
		memset(a3000hmem_bank.baseaddr, 0, a3000hmem_bank.allocated_size);
	expansion_clear ();
	cpuboard_clear();
}

static void restore_roms(void)
{
	roms_modified = false;
	protect_roms (false);
	write_log (_T("ROM loader.. (%s)\n"), currprefs.romfile);
	kickstart_rom = 1;
	a1000_handle_kickstart (0);
	xfree (a1000_bootrom);
	a1000_bootrom = 0;
	a1000_kickstart_mode = 0;

	memcpy(currprefs.romfile, changed_prefs.romfile, sizeof currprefs.romfile);
	memcpy(currprefs.romextfile, changed_prefs.romextfile, sizeof currprefs.romextfile);
	need_hardreset = true;
	mapped_free(&extendedkickmem_bank);
	mapped_free(&extendedkickmem2a_bank);
	mapped_free(&extendedkickmem2b_bank);
	extendedkickmem_bank.reserved_size = 0;
	extendedkickmem2a_bank.reserved_size = 0;
	extendedkickmem2b_bank.reserved_size = 0;
	extendedkickmem_type = 0;
	load_extendedkickstart (currprefs.romextfile, 0);
	load_extendedkickstart (currprefs.romextfile2, EXTENDED_ROM_CDTV);
	kickmem_bank.mask = ROM_SIZE_512 - 1;
	if (!load_kickstart ()) {
		if (currprefs.romfile[0] != '\0') {
			error_log (_T("Failed to open '%s'\n"), currprefs.romfile);
			notify_user (NUMSG_NOROM);
		}
		load_kickstart_replacement ();
	} else {
		struct romdata *rd = getromdatabydata (kickmem_bank.baseaddr, kickmem_bank.reserved_size);
		if (rd) {
			write_log (_T("Known ROM '%s' loaded\n"), rd->name);
#if 1
			if ((rd->cpu & 8) && changed_prefs.cpu_model < 68030) {
				notify_user(NUMSG_KS68030PLUS);
				uae_restart(&currprefs, -1, NULL);
			} else if ((rd->cpu & 3) == 3 && changed_prefs.cpu_model != 68030) {
				notify_user(NUMSG_KS68030);
				uae_restart(&currprefs, -1, NULL);
			} else if ((rd->cpu & 3) == 1 && changed_prefs.cpu_model < 68020) {
				notify_user(NUMSG_KS68EC020);
				uae_restart(&currprefs, -1, NULL);
			} else if ((rd->cpu & 3) == 2 && (changed_prefs.cpu_model < 68020 || changed_prefs.address_space_24)) {
				notify_user(NUMSG_KS68020);
				uae_restart(&currprefs, -1, NULL);
			}
#endif
			if (rd->cloanto)
				cloanto_rom = 1;
			kickstart_rom = 0;
			if ((rd->type & (ROMTYPE_SPECIALKICK | ROMTYPE_KICK)) == ROMTYPE_KICK)
				kickstart_rom = 1;
			if ((rd->cpu & 4) && currprefs.cs_compatible) {
				/* A4000 ROM = need ramsey, gary and ide */
				if (currprefs.cs_ramseyrev < 0)
					changed_prefs.cs_ramseyrev = currprefs.cs_ramseyrev = 0x0f;
				changed_prefs.cs_fatgaryrev = currprefs.cs_fatgaryrev = 0;
				if (currprefs.cs_ide != IDE_A4000)
					changed_prefs.cs_ide = currprefs.cs_ide = -1;
			}
		} else {
			write_log (_T("Unknown ROM '%s' loaded\n"), currprefs.romfile);
		}
	}
	patch_kick ();
	write_log (_T("ROM loader end\n"));
	protect_roms (true);
}

bool read_kickstart_version(struct uae_prefs *p)
{
	kickstart_version = 0;
	cloanto_rom = 0;
	struct zfile *z = get_kickstart_filehandle(p);
	if (!z)
		return false;
	uae_u8 mem[32] = { 0 };
	read_kickstart(z, mem, sizeof mem, 0, 0);
	zfile_fclose(z);
	kickstart_version = (mem[12] << 8) | mem[13];
	if (kickstart_version == 0xffff) {
		// 1.0-1.1 and older
		kickstart_version = (mem[16] << 8) | mem[17];
		if (kickstart_version > 33)
			kickstart_version = 0;
	}
	write_log(_T("KS ver = %d (0x%02x)\n"), kickstart_version & 255, kickstart_version);
	return true;
}

void reload_roms(void)
{
	if (roms_modified)
		restore_roms();
}

void memory_restore(void)
{
	last_address_space_24 = currprefs.address_space_24;
	cpuboard_map();
	map_banks_set(&kickmem_bank, 0xF8, 8, 0);
}

static void kickmem_init(void)
{
	if (!kickmem_bank.baseaddr) {
		kickmem_bank.reserved_size = ROM_SIZE_512;
		mapped_malloc(&kickmem_bank);
		if (kickmem_bank.baseaddr) {
			memset(kickmem_bank.baseaddr, 0, ROM_SIZE_512);
		}
	}
}

void memory_reset (void)
{
	int bnk, bnk_end;
	bool gayleorfatgary;

	highest_ram = 0;
	need_hardreset = false;
	rom_write_enabled = true;
#ifdef JIT
	jit_n_addr_unsafe = 0;
#endif
	/* Use changed_prefs, as m68k_reset is called later.  */
	if (last_address_space_24 != changed_prefs.address_space_24)
		need_hardreset = true;
	last_address_space_24 = changed_prefs.address_space_24;

	if (mem_hardreset > 2) {
		memory_init();
	}
	kickmem_init();

	memset(ce_cachable, CACHE_ENABLE_INS, sizeof ce_cachable);

	be_cnt = be_recursive = 0;
	currprefs.chipmem.size = changed_prefs.chipmem.size;
	currprefs.bogomem.size = changed_prefs.bogomem.size;
	currprefs.mbresmem_low.size = changed_prefs.mbresmem_low.size;
	currprefs.mbresmem_high.size = changed_prefs.mbresmem_high.size;
	currprefs.cs_ksmirror_e0 = changed_prefs.cs_ksmirror_e0;
	currprefs.cs_ksmirror_a8 = changed_prefs.cs_ksmirror_a8;
	currprefs.cs_ciaoverlay = changed_prefs.cs_ciaoverlay;
	currprefs.cs_cdtvram = changed_prefs.cs_cdtvram;
	currprefs.cs_a1000ram = changed_prefs.cs_a1000ram;
	currprefs.cs_ide = changed_prefs.cs_ide;
	currprefs.cs_fatgaryrev = changed_prefs.cs_fatgaryrev;
	currprefs.cs_ramseyrev = changed_prefs.cs_ramseyrev;
	currprefs.cs_unmapped_space = changed_prefs.cs_unmapped_space;
	cpuboard_reset(mem_hardreset);

	gayleorfatgary = ((currprefs.chipset_mask & CSMASK_AGA) || currprefs.cs_pcmcia || currprefs.cs_ide > 0 || currprefs.cs_mbdmac) && !currprefs.cs_cd32cd;

	init_mem_banks ();
	allocate_memory ();
	chipmem_setindirect ();

	if (mem_hardreset > 1 || ((roms_modified || a1000_bootrom) && is_hardreset())
		|| _tcscmp (currprefs.romfile, changed_prefs.romfile) != 0
		|| _tcscmp (currprefs.romextfile, changed_prefs.romextfile) != 0)
	{
		restore_roms();
	}

	if ((cloanto_rom || extendedkickmem_bank.allocated_size) && currprefs.maprom && currprefs.maprom < 0x01000000) {
		currprefs.maprom = changed_prefs.maprom = 0x00a80000;
		if (extendedkickmem2a_bank.allocated_size || extendedkickmem2b_bank.allocated_size) // can't do if 2M ROM
			currprefs.maprom = changed_prefs.maprom = 0;
	}

	map_banks (&custom_bank, 0xC0, 0xE0 - 0xC0, 0);
	map_banks (&cia_bank, 0xA0, 32, 0);
	if (!currprefs.cs_a1000ram && currprefs.cs_rtc != 3)
		/* D80000 - DDFFFF not mapped (A1000 or A2000 = custom chips) */
		map_banks (&dummy_bank, 0xD8, 6, 0);

	/* map "nothing" to 0x200000 - 0x9FFFFF (0xBEFFFF if Gayle or Fat Gary) */
	bnk = chipmem_bank.allocated_size >> 16;
	if (bnk < 0x20 + (currprefs.fastmem[0].size >> 16))
		bnk = 0x20 + (currprefs.fastmem[0].size >> 16);
	bnk_end = currprefs.cs_cd32cd ? 0xBE : (gayleorfatgary ? 0xBF : 0xA0);
	map_banks (&dummy_bank, bnk, bnk_end - bnk, 0);
	if (gayleorfatgary) {
		 // a3000 or a4000 = custom chips from 0xc0 to 0xd0
		if (currprefs.cs_ide == IDE_A4000 || currprefs.cs_mbdmac)
			map_banks (&dummy_bank, 0xd0, 8, 0);
		else
			map_banks (&dummy_bank, 0xc0, 0xd8 - 0xc0, 0);
	} else if (currprefs.cs_cd32cd) {
		// CD32: 0xc0 to 0xd0
		map_banks(&dummy_bank, 0xd0, 8, 0);
		// strange 64k custom mirror
		map_banks(&custom_bank, 0xb9, 1, 0);
	}

	if (bogomem_bank.baseaddr) {
		int t = currprefs.bogomem.size >> 16;
		if (t > 0x1C)
			t = 0x1C;
		if (t > 0x18 && ((currprefs.chipset_mask & CSMASK_AGA) || (currprefs.cpu_model >= 68020 && !currprefs.address_space_24)))
			t = 0x18;
		if (bogomem_aliasing == 2)
			map_banks (&bogomem_bank, 0x08, t, 0);
		else
			map_banks (&bogomem_bank, 0xC0, t, 0);
		initramboard(&bogomem_bank, &currprefs.bogomem);
	}
	if (currprefs.cs_ide || currprefs.cs_pcmcia) {
		if (currprefs.cs_ide == IDE_A600A1200 || currprefs.cs_pcmcia) {
			map_banks (&gayle_bank, 0xD8, 6, 0);
			map_banks (&gayle2_bank, 0xDD, 2, 0);
		}
		if (currprefs.cs_ide == IDE_A4000 || is_device_rom(&currprefs, ROMTYPE_SCSI_A4000T, 0))
			map_banks (&gayle_bank, 0xDD, 1, 0);
		if (currprefs.cs_ide < 0 && !currprefs.cs_pcmcia)
			map_banks (&gayle_bank, 0xD8, 6, 0);
		if (currprefs.cs_ide < 0)
			map_banks (&gayle_bank, 0xDD, 1, 0);
	}
	if (currprefs.cs_rtc == 3) { // A2000 clock
		map_banks (&clock_bank, 0xD8, 4, 0);
		clock_bank.startmask = 0xd80000;
	}
	if (currprefs.cs_rtc == 1 || currprefs.cs_rtc == 2 || currprefs.cs_cdtvram) {
		map_banks (&clock_bank, 0xDC, 1, 0);
		clock_bank.startmask = 0xdc0000;
	}
	else if (currprefs.cs_ksmirror_a8 || currprefs.cs_ide > 0 || currprefs.cs_pcmcia) {
		map_banks (&clock_bank, 0xDC, 1, 0); /* none clock */
		clock_bank.startmask = 0xdc0000;
	}
	if (currprefs.cs_fatgaryrev >= 0 || currprefs.cs_ramseyrev >= 0)
		map_banks (&mbres_bank, 0xDE, 1, 0);
#ifdef CD32
	if (currprefs.cs_cd32c2p || currprefs.cs_cd32cd || currprefs.cs_cd32nvram) {
		map_banks (&akiko_bank, AKIKO_BASE >> 16, 1, 0);
		map_banks (&gayle2_bank, 0xDD, 2, 0);
	}
#endif
	if (mem25bit_bank.baseaddr) {
		map_banks(&mem25bit_bank, mem25bit_bank.start >> 16, mem25bit_bank.allocated_size >> 16, 0);
		initramboard(&mem25bit_bank, &currprefs.mem25bit);
	}
	if (a3000lmem_bank.baseaddr) {
		map_banks(&a3000lmem_bank, a3000lmem_bank.start >> 16, a3000lmem_bank.allocated_size >> 16, 0);
		initramboard(&a3000lmem_bank, &currprefs.mbresmem_low);
	}
	if (a3000hmem_bank.baseaddr) {
		map_banks(&a3000hmem_bank, a3000hmem_bank.start >> 16, a3000hmem_bank.allocated_size >> 16, 0);
		initramboard(&a3000hmem_bank, &currprefs.mbresmem_high);
	}
	if (debugmem_bank.baseaddr) {
		map_banks(&debugmem_bank, debugmem_bank.start >> 16, debugmem_bank.allocated_size >> 16, 0);
	}
	cpuboard_map();
	map_banks_set(&kickmem_bank, 0xF8, 8, 0);
	if (currprefs.maprom && _tcscmp(currprefs.romfile, _T(":AROS"))) {
		if (!cpuboard_maprom())
			map_banks_set(&kickram_bank, currprefs.maprom >> 16, extendedkickmem2a_bank.allocated_size ? 32 : (extendedkickmem_bank.allocated_size ? 16 : 8), 0);
	}
	/* map beta Kickstarts at 0x200000/0xC00000/0xF00000 */
	if (kickmem_bank.baseaddr[0] == 0x11 && kickmem_bank.baseaddr[2] == 0x4e && kickmem_bank.baseaddr[3] == 0xf9 && kickmem_bank.baseaddr[4] == 0x00) {
		uae_u32 addr = kickmem_bank.baseaddr[5];
		if (addr == 0x20 && currprefs.chipmem.size <= 0x200000 && currprefs.fastmem[0].size == 0)
			map_banks_set(&kickmem_bank, addr, 8, 0);
		if (addr == 0xC0 && currprefs.bogomem.size == 0)
			map_banks_set(&kickmem_bank, addr, 8, 0);
		if (addr == 0xF0)
			map_banks_set(&kickmem_bank, addr, 8, 0);
	}

	if (a1000_bootrom)
		a1000_handle_kickstart (1);

#ifdef AUTOCONFIG
	expansion_map();
#endif

	if (a3000_f0)
		map_banks_set(&extendedkickmem_bank, 0xf0, 1, 0);

	/* Map the chipmem into all of the lower 8MB */
	map_overlay (1);

	switch (extendedkickmem_type) {
	case EXTENDED_ROM_KS:
		map_banks_set(&extendedkickmem_bank, 0xE0, 8, 0);
		break;
#ifdef CDTV
	case EXTENDED_ROM_CDTV:
		map_banks_set(&extendedkickmem_bank, 0xF0, extendedkickmem_bank.allocated_size == 2 * ROM_SIZE_512 ? 16 : 8, 0);
		break;
#endif
#ifdef CD32
	case EXTENDED_ROM_CD32:
		map_banks_set(&extendedkickmem_bank, 0xE0, 8, 0);
		break;
#endif
	case EXTENDED_ROM_ALG:
#ifdef ARCADIA
		map_banks_set(&extendedkickmem_bank, 0xF0, 4, 0);
		alg_map_banks();
#endif
		break;
	}

#ifdef AUTOCONFIG
	if ((need_uae_boot_rom (&currprefs) && currprefs.uaeboard == 0) || currprefs.uaeboard == 1)
		map_banks_set(&rtarea_bank, rtarea_base >> 16, 1, 0);
#endif

	if ((cloanto_rom || currprefs.cs_ksmirror_e0) && (currprefs.maprom != 0xe00000) && !extendedkickmem_type) {
		map_banks(&kickmem_bank, 0xE0, 8, 0);
	}
	if (currprefs.cs_ksmirror_a8) {
		if (extendedkickmem2a_bank.allocated_size && extendedkickmem2a_bank.baseaddr) {
			map_banks_set(&extendedkickmem2a_bank, extendedkickmem2a_bank.start >> 16, extendedkickmem2a_bank.allocated_size >> 16, 0);
			if (extendedkickmem2b_bank.allocated_size && extendedkickmem2b_bank.baseaddr) {
				map_banks_set(&extendedkickmem2b_bank, extendedkickmem2b_bank.start >> 16, extendedkickmem2b_bank.allocated_size >> 16, 0);
			}
		} else {
			struct romdata *rd = getromdatabypath (currprefs.cartfile);
			if (!rd || rd->id != 63) {
				if (extendedkickmem_type == EXTENDED_ROM_CD32 || extendedkickmem_type == EXTENDED_ROM_KS)
					map_banks(&extendedkickmem_bank, 0xb0, 8, 0);
				else
					map_banks(&kickmem_bank, 0xb0, 8, 0);
				map_banks(&kickmem_bank, 0xa8, 8, 0);
			}
		}
	} else if (extendedkickmem2a_bank.allocated_size && extendedkickmem2a_bank.baseaddr) {
		map_banks_set(&extendedkickmem2a_bank, extendedkickmem2a_bank.start >> 16, extendedkickmem2a_bank.allocated_size >> 16, 0);
		if (extendedkickmem2b_bank.allocated_size && extendedkickmem2b_bank.baseaddr) {
			map_banks_set(&extendedkickmem2b_bank, extendedkickmem2b_bank.start >> 16, extendedkickmem2b_bank.allocated_size >> 16, 0);
		}
	}

#ifdef ARCADIA
	if (is_arcadia_rom (currprefs.romextfile) == ARCADIA_BIOS) {
		if (_tcscmp (currprefs.romextfile, changed_prefs.romextfile) != 0)
			memcpy (currprefs.romextfile, changed_prefs.romextfile, sizeof currprefs.romextfile);
		if (_tcscmp (currprefs.cartfile, changed_prefs.cartfile) != 0)
			memcpy (currprefs.cartfile, changed_prefs.cartfile, sizeof currprefs.cartfile);
		arcadia_unmap ();
		is_arcadia_rom (currprefs.romextfile);
		is_arcadia_rom (currprefs.cartfile);
		arcadia_map_banks ();
	}
#endif

#ifdef ACTION_REPLAY
#ifdef ARCADIA
	if (!arcadia_bios) {
#endif
		action_replay_memory_reset ();
#ifdef ARCADIA
	}
#endif
#endif

	for (int i = 0; i < 2; i++) {
		if (currprefs.custom_memory_sizes[i]) {
			map_banks (i == 0 ? &custmem1_bank : &custmem2_bank,
				currprefs.custom_memory_addrs[i] >> 16,
				currprefs.custom_memory_sizes[i] >> 16, 0);
			if (currprefs.custom_memory_mask[i]) {
				for (int j = currprefs.custom_memory_addrs[i]; j & currprefs.custom_memory_mask[i]; j += currprefs.custom_memory_sizes[i]) {
					map_banks(i == 0 ? &custmem1_bank : &custmem2_bank, j >> 16, currprefs.custom_memory_sizes[i] >> 16, 0);
				}
			}
		}
	}

	if (mem_hardreset) {
		memory_clear ();
	}
	write_log (_T("memory init end\n"));
}

void memory_init (void)
{
	init_mem_banks ();
	virtualdevice_init ();

	chipmem_bank.reserved_size = 0;
	bogomem_bank.reserved_size = 0;
	extendedkickmem_bank.baseaddr = NULL;
	extendedkickmem_bank.reserved_size = 0;
	extendedkickmem2a_bank.baseaddr = NULL;
	extendedkickmem2a_bank.reserved_size = 0;
	extendedkickmem2b_bank.baseaddr = NULL;
	extendedkickmem2b_bank.reserved_size = 0;
	extendedkickmem_type = 0;
	chipmem_bank.baseaddr = 0;
	mem25bit_bank.reserved_size = 0;
	mem25bit_bank.baseaddr = NULL;
	a3000lmem_bank.reserved_size = a3000hmem_bank.reserved_size = 0;
	a3000lmem_bank.baseaddr = a3000hmem_bank.baseaddr = NULL;
	bogomem_bank.baseaddr = NULL;
	custmem1_bank.reserved_size = custmem2_bank.reserved_size = 0;
	custmem1_bank.baseaddr = NULL;
	custmem2_bank.baseaddr = NULL;

	mapped_free(&kickmem_bank);
	kickmem_init();
	_tcscpy (currprefs.romfile, _T("<none>"));
	currprefs.romextfile[0] = 0;
	cpuboard_reset(1);

#ifdef ACTION_REPLAY
	action_replay_unload (0);
	action_replay_load ();
	action_replay_init (1);
#ifdef ACTION_REPLAY_HRTMON
	hrtmon_load ();
#endif
#endif
}

void memory_cleanup (void)
{
	mapped_free(&mem25bit_bank);
	mapped_free(&a3000lmem_bank);
	mapped_free(&a3000hmem_bank);
	mapped_free(&bogomem_bank);
	mapped_free(&kickmem_bank);
	mapped_free(&kickram_bank);
	mapped_free(&extendedkickmem_bank);
	mapped_free(&extendedkickmem2a_bank);
	mapped_free(&extendedkickmem2b_bank);
	xfree(a1000_bootrom);
	mapped_free(&chipmem_bank);
	mapped_free(&custmem1_bank);
	mapped_free(&custmem2_bank);
	mapped_free(&fakeuaebootrom_bank);

	a1000_bootrom = NULL;
	a1000_kickstart_mode = 0;

	cpuboard_cleanup();
#ifdef ACTION_REPLAY
	action_replay_cleanup();
#endif
#ifdef ARCADIA
	arcadia_unmap ();
#endif
}

void set_roms_modified(void)
{
	roms_modified = true;
}

void memory_hardreset (int mode)
{
	if (mode + 1 > mem_hardreset)
		mem_hardreset = mode + 1;
}

// do not map if it conflicts with custom banks
void map_banks_cond (addrbank *bank, int start, int size, int realsize)
{
	for (int i = 0; i < MAX_CUSTOM_MEMORY_ADDRS; i++) {
		int cstart = currprefs.custom_memory_addrs[i] >> 16;
		if (!cstart)
			continue;
		int csize = currprefs.custom_memory_sizes[i] >> 16;
		if (!csize)
			continue;
		if (start <= cstart && start + size >= cstart)
			return;
		if (cstart <= start && (cstart + size >= start || start + size > cstart))
			return;
	}
	map_banks (bank, start, size, realsize);
}

#ifdef WITH_THREADED_CPU

struct addrbank_thread {
	addrbank *orig;
	addrbank ab;
};

#define MAX_THREAD_BANKS 200
static addrbank_thread *thread_banks[MAX_THREAD_BANKS];
addrbank *thread_mem_banks[MEMORY_BANKS];
static int thread_banks_used;

static void REGPARAM2 threadcpu_lput(uaecptr addr, uae_u32 l)
{
	//write_log(_T("LPUT %08x %08x %08x\n"), addr, l, M68K_GETPC);

	process_cpu_indirect_memory_write(addr, l, 2);
}
static void REGPARAM2 threadcpu_wput(uaecptr addr, uae_u32 w)
{
	//write_log(_T("WPUT %08x %08x %08x\n"), addr, w, M68K_GETPC);

	process_cpu_indirect_memory_write(addr, w, 1);
}
static void REGPARAM2 threadcpu_bput(uaecptr addr, uae_u32 b)
{
	//write_log(_T("BPUT %08x %08x %08x\n"), addr, b, M68K_GETPC);

	process_cpu_indirect_memory_write(addr, b, 0);
}
static uae_u32 REGPARAM2 threadcpu_lget(uaecptr addr)
{
	uae_u32 v = process_cpu_indirect_memory_read(addr, 2);

	//write_log(_T("LGET %08x %08x %08x\n"), addr, v, M68K_GETPC);

	return v;
}
static uae_u32 REGPARAM2 threadcpu_wget(uaecptr addr)
{
	uae_u32 v = process_cpu_indirect_memory_read(addr, 1);

	//write_log(_T("WGET %08x %08x %08x\n"), addr, v, M68K_GETPC);

	return v;
}

static uae_u32 REGPARAM2 threadcpu_bget(uaecptr addr)
{
	uae_u32 v = process_cpu_indirect_memory_read(addr, 0);

	//write_log(_T("BGET %08x %08x %08x\n"), addr, v, M68K_GETPC);

	return v;
}

static addrbank *get_bank_cpu_thread(addrbank *bank)
{
	if ((bank->flags & ABFLAG_THREADSAFE) && !(bank->flags & ABFLAG_IO))
		return bank;
	if (bank == &dummy_bank)
		return bank;

	for (int i = 0; i < thread_banks_used; i++) {
		if (thread_banks[i]->orig == bank) {
			return &thread_banks[i]->ab;
		}
	}
	struct addrbank_thread *at = thread_banks[thread_banks_used];
	if (!at)
		at = xcalloc(addrbank_thread, 1);
	thread_banks[thread_banks_used++] = at;
	at->orig = bank;
	memcpy(&at->ab, bank, sizeof(addrbank));
	addrbank *tb = &at->ab;
	tb->jit_read_flag = S_READ;
	tb->jit_write_flag = S_WRITE;
	tb->lget = threadcpu_lget;
	tb->wget = threadcpu_wget;
	tb->bget = threadcpu_bget;
	tb->lput = threadcpu_lput;
	tb->wput = threadcpu_wput;
	tb->bput = threadcpu_bput;
	// wgeti/lgeti should always point to real RAM
	return tb;
}
#endif

static void set_memory_cacheable(int bnr, addrbank *bank)
{
	uae_u8 cc = bank->flags >> ABFLAG_CACHE_SHIFT;
	if (!currprefs.mmu_model) {
		// if no MMU emulation, make sure chip RAM is not data cacheable
		if (bank->flags & ABFLAG_CHIPRAM) {
			cc &= ~(CACHE_ENABLE_DATA | CACHE_ENABLE_DATA_BURST);
		}
	}
	ce_cachable[bnr] = cc;
}


static void map_banks2 (addrbank *bank, int start, int size, int realsize, int quick)
{
	int bnr, old;
	unsigned long int hioffs = 0, endhioffs = 0x100;
	uae_u32 realstart = start;
	addrbank *orig_bank = NULL;

	bank->flags |= ABFLAG_MAPPED;

#ifdef WITH_THREADED_CPU
	if (currprefs.cpu_thread) {
		addrbank *b = bank;
		bank = get_bank_cpu_thread(bank);
		if (b != bank)
			orig_bank = b;
	}
#endif

#ifdef DEBUGGER
	if (quick <= 0)
		old = debug_bankchange (-1);
#endif
	flush_icache(3); /* Sure don't want to keep any old mappings around! */
#ifdef NATMEM_OFFSET
	if (!quick)
		delete_shmmaps (start << 16, size << 16);
#endif

	if (!realsize)
		realsize = size << 16;

	if ((size << 16) < realsize) {
		write_log (_T("Broken mapping, size=%x, realsize=%x\nStart is %x\n"),
			size, realsize, start);
	}

#ifndef ADDRESS_SPACE_24BIT
	if (start >= MEMORY_BANKS_24) {
		int real_left = 0;
		for (bnr = start; bnr < start + size; bnr++) {
			if (!real_left) {
				realstart = bnr;
				real_left = realsize >> 16;
#ifdef NATMEM_OFFSET
				if (!quick)
					add_shmmaps (realstart << 16, bank);
#endif
			}
			put_mem_bank (bnr << 16, bank, realstart << 16);
			set_memory_cacheable(bnr, bank);
#ifdef WITH_THREADED_CPU
			if (currprefs.cpu_thread) {
				if (orig_bank)
					put_mem_bank(bnr << 16, orig_bank, realstart << 16);
				thread_mem_banks[bnr] = orig_bank;
			}
#endif
			real_left--;
		}
#ifdef DEBUGGER
		if (quick <= 0)
			debug_bankchange (old);
#endif
		return;
	}
#endif
	if (last_address_space_24)
		endhioffs = 0x10000;
#ifdef ADDRESS_SPACE_24BIT
	endhioffs = 0x100;
#endif
	for (hioffs = 0; hioffs < endhioffs; hioffs += 0x100) {
		int real_left = 0;
		for (bnr = start; bnr < start + size; bnr++) {
			if (!real_left) {
				realstart = bnr + hioffs;
				real_left = realsize >> 16;
#ifdef NATMEM_OFFSET
				if (!quick)
					add_shmmaps (realstart << 16, bank);
#endif
			}
			put_mem_bank ((bnr + hioffs) << 16, bank, realstart << 16);
			set_memory_cacheable(bnr + hioffs, bank);
#ifdef WITH_THREADED_CPU
			if (currprefs.cpu_thread) {
				if (orig_bank)
					put_mem_bank((bnr + hioffs) << 16, bank, realstart << 16);
				thread_mem_banks[bnr + hioffs] = orig_bank;
			}
#endif
			real_left--;
		}
	}
#ifdef DEBUGGER
	if (quick <= 0)
		debug_bankchange (old);
#endif
	fill_ce_banks ();
}

#ifdef WITH_PPC
static void ppc_generate_map_banks(addrbank *bank, int start, int size)
{
	uae_u32 bankaddr = start << 16;
	uae_u32 banksize = size << 16;
	if (bank->sub_banks) {
		uae_u32 subbankaddr = bankaddr;
		addrbank *ab = NULL;
		for (int i = 0; i <= 65536; i += MEMORY_MIN_SUBBANK) {
			uae_u32 addr = bankaddr + i;
			addrbank *ab2 = get_sub_bank(&addr);
			if (ab2 != ab && ab != NULL) {
				ppc_map_banks(subbankaddr, (bankaddr + i) - subbankaddr, ab->name, (ab->flags & ABFLAG_PPCIOSPACE) ? NULL : ab->baseaddr, ab == &dummy_bank);
				subbankaddr = bankaddr + i;
			}
			ab = ab2;
		}
	} else {
		uae_u8 *baseaddr = bank->baseaddr;
		if (baseaddr) {
			baseaddr += bankaddr - bank->start;
		}
		// ABFLAG_PPCIOSPACE = map as indirect even if baseaddr is non-NULL
		ppc_map_banks(bankaddr, banksize, bank->name, (bank->flags & ABFLAG_PPCIOSPACE) ? NULL : baseaddr, bank == &dummy_bank);
	}
}
#endif

static addrbank *highram_temp_bank[65536 - 0x100];

void restore_banks(void)
{
	for (int bnr = 0x100; bnr < 65536; bnr++) {
		if (highram_temp_bank[bnr - 0x100]) {
			map_banks(highram_temp_bank[bnr - 0x100], bnr, 1, 0);
		} else {
			map_banks(&dummy_bank, bnr, 1, 0);
		}
	}
}

static void map_banks_draco(addrbank *bank, int start, int size, int realsize)
{
	// draco has Z2 space mapped at 0x03e80000-0x03fffff
	if (start >= 0xe8 && start < 0xf0) {
		map_banks(bank, 0x0300 + start, size, realsize);
	}
}

void map_banks (addrbank *bank, int start, int size, int realsize)
{
	if (start == 0xffffffff)
		return;

	if (currprefs.cs_compatible == CP_DRACO) {
		map_banks_draco(bank, start, size, realsize);
	}

#ifdef JIT
	if ((bank->jit_read_flag | bank->jit_write_flag) & S_N_ADDR) {
		jit_n_addr_unsafe = 1;
	}
#endif

	if (start >= MEMORY_BANKS_24) {
		int real_left = 0;
		for (int bnr = start; bnr < start + size; bnr++) {
			highram_temp_bank[bnr - 0x100] = bank;
		}
		if (currprefs.address_space_24)
			return;
	}
	map_banks2 (bank, start, size, realsize, 0);
#ifdef WITH_PPC
	ppc_generate_map_banks(bank, start, size);
#endif
	if ((bank->flags & ABFLAG_RAM) && !(bank->flags & ABFLAG_RTG) && ((start + size) << 16) > highest_ram) {
		highest_ram = (start + size) << 16;
	}
}

bool validate_banks_z3(addrbank *bank, int start, int size)
{
	if (start < 0x1000 || size <= 0) {
		error_log(_T("Z3 invalid map_banks(%s) start=%08x size=%08x\n"), bank->name, start << 16, size << 16);
		cpu_halt(CPU_HALT_AUTOCONFIG_CONFLICT);
		return false;
	}
	if (size > 0x4000 || start + size > 0xf000) {
		error_log(_T("Z3 invalid map_banks(%s) start=%08x size=%08x\n"), bank->name, start << 16, size << 16);
		return false;
	}
	for (int i = start; i < start + size; i++) {
		addrbank *ab = &get_mem_bank(start << 16);
		if (ab != &dummy_bank && ab != bank) {
			error_log(_T("Z3 map_banks(%s) attempting to override existing memory bank '%s' at %08x!\n"), bank->name, ab->name, i << 16);
			return false;
		}
	}
	return true;
}

void map_banks_z3(addrbank *bank, int start, int size)
{
	if (!validate_banks_z3(bank, start, size))
		return;
	map_banks(bank, start, size, 0);
}

bool validate_banks_z2(addrbank *bank, int start, int size)
{
	if (start < 0x20 || (start >= 0xa0 && start < 0xe9) || start >= 0xf0) {
		error_log(_T("Z2 map_banks(%s) with invalid start address %08X\n"), bank->name, start << 16);
		cpu_halt(CPU_HALT_AUTOCONFIG_CONFLICT);
		return false;
	}
	if (start >= 0xe9) {
		if (start + size > 0xf0) {
			error_log(_T("Z2 map_banks(%s) with invalid region %08x - %08X\n"), bank->name, start << 16, (start + size) << 16);
			cpu_halt(CPU_HALT_AUTOCONFIG_CONFLICT);
			return false;
		}
	} else {
		if (start + size > 0xa0) {
			error_log(_T("Z2 map_banks(%s) with invalid region %08x - %08X\n"), bank->name, start << 16, (start + size) << 16);
			cpu_halt(CPU_HALT_AUTOCONFIG_CONFLICT);
			return false;
		}
	}
	if (size <= 0 || size > 0x80) {
		error_log(_T("Z2 map_banks(%s) with invalid size %08x\n"), bank->name, size);
		cpu_halt(CPU_HALT_AUTOCONFIG_CONFLICT);
		return false;
	}
	for (int i = start; i < start + size; i++) {
		addrbank *ab = &get_mem_bank(start << 16);
		if (ab != &dummy_bank && bank != &dummy_bank) {
			error_log(_T("Z2 map_banks(%s) attempting to override existing memory bank '%s' at %08x!\n"), bank->name, ab->name, i << 16);
			return false;
		}
	}
	return true;
}


void map_banks_z2 (addrbank *bank, int start, int size)
{
	if (!validate_banks_z2(bank, start, size))
		return;
	map_banks (bank, start, size, 0);
}

uae_u32 map_banks_z2_autosize(addrbank *bank, int start)
{
	uae_u32 size = expansion_board_size(bank);
	if (!size) {
		error_log(_T("Z2 map_banks(%s) %08x, invalid size!\n"), bank->name, start << 16);
		return 0;
	}
	map_banks_z2(bank, start, size >> 16);
	return size;
}

void map_banks_quick (addrbank *bank, int start, int size, int realsize)
{
	map_banks2 (bank, start, size, realsize, 1);
#ifdef WITH_PPC
	ppc_generate_map_banks(bank, start, size);
#endif
}
void map_banks_nojitdirect (addrbank *bank, int start, int size, int realsize)
{
	map_banks2 (bank, start, size, realsize, -1);
#ifdef WITH_PPC
	ppc_generate_map_banks(bank, start, size);
#endif
}

#ifdef SAVESTATE

/* memory save/restore code */

uae_u8 *save_bootrom(size_t *len)
{
	if (!uae_boot_rom_type)
		return 0;
	*len = uae_boot_rom_size;
	return rtarea_bank.baseaddr;
}

uae_u8 *save_cram(size_t *len)
{
	*len = chipmem_bank.allocated_size;
	return chipmem_bank.baseaddr;
}

uae_u8 *save_bram(size_t *len)
{
	*len = bogomem_bank.allocated_size;
	return bogomem_bank.baseaddr;
}

static uae_u8 *save_mem25bitram(size_t *len)
{
	*len = mem25bit_bank.allocated_size;
	return mem25bit_bank.baseaddr;
}

uae_u8 *save_a3000lram(size_t *len)
{
	*len = a3000lmem_bank.allocated_size;
	return a3000lmem_bank.baseaddr;
}

uae_u8 *save_a3000hram(size_t *len)
{
	*len = a3000hmem_bank.allocated_size;
	return a3000hmem_bank.baseaddr;
}

void restore_bootrom(int len, size_t filepos)
{
	bootrom_filepos = filepos;
}

void restore_cram(int len, size_t filepos)
{
	chip_filepos = filepos;
	changed_prefs.chipmem.size = len;
}

void restore_bram(int len, size_t filepos)
{
	bogo_filepos = filepos;
	changed_prefs.bogomem.size = len;
}

void restore_a3000lram(int len, size_t filepos)
{
	a3000lmem_filepos = filepos;
	changed_prefs.mbresmem_low.size = len;
}

void restore_a3000hram(int len, size_t filepos)
{
	a3000hmem_filepos = filepos;
	changed_prefs.mbresmem_high.size = len;
}

uae_u8 *restore_rom(uae_u8 *src)
{
	uae_u32 crc32, mem_start, mem_size, mem_type, version;
	TCHAR *s, *romn;
	int i, crcdet;
	struct romlist *rl = romlist_getit ();

	mem_start = restore_u32 ();
	mem_size = restore_u32 ();
	mem_type = restore_u32 ();
	version = restore_u32 ();
	crc32 = restore_u32 ();
	romn = restore_string ();
	crcdet = 0;
	for (i = 0; i < romlist_count (); i++) {
		if (rl[i].rd->crc32 == crc32 && crc32) {
			if (zfile_exists (rl[i].path)) {
				switch (mem_type)
				{
				case 0:
					_tcsncpy (changed_prefs.romfile, rl[i].path, 255);
					break;
				case 1:
					_tcsncpy (changed_prefs.romextfile, rl[i].path, 255);
					break;
				}
				write_log (_T("ROM '%s' = '%s'\n"), romn, rl[i].path);
				crcdet = 1;
			} else {
				write_log (_T("ROM '%s' = '%s' invalid rom scanner path!"), romn, rl[i].path);
			}
			break;
		}
	}
	s = restore_string ();
	if (!crcdet) {
		if (zfile_exists (s)) {
			switch (mem_type)
			{
			case 0:
				_tcsncpy (changed_prefs.romfile, s, 255);
				break;
			case 1:
				_tcsncpy (changed_prefs.romextfile, s, 255);
				break;
			}
			write_log (_T("ROM detected (path) as '%s'\n"), s);
			crcdet = 1;
		}
	}
	xfree (s);
	if (!crcdet)
		write_log (_T("WARNING: ROM '%s' %d.%d (CRC32=%08x %08x-%08x) not found!\n"),
			romn, version >> 16, version & 0xffff, crc32, mem_start, mem_start + mem_size - 1);
	xfree (romn);
	return src;
}

uae_u8 *save_rom(int first, size_t *len, uae_u8 *dstptr)
{
	static int count;
	uae_u8 *dst, *dstbak;
	uae_u8 *mem_real_start;
	uae_u32 version;
	TCHAR *path;
	int mem_start, mem_size, mem_type, saverom;
	int i;
	TCHAR tmpname[1000];

	version = 0;
	saverom = 0;
	if (first)
		count = 0;
	for (;;) {
		mem_type = count;
		mem_size = 0;
		switch (count) {
		case 0: /* Kickstart ROM */
			mem_start = 0xf80000;
			mem_real_start = kickmem_bank.baseaddr;
			mem_size = kickmem_bank.allocated_size;
			path = currprefs.romfile;
			/* 256KB or 512KB ROM? */
			for (i = 0; i < mem_size / 2 - 4; i++) {
				if (get_long(i + mem_start) != get_long(i + mem_start + mem_size / 2))
					break;
			}
			if (i == mem_size / 2 - 4) {
				mem_size /= 2;
				mem_start += ROM_SIZE_256;
			}
			version = get_long(mem_start + 12); /* version+revision */
			_sntprintf (tmpname, sizeof tmpname, _T("Kickstart %d.%d"), get_word(mem_start + 12), get_word(mem_start + 14));
			break;
		case 1: /* Extended ROM */
			if (!extendedkickmem_type)
				break;
			mem_start = extendedkickmem_bank.start;
			mem_real_start = extendedkickmem_bank.baseaddr;
			mem_size = extendedkickmem_bank.allocated_size;
			path = currprefs.romextfile;
			version = get_long(mem_start + 12); /* version+revision */
			if (version == 0xffffffff)
				version = get_long(mem_start + 16);
			_sntprintf (tmpname, sizeof tmpname, _T("Extended"));
			break;
		default:
			return 0;
		}
		count++;
		if (mem_size)
			break;
	}
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 4 + 4 + 4 + 4 + 4 + 256 + 256 + mem_size);
	save_u32 (mem_start);
	save_u32 (mem_size);
	save_u32 (mem_type);
	save_u32 (version);
	save_u32 (get_crc32 (mem_real_start, mem_size));
	save_string (tmpname);
	save_string (path);
	if (saverom) {
		for (i = 0; i < mem_size; i++)
			*dst++ = get_byte(mem_start + i);
	}
	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */

static void REGPARAM2 empty_put(uaecptr addr, uae_u32 v)
{
}

void loadboardfile(addrbank *ab, struct boardloadfile * lf)
{
	if (!ab->baseaddr)
		return;
	if (!lf->loadfile[0])
		return;
	struct zfile* zf = zfile_fopen(lf->loadfile, _T("rb"));
	if (zf) {
		int size = lf->filesize;
		if (!size) {
			size = ab->allocated_size;
		}
		else if (lf->loadoffset + size > ab->allocated_size)
			size = ab->allocated_size - lf->loadoffset;
		if (size > 0) {
			int total = zfile_fread32(ab->baseaddr + lf->loadoffset, 1, size, zf);
			write_log(_T("Expansion file '%s': load %u bytes, offset %u, start addr %08x\n"),
				lf->loadfile, total, lf->loadoffset, ab->start + lf->loadoffset);
		}
		zfile_fclose(zf);
	}
	else {
		write_log(_T("Couldn't open expansion file '%s'\n"), lf->loadfile);
	}
}

void initramboard(addrbank *ab, struct ramboard *rb)
{
	ab->flags &= ~ABFLAG_NODMA;
	if (rb->nodma)
		ab->flags |= ABFLAG_NODMA;
	if (!ab->baseaddr)
		return;
	loadboardfile(ab, &rb->lf);
	if (rb->readonly) {
		ab->lput = empty_put;
		ab->wput = empty_put;
		ab->bput = empty_put;
		ab->jit_write_flag = 0;
	}
}


/* memory helpers */

void memcpyha_safe(uaecptr dst, const uae_u8 *src, int size)
{
	if (!addr_valid (_T("memcpyha"), dst, size))
		return;
	while (size--)
		put_byte (dst++, *src++);
}
void memcpyha(uaecptr dst, const uae_u8 *src, int size)
{
	while (size--)
		put_byte (dst++, *src++);
}
void memcpyah_safe(uae_u8 *dst, uaecptr src, int size)
{
	if (!addr_valid (_T("memcpyah"), src, size))
		return;
	while (size--)
		*dst++ = get_byte (src++);
}
void memcpyah (uae_u8 *dst, uaecptr src, int size)
{
	while (size--)
		*dst++ = get_byte (src++);
}
uae_char *strcpyah_safe (uae_char *dst, uaecptr src, int maxsize)
{
	uae_char *res = dst;
	uae_u8 b;
	dst[0] = 0;
	do {
		if (!addr_valid (_T("_tcscpyah"), src, 1))
			return res;
		b = get_byte (src++);
		*dst++ = b;
		*dst = 0;
		maxsize--;
		if (maxsize <= 1)
			break;
	} while (b);
	return res;
}
uaecptr strcpyha_safe (uaecptr dst, const uae_char *src)
{
	uaecptr res = dst;
	uae_u8 b;
	do {
		if (!addr_valid (_T("_tcscpyha"), dst, 1))
			return res;
		b = *src++;
		put_byte (dst++, b);
	} while (b);
	return res;
}

uae_u32 memory_get_longi(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_r) {
		return call_mem_get_func(ab->lgeti, addr);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_r + addr;
		return do_get_mem_long((uae_u32 *)m);
	}
}
uae_u32 memory_get_wordi(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_r) {
		return call_mem_get_func(ab->wgeti, addr);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_r + addr;
		return do_get_mem_word((uae_u16*)m);
	}
}
uae_u32 memory_get_long(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_r) {
		return call_mem_get_func(ab->lget, addr);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_r + addr;
		return do_get_mem_long((uae_u32*)m);
	}
}
uae_u32 memory_get_word(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_r) {
		return call_mem_get_func(ab->wget, addr);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_r + addr;
		return do_get_mem_word((uae_u16*)m);
	}
}
uae_u32 memory_get_byte(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_r) {
		return call_mem_get_func(ab->bget, addr);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_r + addr;
		return *m;
	}
}

void memory_put_long(uaecptr addr, uae_u32 v)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_w) {
		call_mem_put_func(ab->lput, addr, v);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_w + addr;
		do_put_mem_long((uae_u32*)m, v);
	}
}
void memory_put_word(uaecptr addr, uae_u32 v)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_w) {
		call_mem_put_func(ab->wput, addr, v);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_w + addr;
		do_put_mem_word((uae_u16*)m, v);
	}
}
void memory_put_byte(uaecptr addr, uae_u32 v)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_w) {
		call_mem_put_func(ab->bput, addr, v);
	} else {
		uae_u8 *m;
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		m = ab->baseaddr_direct_w + addr;
		*m = (uae_u8)v;
	}
}

uae_u8 *memory_get_real_address(uaecptr addr)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_r) {
		return get_mem_bank(addr).xlateaddr(addr);
	} else {
		addr -= ab->startaccessmask;
		addr &= ab->mask;
		return ab->baseaddr_direct_r + addr;
	}
}

int memory_valid_address(uaecptr addr, uae_u32 size)
{
	addrbank *ab = &get_mem_bank(addr);
	if (!ab->baseaddr_direct_r) {
		return get_mem_bank(addr).check(addr, size);
	}
	addr -= ab->startaccessmask;
	addr &= ab->mask;
	return addr + size <= ab->allocated_size;
}

void dma_put_word(uaecptr addr, uae_u16 v)
{
	addrbank* ab = &get_mem_bank(addr);
	if (ab->flags & ABFLAG_NODMA)
		return;
	put_word(addr, v);
}
void dma_put_byte(uaecptr addr, uae_u8 v)
{
	addrbank* ab = &get_mem_bank(addr);
	if (ab->flags & ABFLAG_NODMA)
		return;
	put_byte(addr, v);
}
uae_u16 dma_get_word(uaecptr addr)
{
	addrbank* ab = &get_mem_bank(addr);
	if (ab->flags & ABFLAG_NODMA)
		return 0xffff;
	return get_word(addr);
}
uae_u8 dma_get_byte(uaecptr addr)
{
	addrbank* ab = &get_mem_bank(addr);
	if (ab->flags & ABFLAG_NODMA)
		return 0xff;
	return get_byte(addr);
}
