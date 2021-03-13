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
//#include "debug.h"

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

/* we must not change ce-mode while blitter is running.. */
static int blitter_cycle_exact, immediate_blits;
static int blt_statefile_type;

uae_u16 bltcon0, bltcon1;
uae_u32 bltapt, bltbpt, bltcpt, bltdpt;
uae_u32 bltptx;
int bltptxpos, bltptxc;

static int blinea_shift;
static uae_u16 blinea, blineb;
static int blitline, blitfc, blitfill, blitife, blitsing, blitdesc, blit_ovf;
static int blitline_started;
static int blitonedot, blitsign, blitlinepixel;
static int blit_add;
static int blit_modadda, blit_modaddb, blit_modaddc, blit_modaddd;
static int blit_ch;
static bool shifter_skip_b, shifter_skip_y;
static bool shifter_skip_b_old, shifter_skip_y_old;
static uae_u16 bltcon0_old, bltcon1_old;
static bool shifter[4], shifter_out, shifter_first;

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
extern uae_u8 cycle_line[256];
static int blitter_cyclecounter;
static int blitter_hcounter;
static int blitter_vcounter;
#endif

static long blit_firstline_cycles;
static long blit_first_cycle;
static int blit_last_cycle, blit_dmacount, blit_cyclecount;
static int blit_linecycles, blit_extracycles;
static int blit_faulty;
static int blt_delayed_irq;
static uae_u16 ddat1;
static int ddat1use;

static int last_blitter_hpos;

static uae_u16 debug_bltcon0, debug_bltcon1;
static uae_u32 debug_bltapt, debug_bltbpt, debug_bltcpt, debug_bltdpt;
static uae_u16 debug_bltamod, debug_bltbmod, debug_bltcmod, debug_bltdmod;
static uae_u32 debug_bltafwm, debug_bltalwm;
static uae_u32 debug_bltpc;
static int debug_bltcop;
static uae_u16 debug_bltsizev, debug_bltsizeh;
static uae_u16 debug_bltadat, debug_bltbdat, debug_bltcdat;

#define BLITTER_STARTUP_CYCLES 2

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
	if (debug_dma) {
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
	if (debug_dma) {
		if (reg == 0) {
			record_dma_write(reg, v, addr, hpos, vpos, DMARECORD_BLITTER, 3 + (blitline ? 0x20 : (blitfill ? 0x10 : 0)));
		} else {
			int r = 0;
			if (reg == 0x70)
				r = 2;
			if (reg == 0x72)
				r = 1;
			if (reg == 0x74)
				r = 0;
			record_dma_read(reg, addr, hpos, vpos, DMARECORD_BLITTER, r + (blitline ? 0x20 : (blitfill ? 0x10 : 0)));
		}
	}
	if (memwatch_enabled) {
		if (reg == 0) {
			uae_u32 mask = MW_MASK_BLITTER_D_N;
			if (blitfill)
				mask = MW_MASK_BLITTER_D_F;
			if (blitline)
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
	//int chipsize = currprefs.chipmem.size;
	//console_out_f(_T("PT A=%08X B=%08X C=%08X D=%08X\n"), bltapt, bltbpt, bltcpt, bltdpt);
	//console_out_f(_T("CON0=%04X CON1=%04X DAT A=%04X B=%04X C=%04X\n"),
	//	bltcon0, bltcon1, blt_info.bltadat, blt_info.bltbdat, blt_info.bltcdat);
	//console_out_f(_T("AFWM=%04X ALWM=%04X MOD A=%04X B=%04X C=%04X D=%04X\n"),
	//	blt_info.bltafwm, blt_info.bltalwm,
	//	blt_info.bltamod & 0xffff, blt_info.bltbmod & 0xffff, blt_info.bltcmod & 0xffff, blt_info.bltdmod & 0xffff);
	//console_out_f(_T("PC=%08X DMA=%d\n"), m68k_getpc(), dmaen (DMA_BLITTER));

	//if (((bltcon0 & 0x800) && bltapt >= chipsize) || ((bltcon0 & 0x400) && bltbpt >= chipsize) ||
	//	((bltcon0 & 0x200) && bltcpt >= chipsize) || ((bltcon0 & 0x100) && bltdpt >= chipsize))
	//	console_out_f(_T("PT outside of chipram\n"));
}

void blitter_debugdump(void)
{
	//console_out(_T("Blitter registers at start:\n"));
	//console_out_f(_T("PT A=%08X B=%08X C=%08X D=%08X\n"), debug_bltapt, debug_bltbpt, debug_bltcpt, debug_bltdpt);
	//console_out_f(_T("CON0=%04X CON1=%04X DAT A=%04X B=%04X C=%04X\n"),
	//	debug_bltcon0, debug_bltcon1, debug_bltadat, debug_bltbdat, debug_bltcdat);
	//console_out_f(_T("AFWM=%04X ALWM=%04X MOD A=%04X B=%04X C=%04X D=%04X\n"),
	//	debug_bltafwm, debug_bltalwm, debug_bltamod, debug_bltbmod, debug_bltcmod, debug_bltdmod);
	//console_out_f(_T("COP=%d PC=%08X\n"), debug_bltcop, debug_bltpc);
	//console_out(_T("Blitter registers now:\n"));
	//blitter_dump();
}

static int canblit (int hpos)
{
	if (!dmaen (DMA_BLITTER))
		return -1;
	if (is_bitplane_dma (hpos))
		return 0;
	if (cycle_line[hpos] & CYCLE_MASK) {
		return 0;
	}
	return 1;
}

static void markidlecycle (int hpos)
{
	//if (debug_dma)
		//record_dma_event (DMA_EVENT_BLITSTARTFINISH, hpos, vpos);
}

static void reset_channel_mods (void)
{
	if (bltptxpos < 0)
		return;
	bltptxpos = -1;
	switch (bltptxc)
	{
		case 1:
		bltapt = bltptx;
		break;
		case 2:
		bltbpt = bltptx;
		break;
		case 3:
		bltcpt = bltptx;
		break;
		case 4:
		bltdpt = bltptx;
		break;
	}
}

static void check_channel_mods (int hpos, int ch)
{
	static int blit_warned = 100;
	if (bltptxpos != hpos)
		return;
	if (ch == bltptxc) {
		bltptxpos = -1;
		if (blit_warned > 0) {
			write_log(_T("BLITTER: %08X write to %cPT ignored! %08x\n"), bltptx, ch + 'A' - 1, m68k_getpc());
			blit_warned--;
		}
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

static void blitter_interrupt(int hpos, int done)
{
	blt_info.blit_main = 0;
	if (blt_info.blit_interrupt)
		return;
	if (!done && (!blitter_cycle_exact || immediate_blits || currprefs.cpu_model >= 68030 || currprefs.cachesize || currprefs.m68k_speed < 0))
		return;
	blt_info.blit_interrupt = 1;
	send_interrupt (6, (4 + 1) * CYCLE_UNIT);
	//if (debug_dma)
	//	record_dma_event (DMA_EVENT_BLITIRQ, hpos, vpos);
	blitter_done_notify(blitline);
}

static void blitter_done (int hpos)
{
	ddat1use = 0;
	if (blt_info.blit_finald) {
		//if (debug_dma)
			//record_dma_event(DMA_EVENT_BLITFINALD, hpos, vpos);
	}
	blt_info.blit_finald = 0;
	blitter_interrupt (hpos, 1);
	blitter_done_notify(blitline);
	event2_remevent (ev2_blitter);
	unset_special (SPCFLAG_BLTNASTY);
	if (log_blitter & 1)
		write_log (_T("cycles %d, missed %d, total %d\n"),
			blit_totalcyclecounter, blit_misscyclecounter, blit_totalcyclecounter + blit_misscyclecounter);
	blt_info.blitter_dangerous_bpl = 0;
}

static void blitter_maybe_done_early(int hpos)
{
	if (currprefs.chipset_mask & CSMASK_AGA) {
		if (!(bltcon0 & 0x100) || blitline) {
			// immediately done if D disabled or line mode.
			blitter_done(hpos);
			return;
		}
	}
	// busy cleared, interrupt generated.
	// last D write still pending if not linemode and D channel active
	if (blitline) {
		blitter_done(hpos);
	} else {
		if (ddat1use && (bltcon0 & 0x100)) {
			blt_info.blit_finald = 1 + 2;
			blitter_interrupt(hpos, 0);
		} else {
			blitter_done(hpos);
		}
	}
}

STATIC_INLINE void chipmem_agnus_wput2 (uaecptr addr, uae_u32 w)
{
	//last_custom_value1 = w; blitter writes are not stored
	if (!(log_blitter & 4)) {
		//debug_putpeekdma_chipram(addr, w, MW_MASK_BLITTER_D_N, 0x000, 0x054);
		chipmem_wput_indirect (addr, w);
	}
}

static void blitter_dofast (void)
{
	int i,j;
	uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
	uae_u8 mt = bltcon0 & 0xFF;

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

	if (bltcon0 & 0x800) {
		bltadatptr = bltapt;
		bltapt += (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
	}
	if (bltcon0 & 0x400) {
		bltbdatptr = bltbpt;
		bltbpt += (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
	}
	if (bltcon0 & 0x200) {
		bltcdatptr = bltcpt;
		bltcpt += (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
	}
	if (bltcon0 & 0x100) {
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
		uaecptr dstp = 0;
		int dodst = 0;

		for (j = 0; j < blt_info.vblitsize; j++) {
			blitfc = !!(bltcon1 & 0x4);
			for (i = 0; i < blt_info.hblitsize; i++) {
				uae_u32 bltadat, blitahold;
				if (bltadatptr) {
					blt_info.bltadat = bltadat = chipmem_wget_indirect (bltadatptr);
					bltadatptr += 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)blt_info.bltaold << 16) | bltadat) >> blt_info.blitashift;
				blt_info.bltaold = bltadat;

				if (bltbdatptr) {
					uae_u16 bltbdat = chipmem_wget_indirect (bltbdatptr);
					bltbdatptr += 2;
					blitbhold = (((uae_u32)blt_info.bltbold << 16) | bltbdat) >> blt_info.blitbshift;
					blt_info.bltbold = bltbdat;
					blt_info.bltbdat = bltbdat;
				}

				if (bltcdatptr) {
					blt_info.bltcdat = chipmem_wget_indirect (bltcdatptr);
					bltcdatptr += 2;
				}
				if (dodst)
					chipmem_agnus_wput2 (dstp, blt_info.bltddat);
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
			chipmem_agnus_wput2 (dstp, blt_info.bltddat);
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

	blit_masktable[0] = blt_info.bltafwm;
	blit_masktable[blt_info.hblitsize - 1] &= blt_info.bltalwm;

	if (bltcon0 & 0x800) {
		bltadatptr = bltapt;
		bltapt -= (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
	}
	if (bltcon0 & 0x400) {
		bltbdatptr = bltbpt;
		bltbpt -= (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
	}
	if (bltcon0 & 0x200) {
		bltcdatptr = bltcpt;
		bltcpt -= (blt_info.hblitsize * 2 + blt_info.bltcmod) * blt_info.vblitsize;
	}
	if (bltcon0 & 0x100) {
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
			blitfc = !!(bltcon1 & 0x4);
			for (i = 0; i < blt_info.hblitsize; i++) {
				uae_u32 bltadat, blitahold;
				if (bltadatptr) {
					bltadat = blt_info.bltadat = chipmem_wget_indirect (bltadatptr);
					bltadatptr -= 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable[i];
				blitahold = (((uae_u32)bltadat << 16) | blt_info.bltaold) >> blt_info.blitdownashift;
				blt_info.bltaold = bltadat;

				if (bltbdatptr) {
					uae_u16 bltbdat = chipmem_wget_indirect (bltbdatptr);
					bltbdatptr -= 2;
					blitbhold = (((uae_u32)bltbdat << 16) | blt_info.bltbold) >> blt_info.blitdownbshift;
					blt_info.bltbold = bltbdat;
					blt_info.bltbdat = bltbdat;
				}

				if (bltcdatptr) {
					blt_info.bltcdat = blt_info.bltbdat = chipmem_wget_indirect (bltcdatptr);
					bltcdatptr -= 2;
				}
				if (dodst)
					chipmem_agnus_wput2 (dstp, blt_info.bltddat);
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
			chipmem_agnus_wput2 (dstp, blt_info.bltddat);
		blt_info.bltbhold = blitbhold;
	}
	blit_masktable[0] = 0xFFFF;
	blit_masktable[blt_info.hblitsize - 1] = 0xFFFF;
	blt_info.blit_main = 0;
}

static void blitter_read (void)
{
	if (bltcon0 & 0x200) {
		if (!dmaen (DMA_BLITTER))
			return;
		blt_info.bltcdat = chipmem_wget_indirect (bltcpt);
		last_custom_value1 = blt_info.bltcdat;
	}
}

static void blitter_write (void)
{
	if (blt_info.bltddat)
		blt_info.blitzero = 0;
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (bltcon0 & 0x200) {
		if (!dmaen (DMA_BLITTER))
			return;
		chipmem_wput_indirect (bltdpt, blt_info.bltddat);
	}
}

static void blitter_line_incx (void)
{
	if (++blinea_shift == 16) {
		blinea_shift = 0;
		bltcpt += 2;
	}
}

static void blitter_line_decx (void)
{
	if (blinea_shift-- == 0) {
		blinea_shift = 15;
		bltcpt -= 2;
	}
}

static void blitter_line_decy (void)
{
	bltcpt -= blt_info.bltcmod;
	blitonedot = 0;
}

static void blitter_line_incy (void)
{
	bltcpt += blt_info.bltcmod;
	blitonedot = 0;
}

static void blitter_line (void)
{
	uae_u16 blitahold = (blinea & blt_info.bltafwm) >> blinea_shift;
	uae_u16 blitchold = blt_info.bltcdat;

	blt_info.bltbhold = (blineb & 1) ? 0xFFFF : 0;
	blitlinepixel = !blitsing || (blitsing && !blitonedot);
	blt_info.bltddat = blit_func (blitahold, blt_info.bltbhold, blitchold, bltcon0 & 0xFF);
	blitonedot++;
}

static void blitter_line_proc (void)
{
	if (!blitsign) {
		if (bltcon1 & 0x10) {
			if (bltcon1 & 0x8)
				blitter_line_decy();
			else
				blitter_line_incy();
		} else {
			if (bltcon1 & 0x8)
				blitter_line_decx();
			else
				blitter_line_incx();
		}
	}
	if (bltcon1 & 0x10) {
		if (bltcon1 & 0x4)
			blitter_line_decx();
		else
			blitter_line_incx();
	} else {
		if (bltcon1 & 0x4)
			blitter_line_decy();
		else
			blitter_line_incy();
	}

	if (bltcon0 & 0x800) {
		if (blitsign)
			bltapt += (uae_s16)blt_info.bltbmod;
		else
			bltapt += (uae_s16)blt_info.bltamod;
	}
	blitsign = 0 > (uae_s16)bltapt;
}

static void blitter_nxline (void)
{
	blineb = (blineb << 1) | (blineb >> 15);
}

static void actually_do_blit (void)
{
	if (blitline) {
		do {
			record_dma_blit(0x70, 0, bltcpt, last_blitter_hpos);
			blitter_read();
			record_dma_blit_val(blt_info.bltcdat);
			if (ddat1use)
				bltdpt = bltcpt;
			ddat1use = 1;
			blitter_line();
			blitter_line_proc();
			blitter_nxline();
			blt_info.vblitsize--;
			if (blitlinepixel) {
				record_dma_blit(0x00, blt_info.bltddat, bltdpt, last_blitter_hpos);
				blitter_write();
				blitlinepixel = 0;
			}
		} while (blt_info.vblitsize != 0);
		bltdpt = bltcpt;
	} else {
		if (blitdesc)
			blitter_dofast_desc();
		else
			blitter_dofast();
	}
	blt_info.blit_main = 0;
}

static void blitter_doit (void)
{
	if (blt_info.vblitsize == 0 || (blitline && blt_info.hblitsize != 2)) {
		blitter_done (current_hpos());
		return;
	}
	actually_do_blit();
	blitter_done (current_hpos());
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

void blitter_handler (uae_u32 data)
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
		//debugtest (DEBUGTEST_BLITTER, _T("force-unstuck!\n"));
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

static void blit_bltset(int con)
{
	static int blit_warned = 100;

	if (con & 2) {
		blitdesc = bltcon1 & 2;
		blt_info.blitbshift = bltcon1 >> 12;
		blt_info.blitdownbshift = 16 - blt_info.blitbshift;
		if (blit_warned > 0) {
			if ((bltcon1 & 1) && !blitline_started) {
				write_log(_T("BLITTER: linedraw enabled when blitter was already active! %08x\n"), M68K_GETPC);
			} else if (!(bltcon1 & 1) && blitline_started) {
				write_log(_T("BLITTER: linedraw disabled when blitter was already active! %08x\n"), M68K_GETPC);
			}
			blit_warned--;
		}
	}

	if (con & 1) {
		blt_info.blitashift = bltcon0 >> 12;
		blt_info.blitdownashift = 16 - blt_info.blitashift;
	}

	if (!savestate_state && blt_info.blit_main && (bltcon0_old != bltcon0 || bltcon1_old != bltcon1)) {
		if (blit_warned > 0) {
			write_log(_T("BLITTER: BLTCON0 %04x -> %04x BLTCON1 %04x -> %04x PC=%08x\n"), bltcon0_old, bltcon0, bltcon1_old, bltcon1, M68K_GETPC);
			blit_warned--;
		}
		bltcon0_old = bltcon0;
		bltcon1_old = bltcon1;
	}

	blit_ch = (bltcon0 & 0x0f00) >> 8;
	blitline = bltcon1 & 1;
	blit_ovf = (bltcon1 & 0x20) != 0;

	shifter_skip_b = (bltcon0 & 0x400) == 0;
	if (blitline) {
		shifter_skip_y = true;
		blitfill = 0;
	} else {
		blitfill = (bltcon1 & 0x18) != 0;
		blitfc = !!(bltcon1 & 0x4);
		blitife = !!(bltcon1 & 0x8);
		if ((bltcon1 & 0x18) == 0x18) {
			//debugtest(DEBUGTEST_BLITTER, _T("weird fill mode\n"));
			blitife = 0;
		}
		if (blitfill && !blitdesc) {
			//debugtest(DEBUGTEST_BLITTER, _T("fill without desc\n"));
			//if (log_blitter & 16)
			//	activate_debugger();
		}
		shifter_skip_y = (bltcon0 & (0x100 | 0x200)) != 0x300;
		// fill mode idle cycle needed?
		if (blitfill && (bltcon0 & (0x100 | 0x200)) == 0x100) {
			shifter_skip_y = false;
		}
	}
	shifter_out = shifter_skip_y ? shifter[2] : shifter[3];

	blit_cyclecount = 4 - (shifter_skip_b + shifter_skip_y);
	blit_dmacount = ((bltcon0 & 0x800) ? 1 : 0) + ((bltcon0 & 0x400) ? 1 : 0) +
		((bltcon0 & 0x200) ? 1 : 0) + ((bltcon0 & 0x100) ? 1 : 0);

	//if ((bltcon1 & 0x80) && (currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
		//debugtest(DEBUGTEST_BLITTER, _T("ECS BLTCON1 DOFF-bit set\n"));
		//if (log_blitter & 16)
			//activate_debugger();
	//}
}

static int get_current_channel(void)
{
	static int blit_warned = 100;

	if (blit_cyclecounter < 0) {
		return 0;
	}

	if (!blit_faulty && blit_cyclecounter > 0) {
		int cnt = 0;
		for (int i = 0; i < 4; i++) {
			if (shifter[i])
				cnt++;
		}
		if (cnt == 0) {
			if (blit_warned > 0) {
				write_log(_T("Blitter stopped. Shift register empty! PC=%08x\n"), M68K_GETPC);
				blit_warned--;
			}
			blit_faulty = 1;
		}
		if (cnt > 1) {
			if (blit_warned > 0) {
				write_log(_T("Blitter shifter register number of bits %d! CH=%c%c%c%c (%d-%d-%d-%d) PC=%08x\n"),
					cnt,
					((bltcon0 >> 11) & 1) ? 'A' : '-', ((bltcon0 >> 10) & 1) ? 'B' : '-', ((bltcon0 >> 9) & 1) ? 'C' : '-', ((bltcon0 >> 8) & 1) ? 'D' : '-',
					shifter[0], shifter[1], shifter[2], shifter[3], M68K_GETPC);
				blit_warned--;
			}
			blit_faulty = 1;
		}
	}

	if (blitline) {
		if (shifter[0]) {
			// A or idle
			if (blitter_hcounter + 1 == blt_info.hblitsize)
				return 5;
			if (bltcon0 & 0x800)
				return 1;
			return 0;
		}
		// B
		if (shifter[1] && (bltcon0 & 0x400)) {
			return 2;
		}
		// C or D
		if (shifter[2] && (bltcon0 & 0x200)) {
			if (blitter_hcounter + 1 == blt_info.hblitsize)
				return 4;
			return 3;
		}
	} else {
		// order is important when multiple bits in shift register
		// C
		if (shifter[2] && (bltcon0 & 0x200)) {
			return 3;
		}
		// Shift stage 4 active, C enabled and other stage(s) also active:
		// normally would be D but becomes C.
		if (shifter[3] && (bltcon0 & 0x200) && (shifter[0] || shifter[1])) {
			return 3;
		}
		// A
		if (shifter[0] && (bltcon0 & 0x800)) {
			return 1;
		}
		// B
		if (shifter[1] && (bltcon0 & 0x400)) {
			return 2;
		}
		// D only if A, B and C is not currently active
		if (ddat1use) {
			// if stage 3 and C disabled and D enabled: D
			if (shifter[2] && !(bltcon0 & 0x200) && (bltcon0 & 0x100)) {
				return 4;
			}
			// if stage 4 and C enabled and D enabled: D
			if (shifter[3] && (bltcon0 & 0x200) && (bltcon0 & 0x100)) {
				return 4;
			}
		}
	}
	return 0;
}

static uae_u16 blitter_doblit (void)
{
	uae_u32 blitahold;
	uae_u16 bltadat, ddat;
	uae_u8 mt = bltcon0 & 0xFF;

	bltadat = blt_info.bltadat;
	if (blitter_hcounter == 0)
		bltadat &= blt_info.bltafwm;
	if (blitter_hcounter == blt_info.hblitsize - 1)
		bltadat &= blt_info.bltalwm;
	if (blitdesc)
		blitahold = (((uae_u32)bltadat << 16) | blt_info.bltaold) >> blt_info.blitdownashift;
	else
		blitahold = (((uae_u32)blt_info.bltaold << 16) | bltadat) >> blt_info.blitashift;
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

	if (ddat)
		blt_info.blitzero = 0;

	return ddat;
}

static void blitter_next_cycle(void)
{
	bool tmp[4];
	bool out = false;

	memcpy(tmp, shifter, sizeof(shifter));
	memset(shifter, 0, sizeof(shifter));

	if (shifter_skip_b_old && !shifter_skip_b) {
		// if B skip was disabled: A goes both to B and C
		tmp[1] = tmp[0];
		tmp[2] = tmp[0];
		shifter_skip_b_old = shifter_skip_b;
	} else if (!shifter_skip_b_old && shifter_skip_b) {
		// if B skip was enabled: A goes nowhere
		tmp[0] = false;
		shifter_skip_b_old = shifter_skip_b;
	}

	if (shifter_skip_y_old && !shifter_skip_y) {
		// if Y skip was disbled: X goes both to Y and OUT
		tmp[3] = tmp[2];
		shifter_skip_y_old = shifter_skip_y;
	} else if (!shifter_skip_y_old && shifter_skip_y) {
		// if Y skip was enabled: X goes nowhere
		tmp[2] = false;
		shifter_out = false;
		shifter_skip_y_old = shifter_skip_y;
	}

	if (shifter_out) {
		if (!blitline) {
			ddat1 = blitter_doblit();
			if (bltcon0 & 0x100) {
				ddat1use = true;
			}
		}
		blitter_hcounter++;
		if (blitter_hcounter == blt_info.hblitsize) {
			blitter_hcounter = 0;
			blitter_vcounter++;
			blitfc = !!(bltcon1 & 0x4);
			if (blitter_vcounter == blt_info.vblitsize) {
				shifter_out = false;
				blit_cyclecounter = 0;
				blitter_maybe_done_early(last_blitter_hpos);
			}
		}
		shifter[0] = shifter_out;
	}

	if (shifter_first) {
		shifter_first = false;
		shifter[0] = true;
		blitfc = !!(bltcon1 & 0x4);
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
}


static void blitter_doddma_new(int hpos)
{
	record_dma_blit(0x00, ddat1, bltdpt, hpos);
	chipmem_agnus_wput2(bltdpt, ddat1);
	alloc_cycle_blitter(hpos, &bltdpt, 4);

	if (!blitline) {
		bltdpt += blit_add;
		if (blitter_hcounter == 0) {
			bltdpt += blit_modaddd;
		}
	}
}

static void blitter_dodma_new(int ch, int hpos)
{
	uae_u16 dat, reg;
	uae_u32 *addr;
	int mod;

	switch (ch)
	{
	case 1:
		reg = 0x74;
		record_dma_blit(reg, 0, bltapt, hpos);
		blt_info.bltadat = dat = chipmem_wget_indirect(bltapt);
		record_dma_blit_val(dat);
		last_custom_value1 = blt_info.bltadat;
		addr = &bltapt;
		mod = blit_modadda;
		alloc_cycle_blitter(hpos, &bltapt, 1);
		break;
	case 2:
		reg = 0x72;
		record_dma_blit(reg, 0, bltbpt, hpos);
		blt_info.bltbdat = dat = chipmem_wget_indirect(bltbpt);
		record_dma_blit_val(dat);
		last_custom_value1 = blt_info.bltbdat;
		addr = &bltbpt;
		mod = blit_modaddb;
		if (blitdesc)
			blt_info.bltbhold = (((uae_u32)blt_info.bltbdat << 16) | blt_info.bltbold) >> blt_info.blitdownbshift;
		else
			blt_info.bltbhold = (((uae_u32)blt_info.bltbold << 16) | blt_info.bltbdat) >> blt_info.blitbshift;
		blt_info.bltbold = blt_info.bltbdat;
		alloc_cycle_blitter(hpos, &bltbpt, 2);
		break;
	case 3:
		reg = 0x70;
		record_dma_blit(reg, 0, bltcpt, hpos);
		blt_info.bltcdat = dat = chipmem_wget_indirect(bltcpt);
		record_dma_blit_val(dat);
		last_custom_value1 = blt_info.bltcdat;
		addr = &bltcpt;
		mod = blit_modaddc;
		alloc_cycle_blitter(hpos, &bltcpt, 3);
		break;
	default:
		abort();
	}

	if (!blitline) {
		(*addr) += blit_add;
		if (blitter_hcounter + 1 == blt_info.hblitsize) {
			(*addr) += mod;
		}
	}
}


static bool blitter_idle_cycle_register_write(uaecptr addr, uae_u16 v)
{
	addrbank *ab = &get_mem_bank(addr);
	if (ab != &custom_bank)
		return false;
	addr &= 0x1fe;
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

static bool decide_blitter_idle(int lasthpos, int hpos, uaecptr addr, uae_u16 value)
{
	markidlecycle(last_blitter_hpos);
	if (addr != 0xffffffff && lasthpos + 1 == hpos) {
		shifter_skip_b_old = shifter_skip_b;
		shifter_skip_y_old = shifter_skip_y;
		return blitter_idle_cycle_register_write(addr, value);
	}
	return false;
}

void decide_blitter(int hpos)
{
	decide_blitter_maybe_write(hpos, 0xffffffff, 0xffff);
}

bool decide_blitter_maybe_write(int hpos, uaecptr addr, uae_u16 value)
{
	bool written = false;
	int hsync = hpos < 0;

	if (hsync && blt_delayed_irq) {
		if (blt_delayed_irq > 0)
			blt_delayed_irq--;
		if (blt_delayed_irq <= 0) {
			blt_delayed_irq = 0;
			send_interrupt(6, 2 * CYCLE_UNIT);
		}
	}

	if (immediate_blits) {
		if (!blt_info.blit_main)
			return false;
		if (dmaen(DMA_BLITTER))
			blitter_doit();
		return false;
	}

	if (hpos < 0) {
		hpos = maxhpos;
	}

	if (!blt_info.blit_main && !blt_info.blit_finald) {
		last_blitter_hpos = hpos;
		goto end;
	}

	if (log_blitter && blitter_delayed_debug) {
		blitter_delayed_debug = 0;
		blitter_dump();
	}

	if (!blitter_cycle_exact) {
		return false;
	}

	while (last_blitter_hpos < hpos) {

		int c = get_current_channel();

		for (;;) {
			int v = canblit(last_blitter_hpos);

			// final D idle cycle
			// does not need free bus
			if (blt_info.blit_finald > 1) {
				blt_info.blit_finald--;
			}

			// copper bltsize write needs one cycle (any cycle) delay
			// does not need free bus
			if (blit_waitcyclecounter) {
				blit_waitcyclecounter = 0;
				break;
			}

			if (v <= 0) {
				blit_misscyclecounter++;
				break;
			}

			if (blt_info.blit_finald == 1) {
				// final D write
				blitter_doddma_new(last_blitter_hpos);
				blitter_done(last_blitter_hpos);
				break;
			}

			if (blt_info.blit_main) {
				blit_totalcyclecounter++;
				blit_cyclecounter++;
				if (blit_cyclecounter == 0) {
					shifter_first = true;
				}

				blt_info.got_cycle = 1;

				if (c == 0) {

					written = decide_blitter_idle(last_blitter_hpos, hpos, addr, value);

				} else if (c == 1 && blitline) { // line 1/4 (A, free)

					written = decide_blitter_idle(last_blitter_hpos, hpos, addr, value);

				} else if (c == 3 && blitline) { // line 2/4 (C)

					record_dma_blit(0x70, 0, bltcpt, last_blitter_hpos);
					blt_info.bltcdat = chipmem_wget_indirect(bltcpt);
					last_custom_value1 = blt_info.bltcdat;
					record_dma_blit_val(blt_info.bltcdat);
					alloc_cycle_blitter(last_blitter_hpos, &bltcpt, 3);

				} else if (c == 5 && blitline) { // line 3/4 (free)

					blitter_line();

					written = decide_blitter_idle(last_blitter_hpos, hpos, addr, value);

				} else if (c == 4 && blitline) { // line 4/4 (D)

					if (ddat1use)
						bltdpt = bltcpt;
					ddat1use = 1;

					blitter_line_proc();
					blitter_nxline();

					/* onedot mode and no pixel = bus write access is skipped */
					if (blitlinepixel) {
						record_dma_blit(0x00, blt_info.bltddat, bltdpt, last_blitter_hpos);
						if (blt_info.bltddat)
							blt_info.blitzero = 0;
						chipmem_wput_indirect(bltdpt, blt_info.bltddat);
						alloc_cycle_blitter(last_blitter_hpos, &bltdpt, 4);
						blitlinepixel = 0;
					}
					bltdpt = bltcpt;

				} else {
					// normal mode A to D

					if (c == 4) {
						blitter_doddma_new(last_blitter_hpos);
					} else {
						blitter_dodma_new(c, last_blitter_hpos);
					}
				}

				blitter_next_cycle();

				// check this after end check because last D write won't cause any problems.
				check_channel_mods(last_blitter_hpos, c);
			}
			break;
		}

		last_blitter_hpos++;
	}
end:
	reset_channel_mods();
	if (hsync)
		last_blitter_hpos = 0;

	return written;
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
		while (blt_info.blit_main || blt_info.blit_finald && rounds > 0) {
			memset(cycle_line, 0, sizeof cycle_line);
			decide_blitter(-1);
			rounds--;
		}
		if (rounds == 0)
			write_log(_T("blitter froze!?\n"));
	} else {
		actually_do_blit();
	}
	blitter_done(current_hpos());
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
	if (bltcon & 1)
		blinea_shift = bltcon0 >> 12;
	if (bltcon & 2)
		blitsign = bltcon1 & 0x40;
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
	while ((blt_info.blit_main || blt_info.blit_finald) && dmaen (DMA_BLITTER)) {
		waited = true;
		x_do_cycles (8 * CYCLE_UNIT);
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
	blit_faulty = 0;
	blt_info.blitzero = 1;
	blitline_started = bltcon1 & 1;

	blit_bltset (1 | 2);
	shifter_skip_b_old = shifter_skip_b;
	shifter_skip_y_old = shifter_skip_y;
	blit_modset();
	ddat1use = 0;
	blt_info.blit_interrupt = 0;

	blt_info.bltaold = 0;
	blt_info.bltbold = 0;

	if (blitline) {
		blinea = blt_info.bltadat;
		blineb = (blt_info.bltbdat >> blt_info.blitbshift) | (blt_info.bltbdat << (16 - blt_info.blitbshift));
		blitonedot = 0;
		blitlinepixel = 0;
		blitsing = bltcon1 & 0x2;
	}

	if (!(dmacon & DMA_BLITPRI) && blt_info.nasty_cnt >= BLIT_NASTY_CPU_STEAL_CYCLE_COUNT) {
		blt_info.wait_nasty = 1;
	} else {
		blt_info.wait_nasty = 0;
	}
}

void do_blitter(int hpos, int copper, uaecptr pc)
{
	int cycles;

	if ((log_blitter & 2)) {
		if (blt_info.blit_main) {
			write_log (_T("blitter was already active! PC=%08x\n"), M68K_GETPC);
		}
	}

	bltcon0_old = bltcon0;
	bltcon1_old = bltcon1;

	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	immediate_blits = currprefs.immediate_blits;
	blt_info.got_cycle = 0;
	last_blitter_hpos = hpos;
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

	//if (memwatch_enabled) {
		//blitter_debugsave(copper, pc);
	//}

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

	if (blitter_cycle_exact) {
		if (immediate_blits) {
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
			blit_cyclecounter = -BLITTER_STARTUP_CYCLES;
			blit_waitcyclecounter = copper;
			blit_maxcyclecounter = blt_info.hblitsize * blt_info.vblitsize + 2;
			blt_info.blit_pending = 0;
			blt_info.blit_main = 1;
		}
		return;
	}

	if (blt_info.vblitsize == 0 || (blitline && blt_info.hblitsize != 2)) {
		if (dmaen(DMA_BLITTER)) {
			blitter_done(hpos);
		}
		return;
	}

	if (dmaen (DMA_BLITTER)) {
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

void maybe_blit (int hpos, int hack)
{
	static int warned = 10;

	reset_channel_mods();

	if (!blt_info.blit_main)
		return;

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
		//debugtest (DEBUGTEST_BLITTER, _T("program does not wait for blitter tc=%d\n"), blit_cyclecounter);
		if (log_blitter)
			warned = 0;
		if (log_blitter & 2) {
			warned = 10;
			write_log (_T("program does not wait for blitter PC=%08x\n"), M68K_GETPC);
			//activate_debugger();
			//blitter_done (hpos);
		}
	}

	if (blitter_cycle_exact) {
		decide_blitter (hpos);
		goto end;
	}

	if (hack == 1 && (int)get_cycles() - (int)blit_firstline_cycles < 0)
		goto end;

	blitter_handler (0);
end:;
	if (log_blitter)
		blitter_delayed_debug = 1;
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
		return 0;
	if (!dmaen(DMA_BLITTER))
		return 0;
	if (blitter_cycle_exact) {
		blitter_force_finish(false);
		return -1;
	}
	if (blit_last_cycle >= blit_cyclecount && blit_dmacount == blit_cyclecount)
		return 0;
	cycles = (get_cycles() - blit_first_cycle) / CYCLE_UNIT;
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
}

#ifdef SAVESTATE

void restore_blitter_finish (void)
{
	//record_dma_reset();
	//record_dma_reset();
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
	return src;
}

uae_u8 *save_blitter (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak,*dst;
	int forced;

	forced = 0;
	if (blt_info.blit_main || blt_info.blit_finald) {
		write_log (_T("blitter is active, forcing immediate finish\n"));
		/* blitter is active just now but we don't have blitter state support yet */
		blitter_force_finish(true);
		forced = 2;
	}
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 16);
	save_u32 (((blt_info.blit_main || blt_info.blit_finald) ? 0 : 1) | forced | 4);
	*len = dst - dstbak;
	return dstbak;

}

// totally non-real-blitter-like state save but better than nothing..

uae_u8 *restore_blitter_new (uae_u8 *src)
{
	uae_u8 state, tmp;

	blt_statefile_type = 1;
	blitter_cycle_exact = restore_u8();
	if (blitter_cycle_exact == 3) {
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

	blt_info.blitbshift = restore_u8();
	blt_info.blitdownbshift = restore_u8();
	blt_info.blitashift = restore_u8();
	blt_info.blitdownashift = restore_u8();

	ddat1use = restore_u8();
	restore_u8();
	ddat1 = restore_u16();
	restore_u16();

	blitline = restore_u8();
	blitfill = restore_u8();
	blinea = restore_u16();
	blineb = restore_u16();
	blinea_shift = restore_u8();
	blitonedot = restore_u8();
	blitlinepixel = restore_u8();
	blitsing = restore_u8();
	blitlinepixel = restore_u8();
	blt_info.blit_interrupt = restore_u8();
	blt_delayed_irq = restore_u8();
	blt_info.blitzero = restore_u8();
	blt_info.got_cycle = restore_u8();

	restore_u8();
	blit_faulty = restore_u8();
	restore_u8();
	restore_u8();
	restore_u8();

	if (restore_u16() != 0x1234)
		write_log (_T("error\n"));

	blt_info.blitter_nasty = restore_u8();
	tmp = restore_u8();
	shifter[0] = (tmp & 1) != 0;
	shifter[1] = (tmp & 2) != 0;
	shifter[2] = (tmp & 4) != 0;
	shifter[3] = (tmp & 8) != 0;
	blt_info.blit_finald = restore_u8();
	blit_ovf = restore_u8();

	blt_info.blit_main = 0;
	blt_info.blit_finald = 0;
	blt_info.blit_pending = 0;

	if (!blitter_cycle_exact) {
		if (state > 0)
			do_blitter(0, 0, 0);
	} else {
		if (state == 1)
			blt_info.blit_pending = 1;
		else if (state == 2)
			blt_info.blit_main = 1;
		if (blt_info.blit_finald) {
			blt_info.blit_main = 0;
		}
		if (blt_statefile_type == 2) {
			blit_bltset(0);
		}
	}
	return src;
}

uae_u8 *save_blitter_new (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak,*dst;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);

	uae_u8 state;
	save_u8 (blitter_cycle_exact ? 3 : 0);
	if (!blt_info.blit_main && !blt_info.blit_finald)
		state = 0;
	else if (blt_info.blit_pending)
		state = 1;
	else
		state = 2;
	save_u8 (state);

	if (blt_info.blit_main || blt_info.blit_finald) {
		write_log (_T("BLITTER active while saving state\n"));
		if (log_blitter)
			blitter_dump();
	}

	save_u32(blit_first_cycle);
	save_u32(blit_last_cycle);
	save_u32(blit_waitcyclecounter);
	save_u32(0); //(blit_startcycles);
	save_u32(blit_maxcyclecounter);
	save_u32(blit_firstline_cycles);
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

	save_u8(blt_info.blitbshift);
	save_u8(blt_info.blitdownbshift);
	save_u8(blt_info.blitashift);
	save_u8(blt_info.blitdownashift);

	save_u8(ddat1use);
	save_u8(0); //(ddat2use);
	save_u16(ddat1);
	save_u16(0); //(ddat2);

	save_u8(blitline);
	save_u8(blitfill);
	save_u16(blinea);
	save_u16(blineb);
	save_u8(blinea_shift);
	save_u8(blitonedot);
	save_u8(blitlinepixel);
	save_u8(blitsing);
	save_u8(blitlinepixel);
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
	save_u8((shifter[0] ? 1 : 0) | (shifter[1] ? 2 : 0) | (shifter[2] ? 4 : 0) | (shifter[3] ? 8 : 0));
	save_u8(blt_info.blit_finald);
	save_u8(blit_ovf);

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */
