 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Memory management
  *
  * (c) 1995 Bernd Schmidt
  */

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
#include "akiko.h"
#include "gayle.h"
#include "devices.h"

#ifdef JIT
/* Set by each memory handler that does not simply access real memory. */
int special_mem;
#endif
static int mem_hardreset;

static size_t bootrom_filepos, chip_filepos, bogo_filepos, a3000lmem_filepos, a3000hmem_filepos;

/* The address space setting used during the last reset.  */
static bool last_address_space_24;

addrbank *mem_banks[MEMORY_BANKS];

int addr_valid(const TCHAR *txt, uaecptr addr, uae_u32 len)
{
	addrbank *ab = &get_mem_bank(addr);
  if (ab == 0 || !(ab->flags & (ABFLAG_RAM | ABFLAG_ROM)) || addr < 0x100 || len > 16777215 || !valid_address(addr, len)) {
		write_log(_T("corrupt %s pointer %x (%d) detected!\n"), txt, addr, len);
		return 0;
	}
	return 1;
}

/* A dummy bank that only contains zeros */

static uae_u32 REGPARAM3 dummy_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dummy_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dummy_bget (uaecptr) REGPARAM;
static void REGPARAM3 dummy_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dummy_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dummy_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 dummy_check (uaecptr addr, uae_u32 size) REGPARAM;

#define	MAX_ILG 1000
#define NONEXISTINGDATA 0
//#define NONEXISTINGDATA 0xffffffff

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
	return true;
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
	uae_u32 mask = size == 4 ? 0xffffffff : (1 << (size * 8)) - 1;
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
	  if (size == 4) {
			v = regs.db & 0xffff;
			if (addr & 1)
				v = (v << 8) | (v >> 8);
			v = (v << 16) | v;
	  } else if (size == 2) {
			v = regs.db & 0xffff;
			if (addr & 1)
				v = (v << 8) | (v >> 8);
	  } else {
			v = regs.db;
			v = (addr & 1) ? (v & 0xff) : ((v >> 8) & 0xff);
		}
	}
	return v & mask;
}

uae_u32 dummy_get(uaecptr addr, int size, bool inst, uae_u32 defvalue)
{
	uae_u32 v = defvalue;

	if (gary_nonrange(addr) || (size > 1 && gary_nonrange(addr + size - 1))) {
		if (gary_toenb)
			exception2(addr, false, size, (regs.s ? 4 : 0) | (inst ? 0 : 1));
		return v;
	}

	return dummy_get_safe(addr, size, inst, defvalue);
}

static uae_u32 REGPARAM2 dummy_lget(uaecptr addr)
{
	return dummy_get(addr, 4, false, nonexistingdata());
}

static uae_u32 REGPARAM2 dummy_wget(uaecptr addr)
{
	return dummy_get(addr, 2, false, nonexistingdata());
}
uae_u32 REGPARAM2 dummy_wgeti(uaecptr addr)
{
	return dummy_get(addr, 2, true, nonexistingdata());
}

static uae_u32 REGPARAM2 dummy_bget(uaecptr addr)
{
	return dummy_get(addr, 1, false, nonexistingdata());
}

static void REGPARAM2 dummy_lput(uaecptr addr, uae_u32 l)
{
}
static void REGPARAM2 dummy_wput(uaecptr addr, uae_u32 w)
{
}
static void REGPARAM2 dummy_bput(uaecptr addr, uae_u32 b)
{
}

static int REGPARAM2 dummy_check(uaecptr addr, uae_u32 size)
{
	return 0;
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
uae_u32 REGPARAM3 sub_bank_lget(uaecptr addr) REGPARAM
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

uae_u32 chipmem_full_mask;

static uae_u32 REGPARAM2 chipmem_lget(uaecptr addr)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	return do_get_mem_long(m);
}

static uae_u32 REGPARAM2 chipmem_wget(uaecptr addr)
{
	uae_u16 *m, v;

	addr &= chipmem_bank.mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	v = do_get_mem_word(m);
	return v;
}

static uae_u32 REGPARAM2 chipmem_bget(uaecptr addr)
{
	uae_u8 v;
	addr &= chipmem_bank.mask;
	v = chipmem_bank.baseaddr[addr];
	return v;
}

void REGPARAM2 chipmem_lput(uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_long(m, l);
}

void REGPARAM2 chipmem_wput(uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr &= chipmem_bank.mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_word(m, w);
}

void REGPARAM2 chipmem_bput(uaecptr addr, uae_u32 b)
{
	addr &= chipmem_bank.mask;
	chipmem_bank.baseaddr[addr] = b;
}

void REGPARAM2 chipmem_agnus_wput(uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr &= chipmem_full_mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_word(m, w);
}

static int REGPARAM2 chipmem_check(uaecptr addr, uae_u32 size)
{
	addr &= chipmem_bank.mask;
  return (addr + size) <= chipmem_bank.reserved_size;
}

static uae_u8 *REGPARAM2 chipmem_xlate(uaecptr addr)
{
	addr &= chipmem_bank.mask;
	return chipmem_bank.baseaddr + addr;
}

/* Slow memory */

MEMORY_FUNCTIONS(bogomem);

/* A3000 motherboard fast memory */

MEMORY_FUNCTIONS(a3000lmem);
MEMORY_FUNCTIONS(a3000hmem);

/* Kick memory */

uae_u16 kickstart_version;

static void REGPARAM3 kickmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 kickmem_bput (uaecptr, uae_u32) REGPARAM;

MEMORY_BGET(kickmem);
MEMORY_WGET(kickmem);
MEMORY_LGET(kickmem);
MEMORY_CHECK(kickmem);
MEMORY_XLATE(kickmem);

static void REGPARAM2 kickmem_lput (uaecptr addr, uae_u32 b)
{
}

static void REGPARAM2 kickmem_wput (uaecptr addr, uae_u32 b)
{
}

static void REGPARAM2 kickmem_bput (uaecptr addr, uae_u32 b)
{
}

/* CD32/CDTV extended kick memory */

static int extendedkickmem_type;

#define EXTENDED_ROM_CD32 1
#define EXTENDED_ROM_KS 3

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
}
static void REGPARAM2 extendedkickmem_wput (uaecptr addr, uae_u32 b)
{
}
static void REGPARAM2 extendedkickmem_bput (uaecptr addr, uae_u32 b)
{
}


static void REGPARAM3 extendedkickmem2_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 extendedkickmem2_bput (uaecptr, uae_u32) REGPARAM;

MEMORY_BGET(extendedkickmem2);
MEMORY_WGET(extendedkickmem2);
MEMORY_LGET(extendedkickmem2);
MEMORY_CHECK(extendedkickmem2);
MEMORY_XLATE(extendedkickmem2);

static void REGPARAM2 extendedkickmem2_lput (uaecptr addr, uae_u32 b)
{
}
static void REGPARAM2 extendedkickmem2_wput (uaecptr addr, uae_u32 b)
{
}
static void REGPARAM2 extendedkickmem2_bput (uaecptr addr, uae_u32 b)
{
}

/* Default memory access functions */

int REGPARAM2 default_check (uaecptr a, uae_u32 b)
{
  return 0;
}

static int be_cnt, be_recursive;

uae_u8 *REGPARAM2 default_xlate(uaecptr addr)
{
	if (be_recursive)
	{
		cpu_halt(CPU_HALT_OPCODE_FETCH_FROM_NON_EXISTING_ADDRESS);
		return kickmem_xlate(2);
	}

	be_recursive++;
	int size = currprefs.cpu_model >= 68020 ? 4 : 2;
	if (quit_program == 0)
	{
		/* do this only in 68010+ mode, there are some tricky A500 programs.. */
		if (currprefs.cpu_model > 68000 || !currprefs.cpu_compatible)
		{
			if (be_cnt < 3)
			{
				write_log(_T("Your Amiga program just did something terribly stupid %08X PC=%08X\n"), addr, M68K_GETPC);
			}
			if (gary_toenb && (gary_nonrange(addr) || (size > 1 && gary_nonrange(addr + size - 1))))
			{
				exception2(addr, false, size, regs.s ? 4 : 0);
			}
			else
			{
				cpu_halt(CPU_HALT_OPCODE_FETCH_FROM_NON_EXISTING_ADDRESS);
			}
		}
	}
	be_recursive--;
	return kickmem_xlate(2); /* So we don't crash. */
}

/* Address banks */

addrbank dummy_bank = {
	dummy_lget, dummy_wget, dummy_bget,
	dummy_lput, dummy_wput, dummy_bput,
	default_xlate, dummy_check, NULL, NULL, NULL,
	dummy_wgeti,
	ABFLAG_NONE, S_READ, S_WRITE
};

addrbank chipmem_bank = {
	chipmem_lget, chipmem_wget, chipmem_bget,
	chipmem_lput, chipmem_wput, chipmem_bput,
	chipmem_xlate, chipmem_check, NULL, _T("chip"), _T("Chip memory"),
	chipmem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE | ABFLAG_CHIPRAM, 0, 0
};

addrbank bogomem_bank = {
	bogomem_lget, bogomem_wget, bogomem_bget,
	bogomem_lput, bogomem_wput, bogomem_bput,
	bogomem_xlate, bogomem_check, NULL, _T("bogo"), _T("Slow memory"),
	bogomem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

addrbank a3000lmem_bank = {
	a3000lmem_lget, a3000lmem_wget, a3000lmem_bget,
	a3000lmem_lput, a3000lmem_wput, a3000lmem_bput,
	a3000lmem_xlate, a3000lmem_check, NULL, _T("ramsey_low"), _T("RAMSEY memory (low)"),
	a3000lmem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

addrbank a3000hmem_bank = {
	a3000hmem_lget, a3000hmem_wget, a3000hmem_bget,
	a3000hmem_lput, a3000hmem_wput, a3000hmem_bput,
	a3000hmem_xlate, a3000hmem_check, NULL, _T("ramsey_high"), _T("RAMSEY memory (high)"),
	a3000hmem_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

addrbank kickmem_bank = {
	kickmem_lget, kickmem_wget, kickmem_bget,
	kickmem_lput, kickmem_wput, kickmem_bput,
	kickmem_xlate, kickmem_check, NULL, _T("kick"), _T("Kickstart ROM"),
	kickmem_wget,
	ABFLAG_ROM | ABFLAG_THREADSAFE, 0, S_WRITE
};

addrbank extendedkickmem_bank = {
	extendedkickmem_lget, extendedkickmem_wget, extendedkickmem_bget,
	extendedkickmem_lput, extendedkickmem_wput, extendedkickmem_bput,
	extendedkickmem_xlate, extendedkickmem_check, NULL, NULL, _T("Extended Kickstart ROM"),
	extendedkickmem_wget,
	ABFLAG_ROM | ABFLAG_THREADSAFE, 0, S_WRITE
};
addrbank extendedkickmem2_bank = {
	extendedkickmem2_lget, extendedkickmem2_wget, extendedkickmem2_bget,
	extendedkickmem2_lput, extendedkickmem2_wput, extendedkickmem2_bput,
	extendedkickmem2_xlate, extendedkickmem2_check, NULL, _T("rom_a8"), _T("Extended 2nd Kickstart ROM"),
	extendedkickmem2_wget,
	ABFLAG_ROM | ABFLAG_THREADSAFE, 0, S_WRITE
};

MEMORY_FUNCTIONS(custmem1);
MEMORY_FUNCTIONS(custmem2);

addrbank custmem1_bank = {
	custmem1_lget, custmem1_wget, custmem1_bget,
	custmem1_lput, custmem1_wput, custmem1_bput,
	custmem1_xlate, custmem1_check, NULL, _T("custmem1"), _T("Non-autoconfig RAM #1"),
	custmem1_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};
addrbank custmem2_bank = {
	custmem2_lget, custmem2_wget, custmem2_bget,
	custmem2_lput, custmem2_wput, custmem2_bput,
	custmem2_xlate, custmem2_check, NULL, _T("custmem2"), _T("Non-autoconfig RAM #2"),
	custmem2_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

static bool singlebit (uae_u32 v)
{
	while (v && !(v & 1))
		v >>= 1;
	return (v & ~1) == 0;
}

static void allocate_memory_custombanks(void)
{
	if (custmem1_bank.reserved_size != currprefs.custom_memory_sizes[0]) {
		mapped_free (&custmem1_bank);
		custmem1_bank.reserved_size = currprefs.custom_memory_sizes[0];
		// custmem1 and 2 can have non-power of 2 size so only set correct mask if size is power of 2.
		custmem1_bank.mask = singlebit (custmem1_bank.reserved_size) ? custmem1_bank.reserved_size - 1 : -1;
		custmem1_bank.start = currprefs.custom_memory_addrs[0];
		if (custmem1_bank.reserved_size) {
			if (!mapped_malloc (&custmem1_bank))
				custmem1_bank.reserved_size = 0;
		}
	}
	if (custmem2_bank.reserved_size != currprefs.custom_memory_sizes[1]) {
		mapped_free (&custmem2_bank);
		custmem2_bank.reserved_size = currprefs.custom_memory_sizes[1];
		custmem2_bank.mask = singlebit (custmem2_bank.reserved_size) ? custmem2_bank.reserved_size - 1 : -1;
		custmem2_bank.start = currprefs.custom_memory_addrs[1];
		if (custmem2_bank.reserved_size) {
			if (!mapped_malloc (&custmem2_bank))
				custmem2_bank.reserved_size = 0;
		}
	}
}

static const uae_char *kickstring = "exec.library";

static int read_kickstart(struct zfile *f, uae_u8 *mem, int size, int dochecksum, int noalias)
{
	uae_char buffer[20];
	int i, j, oldpos;
	int cr = 0, kickdisk = 0;

	if (size < 0) {
		zfile_fseek(f, 0, SEEK_END);
		size = zfile_ftell(f) & ~0x3ff;
		zfile_fseek(f, 0, SEEK_SET);
	}
	oldpos = zfile_ftell(f);
  i = zfile_fread (buffer, 1, sizeof(buffer), f);
	if (i < sizeof(buffer))
		return 0;
	if (!memcmp(buffer, "KICK", 4)) {
		zfile_fseek(f, 512, SEEK_SET);
		kickdisk = 1;
	}
	else if (memcmp((uae_char*)buffer, "AMIROMTYPE1", 11) != 0) {
		zfile_fseek(f, oldpos, SEEK_SET);
	}
	else {
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
		mem[size - 18] = size >> 8;
		mem[size - 17] = size >> 0;
	}

	i = zfile_fread(mem, 1, size, f);

	if (kickdisk && i > ROM_SIZE_256)
		i = ROM_SIZE_256;
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

static bool load_extendedkickstart(const TCHAR *romextfile, int type)
{
	struct zfile *f;
	int size, off;
	bool ret = false;

	if (_tcslen(romextfile) == 0)
		return false;
	f = read_rom_name(romextfile);
	if (!f)
	{
		notify_user(NUMSG_NOEXTROM);
		return false;
	}
	zfile_fseek(f, 0, SEEK_END);
	size = zfile_ftell(f);
	extendedkickmem_bank.reserved_size = ROM_SIZE_512;
	off = 0;
	if (type == 0)
	{
		if (currprefs.cs_cd32cd)
		{
			extendedkickmem_type = EXTENDED_ROM_CD32;
		}
		else if (size > 300000)
		{
			extendedkickmem_type = EXTENDED_ROM_CD32;
		}
	}
	else
	{
		extendedkickmem_type = type;
	}
	if (extendedkickmem_type)
	{
		zfile_fseek(f, off, SEEK_SET);
		switch (extendedkickmem_type)
		{
		case EXTENDED_ROM_CD32:
			extendedkickmem_bank.label = _T("rom_e0");
			mapped_malloc(&extendedkickmem_bank);
			extendedkickmem_bank.start = 0xe00000;
			break;
		}
		if (extendedkickmem_bank.baseaddr)
		{
			read_kickstart(f, extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated_size, 0, 1);
			extendedkickmem_bank.mask = extendedkickmem_bank.allocated_size - 1;
			ret = true;
		}
	}
	zfile_fclose(f);
	return ret;
}

extern unsigned char arosrom[];
extern unsigned int arosrom_len;
static bool load_kickstart_replacement(void)
{
	struct zfile* f = zfile_fopen_data(_T("aros.gz"), arosrom_len, arosrom);
	if (!f)
		return false;
	f = zfile_gunzip(f);
	if (!f)
		return false;

	extendedkickmem_bank.reserved_size = ROM_SIZE_512;
	extendedkickmem_bank.mask = ROM_SIZE_512 - 1;
	extendedkickmem_bank.label = _T("rom_e0");
	extendedkickmem_type = EXTENDED_ROM_KS;
	mapped_malloc(&extendedkickmem_bank);
	read_kickstart(f, extendedkickmem_bank.baseaddr, ROM_SIZE_512, 0, 1);

	kickmem_bank.reserved_size = ROM_SIZE_512;
	kickmem_bank.mask = ROM_SIZE_512 - 1;
	read_kickstart(f, kickmem_bank.baseaddr, ROM_SIZE_512, 1, 0);

	zfile_fclose(f);

	// if 68000-68020 config without any other fast ram with m68k aros: enable special extra RAM.
	if (currprefs.cpu_model <= 68020 &&
		currprefs.cachesize == 0 &&
		currprefs.fastmem[0].size == 0 &&
		currprefs.z3fastmem[0].size == 0 &&
		currprefs.mbresmem_high_size == 0 &&
		currprefs.mbresmem_low_size == 0) {

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

/* disable incompatible drivers */
static int patch_residents (uae_u8 *kickmemory, int size)
{
  int i, j, patched = 0;
  const uae_char *residents[] = { "NCR scsi.device", NULL };
  // "scsi.device", "carddisk.device", "card.resource" };
  uaecptr base = size == ROM_SIZE_512 ? 0xf80000 : 0xfc0000;

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
  return patched;
}

static void patch_kick(void)
{
  int patched = 0;
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

	f = read_rom_name(p->romfile);
	_tcscpy(tmprom, p->romfile);
	_tcscpy(tmprom2, p->romfile);
  if (f == NULL) {
		_stprintf(tmprom2, _T("%s%s"), start_path_data, p->romfile);
		f = rom_fopen(tmprom2, _T("rb"), ZFD_NORMAL);
  	if (f == NULL) {
	    _stprintf (tmprom2, _T("%sroms/kick.rom"), start_path_data);
    	f = rom_fopen (tmprom2, _T("rb"), ZFD_NORMAL);
    	if (f == NULL) {
    		_stprintf (tmprom2, _T("%skick.rom"), start_path_data);
	      f = rom_fopen(tmprom2, _T("rb"), ZFD_NORMAL);
				if (f == NULL) {
					f = read_rom_name_guess (tmprom, tmprom2);
			}
		}
		}
	}
	if (f) {
    _tcscpy (p->romfile, tmprom2);
  }
	return f;
}

static int load_kickstart(void)
{
	TCHAR tmprom[MAX_DPATH];
	cloanto_rom = 0;
	if (!_tcscmp(currprefs.romfile, _T(":AROS")))
	{
		return load_kickstart_replacement();
	}
	_tcscpy(tmprom, currprefs.romfile);
	struct zfile *f = get_kickstart_filehandle(&currprefs);
	addkeydir(currprefs.romfile);
	if (f == NULL) /* still no luck */
		goto err;

	if (f != NULL)
	{
		int filesize, size, maxsize;
		int kspos = ROM_SIZE_512;
		int extpos = 0;

		maxsize = ROM_SIZE_512;
		zfile_fseek(f, 0, SEEK_END);
		filesize = zfile_ftell(f);
		zfile_fseek(f, 0, SEEK_SET);
		if (filesize == 1760 * 512)
		{
			filesize = ROM_SIZE_256;
			maxsize = ROM_SIZE_256;
		}
		if (filesize == ROM_SIZE_512 + 8)
		{
			/* GVP 0xf0 kickstart */
			zfile_fseek(f, 8, SEEK_SET);
		}
		if (filesize >= ROM_SIZE_512 * 2)
		{
			struct romdata *rd = getromdatabyzfile(f);
			zfile_fseek(f, kspos, SEEK_SET);
		}
		if (filesize >= ROM_SIZE_512 * 4)
		{
			kspos = ROM_SIZE_512 * 3;
			extpos = 0;
			zfile_fseek(f, kspos, SEEK_SET);
		}
		size = read_kickstart(f, kickmem_bank.baseaddr, maxsize, 1, 0);
		if (size == 0)
			goto err;
		kickmem_bank.mask = size - 1;
		kickmem_bank.reserved_size = size;
		if (filesize >= ROM_SIZE_512 * 2 && !extendedkickmem_type)
		{
			extendedkickmem_bank.reserved_size = ROM_SIZE_512;
			extendedkickmem_type = EXTENDED_ROM_KS;
			extendedkickmem_bank.label = _T("rom_e0");
			extendedkickmem_bank.start = 0xe00000;
			mapped_malloc(&extendedkickmem_bank);
			zfile_fseek(f, extpos, SEEK_SET);
			read_kickstart(f, extendedkickmem_bank.baseaddr, extendedkickmem_bank.allocated_size, 0, 1);
			extendedkickmem_bank.mask = extendedkickmem_bank.allocated_size - 1;
		}
		if (filesize > ROM_SIZE_512 * 2)
		{
			extendedkickmem2_bank.reserved_size = ROM_SIZE_512 * 2;
			mapped_malloc(&extendedkickmem2_bank);
			zfile_fseek(f, extpos + ROM_SIZE_512, SEEK_SET);
			read_kickstart(f, extendedkickmem2_bank.baseaddr, ROM_SIZE_512, 0, 1);
			zfile_fseek(f, extpos + ROM_SIZE_512 * 2, SEEK_SET);
			read_kickstart(f, extendedkickmem2_bank.baseaddr + ROM_SIZE_512, ROM_SIZE_512, 0, 1);
			extendedkickmem2_bank.mask = extendedkickmem2_bank.allocated_size - 1;
			extendedkickmem2_bank.start = 0xa80000;
		}
	}

	kickstart_version = (kickmem_bank.baseaddr[12] << 8) | kickmem_bank.baseaddr[13];
	if (kickstart_version == 0xffff)
	{
		// 1.0-1.1 and older
		kickstart_version = (kickmem_bank.baseaddr[16] << 8) | kickmem_bank.baseaddr[17];
		if (kickstart_version > 33)
			kickstart_version = 0;
	}
	zfile_fclose(f);
	return 1;
err:
	_tcscpy(currprefs.romfile, tmprom);
	zfile_fclose(f);
	return 0;
}

static void init_mem_banks(void)
{
	// unsigned so i << 16 won't overflow to negative when i >= 32768
	for (unsigned int i = 0; i < MEMORY_BANKS; i++)
		mem_banks[i] = &dummy_bank;
}

static void map_banks_set(addrbank *bank, int start, int size, int realsize)
{
	map_banks(bank, start, size, realsize);
}

static void allocate_memory(void)
{
	if (chipmem_bank.reserved_size != currprefs.chipmem_size) {
		int memsize;
		mapped_free(&chipmem_bank);
		if (currprefs.chipmem_size > 2 * 1024 * 1024) {
			free_fastmemory(0);
		}

		memsize = chipmem_bank.reserved_size = currprefs.chipmem_size;
		chipmem_full_mask = chipmem_bank.mask = chipmem_bank.reserved_size - 1;
		chipmem_bank.start = chipmem_start_addr;
		if (!currprefs.cachesize && memsize < 0x100000)
			memsize = 0x100000;
		if (memsize > 0x100000 && memsize < 0x200000)
			memsize = 0x200000;
		chipmem_bank.reserved_size = memsize;
		mapped_malloc(&chipmem_bank);
		chipmem_bank.reserved_size = currprefs.chipmem_size;
		chipmem_bank.allocated_size = currprefs.chipmem_size;
		if (chipmem_bank.baseaddr == 0) {
			write_log(_T("Fatal error: out of memory for chipmem.\n"));
			chipmem_bank.reserved_size = 0;
		}
		else {
			if (memsize > chipmem_bank.allocated_size)
				memset(chipmem_bank.baseaddr + chipmem_bank.allocated_size, 0xff, memsize - chipmem_bank.allocated_size);
		}
		currprefs.chipset_mask = changed_prefs.chipset_mask;
		chipmem_full_mask = chipmem_bank.allocated_size - 1;
		if (!currprefs.cachesize) {
			if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
				if (chipmem_bank.allocated_size < 0x100000)
					chipmem_full_mask = 0x100000 - 1;
				if (chipmem_bank.allocated_size > 0x100000 && chipmem_bank.allocated_size < 0x200000)
					chipmem_full_mask = chipmem_bank.mask = 0x200000 - 1;
			}
		}
	}

	if (bogomem_bank.reserved_size != currprefs.bogomem_size) {
		mapped_free(&bogomem_bank);
		bogomem_bank.reserved_size = 0;

		if (currprefs.bogomem_size > 0x1c0000)
			currprefs.bogomem_size = 0x1c0000;
		if (currprefs.bogomem_size > 0x180000 && ((currprefs.chipset_mask & CSMASK_AGA) || (currprefs.cpu_model >= 68020)))
			currprefs.bogomem_size = 0x180000;

		bogomem_bank.reserved_size = currprefs.bogomem_size;
		if (bogomem_bank.reserved_size >= 0x180000)
			bogomem_bank.reserved_size = 0x200000;
		bogomem_bank.mask = bogomem_bank.reserved_size - 1;
		bogomem_bank.start = bogomem_start_addr;

		if (bogomem_bank.reserved_size) {
			if (!mapped_malloc(&bogomem_bank)) {
				write_log(_T("Out of memory for bogomem.\n"));
				bogomem_bank.reserved_size = 0;
			}
		}
	}
	if (a3000lmem_bank.reserved_size != currprefs.mbresmem_low_size) {
		mapped_free(&a3000lmem_bank);

		a3000lmem_bank.reserved_size = currprefs.mbresmem_low_size;
		a3000lmem_bank.mask = a3000lmem_bank.reserved_size - 1;
		a3000lmem_bank.start = 0x08000000 - a3000lmem_bank.reserved_size;
		if (a3000lmem_bank.reserved_size) {
			if (!mapped_malloc(&a3000lmem_bank)) {
				write_log(_T("Out of memory for a3000lowmem.\n"));
				a3000lmem_bank.reserved_size = 0;
			}
		}
	}
	if (a3000hmem_bank.reserved_size != currprefs.mbresmem_high_size) {
		mapped_free(&a3000hmem_bank);

		a3000hmem_bank.reserved_size = currprefs.mbresmem_high_size;
		a3000hmem_bank.mask = a3000hmem_bank.reserved_size - 1;
		a3000hmem_bank.start = 0x08000000;
		if (a3000hmem_bank.reserved_size) {
			if (!mapped_malloc(&a3000hmem_bank)) {
				write_log(_T("Out of memory for a3000highmem.\n"));
				a3000hmem_bank.reserved_size = 0;
			}
		}
	}

	allocate_memory_custombanks();

	if (savestate_state == STATE_RESTORE) {
		if (bootrom_filepos) {
			if (currprefs.uaeboard < 0)
				currprefs.uaeboard = 0;
			protect_roms(false);
			restore_ram(bootrom_filepos, rtarea_bank.baseaddr);
			protect_roms(true);
		}
		restore_ram(chip_filepos, chipmem_bank.baseaddr);
		if (bogomem_bank.allocated_size > 0)
			restore_ram(bogo_filepos, bogomem_bank.baseaddr);
		if (a3000lmem_bank.allocated_size > 0)
			restore_ram(a3000lmem_filepos, a3000lmem_bank.baseaddr);
		if (a3000hmem_bank.allocated_size > 0)
			restore_ram(a3000hmem_filepos, a3000hmem_bank.baseaddr);
	}
	bootrom_filepos = 0;
	chip_filepos = 0;
	bogo_filepos = 0;
	a3000lmem_filepos = 0;
	a3000hmem_filepos = 0;
}

void map_overlay(int chip)
{
	int size;
	addrbank *cb;
	int currPC = m68k_getpc();

	size = chipmem_bank.allocated_size >= 0x180000 ? (chipmem_bank.allocated_size >> 16) : 32;
	cb = &chipmem_bank;
  if (chip) {
		map_banks(&dummy_bank, 0, size, 0);
		map_banks(cb, 0, size, chipmem_bank.allocated_size);
  } else {
		addrbank *rb = NULL;
		if (size < 32)
			size = 32;
		cb = &get_mem_bank(0xf00000);
		if (!rb && cb && (cb->flags & ABFLAG_ROM) && get_word(0xf00000) == 0x1114)
			rb = cb;
		cb = &get_mem_bank(0xe00000);
		if (!rb && cb && (cb->flags & ABFLAG_ROM) && get_word(0xe00000) == 0x1114)
			rb = cb;
		if (!rb)
			rb = &kickmem_bank;
		map_banks(rb, 0, size, 0x80000);
	}
	if (!isrestore() && valid_address(regs.pc, 4))
		m68k_setpc_normal(currPC);
}

void memory_clear(void)
{
	mem_hardreset = 0;
	if (savestate_state == STATE_RESTORE)
		return;
	if (chipmem_bank.baseaddr)
		memset(chipmem_bank.baseaddr, 0, chipmem_bank.allocated_size);
	if (bogomem_bank.baseaddr)
		memset(bogomem_bank.baseaddr, 0, bogomem_bank.allocated_size);
	if (a3000lmem_bank.baseaddr)
		memset(a3000lmem_bank.baseaddr, 0, a3000lmem_bank.allocated_size);
	if (a3000hmem_bank.baseaddr)
		memset(a3000hmem_bank.baseaddr, 0, a3000hmem_bank.allocated_size);
	expansion_clear();
}

static void restore_roms(void)
{
	protect_roms(false);
	write_log(_T("ROM loader.. (%s)\n"), currprefs.romfile);
	kickstart_rom = 1;

	memcpy(currprefs.romfile, changed_prefs.romfile, sizeof currprefs.romfile);
	memcpy(currprefs.romextfile, changed_prefs.romextfile, sizeof currprefs.romextfile);
	mapped_free(&extendedkickmem_bank);
	mapped_free(&extendedkickmem2_bank);
	extendedkickmem_bank.reserved_size = 0;
	extendedkickmem2_bank.reserved_size = 0;
	extendedkickmem_type = 0;
	load_extendedkickstart(currprefs.romextfile, 0);
	kickmem_bank.mask = ROM_SIZE_512 - 1;
	if (!load_kickstart())
	{
		if (_tcslen(currprefs.romfile) > 0)
		{
			error_log(_T("Failed to open '%s'\n"), currprefs.romfile);
			notify_user(NUMSG_NOROM);
		}
		load_kickstart_replacement();
	}
	else
	{
		struct romdata *rd = getromdatabydata(kickmem_bank.baseaddr, kickmem_bank.reserved_size);
		if (rd)
		{
			write_log(_T("Known ROM '%s' loaded\n"), rd->name);
			if ((rd->cpu & 8) && changed_prefs.cpu_model < 68030)
			{
				notify_user(NUMSG_KS68030PLUS);
				uae_restart(-1, NULL);
			}
			else if ((rd->cpu & 3) == 3 && changed_prefs.cpu_model != 68030)
			{
				notify_user(NUMSG_KS68030);
				uae_restart(-1, NULL);
			}
			else if ((rd->cpu & 3) == 1 && changed_prefs.cpu_model < 68020)
			{
				notify_user(NUMSG_KS68EC020);
				uae_restart(-1, NULL);
			}
			else if ((rd->cpu & 3) == 2 && (changed_prefs.cpu_model < 68020 || changed_prefs.address_space_24))
			{
				notify_user(NUMSG_KS68020);
				uae_restart(-1, NULL);
			}
			if (rd->cloanto)
				cloanto_rom = 1;
			kickstart_rom = 0;
			if ((rd->type & (ROMTYPE_SPECIALKICK | ROMTYPE_KICK)) == ROMTYPE_KICK)
				kickstart_rom = 1;
			if ((rd->cpu & 4) && currprefs.cs_compatible)
			{
				/* A4000 ROM = need ramsey, gary and ide */
				if (currprefs.cs_ramseyrev < 0)
					changed_prefs.cs_ramseyrev = currprefs.cs_ramseyrev = 0x0f;
				changed_prefs.cs_fatgaryrev = currprefs.cs_fatgaryrev = 0;
				if (currprefs.cs_ide != IDE_A4000)
					changed_prefs.cs_ide = currprefs.cs_ide = -1;
			}
		}
		else
		{
			write_log(_T("Unknown ROM '%s' loaded\n"), currprefs.romfile);
		}
	}
	patch_kick();
	write_log(_T("ROM loader end\n"));
	protect_roms(true);
}

bool read_kickstart_version(struct uae_prefs *p)
{
	kickstart_version = 0;
	cloanto_rom = 0;
	struct zfile *z = get_kickstart_filehandle(p);
	if (!z)
		return false;
	uae_u8 mem[32] = {0};
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

static void memory_init (void);

void memory_reset(void)
{
	int bnk, bnk_end;
	bool gayleorfatgary;

	/* Use changed_prefs, as m68k_reset is called later.  */
	last_address_space_24 = changed_prefs.address_space_24;

	if (mem_hardreset > 2)
		memory_init();

	be_cnt = be_recursive = 0;
	currprefs.chipmem_size = changed_prefs.chipmem_size;
	currprefs.bogomem_size = changed_prefs.bogomem_size;
	currprefs.mbresmem_low_size = changed_prefs.mbresmem_low_size;
	currprefs.mbresmem_high_size = changed_prefs.mbresmem_high_size;
	currprefs.cs_ksmirror_e0 = changed_prefs.cs_ksmirror_e0;
	currprefs.cs_ksmirror_a8 = changed_prefs.cs_ksmirror_a8;
	currprefs.cs_ciaoverlay = changed_prefs.cs_ciaoverlay;
	currprefs.cs_ide = changed_prefs.cs_ide;
	currprefs.cs_fatgaryrev = changed_prefs.cs_fatgaryrev;
	currprefs.cs_ramseyrev = changed_prefs.cs_ramseyrev;
	currprefs.cs_unmapped_space = changed_prefs.cs_unmapped_space;

	gayleorfatgary = ((currprefs.chipset_mask & CSMASK_AGA) || currprefs.cs_pcmcia || currprefs.cs_ide > 0) && !currprefs.cs_cd32cd;

	init_mem_banks();
	allocate_memory();

	if (mem_hardreset > 1
		|| _tcscmp (currprefs.romfile, changed_prefs.romfile) != 0
  	|| _tcscmp (currprefs.romextfile, changed_prefs.romextfile) != 0)
	{
		restore_roms();
	}

	map_banks(&custom_bank, 0xC0, 0xE0 - 0xC0, 0);
	map_banks(&cia_bank, 0xA0, 32, 0);
	if (currprefs.cs_rtc != 3)
		/* D80000 - DDFFFF not mapped (A1000 or A2000 = custom chips) */
		map_banks(&dummy_bank, 0xD8, 6, 0);

	/* map "nothing" to 0x200000 - 0x9FFFFF (0xBEFFFF if Gayle or Fat Gary) */
	bnk = chipmem_bank.allocated_size >> 16;
	if (bnk < 0x20 + (currprefs.fastmem[0].size >> 16))
		bnk = 0x20 + (currprefs.fastmem[0].size >> 16);
	bnk_end = currprefs.cs_cd32cd ? 0xBE : (gayleorfatgary ? 0xBF : 0xA0);
	map_banks(&dummy_bank, bnk, bnk_end - bnk, 0);
  if (gayleorfatgary) {
		// a4000 = custom chips from 0xc0 to 0xd0
		if (currprefs.cs_ide == IDE_A4000)
			map_banks(&dummy_bank, 0xd0, 8, 0);
		else
			map_banks(&dummy_bank, 0xc0, 0xd8 - 0xc0, 0);
	} else if (currprefs.cs_cd32cd) {
		// CD32: 0xc0 to 0xd0
		map_banks(&dummy_bank, 0xd0, 8, 0);
		// strange 64k custom mirror
		map_banks(&custom_bank, 0xb9, 1, 0);
	}

  if (bogomem_bank.baseaddr) {
		int t = currprefs.bogomem_size >> 16;
		if (t > 0x1C)
			t = 0x1C;
		if (t > 0x18 && ((currprefs.chipset_mask & CSMASK_AGA) || (currprefs.cpu_model >= 68020 && !currprefs.address_space_24)))
			t = 0x18;
		map_banks(&bogomem_bank, 0xC0, t, 0);
	}
	if (currprefs.cs_ide || currprefs.cs_pcmcia)
	{
		if (currprefs.cs_ide == IDE_A600A1200 || currprefs.cs_pcmcia)
		{
			map_banks(&gayle_bank, 0xD8, 6, 0);
			map_banks(&gayle2_bank, 0xDD, 2, 0);
		}
		if (currprefs.cs_ide == IDE_A4000)
			map_banks(&gayle_bank, 0xDD, 1, 0);
		if (currprefs.cs_ide < 0 && !currprefs.cs_pcmcia)
			map_banks(&gayle_bank, 0xD8, 6, 0);
		if (currprefs.cs_ide < 0)
			map_banks(&gayle_bank, 0xDD, 1, 0);
	}
	if (currprefs.cs_rtc == 3) // A2000 clock
		map_banks(&clock_bank, 0xD8, 4, 0);
	if (currprefs.cs_rtc == 1 || currprefs.cs_rtc == 2)
		map_banks(&clock_bank, 0xDC, 1, 0);
	else if (currprefs.cs_ksmirror_a8 || currprefs.cs_ide > 0 || currprefs.cs_pcmcia)
		map_banks(&clock_bank, 0xDC, 1, 0); /* none clock */
	if (currprefs.cs_fatgaryrev >= 0 || currprefs.cs_ramseyrev >= 0)
		map_banks(&mbres_bank, 0xDE, 1, 0);
#ifdef CD32
	if (currprefs.cs_cd32c2p || currprefs.cs_cd32cd || currprefs.cs_cd32nvram) {
		map_banks(&akiko_bank, AKIKO_BASE >> 16, 1, 0);
		map_banks(&gayle2_bank, 0xDD, 2, 0);
	}
#endif
	if (a3000lmem_bank.baseaddr)
		map_banks(&a3000lmem_bank, a3000lmem_bank.start >> 16, a3000lmem_bank.allocated_size >> 16, 0);
	if (a3000hmem_bank.baseaddr)
		map_banks(&a3000hmem_bank, a3000hmem_bank.start >> 16, a3000hmem_bank.allocated_size >> 16, 0);
	map_banks_set(&kickmem_bank, 0xF8, 8, 0);
	/* map beta Kickstarts at 0x200000/0xC00000/0xF00000 */
  if (kickmem_bank.baseaddr[0] == 0x11 && kickmem_bank.baseaddr[2] == 0x4e && kickmem_bank.baseaddr[3] == 0xf9 && kickmem_bank.baseaddr[4] == 0x00) {
		uae_u32 addr = kickmem_bank.baseaddr[5];
		if (addr == 0x20 && currprefs.chipmem_size <= 0x200000 && currprefs.fastmem[0].size == 0)
			map_banks_set(&kickmem_bank, addr, 8, 0);
		if (addr == 0xC0 && currprefs.bogomem_size == 0)
			map_banks_set(&kickmem_bank, addr, 8, 0);
		if (addr == 0xF0)
			map_banks_set(&kickmem_bank, addr, 8, 0);
	}

#ifdef AUTOCONFIG
	expansion_map();
#endif

	/* Map the chipmem into all of the lower 8MB */
	map_overlay(1);

  switch (extendedkickmem_type) {
	case EXTENDED_ROM_KS:
		map_banks_set(&extendedkickmem_bank, 0xE0, 8, 0);
		break;
#ifdef CD32
	case EXTENDED_ROM_CD32:
		map_banks_set(&extendedkickmem_bank, 0xE0, 8, 0);
		break;
#endif
	}

#ifdef AUTOCONFIG
	if ((need_uae_boot_rom(&currprefs) && currprefs.uaeboard == 0) || currprefs.uaeboard == 1)
		map_banks_set(&rtarea_bank, rtarea_base >> 16, 1, 0);
#endif

	if ((cloanto_rom || currprefs.cs_ksmirror_e0) && !extendedkickmem_type) {
		map_banks(&kickmem_bank, 0xE0, 8, 0);
	}
	if (currprefs.cs_ksmirror_a8) {
		if (extendedkickmem2_bank.allocated_size) {
			map_banks_set(&extendedkickmem2_bank, extendedkickmem2_bank.start >> 16, extendedkickmem2_bank.allocated_size >> 16, 0);
		} else {
			struct romdata *rd = getromdatabypath(currprefs.cartfile);
		  if (!rd || rd->id != 63) {
				if (extendedkickmem_type == EXTENDED_ROM_CD32 || extendedkickmem_type == EXTENDED_ROM_KS)
					map_banks(&extendedkickmem_bank, 0xb0, 8, 0);
				else
					map_banks(&kickmem_bank, 0xb0, 8, 0);
				map_banks(&kickmem_bank, 0xa8, 8, 0);
			}
		}
	} else if (extendedkickmem2_bank.allocated_size && extendedkickmem2_bank.baseaddr) {
		map_banks_set(&extendedkickmem2_bank, extendedkickmem2_bank.start >> 16, extendedkickmem2_bank.allocated_size >> 16, 0);
	}

#ifdef ACTION_REPLAY
	action_replay_memory_reset();
#endif

	for (int i = 0; i < 2; i++) {
		if (currprefs.custom_memory_sizes[i]) {
			map_banks(i == 0 ? &custmem1_bank : &custmem2_bank,
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
		memory_clear();
	}
	write_log (_T("memory init end\n"));
}

static void memory_init (void)
{
	init_mem_banks();
	virtualdevice_init();

	chipmem_bank.reserved_size = 0;
	bogomem_bank.reserved_size = 0;
	kickmem_bank.baseaddr = NULL;
	extendedkickmem_bank.baseaddr = NULL;
	extendedkickmem_bank.reserved_size = 0;
	extendedkickmem2_bank.baseaddr = NULL;
	extendedkickmem2_bank.reserved_size = 0;
	extendedkickmem_type = 0;
	chipmem_bank.baseaddr = 0;
	a3000lmem_bank.reserved_size = a3000hmem_bank.reserved_size = 0;
	a3000lmem_bank.baseaddr = a3000hmem_bank.baseaddr = NULL;
	bogomem_bank.baseaddr = NULL;
	custmem1_bank.reserved_size = custmem2_bank.reserved_size = 0;
	custmem1_bank.baseaddr = NULL;
	custmem2_bank.baseaddr = NULL;

	kickmem_bank.reserved_size = ROM_SIZE_512;
	mapped_malloc(&kickmem_bank);
	memset(kickmem_bank.baseaddr, 0, ROM_SIZE_512);
	_tcscpy(currprefs.romfile, _T("<none>"));
	currprefs.romextfile[0] = 0;

#ifdef ACTION_REPLAY
	action_replay_unload(0);
	action_replay_load();
	action_replay_init(1);
#ifdef ACTION_REPLAY_HRTMON
	hrtmon_load();
#endif
#endif
}

void memory_cleanup(void)
{
	mapped_free(&a3000lmem_bank);
	mapped_free(&a3000hmem_bank);
	mapped_free(&bogomem_bank);
	mapped_free(&kickmem_bank);
	mapped_free(&chipmem_bank);
	mapped_free(&custmem1_bank);
	mapped_free(&custmem2_bank);

	bogomem_bank.baseaddr = NULL;
	kickmem_bank.baseaddr = NULL;
	a3000lmem_bank.baseaddr = a3000hmem_bank.baseaddr = NULL;
	chipmem_bank.baseaddr = NULL;
	custmem1_bank.baseaddr = NULL;
	custmem2_bank.baseaddr = NULL;

#ifdef ACTION_REPLAY
	action_replay_cleanup();
#endif
}

void memory_hardreset (int mode)
{
	if (mode + 1 > mem_hardreset)
		mem_hardreset = mode + 1;
}

// do not map if it conflicts with custom banks
void map_banks_cond(addrbank *bank, int start, int size, int realsize)
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
	map_banks(bank, start, size, realsize);
}

static void map_banks2 (addrbank *bank, int start, int size, int realsize)
{
	int bnr;
	unsigned long int hioffs = 0, endhioffs = 0x100;
	uae_u32 realstart = start;

	flush_icache_hard(3); /* Sure don't want to keep any old mappings around! */

	if (!realsize)
		realsize = size << 16;

  if ((size << 16) < realsize) {
		write_log(_T("Broken mapping, size=%x, realsize=%x\nStart is %x\n"),
				  size, realsize, start);
	}

#ifndef ADDRESS_SPACE_24BIT
  if (start >= 0x100) {
    for (bnr = start; bnr < start + size; bnr++) {
			mem_banks[bnr] = bank;
		}
		return;
	}
#endif
	if (last_address_space_24)
		endhioffs = 0x10000;
#ifdef ADDRESS_SPACE_24BIT
	endhioffs = 0x100;
#endif
  for (hioffs = 0; hioffs < endhioffs; hioffs += 0x100) {
    for (bnr = start; bnr < start + size; bnr++) {
			mem_banks[bnr + hioffs] = bank;
		}
	}
}

void map_banks(addrbank *bank, int start, int size, int realsize)
{
	if (start == 0xffffffff)
		return;
	if (start >= 0x100) {
		if (currprefs.address_space_24)
			return;
	}
	map_banks2 (bank, start, size, realsize);
}

static bool validate_banks_z3(addrbank *bank, int start, int size)
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
		if (ab != &dummy_bank) {
			error_log(_T("Z2 map_banks(%s) attempting to override existing memory bank '%s' at %08x!\n"), bank->name, ab->name, i << 16);
			return false;
		}
	}
	return true;
}


void map_banks_z2(addrbank *bank, int start, int size)
{
	if (!validate_banks_z2(bank, start, size))
		return;
	map_banks(bank, start, size, 0);
}

#ifdef SAVESTATE

/* memory save/restore code */

uae_u8 *save_bootrom(int *len)
{
	if (!uae_boot_rom_type)
		return 0;
	*len = uae_boot_rom_size;
	return rtarea_bank.baseaddr;
}

uae_u8 *save_cram(int *len)
{
	*len = chipmem_bank.allocated_size;
	return chipmem_bank.baseaddr;
}

uae_u8 *save_bram(int *len)
{
	*len = bogomem_bank.allocated_size;
	return bogomem_bank.baseaddr;
}

uae_u8 *save_a3000lram(int *len)
{
	*len = a3000lmem_bank.allocated_size;
	return a3000lmem_bank.baseaddr;
}

uae_u8 *save_a3000hram(int *len)
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
	changed_prefs.chipmem_size = len;
}

void restore_bram(int len, size_t filepos)
{
	bogo_filepos = filepos;
	changed_prefs.bogomem_size = len;
}

void restore_a3000lram(int len, size_t filepos)
{
	a3000lmem_filepos = filepos;
	changed_prefs.mbresmem_low_size = len;
}

void restore_a3000hram(int len, size_t filepos)
{
	a3000hmem_filepos = filepos;
	changed_prefs.mbresmem_high_size = len;
}

uae_u8 *restore_rom(uae_u8 *src)
{
	uae_u32 crc32, mem_start, mem_size, mem_type, version;
	TCHAR *s, *romn;
	int i, crcdet;
	struct romlist *rl = romlist_getit();

	mem_start = restore_u32();
	mem_size = restore_u32();
	mem_type = restore_u32();
	version = restore_u32();
	crc32 = restore_u32();
	romn = restore_string();
	crcdet = 0;
  for (i = 0; i < romlist_count (); i++) {
    if (rl[i].rd->crc32 == crc32 && crc32) {
      if (zfile_exists (rl[i].path)) {
				switch (mem_type)
				{
				case 0:
					_tcsncpy(changed_prefs.romfile, rl[i].path, 255);
					break;
				case 1:
					_tcsncpy(changed_prefs.romextfile, rl[i].path, 255);
					break;
				}
				write_log(_T("ROM '%s' = '%s'\n"), romn, rl[i].path);
				crcdet = 1;
      } else {
				write_log(_T("ROM '%s' = '%s' invalid rom scanner path!"), romn, rl[i].path);
			}
			break;
		}
	}
	s = restore_string();
  if (!crcdet) {
    if(zfile_exists (s)) {
			switch (mem_type)
			{
			case 0:
				_tcsncpy(changed_prefs.romfile, s, 255);
				break;
			case 1:
				_tcsncpy(changed_prefs.romextfile, s, 255);
				break;
			}
			write_log(_T("ROM detected (path) as '%s'\n"), s);
			crcdet = 1;
		}
	}
	xfree(s);
	if (!crcdet)
		write_log(_T("WARNING: ROM '%s' %d.%d (CRC32=%08x %08x-%08x) not found!\n"),
				  romn, version >> 16, version & 0xffff, crc32, mem_start, mem_start + mem_size - 1);
	xfree(romn);
	return src;
}

uae_u8 *save_rom(int first, int *len, uae_u8 *dstptr)
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
    		if (get_long (i + mem_start) != get_long (i + mem_start + mem_size / 2))
					break;
			}
	    if (i == mem_size / 2 - 4) {
				mem_size /= 2;
				mem_start += ROM_SIZE_256;
			}
	    version = get_long (mem_start + 12); /* version+revision */
			_stprintf (tmpname, _T("Kickstart %d.%d"), get_word (mem_start + 12), get_word (mem_start + 14));
			break;
		case 1: /* Extended ROM */
			if (!extendedkickmem_type)
				break;
			mem_start = extendedkickmem_bank.start;
			mem_real_start = extendedkickmem_bank.baseaddr;
			mem_size = extendedkickmem_bank.allocated_size;
			path = currprefs.romextfile;
			version = get_long (mem_start + 12); /* version+revision */
			if (version == 0xffffffff)
				version = get_long (mem_start + 16);
			_stprintf(tmpname, _T("Extended"));
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
		dstbak = dst = xmalloc(uae_u8, 4 + 4 + 4 + 4 + 4 + 256 + 256 + mem_size);
	save_u32(mem_start);
	save_u32(mem_size);
	save_u32(mem_type);
	save_u32(version);
	save_u32(get_crc32(mem_real_start, mem_size));
	save_string(tmpname);
	save_string(path);
  if (saverom) {
		for (i = 0; i < mem_size; i++)
      *dst++ = get_byte (mem_start + i);
	}
	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */

/* memory helpers */

void memcpyha_safe(uaecptr dst, const uae_u8 *src, int size)
{
	if (!addr_valid(_T("memcpyha"), dst, size))
		return;
	while (size--)
		put_byte(dst++, *src++);
}
void memcpyha(uaecptr dst, const uae_u8 *src, int size)
{
	while (size--)
		put_byte(dst++, *src++);
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
uaecptr strcpyha_safe(uaecptr dst, const uae_char *src)
{
	uaecptr res = dst;
	uae_u8 b;
  do {
		if (!addr_valid(_T("_tcscpyha"), dst, 1))
			return res;
		b = *src++;
		put_byte(dst++, b);
	} while (b);
	return res;
}
