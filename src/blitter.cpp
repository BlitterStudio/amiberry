/*
* UAE - The Un*x Amiga Emulator
*
* Custom chip emulation
*
* (c) 1995 Bernd Schmidt, Alessandro Bissacco
* (c) 2002 - 2005 Toni Wilen
*/

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

static int original_ch, original_fill, original_line;

static int blinea_shift;
static uae_u16 blinea, blineb;
static int blitline, blitfc, blitfill, blitife, blitsing, blitdesc;
static int blitline_started;
static int blitonedot, blitsign, blitlinepixel;
static int blit_add;
static int blit_modadda, blit_modaddb, blit_modaddc, blit_modaddd;
static int blit_ch;

static int blitter_dontdo;
static int blitter_delayed_debug;
#ifdef BLITTER_SLOWDOWNDEBUG
static int blitter_slowdowndebug;
#endif

struct bltinfo blt_info;

static uae_u8 blit_filltable[256][4][2];
uae_u32 blit_masktable[BLITTER_MAX_WORDS];
enum blitter_states bltstate;

static int blit_cyclecounter, blit_waitcyclecounter;
static uaecptr blit_waitpc;
static int blit_maxcyclecounter, blit_slowdown, blit_totalcyclecounter;
static int blit_startcycles, blit_misscyclecounter;

//#ifdef CPUEMU_13
extern uae_u8 cycle_line[256];
//#endif

static long blit_firstline_cycles;
static long blit_first_cycle;
static int blit_last_cycle, blit_dmacount, blit_dmacount2;
static int blit_linecycles, blit_extracycles, blit_nod;
static const int *blit_diag;
static int blit_frozen, blit_faulty;
static int blit_final;
static int blt_delayed_irq;
static uae_u16 ddat1, ddat2;
static int ddat1use, ddat2use;

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

/*

following 4 channel combinations in fill mode have extra
idle cycle added (still requires free bus cycle)

*/

static const int blit_cycle_diagram_fill[][DIAGSIZE] =
{
	{ 0 },						/* 0 */
	{ 3, 0,0,0,	    0,4,0 },	/* 1 */
	{ 0 },						/* 2 */
	{ 0 },						/* 3 */
	{ 0 },						/* 4 */
	{ 4, 0,2,0,0,   0,2,4,0 },	/* 5 */
	{ 0 },						/* 6 */
	{ 0 },						/* 7 */
	{ 0 },						/* 8 */
	{ 3, 1,0,0,	    1,4,0 },	/* 9 */
	{ 0 },						/* A */
	{ 0 },						/* B */
	{ 0 },						/* C */
	{ 4, 1,2,0,0,   1,2,4,0 },	/* D */
	{ 0 },						/* E */
	{ 0 },						/* F */
};

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

// 5 = internal "processing cycle"
static const int blit_cycle_diagram_line[] =
{
	4, 0,3,5,4,	    0,3,5,4
};

static const int blit_cycle_diagram_finald[] =
{
	2, 0,4,	    0,4
};

static const int blit_cycle_diagram_finalld[] =
{
	2, 0,0,	    0,0
};

static int get_cycle_diagram_type (const int *diag)
{
	for (int i = 0; i < 16; i++) {
		if (diag == &blit_cycle_diagram[i][0])
			return i;
		if (diag == &blit_cycle_diagram_fill[i][0])
			return i + 0x40;
	}
	if (diag == blit_cycle_diagram_line)
		return 0x80;
	if (diag == blit_cycle_diagram_finald)
		return 0x81;
	if (diag == blit_cycle_diagram_finalld)
		return 0x82;
	return 0xff;
}
static const int *set_cycle_diagram_type (uae_u8 diag)
{
	if (diag >= 0x00 && diag <= 0x0f)
		return &blit_cycle_diagram[diag][0];
	if (diag >= 0x40 && diag <= 0x4f)
		return &blit_cycle_diagram_fill[diag][0];
	if (diag == 0x80)
		return blit_cycle_diagram_line;
	if (diag == 0x81)
		return blit_cycle_diagram_finald;
	if (diag == 0x82)
		return blit_cycle_diagram_finalld;
	return NULL;
}

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

STATIC_INLINE void record_dma_blit (uae_u16 reg, uae_u16 dat, uae_u32 addr, int hpos)
{
#ifdef DEBUGGER
	if (debug_dma)
		record_dma (reg, dat, addr, hpos, vpos, DMARECORD_BLITTER, blitline ? 2 : (blitfill ? 1 : 0));
	if (memwatch_enabled) {
		if (reg == 0) {
			uae_u32 mask = MW_MASK_BLITTER_D_N;
			if (blitfill)
				mask = MW_MASK_BLITTER_D_F;
			if (blitline)
				mask = MW_MASK_BLITTER_D_L;
			debug_wputpeekdma_chipram(addr, dat, mask, reg, 0x054);
		} else if (reg == 0x70) {
			debug_wgetpeekdma_chipram(addr, dat, MW_MASK_BLITTER_C, reg, 0x48);
		} else if (reg == 0x72) {
			debug_wgetpeekdma_chipram(addr, dat, MW_MASK_BLITTER_B, reg, 0x4c);
		} else if (reg == 0x74) {
			debug_wgetpeekdma_chipram(addr, dat, MW_MASK_BLITTER_A, reg, 0x52);
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

STATIC_INLINE const int *get_ch (void)
{
	if (blit_faulty)
		return &blit_diag[0];
	if (blit_final)
		return blitline || blit_nod ? blit_cycle_diagram_finalld : blit_cycle_diagram_finald;
	return blit_diag;
}

STATIC_INLINE int channel_state (int cycles)
{
	const int *diag;
	if (cycles < 0)
		return 0;
	diag = get_ch ();
	if (cycles < diag[0])
		return diag[1 + cycles];
	cycles -= diag[0];
	cycles %= diag[0];
	return diag[1 + diag[0] + cycles];
}
STATIC_INLINE int channel_pos (int cycles)
{
	const int *diag;
	if (cycles < 0)
		return 0;
	diag =  get_ch ();
	if (cycles < diag[0])
		return cycles;
	cycles -= diag[0];
	cycles %= diag[0];
	return cycles;
}

int blitter_channel_state (void)
{
	return channel_state (blit_cyclecounter);
}

STATIC_INLINE int canblit (int hpos)
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
	if (bltptxpos != hpos)
		return;
	if (ch == bltptxc) {
		bltptxpos = -1;
		write_log (_T("BLITTER: %08X write to %cPT ignored! %08x\n"), bltptx, ch + 'A' - 1, m68k_getpc ());
		//activate_debugger();
	}
}

// blitter interrupt is set (and busy bit cleared) when
// last "main" cycle has been finished, any non-linedraw
// D-channel blit still needs 2 more cycles before final
// D is written (idle cycle, final D write)
//
// line draw interrupt triggers when last D is written
// (or cycle where last D write would have been if
// ONEDOT was active)

static void blitter_interrupt (int hpos, int done)
{
	if (blt_info.blit_interrupt)
		return;
	if (!done && (!blitter_cycle_exact || immediate_blits || currprefs.cpu_model >= 68030 || currprefs.cachesize || currprefs.m68k_speed < 0))
		return;
	blt_info.blit_interrupt = 1;
	send_interrupt (6, 4 * CYCLE_UNIT);
}

static void blitter_done (int hpos)
{
	ddat1use = ddat2use = 0;
	bltstate = blit_startcycles == 0 || !blitter_cycle_exact || immediate_blits ? BLT_done : BLT_init;
	blitter_interrupt (hpos, 1);
	blitter_done_notify (hpos);
	markidlecycle (hpos);
	event2_remevent (ev2_blitter);
	unset_special (SPCFLAG_BLTNASTY);
	blt_info.blitter_dangerous_bpl = 0;
}

STATIC_INLINE void chipmem_agnus_wput2 (uaecptr addr, uae_u32 w)
{
	chipmem_wput_indirect (addr, w);
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

	if (blitfunc_dofast[mt] && !blitfill) {
		(*blitfunc_dofast[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	}
	else 
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

	bltstate = BLT_done;
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
	if (blitfunc_dofast_desc[mt] && !blitfill) {
		(*blitfunc_dofast_desc[mt])(bltadatptr, bltbdatptr, bltcdatptr, bltddatptr, &blt_info);
	} else
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

	bltstate = BLT_done;
}

STATIC_INLINE void blitter_read (void)
{
	if (bltcon0 & 0x200) {
		if (!dmaen (DMA_BLITTER))
			return;
		blt_info.bltcdat = chipmem_wget_indirect (bltcpt);
		last_custom_value1 = blt_info.bltcdat;
	}
	bltstate = BLT_work;
}

STATIC_INLINE void blitter_write (void)
{
	if (blt_info.bltddat)
		blt_info.blitzero = 0;
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (bltcon0 & 0x200) {
		if (!dmaen (DMA_BLITTER))
			return;
		//last_custom_value1 = blt_info.bltddat; blitter writes are not stored
		chipmem_wput_indirect (bltdpt, blt_info.bltddat);
	}
	bltstate = BLT_next;
}

STATIC_INLINE void blitter_line_incx (void)
{
	if (++blinea_shift == 16) {
		blinea_shift = 0;
		bltcpt += 2;
	}
}

STATIC_INLINE void blitter_line_decx (void)
{
	if (blinea_shift-- == 0) {
		blinea_shift = 15;
		bltcpt -= 2;
	}
}

STATIC_INLINE void blitter_line_decy (void)
{
	bltcpt -= blt_info.bltcmod;
	blitonedot = 0;
}

STATIC_INLINE void blitter_line_incy (void)
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
	if (bltcon0 & 0x800) {
		if (blitsign)
			bltapt += (uae_s16)blt_info.bltbmod;
		else
			bltapt += (uae_s16)blt_info.bltamod;
	}

	if (!blitsign) {
		if (bltcon1 & 0x10) {
			if (bltcon1 & 0x8)
				blitter_line_decy ();
			else
				blitter_line_incy ();
		} else {
			if (bltcon1 & 0x8)
				blitter_line_decx ();
			else
				blitter_line_incx ();
		}
	}
	if (bltcon1 & 0x10) {
		if (bltcon1 & 0x4)
			blitter_line_decx ();
		else
			blitter_line_incx ();
	} else {
		if (bltcon1 & 0x4)
			blitter_line_decy ();
		else
			blitter_line_incy ();
	}

	blitsign = 0 > (uae_s16)bltapt;
	bltstate = BLT_write;
}

STATIC_INLINE void blitter_nxline (void)
{
	blineb = (blineb << 1) | (blineb >> 15);
	blt_info.vblitsize--;
	bltstate = BLT_read;
}

//#ifdef CPUEMU_13

static int blitter_cyclecounter;
static int blitter_hcounter1, blitter_hcounter2;
static int blitter_vcounter1, blitter_vcounter2;

static void decide_blitter_line (int hsync, int hpos)
{
	if (blit_final && blt_info.vblitsize)
		blit_final = 0;
	while (last_blitter_hpos < hpos) {
		int c = channel_state (blit_cyclecounter);

		for (;;) {
			int v = canblit (last_blitter_hpos);

			if (blit_waitcyclecounter) {
				blit_waitcyclecounter = 0;
				break;
			}

			// final 2 idle cycles? does not need free bus
			if (blit_final) { 
				blit_cyclecounter++;
				blit_totalcyclecounter++;
				if (blit_cyclecounter >= 2) {
					blitter_done(last_blitter_hpos);
					return;
				}
				break;
			}

			if (v <= 0) {
				blit_misscyclecounter++;
				break;
			}

			blit_cyclecounter++;
			blit_totalcyclecounter++;

			check_channel_mods (last_blitter_hpos, c);

			if (c == 3) {

				blitter_read ();
				alloc_cycle_blitter (last_blitter_hpos, &bltcpt, 3);
				record_dma_blit (0x70, blt_info.bltcdat, bltcpt, last_blitter_hpos);

			} else if (c == 5) {

				if (ddat1use) {
					bltdpt = bltcpt;
				}
				ddat1use = 1;
				blitter_line ();
				blitter_line_proc ();
				blitter_nxline ();

			} else if (c == 4) {

				/* onedot mode and no pixel = bus write access is skipped */
				if (blitlinepixel) {
					blitter_write ();
					alloc_cycle_blitter (last_blitter_hpos, &bltdpt, 4);
					record_dma_blit (0x00, blt_info.bltddat, bltdpt, last_blitter_hpos);
					blitlinepixel = 0;
				}
				if (blt_info.vblitsize == 0) {
					bltdpt = bltcpt;
					blit_final = 1;
					blit_cyclecounter = 0;
					blit_waitcyclecounter = 0;
					// blit finished bit is set and interrupt triggered
					// immediately after last D write
					blitter_interrupt (last_blitter_hpos, 0);
					break;
				}

			}

			break;
		}
		last_blitter_hpos++;
	}
	if (hsync)
		last_blitter_hpos = 0;
	reset_channel_mods ();
}

//#endif

static void actually_do_blit (void)
{
	if (blitline) {
		do {
			blitter_read ();
			if (ddat1use)
				bltdpt = bltcpt;
			ddat1use = 1;
			blitter_line ();
			blitter_line_proc ();
			blitter_nxline ();
			if (blitlinepixel) {
				blitter_write ();
				blitlinepixel = 0;
			}
			if (blt_info.vblitsize == 0)
				bltstate = BLT_done;
		} while (bltstate != BLT_done);
		bltdpt = bltcpt;
	} else {
		if (blitdesc)
			blitter_dofast_desc ();
		else
			blitter_dofast ();
		bltstate = BLT_done;
	}
}

static void blitter_doit (void)
{
	if (blt_info.vblitsize == 0 || (blitline && blt_info.hblitsize != 2)) {
		blitter_done (current_hpos());
		return;
	}
	if (log_blitter) {
		if (!blitter_dontdo)
			actually_do_blit ();
		else
			bltstate = BLT_done;
	} else {
		actually_do_blit ();
	}
	blitter_done (current_hpos ());
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
	}
	blitter_stuck = 0;
	if (blit_slowdown > 0 && !immediate_blits) {
		event2_newevent (ev2_blitter, makebliteventtime(blit_slowdown), 0);
		blit_slowdown = -1;
		return;
	}
	blitter_doit ();
}

//#ifdef CPUEMU_13

STATIC_INLINE uae_u16 blitter_doblit (void)
{
	uae_u32 blitahold;
	uae_u16 bltadat, ddat;
	uae_u8 mt = bltcon0 & 0xFF;

	bltadat = blt_info.bltadat;
	if (blitter_hcounter1 == 0)
		bltadat &= blt_info.bltafwm;
	if (blitter_hcounter1 == blt_info.hblitsize - 1)
		bltadat &= blt_info.bltalwm;
	if (blitdesc)
		blitahold = (((uae_u32)bltadat << 16) | blt_info.bltaold) >> blt_info.blitdownashift;
	else
		blitahold = (((uae_u32)blt_info.bltaold << 16) | bltadat) >> blt_info.blitashift;
	blt_info.bltaold = bltadat;

	ddat = blit_func (blitahold, blt_info.bltbhold, blt_info.bltcdat, mt) & 0xFFFF;

	if ((bltcon1 & 0x18)) {
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


STATIC_INLINE void blitter_doddma (int hpos)
{
	uae_u16 d;

	if (blit_dmacount2 == 0) {
		d = blitter_doblit ();
	} else if (ddat2use) {
		d = ddat2;
		ddat2use = 0;
	} else if (ddat1use) {
		d = ddat1;
		ddat1use = 0;
	} else {
		static int warn = 10;
		if (warn > 0) {
			warn--;
			write_log (_T("BLITTER: D-channel without nothing to do?\n"));
		}
		return;
	}
	record_dma_blit (0x00, d, bltdpt, hpos);
	//last_custom_value1 = d; blitter writes are not stored
	chipmem_agnus_wput2 (bltdpt, d);
	alloc_cycle_blitter (hpos, &bltdpt, 4);
	bltdpt += blit_add;
	blitter_hcounter2++;
	if (blitter_hcounter2 == blt_info.hblitsize) {
		blitter_hcounter2 = 0;
		bltdpt += blit_modaddd;
		blitter_vcounter2++;
		if (blit_dmacount2 == 0) // d-only
			blitter_vcounter1++;
		if (blitter_vcounter2 > blitter_vcounter1)
			blitter_vcounter1 = blitter_vcounter2;
	}
	if (blit_ch == 1)
		blitter_hcounter1 = blitter_hcounter2;
}

STATIC_INLINE void blitter_dodma (int ch, int hpos)
{
	uae_u16 dat, reg;
	uae_u32 addr;

	switch (ch)
	{
	case 1:
		blt_info.bltadat = dat = chipmem_wget_indirect (bltapt);
		last_custom_value1 = blt_info.bltadat;
		addr = bltapt;
		bltapt += blit_add;
		reg = 0x74;
		alloc_cycle_blitter (hpos, &bltapt, 1);
		break;
	case 2:
		blt_info.bltbdat = dat = chipmem_wget_indirect (bltbpt);
		last_custom_value1 = blt_info.bltbdat;
		addr = bltbpt;
		bltbpt += blit_add;
		if (blitdesc)
			blt_info.bltbhold = (((uae_u32)blt_info.bltbdat << 16) | blt_info.bltbold) >> blt_info.blitdownbshift;
		else
			blt_info.bltbhold = (((uae_u32)blt_info.bltbold << 16) | blt_info.bltbdat) >> blt_info.blitbshift;
		blt_info.bltbold = blt_info.bltbdat;
		reg = 0x72;
		alloc_cycle_blitter (hpos, &bltbpt, 2);
		break;
	case 3:
		blt_info.bltcdat = dat = chipmem_wget_indirect (bltcpt);
		last_custom_value1 = blt_info.bltcdat;
		addr = bltcpt;
		bltcpt += blit_add;
		reg = 0x70;
		alloc_cycle_blitter (hpos, &bltcpt, 3);
		break;
	default:
		abort ();
	}

	blitter_cyclecounter++;
	if (blitter_cyclecounter >= blit_dmacount2) {
		blitter_cyclecounter = 0;
		ddat2 = ddat1;
		ddat2use = ddat1use;
		ddat1use = 0;
		ddat1 = blitter_doblit ();
		if (bltcon0 & 0x100)
			ddat1use = 1;
		blitter_hcounter1++;
		if (blitter_hcounter1 == blt_info.hblitsize) {
			blitter_hcounter1 = 0;
			if (bltcon0 & 0x800)
				bltapt += blit_modadda;
			if (bltcon0 & 0x400)
				bltbpt += blit_modaddb;
			if (bltcon0 & 0x200)
				bltcpt += blit_modaddc;
			blitter_vcounter1++;
			blitfc = !!(bltcon1 & 0x4);
		}
	}
	record_dma_blit (reg, dat, addr, hpos);
}

int blitter_need (int hpos)
{
	int c;
	if (bltstate == BLT_done)
		return 0;
	if (!dmaen (DMA_BLITTER))
		return 0;
	c = channel_state (blit_cyclecounter);
	return c;
}

static void do_startcycles (int hpos)
{
	int vhpos = last_blitter_hpos;
	while (vhpos < hpos) {
		int v = canblit (vhpos);
		vhpos++;
		if (v > 0) {
			blit_startcycles--;
			if (blit_startcycles == 0) {
				if (blit_faulty)
					blit_faulty = -1;
				bltstate = BLT_done;
				blit_final = 0;
				do_blitter(vhpos, 0, blit_waitpc);
				blit_startcycles = 0;
				blit_cyclecounter = 0;
				blit_waitcyclecounter = 0;
				if (blit_faulty)
					blit_faulty = 1;
				return;
			}
		} else {
			markidlecycle (hpos);
		}
	}
}

void decide_blitter (int hpos)
{
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
		if (bltstate == BLT_done)
			return;
		if (dmaen (DMA_BLITTER))
			blitter_doit();
		return;
	}

	if (blit_startcycles > 0)
		do_startcycles (hpos);

	if (bltstate == BLT_done)
		return;

	if (!blitter_cycle_exact)
		return;

	if (hpos < 0)
		hpos = maxhpos;

	if (blitline) {
		blt_info.got_cycle = 1;
		decide_blitter_line (hsync, hpos);
		return;
	}

	while (last_blitter_hpos < hpos) {
		int c;

		c = channel_state (blit_cyclecounter);

		for (;;) {
			int v;

			v = canblit (last_blitter_hpos);

			// copper bltsize write needs one cycle (any cycle) delay
			if (blit_waitcyclecounter) {
				blit_waitcyclecounter = 0;
				markidlecycle (last_blitter_hpos);
				break;
			}
			// idle cycles require free bus.
			// Final empty cycle does not, unless it is fill mode that requires extra idle cycle
			// (CPU can still use this cycle)
#if 1
			if ((blit_cyclecounter < 0 || !blit_final || (blitfill && blit_cycle_diagram_fill[blit_ch][0])) && ((c == 0 && v == 0) || v < 0)) {
				blit_misscyclecounter++;
				break;
			}
#else
			if ((c == 0 && v == 0) || v < 0) {
				if (blit_cyclecounter < 0 || !blit_final) {
					blit_misscyclecounter++;
					break;
				}
				if (blitfill && blit_cycle_diagram_fill[blit_ch][0]) {
					blit_misscyclecounter++;
					blitter_nasty++;
					break;
				}
			}
#endif
			if (blit_frozen) {
				blit_misscyclecounter++;
				break;
			}

			if (c == 0) {
				blt_info.got_cycle = 1;
				blit_cyclecounter++;
				if (blit_cyclecounter == 0)
					blit_final = 0;
				blit_totalcyclecounter++;
				/* check if blit with zero channels has ended  */
				if (blit_ch == 0 && blit_cyclecounter >= blit_maxcyclecounter) {
					blitter_done (last_blitter_hpos);
					return;
				}
				markidlecycle (last_blitter_hpos);
				break;
			}

			if (v <= 0) {
				blit_misscyclecounter++;
				break;
			}

			blt_info.got_cycle = 1;
			if (c == 4) {
				blitter_doddma (last_blitter_hpos);
				blit_cyclecounter++;
				blit_totalcyclecounter++;
			} else {
				if (blitter_vcounter1 < blt_info.vblitsize) {
					blitter_dodma (c, last_blitter_hpos);
				}
				blit_cyclecounter++;
				blit_totalcyclecounter++;
			}

			if (blitter_vcounter1 >= blt_info.vblitsize && blitter_vcounter2 >= blt_info.vblitsize) {
				if (!ddat1use && !ddat2use) {
					blitter_done (last_blitter_hpos);
					return;
				}
			}
			// check this after end check because last D write won't cause any problems.
			check_channel_mods (last_blitter_hpos, c);
			break;
		}

		if (dmaen (DMA_BLITTER) && !blit_final && (blitter_vcounter1 == blt_info.vblitsize || (blitter_vcounter1 == blt_info.vblitsize - 1 && blitter_hcounter1 == blt_info.hblitsize - 1 && blit_dmacount2 == 0))) {
			if (channel_pos (blit_cyclecounter - 1) == blit_diag[0] - 1) {
				blitter_interrupt (last_blitter_hpos, 0);
				blit_cyclecounter = 0;
				blit_final = 1;
			}
		}
		last_blitter_hpos++;
	}
	reset_channel_mods ();
	if (hsync)
		last_blitter_hpos = 0;
}
//#else
//void decide_blitter (int hpos) { }
//#endif

static void blitter_force_finish(bool state)
{
	uae_u16 odmacon;
	if (bltstate == BLT_done)
		return;
	if (bltstate != BLT_done) {
		/* blitter is currently running
		* force finish (no blitter state support yet)
		*/
		odmacon = dmacon;
		dmacon |= DMA_MASTER | DMA_BLITTER;
		if (state)
			write_log (_T("forcing blitter finish\n"));
		if (blitter_cycle_exact && !immediate_blits) {
			int rounds = 10000;
			while (bltstate != BLT_done && rounds > 0) {
				memset (cycle_line, 0, sizeof cycle_line);
				decide_blitter (-1);
				rounds--;
			}
			if (rounds == 0)
				write_log (_T("blitter froze!?\n"));
			blit_startcycles = 0;
		} else {
			actually_do_blit ();
		}
		blitter_done (current_hpos ());
		dmacon = odmacon;
	}
}

static bool invstate (void)
{
	return bltstate != BLT_done && bltstate != BLT_init;
}

static void blit_bltset (int con)
{
	int i;
	const int *olddiag = blit_diag;

	if (con & 2) {
		blitdesc = bltcon1 & 2;
		blt_info.blitbshift = bltcon1 >> 12;
		blt_info.blitdownbshift = 16 - blt_info.blitbshift;
		if ((bltcon1 & 1) && !blitline_started) {
			write_log (_T("BLITTER: linedraw enabled after starting normal blit! %08x\n"), M68K_GETPC);
			return;
		}
	}

	if (con & 1) {
		blt_info.blitashift = bltcon0 >> 12;
		blt_info.blitdownashift = 16 - blt_info.blitashift;
	}

	blit_ch = (bltcon0 & 0x0f00) >> 8;
	blitline = bltcon1 & 1;
	blitfill = !!(bltcon1 & 0x18);

	// disable line draw if bltcon0 is written while it is active
	if (!savestate_state && bltstate != BLT_done && bltstate != BLT_init && blitline && blitline_started) {
		blitline = 0;
		bltstate = BLT_done;
		blt_info.blit_interrupt = 1;
		write_log (_T("BLITTER: register modification during linedraw! %08x\n"), M68K_GETPC);
	}

	if (blitline) {
		blit_diag = blit_cycle_diagram_line;
	} else {
		if (con & 2) {
			blitfc = !!(bltcon1 & 0x4);
			blitife = !!(bltcon1 & 0x8);
			if ((bltcon1 & 0x18) == 0x18) {
				blitife = 0;
			}
		}
		blit_diag = blitfill && blit_cycle_diagram_fill[blit_ch][0] ? blit_cycle_diagram_fill[blit_ch] : blit_cycle_diagram[blit_ch];
	}

	// on the fly switching fillmode from extra cycle to non-extra: blitter freezes
	// non-extra cycle to extra cycle: does not freeze but cycle diagram goes weird,
	// extra free cycle changes to another D write..
	// (Absolute Inebriation vector cube inside semi-filled vector object requires freezing blitter.)
	if (!savestate_state && invstate ()) {
		static int freezes = 10;
		int isen = blit_diag >= &blit_cycle_diagram_fill[0][0] && blit_diag <= &blit_cycle_diagram_fill[15][0];
		int iseo = olddiag >= &blit_cycle_diagram_fill[0][0] && olddiag <= &blit_cycle_diagram_fill[15][0];
		if (iseo != isen) {
			if (freezes > 0) {
				write_log (_T("BLITTER: on the fly %d (%d) -> %d (%d) switch! PC=%08x\n"), original_ch, iseo, blit_ch, isen, M68K_GETPC);
				freezes--;
			}
		}
		if (original_fill == isen) {
			blit_frozen = 0; // switched back to original fill mode? unfreeze
		} else if (iseo && !isen) {
			blit_frozen = 1;
			write_log (_T("BLITTER: frozen! %d (%d) -> %d (%d) %08X\n"), original_ch, iseo, blit_ch, isen, M68K_GETPC);
			//if (log_blitter & 16)
				//activate_debugger ();
		} else if (!iseo && isen) {
			if (!dmaen (DMA_BLITTER)) // subtle shades / nuance bootblock bug
				blit_frozen = 1;
			//if (log_blitter)
				//write_log (_T("BLITTER: on the fly %d (%d) -> %d (%d) switch\n"), original_ch, iseo, blit_ch, isen);
		}
	}

	// on the fly switching from CH=1 to CH=D -> blitter stops writing (Rampage/TEK)
	// currently just switch to no-channels mode, better than crashing the demo..
	if (!savestate_state && invstate ()) {
		static uae_u8 changetable[32 * 32];
		int o = original_ch + (original_fill ? 16 : 0);
		int n = blit_ch + (blitfill ? 16 : 0);
		if (o != n) {
			if (changetable[o * 32 + n] < 10) {
				changetable[o * 32 + n]++;
				write_log (_T("BLITTER: channel mode changed while active (%02X->%02X) PC=%08x\n"), o, n, M68K_GETPC);
				//if (log_blitter & 16)
					//activate_debugger ();
			}
		}
		if (blit_ch == 13 && original_ch == 1) {
			blit_faulty = 1;
		}
	}

	if (blit_faulty) {
		blit_ch = 0;
		blit_diag = blit_cycle_diagram[blit_ch];
	}

	blit_dmacount = blit_dmacount2 = 0;
	blit_nod = 1;
	for (i = 0; i < blit_diag[0]; i++) {
		int v = blit_diag[1 + blit_diag[0] + i];
		if (v <= 4)
			blit_dmacount++;
		if (v > 0 && v < 4)
			blit_dmacount2++;
		if (v == 4)
			blit_nod = 0;
	}
	if (blit_dmacount2 == 0) {
		ddat2use = 0;
		ddat1use = 0;
	}
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
	if (bltstate == BLT_done)
		return;
	if (bltcon)
		blit_bltset (bltcon);
	blit_modset ();
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
	while (bltstate != BLT_done && dmaen (DMA_BLITTER)) {
		waited = true;
		x_do_cycles (8 * CYCLE_UNIT);
	}
	if (warned && waited) {
		warned--;
		write_log (_T("waiting_blits detected PC=%08x\n"), M68K_GETPC);
	}
	if (bltstate == BLT_done)
		return true;
	return false;
}

static void blitter_start_init (void)
{
	blt_info.blitzero = 1;
	blit_frozen = 0;
	blitline_started = bltcon1 & 1;

	blit_bltset (1 | 2);
	blit_modset ();
	ddat1use = ddat2use = 0;
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
	}
	else {
		blt_info.wait_nasty = 0;
	}
}

static void do_blitter2(int hpos, int copper, uaecptr pc)
{
	int cycles;
	int cleanstart;

	if ((log_blitter & 2)) {
		if (bltstate != BLT_done) {
			if (blit_final) {
				write_log (_T("blitter was already active! PC=%08x\n"), M68K_GETPC);
				//activate_debugger();
			}
		}
	}

	cleanstart = 0;
	if (bltstate == BLT_done) {
		if (blit_faulty > 0)
			blit_faulty = 0;
		cleanstart = 1;
	}

	bltstate = BLT_done;

	blitter_cycle_exact = currprefs.blitter_cycle_exact;
	immediate_blits = currprefs.immediate_blits;
	blt_info.got_cycle = 0;
	last_blitter_hpos = hpos + 1;
	blit_firstline_cycles = blit_first_cycle = get_cycles ();
	blit_misscyclecounter = 0;
	blit_last_cycle = 0;
	blit_maxcyclecounter = 0;
	blit_cyclecounter = 0;
	blit_totalcyclecounter = 0;

	blitter_start_init ();

	if (blitline) {
		cycles = blt_info.vblitsize;
	} else {
		cycles = blt_info.vblitsize * blt_info.hblitsize;
		blit_firstline_cycles = blit_first_cycle + (blit_diag[0] * blt_info.hblitsize) * CYCLE_UNIT + cpu_cycles;
	}

	if (cleanstart) {
		original_ch = blit_ch;
		original_fill = blitfill;
		original_line = blitline;
	}

	bltstate = BLT_init;
	blit_slowdown = 0;

	unset_special (SPCFLAG_BLTNASTY);
	if (dmaen (DMA_BLITPRI))
		set_special (SPCFLAG_BLTNASTY);

	if (dmaen (DMA_BLITTER))
		bltstate = BLT_work;

	blit_maxcyclecounter = 0x7fffffff;
	blit_waitcyclecounter = 0;

	if (blitter_cycle_exact) {
		if (immediate_blits) {
			if (dmaen (DMA_BLITTER))
				blitter_doit ();
			return;
		}
		if (log_blitter & 8) {
			blitter_handler (0);
		} else {
			blitter_hcounter1 = blitter_hcounter2 = 0;
			blitter_vcounter1 = blitter_vcounter2 = 0;
			if (blit_nod)
				blitter_vcounter2 = blt_info.vblitsize;
			blit_cyclecounter = -BLITTER_STARTUP_CYCLES;
			blit_waitcyclecounter = copper;
			blit_startcycles = 0;
			blit_maxcyclecounter = blt_info.hblitsize * blt_info.vblitsize + 2;
		}
		return;
	}

	if (blt_info.vblitsize == 0 || (blitline && blt_info.hblitsize != 2)) {
		if (dmaen (DMA_BLITTER))
			blitter_done (hpos);
		return;
	}

	if (dmaen (DMA_BLITTER)) {
		blt_info.got_cycle = 1;
	}

	if (immediate_blits) {
		if (dmaen (DMA_BLITTER))
			blitter_doit ();
		return;
	}
	
	blit_cyclecounter = cycles * (blit_dmacount2 + (blit_nod ? 0 : 1));
	event2_newevent (ev2_blitter, makebliteventtime(blit_cyclecounter), 0);
}

void blitter_check_start (void)
{
	if (bltstate != BLT_init)
		return;
	blitter_start_init ();
	bltstate = BLT_work;
	if (immediate_blits) {
		blitter_doit ();
	}
}

void do_blitter(int hpos, int copper, uaecptr pc)
{
	if (bltstate == BLT_done || !blitter_cycle_exact) {
		do_blitter2(hpos, copper, pc);
		return;
	}

	if (dmaen(DMA_BLITTER) && (log_blitter & 16)) {
		//activate_debugger();
	}
	
	if (!dmaen (DMA_BLITTER) || !blt_info.got_cycle)
		return;
	// previous blit may have last write cycle left
	// and we must let it finish
	blit_startcycles = BLITTER_STARTUP_CYCLES;
	blit_waitcyclecounter = copper;
	blit_waitpc = pc;
}

void maybe_blit (int hpos, int hack)
{
	static int warned = 10;

	reset_channel_mods ();

	if (bltstate == BLT_done)
		return;

	if (savestate_state)
		return;

	if (dmaen (DMA_BLITTER) && (currprefs.cpu_model >= 68020 || !currprefs.cpu_memory_cycle_exact)) {
		bool doit = false;
		if (currprefs.waiting_blits == 3) { // always
			doit = true;
		} else if (currprefs.waiting_blits == 2) { // noidle
			if (blit_dmacount == blit_diag[0] && (regs.spcflags & SPCFLAG_BLTNASTY))
				doit = true;
		} else if (currprefs.waiting_blits == 1) { // automatic
			if (blit_dmacount == blit_diag[0] && (regs.spcflags & SPCFLAG_BLTNASTY))
				doit = true;
			else if (currprefs.m68k_speed < 0)
				doit = true;
		}
		if (doit) {
			if (waitingblits ())
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
			//activate_debugger ();
			//blitter_done (hpos);
		}
	}

	if (blitter_cycle_exact) {
		decide_blitter (hpos);
		return;
	}

	if (hack == 1 && (int)get_cycles() - (int)blit_firstline_cycles < 0)
		return;

	blitter_handler (0);
}

void check_is_blit_dangerous (uaecptr *bplpt, int planes, int words)
{
	blt_info.blitter_dangerous_bpl = 0;
	if (bltstate == BLT_done || !blitter_cycle_exact)
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
	if (bltstate == BLT_done)
		return 0;
	if (!dmaen (DMA_BLITTER))
		return 0;
	if (blitter_cycle_exact) {
		blitter_force_finish(false);
		return -1;
	}
	if (blit_last_cycle >= blit_diag[0] && blit_dmacount == blit_diag[0])
		return 0;
	cycles = (get_cycles () - blit_first_cycle) / CYCLE_UNIT;
	ccnt = 0;
	while (blit_last_cycle < cycles) {
		int c = channel_state (blit_last_cycle++);
		if (!c)
			ccnt++;
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
		int dmacycles = (linecycles * blit_dmacount) / blit_diag[0];
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
	//record_dma_reset ();
	//record_dma_reset ();
	if (blt_statefile_type == 0) {
		blt_info.blit_interrupt = 1;
		if (bltstate == BLT_init) {
			write_log (_T("blitter was started but DMA was inactive during save\n"));
		}
		if (blt_delayed_irq < 0) {
			if (intreq & 0x0040)
				blt_delayed_irq = 3;
			intreq &= ~0x0040;
		}
	} else {
		last_blitter_hpos = 0;
		blit_modset ();
	}
}

uae_u8 *restore_blitter (uae_u8 *src)
{
	uae_u32 flags = restore_u32();

	blt_statefile_type = 0;
	blt_delayed_irq = 0;
	bltstate = BLT_done;
	if (flags & 4) {
		bltstate = (flags & 1) ? BLT_done : BLT_init;
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
	if (bltstate != BLT_done && bltstate != BLT_init) {
		write_log (_T("blitter is active, forcing immediate finish\n"));
		/* blitter is active just now but we don't have blitter state support yet */
		blitter_force_finish(true);
		forced = 2;
	}
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 16);
	save_u32 (((bltstate != BLT_done) ? 0 : 1) | forced | 4);
	*len = dst - dstbak;
	return dstbak;

}

// totally non-real-blitter-like state save but better than nothing..

uae_u8 *restore_blitter_new (uae_u8 *src)
{
	uae_u8 state;
	blt_statefile_type = 1;
	blitter_cycle_exact = restore_u8 ();
	state = restore_u8 ();

	blit_first_cycle = restore_u32 ();
	blit_last_cycle = restore_u32 ();
	blit_waitcyclecounter = restore_u32 ();
	blit_startcycles = restore_u32 ();
	blit_maxcyclecounter = restore_u32 ();
	blit_firstline_cycles = restore_u32 ();
	blit_cyclecounter = restore_u32 ();
	blit_slowdown = restore_u32 ();
	blit_misscyclecounter = restore_u32 ();

	blitter_hcounter1 = restore_u16 ();
	blitter_hcounter2 = restore_u16 ();
	blitter_vcounter1 = restore_u16 ();
	blitter_vcounter2 = restore_u16 ();
	blit_ch = restore_u8 ();
	blit_dmacount = restore_u8 ();
	blit_dmacount2 = restore_u8 ();
	blit_nod = restore_u8 ();
	blit_final = restore_u8 ();
	blitfc = restore_u8 ();
	blitife = restore_u8 ();

	blt_info.blitbshift = restore_u8 ();
	blt_info.blitdownbshift = restore_u8 ();
	blt_info.blitashift = restore_u8 ();
	blt_info.blitdownashift = restore_u8 ();

	ddat1use = restore_u8 ();
	ddat2use = restore_u8 ();
	ddat1 = restore_u16 ();
	ddat2 = restore_u16 ();

	blitline = restore_u8 ();
	blitfill = restore_u8 ();
	blinea = restore_u16 ();
	blineb = restore_u16 ();
	blinea_shift = restore_u8 ();
	blitonedot = restore_u8 ();
	blitlinepixel = restore_u8 ();
	blitsing = restore_u8 ();
	blitlinepixel = restore_u8 ();
	blt_info.blit_interrupt = restore_u8 ();
	blt_delayed_irq = restore_u8 ();
	blt_info.blitzero = restore_u8 ();
	blt_info.got_cycle = restore_u8 ();

	blit_frozen = restore_u8 ();
	blit_faulty = restore_u8 ();
	original_ch = restore_u8 ();
	original_fill = restore_u8 ();
	original_line = restore_u8 ();

	blit_diag = set_cycle_diagram_type (restore_u8 ());

	if (restore_u16 () != 0x1234)
		write_log (_T("error\n"));

	blt_info.blitter_nasty = restore_u8 ();

	bltstate = BLT_done;
	if (!blitter_cycle_exact) {
		if (state > 0)
			do_blitter(0, 0, 0);
	} else {
		if (state == 1)
			bltstate = BLT_init;
		else if (state == 2)
			bltstate = BLT_work;
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
	save_u8 (blitter_cycle_exact ? 1 : 0);
	if (bltstate == BLT_done)
		state = 0;
	else if (bltstate == BLT_init)
		state = 1;
	else
		state = 2;
	save_u8 (state);

	if (bltstate != BLT_done) {
		write_log (_T("BLITTER active while saving state\n"));
	}

	save_u32 (blit_first_cycle);
	save_u32 (blit_last_cycle);
	save_u32 (blit_waitcyclecounter);
	save_u32 (blit_startcycles);
	save_u32 (blit_maxcyclecounter);
	save_u32 (blit_firstline_cycles);
	save_u32 (blit_cyclecounter);
	save_u32 (blit_slowdown);
	save_u32 (blit_misscyclecounter);

	save_u16 (blitter_hcounter1);
	save_u16 (blitter_hcounter2);
	save_u16 (blitter_vcounter1);
	save_u16 (blitter_vcounter2);
	save_u8 (blit_ch);
	save_u8 (blit_dmacount);
	save_u8 (blit_dmacount2);
	save_u8 (blit_nod);
	save_u8 (blit_final);
	save_u8 (blitfc);
	save_u8 (blitife);

	save_u8 (blt_info.blitbshift);
	save_u8 (blt_info.blitdownbshift);
	save_u8 (blt_info.blitashift);
	save_u8 (blt_info.blitdownashift);

	save_u8 (ddat1use);
	save_u8 (ddat2use);
	save_u16 (ddat1);
	save_u16 (ddat2);

	save_u8 (blitline);
	save_u8 (blitfill);
	save_u16 (blinea);
	save_u16 (blineb);
	save_u8 (blinea_shift);
	save_u8 (blitonedot);
	save_u8 (blitlinepixel);
	save_u8 (blitsing);
	save_u8 (blitlinepixel);
	save_u8 (blt_info.blit_interrupt);
	save_u8 (blt_delayed_irq);
	save_u8 (blt_info.blitzero);
	save_u8 (blt_info.got_cycle);
	
	save_u8 (blit_frozen);
	save_u8 (blit_faulty);
	save_u8 (original_ch);
	save_u8 (original_fill);
	save_u8 (original_line);
	save_u8 (get_cycle_diagram_type (blit_diag));

	save_u16 (0x1234);

	save_u8 (blt_info.blitter_nasty);

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */
