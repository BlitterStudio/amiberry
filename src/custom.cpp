 /*
* UAE - The Un*x Amiga Emulator
*
* Custom chip emulation
*
* Copyright 1995-2002 Bernd Schmidt
* Copyright 1995 Alessandro Bissacco
* Copyright 2000-2015 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <cctype>
#include <cmath>

#include "options.h"
#include "uae.h"
#include "audio.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cia.h"
#include "disk.h"
#include "savestate.h"
#include "blitter.h"
#include "xwin.h"
#include "inputdevice.h"
#include "autoconf.h"
#include "traps.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "ar.h"
#include "akiko.h"
#include "devices.h"
#include "rommgr.h"

#define SPR0_HPOS 0x15
#define MAX_SPRITES 8

STATIC_INLINE bool nocustom (void)
{
	return picasso_on;
}

static void uae_abort (const TCHAR *format,...)
{
	static int nomore;
	va_list parms;
	TCHAR buffer[1000];

	va_start (parms, format);
	_vsntprintf (buffer, sizeof (buffer) - 1, format, parms );
	va_end (parms);
	if (nomore) {
		write_log (_T("%s\n"), buffer);
		return;
	}
	gui_message (buffer);
	nomore = 1;
}

#ifdef TINKER

// Alynna //
// Justifications for the numbers set here
// Frametime is 20000 cycles in PAL
//              16667 cycles in NTSC
// The most we can give back is a frame's worth of
// cycles, but we shouldn't give them ALL back for timing
// coordination so I have picked 90% of the cycles.
#define SPEEDUP_CYCLES_JIT_PAL     18000
#define SPEEDUP_CYCLES_JIT_NTSC    15000
#define SPEEDUP_CYCLES_NONJIT       1024
// These are set to give enough time to hit 60 fps
// on the ASUS tinker board, and giving the rest of the
// time to the CPU (minus the 10% reserve above).
// As these numbers are decreased towards -(SPEEDUP_CYCLES),
// more chipset time is given and the CPU gets slower.
// For your platform its best to tune these to the point where
// you hit 60FPS in NTSC and not higher.
// Do not tune above -(SPEEDUP_CYCLES) or the emulation will
// become unstable.
#define SPEEDUP_TIMELIMIT_JIT      -10000
#define SPEEDUP_TIMELIMIT_NONJIT    -960
#else
#define SPEEDUP_CYCLES_JIT_PAL 10000
#define SPEEDUP_CYCLES_JIT_NTSC 6667
#define SPEEDUP_CYCLES_NONJIT 256
#define SPEEDUP_TIMELIMIT_JIT -5000
#define SPEEDUP_TIMELIMIT_NONJIT -5000
#endif

// These define the maximum CPU possible and work well with
// frameskip on and operation at 30fps at full chipset speed
// They give the minimum possible chipset time.  Do not make
// these positive numbers.  Doing so may give you a 500mhz
// 68040 but your emulation will not be able to reset at this
// speed.
#define SPEEDUP_TIMELIMIT_JIT_TURBO      0
#define SPEEDUP_TIMELIMIT_NONJIT_TURBO   0

int pissoff_value = SPEEDUP_CYCLES_JIT_PAL * CYCLE_UNIT;
int speedup_timelimit = SPEEDUP_TIMELIMIT_JIT;

/* Events */

static int rpt_did_reset;
struct ev eventtab[ev_max];
struct ev2 eventtab2[ev2_max];

int vpos;
static int vpos_count, vpos_count_diff;
static int lof_store; // real bit in custom registers
static int lof_current; // what display device thinks
static bool lof_lastline, lof_prev_lastline;
static const int lol = 0;
static int next_lineno, prev_lineno;
static int lof_changed = 0, lof_changing = 0, interlace_changed = 0;
static int lof_changed_previous_field;
static int vposw_change;
static bool lof_lace;
static bool vsync_rendered, frame_rendered, frame_shown;
static int vsynctimeperline;
static int frameskiptime;
static int cia_hsync;

#define LOF_TOGGLES_NEEDED 3
static int lof_togglecnt_lace, lof_togglecnt_nlace;

static int vpos_previous, hpos_previous;

static uae_u32 sprtaba[256],sprtabb[256];
static uae_u32 sprite_ab_merge[256];

/* Tables for collision detection.  */
static uae_u32 sprclx[16], clxmask[16];

/*
 * Hardware registers of all sorts.
 */

static int REGPARAM3 custom_wput_1 (int, uaecptr, uae_u32, int) REGPARAM;

static uae_u16 intena;
uae_u16 intreq;
uae_u16 dmacon;
uae_u16 adkcon; /* used by audio code */
uae_u32 last_custom_value1;

static uae_u32 cop1lc,cop2lc,copcon;
 
int maxhpos = MAXHPOS_PAL;
int maxvpos = MAXVPOS_PAL;
int maxvpos_nom = MAXVPOS_PAL; // nominal value (same as maxvpos but "faked" maxvpos in fake 60hz modes)
int maxvpos_display = MAXVPOS_PAL; // value used for display size
int hsyncendpos, hsyncstartpos;
static int maxvpos_total = 511;
int minfirstline = VBLANK_ENDLINE_PAL;
static int equ_vblank_endline = EQU_ENDLINE_PAL;
static bool equ_vblank_toggle = true;
double vblank_hz = VBLANK_HZ_PAL, fake_vblank_hz, vblank_hz_nom;
double hblank_hz;
static float vblank_hz_lof, vblank_hz_shf, vblank_hz_lace;
static struct chipset_refresh *stored_chipset_refresh;
int syncbase;
static int fmode_saved, fmode;
static uae_u16 beamcon0, new_beamcon0;
static bool varsync_changed;
static uae_u16 vtotal = MAXVPOS_PAL, htotal = MAXHPOS_PAL;
static uae_u16 hsstop, hbstrt, hbstop, vsstop, vbstrt, vbstop, hsstrt, vsstrt, hcenter;
static int diw_hstrt, diw_hstop;
static int diw_hcounter;

#define HSYNCTIME (maxhpos * CYCLE_UNIT)

struct sprite {
  uaecptr pt;
  int xpos;
  int vstart;
  int vstop;
	int dblscan; /* AGA SSCAN2 */
  int armed;
  int dmastate;
  int dmacycle;
};

static struct sprite spr[MAX_SPRITES];
static int plfstrt_sprite;

static int sprite_vblank_endline = VBLANK_SPRITE_PAL;

static uae_u16 sprctl[MAX_SPRITES], sprpos[MAX_SPRITES];
static uae_u16 sprdata[MAX_SPRITES][4], sprdatb[MAX_SPRITES][4];
static int last_sprite_point, nr_armed;
static int sprite_width, sprres;
int sprite_buffer_res;

static bool bpl1dat_written, bpl1dat_written_at_least_once;
static bool bpldmawasactive;
static uae_s16 bpl1mod, bpl2mod, dbpl1mod, dbpl2mod;
static int dbpl1mod_on, dbpl2mod_on;
static uaecptr bplpt[8];

static struct color_entry current_colors;
static unsigned int bplcon0;
static unsigned int bplcon1, bplcon2, bplcon3, bplcon4;
static unsigned int bplcon0d, bplcon0dd, bplcon0_res, bplcon0_planes, bplcon0_planes_limit;
static unsigned int diwstrt, diwstop, diwhigh;
static int diwhigh_written;
static unsigned int ddfstrt, ddfstop;
static int line_cyclebased, badmode;
static int hpos_is_zero_bplcon1_hack = -1;

#define SET_LINE_CYCLEBASED line_cyclebased = 2;

/* The display and data fetch windows */

enum diw_states
{
	DIW_waiting_start, DIW_waiting_stop
};

static int plffirstline, plflastline;
static int plfstrt, plfstop;
static int sprite_minx, sprite_maxx;
static int last_decide_line_hpos;
static int last_fetch_hpos, last_sprite_hpos;
static int diwfirstword, diwlastword;
static int last_hdiw;
static enum diw_states diwstate, hdiwstate, ddfstate;
static int bpl_hstart;

static int last_copper_hpos;
static int copper_access;

/* Sprite collisions */
static unsigned int clxdat, clxcon, clxcon2, clxcon_bpl_enable, clxcon_bpl_match;

enum copper_states {
  COP_stop,
	COP_waitforever,
  COP_read1,
  COP_read2,
  COP_bltwait,
  COP_wait_in2,
  COP_skip_in2,
  COP_wait1,
  COP_wait,
  COP_skip1,
  COP_strobe_delay1,
  COP_strobe_delay2,
	COP_strobe_delay1x,
	COP_strobe_delay2x,
	COP_strobe_extra, // just to skip current cycle when CPU wrote to COPJMP
	COP_start_delay
};

struct copper {
  /* The current instruction words.  */
  unsigned int i1, i2;
  enum copper_states state, state_prev;
  /* Instruction pointer.  */
  uaecptr ip;
  int hpos;
  unsigned int ignore_next;
  int vcmp, hcmp;

  /* When we schedule a copper event, knowing a few things about the future
     of the copper list can reduce the number of sync_with_cpu calls
     dramatically.  */
  unsigned int first_sync;
  unsigned int regtypes_modified;

  int strobe; /* COPJMP1 / COPJMP2 accessed */
  int moveaddr, movedata, movedelay;
};

#define REGTYPE_NONE 0
#define REGTYPE_COLOR 1
#define REGTYPE_SPRITE 2
#define REGTYPE_PLANE 4
#define REGTYPE_BLITTER 8
#define REGTYPE_JOYPORT 16
#define REGTYPE_DISK 32
#define REGTYPE_POS 64
#define REGTYPE_AUDIO 128

#define REGTYPE_ALL 255
/* Always set in regtypes_modified, to enable a forced update when things like
   DMACON, BPLCON0, COPJMPx get written.  */
#define REGTYPE_FORCE 256

static unsigned int regtypes[512];

static struct copper cop_state;
static int copper_enabled_thisline;

/*
 * Statistics
 */
static unsigned long lastframetime = 0;
unsigned long timeframes = 0;
unsigned long hsync_counter = 0;
unsigned long int idletime;

/* Recording of custom chip register changes.  */
struct sprite_entry *curr_sprite_entries = 0;
struct color_change *curr_color_changes = 0;

struct decision line_decisions[2 * (MAXVPOS + 2) + 1];
struct draw_info curr_drawinfo[2 * (MAXVPOS + 2) + 1];
struct color_entry curr_color_tables[COLOR_TABLE_SIZE];

static int next_sprite_entry = 0;
static int next_sprite_forced = 1;

static int next_color_change;
static int next_color_entry, remembered_color_entry;

static struct decision thisline_decision;
static int fetch_cycle, fetch_modulo_cycle;
static int aga_plf_passed_stop2;
static int plf_start_hpos, plf_end_hpos;
static int ddfstop_written_hpos;
static int bitplane_off_delay;
static bool ocs_agnus_ddf_enable_toggle;
static int bpl_dma_off_when_active;
static int bitplane_maybe_start_hpos;
static bool ddfstop_matched;

enum plfstate
{
  plf_idle,
	// enable passed
	plf_passed_enable,
	// ddfstrt match
	plf_passed_start,
	// active (ddfstrt + 4 match)
	plf_active,
	// inactive, waiting
	plf_wait,
	// ddfstop passed
	plf_passed_stop,
	// ddfstop+4 passed
	plf_passed_stop_act,
	// last block finished
	plf_passed_stop2,
	plf_end
} plf_state;

enum plfrenderstate
{
	plfr_idle,
	plfr_active,
	plfr_end,
	plfr_finished
} plfr_state;

enum fetchstate {
  fetch_not_started,
	fetch_started_first,
  fetch_started,
  fetch_was_plane0
} fetch_state;

/*
 * helper functions
 */

STATIC_INLINE int ecsshres(void)
{
	return bplcon0_res == RES_SUPERHIRES && (currprefs.chipset_mask & CSMASK_ECS_DENISE) && !(currprefs.chipset_mask & CSMASK_AGA);
}

STATIC_INLINE int nodraw (void)
{
	return framecnt != 0;
}

void set_speedup_values(void)
{
	if (currprefs.m68k_speed < 0) {
		if (currprefs.cachesize) {
			pissoff_value = ((vblank_hz > 55) ? SPEEDUP_CYCLES_JIT_NTSC : SPEEDUP_CYCLES_JIT_PAL) * CYCLE_UNIT;
			if (currprefs.m68k_speed != -30)
				speedup_timelimit = SPEEDUP_TIMELIMIT_JIT;
			else
				speedup_timelimit = SPEEDUP_TIMELIMIT_JIT_TURBO;
		}
		else {
			pissoff_value = SPEEDUP_CYCLES_NONJIT * CYCLE_UNIT;
			if (currprefs.m68k_speed != -30)
				speedup_timelimit = SPEEDUP_TIMELIMIT_NONJIT;
			else
				speedup_timelimit = SPEEDUP_TIMELIMIT_NONJIT_TURBO;
		}
	}
	else {
		pissoff_value = 0;
		speedup_timelimit = 0;
	}
}

void reset_frame_rate_hack (void)
{
  if (currprefs.m68k_speed >= 0)
  	return;

  rpt_did_reset = 1;
	is_syncline = 0;
  vsyncmintime = read_processor_time() + vsynctimebase;
	write_log (_T("Resetting frame rate hack\n"));
}

STATIC_INLINE void setclr (uae_u16 *p, uae_u16 val)
{
  if (val & 0x8000)
  	*p |= val & 0x7FFF;
  else
  	*p &= ~val;
}

STATIC_INLINE void set_chipset_mode(void)
{
	if (currprefs.chipset_mask & CSMASK_AGA) {
		fmode = fmode_saved;
	} else {
		fmode = 0;
	}
	sprite_width = GET_SPRITEWIDTH (fmode);
}

static void update_mirrors(void)
{
  aga_mode = (currprefs.chipset_mask & CSMASK_AGA) != 0;
	set_chipset_mode();
}

STATIC_INLINE uae_u8 *pfield_xlateptr (uaecptr plpt, int bytecount)
{
  plpt &= chipmem_bank.mask;
  if((plpt + bytecount) > chipmem_bank.allocated_size)
    return NULL;
  return chipmem_bank.baseaddr + plpt;
}

static void docols (struct color_entry *colentry)
{
	int i;
	
	if (aga_mode) {
		for (i = 0; i < 256; i++) {
			int v = colentry->color_regs_aga[i];
	    if (v < 0 || v > 16777215)
				continue;
			colentry->acolors[i] = CONVERT_RGB (v);
		}
	} else {
		for (i = 0; i < 32; i++) {
			int v = colentry->color_regs_ecs[i];
	    if (v < 0 || v > 4095)
				continue;
			colentry->acolors[i] = xcolors[v];
		}
	}
}

extern struct color_entry colors_for_drawing;

void notice_new_xcolors (void)
{
	int i;
	
  update_mirrors ();
	docols(&current_colors);
  docols(&colors_for_drawing);
	for (i = 0; i < (MAXVPOS + 1)*2; i++) {
		docols(curr_color_tables + i);
	}
}

static void do_sprites (int currhp);

static void remember_ctable (void)
{
	/* This can happen when program crashes very badly */
	if (next_color_entry >= COLOR_TABLE_SIZE)
		return;
	if (remembered_color_entry < 0) {
		/* The colors changed since we last recorded a color map. Record a
		 * new one. */
		color_reg_cpy (curr_color_tables + next_color_entry, &current_colors);
		remembered_color_entry = next_color_entry++;
	}
	thisline_decision.ctable = remembered_color_entry;
}

STATIC_INLINE int get_equ_vblank_endline (void)
{
	return equ_vblank_endline + (equ_vblank_toggle ? (lof_current ? 1 : 0) : 0);
}

#define DDF_OFFSET 4
#define HARD_DDF_LIMITS_DISABLED ((beamcon0 & 0x80) || (beamcon0 & 0x4000) || (bplcon0 & 0x40))
/* The HRM says 0xD8, but that can't work... */
#define HARD_DDF_STOP (HARD_DDF_LIMITS_DISABLED ? maxhpos : 0xd4)
#define HARD_DDF_START_REAL 0x14
/* Programmed rates or superhires (!) disable normal DMA limits */
#define HARD_DDF_START (HARD_DDF_LIMITS_DISABLED ? 0x04 : 0x14)

/* Called to determine the state of the horizontal display window state
 * machine at the current position. It might have changed since we last
 * checked.  */
static void decide_diw (int hpos)
{
	/* Last hpos = hpos + 0.5, eg. normal PAL end hpos is 227.5 * 2 = 455
	   OCS Denise: 9 bit hdiw counter does not reset during lines 0 to 9
	   (PAL) or lines 0 to 10 (NTSC). A1000 PAL: 1 to 9, NTSC: 1 to 10.
	   ECS Denise and AGA: no above "features"
	*/

	int hdiw = hpos >= maxhpos ? maxhpos * 2 + 1 : hpos * 2 + 2;
	if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE) && vpos <= get_equ_vblank_endline ())
		hdiw = diw_hcounter;
	/* always mask, bad programs may have set maxhpos = 256 */
	hdiw &= 511;
	for (;;) {
		int lhdiw = hdiw;
		if (last_hdiw > lhdiw)
			lhdiw = 512;

		if (lhdiw >= diw_hstrt && last_hdiw < diw_hstrt && hdiwstate == DIW_waiting_start) {
			if (thisline_decision.diwfirstword < 0)
				thisline_decision.diwfirstword = diwfirstword < 0 ? PIXEL_XPOS(0) : diwfirstword;
			hdiwstate = DIW_waiting_stop;
		}
		if (((hpos >= maxhpos && HARD_DDF_LIMITS_DISABLED) || (lhdiw >= diw_hstop && last_hdiw < diw_hstop)) && hdiwstate == DIW_waiting_stop) {
			if (thisline_decision.diwlastword < 0)
				thisline_decision.diwlastword = diwlastword < 0 ? 0 : diwlastword;
			hdiwstate = DIW_waiting_start;
		}
		if (lhdiw != 512)
			break;
		last_hdiw = 0 - 1;
	}
	last_hdiw = hdiw;
}

static int fetchmode, fetchmode_size, fetchmode_mask, fetchmode_bytes, fetchmode_fmode;
static int real_bitplane_number[3][3][9];

/* Disable bitplane DMA if planes > available DMA slots. This is needed
   e.g. by the Sanity WOC demo (at the "Party Effect").  */
STATIC_INLINE int GET_PLANES_LIMIT (void)
{
  return real_bitplane_number[fetchmode][bplcon0_res][bplcon0_planes];
}

static void reset_moddelays (void)
{
	if (dbpl1mod_on > 0) {
		bpl1mod = dbpl1mod;
		dbpl1mod_on = 0;
	}
	if (dbpl2mod_on > 0) {
		bpl2mod = dbpl2mod;
		dbpl2mod_on = 0;
	}
}

static void add_modulo (int hpos, int nr)
{
  int mod;

	if (dbpl1mod_on != hpos && dbpl1mod_on) {
		bpl1mod = dbpl1mod;
		dbpl1mod_on = 0;
	}
	if (dbpl2mod_on != hpos && dbpl2mod_on) {
		bpl2mod = dbpl2mod;
		dbpl2mod_on = 0;
	}
  if (fmode & 0x4000) {
    if (((diwstrt >> 8) ^ vpos) & 1)
      mod = bpl2mod;
    else
      mod = bpl1mod;
  } else if (nr & 1)
    mod = bpl2mod;
  else
    mod = bpl1mod;
  bplpt[nr] += mod;
	reset_moddelays ();
}

STATIC_INLINE void add_modulos(void)
{
  int m1, m2;
 
	reset_moddelays ();
  if (fmode & 0x4000) {
    if (((diwstrt >> 8) ^ vpos) & 1)
      m1 = m2 = bpl2mod;
    else
      m1 = m2 = bpl1mod;
  } else {
    m1 = bpl1mod;
    m2 = bpl2mod;
  }
 
  switch (bplcon0_planes_limit) {
    case 8: bplpt[7] += m2;
    case 7: bplpt[6] += m1;
    case 6: bplpt[5] += m2;
    case 5: bplpt[4] += m1;
    case 4: bplpt[3] += m2;
    case 3: bplpt[2] += m1;
    case 2: bplpt[1] += m2;
    case 1: bplpt[0] += m1;
  }
}

/* The fetch unit mainly controls ddf stop.  It's the number of cycles that
   are contained in an indivisible block during which ddf is active.  E.g.
   if DDF starts at 0x30, and fetchunit is 8, then possible DDF stops are
   0x30 + n * 8.  */
static int fetchunit, fetchunit_mask;
/* The delay before fetching the same bitplane again.  Can be larger than
   the number of bitplanes; in that case there are additional empty cycles
   with no data fetch (this happens for high fetchmodes and low
   resolutions).  */
static int fetchstart, fetchstart_mask;
/* fm_maxplane holds the maximum number of planes possible with the current
   fetch mode.  This selects the cycle diagram:
   8 planes: 73516240
   4 planes: 3120
   2 planes: 10.  */
static int fm_maxplane;

/* The corresponding values, by fetchmode and display resolution.  */
static const int fetchunits[] = { 8,8,8,0, 16,8,8,0, 32,16,8,0 };
static const int fetchstarts[] = { 3,2,1,0, 4,3,2,0, 5,4,3,0 };
static const int fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

static int cycle_diagram_table[3][3][9][32];
static int cycle_diagram_free_cycles[3][3][9];
static int cycle_diagram_total_cycles[3][3][9];
static int *curr_diagram;
static const int cycle_sequences[3 * 8] = { 2,1,2,1,2,1,2,1, 4,2,3,1,4,2,3,1, 8,4,6,2,7,3,5,1 };

static void create_cycle_diagram_table(void)
{
  int fm, res, cycle, planes, rplanes, v;
  int fetch_start, max_planes, freecycles;
  const int *cycle_sequence;
   
  for (fm = 0; fm <= 2; fm++) {
    for (res = 0; res <= 2; res++) {
      max_planes = fm_maxplanes[fm * 4 + res];
      fetch_start = 1 << fetchstarts[fm * 4 + res];
      cycle_sequence = &cycle_sequences[(max_planes - 1) * 8];
      max_planes = 1 << max_planes;
      for (planes = 0; planes <= 8; planes++) {
        freecycles = 0;
        for (cycle = 0; cycle < 32; cycle++)
          cycle_diagram_table[fm][res][planes][cycle] = -1;
        if (planes <= max_planes) {
          for (cycle = 0; cycle < fetch_start; cycle++) {
            if (cycle < max_planes && planes >= cycle_sequence[cycle & 7]) {
              v = cycle_sequence[cycle & 7];
            } else {
              v = 0;
              freecycles++;
            }
            cycle_diagram_table[fm][res][planes][cycle] = v;
          }
        }
		    cycle_diagram_free_cycles[fm][res][planes] = freecycles;
				cycle_diagram_total_cycles[fm][res][planes] = fetch_start;
     		rplanes = planes;
     		if (rplanes > max_planes)
   		    rplanes = 0;
     		if (rplanes == 7 && fm == 0 && res == 0 && !(aga_mode))
   		    rplanes = 4;
       	real_bitplane_number[fm][res][planes] = rplanes;
      }
    }
  }
}


/* Used by the copper.  */
static int estimated_last_fetch_cycle;
static int cycle_diagram_shift;

static void estimate_last_fetch_cycle (int hpos)
{
	// Last fetch is always max 8 even if fetchunit is larger.
	int lastfetchunit = fetchunit >= 8 ? 8 : fetchunit;

	if (plf_state < plf_passed_stop) {
    int stop;
		
		if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
			// ECS: stop wins if start == stop
			stop = plfstop + DDF_OFFSET < hpos || plfstop > HARD_DDF_STOP ? HARD_DDF_STOP : plfstop;
		} else {
			// OCS: start wins if start == stop
			stop = plfstop + DDF_OFFSET <= hpos || plfstop > HARD_DDF_STOP ? HARD_DDF_STOP : plfstop;
		}
		/* We know that fetching is up-to-date up until hpos, so we can use fetch_cycle.  */
		int fetch_cycle_at_stop = fetch_cycle + (stop - hpos + DDF_OFFSET);
		int starting_last_block_at = (fetch_cycle_at_stop + fetchunit - 1) & ~fetchunit_mask;
		
		estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + lastfetchunit;
	} else {
		int starting_last_block_at = (fetch_cycle + fetchunit - 1) & ~fetchunit_mask;
		if (plf_state == plf_passed_stop2)
			starting_last_block_at -= fetchunit;
		
		estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + lastfetchunit;
	}
}

static uae_u32 outword[MAX_PLANES];
static int out_nbits, out_offs;
static uae_u16 todisplay[MAX_PLANES], todisplay2[MAX_PLANES];
static uae_u16 fetched[MAX_PLANES];
static bool todisplay_fetched[2];
static uae_u64 todisplay_aga[MAX_PLANES], todisplay2_aga[MAX_PLANES];
static uae_u64 fetched_aga[MAX_PLANES];

/* Expansions from bplcon0/bplcon1.  */
static int toscr_res, toscr_res2p;
static int toscr_nr_planes, toscr_nr_planes2, toscr_nr_planes_agnus, toscr_nr_planes_shifter;
static int toscr_delay[2], toscr_delay_adjusted[2], toscr_delay_sh[2];
static int delay_cycles, delay_lastcycle[2];
static bool bplcon1_written;

/* The number of bits left from the last fetched words.  
   This is an optimization - conceptually, we have to make sure the result is
   the same as if toscr is called in each clock cycle.  However, to speed this
   up, we accumulate display data; this variable keeps track of how much. 
   Thus, once we do call toscr_nbits (which happens at least every 16 bits),
   we can do more work at once.  */
static int toscr_nbits;

static void record_color_change2 (int hpos, int regno, uae_u32 value)
{
	int pos = (hpos * 2) * 4;
	if (regno == 0x1000 + 0x10c) {
		pos += 4; // BPLCON4 change needs 1 lores pixel delay
		if (value & 0xff00)
			thisline_decision.xor_seen = true;
  }
	curr_color_changes[next_color_change].linepos = pos;
	curr_color_changes[next_color_change].regno = regno;
	curr_color_changes[next_color_change].value = value;
	next_color_change++;
  curr_color_changes[next_color_change].regno = -1;
}

// OCS/ECS, lores, 7 planes = 4 "real" planes + BPL5DAT and BPL6DAT as static 5th and 6th plane
STATIC_INLINE int isocs7planes (void)
{
	return !(aga_mode) && bplcon0_res == 0 && bplcon0_planes == 7;
}

STATIC_INLINE int is_bitplane_dma (int hpos)
{
  if (hpos < bpl_hstart || fetch_state == fetch_not_started || plf_state == plf_wait) {
    return 0;
  }
  if ((plf_state >= plf_end && hpos >= thisline_decision.plfright)
    || hpos >= estimated_last_fetch_cycle)
    return 0;
  return curr_diagram[(hpos - cycle_diagram_shift) & fetchstart_mask];
}

STATIC_INLINE int islinetoggle (void)
{
	int linetoggle = 0;
  if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS) && currprefs.ntscmode) {
		linetoggle = 1; // hardwired NTSC Agnus
	}
	return linetoggle;
}

/* Expand bplcon0/bplcon1 into the toscr_xxx variables.  */
STATIC_INLINE void compute_toscr_delay (int bplcon1)
{
  int delay1 = (bplcon1 & 0x0f) | ((bplcon1 & 0x0c00) >> 6);
  int delay2 = ((bplcon1 >> 4) & 0x0f) | (((bplcon1 >> 4) & 0x0c00) >> 6);
  int shdelay1 = (bplcon1 >> 8) & 3;
  int shdelay2 = (bplcon1 >> 12) & 3;
	int delaymask = fetchmode_mask >> toscr_res;
    
  toscr_delay[0] = (delay1 & delaymask) << toscr_res;
  toscr_delay[0] |= shdelay1 >> (RES_MAX - toscr_res);
  toscr_delay[1] = (delay2 & delaymask) << toscr_res;
  toscr_delay[1] |= shdelay2 >> (RES_MAX - toscr_res);

	/* SPEEDUP code still needs this hack */
	int	delayoffset = fetchmode_size - (((bpl_hstart - (HARD_DDF_START_REAL + DDF_OFFSET)) & fetchstart_mask) << 1);
  delay1 += delayoffset;
  delay2 += delayoffset;
	toscr_delay_adjusted[0] = (delay1 & delaymask) << toscr_res;
	toscr_delay_adjusted[0] |= shdelay1 >> (RES_MAX - toscr_res);
	toscr_delay_adjusted[1] = (delay2 & delaymask) << toscr_res;
	toscr_delay_adjusted[1] |= shdelay2 >> (RES_MAX - toscr_res);
}

static void set_delay_lastcycle (void)
{
	if (HARD_DDF_LIMITS_DISABLED) {
		delay_lastcycle[0] = (256 * 2) << bplcon0_res;
		delay_lastcycle[1] = (256 * 2) << bplcon0_res;
	} else {
		delay_lastcycle[0] = ((maxhpos + 1) * 2 + 0) << bplcon0_res;
		delay_lastcycle[1] = delay_lastcycle[0];
		if (islinetoggle ())
			delay_lastcycle[1]++;
	}
}

static int bpldmasetuphpos, bpldmasetuphpos_diff;
static int bpldmasetupphase;
static void update_toscr_planes (int fm);

/* set currently active Agnus bitplane DMA sequence */
static void setup_fmodes (int hpos)
{
  switch (fmode & 3) 
  {
 	case 0:
 		fetchmode = 0;
 		break;
 	case 1:
 	case 2:
 		fetchmode = 1;
 		break;
 	case 3:
 		fetchmode = 2;
 		break;
  }
	badmode = GET_RES_AGNUS (bplcon0) != GET_RES_DENISE (bplcon0);
  bplcon0_res = GET_RES_AGNUS (bplcon0);
  bplcon0_planes = GET_PLANES (bplcon0);
  bplcon0_planes_limit = GET_PLANES_LIMIT ();
  fetchunit = fetchunits[fetchmode * 4 + bplcon0_res];
  fetchunit_mask = fetchunit - 1;
  int fetchstart_shift = fetchstarts[fetchmode * 4 + bplcon0_res];
  fetchstart = 1 << fetchstart_shift;
  fetchstart_mask = fetchstart - 1;
  int fm_maxplane_shift = fm_maxplanes[fetchmode * 4 + bplcon0_res];
  fm_maxplane = 1 << fm_maxplane_shift;
  fetch_modulo_cycle = fetchunit - fetchstart;
	fetchmode_size = 16 << fetchmode;
	fetchmode_bytes = 2 << fetchmode;
	fetchmode_mask = fetchmode_size - 1;
	fetchmode_fmode = fmode;
	set_delay_lastcycle ();
	compute_toscr_delay (bplcon1);

	if (thisline_decision.plfleft < 0) {
		thisline_decision.bplres = bplcon0_res;
		thisline_decision.bplcon0 = bplcon0;
		thisline_decision.nr_planes = bplcon0_planes;
	}

  curr_diagram = cycle_diagram_table[fetchmode][bplcon0_res][bplcon0_planes_limit];
	estimate_last_fetch_cycle (hpos);
	bpldmasetuphpos = -1;
	bpldmasetupphase = 0;
	toscr_nr_planes_agnus = bplcon0_planes;
	if (isocs7planes ()) {
		toscr_nr_planes_agnus = 6;
	}
	SET_LINE_CYCLEBASED;
}

static void BPLCON0_Denise (int hpos, uae_u16 v, bool);

// writing to BPLCON0 adds 4 cycle delay before Agnus bitplane DMA sequence changes
// (Note that Denise sees the change after 1 cycle)
// AGA needs extra cycle in some specific situations (Brian The Lion "dialog") but not
// in all situations (Superstardust weapon panel)
#define BPLCON_AGNUS_DELAY (3 + (copper_access ? 1 : 0) + (bplcon0_planes == 8 ? 1 : 0))
#define BPLCON_DENISE_DELAY (copper_access ? 1 : 0)

STATIC_INLINE void maybe_setup_fmodes (int hpos)
{
	switch (bpldmasetupphase)
	{
	case 0:
		BPLCON0_Denise (hpos, bplcon0, false);
		bpldmasetupphase++;
		bpldmasetuphpos += bpldmasetuphpos_diff;
		break;
	case 1:
		setup_fmodes (hpos);
		break;
	}
}

STATIC_INLINE void maybe_check (int hpos)
{
	if (bpldmasetuphpos > 0 && hpos >= bpldmasetuphpos)
		maybe_setup_fmodes (hpos);
}

static void bpldmainitdelay (int hpos)
{
	SET_LINE_CYCLEBASED;
	if (hpos + BPLCON_AGNUS_DELAY < 0x14) {
		BPLCON0_Denise (hpos, bplcon0, false);
		setup_fmodes (hpos);
		return;
	}
	if (bpldmasetuphpos < 0) {
		bpldmasetuphpos = hpos + BPLCON_DENISE_DELAY;
		bpldmasetuphpos_diff = BPLCON_AGNUS_DELAY - BPLCON_DENISE_DELAY;
		bpldmasetupphase = 0;
		if (BPLCON_DENISE_DELAY == 0) {
			maybe_setup_fmodes (hpos);
		}
	}
}

STATIC_INLINE void clear_fetchbuffer (uae_u32 *ptr, int nwords)
{
	memset (ptr, 0, nwords * 4);
}

STATIC_INLINE void update_toscr_planes (int fm)
{
	// This must be called just before new bitplane block starts,
	// not when depth value changes. Depth can change early and can leave
	// 16+ pixel horizontal line of old data visible.
	if (toscr_nr_planes_agnus > thisline_decision.nr_planes) {
    if(out_offs) {
    	int j;
    	for (j = thisline_decision.nr_planes; j < toscr_nr_planes_agnus; j++) {
				clear_fetchbuffer ((uae_u32 *)(line_data[next_lineno] + 2 * MAX_WORDS_PER_LINE * j), out_offs);
				if (thisline_decision.plfleft >= 0) {
					todisplay[j] = 0;
					if (fm)
						todisplay_aga[j] = 0;
        }
      }
	  }
		thisline_decision.nr_planes = toscr_nr_planes_agnus;
  }
}

STATIC_INLINE void maybe_first_bpl1dat (int hpos)
{
	if (thisline_decision.plfleft < 0) {
		thisline_decision.plfleft = hpos;
	}
}

static void fetch (int nr, int fm, bool modulo, int hpos)
{
	if (nr < bplcon0_planes_limit) {
		int add = fetchmode_bytes;

    uaecptr p = bplpt[nr];

		bplpt[nr] += add;

		if (nr == 0)
			bpl1dat_written = true;

    switch (fm) 
    {
      case 0:
			  fetched[nr] = chipmem_wget_indirect (p);
			  last_custom_value1 = fetched[nr];
        break;
      case 1:
		  {
			  uaecptr pm = p & ~3;
			  if (p & 2) {
				  fetched_aga[nr] = chipmem_lget_indirect(pm) & 0x0000ffff;
				  fetched_aga[nr] |= fetched_aga[nr] << 16;
			  } else if (fetchmode_fmode & 2) { // optimized (fetchmode_fmode & 3) == 2
				  fetched_aga[nr] = chipmem_lget_indirect(pm) & 0xffff0000;
				  fetched_aga[nr] |= fetched_aga[nr] >> 16;
			  } else {
				  fetched_aga[nr] = chipmem_lget_indirect(pm);
			  }
			  last_custom_value1 = (uae_u32)fetched_aga[nr];
			  fetched[nr] = (uae_u16)fetched_aga[nr];
			  break;
		  }
      case 2:
		  {
			  uaecptr pm = p & ~7;
			  uaecptr pm1, pm2;
			  if (p & 4) {
				  pm1 = pm + 4;
				  pm2 = pm + 4;
			  } else {
				  pm1 = pm;
				  pm2 = pm + 4;
			  }
			  if (p & 2) {
				  uae_u32 v1 = chipmem_lget_indirect(pm1) & 0x0000ffff;
				  uae_u32 v2 = chipmem_lget_indirect(pm2) & 0x0000ffff;
				  v1 |= v1 << 16;
				  v2 |= v2 << 16;
				  fetched_aga[nr] = (((uae_u64)v1) << 32) | v2;
			  } else {
				  fetched_aga[nr] = ((uae_u64)chipmem_lget_indirect(pm1)) << 32;
				  fetched_aga[nr] |= chipmem_lget_indirect(pm2);
			  }
			  last_custom_value1 = (uae_u32)fetched_aga[nr];
			  fetched[nr] = (uae_u16)fetched_aga[nr];
			  break;
		  }
    }
    if (modulo)
			add_modulo(hpos, nr);
	}
}

STATIC_INLINE void toscr_3_ecs (int oddeven, int step, int nbits)
{
	int i;
	int shift = 16 - nbits;

	// if number of planes decrease (or go to zero), we still need to
	// shift all possible remaining pixels out of Denise's shift register
	for (i = oddeven; i < thisline_decision.nr_planes; i += step) {
		outword[i] <<= nbits;
	}

	for (i = oddeven; i < toscr_nr_planes2; i += step) {
		outword[i] |= todisplay2[i] >> shift;
		todisplay2[i] <<= nbits;
	}
}

STATIC_INLINE void toscr_3_aga (int oddeven, int step, int nbits, int fm)
{
	int i;
	uae_u32 mask = 0xFFFF >> (16 - nbits);
	int shift = fetchmode_size - nbits;

	for (i = oddeven; i < thisline_decision.nr_planes; i += step) {
		outword[i] <<= nbits;
	}

	for (i = oddeven; i < toscr_nr_planes2; i += step) {
		outword[i] |= (todisplay2_aga[i] >> shift) & mask;
		todisplay2_aga[i] <<= nbits;
	}
}

static void toscr_2_0 (int nbits) { toscr_3_ecs (0, 1, nbits); }
static void toscr_2_0_oe (int oddeven, int step, int nbits) { toscr_3_ecs (oddeven, step, nbits); }
static void toscr_2_1 (int nbits) { toscr_3_aga (0, 1, nbits, 1); }
static void toscr_2_1_oe (int oddeven, int step, int nbits) { toscr_3_aga (oddeven, step, nbits, 1); }
static void toscr_2_2 (int nbits) { toscr_3_aga (0, 1, nbits, 2); }
static void toscr_2_2_oe (int oddeven, int step, int nbits) { toscr_3_aga (oddeven, step, nbits, 2); }

static void do_tosrc (int oddeven, int step, int nbits, int fm)
{
	switch (fm) {
	  case 0:
		  if (step == 2)
			  toscr_2_0_oe (oddeven, step, nbits);
		  else
			  toscr_2_0 (nbits);
		  break;
	  case 1:
		  if (step == 2)
			  toscr_2_1_oe (oddeven, step, nbits);
		  else
			  toscr_2_1 (nbits);
		  break;
	  case 2:
		  if (step == 2)
			  toscr_2_2_oe (oddeven, step, nbits);
		  else
			  toscr_2_2 (nbits);
		  break;
	}
}

STATIC_INLINE void do_delays_3_ecs (int nbits)
{
	int delaypos = delay_cycles & fetchmode_mask;
	for (int oddeven = 0; oddeven < 2; oddeven++) {
		int delay = toscr_delay[oddeven];
		if (delaypos > delay)
			delay += fetchmode_size;
		int diff = delay - delaypos;
		int nbits2 = nbits;
		if (nbits2 > diff) {
			do_tosrc (oddeven, 2, diff, 0);
			nbits2 -= diff;
			if (todisplay_fetched[oddeven]) {
				for (int i = oddeven; i < toscr_nr_planes_shifter; i += 2)
					todisplay2[i] = todisplay[i];
				todisplay_fetched[oddeven] = false;
			}
		}
		if (nbits2)
			do_tosrc (oddeven, 2, nbits2, 0);
	}
}

STATIC_INLINE void do_delays_fast_3_ecs (int nbits)
{
	int delaypos = delay_cycles & fetchmode_mask;
	int delay = toscr_delay[0];
	if (delaypos > delay)
		delay += fetchmode_size;
	int diff = delay - delaypos;
	int nbits2 = nbits;
	if (nbits2 > diff) {
		do_tosrc (0, 1, diff, 0);
		nbits2 -= diff;
		if (todisplay_fetched[0]) {
			for (int i = 0; i < toscr_nr_planes_shifter; i++)
				todisplay2[i] = todisplay[i];
			todisplay_fetched[0] = false;
			todisplay_fetched[1] = false;
		}
	}
	if (nbits2)
		do_tosrc (0, 1, nbits2, 0);
}

STATIC_INLINE void do_delays_3_aga (int nbits, int fm)
{
	int delaypos = delay_cycles & fetchmode_mask;
	for (int oddeven = 0; oddeven < 2; oddeven++) {
		int delay = toscr_delay[oddeven];
		if (delaypos > delay)
			delay += fetchmode_size;
		int diff = delay - delaypos;
		int nbits2 = nbits;
		if (nbits2 > diff) {
			do_tosrc (oddeven, 2, diff, fm);
			nbits2 -= diff;
			if (todisplay_fetched[oddeven]) {
				for (int i = oddeven; i < toscr_nr_planes_shifter; i += 2)
					todisplay2_aga[i] = todisplay_aga[i];
				todisplay_fetched[oddeven] = false;
			}
		}
		if (nbits2)
			do_tosrc (oddeven, 2, nbits2, fm);
	}
}

STATIC_INLINE void do_delays_fast_3_aga (int nbits, int fm)
{
	int delaypos = delay_cycles & fetchmode_mask;
	int delay = toscr_delay[0];
	if (delaypos > delay)
		delay += fetchmode_size;
	int diff = delay - delaypos;
	int nbits2 = nbits;
	if (nbits2 > diff) {
		do_tosrc (0, 1, diff, fm);
		nbits2 -= diff;
		if (todisplay_fetched[0]) {
			for (int i = 0; i < toscr_nr_planes_shifter; i++)
				todisplay2_aga[i] = todisplay_aga[i];
			todisplay_fetched[0] = false;
			todisplay_fetched[1] = false;
		}
	}
	if (nbits2)
		do_tosrc (0, 1, nbits2, fm);
}

static void do_delays_2_0 (int nbits) { do_delays_3_ecs (nbits); }
static void do_delays_2_1 (int nbits) { do_delays_3_aga (nbits, 1); }
static void do_delays_2_2 (int nbits) { do_delays_3_aga (nbits, 2); }
static void do_delays_fast_2_0 (int nbits) { do_delays_fast_3_ecs (nbits); }
static void do_delays_fast_2_1 (int nbits) { do_delays_fast_3_aga (nbits, 1); }
static void do_delays_fast_2_2 (int nbits) { do_delays_fast_3_aga (nbits, 2); }

// slower version, odd and even delays are different or crosses maxhpos
static void do_delays (int nbits, int fm)
{
	switch (fm) {
	case 0:
		do_delays_2_0 (nbits);
		break;
	case 1:
		do_delays_2_1 (nbits);
		break;
	case 2:
		do_delays_2_2 (nbits);
		break;
	}
}

// common optimized case: odd delay == even delay
static void do_delays_fast (int nbits, int fm)
{
	switch (fm) {
	case 0:
		do_delays_fast_2_0 (nbits);
		break;
	case 1:
		do_delays_fast_2_1 (nbits);
		break;
	case 2:
		do_delays_fast_2_2 (nbits);
		break;
	}
}

static void toscr_right_edge (int nbits, int fm)
{
	// Emulate hpos counter (delay_cycles) reseting at the end of scanline.
	// (Result is ugly shift in graphics in far right overscan)
	int diff = delay_lastcycle[lol] - delay_cycles;
	int nbits2 = nbits;
	if (nbits2 >= diff) {
		do_delays (diff, fm);
		nbits2 -= diff;
		delay_cycles = 0;
		if (hpos_is_zero_bplcon1_hack >= 0) {
			compute_toscr_delay(hpos_is_zero_bplcon1_hack);
			hpos_is_zero_bplcon1_hack = -1;
		}
		toscr_delay[0] -= 2;
		toscr_delay[0] &= fetchmode_mask;
		toscr_delay[1] -= 2;
		toscr_delay[1] &= fetchmode_mask;
	}
	if (nbits2) {
		do_delays (nbits2, fm);
		delay_cycles += nbits2;
	}
}

STATIC_INLINE void toscr_1 (int nbits, int fm)
{
	if (delay_cycles + nbits >= delay_lastcycle[lol]) {
		toscr_right_edge (nbits, fm);
	} else if (toscr_delay[0] == toscr_delay[1]) {
		// Most common case.
		do_delays_fast (nbits, fm);
		delay_cycles += nbits;
	} else {
		do_delays (nbits, fm);
		delay_cycles += nbits;
	}

	out_nbits += nbits;
	if (out_nbits == 32) {
		int i;
		uae_u8 *dataptr = line_data[next_lineno] + out_offs * 4;
		for (i = 0; i < thisline_decision.nr_planes; i++) {
			uae_u32 *dataptr32 = (uae_u32 *)dataptr;
			*dataptr32 = outword[i];
			outword[i] = 0;
			dataptr += MAX_WORDS_PER_LINE * 2;
		}
		out_offs++;
		out_nbits = 0;
	}
}

static void toscr_1_hires (int nbits_nres, int fm)
{
	int maxnbits = 16;
	while (nbits_nres >= 0) {
		int nbits = nbits_nres >= maxnbits ? maxnbits : nbits_nres;
		if (delay_cycles + nbits >= delay_lastcycle[lol]) {
			toscr_right_edge(nbits, fm);
		} else {
			do_delays(nbits, fm);
			delay_cycles += nbits;
		}

		out_nbits += nbits;
		if (out_nbits == 32) {
			int i;
			uae_u8 *dataptr = line_data[next_lineno] + out_offs * 4;
			for (i = 0; i < thisline_decision.nr_planes; i++) {
				uae_u32 *dataptr32 = (uae_u32 *)dataptr;
				*dataptr32 = outword[i];
				outword[i] = 0;
				dataptr += MAX_WORDS_PER_LINE * 2;
			}
			out_offs++;
			out_nbits = 0;
		}
		nbits_nres -= nbits;
	}
}

static void toscr_fm0 (int);
static void toscr_fm1 (int);
static void toscr_fm2 (int);

STATIC_INLINE void toscr (int nbits, int fm)
{
  switch (fm) {
    case 0: toscr_fm0 (nbits); break;
    case 1: toscr_fm1 (nbits); break;
    case 2: toscr_fm2 (nbits); break;
  }
}

STATIC_INLINE void toscr_0 (int nbits, int fm)
{
	int t;

	if (nbits > 16) {
		toscr (16, fm);
		nbits -= 16;
	}

	t = 32 - out_nbits;
	if (t < nbits) {
		toscr_1 (t, fm);
		nbits -= t;
	}
	toscr_1 (nbits, fm);
}

static void toscr_fm0 (int nbits) { toscr_0 (nbits, 0); }
static void toscr_fm1 (int nbits) { toscr_0 (nbits, 1); }
static void toscr_fm2 (int nbits) { toscr_0 (nbits, 2); }

STATIC_INLINE int flush_plane_data (int fm)
{
	int i = 0;

	if (out_nbits <= 16) {
		i += 16;
		toscr_1 (16, fm);
	}
	if (out_nbits != 0) {
		i += 32 - out_nbits;
		toscr_1 (32 - out_nbits, fm);
	}

	i += 32;
	toscr_1 (16, fm);
	toscr_1 (16, fm);

  if (fm == 2) {
		/* flush AGA full 64-bit shift register + possible data in todisplay */
	  i += 32;
	  toscr_1 (16, fm);
	  toscr_1 (16, fm);
		i += 32;
		toscr_1 (16, fm);
		toscr_1 (16, fm);
	}

	return i >> (1 + toscr_res);
}

STATIC_INLINE void flush_display (int fm)
{
  if (toscr_nbits > 0 && thisline_decision.plfleft >= 0)
    toscr (toscr_nbits, fm);
  toscr_nbits = 0;
}

static void update_denise_shifter_planes (int hpos)
{
	int np = GET_PLANES (bplcon0d);
	// if DMA has ended but there is still data waiting in todisplay,
	// it must be flushed out before number of planes change
	if (np < toscr_nr_planes_shifter && hpos > thisline_decision.plfright && thisline_decision.plfright && (todisplay_fetched[0] || todisplay_fetched[1])) {
		int diff = (hpos - thisline_decision.plfright) << (1 + toscr_res);
		while (diff >= 16) {
			toscr_1 (16, fetchmode);
			diff -= 16;
		}
		if (diff)
			toscr_1 (diff, fetchmode);
		thisline_decision.plfright += hpos - thisline_decision.plfright;
	}
	// FIXME: Samplers / Back In 90 vs Disposable Hero title screen in fast modes
	if (currprefs.cpu_model < 68020) {
		toscr_nr_planes_shifter = np;
		if (isocs7planes()) {
			if (toscr_nr_planes_shifter < 6)
				toscr_nr_planes_shifter = 6;
		}
	}
}

static void update_denise (int hpos)
{
	int res = GET_RES_DENISE (bplcon0d);
	if (res != toscr_res)
		flush_display (fetchmode);
	toscr_res = GET_RES_DENISE (bplcon0d);
	toscr_res2p = 2 << toscr_res;
	delay_cycles = (hpos * 2) << toscr_res;
	if (bplcon0dd != bplcon0d) {
		record_color_change2 (hpos, 0x100 + 0x1000, bplcon0d);
		bplcon0dd = bplcon0d;
	}
	toscr_nr_planes = GET_PLANES (bplcon0d);
	if (isocs7planes ()) {
		if (toscr_nr_planes2 < 6)
			toscr_nr_planes2 = 6;
	} else {
		toscr_nr_planes2 = toscr_nr_planes;
	}
	toscr_nr_planes_shifter = toscr_nr_planes2;
}

STATIC_INLINE void fetch_start (void)
{
	fetch_state = fetch_started;
}

/* Called when all planes have been fetched, i.e. when a new block
   of data is available to be displayed.  The data in fetched[] is
   moved into todisplay[].  */
STATIC_INLINE void beginning_of_plane_block (int hpos, int fm)
{
	int i;

	if (fm == 0)
		for (i = 0; i < MAX_PLANES; i++) {
			todisplay[i] = fetched[i];
    }
	else
		for (i = 0; i < MAX_PLANES; i++) {
			todisplay_aga[i] = fetched_aga[i];
	  }
	todisplay_fetched[0] = todisplay_fetched[1] = true;
  maybe_first_bpl1dat (hpos);
	update_denise (hpos);
	if (toscr_nr_planes_agnus > thisline_decision.nr_planes)
		update_toscr_planes (fm);
}


/* The usual inlining tricks - don't touch unless you know what you are doing. */
STATIC_INLINE void long_fetch_16 (int plane, int nwords, int weird_number_of_bits, int dma)
{
  uae_u16 *real_pt = (uae_u16 *)pfield_xlateptr (bplpt[plane], nwords * 2);
	int delay = toscr_delay_adjusted[plane & 1];
  int tmp_nbits = out_nbits;
  uae_u32 shiftbuffer;
  uae_u32 outval = outword[plane];
  uae_u32 fetchval = fetched[plane];
	uae_u32 *dataptr = (uae_u32 *)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);

  if (dma) {
    bplpt[plane] += nwords *2;
  }

  if (real_pt == 0)
		/* @@@ Don't do this, fall back on chipmem_wget instead.  */
    return;

	shiftbuffer = todisplay2[plane] << delay;

  while (nwords > 0) {
		int bits_left = 32 - tmp_nbits;
		uae_u32 t;

		shiftbuffer |= fetchval;

		t = (shiftbuffer >> delay) & 0xffff;

		if (weird_number_of_bits && bits_left < 16) {
			outval <<= bits_left;
			outval |= t >> (16 - bits_left);
      *dataptr++ = outval;

      outval = t;
			tmp_nbits = 16 - bits_left;
		} else {
			outval = (outval << 16) | t;
			tmp_nbits += 16;
			if (tmp_nbits == 32) {
        *dataptr++ = outval;
				tmp_nbits = 0;
			}
		}
		shiftbuffer <<= 16;
		nwords--;
		if (dma) {
#ifdef ARMV6_ASSEMBLY
			__asm__ (
			    "ldrh    %[val], [%[pt]], #2   \n\t"
			    "rev16   %[val], %[val]        \n\t"
			    : [val] "=r" (fetchval), [pt] "+r" (real_pt) );
#else
			fetchval = do_get_mem_word (real_pt);
			real_pt++;
#endif
		}
	}
  fetched[plane] = fetchval;
	todisplay2[plane] = shiftbuffer >> delay;
  outword[plane] = outval;
}

STATIC_INLINE void long_fetch_32 (int plane, int nwords, int weird_number_of_bits, int dma)
{
	uae_u32 *real_pt = (uae_u32 *)pfield_xlateptr (bplpt[plane] & ~3, nwords * 2);
	int delay = toscr_delay_adjusted[plane & 1];
	int tmp_nbits = out_nbits;
	uae_u64 shiftbuffer;
	uae_u32 outval = outword[plane];
	uae_u32 fetchval = (uae_u32)fetched_aga[plane];
	uae_u32 *dataptr = (uae_u32 *)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);
	bool unaligned = (bplpt[plane] & 2) != 0;

	if (dma) {
		bplpt[plane] += nwords * 2;
  }

	if (real_pt == 0)
		/* @@@ Don't do this, fall back on chipmem_wget instead.  */
		return;

	shiftbuffer = todisplay2_aga[plane] << delay;

	while (nwords > 0) {

		shiftbuffer |= fetchval;

		for (int i = 0; i < 2; i++) {
      uae_u32 t;
			int bits_left = 32 - tmp_nbits;

			t = (shiftbuffer >> (16 + delay)) & 0xffff;

			if (weird_number_of_bits && bits_left < 16) {
				outval <<= bits_left;
				outval |= t >> (16 - bits_left);

        *dataptr++ = outval;

				outval = t;
				tmp_nbits = 16 - bits_left;
			} else {
				outval = (outval << 16) | t;
				tmp_nbits += 16;
				if (tmp_nbits == 32) {
          *dataptr++ = outval;
          tmp_nbits = 0;
				}
			}
			shiftbuffer <<= 16;
		}
		nwords -= 2;
		if (dma) {
#ifdef ARMV6_ASSEMBLY
			__asm__ (
			    "ldr     %[val], [%[pt]], #4   \n\t"
			    "rev     %[val], %[val]        \n\t"
			    : [val] "=r" (fetchval), [pt] "+r" (real_pt) );
#else
			fetchval = do_get_mem_long (real_pt);
			real_pt++;
#endif
			if (unaligned) {
				fetchval &= 0x0000ffff;
				fetchval |= fetchval << 16;
			} else if (fetchmode_fmode & 2) {
				fetchval &= 0xffff0000;
				fetchval |= fetchval >> 16;
			}
		}
	}
	fetched_aga[plane] = fetchval;
	todisplay2_aga[plane] = (shiftbuffer >> delay) & 0xffffffff;
	outword[plane] = outval;
}

STATIC_INLINE void shift32plus (uae_u64 *p, int n)
{
	uae_u64 t = p[1];
	t <<= n;
	t |= p[0] >> (64 - n);
	p[1] = t;
}

STATIC_INLINE void aga_shift (uae_u64 *p, int n)
{
	if (n == 0) return;
	shift32plus (p, n);
	p[0] <<= n;
}

STATIC_INLINE void shift32plusn (uae_u64 *p, int n)
{
	uae_u64 t = p[0];
	t >>= n;
	t |= p[1] << (64 - n);
	p[0] = t;
}

STATIC_INLINE void aga_shift_n (uae_u64 *p, int n)
{
	if (n == 0) return;
	shift32plusn (p, n);
	p[1] >>= n;
}

STATIC_INLINE void long_fetch_64 (int plane, int nwords, int weird_number_of_bits, int dma)
{
	uae_u32 *real_pt = (uae_u32 *)pfield_xlateptr (bplpt[plane] & ~7, nwords * 2);
	int delay = toscr_delay_adjusted[plane & 1];
	int tmp_nbits = out_nbits;
	uae_u64 shiftbuffer[2];
	uae_u32 outval = outword[plane];
	uae_u64 fetchval = fetched_aga[plane];
	uae_u32 *dataptr = (uae_u32 *)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);
	int shift = (64 - 16) + delay;
	bool unaligned2 = (bplpt[plane] & 2) != 0;
	bool unaligned4 = (bplpt[plane] & 4) != 0;

	if (dma) {
		bplpt[plane] += nwords * 2;
  }

	if (real_pt == 0)
		/* @@@ Don't do this, fall back on chipmem_wget instead.  */
		return;

	shiftbuffer[1] = 0;
	shiftbuffer[0] = todisplay2_aga[plane];
	aga_shift (shiftbuffer, delay);

	while (nwords > 0) {
		int i;

		shiftbuffer[0] |= fetchval;

		for (i = 0; i < 4; i++) {
      uae_u32 t;
			int bits_left = 32 - tmp_nbits;

			if (64 - shift > 0) {
				t = (uae_u32)(shiftbuffer[1] << (64 - shift));
				t |= shiftbuffer[0] >> shift;
			} else {
				t = (uae_u32)(shiftbuffer[1] >> (shift - 64));
			}
			t &= 0xffff;

			if (weird_number_of_bits && bits_left < 16) {
				outval <<= bits_left;
				outval |= t >> (16 - bits_left);

        *dataptr++ = outval;

				outval = t;
				tmp_nbits = 16 - bits_left;
			} else {
				outval = (outval << 16) | t;
				tmp_nbits += 16;
				if (tmp_nbits == 32) {
               *dataptr++ = outval;
               tmp_nbits = 0;
				}
			}
			aga_shift (shiftbuffer, 16);
		}

		nwords -= 4;

		if (dma) {
			uae_u32 *real_pt1, *real_pt2;
			if (unaligned4) {
				real_pt1 = real_pt + 1;
				real_pt2 = real_pt + 1;
			} else {
				real_pt1 = real_pt;
				real_pt2 = real_pt + 1;
			}
			if (unaligned2) {
				uae_u32 v1 = do_get_mem_long (real_pt1);
				uae_u32 v2 = do_get_mem_long (real_pt2);
				v1 &= 0x0000ffff;
				v1 |= v1 << 16;
				v2 &= 0x0000ffff;
				v2 |= v2 << 16;
				fetchval = (((uae_u64)v1) << 32) | v2;
			} else {
				fetchval = ((uae_u64)do_get_mem_long (real_pt1)) << 32;
				fetchval |= do_get_mem_long (real_pt2);
			}
		  real_pt += 2;
		}
	}
	fetched_aga[plane] = fetchval;
	aga_shift_n (shiftbuffer, delay);
	todisplay2_aga[plane] = shiftbuffer[0];
	outword[plane] = outval;
}

static void long_fetch_16_0 (int hpos, int nwords, int dma) { long_fetch_16 (hpos, nwords, 0, dma); }
static void long_fetch_16_1 (int hpos, int nwords, int dma) { long_fetch_16 (hpos, nwords, 1, dma); }
static void long_fetch_32_0 (int hpos, int nwords, int dma) { long_fetch_32 (hpos, nwords, 0, dma); }
static void long_fetch_32_1 (int hpos, int nwords, int dma) { long_fetch_32 (hpos, nwords, 1, dma); }
static void long_fetch_64_0 (int hpos, int nwords, int dma) { long_fetch_64 (hpos, nwords, 0, dma); }
static void long_fetch_64_1 (int hpos, int nwords, int dma) { long_fetch_64 (hpos, nwords, 1, dma); }

static void do_long_fetch (int hpos, int nwords, int dma, int fm)
{
	int i;

	flush_display (fm);
	beginning_of_plane_block (hpos, fm);

	switch (fm) {
		case 0:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_16_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_16_0 (i, nwords, dma);
			}
			break;
		case 1:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_32_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_32_0 (i, nwords, dma);
			}
			break;
		case 2:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_64_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_64_0 (i, nwords, dma);
			}
			break;
	}

	out_nbits += nwords * 16;
	out_offs += out_nbits >> 5;
	out_nbits &= 31;
	delay_cycles += nwords * 16;

  if (dma && toscr_nr_planes > 0)
		fetch_state = fetch_was_plane0;
}

STATIC_INLINE void finish_last_fetch (int pos, int fm, bool reallylast)
{
  if (thisline_decision.plfleft < 0)
    return;
	if (plfr_state >= plfr_end)
    return;
	plfr_state = plfr_end;

	flush_display (fm);
	// This may not be the last fetch, store current endpos for future use.
	// There is at least one demo that has two DDFSTRT-DDFSTOP horizontal sections
	// Subtle Shades / Nuance.
  thisline_decision.plfright = pos;

	if (!reallylast) {
	  if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
		  ddfstate = DIW_waiting_start;
		  fetch_state = fetch_not_started;
    }
	}
}

static void reset_bpl_vars(void)
{
	out_nbits = 0;
	out_offs = 0;
	toscr_nbits = 0;
	thisline_decision.bplres = bplcon0_res;
}

/* check special case where last fetch wraps to next line
 * this makes totally corrupted and flickering display on
 * real hardware due to refresh cycle conflicts
 */
static void maybe_finish_last_fetch (int pos, int fm)
{
	if (plf_state > plf_passed_stop2 || plf_state < plf_passed_stop || (fetch_state != fetch_started && fetch_state != fetch_started_first) || !dmaen (DMA_BITPLANE) || bitplane_maybe_start_hpos >= 0x100) {
		finish_last_fetch (pos, fm, true);
	} else {
		SET_LINE_CYCLEBASED;
		finish_last_fetch(pos, fm, true);
	}
}


/* make sure fetch that goes beyond maxhpos is finished */
STATIC_INLINE void finish_final_fetch (void)
{
	if (thisline_decision.plfleft < 0)
			return;

	if (plfr_state < plfr_end)
		finish_last_fetch (maxhpos, fetchmode, true);
	plfr_state = plfr_finished;

	// workaround for too long fetches that don't pass plf_passed_stop2 before end of scanline
	if (aga_plf_passed_stop2 && plf_state >= plf_passed_stop)
		plf_state = plf_end;
	
	// This is really the end of scanline, we can finally flush all remaining data.
	thisline_decision.plfright += flush_plane_data (fetchmode);
	// This can overflow if display setup is really bad.
	if (out_offs > MAX_PIXELS_PER_LINE / 32)
		out_offs = MAX_PIXELS_PER_LINE / 32;
	thisline_decision.plflinelen = out_offs;
}

STATIC_INLINE int one_fetch_cycle_0 (int pos, int dma, int fm)
{
	bool bplactive = true;
	bool diw = diwstate == DIW_waiting_stop;
	if (plf_state == plf_wait && dma && diw) {
		// same timings as when switching off, see below
		bpl_dma_off_when_active = 0;
		bplactive = false;
		if (bitplane_off_delay >= 0)
			bitplane_off_delay = !dma ? -4 : -5;
		if (bitplane_off_delay < 0) {
			bitplane_off_delay++;
			if (bitplane_off_delay == 0) {
				if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
					plf_state = plf_passed_stop;
				} else {
					plf_state = plf_active;
				}
			}
		}
	} else if (!dma || !diw) {
		bplactive = false;
		// dma off: turn off bitplane output after 4 cycles
		// (yes, switching DMA off won't disable it immediately)
		// diw off: turn off bitplane output after 5 cycles
		// (Starflight / Phenomena jumping scroller in ECS)
		// This is not correctly emulated, there probably is
		// 4+ stage shift register that causes these delays.
		if (plf_state == plf_active || plf_state == plf_passed_stop || plf_state == plf_passed_stop_act) {
			bpl_dma_off_when_active = 1;
			if (bitplane_off_delay <= 0)
				bitplane_off_delay = !dma ? 4 : 5;
		}
		if (bitplane_off_delay > 0) {
			bplactive = true;
			bitplane_off_delay--;
			if (bitplane_off_delay == 0) {
				bplactive = false;
				plf_state = plf_wait;
			}
		}
	}

	if ((dma && diw) || (currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
		if (plf_state != plf_wait) {
			if (pos == plfstop && ddfstop_written_hpos != pos) {
				if (plf_state < plf_passed_stop) {
					plf_state = plf_passed_stop;
				}
				plf_end_hpos = pos + DDF_OFFSET;
			} else if (pos == plf_end_hpos) {
				ddfstop_matched = true;
				if (plf_state < plf_passed_stop_act) {
					plf_state = plf_passed_stop_act;
				}
			}
		}
	}

	if ((fetch_cycle & fetchunit_mask) == 0) {
		if (plf_state == plf_passed_stop2) {
			finish_last_fetch(pos, fm, false);
			return 1;
		}
		if (plf_state == plf_passed_stop_act) {
			plf_state = plf_passed_stop2;
		}
	}

	// must be after above test, otherwise same fetch
	// block may pass both stop_act and stop2 tests.
	if (pos == HARD_DDF_STOP) {
		if (plf_state < plf_wait) {
			plf_state = plf_passed_stop_act;
    }
	}

	maybe_check (pos);

	if (bplactive) {
	  /* fetchstart_mask can be larger than fm_maxplane if FMODE > 0.  This means
	   that the remaining cycles are idle; we'll fall through the whole switch
	   without doing anything.  */
		int cycle_start = fetch_cycle & fetchstart_mask;
		bool modulo =  plf_state == plf_passed_stop2 && fetch_cycle >= (fetch_cycle & ~fetchunit_mask) + fetch_modulo_cycle;

		switch (fm_maxplane) {
			case 8:
				switch (cycle_start) {
			    case 0: fetch (7, fm, modulo, pos); break;
			    case 1: fetch (3, fm, modulo, pos); break;
			    case 2: fetch (5, fm, modulo, pos); break;
			    case 3: fetch (1, fm, modulo, pos); break;
			    case 4: fetch (6, fm, modulo, pos); break;
			    case 5: fetch (2, fm, modulo, pos); break;
			    case 6: fetch (4, fm, modulo, pos); break;
			    case 7: fetch (0, fm, modulo, pos); break;
			    default:
			    // if AGA: consider plf_passed_stop2 already
			    // active when last plane has been written,
			    // even if there is still idle cycles left
			    if (!aga_plf_passed_stop2 && plf_state >= plf_passed_stop_act)
				    aga_plf_passed_stop2 = pos + ((8 << fetchmode) - cycle_start);
			    break;
				}
				break;
			case 4:
				switch (cycle_start) {
			    case 0: fetch (3, fm, modulo, pos); break;
			    case 1: fetch (1, fm, modulo, pos); break;
			    case 2: fetch (2, fm, modulo, pos); break;
			    case 3: fetch (0, fm, modulo, pos); break;
			    default:
			    if (!aga_plf_passed_stop2 && plf_state >= plf_passed_stop_act)
				    aga_plf_passed_stop2 = pos + ((8 << fetchmode) - cycle_start);
			    break;
				}
				break;
			case 2:
				switch (cycle_start) {
			    case 0: fetch (1, fm, modulo, pos); break;
			    case 1: fetch (0, fm, modulo, pos); break;
			    default:
			    if (!aga_plf_passed_stop2 && plf_state >= plf_passed_stop_act)
				    aga_plf_passed_stop2 = pos + ((8 << fetchmode) - cycle_start);
			    break;
				}
				break;
		}
	}

	if (bpl1dat_written) {
		// do this here because if program plays with BPLCON0 during scanline
		// it is possible that one DMA BPL1DAT write is completely missed
		// and we must not draw anything at all in next dma block if this happens
		// (Disposable Hero titlescreen)
		fetch_state = fetch_was_plane0;
		bpl1dat_written = false;
	}

	fetch_cycle++;
	toscr_nbits += toscr_res2p;

	if (bplcon1_written) {
		flush_display (fm);
		compute_toscr_delay (bplcon1);
		bplcon1_written = false;
	}

	if (toscr_nbits > 16) {
		uae_abort (_T("toscr_nbits > 16 (%d)"), toscr_nbits);
		toscr_nbits = 0;
	}

	if (toscr_nbits == 16)
		flush_display (fm);
	
	return 0;
}

static int one_fetch_cycle_fm0 (int pos, int dma) { return one_fetch_cycle_0 (pos, dma, 0); }
static int one_fetch_cycle_fm1 (int pos, int dma) { return one_fetch_cycle_0 (pos, dma, 1); }
static int one_fetch_cycle_fm2 (int pos, int dma) { return one_fetch_cycle_0 (pos, dma, 2); }

STATIC_INLINE int one_fetch_cycle (int pos, int dma, int fm)
{
  switch (fm) {
	  case 0: return one_fetch_cycle_fm0 (pos, dma);
	  case 1: return one_fetch_cycle_fm1 (pos, dma);
	  case 2: return one_fetch_cycle_fm2 (pos, dma);
	  default: uae_abort (_T("fm corrupt")); return 0;
  }
}

static void update_fetch_x (int until, int fm)
{
	int pos;

	if (nodraw ())
		return;

	pos = last_fetch_hpos;
	update_toscr_planes (fm);

	// not optimized, update_fetch_x() is extremely rarely used.
	for (; pos < until; pos++) {

		toscr_nbits += toscr_res2p;

		if (toscr_nbits > 16) {
			uae_abort (_T("xtoscr_nbits > 16 (%d)"), toscr_nbits);
			toscr_nbits = 0;
		}
		if (toscr_nbits == 16)
			flush_display (fm);

	}

	if (until >= maxhpos) {
		maybe_finish_last_fetch (pos, fm);
		return;
	}

	flush_display (fm);
}

STATIC_INLINE void update_fetch (int until, int fm)
{
	int pos;
	int dma = dmaen (DMA_BITPLANE);

  if (nodraw() || plf_state >= plf_end)
		return;

	pos = last_fetch_hpos;
  cycle_diagram_shift = last_fetch_hpos - fetch_cycle;

	/* First, a loop that prepares us for the speedup code.  We want to enter
	the SPEEDUP case with fetch_state == fetch_was_plane0 or it is the very
	first fetch cycle (which equals to same state as fetch_was_plane0)
    and then unroll whole blocks, so that we end on the same fetch_state again.  */
	for (; ; pos++) {
		if (pos == until) {
			if (until >= maxhpos) {
				maybe_finish_last_fetch (pos, fm);
				return;
			}
			return;
		}

		if (fetch_state == fetch_was_plane0)
			break;
		fetch_start ();
		if (one_fetch_cycle (pos, dma, fm))
		  return;
	}

	/* Unrolled version of the for loop below.  */
	if (plf_state == plf_active && !line_cyclebased && dma
		&& (fetch_cycle & fetchstart_mask) == (fm_maxplane & fetchstart_mask)
		&& !badmode
		&& toscr_nr_planes == toscr_nr_planes_agnus)
	{
		int ddfstop_to_test_ddf = HARD_DDF_STOP;
		if (plfstop >= last_fetch_hpos - DDF_OFFSET && plfstop < ddfstop_to_test_ddf)
			ddfstop_to_test_ddf = plfstop;
		int ddfstop_to_test = ddfstop_to_test_ddf + DDF_OFFSET;
		int offs = (pos - fetch_cycle) & fetchunit_mask;
		int ddf2 = ((ddfstop_to_test - offs + fetchunit - 1) & ~fetchunit_mask) + offs;
		int ddf3 = ddf2 + fetchunit;
		int stop = until < ddf2 ? until : until < ddf3 ? ddf2 : ddf3;
		int count;

		count = stop - pos;
		if (count >= fetchstart) {
			count &= ~fetchstart_mask;
			int stoppos = pos + count;

			if (thisline_decision.plfleft < 0) {
				compute_toscr_delay (bplcon1);
			}

			do_long_fetch (pos, count >> (3 - toscr_res), dma, fm);

	    /* This must come _after_ do_long_fetch so as not to confuse flush_display
	       into thinking the first fetch has produced any output worth emitting to
	       the screen.  But the calculation of delay_offset must happen _before_.  */
			maybe_first_bpl1dat (pos);

			if (pos <= plfstop && stoppos > plfstop) {
				plf_state = plf_passed_stop;
				plf_end_hpos = plfstop + DDF_OFFSET;
			}
			if (pos <= plfstop + DDF_OFFSET && stoppos > plfstop + DDF_OFFSET) {
				plf_state = plf_passed_stop_act;
				plf_end_hpos = 256 + DDF_OFFSET;
				ddfstop_matched = true;
			}
			if (pos <= HARD_DDF_STOP && stoppos > HARD_DDF_STOP) {
				if (plf_state < plf_wait)
					plf_state = plf_passed_stop_act;
      }
	    if (pos <= ddfstop_to_test && stoppos > ddf2) {
				plf_state = plf_passed_stop2;
      }
 	    if (pos <= ddf2 && stoppos >= ddf2 + fm_maxplane) {
			  add_modulos ();
      }
			pos += count;
			fetch_cycle += count;
		}
	}

	for (; pos < until; pos++) {
		if (fetch_state == fetch_was_plane0) {
			flush_display (fm);
			beginning_of_plane_block (pos, fm);
    }

		fetch_start ();

		if (one_fetch_cycle (pos, dma, fm))
		   return;
	}
	if (until >= maxhpos) {
		maybe_finish_last_fetch (pos, fm);
		return;
	}
	flush_display (fm);
}

static void update_fetch_0 (int hpos) { update_fetch (hpos, 0); }
static void update_fetch_1 (int hpos) { update_fetch (hpos, 1); }
static void update_fetch_2 (int hpos) { update_fetch (hpos, 2); }

STATIC_INLINE void decide_fetch (int hpos)
{
	if (hpos > last_fetch_hpos) {
		if (fetch_state != fetch_not_started) {
	    switch (fetchmode) {
	      case 0: update_fetch_0 (hpos); break;
	      case 1: update_fetch_1 (hpos); break;
	      case 2: update_fetch_2 (hpos); break;
			  default: uae_abort (_T("fetchmode corrupt"));
	    }
		} else if (bpl1dat_written_at_least_once) {
			// "PIO" mode display
			update_fetch_x (hpos, fetchmode);
			bpl1dat_written = false;
	  }

		maybe_check (hpos);
	  last_fetch_hpos = hpos;
  }
}

STATIC_INLINE void decide_fetch_safe (int hpos)
{
	decide_fetch (hpos);
	decide_blitter (hpos);
}

STATIC_INLINE void start_bpl_dma (int hstart)
{
	fetch_state = fetch_started;
	plfr_state = plfr_active;
  ddfstate = DIW_waiting_stop;
	bpl_hstart = hstart;

	if (!bpldmawasactive) {

		if (last_fetch_hpos < 0)
			last_fetch_hpos = 0;
		plfstrt_sprite = hstart;
		// OCS Agnus needs at least 1 empty cycle between
		// sprite fetch and bitplane cycle sequence start.
		if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
			plfstrt_sprite--;
    fetch_cycle = 0;
		update_denise (last_fetch_hpos);
	  if (bpl1dat_written_at_least_once && hstart > last_fetch_hpos) {
		  update_fetch_x (hstart, fetchmode);
		  bpl1dat_written_at_least_once = false;
	  } else {
	    reset_bpl_vars ();
	  }
	  cycle_diagram_shift = hstart;

		bpldmawasactive = true;

	} else {

		// Bitplane DMA restart during FMODE>0 idle cycles?
		if (aga_plf_passed_stop2) {
			if (aga_plf_passed_stop2 > hstart)
				aga_plf_passed_stop2 = hstart;
			thisline_decision.plfright = aga_plf_passed_stop2;
			aga_plf_passed_stop2 = 0;
		}

		flush_display (fetchmode);
		// Calculate difference between last end to new start
		int diff = (hstart - thisline_decision.plfright) << (1 + toscr_res);
		// Render all missing pixels, use toscr because previous data may
		// still be in buffers.
		while (diff >= 16) {
			toscr_1 (16, fetchmode);
			diff -= 16;
		}
		if (diff)
			toscr_1 (diff, fetchmode);

		fetch_cycle = 0;
		cycle_diagram_shift = hstart;
		update_denise (last_fetch_hpos);
		update_fetch_x (hstart, fetchmode);
  }

  last_fetch_hpos = hstart;
	estimate_last_fetch_cycle (hstart);
}

/* This function is responsible for turning on datafetch if necessary.  */
static void decide_line (int hpos)
{
	bool ecs = (currprefs.chipset_mask & CSMASK_ECS_AGNUS) != 0;

	/* Take care of the vertical DIW.  */
	if (vpos == plffirstline) {
		if (vpos >= 0) {
      diwstate = DIW_waiting_stop;
		  SET_LINE_CYCLEBASED;
	  }
	}
	// last line of field can never have bitplane dma active if not A1000 Agnus.
	if (vpos == plflastline) {
		diwstate = DIW_waiting_start;
		SET_LINE_CYCLEBASED;
	}
   
  if (hpos <= last_decide_line_hpos)
    return;

	bool dma = dmaen (DMA_BITPLANE) != 0;
	bool diw = diwstate == DIW_waiting_stop;

	if (ecs) {
		if (1) {
			if (last_decide_line_hpos < plfstrt && hpos >= plfstrt) {
				ddfstop_matched = false;
			}
		}
	} else {
		if (1) {
			if (last_decide_line_hpos < plfstrt && hpos >= plfstrt) {
				ddfstop_matched = false;
				// plfstrt==0 works strangely (Nakudemo / Vision-X)
				if (plfstrt > -DDF_OFFSET)
					ocs_agnus_ddf_enable_toggle = false;
			}
		}
	}

	if (fetch_state == fetch_not_started || (aga_plf_passed_stop2 && plfstrt >= last_decide_line_hpos)) {
		bool strtpassed = false;
		plfstate nextstate = plf_end;
		int hstart;

		hstart = last_decide_line_hpos;
		if (hstart < bitplane_maybe_start_hpos)
			hstart = bitplane_maybe_start_hpos;
		if (hstart < HARD_DDF_START_REAL + DDF_OFFSET)
			hstart = HARD_DDF_START_REAL + DDF_OFFSET;
		// DMA enabled mid-line: DDF_OFFSET delay first
		if (bitplane_maybe_start_hpos + DDF_OFFSET > hstart)
			hstart = bitplane_maybe_start_hpos + DDF_OFFSET;
		if (hstart & 1)
			hstart++;

		if (ecs) {
			// ECS DDFSTRT/STOP matching does not require DMA or DIW.
			if (1) {
				if (last_decide_line_hpos < plfstrt && hpos >= plfstrt) {
					// active == already started because ddfstop was not detected in last line
					if (plf_state != plf_active) {
						plf_state = plf_passed_start;
						strtpassed = true;
						plf_start_hpos = plfstrt + DDF_OFFSET;
					}
				}
			}
			if (1) {
				if ((strtpassed && hpos >= plf_start_hpos) || (last_decide_line_hpos < plf_start_hpos && hpos >= plf_start_hpos)) {
					if (plf_state == plf_passed_start) {
						plf_state = plf_active;
						hstart = plf_start_hpos;
					}
				}
			}
		} else {
			if (1) {
				int start = HARD_DDF_START_REAL;
				if (last_decide_line_hpos < start && hpos >= start) {
					if (!ocs_agnus_ddf_enable_toggle)
						plf_state = plf_passed_enable;
					ocs_agnus_ddf_enable_toggle = true;
				}
			}
			// OCS DDFSTRT/STOP matching requires DMA and DIW enabled.
			if (dma && diw) {
				if (last_decide_line_hpos < plfstrt && hpos >= plfstrt) {
					if (plf_state == plf_passed_enable) {
						plf_state = plf_passed_start;
						strtpassed = true;
						plf_start_hpos = plfstrt + DDF_OFFSET;
					}
					ocs_agnus_ddf_enable_toggle = false;
				}
			}
			if (dma && diw) {
				if ((strtpassed && hpos >= plf_start_hpos) || (last_decide_line_hpos < plf_start_hpos && hpos >= plf_start_hpos)) {
					if (plf_state == plf_passed_start) {
						plf_state = plf_active;
						hstart = plf_start_hpos;
					}
				}
			}
		}

		if (diw && dma) {
			bool test = false;
			if (ecs) {
				test = (plf_state == plf_active && (hpos >= HARD_DDF_START_REAL + DDF_OFFSET || HARD_DDF_LIMITS_DISABLED));
				if (bpl_dma_off_when_active) {
					if (plfstop < hstart) {
						test = false;
					}
				}
			} else {
				test = (plf_state == plf_active);
				// if DMA enabled mid-scanline but ddfstrt not matched (dma was off): start when ddfstop is matched
				// (Crash Landing crack intro / Scoopex)
				if (!test && last_decide_line_hpos < plfstop && hstart > plfstop) {
					if (hstart == ((bitplane_maybe_start_hpos + DDF_OFFSET + 1) & ~1)) {
						hstart = plfstop + DDF_OFFSET;
						test = true;
						nextstate = plf_passed_stop;
						// inform overrun code that this won't overrun.
						bitplane_maybe_start_hpos = 0x100;
					}
				}
			}
			if (test) {
				start_bpl_dma (hstart);
				// if ECS: pre-set plf_end_hpos if we have already passed virtual ddfstop
				if (ecs) {
					if (last_decide_line_hpos < hstart && hstart >= plfstop && hstart - plfstop <= DDF_OFFSET) {
						plf_end_hpos = plfstop + DDF_OFFSET;
						nextstate = plf_passed_stop;
					}
					if (last_decide_line_hpos < HARD_DDF_STOP && hstart > HARD_DDF_STOP) {
						plf_end_hpos = HARD_DDF_STOP + DDF_OFFSET;
						nextstate = plf_passed_stop;
					}
					if (bpl_dma_off_when_active) {
						nextstate = plf_passed_stop_act;
						bpl_dma_off_when_active = 0;
					}
				}
				if (nextstate != plf_end) {
					plf_state = nextstate;
					estimate_last_fetch_cycle(hstart);
				}
				last_decide_line_hpos = hpos;
				do_sprites (hpos);
				return;
			}

		}

		if (ecs) {
			if (1) {
				// ddfstrt == ddfstop: ddfstrt wins.
				if (plfstrt != plfstop && last_decide_line_hpos < plfstop && hpos >= plfstop && plfstop <= maxhpos - DDF_OFFSET) {
					ddfstop_matched = true;
					if (plf_state != plf_wait && plf_state < plf_passed_stop) {
						plf_state = plf_passed_stop;
						plf_end_hpos = plfstop + DDF_OFFSET;
					}
				}
				if (last_decide_line_hpos < HARD_DDF_STOP && hpos >= HARD_DDF_STOP) {
					plf_state = plf_passed_stop_act;
				}
			}
		} else {
			if (dma && diw) {
				if (last_decide_line_hpos < plfstop && hpos >= plfstop && plfstop <= maxhpos - DDF_OFFSET && plf_state != plf_wait) {
					ddfstop_matched = true;
				}
			}
		}
	}

  if (hpos > last_sprite_hpos && last_sprite_hpos < SPR0_HPOS + 4 * MAX_SPRITES)
    do_sprites (hpos);
   
  last_decide_line_hpos = hpos;
}

/* Called when a color is about to be changed (write to a color register),
 * but the new color has not been entered into the table yet. */
static void record_color_change (int hpos, int regno, unsigned long value)
{
	if (regno < 0x1000 && nodraw ())
		return;
	/* Early positions don't appear on-screen. */
  if (vpos < minfirstline)
		return;
	
	decide_diw (hpos);
	decide_line (hpos);
	
	if (thisline_decision.ctable < 0)
		remember_ctable ();
	
  if ((regno < 0x1000 || regno == 0x1000 + 0x10c) && hpos < HBLANK_OFFSET && !(beamcon0 & 0x80) && prev_lineno >= 0) {
  	struct draw_info *pdip = curr_drawinfo + prev_lineno;
  	int idx = pdip->last_color_change;
		int extrahpos = regno == 0x1000 + 0x10c ? 1 : 0;
		bool lastsync = false;
		/* Move color changes in horizontal cycles 0 to HBLANK_OFFSET to end of previous line.
		* Cycles 0 to HBLANK_OFFSET are visible in right border on real Amigas. (because of late hsync)
		*/
		if (curr_color_changes[idx - 1].regno == 0xffff) {
			idx--;
			lastsync = true;
		}
    pdip->last_color_change++;
    pdip->nr_color_changes++;
		curr_color_changes[idx].linepos = ((hpos + maxhpos) * 2 + extrahpos) * 4;
    curr_color_changes[idx].regno = regno;
    curr_color_changes[idx].value = value;
		if (lastsync) {
			curr_color_changes[idx + 1].linepos = (hsyncstartpos * 2) * 4;
			curr_color_changes[idx + 1].regno = 0xffff;
			curr_color_changes[idx + 2].regno = -1;
		} else {
	    curr_color_changes[idx + 1].regno = -1;
		}
  }
  record_color_change2 (hpos, regno, value);
}

static bool isbrdblank (int hpos, uae_u16 bplcon0, uae_u16 bplcon3)
{
	bool brdblank;
	brdblank = (currprefs.chipset_mask & CSMASK_ECS_DENISE) && (bplcon0 & 1) && (bplcon3 & 0x20);
	if (hpos >= 0 && (ce_is_borderblank(current_colors.extra) != brdblank)) {
		record_color_change (hpos, 0, COLOR_CHANGE_BRDBLANK | (brdblank ? 1 : 0) | (ce_is_bordersprite(current_colors.extra) ? 2 : 0));
		current_colors.extra &= ~(1 << CE_BORDERBLANK);
		current_colors.extra |= brdblank ? (1 << CE_BORDERBLANK) : 0;
		remembered_color_entry = -1;
	}
	return brdblank;
}

static bool issprbrd (int hpos, uae_u16 bplcon0, uae_u16 bplcon3)
{
	bool brdsprt;
	brdsprt = (aga_mode) && (bplcon0 & 1) && (bplcon3 & 0x02);
	if (hpos >= 0 && ce_is_bordersprite(current_colors.extra) != brdsprt) {
		record_color_change (hpos, 0, COLOR_CHANGE_BRDBLANK | (ce_is_borderblank(current_colors.extra) ? 1 : 0) | (brdsprt ? 2 : 0));
		current_colors.extra &= ~(1 << CE_BORDERSPRITE);
		current_colors.extra |= brdsprt ? (1 << CE_BORDERSPRITE) : 0;
		remembered_color_entry = -1;
		if (brdsprt && !ce_is_borderblank(current_colors.extra))
			thisline_decision.bordersprite_seen = true;
	}
	return brdsprt && !ce_is_borderblank(current_colors.extra);
}

static void record_register_change (int hpos, int regno, uae_u16 value)
{
  if (regno == 0x100) { // BPLCON0
	  if (value & 0x800)
	    thisline_decision.ham_seen = 1;
		isbrdblank (hpos, value, bplcon3);
		issprbrd (hpos, value, bplcon3);
	} else if (regno == 0x106) { // BPLCON3
		isbrdblank (hpos, bplcon0, value);
		issprbrd (hpos, bplcon0, value);
  }
  record_color_change (hpos, regno + 0x1000, value);
}

typedef int sprbuf_res_t, cclockres_t, hwres_t, bplres_t;

static int expand_sprres (uae_u16 con0, uae_u16 con3)
{
  int res;

	switch ((con3 >> 6) & 3) 
  {
    default:
    	res = RES_LORES;
      break;
		case 0: /* ECS defaults (LORES,HIRES=LORES sprite,SHRES=HIRES sprite) */
		  if ((currprefs.chipset_mask & CSMASK_ECS_DENISE) && GET_RES_DENISE (con0) == RES_SUPERHIRES)
			  res = RES_HIRES;
		  else
			  res = RES_LORES;
			break;
		case 1:
			res = RES_LORES;
			break;
		case 2:
			res = RES_HIRES;
			break;
		case 3:
			res = RES_SUPERHIRES;
			break;
	}
  return res;
}

#define DO_PLAYFIELD_COLLISIONS \
{ \
   if (!clxcon_bpl_enable) \
      clxdat |= 1; \
   else \
      if (!(clxdat & 1)) \
         do_playfield_collisions (); \
}

/* handle very rarely needed playfield collision (CLXDAT bit 0) */
/* only known game needing this is Rotor */
static void do_playfield_collisions (void)
{
  int bplres = bplcon0_res;
  hwres_t ddf_left = thisline_decision.plfleft * 2 << bplres;
  hwres_t hw_diwlast = coord_window_to_diw_x (thisline_decision.diwlastword);
  hwres_t hw_diwfirst = coord_window_to_diw_x (thisline_decision.diwfirstword);
  int i, collided, minpos, maxpos;
  int planes = (aga_mode) ? 8 : 6;

  collided = 0;
  minpos = thisline_decision.plfleft * 2;
  if (minpos < hw_diwfirst)
    minpos = hw_diwfirst;
  maxpos = thisline_decision.plfright * 2;
  if (maxpos > hw_diwlast)
    maxpos = hw_diwlast;
  for (i = minpos; i < maxpos && !collided; i+= 32) {
    int offs = ((i << bplres) - ddf_left) >> 3;
    int j;
    uae_u32 total = 0xffffffff;
    for (j = 0; j < planes; j++) {
      int ena = (clxcon_bpl_enable >> j) & 1;
      int match = (clxcon_bpl_match >> j) & 1;
      uae_u32 t = 0xffffffff;
      if (ena) {
        if (j < thisline_decision.nr_planes) {
          t = *(uae_u32 *)(line_data[next_lineno] + offs + 2 * j * MAX_WORDS_PER_LINE);
          t ^= (match & 1) - 1;
        } else {
          t = (match & 1) - 1;
        }
      }
      total &= t;
    }
    if (total) {
      collided = 1;
    }
  }
  if (collided)
    clxdat |= 1;
}

#define DO_SPRITE_COLLISIONS \
{ \
  if (clxcon_bpl_enable || curr_drawinfo[next_lineno].nr_sprites) { \
    /* all sprite to bitplane collision bits already set? */ \
    if ((clxdat & 0x1fe) != 0x1fe) \
      do_sprite_collisions (); \
  } \
}

/* Sprite-to-sprite collisions are taken care of in record_sprite.  This one does
   playfield/sprite collisions. */
static void do_sprite_collisions (void)
{
  int nr_sprites = curr_drawinfo[next_lineno].nr_sprites;
  int first = curr_drawinfo[next_lineno].first_sprite_entry;
  int i;
  unsigned int collision_mask = clxmask[clxcon >> 12];
  int bplres = bplcon0_res;
  hwres_t ddf_left = thisline_decision.plfleft * 2 << bplres;
  hwres_t hw_diwlast = coord_window_to_diw_x (thisline_decision.diwlastword);
  hwres_t hw_diwfirst = coord_window_to_diw_x (thisline_decision.diwfirstword);

  for (i = 0; i < nr_sprites; i++) {
    struct sprite_entry *e = curr_sprite_entries + first + i;
    sprbuf_res_t j;
    sprbuf_res_t minpos = e->pos;
    sprbuf_res_t maxpos = e->max;
    hwres_t minp1 = minpos >> sprite_buffer_res;
    hwres_t maxp1 = maxpos >> sprite_buffer_res;

    if (maxp1 > hw_diwlast)
      maxpos = hw_diwlast << sprite_buffer_res;
    if (maxp1 > thisline_decision.plfright * 2)
      maxpos = thisline_decision.plfright * 2 << sprite_buffer_res;
    if (minp1 < hw_diwfirst)
      minpos = hw_diwfirst << sprite_buffer_res;
    if (minp1 < thisline_decision.plfleft * 2)
      minpos = thisline_decision.plfleft * 2 << sprite_buffer_res;

    for (j = minpos; j < maxpos; j++) {
      int sprpix = spixels[e->first_pixel + j - e->pos] & collision_mask;
      int k, offs, match = 1;

      if (sprpix == 0)
        continue;

      offs = ((j << bplres) >> sprite_buffer_res) - ddf_left;
      sprpix = sprite_ab_merge[sprpix & 255] | (sprite_ab_merge[sprpix >> 8] << 2);
      sprpix <<= 1;

			// both odd and even collision bits already set?
			if ((clxdat & (sprpix << 0)) && (clxdat & (sprpix << 4)))
				continue;

      /* Loop over number of playfields.  */
      for (k = 1; k >= 0; k--) {
        int l;
        int planes = (aga_mode) ? 8 : 6;
        if (bplcon0 & 0x400)
          match = 1;
        for (l = k; match && l < planes; l += 2) {
          int t = 0;
          if (l < thisline_decision.nr_planes) {
            uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + 2 * l * MAX_WORDS_PER_LINE);
            uae_u32 word = ldata[offs >> 5];
            t = (word >> (31 - (offs & 31))) & 1;
          }
          if (clxcon_bpl_enable & (1 << l)) {
            if (t != ((clxcon_bpl_match >> l) & 1))
              match = 0;
          }
        }
        if (match) {
          clxdat |= sprpix << (k * 4);
        }
      }
    }
  }
}

static void record_sprite_1 (int sprxp, uae_u16 *buf, uae_u32 datab, int num, int dbl,
  unsigned int mask, int do_collisions, uae_u32 collision_mask)
{
	int j = 0;
	while (datab) {
	  unsigned int col = 0;
    unsigned int coltmp = 0;
		
	  if ((sprxp >= sprite_minx && sprxp < sprite_maxx) || (bplcon3 & 2))
      col = (datab & 3) << (2 * num);
		if ((j & mask) == 0) {
 	    unsigned int tmp = (*buf) | col;
			*buf++ = tmp;
	    if (do_collisions)
		    coltmp |= tmp;
	    sprxp++;
    }
	  if (dbl > 0) {
    	unsigned int tmp = (*buf) | col;
			*buf++ = tmp;
	    if (do_collisions)
    		coltmp |= tmp;
	    sprxp++;
	  }
	  if (dbl > 1) {
	    unsigned int tmp;
	    tmp = (*buf) | col;
	    *buf++ = tmp;
	    if (do_collisions)
		    coltmp |= tmp;
	    tmp = (*buf) | col;
	    *buf++ = tmp;
	    if (do_collisions)
		    coltmp |= tmp;
	    sprxp++;
	    sprxp++;
    }
		j++;
		datab >>= 2;
		if (do_collisions) {
			coltmp &= collision_mask;
			if (coltmp) {
				unsigned int shrunk_tmp = sprite_ab_merge[coltmp & 255] | (sprite_ab_merge[coltmp >> 8] << 2);
				clxdat |= sprclx[shrunk_tmp];
			}
		}
	}
}

/* DATAB contains the sprite data; 16 pixels in two-bit packets.  Bits 0/1
   determine the color of the leftmost pixel, bits 2/3 the color of the next
   etc.
   This function assumes that for all sprites in a given line, SPRXP either
   stays equal or increases between successive calls.

   The data is recorded either in lores pixels (if OCS/ECS), or in hires or
   superhires pixels (if AGA).  */

static void record_sprite (int line, int num, int sprxp, uae_u16 *data, uae_u16 *datb, unsigned int ctl)
{
	struct sprite_entry *e = curr_sprite_entries + next_sprite_entry;
	int i;
	int word_offs;
	uae_u32 collision_mask;
	int width, dbl, half;
	unsigned int mask = 0;
  int attachment;
	
  half = 0;
	dbl = sprite_buffer_res - sprres;
  if (dbl < 0) {
    half = -dbl;
    dbl = 0;
	  mask = 1 << half;
  }
  width = (sprite_width << sprite_buffer_res) >> sprres;
  attachment = sprctl[num | 1] & 0x80;

	/* Try to coalesce entries if they aren't too far apart  */
  if (! next_sprite_forced && e[-1].max + sprite_width >= sprxp) {
		e--;
	} else {
		next_sprite_entry++;
		e->pos = sprxp;
		e->has_attached = 0;
	}
	
	if (sprxp < e->pos)
		uae_abort (_T("sprxp < e->pos"));

	e->max = sprxp + width;
	e[1].first_pixel = e->first_pixel + ((e->max - e->pos + 3) & ~3);
	next_sprite_forced = 0;
	
	collision_mask = clxmask[clxcon >> 12];
	word_offs = e->first_pixel + sprxp - e->pos;
	
	for (i = 0; i < sprite_width; i += 16) {
		unsigned int da = *data;
		unsigned int db = *datb;
		uae_u32 datab = ((sprtaba[da & 0xFF] << 16) | sprtaba[da >> 8]
		                 | (sprtabb[db & 0xFF] << 16) | sprtabb[db >> 8]);
    int off = (i << dbl) >> half;
		uae_u16 *buf = spixels + word_offs + off;
  	if (currprefs.collision_level > 0 && collision_mask)
  		record_sprite_1 (sprxp + off, buf, datab, num, dbl, mask, 1, collision_mask);
	  else
		  record_sprite_1 (sprxp + off, buf, datab, num, dbl, mask, 0, collision_mask);
		data++;
		datb++;
	}
	
	/* We have 8 bits per pixel in spixstate, two for every sprite pair.  The
       low order bit records whether the attach bit was set for this pair.  */
  if (attachment && !ecsshres ()) {
		uae_u8 state = 0x03 << (num & ~1);
		uae_u8 *stb1 = spixstate.bytes + word_offs;	
		for (i = 0; i < width; i += 8) {
			stb1[0] |= state;
			stb1[1] |= state;
			stb1[2] |= state;
			stb1[3] |= state;
			stb1[4] |= state;
			stb1[5] |= state;
			stb1[6] |= state;
			stb1[7] |= state;
			stb1 += 8;
  	}
		e->has_attached = 1;
	}
}

static void add_sprite (int *countp, int num, int sprxp, int posns[], int nrs[])
{
  int count = *countp;
  int j, bestp;

	/* Sort the sprites in order of ascending X position before recording them.  */
	for (bestp = 0; bestp < count; bestp++) {
		if (posns[bestp] > sprxp)
			break;
    if (posns[bestp] == sprxp && nrs[bestp] < num)
			break;
	}
	for (j = count; j > bestp; j--) {
		posns[j] = posns[j-1];
		nrs[j] = nrs[j - 1];
	}
	posns[j] = sprxp;
	nrs[j] = num;
	count++;
  *countp = count;
}

static int tospritexdiw (int diw)
{
  return  coord_window_to_hw_x (diw - (DIW_DDF_OFFSET << lores_shift)) << sprite_buffer_res;
}
static int tospritexddf (int ddf)
{
  return (ddf * 2 - DIW_DDF_OFFSET) << sprite_buffer_res;
}

static void calcsprite (void)
{
  sprite_maxx = 0x7fff;
  sprite_minx = 0;
  if (thisline_decision.diwlastword >= 0)
  	sprite_maxx = tospritexdiw (thisline_decision.diwlastword);
  if (thisline_decision.diwfirstword >= 0)
  	sprite_minx = tospritexdiw (thisline_decision.diwfirstword);
  if (thisline_decision.plfleft >= 0) {
  	int min, max;
  	min = tospritexddf (thisline_decision.plfleft);
  	max = tospritexddf (thisline_decision.plfright);
		if (min > sprite_minx && min < max) { /* min < max = full line ddf */
			if (currprefs.chipset_mask & CSMASK_ECS_DENISE) {
	      sprite_minx = min;
			} else {
				if (thisline_decision.plfleft >= 0x28 || bpldmawasactive)
					sprite_minx = min;
			}
		}
  	/* sprites are visible from first BPL1DAT write to end of line
		 * ECS Denise/AGA: no limits
		 * OCS Denise: BPL1DAT write only enables sprite if hpos >= 0x28 or so.
  	 * (undocumented feature)
  	 */
  }
}

static void decide_sprites(int spnr, int hpos, bool quick)
{
	int nrs[MAX_SPRITES * 2], posns[MAX_SPRITES * 2];
	int count, i;
	int point;
	int width = sprite_width;
  int sscanmask = 0x100 << sprite_buffer_res;
	int startnr = 0, endnr = MAX_SPRITES - 1;

	if (thisline_decision.plfleft < 0 && !(bplcon3 & 2))
	  return;

	// let sprite shift register empty completely
	// if sprite is at the very edge of right border
	point = hpos * 2;
	if (hpos >= maxhpos)
		point += ((9 - 2) * 2) * sprite_buffer_res;

  if (nodraw () || hpos < 0x14 || nr_armed == 0 || point == last_sprite_point)
		return;
	
	if (spnr >= 0) {
		startnr = spnr;
		endnr = spnr;
	}
	if (!quick) {
	  decide_diw (hpos);
	  decide_line (hpos);
    calcsprite ();
	}
	
	count = 0;
	for (i = startnr; i <= endnr; i++) {
		int xpos = spr[i].xpos;
		int sprxp = (fmode & 0x8000) ? (xpos & ~sscanmask) : xpos;
		int hw_xp = sprxp >> sprite_buffer_res;
		
  	if (xpos < 0)
	    continue;

	  if (! spr[i].armed)
			continue;

    if (hw_xp > last_sprite_point && hw_xp <= point) {
  		add_sprite (&count, i, sprxp, posns, nrs);
		}

  	/* SSCAN2-bit is fun.. */
  	if ((fmode & 0x8000) && !(sprxp & sscanmask)) {
	    sprxp |= sscanmask;
	    hw_xp = sprxp >> sprite_buffer_res;
	    if (hw_xp > last_sprite_point && hw_xp <= point) {
		    add_sprite (&count, MAX_SPRITES + i, sprxp, posns, nrs);
      }
		} else if (!(fmode & 0x80) && xpos >= (2 << sprite_buffer_res) && xpos <= (9 << sprite_buffer_res)) {
			// right border wrap around. SPRxCTL horizontal bits do not matter.
			sprxp += (maxhpos * 2) << sprite_buffer_res;
			hw_xp = sprxp >> sprite_buffer_res;
			if (hw_xp > last_sprite_point && hw_xp <= point) {
				add_sprite(&count, MAX_SPRITES + i, sprxp, posns, nrs);
			}
			// (not really mutually exclusive of SSCAN2-bit but not worth the trouble)
	  }
	}

	for (i = 0; i < count; i++) {
		int nr = nrs[i] & (MAX_SPRITES - 1);    
		record_sprite (next_lineno, nr, posns[i], sprdata[nr], sprdatb[nr], sprctl[nr]);
	}
	last_sprite_point = point;
}

STATIC_INLINE void decide_sprites(int spnr, int hpos)
{
	decide_sprites(spnr, hpos, false);
}
STATIC_INLINE void maybe_decide_sprites(int spnr, int hpos)
{
	if (!spr[spnr].armed)
		return;
	if (!sprdata[spnr] && !sprdatb[spnr])
		return;
	decide_sprites(spnr, hpos, true);
}

/* End of a horizontal scan line. Finish off all decisions that were not
 * made yet. */
STATIC_INLINE void finish_decisions (void)
{
	struct draw_info *dip;
	struct decision *dp;
	int hpos = maxhpos;
	
	if (nodraw ())
		return;
	
	decide_diw (hpos);
	decide_line (hpos);
	decide_fetch_safe (hpos);
	finish_final_fetch ();
	
	record_color_change2 (hsyncstartpos, 0xffff, 0);
  if (thisline_decision.plfleft >= 0 && thisline_decision.plflinelen < 0) {
	  thisline_decision.plfright = thisline_decision.plfleft;
	  thisline_decision.plflinelen = 0;
	  thisline_decision.bplres = RES_LORES;
  }
	
	/* Large DIWSTOP values can cause the stop position never to be
	 * reached, so the state machine always stays in the same state and
	 * there's a more-or-less full-screen DIW. */
  if (hdiwstate == DIW_waiting_stop) {
		thisline_decision.diwlastword = max_diwlastword;
		if (thisline_decision.diwfirstword < 0)
			thisline_decision.diwfirstword = min_diwlastword;
	}

	dip = curr_drawinfo + next_lineno;
	dp = line_decisions + next_lineno;

	decide_sprites(-1, hpos + 1);
	
	dip->last_sprite_entry = next_sprite_entry;
	dip->last_color_change = next_color_change;
	
	if (thisline_decision.ctable < 0) {
		remember_ctable();
  }
	
	dip->nr_color_changes = next_color_change - dip->first_color_change;
	dip->nr_sprites = next_sprite_entry - dip->first_sprite_entry;
	
	*dp = thisline_decision;

  /* leave free space for possible extra color changes at the end of line */
  next_color_change += (HBLANK_OFFSET + 1) / 2;

	diw_hcounter += maxhpos * 2;
	if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE) && vpos == get_equ_vblank_endline () - 1)
		diw_hcounter++;
	if ((currprefs.chipset_mask & CSMASK_ECS_DENISE) || vpos > get_equ_vblank_endline ()) {
		diw_hcounter = maxhpos * 2;
		last_hdiw = 2 - 1;
	}
}

/* Set the state of all decisions to "undecided" for a new scanline. */
static void reset_decisions (void)
{
  if (nodraw ())
    return;
   
	toscr_nr_planes = toscr_nr_planes2 = 0;
  thisline_decision.bplres = bplcon0_res;
  thisline_decision.nr_planes = 0;
	bpl1dat_written = false;
	bpl1dat_written_at_least_once = false;
   
  thisline_decision.plfleft = -1;
  thisline_decision.plflinelen = -1;
  thisline_decision.ham_seen = !! (bplcon0 & 0x800);
  thisline_decision.ham_at_start = !! (bplcon0 & 0x800);
	thisline_decision.bordersprite_seen = issprbrd (-1, bplcon0, bplcon3);
	thisline_decision.xor_seen = (bplcon4 & 0xff00) != 0;
   
  /* decided_res shouldn't be touched before it's initialized by decide_line(). */
  thisline_decision.diwfirstword = -1;
  thisline_decision.diwlastword = -1;
  if (hdiwstate == DIW_waiting_stop) {
    thisline_decision.diwfirstword = min_diwlastword;
  }
  thisline_decision.ctable = -1;
   
  curr_drawinfo[next_lineno].first_color_change = next_color_change;
  curr_drawinfo[next_lineno].first_sprite_entry = next_sprite_entry;
  next_sprite_forced = 1;
   
  last_sprite_point = 0;
  fetch_state = fetch_not_started;
	if (bpldmasetuphpos >= 0) {
		// this can happen in "too fast" modes
		BPLCON0_Denise (0, bplcon0, true);
		setup_fmodes (0);
	}
	bpldmasetuphpos = -1;
	bpldmasetupphase = 0;
	bpldmawasactive = false;
	reset_moddelays ();
	delay_cycles = 0;
	compute_toscr_delay (bplcon1);

	if (plf_state >= plf_passed_stop2 || plf_state == plf_wait)
 	  plf_state = plf_idle;

	// Only ECS Agnus can keep DDF open between lines
	if ((currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
		if (!ddfstop_matched) {
			plf_state = plf_active;
		}
	}
   
	bpl_hstart = 256;
	plfr_state = plfr_idle;
	plf_start_hpos = 256 + DDF_OFFSET;
	plf_end_hpos = 256 + DDF_OFFSET;
	ddfstop_written_hpos = -1;
	bitplane_maybe_start_hpos = -1;
	bitplane_off_delay = -1;

	if (line_cyclebased) {
		line_cyclebased--;
		if (!line_cyclebased) {
			bpl_dma_off_when_active = 0;
    }
	}

	memset (outword, 0, sizeof outword);
	// fetched[] must not be cleared (Sony VX-90 / Royal Amiga Force)
	todisplay_fetched[0] = todisplay_fetched[1] = false;
	memset (todisplay, 0, sizeof todisplay);
	memset (todisplay2, 0, sizeof todisplay2);
	if (currprefs.chipset_mask & CSMASK_AGA) {
		memset (todisplay_aga, 0, sizeof todisplay_aga);
		memset (todisplay2_aga, 0, sizeof todisplay2_aga);
  }
	aga_plf_passed_stop2 = 0;
   
	reset_bpl_vars ();

	last_decide_line_hpos = -(DDF_OFFSET + 1);
  last_sprite_hpos = -1;
  last_fetch_hpos = -1;

  /* These are for comparison. */
  thisline_decision.bplcon0 = bplcon0;
  thisline_decision.bplcon2 = bplcon2;
  thisline_decision.bplcon3 = bplcon3;
  thisline_decision.bplcon4 = bplcon4;
}

int vsynctimebase_orig;

void compute_vsynctime (void)
{
	double svpos = maxvpos_nom;
	double shpos = maxhpos;
	double syncadjust = 1.0;

	fake_vblank_hz = 0;
	if (fabs (currprefs.chipset_refreshrate) > 0.1) {
	  syncadjust = currprefs.chipset_refreshrate / vblank_hz_nom;
	  vblank_hz = currprefs.chipset_refreshrate;
  }
	if (!fake_vblank_hz)
		fake_vblank_hz = vblank_hz;

	vsynctimebase = (int)(syncbase / fake_vblank_hz);
	vsynctimebase_orig = vsynctimebase;

	if (islinetoggle ()) {
		shpos += 0.5;
	}
	if (interlace_seen) {
		svpos += 0.5;
	} else if (lof_current) {
		svpos += 1.0;
	}
  if (currprefs.produce_sound > 1) {
		double clk = svpos * shpos * fake_vblank_hz;
		devices_update_sound(clk, syncadjust);
  }
	devices_update_sync(svpos, syncadjust);
}

int current_maxvpos (void)
{
	return maxvpos + (lof_store ? 1 : 0);
}

struct chipset_refresh *get_chipset_refresh (void)
{
	int islace = interlace_seen ? 1 : 0;
	int isntsc = (beamcon0 & 0x20) ? 0 : 1;

	if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		isntsc = currprefs.ntscmode ? 1 : 0;

	int def = -1;
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
		struct chipset_refresh *cr = &currprefs.cr[i];
		if (cr->defaultdata)
			def = i;
		if ((cr->horiz < 0 || cr->horiz == maxhpos) &&
			(cr->vert < 0 || cr->vert == maxvpos_display) &&
			(cr->ntsc < 0 || (cr->ntsc == 1 && isntsc) || (cr->ntsc == 0 && !isntsc)) &&
			(cr->lace < 0 || (cr->lace > 0 && islace) || (cr->lace == 0 && !islace)) &&
			(cr->resolution == 0 || cr->resolution == 7) &&
			((cr->rtg && picasso_on) || (!cr->rtg && !picasso_on)) &&
			(cr->vsync < 0 || (cr->vsync > 0 && isvsync_chipset ()) || (cr->vsync == 0 && !isvsync_chipset ())))
				return cr;
	}
	if (def >= 0)
		return &currprefs.cr[def];
	return NULL;
}

static bool changed_chipset_refresh (void)
{
	return stored_chipset_refresh != get_chipset_refresh ();
}

static void compute_framesync (void)
{
	int islace = interlace_seen ? 1 : 0;
	bool found = false;

	if (islace) {
		vblank_hz = vblank_hz_lace;
	} else if (lof_current) {
		vblank_hz = vblank_hz_lof;
	} else {
		vblank_hz = vblank_hz_shf;
	}

	struct chipset_refresh *cr = get_chipset_refresh ();
	while (cr) {
		double v = -1;
	  if (!picasso_on && !picasso_requested_on) {
			if (isvsync_chipset ()) {
				if (cr->index == CHIPSET_REFRESH_PAL || cr->index == CHIPSET_REFRESH_NTSC) {
	        if ((fabs (vblank_hz - 50) < 1 || fabs (vblank_hz - 60) < 1 || fabs (vblank_hz - 100) < 1 || fabs (vblank_hz - 120) < 1)) {
		        vsync_switchmode ((int)vblank_hz);
  	      }
        }	else if (isvsync_chipset () > 0) {
					if (currprefs.gfx_apmode[0].gfx_refreshrate)
						v = abs (currprefs.gfx_apmode[0].gfx_refreshrate);
				}
			} else {
				if (cr->locked == false) {
				changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = vblank_hz;
				break;
				} else {
					v = cr->rate;
				}
      }
			if (v < 0)
				v = cr->rate;
			if (v > 0) {
				changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = v;
			}
    } else {
			if (cr->locked == false)
			v = vblank_hz;
			else
				v = cr->rate;
			changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = v;
			vsync_switchmode((int)v);
    }
		found = true;
		break;
  }
	if (!found) {
    vsync_switchmode((int)vblank_hz);
    changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = vblank_hz;
	}
	stored_chipset_refresh = cr;
	interlace_changed = 0;
	lof_togglecnt_lace = 0;
	lof_togglecnt_nlace = 0;
  lof_changing = 0;

  if (beamcon0 & 0x80) {
	  static bool warned = false;
	  if (!warned) {
		  warned = true;
		  gui_message(_T("Variable beam counter comparator enabled. Not supported in Amiberry."));
	  }
  }

  memset(line_decisions, 0, sizeof line_decisions);
  memset(curr_drawinfo, 0, sizeof curr_drawinfo);
  for (auto& line_decision : line_decisions)
  {
	  line_decision.plfleft = -2;
  }

  compute_vsynctime();

  hblank_hz = double(currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL) / (maxhpos + (islinetoggle() ? 0.5 : 0));

  set_config_changed();

  if (target_graphics_buffer_update()) {
	  reset_drawing();
  }
}

/* set PAL/NTSC or custom timing variables */
static void init_hz (bool checkvposw)
{
	int isntsc, islace;
	int omaxvpos = maxvpos;
	int hzc = 0;

	if (!checkvposw)
		vpos_count = 0;

	vpos_count_diff = vpos_count;

	if ((beamcon0 & 0xA0) != (new_beamcon0 & 0xA0))
		hzc = 1;
	if (beamcon0 != new_beamcon0) {
		vpos_count_diff = vpos_count = 0;
	}
	beamcon0 = new_beamcon0;
	isntsc = (beamcon0 & 0x20) ? 0 : 1;
	islace = (interlace_seen) ? 1 : 0;
	if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		isntsc = currprefs.ntscmode ? 1 : 0;
	int clk = currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL;
	if (!isntsc) {
		maxvpos = MAXVPOS_PAL;
		maxhpos = MAXHPOS_PAL;
		minfirstline = VBLANK_ENDLINE_PAL;
		vblank_hz_nom = vblank_hz = VBLANK_HZ_PAL;
		sprite_vblank_endline = VBLANK_SPRITE_PAL;
		equ_vblank_endline = EQU_ENDLINE_PAL;
		equ_vblank_toggle = true;
		vblank_hz_shf = float(double(clk) / ((maxvpos + 0) * maxhpos));
		vblank_hz_lof = float(double(clk) / ((maxvpos + 1) * maxhpos));
		vblank_hz_lace = float(double(clk) / ((maxvpos + 0.5) * maxhpos));
	}
	else {
		maxvpos = MAXVPOS_NTSC;
		maxhpos = MAXHPOS_NTSC;
		minfirstline = VBLANK_ENDLINE_NTSC;
		vblank_hz_nom = vblank_hz = VBLANK_HZ_NTSC;
		sprite_vblank_endline = VBLANK_SPRITE_NTSC;
		equ_vblank_endline = EQU_ENDLINE_NTSC;
		equ_vblank_toggle = false;
		vblank_hz_shf = float(double(clk) / ((maxvpos + 0) * (maxhpos + 0.5)));
		vblank_hz_lof = float(double(clk) / ((maxvpos + 1) * (maxhpos + 0.5)));
		vblank_hz_lace = float(double(clk) / ((maxvpos + 0.5) * (maxhpos + 0.5)));
	}

  maxvpos_nom = maxvpos;
  maxvpos_display = maxvpos;
  if (vpos_count > 0) {
	  // we come here if vpos_count != maxvpos and beamcon0 didn't change
	  // (someone poked VPOSW)
	  if (vpos_count < 10)
		  vpos_count = 10;
	  vblank_hz = (isntsc ? 15734.0 : 15625.0) / vpos_count;
	  vblank_hz_nom = vblank_hz_shf = vblank_hz_lof = vblank_hz_lace = (float)vblank_hz;
	  maxvpos_nom = vpos_count - (lof_current ? 1 : 0);
		if ((maxvpos_nom >= 256 && maxvpos_nom <= 313) || (beamcon0 & 0x80)) {
		  maxvpos_display = maxvpos_nom;
	  }
	  else if (maxvpos_nom < 256) {
		  maxvpos_display = 255;
	  }
	  else {
		  maxvpos_display = 313;
	  }
	  reset_drawing();
  }
  else if (vpos_count == 0) {
	  // mode reset
	  vpos_count = maxvpos;
	  vpos_count_diff = maxvpos;
  }

  if (beamcon0 & 0x80) {
	  // programmable scanrates (ECS Agnus)
	  static bool warned = false;
	  if (!warned) {
		  warned = true;
		  gui_message(_T("Programmable scanrates (ECS Agnus) not supported."));
	  }
  }
  if (maxvpos_nom >= MAXVPOS)
	  maxvpos_nom = MAXVPOS;
  if (maxvpos_display >= MAXVPOS)
	  maxvpos_display = MAXVPOS;
  if (maxvpos != omaxvpos)
	  hzc = 1;
  /* limit to sane values */
  if (vblank_hz < 10)
	  vblank_hz = 10;
  if (vblank_hz > 300)
	  vblank_hz = 300;
  set_delay_lastcycle();

  hsyncstartpos = maxhpos + 13;
		hsyncendpos = 24;

  eventtab[ev_hsync].oldcycles = get_cycles();
  eventtab[ev_hsync].evtime = get_cycles() + HSYNCTIME;
  events_schedule();
  if (hzc) {
	  interlace_seen = islace;
	  reset_drawing();
  }

	maxvpos_total = (currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? (MAXVPOS_LINES_ECS - 1) : (MAXVPOS_LINES_OCS - 1);
	if (maxvpos_total > MAXVPOS)
		maxvpos_total = MAXVPOS;

  compute_framesync ();

#ifdef PICASSO96
  init_hz_p96 ();
#endif
  inputdevice_tablet_strobe ();

	varsync_changed = false;
}

static void init_hz_vposw (void)
{
	init_hz (true);
}

void init_hz_normal (void)
{
	init_hz (false);
}

static void calcdiw (void)
{
	int hstrt = diwstrt & 0xFF;
	int hstop = diwstop & 0xFF;
	int vstrt = diwstrt >> 8;
	int vstop = diwstop >> 8;
	
	// vertical in ECS Agnus
	if (diwhigh_written && (currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
		vstrt |= (diwhigh & 7) << 8;
		vstop |= ((diwhigh >> 8) & 7) << 8;
	} else {
		if ((vstop & 0x80) == 0)
			vstop |= 0x100;
	}
	// horizontal in ECS Denise
	if (diwhigh_written && (currprefs.chipset_mask & CSMASK_ECS_DENISE)) {
		hstrt |= ((diwhigh >> 5) & 1) << 8;
		hstop |= ((diwhigh >> 13) & 1) << 8;
	} else {
		hstop += 0x100;
	}

	diw_hstrt = hstrt;
	diw_hstop = hstop;
	
	diwfirstword = coord_diw_to_window_x (hstrt);
	diwlastword = coord_diw_to_window_x (hstop);
	
  if (diwfirstword >= diwlastword) {
		diwfirstword = min_diwlastword;
    diwlastword = max_diwlastword;
  }
	if (diwfirstword < min_diwlastword)
		diwfirstword = min_diwlastword;
	
	if (vstrt == vpos && vstop != vpos && diwstate == DIW_waiting_start) {
		// This may start BPL DMA immediately.
		SET_LINE_CYCLEBASED;
		bitplane_maybe_start_hpos = current_hpos();
	}

	plffirstline = vstrt;
	plflastline = vstop;
	
	plfstrt = ddfstrt - DDF_OFFSET;
	plfstop = ddfstop - DDF_OFFSET;
}

/* display mode changed (lores, doubling etc..), recalculate everything */
void init_custom (void)
{
  update_mirrors();
  create_cycle_diagram_table ();
  reset_drawing ();
	init_hz_normal();
  calcdiw ();
}

static int timehack_alive = 0;

static uae_u32 REGPARAM2 timehack_helper (TrapContext *context)
{
#ifdef HAVE_GETTIMEOFDAY
  struct timeval tv;
  if (m68k_dreg (regs, 0) == 0)
  	return timehack_alive;

  timehack_alive = 10;

  gettimeofday (&tv, NULL);
  put_long (m68k_areg (regs, 0), tv.tv_sec - (((365 * 8 + 2) * 24) * 60 * 60));
  put_long (m68k_areg (regs, 0) + 4, tv.tv_usec);
  return 0;
#else
  return 2;
#endif
}

 /*
  * register functions
  */
STATIC_INLINE uae_u16 DENISEID (int *missing)
{
	*missing = 0;
	if (aga_mode) {
		if (currprefs.cs_ide == IDE_A4000)
			return 0xFCF8;
		return 0x00F8;
  }
	if (currprefs.chipset_mask & CSMASK_ECS_DENISE)
		return 0xFFFC;
	if (currprefs.cpu_model == 68000 && currprefs.cpu_compatible)
		*missing = 1;
	return 0xFFFF;
}
STATIC_INLINE uae_u16 DMACONR (int hpos)
{
	dmacon &= ~(0x4000 | 0x2000);
  dmacon |= ((blit_interrupt) ? 0 : 0x4000)
    | (blt_info.blitzero ? 0x2000 : 0);
  return dmacon;
}
STATIC_INLINE uae_u16 INTENAR (void)
{
  return intena;
}
STATIC_INLINE uae_u16 ADKCONR (void)
{
  return adkcon;
}

STATIC_INLINE int issyncstopped (void)
{
	return (bplcon0 & 2);
}

STATIC_INLINE int GETVPOS (void)
{
	return issyncstopped () ? vpos_previous : vpos;
}
STATIC_INLINE int GETHPOS (void)
{
	return issyncstopped () ? hpos_previous : current_hpos ();
}

#define CPU_ACCURATE (currprefs.cpu_model < 68020)

// DFF006 = 0.W must be valid result but better do this only in 68000 modes (whdload black screen!)
// HPOS is shifted by 3 cycles and VPOS increases when shifted HPOS==1
#define HPOS_OFFSET (CPU_ACCURATE ? HPOS_SHIFT : 0)
#define VPOS_INC_DELAY (HPOS_OFFSET ? 1 : 0)

STATIC_INLINE uae_u16 VPOSR (void)
{
  unsigned int csbit = 0;
	uae_u16 vp = GETVPOS ();
	uae_u16 hp = GETHPOS ();

	if (hp + HPOS_OFFSET >= maxhpos + VPOS_INC_DELAY) {
		vp++;
		if (vp >= maxvpos + lof_store)
			vp = 0;
	}
  vp = (vp >> 8) & 7;

	csbit |= (aga_mode) ? 0x2300 : 0;
	csbit |= (currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 0x2000 : 0;
	if (currprefs.ntscmode)
	  csbit |= 0x1000;

  if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
    vp &= 1;
  vp |= (lof_store ? 0x8000 : 0) | csbit;
  return vp;
}

static void VPOSW (uae_u16 v)
{
	int oldvpos = vpos;
	if (lof_store != ((v & 0x8000) ? 1 : 0)) {
  	lof_store = (v & 0x8000) ? 1 : 0;
		lof_changing = lof_store ? 1 : -1;
  }
	if (lof_changing)
		return;
	vpos &= 0x00ff;
	v &= 7;
	if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		v &= 1;
	vpos |= v << 8;
	if (vpos != oldvpos)
		vposw_change++;
	if (vpos < oldvpos)
		vpos = oldvpos;
}

static void VHPOSW (uae_u16 v)
{
	int oldvpos = vpos;
	bool changed = false;

	v >>= 8;
	vpos &= 0xff00;
	vpos |= v;
	if (vpos != oldvpos && !changed)
		vposw_change++;
	if (vpos < oldvpos) {
		vpos = oldvpos;
	} else if (vpos < minfirstline && oldvpos < minfirstline) {
		vpos = oldvpos;
	}
}

STATIC_INLINE uae_u16 VHPOSR (void)
{
	uae_u16 vp = GETVPOS ();
	uae_u16 hp = GETHPOS ();

	hp += HPOS_OFFSET;
  if (hp >= maxhpos) {
   	hp -= maxhpos;
		// vpos increases when hp==1, not when hp==0
		if (hp >= VPOS_INC_DELAY) {
	    vp++;
	    if (vp >= maxvpos + lof_store)
	      vp = 0;
    }
	}
	if (HPOS_OFFSET) {
	  hp += 1;
	  if (hp >= maxhpos)
		  hp -= maxhpos;
	}

  vp <<= 8;
  vp |= hp;
  return vp;
}

static int test_copper_dangerous (unsigned int address)
{
  int addr = address & 0x01fe;
  if (addr < ((copcon & 2) ? ((currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 0 : 0x40) : 0x80)) {
	  cop_state.state = COP_stop;	
	  copper_enabled_thisline = 0;
	  unset_special (SPCFLAG_COPPER);
	  return 1;
  }
  return 0;
}

static void immediate_copper (int num)
{
  int pos = 0;
  int oldpos = 0;

  cop_state.state = COP_stop;
  cop_state.hpos = current_hpos () & ~1;
  cop_state.ip = num == 1 ? cop1lc : cop2lc;

  while (pos < (maxvpos << 5)) {
  	if (oldpos > pos)
	    pos = oldpos;
  	if (!dmaen(DMA_COPPER))
	    break;
  	if (cop_state.ip >= currprefs.chipmem_size)
	    break;
	  pos++;
	  oldpos = pos;
	  cop_state.i1 = chipmem_wget_indirect (cop_state.ip);
	  cop_state.i2 = chipmem_wget_indirect (cop_state.ip + 2);
	  cop_state.ip += 4;
	  if (!(cop_state.i1 & 1)) { // move
	    cop_state.i1 &= 0x1fe;
	    if (cop_state.i1 == 0x88) {
        cop_state.ip = cop1lc;
    		continue;
	    }
	    if (cop_state.i1 == 0x8a) {
        cop_state.ip = cop2lc;
    		continue;
	    }
	    if (test_copper_dangerous (cop_state.i1))
    		break;
	    custom_wput_1 (0, cop_state.i1, cop_state.i2, 0);
  	} else { // wait or skip
	    if ((cop_state.i1 >> 8) > ((pos >> 5) & 0xff))
    		pos = (((pos >> 5) & 0x100) | ((cop_state.i1 >> 8)) << 5) | ((cop_state.i1 & 0xff) >> 3);
	    if (cop_state.i1 >= 0xffdf && cop_state.i2 == 0xfffe)
    		break;
  	}
  }
  cop_state.state = COP_stop;
  unset_special (SPCFLAG_COPPER);
}

STATIC_INLINE void COP1LCH (uae_u16 v)
{
	cop1lc = (cop1lc & 0xffff) | ((uae_u32)v << 16);
}
STATIC_INLINE void COP1LCL (uae_u16 v)
{
	cop1lc = (cop1lc & ~0xffff) | (v & 0xfffe);
}
STATIC_INLINE void COP2LCH (uae_u16 v)
{
	cop2lc = (cop2lc & 0xffff) | ((uae_u32)v << 16);
}
STATIC_INLINE void COP2LCL (uae_u16 v)
{
	cop2lc = (cop2lc & ~0xffff) | (v & 0xfffe);
}

static void compute_spcflag_copper (int hpos);

// vblank = copper starts at hpos=2
// normal COPJMP write: takes 2 more cycles
static void COPJMP (int num, int vblank)
{
	int was_active = eventtab[ev_copper].active;
  int oldstrobe = cop_state.strobe;
	bool wasstopped = cop_state.state == COP_stop && !vblank;

  unset_special (SPCFLAG_COPPER);
	cop_state.ignore_next = 0;

  if (!oldstrobe)
  	cop_state.state_prev = cop_state.state;
	if ((cop_state.state == COP_wait || cop_state.state == COP_waitforever) && !vblank && dmaen(DMA_COPPER)) {
		cop_state.state = COP_strobe_delay1x;
	} else {
		cop_state.state = vblank ? COP_start_delay : (copper_access ? COP_strobe_delay1 : COP_strobe_extra);
	}
	cop_state.hpos = current_hpos () & ~1;
  copper_enabled_thisline = 0;
  cop_state.strobe = num;
	
  eventtab[ev_copper].active = 0;

  if (nocustom()) {
	  immediate_copper (num);
	  return;
  }

  if (was_active)
	  events_schedule ();

	if (dmaen (DMA_COPPER)) {
		compute_spcflag_copper (current_hpos ());
	} else if (wasstopped || (oldstrobe > 0 && oldstrobe != num && cop_state.state_prev == COP_wait)) {
	  /* dma disabled, copper idle and accessed both COPxJMPs -> copper stops! */
	  cop_state.state = COP_stop;
	}
}

STATIC_INLINE void COPCON (uae_u16 a)
{
  copcon = a;
}

STATIC_INLINE void check_copper_stop(void)
{
	if (copper_enabled_thisline < 0 && !((dmacon & DMA_COPPER) && (dmacon & DMA_MASTER))) {
		copper_enabled_thisline = 0;
		unset_special (SPCFLAG_COPPER);
	}
}

STATIC_INLINE void copper_stop(void)
{
	if (copper_enabled_thisline) {
		// let MOVE to finish
		switch (cop_state.state)
		{
			case COP_read2:
			  copper_enabled_thisline = -1;
			  break;
		}
	}
	if (copper_enabled_thisline >= 0) {
		copper_enabled_thisline = 0;
		unset_special (SPCFLAG_COPPER);
	}
}

static void DMACON (int hpos, uae_u16 v)
{
	int oldcop, newcop;
  uae_u16 changed;
	
	uae_u16 oldcon = dmacon;
	
	decide_line (hpos);
	decide_fetch_safe (hpos);
	
	setclr (&dmacon, v);
	dmacon &= 0x07FF;
	
  changed = dmacon ^ oldcon;
  oldcop = (oldcon & DMA_COPPER) && (oldcon & DMA_MASTER);
  newcop = (dmacon & DMA_COPPER) && (dmacon & DMA_MASTER);

  if (oldcop != newcop) {
	  eventtab[ev_copper].active = 0;

  	if (newcop && !oldcop) {
			compute_spcflag_copper (hpos);
	  } else if (!newcop) {
			copper_stop();
    }
	}

	if ((dmacon & DMA_BLITPRI) > (oldcon & DMA_BLITPRI) && bltstate != BLT_done)
		set_special (SPCFLAG_BLTNASTY);

	if (dmaen (DMA_BLITTER) && bltstate == BLT_init) {
	  blitter_check_start ();
	}

	if ((dmacon & (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER)) != (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER))
		unset_special (SPCFLAG_BLTNASTY);
	
  if (changed & (DMA_MASTER | 0x0f))
		audio_state_machine ();

  if (changed & (DMA_MASTER | DMA_BITPLANE)) {
	  SET_LINE_CYCLEBASED;
		bitplane_maybe_start_hpos = hpos;
  }

	events_schedule();
}

int intlev (void)
{
  uae_u16 imask = intreq & intena;
  if (!(imask && (intena & 0x4000)))
  	return -1;
  if (imask & (0x4000 | 0x2000)) // 13 14
    return 6;
  if (imask & (0x1000 | 0x0800)) // 11 12
    return 5;
  if (imask & (0x0400 | 0x0200 | 0x0100 | 0x0080)) // 7 8 9 10
    return 4;
  if (imask & (0x0040 | 0x0020 | 0x0010)) // 4 5 6
    return 3;
  if (imask & 0x0008) // 3
    return 2;
  if (imask & (0x0001 | 0x0002 | 0x0004)) // 0 1 2
    return 1;
  return -1;
}

void rethink_uae_int(void)
{
	bool irq2 = false;
	bool irq6 = false;

  if (uae_int_requested) {
		if (uae_int_requested & 0xff00)
			irq6 = true;
		if (uae_int_requested & 0x00ff)
			irq2 = true;
  }

	{
		extern void bsdsock_fake_int_handler (void);
		extern int volatile bsd_int_requested;
		if (bsd_int_requested)
			bsdsock_fake_int_handler ();
	}

	uae_u16 mask = (irq6 ? 0x2000 : 0) | (irq2 ? 0x0008 : 0);
	if (mask)
		INTREQ_0(0x8000 | mask);
}

STATIC_INLINE void rethink_intreq (void)
{
	devices_rethink();
}

static void INTENA (uae_u16 v)
{
	uae_u16 old = intena;
  setclr (&intena,v);

	if (!(v & 0x8000) && old == intena)
		return;

  if (v & 0x8000)
    doint();
}

void INTREQ_f (uae_u16 v)
{
 	setclr (&intreq, v);
}

bool INTREQ_0 (uae_u16 v)
{
	uae_u16 old = intreq;
	setclr (&intreq, v);

	if (old == intreq)
		return false;
  if (v & 0x8000)
  	doint();
  return true;
}

void INTREQ (uae_u16 data)
{
  if (INTREQ_0 (data))
		rethink_intreq ();
}

static void ADKCON (int hpos, uae_u16 v)
{
  if (currprefs.produce_sound > 0)
    update_audio ();
  DISK_update (hpos);
	DISK_update_adkcon (hpos, v);
  setclr (&adkcon,v);
  audio_update_adkmasks ();
}

static void BEAMCON0 (uae_u16 v)
{
  if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
    new_beamcon0 = v;
		calcdiw();
  }
}

STATIC_INLINE void varsync (void)
{
  if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	  return;
  if (!(beamcon0 & 0x80))
	  return;
	varsync_changed = true;
}

static void BPLxPTH (int hpos, uae_u16 v, int num)
{
  decide_line (hpos);
	decide_fetch_safe (hpos);
	if (copper_access && is_bitplane_dma (hpos + 1) == num + 1) {
		SET_LINE_CYCLEBASED;
		return;
	}

  bplpt[num] = (bplpt[num] & 0x0000ffff) | ((uae_u32)v << 16);
}

static void BPLxPTL (int hpos, uae_u16 v, int num)
{
  decide_line (hpos);
	decide_fetch_safe (hpos);

	/* chipset feature:
	 * BPLxPTL write and next cycle doing DMA fetch using same pointer register ->
	 * next DMA cycle uses old value.
	 * (Multiscroll / Cult)
	 *
	 * If following cycle is not BPL DMA: written value is lost
	 *
	 * last fetch block does not have this side-effect, probably due to modulo adds.
	 * Also it seems only plane 0 fetches have this feature (because of above reason!)
	 * (MoreNewStuffy / PlasmaForce)
	 */
	/* only detect copper accesses to prevent too fast CPU mode glitches */
	if (copper_access && is_bitplane_dma (hpos + 1) == num + 1) {
		SET_LINE_CYCLEBASED;
		return;
  }
  bplpt[num] = (bplpt[num] & 0xffff0000) | (v & 0x0000fffe);
}

static void BPLCON0_Denise (int hpos, uae_u16 v, bool immediate)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
		v &= ~0x00F1;
	else if (! (aga_mode))
		v &= ~0x00B0;
	v &= ~(0x0200 | 0x0100 | 0x0080 | 0x0020);

	if (bplcon0d == v && !immediate)
		return;

	bplcon0dd = -1;

	if (immediate) {
		record_register_change (hpos, 0x100, v);
	} else {
	  record_register_change (hpos, 0x100, (bplcon0d & ~(0x800 | 0x400 | 0x80)) | (v & (0x0800 | 0x400 | 0x80 | 0x01)));
	}

	bplcon0d = v;

	if (currprefs.chipset_mask & CSMASK_ECS_DENISE) {
		decide_sprites(-1, hpos);
		sprres = expand_sprres (v, bplcon3);
	}
	if (thisline_decision.plfleft < 0)
		update_denise (hpos);
	else
		update_denise_shifter_planes (hpos);
}

static void BPLCON0 (int hpos, uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
		v &= ~0x00F1;
	else if (! (aga_mode))
		v &= ~0x00B0;
	v &= ~0x0080;
 	
	if (bplcon0 == v)
		return;
	
	SET_LINE_CYCLEBASED;
	decide_diw (hpos);
	decide_line (hpos);
	decide_fetch_safe (hpos);

	if (!issyncstopped ()) {
		vpos_previous = vpos;
		hpos_previous = hpos;
	}

	bplcon0 = v;

	bpldmainitdelay (hpos);

	if (thisline_decision.plfleft < 0)
		BPLCON0_Denise (hpos, v, true);
}

STATIC_INLINE void BPLCON1 (int hpos, uae_u16 v)
{
  if (!(aga_mode))
	  v &= 0xff;
	if (bplcon1 == v)
		return;
	SET_LINE_CYCLEBASED;
	decide_line (hpos);
	decide_fetch_safe (hpos);
	bplcon1_written = true;
	bplcon1 = v;
}

STATIC_INLINE void BPLCON2 (int hpos, uae_u16 v)
{
  if (!(aga_mode))
		v &= ~(0x100 | 0x80); // RDRAM and SOGEN
	if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE))
	  v &= 0x7f;
	v &= ~0x8000; // unused
	if (bplcon2 == v)
		return;
	decide_line (hpos);
	bplcon2 = v;
	record_register_change (hpos, 0x104, bplcon2);
}

STATIC_INLINE void BPLCON3 (int hpos, uae_u16 v)
{
  if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
  	return;
  if (!(aga_mode)) {
	  v &= 0x003f;
	  v |= 0x0c00;
  }
	if (bplcon3 == v)
		return;
	decide_line (hpos);
	decide_sprites(-1, hpos);
	bplcon3 = v;
  sprres = expand_sprres (bplcon0, bplcon3);
  record_register_change (hpos, 0x106, v);
}

STATIC_INLINE void BPLCON4 (int hpos, uae_u16 v)
{
	if (! (aga_mode))
		return;
	if (bplcon4 == v)
		return;
	decide_line (hpos);
	bplcon4 = v;
  record_register_change (hpos, 0x10c, v);
}

static void BPL1MOD (int hpos, uae_u16 v)
{
  v &= ~1;
	if ((uae_s16)bpl1mod != (uae_s16)v) {
    decide_line (hpos);
		decide_fetch_safe (hpos);
	}
	// write to BPLxMOD one cycle before
	// BPL fetch that also adds modulo:
	// Old BPLxMOD value is added.
	if (1 && (is_bitplane_dma (hpos + 1) & 1)) {
		dbpl1mod = v;
		dbpl1mod_on = hpos + 1;
	} else {
		bpl1mod = v;
		dbpl1mod_on = 0;
	}
}

static void BPL2MOD (int hpos, uae_u16 v)
{
  v &= ~1;
	if ((uae_s16)bpl2mod != (uae_s16)v) {
    decide_line (hpos);
		decide_fetch_safe (hpos);
	}
	if (1 && (is_bitplane_dma (hpos + 1) & 2)) {
		dbpl2mod = v;
		dbpl2mod_on = hpos + 1;
	} else {
    bpl2mod = v;
		dbpl2mod_on = 0;
	}
}

/* Needed in special OCS/ECS "7-plane" mode,
 * also handles CPU generated bitplane data
 */
static void BPLxDAT (int hpos, int num, uae_u16 v)
{
	// only BPL1DAT access can do anything visible
	if (num == 0 && hpos >= 8) {
    decide_line (hpos);
		decide_fetch_safe (hpos);
	}
	flush_display (fetchmode);
	fetched[num] = v;
	fetched_aga[num] = v;
	if (num == 0 && hpos >= 8) {
	  bpl1dat_written = true;
	  bpl1dat_written_at_least_once = true;
		if (thisline_decision.plfleft < 0)
		  reset_bpl_vars ();
		beginning_of_plane_block (hpos, fetchmode);
  }
}

static void DIWSTRT (int hpos, uae_u16 v)
{
  if (diwstrt == v && ! diwhigh_written)
		return;
	decide_diw (hpos);
	decide_line (hpos);
	diwhigh_written = 0;
	diwstrt = v;
	calcdiw ();
}

static void DIWSTOP (int hpos, uae_u16 v)
{
  if (diwstop == v && ! diwhigh_written)
		return;
	decide_diw (hpos);
	decide_line (hpos);
	diwhigh_written = 0;
	diwstop = v;
	calcdiw ();
}

static void DIWHIGH (int hpos, uae_u16 v)
{
  if (!(currprefs.chipset_mask & (CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS)))
	  return;
	if (!(aga_mode))
		v &= ~(0x0008 | 0x0010 | 0x1000 | 0x0800);
  v &= ~(0x8000 | 0x4000 | 0x0080 | 0x0040);
  if (diwhigh_written && diwhigh == v)
		return;
	decide_line (hpos);
	diwhigh_written = 1;
	diwhigh = v;
	calcdiw ();
}

static void DDFSTRT (int hpos, uae_u16 v)
{
  v &= 0xfe;
  if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	  v &= 0xfc;
	decide_line (hpos);
  SET_LINE_CYCLEBASED;
	// Move state back to passed_enable if this DDFSTRT write was done exactly when
	// it would match and start bitplane DMA.
	if (hpos == ddfstrt - DDF_OFFSET && plf_state == plf_passed_start && plf_start_hpos == hpos + DDF_OFFSET) {
		plf_state = plf_passed_enable;
		plf_start_hpos = maxhpos;
	}
	ddfstrt = v;
	calcdiw ();
	if (fetch_state != fetch_not_started)
		estimate_last_fetch_cycle (hpos);
}

static void DDFSTOP (int hpos, uae_u16 v)
{
	v &= 0xfe;
  if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	  v &= 0xfc;
	decide_line (hpos);
	decide_fetch_safe (hpos);
	SET_LINE_CYCLEBASED;
	// DDFSTOP write when old DDFSTOP value match: old value matches normally.
	// Works differently than DDFSTRT which is interesting.
	if (hpos == v - DDF_OFFSET) {
		if (plf_state == plf_passed_stop && plf_end_hpos == hpos + DDF_OFFSET) {
			plf_state = plf_active;
			plf_end_hpos = 256 + DDF_OFFSET;
			// don't let one_fetch_cycle_0() to do this again
			ddfstop_written_hpos = hpos;
		}
	} else if (hpos == ddfstop - DDF_OFFSET) {
		// if old ddfstop would have matched, emulate it here
		if (plf_state == plf_active) {
			plf_state = plf_passed_stop;
			plf_end_hpos = hpos + DDF_OFFSET;
		}
	}
	ddfstop = v;
	calcdiw ();
	if (fetch_state != fetch_not_started)
		estimate_last_fetch_cycle (hpos);
}

static void FMODE (int hpos, uae_u16 v)
{
	if (!(aga_mode)) {
  	v = 0;
	}
	v &= 0xC00F;
	if (fmode == v)
		return;
	SET_LINE_CYCLEBASED;
	fmode_saved = v;
	set_chipset_mode();
	bpldmainitdelay (hpos);
}

static void FNULL (uae_u16 v)
{

}

static void BLTADAT (uae_u16 v)
{
  maybe_blit (0);

  blt_info.bltadat = v;
}
/*
 * "Loading data shifts it immediately" says the HRM. Well, that may
 * be true for BLTBDAT, but not for BLTADAT - it appears the A data must be
 * loaded for every word so that AFWM and ALWM can be applied.
 */
static void BLTBDAT (uae_u16 v)
{
  maybe_blit (0);

	int shift = bltcon1 >> 12;

  if (bltcon1 & 2)
		blt_info.bltbhold = (((uae_u32)v << 16) | blt_info.bltbold) >> (16 - shift);
  else
		blt_info.bltbhold = (((uae_u32)blt_info.bltbold << 16) | v) >> shift;

  blt_info.bltbdat = v;
	blt_info.bltbold = v;
}
STATIC_INLINE void BLTCDAT (uae_u16 v) { maybe_blit (0); blt_info.bltcdat = v; }

STATIC_INLINE void BLTAMOD (uae_u16 v) { maybe_blit (1); blt_info.bltamod = (uae_s16)(v & 0xFFFE); }
STATIC_INLINE void BLTBMOD (uae_u16 v) { maybe_blit (1); blt_info.bltbmod = (uae_s16)(v & 0xFFFE); }
STATIC_INLINE void BLTCMOD (uae_u16 v) { maybe_blit (1); blt_info.bltcmod = (uae_s16)(v & 0xFFFE); }
STATIC_INLINE void BLTDMOD (uae_u16 v) { maybe_blit (1); blt_info.bltdmod = (uae_s16)(v & 0xFFFE); }

STATIC_INLINE void BLTCON0 (uae_u16 v) { maybe_blit (2); bltcon0 = v; reset_blit (1); }

/* The next category is "Most useless hardware register".
 * And the winner is... */
static void BLTCON0L (uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		return; // ei voittoa.
	maybe_blit (2); 
	bltcon0 = (bltcon0 & 0xFF00) | (v & 0xFF);
  reset_blit (1);
}
STATIC_INLINE void BLTCON1 (uae_u16 v) { maybe_blit (2); bltcon1 = v; reset_blit (2); }

STATIC_INLINE void BLTAFWM (uae_u16 v) { maybe_blit (2); blt_info.bltafwm = v; }
STATIC_INLINE void BLTALWM (uae_u16 v) { maybe_blit (2); blt_info.bltalwm = v; }

STATIC_INLINE void BLTAPTH (uae_u16 v)
{
  maybe_blit (0); 
  bltapt = (bltapt & 0xffff) | ((uae_u32)v << 16); 
}
STATIC_INLINE void BLTAPTL (uae_u16 v)
{ 
  maybe_blit (0); 
  bltapt = (bltapt & ~0xffff) | (v & 0xFFFE); 
}
STATIC_INLINE void BLTBPTH (uae_u16 v) 
{ 
  maybe_blit (0); 
  bltbpt = (bltbpt & 0xffff) | ((uae_u32)v << 16); 
}
STATIC_INLINE void BLTBPTL (uae_u16 v) 
{ 
  maybe_blit (0); 
  bltbpt = (bltbpt & ~0xffff) | (v & 0xFFFE); 
}
STATIC_INLINE void BLTCPTH (uae_u16 v) 
{ 
  maybe_blit (0); 
  bltcpt = (bltcpt & 0xffff) | ((uae_u32)v << 16); 
}
STATIC_INLINE void BLTCPTL (uae_u16 v) 
{ 
  maybe_blit (0); 
  bltcpt = (bltcpt & ~0xffff) | (v & 0xFFFE); 
}
STATIC_INLINE void BLTDPTH (uae_u16 v) 
{ 
  maybe_blit (0); 
  bltdpt = (bltdpt & 0xffff) | ((uae_u32)v << 16); 
}
STATIC_INLINE void BLTDPTL (uae_u16 v) 
{ 
  maybe_blit (0); 
  bltdpt = (bltdpt & ~0xffff) | (v & 0xFFFE); 
}

STATIC_INLINE void BLTSIZE (int hpos, uae_u16 v)
{
  maybe_blit (0);

  blt_info.vblitsize = v >> 6;
  blt_info.hblitsize = v & 0x3F;
  if (!blt_info.vblitsize) 
    blt_info.vblitsize = 1024;
  if (!blt_info.hblitsize) 
    blt_info.hblitsize = 64;
	do_blitter (hpos);
}

STATIC_INLINE void BLTSIZV (uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		return;
	maybe_blit (0);
	blt_info.vblitsize = v & 0x7FFF;
}

STATIC_INLINE void BLTSIZH (int hpos, uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
		return;
	maybe_blit (0);
	blt_info.hblitsize = v & 0x7FF;
	if (!blt_info.vblitsize) 
	  blt_info.vblitsize = 0x8000;
	if (!blt_info.hblitsize) 
	  blt_info.hblitsize = 0x0800;
	do_blitter (hpos);
}

STATIC_INLINE void spr_arm (int num, int state)
{
  switch (state) {
    case 0:
      nr_armed -= spr[num].armed;
      spr[num].armed = 0;
      break;
    default:
      nr_armed += 1 - spr[num].armed;
      spr[num].armed = 1;
      break;
  }
}

STATIC_INLINE void sprstartstop (struct sprite *s)
{
	if (vpos < sprite_vblank_endline)
		return;
  if (vpos == s->vstart)
    s->dmastate = 1;
  if (vpos == s->vstop)
    s->dmastate = 0;
}

STATIC_INLINE void SPRxCTLPOS (int num)
{
  int sprxp;
  struct sprite *s = &spr[num];

  sprstartstop (s);
  sprxp = (sprpos[num] & 0xFF) * 2 + (sprctl[num] & 1);
  sprxp <<= sprite_buffer_res;
  /* Quite a bit salad in this register... */
	if (aga_mode) {
		sprxp |= ((sprctl[num] >> 3) & 3) >> (RES_MAX - sprite_buffer_res);
		s->dblscan = sprpos[num] & 0x80;
	}
	else if (currprefs.chipset_mask & CSMASK_ECS_DENISE) {
		sprxp |= ((sprctl[num] >> 3) & 2) >> (RES_MAX - sprite_buffer_res);
  }
  s->xpos = sprxp;
  s->vstart = sprpos[num] >> 8;
  s->vstart |= (sprctl[num] & 0x04) ? 0x0100 : 0;
  s->vstop = sprctl[num] >> 8;
  s->vstop |= (sprctl[num] &0x02) ? 0x100 : 0;
  if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
		s->vstart |= (sprctl[num] & 0x40) ? 0x0200 : 0;
		s->vstop |= (sprctl[num] & 0x20) ? 0x0200 : 0;
  }
  sprstartstop (s);
}

STATIC_INLINE void SPRxCTL_1 (uae_u16 v, int num)
{
  sprctl[num] = v;
  spr_arm (num, 0);
  SPRxCTLPOS (num);
}
STATIC_INLINE void SPRxPOS_1 (uae_u16 v, int num)
{
  sprpos[num] = v;
  SPRxCTLPOS (num);
}
STATIC_INLINE void SPRxDATA_1 (uae_u16 v, int num)
{
  sprdata[num][0] = v;
  sprdata[num][1] = v;
  sprdata[num][2] = v;
  sprdata[num][3] = v;
  spr_arm (num, 1);
}
STATIC_INLINE void SPRxDATB_1 (uae_u16 v, int num)
{
  sprdatb[num][0] = v;
  sprdatb[num][1] = v;
  sprdatb[num][2] = v;
  sprdatb[num][3] = v;
}

/*
 SPRxDATA and SPRxDATB is moved to shift register when SPRxPOS matches.

 When copper writes to SPRxDATx exactly when SPRxPOS matches:
 - If sprite low x bit (SPRCTL bit 0) is not set, shift register copy
   is done first (previously loaded SPRxDATx value is shown) and then
   new SPRxDATx gets stored for future use.
 - If sprite low x bit is set, new SPRxDATx is stored, then SPRxPOS
   matches and value written to SPRxDATx is visible.

 - Writing to SPRxPOS when SPRxPOS matches: shift register
   copy is always done first, then new SPRxPOS value is stored
   for future use. (SPRxCTL not tested)
*/

STATIC_INLINE void SPRxDATA (int hpos, uae_u16 v, int num) 
{ 
	decide_sprites(-1, hpos, false);
  SPRxDATA_1 (v, num); 
}
STATIC_INLINE void SPRxDATB (int hpos, uae_u16 v, int num) 
{ 
	decide_sprites(-1, hpos, false);
  SPRxDATB_1 (v, num); 
}

STATIC_INLINE void SPRxCTL (int hpos, uae_u16 v, int num) 
{ 
	decide_sprites(-1, hpos);
  SPRxCTL_1 (v, num); 
}
STATIC_INLINE void SPRxPOS (int hpos, uae_u16 v, int num)
{
	decide_sprites(-1, hpos);
  SPRxPOS_1 (v, num);
}

static void SPRxPTH (int hpos, uae_u16 v, int num)
{
	decide_sprites(-1, hpos);
	if (hpos - 1 != MAXHPOS) {
    spr[num].pt &= 0xffff;
    spr[num].pt |= (uae_u32)v << 16;
  }
}
static void SPRxPTL (int hpos, uae_u16 v, int num)
{
	decide_sprites(-1, hpos);
	if (hpos - 1 != MAXHPOS) {
    spr[num].pt &= ~0xffff;
		spr[num].pt |= v & ~1;
  }
}

static void CLXCON (uae_u16 v)
{
  clxcon = v;
  clxcon_bpl_enable = (v >> 6) & 63;
  clxcon_bpl_match = v & 63;
}

static void CLXCON2 (uae_u16 v)
{
	if (!(aga_mode))
		return;
	clxcon2 = v;
	clxcon_bpl_enable |= v & (0x40|0x80);
	clxcon_bpl_match |= (v & (0x01|0x02)) << 6;
}

static uae_u16 CLXDAT (void)
{
  uae_u16 v = clxdat | 0x8000;
  clxdat = 0;
  return v;
}

static uae_u16 COLOR_READ (int num)
{
	int cr, cg, cb, colreg;
	uae_u16 cval;
	
	if (!(aga_mode) || !(bplcon2 & 0x0100))
		return 0xffff;
	
	colreg = ((bplcon3 >> 13) & 7) * 32 + num;
	cr = (current_colors.color_regs_aga[colreg] >> 16) & 0xFF;
	cg = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
	cb = current_colors.color_regs_aga[colreg] & 0xFF;
	if (bplcon3 & 0x200) {
		cval = ((cr & 15) << 8) | ((cg & 15) << 4) | ((cb & 15) << 0);
	} else {
		cval = ((cr >> 4) << 8) | ((cg >> 4) << 4) | ((cb >> 4) << 0);
	}
  return cval;
}

static void COLOR_WRITE (int hpos, uae_u16 v, int num)
{
	if (aga_mode) {
		int r,g,b;
		int cr,cg,cb;
		int colreg;
		uae_u32 cval;
		
		/* writing is disabled when RDRAM=1 */
		if (bplcon2 & 0x0100)
			return;
		
		colreg = ((bplcon3 >> 13) & 7) * 32 + num;
		r = (v & 0xF00) >> 8;
		g = (v & 0xF0) >> 4;
		b = (v & 0xF) >> 0;
		cr = (current_colors.color_regs_aga[colreg] >> 16) & 0xFF;
		cg = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
		cb = current_colors.color_regs_aga[colreg] & 0xFF;
		
		if (bplcon3 & 0x200) {
			cr &= 0xF0; cr |= r;
			cg &= 0xF0; cg |= g;
			cb &= 0xF0; cb |= b;
		} else {
			cr = r + (r << 4);
			cg = g + (g << 4);
			cb = b + (b << 4);
		}
		cval = (cr << 16) | (cg << 8) | cb;

		if (cval == current_colors.color_regs_aga[colreg])
			return;
		
		/* Call this with the old table still intact. */
		record_color_change (hpos, colreg, cval);
		remembered_color_entry = -1;
		current_colors.color_regs_aga[colreg] = cval;
		current_colors.acolors[colreg] = CONVERT_RGB (cval);

	} else {
		v &= 0x8fff;
		if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE))
			v &= 0xfff;
	  if (current_colors.color_regs_ecs[num] == v)
			return;

		/* Call this with the old table still intact. */
		record_color_change (hpos, num, v);
		remembered_color_entry = -1;
	  current_colors.color_regs_ecs[num] = v;
		current_colors.acolors[num] = xcolors[v];
	}
}

/* The copper code.  The biggest nightmare in the whole emulator.

   Alright.  The current theory:
   1. Copper moves happen 2 cycles after state READ2 is reached.
      It can't happen immediately when we reach READ2, because the
      data needs time to get back from the bus.  An additional 2
      cycles are needed for non-Agnus registers, to take into account
      the delay for moving data from chip to chip.
   2. As stated in the HRM, a WAIT really does need an extra cycle
      to wake up.  This is implemented by _not_ falling through from
      a successful wait to READ1, but by starting the next cycle.
      (Note: the extra cycle for the WAIT apparently really needs a
      free cycle; i.e. contention with the bitplane fetch can slow
      it down).
   3. Apparently, to compensate for the extra wake up cycle, a WAIT
      will use the _incremented_ horizontal position, so the WAIT
      cycle normally finishes two clocks earlier than the position
      it was waiting for.  The extra cycle then takes us to the
      position that was waited for.
      If the earlier cycle is busy with a bitplane, things change a bit.
      E.g., waiting for position 0x50 in a 6 plane display: In cycle
      0x4e, we fetch BPL5, so the wait wakes up in 0x50, the extra cycle
      takes us to 0x54 (since 0x52 is busy), then we have READ1/READ2,
      and the next register write is at 0x5c.
   4. The last cycle in a line is not usable for the copper.
   5. A 4 cycle delay also applies to the WAIT instruction.  This means
      that the second of two back-to-back WAITs (or a WAIT whose
      condition is immediately true) takes 8 cycles.
   6. This also applies to a SKIP instruction.  The copper does not
      fetch the next instruction while waiting for the second word of
      a WAIT or a SKIP to arrive.
   7. A SKIP also seems to need an unexplained additional two cycles
      after its second word arrives; this is _not_ a memory cycle (I
      think, the documentation is pretty clear on this).
   8. Two additional cycles are inserted when writing to COPJMP1/2.  */

/* Determine which cycles are available for the copper in a display
 * with a agiven number of planes.  */

STATIC_INLINE int copper_cant_read (int hpos)
{
  if (hpos + 1 >= maxhpos) // first refresh slot
    return 1;
	if ((hpos == maxhpos - 3) && (maxhpos & 1)) {
		return -1;
	}
  return is_bitplane_dma (hpos);
}


/* The future, Conan?
   We try to look ahead in the copper list to avoid doing continuous calls
   to updat_copper (which is what happens when SPCFLAG_COPPER is set).  If
   we find that the same effect can be achieved by setting a delayed event
   and then doing multiple copper insns in one batch, we can get a massive
   speedup.

   We don't try to be precise here.  All copper reads take exactly 2 cycles,
   the effect of bitplane contention is ignored.  Trying to get it exactly
   right would be much more complex and as such carry a huge risk of getting
   it subtly wrong; and it would also be more expensive - we want this code
   to be fast.  */

static void predict_copper (void)
{
	uaecptr ip = cop_state.ip;
	unsigned int c_hpos = cop_state.hpos;
	enum copper_states state = cop_state.state;
	unsigned int w1, w2, cycle_count;
	unsigned int modified = REGTYPE_FORCE;

	switch (state) {
		case COP_read1:
		  break;
		  
		case COP_read2:
			w1 = cop_state.i1;
			w2 = chipmem_wget_indirect (ip);
  		if (w1 & 1) {
  			if (w2 & 1)
  				return; // SKIP
  			state = COP_wait; // WAIT
  			c_hpos += 4;
  		} else { // MOVE
  			modified |= regtypes[w1 & 0x1FE];
  			state = COP_read1;
  			c_hpos += 2;
  		}
  		ip += 2;	
			break;
			
		case COP_strobe_delay2:
		case COP_strobe_delay2x:
    case COP_start_delay:
			c_hpos += 2;
			state = COP_read1;
			break;
			
    case COP_strobe_extra:
			c_hpos += 6;
			state = COP_read1;
			break;

    case COP_strobe_delay1:
    case COP_strobe_delay1x:
			c_hpos += 4;
			state = COP_read1;
			break;

		case COP_stop:
    case COP_waitforever:
		case COP_bltwait:
		case COP_skip_in2:
    case COP_skip1:
			return;
			
		case COP_wait_in2:
			c_hpos += 2;
		case COP_wait1:
		  w1 = cop_state.i1;
		  w2 = cop_state.i2;
			state = COP_wait;
			break;
			
		case COP_wait:
		  w1 = cop_state.i1;
		  w2 = cop_state.i2;
			break;

		default:
		  return;
	}
	/* Only needed for COP_wait, but let's shut up the compiler.  */
	cop_state.first_sync = c_hpos;
	
	while (c_hpos + 1 < maxhpos) {
		if (state == COP_read1) {
			w1 = chipmem_wget_indirect (ip);
			if (w1 & 1) {
				w2 = chipmem_wget_indirect (ip + 2);
				if (w2 & 1)
					break; // SKIP
				state = COP_wait; // WAIT
				c_hpos += 6;
			} else { // MOVE
				modified |= regtypes[w1 & 0x1FE];
				c_hpos += 4;
			}
			ip += 4;
		} else { // state is COP_wait
			if ((w2 & 0xFE) != 0xFE)
				break;
			else {
				unsigned int vcmp = (w1 & (w2 | 0x8000)) >> 8;
				unsigned int hcmp = (w1 & 0xFE);
				
				unsigned int vp = vpos & (((w2 >> 8) & 0x7F) | 0x80);
				if (vp < vcmp) {
					/* Whee.  We can wait until the end of the line!  */
					c_hpos = maxhpos;
		    } else if (vp > vcmp || hcmp <= c_hpos) {
					state = COP_read1;
					/* minimum wakeup time */
					c_hpos += 2;
				} else {
					state = COP_read1;
					c_hpos = hcmp + 2;
				}
				/* If this is the current instruction, remember that we don't
		       need to sync CPU and copper anytime soon.  */
				if (cop_state.ip == ip) {
					cop_state.first_sync = c_hpos;
				}
			}
		}
	}
	
	cycle_count = c_hpos - cop_state.hpos;
	if (cycle_count >= 8) {
  	cop_state.regtypes_modified = modified;
		unset_special (SPCFLAG_COPPER);
		eventtab[ev_copper].active = 1;
		eventtab[ev_copper].evtime = get_cycles () + cycle_count * CYCLE_UNIT;
		events_schedule ();
	}
}

STATIC_INLINE int custom_wput_copper (int hpos, uaecptr addr, uae_u32 value, int noget)
{
	int v;

	copper_access = 1;
	v = custom_wput_1 (hpos, addr, value, noget);
	copper_access = 0;
	return v;
}

// "emulate" chip internal delays, not the right place but fast and 99.9% programs
// use only copper to write BPLCON1 etc.. (exception is HulkaMania/TSP..)
// this table should be filled with zeros and done somewhere else..
static int customdelay[]= {
  1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0, /* 32 0x00 - 0x3e */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x40 - 0x5e */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x60 - 0x7e */
	0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0, /* 0x80 - 0x9e */
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 32 0xa0 - 0xde */
  /* BPLxPTH/BPLxPTL */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 16 */
  /* BPLCON0-3,BPLMOD1-2 */
	0,0,0,0,0,0,0,0, /* 8 */
	/* BPLxDAT */
	0,0,0,0,0,0,0,0, /* 8 */
  /* SPRxPTH/SPRxPTL */
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 16 */
  /* SPRxPOS/SPRxCTL/SPRxDATA/SPRxDATB */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /* COLORxx */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  /* RESERVED */
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/*
	CPU write COPJMP wakeup sequence when copper is waiting:
	- Idle cycle (can be used by other DMA channel)
	- Read word from current copper pointer (next word after wait instruction) to 1FE
	  This cycle can conflict with blitter DMA.
	Normal copper cycles resume
	- Write word from new copper pointer to 8C
*/

static void update_copper (int until_hpos)
{
	int vp = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
	int c_hpos = cop_state.hpos;
	
  if (nocustom()) {
    eventtab[ev_copper].active = 0;
  	return;
  }

  if(currprefs.fast_copper) {
  	if (eventtab[ev_copper].active) {
      eventtab[ev_copper].active = 0;
  		return;
    }
  }
	
  if (cop_state.state == COP_wait && vp < cop_state.vcmp) {
 	  eventtab[ev_copper].active = 0;
	 	copper_enabled_thisline = 0;
	  cop_state.state = COP_stop;
	  unset_special(SPCFLAG_COPPER);
		return;
	}
	
	if (until_hpos <= last_copper_hpos)
		return;
	
	if (until_hpos > (maxhpos & ~1))
		until_hpos = maxhpos & ~1;
	
	for (;;) {
		int old_hpos = c_hpos;
		int hp;
		
		if (c_hpos >= until_hpos)
			break;
		
		/* So we know about the fetch state.  */
		decide_line (c_hpos);
		// bitplane only, don't want blitter to steal our cycles.
		decide_fetch (c_hpos);

	  if (cop_state.movedelay) { // cop_state.movedelay is 0 or 1
	    cop_state.movedelay = 0;
      custom_wput_copper (c_hpos, cop_state.moveaddr, cop_state.movedata, 0);
	    if (! copper_enabled_thisline)
	      goto out;
	  }

		if ((c_hpos == maxhpos - 3) && (maxhpos & 1))
			c_hpos += 1;
		else
		  c_hpos += 2;

		switch (cop_state.state) 
    {
    	case COP_wait_in2:
  	    if (copper_cant_read (old_hpos))
      		continue;
  	    cop_state.state = COP_wait1;
  	    break;
    	case COP_skip_in2:
  	    if (copper_cant_read (old_hpos))
      		continue;
  	    cop_state.state = COP_skip1;
			  break;

		  case COP_strobe_extra:
			  // Wait 1 copper cycle doing nothing
			  cop_state.state = COP_strobe_delay1;
			  break;
    	case COP_strobe_delay1:
			  // First cycle after COPJMP is just like normal first read cycle
			  // Cycle is used and needs to be free.
			  if (copper_cant_read (old_hpos))
				  continue;
			  if (old_hpos == maxhpos - 2) {
				  // if COP_strobe_delay2 would cross scanlines (positioned immediately
				  // after first strobe/refresh slot) it will disappear!
				  cop_state.state = COP_read1;
				  if (cop_state.strobe == 1)
					  cop_state.ip = cop1lc;
				  else
					  cop_state.ip = cop2lc;
				  cop_state.strobe = 0;
			  } else {
    	    cop_state.state = COP_strobe_delay2;
    			cop_state.ip += 2;
        }
  	    break;
    	case COP_strobe_delay2:
			  // Second cycle after COPJMP. This is the strange one.
			  // This cycle does not need to be free
			  // But it still gets allocated by copper if it is free = CPU and blitter can't use it.
			  cop_state.state = COP_read1;
			  // Next cycle finally reads from new pointer
			  if (cop_state.strobe == 1)
				  cop_state.ip = cop1lc;
			  else
				  cop_state.ip = cop2lc;
			  cop_state.strobe = 0;
			  break;

		  case COP_strobe_delay1x:
			// First cycle after COPJMP and Copper was waiting.
			  cop_state.state = COP_strobe_delay2x;
			  break;
		  case COP_strobe_delay2x:
			// Second cycle fetches following word and tosses it away.
			// Cycle can be free and copper won't allocate it.
			// If Blitter uses this cycle = Copper's PC gets copied to blitter DMA pointer..
			  if (copper_cant_read (old_hpos))
				  continue;
			  cop_state.state = COP_read1;
			  // Next cycle finally reads from new pointer
			  if (cop_state.strobe == 1)
				  cop_state.ip = cop1lc;
			  else
				  cop_state.ip = cop2lc;
			  cop_state.strobe = 0;
		    break;

		  case COP_start_delay:
			// cycle after vblank strobe fetches word from old pointer first
			  if (copper_cant_read (old_hpos))
				  continue;
			  cop_state.state = COP_read1;
			  cop_state.i1 = last_custom_value1 = chipmem_wget_indirect (cop_state.ip);
  			cop_state.ip = cop1lc;
			  break;

			case COP_read1:
  	    if (copper_cant_read (old_hpos))
      		continue;
				cop_state.i1 = last_custom_value1 = chipmem_wget_indirect (cop_state.ip);
				cop_state.ip += 2;
				cop_state.state = COP_read2;
				break;
				
			case COP_read2:
  	    if (copper_cant_read (old_hpos))
      		continue;
				cop_state.i2 = last_custom_value1 = chipmem_wget_indirect (cop_state.ip);
				cop_state.ip += 2;
				
				if (cop_state.i1 & 1) { // WAIT or SKIP
		      cop_state.ignore_next = 0;
					if (cop_state.i2 & 1)
						cop_state.state = COP_skip_in2;
					else
						cop_state.state = COP_wait_in2;
				} else { // MOVE
					unsigned int reg = cop_state.i1 & 0x1FE;
				  uae_u16 data = cop_state.i2;
					cop_state.state = COP_read1;
		      test_copper_dangerous (reg);
		      if (! copper_enabled_thisline)
    			  goto out; // was "dangerous" register -> copper stopped

  				if (cop_state.ignore_next) {
    		    reg = 0x1fe;
    		    cop_state.ignore_next = 0;
      		}

		      if (reg == 0x88) {
		        cop_state.strobe = 1;
		        cop_state.state = COP_strobe_delay1;
		      } else if (reg == 0x8a) {
		        cop_state.strobe = 2;
		        cop_state.state = COP_strobe_delay1;
		      } else {
					  // FIX: all copper writes happen 1 cycle later than CPU writes
					  if (customdelay[reg / 2]) {
						  cop_state.moveaddr = reg;
						  cop_state.movedata = data;
						  cop_state.movedelay = 1;
					  } else {
              custom_wput_copper (old_hpos, reg, data, 0);
					  }
		      }
	      }
			  check_copper_stop();
				break;
				
			case COP_wait1:
				cop_state.state = COP_wait;
				
				cop_state.vcmp = (cop_state.i1 & (cop_state.i2 | 0x8000)) >> 8;
				cop_state.hcmp = (cop_state.i1 & cop_state.i2 & 0xFE);
				
				vp = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
				
	      if (cop_state.i1 == 0xFFFF && cop_state.i2 == 0xFFFE) {
				  cop_state.state = COP_waitforever;
					copper_enabled_thisline = 0;
					unset_special (SPCFLAG_COPPER);
					goto out;
				}
				if (vp < cop_state.vcmp) {
					copper_enabled_thisline = 0;
					unset_special (SPCFLAG_COPPER);
					goto out;
				}
				
				/* fall through */
			case COP_wait:
			  {
				  int ch_comp = c_hpos;
				  if (ch_comp & 1)
					  ch_comp = 0;

				/* First handle possible blitter wait
				 * Must be before following free cycle check
				 */
			    if ((cop_state.i2 & 0x8000) == 0) {
					  decide_blitter(old_hpos);
				    if (bltstate != BLT_done) {
					    /* We need to wait for the blitter.  */
					    cop_state.state = COP_bltwait;
					    copper_enabled_thisline = 0;
					    unset_special (SPCFLAG_COPPER);
					    goto out;
				    }
			    }
				
    	    if (copper_cant_read (old_hpos))
        		continue;
				
				  hp = ch_comp & (cop_state.i2 & 0xFE);
	        if (vp == cop_state.vcmp && hp < cop_state.hcmp) {
					  /* Position not reached yet.  */
            if(currprefs.fast_copper) {
    					if ((cop_state.i2 & 0xFE) == 0xFE) {
    						int wait_finish = cop_state.hcmp - 2;
    						/* This will leave c_hpos untouched if it's equal to wait_finish.  */
    						if (wait_finish < c_hpos)
    							return;
    						else if (wait_finish <= until_hpos) {
    							c_hpos = wait_finish;
    						} else
    							c_hpos = until_hpos;
    					}
    				}
					  break;
				  }
				  cop_state.state = COP_read1;
			  }
				break;
				
	  case COP_skip1:
	    {
	      unsigned int vcmp, hcmp, vp1, hp1;

				if (c_hpos >= (maxhpos & ~1) || (c_hpos & 1))
      		break;

	      if (copper_cant_read (old_hpos))
		      continue;

	      vcmp = (cop_state.i1 & (cop_state.i2 | 0x8000)) >> 8;
	      hcmp = (cop_state.i1 & cop_state.i2 & 0xFE);
	      vp1 = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
	      hp1 = c_hpos & (cop_state.i2 & 0xFE);

	      if ((vp1 > vcmp || (vp1 == vcmp && hp1 >= hcmp)) && ((cop_state.i2 & 0x8000) != 0 || bltstate == BLT_done))
		      cop_state.ignore_next = 1;

	      cop_state.state = COP_read1;
	      break;
	    }
			default:
				break;
		}
	}
	
out:
	cop_state.hpos = c_hpos;
	last_copper_hpos = until_hpos;
	
  if(currprefs.fast_copper) {
    /* The test against maxhpos also prevents us from calling predict_copper
       when we are being called from hsync_handler, which would not only be
       stupid, but actively harmful.  */
	  if ((regs.spcflags & SPCFLAG_COPPER) && (c_hpos + 8 < maxhpos))
		  predict_copper ();
  }
}

static void compute_spcflag_copper (int hpos)
{
	int wasenabled = copper_enabled_thisline;

	copper_enabled_thisline = 0;
	unset_special (SPCFLAG_COPPER);
  if (!dmaen (DMA_COPPER) || cop_state.state == COP_stop || cop_state.state == COP_waitforever || cop_state.state == COP_bltwait || nocustom())
		return;
	
	if (cop_state.state == COP_wait) {
		int vp = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
		
		if (vp < cop_state.vcmp)
			return;
	}
	// do not use past cycles if starting for the first time in this line
	// (write to DMACON for example) hpos+1 for long lines
	if (!wasenabled && cop_state.hpos < hpos && hpos < maxhpos) {
		hpos = (hpos + 2) & ~1;
		if (hpos > (maxhpos & ~1))
			hpos = maxhpos & ~1;
		cop_state.hpos = hpos;
	}

	// if COPJMPx was written while DMA was disabled, advance to next state,
	// COP_strobe_extra is single cycle only and does not need free bus.
	// (copper state emulation does not run if DMA is disabled)
	if (!wasenabled && cop_state.state == COP_strobe_extra)
		cop_state.state = COP_strobe_delay1;

	copper_enabled_thisline = 1;
	
  if(currprefs.fast_copper) {
	  predict_copper ();
	  if (! eventtab[ev_copper].active)
	    set_special (SPCFLAG_COPPER);
	}
  else
	  set_special (SPCFLAG_COPPER);
}

static void copper_handler (void)
{
  /* This will take effect immediately, within the same cycle.  */
  set_special (SPCFLAG_COPPER);
	
	if (! copper_enabled_thisline)
		return;
	
	eventtab[ev_copper].active = 0;
}

void blitter_done_notify (int hpos)
{
	if (cop_state.state != COP_bltwait)
		return;
	
	int vp_wait = vpos & (((cop_state.i2 >> 8) & 0x7F) | 0x80);
	int vp = vpos;

	hpos++;
	hpos &= ~1;
	if (hpos >= maxhpos) {
		hpos -= maxhpos;
		vp++;
	}
	cop_state.hpos = hpos;
	cop_state.state = COP_wait;
	/* No need to check blitter state again */
	cop_state.i2 |= 0x8000;

	if (dmaen(DMA_COPPER) && vp_wait >= cop_state.vcmp) {
		copper_enabled_thisline = 1;
		set_special(SPCFLAG_COPPER);
	} else {
		unset_special (SPCFLAG_COPPER);
	}
}

void do_copper (void)
{
	int hpos = current_hpos ();
	update_copper (hpos);
}

/* ADDR is the address that is going to be read/written; this access is
   the reason why we want to update the copper.  This function is also
   used from hsync_handler to finish up the line; for this case, we check
   hpos against maxhpos.  */
STATIC_INLINE void sync_copper_with_cpu (int hpos, int do_schedule, unsigned int addr)
{
	if (eventtab[ev_copper].active) {
		if (hpos != maxhpos) {
			/* There might be reasons why we don't actually need to bother
	       updating the copper.  */
			if (hpos < cop_state.first_sync)
				return;
			
			if ((cop_state.regtypes_modified & regtypes[addr & 0x1FE]) == 0)
				return;
		}
		
		eventtab[ev_copper].active = 0;
		if (do_schedule)
			events_schedule ();
	  set_special (SPCFLAG_COPPER);
	}

	/* Need to let the copper advance to the current position.  */
	if (copper_enabled_thisline)
		update_copper (hpos);
}

STATIC_INLINE uae_u16 sprite_fetch(struct sprite *s, uaecptr pt)
{
  last_custom_value1 = chipmem_wget_indirect (pt);
  return last_custom_value1;
}

STATIC_INLINE void sprite_fetch_full(struct sprite *s, int hpos, uae_u16 *v0, uae_u32 *v1, uae_u32 *v2)
{
	uae_u32 data321 = 0, data322 = 0;
	uae_u16 data16;

	if (sprite_width == 16) {

		data16 = sprite_fetch (s, s->pt);
		s->pt += 2;

	} else if (sprite_width == 64) {

		uaecptr pm = s->pt & ~7;
		uaecptr pm1, pm2;
		if (s->pt & 4) {
			pm1 = pm + 4;
			pm2 = pm + 4;
		} else {
			pm1 = pm;
			pm2 = pm + 4;
		}
		data321 = sprite_fetch(s, pm1) << 16;
		data321 |= chipmem_wget_indirect(pm1 + 2);
		data322 = chipmem_wget_indirect(pm2) << 16;
		data322 |= chipmem_wget_indirect(pm2 + 2);
		if (s->pt & 2) {
			data321 &= 0x0000ffff;
			data322 &= 0x0000ffff;
			data321 |= data321 << 16;
			data322 |= data322 << 16;
		}
		data16 = data321 >> 16;
		s->pt += 8;

	} else { // 32

		uaecptr pm = s->pt & ~3;
		data321 = sprite_fetch(s, pm) << 16;
		data321 |= chipmem_wget_indirect(pm + 2);
		if (s->pt & 2) {
			data321 &= 0x0000ffff;
			data321 |= data321 << 16;
		} else if (fetchmode_fmode & 8) {
			data321 &= 0xffff0000;
			data321 |= data321 >> 16;
		}
		data16 = data321 >> 16;
		s->pt += 4;

	}

	*v0 = data16;
	*v1 = data321;
	*v2 = data322;
}

STATIC_INLINE void do_sprites_1(int num, int cycle, int hpos)
{
	struct sprite *s = &spr[num];
	int posctl = 0;
	uae_u16 data;
	// fetch both sprite pairs even if DMA was switched off between sprites
	int isdma = dmaen (DMA_SPRITE) || ((num & 1) && spr[num & ~1].dmacycle);
	bool unaligned = (spr[num].pt & 2) != 0;

	if (isdma && s->dblscan && (fmode & 0x8000) && (vpos & 1) != (s->vstart & 1) && s->dmastate) {
		spr_arm (num, 1);
		return;
	}
	if (vpos == s->vstart) {
		s->dmastate = 1;
	}
	if (vpos == s->vstop || vpos == sprite_vblank_endline) {
		s->dmastate = 0;
	}

	if (!isdma)
		return;

	int dma = hpos < plfstrt_sprite || diwstate != DIW_waiting_stop;
	int sprxp = s->xpos >> (sprite_buffer_res + 1);
	bool start_before_dma = hpos >= sprxp && sprxp >= 16;
	if (vpos == s->vstop || vpos == sprite_vblank_endline) {
		s->dmastate = 0;
		posctl = 1;
		if (dma) {
			uae_u32 data321, data322;
			sprite_fetch_full(s, hpos, &data, &data321, &data322);
			if (cycle == 0) {
				if (start_before_dma && s->armed) {
					maybe_decide_sprites(num, hpos);
				}
				SPRxPOS_1 (data, num);
				s->dmacycle = 1;
			} else {
				// This is needed to disarm previous field's sprite.
				// It can be seen on OCS Agnus + ECS Denise combination where
				// this cycle is disabled due to weird DDFTSTR=$18 copper list
				// which causes corrupted sprite to "wrap around" the display.
				SPRxCTL_1 (data, num);
				s->dmastate = 0;
				sprstartstop (s);
			}
		}
		if (vpos == sprite_vblank_endline) {
			// s->vstart == sprite_vblank_endline won't enable the sprite.
			s->dmastate = 0;
		}
	}
	if (s->dmastate && !posctl && dma) {
		uae_u32 data321, data322;
		sprite_fetch_full(s, hpos, &data, &data321, &data322);
		if (cycle == 0) {
			// if xpos is earlier than this cycle, decide it first.
			if (start_before_dma) {
				maybe_decide_sprites(num, hpos);
			}
			SPRxDATA_1 (data, num);
			s->dmacycle = 1;
		} else {
			// This is needed if xpos is between DATA and DATB fetches
			// Test does not need to be accurate, only purpose is to
			// not lose performance when sprites have "normal" positioning.
			if (start_before_dma) {
				maybe_decide_sprites(num, hpos);
			}
			SPRxDATB_1 (data, num);
		}
		switch (sprite_width)
		{
		case 64:
			if (cycle == 0) {
				sprdata[num][1] = data321;
				sprdata[num][2] = data322 >> 16;
				sprdata[num][3] = data322;
			} else {
				sprdatb[num][1] = data321;
				sprdatb[num][2] = data322 >> 16;
				sprdatb[num][3] = data322;
			}
			break;
		case 32:
			if (cycle == 0)
				sprdata[num][1] = data321;
			else
				sprdatb[num][1] = data321;
			break;
		}
	}
}

static void do_sprites (int hpos)
{
  int maxspr, minspr;
  int i;

  if (vpos < sprite_vblank_endline)
  	return;
  
  maxspr = hpos;
  minspr = last_sprite_hpos + 1;
  
  if (minspr >= maxspr || last_sprite_hpos == hpos)
  	return;
  
  if (maxspr >= SPR0_HPOS + MAX_SPRITES * 4)
  	maxspr = SPR0_HPOS + MAX_SPRITES * 4 - 1;
  if (minspr < SPR0_HPOS)
  	minspr = SPR0_HPOS;
  
  if (minspr == maxspr)
	  return;

  for (i = minspr; i <= maxspr; i++) {
    int cycle = -1;
    int num = (i - SPR0_HPOS) / 4;
    switch ((i - SPR0_HPOS) & 3)
    {
      case 0:
        cycle = 0;
        spr[num].dmacycle = 0;
        break;
      case 2:
        cycle = 1;
        break;
    }
    if (cycle >= 0) {
      do_sprites_1 (num, cycle, i);
    }
  }

  last_sprite_hpos = hpos;
}

static void init_sprites (void)
{
  memset (sprpos, 0, sizeof sprpos);
  memset (sprctl, 0, sizeof sprctl);
}

static void init_hardware_frame (void)
{
  next_lineno = 0;
  prev_lineno = -1;
  diwstate = DIW_waiting_start;
  ddfstate = DIW_waiting_start;
  plf_state = plf_end;
}

void init_hardware_for_drawing_frame (void)
{
	/* Avoid this code in the first frame after a customreset.  */
	if (next_sprite_entry > 0) {
		int npixels = curr_sprite_entries[next_sprite_entry].first_pixel;
		memset(spixels, 0, npixels * sizeof *spixels);
		memset(spixstate.bytes, 0, npixels * sizeof *spixstate.bytes);
	}
	
	next_color_change = 0;
	next_sprite_entry = 0;
	next_color_entry = 0;
	remembered_color_entry = -1;
	
	curr_sprite_entries[0].first_pixel = 0;
	next_sprite_forced = 1;
}

static int rpt_vsync (void)
{
	frame_time_t curr_time = read_processor_time ();
	int v = curr_time - vsyncwaittime;
	if (v > syncbase || v < -syncbase) {
		vsyncmintime = vsyncmaxtime = vsyncwaittime = curr_time;
		v = 0;
	}
	return v;
}

STATIC_INLINE void rtg_vsync (void)
{
#ifdef PICASSO96
	frame_time_t start, end;
	start = read_processor_time ();
	picasso_handle_vsync ();
	end = read_processor_time ();
	frameskiptime += end - start;
#endif
}


// moving average algorithm
#define MAVG_MAX_SIZE 128
struct mavg_data
{
	int values[MAVG_MAX_SIZE];
	int size;
	int offset;
	int mavg;
};

static void mavg_clear (struct mavg_data *md)
{
	md->size = 0;
	md->offset = 0;
	md->mavg = 0;
}

static int mavg (struct mavg_data *md, int newval, int size)
{
	if (md->size < size) {
		md->values[md->size++] = newval;
		md->mavg += newval;
	} else {
		md->mavg -= md->values[md->offset];
		md->values[md->offset] = newval;
		md->mavg += newval;
		md->offset++;
		if (md->offset >= size)
			md->offset -= size;
	}
	return md->mavg / md->size;
}

#define MAVG_VSYNC_SIZE 128

static bool framewait (void)
{
	frame_time_t curr_time;
	frame_time_t start;
	int vs = isvsync_chipset ();
	int status = 0;

	is_syncline = 0;

	static struct mavg_data ma_frameskipt;
	int frameskipt_avg = mavg (&ma_frameskipt, frameskiptime, MAVG_VSYNC_SIZE);

	frameskiptime = 0;

	if (vs > 0) {
		static struct mavg_data ma_legacy;
		static frame_time_t vsync_time;
		int t;

		curr_time = read_processor_time();
		vsyncwaittime = vsyncmaxtime = curr_time + vsynctimebase;
  		if (!frame_rendered && !picasso_on)
  			frame_rendered = render_screen (false);
  
		start = read_processor_time();
		t = 0;
		if ((int)start - (int)vsync_time >= 0 && (int)start - (int)vsync_time < vsynctimebase)
			t += (int)start - (int)vsync_time;

  		if (!frame_shown) {
  			show_screen (1);
  		}

		int legacy_avg = mavg(&ma_legacy, t, MAVG_VSYNC_SIZE);
		if (t > legacy_avg)
			legacy_avg = t;
		t = legacy_avg;

		vsync_time = read_processor_time();
		if (t > vsynctimebase * 2 / 3)
			t = vsynctimebase * 2 / 3;
    
		if (currprefs.m68k_speed < 0) {
			vsynctimeperline = (vsynctimebase - t) / (maxvpos_display + 1);
		}
		else {
			vsynctimeperline = (vsynctimebase - t) / 3;
		}

		if (vsynctimeperline < 1)
			vsynctimeperline = 1;

		frame_shown = true;
		return 1;
	}

	status = 1;

	int vstb = vsynctimebase;

	if (currprefs.m68k_speed < 0) {

		if (!frame_rendered && !picasso_on)
			frame_rendered = render_screen (false);

		curr_time = read_processor_time ();

		int max;
		int adjust = 0;
		if ((int)curr_time - (int)vsyncwaittime > 0 && (int)curr_time - (int)vsyncwaittime < vstb / 2)
			adjust += curr_time - vsyncwaittime;
		max = (int)(vstb * (1000.0) / 1000.0 - adjust);
		vsyncwaittime = curr_time + vstb - adjust;
		vsyncmintime = curr_time;

		if (max < 0) {
			max = 0;
			vsynctimeperline = 1;
		} else {
			vsynctimeperline = max / (maxvpos_display + 1);
		}
		vsyncmaxtime = curr_time + max;

	} else {

		int t = 0;

		start = read_processor_time ();
		if (!frame_rendered && !picasso_on) {
			frame_rendered = render_screen (false);
			t = read_processor_time () - start;
		}
		while (true) {
			auto v = rpt_vsync() / (syncbase / 1000.0);
			if (v >= -2)
				break;
			cpu_sleep_millis(1);
		}
		while (rpt_vsync () < 0) {
			;
		}
		idletime += read_processor_time() - start;
		curr_time = read_processor_time ();
		vsyncmintime = curr_time;
		vsyncmaxtime = vsyncwaittime = curr_time + vstb;
		if (frame_rendered) {
			show_screen (0);
			t += read_processor_time () - curr_time;
		}
		t += frameskipt_avg;
		vsynctimeperline = (vstb - t) / 3;
		if (vsynctimeperline < 0)
			vsynctimeperline = 0;
		else if (vsynctimeperline > vstb / 3)
			vsynctimeperline = vstb / 3;
		
		frame_shown = true;
  }
	return status != 0;
}

#define FPSCOUNTER_MAVG_SIZE 10
static struct mavg_data fps_mavg, idle_mavg;

void fpscounter_reset (void)
{
	mavg_clear (&fps_mavg);
	mavg_clear (&idle_mavg);
  lastframetime = read_processor_time ();
	idletime = 0;
}

static void fpscounter(bool frameok)
{
	frame_time_t now, last;

	now = read_processor_time();
	last = now - lastframetime;
	lastframetime = now;

	if ((int)last < 0)
		return;

	if (currprefs.gfx_framerate)
		idletime >>= 1;

	mavg(&fps_mavg, last / 10, FPSCOUNTER_MAVG_SIZE);
	mavg(&idle_mavg, idletime / 10, FPSCOUNTER_MAVG_SIZE);
	idletime = 0;

	timeframes++;

	if ((timeframes & 7) == 0) {
		double idle = 1000 - (idle_mavg.mavg == 0 ? 0.0 : (double)idle_mavg.mavg * 1000.0 / vsynctimebase);
		int fps = fps_mavg.mavg == 0 ? 0 : syncbase * 10 / fps_mavg.mavg;
		if (fps > 9999)
			fps = 9999;
		if (idle < 0)
			idle = 0;
		if (idle > 100 * 10)
			idle = 100 * 10;
		gui_data.fps = fps;
		gui_data.idle = (int)idle;
		gui_data.fps_color = frameok ? 0 : 1;
	}
}

// vsync functions that are not hardware timing related
static void vsync_handler_pre(void)
{
	config_check_vsync();
	if (timehack_alive > 0)
		timehack_alive--;

#ifdef PICASSO96
	rtg_vsync();
#endif

	if (!vsync_rendered) {
		frame_time_t start, end;
		start = read_processor_time();
		vsync_handle_redraw();
		vsync_rendered = true;
		end = read_processor_time();
		frameskiptime += end - start;
	}

	bool frameok = framewait();

	if (!picasso_on && !nodraw()) {
		if (!frame_rendered) {
			frame_rendered = render_screen(false);
		}
		if (frame_rendered && !frame_shown) {
			frame_shown = show_screen_maybe(true);
		}
	}

	// GUI check here, must be after frame rendering
	devices_vsync_pre();

	if (!nodraw() || (picasso_on && picasso_rendered))
		fpscounter(frameok);

	handle_events();

	if (quit_program > 0) {
		/* prevent possible infinite loop at wait_cycles().. */
		framecnt = 0;
		reset_decisions();
		return;
	}

	vsync_rendered = false;
	frame_shown = false;
	frame_rendered = false;

	vsync_handle_check();
}

// emulated hardware vsync
static void vsync_handler_post (void)
{
	DISK_vsync ();

	if (bplcon0 & 4) {
		lof_store = lof_store ? 0 : 1;
	}

	if (lof_prev_lastline != lof_lastline) {
		if (lof_togglecnt_lace < LOF_TOGGLES_NEEDED)
			lof_togglecnt_lace++;
		if (lof_togglecnt_lace >= LOF_TOGGLES_NEEDED)
			lof_togglecnt_nlace = 0;
	} else {
		// only 1-2 vblanks with bplcon0 lace bit set?
		// lets check if lof has changed
		if (!(bplcon0 & 4) && lof_togglecnt_lace > 0 && lof_togglecnt_lace < LOF_TOGGLES_NEEDED && !interlace_seen) {
			lof_changed = 1;
		}
		lof_togglecnt_nlace = LOF_TOGGLES_NEEDED;
		lof_togglecnt_lace = 0;
	}
	lof_prev_lastline = lof_lastline;
	lof_current = lof_store;
	if (lof_togglecnt_lace >= LOF_TOGGLES_NEEDED) {
		interlace_changed = notice_interlace_seen (true);
	} else if (lof_togglecnt_nlace >= LOF_TOGGLES_NEEDED) {
		interlace_changed = notice_interlace_seen (false);
	}
	if (lof_changing) {
		// still same? Trigger change now.
		if ((!lof_store && lof_changing < 0) || (lof_store && lof_changing > 0)) {
			lof_changed_previous_field++;
			lof_changed = 1;
			// lof toggling? decide as interlace.
			if (lof_changed_previous_field >= LOF_TOGGLES_NEEDED) {
				lof_changed_previous_field = LOF_TOGGLES_NEEDED;
				if (lof_lace == false)
					lof_lace = true;
				else
					lof_changed = 0;
			}
			if (bplcon0 & 4)
				lof_changed = 0;
		}
		lof_changing = 0;
	} else {
		lof_changed_previous_field = 0;
		lof_lace = false;
	}
	
	if (varsync_changed || (beamcon0 & (0x10 | 0x20 | 0x80 | 0x100 | 0x200)) != (new_beamcon0 & (0x10 | 0x20 | 0x80 | 0x100 | 0x200))) {
		init_hz_normal();
  } else if (vpos_count > 0 && abs (vpos_count - vpos_count_diff) > 1 && vposw_change < 4) {
		init_hz_vposw();
	} else if (interlace_changed || changed_chipset_refresh () || lof_changed) {
		compute_framesync ();
  }

	lof_changed = 0;
	vposw_change = 0;

  eventtab[ev_copper].active = 0;
  COPJMP (1, 1);
		
	init_hardware_frame ();
}

static void events_dmal (int);
static uae_u16 dmal, dmal_hpos;

static void dmal_emu (uae_u32 v)
{
	// Disk and Audio DMA bits are ignored by Agnus, Agnus only checks DMAL and master bit
	if (!(dmacon & DMA_MASTER))
		return;
	int hpos = current_hpos ();
	if (v >= 6) {
		v -= 6;
		int nr = v / 2;
		uaecptr pt = audio_getpt (nr, (v & 1) != 0);
		uae_u16 dat = chipmem_wget_indirect (pt);
		last_custom_value1 = dat;
		AUDxDAT (nr, dat);
	} else {
		uae_u16 dat;
		int w = v & 1;
		uaecptr pt = disk_getpt ();
		// disk_fifostatus() needed in >100% disk speed modes
		if (w) {
			// write to disk
			if (disk_fifostatus () <= 0) {
				dat = chipmem_wget_indirect (pt);
        last_custom_value1 = dat;
				DSKDAT (dat);
			}
		} else {
			// read from disk
			if (disk_fifostatus () >= 0) {
				dat = DSKDATR ();
				chipmem_wput_indirect (pt, dat);
			}
		}
	}
}
			  
static void dmal_func (uae_u32 v)
{
	dmal_emu (v);
	events_dmal (0);
}

static void dmal_func2 (uae_u32 v)
{
	while (dmal) {
		if (dmal & 3)
			dmal_emu (dmal_hpos + ((dmal & 2) ? 1 : 0));
		dmal_hpos += 2;
		dmal >>= 2;
	}
}

static void events_dmal (int hp)
{
	if (!dmal)
		return;
	if (currprefs.cachesize) {
		dmal_func2 (0);
	} else {
		event2_newevent2 (hp, 13, dmal_func2);
	}
}

static void events_dmal_hsync (void)
{
	dmal = audio_dmal ();
	dmal <<= 6;
	dmal |= disk_dmal ();
	if (!dmal)
		return;
	dmal_hpos = 0;
	events_dmal (7);
}

static bool is_custom_vsync (void)
{
	int vp = vpos + 1;
	int vpc = vpos_count + 1;
	/* Agnus vpos counter keeps counting until it wraps around if VPOSW writes put it past maxvpos */
	if (vp >= maxvpos_total)
		vp = 0;
	if (vp == maxvpos + lof_store || vp == maxvpos + lof_store + 1 || vpc >= MAXVPOS) {
		// vpos_count >= MAXVPOS just to not crash if VPOSW writes prevent vsync completely
		return true;
	}
	return false;
}

static void set_hpos (void)
{
	eventtab[ev_hsync].evtime = get_cycles () + HSYNCTIME;
	eventtab[ev_hsync].oldcycles = get_cycles ();
}

// this finishes current line
static void hsync_handler_pre (bool onvsync)
{
  int hpos = current_hpos ();

  if (!nocustom()) {
    /* Using 0x8A makes sure that we don't accidentally trip over the
       modified_regtypes check.  */
    sync_copper_with_cpu (maxhpos, 0, 0x8A);

	// Seven Seas scrolling quick fix hack
	// checks if copper is going to modify BPLCON1 in next cycle.
	if (copper_enabled_thisline && cop_state.state == COP_read2 && (cop_state.i1 & 0x1fe) == 0x102) {
		// it did, pre-load value for Denise shifter emulation
		hpos_is_zero_bplcon1_hack = chipmem_wget_indirect(cop_state.ip);
		// following finish_decision() is going to finish this line
		// it is too late when copper actually does the move
	}

    finish_decisions ();
    if (thisline_decision.plfleft >= 0) {
      if (currprefs.collision_level > 1)
        DO_SPRITE_COLLISIONS
      if (currprefs.collision_level > 2)
        DO_PLAYFIELD_COLLISIONS
    }
    hsync_record_line_state (next_lineno);
  }
   
	devices_hsync();

  hsync_counter++;

  vpos++;
	vpos_count++;
  if (vpos >= maxvpos_total)
    vpos=0;
	if (onvsync) {
		vpos = 0;
	}
	set_hpos ();
}

STATIC_INLINE bool is_last_line (void)
{
	return vpos + 1 == maxvpos + lof_store;
}

// this prepares for new line
static void hsync_handler_post (bool onvsync)
{
	last_copper_hpos = 0;

	bool ciahsyncs = !(bplcon0 & 2);
	bool ciavsyncs = !(bplcon0 & 2);

	CIA_hsync_posthandler(false);
	if (currprefs.cs_cd32cd) {
		CIA_hsync_posthandler(true);
		CIAB_tod_handler(18);
	} else if (ciahsyncs) {
		CIA_hsync_posthandler(true);
		if (beamcon0 & (0x80 | 0x100)) {
			if (hsstop < (maxhpos & ~1) && hsstrt < maxhpos)
				CIAB_tod_handler(hsstop);
		} else {
			CIAB_tod_handler(18);
		}
	}

	if (currprefs.cs_cd32cd) {

		if (cia_hsync < maxhpos) {
			CIAA_tod_inc(cia_hsync);
			cia_hsync += (akiko_ntscmode() ? 262 : 313) * maxhpos;
		} else {
			cia_hsync -= maxhpos;
		}

	} else if (currprefs.cs_ciaatod > 0) {
		if (cia_hsync < maxhpos) {
			int newcount;
			CIAA_tod_inc (cia_hsync);
			newcount = (int)((vblank_hz * (2 * maxvpos + (interlace_seen ? 1 : 0)) * (2 * maxhpos + (islinetoggle () ? 1 : 0))) / ((currprefs.cs_ciaatod == 2 ? 60 : 50) * 4));
			cia_hsync += newcount;
		} else {
			cia_hsync -= maxhpos;
		}
	} else if (currprefs.cs_ciaatod == 0 && ciavsyncs) {
		// CIA-A TOD counter increases when vsync pulse ends
		if (beamcon0 & (0x80 | 0x200)) {
			if (vpos == vsstop && vsstrt <= maxvpos)
				CIAA_tod_inc (lof_store ? hsstop : hsstop + hcenter);
		} else {
			if (vpos == (currprefs.ntscmode ? VSYNC_ENDLINE_NTSC : VSYNC_ENDLINE_PAL)) {
				CIAA_tod_inc (lof_store ? 132 : 18);
			}
		}
	}

  inputdevice_hsync ();

  if (!nocustom()) {
	  if (bltstate != BLT_done && dmaen (DMA_BITPLANE) && diwstate == DIW_waiting_stop) {
		  blitter_slowdown (thisline_decision.plfleft, thisline_decision.plfright - (16 << fetchmode),
		    cycle_diagram_total_cycles[fetchmode][bplcon0_res][bplcon0_planes_limit],
		    cycle_diagram_free_cycles[fetchmode][bplcon0_res][bplcon0_planes_limit]);
	  }
  }

	if (onvsync) {
		vpos = 0;
		vsync_handler_post ();
		vpos_count = 0;
  }
	if (vpos == 0)
		send_interrupt (5);
   
	// lastline - 1?
	if (vpos + 1 == maxvpos + lof_store || vpos + 1 == maxvpos + lof_store + 1) {
		lof_lastline = lof_store != 0;
	}

	events_dmal_hsync ();
  
  if (currprefs.m68k_speed < 0) {
	  if (is_last_line ()) {
		  /* really last line, just run the cpu emulation until whole vsync time has been used */
		  vsyncmintime = vsyncmaxtime; /* emulate if still time left */
		  if(vsyncmaxtime - read_processor_time () > -speedup_timelimit)
		    is_syncline = 2;
	  } else {
		  /* end of scanline, run cpu emulation as long as we still have time */
		  vsyncmintime += vsynctimeperline;
		  is_syncline = 0;
		  if ((int)vsyncmaxtime - (int)vsyncmintime > 0) {
			  if ((int)vsyncwaittime - (int)vsyncmintime > 0) {
				  frame_time_t rpt = read_processor_time ();
				  /* Extra time left? Do some extra CPU emulation */
				  if ((int)vsyncmintime - (int)rpt > -speedup_timelimit) {
					  is_syncline = 1;
				  }
			  }
		  }
    }
	} else {
		if (vpos + 1 < maxvpos + lof_store && (vpos == maxvpos_display * 1 / 3 || vpos == maxvpos_display * 2 / 3)) {
			vsyncmintime += vsynctimeperline;
		}
  }

  if (!nocustom()) {
    int lineno = vpos;
		if (lineno >= MAXVPOS)
			lineno %= MAXVPOS;
		if (currprefs.gfx_vresolution) {
			lineno *= 2;
			if (interlace_seen) {
				if (!lof_current) {
					lineno++;
				}
			}
		}
  	prev_lineno = next_lineno;
    next_lineno = lineno;
    reset_decisions ();
  }

	rethink_uae_int();

	/* Default to no bitplane DMA overriding sprite DMA */
	plfstrt_sprite = 0xff;
  /* See if there's a chance of a copper wait ending this line.  */
  cop_state.hpos = 0;
	compute_spcflag_copper (maxhpos);
}

static void init_regtypes (void)
{
  int i;
  for (i = 0; i < 512; i += 2) {
  	regtypes[i] = REGTYPE_ALL;
  	if ((i >= 0x20 && i < 0x26) || i == 0x7E)
	    regtypes[i] = REGTYPE_DISK;
  	else if (i >= 0x68 && i < 0x70)
	    regtypes[i] = REGTYPE_NONE;
  	else if (i >= 0x40 && i < 0x76)
	    regtypes[i] = REGTYPE_BLITTER;
  	else if (i >= 0xA0 && i < 0xE0 && (i & 0xF) < 0xC)
	    regtypes[i] = REGTYPE_AUDIO;
  	else if ((i >= 0xA0 && i < 0xE0) || (i >= 0x1C0 && i < 0x1E4) || (i >= 0x1E6 && i < 0x1FC))
	    regtypes[i] = REGTYPE_NONE;
  	else if ((i >= 0xE0 && i < 0x100) || (i >= 0x110 && i < 0x120))
	    regtypes[i] = REGTYPE_PLANE;
  	else if (i >= 0x120 && i < 0x180)
	    regtypes[i] = REGTYPE_SPRITE;
  	else if (i >= 0x180 && i < 0x1C0)
	    regtypes[i] = REGTYPE_COLOR;
  	else switch (i) {
  		case 0x00:
  		case 0x08:
  		case 0x18:
  		case 0x26: case 0x28:
  		case 0x30: case 0x32:
  		case 0x38: case 0x3A: case 0x3C: case 0x3E:
  		case 0x76: case 0x78: case 0x7A: case 0x7C:
  		case 0x8C:
  		case 0x1FE:
  		  regtypes[i] = REGTYPE_NONE;
  		  break;
  		case 0x02:
  			/* DMACONR - setting this to REGTYPE_BLITTER will cause it to
	       conflict with DMACON (since that is REGTYPE_ALL), and the
	       blitter registers (for the BBUSY bit), but nothing else,
	       which is (I think) what we want. */
  			regtypes[i] = REGTYPE_BLITTER;
  			break;
  		case 0x04: case 0x06: case 0x2A: case 0x2C:
  		case 0x0A: case 0x0C: /* Mouse position is calculated with vpos */
  			regtypes[i] = REGTYPE_POS;
  			break;
  		case 0x0E:
  		  regtypes[i] = REGTYPE_SPRITE;
  		  break;
  		case 0x10: case 0x9E:
  		  regtypes[i] = REGTYPE_AUDIO | REGTYPE_DISK;
  		  break;
  		case 0x12: case 0x14: case 0x16:
  		case 0x34: case 0x36:
  			regtypes[i] = REGTYPE_JOYPORT;
  			break;
  		case 0x1A:
  		  regtypes[i] = REGTYPE_DISK;
  		  break;
  		case 0x102:	case 0x104:	case 0x106:	case 0x108:
  		case 0x10A:
  			regtypes[i] = REGTYPE_PLANE;
  			break;
  	  case 0x10C:
  	    regtypes[i] = REGTYPE_PLANE | REGTYPE_SPRITE;
  			break;
  		case 0x88: case 0x8A:
  		case 0x8E: case 0x90: case 0x92: case 0x94:
  		case 0x96:
  		case 0x100:
  		case 0x1FC:
  			regtypes[i] |= REGTYPE_FORCE;
  			break;
  	}
  }
}

static void hsync_handler (void)
{
	bool vs = is_custom_vsync ();
	hsync_handler_pre (vs);
	if (vs) {
		vsync_handler_pre ();
		if (savestate_check ()) {
			uae_reset (0, 0);
			return;
		}
	}
	hsync_handler_post (vs);
}

void init_eventtab (void)
{
	int i;
	
  nextevent = 0;
	for (i = 0; i < ev_max; i++) {
		eventtab[i].active = 0;
		eventtab[i].oldcycles = get_cycles ();
	}
  for (i = 0; i < ev2_max; i++) {
  	eventtab2[i].active = 0;
	}
	
	eventtab[ev_cia].handler = CIA_handler;
	eventtab[ev_hsync].handler = hsync_handler;
  eventtab[ev_hsync].evtime = get_cycles () + HSYNCTIME;
	eventtab[ev_hsync].active = 1;
	eventtab[ev_copper].handler = copper_handler;
	eventtab[ev_copper].active = 0;
  eventtab[ev_misc].handler = MISC_handler;
	eventtab[ev_audio].handler = audio_evhandler;

	eventtab2[ev2_blitter].handler = blitter_handler;
  eventtab2[ev2_disk].handler = DISK_handler;

	events_schedule ();
}

void custom_prepare (void)
{
	set_hpos ();
	hsync_handler_post (true);
}

void custom_reset (bool hardreset, bool keyboardreset)
{
	if (hardreset)
		board_prefs_changed(-1, -1);

  target_reset ();
  reset_all_systems ();
	write_log (_T("Reset at %08X. Chipset mask = %08X\n"), M68K_GETPC, currprefs.chipset_mask);

	nr_armed = 0;

  if (! savestate_state) {
		cia_hsync = 0;
    hsync_counter = 0;
		currprefs.chipset_mask = changed_prefs.chipset_mask;
  	update_mirrors();

		if (hardreset) {
		  if (!aga_mode) {
				uae_u16 c = ((currprefs.chipset_mask & CSMASK_ECS_DENISE) && !(currprefs.chipset_mask & CSMASK_AGA)) ? 0xfff : 0x000;
			  for (int i = 0; i < 32; i++) {
		      current_colors.color_regs_ecs[i] = c;
					current_colors.acolors[i] = getxcolor (c);
			  }
		  } else {
				uae_u32 c = 0;
			  for (int i = 0; i < 256; i++) {
				  current_colors.color_regs_aga[i] = c;
					current_colors.acolors[i] = getxcolor (c);
			  }
		  }
		  lof_store = lof_current = 0;
		  lof_lace = false;
    }		

		clxdat = 0;
		
		/* Clear the armed flags of all sprites.  */
		memset (spr, 0, sizeof spr);
		
		dmacon = 0;
    intreq = 0;
		intena = 0;

		copcon = 0;
		DSKLEN (0, 0);
		
		bplcon0 = 0;
		bplcon4 = 0x0011; /* Get AGA chipset into ECS compatibility mode */
		bplcon3 = 0x0C00;

		diwhigh = 0;
		diwhigh_written = 0;
		hdiwstate = DIW_waiting_start; // this does not reset at vblank
		
		FMODE (0, 0);
		CLXCON (0);
		CLXCON2 (0);
		setup_fmodes (0);
		sprite_width = GET_SPRITEWIDTH (fmode);
		beamcon0 = new_beamcon0 = currprefs.ntscmode ? 0x00 : 0x20;
		bltstate = BLT_done;
		blit_interrupt = 1;
		init_sprites ();
	}
		
	devices_reset(hardreset);

  unset_special (~(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE));
	
	vpos = 0;
	vpos_count = vpos_count_diff = 0;
	
  inputdevice_reset();
  timehack_alive = 0;
	
	curr_sprite_entries[0].first_pixel = 0;
	curr_sprite_entries[1].first_pixel = 0;
	next_sprite_entry = 0;
	memset (spixels, 0, sizeof spixels);
	memset (&spixstate, 0, sizeof spixstate);
	toscr_delay_sh[0] = 0;
	toscr_delay_sh[1] = 0;
	
	cop_state.state = COP_stop;
	cop_state.movedelay = 0;
	cop_state.strobe = 0;
	cop_state.ignore_next = 0;
	diwstate = DIW_waiting_start;
	
	dmal = 0;
	new_beamcon0 = currprefs.ntscmode ? 0x00 : 0x20;
	time_per_frame = 1000 * 1000 / (currprefs.ntscmode ? 60 : 50);
	init_hz_normal();
	// init_hz sets vpos_count
	vpos_count = 0;
	lof_changing = 0;
	lof_togglecnt_nlace = lof_togglecnt_lace = 0;
	
	audio_reset ();
  if (!isrestore ()) {
	  /* must be called after audio_reset */
	  adkcon = 0;
	  audio_update_adkmasks ();
  }
	
	init_hardware_frame ();
  drawing_init ();
	
	reset_decisions ();
	
	vsync_rendered = false;
	frame_shown = false;
	frame_rendered = false;

	init_regtypes ();	

  if (isrestore ()) {
		uae_u16 v;
		uae_u32 vv;
		
		audio_update_adkmasks ();
		INTENA (0);
		INTREQ (0);
		
		if (diwhigh)
			diwhigh_written = 1;
		else
		  diwhigh_written = 0;
		COPJMP (1, 1);
		v = bplcon0;
		BPLCON0 (0, 0);
		BPLCON0 (0, v);
		FMODE (0, fmode);
		
		if (!(currprefs.chipset_mask & CSMASK_AGA)) {
			for(int i = 0 ; i < 32 ; i++)  {
				vv = current_colors.color_regs_ecs[i];
				current_colors.color_regs_ecs[i] = (unsigned short)-1;
				record_color_change (0, i, vv);
				remembered_color_entry = -1;
				current_colors.color_regs_ecs[i] = vv;
				current_colors.acolors[i] = xcolors[vv];
			}
		} else {
			for(int i = 0 ; i < 256 ; i++)  {
				vv = current_colors.color_regs_aga[i];
				current_colors.color_regs_aga[i] = -1;
				record_color_change (0, i, vv);
				remembered_color_entry = -1;
				current_colors.color_regs_aga[i] = vv;
				current_colors.acolors[i] = CONVERT_RGB(vv);
			}
		}
		CLXCON (clxcon);
		CLXCON2 (clxcon2);
		calcdiw ();
		for (int i = 0; i < 8; i++) {
			SPRxCTLPOS (i);
			nr_armed += spr[i].armed != 0;
		}
  	if (! currprefs.produce_sound) {
  	    eventtab[ev_audio].active = 0;
  	    events_schedule ();
  	}
	}
	sprres = expand_sprres (bplcon0, bplcon3);
	sprite_width = GET_SPRITEWIDTH (fmode);
	setup_fmodes (0);

#ifdef ACTION_REPLAY
	/* Doing this here ensures we can use the 'reset' command from within AR */
	action_replay_reset (hardreset, keyboardreset);
#endif

  if (hardreset)
  	rtc_hardreset();

#ifdef PICASSO96
  picasso_reset ();
#endif
}

static void gen_custom_tables (void)
{
	for (int i = 0; i < 256; i++) {
		sprtaba[i] = ((((i >> 7) & 1) << 0)
			| (((i >> 6) & 1) << 2)
			| (((i >> 5) & 1) << 4)
			| (((i >> 4) & 1) << 6)
			| (((i >> 3) & 1) << 8)
			| (((i >> 2) & 1) << 10)
			| (((i >> 1) & 1) << 12)
			| (((i >> 0) & 1) << 14));
	  sprtabb[i] = sprtaba[i] * 2;
		sprite_ab_merge[i] = (((i & 15) ? 1 : 0)
			| ((i & 240) ? 2 : 0));
	}
	for (int i = 0; i < 16; i++) {
		clxmask[i] = (((i & 1) ? 0xF : 0x3)
			| ((i & 2) ? 0xF0 : 0x30)
			| ((i & 4) ? 0xF00 : 0x300)
			| ((i & 8) ? 0xF000 : 0x3000));
		sprclx[i] = (((i & 0x3) == 0x3 ? 1 : 0)
			| ((i & 0x5) == 0x5 ? 2 : 0)
			| ((i & 0x9) == 0x9 ? 4 : 0)
			| ((i & 0x6) == 0x6 ? 8 : 0)
			| ((i & 0xA) == 0xA ? 16 : 0)
			| ((i & 0xC) == 0xC ? 32 : 0)) << 9;
	}
}

static int allocate_sprite_tables (void)
{
  if(curr_sprite_entries == 0)
    curr_sprite_entries = xmalloc (struct sprite_entry, MAX_SPR_PIXELS / 16);
  if(curr_color_changes == 0)
    curr_color_changes = xmalloc (struct color_change, MAX_REG_CHANGE);

  return 1;
}

int custom_init (void)
{
  if (!allocate_sprite_tables())
  	return 0;

#ifdef AUTOCONFIG
	if (uae_boot_rom_type) {
  	uaecptr pos;
    pos = here ();

    org (rtarea_base + 0xFFA0);
    calltrap (deftrap (timehack_helper));
    dw (RTS);

    org (pos);
  }
#endif

  gen_custom_tables ();
  build_blitfilltable ();

  next_sprite_entry = 0;
  drawing_init ();

  create_cycle_diagram_table ();

  return 1;
}

/* Custom chip memory bank */

static uae_u32 REGPARAM3 custom_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_bget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_lgeti (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_wgeti (uaecptr) REGPARAM;
static void REGPARAM3 custom_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 custom_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 custom_bput (uaecptr, uae_u32) REGPARAM;

addrbank custom_bank = {
  custom_lget, custom_wget, custom_bget,
  custom_lput, custom_wput, custom_bput,
	default_xlate, default_check, NULL, NULL, _T("Custom chipset"),
	custom_lgeti, custom_wgeti,
	ABFLAG_IO, S_READ, S_WRITE, NULL, 0x1ff, 0xdff000
};

static uae_u32 REGPARAM2 custom_wgeti (uaecptr addr)
{
    if (currprefs.cpu_model >= 68020)
	return dummy_wgeti(addr);
    return custom_wget(addr);
}
static uae_u32 REGPARAM2 custom_lgeti (uaecptr addr)
{
    if (currprefs.cpu_model >= 68020)
	return dummy_lgeti(addr);
    return custom_lget(addr);
}

STATIC_INLINE uae_u32 REGPARAM2 custom_wget_1 (int hpos, uaecptr addr, int noput)
{
  uae_u16 v;
	int missing;
	addr &= 0xfff;
  switch (addr & 0x1fe) {
    case 0x000: v = 0xffff; break; /* BPLDDAT */
	  case 0x002: v = DMACONR (hpos); break;
    case 0x004: v = VPOSR (); break;
    case 0x006: v = VHPOSR (); break;

    case 0x008: v = 0xffff; break;

    case 0x00A: v = JOY0DAT (); break;
    case 0x00C: v =  JOY1DAT (); break;
    case 0x00E: v =  CLXDAT (); break;
    case 0x010: v = ADKCONR (); break;

    case 0x012: v = POT0DAT (); break;
    case 0x014: v = POT1DAT (); break;
    case 0x016: v = POTGOR (); break;
    case 0x018: v = 0x3000 /* no data */; break;
	  case 0x01A: v = DSKBYTR (hpos); break;
    case 0x01C: v = INTENAR (); break;
    case 0x01E: v = INTREQR (); break;
	  case 0x07C:
		  v = DENISEID (&missing);
		  if (missing)
			  goto writeonly;
		  break;

    case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
    case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
    case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
    case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
    case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
    case 0x1BC: case 0x1BE:
		  if (!(aga_mode))
			  goto writeonly;
		  v = COLOR_READ ((addr & 0x3E) / 2);
		  break;

    default:
writeonly:
	    /* OCS/ECS:
	     * reading write-only register causes write with last value in chip
	     * bus (custom registers, chipram, slowram)
		   * and finally returns either all ones or something weird if DMA happens
		   * in next (or previous) cycle.. FIXME.
		   *
		   * OCS-only special case: DFF000 (BLTDDAT) will always return whatever was left in bus
		   *
	     * AGA:
		  * Can also return last CPU accessed value
		  * Remembers old last_custom_value1
	     */
	    v = last_custom_value1;
		  SET_LINE_CYCLEBASED;
    	if (!noput) {
			  int bmdma;
        uae_u16 l;

			  if (aga_mode) {
				  l = 0;
			  } else {
				  // last chip bus value (read or write) is written to register
          if(currprefs.cpu_compatible && currprefs.cpu_model == 68000) {
			      l =  regs.irc;
          } else {
            l = 0xffff;
				  }
        }
        decide_line (hpos);
			  decide_fetch_safe (hpos);
        custom_wput_1 (hpos, addr, l, 1);

			  // CPU gets back (OCS/ECS only):
			  // - if last cycle was DMA cycle: DMA cycle data
			  // - if last cycle was not DMA cycle: FFFF or some ANDed old data.
			  //
			  bmdma = is_bitplane_dma(hpos);
			  if (aga_mode) {
				  if (bmdma) {
					  v = last_custom_value1;
				  } else {
					  v = last_custom_value1 >> ((addr & 2) ? 0 : 16);
				  }
			  } else {
				  if (bmdma) {
					  v = last_custom_value1;
				  } else {
					  // refresh checked because refresh cycles do not always
					  // set last_custom_value1 for performance reasons.
					  v = 0xffff;
				  }
        }
	      return v;
		}
  }
  return v;
}

STATIC_INLINE uae_u32 custom_wget2 (uaecptr addr)
{
  uae_u32 v;
	int hpos = current_hpos ();

  sync_copper_with_cpu (hpos, 1, addr);
	v = custom_wget_1 (hpos, addr, 0);
#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_COMMON
	addr &= 0x1ff;
	ar_custom[addr + 0] = (uae_u8)(v >> 8);
	ar_custom[addr + 1] = (uae_u8)(v);
#endif
#endif
  return v;
}

static uae_u32 REGPARAM2 custom_wget (uaecptr addr)
{
  uae_u32 v;

	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 2, false, 0);
  if (addr & 1) {
    /* think about move.w $dff005,d0.. (68020+ only) */
    addr &= ~1;
    v = custom_wget2 (addr) << 8;
    v |= custom_wget2 (addr + 2) >> 8;
    return v & 0xFFFF;
  }
  return custom_wget2 (addr);
}

static uae_u32 REGPARAM2 custom_bget (uaecptr addr)
{
	uae_u32 v;
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 1, false, 0);
  v = custom_wget2 (addr & ~1);
  v >>= (addr & 1 ? 0 : 8);
  return v & 0xFF;
}

static uae_u32 REGPARAM2 custom_lget (uaecptr addr)
{
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 4, false, 0);
  return ((uae_u32)custom_wget (addr) << 16) | custom_wget (addr + 2);
}

static int REGPARAM2 custom_wput_1 (int hpos, uaecptr addr, uae_u32 value, int noget)
{
  addr &= 0x1FE;
  value &= 0xffff;
#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_COMMON
	ar_custom[addr+0]=(uae_u8)(value>>8);
	ar_custom[addr+1]=(uae_u8)(value);
#endif
#endif
  switch (addr) {
    case 0x00E: CLXDAT (); break;

    case 0x020: DSKPTH (value); break;
    case 0x022: DSKPTL (value); break;
    case 0x024: DSKLEN (value, hpos); break;
	  case 0x026: /* DSKDAT (value). Writing to DMA write registers won't do anything */; break;
  	case 0x028: break;
    case 0x02A: VPOSW (value); break;
    case 0x02C: VHPOSW (value); break;
    case 0x02E: COPCON (value); break;
    case 0x030: break;
    case 0x032: break;
    case 0x034: POTGO (value); break;
    case 0x036: JOYTEST (value); break;

    case 0x040: BLTCON0 (value); break;
    case 0x042: BLTCON1 (value); break;

    case 0x044: BLTAFWM (value); break;
    case 0x046: BLTALWM (value); break;

    case 0x048: BLTCPTH (value); break;
    case 0x04A: BLTCPTL (value); break;
    case 0x04C: BLTBPTH (value); break;
    case 0x04E: BLTBPTL (value); break;
    case 0x050: BLTAPTH (value); break;
    case 0x052: BLTAPTL (value); break;
    case 0x054: BLTDPTH (value); break;
    case 0x056: BLTDPTL (value); break;

	  case 0x058: BLTSIZE (hpos, value); break;
    case 0x05A: BLTCON0L (value); break;
    case 0x05C: BLTSIZV (value); break;
	  case 0x05E: BLTSIZH (hpos, value); break;

    case 0x064: BLTAMOD (value); break;
    case 0x062: BLTBMOD (value); break;
    case 0x060: BLTCMOD (value); break;
    case 0x066: BLTDMOD (value); break;

    case 0x070: BLTCDAT (value); break;
    case 0x072: BLTBDAT (value); break;
    case 0x074: BLTADAT (value); break;

    case 0x07E: DSKSYNC (hpos, value); break;

    case 0x080: COP1LCH (value); break;
    case 0x082: COP1LCL (value); break;
    case 0x084: COP2LCH (value); break;
    case 0x086: COP2LCL (value); break;

    case 0x088: COPJMP (1, 0); break;
    case 0x08A: COPJMP (2, 0); break;

    case 0x08E: DIWSTRT (hpos, value); break;
    case 0x090: DIWSTOP (hpos, value); break;
    case 0x092: DDFSTRT (hpos, value); break;
    case 0x094: DDFSTOP (hpos, value); break;

    case 0x096: DMACON (hpos, value); break;
    case 0x098: CLXCON (value); break;
    case 0x09A: INTENA (value); break;
    case 0x09C: INTREQ (value); break;
    case 0x09E: ADKCON (hpos, value); break;

    case 0x0A0: AUDxLCH (0, value); break;
    case 0x0A2: AUDxLCL (0, value); break;
    case 0x0A4: AUDxLEN (0, value); break;
    case 0x0A6: AUDxPER (0, value); break;
    case 0x0A8: AUDxVOL (0, value); break;
    case 0x0AA: AUDxDAT (0, value); break;

    case 0x0B0: AUDxLCH (1, value); break;
    case 0x0B2: AUDxLCL (1, value); break;
    case 0x0B4: AUDxLEN (1, value); break;
    case 0x0B6: AUDxPER (1, value); break;
    case 0x0B8: AUDxVOL (1, value); break;
    case 0x0BA: AUDxDAT (1, value); break;

    case 0x0C0: AUDxLCH (2, value); break;
    case 0x0C2: AUDxLCL (2, value); break;
    case 0x0C4: AUDxLEN (2, value); break;
    case 0x0C6: AUDxPER (2, value); break;
    case 0x0C8: AUDxVOL (2, value); break;
    case 0x0CA: AUDxDAT (2, value); break;

    case 0x0D0: AUDxLCH (3, value); break;
    case 0x0D2: AUDxLCL (3, value); break;
    case 0x0D4: AUDxLEN (3, value); break;
    case 0x0D6: AUDxPER (3, value); break;
    case 0x0D8: AUDxVOL (3, value); break;
    case 0x0DA: AUDxDAT (3, value); break;

    case 0x0E0: BPLxPTH (hpos, value, 0); break;
    case 0x0E2: BPLxPTL (hpos, value, 0); break;
    case 0x0E4: BPLxPTH (hpos, value, 1); break;
    case 0x0E6: BPLxPTL (hpos, value, 1); break;
    case 0x0E8: BPLxPTH (hpos, value, 2); break;
    case 0x0EA: BPLxPTL (hpos, value, 2); break;
    case 0x0EC: BPLxPTH (hpos, value, 3); break;
    case 0x0EE: BPLxPTL (hpos, value, 3); break;
    case 0x0F0: BPLxPTH (hpos, value, 4); break;
    case 0x0F2: BPLxPTL (hpos, value, 4); break;
    case 0x0F4: BPLxPTH (hpos, value, 5); break;
    case 0x0F6: BPLxPTL (hpos, value, 5); break;
    case 0x0F8: BPLxPTH (hpos, value, 6); break;
    case 0x0FA: BPLxPTL (hpos, value, 6); break;
    case 0x0FC: BPLxPTH (hpos, value, 7); break;
    case 0x0FE: BPLxPTL (hpos, value, 7); break;

    case 0x100: BPLCON0 (hpos, value); break;
    case 0x102: BPLCON1 (hpos, value); break;
    case 0x104: BPLCON2 (hpos, value); break;
    case 0x106: BPLCON3 (hpos, value); break;

    case 0x108: BPL1MOD (hpos, value); break;
    case 0x10A: BPL2MOD (hpos, value); break;
    case 0x10C: BPLCON4 (hpos, value); break;
    case 0x10E: CLXCON2 (value); break;

	  case 0x110: BPLxDAT (hpos, 0, value); break;
	  case 0x112: BPLxDAT (hpos, 1, value); break;
	  case 0x114: BPLxDAT (hpos, 2, value); break;
	  case 0x116: BPLxDAT (hpos, 3, value); break;
	  case 0x118: BPLxDAT (hpos, 4, value); break;
	  case 0x11A: BPLxDAT (hpos, 5, value); break;
	  case 0x11C: BPLxDAT (hpos, 6, value); break;
	  case 0x11E: BPLxDAT (hpos, 7, value); break;

    case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
    case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
    case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
    case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
    case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
    case 0x1BC: case 0x1BE:
   	  COLOR_WRITE (hpos, value & 0xFFF, (addr & 0x3E) / 2);
    	break;
    case 0x120: case 0x124: case 0x128: case 0x12C:
    case 0x130: case 0x134: case 0x138: case 0x13C:
   	  SPRxPTH (hpos, value, (addr - 0x120) / 4);
    	break;
    case 0x122: case 0x126: case 0x12A: case 0x12E:
    case 0x132: case 0x136: case 0x13A: case 0x13E:
    	SPRxPTL (hpos, value, (addr - 0x122) / 4);
    	break;
    case 0x140: case 0x148: case 0x150: case 0x158:
    case 0x160: case 0x168: case 0x170: case 0x178:
    	SPRxPOS (hpos, value, (addr - 0x140) / 8);
    	break;
    case 0x142: case 0x14A: case 0x152: case 0x15A:
    case 0x162: case 0x16A: case 0x172: case 0x17A:
    	SPRxCTL (hpos, value, (addr - 0x142) / 8);
    	break;
    case 0x144: case 0x14C: case 0x154: case 0x15C:
    case 0x164: case 0x16C: case 0x174: case 0x17C:
    	SPRxDATA (hpos, value, (addr - 0x144) / 8);
    	break;
    case 0x146: case 0x14E: case 0x156: case 0x15E:
    case 0x166: case 0x16E: case 0x176: case 0x17E:
    	SPRxDATB (hpos, value, (addr - 0x146) / 8);
    	break;

    case 0x1C0: if (htotal != value) { htotal = value & (MAXHPOS_ROWS - 1); varsync (); } break;
    case 0x1C2: if (hsstop != value) { hsstop = value & (MAXHPOS_ROWS - 1); varsync (); } break;
    case 0x1C4: if (hbstrt != value) { hbstrt = value & (MAXHPOS_ROWS - 1); varsync (); } break;
    case 0x1C6: if (hbstop != value) { hbstop = value & (MAXHPOS_ROWS - 1); varsync (); } break;
    case 0x1C8: if (vtotal != value) { vtotal = value & (MAXVPOS_LINES_ECS - 1); varsync (); } break;
    case 0x1CA: if (vsstop != value) { vsstop = value & (MAXVPOS_LINES_ECS - 1); varsync (); } break;
    case 0x1CC: if (vbstrt < value || vbstrt > (value & (MAXVPOS_LINES_ECS - 1)) + 1) { vbstrt = value & (MAXVPOS_LINES_ECS - 1); varsync (); } break;
    case 0x1CE: if (vbstop < value || vbstop > (value & (MAXVPOS_LINES_ECS - 1)) + 1) { vbstop = value & (MAXVPOS_LINES_ECS - 1); varsync (); } break;
    case 0x1DC: BEAMCON0 (value); break;
    case 0x1DE: if (hsstrt != value) { hsstrt = value & (MAXHPOS_ROWS - 1); varsync (); } break;
    case 0x1E0: if (vsstrt != value) { vsstrt = value & (MAXVPOS_LINES_ECS - 1); varsync (); } break;
    case 0x1E2: if (hcenter != value) { hcenter = value & (MAXHPOS_ROWS - 1); varsync (); } break;
    case 0x1E4: DIWHIGH (hpos, value); break;

    case 0x1FC: FMODE (hpos, value); break;
    case 0x1FE: FNULL (value); break;

      /* writing to read-only register causes read access */
    default: 
	    if (!noget) {
			  custom_wget_1 (hpos, addr, 1);
      }
	    return 1;
  }
  return 0;
}

static void REGPARAM2 custom_wput (uaecptr addr, uae_u32 value)
{
  int hpos = current_hpos ();

	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		return;
	}
  sync_copper_with_cpu (hpos, 1, addr);
  if (addr & 1) {
  	addr &= ~1;
  	custom_wput_1 (hpos, addr, (value >> 8) | (value & 0xff00), 0);
  	custom_wput_1 (hpos, addr + 2, (value << 8) | (value & 0x00ff), 0);
  	return;
  }
  custom_wput_1 (hpos, addr, value, 0);
}

static void REGPARAM2 custom_bput (uaecptr addr, uae_u32 value)
{
	uae_u16 rval;

	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		return;
	}
	if (aga_mode) {
	  if (addr & 1) {
		  rval = value & 0xff;
	  } else {
		  rval = (value << 8) | (value & 0xFF);
	  }
	} else {
		rval = (value << 8) | (value & 0xff);
	}

	if (currprefs.cs_bytecustomwritebug) {
		if (addr & 1)
      custom_wput (addr & ~1, rval);
		else
			custom_wput (addr, value << 8);
	} else {
		custom_wput (addr & ~1, rval);
	}
}

static void REGPARAM2 custom_lput(uaecptr addr, uae_u32 value)
{
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		return;
	}
  custom_wput (addr & 0xfffe, value >> 16);
  custom_wput ((addr + 2) & 0xfffe, (uae_u16)value);
}

#ifdef SAVESTATE

void custom_prepare_savestate (void)
{
  int i;

  for (i = 0; i < ev2_max; i++) {
  	if (eventtab2[i].active) {
	    eventtab2[i].active = 0;
	    eventtab2[i].handler(eventtab2[i].data);
  	}
  }
}

#define RB restore_u8 ()
#define SRB (uae_s8)restore_u8 ()
#define RBB restore_u8 () != 0
#define RW restore_u16 ()
#define RL restore_u32 ()

uae_u8 *restore_custom (uae_u8 *src)
{
  uae_u16 dsklen, dskbytr, ru16;
  int dskpt;
  int i;

  audio_reset ();

  changed_prefs.chipset_mask = currprefs.chipset_mask = RL & CSMASK_MASK;
  update_mirrors();
  blt_info.bltddat = RW;	/* 000 BLTDDAT */
  ru16 = RW;	    /* 002 DMACONR -> see also 096 */
  if((ru16 & 0x4000) == 0)
    bltstate = BLT_done;
  blt_info.blitzero = (ru16 & 0x2000 ? 1 : 0);
  RW;				/* 004 VPOSR */
  RW;				/* 006 VHPOSR */
  RW;		    /* 008 DSKDATR (dummy register) */
	JOYSET(0, RW);	/* 00A JOY0DAT */
	JOYSET(1, RW);	/* 00C JOY1DAT */
  clxdat = RW;		/* 00E CLXDAT */
  RW;				      /* 010 ADKCONR -> see 09E */
  RW;				      /* 012 POT0DAT */
  RW;				      /* 014 POT1DAT */
  RW;				      /* 016 POTINP -> see 034 */
  RW;				      /* 018 SERDATR* */
  dskbytr = RW;		/* 01A DSKBYTR */
  RW;				      /* 01C INTENAR -> see 09A */
  RW;				      /* 01E INTREQR -> see 09C */
  dskpt =  RL;		/* 020-022 DSKPT */
  dsklen = RW;		/* 024 DSKLEN */
  RW;				      /* 026 DSKDAT */
  RW;				      /* 028 REFPTR */
  i = RW; lof_store = lof_current = (i & 0x8000) ? 1 : 0;	/* 02A VPOSW */
  RW;				      /* 02C VHPOSW */
  COPCON(RW);	    /* 02E COPCON */
  RW;				      /* 030 SERDAT */
  RW;				      /* 032 SERPER* */
	potgo_value = 0; POTGO (RW); /* 034 POTGO */
  RW;				      /* 036 JOYTEST* */
  RW;				      /* 038 STREQU */
  RW;				      /* 03A STRVHBL */
  RW;				      /* 03C STRHOR */
  RW;				      /* 03E STRLONG */
  BLTCON0(RW);	  /* 040 BLTCON0 */
  BLTCON1(RW);	  /* 042 BLTCON1 */
  BLTAFWM(RW);	  /* 044 BLTAFWM */
  BLTALWM(RW);	  /* 046 BLTALWM */
  bltcpt = RL; 	  /* 048-04B BLTCPT */
  bltbpt = RL; 	  /* 04C-04F BLTBPT */
  bltapt = RL; 	  /* 050-053 BLTAPT */
  bltdpt = RL; 	  /* 054-057 BLTDPT */
  RW;				      /* 058 BLTSIZE */
  RW;				      /* 05A BLTCON0L -> see 040 */
  blt_info.vblitsize=RW;  /* 05C BLTSIZV */
  blt_info.hblitsize=RW;	/* 05E BLTSIZH */
  blt_info.bltcmod = RW;  /* 060 BLTCMOD */
  blt_info.bltbmod = RW;  /* 062 BLTBMOD */
  blt_info.bltamod = RW;  /* 064 BLTAMOD */
  blt_info.bltdmod = RW;  /* 066 BLTDMOD */
  RW;				/* 068 ? */
  RW;				/* 06A ? */
  RW;				/* 06C ? */
  RW;				/* 06E ? */
  blt_info.bltcdat =RW;   /* 070 BLTCDAT */
  BLTBDAT(RW);	  /* 072 BLTBDAT */
  blt_info.bltadat=RW;    /* 074 BLTADAT */
  RW;				/* 076 ? */
  RW;				/* 078 ? */
  RW;				/* 07A ? */
  RW;				/* 07C LISAID */
  ru16=RW; DSKSYNC(-1, ru16);	/* 07E DSKSYNC */
  cop1lc =  RL;		/* 080/082 COP1LC */
  cop2lc =  RL;		/* 084/086 COP2LC */
  RW;				      /* 088 COPJMP1 */
  RW;				      /* 08A COPJMP2 */
  RW;				      /* 08C COPINS */
  diwstrt = RW;		/* 08E DIWSTRT */
  diwstop = RW;		/* 090 DIWSTOP */
  ddfstrt = RW;		/* 092 DDFSTRT */
  ddfstop = RW;		/* 094 DDFSTOP */
  dmacon = RW & ~(0x2000|0x4000); /* 096 DMACON */
  CLXCON(RW);	    /* 098 CLXCON */
  intena = RW;		/* 09A INTENA */
  intreq = RW;	  /* 09C INTREQ */
  adkcon = RW;		/* 09E ADKCON */
  /* 0A0 - 0DE Audio regs */
  for (i = 0; i < 8; i++)
  	bplpt[i] = RL;
  bplcon0 = RW;		/* 100 BPLCON0 */
  bplcon1 = RW;		/* 102 BPLCON1 */
  bplcon2 = RW;		/* 104 BPLCON2 */
  bplcon3 = RW;		/* 106 BPLCON3 */
  bpl1mod = RW;		/* 108 BPL1MOD */
  bpl2mod = RW;		/* 10A BPL2MOD */
  bplcon4 = RW;		/* 10C BPLCON4 */
  CLXCON2(RW);	  /* 10E CLXCON2* */
  for(i = 0; i < 8; i++)
  	fetched[i] = RW;			/*     BPLXDAT */
  /* 120 - 17E Sprite regs */
  for(i = 0; i < 32; i++) {
		uae_u16 v = RW;
  	current_colors.color_regs_ecs[i] = v & 0xfff; /* 180-1BE COLORxx */
  }
  htotal = RW;		/* 1C0 HTOTAL */
  hsstop = RW;		/* 1C2 HSTOP ? */
  hbstrt = RW;		/* 1C4 HBSTRT ? */
  hbstop = RW;		/* 1C6 HBSTOP ? */
  vtotal = RW;		/* 1C8 VTOTAL */
  vsstop = RW;		/* 1CA VSSTOP */
  vbstrt = RW;		/* 1CC VBSTRT */
  vbstop = RW;		/* 1CE VBSTOP */
  RW;				/* 1D0 ? */
  RW;				/* 1D2 ? */
  RW;				/* 1D4 ? */
  RW;				/* 1D6 ? */
  RW;				/* 1D8 ? */
  RW;				/* 1DA ? */
  new_beamcon0 = RW;		/* 1DC BEAMCON0 */
  hsstrt = RW;		/* 1DE HSSTRT */
  vsstrt = RW;		/* 1E0 VSSTT */
  hcenter = RW;		/* 1E2 HCENTER */
  diwhigh = RW;		/* 1E4 DIWHIGH */
  diwhigh_written = (diwhigh & 0x8000) ? 1 : 0;
  hdiwstate = (diwhigh & 0x4000) ? DIW_waiting_stop : DIW_waiting_start;
  diwhigh &= 0x3fff;
  RW;				/* 1E6 ? */
  RW;				/* 1E8 ? */
  RW;				/* 1EA ? */
  RW;				/* 1EC ? */
  RW;				/* 1EE ? */
  RW;				/* 1F0 ? */
  RW;				/* 1F2 ? */
  RW;				/* 1F4 ? */
  RW;				/* 1F6 ? */
  RW;				/* 1F8 ? */
  i = RW;					/* 1FA ? */
  if (i & 0x8000)
	  currprefs.ntscmode = changed_prefs.ntscmode = i & 1;
	fmode = fmode_saved = RW; /* 1FC FMODE */
  last_custom_value1 = RW;	/* 1FE ? */

	current_colors.extra = 0;
	if (isbrdblank (-1, bplcon0, bplcon3))
		current_colors.extra |= 1 << CE_BORDERBLANK;
	if (issprbrd(-1, bplcon0, bplcon3))
		current_colors.extra |= 1 << CE_BORDERSPRITE;

  DISK_restore_custom (dskpt, dsklen, dskbytr);

	FMODE (0, fmode);

  return src;
}

#define SB save_u8
#define SW save_u16
#define SL save_u32

uae_u8 *save_custom (int *len, uae_u8 *dstptr, int full)
{
  uae_u8 *dstbak, *dst;
	int i, dummy;
  uae_u32 dskpt;
  uae_u16 dsklen, dsksync, dskbytr;

  DISK_save_custom (&dskpt, &dsklen, &dsksync, &dskbytr);

  if (dstptr)
  	dstbak = dst = dstptr;
  else
    dstbak = dst = xmalloc (uae_u8, 8 + 256 * 2);

  SL (currprefs.chipset_mask);
  SW (blt_info.bltddat);	/* 000 BLTDDAT */
  SW (dmacon);	      /* 002 DMACONR */
  SW (VPOSR());		    /* 004 VPOSR */
  SW (VHPOSR());		  /* 006 VHPOSR */
  SW (0);		          /* 008 DSKDATR */
  SW (JOYGET (0));		/* 00A JOY0DAT */
  SW (JOYGET (1));		/* 00C JOY1DAT */
  SW (clxdat | 0x8000);		/* 00E CLXDAT */
  SW (ADKCONR());		  /* 010 ADKCONR */
  SW (POT0DAT());		  /* 012 POT0DAT */
  SW (POT1DAT());		  /* 014 POT1DAT */
  SW (0)	;		        /* 016 POTINP * */
  SW (0);			        /* 018 SERDATR * */
  SW (dskbytr);		    /* 01A DSKBYTR */
  SW (INTENAR());		  /* 01C INTENAR */
  SW (INTREQR());		  /* 01E INTREQR */
  SL (dskpt);			    /* 020-023 DSKPT */
  SW (dsklen);		    /* 024 DSKLEN */
  SW (0);			        /* 026 DSKDAT */
  SW (0);			        /* 028 REFPTR */
  SW ((lof_store ? 0x8001 : 0));	/* 02A VPOSW */
  SW (0);			        /* 02C VHPOSW */
  SW (copcon);		    /* 02E COPCON */
  SW (0);		          /* 030 SERDAT * */
  SW (0);		          /* 032 SERPER * */
  SW (potgo_value);		/* 034 POTGO */
  SW (0);			        /* 036 JOYTEST * */
  SW (0);			        /* 038 STREQU */
  SW (0);			        /* 03A STRVBL */
  SW (0);			        /* 03C STRHOR */
  SW (0);			        /* 03E STRLONG */
  SW (bltcon0);		    /* 040 BLTCON0 */
  SW (bltcon1);		    /* 042 BLTCON1 */
  SW (blt_info.bltafwm);	/* 044 BLTAFWM */
  SW (blt_info.bltalwm);	/* 046 BLTALWM */
  SL (bltcpt);		    /* 048-04B BLTCPT */
  SL (bltbpt);		    /* 04C-04F BLTBPT */
  SL (bltapt);		    /* 050-053 BLTAPT */
  SL (bltdpt);		    /* 054-057 BLTDPT */
  SW (0);			        /* 058 BLTSIZE */
  SW (0);			        /* 05A BLTCON0L (use BLTCON0 instead) */
  SW (blt_info.vblitsize);	/* 05C BLTSIZV */
  SW (blt_info.hblitsize);	/* 05E BLTSIZH */
  SW (blt_info.bltcmod);	  /* 060 BLTCMOD */
  SW (blt_info.bltbmod);	  /* 062 BLTBMOD */
  SW (blt_info.bltamod);	  /* 064 BLTAMOD */
  SW (blt_info.bltdmod);	  /* 066 BLTDMOD */
  SW (0);			        /* 068 ? */
  SW (0);			        /* 06A ? */
  SW (0);			        /* 06C ? */
  SW (0);			        /* 06E ? */
  SW (blt_info.bltcdat);	  /* 070 BLTCDAT */
  SW (blt_info.bltbdat);	  /* 072 BLTBDAT */
  SW (blt_info.bltadat);	  /* 074 BLTADAT */
  SW (0);			        /* 076 ? */
  SW (0);			        /* 078 ? */
  SW (0);			        /* 07A ? */
	SW (DENISEID (&dummy));	  /* 07C DENISEID/LISAID */
  SW (dsksync);		    /* 07E DSKSYNC */
  SL (cop1lc);		    /* 080-083 COP1LC */
  SL (cop2lc);		    /* 084-087 COP2LC */
  SW (0);			        /* 088 COPJMP1 */
  SW (0);			        /* 08A COPJMP2 */
  SW (0);			        /* 08C COPINS */
  SW (diwstrt);		    /* 08E DIWSTRT */
  SW (diwstop);		    /* 090 DIWSTOP */
  SW (ddfstrt);		    /* 092 DDFSTRT */
  SW (ddfstop);		    /* 094 DDFSTOP */
  SW (dmacon);		    /* 096 DMACON */
  SW (clxcon);		    /* 098 CLXCON */
  SW (intena);		    /* 09A INTENA */
  SW (intreq);		    /* 09C INTREQ */
  SW (adkcon);		    /* 09E ADKCON */
  /* 0A0 - 0DE Audio regs */
  for (i = 0; full && i < 32; i++)
    SW (0);
  for (i = 0; i < 8; i++)
    SL (bplpt[i]);		/* 0E0-0FE BPLxPT */
  SW (bplcon0);		/* 100 BPLCON0 */
  SW (bplcon1);		/* 102 BPLCON1 */
  SW (bplcon2);		/* 104 BPLCON2 */
  SW (bplcon3);		/* 106 BPLCON3 */
  SW (bpl1mod);		/* 108 BPL1MOD */
  SW (bpl2mod);		/* 10A BPL2MOD */
  SW (bplcon4);		/* 10C BPLCON4 */
  SW (clxcon2);		/* 10E CLXCON2 */
  for (i = 0;i < 8; i++)
    SW (fetched[i]);			/* 110 BPLxDAT */
  /* 120 - 17E Sprite regs */
  if (full) {
    for (i = 0; i < 8; i++) {
      SL (spr[i].pt);	    /* 120-13E SPRxPT */
		}
		for (i = 0; i < 8; i++) {
      SW (sprpos[i]);	    /* 1x0 SPRxPOS */
      SW (sprctl[i]);	    /* 1x2 SPRxPOS */
      SW (sprdata[i][0]);	    /* 1x4 SPRxDATA */
      SW (sprdatb[i][0]);	    /* 1x6 SPRxDATB */
    }
  }
  for ( i = 0; i < 32; i++)
	  SW (current_colors.color_regs_ecs[i]); /* 180-1BE COLORxx */
  SW (htotal);		/* 1C0 HTOTAL */
  SW (hsstop);		/* 1C2 HSTOP */
  SW (hbstrt);		/* 1C4 HBSTRT */
  SW (hbstop);		/* 1C6 HBSTOP */
  SW (vtotal);		/* 1C8 VTOTAL */
  SW (vsstop);		/* 1CA VSSTOP */
  SW (vbstrt);		/* 1CC VBSTRT */
  SW (vbstop);		/* 1CE VBSTOP */
  SW (0);			/* 1D0 */
  SW (0);			/* 1D2 */
  SW (0);			/* 1D4 */
  SW (0);			/* 1D6 */
  SW (0);			/* 1D8 */
  SW (0);			/* 1DA */
  SW (beamcon0);	/* 1DC BEAMCON0 */
  SW (hsstrt);		/* 1DE HSSTRT */
  SW (vsstrt);		/* 1E0 VSSTRT */
  SW (hcenter);		/* 1E2 HCENTER */
  SW (diwhigh | (diwhigh_written ? 0x8000 : 0) | (hdiwstate == DIW_waiting_stop ? 0x4000 : 0)); /* 1E4 DIWHIGH */
  SW (0);			/* 1E6 */
  SW (0);			/* 1E8 */
  SW (0);			/* 1EA */
  SW (0);			/* 1EC */
  SW (0);			/* 1EE */
  SW (0);			/* 1F0 */
  SW (0);			/* 1F2 */
  SW (0);			/* 1F4 */
  SW (0);			/* 1F6 */
  SW (0);			/* 1F8 */
  SW (0x8000 | (currprefs.ntscmode ? 1 : 0));			/* 1FA (re-used for NTSC) */
  SW (fmode);			/* 1FC FMODE */
  SW (last_custom_value1);		/* 1FE */

  *len = dst - dstbak;
  return dstbak;
}

uae_u8 *restore_custom_agacolors (uae_u8 *src)
{
  int i;

  for (i = 0; i < 256; i++) {
		uae_u32 v = RL;
		v &= 0x00ffffff;
  	current_colors.color_regs_aga[i] = v;
  }
  return src;
}

uae_u8 *save_custom_agacolors (int *len, uae_u8 *dstptr)
{
  uae_u8 *dstbak, *dst;

  if (dstptr)
  	dstbak = dst = dstptr;
  else
    dstbak = dst = xmalloc (uae_u8, 256 * 4);
  for (int i = 0; i < 256; i++)
  	SL (current_colors.color_regs_aga[i]);
  *len = dst - dstbak;
  return dstbak;
}

uae_u8 *restore_custom_sprite (int num, uae_u8 *src)
{
  memset (&spr[num], 0, sizeof (struct sprite));
  spr[num].pt = RL;		/* 120-13E SPRxPT */
  sprpos[num] = RW;		/* 1x0 SPRxPOS */
  sprctl[num] = RW;		/* 1x2 SPRxPOS */
  sprdata[num][0] = RW;	/* 1x4 SPRxDATA */
  sprdatb[num][0] = RW;	/* 1x6 SPRxDATB */
  sprdata[num][1] = RW;
  sprdatb[num][1] = RW;
  sprdata[num][2] = RW;
  sprdatb[num][2] = RW;
  sprdata[num][3] = RW;
  sprdatb[num][3] = RW;
	spr[num].armed = RB & 1;
  return src;
}

uae_u8 *save_custom_sprite(int num, int *len, uae_u8 *dstptr)
{
  uae_u8 *dstbak, *dst;

  if (dstptr)
  	dstbak = dst = dstptr;
  else
    dstbak = dst = xmalloc (uae_u8, 30);
  SL (spr[num].pt);		/* 120-13E SPRxPT */
  SW (sprpos[num]);		/* 1x0 SPRxPOS */
  SW (sprctl[num]);		/* 1x2 SPRxPOS */
  SW (sprdata[num][0]);	/* 1x4 SPRxDATA */
  SW (sprdatb[num][0]);	/* 1x6 SPRxDATB */
  SW (sprdata[num][1]);
  SW (sprdatb[num][1]);
  SW (sprdata[num][2]);
  SW (sprdatb[num][2]);
  SW (sprdata[num][3]);
  SW (sprdatb[num][3]);
  SB (spr[num].armed ? 1 : 0);
  *len = dst - dstbak;
  return dstbak;
}

uae_u8 *restore_custom_extra (uae_u8 *src)
{
	uae_u32 v = restore_u32 ();

	if (!(v & 1))
		v = 0;
	currprefs.cs_compatible = changed_prefs.cs_compatible = v >> 24;
	cia_set_overlay ((v & 2) != 0);

	currprefs.cs_rtc = changed_prefs.cs_rtc = RB;

	currprefs.cs_pcmcia = changed_prefs.cs_pcmcia = RBB;
	currprefs.cs_ciaatod = changed_prefs.cs_ciaatod = RB;
	currprefs.cs_ciaoverlay = changed_prefs.cs_ciaoverlay = RBB;

	currprefs.cs_fatgaryrev = changed_prefs.cs_fatgaryrev = SRB;
	currprefs.cs_ramseyrev = changed_prefs.cs_ramseyrev = SRB;

	currprefs.cs_cd32c2p = changed_prefs.cs_cd32c2p = RBB;
	currprefs.cs_cd32cd = changed_prefs.cs_cd32cd = RBB;
	currprefs.cs_cd32nvram = changed_prefs.cs_cd32nvram = RBB;

	currprefs.cs_df0idhw = changed_prefs.cs_df0idhw = RBB;
	currprefs.cs_ide = changed_prefs.cs_ide = RB;
	currprefs.cs_ksmirror_a8 = changed_prefs.cs_ksmirror_a8 = RBB;
	currprefs.cs_ksmirror_e0 = changed_prefs.cs_ksmirror_e0 = RBB;
	currprefs.cs_z3autoconfig = changed_prefs.cs_z3autoconfig = RBB;

	currprefs.cs_ciatype[0] = changed_prefs.cs_ciatype[0] = RBB;
	currprefs.cs_ciatype[1] = changed_prefs.cs_ciatype[1] = RBB;

	return src;
}

uae_u8 *save_custom_extra (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);

	SL ((currprefs.cs_compatible << 24) | (&get_mem_bank (0) != &chipmem_bank ? 2 : 0) | 1);
	SB (currprefs.cs_rtc);

	SB (currprefs.cs_pcmcia ? 1 : 0);
	SB (currprefs.cs_ciaatod);
	SB (currprefs.cs_ciaoverlay ? 1 : 0);

	SB (currprefs.cs_fatgaryrev);
	SB (currprefs.cs_ramseyrev);

	SB (currprefs.cs_cd32c2p);
	SB (currprefs.cs_cd32cd);
	SB (currprefs.cs_cd32nvram);

	SB (currprefs.cs_df0idhw ? 1 : 0);
	SB (currprefs.cs_ide);
	SB (currprefs.cs_ksmirror_a8 ? 1 : 0);
	SB (currprefs.cs_ksmirror_e0 ? 1 : 0);
	SB (currprefs.cs_z3autoconfig ? 1 : 0);
	SB(currprefs.cs_ciatype[0]);
	SB(currprefs.cs_ciatype[1]);

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE */

void check_prefs_changed_custom (void)
{
	if (!config_changed)
		return;
  currprefs.gfx_framerate = changed_prefs.gfx_framerate;
  if (inputdevice_config_change_test ())
  	inputdevice_copyconfig (&changed_prefs, &currprefs);
  currprefs.immediate_blits = changed_prefs.immediate_blits;
	currprefs.waiting_blits = changed_prefs.waiting_blits;
  currprefs.collision_level = changed_prefs.collision_level;
  currprefs.fast_copper = changed_prefs.fast_copper;

  currprefs.cs_ciaatod = changed_prefs.cs_ciaatod;
	currprefs.cs_rtc = changed_prefs.cs_rtc;
	currprefs.cs_cd32cd = changed_prefs.cs_cd32cd;
	currprefs.cs_cd32c2p = changed_prefs.cs_cd32c2p;
	currprefs.cs_cd32nvram = changed_prefs.cs_cd32nvram;
	currprefs.cs_ide = changed_prefs.cs_ide;
	currprefs.cs_pcmcia = changed_prefs.cs_pcmcia;
	currprefs.cs_fatgaryrev = changed_prefs.cs_fatgaryrev;
	currprefs.cs_ramseyrev = changed_prefs.cs_ramseyrev;
	currprefs.cs_df0idhw = changed_prefs.cs_df0idhw;
	currprefs.cs_z3autoconfig = changed_prefs.cs_z3autoconfig;
	currprefs.cs_unmapped_space = changed_prefs.cs_unmapped_space;
	currprefs.cs_ciatype[0] = changed_prefs.cs_ciatype[0];
	currprefs.cs_ciatype[1] = changed_prefs.cs_ciatype[1];
  
  if (currprefs.chipset_mask != changed_prefs.chipset_mask ||
	  currprefs.ntscmode != changed_prefs.ntscmode) {
	    currprefs.chipset_mask = changed_prefs.chipset_mask;
      if (currprefs.ntscmode != changed_prefs.ntscmode) {
	      currprefs.ntscmode = changed_prefs.ntscmode;
	      new_beamcon0 = currprefs.ntscmode ? 0x00 : 0x20;
      }
    	init_custom ();
  }
}
