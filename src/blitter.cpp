/*
* UAE - The Un*x Amiga Emulator
*
* Custom chip emulation
*
* (c) 1995 Bernd Schmidt, Alessandro Bissacco
* (c) 2002 - 2021 Toni Wilen
*/

#define SPEEDUP 1
#define BLITTER_DEBUG 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "blitter.h"
#include "blit.h"
#include "savestate.h"
#include "debug.h"

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

/* we must not change ce-mode while blitter is running.. */
static int blitter_cycle_exact, immediate_blits;
static int blt_statefile_type;

uae_u16 bltcon0, bltcon1;
uae_u32 bltapt, bltbpt, bltcpt, bltdpt;
uae_u32 bltptx;
int bltptxpos, bltptxc;

static uae_u16 blineb;
static int blitline, blitfc, blitfill, blitife, blitdesc, blit_ovf;
static bool blitfill_idle;
static int blitline_started, blitlineloop;
static int blitonedot, blitlinepixel;
static int blit_add;
static int blit_modadda, blit_modaddb, blit_modaddc, blit_modaddd;
static int blit_ch;
static bool shifter_skip_b, shifter_skip_y;
static bool shifter_skip_b_old, shifter_skip_y_old;
static uae_u16 bltcon0_old, bltcon1_old;
static bool shifter[4], shifter_out;
static int shifter_first;
static bool blitline_c, blitfill_c;

static int blitter_delayed_debug;
#ifdef BLITTER_SLOWDOWNDEBUG
static int blitter_slowdowndebug;
#endif

struct bltinfo blt_info;

static uae_u8 blit_filltable[256][4][2];
uae_u32 blit_masktable[BLITTER_MAX_WORDS];

static int blit_cyclecounter, blit_waitcyclecounter;
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
static int blit_faulty;
static int blt_delayed_irq;
static uae_u16 ddat;
static int ddatuse;
static int blit_dof;

static int last_blitter_hpos;

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
#define CYCLECOUNT_START 3

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

void build_blitfilltable (void)
{
	unsigned int d, fillmask;
	int i;

	for (i = 0; i < BLITTER_MAX_WORDS; i++)
		blit_masktable[i] = 0xFFFF;

	for (d = 0; d < 256; d++) {
		for (i = 0; i < 4; i++) {
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

static void record_dma_blit(uae_u16 reg, uae_u16 v, uae_u32 addr, int hpos)
{
#ifdef DEBUGGER
	if (debug_dma && blitter_cycle_exact) {
		if (reg == 0) {
			record_dma_write(reg, v, addr, hpos, vpos, DMARECORD_BLITTER, 3 + (blitline_c ? 0x20 : (blitfill_c ? 0x10 : 0)));
		} else {
			int r = 0;
			if (reg == 0x70)
				r = 2;
			if (reg == 0x72)
				r = 1;
			if (reg == 0x74)
				r = 0;
			record_dma_read(reg, addr, hpos, vpos, DMARECORD_BLITTER, r + (blitline_c ? 0x20 : (blitfill_c ? 0x10 : 0)));
		}
	}
	if (memwatch_enabled) {
		if (reg == 0) {
			uae_u32 mask = MW_MASK_BLITTER_D_N;
			if (blitfill_c)
				mask = MW_MASK_BLITTER_D_F;
			if (blitline_c)
				mask = MW_MASK_BLITTER_D_L;
			debug_putpeekdma_chipram(addr, v, mask, reg, 0x054);
		} else if (reg == 0x70) {
			debug_getpeekdma_chipram(addr, MW_MASK_BLITTER_C, reg, 0x48);
		} else if (reg == 0x72) {
			debug_getpeekdma_chipram(addr, MW_MASK_BLITTER_B, reg, 0x4c);
		} else if (reg == 0x74) {
			debug_getpeekdma_chipram(addr, MW_MASK_BLITTER_A, reg, 0x52);
		}
	}
#endif
}

static void blitter_debugsave(int copper, uaecptr pc)
{
	debug_bltcon0 = bltcon0;
	debug_bltcon1 = bltcon1;
	debug_bltsizev = blt_info.vblitsize;
	debug_bltsizeh = blt_info.hblitsize;
	debug_bltapt = bltapt;
	debug_bltbpt = bltbpt;
	debug_bltcpt = bltcpt;
	debug_bltdpt = bltdpt;
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

static void blitter_dump (void)
{
	int chipsize = currprefs.chipmem.size;
	console_out_f(_T("PT A=%08X B=%08X C=%08X D=%08X\n"), bltapt, bltbpt, bltcpt, bltdpt);
	console_out_f(_T("CON0=%04X CON1=%04X DAT A=%04X B=%04X C=%04X\n"),
		bltcon0, bltcon1, blt_info.bltadat, blt_info.bltbdat, blt_info.bltcdat);
	console_out_f(_T("AFWM=%04X ALWM=%04X MOD A=%04X B=%04X C=%04X D=%04X\n"),
		blt_info.bltafwm, blt_info.bltalwm,
		blt_info.bltamod & 0xffff, blt_info.bltbmod & 0xffff, blt_info.bltcmod & 0xffff, blt_info.bltdmod & 0xffff);
	console_out_f(_T("PC=%08X DMA=%d\n"), m68k_getpc(), dmaen (DMA_BLITTER));

	if (((bltcon0 & BLTCHA) && bltapt >= chipsize) || ((bltcon0 & BLTCHB) && bltbpt >= chipsize) ||
		((bltcon0 & BLTCHC) && bltcpt >= chipsize) || ((bltcon0 & BLTCHD) && bltdpt >= chipsize))
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

static void markidlecycle(int hpos)
{
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event(DMA_EVENT_BLITSTARTFINISH, hpos, vpos);
	}
#endif
}

static void check_channel_mods(int hpos, int ch, uaecptr *pt)
{
	static int blit_warned = 100;

	if (bltptxpos < 0)
		return;
	if (bltptxpos != hpos)
		return;
	// if CPU write and non-CE: skip
	if (bltptxc < 0) {
		if (currprefs.cpu_model >= 68020 || !currprefs.cpu_cycle_exact) {
			bltptxpos = -1;
			return;
		}
	}
	if (ch == bltptxc || ch == -bltptxc) {
		if (blit_warned > 0) {
			write_log(_T("BLITTER: %08X -> %08X write to %cPT ignored! %08x\n"), bltptx, *pt, ch + 'A' - 1, m68k_getpc());
			blit_warned--;
		}
		bltptxpos = -1;
		*pt = bltptx;
	}
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
	if (blt_info.blit_interrupt) {
		return;
	}
	blt_info.blit_interrupt = 1;
	INTREQ_INT(6, 3);
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event(DMA_EVENT_BLITIRQ, current_hpos(), vpos);
	}
#endif
}

static void blitter_end(void)
{
	blt_info.blit_main = 0;
	blt_info.blit_finald = 0;
	blt_info.blit_queued = 0;
	event2_remevent(ev2_blitter);
	unset_special(SPCFLAG_BLTNASTY);
#if BLITTER_DEBUG
	if (log_blitter & 1) {
		write_log(_T("cycles %d, missed %d, total %d\n"),
			blit_totalcyclecounter, blit_misscyclecounter, blit_totalcyclecounter + blit_misscyclecounter);

	}
#endif
	blt_info.blitter_dangerous_bpl = 0;
}

static void blitter_done_all(int hpos)
{
	blt_info.blit_main = 0;
	blt_info.blit_finald = 0;
	if (m68k_interrupt_delay && hpos >= 0) {
		blt_info.finishhpos = hpos;
	} else {
		blt_info.finishhpos = -1;
	}
	blitter_interrupt();
	blitter_done_notify(blitline);
	if (!blt_info.blit_queued && !blt_info.blit_finald) {
		blitter_end();
	}
}

static void blitter_done_except_d(int hpos)
{
	blt_info.blit_main = 0;
	if (m68k_interrupt_delay && hpos >= 0) {
		blt_info.finishhpos = hpos;
	} else {
		blt_info.finishhpos = -1;
	}
	blitter_interrupt();
	blitter_done_notify(blitline);
}


static void blit_chipmem_agnus_wput(uaecptr addr, uae_u32 w, uae_u32 typemask)
{
	if (!(log_blitter & 4)) {
		if (blit_dof) {
			w = regs.chipset_latch_rw;
		}
#ifdef DEBUGGER
		debug_putpeekdma_chipram(addr, w, typemask, 0x000, 0x054);
#endif
		chipmem_wput_indirect(addr, w);
		regs.chipset_latch_rw = w;
	}
}

static void blitter_dofast (void)
{
	int i,j;
	uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
	uae_u8 mt = bltcon0 & 0xFF;
	uae_u16 ashift = bltcon0 >> 12;
	uae_u16 bshift = bltcon1 >> 12;

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

	if (bltcon0 & BLTCHA) {
		bltadatptr = bltapt;
		bltapt += (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
	}
	if (bltcon0 & BLTCHB) {
		bltbdatptr = bltbpt;
		bltbpt += (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
	}
	if (bltcon0 & BLTCHC) {
		bltcdatptr = bltcpt;
		bltcpt += (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
	}
	if (bltcon0 & BLTCHD) {
		bltddatptr = bltdpt;
		bltdpt += (blt_info.hblitsize * 2 + blt_info.bltdmod) * blt_info.vblitsize;
	}

#if SPEEDUP
	if (blitfunc_dofast[mt] && !blitfill) {
		(*blitfunc_dofast[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else
#endif
	{
		uae_u32 blitbhold = blt_info.bltbhold;
		int ashift = bltcon0 >> 12;
		int bshift = bltcon1 >> 12;
		uaecptr dstp = 0;
		int dodst = 0;

		for (j = 0; j < blt_info.vblitsize; j++) {
			blitfc = !!(bltcon1 & BLTFC);
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
				blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt) & 0xFFFF;
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

static void blitter_dofast_desc (void)
{
	int i,j;
	uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
	uae_u8 mt = bltcon0 & 0xFF;
	uae_u16 ashift = 16 - (bltcon0 >> 12);
	uae_u16 bshift = 16 - (bltcon1 >> 12);

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

	if (bltcon0 & BLTCHA) {
		bltadatptr = bltapt;
		bltapt -= (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
	}
	if (bltcon0 & BLTCHB) {
		bltbdatptr = bltbpt;
		bltbpt -= (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
	}
	if (bltcon0 & BLTCHC) {
		bltcdatptr = bltcpt;
		bltcpt -= (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
	}
	if (bltcon0 & BLTCHD) {
		bltddatptr = bltdpt;
		bltdpt -= (blt_info.hblitsize * 2 + blt_info.bltdmod) * blt_info.vblitsize;
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
			blitfc = !!(bltcon1 & BLTFC);
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
				blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt) & 0xFFFF;
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
	if (bltcon0 & BLTCHB) {
		// B (normally not enabled)
		record_dma_blit(0x72, 0, bltbpt, last_blitter_hpos);
		blt_info.bltbdat = chipmem_wget_indirect(bltbpt);
		regs.chipset_latch_rw = blt_info.bltbdat;
		record_dma_blit_val(blt_info.bltbdat);
		bltbpt += blt_info.bltbmod;
	}
}
static void blitter_line_read_c(void)
{
	if (bltcon0 & BLTCHC) {
		// C
		record_dma_blit(0x70, 0, bltcpt, last_blitter_hpos);
		blt_info.bltcdat = chipmem_wget_indirect(bltcpt);
		regs.chipset_latch_rw = blt_info.bltcdat;
		record_dma_blit_val(blt_info.bltcdat);
	}
}

static void blitter_line_write(void)
{
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (bltcon0 & BLTCHC) {
		blit_chipmem_agnus_wput(bltdpt, blt_info.bltddat, MW_MASK_BLITTER_D_L);
	}
}

static void blitter_line_minterm(uae_u16 dat)
{
	uae_u16 mask = blt_info.bltafwm;	
	if (dat & BLITTER_PIPELINE_LAST) {
		mask &= blt_info.bltalwm;
	}

	int ashift = bltcon0 >> 12;
	uae_u16 blitahold = (blt_info.bltadat & mask) >> ashift;

	if (bltcon0 & BLTCHB) {
		// B special case if enabled
		int bshift = bltcon1 >> 12;
		blineb = (((uae_u32)blt_info.bltbold << 16) | blt_info.bltbdat) >> bshift;
	}
	blt_info.bltbhold = (blineb & 0x0001) ? 0xFFFF : 0;
	blt_info.bltddat = blit_func(blitahold, blt_info.bltbhold, blt_info.bltcdat, bltcon0 & 0xFF);

	if (blt_info.bltddat) {
		blt_info.blitzero = 0;
	}
}

static void blitter_line_minterm_extra(void)
{
	int ashift = bltcon0 >> 12;
	// never first or last, no masking needed.
	uae_u16 blitahold = blt_info.bltadat >> ashift;

	blt_info.bltddat = blit_func(blitahold, blt_info.bltbhold, blt_info.bltcdat, bltcon0 & 0xFF);
}

static void blitter_line_adat(void)
{
	int ashift = bltcon0 >> 12;

	blt_info.bltaold = (((uae_u32)blt_info.bltaold << 16) | (blt_info.bltadat & blt_info.bltafwm)) >> ashift;
}

static void blitter_line_proc_apt(void)
{
	if (bltcon0 & BLTCHA) {
		bool sign = (bltcon1 & BLTSIGN) != 0;
		if (sign)
			bltapt += (uae_s16)blt_info.bltbmod;
		else
			bltapt += (uae_s16)blt_info.bltamod;
	}
}

static void blitter_line_ovf(void)
{
	uae_u16 ashift = bltcon0 >> 12;
	ashift += blit_ovf;
	ashift &= 15;
	bltcon0 &= 0x0fff;
	bltcon0 |= ashift << 12;
	blit_ovf = 0;
}

static void blitter_line_incx(void)
{
	uae_u16 ashift = bltcon0 >> 12;
	if (ashift == 15) {
		bltcpt += 2;
	}
	blit_ovf = 1;
}

static void blitter_line_decx(void)
{
	uae_u16 ashift = bltcon0 >> 12;
	if (ashift == 0) {
		bltcpt -= 2;
	}
	blit_ovf = -1;
}

static void blitter_line_decy(void)
{
	if (bltcon0 & BLTCHC) {
		bltcpt -= blt_info.bltcmod;
		blitonedot = 0;
	}
}

static void blitter_line_incy(void)
{
	if (bltcon0 & BLTCHC) {
		bltcpt += blt_info.bltcmod;
		blitonedot = 0;
	}
}

static void blitter_line_proc_cpt_y(void)
{
	bool sign = (bltcon1 & BLTSIGN) != 0;

	if (!sign) {
		if (bltcon1 & BLTSUD) {
			if (bltcon1 & BLTSUL)
				blitter_line_decy();
			else
				blitter_line_incy();
		}
	}
	if (!(bltcon1 & BLTSUD)) {
		if (bltcon1 & BLTAUL)
			blitter_line_decy();
		else
			blitter_line_incy();
	}
}

static void blitter_line_proc_cpt_x(void)
{
	bool sign = (bltcon1 & BLTSIGN) != 0;
	
	if (!sign) {
		if (!(bltcon1 & BLTSUD)) {
			if (bltcon1 & BLTSUL)
				blitter_line_decx();
			else
				blitter_line_incx();
		}
	}
	if (bltcon1 & BLTSUD) {
		if (bltcon1 & BLTAUL)
			blitter_line_decx();
		else
			blitter_line_incx();
	}
}

static void blitter_line_proc_status(void)
{
	blitlinepixel = !(bltcon1 & BLTSING) || ((bltcon1 & BLTSING) && !blitonedot);
	blitonedot = 1;
}

static void blitter_line_sign(void)
{
	bool blitsign = (uae_s16)bltapt < 0;
	if (blitsign) {
		bltcon1 |= BLTSIGN;
	} else {
		bltcon1 &= ~BLTSIGN;
	}
}

static void blitter_nxline(void)
{
	int bshift = bltcon1 >> 12;

	bshift--;
	bshift &= 15;

	blineb = (blt_info.bltbdat >> bshift) | (blt_info.bltbdat << (16 - bshift));

	bltcon1 &= 0x0fff;
	bltcon1 |= bshift << 12;

}

static void actually_do_blit (void)
{
	if (blitline) {
		do {
			blitter_line_proc_status();
			blitter_line_proc_apt();
			if (blt_info.hblitsize > 1) {
				blitter_line_read_b();
				blitter_line_read_c();
				blitter_line_minterm(BLITTER_PIPELINE_FIRST);
				blitter_line_proc_cpt_x();
			}
			if (blt_info.hblitsize > 2) {
				if (blitlineloop && !(bltcon1 & BLTSUD)) {
					blitter_line_proc_cpt_y();
					blitlineloop = 0;
				}
				blitter_line_read_c();
				blitter_line_minterm_extra();
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
			bltdpt = bltcpt;
			blitter_line_minterm(BLITTER_PIPELINE_FIRST | BLITTER_PIPELINE_LAST);
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

static void blitter_doit (int hpos)
{
	if (blt_info.vblitsize == 0) {
		blitter_done_all(hpos);
		return;
	}
	actually_do_blit();
	blitter_done_all(hpos);
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
	blitter_doit(-1);
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

	if (con & 2) {
		blitdesc = bltcon1 & BLTDESC;
		if (!savestate_state && blit_warned > 0) {
			if ((bltcon1 & BLTLINE) && !blitline_started) {
				write_log(_T("BLITTER: linedraw enabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			} else if (!(bltcon1 & BLTLINE) && blitline_started) {
				write_log(_T("BLITTER: linedraw disabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			}
			if ((bltcon1 & BLTFILL) && !(bltcon1_old & BLTFILL)) {
				write_log(_T("BLITTER: fill enabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			} else if (!(bltcon1 & BLTFILL) && (bltcon1_old & BLTFILL)) {
				write_log(_T("BLITTER: fill disabled when blitter is active! %08x\n"), M68K_GETPC);
				blit_warned--;
#ifdef DEBUGGER
				if (log_blitter & 16)
					activate_debugger();
#endif
			}
		}
	}

	if (!savestate_state && blt_info.blit_main && (bltcon0_old != bltcon0 || bltcon1_old != bltcon1)) {
		blit_changed = true;
		if (blit_warned > 0) {
			write_log(_T("BLITTER: BLTCON0 %04x -> %04x BLTCON1 %04x -> %04x PC=%08x (%d %d)\n"), bltcon0_old, bltcon0, bltcon1_old, bltcon1, M68K_GETPC, current_hpos(), vpos);
			//blitter_dump();
			blitshifterdebug(bltcon0_old, false);
			blit_warned--;
#ifdef DEBUGGER
			if (log_blitter & 16)
				activate_debugger();
#endif
		}
		bltcon0_old = bltcon0;
		bltcon1_old = bltcon1;
	}

	blit_ch = (bltcon0 & 0x0f00) >> 8;
	blitline = bltcon1 & BLTLINE;
	blit_ovf = (bltcon1 & BLTOVF) != 0;

	blitfill_idle = false;
	shifter_skip_b = (bltcon0 & BLTCHB) == 0;
	if (blitline) {
		shifter_skip_y = true;
		blitfill = 0;
		shifter_out = shifter_skip_y ? shifter[2] : shifter[3];
	} else {
		int oldfill = blitfill;
		blitfill = (bltcon1 & BLTFILL) != 0;
		blitfc = !!(bltcon1 & BLTFC);
		blitife = !!(bltcon1 & BLTIFE);
		if ((bltcon1 & BLTFILL) == BLTFILL) {
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

		shifter_skip_y = (bltcon0 & (BLTCHD | BLTCHC)) != 0x300;
		// fill mode idle cycle needed? (D enabled but C not enabled)
		if (blitfill && (bltcon0 & (BLTCHD | BLTCHC)) == 0x100) {
			shifter_skip_y = false;
			blitfill_idle = true;
		}

		shifter_out = shifter_skip_y ? shifter[2] : shifter[3];
	}

	blit_cyclecount = 4 - (shifter_skip_b + shifter_skip_y);
	blit_dmacount = ((bltcon0 & BLTCHA) ? 1 : 0) + ((bltcon0 & BLTCHB) ? 1 : 0) +
		((bltcon0 & BLTCHC) ? 1 : 0) + (((bltcon0 & BLTCHD) && !blitline) ? 1 : 0);

	if (blt_info.blit_main || blt_info.blit_pending) {
		blitfill_c = blitfill;
		blitline_c = blitline;
	}

	blit_dof = 0;
	if ((bltcon1 & BLTDOFF) && ecs_agnus) {
		blit_dof = 1;
#ifdef DEBUGGER
		debugtest(DEBUGTEST_BLITTER, _T("ECS BLTCON1 DOFF-bit set\n"));
		if (log_blitter & 16)
			activate_debugger();
#endif
	}

	if (blit_changed && blit_warned > 0 && !savestate_state) {
		blitshifterdebug(bltcon0, false);
	}

}

static int get_current_channel(void)
{
	if (blit_cyclecounter < 0) {
		return 0;
	}

	if (blitline) {
		if (shifter[0]) {
			// A or idle
			if (blitter_hcounter + 1 == blt_info.hblitsize)
				return 5;
			if (bltcon0 & BLTCHA)
				return 1;
			return 0;
		}
		// B
		if (shifter[1] && (bltcon0 & BLTCHB)) {
			return 2;
		}
		// C or D
		if (shifter[2]) {
			// enabled C
			if (bltcon0 & BLTCHC) {
				if (blitter_hcounter + 1 == blt_info.hblitsize)
					return 4;
				return 3;
			} else {
				// disabled C
				if (blitter_hcounter + 1 == blt_info.hblitsize)
					return 6;
			}
		}
	} else {
		// order is important when multiple bits in shift register
		// A (if not also D)
		if (shifter[0] && (bltcon0 & BLTCHA) && !shifter[3]) {
			return 1;
		}
		// C
		if (shifter[2] && (bltcon0 & BLTCHC)) {
			return 3;
		}
		// Shift stage 4 active, C enabled and other stage(s) also active:
		// normally would be D but becomes C.
		if (shifter[3] && (bltcon0 & BLTCHC) && (shifter[0] || shifter[1])) {
			return 3;
		}
		// B
		if (shifter[1] && (bltcon0 & BLTCHB)) {
			return 2;
		}
		// D is disabled if position A is non-zero, even if A is disabled.
		if (shifter[0]) {
			return 0;
		}
		if (shifter_first >= 0) {
			return 0;
		}
		// D only if A, B and C is not currently active
		if (ddatuse) {
			// idle fill cycle: 3 = D, 4 = idle
			if (blitfill_idle) {
				// if stage 4: idle
				if (shifter[3]) {
					return 0;
				}
				// if stage 3: D
				if (shifter[2] && (bltcon0 & BLTCHD)) {
					return 4;
				}
			} else {
				// if stage 3 and C disabled and D enabled: D
				if (shifter[2] && !(bltcon0 & BLTCHC) && (bltcon0 & BLTCHD)) {
					return 4;
				}
				// if stage 4 and C enabled and D enabled: D
				if (shifter[3] && (bltcon0 & BLTCHC) && (bltcon0 & BLTCHD)) {
					return 4;
				}
			}
		}
	}
	return 0;
}

static uae_u16 blitter_doblit(uae_u16 dat)
{
	uae_u32 blitahold;
	uae_u16 bltadat, ddat;
	uae_u8 mt = bltcon0 & 0xFF;
	uae_u16 ashift = bltcon0 >> 12;

	bltadat = blt_info.bltadat;
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

	ddat = blit_func (blitahold, blt_info.bltbhold, blt_info.bltcdat, mt) & 0xFFFF;

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

	return ddat;
}

static int blitter_next_cycle(void)
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

	if (shifter_out) {
		dodat = true;
		if (!blitline) {
			if (bltcon0 & BLTCHD) {
				ddatuse = true;
			}
		}
		blitter_hcounter++;
		if (blitter_hcounter >= blt_info.hblitsize) {
			blitter_hcounter = 0;
			blitter_vcounter++;
			if (blitter_vcounter >= blt_info.vblitsize) {
				shifter_out = false;
				blit_cyclecounter = CYCLECOUNT_FINISHED;
				if (!blitline) {
					// do we need final D write?
					if (ddatuse && (bltcon0 & BLTCHD)) {
						if (blt_info.blit_finald) {
							write_log(_T("blit finald already set!?\n"));
						}
						blt_info.blit_finald = 1 + 2;
						blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
					}
				}
			}
		}
		shifter[0] = shifter_out;
	}

	if (shifter_first > 0) {
		shifter_first = -1;
		shifter[0] = true;
		blitfc = !!(bltcon1 & BLTFC);
	} else {
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
	}

	shifter_out = out;

	if (blit_cyclecounter > 0 && blitchanged) {
		static int blit_warned = 100;
		if (blit_warned > 0) {
			if (blitshifterdebug(bltcon0, true)) {
				blit_faulty = 1;
			}
			blit_warned--;
		}
	}
	return dodat;
}


static void blitter_doddma_new(int hpos, bool addmod)
{
	uaecptr *hpt = NULL;
	bool skip = false;

	check_channel_mods(hpos, 4, &bltdpt);
	bltdpt |= alloc_cycle_blitter_conflict_or(hpos, 4, &skip);
	// alloc_cycle_blitter() can change modulo, old modulo is used during this cycle.
	int mod = blit_modaddd;
	if (!skip) {
		record_dma_blit(0x00, ddat, bltdpt, hpos);
		blit_chipmem_agnus_wput(bltdpt, ddat, MW_MASK_BLITTER_D_N);
	}
	bool skipadd = alloc_cycle_blitter(hpos, &bltdpt, 4, addmod ? mod : 0);

	if (!blitline && !skipadd) {
		bltdpt += blit_add;
	}
	if (addmod) {
		bltdpt += mod;
	}
}

static void blitter_dodma_new(int ch, int hpos, bool addmod)
{
	uae_u16 dat;
	uae_u32 *addr;
	bool skipadd = false;
	int mod;
	bool skip = false;

	uaecptr orptr = alloc_cycle_blitter_conflict_or(hpos, ch, &skip);

	switch (ch)
	{
	case 1: // A
	{
		bltapt |= orptr;
		check_channel_mods(hpos, 1, &bltapt);
		if (!skip) {
			uae_u16 reg = 0x74;
			record_dma_blit(reg, 0, bltapt, hpos);
			blt_info.bltadat = dat = chipmem_wget_indirect(bltapt);
			record_dma_blit_val(dat);
			regs.chipset_latch_rw = blt_info.bltadat;
		}
		addr = &bltapt;
		mod = blit_modadda;
		skipadd = alloc_cycle_blitter(hpos, &bltapt, 1, addmod ? mod : 0);
		break;
	}
	case 2: // B
	{
		int bshift = bltcon1 >> 12;
		bltbpt |= orptr;
		check_channel_mods(hpos, 2, &bltbpt);
		if (!skip) {
			uae_u16 reg = 0x72;
			record_dma_blit(reg, 0, bltbpt, hpos);
			blt_info.bltbdat = dat = chipmem_wget_indirect(bltbpt);
			record_dma_blit_val(dat);
			regs.chipset_latch_rw = blt_info.bltbdat;
		}
		addr = &bltbpt;
		mod = blit_modaddb;
		if (blitdesc)
			blt_info.bltbhold = (((uae_u32)blt_info.bltbdat << 16) | blt_info.bltbold) >> (16 - bshift);
		else
			blt_info.bltbhold = (((uae_u32)blt_info.bltbold << 16) | blt_info.bltbdat) >> bshift;
		blineb = blt_info.bltbhold;
		blt_info.bltbold = blt_info.bltbdat;
		skipadd = alloc_cycle_blitter(hpos, &bltbpt, 2, addmod ? mod : 0);
		break;
	}
	case 3: // C
	{
		bltcpt |= orptr;
		check_channel_mods(hpos, 3, &bltcpt);
		if (!skip) {
			uae_u16 reg = 0x70;
			record_dma_blit(reg, 0, bltcpt, hpos);
			blt_info.bltcdat = dat = chipmem_wget_indirect(bltcpt);
			record_dma_blit_val(dat);
			regs.chipset_latch_rw = blt_info.bltcdat;
		}
		addr = &bltcpt;
		mod = blit_modaddc;
		skipadd = alloc_cycle_blitter(hpos, &bltcpt, 3, addmod ? mod : 0);
		break;
	}
	default:
		abort();
	}

	if (!blitline && !skipadd) {
		(*addr) += blit_add;
	}
	if (addmod) {
		(*addr) += mod;
	}
}


static bool blitter_idle_cycle_register_write(uaecptr addr, uae_u32 v)
{
	addrbank *ab = &get_mem_bank(addr);
	if (ab != &custom_bank)
		return false;
	addr &= 0x1fe;
	if (v == 0xffffffff) {
		v = regs.chipset_latch_rw;
	}
	if (addr == 0x40) {
		bltcon0 = v;
		blit_bltset(1);
		return true;
	} else if (addr == 0x42) {
		bltcon1 = v;
		blit_bltset(2);
		return true;
	}
	return false;
}

static bool decide_blitter_idle(int lasthpos, int hpos, uaecptr addr, uae_u32 value)
{
	markidlecycle(last_blitter_hpos);
	if (addr != 0xffffffff && lasthpos + 1 == hpos) {
		shifter_skip_b_old = shifter_skip_b;
		shifter_skip_y_old = shifter_skip_y;
		return blitter_idle_cycle_register_write(addr, value);
	}
	return false;
}

uae_u16 blitter_pipe[MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST + MAX_CHIPSETSLOTS_EXTRA];

void set_blitter_last(int hp)
{
	last_blitter_hpos = hp;
}

static bool decide_blitter_maybe_write2(int until_hpos, uaecptr addr, uae_u32 value)
{
	bool written = false;
	int hsync = until_hpos < 0;

	if (scandoubled_line) {
		return 0;
	}

	if (hsync && blt_delayed_irq) {
		if (blt_delayed_irq > 0)
			blt_delayed_irq--;
		if (blt_delayed_irq <= 0) {
			blt_delayed_irq = 0;
			INTREQ_INT(6, 3);
		}
	}

	if (until_hpos < 0 || until_hpos > maxhposm0) {
		until_hpos = maxhposm0;
	}

	if (last_blitter_hpos >= until_hpos) {
		goto end;
	}

	if (immediate_blits) {
		if (!blt_info.blit_main) {
			return false;
		}
		if (dmaen(DMA_BLITTER)) {
			blitter_doit(last_blitter_hpos);
		}
		goto end2;
	}

#if BLITTER_DEBUG
	if (log_blitter && blitter_delayed_debug) {
		blitter_delayed_debug = 0;
		blitter_dump();
	}
#endif

	if (!blitter_cycle_exact) {
		goto end2;
	}

	while (last_blitter_hpos < until_hpos) {
		int hpos = last_blitter_hpos;

		// dma transfers and processing
		for (;;) {
			if (!cycle_line_slot[hpos] && blt_info.blit_queued > 0) {
				blt_info.blit_queued--;
				if (!blt_info.blit_queued && !blt_info.blit_finald) {
					blitter_end();
				}
			}
			uae_u16 dat = blitter_pipe[hpos];
			if (dat) {
				blitter_pipe[hpos] = 0;
			}
			if (!(dat & CYCLE_PIPE_BLITTER)) {
				break;
			}
			cycle_line_pipe[hpos] = 0;

			if (dat & CYCLE_PIPE_CPUSTEAL) {
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_CPUBLITTERSTOLEN, hpos, vpos);
				}
#endif
				break;
			}

			int c = dat & 7;
			bool line = (dat & BLITTER_PIPELINE_LINE) != 0;

			// last D write?
			if (dat & BLITTER_PIPELINE_LASTD) {
				line = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_BLITFINALD, hpos, vpos);
				}
#endif
				if (cycle_line_slot[hpos]) {
					write_log("Blitter cycle allocated by %d!?\n", cycle_line_slot[hpos]);
				}
				if (!blt_info.blit_main && !blt_info.blit_finald && !blt_info.blit_queued) {
					blitter_end();
				}
				//activate_debugger();
			}

			// finished?
			if (dat & BLITTER_PIPELINE_FINISHED) {
				if (line) {
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event(DMA_EVENT_BLITFINALD, hpos, vpos);
						//activate_debugger();
					}
#endif
				}
			}

			bool addmod = (dat & BLITTER_PIPELINE_ADDMOD) != 0;

			blit_totalcyclecounter++;

			if (c == 0) {

				written = decide_blitter_idle(hpos, until_hpos, addr, value);

			} else if (c == 1 && line) { // line 1/4 (A, free)

				written = decide_blitter_idle(hpos, until_hpos, addr, value);
				if (dat & BLITTER_PIPELINE_FIRST) {
					blitter_line_proc_status();
					blitter_line_proc_apt();
				}

			} else if (c == 3 && line) { // line 2/4 (C)

				if (!(dat & BLITTER_PIPELINE_FIRST) && blitlineloop && !(bltcon1 & BLTSUD)) {
					blitter_line_proc_cpt_y();
					blitlineloop = 0;
				}
				bool skip = false;
				uaecptr orptr = alloc_cycle_blitter_conflict_or(hpos, 3, &skip);
				record_dma_blit(0x70, 0, bltcpt | orptr, hpos);
				check_channel_mods(hpos, 3, &bltcpt);
				if (!skip) {
					blt_info.bltcdat = chipmem_wget_indirect(bltcpt | orptr);
					regs.chipset_latch_rw = blt_info.bltcdat;
					record_dma_blit_val(blt_info.bltcdat);
				}
				alloc_cycle_blitter(hpos, &bltcpt, 3, 0);

				if (dat & BLITTER_PIPELINE_FIRST) {
					blitter_line_minterm(dat);
					blitter_line_proc_cpt_x();
				} else {
					// W>2 special case
					blitter_line_minterm_extra();
				}

			} else if (c == 5 && line) { // line 3/4 (free)

				written = decide_blitter_idle(hpos, until_hpos, addr, value);

			} else if ((c == 4 || c == 6) && line) { // line 4/4 (C/D)

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

				/* onedot mode and no pixel = bus write access is skipped */
				if (blitlinepixel && c == 4) {
					bool skip = false;
					uaecptr orptr = alloc_cycle_blitter_conflict_or(hpos, 4, &skip);
					record_dma_blit(0x00, blt_info.bltddat, bltdpt | orptr, hpos);
					check_channel_mods(hpos, 4, &bltdpt);
					if (!skip) {
						blit_chipmem_agnus_wput(bltdpt | orptr, blt_info.bltddat, MW_MASK_BLITTER_D_L);
					}
					alloc_cycle_blitter(hpos, &bltdpt, 4, 0);
				} else {
					markidlecycle(hpos);
				}
				blitlinepixel = 0;
				blitlineloop = 1;

				bltdpt = bltcpt;

				blitter_line_minterm(dat);

			} else {

				// normal mode channels
				if (c == 4) {
					blitter_doddma_new(hpos, addmod);
				} else {
					if (c == 5) {
						c = 1;
					}
					blitter_dodma_new(c, hpos, addmod);
				}
			}

			if ((dat & BLITTER_PIPELINE_BLIT) && !blitline) {
				ddat = blitter_doblit(dat);
				blt_info.bltddat = ddat;
			}

			if (dat & BLITTER_PIPELINE_PROCESS) {
				blitfc = !!(bltcon1 & BLTFC);
			}
			break;
		}

		if (blt_info.blit_finald || blt_info.blit_main) {

			blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;

			// cycle allocations
			for (;;) {

				// final D idle cycle
				// does not need free bus
				if (blt_info.blit_finald > 1) {
					blt_info.blit_finald--;
				}

				// Skip BLTSIZE write cycle
				if (blit_waitcyclecounter) {
					blit_waitcyclecounter = 0;
					break;
				}

				bool cant = blitter_cant_access(hpos);
				if (cant) {
					blit_misscyclecounter++;
					break;
				}

				// CPU steals the cycle if CPU has waited long enough and current cyle is not free.
				if (!(dmacon & DMA_BLITPRI) && blt_info.nasty_cnt >= BLIT_NASTY_CPU_STEAL_CYCLE_COUNT && currprefs.cpu_memory_cycle_exact && ((cycle_line_slot[hpos] & CYCLE_MASK) != 0 || bitplane_dma_access(hpos, 0) != 0)) {
					int offset = get_rga_pipeline(hpos, RGA_PIPELINE_OFFSET_BLITTER);
					blitter_pipe[offset] = CYCLE_PIPE_BLITTER | CYCLE_PIPE_CPUSTEAL;
					cycle_line_pipe[offset] = CYCLE_PIPE_BLITTER | CYCLE_PIPE_CPUSTEAL;
					blt_info.nasty_cnt = -1;
					break;
				}

				if (blt_info.blit_finald == 1) {
					// final D write. Only if BLTCON D and line mode is off.
					if ((bltcon0 & BLTCHD) && !(bltcon1 & BLTLINE)) {
						int offset = get_rga_pipeline(hpos, RGA_PIPELINE_OFFSET_BLITTER);
						cycle_line_pipe[offset] = CYCLE_PIPE_BLITTER;
						blitter_pipe[offset] = CYCLE_PIPE_BLITTER | 4 | BLITTER_PIPELINE_ADDMOD | BLITTER_PIPELINE_LASTD;
					}
					if (currprefs.chipset_mask & CSMASK_AGA) {
						blitter_done_all(hpos);
					}
					blt_info.blit_finald = 0;
					if (!blt_info.blit_queued) {
						blitter_end();
					}
					break;
				}

				if (blt_info.blit_main) {
					blit_cyclecounter++;
					if (blit_cyclecounter == 0) {
						shifter_first = 1;
						// clear possible still pending final D write
						blt_info.blit_finald = 0;
						blitter_next_cycle();
					}
					int c = get_current_channel();
					blt_info.got_cycle = 1;

					bool addmod = false;
					if (c == 1 || c == 2 || c == 3) {
						if (blitter_hcounter + 1 == blt_info.hblitsize) {
							addmod = true;
						}
					} else if (c == 4) {
						if (blitter_hcounter == 0) {
							addmod = true;
						}
					}

					uae_u16 v = CYCLE_PIPE_BLITTER | c | (addmod ? BLITTER_PIPELINE_ADDMOD : 0);

					if (blitter_hcounter == 0) {
						v |= BLITTER_PIPELINE_FIRST;
					}
					if (blitter_hcounter == blt_info.hblitsize - 1) {
						v |= BLITTER_PIPELINE_LAST;
					}
					if (blitline) {
						v |= BLITTER_PIPELINE_LINE;
					}

					bool doddat = false;
					if (blit_cyclecounter >= 0) {
						doddat = blitter_next_cycle();
					}

					int offset = get_rga_pipeline(hpos, RGA_PIPELINE_OFFSET_BLITTER);
					if (cycle_line_pipe[offset]) {
						write_log("Blitter cycle conflict %d\n", cycle_line_pipe[offset]);
					}

					if (doddat) {
						v |= BLITTER_PIPELINE_BLIT;
						if (blitter_hcounter == 0) {
							v |= BLITTER_PIPELINE_PROCESS;
						}
					}

					// finished?
					if (blit_cyclecounter < -CYCLECOUNT_START) {
						v |= BLITTER_PIPELINE_FINISHED;
						if (!blt_info.blit_main) {
							write_log(_T("blitter blit_main already cleared!?\n"));
						}
						// has final D write?
						if (blt_info.blit_finald) {
							if (!(currprefs.chipset_mask & CSMASK_AGA)) {
								blitter_done_except_d(hpos);
							}
						} else {
							blitter_done_all(hpos);
						}
					}

					cycle_line_pipe[offset] = CYCLE_PIPE_BLITTER;
					blitter_pipe[offset] = v;
				}
				break;
			}

		}

		last_blitter_hpos++;
		bltptxpos = -1;
	}
end2:
	bltptxpos = -1;
end:
	if (hsync) {
		last_blitter_hpos = 0;
	}

	return written;
}

bool decide_blitter_maybe_write(int until_hpos, uaecptr addr, uae_u32 value)
{
	int reg = addr & 0x1fe;
	// early exit check
	if (reg != 0x40 && reg != 0x42) {
		addr = 0xffffffff;
	} else {
		return decide_blitter_maybe_write2(until_hpos, addr, value);
	}
	return decide_blitter_maybe_write2(until_hpos, addr, value);
}

void decide_blitter(int until_hpos)
{
	decide_blitter_maybe_write2(until_hpos, 0xffffffff, 0xffffffff);
}


#else
void decide_blitter (int hpos) { }
#endif

static void blitter_force_finish(bool state)
{
	uae_u16 odmacon;
	if (!blt_info.blit_main && !blt_info.blit_finald)
		return;
	/* blitter is currently running
	* force finish (no blitter state support yet)
	*/
	odmacon = dmacon;
	dmacon |= DMA_MASTER | DMA_BLITTER;
	if (state)
		write_log(_T("forcing blitter finish\n"));
	if (blitter_cycle_exact && !immediate_blits) {
		int rounds = 10000;
		while ((blt_info.blit_main || blt_info.blit_finald) && rounds > 0) {
			memset(cycle_line_slot, 0, sizeof(cycle_line_slot));
			decide_blitter(-1);
			rounds--;
		}
		if (rounds == 0)
			write_log(_T("blitter froze!?\n"));
	} else {
		actually_do_blit();
	}
	blitter_done_all(-1);
	dmacon = odmacon;
}

static void blit_modset (void)
{
	int mult;

	blit_add = blitdesc ? -2 : 2;
	mult = blitdesc ? -1 : 1;
	blit_modadda = mult * blt_info.bltamod;
	blit_modaddb = mult * blt_info.bltbmod;
	blit_modaddc = mult * blt_info.bltcmod;
	blit_modaddd = mult * blt_info.bltdmod;
}

void reset_blit (int bltcon)
{
	if (!blt_info.blit_main && !blt_info.blit_finald)
		return;
	if (bltcon)
		blit_bltset (bltcon);
	blit_modset();
}

static bool waitingblits (void)
{
	static int warned = 10;

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
	while ((blt_info.blit_main || blt_info.blit_finald) && dmaen (DMA_BLITTER)) {
		waited = true;
		x_do_cycles (8 * CYCLE_UNIT);
		if (vpos_prev != vpos) {
			vpos_prev = vpos;
			waiting++;
			if (waiting > maxvpos * 5) {
				break;
			}
		}
		if (blitter_cycle_exact && blit_cyclecounter > 0 && !shifter[0] && !shifter[1] && !shifter[2] && !shifter[3]) {
			break;
		}
	}
	if (warned && waited) {
		warned--;
		write_log (_T("waiting_blits detected PC=%08x\n"), M68K_GETPC);
	}
	if (!blt_info.blit_main && !blt_info.blit_finald)
		return true;
	return false;
}

static void blitter_start_init (void)
{
	shifter_first = 0;
	blt_info.blit_queued = 0;
	blit_faulty = 0;
	blt_info.blitzero = 1;
	blitline_started = bltcon1 & BLTLINE;
	blitlineloop = 1;

	blit_bltset(1 | 2);
	blit_modset();
	ddatuse = 0;
	blt_info.blit_interrupt = 0;

	blt_info.bltaold = 0;
	blt_info.bltbold = 0;

	int bshift = bltcon1 >> 12;
	blineb = (blt_info.bltbdat >> bshift) | (blt_info.bltbdat << (16 - bshift));
	blitonedot = 0;
	blitlinepixel = 0;

	if (!(dmacon & DMA_BLITPRI) && blt_info.nasty_cnt >= BLIT_NASTY_CPU_STEAL_CYCLE_COUNT) {
		blt_info.wait_nasty = 1;
	} else {
		blt_info.wait_nasty = 0;
	}
}

void do_blitter(int hpos, int copper, uaecptr pc)
{
	int cycles;

#if BLITTER_DEBUG
	if ((log_blitter & 2)) {
		if (blt_info.blit_main) {
			write_log (_T("blitter was already active! PC=%08x\n"), M68K_GETPC);
		}
	}
#endif

	bltcon0_old = bltcon0;
	bltcon1_old = bltcon1;

	if (blitter_cycle_exact != (currprefs.blitter_cycle_exact ? 1 : 0)) {
		memset(blitter_pipe, 0, sizeof(blitter_pipe));
	}
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
	blit_waitcyclecounter = 0;
	last_blitter_hpos = hpos;

	if (blitter_cycle_exact) {
		if (immediate_blits) {
			if (dmaen(DMA_BLITTER)) {
				blitter_doit(-1);
			}
			return;
		}
		if (log_blitter & 8) {
			blitter_handler (0);
		} else {
			blitter_hcounter = 0;
			blitter_vcounter = 0;
			blit_cyclecounter = -CYCLECOUNT_START;
			blit_waitcyclecounter = 1;
			blit_maxcyclecounter = blt_info.hblitsize * blt_info.vblitsize + 2;
			blt_info.blit_pending = 0;
			blt_info.blit_main = 1;
			blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
		}
		return;
	}

	if (blt_info.vblitsize == 0) {
		if (dmaen(DMA_BLITTER)) {
			blitter_done_all(-1);
		}
		return;
	}

	if (dmaen (DMA_BLITTER)) {
		blt_info.got_cycle = 1;
	}

	if (immediate_blits) {
		if (dmaen(DMA_BLITTER)) {
			blitter_doit(-1);
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
			blitter_doit(-1);
		}
	}
}

void maybe_blit (int hpos, int hack)
{
	static int warned = 10;

	if (!blt_info.blit_main) {
		decide_blitter(hpos);
		return;
	}

	if (savestate_state)
		return;

	if (dmaen (DMA_BLITTER) && (currprefs.cpu_model >= 68020 || !currprefs.cpu_memory_cycle_exact)) {
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
		decide_blitter(hpos);
		goto end;
	}

	if (hack == 1 && get_cycles() < blit_firstline_cycles)
		goto end;

	blitter_handler(0);
end:;
#if BLITTER_DEBUG
	if (log_blitter)
		blitter_delayed_debug = 1;
#endif
}

void check_is_blit_dangerous (uaecptr *bplpt, int planes, int words)
{
	blt_info.blitter_dangerous_bpl = 0;
	if ((!blt_info.blit_main && !blt_info.blit_finald) || !blitter_cycle_exact)
		return;
	// too simple but better than nothing
	for (int i = 0; i < planes; i++) {
		uaecptr bpl = bplpt[i];
		uaecptr dpt = bltdpt & chipmem_bank.mask;
		if (dpt >= bpl - 2 * words && dpt < bpl + 2 * words) {
			blt_info.blitter_dangerous_bpl = 1;
			return;
		}
	}
}

int blitnasty (void)
{
	int cycles, ccnt;
	if (!blt_info.blit_main)
		return -1;
	if (!dmaen(DMA_BLITTER))
		return -1;
	if (blitter_cycle_exact) {
		return -1;
	}
	if (blit_last_cycle >= blit_cyclecount && blit_dmacount == blit_cyclecount)
		return 0;
	cycles = int((get_cycles() - blit_first_cycle) / CYCLE_UNIT);
	ccnt = 0;
	while (blit_last_cycle + blit_cyclecount < cycles) {
		ccnt += blit_dmacount;
		blit_last_cycle += blit_cyclecount;
	}
	return ccnt;
}

/* very approximate emulation of blitter slowdown caused by bitplane DMA */
void blitter_slowdown (int ddfstrt, int ddfstop, int totalcycles, int freecycles)
{
	static int oddfstrt, oddfstop, ototal, ofree;
	static int slow;

	if (!totalcycles || ddfstrt < 0 || ddfstop < 0)
		return;
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
	if (blit_slowdown < 0 || blitline)
		return;
	blit_slowdown += slow;
	blit_misscyclecounter += slow;
}

void blitter_reset (void)
{
	bltptxpos = -1;
	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	immediate_blits = currprefs.immediate_blits;
	shifter[0] = shifter[1] = shifter[2] = shifter[3] = 0;
	shifter_skip_b = false;
	shifter_skip_y = false;
	bltcon0 = 0;
	bltcon1 = 0;
	blitter_start_init();
	blt_info.blit_main = 0;
	blt_info.blit_pending = 0;
	blt_info.blit_finald = 0;
	blt_info.blit_queued = 0;
}

#ifdef SAVESTATE

void restore_blitter_start(void)
{
	blitter_reset();
}

void restore_blitter_finish (void)
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
			if (intreq & 0x0040)
				blt_delayed_irq = 3;
			intreq &= ~0x0040;
		}
	} else {
		last_blitter_hpos = 0;
		blit_modset();
	}
}

uae_u8 *restore_blitter (uae_u8 *src)
{
	uae_u32 flags = restore_u32();

	if (!(flags & 8)) {
		blt_statefile_type = 0;
		blt_delayed_irq = 0;
		blt_info.blit_pending = 0;
		blt_info.blit_finald = 0;
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
	bltcon0_old = bltcon0;
	bltcon1_old = bltcon1;
	return src;
}

uae_u8 *save_blitter (size_t *len, uae_u8 *dstptr, bool newstate)
{
	uae_u8 *dstbak,*dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 16);
	if (blt_info.blit_main || blt_info.blit_finald) {
		save_u32(8);
	} else {
		save_u32(1 | 4);
	}
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
	blit_waitcyclecounter = restore_u32();
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
	blt_info.blit_finald = restore_u8();
	blitfc = restore_u8();
	blitife = restore_u8();

	restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();

	ddatuse = restore_u8();
	restore_u8();
	ddat = restore_u16();
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
	blit_faulty = restore_u8();
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
		blt_info.blit_finald = 0;
	} else {
		shifter[0] = (tmp & 1) != 0;
		shifter[1] = (tmp & 2) != 0;
		shifter[2] = (tmp & 4) != 0;
		shifter[3] = (tmp & 8) != 0;
		shifter_skip_b_old = (tmp & 16) != 0;
		shifter_skip_y_old = (tmp & 32) != 0;
		blt_info.blit_finald = restore_u8();
		blit_ovf = restore_u8();
	}

	blt_info.blit_main = 0;
	blt_info.blit_pending = 0;

	if (!blitter_cycle_exact) {
		if (state > 0)
			do_blitter(0, 0, 0);
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
			if (blt_info.blit_finald) {
				blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
				blt_info.blit_main = 0;
			}
			if (blt_statefile_type == 2) {
				blit_bltset(0);
			}
		}
		shifter_skip_b_old = shifter_skip_b;
		shifter_skip_y_old = shifter_skip_y;
	}

	blit_first_cycle |= ((uae_u64)restore_u32()) << 32;
	blit_firstline_cycles |= ((uae_u64)restore_u32()) << 32;

	blt_info.bltaold = restore_u16();
	blt_info.bltbold = restore_u16();

	blt_info.nasty_cnt = restore_u8();
	blt_info.wait_nasty = restore_u8();

	shifter_first = (uae_s8)restore_u8();
	blt_info.finishhpos = restore_u8();

	blit_cyclecounter = restore_u32();

	for (int i = 0; i < 4; i++) {
		blitter_pipe[i] = restore_u16();
		if (blitter_pipe[i]) {
			blt_info.blit_queued = BLITTER_MAX_PIPELINED_CYCLES;
		}
		cycle_line_pipe[i] = restore_u16();
		cycle_line_slot[i] = restore_u8();
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
	if (!blt_info.blit_main && !blt_info.blit_finald) {
		state = 0;
	} else if (blt_info.blit_pending) {
		state = 1;
	} else if (blt_info.blit_finald) {
		state = 3;
	} else {
		state = 2;
	}
	save_u8(state);

	if (blt_info.blit_main || blt_info.blit_finald) {
		write_log(_T("BLITTER active while saving state\n"));
#if BLITTER_DEBUG
		if (log_blitter)
			blitter_dump();
#endif
	}

	save_u32((uae_u32)blit_first_cycle);
	save_u32(blit_last_cycle);
	save_u32(blit_waitcyclecounter);
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
	save_u8(blt_info.blit_finald);
	save_u8(blitfc);
	save_u8(blitife);

	save_u8(bltcon1 >> 12);
	save_u8((16 - (bltcon1 >> 12)));
	save_u8(bltcon0 >> 12);
	save_u8(16 - (bltcon0 >> 12));

	save_u8(ddatuse);
	save_u8(0); //(ddat2use);
	save_u16(ddat);
	save_u16(0); //(ddat2);

	save_u8(blitline);
	save_u8(blitfill);
	save_u16(blt_info.bltadat);
	save_u16(blineb);
	save_u8(bltcon0 >> 12);
	save_u8(blitonedot);
	save_u8(blitlinepixel);
	save_u8((bltcon1 & BLTSING) != 0);
	save_u8((blitlinepixel ? 1 : 0) | (blitlineloop ? 2 : 0));
	save_u8(blt_info.blit_interrupt);
	save_u8(blt_delayed_irq);
	save_u8(blt_info.blitzero);
	save_u8(blt_info.got_cycle);
	
	save_u8(0); //(blit_frozen);
	save_u8(blit_faulty);
	save_u8(0); //original_ch);
	save_u8(0); //original_fill);
	save_u8(0); //original_line);
	save_u8(0); //get_cycle_diagram_type (blit_diag));

	save_u16(0x1234);

	save_u8(blt_info.blitter_nasty);
	save_u8((shifter[0] ? 1 : 0) | (shifter[1] ? 2 : 0) | (shifter[2] ? 4 : 0) | (shifter[3] ? 8 : 0) |
		(shifter_skip_b_old ? 16 : 0) | (shifter_skip_y_old ? 32 : 0));
	save_u8(blt_info.blit_finald);
	save_u8(blit_ovf);

	save_u32(blit_first_cycle >> 32);
	save_u32(blit_firstline_cycles >> 32);

	save_u16(blt_info.bltaold);
	save_u16(blt_info.bltbold);

	save_u8(blt_info.nasty_cnt);
	save_u8(blt_info.wait_nasty);

	save_u8(shifter_first);
	save_u8(blt_info.finishhpos);

	save_u32(blit_cyclecounter);

	for (int i = 0; i < 4; i++) {
		save_u16(blitter_pipe[i]);
		save_u16(cycle_line_pipe[i]);
		save_u8(cycle_line_slot[i]);
	}

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */
