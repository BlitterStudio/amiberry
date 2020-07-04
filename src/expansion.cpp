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
#include "traps.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "newcpu.h"
#include "savestate.h"
#include "cdtv.h"
#include "cdtvcr.h"
#include "gfxboard.h"
#ifdef CD32
#include "cd32_fmv.h"
#endif
#include "gayle.h"
#include "autoconf.h"
#include "devices.h"


#define CARD_FLAG_CAN_Z3 1
#define CARD_FLAG_CHILD 8
#define CARD_FLAG_UAEROM 16

// More information in first revision HRM Appendix_G
#define BOARD_PROTOAUTOCONFIG 1

#define BOARD_AUTOCONFIG_Z2 2
#define BOARD_AUTOCONFIG_Z3 3
#define BOARD_NONAUTOCONFIG_BEFORE 4
#define BOARD_NONAUTOCONFIG_AFTER_Z2 5
#define BOARD_NONAUTOCONFIG_AFTER_Z3 6
#define BOARD_IGNORE 7

#define KS12_BOOT_HACK 1

#define MAX_EXPANSION_BOARD_SPACE 16

/* ********************************************************** */
/* 00 / 02 */
/* er_Type */

#define Z2_MEM_8MB		0x00 /* Size of Memory Block */
#define Z2_MEM_4MB		0x07
#define Z2_MEM_2MB		0x06
#define Z2_MEM_1MB		0x05
#define Z2_MEM_512KB	0x04
#define Z2_MEM_256KB	0x03
#define Z2_MEM_128KB	0x02
#define Z2_MEM_64KB		0x01
/* extended definitions */
#define Z3_MEM_16MB		0x00
#define Z3_MEM_32MB		0x01
#define Z3_MEM_64MB		0x02
#define Z3_MEM_128MB	0x03
#define Z3_MEM_256MB	0x04
#define Z3_MEM_512MB	0x05
#define Z3_MEM_1GB		0x06

#define chainedconfig	0x08 /* Next config is part of the same card */
#define rom_card	0x10 /* ROM vector is valid */
#define add_memory	0x20 /* Link RAM into free memory list */

/* Type of Expansion Card */
#define protoautoconfig 0x40
#define zorroII		0xc0
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
#define commodore_a2091	    3 /* A2091 / A590 Card from C= */
#define commodore_a2091_ram 10 /* A2091 / A590 Ram on HD-Card */
#define commodore_a2232	    70 /* A2232 Multiport Expansion */
#define ass_nexus_scsi	    1 /* Nexus SCSI Controller */

#define gvp_series_2_scsi   11
#define gvp_iv_24_gfx	    32

/* ********************************************************** */
/* 08 - 0A  */
/* er_Flags */
#define Z3_SS_MEM_SAME		0x00
#define Z3_SS_MEM_AUTO		0x01
#define Z3_SS_MEM_64KB		0x02
#define Z3_SS_MEM_128KB		0x03
#define Z3_SS_MEM_256KB		0x04
#define Z3_SS_MEM_512KB		0x05
#define Z3_SS_MEM_1MB		0x06 /* Zorro III card subsize */
#define Z3_SS_MEM_2MB		0x07
#define Z3_SS_MEM_4MB		0x08
#define Z3_SS_MEM_6MB		0x09
#define Z3_SS_MEM_8MB		0x0a
#define Z3_SS_MEM_10MB		0x0b
#define Z3_SS_MEM_12MB		0x0c
#define Z3_SS_MEM_14MB		0x0d
#define Z3_SS_MEM_defunct1	0x0e
#define Z3_SS_MEM_defunct2	0x0f

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
int uae_boot_rom_type;
int uae_boot_rom_size; /* size = code size only */
static bool chipdone;
static int do_mount;

#define FILESYS_DIAGPOINT 0x01e0
#define FILESYS_BOOTPOINT 0x01f0
#define FILESYS_DIAGAREA 0x2000

/* ********************************************************** */

struct card_data
{
	bool (*initrc)(struct autoconfig_info*);
	bool (*initnum)(struct autoconfig_info*);
	addrbank *(*map)(struct autoconfig_info*);
	struct romconfig *rc;
	const TCHAR *name;
	int flags;
	int zorro;
	uaecptr base;
	uae_u32 size;
	// parsing updated fields
	const struct expansionromtype *ert;
	const struct cpuboardsubtype *cst;
	struct autoconfig_info aci;
};

static struct card_data cards_set[MAX_EXPANSION_BOARD_SPACE];
static struct card_data *cards[MAX_EXPANSION_BOARD_SPACE];

static int ecard, cardno, z3num;
static int restore_cardno;
static addrbank *expamem_bank_current;

static uae_u16 uae_id;

static bool isnonautoconfig(int v)
{
	return v == BOARD_NONAUTOCONFIG_AFTER_Z2 ||
		v == BOARD_NONAUTOCONFIG_AFTER_Z3 ||
		v == BOARD_NONAUTOCONFIG_BEFORE;
}

static bool ks12orolder(void)
{
	/* check if Kickstart version is below 1.3 */
	return kickstart_version && kickstart_version < 34;
}
static bool ks11orolder(void)
{
	return kickstart_version && kickstart_version < 33;
}


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
static uae_u8 expamem[65536 + 4];
static uae_u8 expamem_write_space[65536 + 4];
#define AUTOMATIC_AUTOCONFIG_MAX_ADDRESS 0x80
static int expamem_autoconfig_mode;
static addrbank*(*expamem_map)(struct autoconfig_info*);

static uae_u8 expamem_lo;
static uae_u16 expamem_hi;
uaecptr expamem_z3_pointer_real, expamem_z3_pointer_uae;
uaecptr expamem_z3_highram_real, expamem_z3_highram_uae;
uaecptr expamem_highmem_pointer;
uae_u32 expamem_board_size;
uaecptr expamem_board_pointer;
static uae_u8 slots_e8[8] = { 0 };
static uae_u8 slots_20[(8 * 1024 * 1024) / 65536] = { 0 };

static int z3hack_override;

void set_expamem_z3_hack_mode(int mode)
{
	z3hack_override = mode;
}

bool expamem_z3hack(struct uae_prefs *p)
{
	if (z3hack_override == Z3MAPPING_UAE)
		return true;
	if (z3hack_override == Z3MAPPING_REAL)
		return false;
	return p->z3_mapping_mode == Z3MAPPING_UAE;
}

/* Ugly hack for >2M chip RAM in single pool
 * We can't add it any later or early boot menu
 * stops working because it sets kicktag at the end
 * of chip ram...
 */
static void addextrachip (uae_u32 sysbase)
{
	if (currprefs.chipmem_size <= 0x00200000)
		return;
	if (sysbase & 0x80000001)
		return;
	if (!valid_address (sysbase, 1000))
		return;
	uae_u32 ml = get_long (sysbase + 322);
	if (!valid_address (ml, 32))
		return;
	uae_u32 next;
	while ((next = get_long (ml))) {
		if (!valid_address (ml, 32))
			return;
		uae_u32 upper = get_long (ml + 24);
		uae_u32 lower = get_long (ml + 20);
		if (lower & ~0xffff) {
			ml = next;
			continue;
		}
		uae_u16 attr = get_word (ml + 14);
		if ((attr & 0x8002) != 2) {
			ml = next;
			continue;
		}
		if (upper >= currprefs.chipmem_size)
			return;
		uae_u32 added = currprefs.chipmem_size - upper;
		uae_u32 first = get_long (ml + 16);
		put_long (ml + 24, currprefs.chipmem_size); // mh_Upper
		put_long (ml + 28, get_long (ml + 28) + added); // mh_Free
		uae_u32 next = 0;
		while (first) {
			next = first;
			first = get_long (next);
		}
		if (next) {
			uae_u32 bytes = get_long(next + 4);
			if (next + bytes == 0x00200000) {
				put_long(next + 4, currprefs.chipmem_size - next);
			} else {
				put_long(0x00200000 + 0, 0);
				put_long(0x00200000 + 4, added);
				put_long(next, 0x00200000);
			}
		}
		return;
	}
}

addrbank expamem_null, expamem_none;

DECLARE_MEMORY_FUNCTIONS(expamem);
addrbank expamem_bank = {
	expamem_lget, expamem_wget, expamem_bget,
	expamem_lput, expamem_wput, expamem_bput,
	default_xlate, default_check, NULL, NULL, _T("Autoconfig Z2"),
	dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};
DECLARE_MEMORY_FUNCTIONS(expamemz3);
static addrbank expamemz3_bank = {
	expamemz3_lget, expamemz3_wget, expamemz3_bget,
	expamemz3_lput, expamemz3_wput, expamemz3_bput,
	default_xlate, default_check, NULL, NULL, _T("Autoconfig Z3"),
	dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};

static addrbank *expamem_map_clear (void)
{
	write_log (_T("expamem_map_clear() got called. Shouldn't happen.\n"));
	return NULL;
}

static void expamem_init_clear (void)
{
	memset (expamem, 0xff, sizeof expamem);
	expamem_hi = expamem_lo = 0;
	expamem_map = NULL;
}
/* autoconfig area is "non-existing" after last device */
static void expamem_init_clear_zero (void)
{
	if (currprefs.cpu_model < 68020) {
		map_banks(&dummy_bank, 0xe8, 1, 0);
		if (!currprefs.address_space_24)
			map_banks(&dummy_bank, AUTOCONFIG_Z3 >> 16, 1, 0);
	} else {
		map_banks(&expamem_bank, 0xe8, 1, 0);
		if (!currprefs.address_space_24)
			map_banks(&expamemz3_bank, AUTOCONFIG_Z3 >> 16, 1, 0);
	}
	expamem_bank_current = NULL;
}

static void expamem_init_clear2 (void)
{
	expamem_bank.name = _T("Autoconfig Z2");
	expamemz3_bank.name = _T("Autoconfig Z3");
	expamem_init_clear_zero ();
	ecard = cardno;
}

static addrbank *expamem_init_last (void)
{
	expamem_init_clear2 ();
	write_log (_T("Memory map after autoconfig:\n"));
	//memory_map_dump ();
	return NULL;
}

static uae_u8 REGPARAM2 expamem_read(int addr)
{
	uae_u8 b = (expamem[addr] & 0xf0) | (expamem[addr + 2] >> 4);
	if (addr == 0 || addr == 2 || addr == 0x40 || addr == 0x42)
		return b;
	b = ~b;
	return b;
}

static void REGPARAM2 expamem_write(uaecptr addr, uae_u32 value)
{
	addr &= 0xffff;
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		expamem[addr] = (value & 0xf0);
		expamem[addr + 2] = (value & 0x0f) << 4;
	} else {
		expamem[addr] = ~(value & 0xf0);
		expamem[addr + 2] = ~((value & 0x0f) << 4);
	}
}

static int REGPARAM2 expamem_type (void)
{
	return expamem_read(0) & 0xc0;
}

static void call_card_init(int index)
{	
	addrbank *ab, *abe;
	bool ok = false;
	card_data *cd = cards[ecard];
	struct autoconfig_info *aci = &cd->aci;

	expamem_bank.name = cd->name ? cd->name : _T("None");
	aci->prefs = &currprefs;
	aci->doinit = true;
	aci->devnum = (cd->flags >> 16) & 255;
	aci->ert = cd->ert;
	aci->cst = cd->cst;
	aci->rc = cd->rc;
	aci->zorro = cd->zorro;
	memset(aci->autoconfig_raw, 0xff, sizeof aci->autoconfig_raw);
	if (cd->initnum) {
		ok = cd->initnum(aci);
	} else {
		ok = cd->initrc(aci);
	}
	if (ok) {
		ab = NULL;
		if (!cd->map)
			ab = aci->addrbank;
	} else {
		write_log(_T("Card %d: skipping autoconfig (init failed)\n"), ecard);
		expamem_next(NULL, NULL);
		return;
	}
	if (ab == &expamem_null || cd->zorro < 1 || cd->zorro > 3 || aci->zorro < 0) {
		write_log(_T("Card %d: skipping autoconfig (not autoconfig)\n"), ecard);
		expamem_next(NULL, NULL);
		return;
	}

	expamem_board_pointer = cd->base;
	expamem_board_size = cd->size;

	abe = ab;
	if (!abe)
		abe = &expamem_bank;
	expamem_autoconfig_mode = 0;
	if (abe != &expamem_bank) {
		for (int i = 0; i < 16 * 4; i++) {
			expamem[i] = abe->sub_banks ? abe->sub_banks[0].bank->bget(i) : abe->bget(i);
		}
	}

	if (ab) {
		// non-NULL: not using expamem_bank
		expamem_bank_current = ab;
		if ((cd->flags & CARD_FLAG_CAN_Z3) && currprefs.cs_z3autoconfig && !currprefs.address_space_24) {
			map_banks(&expamemz3_bank, AUTOCONFIG_Z3 >> 16, 1, 0);
			map_banks(&dummy_bank, 0xE8, 1, 0);
		} else {
			map_banks(&expamem_bank, 0xE8, 1, 0);
			if (!currprefs.address_space_24)
				map_banks(&dummy_bank, AUTOCONFIG_Z3 >> 16, 1, 0);
		}
	} else {
		if ((cd->flags & CARD_FLAG_CAN_Z3) && currprefs.cs_z3autoconfig && !currprefs.address_space_24) {
			map_banks(&expamemz3_bank, AUTOCONFIG_Z3 >> 16, 1, 0);
			map_banks(&dummy_bank, 0xE8, 1, 0);
			expamem_bank_current = &expamem_bank;
		} else {
			map_banks(&expamem_bank, 0xE8, 1, 0);
			if (!currprefs.address_space_24)
				map_banks(&dummy_bank, AUTOCONFIG_Z3 >> 16, 1, 0);
			expamem_bank_current = NULL;
		}
	}
}

static void boardmessage(addrbank *mapped, bool success)
{
	uae_u8 type = expamem_read(0);
	int size = expamem_board_size;
	TCHAR sizemod = 'K';

	size /= 1024;
	if (size > 8 * 1024) {
		sizemod = 'M';
		size /= 1024;
	}
	write_log (_T("Card %d: Z%d 0x%08x %4d%c %s %s%s\n"),
		ecard + 1, (type & 0xc0) == zorroII ? 2 : ((type & 0xc0) == zorroIII ? 3 : 1),
		expamem_board_pointer, size, sizemod,
		type & rom_card ? _T("ROM") : (type & add_memory ? _T("RAM") : _T("IO ")),
		mapped->name,
		success ? _T("") : _T(" [SHUT UP]"));
}

void expamem_shutup(addrbank *mapped)
{
	if (mapped) {
		mapped->start = 0xffffffff;
		boardmessage(mapped, false);
	}
}

void expamem_next(addrbank *mapped, addrbank *next)
{
	if (mapped)
		boardmessage(mapped, mapped->start != 0xffffffff);

	expamem_init_clear();
	expamem_init_clear_zero();
	for (;;) {
		++ecard;
		if (ecard >= cardno)
			break;
		struct card_data *ec = cards[ecard];
		if (ec->zorro == 3 && ec->base == 0xffffffff) {
			write_log(_T("Autoconfig chain enumeration aborted, 32-bit address space overflow.\n"));
			ecard = cardno;
			break;
		}
		if (ec->initrc && isnonautoconfig(ec->zorro)) {
			struct autoconfig_info aci = { 0 };
			aci.doinit = true;
			aci.prefs = &currprefs;
			aci.rc = cards[ecard]->rc;
			ec->initrc(&aci);
		} else {
			call_card_init(ecard);
			break;
		}
	}
	if (ecard >= cardno) {
		expamem_init_clear2();
		expamem_init_last();
	}
}

static void expamemz3_map(void)
{
	uaecptr addr = ((expamem_hi << 8) | expamem_lo) << 16;
	if (!expamem_z3hack(&currprefs)) {
		expamem_board_pointer = addr;
	} else {
		if (addr != expamem_board_pointer) {
			put_word (regs.regs[11] + 0x20, expamem_board_pointer >> 16);
			put_word (regs.regs[11] + 0x28, expamem_board_pointer >> 16);
		}
	}
}

