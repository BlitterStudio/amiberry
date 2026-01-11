/*
* UAE - The Un*x Amiga Emulator
*
* Custom chip emulation
*
* (c) 1995 Bernd Schmidt, Alessandro Bissacco
* (c) 2002 - 2025 Toni Wilen
*/

#define SPEEDUP 1
#define BLITTER_DEBUG 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "blitter.h"
#include "blit.h"
#include "savestate.h"
#include "debug.h"

#define BLIT_TRACE 0
#if BLIT_TRACE
static uae_u32 *bdp;
static uae_u32 blit_tracer[2 * 10000000];
#endif

// 1 = logging
// 2 = no wait detection
// 4 = no D
// 8 = instant
// 16 = activate debugger if weird things
// 32 = logging (no line)

#if BLITTER_DEBUG
int log_blitter = 1 | 16;
#else
int log_blitter = 0;
#endif

extern uae_u8 agnus_hpos;

#define BLTCHA 0x800
#define BLTCHB 0x400
#define BLTCHC 0x200
#define BLTCHD 0x100

#define BLTDOFF 0x80
#define BLTEFE 0x10
#define BLTIFE 0x08
#define BLTFILL (BLTEFE | BLTIFE)
#define BLTFC 0x04
#define BLTDESC 0x02
#define BLTLINE 0x01

#define BLTSIGN 0x40
#define BLTOVF 0x20
#define BLTSUD 0x10
#define BLTSUL 0x08
#define BLTAUL 0x04
#define BLTSING 0x02

#define BLITTER_PIPELINE_BLIT 0x0008
#define BLITTER_PIPELINE_ADDMOD 0x0010
#define BLITTER_PIPELINE_LINE 0x0020
#define BLITTER_PIPELINE_PROCESS 0x0200
#define BLITTER_PIPELINE_FINISHED 0x0400
#define BLITTER_PIPELINE_LASTD 0x1000
#define BLITTER_PIPELINE_FIRST 0x2000
#define BLITTER_PIPELINE_LAST 0x4000

#define BLIT_NASTY_CPU_STEAL_CYCLE_COUNT 3

/* we must not change ce-mode while blitter is running.. */
static int blitter_cycle_exact, immediate_blits;
static int blt_statefile_type;

static uae_u16 bltcon0_next, bltcon1_next;
bool blitter_delayed_update;

static uae_u16 blineb;
static int blitline, blitfc, blitfill, blitife, blit_ovf, blitfill_old;
static int blitdesc_next, blitdesc;
static int blitline_started, blitlineloop;
static int blitonedot, blitlinepixel, blitlinepixel2;
static int blit_add;
static int blit_modadda, blit_modaddb, blit_modaddc, blit_modaddd;
static int blit_ch;
static bool shifter_skip_b, shifter_skip_y;
static bool shifter_skip_b_old, shifter_skip_y_old;
static uae_u16 bltcon0_old, bltcon1_old;
static bool shifter[4], shifter_out;
static bool shifter_d_armed;
static uae_u32 shifter_d1, shifter_d2, shifter_d_aga;
static bool blitline_c, blitfill_c;

static int blitter_delayed_debug;
#ifdef BLITTER_SLOWDOWNDEBUG
static int blitter_slowdowndebug;
#endif

struct bltinfo blt_info;

static uae_u8 blit_filltable[256][4][2];
uae_u32 blit_masktable[BLITTER_MAX_WORDS];

static int blit_cyclecounter;
static int blit_maxcyclecounter, blit_slowdown, blit_totalcyclecounter;
static int blit_misscyclecounter;

#ifdef CPUEMU_13
static int blitter_cyclecounter;
static int blitter_hcounter;
static int blitter_vcounter;
#endif

static evt_t blit_firstline_cycles;
static evt_t blit_first_cycle;
static int blit_last_cycle, blit_dmacount, blit_cyclecount;
static int blt_delayed_irq;
static int blit_dof;

static uae_u16 debug_bltcon0, debug_bltcon1;
static uae_u32 debug_bltapt, debug_bltbpt, debug_bltcpt, debug_bltdpt;
static uae_u16 debug_bltamod, debug_bltbmod, debug_bltcmod, debug_bltdmod;
static uae_u32 debug_bltafwm, debug_bltalwm;
static uae_u32 debug_bltpc;
static int debug_bltcop;
static uae_u16 debug_bltsizev, debug_bltsizeh;
static uae_u16 debug_bltadat, debug_bltbdat, debug_bltcdat;

#define BLITTER_MAX_PIPELINED_CYCLES 4

#define CYCLECOUNT_FINISHED -1000
#define CYCLECOUNT_START 2

/*
Blitter Idle Cycle:

Cycles that are free cycles (available for CPU) and
are not used by any other Agnus DMA channel. Blitter
idle cycle is not "used" by blitter, CPU can still use
it normally if it needs the bus.

same in both block and line modes

number of cycles, initial cycle, main cycle
*/

#if 0
#define DIAGSIZE 10
static const int blit_cycle_diagram[][DIAGSIZE] =
{
	{ 2, 0,0,	    0,0 },		/* 0   -- */
	{ 2, 0,0,	    0,4 },		/* 1   -D */
	{ 2, 0,3,	    0,3 },		/* 2   -C */
	{ 3, 0,3,0,	    0,3,4 },    /* 3  -CD */
	{ 3, 0,2,0,	    0,2,0 },    /* 4  -B- */
	{ 3, 0,2,0,	    0,2,4 },    /* 5  -BD */
	{ 3, 0,2,3,	    0,2,3 },    /* 6  -BC */
	{ 4, 0,2,3,0,   0,2,3,4 },  /* 7 -BCD */
	{ 2, 1,0,	    1,0 },		/* 8   A- */
	{ 2, 1,0,	    1,4 },		/* 9   AD */
	{ 2, 1,3,	    1,3 },		/* A   AC */
	{ 3, 1,3,0,	    1,3,4, },	/* B  ACD */
	{ 3, 1,2,0,	    1,2,0 },	/* C  AB- */
	{ 3, 1,2,0,	    1,2,4 },	/* D  ABD */
	{ 3, 1,2,3,	    1,2,3 },	/* E  ABC */
	{ 4, 1,2,3,0,   1,2,3,4 }	/* F ABCD */
};
#endif

/*

following 4 channel combinations in fill mode have extra
idle cycle added (still requires free bus cycle)

Condition: If D without C: Add extra cycle.

*/

#if 0

// Cycle sequences are now generated using same
// logic as real blitter. Tables are not used anymore.

static const int blit_cycle_diagram_fill[][DIAGSIZE] =
{
	{ 0 },						/* 0   -- */
	{ 3, 0,0,0,	    0,4,0 },	/* 1   -D */
	{ 0 },						/* 2   -C */
	{ 0 },						/* 3  -CD */
	{ 0 },						/* 4  -B- */
	{ 4, 0,2,0,0,   0,2,4,0 },	/* 5  -BD */
	{ 0 },						/* 6  -BC */
	{ 0 },						/* 7 -BCD */
	{ 0 },						/* 8   A- */
	{ 3, 1,0,0,	    1,4,0 },	/* 9   AD */
	{ 0 },						/* A   AC */
	{ 0 },						/* B  ACD */
	{ 0 },						/* C  AB- */
	{ 4, 1,2,0,0,   1,2,4,0 },	/* D  ABD */
	{ 0 },						/* E  ABC */
	{ 0 },						/* F ABCD */
};
#endif

/*
-C-D C-D- ... C-D- --

line draw takes 4 cycles (-C-D)
idle cycles do the same as above, 2 dma fetches
(read from C, write to D, but see below)

Oddities:

- first word is written to address pointed by BLTDPT
but all following writes go to address pointed by BLTCPT!
(some kind of internal copy because all bus cyles are
using normal BLTDDAT)
- BLTDMOD is ignored by blitter (BLTCMOD is used)
- state of D-channel enable bit does not matter!
- disabling A-channel freezes the content of BPLAPT
- C-channel disabled: nothing is written

There is one tricky situation, writing to DFF058 just before
last D write cycle (which is normally free) does not disturb
blitter operation, final D is still written correctly before
blitter starts normally (after 2 idle cycles)

There is at least one demo that does this..

*/

/* Copper pointer to Blitter register copy bug

	1: -d = D (-D)
	2: -c = C (-C)
	3: - (-CD)
	4: - (-B-)
	5: - (-BD)
	6: - (-BC)
	7: -BcD = C, -BCd = D
	8: - (A-)
	9: - (AD)
	A: - (AC)
	B: A (ACD)
	C: - (AB-)
	D: - (ABD-)
	E: - (ABC)
	F: AxBxCxD = -, aBxCxD = A, 

	1FE,8C,RGA,8C

 */

void build_blitfilltable(void)
{
	uae_u32 fillmask;

	for (int i = 0; i < BLITTER_MAX_WORDS; i++) {
		blit_masktable[i] = 0xFFFF;
	}

	for (int d = 0; d < 256; d++) {
		for (int i = 0; i < 4; i++) {
			int fc = i & 1;
			uae_u8 data = d;
			for (fillmask = 1; fillmask != 0x100; fillmask <<= 1) {
				uae_u16 tmp = data;
				if (fc) {
					if (i & 2)
						data |= fillmask;
					else
						data ^= fillmask;
				}
				if (tmp & fillmask) fc = !fc;
			}
			blit_filltable[d][i][0] = data;
			blit_filltable[d][i][1] = fc;
		}
	}
}

static void record_dma_blit_val(uae_u32 v)
{
#ifdef DEBUGGER
	if (debug_dma && blitter_cycle_exact) {
		record_dma_read_value(v);
	}
	if (memwatch_enabled) {
		debug_getpeekdma_value(v);
	}
#endif
}

static void record_dma_blit(uae_u16 reg, uae_u16 v, uae_u32 addr)
{
#ifdef DEBUGGER
	if (debug_dma && blitter_cycle_exact) {
		if (reg == 0) {
			record_dma_write(reg, v, addr, DMARECORD_BLITTER, 3 + (blitline_c ? 0x20 : (blitfill_c ? 0x10 : 0)));
		} else {
			int r = 0;
			if (reg == 0x70)
				r = 2;
			if (reg == 0x72)
				r = 1;
			if (reg == 0x74)
				r = 0;
			record_dma_read(reg, addr, DMARECORD_BLITTER, r + (blitline_c ? 0x20 : (blitfill_c ? 0x10 : 0)));
		}
	}
	if (memwatch_enabled) {
		if (reg == 0) {
			uae_u32 mask = MW_MASK_BLITTER_D_N;
			if (blitfill_c)
				mask = MW_MASK_BLITTER_D_F;
			if (blitline_c)
				mask = MW_MASK_BLITTER_D_L;
			debug_putpeekdma_chipram(addr, v, mask, reg);
		} else if (reg == 0x70) {
			debug_getpeekdma_chipram(addr, MW_MASK_BLITTER_C, reg);
		} else if (reg == 0x72) {
			debug_getpeekdma_chipram(addr, MW_MASK_BLITTER_B, reg);
		} else if (reg == 0x74) {
			debug_getpeekdma_chipram(addr, MW_MASK_BLITTER_A, reg);
		}
	}
#endif
}

static void blitter_debugsave(int copper, uaecptr pc)
{
	debug_bltcon0 = blt_info.bltcon0;
	debug_bltcon1 = blt_info.bltcon1;
	debug_bltsizev = blt_info.vblitsize;
	debug_bltsizeh = blt_info.hblitsize;
	debug_bltapt = blt_info.bltapt;
	debug_bltbpt = blt_info.bltbpt;
	debug_bltcpt = blt_info.bltcpt;
	debug_bltdpt = blt_info.bltdpt;
	debug_bltadat = blt_info.bltadat;
	debug_bltbdat = blt_info.bltbdat;
	debug_bltcdat = blt_info.bltcdat;
	debug_bltamod = blt_info.bltamod;
	debug_bltbmod = blt_info.bltbmod;
	debug_bltcmod = blt_info.bltcmod;
	debug_bltdmod = blt_info.bltdmod;
	debug_bltafwm = blt_info.bltafwm;
	debug_bltalwm = blt_info.bltalwm;
	debug_bltpc = pc;
	debug_bltcop = copper;
}

static void blitter_dump(void)
{
	int chipsize = currprefs.chipmem.size;
	console_out_f(_T("PT A=%08X B=%08X C=%08X D=%08X\n"), blt_info.bltapt, blt_info.bltbpt, blt_info.bltcpt, blt_info.bltdpt);
	console_out_f(_T("CON0=%04X CON1=%04X DAT A=%04X B=%04X C=%04X\n"),
		blt_info.bltcon0, blt_info.bltcon1, blt_info.bltadat, blt_info.bltbdat, blt_info.bltcdat);
	console_out_f(_T("AFWM=%04X ALWM=%04X MOD A=%04X B=%04X C=%04X D=%04X\n"),
		blt_info.bltafwm, blt_info.bltalwm,
		blt_info.bltamod & 0xffff, blt_info.bltbmod & 0xffff, blt_info.bltcmod & 0xffff, blt_info.bltdmod & 0xffff);
	console_out_f(_T("PC=%08X DMA=%d\n"), m68k_getpc(), dmaen (DMA_BLITTER));

	if (((blt_info.bltcon0 & BLTCHA) && blt_info.bltapt >= chipsize) || ((blt_info.bltcon0 & BLTCHB) && blt_info.bltbpt >= chipsize) ||
		((blt_info.bltcon0 & BLTCHC) && blt_info.bltcpt >= chipsize) || ((blt_info.bltcon0 & BLTCHD) && blt_info.bltdpt >= chipsize))
		console_out_f(_T("PT outside of chipram\n"));
}

void blitter_debugdump(void)
{
	console_out(_T("Blitter registers at start:\n"));
	console_out_f(_T("PT A=%08X B=%08X C=%08X D=%08X\n"), debug_bltapt, debug_bltbpt, debug_bltcpt, debug_bltdpt);
	console_out_f(_T("CON0=%04X CON1=%04X DAT A=%04X B=%04X C=%04X\n"),
		debug_bltcon0, debug_bltcon1, debug_bltadat, debug_bltbdat, debug_bltcdat);
	console_out_f(_T("AFWM=%04X ALWM=%04X MOD A=%04X B=%04X C=%04X D=%04X\n"),
		debug_bltafwm, debug_bltalwm, debug_bltamod, debug_bltbmod, debug_bltcmod, debug_bltdmod);
	console_out_f(_T("COP=%d PC=%08X\n"), debug_bltcop, debug_bltpc);
	console_out(_T("Blitter registers now:\n"));
	blitter_dump();
}

static bool blit_pending(void)
{
	if (shifter_d_armed || shifter[0] || shifter[1] || shifter[2] || shifter[3] ||
		shifter_d1 || shifter_d2 ||
		shifter_d_aga) {
		return true;
	}
	return false;
}

static void markidlecycle(void)
{
#ifdef DEBUGGER
	if (debug_dma && blit_pending()) {
		record_dma_event(DMA_EVENT_BLITSTARTFINISH);
	}
#endif
}

// blitter interrupt is set (and busy bit cleared) when
// last "main" cycle has been finished, any non-linedraw
// D-channel blit still needs 2 more cycles before final
// D is written (idle cycle, final D write)
//
// According to schematics, AGA has workaround delay circuit
// that adds 2 extra cycles if D is enabled and not line mode.
//
// line draw interrupt triggers when last D is written
// (or cycle where last D write would have been if
// ONEDOT was active)

static void blitter_interrupt(void)
{
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event(DMA_EVENT_BLITIRQ);
	}
#endif
	if (blt_info.blit_interrupt) {
		return;
	}
	blt_info.blit_interrupt = 1;
	INTREQ_INT(6, 3);
}

static void blitter_end(void)
{
	blt_info.blit_main = 0;
	blt_info.blit_queued = 0;
	event2_remevent(ev2_blitter);
	unset_special(SPCFLAG_BLTNASTY);
#if BLITTER_DEBUG
	if (log_blitter & 1) {

		if (blt_info.vblitsize > 100) {
			blitter_dump();
		}

		write_log(_T("cycles %d, missed %d, total %d\n"),
			blit_totalcyclecounter, blit_misscyclecounter, blit_totalcyclecounter + blit_misscyclecounter);

	}
#endif
}

static void blitter_done_all(bool all)
{
	blt_info.blit_main = 0;
	blt_info.blit_queued = blitter_cycle_exact ? BLITTER_MAX_PIPELINED_CYCLES : 0;
	blt_info.finishcycle_dmacon = get_cycles();
	blt_info.finishcycle_copper = get_cycles() + 1 * CYCLE_UNIT;
	blitter_interrupt();
	if (all) {
		blitter_done_notify();
	}
}

static void blit_chipmem_agnus_wput(uaecptr addr, uae_u32 w, uae_u32 typemask)
{
	if (!(log_blitter & 4)) {
		if (blit_dof) {
			w = regs.chipset_latch_rw;
		}
#ifdef DEBUGGER
		debug_putpeekdma_chipram(addr, w, typemask, 0x000);
#endif
		chipmem_wput_indirect(addr, w);
		regs.chipset_latch_rw = w;
	}
}

static void blitter_dofast(void)
{
	int i,j;
	uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
	uae_u8 mt = blt_info.bltcon0 & 0xFF;
	uae_u16 ashift = blt_info.bltcon0 >> 12;
	uae_u16 bshift = blt_info.bltcon1 >> 12;

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

	if (blt_info.bltcon0 & BLTCHA) {
		bltadatptr = blt_info.bltapt;
		blt_info.bltapt += (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
	}
	if (blt_info.bltcon0 & BLTCHB) {
		bltbdatptr = blt_info.bltbpt;
		blt_info.bltbpt += (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
	}
	if (blt_info.bltcon0 & BLTCHC) {
		bltcdatptr = blt_info.bltcpt;
		blt_info.bltcpt += (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
	}
	if (blt_info.bltcon0 & BLTCHD) {
		bltddatptr = blt_info.bltdpt;
		blt_info.bltdpt += (blt_info.hblitsize * 2 + blt_info.bltdmod) * blt_info.vblitsize;
	}

#if SPEEDUP
	if (blitfunc_dofast[mt] && !blitfill) {
		(*blitfunc_dofast[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else
#endif
	{
		uae_u32 blitbhold = blt_info.bltbhold;
		int ashift = blt_info.bltcon0 >> 12;
		int bshift = blt_info.bltcon1 >> 12;
		uaecptr dstp = 0;
		int dodst = 0;

		for (j = 0; j < blt_info.vblitsize; j++) {
			blitfc = !!(blt_info.bltcon1 & BLTFC);
			for (i = 0; i < blt_info.hblitsize; i++) {
				uae_u32 bltadat, blitahold;
				if (bltadatptr) {
					blt_info.bltadat = bltadat = chipmem_wget_indirect (bltadatptr);
					bltadatptr += 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)blt_info.bltaold << 16) | bltadat) >> ashift;
				blt_info.bltaold = bltadat;

				if (bltbdatptr) {
					uae_u16 bltbdat = chipmem_wget_indirect (bltbdatptr);
					bltbdatptr += 2;
					blitbhold = (((uae_u32)blt_info.bltbold << 16) | bltbdat) >> bshift;
					blt_info.bltbold = bltbdat;
					blt_info.bltbdat = bltbdat;
				}

				if (bltcdatptr) {
					blt_info.bltcdat = chipmem_wget_indirect (bltcdatptr);
					bltcdatptr += 2;
				}
				if (dodst)
					blit_chipmem_agnus_wput(dstp, blt_info.bltddat, MW_MASK_BLITTER_D_N);
				blt_info.bltddat = blit_func(blitahold, blitbhold, blt_info.bltcdat, mt) & 0xFFFF;
				if (blitfill) {
					uae_u16 d = blt_info.bltddat;
					int ifemode = blitife ? 2 : 0;
					int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
					blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
						+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
					blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
				}
				if (blt_info.bltddat)
					blt_info.blitzero = 0;
				if (bltddatptr) {
					dodst = 1;
					dstp = bltddatptr;
					bltddatptr += 2;
				}
			}
			if (bltadatptr)
				bltadatptr += blt_info.bltamod;
			if (bltbdatptr)
				bltbdatptr += blt_info.bltbmod;
			if (bltcdatptr)
				bltcdatptr += blt_info.bltcmod;
			if (bltddatptr)
				bltddatptr += blt_info.bltdmod;
		}
		if (dodst)
			blit_chipmem_agnus_wput(dstp, blt_info.bltddat, MW_MASK_BLITTER_D_N);
		blt_info.bltbhold = blitbhold;
	}
	blit_masktable[0] = 0xFFFF;
	blit_masktable[blt_info.hblitsize - 1] = 0xFFFF;
	blt_info.blit_main = 0;
}

static void blitter_dofast_desc(void)
{
	int i,j;
	uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
	uae_u8 mt = blt_info.bltcon0 & 0xFF;
	uae_u16 ashift = 16 - (blt_info.bltcon0 >> 12);
	uae_u16 bshift = 16 - (blt_info.bltcon1 >> 12);

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

	if (blt_info.bltcon0 & BLTCHA) {
		bltadatptr = blt_info.bltapt;
		blt_info.bltapt -= (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
	}
	if (blt_info.bltcon0 & BLTCHB) {
		bltbdatptr = blt_info.bltbpt;
		blt_info.bltbpt -= (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
	}
	if (blt_info.bltcon0 & BLTCHC) {
		bltcdatptr = blt_info.bltcpt;
		blt_info.bltcpt -= (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
	}
	if (blt_info.bltcon0 & BLTCHD) {
		bltddatptr = blt_info.bltdpt;
		blt_info.bltdpt -= (blt_info.hblitsize * 2 + blt_info.bltdmod) * blt_info.vblitsize;
	}
#if SPEEDUP
	if (blitfunc_dofast_desc[mt] && !blitfill) {
		(*blitfunc_dofast_desc[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else
#endif
	{
		uae_u32 blitbhold = blt_info.bltbhold;
		uaecptr dstp = 0;
		int dodst = 0;

		for (j = 0; j < blt_info.vblitsize; j++) {
			blitfc = !!(blt_info.bltcon1 & BLTFC);
			for (i = 0; i < blt_info.hblitsize; i++) {
				uae_u32 bltadat, blitahold;
				if (bltadatptr) {
					bltadat = blt_info.bltadat = chipmem_wget_indirect (bltadatptr);
					bltadatptr -= 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)bltadat << 16) | blt_info.bltaold) >> ashift;
				blt_info.bltaold = bltadat;

				if (bltbdatptr) {
					uae_u16 bltbdat = chipmem_wget_indirect (bltbdatptr);
					bltbdatptr -= 2;
					blitbhold = (((uae_u32)bltbdat << 16) | blt_info.bltbold) >> bshift;
					blt_info.bltbold = bltbdat;
					blt_info.bltbdat = bltbdat;
				}

				if (bltcdatptr) {
					blt_info.bltcdat = blt_info.bltbdat = chipmem_wget_indirect (bltcdatptr);
					bltcdatptr -= 2;
				}
				if (dodst)
					blit_chipmem_agnus_wput(dstp, blt_info.bltddat, MW_MASK_BLITTER_D_N);
				blt_info.bltddat = blit_func(blitahold, blitbhold, blt_info.bltcdat, mt) & 0xFFFF;
				if (blitfill) {
					uae_u16 d = blt_info.bltddat;
					int ifemode = blitife ? 2 : 0;
					int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
					blt_info.bltddat = (blit_filltable[d & 255][ifemode + blitfc][0]
					+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
					blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
				}
				if (blt_info.bltddat)
					blt_info.blitzero = 0;
				if (bltddatptr) {
					dstp = bltddatptr;
					dodst = 1;
					bltddatptr -= 2;
				}
			}
			if (bltadatptr)
				bltadatptr -= blt_info.bltamod;
			if (bltbdatptr)
				bltbdatptr -= blt_info.bltbmod;
			if (bltcdatptr)
				bltcdatptr -= blt_info.bltcmod;
			if (bltddatptr)
				bltddatptr -= blt_info.bltdmod;
		}
		if (dodst)
			blit_chipmem_agnus_wput(dstp, blt_info.bltddat, MW_MASK_BLITTER_D_N);
		blt_info.bltbhold = blitbhold;
	}
	blit_masktable[0] = 0xFFFF;
	blit_masktable[blt_info.hblitsize - 1] = 0xFFFF;
	blt_info.blit_main = 0;
}

static void blitter_line_read_b(void)
{
	if (blt_info.bltcon0 & BLTCHB) {
		// B (normally not enabled)
		record_dma_blit(0x72, 0, blt_info.bltbpt);
		blt_info.bltbdat = chipmem_wget_indirect(blt_info.bltbpt);
		regs.chipset_latch_rw = blt_info.bltbdat;
		record_dma_blit_val(blt_info.bltbdat);
		blt_info.bltbpt += blt_info.bltbmod;
	}
}
static void blitter_line_read_c(void)
{
	if (blt_info.bltcon0 & BLTCHC) {
		// C
		record_dma_blit(0x70, 0, blt_info.bltcpt);
		blt_info.bltcdat = chipmem_wget_indirect(blt_info.bltcpt);
		regs.chipset_latch_rw = blt_info.bltcdat;
		record_dma_blit_val(blt_info.bltcdat);
	}
}

static void blitter_line_write(void)
{
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (blt_info.bltcon0 & BLTCHC) {
		blit_chipmem_agnus_wput(blt_info.bltdpt, blt_info.bltddat, MW_MASK_BLITTER_D_L);
	}
}

static void blitter_line_minterm_extra(void)
{
	int ashift = blt_info.bltcon0 >> 12;
	// never first or last, no masking needed.
	uae_u16 blitahold = blt_info.bltadat >> ashift;

	blt_info.bltahold2 = blitahold;
	blt_info.bltbhold2 = blt_info.bltbhold;
	blt_info.bltchold2 = blt_info.bltcdat;
}

static void blitter_line_adat(void)
{
	int ashift = blt_info.bltcon0 >> 12;

	blt_info.bltaold = (((uae_u32)blt_info.bltaold << 16) | (blt_info.bltadat & blt_info.bltafwm)) >> ashift;
}

static void blitter_line_proc_apt(void)
{
	if (blt_info.bltcon0 & BLTCHA) {
		bool sign = (blt_info.bltcon1 & BLTSIGN) != 0;
		if (sign)
			blt_info.bltapt += (uae_s16)blt_info.bltbmod;
		else
			blt_info.bltapt += (uae_s16)blt_info.bltamod;
	}
}

static void blitter_line_ovf(void)
{
	uae_u16 ashift = blt_info.bltcon0 >> 12;
	ashift += blit_ovf;
	ashift &= 15;
	blt_info.bltcon0 &= 0x0fff;
	blt_info.bltcon0 |= ashift << 12;
	blit_ovf = 0;
}

static void blitter_line_incx(void)
{
	uae_u16 ashift = blt_info.bltcon0 >> 12;
	if (ashift == 15) {
		blt_info.bltcpt += 2;
	}
	blit_ovf = 1;
}

static void blitter_line_decx(void)
{
	uae_u16 ashift = blt_info.bltcon0 >> 12;
	if (ashift == 0) {
		blt_info.bltcpt -= 2;
	}
	blit_ovf = -1;
}

static void blitter_line_decy(void)
{
	if (blt_info.bltcon0 & BLTCHC) {
		blt_info.bltcpt -= blt_info.bltcmod;
		blitonedot = 0;
	}
}

static void blitter_line_incy(void)
{
	if (blt_info.bltcon0 & BLTCHC) {
		blt_info.bltcpt += blt_info.bltcmod;
		blitonedot = 0;
	}
}

static void blitter_line_proc_cpt_y(void)
{
	bool sign = (blt_info.bltcon1 & BLTSIGN) != 0;

	if (!sign) {
		if (blt_info.bltcon1 & BLTSUD) {
			if (blt_info.bltcon1 & BLTSUL)
				blitter_line_decy();
			else
				blitter_line_incy();
		}
	}
	if (!(blt_info.bltcon1 & BLTSUD)) {
		if (blt_info.bltcon1 & BLTAUL)
			blitter_line_decy();
		else
			blitter_line_incy();
	}
}

static void blitter_line_proc_cpt_x(void)
{
	bool sign = (blt_info.bltcon1 & BLTSIGN) != 0;
	
	if (!sign) {
		if (!(blt_info.bltcon1 & BLTSUD)) {
			if (blt_info.bltcon1 & BLTSUL)
				blitter_line_decx();
			else
				blitter_line_incx();
		}
	}
	if (blt_info.bltcon1 & BLTSUD) {
		if (blt_info.bltcon1 & BLTAUL)
			blitter_line_decx();
		else
			blitter_line_incx();
	}
}

static void blitter_line_proc_status(void)
{
	blitlinepixel = !(blt_info.bltcon1 & BLTSING) || ((blt_info.bltcon1 & BLTSING) && !blitonedot);
	blitonedot = 1;
}

static void blitter_line_sign(void)
{
	bool blitsign = (uae_s16)blt_info.bltapt < 0;
	if (blitsign) {
		blt_info.bltcon1 |= BLTSIGN;
	} else {
		blt_info.bltcon1 &= ~BLTSIGN;
	}
}

static void blitter_nxline(void)
{
	int bshift = blt_info.bltcon1 >> 12;

	bshift--;
	bshift &= 15;

	blineb = (blt_info.bltbdat >> bshift) | (blt_info.bltbdat << (16 - bshift));

	blt_info.bltcon1 &= 0x0fff;
	blt_info.bltcon1 |= bshift << 12;

}

static void blitter_line_minterm(uae_u16 dat)
{
	uae_u16 mask = blt_info.bltafwm;	
	if (dat & BLITTER_PIPELINE_LAST) {
		mask &= blt_info.bltalwm;
	}

	int ashift = blt_info.bltcon0 >> 12;
	uae_u16 blitahold = (blt_info.bltadat & mask) >> ashift;

	if (blt_info.bltcon0 & BLTCHB) {
		// B special case if enabled
		int bshift = blt_info.bltcon1 >> 12;
		blineb = (((uae_u32)blt_info.bltbold << 16) | blt_info.bltbdat) >> bshift;
	}
	blt_info.bltbhold = (blineb & 0x0001) ? 0xFFFF : 0;

	blt_info.bltahold2 = blitahold;
	blt_info.bltbhold2 = blt_info.bltbhold;
	blt_info.bltchold2 = blt_info.bltcdat;
}


static void blitter_loadadat(uae_u16 dat)
{
	uae_u32 blitahold;
	uae_u16 ashift = blt_info.bltcon0 >> 12;
	uae_u16 bltadat = blt_info.bltadat;

	if (dat & BLITTER_PIPELINE_FIRST) {
		bltadat &= blt_info.bltafwm;
	}
	if (dat & BLITTER_PIPELINE_LAST) {
		bltadat &= blt_info.bltalwm;
	}

	if (blitdesc) {
		blitahold = (((uae_u32)bltadat << 16) | blt_info.bltaold) >> (16 - ashift);
	} else {
		blitahold = (((uae_u32)blt_info.bltaold << 16) | bltadat) >> ashift;
	}

	blt_info.bltaold = bltadat;
	blt_info.bltahold2 = blitahold;
}

static void blitter_loadblit(uae_u16 dat)
{
	blitter_loadadat(dat);
	blt_info.bltbhold2 = blt_info.bltbhold;
}

static void blitter_doblit_last(void)
{
	uae_u8 mt = blt_info.bltcon0 & 0xFF;

	uae_u16 ddat = blit_func(blt_info.bltahold2, blt_info.bltbhold2, blt_info.bltchold2, mt) & 0xFFFF;

	if (blitfill) {
		uae_u16 d = ddat;
		int ifemode = blitife ? 2 : 0;
		int fc1 = blit_filltable[d & 255][ifemode + blitfc][1];
		ddat = (blit_filltable[d & 255][ifemode + blitfc][0]
			+ (blit_filltable[d >> 8][ifemode + fc1][0] << 8));
		blitfc = blit_filltable[d >> 8][ifemode + fc1][1];
	}

	if (ddat) {
		blt_info.blitzero = 0;
	}

	blt_info.bltddat = ddat;
}

// immediate blit
static void actually_do_blit(void)
{
	if (blitline) {
		do {
			blitter_line_proc_status();
			blitter_line_proc_apt();
			if (blt_info.hblitsize > 1) {
				blitter_line_read_b();
				blitter_line_read_c();
				blitter_line_minterm(BLITTER_PIPELINE_FIRST);
				blitter_doblit_last();
				blitter_line_proc_cpt_x();
			}
			if (blt_info.hblitsize > 2) {
				if (blitlineloop && !(blt_info.bltcon1 & BLTSUD)) {
					blitter_line_proc_cpt_y();
					blitlineloop = 0;
				}
				blitter_line_read_c();
				blitter_line_minterm_extra();
				blitter_doblit_last();
			}
			blitter_line_adat();
			blitter_line_ovf();
			if (blt_info.hblitsize >= 2) {
				if (blitlineloop) {
					blitter_line_proc_cpt_y();
					blitlineloop = 0;
				}
			}
			blitter_line_sign();
			blitter_nxline();
			if (blitlinepixel) {
				blitter_line_write();
				blitlinepixel = 0;
			}
			blt_info.bltdpt = blt_info.bltcpt;
			blitter_line_minterm(BLITTER_PIPELINE_FIRST | BLITTER_PIPELINE_LAST);
			blitter_doblit_last();
			blitlineloop = 1;
			blt_info.vblitsize--;
		} while (blt_info.vblitsize != 0);
	} else {
		if (blitdesc)
			blitter_dofast_desc();
		else
			blitter_dofast();
	}
	blt_info.blit_main = 0;
}

static void blitter_doit(void)
{
	if (blt_info.vblitsize == 0) {
		blitter_done_all(true);
		return;
	}
	actually_do_blit();
	blitter_done_all(true);
}

static int makebliteventtime(int delay)
{
	if (delay) {
		delay += delay - (int)(delay * (1 + currprefs.blitter_speed_throttle) + 0.5);
		if (delay <= 0)
			delay = 1;
	}
	return delay;
}

void blitter_handler(uae_u32 data)
{
	static int blitter_stuck;

	if (!dmaen (DMA_BLITTER)) {
		event2_newevent (ev2_blitter, 10, 0);
		blitter_stuck++;
		if (blitter_stuck < 20000 || !immediate_blits)
			return; /* gotta come back later. */
		/* "free" blitter in immediate mode if it has been "stuck" ~3 frames
		* fixes some JIT game incompatibilities
		*/
#ifdef DEBUGGER
		debugtest (DEBUGTEST_BLITTER, _T("force-unstuck!\n"));
#endif
	}
	blitter_stuck = 0;
	if (blit_slowdown > 0 && !immediate_blits) {
		event2_newevent (ev2_blitter, makebliteventtime(blit_slowdown), 0);
		blit_slowdown = -1;
		return;
	}
	blitter_doit();
}

#ifdef CPUEMU_13

static bool blitshifterdebug(uae_u16 con0, bool check)
{
	int cnt = 0;
	for (int i = 0; i < 4; i++) {
		if (shifter[i]) {
			cnt++;
		}
	}
	if (cnt != 1 || !check) {
		write_log(_T("Blitter shifter bits %d: CH=%c%c%c%c (%d-%d-%d-%d O=%d) PC=%08x\n"),
			cnt,
			((con0 >> 11) & 1) ? 'A' : '-', ((con0 >> 10) & 1) ? 'B' : '-', ((con0 >> 9) & 1) ? 'C' : '-', ((con0 >> 8) & 1) ? 'D' : '-',
			shifter[0], shifter[1], shifter[2], shifter[3], shifter_out, M68K_GETPC);
		return true;
	}
	return false;
}

static void blit_bltset(int con)
{
	static int blit_warned = 100;
	bool blit_changed = false;
	uae_u16 con0_old = bltcon0_old;

	if (bltcon0_next != blt_info.bltcon0) {
		blitter_delayed_update = true;
	}
	bltcon0_next = blt_info.bltcon0;
	if (bltcon1_next != blt_info.bltcon1) {
		blitter_delayed_update = true;
	}
	bltcon1_next = blt_info.bltcon1;
	blt_info.blt_change_cycles = get_cycles() + CYCLE_UNIT;

	if (con & 2) {
		if (!savestate_state && blt_info.blit_main && blit_warned > 0) {
			if ((blt_info.bltcon1 & BLTLINE) && !blitline_started) {
				write_log(_T("BLITTER: linedraw enabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			} else if (!(blt_info.bltcon1 & BLTLINE) && blitline_started) {
				write_log(_T("BLITTER: linedraw disabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			}
			if ((blt_info.bltcon1 & BLTFILL) && !(bltcon1_old & BLTFILL)) {
				write_log(_T("BLITTER: fill enabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			} else if (!(blt_info.bltcon1 & BLTFILL) && (bltcon1_old & BLTFILL)) {
				write_log(_T("BLITTER: fill disabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			}
		}
	}

	if (!savestate_state && blt_info.blit_main) {
		if ((bltcon0_old & 0x0f00) != (blt_info.bltcon0 & 0x0f00) ||
			(bltcon1_old & 1) != (blt_info.bltcon1 & 1) ||
			((bltcon1_old & 1) && (bltcon1_old & 0x7f) != (blt_info.bltcon1 & 0x7f)) ||
			(!(bltcon1_old & 1) && (bltcon1_old & (4 | 8 | 16)) != (blt_info.bltcon1 & (4 | 8 | 16)))) {
			blit_changed = true;
			if (blit_warned > 0) {
				write_log(_T("BLITTER: BLTCON0 %04x -> %04x BLTCON1 %04x -> %04x PC=%08x (%d %d)\n"),
					bltcon0_old, blt_info.bltcon0, bltcon1_old, blt_info.bltcon1, M68K_GETPC, current_hpos(), vpos);
				//blitter_dump();
				blitshifterdebug(bltcon0_old, false);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			}
		}
	}
	bltcon0_old = blt_info.bltcon0;
	bltcon1_old = blt_info.bltcon1;

	blit_ch = (blt_info.bltcon0 & (BLTCHD | BLTCHC | BLTCHB | BLTCHA)) >> 8;
	blitline = blt_info.bltcon1 & BLTLINE;
	blit_ovf = (blt_info.bltcon1 & BLTOVF) != 0;

	shifter_skip_b = (blt_info.bltcon0 & BLTCHB) == 0;
	if (blitline) {
		shifter_skip_y = true;
		blitfill = 0;
		shifter_out = shifter_skip_y ? shifter[2] : shifter[3];
		shifter_d_armed = false;
	} else {
		blitfill = (blt_info.bltcon1 & BLTFILL) != 0;
		blitfc = !!(blt_info.bltcon1 & BLTFC);
		blitife = !!(blt_info.bltcon1 & BLTIFE);
		if ((blt_info.bltcon1 & BLTFILL) == BLTFILL) {
#ifdef DEBUGGER
			debugtest(DEBUGTEST_BLITTER, _T("weird fill mode\n"));
#endif
			blitife = 0;
		}
		if (blitfill && !blitdesc) {
#ifdef DEBUGGER
			debugtest(DEBUGTEST_BLITTER, _T("fill without desc\n"));
			if (log_blitter & 16) {
				activate_debugger();
			}
#endif
		}

		shifter_skip_y = (blt_info.bltcon0 & (BLTCHD | BLTCHC)) != (BLTCHD | BLTCHC);
		// fill mode idle cycle needed? (D enabled but C not enabled)
		if (blitfill && (blt_info.bltcon0 & (BLTCHD | BLTCHC)) == BLTCHD) {
			shifter_skip_y = false;
		}

		shifter_out = shifter_skip_y ? shifter[2] : shifter[3];
	}

	blit_cyclecount = 4 - (shifter_skip_b + shifter_skip_y);
	blit_dmacount = ((blt_info.bltcon0 & BLTCHA) ? 1 : 0) + ((blt_info.bltcon0 & BLTCHB) ? 1 : 0) +
		((blt_info.bltcon0 & BLTCHC) ? 1 : 0) + (((blt_info.bltcon0 & BLTCHD) && !blitline) ? 1 : 0);

	if (blt_info.blit_main || blt_info.blit_pending) {
		blitfill_c = blitfill;
		blitline_c = blitline;
	}
	if (!(blt_info.bltcon0 & BLTCHD)) {
		shifter_d_armed = false;
	}

	if ((bltcon1_next & BLTDOFF) && ecs_agnus) {
#ifdef DEBUGGER
		debugtest(DEBUGTEST_BLITTER, _T("ECS BLTCON1 DOFF-bit set\n"));
		if (log_blitter & 16)
			activate_debugger();
#endif
	}

	if (blit_changed && blit_warned > 0 && !savestate_state) {
		blitshifterdebug(blt_info.bltcon0, false);
	}

}

static int get_current_channel(void)
{
	if (blitline) {
		int nreg = 0x1fe;
		bool lastw = blitter_hcounter + 1 == blt_info.hblitsize;
		if (shifter[0]) {
			// A or idle
			if (lastw) {
				return 5;
			}
			if (blt_info.bltcon0 & BLTCHA) {
				nreg &= 0x74; // A
			}
		}
		// B
		if (shifter[1] && (blt_info.bltcon0 & BLTCHB)) {
			nreg &= 0x72; // B
		}
		// C
		if (shifter[2] && (blt_info.bltcon0 & BLTCHC) && !lastw) {
			nreg &= 0x70; // C
		}

		if (nreg == 0x70) {
			return 3; // C
		}
		if (nreg == 0x72) {
			return 2; // B
		}
		if (nreg == 0x74) {
			return 1; // A
		}
		if (nreg != 0x1fe) {
			return 0;
		}

		// D (C in line mode)
		if (shifter[2] && lastw) {
			if (blt_info.bltcon0 & BLTCHC) {
				// enabled C->D
				if (!shifter[0]) {
					/* onedot mode and no pixel = bus write access is skipped */
					if (blitlinepixel2) {
						return 4;
					}
					return 6;
				}
			} else {
				// disabled C
				return 6;
			}
		}

	} else {

		int nreg = 0x1fe;
		if (shifter[0] && (blt_info.bltcon0 & BLTCHA)) {
			nreg &= 0x74; // A
		}
		if (shifter[1] && (blt_info.bltcon0 & BLTCHB)) {
			nreg &= 0x72; // B
		}
		if (shifter[2] && (blt_info.bltcon0 & BLTCHC)) {
			nreg &= 0x70; // C
		}
		if (nreg == 0x70) {
			return 3; // C
		}
		if (nreg == 0x72) {
			return 2; // B
		}
		if (nreg == 0x74) {
			return 1; // A
		}
		if (nreg != 0x1fe) {
			return 0;
		}
		if (shifter_d_armed && !shifter[0] && (blt_info.bltcon0 & BLTCHD)) {
			return 4; // D
		}

	}
	return 0;
}

// latches/shifters that run every CCK
static void blitter_next_cycle_always(void)
{
	if (shifter_d2) {
		if (!blitline && (blt_info.bltcon0 & BLTCHD)) {
			shifter_d_armed = true;
		}
	}
	shifter_d2 = (shifter_d1 & 3) == 1;
	shifter_d1 <<= 1;
	shifter_d1 &= 3;

	if (aga_mode) {
		// AGA 2 CCK delay busy fix
		shifter_d_aga <<= 1;
		shifter_d_aga &= 7;
		if ((shifter_d_aga & (1 << 2)) && blt_info.blit_count_done) {
			blitter_done_all(false);
		}
	}
}

static int blitter_next_cycle(bool injectbit)
{
	bool tmp[4];
	bool out = false;
	bool blitchanged = false;
	bool dodat = false;

	memcpy(tmp, shifter, sizeof(shifter));
	memset(shifter, 0, sizeof(shifter));

	if (shifter_skip_b_old && !shifter_skip_b) {
		// if B skip was disabled: A goes to B
		tmp[1] = tmp[0];
		//activate_debugger();
		shifter_skip_b_old = shifter_skip_b;
		blitchanged = true;
	} else if (!shifter_skip_b_old && shifter_skip_b) {
		// if B skip was enabled: A goes nowhere
		tmp[0] = false;
		shifter_skip_b_old = shifter_skip_b;
		blitchanged = true;
	}

	if (shifter_skip_y_old && !shifter_skip_y) {
		// if Y skip was disabled: X goes both to Y and OUT
		tmp[3] = tmp[2];
		shifter_out = tmp[3];
		shifter_skip_y_old = shifter_skip_y;
		blitchanged = true;
	} else if (!shifter_skip_y_old && shifter_skip_y) {
		// if Y skip was enabled: X goes nowhere
		tmp[2] = false;
		shifter_out = false;
		shifter_skip_y_old = shifter_skip_y;
		blitchanged = true;
	}

	shifter_d1 &= ~1;
	shifter_d1 |= shifter_out ? 1 : 0;
	if (shifter_out) {
		dodat = true;
		if (blt_info.blit_count_done) {
			// if there is still bit in shifter and counter has already finished: next bit is routed back to shifter A.
			if (blt_info.blit_count_done > 1) {
				shifter_out = false;
				shifter[0] = false;
			}
			blt_info.blit_count_done++;
		} else {
			if (blit_cyclecounter >= 0) {
				blitter_hcounter++;
				if (blitter_hcounter >= blt_info.hblitsize) {
					blitter_hcounter = 0;
					blitter_vcounter++;
					if (blitter_vcounter >= blt_info.vblitsize) {
						blit_cyclecounter = CYCLECOUNT_FINISHED;
						blt_info.blit_count_done = 1;
						shifter_out = false;
					}
				}
				shifter[0] = shifter_out;
			}
		}
	}

	// counters finished
	//if (blt_info.blit_count_done & 1) {
	//	shifter[0] = false;
	//}

	if (injectbit) {
		shifter[0] = true;
		blitfc = !!(blt_info.bltcon1 & BLTFC);
	}

	if (shifter_skip_b) {
		shifter[2] = tmp[0];
	} else {
		shifter[1] = tmp[0];
		shifter[2] = tmp[1];
	}
	if (shifter_skip_y) {
		out = shifter[2];
	} else {
		shifter[3] = tmp[2];
		out = shifter[3];
	}

	shifter_out = out;

	if (blit_cyclecounter > 0 && blitchanged && blt_info.blit_main) {
		static int blit_warned = 100;
		if (blit_warned > 0) {
			blitshifterdebug(blt_info.bltcon0, true);
			blit_warned--;
		}
	}
	return dodat;
}


static void blitter_doddma_new(struct rgabuf *rga, bool addmod)
{
	uaecptr pt = rga->pv;
	record_dma_blit(rga->reg, blt_info.bltddat, pt);
	blit_chipmem_agnus_wput(pt, blt_info.bltddat, MW_MASK_BLITTER_D_N);
	regs.chipset_latch_rw = blt_info.bltddat;

#if BLIT_TRACE == 2
	uae_u32 v1 = (4 << 16) | blt_info.bltddat;
	uae_u32 v2 = pt;
	if (bdp[0] != v1 || bdp[1] != v2) {
		write_log("MISMATCH!\n");
		activate_debugger();
	}
	bdp += 2;
#endif
#if BLIT_TRACE == 1
	*bdp++ = (4 << 16) | blt_info.bltddat;
	*bdp++ = pt;
#endif
}

void blitter_loadcdat(uae_u16 v)
{
	blt_info.bltchold2 = blt_info.bltcdat;
}

void blitter_loadbdat(uae_u16 v)
{
	int shift = blt_info.bltcon1 >> 12;
	if (blitdesc) {
		blt_info.bltbhold = (((uae_u32)v << 16) | blt_info.bltbold) >> (16 - shift);
	} else {
		blt_info.bltbhold = (((uae_u32)blt_info.bltbold << 16) | v) >> shift;
	}
	blt_info.bltbold = v;
}

static void blitter_dodma_new(struct rgabuf *rga, int ch, bool addmod)
{
	uae_u16 dat = 0;
	uaecptr pt = rga->pv;

	switch (ch)
	{
	case 1: // A
	{
		uae_u16 reg = 0x74;
		record_dma_blit(rga->reg, 0, pt);
		blt_info.bltadat = dat = chipmem_wget_indirect(pt);
		record_dma_blit_val(dat);
		regs.chipset_latch_rw = blt_info.bltadat;
		break;
	}
	case 2: // B
	{
		uae_u16 reg = 0x72;
		record_dma_blit(rga->reg, 0, pt);
		blt_info.bltbdat = dat = chipmem_wget_indirect(pt);
		record_dma_blit_val(dat);
		regs.chipset_latch_rw = blt_info.bltbdat;
		blitter_loadbdat(dat);
		blineb = blt_info.bltbhold;
		break;
	}
	case 3: // C
	{
		uae_u16 reg = 0x70;
		record_dma_blit(rga->reg, 0, pt);
		blt_info.bltcdat = dat = chipmem_wget_indirect(pt);
		record_dma_blit_val(dat);
		regs.chipset_latch_rw = blt_info.bltcdat;
		blt_info.bltchold2 = blt_info.bltcdat;
		break;
	}
	default:
		abort();
	}

	regs.chipset_latch_rw = dat;
#if BLIT_TRACE == 2
	if (ch == 1 || ch == 2 || ch == 3) {
		uae_u32 v1 = (ch << 16) | dat;
		uae_u32 v2 = pt;
		if (bdp[0] != v1 || bdp[1] != v2) {
			write_log("MISMATCH!\n");
			activate_debugger();
		}
		bdp += 2;
	}
#endif
#if BLIT_TRACE == 1
	if (ch == 1 || ch == 2 || ch == 3) {
		*bdp++ = (4 << 16) | dat;
		*bdp++ = pt;
	}
#endif
}

static void decide_blitter_idle(uae_u32 value)
{
	markidlecycle();
}

static void calc_mods(void)
{
	blit_modadda = blt_info.bltamod;
	blit_modaddb = blt_info.bltbmod;
	blit_modaddc = blt_info.bltcmod;
	blit_modaddd = blt_info.bltdmod;
	if (blitdesc) {
		blit_modadda = -blit_modadda;
		blit_modaddb = -blit_modaddb;
		blit_modaddc = -blit_modaddc;
		blit_modaddd = -blit_modaddd;
	}
	blit_add = blitdesc ? -2 : 2;
}

static void maybe_load_mods(void)
{
	if (blitter_delayed_update) {
		evt_t c = get_cycles();
		if (c == blt_info.blt_mod_cycles[0]) {
			blt_info.bltamod = blt_info.bltamod_next;
			blt_info.blt_mod_cycles[0] = 0;
		}
		if (c == blt_info.blt_mod_cycles[1]) {
			blt_info.bltbmod = blt_info.bltbmod_next;
			blt_info.blt_mod_cycles[1] = 0;
		}
		if (c == blt_info.blt_mod_cycles[2]) {
			blt_info.bltcmod = blt_info.bltcmod_next;
			blt_info.blt_mod_cycles[2] = 0;
		}
		if (c == blt_info.blt_mod_cycles[3]) {
			blt_info.bltdmod = blt_info.bltdmod_next;
			blt_info.blt_mod_cycles[3] = 0;
		}
		if (c == blt_info.blt_change_cycles) {
			blitter_doblit_last();
		}
		blitdesc = bltcon1_next & BLTDESC;
		blit_dof = ecs_agnus && (bltcon1_next & BLTDOFF);

		calc_mods();
		if (!blt_info.blt_mod_cycles[0] && !blt_info.blt_mod_cycles[1] && !blt_info.blt_mod_cycles[2] && !blt_info.blt_mod_cycles[3]) {
			blitter_delayed_update = false;
		}
	}
}

void process_blitter(struct rgabuf *rga)
{
	int hpos = agnus_hpos;
	uae_u32 dat = rga->bltdat;

	int c = dat & 7;
	bool line = (dat & BLITTER_PIPELINE_LINE) != 0;

	bool addmod = (dat & BLITTER_PIPELINE_ADDMOD) != 0;
	uae_u32 value = 0;
	int added = 0;

	blit_totalcyclecounter++;

	if (c == 0) {

		decide_blitter_idle(value);

	} else if (c == 1 && line) { // line 1/4 (A, free)

		decide_blitter_idle(value);
		if (dat & BLITTER_PIPELINE_FIRST) {
			blitter_line_proc_status();
			blitter_line_proc_apt();
		}

	} else if (c == 3 && line) { // line 2/4 (C)

		if (!(dat & BLITTER_PIPELINE_FIRST) && blitlineloop && !(blt_info.bltcon1 & BLTSUD)) {
			blitter_line_proc_cpt_y();
			blitlineloop = 0;
		}
		bool skip = rga->conflict;
		record_dma_blit(rga->reg, 0, blt_info.bltcpt);
		if (!skip) {
			blt_info.bltcdat = chipmem_wget_indirect(blt_info.bltcpt);
			regs.chipset_latch_rw = blt_info.bltcdat;
			record_dma_blit_val(blt_info.bltcdat);
		}

		if (dat & BLITTER_PIPELINE_FIRST) {
			blitter_line_minterm(dat);
			blitter_doblit_last();
			blitter_line_proc_cpt_x();
		} else {
			// W>2 special case
			blitter_line_minterm_extra();
			blitter_doblit_last();
		}

	} else if (c == 5 && line) { // line 3/4 (free)

		decide_blitter_idle(value);

		// this needs to be done before D channel transfer
		// because onedot state needs to be known 1 CCK in advance
		// so that blitter/cpu cycle allocation works correctly.
		blitter_line_adat();
		blitter_line_ovf();
		if (blt_info.hblitsize == 1) {
			blitter_line_proc_status();
		} else {
			if (blitlineloop) {
				blitter_line_proc_cpt_y();
				blitlineloop = 0;
			}
		}
		blitter_line_sign();
		blitter_nxline();

		blt_info.bltddatl = blt_info.bltddat;
		blt_info.bltdptl = blt_info.bltdpt;
		blitlinepixel2 = blitlinepixel;

		blitlinepixel = 0;
		blitlineloop = 1;

		blt_info.bltdpt = blt_info.bltcpt;

		blitter_line_minterm(dat);
		blitter_doblit_last();

	} else if ((c == 4 || c == 6) && line) { // line 4/4 (C/D)

		if (c == 4) {
			bool skip = rga->conflict;
			record_dma_blit(rga->reg, blt_info.bltddatl, blt_info.bltdptl);
			if (!skip) {
				blit_chipmem_agnus_wput(blt_info.bltdptl, blt_info.bltddatl, MW_MASK_BLITTER_D_L);
				regs.chipset_latch_rw = blt_info.bltddatl;
			}

			// copper conflict: after blitter dma transfer,
			// copy new copper pointer to conflicting blitter address pointer.
			if (rga->conflict) {
				rga->p = rga->conflict;
				*rga->p = rga->conflictaddr;
				rga->pv = rga->conflictaddr;
			}
		} else {
			markidlecycle();
		}

	} else {

		// normal mode channels
		if (c == 4) {
			blitter_doddma_new(rga, addmod);

			if (dat & BLITTER_PIPELINE_LASTD) {
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_BLITFINALD);
				}
#endif
			}

		} else {
			if (c == 5) {
				c = 1;
			}
			blitter_dodma_new(rga, c, addmod);
		}

		uaecptr pt = rga->pv;
		if (!blitline) {
			pt += rga->bltadd;
		}
		if (addmod) {
			pt += rga->bltmod;
		}
		*rga->p = pt;

		// copper conflict: after blitter dma transfer,
		// copy new copper pointer to conflicting blitter address pointer OR used modulo (if any)
		if (rga->conflict) {
			rga->p = rga->conflict;
			*rga->p = rga->conflictaddr | rga->bltmod;
			rga->pv = rga->conflictaddr | rga->bltmod;
		}

	}

#ifdef DEBUGGER
	if (debug_dma && c && addmod) {
		record_dma_event(DMA_EVENT_MODADD);
	}
#endif

	if ((dat & BLITTER_PIPELINE_BLIT) && !blitline) {
		blitter_loadblit(dat);
		blitter_doblit_last();
	}

	if (dat & BLITTER_PIPELINE_PROCESS) {
		blitfc = !!(blt_info.bltcon1 & BLTFC);
	}
}

static bool is_done(void)
{
	if (blt_info.blit_main) {
		return false;
	}
	if (!blitter_cycle_exact) {
		return true;
	}
	if (!shifter_d_armed && blt_info.blit_count_done && !shifter[0] && !shifter[1] && !shifter[2] && !shifter[3] &&
		!shifter_d1 && !shifter_d2 &&
		!shifter_d_aga) {
		return true;
	}
	return false;
}

void generate_blitter(void)
{
	if (!blitter_cycle_exact) {
		return;
	}

	if (get_cycles() == blt_info.finishcycle_copper) {
		blitter_done_notify();
	}

	blitter_next_cycle_always();

	// fully idle?
	if (!shifter_d_armed && blt_info.blit_count_done && !shifter[0] && !shifter[1] && !shifter[2] && !shifter[3] &&
		!shifter_d1 && !shifter_d2 &&
		!shifter_d_aga) {
		if (blt_info.blit_queued == 1) {
			blitter_end();
			goto end;
		}
	}


	if (!blt_info.blit_count_done) {
		blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
	}

	if (blt_info.blit_queued) {

		bool ena = blitter_cant_access() == 0;
		bool alloc = check_rga_free_slot_in() == false;
		bool pri = (dmacon & DMA_BLITPRI) != 0;
		bool bstreq = blt_info.nasty_cnt >= BLIT_NASTY_CPU_STEAL_CYCLE_COUNT && !pri;

		// CPU steals the cycle if CPU has waited long enough and current cyle is not free.
		if (!ena || alloc || bstreq) {
			blit_misscyclecounter++;
			if (ena && !alloc && bstreq) {
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_CPUBLITTERSTOLEN);
					//activate_debugger();
				}
#endif
			}
			// reset any pending delayed modulo changes
			blt_info.bltamod = blt_info.bltamod_next;
			blt_info.bltbmod = blt_info.bltbmod_next;
			blt_info.bltcmod = blt_info.bltcmod_next;
			blt_info.bltdmod = blt_info.bltdmod_next;
			calc_mods();
			goto end;
		}

		int c = get_current_channel();

		blt_info.blit_queued--;
		if (!blt_info.blit_count_done) {
			blit_cyclecounter++;
			if (blit_cyclecounter == (-CYCLECOUNT_START) + 1) {
				shifter_d_armed = false;
				blt_info.blitzero = 1;
				blt_info.got_cycle = 1;
			}
		}

		bool addmod = false;
		int reg = 0x1fe;
		int mod = 0;
		uaecptr *p = NULL;
		bool idlecycle = false;

		if (c == 1) {
			reg = 0x74;
			p = &blt_info.bltapt;
			mod = blit_modadda;
		} else if (c == 2) {
			reg = 0x72;
			p = &blt_info.bltbpt;
			mod = blit_modaddb;
		} else if (c == 3) {
			reg = 0x70;
			p = &blt_info.bltcpt;
			mod = blit_modaddc;
		} else if (c == 4) {
			reg = 0x00;
			p = &blt_info.bltdpt;
			mod = blit_modaddd;
			shifter_d_armed = false;
		}

		// restore copper BLTxPTx write in same cycle
		if (c >= 1 && c <= 4) {
			evt_t cycs = get_cycles();
			if (cycs == blt_info.blt_ch_cycles[c - 1]) {
				blt_info.blt_ch_cycles[c - 1] = 0;
				switch (c)
				{
					case 1:
					blt_info.bltapt = blt_info.bltapt_prev;
					break;
					case 2:
					blt_info.bltbpt = blt_info.bltbpt_prev;
					break;
					case 3:
					blt_info.bltcpt = blt_info.bltcpt_prev;
					break;
					case 4:
					blt_info.bltdpt = blt_info.bltdpt_prev;
					break;
				}
			}
		}

		uae_u32 v = CYCLE_PIPE_BLITTER;

		if (c == 1 || c == 2 || c == 3) {
			if (blitter_hcounter + 1 == blt_info.hblitsize) {
				addmod = true;
			}
		} else if (c == 4) {
			if (blitter_hcounter == 0) {
				addmod = true;
			}
			if (blt_info.blit_count_done) {
				v |= BLITTER_PIPELINE_LASTD;
			}
		}

		v |= c | (addmod ? BLITTER_PIPELINE_ADDMOD : 0);

		if (blitter_hcounter == 0) {
			v |= BLITTER_PIPELINE_FIRST;
		}
		if (blitter_hcounter == blt_info.hblitsize - 1) {
			v |= BLITTER_PIPELINE_LAST;
		}
		if (blitline) {
			v |= BLITTER_PIPELINE_LINE;

			if (c == 1) {
				idlecycle = true;
			}
			if (c == 5) {
				idlecycle = true;
			}
		}

		bool doddat = false;
		bool wasdone = blt_info.blit_count_done;

		doddat = blitter_next_cycle(blit_cyclecounter == 0);

		if (doddat) {
			v |= BLITTER_PIPELINE_BLIT;
			if (blitter_hcounter == 0) {
				v |= BLITTER_PIPELINE_PROCESS;
			}
		}
		// finished?
		if (!wasdone && blt_info.blit_count_done) {
			v |= BLITTER_PIPELINE_FINISHED;
			if (!blt_info.blit_main) {
				write_log(_T("blitter blit_main already cleared!?\n"));
			}
			if (blitline || !(blt_info.bltcon0 & BLTCHD) || !aga_mode) {
				blitter_done_all(false);
			} else {
				// AGA and blit has final D write: delay busy clear by 2 CCKs
				shifter_d_aga |= 1;
			}
		}

		if (reg == 0x1fe) {
			idlecycle = true;
		}

		struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_BLITTER, reg, p);
		rga->bltdat = v;
		rga->bltmod = mod;
		rga->bltadd = blit_add;
		if (idlecycle) {
			rga->alloc = -1;
		}
	}
end:;
	maybe_load_mods();
}

void decide_blitter(int until_hpos)
{
}

#else
void decide_blitter (int hpos) { }
#endif

static void blitter_force_finish(bool state)
{
	uae_u16 odmacon;
	if (!blt_info.blit_main) {
		return;
	}
	/* blitter is currently running
	* force finish (no blitter state support yet)
	*/
	odmacon = dmacon;
	dmacon |= DMA_MASTER | DMA_BLITTER;
	if (state) {
		write_log(_T("forcing blitter finish\n"));
	}
	if (blitter_cycle_exact && !immediate_blits) {
		int rounds = 10000;
		while ((blt_info.blit_main) && rounds > 0) {
			decide_blitter(-1);
			rounds--;
		}
		if (rounds == 0) {
			write_log(_T("blitter froze!?\n"));
		}
	} else {
		actually_do_blit();
	}
	blitter_done_all(true);
	dmacon = odmacon;
}

void reset_blit(int bltcon)
{
	if (!blt_info.blit_queued) {
		blitdesc = blt_info.bltcon1 & BLTDESC;
		return;
	}
	if (bltcon) {
		blit_bltset(bltcon);
	}
}

static bool waitingblits(void)
{
	static int warned = 50;

	// crazy large blit size? don't wait.. (Vital / Mystic)
	if (blt_info.vblitsize * blt_info.hblitsize * 2 > 2 * 1024 * 1024) {
		if (warned) {
			warned--;
			write_log(_T("Crazy waiting_blits detected PC=%08x W=%d H=%d\n"), M68K_GETPC, blt_info.vblitsize, blt_info.hblitsize);
		}
		return false;
	}

	bool waited = false;
	int waiting = 0;
	int vpos_prev = vpos;
	while (!is_done() && dmaen(DMA_BLITTER)) {
		waited = true;
		x_do_cycles (4 * CYCLE_UNIT);
		if (vpos_prev != vpos) {
			vpos_prev = vpos;
			waiting++;
			if (waiting > maxvpos * 5) {
				break;
			}
		}
	}
	if (warned && waited) {
		warned--;
		write_log(_T("waiting_blits detected PC=%08x\n"), M68K_GETPC);
	}
	if (is_done()) {
		return true;
	}
	return false;
}

static void blitter_start_init(void)
{
#if 0
	if (blt_info.blit_main) {
		write_log("Blitter start but blitter active");
	}
	if (blt_info.blit_finald) {
		write_log("Blitter start but final D still pending");
	}
#endif

	blt_info.blit_queued = 0;
	blitline_started = blt_info.bltcon1 & BLTLINE;
	blitlineloop = 1;
	blt_info.finishcycle_dmacon = 0;
	blt_info.finishcycle_copper = 0;

	blit_bltset(1 | 2);
	blitter_delayed_update = true;
	maybe_load_mods();
	blt_info.blit_interrupt = 0;

	blt_info.bltaold = 0;
	blt_info.bltbold = 0;

	int bshift = blt_info.bltcon1 >> 12;
	blineb = (blt_info.bltbdat >> bshift) | (blt_info.bltbdat << (16 - bshift));
	blitonedot = 0;
	blitlinepixel = 0;

	if (!(dmacon & DMA_BLITPRI) && blt_info.nasty_cnt >= BLIT_NASTY_CPU_STEAL_CYCLE_COUNT) {
		blt_info.wait_nasty = 1;
	} else {
		blt_info.wait_nasty = 0;
	}
}

void do_blitter(int copper, uaecptr pc)
{
	int cycles;

#if BLITTER_DEBUG
	if ((log_blitter & 2)) {
		if (blt_info.blit_main) {
			write_log (_T("blitter was already active! PC=%08x\n"), M68K_GETPC);
		}
	}
#endif

	bltcon0_old = blt_info.bltcon0;
	bltcon1_old = blt_info.bltcon1;

	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	immediate_blits = currprefs.immediate_blits;
	blt_info.got_cycle = 0;
	blit_firstline_cycles = blit_first_cycle = get_cycles();
	blit_misscyclecounter = 0;
	blit_last_cycle = 0;
	blit_maxcyclecounter = 0;
	blit_cyclecounter = 0;
	blit_totalcyclecounter = 0;
	blt_info.blit_pending = 1;
	blt_info.blit_count_done = 0;
	blt_info.blit_queued = 0;

	blitter_start_init();

	if (blitline) {
		cycles = blt_info.vblitsize * blt_info.hblitsize;
	} else {
		cycles = blt_info.vblitsize * blt_info.hblitsize;
		blit_firstline_cycles = blit_first_cycle + (blit_cyclecount * blt_info.hblitsize) * CYCLE_UNIT + cpu_cycles;
	}

#ifdef DEBUGGER
	if (memwatch_enabled || BLITTER_DEBUG) {
		blitter_debugsave(copper, pc);
	}
#endif

#if BLITTER_DEBUG
	if ((log_blitter & 1) || ((log_blitter & 32) && !blitline)) {
		if (1) {
			int ch = 0;
			if (blit_ch & 1)
				ch++;
			if (blit_ch & 2)
				ch++;
			if (blit_ch & 4)
				ch++;
			if (blit_ch & 8)
				ch++;
			write_log(_T("blitstart: %dx%d ch=%d %d d=%d f=%02x n=%d pc=%08x l=%d dma=%04x %s\n"),
				blt_info.hblitsize, blt_info.vblitsize, ch, cycles,
				blitdesc ? 1 : 0, blitfill, dmaen(DMA_BLITPRI) ? 1 : 0, M68K_GETPC, blitline,
				dmacon, ((dmacon & (DMA_MASTER | DMA_BLITTER)) == (DMA_MASTER | DMA_BLITTER)) ? _T("") : _T(" off!"));
			blitter_dump();
		}
	}
#endif

	blit_slowdown = 0;

	unset_special (SPCFLAG_BLTNASTY);
	if (dmaen(DMA_BLITPRI)) {
		set_special(SPCFLAG_BLTNASTY);
	}

	if (dmaen(DMA_BLITTER)) {
		blt_info.blit_main = 1;
		blt_info.blit_pending = 0;
	}

	blit_maxcyclecounter = 0x7fffffff;

	if (blitter_cycle_exact) {
		if (immediate_blits) {
			blt_info.blitzero = 1;
			if (dmaen(DMA_BLITTER)) {
				blitter_doit();
			}
			return;
		}
		if (log_blitter & 8) {
			blitter_handler (0);
		} else {
			blitter_hcounter = 0;
			blitter_vcounter = 0;
			blit_cyclecounter = -CYCLECOUNT_START;
			blit_maxcyclecounter = blt_info.hblitsize * blt_info.vblitsize + 2;
			blt_info.blit_pending = 0;
			blt_info.blit_main = 1;
			blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
		}
		return;
	}

	blt_info.blitzero = 1;
	if (blt_info.vblitsize == 0) {
		if (dmaen(DMA_BLITTER)) {
			blitter_done_all(true);
		}
		return;
	}

	if (dmaen(DMA_BLITTER)) {
		blt_info.got_cycle = 1;
	}

	if (immediate_blits) {
		if (dmaen(DMA_BLITTER)) {
			blitter_doit();
		}
		return;
	}
	
	blit_cyclecounter = cycles * blit_cyclecount;
	event2_newevent (ev2_blitter, makebliteventtime(blit_cyclecounter), 0);
}

void blitter_check_start (void)
{
	if (blt_info.blit_pending && !blt_info.blit_main) {
		blt_info.blit_pending = 0;
		blt_info.blit_main = 1;
		blitter_start_init();
		if (immediate_blits) {
			blitter_doit();
		}
	}
}

void maybe_blit(int hack)
{
	static int warned = 10;

	if (is_done()) {
		return;
	}

	if (savestate_state)
		return;

	if (dmaen(DMA_BLITTER) && (currprefs.cpu_model >= 68020 || !currprefs.cpu_memory_cycle_exact)) {
		bool doit = false;
		if (currprefs.waiting_blits == 3) { // always
			doit = true;
		} else if (currprefs.waiting_blits == 2) { // noidle
			if (blit_dmacount == blit_cyclecount && (regs.spcflags & SPCFLAG_BLTNASTY))
				doit = true;
		} else if (currprefs.waiting_blits == 1) { // automatic
			if (blit_dmacount == blit_cyclecount && (regs.spcflags & SPCFLAG_BLTNASTY))
				doit = true;
			else if (currprefs.m68k_speed < 0)
				doit = true;
		}
		if (doit) {
			if (waitingblits())
				return;
		}
	}

	if (warned && dmaen (DMA_BLITTER) && blt_info.got_cycle) {
		warned--;
#ifdef DEBUGGER
		debugtest (DEBUGTEST_BLITTER, _T("program does not wait for blitter tc=%d\n"), blit_cyclecounter);
#endif
#if BLITTER_DEBUG
		if (log_blitter)
			warned = 0;
		if (log_blitter & 2) {
			warned = 10;
			write_log (_T("program does not wait for blitter PC=%08x\n"), M68K_GETPC);
			//activate_debugger();
			//blitter_done (hpos);
		}
#endif
	}

	if (blitter_cycle_exact) {
		goto end;
	}

	if (hack == 1 && get_cycles() < blit_firstline_cycles) {
		goto end;
	}

	blitter_handler(0);
end:;
#if BLITTER_DEBUG
	if (log_blitter)
		blitter_delayed_debug = 1;
#endif
}

int blitnasty(void)
{
	int cycles, ccnt;

	if (!blt_info.blit_main) {
		return -1;
	}
	if (!dmaen(DMA_BLITTER)) {
		return -1;
	}
	if (blitter_cycle_exact) {
		return -1;
	}
	if (blit_last_cycle >= blit_cyclecount && blit_dmacount == blit_cyclecount) {
		return 0;
	}
	cycles = int((get_cycles() - blit_first_cycle) / CYCLE_UNIT);
	ccnt = 0;
	while (blit_last_cycle + blit_cyclecount < cycles) {
		ccnt += blit_dmacount;
		blit_last_cycle += blit_cyclecount;
	}
	return ccnt;
}

/* very approximate emulation of blitter slowdown caused by bitplane DMA */
void blitter_slowdown(int ddfstrt, int ddfstop, int totalcycles, int freecycles)
{
	static int oddfstrt, oddfstop, ototal, ofree;
	static int slow;

	if (!totalcycles || ddfstrt < 0 || ddfstop < 0) {
		return;
	}
	if (ddfstrt != oddfstrt || ddfstop != oddfstop || totalcycles != ototal || ofree != freecycles) {
		int linecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * totalcycles;
		int freelinecycles = ((ddfstop - ddfstrt + totalcycles - 1) / totalcycles) * freecycles;
		int dmacycles = (linecycles * blit_dmacount) / blit_cyclecount;
		oddfstrt = ddfstrt;
		oddfstop = ddfstop;
		ototal = totalcycles;
		ofree = freecycles;
		slow = 0;
		if (dmacycles > freelinecycles)
			slow = dmacycles - freelinecycles;
	}
	if (blit_slowdown < 0 || blitline) {
		return;
	}
	blit_slowdown += slow;
	blit_misscyclecounter += slow;
}

#if BLIT_TRACE == 1
void blitter_save_trace(void)
{
	FILE *f = fopen("c:\\temp\\blitter_trace.bin", "wb");
	if (f) {
		fwrite(blit_tracer, 1, bdp - blit_tracer, f);
		fclose(f);
	}
}
#endif

void blitter_reset(void)
{
	blt_info.blt_ch_cycles[0] = blt_info.blt_ch_cycles[1] = blt_info.blt_ch_cycles[2] = blt_info.blt_ch_cycles[3] = 0;
	blt_info.blt_mod_cycles[0] = blt_info.blt_mod_cycles[1] = blt_info.blt_mod_cycles[2] = blt_info.blt_mod_cycles[3] = 0;
	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	immediate_blits = currprefs.immediate_blits;
	shifter[0] = shifter[1] = shifter[2] = shifter[3] = false;
	shifter_d1 = shifter_d2 = 0;
	shifter_d_aga = 0;
	shifter_d_armed = false;
	shifter_skip_b = false;
	shifter_skip_y = false;
	blitfill = 0;
	blitter_start_init();
	blt_info.blit_main = 0;
	blt_info.blit_pending = 0;
	blt_info.blit_queued = 0;
	blt_info.blit_count_done = 0;
#if BLIT_TRACE == 1
	bdp = blit_tracer;
#endif
#if BLIT_TRACE == 2
	bdp = blit_tracer;
	FILE* f = fopen("c:\\temp\\blitter_trace.bin", "rb");
	if (f) {
		fread(bdp, 1, sizeof(blit_tracer), f);
		fclose(f);
	}
#endif
}

#ifdef SAVESTATE

void restore_blitter_start(void)
{
	blt_statefile_type = -1;
	bltcon0_next = blt_info.bltcon0;
	bltcon1_next = blt_info.bltcon1;
	blitter_reset();
}

void restore_blitter_finish(void)
{
#ifdef DEBUGGER
	record_dma_reset(0);
	record_dma_reset(0);
#endif
	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	immediate_blits = currprefs.immediate_blits;
	if (blt_statefile_type == 0) {
		blt_info.blit_interrupt = 1;
		if (blt_info.blit_pending) {
			write_log (_T("blitter was started but DMA was inactive during save\n"));
			//do_blitter (0);
		}
		if (blt_delayed_irq < 0) {
			if (intreq & 0x0040) {
				blt_delayed_irq = 3;
			}
			intreq &= ~0x0040;
		}
	} else {
		blitter_delayed_update = true;
		maybe_load_mods();
	}
	//activate_debugger();
}

uae_u8 *restore_blitter(uae_u8 *src)
{
	uae_u32 flags = restore_u32();

	if (!(flags & 8)) {
		blt_statefile_type = 0;
		blt_delayed_irq = 0;
		blt_info.blit_pending = 0;
		blt_info.blit_main = 0;
		if (flags & 4) {
			if (!(flags & 1)) {
				blt_info.blit_pending = 1;
			}
		}
		if (flags & 2) {
			write_log (_T("blitter was force-finished when this statefile was saved\n"));
			write_log (_T("contact the author if restored program freezes\n"));
			// there is a problem. if system ks vblank is active, we must not activate
			// "old" blit's intreq until vblank is handled or ks 1.x thinks it was blitter
			// interrupt..
			blt_delayed_irq = -1;
		}
	}
	bltcon0_old = blt_info.bltcon0;
	bltcon1_old = blt_info.bltcon1;
	return src;
}

uae_u8 *save_blitter(size_t *len, uae_u8 *dstptr, bool newstate)
{
	uae_u8 *dstbak,*dst;

	if (dstptr) {
		dstbak = dst = dstptr;
	} else {
		dstbak = dst = xmalloc(uae_u8, 1000);
	}

	if (blt_info.blit_main) {
		save_u32(8 | 16);
	} else {
		save_u32(1 | 4 | 8 | 16);
	}

	save_u32(blit_cyclecounter);
	save_u16(blitter_hcounter);
	save_u16(blitter_vcounter);
	save_u8((shifter[0] ? 1 : 0) | (shifter[1] ? 2 : 0) | (shifter[2] ? 4 : 0) | (shifter[3] ? 8 : 0) |
		(shifter_skip_b_old ? 16 : 0) | (shifter_skip_y_old ? 32 : 0));
	save_u16(blt_info.bltaold);
	save_u16(blt_info.bltbold);
	save_u16(blt_info.bltamod_next);
	save_u16(blt_info.bltbmod_next);
	save_u16(blt_info.bltcmod_next);
	save_u16(blt_info.bltdmod_next);
	save_u16(bltcon0_next);
	save_u16(bltcon1_next);
	save_u8((shifter_d1 << 0) | (shifter_d2 << 2) | (shifter_d_armed ? 128 : 0));
	save_u8(shifter_d_aga);
	save_u64(blt_info.blt_mod_cycles[0]);
	save_u64(blt_info.blt_mod_cycles[1]);
	save_u64(blt_info.blt_mod_cycles[2]);
	save_u64(blt_info.blt_mod_cycles[3]);

	*len = dst - dstbak;
	return dstbak;

}

// totally non-real-blitter-like state save but better than nothing..

uae_u8 *restore_blitter_new(uae_u8 *src)
{
	uae_u8 state, tmp;

	blt_statefile_type = 1;
	blitter_cycle_exact = restore_u8();
	if (blitter_cycle_exact & 2) {
		blt_statefile_type = 2;
		blitter_cycle_exact = 1;
	}

	state = restore_u8();

	blit_first_cycle = restore_u32();
	blit_last_cycle = restore_u32();
	restore_u32();
	restore_u32();
	blit_maxcyclecounter = restore_u32();
	blit_firstline_cycles = restore_u32();
	blit_cyclecounter = restore_u32();
	blit_slowdown = restore_u32();
	blit_misscyclecounter = restore_u32();

	blitter_hcounter = restore_u16();
	restore_u16();
	blitter_vcounter = restore_u16();
	restore_u16();
	blit_ch = restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();
	blitfc = restore_u8();
	blitife = restore_u8();

	restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();

	restore_u8();
	restore_u8();
	blt_info.bltddat = restore_u16();
	restore_u16();

	blitline = restore_u8();
	blitfill = restore_u8();
	restore_u16(); //blinea = restore_u16();
	blineb = restore_u16();
	restore_u8();
	blitonedot = restore_u8();
	blitlinepixel = restore_u8();
	restore_u8();
	tmp = restore_u8();
	blitlinepixel = (tmp & 1) != 0;
	blitlineloop = (tmp & 2) != 0;
	blt_info.blit_interrupt = restore_u8();
	blt_delayed_irq = restore_u8();
	blt_info.blitzero = restore_u8();
	blt_info.got_cycle = restore_u8();

	restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();

	if (restore_u16() != 0x1234) {
		write_log(_T("blitter state restore error\n"));
	}

	blt_info.blitter_nasty = restore_u8();
	tmp = restore_u8();
	if (blt_statefile_type < 2) {
		tmp = 0;
		//blt_info.blit_finald = 0;
		restore_u8();
		restore_u8();
	} else {
		shifter[0] = (tmp & 1) != 0;
		shifter[1] = (tmp & 2) != 0;
		shifter[2] = (tmp & 4) != 0;
		shifter[3] = (tmp & 8) != 0;
		shifter_skip_b_old = (tmp & 16) != 0;
		shifter_skip_y_old = (tmp & 32) != 0;
		restore_u8();
		blit_ovf = restore_u8();
	}

	blt_info.blit_main = 0;
	blt_info.blit_pending = 0;

	if (!blitter_cycle_exact) {
		if (state > 0)
			do_blitter(0, 0);
	} else {
		// if old blitter active state file:
		// stop blitter. We can't support them anymore.
		if (state == 2 && blt_statefile_type < 2) {
			blt_info.blit_pending = 0;
			blt_info.blit_main = 0;
		} else {
			if (state == 1) {
				blt_info.blit_pending = 1;
			} else if (state == 2) {
				blt_info.blit_main = 1;
				blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
			}
//			if (blt_info.blit_finald) {
//				blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
//				blt_info.blit_main = 0;
//			}
			if (blt_statefile_type == 2) {
				blit_bltset(0);
			}
		}
		shifter_skip_b_old = shifter_skip_b;
		shifter_skip_y_old = shifter_skip_y;
		blitfill_old = blitfill;
	}

	blit_first_cycle |= ((uae_u64)restore_u32()) << 32;
	blit_firstline_cycles |= ((uae_u64)restore_u32()) << 32;

	blt_info.bltaold = restore_u16();
	blt_info.bltbold = restore_u16();

	blt_info.nasty_cnt = restore_u8();
	blt_info.wait_nasty = restore_u8();

	restore_u8();

	restore_u8(); //blt_info.finishhpos = restore_u8();

	blit_cyclecounter = restore_u32();

	for (int i = 0; i < 4; i++) {
//		blitter_pipe[i] = restore_u16();
//		if (blitter_pipe[i]) {
//			blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
//		}
		restore_u16();
		restore_u16();
		restore_u8();
//		cycle_line_pipe[i] = restore_u16();
//		cycle_line_slot[i] = restore_u8();
	}

	if (restore_u16() == 0x1235) {
		blt_info.bltamod_next = restore_u16();
		blt_info.bltbmod_next = restore_u16();
		blt_info.bltcmod_next = restore_u16();
		blt_info.bltdmod_next = restore_u16();
		bltcon0_next = restore_u16();
		bltcon1_next = restore_u16();
		tmp = restore_u8();
		shifter_d1 = tmp & 3;
		shifter_d2 = (tmp >> 2) & 3;
		shifter_d_armed = (tmp & 128) != 0;
		tmp = restore_u8();
		shifter_d_aga = tmp & 7;
		blt_info.finishcycle_dmacon = restore_u64();
		blt_info.finishcycle_copper = restore_u64();
		blt_info.blt_mod_cycles[0] = restore_u64();
		blt_info.blt_mod_cycles[1] = restore_u64();
		blt_info.blt_mod_cycles[2] = restore_u64();
		blt_info.blt_mod_cycles[3] = restore_u64();
		blt_info.blt_ch_cycles[0] = restore_u64();
		blt_info.blt_ch_cycles[1] = restore_u64();
		blt_info.blt_ch_cycles[2] = restore_u64();
		blt_info.blt_ch_cycles[3] = restore_u64();
	}

	//activate_debugger();

	return src;
}

uae_u8 *save_blitter_new(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak,*dst;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);

	uae_u8 state;
	save_u8(blitter_cycle_exact ? 3 : 0);
	if (!blt_info.blit_main) {
		state = 0;
	} else if (blt_info.blit_pending) {
		state = 1;
	} else {
		state = 2;
	}
	save_u8(state);

	if (blt_info.blit_main) {
		write_log(_T("BLITTER active while saving state\n"));
#if BLITTER_DEBUG
		if (log_blitter)
			blitter_dump();
#endif
	}

	save_u32((uae_u32)blit_first_cycle);
	save_u32(blit_last_cycle);
	save_u32(0);
	save_u32(0); //(blit_startcycles);
	save_u32(blit_maxcyclecounter);
	save_u32((uae_u32)blit_firstline_cycles);
	save_u32(blit_cyclecounter);
	save_u32(blit_slowdown);
	save_u32(blit_misscyclecounter);

	save_u16(blitter_hcounter);
	save_u16(0); //(blitter_hcounter2);
	save_u16(blitter_vcounter);
	save_u16(0); //(blitter_vcounter2);
	save_u8(blit_ch);
	save_u8(blit_dmacount);
	save_u8(blit_cyclecount);
	save_u8(0); //(blit_nod);
	save_u8(0); //blt_info.blit_finald);
	save_u8(blitfc);
	save_u8(blitife);

	save_u8(blt_info.bltcon1 >> 12);
	save_u8((16 - (blt_info.bltcon1 >> 12)));
	save_u8(blt_info.bltcon0 >> 12);
	save_u8(16 - (blt_info.bltcon0 >> 12));

	save_u8(0);
	save_u8(0); //(ddat2use);
	save_u16(blt_info.bltddat);
	save_u16(0); //(ddat2);

	save_u8(blitline);
	save_u8(blitfill);
	save_u16(blt_info.bltadat);
	save_u16(blineb);
	save_u8(blt_info.bltcon0 >> 12);
	save_u8(blitonedot);
	save_u8(blitlinepixel);
	save_u8((blt_info.bltcon1 & BLTSING) != 0);
	save_u8((blitlinepixel ? 1 : 0) | (blitlineloop ? 2 : 0));
	save_u8(blt_info.blit_interrupt);
	save_u8(blt_delayed_irq);
	save_u8(blt_info.blitzero);
	save_u8(blt_info.got_cycle);
	
	save_u8(0); //(blit_frozen);
	save_u8(0);
	save_u8(0); //original_ch);
	save_u8(0); //original_fill);
	save_u8(0); //original_line);
	save_u8(0); //get_cycle_diagram_type (blit_diag));

	save_u16(0x1234);

	save_u8(blt_info.blitter_nasty);
	save_u8((shifter[0] ? 1 : 0) | (shifter[1] ? 2 : 0) | (shifter[2] ? 4 : 0) | (shifter[3] ? 8 : 0) |
		(shifter_skip_b_old ? 16 : 0) | (shifter_skip_y_old ? 32 : 0));
	save_u8(0); //blt_info.blit_finald);
	save_u8(blit_ovf);

	save_u32(blit_first_cycle >> 32);
	save_u32(blit_firstline_cycles >> 32);

	save_u16(blt_info.bltaold);
	save_u16(blt_info.bltbold);

	save_u8(blt_info.nasty_cnt);
	save_u8(blt_info.wait_nasty);

	save_u8(0);
	save_u8(0xff); //blt_info.finishhpos);

	save_u32(blit_cyclecounter);

	for (int i = 0; i < 4; i++) {
		save_u16(0); //save_u16(blitter_pipe[i]);
		save_u16(0); //cycle_line_pipe[i]);
		save_u8(0); //cycle_line_slot[i]);
	}

	save_u16(0x1235);
	save_u16(blt_info.bltamod_next);
	save_u16(blt_info.bltbmod_next);
	save_u16(blt_info.bltcmod_next);
	save_u16(blt_info.bltdmod_next);
	save_u16(bltcon0_next);
	save_u16(bltcon1_next);
	save_u8((shifter_d1 << 0) | (shifter_d2 << 2) | (shifter_d_armed ? 128 : 0));
	save_u8(shifter_d_aga & 7);
	save_u64(blt_info.finishcycle_dmacon);
	save_u64(blt_info.finishcycle_copper);

	save_u64(blt_info.blt_mod_cycles[0]);
	save_u64(blt_info.blt_mod_cycles[1]);
	save_u64(blt_info.blt_mod_cycles[2]);
	save_u64(blt_info.blt_mod_cycles[3]);
	save_u64(blt_info.blt_ch_cycles[0]);
	save_u64(blt_info.blt_ch_cycles[1]);
	save_u64(blt_info.blt_ch_cycles[2]);
	save_u64(blt_info.blt_ch_cycles[3]);

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */
