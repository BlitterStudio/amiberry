/*
* UAE - The Un*x Amiga Emulator
*
* Misc accelerator board special features
* Blizzard 1230 IV, 1240/1260, 2040/2060, PPC
* CyberStorm MK1, MK2, MK3, PPC.
* TekMagic
* Warp Engine
*
* And more.
*
* Copyright 2014 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "zfile.h"
#include "rommgr.h"
#include "autoconf.h"
#include "cpuboard.h"
#include "custom.h"
#include "newcpu.h"
#include "ncr_scsi.h"
#include "ncr9x_scsi.h"
#include "debug.h"
#include "flashrom.h"
#include "uae.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "uae/vm.h"
#include "idecontrollers.h"
#include "scsi.h"
#include "cpummu030.h"
#include "devices.h"

// ROM expansion board diagrom call
// 00F83B7C 3.1 A4000
// 00F83B7C 3.0 A1200
// 00F83C96 3.1 A1200
// 00FC4E28 1.3

#define MAPROM_DEBUG 0
#define PPC_IRQ_DEBUG 0
#define CPUBOARD_IO_LOG 0
#define CPUBOARD_IRQ_LOG 0

#define F0_WAITSTATES (2 * CYCLE_UNIT)

// CS MK3/PPC
#define CYBERSTORM_MAPROM_BASE 0xfff00000
#define CSIII_NCR			0xf40000
#define CSIII_BASE			0xf60000
#define CSIII_REG_RESET		0x00 // 0x00
#define CSIII_REG_IRQ		0x01 // 0x08
#define CSIII_REG_WAITSTATE	0x02 // 0x10
#define CSIII_REG_SHADOW	0x03 // 0x18
#define CSIII_REG_LOCK		0x04 // 0x20
#define CSIII_REG_INT		0x05 // 0x28
#define CSIII_REG_IPL_EMU	0x06 // 0x30
#define CSIII_REG_INT_LVL	0x07 // 0x38

// BPPC only
#define BPPC_MAPROM_ON		0x12
#define BPPC_MAPROM_OFF		0x13
#define BPPC_UNLOCK_FLASH	0x92
#define BPPC_LOCK_FLASH		0x93
#define BPPC_MAGIC_UNLOCK_VALUE 0x42

/* bit definitions */
#define	P5_SET_CLEAR		0x80

/* REQ_RESET 0x00 */
// M68K can only reset PPC and vice versa
// if P5_SELF_RESET is not active.
#define	P5_PPC_RESET		0x10
#define	P5_M68K_RESET		0x08
#define	P5_AMIGA_RESET		0x04
#define	P5_AUX_RESET		0x02
#define	P5_SCSI_RESET		0x01

/* REG_IRQ 0x08 */
#define P5_IRQ_SCSI			0x01
#define P5_IRQ_SCSI_EN		0x02
// 0x04
#define P5_IRQ_PPC_1		0x08
#define P5_IRQ_PPC_2		0x10
#define P5_IRQ_PPC_3		0x20 // MOS sets this bit
// 0x40 always cleared

/* REG_WAITSTATE 0x10 */
#define	P5_PPC_WRITE		0x08
#define	P5_PPC_READ			0x04
#define	P5_M68K_WRITE		0x02
#define	P5_M68K_READ		0x01

/* REG_SHADOW 0x18 */
#define	P5_SELF_RESET		0x40
// 0x20
// 0x10
// 0x08 always set
#define P5_FLASH			0x04 // Flash writable (CSMK3,CSPPC only)
// 0x02 (can be modified even when locked)
#define	P5_SHADOW			0x01 // KS MAP ROM (CSMK3,CSPPC only)

/* REG_LOCK 0x20. CSMK3,CSPPC only */
#define	P5_MAGIC1			0x60 // REG_SHADOW and flash write protection unlock sequence
#define	P5_MAGIC2			0x50
#define	P5_MAGIC3			0x30
#define	P5_MAGIC4			0x70
// when cleared, another CPU gets either stopped or can't access memory until set again.
#define P5_LOCK_CPU			0x01 

/* REG_INT 0x28 */
// 0x40 always set
// 0x20
// 0x10 always set
// 0x08
// 0x04 // MOS sets this bit
#define	P5_ENABLE_IPL		0x02
#define	P5_INT_MASTER		0x01 // 1=m68k gets interrupts, 0=ppc gets interrupts.

/* IPL_EMU 0x30 */
#define	P5_DISABLE_INT		0x40 // if set: all CPU interrupts disabled?
#define	P5_M68K_IPL2		0x20
#define	P5_M68K_IPL1		0x10
#define	P5_M68K_IPL0		0x08
#define P5_M68k_IPL_MASK	0x38
#define	P5_PPC_IPL2			0x04
#define	P5_PPC_IPL1			0x02
#define	P5_PPC_IPL0			0x01
#define P5_PPC_IPL_MASK		0x07

/* INT_LVL 0x38 */
#define	P5_LVL7				0x40
#define	P5_LVL6				0x20
#define	P5_LVL5				0x10
#define	P5_LVL4				0x08
#define	P5_LVL3				0x04
#define	P5_LVL2				0x02
#define	P5_LVL1				0x01

#define CS_RAM_BASE 0x08000000

#define BLIZZARDMK4_RAM_BASE_48 0x48000000
#define BLIZZARDMK4_MAPROM_BASE	0x4ff80000
#define BLIZZARDMK2_MAPROM_BASE	0x0ff80000
#define BLIZZARDMK3_MAPROM_BASE 0x1ef80000
#define BLIZZARD_MAPROM_ENABLE  0x80ffff00
#define BLIZZARD_BOARD_DISABLE  0x80fa0000
#define BLIZZARD_BOARD_DISABLE2 0x80f00000

#define CSMK2_2060_BOARD_DISABLE 0x83000000

static int cpuboard_size = -1, cpuboard2_size = -1;
static int configured;
static int blizzard_jit;
static int maprom_state;
static uae_u32 maprom_base;
static int delayed_rom_protect;
static int f0rom_size, earom_size;
static uae_u8 io_reg[64];
static void *flashrom, *flashrom2;
static struct zfile *flashrom_file;
static int flash_unlocked;
static int csmk2_flashaddressing;
static bool blizzardmaprom_bank_mapped, blizzardmaprom2_bank_mapped;
static bool cpuboard_non_byte_ea;
static uae_u16 a2630_io;

#ifdef WITH_PPC
static bool ppc_irq_pending;

static void set_ppc_interrupt(void)
{
	if (ppc_irq_pending)
		return;
#if PPC_IRQ_DEBUG
	write_log(_T("set_ppc_interrupt\n"));
#endif
	uae_ppc_interrupt(true);
	ppc_irq_pending = true;
}
static void clear_ppc_interrupt(void)
{
	if (!ppc_irq_pending)
		return;
#if PPC_IRQ_DEBUG
	write_log(_T("clear_ppc_interrupt\n"));
#endif
	uae_ppc_interrupt(false);
	ppc_irq_pending = false;
}

static void check_ppc_int_lvl(void)
{
	bool m68kint = (io_reg[CSIII_REG_INT] & P5_INT_MASTER) != 0;
	bool active = (io_reg[CSIII_REG_IPL_EMU] & P5_DISABLE_INT) == 0;
	bool iplemu = (io_reg[CSIII_REG_INT] & P5_ENABLE_IPL) == 0;

	if (m68kint && iplemu && active) {
		uae_u8 ppcipl = (~io_reg[CSIII_REG_IPL_EMU]) & P5_PPC_IPL_MASK;
		if (ppcipl < 7) {
			uae_u8 ilvl = (~io_reg[CSIII_REG_INT_LVL]) & 0x7f;
			if (ilvl) {
				for (int i = ppcipl; i < 7; i++) {
					if (ilvl & (1 << i)) {
						set_ppc_interrupt();
						return;
					}
				}
			}
		}
		clear_ppc_interrupt();
	}
}

bool ppc_interrupt(int new_m68k_ipl)
{
	bool m68kint = (io_reg[CSIII_REG_INT] & P5_INT_MASTER) != 0;
	bool active = (io_reg[CSIII_REG_IPL_EMU] & P5_DISABLE_INT) == 0;
	bool iplemu = (io_reg[CSIII_REG_INT] & P5_ENABLE_IPL) == 0;

	if (!active)
		return false;

	if (!m68kint && iplemu && active) {

		uae_u8 ppcipl = (~io_reg[CSIII_REG_IPL_EMU]) & P5_PPC_IPL_MASK;
		if (new_m68k_ipl < 0)
			new_m68k_ipl = 0;
		io_reg[CSIII_REG_IPL_EMU] &= ~P5_M68k_IPL_MASK;
		io_reg[CSIII_REG_IPL_EMU] |= (new_m68k_ipl << 3) ^ P5_M68k_IPL_MASK;
		if (new_m68k_ipl > ppcipl) {
			set_ppc_interrupt();
		} else {
			clear_ppc_interrupt();
		}
	}

	return m68kint;
}
#endif

static bool mapromconfigured(void)
{
	if (currprefs.maprom && !currprefs.cpuboard_type)
		return true;
	if (currprefs.cpuboard_settings & 1)
		return true;
	return false;
}

void cpuboard_set_flash_unlocked(bool unlocked)
{
	flash_unlocked = unlocked;
}

static bool is_blizzard1230mk2(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230II);
}
static bool is_blizzard1230mk3(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230III);
}
static bool is_blizzard(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230IV) || ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1260);
}
static bool is_blizzard2060(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_2060);
}
static bool is_csmk1(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK1);
}
static bool is_csmk2(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK2);
}
static bool is_csmk3(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK3) || ISCPUBOARDP(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_PPC);
}
static bool is_blizzardppc(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC);
}
static bool is_ppc(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC) || ISCPUBOARDP(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_PPC);
}
static bool is_tekmagic(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_GVP, BOARD_GVP_SUB_TEKMAGIC);
}
static bool is_trexii(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_GVP, BOARD_GVP_SUB_TREXII);
}
static bool is_a2630(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_COMMODORE, BOARD_COMMODORE_SUB_A26x0);
}
static bool is_harms_3kp(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_HARMS, BOARD_HARMS_SUB_3KPRO);
}
static bool is_dkb_12x0(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_DKB, BOARD_DKB_SUB_12x0);
}
static bool is_dkb_wildfire(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_DKB, BOARD_DKB_SUB_WILDFIRE);
}
static bool is_mtec_ematrix530(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_MTEC, BOARD_MTEC_SUB_EMATRIX530);
}
static bool is_dce_typhoon2(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_DCE, BOARD_DCE_SUB_TYPHOON2);
}
static bool is_fusionforty(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_RCS, BOARD_RCS_SUB_FUSIONFORTY);
}
static bool is_apollo12xx(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_ACT, BOARD_ACT_SUB_APOLLO_12xx);
}
static bool is_apollo630(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_ACT, BOARD_ACT_SUB_APOLLO_630);
}
static bool is_kupke(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_KUPKE, 0);
}
static bool is_sx32pro(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_DCE, 0);
}
static bool is_ivsvector(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_IVS, BOARD_IVS_SUB_VECTOR);
}
static bool is_12gauge(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_CSA, BOARD_CSA_SUB_12GAUGE);
}
static bool is_magnum40(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_CSA, BOARD_CSA_SUB_MAGNUM40);
}
static bool is_falcon40(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_MACROSYSTEM, BOARD_MACROSYSTEM_SUB_FALCON040);
}
static bool is_tqm(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_HARDITAL, BOARD_HARDITAL_SUB_TQM);
}
static bool is_a1230s1(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_GVP, BOARD_GVP_SUB_A1230SI);
}
static bool is_a1230s2(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_GVP, BOARD_GVP_SUB_A1230SII);
}
static bool is_quikpak(struct uae_prefs *p)
{
	return ISCPUBOARDP(p, BOARD_GVP, BOARD_GVP_SUB_QUIKPAK);
}

static bool is_aca500(struct uae_prefs *p)
{
	return false; //return ISCPUBOARDP(p, BOARD_IC, BOARD_IC_ACA500);
}

extern addrbank cpuboardmem1_bank;
MEMORY_FUNCTIONS(cpuboardmem1);
addrbank cpuboardmem1_bank = {
	cpuboardmem1_lget, cpuboardmem1_wget, cpuboardmem1_bget,
	cpuboardmem1_lput, cpuboardmem1_wput, cpuboardmem1_bput,
	cpuboardmem1_xlate, cpuboardmem1_check, NULL, _T("*B"), _T("cpuboard ram"),
	cpuboardmem1_lget, cpuboardmem1_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};
extern addrbank cpuboardmem2_bank;
MEMORY_FUNCTIONS(cpuboardmem2);
addrbank cpuboardmem2_bank = {
	cpuboardmem2_lget, cpuboardmem2_wget, cpuboardmem2_bget,
	cpuboardmem2_lput, cpuboardmem2_wput, cpuboardmem2_bput,
	cpuboardmem2_xlate, cpuboardmem2_check, NULL, _T("*B"), _T("cpuboard ram #2"),
	cpuboardmem2_lget, cpuboardmem2_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

#ifdef WITH_CPUBOARD
DECLARE_MEMORY_FUNCTIONS(blizzardio);
static addrbank blizzardio_bank = {
	blizzardio_lget, blizzardio_wget, blizzardio_bget,
	blizzardio_lput, blizzardio_wput, blizzardio_bput,
	default_xlate, default_check, NULL, NULL, _T("CPUBoard IO"),
	blizzardio_wget, blizzardio_bget,
	ABFLAG_IO, S_READ, S_WRITE
};

DECLARE_MEMORY_FUNCTIONS(blizzardram);
static addrbank blizzardram_bank = {
	blizzardram_lget, blizzardram_wget, blizzardram_bget,
	blizzardram_lput, blizzardram_wput, blizzardram_bput,
	blizzardram_xlate, blizzardram_check, NULL, NULL, _T("CPUBoard RAM"),
	blizzardram_lget, blizzardram_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, 0, 0
};

DECLARE_MEMORY_FUNCTIONS(blizzardea);
static addrbank blizzardea_bank = {
	blizzardea_lget, blizzardea_wget, blizzardea_bget,
	blizzardea_lput, blizzardea_wput, blizzardea_bput,
	blizzardea_xlate, blizzardea_check, NULL, _T("rom_ea"), _T("CPUBoard E9/EA Autoconfig"),
	blizzardea_lget, blizzardea_wget,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

DECLARE_MEMORY_FUNCTIONS(blizzarde8);
static addrbank blizzarde8_bank = {
	blizzarde8_lget, blizzarde8_wget, blizzarde8_bget,
	blizzarde8_lput, blizzarde8_wput, blizzarde8_bput,
	blizzarde8_xlate, blizzarde8_check, NULL, NULL, _T("CPUBoard E8 Autoconfig"),
	blizzarde8_lget, blizzarde8_wget,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

DECLARE_MEMORY_FUNCTIONS(blizzardf0);
static addrbank blizzardf0_bank = {
	blizzardf0_lget, blizzardf0_wget, blizzardf0_bget,
	blizzardf0_lput, blizzardf0_wput, blizzardf0_bput,
	blizzardf0_xlate, blizzardf0_check, NULL, _T("rom_f0_ppc"), _T("CPUBoard F00000"),
	blizzardf0_lget, blizzardf0_wget,
	ABFLAG_ROM, S_READ, S_WRITE
};

#if 0
DECLARE_MEMORY_FUNCTIONS(blizzardram_nojit);
static addrbank blizzardram_nojit_bank = {
	blizzardram_nojit_lget, blizzardram_nojit_wget, blizzardram_nojit_bget,
	blizzardram_nojit_lput, blizzardram_nojit_wput, blizzardram_nojit_bput,
	blizzardram_nojit_xlate, blizzardram_nojit_check, NULL, NULL, _T("CPUBoard RAM"),
	blizzardram_nojit_lget, blizzardram_nojit_wget,
	ABFLAG_RAM | ABFLAG_THREADSAFE, S_READ, S_WRITE
};
#endif

DECLARE_MEMORY_FUNCTIONS(blizzardmaprom);
static addrbank blizzardmaprom_bank = {
	blizzardmaprom_lget, blizzardmaprom_wget, blizzardmaprom_bget,
	blizzardmaprom_lput, blizzardmaprom_wput, blizzardmaprom_bput,
	blizzardmaprom_xlate, blizzardmaprom_check, NULL, _T("maprom"), _T("CPUBoard MAPROM"),
	blizzardmaprom_lget, blizzardmaprom_wget,
	ABFLAG_RAM, S_READ, S_WRITE
};
DECLARE_MEMORY_FUNCTIONS(blizzardmaprom2);
static addrbank blizzardmaprom2_bank = {
	blizzardmaprom2_lget, blizzardmaprom2_wget, blizzardmaprom2_bget,
	blizzardmaprom2_lput, blizzardmaprom2_wput, blizzardmaprom2_bput,
	blizzardmaprom2_xlate, blizzardmaprom2_check, NULL, _T("maprom2"), _T("CPUBoard MAPROM2"),
	blizzardmaprom2_lget, blizzardmaprom2_wget,
	ABFLAG_RAM, S_READ, S_WRITE
};

// hack to map F41000 SCSI SCRIPTS RAM to JIT friendly address
void cyberstorm_scsi_ram_put(uaecptr addr, uae_u32 v)
{
	addr &= 0xffff;
	addr += (CSIII_NCR & 0x7ffff);
	blizzardf0_bank.baseaddr[addr] = v;
}
uae_u32 cyberstorm_scsi_ram_get(uaecptr addr)
{
	uae_u32 v;
	addr &= 0xffff;
	addr += (CSIII_NCR & 0x7ffff);
	v = blizzardf0_bank.baseaddr[addr];
	return v;
}
uae_u8 *REGPARAM2 cyberstorm_scsi_ram_xlate(uaecptr addr)
{
	addr &= 0xffff;
	addr += (CSIII_NCR & 0x7ffff);
	return blizzardf0_bank.baseaddr + addr;
}
int REGPARAM2 cyberstorm_scsi_ram_check(uaecptr a, uae_u32 b)
{
	a &= 0xffff;
	return a >= 0x1000 && a + b < 0x3000;
}

MEMORY_FUNCTIONS(blizzardram);

#if 0
MEMORY_BGET(blizzardram_nojit);
MEMORY_WGET(blizzardram_nojit);
MEMORY_LGET(blizzardram_nojit);
MEMORY_CHECK(blizzardram_nojit);
MEMORY_XLATE(blizzardram_nojit);

static void REGPARAM2 blizzardram_nojit_lput(uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	addr &= blizzardram_nojit_bank.mask;
	if (maprom_state && addr >= maprom_base)
		return;
	m = (uae_u32 *)(blizzardram_nojit_bank.baseaddr + addr);
	do_put_mem_long(m, l);
}
static void REGPARAM2 blizzardram_nojit_wput(uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr &= blizzardram_nojit_bank.mask;
	if (maprom_state && addr >= maprom_base)
		return;
	m = (uae_u16 *)(blizzardram_nojit_bank.baseaddr + addr);
	do_put_mem_word(m, w);
}
static void REGPARAM2 blizzardram_nojit_bput(uaecptr addr, uae_u32 b)
{
	addr &= blizzardram_nojit_bank.mask;
	if (maprom_state && addr >= maprom_base)
		return;
	blizzardram_nojit_bank.baseaddr[addr] = b;
}
#endif
#endif // WITH_CPUBOARD

static void no_rom_protect(void)
{
	if (delayed_rom_protect)
		return;
	delayed_rom_protect = 10;
	protect_roms(false);
}

static uae_u8 *writeprotect_addr;
static int writeprotect_size;

static void maprom_unwriteprotect(void)
{
	if (!writeprotect_addr)
		return;
	if (currprefs.cachesize && !currprefs.comptrustlong)
		uae_vm_protect(writeprotect_addr, writeprotect_size, UAE_VM_READ_WRITE);
	writeprotect_addr = NULL;
}

static void maprom_writeprotect(uae_u8 *addr, int size)
{
	maprom_unwriteprotect();
	writeprotect_addr = addr;
	writeprotect_size = size;
	if (currprefs.cachesize && !currprefs.comptrustlong)
		uae_vm_protect(addr, size, UAE_VM_READ);
}

#ifdef WITH_CPUBOARD
MEMORY_BGET(blizzardmaprom2);
MEMORY_WGET(blizzardmaprom2);
MEMORY_LGET(blizzardmaprom2);
MEMORY_CHECK(blizzardmaprom2);
MEMORY_XLATE(blizzardmaprom2);

static void REGPARAM2 blizzardmaprom2_lput(uaecptr addr, uae_u32 l)
{
}
static void REGPARAM2 blizzardmaprom2_wput(uaecptr addr, uae_u32 l)
{
}
static void REGPARAM2 blizzardmaprom2_bput(uaecptr addr, uae_u32 l)
{
}

MEMORY_BGET(blizzardmaprom);
MEMORY_WGET(blizzardmaprom);
MEMORY_LGET(blizzardmaprom);
MEMORY_CHECK(blizzardmaprom);
MEMORY_XLATE(blizzardmaprom);

static void REGPARAM2 blizzardmaprom_lput(uaecptr addr, uae_u32 l)
{
#if MAPROM_DEBUG
	write_log(_T("MAPROM LPUT %08x %08x %d %08x\n"), addr, l, maprom_state, M68K_GETPC);
#endif
	uae_u32 *m;
	if (is_blizzard2060(&currprefs) && !maprom_state)
		return;
	addr &= blizzardmaprom_bank.mask;
	m = (uae_u32 *)(blizzardmaprom_bank.baseaddr + addr);
	do_put_mem_long(m, l);
	if (maprom_state > 0 && !(addr & 0x80000)) {
		no_rom_protect();
		m = (uae_u32 *)(kickmem_bank.baseaddr + addr);
		do_put_mem_long(m, l);
	}
}
static void REGPARAM2 blizzardmaprom_wput(uaecptr addr, uae_u32 w)
{
#if MAPROM_DEBUG
	write_log(_T("MAPROM WPUT %08x %08x %d\n"), addr, w, maprom_state);
#endif
	uae_u16 *m;
	if (is_blizzard2060(&currprefs) && !maprom_state)
		return;
	addr &= blizzardmaprom_bank.mask;
	m = (uae_u16 *)(blizzardmaprom_bank.baseaddr + addr);
	do_put_mem_word(m, w);
	if (maprom_state > 0 && !(addr & 0x80000)) {
		no_rom_protect();
		m = (uae_u16 *)(kickmem_bank.baseaddr + addr);
		do_put_mem_word(m, w);
	}
}
static void REGPARAM2 blizzardmaprom_bput(uaecptr addr, uae_u32 b)
{
#if MAPROM_DEBUG
	write_log(_T("MAPROM LPUT %08x %08x %d\n"), addr, b, maprom_state);
#endif
	if (is_blizzard2060(&currprefs) && !maprom_state)
		return;
	addr &= blizzardmaprom_bank.mask;
	blizzardmaprom_bank.baseaddr[addr] = b;
	if (maprom_state > 0 && !(addr & 0x80000)) {
		no_rom_protect();
		kickmem_bank.baseaddr[addr] = b;
	}
}

MEMORY_CHECK(blizzardea);
MEMORY_XLATE(blizzardea);

static void blizzardf0_slow(int size)
{
	if (is_blizzard(&currprefs) || is_blizzardppc(&currprefs) || is_blizzard2060(&currprefs)) {
		if (size == 4)
			regs.memory_waitstate_cycles += F0_WAITSTATES * 6;
		else if (size == 2)
			regs.memory_waitstate_cycles += F0_WAITSTATES * 3;
		else
			regs.memory_waitstate_cycles += F0_WAITSTATES * 1;
	}
}

static int REGPARAM2 blizzarde8_check(uaecptr addr, uae_u32 size)
{
	return 0;
}
static uae_u8 *REGPARAM2 blizzarde8_xlate(uaecptr addr)
{
	return NULL;
}

static uae_u32 REGPARAM2 blizzardf0_bget(uaecptr addr)
{
	uae_u8 v;

	blizzardf0_slow(1);

	addr &= blizzardf0_bank.mask;
	if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		if (flash_unlocked) {
			return flash_read(flashrom, addr);
		}
	} else if (is_csmk2(&currprefs)) {
		addr &= 65535;
		addr += 65536;
		return flash_read(flashrom, addr);
	} else if (is_dkb_wildfire(&currprefs)) {
		if (flash_unlocked) {
			if (addr & 1)
				return flash_read(flashrom2, addr);
			else
				return flash_read(flashrom, addr);
		}
	}
	v = blizzardf0_bank.baseaddr[addr];
	return v;
}
static uae_u32 REGPARAM2 blizzardf0_lget(uaecptr addr)
{
	uae_u32 *m;

	//write_log(_T("F0 LONGGET %08x\n"), addr);

	blizzardf0_slow(4);

	addr &= blizzardf0_bank.mask;
	m = (uae_u32 *)(blizzardf0_bank.baseaddr + addr);
	return do_get_mem_long(m);
}
static uae_u32 REGPARAM2 blizzardf0_wget(uaecptr addr)
{
	uae_u16 *m, v;

	blizzardf0_slow(2);
	if (is_dkb_wildfire(&currprefs) && flash_unlocked) {
		v = blizzardf0_bget(addr + 0) << 8;
		v |= blizzardf0_bget(addr + 1);
	} else {
		addr &= blizzardf0_bank.mask;
		m = (uae_u16 *)(blizzardf0_bank.baseaddr + addr);
		v = do_get_mem_word(m);
	}
	return v;
}

static void REGPARAM2 blizzardf0_bput(uaecptr addr, uae_u32 b)
{
	blizzardf0_slow(1);

	addr &= blizzardf0_bank.mask;
	if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		if (flash_unlocked) {
			flash_write(flashrom, addr, b);
		}
	} else if (is_csmk2(&currprefs)) {
		addr += 65536;
		addr &= ~3;
		addr |= csmk2_flashaddressing;
		flash_write(flashrom, addr, b);
	} else if (is_dkb_wildfire(&currprefs)) {
		if (flash_unlocked) {
			if (addr & 1)
				flash_write(flashrom2, addr, b);
			else
				flash_write(flashrom, addr, b);
		}
	}
}
static void REGPARAM2 blizzardf0_lput(uaecptr addr, uae_u32 b)
{
	blizzardf0_slow(4);
}
static void REGPARAM2 blizzardf0_wput(uaecptr addr, uae_u32 b)
{
	blizzardf0_slow(2);
	if (is_dkb_wildfire(&currprefs)) {
		blizzardf0_bput(addr + 0, b >> 8);
		blizzardf0_bput(addr + 1, b >> 0);
	}
}

MEMORY_CHECK(blizzardf0);
MEMORY_XLATE(blizzardf0);

static uae_u32 REGPARAM2 blizzardea_lget(uaecptr addr)
{
	uae_u32 v = 0;
	if (cpuboard_non_byte_ea) {
		v = blizzardea_bget(addr + 0) <<  24;
		v |= blizzardea_bget(addr + 1) << 16;
		v |= blizzardea_bget(addr + 2) <<  8;
		v |= blizzardea_bget(addr + 3) <<  0;
	}
	return v;
}
static uae_u32 REGPARAM2 blizzardea_wget(uaecptr addr)
{
	uae_u32 v = 0;
	if (cpuboard_non_byte_ea) {
		v  = blizzardea_bget(addr + 0) <<  8;
		v |= blizzardea_bget(addr + 1) <<  0;
	}
	return v;
}
static uae_u32 REGPARAM2 blizzardea_bget(uaecptr addr)
{
	uae_u8 v = 0;

	addr &= blizzardea_bank.mask;
	if (is_tekmagic(&currprefs) || is_trexii(&currprefs)) {
		cpuboard_non_byte_ea = true;
		v = cpuboard_ncr710_io_bget(addr);
	} else if (is_quikpak(&currprefs)) {
		cpuboard_non_byte_ea = true;
		v = cpuboard_ncr720_io_bget(addr);
	} else if (is_blizzard2060(&currprefs) && addr >= BLIZZARD_2060_SCSI_OFFSET) {
		v = cpuboard_ncr9x_scsi_get(addr);
	} else if (is_blizzard1230mk2(&currprefs) && addr >= 0x10000 && (currprefs.cpuboard_settings & 2)) {
		v = cpuboard_ncr9x_scsi_get(addr);
	} else if (is_blizzard(&currprefs)) {
		if (addr & BLIZZARD_SCSI_KIT4_SCSI_OFFSET)
			v = cpuboard_ncr9x_scsi_get(addr);
		else
			v = blizzardea_bank.baseaddr[addr];
	} else if (is_blizzard1230mk3(&currprefs)) {
		if (addr & BLIZZARD_SCSI_KIT3_SCSI_OFFSET)
			v = cpuboard_ncr9x_scsi_get(addr);
		else
			v = blizzardea_bank.baseaddr[addr];
	} else if (is_csmk1(&currprefs)) {
		if (addr >= CYBERSTORM_MK1_SCSI_OFFSET)
			v = cpuboard_ncr9x_scsi_get(addr);
		else
			v = blizzardea_bank.baseaddr[addr];
	} else if (is_csmk2(&currprefs)) {
		if (addr >= CYBERSTORM_MK2_SCSI_OFFSET) {
			v = cpuboard_ncr9x_scsi_get(addr);
		} else if (flash_active(flashrom, addr)) {
			v = flash_read(flashrom, addr);
		} else {
			v = blizzardea_bank.baseaddr[addr];
		}
	} else {
		v = blizzardea_bank.baseaddr[addr];
	}
	return v;
}

static void REGPARAM2 blizzardea_lput(uaecptr addr, uae_u32 l)
{
	if (cpuboard_non_byte_ea) {
		blizzardea_bput(addr + 0, l >> 24);
		blizzardea_bput(addr + 1, l >> 16);
		blizzardea_bput(addr + 2, l >>  8);
		blizzardea_bput(addr + 3, l >>  0);
	}
}
static void REGPARAM2 blizzardea_wput(uaecptr addr, uae_u32 w)
{
	if (cpuboard_non_byte_ea) {
		blizzardea_bput(addr + 0, w >> 8);
		blizzardea_bput(addr + 1, w >> 0);
	}
}
static void REGPARAM2 blizzardea_bput(uaecptr addr, uae_u32 b)
{
	addr &= blizzardea_bank.mask;
	if (is_tekmagic(&currprefs) || is_trexii(&currprefs)) {
		cpuboard_non_byte_ea = true;
		cpuboard_ncr710_io_bput(addr, b);
	} else if (is_quikpak(&currprefs)) {
		cpuboard_non_byte_ea = true;
		cpuboard_ncr720_io_bput(addr, b);
	} else if (is_blizzard1230mk2(&currprefs) && addr >= 0x10000 && (currprefs.cpuboard_settings & 2)) {
		cpuboard_ncr9x_scsi_put(addr, b);
	} else if (is_blizzard2060(&currprefs) && addr >= BLIZZARD_2060_SCSI_OFFSET) {
		cpuboard_ncr9x_scsi_put(addr, b);
	} else if ((is_blizzard(&currprefs)) && addr >= BLIZZARD_SCSI_KIT4_SCSI_OFFSET) {
		cpuboard_ncr9x_scsi_put(addr, b);
	} else if ((is_blizzard1230mk3(&currprefs)) && addr >= BLIZZARD_SCSI_KIT3_SCSI_OFFSET) {
		cpuboard_ncr9x_scsi_put(addr, b);
	} else if (is_csmk1(&currprefs)) {
		if (addr >= CYBERSTORM_MK1_SCSI_OFFSET) {
			cpuboard_ncr9x_scsi_put(addr, b);
		}
	} else if (is_csmk2(&currprefs)) {
		if (addr >= CYBERSTORM_MK2_SCSI_OFFSET) {
			cpuboard_ncr9x_scsi_put(addr, b);
		}  else {
			addr &= 65535;
			addr &= ~3;
			addr |= csmk2_flashaddressing;
			flash_write(flashrom, addr, b);
		}
	} else if (is_mtec_ematrix530(&currprefs) || is_dce_typhoon2(&currprefs)) {	
		if (cpuboardmem1_bank.allocated_size < 128 * 1024 * 1024 / 2) {
			if (cpuboardmem2_bank.allocated_size)
				map_banks(&cpuboardmem2_bank, (0x18000000 + cpuboardmem1_bank.allocated_size) >> 16, cpuboardmem2_bank.allocated_size >> 16, 0);
			else
				map_banks(&dummy_bank, (0x18000000 + cpuboardmem1_bank.allocated_size) >> 16, cpuboardmem1_bank.allocated_size >> 16, 0);
		}
	}
}

static void REGPARAM2 blizzarde8_lput(uaecptr addr, uae_u32 b)
{
}
static void REGPARAM2 blizzarde8_wput(uaecptr addr, uae_u32 b)
{
}
static void REGPARAM2 blizzarde8_bput(uaecptr addr, uae_u32 b)
{
	b &= 0xff;
	addr &= 65535;
	if (addr == 0x48 && !configured) {
		uae_u32 size = map_banks_z2_autosize(&blizzardea_bank, b);
		write_log(_T("Accelerator Z2 board autoconfigured at %02X0000, size %08x\n"), b, size);
		configured = 1;
		expamem_next (&blizzardea_bank, NULL);
		return;
	}
	if (addr == 0x4c && !configured) {
		write_log(_T("Blizzard Z2 SHUT-UP!\n"));
		configured = 1;
		expamem_next (NULL, NULL);
		return;
	}
}
static uae_u32 REGPARAM2 blizzarde8_bget(uaecptr addr)
{
	uae_u32 v = 0xffff;
	v = blizzardea_bank.baseaddr[addr & blizzardea_bank.mask];
	return v;
}
static uae_u32 REGPARAM2 blizzarde8_wget(uaecptr addr)
{
	uae_u32 v = 0xffff;
	v = (blizzardea_bank.baseaddr[addr & blizzardea_bank.mask] << 8) | blizzardea_bank.baseaddr[(addr + 1) & blizzardea_bank.mask];
	return v;
}
static uae_u32 REGPARAM2 blizzarde8_lget(uaecptr addr)
{
	uae_u32 v = 0xffff;
	v = (blizzarde8_wget(addr) << 16) | blizzarde8_wget(addr + 2);
	return v;
}

static void blizzard_copymaprom(void)
{
	if (!maprom_state) {
		reload_roms();
	} else {
		uae_u8 *src = NULL;
		if (is_blizzardppc(&currprefs)) {
			src = blizzardram_bank.baseaddr + cpuboard_size - 524288;
		} else {
			src = blizzardmaprom_bank.baseaddr;
		}
		if (src) {
			uae_u8 *dst = kickmem_bank.baseaddr;
			protect_roms(false);
			memcpy(dst, src, 524288);
			protect_roms(true);
			set_roms_modified();
		}
		if (is_blizzard1230mk2(&currprefs) && cpuboard_size >= 64 * 1024 * 1024) {
			map_banks(&blizzardmaprom_bank, BLIZZARDMK2_MAPROM_BASE >> 16, 524288 >> 16, 0);
		}
		if (is_blizzard(&currprefs) && cpuboard_size >= 256 * 1024 * 1024) {
			map_banks(&blizzardmaprom_bank, BLIZZARDMK4_MAPROM_BASE >> 16, 524288 >> 16, 0);
		}
	}
}
static void cyberstorm_copymaprom(void)
{
	if (!maprom_state) {
		reload_roms();
	} else if (blizzardmaprom_bank.baseaddr) {
		uae_u8 *src = blizzardmaprom_bank.baseaddr;
		uae_u8 *dst = kickmem_bank.baseaddr;
		protect_roms(false);
		memcpy(dst, src, 524288);
		protect_roms(true);
		set_roms_modified();
	}
}
static void cyberstormmk2_copymaprom(void)
{
	if (a3000hmem_bank.baseaddr) {
		uae_u8 *src = a3000hmem_bank.baseaddr + a3000hmem_bank.allocated_size - 524288;
		uae_u8 *dst = kickmem_bank.baseaddr;
		protect_roms(false);
		memcpy(dst, src, 524288);
		protect_roms(true);
		set_roms_modified();
	}
}
static void cyberstormmk1_copymaprom(void)
{
	if (blizzardmaprom_bank.baseaddr) {
		uae_u8 *src = blizzardmaprom_bank.baseaddr;
		uae_u8 *dst = kickmem_bank.baseaddr;
		protect_roms(false);
		memcpy(dst, src, 524288);
		protect_roms(true);
		set_roms_modified();
	}
}

static void csamagnum40_domaprom(void)
{
	if (!maprom_state) {
		reload_roms();
	} else if (a3000hmem_bank.baseaddr && a3000hmem_bank.allocated_size >= 0x1000000) {
		uae_u8 *src = a3000hmem_bank.baseaddr + 0xf80000;
		uae_u8 *dst = kickmem_bank.baseaddr;
		protect_roms(false);
		memcpy(dst, src, 524288);
		protect_roms(true);
		set_roms_modified();
	}
}
#endif

static const uae_u32 gvp_a530_maprom[7] =
{
	0, 0, 0,
	0x280000,
	0x580000,
	0x380000,
	0x980000
};

void cpuboard_gvpmaprom(int b)
{
	if (!ISCPUBOARDP(&currprefs, BOARD_GVP, BOARD_GVP_SUB_A530) &&
		!ISCPUBOARDP(&currprefs, BOARD_GVP, BOARD_GVP_SUB_GFORCE030) && 
		!ISCPUBOARDP(&currprefs, BOARD_GVP, BOARD_GVP_SUB_GFORCE040))
		return;

	if (b < 0 || b > 7)
		return;
	if (!b) {
		if (maprom_state)
			reload_roms();
		maprom_state = 0;
	} else {
		const uae_u32* addrp = 0;
		maprom_state = b;
		if (ISCPUBOARDP(&currprefs, BOARD_GVP, BOARD_GVP_SUB_A530)) {
			addrp = gvp_a530_maprom;
		}
		if (addrp) {
			uae_u32 addr = addrp[b];
			if (addr) {
				uae_u8 *src = get_real_address(addr);
				uae_u8 *dst = kickmem_bank.baseaddr;
				protect_roms(false);
				memcpy(dst, src, 524288);
				protect_roms(true);
				set_roms_modified();
			}
		}
	}
}

bool cpuboard_is_ppcboard_irq(void)
{
	if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		if (!(io_reg[CSIII_REG_IRQ] & (P5_IRQ_SCSI_EN | P5_IRQ_SCSI))) {
			return true;
		} else if (!(io_reg[CSIII_REG_IRQ] & (P5_IRQ_PPC_1 | P5_IRQ_PPC_2))) {
			return true;
		}
	}
	return false;
}

void cpuboard_rethink(void)
{
	if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		if (!(io_reg[CSIII_REG_IRQ] & (P5_IRQ_SCSI_EN | P5_IRQ_SCSI))) {
			safe_interrupt_set(IRQ_SOURCE_CPUBOARD, 0, false);
			if (currprefs.cachesize)
				atomic_or(&uae_int_requested, 0x010000);
#ifdef WITH_PPC
			uae_ppc_wakeup_main();
#endif
		} else if (!(io_reg[CSIII_REG_IRQ] & (P5_IRQ_PPC_1 | P5_IRQ_PPC_2))) {
			safe_interrupt_set(IRQ_SOURCE_CPUBOARD, 1, false);
			if (currprefs.cachesize)
				atomic_or(&uae_int_requested, 0x010000);
#ifdef WITH_PPC
			uae_ppc_wakeup_main();
#endif
		} else {
			atomic_and(&uae_int_requested, ~0x010000);
		}
#ifdef WITH_PPC
		check_ppc_int_lvl();
		ppc_interrupt(intlev());
#endif
	}
}

#ifdef WITH_CPUBOARD

static void blizzardppc_maprom(void)
{
	if (cpuboard_size <= 2 * 524288)
		return;

	map_banks(&dummy_bank, CYBERSTORM_MAPROM_BASE >> 16, 524288 >> 16, 0);

	if (blizzardmaprom_bank_mapped)
		mapped_free(&blizzardmaprom_bank);
	if (blizzardmaprom2_bank_mapped)
		mapped_free(&blizzardmaprom2_bank);

	// BPPC write protects MAP ROM region. CSPPC does not.

	if (maprom_state) {
		blizzardmaprom_bank.baseaddr = blizzardram_bank.baseaddr + cpuboard_size - 2 * 524288;

		blizzardmaprom2_bank.baseaddr = blizzardram_bank.baseaddr + cpuboard_size - 524288;
		blizzardmaprom2_bank.start = blizzardram_bank.start + cpuboard_size - 524288;
		blizzardmaprom2_bank.reserved_size = 524288;
		blizzardmaprom2_bank.mask = 524288 - 1;
		blizzardmaprom2_bank.flags |= ABFLAG_INDIRECT | ABFLAG_NOALLOC;
		mapped_malloc(&blizzardmaprom2_bank);
		blizzardmaprom2_bank_mapped = true;
		map_banks(&blizzardmaprom2_bank, blizzardmaprom2_bank.start >> 16, 524288 >> 16, 0);

		maprom_writeprotect(blizzardmaprom2_bank.baseaddr, 524288);

	} else {

		blizzardmaprom_bank.baseaddr = blizzardram_bank.baseaddr + cpuboard_size - 524288;
		blizzardmaprom2_bank.baseaddr = NULL;
		maprom_unwriteprotect();

	}

	blizzardmaprom_bank.start = CYBERSTORM_MAPROM_BASE;
	blizzardmaprom_bank.reserved_size = 524288;
	blizzardmaprom_bank.mask = 524288 - 1;
	blizzardmaprom_bank.flags |= ABFLAG_INDIRECT | ABFLAG_NOALLOC;
	mapped_malloc(&blizzardmaprom_bank);
	blizzardmaprom_bank_mapped = true;
	map_banks(&blizzardmaprom_bank, CYBERSTORM_MAPROM_BASE >> 16, 524288 >> 16, 0);
}

static void cyberstorm_maprom(void)
{
	if (a3000hmem_bank.reserved_size <= 2 * 524288)
		return;
	if (currprefs.cachesize && !currprefs.comptrustbyte && ISCPUBOARDP(&currprefs, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK3)) {
		write_log(_T("JIT Direct enabled: CSMK3 MAPROM not available.\n"));
		return;
	}

	map_banks(&dummy_bank, CYBERSTORM_MAPROM_BASE >> 16, 524288 >> 16, 0);
	if (blizzardmaprom_bank_mapped)
		mapped_free(&blizzardmaprom_bank);
	if (maprom_state) {
		blizzardmaprom_bank.baseaddr = a3000hmem_bank.baseaddr + a3000hmem_bank.reserved_size - 2 * 524288;
	} else {
		blizzardmaprom_bank.baseaddr = a3000hmem_bank.baseaddr + a3000hmem_bank.reserved_size - 524288;
	}
	blizzardmaprom_bank.start = CYBERSTORM_MAPROM_BASE;
	blizzardmaprom_bank.reserved_size = 524288;
	blizzardmaprom_bank.mask = 524288 - 1;
	blizzardmaprom_bank.flags |= ABFLAG_INDIRECT | ABFLAG_NOALLOC;
	mapped_malloc(&blizzardmaprom_bank);
	blizzardmaprom_bank_mapped = true;

	map_banks(&blizzardmaprom_bank, CYBERSTORM_MAPROM_BASE >> 16, 524288 >> 16, 0);
}

static void cyberstormmk2_maprom(void)
{
	if (maprom_state)
		map_banks_nojitdirect(&blizzardmaprom_bank, blizzardmaprom_bank.start >> 16, 524288 >> 16, 0);
}

void cyberstorm_mk3_ppc_irq_setonly(int id, int level)
{
	if (level)
		io_reg[CSIII_REG_IRQ] &= ~P5_IRQ_SCSI;
	else
		io_reg[CSIII_REG_IRQ] |= P5_IRQ_SCSI;
}
void cyberstorm_mk3_ppc_irq(int id, int level)
{
	cyberstorm_mk3_ppc_irq_setonly(id, level);
	devices_rethink_all(cpuboard_rethink);
}

void blizzardppc_irq_setonly(int level)
{
	if (level)
		io_reg[CSIII_REG_IRQ] &= ~P5_IRQ_SCSI;
	else
		io_reg[CSIII_REG_IRQ] |= P5_IRQ_SCSI;
}
void blizzardppc_irq(int id, int level)
{
	blizzardppc_irq_setonly(level);
	devices_rethink_all(cpuboard_rethink);
}

static uae_u32 REGPARAM2 blizzardio_bget(uaecptr addr)
{
	uae_u8 v = 0;
#if CPUBOARD_IO_LOG > 1
	write_log(_T("CS IO XBGET %08x=%02X PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
#endif
	if (is_magnum40(&currprefs)) {
		if ((addr & 0xff0f) == 0x0c0c) {
			int reg = (addr >> 4) & 7;
			v = io_reg[reg];
			if (reg >= 0 && reg <= 3)
				v &= ~0x04; // REV0 to REV3
			if (reg == 0)
				v |= 0x04; // REV0=1
			write_log(_T("CSA Magnum40 read reg %0d = %d %08x\n"), reg, v, M68K_GETPC);
		}
	} else if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		uae_u32 bank = addr & 0x10000;
		if (bank == 0) {
			int reg = (addr & 0xff) / 8;
			v = io_reg[reg];
			if (reg == CSIII_REG_LOCK) {
				v &= 0x01;
				v |= 0x10;
				if (is_blizzardppc(&currprefs)) {
					v |= 0x08; // BPPC identification
					if (maprom_state)
						v |= 0x04;
				}
			} else if (reg == CSIII_REG_IRQ)  {
				v &= 0x3f;
			} else if (reg == CSIII_REG_INT) {
				v |= 0x40 | 0x10;
			} else if (reg == CSIII_REG_SHADOW) {
				v |= 0x08;
			} else if (reg == CSIII_REG_RESET) {
				v &= 0x1f;
			} else if (reg == CSIII_REG_IPL_EMU) {
				// PPC not master: m68k IPL = all ones
				if (io_reg[CSIII_REG_INT] & P5_INT_MASTER)
					v |= P5_M68k_IPL_MASK;
			}
#if CPUBOARD_IO_LOG > 0
			if (reg != CSIII_REG_IRQ || CPUBOARD_IO_LOG > 2)
				write_log(_T("CS IO BGET %08x=%02X PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
#endif
		} else {
#if CPUBOARD_IO_LOG > 0
			write_log(_T("CS IO BGET %08x=%02X PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
#endif
		}
	}
	return v;
}
static uae_u32 REGPARAM2 blizzardio_wget(uaecptr addr)
{
#if CPUBOARD_IO_LOG > 1
	write_log(_T("CS IO XWGET %08x PC=%08x\n"), addr, M68K_GETPC);
#endif
	if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		;//write_log(_T("CS IO WGET %08x\n"), addr);
		//activate_debugger();
	} else if (is_aca500(&currprefs)) {
		addr &= 0x3f000;
		switch (addr)
		{
			case 0x03000:
			return 0;
			case 0x07000:
			return 0;
			case 0x0b000:
			return 0;
			case 0x0f000:
			return 0;
			case 0x13000:
			return 0;
			case 0x17000:
			return 0;
			case 0x1b000:
			return 0x8000;
			case 0x1f000:
			return 0x8000;
			case 0x23000:
			return 0;
			case 0x27000:
			return 0;
			case 0x2b000:
			return 0;
			case 0x2f000:
			return 0;
			case 0x33000:
			return 0x8000;
			case 0x37000:
			return 0;
			case 0x3b000:
			return 0;
		}
	}
	return 0;
}
static uae_u32 REGPARAM2 blizzardio_lget(uaecptr addr)
{
#if CPUBOARD_IO_LOG > 1
	write_log(_T("CS IO LGET %08x PC=%08x\n"), addr, M68K_GETPC);
#endif
	if (is_blizzard2060(&currprefs) && mapromconfigured()) {
		if (addr & 0x10000000) {
			maprom_state = 0;
		} else {
			maprom_state = 1;
		}
	}
	return 0;
}
static void REGPARAM2 blizzardio_bput(uaecptr addr, uae_u32 v)
{
#if CPUBOARD_IO_LOG > 1
	write_log(_T("CS IO XBPUT %08x %02x PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
#endif
	if (is_magnum40(&currprefs)) {
		if ((addr & 0xff0f) == 0x0c0c) {
			int reg = (addr >> 4) & 7;
			if (reg == 3 && ((v ^ io_reg[reg]) & 1)) {
				maprom_state = v & 1;
				write_log(_T("CSA Magnum40 MAPROM=%d\n"), maprom_state);
				csamagnum40_domaprom();
			}
			io_reg[reg] = (uae_u8)v;
			write_log(_T("CSA Magnum40 write reg %0d = %02x %08x\n"), reg, v & 0xff, M68K_GETPC);
		}
	} else if (is_fusionforty(&currprefs)) {
		write_log(_T("FusionForty IO XBPUT %08x %02x PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
	} else if (is_csmk2(&currprefs)) {
		csmk2_flashaddressing = addr & 3;
		if (addr == 0x880000e3 && v == 0x2a) {
			maprom_state = 1;
			write_log(_T("CSMKII: MAPROM enabled\n"));
			cyberstormmk2_copymaprom();
		}
	} else if (is_blizzard(&currprefs) || is_blizzard1230mk2(&currprefs) || is_blizzard1230mk3(&currprefs)) {
		if ((addr & 65535) == (BLIZZARD_MAPROM_ENABLE & 65535)) {
			if (v != 0x42 || maprom_state || !mapromconfigured())
				return;
			maprom_state = 1;
			write_log(_T("Blizzard: MAPROM enabled\n"));
			blizzard_copymaprom();
		}
	} else if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
#if CPUBOARD_IO_LOG > 0
		write_log(_T("CS IO BPUT %08x %02x PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
#endif
		uae_u32 bank = addr & 0x10000;
		if (bank == 0) {
			addr &= 0xff;
			if (is_blizzardppc(&currprefs)) {
				if (addr == BPPC_UNLOCK_FLASH && v == BPPC_MAGIC_UNLOCK_VALUE) {
					flash_unlocked = 1;
					write_log(_T("BPPC: flash unlocked\n"));
				} else 	if (addr == BPPC_LOCK_FLASH) {
					flash_unlocked = 0;
					write_log(_T("BPPC: flash locked\n"));
				} else if (addr == BPPC_MAPROM_ON) {
					write_log(_T("BPPC: maprom enabled\n"));
					maprom_state = -1;
					cyberstorm_copymaprom();
					blizzardppc_maprom();
				} else if (addr == BPPC_MAPROM_OFF) {
					write_log(_T("BPPC: maprom disabled\n"));
					maprom_state = 0;
					blizzardppc_maprom();
					cyberstorm_copymaprom();
				}
			}
			addr /= 8;
			uae_u8 oldval = io_reg[addr];
			if (addr == CSIII_REG_LOCK) {
				uae_u8 regval = io_reg[CSIII_REG_LOCK] & 0x0f;
				if (v == P5_MAGIC1)
					regval |= P5_MAGIC1;
				else if ((v & 0x70) == P5_MAGIC2 && (io_reg[CSIII_REG_LOCK] & 0x70) == P5_MAGIC1)
					regval |= P5_MAGIC2;
				else if ((v & 0x70) == P5_MAGIC3 && (io_reg[CSIII_REG_LOCK] & 0x70) == P5_MAGIC2)
					regval |= P5_MAGIC3;
				else if ((v & 0x70) == P5_MAGIC4 && (io_reg[CSIII_REG_LOCK] & 0x70) == P5_MAGIC3)
					regval |= P5_MAGIC4;
				if ((regval & 0x70) == P5_MAGIC3)
					flash_unlocked = 1;
				else
					flash_unlocked &= ~2;
				if (v & P5_LOCK_CPU) {
					if (v & 0x80) {
						if (uae_ppc_cpu_unlock())
							regval |= P5_LOCK_CPU;
					} else {
						if (!(regval & P5_LOCK_CPU))
							uae_ppc_cpu_lock();
						regval &= ~P5_LOCK_CPU;
					}
#if CPUBOARD_IO_LOG > 0
					write_log(_T("CSIII_REG_LOCK bit 0 = %d!\n"), (v & 0x80) ? 1 : 0);
#endif
				}
				io_reg[CSIII_REG_LOCK] = regval;
			} else {
				uae_u32 regval;
				// reg shadow is only writable if unlock sequence is active
				if (addr == CSIII_REG_SHADOW) {
					if (v & 2) {
						// for some reason this unknown bit can be modified even when locked
						io_reg[CSIII_REG_LOCK] &= ~2;
						if (v & 0x80)
							io_reg[CSIII_REG_LOCK] |= 2;
					}
					if ((io_reg[CSIII_REG_LOCK] & 0x70) != P5_MAGIC3)
						return;
				}
				if (v & 0x80)
					io_reg[addr] |= v & 0x7f;
				else
					io_reg[addr] &= ~v;
				regval = io_reg[addr];
				if (addr == CSIII_REG_RESET) {
					map_banks(&dummy_bank, 0xf00000 >> 16, 0x80000 >> 16, 0);
					map_banks(&blizzardio_bank, 0xf60000 >> 16, 0x10000 >> 16, 0);
					if (regval & P5_SCSI_RESET) {
						if ((regval & P5_SCSI_RESET) != (oldval & P5_SCSI_RESET))
							write_log(_T("CS: SCSI reset cleared\n"));
						map_banks(&blizzardf0_bank, 0xf00000 >> 16, 0x40000 >> 16, 0);
						if (is_blizzardppc(&currprefs) || flash_size(flashrom) >= 262144) {
							map_banks(&ncr_bank_generic, 0xf40000 >> 16, 0x10000 >> 16, 0);
						} else {
							map_banks(&ncr_bank_cyberstorm, 0xf40000 >> 16, 0x10000 >> 16, 0);
							map_banks(&blizzardio_bank, 0xf50000 >> 16, 0x10000 >> 16, 0);
						}
					} else {
						if ((regval & P5_SCSI_RESET) != (oldval & P5_SCSI_RESET))
							write_log(_T("CS: SCSI reset\n"));
						map_banks(&blizzardf0_bank, 0xf00000 >> 16, 0x60000 >> 16, 0);
					}
					if (!(io_reg[CSIII_REG_SHADOW] & P5_SELF_RESET) || uae_self_is_ppc() == false) {
						if ((oldval & P5_PPC_RESET) && !(regval & P5_PPC_RESET)) {
							uae_ppc_cpu_stop();
						} else if (!(oldval & P5_PPC_RESET) && (regval & P5_PPC_RESET)) {
							uae_ppc_cpu_reboot();
						}
					} else {
						io_reg[CSIII_REG_RESET] &= ~P5_PPC_RESET;
						io_reg[CSIII_REG_RESET] |= oldval & P5_PPC_RESET;
					}
					if (!(io_reg[CSIII_REG_SHADOW] & P5_SELF_RESET) || uae_self_is_ppc() == true) {
						if ((regval & P5_M68K_RESET) && !(oldval & P5_M68K_RESET)) {
							m68k_reset();
							write_log(_T("CS: M68K Reset\n"));
						} else if (!(regval & P5_M68K_RESET) && (oldval & P5_M68K_RESET)) {
							write_log(_T("CS: M68K Halted\n"));
							if (!(regval & P5_PPC_RESET)) {
								write_log(_T("CS: PPC is also halted. STOP.\n"));
								cpu_halt(CPU_HALT_ALL_CPUS_STOPPED);
							} else {
								// halt 68k, leave ppc message processing active.
								cpu_halt(CPU_HALT_PPC_ONLY);
							}
						}
					} else {
						io_reg[CSIII_REG_RESET] &= ~P5_M68K_RESET;
						io_reg[CSIII_REG_RESET] |= oldval & P5_M68K_RESET;
					}
					if (!(io_reg[CSIII_REG_SHADOW] & P5_SELF_RESET)) {
						if (!(regval & P5_AMIGA_RESET)) {
							uae_ppc_cpu_stop();
							uae_reset(0, 0);
							write_log(_T("CS: Amiga Reset\n"));
							io_reg[addr] |= P5_AMIGA_RESET;
						}
					} else {
						io_reg[CSIII_REG_RESET] &= ~P5_AMIGA_RESET;
						io_reg[CSIII_REG_RESET] |= oldval & P5_AMIGA_RESET;
					}
				} else if (addr == CSIII_REG_IPL_EMU) {
					// M68K_IPL_MASK is read-only
					regval &= ~P5_M68k_IPL_MASK;
					regval |= oldval & P5_M68k_IPL_MASK;
					bool active = (regval & P5_DISABLE_INT) == 0;
					if (!active)
						clear_ppc_interrupt();
					io_reg[addr] = regval;
#if CPUBOARD_IRQ_LOG > 0
					if ((regval & P5_DISABLE_INT) != (oldval & P5_DISABLE_INT))
						write_log(_T("CS: interrupt state: %s\n"), (regval & P5_DISABLE_INT) ? _T("disabled") : _T("enabled"));
					if ((regval & P5_PPC_IPL_MASK) != (oldval & P5_PPC_IPL_MASK))
						write_log(_T("CS: PPC IPL %02x\n"), (~regval) & P5_PPC_IPL_MASK);
#endif
				} else if (addr == CSIII_REG_INT) {
#if CPUBOARD_IRQ_LOG > 0
					if ((regval & P5_INT_MASTER) != (oldval & P5_INT_MASTER))
						write_log(_T("CS: interrupt master: %s\n"), (regval & P5_INT_MASTER) ? _T("m68k") : _T("ppc"));
					if ((regval & P5_ENABLE_IPL) != (oldval & P5_ENABLE_IPL))
						write_log(_T("CS: IPL state: %s\n"), (regval & P5_ENABLE_IPL) ? _T("disabled") : _T("enabled"));
#endif
				} else if (addr == CSIII_REG_INT_LVL) {
#if CPUBOARD_IRQ_LOG > 0
					if (regval != oldval)
						write_log(_T("CS: interrupt level: %02x\n"), regval);
#endif
				} else if (addr == CSIII_REG_IRQ) {
#if CPUBOARD_IRQ_LOG > 0
					if (regval != oldval)
						write_log(_T("CS: IRQ: %02x\n"), regval);
#endif
				} else if (addr == CSIII_REG_SHADOW) {
					if (is_csmk3(&currprefs) && ((oldval ^ regval) & 1)) {
						maprom_state = (regval & P5_SHADOW) ? 0 : -1;
						write_log(_T("CyberStorm MAPROM = %d\n"), maprom_state);
						cyberstorm_copymaprom();
						cyberstorm_maprom();
					}
					if ((regval & P5_FLASH) != (oldval & P5_FLASH)) {
						flash_unlocked = (regval & P5_FLASH) ? 0 : 1;
						write_log(_T("CS: Flash writable = %d\n"), flash_unlocked);
					}
//					if ((oldval ^ regval) & 7) {
//						activate_debugger();
//					}
				}
				cpuboard_rethink();
			}
		}
	}
}
static void REGPARAM2 blizzardio_wput(uaecptr addr, uae_u32 v)
{
#if CPUBOARD_IO_LOG > 1
	write_log(_T("CS IO WPUT %08x %04x PC=%08x\n"), addr, v, M68K_GETPC);
#endif
	if (is_fusionforty(&currprefs)) {
		write_log(_T("FusionForty IO WPUT %08x %04x %08x\n"), addr, v, M68K_GETPC);
	} else if (is_blizzard(&currprefs)) {
		if((addr & 65535) == (BLIZZARD_BOARD_DISABLE & 65535)) {
			if (v != 0xcafe)
				return;
			write_log(_T("Blizzard: fallback CPU!\n"));
			cpu_fallback(0);
		}
	} else if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		write_log(_T("CS IO WPUT %08x %04x\n"), addr, v);
	} else if (is_csmk2(&currprefs) || is_blizzard2060(&currprefs)) {
		write_log(_T("CS IO WPUT %08x %04x\n"), addr, v);
		if (addr == CSMK2_2060_BOARD_DISABLE) {
			if (v != 0xcafe)
				return;
			write_log(_T("CSMK2/2060: fallback CPU!\n"));
			cpu_fallback(0);
		}
	} else if (is_a1230s2(&currprefs)) {
		extern void gvp_accelerator_set_dma_bank(uae_u8);
		io_reg[0] = v & 0xff;
		gvp_accelerator_set_dma_bank((v >> 4) & 3);
	}
}
static void REGPARAM2 blizzardio_lput(uaecptr addr, uae_u32 v)
{
#if CPUBOARD_IO_LOG > 1
	write_log(_T("CPU IO LPUT %08x %08x\n"), addr, v);
#endif
	if (is_csmk1(&currprefs)) {
		if (addr == 0x80f80000) {
			maprom_state = 1;
			cyberstormmk1_copymaprom();
		}
	}
	if (is_blizzard2060(&currprefs) && mapromconfigured()) {
		if (addr & 0x10000000) {
			maprom_state = 0;
		} else {
			maprom_state = 1;
		}
	}
}

#endif //WITH_CPUBOARD

static void cpuboard_hsync(void)
{
	// we should call check_ppc_int_lvl() immediately
	// after PPC CPU's interrupt flag is cleared but this
	// should be fast enough for now
#ifdef WITH_PPC
	if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		check_ppc_int_lvl();
	}
#endif
}

static void cpuboard_vsync(void)
{
	if (delayed_rom_protect <= 0)
		return;
	delayed_rom_protect--;
	if (delayed_rom_protect == 0)
		protect_roms(true);
}

void cpuboard_map(void)
{
	if (!currprefs.cpuboard_type)
		return;

	bool fallback_cpu = currprefs.cpu_model < 68020;
#ifdef WITH_CPUBOARD
	if (is_magnum40(&currprefs)) {
		map_banks(&blizzardio_bank, 0x0c0c0000 >> 16, 0x10000 >> 16, 0);
	}
	if (is_12gauge(&currprefs)) {
		map_banks(&cpuboardmem1_bank, cpuboardmem1_bank.start >> 16, 0x01000000 >> 16, (cpuboard_size / 2) >> 16);
		map_banks(&cpuboardmem2_bank, cpuboardmem2_bank.start >> 16, 0x01000000 >> 16, (cpuboard_size / 2) >> 16);
	}

	if (is_blizzard1230mk2(&currprefs) || is_blizzard1230mk3(&currprefs)) {
		map_banks(&blizzardram_bank, blizzardram_bank.start >> 16, cpuboard_size >> 16, 0);
		map_banks(&blizzardio_bank, BLIZZARD_MAPROM_ENABLE >> 16, 65536 >> 16, 0);
		if (is_blizzard1230mk2 (&currprefs) && cpuboard_size < 64 * 1024 * 1024 && cpuboard_size > 524288) {
			map_banks(&blizzardmaprom_bank, BLIZZARDMK2_MAPROM_BASE >> 16, 524288 >> 16, 0);
		}
		if (is_blizzard1230mk3(&currprefs) && cpuboard_size > 524288) {
			map_banks(&blizzardmaprom_bank, BLIZZARDMK3_MAPROM_BASE >> 16, 524288 >> 16, 0);
		}
	}
	if (is_blizzard(&currprefs) || is_blizzardppc(&currprefs)) {
		if (!fallback_cpu) {
			map_banks(&blizzardram_bank, blizzardram_bank.start >> 16, cpuboard_size >> 16, 0);
			// 1M + maprom uses different RAM mirror for some reason
			if (is_blizzard(&currprefs) && cpuboard_size == 2 * 524288)
				map_banks(&blizzardram_bank, (0x58000000 - cpuboard_size) >> 16, cpuboard_size >> 16, 0);
			if (!is_blizzardppc(&currprefs)) {
				if (cpuboard_size < 256 * 1024 * 1024 && cpuboard_size > 524288)
					map_banks(&blizzardmaprom_bank, BLIZZARDMK4_MAPROM_BASE >> 16, 524288 >> 16, 0);
				map_banks(&blizzardf0_bank, 0xf00000 >> 16, 65536 >> 16, 0);
				map_banks(&blizzardio_bank, BLIZZARD_MAPROM_ENABLE >> 16, 65536 >> 16, 0);
				map_banks(&blizzardio_bank, BLIZZARD_BOARD_DISABLE >> 16, 65536 >> 16, 0);
				map_banks(&blizzardio_bank, BLIZZARD_BOARD_DISABLE2 >> 16, 65536 >> 16, 0);
			} else {
				map_banks(&blizzardf0_bank, 0xf00000 >> 16, 0x60000 >> 16, 0);
				map_banks(&blizzardio_bank, 0xf60000 >> 16, (2 * 65536) >> 16, 0);
				blizzardppc_maprom();
			}
		} else {
			map_banks(&dummy_bank, 0xf00000 >> 16, 0x80000 >> 16, 0);
		}
	}
	if (is_csmk3(&currprefs)) {
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 0x40000 >> 16, 0);
		map_banks(&blizzardio_bank, 0xf50000 >> 16, (3 * 65536) >> 16, 0);
		cyberstorm_maprom();
	}
	if (is_csmk2(&currprefs)) {
		if (!fallback_cpu) {
			map_banks(&blizzardio_bank, 0x88000000 >> 16, 65536 >> 16, 0);
			map_banks(&blizzardio_bank, 0x83000000 >> 16, 65536 >> 16, 0);
			map_banks(&blizzardf0_bank, 0xf00000 >> 16, 65536 >> 16, 0);
			cyberstormmk2_maprom();
		} else {
			map_banks(&dummy_bank, 0xf00000 >> 16, 0x80000 >> 16, 0);
		}
	}
	if (is_csmk1(&currprefs)) {
		map_banks(&blizzardio_bank, 0x80f80000 >> 16, 65536 >> 16, 0);
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 65536 >> 16, 0);
		if (cpuboard_size > 524288)
			map_banks(&blizzardmaprom_bank, 0x07f80000 >> 16, 524288 >> 16, 0);
	}
	if (is_blizzard2060(&currprefs)) {
		if (!fallback_cpu) {
			map_banks(&blizzardf0_bank, 0xf00000 >> 16, 65536 >> 16, 0);
			map_banks(&blizzardio_bank, 0x80000000 >> 16, 0x10000000 >> 16, 0);
			if (mapromconfigured() && cpuboard_size > 524288)
				map_banks_nojitdirect(&blizzardmaprom_bank, (a3000hmem_bank.start + a3000hmem_bank.allocated_size - 524288) >> 16, 524288 >> 16, 0);
		} else {
			map_banks(&dummy_bank, 0xf00000 >> 16, 0x80000 >> 16, 0);
		}
	}
	if (is_quikpak(&currprefs) || is_trexii(&currprefs)) {
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 131072 >> 16, 0);
		map_banks(&blizzardea_bank, 0xf40000 >> 16, 65536 >> 16, 0);
	}
	if (is_fusionforty(&currprefs)) {
		map_banks(&blizzardf0_bank, 0x00f40000 >> 16, 131072 >> 16, 0);
		map_banks(&blizzardf0_bank, 0x05000000 >> 16, 131072 >> 16, 0);
		map_banks(&blizzardio_bank, 0x021d0000 >> 16, 65536 >> 16, 0);
		map_banks(&blizzardram_bank, blizzardram_bank.start >> 16, cpuboard_size >> 16, 0);
	}
	if (is_apollo12xx(&currprefs)) {
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 131072 >> 16, 0);
	}
	if (is_a1230s1(&currprefs)) {
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 65536 >> 16, 0);
	}
	if (is_dkb_wildfire(&currprefs)) {
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 0x80000 >> 16, 0);
	}
	if (is_dkb_12x0(&currprefs)) {
		if (cpuboard_size >= 4 * 1024 * 1024) {
			if (cpuboard_size <= 0x4000000) {
				map_banks(&blizzardram_bank, blizzardram_bank.start >> 16, 0x4000000 >> 16, cpuboard_size >> 16);
			} else {
				map_banks(&blizzardram_bank, blizzardram_bank.start >> 16, cpuboard_size >> 16, 0);
			}
		}
	}
	if (is_aca500(&currprefs)) {
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 524288 >> 16, 0);
		map_banks(&blizzardf0_bank, 0xa00000 >> 16, 524288 >> 16, 0);
		map_banks(&blizzardio_bank, 0xb00000 >> 16, 262144 >> 16, 0);
	}
	if (is_ivsvector(&currprefs)) {
		if (!currprefs.address_space_24) {
			map_banks(&cpuboardmem1_bank, cpuboardmem1_bank.start >> 16, (0x04000000 - cpuboardmem1_bank.start) >> 16, cpuboardmem1_bank.allocated_size);
			if (cpuboardmem2_bank.allocated_size)
				map_banks(&cpuboardmem2_bank, cpuboardmem2_bank.start >> 16, (0x10000000 - cpuboardmem2_bank.start) >> 16, cpuboardmem2_bank.allocated_size);
		}
	}

	if (is_mtec_ematrix530(&currprefs) || is_sx32pro(&currprefs) || is_apollo12xx(&currprefs) ||
		is_apollo630(&currprefs) || is_dce_typhoon2(&currprefs)) {
		if (cpuboardmem1_bank.allocated_size) {
			uae_u32 max = 0x08000000;
			// don't cross 0x08000000
			if (cpuboardmem1_bank.start < 0x08000000 && cpuboardmem1_bank.start + max > 0x08000000 && cpuboardmem1_bank.start + cpuboardmem1_bank.allocated_size < 0x08000000) {
				max = 0x08000000 - cpuboardmem1_bank.start;
			}
			map_banks(&cpuboardmem1_bank, cpuboardmem1_bank.start >> 16, max >> 16, cpuboardmem1_bank.allocated_size >> 16);
		}
		if (cpuboardmem2_bank.allocated_size && cpuboardmem2_bank.start < 0x18000000) {
			map_banks(&cpuboardmem2_bank, cpuboardmem2_bank.start >> 16, cpuboardmem2_bank.allocated_size >> 16, 0);
		}
	}

	if (is_a1230s1(&currprefs)) {
		if (cpuboardmem1_bank.allocated_size) {
			map_banks(&cpuboardmem1_bank, cpuboardmem1_bank.start >> 16, cpuboardmem1_bank.allocated_size >> 16, cpuboardmem1_bank.allocated_size >> 16);
		}
	}

	if (is_tekmagic(&currprefs)) {
		map_banks(&blizzardf0_bank, 0xf00000 >> 16, 131072 >> 16, 0);
		map_banks(&blizzardea_bank, 0xf40000 >> 16, 65536 >> 16, 0);
		if (cpuboardmem1_bank.allocated_size) {
			map_banks(&cpuboardmem1_bank, cpuboardmem1_bank.start >> 16, cpuboardmem1_bank.allocated_size >> 16, cpuboardmem1_bank.allocated_size >> 16);
		}
	}

	if (is_falcon40(&currprefs)) {
		map_banks(&blizzardram_bank, blizzardram_bank.start >> 16, cpuboard_size >> 16, 0);
	}

	if (is_a1230s2(&currprefs)) {
		map_banks(&blizzardio_bank, 0x03000000 >> 16, 1, 0);
	}
#endif //WITH_CPUBOARD
}

bool cpuboard_forced_hardreset(void)
{
	if (is_blizzardppc(&currprefs)) {
		return true;
	}
	return false;
}

void cpuboard_reset(int hardreset)
{
#if 0
	if (is_blizzard() || is_blizzardppc())
		canbang = 0;
#endif
	configured = false;
	delayed_rom_protect = 0;
	currprefs.cpuboardmem1.size = changed_prefs.cpuboardmem1.size;
	maprom_unwriteprotect();
	if (hardreset || (!mapromconfigured() && (is_blizzard(&currprefs) || is_blizzard2060(&currprefs))))
		maprom_state = 0;
	if (is_csmk3(&currprefs) || is_blizzardppc(&currprefs)) {
		if (hardreset)
			memset(io_reg, 0x7f, sizeof io_reg);
		io_reg[CSIII_REG_RESET] = 0x7f;
		io_reg[CSIII_REG_IRQ] = 0x7f;
		io_reg[CSIII_REG_IPL_EMU] = 0x40;
		io_reg[CSIII_REG_LOCK] = 0x01;
	}
	if (hardreset || is_keyboardreset())
		a2630_io = 0;

	flash_unlocked = 0;
	cpuboard_non_byte_ea = false;

	flash_free(flashrom);
	flashrom = NULL;
	flash_free(flashrom2);
	flashrom2 = NULL;
	zfile_fclose(flashrom_file);
	flashrom_file = NULL;
}

void cpuboard_cleanup(void)
{
	configured = false;
	maprom_state = 0;

	flash_free(flashrom);
	flashrom = NULL;
	flash_free(flashrom2);
	flashrom2 = NULL;
	zfile_fclose(flashrom_file);
	flashrom_file = NULL;

//	if (blizzard_jit) {
#ifdef WITH_CPUBOARD
		mapped_free(&blizzardram_bank);
#endif //WITH_CPUBOARD
#if 0
} else {
		if (blizzardram_nojit_bank.baseaddr) {
#ifdef CPU_64_BIT
			uae_vm_free(blizzardram_nojit_bank.baseaddr, blizzardram_nojit_bank.allocated);
#else
			xfree(blizzardram_nojit_bank.baseaddr);
#endif
		}
	}
#endif
#ifdef WITH_CPUBOARD
	if (blizzardmaprom_bank_mapped)
		mapped_free(&blizzardmaprom_bank);
	if (blizzardmaprom2_bank_mapped)
		mapped_free(&blizzardmaprom2_bank);
#endif //WITH_CPUBOARD
	maprom_unwriteprotect();

#ifdef WITH_CPUBOARD
	blizzardram_bank.baseaddr = NULL;
//	blizzardram_nojit_bank.baseaddr = NULL;
	blizzardmaprom_bank.baseaddr = NULL;
	blizzardmaprom2_bank.baseaddr = NULL;
	blizzardmaprom_bank_mapped = false;
	blizzardmaprom2_bank_mapped = false;

	mapped_free(&blizzardf0_bank);
	blizzardf0_bank.baseaddr = NULL;

	mapped_free(&blizzardea_bank);
	blizzardea_bank.baseaddr = NULL;
#endif //WITH_CPUBOARD

	cpuboard_size = cpuboard2_size = -1;

#ifdef WITH_CPUBOARD
	blizzardmaprom_bank.flags &= ~(ABFLAG_INDIRECT | ABFLAG_NOALLOC);
	blizzardmaprom2_bank.flags &= ~(ABFLAG_INDIRECT | ABFLAG_NOALLOC);
#endif //WITH_CPUBOARD

	mapped_free(&cpuboardmem1_bank);
	mapped_free(&cpuboardmem2_bank);
	cpuboardmem1_bank.jit_read_flag = 0;
	cpuboardmem1_bank.jit_write_flag = 0;
	cpuboardmem2_bank.jit_read_flag = 0;
	cpuboardmem2_bank.jit_write_flag = 0;
}

static void memory_mirror_bank(addrbank *bank, uaecptr end_addr)
{
	uaecptr addr = bank->start;
	while (addr + bank->allocated_size < end_addr) {
		map_banks(bank, (addr + bank->allocated_size) >> 16, bank->allocated_size >> 16, 0);
		addr += bank->allocated_size;
	}
}

static void cpuboard_custom_memory(uaecptr addr, int max, bool swap, bool alias)
{
	int size1 = swap ? currprefs.cpuboardmem2.size : currprefs.cpuboardmem1.size;
	int size2 = swap ? currprefs.cpuboardmem1.size : currprefs.cpuboardmem2.size;
	int size_low = size1;
	max *= 1024 * 1024;
	if (size_low > max)
		size_low = max;
	int size_high;
	if (size2 == 0 && size1 > max) {
		size_high = size1 - max;
	} else {
		size_high = size2;
	}
	if (!size_low && !size_high)
		return;
	cpuboardmem1_bank.start = addr - size_low;
	cpuboardmem1_bank.reserved_size = size_low + size_high;
	cpuboardmem1_bank.mask = cpuboardmem1_bank.reserved_size - 1;
	mapped_malloc(&cpuboardmem1_bank);
}


static void cpuboard_init_2(void)
{
	if (!currprefs.cpuboard_type)
		return;

	cpuboard_size = currprefs.cpuboardmem1.size;
	cpuboardmem1_bank.reserved_size = 0;
	cpuboardmem2_bank.reserved_size = 0;
#ifdef WITH_CPUBOARD

	if (is_kupke(&currprefs) || is_mtec_ematrix530(&currprefs) || is_sx32pro(&currprefs) || is_dce_typhoon2(&currprefs) || is_apollo630(&currprefs)) {
		// plain 64k autoconfig, nothing else.
		blizzardea_bank.reserved_size = 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

		if (is_mtec_ematrix530(&currprefs) || is_sx32pro(&currprefs) || is_dce_typhoon2(&currprefs)  || is_apollo630(&currprefs)) {
			if (cpuboard_size == 2 * 1024 * 1024 || cpuboard_size == 8 * 1024 * 1024 || cpuboard_size == 32 * 1024 * 1024 || is_sx32pro(&currprefs)) {
				cpuboardmem1_bank.start = 0x18000000;
				cpuboardmem1_bank.reserved_size = cpuboard_size / 2;
				cpuboardmem1_bank.mask = cpuboardmem1_bank.reserved_size - 1;
				mapped_malloc(&cpuboardmem1_bank);
				cpuboardmem2_bank.start = 0x18000000 + cpuboard_size / 2;
				cpuboardmem2_bank.reserved_size = cpuboard_size / 2;
				cpuboardmem2_bank.mask = cpuboardmem2_bank.reserved_size - 1;
				mapped_malloc(&cpuboardmem2_bank);
			} else if (cpuboard_size == 128 * 1024 * 1024) {
				cpuboardmem1_bank.start = 0x18000000;
				cpuboardmem1_bank.reserved_size = cpuboard_size / 2;
				cpuboardmem1_bank.mask = cpuboardmem1_bank.reserved_size - 1;
				mapped_malloc(&cpuboardmem1_bank);
				cpuboardmem2_bank.start = 0x18000000 - cpuboard_size / 2;
				cpuboardmem2_bank.reserved_size = cpuboard_size / 2;
				cpuboardmem2_bank.mask = cpuboardmem2_bank.reserved_size - 1;
				mapped_malloc(&cpuboardmem2_bank);
			} else {
				cpuboardmem1_bank.start = 0x18000000;
				cpuboardmem1_bank.reserved_size = cpuboard_size;
				cpuboardmem1_bank.mask = cpuboardmem1_bank.reserved_size - 1;
				mapped_malloc(&cpuboardmem1_bank);
			}
		}

	} else if (is_a1230s1(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 65536;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

	} else if (is_aca500(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 524288;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

	} else if (is_a2630(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 131072;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		blizzardea_bank.reserved_size = 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

	} else if (is_harms_3kp(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 65536;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		blizzardea_bank.reserved_size = 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

	} else if (is_dkb_12x0(&currprefs)) {

		blizzardram_bank.start = 0x10000000;
		blizzardram_bank.reserved_size = cpuboard_size;
		blizzardram_bank.mask = blizzardram_bank.reserved_size - 1;
		blizzardram_bank.startmask = 0x10000000;
		blizzardram_bank.label = _T("dkb");
		mapped_malloc(&blizzardram_bank);

	} else if (is_tqm(&currprefs)) {

		blizzardea_bank.reserved_size = 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

	} else if (is_falcon40(&currprefs)) {

		blizzardea_bank.reserved_size = 262144;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

		blizzardram_bank.start = 0x10000000;
		blizzardram_bank.reserved_size = cpuboard_size;
		blizzardram_bank.mask = blizzardram_bank.reserved_size - 1;
		blizzardram_bank.startmask = 0x10000000;
		blizzardram_bank.label = _T("tqm");
		mapped_malloc(&blizzardram_bank);

	} else if (is_dkb_wildfire(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 65536;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

	} else if (is_apollo12xx(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 131072;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		cpuboard_custom_memory(0x03000000, 32, false, true);

	} else if (is_fusionforty(&currprefs)) {

		blizzardf0_bank.start = 0x00f40000;
		blizzardf0_bank.reserved_size = 262144;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		blizzardram_bank.start = 0x11000000;
		blizzardram_bank.reserved_size = cpuboard_size;
		blizzardram_bank.mask = blizzardram_bank.reserved_size - 1;
		blizzardram_bank.startmask = 0x11000000;
		blizzardram_bank.label = _T("fusionforty");
		mapped_malloc(&blizzardram_bank);

	} else if (is_tekmagic(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 131072;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		blizzardea_bank.reserved_size = 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

		cpuboardmem1_bank.start = 0x02000000;
		cpuboardmem1_bank.reserved_size = cpuboard_size;
		cpuboardmem1_bank.mask = cpuboardmem1_bank.reserved_size - 1;
		mapped_malloc(&cpuboardmem1_bank);

	} else if (is_quikpak(&currprefs) || is_trexii(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 131072;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		blizzardea_bank.reserved_size = 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

	} else if (is_blizzard1230mk2(&currprefs)) {

		blizzardea_bank.reserved_size = 2 * 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

		blizzardram_bank.start = 0x0e000000 - cpuboard_size / 2;
		blizzardram_bank.reserved_size = cpuboard_size;
		blizzardram_bank.mask = blizzardram_bank.reserved_size - 1;
		if (cpuboard_size) {
			blizzardram_bank.label = _T("*");
			mapped_malloc(&blizzardram_bank);
		}

		blizzardmaprom_bank.baseaddr = blizzardram_bank.baseaddr + cpuboard_size - 524288;
		blizzardmaprom_bank.start = BLIZZARDMK2_MAPROM_BASE;
		blizzardmaprom_bank.reserved_size = 524288;
		blizzardmaprom_bank.mask = 524288 - 1;
		blizzardmaprom_bank.flags |= ABFLAG_INDIRECT | ABFLAG_NOALLOC;
		mapped_malloc(&blizzardmaprom_bank);
		blizzardmaprom_bank_mapped = true;

		maprom_base = blizzardram_bank.reserved_size - 524288;

	} else if (is_blizzard1230mk3(&currprefs)) {

		blizzardea_bank.reserved_size = 2 * 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

		blizzardram_bank.start = 0x1e000000 - cpuboard_size / 2;
		blizzardram_bank.reserved_size = cpuboard_size;
		blizzardram_bank.mask = blizzardram_bank.reserved_size - 1;
		if (cpuboard_size) {
			blizzardram_bank.label = _T("*");
			mapped_malloc(&blizzardram_bank);
		}

		blizzardmaprom_bank.baseaddr = blizzardram_bank.baseaddr + cpuboard_size - 524288;
		blizzardmaprom_bank.start = BLIZZARDMK3_MAPROM_BASE;
		blizzardmaprom_bank.reserved_size = 524288;
		blizzardmaprom_bank.mask = 524288 - 1;
		blizzardmaprom_bank.flags |= ABFLAG_INDIRECT | ABFLAG_NOALLOC;
		mapped_malloc(&blizzardmaprom_bank);
		blizzardmaprom_bank_mapped = true;

		maprom_base = blizzardram_bank.reserved_size - 524288;


	} else if (is_blizzard(&currprefs) || is_blizzardppc(&currprefs)) {

		blizzard_jit = 1;
		blizzardram_bank.start = BLIZZARDMK4_RAM_BASE_48 - cpuboard_size / 2;
		blizzardram_bank.reserved_size = cpuboard_size;
		blizzardram_bank.mask = blizzardram_bank.reserved_size - 1;
		if (cpuboard_size) {
			blizzardram_bank.label = _T("*");
			mapped_malloc(&blizzardram_bank);
		}

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 524288;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		if (!is_blizzardppc(&currprefs)) {
			blizzardea_bank.reserved_size = 2 * 65536;
			blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
			// Blizzard 12xx autoconfig ROM must be mapped at $ea0000-$ebffff, board requires it.
			mapped_malloc(&blizzardea_bank);
		}

		if (cpuboard_size > 524288) {
			if (!is_blizzardppc(&currprefs)) {
				blizzardmaprom_bank.baseaddr = blizzardram_bank.baseaddr + cpuboard_size - 524288;
				blizzardmaprom_bank.start = BLIZZARDMK4_MAPROM_BASE;
				blizzardmaprom_bank.reserved_size = 524288;
				blizzardmaprom_bank.mask = 524288 - 1;
				blizzardmaprom_bank.flags |= ABFLAG_INDIRECT | ABFLAG_NOALLOC;
				mapped_malloc(&blizzardmaprom_bank);
				blizzardmaprom_bank_mapped = true;
			}
		}

		maprom_base = blizzardram_bank.reserved_size - 524288;

	} else if (is_csmk1(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 65536;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		blizzardea_bank.reserved_size = 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

		blizzardmaprom_bank.reserved_size = 524288;
		blizzardmaprom_bank.start = 0x07f80000;
		blizzardmaprom_bank.mask = 524288 - 1;
		blizzardmaprom_bank_mapped = true;
		mapped_malloc(&blizzardmaprom_bank);

	} else if (is_csmk2(&currprefs) || is_blizzard2060(&currprefs)) {

		blizzardea_bank.reserved_size = 2 * 65536;
		blizzardea_bank.mask = blizzardea_bank.reserved_size - 1;
		mapped_malloc(&blizzardea_bank);

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 65536;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

		if (a3000hmem_bank.baseaddr) {
			blizzardmaprom_bank.baseaddr = a3000hmem_bank.baseaddr + a3000hmem_bank.reserved_size - 524288;
			blizzardmaprom_bank.start = a3000hmem_bank.start + a3000hmem_bank.reserved_size - 524288;
			blizzardmaprom_bank.reserved_size = 524288;
			blizzardmaprom_bank.mask = 524288 - 1;
			blizzardmaprom_bank.flags |= ABFLAG_INDIRECT | ABFLAG_NOALLOC;
			mapped_malloc(&blizzardmaprom_bank);
		}

	} else if (is_csmk3(&currprefs)) {

		blizzardf0_bank.start = 0x00f00000;
		blizzardf0_bank.reserved_size = 524288;
		blizzardf0_bank.mask = blizzardf0_bank.reserved_size - 1;
		mapped_malloc(&blizzardf0_bank);

	} else if (is_ivsvector(&currprefs)) {
	
		cpuboardmem1_bank.start = 0x02000000;
		cpuboardmem1_bank.reserved_size = 32 * 1024 * 1024;
		cpuboardmem1_bank.mask = cpuboardmem1_bank.reserved_size - 1;
		mapped_malloc(&cpuboardmem1_bank);

		cpuboardmem2_bank.start = 0x0c000000;
		cpuboardmem2_bank.reserved_size = cpuboard_size;
		cpuboardmem2_bank.mask = cpuboard_size - 1;
		mapped_malloc(&cpuboardmem2_bank);

	} else if (is_12gauge(&currprefs)) {

		cpuboardmem1_bank.start = 0x08000000;
		cpuboardmem1_bank.reserved_size = cpuboard_size / 2;
		cpuboardmem1_bank.mask = cpuboardmem1_bank.reserved_size - 1;

		cpuboardmem2_bank.start = 0x09000000;
		cpuboardmem2_bank.reserved_size = cpuboard_size / 2;
		cpuboardmem2_bank.mask = cpuboardmem2_bank.reserved_size - 1;

		if (cpuboard_size) {
			cpuboardmem1_bank.label = _T("*");
			mapped_malloc(&cpuboardmem1_bank);
			cpuboardmem2_bank.label = _T("*");
			mapped_malloc(&cpuboardmem2_bank);
		}

	}

#endif //WITH_CPUBOARD

	if (!cpuboardmem1_bank.baseaddr)
		cpuboardmem1_bank.reserved_size = 0;
	if (!cpuboardmem2_bank.baseaddr)
		cpuboardmem2_bank.reserved_size = 0;
}

void cpuboard_init(void)
{
	if (!currprefs.cpuboard_type)
		return;

	if (cpuboard_size == currprefs.cpuboardmem1.size)
		return;

	cpuboard_cleanup();
	cpuboard_init_2();
}

void cpuboard_overlay_override(void)
{
	if (is_a2630(&currprefs) || is_harms_3kp(&currprefs)) {
#ifdef WITH_CPUBOARD
		if (!(a2630_io & 2))
			map_banks(&blizzardf0_bank, 0xf80000 >> 16, f0rom_size >> 16, 0);
#endif //WITH_CPUBOARD
		if (mem25bit_bank.allocated_size)
			map_banks(&chipmem_bank, (mem25bit_bank.start + mem25bit_bank.allocated_size) >> 16, (1024 * 1024) >> 16, 0);
		else
			map_banks(&chipmem_bank, 0x01000000 >> 16, (1024 * 1024) >> 16, 0);
	}
}

void cpuboard_clear(void)
{
#ifdef WITH_CPUBOARD
	if (blizzardmaprom_bank.baseaddr)
		memset(blizzardmaprom_bank.baseaddr, 0, 524288);
	if (blizzardmaprom2_bank.baseaddr)
		memset(blizzardmaprom2_bank.baseaddr, 0, 524288);
	if (is_csmk3(&currprefs)) // SCSI RAM
		memset(blizzardf0_bank.baseaddr + 0x40000, 0, 0x10000);
#endif
}

// Adds resource resident that CSPPC/BPPC flash updater checks.

#define FAKEPPCROM_OFFSET 32
static const uae_u8 fakeppcromstart[] = {
	0x11, 0x11,
	// moveq #2,d0
	0x70, 0x02,
	// movec d0,pcr
	0x4e, 0x7b, 0x08, 0x08,
	// jmp (a5)
	0x4e, 0xd5
};

static const uae_u8 fakeppcrom[] = {
	// struct Resident
	0x4a, 0xfc,
	0x00, 0xf0, 0x00, FAKEPPCROM_OFFSET,
	0x00, 0xf0, 0x01, 0x00,
	0x02, 0x01, 0x00, 0x78,
	0x00, 0xf0, 0x00, FAKEPPCROM_OFFSET + 30,
	0x00, 0xf0, 0x00, FAKEPPCROM_OFFSET + 30,
	0x00, 0xf0, 0x00, FAKEPPCROM_OFFSET + 26,
	// moveq #0,d0
	0x70, 0x00,
	// rts
	0x4e, 0x75,
};
static const char fakeppcromtxt_cs[] = { "CyberstormPPC.IDTag" };
static const char fakeppcromtxt_bz[] = { "BlizzardPPC.IDTag" };

static void makefakeppcrom(uae_u8 *rom, int type)
{
	memset(rom, 0, FAKEPPCROM_OFFSET);
	// 68060: disable FPU because we don't have ROM that handles it.
	if (currprefs.fpu_model == 68060) {
		memcpy(rom, fakeppcromstart, sizeof fakeppcromstart);
	}
	memcpy(rom + FAKEPPCROM_OFFSET, fakeppcrom, sizeof fakeppcrom);
	const char *txt = type ? fakeppcromtxt_bz : fakeppcromtxt_cs;
	memcpy(rom + FAKEPPCROM_OFFSET + sizeof fakeppcrom, txt, strlen(txt) + 1);
}

bool is_ppc_cpu(struct uae_prefs *p)
{
	return is_ppc(p);
}

bool cpuboard_maprom(void)
{
	if (!currprefs.cpuboard_type || !cpuboard_size)
		return false;
#ifdef WITH_CPUBOARD
	if (is_blizzard(&currprefs) || is_blizzardppc(&currprefs)) {
		if (maprom_state)
			blizzard_copymaprom();
	} else if (is_csmk3(&currprefs)) {
		if (maprom_state)
			cyberstorm_copymaprom();
	}
#endif
	return true;
}

bool cpuboard_jitdirectompatible(struct uae_prefs *p)
{
	if (cpuboard_memorytype(p) == BOARD_MEMORY_BLIZZARD_12xx || cpuboard_memorytype(p) == BOARD_MEMORY_BLIZZARD_PPC) {
		return false;
	}
	return true;
}

bool cpuboard_32bit(struct uae_prefs *p)
{
	int b = cpuboard_memorytype(p);
	if (p->cpuboard_type) {
		if (cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype].deviceflags & EXPANSIONTYPE_HAS_FALLBACK)
			return false;
		if (!(cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype].deviceflags & EXPANSIONTYPE_24BIT))
			return true;
	}
	return b == BOARD_MEMORY_HIGHMEM ||
		b == BOARD_MEMORY_BLIZZARD_12xx ||
		b == BOARD_MEMORY_BLIZZARD_PPC ||
		b == BOARD_MEMORY_Z3 ||
		b == BOARD_MEMORY_25BITMEM ||
		b == BOARD_MEMORY_CUSTOM_32;
}

int cpuboard_memorytype(struct uae_prefs *p)
{
	if (cpuboards[p->cpuboard_type].subtypes)
		return cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype].memorytype;
	return 0;
}

int cpuboard_maxmemory(struct uae_prefs *p)
{
	if (cpuboards[p->cpuboard_type].subtypes)
		return cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype].maxmemory;
	return 0;
}

void cpuboard_setboard(struct uae_prefs *p, int type, int subtype)
{
	p->cpuboard_type = 0;
	p->cpuboard_subtype = 0;
	for (int i = 0; cpuboards[i].name; i++) {
		if (cpuboards[i].id == type) {
			p->cpuboard_type = type;
			if (subtype >= 0)
				p->cpuboard_subtype = subtype;
			return;
		}
	}
}

uaecptr cpuboard_get_reset_pc(uaecptr *stack)
{
	if (is_aca500(&currprefs)) {
		*stack = get_long(0xa00000);
		return get_long(0xa00004);
	} else {
		*stack = get_long(0);
		return get_long(4);
	}
}

bool cpuboard_io_special(int addr, uae_u32 *val, int size, bool write)
{
	addr &= 65535;
	if (write) {
		uae_u16 w = *val;
		if (is_a2630(&currprefs) || is_harms_3kp(&currprefs)) {
			if ((addr == 0x0040 && size == 2) || (addr == 0x0041 && size == 1)) {
				write_log(_T("A2630 write %04x s=%d PC=%08x\n"), w, size, M68K_GETPC);
				a2630_io = w;
				if (is_harms_3kp(&currprefs)) {
					w |= 2;
				}
				// bit 0: unmap 0x000000
				// bit 1: unmap 0xf80000
				if (w & 2) {
					if (currprefs.mmu_model == 68030) {
						// HACK!
#ifdef CPUEMU_22
						mmu030_fake_prefetch = 0x4ed0;
#endif
					}
					map_banks(&kickmem_bank, 0xF8, 8, 0);
					write_log(_T("A2630 boot rom unmapped\n"));
				}
				// bit 2: autoconfig region enable
				if (w & 4) {
					write_log(_T("A2630 Autoconfig enabled\n"));
					expamem_next(NULL, NULL);
				}
				// bit 3: unknown
				// bit 4: 68000 mode
				if (w & 0x10) {
					write_log(_T("A2630 fallback CPU mode!\n"));
					cpu_fallback(0);
				}
				return true;
			}
		}
		return false;
	} else {
		if (is_a2630(&currprefs) || is_harms_3kp(&currprefs)) {
			// osmode (j304)
			if (addr == 0x0c && (a2630_io & 4) == 0) {
				(*val) |= 0x80;
				if (currprefs.cpuboard_settings & 1)
					(*val) &= ~0x80;
				return true;
			}
		}
	}
	return false;
}

bool cpuboard_fc_check(uaecptr addr, uae_u32 *v, int size, bool write)
{
	if (!currprefs.cpuboard_type)
		return false;

	if (is_12gauge(&currprefs)) {
		if ((addr == 0xc0000 || addr == 0xc0001) && size == 0) {
			uae_u8 val = 0;
			if (!write) {
				if (addr == 0xc0000)
					val = 0x00;
				if (addr == 0xc0001)
					val = 0x40; // bit 6 set = scsi driver at base+$c000  (0 = $8000000)
				*v = val;
			}
			if (write)
				write_log(_T("12GAUGE W %08x = %02x\n"), addr, (*v) & 0xff);
			else
				write_log(_T("12GAUGE R %08x = %02x\n"), addr, (*v) & 0xff);
			return true;
		}
		return false;
	}

	return false;
}

static void fixserial(struct uae_prefs *p, uae_u8 *rom, int size)
{
	uae_u8 value1 = rom[16];
	uae_u8 value2 = rom[17];
	uae_u8 value3 = rom[18];
	int seroffset = 17, longseroffset = 24;
	uae_u32 serialnum = 0x1234;
	char serial[10];
	int type = -1;

	if (ISCPUBOARDP(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC)) {
		value1 = 'I';
		value2 = 'D';
		if (currprefs.cpu_model == 68060)
			value3 = 'H';
		else if (currprefs.fpu_model)
			value3 = 'B';
		else
			value3 = 'A';
		seroffset = 19;
		_sntprintf(serial, sizeof serial, "%04X", serialnum);
		type = 1;
	} else if (ISCPUBOARDP(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_PPC)) {
		value1 = 'D';
		value2 = 'B';
		_sntprintf(serial, sizeof serial, "%05X", serialnum);
		value3 = 0;
		seroffset = 18;
		type = 0;
	} else if (ISCPUBOARDP(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK3)) {
		value1 = 'F';
		_sntprintf(serial, sizeof serial, "%05X", serialnum);
		value2 = value3 = 0;
	} else {
		return;
	}

	rom[16] = value1;
	if (value2)
		rom[17] = value2;
	if (value3)
		rom[18] = value3;
	if (rom[seroffset] == 0) {
		strcpy((char*)rom + seroffset, serial);
		if (longseroffset) {
			rom[longseroffset + 0] = serialnum >> 24;
			rom[longseroffset + 1] = serialnum >> 16;
			rom[longseroffset + 2] = serialnum >>  8;
			rom[longseroffset + 3] = serialnum >>  0;
		}
	}

	if (type >= 0 && rom[0] == 0 && rom[1] == 0)
		makefakeppcrom(rom, type);
}

static struct zfile *board_rom_open(int romtype, const TCHAR *name)
{
	struct zfile *zf = NULL;
	struct romlist *rl = getromlistbyromtype(romtype, name);
	if (rl)
		zf = read_rom(rl->rd);
	if (!zf && name) {
		zf = zfile_fopen(name, _T("rb"), ZFD_NORMAL);
		if (zf) {
			return zf;
		}
		TCHAR path[MAX_DPATH];
		get_rom_path(path, sizeof path / sizeof(TCHAR));
		_tcscat(path, name);
		zf = zfile_fopen(path, _T("rb"), ZFD_NORMAL);
	}
	return zf;
}

static void ew(uae_u8 *p, int addr, uae_u8 value)
{
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		p[addr] = (value & 0xf0);
		p[addr + 2] = (value & 0x0f) << 4;
	} else {
		p[addr] = ~(value & 0xf0);
		p[addr + 2] = ~((value & 0x0f) << 4);
	}
}

bool cpuboard_autoconfig_init(struct autoconfig_info *aci)
{
	struct zfile *autoconfig_rom = NULL;
	struct boardromconfig *brc;
	bool autoconf = true;
	bool autoconf_stop = false;
	bool autoconfig_nomandatory = false;
	const TCHAR *defaultromname = NULL;
	const TCHAR *romname = NULL;
	bool isflashrom = false;
	struct romdata *rd = NULL;
	const TCHAR *boardname;
	struct uae_prefs *p = aci->prefs;
	uae_u32 romtype = 0, romtype2 = 0;
	uae_u8 autoconfig_data[16] = { 0 };

	boardname = cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype].name;
	aci->label = boardname;

	int idx;
	brc = get_device_rom(p, ROMTYPE_CPUBOARD, 0, &idx);
	if (brc)
		romname = brc->roms[idx].romfile;

	device_add_reset(cpuboard_reset);

	cpuboard_non_byte_ea = false;
	int boardid = cpuboards[p->cpuboard_type].id;
	switch (boardid)
	{
//		case BOARD_IC:
//		break;

		case BOARD_IVS:
		switch (p->cpuboard_subtype)
		{
			case BOARD_IVS_SUB_VECTOR:
			aci->addrbank = &expamem_null;
			return true;
		}
		break;

		case BOARD_COMMODORE:
		switch(p->cpuboard_subtype)
		{
			case BOARD_COMMODORE_SUB_A26x0:
			romtype = ROMTYPE_CB_A26x0;
			break;
		}
		break;

		case BOARD_HARMS:
		switch(p->cpuboard_subtype)
		{
			case BOARD_HARMS_SUB_3KPRO:
			romtype = ROMTYPE_CB_HARMS3KP;
			break;
		}
		break;

		case BOARD_ACT:
		switch(p->cpuboard_subtype)
		{
			case BOARD_ACT_SUB_APOLLO_12xx:
			romtype = ROMTYPE_CB_APOLLO_12xx;
			break;
			case BOARD_ACT_SUB_APOLLO_630:
			romtype = ROMTYPE_CB_APOLLO_630;
			break;
		}
		break;

		case BOARD_MTEC:
		switch (p->cpuboard_subtype)
		{
		case BOARD_MTEC_SUB_EMATRIX530:
			romtype = ROMTYPE_CB_EMATRIX;
			break;
		}
		break;

		case BOARD_MACROSYSTEM:
		switch (p->cpuboard_subtype)
		{
		case BOARD_MACROSYSTEM_SUB_WARPENGINE_A4000:
			aci->addrbank = &expamem_null;
			return true;
		case BOARD_MACROSYSTEM_SUB_FALCON040:
			romtype = ROMTYPE_CB_FALCON40;
			break;
		case BOARD_MACROSYSTEM_SUB_DRACO:
			return false;
		}
		break;

		case BOARD_PPS:
		switch(p->cpuboard_subtype)
		{
		case BOARD_PPS_SUB_ZEUS040:
			aci->addrbank = &expamem_null;
			return true;
		}
		break;

		case BOARD_CSA:
		switch(p->cpuboard_subtype)
		{
		case BOARD_CSA_SUB_12GAUGE:
			aci->addrbank = &expamem_null;
			return true;
		case BOARD_CSA_SUB_MAGNUM40:
			aci->addrbank = &expamem_null;
			return true;
		}
		break;

		case BOARD_DKB:
		switch(p->cpuboard_subtype)
		{
			case BOARD_DKB_SUB_12x0:
			aci->addrbank = &expamem_null;
			return true;
			case BOARD_DKB_SUB_WILDFIRE:
			romtype = ROMTYPE_CB_DBK_WF;
			break;
		}
		break;

		case BOARD_RCS:
		switch(p->cpuboard_subtype)
		{
			case BOARD_RCS_SUB_FUSIONFORTY:
			romtype = ROMTYPE_CB_FUSION;
			break;
		}
		break;
	
		case BOARD_GVP:
		switch(p->cpuboard_subtype)
		{
			case BOARD_GVP_SUB_A530:
			case BOARD_GVP_SUB_A1230SII:
			case BOARD_GVP_SUB_GFORCE030:
			case BOARD_GVP_SUB_GFORCE040:
				aci->addrbank = &expamem_null;
				return true;
			case BOARD_GVP_SUB_A3001SI:
			case BOARD_GVP_SUB_A3001SII:
				aci->addrbank = &expamem_null;
				return true;
			case BOARD_GVP_SUB_TEKMAGIC:
				romtype = ROMTYPE_CB_TEKMAGIC;
				break;
			case BOARD_GVP_SUB_QUIKPAK:
				romtype = romtype = ROMTYPE_CB_QUIKPAK;
				break;
			case BOARD_GVP_SUB_A1230SI:
				romtype = romtype = ROMTYPE_CB_A1230S1;
				break;
		}
		break;

		case BOARD_CYBERSTORM:
		switch(p->cpuboard_subtype)
		{
			case BOARD_CYBERSTORM_SUB_MK1:
				romtype = ROMTYPE_CB_CSMK1;
				romtype2 = ROMTYPE_CSMK1SCSI;
				autoconfig_nomandatory = true;
				break;
			case BOARD_CYBERSTORM_SUB_MK2:
				romtype = ROMTYPE_CB_CSMK2;
				isflashrom = true;
				break;
			case BOARD_CYBERSTORM_SUB_MK3:
				romtype = ROMTYPE_CB_CSMK3;
				isflashrom = true;
				break;
			case BOARD_CYBERSTORM_SUB_PPC:
				romtype = ROMTYPE_CB_CSPPC;
				isflashrom = true;
				break;
		}
		break;

		case BOARD_BLIZZARD:
		switch(p->cpuboard_subtype)
		{
			case BOARD_BLIZZARD_SUB_1230II:
				romtype = ROMTYPE_CB_B1230MK2;
				break;
			case BOARD_BLIZZARD_SUB_1230III:
				romtype = ROMTYPE_CB_B1230MK3;
				romtype2 = ROMTYPE_BLIZKIT3;
				break;
			case BOARD_BLIZZARD_SUB_1230IV:
				romtype = ROMTYPE_CB_BLIZ1230;
				romtype2 = ROMTYPE_BLIZKIT4;
				break;
			case BOARD_BLIZZARD_SUB_1260:
				romtype = ROMTYPE_CB_BLIZ1260;
				romtype2 = ROMTYPE_BLIZKIT4;
				break;
			case BOARD_BLIZZARD_SUB_2060:
				romtype = ROMTYPE_CB_BLIZ2060;
				break;
			case BOARD_BLIZZARD_SUB_PPC:
				romtype = ROMTYPE_CB_BLIZPPC;
				isflashrom = true;
				break;
		}
		break;

		case BOARD_KUPKE:
			romtype = ROMTYPE_CB_GOLEM030;
		break;

		case BOARD_DCE:
			switch (p->cpuboard_subtype)
			{
			case BOARD_DCE_SUB_SX32PRO:
				romtype = ROMTYPE_CB_SX32PRO;
				break;
			case BOARD_DCE_SUB_TYPHOON2:
				romtype = ROMTYPE_CB_TYPHOON2;
				break;
			}
			break;

		case BOARD_HARDITAL:
		switch (p->cpuboard_subtype)
		{
		case BOARD_HARDITAL_SUB_TQM:
			romtype = ROMTYPE_CB_TQM;
			break;
		}
		break;

		default:
			return false;
	}

	struct romlist *rl = NULL;
	if (romtype) {
		rl = getromlistbyromtype(romtype, romname);
		if (!rl) {
			rd = getromdatabytype(romtype);
			if (!rd)
				return false;
		} else {
			rd = rl->rd;
		}
		defaultromname = rd->defaultfilename;
	}
	if (!isflashrom) {
		if (romname)
			autoconfig_rom = zfile_fopen(romname, _T("rb"));
		if (!autoconfig_rom && defaultromname)
			autoconfig_rom = zfile_fopen(defaultromname, _T("rb"));
		if (rl) {
			if (autoconfig_rom) {
				struct romdata *rd2 = getromdatabyid(romtype);
				// Do not use image if it is not long enough (odd or even only?)
				if (!rd2 || zfile_size(autoconfig_rom) < rd2->size) {
					zfile_fclose(autoconfig_rom);
					autoconfig_rom = NULL;
				}
			}
			if (!autoconfig_rom)
				autoconfig_rom = read_rom(rl->rd);
		}
	} else {
		autoconfig_rom = flashromfile_open(romname);
		if (!autoconfig_rom) {
			if (rl)
				autoconfig_rom = flashromfile_open(rl->path);
			if (!autoconfig_rom)
				autoconfig_rom = flashromfile_open(defaultromname);
		}
		if (!autoconfig_rom) {
			if (aci->doinit)
				romwarning(romtype);
			write_log(_T("Couldn't open CPUBoard '%s' rom '%s'\n"), boardname, defaultromname);
			return false;
		}
	}
	
	if (!autoconfig_nomandatory) {
		if (!autoconfig_rom && romtype) {
			if (aci->doinit)
				romwarning(romtype);
			write_log(_T("ROM id %08X not found for CPUBoard '%s' emulation\n"), romtype, boardname);
			return false;
		}
		if (!autoconfig_rom) {
			write_log(_T("Couldn't open CPUBoard '%s' rom '%s'\n"), boardname, defaultromname);
			return false;
		}

		write_log(_T("CPUBoard '%s' ROM '%s' %lld loaded\n"), boardname, zfile_getname(autoconfig_rom), zfile_size(autoconfig_rom));
	}
#ifdef WITH_CPUBOARD
	uae_u8 *blizzardea_tmp = blizzardea_bank.baseaddr;
	uae_u8 *blizzardf0_tmp = blizzardf0_bank.baseaddr;
	if (!aci->doinit) {
		blizzardea_bank.baseaddr = xcalloc(uae_u8, 262144);
		blizzardf0_bank.baseaddr = xcalloc(uae_u8, 524288);
	} else {
		protect_roms(false);
	}
#else
		protect_roms(false);
#endif

	cpuboard_non_byte_ea = true;
#ifdef WITH_CPUBOARD

	if (is_sx32pro(p)) {
		earom_size = 65536;
		for (int i = 0; i < 32768; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0] = b;
			blizzardea_bank.baseaddr[i * 2 + 1] = 0xff;
		}
	} else if (is_apollo630(p)) {
		static const uae_u8 apollo_autoconfig_630[16] = { 0xd1, 0x23, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };
		earom_size = 65536;
		for (int i = 0; i < 16384; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0x8000] = b;
			blizzardea_bank.baseaddr[i * 2 + 0x8001] = 0xff;
		}
		memcpy(autoconfig_data, apollo_autoconfig_630, sizeof(apollo_autoconfig_630));
		if (!(aci->prefs->cpuboard_settings & 1)) {
			autoconfig_data[6 + 3] |= 1; // memory enable
		}
	} else if (is_mtec_ematrix530(p) || is_dce_typhoon2(p)) {
		earom_size = 65536;
		for (int i = 0; i < 32768; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0] = b;
			blizzardea_bank.baseaddr[i * 2 + 1] = 0xff;
		}
	} else if (is_dkb_wildfire(p)) {
		f0rom_size = 65536;
		zfile_fread(blizzardf0_bank.baseaddr, 1, f0rom_size, autoconfig_rom);
		flashrom = flash_new(blizzardf0_bank.baseaddr + 0, 32768, 65536, 0x01, 0x20, flashrom_file, FLASHROM_EVERY_OTHER_BYTE | FLASHROM_PARALLEL_EEPROM);
		flashrom2 = flash_new(blizzardf0_bank.baseaddr + 1, 32768, 65536, 0x01, 0x20, flashrom_file, FLASHROM_EVERY_OTHER_BYTE | FLASHROM_EVERY_OTHER_BYTE_ODD | FLASHROM_PARALLEL_EEPROM);
		autoconf = false;
		aci->start = 0xf00000;
		aci->size = 65536;
	} else if (is_aca500(p)) {
		f0rom_size = 524288;
		zfile_fread(blizzardf0_bank.baseaddr, f0rom_size, 1, autoconfig_rom);
		autoconf = false;
		if (zfile_needwrite(autoconfig_rom)) {
			flashrom_file = autoconfig_rom;
			autoconfig_rom = NULL;
		}
		flashrom = flash_new(blizzardf0_bank.baseaddr, f0rom_size, f0rom_size, 0x01, 0xa4, flashrom_file, 0);
	} else if (is_a2630(p)) {
		f0rom_size = 65536;
		zfile_fread(blizzardf0_bank.baseaddr, 1, f0rom_size, autoconfig_rom);
		autoconf = false;
		autoconf_stop = true;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
	} else if (is_harms_3kp(p)) {
		f0rom_size = 65536;
		zfile_fread(blizzardf0_bank.baseaddr, 1, f0rom_size, autoconfig_rom);
		autoconf = false;
		autoconf_stop = true;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
	} else if (is_kupke(p)) {
		earom_size = 65536;
		for (int i = 0; i < 8192; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0] = b;
			blizzardea_bank.baseaddr[i * 2 + 1] = 0xff;
		}
	} else if (is_apollo12xx(p)) {
		f0rom_size = 131072;
		zfile_fread(blizzardf0_bank.baseaddr, 1, 131072, autoconfig_rom);
		autoconf = false;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
	} else if (is_a1230s1(p)) {
		f0rom_size = 65536;
		zfile_fread(blizzardf0_bank.baseaddr, 1, 32768, autoconfig_rom);
		autoconf = false;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
	} else if (is_fusionforty(p)) {
		f0rom_size = 262144;
		zfile_fread(blizzardf0_bank.baseaddr, 1, 131072, autoconfig_rom);
		autoconf = false;
		aci->start = 0xf40000;
		aci->size = f0rom_size;
	} else if (is_tekmagic(p)) {
		earom_size = 65536;
		f0rom_size = 131072;
		zfile_fread(blizzardf0_bank.baseaddr, 1, f0rom_size, autoconfig_rom);
		autoconf = false;
		cpuboard_non_byte_ea = false;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
	} else if (is_trexii(p)) {
		earom_size = 65536;
		f0rom_size = 131072;
		zfile_fread(blizzardf0_bank.baseaddr, 1, f0rom_size, autoconfig_rom);
		autoconf = false;
		cpuboard_non_byte_ea = false;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
	} else if (is_quikpak(p)) {
		earom_size = 65536;
		f0rom_size = 131072;
		zfile_fread(blizzardf0_bank.baseaddr, 1, f0rom_size, autoconfig_rom);
		autoconf = false;
		cpuboard_non_byte_ea = false;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
	} else if (is_blizzard2060(p)) {
		f0rom_size = 65536;
		earom_size = 131072;
		// 2060 = 2x32k
		for (int i = 0; i < 32768; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardf0_bank.baseaddr[i * 2 + 1] = b;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardf0_bank.baseaddr[i * 2 + 0] = b;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 1] = b;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0] = b;
		}
	} else if (is_csmk1(p)) {
		earom_size = 65536;
		f0rom_size = 65536;
		aci->start = 0xf00000;
		aci->size = f0rom_size;
		if (autoconfig_rom) {
			zfile_fread(blizzardf0_bank.baseaddr, 1, f0rom_size, autoconfig_rom);
			zfile_fclose(autoconfig_rom);
			autoconfig_rom = NULL;
		}
		if (romtype2) {
			int idx2;
			struct boardromconfig *brc2 = get_device_rom(p, romtype2, 0, &idx2);
			if (brc2 && brc2->roms[idx2].romfile[0])
				autoconfig_rom = board_rom_open(romtype2, brc2->roms[idx2].romfile);
			if (autoconfig_rom) {
				memset(blizzardea_bank.baseaddr, 0xff, 65536);
				for (int i = 0; i < 32768; i++) {
					uae_u8 b = 0xff;
					zfile_fread(&b, 1, 1, autoconfig_rom);
					blizzardea_bank.baseaddr[i * 2 + 0] = b;
				}
			}
		} else {
			autoconf = false;
		}
	} else if (is_csmk2(p)) {
		earom_size = 131072;
		f0rom_size = 65536;
		zfile_fread(blizzardea_bank.baseaddr, 1,earom_size, autoconfig_rom);
		if (zfile_needwrite(autoconfig_rom)) {
			flashrom_file = autoconfig_rom;
			autoconfig_rom = NULL;
		}
		flashrom = flash_new(blizzardea_bank.baseaddr, earom_size, earom_size, 0x01, 0x20, flashrom_file, 0);
		memcpy(blizzardf0_bank.baseaddr, blizzardea_bank.baseaddr + 65536, 65536);
	} else if (is_csmk3(p) || is_blizzardppc(p)) {
		uae_u8 flashtype;
		earom_size = 0;
		if (is_blizzardppc(p)) {
			flashtype = 0xa4;
			f0rom_size = 524288;
			aci->last_high_ram = BLIZZARDMK4_RAM_BASE_48 + p->cpuboardmem1.size / 2;
		} else {
			flashtype = 0x20;
			f0rom_size = 131072;
			if (zfile_size(autoconfig_rom) >= 262144) {
				flashtype = 0xa4;
				f0rom_size = 524288;
			}
		}
		// empty rom space must be cleared, not set to ones.
		memset(blizzardf0_bank.baseaddr, 0x00, f0rom_size);
		if (f0rom_size == 524288) // but empty config data must be 0xFF
			memset(blizzardf0_bank.baseaddr + 0x50000, 0xff, 0x10000);
		zfile_fread(blizzardf0_bank.baseaddr, f0rom_size, 1, autoconfig_rom);
		autoconf = false;
		if (zfile_needwrite(autoconfig_rom)) {
			flashrom_file = autoconfig_rom;
			autoconfig_rom = NULL;
		}
		fixserial(p, blizzardf0_bank.baseaddr, f0rom_size);
		flashrom = flash_new(blizzardf0_bank.baseaddr, f0rom_size, f0rom_size, 0x01, flashtype, flashrom_file, 0);
		aci->start = 0xf00000;
		aci->size = 0x80000;
	} else if (is_blizzard(p)) {
		// 1230 MK IV / 1240/60
		f0rom_size = 65536;
		earom_size = 131072;
		// 12xx = 1x32k but read full 64k.
		for (int i = 0; i < 65536 / 2; i++) {
			uae_u8 b = 0xff;
			if (!zfile_fread(&b, 1, 1, autoconfig_rom))
				break;
			blizzardf0_bank.baseaddr[i] = b;
			if (!zfile_fread(&b, 1, 1, autoconfig_rom))
				break;
			blizzardea_bank.baseaddr[i] = b;
		}
		zfile_fclose(autoconfig_rom);
		autoconfig_rom = NULL;
		if (romtype2) {
			int idx2;
			struct boardromconfig *brc2 = get_device_rom(p, romtype2, 0, &idx2);
			if (brc2 && brc2->roms[idx2].romfile[0])
				autoconfig_rom = board_rom_open(romtype2, brc2->roms[idx2].romfile);
			if (autoconfig_rom) {
				memset(blizzardea_bank.baseaddr + 0x10000, 0xff, 65536);
				zfile_fread(blizzardea_bank.baseaddr + 0x10000, 32768, 1, autoconfig_rom);
			}
		}
		aci->last_high_ram = BLIZZARDMK4_RAM_BASE_48 + p->cpuboardmem1.size / 2;
	} else if (is_blizzard1230mk2(p)) {
		earom_size = 131072;
		for (int i = 0; i < 32768; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0] = b;
			blizzardea_bank.baseaddr[i * 2 + 1] = 0xff;
		}
		zfile_fclose(autoconfig_rom);
		autoconfig_rom = NULL;
	} else if (is_blizzard1230mk3(p)) {
		earom_size = 131072;
		zfile_fread(blizzardea_bank.baseaddr, 65536, 1, autoconfig_rom);
		zfile_fclose(autoconfig_rom);
		autoconfig_rom = NULL;
		if (romtype2) {
			int idx2;
			struct boardromconfig *brc2 = get_device_rom(p, romtype2, 0, &idx2);
			if (brc2 && brc2->roms[idx2].romfile[0])
				autoconfig_rom = board_rom_open(romtype2, brc2->roms[idx2].romfile);
			if (autoconfig_rom) {
				zfile_fread(blizzardea_bank.baseaddr, 65536, 1, autoconfig_rom);
				zfile_fclose(autoconfig_rom);
				autoconfig_rom = NULL;
			}
		}
	} else if (is_falcon40(p)) {
		earom_size = 262144;
		for (int i = 0; i < 131072; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0] = b;
			blizzardea_bank.baseaddr[i * 2 + 1] = 0xff;
		}
		zfile_fclose(autoconfig_rom);
		autoconfig_rom = NULL;
	} else if (is_tqm(p)) {
		earom_size = 65536;
		for (int i = 0; i < 32768; i++) {
			uae_u8 b = 0xff;
			zfile_fread(&b, 1, 1, autoconfig_rom);
			blizzardea_bank.baseaddr[i * 2 + 0] = b;
			blizzardea_bank.baseaddr[i * 2 + 1] = 0xff;
		}
		zfile_fclose(autoconfig_rom);
		autoconfig_rom = NULL;
	}

#endif //WITH_CPUBOARD
	if (autoconf_stop) {
		aci->addrbank = &expamem_none;
	} else if (!autoconf) {
		aci->zorro = 0;
	} else if (autoconfig_data[0]) {
#ifdef WITH_CPUBOARD
		aci->addrbank = &blizzarde8_bank;
		memcpy(aci->autoconfig_bytes, autoconfig_data, sizeof aci->autoconfig_bytes);
		aci->autoconfigp = aci->autoconfig_bytes;
#endif
		aci->zorro = 2;
		aci->autoconfig_automatic = true;
	} else {
#ifdef WITH_CPUBOARD
		aci->addrbank = &blizzarde8_bank;
		memcpy(aci->autoconfig_raw, blizzardea_bank.baseaddr, sizeof aci->autoconfig_raw);
#endif
		aci->zorro = 2;
	}

	zfile_fclose(autoconfig_rom);

	if (!aci->doinit) {
#ifdef WITH_CPUBOARD
		xfree(blizzardea_bank.baseaddr);
		xfree(blizzardf0_bank.baseaddr);
		blizzardea_bank.baseaddr = blizzardea_tmp;
		blizzardf0_bank.baseaddr = blizzardf0_tmp;
#endif
		flash_free(flashrom);
		flashrom = NULL;
		return true;
	} else {
		protect_roms(true);
	}
#ifdef WITH_CPUBOARD

	if (f0rom_size) {
		if (is_a2630(p)) {
			if (!(a2630_io & 2))
				map_banks(&blizzardf0_bank, 0xf80000 >> 16, f0rom_size >> 16, 0);
			if (!(a2630_io & 1))
				map_banks(&blizzardf0_bank, 0x000000 >> 16, f0rom_size >> 16, 0);
		} else if (is_harms_3kp(p)) {
			if (!(a2630_io & 2)) {
				map_banks(&blizzardf0_bank, 0xf80000 >> 16, f0rom_size >> 16, 0);
				map_banks(&blizzardf0_bank, 0xf70000 >> 16, f0rom_size >> 16, 0);
			}
			if (!(a2630_io & 1)) {
				map_banks(&blizzardf0_bank, 0x000000 >> 16, f0rom_size >> 16, 0);
			}
		} else if (is_fusionforty(p)) {
			map_banks(&blizzardf0_bank, 0x00f40000 >> 16, f0rom_size >> 16, 0);
			map_banks(&blizzardf0_bank, 0x05000000 >> 16, f0rom_size >> 16, 0);
			map_banks(&blizzardf0_bank, 0x00000000 >> 16, f0rom_size >> 16, 0);
		} else {
			map_banks(&blizzardf0_bank, 0xf00000 >> 16, (f0rom_size > 262144 ? 262144 : f0rom_size) >> 16, 0);
		}
	}
#endif //WITH_CPUBOARD

	device_add_vsync_pre(cpuboard_vsync);
	device_add_hsync(cpuboard_hsync);

	return true;
}

void cpuboard_set_cpu(struct uae_prefs *p)
{
	if (!p->cpuboard_type)
		return;
	
	const struct cpuboardsubtype *cbt = &cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype];

	// Lower to higher model
	if (p->cpu_model < cbt->cputype * 10 + 68000) {
		p->cpu_model = cbt->cputype * 10 + 68000;
		if ((p->fpu_model == 68881 || p->fpu_model == 68882) && cbt->cputype >= 4) {
			p->fpu_model = cbt->cputype * 10 + 68000;
		}
		if (p->mmu_model && p->cpu_model >= 68030) {
			p->mmu_model = p->cpu_model;
		} else {
			p->mmu_model = 0;
		}
	}
	// 68040/060 to 68020/030
	if (p->cpu_model >= 68040 && cbt->cputype < 4) {
		p->cpu_model = cbt->cputype * 10 + 68000;
		if (p->fpu_model == 68040 || p->fpu_model == 68060) {
			p->fpu_model = 68882;
		}
		if (p->mmu_model && p->cpu_model == 68030) {
			p->mmu_model = 68030;
		} else {
			p->mmu_model = 0;
		}
	}
	p->address_space_24 = false;
}