static uae_u32 REGPARAM2 expamem_lget (uaecptr addr)
{
	if (expamem_bank_current && expamem_bank_current != &expamem_bank) {
		if (expamem_autoconfig_mode && (addr & 0xffff) < AUTOMATIC_AUTOCONFIG_MAX_ADDRESS) {
			return (expamem_wget(addr) << 16) | expamem_wget(addr + 2);
		}
		return expamem_bank_current->sub_banks ? expamem_bank_current->sub_banks[0].bank->lget(addr) : expamem_bank_current->lget(addr);
	}
	write_log (_T("warning: Z2 READ.L from address $%08x PC=%x\n"), addr, M68K_GETPC);
	return (expamem_wget (addr) << 16) | expamem_wget (addr + 2);
}

static uae_u32 REGPARAM2 expamem_wget (uaecptr addr)
{
	if (expamem_bank_current && expamem_bank_current != &expamem_bank) {
		if (expamem_autoconfig_mode && (addr & 0xffff) < AUTOMATIC_AUTOCONFIG_MAX_ADDRESS) {
			return (expamem_bget(addr) << 8) | expamem_bget(addr + 1);
		}
		return expamem_bank_current->sub_banks ? expamem_bank_current->sub_banks[0].bank->wget(addr) : expamem_bank_current->wget(addr);
	}
	if (expamem_type() != zorroIII) {
		if (expamem_bank_current && expamem_bank_current != &expamem_bank)
			return expamem_bank_current->bget(addr) << 8;
	}
	uae_u32 v = (expamem_bget (addr) << 8) | expamem_bget (addr + 1);
	write_log (_T("warning: READ.W from address $%08x=%04x PC=%x\n"), addr, v & 0xffff, M68K_GETPC);
	return v;
}

static uae_u32 REGPARAM2 expamem_bget (uaecptr addr)
{
	uae_u8 b;
	if (!chipdone) {
		chipdone = true;
		addextrachip (get_long (4));
	}
	if (expamem_bank_current && expamem_bank_current != &expamem_bank) {
		if (expamem_autoconfig_mode && (addr & 0xffff) < AUTOMATIC_AUTOCONFIG_MAX_ADDRESS) {
			return expamem[addr & 0xffff];
		}
		return expamem_bank_current->sub_banks ? expamem_bank_current->sub_banks[0].bank->bget(addr) : expamem_bank_current->bget(addr);
	}
	addr &= 0xFFFF;
	b = expamem[addr];
	return b;
}

static void REGPARAM2 expamem_lput (uaecptr addr, uae_u32 value)
{
	if (expamem_bank_current && expamem_bank_current != &expamem_bank) {
		if (expamem_autoconfig_mode && (addr & 0xffff) < AUTOMATIC_AUTOCONFIG_MAX_ADDRESS) {
			return;
		}
		expamem_bank_current->sub_banks ? expamem_bank_current->sub_banks[0].bank->lput(addr, value) : expamem_bank_current->lput(addr, value);
		return;
	}
	write_log (_T("warning: Z2 WRITE.L to address $%08x : value $%08x\n"), addr, value);
}

static void REGPARAM2 expamem_wput (uaecptr addr, uae_u32 value)
{
	value &= 0xffff;
	if (ecard >= cardno)
		return;
	card_data *cd = cards[ecard];
	if (!expamem_map)
		expamem_map = cd->map;
	if (expamem_type () != zorroIII) {
		write_log (_T("warning: WRITE.W to address $%08x : value $%x PC=%08x\n"), addr, value, M68K_GETPC);
	}
	switch (addr & 0xff) {
	case 0x48:
		// A2630 boot rom writes WORDs to Z2 boards!
		if (expamem_type() == zorroII) {
			expamem_lo = 0;
			expamem_hi = (value >> 8) & 0xff;
			expamem_board_pointer = (expamem_hi | (expamem_lo >> 4)) << 16;
			if (expamem_map) {
				expamem_next(expamem_map(&cd->aci), NULL);
				return;
			}
			if (expamem_autoconfig_mode) {
				map_banks_z2(cd->aci.addrbank, expamem_board_pointer >> 16, expamem_board_size >> 16);
				cd->initrc(&cd->aci);
				expamem_next(cd->aci.addrbank, NULL);
				return;
			}
			if (expamem_bank_current && expamem_bank_current != &expamem_bank) {
				expamem_bank_current->sub_banks ? expamem_bank_current->sub_banks[0].bank->bput(addr, value >> 8) : expamem_bank_current->bput(addr, value >> 8);
				return;
			}
		}
		// Z3 = do nothing
		break;
	case 0x44:
		if (expamem_type() == zorroIII) {
			expamem_hi = (value & 0xff00) >> 8;
			expamem_lo = value & 0x00ff;
			expamemz3_map();
		}
		if (expamem_map) {
			expamem_next(expamem_map(&cd->aci), NULL);
			return;
		}
		if (expamem_autoconfig_mode) {
			map_banks_z3(cd->aci.addrbank, expamem_board_pointer >> 16, expamem_board_size >> 16);
			cd->initrc(&cd->aci);
			expamem_next(cd->aci.addrbank, NULL);
			return;
		}
		break;
	case 0x4c:
		if (expamem_map) {
			expamem_next (NULL, NULL);
			return;
		}
		break;
	}
	if (expamem_bank_current && expamem_bank_current != &expamem_bank) {
		expamem_bank_current->sub_banks ? expamem_bank_current->sub_banks[0].bank->wput(addr, value) : expamem_bank_current->wput(addr, value);
	}
}

static void REGPARAM2 expamem_bput (uaecptr addr, uae_u32 value)
{
	value &= 0xff;
	if (ecard >= cardno)
		return;
	card_data *cd = cards[ecard];
	if (!expamem_map)
		expamem_map = cd->map;
	if (expamem_type() == protoautoconfig) {
		switch (addr & 0xff) {
		case 0x22:
			expamem_hi = value & 0x7f;
			expamem_board_pointer = AUTOCONFIG_Z2 | (expamem_hi * 4096);
			if (expamem_map) {
				expamem_next(expamem_map(&cd->aci), NULL);
				return;
			}
			break;
		}
	} else {
		switch (addr & 0xff) {
		  case 0x48:
			  if (expamem_type () == zorroII) {
			    expamem_hi = value & 0xff;
				  expamem_board_pointer = (expamem_hi | (expamem_lo >> 4)) << 16;
				  if (expamem_map) {
					  expamem_next(expamem_map(&cd->aci), NULL);
					  return;
				  }
				  if (expamem_autoconfig_mode) {
					  map_banks_z2(cd->aci.addrbank, expamem_board_pointer >> 16, expamem_board_size >> 16);
					  cd->initrc(&cd->aci);
					  expamem_next(cd->aci.addrbank, NULL);
					  return;
				  }
		    } else {
			    expamem_lo = value & 0xff;
		    }
			  break;
		  case 0x44:
			  if (expamem_type() == zorroIII) {
				  expamem_hi = value & 0xff;
				  expamemz3_map();
				  if (expamem_map) {
					  expamem_next(expamem_map(&cd->aci), NULL);
					  return;
				  }
				  if (expamem_autoconfig_mode) {
					  map_banks_z3(cd->aci.addrbank, expamem_board_pointer >> 16, expamem_board_size >> 16);
					  cd->initrc(&cd->aci);
					  expamem_next(cd->aci.addrbank, NULL);
					  return;
				  }
			  }
			  break;
		  case 0x4a:
			  if (expamem_type () == zorroII) {
			    expamem_lo = value & 0xff;
        }
			  if (expamem_autoconfig_mode)
				  return;
			  break;

		case 0x4c:
			  if (expamem_map) {
				  expamem_hi = expamem_lo = 0xff;
				  expamem_board_pointer = 0xffffffff;
				  addrbank *ab = expamem_map(&cd->aci);
				  if (ab)
					  ab->start = 0xffffffff;
				  expamem_next(ab, NULL);
				  return;
			  }
			  break;
	  }
	}
	if (expamem_bank_current && expamem_bank_current != &expamem_bank) {
		expamem_bank_current->sub_banks ? expamem_bank_current->sub_banks[0].bank->bput(addr, value) : expamem_bank_current->bput(addr, value);
	}
}

static uae_u32 REGPARAM2 expamemz3_bget (uaecptr addr)
{
	int reg = addr & 0xff;
	if (!expamem_bank_current)
		return 0;
	if (addr & 0x100)
		reg += 2;
	return expamem_bank_current->bget(reg + 0);
}

static uae_u32 REGPARAM2 expamemz3_wget (uaecptr addr)
{
	uae_u32 v = (expamemz3_bget (addr) << 8) | expamemz3_bget (addr + 1);
	write_log (_T("warning: Z3 READ.W from address $%08x=%04x PC=%x\n"), addr, v & 0xffff, M68K_GETPC);
	return v;
}

static uae_u32 REGPARAM2 expamemz3_lget (uaecptr addr)
{
	write_log (_T("warning: Z3 READ.L from address $%08x PC=%x\n"), addr, M68K_GETPC);
	return (expamemz3_wget (addr) << 16) | expamemz3_wget (addr + 2);
}

static void REGPARAM2 expamemz3_bput (uaecptr addr, uae_u32 value)
{
	int reg = addr & 0xff;
	if (!expamem_bank_current)
		return;
	if (addr & 0x100)
		reg += 2;
	if (reg == 0x48) {
		if (expamem_type() == zorroII) {
			expamem_hi = value & 0xff;
			expamem_board_pointer = (expamem_hi | (expamem_lo >> 4)) << 16;
		} else {
			expamem_lo = value & 0xff;
		}
	} else if (reg == 0x44) {
		if (expamem_type() == zorroIII) {
			expamem_hi = value & 0xff;
			expamemz3_map();
		}
	} else if (reg == 0x4a) {
		if (expamem_type() == zorroII)
			expamem_lo = value & 0xff;
	}
	expamem_bank_current->bput(reg, value);
}

static void REGPARAM2 expamemz3_wput (uaecptr addr, uae_u32 value)
{
	int reg = addr & 0xff;
	if (!expamem_bank_current)
		return;
	if (addr & 0x100)
		reg += 2;
	if (reg == 0x44) {
		if (expamem_type() == zorroIII) {
			expamem_hi = (value & 0xff00) >> 8;
			expamem_lo = value & 0x00ff;
			expamemz3_map();
		}
	}
	expamem_bank_current->wput(reg, value);
}
static void REGPARAM2 expamemz3_lput (uaecptr addr, uae_u32 value)
{
	write_log (_T("warning: Z3 WRITE.L to address $%08x : value $%08x\n"), addr, value);
}

#ifdef CD32

static bool expamem_init_cd32fmv (struct autoconfig_info *aci)
{
	expamem_init_clear ();
	load_rom_rc(aci->rc, ROMTYPE_CD32CART, 262144, 0, expamem, 128, 0);
	memcpy(aci->autoconfig_raw, expamem, sizeof aci->autoconfig_raw);
	expamem_map = cd32_fmv_init;
	return true;
}

#endif

/* ********************************************************** */

/*
*  Fast Memory
*/


MEMORY_ARRAY_FUNCTIONS(fastmem, 0);
MEMORY_ARRAY_FUNCTIONS(fastmem, 1);
MEMORY_ARRAY_FUNCTIONS(fastmem, 2);
MEMORY_ARRAY_FUNCTIONS(fastmem, 3);

addrbank fastmem_bank[MAX_RAM_BOARDS] =
{
	{
		fastmem0_lget, fastmem0_wget, fastmem0_bget,
		fastmem0_lput, fastmem0_wput, fastmem0_bput,
		fastmem0_xlate, fastmem0_check, NULL, _T("*"), _T("Fast memory"),
		fastmem0_wget,
		ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
	}
};

#ifdef FILESYS

/*
* Filesystem device ROM/RAM space
*/

DECLARE_MEMORY_FUNCTIONS(filesys);
addrbank filesys_bank = {
	filesys_lget, filesys_wget, filesys_bget,
	filesys_lput, filesys_wput, filesys_bput,
	filesys_xlate, filesys_check, NULL, _T("*"), _T("Filesystem autoconfig"),
	filesys_wget,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static bool filesys_write(uaecptr addr)
{
	return addr >= 0x4000;
}

static uae_u32 REGPARAM2 filesys_lget (uaecptr addr)
{
  uae_u8 *m;
	addr -= filesys_bank.start;
  addr &= 65535;
  m = filesys_bank.baseaddr + addr;
  return do_get_mem_long ((uae_u32 *)m);
}

static uae_u32 REGPARAM2 filesys_wget (uaecptr addr)
{
  uae_u8 *m;
	addr -= filesys_bank.start;
  addr &= 65535;
  m = filesys_bank.baseaddr + addr;
  return do_get_mem_word ((uae_u16 *)m);
}

static uae_u32 REGPARAM2 filesys_bget (uaecptr addr)
{
	addr -= filesys_bank.start;
  addr &= 65535;
  return filesys_bank.baseaddr[addr];
}


static void REGPARAM2 filesys_bput(uaecptr addr, uae_u32 b)
{
	addr -= filesys_bank.start;
	addr &= 65535;
	if (!filesys_write(addr))
		return;
	filesys_bank.baseaddr[addr] = b;
}

static void REGPARAM2 filesys_lput (uaecptr addr, uae_u32 l)
{
	addr -= filesys_bank.start;
	addr &= 65535;
	if (!filesys_write(addr))
		return;
	filesys_bank.baseaddr[addr + 0] = l >> 24;
	filesys_bank.baseaddr[addr + 1] = l >> 16;
	filesys_bank.baseaddr[addr + 2] = l >> 8;
	filesys_bank.baseaddr[addr + 3] = l >> 0;
}

static void REGPARAM2 filesys_wput (uaecptr addr, uae_u32 w)
{
	addr -= filesys_bank.start;
	addr &= 65535;
	if (!filesys_write(addr))
		return;
	filesys_bank.baseaddr[addr + 0] = w >> 8;
	filesys_bank.baseaddr[addr + 1] = w & 0xff;
}

static int REGPARAM2 filesys_check(uaecptr addr, uae_u32 size)
{
	addr -= filesys_bank.start;
	addr &= 65535;
	return (addr + size) <= filesys_bank.allocated_size;
}
static uae_u8 *REGPARAM2 filesys_xlate(uaecptr addr)
{
	addr -= filesys_bank.start;
	addr &= 65535;
	return filesys_bank.baseaddr + addr;
}

#endif /* FILESYS */

DECLARE_MEMORY_FUNCTIONS(uaeboard);
addrbank uaeboard_bank = {
	uaeboard_lget, uaeboard_wget, uaeboard_bget,
	uaeboard_lput, uaeboard_wput, uaeboard_bput,
	uaeboard_xlate, uaeboard_check, NULL, _T("*"), _T("UAE Board"),
	dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

uae_u32 uaeboard_base; /* Determined by the OS */
static uae_u32 uaeboard_ram_start;
#define UAEBOARD_WRITEOFFSET 0x4000

static bool uaeboard_write(uaecptr addr)
{
	if (addr >= UAEBOARD_WRITEOFFSET)
		return true;
	return false;
}

static uae_u32 REGPARAM2 uaeboard_wget(uaecptr addr)
{
	uae_u8 *m;
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	m = uaeboard_bank.baseaddr + addr;
	uae_u16 v = do_get_mem_word((uae_u16 *)m);
	return v;
}

static uae_u32 REGPARAM2 uaeboard_lget(uaecptr addr)
{
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	uae_u32 v = uaeboard_wget(addr) << 16;
	v |= uaeboard_wget(addr + 2);
	return v;
}

static uae_u32 REGPARAM2 uaeboard_bget(uaecptr addr)
{
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	return uaeboard_bank.baseaddr[addr];
}

static void REGPARAM2 uaeboard_wput(uaecptr addr, uae_u32 w)
{
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	if (!uaeboard_write(addr))
		return;
	uae_u8 *m = uaeboard_bank.baseaddr + addr;
	put_word_host(m, w);
}

static void REGPARAM2 uaeboard_lput(uaecptr addr, uae_u32 l)
{
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	if (!uaeboard_write(addr))
		return;
	uaeboard_wput(addr, l >> 16);
	uaeboard_wput(addr + 2, l);
}

static void REGPARAM2 uaeboard_bput(uaecptr addr, uae_u32 b)
{
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	if (!uaeboard_write(addr))
		return;
	uaeboard_bank.baseaddr[addr] = b;
}

static int REGPARAM2 uaeboard_check(uaecptr addr, uae_u32 size)
{
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	return (addr + size) <= uaeboard_bank.allocated_size;
}
static uae_u8 *REGPARAM2 uaeboard_xlate(uaecptr addr)
{
	addr -= uaeboard_base & 65535;
	addr &= 65535;
	return uaeboard_bank.baseaddr + addr;
}

static addrbank *expamem_map_uaeboard(struct autoconfig_info *aci)
{
	uaeboard_base = expamem_board_pointer;
	uaeboard_bank.start = uaeboard_base;
	map_banks_z2(&uaeboard_bank, uaeboard_base >> 16, 1);
	return &uaeboard_bank;
}

static bool get_params_filesys(struct uae_prefs *prefs, struct expansion_params *p)
{
	p->device_order = prefs->uaeboard_order;
	return true;
}

static void add_rtarea_pointer(struct autoconfig_info *aci)
{
	if (aci->doinit) {
		uaecptr addr = 0;
		if (aci->prefs->uaeboard == 1) {
			addr = rtarea_base;
		}
		if (addr) {
			expamem[0x48] = addr >> 24;
			expamem[0x49] = addr >> 16;
			expamem[0x4a] = addr >> 8;
			expamem[0x4b] = addr >> 0;
		}
	}
}

static bool expamem_init_uaeboard(struct autoconfig_info *aci)
{
	bool ks12 = ks12orolder();
	struct uae_prefs *p = aci->prefs;

	aci->label = _T("UAE Boot ROM");
	aci->addrbank = &uaeboard_bank;
	aci->get_params = get_params_filesys;

	expamem_init_clear();
	expamem_write(0x00, Z2_MEM_64KB | zorroII);

	expamem_write(0x08, no_shutup);

	expamem_write(0x04, 1);
	expamem_write(0x10, 6502 >> 8);
	expamem_write(0x14, 6502 & 0xff);

	expamem_write(0x18, 0x00); /* ser.no. Byte 0 */
	expamem_write(0x1c, 0x00); /* ser.no. Byte 1 */
	expamem_write(0x20, p->uaeboard); /* ser.no. Byte 2 */
	expamem_write(0x24, 0x03); /* ser.no. Byte 3 */

	uae_u8 *ptr = uaeboard_bank.baseaddr;

	add_rtarea_pointer(aci);

	expamem_write(0x28, 0x00); /* ROM-Offset hi */
	expamem_write(0x2c, 0x00); /* ROM-Offset lo */

	memcpy(aci->autoconfig_raw, expamem, sizeof aci->autoconfig_raw);

	if (!aci->doinit)
		return true;

	memcpy(ptr, expamem, 0x100);

	return true;
}

/*
 *  Z3fastmem Memory
 */

MEMORY_ARRAY_FUNCTIONS(z3fastmem, 0);

addrbank z3fastmem_bank[MAX_RAM_BOARDS] =
{
	{
		z3fastmem0_lget, z3fastmem0_wget, z3fastmem0_bget,
		z3fastmem0_lput, z3fastmem0_wput, z3fastmem0_bput,
		z3fastmem0_xlate, z3fastmem0_check, NULL, _T("*"), _T("Zorro III Fast RAM"),
		z3fastmem0_wget,
		ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
  }
};

/* ********************************************************** */

/*
*     Expansion Card (ZORRO II) for 64/128/256/512KB 1/2/4/8MB of Fast Memory
 */

static addrbank *expamem_map_fastcard(struct autoconfig_info *aci)
{
	int devnum = aci->devnum;
	uae_u32 start = ((expamem_hi | (expamem_lo >> 4)) << 16);
	addrbank *ab = &fastmem_bank[devnum];
	if (start == 0x00ff0000)
		return ab;
	uae_u32 size = ab->allocated_size;
	ab->start = start;
	if (ab->start && size) {
		map_banks_z2 (ab, ab->start >> 16, size >> 16);
  }
	return ab;
}

static bool fastmem_autoconfig(struct uae_prefs *p, struct autoconfig_info *aci, int zorro, uae_u8 type, uae_u32 serial, int allocated)
{
  uae_u16 mid = 0;
  uae_u8 pid;
	uae_u8 flags = 0;
	uae_u8 ac[16] = { 0 };
	int boardnum = aci->devnum;
	bool canforceac = false;

	if (aci->ert) {
		canforceac = true;
	}

	uae_u8 *forceac = NULL;
	struct ramboard *rb = NULL;

	if (!mid) {
		if (zorro <= 2) {
			rb = &p->fastmem[boardnum];
			if (rb->autoconfig[0]) {
				forceac = rb->autoconfig;
			} else {
			  pid = 81;
			}
		} else {
			int subsize = (allocated == 0x100000 ? Z3_SS_MEM_1MB
						   : allocated == 0x200000 ? Z3_SS_MEM_2MB
						   : allocated == 0x400000 ? Z3_SS_MEM_4MB
						   : allocated == 0x800000 ? Z3_SS_MEM_8MB
						   : Z3_SS_MEM_SAME);
			rb = &p->z3fastmem[boardnum];
			if (rb->autoconfig[0]) {
				forceac = rb->autoconfig;
			} else {
  			pid = 83;
      }
			flags |= care_addr | force_z3 | (allocated > 0x800000 ? ext_size : subsize);
		}
	} else if (canforceac) {
		if (zorro <= 2) {
			rb = &p->fastmem[boardnum];
		} else if (zorro == 3) {
			rb = &p->z3fastmem[boardnum];
		}
		if (rb && rb->autoconfig[0]) {
			forceac = rb->autoconfig;
		}
	}
	if (!mid) {
		mid = uae_id;
		serial = 1;
	}

	if (forceac) {
		for (int i = 0; i < 16; i++) {
			ac[i] = forceac[i];
			ac[0] &= ~7;
			ac[0] |= type & 7;
			if (flags)
				ac[0x08 / 4] = flags;
		}
	} else {
		ac[0x00 / 4] = type;
		ac[0x04 / 4] = pid;
		ac[0x08 / 4] = flags;
		ac[0x10 / 4] = mid >> 8;
		ac[0x14 / 4] = (uae_u8)mid;
		ac[0x18 / 4] = serial >> 24;
		ac[0x1c / 4] = serial >> 16;
		ac[0x20 / 4] = serial >> 8;
		ac[0x24 / 4] = serial >> 0;
	}

	expamem_write(0x00, ac[0x00 / 4]);
	expamem_write(0x04, ac[0x04 / 4]);
	expamem_write(0x08, ac[0x08 / 4]);
	expamem_write(0x10, ac[0x10 / 4]);
	expamem_write(0x14, ac[0x14 / 4]);

	expamem_write(0x18, ac[0x18 / 4]); /* ser.no. Byte 0 */
	expamem_write(0x1c, ac[0x1c / 4]); /* ser.no. Byte 1 */
	expamem_write(0x20, ac[0x20 / 4]); /* ser.no. Byte 2 */
	expamem_write(0x24, ac[0x24 / 4]); /* ser.no. Byte 3 */

  expamem_write (0x28, 0x00); /* ROM-Offset hi */
  expamem_write (0x2c, 0x00); /* ROM-Offset lo */

  expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/

	return true;
}

static bool expamem_init_fastcard_2(struct autoconfig_info *aci, int zorro)
{
	struct uae_prefs *p = aci->prefs;
	addrbank *bank = &fastmem_bank[aci->devnum];
	uae_u8 type = add_memory | zorroII;
	int size = p->fastmem[aci->devnum].size;

	aci->label = zorro == 1 ? _T("Z1 Fast RAM") : _T("Z2 Fast RAM");
	aci->zorro = zorro;

	expamem_init_clear ();
	if (size == 65536)
		type |= Z2_MEM_64KB;
	else if (size == 131072)
		type |= Z2_MEM_128KB;
	else if (size == 262144)
		type |= Z2_MEM_256KB;
	else if (size == 524288)
		type |= Z2_MEM_512KB;
	else if (size == 0x100000)
		type |= Z2_MEM_1MB;
	else if (size == 0x200000)
		type |= Z2_MEM_2MB;
	else if (size == 0x400000)
		type |= Z2_MEM_4MB;
	else if (size == 0x800000)
		type |= Z2_MEM_8MB;

	aci->addrbank = bank;

	if (!fastmem_autoconfig(p, aci, BOARD_AUTOCONFIG_Z2, type, 1, size)) {
		aci->zorro = -1;
	}

	memcpy(aci->autoconfig_raw, expamem, sizeof aci->autoconfig_raw);

	return true;
}

static bool expamem_init_fastcard(struct autoconfig_info *aci)
{
	return expamem_init_fastcard_2(aci, 2);
}

/* ********************************************************** */

#ifdef FILESYS

static bool expamem_rtarea_init(struct autoconfig_info *aci)
{
	aci->start = rtarea_base;
	aci->size = 65536;
	aci->addrbank = &rtarea_bank;
	aci->label = _T("UAE Boot ROM");
	return true;
}

/* 
 * Filesystem device
 */

static void expamem_map_filesys_update(void)
{
	/* 68k code needs to know this. */
	uaecptr a = here();
	org(rtarea_base + RTAREA_FSBOARD);
	dl(filesys_bank.start + 0x2000);
	org(a);
}

static addrbank *expamem_map_filesys (struct autoconfig_info *aci)
{
	mapped_free(&filesys_bank);
	filesys_bank.start = expamem_board_pointer;
	filesys_bank.mask = filesys_bank.reserved_size - 1;
	if (expamem_board_pointer == 0xffffffff)
		return &filesys_bank;
	mapped_malloc(&filesys_bank);
	memcpy (filesys_bank.baseaddr, expamem, 0x3000);
	map_banks_z2(&filesys_bank, filesys_bank.start >> 16, 1);
	expamem_map_filesys_update();
	return &filesys_bank;
}

#if KS12_BOOT_HACK
static uaecptr ks12_resident;
static void set_ks12_boot_hack(void)
{
	uaecptr old = here();
	org(ks12_resident);
	dw(ks12orolder() ? 0x4afc : 0x0000);
	org(ks12_resident + 11);
	db((uae_u8)kickstart_version);
	org(old);
}

void create_ks12_boot(void)
{
	// KS 1.2 boot resident
	uaecptr name = ds(_T("UAE boot"));
	align(2);
	uaecptr code = here();
	// allocate fake diagarea
	dl(0x48e73f3e); // movem.l d2-d7/a2-a6,-(sp)
	dw(0x203c); // move.l #x,d0
	dl(0x0300);
	dw(0x7201); // moveq #1,d1
	dl(0x4eaeff3a); // jsr -0xc6(a6)
	dw(0x2440); // move.l d0,a2 ;diag area
	dw(0x9bcd); // sub.l a5,a5 ;expansionbase
	dw(0x97cb); // sub.l a3,a3 ;configdev
	dw(0x4eb9); // jsr
	dl(ROM_filesys_diagentry);
	dl(0x4cdf7cfc); // movem.l (sp)+,d2-d7/a2-a6
	dw(0x4e75);
	// struct Resident
	ks12_resident = here();
	dw(0x0000);
	dl(ks12_resident);
	dl(ks12_resident + 26);
	db(1); // RTF_COLDSTART
	db((uae_u8)kickstart_version); // version
	db(0); // NT_UNKNOWN
	db(1); // priority
	dl(name);
	dl(name);
	dl(code);
	set_ks12_boot_hack();
}
#endif

static bool expamem_init_filesys(struct autoconfig_info *aci)
{
	bool ks12 = ks12orolder();
	bool rom = !(ks12 || !do_mount);

	if (aci) {
		aci->label = ks12 ? _T("Pre-KS 1.3 UAE FS ROM") : _T("UAE FS ROM");
		aci->get_params = get_params_filesys;
		aci->addrbank = &filesys_bank;
	}

  /* struct DiagArea - the size has to be large enough to store several device ROMTags */
	const uae_u8 diagarea[] = {
		0x90, 0x00, /* da_Config, da_Flags */
    0x02, 0x00, /* da_Size */
		FILESYS_DIAGPOINT >> 8, FILESYS_DIAGPOINT & 0xff,
		FILESYS_BOOTPOINT >> 8, FILESYS_BOOTPOINT & 0xff,
		0, (uae_u8)14, // Name offset
		0, 0, 0, 0,
		(uae_u8)('U'), (uae_u8)('A'), (uae_u8)('E'), 0
  };

  expamem_init_clear();
	expamem_write (0x00, Z2_MEM_64KB | zorroII | (rom ? rom_card : 0));

  expamem_write (0x08, no_shutup);

  expamem_write (0x04, 82);
  expamem_write (0x10, uae_id >> 8);
  expamem_write (0x14, uae_id & 0xff);

  expamem_write (0x18, 0x00); /* ser.no. Byte 0 */
  expamem_write (0x1c, 0x00); /* ser.no. Byte 1 */
  expamem_write (0x20, 0x00); /* ser.no. Byte 2 */
	expamem_write (0x24, 0x03); /* ser.no. Byte 3 */

  /* er_InitDiagVec */
	expamem_write (0x28, rom ? 0x20 : 0x00); /* ROM-Offset hi */
  expamem_write (0x2c, 0x00); /* ROM-Offset lo */

  expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/

	add_rtarea_pointer(aci);

	if (aci && !aci->doinit) {
		memcpy(aci->autoconfig_raw, expamem, sizeof aci->autoconfig_raw);
		return true;
	}

  /* Build a DiagArea */
	memcpy (expamem + FILESYS_DIAGAREA, diagarea, sizeof diagarea);

	put_word_host(expamem + FILESYS_DIAGAREA + FILESYS_DIAGPOINT + 0,
		0x7000 | 2); // MOVEQ #x,D0
	/* Call hwtrap_install */
	put_word_host(expamem + FILESYS_DIAGAREA + FILESYS_DIAGPOINT + 2, 0x4EB9); /* JSR */
	put_long_host(expamem + FILESYS_DIAGAREA + FILESYS_DIAGPOINT + 4, filesys_get_entry(9));
  /* Call DiagEntry */
	put_word_host(expamem + FILESYS_DIAGAREA + FILESYS_DIAGPOINT + 8, 0x4EF9); /* JMP */
	put_long_host(expamem + FILESYS_DIAGAREA + FILESYS_DIAGPOINT + 10, ROM_filesys_diagentry);

  /* What comes next is a plain bootblock */
	put_word_host(expamem + FILESYS_DIAGAREA + FILESYS_BOOTPOINT, 0x4EF9); /* JMP */
	put_long_host(expamem + FILESYS_DIAGAREA + FILESYS_BOOTPOINT + 2, EXPANSION_bootcode);
  
	set_ks12_boot_hack();

	return true;
}

#endif

/*
 * Zorro III expansion memory
 */

static addrbank *expamem_map_z3fastmem (struct autoconfig_info *aci)
{
	int devnum = aci->devnum;
	addrbank *ab = &z3fastmem_bank[devnum];
	uaecptr z3fs = expamem_board_pointer;
	uae_u32 size = currprefs.z3fastmem[devnum].size;

	if (ab->allocated_size) {
		map_banks_z3(ab, z3fs >> 16, size >> 16);
  }
	return ab;
}

static bool expamem_init_z3fastmem(struct autoconfig_info *aci)
{
	addrbank *bank = &z3fastmem_bank[aci->devnum];
	
	uae_u32 size = aci->prefs->z3fastmem[aci->devnum].size;

	aci->label = _T("Z3 Fast RAM");

	int code = (size == 0x100000 ? Z2_MEM_1MB
		: size == 0x200000 ? Z2_MEM_2MB
		: size == 0x400000 ? Z2_MEM_4MB
		: size == 0x800000 ? Z2_MEM_8MB
		: size == 0x1000000 ? Z3_MEM_16MB
		: size == 0x2000000 ? Z3_MEM_32MB
		: size == 0x4000000 ? Z3_MEM_64MB
		: size == 0x8000000 ? Z3_MEM_128MB
		: size == 0x10000000 ? Z3_MEM_256MB
		: size == 0x20000000 ? Z3_MEM_512MB
		: Z3_MEM_1GB);

	if (size < 0x1000000)
		code = Z3_MEM_16MB; /* Z3 physical board size is always at least 16M */

  expamem_init_clear();
	if (!fastmem_autoconfig(aci->prefs, aci, BOARD_AUTOCONFIG_Z3, add_memory | zorroIII | code, 1, size))
		aci->zorro = -1;

	memcpy(aci->autoconfig_raw, expamem, sizeof aci->autoconfig_raw);
	aci->addrbank = bank;

	if (!aci->doinit)
		return true;

	uae_u32 start = bank->start;
	bool alwaysmapz3 = aci->prefs->z3_mapping_mode != Z3MAPPING_REAL;
	if ((alwaysmapz3 || expamem_z3hack(aci->prefs)) && bank->allocated_size) {
	  map_banks_z3(bank, start >> 16, size >> 16);
  }
	return true;
}

#ifdef PICASSO96
/*
 *  Fake Graphics Card (ZORRO III) - BDK
 */

static addrbank *expamem_map_gfxcard_z3 (struct autoconfig_info *aci)
{
	int devnum = aci->devnum;
	gfxmem_banks[devnum]->start = expamem_board_pointer;
	map_banks_z3(gfxmem_banks[devnum], gfxmem_banks[devnum]->start >> 16, gfxmem_banks[devnum]->allocated_size >> 16);
	return gfxmem_banks[devnum];
}

static addrbank *expamem_map_gfxcard_z2 (struct autoconfig_info *aci)
{
	int devnum = aci->devnum;
	gfxmem_banks[devnum]->start = expamem_board_pointer;
	map_banks_z2(gfxmem_banks[devnum], gfxmem_banks[devnum]->start >> 16, gfxmem_banks[devnum]->allocated_size >> 16);
	return gfxmem_banks[devnum];
}

static bool expamem_init_gfxcard (struct autoconfig_info *aci, bool z3)
{
	int devnum = aci->devnum;
	struct uae_prefs *p = aci->prefs;
	int size = p->rtgboards[devnum].rtgmem_size;
  int code = (size == 0x100000 ? Z2_MEM_1MB
    : size == 0x200000 ? Z2_MEM_2MB
    : size == 0x400000 ? Z2_MEM_4MB
    : size == 0x800000 ? Z2_MEM_8MB
    : size == 0x1000000 ? Z3_MEM_16MB
    : size == 0x2000000 ? Z3_MEM_32MB
    : size == 0x4000000 ? Z3_MEM_64MB
  	: size == 0x8000000 ? Z3_MEM_128MB
  	: size == 0x10000000 ? Z3_MEM_256MB
  	: size == 0x20000000 ? Z3_MEM_512MB
  	: Z3_MEM_1GB);
  int subsize = (size == 0x100000 ? Z3_SS_MEM_1MB
  	: size == 0x200000 ? Z3_SS_MEM_2MB
  	: size == 0x400000 ? Z3_SS_MEM_4MB
  	: size == 0x800000 ? Z3_SS_MEM_8MB
  	: Z3_SS_MEM_SAME);

	aci->label = _T("UAE RTG");
	aci->direct_vram = true;

	if (size < 0x1000000 && z3)
		code = Z3_MEM_16MB; /* Z3 physical board size is always at least 16M */

  expamem_init_clear();
	expamem_write (0x00, (z3 ? zorroIII : zorroII) | code);

	expamem_write (0x08, care_addr | (z3 ? (force_z3 | (size > 0x800000 ? ext_size: subsize)) : 0));
  expamem_write (0x04, 96);

  expamem_write (0x10, uae_id >> 8);
  expamem_write (0x14, uae_id & 0xff);

  expamem_write (0x18, 0x00); /* ser.no. Byte 0 */
  expamem_write (0x1c, 0x00); /* ser.no. Byte 1 */
  expamem_write (0x20, 0x00); /* ser.no. Byte 2 */
  expamem_write (0x24, 0x01); /* ser.no. Byte 3 */

  expamem_write (0x28, 0x00); /* ROM-Offset hi */
  expamem_write (0x2c, 0x00); /* ROM-Offset lo */

  expamem_write (0x40, 0x00); /* Ctrl/Statusreg.*/

	memcpy(aci->autoconfig_raw, expamem, sizeof aci->autoconfig_raw);
	aci->addrbank = gfxmem_banks[devnum];
	return true;
}
static bool expamem_init_gfxcard_z3(struct autoconfig_info *aci)
{
	return expamem_init_gfxcard (aci, true);
}
static bool expamem_init_gfxcard_z2 (struct autoconfig_info *aci)
{
	return expamem_init_gfxcard (aci, false);
}
#endif


#ifdef SAVESTATE
static size_t fast_filepos[MAX_RAM_BOARDS], z3_filepos[MAX_RAM_BOARDS];
static size_t p96_filepos;
#endif

void free_fastmemory (int boardnum)
{
	mapped_free (&fastmem_bank[boardnum]);
}

static bool mapped_malloc_dynamic (uae_u32 *currpsize, uae_u32 *changedpsize, addrbank *bank, int max, const TCHAR *label)
{
	int alloc = *currpsize;

	bank->allocated_size = 0;
	bank->reserved_size = alloc;
	bank->baseaddr = NULL;
	bank->mask = 0;

	if (!alloc)
		return false;

	bank->mask = alloc - 1;
	bank->label = label ? label : _T("*");
	if (mapped_malloc (bank)) {
		*currpsize = alloc;
		*changedpsize = alloc;
		return true;
	}
	write_log (_T("Out of memory for %s, %d bytes.\n"), label ? label : _T("?"), alloc);

	return false;
}

static void allocate_expamem (void)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		memcpy(&currprefs.rtgboards[i], &changed_prefs.rtgboards[i], sizeof(struct rtgboardconfig));
	}

	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
    currprefs.fastmem[i].size = changed_prefs.fastmem[i].size;
    currprefs.z3fastmem[i].size = changed_prefs.z3fastmem[i].size;
  }

	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
    if (fastmem_bank[i].reserved_size != currprefs.fastmem[i].size) {
      free_fastmemory (i);

			if (fastmem_bank[i].start == 0xffffffff) {
				fastmem_bank[i].reserved_size = 0;
			} else {
      	fastmem_bank[i].reserved_size = currprefs.fastmem[i].size;
		    fastmem_bank[i].mask = fastmem_bank[i].reserved_size - 1;
      	if (fastmem_bank[i].reserved_size && fastmem_bank[i].start != 0xffffffff) {
			    mapped_malloc (&fastmem_bank[i]);
      		if (fastmem_bank[i].baseaddr == 0) {
      			write_log (_T("Out of memory for fastmem card.\n"));
      		}
      	}
			}
      memory_hardreset(1);
    }
  }

  if (z3fastmem_bank[0].reserved_size != currprefs.z3fastmem[0].size) {
		mapped_free (&z3fastmem_bank[0]);
		mapped_malloc_dynamic (&currprefs.z3fastmem[0].size, &changed_prefs.z3fastmem[0].size, &z3fastmem_bank[0], 1, _T("*"));
    memory_hardreset(1);
  }

#ifdef PICASSO96
	struct rtgboardconfig *rbc = &currprefs.rtgboards[0];
	if (gfxmem_banks[0]->reserved_size != rbc->rtgmem_size) {
		mapped_free (gfxmem_banks[0]);
		mapped_malloc_dynamic (&rbc->rtgmem_size, &changed_prefs.rtgboards[0].rtgmem_size, gfxmem_banks[0], 1, NULL);
    memory_hardreset(1);
  }
#endif

#ifdef SAVESTATE
  if (savestate_state == STATE_RESTORE) {
		for (int i = 0; i < MAX_RAM_BOARDS; i++) {
    	if (fastmem_bank[i].allocated_size > 0) {
				restore_ram (fast_filepos[i], fastmem_bank[i].baseaddr);
			  if (!fastmem_bank[i].start) {
				  // old statefile compatibility support
				  fastmem_bank[i].start = 0x00200000;
			  }
    		map_banks (&fastmem_bank[i], fastmem_bank[i].start >> 16, currprefs.fastmem[i].size >> 16,
    			fastmem_bank[i].allocated_size);
    	}
    	if (z3fastmem_bank[i].allocated_size > 0) {
				restore_ram (z3_filepos[i], z3fastmem_bank[i].baseaddr);
    		map_banks (&z3fastmem_bank[i], z3fastmem_bank[i].start >> 16, currprefs.z3fastmem[i].size >> 16,
    			z3fastmem_bank[i].allocated_size);
    	}
    }
#ifdef PICASSO96
		if (gfxmem_banks[0]->allocated_size > 0 && gfxmem_banks[0]->start > 0) {
			restore_ram (p96_filepos, gfxmem_banks[0]->baseaddr);
			map_banks(gfxmem_banks[0], gfxmem_banks[0]->start >> 16, currprefs.rtgboards[0].rtgmem_size >> 16,
				gfxmem_banks[0]->allocated_size);
  	}
#endif
  }
#endif /* SAVESTATE */
}

static uaecptr check_boot_rom(struct uae_prefs* p, int* boot_rom_type)
{
	uaecptr b = RTAREA_DEFAULT;
	addrbank* ab;

	*boot_rom_type = 0;
	if (p->boot_rom == 1)
		return 0;
	*boot_rom_type = 1;
	if (p->cs_cdtvcd || is_device_rom(p, ROMTYPE_CDTVSCSI, 0) >= 0 || p->uae_hide > 1)
		b = RTAREA_BACKUP;
	if (p->cs_mbdmac == 1 || p->cpuboard_type)
		b = RTAREA_BACKUP;
	ab = &get_mem_bank(RTAREA_DEFAULT);
	if (ab) {
		if (valid_address(RTAREA_DEFAULT, 65536))
			b = RTAREA_BACKUP;
	}
	if (nr_directory_units(NULL))
		return b;
	if (nr_directory_units(p))
		return b;
	if (p->socket_emu)
		return b;
	if (p->uaeserial)
		return b;
	if (p->scsi == 1)
		return b;
	if (p->sana2)
		return b;
	if (p->input_tablet > 0)
		return b;
	if (p->rtgboards[0].rtgmem_size)
		return b;
	if (p->chipmem_size > 2 * 1024 * 1024)
		return b;
	if (p->z3chipmem_size)
		return b;
	if (p->boot_rom >= 3)
		return b;
	if (p->boot_rom == 2 && b == 0xf00000) {
		*boot_rom_type = -1;
		return b;
	}
	*boot_rom_type = 0;
	return 0;
}

uaecptr need_uae_boot_rom(struct uae_prefs* p)
{
	uaecptr v;

	uae_boot_rom_type = 0;
	v = check_boot_rom(p, &uae_boot_rom_type);
	if (!rtarea_base) {
		uae_boot_rom_type = 0;
		v = 0;
	}
	return v;
}

static void add_expansions(struct uae_prefs *p, int zorro, int *fastmem_nump, int mode)
{
	int fastmem_num = MAX_RAM_BOARDS;
	if (fastmem_nump)
		fastmem_num = *fastmem_nump;
	for (int i = 0; expansionroms[i].name; i++) {
		const struct expansionromtype *ert = &expansionroms[i];
		if (ert->zorro == zorro) {
			for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
				struct romconfig *rc = get_device_romconfig(p, ert->romtype, j);
				if (rc) {
					if (mode == 2)
						continue;
					cards_set[cardno].flags = 0;
					cards_set[cardno].name = ert->name;
					cards_set[cardno].initrc = ert->init;
					cards_set[cardno].rc = rc;
					cards_set[cardno].zorro = zorro;
					cards_set[cardno].ert = ert;
					cards_set[cardno++].map = NULL;
				}
			}
		}
	}
	if (fastmem_nump)
		*fastmem_nump = fastmem_num;
}

uae_u32 expansion_board_size(addrbank* ab)
{
	uae_u32 size = 0;
	uae_u8 code = (ab->bget(0) & 0xf0) | ((ab->bget(2) & 0xf0) >> 4);
	if ((code & 0xc0) == zorroII) {
		// Z2
		code &= 7;
		if (code == 0)
			size = 8 * 1024 * 1024;
		else
			size = 32768 << code;
	}
	return size;
}

static uae_u8 autoconfig_read(const uae_u8 *autoconfig, int offset)
{
	uae_u8 b = (autoconfig[offset] & 0xf0) | (autoconfig[offset + 2] >> 4);
	if (offset == 0 || offset == 2 || offset == 0x40 || offset == 0x42)
		return b;
	b = ~b;
	return b;
}

static void expansion_parse_autoconfig(struct card_data *cd, const uae_u8 *autoconfig)
{
	uae_u8 code = autoconfig[0];
	uae_u32 expamem_z3_size;

	if ((code & 0xc0) == zorroII) {
		int slotsize;
		// Z2
		cd->zorro = 2;
		code &= 7;
		if (code == 0)
			expamem_board_size = 8 * 1024 * 1024;
		else
			expamem_board_size = 32768 << code;
		slotsize = expamem_board_size / 65536;

		expamem_board_pointer = 0xffffffff;

		for (int slottype = 0; slottype < 2; slottype++)
		{
			uae_u8 *slots = slots_e8;
			int numslots = sizeof slots_e8;
			uaecptr slotaddr = AUTOCONFIG_Z2;
			if (slotsize >= 8 || slottype > 0) {
				slots = slots_20;
				numslots = sizeof slots_20;
				slotaddr = AUTOCONFIG_Z2_MEM;
			}
			for (int i = 0; i < numslots; i++) {
				if (((slotsize - 1) & i) == 0) {
					bool free = true;
					for (int j = 0; j < slotsize && j + i < numslots; j++) {
						if (slots[i + j] != 0) {
							free = false;
							break;
						}
					}
					if (free) {
						for (int j = 0; j < slotsize; j++) {
							slots[i + j] = 1;
						}
						expamem_board_pointer = slotaddr + i * 65536;
						break;
					}
				}
			}
			if (expamem_board_pointer != 0xffffffff || slotsize >= 8)
				break;
		}

	} else if ((code & 0xc0) == zorroIII) {
		// Z3

		cd->zorro = 3;
		code &= 7;
		if (autoconfig[2] & ext_size)
			expamem_z3_size = (16 * 1024 * 1024) << code;
		else
			expamem_z3_size = 16 * 1024 * 1024;

		uaecptr newp = (expamem_z3_pointer_real + expamem_z3_size - 1) & ~(expamem_z3_size - 1);
		if (newp < expamem_z3_pointer_real)
			newp = 0xffffffff;
		expamem_z3_pointer_real = newp;

		expamem_board_pointer = expamem_z3hack(cd->aci.prefs) ? expamem_z3_pointer_uae : expamem_z3_pointer_real;
		expamem_board_size = expamem_z3_size;

	} else if ((code & 0xc0) == 0x40) {
		cd->zorro = 1;
		// 0x40 = "Box without init/diagnostic code"
		// proto autoconfig "box" size.
		//expamem_z2_size = (1 << ((code >> 3) & 7)) * 4096;
		// much easier this way, all old-style boards were made for
		// A1000 and didn't have passthrough connector.
		expamem_board_size = 65536;
		expamem_board_pointer = 0xe90000;
	}

}

static void reset_ac_data(struct uae_prefs *p)
{
	expamem_z3_pointer_real = Z3BASE_REAL;
	expamem_z3_pointer_uae = Z3BASE_UAE;
	expamem_z3_highram_real = 0;
	expamem_z3_highram_uae = 0;

	if (p->mbresmem_high_size >= 128 * 1024 * 1024)
		expamem_z3_pointer_uae += (p->mbresmem_high_size - 128 * 1024 * 1024) + 16 * 1024 * 1024;
	expamem_board_pointer = 0;
	expamem_board_size = 0;
	memset(slots_20, 0, sizeof slots_20);
	memset(slots_e8, 0, sizeof slots_e8);
	slots_e8[0] = 1;
}

static void reset_ac(struct uae_prefs *p)
{
	do_mount = 1;

	if (need_uae_boot_rom(p) == 0)
    do_mount = 0;
	if (uae_boot_rom_type <= 0)
		do_mount = 0;

  /* check if Kickstart version is below 1.3 */
	if (ks12orolder() && do_mount) {
    /* warn user */
#if KS12_BOOT_HACK
		do_mount = -1;
#else
    write_log (_T("Kickstart version is below 1.3!  Disabling automount devices.\n"));
    do_mount = 0;
#endif
  }

	uae_id = hackers_id;

	if (restore_cardno == 0) {
	  for (int i = 0; i < MAX_EXPANSION_BOARD_SPACE; i++) {
		  memset(&cards_set[i], 0, sizeof(struct card_data));
	  }
  }

	ecard = 0;
	cardno = 0;

	reset_ac_data(p);
}

void expansion_generate_autoconfig_info(struct uae_prefs *p)
{
	expansion_scan_autoconfig(p, true);
}

struct autoconfig_info* expansion_get_autoconfig_data(struct uae_prefs* p, int index)
{
	if (index >= cardno)
		return NULL;
	struct card_data* cd = cards[index];
	return &cd->aci;
}

struct autoconfig_info* expansion_get_autoconfig_by_address(struct uae_prefs* p, uaecptr addr, int index)
{
	if (index >= cardno)
		return NULL;
	for (int i = index; i < cardno; i++) {
		struct card_data *cd = cards[i];
		if (addr >= cd->base && addr < cd->base + cd->size)
			return &cd->aci;
	}
	return NULL;
}

static void expansion_init_cards(struct uae_prefs *p)
{
	reset_ac_data(p);

	for (int i = 0; i < cardno; i++) {
		bool ok;
		struct card_data *cd = &cards_set[i];
		struct autoconfig_info *aci = &cd->aci;
		memset(aci->autoconfig_raw, 0xff, sizeof aci->autoconfig_raw);
		cd->base = 0;
		cd->size = 0;
		aci->devnum = (cd->flags >> 16) & 255;
		aci->prefs = p;
		aci->ert = cd->ert;
		aci->start = 0xffffffff;
		aci->zorro = cd->zorro;
		if (cd->initnum) {
			ok = cd->initnum(aci);
		} else {
			aci->rc = cd->rc;
			ok = cd->initrc(aci);
		}
		
		if ((aci->zorro == 1 || aci->zorro == 2 || aci->zorro == 3) && aci->addrbank != &expamem_null && (aci->autoconfig_raw[0] != 0xff || aci->autoconfigp)) {
			uae_u8 ac2[16];
			const uae_u8 *a = aci->autoconfigp;
			if (!a) {
				for (int i = 0; i < 16; i++) {
					ac2[i] = autoconfig_read(aci->autoconfig_raw, i * 4);
				}
				a = ac2;
			}
			expansion_parse_autoconfig(cd, a);
			cd->size = expamem_board_size;
		}
	}
}

static int get_order(struct uae_prefs *p, struct card_data *cd)
{
	if (!cd)
		return EXPANSION_ORDER_MAX - 1;
	if (cd->aci.get_params) {
		struct expansion_params parms;
		if (cd->aci.get_params(p, &parms))
			return parms.device_order;
	}
	if (cd->zorro <= 0)
		return -1;
	if (cd->zorro >= 4)
		return -2;
	if (cd->rc)
		return cd->rc->back->device_order;
	int devnum = (cd->flags >> 16) & 255;
	if (!_tcsicmp(cd->name, _T("Z2Fast")))
		return p->fastmem[devnum].device_order;
	if (!_tcsicmp(cd->name, _T("Z3Fast")))
		return p->z3fastmem[devnum].device_order;
	if (!_tcsicmp(cd->name, _T("Z3RTG")) || !_tcsicmp(cd->name, _T("Z2RTG")))
		return p->rtgboards[devnum].device_order;
	if (!_tcsicmp(cd->name, _T("MegaChipRAM")))
		return -1;
	return EXPANSION_ORDER_MAX - 1;
}

static void expansion_parse_cards(struct uae_prefs *p, bool log)
{
	if (log)
		write_log(_T("Autoconfig board list:\n"));
	reset_ac_data(p);
	for (int i = 0; i < cardno; i++) {
		bool ok;
		struct card_data *cd = cards[i];
		struct autoconfig_info *aci = &cd->aci;
		memset(aci->autoconfig_raw, 0xff, sizeof aci->autoconfig_raw);
		memset(aci->autoconfig_bytes, 0xff, sizeof aci->autoconfig_bytes);
		cd->base = 0;
		cd->size = 0;
		aci->devnum = (cd->flags >> 16) & 255;
		aci->prefs = p;
		aci->ert = cd->ert;
		aci->zorro = cd->zorro;
		aci->start = 0xffffffff;
		if (cd->initnum) {
			ok = cd->initnum(aci);
		} else {
			aci->rc = cd->rc;
			ok = cd->initrc(aci);
		}
		if (log)
			write_log(_T("Card %02d: "), i + 1);
		if (ok) {
			TCHAR label[MAX_DPATH];
			label[0] = 0;
			if (cd->rc && !label[0]) {
				const struct expansionromtype *ert = get_device_expansion_rom(cd->rc->back->device_type);
				if (ert) {
					_tcscpy(label, ert->friendlyname);
				}
			}
			if (!label[0]) {
				if (aci->label) {
					_tcscpy(label, aci->label);
				} else if (aci->addrbank && aci->addrbank->label) {
					_tcscpy(label, aci->addrbank->label);
				} else {
					_tcscpy(label, _T("<no name>"));
				}
			}
			if (aci->devnum > 0) {
				TCHAR *s = label + _tcslen(label);
				_stprintf(s, _T(" [%d]"), aci->devnum + 1);
			}

			if ((aci->zorro == 1 || aci->zorro == 2 || aci->zorro == 3) && (aci->autoconfig_raw[0] != 0xff || aci->autoconfigp)) {
				uae_u8 ac2[16];
				const uae_u8 *a = aci->autoconfigp;
				if (!a) {
					for (int i = 0; i < 16; i++) {
						ac2[i] = autoconfig_read(aci->autoconfig_raw, i * 4);
					}
					a = ac2;
				}
				if (log) {
					write_log(_T("'%s'\n"), label);
					write_log(_T("  %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n"),
						a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
						a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);
					write_log(_T("  MID %u (%04x) PID %u (%02x) SER %08x\n"),
						(a[4] << 8) | a[5], (a[4] << 8) | a[5],
						a[1], a[1],
						(a[6] << 24) | (a[7] << 16) | (a[8] << 8) | a[9]);
				}
				expansion_parse_autoconfig(cd, a);
				uae_u32 size = expamem_board_size;
				TCHAR sizemod = 'K';
				uae_u8 type = a[0];
				size /= 1024;
				if (size > 8 * 1024) {
					sizemod = 'M';
					size /= 1024;
				}
				bool z3 = (type & 0xc0) == zorroIII;
				if (log) {
					write_log(_T("  Z%d 0x%08x 0x%08x %4d%c %s %d\n"),
						(type & 0xc0) == zorroII ? 2 : (z3 ? 3 : 1),
						z3 && !currprefs.z3autoconfig_start ? expamem_z3_pointer_real : expamem_board_pointer,
						z3 && !currprefs.z3autoconfig_start ? expamem_z3_pointer_uae : expamem_board_pointer,
						size, sizemod,
						type & rom_card ? _T("ROM") : (type & add_memory ? _T("RAM") : _T("IO ")), get_order(p, cd));
				}
				cd->base = expamem_board_pointer;
				cd->size = expamem_board_size;
				if ((type & 0xc0) == zorroIII) {
					aci->start = expamem_z3hack(p) ? expamem_z3_pointer_uae : expamem_z3_pointer_real;
				} else {
					aci->start = expamem_board_pointer;
				}
				aci->size = cd->size;
				memcpy(aci->autoconfig_bytes, a, sizeof aci->autoconfig_bytes);
				if (aci->addrbank) {
					aci->addrbank->start = expamem_board_pointer;
					if (aci->addrbank->reserved_size == 0 && !(type & add_memory) && expamem_board_size < 524288) {
						aci->addrbank->reserved_size = expamem_board_size;
					}
				}
				aci->zorro = cd->zorro;
				if (cd->zorro == 3) {
					if (expamem_z3_pointer_real + expamem_board_size < expamem_z3_pointer_real) {
						expamem_z3_pointer_real = 0xffffffff;
						expamem_z3_highram_real = 0xffffffff;
					} else {
						expamem_z3_pointer_real += expamem_board_size;
					}
					if (expamem_z3_pointer_uae + expamem_board_size < expamem_z3_pointer_uae) {
						expamem_z3_pointer_uae = 0xffffffff;
						expamem_z3_highram_uae = 0xffffffff;
					} else {
						expamem_z3_pointer_uae += expamem_board_size;
					}
					if (expamem_board_pointer + expamem_board_size < expamem_board_pointer) {
						expamem_board_pointer = 0xffffffff;
					} else {
						expamem_board_pointer += expamem_board_size;
					}
					if ((type & add_memory) || aci->direct_vram) {
						if (expamem_z3_pointer_uae != 0xffffffff && expamem_z3_pointer_uae > expamem_z3_highram_uae)
							expamem_z3_highram_uae = expamem_z3_pointer_uae;
						if (expamem_z3_pointer_real != 0xffffffff && expamem_z3_pointer_real > expamem_z3_highram_real)
							expamem_z3_highram_real = expamem_z3_pointer_real;
					}

				}
			} else {
				cd->base = aci->start;
				cd->size = aci->size;
				if (log)
					write_log(_T("'%s' no autoconfig %08x - %08x.\n"), aci->label ? aci->label : _T("<no name>"), cd->base, cd->base + cd->size - 1);
			}
			_tcscpy(aci->name, label);
		} else {
			if (log)
				write_log(_T("init failed.\n"), i);
		}
	}
	if (log)
		write_log(_T("END\n"));
}

static bool add_card_sort(int index, bool *inuse, int *new_cardnop)
{
	struct card_data *cd = &cards_set[index];
	int new_cardno = *new_cardnop;
	cards[new_cardno++] = cd;
	inuse[index] = true;
	*new_cardnop = new_cardno;
	return true;
}

static void expansion_autoconfig_sort(struct uae_prefs *p)
{
	const int zs[] = { BOARD_NONAUTOCONFIG_BEFORE, 0, BOARD_PROTOAUTOCONFIG, BOARD_AUTOCONFIG_Z2, BOARD_NONAUTOCONFIG_AFTER_Z2, BOARD_AUTOCONFIG_Z3, BOARD_NONAUTOCONFIG_AFTER_Z3, -1 };
	bool inuse[MAX_EXPANSION_BOARD_SPACE];
	struct card_data *tcards[MAX_EXPANSION_BOARD_SPACE];
	int new_cardno = 0;

	// default sort first, sets correct parent/child order
	for (int i = 0; i < cardno; i++) {
		inuse[i] = false;
		cards[i] = NULL;
	}
	cards[cardno] = NULL;
	for (int type = 0; zs[type] >= 0; type++) {
		bool changed = true;
		int z = zs[type];
		bool inuse2[MAX_EXPANSION_BOARD_SPACE];
		memset(inuse2, 0, sizeof inuse2);
		while (changed) {
			changed = false;
			// unmovables first
			int testorder = 0;
			int idx = -1;
			for (int i = 0; i < cardno; i++) {
				if (inuse[i])
					continue;
				if (inuse2[i])
					continue;
				struct card_data *cd = &cards_set[i];
				if (cd->zorro != z)
					continue;
				int order = get_order(p, cd);
				if (order >= 0)
					continue;
				if (testorder > order) {
					testorder = order;
					idx = i;
				}
			}
			if (idx >= 0) {
				inuse2[idx] = true;
				add_card_sort(idx, inuse, &new_cardno);
				changed = true;
			}
		}
		changed = true;
		memset(inuse2, 0, sizeof inuse2);
		while (changed) {
			changed = false;
			for (int i = 0; i < cardno; i++) {
				if (inuse[i])
					continue;
				if (inuse2[i])
					continue;
				struct card_data *cd = &cards_set[i];
				if (cd->zorro != z)
					continue;
				if (get_order(p, cd) < EXPANSION_ORDER_MAX - 1)
					continue;
				inuse2[i] = true;
				add_card_sort(i, inuse, &new_cardno);
				changed = true;
			}
		}
		changed = true;
		memset(inuse2, 0, sizeof inuse2);
		while (changed) {
			// the rest
			changed = false;
			for (int i = 0; i < cardno; i++) {
				if (inuse[i])
					continue;
				if (inuse2[i])
					continue;
				struct card_data *cd = &cards_set[i];
				if (cd->zorro != z)
					continue;
				inuse2[i] = true;
				add_card_sort(i, inuse, &new_cardno);
				changed = true;
			}
		}
	}
	for (int i = 0; i < cardno; i++) {
		if (inuse[i])
			continue;
		cards[new_cardno++] = &cards_set[i];
	}
	for (int i = 0; i < cardno; i++) {
		struct autoconfig_info *aci = &cards[i]->aci;
		tcards[i] = cards[i];
	}

	new_cardno = 0;

	// re-sort by board size
	for (int idx = 0; idx < cardno; idx++) {
		struct card_data *cd = tcards[idx];
		if (!cd)
			continue;
		int z = cd->zorro;
		if (z != 2 && z != 3) {
			cards[new_cardno++] = cd;
			tcards[idx] = NULL;
		}
	}
	for (int z = 2; z <= 3; z++) {
		for (;;) {
			int idx2 = -1;
			uae_u32 size = 0;
			for (int j = 0; j < cardno; j++) {
				struct card_data *cdc = tcards[j];
				if (cdc && cdc->size > size && cdc->zorro == z) {
					size = cdc->size;
					idx2 = j;
				}
			}
			if (idx2 < 0)
				break;
			cards[new_cardno++] = tcards[idx2];
			tcards[idx2] = NULL;
		}
	}

	for (int i = 0; i < cardno; i++) {
		if (tcards[i]) {
			cards[new_cardno++] = tcards[i];
		}
	}
}

static void expansion_add_autoconfig(struct uae_prefs *p)
{
	int fastmem_num;

	reset_ac(p);

	// add possible non-autoconfig boards
	add_expansions(p, BOARD_NONAUTOCONFIG_BEFORE, NULL, 0);

	fastmem_num = 0;
	add_expansions(p, BOARD_PROTOAUTOCONFIG, &fastmem_num, 0);
	// immediately after Z2Fast so that it can be emulated as A590/A2091 with fast ram.
	add_expansions(p, BOARD_AUTOCONFIG_Z2, &fastmem_num, 0);

	add_expansions(p, BOARD_NONAUTOCONFIG_AFTER_Z2, NULL, 0);

	while (fastmem_num < MAX_RAM_BOARDS) {
		if (p->fastmem[fastmem_num].size) {
			cards_set[cardno].flags = fastmem_num << 16;
			cards_set[cardno].name = _T("Z2Fast");
			cards_set[cardno].zorro = 2;
			cards_set[cardno].initnum = expamem_init_fastcard;
			cards_set[cardno++].map = expamem_map_fastcard;
		}
		fastmem_num++;
  }

#ifdef FILESYS
	if (do_mount && p->uaeboard >= 0) {
		cards_set[cardno].flags = CARD_FLAG_UAEROM;
		cards_set[cardno].name = _T("UAEFS");
		cards_set[cardno].zorro = 2;
		cards_set[cardno].initnum = expamem_init_filesys;
		cards_set[cardno++].map = expamem_map_filesys;
	}
	if (p->uaeboard > 0) {
		cards_set[cardno].flags = CARD_FLAG_UAEROM;
		cards_set[cardno].name = _T("UAEBOARD");
		cards_set[cardno].zorro = 2;
		cards_set[cardno].initnum = expamem_init_uaeboard;
		cards_set[cardno++].map = expamem_map_uaeboard;
	}
	if (do_mount) {
		cards_set[cardno].flags = CARD_FLAG_UAEROM;
		cards_set[cardno].name = _T("UAEBOOTROM");
		cards_set[cardno].zorro = BOARD_NONAUTOCONFIG_BEFORE;
		cards_set[cardno].initnum = expamem_rtarea_init;
		cards_set[cardno++].map = NULL;
	}
#endif
#ifdef PICASSO96
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtgboardconfig *rbc = &p->rtgboards[i];
		if (rbc->rtgmem_size && rbc->rtgmem_type == GFXBOARD_UAE_Z2) {
			cards_set[cardno].flags = 4 | (i << 16);
			cards_set[cardno].zorro = 2;
			cards_set[cardno].name = _T("Z2RTG");
			cards_set[cardno].initnum = expamem_init_gfxcard_z2;
			cards_set[cardno++].map = expamem_map_gfxcard_z2;
		}
  }
#endif

	/* Z3 boards last */

	fastmem_num = 0;

	// add combo Z3 boards (something + Z3 RAM)
	add_expansions(p, BOARD_AUTOCONFIG_Z3, &fastmem_num, 2);

	// add remaining RAM boards
	for (int i = fastmem_num; i < MAX_RAM_BOARDS; i++) {
		if (p->z3fastmem[i].size) {
			z3num = 0;
			cards_set[cardno].flags = (2 | CARD_FLAG_CAN_Z3) | (i << 16);
			cards_set[cardno].name = _T("Z3Fast");
			cards_set[cardno].zorro = 3;
			cards_set[cardno].initnum = expamem_init_z3fastmem;
			cards_set[cardno++].map = expamem_map_z3fastmem;
		}
	}
#ifdef PICASSO96
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		if (p->rtgboards[i].rtgmem_size && p->rtgboards[i].rtgmem_type == GFXBOARD_UAE_Z3) {
			cards_set[cardno].flags = 4 | CARD_FLAG_CAN_Z3 | (i << 16);
			cards_set[cardno].name = _T("Z3RTG");
			cards_set[cardno].zorro = 3;
			cards_set[cardno].initnum = expamem_init_gfxcard_z3;
			cards_set[cardno++].map = expamem_map_gfxcard_z3;
		}
  }
#endif

	// add non-memory Z3 boards
	add_expansions(p, BOARD_AUTOCONFIG_Z3, NULL, 1);

	add_expansions(p, BOARD_NONAUTOCONFIG_AFTER_Z3, NULL, 0);
}

void expansion_scan_autoconfig(struct uae_prefs *p, bool log)
{
	cfgfile_compatibility_romtype(p);
	expansion_add_autoconfig(p);
	expansion_init_cards(p);
	expansion_autoconfig_sort(p);
	expansion_parse_cards(p, log);
}

void expamem_reset (int hardreset)
{
	reset_ac(&currprefs);

	chipdone = false;

	allocate_expamem ();
	expamem_bank.name = _T("Autoconfig [reset]");

	expansion_add_autoconfig(&currprefs);
	expansion_init_cards(&currprefs);
	expansion_autoconfig_sort(&currprefs);
	expansion_parse_cards(&currprefs, true);

	// this also resets all autoconfig devices
	devices_reset_ext(hardreset);

	if (cardno == 0 || savestate_state) {
		expamem_init_clear_zero ();
	} else {
		set_ks12_boot_hack();
		call_card_init(0);
	}
}

void expansion_init(void)
{
	if (savestate_state != STATE_RESTORE) {

		for (int i = 0; i < MAX_RAM_BOARDS; i++) {
			fastmem_bank[i].reserved_size = 0;
			fastmem_bank[i].mask = 0;
			fastmem_bank[i].baseaddr = NULL;
		}

#ifdef PICASSO96
		for (int i = 0; i < MAX_RTG_BOARDS; i++) {
			gfxmem_banks[i]->reserved_size = 0;
			gfxmem_banks[i]->mask = 0;
			gfxmem_banks[i]->baseaddr = NULL;
		}
#endif

		for (int i = 0; i < MAX_RAM_BOARDS; i++) {
			z3fastmem_bank[i].reserved_size = 0;
			z3fastmem_bank[i].mask = 0;
			z3fastmem_bank[i].baseaddr = NULL;
		}

		//z3chipmem_bank.reserved_size = 0;
		//z3chipmem_bank.mask = z3chipmem_bank.start = 0;
		//z3chipmem_bank.baseaddr = NULL;
	}

	allocate_expamem();

	if (currprefs.uaeboard) {
		uaeboard_bank.reserved_size = 0x10000;
		mapped_malloc(&uaeboard_bank);
	}

}

void expansion_cleanup(void)
{
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		mapped_free(&fastmem_bank[i]);
		mapped_free(&z3fastmem_bank[i]);
	}
	//mapped_free(&z3chipmem_bank);
	
#ifdef PICASSO96
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		mapped_free(gfxmem_banks[i]);
	}
#endif

#ifdef FILESYS
	mapped_free(&filesys_bank);
#endif
	if (currprefs.uaeboard) {
		mapped_free(&uaeboard_bank);
	}
}

void expansion_map(void)
{
	map_banks(&expamem_bank, 0xE8, 1, 0);
	if (do_mount < 0 && ks11orolder()) {
		filesys_bank.start = 0xe90000;
		mapped_free(&filesys_bank);
		mapped_malloc(&filesys_bank);
		map_banks_z2(&filesys_bank, filesys_bank.start >> 16, 1);
		expamem_init_filesys(0);
		expamem_map_filesys_update();
	}
}

static void clear_bank (addrbank *ab)
{
	if (!ab->baseaddr || !ab->allocated_size)
		return;
	memset (ab->baseaddr, 0, ab->allocated_size > 0x800000 ? 0x800000 : ab->allocated_size);
}

void expansion_clear(void)
{
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
	  clear_bank (&fastmem_bank[i]);
	  clear_bank (&z3fastmem_bank[i]);
  }
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		clear_bank (gfxmem_banks[i]);
	}
}

#ifdef SAVESTATE

/* State save/restore code.  */

uae_u8 *save_fram (int *len, int num)
{
  *len = fastmem_bank[num].allocated_size;
  return fastmem_bank[num].baseaddr;
}

uae_u8 *save_zram (int *len, int num)
{
  *len = z3fastmem_bank[num].allocated_size;
  return z3fastmem_bank[num].baseaddr;
}

uae_u8 *save_pram (int *len)
{
	*len = gfxmem_banks[0]->allocated_size;
	return gfxmem_banks[0]->baseaddr;
}

void restore_fram (int len, size_t filepos, int num)
{
	fast_filepos[num] = filepos;
  changed_prefs.fastmem[num].size = len;
}

void restore_zram (int len, size_t filepos, int num)
{
	z3_filepos[num] = filepos;
  changed_prefs.z3fastmem[num].size = len;
}

void restore_pram (int len, size_t filepos)
{
  p96_filepos = filepos;
	changed_prefs.rtgboards[0].rtgmem_size = len;
}

uae_u8 *save_expansion (int *len, uae_u8 *dstptr)
{
	uae_u8 *dst, *dstbak;
  if (dstptr)
  	dst = dstbak = dstptr;
  else
		dstbak = dst = xmalloc (uae_u8, 6 * 4);
  save_u32 (fastmem_bank[0].start);
  save_u32 (z3fastmem_bank[0].start);
	save_u32 (gfxmem_banks[0]->start);
  save_u32 (rtarea_base);
  *len = dst - dstbak;
  return dstbak;
}

uae_u8 *restore_expansion (uae_u8 *src)
{
	fastmem_bank[0].start = restore_u32 ();
  z3fastmem_bank[0].start = restore_u32 ();
	gfxmem_banks[0]->start = restore_u32 ();
  rtarea_base = restore_u32 ();
  if (rtarea_base != 0 && rtarea_base != RTAREA_DEFAULT && rtarea_base != RTAREA_BACKUP && (rtarea_base < 0xe90000 || rtarea_base >= 0xf00000))
  	rtarea_base = 0;
  return src;
}

uae_u8 *save_expansion_boards(int *len, uae_u8 *dstptr, int cardnum)
{
	uae_u8 *dst, *dstbak;
	if (cardnum >= cardno)
		return NULL;
	if (dstptr)
		dst = dstbak = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);
	save_u32(3);
	save_u32(0);
	save_u32(cardnum);
	struct card_data *ec = cards[cardnum];
	save_u32(ec->base);
	save_u32(ec->size);
	save_u32(ec->flags);
	save_string(ec->name);
	for (int j = 0; j < 16; j++) {
		save_u8(ec->aci.autoconfig_bytes[j]);
	}
	struct romconfig *rc = ec->rc;
	if (rc) {
		save_u32(rc->back->device_type);
		save_u32(rc->back->device_num);
		save_string(rc->romfile);
		save_string(rc->romident);
	} else {
		save_u32(0xffffffff);
	}
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_expansion_boards(uae_u8 *src)
{
	if (!src) {
		restore_cardno = 0;
		return NULL;
	}
	TCHAR *s;
	uae_u32 flags = restore_u32();
	if (!(flags & 2))
		return src;
	restore_u32();
	int cardnum = restore_u32();
	restore_cardno = cardnum + 1;
	struct card_data *ec = &cards_set[cardnum];
	cards[cardnum] = ec;

	ec->base = restore_u32();
	ec->size = restore_u32();
	ec->flags = restore_u32();
	s = restore_string();
	xfree(s);
	for (int j = 0; j < 16; j++) {
		ec->aci.autoconfig_bytes[j] = restore_u8();
	}

	uae_u32 dev_num = 0;
	uae_u32 romtype = restore_u32();
	if (romtype != 0xffffffff) {
		dev_num = restore_u32();
		ec->aci.devnum = dev_num;
		struct boardromconfig* brc = get_device_rom(&currprefs, romtype, dev_num, NULL);
		if (!brc) {
			brc = get_device_rom_new(&currprefs, romtype, dev_num, NULL);
		}
		struct romconfig *rc = get_device_romconfig(&currprefs, romtype, dev_num);
		if (rc) {
			ec->rc = rc;
			rc->back = brc;
			ec->ert = get_device_expansion_rom(romtype);
			s = restore_string();
			_tcscpy(rc->romfile, s);
			xfree(s);
			s = restore_string();
			_tcscpy(rc->romident, s);
			xfree(s);
	  }
	}
	return src;
}

void restore_expansion_finish(void)
{
	cardno = restore_cardno;
	for (int i = 0; i < cardno; i++) {
		struct card_data* ec = &cards_set[i];
		cards[i] = ec;
		struct romconfig* rc = ec->rc;
		expamem_board_pointer = ec->base;
		// Handle only IO boards, RAM boards are handled differently
		ec->aci.doinit = false;
		ec->aci.start = ec->base;
		ec->aci.size = ec->size;
		ec->aci.prefs = &currprefs;
		ec->aci.ert = ec->ert;
		ec->aci.rc = rc;
		if (rc && ec->ert) {
			_tcscpy(ec->aci.name, ec->ert->friendlyname);
			if (ec->ert->init) {
				if (ec->ert->init(&ec->aci)) {
					if (ec->aci.addrbank) {
						map_banks(ec->aci.addrbank, ec->base >> 16, ec->size >> 16, 0);
					}
				}
			}
			ec->aci.doinit = true;
		}
	}
}

#endif /* SAVESTATE */

static const struct expansionboardsettings cdtvsram_settings[] = {
	{
		_T("SRAM size\0") _T("64k\0") _T("128k\0") _T("256k\0"),
		_T("sram\0") _T("64k\0") _T("128k\0") _T("256k\0"),
		true
	},
	{
		NULL
	}
};

const struct expansionromtype expansionroms[] = {
	//{
	//	_T("cpuboard"), _T("Accelerator"), _T("Accelerator"),
	//	NULL, NULL, NULL, add_cpuboard_unit, ROMTYPE_CPUBOARD, 0, 0, 0, true,
	//	NULL, 0,
	//	false, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_IDE
	//},

	/* built-in controllers */
	{
		_T("cd32fmv"), _T("CD32 FMV"), _T("Commodore"),
		NULL, expamem_init_cd32fmv, NULL, NULL, ROMTYPE_CD32CART, 0, 0, BOARD_AUTOCONFIG_Z2, true,
		NULL, 0,
		false, EXPANSIONTYPE_INTERNAL
	},
	{
		_T("cdtvdmac"), _T("CDTV DMAC"), _T("Commodore"),
		NULL, cdtv_init, NULL, NULL, ROMTYPE_CDTVDMAC | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
		NULL, 0,
		false, EXPANSIONTYPE_INTERNAL
	},
	//{
	//	_T("cdtvscsi"), _T("CDTV SCSI"), _T("Commodore"),
	//	NULL, cdtvscsi_init, NULL, cdtv_add_scsi_unit, ROMTYPE_CDTVSCSI | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_AFTER_Z2, true,
	//	NULL, 0,
	//	false, EXPANSIONTYPE_INTERNAL | EXPANSIONTYPE_SCSI
	//},
	{
		_T("cdtvsram"), _T("CDTV SRAM"), _T("Commodore"),
		NULL, cdtvsram_init, NULL, NULL, ROMTYPE_CDTVSRAM | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
		NULL, 0,
		false, EXPANSIONTYPE_INTERNAL,
		0, 0, 0, false, NULL,
		false, 0, cdtvsram_settings
	},
	{
		_T("cdtvcr"), _T("CDTV-CR"), _T("Commodore"),
		NULL, cdtvcr_init, NULL, NULL, ROMTYPE_CDTVCR | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_AFTER_Z2, true,
		NULL, 0,
		false, EXPANSIONTYPE_INTERNAL
	},
	//{
	//	_T("scsi_a3000"), _T("A3000 SCSI"), _T("Commodore"),
	//	NULL, a3000scsi_init, NULL, a3000_add_scsi_unit, ROMTYPE_SCSI_A3000 | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_AFTER_Z2, true,
	//	NULL, 0,
	//	false, EXPANSIONTYPE_INTERNAL | EXPANSIONTYPE_SCSI
	//},
	//{
	//	_T("scsi_a4000t"), _T("A4000T SCSI"), _T("Commodore"),
	//	NULL, a4000t_scsi_init, NULL, a4000t_add_scsi_unit, ROMTYPE_SCSI_A4000T | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
	//	NULL, 0,
	//	false, EXPANSIONTYPE_INTERNAL | EXPANSIONTYPE_SCSI
	//},
	{
		_T("ide_mb"), _T("A600/A1200/A4000 IDE"), _T("Commodore"),
		NULL, gayle_ide_init, NULL, gayle_add_ide_unit, ROMTYPE_MB_IDE | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
		NULL, 0,
		false, EXPANSIONTYPE_INTERNAL | EXPANSIONTYPE_IDE,
		0, 0, 0, false, NULL, false, 1
	},
	{
		_T("pcmcia_mb"), _T("A600/A1200 PCMCIA"), _T("Commodore"),
		NULL, gayle_init_pcmcia, NULL, NULL, ROMTYPE_MB_PCMCIA | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
		NULL, 0,
		false, EXPANSIONTYPE_INTERNAL
	},

	/* PCI Bridgeboards */
//
//	{
//		_T("grex"), _T("G-REX"), _T("DCE"),
//		NULL, grex_init, NULL, NULL, ROMTYPE_GREX | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_PCI_BRIDGE
//	},
//	{
//		_T("mediator"), _T("Mediator"), _T("Elbox"),
//		NULL, mediator_init, mediator_init2, NULL, ROMTYPE_MEDIATOR | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		mediator_sub, 0,
//		false, EXPANSIONTYPE_PCI_BRIDGE,
//		0, 0, 0, false, NULL,
//		false, 0, mediator_settings
//	},
//	{
//		_T("prometheus"), _T("Prometheus"), _T("Matay"),
//		NULL, prometheus_init, NULL, NULL, ROMTYPE_PROMETHEUS | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z3, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_PCI_BRIDGE,
//		0, 0, 0, false, NULL,
//		false, 0, bridge_settings
//	},
//
//	/* SCSI/IDE expansion */
//
//	{
//		_T("pcmciaide"), _T("PCMCIA IDE"), NULL,
//		NULL, gayle_init_board_io_pcmcia, NULL, NULL, ROMTYPE_PCMCIAIDE | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_IDE | EXPANSIONTYPE_PCMCIA,
//	},
//	{
//		_T("apollo"), _T("Apollo 500/2000"), _T("3-State"),
//		NULL, apollo_init_hd, NULL, apollo_add_scsi_unit, ROMTYPE_APOLLOHD, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_IDE,
//		8738, 0, 0
//	},
//	{
//		_T("add500"), _T("ADD-500"), _T("Archos"),
//		NULL, add500_init, NULL, add500_add_scsi_unit, ROMTYPE_ADD500, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		8498, 27, 0, true, NULL
//	},
//	{
//		_T("overdrivehd"), _T("Overdrive HD"), _T("Archos"),
//		NULL, gayle_init_board_common_pcmcia, NULL, NULL, ROMTYPE_ARCHOSHD, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_IDE | EXPANSIONTYPE_PCMCIA,
//	},
//	{
//		_T("addhard"), _T("AddHard"), _T("Ashcom Design"),
//		NULL, addhard_init, NULL, addhard_add_scsi_unit, ROMTYPE_ADDHARD, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 0x30, 0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 },
//	},
//	{
//		_T("blizzardscsikitiii"), _T("SCSI Kit III"), _T("Phase 5"),
//		NULL, NULL, NULL, cpuboard_ncr9x_add_scsi_unit, ROMTYPE_BLIZKIT3, 0, 0, 0, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("blizzardscsikitiv"), _T("SCSI Kit IV"), _T("Phase 5"),
//		NULL, NULL, NULL, cpuboard_ncr9x_add_scsi_unit, ROMTYPE_BLIZKIT4, 0, 0, 0, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("accessx"), _T("AccessX"), _T("Breitfeld Computersysteme"),
//		NULL, accessx_init, NULL, accessx_add_ide_unit, ROMTYPE_ACCESSX, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		accessx_sub, 0,
//		true, EXPANSIONTYPE_IDE,
//		0, 0, 0, true, NULL,
//		false, 2
//	},
//	{
//		_T("oktagon2008"), _T("Oktagon 2008"), _T("BSC/Alfa Data"),
//		NULL, ncr_oktagon_autoconfig_init, NULL, oktagon_add_scsi_unit, ROMTYPE_OKTAGON, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("alfapower"), _T("AlfaPower/AT-Bus 2008"), _T("BSC/Alfa Data"),
//		NULL, alf_init, NULL, alf_add_ide_unit, ROMTYPE_ALFA, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_IDE,
//		2092, 8, 0
//	},
//	{
//		_T("alfapowerplus"), _T("AlfaPower Plus"), _T("BSC/Alfa Data"),
//		NULL, alf_init, NULL, alf_add_ide_unit, ROMTYPE_ALFAPLUS, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_IDE,
//		2092, 8, 0
//	},
//	{
//		_T("tandem"), _T("Tandem"), _T("BSC"),
//		NULL, tandem_init, NULL, tandem_add_ide_unit, ROMTYPE_TANDEM | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_IDE,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xc1, 6, 0x00, 0x00, 0x08, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
//	},
//	{
//		_T("malibu"), _T("Malibu"), _T("California Access"),
//		NULL, malibu_init, NULL, malibu_add_scsi_unit, ROMTYPE_MALIBU, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 0x01, 0x00, 0x00, 0x08, 0x11, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00 },
//	},
//	{
//		_T("cltda1000scsi"), _T("A1000/A2000 SCSI"), _T("C-Ltd"),
//		NULL, cltda1000scsi_init, NULL, cltda1000scsi_add_scsi_unit, ROMTYPE_CLTDSCSI | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xc1, 0x0c, 0x00, 0x00, 0x03, 0xec, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
//	},
//	{
//		_T("a2090a"), _T("A2090a"), _T("Commodore"),
//		NULL, a2090_init, NULL, a2090_add_scsi_unit, ROMTYPE_A2090 | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_CUSTOM_SECONDARY,
//		0, 0, 0, false, NULL,
//		false, 0, a2090a_settings
//	},
//	{
//		_T("a2090b"), _T("A2090 Combitec"), _T("Commodore"),
//		a2090b_preinit, a2090b_init, NULL, a2090_add_scsi_unit, ROMTYPE_A2090B | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_CUSTOM_SECONDARY,
//		0, 0, 0, false, NULL,
//		false, 0, a2090a_settings
//	},
//	{
//		_T("a2091"), _T("A590/A2091"), _T("Commodore"),
//		NULL, a2091_init, NULL, a2091_add_scsi_unit, ROMTYPE_A2091 | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		a2091_sub, 1,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_CUSTOM_SECONDARY,
//		commodore, commodore_a2091, 0, true, NULL
//	},
//	{
//		_T("a4091"), _T("A4091"), _T("Commodore"),
//		NULL, ncr710_a4091_autoconfig_init, NULL, a4091_add_scsi_unit, ROMTYPE_A4091, 0, 0, BOARD_AUTOCONFIG_Z3, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		true, 0, a4091_settings
//	},
//	{
//		_T("comspec"), _T("SA series"), _T("Comspec Communications"),
//		comspec_preinit, comspec_init, NULL, comspec_add_scsi_unit, ROMTYPE_COMSPEC, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		comspec_sub, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, comspec_settings
//	},
//	{
//		_T("rapidfire"), _T("RapidFire/SpitFire"), _T("DKB"),
//		NULL, ncr_rapidfire_init, NULL, rapidfire_add_scsi_unit, ROMTYPE_RAPIDFIRE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		2012, 1, 0, true, NULL,
//		true, 0, NULL,
//		{ 0xd2, 0x0f ,0x00, 0x00, 0x07, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
//	},
//	{
//		_T("fastata4000"), _T("FastATA 4000"), _T("Elbox"),
//		NULL, fastata4k_init, NULL, fastata4k_add_ide_unit, ROMTYPE_FASTATA4K, 0, 0, BOARD_AUTOCONFIG_Z3, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_IDE,
//		0, 0, 0, true, NULL,
//		true, 2, fastata_settings,
//		{ 0x90, 0, 0x10, 0x00, 0x08, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 },
//	},
//	{
//		_T("elsathd"), _T("Mega Ram HD"), _T("Elsat"),
//		NULL, elsathd_init, NULL, elsathd_add_ide_unit, ROMTYPE_ELSATHD, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_IDE,
//		17740, 1, 0
//	},
//	{
//		_T("eveshamref"), _T("Reference 40/100"), _T("Evesham Micros"),
//		NULL, eveshamref_init, NULL, eveshamref_add_scsi_unit, ROMTYPE_EVESHAMREF, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		8504, 2, 0
//	},
//	{
//		_T("dataflyerscsiplus"), _T("DataFlyer SCSI+"), _T("Expansion Systems"),
//		NULL, dataflyer_init, NULL, dataflyer_add_scsi_unit, ROMTYPE_DATAFLYERP | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("dataflyerplus"), _T("DataFlyer Plus"), _T("Expansion Systems"),
//		NULL, dataflyerplus_init, NULL, dataflyerplus_add_idescsi_unit, ROMTYPE_DATAFLYER, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_IDE,
//		0, 0, 0, false, NULL,
//		false, 0, dataflyersplus_settings
//	},
//	{
//		_T("arriba"), _T("Arriba"), _T("Gigatron"),
//		NULL, arriba_init, NULL, arriba_add_ide_unit, ROMTYPE_ARRIBA | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_IDE,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 0x01, 0x00, 0x00, 0x08, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
//	},
//	{
//		_T("gvp1"), _T("GVP Series I"), _T("Great Valley Products"),
//		NULL, gvp_init_s1, NULL, gvp_s1_add_scsi_unit, ROMTYPE_GVPS1 | ROMTYPE_NONE, ROMTYPE_GVPS12, 0, BOARD_AUTOCONFIG_Z2, false,
//		gvp1_sub, 1,
//		true, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("gvp"), _T("GVP Series II"), _T("Great Valley Products"),
//		NULL, gvp_init_s2, NULL, gvp_s2_add_scsi_unit, ROMTYPE_GVPS2 | ROMTYPE_NONE, ROMTYPE_GVPS12, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		2017, 10, 0
//	},
//	{
//		_T("dotto"), _T("Dotto"), _T("Hardital"),
//		NULL, dotto_init, NULL, dotto_add_ide_unit, ROMTYPE_DOTTO, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_IDE
//	},
//	{
//		_T("vector"), _T("Vector Falcon 8000"), _T("HK-Computer"),
//		NULL, vector_init, NULL, vector_add_scsi_unit, ROMTYPE_VECTOR, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("surfsquirrel"), _T("Surf Squirrel"), _T("HiSoft"),
//		NULL, gayle_init_board_io_pcmcia, NULL, NULL, ROMTYPE_SSQUIRREL | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_PCMCIA,
//	},
//	{
//		_T("adide"), _T("AdIDE"), _T("ICD"),
//		NULL, adide_init, NULL, adide_add_ide_unit, ROMTYPE_ADIDE | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_IDE
//	},
//	{
//		_T("adscsi2000"), _T("AdSCSI Advantage 2000/2080"), _T("ICD"),
//		NULL, adscsi_init, NULL, adscsi_add_scsi_unit, ROMTYPE_ADSCSI, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		2071, 4, 0, false, NULL,
//		true, 0, adscsi2000_settings
//	},
//	{
//		_T("trifecta"), _T("Trifecta"), _T("ICD"),
//		NULL, trifecta_init, NULL, trifecta_add_idescsi_unit, ROMTYPE_TRIFECTA | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		trifecta_sub, 0,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_IDE,
//		2071, 32, 0, false, NULL,
//		true, 0, trifecta_settings,
//		{ 0xd1, 0x23, 0x40, 0x00, 0x08, 0x17, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 }
//	},
//	{
//		_T("buddha"), _T("Buddha"), _T("Individual Computers"),
//		NULL, buddha_init, NULL, buddha_add_ide_unit, ROMTYPE_BUDDHA, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_IDE,
//		0, 0, 0, false, NULL,
//		false, 4, buddha_settings,
//		{ 0xd1, 0x00, 0x00, 0x00, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 },
//	},
//	{
//		_T("trumpcard"), _T("Trumpcard"), _T("IVS"),
//		NULL, trumpcard_init, NULL, trumpcard_add_scsi_unit, ROMTYPE_IVSTC, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		2112, 4, 0, false, NULL,
//		true, 0, NULL,
//		{  0xd1, 0x30, 0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 },
//	},
//	{
//		_T("trumpcardpro"), _T("Grand Slam"), _T("IVS"),
//		NULL, trumpcardpro_init, NULL, trumpcardpro_add_scsi_unit, ROMTYPE_IVSTPRO, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		2112, 4, 0, false, NULL,
//		true, 0, NULL,
//		{  0xd1, 0x34, 0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 },
//	},
//	{
//		_T("trumpcardat"), _T("Trumpcard 500AT"), _T("IVS"),
//		NULL, trumpcard500at_init, NULL, trumpcard500at_add_ide_unit, ROMTYPE_IVST500AT, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_IDE,
//		2112, 4, 0, false, NULL,
//		true, 0, NULL,
//		{ 0xd1, 0x31, 0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 },
//	},
//	{
//		_T("kommos"), _T("Kommos A500/A2000 SCSI"), _T("Jürgen Kommos"),
//		NULL, kommos_init, NULL, kommos_add_scsi_unit, ROMTYPE_KOMMOS, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("golem"), _T("HD3000"), _T("Kupke"),
//		NULL, hd3000_init, NULL, hd3000_add_scsi_unit, ROMTYPE_GOLEMHD3000, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("golem"), _T("Golem SCSI II"), _T("Kupke"),
//		NULL, golem_init, NULL, golem_add_scsi_unit, ROMTYPE_GOLEM, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		2079, 3, 0
//	},
//	{
//		_T("golemfast"), _T("Golem Fast SCSI/IDE"), _T("Kupke"),
//		NULL, golemfast_init, NULL, golemfast_add_idescsi_unit, ROMTYPE_GOLEMFAST, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_IDE,
//		0, 0, 0, false, NULL,
//		true, 0, golemfast_settings
//	},
//	{
//		_T("multievolution"), _T("Multi Evolution 500/2000"), _T("MacroSystem"),
//		NULL, ncr_multievolution_init, NULL, multievolution_add_scsi_unit, ROMTYPE_MEVOLUTION, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		18260, 8, 0, true
//	},
//	{
//		_T("scram8490"), _T("SCRAM (DP8490V)"), _T("MegaMicro"),
//		NULL, scram5380_init, NULL, scram5380_add_scsi_unit, ROMTYPE_SCRAM5380, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		4096, 4, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 3, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00 }
//	},
//	{
//		_T("scram5394"), _T("SCRAM (NCR53C94)"), _T("MegaMicro"),
//		NULL, ncr_scram5394_init, NULL, scram5394_add_scsi_unit, ROMTYPE_SCRAM5394, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		4096, 4, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 7, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00 }
//	},
//	{
//		_T("paradox"), _T("Paradox SCSI"), _T("Mainhattan Data"),
//		NULL, paradox_init, NULL, paradox_add_scsi_unit, ROMTYPE_PARADOX | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_PARALLEL_ADAPTER
//	},
//	{
//		_T("ateam"), _T("A-Team"), _T("Mainhattan Data"),
//		NULL, ateam_init, NULL, ateam_add_ide_unit, ROMTYPE_ATEAM, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_IDE
//	},
//	{
//		_T("mtecat"), _T("AT 500"), _T("M-Tec"),
//		NULL, mtec_init, NULL, mtec_add_ide_unit, ROMTYPE_MTEC, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_IDE
//	},
//	{
//		_T("mtecmastercard"), _T("Mastercard"), _T("M-Tec"),
//		NULL, ncr_mtecmastercard_init, NULL, mtecmastercard_add_scsi_unit, ROMTYPE_MASTERCARD, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("masoboshi"), _T("MasterCard"), _T("Masoboshi"),
//		NULL, masoboshi_init, NULL, masoboshi_add_idescsi_unit, ROMTYPE_MASOBOSHI | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		masoboshi_sub, 0,
//		true, EXPANSIONTYPE_SCSI | EXPANSIONTYPE_IDE
//	},
//	{
//		_T("hardframe"), _T("HardFrame"), _T("Microbotics"),
//		NULL, hardframe_init, NULL, hardframe_add_scsi_unit, ROMTYPE_HARDFRAME, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		true
//	},
//	{
//		_T("stardrive"), _T("StarDrive"), _T("Microbotics"),
//		NULL, stardrive_init, NULL, stardrive_add_scsi_unit, ROMTYPE_STARDRIVE | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		1010, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xc1, 2, 0x00, 0x00, 0x03, 0xf2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
//
//	},
//	{
//		_T("filecard2000"), _T("Filecard 2000/OSSI 500"), _T("Otronic"),
//		NULL, ossi_init, NULL, ossi_add_scsi_unit, ROMTYPE_OSSI, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xc1, 1, 0x00, 0x00, 0x07, 0xf4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//	},
//	{
//		_T("pacificoverdrive"), _T("Overdrive"), _T("Pacific Peripherals/IVS"),
//		NULL, overdrive_init, NULL, overdrive_add_scsi_unit, ROMTYPE_OVERDRIVE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 16, 0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 }
//	},
//	{
//		_T("fastlane"), _T("Fastlane"), _T("Phase 5"),
//		NULL, ncr_fastlane_autoconfig_init, NULL, fastlane_add_scsi_unit, ROMTYPE_FASTLANE, 0, 0, BOARD_AUTOCONFIG_Z3, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		8512, 10, 0, false, fastlane_memory_callback
//	},
//	{
//		_T("phoenixboard"), _T("Phoenix Board SCSI"), _T("Phoenix Microtechnologies"),
//		NULL, phoenixboard_init, NULL, phoenixboard_add_scsi_unit, ROMTYPE_PHOENIXB, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//	},
//	{
//		_T("ptnexus"), _T("Nexus"), _T("Preferred Technologies"),
//		NULL, ptnexus_init, NULL, ptnexus_add_scsi_unit, ROMTYPE_PTNEXUS | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		2102, 4, 0, false, nexus_memory_callback,
//		false, 0, nexus_settings,
//		{ 0xd1, 0x01, 0x00, 0x00, 0x08, 0x36, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 },
//	},
//	{
//		_T("profex"), _T("HD 3300"), _T("Profex Electronics"),
//		NULL, profex_init, NULL, profex_add_scsi_unit, ROMTYPE_PROFEX, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		true, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("protar"), _T("A500 HD"), _T("Protar"),
//		NULL, protar_init, NULL, protar_add_ide_unit, ROMTYPE_PROTAR, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		4149, 51, 0
//	},
//	{
//		_T("rochard"), _T("RocHard RH800C"), _T("Roctec"),
//		NULL, rochard_init, NULL, rochard_add_idescsi_unit, ROMTYPE_ROCHARD | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		rochard_sub, 0,
//		true, EXPANSIONTYPE_IDE | EXPANSIONTYPE_SCSI,
//		2144, 2, 0, false, NULL,
//		false, 2, NULL
//	},
//	{
//		_T("inmate"), _T("InMate"), _T("Spirit Technology"),
//		NULL, inmate_init, NULL, inmate_add_scsi_unit, ROMTYPE_INMATE | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, inmate_settings
//	},
//	{
//		_T("supradrive"), _T("SupraDrive"), _T("Supra Corporation"),
//		NULL, supra_init, NULL, supra_add_scsi_unit, ROMTYPE_SUPRA | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		supra_sub, 0,
//		true, EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("emplant"), _T("Emplant (SCSI only)"), _T("Utilities Unlimited"),
//		NULL, emplant_init, NULL, emplant_add_scsi_unit, ROMTYPE_EMPLANT | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 0x15, 0x40, 0x00, 0x08, 0x7b, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x00 },
//	},
//#if 0 /* driver is MIA, 3rd party ScottDevice driver is not enough for full implementation. */
//	{
//		NULL, _T("microforge"), _T("Hard Disk"), _T("Micro Forge"),
//		microforge_init, NULL, microforge_add_scsi_unit, ROMTYPE_MICROFORGE | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SASI | EXPANSIONTYPE_SCSI
//	},
//#endif
//
//	{
//		_T("omtiadapter"), _T("OMTI-Adapter"), _T("C't"),
//		NULL, omtiadapter_init, NULL, omtiadapter_scsi_unit, ROMTYPE_OMTIADAPTER | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("alf1"), _T("A.L.F."), _T("Elaborate Bytes"),
//		NULL, alf1_init, NULL, alf1_add_scsi_unit, ROMTYPE_ALF1 | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("alf3"), _T("A.L.F.3"), _T("Elaborate Bytes"),
//		NULL, ncr_alf3_autoconfig_init, NULL, alf3_add_scsi_unit, ROMTYPE_ALF3 | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		true, 0, alf3_settings
//	},
//	{
//		_T("promigos"), _T("Promigos"), _T("Flesch und Hörnemann"),
//		NULL, promigos_init, NULL, promigos_add_scsi_unit, ROMTYPE_PROMIGOS | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("wedge"), _T("Wedge"), _T("Reiter Software"),
//		wedge_preinit, wedge_init, NULL, wedge_add_scsi_unit, ROMTYPE_WEDGE | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("tecmar"), _T("T-Card/T-Disk"), _T("Tecmar"),
//		NULL, tecmar_init, NULL, tecmar_add_scsi_unit, ROMTYPE_TECMAR | ROMTYPE_NOT, 0, 0, BOARD_PROTOAUTOCONFIG, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SASI | EXPANSIONTYPE_SCSI,
//		1001, 1, 0
//	},
//	{
//		_T("system2000"), _T("System 2000"), _T("Vortex"),
//		system2000_preinit, system2000_init, NULL, system2000_add_scsi_unit, ROMTYPE_SYSTEM2000 | ROMTYPE_NONE, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("xebec"), _T("9720H"), _T("Xebec"),
//		NULL, xebec_init, NULL, xebec_add_scsi_unit, ROMTYPE_XEBEC | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SASI | EXPANSIONTYPE_SCSI
//	},
//	{
//		_T("kronos"), _T("Kronos"), _T("C-Ltd"),
//		NULL, kronos_init, NULL, kronos_add_scsi_unit, ROMTYPE_KRONOS, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xd1, 0x04, 0x40, 0x00, 0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 },
//	},
//	{
//		_T("hda506"), _T("HDA-506"), _T("Spirit Technology"),
//		NULL, hda506_init, NULL, hda506_add_scsi_unit, ROMTYPE_HDA506 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xc1, 0x04, 0x00, 0x00, 0x07, 0xf2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//	},
//	{
//		_T("fasttrak"), _T("FastTrak"), _T("Xetec"),
//		NULL, fasttrak_init, NULL, fasttrak_add_scsi_unit, ROMTYPE_FASTTRAK, 0, 0, BOARD_AUTOCONFIG_Z2, false,
//		NULL, 0,
//		true, EXPANSIONTYPE_SCSI,
//		2022, 2, 0
//	},
//	{
//		_T("amax"), _T("AMAX ROM dongle"), _T("ReadySoft"),
//		NULL, NULL, NULL, NULL, ROMTYPE_AMAX | ROMTYPE_NONE, 0, 0, 0, false
//	},
//	{
//		_T("x86athdprimary"), _T("AT IDE Primary"), NULL,
//		NULL, x86_at_hd_init_1, NULL, x86_add_at_hd_unit_1, ROMTYPE_X86_AT_HD1 | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_AFTER_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_EXPANSION | EXPANSIONTYPE_IDE,
//	},
//	{
//		_T("x86athdxt"), _T("XTIDE Universal BIOS HD"), NULL,
//		NULL, x86_at_hd_init_xt, NULL, x86_add_at_hd_unit_xt, ROMTYPE_X86_XT_IDE | ROMTYPE_NONE, 0, 0, BOARD_NONAUTOCONFIG_AFTER_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_EXPANSION | EXPANSIONTYPE_IDE,
//		0, 0, 0, false, NULL,
//		false, 0, x86_athdxt_settings
//	},
//	{
//		_T("x86rt1000"), _T("Rancho RT1000"), NULL,
//		NULL, x86_rt1000_init, NULL, x86_rt1000_add_unit, ROMTYPE_X86_RT1000 | ROMTYPE_NONE, 0, 0, BOARD_NONAUTOCONFIG_AFTER_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_EXPANSION | EXPANSIONTYPE_SCSI,
//		0, 0, 0, false, NULL,
//		false, 0, x86_rt1000_settings
//
//	},
//
//	/* PC Bridgeboards */
//
//	{
//		_T("a1060"), _T("A1060 Sidecar"), _T("Commodore"),
//		NULL, a1060_init, NULL, NULL, ROMTYPE_A1060 | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_BRIDGE,
//		0, 0, 0, false, NULL,
//		false, 0, x86_bridge_sidecar_settings
//	},
//	{
//		_T("a2088"), _T("A2088"), _T("Commodore"),
//		NULL, a2088xt_init, NULL, NULL, ROMTYPE_A2088 | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_BRIDGE,
//		0, 0, 0, false, NULL,
//		false, 0, x86_bridge_settings
//	},
//	{
//		_T("a2088t"), _T("A2088T"), _T("Commodore"),
//		NULL, a2088t_init, NULL, NULL, ROMTYPE_A2088T | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_BRIDGE,
//		0, 0, 0, false, NULL,
//		false, 0, x86_bridge_settings
//	},
//	{
//		_T("a2286"), _T("A2286"), _T("Commodore"),
//		NULL, a2286_init, NULL, NULL, ROMTYPE_A2286 | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_BRIDGE,
//		0, 0, 0, false, NULL,
//		false, 0, x86at286_bridge_settings
//	},
//	{
//		_T("a2386"), _T("A2386SX"), _T("Commodore"),
//		NULL, a2386_init, NULL, NULL, ROMTYPE_A2386 | ROMTYPE_NONE, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_BRIDGE,
//		0, 0, 0, false, NULL,
//		false, 0, x86at386_bridge_settings
//	},
//
//	// only here for rom selection and settings
//	{
//		_T("picassoiv"), _T("Picasso IV"), _T("Village Tronic"),
//		NULL, NULL, NULL, NULL, ROMTYPE_PICASSOIV | ROMTYPE_NONE, 0, 0, BOARD_IGNORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_RTG
//	},
//	{
//		_T("x86vga"), _T("x86 VGA"), NULL,
//		NULL, NULL, NULL, NULL, ROMTYPE_x86_VGA | ROMTYPE_NONE, 0, 0, BOARD_IGNORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_EXPANSION,
//		0, 0, 0, false, NULL,
//		false, 0, x86vga_settings
//	},
//	{
//		_T("harlequin"), _T("Harlequin"), _T("ACS"),
//		NULL, NULL, NULL, NULL, ROMTYPE_HARLEQUIN | ROMTYPE_NOT, 0, 0, BOARD_IGNORE, false,
//		NULL, 0,
//		false, EXPANSIONTYPE_RTG,
//		0, 0, 0, false, NULL,
//		false, 0, harlequin_settings
//	},
//
//	/* Sound Cards */
//	{
//		_T("prelude"), _T("Prelude"), _T("Albrecht Computer Technik"),
//		NULL, prelude_init, NULL, NULL, ROMTYPE_PRELUDE | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND,
//		0, 0, 0, false, NULL,
//		false, 0, toccata_soundcard_settings,
//		{ 0xc1, 1, 0, 0, 0x42, 0x31, 0, 0, 0, 3 }
//	},
//	{
//		_T("prelude1200"), _T("Prelude 1200"), _T("Albrecht Computer Technik"),
//		NULL, prelude1200_init, NULL, NULL, ROMTYPE_PRELUDE1200 | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND,
//		0, 0, 0, false, NULL,
//		false, 0, toccata_soundcard_settings
//	},
//	{
//		_T("toccata"), _T("Toccata"), _T("MacroSystem"),
//		NULL, toccata_init, NULL, NULL, ROMTYPE_TOCCATA | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND,
//		0, 0, 0, false, NULL,
//		false, 0, toccata_soundcard_settings,
//		{ 0xc1, 12, 0, 0, 18260 >> 8, 18260 & 255 }
//	},
//	{
//		_T("es1370"), _T("ES1370 PCI"), _T("Ensoniq"),
//		NULL, pci_expansion_init, NULL, NULL, ROMTYPE_ES1370 | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND
//	},
//	{
//		_T("fm801"), _T("FM801 PCI"), _T("Fortemedia"),
//		NULL, pci_expansion_init, NULL, NULL, ROMTYPE_FM801 | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND
//	},
//	{
//		_T("uaesnd_z2"), _T("UAESND Z2"), NULL,
//		NULL, uaesndboard_init_z2, NULL, NULL, ROMTYPE_UAESNDZ2 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xc1, 2, 0x00, 0x00, 6502 >> 8, 6502 & 255, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//	},
//	{
//		_T("uaesnd_z3"), _T("UAESND Z3"), NULL,
//		NULL, uaesndboard_init_z3, NULL, NULL, ROMTYPE_UAESNDZ3 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z3, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0x80, 2, 0x10, 0x00, 6502 >> 8, 6502 & 255, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//	},
//	{
//		_T("sb_isa"), _T("SoundBlaster ISA (Creative)"), NULL,
//		NULL, isa_expansion_init, NULL, NULL, ROMTYPE_SBISA | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_X86_EXPANSION | EXPANSIONTYPE_SOUND,
//		0, 0, 0, false, NULL,
//		false, 0, sb_isa_settings
//	},
//
//
//#if 0
//	{
//		_T("pmx"), _T("pmx"), NULL,
//		NULL, pmx_init, NULL, NULL, ROMTYPE_PMX | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//		NULL, 0,
//		false, EXPANSIONTYPE_SOUND,
//		0, 0, 0, false, NULL,
//		false, 0, NULL,
//		{ 0xc1, 0x30, 0x00, 0x00, 0x0e, 0x3b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//	},
//#endif
//
//	/* Network */
//{
//	_T("a2065"), _T("A2065"), _T("Commodore"),
//	NULL, a2065_init, NULL, NULL, ROMTYPE_A2065 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//	{ 0xc1, 0x70, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//},
//{
//	_T("ariadne"), _T("Ariadne"), _T("Village Tronic"),
//	NULL, ariadne_init, NULL, NULL, ROMTYPE_ARIADNE | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//	{ 0xc1, 0xc9, 0x00, 0x00, 2167 >> 8, 2167 & 255, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//},
//{
//	_T("ariadne2"), _T("Ariadne II"), _T("Village Tronic"),
//	NULL, ariadne2_init, NULL, NULL, ROMTYPE_ARIADNE2 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//	{ 0xc1, 0xca, 0x00, 0x00, 2167 >> 8, 2167 & 255, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//},
//{
//	_T("hydra"), _T("AmigaNet"), _T("Hydra Systems"),
//	NULL, hydra_init, NULL, NULL, ROMTYPE_HYDRA | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//	{ 0xc1, 0x01, 0x00, 0x00, 2121 >> 8, 2121 & 255, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//},
//{
//	_T("eb920"), _T("LAN Rover/EB920"), _T("ASDG"),
//	NULL, lanrover_init, NULL, NULL, ROMTYPE_LANROVER | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, lanrover_settings,
//	{ 0xc1, 0xfe, 0x00, 0x00, 1023 >> 8, 1023 & 255, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//},
//{
//	_T("xsurf"), _T("X-Surf"), _T("Individual Computers"),
//	NULL, xsurf_init, NULL, NULL, ROMTYPE_XSURF | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//	{ 0xc1, 0x17, 0x00, 0x00, 4626 >> 8, 4626 & 255, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
//},
//{
//	_T("xsurf100z2"), _T("X-Surf-100 Z2"), _T("Individual Computers"),
//	NULL, xsurf100_init, NULL, NULL, ROMTYPE_XSURF100Z2 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//	{ 0xc1, 0x64, 0x10, 0x00, 4626 >> 8, 4626 & 255, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00 }
//},
//{
//	_T("xsurf100z3"), _T("X-Surf-100 Z3"), _T("Individual Computers"),
//	NULL, xsurf100_init, NULL, NULL, ROMTYPE_XSURF100Z3 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z3, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//	{ 0x82, 0x64, 0x32, 0x00, 4626 >> 8, 4626 & 255, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00 }
//},
//{
//	_T("ne2000pcmcia"), _T("RTL8019 PCMCIA (NE2000 compatible)"), NULL,
//	NULL, gayle_init_board_io_pcmcia, NULL, NULL, ROMTYPE_NE2KPCMCIA | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET | EXPANSIONTYPE_PCMCIA,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//},
//{
//	_T("ne2000_pci"), _T("RTL8029 PCI (NE2000 compatible)"), NULL,
//	NULL, pci_expansion_init, NULL, NULL, ROMTYPE_NE2KPCI | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET,
//	0, 0, 0, false, NULL,
//	false, 0, ethernet_settings,
//},
//{
//	_T("ne2000_isa"), _T("RTL8019 ISA (NE2000 compatible)"), NULL,
//	NULL, isa_expansion_init, NULL, NULL, ROMTYPE_NE2KISA | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_NET | EXPANSIONTYPE_X86_EXPANSION,
//	0, 0, 0, false, NULL,
//	false, 0, ne2k_isa_settings
//},
//
///* Catweasel */
//{
//	_T("catweasel"), _T("Catweasel"), _T("Individual Computers"),
//	NULL, expamem_init_catweasel, NULL, NULL, ROMTYPE_CATWEASEL | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_FLOPPY
//},
//
//// misc
//
//{
//	_T("pcmciasram"), _T("PCMCIA SRAM"), NULL,
//	NULL, gayle_init_board_common_pcmcia, NULL, NULL, ROMTYPE_PCMCIASRAM | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_CUSTOM | EXPANSIONTYPE_PCMCIA | EXPANSIONTYPE_CUSTOMDISK,
//},
//{
//	_T("uaeboard_z2"), _T("UAEBOARD Z2"), NULL,
//	NULL, uaesndboard_init_z2, NULL, NULL, ROMTYPE_UAEBOARDZ2 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z2, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_CUSTOM
//},
//{
//	_T("uaeboard_z3"), _T("UAEBOARD Z3"), NULL,
//	NULL, uaesndboard_init_z3, NULL, NULL, ROMTYPE_UAEBOARDZ3 | ROMTYPE_NOT, 0, 0, BOARD_AUTOCONFIG_Z3, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_CUSTOM
//},
//{
//	_T("cubo"), _T("Cubo CD32"), NULL,
//	NULL, cubo_init, NULL, NULL, ROMTYPE_CUBO | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_CUSTOM,
//	0, 0, 0, false, NULL,
//	false, 0, cubo_settings,
//},
//{
//	_T("x86_mouse"), _T("x86 Bridgeboard mouse"), NULL,
//	NULL, isa_expansion_init, NULL, NULL, ROMTYPE_X86MOUSE | ROMTYPE_NOT, 0, 0, BOARD_NONAUTOCONFIG_BEFORE, true,
//	NULL, 0,
//	false, EXPANSIONTYPE_X86_EXPANSION,
//	0, 0, 0, false, NULL,
//	false, 0, x86_mouse_settings
//},


{
	NULL
}
};

