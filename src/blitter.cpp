 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Custom chip emulation
  *
  * (c) 1995 Bernd Schmidt, Alessandro Bissacco
  * (c) 2002 - 2005 Toni Wilen
  */

#define SPEEDUP 1

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "events.h"
#include "savestate.h"
#include "blitter.h"
#include "blit.h"

static int blt_statefile_type;

uae_u16 bltcon0, bltcon1;
uae_u32 bltapt, bltbpt, bltcpt, bltdpt;

static int original_ch;

static int blinea_shift;
static uae_u16 blinea, blineb;
static int blitline, blitfc, blitfill, blitife, blitsing, blitdesc;
static int blitline_started;
static int blitonedot, blitsign;
static int blit_ch;

struct bltinfo blt_info;

static uae_u8 blit_filltable[256][4][2];
uae_u32 blit_masktable[BLITTER_MAX_WORDS];
enum blitter_states bltstate;

static int blit_cyclecounter;
static int blit_slowdown;

static long blit_firstline_cycles;
static long blit_first_cycle;
static int blit_last_cycle, blit_dmacount, blit_dmacount2;
static int blit_nod;
static const int *blit_diag;
static int blit_faulty;
static int ddat1use;

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

static void blitter_dump (void)
{
	int chipsize = currprefs.chipmem_size;
	write_log (_T("PT A=%08X B=%08X C=%08X D=%08X\n"), bltapt, bltbpt, bltcpt, bltdpt);
	write_log (_T("CON0=%04X CON1=%04X DAT A=%04X B=%04X C=%04X\n"),
		bltcon0, bltcon1, blt_info.bltadat, blt_info.bltbdat, blt_info.bltcdat);
	write_log (_T("AFWM=%04X ALWM=%04X MOD A=%04X B=%04X C=%04X D=%04X\n"),
		blt_info.bltafwm, blt_info.bltalwm,
		blt_info.bltamod & 0xffff, blt_info.bltbmod & 0xffff, blt_info.bltcmod & 0xffff, blt_info.bltdmod & 0xffff);
	write_log (_T("PC=%08X DMA=%d\n"), m68k_getpc (), dmaen (DMA_BLITTER));

	if (((bltcon0 & 0x800) && bltapt >= chipsize) || ((bltcon0 & 0x400) && bltbpt >= chipsize) ||
		((bltcon0 & 0x200) && bltcpt >= chipsize) || ((bltcon0 & 0x100) && bltdpt >= chipsize))
		write_log (_T("PT outside of chipram\n"));
}

STATIC_INLINE int channel_state (int cycles)
{
	if (cycles < blit_diag[0])
		return blit_diag[1 + cycles];
	cycles -= blit_diag[0];
	cycles %= blit_diag[0];
	return blit_diag[1 + blit_diag[0] + cycles];
}

static void blitter_done (void)
{
	ddat1use = 0;
	bltstate = BLT_done;
	send_interrupt (6);
	blitter_done_notify ();
  event_remevent(ev_blitter);
	unset_special (SPCFLAG_BLTNASTY);
}

STATIC_INLINE void chipmem_agnus_wput2 (uaecptr addr, uae_u32 w)
{
	chipmem_wput_indirect (addr, w);
}

static void blitter_dofast(void)
{
  int i,j;
  uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
  uae_u8 mt = bltcon0 & 0xFF;

  blit_masktable[BLITTER_MAX_WORDS - 1] = blt_info.bltafwm;
  blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] &= blt_info.bltalwm;

  if (bltcon0 & 0x800) {
	  bltadatptr = (uaecptr)get_real_address(bltapt);
	  bltapt += (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x400) {
	  bltbdatptr = (uaecptr)get_real_address(bltbpt);
	  bltbpt += (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x200) {
	  bltcdatptr = (uaecptr)get_real_address(bltcpt);
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
    uae_u32 preva = 0, prevb = 0;
	  uaecptr dstp = 0;
	  uae_u32 *blit_masktable_p = blit_masktable + BLITTER_MAX_WORDS - blt_info.hblitsize;

  	for (j = blt_info.vblitsize; j--;) {
	    blitfc = !!(bltcon1 & 0x4);
	    for (i = blt_info.hblitsize; i--;) {
		    uae_u32 bltadat, blitahold;
		    if (bltadatptr) {
		      blt_info.bltadat = bltadat = do_get_mem_word ((uae_u16 *)bltadatptr);
		      bltadatptr += 2;
    		} else
  		    bltadat = blt_info.bltadat;
		    bltadat &= blit_masktable_p[i];
		    blitahold = (((uae_u32)preva << 16) | bltadat) >> blt_info.blitashift;
		    preva = bltadat;

    		if (bltbdatptr) {
		      uae_u16 bltbdat;
		      blt_info.bltbdat = bltbdat = do_get_mem_word ((uae_u16 *)bltbdatptr);
		      bltbdatptr += 2;
		      blitbhold = (((uae_u32)prevb << 16) | bltbdat) >> blt_info.blitbshift;
		      prevb = bltbdat;
		    }
    		if (bltcdatptr) {
		      blt_info.bltcdat = do_get_mem_word ((uae_u16 *)bltcdatptr);
		      bltcdatptr += 2;
    		}
    		if (dstp) 
    		  chipmem_agnus_wput2 (dstp, blt_info.bltddat);
    		blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt);
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
	  if (dstp)
	    chipmem_agnus_wput2 (dstp, blt_info.bltddat);
	  blt_info.bltbhold = blitbhold;
  }
  blit_masktable[BLITTER_MAX_WORDS - 1] = 0xFFFF;
  blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] = 0xFFFF;

  bltstate = BLT_done;
}

static void blitter_dofast_desc(void)
{
  int i,j;
  uaecptr bltadatptr = 0, bltbdatptr = 0, bltcdatptr = 0, bltddatptr = 0;
  uae_u8 mt = bltcon0 & 0xFF;

  blit_masktable[BLITTER_MAX_WORDS - 1] = blt_info.bltafwm;
  blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] &= blt_info.bltalwm;

  if (bltcon0 & 0x800) {
	  bltadatptr = (uaecptr)get_real_address(bltapt);
	  bltapt -= (blt_info.hblitsize * 2 + blt_info.bltamod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x400) {
	  bltbdatptr = (uaecptr)get_real_address(bltbpt);
	  bltbpt -= (blt_info.hblitsize * 2 + blt_info.bltbmod) * blt_info.vblitsize;
  }
  if (bltcon0 & 0x200) {
	  bltcdatptr = (uaecptr)get_real_address(bltcpt);
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
    uae_u32 preva = 0, prevb = 0;
	  uaecptr dstp = 0;
	  uae_u32 *blit_masktable_p = blit_masktable + BLITTER_MAX_WORDS - blt_info.hblitsize;

	  for (j = blt_info.vblitsize; j--;) {
			blitfc = !!(bltcon1 & 0x4);
	    for (i = blt_info.hblitsize; i--;) {
				uae_u32 bltadat, blitahold;
				if (bltadatptr) {
		      bltadat = blt_info.bltadat = do_get_mem_word ((uae_u16 *)bltadatptr);
					bltadatptr -= 2;
				} else
					bltadat = blt_info.bltadat;
				bltadat &= blit_masktable_p[i];
				blitahold = (((uae_u32)bltadat << 16) | preva) >> blt_info.blitdownashift;
				preva = bltadat;

				if (bltbdatptr) {
		      uae_u16 bltbdat;
		      blt_info.bltbdat = bltbdat = do_get_mem_word ((uae_u16 *)bltbdatptr);
					  bltbdatptr -= 2;
					  blitbhold = (((uae_u32)bltbdat << 16) | prevb) >> blt_info.blitdownbshift;
					  prevb = bltbdat;
				  }

				  if (bltcdatptr) {
    		    blt_info.bltcdat = blt_info.bltbdat = do_get_mem_word ((uae_u16 *)bltcdatptr);
  					bltcdatptr -= 2;
  				}
  				if (dstp)
      		  chipmem_agnus_wput2 (dstp, blt_info.bltddat);
      		blt_info.bltddat = blit_func (blitahold, blitbhold, blt_info.bltcdat, mt);
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
	  if (dstp)
	    chipmem_agnus_wput2 (dstp, blt_info.bltddat);
		blt_info.bltbhold = blitbhold;
	}
  blit_masktable[BLITTER_MAX_WORDS - 1] = 0xFFFF;
  blit_masktable[BLITTER_MAX_WORDS - blt_info.hblitsize] = 0xFFFF;

  bltstate = BLT_done;
}

STATIC_INLINE void blitter_read(void)
{
	if (bltcon0 & 0x200) {
    blt_info.bltcdat = chipmem_wget_indirect(bltcpt);
	}
}

STATIC_INLINE void blitter_write(void)
{
	if (blt_info.bltddat)
		blt_info.blitzero = 0;
	/* D-channel state has no effect on linedraw, but C must be enabled or nothing is drawn! */
	if (bltcon0 & 0x200) {
    chipmem_wput_indirect (bltdpt, blt_info.bltddat);
	}
}

STATIC_INLINE void blitter_line_incx(void)
{
  if (++blinea_shift == 16) {
  	blinea_shift = 0;
  	bltcpt += 2;
  }
}

STATIC_INLINE void blitter_line_decx(void)
{
  if (blinea_shift-- == 0) {
	  blinea_shift = 15;
	  bltcpt -= 2;
  }
}

STATIC_INLINE void blitter_line_decy(void)
{
  bltcpt -= blt_info.bltcmod;
  blitonedot = 0;
}

STATIC_INLINE void blitter_line_incy(void)
{
  bltcpt += blt_info.bltcmod;
  blitonedot = 0;
}

static int blitter_line(void)
{
	uae_u16 blitahold = (blinea & blt_info.bltafwm) >> blinea_shift;

	blt_info.bltbhold = (blineb & 1) ? 0xFFFF : 0;
	int blitlinepixel = !blitsing || (blitsing && !blitonedot);
	blt_info.bltddat = blit_func (blitahold, blt_info.bltbhold, blt_info.bltcdat, bltcon0 & 0xFF);
	blitonedot++;

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
	return blitlinepixel;
}

STATIC_INLINE void blitter_nxline(void)
{
	blineb = (blineb << 1) | (blineb >> 15);
	blt_info.vblitsize--;
}

static void actually_do_blit(void)
{
  if (blitline) {
  	do {
			blitter_read ();
			if (ddat1use)
				bltdpt = bltcpt;
			ddat1use = 1;
			if (blitter_line ()) {
				blitter_write ();
			}
			blitter_nxline ();
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
	actually_do_blit ();
	blitter_done ();
}

void blitter_handler(void)
{
	static int blitter_stuck;

	if (!dmaen (DMA_BLITTER)) {
	  event_newevent (ev_blitter, 10);
		blitter_stuck++;
		if (blitter_stuck < 20000 || !currprefs.immediate_blits)
			return; /* gotta come back later. */
		/* "free" blitter in immediate mode if it has been "stuck" ~3 frames
		* fixes some JIT game incompatibilities
		*/
	}
	blitter_stuck = 0;
	if (blit_slowdown > 0 && !currprefs.immediate_blits) {
	  event_newevent (ev_blitter, blit_slowdown);
		blit_slowdown = -1;
		return;
	}
	blitter_doit ();
}

static void blitter_force_finish (void)
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
	  write_log (_T("forcing blitter finish\n"));
	  actually_do_blit ();
	  blitter_done ();
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

	if (con & 2) {
		blitdesc = bltcon1 & 2;
		blt_info.blitbshift = bltcon1 >> 12;
		blt_info.blitdownbshift = 16 - blt_info.blitbshift;
		if ((bltcon1 & 1) && !blitline_started) {
			write_log (_T("BLITTER: linedraw enabled after starting normal blit! %08x\n"), M68K_GETPC);
			return;
		}
    if (bltstate != BLT_done) {
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

	// on the fly switching from CH=1 to CH=D -> blitter stops writing (Rampage/TEK)
	// currently just switch to no-channels mode, better than crashing the demo..
	if (!savestate_state && invstate ()) {
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
		ddat1use = 0;
	}
}

void reset_blit (int bltcon)
{
  if (bltstate == BLT_done)
  	return;
	blit_bltset (bltcon);
}

static bool waitingblits (void)
{
	bool waited = false;
	while (bltstate != BLT_done && dmaen (DMA_BLITTER)) {
		waited = true;
		x_do_cycles (8 * CYCLE_UNIT);
	}
	if (bltstate == BLT_done)
		return true;
	return false;
}

static void blitter_start_init (void)
{
	blt_info.blitzero = 1;
	blitline_started = bltcon1 & 1;

	blit_bltset (1 | 2);
	ddat1use = 0;

	if (blitline) {
    blinea_shift = bltcon0 >> 12;
		blinea = blt_info.bltadat;
		blineb = (blt_info.bltbdat >> blt_info.blitbshift) | (blt_info.bltbdat << (16 - blt_info.blitbshift));
		blitonedot = 0;
		blitsing = bltcon1 & 0x2;
    blitsign = bltcon1 & 0x40;
	}
}

void do_blitter ()
{
	int cycles;
	int cleanstart;

	cleanstart = 0;
	if (bltstate == BLT_done) {
		blit_faulty = 0;
		cleanstart = 1;
	}

	bltstate = BLT_done;

	blit_firstline_cycles = blit_first_cycle = get_cycles ();
	blit_last_cycle = 0;
	blit_cyclecounter = 0;

	blitter_start_init ();

	if (blitline) {
		cycles = blt_info.vblitsize;
	} else {
		cycles = blt_info.vblitsize * blt_info.hblitsize;
		blit_firstline_cycles = blit_first_cycle + (blit_diag[0] * blt_info.hblitsize) * CYCLE_UNIT + cpu_cycles;
	}

	if (cleanstart) {
		original_ch = blit_ch;
	}

  bltstate = BLT_init;
	blit_slowdown = 0;

  if (dmaen(DMA_BLITPRI))
    set_special (SPCFLAG_BLTNASTY);
  else
  	unset_special (SPCFLAG_BLTNASTY);

	if (blt_info.vblitsize == 0 || (blitline && blt_info.hblitsize != 2)) {
		if (dmaen (DMA_BLITTER))
		  blitter_done ();
		return;
	}

  blit_cyclecounter = cycles * (blit_dmacount2 + (blit_nod ? 0 : 1)); 
	if (!dmaen (DMA_BLITTER))
    return;

	bltstate = BLT_work;

	if (currprefs.immediate_blits) {
		if (dmaen (DMA_BLITTER))
	    blitter_doit ();
    return;
	}

  event_newevent(ev_blitter, blit_cyclecounter);

	if (dmaen (DMA_BLITTER)) {
		if (currprefs.waiting_blits) {
			// wait immediately if all cycles in use and blitter nastry
			if (blit_dmacount == blit_diag[0] && (regs.spcflags & SPCFLAG_BLTNASTY)) {
				waitingblits ();
			}
		}
	}
}

void blitter_check_start (void)
{
	blitter_start_init ();
	bltstate = BLT_work;

	if (currprefs.immediate_blits) {
		blitter_doit ();
	} else {
    event_newevent(ev_blitter, blit_cyclecounter);
  }
}

void maybe_blit2 (int hack)
{
	if (dmaen (DMA_BLITTER)) {
		bool doit = false;
		if (currprefs.waiting_blits) { // automatic
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

  if (hack == 1 && get_cycles() < blit_firstline_cycles)
  	return;

  blitter_handler ();
}

int blitnasty (void)
{
	int cycles, ccnt;
	if (bltstate == BLT_done)
		return 0;
	if (!dmaen (DMA_BLITTER))
		return 0;
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
}

#ifdef SAVESTATE

uae_u8 *restore_blitter (uae_u8 *src)
{
  uae_u32 flags = restore_u32();

	blt_statefile_type = 0;
	bltstate = BLT_done;
	if (flags & 4) {
    bltstate = (flags & 1) ? BLT_done : BLT_init;
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
	  blitter_force_finish ();
	  forced = 2;
  }
  if (dstptr)
  	dstbak = dst = dstptr;
  else
    dstbak = dst = xmalloc (uae_u8, 16);
  save_u32(((bltstate != BLT_done) ? 0 : 1) | forced | 4);
  *len = dst - dstbak;
  return dstbak;
}

// totally non-real-blitter-like state save but better than nothing..

uae_u8 *restore_blitter_new (uae_u8 *src)
{
	uae_u8 state;
	blt_statefile_type = 1;
	state = restore_u8 ();

	blit_first_cycle = restore_u32 ();
	blit_last_cycle = restore_u32 ();
	blit_firstline_cycles = restore_u32 ();
	blit_cyclecounter = restore_u32 ();
	blit_slowdown = restore_u32 ();

	blit_ch = restore_u8 ();
	blit_dmacount = restore_u8 ();
	blit_dmacount2 = restore_u8 ();
	blit_nod = restore_u8 ();
	blitfc = restore_u8 ();
	blitife = restore_u8 ();

	blt_info.blitbshift = restore_u8 ();
	blt_info.blitdownbshift = restore_u8 ();
	blt_info.blitashift = restore_u8 ();
	blt_info.blitdownashift = restore_u8 ();

	ddat1use = restore_u8 ();

	blitline = restore_u8 ();
	blitfill = restore_u8 ();
	blinea = restore_u16 ();
	blineb = restore_u16 ();
	blinea_shift = restore_u8 ();
	blitonedot = restore_u8 ();
	blitsing = restore_u8 ();
	blt_info.blitzero = restore_u8 ();

	blit_faulty = restore_u8 ();
	original_ch = restore_u8 ();

	blit_diag = set_cycle_diagram_type (restore_u8 ());

	if (restore_u16 () != 0x1234)
		write_log (_T("error\n"));

	bltstate = BLT_done;
	if (state > 0)
		do_blitter ();
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
	if (bltstate == BLT_done)
		state = 0;
	else if (bltstate == BLT_init)
		state = 1;
	else
		state = 2;
	save_u8 (state);

	if (bltstate != BLT_done) {
		write_log (_T("BLITTER active while saving state\n"));
		blitter_dump ();
	}

	save_u32 (blit_first_cycle);
	save_u32 (blit_last_cycle);
	save_u32 (blit_firstline_cycles);
	save_u32 (blit_cyclecounter);
	save_u32 (blit_slowdown);

	save_u8 (blit_ch);
	save_u8 (blit_dmacount);
	save_u8 (blit_dmacount2);
	save_u8 (blit_nod);
	save_u8 (blitfc);
	save_u8 (blitife);

	save_u8 (blt_info.blitbshift);
	save_u8 (blt_info.blitdownbshift);
	save_u8 (blt_info.blitashift);
	save_u8 (blt_info.blitdownashift);

	save_u8 (ddat1use);

	save_u8 (blitline);
	save_u8 (blitfill);
	save_u16 (blinea);
	save_u16 (blineb);
	save_u8 (blinea_shift);
	save_u8 (blitonedot);
	save_u8 (blitsing);
	save_u8 (blt_info.blitzero);
	
	save_u8 (blit_faulty);
	save_u8 (original_ch);
	save_u8 (get_cycle_diagram_type (blit_diag));

	save_u16 (0x1234);

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */
