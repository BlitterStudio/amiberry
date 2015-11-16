 /* 
  * UAE - The Un*x Amiga Emulator
  *
  * AutoConfig (tm) Expansions (ZorroII/III)
  *
  * Copyright 1996,1997 Stefan Reinauer <stepan@linux.de>
  * Copyright 1997 Brian King <Brian_King@Mitel.com>
  *   - added gfxcard code
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "autoconf.h"
#include "custom.h"
#include "newcpu.h"
#include "savestate.h"
#include "zfile.h"
#include <SDL.h>

#define MAX_EXPANSION_BOARDS	8

/* ********************************************************** */
/* 00 / 02 */
/* er_Type */

#define Z2_MEM_8MB	0x00 /* Size of Memory Block */
#define Z2_MEM_4MB	0x07
#define Z2_MEM_2MB	0x06
#define Z2_MEM_1MB	0x05
#define Z2_MEM_512KB	0x04
#define Z2_MEM_256KB	0x03
#define Z2_MEM_128KB	0x02
#define Z2_MEM_64KB	0x01
/* extended definitions */
#define Z2_MEM_16MB	0x00
#define Z2_MEM_32MB	0x01
#define Z2_MEM_64MB	0x02
#define Z2_MEM_128MB	0x03
#define Z2_MEM_256MB	0x04
#define Z2_MEM_512MB	0x05
#define Z2_MEM_1GB	0x06

#define chainedconfig	0x08 /* Next config is part of the same card */
#define rom_card	0x10 /* ROM vector is valid */
#define add_memory	0x20 /* Link RAM into free memory list */

#define zorroII		0xc0 /* Type of Expansion Card */
#define zorroIII	0x80

/* ********************************************************** */
/* 04 - 06 & 10-16 */

/* Manufacturer */
#define commodore_g	 513 /* Commodore Braunschweig (Germany) */
#define commodore	 514 /* Commodore West Chester */
#define gvp		2017 /* GVP */
#define ass		2102 /* Advanced Systems & Software */
#define hackers_id	2011 /* Special ID for test cards */

/* Card Type */
#define commodore_a2091	     3 /* A2091 / A590 Card from C= */
#define commodore_a2091_ram 10 /* A2091 / A590 Ram on HD-Card */
#define commodore_a2232	    70 /* A2232 Multiport Expansion */
#define ass_nexus_scsi	     1 /* Nexus SCSI Controller */

#define gvp_series_2_scsi   11
#define gvp_iv_24_gfx	    32

/* ********************************************************** */
/* 08 - 0A  */
/* er_Flags */
#define Z3_MEM_64KB	0x02
#define Z3_MEM_128KB	0x03
#define Z3_MEM_256KB	0x04
#define Z3_MEM_512KB	0x05
#define Z3_MEM_1MB	0x06 /* Zorro III card subsize */
#define Z3_MEM_2MB	0x07
#define Z3_MEM_4MB	0x08
#define Z3_MEM_6MB	0x09
#define Z3_MEM_8MB	0x0a
#define Z3_MEM_10MB	0x0b
#define Z3_MEM_12MB	0x0c
#define Z3_MEM_14MB	0x0d
#define Z3_MEM_16MB	0x00
#define Z3_MEM_AUTO	0x01
#define Z3_MEM_defunct1	0x0e
#define Z3_MEM_defunct2	0x0f

#define force_z3	0x10 /* *MUST* be set if card is Z3 */
#define ext_size	0x20 /* Use extended size table for bits 0-2 of er_Type */
#define no_shutup	0x40 /* Card cannot receive Shut_up_forever */
#define care_addr	0x80 /* Z2=Adress HAS to be $200000-$9fffff Z3=1->mem,0=io */

/* ********************************************************** */
/* 40-42 */
/* ec_interrupt (unused) */

#define enable_irq	0x01 /* enable Interrupt */
#define reset_card	0x04 /* Reset of Expansion Card - must be 0 */
#define card_int2	0x10 /* READ ONLY: IRQ 2 active */
#define card_irq6	0x20 /* READ ONLY: IRQ 6 active */
#define card_irq7	0x40 /* READ ONLY: IRQ 7 active */
#define does_irq	0x80 /* READ ONLY: Card currently throws IRQ */

/* ********************************************************** */

/* ROM defines (DiagVec) */

#define rom_4bit	(0x00<<14) /* ROM width */
#define rom_8bit	(0x01<<14)
#define rom_16bit	(0x02<<14)

#define rom_never	(0x00<<12) /* Never run Boot Code */
#define rom_install	(0x01<<12) /* run code at install time */
#define rom_binddrv	(0x02<<12) /* run code with binddrivers */

uaecptr ROM_filesys_resname, ROM_filesys_resid;
uaecptr ROM_filesys_diagentry;
uaecptr ROM_hardfile_resname, ROM_hardfile_resid;
uaecptr ROM_hardfile_init;
int uae_boot_rom, uae_boot_rom_size; /* size = code size only */

/* ********************************************************** */

static void (*card_init[MAX_EXPANSION_BOARDS]) (void);
static void (*card_map[MAX_EXPANSION_BOARDS]) (void);

static int ecard, cardno;

static uae_u16 uae_id;

/* ********************************************************** */

/* Please note: ZorroIII implementation seems to work different
 * than described in the HRM. This claims that ZorroIII config
 * address is 0xff000000 while the ZorroII config space starts
 * at 0x00e80000. In reality, both, Z2 and Z3 cards are 
 * configured in the ZorroII config space. Kickstart 3.1 doesn't
 * even do a single read or write access to the ZorroIII space.
 * The original Amiga include files tell the same as the HRM.
 * ZorroIII: If you set ext_size in er_Flags and give a Z2-size
 * in er_Type you can very likely add some ZorroII address space
 * to a ZorroIII card on a real Amiga. This is not implemented
 * yet.
 *  -- Stefan
 * 
 * Surprising that 0xFF000000 isn't used. Maybe it depends on the
 * ROM. Anyway, the HRM says that Z3 cards may appear in Z2 config
 * space, so what we are doing here is correct.
 *  -- Bernd
 */

/* Autoconfig address space at 0xE80000 */
static uae_u8 expamem[65536];

static uae_u8 expamem_lo;
static uae_u16 expamem_hi;

static uae_u32 REGPARAM3 expamem_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 expamem_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 expamem_bget (uaecptr) REGPARAM;
static void REGPARAM3 expamem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 expamem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 expamem_bput (uaecptr, uae_u32) REGPARAM;

addrbank expamem_bank = {
    expamem_lget, expamem_wget, expamem_bget,
    expamem_lput, expamem_wput, expamem_bput,
    default_xlate, default_check, NULL, "Autoconfig",
    dummy_lgeti, dummy_wgeti, ABFLAG_IO | ABFLAG_SAFE
};

static void expamem_map_clear (void)
{
    write_log ("expamem_map_clear() got called. Shouldn't happen.\n");
}

static void expamem_init_clear (void)
{
    memset (expamem, 0xff, sizeof expamem);
}
/* autoconfig area is "non-existing" after last device */
static void expamem_init_clear_zero (void)
{
    map_banks (&dummy_bank, 0xe8, 1, 0);
}

static void expamem_init_clear2 (void)
{
    expamem_init_clear_zero();
    ecard = cardno;
}

static void expamem_init_last (void)
{
    expamem_init_clear2();
}

static uae_u32 REGPARAM2 expamem_lget (uaecptr addr)
{
    write_log ("warning: READ.L from address $%lx PC=%x\n", addr, M68K_GETPC);
    return (expamem_wget (addr) << 16) | expamem_wget (addr + 2);
}

static uae_u32 REGPARAM2 expamem_wget (uaecptr addr)
{
    uae_u32 v = (expamem_bget (addr) << 8) | expamem_bget (addr + 1);
    write_log ("warning: READ.W from address $%lx=%04x PC=%x\n", addr, v & 0xffff, M68K_GETPC);
    return v;
}

static uae_u32 REGPARAM2 expamem_bget (uaecptr addr)
{
    uae_u8 b;
#ifdef JIT
    special_mem |= S_READ;
#endif
    addr &= 0xFFFF;
    b = expamem[addr];
    //write_log("%08x=%02X\n", addr, b);
    return b;
}

static void REGPARAM2 expamem_write (uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    addr &= 0xffff;
    if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
    	do_put_mem_byte ((uae_u8 *)(expamem + addr), (value & 0xf0));
    	do_put_mem_byte ((uae_u8 *)(expamem + addr + 2), (value & 0x0f) << 4);
    } else {
    	do_put_mem_byte ((uae_u8 *)(expamem + addr), ~(value & 0xf0));
    	do_put_mem_byte ((uae_u8 *)(expamem + addr + 2), ~(value & 0x0f) << 4);
    }
}

static int REGPARAM2 expamem_type (void)
{
    return ((do_get_mem_byte(expamem + 0) | do_get_mem_byte(expamem + 2) >> 4) & 0xc0);
}

static void REGPARAM2 expamem_lput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log ("warning: WRITE.L to address $%lx : value $%lx\n", addr, value);
}

static void REGPARAM2 expamem_wput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    value &= 0xffff;
    if (ecard >= cardno)
	return;
    if (expamem_type() != zorroIII)
	write_log ("warning: WRITE.W to address $%lx : value $%x\n", addr, value);
    else {
	switch (addr & 0xff) {
	 case 0x44:
	    if (expamem_type() == zorroIII) {
		uae_u32 p1, p2;
		// +Bernd Roesch & Toni Wilen
		p1 = get_word (regs.regs[11] + 0x20);
		if (expamem[0] & add_memory) {
		  // Z3 RAM expansion
  		p2 = z3fastmem_start >> 16;
		} else {
		    // Z3 P96 RAM
#ifdef PICASSO96
		    p2 = p96ram_start >> 16;
#endif
		}
		put_word (regs.regs[11] + 0x20, p2);
		put_word (regs.regs[11] + 0x28, p2);
		// -Bernd Roesch
		expamem_hi = p2;
		(*card_map[ecard]) ();
		ecard++;
		if (p1 != p2)
		    write_log ("   Card %d remapped %04x0000 -> %04x0000\n", ecard, p1, p2);
		write_log ("   Card %d (Zorro%s) done.\n", ecard, expamem_type() == 0xc0 ? "II" : "III");
		if (ecard < cardno)
		    (*card_init[ecard]) ();
		else
		    expamem_init_clear2 ();
	    }
	    break;
	}
    }
}

static void REGPARAM2 expamem_bput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    if (ecard >= cardno)
	return;
    value &= 0xff;
	switch (addr & 0xff) {
		case 0x30:
		case 0x32:
			expamem_hi = 0;
			expamem_lo = 0;
			expamem_write (0x48, 0x00);
			break;

		case 0x48:
			if (expamem_type () == zorroII) {
				expamem_hi = value;
				(*card_map[ecard]) ();
				write_log ("   Card %d (Zorro%s) done.\n", ecard + 1, expamem_type() == 0xc0 ? "II" : "III");
				++ecard;
				if (ecard < cardno)
					(*card_init[ecard]) ();
				else
					expamem_init_clear2 ();
			} else if (expamem_type() == zorroIII)
				expamem_lo = value;
			break;

		case 0x4a:
			if (expamem_type () == zorroII)
				expamem_lo = value;
			break;

		case 0x4c:
			write_log ("   Card %d (Zorro %s) had no success.\n", ecard + 1, expamem_type() == 0xc0 ? "II" : "III");
			++ecard;
			if (ecard < cardno)
				(*card_init[ecard]) ();
			else
				expamem_init_clear2 ();
			break;
	}
}

/* ********************************************************** */

/*
 *  Fast Memory
 */

static uae_u32 fastmem_mask;

static uae_u32 REGPARAM3 fastmem_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 fastmem_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 fastmem_bget (uaecptr) REGPARAM;
static void REGPARAM3 fastmem_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 fastmem_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 fastmem_bput (uaecptr, uae_u32) REGPARAM;
static int REGPARAM3 fastmem_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 fastmem_xlate (uaecptr addr) REGPARAM;

uaecptr fastmem_start; /* Determined by the OS */
static uae_u8 *fastmemory;

static uae_u32 REGPARAM2 fastmem_lget (uaecptr addr)
{
    uae_u8 *m;
    addr -= fastmem_start;
    addr &= fastmem_mask;
    m = fastmemory + addr;
    return do_get_mem_long ((uae_u32 *)m);
}

static uae_u32 REGPARAM2 fastmem_wget (uaecptr addr)
{
    uae_u8 *m;
    addr -= fastmem_start;
    addr &= fastmem_mask;
    m = fastmemory + addr;
    return do_get_mem_word ((uae_u16 *)m);
}

static uae_u32 REGPARAM2 fastmem_bget (uaecptr addr)
{
    uae_u8 *m;
    addr -= fastmem_start;
    addr &= fastmem_mask;
    m = (uae_u8 *)(fastmemory + addr);
    return do_get_mem_byte(m);
}

static void REGPARAM2 fastmem_lput (uaecptr addr, uae_u32 l)
{
    uae_u8 *m;
    addr -= fastmem_start;
    addr &= fastmem_mask;
    m = fastmemory + addr;
    do_put_mem_long ((uae_u32 *)m, l);
}

static void REGPARAM2 fastmem_wput (uaecptr addr, uae_u32 w)
{
    uae_u8 *m;
    addr -= fastmem_start;
    addr &= fastmem_mask;
    m = fastmemory + addr;
    do_put_mem_word ((uae_u16 *)m, w);
}

static void REGPARAM2 fastmem_bput (uaecptr addr, uae_u32 b)
{
    uae_u8 *m;
    addr -= fastmem_start;
    addr &= fastmem_mask;
    m = (uae_u8 *)(fastmemory + addr);
    do_put_mem_byte(m, b);
}

static int REGPARAM2 fastmem_check (uaecptr addr, uae_u32 size)
{
    addr -= fastmem_start;
    addr &= fastmem_mask;
    return (addr + size) <= allocated_fastmem;
}

static uae_u8 *REGPARAM2 fastmem_xlate (uaecptr addr)
{
    addr -= fastmem_start;
    addr &= fastmem_mask;
    return fastmemory + addr;
}

addrbank fastmem_bank = {
    fastmem_lget, fastmem_wget, fastmem_bget,
    fastmem_lput, fastmem_wput, fastmem_bput,
    fastmem_xlate, fastmem_check, NULL, "Fast memory",
    fastmem_lget, fastmem_wget, ABFLAG_RAM
};

#ifdef FILESYS

/*
 * Filesystem device ROM
 * This is very simple, the Amiga shouldn't be doing things with it.
 */

static uae_u32 REGPARAM3 filesys_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 filesys_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 filesys_bget (uaecptr) REGPARAM;
static void REGPARAM3 filesys_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 filesys_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 filesys_bput (uaecptr, uae_u32) REGPARAM;

static uae_u32 filesys_start; /* Determined by the OS */
uae_u8 *filesysory;

static uae_u32 REGPARAM2 filesys_lget (uaecptr addr)
{
    uae_u8 *m;
#ifdef JIT
    special_mem |= S_READ;
#endif
    addr -= filesys_start;
    addr &= 65535;
    m = filesysory + addr;
    return do_get_mem_long ((uae_u32 *)m);
}

static uae_u32 REGPARAM2 filesys_wget (uaecptr addr)
{
    uae_u8 *m;
#ifdef JIT
    special_mem |= S_READ;
#endif
    addr -= filesys_start;
    addr &= 65535;
    m = filesysory + addr;
    return do_get_mem_word ((uae_u16 *)m);
}

static uae_u32 REGPARAM2 filesys_bget (uaecptr addr)
{
#ifdef JIT
    special_mem |= S_READ;
#endif
    addr -= filesys_start;
    addr &= 65535;
    return filesysory[addr];
}

static void REGPARAM2 filesys_lput (uaecptr addr, uae_u32 l)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log ("filesys_lput called PC=%p\n", M68K_GETPC);
}

static void REGPARAM2 filesys_wput (uaecptr addr, uae_u32 w)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    write_log ("filesys_wput called PC=%p\n", M68K_GETPC);
}

static void REGPARAM2 filesys_bput (uaecptr addr, uae_u32 b)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
}

static addrbank filesys_bank = {
    filesys_lget, filesys_wget, filesys_bget,
    filesys_lput, filesys_wput, filesys_bput,
    default_xlate, default_check, NULL, "Filesystem Autoconfig Area",
    dummy_lgeti, dummy_wgeti, ABFLAG_IO | ABFLAG_SAFE
};

#endif /* FILESYS */

/*
 *  Z3fastmem Memory
 */

static uae_u32 z3fastmem_mask;
uaecptr z3fastmem_start;
static uae_u8 *z3fastmem;

static uae_u32 REGPARAM2 z3fastmem_lget (uaecptr addr)
{
    uae_u8 *m;
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    m = z3fastmem + addr;
    return do_get_mem_long ((uae_u32 *)m);
}

static uae_u32 REGPARAM2 z3fastmem_wget (uaecptr addr)
{
    uae_u8 *m;
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    m = z3fastmem + addr;
    return do_get_mem_word ((uae_u16 *)m);
}

static uae_u32 REGPARAM2 z3fastmem_bget (uaecptr addr)
{
    uae_u8 *m;    
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    m = z3fastmem + addr;
    return do_get_mem_byte (m);
}

static void REGPARAM2 z3fastmem_lput (uaecptr addr, uae_u32 l)
{
    uae_u8 *m;
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    m = z3fastmem + addr;
    do_put_mem_long ((uae_u32 *)m, l);
}

static void REGPARAM2 z3fastmem_wput (uaecptr addr, uae_u32 w)
{
    uae_u8 *m;
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    m = z3fastmem + addr;
    do_put_mem_word ((uae_u16 *)m, w);
}

static void REGPARAM2 z3fastmem_bput (uaecptr addr, uae_u32 b)
{
    uae_u8 *m;
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    m = z3fastmem + addr;
    do_put_mem_byte (m, b);
}

static int REGPARAM2 z3fastmem_check (uaecptr addr, uae_u32 size)
{
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    return (addr + size) <= allocated_z3fastmem;
}

static uae_u8 *REGPARAM2 z3fastmem_xlate (uaecptr addr)
{
    addr -= z3fastmem_start;
    addr &= z3fastmem_mask;
    return z3fastmem + addr;
}

addrbank z3fastmem_bank = {
    z3fastmem_lget, z3fastmem_wget, z3fastmem_bget,
    z3fastmem_lput, z3fastmem_wput, z3fastmem_bput,
    z3fastmem_xlate, z3fastmem_check, NULL, "ZorroIII Fast RAM",
    z3fastmem_lget, z3fastmem_wget, ABFLAG_RAM
};

/* Z3-based UAEGFX-card */
uae_u32 gfxmem_mask; /* for memory.c */
uae_u8 *gfxmemory;
uae_u32 gfxmem_start;

/* ********************************************************** */

/*
 *     Expansion Card (ZORRO II) for 1/2/4/8 MB of Fast Memory
 */

static void expamem_map_fastcard (void)
{
    fastmem_start = ((expamem_hi | (expamem_lo >> 4)) << 16);
    map_banks (&fastmem_bank, fastmem_start >> 16, allocated_fastmem >> 16, 0);
    write_log ("Fastcard: mapped @$%lx: %dMB fast memory\n", fastmem_start, allocated_fastmem >> 20);
}

static void expamem_init_fastcard (void)
{
    uae_u16 mid = uae_id;
    uae_u8 pid = 1;

    expamem_init_clear();
    if (allocated_fastmem == 0x100000)
	expamem_write (0x00, Z2_MEM_1MB + add_memory + zorroII);
    else if (allocated_fastmem == 0x200000)
	expamem_write (0x00, Z2_MEM_2MB + add_memory + zorroII);
    else if (allocated_fastmem == 0x400000)
	expamem_write (0x00, Z2_MEM_4MB + add_memory + zorroII);
    else if (allocated_fastmem == 0x800000)
	expamem_write (0x00, Z2_MEM_8MB + add_memory + zorroII);

    expamem_write (0x08, care_addr);

    expamem_write (0x04, pid);

    expamem_write (0x10, mid >> 8);
    expamem_write (0x14, mid & 0xff);

    expamem_write (0x18, 0x00); /* ser.no. Byte 0 */
    expamem_write (0x1c, 0x00); /* ser.no. Byte 1 */
    expamem_write (0x20, 0x00); /* ser.no. Byte 2 */
    expamem_write (0x24, 0x01); /* ser.no. Byte 3 */

    expamem_write (0x28, 0x00); /* Rom-Offset hi */
    expamem_write (0x2c, 0x00); /* ROM-Offset lo */

    expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/
}

/* ********************************************************** */

#ifdef FILESYS

/* 
 * Filesystem device
 */

static void expamem_map_filesys (void)
{
    uaecptr a;

    filesys_start = ((expamem_hi | (expamem_lo >> 4)) << 16);
    map_banks (&filesys_bank, filesys_start >> 16, 1, 0);
    write_log ("Filesystem: mapped memory @$%lx.\n", filesys_start);
    /* 68k code needs to know this. */
    a = here ();
    org (rtarea_base + 0xFFFC);
    dl (filesys_start + 0x2000);
    org (a);
}

static void expamem_init_filesys (void)
{
    /* struct DiagArea - the size has to be large enough to store several device ROMTags */
    uae_u8 diagarea[] = { 0x90, 0x00, /* da_Config, da_Flags */
                          0x02, 0x00, /* da_Size */
                          0x01, 0x00, /* da_DiagPoint */
                          0x01, 0x06  /* da_BootPoint */
    };

    expamem_init_clear();
    expamem_write (0x00, Z2_MEM_64KB | rom_card | zorroII);

    expamem_write (0x08, no_shutup);

    expamem_write (0x04, 2);
    expamem_write (0x10, uae_id >> 8);
    expamem_write (0x14, uae_id & 0xff);

    expamem_write (0x18, 0x00); /* ser.no. Byte 0 */
    expamem_write (0x1c, 0x00); /* ser.no. Byte 1 */
    expamem_write (0x20, 0x00); /* ser.no. Byte 2 */
    expamem_write (0x24, 0x01); /* ser.no. Byte 3 */

    /* er_InitDiagVec */
    expamem_write (0x28, 0x10); /* Rom-Offset hi */
    expamem_write (0x2c, 0x00); /* ROM-Offset lo */

    expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/

    /* Build a DiagArea */
    memcpy (expamem + 0x1000, diagarea, sizeof diagarea);

    /* Call DiagEntry */
    do_put_mem_word ((uae_u16 *)(expamem + 0x1100), 0x4EF9); /* JMP */
    do_put_mem_long ((uae_u32 *)(expamem + 0x1102), ROM_filesys_diagentry);

    /* What comes next is a plain bootblock */
    do_put_mem_word ((uae_u16 *)(expamem + 0x1106), 0x4EF9); /* JMP */
    do_put_mem_long ((uae_u32 *)(expamem + 0x1108), EXPANSION_bootcode);
    
    memcpy (filesysory, expamem, 0x3000);
}

#endif

/*
 * Zorro III expansion memory
 */

static void expamem_map_z3fastmem_2 (addrbank *bank, int *startp, uae_u32 size, uae_u32 allocated)
{
	int z3fs = ((expamem_hi | (expamem_lo >> 4)) << 16);
  int start = *startp;

	if (start != z3fs) {
    	write_log("WARNING: Z3FAST mapping changed from $%08x to $%08x\n", start, z3fs);
		map_banks(&dummy_bank, start >> 16, size >> 16,
			allocated);
		*startp = z3fs;
    map_banks (bank, start >> 16, size >> 16,
	       allocated);
	}
    write_log ("Fastmem (32bit): mapped @$%08x: %d MB Zorro III fast memory \n",
	       start, allocated / 0x100000);
}

static void expamem_map_z3fastmem (void)
{
    expamem_map_z3fastmem_2 (&z3fastmem_bank, (int *)&z3fastmem_start, currprefs.z3fastmem_size, allocated_z3fastmem);
}

static void expamem_init_z3fastmem_2 (addrbank *bank, uae_u32 start, uae_u32 size, uae_u32 allocated)
{
    int code = (allocated == 0x100000 ? Z2_MEM_1MB
		: allocated == 0x200000 ? Z2_MEM_2MB
		: allocated == 0x400000 ? Z2_MEM_4MB
		: allocated == 0x800000 ? Z2_MEM_8MB
		: allocated == 0x1000000 ? Z2_MEM_16MB
		: allocated == 0x2000000 ? Z2_MEM_32MB
		: allocated == 0x4000000 ? Z2_MEM_64MB
		: allocated == 0x8000000 ? Z2_MEM_128MB
		: allocated == 0x10000000 ? Z2_MEM_256MB
		: allocated == 0x20000000 ? Z2_MEM_512MB
		: Z2_MEM_1GB);
    expamem_init_clear();
    expamem_write (0x00, add_memory | zorroIII | code);

    expamem_write (0x08, care_addr | no_shutup | force_z3 | (allocated > 0x800000 ? ext_size : Z3_MEM_AUTO));

    expamem_write (0x04, 3);

    expamem_write (0x10, uae_id >> 8);
    expamem_write (0x14, uae_id & 0xff);

    expamem_write (0x18, 0x00); /* ser.no. Byte 0 */
    expamem_write (0x1c, 0x00); /* ser.no. Byte 1 */
    expamem_write (0x20, 0x00); /* ser.no. Byte 2 */
    expamem_write (0x24, 0x01); /* ser.no. Byte 3 */

    expamem_write (0x28, 0x00); /* Rom-Offset hi */
    expamem_write (0x2c, 0x00); /* ROM-Offset lo */

    expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/

    map_banks (bank, start >> 16, size >> 16, allocated);
}
static void expamem_init_z3fastmem (void)
{
    expamem_init_z3fastmem_2 (&z3fastmem_bank, z3fastmem_start, currprefs.z3fastmem_size, allocated_z3fastmem);
}

#ifdef PICASSO96
/*
 *  Fake Graphics Card (ZORRO III) - BDK
 */

uaecptr p96ram_start;

static void expamem_map_gfxcard (void)
{
    gfxmem_start = (expamem_hi | (expamem_lo >> 4)) << 16;
    map_banks (&gfxmem_bankx, gfxmem_start >> 16, allocated_gfxmem >> 16, allocated_gfxmem);
    write_log ("UAEGFX-card: mapped @$%lx, %d MB RTG RAM\n", gfxmem_start, allocated_gfxmem / 0x100000);
}

static void expamem_init_gfxcard (void)
{
    int code = (allocated_gfxmem == 0x100000 ? Z2_MEM_1MB
        : allocated_gfxmem == 0x200000 ? Z2_MEM_2MB
        : allocated_gfxmem == 0x400000 ? Z2_MEM_4MB
        : allocated_gfxmem == 0x800000 ? Z2_MEM_8MB
        : allocated_gfxmem == 0x1000000 ? Z2_MEM_16MB
        : allocated_gfxmem == 0x2000000 ? Z2_MEM_32MB
        : allocated_gfxmem == 0x4000000 ? Z2_MEM_64MB
      	: allocated_gfxmem == 0x8000000 ? Z2_MEM_128MB
      	: allocated_gfxmem == 0x10000000 ? Z2_MEM_256MB
      	: allocated_gfxmem == 0x20000000 ? Z2_MEM_512MB
      	: Z2_MEM_1GB);
    int subsize = (allocated_gfxmem == 0x100000 ? Z3_MEM_1MB
      	: allocated_gfxmem == 0x200000 ? Z3_MEM_2MB
      	: allocated_gfxmem == 0x400000 ? Z3_MEM_4MB
      	: allocated_gfxmem == 0x800000 ? Z3_MEM_8MB
      	: 0);

    expamem_init_clear();
    expamem_write (0x00, zorroIII | code);

    expamem_write (0x08, care_addr | no_shutup | force_z3 | ext_size | subsize);

    expamem_write (0x04, 96);

    expamem_write (0x10, uae_id >> 8);
    expamem_write (0x14, uae_id & 0xff);

    expamem_write (0x18, 0x00); /* ser.no. Byte 0 */
    expamem_write (0x1c, 0x00); /* ser.no. Byte 1 */
    expamem_write (0x20, 0x00); /* ser.no. Byte 2 */
    expamem_write (0x24, 0x01); /* ser.no. Byte 3 */

    expamem_write (0x28, 0x00); /* Rom-Offset hi */
    expamem_write (0x2c, 0x00); /* ROM-Offset lo */

    expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/
}
#endif

static size_t fast_filepos, z3_filepos, p96_filepos;

void free_fastmemory (void)
{
    if (fastmemory)
        mapped_free (fastmemory);
    fastmemory = 0;
}

static void allocate_expamem (void)
{
    currprefs.fastmem_size = changed_prefs.fastmem_size;
    currprefs.z3fastmem_size = changed_prefs.z3fastmem_size;
    currprefs.gfxmem_size = changed_prefs.gfxmem_size;

    if (allocated_fastmem != currprefs.fastmem_size) {
	    free_fastmemory ();
    	allocated_fastmem = currprefs.fastmem_size;
    	fastmem_mask = allocated_fastmem - 1;

    	if (allocated_fastmem) {
    		fastmemory = mapped_malloc (allocated_fastmem, "fast");
    		if (fastmemory == 0) {
    			write_log ("Out of memory for fastmem card.\n");
    			allocated_fastmem = 0;
    		}
    	}
	    memory_hardreset();
    }
    if (allocated_z3fastmem != currprefs.z3fastmem_size) {
    	if (z3fastmem)
    		mapped_free (z3fastmem);
    	z3fastmem = 0;

    	allocated_z3fastmem = currprefs.z3fastmem_size;
    	z3fastmem_mask = allocated_z3fastmem - 1;

    	if (allocated_z3fastmem) {
        if(z3fastmem_start != 0x1000000)
          z3fastmem_start = 0x1000000;
    		z3fastmem = mapped_malloc (allocated_z3fastmem, "z3");
    		if (z3fastmem == 0) {
    			write_log ("Out of memory for 32 bit fast memory.\n");
    			allocated_z3fastmem = 0;
    		}
    	}
	    memory_hardreset();
    }
#ifdef PICASSO96
    if (allocated_gfxmem != currprefs.gfxmem_size) {
    	if (gfxmemory)
    		mapped_free (gfxmemory);
    	gfxmemory = 0;

    	allocated_gfxmem = currprefs.gfxmem_size;
    	gfxmem_mask = allocated_gfxmem - 1;

    	if (allocated_gfxmem) {
    		gfxmemory = mapped_malloc (allocated_gfxmem, "gfx");
    		if (gfxmemory == 0) {
    			write_log ("Out of memory for graphics card memory\n");
    			allocated_gfxmem = 0;
    		}
    	}
	    memory_hardreset();
    }
#endif

    z3fastmem_bank.baseaddr = z3fastmem;
    fastmem_bank.baseaddr = fastmemory;
#ifdef PICASSO96
    gfxmem_bankx.baseaddr = gfxmemory;
#endif

#ifdef SAVESTATE
    if (savestate_state == STATE_RESTORE) {
    	if (allocated_fastmem > 0) {
	      restore_ram (fast_filepos, fastmemory);
    		map_banks (&fastmem_bank, fastmem_start >> 16, currprefs.fastmem_size >> 16,
    				allocated_fastmem);
    	}
    	if (allocated_z3fastmem > 0) {
	      restore_ram (z3_filepos, z3fastmem);
    		map_banks (&z3fastmem_bank, z3fastmem_start >> 16, currprefs.z3fastmem_size >> 16,
    				allocated_z3fastmem);
    	}
#ifdef PICASSO96
	    if (allocated_gfxmem > 0 && gfxmem_start > 0) {
	      restore_ram (p96_filepos, gfxmemory);
        map_banks (&gfxmem_bankx, gfxmem_start >> 16, currprefs.gfxmem_size >> 16,
          allocated_gfxmem);
    	}
#endif
    }
#endif /* SAVESTATE */
}

static uaecptr check_boot_rom (void)
{
    uaecptr b = RTAREA_DEFAULT;
    addrbank *ab;

    ab = &get_mem_bank (RTAREA_DEFAULT);
    if (ab) {
	if (valid_address (RTAREA_DEFAULT, 65536))
	    b = RTAREA_BACKUP;
    }
    if (nr_directory_units (NULL))
    	return b;
    if (nr_directory_units (&currprefs))
    	return b;
    if (currprefs.gfxmem_size)
	return b;
    if (currprefs.chipmem_size > 2 * 1024 * 1024)
	return b;
    return 0;
}

uaecptr need_uae_boot_rom (void)
{
    uaecptr v;

    uae_boot_rom = 0;
    v = check_boot_rom ();
    if (v)
	uae_boot_rom = 1;
    return v;
}

void expamem_next(void)
{
    expamem_init_clear();
    map_banks (&expamem_bank, 0xE8, 1, 0);
    ++ecard;
    if (ecard < cardno)
        (*card_init[ecard]) ();
    else
        expamem_init_clear2 ();
}
#ifdef PICASSO96
void p96memstart(void)
{
    p96ram_start = 0x3000000;
}
#endif
void expamem_reset (void)
{
   int do_mount = 1;

   ecard = 0;
   cardno = 0;

   uae_id = hackers_id;

   allocate_expamem ();

   /* check if Kickstart version is below 1.3 */
   if (! ersatzkickfile && kickstart_version
         && (/* Kickstart 1.0 & 1.1! */
               kickstart_version == 0xFFFF
               /* Kickstart < 1.3 */
               || kickstart_version < 34))
   {
      /* warn user */
      write_log ("Kickstart version is below 1.3!  Disabling autoconfig devices.\n");
      do_mount = 0;
   }
    if (need_uae_boot_rom() == 0)
      do_mount = 0;
   if (fastmemory != NULL) {
      card_init[cardno] = expamem_init_fastcard;
      card_map[cardno++] = expamem_map_fastcard;
   }

   z3fastmem_start = currprefs.z3fastmem_start;
   if (z3fastmem != NULL) {
      card_init[cardno] = expamem_init_z3fastmem;
      card_map[cardno++] = expamem_map_z3fastmem;
    	map_banks (&z3fastmem_bank, z3fastmem_start >> 16, currprefs.z3fastmem_size >> 16, allocated_z3fastmem);
   }
#ifdef PICASSO96
   if (gfxmemory != NULL) {
      card_init[cardno] = expamem_init_gfxcard;
      card_map[cardno++] = expamem_map_gfxcard;
   }
#endif
#ifdef FILESYS
   if (do_mount && ! ersatzkickfile) {
      card_init[cardno] = expamem_init_filesys;
      card_map[cardno++] = expamem_map_filesys;
   }
#endif
    if (cardno > 0 && cardno < MAX_EXPANSION_BOARDS) {
	    card_init[cardno] = expamem_init_last;
	    card_map[cardno++] = expamem_map_clear;
    }

    if (cardno == 0)
	expamem_init_clear_zero ();
    else
  (*card_init[0]) ();
}

void expansion_init (void)
{
    allocated_fastmem = 0;
    fastmem_mask = fastmem_start = 0;
    fastmemory = 0;
#ifdef PICASSO96
    allocated_gfxmem = 0;
    gfxmem_mask = gfxmem_start = 0;
    gfxmemory = 0;
#endif

#ifdef FILESYS
    filesys_start = 0;
    filesysory = 0;
#endif

    allocated_z3fastmem = 0;
    z3fastmem_mask = z3fastmem_start = 0;
    z3fastmem = 0;

    expamem_lo = 0;
    expamem_hi = 0;
    
    allocate_expamem ();

#ifdef FILESYS
    filesysory = (uae_u8 *) malloc (0x10000); //mapped_malloc (0x10000, "filesys");
    if (!filesysory) {
	write_log ("virtual memory exhausted (filesysory)!\n");
	abort();
    }
    filesys_bank.baseaddr = (uae_u8*)filesysory;
#endif
}

void expansion_cleanup (void)
{
    if (fastmemory)
	mapped_free (fastmemory);
    fastmemory = 0;
    if (z3fastmem)
	mapped_free (z3fastmem);
    z3fastmem = 0;
#ifdef PICASSO96
    if (gfxmemory)
	mapped_free (gfxmemory);
    gfxmemory = 0;
#endif
#ifdef FILESYS
    if (filesysory)
	free (filesysory);
    filesysory = 0;
#endif
}

void expansion_clear(void)
{
    if (fastmemory)
	memset (fastmemory, 0, allocated_fastmem);
    if (z3fastmem)
	memset (z3fastmem, 0, allocated_z3fastmem > 0x800000 ? 0x800000 : allocated_z3fastmem);
    if (gfxmemory)
	memset (gfxmemory, 0, allocated_gfxmem);
}

#ifdef SAVESTATE

/* State save/restore code.  */

uae_u8 *save_fram (int *len)
{
    *len = allocated_fastmem;
    return fastmemory;
}

uae_u8 *save_zram (int *len, int num)
{
    *len = allocated_z3fastmem;
    return z3fastmem;
}

uae_u8 *save_pram (int *len)
{
    *len = allocated_gfxmem;
    return gfxmemory;
}

void restore_fram (uae_u32 len, size_t filepos)
{
    fast_filepos = filepos;
    changed_prefs.fastmem_size = len;
}

void restore_zram (uae_u32 len, size_t filepos, int num)
{
    z3_filepos = filepos;
    changed_prefs.z3fastmem_size = len;
}

void restore_pram (int len, size_t filepos)
{
    p96_filepos = filepos;
    changed_prefs.gfxmem_size = len;
}

uae_u8 *save_expansion (int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak,*dst;
    if (dstptr)
    	dst = dstbak = dstptr;
    else
      dstbak = dst = (uae_u8 *)malloc (20);
    save_u32 (fastmem_start);
    save_u32 (z3fastmem_start);
    save_u32 (gfxmem_start);
    save_u32 (rtarea_base);
    *len = dst - dstbak;
    return dstbak;
}

uae_u8 *restore_expansion (uae_u8 *src)
{
    fastmem_start = restore_u32 ();
    z3fastmem_start = restore_u32 ();
    gfxmem_start = restore_u32 ();
    rtarea_base = restore_u32 ();
    if (rtarea_base != 0 && rtarea_base != RTAREA_DEFAULT && rtarea_base != RTAREA_BACKUP)
	rtarea_base = 0;
    return src;
}

#endif /* SAVESTATE */
