 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Custom chip emulation
  *
  * Copyright 1995-2002 Bernd Schmidt
  * Copyright 1995 Alessandro Bissacco
  * Copyright 2000-2008 Toni Wilen
  */

#define SPR0_HPOS 0x15
#define MAX_SPRITES 8

#include "sysconfig.h"
#include "sysdeps.h"
#include <ctype.h>
#include <assert.h>
#include "options.h"
#include "uae.h"
#include "gensound.h"
#include "audio.h"
#include "sd-pandora/sound.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cia.h"
#include "disk.h"
#include "blitter.h"
#include "xwin.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "serial.h"
#include "osemu.h"
#include "autoconf.h"
#include "traps.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "savestate.h"
#include "filesys.h"
#include <SDL.h>

STATIC_INLINE int nocustom(void)
{
    if (picasso_on && currprefs.picasso96_nocustom)
	return 1;
    return 0;
}

static uae_u16 last_custom_value;

/* Mouse and joystick emulation */
int buttonstate[3];
int joy0button, joy1button;
unsigned int joy0dir, joy1dir;


/* Events */
unsigned long int nextevent, is_lastline, currcycle;
unsigned long last_synctime = 0;
static int frh_count = 0;
#define LAST_SPEEDUP_LINE 30
int speedup_cycles = 3000 * CYCLE_UNIT;
int speedup_timelimit = -1000;

static int rpt_did_reset;
struct ev eventtab[ev_max];
struct ev2 eventtab2[ev2_max];

volatile frame_time_t vsynctime, vsyncmintime;

#ifdef JIT
extern uae_u8* compiled_code;
#endif

int vpos;
int hack_vpos;
static uae_u16 lof;
static int next_lineno;
static int lof_changed = 0;

static uae_u32 sprtaba[256],sprtabb[256];
static uae_u32 sprite_ab_merge[256];

/* Tables for collision detection.  */
static uae_u32 sprclx[16], clxmask[16];

/*
 * Hardware registers of all sorts.
 */
static int REGPARAM3 custom_wput_1 (int, uaecptr, uae_u32, int) REGPARAM;

uae_u16 intena, intreq, intreqr;
uae_u16 dmacon;
uae_u16 adkcon; /* used by audio code */

static uae_u32 cop1lc,cop2lc,copcon;
 
int maxhpos = MAXHPOS_PAL;
int maxvpos = MAXVPOS_PAL;
int minfirstline = VBLANK_ENDLINE_PAL;
int vblank_hz = VBLANK_HZ_PAL;
frame_time_t syncbase;
static int fmode;
unsigned int beamcon0, new_beamcon0;
uae_u16 vtotal = MAXVPOS_PAL, htotal = MAXHPOS_PAL;
static uae_u16 hsstop, hbstrt, hbstop, vsstop, vbstrt, vbstop, hsstrt, vsstrt, hcenter;

#define HSYNCTIME (maxhpos * CYCLE_UNIT);

/* This is but an educated guess. It seems to be correct, but this stuff
 * isn't documented well. */
struct sprite {
    uaecptr pt;
    int xpos;
    int vstart;
    int vstop;
    int armed;
    int dmastate;
    int dmacycle;
};

static struct sprite spr[MAX_SPRITES];

static int sprite_vblank_endline = VBLANK_SPRITE_PAL;

static unsigned int sprctl[MAX_SPRITES], sprpos[MAX_SPRITES];
static uae_u16 sprdata[MAX_SPRITES][4], sprdatb[MAX_SPRITES][4];
static int sprite_last_drawn_at[MAX_SPRITES];

static int last_sprite_point, nr_armed;
static int sprite_width, sprres;
int sprite_buffer_res;

static uae_u32 bpl1dat;
static uae_s16 bpl1mod, bpl2mod;

static uaecptr bplpt[8];

static struct color_entry current_colors;
static unsigned int bplcon0, bplcon1, bplcon2, bplcon3, bplcon4;
static unsigned int planes_bplcon0, res_bplcon0, planes_limit_bplcon0;
static unsigned int diwstrt, diwstop, diwhigh;
static int diwhigh_written;
static unsigned int ddfstrt, ddfstop, ddfstrt_old_hpos, ddfstrt_old_vpos;
static int ddf_change;

/* The display and data fetch windows */
enum diw_states
{
    DIW_waiting_start, DIW_waiting_stop
};

static int plffirstline, plflastline;
static int plfstrt, plfstop;
static int last_diw_pix_hpos;
static int last_decide_line_hpos, last_sprite_decide_line_hpos;
static int last_fetch_hpos, last_sprite_hpos;
int diwfirstword, diwlastword;
static enum diw_states diwstate, hdiwstate, ddfstate;

/* Sprite collisions */
static unsigned int clxdat, clxcon, clxcon2, clxcon_bpl_enable, clxcon_bpl_match;

enum copper_states {
    COP_stop,
    COP_read1_in2,
    COP_read1_wr_in4,
    COP_read1_wr_in2,
    COP_read1,
    COP_read2_wr_in2,
    COP_read2,
    COP_bltwait,
    COP_wait_in4,
    COP_wait_in2,
    COP_skip_in4,
    COP_skip_in2,
    COP_wait1,
    COP_wait,
    COP_skip1,
    COP_strobe_delay
};

struct copper {
    /* The current instruction words.  */
    unsigned int i1, i2;
    unsigned int saved_i1, saved_i2;
    enum copper_states state, state_prev;
    /* Instruction pointer.  */
    uaecptr ip, saved_ip;
    int hpos, vpos;
    unsigned int ignore_next;
    int vcmp, hcmp;

#ifdef FAST_COPPER
    /* When we schedule a copper event, knowing a few things about the future
       of the copper list can reduce the number of sync_with_cpu calls
       dramatically.  */
    unsigned int first_sync;
    unsigned int regtypes_modified;
#endif
    int strobe; /* COPJMP1 / COPJMP2 accessed */
};

#ifdef FAST_COPPER
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
#endif

static struct copper cop_state;
static int copper_enabled_thisline;

/*
 * Statistics
 */

static unsigned long int lastframetime = 0, timeframes = 0;
unsigned long hsync_counter = 0, vsync_counter = 0, ciavsync_counter = 0;

/* Recording of custom chip register changes.  */
struct sprite_entry *curr_sprite_entries;
struct color_change *curr_color_changes;
#define MAX_SPRITE_ENTRY (1024*6)
#define MAX_COLOR_CHANGE (1024*10)

struct decision line_decisions[2 * (MAXVPOS + 1) + 1];
struct draw_info curr_drawinfo[2 * (MAXVPOS + 1) + 1];
struct color_entry curr_color_tables[(MAXVPOS + 1) * 2];

static int next_sprite_entry = 0;
static int next_sprite_forced = 1;

static int next_color_change;
static int next_color_entry, remembered_color_entry;

static struct decision thisline_decision;
static int passed_plfstop, fetch_cycle, fetch_modulo_cycle;

enum fetchstate {
    fetch_not_started,
    fetch_started,
    fetch_was_plane0
} fetch_state;

/*
 * helper functions
 */

#define nodraw() (framecnt != 0)

uae_u32 get_copper_address (int copno)
{
    switch (copno) {
    case 1: return cop1lc;
    case 2: return cop2lc;
    case -1: return cop_state.ip;
    default: return 0;
    }
}

void reset_frame_rate_hack (void)
{
    if (currprefs.m68k_speed != -1)
	return;

    rpt_did_reset = 1;
    is_lastline = 0;
    vsyncmintime = read_processor_time() + vsynctime;
}

STATIC_INLINE void setclr (uae_u16 *_GCCRES_ p, uae_u16 val)
{
    if (val & 0x8000)
	*p |= val & 0x7FFF;
    else
	*p &= ~val;
}

STATIC_INLINE void alloc_cycle(int hpos, int type)
{
#ifdef CPUEMU_12
#if 0
    if (cycle_line[hpos])
	write_log ("hpos=%d, old=%d, new=%d\n", hpos, cycle_line[hpos], type);
    if ((type == CYCLE_CPU || type == CYCLE_COPPER) && (hpos & 1))
	write_log ("odd %d cycle %d\n", hpos);
    if (!(hpos & 1) && (type == CYCLE_SPRITE || type == CYCLE_REFRESH || type == CYCLE_MISC))
	write_log ("even %d cycle %d\n", type, hpos);
#endif
    cycle_line[hpos] = type;
#endif
}

void alloc_cycle_ext(int hpos, int type)
{
    alloc_cycle (hpos, type);
}

static void update_mirrors(void)
{
    aga_mode = (currprefs.chipset_mask & CSMASK_AGA) ? 1 : 0;
    direct_rgb = aga_mode;
}

STATIC_INLINE uae_u8 *_GCCRES_ pfield_xlateptr (uaecptr plpt, int bytecount)
{
  plpt &= chipmem_mask;
  if((plpt + bytecount) > allocated_chipmem)
    return NULL;
  return chipmemory + plpt;
}

STATIC_INLINE void docols (struct color_entry *_GCCRES_ colentry)
{
	int i;
	
	if (currprefs.chipset_mask & CSMASK_AGA) {
		for (i = 0; i < 256; i++) {
			int v = color_reg_get (colentry, i);
	    if (v < 0 || v > 16777215)
				continue;
			colentry->acolors[i] = CONVERT_RGB (v);
		}
	} else {
		for (i = 0; i < 32; i++) {
			int v = color_reg_get (colentry, i);
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
	if (remembered_color_entry == -1) {
		/* The colors changed since we last recorded a color map. Record a
		 * new one. */
		color_reg_cpy (curr_color_tables + next_color_entry, &current_colors);
		remembered_color_entry = next_color_entry++;
	}
	thisline_decision.ctable = remembered_color_entry;
}

/* Called to determine the state of the horizontal display window state
 * machine at the current position. It might have changed since we last
 * checked.  */
static void decide_diw (int hpos)
{
     /* Last hpos = hpos + 0.5, eg. normal PAL end hpos is 227.5 * 2 = 455 */
    int pix_hpos = coord_diw_to_window_x (hpos == maxhpos ? hpos * 2 + 1 : hpos * 2);
	if (hdiwstate == DIW_waiting_start && thisline_decision.diwfirstword == -1
	&& pix_hpos >= diwfirstword && last_diw_pix_hpos < diwfirstword)
		{
			thisline_decision.diwfirstword = diwfirstword < 0 ? 0 : diwfirstword;
		hdiwstate = DIW_waiting_stop;
	}
	if (hdiwstate == DIW_waiting_stop&& thisline_decision.diwlastword == -1
	&& pix_hpos >= diwlastword && last_diw_pix_hpos < diwlastword)
	{
			thisline_decision.diwlastword = diwlastword < 0 ? 0 : diwlastword;
		hdiwstate = DIW_waiting_start;
	}
	last_diw_pix_hpos = pix_hpos;
}

static int fetchmode;
static int delaymask;
static int real_bitplane_number[3][3][9];

/* Disable bitplane DMA if planes > available DMA slots. This is needed
   e.g. by the Sanity WOC demo (at the "Party Effect").  */
//STATIC_INLINE int GET_PLANES_LIMIT (uae_u16 bc0)
//{
//    int res = GET_RES(bc0);
//    int planes = GET_PLANES(bc0);
//    return real_bitplane_number[fetchmode][res][planes];
//}
#define GET_PLANES_LIMIT() real_bitplane_number[fetchmode][res_bplcon0][planes_bplcon0]

/* The HRM says 0xD8, but that can't work... */
#define HARD_DDF_STOP 0xd4
#define HARD_DDF_START 0x18

static __inline__ void add_modulos(void)
{
  int m1, m2;
 
  if (fmode & 0x4000) {
    if (((diwstrt >> 8) ^ vpos) & 1)
      m1 = m2 = bpl2mod;
    else
      m1 = m2 = bpl1mod;
  } else {
    m1 = bpl1mod;
    m2 = bpl2mod;
  }
 
  switch (planes_limit_bplcon0) {
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

static __inline__ void finish_playfield_line(void)
{
    /* These are for comparison. */
    thisline_decision.bplcon0 = bplcon0;
    thisline_decision.bplcon2 = bplcon2;
    thisline_decision.bplcon3 = bplcon3;
    thisline_decision.bplcon4 = bplcon4;
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
        		if (rplanes == 7 && fm == 0 && res == 0 && !(currprefs.chipset_mask & CSMASK_AGA))
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
	if (! passed_plfstop) {
	int stop = plfstop < hpos || plfstop > HARD_DDF_STOP ? HARD_DDF_STOP : plfstop;
		/* We know that fetching is up-to-date up until hpos, so we can use fetch_cycle.  */
		int fetch_cycle_at_stop = fetch_cycle + (stop - hpos);
		int starting_last_block_at = (fetch_cycle_at_stop + fetchunit - 1) & ~fetchunit_mask;
		
		estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + fetchunit;
	} else {
		int starting_last_block_at = (fetch_cycle + fetchunit - 1) & ~fetchunit_mask;
		if (passed_plfstop == 2)
			starting_last_block_at -= fetchunit;
		
		estimated_last_fetch_cycle = hpos + (starting_last_block_at - fetch_cycle) + fetchunit;
	}
}

static uae_u32 outword[MAX_PLANES];
static int out_nbits, out_offs;
static uae_u32 todisplay[MAX_PLANES][4];
static uae_u32 fetched[MAX_PLANES];
static uae_u32 fetched_aga0[MAX_PLANES];
static uae_u32 fetched_aga1[MAX_PLANES];

/* Expansions from bplcon0/bplcon1.  */
static int toscr_res, toscr_nr_planes;
static int toscr_res2;
static int toscr_delay[2];
static int toscr_delay1x, toscr_delay2x;

/* The number of bits left from the last fetched words.  
   This is an optimization - conceptually, we have to make sure the result is
   the same as if toscr is called in each clock cycle.  However, to speed this
   up, we accumulate display data; this variable keeps track of how much. 
   Thus, once we do call toscr_nbits (which happens at least every 16 bits),
   we can do more work at once.  */
static int toscr_nbits;

/* undocumented bitplane delay hardware feature */
static int delayoffset;

STATIC_INLINE void compute_delay_offset (void)
{
  delayoffset = (16 << fetchmode) - (((plfstrt - HARD_DDF_START) & fetchstart_mask) << 1);
}

static void expand_fmodes (void)
{
//    int res = res_bplcon0;
    int fm_index = fetchmode * 4 + res_bplcon0;
    fetchunit = fetchunits[fm_index];
    fetchunit_mask = fetchunit - 1;
    int fetchstart_shift = fetchstarts[fm_index];
    fetchstart = 1 << fetchstart_shift;
    fetchstart_mask = fetchstart - 1;
    int fm_maxplane_shift = fm_maxplanes[fm_index];
    fm_maxplane = 1 << fm_maxplane_shift;
    fetch_modulo_cycle = fetchunit - fetchstart;
}

/* Expand bplcon0/bplcon1 into the toscr_xxx variables.  */
static __inline__ void compute_toscr_delay_1 (void)
{
    int delay1 = (bplcon1 & 0x0f) | ((bplcon1 & 0x0c00) >> 6);
    int delay2 = ((bplcon1 >> 4) & 0x0f) | (((bplcon1 >> 4) & 0x0c00) >> 6);
    int shdelay1 = (bplcon1 >> 12) & 3;
    int shdelay2 = (bplcon1 >> 8) & 3;
    
    delay1 += delayoffset;
    delay2 += delayoffset;
    toscr_delay1x = (delay1 & (delaymask >> toscr_res)) << toscr_res;
    toscr_delay1x |= shdelay1 >> (2 - toscr_res);
    toscr_delay2x = (delay2 & (delaymask >> toscr_res)) << toscr_res;
    toscr_delay2x |= shdelay2 >> (2 - toscr_res);
}

STATIC_INLINE void compute_toscr_delay ()
{
   toscr_res = res_bplcon0;
   toscr_nr_planes = planes_limit_bplcon0;
   toscr_res2 = 2 << toscr_res;
   compute_toscr_delay_1 ();
}

STATIC_INLINE void maybe_first_bpl1dat (int hpos)
{
    if (thisline_decision.plfleft == -1) {
	thisline_decision.plfleft = hpos;
	compute_delay_offset ();
	compute_toscr_delay_1 ();
    }
}

STATIC_INLINE void fetch (int nr, int fm)
{
  uaecptr p;
  if (nr >= toscr_nr_planes)
  	return;
  p = bplpt[nr];
  switch (fm) {
     case 0:
        fetched[nr] = last_custom_value = CHIPMEM_AGNUS_WGET_CUSTOM (p);
        bplpt[nr] += 2;
        break;
     case 1:
        fetched_aga0[nr] = CHIPMEM_LGET_CUSTOM (p);
      	last_custom_value = (uae_u16)fetched_aga0[nr];
        bplpt[nr] += 4;
        break;
     case 2:
        fetched_aga1[nr] = CHIPMEM_LGET_CUSTOM (p);
        fetched_aga0[nr] = CHIPMEM_LGET_CUSTOM (p + 4);
      	last_custom_value = (uae_u16)fetched_aga0[nr];
        bplpt[nr] += 8;
        break;
  }
  if (passed_plfstop == 2 && fetch_cycle >= (fetch_cycle & ~fetchunit_mask) + fetch_modulo_cycle) {
     int mod;
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
  }
  if (nr == 0)
       fetch_state = fetch_was_plane0;
}

STATIC_INLINE void update_toscr_planes (void)
{
  if (toscr_nr_planes > thisline_decision.nr_planes) {
    if(thisline_decision.nr_planes > 0)
    {
    	int j;
    	for (j = thisline_decision.nr_planes; j < toscr_nr_planes; j++)
    	{
        memset ((uae_u32 *)(line_data[next_lineno] + 2 * MAX_WORDS_PER_LINE * j), 0, out_offs * 4);
      }
    }
    thisline_decision.nr_planes = toscr_nr_planes;
  }
}

STATIC_INLINE void toscr_3_ecs (int nbits)
{
	int delay1 = toscr_delay[0];
	int delay2 = toscr_delay[1];
	int i;
	uae_u32 mask = 0xFFFF >> (16 - nbits);
	
	for (i = 0; i < toscr_nr_planes; i += 2) {
		outword[i] <<= nbits;
		outword[i] |= (todisplay[i][0] >> (16 - nbits + delay1)) & mask;
		todisplay[i][0] <<= nbits;
	}
	for (i = 1; i < toscr_nr_planes; i += 2) {
		outword[i] <<= nbits;
		outword[i] |= (todisplay[i][0] >> (16 - nbits + delay2)) & mask;
		todisplay[i][0] <<= nbits;
	}
}


STATIC_INLINE void shift32plus (uae_u32 *p, const int n)
{
	uae_u32 t = p[1];
	t <<= n;
	t |= p[0] >> (32 - n);
	p[1] = t;
}

STATIC_INLINE void aga_shift (uae_u32 *p, const int n, const int fm)
{
	if (fm == 2) {
		shift32plus (p + 2, n);
		shift32plus (p + 1, n);
	}
	shift32plus (p + 0, n);
	p[0] <<= n;
}


STATIC_INLINE void toscr_3_aga (int nbits, int fm)
{
  int offs[2];
  int off1[2];
	int i;
	uae_u32 mask = 0xFFFF >> (16 - nbits);
  
	offs[0] = (16 << fm) - nbits + toscr_delay[0];
	off1[0] = offs[0] >> 5;
	if (off1[0] == 3)
		off1[0] = 2;
	offs[0] -= off1[0] * 32;

	offs[1] = (16 << fm) - nbits + toscr_delay[1];
	off1[1] = offs[1] >> 5;
	if (off1[1] == 3)
		off1[1] = 2;
	offs[1] -= off1[1] * 32;

	for (i = 0; i < toscr_nr_planes; i += 1) {
    int idx = i & 1;
		uae_u32 t0 = todisplay[i][off1[idx]];
		uae_u32 t1 = todisplay[i][off1[idx] + 1];
		uae_u64 t = (((uae_u64)t1) << 32) | t0;
		outword[i] <<= nbits;
		outword[i] |= (t >> offs[idx]) & mask;
		aga_shift (todisplay[i], nbits, fm);
	}
}

STATIC_INLINE void toscr_1 (int nbits, int fm)
{
   if (fm == 0)
      toscr_3_ecs (nbits);
   else
      toscr_3_aga (nbits, fm);

	out_nbits += nbits;
   if (out_nbits == 32) {
      int i;
      uae_u32 *dataptr32_start = (uae_u32 *)(line_data[next_lineno]);
      uae_u32 *dataptr32 = dataptr32_start + out_offs;
      
      for (i = 0; i < thisline_decision.nr_planes; i++) {
       	 if (i >= toscr_nr_planes)
       	   outword[i] = 0;
         *dataptr32 = outword[i];
         dataptr32 += MAX_WORDS_PER_LINE >> 1;
      }
      out_offs++;
      out_nbits = 0;
   }
}

STATIC_INLINE void toscr_0 (int nbits, int fm)
{
	int t;

	if (nbits > 16) {
		toscr_0 (16, fm);
		nbits -= 16;
	}

	t = 32 - out_nbits;
	if (t < nbits) {
		toscr_1 (t, fm);
		nbits -= t;
	}
	toscr_1 (nbits, fm);
}

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
	return i >> (1 + toscr_res);
}

STATIC_INLINE void flush_display (int fm)
{
    if (toscr_nbits > 0 && thisline_decision.plfleft != -1)
      toscr_0 (toscr_nbits, fm);
    toscr_nbits = 0;
} 

/* Called when all planes have been fetched, i.e. when a new block
   of data is available to be displayed.  The data in fetched[] is
   moved into todisplay[].  */
STATIC_INLINE void beginning_of_plane_block (int pos, int fm)
{
   flush_display (fm);

   if (fm == 0) {
      todisplay[0][0] |= fetched[0];
      todisplay[1][0] |= fetched[1];
      todisplay[2][0] |= fetched[2];
      todisplay[3][0] |= fetched[3];
      todisplay[4][0] |= fetched[4];
      todisplay[5][0] |= fetched[5];
      todisplay[6][0] |= fetched[6];
      todisplay[7][0] |= fetched[7];
   }
   else {
         if (fm == 2) {
            todisplay[0][1] = fetched_aga1[0];
            todisplay[1][1] = fetched_aga1[1];
            todisplay[2][1] = fetched_aga1[2];
            todisplay[3][1] = fetched_aga1[3];
            todisplay[4][1] = fetched_aga1[4];
            todisplay[5][1] = fetched_aga1[5];
            todisplay[6][1] = fetched_aga1[6];
            todisplay[7][1] = fetched_aga1[7];
         }
         todisplay[0][0] = fetched_aga0[0];
         todisplay[1][0] = fetched_aga0[1];
         todisplay[2][0] = fetched_aga0[2];
         todisplay[3][0] = fetched_aga0[3];
         todisplay[4][0] = fetched_aga0[4];
         todisplay[5][0] = fetched_aga0[5];
         todisplay[6][0] = fetched_aga0[6];
         todisplay[7][0] = fetched_aga0[7];
	}

   maybe_first_bpl1dat (pos);
   toscr_delay[0] = toscr_delay1x;
   toscr_delay[1] = toscr_delay2x;
}


#define long_fetch_ecs_init(PLANE, NWORDS, DMA) \
    uae_u16 *real_pt = (uae_u16 *)pfield_xlateptr (bplpt[PLANE], NWORDS << 1); \
    int delay = toscr_delay[(PLANE & 1)]; \
    int tmp_nbits = out_nbits; \
    uae_u32 shiftbuffer = todisplay[PLANE][0]; \
    uae_u32 outval = outword[PLANE]; \
    uae_u32 fetchval = fetched[PLANE]; \
    uae_u32 *dataptr_start = (uae_u32 *)(line_data[next_lineno] + ((PLANE<<1)*MAX_WORDS_PER_LINE)); \
    register uae_u32 *dataptr = dataptr_start + out_offs; \
    if (DMA) \
		bplpt[PLANE] += NWORDS << 1; \
    if (real_pt == 0) \
	    return; \
    while (NWORDS > 0) { \
		int bits_left = 32 - tmp_nbits; \
		uae_u32 t; \
		shiftbuffer |= fetchval; \
		t = (shiftbuffer >> delay) & 0xFFFF;

#define long_fetch_ecs_weird() \
		if (bits_left < 16) { \
			outval <<= bits_left; \
			outval |= t >> (16 - bits_left); \
      *dataptr++ = outval; \
      outval = t; \
			tmp_nbits = 16 - bits_left; \
			shiftbuffer <<= 16; \
		} else


#define long_fetch_ecs_end(PLANE,NWORDS, DMA) \
		{ \
			outval = (outval << 16) | t; \
			shiftbuffer <<= 16; \
			tmp_nbits += 16; \
			if (tmp_nbits == 32) { \
        *dataptr++ = outval; \
				tmp_nbits = 0; \
			} \
		} \
		NWORDS--; \
		if (DMA) { \
			__asm__ __volatile__ (                   \
			    "ldrh    %[val], [%[pt]], #2   \n\t"   \
			    "rev16   %[val], %[val]        \n\t"   \
			    : [val] "=r" (fetchval), [pt] "+r" (real_pt) ); \
		} \
	} \
    fetched[PLANE] = fetchval; \
    todisplay[PLANE][0] = shiftbuffer; \
    outword[PLANE] = outval;


static __inline__ void long_fetch_ecs_0(int plane, int nwords, int dma)
{
	long_fetch_ecs_init(plane, nwords, dma)
	long_fetch_ecs_end(plane, nwords, dma)
}

static __inline__ void long_fetch_ecs_1(int plane, int nwords, int dma)
{
	long_fetch_ecs_init(plane, nwords, dma)
	long_fetch_ecs_weird()
	long_fetch_ecs_end(plane, nwords, dma)
}


#define long_fetch_aga_1_init()                                                                  \
	uae_u32 *real_pt = (uae_u32 *)pfield_xlateptr (bplpt[plane], nwords * 2);    \
	int tmp_nbits = out_nbits;                                                                     \
	uae_u32 outval = outword[plane];                                                               \
	uae_u32 fetchval0 = fetched_aga0[plane];                                                       \
	uae_u32 fetchval1 = fetched_aga1[plane];                                                       \
   uae_u32 *dataptr_start = (uae_u32 *)(line_data[next_lineno] + (plane<<1)*MAX_WORDS_PER_LINE); \
   uae_u32 *dataptr = dataptr_start + out_offs;                                                  \
                                                                                                 \
  int offs = (16 << 1) - 16 + toscr_delay[plane & 1];                                            \
	int off1 = offs >> 5;                                                                          \
	if (off1 == 3)                                                                                 \
		off1 = 2;                                                                                    \
	offs -= off1 << 5;                                                                             \
                                                                                                 \
	if (dma)                                                                                       \
		bplpt[plane] += nwords << 1;                                                                \
                                                                                                 \
	if (real_pt == 0)                                                                              \
		/* @@@ Don't do this, fall back on chipmem_wget instead.  */                                 \
		return;                                                                                      \
                                                                                                 \
  /* Instead of shifting a 64 bit value more than 16 bits, we   */                               \
  /* move the pointer for x bytes and shift a 32 bit value less */                               \
  /* than 16 bits. See (1)                                      */                               \
  int buffer_add = (offs >> 4);                                                                  \
  offs &= 15;                                                                                    \
                                                                                                 \
	while (nwords > 0) {                                                                           \
		int i;                                                                                       \
  	uae_u32 *shiftbuffer = todisplay[plane];                                                 \
                                                                                                 \
		shiftbuffer[0] = fetchval0;                                                                  \
                                                                                                 \
    /* (1) */                                                                                    \
    if(buffer_add)                                                                               \
      shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer + buffer_add);                            \
                                                                                                 \
		for (i = 0; i < (1 << 1); i++) {                                                             \
			int bits_left = 32 - tmp_nbits;                                                            \
                                                                                                 \
			uae_u32 t0 = shiftbuffer[off1];                                                            \
			t0 = (uae_u32)(t0 >> offs) & 0xFFFF;

#define long_fetch_aga_2_init()                                                                  \
	uae_u32 *real_pt = (uae_u32 *)pfield_xlateptr (bplpt[plane], nwords * 2);    \
	int tmp_nbits = out_nbits;                                                                     \
	uae_u32 outval = outword[plane];                                                               \
	uae_u32 fetchval0 = fetched_aga0[plane];                                                       \
	uae_u32 fetchval1 = fetched_aga1[plane];                                                       \
   uae_u32 *dataptr_start = (uae_u32 *)(line_data[next_lineno] + (plane<<1)*MAX_WORDS_PER_LINE); \
   uae_u32 *dataptr = dataptr_start + out_offs;                                                  \
                                                                                                 \
  int offs = (16 << 2) - 16 + toscr_delay[plane & 1];                                            \
	int off1 = offs >> 5;                                                                          \
	if (off1 == 3)                                                                                 \
		off1 = 2;                                                                                    \
	offs -= off1 << 5;                                                                             \
                                                                                                 \
	if (dma)                                                                                       \
		bplpt[plane] += nwords << 1;                                                                \
                                                                                                 \
	if (real_pt == 0)                                                                              \
		/* @@@ Don't do this, fall back on chipmem_wget instead.  */                                 \
		return;                                                                                      \
                                                                                                 \
  /* Instead of shifting a 64 bit value more than 16 bits, we   */                               \
  /* move the pointer for x bytes and shift a 32 bit value less */                               \
  /* than 16 bits. See (1)                                      */                               \
  int buffer_add = (offs >> 4);                                                                  \
  offs &= 15;                                                                                    \
                                                                                                 \
	while (nwords > 0) {                                                                           \
		int i;                                                                                       \
  	uae_u32 *shiftbuffer = todisplay[plane];                                                 \
                                                                                                 \
		shiftbuffer[0] = fetchval0;                                                                  \
  	shiftbuffer[1] = fetchval1;                                                                  \
                                                                                                 \
    /* (1) */                                                                                    \
    if(buffer_add)                                                                               \
      shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer + buffer_add);                            \
                                                                                                 \
		for (i = 0; i < (1 << 2); i++) {                                                             \
			int bits_left = 32 - tmp_nbits;                                                            \
                                                                                                 \
			uae_u32 t0 = shiftbuffer[off1];                                                            \
			t0 = (uae_u32)(t0 >> offs) & 0xFFFF;

#define long_fetch_aga_weird()                                    \
			if (bits_left < 16) {                                       \
				outval <<= bits_left;                                     \
				outval |= t0 >> (16 - bits_left);                         \
        *dataptr++ = outval;                                      \
				outval = t0;                                              \
				tmp_nbits = 16 - bits_left;                               \
				/* Instead of shifting 128 bit of data for 16 bit,     */ \
				/* we move the pointer two bytes. See also (2) and (3) */ \
				/*aga_shift (shiftbuffer, 16, fm);                     */ \
				shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer - 1);    \
			} else 


#define long_fetch_aga_1_end()                                    \
			{                                                           \
				outval = (outval << 16) | t0;                             \
				/* (2)                              */                    \
				/*aga_shift (shiftbuffer, 16, fm);  */                    \
				shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer - 1);    \
				tmp_nbits += 16;                                          \
				if (tmp_nbits == 32) {                                    \
               *dataptr++ = outval;                               \
               tmp_nbits = 0;                                     \
				}                                                         \
			}                                                           \
		}                                                             \
                                                                  \
    /* (3)                                               */       \
    /* We have to move the data, but now, we can simply  */       \
    /* copy long values and have to do it only once.     */       \
      todisplay[plane][1] = todisplay[plane][0];                  \
                                                                  \
		nwords -= 1 << 1;                                             \
                                                                  \
		if (dma) {                                                    \
			  fetchval0 = do_get_mem_long (real_pt);                    \
        real_pt += 1;                                             \
		}                                                             \
	}                                                               \
	fetched_aga0[plane] = fetchval0;                                \
	fetched_aga1[plane] = fetchval1;                                \
	outword[plane] = outval;

#define long_fetch_aga_2_end()                                    \
			{                                                           \
				outval = (outval << 16) | t0;                             \
				/* (2)                              */                    \
				/*aga_shift (shiftbuffer, 16, fm);  */                    \
				shiftbuffer = (uae_u32 *)((uae_u16 *)shiftbuffer - 1);    \
				tmp_nbits += 16;                                          \
				if (tmp_nbits == 32) {                                    \
               *dataptr++ = outval;                               \
               tmp_nbits = 0;                                     \
				}                                                         \
			}                                                           \
		}                                                             \
                                                                  \
    /* (3)                                               */       \
    /* We have to move the data, but now, we can simply  */       \
    /* copy long values and have to do it only once.     */       \
      todisplay[plane][3] = todisplay[plane][1];                  \
      todisplay[plane][2] = todisplay[plane][0];                  \
                                                                  \
		nwords -= 1 << 2;                                             \
                                                                  \
		if (dma) {                                                    \
		    fetchval1 = do_get_mem_long (real_pt);                    \
		    fetchval0 = do_get_mem_long (real_pt + 1);                \
        real_pt += 2;                                            \
		}                                                             \
	}                                                               \
	fetched_aga0[plane] = fetchval0;                                \
	fetched_aga1[plane] = fetchval1;                                \
	outword[plane] = outval;


static __inline__ void long_fetch_aga_1_0(int plane, int nwords, int dma)
{
	long_fetch_aga_1_init()
	long_fetch_aga_1_end()
}

static __inline__ void long_fetch_aga_2_0(int plane, int nwords, int dma)
{
	long_fetch_aga_2_init()
	long_fetch_aga_2_end()
}

static __inline__ void long_fetch_aga_1_1(int plane, int nwords, int dma)
{
	long_fetch_aga_1_init()
	long_fetch_aga_weird()
	long_fetch_aga_1_end()
}
    
static __inline__ void long_fetch_aga_2_1(int plane, int nwords, int dma)
{
	long_fetch_aga_2_init()
	long_fetch_aga_weird()
	long_fetch_aga_2_end()
}

STATIC_INLINE void do_long_fetch (int nwords, int dma, int fm)
{
	int i;

	flush_display (fm);
	switch (fm) {
		case 0:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_ecs_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_ecs_0 (i, nwords, dma);
			}
			break;
		case 1:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_1_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_1_0 (i, nwords, dma);
			}
			break;
		case 2:
			if (out_nbits & 15) {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_2_1 (i, nwords, dma);
			} else {
				for (i = 0; i < toscr_nr_planes; i++)
					long_fetch_aga_2_0 (i, nwords, dma);
			}
			break;
	}

	out_nbits += nwords * 16;
	out_offs += out_nbits >> 5;
	out_nbits &= 31;

    if (dma && toscr_nr_planes > 0)
		fetch_state = fetch_was_plane0;
}


/* make sure fetch that goes beyond maxhpos is finished */
STATIC_INLINE void finish_final_fetch (int i, int fm)
{
   if ((thisline_decision.plfleft == -1) || 
            (passed_plfstop == 3))
      return;

   passed_plfstop = 3;

   ddfstate = DIW_waiting_start;
   i += flush_plane_data (fm);
   thisline_decision.plfright = i;
   thisline_decision.plflinelen = out_offs;
   finish_playfield_line ();
}

STATIC_INLINE int ONE_FETCH(int i, int ddfstop_to_test, int dma, int fm)
{
	if (! passed_plfstop && (i == ddfstop_to_test))
		passed_plfstop = 1;
	if ((fetch_cycle & fetchunit_mask) == 0) {
		if (passed_plfstop == 2) {
			finish_final_fetch (i, fm);
			return 1;
		}
		if (passed_plfstop)
			passed_plfstop++;
	}
	if (dma) {
	/* fetchstart_mask can be larger than fm_maxplane if FMODE > 0.  This means
	   that the remaining cycles are idle; we'll fall through the whole switch
	   without doing anything.  */
		int cycle_start = fetch_cycle & fetchstart_mask;
		switch (fm_maxplane) {
			case 8:
				switch (cycle_start) {
					case 0: fetch (7, fm); break;
					case 1: fetch (3, fm); break;
					case 2: fetch (5, fm); break;
					case 3: fetch (1, fm); break;
					case 4: fetch (6, fm); break;
					case 5: fetch (2, fm); break;
					case 6: fetch (4, fm); break;
					case 7: fetch (0, fm); break;
				}
				break;
			case 4:
				switch (cycle_start) {
					case 0: fetch (3, fm); break;
					case 1: fetch (1, fm); break;
					case 2: fetch (2, fm); break;
					case 3: fetch (0, fm); break;
				}
				break;
			case 2:
				switch (cycle_start) {
					case 0: fetch (1, fm); break;
					case 1: fetch (0, fm); break;
				}
				break;
		}
	}
	fetch_cycle++;
	toscr_nbits += toscr_res2;

	if (toscr_nbits == 16)
		flush_display (fm);
	
	return 0;
}

static void update_fetch (int until, int fm)
{
	int pos;
	int dma = dmaen (DMA_BITPLANE);
	int ddfstop_to_test;

    if (nodraw() || passed_plfstop == 3)
		return;

    /* We need an explicit test against HARD_DDF_STOP here to guard against
       programs that move the DDFSTOP before our current position before we
       reach it.  */
	ddfstop_to_test = HARD_DDF_STOP;
	if ((ddfstop >= last_fetch_hpos) && (plfstop < HARD_DDF_STOP))
		ddfstop_to_test = plfstop;

	compute_toscr_delay();
	update_toscr_planes();

	pos = last_fetch_hpos;
	cycle_diagram_shift = (last_fetch_hpos - fetch_cycle) & fetchstart_mask;

	/* First, a loop that prepares us for the speedup code.  We want to enter
       the SPEEDUP case with fetch_state == fetch_was_plane0, and then unroll
       whole blocks, so that we end on the same fetch_state again.  */
	for (; ; pos++) {
		if (pos == until) {
			if (until >= maxhpos) {
				finish_final_fetch (pos, fm);
				return;
			}
			flush_display (fm);
			return;
		}

		if (fetch_state == fetch_was_plane0)
			break;

		fetch_state = fetch_started;
		if (ONE_FETCH(pos, ddfstop_to_test, dma, fm))
		   return;
	}

	if (! passed_plfstop && (ddf_change != vpos) && (ddf_change + 1 != vpos)
			&& dma
			&& ((fetch_cycle & fetchstart_mask) == (fm_maxplane & fetchstart_mask))
			&& ((toscr_delay[0] == toscr_delay1x) && (toscr_delay[1] == toscr_delay2x))
			&& (toscr_nr_planes == thisline_decision.nr_planes))
	{
		int offs = (pos - fetch_cycle) & fetchunit_mask;
		int ddf2 = ((ddfstop_to_test - offs + fetchunit - 1) & ~fetchunit_mask) + offs;
		int ddf3 = ddf2 + fetchunit;
		int stop = until < ddf2 ? until : until < ddf3 ? ddf2 : ddf3;
		int count;

		count = stop - pos;

		if (count >= fetchstart) {
			count &= ~fetchstart_mask;

			if (thisline_decision.plfleft == -1) {
				compute_delay_offset ();
				compute_toscr_delay_1 ();
			}
			do_long_fetch (count >> (3 - toscr_res), dma, fm);

	    /* This must come _after_ do_long_fetch so as not to confuse flush_display
	       into thinking the first fetch has produced any output worth emitting to
	       the screen.  But the calculation of delay_offset must happen _before_.  */
			maybe_first_bpl1dat (pos);

	    if (pos <= ddfstop_to_test && pos + count > ddfstop_to_test)
				passed_plfstop = 1;
	    if (pos <= ddfstop_to_test && pos + count > ddf2)
				passed_plfstop = 2;
 	    if (pos <= ddf2 && pos + count >= ddf2 + fm_maxplane)
			   add_modulos ();
			pos += count;
			fetch_cycle += count;
		}
	}

	for (; pos < until; pos++) {
		if (fetch_state == fetch_was_plane0)
			beginning_of_plane_block (pos, fm);
		fetch_state = fetch_started;
		if (ONE_FETCH(pos, ddfstop_to_test, dma, fm))
		   return;
	}

	if (until >= maxhpos) {
		finish_final_fetch (pos, fm);
		return;
	}
	flush_display (fm);
}

STATIC_INLINE void decide_fetch (int hpos)
{
  if (fetch_state != fetch_not_started && hpos > last_fetch_hpos) {
    update_fetch (hpos, fetchmode);
	}
	last_fetch_hpos = hpos;
}

static __inline__ void start_bpl_dma (int hstart)
{
   fetch_state = fetch_started;
   fetch_cycle = 0;
   last_fetch_hpos = hstart;
   out_nbits = 0;
   out_offs = 0;
   toscr_nbits = 0;
   thisline_decision.bplres = res_bplcon0;

   ddfstate = DIW_waiting_stop;
   compute_toscr_delay ();

   /* If someone already wrote BPL1DAT, clear the area between that point and
       the real fetch start.  */
   if (!nodraw ()) {
      if (thisline_decision.plfleft != -1) {
         out_nbits = (plfstrt - thisline_decision.plfleft) << (1 + toscr_res);
         out_offs = out_nbits >> 5;
         out_nbits &= 31;
      }
      update_toscr_planes ();
   }
}

/* this may turn on datafetch if program turns dma on during the ddf */
static __inline__ void maybe_start_bpl_dma (int hpos)
{
   /* OCS: BPL DMA never restarts if DMA is turned on during DDF
    * ECS/AGA: BPL DMA restarts but only if DMA was turned off
       outside of DDF or during current line, otherwise display
       processing jumps immediately to "DDFSTOP passed"-condition */
   if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
      return;
   if (fetch_state != fetch_not_started)
      return;
   if (diwstate != DIW_waiting_stop)
      return;
   if (hpos <= plfstrt)
      return;
   if (hpos > plfstop - fetchunit)
      return;
   if (ddfstate != DIW_waiting_start)
      passed_plfstop = 1;
   start_bpl_dma (hpos);
}

/* This function is responsible for turning on datafetch if necessary.  */
STATIC_INLINE void decide_line (int hpos)
{
	/* Take care of the vertical DIW.  */
	if (vpos == plffirstline) {
		diwstate = DIW_waiting_stop;
		ddf_change = vpos;
	}
	if (vpos == plflastline) {
		diwstate = DIW_waiting_start;
		ddf_change = vpos;
	}
   
   if (hpos <= last_decide_line_hpos)
      return;
   if (fetch_state != fetch_not_started)
      return;

   /* Test if we passed the start of the DDF window.  */
    if (last_decide_line_hpos < plfstrt && hpos >= plfstrt) {
    	/* If DMA isn't on by the time we reach plfstrt, then there's no
	       bitplane DMA at all for the whole line.  */
       if (dmaen (DMA_BITPLANE) && diwstate == DIW_waiting_stop) {
         /* hack warning.. Writing to DDFSTRT when DMA should start must be ignored
          * (correct fix would be emulate this delay for every custom register, but why bother..) */
         if ((hpos - 2 != ddfstrt_old_hpos) || (ddfstrt_old_vpos != vpos))
         {
            start_bpl_dma (plfstrt);
            estimate_last_fetch_cycle (plfstrt);
            last_decide_line_hpos = hpos;
            do_sprites (plfstrt);
            return;
         }
      }
   }

   if (last_sprite_decide_line_hpos < SPR0_HPOS + 4 * MAX_SPRITES)
      do_sprites (hpos);
   last_sprite_decide_line_hpos = hpos;
   
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
	
	if (thisline_decision.ctable == -1)
		remember_ctable ();
	
	curr_color_changes[next_color_change].linepos = hpos;
	curr_color_changes[next_color_change].regno = regno;
	curr_color_changes[next_color_change++].value = value;
  curr_color_changes[next_color_change].regno = -1;
}

static void record_register_change (int hpos, int regno, unsigned long value)
{
  if (regno == 0x100) {
	  if (value & 0x800)
	    thisline_decision.ham_seen = 1;
	  if (hpos < HARD_DDF_START || hpos < plfstrt + 0x20) {
	    thisline_decision.bplcon0 = value;
	    thisline_decision.bplres = res_bplcon0;
	  }
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

#define DO_SPRITE_COLLISIONS \
{ \
   if (!clxcon_bpl_enable) \
      clxdat |= 0x1FE; \
   else \
      do_sprite_collisions (); \
}

/* handle very rarely needed playfield collision (CLXDAT bit 0) */
static void do_playfield_collisions (void)
{
   int bplres = res_bplcon0;
   hwres_t ddf_left = thisline_decision.plfleft * 2 << bplres;
   hwres_t hw_diwlast = coord_window_to_diw_x (thisline_decision.diwlastword);
   hwres_t hw_diwfirst = coord_window_to_diw_x (thisline_decision.diwfirstword);
   int i, collided, minpos, maxpos;
   int planes = (currprefs.chipset_mask & CSMASK_AGA) ? 8 : 6;

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

/* Sprite-to-sprite collisions are taken care of in record_sprite.  This one does
   playfield/sprite collisions. */
static void do_sprite_collisions (void)
{
   int nr_sprites = curr_drawinfo[next_lineno].nr_sprites;
   int first = curr_drawinfo[next_lineno].first_sprite_entry;
   int i;
   unsigned int collision_mask = clxmask[clxcon >> 12];
   int bplres = res_bplcon0;
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
	       int k;
	       int offs;
	       int match = 1;

         if (sprpix == 0)
            continue;

         offs = ((j << bplres) >> sprite_buffer_res) - ddf_left;
         sprpix = sprite_ab_merge[sprpix & 255] | (sprite_ab_merge[sprpix >> 8] << 2);
         sprpix <<= 1;

         /* Loop over number of playfields.  */
         for (k = 1; k >= 0; k--) {
            int l;
            int planes = (currprefs.chipset_mask & CSMASK_AGA) ? 8 : 6;

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

STATIC_INLINE void record_sprite_1 (uae_u16 *_GCCRES_ buf, uae_u32 datab, int num, int dbl,
				    unsigned int mask, int do_collisions, uae_u32 collision_mask)
{
	int j = 0;
	while (datab) {
		unsigned int tmp = *buf;
		unsigned int col = (datab & 3) << (2 * num);
		tmp |= col;
		if ((j & mask) == 0)
			*buf++ = tmp;
	  if (dbl > 0)
			*buf++ = tmp;
		j++;
		datab >>= 2;
		if (do_collisions) {
			tmp &= collision_mask;
			if (tmp) {
				unsigned int shrunk_tmp = sprite_ab_merge[tmp & 255] | (sprite_ab_merge[tmp >> 8] << 2);
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

   The data is recorded either in lores pixels (if ECS), or in hires pixels
   (if AGA).  No support for SHRES sprites.  */

static void record_sprite (int line, int num, int sprxp, uae_u16 *_GCCRES_ data, uae_u16 *_GCCRES_ datb, unsigned int ctl)
{
	struct sprite_entry *e = curr_sprite_entries + next_sprite_entry;
	int i;
	int word_offs;
	uae_u16 *buf;
	uae_u32 collision_mask;
	int width, dbl, half;
	unsigned int mask = 0;
	
  half = 0;
	dbl = sprite_buffer_res - sprres;
  if (dbl < 0) {
    half = -dbl;
    dbl = 0;
  }
	mask = sprres == RES_SUPERHIRES ? 1 : 0;
  width = (sprite_width << sprite_buffer_res) >> sprres;
	
	/* Try to coalesce entries if they aren't too far apart.  */
  if (! next_sprite_forced && e[-1].max + 16 >= sprxp) {
		e--;
	} else {
		next_sprite_entry++;
		e->pos = sprxp;
		e->has_attached = 0;
	}
	
	if (sprxp < e->pos)
		return;
	
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
		
		buf = spixels + word_offs + ((i << dbl) >> half);
  	if (currprefs.collision_level > 0 && collision_mask)
  		record_sprite_1 (buf, datab, num, dbl, mask, 1, collision_mask);
	  else
		  record_sprite_1 (buf, datab, num, dbl, mask, 0, collision_mask);
		data++;
		datb++;
	}
	
	/* We have 8 bits per pixel in spixstate, two for every sprite pair.  The
       low order bit records whether the attach bit was set for this pair.  */
  if (spr[num & ~1].armed && ((sprctl[num | 1] & 0x80) || (!(currprefs.chipset_mask & CSMASK_AGA) && (sprctl[num & ~1] & 0x80)))) {
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

static void decide_sprites (int hpos)
{
	int nrs[MAX_SPRITES], posns[MAX_SPRITES];
	int count, i;
  /* apparantly writes to custom registers happen in the 3/4th of cycle 
   * and sprite xpos comparator sees it immediately */
	int point = hpos * 2 - 3;
	int width = sprite_width;
	int window_width = width >> sprres;
	
  if (nodraw () || hpos < 0x14 || nr_armed == 0 || point == last_sprite_point)
		return;
	
	decide_diw (hpos);
	decide_line (hpos);
	
	count = 0;
	for (i = 0; i < MAX_SPRITES; i++) {
		int sprxp = spr[i].xpos;
		int hw_xp = sprxp >> sprite_buffer_res;
		int window_xp = coord_hw_to_window_x (hw_xp) + (DIW_DDF_OFFSET);
		int j, bestp;
		
	  if (! spr[i].armed)
			continue;

    if (sprxp < 0 || hw_xp <= last_sprite_point || hw_xp > point)
	    continue;

		if ( !(bplcon3 & 2) && /* sprites outside playfields enabled? */
	    ((thisline_decision.diwfirstword >= 0 && window_xp + window_width < thisline_decision.diwfirstword)
	    || (thisline_decision.diwlastword >= 0 && window_xp > thisline_decision.diwlastword)))
			continue;
		
		/* Sort the sprites in order of ascending X position before recording them.  */
		for (bestp = 0; bestp < count; bestp++) {
			if (posns[bestp] > sprxp)
				break;
	    if (posns[bestp] == sprxp && nrs[bestp] < i)
				break;
		}
		for (j = count; j > bestp; j--) {
			posns[j] = posns[j-1];
			nrs[j] = nrs[j-1];
		}
		posns[j] = sprxp;
		nrs[j] = i;
		count++;
	}
	for (i = 0; i < count; i++) {
		int nr = nrs[i];    
		record_sprite (next_lineno, nr, spr[nr].xpos, sprdata[nr], sprdatb[nr], sprctl[nr]);
	}
	last_sprite_point = point;
}

/* End of a horizontal scan line. Finish off all decisions that were not
 * made yet. */
static __inline__ void finish_decisions (void)
{
	struct draw_info *dip;
	
	if (nodraw ())
		return;
	
	int hpos = current_hpos ();

	decide_diw (hpos);
	decide_line (hpos);
	decide_fetch (hpos);
	
    if (thisline_decision.plfleft != -1 && thisline_decision.plflinelen == -1) {
	    if (fetch_state != fetch_not_started) {
			    return;
	    }
		  thisline_decision.plfright = thisline_decision.plfleft;
		  thisline_decision.plflinelen = 0;
		  thisline_decision.bplres = RES_LORES;
	  }
	
	/* Large DIWSTOP values can cause the stop position never to be
	 * reached, so the state machine always stays in the same state and
	 * there's a more-or-less full-screen DIW. */
    if (hdiwstate == DIW_waiting_stop || thisline_decision.diwlastword > max_diwlastword)
		thisline_decision.diwlastword = max_diwlastword;
	
	if (thisline_decision.plfleft != -1 || (bplcon3 & 2))
		decide_sprites (hpos + 1);
	
	dip = curr_drawinfo + next_lineno;

	dip->last_sprite_entry = next_sprite_entry;
	dip->last_color_change = next_color_change;
	
	if (thisline_decision.ctable == -1) {
		remember_ctable();
  }
	
	dip->nr_color_changes = next_color_change - dip->first_color_change;
	dip->nr_sprites = next_sprite_entry - dip->first_sprite_entry;
	
	line_decisions[next_lineno] = thisline_decision;
}

/* Set the state of all decisions to "undecided" for a new scanline. */
static __inline__ void reset_decisions (void)
{
   if (nodraw ())
      return;
   
   thisline_decision.bplres = res_bplcon0;
   thisline_decision.nr_planes = 0;
   
   thisline_decision.plfleft = -1;
   thisline_decision.plflinelen = -1;
   thisline_decision.ham_seen = !! (bplcon0 & 0x800);
   thisline_decision.ham_at_start = !! (bplcon0 & 0x800);
   
   /* decided_res shouldn't be touched before it's initialized by decide_line(). */
   thisline_decision.diwfirstword = -1;
   thisline_decision.diwlastword = -1;
   if (hdiwstate == DIW_waiting_stop) {
      thisline_decision.diwfirstword = 0;
   }
   thisline_decision.ctable = -1;
   
   curr_drawinfo[next_lineno].first_color_change = next_color_change;
   curr_drawinfo[next_lineno].first_sprite_entry = next_sprite_entry;
   next_sprite_forced = 1;
   
   last_sprite_point = 0;
   fetch_state = fetch_not_started;
   passed_plfstop = 0;
   
   {
      register int i;
      register int c = 0;
      register unsigned *ptr0=(uae_u32 *)todisplay;  /* clear only todisplay[i][0] if OCS/ECS */
      register unsigned *ptr1=fetched;
      register unsigned *ptr2=outword;
      if (currprefs.chipset_mask & CSMASK_AGA) 
      {
        register unsigned *ptr3=fetched_aga0;
        register unsigned *ptr4=fetched_aga1;
        for(i=0;i<MAX_PLANES;i++)
        {
            *ptr0++ = c;
            *ptr0++ = c;
            *ptr0++ = c;
            *ptr0++ = c;
            *ptr3++ = c;
            *ptr4++ = c;
            *ptr1++ = c;
            *ptr2++ = c;
        }
      } 
      else 
      {
        for(i=0;i<MAX_PLANES;i++)
        {
            *ptr0 = c;
            ptr0 += 4;
            *ptr1++ = c;
            *ptr2++ = c;
        }
      }
   } 
   
   last_decide_line_hpos = -1;
   last_sprite_decide_line_hpos = -1;
   last_diw_pix_hpos = -1;
   last_sprite_hpos = -1;
   last_fetch_hpos = -1;

    /* These are for comparison. */
    thisline_decision.bplcon0 = bplcon0;
    thisline_decision.bplcon2 = bplcon2;
    thisline_decision.bplcon3 = bplcon3;
    thisline_decision.bplcon4 = bplcon4;
}

void compute_vsynctime (void)
{
    if (currprefs.chipset_refreshrate) {
	vblank_hz = currprefs.chipset_refreshrate;
    }
//    if (turbo_emulation)
//	vsynctime = 1;
//    else
	vsynctime = syncbase / vblank_hz;
    if (currprefs.produce_sound > 1)
  	  update_sound (vblank_hz);
}

static void dumpsync (void)
{
#if 0
    write_log ("BEAMCON0 = %04.4X VTOTAL=%04.4X HTOTAL=%04.4X\n", new_beamcon0, vtotal, htotal);
    write_log ("HSSTOP=%04.4X HBSTRT=%04.4X HBSTOP=%04.4X\n", hsstop, hbstrt, hbstop);
    write_log ("VSSTOP=%04.4X VBSTRT=%04.4X VBSTOP=%04.4X\n", vsstop, vbstrt, vbstop);
    write_log ("HSSTRT=%04.4X VSSTRT=%04.4X HCENTER=%04.4X\n", hsstrt, vsstrt, hcenter);
#endif
}

/* set PAL or NTSC timing variables */
void init_hz (void)
{
	int isntsc;
	
	beamcon0 = new_beamcon0;
	isntsc = beamcon0 & 0x20 ? 0 : 1;
  if (hack_vpos > 0) {
	  if (maxvpos == hack_vpos) 
	    return;
	  maxvpos = hack_vpos;
	  vblank_hz = 15600 / hack_vpos;
	  hack_vpos = -1;
  } else if (hack_vpos < 0) {
	  hack_vpos = 0;
  }
  if (hack_vpos == 0) {
	  if (!isntsc) {
		  maxvpos = MAXVPOS_PAL;
		  maxhpos = MAXHPOS_PAL;
		  minfirstline = VBLANK_ENDLINE_PAL;
	    vblank_hz = VBLANK_HZ_PAL;
	    sprite_vblank_endline = VBLANK_SPRITE_PAL;
	  } else {
		  maxvpos = MAXVPOS_NTSC;
		  maxhpos = MAXHPOS_NTSC;
		  minfirstline = VBLANK_ENDLINE_NTSC;
	    vblank_hz = VBLANK_HZ_NTSC;
	    sprite_vblank_endline = VBLANK_SPRITE_NTSC;
	  }
	}
  if (beamcon0 & 0x80) {
	  if (vtotal >= MAXVPOS)
	    vtotal = MAXVPOS - 1;
	  maxvpos = vtotal + 1;
	  if (htotal >= MAXHPOS)
	    htotal = MAXHPOS - 1;
	  maxhpos = htotal + 1;
	  vblank_hz = 227 * 312 * 50 / (maxvpos * maxhpos);
	  minfirstline = vsstop;
	  if (minfirstline < 2)
	      minfirstline = 2;
	  if (minfirstline >= maxvpos)
	      minfirstline = maxvpos - 1;
	  sprite_vblank_endline = minfirstline - 2;
  }
  /* limit to sane values */
  if (vblank_hz < 10)
	  vblank_hz = 10;
  if (vblank_hz > 300)
	  vblank_hz = 300;
  eventtab[ev_hsync].oldcycles = get_cycles ();
  eventtab[ev_hsync].evtime = get_cycles() + HSYNCTIME;
  events_schedule ();
  compute_vsynctime ();
#ifdef PICASSO96
  init_hz_p96 ();
#endif
}

static void calcdiw (void)
{
	int hstrt = diwstrt & 0xFF;
	int hstop = diwstop & 0xFF;
	int vstrt = diwstrt >> 8;
	int vstop = diwstop >> 8;
	
	if (diwhigh_written) {
		hstrt |= ((diwhigh >> 5) & 1) << 8;
		hstop |= ((diwhigh >> 13) & 1) << 8;
		vstrt |= (diwhigh & 7) << 8;
		vstop |= ((diwhigh >> 8) & 7) << 8;
	} else {
		hstop += 0x100;
		if ((vstop & 0x80) == 0)
			vstop |= 0x100;
	}
	
	diwfirstword = coord_diw_to_window_x (hstrt);
	diwlastword = coord_diw_to_window_x (hstop);
   if (diwfirstword >= diwlastword) {
      diwfirstword = min_diwlastword;
      diwlastword = max_diwlastword;
   }
	if (diwfirstword < 0)
		diwfirstword = 0;
	
	plffirstline = vstrt;
	plflastline = vstop;
	
	plfstrt = ddfstrt;
	plfstop = ddfstop;
   if (ddfstop > maxhpos)
       plfstrt = 0;
   if (plfstrt < HARD_DDF_START)
      plfstrt = HARD_DDF_START;
}

/* display mode changed (lores, doubling etc..), recalculate everything */
void init_custom (void)
{
    update_mirrors();
    create_cycle_diagram_table ();
    reset_drawing ();
    init_hz ();
    calcdiw ();
}

static int timehack_alive = 0;

static uae_u32 REGPARAM2 timehack_helper (TrapContext *context)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    if (m68k_dreg (&context->regs, 0) == 0)
	return timehack_alive;

    timehack_alive = 10;

    gettimeofday (&tv, NULL);
    put_long (m68k_areg (&context->regs, 0), tv.tv_sec - (((365 * 8 + 2) * 24) * 60 * 60));
    put_long (m68k_areg (&context->regs, 0) + 4, tv.tv_usec);
    return 0;
#else
    return 2;
#endif
}

 /*
  * register functions
  */
STATIC_INLINE uae_u16 DENISEID (void)
{
	if (currprefs.chipset_mask & CSMASK_AGA)
		return 0xF8;
	if (currprefs.chipset_mask & CSMASK_ECS_DENISE)
		return 0xFC;
  return 0xffff;
}
STATIC_INLINE uae_u16 DMACONR (void)
{
    return (dmacon | (bltstate == BLT_done ? 0 : 0x4000) | (blt_info.blitzero ? 0x2000 : 0));
}
STATIC_INLINE uae_u16 INTENAR (void)
{
    return intena;
}
uae_u16 INTREQR (void)
{
    return intreqr;
}
STATIC_INLINE uae_u16 ADKCONR (void)
{
    return adkcon;
}

STATIC_INLINE uae_u16 VPOSR (void)
{
  unsigned int csbit = 0;
  int vp = (vpos >> 8) & 7;

	csbit |= (currprefs.chipset_mask & CSMASK_AGA) ? 0x2300 : 0;
	csbit |= (currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 0x2000 : 0;
	if ((currprefs.chipmem_size > 1024 * 1024) && (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
	    csbit |= 0x2100;
	if (currprefs.ntscmode)
	    csbit |= 0x1000;

    //if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS))
    //	vp &= 1;
    vp = vp | lof | csbit;
    return vp;
}

static void VPOSW (uae_u16 v)
{
	if (lof != (v & 0x8000))
		lof_changed = 1;
	lof = v & 0x8000;
  if ( (v & 1) && vpos > 0)
	  hack_vpos = vpos;
}

#define VHPOSR() ((uae_u16)((vpos << 8) | current_hpos ()))

static __inline__ int test_copper_dangerous (unsigned int address)
{
    if ((address & 0x1fe) < ((copcon & 2) ? ((currprefs.chipset_mask & CSMASK_ECS_AGNUS) ? 0 : 0x40) : 0x80)) {
	cop_state.state = COP_stop;	
	copper_enabled_thisline = 0;
	unset_special (&regs, SPCFLAG_COPPER);
	return 1;
    }
    return 0;
}

static void immediate_copper (int num)
{
    int pos = 0;
    int oldpos = 0;

    cop_state.state = COP_stop;
    cop_state.vpos = vpos;
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
	cop_state.i1 = chipmem_agnus_wget (cop_state.ip);
	cop_state.i2 = chipmem_agnus_wget (cop_state.ip + 2);
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
    unset_special (&regs, SPCFLAG_COPPER);
}

#define COP1LCH(V) cop1lc = (cop1lc & 0xffff) | ((uae_u32)V << 16)
#define COP1LCL(V) cop1lc = (cop1lc & ~0xffff) | (V & 0xfffe)
#define COP2LCH(V) cop2lc = (cop2lc & 0xffff) | ((uae_u32)V << 16)
#define COP2LCL(V) cop2lc = (cop2lc & ~0xffff) | (V & 0xfffe)

static void compute_spcflag_copper (void);

static void COPJMP (int num)
{
#ifdef FAST_COPPER
	int was_active = eventtab[ev_copper].active;
#endif
  int oldstrobe = cop_state.strobe;

  unset_special (&regs, SPCFLAG_COPPER);
	cop_state.ignore_next = 0;
  if (!oldstrobe)
  	cop_state.state_prev = cop_state.state;
	cop_state.state = COP_read1;
	cop_state.vpos = vpos;
	cop_state.hpos = current_hpos () & ~1;
  copper_enabled_thisline = 0;
  cop_state.strobe = num;
	
#ifdef FAST_COPPER
	eventtab[ev_copper].active = 0;
#endif
    if (nocustom()) {
	immediate_copper (num);
	return;
    }
#ifdef FAST_COPPER
	if (was_active)
		events_schedule ();
#endif

	if (dmaen (DMA_COPPER)) {
	  compute_spcflag_copper ();
	} else if (oldstrobe > 0 && oldstrobe != num && cop_state.state_prev == COP_wait) {
	/* dma disabled, copper idle and accessing both COPxJMPs -> copper stops! */
	  cop_state.state = COP_stop;
	}
}

STATIC_INLINE void COPCON (uae_u16 a)
{
    copcon = a;
}

static void DMACON (int hpos, uae_u16 v)
{
	int oldcop, newcop;
  uae_u16 changed;
	
	uae_u16 oldcon = dmacon;
	
	decide_line (hpos);
	decide_fetch (hpos);
	
	setclr (&dmacon, v);
	dmacon &= 0x1FFF;
	
  changed = dmacon ^ oldcon;
  oldcop = (oldcon & DMA_COPPER) && (oldcon & DMA_MASTER);
  newcop = (dmacon & DMA_COPPER) && (dmacon & DMA_MASTER);

  if (oldcop != newcop) {
#ifdef FAST_COPPER
		eventtab[ev_copper].active = 0;
#endif
  	if (newcop && !oldcop) {
  	  compute_spcflag_copper ();
	  } else if (!newcop) {
	    copper_enabled_thisline = 0;
  	  unset_special (&regs, SPCFLAG_COPPER);
    }
	}
	if (((dmacon & DMA_BLITPRI) > (oldcon & DMA_BLITPRI)) && (bltstate != BLT_done)) {
		set_special (&regs, SPCFLAG_BLTNASTY);
	}

	if (dmaen (DMA_BLITTER))
	{ 
	  if(bltstate == BLT_init)
		  blitter_check_start ();
		else if(bltstate == BLT_waitDMA)
		  blitter_dma_enabled();
	}
	else if (bltstate == BLT_work) {
    blitter_dma_disabled();
  }

	if ((dmacon & (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER)) != (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER)) {
		unset_special (&regs, SPCFLAG_BLTNASTY);
  }	
	
  if (changed & (DMA_MASTER | 0x0f))
	  audio_hsync (0);

  if (changed & (DMA_MASTER | DMA_BITPLANE)) {
	  ddf_change = vpos;
    if (dmaen (DMA_BITPLANE))
      maybe_start_bpl_dma (hpos);
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

static void INTENA_f(uae_u32 data)
{
    doint();
}
STATIC_INLINE void INTENA (uae_u16 v)
{
  setclr (&intena,v);
    if (v & 0x8000) {
	if (!currprefs.cpu_compatible > 0)
	    INTENA_f(0);
	else
	    event2_newevent2 (6, 0, INTENA_f);
    }
}

void INTREQ_0 (uae_u16 v)
{
	setclr(&intreq, v);
  intreqr = intreq;
	doint();
}

void INTREQ_f(uae_u32 data)
{
    INTREQ_0 (data);
//    serial_check_irq ();
    rethink_cias ();
}

static void INTREQ_d (uae_u16 v, int d)
{
    intreqr = intreq;
    /* data in intreq is immediately available (vsync only currently because there is something unknown..) */
    setclr (&intreqr, v & (0x8000 | 0x20));
    if (!currprefs.cpu_compatible || v == 0)
	INTREQ_f(v);
    else
	event2_newevent2(d, v, INTREQ_f);
}

void INTREQ (uae_u16 v)
{
    if (!currprefs.cpu_compatible)
	INTREQ_f(v);
    else
	INTREQ_d(v, 6);
}

static void ADKCON (int hpos, uae_u16 v)
{
  if (currprefs.produce_sound > 0)
    update_audio ();

  setclr (&adkcon,v);
  audio_update_adkmasks ();
  DISK_update (hpos);
}

static void BEAMCON0 (uae_u16 v)
{
	if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE))
	  v &= 0x20;
  new_beamcon0 = v;
}

static __inline__ void varsync (void)
{
    if (!(currprefs.chipset_mask & CSMASK_ECS_DENISE))
	return;
    if (!(beamcon0 & 0x80))
	return;
    hack_vpos = -1;
}

static __inline__ int is_bitplane_dma (int hpos)
{
   if (fetch_state == fetch_not_started || hpos < thisline_decision.plfleft)
      return 0;
   if ((passed_plfstop == 3 && hpos >= thisline_decision.plfright)
            || hpos >= estimated_last_fetch_cycle)
      return 0;
   return curr_diagram[(hpos - cycle_diagram_shift) & fetchstart_mask];
}

static void BPLxPTH (int hpos, uae_u16 v, int num)
{
    decide_line (hpos);
    decide_fetch (hpos);
    bplpt[num] = (bplpt[num] & 0xffff) | ((uae_u32)v << 16);
}

static void BPLxPTL (int hpos, uae_u16 v, int num)
{
   int delta = 0;
   decide_line (hpos);
   decide_fetch (hpos);
   /* fix for "bitplane dma fetch at the same time while updating BPLxPTL" */
   /* fixes "3v Demo" by Cave and "New Year Demo" by Phoenix */
   if (is_bitplane_dma(hpos - 1) == num + 1 && num > 0)
      delta = 2 << fetchmode;
   bplpt[num] = (bplpt[num] & ~0xffff) | ((v + delta) & 0xfffe);
}

static void BPLCON0 (int hpos, uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
		v &= ~0x00F1;
	else if (! (currprefs.chipset_mask & CSMASK_AGA))
		v &= ~0x00B1;
 	
	if (bplcon0 == v)
		return;
	
  ddf_change = vpos;
	decide_line (hpos);
	decide_fetch (hpos);
	
	bplcon0 = v;
  if((((v) & 0x8000) ? RES_HIRES : ((v) & 0x40) ? RES_SUPERHIRES : RES_LORES) == RES_SUPERHIRES)
	{
	  printf("RES_SUPERHIRES not supported...\n");
	  abort();
	}
	res_bplcon0=GET_RES(v);
	planes_bplcon0=GET_PLANES(v);
	planes_limit_bplcon0 = GET_PLANES_LIMIT();
	curr_diagram = cycle_diagram_table[fetchmode][res_bplcon0][planes_limit_bplcon0];
  record_register_change (hpos, 0x100, v);

  if (currprefs.chipset_mask & CSMASK_ECS_DENISE) {
  	decide_sprites (hpos);
	  sprres = expand_sprres (bplcon0, bplcon3);
  }

  expand_fmodes ();
  calcdiw ();
  estimate_last_fetch_cycle (hpos);
}

STATIC_INLINE void BPLCON1 (int hpos, uae_u16 v)
{
  if (!(currprefs.chipset_mask & CSMASK_AGA))
	  v &= 0xff;
	if (bplcon1 == v)
		return;
   ddf_change = vpos;
	decide_line (hpos);
	decide_fetch (hpos);
	bplcon1 = v;
}

STATIC_INLINE void BPLCON2 (int hpos, uae_u16 v)
{
  if (!(currprefs.chipset_mask & CSMASK_AGA))
	  v &= 0x7f;
	if (bplcon2 == v)
		return;
	decide_line (hpos);
	bplcon2 = v;
  record_register_change (hpos, 0x104, v);
}

STATIC_INLINE void BPLCON3 (int hpos, uae_u16 v)
{
    if (! (currprefs.chipset_mask & CSMASK_ECS_DENISE))
	return;
    if (!(currprefs.chipset_mask & CSMASK_AGA)) {
	v &= 0x3f;
	v |= 0x0c00;
    }
	if (bplcon3 == v)
		return;
	decide_line (hpos);
	decide_sprites (hpos);
	bplcon3 = v;
    sprres = expand_sprres (bplcon0, bplcon3);
  record_register_change (hpos, 0x106, v);
}

STATIC_INLINE void BPLCON4 (int hpos, uae_u16 v)
{
	if (! (currprefs.chipset_mask & CSMASK_AGA))
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
   if ((uae_s16)bpl1mod == (uae_s16)v)
      return;
   decide_line (hpos);
   decide_fetch (hpos);
   bpl1mod = v;
}

static void BPL2MOD (int hpos, uae_u16 v)
{
   v &= ~1;
   if ((uae_s16)bpl2mod == (uae_s16)v)
      return;
   decide_line (hpos);
   decide_fetch (hpos);
   bpl2mod = v;
}

STATIC_INLINE void BPL1DAT (int hpos, uae_u16 v)
{
    decide_line (hpos);
    bpl1dat = v;

    maybe_first_bpl1dat (hpos);
}

static void DIWSTRT (int hpos, uae_u16 v)
{
  if (diwstrt == v && ! diwhigh_written)
		return;
	decide_line (hpos);
	diwhigh_written = 0;
	diwstrt = v;
	calcdiw ();
}

static void DIWSTOP (int hpos, uae_u16 v)
{
  if (diwstop == v && ! diwhigh_written)
		return;
	decide_line (hpos);
	diwhigh_written = 0;
	diwstop = v;
	calcdiw ();
}

static void DIWHIGH (int hpos, uae_u16 v)
{
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
	if (ddfstrt == v && hpos != plfstrt - 2)
		return;
  ddf_change = vpos;
	decide_line (hpos);
  ddfstrt_old_hpos = hpos;
  ddfstrt_old_vpos = vpos;
	ddfstrt = v;
	calcdiw ();
}

static void DDFSTOP (int hpos, uae_u16 v)
{
	v &= 0xfe;
	if (ddfstop == v)
		return;
	decide_line (hpos);
	decide_fetch (hpos);
	ddfstop = v;
	calcdiw ();
	if (fetch_state != fetch_not_started)
		estimate_last_fetch_cycle (hpos);
}

static void FMODE (uae_u16 v)
{
    if (!(currprefs.chipset_mask & CSMASK_AGA))
    	v = 0;
    
    ddf_change = vpos;
    fmode = v;
    sprite_width = GET_SPRITEWIDTH (fmode);
    switch (fmode & 3) {
    	case 0:
    		fetchmode = 0;
    		delaymask = 15; // (16 << fetchmode) - 1;
    		break;
    	case 1:
    	case 2:
    		fetchmode = 1;
    		delaymask = 31; // (16 << fetchmode) - 1;
    		break;
    	case 3:
    		fetchmode = 2;
    		delaymask = 63; // (16 << fetchmode) - 1;
    		break;
    }
    planes_limit_bplcon0 = GET_PLANES_LIMIT();
    curr_diagram = cycle_diagram_table[fetchmode][GET_RES (v)][planes_limit_bplcon0];
    expand_fmodes ();
    calcdiw ();
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

    if (bltcon1 & 2)
	blt_info.bltbhold = v << (bltcon1 >> 12);
    else
	blt_info.bltbhold = v >> (bltcon1 >> 12);
    blt_info.bltbdat = v;
}

#define BLTCDAT(V) maybe_blit (0); blt_info.bltcdat = V
#define BLTAMOD(V) maybe_blit (1); blt_info.bltamod = (uae_s16)(V & 0xFFFE)
#define BLTBMOD(V) maybe_blit (1); blt_info.bltbmod = (uae_s16)(V & 0xFFFE)
#define BLTCMOD(V) maybe_blit (1); blt_info.bltcmod = (uae_s16)(V & 0xFFFE)
#define BLTDMOD(V) maybe_blit (1); blt_info.bltdmod = (uae_s16)(V & 0xFFFE)
#define BLTCON0(V) maybe_blit (0); bltcon0 = V; blinea_shift = V >> 12

/* The next category is "Most useless hardware register".
 * And the winner is... */
static void BLTCON0L (uae_u16 v)
{
//	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
//		return;
	maybe_blit (0); 
	bltcon0 = (bltcon0 & 0xFF00) | (v & 0xFF);
	blinea_shift = bltcon0 >> 12;
}

#define BLTCON1(V) maybe_blit (0); bltcon1 = V; blitsign = bltcon1 & 0x40;
#define BLTAFWM(V) maybe_blit (0); blt_info.bltafwm = V
#define BLTALWM(V) maybe_blit (0); blt_info.bltalwm = V
#define BLTAPTH(V) maybe_blit (0); bltapt = (bltapt & 0xffff) | ((uae_u32)V << 16)
#define BLTAPTL(V) maybe_blit (0); bltapt = (bltapt & ~0xffff) | (V & 0xFFFE)
#define BLTBPTH(V) maybe_blit (0); bltbpt = (bltbpt & 0xffff) | ((uae_u32)V << 16)
#define BLTBPTL(V) maybe_blit (0); bltbpt = (bltbpt & ~0xffff) | (V & 0xFFFE)
#define BLTCPTH(V) maybe_blit (0); bltcpt = (bltcpt & 0xffff) | ((uae_u32)V << 16)
#define BLTCPTL(V) maybe_blit (0); bltcpt = (bltcpt & ~0xffff) | (V & 0xFFFE)
#define BLTDPTH(V) maybe_blit (0); bltdpt = (bltdpt & 0xffff) | ((uae_u32)V << 16)
#define BLTDPTL(V) maybe_blit (0); bltdpt = (bltdpt & ~0xffff) | (V & 0xFFFE)

static void BLTSIZE (uae_u16 v)
{
    maybe_blit (0);

    blt_info.vblitsize = v >> 6;
    blt_info.hblitsize = v & 0x3F;
    if (!blt_info.vblitsize) 
      blt_info.vblitsize = 1024;
    if (!blt_info.hblitsize) 
      blt_info.hblitsize = 64;

    do_blitter ();
}

static void BLTSIZV (uae_u16 v)
{
//	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
//		return;
	maybe_blit (0);
	blt_info.vblitsize = v & 0x7FFF;
}

static void BLTSIZH (uae_u16 v)
{
//	if (! (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
//		return;
	maybe_blit (0);
	blt_info.hblitsize = v & 0x7FF;
	if (!blt_info.vblitsize) 
	  blt_info.vblitsize = 32768;
	if (!blt_info.hblitsize) 
	  blt_info.hblitsize = 0x800;
	do_blitter ();
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
    if (currprefs.chipset_mask & CSMASK_ECS_DENISE) {
	    sprxp |= (sprctl[num] >> 4) & 1;
    }
    s->xpos = sprxp;
    s->vstart = (sprpos[num] >> 8) | ((sprctl[num] << 6) & 0x100);
    s->vstop = (sprctl[num] >> 8) | ((sprctl[num] << 7) & 0x100);
    if (currprefs.chipset_mask & CSMASK_ECS_AGNUS) {
	    s->vstart |= (sprctl[num] << 3) & 0x200;
	    s->vstop |= (sprctl[num] << 4) & 0x200;
    }
    sprstartstop (s);
}

STATIC_INLINE void SPRxCTL_1 (uae_u16 v, int num, int hpos)
{
    struct sprite *s = &spr[num];
    sprctl[num] = v;
    spr_arm (num, 0);
    SPRxCTLPOS (num);
}
STATIC_INLINE void SPRxPOS_1 (uae_u16 v, int num, int hpos)
{
    struct sprite *s = &spr[num];
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
#define SPRxDATA(HPOS,V,NUM) decide_sprites (HPOS); SPRxDATA_1 (V, NUM)
#define SPRxDATB(HPOS,V,NUM) decide_sprites (HPOS); SPRxDATB_1 (V, NUM)
#define SPRxCTL(HPOS,V,NUM) decide_sprites (HPOS); SPRxCTL_1 (V, NUM, HPOS)
#define SPRxPOS(HPOS,V,NUM) decide_sprites (HPOS); SPRxPOS_1 (V, NUM, HPOS)

static void SPRxPTH (int hpos, uae_u16 v, int num)
{
    decide_sprites (hpos);
    spr[num].pt &= 0xffff;
    spr[num].pt |= (uae_u32)v << 16;
}
static void SPRxPTL (int hpos, uae_u16 v, int num)
{
    decide_sprites (hpos);
    spr[num].pt &= ~0xffff;
    spr[num].pt |= v;
}

static void CLXCON (uae_u16 v)
{
    clxcon = v;
    clxcon_bpl_enable = (v >> 6) & 63;
    clxcon_bpl_match = v & 63;
}

static void CLXCON2 (uae_u16 v)
{
	if (!(currprefs.chipset_mask & CSMASK_AGA))
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
	
	if (!(currprefs.chipset_mask & CSMASK_AGA) || !(bplcon2 & 0x0100))
		return 0xffff;
	
	colreg = ((bplcon3 >> 13) & 7) * 32 + num;
	cr = current_colors.color_regs_aga[colreg] >> 16;
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
	v &= 0xFFF;
	if (currprefs.chipset_mask & CSMASK_AGA) {
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
		cr = current_colors.color_regs_aga[colreg] >> 16;
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
   if (hpos + 1 >= maxhpos)
      return 1;
   return is_bitplane_dma (hpos);
}

STATIC_INLINE int dangerous_reg (int reg)
{
	/* Safe:
	 * Bitplane pointers, control registers, modulos and data.
	 * Sprite pointers, control registers, and data.
	 * Color registers.  */
  if (reg >= 0xE0 && reg < 0x1C0)
		return 0;
	return 1;
}

#ifdef FAST_COPPER
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
	
	switch (state) {
		case COP_read1_wr_in2:
		case COP_read2_wr_in2:
		case COP_read1_wr_in4:
			if (dangerous_reg (cop_state.saved_i1))
				return;
			state = state == COP_read2_wr_in2 ? COP_read2 : COP_read1;
			break;
			
		case COP_read1_in2:
			c_hpos += 2;
			state = COP_read1;
			break;
			
		case COP_stop:
		case COP_bltwait:
		case COP_wait1:
		case COP_skip_in4:
		case COP_skip_in2:
    case COP_skip1:
    case COP_strobe_delay:
			return;
			
		case COP_wait_in4:
			c_hpos += 2;
			/* fallthrough */
		case COP_wait_in2:
			c_hpos += 2;
			/* fallthrough */
		case COP_wait:
			state = COP_wait;
			break;
			
		default:
			break;
	}
	/* Only needed for COP_wait, but let's shut up the compiler.  */
	w1 = cop_state.saved_i1;
	w2 = cop_state.saved_i2;
	cop_state.first_sync = c_hpos;
	cop_state.regtypes_modified = REGTYPE_FORCE;
	
	/* Get this case out of the way, so that the loop below only has to deal
       with read1 and wait.  */
	if (state == COP_read2) {
		w1 = cop_state.i1;
		if (w1 & 1) {
			w2 = CHIPMEM_AGNUS_WGET_CUSTOM (ip);
			if (w2 & 1)
				goto done;
			state = COP_wait;
			c_hpos += 4;
		} else if (dangerous_reg (w1)) {
			c_hpos += 4;
			goto done;
		} else {
			cop_state.regtypes_modified |= regtypes[w1 & 0x1FE];
			state = COP_read1;
			c_hpos += 2;
		}
		ip += 2;	
	}
	
	while (c_hpos + 1 < maxhpos) {
		if (state == COP_read1) {
			w1 = CHIPMEM_AGNUS_WGET_CUSTOM (ip);
			if (w1 & 1) {
				w2 = CHIPMEM_AGNUS_WGET_CUSTOM (ip + 2);
				if (w2 & 1)
					break;
				state = COP_wait;
				c_hpos += 6;
			} else if (dangerous_reg (w1)) {
				c_hpos += 6;
				break;
			} else {
				cop_state.regtypes_modified |= regtypes[w1 & 0x1FE];
				c_hpos += 4;
			}
			ip += 4;
		} else if (state == COP_wait) {
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
					c_hpos = hcmp;
				}
				/* If this is the current instruction, remember that we don't
		   need to sync CPU and copper anytime soon.  */
				if (cop_state.ip == ip) {
					cop_state.first_sync = c_hpos;
				}
			}
		} else
			return;
	}
	
	done:
	cycle_count = c_hpos - cop_state.hpos;
	if (cycle_count >= 8) {
		unset_special (&regs, SPCFLAG_COPPER);
		eventtab[ev_copper].active = 1;
		eventtab[ev_copper].oldcycles = get_cycles ();
		eventtab[ev_copper].evtime = get_cycles () + cycle_count * CYCLE_UNIT;
		events_schedule ();
	}
}
#endif

static __inline__ void perform_copper_write (int old_hpos)
{
	unsigned int address = cop_state.saved_i1 & 0x1FE;
	
	if (test_copper_dangerous(address))
		return;
	
	if (address == 0x88) {
		cop_state.ip = cop1lc;
	  cop_state.state = COP_strobe_delay;
	} else if (address == 0x8A) {
		cop_state.ip = cop2lc;
	  cop_state.state = COP_strobe_delay;
	} else {
		custom_wput_1 (old_hpos, cop_state.saved_i1, cop_state.saved_i2, 0);
	  old_hpos++;
	  if (!nocustom() && cop_state.saved_i1 >= 0x140 && cop_state.saved_i1 < 0x180 && old_hpos >= SPR0_HPOS && old_hpos < SPR0_HPOS + 4 * MAX_SPRITES) {
	    //write_log ("%d:%d %04.4X:%04.4X\n", vpos, old_hpos, cop_state.saved_i1, cop_state.saved_i2);
	    do_sprites (old_hpos);
  	}
  }
}

static int isagnus[]= {
    1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0, /* 32 0x00 - 0x3e */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 27 0x40 - 0x74 */
    0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0, /* 21 */
    1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0, /* 32 0xa0 - 0xde */
    /* BPLxPTH/BPLxPTL */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 16 */
    /* BPLCON0-3,BPLMOD1-2 */
    0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0, /* 16 */
    /* SPRxPTH/SPRxPTL */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 16 */
    /* SPRxPOS/SPRxCTL/SPRxDATA/SPRxDATB */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    /* COLORxx */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* RESERVED */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static void update_copper (int until_hpos)
{
	int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);    
	int c_hpos = cop_state.hpos;
	
    if (nocustom()) {
#ifdef FAST_COPPER
	eventtab[ev_copper].active = 0;
#endif
	return;
    }

#ifdef FAST_COPPER
	if (eventtab[ev_copper].active) {
    eventtab[ev_copper].active = 0;
		return;
  }
#endif
	
  if (cop_state.state == COP_wait && vp < cop_state.vcmp) {
#ifdef FAST_COPPER
  	eventtab[ev_copper].active = 0;
#endif
	 	copper_enabled_thisline = 0;
	  cop_state.state = COP_stop;
	  unset_special(&regs, SPCFLAG_COPPER);
		return;
	}
	
	until_hpos &= ~1;
	
	if (until_hpos > (maxhpos & ~1))
		until_hpos = maxhpos & ~1;
	
	until_hpos += 2;
	for (;;) {
		int old_hpos = c_hpos;
		int hp;
		
		if (c_hpos >= until_hpos)
			break;
		
		/* So we know about the fetch state.  */
		decide_line (c_hpos);

		switch (cop_state.state) {
			case COP_read1_in2:
				cop_state.state = COP_read1;
				break;
			case COP_read1_wr_in2:
				cop_state.state = COP_read1;
				perform_copper_write (old_hpos);
				/* That could have turned off the copper.  */
				if (! copper_enabled_thisline)
					goto out;
				
				break;
			case COP_read1_wr_in4:
				cop_state.state = COP_read1_wr_in2;
				break;
			case COP_read2_wr_in2:
				cop_state.state = COP_read2;
				perform_copper_write (old_hpos);
				/* That could have turned off the copper.  */
				if (! copper_enabled_thisline)
					goto out;
				
				break;
			case COP_wait_in2:
				cop_state.state = COP_wait1;
				break;
			case COP_wait_in4:
				cop_state.state = COP_wait_in2;
				break;
			case COP_skip_in2:
	      cop_state.state = COP_skip1;
			  break;
			case COP_skip_in4:
				cop_state.state = COP_skip_in2;
				break;
      case COP_strobe_delay:
	      cop_state.state = COP_read1_in2;
				break;
			default:
				break;
		}
		
		c_hpos += 2;
    if (cop_state.strobe) {
      cop_state.ip = cop_state.strobe == 1 ? cop1lc : cop2lc;
	    cop_state.strobe = 0;
	  }

		switch (cop_state.state) {
			case COP_read1_wr_in4:
				return;
				
			case COP_read1_wr_in2:
			case COP_read1:
  	    if (copper_cant_read (old_hpos))
      		continue;
				cop_state.i1 = CHIPMEM_AGNUS_WGET_CUSTOM (cop_state.ip);
				cop_state.ip += 2;
				cop_state.state = cop_state.state == COP_read1 ? COP_read2 : COP_read2_wr_in2;
				break;
				
			case COP_read2_wr_in2:
				return;
				
			case COP_read2:
  	    if (copper_cant_read (old_hpos))
      		continue;
				cop_state.i2 = CHIPMEM_AGNUS_WGET_CUSTOM (cop_state.ip);
				cop_state.ip += 2;
				if (cop_state.ignore_next) {
					cop_state.ignore_next = 0;
					cop_state.state = COP_read1;
					break;
				}
				
				cop_state.saved_i1 = cop_state.i1;
				cop_state.saved_i2 = cop_state.i2;
				cop_state.saved_ip = cop_state.ip;
				
				if (cop_state.i1 & 1) {
					if (cop_state.i2 & 1)
						cop_state.state = COP_skip_in4;
					else
						cop_state.state = COP_wait_in4;
				} else {
					unsigned int reg = cop_state.i1 & 0x1FE;
					cop_state.state = isagnus[reg >> 1] ? COP_read1_wr_in2 : COP_read1_wr_in4;
				}
				break;
				
			case COP_wait1:
				/* There's a nasty case here.  As stated in the "Theory" comment above, we
			   test against the incremented copper position.  I believe this means that
			   we have to increment the _vertical_ position at the last cycle in the line,
			   and set the horizontal position to 0.
			   Normally, this isn't going to make a difference, since we consider these
			   last cycles unavailable for the copper, so waking up in the last cycle has
			   the same effect as waking up at the start of the line.  However, there is
			   one possible problem:  If we're at 0xFFE0, any wait for an earlier position
			   must _not_ complete (since, in effect, the current position will be back
			   at 0/0).  This can be seen in the Superfrog copper list.
			   Things get monstrously complicated if we try to handle this "properly" by
			   incrementing vpos and setting c_hpos to 0.  Especially the various speedup
			   hacks really assume that vpos remains constant during one line.  Hence,
			   this hack: defer the entire decision until the next line if necessary.  */
				if (c_hpos >= (maxhpos & ~1))
					break;
				cop_state.state = COP_wait;
				
				cop_state.vcmp = (cop_state.saved_i1 & (cop_state.saved_i2 | 0x8000)) >> 8;
				cop_state.hcmp = (cop_state.saved_i1 & cop_state.saved_i2 & 0xFE);
				
				vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
				
	      if (cop_state.saved_i1 == 0xFFFF && cop_state.saved_i2 == 0xFFFE) {
					cop_state.state = COP_stop;
					copper_enabled_thisline = 0;
					unset_special (&regs, SPCFLAG_COPPER);
					goto out;
				}
				if (vp < cop_state.vcmp) {
					copper_enabled_thisline = 0;
					unset_special (&regs, SPCFLAG_COPPER);
					goto out;
				}
				
				/* fall through */
			case COP_wait:
				if (vp < cop_state.vcmp)
					return;
  	    if (copper_cant_read (old_hpos))
      		continue;
				
				hp = c_hpos & (cop_state.saved_i2 & 0xFE);
	      if (vp == cop_state.vcmp && hp < cop_state.hcmp) {
					/* Position not reached yet.  */
#ifdef FAST_COPPER
					if ((cop_state.saved_i2 & 0xFE) == 0xFE) {
						int wait_finish = cop_state.hcmp - 2;
						/* This will leave c_hpos untouched if it's equal to wait_finish.  */
						if (wait_finish < c_hpos)
							return;
						else if (wait_finish <= until_hpos) {
							c_hpos = wait_finish;
						} else
							c_hpos = until_hpos;
					}	      
#endif
					break;
				}
				
				/* Now we know that the comparisons were successful.  We might still
			   have to wait for the blitter though.  */
	      if ((cop_state.saved_i2 & 0x8000) == 0 && (DMACONR() & 0x4000)) {
					/* We need to wait for the blitter.  */
					cop_state.state = COP_bltwait;
					copper_enabled_thisline = 0;
					unset_special (&regs, SPCFLAG_COPPER);
					goto out;
				}
				
				cop_state.state = COP_read1;
				break;
				
	  case COP_skip1:
	    {
	      unsigned int vcmp, hcmp, vp1, hp1;

	      if (c_hpos >= (maxhpos & ~1))
      		break;

	      vcmp = (cop_state.saved_i1 & (cop_state.saved_i2 | 0x8000)) >> 8;
	      hcmp = (cop_state.saved_i1 & cop_state.saved_i2 & 0xFE);
	      vp1 = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
	      hp1 = c_hpos & (cop_state.saved_i2 & 0xFE);

	      if ((vp1 > vcmp || (vp1 == vcmp && hp1 >= hcmp))
		    && ((cop_state.saved_i2 & 0x8000) != 0 || ! (DMACONR() & 0x4000)))
		      cop_state.ignore_next = 1;
	      if (CHIPMEM_AGNUS_WGET_CUSTOM (cop_state.ip) & 1) { /* FIXME: HACK!!! */
		      /* copper never skips if following instruction is WAIT or another SKIP... */
		      cop_state.ignore_next = 0;
	      }

	      cop_state.state = COP_read1;

	      if (cop_state.ignore_next && (CHIPMEM_AGNUS_WGET_CUSTOM (cop_state.ip) & 1) == 0) {
		      /* another undocumented copper feature:
		      copper stops if skipped instruction is MOVE to dangerous register...
		      */
		      test_copper_dangerous (CHIPMEM_AGNUS_WGET_CUSTOM(cop_state.ip));
	      }

	      break;
	    }
			default:
				break;
		}
	}
	
	out:
	cop_state.hpos = c_hpos;
	
#ifdef FAST_COPPER
    /* The test against maxhpos also prevents us from calling predict_copper
       when we are being called from hsync_handler, which would not only be
       stupid, but actively harmful.  */
	if ((regs.spcflags & SPCFLAG_COPPER) && (c_hpos + 8 < maxhpos))
		predict_copper ();
#endif
}

static void compute_spcflag_copper (void)
{
	copper_enabled_thisline = 0;
	unset_special (&regs, SPCFLAG_COPPER);
  if (!dmaen (DMA_COPPER) || cop_state.state == COP_stop || cop_state.state == COP_bltwait || nocustom())
		return;
	
	if (cop_state.state == COP_wait) {
		int vp = vpos & (((cop_state.saved_i2 >> 8) & 0x7F) | 0x80);
		
		if (vp < cop_state.vcmp)
			return;
	}
	copper_enabled_thisline = 1;
	
#ifdef FAST_COPPER
	predict_copper ();
	if (! eventtab[ev_copper].active)
#endif
	  set_special (&regs, SPCFLAG_COPPER);
}

#ifdef FAST_COPPER
static void copper_handler (void)
{
  /* This will take effect immediately, within the same cycle.  */
  set_special (&regs, SPCFLAG_COPPER);
	
	if (! copper_enabled_thisline)
		return;
	
	eventtab[ev_copper].active = 0;
}
#endif

void blitter_done_notify (void)
{
	if (cop_state.state != COP_bltwait)
		return;
	
	cop_state.hpos = current_hpos () & ~1;
	cop_state.vpos = vpos;
  /* apparently there is small delay until copper wakes up.. */
  cop_state.state = COP_wait_in2;
	compute_spcflag_copper ();
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
#ifdef FAST_COPPER
STATIC_INLINE void sync_copper_with_cpu (int hpos, int do_schedule, unsigned int addr)
#else
STATIC_INLINE void sync_copper_with_cpu (int hpos, int do_schedule)
#endif
{
#ifdef FAST_COPPER
	/* Need to let the copper advance to the current position.  */
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
	  set_special (&regs, SPCFLAG_COPPER);
	}
#endif
	if (copper_enabled_thisline)
		update_copper (hpos);
}

STATIC_INLINE uae_u16 sprite_fetch (struct sprite *s, int dma)
{
    if (dma)
	    last_custom_value = CHIPMEM_AGNUS_WGET_CUSTOM (s->pt);
    s->pt += 2;
    return last_custom_value;
}

STATIC_INLINE void do_sprites_1 (int num, int cycle, int hpos)
{
	struct sprite *s = &spr[num];
    int dma, posctl = 0;
    uae_u16 data;

    if (vpos == sprite_vblank_endline)
	    spr_arm (num, 0);
	
    if (vpos == s->vstart) {
        s->dmastate = 1;
    }
    if (vpos == s->vstop || vpos == sprite_vblank_endline) {
        s->dmastate = 0;
    }
	if (!dmaen (DMA_SPRITE))
		return;
  if (cycle && !s->dmacycle)
    return; /* Superfrog intro flashing bee fix */

  dma = hpos < plfstrt || diwstate != DIW_waiting_stop;
  if (vpos == s->vstop || vpos == sprite_vblank_endline) {
	  s->dmastate = 0;
	  posctl = 1;
    if (dma) {
	    data = sprite_fetch (s, dma);
      // instead of switch: s->pt += (sprite_width >> 3) - 2;
      // but then, last_custom_value not set to correct value
	    switch (sprite_width)
	    {
    		case 64:
    		sprite_fetch (s, dma);
    		sprite_fetch (s, dma);
    		case 32:
    		sprite_fetch (s, dma);
    		break;
	    }
		} else {
      data = cycle == 0 ? sprpos[num] : sprctl[num];
		}
		  if (cycle == 0) {
			  SPRxPOS_1 (data, num, hpos);
	      s->dmacycle = 1;
	    } else {
			  SPRxCTL_1 (data, num, hpos);
  	    s->dmastate = 0;
	      sprstartstop (s);
	    }
	}

  if (s->dmastate && !posctl) {
    uae_u16 data;

    data = sprite_fetch (s, dma);
	  /* Hack for X mouse auto-calibration */
    if (num == 0 && cycle == 0)
		  mousehack_handle (sprctl[0], sprpos[0]);
    if (cycle == 0) {
	    SPRxDATA_1 (dma ? data : sprdata[num][0], num);
      s->dmacycle = 1;
    } else {
	    SPRxDATB_1 (dma ? data : sprdatb[num][0], num);
      spr_arm (num, 1);
    }
		switch (sprite_width)
		{
			case 64:
			{
		uae_u16 data32 = sprite_fetch (s, dma);
		uae_u16 data641 = sprite_fetch (s, dma);
		uae_u16 data642 = sprite_fetch (s, dma);
		if (dma) {
				if (cycle == 0) {
					sprdata[num][3] = data642;
					sprdata[num][2] = data641;
					sprdata[num][1] = data32;
				} else {
					sprdatb[num][3] = data642;
					sprdatb[num][2] = data641;
					sprdatb[num][1] = data32;
				}
		}
	    }
			break;
			case 32:
			{	
		uae_u16 data32 = sprite_fetch (s, dma);
		if (dma) {
				if (cycle == 0)
					sprdata[num][1] = data32;
				else
					sprdatb[num][1] = data32;
		}
			}
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
    minspr = last_sprite_hpos;
    
    if (minspr >= SPR0_HPOS + MAX_SPRITES * 4 || maxspr < SPR0_HPOS)
    	return;
    
    if (maxspr > SPR0_HPOS + MAX_SPRITES * 4)
    	maxspr = SPR0_HPOS + MAX_SPRITES * 4;
    if (minspr < SPR0_HPOS)
    	minspr = SPR0_HPOS;
    
    for (i = minspr; i < maxspr; i++) {
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
	    if (cycle >= 0)
	      do_sprites_1 (num, cycle, i);
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
    diwstate = DIW_waiting_start;
    hdiwstate = DIW_waiting_start;
    ddfstate = DIW_waiting_start;
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
  int v = read_processor_time () - vsyncmintime;
  if (v > (int)syncbase || v < -((int)syncbase)) {
	  vsyncmintime = read_processor_time();
	  v = 0;
  }
  return v;
}

static void framewait (void)
{
  frame_time_t curr_time;
  frame_time_t start;

  for (;;) {
	  double v = rpt_vsync () / (syncbase / 1000.0);
	  if (v >= -4)
	    break;
	  sleep_millis (2);
  }
  curr_time = start = read_processor_time();
//    if (!isvsync()) {
	while (rpt_vsync () < 0);
    curr_time = read_processor_time ();
//    }
  vsyncmintime = curr_time + vsynctime;
//    idletime += read_processor_time() - start;
}

static frame_time_t frametime2;

void fpscounter_reset (void)
{
    timeframes = 0;
    frametime2 = 0;
    lastframetime = read_processor_time ();
}

static void fpscounter (void)
{
    frame_time_t now, last;
    int mcnt = 25;

    now = read_processor_time ();
    last = now - lastframetime;
    lastframetime = now;

    frametime2 += last;
    timeframes++;
    if ((timeframes % mcnt) == 0) {
    	int fps = frametime2 == 0 ? 0 : (syncbase * mcnt) / (frametime2 / 10);
      if(currprefs.gfx_framerate > 0)
        fps = fps / (currprefs.gfx_framerate + 1);
    	if (fps > 9999)
    	    fps = 9999;
    	gui_data.fps = (fps + 5) / 10;
    	frametime2 = 0;
    }
}

static int vsync_handler_cnt_disk_change=0;

static void vsync_handler (void)
{
  fpscounter();

#ifdef JIT
	if (!compiled_code) {
#endif
	  if (currprefs.m68k_speed == -1) {
		  frame_time_t curr_time = read_processor_time ();
		  /* @@@ Mathias? How do you think we should do this? */
		  /* If we are too far behind, or we just did a reset, adjust the
		   * needed time. */
		  if (rpt_did_reset)
		    vsyncmintime = curr_time + vsynctime;
		    rpt_did_reset = 0;
	  } else {
//		  framewait ();
	  }
#ifdef JIT
	} else {
//	  if (currprefs.m68k_speed >= 0) {
//		framewait ();
//	  }
	}
#endif

	handle_events ();
	
  INTREQ_d (0x8000 | 0x0020, 3);
	if (bplcon0 & 4)
		lof ^= 0x8000;
	
#ifdef PICASSO96
  /* And now let's update the Picasso palette, if required */
  if (picasso_on)
	  picasso_handle_vsync ();
#endif

	if (quit_program > 0) {
	  /* prevent possible infinite loop at wait_cycles().. */
		framecnt = 0;
    reset_decisions ();
	  return;
  }
	
	if (vsync_handler_cnt_disk_change == 0) {
		/* resolution_check_change (); */
		DISK_check_change ();
		vsync_handler_cnt_disk_change = 5; //20;
	}
	vsync_handler_cnt_disk_change--;
	
	vsync_handle_redraw (lof, lof_changed);

#ifdef JIT
  if (compiled_code) {
    vsyncmintime = last_synctime;
    frh_count = 0;
    speedup_cycles = 3000 * CYCLE_UNIT;
    speedup_timelimit = -1000; // 1 ms for JIT
  }
  else
#endif
  {
    vsyncmintime = last_synctime + vsynctime * (maxvpos - LAST_SPEEDUP_LINE) / maxvpos;
    speedup_cycles = 500 * CYCLE_UNIT;
    speedup_timelimit = -2000; // 2 ms for non JIT
  }

/* For now, let's only allow this to change at vsync time.  It gets too
	 * hairy otherwise.  */
	if ((beamcon0 & (0x20|0x80)) != (new_beamcon0 & (0x20|0x80)) || hack_vpos)
		init_hz ();
	
	lof_changed = 0;
	
#ifdef FAST_COPPER
  eventtab[ev_copper].active = 0;
#endif
  COPJMP (1);
	
  if (timehack_alive > 0)
	  timehack_alive--;
  inputdevice_vsync ();
  filesys_vsync ();
	
	init_hardware_frame ();
}

#ifdef JIT

#define N_LINES 8

static __inline__ int trigger_frh(int v)
{
    return (v & (N_LINES - 1)) == 0;
}

long int diff32(frame_time_t x, frame_time_t y)
{
    return (long int)(x-y);
}
static void frh_handler(void)
{
    if (currprefs.m68k_speed == -1) {
	frame_time_t curr_time = read_processor_time ();
	vsyncmintime += vsynctime * N_LINES / maxvpos;
	/* @@@ Mathias? How do you think we should do this? */
	/* If we are too far behind, or we just did a reset, adjust the
	 * needed time. */
	if (rpt_did_reset) {
	    vsyncmintime = curr_time + vsynctime;
	    rpt_did_reset = 0;
	}
	/* Allow this to be one frame's worth of cycles out */
	while (diff32 (curr_time, vsyncmintime + vsynctime) > 0) {
	    vsyncmintime += vsynctime * N_LINES / maxvpos;
	}
    }
}
#endif

static void CIA_vsync_prehandler(void)
{
    CIA_vsync_handler ();
    ciavsync_counter++;
}

static void hsync_handler (void)
{
  if (!nocustom()) {
#ifdef FAST_COPPER
   /* Using 0x8A makes sure that we don't accidentally trip over the
       modified_regtypes check.  */
    sync_copper_with_cpu (maxhpos, 0, 0x8A);
#else
    sync_copper_with_cpu (maxhpos, 0);
#endif
   
    if(currprefs.pandora_partial_blits && bltstate == BLT_work)
      blitter_do_partial(0);

    finish_decisions ();
    if (thisline_decision.plfleft != -1) {
      if (currprefs.collision_level > 1)
        DO_SPRITE_COLLISIONS
      if (currprefs.collision_level > 2)
        DO_PLAYFIELD_COLLISIONS
    }
   
    hsync_record_line_state (next_lineno);
  }
   
  eventtab[ev_hsync].evtime += get_cycles () - eventtab[ev_hsync].oldcycles;
  eventtab[ev_hsync].oldcycles = get_cycles ();
  CIA_hsync_handler ();
   
#ifdef PICASSO96
  picasso_handle_hsync ();
#endif

// Slows down emulation without improving compatability a lot
//    if ((currprefs.chipset_mask & CSMASK_AGA) || (!currprefs.chipset_mask & CSMASK_ECS_AGNUS))
//	    last_custom_value = uaerand ();
//    else
//	    last_custom_value = 0xffff;
 
  if (currprefs.produce_sound)
  	audio_hsync (1);

/* blitter_slowdown doesn't work at the moment (causes gfx glitches in Shadow of the Beast)
	 if (bltstate != BLT_done && dmaen (DMA_BITPLANE) && diwstate == DIW_waiting_stop) {
		  blitter_slowdown (ddfstrt, ddfstop,
		   cycle_diagram_total_cycles[fetchmode][res_bplcon0][planes_limit_bplcon0],
		   cycle_diagram_free_cycles[fetchmode][res_bplcon0][planes_limit_bplcon0]);
	 }
*/

  /* In theory only an equality test is needed here - but if a program
       goes haywire with the VPOSW register, it can cause us to miss this,
       with vpos going into the thousands (and all the nasty consequences
       this has).  */
   
   vpos++;
   if (vpos  >= (maxvpos + (lof == 0 ? 0 : 1)))
   {
      vpos=0;
      vsync_handler ();
	    vsync_counter++;
      CIA_vsync_prehandler();
   }
   
   DISK_hsync (maxhpos);
   
#ifdef JIT
  if (compiled_code) {
	  if (currprefs.m68k_speed == -1) {
	    frh_count++;
	    if (trigger_frh(frh_count) && vpos < maxvpos - LAST_SPEEDUP_LINE) {
		    frh_handler();
	    }
	    is_lastline = trigger_frh(frh_count+1) && vpos < maxvpos - LAST_SPEEDUP_LINE + 1 && ! rpt_did_reset;
	  } else {
	    is_lastline=0;
	  }
  } else {
#endif
	  is_lastline = vpos + 1 == maxvpos + (lof == 0 ? 0 : 1) - LAST_SPEEDUP_LINE && currprefs.m68k_speed == -1 && ! rpt_did_reset;
#ifdef JIT
    }
#endif

  if (!nocustom()) {
      int lineno = vpos;
      next_lineno = lineno;
      reset_decisions ();
  }

   if (uae_int_requested) {
	    INTREQ (0x8000 | 0x0008);
   }

   /* See if there's a chance of a copper wait ending this line.  */
   cop_state.hpos = 0;
   compute_spcflag_copper ();

   inputdevice_hsync ();

   hsync_counter++;
}

#ifdef FAST_COPPER
static void init_regtypes (void)
{
    int i;
    for (i = 0; i < 512; i += 2) {
	regtypes[i] = REGTYPE_ALL;
	if ((i >= 0x20 && i < 0x28) || i == 0x08 || i == 0x7E)
	    regtypes[i] = REGTYPE_DISK;
	else if (i >= 0x68 && i < 0x70)
	    regtypes[i] = REGTYPE_NONE;
	else if (i >= 0x40 && i < 0x78)
	    regtypes[i] = REGTYPE_BLITTER;
	else if (i >= 0xA0 && i < 0xE0 && (i & 0xF) < 0xE)
	    regtypes[i] = REGTYPE_AUDIO;
	else if (i >= 0xA0 && i < 0xE0)
	    regtypes[i] = REGTYPE_NONE;
	else if (i >= 0xE0 && i < 0x100)
	    regtypes[i] = REGTYPE_PLANE;
	else if (i >= 0x120 && i < 0x180)
	    regtypes[i] = REGTYPE_SPRITE;
	else if (i >= 0x180 && i < 0x1C0)
	    regtypes[i] = REGTYPE_COLOR;
	else switch (i) {
		case 0x02:
			/* DMACONR - setting this to REGTYPE_BLITTER will cause it to
	       conflict with DMACON (since that is REGTYPE_ALL), and the
	       blitter registers (for the BBUSY bit), but nothing else,
	       which is (I think) what we want.  */
			regtypes[i] = REGTYPE_BLITTER;
			break;
		case 0x04: case 0x06: case 0x2A: case 0x2C:
			regtypes[i] = REGTYPE_POS;
			break;
		case 0x0A: case 0x0C:
		case 0x12: case 0x14: case 0x16:
		case 0x36:
			regtypes[i] = REGTYPE_JOYPORT;
			break;
		case 0x104:
		case 0x102:
			regtypes[i] = REGTYPE_PLANE;
			break;
		case 0x88: case 0x8A:
		case 0x8E: case 0x90: case 0x92: case 0x94:
		case 0x96:
		case 0x100:
			regtypes[i] |= REGTYPE_FORCE;
			break;
	}
    }
}
#endif

static void MISC_handler(void)
{
    int i, recheck;
    evt mintime;
    evt ct = get_cycles();
    static int recursive;

    if (recursive)
	return;
    recursive++;
    eventtab[ev_misc].active = 0;
    recheck = 1;
    while (recheck) {
	recheck = 0;
	mintime = ~0L;
	for (i = 0; i < ev2_max; i++) {
	    if (eventtab2[i].active) {
		if (eventtab2[i].evtime == ct) {
		    eventtab2[i].active = 0;
		    eventtab2[i].handler(eventtab2[i].data);
		    if (eventtab2[i].active)
			recheck = 1;
		} else {
		    evt eventtime = eventtab2[i].evtime - ct;
		    if (eventtime < mintime)
			mintime = eventtime;
		}
	    }
	}
    }
    if (mintime != ~0L) {
	eventtab[ev_misc].active = 1;
	eventtab[ev_misc].oldcycles = ct;
	eventtab[ev_misc].evtime = ct + mintime;
	events_schedule();
    }
    recursive--;
}

STATIC_INLINE void event2_newevent_x(int no, evt t, uae_u32 data, evfunc2 func)
{
    evt et = t * CYCLE_UNIT + get_cycles();

    if (no < 0) {
	for (no = ev2_misc; no < ev2_max; no++) {
	    if (!eventtab2[no].active)
		break;
	    if (eventtab2[no].evtime == et && eventtab2[no].handler == func) {
		eventtab2[no].handler(eventtab2[no].data);
		break;
	    }
	}
	if (no == ev2_max) {
	    write_log("out of event2's! PC=%x\n", M68K_GETPC);
	    return;
	}
    }
    eventtab2[no].active = 1;
    eventtab2[no].evtime = et;
    eventtab2[no].handler = func;
    eventtab2[no].data = data;
    MISC_handler();
}
void event2_newevent(int no, evt t)
{
    event2_newevent_x(no, t, 0, eventtab2[no].handler);
}
void event2_newevent2(evt t, uae_u32 data, evfunc2 func)
{
    event2_newevent_x(-1, t, data, func);
}

void event2_remevent(int no)
{
    eventtab2[no].active = 0;
}

void init_eventtab (void)
{
	int i;
	
  nextevent = 0;
	currcycle = 0;
	for (i = 0; i < ev_max; i++) {
		eventtab[i].active = 0;
		eventtab[i].oldcycles = 0;
	}
    for (i = 0; i < ev2_max; i++) {
	eventtab2[i].active = 0;
    }
	
	eventtab[ev_cia].handler = CIA_handler;
	eventtab[ev_hsync].handler = hsync_handler;
  eventtab[ev_hsync].evtime = get_cycles () + HSYNCTIME;
	eventtab[ev_hsync].active = 1;
#ifdef FAST_COPPER
	eventtab[ev_copper].handler = copper_handler;
	eventtab[ev_copper].active = 0;
#endif
  eventtab[ev_misc].handler = MISC_handler;
	eventtab[ev_audio].handler = audio_evhandler;

  eventtab2[ev2_blitter].handler = blitter_handler;
  eventtab2[ev2_disk].handler = DISK_handler;

	events_schedule ();
}

void customreset (int hardreset)
{
	int i;
	int zero = 0;

  reset_all_systems ();

  hsync_counter = 0;
  vsync_counter = 0;
  ciavsync_counter = 0;
  if (! savestate_state) {
		currprefs.chipset_mask = changed_prefs.chipset_mask;
  	update_mirrors();
		if (!aga_mode) {
			for (i = 0; i < 32; i++) {
		    current_colors.color_regs_ecs[i] = 0;
				current_colors.acolors[i] = xcolors[0];
			}
		} else {
			for (i = 0; i < 256; i++) {
				current_colors.color_regs_aga[i] = 0;
				current_colors.acolors[i] = CONVERT_RGB (zero);
			}
		}
		
		clxdat = 0;
		
		/* Clear the armed flags of all sprites.  */
		memset (spr, 0, sizeof spr);
		nr_armed = 0;
		
		dmacon = intena = 0;
		
		copcon = 0;
		DSKLEN (0, 0);
		
		bplcon0 = 0;
		planes_bplcon0=0;
		planes_limit_bplcon0=0;
		res_bplcon0=0;
		bplcon4 = 0x0011; /* Get AGA chipset into ECS compatibility mode */
		bplcon3 = 0x0C00;
		diwhigh = 0;
		diwhigh_written = 0;
		hdiwstate = DIW_waiting_start; // this does not reset at vblank
		
		FMODE (0);
		CLXCON (0);
		CLXCON2 (0);
		lof = 0;
	}
		
	vsync_handler_cnt_disk_change = 0;
	
#ifdef AUTOCONFIG
	expamem_reset ();
#endif
  a1000_reset ();
	DISK_reset ();
	CIA_reset ();
#ifdef JIT
  compemu_reset ();
#endif
  unset_special (&regs, ~(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE));
	
	vpos = 0;
	
  inputdevice_reset();
  timehack_alive = 0;
	
	curr_sprite_entries[0].first_pixel = 0;
	curr_sprite_entries[1].first_pixel = 0;
	next_sprite_entry = 0;
	memset (spixels, 0, sizeof spixels);
	memset (&spixstate, 0, sizeof spixstate);
	
	bltstate = BLT_done;
	cop_state.state = COP_stop;
	diwstate = DIW_waiting_start;
	hdiwstate = DIW_waiting_start;
	currcycle = 0;
	
	new_beamcon0 = currprefs.ntscmode ? 0x00 : 0x20;
  hack_vpos = 0;
	time_per_frame = 1000 * 1000 / (currprefs.ntscmode ? 60 : 50);
	init_hz ();
	
	audio_reset ();
  if (savestate_state != STATE_RESTORE) {
	  /* must be called after audio_reset */
	  adkcon = 0;
	  audio_update_adkmasks ();
  }
	
	init_sprites ();
	
	init_hardware_frame ();
  drawing_init ();
	
	reset_decisions ();
	
#ifdef FAST_COPPER
	init_regtypes ();	
#endif

	sprite_buffer_res = currprefs.chipset_mask & CSMASK_AGA ? RES_HIRES : RES_LORES;
  if (savestate_state == STATE_RESTORE) {
		uae_u16 v;
		uae_u32 vv;
		
		audio_update_adkmasks ();
		INTENA_f (0);
		INTREQ_f (0);
		
		if (diwhigh)
			diwhigh_written = 1;
		else
		  diwhigh_written = 0;
		COPJMP (1);
		v = bplcon0;
		BPLCON0 (0, 0);
		BPLCON0 (0, v);
		FMODE (fmode);
		
		if (!(currprefs.chipset_mask & CSMASK_AGA)) {
			for(i = 0 ; i < 32 ; i++)  {
				vv = current_colors.color_regs_ecs[i];
				current_colors.color_regs_ecs[i] = (unsigned short)-1;
				record_color_change (0, i, vv);
				remembered_color_entry = -1;
				current_colors.color_regs_ecs[i] = vv;
				current_colors.acolors[i] = xcolors[vv];
			}
		} else {
			for(i = 0 ; i < 256 ; i++)  {
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
		for (i = 0; i < 8; i++)
			nr_armed += spr[i].armed != 0;
  	if (! currprefs.produce_sound) {
  	    eventtab[ev_audio].active = 0;
  	    events_schedule ();
  	}
	}
	sprres = expand_sprres (bplcon0, bplcon3);

  if (hardreset)
  	rtc_hardreset();
}

void dumpcustom (void)
{
    write_log ("DMACON: %x INTENA: %x INTREQ: %x VPOS: %x HPOS: %x\n", DMACONR(),
	       (unsigned int)intena, (unsigned int)intreq, (unsigned int)vpos, (unsigned int)current_hpos());
    write_log ("COP1LC: %08lx, COP2LC: %08lx COPPTR: %08lx\n", (unsigned long)cop1lc, (unsigned long)cop2lc, cop_state.ip);
    write_log ("DIWSTRT: %04x DIWSTOP: %04x DDFSTRT: %04x DDFSTOP: %04x\n",
	       (unsigned int)diwstrt, (unsigned int)diwstop, (unsigned int)ddfstrt, (unsigned int)ddfstop);
    write_log ("BPLCON 0: %04x 1: %04x 2: %04x 3: %04x 4: %04x\n", bplcon0, bplcon1, bplcon2, bplcon3, bplcon4);
}


static void gen_custom_tables (void)
{
	int i;
	for (i = 0; i < 256; i++) {
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
	for (i = 0; i < 16; i++) {
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

/* mousehack is now in "filesys boot rom" */
static uae_u32 REGPARAM2 mousehack_helper_old (struct TrapContext *ctx)
{
    return 0;
}

static int allocate_sprite_tables (void)
{
    curr_sprite_entries = (struct sprite_entry *)xmalloc (MAX_SPRITE_ENTRY * sizeof (struct sprite_entry));
    curr_color_changes = (struct color_change *)xmalloc (MAX_COLOR_CHANGE * sizeof (struct color_change));

    return 1;
}

int custom_init (void)
{
    if (!allocate_sprite_tables())
	return 0;

#ifdef AUTOCONFIG
    if (uae_boot_rom) {
    	uaecptr pos;
      pos = here ();

      org (rtarea_base + 0xFF70);
      calltrap (deftrap (mousehack_helper_old));
      dw (RTS);

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
    default_xlate, default_check, NULL, "Custom chipset",
    custom_lgeti, custom_wgeti, ABFLAG_IO
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

STATIC_INLINE uae_u32 REGPARAM2 custom_wget_1 (uaecptr addr, int noput)
{
    uae_u16 v;
    
#ifdef JIT
    special_mem |= S_READ;
#endif
    switch (addr & 0x1fe) {
     case 0x000: v = 0xffff; break; /* BPLDDAT */
     case 0x002: v = DMACONR (); break;
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
     case 0x01A: v = DSKBYTR (current_hpos ()); break;
     case 0x01C: v = INTENAR (); break;
     case 0x01E: v = INTREQR (); break;
     case 0x07C: v = DENISEID (); break;

     case 0x02E: v = 0xffff; break; /* temporary hack */

     case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
     case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
     case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
     case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
     case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
     case 0x1BC: case 0x1BE:
		v = COLOR_READ ((addr & 0x3E) / 2);
		break;

     default:
	    /* reading write-only register causes write with last value in bus */
	    v = last_custom_value;
    	if (!noput) {
	      int hpos = current_hpos ();
        decide_line (hpos);
	      decide_fetch (hpos);
        custom_wput_1 (hpos, addr, v, 1);
      }
	    return v;
    }
    return v;
}

static uae_u32 REGPARAM2 custom_wget (uaecptr addr)
{
#ifdef FAST_COPPER
    sync_copper_with_cpu (current_hpos (), 1, addr);
#else
    sync_copper_with_cpu (current_hpos (), 1);
#endif
    if (addr & 1) {
       uae_u32 v;
       
       /* think about move.w $dff005,d0.. (68020+ only) */
       addr &= ~1;
       v = custom_wget_1 (addr, 0) << 8;
       v |= custom_wget_1 (addr + 2, 0) >> 8;
       return v & 0xFFFF;
    }
    return custom_wget_1 (addr, 0);
}

static uae_u32 REGPARAM2 custom_bget (uaecptr addr)
{
#ifdef JIT
    special_mem |= S_READ;
#endif
    return (custom_wget (addr & ~1) >> (addr & 1 ? 0 : 8)) & 0xFF;
}

static uae_u32 REGPARAM2 custom_lget (uaecptr addr)
{
#ifdef JIT
    special_mem |= S_READ;
#endif
    return ((uae_u32)custom_wget (addr & 0xfffe) << 16) | custom_wget ((addr + 2) & 0xfffe);
}

static int REGPARAM2 custom_wput_1 (int hpos, uaecptr addr, uae_u32 value, int noget)
{
    addr &= 0x1FE;
    value &= 0xffff;
    last_custom_value = value;
    switch (addr) {
     case 0x00E: CLXDAT (); break;

     case 0x020: DSKPTH (value); break;
     case 0x022: DSKPTL (value); break;
     case 0x024: DSKLEN (value, hpos); break;
     case 0x026: DSKDAT (value); break;

     case 0x02A: VPOSW (value); break;
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

     case 0x058: BLTSIZE (value); break;
     case 0x05A: BLTCON0L (value); break;
     case 0x05C: BLTSIZV (value); break;
     case 0x05E: BLTSIZH (value); break;

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

     case 0x088: COPJMP (1); break;
     case 0x08A: COPJMP (2); break;

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

     case 0x110: BPL1DAT (hpos, value); break;
     case 0x112: /* BPL2DAT (value); */ break;
     case 0x114: /* BPL3DAT (value); */ break;
     case 0x116: /* BPL4DAT (value); */ break;
     case 0x118: /* BPL5DAT (value); */ break;
     case 0x11A: /* BPL6DAT (value); */ break;
     case 0x11C: /* BPL7DAT (value); */ break;
     case 0x11E: /* BPL8DAT (value); */ break;

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

     case 0x1C0: if (htotal != value) { htotal = value; varsync (); } break;
     case 0x1C2: if (hsstop != value) { hsstop = value; varsync (); } break;
     case 0x1C4: if (hbstrt != value) { hbstrt = value; varsync (); } break;
     case 0x1C6: if (hbstop != value) { hbstop = value; varsync (); } break;
     case 0x1C8: if (vtotal != value) { vtotal = value; varsync (); } break;
     case 0x1CA: if (vsstop != value) { vsstop = value; varsync (); } break;
     case 0x1CC: if (vbstrt < value || vbstrt > value + 1) { vbstrt = value; varsync (); } break;
     case 0x1CE: if (vbstop < value || vbstop > value + 1) { vbstop = value; varsync (); } break;
     case 0x1DC: BEAMCON0 (value); break;
     case 0x1DE: if (hsstrt != value) { hsstrt = value; varsync (); } break;
     case 0x1E0: if (vsstrt != value) { vsstrt = value; varsync (); } break;
     case 0x1E2: if (hcenter != value) { hcenter = value; varsync (); } break;
     case 0x1E4: DIWHIGH (hpos, value); break;
     case 0x1FC: FMODE (value); break;
     /* writing to read-only register causes read access */
     default: 
	    if (!noget) 
        custom_wget_1 (addr, 1);
	    last_custom_value = 0xffff;
	    return 1;
    }
    return 0;
}

static void REGPARAM2 custom_wput (uaecptr addr, uae_u32 value)
{
    int hpos = current_hpos ();

#ifdef JIT
    special_mem |= S_WRITE;
#endif
#ifdef FAST_COPPER
    sync_copper_with_cpu (hpos, 1, addr);
#else
    sync_copper_with_cpu (hpos, 1);
#endif
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
    uae_u16 rval = (value << 8) | (value & 0xFF);
#ifdef JIT
    special_mem |= S_WRITE;
#endif
    custom_wput (addr & ~1, rval);
}

static void REGPARAM2 custom_lput(uaecptr addr, uae_u32 value)
{
#ifdef JIT
    special_mem |= S_WRITE;
#endif
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
#define RW restore_u16 ()
#define RL restore_u32 ()

uae_u8 *restore_custom (uae_u8 *src)
{
    uae_u16 dsklen, dskbytr, ru16;
    int dskpt;
    int i;

    audio_reset ();

    changed_prefs.chipset_mask = currprefs.chipset_mask = RL;
    update_mirrors();
    blt_info.bltddat = RW;	/* 000 BLTDDAT */
    ru16 = RW;	    /* 002 DMACONR -> see also 096 */
    if((ru16 & 0x4000) == 0)
      bltstate = BLT_done;
    blt_info.blitzero = (ru16 & 0x2000 ? 1 : 0);
    RW;				/* 004 VPOSR */
    RW;				/* 006 VHPOSR */
    RW;		    /* 008 DSKDATR returns always 0 in uae4all... */
    RW;				/* 00A JOY0DAT */
    RW;				/* 00C JOY1DAT */
    clxdat = RW;		/* 00E CLXDAT */
    RW;				      /* 010 ADKCONR -> see 09E */
    RW;				      /* 012 POT0DAT */
    RW;				      /* 014 POT1DAT */
    RW;				      /* 016 POTGOR -> see 034 */
    RW;				/* 018 SERDATR* */
    dskbytr = RW;		/* 01A DSKBYTR */
    RW;				      /* 01C INTENAR -> see 09A */
    RW;				      /* 01E INTREQR -> see 09C */
    dskpt =  RL;			/* 020-022 DSKPT */
    dsklen = RW;		/* 024 DSKLEN */
    RW;				/* 026 DSKDAT */
    RW;				/* 028 REFPTR */
    lof = RW;			/* 02A VPOSW */
    RW;				/* 02C VHPOSW */
    ru16=RW; COPCON(ru16);	/* 02E COPCON */
    RW;				      /* 030 SERDAT */
    RW;				/* 032 SERPER* */
    ru16=RW; POTGO(ru16);		/* 034 POTGO */
    RW;				/* 036 JOYTEST* */
    RW;				/* 038 STREQU */
    RW;				/* 03A STRVHBL */
    RW;				/* 03C STRHOR */
    RW;				/* 03E STRLONG */
    ru16=RW; BLTCON0(ru16);	/* 040 BLTCON0 */
    ru16=RW; BLTCON1(ru16);	/* 042 BLTCON1 */
    ru16=RW; BLTAFWM(ru16);	/* 044 BLTAFWM */
    ru16=RW; BLTALWM(ru16);	/* 046 BLTALWM */
    bltcpt=RL; // u32=RL; BLTCPTH(u32);	/* 048-04B BLTCPT */
    bltbpt=RL; // u32=RL; BLTBPTH(u32);	/* 04C-04F BLTBPT */
    bltapt=RL; // u32=RL; BLTAPTH(u32);	/* 050-053 BLTAPT */
    bltdpt=RL; // u32=RL; BLTDPTH(u32);	/* 054-057 BLTDPT */
    RW;				/* 058 BLTSIZE */
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
    ru16=RW; BLTBDAT(ru16);	/* 072 BLTBDAT */
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
    ru16=RW; CLXCON(ru16);	/* 098 CLXCON */
    intena = RW;		/* 09A INTENA */
    intreq = intreqr = RW;	/* 09C INTREQ */
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
    ru16=RW; CLXCON2(ru16);	/* 10E CLXCON2* */
    for(i = 0; i < 8; i++)
	RW;			/*     BPLXDAT */
	  /* 120 - 17E Sprite regs */
    for(i = 0; i < 32; i++)
	current_colors.color_regs_ecs[i] = RW; /* 180-1BE COLORxx */
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
    diwhigh &= 0x7fff;

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
    RW;				/* 1FA ? */
    fmode = RW;			/* 1FC FMODE */
    last_custom_value = RW;	/* 1FE ? */

    DISK_restore_custom (dskpt, dsklen, dskbytr);

  	res_bplcon0=GET_RES(bplcon0);
 	  planes_bplcon0=GET_PLANES(bplcon0);
  	planes_limit_bplcon0 = GET_PLANES_LIMIT();
 		FMODE (fmode);

    return src;
}

#define SB save_u8
#define SW save_u16
#define SL save_u32

uae_u8 *save_custom (int *len, uae_u8 *dstptr, int full)
{
    uae_u8 *dstbak, *dst;
    int i;
    uae_u32 dskpt;
    uae_u16 dsklen, dsksync, dskbytr;

    DISK_save_custom (&dskpt, &dsklen, &dsksync, &dskbytr);

    if (dstptr)
    	dstbak = dst = dstptr;
    else
      dstbak = dst = (uae_u8 *)malloc (8+256*2);

    SL (currprefs.chipset_mask);			/* 000 chipset_mask */
    SW (blt_info.bltddat);	/* 000 BLTDDAT */
    SW (DMACONR());	  /* 002 DMACONR */
    SW (VPOSR());		  /* 004 VPOSR */
    SW (VHPOSR());		/* 006 VHPOSR */
    SW (0);		        /* 008 DSKDATR */
    SW (JOY0DAT());		/* 00A JOY0DAT */
    SW (JOY1DAT());		/* 00C JOY1DAT */
    SW (clxdat);		/* 00E CLXDAT */
    SW (ADKCONR());		/* 010 ADKCONR */
    SW (POT0DAT());		/* 012 POT0DAT */
    SW (POT0DAT());		/* 014 POT1DAT */
    SW (0)	;		/* 016 POTINP * */
    SW (0);			/* 018 SERDATR * */
    SW (dskbytr);		/* 01A DSKBYTR */
    SW (INTENAR());		/* 01C INTENAR */
    SW (INTREQR());		/* 01E INTREQR */
    SL (dskpt);			/* 020-023 DSKPT */
    SW (dsklen);		/* 024 DSKLEN */
    SW (0);			/* 026 DSKDAT */
    SW (0);			/* 028 REFPTR */
    SW (lof);			/* 02A VPOSW */
    SW (0);			/* 02C VHPOSW */
    SW (copcon);		/* 02E COPCON */
    SW (0); //serper);		/* 030 SERDAT * */
    SW (0); //serdat);		/* 032 SERPER * */
    SW (potgo_value);		/* 034 POTGO */
    SW (0);			/* 036 JOYTEST * */
    SW (0);			/* 038 STREQU */
    SW (0);			/* 03A STRVBL */
    SW (0);			/* 03C STRHOR */
    SW (0);			/* 03E STRLONG */
    SW (bltcon0);		/* 040 BLTCON0 */
    SW (bltcon1);		/* 042 BLTCON1 */
    SW (blt_info.bltafwm);	/* 044 BLTAFWM */
    SW (blt_info.bltalwm);	/* 046 BLTALWM */
    SL (bltcpt);		/* 048-04B BLTCPT */
    SL (bltbpt);		  /* 04C-04F BLTBPT */
    SL (bltapt);		  /* 050-053 BLTAPT */
    SL (bltdpt);		  /* 054-057 BLTDPT */
    SW (0);			/* 058 BLTSIZE */
    SW (0);			/* 05A BLTCON0L (use BLTCON0 instead) */
    SW (blt_info.vblitsize);		/* 05C BLTSIZV */
    SW (blt_info.hblitsize);	/* 05E BLTSIZH */
    SW (blt_info.bltcmod);	/* 060 BLTCMOD */
    SW (blt_info.bltbmod);	/* 062 BLTBMOD */
    SW (blt_info.bltamod);	/* 064 BLTAMOD */
    SW (blt_info.bltdmod);	/* 066 BLTDMOD */
    SW (0);			/* 068 ? */
    SW (0);			/* 06A ? */
    SW (0);			/* 06C ? */
    SW (0);			/* 06E ? */
    SW (blt_info.bltcdat);	/* 070 BLTCDAT */
    SW (blt_info.bltbdat);	/* 072 BLTBDAT */
    SW (blt_info.bltadat);	/* 074 BLTADAT */
    SW (0);			/* 076 ? */
    SW (0);			/* 078 ? */
    SW (0);			/* 07A ? */
    SW (DENISEID());		/* 07C DENISEID/LISAID */
    SW (dsksync);		/* 07E DSKSYNC */
    SL (cop1lc);		/* 080-083 COP1LC */
    SL (cop2lc);		/* 084-087 COP2LC */
    SW (0);			      /* 088 COPJMP1 */
    SW (0);			      /* 08A COPJMP2 */
    SW (0);			      /* 08C COPINS */
    SW (diwstrt);		/* 08E DIWSTRT */
    SW (diwstop);		/* 090 DIWSTOP */
    SW (ddfstrt);		/* 092 DDFSTRT */
    SW (ddfstop);		/* 094 DDFSTOP */
    SW (dmacon);		/* 096 DMACON */
    SW (clxcon);		/* 098 CLXCON */
    SW (intena);		/* 09A INTENA */
    SW (intreq);		/* 09C INTREQ */
    SW (adkcon);		/* 09E ADKCON */
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
	    SW (0);			/* 110 BPLxDAT */
	  /* 120 - 17E Sprite regs */
    if (full) {
	    for (i = 0; i < 8; i++) {
	      SL (spr[i].pt);	    /* 120-13E SPRxPT */
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
    SW (diwhigh | (diwhigh_written ? 0x8000 : 0)); /* 1E4 DIWHIGH */
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
    SW (0);			/* 1FA */
    SW (fmode);			/* 1FC FMODE */
    SW (last_custom_value);		/* 1FE */

    *len = dst - dstbak;
    return dstbak;
}

uae_u8 *restore_custom_agacolors (uae_u8 *src)
{
    int i;

    for (i = 0; i < 256; i++) {
	current_colors.color_regs_aga[i] = RL;
    }
    return src;
}

uae_u8 *save_custom_agacolors (int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak, *dst;
    int i;

    if (dstptr)
    	dstbak = dst = dstptr;
    else
      dstbak = dst = (uae_u8 *)malloc (256*4);
    for (i = 0; i < 256; i++)
	SL (current_colors.color_regs_aga[i]);
    *len = dst - dstbak;
    return dstbak;
}

uae_u8 *restore_custom_sprite (int num, uae_u8 *src)
{
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
    spr[num].armed = RB;
    return src;
}

uae_u8 *save_custom_sprite(int num, int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak, *dst;

    if (dstptr)
    	dstbak = dst = dstptr;
    else
      dstbak = dst = (uae_u8 *)malloc (30);
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

#endif /* SAVESTATE */

void check_prefs_changed_custom (void)
{
  currprefs.gfx_framerate = changed_prefs.gfx_framerate;
  if (inputdevice_config_change_test ())
  	inputdevice_copyconfig (&changed_prefs, &currprefs);
  currprefs.immediate_blits = changed_prefs.immediate_blits;
  currprefs.pandora_partial_blits = changed_prefs.pandora_partial_blits;
  currprefs.collision_level = changed_prefs.collision_level;

  if (currprefs.chipset_mask != changed_prefs.chipset_mask ||
	currprefs.picasso96_nocustom != changed_prefs.picasso96_nocustom ||
	currprefs.ntscmode != changed_prefs.ntscmode) {
	  currprefs.picasso96_nocustom = changed_prefs.picasso96_nocustom;
	  currprefs.chipset_mask = changed_prefs.chipset_mask;
    if (currprefs.ntscmode != changed_prefs.ntscmode) {
	    currprefs.ntscmode = changed_prefs.ntscmode;
	    new_beamcon0 = currprefs.ntscmode ? 0x00 : 0x20;
    }
  	init_custom ();
  }
}

