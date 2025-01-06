/*
* UAE - The Un*x Amiga Emulator
*
* Custom chip emulation
*
* Copyright 1995-2002 Bernd Schmidt
* Copyright 1995 Alessandro Bissacco
* Copyright 2000-2023 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <algorithm>
#include <cctype>
#include <cassert>
#include <cmath>

#include "options.h"
#include "uae.h"
#include "gensound.h"
#include "audio.h"
#include "sounddep/sound.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "cia.h"
#include "disk.h"
#include "blitter.h"
#include "xwin.h"
#include "inputdevice.h"
#ifdef SERIAL_PORT
#include "serial.h"
#endif
#include "autoconf.h"
#include "traps.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "savestate.h"
#include "ar.h"
#include "debug.h"
#include "akiko.h"
#if defined(ENFORCER)
#include "enforcer.h"
#endif
#include "threaddep/thread.h"
#ifdef WITH_LUA
#include "luascript.h"
#endif
#include "crc32.h"
#include "devices.h"
#include "rommgr.h"
#ifdef WITH_SPECIALMONITORS
#include "specialmonitors.h"
#endif

#define BPL_ERASE_TEST 0

#define FRAMEWAIT_MIN_MS 2
#define FRAMEWAIT_SPLIT 4

#define CYCLE_CONFLICT_LOGGING 0

#define SPEEDUP 1

#define CUSTOM_DEBUG 0
#define SPRITE_DEBUG 0
#define SPRITE_DEBUG_MINY 0
#define SPRITE_DEBUG_MAXY 0x30
#define MAX_SPRITES 8
#define AUTOSCALE_SPRITES 1
#define ALL_SUBPIXEL 1

#define RGA_COPPER_PIPELINE_DEPTH 2
#define RGA_SPRITE_PIPELINE_DEPTH 2
#define REFRESH_FIRST_HPOS 3
#define DMAL_FIRST_HPOS 11
#define SPR_FIRST_HPOS 25
#define COPPER_CYCLE_POLARITY 0
#define HARDWIRED_DMA_TRIGGER_HPOS 1

#define REF_RAS_ADD_AGA 0x000
#define REF_RAS_ADD_ECS 0x200
#define REF_RAS_ADD_OCS 0x002

#define SPRBORDER 0

#define EXTRAWIDTH_BROADCAST 15
#define EXTRAHEIGHT_BROADCAST_TOP 0
#define EXTRAHEIGHT_BROADCAST_BOTTOM 0
#define EXTRAWIDTH_EXTREME 33
#define EXTRAHEIGHT_EXTREME 24
#define EXTRAWIDTH_ULTRA 77

#define LORES_TO_SHRES_SHIFT 2

#ifdef SERIAL_PORT
extern uae_u16 serper;
#endif

static void uae_abort (const TCHAR *format,...)
{
	static int nomore;
	va_list parms{};
	TCHAR buffer[1000];

	va_start (parms, format);
	_vsntprintf (buffer, sizeof (buffer) / sizeof(TCHAR) - 1, format, parms );
	va_end (parms);
	if (nomore) {
		write_log (_T("%s\n"), buffer);
		return;
	}
	gui_message (buffer);
	nomore = 1;
}

static uae_u32 total_skipped = 0;

extern int cpu_last_stop_vpos, cpu_stopped_lines;
static int cpu_sleepmode, cpu_sleepmode_cnt;

extern int vsync_activeheight, vsync_totalheight;
extern float vsync_vblank, vsync_hblank;

/* Events */

evt_t vsync_cycles;
static int extra_cycle;

static int rpt_did_reset;
struct ev eventtab[ev_max];
struct ev2 eventtab2[ev2_max];

int vpos, vpos_prev, vposh;
static int hpos_hsync_extra;
static int vpos_count, vpos_count_diff;
int lof_store; // real bit in custom registers
int lof_display; // what display device thinks
int scandoubled_line;
static bool lof_lastline, lof_prev_lastline;
static int lol, lol_prev;
static bool linetoggle;
static int next_lineno;
static int linear_vpos;
static enum nln_how nextline_how;
static int lof_changed = 0, lof_changing = 0, interlace_changed = 0;
static int lof_changed_previous_field;
static int vposw_change;
static bool lof_lace;
static bool prevlofs[3];
static bool bplcon0_interlace_seen;
static bool vsync_rendered, frame_rendered, frame_shown;
static frame_time_t vsynctimeperline;
static frame_time_t frameskiptime;
static bool genlockhtoggle;
static bool genlockvtoggle;
static bool graphicsbuffer_retry;
static int cia_hsync;
static bool toscr_scanline_complex_bplcon1, toscr_scanline_complex_bplcon1_off;
static int toscr_hend;
static int nosignal_cnt, nosignal_status;
static bool nosignal_trigger;
static int issyncstopped_count;
int display_reset;
static evt_t line_start_cycles;
static bool initial_frame;
static evt_t custom_color_write_cycle;
static int color_writes_num;
static bool line_equ_freerun;

#define LOF_TOGGLES_NEEDED 3
//#define NLACE_CNT_NEEDED 50
static int lof_togglecnt_lace, lof_togglecnt_nlace; //, nlace_cnt;

/* Stupid genlock-detection prevention hack.
* We should stop calling vsync_handler() and
* hstop_handler() completely but it is not
* worth the trouble..
*/
static int vpos_previous, hpos_previous;
static int vpos_lpen, hpos_lpen, hhpos_lpen, lightpen_triggered;
int lightpen_x[2], lightpen_y[2];
int lightpen_cx[2], lightpen_cy[2], lightpen_active, lightpen_enabled, lightpen_enabled2;

static uae_u32 sprtaba[256],sprtabb[256];
static uae_u32 sprite_ab_merge[256];
/* Tables for collision detection.  */
static uae_u32 sprclx[16], clxmask[16];

/* T genlock bit in ECS Denise and AGA color registers */
static uae_u8 color_regs_genlock[256];

/*
* Hardware registers of all sorts.
*/

static uae_u16 cregs[256];

uae_u16 intena, intreq;
static uae_u16 intena2, intreq2;
uae_u16 dmacon;
uae_u16 adkcon; /* used by audio code */
uae_u16 last_custom_value;
static bool dmacon_bpl, vdiwstate_bpl;

static uae_u32 cop1lc, cop2lc, copcon;

/*
* Horizontal defaults
* 
* 0x00   0 HCB
* 0x01   1 HC1 (HSTART)
* 0x09   9 VR1 (HBLANK start)
* 0x12  18 SHS (Horizontal sync start)
* 0x1a  26 VER1 PAL 
* 0x1b  27 VER1 NTSC
* 0x23  35 RHS (Horizontal sync end)
* 0x73 115 VR2
* 0x84 132 CEN (HCENTER)
* 0x8c 140 VER2 PAL (CEN->VER2 = CSYNC qualising pulse 2)
* 0x8d 141 VER2 NTSC
* 0xe2 226 HC226 (short line, selected if LOL=1, NTSC only)
* 0xe3 227 HC227 (NTSC long line/PAL)
* 
* SHS->VER1 = CSYNC equalising pulse 1
* CEN->VER2 = CSYNC equalising pulse 2
* 
* HC1->SHS = Inactivate part of CSYNC Vsync+Hsync pulse 1
* VR2->CEN = Inactivate part of CSYNC Vsync+HSync pulse 2
* 
*
* Vertical defaults
* 
* PAL
* 
* 0    SVB
* 2    VC2
* 3    VC3
* 5    VC5
* 7    VC7
* 8    VC8
* 25   RVB (Vertical blank end)
* 311  VC311 short field (Vertical blank start)
* 312  VC312 long field (Vertical blank start)
* 
* Odd field:
*
* HSYNC SHS to RHS
* VSYNC VC2/CEN to VC5/SHS
* CSYNC HSYNC + if VSYNC active: SHS to VER1 and CEN to VER2
*
* Vertical blank = VC312 + HC1 to RVB + HC1
* Vertical equalization = SVB + VR1 to VC7 + VR2
*
* Even field:
*
* HSYNC SHS to RHS
* VSYNC VC3/SHS to VC5/CEN
* CSYNC HSYNC + if VSYNC active: SHS to VER1 and CEN to VER2
*
* Vertical blank = VC311 + HC1 to RVB + HC1
* Vertical equalization start = SVB + VR2 to VC8 + VR1
*
*
* NTSC
* 
* 0    SVB
* 3    VC3
* 6    VC6
* 9    VC9
* 20   RVB (Vertical blank end)
* 261  VC261 short field (Vertical blank start)
* 262  VC262 long field (Vertical blank start)
* 
* Odd field:
* 
* HSYNC SHS to RHS
* VSYNC VC3/SHS to VC6/SHS
* CSYNC HSYNC + if VSYNC active: SHS to VER1 and CEN to VER2
*
* Vertical blank = VC262/HC1 to RVB/HC1
* Vertical equalization = SVB/VR1 to VC9/VR1
*
* Even field:
*
* HSYNC SHS to RHS
* VSYNC VC3/CEN to VC6/CEN
* CSYNC HSYNC + if VSYNC active: SHS to VER1 and CEN to VER2
*
* Vertical blank = VC261/HC1 to RVB/HC1
* Vertical equalization = SVB/VR2 to VC9/VR2
*
*/

/*
* Bitplane DMA enable logic OCS/ECS differences:
* 
* OCS: DDFSTRT hard start limit flag is disabled when horizontal hard start position matches.
* It is enabled during active bitplane DMA's last cycle. (Ending due to either DDFSTOP or hardstop match).
* ECS: DDFTSTR/STOP hard limit work as documented.
* It is cleared when hard start matches and set when hard stop matches.
* 
* OCS hard start limit bug: if line didn't have BPL DMA active, next line's BPL DMA can start earlier than start limit.
* (See music disk Ode to Ramon by Digital Force, bottom scroller "scanline affect" )
* This feature also prevents multiple DDFSTRT/STOP regions in same scanline. ECS/AGA does not have this limit.
* 
* DDFSTRT match is ignored if DMACON BPLEN is not enabled. ECS/AGA allows it. Does not affect DDFSTOP.
* Switch DMACON BPLEN off, wait value to DDFSTRT that matches during current scanline,
* switch BPLEN on: if OCS, bitplane DMA won't start, ECS/AGA: bitplane DMA starts.
* 
*/


int maxhpos = MAXHPOS_PAL;
int maxhpos_short = MAXHPOS_PAL;
int maxvpos = MAXVPOS_PAL;
int maxvpos_nom = MAXVPOS_PAL; // nominal value (same as maxvpos but "faked" maxvpos in fake 60hz modes)
int maxvpos_display = MAXVPOS_PAL; // value used for display size
int maxhpos_display = AMIGA_WIDTH_MAX;
int maxvsize_display = AMIGA_HEIGHT_MAX;
int maxvpos_display_vsync; // extra lines from top visible in bottom
static bool maxvpos_display_vsync_next;
static int vblank_extraline;
static int maxhposm1;
int maxhposm0 = MAXHPOS_PAL;
static bool maxhposeven, maxhposeven_prev;
static int hsyncendpos, hsyncstartpos;
int hsync_end_left_border;
static int hsyncstartpos_start, hsyncstartpos_start_cycles;

static int hsyncstartpos_start_hw;
int hsyncstartpos_hw;
int hsyncendpos_hw;

int denisehtotal;
static int maxvpos_total = 511;
int minfirstline = VBLANK_ENDLINE_PAL;
int minfirstline_linear = VBLANK_ENDLINE_PAL;
static int firstblankedline;
static int equ_vblank_endline = EQU_ENDLINE_PAL;
static bool equ_vblank_toggle = true;
float vblank_hz = VBLANK_HZ_PAL, fake_vblank_hz, vblank_hz_stored, vblank_hz_nom;
float hblank_hz;
static float vblank_hz_lof, vblank_hz_shf, vblank_hz_lace;
static int vblank_hz_mult, vblank_hz_state;
static struct chipset_refresh *stored_chipset_refresh;
int doublescan;
int programmedmode;
frame_time_t syncbase;
static int fmode_saved, fmode;
uae_u16 beamcon0, new_beamcon0;
uae_u16 bemcon0_hsync_mask, bemcon0_vsync_mask;
static uae_u16 beamcon0_saved;
static uae_u16 bplcon0_saved, bplcon1_saved, bplcon2_saved;
static uae_u16 bplcon3_saved, bplcon4_saved;
static int varsync_changed, varsync_maybe_changed[2];
static int varhblank_lines, varhblank_val[2];
static int exthblank_lines[2];
static uae_u16 vt_old, ht_old, hs_old, vs_old;
uae_u16 vtotal, htotal;
static int maxvpos_stored, maxhpos_stored;
uae_u16 hsstop, hsstrt;
uae_u16 hbstop, hbstrt;
static int hsstop_detect, hsstop_detect2;
static uae_u16 vsstop, vsstrt;
static uae_u16 vbstop, vbstrt;
static uae_u16 hcenter, hcenter_v2, hcenter_v2_end, hcenter_sync_v2;
static bool hcenter_active;
static uae_u16 hbstop_v, hbstrt_v, hbstop_v2, hbstrt_v2, hbstop_sync_v2, hbstrt_sync_v2, hbstop_reg, hbstrt_reg;
static uae_u16 hsstrt_v2, hsstop_v2;
static int vsstrt_m, vsstop_m, vbstrt_m, vbstop_m;
static bool vb_state, vb_end_line;
static bool vs_state_var, vs_state_on, vs_state_on_old, vs_state_hw, vs_state_var_old;
static bool vb_end_next_line, vb_end_next_line2;
static int vb_start_line;
static bool ocs_blanked;
static uae_u16 sprhstrt, sprhstop, bplhstrt, bplhstop, hhpos, hhpos_hpos;
static uae_u16 sprhstrt_v, sprhstop_v, bplhstrt_v, bplhstop_v;
static int hhbpl, hhspr;
static int ciavsyncmode;
static int diw_hstrt, diw_hstop;
static int hdiw_counter;
static int hstrobe_hdiw_min;
static uae_u32 ref_ras_add;
static uaecptr refptr, refptr_p;
static uae_u32 refmask;
static int refresh_handled_slot;
static bool refptr_preupdated;
static bool hstrobe_conflict;
static uae_u16 strobe_vblank;
static bool vhposw_modified;
static int line_disabled;
static bool custom_disabled;

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
	int width;
	bool ecs_denise_hires;
	bool firstslotdone;

	uae_u16 ctl, pos;
#ifdef AGA
	uae_u16 data[4], datb[4];
#else
	uae_u16 data[1], datb[1];
#endif
};

static struct sprite spr[MAX_SPRITES];
static int plfstrt_sprite;
static int sprbplconflict, sprbplconflict_hpos;
static int sprbplconflict2, sprbplconflict_hpos2;
static uae_u16 sprbplconflict_dat;
uaecptr sprite_0;
int sprite_0_width, sprite_0_height, sprite_0_doubled;
uae_u32 sprite_0_colors[4];
static uae_u8 magic_sprite_mask = 0xff;

static int hardwired_vbstrt, hardwired_vbstop;

static int last_sprite_point, last_sprite_point_abs;
static int nr_armed;
static int sprite_width, sprres;
static int sprite_sprctlmask;
int sprite_buffer_res;

uae_u8 cycle_line_slot[MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST + MAX_CHIPSETSLOTS_EXTRA];
uae_u16 cycle_line_pipe[MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST + MAX_CHIPSETSLOTS_EXTRA];
static uae_u8 cycle_line_slot_last;

static uae_s16 bpl1mod, bpl2mod, bpl1mod_prev, bpl2mod_prev;
static int bpl1mod_hpos, bpl2mod_hpos;
static uaecptr prevbpl[2][MAXVPOS][MAX_PLANES];
static uaecptr bplpt[MAX_PLANES], bplptx[MAX_PLANES];

static struct color_entry current_colors;
uae_u16 bplcon0;
static uae_u16 bplcon1, bplcon2, bplcon3, bplcon4;
static int bplcon0d, bplcon0d_old;
static bool bplcon0d_change;
static uae_u32 bplcon0_res, bplcon0_planes, bplcon0_planes_limit;
static int diwstrt, diwstop, diwhigh;
static int diwhigh_written;
static int ddfstrt, ddfstop, plfstop_prev;
static int ddfstrt_hpos, ddfstop_hpos;
static int line_cyclebased, diw_change;
static int bpl_shifter;

static void SET_LINE_CYCLEBASED(int hpos);

/* The display and data fetch windows */

enum class diw_states
{
	DIW_waiting_start, DIW_waiting_stop
};

static int plffirstline, plflastline;
int plffirstline_total, plflastline_total;
int vblank_firstline_hw;
static int autoscale_bordercolors;
static int plfstrt, plfstop;
static int sprite_minx;
static int first_bpl_vpos;
static int last_decide_line_hpos;
static int last_fetch_hpos, last_decide_sprite_hpos;
static int diwfirstword, diwlastword;
static int last_diwlastword;
static int hb_last_diwlastword;
static int last_hdiw;
static diw_states vdiwstate, hdiwstate, hdiwstate_blank;
static int hdiwbplstart;
static int bpl_hstart;
static bool exthblank, exthblank_prev, exthblank_state, hcenterblank_state;
static int hsyncdebug;
static int last_diw_hpos;
static int last_recorded_diw_hpos;
static int last_hblank_start;
static int collision_hpos;

int first_planes_vpos, last_planes_vpos;
static int first_bplcon0, first_bplcon0_old;
static int first_planes_vpos_old, last_planes_vpos_old;
int diwfirstword_total, diwlastword_total;
int ddffirstword_total, ddflastword_total;
static int diwfirstword_total_old, diwlastword_total_old;
static int ddffirstword_total_old, ddflastword_total_old;
bool vertical_changed, horizontal_changed;
int firstword_bplcon1;

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
	COP_bltwait2,
	COP_wait_in2,
	COP_skip_in2,
	COP_wait1,
	COP_wait,
	COP_skip,
	COP_skip1,
	COP_strobe_delay1,
	COP_strobe_delay2,
	COP_strobe_delay3,
	COP_strobe_delay4,
	COP_strobe_delay5,
	COP_strobe_delay1x,
	COP_strobe_delay2x,
	COP_strobe_extra, // just to skip current cycle when CPU wrote to COPJMP
	COP_start_delay
};

struct copper {
	/* The current instruction words.  */
	uae_u16 ir[2];
	enum copper_states state, state_prev;
	/* Instruction pointer.  */
	uaecptr ip;
	// following move does not enable COPRS
	int ignore_next;
	int vcmp, hcmp;

	int strobe; /* COPJMP1 / COPJMP2 accessed */
	int last_strobe;
	int moveaddr, movedata, movedelay;
	uaecptr moveptr;
	uaecptr vblankip;
};

static struct copper cop_state;
static int copper_enabled_thisline;
static evt_t copper_bad_cycle;
static uaecptr copper_bad_cycle_pc_old;
static evt_t copper_bad_cycle_start;
static uaecptr copper_bad_cycle_pc_new;

static evt_t copper_dma_change_cycle;
static evt_t blitter_dma_change_cycle;
static evt_t sprite_dma_change_cycle_on, sprite_dma_change_cycle_off;

/*
* Statistics
*/
uae_u32 timeframes;
evt_t frametime;
frame_time_t lastframetime;
uae_u32 hsync_counter, vsync_counter;
frame_time_t idletime;
int bogusframe;
static int display_vsync_counter;

/* Recording of custom chip register changes.  */
static int current_change_set;
#define MAX_SPRITE_ENTRIES ((MAXVPOS + MAXVPOS_WRAPLINES) * 16)
static struct sprite_entry sprite_entries[2][MAX_SPRITE_ENTRIES];
static struct color_change color_changes[2][MAX_REG_CHANGE];

struct decision line_decisions[2 * (MAXVPOS + MAXVPOS_WRAPLINES) + 1];
static struct draw_info line_drawinfo[2][2 * (MAXVPOS + MAXVPOS_WRAPLINES) + 1];
#define COLOR_TABLE_SIZE (MAXVPOS + MAXVPOS_WRAPLINES) * 2
static struct color_entry color_tables[2][COLOR_TABLE_SIZE];

static int next_sprite_entry = 0, end_sprite_entry;
static int prev_next_sprite_entry;
static int next_sprite_forced = 1;
static int spixels_max;

struct sprite_entry *curr_sprite_entries, *prev_sprite_entries;
struct color_change *curr_color_changes, *prev_color_changes;
struct draw_info *curr_drawinfo, *prev_drawinfo;
struct color_entry *curr_color_tables, *prev_color_tables;

static int next_color_change, last_color_change;
static int next_color_entry, remembered_color_entry;
static int color_src_match, color_dest_match, color_compare_result;

static uae_u32 thisline_changed;

#ifdef SMART_UPDATE
#define MARK_LINE_CHANGED do { thisline_changed = 1; } while (0)
#else
#define MARK_LINE_CHANGED do { ; } while (0)
#endif

static struct decision thisline_decision;
static int fetch_cycle, fetch_modulo_cycle;

static bool ddf_limit, ddf_limit_latch, ddfstrt_match, hwi_old;
static int ddf_stopping, ddf_enable_on;
static int bprun, bprun_end;
static int bprun_cycle;
static int bprun_pipeline_flush_delay;
static bool plane0, plane0p, plane0p_enabled, plane0p_forced, plane0_first_done;
static int last_bpl1dat_hpos;
static bool last_bpl1dat_hpos_early;
static bool harddis_v, harddis_h;
static uae_u16 dmal_alloc_mask;

#define RGA_PIPELINE_OFFSET_BPL_WRITE 3
#define RGA_PIPELINE_OFFSET_COPPER 2
#define RGA_PIPELINE_OFFSET_SPRITE 2
#define RGA_PIPELINE_OFFSET_DMAL 2

struct custom_store custom_storage[256];

#define TOSCR_RES_PIXELS_LORES 4
#define TOSCR_RES_PIXELS_HIRES 2
#define TOSCR_RES_PIXELS_SHRES 1

#define TOSCR_NBITS 16
#define TOSCR_NBITSHR 32

static int out_nbits, out_offs;
static uae_u32 todisplay[MAX_PLANES], todisplay2[MAX_PLANES];
static uae_u32 outword[MAX_PLANES];
static uae_u64 outword64[MAX_PLANES], outword64_extra[MAX_PLANES];
static uae_u16 fetched[MAX_PLANES];
static uae_u16 todisplay_fetched;
#ifdef AGA
static uae_u64 todisplay_aga[MAX_PLANES], todisplay2_aga[MAX_PLANES];
static bool todisplay2_lastbit[MAX_PLANES];
static uae_u64 fetched_aga[MAX_PLANES];
static uae_u32 fetched_aga_spr[MAX_PLANES];
#endif

/* Expansions from bplcon0/bplcon1.  */
static int toscr_res2p, toscr_res_mult;
static int toscr_res, toscr_res_old;
static int toscr_res_pixels, toscr_res_pixels_shift, toscr_res_pixels_shift_old;
static int toscr_res_pixels_hr, toscr_res_pixels_shift_hr, toscr_res_pixels_mask_hr, toscr_res_pixels_mask_hr_old, toscr_res_pixels_mask_hr2;
static int toscr_nr_planes, toscr_nr_planes2, toscr_nr_planes3, toscr_nr_planes_agnus;
static bool toscr_nr_changed;
static int toscr_nr_planes_shifter, toscr_nr_planes_shifter_new;
static int fetchwidth;
static int toscr_delay_adjusted[2], toscr_delay_sh[2];
static bool shdelay_disabled;
static int delay_cycles, delay_cycles2;
static int delay_lastcycle[2], delay_hsynccycle;
static int vhposr_delay_offset;
static bool bplcon1_written;
static bool bplcon0_planes_changed;
static bool sprites_enabled_this_line;
static int shifter_mask, toscr_delay_shifter[2];
static int out_subpix[2];
static bool speedup_first;

static void events_dmal(int);
static uae_u16 dmal, dmal_hpos;
static uae_u16 dmal_htotal_mask;
static bool dmal_ce;

static void update_copper(int until_hpos);
static void decide_sprites_fetch(int endhpos);
static void decide_line(int endhpos);
static void decide_sprites(int hpos, bool quick, bool halfcycle);
static void decide_sprites(int hpos);

/* The number of bits left from the last fetched words.
This is an optimization - conceptually, we have to make sure the result is
the same as if toscr is called in each clock cycle.  However, to speed this
up, we accumulate display data; this variable keeps track of how much.
Thus, once we do call toscr_nbits (which happens at least every 16 bits),
we can do more work at once.  */
static int toscr_nbits;

static int REGPARAM3 custom_wput_1(int, uaecptr, uae_u32, int) REGPARAM;

/*
* helper functions
*/

static bool safecpu(void)
{
	return currprefs.cpu_model == 68000 && currprefs.cpu_cycle_exact && currprefs.blitter_cycle_exact && currprefs.m68k_speed == 0 && !(currprefs.cs_hacks & 16);
}

static void check_nocustom(void)
{
	struct amigadisplay* ad = &adisplays[0];
	if (ad->picasso_on && currprefs.picasso96_nocustom) {
		custom_disabled = true;
		line_disabled |= 2;
	} else {
		custom_disabled = false;
		line_disabled &= ~2;
	}
}

STATIC_INLINE bool ecsshres(void)
{
	return bplcon0_res == RES_SUPERHIRES && ecs_denise && !aga_mode;
}

STATIC_INLINE bool nodraw(void)
{
	struct amigadisplay *ad = &adisplays[0];
	bool nd = !currprefs.cpu_memory_cycle_exact && ad->framecnt != 0;
	return nd;
}

STATIC_INLINE int diw_to_hpos(int diw)
{
	diw /= 4;
	diw += DDF_OFFSET;
	diw /= 2;
	return diw;
}

STATIC_INLINE int hpos_to_diw(int pos)
{
	return (pos * 2 - DDF_OFFSET) * 4;
}

STATIC_INLINE int hpos_to_diwx(int pos)
{
	return ((pos + hpos_hsync_extra) * 2 - DDF_OFFSET) * 4;
}

static bool doflickerfix_possible(void)
{
	return currprefs.gfx_vresolution && doublescan < 0 && vpos < MAXVPOS;
}
static bool doflickerfix_active(void)
{
	return interlace_seen > 0 && doflickerfix_possible();
}

uae_u32 get_copper_address(int copno)
{
	switch (copno) {
	case 1: return cop1lc;
	case 2: return cop2lc;
	case 3: return cop_state.vblankip;
	case -1: return cop_state.ip;
	default: return 0;
	}
}

void reset_frame_rate_hack(void)
{
	if (currprefs.m68k_speed >= 0) {
		return;
	}

	rpt_did_reset = 1;
	events_reset_syncline();
	vsyncmintime = read_processor_time() + vsynctimebase;
	write_log(_T("Resetting frame rate hack\n"));
}

static void setclr(uae_u16 *p, uae_u16 val)
{
	if (val & 0x8000) {
		*p |= val & 0x7FFF;
	} else {
		*p &= ~val;
	}
}

static int adjust_hr(int v)
{
	if (currprefs.gfx_resolution >= RES_HIRES) {
		if (currprefs.chipset_hr) {
			v &= ~(3 >> currprefs.gfx_resolution);
		} else {
			v &= ~3;
		}
	} else {
		v &= ~1;
	}
	return v;
}
static int adjust_hr2(int v)
{
	if (currprefs.gfx_resolution >= RES_HIRES) {
		if (currprefs.chipset_hr) {
			v &= ~(1 >> currprefs.gfx_resolution);
		} else {
			v &= ~1;
		}
	}
	return v;
}

STATIC_INLINE bool is_last_line(void)
{
	return vpos + 1 == maxvpos + lof_store;
}

static void alloc_cycle(int hpos, int type)
{
#ifdef CPUEMU_13
#if CYCLE_CONFLICT_LOGGING
	if (cycle_line_slot[hpos] && cycle_line_slot[hpos] != type) {
		write_log(_T("alloc cycle conflict %d: %02x <- %02x\n"), hpos, type, cycle_line_slot[hpos]);
	}
#endif
	cycle_line_slot[hpos] = type;
#endif
}
static void alloc_cycle_maybe(int hpos, int type)
{
	if ((cycle_line_slot[hpos] & CYCLE_MASK) == 0) {
		alloc_cycle(hpos, type);
	}
}

void alloc_cycle_ext(int hpos, int type)
{
	alloc_cycle(hpos, type);
}

uaecptr alloc_cycle_blitter_conflict_or(int hpos, int chnum, bool *skip)
{
	uaecptr orptr = 0;
	if (copper_bad_cycle && line_start_cycles + hpos * CYCLE_UNIT == copper_bad_cycle) {
		orptr = copper_bad_cycle_pc_old;
	} else if (hpos == sprbplconflict_hpos2) {
		uae_u16 bltdat = chnum == 4 ? 0x000 : (3 - chnum) * 2 + 0x70;
		uae_u16 rga = bltdat & sprbplconflict2;
		int spnum = (sprbplconflict2 - 0x140) / 8;
		orptr = spr[spnum].pt;
		if (rga != bltdat) {
			*skip = true;
		}
	}
	return orptr;
}

bool alloc_cycle_blitter(int hpos, uaecptr *ptr, int chnum, int add)
{
	bool skipadd = false;
	if (copper_bad_cycle && line_start_cycles + hpos * CYCLE_UNIT == copper_bad_cycle) {
		write_log("Copper PT=%08x/%08x. Blitter CH=%d MOD=%d PT=%08x. Conflict bug!\n", copper_bad_cycle_pc_old, copper_bad_cycle_pc_new, chnum, add, *ptr);
		cop_state.ip += add;
		*ptr = copper_bad_cycle_pc_old;
		skipadd = true;
		copper_bad_cycle = 0;
	} else if (hpos == sprbplconflict_hpos2) {
		uae_u16 v = chipmem_wget_indirect(*ptr);
		uae_u16 bltdat = chnum == 4 ? 0x000 : (3 - chnum) * 2 + 0x70;
		uae_u16 rga = bltdat & sprbplconflict2;
		int spnum = (sprbplconflict2 - 0x140) / 8;
		uaecptr pt = spr[spnum].pt;
		spr[spnum].pt = *ptr + 2 + add;
		custom_wput_1(hpos, rga, v, 1);
		sprbplconflict_hpos2 = -1;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(v);
			record_dma_read(rga, *ptr, hpos, vpos, DMARECORD_SPRITE, spnum);
		}
#endif
		write_log("Sprite %d. Blitter CH=%d MOD=%d PT=%08x. Conflict bug!\n", spnum, chnum, add, *ptr);
	}
	alloc_cycle(hpos, CYCLE_BLITTER);
	return skipadd;
}

static int expand_sprres(uae_u16 con0, uae_u16 con3)
{
	int res;

	switch ((con3 >> 6) & 3)
	{
	default:
		res = RES_LORES;
		break;
#ifdef ECS_DENISE
	case 0: /* ECS defaults (LORES,HIRES=LORES sprite,SHRES=HIRES sprite) */
		if (ecs_denise && GET_RES_DENISE(con0) == RES_SUPERHIRES)
			res = RES_HIRES;
		else
			res = RES_LORES;
		break;
#endif
#ifdef AGA
	case 1:
		res = RES_LORES;
		break;
	case 2:
		res = RES_HIRES;
		break;
	case 3:
		res = RES_SUPERHIRES;
		break;
#endif
	}
	return res;
}

static int islinetoggle(void)
{
	int linetoggle = 0;
	if (!(new_beamcon0 & BEAMCON0_LOLDIS) && !(new_beamcon0 & BEAMCON0_PAL) && ecs_agnus) {
		linetoggle = 1; // NTSC and !LOLDIS -> LOL toggles every line
	} else if (!ecs_agnus && currprefs.ntscmode) {
		linetoggle = 1; // hardwired NTSC Agnus
	}
	return linetoggle;
}

STATIC_INLINE uae_u8 *pfield_xlateptr(uaecptr plpt, int bytecount)
{
	if (!chipmem_check_indirect(plpt, bytecount)) {
		static int count = 0;
		if (!count) {
			count++;
			write_log(_T("Warning: Bad playfield pointer %08x\n"), plpt);
		}
		return NULL;
	}
	return chipmem_xlate_indirect(plpt);
}

static void docols(struct color_entry *colentry)
{
#ifdef AGA
	if (aga_mode) {
		for (int i = 0; i < 256; i++) {
			int v = color_reg_get(colentry, i);
			colentry->acolors[i] = getxcolor(v);
		}
	} else {
#endif
		for (int i = 0; i < 32; i++) {
			int v = color_reg_get(colentry, i);
			colentry->acolors[i] = getxcolor(v);
		}
#ifdef AGA
	}
#endif
}

static void remember_ctable(void)
{
	/* This can happen when program crashes very badly */
	if (next_color_entry >= COLOR_TABLE_SIZE) {
		return;
	}
	if (remembered_color_entry < 0) {
		/* The colors changed since we last recorded a color map. Record a
		* new one. */
		color_reg_cpy(curr_color_tables + next_color_entry, &current_colors);
		remembered_color_entry = next_color_entry++;
	}
	thisline_decision.ctable = remembered_color_entry;
	if (color_src_match < 0 || color_dest_match != remembered_color_entry
		|| line_decisions[next_lineno].ctable != color_src_match)
	{
		/* The remembered comparison didn't help us - need to compare again. */
		int oldctable = line_decisions[next_lineno].ctable;
		int changed = 0;

		if (oldctable < 0) {
			changed = 1;
			color_src_match = color_dest_match = -1;
		} else {
			color_compare_result = color_reg_cmp(&prev_color_tables[oldctable], &current_colors) != 0;
			if (color_compare_result) {
				changed = 1;
			}
			color_src_match = oldctable;
			color_dest_match = remembered_color_entry;
		}
		thisline_changed |= changed;
	} else {
		/* We know the result of the comparison */
		if (color_compare_result) {
			thisline_changed = 1;
		}
	}
}

static void remember_ctable_for_border(void)
{
	remember_ctable();
}

static int get_equ_vblank_endline(void)
{
	if (new_beamcon0 & (BEAMCON0_BLANKEN | BEAMCON0_VARCSYEN)) {
		return -1;
	}
	return equ_vblank_endline + (equ_vblank_toggle ? (lof_store ? 1 : 0) : 0) + (agnusa1000 ? 1 : 0);
}
static int get_equ_vblank_startline(void)
{
	if (new_beamcon0 & (BEAMCON0_BLANKEN | BEAMCON0_VARCSYEN)) {
		return 30000;
	}
	if (agnusa1000) {
		return 1;
	} else {
		return 0;
	}
}

static int output_res(int res)
{
	if (currprefs.chipset_hr && res < currprefs.gfx_resolution) {
		return currprefs.gfx_resolution;
	}
	return res;
}

static void reset_bpl_vars()
{
	out_subpix[0] = 0;
	out_subpix[1] = 0;
	out_nbits = 0;
	out_offs = 0;
	toscr_nbits = 0;
	thisline_decision.bplres = output_res(bplcon0_res);
}

STATIC_INLINE bool line_hidden(void)
{
	return linear_vpos >= maxvpos_display_vsync && linear_vpos < minfirstline - 1 && currprefs.gfx_overscanmode < OVERSCANMODE_ULTRA;
}

// hblank start = enable border (bitplane not visible until next BPL1DAT)..
static void hblank_reset(int hblankpos)
{
	if (thisline_decision.plfleft < 0) {
		return;
	}
	// border re-enable takes 1 lores pixel
	hb_last_diwlastword = hblankpos + 4;
}

static void addcc(int pos, int reg, int v)
{
	color_change *cc = &curr_color_changes[next_color_change++];
	cc->linepos = pos;
	cc->regno = reg;
	cc->value = v;
	cc++;
	cc->regno = -1;
}

static void addccmp(int pos, int reg, int v, int chpos)
{
	if (pos < chpos && pos >= last_recorded_diw_hpos) {
		addcc(pos, reg, v);
	}
}

#ifdef DEBUGGER
static void syncdebugmarkers(int chpos)
{
	bool pal = (beamcon0 & BEAMCON0_PAL) != 0;
	if (hsyncdebug == 3) {
		// CSYNC
		int con = 1;
		int coff = 0;
		if (beamcon0 & BEAMCON0_CSYTRUE) {
			con = 0;
			coff = 1;
		}
		bool varcsy = (beamcon0 & BEAMCON0_VARCSYEN) != 0;
		bool blanken = (beamcon0 & BEAMCON0_BLANKEN) != 0;
		int hc1 = 1 + maxhpos;
		hc1 <<= CCK_SHRES_SHIFT;
		int nline = pal ? 8 : 10;
		addccmp(hbstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x204, 1, chpos);
		addccmp(hbstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x204, 0, chpos);
		if (blanken) {
			addccmp(hbstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
			addccmp(hbstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
		} else if (!varcsy) {
			int ver1 = pal ? 26 : 27;
			int ver2 = pal ? 140 : 141;
			int vr2 = 115;
			ver1 <<= CCK_SHRES_SHIFT;
			ver2 <<= CCK_SHRES_SHIFT;
			vr2 <<= CCK_SHRES_SHIFT;
			if (vpos >= nline) {
				addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
				addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
			}
			if (pal) {
				// PAL
				if (lof_display) {
					// EVEN
					if (vpos == 0) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 1 || vpos == 2 || vpos == 6 || vpos == 7) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 3 || vpos == 4) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(vr2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 5) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(vr2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
				} else {
					// ODD
					if (vpos == 0 || vpos == 1 || vpos == 5 || vpos == 6) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 2) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 3 || vpos == 4) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(vr2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 7) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
				}
			} else {
				// NTSC
				if (lof_display) {
					// EVEN
					if (vpos == 0) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 1 || vpos == 2 || vpos == 7 || vpos == 8 || vpos == 9) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 4 || vpos == 5) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(vr2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 3) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 6) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(vr2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
				} else {
					// ODD
					if (vpos == 0 || vpos == 1 || vpos == 2 || vpos == 6 || vpos == 7 || vpos == 8) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 3) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(ver1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 4 || vpos == 5) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(vr2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						addccmp(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hc1, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
					if (vpos == 9) {
						addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
						addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
						thisline_changed = 1;
					}
				}
			}
		} else {
			if (vs_state_var && !vs_state_var_old && lof_display) {
				// First VS=1 line, LOF=1: VS=1 at HCENTER start
				addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
				if (hsstop_v2 < hcenter_sync_v2) {
					addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
				}
				if (hbstrt_sync_v2 >= hcenter_sync_v2) {
					addccmp(hbstrt_sync_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
				}
				if (hbstop_sync_v2 >= hcenter_sync_v2) {
					addccmp(hbstop_sync_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
				}
				addccmp(hcenter_sync_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
				thisline_changed = 1;
			} else if (vs_state_var) {
				addccmp(hbstrt_sync_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
				addccmp(hbstop_sync_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
				addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
				addccmp(hcenter_sync_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
				thisline_changed = 1;
			} else {
				addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, con, chpos);
				addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, coff, chpos);
			}
		}
	} else {
		// H/V SYNC
		int hon = 1;
		int hoff = 0;
		if (beamcon0 & BEAMCON0_HSYTRUE) {
			hon = 0;
			hoff = 1;
		}
		if (hcenter_v2 < chpos && hcenter_v2 >= last_recorded_diw_hpos) {
			if (pal) {
				if (vs_state_var && !lof_display) {
					addcc(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x206, 0);
				} else if (!vs_state_var && vs_state_var_old && lof_display) {
					addcc(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x206, 1);
				}
			} else {
				if (vs_state_var && lof_display) {
					addcc(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x206, 0);
				} else if (!vs_state_var && vs_state_var_old && lof_display) {
					addcc(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x206, 1);
				}
			}
		}
		addccmp(hsstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, hon, chpos);
		addccmp(hsstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x202, hoff, chpos);
		addccmp(hbstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x204, 1, chpos);
		addccmp(hbstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x204, 0, chpos);
	}
}
#endif

static void record_color_change2(int hpos, int regno, uae_u32 value)
{
	if (line_disabled) {
		return;
	}

	if (next_color_change > MAX_REG_CHANGE - 30) {
		return;
	}

	int pos = hpos < 0 ? -hpos : hpos_to_diwx(hpos);
	int start_color_change = next_color_change;

	// AGA has extra hires pixel delay in color changes
	if ((regno < RECORDED_REGISTER_CHANGE_OFFSET || regno == RECORDED_REGISTER_CHANGE_OFFSET + 0x10c || regno == RECORDED_REGISTER_CHANGE_OFFSET + 0x201) && aga_mode) {
		if (currprefs.chipset_hr) {
			pos += 2;
		}
		if (regno == RECORDED_REGISTER_CHANGE_OFFSET + 0x201) {
			// EHB delay
			pos += 2;
		}
		if (regno == RECORDED_REGISTER_CHANGE_OFFSET + 0x10c) {
			// BPLCON4:
			// Bitplane XOR change: 2 hires pixel delay
			// Sprite bank change: 1 hires pixel delay
			if (!currprefs.chipset_hr) {
				pos += 2;
			}
			if (value & 0xff00) {
				thisline_decision.xor_seen = true;
			}
			pos += 2;
			if ((value & 0x00ff) != (bplcon4 & 0x00ff)) {
				// Sprite bank delay
				addcc(pos, regno | 1, value);
			}
			pos += 2;
		}
	}

	// HCENTER blanking (ECS Denise only)
	if (hcenter_active && vs_state_on && lof_display) {
		int chpos = pos;
		if (!hcenterblank_state && hcenter_v2 < chpos && hcenter_v2 >= last_recorded_diw_hpos) {
			hcenterblank_state = true;
			if ((bplcon0 & 1) && (bplcon3 & 1)) {
				addcc(hcenter_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 1);
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HBS, diw_to_hpos(hcenter_v2), vpos);
				}
#endif
				thisline_changed = 1;
			}
		}
		if (hcenterblank_state && hcenter_v2_end < chpos && hcenter_v2_end >= last_recorded_diw_hpos) {
			hcenterblank_state = false;
			if ((bplcon0 & 1) && (bplcon3 & 1)) {
				addcc(hcenter_v2_end, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 0);
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HBE, diw_to_hpos(hcenter_v2), vpos);
				}
#endif
				thisline_changed = 1;
			}
		}
	}

	if (exthblank) {
		int chpos = pos;
		// inject programmed hblank start and end in color changes
		if (hbstrt_v2 <= hbstop_v2) {
			if (hbstrt_v2 < chpos && hbstrt_v2 >= last_recorded_diw_hpos) {
				addcc(hbstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 1);
				hblank_reset(hbstrt_v2);
				exthblank_state = true;
				last_hblank_start = hbstrt_v2;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HBS, diw_to_hpos(hbstrt_v2), vpos);
				}
#endif
			}
			if (hbstop_v2 < chpos && hbstop_v2 >= last_recorded_diw_hpos) {
				// do_color_changes() HBLANK workaround
				if (next_color_change == last_color_change && exthblank_state) {
					addcc(0, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 1);
				}
				addcc(hbstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 0);
				exthblank_state = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HBE, diw_to_hpos(hbstop_v2), vpos);
				}
#endif
			}
		} else if (hbstrt_v2 > hbstop_v2) { // equal: blank disable wins
			if (hbstop_v2 < chpos && hbstop_v2 >= last_recorded_diw_hpos) {
				if (next_color_change == last_color_change && exthblank_state) {
					addcc(0, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 1);
				}
				addcc(hbstop_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 0);
				exthblank_state = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HBE, diw_to_hpos(hbstop_v2), vpos);
				}
#endif
			}
			if (hbstrt_v2 < chpos && hbstrt_v2 >= last_recorded_diw_hpos) {
				addcc(hbstrt_v2, RECORDED_REGISTER_CHANGE_OFFSET + 0x200, 1);
				hblank_reset(hbstrt_v2);
				exthblank_state = true;
				last_hblank_start = hbstrt_v2;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HBS, diw_to_hpos(hbstrt_v2), vpos);
				}
#endif
			}
		}
	}

#ifdef DEBUGGER
	// inject hsync and end in color changes (ultra mode debug)
	if (hsyncdebug) {
		syncdebugmarkers(pos);
	}
#endif

	if (regno != 0xffff) {
		addcc(pos, regno, value);
	}

	last_recorded_diw_hpos = std::max(pos, last_recorded_diw_hpos);

#ifdef DEBUGGER
	int cchanges = next_color_change - start_color_change;
	if (cchanges > 1 && hsyncdebug) {
		if (cchanges == 2) {
			color_change *cc1 = &curr_color_changes[start_color_change];
			color_change *cc2 = &curr_color_changes[start_color_change + 1];
			if (cc1->linepos > cc2->linepos) {
				color_change cc = *cc2;
				*cc2 = *cc1;
				*cc1 = cc;
			}
		} else {
			// debug only, can be slow
			for (int i = 0; i < cchanges; i++) {
				color_change *cc1 = &curr_color_changes[start_color_change + i];
				for(int j = i + 1; j < cchanges; j++) {
					color_change *cc2 = &curr_color_changes[start_color_change + j];
					if (cc1->linepos > cc2->linepos) {
						color_change cc = *cc2;
						*cc2 = *cc1;
						*cc1 = cc;
					}
				}
			}
		}
	}
#endif
}

static void sync_color_changes(int hpos)
{
	record_color_change2(hpos, 0xffff, 0);
}

static void insert_actborder(int diw, bool onoff, bool blank)
{
	if (custom_disabled) {
		return;
	}

	int i;
	// already exists?
	for (i = last_color_change; i < next_color_change; i++) {
		struct color_change *cc = &curr_color_changes[i];
		if (cc->linepos == diw && cc->regno == 0 && (cc->value & COLOR_CHANGE_MASK) == COLOR_CHANGE_ACTBORDER) {
			cc->value = COLOR_CHANGE_ACTBORDER | (onoff ? 1 : 0) | (blank ? 2 : 0);
			return;
		}
	}
	// find slot to insert into
	for (i = last_color_change; i < next_color_change; i++) {
		struct color_change *cc = &curr_color_changes[i];
		if (cc->linepos > diw) {
			break;
		}
	}
	if (i < next_color_change) {
		int num = next_color_change - i;
		memmove(&curr_color_changes[i + 1], &curr_color_changes[i], sizeof(struct color_change) * num);
	}

	struct color_change *cc = &curr_color_changes[i];
	cc->linepos = diw;
	cc->regno = 0;
	cc->value = COLOR_CHANGE_ACTBORDER | (onoff ? 1 : 0) | (blank ? 2 : 0);
	next_color_change++;
}

// hdiw opened again in same scanline
// erase (color0 or bblank) area between previous end and new start
static void hdiw_restart(int diw_last, int diw_current, bool blank)
{
	if (diw_last >= diw_current || line_hidden() || custom_disabled) {
		return;
	}
	// update state
	diw_current = adjust_hr(diw_current);
	diw_last = adjust_hr(diw_last);
	record_color_change2(-diw_current, 0xffff, 0);

	// find slot to insert into
	int i;
	for (i = last_color_change; i < next_color_change; i++) {
		struct color_change *cc = &curr_color_changes[i];
		if (cc->linepos > diw_last) {
			break;
		}
	}
	if (i < next_color_change) {
		int num = next_color_change - i;
		memmove(&curr_color_changes[i + 1], &curr_color_changes[i], sizeof(struct color_change) * num);
	}

	struct color_change *cc = &curr_color_changes[i];
	cc->linepos = diw_last;
	cc->regno = 0;
	cc->value = COLOR_CHANGE_ACTBORDER | 1 | (blank ? 2 : 0);
	next_color_change++;

	record_color_change2(-diw_current, 0, COLOR_CHANGE_ACTBORDER | 0);
}

/* Called to determine the state of the horizontal display window state
* machine at the current position. It might have changed since we last
* checked.  */

static int hdiw_denisecounter, hdiw_denisecounter_abs, hdiw_denisecounter_reset;

// hblank start
static void decide_hdiw_blank_check_start(int hpos, int start_diw_hpos, int end_diw_hpos)
{
	if (hdiwstate_blank == diw_states::DIW_waiting_start) {
		if (hbstrt_reg >= start_diw_hpos && hbstrt_reg < end_diw_hpos) {
			int val = hdiw_denisecounter_abs + (hbstrt_reg - start_diw_hpos);
			insert_actborder(val, true, true);
			MARK_LINE_CHANGED;
			hdiwstate_blank = diw_states::DIW_waiting_stop;
		}
	}
}
// hblank end
static void decide_hdiw_blank_check_stop(int hpos, int start_diw_hpos, int end_diw_hpos)
{
	if (hdiwstate_blank == diw_states::DIW_waiting_stop) {
		if (hbstop_reg >= start_diw_hpos && hbstop_reg < end_diw_hpos) {
			int val = hdiw_denisecounter_abs + (hbstop_reg - start_diw_hpos);
			insert_actborder(val, false, true);
			MARK_LINE_CHANGED;
			hdiwstate_blank = diw_states::DIW_waiting_start;
		}
	}
}
// hdiw open
static void decide_hdiw_check_start(int start_diw_hpos, int end_diw_hpos)
{
	if (hdiwstate == diw_states::DIW_waiting_start) {
		if (diw_hstrt >= start_diw_hpos && diw_hstrt < end_diw_hpos) {
			int val = hdiw_denisecounter_abs + (diw_hstrt - start_diw_hpos);
			int first = coord_diw_shres_to_window_x(val);
			if (last_diwlastword >= 0) {
				// was closed previously in same line: fill closed part using color0.
				if (denisea1000) {
					last_diwlastword += 4 << shres_shift;
					if (last_diwlastword < val) {
						hdiw_restart(last_diwlastword, val, false);
					}
				} else {
					hdiw_restart(last_diwlastword, val, false);
				}
				last_diwlastword = -1;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HDIWS, diw_to_hpos(val), vpos);
				}
#endif
			}
			if (thisline_decision.diwfirstword < 0) {
				thisline_decision.diwfirstword = adjust_hr2(first);
				// opened before first BPL1DAT?
				if (!plane0_first_done) {
					plane0p_enabled = ecs_denise && (bplcon0 & 1) && (bplcon3 & 0x20);
				}
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HDIWS, diw_to_hpos(first), vpos);
				}
#endif
			}
			hdiwstate = diw_states::DIW_waiting_stop;
		}
	}
}
// hdiw close
static void decide_hdiw_check_stop(int start_diw_hpos, int end_diw_hpos)
{
	if (hdiwstate == diw_states::DIW_waiting_stop) {
		if (diw_hstop >= start_diw_hpos && diw_hstop < end_diw_hpos) {
			int val = hdiw_denisecounter_abs + (diw_hstop - start_diw_hpos);
			int last = coord_diw_shres_to_window_x(val);
			if (last > thisline_decision.diwlastword) {
				thisline_decision.diwlastword = adjust_hr2(last);
				// if HDIW opens again in same line
				last_diwlastword = val;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_HDIWE, diw_to_hpos(last), vpos);
				}
#endif
			}
			hdiwstate = diw_states::DIW_waiting_start;
		}
	}
}

static void decide_hdiw_check(int hpos, int start_diw_hpos, int end_diw_hpos)
{
	if ((diw_hstrt >= start_diw_hpos && diw_hstrt < end_diw_hpos) &&
		(diw_hstop >= start_diw_hpos && diw_hstop < end_diw_hpos)) {
		if (diw_hstrt > diw_hstop) {
			decide_hdiw_check_stop(start_diw_hpos, end_diw_hpos);
			decide_hdiw_check_start(start_diw_hpos, end_diw_hpos);
		} else {
			decide_hdiw_check_start(start_diw_hpos, end_diw_hpos);
			decide_hdiw_check_stop(start_diw_hpos, end_diw_hpos);
		}
	} else {
		decide_hdiw_check_start(start_diw_hpos, end_diw_hpos);
		decide_hdiw_check_stop(start_diw_hpos, end_diw_hpos);
	}
	// check also hblank if there is chance it has been moved to visible area
	if (hstrobe_conflict || vhposw_modified) {
		decide_hdiw_blank_check_start(hpos, start_diw_hpos, end_diw_hpos);
	}
	if (hdiwstate_blank == diw_states::DIW_waiting_stop) {
		decide_hdiw_blank_check_stop(hpos, start_diw_hpos, end_diw_hpos);
	}
	hdiw_denisecounter_abs += end_diw_hpos - start_diw_hpos;
}

static void decide_hdiw2(int hpos, bool halfcycle)
{
	int hp = (hpos << 1) + halfcycle; // cck->lores
	int c = (hp - last_diw_hpos) << (CCK_SHRES_SHIFT - 1); // ->shres
	if (c <= 0) {
		return;
	}
	if (last_diw_hpos < (HARDWIRED_DMA_TRIGGER_HPOS << 1) && hp >= (HARDWIRED_DMA_TRIGGER_HPOS << 1)) {
		hdiw_denisecounter_reset = (REFRESH_FIRST_HPOS - HARDWIRED_DMA_TRIGGER_HPOS + 3) << CCK_SHRES_SHIFT;
		//hdiw_denisecounter_reset += 1 << (CCK_SHRES_SHIFT - 1);
	}

	int start = hdiw_denisecounter;
	int max = 512 << (CCK_SHRES_SHIFT - 1);

	if (hdiw_denisecounter_reset > 0 && c >= hdiw_denisecounter_reset && !line_equ_freerun && !hstrobe_conflict) {
		// strobe crossed?
		hdiw_denisecounter += hdiw_denisecounter_reset;
		decide_hdiw_check(hpos, start, hdiw_denisecounter);
		hdiw_denisecounter = 2 << LORES_TO_SHRES_SHIFT;
		start = hdiw_denisecounter;
		hdiw_denisecounter += c - hdiw_denisecounter_reset;
		decide_hdiw_check(hpos, start, hdiw_denisecounter);
		hdiw_denisecounter_reset = -1;
	} else {
		hdiw_denisecounter += c;
		// wrap around?
		if (hdiw_denisecounter >= max) {
			decide_hdiw_check(hpos, start, max);
			hdiw_denisecounter &= max - 1;
			if (hdiw_denisecounter > 0) {
				decide_hdiw_check(hpos, 0, hdiw_denisecounter);
			}
		} else {
			decide_hdiw_check(hpos, start, hdiw_denisecounter);
		}
		if (hdiw_denisecounter_reset >= 0) {
			hdiw_denisecounter_reset -= c;
		}
	}

	last_diw_hpos = hp;
}

static void decide_hdiw(int hpos, bool halfcycle = false)
{
#ifdef DEBUGGER
	if (!debug_dma) {
		decide_hdiw2(hpos, halfcycle);
	} else {
		for (int i = (last_diw_hpos >> 1); i <= hpos; i++) {
			decide_hdiw2(i, halfcycle);
			halfcycle = false;
			record_dma_denise(i, hdiw_denisecounter >> 2);
		}
	}
#else
	decide_hdiw2(hpos, halfcycle);
#endif
}

static void sync_changes(int hpos)
{
	decide_hdiw(hpos);
	decide_sprites(hpos);
	record_color_change2(hpos, 0xffff, 0);

}

static int fetchmode, fetchmode_size, fetchmode_mask, fetchmode_bytes;
static int fetchmode_fmode_bpl, fetchmode_fmode_spr;
static int real_bitplane_number[3][3][9];

/* Disable bitplane DMA if planes > available DMA slots. This is needed
e.g. by the Sanity WOC demo (at the "Party Effect").  */
STATIC_INLINE int GET_PLANES_LIMIT(uae_u16 bc0)
{
	int res = GET_RES_AGNUS(bc0);
	int planes = GET_PLANES(bc0);
	return real_bitplane_number[fetchmode][res][planes];
}

static void add_mod(int nr, uae_s16 mod)
{
#ifdef AGA
	if (fetchmode_fmode_bpl == 1 || fetchmode_fmode_bpl == 2) {
		// FMODE=1/2, unaligned bpl and modulo: bit 1 carry is ignored.
		if ((bplpt[nr] & 2) && (mod & 2)) {
			mod -= 4;
		}
	} else if (fetchmode_fmode_bpl == 3) {
		// FMODE=3, unaligned bpl and modulo: bit 2 carry is ignored.
		if ((bplpt[nr] & (4 | 2)) + (mod & (4 | 2)) >= 8) {
			mod -= 8;
		}
	}
#endif

	bplpt[nr] += mod;
	bplptx[nr] += mod;
}

static void add_modulo(int hpos, int nr)
{
	uae_s16 mod, m1, m2;

	m1 = bpl1mod;
	m2 = bpl2mod;
	if (bpl1mod_hpos == hpos) {
		m1 = bpl1mod_prev;
	}
	if (bpl2mod_hpos == hpos) {
		m2 = bpl2mod_prev;
	}
	if (fmode & 0x4000) {
		if (((diwstrt >> 8) ^ (vposh ^ 1)) & 1) {
			mod = m2;
		} else {
			mod = m1;
		}
	} else if (nr & 1) {
		mod = m2;
	} else {
		mod = m1;
	}
	add_mod(nr, mod);
}

static void add_modulos(void)
{
	uae_s16 m1, m2;

	if (fmode & 0x4000) {
		if (((diwstrt >> 8) ^ (vposh ^ 1)) & 1) {
			m1 = m2 = bpl2mod;
		} else {
			m1 = m2 = bpl1mod;
		}
	} else {
		m1 = bpl1mod;
		m2 = bpl2mod;
	}

	switch (bplcon0_planes_limit) {
#ifdef AGA
	case 8: add_mod(7, m2);
	case 7: add_mod(6, m1);
#endif
	case 6: add_mod(5, m2);
	case 5: add_mod(4, m1);
	case 4: add_mod(3, m2);
	case 3: add_mod(2, m1);
	case 2: add_mod(1, m2);
	case 1: add_mod(0, m1);
	}
}

/* The fetch unit mainly controls ddf stop.  It's the number of cycles that
are contained in an indivisible block during which ddf is active.  E.g.
if DDF starts at 0x30, and fetchunit is 8, then possible DDF stops are
0x30 + n * 8.  */
static uae_u32 fetchunit, fetchunit_mask;
/* The delay before fetching the same bitplane again.  Can be larger than
the number of bitplanes; in that case there are additional empty cycles
with no data fetch (this happens for high fetchmodes and low
resolutions).  */
static uae_u32 fetchstart, fetchstart_shift, fetchstart_mask;
/* fm_maxplane holds the maximum number of planes possible with the current
fetch mode.  This selects the cycle diagram:
8 planes: 73516240
4 planes: 3120
2 planes: 10.  */
static uae_u32 fm_maxplane, fm_maxplane_shift;

/* The corresponding values, by fetchmode and display resolution.  */
static const uae_u8 fetchunits[] = { 8,8,8,0, 16,8,8,0, 32,16,8,0 };
static const uae_u8 fetchstarts[] = { 3,2,1,0, 4,3,2,0, 5,4,3,0 };
static const uae_u8 fm_maxplanes[] = { 3,2,1,0, 3,3,2,0, 3,3,3,0 };

static uae_s8 cycle_diagram_table[3][3][9][32];
static uae_s8 cycle_diagram_free_cycles[3][3][9];
static uae_s8 cycle_diagram_total_cycles[3][3][9];
static uae_s8 *curr_diagram;
static const uae_s8 cycle_sequences[3 * 8] = { 2,1,2,1,2,1,2,1, 4,2,3,1,4,2,3,1, 8,4,6,2,7,3,5,1 };

static void debug_cycle_diagram(void)
{
	int fm, res, planes, cycle, v;
	TCHAR aa;

	for (fm = 0; fm <= 2; fm++) {
		write_log(_T("FMODE %d\n=======\n"), fm);
		for (res = 0; res <= 2; res++) {
			for (planes = 0; planes <= 8; planes++) {
				write_log(_T("%d: "), planes);
				for (cycle = 0; cycle < 32; cycle++) {
					v = cycle_diagram_table[fm][res][planes][cycle];
					if (v == 0) aa = '-'; else if (v > 0) aa = '1'; else aa = 'X';
					write_log(_T("%c"), aa);
				}
				write_log(_T(" %d:%d\n"), cycle_diagram_free_cycles[fm][res][planes], cycle_diagram_total_cycles[fm][res][planes]);
			}
			write_log(_T("\n"));
		}
	}
	fm = 0;
}

static void create_cycle_diagram_table(void)
{
	int fm, res, cycle, planes, rplanes, v;
	int fetch_start, max_planes, freecycles;
	const uae_s8 *cycle_sequence;

	for (fm = 0; fm <= 2; fm++) {
		for (res = 0; res <= 2; res++) {
			max_planes = fm_maxplanes[fm * 4 + res];
			fetch_start = 1 << fetchstarts[fm * 4 + res];
			cycle_sequence = &cycle_sequences[(max_planes - 1) * 8];
			max_planes = 1 << max_planes;
			for (planes = 0; planes <= 8; planes++) {
				freecycles = 0;
				for (cycle = 0; cycle < 32; cycle++) {
					cycle_diagram_table[fm][res][planes][cycle] = -1;
				}
				if (planes <= max_planes) {
					for (cycle = 0; cycle < fetch_start; cycle++) {
						if (cycle < max_planes && planes >= cycle_sequence[cycle & 7]) {
							v = cycle_sequence[cycle & 7];
						} else {
							v = -1;
							freecycles++;
						}
						cycle_diagram_table[fm][res][planes][cycle] = v;
					}
				}
				cycle_diagram_free_cycles[fm][res][planes] = freecycles;
				cycle_diagram_total_cycles[fm][res][planes] = fetch_start;
				rplanes = planes;
				if (rplanes > max_planes) {
					rplanes = 0;
				}
				if (rplanes == 7 && fm == 0 && res == 0 && !aga_mode) {
					rplanes = 4;
				}
				real_bitplane_number[fm][res][planes] = rplanes;
			}
		}
	}
#if 0
	debug_cycle_diagram();
#endif
}

static void clear_bitplane_pipeline(int type)
{
	// clear bitplane allocations
	int safepos = hsyncstartpos_start_cycles - 1;
	int count = RGA_PIPELINE_ADJUST + 1;
	if (type) {
		for (int i = 0; i < maxhposm0 + RGA_PIPELINE_ADJUST; i++) {
			if (i < safepos || i >= safepos + count) {
				uae_u16 v = cycle_line_pipe[i];
				if (v & CYCLE_PIPE_BITPLANE) {
					cycle_line_pipe[i] = 0;
				}
			}
		}
	} else {
		for (int i = safepos; i < safepos + count; i++) {
			uae_u16 v = cycle_line_pipe[i];
			if (v & CYCLE_PIPE_BITPLANE) {
				cycle_line_pipe[i] = 0;
			}
		}
	}
}

#define HARD_DDF_STOP 0xd8

#define ESTIMATED_FETCH_MODE 1
#define OPTIMIZED_ESTIMATE 1

static uae_s8 estimated_cycles_buf0[MAX_CHIPSETSLOTS + MAX_CHIPSETSLOTS_EXTRA];
static uae_s8 estimated_cycles_buf1[MAX_CHIPSETSLOTS + MAX_CHIPSETSLOTS_EXTRA];
static uae_s8 estimated_cycles_empty[MAX_CHIPSETSLOTS + MAX_CHIPSETSLOTS_EXTRA];
static int estimate_cycles_empty_index = -1;
static uae_u16 estimated_bplcon0, estimated_fm, estimated_plfstrt, estimated_plfstop;
static uae_s8 *estimated_cycles = estimated_cycles_empty;
static uae_s8 *estimated_cycles_next = estimated_cycles_empty;
static bool estimated_empty;
static int estimated_maxhpos[2];

#if ESTIMATED_FETCH_MODE

static void end_estimate_last_fetch_cycle(int hpos)
{
#if OPTIMIZED_ESTIMATE
	if (estimate_cycles_empty_index >= 0) {
		for (int i = 0; i < RGA_PIPELINE_ADJUST; i++) {
			int pos = (estimate_cycles_empty_index + i) % maxhpos;
			estimated_cycles_empty[pos] = 0;
		}
		estimate_cycles_empty_index = -1;
	}
	if (estimated_cycles_empty != estimated_cycles) {
		estimated_cycles = estimated_cycles_empty;
		estimated_cycles_next = estimated_cycles_empty;
		estimate_cycles_empty_index = hpos;
		if (linetoggle) {
			uae_s8 *est = maxhposeven ? estimated_cycles_buf1 : estimated_cycles_buf0;
			uae_s8 *est_n = maxhposeven ? estimated_cycles_buf0 : estimated_cycles_buf1;
			if (maxhpos == estimated_maxhpos[0]) {
				est = estimated_cycles_buf0;
				est_n = estimated_cycles_buf1;
			} else if (maxhpos == estimated_maxhpos[1]) {
				est = estimated_cycles_buf1;
				est_n = estimated_cycles_buf0;
			}
			for (int i = 0; i < RGA_PIPELINE_ADJUST; i++) {
				int pos = hpos + i;
				if (pos >= maxhpos) {
					pos -= maxhpos;
					estimated_cycles_empty[pos] = est_n[pos];
				} else {
					estimated_cycles_empty[pos] = est[pos];
				}
			}
		} else {
			uae_s8 *est = maxhposeven ? estimated_cycles_buf1 : estimated_cycles_buf0;
			if (maxhpos == estimated_maxhpos[0]) {
				est = estimated_cycles_buf0;
			} else if (maxhpos == estimated_maxhpos[1]) {
				est = estimated_cycles_buf1;
			}
			for (int i = 0; i < RGA_PIPELINE_ADJUST; i++) {
				int pos = hpos + i;
				if (pos >= maxhpos) {
					pos -= maxhpos;
				}
				estimated_cycles_empty[pos] = est[pos];
			}
		}
	}
#else
	estimated_cycles = estimated_cycles_buf0;
	int start_pos = (hpos + RGA_PIPELINE_ADJUST) % maxhpos;
	if (start_pos >= hpos) {
		memset(estimated_cycles + start_pos, 0, maxhpos - start_pos);
		memset(estimated_cycles, 0, hpos);
	} else {
		memset(estimated_cycles + start_pos, 0, hpos - start_pos);
	}
#endif
	estimated_empty = true;
}

static void estimate_last_fetch_cycle(int hpos)
{
	if (!bprun) {

		end_estimate_last_fetch_cycle(hpos);

	} else {

#if OPTIMIZED_ESTIMATE
		if (estimated_bplcon0 == bplcon0 && bpl_hstart == hpos && estimated_plfstrt == bpl_hstart && estimated_plfstop == plfstop && estimated_fm == fetchmode) {
			if (linetoggle) {
				if (maxhpos == estimated_maxhpos[0]) {
					estimated_cycles = estimated_cycles_buf0;
					estimated_cycles_next = estimated_cycles_buf1;
					return;
				}
				if (maxhpos == estimated_maxhpos[1]) {
					estimated_cycles = estimated_cycles_buf1;
					estimated_cycles_next = estimated_cycles_buf0;
					return;
				}
			} else {
				if (maxhpos == estimated_maxhpos[0]) {
					estimated_cycles = estimated_cycles_buf0;
					estimated_cycles_next = estimated_cycles;
					return;
				}
				if (maxhpos == estimated_maxhpos[1]) {
					estimated_cycles = estimated_cycles_buf1;
					estimated_cycles_next = estimated_cycles;
					return;
				}
			}
		} else {
			estimated_maxhpos[0] = estimated_maxhpos[1] = -1;
		}
#endif
#if 0
		// plfstrt=24 plfstop=32 bpl_hstart=56 hard_ddf_stop=216 ddf_stopping=0 fetch_cycle=136 fetchunit=8 lastfetchunit=8
		bpl_hstart = 56;
		plfstrt = 24;
		plfstop = 32;
		bplcon0_res = 0;
		fetchmode = 0;
		fetch_cycle = 136;
		ddf_stopping = 0;
		hpos = 0;
		curr_diagram = cycle_diagram_table[fetchmode][bplcon0_res][8];
#endif
		int hard_ddf_stop = harddis_h ? 0x100 : HARD_DDF_STOP;
		int start = bpl_hstart;
		int adjusted_plfstop = plfstop;
		int estimated_cycle_count;
		int fetchunit = fetchunits[fetchmode * 4 + bplcon0_res];
		// Last fetch is always max 8 even if fetchunit is larger.
		int lastfetchunit = fetchunit >= 8 ? 8 : fetchunit;

		if (!ddf_stopping) {
			int stop;
			if (ecs_agnus) {
				// ECS: stop wins if start == stop
				stop = adjusted_plfstop < hpos || adjusted_plfstop > hard_ddf_stop ? hard_ddf_stop : adjusted_plfstop;
			} else {
				// OCS: start wins if start == stop
				stop = adjusted_plfstop <= hpos || adjusted_plfstop > hard_ddf_stop ? hard_ddf_stop : adjusted_plfstop;
			}
			/* We know that fetching is up-to-date up until hpos, so we can use fetch_cycle.  */
			if (start > stop) {
				// weird case handling
				if (stop < fetch_cycle) {
					stop = hard_ddf_stop;
				}
				int starting_last_block_at = (stop + fetchunit - 1) & ~(fetchunit - 1);
				estimated_cycle_count = (starting_last_block_at - fetch_cycle) + lastfetchunit;
				if (estimated_cycle_count < 0) {
					estimated_cycle_count += maxhpos;
				}
			} else {
				int fetch_cycle_at_stop = fetch_cycle + (stop - start);
				int starting_last_block_at = (fetch_cycle_at_stop + fetchunit - 1) & ~(fetchunit - 1);
				estimated_cycle_count = (starting_last_block_at - fetch_cycle) + lastfetchunit;
			}
		} else {
			int fc = fetch_cycle;
			int starting_last_block_at = (fc + fetchunit - 1) & ~(fetchunit - 1);
			if (ddf_stopping == 2) {
				if (starting_last_block_at <= fetchunit) {
					starting_last_block_at = 0;
				} else {
					starting_last_block_at -= fetchunit;
				}
			}
			estimated_cycle_count = (starting_last_block_at - fc) + lastfetchunit;
		}

#if OPTIMIZED_ESTIMATE
		estimated_cycles = maxhposeven ? estimated_cycles_buf1 : estimated_cycles_buf0;
		if (linetoggle) {
			estimated_cycles_next = maxhposeven ? estimated_cycles_buf0 : estimated_cycles_buf1;
		} else {
			estimated_cycles_next = estimated_cycles;
		}
#else
		estimated_cycles = estimated_cycles_buf0;
		estimated_cycles_next = estimated_cycles;
#endif
		uae_s8 *ecycs = estimated_cycles;
		// bitplane DMA end can wrap around, even in non-overrun cases
		int start_pos = (hpos + RGA_PIPELINE_ADJUST) % maxhpos;
		int start_pos2 = start_pos;
		int end_pos = (hpos + RGA_PIPELINE_ADJUST + estimated_cycle_count) % maxhpos;
		int off = (start_pos - (bpl_hstart + RGA_PIPELINE_ADJUST)) & (fetchunit - 1);
		while (start_pos != end_pos && end_pos >= 0) {
			int off2 = off & fetchstart_mask;

#if 0
			if (start_pos < 0 || start_pos > maxhpos || fetchstart > 32 || end_pos < 0 || end_pos > maxhpos) {
				gui_message(_T("ERROR: %d %d %d %d %d %d %d %d %d %d\n%d %d %d %d %d %d %d %d %04x %d\n"),
					hpos, bpl_hstart, estimated_cycle_count, start_pos, start_pos2, end_pos, off, off2, fetchstart, maxhpos,
					plfstrt, plfstop, hard_ddf_stop, ddf_stopping, fetch_cycle, fetchunit, lastfetchunit, vpos, bplcon0, fmode);
			}
#endif

			if (!off2 && start_pos + fetchstart <= end_pos) {
				memcpy(ecycs + start_pos, curr_diagram, fetchstart);
				start_pos += fetchstart;
			} else {
				ecycs[start_pos] = curr_diagram[off2];
				start_pos++;
				if (start_pos >= maxhpos) {
					start_pos = 0;
					if (linetoggle) {
						ecycs = estimated_cycles_next;
					}
				}
				if (start_pos == REFRESH_FIRST_HPOS) {
					// bpl sequencer repeated this cycle
					// (request was done at cycle 0)
					off--;
				}
				off++;
			}
		}
#if OPTIMIZED_ESTIMATE
		if (estimated_empty && bpl_hstart == hpos && !ddf_stopping) {
			estimated_empty = false;
			// zero rest of buffer
			if (end_pos != start_pos2) {
				if (end_pos > start_pos2) {
					memset(ecycs + end_pos, 0, maxhpos - end_pos);
					memset(ecycs, 0, start_pos2);
				} else {
					memset(ecycs + end_pos, 0, start_pos2 - end_pos);
				}
			}
			estimated_bplcon0 = bplcon0;
			estimated_plfstrt = bpl_hstart;
			estimated_plfstop = plfstop;
			estimated_fm = fetchmode;
			estimated_maxhpos[maxhposeven] = maxhpos;
		} else {
			estimated_fm = 0xffff;
			estimated_maxhpos[0] = -1;
			estimated_maxhpos[1] = -1;
		}
#endif
	}
}

#else

struct bpl_estimate {
	uae_u16 startend;
	uae_u16 start_pos;
	uae_u16 end_pos;
	uae_u16 vpos;
	uae_s8 *cycle_diagram;
	uae_u16 ce_offset;
};

#define MAX_BPL_ESTIMATES 2
static struct bpl_estimate bpl_estimates[MAX_BPL_ESTIMATES];
static int bpl_estimate_index;

static void end_estimate_last_fetch_cycle(int hpos)
{
	for (int i = 0; i < MAX_BPL_ESTIMATES; i++) {
		struct bpl_estimate *be = &bpl_estimates[i];
		if (be->startend && be->vpos == vposh) {
			if (be->end_pos > hpos + RGA_PIPELINE_ADJUST) {
				be->end_pos = hpos + RGA_PIPELINE_ADJUST;
			} else if (be->start_pos > be->end_pos && ((hpos + RGA_PIPELINE_ADJUST) % maxhpos) > be->start_pos) {
				be->end_pos = (hpos + RGA_PIPELINE_ADJUST) % maxhpos;
				be->startend = 1;
			}
		}
	}
}

static void estimate_last_fetch_cycle(int hpos)
{
	int hard_ddf_stop = harddis_h ? 0x100 : HARD_DDF_STOP;
	int start = bpl_hstart;
	int adjusted_plfstop = plfstop;
	int estimated_cycle_count;
	int fetchunit = fetchunits[fetchmode * 4 + bplcon0_res];
	// Last fetch is always max 8 even if fetchunit is larger.
	int lastfetchunit = fetchunit >= 8 ? 8 : fetchunit;

	end_estimate_last_fetch_cycle(hpos);

	if (!bprun) {
		return;
	}

	bpl_estimate_index++;
	bpl_estimate_index &= MAX_BPL_ESTIMATES - 1;
	struct bpl_estimate *be = &bpl_estimates[bpl_estimate_index];

	if (!ddf_stopping) {
		int stop;
		if (ecs_agnus) {
			// ECS: stop wins if start == stop
			stop = adjusted_plfstop < hpos || adjusted_plfstop > hard_ddf_stop ? hard_ddf_stop : adjusted_plfstop;
		} else {
			// OCS: start wins if start == stop
			stop = adjusted_plfstop <= hpos || adjusted_plfstop > hard_ddf_stop ? hard_ddf_stop : adjusted_plfstop;
		}
		/* We know that fetching is up-to-date up until hpos, so we can use fetch_cycle.  */
		int fetch_cycle_at_stop = fetch_cycle + (stop - start);
		int starting_last_block_at = (fetch_cycle_at_stop + fetchunit - 1) & ~(fetchunit - 1);
		estimated_cycle_count = (starting_last_block_at - fetch_cycle) + lastfetchunit;
	} else {
		int fc = fetch_cycle;
		int starting_last_block_at = (fc + fetchunit - 1) & ~(fetchunit - 1);
		if (ddf_stopping == 2) {
			starting_last_block_at -= fetchunit;
		}
		estimated_cycle_count = (starting_last_block_at - fc) + lastfetchunit;
	}

	// bitplane DMA end can wrap around, even in non-overrun cases
	be->start_pos = (hpos + RGA_PIPELINE_ADJUST) % maxhpos;
	be->end_pos = (hpos + RGA_PIPELINE_ADJUST + estimated_cycle_count) % maxhpos;
	if (be->end_pos > be->start_pos) {
		be->startend = 1;
	} else {
		be->startend = 2;
	}
	be->vpos = vposh;
	be->cycle_diagram = curr_diagram;
	be->ce_offset = ((bpl_hstart + RGA_PIPELINE_ADJUST) - be->start_pos) & (fetchunit - 1);
}
#endif

static void calcdiw(void);
static void set_chipset_mode(void)
{
	fmode = fmode_saved;
	bplcon0 = bplcon0_saved;
	bplcon1 = bplcon1_saved;
	bplcon2 = bplcon2_saved;
	bplcon3 = bplcon3_saved;
	bplcon4 = bplcon4_saved;
	if (!aga_mode) {
		fmode = 0;
		bplcon0 &= ~(0x0010 | 0x0020);
		bplcon1 &= ~(0xff00);
		bplcon2 &= ~(0x0100 | 0x0080);
		bplcon3 &= 0x003f;
		bplcon3 |= 0x0c00;
		bplcon4 = 0x0011;
		if (!ecs_agnus) {
			bplcon0 &= ~0x0080;
			diwhigh_written = 0;
		}
		if (!ecs_denise) {
			bplcon0 &= ~0x0001;
			bplcon2 &= 0x007f;
			bplcon3 = 0x0c00;
		}
	}
	sprres = expand_sprres(bplcon0, bplcon3);
	sprite_width = GET_SPRITEWIDTH(fmode);
	for (int i = 0; i < MAX_SPRITES; i++) {
		spr[i].width = sprite_width;
	}
	shdelay_disabled = false;
	exthblank_state = false;
	refmask = 0x1fffff;
	ref_ras_add = REF_RAS_ADD_AGA;
	if (!aga_mode) {
		ref_ras_add = REF_RAS_ADD_ECS;
		refmask >>= 1;
		if (!ecs_agnus) {
			ref_ras_add = REF_RAS_ADD_OCS;
			refmask >>= 1;
		}
	}
	calcdiw();
}

static void update_mirrors(void)
{
	aga_mode = (currprefs.chipset_mask & CSMASK_AGA) != 0;
	ecs_agnus = (currprefs.chipset_mask & CSMASK_ECS_AGNUS) != 0;
	ecs_denise = (currprefs.chipset_mask & CSMASK_ECS_DENISE) != 0;
	agnusa1000 = currprefs.cs_agnusmodel == AGNUSMODEL_A1000 || currprefs.cs_agnusmodel == AGNUSMODEL_VELVET;
	if (agnusa1000) {
		ecs_agnus = false;
		aga_mode = false;
	}
	denisea1000_noehb = currprefs.cs_denisemodel == DENISEMODEL_VELVET || currprefs.cs_denisemodel == DENISEMODEL_A1000NOEHB;
	denisea1000 = currprefs.cs_denisemodel == DENISEMODEL_VELVET || currprefs.cs_denisemodel == DENISEMODEL_A1000NOEHB || currprefs.cs_denisemodel == DENISEMODEL_A1000;
	direct_rgb = aga_mode;
	if (aga_mode) {
		sprite_sprctlmask = 0x01 | 0x08 | 0x10;
	} else if (ecs_denise) {
		sprite_sprctlmask = 0x01 | 0x10;
	} else {
		sprite_sprctlmask = 0x01;
	}
	if (aga_mode) {
		for (int i = 0; i < 256; i++) {
			current_colors.acolors[i] = getxcolor(current_colors.color_regs_aga[i]);
		}
	}
	set_chipset_mode();
	hsyncdebug = 0;
	if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
		hsyncdebug = currprefs.gfx_overscanmode - OVERSCANMODE_ULTRA + 1;
	}
}

extern struct color_entry colors_for_drawing;

void notice_new_xcolors(void)
{
	update_mirrors();
	docols(&current_colors);
	docols(&colors_for_drawing);
	for (int i = 0; i < (MAXVPOS + 1) * 2; i++) {
		docols(color_tables[0] + i);
		docols(color_tables[1] + i);
	}
}

static bool isehb(uae_u16 bplcon0, uae_u16 bplcon2)
{
	bool bplehb;
	if (aga_mode) {
		bplehb = (bplcon0 & 0x7010) == 0x6000;
	} else if (ecs_denise) {
		bplehb = ((bplcon0 & 0xFC00) == 0x6000 || (bplcon0 & 0xFC00) == 0x7000);
	} else {
		bplehb = ((bplcon0 & 0xFC00) == 0x6000 || (bplcon0 & 0xFC00) == 0x7000) && !denisea1000_noehb;
	}
	return bplehb;
}

// OCS/ECS, lores, 7 planes = 4 "real" planes + BPL5DAT and BPL6DAT as static 5th and 6th plane
STATIC_INLINE int isocs7planes(void)
{
	return !aga_mode && bplcon0_res == 0 && bplcon0_planes == 7;
}

int get_sprite_dma_rel(int hpos, int off)
{
	int offset = get_rga_pipeline(hpos, off);
	uae_u16 v = cycle_line_pipe[offset];
	if (v & CYCLE_PIPE_SPRITE) {
		return v & 7;
	}
	return -1;
}

int get_bitplane_dma_rel(int hpos, int off)
{
	int offset = get_rga_pipeline(hpos, off);
	uae_u16 v = cycle_line_pipe[offset];
	if (v & CYCLE_PIPE_BITPLANE) {
		return v & 0x0f;
	}
	return 0;
}

static void compute_shifter_mask(void)
{
	int shifter_size = 16 << (fetchmode + LORES_TO_SHRES_SHIFT - toscr_res);
	shifter_mask = shifter_size - 1;
}

/* Expand bplcon0/bplcon1 into the toscr_xxx variables.  */
static void compute_toscr_delay(int bplcon1)
{
	int delay1 = (bplcon1 & 0x0f) | ((bplcon1 & 0x0c00) >> 6);
	int delay2 = ((bplcon1 >> 4) & 0x0f) | (((bplcon1 >> 4) & 0x0c00) >> 6);
	int delaymask = fetchmode_mask >> toscr_res;

	compute_shifter_mask();

	toscr_delay_shifter[0] = (delay1 & delaymask) << LORES_TO_SHRES_SHIFT;
	toscr_delay_shifter[1] = (delay2 & delaymask) << LORES_TO_SHRES_SHIFT;

	int shdelay1 = (bplcon1 >> 8) & 3;
	int shdelay2 = (bplcon1 >> 12) & 3;
	if (!currprefs.chipset_hr) {
		// subpixel scrolling not possible
		if (toscr_res == RES_HIRES) {
			shdelay1 &= ~1;
			shdelay2 &= ~1;
		} else if (toscr_res == RES_LORES) {
			shdelay1 &= ~3;
			shdelay2 &= ~3;
		}
	}
	toscr_delay_shifter[0] |= shdelay1;
	toscr_delay_shifter[1] |= shdelay2;

	if (toscr_delay_shifter[0] != toscr_delay_shifter[1] || 1) {
		toscr_scanline_complex_bplcon1 = true;
		toscr_scanline_complex_bplcon1_off = false;
	} else {
		if (toscr_scanline_complex_bplcon1) {
			toscr_scanline_complex_bplcon1_off = true;
		}
	}

#if SPEEDUP
	/* SPEEDUP code still needs this hack */
	int	delayoffset = fetchmode_size - (((bpl_hstart - 0x18) & fetchstart_mask) << 1);
	delay1 += delayoffset;
	delay2 += delayoffset;
	toscr_delay_adjusted[0] = (delay1 & delaymask) << toscr_res;
	toscr_delay_adjusted[0] |= shdelay1 >> (RES_MAX - toscr_res);
	toscr_delay_adjusted[1] = (delay2 & delaymask) << toscr_res;
	toscr_delay_adjusted[1] |= shdelay2 >> (RES_MAX - toscr_res);
	toscr_nr_planes = GET_PLANES_LIMIT(bplcon0);
#endif
}

static void set_delay_lastcycle(void)
{
	delay_lastcycle[0] = (((maxhpos + 0) * 2) + 2) << LORES_TO_SHRES_SHIFT;
	delay_lastcycle[1] = delay_lastcycle[0];
	if (linetoggle) {
		delay_lastcycle[1] += 1 << LORES_TO_SHRES_SHIFT;
	}
	delay_hsynccycle = (((maxhpos + hsyncstartpos_start_cycles) * 2) - DDF_OFFSET) << LORES_TO_SHRES_SHIFT;
}

static void update_toscr_vars(void)
{
	toscr_res_pixels_shift = 2 - toscr_res;
	toscr_res_pixels_shift_hr = 2 - currprefs.gfx_resolution;
	if (currprefs.chipset_hr) {
		if (toscr_res < currprefs.gfx_resolution) {
			toscr_res_mult = currprefs.gfx_resolution - toscr_res;
		} else {
			toscr_res_mult = 0;
		}
		toscr_res_pixels_mask_hr = (1 << toscr_res_mult) - 1;
		if (toscr_res > currprefs.gfx_resolution) {
			toscr_res_pixels_shift_hr -= toscr_res - currprefs.gfx_resolution;
			toscr_res_pixels_shift_hr = std::max(toscr_res_pixels_shift_hr, 0);
		}
		toscr_res_pixels_hr = 1 << toscr_res_pixels_shift_hr;
	} else {
		toscr_res_mult = 0;
	}
	toscr_res_pixels = 1 << toscr_res_pixels_shift;
	toscr_res2p = 2 << toscr_res;

}

/* fetchstart_mask can be larger than fm_maxplane if FMODE > 0.
   This means that the remaining cycles are idle.
 */
static const uae_u8 bpl_sequence_8[32] = { 8, 4, 6, 2, 7, 3, 5, 1 };
static const uae_u8 bpl_sequence_4[32] = { 4, 2, 3, 1 };
static const uae_u8 bpl_sequence_2[32] = { 2, 1 };
static const uae_u8 *bpl_sequence;

/* set currently active Agnus bitplane DMA sequence */
static void setup_fmodes(int hpos, uae_u16 con0)
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
	if (GET_RES_AGNUS(con0) != GET_RES_DENISE(con0)) {
		SET_LINE_CYCLEBASED(hpos);
	}
	bplcon0_res = GET_RES_AGNUS(con0);
	toscr_res_old = -1;
	update_toscr_vars();

	bplcon0_planes = GET_PLANES(con0);
	bplcon0_planes_limit = GET_PLANES_LIMIT(con0);
	fetchunit = fetchunits[fetchmode * 4 + bplcon0_res];
	fetchunit_mask = fetchunit - 1;
	fetchstart_shift = fetchstarts[fetchmode * 4 + bplcon0_res];
	fetchstart = 1 << fetchstart_shift;
	fetchstart_mask = fetchstart - 1;
	fm_maxplane_shift = fm_maxplanes[fetchmode * 4 + bplcon0_res];
	fm_maxplane = 1 << fm_maxplane_shift;
	switch (fm_maxplane) {
	default:
	case 8:
		bpl_sequence = bpl_sequence_8;
		break;
	case 4:
		bpl_sequence = bpl_sequence_4;
		break;
	case 2:
		bpl_sequence = bpl_sequence_2;
		break;
	}
	fetch_modulo_cycle = fetchunit - fetchstart;
	fetchmode_size = 16 << fetchmode;
	fetchmode_bytes = 2 << fetchmode;
	fetchmode_mask = fetchmode_size - 1;
	fetchmode_fmode_bpl = fmode & 3;
	fetchmode_fmode_spr = (fmode >> 2) & 3;
	compute_toscr_delay(bplcon1);

	if (thisline_decision.plfleft < 0) {
		thisline_decision.bplres = output_res(bplcon0_res);
		thisline_decision.bplcon0 = con0;
		thisline_decision.nr_planes = bplcon0_planes;
	}

	curr_diagram = cycle_diagram_table[fetchmode][bplcon0_res][bplcon0_planes_limit];
	estimate_last_fetch_cycle(hpos);
	toscr_nr_planes_agnus = bplcon0_planes;
	if (isocs7planes()) {
		toscr_nr_planes_agnus = 6;
	}
	SET_LINE_CYCLEBASED(hpos);
}

static int REGPARAM2 custom_wput_1(int hpos, uaecptr addr, uae_u32 value, int noget);

static uae_u16 get_strobe_reg(int slot)
{
	uae_u16 strobe = 0x1fe;
	if (slot == 0) {
		strobe = 0x3c;
		if (vb_start_line > 1 && vpos < get_equ_vblank_endline() && vpos >= get_equ_vblank_startline()) {
			strobe = 0x38;
			if (vpos == 0) {
				if (agnusa1000) {
					strobe = 0x3c;
				} else if (!ecs_agnus) {
					strobe = 0x3a;
				}
			}
		} else if (vb_start_line > 1 || vb_end_line || vb_end_next_line) {
			strobe = 0x3a;
		}
	} else if (slot == 1 && ecs_agnus && lol) {
		strobe = 0x3e;
	}
	return strobe;
}

static uae_u16 fetch16(uaecptr p, int nr)
{
	uae_u16 v;
	if (aga_mode) {
		// AGA always does 32-bit fetches, this is needed
		// to emulate 64 pixel wide sprite side-effects.
		uae_u32 vv = chipmem_lget_indirect(p & ~3);
		if (p & 2) {
			v = (uae_u16)vv;
			if (nr >= 0) {
				fetched_aga_spr[nr] = (v << 16) | v;
			}
		} else {
			v = vv >> 16;
			if (nr >= 0) {
				fetched_aga_spr[nr] = vv;
			}
		}
	} else {
		v = chipmem_wget_indirect(p);
	}
#ifdef DEBUGGER
	if (memwatch_enabled) {
		debug_getpeekdma_value(v);
	}
	if (debug_dma) {
		record_dma_read_value(v);
	}
#endif
	return v;
}

static uae_u32 fetch32(uaecptr p)
{
	uae_u32 v;
	uaecptr pm = p & ~3;
	if (p & 2) {
		v = chipmem_lget_indirect(pm) & 0x0000ffff;
		v |= v << 16;
	} else if (fetchmode_fmode_bpl & 2) { // optimized (fetchmode_fmode & 3) == 2
		v = chipmem_lget_indirect(pm) & 0xffff0000;
		v |= v >> 16;
	} else {
		v = chipmem_lget_indirect(pm);
	}
#ifdef DEBUGGER
	if (memwatch_enabled) {
		debug_getpeekdma_value_long(v, p - pm);
	}
	if (debug_dma) {
		record_dma_read_value_wide(v, false);
	}
#endif
	return v;
}

static uae_u64 fetch64(uaecptr p)
{
	uae_u64 v;
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
		v = (((uae_u64)v1) << 32) | v2;
	} else {
		v = ((uae_u64)chipmem_lget_indirect(pm1)) << 32;
		v |= chipmem_lget_indirect(pm2);
	}
#ifdef DEBUGGER
	if (memwatch_enabled) {
		debug_getpeekdma_value_long((uae_u32)(v >> 32), p - pm1);
		debug_getpeekdma_value_long((uae_u32)(v >> 0), p - pm2);
	}
	if (debug_dma) {
		record_dma_read_value_wide(v, true);
	}
#endif
	return v;
}

// Nasty chipset bitplane/sprite DMA conflict bug
//
// If bitplane DMA ends before sprite slots and last bitplane slot also has active sprite slot, sprite DMA is not stolen by bitplane DMA but sprite DMA conflicts with bitplane DMA:
// Target DMA custom register becomes AND of last bitplane BPLxDAT register and conflicting sprite register, for example 0x110 (BPL1DAT) & 0x168 (SPR5POS) = 0x100 (BPLCON0).
// Source DMA address becomes OR of bitplane and sprite pointer addresses.
// DMA reads word (or larger if FMODE>0) from OR'd DMA address and writes it to new target custom register.
// Bitplane +2/+4/+8 and possible modulo is added to OR'd DMA address.
// DMA address gets written back to both bitplane and sprite pointers.
// This is fully repeatable, no random side-effects noticed.

static int bplsprchipsetbug(int nr, int fm, int hpos)
{
	uae_u16 breg = 0x110 + nr * 2;
	uae_u16 sreg = sprbplconflict;
	uae_u16 creg = breg & sreg;
	int snum = (sreg - 0x140) / 8;
	uaecptr px = bplpt[nr] | spr[snum].pt;

	if (creg < 0x110 || creg >= 0x120) {
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(creg, px, hpos, vpos, DMARECORD_BITPLANE, nr);
		}
#endif
	}

	uae_u64 v;
	uae_u16 v2;
	if (fm == 0) {
		v = fetch16(px, -1);
		v2 = (uae_u16)v;
	} else if (fm == 1) {
		v = fetch32(px);
		v2 = (uae_u16)(v >> 16);
	} else {
		v = fetch64(px);
		v2 = v >> 48;
	}
	bplpt[nr] = px;

	if (creg < 0x110 || creg >= 0x120) {
		custom_wput_1(hpos, creg, v2, 1);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value_wide(v, false);
			record_dma_read(breg, px, hpos, vpos, DMARECORD_BITPLANE, nr);
		}
#endif
		nr = -1;

	} else {

		uaecptr px = bplpt[nr] | spr[snum].pt;
		spr[snum].pt = px;
		bplpt[nr] = px;

		nr = (creg - 0x110) / 2;
	}

	return nr;
}

bool get_ras_cas(uaecptr addr, int *rasp, int *casp)
{
	int ras, cas;
	bool ret = false;
	if (aga_mode) {
		ras = (((addr >> 12) & 127) << 1) | ((addr >> 9) & 1);
		cas = (addr >> 0) & 0xff;
		if ((addr >> 8) & 1) {
			ras |= 0x100;
		}
		if ((addr >> 19) & 1) {
			ras |= 0x200;
		}
		if ((addr >> 10) & 1) {
			cas |= 0x100;
		}
		if ((addr >> 11) & 1) {
			cas |= 0x200;
		}
	} else if (ecs_agnus && currprefs.chipmem.size > 0x100000) {
		ras = (addr >> 9) & 0x1ff;
		cas = (addr >> 1) & 0xff;
		if ((addr >> 19) & 1) {
			ras |= 0x200;
		}
		if ((addr >> 18) & 1) {
			cas |= 0x100;
		}
	} else if (ecs_agnus) {
		ras = (addr >> 9) & 0x1ff;
		cas = (addr >> 1) & 0xff;
		if ((addr >> 19) & 1) {
			ret = true;
		}
		if ((addr >> 18) & 1) {
			cas |= 0x100;
		}
	} else {
		ras = (addr >> 1) & 0xff;
		cas = (addr >> 9) & 0xff;
		if ((addr >> 17) & 1) {
			ras |= 0x100;
		}
		if ((addr >> 18) & 1) {
			cas |= 0x100;
		}
	}
	*rasp = ras;
	*casp = cas;
	return ret;
}


static uaecptr update_refptr(int slot, int end, bool process, bool overwrite)
{
	if (refptr_preupdated) {
		refptr_preupdated = false;
		refptr = refptr_p;
	}

	uaecptr rp = refptr;
	for (int i = slot + 1; i < end; i++) {
		if (!(refresh_handled_slot & (1 << i))) {
#ifdef DEBUGGER
			if (debug_dma) {
				int hp = REFRESH_FIRST_HPOS + i * 2;
				if (overwrite || (!overwrite && !record_dma_check(hp, vpos))) {
					uae_u16 strobe = get_strobe_reg(i);
					record_dma_clear(hp, vpos);
					record_dma_read(strobe, rp & refmask, hp, vpos, DMARECORD_REFRESH, i);
					record_dma_read_value(0xffff);
				}
			}
#endif
			rp += ref_ras_add;
		}
		if (process) {
			refresh_handled_slot |= 1 << i;
		}
	}
	return rp;
}

// Strobe+refresh (always first, second possible if ECS and NTSC) slot conflict
static void fetch_strobe_conflict(int nr, int fm, int hpos, bool addmodulo)
{
	int slot = (hpos - REFRESH_FIRST_HPOS) / 2;
	static int warned1 = 30;

	refptr = update_refptr(-1, slot, true, true);
	refresh_handled_slot |= 1 << slot;

	uae_u16 rreg = get_strobe_reg(slot);
	uae_u16 breg = 0x110 + nr * 2;
	uae_u16 creg = rreg & breg;

	uaecptr bplptx = bplpt[nr];
	uaecptr refptx = refptr;
	uaecptr px = bplptx | refptx;

	if (creg != rreg) {
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_clear(hpos, vpos);
			record_dma_read(creg, px, hpos, vpos, DMARECORD_REFRESH, nr);
		}
#endif
	}

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read_value_wide(0xffff, false);
		record_dma_read(breg, px, hpos, vpos, DMARECORD_BITPLANE, nr);
	}
#endif

	if (addmodulo) {
		// modulo is OR'd with REF_RAS_ADD
		bplpt[nr] = px;
		uae_s16 m1 = bpl1mod, m2 = bpl2mod;
		uae_s16 m3 = bpl1mod_prev, m4 = bpl2mod_prev;
		bpl1mod |= ref_ras_add;
		bpl2mod |= ref_ras_add;
		bpl1mod_prev |= ref_ras_add;
		bpl2mod_prev |= ref_ras_add;
		add_modulo(hpos, nr);
		bpl1mod = m1; bpl2mod = m2;
		bpl1mod_prev = m3; bpl2mod_prev = m4;
		px = bplpt[nr];
	} else {
		px += ref_ras_add;
	}

	px &= refmask;
	bplpt[nr] = px;
	refptr = px;
	update_refptr(slot, 4, false, true);

	if (warned1 >= 0) {
		write_log(_T("WARNING: BPL strobe refresh conflict, slot %d!\n"), slot);
		warned1--;
	}

	// decide sprites before sprite offset change
	decide_sprites(hpos + 1, true, false);

	hstrobe_conflict = true;

	SET_LINE_CYCLEBASED(hpos);
}

// refresh only slot conflict
static void fetch_refresh_conflict(int nr, int fm, int hpos, bool addmodulo)
{
	int slot = (hpos - REFRESH_FIRST_HPOS) / 2;
	static int warned1 = 30;

	refptr = update_refptr(-1, slot, true, true);
	refresh_handled_slot |= 1 << slot;

	uaecptr bplptx = bplpt[nr];
	uaecptr refptx = refptr;
	uaecptr px = bplptx | refptx;
	uaecptr p = px;

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_clear(hpos, vpos);
		record_dma_read(0x1fe, px, hpos, vpos, DMARECORD_REFRESH, slot);
	}
#endif

	if (addmodulo) {
		// modulo is OR'd with REF_RAS_ADD
		bplpt[nr] = px;
		uae_s16 m1 = bpl1mod, m2 = bpl2mod;
		uae_s16 m3 = bpl1mod_prev, m4 = bpl2mod_prev;
		bpl1mod |= ref_ras_add;
		bpl2mod |= ref_ras_add;
		bpl1mod_prev |= ref_ras_add;
		bpl2mod_prev |= ref_ras_add;
		add_modulo(hpos, nr);
		bpl1mod = m1; bpl2mod = m2;
		bpl1mod_prev = m3; bpl2mod_prev = m4;
		px = bplpt[nr];
	} else {
		px += ref_ras_add;
	}

	px &= refmask;
	bplpt[nr] = px;
	refptr = px;
	update_refptr(slot, 4, false, true);

	if (warned1 >= 0) {
		write_log(_T("WARNING: BPL refresh conflict, slot %d!\n"), slot);
		warned1--;
	}

	SET_LINE_CYCLEBASED(hpos);

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read(0x110 + nr * 2, p, hpos, vpos, DMARECORD_BITPLANE, nr);
	}
	if (memwatch_enabled) {
		debug_getpeekdma_chipram(p, MW_MASK_BPL_0 << nr, 0x110 + nr * 2, 0xe0 + nr * 4);
	}
#endif

	switch (fm)
	{
		case 0:
		{
			fetched_aga[nr] = fetched[nr] = 0xffff;
			regs.chipset_latch_rw = 0xffff;
			last_custom_value = (uae_u16)regs.chipset_latch_rw;
			break;
		}
#ifdef AGA
		case 1:
		{
			fetched_aga[nr] = 0xffffffff;
			regs.chipset_latch_rw = (uae_u32)fetched_aga[nr];
			last_custom_value = (uae_u16)regs.chipset_latch_rw;
			fetched[nr] = (uae_u16)fetched_aga[nr];
			break;
		}
		case 2:
		{
			fetched_aga[nr] = 0xffffffffffffffff;
			regs.chipset_latch_rw = (uae_u32)fetched_aga[nr];
			last_custom_value = (uae_u16)regs.chipset_latch_rw;
			fetched[nr] = (uae_u16)fetched_aga[nr];
			break;
		}
#endif
	}
}

static bool fetch(int nr, int fm, int hpos, bool addmodulo)
{
	bool sprbplconflict = false;
	bool stroberefbplconflict = false;
	bool refbplconflict = false;
	int snum = 0;

	uae_u8 dmaslot = cycle_line_slot[hpos];
	uaecptr p = bplpt[nr];

	if (dmaslot > CYCLE_BITPLANE) {
		if (dmaslot == CYCLE_STROBE) {
			// strobe refresh conflict
			fetch_strobe_conflict(nr, fm, hpos, addmodulo);
			return false;
		} else if (dmaslot == CYCLE_REFRESH) {
			// refresh conflict
			fetch_refresh_conflict(nr, fm, hpos, addmodulo);
			return nr == 0;
		} else if (dmaslot == CYCLE_MISC) {
			// DMAL conflict
			// AUDxDAT AND BPLxDAT = read-only register
			// DSKDATR AND BLPxDAT = read-only register
			// DSKDAT AND BPLxDAT = read-only register
			return false;
		}
	} else {
		if (hpos == sprbplconflict_hpos) {
			nr = bplsprchipsetbug(nr, fm, hpos);
			if (nr < 0) {
				return false;
			}
			p = bplpt[nr];
		}

		// normal BPL cycle
		cycle_line_slot[hpos] = CYCLE_BITPLANE;

		bplpt[nr] += fetchmode_bytes;
		bplptx[nr] += fetchmode_bytes;
		if (addmodulo) {
			add_modulo(hpos, nr);
		}

	}

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read(0x110 + nr * 2, p, hpos, vpos, DMARECORD_BITPLANE, nr);
	}
	if (memwatch_enabled) {
		debug_getpeekdma_chipram(p, MW_MASK_BPL_0 << nr, 0x110 + nr * 2, 0xe0 + nr * 4);
	}
#endif

	switch (fm)
	{
	case 0:
	{
		uae_u16 v = fetch16(p, nr);
		fetched_aga[nr] = fetched[nr] = v;
		regs.chipset_latch_rw = v;
		last_custom_value = (uae_u16)regs.chipset_latch_rw;
		break;
	}
#ifdef AGA
	case 1:
	{
		fetched_aga[nr] = fetch32(p);
		regs.chipset_latch_rw = (uae_u32)fetched_aga[nr];
		last_custom_value = (uae_u16)regs.chipset_latch_rw;
		fetched[nr] = (uae_u16)fetched_aga[nr];
		break;
	}
	case 2:
	{
		fetched_aga[nr] = fetch64(p);
		regs.chipset_latch_rw = (uae_u32)fetched_aga[nr];
		last_custom_value = (uae_u16)regs.chipset_latch_rw;
		fetched[nr] = (uae_u16)fetched_aga[nr];
		break;
	}
#endif
	}

	return nr == 0;
}

STATIC_INLINE void toscr_3_ecs(int oddeven, int step, int nbits)
{
	int shift = 16 - nbits;

	for (int i = oddeven; i < toscr_nr_planes2 && nbits; i += step) {
		outword[i] <<= nbits;
		outword[i] |= todisplay2[i] >> shift;
		todisplay2[i] <<= nbits;
	}
}

#ifdef AGA

STATIC_INLINE void toscr_3_aga(int oddeven, int step, int nbits, int fm_size)
{
	uae_u32 mask = 0xFFFF >> (16 - nbits);
	int shift = fm_size - nbits;

	if (shift == 64) {
		for (int i = oddeven; i < toscr_nr_planes2; i += step) {
			outword[i] <<= nbits;
			todisplay2_aga[i] <<= nbits;
		}
	} else {
		for (int i = oddeven; i < toscr_nr_planes2 && bits; i += step) {
			outword[i] <<= nbits;
			outword[i] |= (todisplay2_aga[i] >> shift) & mask;
			todisplay2_aga[i] <<= nbits;
		}
	}
}

// very, very slow and unoptimized..
STATIC_INLINE void toscr_3_aga_hr(int oddeven, int step, int nbits, int fm_size_minusone)
{
	for (int i = oddeven; i < toscr_nr_planes2; i += step) {
		int subpix = out_subpix[oddeven];
		for (int j = 0; j < nbits; j++) {
			uae_u32 bit = (todisplay2_aga[i] >> fm_size_minusone) & 1;
			outword64[i] <<= 1;
			outword64[i] |= bit;
			subpix++;
			subpix &= toscr_res_pixels_mask_hr;
			if (subpix == 0) {
				todisplay2_aga[i] <<= 1;
			}
		}
	}
	out_subpix[oddeven] += nbits;
}

#endif

static void toscr_2_0(int nbits) { toscr_3_ecs(0, 1, nbits); }
static void toscr_2_0_oe(int oddeven, int step, int nbits) { toscr_3_ecs(oddeven, step, nbits); }
#ifdef AGA
static void toscr_2_1(int nbits) { toscr_3_aga(0, 1, nbits, 32); }
static void toscr_2_1_oe(int oddeven, int step, int nbits) { toscr_3_aga(oddeven, step, nbits, 32); }
static void toscr_2_2(int nbits) { toscr_3_aga(0, 1, nbits, 64); }
static void toscr_2_2_oe(int oddeven, int step, int nbits) { toscr_3_aga(oddeven, step, nbits, 64); }

static void toscr_2_0_hr(int nbits) { toscr_3_aga_hr(0, 1, nbits, 16 - 1); }
static void toscr_2_0_hr_oe(int oddeven, int step, int nbits) { toscr_3_aga_hr(oddeven, step, nbits, 16 - 1); }
static void toscr_2_1_hr(int nbits) { toscr_3_aga_hr(0, 1, nbits, 32 - 1); }
static void toscr_2_1_hr_oe(int oddeven, int step, int nbits) { toscr_3_aga_hr(oddeven, step, nbits, 32 - 1); }
static void toscr_2_2_hr(int nbits) { toscr_3_aga_hr(0, 1, nbits, 64 - 1); }
static void toscr_2_2_hr_oe(int oddeven, int step, int nbits) { toscr_3_aga_hr(oddeven, step, nbits, 64 - 1); }
#endif

STATIC_INLINE void do_tosrc(int oddeven, int step, int nbits, int fm)
{
	switch (fm) {
	case 0:
		if (step == 2)
			toscr_2_0_oe(oddeven, step, nbits);
		else
			toscr_2_0(nbits);
		break;
#ifdef AGA
	case 1:
		if (step == 2)
			toscr_2_1_oe(oddeven, step, nbits);
		else
			toscr_2_1(nbits);
		break;
	case 2:
		if (step == 2)
			toscr_2_2_oe(oddeven, step, nbits);
		else
			toscr_2_2(nbits);
		break;
#endif
	}
}

#ifdef AGA
STATIC_INLINE void do_tosrc_hr(int oddeven, int step, int nbits, int fm)
{
	switch (fm) {
	case 0:
		if (step == 2)
			toscr_2_0_hr_oe(oddeven, step, nbits);
		else
			toscr_2_0_hr(nbits);
		break;
	case 1:
		if (step == 2)
			toscr_2_1_hr_oe(oddeven, step, nbits);
		else
			toscr_2_1_hr(nbits);
		break;
	case 2:
		if (step == 2)
			toscr_2_2_hr_oe(oddeven, step, nbits);
		else
			toscr_2_2_hr(nbits);
		break;
	}
}
#endif

STATIC_INLINE void do_delays_3_ecs(int nbits)
{
	int delaypos = delay_cycles;
	for (int oddeven = 0; oddeven < 2; oddeven++) {
		int delay = toscr_delay_shifter[oddeven];
		int diff = ((delay - delaypos) & shifter_mask) >> toscr_res_pixels_shift;
		int nbits2 = nbits;
		if (nbits2 > diff) {
			do_tosrc(oddeven, 2, diff, 0);
			nbits2 -= diff;
			if (todisplay_fetched & (oddeven + 1)) {
				for (int i = oddeven; i < toscr_nr_planes_shifter; i += 2) {
					todisplay2[i] = todisplay[i];
				}
				todisplay_fetched -= oddeven + 1;
			}
		}
		if (nbits2) {
			do_tosrc(oddeven, 2, nbits2, 0);
		}
	}
}

STATIC_INLINE void do_delays_fast_3_ecs(int nbits)
{
	int delaypos = delay_cycles;
	int delay = toscr_delay_shifter[0];
	int diff = ((delay - delaypos) & shifter_mask) >> toscr_res_pixels_shift;
	int nbits2 = nbits;
	if (nbits2 > diff) {
		do_tosrc(0, 1, diff, 0);
		nbits2 -= diff;
		if (todisplay_fetched) {
			memcpy(todisplay2, todisplay, toscr_nr_planes_shifter * sizeof(uae_u32));
			todisplay_fetched = 0;
		}
	}
	if (nbits2) {
		do_tosrc(0, 1, nbits2, 0);
	}
}

STATIC_INLINE void do_delays_3_aga(int nbits, int fm)
{
	int delaypos = delay_cycles;
	for (int oddeven = 0; oddeven < 2; oddeven++) {
		int delay = toscr_delay_shifter[oddeven];
		int diff = ((delay - delaypos) & shifter_mask) >> toscr_res_pixels_shift;
		int nbits2 = nbits;
		if (nbits2 > diff) {
			do_tosrc(oddeven, 2, diff, fm);
			nbits2 -= diff;
			if (todisplay_fetched & (oddeven + 1)) {
				for (int i = oddeven; i < toscr_nr_planes_shifter; i += 2) {
					todisplay2_aga[i] = todisplay_aga[i];
				}
				todisplay_fetched -= oddeven + 1;
			}
		}
		if (nbits2) {
			do_tosrc(oddeven, 2, nbits2, fm);
		}
	}
}

static const uae_u64 firstpixel[] = { 0x8000, 0x80000000, 0x8000000000000000, 0x8000000000000000 };

STATIC_INLINE void do_delays_fast_3_aga(int nbits, int fm)
{
	int delaypos = delay_cycles;
	int delay = toscr_delay_shifter[0];
	int diff = ((delay - delaypos) & shifter_mask) >> toscr_res_pixels_shift;
	int nbits2 = nbits;
	if (nbits2 > diff) {
		do_tosrc(0, 1, diff, fm);
		nbits2 -= diff;
		if (todisplay_fetched) {
			int i;
			for (i = 0; i < toscr_nr_planes_shifter; i++) {
				todisplay2_aga[i] = todisplay_aga[i];
				todisplay2_lastbit[i] = todisplay_aga[i] & 1;
			}
			todisplay_fetched = 0;
			// Chipset bug. if bitplane gets disabled mid-scanline and its last pixel in shifter
			// is one, it gets duplicated.
			if (toscr_nr_changed) {
				while (i < toscr_nr_planes2) {
					if (todisplay2_lastbit[i]) {
						todisplay2_aga[i] = firstpixel[fm];
						todisplay2_lastbit[i] = 0;
					}
					i++;
				}
			}
		}
	}
	if (nbits2) {
		do_tosrc(0, 1, nbits2, fm);
	}
}

static bool todisplay_copy_hr_oddeven(int oddeven, int fm)
{
	if (todisplay_fetched & (oddeven + 1)) {
		int i;
		for (i = oddeven; i < toscr_nr_planes_shifter; i += 2) {
			todisplay2_aga[i] = todisplay_aga[i];
			todisplay2_lastbit[i] = todisplay_aga[i] & 1;
		}
		todisplay_fetched -= oddeven + 1;
		out_subpix[oddeven] = 0;
		if (toscr_nr_changed) {
			while (i < toscr_nr_planes2) {
				if (todisplay2_lastbit[i]) {
					todisplay2_aga[i] = firstpixel[fm];
					todisplay2_lastbit[i] = 0;
				}
				i += 2;
			}
		}
		return true;
	}
	return false;
}

STATIC_INLINE void do_delays_3_aga_hr2(int nbits, int fm)
{
	int delaypos = delay_cycles;
	for (int oddeven = 0; oddeven < 2; oddeven++) {
		int delay = toscr_delay_shifter[oddeven];
		int diff = ((delay - delaypos) & shifter_mask) >> toscr_res_pixels_shift_hr;
		int nbits2 = nbits;
		if (nbits2 > diff) {
			do_tosrc_hr(oddeven, 2, diff, fm);
			nbits2 -= diff;
			todisplay_copy_hr_oddeven(oddeven, fm);
		}
		if (nbits2) {
			do_tosrc_hr(oddeven, 2, nbits2, fm);
		}
	}
}

// This is very very slow. But really rarely needed.
static void pull_toscr_output_bits(int nbits, int planes, uae_u32 *ptrs)
{
	uae_u64 mask = (1 << nbits) - 1;
	if (out_nbits >= nbits) {
		for (int i = 0; i < planes; i++) {
			ptrs[i] = (uae_u32)(outword64[i] & mask);
		}
		return;
	}
	int tbits = nbits - out_nbits;
	uae_u64 nmask = (1 << out_nbits) - 1;
	for (int i = 0; i < planes; i++) {
		ptrs[i] = 0;
		uae_u64 v = outword64_extra[i];
		ptrs[i] = (uae_u32)((v << out_nbits) & mask);
		ptrs[i] |= outword64[i] & nmask;
	}
}

static void push_toscr_output_bits(int nbits, int planes, uae_u32 *ptrs)
{
	uae_u64 mask = (1 << nbits) - 1;
	if (out_nbits >= nbits) {
		for (int i = 0; i < planes; i++) {
			outword64[i] &= ~mask;
			outword64[i] |= ptrs[i];
		}
		return;
	}
	int tbits = nbits - out_nbits;
	uae_u64 tmask = (1 << tbits) - 1;
	uae_u64 nmask = (1 << out_nbits) - 1;
	for (int i = 0; i < planes; i++) {
		uae_u64 v = outword64_extra[i];
		v &= ~tmask;
		v |= (ptrs[i] >> out_nbits) & tmask;
		outword64_extra[i] = v;
		outword64[i] &= ~nmask;
		outword64[i] |= ptrs[i] & nmask;
	}
}

STATIC_INLINE void toscr_1_hr_nbits(void)
{
	if (out_offs < MAX_WORDS_PER_LINE * 2 / 4 - 1) {
		uae_u8 *dataptr = line_data[next_lineno] + out_offs * 4;
		for (int i = 0; i < toscr_nr_planes2; i++) {
			uae_u64 *dataptr64 = (uae_u64 *)dataptr;
			uae_u64 v = outword64[i];
//			uae_u64 v = outword64_extra[i];
//			outword64_extra[i] = outword64[i];
			v = (v >> 32) | (v << 32);
			if (thisline_changed || *dataptr64 != v) {
				thisline_changed = 1;
				*dataptr64 = v;
			}
			dataptr += MAX_WORDS_PER_LINE * 2;
		}
		out_offs += 2;
	}
	out_nbits = 0;
}

#define TOSCR_SPC_LORES 1
#define TOSCR_SPC_HIRES 2
#define TOSCR_SPC_LORES_END 16
#define TOSCR_SPC_HIRES_END 32
#define TOSCR_SPC_LORES_START 64
#define TOSCR_SPC_HIRES_START 128
#define TOSCR_SPC_CLEAR 256
#define TOSCR_SPC_CLEAR2 512
#define TOSCR_SPC_6HIRESTOLORES 1024
#define TOSCR_SPC_SETHIRES 2048
#define TOSCR_SPC_SETLORES 4096
#define TOSCR_SPC_DUAL 8192
#define TOSCR_SPC_MARK 16384
#define TOSCR_SPC_SKIP 32768

static const uae_u16 toscr_spc_aga_lores_to_hires[] =
{
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_HIRES_END,
	0, 0, 0, 0
};
static const uae_u16 toscr_spc_aga_hires_to_lores[] =
{
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	0, 0, 0, 0
};

static const uae_u16 toscr_spc_ecs_lores_to_hires[] =
{
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	0, 0, 0, 0
};
static const uae_u16 toscr_spc_ecs_hires_to_lores[] =
{
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END | TOSCR_SPC_6HIRESTOLORES,
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	0, 0, 0, 0
};

#if 0
static const uae_u16 toscr_spc_ocs_lores_to_hires[] =
{
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR | TOSCR_SPC_DUAL,

	0, 0, 0, 0
};

static const uae_u16 toscr_spc_ocs_hires_to_lores[] =
{
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_LORES_START | TOSCR_SPC_LORES_END,
	0, 0, 0, 0
};
#else

static const uae_u16 toscr_spc_ocs_lores_to_hires[] =
{
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END | TOSCR_SPC_CLEAR,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	TOSCR_SPC_LORES | TOSCR_SPC_HIRES_START | TOSCR_SPC_HIRES_END,
	0, 0, 0, 0
};

static const uae_u16 toscr_spc_ocs_hires_to_lores[] =
{
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	TOSCR_SPC_HIRES | TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES_END,
	0, 0, 0, 0
};
#endif


static const uae_u16 *toscr_special_skip_ptr = NULL;

static int process_special_pixel(int delaypos, int fm, uae_u16 cmd)
{
	int ret = 0;
	int delay1 = (bplcon1 & 0x0f) | ((bplcon1 & 0x0c00) >> 6);
	int delay2 = ((bplcon1 >> 4) & 0x0f) | (((bplcon1 >> 4) & 0x0c00) >> 6);
	int delaymask, shiftmask;
	if (cmd & TOSCR_SPC_LORES) {
		delaymask = fetchmode_mask >> 0;
		shiftmask = 63;
	} else {
		delaymask = fetchmode_mask >> 1;
		shiftmask = 31;
	}
	if (cmd & TOSCR_SPC_HIRES_START) {
		toscr_res_pixels_mask_hr = 1 >> toscr_res_pixels_shift_hr;
	}
	if (cmd & TOSCR_SPC_LORES_START) {
		toscr_res_pixels_mask_hr = 3 >> toscr_res_pixels_shift_hr;
	}
	int shifter[2]{};
	shifter[0] = (delay1 & delaymask) << LORES_TO_SHRES_SHIFT;
	shifter[1] = (delay2 & delaymask) << LORES_TO_SHRES_SHIFT;
	for (int oddeven = 0; oddeven < 2; oddeven++) {
		int delay = shifter[oddeven];
		int diff = ((delay - delaypos) & shiftmask) >> toscr_res_pixels_shift_hr;
		if (diff == 0) {
			if (todisplay_copy_hr_oddeven(oddeven, fm)) {
				ret |= 1 << oddeven;
			}
		}
		do_tosrc_hr(oddeven, 2, 1, fm);
	}
	if (cmd & TOSCR_SPC_LORES_END) {
		toscr_res_pixels_mask_hr = 3 >> toscr_res_pixels_shift_hr;
	}
	if (cmd & TOSCR_SPC_HIRES_END) {
		toscr_res_pixels_mask_hr = 1 >> toscr_res_pixels_shift_hr;
	}
	return ret;
}

static void flush_display(int fm);
STATIC_INLINE void do_delays_3_aga_hr(int nbits, int fm)
{
#if 0
	if (0 && (toscr_special_skip_ptr == toscr_spc_ocs_hires_to_lores || toscr_special_skip_ptr == toscr_spc_ocs_lores_to_hires)) {
		int difsize = 0;
		if (toscr_special_res_change_count > 0) {
			int delaypos = delay_cycles;
			while (nbits > 0) {
				if (toscr_special_res_change_count2 > 0) {
					toscr_special_res_change_count2--;
				}
				uae_u16 cmd = 0;
				if (toscr_special_skip_ptr == toscr_spc_ocs_hires_to_lores) {
					if (toscr_special_res_change_count2 > 0) {
						cmd = TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES | TOSCR_SPC_LORES_END;
					} else {
						cmd = TOSCR_SPC_LORES_START | TOSCR_SPC_LORES | TOSCR_SPC_LORES_END;
					}
				} else if (toscr_special_skip_ptr == toscr_spc_ocs_lores_to_hires) {
					if (toscr_special_res_change_count2 > 0) {
						cmd = TOSCR_SPC_LORES_START | TOSCR_SPC_LORES | TOSCR_SPC_HIRES_END;
					} else {
						cmd = TOSCR_SPC_HIRES_START | TOSCR_SPC_LORES | TOSCR_SPC_HIRES_END;
					}
				}
				if (process_special_pixel(delaypos, fm, cmd)) {
					toscr_special_res_change_count_done = toscr_special_res_change_count;
				}
				delaypos += 1 << toscr_res_pixels_shift_hr;
				nbits--;
				difsize++;
				toscr_special_res_change_count--;
				if (toscr_special_res_change_count == 0) {
					uae_u32 ptrs[MAX_PLANES];
					if (toscr_special_skip_ptr == toscr_spc_ocs_hires_to_lores) {
						int c = 8;
						pull_toscr_output_bits(c, toscr_nr_planes_shifter, ptrs);
						for(int i = 0; i < toscr_nr_planes_shifter; i++) {
							uae_u32 v = ptrs[i];
							for (int j = 0; j < c; j += 2) {
								int m1 = 1 << (j + 1);
								int m2 = 1 << (j + 0);
								if (v & m1) {
									v |= m2;
								} else {
									v &= ~m2;
								}
							}
							ptrs[i] = v;
						}
						push_toscr_output_bits(c, toscr_nr_planes_shifter, ptrs);
					} else if (toscr_special_skip_ptr == toscr_spc_ocs_lores_to_hires) {
						int c = 8;
						pull_toscr_output_bits(c, toscr_nr_planes_shifter, ptrs);
						for (int i = 0; i < toscr_nr_planes_shifter; i++) {
							uae_u32 v = ptrs[i];
							for (int j = 0; j < c; j += 2) {
								int m1 = 1 << (j + 0);
								v &= ~m1;
							}
							ptrs[i] = v;
							//ptrs[i] = 0x5555;
						}
						push_toscr_output_bits(c, toscr_nr_planes_shifter, ptrs);
					}
					toscr_special_skip_ptr = NULL;
					break;
				}
			}
		}
		if (nbits > 0) {
			delay_cycles += difsize * (1 << toscr_res_pixels_shift_hr);
			do_delays_3_aga_hr2(nbits, fm);
			delay_cycles -= difsize * (1 << toscr_res_pixels_shift_hr);
		}
	}
#endif
	if (toscr_special_skip_ptr) {
		int difsize = 0;
		int delaypos = delay_cycles;
		while (nbits > 0) {
			uae_u16 cmd = *toscr_special_skip_ptr;
			if (cmd & TOSCR_SPC_SKIP) {
				delay_cycles += difsize * (1 << toscr_res_pixels_shift_hr);
				do_delays_3_aga_hr2(1, fm);
				delay_cycles -= difsize * (1 << toscr_res_pixels_shift_hr);
				delaypos += 1 << toscr_res_pixels_shift_hr;
				nbits--;
				difsize++;
				toscr_special_skip_ptr++;
			} else {
				uae_u64 toda[MAX_PLANES]{};
				if (cmd & TOSCR_SPC_DUAL) {
					for (int i = 0; i < toscr_nr_planes_shifter; i++) {
						toda[i] = todisplay2_aga[i];
					}
				}
				if (cmd & TOSCR_SPC_CLEAR) {
					for (int i = 0; i < toscr_nr_planes_shifter; i++) {
						todisplay2_aga[i] &= ~0x8000;
					}
				}
				if (cmd & TOSCR_SPC_CLEAR2) {
					for (int i = 0; i < toscr_nr_planes_shifter; i++) {
						todisplay2_aga[i] &= 0xff00;
					}
				}
				if ((cmd & TOSCR_SPC_MARK) && (vpos & 1)) {
					for (int i = 0; i < toscr_nr_planes_shifter; i++) {
						todisplay2_aga[i] ^= 0x8000;
					}
				}
				process_special_pixel(delaypos, fm, cmd);
#if 0
				int delay1 = (bplcon1 & 0x0f) | ((bplcon1 & 0x0c00) >> 6);
				int delay2 = ((bplcon1 >> 4) & 0x0f) | (((bplcon1 >> 4) & 0x0c00) >> 6);
				int delaymask, shiftmask;
				if (cmd & TOSCR_SPC_LORES) {
					delaymask = fetchmode_mask >> 0;
					shiftmask = 63;
				} else {
					delaymask = fetchmode_mask >> 1;
					shiftmask = 31;
				}
				if (cmd & TOSCR_SPC_HIRES_START) {
					toscr_res_pixels_mask_hr = 1 >> toscr_res_pixels_shift_hr;
				}
				if (cmd & TOSCR_SPC_LORES_START) {
					toscr_res_pixels_mask_hr = 3 >> toscr_res_pixels_shift_hr;
				}
				int shifter[2];
				shifter[0] = (delay1 & delaymask) << LORES_TO_SHRES_SHIFT;
				shifter[1] = (delay2 & delaymask) << LORES_TO_SHRES_SHIFT;
				for (int oddeven = 0; oddeven < 2; oddeven++) {
					int delay = shifter[oddeven];
					int diff = ((delay - delaypos) & shiftmask) >> toscr_res_pixels_shift_hr;
					if (diff == 0) {
						todisplay_copy_hr_oddeven(oddeven, fm);
					}
					do_tosrc_hr(oddeven, 2, 1, fm);
				}
				if (cmd & TOSCR_SPC_LORES_END) {
					toscr_res_pixels_mask_hr = 3 >> toscr_res_pixels_shift_hr;
				}
				if (cmd & TOSCR_SPC_HIRES_END) {
					toscr_res_pixels_mask_hr = 1 >> toscr_res_pixels_shift_hr;
				}
#endif
				if (cmd & TOSCR_SPC_DUAL) {
					for (int i = 0; i < toscr_nr_planes_shifter; i++) {
						todisplay2_aga[i] = toda[i];
					}
				}

				delaypos += 1 << toscr_res_pixels_shift_hr;
				nbits--;
				difsize++;

				int sh = 1 << toscr_res_pixels_shift_hr;
				toscr_special_skip_ptr += sh;
				if (*toscr_special_skip_ptr == 0) {
					toscr_special_skip_ptr = NULL;
					break;
				}
			}
		}
		if (nbits > 0) {
			delay_cycles += difsize * (1 << toscr_res_pixels_shift_hr);
			do_delays_3_aga_hr2(nbits, fm);
			delay_cycles -= difsize * (1 << toscr_res_pixels_shift_hr);
		}
	} else {
		do_delays_3_aga_hr2(nbits, fm);
	}
}


STATIC_INLINE void do_delays_fast_3_aga_hr(int nbits, int fm)
{
	int delaypos = delay_cycles;
	int delay = toscr_delay_shifter[0];
	int diff = ((delay - delaypos) & shifter_mask) >> toscr_res_pixels_shift_hr;
	int nbits2 = nbits;
	if (nbits2 > diff) {
		do_tosrc_hr(0, 1, diff, fm);
		nbits2 -= diff;
		if (todisplay_fetched) {
			memcpy(todisplay2_aga, todisplay_aga, toscr_nr_planes_shifter * sizeof(uae_u64));
			todisplay_fetched = 0;
			out_subpix[0] = 0;
			out_subpix[1] = 0;
		}
	}
	if (nbits2) {
		do_tosrc_hr(0, 1, nbits2, fm);
	}
}

static void do_delays_2_0(int nbits) { do_delays_3_ecs(nbits); }
#ifdef AGA
static void do_delays_2_1(int nbits) { do_delays_3_aga(nbits, 1); }
static void do_delays_2_2(int nbits) { do_delays_3_aga(nbits, 2); }

static void do_delays_2_0_hr(int nbits) { do_delays_3_aga_hr(nbits, 0); }
static void do_delays_2_1_hr(int nbits) { do_delays_3_aga_hr(nbits, 1); }
static void do_delays_2_2_hr(int nbits) { do_delays_3_aga_hr(nbits, 2); }
static void do_delays_fast_2_0_hr(int nbits) { do_delays_fast_3_aga_hr(nbits, 0); }
static void do_delays_fast_2_1_hr(int nbits) { do_delays_fast_3_aga_hr(nbits, 1); }
static void do_delays_fast_2_2_hr(int nbits) { do_delays_fast_3_aga_hr(nbits, 2); }
#endif

static void do_delays_fast_2_0(int nbits) { do_delays_fast_3_ecs(nbits); }
#ifdef AGA
static void do_delays_fast_2_1(int nbits) { do_delays_fast_3_aga(nbits, 1); }
static void do_delays_fast_2_2(int nbits) { do_delays_fast_3_aga(nbits, 2); }
#endif


// slower version, odd and even delays are different or crosses maxhpos
STATIC_INLINE void do_delays(int nbits, int fm)
{
	switch (fm) {
	case 0:
		do_delays_2_0(nbits);
		break;
#ifdef AGA
	case 1:
		do_delays_2_1(nbits);
		break;
	case 2:
		do_delays_2_2(nbits);
		break;
#endif
	}
}

// common optimized case: odd delay == even delay
STATIC_INLINE void do_delays_fast(int nbits, int fm)
{
	switch (fm) {
	case 0:
		do_delays_fast_2_0(nbits);
		break;
#ifdef AGA
	case 1:
		do_delays_fast_2_1(nbits);
		break;
	case 2:
		do_delays_fast_2_2(nbits);
		break;
#endif
	}
}

#ifdef AGA
STATIC_INLINE void do_delays_hr(int nbits, int fm)
{
	switch (fm) {
	case 0:
		do_delays_2_0_hr(nbits);
		break;
	case 1:
		do_delays_2_1_hr(nbits);
		break;
	case 2:
		do_delays_2_2_hr(nbits);
		break;
	}
}

STATIC_INLINE void do_delays_fast_hr(int nbits, int fm)
{
	switch (fm) {
	case 0:
		do_delays_fast_2_0_hr(nbits);
		break;
	case 1:
		do_delays_fast_2_1_hr(nbits);
		break;
	case 2:
		do_delays_fast_2_2_hr(nbits);
		break;
	}
}
#endif

STATIC_INLINE void do_delays_null(int nbits)
{
	for (int i = 0; i < MAX_PLANES; i++) {
		outword[i] <<= nbits;
		outword64[i] <<= nbits;
	}
}

static uae_u32 todisplay2_saved[MAX_PLANES], todisplay_saved[MAX_PLANES];
static uae_u64 todisplay2_aga_saved[MAX_PLANES], todisplay_aga_saved[MAX_PLANES];
static uae_u16 todisplay_fetched_saved;

STATIC_INLINE void toscr_special(int nbits, int fm)
{
	int total = nbits << toscr_res_pixels_shift;
	bool slow = false;
	if (delay_cycles2 <= delay_lastcycle[lol] && delay_cycles2 + total > delay_lastcycle[lol]) {
		slow = true;
	}
	if (delay_cycles2 <= delay_hsynccycle && delay_cycles2 + total > delay_hsynccycle) {
		slow = true;
	}
	if (slow) {
		for (int i = 0; i < nbits; i++) {
			if (toscr_hend == 0 && delay_cycles2 == delay_lastcycle[lol]) {
				toscr_hend = 1;
				if (!hstrobe_conflict) {
					delay_cycles = 2 << LORES_TO_SHRES_SHIFT;
				}
			}
			if (delay_cycles2 == delay_hsynccycle) {
				toscr_hend = 2;
				for (int i = 0; i < MAX_PLANES; i++) {
					todisplay_saved[i] = todisplay[i];
					todisplay_aga_saved[i] = todisplay_aga[i];
					todisplay2_saved[i] = todisplay2[i];
					todisplay2_aga_saved[i] = todisplay2_aga[i];
				}
				todisplay_fetched_saved = todisplay_fetched;
			}
			if (toscr_hend <= 1) {
				if (!toscr_scanline_complex_bplcon1) {
					do_delays_fast(1, fm);
				} else {
					do_delays(1, fm);
				}
			} else {
				do_delays_null(1);
			}
			delay_cycles += toscr_res_pixels;
			delay_cycles2 += toscr_res_pixels;
		}
	} else {
		if (toscr_hend <= 1) {
			if (!toscr_scanline_complex_bplcon1) {
				do_delays_fast(nbits, fm);
			} else {
				do_delays(nbits, fm);
			}
		} else {
			do_delays_null(nbits);
		}
		delay_cycles += total;
		delay_cycles2 += total;
	}
}

STATIC_INLINE void toscr_special_hr(int nbits, int fm)
{
	int total = nbits << toscr_res_pixels_shift_hr;
	bool slow = false;
	if (delay_cycles2 <= delay_lastcycle[lol] && delay_cycles2 + total > delay_lastcycle[lol]) {
		slow = true;
	}
	if (delay_cycles2 <= delay_hsynccycle && delay_cycles2 + total > delay_hsynccycle) {
		slow = true;
	}
	if (slow) {
		for (int i = 0; i < nbits; i++) {
			if (toscr_hend == 0 && delay_cycles2 == delay_lastcycle[lol]) {
				toscr_hend = 1;
				if (!hstrobe_conflict) {
					delay_cycles = 2 << LORES_TO_SHRES_SHIFT;
				}
			}
			if (delay_cycles2 == delay_hsynccycle) {
				toscr_hend = 2;
				for (int i = 0; i < MAX_PLANES; i++) {
					todisplay_saved[i] = todisplay[i];
					todisplay_aga_saved[i] = todisplay_aga[i];
					todisplay2_saved[i] = todisplay2[i];
					todisplay2_aga_saved[i] = todisplay2_aga[i];
				}
				todisplay_fetched_saved = todisplay_fetched;
			}
			if (toscr_hend <= 1) {
				if (!toscr_scanline_complex_bplcon1) {
					do_delays_fast_hr(1, fm);
				} else {
					do_delays_hr(1, fm);
				}
			} else {
				do_delays_null(1);
			}
			delay_cycles += toscr_res_pixels_hr;
			delay_cycles2 += toscr_res_pixels_hr;
		}
	} else {
		if (toscr_hend <= 1) {
			if (!toscr_scanline_complex_bplcon1) {
				do_delays_fast_hr(nbits, fm);
			} else {
				do_delays_hr(nbits, fm);
			}
		} else {
			do_delays_null(nbits);
		}
		delay_cycles += total;
		delay_cycles2 += total;
	}
}

STATIC_INLINE void toscr_1(int nbits, int fm)
{
	int total = nbits << toscr_res_pixels_shift;
	if (toscr_hend || delay_cycles2 + total > delay_lastcycle[lol]) {
		toscr_special(nbits, fm);
	} else if (!toscr_scanline_complex_bplcon1) {
		// Most common case.
		do_delays_fast(nbits, fm);
		delay_cycles += total;
		delay_cycles2 += total;
	} else {
		// if scanline has at least one complex case (odd != even)
		// all possible remaining odd == even cases in same scanline
		// must also use complex case routine.
		do_delays(nbits, fm);
		delay_cycles += total;
		delay_cycles2 += total;
	}

	out_nbits += nbits;
	if (out_nbits == 32) {
		if (out_offs < MAX_WORDS_PER_LINE * 2 / 4) {
			uae_u8 *dataptr = line_data[next_lineno] + out_offs * 4;
			for (int i = 0; i < toscr_nr_planes2; i++) {
				uae_u32 *dataptr32 = (uae_u32*)dataptr;
				if (*dataptr32 != outword[i]) {
					thisline_changed = 1;
					*dataptr32 = outword[i];
				}
				dataptr += MAX_WORDS_PER_LINE * 2;
			}
			out_offs++;
		}
		out_nbits = 0;
	}
}

STATIC_INLINE void toscr_1_hr(int nbits, int fm)
{
	toscr_special_hr(nbits, fm);

	out_nbits += nbits;
	if (out_nbits == 64) {
		toscr_1_hr_nbits();
	}
}

static void toscr_fm0(int);
static void toscr_fm1(int);
static void toscr_fm2(int);
static void toscr_hr_fm0(int);
static void toscr_hr_fm1(int);
static void toscr_hr_fm2(int);

STATIC_INLINE void toscr(int nbits, int fm)
{
	switch (fm) {
	case 0: toscr_fm0(nbits); break;
#ifdef AGA
	case 1: toscr_fm1(nbits); break;
	case 2: toscr_fm2(nbits); break;
#endif
	}
}

STATIC_INLINE void toscr_hr(int nbits, int fm)
{
	switch (fm) {
	case 0: toscr_hr_fm0(nbits); break;
#ifdef AGA
	case 1: toscr_hr_fm1(nbits); break;
	case 2: toscr_hr_fm2(nbits); break;
#endif
	}
}

STATIC_INLINE void toscr_0(int nbits, int fm)
{
	int t;

	while(nbits > TOSCR_NBITS) {
		toscr(TOSCR_NBITS, fm);
		nbits -= TOSCR_NBITS;
	}

	t = 2 * TOSCR_NBITS - out_nbits;
	if (t < nbits) {
		toscr_1(t, fm);
		nbits -= t;
	}
	toscr_1(nbits, fm);
}

STATIC_INLINE void toscr_0_hr(int nbits, int fm)
{
	nbits <<= toscr_res_mult;
	while(nbits > 0) {
		int n = 2 * TOSCR_NBITSHR - out_nbits;
		n = std::min(n, nbits);
		toscr_1_hr(n, fm);
		nbits -= n;
	}
}

static void toscr_fm0(int nbits) { toscr_0(nbits, 0); }
static void toscr_fm1(int nbits) { toscr_0(nbits, 1); }
static void toscr_fm2(int nbits) { toscr_0(nbits, 2); }

static void toscr_hr_fm0(int nbits) { toscr_0_hr(nbits, 0); }
static void toscr_hr_fm1(int nbits) { toscr_0_hr(nbits, 1); }
static void toscr_hr_fm2(int nbits) { toscr_0_hr(nbits, 2); }

static void flush_plane_data_n(int fm)
{
	// flush pending data
	if (out_nbits <= TOSCR_NBITS) {
		toscr_1(TOSCR_NBITS, fm);
	}
	if (out_nbits != 0) {
		toscr_1(2 * TOSCR_NBITS - out_nbits, fm);
	}
	// flush until hsync
	int maxcnt = 32;
	while ((toscr_hend == 0 || toscr_hend == 1) && maxcnt-- >= 0) {
		toscr_1(TOSCR_NBITS, fm);
	}
}

static void flush_plane_data_hr(int fm)
{
	// flush pending data
	if (out_nbits <= TOSCR_NBITSHR) {
		toscr_1_hr(TOSCR_NBITSHR, fm);
	}
	if (out_nbits != 0) {
		toscr_1_hr(2 * TOSCR_NBITSHR - out_nbits, fm);
	}
	// flush until hsync
	int maxcnt = 32;
	while ((toscr_hend == 0 || toscr_hend == 1) && maxcnt-- >= 0) {
		toscr_1_hr(TOSCR_NBITSHR, fm);
	}
}

static void quick_add_delay_cycles(int total)
{
	while (total > 0) {
		int total2 = total;
		if (delay_cycles2 <= delay_lastcycle[lol] && delay_cycles2 + total2 > delay_lastcycle[lol]) {
			total2 = delay_lastcycle[lol] - delay_cycles2;
		}
		if (delay_cycles2 <= delay_hsynccycle && delay_cycles2 + total2 > delay_hsynccycle) {
			total2 = delay_hsynccycle - delay_cycles2;
		}
		delay_cycles += total2;
		delay_cycles2 += total2;

#if 0
0		if (toscr_special_res_change_count > 0) {
			int tot = total2;
			while (tot > 0) {
				if (toscr_special_res_change_count2 > 0) {
					toscr_special_res_change_count2--;
				}
				toscr_special_res_change_count--;
				if (toscr_special_res_change_count == 0) {
					break;
				}
				tot--;
			}
		}
#endif

		if (toscr_special_skip_ptr) {
			int tot = total2;
			while (tot > 0) {
				toscr_special_skip_ptr++;
				tot--;
				if (*toscr_special_skip_ptr == 0) {
					toscr_special_skip_ptr = NULL;
					break;
				}
			}
		}			

		total -= total2;
		if (total <= 0) {
			break;
		}
		if (currprefs.chipset_hr) {
			toscr_special_hr(1, fetchmode);
		} else {
			toscr_special(1, fetchmode);
		}
	}
}

// flush remaining data but leave data after hsync
static void flush_plane_data(int fm)
{
	if (currprefs.chipset_hr) {
		flush_plane_data_hr(fm);
	} else {
		flush_plane_data_n(fm);
	}
}

static void flush_display(int fm)
{
	if (toscr_nbits && bpl_shifter) {
		if (currprefs.chipset_hr) {
			toscr_hr(toscr_nbits, fm);
		} else {
			toscr(toscr_nbits, fm);
		}
	} else if (toscr_nbits) {
		int total;
		if (currprefs.chipset_hr) {
			total = toscr_nbits << (toscr_res_mult + toscr_res_pixels_shift_hr);
		} else {
			total = toscr_nbits << toscr_res_pixels_shift;
		}
		quick_add_delay_cycles(total);
	}
	toscr_nbits = 0;
}

static void record_color_change(int hpos, int regno, uae_u32 value);

static void hack_shres_delay(int hpos)
{
#if 0
	if (currprefs.chipset_hr)
		return;
	if (!aga_mode && !toscr_delay_sh[0] && !toscr_delay_sh[1])
		return;
	int o0 = toscr_delay_sh[0];
	int o1 = toscr_delay_sh[1];
	int shdelay1 = (bplcon1 >> 8) & 3;
	int shdelay2 = (bplcon1 >> 12) & 3;
	if (shdelay1 != shdelay2) {
		shdelay_disabled = true;
	}
	if (shdelay_disabled) {
		toscr_delay_sh[0] = 0;
		toscr_delay_sh[1] = 0;
	} else {
		toscr_delay_sh[0] = (shdelay1 & 3) >> toscr_res;
		toscr_delay_sh[1] = (shdelay2 & 3) >> toscr_res;
	}
	if (hpos >= 0 && (toscr_delay_sh[0] != o0 || toscr_delay_sh[1] != o1)) {
		record_color_change(hpos, 0, COLOR_CHANGE_SHRES_DELAY | toscr_delay_sh[0]);
		current_colors.extra &= ~(1 << CE_SHRES_DELAY_SHIFT);
		current_colors.extra &= ~(1 << (CE_SHRES_DELAY_SHIFT + 1));
		current_colors.extra |= toscr_delay_sh[0] << CE_SHRES_DELAY_SHIFT;
		remembered_color_entry = -1;
		thisline_changed = 1;
	}
#endif
}

static void update_denise_vars(void)
{
	int res = GET_RES_DENISE(bplcon0d);
	if (res == toscr_res_old)
		return;
	flush_display(fetchmode);
	toscr_res = res;
	toscr_res_old = res;
	update_toscr_vars();
	compute_toscr_delay(bplcon1);
	compute_shifter_mask();
}

static void update_denise(int hpos)
{
	update_denise_vars();
	if (bplcon0d_old != bplcon0d) {
		bplcon0d_old = bplcon0d;
		record_color_change2(hpos, 0x100 + RECORDED_REGISTER_CHANGE_OFFSET, bplcon0d);
		toscr_nr_planes = GET_PLANES(bplcon0d);
		hack_shres_delay(hpos);
	}
}

STATIC_INLINE void clear_fetchbuffer(uae_u32 *ptr, int nwords)
{
	thisline_changed = 1;
	memset(ptr, 0, nwords * 4);
}

static void update_toscr_planes(int newplanes, int fm)
{
	// This must be called just before new bitplane block starts,
	// not when depth value changes. Depth can change early and can leave
	// 16+ pixel horizontal line of old data visible.
	if (out_offs) {
		for (int j = toscr_nr_planes2; j < newplanes ; j++) {
			clear_fetchbuffer((uae_u32 *)(line_data[next_lineno] + 2 * MAX_WORDS_PER_LINE * j), out_offs);
			if (thisline_decision.plfleft >= 0) {
				todisplay[j] = 0;
#ifdef AGA
				if (fm) {
					todisplay_aga[j] = 0;
				}
#endif
			}
		}
	}
}

static void hbstrt_bordercheck(int hpos, bool early)
{
	if (exthblank) {
		sync_color_changes(hpos);
	}
	if (hb_last_diwlastword < 0) {
		return;
	}
	// if HBSTRT re-enabled border (and HDIW was already open), BPL1DAT access will disable border again.
	uae_u16 pos = hpos_to_diwx(hpos);
	if (early) {
		pos -= 2;
	}
	pos -= 2; // 1 hires pixel early
	pos = adjust_hr(pos);
	hdiw_restart(hb_last_diwlastword, pos, false);
	hb_last_diwlastword = -1;
}

static void beginning_of_plane_block_early(int hpos)
{
	plane0p = false;
	plane0p_enabled = false;
	if (thisline_decision.plfleft >= 0) {
		return;
	}
	bprun_pipeline_flush_delay = maxhpos;
	flush_display(fetchmode);
	reset_bpl_vars();
	bpl_shifter = 1;
	int left = hpos + hpos_hsync_extra;
	thisline_decision.plfleft = left * 2;
	if (hdiwstate == diw_states::DIW_waiting_stop && thisline_decision.diwfirstword < 0) {
		// 1.5 lores pixels
		int v = hpos_to_diwx(hpos);
		v += 6;
		v = adjust_hr(v);
		thisline_decision.diwfirstword = coord_diw_shres_to_window_x(v);

	}
	hbstrt_bordercheck(hpos, false);
}

static void start_noborder(int hpos)
{
	if (bpl_shifter <= 0) {
		bpl_shifter = 1;
		reset_bpl_vars();
	}
	if (thisline_decision.plfleft < 0) {
		thisline_decision.plfleft = hpos * 2;
		if (hdiwstate == diw_states::DIW_waiting_stop && thisline_decision.diwfirstword < 0) {
			thisline_decision.diwfirstword = min_diwlastword;
		}
	}
}

/* Called when all planes have been fetched, i.e. when a new block
of data is available to be displayed.  The data in fetched[] is
moved into todisplay[].  */
static void beginning_of_plane_block(int hpos)
{
	flush_display(fetchmode);

	if (fetchmode == 0 && (!currprefs.chipset_hr || !ALL_SUBPIXEL))
		for (int i = 0; i < MAX_PLANES; i++) {
			todisplay[i] = fetched[i];
		}
#ifdef AGA
	else
		for (int i = 0; i < MAX_PLANES; i++) {
			todisplay_aga[i] = fetched_aga[i];
		}
#endif
	todisplay_fetched = 3;
	plane0_first_done = true;
	last_bpl1dat_hpos = std::max(hpos, last_bpl1dat_hpos);
	// HBLANK start to HSYNC start?
	if (!exthblank && ecs_denise && hpos >= 12 && hpos < hsyncstartpos_start_cycles) {
		last_bpl1dat_hpos_early = true;
	}

	// do not mistake end of bitplane as start of low value hblank programmed mode
	if (hpos > REFRESH_FIRST_HPOS) {
		if (ecs_denise && bpl_shifter <= 0) {
			bpl_shifter = 1;
			reset_bpl_vars();
			thisline_decision.plfleft = hpos * 2;
			if (hdiwstate == diw_states::DIW_waiting_stop && thisline_decision.diwfirstword < 0) {
				thisline_decision.diwfirstword = min_diwlastword;
			}
		} else if (!ecs_denise && hdiwbplstart < 0) {
			// if OCS Denise and first BPL1DAT is earlier than OCS_DENISE_HBLANK_DISABLE_HPOS:
			// -> bitplane shifter works normally but HDIW won't open.
			if (bpl_shifter <= 0) {
				bpl_shifter = 1;
				reset_bpl_vars();
				thisline_decision.plfleft = hpos * 2;
				if (hpos >= OCS_DENISE_HBLANK_DISABLE_HPOS) {
					if (hdiwstate == diw_states::DIW_waiting_stop && thisline_decision.diwfirstword < 0) {
						thisline_decision.diwfirstword = min_diwlastword;
					}
					hdiwbplstart = min_diwlastword;
				} else {
					hdiwbplstart = -1;
				}
			}
			// HDIW only opens when next BPL1DAT position is >= OCS_DENISE_HBLANK_DISABLE_HPOS
			if (hdiwbplstart < 0 && hpos >= OCS_DENISE_HBLANK_DISABLE_HPOS) {
				int v = hpos_to_diwx(hpos);
				v -= 4; // 1 lores pixel
				v = coord_diw_shres_to_window_x(v);
				hdiwbplstart = v;
			}
		}
	}

	hbstrt_bordercheck(hpos, true);

	update_denise(hpos);
	thisline_decision.nr_planes = std::max<int>(toscr_nr_planes_agnus, thisline_decision.nr_planes);
}


#if SPEEDUP

#define MAX_FETCH_TEMP 128
static uae_u32 fetch_tmp[MAX_FETCH_TEMP];

/* The usual inlining tricks - don't touch unless you know what you are doing. */
STATIC_INLINE void long_fetch_16(int plane, int nwords, int weird_number_of_bits)
{
	uaecptr bpladdr = bplpt[plane];
	int bploffset = 0;
	uae_u16 *real_pt = (uae_u16*)pfield_xlateptr(bpladdr, nwords * 2);
	int delay = toscr_delay_adjusted[plane & 1];
	int tmp_nbits = out_nbits;
	uae_u32 shiftbuffer;
	uae_u32 outval = outword[plane];
	uae_u32 fetchval = fetched[plane];
	uae_u32 *dataptr = (uae_u32*)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);

	bplpt[plane] += nwords * 2;
	bplptx[plane] += nwords * 2;

	if (real_pt == NULL) {
		if (nwords > MAX_FETCH_TEMP) {
			return;
		}
		for (int i = 0; i < nwords; i++) {
			fetch_tmp[i] = chipmem_wget_indirect(bpladdr);
			bpladdr += 2;
		}
	}

	shiftbuffer = todisplay2[plane] << delay;

	while (nwords > 0) {
		int bits_left = 32 - tmp_nbits;
		uae_u32 t;

		shiftbuffer |= fetchval;

		t = (shiftbuffer >> delay) & 0xffff;

		if (weird_number_of_bits && bits_left < 16) {
			outval <<= bits_left;
			outval |= t >> (16 - bits_left);
			thisline_changed |= *dataptr ^ outval;
			*dataptr++ = outval;

			outval = t;
			tmp_nbits = 16 - bits_left;
		} else {
			outval = (outval << 16) | t;
			tmp_nbits += 16;
			if (tmp_nbits == 32) {
				thisline_changed |= *dataptr ^ outval;
				*dataptr++ = outval;
				tmp_nbits = 0;
			}
		}
		shiftbuffer <<= 16;
		nwords--;
		if (real_pt) {
			fetchval = do_get_mem_word(real_pt);
			real_pt++;
		} else {
			fetchval = fetch_tmp[bploffset];
			bploffset++;
		}
	}
	fetched[plane] = fetchval;
	todisplay2[plane] = shiftbuffer >> delay;
	outword[plane] = outval;
}

#ifdef AGA
STATIC_INLINE void long_fetch_32 (int plane, int nwords, int weird_number_of_bits)
{
	uaecptr bpladdr = bplpt[plane] & ~3;
	int bploffset = 0;
	uae_u32 *real_pt = (uae_u32*)pfield_xlateptr(bpladdr, nwords * 2);
	int delay = toscr_delay_adjusted[plane & 1];
	int tmp_nbits = out_nbits;
	uae_u64 shiftbuffer;
	uae_u32 outval = outword[plane];
	uae_u32 fetchval = (uae_u32)fetched_aga[plane];
	uae_u32 *dataptr = (uae_u32*)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);
	bool unaligned = (bplpt[plane] & 2) != 0;

	bplpt[plane] += nwords * 2;
	bplptx[plane] += nwords * 2;

	if (real_pt == NULL) {
		if (nwords > MAX_FETCH_TEMP) {
			return;
		}
		for (int i = 0; i < nwords; i++) {
			fetch_tmp[i] = chipmem_lget_indirect(bpladdr);
			bpladdr += 4;
		}
	}

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

				thisline_changed |= *dataptr ^ outval;
				*dataptr++ = outval;

				outval = t;
				tmp_nbits = 16 - bits_left;
			} else {
				outval = (outval << 16) | t;
				tmp_nbits += 16;
				if (tmp_nbits == 32) {
					thisline_changed |= *dataptr ^ outval;
					*dataptr++ = outval;
					tmp_nbits = 0;
				}
			}
			shiftbuffer <<= 16;
		}
		nwords -= 2;
		if (real_pt) {
			fetchval = do_get_mem_long(real_pt);
			real_pt++;
		} else {
			fetchval = fetch_tmp[bploffset];
			bploffset++;
		}
		if (unaligned) {
			fetchval &= 0x0000ffff;
			fetchval |= fetchval << 16;
		} else if (fetchmode_fmode_bpl & 2) {
			fetchval &= 0xffff0000;
			fetchval |= fetchval >> 16;
		}

	}
	fetched_aga[plane] = fetchval;
	todisplay2_aga[plane] = (shiftbuffer >> delay) & 0xffffffff;
	outword[plane] = outval;
}

#ifdef HAVE_UAE_U128
/* uae_u128 is available, custom shift functions not necessary */
#else

STATIC_INLINE void shift32plus(uae_u64 *p, int n)
{
	uae_u64 t = p[1];
	t <<= n;
	t |= p[0] >> (64 - n);
	p[1] = t;
}

STATIC_INLINE void aga_shift(uae_u64 *p, int n)
{
	if (n == 0) return;
	shift32plus(p, n);
	p[0] <<= n;
}

STATIC_INLINE void shift32plusn(uae_u64 *p, int n)
{
	uae_u64 t = p[0];
	t >>= n;
	t |= p[1] << (64 - n);
	p[0] = t;
}

STATIC_INLINE void aga_shift_n(uae_u64 *p, int n)
{
	if (n == 0) return;
	shift32plusn(p, n);
	p[1] >>= n;
}

#endif

STATIC_INLINE void long_fetch_64(int plane, int nwords, int weird_number_of_bits)
{
	uaecptr bpladdr = bplpt[plane] & ~7;
	int bploffset = 0;
	uae_u32 *real_pt = (uae_u32*)pfield_xlateptr(bpladdr, nwords * 2);
	int delay = toscr_delay_adjusted[plane & 1];
	int tmp_nbits = out_nbits;
#ifdef HAVE_UAE_U128
	uae_u128 shiftbuffer;
#else
	uae_u64 shiftbuffer[2]{};
#endif
	uae_u32 outval = outword[plane];
	uae_u64 fetchval = fetched_aga[plane];
	uae_u32 *dataptr = (uae_u32*)(line_data[next_lineno] + 2 * plane * MAX_WORDS_PER_LINE + 4 * out_offs);
	int shift = (64 - 16) + delay;
	bool unaligned2 = (bplpt[plane] & 2) != 0;
	bool unaligned4 = (bplpt[plane] & 4) != 0;

	bplpt[plane] += nwords * 2;
	bplptx[plane] += nwords * 2;

	if (real_pt == NULL) {
		if (nwords * 2 > MAX_FETCH_TEMP) {
			return;
		}
		for (int i = 0; i < nwords * 2; i++) {
			fetch_tmp[i] = chipmem_lget_indirect(bpladdr);
			bpladdr += 4;
		}
	}

#ifdef HAVE_UAE_U128
	shiftbuffer = todisplay2_aga[plane] << delay;
#else
	shiftbuffer[1] = 0;
	shiftbuffer[0] = todisplay2_aga[plane];
	aga_shift(shiftbuffer, delay);
#endif

	while (nwords > 0) {
#ifdef HAVE_UAE_U128
		shiftbuffer |= fetchval;
#else
		shiftbuffer[0] |= fetchval;
#endif

		for (int i = 0; i < 4; i++) {
			uae_u32 t;
			int bits_left = 32 - tmp_nbits;

#ifdef HAVE_UAE_U128
			t = (shiftbuffer >> shift) & 0xffff;
#else
			if (64 - shift > 0) {
				t = (uae_u32)(shiftbuffer[1] << (64 - shift));
				t |= shiftbuffer[0] >> shift;
			} else {
				t = (uae_u32)(shiftbuffer[1] >> (shift - 64));
			}
			t &= 0xffff;
#endif

			if (weird_number_of_bits && bits_left < 16) {
				outval <<= bits_left;
				outval |= t >> (16 - bits_left);

				thisline_changed |= *dataptr ^ outval;
				*dataptr++ = outval;

				outval = t;
				tmp_nbits = 16 - bits_left;
			} else {
				outval = (outval << 16) | t;
				tmp_nbits += 16;
				if (tmp_nbits == 32) {
					thisline_changed |= *dataptr ^ outval;
					*dataptr++ = outval;
					tmp_nbits = 0;
				}
			}
#ifdef HAVE_UAE_U128
			shiftbuffer <<= 16;
#else
			aga_shift(shiftbuffer, 16);
#endif
		}

		nwords -= 4;

		int offset1 = 0;
		int offset2 = 1;
		if (unaligned4) {
			offset1 = 1;
		}
		uae_u32 v1, v2;
		if (real_pt) {
			v1 = do_get_mem_long(real_pt + offset1);
			v2 = do_get_mem_long(real_pt + offset2);
			real_pt += 2;
		} else {
			v1 = fetch_tmp[bploffset + offset1];
			v2 = fetch_tmp[bploffset + offset2];
			bploffset += 2;
		}
		if (unaligned2) {
			v1 &= 0x0000ffff;
			v1 |= v1 << 16;
			v2 &= 0x0000ffff;
			v2 |= v2 << 16;
			fetchval = (((uae_u64)v1) << 32) | v2;
		} else {
			fetchval = ((uae_u64)v1) << 32;
			fetchval |= v2;
		}
	}
	fetched_aga[plane] = fetchval;
#ifdef HAVE_UAE_U128
	todisplay2_aga[plane] = shiftbuffer >> delay;
#else
	aga_shift_n(shiftbuffer, delay);
	todisplay2_aga[plane] = shiftbuffer[0];
#endif
	outword[plane] = outval;
}
#endif

static void long_fetch_16_0 (int hpos, int nwords) { long_fetch_16 (hpos, nwords, 0); }
static void long_fetch_16_1 (int hpos, int nwords) { long_fetch_16 (hpos, nwords, 1); }
#ifdef AGA
static void long_fetch_32_0 (int hpos, int nwords) { long_fetch_32 (hpos, nwords, 0); }
static void long_fetch_32_1 (int hpos, int nwords) { long_fetch_32 (hpos, nwords, 1); }
static void long_fetch_64_0 (int hpos, int nwords) { long_fetch_64 (hpos, nwords, 0); }
static void long_fetch_64_1 (int hpos, int nwords) { long_fetch_64 (hpos, nwords, 1); }
#endif

static void do_long_fetch(int hpos, int nwords, int fm)
{

	beginning_of_plane_block(hpos);

	// adjust to current resolution
	int rnwords = nwords >> (3 - toscr_res);

	switch (fm) {
	case 0:
		if (out_nbits & 15) {
			for (int i = 0; i < toscr_nr_planes; i++) {
				long_fetch_16_1(i, rnwords);
			}
		} else {
			for (int i = 0; i < toscr_nr_planes; i++) {
				long_fetch_16_0(i, rnwords);
			}
		}
		break;
#ifdef AGA
	case 1:
		if (out_nbits & 15) {
			for (int i = 0; i < toscr_nr_planes; i++) {
				long_fetch_32_1(i, rnwords);
			}
		} else {
			for (int i = 0; i < toscr_nr_planes; i++) {
				long_fetch_32_0(i, rnwords);
			}
		}
		break;
	case 2:
		if (out_nbits & 15) {
			for (int i = 0; i < toscr_nr_planes; i++) {
				long_fetch_64_1(i, rnwords);
			}
		} else {
			for (int i = 0; i < toscr_nr_planes; i++) {
				long_fetch_64_0(i, rnwords);
			}
		}
		break;
#endif
	}

	plane0 = toscr_nr_planes > 0;

	int cnt = nwords;
	while (exthblank) {
		cnt -= fetchstart;
		hpos += fetchstart;
		sync_color_changes(hpos);
		if (hb_last_diwlastword >= 0) {
			hbstrt_bordercheck(hpos, false);
		}
		if (cnt <= 0) {
			break;
		}
	}

	out_nbits += rnwords * 16;
	out_offs += out_nbits >> 5;
	out_nbits &= 31;
	quick_add_delay_cycles((rnwords * 16) << toscr_res_pixels_shift);

}

#endif

/* make sure fetch that goes beyond maxhpos is finished */
static void finish_final_fetch(int hpos)
{
	clear_bitplane_pipeline(1);

	if (thisline_decision.plfleft < 0) {
		// registers that affect programmed vblank must be checked even if line is blank
		if (line_decisions[next_lineno].bplcon3 != thisline_decision.bplcon3
			|| (line_decisions[next_lineno].bplcon0 & 1) != (thisline_decision.bplcon0 & 1)
			|| line_decisions[next_lineno].vb != thisline_decision.vb
		)
			thisline_changed = 1;
		return;
	}

	flush_display(fetchmode);

	// This is really the end of scanline, we can finally flush all remaining data.
	//thisline_decision.plfright += flush_plane_data(fetchmode, hpos);
	flush_plane_data(fetchmode);

	// This can overflow if display setup is really bad.
	out_offs = std::min(out_offs, MAX_PIXELS_PER_LINE / 32);
	thisline_decision.plflinelen = out_offs;

	/* The latter condition might be able to happen in interlaced frames. */
	if (vposh >= minfirstline && (thisframe_first_drawn_line < 0 || vposh < thisframe_first_drawn_line)) {
		thisframe_first_drawn_line = vposh;
	}
	thisframe_last_drawn_line = vposh;

#ifdef SMART_UPDATE
	if (line_decisions[next_lineno].plflinelen != thisline_decision.plflinelen
		|| line_decisions[next_lineno].plfleft != thisline_decision.plfleft
		|| line_decisions[next_lineno].bplcon0 != thisline_decision.bplcon0
		|| line_decisions[next_lineno].bplcon2 != thisline_decision.bplcon2
#ifdef ECS_DENISE
		|| line_decisions[next_lineno].bplcon3 != thisline_decision.bplcon3
#endif
#ifdef AGA
		|| line_decisions[next_lineno].bplcon4bm != thisline_decision.bplcon4bm
		|| line_decisions[next_lineno].fmode != thisline_decision.fmode
		|| line_decisions[next_lineno].vb != thisline_decision.vb
#endif
		)
#endif /* SMART_UPDATE */
		thisline_changed = 1;
}

STATIC_INLINE int bpl_select_plane(int hpos, int plane, bool modulo)
{
	if (plane >= 1 && plane <= bplcon0_planes_limit) {
		int offset = get_rga_pipeline(hpos, RGA_PIPELINE_OFFSET_BPL_WRITE);
#if CYCLE_CONFLICT_LOGGING
		if (cycle_line_pipe[offset]) {
			write_log(_T("bitplane slot already allocated by %04x!\n"), cycle_line_pipe[offset]);
		}
#endif
		cycle_line_pipe[offset] = plane | (modulo ? CYCLE_PIPE_MODULO : 0) | CYCLE_PIPE_BITPLANE;
		return true;
	}
	return false;
}
static void do_copper_fetch(int hpos, uae_u16 id);
static void do_sprite_fetch(int hpos, uae_u16 dat);

static void set_maxhpos(int maxhp)
{
	maxhposm0 = maxhp;
	maxhposm1 = maxhp - 1;
	maxhposeven = (maxhp & 1) == 0;
}

static void scandoubler_bpl_dma_start(void)
{
	if (!scandoubled_line && doflickerfix_active()) {
		int lof = 1 - lof_display;
		for (int i = 0; i < MAX_PLANES; i++) {
			prevbpl[lof][vpos][i] = bplptx[i];
			if (!lof && (bplcon0 & 4)) {
				bplpt[i] = prevbpl[1 - lof][vpos][i];
			}
			if (!(bplcon0 & 4) || interlace_seen < 0) {
				prevbpl[1 - lof][vpos][i] = prevbpl[lof][vpos][i] = 0;
			}
		}
	}
}

static void bpl_dma_normal_stop(int hpos)
{
	ddf_stopping = 0;
	bprun = 0;
	bprun_end = hpos;
	plfstrt_sprite = 0x100;
	bprun_pipeline_flush_delay = 8;
	if (!ecs_agnus) {
		ddf_limit = true;
	}
	end_estimate_last_fetch_cycle(hpos);
}

STATIC_INLINE bool islastbplseq(void)
{
	bool last = (bprun_cycle & fetchunit_mask) == fetchunit_mask;
	// AGA stops all fetches after 8 cycles
	if (aga_mode && ddf_stopping == 2) {
		last = (bprun_cycle & 7) == 7;
	}
	return last;
}

STATIC_INLINE void one_fetch_cycle_0(int hpos, int fm)
{
	bool last = islastbplseq();

	if (bprun > 0) {
		int cycle_pos = bprun_cycle & fetchstart_mask;
		// 226 -> 0 (even to even cycle)
		bool skip = hpos == maxhposm1 && !maxhposeven;
		if (dmacon_bpl && !skip) {
			bool domod = false;
			if (ddf_stopping == 2) {
				int cycle = bprun_cycle & 7;
				if (fm_maxplane == 8 || (fm_maxplane == 4 && cycle >= 4) || (fm_maxplane == 2 && cycle >= 6)) {
					domod = true;
				}
			}
			int plane = bpl_sequence[cycle_pos];
			bpl_select_plane(hpos, plane, domod);
		}
		if (skip) {
			last = false;
		} else {
			bprun_cycle++;
		}
	} else {
		bpl_select_plane(hpos, 0, false);
	}
	if (last && bprun > 0) {
		if (ddf_stopping == 2) {
			bpl_dma_normal_stop(hpos);
		}
		if (ddf_stopping == 1) {
			ddf_stopping = 2;
		}
	}
	uae_u16 datreg = cycle_line_pipe[hpos];
	plane0 = false;
	if (datreg & CYCLE_PIPE_BITPLANE) {
		plane0 = fetch((datreg - 1) & 7, fm, hpos, (datreg & CYCLE_PIPE_MODULO) != 0);
	} else if (datreg & CYCLE_PIPE_COPPER) {
		cycle_line_pipe[hpos] = 0;
		do_copper_fetch(hpos, datreg);
	}

	if (plane0p_enabled) {
		int offset = get_rga_pipeline(hpos, 1);
		uae_u16 d = cycle_line_pipe[offset];
		if ((d & CYCLE_PIPE_BITPLANE) && (d & 7) == 1) {
			decide_hdiw(hpos);
			if (hdiwstate == diw_states::DIW_waiting_stop || plane0p_forced) {
				plane0p = true;
			}
		}
	}

	fetch_cycle++;
	toscr_nbits += toscr_res2p;

	if (bplcon1_written) {
		flush_display(fm);
		compute_toscr_delay(bplcon1);
		bplcon1_written = false;
	}

	// BPLCON0 modification immediately after BPL1DAT can affect BPL1DAT finished plane block
	if (bplcon0_planes_changed) {
		if (bprun) {
			if (((hpos - bpl_hstart) & fetchstart_mask) < fetchstart_mask) {
				flush_display(fm);
				toscr_nr_planes_shifter = toscr_nr_planes_shifter_new;
				bplcon0_planes_changed = false;
			}
		} else {
			flush_display(fm);
			toscr_nr_planes_shifter = toscr_nr_planes_shifter_new;
			bplcon0_planes_changed = false;
		}
	}

	if (toscr_nbits == TOSCR_NBITS) {
		flush_display(fm);
	}

	if (bprun_pipeline_flush_delay > 0) {
		bprun_pipeline_flush_delay--;
	}
}

static void one_fetch_cycle_fm0(int pos) { one_fetch_cycle_0(pos, 0); }
static void one_fetch_cycle_fm1(int pos) { one_fetch_cycle_0(pos, 1); }
static void one_fetch_cycle_fm2(int pos) { one_fetch_cycle_0(pos, 2); }

STATIC_INLINE void one_fetch_cycle(int pos, int fm)
{
	switch (fm) {
	case 0:
		one_fetch_cycle_fm0(pos);
		break;
#ifdef AGA
	case 1:
		one_fetch_cycle_fm1(pos);
		break;
	case 2:
		one_fetch_cycle_fm2(pos);
		break;
#endif
	}
}

static void update_fetch(int until, int fm)
{
	int hpos = last_fetch_hpos;

	if (hpos >= until) {
		return;
	}

	/* First, a loop that prepares us for the speedup code.  We want to enter
	the SPEEDUP case with fetch_state == plane0 or it is the very
	first fetch cycle (which equals to same state as fetch_was_plane0)
    and then unroll whole blocks, so that we end on the same fetch_state again.  */
	for(;;) {
		if (hpos == until || hpos >= maxhpos) {
			if (until >= maxhpos) {
				flush_display(fm);
			}
			last_fetch_hpos = until;
			return;
		}
		if (plane0p) {
			beginning_of_plane_block_early(hpos);
		}
		if (plane0) {
			break;
		}
		one_fetch_cycle(hpos, fm);
		hpos++;
	}

#if SPEEDUP
	// Unrolled simple version of the for loop below.
	if (bprun && !line_cyclebased && dmacon_bpl && !ddf_stopping
		&& plane0
		&& (ecs_denise || hpos >= OCS_DENISE_HBLANK_DISABLE_HPOS)
		&& !currprefs.chipset_hr
#ifdef DEBUGGER
		&& !debug_dma
#endif
		&& GET_PLANES_LIMIT(bplcon0) == toscr_nr_planes_agnus)
	{
		int hard_ddf_stop = harddis_h ? 0x100 : HARD_DDF_STOP;
		int adjusted_plfstop = plfstop;
		int ddfstop_to_test_ddf = hard_ddf_stop;
		if (adjusted_plfstop >= last_fetch_hpos && adjusted_plfstop < ddfstop_to_test_ddf) {
			ddfstop_to_test_ddf = adjusted_plfstop;
		}
		int ddfstop_to_test = ddfstop_to_test_ddf;
		int offs = (fetch_cycle - 4) & fetchunit_mask;
		int ddf2 = ((ddfstop_to_test - offs + fetchunit - 1) & ~fetchunit_mask) + offs;
		int ddf3 = ddf2 + fetchunit;
		int stop = until < ddf2 ? until : until < ddf3 ? ddf2 : ddf3;
		int count;

		count = stop - hpos;
		if (count >= fetchstart) {
			count &= ~fetchstart_mask;

			int stoppos = hpos + count;

			if (speedup_first) {
				compute_toscr_delay(bplcon1);
				speedup_first = false;
			}

			do_long_fetch(hpos, count, fm);

			if ((hpos <= adjusted_plfstop && stoppos > adjusted_plfstop) || (hpos <= hard_ddf_stop && stoppos > hard_ddf_stop)) {
				ddf_stopping = 1;
			}
			if (hpos <= ddfstop_to_test && stoppos > ddf2) {
				ddf_stopping = 2;
			}
			if (hpos <= ddf2 && stoppos >= ddf2 + fm_maxplane) {
				add_modulos();
				bpl_dma_normal_stop(stoppos);
				plane0 = false;
			}

			// Copy pipelined data to new hpos. They are guaranteed to be same because
			// above routine always works from BPL1DAT to BPL1DAT.
			if (bprun) {
				for (int i = RGA_PIPELINE_ADJUST - 1; i >= 0; i--) {
					int roffset = get_rga_pipeline(hpos, i);
					int woffset = get_rga_pipeline(hpos, count + i);
					cycle_line_pipe[woffset] = cycle_line_pipe[roffset];
					cycle_line_pipe[roffset] = 0;
				}
			} else {
				for (int i = 0; i < RGA_PIPELINE_ADJUST; i++) {
					int roffset = get_rga_pipeline(hpos, i);
					cycle_line_pipe[roffset] = 0;
				}
			}

			hpos += count;
			fetch_cycle += count;
			bprun_cycle += count;
			if (bprun_pipeline_flush_delay > 0) {
				bprun_pipeline_flush_delay -= count;
				bprun_pipeline_flush_delay = std::max(bprun_pipeline_flush_delay, 0);
			}
			if (plane0) {
				last_bpl1dat_hpos = hpos;
			}
		}
	}
#endif
	while (hpos < until) {
		if (plane0p) {
			beginning_of_plane_block_early(hpos);
		}
		if (plane0) {
			beginning_of_plane_block(hpos);
		}
		one_fetch_cycle(hpos, fm);
		hpos++;
	}
	last_fetch_hpos = hpos;
}

static void update_fetch_0(int hpos) { update_fetch(hpos, 0); }
static void update_fetch_1(int hpos) { update_fetch(hpos, 1); }
static void update_fetch_2(int hpos) { update_fetch(hpos, 2); }

static void decide_bpl_fetch(int endhpos)
{
	int hpos = last_fetch_hpos;
	if (hpos >= endhpos) {
		return;
	}

	if (1 && (nodraw() || !bprun_pipeline_flush_delay)) {
		if (hpos < endhpos) {
			if (thisline_decision.plfleft >= 0) {
				while (hpos < endhpos) {
					if (toscr_nbits == TOSCR_NBITS) {
						flush_display(fetchmode);
					}
					toscr_nbits += toscr_res2p;
					hpos++;
				}
			} else {
				int diff = endhpos - hpos;
				int total = diff << (1 + LORES_TO_SHRES_SHIFT);
				quick_add_delay_cycles(total);
			}
			last_fetch_hpos = endhpos;
		}
		return;
	}

	switch (fetchmode) {
		case 0: update_fetch_0(endhpos); break;
#ifdef AGA
		case 1: update_fetch_1(endhpos); break;
		case 2: update_fetch_2(endhpos); break;
#endif
		default: uae_abort(_T("fetchmode corrupt"));
	}
}

static void vdiw_change(uae_u32 v)
{
	vdiwstate_bpl = v != 0;
}

/* Take care of the vertical DIW.  */
static void decide_vline(int hpos)
{
	bool forceoff = (vb_start_line == 1 && !harddis_v);
	bool start = vpos == plffirstline && !forceoff;
	// VB start line forces vertical display window off (if HARDDIS=0)
	bool end = vpos == plflastline || forceoff;

	if (start && !end) {
		if (vdiwstate != diw_states::DIW_waiting_stop) {
			event2_newevent_xx(-1, CYCLE_UNIT, 1, vdiw_change);
		}
		vdiwstate = diw_states::DIW_waiting_stop;
		SET_LINE_CYCLEBASED(hpos);
	} else if (end) {
		if (vdiwstate != diw_states::DIW_waiting_start) {
			event2_newevent_xx(-1, CYCLE_UNIT, 0, vdiw_change);
		}
		vdiwstate = diw_states::DIW_waiting_start;
		SET_LINE_CYCLEBASED(hpos);
	}
}

static void decide_line_decision_fetches(int endhpos)
{
	decide_bpl_fetch(endhpos);
	decide_sprites_fetch(endhpos);
}

static void decide_fetch_safe(int endhpos)
{
	int hpos = last_fetch_hpos;
	if (!blt_info.blitter_dangerous_bpl && !copper_enabled_thisline) {
		decide_line_decision_fetches(endhpos);
		decide_blitter(endhpos);
	} else if (copper_enabled_thisline && blt_info.blitter_dangerous_bpl) {
		while (hpos <= endhpos) {
			decide_line_decision_fetches(hpos);
			update_copper(hpos);
			decide_blitter(hpos);
			hpos++;
		}
	} else {
		while (hpos <= endhpos) {
			decide_line_decision_fetches(hpos);
			decide_blitter(hpos);
			hpos++;
		}
	}
}

/* Called when a color is about to be changed (write to a color register),
* but the new color has not been entered into the table yet. */
static void record_color_change(int hpos, int regno, uae_u32 value)
{
	if (regno < RECORDED_REGISTER_CHANGE_OFFSET && nodraw())
		return;

	decide_hdiw(hpos);

	/* vsync period don't appear on-screen. */
	if (line_hidden())
		return;

	decide_line(hpos);

	if (thisline_decision.ctable < 0) {
		remember_ctable();
	}

	hpos += vhposr_delay_offset;

	record_color_change2(hpos, regno, value);
}

static void setextblank(void)
{
	bool extblank = (bplcon0 & 1) && (bplcon3 & 0x01);
	current_colors.extra &= ~(1 << CE_EXTBLANKSET);
	if (extblank) {
		current_colors.extra |= 1 << CE_EXTBLANKSET;
	}
}

static bool isbrdblank(int hpos, uae_u16 bplcon0, uae_u16 bplcon3)
{
	bool brdblank, brdntrans, extblank;
#ifdef ECS_DENISE
	brdblank = (ecs_denise && (bplcon0 & 1) && (bplcon3 & 0x20)) || (currprefs.genlock_effects & (1 << 5));
	brdntrans = (ecs_denise && (bplcon0 & 1) && (bplcon3 & 0x10)) || (currprefs.genlock_effects & (1 << 4));
	// ECSENA=0: hardwired horizontal, strobe vertical
	// ECSENA=1: EXTBLKEN=0: hardwired blanking, strobe vertical
	// ECSENA=1: EXTBLKEN=1: blanking equals HSYNC if VARCSYEN=1, blanking equals HBSTRT-HBSTOP if VARCSYEN=0, no vertical, AGA: programmed horizontal, strobe vertical
	extblank = (bplcon0 & 1) && (bplcon3 & 1);
#else
	brdblank = false;
	brdntrans = false;
	extblank = false;
#endif
	if (hpos >= 0 && (ce_is_borderblank(current_colors.extra) != brdblank || ce_is_borderntrans(current_colors.extra) != brdntrans || ce_is_extblankset(current_colors.extra) != extblank)) {
		record_color_change(hpos, 0, COLOR_CHANGE_BRDBLANK | (brdblank ? 1 : 0) | (ce_is_bordersprite(current_colors.extra) ? 2 : 0) | (brdntrans ? 4 : 0) | (extblank ? 8 : 0));
		current_colors.extra &= ~(1 << CE_BORDERBLANK);
		current_colors.extra &= ~(1 << CE_BORDERNTRANS);
		current_colors.extra &= ~(1 << CE_EXTBLANKSET);
		current_colors.extra |= brdblank ? (1 << CE_BORDERBLANK) : 0;
		current_colors.extra |= brdntrans ? (1 << CE_BORDERNTRANS) : 0;
		current_colors.extra |= extblank ? (1 << CE_EXTBLANKSET) : 0;
		remembered_color_entry = -1;
	}
	return brdblank;
}

static bool brdblankactive(void)
{
	return (bplcon0 & 1) && (bplcon3 & 0x20);
}

static bool brdspractive(void)
{
	return (bplcon3 & 2) && (bplcon0 & 1);
}

static bool issprbrd(int hpos, uae_u16 bplcon0, uae_u16 bplcon3)
{
	bool brdsprt;
#ifdef AGA
	brdsprt = aga_mode && brdspractive();
#else
	brdsprt = false;
#endif
	if (hpos >= 0 && ce_is_bordersprite(current_colors.extra) != brdsprt) {
		record_color_change(hpos, 0, COLOR_CHANGE_BRDBLANK | (ce_is_borderblank(current_colors.extra) ? 1 : 0) | (brdsprt ? 2 : 0) |
			(ce_is_borderntrans(current_colors.extra) ? 4 : 0) | (ce_is_extblankset(current_colors.extra) ? 8 : 0));
		current_colors.extra &= ~(1 << CE_BORDERSPRITE);
		current_colors.extra |= brdsprt ? (1 << CE_BORDERSPRITE) : 0;
		remembered_color_entry = -1;
		if (brdsprt && !ce_is_borderblank(current_colors.extra)) {
			thisline_decision.bordersprite_seen = true;
		}
	}
	if (!plane0_first_done) {
		plane0p_enabled = ecs_denise && (bplcon0 & 1) && (bplcon3 & 0x20);
	}
	return brdsprt && !ce_is_borderblank(current_colors.extra);
}

static bool isham(uae_u16 bplcon0)
{
	int p = GET_PLANES(bplcon0);
	if (!(bplcon0 & 0x800))
		return 0;
	if (aga_mode) {
		// AGA only has 6 or 8 plane HAM
		if (p == 6 || p == 8)
			return 1;
	} else {
		// OCS/ECS also supports 5 plane HAM
		if (GET_RES_DENISE(bplcon0) > 0)
			return 0;
		if (p >= 5)
			return 1;
	}
	return 0;
}

static void record_register_change(int hpos, int regno, uae_u16 value)
{
	if (regno == 0x0100 || regno == 0x0101) { // BPLCON0
		if (value & 0x800) {
			thisline_decision.ham_seen |= isham(value);
		}
		thisline_decision.ehb_seen |= isehb(value, bplcon2);
		isbrdblank(hpos, value, bplcon3);
		issprbrd(hpos, value, bplcon3);
	} else if (regno == 0x104) { // BPLCON2
		thisline_decision.ehb_seen |= isehb(bplcon0, value);
	} else if (regno == 0x106) { // BPLCON3
		isbrdblank(hpos, bplcon0, value);
		issprbrd(hpos, bplcon0, value);
	}
	record_color_change(hpos, regno + RECORDED_REGISTER_CHANGE_OFFSET, value);
}

typedef int sprbuf_res_t, cclockres_t, hwres_t,	bplres_t;

/* handle very rarely needed playfield collision (CLXDAT bit 0) */
/* only known game needing this is Rotor */
static void do_playfield_collisions(int startpos, int endpos)
{
	int bplres = output_res(bplcon0_res);
	hwres_t ddf_left = (thisline_decision.plfleft - DDF_OFFSET) << bplres;
	hwres_t hw_diwlast = coord_window_to_diw_x(endpos);
	hwres_t hw_diwfirst = coord_window_to_diw_x(startpos);
	int i, collided, minpos, maxpos;
#ifdef AGA
	int planes = aga_mode ? 8 : 6;
#else
	int planes = 6;
#endif

	if (clxcon_bpl_enable == 0) {
		clxdat |= 1;
		return;
	}
	// collision bit already set?
	if (clxdat & 1) {
		return;
	}

	collided = 0;
	minpos = thisline_decision.plfleft - DDF_OFFSET;
	minpos = std::max(minpos, hw_diwfirst);
	maxpos = thisline_decision.plfright - DDF_OFFSET;
	maxpos = std::min(maxpos, hw_diwlast);

	for (i = minpos; i < maxpos && !collided; i += 32) {
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
#if 0
			{
				int k;
				for (k = 0; k < 1; k++) {
					uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + offs + 2 * k * MAX_WORDS_PER_LINE);
					*ldata ^= 0x5555555555;
				}
			}
#endif

		}
	}
	if (collided) {
		clxdat |= 1;
	}
}

/* Sprite-to-sprite collisions are taken care of in record_sprite.  This one does
playfield/sprite collisions. */
static void do_sprite_collisions(int startpos, int endpos)
{
	int nr_sprites = curr_drawinfo[next_lineno].nr_sprites;
	int first = curr_drawinfo[next_lineno].first_sprite_entry;
	uae_u32 collision_mask = clxmask[clxcon >> 12];
	int bplres = output_res(bplcon0_res);
	int ddf_left = (thisline_decision.plfleft - DDF_OFFSET) << bplres;
	int hw_diwlast = coord_window_to_diw_x(endpos);
	int hw_diwfirst = coord_window_to_diw_x(startpos);

	if (clxcon_bpl_enable == 0 && !nr_sprites) {
		return;
	}

	// all sprite to bitplane collision bits already set?
	if ((clxdat & 0x1fe) == 0x1fe) {
		return;
	}

	for (int i = 0; i < nr_sprites; i++) {
		struct sprite_entry *e = curr_sprite_entries + first + i;
		sprbuf_res_t minpos = e->pos;
		sprbuf_res_t maxpos = e->max;
		int minp1 = minpos >> sprite_buffer_res;
		int maxp1 = maxpos >> sprite_buffer_res;

		if (maxp1 > hw_diwlast) {
			maxp1 = hw_diwlast;
			maxpos = hw_diwlast << sprite_buffer_res;
		}
		if (maxp1 > thisline_decision.plfright - DDF_OFFSET) {
			maxpos = (thisline_decision.plfright - DDF_OFFSET) << sprite_buffer_res;
		}
		if (minp1 < hw_diwfirst) {
			minp1 = hw_diwfirst;
			minpos = hw_diwfirst << sprite_buffer_res;
		}
		if (minp1 < thisline_decision.plfleft - DDF_OFFSET) {
			minpos = (thisline_decision.plfleft - DDF_OFFSET) << sprite_buffer_res;
		}

		for (sprbuf_res_t j = minpos; j < maxpos; j++) {
			int sprpix = spixels[e->first_pixel + j - e->pos] & collision_mask;
			int offs, match = 1;

			if (sprpix == 0) {
				continue;
			}

			offs = ((j << bplres) >> sprite_buffer_res) - ddf_left;
			sprpix = sprite_ab_merge[sprpix & 255] | (sprite_ab_merge[sprpix >> 8] << 2);
			sprpix <<= 1;

			// both odd and even collision bits already set?
			if (((clxdat & (sprpix << 0)) == (sprpix << 0)) && ((clxdat & (sprpix << 4)) == (sprpix << 4))) {
				continue;
			}

			/* Loop over number of playfields.  */
			for (int k = 1; k >= 0; k--) {
#ifdef AGA
				int planes = aga_mode ? 8 : 6;
#else
				int planes = 6;
#endif
				if (bplcon0 & 0x400) {
					match = 1;
				}
				for (int l = k; match && l < planes; l += 2) {
					int t = 0;
					if (l < thisline_decision.nr_planes) {
						uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + 2 * l * MAX_WORDS_PER_LINE);
						uae_u32 word = ldata[offs >> 5];
						t = (word >> (31 - (offs & 31))) & 1;
#if 0 /* debug: draw collision mask */
						if (1) {
							for (int m = 0; m < 5; m++) {
								ldata = (uae_u32 *)(line_data[next_lineno] + 2 * m * MAX_WORDS_PER_LINE);
								ldata[(offs >> 5) + 1] |= 15 << (31 - (offs & 31));
							}
						}
#endif
					}
					if (clxcon_bpl_enable & (1 << l)) {
						if (t != ((clxcon_bpl_match >> l) & 1)) {
							match = 0;
						}
					}
				}
				if (match) {
#if 0 /* debug: mark lines where collisions are detected */
					if (0) {
						int l;
						for (l = 0; l < 5; l++) {
							uae_u32 *ldata = (uae_u32 *)(line_data[next_lineno] + 2 * l * MAX_WORDS_PER_LINE);
							ldata[(offs >> 5) + 1] |= 15 << (31 - (offs & 31));
						}
					}
#endif
					clxdat |= sprpix << (k * 4);
				}
			}
		}
	}
#if 0
	{
		static int olx;
		if (clxdat != olx)
			write_log(_T("%d: %04X\n"), vpos, clxdat);
		olx = clxdat;
	}
#endif
}

static void check_collisions(int hpos)
{
	hpos += hpos_hsync_extra;
	if (hpos < collision_hpos) {
		return;
	}
	int startpos = thisline_decision.diwfirstword;
	int endpos = thisline_decision.diwlastword;
	if (startpos < 0 || endpos < 0) {
		return;
	}
	int start = coord_diw_shres_to_window_x(collision_hpos << CCK_SHRES_SHIFT);
	startpos = std::max(startpos, start);
	int end = coord_diw_shres_to_window_x(hpos << CCK_SHRES_SHIFT);
	endpos = std::min(endpos, end);
	if (sprites_enabled_this_line || brdspractive()) {
		if (currprefs.collision_level > 1) {
			do_sprite_collisions(startpos, endpos);
		}
	}
	if (currprefs.collision_level > 2) {
		do_playfield_collisions(startpos, endpos);
	}
	collision_hpos = hpos;
}

static int tospritexdiw(int diw)
{
	int v = (coord_window_to_hw_x(diw) - DIW_DDF_OFFSET) << sprite_buffer_res;
	v -= (1 << sprite_buffer_res) - 1;
	return v;
}
static int tospritexddf(int ddf)
{
	return (ddf - DIW_DDF_OFFSET - DDF_OFFSET) << sprite_buffer_res;
}
static int fromspritexdiw(int ddf)
{
	return coord_hw_to_window_x_lores(ddf >> sprite_buffer_res) + (DIW_DDF_OFFSET << lores_shift);
}

static void record_sprite_1(int sprxp, uae_u16 *buf, uae_u32 datab, int num, int dbl,
	unsigned int mask, int do_collisions, uae_u32 collision_mask)
{
	uae_u16 erasemask = ~(3 << (2 * num));
	int j = 0;
	while (datab) {
		unsigned int col = 0;
		unsigned coltmp = 0;

		if ((sprxp >= sprite_minx) || brdspractive() || hstrobe_conflict)
			col = (datab & 3) << (2 * num);
#if 0
		if (sprxp == sprite_minx)
			col ^= (uaerand () << 16) | uaerand ();
#endif
		if ((j & mask) == 0) {
			unsigned int tmp = ((*buf) & erasemask) | col;
			*buf++ = tmp;
			if (do_collisions)
				coltmp |= tmp;
			sprxp++;
		}
		if (dbl > 0) {
			unsigned int tmp = ((*buf) & erasemask) | col;
			*buf++ = tmp;
			if (do_collisions)
				coltmp |= tmp;
			sprxp++;
		}
		if (dbl > 1) {
			unsigned int tmp;
			tmp = ((*buf) & erasemask) | col;
			*buf++ = tmp;
			if (do_collisions)
				coltmp |= tmp;
			tmp = ((*buf) & erasemask) | col;
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

static void record_sprite(int num, int sprxp, uae_u16 *data, uae_u16 *datb, unsigned int ctl)
{
	struct sprite_entry *e = curr_sprite_entries + next_sprite_entry;
	int word_offs;
	uae_u32 collision_mask;
	int width, dbl, half;
	unsigned int mask = 0;
	int attachment;
	int spr_width;

	// do nothing if buffer is full (shouldn't happen normally)
	if (next_sprite_entry >= end_sprite_entry) {
		return;
	}
	if (e->first_pixel >= spixels_max) {
		return;
	}

	half = 0;
	dbl = sprite_buffer_res - sprres;
	if (dbl < 0) {
		half = -dbl;
		dbl = 0;
		mask = 1 << half;
	}
	spr_width = spr[num].width;
	width = (spr_width << sprite_buffer_res) >> sprres;
	attachment = spr[num | 1].ctl & 0x80;

	/* Try to coalesce entries if they aren't too far apart  */
	/* Don't coelesce 64-bit wide sprites, needed to support FMODE change tricks */
	if (next_sprite_entry > 0 && !next_sprite_forced && e[-1].max + spr_width >= sprxp) {
		e--;
	} else {
		next_sprite_entry++;
		e->pos = sprxp;
		e->has_attached = 0;
	}

	if (sprxp < e->pos) {
		write_log(_T("sprxp (%d) < e->pos (%d)\n"), sprxp, e->pos);
		return;
	}

	e->max = sprxp + width;
	e[1].first_pixel = e->first_pixel + ((e->max - e->pos + 3) & ~3);
	next_sprite_forced = 0;

	collision_mask = clxmask[clxcon >> 12];
	word_offs = e->first_pixel + sprxp - e->pos;

	for (int i = 0; i < spr_width; i += 16) {
		unsigned int da = *data;
		unsigned int db = *datb;
		uae_u32 datab = ((sprtaba[da & 0xFF] << 16) | sprtaba[da >> 8]
			| (sprtabb[db & 0xFF] << 16) | sprtabb[db >> 8]);
		int off = (i << dbl) >> half;
		uae_u16 *buf = spixels + word_offs + off;
		if (currprefs.collision_level > 0 && collision_mask) {
			record_sprite_1(sprxp + off, buf, datab, num, dbl, mask, 1, collision_mask);
		} else {
			record_sprite_1(sprxp + off, buf, datab, num, dbl, mask, 0, collision_mask);
		}
		data++;
		datb++;
	}

	/* We have 8 bits per pixel in spixstate, two for every sprite pair.  The
	low order bit records whether the attach bit was set for this pair.  */
	if (attachment) {
		uae_u32 state = 0x01010101 << (num & ~1);
		uae_u32 *stb1 = (uae_u32*)(spixstate.stb + word_offs);
		for (int i = 0; i < width; i += 8) {
			stb1[0] |= state;
			stb1[1] |= state;
			stb1 += 2;
		}
		e->has_attached = 1;
	}
	/* 64 pixel wide sprites' first 32 pixels work differently than
	 * last 32 pixels if FMODE is changed when sprite is being drawn
	 */
	if (spr_width == 64) {
		uae_u16 *stbfm = spixstate.stbfm + word_offs;
		uae_u16 state = (3 << (2 * num));
		for (int i = 0; i < width / 2; i += 8) {
			stbfm[0] |= state;
			stbfm[1] |= state;
			stbfm[2] |= state;
			stbfm[3] |= state;
			stbfm[4] |= state;
			stbfm[5] |= state;
			stbfm[6] |= state;
			stbfm[7] |= state;
			stbfm += 8;
		}
	}
}

static void add_sprite(int *countp, int num, int sprxp, int posns[], int nrs[])
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
		posns[j] = posns[j - 1];
		nrs[j] = nrs[j - 1];
	}
	posns[j] = sprxp;
	nrs[j] = num;
	count++;
	*countp = count;

	sprites_enabled_this_line = true;
}

static void calcsprite(void)
{
	sprite_minx = 0;
	if (thisline_decision.diwfirstword >= 0) {
		sprite_minx = tospritexdiw(thisline_decision.diwfirstword);
	}
	if (thisline_decision.plfleft >= 0) {
		int min = tospritexddf(thisline_decision.plfleft);
		int max = tospritexddf(thisline_decision.plfright);
		if (min > sprite_minx && min < max) { /* min < max = full line ddf */
			sprite_minx = min;
		}
		/* sprites are visible from first BPL1DAT write to end of line
		 * ECS Denise/AGA: no limits
		 * OCS Denise: BPL1DAT write only enables sprite if hpos >= 0x2e (OCS_DENISE_HBLANK_DISABLE_HPOS).
		 * (undocumented feature)
		 */
	}
}

static int last_sprite_hpos, last_sprite_hpos_reset;

static void decide_sprites2(int start, int end, int *countp, int *nrs, int *posns)
{
	int count = *countp;
	int sscanmask = 0x100 << sprite_buffer_res;

	if (nodraw()) {
		return;
	}

	for (int i = 0; i < MAX_SPRITES; i++) {
		struct sprite *s = &spr[i];
		int xpos = spr[i].xpos;
		int sprxp = (fmode & 0x8000) ? (xpos & ~sscanmask) : xpos;
		int hw_xp = sprxp >> sprite_buffer_res;

		// Sprite does not start if X=0 but SSCAN2 sprite does. Don't do X == 0 check here.
		if (sprxp < 0) {
			continue;
		}
#ifdef DEBUGGER
		if (!((debug_sprite_mask & magic_sprite_mask) & (1 << i))) {
			continue;
		}
#endif

		if (!spr[i].armed) {
			continue;
		}

		// ECS Denise and superhires resolution: sprites 4 to 7 are disabled.
		if (!aga_mode && ecs_denise && GET_RES_AGNUS(bplcon0) == RES_SUPERHIRES && i >= 4) {
			continue;
		}

		if (s->ecs_denise_hires && (bplcon0d & 0x0040)) {
			xpos |= 2 >> (RES_MAX - sprite_buffer_res);
			sprxp = xpos;
			hw_xp = sprxp >> sprite_buffer_res;
		}

		if (((hw_xp >= start) || ((spr[i].pos & 1) && hw_xp == start - 1)) && hw_xp < end) {
			int xdiff = hw_xp - start;
			int sprxp_abs = (last_sprite_point_abs + xdiff) << sprite_buffer_res;
			// add hires/shres back
			sprxp_abs |= xpos & (3 >> (RES_MAX - sprite_buffer_res));
			add_sprite(&count, i, sprxp_abs, posns, nrs);
		}

		/* SSCAN2-bit is fun.. */
		if (1 && (fmode & 0x8000) && !(sprxp & sscanmask)) {
			sprxp |= sscanmask;
			hw_xp = sprxp >> sprite_buffer_res;
			if (hw_xp >= start && hw_xp < end) {
				int xdiff = hw_xp - start;
				int sprxp_abs = (last_sprite_point_abs + xdiff) << sprite_buffer_res;
				sprxp_abs |= xpos & (3 >> (RES_MAX - sprite_buffer_res));
				add_sprite(&count, MAX_SPRITES + i, sprxp_abs, posns, nrs);
			}
		}
	}
	*countp = count;


}

static void decide_sprites(int hpos, bool quick, bool halfcycle = false)
{
	int count = 0;
	int gotdata = 0;
	int nrs[MAX_SPRITES * 2], posns[MAX_SPRITES * 2];
	int hp = (hpos << 1) + halfcycle;

	// don't bother with sprites during vblank lines
	if (vb_start_line > 1) {
		last_sprite_hpos = hp;
		return;
	}

	int c = hp - last_sprite_hpos;
	if (c <= 0) {
		return;
	}

	if (!quick) {
		decide_hdiw(hpos);
		decide_line(hpos);
	}

	if (last_sprite_hpos < (HARDWIRED_DMA_TRIGGER_HPOS << 1) && hp >= (HARDWIRED_DMA_TRIGGER_HPOS << 1)) {
		last_sprite_hpos_reset = (REFRESH_FIRST_HPOS - HARDWIRED_DMA_TRIGGER_HPOS + 2) << 1;
		last_sprite_hpos_reset += 1;
	}

	int start = last_sprite_point;
	int max = 512;

	if (last_sprite_hpos_reset > 0 && c >= last_sprite_hpos_reset && !line_equ_freerun && !hstrobe_conflict) {
		// strobe crossed?
		last_sprite_point += last_sprite_hpos_reset + 0;
		if (nr_armed) {
			decide_sprites2(start, last_sprite_point, &count, nrs, posns);
		}
		last_sprite_point_abs += last_sprite_point - start;
		last_sprite_point = 2;
		start = last_sprite_point;
		last_sprite_point += c - last_sprite_hpos_reset;
		if (nr_armed) {
			decide_sprites2(start, last_sprite_point, &count, nrs, posns);
		}
		last_sprite_point_abs += last_sprite_point - start;
		last_sprite_hpos_reset = -1;
	} else {
		last_sprite_point += c;
		// wrap around?
		if (last_sprite_point >= max) {
			if (nr_armed) {
				decide_sprites2(start, max, &count, nrs, posns);
			}
			last_sprite_point_abs += max - start;
			last_sprite_point &= max - 1;
			if (last_sprite_point > 0) {
				if (nr_armed) {
					decide_sprites2(0, last_sprite_point, &count, nrs, posns);
				}
				last_sprite_point_abs += last_sprite_point - 0;
			}
		} else {
			if (nr_armed) {
				decide_sprites2(start, last_sprite_point, &count, nrs, posns);
			}
			last_sprite_point_abs += last_sprite_point - start;
		}
		if (last_sprite_hpos_reset >= 0) {
			last_sprite_hpos_reset -= c;
		}
	}
	last_sprite_hpos = hp;

	if (count > 0) {
		calcsprite();

		for (int i = 0; i < count; i++) {
			int nr = nrs[i] & (MAX_SPRITES - 1);
			struct sprite *s = &spr[nr];

			// ECS Denise weird behavior in shres
			if (s->ecs_denise_hires && !(bplcon0d & 0x40)) {
				s->data[0] &= 0x7fff;
				s->datb[0] &= 0x7fff;
			}

			record_sprite(nr, posns[i], s->data, s->datb, s->ctl);

			/* get left and right sprite edge if brdsprt enabled */
#if AUTOSCALE_SPRITES
			if (dmaen(DMA_SPRITE) && brdspractive() && !(bplcon3 & 0x20) && nr > 0) {
				int j, jj;
				for (j = 0, jj = 0; j < sprite_width; j += 16, jj++) {
					int nx = fromspritexdiw(posns[i] + j);
					if (s->data[jj] || s->datb[jj]) {
						if (diwfirstword_total > nx && nx >= (48 << currprefs.gfx_resolution)) {
							diwfirstword_total = nx;
						}
						if (diwlastword_total < nx + 16 && nx <= (448 << currprefs.gfx_resolution)) {
							diwlastword_total = nx + 16;
						}
					}
				}
				gotdata = 1;
			}
#endif
		}

#if AUTOSCALE_SPRITES
		/* get upper and lower sprite position if brdsprt enabled */
		if (gotdata) {
			first_planes_vpos = std::min(vpos, first_planes_vpos);
			plffirstline_total = std::min(vpos, plffirstline_total);
			last_planes_vpos = std::max(vpos, last_planes_vpos);
			plflastline_total = std::max(vpos, plflastline_total);
		}
#endif
	}
}

static void decide_sprites(int hpos)
{
	decide_sprites(hpos, false, false);
}

static int sprites_differ(struct draw_info *dip, struct draw_info *dip_old)
{
	struct sprite_entry *this_first = curr_sprite_entries + dip->first_sprite_entry;
	struct sprite_entry *this_last = curr_sprite_entries + dip->last_sprite_entry;
	struct sprite_entry *prev_first = prev_sprite_entries + dip_old->first_sprite_entry;

	if (dip->nr_sprites != dip_old->nr_sprites) {
		return 1;
	}

	if (dip->nr_sprites == 0) {
		return 0;
	}

	return 1;
#if 0
	int npixels;
	for (int i = 0; i < dip->nr_sprites; i++) {
		if (this_first[i].pos != prev_first[i].pos
			|| this_first[i].max != prev_first[i].max
			|| this_first[i].has_attached != prev_first[i].has_attached) {
			return 1;
		}
	}

    npixels = this_last->first_pixel + (this_last->max - this_last->pos) - this_first->first_pixel;
	if (memcmp(spixels + this_first->first_pixel, spixels + prev_first->first_pixel, npixels * sizeof(uae_u16)) != 0) {
		return 1;
	}
	if (memcmp(spixstate.stb + this_first->first_pixel, spixstate.stb + prev_first->first_pixel, npixels) != 0) {
		return 1;
	}
	return 0;
#endif
}

static bool color_changes_differ(struct draw_info *dip, struct draw_info *dip_old)
{
	if (dip->nr_color_changes != dip_old->nr_color_changes) {
		return true;
	}

	if (dip->nr_color_changes == 0) {
		return false;
	}
	if (memcmp(curr_color_changes + dip->first_color_change,
		prev_color_changes + dip_old->first_color_change,
		dip->nr_color_changes * sizeof * curr_color_changes) != 0) {
		return true;
	}
	return false;
}

/* End of a horizontal scan line. Finish off all decisions that were not
* made yet. */

static void sync_copper(int hpos);

static void finish_partial_decision(int hpos)
{
	sync_copper(hpos);
	decide_hdiw(hpos);
	decide_line(hpos);
	decide_fetch_safe(hpos);
	decide_sprites(hpos);
	if (thisline_decision.plfleft >= 0 && thisline_decision.plfright < (hpos + hpos_hsync_extra) * 2) {
		thisline_decision.plfright = (hpos + hpos_hsync_extra) * 2;
	}
}

static void hstrobe_conflict_check(uae_u32 v)
{
	int hpos = current_hpos();
	finish_partial_decision(hpos);

	SET_LINE_CYCLEBASED(hpos);
	uae_u16 datreg = cycle_line_pipe[hpos];
	// not conflict?
	if (!(datreg & CYCLE_PIPE_BITPLANE)) {
		hstrobe_conflict = false;
		int pos = hpos_to_diwx(hpos);
		addcc(pos, 0, COLOR_CHANGE_ACTBORDER | 2);
		MARK_LINE_CHANGED;
	}
}

static void finish_decisions(int hpos)
{
	struct amigadisplay *ad = &adisplays[0];
	struct draw_info *dip;
	struct draw_info *dip_old;
	struct decision *dp;
	int changed;

	// update decisions to start of hsync
	finish_partial_decision(hpos);

	finish_final_fetch(hpos);

	if (thisline_decision.plfleft >= 0 && thisline_decision.plflinelen < 0) {
		thisline_decision.plfright = thisline_decision.plfleft;
		thisline_decision.plflinelen = 0;
		thisline_decision.bplres = output_res(RES_LORES);
	}
	if (!ecs_denise) {
		if (thisline_decision.diwfirstword < hdiwbplstart) {
			thisline_decision.diwfirstword = hdiwbplstart;
		} else if (hdiwbplstart < 0) {
			thisline_decision.diwfirstword = -1;
			thisline_decision.diwlastword = -1;
		}
	}

#if 0
	if (hstrobe_conflict) {
		sync_color_changes(hpos + DDF_OFFSET / 2 + 1);
		decide_hstrobe_hdiw(hpos);
	}
#endif

	// Add DDF_OFFSET to detect blanking changes past hpos max
	sync_color_changes(hpos + DDF_OFFSET / 2 + 1);

	/* Large DIWSTOP values can cause the stop position never to be
	* reached, so the state machine always stays in the same state and
	* there's a more-or-less full-screen DIW. */
	if ((hdiwstate == diw_states::DIW_waiting_stop && thisline_decision.diwfirstword >= 0) || hstrobe_conflict) {
		thisline_decision.diwlastword = max_diwlastword;
	}

	if (thisline_decision.diwfirstword != line_decisions[next_lineno].diwfirstword) {
		MARK_LINE_CHANGED;
	}
	if (thisline_decision.diwlastword != line_decisions[next_lineno].diwlastword) {
		MARK_LINE_CHANGED;
	}

	dip = curr_drawinfo + next_lineno;
	dip_old = prev_drawinfo + next_lineno;
	dp = line_decisions + next_lineno;
	changed = thisline_changed | ad->custom_frame_redraw_necessary;
	if (thisline_decision.plfleft >= 0 && thisline_decision.nr_planes > 0) {
		record_diw_line(thisline_decision.plfleft, diwfirstword, diwlastword);
	}

	dip->last_sprite_entry = next_sprite_entry - 1;
	dip->last_color_change = next_color_change;

	if (thisline_decision.ctable < 0) {
		if (thisline_decision.plfleft < 0) {
			remember_ctable_for_border();
		} else {
			remember_ctable();
		}
	}

	dip->nr_color_changes = next_color_change - dip->first_color_change;
	if (dip->nr_color_changes < 0) {
		write_log("negative nr_color_changes: %d. FIXME!\n", dip->nr_color_changes);
		dip->nr_color_changes = 0;
	}
	dip->nr_sprites = next_sprite_entry - dip->first_sprite_entry;

	if (thisline_decision.plfleft != line_decisions[next_lineno].plfleft) {
		changed = 1;
	}
	if (!changed && color_changes_differ(dip, dip_old)) {
		changed = 1;
	}
	if (!changed && /* bitplane visible in this line OR border sprites enabled */
		(thisline_decision.plfleft >= 0 || ((thisline_decision.bplcon0 & 1) && (thisline_decision.bplcon3 & 0x02) && !(thisline_decision.bplcon3 & 0x20)))
		&& sprites_differ(dip, dip_old))
	{
		changed = 1;
	}

	if (changed) {
		thisline_changed = 1;
		*dp = thisline_decision;
	} else {
		/* The only one that may differ: */
		dp->ctable = thisline_decision.ctable;
	}

	check_collisions(hpos);

	if (next_color_change >= MAX_REG_CHANGE - 30) {
		write_log(_T("color_change buffer overflow!\n"));
		next_color_change = 0;
		last_color_change = 0;
		dip->nr_color_changes = 0;
		dip->first_color_change = 0;
		dip->last_color_change = 0;
	}
}

static void set_ocs_denise_blank(uae_u32 v)
{
	int hpos = current_hpos();
	sync_color_changes(hpos);
	int pos = hpos_to_diwx(hpos) + 2; // +1 hires
	record_color_change2(-pos, 0, COLOR_CHANGE_BLANK | 1);
}
static void reset_ocs_denise_blank(uae_u32 v)
{
	int hpos = current_hpos();
	sync_color_changes(hpos);
	int pos = hpos_to_diwx(hpos) + 2; // +1 hires
	record_color_change2(-pos, 0, COLOR_CHANGE_BLANK | 0);
}


/* Set the state of all decisions to "undecided" for a new scanline. */
static void reset_decisions_scanline_start(void)
{
	if (nodraw())
		return;

	if (hstrobe_conflict) {
		event2_newevent_xx(-1, REFRESH_FIRST_HPOS * CYCLE_UNIT, 0, hstrobe_conflict_check);
	}

	last_decide_line_hpos = 0;
	last_decide_sprite_hpos = 0;
	last_fetch_hpos = 0;
	last_copper_hpos = 0;
	ddfstrt_hpos = -1;
	ddfstop_hpos = -1;
	last_diw_hpos = 0;
	last_sprite_hpos = 0;
	last_sprite_hpos_reset = 0;
	blt_info.finishhpos = -1;

	/* Default to no bitplane DMA overriding sprite DMA */
	plfstrt_sprite = 0x100;
	sprbplconflict_hpos = -1;
	sprbplconflict_hpos2 = -1;
	bprun_end = 0;

	// clear sprite allocations
	for (int i = 0; i < maxhposm0; i++) {
		uae_u16 v = cycle_line_pipe[i];
		if (v & CYCLE_PIPE_SPRITE) {
			cycle_line_pipe[i] = 0;
		}
	}

	if (!ecs_denise && currprefs.gfx_overscanmode > OVERSCANMODE_OVERSCAN) {
		if (!ecs_agnus) {
			if (vb_start_line == 2 + vblank_extraline) {
				event2_newevent_xx(-1, 2 * CYCLE_UNIT, 0, set_ocs_denise_blank);
			}
		} else {
			if (vb_start_line == 1 + vblank_extraline) {
				event2_newevent_xx(-1, 2 * CYCLE_UNIT, 0, set_ocs_denise_blank);
			}
		}
		if (vb_end_next_line2) {
			event2_newevent_xx(-1, 2 * CYCLE_UNIT, 0, reset_ocs_denise_blank);
		}
	}

}

static void get_strobe_conflict_shift(int hpos)
{
	// Because Denise hcounter is not synced to Agnus hcounter,
	// BPLCON1 offset changes every line, causing jagged display.
	int unalign = hpos * 2 + hdiw_counter;
	unalign /= 2;
	unalign -= 4;
	unalign &= 7;
	unalign *= 2;
	delay_cycles = unalign << LORES_TO_SHRES_SHIFT;
}

static uae_u16 BPLCON0_Denise_mask(uae_u16 v)
{
	if (!ecs_denise) {
		v &= ~0x00F1;
	} else if (!aga_mode) {
		v &= ~0x00B0;
	}

	v &= ~((currprefs.cs_color_burst ? 0x0000 : 0x0200) | 0x0100 | 0x0080 | 0x0020);
#if SPRBORDER
	v |= 1;
#endif
	return v;
}

static uae_u16 BPLCON0_Agnus_mask(uae_u16 v)
{
	if (!ecs_agnus) {
		v &= 0xff0f;
	} else if (!aga_mode) {
		v &= 0xffcf;
	}
	return v;
}

static void reset_decisions_hsync_start(void)
{
	if (nodraw()) {
		return;
	}

	int hpos = current_hpos();

	thisline_decision.diwfirstword = -1;
	thisline_decision.diwlastword = -1;
	// hdiw already open?
	if (hdiwstate == diw_states::DIW_waiting_stop) {
		thisline_decision.diwfirstword = min_diwlastword;
		if (thisline_decision.diwfirstword != line_decisions[next_lineno].diwfirstword) {
			MARK_LINE_CHANGED;
		}
	}
	hdiwbplstart = -1;

	thisline_decision.ctable = -1;
	thisline_changed = 0;

	curr_drawinfo[next_lineno].first_color_change = next_color_change;
	last_color_change = next_color_change;
	curr_drawinfo[next_lineno].first_sprite_entry = next_sprite_entry;
	next_sprite_forced = 1;
	last_sprite_point_abs = ((1 + (hpos - REFRESH_FIRST_HPOS - 2)) << 1) + 1;
	hdiw_denisecounter_abs = ((1 + (hpos - REFRESH_FIRST_HPOS - 2)) << CCK_SHRES_SHIFT) + 0;

	sprites_enabled_this_line = false;
	bpl1mod_hpos = -1;
	bpl2mod_hpos = -1;
	last_recorded_diw_hpos = 0;
	collision_hpos = 0;

	vhposr_delay_offset = 0;
	vhposw_modified = false;

	compute_toscr_delay(bplcon1);

	last_diwlastword = -1;
	hb_last_diwlastword = -1;

	if (line_cyclebased > 0) {
		line_cyclebased--;
	}

	if (toscr_scanline_complex_bplcon1_off) {
		toscr_scanline_complex_bplcon1_off = false;
		toscr_scanline_complex_bplcon1 = false;
	}

#if BPL_ERASE_TEST
	for (int n = 0; n < 8; n++) {
		static uae_u8 v;
		v += 2;
		uae_u8 *dataptr = line_data[next_lineno] + n * MAX_WORDS_PER_LINE * 2;
		memset(dataptr, v ^ vpos, MAX_WORDS_PER_LINE * 2);
	}
#endif

	if (toscr_nr_planes3 < toscr_nr_planes2 && toscr_nr_planes3 > 0 && out_offs > 0) {
		for (int n = toscr_nr_planes3; n < toscr_nr_planes2; n++) {
			uae_u8 *dataptr = line_data[next_lineno] + n * MAX_WORDS_PER_LINE * 2;
			memset(dataptr, 0, out_offs * 4);
		}
	}

	reset_bpl_vars();

	/* These are for comparison. */
	thisline_decision.bplcon0 = bplcon0;
	thisline_decision.bplcon2 = bplcon2;
#ifdef ECS_DENISE
	thisline_decision.bplcon3 = bplcon3;
#endif
#ifdef AGA
	thisline_decision.bplcon4bm = bplcon4;
	thisline_decision.bplcon4sp = bplcon4;
	thisline_decision.fmode = fmode;

	if (!aga_mode && ecs_denise && exthblank) {
		// ECS Denise + EXTBLANK: VBLANK blanking is different.
		if (new_beamcon0 & BEAMCON0_BLANKEN) {
			// Follow Agnus VBLANK state directly via CSYNC connection. Ignore strobe vblank state.
			thisline_decision.vb = vb_state || vb_end_line ? VB_PRGVB : VB_NOVB;
		} else {
			// CSYNC: follow CSYNC state
			thisline_decision.vb = vs_state_on ? VB_PRGVB : VB_NOVB;
		}
	} else if (ecs_agnus) {
		// Visible vblank end is delayed by 1 line
		thisline_decision.vb = vb_start_line >= 2 + vblank_extraline || vb_end_next_line ? 0 : VB_NOVB;
		if (hstrobe_conflict && strobe_vblank && thisline_decision.vb) {
			thisline_decision.vb = VB_XBLANK;
		}
	} else {
		thisline_decision.vb = vb_start_line >= 2 + vblank_extraline || vb_end_next_line ? 0 : VB_NOVB;
		if (hstrobe_conflict && strobe_vblank && thisline_decision.vb) {
			thisline_decision.vb = VB_XBLANK;
		}
	}

	// if programmed vblank
	if ((new_beamcon0 & BEAMCON0_VARVBEN) && ecs_agnus) {
		if (!thisline_decision.vb) {
			thisline_decision.vb |= VB_PRGVB;
		}
	}

	// doublescan last line garbage workaround
	if (doflickerfix_active()) {
		if (!(thisline_decision.vb & VB_PRGVB)) {
			if ((vpos < maxvpos_display_vsync) || (lof_display && vpos >= maxvpos - 1) || (!lof_display && vpos >= maxvpos - 1)) {
				thisline_decision.vb = VB_XBORDER;
			}
		}
	}
#endif

	if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
		if (thisline_decision.vb != VB_NOVB) {
			thisline_decision.vb = VB_VB | VB_NOVB;
		}
		if ((vs_state_var && (new_beamcon0 & BEAMCON0_VARVSYEN)) || (vs_state_hw && !(new_beamcon0 & BEAMCON0_VARVSYEN))) {
			thisline_decision.vb |= VB_VS;
		}
	}
	if (nosignal_status == 1) {
		thisline_decision.vb = VB_XBLANK;
		MARK_LINE_CHANGED;
	} else if (nosignal_status < 0) {
		MARK_LINE_CHANGED;
	}

	int left = thisline_decision.plfleft;

	bpl_shifter = 0;
	thisline_decision.plfleft = -1;
	thisline_decision.plflinelen = -1;
	thisline_decision.plfright = -1;
	thisline_decision.ham_seen = isham(bplcon0);
	thisline_decision.ehb_seen = isehb(bplcon0, bplcon2);
	thisline_decision.ham_at_start = (bplcon0 & 0x800) != 0;
	thisline_decision.bordersprite_seen = issprbrd(-1, bplcon0, bplcon3);
	thisline_decision.xor_seen = (bplcon4 & 0xff00) != 0;
	thisline_decision.nr_planes = toscr_nr_planes_agnus;

	// workaround for glitches in faster modes
	// update Denise state immediately if bitplane DMA is idle and shifters are empty
	if (!bprun && !plane0 && !plane0p) {
		uae_u16 bcon0 = BPLCON0_Denise_mask(bplcon0);
		toscr_nr_planes_shifter = GET_PLANES(bcon0);
	}

	toscr_nr_planes2 = GET_PLANES(bplcon0d);
	if (isocs7planes()) {
		toscr_nr_planes2 = std::max(toscr_nr_planes2, 6);
	}
	toscr_nr_planes3 = toscr_nr_planes2;
	toscr_nr_changed = false;

	thisline_decision.max_planes = toscr_nr_planes2 > toscr_nr_planes_agnus ? toscr_nr_planes2 : toscr_nr_planes_agnus;

	hpos_hsync_extra = 0;
	bool normalstart = true;
	plane0p_enabled = ecs_denise && (bplcon0 & 1) && (bplcon3 & 0x20);
	plane0p_forced = false;
	plane0_first_done = false;
	speedup_first = true;
	delay_cycles = ((hpos) * 2 - DDF_OFFSET + 0) << LORES_TO_SHRES_SHIFT;
	delay_cycles2 = delay_cycles;
	set_delay_lastcycle();
	if (hstrobe_conflict) {
		get_strobe_conflict_shift(hpos);
	}

	if (1 && fetchmode >= 2 && !doflickerfix_active()) {
		// handle bitplane data wrap around
		bool toshift = false;
		if ((exthblank || (beamcon0 & bemcon0_hsync_mask)) && (thisline_decision.bplres == 0 || currprefs.chipset_hr)) {
			for (int i = 0; i < thisline_decision.nr_planes; i++) {
				if (todisplay2_aga_saved[i]) {
					toshift = true;
				}
			}
		}
		if (bprun != 0 || todisplay_fetched || plane0 || plane0p || toshift) {
			normalstart = false;
			SET_LINE_CYCLEBASED(hpos);
			if (toscr_hend == 2) {
				for (int i = 0; i < MAX_PLANES; i++) {
					todisplay[i] = todisplay_saved[i];
					todisplay_aga[i] = todisplay_aga_saved[i];
					todisplay2[i] = todisplay2_saved[i];
					todisplay2_aga[i] = todisplay2_aga_saved[i];
				}
				todisplay_fetched = todisplay_fetched_saved;
			}
			if (plane0p) {
				beginning_of_plane_block_early(hpos);
				plane0p = false;
			}
			if (plane0) {
				beginning_of_plane_block(hpos);
				plane0 = false;
			}
			// make sure bprun stays enabled until next line
			bprun_pipeline_flush_delay = maxhpos;
			plane0p_enabled = true;
			plane0p_forced = true;
			bpl_shifter = -1;
		}
	}

	// HBLANK start before HSYNC (or HBLANK never triggered) and BPL1DAT between HBLANK and HSYNC?
	if (exthblank) {
		int hblankstart = -1;
		if (last_hblank_start >= 0) {
			hblankstart = diw_to_hpos(last_hblank_start + 4);
		} else if (exthblank_state) {
			hblankstart = last_bpl1dat_hpos - 1;
		} else {
			// AGA only, ECS Denise sees HBLANK in CSYNC blank line which always opens border.
			if (aga_mode) {
				// no hblank start: Border stays in closed state
				start_noborder(hpos);
			}
		}
		if (hblankstart >= 0) {
			int bpl1 = last_bpl1dat_hpos;
			if (bpl1 < hblankstart) {
				bpl1 += maxhpos;
			}
			int hp = hpos;
			if (hp < hblankstart || hp < bpl1) {
				hp += maxhpos;
			}
			int comp = maxhpos / 2 - 1;
			if (bpl1 > hblankstart && bpl1 < hp && (bpl1 - hblankstart) < comp && (bpl1 - hp) < comp) {
				// Close border. (HBLANK start opens border, BPL1DAT closes it)
				start_noborder(hpos);
			}
		}
	}
	// vblank end and bitplane was active inside vblank: borderblank bug
	if (ecs_agnus && ecs_denise && vb_end_next_line2 && last_bpl1dat_hpos >= 0) {
		start_noborder(hpos);
	}

	if (last_bpl1dat_hpos_early && !bpl_shifter) {
		// BPLxDAT access already happened between refresh slot and hsync (this hpos):
		// Close border
		start_noborder(hpos);
	}

	if (normalstart) {
		plane0 = false;
		plane0p = false;
		memset(outword, 0, sizeof outword);
		// fetched[] must not be cleared (Sony VX-90 / Royal Amiga Force)
		todisplay_fetched = 0;
		//memset(todisplay, 0, sizeof todisplay);
		memset(todisplay2, 0, sizeof todisplay2);
#ifdef AGA
		if (aga_mode || ALL_SUBPIXEL) {
			//memset(todisplay_aga, 0, sizeof todisplay_aga);
			memset(todisplay2_aga, 0, sizeof todisplay2_aga);
			memset(outword64, 0, sizeof outword64);
			memset(outword64_extra, 0, sizeof outword64_extra);
		}
#endif
	}

#ifdef DEBUGGER
	if (debug_dma && (new_beamcon0 & bemcon0_hsync_mask)) {
		record_dma_event(DMA_EVENT_HSS, hpos, vpos);
		record_dma_event(DMA_EVENT_HSE, hsstop, vpos);
	}
#endif

	toscr_hend = 0;
	last_bpl1dat_hpos = -1;
	last_bpl1dat_hpos_early = false;
	last_hblank_start = -1;
	hcenterblank_state = false;
	hstrobe_hdiw_min = hsyncstartpos_start_cycles;
	if (hstrobe_conflict) {
		SET_LINE_CYCLEBASED(hpos);
	}
}

frame_time_t vsynctimebase_orig;

void compute_vsynctime(void)
{
	float svpos = maxvpos_nom + 0.0f;
	float shpos = maxhpos_short + 0.0f;
	float syncadjust = 1.0;

	fake_vblank_hz = 0;
	vblank_hz_mult = 0;
	vblank_hz_state = 1;
	if (fabs (currprefs.chipset_refreshrate) > 0.1) {
		syncadjust = currprefs.chipset_refreshrate / vblank_hz_nom;
		vblank_hz = currprefs.chipset_refreshrate;
		if (isvsync_chipset() && !currprefs.gfx_variable_sync) {
			int mult = 0;
			if (getvsyncrate(0, vblank_hz, &mult) != vblank_hz) {
				vblank_hz = getvsyncrate(0, vblank_hz, &vblank_hz_mult);
				if (vblank_hz_mult > 0) {
					vblank_hz_state = 0;
				}
			}
		}
	}
	if (!fake_vblank_hz)
		fake_vblank_hz = vblank_hz;

	if (currprefs.turbo_emulation) {
		if (currprefs.turbo_emulation_limit > 0) {
			vsynctimebase = (frame_time_t)(syncbase / currprefs.turbo_emulation_limit);
		} else {
			vsynctimebase = 1;
		}
	} else {
		vsynctimebase = (frame_time_t)(syncbase / fake_vblank_hz);
	}
	vsynctimebase_orig = vsynctimebase;

	if (linetoggle) {
		shpos += 0.5f;
	}
	if (interlace_seen) {
		svpos += 0.5f;
	} else if (lof_display) {
		svpos += 1.0f;
	}
	if (currprefs.produce_sound > 1) {
		float clk = svpos * shpos * fake_vblank_hz;
		write_log(_T("SNDRATE %.1f*%.1f*%.6f=%.6f\n"), svpos, shpos, fake_vblank_hz, clk);
		devices_update_sound(clk, syncadjust);
	}
	devices_update_sync(svpos, syncadjust);
}

void getsyncregisters(uae_u16 *phsstrt, uae_u16 *phsstop, uae_u16 *pvsstrt, uae_u16 *pvsstop)
{
	*phsstrt = hsstrt;
	*phsstop = hsstop_detect2 / 2;
	*pvsstrt = vsstrt;
	*pvsstop = vsstop;
}

static void dumpsync(void)
{
	static int cnt = 100;
	if (cnt < 0) {
		return;
	}
	cnt--;
	write_log(_T("BEAMCON0=%04X VTOTAL=%04X (%03d) HTOTAL=%04X (%03d)\n"), new_beamcon0, vtotal, vtotal, htotal, htotal);
	write_log(_T(" HS=%04X-%04X (%d-%d) HB=%04X-%04X (%d-%d) HC=%04X (%d)\n"),
		hsstrt, hsstop, hsstrt, hsstop, hbstrt, hbstop, hbstrt, hbstop, hcenter, hcenter);
	write_log(_T(" VS=%04X-%04X (%d-%d) VB=%04X-%04X (%d-%d)\n"),
		vsstrt, vsstop, vsstrt, vsstop, vbstrt, vbstop, vbstrt, vbstop);
	write_log(_T(" HSYNCSTART=%04X.%X (%d.%d) HSYNCEND=%04X.%X (%d.%d)\n"),
		hsyncstartpos >> CCK_SHRES_SHIFT, hsyncstartpos & ((1 << CCK_SHRES_SHIFT) - 1),
		hsyncstartpos >> CCK_SHRES_SHIFT, hsyncstartpos & ((1 << CCK_SHRES_SHIFT) - 1),
		hsyncendpos >> CCK_SHRES_SHIFT, hsyncendpos & ((1 << CCK_SHRES_SHIFT) - 1),
		hsyncendpos >> CCK_SHRES_SHIFT, hsyncendpos &((1 << CCK_SHRES_SHIFT) - 1));
	write_log(_T(" Lines=%04X-%04X (%d-%d)\n"),
		minfirstline, maxvpos_display + maxvpos_display_vsync,
		minfirstline, maxvpos_display + maxvpos_display_vsync);
	write_log(_T("PC=%08x COP=%08x\n"), M68K_GETPC, cop_state.ip);
}

int current_maxvpos(void)
{
	return maxvpos + (lof_store ? 1 : 0);
}

#if 0
static void checklacecount(bool lace)
{
	if (!interlace_changed) {
		if (nlace_cnt >= NLACE_CNT_NEEDED && lace) {
			lof_togglecnt_lace = LOF_TOGGLES_NEEDED;
			lof_togglecnt_nlace = 0;
			//write_log (_T("immediate lace\n"));
			nlace_cnt = 0;
		} else if (nlace_cnt <= -NLACE_CNT_NEEDED && !lace) {
			lof_togglecnt_nlace = LOF_TOGGLES_NEEDED;
			lof_togglecnt_lace = 0;
			//write_log (_T("immediate nlace\n"));
			nlace_cnt = 0;
		}
	}
	if (lace) {
		if (nlace_cnt > 0)
			nlace_cnt = 0;
		nlace_cnt--;
		if (nlace_cnt < -NLACE_CNT_NEEDED * 2)
			nlace_cnt = -NLACE_CNT_NEEDED * 2;
	} else if (!lace) {
		if (nlace_cnt < 0)
			nlace_cnt = 0;
		nlace_cnt++;
		if (nlace_cnt > NLACE_CNT_NEEDED * 2)
			nlace_cnt = NLACE_CNT_NEEDED * 2;
	}
}
#endif

static void set_hcenter(void)
{
	hcenter_sync_v2 = (hcenter & 0xff) << CCK_SHRES_SHIFT;
	if (!aga_mode && ecs_denise) {
		if (beamcon0 & bemcon0_vsync_mask) {
			hcenter_v2 = (hcenter & 0xff) << CCK_SHRES_SHIFT;
		} else {
			hcenter_v2 = 132 << CCK_SHRES_SHIFT;
		}
		uae_u16 hc = hcenter_v2;
		hcenter_v2 -= 3;
		if (hc >= (1 << CCK_SHRES_SHIFT) && hcenter_v2 < (1 << CCK_SHRES_SHIFT)) {
			hcenter_v2 += maxhpos << CCK_SHRES_SHIFT;
		}
		hcenter_v2 = adjust_hr(hcenter_v2);
		if (hcenter_v2 < (1 << CCK_SHRES_SHIFT)) {
			hcenter_v2 = 0;
		}
		hcenter_v2_end = hcenter_v2;
		hcenter_v2_end += ((beamcon0 & BEAMCON0_PAL) ? 8 : 9) << CCK_SHRES_SHIFT;
		if (hcenter_v2_end >= maxhpos << CCK_SHRES_SHIFT) {
			hcenter_v2_end -= maxhpos << CCK_SHRES_SHIFT;
		}
		hcenter_active = true;
	} else {
		if (beamcon0 & bemcon0_vsync_mask) {
			hcenter_v2 = (hcenter & 0xff) << CCK_SHRES_SHIFT;
		} else {
			hcenter_v2 = 132 << CCK_SHRES_SHIFT;
		}
		hcenter_active = false;
	}
}

static void updateextblk(void)
{
	hsyncstartpos_start_hw = 13;
	hsyncstartpos_hw = maxhpos_short + hsyncstartpos_start_hw;
	hsyncendpos_hw = 24;

	hbstrt_v = (hbstrt & 0xff) << CCK_SHRES_SHIFT;
	hbstop_v = (hbstop & 0xff) << CCK_SHRES_SHIFT;
	// Agnus/Alice side HBxxxx, used by VARCSY
	hbstrt_sync_v2 = hbstrt_v;
	hbstop_sync_v2 = hbstop_v;

	if (aga_mode) {
		// AGA horizontal blanking is simple
		hbstrt_v |= (hbstrt >> 8) & 7;
		hbstop_v |= (hbstop >> 8) & 7;
		// hblank has 1 lores pixel offset
		hbstrt_v2 = hbstrt_v - 4;
		if (hbstrt_v >= (1 << CCK_SHRES_SHIFT) && hbstrt_v2 < (1 << CCK_SHRES_SHIFT)) {
			hbstrt_v2 += maxhpos_short << CCK_SHRES_SHIFT;
		}
		hbstop_v2 = hbstop_v - 4;
		if (hbstop_v >= (1 << CCK_SHRES_SHIFT) && hbstop_v2 < (1 << CCK_SHRES_SHIFT)) {
			hbstop_v2 += maxhpos_short << CCK_SHRES_SHIFT;
		}
		exthblank = (bplcon0 & 1) && (bplcon3 & 1);
	} else {
		// ECS Denise CSYNC horizontal blanking is complex
		if (new_beamcon0 & BEAMCON0_VARCSYEN) {
			// ECS Denise HBLANK uses CSYNC input and matches programmed HSYNC period.
			hbstrt_v2 = (hsstrt & 0xff) << CCK_SHRES_SHIFT;
			hbstop_v2 = (hsstop & 0xff) << CCK_SHRES_SHIFT;
		} else if (new_beamcon0 & BEAMCON0_BLANKEN) {
			// HBSTRT/HBSTOP blanking
			hbstrt_v2 = hbstrt_v;
			hbstop_v2 = hbstop_v;
		} else {
			// ECS Denise HBLANK uses CSYNC input and matches hardwired HSYNC period.
			hbstrt_v2 = 18 << CCK_SHRES_SHIFT;
			hbstop_v2 = 35 << CCK_SHRES_SHIFT;
		}
		if (new_beamcon0 & BEAMCON0_CSYTRUE) {
			uae_u16 tmp = hbstrt_v2;
			hbstrt_v2 = hbstop_v2;
			hbstop_v2 = tmp;
		}
		hbstrt_v = hbstrt_v2;
		hbstop_v = hbstop_v2;
		// hblank has 1 lores pixel offset
		hbstrt_v2 -= 4;
		hbstop_v2 -= 4;
		if (hbstrt_v >= (1 << CCK_SHRES_SHIFT) && hbstrt_v2 < (1 << CCK_SHRES_SHIFT)) {
			hbstrt_v2 += maxhpos_short << CCK_SHRES_SHIFT;
		}
		if (hbstop_v >= (1 << CCK_SHRES_SHIFT) && hbstop_v2 < (1 << CCK_SHRES_SHIFT)) {
			hbstop_v2 += maxhpos_short << CCK_SHRES_SHIFT;
		}
		exthblank = (bplcon0 & 1) && (bplcon3 & 1);
	}

	if (!exthblank) {
		hbstrt_v2 = (8 << CCK_SHRES_SHIFT) - 3;
		if (!ecs_denise) {
			hbstrt_v2 -= 4;
		}
		hbstop_v2 = (47 << CCK_SHRES_SHIFT) - 7;
		hbstrt_v2 = adjust_hr(hbstrt_v2);
		hbstop_v2 = adjust_hr(hbstop_v2);
	}

	hbstrt_reg = hbstrt_v2;
	hbstop_reg = hbstop_v2;

	if ((new_beamcon0 & bemcon0_hsync_mask) && (!currprefs.monitoremu || currprefs.cs_hvcsync > 0)) {

		hsyncstartpos = hsstrt + 2;

		if (hsyncstartpos >= maxhpos_short) {
			hsyncstartpos -= maxhpos_short;
		}
		hsyncendpos = hsstop;

		hsstop_detect2 = (hsstrt + 21) * 2;
		if (hsstop_detect2 >= maxhpos_short * 2) {
			hsstop_detect2 -= maxhpos_short * 2;
		}

		hsyncstartpos_start = hsyncstartpos;
		if (hsyncstartpos < maxhpos_short / 2) {
			hsyncstartpos = maxhpos_short + hsyncstartpos_start;
			denisehtotal = hsyncstartpos;
		} else {
			denisehtotal = maxhpos_short + hsyncstartpos;
		}
		// make sure possible BPL DMA cycles before first refresh slot are processed before hsync
		hsyncstartpos_start = std::max(hsyncstartpos_start, REFRESH_FIRST_HPOS + 1);

		hsstrt_v2 = (hsstrt & 0xff) << CCK_SHRES_SHIFT;
		hsstop_v2 = (hsstop & 0xff) << CCK_SHRES_SHIFT;

	} else {

		hsyncstartpos_start = hsyncstartpos_start_hw;
		hsyncstartpos = hsyncstartpos_hw;
		denisehtotal = maxhpos_short + 7;
		hsstop_detect2 = (35 + 9) * 2;

		hsstrt_v2 = 18 << CCK_SHRES_SHIFT;
		hsstop_v2 = 35 << CCK_SHRES_SHIFT;
		hsyncendpos = hsyncendpos_hw;

	}

	hsstrt_v2 = adjust_hr(hsstrt_v2);
	hsstop_v2 = adjust_hr(hsstop_v2);


	// Out of range left. Denise/Lisa hcounter starts from 2 (skips first 2 lores pixels)
	if (hbstrt_v2 < (1 << CCK_SHRES_SHIFT)) {
		hbstrt_v2 = 0xffff;
	}
	if (hbstop_v2 < (1 << CCK_SHRES_SHIFT)) {
		hbstop_v2 = 0xffff;
	}
	// Out of range right
	if (hbstrt_v2 >= (maxhpos_short << CCK_SHRES_SHIFT) + (1 << CCK_SHRES_SHIFT)) {
		hbstrt_v2 = 0xffff;
	}
	if (hbstop_v2 >= (maxhpos_short << CCK_SHRES_SHIFT) + (1 << CCK_SHRES_SHIFT)) {
		hbstop_v2 = 0xffff;
	}
	// hsync start offset adjustment
	if (hbstrt_v2 <= (hsyncstartpos_start << CCK_SHRES_SHIFT)) {
		hbstrt_v2 += maxhpos_short << CCK_SHRES_SHIFT;
		if (hbstrt_v2 > (maxhpos_short + hsyncstartpos_start_cycles) << CCK_SHRES_SHIFT) {
			hbstrt_v2 = 1 << CCK_SHRES_SHIFT;
		}
	}
	if (hbstop_v2 <= (hsyncstartpos_start << CCK_SHRES_SHIFT)) {
		hbstop_v2 += maxhpos_short << CCK_SHRES_SHIFT;
		if (hbstop_v2 > (maxhpos_short + hsyncstartpos_start_cycles) << CCK_SHRES_SHIFT) {
			hbstop_v2 = 1 << CCK_SHRES_SHIFT;
		}
	}

	int strt = hbstrt_v2;
	int stop = hbstop_v2;
	hbstrt_v2 = adjust_hr(hbstrt_v2);
	hbstop_v2 = adjust_hr(hbstop_v2);
	// if same after rounding: increase start/stop by minimum unit
	if (hbstrt_v2 == hbstop_v2) {
		int add = currprefs.chipset_hr ? (1 << currprefs.gfx_resolution) : 4;
		if (strt < stop) {
			hbstop_v2 += add;
			if (hbstop_v2 >= (maxhpos_short << CCK_SHRES_SHIFT) + (1 << CCK_SHRES_SHIFT)) {
				hbstop_v2 = 0;
			}
		} else if (strt > stop) {
			hbstrt_v2 += add;
			if (hbstrt_v2 >= (maxhpos_short << CCK_SHRES_SHIFT) + (1 << CCK_SHRES_SHIFT)) {
				hbstrt_v2 = 0;
			}
		}
	}

	hsyncstartpos_start_hw <<= CCK_SHRES_SHIFT;
	hsyncstartpos_hw <<= CCK_SHRES_SHIFT;
	hsyncendpos_hw <<= CCK_SHRES_SHIFT;

	hsyncstartpos_start_cycles = hsyncstartpos_start;

	hsyncstartpos_start <<= CCK_SHRES_SHIFT;
	hsyncstartpos <<= CCK_SHRES_SHIFT;
	hsyncendpos <<= CCK_SHRES_SHIFT;

	denisehtotal <<= CCK_SHRES_SHIFT;

	// ECS Denise has 1 extra lores pixel in right border
	if (ecs_denise) {
		denisehtotal += 1 << (CCK_SHRES_SHIFT - 1);
	}

	set_hcenter();

	calcdiw();
}

struct chipset_refresh *get_chipset_refresh(struct uae_prefs *p)
{
	struct amigadisplay *ad = &adisplays[0];
	int islace = interlace_seen ? 1 : 0;
	int isntsc = (beamcon0 & BEAMCON0_PAL) ? 0 : 1;
	int custom = programmedmode == 1 ? 1 : 0;

	if (!ecs_agnus) {
		isntsc = currprefs.ntscmode ? 1 : 0;
	}

	int def = -1;
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
		struct chipset_refresh *cr = &p->cr[i];
		if (cr->defaultdata)
			def = i;
		if (cr->inuse) {
			if ((cr->horiz < 0 || cr->horiz == maxhpos) &&
				(cr->vert < 0 || cr->vert == maxvpos_display) &&
				(cr->ntsc < 0 || (cr->ntsc == 1 && isntsc && !custom) || (cr->ntsc == 0 && !isntsc && !custom) || (cr->ntsc == 2 && custom)) &&
				(cr->lace < 0 || (cr->lace > 0 && islace) || (cr->lace == 0 && !islace)) &&
				(cr->resolution == 0 || cr->resolution == 7 || (cr->resolution & (1 << detected_screen_resolution))) &&
				(cr->framelength < 0 || (cr->framelength > 0 && lof_store) || (cr->framelength == 0 && !lof_store) || (cr->framelength >= 0 && islace)) &&
				((cr->rtg && ad->picasso_on) || (!cr->rtg && !ad->picasso_on)) &&
				(cr->vsync < 0 || (cr->vsync > 0 && isvsync_chipset ()) || (cr->vsync == 0 && !isvsync_chipset ())))
					return cr;
		}
	}
	if (def >= 0) {
		return &p->cr[def];
	}
	return NULL;
}

static bool changed_chipset_refresh(void)
{
	return stored_chipset_refresh != get_chipset_refresh(&currprefs);
}

void compute_framesync(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct amigadisplay *ad = &adisplays[0];
	int islace = interlace_seen ? 1 : 0;
	int isntsc = (beamcon0 & BEAMCON0_PAL) ? 0 : 1;
	bool found = false;

	if (islace) {
		vblank_hz = vblank_hz_lace;
	} else if (lof_store) {
		vblank_hz = vblank_hz_lof;
	} else {
		vblank_hz = vblank_hz_shf;
	}

	vblank_hz = target_adjust_vblank_hz(0, vblank_hz);

	struct chipset_refresh *cr = get_chipset_refresh(&currprefs);
	while (cr) {
		float v = -1;
		if (!ad->picasso_on && !ad->picasso_requested_on) {
			if (isvsync_chipset ()) {
				if (!currprefs.gfx_variable_sync) {
					if (cr->index == CHIPSET_REFRESH_PAL || cr->index == CHIPSET_REFRESH_NTSC) {
						if ((fabs(vblank_hz - 50.0f) < 1 || fabs(vblank_hz - 60.0f) < 1 || fabs(vblank_hz - 100.0f) < 1 || fabs(vblank_hz - 120.0f) < 1) && currprefs.gfx_apmode[0].gfx_vsync == 2 && currprefs.gfx_apmode[0].gfx_fullscreen > 0) {
							vsync_switchmode(0, (int)vblank_hz);
						}
					}
					if (isvsync_chipset() < 0) {

						float v2;
						v2 = target_getcurrentvblankrate(0);
						if (!cr->locked)
							v = v2;
					} else if (isvsync_chipset() > 0) {
						if (currprefs.gfx_apmode[0].gfx_refreshrate) {
							v = (float)abs(currprefs.gfx_apmode[0].gfx_refreshrate);
						}
					}
				}
			} else {
				if (cr->locked == false) {
					changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = vblank_hz;
					cfgfile_parse_lines (&changed_prefs, cr->commands, -1);
					if (cr->commands[0]) {
						write_log(_T("CMD1: '%s'\n"), cr->commands);
					}
					break;
				} else {
					v = cr->rate;
				}
			}
			if (v < 0)
				v = cr->rate;
			if (v > 0) {
				changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = v;
				cfgfile_parse_lines (&changed_prefs, cr->commands, -1);
				if (cr->commands[0]) {
					write_log(_T("CMD2: '%s'\n"), cr->commands);
				}
			}
		} else {
			if (cr->locked == false)
				v = vblank_hz;
			else
				v = cr->rate;
			changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = v;
			cfgfile_parse_lines (&changed_prefs, cr->commands, -1);
			if (cr->commands[0]) {
				write_log(_T("CMD3: '%s'\n"), cr->commands);
			}
		}
		found = true;
		break;
	}
	if (!found) {
		changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate = vblank_hz;
	}
	stored_chipset_refresh = cr;
	interlace_changed = 0;
	lof_togglecnt_lace = 0;
	lof_togglecnt_nlace = 0;
	//nlace_cnt = NLACE_CNT_NEEDED;
	lof_changing = 0;
	vidinfo->drawbuffer.inxoffset = -1;
	vidinfo->drawbuffer.inyoffset = -1;
	updateextblk();

	hsync_end_left_border = hsstop_detect;

	int res = GET_RES_AGNUS(bplcon0);
	int eres = 0;

	int res2 = currprefs.gfx_resolution;

	if (doublescan > 0) {
		res2++;
		eres++;
	}
	if (res2 > RES_MAX) {
		res2 = RES_MAX;
		eres--;
	}

	hsync_end_left_border <<= eres;

	int vres2 = currprefs.gfx_vresolution;
	if (doublescan > 0 && !islace) {
		vres2--;
	}

	vres2 = std::max(vres2, 0);
	vres2 = std::min(vres2, VRES_QUAD);

	vidinfo->drawbuffer.inwidth = maxhpos_display << res2;
	vidinfo->drawbuffer.inwidth2 = vidinfo->drawbuffer.inwidth;
	vidinfo->drawbuffer.extrawidth = -2;
	if (currprefs.gfx_extrawidth > 0) {
		vidinfo->drawbuffer.extrawidth = currprefs.gfx_extrawidth << res2;
	}
	vidinfo->drawbuffer.extraheight = -2;
	if (currprefs.gfx_extraheight > 0) {
		vidinfo->drawbuffer.extraheight = currprefs.gfx_extraheight << vres2;
	}
	if (vidinfo->drawbuffer.extrawidth == -2 && ((new_beamcon0 & (BEAMCON0_VARVBEN | bemcon0_vsync_mask)) || currprefs.gfx_overscanmode >= OVERSCANMODE_EXTREME)) {
		vidinfo->drawbuffer.extrawidth = -1;
	}
	vidinfo->drawbuffer.inheight = maxvsize_display << vres2;
	vidinfo->drawbuffer.inheight2 = vidinfo->drawbuffer.inheight;
	vidinfo->drawbuffer.inxoffset = 0;

	//write_log(_T("Width %d Height %d\n"), vidinfo->drawbuffer.inwidth, vidinfo->drawbuffer.inheight);

	vidinfo->drawbuffer.inwidth = std::max(vidinfo->drawbuffer.inwidth, 16);
	vidinfo->drawbuffer.inwidth2 = std::max(vidinfo->drawbuffer.inwidth2, 16);
	vidinfo->drawbuffer.inheight = std::max(vidinfo->drawbuffer.inheight, 1);
	vidinfo->drawbuffer.inheight2 = std::max(vidinfo->drawbuffer.inheight2, 1);

	vidinfo->drawbuffer.inwidth = std::min(vidinfo->drawbuffer.inwidth, vidinfo->drawbuffer.width_allocated);
	vidinfo->drawbuffer.inwidth2 = std::min(vidinfo->drawbuffer.inwidth2, vidinfo->drawbuffer.width_allocated);

	vidinfo->drawbuffer.inheight = std::min(vidinfo->drawbuffer.inheight, vidinfo->drawbuffer.height_allocated);
	vidinfo->drawbuffer.inheight2 = std::min(vidinfo->drawbuffer.inheight2, vidinfo->drawbuffer.height_allocated);

	vidinfo->drawbuffer.outwidth = vidinfo->drawbuffer.inwidth;
	vidinfo->drawbuffer.outheight = vidinfo->drawbuffer.inheight;

	vidinfo->drawbuffer.outwidth = std::min(vidinfo->drawbuffer.outwidth, vidinfo->drawbuffer.width_allocated);

	vidinfo->drawbuffer.outheight = std::min(vidinfo->drawbuffer.outheight, vidinfo->drawbuffer.height_allocated);

	memset(line_decisions, 0, sizeof(line_decisions));
	memset(line_drawinfo, 0, sizeof(line_drawinfo));
	for (auto& line_decision : line_decisions)
	{
		line_decision.plfleft = -2;
	}

	check_nocustom();

	compute_vsynctime();

	hblank_hz = (currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL) / (maxhpos + (linetoggle ? 0.5f : 0.0f));

	write_log(_T("%s mode%s%s V=%.4fHz H=%0.4fHz (%dx%d+%d) IDX=%d (%s) D=%d RTG=%d/%d\n"),
		isntsc ? _T("NTSC") : _T("PAL"),
		islace ? _T(" lace") : (lof_lace ? _T(" loflace") : _T("")),
		doublescan > 0 ? _T(" dblscan") : _T(""),
		vblank_hz,
		hblank_hz,
		maxhpos, maxvpos, lof_store ? 1 : 0,
		cr ? cr->index : -1,
		cr != NULL && cr->label != NULL ? cr->label : _T("<?>"),
		currprefs.gfx_apmode[ad->picasso_on ? 1 : 0].gfx_display, ad->picasso_on, ad->picasso_requested_on
	);

	set_config_changed();

	if (currprefs.monitoremu_mon != 0) {
		target_graphics_buffer_update(currprefs.monitoremu_mon, false);
	}
	if (target_graphics_buffer_update(0, false)) {
		reset_drawing();
	}
}

/* set PAL/NTSC or custom timing variables */
static void init_beamcon0(bool fakehz)
{
	int isntsc, islace;
	int hpos = current_hpos();
	int oldmaxhpos = maxhpos;

	beamcon0 = new_beamcon0;

	isntsc = (beamcon0 & BEAMCON0_PAL) ? 0 : 1;
	islace = (interlace_seen) ? 1 : 0;
	if (!ecs_agnus) {
		isntsc = currprefs.ntscmode ? 1 : 0;
	}

	if (currprefs.cs_hvcsync == 0) {
		bemcon0_hsync_mask = BEAMCON0_VARHSYEN | BEAMCON0_VARCSYEN;
		bemcon0_vsync_mask = BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN;
	} else if (currprefs.cs_hvcsync == 1) {
		bemcon0_hsync_mask = BEAMCON0_VARCSYEN;
		bemcon0_vsync_mask = BEAMCON0_VARCSYEN;
	} else {
		bemcon0_hsync_mask = BEAMCON0_VARHSYEN;
		bemcon0_vsync_mask = BEAMCON0_VARVSYEN;
	}

#ifdef AMIBERRY // Don't change vblank_hz when opening P96 screens
	if (picasso_is_active(0)) {
		isntsc = currprefs.ntscmode ? 1 : 0;
	}
#endif
	float clk = (float)(currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL);
	int hardwired_vsstrt = 0;
	if (!isntsc) {
		maxvpos = MAXVPOS_PAL;
		maxhpos = MAXHPOS_PAL;
		minfirstline = VBLANK_ENDLINE_PAL;
		vblank_hz_nom = vblank_hz = VBLANK_HZ_PAL;
		hardwired_vbstop = VBLANK_STOP_PAL;
		hardwired_vbstrt = maxvpos;
		equ_vblank_endline = EQU_ENDLINE_PAL;
		equ_vblank_toggle = true;
		vblank_hz_shf = clk / ((maxvpos + 0.0f) * maxhpos);
		vblank_hz_lof = clk / ((maxvpos + 1.0f) * maxhpos);
		vblank_hz_lace = clk / ((maxvpos + 0.5f) * maxhpos);
	} else {
		maxvpos = MAXVPOS_NTSC;
		maxhpos = MAXHPOS_NTSC;
		minfirstline = VBLANK_ENDLINE_NTSC;
		vblank_hz_nom = vblank_hz = VBLANK_HZ_NTSC;
		hardwired_vbstop = VBLANK_STOP_NTSC;
		hardwired_vbstrt = maxvpos;
		equ_vblank_endline = EQU_ENDLINE_NTSC;
		equ_vblank_toggle = false;
		float half = (beamcon0 & BEAMCON0_LOLDIS) ? 0 : 0.5f;
		vblank_hz_shf = clk / ((maxvpos + 0.0f) * (maxhpos + half));
		vblank_hz_lof = clk / ((maxvpos + 1.0f) * (maxhpos + half));
		vblank_hz_lace = clk / ((maxvpos + 0.5f) * (maxhpos + half));
	}

	dmal_htotal_mask = 0xffff;
	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		// programmable scanrates (ECS Agnus)
		if (vtotal >= MAXVPOS) {
			vtotal = MAXVPOS - 1;
		}
		maxvpos = vtotal + 1;
		if (htotal >= MAXHPOS) {
			htotal = MAXHPOS - 1;
		}
		maxhpos = htotal + 1;

		if (maxhpos < DMAL_FIRST_HPOS) {
			dmal_htotal_mask = 0;
		} else 	if (maxhpos < DMAL_FIRST_HPOS + 6 * 2) {
			int cnt = (maxhpos - DMAL_FIRST_HPOS) / 2;
			dmal_htotal_mask = (1 << cnt) - 1;
		}
	}

	// after vsync, it seems earlier possible visible line is vsync+3.
	int vsync_startline = 3;
	if ((beamcon0 & BEAMCON0_VARVBEN) && (beamcon0 & bemcon0_vsync_mask)) {
		vsync_startline += vsstrt;
		if (vsync_startline >= maxvpos / 2) {
			vsync_startline = 3;
		}
	}

	maxvpos_nom = maxvpos;
	maxvpos_display = maxvpos;
	maxvpos_display_vsync = 1;
	if (agnusa1000) {
		// A1000 Agnus VBSTRT=first line, OCS and later: VBSTRT=last line
		maxvpos_display_vsync++;
		hardwired_vsstrt++;
	} else if (!ecs_agnus && !ecs_denise) {
		// OCS Agnus + OCS Denise: line 0 is visible
		maxvpos_display_vsync++;
	}

	vblank_firstline_hw = minfirstline;

	// calculate max possible display width in lores pixels
	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		// assume VGA-like monitor if VARBEAMEN
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
			maxhpos_display = maxhpos + 7;
			hsstop_detect = hsstrt * 2;
			if (hsstop_detect > maxhpos / 2 * 2 || hsstop_detect < 4 * 2) {
				hsstop_detect = 4 * 2;
			}
			minfirstline = 0;
		} else {
			int hp2 = maxhpos * 2;
			if (exthblank) {

				int hb = 1;
				int hbstrtx = (hbstrt & 0xff) * 2;
				int hbstopx = (hbstop & 0xff) * 2;
				if (aga_mode) {
					hbstrtx |= (hbstrt & 0x400) ? 1 : 0;
					hbstopx |= (hbstop & 0x400) ? 1 : 0;
				}
				hbstrtx = std::min(hbstrtx, hp2);
				hbstopx = std::min(hbstopx, hp2);
				if (hbstrtx > hp2 / 2 && hbstopx < hp2 / 2) {
					hb = (hp2 - hbstrtx) + hbstopx;
				} else if (hbstopx < hp2 / 2 && hbstrtx < hbstopx) {
					hb = hbstopx - hbstrtx;
				}
				if (hbstopx > hp2 / 2) {
					hbstopx = 0;
				}
				hb = std::max(hb, 1);

#if 0
				// HSYNC adjustment
				int hsz = 0;
				if (hsstrt > maxhpos / 2 && hsstop > hsstrt) {
					hsz = hsstop - hsstrt;
				} else if (hsstrt > maxhpos / 2 && hsstop < maxhpos / 2) {
					hsz = (maxhpos - hsstrt) - hsstop;
				} else if (hsstop < maxhpos / 2 && hsstrt < hsstop) {
					hsz = hsstop - hsstrt;
				}
#endif
				maxhpos_display = maxhpos - ((hb + 1) / 2);
				hsstop_detect = hbstopx - 1;

			} else {

				// hardwired
				int hbstrtx = 7 * 2;
				int hbstopx = 46 * 2;
				int hb = hbstopx - hbstrtx;
				maxhpos_display = maxhpos - (hb / 2);
				hsstop_detect = 46 * 2 - 2;
			}

			if (currprefs.gfx_overscanmode >= OVERSCANMODE_EXTREME) {
				int diff = (maxhpos - 2) - maxhpos_display;
				if (diff > 0) {
					hsstop_detect -= (diff / 2) * 2;
				}
				maxhpos_display = maxhpos - 2;
				maxvpos_display_vsync += 2;
			} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
				maxhpos_display += EXTRAWIDTH_BROADCAST;
				hsstop_detect--;
				maxvpos_display_vsync++;
			}

		}
		maxhpos_display *= 2;

	} else {

		maxhpos_display = AMIGA_WIDTH_MAX;
		hsstop_detect = hsstop_detect2;

		if (beamcon0 & bemcon0_hsync_mask) {
			if (currprefs.gfx_overscanmode < OVERSCANMODE_BROADCAST) {
				hsstop_detect += 7;
			} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
				hsstop_detect += 5;
			} else if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
				maxhpos_display += EXTRAWIDTH_ULTRA;
				maxvpos_display_vsync += vsync_startline;
				minfirstline = 0;
				hsstop_detect = hsyncstartpos_start_cycles * 2 - 4;
			}
		} else {

			if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
				maxhpos_display += EXTRAWIDTH_ULTRA;
				maxvpos_display_vsync += vsync_startline;
				hsstop_detect = 18 * 2;
				minfirstline = 1;
			}
		}

		if (currprefs.gfx_overscanmode < OVERSCANMODE_BROADCAST) {
			// one pixel row missing from right border if OCS
			if (!ecs_denise) {
				maxhpos_display--;
			}
		} else if (currprefs.gfx_overscanmode == OVERSCANMODE_EXTREME) {
			maxhpos_display += EXTRAWIDTH_EXTREME;
			maxvpos_display_vsync += 2;
			hsstop_detect -= 4;
			minfirstline--;
		} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
			maxhpos_display += EXTRAWIDTH_BROADCAST;
			maxvpos_display_vsync++;
			minfirstline--;
			hsstop_detect -= 1;
		}
	}

	if (currprefs.gfx_extrawidth > 0) {
		maxhpos_display += currprefs.gfx_extrawidth;
	}

	hsstop_detect = std::max(hsstop_detect, 0);
	minfirstline = std::max(minfirstline, 0);

	vblank_extraline = !agnusa1000 && !ecs_denise ? 1 : 0;

	int minfirstline_hw = minfirstline;
	if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
		minfirstline = 0;
		minfirstline_hw = 0;
	} else if (currprefs.gfx_overscanmode == OVERSCANMODE_EXTREME) {
		minfirstline_hw -= EXTRAHEIGHT_EXTREME / 2;
	} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
		minfirstline_hw -= EXTRAHEIGHT_BROADCAST_TOP;
	}
	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		minfirstline_hw = 0;
	}

	if (fakehz && vpos_count > maxvpos_display_vsync) {
		// we come here if vpos_count != maxvpos and beamcon0 didn't change
		// (someone poked VPOSW)
		vpos_count = std::max(vpos_count, 10);
		vblank_hz = (isntsc ? 15734.0f : 15625.0f) / vpos_count;
		vblank_hz_nom = vblank_hz_shf = vblank_hz_lof = vblank_hz_lace = (float)vblank_hz;
		maxvpos_nom = vpos_count - (lof_store ? 1 : 0);
		if ((maxvpos_nom >= 256 && maxvpos_nom <= 313) || (beamcon0 & BEAMCON0_VARBEAMEN)) {
			maxvpos_display = maxvpos_nom;
		} else if (maxvpos_nom < 256) {
			maxvpos_display = 255;
		} else {
			maxvpos_display = 313;
		}
		reset_drawing();
	} else if (vpos_count == 0) {
		// mode reset
		vpos_count = maxvpos;
		vpos_count_diff = maxvpos;
	}

	if ((beamcon0 & BEAMCON0_VARVBEN) && (beamcon0 & bemcon0_vsync_mask)) {
		minfirstline = vsync_startline;
		if (minfirstline > maxvpos / 2) {
			minfirstline = vsync_startline;
		}
		minfirstline++;
		firstblankedline = vbstrt;
		if (vsstrt > 0 && vsstrt < maxvpos / 2) {
			maxvpos_display_vsync += vsstrt - 1;
		}
		// if (weird mode where) vblank starts after vsync start+3: minfirstline = vsstrt+3
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_EXTREME && firstblankedline >= vsstrt + 3 && minfirstline > vsstrt + 3 && firstblankedline < minfirstline) {
			minfirstline = vsstrt + 3;
			minfirstline_hw = std::min(minfirstline_hw, minfirstline);
		}
	} else if (beamcon0 & bemcon0_vsync_mask) {
		firstblankedline = maxvpos + 1;
	} else if (beamcon0 & BEAMCON0_VARVBEN) {
		if (minfirstline > vbstop) {
			minfirstline = vbstop;
			minfirstline = std::max(minfirstline, 3);
		}
		firstblankedline = vbstrt;
		// if (weird mode where) vblank starts after vsync start+3: minfirstline = vsstrt+3
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_EXTREME && firstblankedline >= hardwired_vsstrt + 3 && minfirstline > hardwired_vsstrt + 3 && firstblankedline < minfirstline) {
			minfirstline = hardwired_vsstrt + 3;
			minfirstline_hw = std::min(minfirstline_hw, minfirstline);
		}
	} else {
		firstblankedline = maxvpos + 1;
	}
	if (beamcon0 & (BEAMCON0_VARVBEN | bemcon0_vsync_mask)) {
		programmedmode = 2;
	}

	int eh = currprefs.gfx_extraheight;
	if (eh > 0) {
		if (beamcon0 & bemcon0_vsync_mask) {
			maxvpos_display_vsync += eh / 2;
			minfirstline -= eh / 2;
		} else {
			maxvpos_display_vsync = 3;
			minfirstline -= eh / 2;
		}
	}

	maxvpos_display_vsync = std::max(maxvpos_display_vsync, 0);

	minfirstline = std::max(minfirstline, vsync_startline);

	if (minfirstline >= maxvpos) {
		minfirstline = maxvpos - 1;
	}

	if (firstblankedline < minfirstline) {
		firstblankedline = maxvpos + maxvpos_display_vsync + 1;
	}

	minfirstline = std::max(minfirstline, minfirstline_hw);

	if (beamcon0 & bemcon0_vsync_mask) {
		maxvpos_display_vsync = std::min(maxvpos_display_vsync, vsstrt + 3);
		minfirstline = std::max(minfirstline, vsync_startline);
	}

	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		float half = (beamcon0 & BEAMCON0_PAL) ? 0 : ((beamcon0 & BEAMCON0_LOLDIS) ? 0 : 0.5f);
		vblank_hz_nom = vblank_hz = clk / (maxvpos * (maxhpos + half));
		vblank_hz_shf = vblank_hz;
		vblank_hz_lof = clk / ((maxvpos + 1.0f) * (maxhpos + half));
		vblank_hz_lace = clk / ((maxvpos + 0.5f) * (maxhpos + half));

		maxvpos_nom = maxvpos;
		maxvpos_display = maxvpos;
		equ_vblank_endline = -1;

		programmedmode = 2;
		if ((htotal < 226 || htotal > 229) || (vtotal < 256 || vtotal > 320)) {
			doublescan = htotal <= 164 && vtotal >= 350 ? 1 : 0;
			// if superhires and wide enough: not doublescan
			if (doublescan && htotal >= 140 && (bplcon0 & 0x0040))
				doublescan = 0;
			programmedmode = 1;
		}

		vpos_count = maxvpos_nom;
		vpos_count_diff = maxvpos_nom;
	}

	// exclude possible extra lines that are inside vertical blank
	if (currprefs.gfx_overscanmode < OVERSCANMODE_EXTREME) {
		if (beamcon0 & BEAMCON0_VARBEAMEN) {
			if (firstblankedline < maxvpos / 2 && maxvpos_display_vsync > firstblankedline) {
				maxvpos_display_vsync = firstblankedline;
			} else if (firstblankedline <= maxvpos) {
				maxvpos_display_vsync = 1;
			}
		}
	}

	if (maxvpos_display_vsync <= 0) {
		maxvpos_display_vsync = 1;
	}

	maxvpos_nom = std::min(maxvpos_nom, MAXVPOS);
	maxvpos_display = std::min(maxvpos_display, MAXVPOS);
	if (currprefs.gfx_scandoubler && doublescan == 0) {
		doublescan = -1;
	}
	/* limit to sane values */
	vblank_hz = std::max<float>(vblank_hz, 10);
	vblank_hz = std::min<float>(vblank_hz, 300);
	maxhpos_short = maxhpos;
	minfirstline_linear = minfirstline;
	set_delay_lastcycle();
	updateextblk();
	maxvpos_total = ecs_agnus ? (MAXVPOS_LINES_ECS - 1) : (MAXVPOS_LINES_OCS - 1);
	maxvpos_total = std::min(maxvpos_total, MAXVPOS);
	set_maxhpos(maxhpos);
	estimated_fm = 0xffff;
	maxvsize_display = maxvpos_display + maxvpos_display_vsync - minfirstline;

	if ((currprefs.m68k_speed == 0 || copper_access) && maxhpos != oldmaxhpos) {
		if (hpos >= maxhpos) {
			// count until 255 -> 0 wrap around
			eventtab[ev_hsync].evtime = line_start_cycles + 256 * CYCLE_UNIT;
			// mark current line lenght = 0x100
			set_maxhpos(0x100);
		} else {
			// adjust current scanline length to match new maxhpos
			eventtab[ev_hsync].evtime = line_start_cycles + maxhpos * CYCLE_UNIT;
		}
		current_hpos();
		events_schedule();
	}
}

static void init_hz(bool checkvposw)
{
	int isntsc, islace;
	int odbl = doublescan;
	double ovblank = vblank_hz;
	int hzc = 0;
	int omaxhpos = maxhpos;
	int omaxvpos = maxvpos;

	if (!checkvposw) {
		vpos_count = 0;
	}

	isntsc = (new_beamcon0 & BEAMCON0_PAL) ? 0 : 1;
	islace = (interlace_seen) ? 1 : 0;
	if (!ecs_agnus) {
		isntsc = currprefs.ntscmode ? 1 : 0;
	}

	vpos_count_diff = vpos_count;

	doublescan = 0;
	programmedmode = 0;
	if ((beamcon0 & (BEAMCON0_VARBEAMEN | BEAMCON0_PAL)) != (new_beamcon0 & (BEAMCON0_VARBEAMEN | BEAMCON0_PAL))) {
		hzc = 1;
	}
	if (beamcon0 != new_beamcon0) {
		vpos_count_diff = vpos_count = 0;
	}
	if ((beamcon0 & (BEAMCON0_VARVBEN | BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN)) != (new_beamcon0 & (BEAMCON0_VARVBEN | BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN))) {
		hzc = 1;
	}

	init_beamcon0(checkvposw);

	if (doublescan != odbl || maxvpos != omaxvpos) {
		hzc = 1;
	}

	eventtab[ev_hsync].oldcycles = get_cycles();
	eventtab[ev_hsync].evtime = get_cycles() + HSYNCTIME;
	events_schedule();
	if (hzc) {
		interlace_seen = islace;
		reset_drawing();
	}

	if (maxvpos != omaxvpos || maxhpos != omaxhpos) {
		nosignal_trigger = true;
	}

#ifdef PICASSO96
	if (!p96refresh_active) {
		maxvpos_stored = maxvpos;
		maxhpos_stored = maxhpos;
		vblank_hz_stored = vblank_hz;
	}
#endif

	compute_framesync();
	devices_syncchange();

#ifdef PICASSO96
	init_hz_p96(0);
#endif
	if (vblank_hz != ovblank) {
		updatedisplayarea(0);
	}
	inputdevice_tablet_strobe();

	if (varsync_changed > 0) {
		varsync_changed = 0;
		dumpsync();
	}
}

static void init_hz_vposw(void)
{
	init_hz(true);
}

void init_hz_normal(void)
{
	init_hz(false);
}

static void calcdiw(void)
{
	int hstrt = (diwstrt & 0xFF) << 2;
	int hstop = (diwstop & 0xFF) << 2;
	int vstrt = diwstrt >> 8;
	int vstop = diwstop >> 8;

	// ECS Agnus/AGA: DIWHIGH vertical high bits.
	if (diwhigh_written && ecs_agnus) {
		if (aga_mode) {
			vstrt |= (diwhigh & 7) << 8;
			vstop |= ((diwhigh >> 8) & 7) << 8;
		} else {
			// ECS Agnus has undocumented V11!
			vstrt |= (diwhigh & 15) << 8;
			vstop |= ((diwhigh >> 8) & 15) << 8;
		}
	} else {
		if ((vstop & 0x80) == 0)
			vstop |= 0x100;
	}
	// ECS Denise/AGA: horizontal DIWHIGH high bit.
	if (diwhigh_written && ecs_denise) {
		hstrt |= ((diwhigh >> 5) & 1) << (8 + 2);
		hstop |= ((diwhigh >> 13) & 1) << (8 + 2);
	} else {
		hstop |= 0x100 << 2;
	}
	// AGA only: horizontal DIWHIGH hires/shres bits.
	if (diwhigh_written && aga_mode) {
		hstrt |= (diwhigh >> 3) & 3;
		hstop |= (diwhigh >> 11) & 3;
	}

	diw_hstrt = hstrt;
	diw_hstop = hstop;

	diwfirstword = coord_diw_shres_to_window_x(hstrt);
	diwlastword = coord_diw_shres_to_window_x(hstop);
	int mindiw = min_diwlastword;
	diwfirstword = std::max(diwfirstword, mindiw);

	int hpos = current_hpos();

	if (vstrt == vpos && vstop != vpos && vdiwstate == diw_states::DIW_waiting_start) {
		// This may start BPL DMA immediately.
		SET_LINE_CYCLEBASED(hpos);
	}

	plffirstline = vstrt;
	plflastline = vstop;

	plfstrt = ddfstrt;
	plfstop = ddfstop;
	if (!ecs_agnus) {
		plfstrt &= 0x00fc;
		plfstop &= 0x00fc;
	}

	diw_change = 2;

	decide_vline(hpos);
}

/* display mode changed (lores, doubling etc..), recalculate everything */
void init_custom(void)
{
	check_nocustom();
	update_mirrors();
	create_cycle_diagram_table();
	reset_drawing();
	init_hz_normal();
	calcdiw();
	update_toscr_vars();
	compute_shifter_mask();
	compute_toscr_delay(bplcon1);
	set_delay_lastcycle();
}

static int timehack_alive = 0;

static uae_u32 REGPARAM2 timehack_helper(TrapContext *context)
{
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tv;
	if (m68k_dreg(regs, 0) == 0) {
		return timehack_alive;
	}

	timehack_alive = 10;

	gettimeofday(&tv, NULL);
	put_long(m68k_areg(regs, 0), tv.tv_sec - (((365 * 8 + 2) * 24) * 60 * 60));
	put_long(m68k_areg(regs, 0) + 4, tv.tv_usec);
	return 0;
#else
	return 2;
#endif
}

/*
* register functions
*/
static uae_u16 DENISEID(int *missing)
{
	*missing = 0;
	if (currprefs.cs_deniserev >= 0) {
		return currprefs.cs_deniserev;
	}
#ifdef AGA
	if (aga_mode) {
		if (currprefs.cs_ide == IDE_A4000) {
			return 0xFCF8;
		}
		return 0x00F8;
	}
#endif
	if (ecs_denise) {
		return 0xFFFC;
	}
	if (currprefs.cpu_model == 68000 && (currprefs.cpu_compatible || currprefs.cpu_memory_cycle_exact)) {
		*missing = 1;
	}
	return 0xFFFF;
}

static bool blit_busy(int hpos, bool dmaconr)
{
	// DMACONR latch load takes 1 cycle. Copper sees it immediately.
	if (dmaconr && blt_info.finishhpos == hpos) {
		return true;
	}
	if (!blt_info.blit_main && !blt_info.blit_finald) {
		return false;
	}
	// AGA apparently fixes both bugs.
	if (agnusa1000) {
		// Blitter busy bug: A1000 Agnus only sets busy-bit when blitter gets first DMA slot.
		if (!blt_info.got_cycle) {
			return false;
		}
		if (blt_info.blit_pending) {
			return true;
		}
		// Blitter is considered finished even if last D has not yet been written
		if (!blt_info.blit_main) {
			return false;
		}
	} else if (!aga_mode) {
		if (blt_info.blit_pending) {
			return true;
		}
		// Blitter is considered finished even if last D has not yet been written
		if (!blt_info.blit_main) {
			return false;
		}
	}
	return true;
}

STATIC_INLINE uae_u16 DMACONR(int hpos)
{
	decide_line(hpos);
	decide_fetch_safe(hpos);
	dmacon &= ~(0x4000 | 0x2000);
	dmacon |= (blit_busy(hpos, true) ? 0x4000 : 0x0000) | (blt_info.blitzero ? 0x2000 : 0);
	return dmacon;
}
STATIC_INLINE uae_u16 INTENAR(void)
{
	return intena & 0x7fff;
}
uae_u16 INTREQR(void)
{
	return intreq & 0x7fff;
}
STATIC_INLINE uae_u16 ADKCONR(void)
{
	return adkcon & 0x7fff;
}

STATIC_INLINE int islightpentriggered(void)
{
	if (beamcon0 & BEAMCON0_LPENDIS)
		return 0;
	return lightpen_triggered != 0;
}
STATIC_INLINE int issyncstopped(void)
{
	return (bplcon0 & 2) && (!currprefs.genlock || currprefs.genlock_effects);
}

STATIC_INLINE int GETVPOS(void)
{
	return islightpentriggered() ? vpos_lpen : (issyncstopped() ? vpos_previous : vpos);
}
STATIC_INLINE int GETHPOS(void)
{
	return islightpentriggered() ? hpos_lpen : (issyncstopped() ? hpos_previous : current_hpos());
}

// fake changing hpos when rom genlock test runs and genlock is connected
static bool hsyncdelay(void)
{
	if (!currprefs.genlock || currprefs.genlock_effects) {
		return false;
	}
	if (currprefs.cpu_memory_cycle_exact || currprefs.m68k_speed >= 0) {
		return false;
	}
	if (bplcon0 == (0x0100 | 0x0002)) {
		return true;
	}
	return false;
}

#define CPU_ACCURATE (currprefs.cpu_model < 68020 || (currprefs.cpu_model == 68020 && currprefs.cpu_memory_cycle_exact))

static void check_line_enabled(void)
{
	line_disabled &= ~3;
	line_disabled |= line_hidden() ? 1 : 0;
	line_disabled |= custom_disabled ? 2 : 0;
}

void get_mode_blanking_limits(int *phbstop, int *phbstrt, int *pvbstop, int *pvbstrt)
{
	if (new_beamcon0 & BEAMCON0_VARVBEN) {
		*pvbstop = vbstop;
		*pvbstrt = vbstrt;
		*phbstop = hbstop_v;
		*phbstrt = hbstrt_v;
	} else {
		*pvbstop = hardwired_vbstop;
		*pvbstrt = hardwired_vbstrt;
		*phbstop = (47 << CCK_SHRES_SHIFT) - 7;
		*phbstrt = ((maxhpos_short + 8) << CCK_SHRES_SHIFT) - 3;
	}
}

static void vb_check(void)
{
	check_line_enabled();

	// A1000 Agnus VBSTRT=first line, OCS and later: VBSTRT=last line
	if (agnusa1000) {
		if (vpos == 0) {
			vb_start_line = 1;
			vb_state = true;
		}
	} else {
		if (vpos == hardwired_vbstrt + lof_store - 1) {
			vb_start_line = 1;
			vb_state = true;
		}
	}
	if (vpos == hardwired_vbstop - 1) {
		vb_end_line = true;
		vb_state = false;
	}
	if (vpos == hardwired_vbstop) {
		vb_end_line = false;
		vb_state = false;
		vb_start_line = 0;
		vb_end_next_line = true;
	}
}

static void vhpos_adj(uae_u16 *hpp, uae_u16 *vpp)
{
	uae_u16 hp = *hpp;
	uae_u16 vp = *vpp;

	hp++;
	if (hp == maxhposm0) {
		hp = 0;
	} else if (hp == 1) {
		// HP=0-1: VP = previous line.
		vp = vpos_prev;
	}

	*hpp = hp;
	*vpp = vp;
}

static uae_u16 VPOSR(void)
{
	uae_u16 csbit = 0;
	uae_u16 vp = GETVPOS();
	uae_u16 hp = GETHPOS();
	int lofr = lof_store;
	int lolr = lol;

	if (hp == 0) {
		// LOF and LOL toggles when HPOS=1
		// Return pre-toggled value if VPOSR is read when HPOS=0
		if (vp == 0) {
			if ((bplcon0 & 4) && CPU_ACCURATE) {
				lofr = lofr ? 0 : 1;
			}
		}
		if (linetoggle) {
			lolr = lolr ? 0 : 1;
		}
	}
	vhpos_adj(&hp, &vp);

	vp = (vp >> 8) & 7;

	if (currprefs.cs_agnusrev >= 0) {
		csbit |= currprefs.cs_agnusrev << 8;
	} else {
#ifdef AGA
		csbit |= aga_mode ? 0x2300 : 0;
#endif
		csbit |= ecs_agnus ? 0x2000 : 0;
#if 0 /* apparently "8372 (Fat-hr) (agnushr),rev 5" does not exist */
		if (currprefs.chipmem_size > 1024 * 1024 && (currprefs.chipset_mask & CSMASK_ECS_AGNUS))
			csbit |= 0x2100;
#endif
		if (currprefs.ntscmode) {
			csbit |= 0x1000;
		}
	}

	if (!ecs_agnus) {
		vp &= 1;
	} else {
		vp |= lolr ? 0x80 : 0;
	}
	vp |= (lofr ? 0x8000 : 0) | csbit;
	hsyncdelay();
#if 0
	if (1 || (M68K_GETPC < 0x00f00000 || M68K_GETPC >= 0x10000000))
		write_log (_T("VPOSR %04x at %08x\n"), vp, M68K_GETPC);
#endif
	return vp;
}

static void VPOSW(int hpos, uae_u16 v)
{
	int oldvpos = vpos;
	int newvpos = vpos;

#if 0
	if (M68K_GETPC < 0xf00000 || 1)
		write_log (_T("VPOSW %04X PC=%08x\n"), v, M68K_GETPC);
#endif
	if (lof_store != ((v & 0x8000) ? 1 : 0)) {
		lof_store = (v & 0x8000) ? 1 : 0;
		lof_changing = lof_store ? 1 : -1;
	}
	if (ecs_agnus) {
		// LOL is always reset when VPOSW is written to.
		lol = 0;
	}
	if (lof_changing) {
		return;
	}
	newvpos &= 0x00ff;
	v &= 7;
	if (!ecs_agnus) {
		v &= 1;
	}
	newvpos |= v << 8;

	if (newvpos != oldvpos) {
		decide_line(hpos);
		decide_fetch_safe(hpos);
		cia_adjust_eclock_phase((newvpos - oldvpos) * maxhpos);
		vposw_change++;
		vpos = newvpos;
		vb_check();
		decide_vline(hpos);
		SET_LINE_CYCLEBASED(hpos);
	}
}

static void VHPOSW_delayed(uae_u32 v)
{
	int oldvpos = vpos;
	int newvpos = vpos;
	int hpos_org = current_hpos();
	int hnew = hpos_org;
	bool enabled = false;

	decide_vline(hpos_org);
	decide_hdiw(hpos_org);
	decide_line(hpos_org);
	decide_fetch_safe(hpos_org);
	decide_sprites(hpos_org);
	flush_display(fetchmode);

#if 0
	if (M68K_GETPC < 0xf00000 || 1)
		write_log (_T("VHPOSW %04X PC=%08x\n"), v, M68K_GETPC);
#endif

	bool cpu_accurate = currprefs.m68k_speed >= 0 && !currprefs.cachesize && currprefs.cpu_memory_cycle_exact;

	if (cpu_accurate || copper_access) {
		enabled = true;
		int hpos = hpos_org;
		hnew = (v & 0xff);
		int hnew_org = hnew;
		bool newinc = false;
		if (hpos == 0 || hpos == 1) {
			hpos += maxhpos;
		}
		if (hnew == 0 || hnew == 1) {
			hnew += maxhpos;
			newinc = true;
		}
		int hdiff = hnew - hpos;

		//write_log("%02x %02x %d\n", hpos_org, hnew_org, hdiff);

		if (hdiff <= -maxhpos / 2 || hdiff >= maxhpos / 2) {
			hdiff = 0;
		}
		if (copper_access && (hdiff & 1)) {
			write_log("VHPOSW write %04X. New horizontal value is odd. Copper confusion possible.\n", v);
		}

		if (hdiff) {
			bool fail = false;
			if (hdiff & 1) {
				vhposr_delay_offset = 1;
			}

			evt_t hsync_evtime = eventtab[ev_hsync].evtime;
			evt_t hsync_oldcycles = eventtab[ev_hsync].oldcycles;
			evt_t hsynch_evtime = eventtab[ev_hsynch].evtime;
			evt_t hsynch_oldcycles = eventtab[ev_hsynch].oldcycles;

			set_maxhpos(maxhpos);
			if (newinc && hnew == maxhpos + 1) {
				// 0000 -> 0001 (0 and 1 are part of previous line, vpos increases when hpos=1). No need to do anything
			} else if (hnew_org == 0 && hpos_org > 1) {
				fail = true;
			} else if (hpos < maxhpos && hnew >= maxhpos) {
				// maxhpos check skip: counter counts until it wraps around 0xFF->0x00
				int hdiff2 = (0x100 - hnew) - (maxhpos - hpos);
				hdiff2 *= CYCLE_UNIT;
				eventtab[ev_hsync].evtime += hdiff2;
				eventtab[ev_hsync].oldcycles = get_cycles() - hnew * CYCLE_UNIT;
				eventtab[ev_hsynch].evtime += hdiff2;
				eventtab[ev_hsynch].oldcycles += hdiff2;
				// mark current line lenght = 0x100
				set_maxhpos(0x100);
			} else if (hdiff) {
				hdiff = -hdiff;
				hdiff *= CYCLE_UNIT;
				eventtab[ev_hsync].evtime += hdiff;
				eventtab[ev_hsync].oldcycles += hdiff;
				eventtab[ev_hsynch].evtime += hdiff;
				eventtab[ev_hsynch].oldcycles += hdiff;
			}

			int hpos2 = current_hpos_safe();
			if (hpos2 < 0 || hpos2 > 255 || fail) {
				eventtab[ev_hsync].evtime = hsync_evtime;
				eventtab[ev_hsync].oldcycles = hsync_oldcycles;
				eventtab[ev_hsynch].evtime = hsynch_evtime;
				eventtab[ev_hsynch].oldcycles = hsynch_oldcycles;
				hdiff = 0;
				hpos_org = -1;
				set_maxhpos(maxhpos);
			}

			events_schedule();

		#ifdef DEBUGGER
			if (newvpos == oldvpos && hdiff) {
				record_dma_reoffset(vpos, hpos, hnew);
			}
		#endif

			if (hdiff) {
				int hold = hpos;
				memset(cycle_line_slot + MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST, 0, sizeof(uae_u8) * MAX_CHIPSETSLOTS_EXTRA);
				memset(cycle_line_pipe + MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST, 0, sizeof(uae_u16) * MAX_CHIPSETSLOTS_EXTRA);
				memset(blitter_pipe + MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST, 0, sizeof(uae_u16) * MAX_CHIPSETSLOTS_EXTRA);
				int total = (MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST + MAX_CHIPSETSLOTS_EXTRA) - (hnew > hold ? hnew : hold);
				if (total > 0) {
					memmove(cycle_line_slot + hnew, cycle_line_slot + hold, total * sizeof(uae_u8));
					memmove(cycle_line_pipe + hnew, cycle_line_pipe + hold, total * sizeof(uae_u16));
					memmove(blitter_pipe + hnew, blitter_pipe + hold, total * sizeof(uae_u16));
					if (hnew > hold) {
						int t = hnew - hold;
						memset(cycle_line_slot, 0, t * sizeof(uae_u8));
						memset(cycle_line_pipe, 0, t * sizeof(uae_u16));
						memset(blitter_pipe, 0, t * sizeof(uae_u16));
					}
				}
				int nhp = hnew + 1;
				set_blitter_last(nhp);
				last_copper_hpos = nhp;
				last_fetch_hpos = nhp;
				last_decide_line_hpos = nhp;
				last_decide_sprite_hpos = nhp;
			}
		}

		delay_cycles = ((hnew + 0) & 0xff) << CCK_SHRES_SHIFT;

		last_diw_hpos = hnew << 1;
		hdiw_denisecounter = ((hnew + 0) & 0xff) << CCK_SHRES_SHIFT;
	}

	newvpos &= 0xff00;
	newvpos |= v >> 8;
	if (enabled && (hpos_org == 0 || hpos_org == 1)) {
		newvpos++;
	}
	if (newvpos > vpos || (newvpos <= maxvpos && vpos > maxvpos) || cpu_accurate || copper_access) {
		if (newvpos != oldvpos) {
			vposw_change++;
#ifdef DEBUGGER
			if (hpos_org >= 0) {
				record_dma_hsync(hpos_org);
				if (debug_dma) {
					int vp = vpos;
					vpos = newvpos;
					record_dma_hsync(maxhpos);
					vpos = vp;
				}
			}
#endif
		}
		vpos = newvpos;
#ifdef DEBUGGER
		record_dma_denise(hnew, hdiw_denisecounter >> 2);
#endif
		vb_check();
		decide_vline(hnew);
		vhposw_modified = true;
	}
}

static void VHPOSW_delayed1(uae_u32 v)
{
	int hpos = current_hpos();
	decide_hdiw(hpos);
	decide_line(hpos);
	decide_fetch_safe(hpos);
	decide_sprites(hpos, false, true);

	int hnew = v & 0xff;
	last_sprite_hpos = (hnew << 1) - 1;
	last_sprite_point = (((hnew + 0) & 0xff) << 1) + 0;

	event2_newevent_xx(-1, 1 * CYCLE_UNIT, v, VHPOSW_delayed);
}

static void VHPOSW(uae_u16 v)
{
	event2_newevent_xx(-1, 1 * CYCLE_UNIT, v, VHPOSW_delayed1);
}

static uae_u16 VHPOSR(void)
{
	static uae_u16 oldhp;
	uae_u16 vp = GETVPOS();
	uae_u16 hp = GETHPOS();

	vhpos_adj(&hp, &vp);

	vp <<= 8;

	if (hsyncdelay()) {
		// fake continuously changing hpos in fastest possible modes
		hp = oldhp % maxhpos;
		oldhp++;
	}

	vp |= hp;

#if 0
	if (M68K_GETPC < 0x00f00000 || M68K_GETPC >= 0x10000000)
		write_log (_T("VPOS %04x %04x at %08x\n"), VPOSR (), vp, M68K_GETPC);
#endif
	return vp;
}

static uae_u16 HHPOSR(void)
{
	uae_u16 v;
	if (islightpentriggered()) {
		v = hhpos_lpen;
	} else {
		uae_u16 max = (new_beamcon0 & BEAMCON0_DUAL) ? htotal : maxhpos - 1;
		v = hhpos + current_hpos() - hhpos_hpos;
		if (hhpos <= max || v >= 0x100) {
			if (max) {
				v %= max;
			} else {
				v = 0;
			}
		}
	}
	v &= 0xff;
	return v;
}

static void HHPOS(int hpos, uae_u16 v)
{
	hhpos = v & (MAXHPOS_ROWS - 1);
	hhpos_hpos = hpos;
}

static void SPRHSTRT(int hpos, uae_u16 v)
{
	sprhstrt = v;
	sprhstrt_v = v & (MAXVPOS_LINES_ECS - 1);
}
static void SPRHSTOP(int hpos, uae_u16 v)
{
	sprhstop = v;
	sprhstop_v = v & (MAXVPOS_LINES_ECS - 1);
}
static void BPLHSTRT(int hpos, uae_u16 v)
{
	bplhstrt = v;
	bplhstrt_v = v & (MAXVPOS_LINES_ECS - 1);
}
static void BPLHSTOP(int hpos, uae_u16 v)
{
	bplhstop = v;
	bplhstop_v = v & (MAXVPOS_LINES_ECS - 1);
}

/*

	REFPTR bit mapping to hidden refresh DMA pointer register.

	OCS Agnus (RAS/CAS 9/9, 8-bit ROR refresh):

	B  RAS CAS
	0: 000 080
	1: 101 000
	2: 002 100
	3: 004 000
	4: 008 000
	5: 010 000
	6: 020 000
	7: 040 000
	8: 080 000
	9: 000 001
	A: 000 002
	B: 000 004
	C: 000 008
	D: 000 010
	E: 000 020
	F: 000 040

	ECS Agnus 1M (RAS/CAS 9/9, 9-bit ROR refresh):

	B  RAS CAS
	0: 080 000
	1: 100 001
	2: 000 102
	3: 000 004
	4: 000 008
	5: 000 010
	6: 000 020
	7: 000 040
	8: 000 080
	9: 001 000
	A: 002 000
	B: 004 000
	C: 008 000
	D: 010 000
	E: 020 000
	F: 040 000

	ECS Agnus 2M (RAS/CAS 10/10, 9-bit ROR refresh):

	B  RAS CAS
	0: 080 000
	1: 100 001
	2: 000 102
	3: 200 004
	4: 000 208
	5: 000 010
	6: 000 020
	7: 000 040
	8: 000 080
	9: 001 000
	A: 002 000
	B: 004 000
	C: 008 000
	D: 010 000
	E: 020 000
	F: 040 000

	AGA (RAS/CAS 10/10, CBR refresh)

	0: 040/000
	1: 080/001
	2: 100/002
	3: 200/004
	4: 001/008
	5: 000/010
	6: 000/020
	7: 000/040
	8: 000/080
	9: 000/100
	A: 000/200
	B: 002/000
	C: 004/000
	D: 008/000
	E: 010/000
	F: 020/000

*/

static void REFPTR(int hpos, uae_u16 v)
{
	int diff = hpos - REFRESH_FIRST_HPOS;
	// write exactly 1 cycle before refresh slot: write ignored.
	if (diff == -1 || diff == 1 || diff == 3 || diff == 5) {
		return;
	}
	int slot = diff / 2;
	if (slot >= 0) {
		slot = std::min(slot, 4);
		update_refptr(-1, slot, true, true);
	}

	if (aga_mode) {
		refptr = 0;
		if (v & (1 << 0)) {
			refptr |= 0x020000;
		}
		if (v & (1 << 1)) {
			refptr |= 0x040000 | 0x000001;
		}
		if (v & (1 << 2)) {
			refptr |= 0x000100 | 0x000002;
		}
		if (v & (1 << 3)) {
			refptr |= 0x080000 | 0x000004;
		}
		if (v & (1 << 4)) {
			refptr |= 0x000200 | 0x000008;
		}
		if (v & (1 << 5)) {
			refptr |= 0x000010;
		}
		if (v & (1 << 6)) {
			refptr |= 0x000020;
		}
		if (v & (1 << 7)) {
			refptr |= 0x000040;
		}
		if (v & (1 << 8)) {
			refptr |= 0x000080;
		}
		if (v & (1 << 9)) {
			refptr |= 0x000400;
		}
		if (v & (1 << 10)) {
			refptr |= 0x000800;
		}
		if (v & (1 << 11)) {
			refptr |= 0x001000;
		}
		if (v & (1 << 12)) {
			refptr |= 0x002000;
		}
		if (v & (1 << 13)) {
			refptr |= 0x004000;
		}
		if (v & (1 << 14)) {
			refptr |= 0x008000;
		}
		if (v & (1 << 15)) {
			refptr |= 0x010000;
		}
		refptr <<= 1;
	} else if (ecs_agnus) {
		refptr = v & 0xfffe;
		if (v & 1) {
			refptr |= 0x10000;
		}
		if (v & 2) {
			refptr |= 0x20000;
		}
		if (v & 4) {
			refptr |= 0x40000;
		}
		if (v & 8) {
			refptr |= 0x80000;
		}
		if (currprefs.chipmem.size > 0x100000) {
			if (v & 16) {
				refptr |= 0x100000;
			}
		}
	} else {
		refptr = v & 0xfffe;
		if (v & 1) {
			refptr |= 0x10000;
		}
		if (v & 2) {
			refptr |= 0x20000;
		}
		if (v & 4) {
			refptr |= 0x40000;
		}
	}
#if 0
	int ras, cas;
	bool x = get_ras_cas(refptr, &ras, &cas);
	write_log("%04x %08x %c %03x %03X\n", v, refptr, x ? '+' : ' ', ras, cas);
#endif
}

static int test_copper_dangerous(uaecptr address)
{
	int addr = address & 0x01fe;
	if (addr < ((copcon & 2) ? (ecs_agnus ? 0 : 0x40) : 0x80)) {
		cop_state.state = COP_stop;
		copper_enabled_thisline = 0;
		unset_special (SPCFLAG_COPPER);
		return 1;
	}
	return 0;
}

// if DMA was changed during same cycle: previous value is used
static bool is_blitter_dma(void)
{
	bool dma = dmaen(DMA_BLITTER);
	if (get_cycles() == blitter_dma_change_cycle) {
		return dma == false;
	}
	return dma;
}
static bool is_copper_dma(bool checksame)
{
	bool dma = dmaen(DMA_COPPER);
	if (checksame && get_cycles() == copper_dma_change_cycle) {
		return dma == false;
	}
	return dma;
}

static void immediate_copper(int num)
{
	int pos = 0;
	int oldpos = 0;

	cop_state.state = COP_stop;
	cop_state.ip = num == 1 ? cop1lc : cop2lc;

	while (pos < (maxvpos << 5)) {
		pos = std::max(oldpos, pos);
		if (!is_copper_dma(false)) {
			break;
		}
		if (cop_state.ip >= currprefs.chipmem.size &&
			(cop_state.ip < currprefs.z3chipmem.start_address || cop_state.ip >= currprefs.z3chipmem.start_address + currprefs.z3chipmem.size) &&
			(cop_state.ip < debugmem_bank.start || cop_state.ip >= debugmem_bank.start + debugmem_bank.allocated_size))
			break;
		pos++;
		oldpos = pos;
		cop_state.ir[0] = chipmem_wget_indirect (cop_state.ip);
		cop_state.ir[1] = chipmem_wget_indirect (cop_state.ip + 2);
		cop_state.ip += 4;
		if (!(cop_state.ir[0] & 1)) { // move
			cop_state.ir[0] &= 0x1fe;
			if (cop_state.ir[0] == 0x88) {
				cop_state.ip = cop1lc;
				continue;
			}
			if (cop_state.ir[0] == 0x8a) {
				cop_state.ip = cop2lc;
				continue;
			}
			if (test_copper_dangerous(cop_state.ir[0])) {
				break;
			}
			custom_wput_1 (0, cop_state.ir[0], cop_state.ir[1], 0);
		} else { // wait or skip
			if ((cop_state.ir[0] >> 8) > ((pos >> 5) & 0xff))
				pos = (((pos >> 5) & 0x100) | ((cop_state.ir[0] >> 8)) << 5) | ((cop_state.ir[0] & 0xff) >> 3);
			if (cop_state.ir[0] >= 0xffdf && cop_state.ir[1] == 0xfffe) {
				break;
			}
		}
	}
	cop_state.state = COP_stop;
	unset_special (SPCFLAG_COPPER);
}

STATIC_INLINE void COP1LCH(uae_u16 v)
{
	cop1lc = (cop1lc & 0xffff) | ((uae_u32)v << 16);
}
STATIC_INLINE void COP1LCL(uae_u16 v)
{
	cop1lc = (cop1lc & ~0xffff) | (v & 0xfffe);
}
STATIC_INLINE void COP2LCH(uae_u16 v)
{
	cop2lc = (cop2lc & 0xffff) | ((uae_u32)v << 16);
}
STATIC_INLINE void COP2LCL(uae_u16 v)
{
	cop2lc = (cop2lc & ~0xffff) | (v & 0xfffe);
}

static void compute_spcflag_copper(void);

// normal COPJMP write: takes 2 more cycles
static void COPJMP(int num, int vblank)
{
	unset_special(SPCFLAG_COPPER);
	cop_state.ignore_next = 0;

	if (!cop_state.strobe) {
		cop_state.state_prev = cop_state.state;
	}
	if ((cop_state.state == COP_wait1 || cop_state.state == COP_waitforever) && !vblank && is_copper_dma(false)) {
		if (blt_info.blit_main) {
			static int warned = 100;
			if (warned > 0) {
				write_log(_T("Potential buggy copper cycle conflict with blitter PC=%08x, COP=%08x\n"), M68K_GETPC, cop_state.ip);
				warned--;
			}
		}
		int hp = current_hpos();
		if ((hp & 1) && safecpu()) {
			// CPU unaligned COPJMP while waiting
			cop_state.state = COP_strobe_delay1x;
			copper_bad_cycle_start = get_cycles();
		} else {
			cop_state.state = COP_strobe_delay1;
		}
	} else {
		if (vblank) {
			cop_state.state = COP_strobe_delay2;
			switch (cop_state.state_prev)
			{
				case copper_states::COP_read1:
					// Wake up is delayed by 1 copper cycle if copper is currently loading words
					cop_state.state = COP_strobe_delay3;
					break;
				case copper_states::COP_read2:
					// Wake up is delayed by 1 copper cycle if copper is currently loading words
					cop_state.state = COP_strobe_delay4;
					break;
			}
		} else {
			cop_state.state = copper_access ? COP_strobe_delay1 : COP_strobe_extra;
		}
	}
	cop_state.vblankip = cop1lc;
	copper_enabled_thisline = 0;
	if (vblank) {
		cop_state.strobe = num;
	} else {
		cop_state.strobe |= num;
	}
	cop_state.last_strobe = num;

	if (custom_disabled) {
		immediate_copper(num);
		return;
	}

	compute_spcflag_copper();
}

STATIC_INLINE void COPCON(uae_u16 a)
{
	copcon = a;
}

static void check_copper_stop(void)
{
	if (copper_enabled_thisline < 0 && !(is_copper_dma(false) && (dmacon & DMA_MASTER))) {
		copper_enabled_thisline = 0;
		unset_special(SPCFLAG_COPPER);
	}
}

static void copper_stop(void)
{
	copper_enabled_thisline = 0;
	unset_special(SPCFLAG_COPPER);
}

static void bitplane_dma_change(uae_u32 v)
{
	dmacon_bpl = (v & DMA_BITPLANE) && (v & DMA_MASTER);
}

static void compute_spcflag_copper_delayed(uae_u32 v)
{
	compute_spcflag_copper();
}

static void DMACON(int hpos, uae_u16 v)
{
	uae_u16 changed;
	uae_u16 oldcon = dmacon;

	decide_line(hpos);
	decide_fetch_safe(hpos);

	setclr(&dmacon, v);
	dmacon &= 0x07FF;

	changed = dmacon ^ oldcon;
#if 0
	if (changed)
		write_log(_T("%04x -> %04x %08x\n"), oldcon, dmacon, m68k_getpc ());
#endif

	int oldcop = (oldcon & DMA_COPPER) && (oldcon & DMA_MASTER);
	int newcop = (dmacon & DMA_COPPER) && (dmacon & DMA_MASTER);
	if (!oldcop && newcop) {
		bootwarpmode();
	}
	if (newcop && !oldcop) {
		if (copper_access || safecpu()) {
			copper_dma_change_cycle = get_cycles();
			event2_newevent_xx(-1, CYCLE_UNIT, 0, compute_spcflag_copper_delayed);
		} else {
			compute_spcflag_copper();
		}
	} else if (!newcop && oldcop) {
		if (copper_access || safecpu()) {
			copper_dma_change_cycle = get_cycles();
		}
		compute_spcflag_copper();
	}

	int oldblt = (oldcon & DMA_BLITTER) && (oldcon & DMA_MASTER);
	int newblt = (dmacon & DMA_BLITTER) && (dmacon & DMA_MASTER);
	if (oldblt != newblt && (copper_access || safecpu())) {
		if (copper_access) {
			blitter_dma_change_cycle = get_cycles();
		} else {
			// because of CPU vs blitter emulation side-effect
			blitter_dma_change_cycle = get_cycles() + CYCLE_UNIT;
		}
	}

	int oldspr = (oldcon & DMA_SPRITE) && (oldcon & DMA_MASTER);
	int newspr = (dmacon & DMA_SPRITE) && (dmacon & DMA_MASTER);
	if (!oldspr && newspr) {
		if (copper_access || safecpu()) {
			sprite_dma_change_cycle_on = get_cycles() + CYCLE_UNIT;
		}
	}

#if 0
	int oldb = (oldcon & DMA_BLITTER) && (oldcon & DMA_MASTER);
	int newb = (dmacon & DMA_BLITTER) && (dmacon & DMA_MASTER);
	int oldbn = (oldcon & DMA_BLITPRI) != 0;
	int newbn = (dmacon & DMA_BLITPRI) != 0;
	if (oldbn != newbn)
		write_log (_T("BLITTER NASTY: %d -> %d %08x\n"), oldbn, newbn, m68k_getpc ());
#endif

#if SPRITE_DEBUG > 0
	{
		int olds = (oldcon & DMA_SPRITE) && (oldcon & DMA_MASTER);
		int news = (dmacon & DMA_SPRITE) && (dmacon & DMA_MASTER);
		if (olds != news)
			write_log (_T("SPRITE DMA: %d -> %d %08x\n"), olds, news, m68k_getpc ());
	}
#endif

	if ((dmacon & DMA_BLITPRI) > (oldcon & DMA_BLITPRI) && (blt_info.blit_main || blt_info.blit_finald || blt_info.blit_queued)) {
		set_special(SPCFLAG_BLTNASTY);
	}

	if (dmaen(DMA_BLITTER) && blt_info.blit_pending) {
		blitter_check_start();
	}

	if ((dmacon & (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER)) != (DMA_BLITPRI | DMA_BLITTER | DMA_MASTER))
		unset_special(SPCFLAG_BLTNASTY);

	if (changed & (DMA_MASTER | DMA_AUD3 | DMA_AUD2 | DMA_AUD1 | DMA_AUD0)) {
		audio_state_machine();
	}

	if (changed & (DMA_MASTER | DMA_BITPLANE)) {
		// if ECS, DDFSTRT has already passed but DMA was off and DMA gets turned on: BPRUN actvates 1 cycle earlier
		bool bpl = (dmacon & DMA_BITPLANE) && (dmacon & DMA_MASTER);
		if (ecs_agnus && bpl && !dmacon_bpl && ddf_enable_on > 0) {
			dmacon_bpl = true;
		} else {
			event2_newevent_xx(-1, CYCLE_UNIT, dmacon, bitplane_dma_change);
		}
		SET_LINE_CYCLEBASED(hpos);
	}
}

static int irq_forced;
static evt_t irq_forced_delay;

void IRQ_forced(int lvl, int delay)
{
	irq_forced = lvl;
	irq_forced_delay = 0;
	if (delay > 0 && currprefs.cpu_compatible) {
		irq_forced_delay = get_cycles() + delay * CYCLE_UNIT;
	} else if (delay < 0) {
		irq_forced_delay = -1;
	}
	doint();
}

void intlev_ack(int lvl)
{
	if (!irq_forced) {
		return;
	}
	if (lvl >= irq_forced && irq_forced_delay < 0) {
		irq_forced_delay = 0;
		irq_forced = 0;
	}
}

int intlev(void)
{
	if (irq_forced) {
		int lvl = irq_forced;
		if (irq_forced_delay == 0) {
			irq_forced = 0;
		} else if (irq_forced_delay > 0 && get_cycles() > irq_forced_delay) {
			irq_forced = 0;
			irq_forced_delay = 0;
		}
		return lvl;
	}
	uae_u16 imask = intreq2 & intena2;
	if (!(imask && (intena2 & 0x4000)))
		return 0;
	if (imask & (0x4000 | 0x2000))						// 13 14
		return 6;
	if (imask & (0x1000 | 0x0800))						// 11 12
		return 5;
	if (imask & (0x0400 | 0x0200 | 0x0100 | 0x0080))	// 7 8 9 10
		return 4;
	if (imask & (0x0040 | 0x0020 | 0x0010))				// 4 5 6
		return 3;
	if (imask & 0x0008)									// 3
		return 2;
	if (imask & (0x0001 | 0x0002 | 0x0004))				// 0 1 2
		return 1;
	return 0;
}

void rethink_uae_int(void)
{
	bool irq2 = false;
	bool irq6 = false;

	if (uae_int_requested) {
		if (uae_int_requested & 0xff00) {
			irq6 = true;
		}
		if (uae_int_requested & 0x00ff) {
			irq2 = true;
		}
	}

	{
		extern void bsdsock_fake_int_handler(void);
		extern int volatile bsd_int_requested;
		if (bsd_int_requested) {
			bsdsock_fake_int_handler();
		}
	}
	if (irq6) {
		safe_interrupt_set(IRQ_SOURCE_UAE, 0, true);
	}
	if (irq2) {
		safe_interrupt_set(IRQ_SOURCE_UAE, 0, false);
	}
}

static void rethink_intreq(void)
{
#ifdef SERIAL_PORT
	serial_rethink();
#endif
	devices_rethink();
}

static void intreq_checks(uae_u16 oldreq, uae_u16 newreq)
{
	if ((oldreq & 0x0800) != (newreq & 0x0800)) {
		serial_rbf_change((newreq & 0x0800) ? 1 : 0);
	}
}

static void event_doint_delay_do_ext(uae_u32 v)
{
	uae_u16 old = intreq2;
	setclr(&intreq, (1 << v) | 0x8000);
	setclr(&intreq2, (1 << v) | 0x8000);

	doint();
}

static void event_send_interrupt_do_ext(uae_u32 v)
{
	event2_newevent_xx(-1, 1 * CYCLE_UNIT, v, event_doint_delay_do_ext);
}

// external delayed interrupt
void INTREQ_INT(int num, int delay)
{
	if (m68k_interrupt_delay) {
		if (delay < CYCLE_UNIT) {
			delay *= CYCLE_UNIT;
		}
		event2_newevent_xx(-1, delay + CYCLE_UNIT, num, event_doint_delay_do_ext);
	} else {
		event_doint_delay_do_ext(num);
	}
}

static void event_doint_delay_do_intreq(uae_u32 v)
{
	uae_u16 old = intreq2;
	setclr(&intreq2, v);

	doint();
}

static void doint_delay_intreq(uae_u16 v)
{
	if (m68k_interrupt_delay) {
		// INTREQ or INTENA write: IPL line changes 0.5 CCKs later.
		// 68000 needs one more CPU clock (0.5 CCK) before it detects it.
		event2_newevent_xx(-1, 1 * CYCLE_UNIT, v, event_doint_delay_do_intreq);
	} else {
		event_doint_delay_do_intreq(v);
	}
}

static void event_doint_delay_do_intena(uae_u32 v)
{
	setclr(&intena2, v);

	doint();
}

static void doint_delay_intena(uae_u16 v)
{
	if (m68k_interrupt_delay) {
		// INTREQ or INTENA write: IPL line changes 0.5 CCKs later.
		// 68000 needs one more CPU clock (0.5 CCK) before it detects it.
		event2_newevent_xx(-1, 1 * CYCLE_UNIT, v, event_doint_delay_do_intena);
	}
	else {
		event_doint_delay_do_intena(v);
	}
}

static void INTENA(uae_u16 v)
{
	uae_u16 old = intena;
	setclr(&intena, v);

	if (old != intena) {
		doint_delay_intena(v);
	}
}

static void INTREQ_nodelay(uae_u16 v)
{
	uae_u16 old = intreq;
	setclr(&intreq, v);
	intreq2 = intreq;
	intreq_checks(old, intreq);
	doint();
}

void INTREQ_f(uae_u16 v)
{
	uae_u16 old = intreq;
	setclr(&intreq, v);
	intreq2 = intreq;
	intreq_checks(old, intreq);
}

bool INTREQ_0(uae_u16 v)
{
	uae_u16 old = intreq;
	setclr(&intreq, v);

	if (old != intreq) {
		doint_delay_intreq(v);
		intreq_checks(old, intreq);
	}
	return true;
}

void INTREQ(uae_u16 data)
{
	if (INTREQ_0(data)) {
		rethink_intreq();
	}
}

static void ADKCON(int hpos, uae_u16 v)
{
	if (currprefs.produce_sound > 0) {
		update_audio();
	}
	DISK_update(hpos);
	DISK_update_adkcon(hpos, v);
	setclr(&adkcon, v);
	DISK_update_predict();
	audio_update_adkmasks();
#ifdef SERIAL_PORT
	if ((v >> 11) & 1) {
		serial_uartbreak((adkcon >> 11) & 1);
	}
#endif
}

static void check_harddis(void)
{
	// VARBEAMEN, HARDDIS, SHRES, UHRES
	harddis_h = ecs_agnus && ((new_beamcon0 & BEAMCON0_VARBEAMEN) || (new_beamcon0 & BEAMCON0_HARDDIS) || (bplcon0 & 0x0040) || (bplcon0 & 0x0080));
	// VARBEAMEN, VARVBEN, HARDDIS
	harddis_v = ecs_agnus && ((new_beamcon0 & BEAMCON0_VARBEAMEN) || (new_beamcon0 & BEAMCON0_VARVBEN) || (new_beamcon0 & BEAMCON0_HARDDIS));
}

static void BEAMCON0(int hpos, uae_u16 v)
{
	if (ecs_agnus) {
		bool beamcon0_changed = false;
		if (v != new_beamcon0) {
			sync_changes(hpos);
			new_beamcon0 = v;
			write_log(_T("BEAMCON0 = %04X, PC=%08X\n"), v, M68K_GETPC);
			// not LPENDIS, LOLDIS, PAL, BLANKEN, SYNC INVERT
			if (v & ~(BEAMCON0_LPENDIS | BEAMCON0_LOLDIS | BEAMCON0_PAL | BEAMCON0_BLANKEN | BEAMCON0_CSYTRUE | BEAMCON0_VSYTRUE | BEAMCON0_HSYTRUE)) {
				dumpsync();
			}
			beamcon0_changed = true;
			beamcon0_saved = v;
			record_register_change(hpos, 0x1dc, new_beamcon0);
			check_harddis();
			calcdiw();
			vb_check();
			decide_vline(hpos);
		}
		if (beamcon0_changed) {
			init_beamcon0(false);
		}
	}
}

static void varsync(int reg, bool resync, int oldval)
{
	struct amigadisplay *ad = &adisplays[0];
	if (!ecs_agnus) {
		return;
	}
#ifdef PICASSO96
	if (ad->picasso_on && p96refresh_active) {
		vtotal = p96refresh_active;
		return;
	}
#endif
	thisline_changed = 1;
	updateextblk();

	// VB
	if ((reg == 0x1cc || reg == 0x1ce) && (beamcon0 & BEAMCON0_VARVBEN)) {
		// check for >1 because of interlace modes
		if ((reg == 0x1cc && abs(vbstrt - oldval) > 1) || (reg == 0x1ce && abs(vbstop - oldval) > 1)) {
			// check VB changes only if there are not many changes per frame
			varsync_maybe_changed[reg == 0x1cc ? 0 : 1]++;
		}
	}
	// HB
	if ((reg == 0x1c4 || reg == 0x1c6) && exthblank) {
		// only do resync if HB value has been valid at least 66% of field
		varhblank_lines = (maxvpos / 3) * 2;
	}

	if (!resync) {
		return;
	}

	// TOTAL
	if ((reg == 0x1c0 || reg == 0x1c8) && (beamcon0 & BEAMCON0_VARBEAMEN)) {
		varsync_changed = 1;
		nosignal_trigger = true;
	}
	// VB
	if ((reg == 0x1cc || reg == 0x1ce) && (beamcon0 & BEAMCON0_VARVBEN)) {
		varsync_changed = 1;
	}
	// VS
	if ((reg == 0x1e0 || reg == 0x1ca) && (beamcon0 & bemcon0_vsync_mask)) {
		varsync_changed = 1;
		nosignal_trigger = true;
	}
	// HS
	if ((reg == 0x1de || reg == 0x1c2) && (beamcon0 & bemcon0_hsync_mask)) {
		varsync_changed = 1;
	}

	if (varsync_changed) {
		varsync_maybe_changed[0] = 0;
		varsync_maybe_changed[1] = 0;
	}

	if (varsync_changed) {
		init_beamcon0(false);
	}
}

#ifdef PICASSO96
void set_picasso_hack_rate(int hz)
{
	struct amigadisplay *ad = &adisplays[0];
	if (!ad->picasso_on)
		return;
	vpos_count = 0;
	p96refresh_active = (int)(maxvpos_stored * vblank_hz_stored / hz);
	if (!currprefs.cs_ciaatod)
		changed_prefs.cs_ciaatod = currprefs.cs_ciaatod = currprefs.ntscmode ? 2 : 1;
	if (p96refresh_active > 0) {
		new_beamcon0 |= BEAMCON0_VARBEAMEN;
	}
	varsync_changed = 1;
}
#endif

/* "Dangerous" blitter D-channel: Writing to memory which is also currently
 * read by bitplane DMA
 */
static void dcheck_is_blit_dangerous(void)
{
	check_is_blit_dangerous(bplpt, bplcon0_planes, 50 << bplcon0_res);
}

static void BPLxPTH(int hpos, uae_u16 v, int num)
{
	decide_line(hpos);
	decide_fetch_safe(hpos);
	if (copper_access) {
		int n = get_bitplane_dma_rel(hpos, 1);
		if (n == num + 1) {
			SET_LINE_CYCLEBASED(hpos);
			return;
		}
	}

	bplpt[num] = (bplpt[num] & 0x0000ffff) | ((uae_u32)v << 16);
	bplptx[num] = (bplptx[num] & 0x0000ffff) | ((uae_u32)v << 16);
	dcheck_is_blit_dangerous ();
	//write_log (_T("%d:%d:BPL%dPTH %08X COP=%08x\n"), hpos, vpos, num, bplpt[num], cop_state.ip);
}
static void BPLxPTL(int hpos, uae_u16 v, int num)
{
	decide_line(hpos);
	decide_fetch_safe(hpos);
	/* only detect copper accesses to prevent too fast CPU mode glitches */
	if (copper_access) {
		int n = get_bitplane_dma_rel(hpos, 1);
		if (n == num + 1) {
			SET_LINE_CYCLEBASED(hpos);
			return;
		}
	}
	bplpt[num] = (bplpt[num] & 0xffff0000) | (v & 0x0000fffe);
	bplptx[num] = (bplptx[num] & 0xffff0000) | (v & 0x0000fffe);
	dcheck_is_blit_dangerous ();
	//write_log (_T("%d:%d:BPL%dPTL %08X COP=%08x\n"), hpos, vpos, num, bplpt[num], cop_state.ip);
}

static void BPLCON0_Denise(int hpos, uae_u16 v)
{
	v = BPLCON0_Denise_mask(v);

	if (bplcon0d == v && !bplcon0d_change) {
		return;
	}
	bplcon0d_change = false;

	// fake unused 0x0080 bit as an EHB bit (see below)
	if (isehb(bplcon0d, bplcon2)) {
		v |= 0x0080;
	}

	record_register_change(hpos, 0x101, v);

	bplcon0d = v & ~0x0080;
	bplcon0d_old = -1;

#ifdef ECS_DENISE
	if (ecs_denise) {
		decide_sprites(hpos);
		sprres = expand_sprres(v, bplcon3);
	}
#endif
	if (thisline_decision.plfleft < 0) {
		update_denise(hpos);
	}
}

static void bpldmainit(int hpos, uae_u16 bplcon0)
{
	BPLCON0_Denise(hpos, bplcon0);
	setup_fmodes(hpos, bplcon0);
}

static void BPLCON0(int hpos, uae_u16 v);

static void bplcon0_denise_reschange(int res, int oldres)
{
	flush_display(fetchmode);

	toscr_res = res;
	toscr_res_old = res;
	update_toscr_vars();
	compute_toscr_delay(bplcon1);

	if (aga_mode) {
		if (oldres == RES_LORES && res == RES_HIRES) {
			toscr_special_skip_ptr = toscr_spc_aga_lores_to_hires;
		}
		if (oldres == RES_HIRES && res == RES_LORES) {
			toscr_special_skip_ptr = toscr_spc_aga_hires_to_lores;
		}
	} else if (0 && ecs_denise) {
		if (oldres == RES_LORES && res == RES_HIRES) {
			toscr_special_skip_ptr = toscr_spc_ecs_lores_to_hires;
		}
		if (oldres == RES_HIRES && res == RES_LORES) {
			toscr_special_skip_ptr = toscr_spc_ecs_hires_to_lores;
		}
	} else if (1) {
		if (oldres == RES_LORES && res == RES_HIRES) {
			toscr_special_skip_ptr = toscr_spc_ocs_lores_to_hires;
		}
		if (oldres == RES_HIRES && res == RES_LORES) {
			toscr_special_skip_ptr = toscr_spc_ocs_hires_to_lores;
		}
	}
}

static void bplcon0_denise_change_early(int hpos, uae_u16 con0)
{
	uae_u16 dcon0 = BPLCON0_Denise_mask(con0);
	uae_u16 dcon0o = BPLCON0_Denise_mask(bplcon0);

	int np = GET_PLANES(dcon0);
	int res = GET_RES_DENISE(dcon0);
	if (bplcon0d != dcon0) {
		bplcon0d = dcon0;
		bplcon0d_change = true;
	}
	if (np == toscr_nr_planes_shifter_new) {
		toscr_nr_planes_shifter = np;
	} else {
		bplcon0_planes_changed = true;
	}
	
	decide_hdiw(hpos);
	SET_LINE_CYCLEBASED(hpos);

	if (aga_mode) {
		int e1 = isehb(dcon0, bplcon2);
		int e2 = isehb(dcon0o, bplcon2);
		if (e1 ^ e2) {
			record_register_change(hpos, 0x201, dcon0);
		}
	}

	toscr_nr_planes_shifter_new = np;
	if (ecs_denise || aga_mode) {
		toscr_nr_changed = np != toscr_nr_planes3 && res == toscr_res;
	}
	toscr_nr_planes3 = np;

	if (currprefs.chipset_hr && res != toscr_res) {
		bplcon0_denise_reschange(res, toscr_res);
	}

	if (isocs7planes()) {
		toscr_nr_planes_shifter_new = std::max(toscr_nr_planes_shifter_new, 6);
	}

	if (np > toscr_nr_planes2) {
		update_toscr_planes(np, fetchmode);
		toscr_nr_planes2 = np;
		thisline_decision.max_planes = np;
	}
}

static void BPLCON0(int hpos, uae_u16 v)
{
	uae_u16 old = bplcon0;
	bplcon0_saved = v;
	v = BPLCON0_Agnus_mask(v);

#if SPRBORDER
	v |= 1;
#endif
	if (bplcon0 == v) {
		return;
	}

	decide_hdiw(hpos);
	SET_LINE_CYCLEBASED(hpos);

	if ((v & 1) != (bplcon0 & 1)) {
		sync_color_changes(hpos);
	}

	if (!issyncstopped()) {
		vpos_previous = vpos;
		hpos_previous = hpos;
	}

	if (v & 4) {
		bplcon0_interlace_seen = true;
	}

	if ((v & 8) && !lightpen_triggered && vb_state) {
		// setting lightpen bit immediately freezes VPOSR if inside vblank and not already frozen
		hhpos_lpen = HHPOSR();
		lightpen_triggered = 1;
		vpos_lpen = vpos;
		hpos_lpen = hpos;
	} 
	if (!(v & 8)) {
		// clearing lightpen bit immediately returns VPOSR back to normal
		lightpen_triggered = 0;
	}

	bplcon0 = v;

	check_harddis();

	if ((old & 1) != (bplcon0 & 1)) {
		updateextblk();
	}

	bpldmainit(hpos, bplcon0);

	if (!copper_access) {
		bplcon0_denise_change_early(hpos, bplcon0);
	}
}

static void BPLCON1(int hpos, uae_u16 v)
{
	bplcon1_saved = v;
	if (!aga_mode) {
		v &= 0xff;
	}
	if (bplcon1 == v) {
		return;
	}
	SET_LINE_CYCLEBASED(hpos);
	bplcon1_written = true;
	bplcon1 = v;
	hack_shres_delay(hpos);
}

static void BPLCON2(int hpos, uae_u16 v)
{
	bplcon2_saved = v;
	if (!aga_mode) {
		v &= ~(0x100 | 0x80); // RDRAM and SOGEN
	}
	if (!ecs_denise) {
		v &= 0x7f;
	}
	v &= ~0x8000; // unused
	if (bplcon2 == v) {
		return;
	}
	decide_line(hpos);
	bplcon2 = v;
	record_register_change(hpos, 0x104, bplcon2);
}

#ifdef ECS_DENISE
static void BPLCON3(int hpos, uae_u16 v)
{
	uae_u16 old = bplcon3;
	bplcon3_saved = v;
	if (!ecs_denise) {
		return;
	}
	if (!aga_mode) {
		v &= (0x0020 | 0x0010 | 0x004 | 0x001);
		v |= 0x0c00;
	}
#if SPRBORDER
	v |= 2;
#endif
	if (bplcon3 == v) {
		return;
	}
	decide_line(hpos);
	decide_sprites(hpos);
	if ((bplcon3 & 1) != (v & 1)) {
		sync_color_changes(hpos);
	}
	bplcon3 = v;
	if ((bplcon3 & 1) != (old & 1)) {
		updateextblk();
	}
	sprres = expand_sprres(bplcon0, bplcon3);
	record_register_change(hpos, 0x106, v);
}
#endif
#ifdef AGA
static void BPLCON4(int hpos, uae_u16 v)
{
	bplcon4_saved = v;
	if (!aga_mode) {
		return;
	}
	if (bplcon4 == v) {
		return;
	}
	decide_line(hpos);
	record_register_change(hpos, 0x10c, v);
	bplcon4 = v;
}
#endif

static void BPL1MOD(int hpos, uae_u16 v)
{
	v &= ~1;
	decide_line(hpos);
	decide_fetch_safe(hpos);
	bpl1mod_prev = bpl1mod;
	bpl1mod_hpos = hpos + 1;
	if (bpl1mod_hpos >= maxhpos) {
		bpl1mod_hpos -= maxhpos;
	}
	bpl1mod = v;
}

static void BPL2MOD(int hpos, uae_u16 v)
{
	v &= ~1;
	decide_line(hpos);
	decide_fetch_safe(hpos);
	bpl2mod_prev = bpl2mod;
	bpl2mod_hpos = hpos + 1;
	if (bpl2mod_hpos >= maxhpos) {
		bpl2mod_hpos -= maxhpos;
	}
	bpl2mod = v;
}

/* Needed in special OCS/ECS "7-plane" mode,
 * also handles CPU generated bitplane data
 */
static void BPLxDAT_next(uae_u32 vv)
{
	int num = vv >> 16;
	uae_u16 data = vv;

	int hpos = current_hpos();

	decide_line(hpos);
	decide_fetch_safe(hpos);
	flush_display(fetchmode);

	fetched[num] = data;
	if ((fmode & 3) == 3) {
		fetched_aga[num] = ((uae_u64)last_custom_value << 48) | ((uae_u64)data << 32) | ((uae_u64)data << 16) | data;
	} else if ((fmode & 3) == 2) {
		fetched_aga[num] = ((uae_u32)last_custom_value << 16) | data;
	} else if ((fmode & 3) == 1) {
		fetched_aga[num] = ((uae_u32)data << 16) | data;
	} else {
		fetched_aga[num] = data;
	}

	if (num == 0) {
		// ECS/AGA: HSYNC start - 1: $0C is first possible.
		if (hpos != hsyncstartpos_start_cycles - 1) {
			beginning_of_plane_block(hpos);
			bprun_pipeline_flush_delay = maxhpos;
			if (bplcon0_planes_changed) {
				flush_display(fetchmode);
				toscr_nr_planes_shifter = toscr_nr_planes_shifter_new;
				bplcon0_planes_changed = false;
			}
		}
	}
}

static void BPLxDAT(int hpos, int num, uae_u16 data)
{
	uae_u32 vv = (num << 16) | data;
	if (!num) {
		SET_LINE_CYCLEBASED(hpos);
	}
	event2_newevent_xx_ce(1 * CYCLE_UNIT, vv, BPLxDAT_next);
}

static void DIWSTRT_next(uae_u32 v)
{
	int hpos = current_hpos();
	decide_hdiw(hpos);
	decide_line(hpos);
	diwhigh_written = 0;
	diwstrt = v;
	calcdiw();
}
static void DIWSTRT(int hpos, uae_u16 v)
{
	if (diwstrt == v && !diwhigh_written) {
		return;
	}
	event2_newevent_xx_ce(1 * CYCLE_UNIT, v, DIWSTRT_next);
}

static void DIWSTOP_next(uae_u32 v)
{
	int hpos = current_hpos();
	decide_hdiw(hpos);
	decide_line(hpos);
	diwhigh_written = 0;
	diwstop = v;
	calcdiw();
}
static void DIWSTOP(int hpos, uae_u16 v)
{
	if (diwstop == v && !diwhigh_written) {
		return;
	}
	event2_newevent_xx_ce(1 * CYCLE_UNIT, v, DIWSTOP_next);
}

static void DIWHIGH_next(uae_u32 v)
{
	int hpos = current_hpos();
	// DIWHIGH has 1.5CCK delay
	decide_hdiw(hpos, true);
	decide_line(hpos);
	diwhigh_written = 1;
	diwhigh = v;
	calcdiw();
}
static void DIWHIGH(int hpos, uae_u16 v)
{
	if (!ecs_agnus && !ecs_denise) {
		return;
	}
	if (!aga_mode) {
		v &= ~(0x0010 | 0x1000);
	}
	v &= ~(0x8000 | 0x4000 | 0x0080 | 0x0040);
	if (diwhigh_written && diwhigh == v) {
		return;
	}
	event2_newevent_xx_ce(1 * CYCLE_UNIT, v, DIWHIGH_next);
}

static void DDFSTRT(int hpos, uae_u16 v)
{
	v &= 0xfe;
	if (!ecs_agnus) {
		v &= 0xfc;
	}
	SET_LINE_CYCLEBASED(hpos);
	ddfstrt = v;
	calcdiw();
	// DDFSTRT modified when DDFSTRT==hpos: neither value matches
	ddfstrt_hpos = hpos;
}

static void DDFSTOP(int hpos, uae_u16 v)
{
	v &= 0xfe;
	if (!ecs_agnus) {
		v &= 0xfc;
	}
	SET_LINE_CYCLEBASED(hpos);
	plfstop_prev = plfstop;
	ddfstop = v;
	calcdiw();
	// DDFSTOP modified when DDFSTOP==hpos: old value matches
	ddfstop_hpos = hpos;
	estimate_last_fetch_cycle(hpos);
}

static void FMODE(int hpos, uae_u16 v)
{
	if (!aga_mode) {
#ifdef WITH_SPECIALMONITORS
		if (currprefs.monitoremu) {
			specialmonitor_store_fmode(vpos, hpos, v);
		}
#endif
		fmode_saved = v;
		v = 0;
	}
	v &= 0xC00F;
	if (fmode == v) {
		return;
	}
	SET_LINE_CYCLEBASED(hpos);
	if (aga_mode) {
		fmode_saved = v;
	}
	set_chipset_mode();
	bpldmainit(hpos, bplcon0);
	record_register_change(hpos, 0x1fc, fmode);
}

static void FNULL(uae_u16 v)
{

}

static void BLTADAT(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	blt_info.bltadat = v;
}
/*
* "Loading data shifts it immediately" says the HRM. Well, that may
* be true for BLTBDAT, but not for BLTADAT - it appears the A data must be
* loaded for every word so that AFWM and ALWM can be applied.
*/
static void BLTBDAT(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	int shift = bltcon1 >> 12;
	if (bltcon1 & 2) {
		blt_info.bltbhold = (((uae_u32)v << 16) | blt_info.bltbold) >> (16 - shift);
	} else {
		blt_info.bltbhold = (((uae_u32)blt_info.bltbold << 16) | v) >> shift;
	}
	blt_info.bltbdat = v;
	blt_info.bltbold = v;
}
static void BLTCDAT(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	blt_info.bltcdat = v;
	reset_blit(0);
}
static void BLTAMOD(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 1);
	blt_info.bltamod = (uae_s16)(v & 0xFFFE);
	reset_blit(0);
}
static void BLTBMOD(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 1);
	blt_info.bltbmod = (uae_s16)(v & 0xFFFE);
	reset_blit(0);
}
static void BLTCMOD(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 1);
	blt_info.bltcmod = (uae_s16)(v & 0xFFFE);
	reset_blit(0);
}
static void BLTDMOD(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 1);
	blt_info.bltdmod = (uae_s16)(v & 0xFFFE);
	reset_blit(0);
}
static void BLTCON0(int hpos, uae_u16 v)
{
	maybe_blit (hpos, 2);
	bltcon0 = v;
	reset_blit(1);
}
/* The next category is "Most useless hardware register".
* And the winner is... */
static void BLTCON0L(int hpos, uae_u16 v)
{
	if (!ecs_agnus)
		return; // ei voittoa.
	maybe_blit(hpos, 2); bltcon0 = (bltcon0 & 0xFF00) | (v & 0xFF);
	reset_blit(0);
}
static void BLTCON1(int hpos, uae_u16 v) {
	maybe_blit(hpos, 2);
	bltcon1 = v;
	reset_blit(2);
}
static void BLTAFWM(int hpos, uae_u16 v) {
	maybe_blit(hpos, 2);
	blt_info.bltafwm = v;
	reset_blit(0);
}
static void BLTALWM(int hpos, uae_u16 v) {
	maybe_blit(hpos, 2);
	blt_info.bltalwm = v;
	reset_blit(0);
}
static void setblitx(int hpos, int n)
{
	bltptxpos = (hpos + 1) % maxhpos;
	bltptxc = copper_access ? n : -n;
}
static void BLTAPTH(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	bltptx = bltapt;
	setblitx(hpos, 1);
	bltapt = (bltapt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTAPTL(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	bltptx = bltapt;
	setblitx(hpos, 1);
	bltapt = (bltapt & ~0xffff) | (v & 0xfffe);
}
static void BLTBPTH(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	bltptx = bltbpt;
	bltptxpos = hpos;
	bltptxc = copper_access ? 2 : -2;
	bltbpt = (bltbpt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTBPTL(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	bltptx = bltbpt;
	setblitx(hpos, 2);
	bltbpt = (bltbpt & ~0xffff) | (v & 0xfffe);
}
static void BLTCPTH(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	bltptx = bltcpt;
	setblitx(hpos, 3);
	bltcpt = (bltcpt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTCPTL(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	bltptx = bltcpt;
	setblitx(hpos, 3);
	bltcpt = (bltcpt & ~0xffff) | (v & 0xfffe);
}
static void BLTDPTH (int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);

	if (blt_info.blit_finald && copper_access) {
		static int warned = 100;
		if (warned > 0) {
			warned--;
			write_log("Possible Copper Blitter wait bug detected COP=%08x\n", cop_state.ip);
		}
	}
	bltptx = bltdpt;
	setblitx(hpos, 4);
	bltdpt = (bltdpt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTDPTL(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);

	if (blt_info.blit_finald && copper_access) {
		static int warned = 100;
		if (warned > 0) {
			warned--;
			write_log("Possible Copper Blitter wait bug detected COP=%08x\n", cop_state.ip);
		}
	}

	bltptx = bltdpt;
	setblitx(hpos, 4);
	bltdpt = (bltdpt & ~0xffff) | (v & 0xfffe);
}

static void BLTSIZE(int hpos, uae_u16 v)
{
	maybe_blit(hpos, 0);
	blt_info.vblitsize = v >> 6;
	blt_info.hblitsize = v & 0x3F;
	if (!blt_info.vblitsize) {
		blt_info.vblitsize = 1024;
	}
	if (!blt_info.hblitsize) {
		blt_info.hblitsize = 64;
	}
	do_blitter(hpos, copper_access, copper_access ? cop_state.ip : M68K_GETPC);
	dcheck_is_blit_dangerous();
}

static void BLTSIZV(int hpos, uae_u16 v)
{
	if (!ecs_agnus) {
		return;
	}
	maybe_blit(hpos, 0);
	blt_info.vblitsize = v & 0x7FFF;
	if (!blt_info.vblitsize) {
		blt_info.vblitsize = 0x8000;
	}
}

static void BLTSIZH(int hpos, uae_u16 v)
{
	if (!ecs_agnus) {
		return;
	}
	maybe_blit(hpos, 0);
	blt_info.hblitsize = v & 0x7FF;
	if (!blt_info.vblitsize) {
		blt_info.vblitsize = 0x8000;
	}
	if (!blt_info.hblitsize) {
		blt_info.hblitsize = 0x0800;
	}
	do_blitter(hpos, copper_access, copper_access ? cop_state.ip : M68K_GETPC);
	dcheck_is_blit_dangerous();
}

static void spr_arm(int num, int state)
{
#if SPRITE_DEBUG > 0
	if (spr[num].armed != state)
		write_log (_T("SPR%d ARM=%d\n"), num, state);
#endif
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

static void sprstartstop(struct sprite *s)
{
	if (vb_state || vb_end_line) {
		return;
	}
	if (vpos == s->vstart) {
		s->dmastate = 1;
	}
	if (vpos == s->vstop) {
		s->dmastate = 0;
		s->dmacycle = 0;
	}
}

static void SPRxCTLPOS(int num)
{
	int sprxp;
	struct sprite *s = &spr[num];

	sprstartstop(s);
	sprxp = (s->pos & 0xFF) * 2 + (s->ctl & 1);
	sprxp <<= sprite_buffer_res;
	s->dblscan = 0;
	s->ecs_denise_hires = 0;
	/* Quite a bit salad in this register... */
	if (0) {
	}
#ifdef AGA
	else if (aga_mode) {
		sprxp |= ((s->ctl >> 3) & 3) >> (RES_MAX - sprite_buffer_res);
		s->dblscan = s->pos & 0x80;
	}
#endif
#ifdef ECS_DENISE
	else if (ecs_denise && (s->ctl & 0x10)) {
		// This bit only works as documented if superhires bitplane resolution.
		// If bitplane resolution is lores or hires: sprite's first pixel row
		// becomes transparent.
		s->ecs_denise_hires = 1;
	}
#endif
	s->xpos = sprxp;
	s->vstart = s->pos >> 8;
	s->vstart |= (s->ctl & 0x04) ? 0x0100 : 0;
	s->vstop = s->ctl >> 8;
	s->vstop |= (s->ctl & 0x02) ? 0x100 : 0;
	if (ecs_agnus) {
		s->vstart |= (s->ctl & 0x40) ? 0x0200 : 0;
		s->vstop |= (s->ctl & 0x20) ? 0x0200 : 0;
	}
	sprstartstop(s);
}

static void SPRxCTL_1(uae_u16 v, int num, int hpos)
{
	struct sprite *s = &spr[num];
	s->ctl = v;
	spr_arm(num, 0);
	SPRxCTLPOS(num);

#if 0
	if (!(currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
		if (s->ctl & (0x02 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40)) {
			write_log(_T("ECS sprite %04x\n"), s->ctl);
		}
}
#endif
#if SPRITE_DEBUG > 0
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log (_T("%d:%d:SPR%dCTL %04X P=%06X VSTRT=%d VSTOP=%d HSTRT=%d D=%d A=%d CP=%x PC=%x\n"),
			vpos, hpos, num, v, s->pt, s->vstart, s->vstop, s->xpos, spr[num].dmastate, spr[num].armed, cop_state.ip, M68K_GETPC);
	}
#endif

}
static void SPRxPOS_1(uae_u16 v, int num, int hpos)
{
	struct sprite *s = &spr[num];

	s->pos = v;
	SPRxCTLPOS(num);
#if SPRITE_DEBUG > 0
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log (_T("%d:%d:SPR%dPOS %04X P=%06X VSTRT=%d VSTOP=%d HSTRT=%d D=%d A=%d CP=%x PC=%x\n"),
			vpos, hpos, num, v, s->pt, s->vstart, s->vstop, s->xpos, spr[num].dmastate, spr[num].armed, cop_state.ip, M68K_GETPC);
	}
#endif
}
static void SPRxDATA_1(uae_u16 v, int num, int hpos)
{
	struct sprite *s = &spr[num];
	s->data[0] = v;
#ifdef AGA
	if (aga_mode) {
		s->data[1] = v;
		s->data[2] = v;
		s->data[3] = v;
		s->width = sprite_width;
	}
#endif
	spr_arm(num, 1);
#if SPRITE_DEBUG >= 256
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log (_T("%d:%d:SPR%dDATA %04X P=%06X D=%d A=%d PC=%x\n"),
			vpos, hpos, num, v, spr[num].pt, spr[num].dmastate, spr[num].armed, M68K_GETPC);
	}
#endif
}
static void SPRxDATB_1(uae_u16 v, int num, int hpos)
{
	struct sprite *s = &spr[num];
	s->datb[0] = v;
#ifdef AGA
	if (aga_mode) {
		s->datb[1] = v;
		s->datb[2] = v;
		s->datb[3] = v;
		s->width = sprite_width;
	}
#endif
#if SPRITE_DEBUG >= 256
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log (_T("%d:%d:SPR%dDATB %04X P=%06X D=%d A=%d PC=%x\n"),
			vpos, hpos, num, v, spr[num].pt, spr[num].dmastate, spr[num].armed, M68K_GETPC);
	}
#endif
}

// Undocumented AGA feature: if sprite is 64 pixel wide, SPRxDATx is written and next
// cycle is DMA fetch: sprite's first 32 pixels get replaced with bitplane data.
static void sprite_get_bpl_data(int hpos, struct sprite *s, uae_u16 *dat)
{
	int nr = get_bitplane_dma_rel(hpos, 1);
	uae_u32 v = (uae_u32)((fmode & 3) ? fetched_aga[nr] : fetched_aga_spr[nr]);
	dat[0] = v >> 16;
	dat[1] = (uae_u16)v;
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

static void SPRxDATA(int hpos, uae_u16 v, int num)
{
	struct sprite *s = &spr[num];
	decide_sprites(hpos, false);
	SPRxDATA_1(v, num, hpos);
	// if 32 (16-bit double CAS only) or 64 pixel wide sprite and SPRxDATx write:
	// - first 16 pixel part: previous chipset bus data
	// - following 16 pixel parts: written data
	if (fmode & 8) {
		if ((fmode & 4) && get_bitplane_dma_rel(hpos, -1)) {
			sprite_get_bpl_data(hpos, s, &s->data[0]);
		} else {
			s->data[0] = last_custom_value;
		}
	}
}

static void SPRxDATB(int hpos, uae_u16 v, int num)
{
	struct sprite *s = &spr[num];
	decide_sprites(hpos, false);
	SPRxDATB_1(v, num, hpos);
	// See above
	if (fmode & 8) {
		if ((fmode & 4) && get_bitplane_dma_rel(hpos, -1)) {
			sprite_get_bpl_data(hpos, s, &s->datb[0]);
		} else {
			s->datb[0] = last_custom_value;
		}
	}
}

static void SPRxCTL_2(uae_u32 vv)
{
	int hpos = current_hpos();
	if (!currprefs.cpu_memory_cycle_exact) {
		// current_hpos() assumes cycle-by-cycle emulation which does not happen in non-CE modes. FIXME!
		hpos = ((vv >> 24) + 1) % maxhposm0;
	}
	uae_u16 v = vv >> 8;
	int num = vv & 7;
#if SPRITE_DEBUG > 0
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log(_T("%d:%d:SPR%dCTLC %06X\n"), vpos, hpos, num, spr[num].pt);
	}
#endif

	decide_sprites(hpos);
	SPRxCTL_1(v, num, hpos);
}
static void SPRxCTL(int hpos, uae_u16 v, int num)
{
	uae_u32 vv = (hpos << 24) | (v << 8) | num;
	event2_newevent_xx_ce(1 * CYCLE_UNIT, vv, SPRxCTL_2);
}

static void SPRxPOS_2(uae_u32 vv)
{
	int hpos = current_hpos();
	if (!currprefs.cpu_memory_cycle_exact) {
		hpos = ((vv >> 24) + 1) % maxhposm0;
	}
	uae_u16 v = vv >> 8;
	int num = vv & 7;
	struct sprite *s = &spr[num];
#if SPRITE_DEBUG > 0
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log(_T("%d:%d:SPR%dPOSC %06X\n"), vpos, hpos, num, s->pt);
	}
#endif
	decide_sprites(hpos);
	SPRxPOS_1(v, num, hpos);
}
static void SPRxPOS(int hpos, uae_u16 v, int num)
{
	uae_u32 vv = (hpos << 24) | (v << 8) | num;
	event2_newevent_xx_ce(1 * CYCLE_UNIT, vv, SPRxPOS_2);
}

static void SPRxPTH(int hpos, uae_u16 v, int num)
{
	decide_line(hpos);
	decide_fetch_safe(hpos);
	decide_sprites(hpos);
	if (get_sprite_dma_rel(hpos, 1) != num || (!copper_access && !currprefs.cpu_memory_cycle_exact)) {
		spr[num].pt &= 0xffff;
		spr[num].pt |= (uae_u32)v << 16;
#if 0
	} else {
		write_log(_T("SPRxPTH %d\n"), num);
#endif
	}
#if SPRITE_DEBUG > 0
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log (_T("%d:%d:SPR%dPTH %06X\n"), vpos, hpos, num, spr[num].pt);
	}
#endif
}
static void SPRxPTL(int hpos, uae_u16 v, int num)
{
	decide_line(hpos);
	decide_fetch_safe(hpos);
	decide_sprites(hpos);
	if (get_sprite_dma_rel(hpos, 1) != num || (!copper_access && !currprefs.cpu_memory_cycle_exact)) {
		spr[num].pt &= ~0xffff;
		spr[num].pt |= v & ~1;
#if 0
	} else {
		write_log(_T("SPRxPTL %d\n"), num);
#endif
	}
#if SPRITE_DEBUG > 0
	if (vpos >= SPRITE_DEBUG_MINY && vpos <= SPRITE_DEBUG_MAXY && (SPRITE_DEBUG & (1 << num))) {
		write_log (_T("%d:%d:SPR%dPTL %06X\n"), vpos, hpos, num, spr[num].pt);
	}
#endif
}

static void CLXCON(int hpos, uae_u16 v)
{
	if (hpos >= 0) {
		check_collisions(hpos);
	}
	clxcon = v;
	clxcon_bpl_enable = (v >> 6) & 63;
	clxcon_bpl_match = v & 63;
	//write_log("%08x %04x %d %d\n", M68K_GETPC, v, clxcon_bpl_enable, clxcon_bpl_match);
}

static void CLXCON2(int hpos, uae_u16 v)
{
	if (!aga_mode)
		return;
	if (hpos >= 0) {
		check_collisions(hpos);
	}
	clxcon2 = v;
	clxcon_bpl_enable |= v & (0x40 | 0x80);
	clxcon_bpl_match |= (v & (0x01 | 0x02)) << 6;
}

static uae_u16 CLXDAT(int hpos)
{
	if (hpos >= 0) {
		check_collisions(hpos);
	}
	uae_u16 v = clxdat | 0x8000;
	//write_log("%08x %04x\n", M68K_GETPC, v);
	clxdat = 0;
	return v;
}

#ifdef AGA

void dump_aga_custom(void)
{
	int c1, c2, c3, c4;
	uae_u32 rgb1, rgb2, rgb3, rgb4;

	for (c1 = 0; c1 < 64; c1++) {
		c2 = c1 + 64;
		c3 = c2 + 64;
		c4 = c3 + 64;
		rgb1 = current_colors.color_regs_aga[c1];
		rgb2 = current_colors.color_regs_aga[c2];
		rgb3 = current_colors.color_regs_aga[c3];
		rgb4 = current_colors.color_regs_aga[c4];
		console_out_f (_T("%3d %08X %3d %08X %3d %08X %3d %08X\n"),
			c1, rgb1, c2, rgb2, c3, rgb3, c4, rgb4);
	}
}

static uae_u16 COLOR_READ(int num)
{
	int cr, cg, cb, colreg;
	uae_u16 cval;

	if (!aga_mode || !(bplcon2 & 0x0100))
		return 0xffff;

	colreg = ((bplcon3 >> 13) & 7) * 32 + num;
	cr = (current_colors.color_regs_aga[colreg] >> 16) & 0xFF;
	cg = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
	cb = current_colors.color_regs_aga[colreg] & 0xFF;
	if (bplcon3 & 0x200) {
		cval = ((cr & 15) << 8) | ((cg & 15) << 4) | ((cb & 15) << 0);
	} else {
		cval = ((cr >> 4) << 8) | ((cg >> 4) << 4) | ((cb >> 4) << 0);
		if (color_regs_genlock[num])
			cval |= 0x8000;
	}
	return cval;
}
#endif

static void checkautoscalecol0(void)
{
	if (!copper_access)
		return;
	if (vpos < 20)
		return;
	if (isbrdblank(-1, bplcon0, bplcon3))
		return;
	// autoscale if copper changes COLOR00 on top or bottom of screen
	if (vpos >= minfirstline) {
		int vpos2 = autoscale_bordercolors ? minfirstline : vpos;
		if (first_planes_vpos == 0) {
			first_planes_vpos = vpos2 - 2;
		}
		if (plffirstline_total == current_maxvpos()) {
			plffirstline_total = vpos2 - 2;
		}
		if (vpos2 > last_planes_vpos || vpos2 > plflastline_total) {
			plflastline_total = last_planes_vpos = vpos2 + 3;
		}
		autoscale_bordercolors = 0;
	} else {
		autoscale_bordercolors++;
	}
}

bool get_custom_color_reg(int colreg, uae_u8 *r, uae_u8 *g, uae_u8 *b)
{
	if (colreg < 0)
		return false;
	if (colreg >= 32 && !aga_mode) {
		return false;
	}
	if (colreg >= 256) {
		return false;
	}
	if (aga_mode) {
		*r = (current_colors.color_regs_aga[colreg] >> 16) & 0xFF;
		*g = (current_colors.color_regs_aga[colreg] >> 8) & 0xFF;
		*b = current_colors.color_regs_aga[colreg] & 0xFF;
	} else {
		*r = (current_colors.color_regs_ecs[colreg] >> 8) & 15;
		*r |= (*r) << 4;
		*g = (current_colors.color_regs_ecs[colreg] >> 4) & 15;
		*g |= (*g) << 4;
		*b = (current_colors.color_regs_ecs[colreg] >> 0) & 15;
		*b |= (*b) << 4;
	}
	return true;
}

static void COLOR_WRITE(int hpos, uae_u16 v, int num)
{
	bool samecycle = false;

	// skip color register write color change state update if COLOR0 register was already written in same cycle
	// fast CPU modes can write tens of thousands of color registers in single frame.
	if (currprefs.m68k_speed < 0 && num == 0) {
		if (line_start_cycles == custom_color_write_cycle) {
			if (color_writes_num++ > maxhpos / 2) {
				samecycle = true;
			}
		} else {
			color_writes_num = 0;
			custom_color_write_cycle = line_start_cycles;
		}
	}

#ifdef AGA
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
			color_regs_genlock[colreg] = v >> 15;
		}
		cval = (cr << 16) | (cg << 8) | cb | (color_regs_genlock[colreg] ? COLOR_CHANGE_GENLOCK : 0);
		if (cval == current_colors.color_regs_aga[colreg]) {
			return;
		}
		if ((cval & 0xffffff) && colreg == 0) {
			checkautoscalecol0();
		}

		/* Call this with the old table still intact. */
		if (!samecycle) {
			record_color_change(hpos, colreg, cval);
		}
		remembered_color_entry = -1;
		current_colors.color_regs_aga[colreg] = cval;
		current_colors.acolors[colreg] = getxcolor(cval);

	} else {
#endif
		if (!ecs_denise) {
			v &= 0xfff;
		}
		color_regs_genlock[num] = v >> 15;
		if (current_colors.color_regs_ecs[num] == v) {
			return;
		}
		if ((v & 0xfff) && num == 0) {
			checkautoscalecol0();
		}

		/* Call this with the old table still intact. */
		if (!samecycle) {
			record_color_change(hpos, num, v);
		}
		remembered_color_entry = -1;
		current_colors.color_regs_ecs[num] = v;
		current_colors.acolors[num] = getxcolor(v);
#ifdef AGA
	}
#endif
}

#if ESTIMATED_FETCH_MODE
bool bitplane_dma_access(int hpos, int coffset)
{
	if (line_cyclebased) {
		int offset = get_rga_pipeline(hpos, coffset);
		if (cycle_line_pipe[offset] & CYCLE_PIPE_BITPLANE) {
			return true;
		}
	} else {
		hpos += coffset;
		if (hpos >= maxhpos) {
			hpos -= maxhpos;
			if (estimated_cycles_next[hpos] > 0) {
				return true;
			}
		} else {
			if (estimated_cycles[hpos] > 0) {
				return true;
			}
		}
	}
	return false;
}
#else
bool bitplane_dma_access(int hpos, int offset)
{
	hpos += offset;
	if (hpos >= maxhpos) {
		hpos -= maxhpos;
	}

	int i = bpl_estimate_index;
	for (int j = 0; j < MAX_BPL_ESTIMATES; j++) {
		struct bpl_estimate *be = &bpl_estimates[i];
		if (be->vpos == vposh && be->startend) {
			uae_u16 bploffset = 0xffff;
			if (be->startend == 1) {
				if (hpos >= be->start_pos && hpos < be->end_pos) {
					bploffset = hpos - be->start_pos;
				}
			} else if (be->startend == 2) {
				if (hpos >= be->start_pos) {
					bploffset = hpos - be->start_pos;
				} else if (hpos < be->end_pos) {
					bploffset = maxhpos - be->start_pos + hpos;
				}
			}
			if (bploffset < 0xffff) {
				uae_u16 idx = (bploffset - be->ce_offset) & fetchstart_mask;
				uae_s8 *cd = be->cycle_diagram;
				int v = cd[idx];
				if (v > 0) {
					return true;
				}
				return false;
			}
		}
		i--;
		if (i < 0) {
			i += MAX_BPL_ESTIMATES;
		}
	}
	return false;
}
#endif

bool blitter_cant_access(int hpos)
{
	if (!is_blitter_dma()) {
		return true;
	}
	// bitplane dma check
	int coffset = RGA_PIPELINE_OFFSET_BLITTER;
	if (bitplane_dma_access(hpos, coffset)) {
		return true;
	}
	// other DMA channel check
	int offset = get_rga_pipeline(hpos, coffset);
	if (cycle_line_pipe[offset] != 0 || blitter_pipe[offset] != 0) {
		return true;
	}
	// static cycles are not in cycle_line_pipe
	uae_u8 v = cycle_line_slot[offset];
	if (v == CYCLE_REFRESH || v == CYCLE_STROBE) {
		return true;
	}
	// DMAL is not in cycle_line_pipe
	if (!dmal_alloc_mask || (hpos & 1) != ((DMAL_FIRST_HPOS - RGA_PIPELINE_OFFSET_BLITTER) & 1)) {
		return false;
	}
	int dmaloffset = hpos - (DMAL_FIRST_HPOS - RGA_PIPELINE_OFFSET_BLITTER);
	// 3 disk + 4 audio
	if (dmaloffset >= 0 && dmaloffset < 3 * 2 + 4 * 2) {
		if (dmal_alloc_mask & (3 << dmaloffset)) {
			return true;
		}
	}
	return false;
}

static bool copper_cant_read(int hpos, uae_u16 alloc)
{
	if (!is_copper_dma(true)) {
		return true;
	}

	int coffset = RGA_PIPELINE_OFFSET_COPPER;
	if (hpos == maxhposm1 && maxhposeven == COPPER_CYCLE_POLARITY) {
		// if copper used last cycle of scanline and it is even cycle and
		// it wants next available copper cycle:
		// next scanline's cycles 1 and 2 gets allocated.
		// cycle 1 is not used and also not usable by CPU or blitter.
		// cycle 2 is used by the copper.
		int offset = get_rga_pipeline(hpos, coffset);
		if (alloc && !bitplane_dma_access(hpos, coffset) && !cycle_line_pipe[offset]) {
			cycle_line_pipe[offset] = CYCLE_PIPE_NONE | CYCLE_PIPE_COPPER;
			if (currprefs.blitter_cycle_exact) {
				blitter_pipe[offset] = CYCLE_PIPE_COPPER;
			}
#ifdef DEBUGGER
			int dvpos = vpos + 1;
			if (is_last_line()) {
				dvpos = 0;
			}
			if (debug_dma) {
				record_dma_event2(DMA_EVENT2_COPPERUSE, offset, dvpos);
			}
#endif
		}
		coffset++;
	}

	if (bitplane_dma_access(hpos, coffset)) {
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event(DMA_EVENT_COPPERWANTED, hpos, vpos);
		}
#endif
		return true;
	}

	int offset = get_rga_pipeline(hpos, coffset);

	uae_u16 v = cycle_line_pipe[offset];
	if (v != 0 && !(v & CYCLE_PIPE_COPPER)) {
#if CYCLE_CONFLICT_LOGGING
		if ((v & CYCLE_PIPE_BLITTER) || (v & CYCLE_PIPE_CPUSTEAL) || (v & CYCLE_PIPE_SPRITE)) {
			write_log(_T("Copper's cycle stolen by lower priority channel %04x!?\n"), v);
		}
#endif
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event(DMA_EVENT_COPPERWANTED, hpos, vpos);
		}
#endif
		return true;
	}

	if (alloc) {
		cycle_line_pipe[offset] = alloc;
		// Keep copper cycles, without it blitter would
		// think copper cycles are free cycles because they
		// are cleared after copper has processed them
		if (currprefs.blitter_cycle_exact) {
			blitter_pipe[offset] = CYCLE_PIPE_COPPER;
		}
	}

	return false;
}

static int custom_wput_copper(int hpos, uaecptr pt, uaecptr addr, uae_u32 value, int noget)
{
	int v;

#ifdef DEBUGGER
	value = debug_putpeekdma_chipset(0xdff000 + addr, value, MW_MASK_COPPER, 0x08c);
#endif
	copper_access = 1;
	v = custom_wput_1(hpos, addr, value, noget);
	copper_access = 0;
	return v;
}

static void bprun_start(int hpos)
{
	bpl_hstart = hpos;
	fetch_cycle = 0;
	estimate_last_fetch_cycle(hpos);
}

static void decide_line(int endhpos)
{
	int hpos = last_decide_line_hpos;
	if (hpos >= endhpos) {
		return;
	}

	bool ecs = ecs_agnus;

	while (hpos < endhpos) {

		// Do copper DMA first because we need to know
		// new DDFSTRT etc values before bitplane decisions
		if (cop_state.movedelay > 0) {
			cop_state.movedelay--;
			if (cop_state.movedelay == 0) {
				custom_wput_copper(hpos, cop_state.moveptr, cop_state.moveaddr, cop_state.movedata, 0);
			}
		}
		uae_u16 datreg = cycle_line_pipe[hpos];
		if (datreg & CYCLE_PIPE_COPPER) {
			cycle_line_pipe[hpos] = 0;
			do_copper_fetch(hpos, datreg);
		}

		bool dma = dmacon_bpl;
		bool diw = vdiwstate_bpl;

		if (ecs) {
			// ECS/AGA

			// BPRUN latched: on
			if (bprun < 0 && (hpos & 1)) {
				decide_line_decision_fetches(hpos);
				bprun = 1;
				bprun_pipeline_flush_delay = maxhpos;
				bprun_cycle = 0;
				scandoubler_bpl_dma_start();
			}

			// BPRUN latched: off
			if (bprun == 3) {
				decide_line_decision_fetches(hpos);
				if (ddf_stopping == 1) {
					// If bpl sequencer counter was all ones (last cycle of block): ddf passed jumps to last step.
					if (islastbplseq()) {
						ddf_stopping = 2;
					}
				}
				bprun = 0;
				bprun_end = hpos;
				plfstrt_sprite = 0x100;
				bprun_pipeline_flush_delay = maxhpos;
				end_estimate_last_fetch_cycle(hpos);
			}

			// DDFSTRT == DDFSTOP: BPRUN gets enabled and DDF passed state in next cycle.
			if (ddf_enable_on < 0) {
				ddf_enable_on = 0;
				if (bprun && !ddf_stopping) {
					decide_line_decision_fetches(hpos);
					ddf_stopping = 1;
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event(DMA_EVENT_DDFSTOP2, hpos, vpos);
					}
#endif
				}
			}

			// Hard start limit
			if (hpos == (0x18 | 0)) {
				ddf_limit = false;
			}

			// DDFSTRT
			if (hpos == (plfstrt | 0) && hpos != ddfstrt_hpos) {
				ddf_enable_on = 1;
			}

			// Hard stop limit
			if (hpos == (0xd7 + 0)) {
				// Triggers DDFSTOP condition if hard limits are not disabled.
				ddf_limit = true;
				if (bprun && !ddf_stopping) {
					if (!harddis_h) {
						decide_line_decision_fetches(hpos);
						ddf_stopping = 1;
#ifdef DEBUGGER
						if (debug_dma) {
							record_dma_event(DMA_EVENT_DDFSTOP2, hpos, vpos);
						}
#endif
					}
				}
			}

			// DDFSTOP
			// Triggers DDFSTOP condition.
			// Clears DDF allowed flag.
			if ((hpos == plfstop && hpos != ddfstop_hpos) || (hpos == plfstop_prev && hpos == ddfstop_hpos)) {
				if (bprun && !ddf_stopping) {
					decide_line_decision_fetches(hpos);
					ddf_stopping = 1;
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event(DMA_EVENT_DDFSTOP, hpos, vpos);
					}
#endif
				}
				if (plfstop != plfstrt) {
					if (ddf_enable_on) {
						ddf_enable_on = -1;
					} else {
						ddf_enable_on = 0;
					}
				}
			}

			// BPRUN can only start if DMA, DIW or DDF state has changed since last time
			if (!(hpos & 1)) {
				bool hwi = dma && diw && ddf_enable_on > 0 && (!ddf_limit || harddis_h);
				if (!bprun && dma && diw && hwi && !hwi_old) {
					decide_line_decision_fetches(hpos);
					// Bitplane sequencer activated
					bprun = -1;
					plfstrt_sprite = std::min(plfstrt_sprite, hpos + 1);
					bprun_start(hpos);
					if (ddf_stopping) {
						bprun_pipeline_flush_delay = maxhpos;
						SET_LINE_CYCLEBASED(hpos);
					}
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event(DMA_EVENT_DDFSTRT, hpos, vpos);
					}
#endif
				}

				hwi_old = hwi;
			}

			if (bprun == 2) {
				bprun = 3;
				// If DDF has passed, jumps to last step.
				// (For example Scoopex Crash landing crack intro)
				if (ddf_stopping == 1) {
					ddf_stopping = 2;
				} else if (ddf_stopping == 0) {
					// If DDF has not passed, set it as passed.
					ddf_stopping = 1;
				}
			}

			// DIW or DMA switched off: clear BPRUN
			if ((!dma || !diw) && bprun == 1) {
				decide_line_decision_fetches(hpos);
				bprun = 2;
				if (ddf_stopping == 1) {
					ddf_stopping = 2;
					bprun = 3;
				}
				SET_LINE_CYCLEBASED(hpos);
			}

		} else {
			// OCS

			// BPRUN latched: On
			if (bprun < 0 && (hpos & 1)) {
				decide_line_decision_fetches(hpos);
				bprun = 1;
				bprun_pipeline_flush_delay = maxhpos;
				bprun_cycle = 0;
				scandoubler_bpl_dma_start();
			}
			// BPRUN latched: off
			if (bprun == 3) {
				decide_line_decision_fetches(hpos);
				bprun = 0;
				bprun_end = hpos;
				plfstrt_sprite = 0x100;
				bprun_pipeline_flush_delay = maxhpos;
				end_estimate_last_fetch_cycle(hpos);
			}

			// Hard start limit
			if (hpos == 0x18) {
				ddf_limit = false;
			}

			// DDFSTOP
			// Triggers DDFSTOP condition.
			if ((hpos == plfstop && hpos != ddfstop_hpos) || (hpos == plfstop_prev && hpos == ddfstop_hpos)) {
				if (bprun && !ddf_stopping) {
					decide_line_decision_fetches(hpos);
					ddf_stopping = 1;
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event(DMA_EVENT_DDFSTOP, hpos, vpos);
					}
#endif
				}
			}

			// Hard stop limit
			// Triggers DDFSTOP condition. Last cycle of bitplane DMA resets DDFSTRT limit.
			if (hpos == (0xd7 + 0)) {
				if (bprun && !ddf_stopping) {
					decide_line_decision_fetches(hpos);
					ddf_stopping = 1;
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event(DMA_EVENT_DDFSTOP2, hpos, vpos);
					}
#endif
				}
			}

			// DDFSTRT
			if (hpos == (plfstrt | 0) && hpos != ddfstrt_hpos) {
				ddfstrt_match = true;
			} else {
				ddfstrt_match = false;
			}

			if (!ddf_limit && ddfstrt_match && !bprun && dma && diw) {
				decide_line_decision_fetches(hpos);
				// Bitplane sequencer activated
				bprun = -1;
				plfstrt_sprite = std::min(plfstrt_sprite, hpos + 0);
				bprun_start(hpos);
				if (ddf_stopping) {
					bprun_pipeline_flush_delay = maxhpos;
					SET_LINE_CYCLEBASED(hpos);
				}
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_DDFSTRT, hpos, vpos);
				}
#endif
			}

			if (bprun == 2) {
				// If DDF has passed, jumps to last step.
				// (For example Scoopex Crash landing crack intro)
				if (ddf_stopping == 1) {
					ddf_stopping = 2;
				}
				bprun = 3;
			}

			// DMA or DIW off: clear BPRUN
			if ((!dma || !diw) && bprun == 1) {
				decide_line_decision_fetches(hpos);
				bprun = 2;
				if (ddf_stopping == 1) {
					ddf_stopping = 2;
					bprun = 3;
				}
				SET_LINE_CYCLEBASED(hpos);
			}

		}

		if (line_cyclebased) {
			decide_bpl_fetch(hpos);
		}

		hpos++;
	}

	decide_sprites_fetch(endhpos);

	last_decide_line_hpos = endhpos;
}

static void setstrobecopip(void)
{
	if (cop_state.strobe == 3) {
		cop_state.ip = cop1lc | cop2lc;
	} else if (cop_state.strobe == 1) {
		cop_state.ip = cop1lc;
	} else {
		cop_state.ip = cop2lc;
	}
	cop_state.strobe = 0;
}

/*
	CPU write COPJMP wakeup sequence when copper is waiting:
	- Idle cycle (can be used by other DMA channel)
	- Read word from current copper pointer (next word after wait instruction) to 1FE
	  This cycle can conflict with blitter DMA.
	Normal copper cycles resume
	- Write word from new copper pointer to 8C
*/

static void do_copper_fetch(int hpos, uae_u16 id)
{
	if (scandoubled_line) {
		return;
	}

	if (id & CYCLE_PIPE_NONE) {
		alloc_cycle(hpos, CYCLE_COPPER);
		return;
	}

	switch (cop_state.state)
	{
	case COP_strobe_delay1:
	case COP_strobe_delay3:
	{
		// fake MOVE phase 1
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x8c, cop_state.ip, hpos, vpos, DMARECORD_COPPER, 0);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c, cop_state.last_strobe == 2 ? 0x84 : 0x80);
		}
#endif
		cop_state.ir[0] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[0]);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_value(cop_state.ir[0]);
		}
#endif
		cop_state.ip += 2;
		if (cop_state.state == COP_strobe_delay3) {
			cop_state.state = COP_strobe_delay5;
			setstrobecopip();
		} else {
			cop_state.state = COP_strobe_delay2;
		}
		alloc_cycle(hpos, CYCLE_COPPER);
		break;
	}
	case COP_strobe_delay2:
	case COP_strobe_delay4:
	{
		// fake MOVE phase 2
#ifdef DEBUGGER
		uae_u16 reg = 0x8c; // COPINS
		if (debug_dma) {
			record_dma_read(reg, cop_state.ip, hpos, vpos, DMARECORD_COPPER, 0);
		}
#endif
		cop_state.ir[1] = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[1]);
			record_dma_event2(DMA_EVENT2_COPPERUSE, hpos, vpos);
		}
#endif
		if (cop_state.state == COP_strobe_delay4) {
			cop_state.state = COP_strobe_delay5;
		} else {
			cop_state.state = COP_read1;
		}
		// Next cycle finally reads from new pointer
		setstrobecopip();
		alloc_cycle(hpos, CYCLE_COPPER);
	}
	break;
	case COP_strobe_delay5:
	{
		// COPJMP when previous instruction is mid-cycle
		cop_state.state = COP_read1;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event2(DMA_EVENT2_COPPERUSE, hpos, vpos);
		}
#endif
		alloc_cycle(hpos, CYCLE_COPPER);
	}
	break;
	case COP_strobe_delay2x:
#ifdef DEBUGGER
		{
			if (debug_dma) {
				record_dma_read(0x1fe, cop_state.ip, hpos, vpos, DMARECORD_COPPER, 2);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x1fe, 0x1fe);
			}
			uae_u16 v = chipmem_wget_indirect(cop_state.ip);
			if (debug_dma) {
				record_dma_read_value(v);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(v);
			}
		}
#endif
		cop_state.state = COP_read1;
		// Next cycle finally reads from new pointer
		setstrobecopip();
		alloc_cycle(hpos, CYCLE_COPPER);
		break;
	case COP_start_delay:
		cop_state.state = COP_read1;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x1fe, cop_state.ip, hpos, vpos, DMARECORD_COPPER, 2);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x1fe, 0x1fe);
		}
#endif
		cop_state.ir[0] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[0]);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_value(cop_state.ir[0]);
		}
#endif
		alloc_cycle(hpos, CYCLE_COPPER);
		cop_state.ip = cop1lc;
		break;
	case COP_read1:
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x8c, cop_state.ip, hpos, vpos, DMARECORD_COPPER, 0);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c, cop_state.last_strobe == 2 ? 0x84 : 0x80);
		}
#endif
		cop_state.ir[0] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[0]);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_value(cop_state.ir[0]);
		}
#endif
		alloc_cycle(hpos, CYCLE_COPPER);
		cop_state.ip += 2;
		cop_state.state = COP_read2;
		break;
	case COP_read2:
		if (cop_state.ir[0] & 1) {
			// WAIT or SKIP
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(0x8c, cop_state.ip, hpos, vpos, DMARECORD_COPPER, (cop_state.ir[0] & 1) ? 1 : 0);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c, cop_state.last_strobe == 2 ? 0x84 : 0x80);
			}
#endif
			cop_state.ir[1] = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read_value(cop_state.ir[1]);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(cop_state.ir[1]);
			}
#endif
			alloc_cycle(hpos, CYCLE_COPPER);
			cop_state.ip += 2;

#ifdef DEBUGGER
			uaecptr debugip = cop_state.ip;
#endif
			cop_state.ignore_next = 0;
			if (cop_state.ir[1] & 1) {
				cop_state.state = COP_skip_in2;
			} else {
				cop_state.state = COP_wait_in2;
			}

			cop_state.vcmp = (cop_state.ir[0] & (cop_state.ir[1] | 0x8000)) >> 8;
			cop_state.hcmp = (cop_state.ir[0] & cop_state.ir[1] & 0xFE);

#ifdef DEBUGGER
			record_copper(debugip - 4, debugip, cop_state.ir[0], cop_state.ir[1], hpos, vpos);
#endif

		} else {
			// MOVE
			uae_u16 reg = cop_state.ir[0] & 0x1FE;

#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(reg, cop_state.ip, hpos, vpos, DMARECORD_COPPER, 0);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c, cop_state.last_strobe == 2 ? 0x84 : 0x80);
			}
#endif
			cop_state.ir[1] = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read_value(cop_state.ir[1]);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(cop_state.ir[1]);
			}
#endif
			alloc_cycle(hpos, CYCLE_COPPER);
			cop_state.ip += 2;

#ifdef DEBUGGER
			uaecptr debugip = cop_state.ip;
#endif
			uae_u16 data = cop_state.ir[1];
			cop_state.state = COP_read1;

			test_copper_dangerous(reg);
			// was "dangerous" register -> copper stopped
			if (!copper_enabled_thisline)
				return;

			// Previous instruction was SKIP that skipped
			if (cop_state.ignore_next > 0) {
				reg = 0x1fe;
			}

			if (reg == 0x88) {
				cop_state.strobe = 1;
				cop_state.last_strobe = 1;
				cop_state.state = COP_strobe_delay1;
			} else if (reg == 0x8a) {
				cop_state.strobe = 2;
				cop_state.last_strobe = 2;
				cop_state.state = COP_strobe_delay1;
			} else {
				if (reg == 0x100) {
					// BPLCON0 new value is needed early
					bplcon0_denise_change_early(hpos, data);
#if 1
					cop_state.moveaddr = reg;
					cop_state.movedata = data;
					cop_state.movedelay = 1;
					cop_state.moveptr = cop_state.ip;
#else
					custom_wput_copper(hpos, cop_state.ip, reg, data, 0);
#endif
				} else {
					custom_wput_copper(hpos, cop_state.ip, reg, data, 0);
				}
			}
#ifdef DEBUGGER
			if (debug_copper && cop_state.ignore_next <= 0) {
				uaecptr next = 0xffffffff;
				if (reg == 0x88) {
					next = cop1lc;
				} else if (reg == 0x8a) {
					next = cop2lc;
				}
				record_copper(debugip - 4, next, cop_state.ir[0], cop_state.ir[1], hpos, vpos);
			}
#endif
			cop_state.ignore_next = 0;
		}
		regs.chipset_latch_rw = last_custom_value = cop_state.ir[1];
		check_copper_stop();
		break;

	case COP_strobe_extra:
		// do nothing, happens if CPU wrote to COPxJMP but copper had already requested DMA cycle
		// Cycle will complete but nothing will happen because COPxJMP write resets copper state.
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x8c, cop_state.ip, hpos, vpos, DMARECORD_COPPER, 0);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c, cop_state.last_strobe == 2 ? 0x84 : 0x80);
		}
#endif
		alloc_cycle(hpos, CYCLE_COPPER);
		break;
	default:
		write_log(_T("copper_fetch invalid state %d! %02x\n"), cop_state.state, id);
		break;
	}
}

static int coppercomp(int hpos, bool blitwait)
{
	int hpos_cmp = hpos;
	int vpos_cmp = vpos;

	// If waiting for last cycle of line and last cycle is even cycle:
	// Horizontal counter has already wrapped around to zero.
	if (hpos_cmp == maxhposm1 && maxhposeven == COPPER_CYCLE_POLARITY) {
		hpos_cmp = 0;
	}

	int vp = vpos_cmp & (((cop_state.ir[1] >> 8) & 0x7F) | 0x80);
	int hp = hpos_cmp & (cop_state.ir[1] & 0xFE);

	if (vp < cop_state.vcmp) {
		return -1;
	}

	// Cycle 0: copper can't wake up
	if (hpos == 0) {
		return 1;
	}

	if ((cop_state.ir[1] & 0x8000) == 0) {
		decide_blitter(hpos);
		if (blit_busy(-1, false)) {
			if (blitwait) {
				/* We need to wait for the blitter.  */
				cop_state.state = COP_bltwait;
				return -1;
			}
			return 1;
		}
	}

	if (vp > cop_state.vcmp) {
		return 0;
	}
	if (vp == cop_state.vcmp && hp >= cop_state.hcmp) {
		return 0;
	}

	return 1;
}

static void update_copper(int until_hpos)
{
	if (1 && (custom_disabled || !copper_enabled_thisline)) {
		last_copper_hpos = until_hpos;
		return;
	}

	int hpos = last_copper_hpos;
	while (hpos < until_hpos) {

		int hp = hpos + 1;
		// So we know about the fetch state.
		decide_line(hp);
		// bitplane only, don't want blitter to steal our cycles.
		decide_bpl_fetch(hp);

		// Handle copper DMA here if no bitplanes enabled.
		// NOTE: can use odd cycles if DMA request was done during last cycle of line and it was even cycle (always in PAL).
		// request cycle 226 (even), request always completes in 2 cycles = cycle 1 (odd).
		// pipelined copper DMA read?
		bool copper_dma = (cycle_line_pipe[hpos] & CYCLE_PIPE_COPPER) != 0;
		if (copper_dma) {
			uae_u16 v = cycle_line_pipe[hpos];
			cycle_line_pipe[hpos] = 0;
			do_copper_fetch(hpos, v);
		}

		if (!copper_enabled_thisline) {
			goto next;
		}

		if ((hpos & 1) != COPPER_CYCLE_POLARITY) {
			goto next;
		}

		// cycle 0 is only available if previous line ends to odd cycle
		if (hpos == 0 && !maxhposeven_prev && cop_state.state != COP_start_delay) {
			goto next;
		}

#if CYCLE_CONFLICT_LOGGING
		{
			uae_u8 c = cycle_line_slot[hpos] & CYCLE_MASK;
			if (c && c != CYCLE_BITPLANE && c != CYCLE_COPPER) {
				write_log(_T("Only bitplanes has higher priority can copper. Cycle conflict %d!!\n"), c);
			}
		}
#endif

		switch (cop_state.state)
		{
		case COP_strobe_delay1:
			// First cycle after COPJMP. This is the strange one.
			// This cycle does not need to be free
			// But it still gets allocated by copper if it is free = CPU and blitter can't use it.
			if (copper_cant_read(hpos, 0)) {
				cop_state.state = COP_strobe_delay2;
			} else {
				if (hpos == maxhposm1 && maxhposeven == COPPER_CYCLE_POLARITY) {
					// if COP_strobe_delay2 would cross scanlines, it will be skipped!
					cop_state.state = COP_read1;
					setstrobecopip();
				} else {
					copper_cant_read(hpos, CYCLE_PIPE_COPPER | 0x08);
				}
			}
			break;
		case COP_strobe_delay2:
		case COP_strobe_delay3:
		case COP_strobe_delay4:
		case COP_strobe_delay5:
			// Second cycle after COPJMP does basically skipped MOVE (MOVE to 1FE)
			// Cycle is used and needs to be free.
			copper_cant_read(hpos, CYCLE_PIPE_COPPER);
			break;

		case COP_strobe_delay1x:
			// First cycle after CPU write to COPJMP while Copper was waiting.
			// Cycle can be free and copper won't allocate it.
			if (copper_cant_read(hpos, 0)) {
				// becomes normal non-buggy cycle if cycle was not free
				cop_state.state = COP_strobe_delay2;
			} else {
				cop_state.state = COP_strobe_delay2x;
			}
			break;
		case COP_strobe_delay2x:
			// Second cycle fetches following word and tosses it away.
			// Cycle can be free and copper won't allocate it.
			// If Blitter uses this cycle = Copper's new PC gets copied to blitter DMA pointer..
			if (!copper_cant_read(hpos, CYCLE_PIPE_COPPER | 0x09)) {
				copper_bad_cycle = get_cycles();
				if (copper_bad_cycle - copper_bad_cycle_start != 3 * CYCLE_UNIT) {
					copper_bad_cycle = 0;
				} else {
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event(DMA_EVENT_SPECIAL, hpos, vpos);
					}
#endif
					// early COPJMP processing
					cop_state.state = COP_read1;
					copper_bad_cycle_pc_old = cop_state.ip;
					setstrobecopip();
					copper_bad_cycle_pc_new = cop_state.ip;
				}
			}
			break;

		case COP_start_delay:
			// cycle after vblank strobe fetches word from old pointer first
			copper_cant_read(hpos, CYCLE_PIPE_COPPER | 0x01);
			break;

			// Request IR1
		case COP_read1:
			copper_cant_read(hpos, CYCLE_PIPE_COPPER | 0x02);
			break;

			// Request IR2
		case COP_read2:
			copper_cant_read(hpos, CYCLE_PIPE_COPPER | 0x03);
			break;

			// WAIT: Got IR2, first idle cycle.
			// Need free cycle, cycle not allocated.
		case COP_wait_in2:
			{
				if (copper_cant_read(hpos, 0)) {
					goto next;
				}
				cop_state.state = COP_wait1;
			}
			break;

			// WAIT: Second idle cycle. Wait until comparison matches.
			// Need free cycle, cycle not allocated.
		case COP_wait1:
			{
				int comp = coppercomp(hpos, true);
				if (comp < 0) {
					// If we need to wait for later scanline or blitter: no need to emulate copper cycle-by-cycle
					if (cop_state.ir[0] == 0xFFFF && cop_state.ir[1] == 0xFFFE && maxhpos < 250) {
						cop_state.state = COP_waitforever;
					}
					copper_enabled_thisline = 0;
					unset_special(SPCFLAG_COPPER);
					goto next;
				}

				if (comp) {
					goto next;
				}

				if (copper_cant_read(hpos, 0)) {
					goto next;
				}

				cop_state.state = COP_wait;
			}
			break;

			// Wait finished, request IR1.
		case COP_wait:
			{
				if (copper_cant_read(hpos, CYCLE_PIPE_COPPER | 0x04)) {
					goto next;
				}
#ifdef DEBUGGER
				if (debug_dma)
					record_dma_event(DMA_EVENT_COPPERWAKE, hpos, vpos);
				if (debug_copper)
					record_copper(cop_state.ip - 4, 0xffffffff, cop_state.ir[0], cop_state.ir[1], hpos, vpos);
#endif
				cop_state.state = COP_read1;
			}
			break;

			// SKIP: Got IR2. First idle cycle.
			// Free cycle needed, cycle not allocated.
		case COP_skip_in2:

			if (copper_cant_read(hpos, 0)) {
				goto next;
			}
			cop_state.state = COP_skip1;
			break;

			// SKIP: Second idle cycle. Do nothing.
			// Free cycle needed, cycle not allocated.
		case COP_skip1:

			if (copper_cant_read(hpos, 0)) {
				goto next;
			}

			cop_state.state = COP_skip;
			break;

			// Check comparison. SKIP finished. Request IR1.
		case COP_skip:
			if (copper_cant_read(hpos, CYCLE_PIPE_COPPER | 0x005)) {
				goto next;
			}

			if (!coppercomp(hpos, false)) {
				cop_state.ignore_next = 1;
			} else {
				cop_state.ignore_next = -1;
			}

#ifdef DEBUGGER
			if (debug_dma && cop_state.ignore_next > 0)
				record_dma_event(DMA_EVENT_COPPERSKIP, hpos, vpos);
			if (debug_copper)
				record_copper(cop_state.ip - 4, 0xffffffff, cop_state.ir[0], cop_state.ir[1], hpos, vpos);
#endif

			cop_state.state = COP_read1;
			break;

		default:
			break;
		}
next:
		if (copper_enabled_thisline) {
			switch (cop_state.state)
			{
			case COP_strobe_extra:
				// Wait 1 copper cycle doing nothing
				cop_state.state = COP_strobe_delay1;
				break;
			}
		}

		hpos++;
		last_copper_hpos = hpos;
	}
}

static void compute_spcflag_copper(void)
{
	copper_enabled_thisline = 0;
	unset_special(SPCFLAG_COPPER);
	if (!is_copper_dma(true) || cop_state.state == COP_stop || cop_state.state == COP_waitforever || cop_state.state == COP_bltwait || cop_state.state == COP_bltwait2 || custom_disabled)
		return;
	if (cop_state.state == COP_wait1 && is_copper_dma(false)) {
		int vp = vpos & (((cop_state.ir[1] >> 8) & 0x7F) | 0x80);
		if (vp < cop_state.vcmp) {
			return;
		}
	}
	last_copper_hpos = current_hpos();
	if (issyncstopped_count <= 2) {
		copper_enabled_thisline = 1;
		set_special(SPCFLAG_COPPER);
	}
}

static void blitter_done_notify_wakeup(uae_u32 temp)
{
	if (cop_state.state != COP_bltwait2) {
		return;
	}
	// blitter_done_notify() might be called too early, wait a bit if blitter is still busy.
	if (blit_busy(-1, false)) {
		event2_newevent_xx(-1, 1 * CYCLE_UNIT, 0, blitter_done_notify_wakeup);
		return;
	}

	cop_state.state = COP_wait1;
	compute_spcflag_copper();
#ifdef DEBUGGER
	if (copper_enabled_thisline) {
		int hpos = current_hpos();
		if (debug_dma) {
			record_dma_event(DMA_EVENT_COPPERWAKE, hpos, vpos);
		}
		if (debug_copper) {
			record_copper_blitwait(cop_state.ip - 4, hpos, vpos);
		}
	}
#endif
}

void blitter_done_notify(int blitline)
{
	if (cop_state.state != COP_bltwait) {
		return;
	}
	cop_state.state = COP_bltwait2;
	event2_newevent_xx(-1, 1 * CYCLE_UNIT, 0, blitter_done_notify_wakeup);
}

static void sync_copper(int hpos)
{
	if (copper_enabled_thisline) {
		update_copper(hpos);
	}
}

static void cursorsprite(void)
{
	if (!dmaen(DMA_SPRITE) || first_planes_vpos == 0) {
		return;
	}
	struct sprite *s = &spr[0];
	sprite_0 = s->pt;
	sprite_0_height = s->vstop - s->vstart;
	sprite_0_colors[0] = 0;
	sprite_0_doubled = 0;
	if (sprres == 0) {
		sprite_0_doubled = 1;
	}
	if (spr[0].dblscan) {
		sprite_0_height /= 2;
	}
	if (aga_mode) {
		int sbasecol = ((bplcon4 >> 4) & 15) << 4;
		sprite_0_colors[1] = current_colors.color_regs_aga[sbasecol + 1] & 0xffffff;
		sprite_0_colors[2] = current_colors.color_regs_aga[sbasecol + 2] & 0xffffff;
		sprite_0_colors[3] = current_colors.color_regs_aga[sbasecol + 3] & 0xffffff;
	} else {
		sprite_0_colors[1] = xcolors[current_colors.color_regs_ecs[17] & 0xfff];
		sprite_0_colors[2] = xcolors[current_colors.color_regs_ecs[18] & 0xfff];
		sprite_0_colors[3] = xcolors[current_colors.color_regs_ecs[19] & 0xfff];
	}
	sprite_0_width = sprite_width;
	if (currprefs.input_tablet && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		if (currprefs.input_magic_mouse_cursor == MAGICMOUSE_HOST_ONLY && mousehack_alive()) {
			magic_sprite_mask &= ~1;
		} else {
			magic_sprite_mask |= 1;
		}
	}
}

static uae_u16 sprite_fetch(struct sprite *s, uaecptr pt, int hpos, int slot, int mode)
{
	uae_u16 data = regs.chipset_latch_rw;

#if CYCLE_CONFLICT_LOGGING
	if ((hpos & 1) != (SPR_FIRST_HPOS & 1)) {
		int num = s - &spr[0];
		write_log(_T("Sprite %d, hpos %d wrong cycle polarity!\n"), num, hpos);
	}
#endif
#ifdef DEBUGGER
	int num = addrdiff(s, &spr[0]);
	if (debug_dma) {
		record_dma_read(num * 8 + 0x140 + mode * 4 + slot * 2, pt, hpos, vpos, DMARECORD_SPRITE, num);
	}
	if (memwatch_enabled) {
		debug_getpeekdma_chipram(pt, MW_MASK_SPR_0 << num, num * 8 + 0x140 + mode * 4 + slot * 2, num * 4 + 0x120);
	}
#endif
	data = regs.chipset_latch_rw = chipmem_wget_indirect(pt);
	alloc_cycle(hpos, CYCLE_SPRITE);
	return data;
}

static void sprite_fetch_full(struct sprite *s, int hpos, int slot, int mode, uae_u16 *v0, uae_u32 *v1, uae_u32 *v2)
{
	uae_u32 data321 = 0, data322 = 0;
	uae_u16 data16;

	if (sprite_width == 16) {

		data16 = sprite_fetch(s, s->pt, hpos, slot, mode);

#ifdef DEBUGGER
		if (memwatch_enabled) {
			debug_getpeekdma_value(data16);
		}
		if (debug_dma) {
			record_dma_read_value(data16);
		}
#endif

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
		data321 = sprite_fetch(s, pm1, hpos, slot, mode) << 16;
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

#ifdef DEBUGGER
		if (memwatch_enabled) {
			debug_getpeekdma_value_long(data321, pm1 - s->pt);
			debug_getpeekdma_value_long(data322, pm2 - s->pt);
		}
		if (debug_dma) {
			record_dma_read_value_wide((((uae_u64)data321) << 32) | data322, true);
		}
#endif

		s->pt += 8;

	} else { // 32

		uaecptr pm = s->pt & ~3;
		data321 = sprite_fetch(s, pm, hpos, slot, mode) << 16;
		data321 |= chipmem_wget_indirect(pm + 2);
		if (s->pt & 2) {
			data321 &= 0x0000ffff;
			data321 |= data321 << 16;
		} else if (fetchmode_fmode_spr & 2) {
			data321 &= 0xffff0000;
			data321 |= data321 >> 16;
		}
		data16 = data321 >> 16;

#ifdef DEBUGGER
		if (memwatch_enabled) {
			debug_getpeekdma_value_long(data321, pm - s->pt);
		}
		if (debug_dma) {
			record_dma_read_value_wide(data321, true);
		}
#endif

		s->pt += 4;
	}

	*v0 = data16;
	*v1 = data321;
	*v2 = data322;
}

static void do_sprite_fetch(int hpos, uae_u16 dat)
{
	int num = dat & 7;
	struct sprite *s = &spr[num];
	uae_u32 data321, data322;
	uae_u16 data;
	bool slot = (dat & 8) != 0;
	bool dmastate = (dat & 0x10) != 0;

	// render sprites first
	decide_sprites(hpos, true);

	sprite_fetch_full(s, hpos, slot, dmastate, &data, &data321, &data322);

	if (dmastate) {
		if (!slot) {
			SPRxDATA_1(data, num, hpos);
		} else {
			SPRxDATB_1(data, num, hpos);
		}
#ifdef AGA
		switch (sprite_width)
		{
			case 64:
			if (!slot) {
				s->data[1] = data321;
				s->data[2] = data322 >> 16;
				s->data[3] = data322;
			} else {
				s->datb[1] = data321;
				s->datb[2] = data322 >> 16;
				s->datb[3] = data322;
			}
			break;
			case 32:
			if (!slot) {
				s->data[1] = data321;
				s->data[2] = data;
				s->data[3] = data321;
			} else {
				s->datb[1] = data321;
				s->datb[2] = data;
				s->datb[3] = data321;
			}
			break;
		}
#endif
	} else {
		if (!slot) {
			SPRxPOS_1(data, num, hpos);
		} else {
			SPRxCTL_1(data, num, hpos);
			// This is needed to disarm previous field's sprite.
			// It can be seen on OCS Agnus + ECS Denise combination where
			// this cycle is disabled due to weird DDFTSTR=$18 copper list
			// which causes corrupted sprite to "wrap around" the display.
			s->dmastate = 0;
			sprstartstop(s);
		}
		// Sprite can't start if SPRxPOS/CTL DMA line matched on line after VBLANK ended
		if (vb_end_line || vb_end_next_line) {
			s->dmastate = 0;
			s->dmacycle = 0;
		}
	}

}

static void sprite_stolen_cycle(uae_u32 num)
{
	uae_u32 v = regs.chipset_latch_rw;
	struct sprite *s = &spr[num & 7];
	if (num & 0x100) {
		s->datb[0] = v;
		s->datb[1] = v >> 16;
	}
}

static void decide_sprites_fetch(int endhpos)
{
	int hpos = last_decide_sprite_hpos;
	if (hpos >= endhpos) {
		return;
	}
	if (vb_state || (doflickerfix_active() && (next_lineno & 1))) {
		last_decide_sprite_hpos = endhpos;
		return;
	}

	while (hpos < endhpos) {
		if (hpos >= SPR_FIRST_HPOS - RGA_SPRITE_PIPELINE_DEPTH && hpos < SPR_FIRST_HPOS + MAX_SPRITES * 4) {

			if (hpos < SPR_FIRST_HPOS + MAX_SPRITES * 4 - RGA_SPRITE_PIPELINE_DEPTH) {
				int num = (hpos - (SPR_FIRST_HPOS - RGA_SPRITE_PIPELINE_DEPTH)) / 4;
				int slot = (hpos - (SPR_FIRST_HPOS - RGA_SPRITE_PIPELINE_DEPTH)) & 3;
				if (slot == 0 || slot == 2) {
					struct sprite *s = &spr[num];
					if (slot == 0) {
						s->firstslotdone = false;
						if (!s->dmacycle && s->dmastate) {
							s->dmacycle = 1;
						}
						if (vpos == s->vstart && !vb_end_line && !vb_end_next_line) {
							s->dmastate = 1;
							s->dmacycle = 1;
							if (num == 0 && slot == 0) {
								cursorsprite();
							}
						}
						if (vpos == s->vstop || vb_end_next_line) {
							s->dmastate = 0;
							s->dmacycle = 1;
						}
					}
					if (dmaen(DMA_SPRITE) && s->dmacycle && !vb_start_line && !vb_end_line) {
						bool dodma = false;

						decide_bpl_fetch(hpos + 1);

						if (hpos <= plfstrt_sprite) {
							dodma = true;
						} else {
							// bitplane stole this cycle
							if (slot == 2 && sprite_width > 16 && s->firstslotdone) {
								event2_newevent_xx(-1, RGA_PIPELINE_OFFSET_SPRITE * CYCLE_UNIT, num | (s->dmastate ? 0x100 : 0), sprite_stolen_cycle);
							}
						}
#ifdef AGA
						if (dodma && s->dblscan && (fmode & 0x8000) && (vpos & 1) != (s->vstart & 1) && s->dmastate) {
							spr_arm(num, 1);
							dodma = false;
						}
#endif
						if (dodma) {
							int offset = get_rga_pipeline(hpos, RGA_PIPELINE_OFFSET_SPRITE);
							uae_u8 dat = CYCLE_PIPE_SPRITE | (s->dmastate ? 0x10 : 0x00) | (s->dmacycle == 1 ? 0 : 8) | num;
#if 0
							if (cycle_line_pipe[offset]) {
								write_log(_T("sprite cycle already allocated! %02x\n"), cycle_line_pipe[offset]);
							}
#endif
							evt_t t = line_start_cycles + hpos * CYCLE_UNIT;
							if (t == sprite_dma_change_cycle_on) {
								// if sprite DMA is switched on just when sprite DMA is decided, channel is still decided but it is not allocated!
								sprbplconflict2 = 0x140 + num * 8 + slot + (s->dmacycle ? 4 : 0);
								sprbplconflict_hpos = sprbplconflict_hpos2 = hpos + RGA_PIPELINE_OFFSET_SPRITE;
								sprbplconflict_dat = dat & ~CYCLE_PIPE_SPRITE;
							} else if (bprun_end == hpos || (cycle_line_pipe[offset] & CYCLE_PIPE_BITPLANE)) {
								// last bitplane cycle is available for sprites (if bitplane ends before all sprites)
								sprbplconflict = 0x140 + num * 8 + slot + (s->dmacycle ? 4 : 0);
								sprbplconflict_hpos = hpos + RGA_PIPELINE_OFFSET_SPRITE;
								sprbplconflict_dat = dat & ~CYCLE_PIPE_SPRITE;
							} else {
								cycle_line_pipe[offset] = dat;
							}
							s->firstslotdone = true;
						}
					}
					if (!vb_end_line && s->dmacycle) {
						s->dmacycle++;
						if (s->dmacycle > 2) {
							s->dmacycle = 0;
							if (s->dmastate) {
								s->dmacycle = 1;
							}
						}
					}
				}
			}

			bool sprite_dma = (cycle_line_pipe[hpos] & CYCLE_PIPE_SPRITE) != 0;
			if (sprite_dma) {
				uae_u16 dat = cycle_line_pipe[hpos];
				do_sprite_fetch(hpos, dat);
			}
			if (hpos == sprbplconflict_hpos2) {
				decide_bpl_fetch(hpos + 1);
				decide_blitter(hpos + 1);
				int num = sprbplconflict_dat & 7;
				struct sprite *s = &spr[num];
#ifdef DEBUGGER
				int slot = (sprbplconflict_dat & 8) != 0;
				int mode = (sprbplconflict_dat & 0x10) != 0;
				if (debug_dma) {
					record_dma_read(num * 8 + 0x140 + mode * 4 + slot * 2, s->pt, hpos, vpos, DMARECORD_SPRITE, num);
					record_dma_event(DMA_EVENT_SPECIAL, hpos, vpos);
				}
				if (memwatch_enabled) {
					debug_getpeekdma_chipram(s->pt, MW_MASK_SPR_0 << num, num * 8 + 0x140 + mode * 4 + slot * 2, num * 4 + 0x120);
				}
#endif
				if (hpos == sprbplconflict_hpos2) {
					s->pt += sprite_width / 8;
				}
			}
		}
		hpos++;
	}
	last_decide_sprite_hpos = endhpos;
}

static void init_sprites(void)
{
	for (int i = 0; i < MAX_SPRITES; i++) {
		struct sprite *s = &spr[i];
		memset(s, 0, sizeof(struct sprite));
	}
}

// line=0
static void init_hardware_frame(void)
{
	if (!harddis_v) {
		vdiwstate = diw_states::DIW_waiting_start;
		vdiw_change(0);
	}
}

static int calculate_lineno(int vp)
{
	int lineno = vp;
	if (lineno >= MAXVPOS) {
		lineno %= MAXVPOS;
	}
	nextline_how = nln_normal;
	if (doflickerfix_active()) {
		lineno *= 2;
		lineno++;
	} else if (!interlace_seen && doublescan <= 0 && currprefs.gfx_vresolution && currprefs.gfx_pscanlines > 1) {
		lineno *= 2;
		if (display_vsync_counter & 1) {
			lineno++;
			nextline_how = currprefs.gfx_pscanlines == 3 ? nln_lower_black_always : nln_lower_black;
		} else {
			nextline_how = currprefs.gfx_pscanlines == 3 ? nln_upper_black_always : nln_upper_black;
		}
	} else if ((doublescan <= 0 || interlace_seen > 0) && currprefs.gfx_vresolution && currprefs.gfx_iscanlines) {
		lineno *= 2;
		if (interlace_seen) {
			if (!lof_display) {
				lineno++;
				nextline_how = currprefs.gfx_iscanlines == 2 ? nln_lower_black_always : nln_lower_black;
			} else {
				nextline_how = currprefs.gfx_iscanlines == 2 ? nln_upper_black_always : nln_upper_black;
			}
		} else {
			nextline_how = currprefs.gfx_vresolution > VRES_NONDOUBLE && currprefs.gfx_pscanlines == 1 ? nln_nblack : nln_doubled;
		}
	} else if (currprefs.gfx_vresolution && (doublescan <= 0 || interlace_seen > 0)) {
		lineno *= 2;
		if (interlace_seen) {
			if (!lof_display) {
				lineno++;
				nextline_how = nln_lower;
			} else {
				nextline_how = nln_upper;
			}
		} else {
			nextline_how = currprefs.gfx_vresolution > VRES_NONDOUBLE && currprefs.gfx_pscanlines == 1 ? nln_nblack : nln_doubled;
		}
	}
	return lineno;
}

// vsync start
void init_hardware_for_drawing_frame(void)
{
	/* Avoid this code in the first frame after a customreset.  */
	if (prev_sprite_entries) {
		int first_pixel = prev_sprite_entries[0].first_pixel;
		int npixels = prev_sprite_entries[prev_next_sprite_entry].first_pixel - first_pixel;
		if (npixels > 0) {
			memset(spixels + first_pixel, 0, npixels * sizeof *spixels);
			memset(spixstate.stb + first_pixel, 0, npixels * sizeof *spixstate.stb);
			if (aga_mode) {
				memset(spixstate.stbfm + first_pixel, 0, npixels * sizeof *spixstate.stbfm);
			}
		}
	}

	prev_next_sprite_entry = next_sprite_entry;
	next_sprite_entry = 0;
	end_sprite_entry = MAX_SPRITE_ENTRIES - 2;
	spixels_max = sizeof(spixels) / sizeof(*spixels) - MAX_PIXELS_PER_LINE;

	linear_vpos = vpos;
	next_lineno = calculate_lineno(linear_vpos);
	last_color_change = 0;
	next_color_change = 0;
	next_color_entry = 0;
	remembered_color_entry = -1;

	prev_sprite_entries = sprite_entries[current_change_set];
	curr_sprite_entries = sprite_entries[current_change_set ^ 1];
	prev_color_changes = color_changes[current_change_set];
	curr_color_changes = color_changes[current_change_set ^ 1];
	prev_color_tables = color_tables[current_change_set];
	curr_color_tables = color_tables[current_change_set ^ 1];

	prev_drawinfo = line_drawinfo[current_change_set];
	curr_drawinfo = line_drawinfo[current_change_set ^ 1];
	current_change_set ^= 1;

	color_src_match = color_dest_match = -1;

	/* Use both halves of the array in alternating fashion.  */
	curr_sprite_entries[0].first_pixel = current_change_set * MAX_SPR_PIXELS;
	next_sprite_forced = 1;
}

static frame_time_t rpt_vsync(int adjust)
{
	frame_time_t curr_time = read_processor_time();
	frame_time_t v = curr_time - vsyncwaittime + adjust;
	if (v > syncbase || v < -syncbase) {
		vsyncmintime = vsyncmaxtime = vsyncwaittime = curr_time;
		v = 0;
	}
	return v;
}

static void rtg_vsync (void)
{
#ifdef PICASSO96
	frame_time_t start, end;
	start = read_processor_time();
	picasso_handle_vsync();
	end = read_processor_time();
	frameskiptime += end - start;
#endif
}

static void rtg_vsynccheck(void)
{
#if 0
	if (vblank_found_rtg) {
		vblank_found_rtg = false;
		rtg_vsync();
	}
#endif
}


static void maybe_process_pull_audio(void)
{
	audio_finish_pull();
}

static bool crender_screen(int monid, int mode, bool immediate)
{
	bool v = render_screen(monid, mode, immediate);
	if (display_reset > 0) {
		display_reset--;
	}
	return v;
}

// moving average algorithm
#define MAVG_MAX_SIZE 128
struct mavg_data
{
	frame_time_t values[MAVG_MAX_SIZE];
	int size;
	int offset;
	frame_time_t mavg;
};

static void mavg_clear (struct mavg_data *md)
{
	md->size = 0;
	md->offset = 0;
	md->mavg = 0;
}

static frame_time_t mavg(struct mavg_data *md, frame_time_t newval, int size)
{
	if (md->size < size) {
		md->values[md->size++] = newval;
		md->mavg += newval;
	} else {
		md->mavg -= md->values[md->offset];
		md->values[md->offset] = newval;
		md->mavg += newval;
		md->offset++;
		if (md->offset >= size) {
			md->offset -= size;
		}
	}
	return md->mavg / md->size;
}

#define MAVG_VSYNC_SIZE 128
#ifdef DEBUGGER
extern int log_vsync, debug_vsync_min_delay, debug_vsync_forced_delay;
#endif
static bool framewait(void)
{
	struct amigadisplay *ad = &adisplays[0];
	frame_time_t curr_time;
	frame_time_t start;
	int vs = isvsync_chipset();
	int status = 0;

	events_reset_syncline();

	static struct mavg_data ma_frameskipt;
	frame_time_t frameskipt_avg = mavg(&ma_frameskipt, frameskiptime, MAVG_VSYNC_SIZE);

	frameskiptime = 0;

	if (vs > 0) {

		static struct mavg_data ma_legacy;
		static frame_time_t vsync_time;
		frame_time_t t;

		curr_time = read_processor_time();
		vsyncwaittime = vsyncmaxtime = curr_time + vsynctimebase;
		if (!frame_rendered && !ad->picasso_on) {
			frame_rendered = crender_screen(0, 1, false);
		}

		start = read_processor_time();
		t = 0;
		if (start - vsync_time >= 0 && start - vsync_time < vsynctimebase) {
			t += start - vsync_time;
		}

		if (!frame_shown) {
			show_screen(0, 1);
			if (currprefs.gfx_apmode[0].gfx_strobo) {
				show_screen(0, 4);
			}
		}

		maybe_process_pull_audio();

		frame_time_t legacy_avg = mavg(&ma_legacy, t, MAVG_VSYNC_SIZE);
		legacy_avg = std::max(t, legacy_avg);
		t = legacy_avg;

#ifdef DEBUGGER
		if (debug_vsync_min_delay && t < debug_vsync_min_delay * vsynctimebase / 100) {
			t = debug_vsync_min_delay * vsynctimebase / 100;
		}
		if (debug_vsync_forced_delay > 0) {
			t = debug_vsync_forced_delay * vsynctimebase / 100;
		}
#endif

		vsync_time = read_processor_time();
		t = std::min(t, vsynctimebase * 2 / 3);

		if (currprefs.m68k_speed < 0) {
			vsynctimeperline = (vsynctimebase - t) / (maxvpos_display + 1);
		} else {
			vsynctimeperline = (vsynctimebase - t) / 3;
		}

		vsynctimeperline = std::max<frame_time_t>(vsynctimeperline, 1);

#ifdef DEBUGGER
		if (0 || (log_vsync & 2)) {
			write_log (_T("%06d %06d/%06d %03d%%\n"), t, vsynctimeperline, vsynctimebase, t * 100 / vsynctimebase);
		}
#endif

		frame_shown = true;
		return 1;

	} else if (vs < 0) {

		if (!vblank_hz_state) {
			return status != 0;
		}

		frame_shown = true;
		status = 1;
		return status != 0;
	}

	status = 1;

	int clockadjust = 0;
	frame_time_t vstb = vsynctimebase;

	if (currprefs.m68k_speed < 0 && !cpu_sleepmode && !currprefs.cpu_memory_cycle_exact) {

		if (!frame_rendered && !ad->picasso_on)
			frame_rendered = crender_screen(0, 1, false);

		if (currprefs.m68k_speed_throttle) {
			// this delay can safely overshoot frame time by 1-2 ms, following code will compensate for it.
			for (;;) {
				curr_time = read_processor_time();
				if (vsyncwaittime - curr_time <= 0 || vsyncwaittime - curr_time > 2 * vsynctimebase) {
					break;
				}
				//rtg_vsynccheck (); //no-op
				if (cpu_sleep_millis(1) < 0) {
					curr_time = read_processor_time();
					break;
				}
			}
		} else {
			curr_time = read_processor_time();
		}

		int max;
		frame_time_t adjust = 0;
		if (curr_time - vsyncwaittime > 0 && curr_time - vsyncwaittime < vstb / 2) {
			adjust += curr_time - vsyncwaittime;
		}
		adjust += clockadjust;
		max = (int)(vstb * (1000.0 + currprefs.m68k_speed_throttle) / 1000.0 - adjust);
		vsyncwaittime = curr_time + vstb - adjust;
		vsyncmintime = curr_time;

		if (max < 0) {
			max = 0;
			vsynctimeperline = 1;
		} else {
			vsynctimeperline = max / (maxvpos_display + 1);
		}
		vsyncmaxtime = curr_time + max;

		if (0)
			write_log (_T("%06d:%06d/%06d %d %d\n"), adjust, vsynctimeperline, vstb, max, maxvpos_display);
	
	} else {

		frame_time_t t = 0;

		start = read_processor_time();
		if (!frame_rendered && !ad->picasso_on) {
			frame_rendered = crender_screen(0, 1, false);
			t = read_processor_time() - start;
		}
		if (!currprefs.cpu_thread) {
			while (!currprefs.turbo_emulation) {
				float v = rpt_vsync(clockadjust) / (syncbase / 1000.0f);
				if (v >= -FRAMEWAIT_MIN_MS)
					break;
				//rtg_vsynccheck(); //no-op
				maybe_process_pull_audio();
				if (cpu_sleep_millis(1) < 0)
					break;
			}
			while (rpt_vsync(clockadjust) < 0) {
				//rtg_vsynccheck(); //no-op
				if (audio_is_pull_event()) {
					maybe_process_pull_audio();
					break;
				}
			}
		}
		evt_t tnow = read_processor_time();
		idletime += tnow - start;
		curr_time = tnow;
		vsyncmintime = curr_time;
		vsyncmaxtime = vsyncwaittime = curr_time + vstb;
		if (frame_rendered) {
			show_screen(0, 0);
			t += read_processor_time() - curr_time;
		}
		t += frameskipt_avg;

		vsynctimeperline = (vstb - t) / FRAMEWAIT_SPLIT;
		if (vsynctimeperline < 1) {
			vsynctimeperline = 1;
		} else if (vsynctimeperline > vstb / FRAMEWAIT_SPLIT) {
			vsynctimeperline = vstb / FRAMEWAIT_SPLIT;
		}

		frame_shown = true;

	}
	return status != 0;
}

static void reset_cpu_idle(void)
{
	cpu_sleepmode_cnt = 0;
	if (cpu_sleepmode) {
		cpu_sleepmode = 0;
		//write_log(_T("woken\n"));
	}
}

#define FPSCOUNTER_MAVG_SIZE 10
static struct mavg_data fps_mavg, idle_mavg;

void fpscounter_reset(void)
{
	mavg_clear(&fps_mavg);
	mavg_clear(&idle_mavg);
	bogusframe = 2;
	lastframetime = read_processor_time();
	idletime = 0;
}

static void fpscounter(bool frameok)
{
	frame_time_t now, last;

	now = read_processor_time();
	last = now - lastframetime;
	lastframetime = now;

	if (bogusframe || last < 0) {
		return;
	}

	mavg(&fps_mavg, last / 10, FPSCOUNTER_MAVG_SIZE);
	mavg(&idle_mavg, idletime / 10, FPSCOUNTER_MAVG_SIZE);
	idletime = 0;

	frametime += last;
	timeframes++;

	if ((timeframes & 7) == 0) {
		double idle = 1000 - (idle_mavg.mavg == 0 ? 0.0 : (double)idle_mavg.mavg * 1000.0 / vsynctimebase);
		int fps = fps_mavg.mavg == 0 ? 0 : (int)(syncbase * 10 / fps_mavg.mavg);
		fps = std::min(fps, 99999);
		idle = std::max<double>(idle, 0);
		idle = std::min<double>(idle, 100 * 10);
		if (fake_vblank_hz * 10 > fps) {
			double mult = (double)fake_vblank_hz * 10.0 / fps;
			idle *= mult;
		}
		if (currprefs.turbo_emulation && idle < 100 * 10)
			idle = 100 * 10;
		gui_data.fps = fps;
		gui_data.idle = (int)idle;
		gui_data.fps_color = nosignal_status == 1 ? 2 : (nosignal_status == 2 ? 3 : (frameok ? 0 : 1));
		if ((timeframes & 15) == 0) {
			gui_fps(fps, gui_data.lines, gui_data.lace, (int)idle, gui_data.fps_color);
		}
	}
}

// vsync functions that are not hardware timing related
// called when vsync starts which is not necessarily last line
// it can line 0 or even later.
static void vsync_handler_render(void)
{
	struct amigadisplay *ad = &adisplays[0];

#if 1
	if (currprefs.m68k_speed < 0) {
		if (regs.stopped) {
			if (cpu_last_stop_vpos >= 0) {
				cpu_stopped_lines += maxvpos - cpu_last_stop_vpos;
			} else {
				cpu_stopped_lines = 0;
			}
		}
		int mv = 12 - currprefs.cpu_idle / 15;
		if (mv >= 1 && mv <= 11) {
			mv = 11 - mv;
			if (cpu_stopped_lines >= maxvpos * (mv * 10) / 100) {
				cpu_sleepmode_cnt++;
				if (cpu_sleepmode_cnt >= 50) {
					cpu_sleepmode_cnt = 50;
					if (!cpu_sleepmode) {
						cpu_sleepmode = 1;
						//write_log(_T("sleep\n"));
					}
				}
			} else {
				reset_cpu_idle();
			}
		} else {
			reset_cpu_idle();
		}
	}
	if (regs.halted < 0) {
		reset_cpu_idle();
	}
	cpu_last_stop_vpos = 0;
	cpu_stopped_lines = 0;
#endif

#ifdef PICASSO96
	if (isvsync_rtg() >= 0) {
		rtg_vsync();
	}
#endif

	if (!vsync_rendered) {
		frame_time_t start, end;
		start = read_processor_time();
		vsync_handle_redraw(lof_store, lof_changed, bplcon0, bplcon3, isvsync_chipset() >= 0, initial_frame);
		initial_frame = false;
		vsync_rendered = true;
		end = read_processor_time();
		frameskiptime += end - start;
	}

	linear_vpos = vpos;
	next_lineno = calculate_lineno(linear_vpos);

	bool frameok = framewait();
	
	if (!ad->picasso_on) {
		if (!frame_rendered && vblank_hz_state) {
			frame_rendered = crender_screen(0, 1, false);
		}
		if (frame_rendered && !frame_shown) {
			frame_shown = show_screen_maybe(0, isvsync_chipset () >= 0);
		}
	}

	fpscounter(frameok);

	bool waspaused = false;
	while (handle_events()) {
		if (!waspaused) {
			if (crender_screen(0, 1, true)) {
				show_screen(0, 0);
			}
			waspaused = true;
		}
		// we are paused, do all config checks but don't do any emulation
		if (vsync_handle_check()) {
			redraw_frame();
			if (crender_screen(0, 1, true)) {
				show_screen(0, 0);
			}
		}
		config_check_vsync();
	}

	vsync_rendered = false;
	frame_shown = false;
	frame_rendered = false;

	if (quit_program > 0) {
		/* prevent possible infinite loop at wait_cycles().. */
		ad->framecnt = 0;
		reset_decisions_scanline_start();
		reset_decisions_hsync_start();
		return;
	}

	if (vblank_hz_mult > 0) {
		vblank_hz_state ^= 1;
	} else {
		vblank_hz_state = 1;
	}

	nextline_how = nln_normal;

	//checklacecount (bplcon0_interlace_seen || lof_lace);
}

static bool vsync_display_rendered;

static void vsync_display_render(void)
{
	if (!vsync_display_rendered) {
		vsyncmintimepre = read_processor_time();
		vsync_handler_render();
		vsync_display_rendered = true;
	}
}

static void vsync_check_vsyncmode(void)
{
	if (varsync_maybe_changed[0] == 1 || varsync_maybe_changed[1] == 1 || varhblank_lines == -1) {
		init_hz_normal();
	} else if (varsync_changed == 1 || ((beamcon0 & BEAMCON0_VARBEAMEN) && exthblank_prev != exthblank && (abs(exthblank_lines[0] - exthblank_lines[1]) > maxvpos * 3 / 4))) {
		init_hz_normal();
	} else if (vpos_count > 0 && abs(vpos_count - vpos_count_diff) > 1 && vposw_change && vposw_change < 4) {
		init_hz_vposw();
	} else if (interlace_changed || changed_chipset_refresh() || lof_changed) {
		compute_framesync();
		display_reset = 2;
	}
	if (varsync_changed > 0) {
		varsync_changed--;
	}
	varsync_maybe_changed[0] = 0;
	varsync_maybe_changed[1] = 0;
	varhblank_lines = 0;
	exthblank_lines[0] = exthblank_lines[1] = 0;
	exthblank_prev = exthblank;
}

static void check_display_mode_change(void)
{
	struct amigadisplay *ad = &adisplays[0];
	int vt, ht, hs, vs;

	if (new_beamcon0 & BEAMCON0_VARBEAMEN) {
		vt = vtotal;
		ht = htotal;
	} else {
		vt = maxvpos;
		ht = maxhpos;
	}
	if (new_beamcon0 & (BEAMCON0_VARHSYEN | BEAMCON0_VARCSYEN)) {
		hs = hsstrt;
	} else {
		hs = 18;
	}
	if (new_beamcon0 & (BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN)) {
		vs = vsstrt;
	} else {
		vs = (new_beamcon0 & BEAMCON0_PAL) ? 2 : 3;
	}

	// recalculate display if vtotal, htotal, hsync start or vsync start changed > 1
	if ((abs(vt - vt_old) > 1 || abs(ht - ht_old) > 1 || abs(hs - hs_old) > 1 || abs(vs - vs_old) > 1)  && vt_old && ht_old) {
		varsync_changed = 1;
		if (!ad->picasso_on) {
			nosignal_trigger = true;
			display_reset = 2;
		}
	}

	vt_old = vt;
	ht_old = ht;
	vs_old = vs;
	hs_old = hs;
}

// emulated hardware vsync
static void vsync_handler_post(void)
{
	int monid = 0;
	static frame_time_t prevtime;

	//write_log (_T("%d %d %d\n"), vsynctimebase, read_processor_time () - vsyncmintime, read_processor_time () - prevtime);
	prevtime = read_processor_time();

	check_nocustom();

#if CUSTOM_DEBUG > 1
	if ((intreq & 0x0020) && (intena & 0x0020))
		write_log(_T("vblank interrupt not cleared\n"));
#endif
	DISK_vsync ();

#ifdef WITH_LUA
	uae_lua_run_handler("on_uae_vsync");
#endif

	if (bplcon0 & 4) {
		lof_store = lof_store ? 0 : 1;
	}
	if ((bplcon0 & 2) && currprefs.genlock) {
		genlockvtoggle = lof_store ? 1 : 0;
	}

	if (issyncstopped()) {
		nosignal_trigger = true;
	}
	// Inverted CSYNC
	if ((beamcon0 & BEAMCON0_CSYTRUE) && currprefs.cs_hvcsync == 1) {
		nosignal_trigger = true;
	}
	// BLANKEN set: horizontal blanking is merged with CSYNC
	if ((beamcon0 & BEAMCON0_BLANKEN) && currprefs.cs_hvcsync == 1) {
		nosignal_trigger = true;
	}
	if ((beamcon0 & BEAMCON0_CSCBEN) && currprefs.cs_hvcsync == 2) {
		nosignal_trigger = true;
	}
	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		if (htotal < 50 || htotal > 250) {
			nosignal_trigger = true;
		}
		if (vtotal < 100 || vtotal > 1000) {
			nosignal_trigger = true;
		}
		// CSY output is invalid (no vsync part included) if HTOTAL is too small + hardwired CSYNC.
		int csyh = (beamcon0 & 0x20) ? 0x8c : 0x8d;
		if (htotal < csyh && !(beamcon0 & BEAMCON0_VARCSYEN) && currprefs.cs_hvcsync == 1) {
			nosignal_trigger = true;
		}
	}
	// Too small or large HSYNC
	if (beamcon0 & (BEAMCON0_VARHSYEN | BEAMCON0_VARCSYEN)) {
		if (hsstop < hsstrt) {
			hsstop += maxhpos;
		}
		if (hsstop - hsstrt <= 2 || hsstop - hsstrt > 80) {
			nosignal_trigger = true;
		}
	}
	// Too small or large VSYNC
	if (beamcon0 & (BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN)) {
		if (vsstop < vsstrt) {
			vsstop += maxvpos;
		}
		if (vsstop - vsstrt <= 1 || vsstop - vsstrt > 80) {
			nosignal_trigger = true;
		}
	}

	if (lof_prev_lastline != lof_lastline) {
		if (lof_togglecnt_lace < LOF_TOGGLES_NEEDED) {
			lof_togglecnt_lace++;
		}
		if (lof_togglecnt_lace >= LOF_TOGGLES_NEEDED) {
			lof_togglecnt_nlace = 0;
		}
	} else {
		// only 1-2 vblanks with bplcon0 lace bit set?
		// lets check if lof has changed
		if (!(bplcon0 & 4) && lof_togglecnt_lace > 0 && lof_togglecnt_lace < LOF_TOGGLES_NEEDED && !interlace_seen) {
			lof_changed = 1;
		}
		lof_togglecnt_nlace = LOF_TOGGLES_NEEDED;
		lof_togglecnt_lace = 0;
#if 0
		if (lof_togglecnt_nlace < LOF_TOGGLES_NEEDED)
			lof_togglecnt_nlace++;
		if (lof_togglecnt_nlace >= LOF_TOGGLES_NEEDED)
			lof_togglecnt_lace = 0;
#endif
	}
	lof_prev_lastline = lof_lastline;
	if (lof_togglecnt_lace >= LOF_TOGGLES_NEEDED) {
		interlace_changed = notice_interlace_seen(monid, true);
		if (interlace_changed) {
			notice_screen_contents_lost(monid);
		}
	} else if (lof_togglecnt_nlace >= LOF_TOGGLES_NEEDED) {
		interlace_changed = notice_interlace_seen(monid, false);
		if (interlace_changed) {
			notice_screen_contents_lost(monid);
		}
	}
	if (lof_changing) {
		// if not interlace and LOF was changed during previous field but LOF is now same as beginning of previous field: do nothing
		if (!(bplcon0 & 4) && ((lof_changing > 0 && prevlofs[0]) || (lof_changing < 0 && !prevlofs[0]))) {
			lof_changing = 0;
		}
	}
	if (lof_changing) {
		if (lof_store != (prevlofs[0] ? 1 : 0) && prevlofs[0] == prevlofs[1] && prevlofs[1] == prevlofs[2]) {
			// resync if LOF was not toggling previously
			nosignal_trigger = true;
		}

		// still same? Trigger change now.
		if ((!lof_store && lof_changing < 0) || (lof_store && lof_changing > 0)) {
			lof_changed_previous_field++;
			lof_changed = 1;
			// lof toggling? decide as interlace.
			if (lof_changed_previous_field >= LOF_TOGGLES_NEEDED) {
				lof_changed_previous_field = LOF_TOGGLES_NEEDED;
				if (lof_lace == false) {
					lof_lace = true;
				} else {
					lof_changed = 0;
				}
			}
			if (bplcon0 & 4) {
				lof_changed = 0;
			}
		}
		lof_changing = 0;
	} else {
		lof_changed_previous_field = 0;
		lof_lace = false;
	}

	prevlofs[2] = prevlofs[1];
	prevlofs[1] = prevlofs[0];
	prevlofs[0] = lof_store;

#ifdef DEBUGGER
	if (debug_copper) {
		record_copper_reset();
	}
	if (debug_dma) {
		record_dma_reset(0);
	}
#endif

#ifdef PICASSO96
	if (p96refresh_active) {
		vpos_count = p96refresh_active;
		vpos_count_diff = p96refresh_active;
		vtotal = vpos_count;
	}
#endif

	devices_vsync_post();

	check_display_mode_change();

	vsync_check_vsyncmode();

	if (bogusframe > 0) {
		bogusframe--;
	}

	nosignal_status = std::max(nosignal_status, 0);
	if (nosignal_cnt) {
		nosignal_cnt--;
		if (nosignal_cnt == 0) {
			nosignal_status = -1;
		}
	}
	if (nosignal_trigger) {
		struct amigadisplay *ad = &adisplays[0];
		nosignal_trigger = false;
		if (!ad->specialmonitoron) {
			if (currprefs.gfx_monitorblankdelay > 0) {
				nosignal_status = 1;
				nosignal_cnt = (int)(currprefs.gfx_monitorblankdelay / (1000.0f / vblank_hz));
				if (nosignal_cnt <= 0) {
					nosignal_cnt = 1;
				}
			} else {
				nosignal_status = 2;
				nosignal_cnt = (int)(vblank_hz / 2);
			}
		}
	}

	config_check_vsync();
	if (timehack_alive > 0) {
		timehack_alive--;
	}

	lof_changed = 0;
	vposw_change = 0;
	bplcon0_interlace_seen = false;

	vsync_handle_check();

	init_hardware_frame();

	vsync_cycles = get_cycles();
}

static void copper_check(int n)
{
	if (cop_state.state == COP_wait) {
		int vp = vpos & (((cop_state.ir[1] >> 8) & 0x7F) | 0x80);
		if (vp < cop_state.vcmp) {
			if (copper_enabled_thisline) {
				write_log(_T("COPPER BUG %d: vp=%d vpos=%d vcmp=%d thisline=%d\n"), n, vp, vpos, cop_state.vcmp, copper_enabled_thisline);
			}
		}
	}
}

static void reset_scandoubler_sync(int hpos)
{
	last_decide_line_hpos = hpos;
	last_decide_sprite_hpos = hpos;
	last_fetch_hpos = hpos;
	last_copper_hpos = hpos;
	last_diw_hpos = hpos;
	last_sprite_hpos = hpos;
	sprites_enabled_this_line = false;
	plfstrt_sprite = 0x100;
	bprun = 0;
	bprun_cycle = 0;
	ddf_stopping = 0;
	int hnew = hpos - (REFRESH_FIRST_HPOS - HARDWIRED_DMA_TRIGGER_HPOS + 2);
	hdiw_denisecounter = ((hnew + 0) & 0xff) << CCK_SHRES_SHIFT;
	last_sprite_hpos = last_sprite_point = (((hnew + 0) & 0xff) << 1) + 1;
}


/*

0 0 -
1 1 --
2 2 -
3 3 --
4 4 -
5 5 --

0 x -+
1 0 --
2 1 -
3 2 --
4 3 -
5 4 --

*/

static void hsync_scandoubler(int hpos)
{
	uae_u16 odmacon = dmacon;
	int ocop = copper_enabled_thisline;
	uaecptr bpltmp[MAX_PLANES]{}, bpltmpx[MAX_PLANES]{};
	int lof = lof_display;

	if (vb_start_line > 2) {
		return;
	}

	scandoubled_line = 1;
	line_disabled |= 8;
	last_hdiw = 0 - 1;

	for (int i = 0; i < MAX_PLANES; i++) {
		bpltmp[i] = bplpt[i];
		bpltmpx[i] = bplptx[i];
		uaecptr pb1 = prevbpl[lof][vpos][i];
		uaecptr pb2 = prevbpl[1 - lof][vpos][i];
		if (pb1 && pb2) {
			int diff = pb1 - pb2;
			if (lof) {
				if (bplcon0 & 4) {
					bplpt[i] = pb1 - diff;
				}
			} else {
				if (bplcon0 & 4) {
					bplpt[i] = pb1;
				} else {
					bplpt[i] = bplpt[i] - diff;
				}
			}
		}
	}

	uae_u8 cycle_line_slot_tmp[MAX_CHIPSETSLOTS];
	uae_u16 cycle_line_pipe_tmp[MAX_CHIPSETSLOTS];

	memcpy(cycle_line_slot_tmp, cycle_line_slot, sizeof(uae_u8) * MAX_CHIPSETSLOTS);
	memcpy(cycle_line_pipe_tmp, cycle_line_pipe, sizeof(uae_u16) * MAX_CHIPSETSLOTS);

	reset_decisions_scanline_start();
	reset_scandoubler_sync(hpos);
	reset_decisions_hsync_start();

	// Bitplane and sprites only
	dmacon = odmacon & (DMA_MASTER | DMA_BITPLANE | DMA_SPRITE);
	copper_enabled_thisline = 0;

	// copy color changes
	struct draw_info *dip1 = curr_drawinfo + next_lineno - 1;
	for (int idx1 = dip1->first_color_change; idx1 < dip1->last_color_change; idx1++) {
		struct color_change *cs2 = &curr_color_changes[idx1];
		struct color_change *cs1 = &curr_color_changes[next_color_change];
		memcpy(cs1, cs2, sizeof(struct color_change));
		next_color_change++;
	}
	curr_color_changes[next_color_change].regno = -1;

	hpos_hsync_extra = 0;
	finish_partial_decision(maxhpos);
	hpos_hsync_extra = maxhpos;
	finish_decisions(hpos);
	hsync_record_line_state(next_lineno, nln_normal, thisline_changed);

	scandoubled_line = 0;
	line_disabled &= ~8;

	dmacon = odmacon;
	copper_enabled_thisline = ocop;

	memcpy(cycle_line_slot, cycle_line_slot_tmp, sizeof(uae_u8) * MAX_CHIPSETSLOTS);
	memcpy(cycle_line_pipe, cycle_line_pipe_tmp, sizeof(uae_u16) * MAX_CHIPSETSLOTS);

	for (int i = 0; i < MAX_PLANES; i++) {
		bplpt[i] = bpltmp[i];
		bplptx[i] = bpltmpx[i];
	}

	reset_decisions_scanline_start();
	reset_scandoubler_sync(hpos);
}

static void dmal_emu(uae_u32 val)
{
	// Disk and Audio DMA bits are ignored by Agnus. Including DMA master bit.
	int hpos = current_hpos();

	if (currprefs.cpu_memory_cycle_exact) {
		// BPL conflict
		if (bitplane_dma_access(hpos, 0)) {
			return;
		}
	}
	int dmalbits = val & 3;
	int dmalpos = val >> 8;

	if (dmalpos >= 6 && dmalpos < 14) {
		dmalpos -= 6;
		int nr = dmalpos / 2;
		uaecptr pt = audio_getpt(nr, (dmalbits & 1) != 0);
		if (dmal_ce) {
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(0xaa + nr * 16, pt, hpos, vpos, DMARECORD_AUDIO, nr);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(pt, MW_MASK_AUDIO_0 << nr, 0xaa + nr * 16, 0xa0 + nr * 16);
			}
#endif
		}
		uae_u16 dat = chipmem_wget_indirect(pt);
		if (dmal_ce) {
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read_value(dat);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(dat);
			}
#endif
		}
		regs.chipset_latch_rw = last_custom_value = dat;
		AUDxDAT(nr, dat, pt);
	} else if (dmalpos >= 0 && dmalpos < 6) {
		uae_u16 dat = 0;
		int s = (dmalpos / 2);
		int w = (dmalbits & 3) == 3;
		// disk_fifostatus() needed in >100% disk speed modes
		if (w) {
			// write to disk
			if (disk_fifostatus() <= 0) {
				uaecptr pt = disk_getpt();
				if (dmal_ce) {
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_read(0x26, pt, hpos, vpos, DMARECORD_DISK, dmalpos / 2);
					}
					if (memwatch_enabled) {
						debug_getpeekdma_chipram(pt, MW_MASK_DISK, 0x26, 0x20);
					}
#endif
				}
				dat = chipmem_wget_indirect(pt);
				if (dmal_ce) {
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_read_value(dat);
					}
					if (memwatch_enabled) {
						debug_getpeekdma_value(dat);
					}
#endif
				}
				regs.chipset_latch_rw = last_custom_value = dat;
				DSKDAT(dat);
			}
		} else {
			// read from disk
			if (disk_fifostatus() >= 0) {
				uaecptr pt = disk_getpt();
				dat = DSKDATR(s);
				if (dmal_ce) {
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_write(0x08, dat, pt, hpos, vpos, DMARECORD_DISK, dmalpos / 2);
					}
					if (memwatch_enabled) {
						debug_putpeekdma_chipram(pt, dat, MW_MASK_DISK, 0x08, 0x20);
					}
#endif
				}
				chipmem_wput_indirect(pt, dat);
				regs.chipset_latch_rw = last_custom_value = dat;
			}
		}
	} else {
		write_log(_T("invalid DMAL position %d\n"), dmalpos);
	}
}

static void dmal_func(uae_u32 v)
{
	dmal_emu(v);
	events_dmal(0);
}

static void dmal_func2(uae_u32 v)
{
	while (dmal) {
		if (dmal & 3) {
			dmal_emu((dmal_hpos << 8) | (dmal & 3));
		}
		dmal_hpos += 2;
		dmal >>= 2;
	}
}

static void events_dmal(int hpos)
{
	dmal_ce = false;
	if (!dmal)
		return;
	if (currprefs.cachesize) {
		dmal_func2(0);
	} else if (currprefs.cpu_compatible) {
		while (dmal) {
			if (dmal & 3)
				break;
			hpos += 2;
			dmal >>= 2;
			dmal_hpos += 2;
		}
		dmal_ce = true;
		event2_newevent2(hpos, (dmal_hpos << 8) | (dmal & 3), dmal_func);
		dmal &= ~3;
	} else {
		event2_newevent2(hpos, 0, dmal_func2);
	}
}

static void events_dmal_hsync2(uae_u32 v)
{
	int dmal_first = DMAL_FIRST_HPOS - v;
	// 3 disk + 4 audio
	uae_u16 dmalt = audio_dmal();
	dmalt <<= (3 * 2);
	dmalt |= disk_dmal();
	dmalt &= dmal_htotal_mask;
	dmal |= dmalt;
	dmal_alloc_mask = dmal;
	if (!dmal) {
		return;
	}
	if (hstrobe_conflict) {
		return;
	}
	dmal_hpos = 0;
	if (currprefs.cpu_memory_cycle_exact) {
		for (int i = 0; i < 3 * 2 + 4 * 2; i += 2) {
			if (dmal & (3 << i)) {
				alloc_cycle_ext(DMAL_FIRST_HPOS + i, CYCLE_MISC);
			}
		}
	}
	events_dmal(dmal_first);
}

static void events_dmal_hsync(void)
{
	if (currprefs.cpu_compatible) {
		int delay = REFRESH_FIRST_HPOS + 1;
		event2_newevent2(delay, delay, events_dmal_hsync2);
	} else {
		events_dmal_hsync2(0);
	}
}

static void lightpen_trigger_func(uae_u32 v)
{
	vpos_lpen = vpos;
	hpos_lpen = v;
	hhpos_lpen = HHPOSR();
	lightpen_triggered = 1;
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

static bool do_render_slice(int mode, int slicecnt, int lastline)
{
	draw_lines(lastline, slicecnt);
	crender_screen(0, mode, true);
	return true;
}

static bool do_display_slice(void)
{
	show_screen(0, -1);
	inputdevice_hsync(true);
	return true;
}

static void reset_autoscale(void)
{
	first_bpl_vpos = -1;
	if (first_bplcon0 != first_bplcon0_old) {
		vertical_changed = horizontal_changed = true;
	}
	first_bplcon0_old = first_bplcon0;

	if (first_planes_vpos != first_planes_vpos_old ||
		last_planes_vpos != last_planes_vpos_old) {
		vertical_changed = true;
	}
	first_planes_vpos_old = first_planes_vpos;
	last_planes_vpos_old = last_planes_vpos;

	if (diwfirstword_total != diwfirstword_total_old ||
		diwlastword_total != diwlastword_total_old ||
		ddffirstword_total != ddffirstword_total_old ||
		ddflastword_total != ddflastword_total_old) {
		horizontal_changed = true;
	}
	diwfirstword_total_old = diwfirstword_total;
	diwlastword_total_old = diwlastword_total;
	ddffirstword_total_old = ddffirstword_total;
	ddflastword_total_old = ddflastword_total;

	first_planes_vpos = 0;
	last_planes_vpos = 0;
	diwfirstword_total = max_diwlastword;
	diwlastword_total = 0;
	ddffirstword_total = max_diwlastword;
	ddflastword_total = 0;
	plflastline_total = 0;
	plffirstline_total = current_maxvpos();
	first_bplcon0 = 0;
	autoscale_bordercolors = 0;
}

static void hautoscale_check(int vp)
{
	int vp_start = vp;
	int vp_end = vp + 1;
	// border detection/autoscale
	if (GET_PLANES(bplcon0) > 0 && dmaen(DMA_BITPLANE) && !vb_state && !vb_end_line && !vb_start_line) {
		if (first_bplcon0 == 0) {
			first_bplcon0 = bplcon0;
		}
		last_planes_vpos = std::max(vp_end, last_planes_vpos);
		if (vp_start >= minfirstline && first_planes_vpos == 0) {
			first_planes_vpos = vp_start;
		} else if (vp_end >= current_maxvpos() - 1) {
			last_planes_vpos = current_maxvpos();
		}
	}
	if (diw_change == 0) {
		if (vp_start >= first_planes_vpos && vp_end <= last_planes_vpos) {
			int diwfirstword_lores = diwfirstword;
			int diwlastword_lores = diwlastword;
			// >0x1c7? Calculate "real" right edge for scaling modes
			if (diwlastword_lores < min_diwlastword && diwfirstword_lores >= 2) {
				if (diwlastword_lores < diwfirstword_lores) {
					diwlastword_lores = max_diwlastword + (diwlastword_lores - 2);
				} else {
					diwlastword_lores = min_diwlastword;
				}
			}
			if (diwlastword_lores > diwlastword_total) {
				diwlastword_total = diwlastword_lores;
				diwlastword_total = std::min(diwlastword_total, coord_diw_shres_to_window_x(hsyncstartpos));
			}
			if (diwfirstword_lores < diwfirstword_total) {
				diwfirstword_total = diwfirstword_lores;
				diwfirstword_total = std::max(diwfirstword_total, coord_diw_shres_to_window_x(hsyncendpos));
				firstword_bplcon1 = bplcon1;
			}
		}
		if (vdiwstate == diw_states::DIW_waiting_stop) {
			int f = 8 << fetchmode;
			if (plfstrt + f < ddffirstword_total + f) {
				ddffirstword_total = plfstrt + f;
			}
			if (plfstop + 2 * f > ddflastword_total + 2 * f) {
				ddflastword_total = plfstop + 2 * f;
			}
		}
		if ((plffirstline < plffirstline_total || (plffirstline_total == minfirstline && vp_start > minfirstline)) && plffirstline < maxvpos / 2) {
			firstword_bplcon1 = bplcon1;
			if (plffirstline < minfirstline) {
				plffirstline_total = minfirstline;
			} else {
				plffirstline_total = plffirstline;
			}
		}
		if (plflastline > plflastline_total && plflastline > plffirstline_total && plflastline > maxvpos / 2) {
			plflastline_total = plflastline;
		}
	}
	if (diw_change > 0) {
		diw_change--;
	}
}

static void hsync_handlerh(bool onvsync)
{
	int hpos = current_hpos();

	if (!custom_disabled) {

		if (doflickerfix_active()) {
			finish_decisions(hpos);
			hsync_record_line_state(next_lineno, nextline_how, thisline_changed);
			next_lineno++;
			hsync_scandoubler(hpos);
		} else {
			finish_decisions(hpos);
			hsync_record_line_state(next_lineno, nextline_how, thisline_changed);
		}

		notice_resolution_seen(GET_RES_AGNUS(bplcon0), interlace_seen != 0);

		hautoscale_check(vposh);

		next_lineno = calculate_lineno(linear_vpos);
		reset_decisions_hsync_start();
	}

	linear_vpos++;
	if (vpos >= minfirstline && minfirstline_linear == minfirstline) {
		minfirstline_linear = linear_vpos;
	}
	vposh++;

	hpos_hsync_extra = 0;
	estimate_last_fetch_cycle(hpos);

	if (vb_end_next_line && !ecs_denise && currprefs.gfx_overscanmode > OVERSCANMODE_OVERSCAN) {
		event2_newevent_xx(-1, 2 * CYCLE_UNIT, 0, set_ocs_denise_blank);
	}

	eventtab[ev_hsynch].evtime = get_cycles() + HSYNCTIME;
	eventtab[ev_hsynch].active = 1;
	eventtab[ev_hsynch].oldcycles = get_cycles();
	events_schedule();
}

static void set_hpos(void)
{
	line_start_cycles = (get_cycles() + CYCLE_UNIT - 1) & ~(CYCLE_UNIT - 1);
	maxhposeven_prev = maxhposeven;
	maxhpos = maxhpos_short + lol;
	set_maxhpos(maxhpos);
	eventtab[ev_hsync].evtime = line_start_cycles + HSYNCTIME;
	eventtab[ev_hsync].oldcycles = line_start_cycles;
}

// this finishes current line
static void hsync_handler_pre(bool onvsync)
{
	if (!custom_disabled) {

		// make sure decisions are done to end of scanline
		finish_partial_decision(maxhpos);
		clear_bitplane_pipeline(0);

		/* reset light pen latch */
		if (vb_end_line) {
			lightpen_triggered = 0;
			sprite_0 = 0;
		}

		if (!lightpen_triggered && (bplcon0 & 8)) {
			// lightpen always triggers at the beginning of the last line
			if (vb_start_line == 1) {
				vpos_lpen = vpos;
				hpos_lpen = 1;
				hhpos_lpen = HHPOSR();
				lightpen_triggered = 1;
			} else if (lightpen_enabled && !vb_state) {
				int lpnum = inputdevice_get_lightpen_id();
				lpnum = std::max(lpnum, 0);
				if (lightpen_cx[lpnum] > 0 && lightpen_cy[lpnum] == vpos) {
					event2_newevent_xx(-1, lightpen_cx[lpnum] * CYCLE_UNIT, lightpen_cx[lpnum], lightpen_trigger_func);
				}
			}
		}

		hdiw_counter += maxhpos * 2;
		if (!hstrobe_conflict) {
			// OCS Denise freerunning horizontal counter
			if (!ecs_denise && vpos == get_equ_vblank_endline() - 1) {
				hdiw_counter++;
			}
			if (ecs_denise || vpos > get_equ_vblank_endline() || (agnusa1000 && vpos == 0)) {
				hdiw_counter = maxhpos * 2;
			}
		}
		hdiw_counter &= 511;
	}

	devices_hsync();

	if (hsyncdebug) {
		sync_color_changes(maxhpos);
	}

	hsync_counter++;

	int currentmaxhp = current_hpos();
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_hsync(currentmaxhp);
	}
#endif
	// just to be sure
	if (currentmaxhp > 0) {
		cycle_line_slot_last = cycle_line_slot[currentmaxhp - 1];
	} else {
		cycle_line_slot_last = 0;
	}
	set_hpos();

	vpos_prev = vpos;
	vpos++;
	vpos_count++;
	if (vpos >= maxvpos_total) {
		vpos = 0;
	}

	line_equ_freerun = !ecs_denise && vpos >= get_equ_vblank_startline() && vpos <= get_equ_vblank_endline();

	if (onvsync) {
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_vsync(vpos);
		}
#endif
		vpos = 0;
		vsync_counter++;
	}
	check_line_enabled();

	hpos_hsync_extra = maxhpos;

	lol_prev = lol;
	if (islinetoggle()) {
		lol = lol ? 0 : 1;
		estimated_cycles_next = estimated_cycles;
		if (maxhpos == estimated_maxhpos[0]) {
			estimated_cycles = estimated_cycles_buf0;
			estimated_cycles_next = estimated_cycles_buf1;
		} else if (maxhpos == estimated_maxhpos[1]) {
			estimated_cycles = estimated_cycles_buf1;
			estimated_cycles_next = estimated_cycles_buf0;
		}
		linetoggle = true;
	} else {
		lol = 0;
		linetoggle = false;
	}
	memset(cycle_line_slot, 0, maxhposm1 + 1);

	// to record decisions correctly between end of scanline and start of hsync
	if (!eventtab[ev_hsynch].active) {
		eventtab[ev_hsynch].evtime = get_cycles() + hsyncstartpos_start_cycles * CYCLE_UNIT;
		eventtab[ev_hsynch].active = 1;
		events_schedule();
	}

	for (int i = 0; i < 4; i++) {
		if (!(refresh_handled_slot & (1 << i))) {
#ifdef DEBUGGER
			if (debug_dma) {
				debug_mark_refreshed(refptr_p + i * ref_ras_add);
			}
#endif
			if (!refptr_preupdated) {
				refptr += ref_ras_add;
			}
		}
	}

#ifdef DEBUGGER
	debug_hsync();
#endif
}

// low latency vsync

#ifdef WITH_BEAMRACER
#define LLV_DEBUG 0

static bool sync_timeout_check(frame_time_t max)
{
#if LLV_DEBUG
	return true;
#else
	frame_time_t rpt = read_processor_time();
	return rpt - max <= 0;
#endif
}

extern int busywait;
static void scanlinesleep(int currline, int nextline)
{
	if (currline < 0)
		return;
	if (currline >= nextline)
		return;
	if (vsync_hblank) {
		int diff = (int)(vsync_hblank / (nextline - currline));
		int us = 1000000 / diff;
		if (us < target_sleep_nanos(-1)) { // spin if less than minimum sleep time
			target_spin(nextline - currline - 1);
			return;
		}
	}
	if (busywait) {
		target_spin(1);
		return;
	}
	if (currprefs.m68k_speed < 0)
		sleep_millis_main(1);
	else
		target_sleep_nanos(500);
}

static void linesync_first_last_line(int *first, int *last)
{
	int x, y, w, h;
	*first = minfirstline;
	*last = maxvpos_display;
	get_custom_raw_limits(&w, &h, &x, &y);
	if (y > 0)
		*first += y;
	//write_log(_T("y=%d h=%d mfl=%d maxvpos=%d\n"), y, h, minfirstline, maxvpos_display);
}

static int vsyncnextscanline;
static int vsyncnextscanline_add;
static int nextwaitvpos;
static int display_slice_cnt;
static int display_slice_lines;
static int display_slices;
static bool display_rendered;
static int vsyncnextstep;

static bool linesync_beam_single_dual(void)
{
	bool was_syncline = is_syncline != 0;
	frame_time_t maxtime = read_processor_time() + 2 * vsynctimebase;
	int vp;

	if (is_last_line()) {
		do_render_slice(-1, 0, vpos);
		while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
			maybe_process_pull_audio();
			target_spin(0);
			vp = target_get_display_scanline(-1);
			if (vp >= 0)
				break;
		}
		vsyncmintime = read_processor_time() + vsynctimebase;
		vsync_clear();
		while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
			maybe_process_pull_audio();
			vp = target_get_display_scanline(-1);
			if (vp >= vsync_activeheight - 1 || vp < 0)
				break;
			scanlinesleep(vp, vsync_activeheight - 1);
		}
		frame_rendered = true;
		frame_shown = true;
		do_display_slice();
		int vv = (int)vsync_vblank;
		while (vv >= 85) {
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				target_spin(0);
				vp = target_get_display_scanline(-1);
				if (vp < vsync_activeheight / 2)
					break;
			}
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				target_spin(0);
				vp = target_get_display_scanline(-1);
				if (vp >= vsync_activeheight / 2)
					break;
			}
			show_screen(0, 3);
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				target_spin(0);
				vp = target_get_display_scanline(-1);
				if (vp >= vsync_activeheight - 1 || vp < vsync_activeheight / 2)
					break;
			}
			show_screen(0, 2);
			vv -= currprefs.ntscmode ? 60 : 50;
		}
		return true;
	}
	return false;
}

static bool linesync_beam_single_single(void)
{
	bool was_syncline = is_syncline != 0;
	frame_time_t maxtime = read_processor_time() + 2 * vsynctimebase;
	int vp;

	if (is_last_line()) {
		if (vsyncnextstep != 1) {
			vsyncnextstep = 1;
			do_render_slice(-1, 0, vpos);
			// wait until out of vblank
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				vp = target_get_display_scanline(-1);
				if (vp >= 0 && vp < vsync_activeheight / 2)
					break;
				if (currprefs.m68k_speed < 0 && !was_syncline) {
					is_syncline = -3;
					return 0;
				}
				target_spin(0);
			}
		}
		if (vsyncnextstep != 2) {
			vsyncnextstep = 2;
			vsync_clear();
			// wait until second half of display
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				vp = target_get_display_scanline(-1);
				if (vp >= vsync_activeheight / 2)
					break;
				if (currprefs.m68k_speed < 0 && !was_syncline) {
					is_syncline = -2;
					is_syncline_end = vsync_activeheight - 1;
					return 0;
				}
				scanlinesleep(vp, vsync_activeheight / 2);
			}
		}
		if (vsyncnextstep != 3) {
			vsyncnextstep = 3;
			// wait until end of display (or start of next field)
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				vp = target_get_display_scanline(-1);
				if (vp >= vsync_activeheight - 1 || vp < vsync_activeheight / 2)
					break;
				if (currprefs.m68k_speed < 0 && !was_syncline) {
					is_syncline = -2;
					is_syncline_end = vsync_activeheight - 1;
					return 0;
				}
				scanlinesleep(vp, vsync_activeheight - 1);
			}
		}
		if (vsyncnextstep != 4) {
			vsyncnextstep = 4;
			do_display_slice();
			frame_rendered = true;
			frame_shown = true;
			// wait until first half of display
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				vp = target_get_display_scanline(-1);
				if (vp < vsync_activeheight / 2)
					break;
				if (currprefs.m68k_speed < 0 && !was_syncline) {
					is_syncline = -(100 + vsync_activeheight / 2);
					return 0;
				}
				target_spin(0);
			}
		}
		return true;
	}
	return false;
}

static bool linesync_beam_multi_dual(void)
{
	frame_time_t maxtime = read_processor_time() + 2 * vsynctimebase;
	bool input_read_done = false;
	bool was_syncline = is_syncline != 0;

	events_reset_syncline();
	if (vpos == 0 && !was_syncline) {
		int firstline, lastline;
		linesync_first_last_line(&firstline, &lastline);

		display_slices = currprefs.gfx_display_sections;
		if (display_slices <= 0)
			display_slices = 1;
		display_slice_cnt = 0;
		vsyncnextscanline = vsync_activeheight / display_slices + 1;
		vsyncnextscanline_add = vsync_activeheight / display_slices;
		display_slice_lines = (lastline - firstline) / display_slices + 1;
		nextwaitvpos = firstline + display_slice_lines + display_slice_lines / 2;
		if (display_slices <= 1)
			nextwaitvpos = lastline + 1;
		if (display_slices <= 2 && vsyncnextscanline > vsync_activeheight * 2 / 3)
			vsyncnextscanline = vsync_activeheight * 2 / 3;

		display_rendered = false;
		frame_rendered = true;
		frame_shown = true;
	}

	if (!display_slices)
		return false;

	if (vpos >= nextwaitvpos || is_last_line()) {

		if (display_slice_cnt == 0) {

			if (!was_syncline) {
				do_render_slice(is_last_line() ? 1 : 2, display_slice_cnt, vpos);
				display_rendered = true;
			}

			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				maybe_process_pull_audio();
				target_spin(0);
				int vp = target_get_display_scanline(-1);
				if (vp >= vsync_activeheight - 1 || vp < 0)
					break;
			}

			do_display_slice();
			display_rendered = false;
			input_read_done = true;

		} else {

			if (!currprefs.turbo_emulation) {
				if (!was_syncline && !display_rendered) {
					do_render_slice(0, display_slice_cnt, vpos - 1);
					display_rendered = true;
				}
				while(sync_timeout_check(maxtime)) {
					int vp = target_get_display_scanline(-1);
					if (vp == -1) {
						maybe_process_pull_audio();
						target_spin(0);
						continue;
					}
					if (vp < 0 || vp >= vsyncnextscanline)
						break;
					if (currprefs.m68k_speed < 0 && !was_syncline) {
						is_syncline = vsyncnextscanline;
						return 0;
					}
					maybe_process_pull_audio();
					scanlinesleep(vp, vsyncnextscanline);
				}
				do_display_slice();
			}

			if (is_last_line()) {
				// wait extra frames
				int vv = (int)vsync_vblank;
				for(;;) {
					while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
						maybe_process_pull_audio();
						target_spin(0);
						int vp = target_get_display_scanline(-1);
						if (vp >= vsync_activeheight - 1 || vp < 0)
							break;
					}
					show_screen(0, 3);
					show_screen(0, 2);
					while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
						maybe_process_pull_audio();
						target_spin(0);
						int vp = target_get_display_scanline(-1);
						if (vp >= vsync_activeheight / 2)
							break;
					}
					vv -= currprefs.ntscmode ? 60 : 50;
					if (vv < 85)
						break;

					while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
						maybe_process_pull_audio();
						target_spin(0);
						int vp = target_get_display_scanline(-1);
						if (vp < vsync_activeheight / 2 && vp >= 0)
							break;
					}
				}

			}

			input_read_done = true;
			display_rendered = false;

			vsyncnextscanline += vsync_activeheight / display_slices;
			vsync_clear();
		}

		nextwaitvpos += display_slice_lines;
		display_slice_cnt++;


	}
	return input_read_done;
}

static bool linesync_beam_multi_single(void)
{
	frame_time_t maxtime = read_processor_time() + 2 * vsynctimebase;
	bool input_read_done = false;
	bool was_syncline = is_syncline != 0;

	events_reset_syncline();
	if (vpos == 0 && !was_syncline) {
		int firstline, lastline;
		linesync_first_last_line(&firstline, &lastline);

		display_slices = currprefs.gfx_display_sections;
		if (!display_slices)
			display_slices = 1;
		display_slice_cnt = 0;
		vsyncnextscanline = vsync_activeheight / display_slices + 1;
		vsyncnextscanline_add = vsync_activeheight / display_slices;
		display_slice_lines = (lastline - firstline) / display_slices + 1;
		nextwaitvpos = firstline + display_slice_lines + display_slice_lines / 2;
		if (display_slices <= 1)
			nextwaitvpos = lastline + 1;
		if (display_slices <= 2 && vsyncnextscanline > vsync_activeheight * 2 / 3)
			vsyncnextscanline = vsync_activeheight * 2 / 3;

		display_rendered = false;
		frame_rendered = true;
		frame_shown = true;
	}

	if (!display_slices)
		return false;

	if (is_last_line()) {

		if (!was_syncline && !display_rendered) {
			do_render_slice(1, display_slice_cnt, vpos);
			display_rendered = true;
		}
		// if 2 slices: make sure we are out of vblank.
		if (display_slices <= 2) {
			while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
				int vp = target_get_display_scanline(-1);
				if (vp != -1)
					break;
				maybe_process_pull_audio();
				if (currprefs.m68k_speed < 0 && !was_syncline) {
					is_syncline = -3;
					return 0;
				}
				target_spin(0);
			}
			vsync_clear();
		}

		while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
			int vp = target_get_display_scanline(-1);
			if (vp < 0 || vp >= vsyncnextscanline)
				break;
			maybe_process_pull_audio();
			if (currprefs.m68k_speed < 0 && !was_syncline) {
				is_syncline = vsyncnextscanline;
				return 0;
			}
			scanlinesleep(vp, vsyncnextscanline);
#if LLV_DEBUG
			write_log(_T("1:%d:%d:%d:%d."), vpos, vp, nextwaitvpos, vsyncnextscanline);
#endif
		}
		do_display_slice();
		input_read_done = true;
		display_slice_cnt = -1;
		display_rendered = false;
#if LLV_DEBUG
		write_log("\n");
#endif

	} else if (vpos >= nextwaitvpos) {

		// topmost/first slice?
		if (display_slice_cnt == 0) {

			if (!currprefs.turbo_emulation) {
				if (!was_syncline) {
					do_render_slice(2, display_slice_cnt, vpos - 1);
					display_rendered = true;
				}

				// flip slightly early because flip regularly gets delayed if done during vblank
				int lastflipline = vsync_activeheight - vsyncnextscanline_add / 5;
				while (sync_timeout_check(maxtime)) {
					int vp = target_get_display_scanline(-1);
					maybe_process_pull_audio();
					if (vp < vsync_activeheight / 2 || vp >= lastflipline)
						break;
					if (currprefs.m68k_speed < 0 && !was_syncline) {
						is_syncline_end = lastflipline;
						is_syncline = -2;
						return 0;
					}
					target_spin(0);
#if LLV_DEBUG
					write_log(_T("2:%d:%d:%d:%d."), vpos, vp, nextwaitvpos, vsyncnextscanline);
#endif
				}
				do_display_slice();
				display_rendered = false;
				input_read_done = true;

#if 1
				// if flipped before vblank, wait for vblank
				while (sync_timeout_check(maxtime)) {
					int vp = target_get_display_scanline(-1);
					if (vp < vsync_activeheight / 2)
						break;
					if (currprefs.m68k_speed < 0 && !was_syncline) {
						is_syncline = -1;
						is_syncline_end = vp;
						return 0;
					}
					maybe_process_pull_audio();
					target_spin(0);
				}
#endif
			}

		} else {

			// skip if too close
			int vp2 = target_get_display_scanline(-1);
			if (!currprefs.turbo_emulation && (currprefs.m68k_speed < 0 || vp2 < vsyncnextscanline - vsyncnextscanline_add / 10)) {
				if (!was_syncline && !display_rendered) {
					do_render_slice(0, display_slice_cnt, vpos - 1);
					display_rendered = true;
				}
				while (sync_timeout_check(maxtime)) {
					int vp = target_get_display_scanline(-1);
					// We are still in vblank and second slice? Poll until vblank ends.
					if (display_slice_cnt == 1 && vp == -1) {
						maybe_process_pull_audio();
						if (currprefs.m68k_speed < 0 && !was_syncline) {
							is_syncline = -3;
							return 0;
						}

						target_spin(0);
#if LLV_DEBUG
						write_log(_T("3:%d:%d:%d:%d."), vpos, vp, nextwaitvpos, vsyncnextscanline);
#endif
						continue;
					}
					if (vp < 0 || vp >= vsyncnextscanline)
						break;
					maybe_process_pull_audio();
					if (currprefs.m68k_speed < 0 && !was_syncline) {
						is_syncline = vsyncnextscanline;
						return 0;
					}
					scanlinesleep(vp, vsyncnextscanline);
#if LLV_DEBUG
					write_log(_T("4:%d:%d:%d:%d."), vpos, vp, nextwaitvpos, vsyncnextscanline);
#endif
				}
				do_display_slice();
				input_read_done = true;
				display_rendered = false;
			}
			vsyncnextscanline += vsyncnextscanline_add;
			vsync_clear();
		}
		nextwaitvpos += display_slice_lines;
		display_slice_cnt++;
#if LLV_DEBUG
		write_log("\n");
#endif
	}

	return input_read_done;
}

static bool linesync_beam_vrr(void)
{
	frame_time_t maxtime = read_processor_time() + 2 * vsynctimebase;
	bool input_read_done = false;
	bool was_syncline = is_syncline != 0;

	events_reset_syncline();
	if (vpos == 0 && !was_syncline) {
		int firstline, lastline;
		linesync_first_last_line(&firstline, &lastline);

		display_slices = currprefs.gfx_display_sections;
		if (!display_slices)
			display_slices = 1;
		display_slice_cnt = 0;
		vsyncnextscanline = vsync_activeheight / display_slices + 1;
		vsyncnextscanline_add = vsync_activeheight / display_slices;
		display_slice_lines = (lastline - firstline) / display_slices + 1;
		nextwaitvpos = firstline + display_slice_lines + display_slice_lines / 2;
		if (display_slices <= 1)
			nextwaitvpos = lastline + 1;
		if (display_slices <= 2 && vsyncnextscanline > vsync_activeheight * 2 / 3)
			vsyncnextscanline = vsync_activeheight * 2 / 3;

		display_rendered = false;
		frame_rendered = true;
		frame_shown = true;
	}

	if (!display_slices)
		return false;

	if (is_last_line()) {

		if (!was_syncline && !display_rendered) {
			do_render_slice(1, display_slice_cnt, vpos);
			display_rendered = true;
		}

		while (!currprefs.turbo_emulation && sync_timeout_check(maxtime)) {
			int vp = target_get_display_scanline(-1);
			if (vp < 0 || vp >= vsyncnextscanline)
				break;
			maybe_process_pull_audio();
			if (currprefs.m68k_speed < 0 && !was_syncline) {
				is_syncline = vsyncnextscanline;
				return 0;
			}
			scanlinesleep(vp, vsyncnextscanline);
#if LLV_DEBUG
			write_log(_T("1:%d:%d:%d:%d."), vpos, vp, nextwaitvpos, vsyncnextscanline);
#endif
		}
		do_display_slice();
		input_read_done = true;
		display_slice_cnt = -1;
		display_rendered = false;
#if LLV_DEBUG
		write_log("\n");
#endif

	} else if (vpos >= nextwaitvpos) {

		// topmost/first slice?
		if (display_slice_cnt == 0) {

			if (!currprefs.turbo_emulation) {

				frame_time_t rpt;
				for (;;) {
					rpt = read_processor_time();
					if (rpt - (vsyncmintime - vsynctimebase * 2 / 3) >= 0 || rpt - vsyncmintime < -2 * vsynctimebase)
						break;
					maybe_process_pull_audio();
					if (currprefs.m68k_speed < 0 && !was_syncline) {
						is_syncline = -1;
						is_syncline_end = target_get_display_scanline(-1);
						return 0;
					}
					target_spin(0);
				}

				if (!was_syncline) {
					do_render_slice(2, display_slice_cnt, vpos - 1);
					display_rendered = true;
				}

				for (;;) {
					rpt = read_processor_time();
					if (rpt - vsyncmintime >= 0 || rpt - vsyncmintime < -2 * vsynctimebase)
						break;
					maybe_process_pull_audio();
					if (currprefs.m68k_speed < 0 && !was_syncline) {
						is_syncline = -1;
						is_syncline_end = target_get_display_scanline(-1);
						return 0;
					}
					target_spin(0);
				}

				if (rpt - vsyncmintime < vsynctimebase && rpt - vsyncmintime > -vsynctimebase) {
					vsyncmintime += vsynctimebase;
				} else {
					vsyncmintime = rpt + vsynctimebase;
				}
				do_display_slice();
				display_rendered = false;
				input_read_done = true;
			}

		} else {

			if (!currprefs.turbo_emulation) {
				if (!was_syncline && !display_rendered) {
					do_render_slice(0, display_slice_cnt, vpos - 1);
					display_rendered = true;
				}
				while (sync_timeout_check(maxtime)) {
					int vp = target_get_display_scanline(-1);
					// We are still in vblank and second slice? Poll until vblank ends.
					if (display_slice_cnt == 1 && vp == -1) {
						maybe_process_pull_audio();
						target_spin(0);
#if LLV_DEBUG
						write_log(_T("3:%d:%d:%d:%d."), vpos, vp, nextwaitvpos, vsyncnextscanline);
#endif
						continue;
					}
					if (vp < 0 || vp >= vsyncnextscanline)
						break;
					maybe_process_pull_audio();
					if (currprefs.m68k_speed < 0 && !was_syncline) {
						is_syncline = vsyncnextscanline;
						return 0;
					}
					scanlinesleep(vp, vsyncnextscanline);
#if LLV_DEBUG
					write_log(_T("4:%d:%d:%d:%d."), vpos, vp, nextwaitvpos, vsyncnextscanline);
#endif
				}
				do_display_slice();
				input_read_done = true;
				display_rendered = false;
			}
			vsyncnextscanline += vsyncnextscanline_add;
			vsync_clear();
		}
		nextwaitvpos += display_slice_lines;
		display_slice_cnt++;
#if LLV_DEBUG
		write_log("\n");
#endif
	}

	return input_read_done;
}
#endif

// called when extra CPU wait is done
void vsync_event_done(void)
{
	if (!isvsync_chipset()) {
		events_reset_syncline();
		return;
	}
#ifdef WITH_BEAMRACER
	if (currprefs.gfx_display_sections <= 1) {
		if (vsync_vblank >= 85)
			linesync_beam_single_dual();
		else
			linesync_beam_single_single();
	} else {
		if (currprefs.gfx_variable_sync)
			linesync_beam_vrr();
		else if (vsync_vblank >= 85)
			linesync_beam_multi_dual();
		else
			linesync_beam_multi_single();
	}
#endif
}

static void check_vblank_copjmp(uae_u32 v)
{
	COPJMP(1, 1);
}

static void delayed_framestart(uae_u32 v)
{
	check_vblank_copjmp(0);
	INTREQ_INT(5, 0); // total REFRESH_FIRST_HPOS - 1 + 1 (in INTREQ_INT)
}

// this prepares for new line
static void hsync_handler_post(bool onvsync)
{
	// genlock active:
	// vertical: interlaced = toggles every other field, non-interlaced = both fields (normal)
	// horizontal: PAL = every line, NTSC = every other line
	genlockhtoggle = !genlockhtoggle;
	bool ciahsyncs = !(bplcon0 & 2) || ((bplcon0 & 2) && currprefs.genlock && (!currprefs.ntscmode || genlockhtoggle));
	bool ciavsyncs = !(bplcon0 & 2) || ((bplcon0 & 2) && currprefs.genlock && genlockvtoggle);

	CIA_hsync_posthandler(false, false);
	if (currprefs.cs_cd32cd) {
		CIA_hsync_posthandler(true, true);
		CIAB_tod_handler(35);
	} else if (ciahsyncs) {
		CIA_hsync_posthandler(true, ciahsyncs);
		if (beamcon0 & BEAMCON0_VARHSYEN) {
			if (hsstop < (maxhpos & ~1) && hsstrt < maxhpos) {
				CIAB_tod_handler(hsstop);
			}
		} else {
			CIAB_tod_handler(35); // hsync end
		}
	}

	if (currprefs.cs_cd32cd) {

		if (cia_hsync < maxhpos) {
			CIAA_tod_handler(cia_hsync);
			cia_hsync += (akiko_ntscmode() ? 262 : 313) * maxhpos;
		} else {
			cia_hsync -= maxhpos;
		}

	} else if (currprefs.cs_ciaatod > 0) {
#if 0
		static uae_s32 oldtick;
		uae_s32 tick = read_system_time (); // milliseconds
		int ms = 1000 / (currprefs.cs_ciaatod == 2 ? 60 : 50);
		if (tick - oldtick > 2000 || tick - oldtick < -2000) {
			oldtick = tick - ms;
			write_log (_T("RESET\n"));
		} 
		if (tick - oldtick >= ms) {
			CIA_vsync_posthandler (1);
			oldtick += ms;
		}
#else
		if (cia_hsync < maxhpos) {
			int newcount;
			CIAA_tod_handler(cia_hsync);
			newcount = (int)((vblank_hz * (2 * maxvpos + (interlace_seen ? 1 : 0)) * (2 * maxhpos + (linetoggle ? 1 : 0))) / ((currprefs.cs_ciaatod == 2 ? 60 : 50) * 4));
			cia_hsync += newcount;
		} else {
			cia_hsync -= maxhpos;
		}
#endif
	} else if (currprefs.cs_ciaatod == 0 && ciavsyncs) {
		// CIA-A TOD counter increases when vsync pulse ends
		if (beamcon0 & BEAMCON0_VARVSYEN) {
			if (vsstop == vpos) {
				// Always uses HCENTER and HSSTRT registers. Even if VARHSYEN=0.
				CIAA_tod_handler(lof_store ? hcenter : hsstrt);
			}
		} else {
			if (vpos == (currprefs.ntscmode ? VSYNC_ENDLINE_NTSC : VSYNC_ENDLINE_PAL)) {
				CIAA_tod_handler(lof_store ? 132 : 18);
			}
		}
	}

	if (!custom_disabled) {
		if (!currprefs.blitter_cycle_exact && blt_info.blit_main && dmaen (DMA_BITPLANE) && vdiwstate == diw_states::DIW_waiting_stop) {
			blitter_slowdown(thisline_decision.plfleft, thisline_decision.plfright - (16 << fetchmode),
				cycle_diagram_total_cycles[fetchmode][GET_RES_AGNUS (bplcon0)][GET_PLANES_LIMIT (bplcon0)],
				cycle_diagram_free_cycles[fetchmode][GET_RES_AGNUS (bplcon0)][GET_PLANES_LIMIT (bplcon0)]);
		}
	}

	if (onvsync) {
		// vpos_count >= MAXVPOS just to not crash if VPOSW writes prevent vsync completely
		vpos = 0;
		vsync_handler_post();
		vpos_count = 0;
	}

	// vblank interrupt = next line after VBSTRT
	if (vpos == 0 && vb_start_line == 1) {
		// first refresh (strobe) slot triggers vblank interrupt
		// copper and vblank trigger in same line
		event2_newevent_xx(-1, 2 * CYCLE_UNIT, 0, delayed_framestart);
	} else if (vb_start_line == 1) {
		INTREQ_INT(5, REFRESH_FIRST_HPOS - 1);
	} else if (vpos == 0) {
		event2_newevent_xx(-1, 2 * CYCLE_UNIT, 0, check_vblank_copjmp);
	}

	// lastline - 1?
	if (vpos + 1 == maxvpos + lof_store || vpos + 1 == maxvpos + lof_store + 1) {
		lof_lastline = lof_store != 0;
	}

	if (vb_end_next_line2) {
		vb_end_next_line2 = false;
	}
	if (vb_end_next_line) {
		vb_end_next_line2 = true;
		vb_end_next_line = false;
	}
	if (vb_start_line) {
		vb_start_line++;
	}
	if (vb_end_line) {
		vb_start_line = 0;
		vb_end_line = false;
		vb_end_next_line = true;
	}

	vs_state_var_old = vs_state_var;
	if (ecs_agnus) {
		if (vsstrt_m == vpos) {
			vsstrt_m = -1;
		}
		if (vsstop_m == vpos) {
			vsstop_m = -1;
		}

		if (vbstrt_m == vpos) {
			vbstrt_m = -1;
		}
		if (vbstop_m == vpos) {
			vbstop_m = -1;
		}

		if (vsstrt == vpos) {
			vsstrt_m = vpos;
			vs_state_var = true;
		}
		if (vsstop == vpos) {
			vsstop_m = vpos;
			vs_state_var = false;
		}

		if (new_beamcon0 & BEAMCON0_VARVBEN) {
			if (vbstrt == vpos) {
				vbstrt_m = vpos;
				vb_start_line = 1;
				vb_state = true;
			}
			int vbs = vbstop - 1;
			if (vbs < 0) {
				vbs += maxvpos;
			}
			if (vbs == vpos) {
				vbstop_m = vpos;
				vb_end_line = true;
				vb_state = false;
				if (vbstrt == vpos) {
					vb_start_line = 0;
				}
			}
		}

		if (vpos == sprhstrt_v) {
			hhspr = 1;
		}
		if (vpos == sprhstop_v) {
			hhspr = 0;
		}
		if (vpos == bplhstrt_v) {
			hhbpl = 1;
		}
		if (vpos == bplhstop_v) {
			hhbpl = 0;
		}
		uae_u16 add = maxhpos - 1;
		uae_u16 max = (new_beamcon0 & BEAMCON0_DUAL) ? htotal : add;
		uae_u16 hhpos_old = hhpos;
		hhpos += add;
		if (hhpos_old <= max || hhpos >= 0x100) {
			if (max)
				hhpos %= max;
			else
				hhpos = 0;
		}
		if (hhpos_hpos) {
			hhpos -= add - hhpos_hpos;
			hhpos_hpos = 0;
		}
		hhpos &= 0xff;
	}

	if (beamcon0 & BEAMCON0_PAL) {
		if (vpos == 2 && !lof_store) {
			vs_state_hw = true;
		}
		if (vpos == 3 && lof_store) {
			vs_state_hw = true;
		}
		if (vpos == 5 + 1) {
			vs_state_hw = false;
		}
	} else {
		if (vpos == 3) {
			vs_state_hw = true;
		}
		if (vpos == 6 + 1) {
			vs_state_hw = false;
		}
	}

	vs_state_on_old = vs_state_on;
	if (new_beamcon0 & bemcon0_vsync_mask) {
		vs_state_on = vs_state_var;
	} else {
		vs_state_on = vs_state_hw;
	}

	if (!(new_beamcon0 & BEAMCON0_VARVBEN)) {
		vb_check();
	}

	exthblank_lines[exthblank ? 1 : 0]++;

	if (varhblank_lines > 0) {
		varhblank_lines--;
		if (!varhblank_lines) {
			if (varhblank_val[0] != hbstrt || varhblank_val[1] != hbstop) {
				varhblank_lines = -1;
				varhblank_val[0] = hbstrt;
				varhblank_val[1] = hbstop;
			}
		}
	}

	decide_vline(0);

	if (issyncstopped()) {
		issyncstopped_count++;
	} else {
		issyncstopped_count = 0;
	}


	int hp = REFRESH_FIRST_HPOS;
	refptr_p = refptr;
	for (int i = 0; i < 4; i++) {
		uae_u16 strobe = get_strobe_reg(i);
		if (i == 0) {
			if (!hstrobe_conflict) {
				if (strobe == 0x38 || (strobe == 0x3a && ecs_denise)) {
					// OCS: only STREQU enables vblank. STREQU and STRVBL if ECS Denise.
					strobe_vblank = 1;
				} else if (strobe == 0x3c) {
					// STRHOR disables vblank
					strobe_vblank = 0;
				}
			}
		}
		alloc_cycle(hp, strobe != 0x1fe ? CYCLE_STROBE : CYCLE_REFRESH);
		// assume refresh pointer not changed or conflicted
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(strobe, refptr & refmask, hp, vpos, DMARECORD_REFRESH, i);
			record_dma_read_value(0xffff);
		}
#endif
		refptr += ref_ras_add;
		hp += 2;
	}
	refresh_handled_slot = 0;
	refptr_preupdated = true;

#ifdef DEBUGGER
	if (debug_dma) {
		if (vs_state_on) {
			record_dma_event(DMA_EVENT_VS, REFRESH_FIRST_HPOS, vpos);
		}
		if (vb_start_line) {
			record_dma_event(DMA_EVENT_VB, REFRESH_FIRST_HPOS, vpos);
		}
		if (vdiwstate == diw_states::DIW_waiting_stop) {
			record_dma_event(DMA_EVENT_VDIW, REFRESH_FIRST_HPOS, vpos);
		}
		if (lof_store) {
			record_dma_event(DMA_EVENT_LOF, REFRESH_FIRST_HPOS + 2, vpos);
		}
		if (lol) {
			record_dma_event(DMA_EVENT_LOL, REFRESH_FIRST_HPOS + 2, vpos);
		}
	}
#endif

	events_dmal_hsync();
#if 0
	// AF testing stuff
	static int cnt = 0;
	cnt++;
	if (cnt == 500) {
		int port_insert_custom (int inputmap_port, int devicetype, DWORD flags, const TCHAR *custom);
		//port_insert_custom (0, 2, 0, _T("Left=0xCB Right=0xCD Up=0xC8 Down=0xD0 Fire=0x39 Fire.autorepeat=0xD2"));
		port_insert_custom (1, 2, 0, _T("Left=0x1E Right=0x20 Up=0x11 Down=0x1F Fire=0x38"));
	} else if (0 && cnt == 1000) {
		TCHAR out[256];
		bool port_get_custom (int inputmap_port, TCHAR *out);
		port_get_custom (0, out);
		port_get_custom (1, out);
	}
#endif
	bool input_read_done = false;

	if (currprefs.cpu_thread) {

		maybe_process_pull_audio();

	} else if (isvsync_chipset() < 0) {
#ifdef WITH_BEAMRACER
		if (currprefs.gfx_display_sections <= 1) {
			if (vsync_vblank >= 85)
				input_read_done = linesync_beam_single_dual();
			else
				input_read_done = linesync_beam_single_single();
		} else {
			if (currprefs.gfx_variable_sync)
				input_read_done = linesync_beam_vrr();
			else if (vsync_vblank >= 85)
				input_read_done = linesync_beam_multi_dual();
			else
				input_read_done = linesync_beam_multi_single();
		}
#endif
	} else if (!currprefs.cpu_thread && !cpu_sleepmode && currprefs.m68k_speed < 0 && !currprefs.cpu_memory_cycle_exact) {

		static int sleeps_remaining;
		if (is_last_line ()) {
			sleeps_remaining = (165 - currprefs.cpu_idle) / 6;
			sleeps_remaining = std::max(sleeps_remaining, 0);
			/* really last line, just run the cpu emulation until whole vsync time has been used */
			if (regs.stopped && currprefs.cpu_idle) {
				// CPU in STOP state: sleep if enough time left.
				frame_time_t rpt = read_processor_time();
				while (vsync_isdone(NULL) <= 0 && vsyncmintime - (rpt + vsynctimebase / 10) > 0 && vsyncmintime - rpt < vsynctimebase) {
					maybe_process_pull_audio();
//					if (!execute_other_cpu(rpt + vsynctimebase / 10)) {
						if (cpu_sleep_millis(1) < 0)
							break;
//					}
					rpt = read_processor_time();
				}
			} else if (currprefs.m68k_speed_throttle) {
				vsyncmintime = read_processor_time(); /* end of CPU emulation time */
				events_reset_syncline();
				maybe_process_pull_audio();
			} else {
				vsyncmintime = vsyncmaxtime; /* emulate if still time left */
				is_syncline_end = read_processor_time() + vsynctimebase; /* far enough in future, we never wait that long */
				is_syncline = -12;
				maybe_process_pull_audio();
			}
		} else {
			static int linecounter;
			/* end of scanline, run cpu emulation as long as we still have time */
			vsyncmintime += vsynctimeperline;
			linecounter++;
			events_reset_syncline();
			if (vsync_isdone(NULL) <= 0 && !currprefs.turbo_emulation) {
				if (vsyncmaxtime - vsyncmintime > 0) {
					if (vsyncwaittime - vsyncmintime > 0) {
						frame_time_t rpt = read_processor_time();
						/* Extra time left? Do some extra CPU emulation */
						if (vsyncmintime - rpt > 0) {
							if (regs.stopped && currprefs.cpu_idle && sleeps_remaining > 0) {
								// STOP STATE: sleep.
								cpu_sleep_millis(1);
								sleeps_remaining--;
								maybe_process_pull_audio();
							} else {
								is_syncline = -11;
								/* limit extra time */
								is_syncline_end = rpt + vsynctimeperline;
								linecounter = 0;
							}
						}
					}
					if (!isvsync ()) {
						// extra cpu emulation time if previous 10 lines without extra time.
						if (!is_syncline && linecounter >= 10 && (!regs.stopped || !currprefs.cpu_idle)) {
							is_syncline = -10;
							is_syncline_end = read_processor_time() + vsynctimeperline;
							linecounter = 0;
						}
					}
				}
			}
			maybe_process_pull_audio();
		}

	} else if (!currprefs.cpu_thread) {

		// the rest
		static int nextwaitvpos;
		if (vpos == 0)
			nextwaitvpos = maxvpos_display * 1 / 4;
		if (audio_is_pull() > 0 && !currprefs.turbo_emulation) {
			maybe_process_pull_audio();
			frame_time_t rpt = read_processor_time();
			while (audio_pull_buffer() > 1 && (!isvsync() || (vsync_isdone(NULL) <= 0 && vsyncmintime - (rpt + vsynctimebase / 10) > 0 && vsyncmintime - rpt < vsynctimebase))) {
				cpu_sleep_millis(1);
				maybe_process_pull_audio();
				rpt = read_processor_time();
			}
		}
		if (vpos + 1 < maxvpos + lof_store && vpos >= nextwaitvpos && vpos < maxvpos - (maxvpos / 3) && (audio_is_pull() <= 0 || (audio_is_pull() > 0 && audio_pull_buffer()))) {
			nextwaitvpos += maxvpos_display * 1 / 3;
			vsyncmintime += vsynctimeperline;
			if (vsync_isdone(NULL) <= 0 && !currprefs.turbo_emulation) {
				frame_time_t rpt = read_processor_time();
				// sleep if more than 2ms "free" time
				while (vsync_isdone(NULL) <= 0 && vsyncmintime - (rpt + vsynctimebase / 10) > 0 && vsyncmintime - rpt < vsynctimebase) {
					maybe_process_pull_audio();
//					if (!execute_other_cpu(rpt + vsynctimebase / 10)) {
						if (cpu_sleep_millis(1) < 0)
							break;
//					}
					rpt = read_processor_time();
				}
			}
		}
	}

	if (!input_read_done) {
		inputdevice_hsync(false);
	}

	reset_decisions_scanline_start();

	rethink_uae_int();

	/* See if there's a chance of a copper wait ending this line.  */
	compute_spcflag_copper();

	// check reset and process it immediately, don't wait for vsync
	if (quit_program == -UAE_RESET || quit_program == -UAE_RESET_KEYBOARD || quit_program == -UAE_RESET_HARD) {
		quit_program = -quit_program;
		set_special(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
	}


#if 0
	/* fastest possible + last line and no vflip wait: render the frame as early as possible */
	if (is_last_line () && isvsync_chipset () <= -2 && !vsync_rendered && currprefs.gfx_apmode[0].gfx_vflip == 0) {
		frame_time_t start, end;
		start = read_processor_time ();
		vsync_rendered = true;
		vsync_handle_redraw (lof_store, lof_changed, bplcon0, bplcon3);
		if (vblank_hz_state) {
			frame_rendered = crender_screen(1, true);
		}
		end = read_processor_time ();
		frameskiptime += end - start;
	}
#endif

	rtg_vsynccheck ();

#if 0
	{
		static int skip;
		if (M68K_GETPC >= 0x0C0D7A2 && M68K_GETPC < 0x00C0D7B2 && vpos == 0xf3) {
			if (!skip)
				activate_debugger ();
			skip = 1;
		}
		if (vpos != 0xf3)
			skip = 0;
	}
#endif

#if 0
	if (vpos == 300 && get_cycles() < 312 * 227 * CYCLE_UNIT)
		activate_debugger();
#endif
}

static bool vsync_line;
// executed at start of scanline
static void hsync_handler(void)
{
	bool vs = is_custom_vsync();
	hsync_handler_pre(vs);
	if (vs) {
		devices_vsync_pre();
		if (savestate_check()) {
			uae_reset(0, 0);
			return;
		}
		eventtab[ev_hsynch].evtime = get_cycles() + hsyncstartpos_start_cycles * CYCLE_UNIT;
		eventtab[ev_hsynch].active = 1;
		events_schedule();

	}
	if (vpos == maxvpos_display_vsync + 1 && !maxvpos_display_vsync_next) {
		hsync_record_line_state_last(next_lineno, nextline_how, 0);
		inputdevice_read_msg(true);
		vsync_display_render();
		vsync_display_rendered = false;
		lof_display = lof_store;
		hstrobe_conflict = false;
		minfirstline_linear = minfirstline;
		reset_autoscale();
		display_vsync_counter++;
		maxvpos_display_vsync_next = true;
	} else if (vpos != maxvpos_display_vsync + 1 && maxvpos_display_vsync_next) {
		// protect against weird VPOSW writes causing continuous vblanks
		maxvpos_display_vsync_next = false;
	}
	vsync_line = vs;
	hsync_handler_post(vs);
}

// executed at start of hsync
static void hsync_handlerh(void)
{
	if (vsync_line) {
		eventtab[ev_hsynch].evtime = get_cycles() + HSYNCTIME;
		eventtab[ev_hsynch].active = 1;
		events_schedule();
	}
	if (vpos == maxvpos_display_vsync) {
		vposh = maxvpos_display_vsync;
	}
	hsync_handlerh(vsync_line);
}

static void audio_evhandler2(void)
{
	// update copper first
	// if copper had written to audio registers
	if (copper_enabled_thisline) {
		int hpos = current_hpos();
		sync_copper(hpos);
	}
	audio_evhandler();
}

void init_eventtab(void)
{
	if (!savestate_state) {
		clear_events();
	}

	eventtab[ev_cia].handler = CIA_handler;
	eventtab[ev_hsync].handler = hsync_handler;
	eventtab[ev_hsync].evtime = get_cycles() + HSYNCTIME;
	eventtab[ev_hsync].active = 1;
	eventtab[ev_hsynch].handler = hsync_handlerh;
	eventtab[ev_hsynch].evtime = get_cycles() + HSYNCTIME;
	eventtab[ev_hsynch].active = 0;
	eventtab[ev_misc].handler = MISC_handler;
	eventtab[ev_audio].handler = audio_evhandler2;

	eventtab2[ev2_blitter].handler = blitter_handler;

	events_schedule();
}

void custom_prepare(void)
{
	set_hpos();
	hsync_handler_post(true);
}

void custom_cpuchange(void)
{
	// both values needs to be same but also different
	// after CPU mode changes
	intreq = intreq | 0x8000;
	intena = intena | 0x8000;
	intreq2 = intreq;
	intena2 = intena;
}


void custom_reset(bool hardreset, bool keyboardreset)
{
	if (hardreset) {
		board_prefs_changed(-1, -1);
		initial_frame = true;
	}

	target_reset();
	devices_reset(hardreset);
	write_log(_T("Reset at %08X. Chipset mask = %08X\n"), M68K_GETPC, currprefs.chipset_mask);
#ifdef DEBUGGER
	memory_map_dump();
#endif

	bool ntsc = currprefs.ntscmode;

	lightpen_active = 0;
	lightpen_triggered = 0;
	lightpen_cx[0] = lightpen_cy[0] = -1;
	lightpen_cx[1] = lightpen_cy[1] = -1;
	lightpen_x[0] = -1;
	lightpen_y[0] = -1;
	lightpen_x[1] = -1;
	lightpen_y[1] = -1;
	nr_armed = 0;
	next_lineno = 0;
	vb_start_line = 1;
	if (agnusa1000) {
		vb_start_line = 0;
	}
	vb_state = true;
	vs_state_var = false;
	vs_state_hw = false;
	vs_state_on = false;
	dmal_htotal_mask = 0xffff;
	memset(custom_storage, 0, sizeof(custom_storage));
	toscr_res_old = -1;
	toscr_nbits = 0;
	update_denise_vars();
	estimated_fm = 0xffff;
	exthblank = false;
	exthblank_state = false;
	hbstrt_v2 = 0;
	hbstop_v2 = 0;
	hcenter_v2 = 0;
	hcenter_active = false;
	set_hcenter();
	display_reset = 1;
	copper_bad_cycle = 0;
	copper_dma_change_cycle = -1;
	blitter_dma_change_cycle = -1;
	sprite_dma_change_cycle_on = -1;
	custom_color_write_cycle = -1;
	vt_old = 0;
	ht_old = 0;
	hdiwstate_blank = diw_states::DIW_waiting_start;
	maxvpos_display_vsync_next = false;
	hstrobe_conflict = false;
	strobe_vblank = 0;

	irq_forced_delay = 0;
	irq_forced = 0;

	if (hardreset || savestate_state) {
		maxhpos = ntsc ? MAXHPOS_NTSC : MAXHPOS_PAL;
		maxhpos_short = ntsc ? MAXHPOS_NTSC : MAXHPOS_PAL;
		maxvpos = ntsc ? MAXVPOS_NTSC : MAXVPOS_PAL;
		maxvpos_nom = ntsc ? MAXVPOS_NTSC : MAXVPOS_PAL;
		maxvpos_display = ntsc ? MAXVPOS_NTSC : MAXVPOS_PAL;
	}

	if (!savestate_state) {
		cia_hsync = 0;
		extra_cycle = 0;
		hsync_counter = 0;
		vsync_counter = 0;
		display_vsync_counter = 0;
		currprefs.chipset_mask = changed_prefs.chipset_mask;
		update_mirrors();
		blitter_reset();

		if (hardreset) {

			vtotal = MAXVPOS_LINES_ECS - 1;
			htotal = MAXHPOS_ROWS - 1;
			hbstrt = 0;
			hbstop = 0;
			hsstrt = 0;
			hsstop = 0;
			vbstrt = 0;
			vbstop = 0;
			vsstrt = 0;
			vsstop = 0;
			hcenter = 0;

			for (int i = 0; i < 32; i++) {
				uae_u16 c;
				if (i == 0) {
					c = ((ecs_denise && !aga_mode) || denisea1000) ? 0xfff : 0x000;
				} else {
					c |= uaerand();
					c |= uaerand();
				}
				c &= 0xfff;
				current_colors.color_regs_ecs[i] = c;
			}
			for (int i = 0; i < 256; i++) {
				uae_u32 c = 0;
				if (i > 0) {
					c |= uaerand();
					c |= uaerand();
				}
				c &= 0xffffff;
				current_colors.color_regs_aga[i] = c;
			}
			if (!aga_mode) {
				for (int i = 0; i < 32; i++) {
					current_colors.acolors[i] = getxcolor(current_colors.color_regs_ecs[i]);
				}
#ifdef AGA
			} else {
				for (int i = 0; i < 256; i++) {
					current_colors.acolors[i] = getxcolor(current_colors.color_regs_aga[i]);
				}
#endif
			}
			lof_store = lof_display = 0;
			lof_lace = false;
			setextblank();
		}

		clxdat = 0;

		/* Clear the armed flags of all sprites.  */
		memset(spr, 0, sizeof spr);

		dmacon = 0;
		intreq = intreq2 = 0;
		intena = intena2 = 0;

		copcon = 0;
		DSKLEN (0, 0);

		bplcon0 = 0;
		bplcon4 = 0x0011; /* Get AGA chipset into ECS compatibility mode */
		bplcon3 = 0x0C00;

		bplcon0_saved = bplcon0;
		bplcon1_saved = bplcon1;
		bplcon2_saved = bplcon2;
		bplcon3_saved = bplcon3;
		bplcon4_saved = bplcon4;

		diwhigh = 0;
		diwhigh_written = 0;
		hdiwstate = diw_states::DIW_waiting_start; // this does not reset at vblank

		refptr = 0;
		if (aga_mode) {
			refptr = 0x1ffffe;
		}
		refptr_p = refptr;
		FMODE(0, 0);
		CLXCON(-1, 0);
		CLXCON2(-1, 0);
		setup_fmodes(0, bplcon0);
		beamcon0 = new_beamcon0 = beamcon0_saved = currprefs.ntscmode ? 0x00 : BEAMCON0_PAL;
		blt_info.blit_main = 0;
		blt_info.blit_finald = 0;
		blt_info.blit_pending = 0;
		blt_info.blit_interrupt = 1;
		blt_info.blit_queued = 0;
		init_sprites();

		maxhpos = ntsc ? MAXHPOS_NTSC : MAXHPOS_PAL;
		maxhpos_short = maxhpos;
		updateextblk();

		if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
			// fake draco interrupts
			INTENA(0x8000 | 0x4000 | 0x1000 | 0x2000 | 0x0080 | 0x0010 | 0x0008 | 0x0001);
		}
	}
#ifdef WITH_SPECIALMONITORS
	specialmonitor_reset();
#endif

	unset_special (~(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE));

	vpos = 0;
	vpos_prev = maxvpos - 1;
	vpos_count = vpos_count_diff = 0;

	inputdevice_reset();
	timehack_alive = 0;

	curr_sprite_entries = 0;
	prev_sprite_entries = 0;
	sprite_entries[0][0].first_pixel = 0;
	sprite_entries[1][0].first_pixel = MAX_SPR_PIXELS;
	sprite_entries[0][1].first_pixel = 0;
	sprite_entries[1][1].first_pixel = MAX_SPR_PIXELS;
	memset(spixels, 0, 2 * MAX_SPR_PIXELS * sizeof *spixels);
	memset(&spixstate, 0, sizeof spixstate);
	toscr_delay_sh[0] = 0;
	toscr_delay_sh[1] = 0;

	vdiwstate = diw_states::DIW_waiting_start;
	vdiw_change(0);
	check_harddis();

	dmal = 0;
	init_hz_normal();
	// init_hz sets vpos_count
	vpos_count = 0;
	vpos_lpen = -1;
	lof_changing = 0;
	lof_togglecnt_nlace = lof_togglecnt_lace = 0;
	//nlace_cnt = NLACE_CNT_NEEDED;

	audio_reset();
	if (!isrestore()) {
		memset(&cop_state, 0, sizeof(cop_state));
		cop_state.state = COP_stop;
		/* must be called after audio_reset */
		adkcon = 0;
#ifdef SERIAL_PORT
		serial_uartbreak(0);
#endif
		audio_update_adkmasks();
	}

	init_hardware_frame();
	drawing_init();

	reset_decisions_scanline_start();
	reset_decisions_hsync_start();

	bogusframe = 1;
	vsync_rendered = false;
	frame_shown = false;
	frame_rendered = false;

	if (isrestore()) {
		uae_u16 v;
		uae_u32 vv;

		audio_update_adkmasks();
		INTENA(0);
		INTREQ(0);
		COPJMP(1, 1);
		v = bplcon0;
		BPLCON0(0, 0);
		BPLCON0(0, v);
		FMODE(0, fmode);
		if (!aga_mode) {
			for(int i = 0 ; i < 32 ; i++)  {
				vv = current_colors.color_regs_ecs[i];
				current_colors.color_regs_ecs[i] = -1;
				record_color_change(0, i, vv);
				remembered_color_entry = -1;
				current_colors.color_regs_ecs[i] = vv;
				current_colors.acolors[i] = xcolors[vv];
			}
#ifdef AGA
		} else {
			for(int i = 0 ; i < 256 ; i++)  {
				vv = current_colors.color_regs_aga[i];
				current_colors.color_regs_aga[i] = -1;
				record_color_change(0, i, vv);
				remembered_color_entry = -1;
				current_colors.color_regs_aga[i] = vv;
				current_colors.acolors[i] = CONVERT_RGB(vv);
			}
#endif
		}
		CLXCON(-1, clxcon);
		CLXCON2(-1, clxcon2);
		calcdiw();
#ifdef SERIAL_PORT
		v = serper;
		serper = 0;
		SERPER(v);
#endif
		for (int i = 0; i < 8; i++) {
			SPRxCTLPOS(i);
			nr_armed += spr[i].armed != 0;
		}
		if (!currprefs.produce_sound) {
			eventtab[ev_audio].active = 0;
			events_schedule();
		}

		write_log(_T("CPU=%d Chipset=%s %s\n"),
			currprefs.cpu_model,
			(aga_mode ? _T("AGA") :
			(ecs_agnus && ecs_denise ? _T("Full ECS") :
			(ecs_denise ? _T("ECS Denise") :
			(ecs_agnus ? _T("ECS") : _T("OCS"))))),
			currprefs.ntscmode ? _T("NTSC") : _T("PAL"));
		write_log(_T("State restored\n"));
	}

	sprres = expand_sprres(bplcon0, bplcon3);
	sprite_width = GET_SPRITEWIDTH(fmode);
	for (int i = 0; i < MAX_SPRITES; i++) {
		spr[i].width = sprite_width;
	}
	setup_fmodes(0, bplcon0);
	shdelay_disabled = false;

#ifdef ACTION_REPLAY
	/* Doing this here ensures we can use the 'reset' command from within AR */
	action_replay_reset(hardreset, keyboardreset);
#endif
#if defined(ENFORCER)
	enforcer_disable();
#endif

	if (hardreset) {
		rtc_hardreset();
	}

	// must be last
#ifdef AUTOCONFIG
	expamem_reset(hardreset);
#endif

#if 0
	record_dma_reset(-1);
#endif

}

void custom_dumpstate(int mode)
{
	if (!mode) {
		console_out_f(_T("VPOS: %03d ($%03x) HPOS: %03d ($%03x) COP: $%08x\n"),
			vpos, vpos, current_hpos(), current_hpos(),
			cop_state.ip);
	}
}

void dumpcustom(void)
{
	console_out_f(_T("DMACON: $%04x INTENA: $%04x ($%04x) INTREQ: $%04x ($%04x) VPOS: %03d ($%03x) HPOS: %03d ($%03x)\n"),
		DMACONR(current_hpos()),
		intena, intena, intreq, intreq, vpos, vpos, current_hpos(), current_hpos());
	console_out_f(_T("INT: $%04x IPL: %d\n"), intena & intreq, intlev());
	console_out_f(_T("COP1LC: $%08x, COP2LC: $%08x COPPTR: $%08x\n"), cop1lc, cop2lc, cop_state.ip);
	console_out_f(_T("DIWSTRT: $%04x DIWSTOP: $%04x DDFSTRT: $%04x DDFSTOP: $%04x\n"),
		diwstrt, diwstop, ddfstrt, ddfstop);
	console_out_f(_T("BPLCON 0: $%04x 1: $%04x 2: $%04x 3: $%04x 4: $%04x LOF=%d/%d HDIW=%d VDIW=%d\n"),
		bplcon0, bplcon1, bplcon2, bplcon3, bplcon4,
		lof_display, lof_store,
		hdiwstate == diw_states::DIW_waiting_start ? 0 : 1, vdiwstate == diw_states::DIW_waiting_start ? 0 : 1);
	if (timeframes && vsync_counter) {
		console_out_f(_T("Average frame time: %.2f ms [frames: %d time: %d]\n"),
			(double)frametime / timeframes, vsync_counter, frametime);
		if (total_skipped)
			console_out_f(_T("Skipped frames: %d\n"), total_skipped);
	}
}

static void gen_custom_tables(void)
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

/* mousehack is now in "filesys boot rom" */
static uae_u32 REGPARAM2 mousehack_helper_old(struct TrapContext *ctx)
{
	return 0;
}

int custom_init(void)
{

#ifdef AUTOCONFIG
	if (uae_boot_rom_type) {
		uaecptr pos;
		pos = here();

		org(rtarea_base + 0xFF70);
		calltrap(deftrap(mousehack_helper_old));
		dw(RTS);

		org(rtarea_base + 0xFFA0);
		calltrap(deftrap(timehack_helper));
		dw(RTS);

		org(pos);
	}
#endif

	gen_custom_tables();
	build_blitfilltable();

	drawing_init();

	update_mirrors();
	create_cycle_diagram_table();

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
	if (currprefs.cpu_model >= 68020 && !currprefs.cpu_compatible)
		return dummy_wgeti (addr);
	return custom_wget (addr);
}
static uae_u32 REGPARAM2 custom_lgeti (uaecptr addr)
{
	if (currprefs.cpu_model >= 68020 && !currprefs.cpu_compatible)
		return dummy_lgeti (addr);
	return custom_lget (addr);
}

static uae_u32 REGPARAM2 custom_wget_1(int hpos, uaecptr addr, int noput, bool isbyte)
{
	uae_u16 v;
	int missing;
#if CUSTOM_DEBUG > 2
	write_log (_T("%d:%d:wget: %04X=%04X pc=%p\n"), current_hpos(), vpos, addr, addr & 0x1fe, m68k_getpc ());
#endif
#ifdef DEBUGGER
	if (memwatch_access_validator)
		debug_check_reg(addr, 0, 0);
#endif

	addr &= 0xfff;

	switch (addr & 0x1fe) {
	case 0x002: v = DMACONR(hpos); break;
	case 0x004: v = VPOSR(); break;
	case 0x006: v = VHPOSR(); break;

	case 0x00A: v = JOY0DAT(); break;
	case 0x00C: v = JOY1DAT(); break;
	case 0x00E: v = CLXDAT(hpos); break;
	case 0x010: v = ADKCONR(); break;

	case 0x012: v = POT0DAT(); break;
	case 0x014: v = POT1DAT(); break;
	case 0x016: v = POTGOR(); break;
#ifdef SERIAL_PORT
	case 0x018: v = SERDATR(); break;
#else
	case 0x018: v = 0x3000 /* no data */; break;
#endif
	case 0x01A: v = DSKBYTR(hpos); break;
	case 0x01C: v = INTENAR(); break;
	case 0x01E: v = INTREQR(); break;
	case 0x07C:
		v = DENISEID(&missing);
		if (missing)
			goto writeonly;
		break;

	case 0x1DA:
		if (!ecs_agnus)
			goto writeonly;
		v = HHPOSR();
		break;

#ifdef AGA
	case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
	case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
	case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
	case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
	case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
	case 0x1BC: case 0x1BE:
		if (!aga_mode)
			goto writeonly;
		v = COLOR_READ((addr & 0x3E) / 2);
		break;
#endif

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
		* Remembers old regs.chipset_latch_rw
		*/
		v = regs.chipset_latch_rw;
		SET_LINE_CYCLEBASED(hpos);
		if (!noput) {
			int r, c, bmdma;
			uae_u16 l;

			if (aga_mode) {
				l = 0;
			} else {
				// last chip bus value (read or write) is written to register
				if (currprefs.cpu_compatible && currprefs.cpu_model == 68000) {
					if (isbyte)
						l = (regs.chipset_latch_rw << 8) | (regs.chipset_latch_rw & 0xff);
					else
						l = regs.chipset_latch_rw;
				} else {
					l = regs.chipset_latch_rw;
				}
			}
			decide_line(hpos);
			decide_fetch_safe(hpos);
			decide_blitter(hpos);
#ifdef DEBUGGER
			debug_wputpeek(0xdff000 + addr, l);
#endif
			r = custom_wput_1(hpos, addr, l, 1);
			
			// CPU gets back (OCS/ECS only):
			// - if last cycle was DMA cycle: DMA cycle data
			// - if last cycle was not DMA cycle: FFFF or some ANDed old data.
			//
			if (hpos == 0) {
				int hp = maxhpos - 1;
				c = cycle_line_slot_last & CYCLE_MASK;
				bmdma = bitplane_dma_access(hp, 0);
			} else {
				int hp = hpos - 1;
				c = cycle_line_slot[hp] & CYCLE_MASK;
				bmdma = bitplane_dma_access(hp, 0);
			}
			if (aga_mode) {
				if (bmdma || (c > CYCLE_REFRESH && c < CYCLE_CPU)) {
					v = regs.chipset_latch_rw;
				} else if (c == CYCLE_CPU) {
					v = regs.db;
				} else {
					v = regs.chipset_latch_rw >> ((addr & 2) ? 0 : 16);
				}
			} else {
				if (bmdma || (c > CYCLE_REFRESH && c < CYCLE_CPU)) {
					v = regs.chipset_latch_rw;
				} else {
					// refresh checked because refresh cycles do not always
					// set regs.chipset_latch_rw for performance reasons.
					v = 0xffff;
				}
			}
#if CUSTOM_DEBUG > 0
			write_log (_T("%08X read = %04X. Value written=%04X PC=%08x\n"), 0xdff000 | addr, v, l, M68K_GETPC);
#endif
			return v;
		}
	}
	return v;
}

static uae_u32 custom_wget2(uaecptr addr, bool byte)
{
	uae_u32 v;
	int hpos = current_hpos();

	sync_copper(hpos);
	v = custom_wget_1 (hpos, addr, 0, byte);
#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_COMMON
	addr &= 0x1fe;
	ar_custom[addr + 0] = (uae_u8)(v >> 8);
	ar_custom[addr + 1] = (uae_u8)(v);
#endif
#endif
	return v;
}

static uae_u32 REGPARAM2 custom_wget(uaecptr addr)
{
	uae_u32 v;

	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 2, false, 0);
	if (addr & 1) {
#ifdef DEBUGGER
		debug_invalid_reg(addr, 2, 0);
#endif
		/* think about move.w $dff005,d0.. (68020+ only) */
		addr &= ~1;
		v = custom_wget2(addr, false) << 8;
		v |= custom_wget2(addr + 2, false) >> 8;
		return v;
	}
	return custom_wget2(addr, false);
}

static uae_u32 REGPARAM2 custom_bget(uaecptr addr)
{
	uae_u32 v;
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 1, false, 0);
#ifdef DEBUGGER
	debug_invalid_reg(addr, 1, 0);
#endif
	v = custom_wget2(addr & ~1, true);
	v >>= (addr & 1) ? 0 : 8;
	return v;
}

static uae_u32 REGPARAM2 custom_lget (uaecptr addr)
{
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 4, false, 0);
	return ((uae_u32)custom_wget(addr) << 16) | custom_wget (addr + 2);
}
static int REGPARAM2 custom_wput_1 (int hpos, uaecptr addr, uae_u32 value, int noget)
{
	uaecptr oaddr = addr;
	addr &= 0x1FE;
	value &= 0xffff;
	custom_storage[addr >> 1].value = (uae_u16)value;
	custom_storage[addr >> 1].pc = copper_access ? cop_state.ip | 1 : M68K_GETPC;
#ifdef ACTION_REPLAY
#ifdef ACTION_REPLAY_COMMON
	ar_custom[addr + 0]=(uae_u8)(value >> 8);
	ar_custom[addr + 1]=(uae_u8)(value);
#endif
#endif
#ifdef DEBUGGER
	if (memwatch_access_validator) {
		debug_check_reg(oaddr, 1, value);
	}
#endif

	switch (addr) {
	case 0x00E: CLXDAT(hpos); break;

	case 0x020: DSKPTH(value); break;
	case 0x022: DSKPTL(value); break;
	case 0x024: DSKLEN(value, hpos); break;
	case 0x026: /* DSKDAT(value). Writing to DMA write registers won't do anything */; break;
	case 0x028: REFPTR(hpos, value); break;
	case 0x02A: VPOSW(hpos, value); break;
	case 0x02C: VHPOSW(value); break;
	case 0x02E: COPCON(value); break;
#ifdef SERIAL_PORT
	case 0x030: SERDAT(value); break;
	case 0x032: SERPER(value); break;
#else
	case 0x030: break;
	case 0x032: break;
#endif
	case 0x034: POTGO(value); break;

	case 0x038:
		strobe_vblank = 1;
		break;
	case 0x03a:
		if (ecs_denise) {
			strobe_vblank = 1;
		}
		break;
	case 0x03c:
		strobe_vblank = 0;
		break;

	case 0x040: BLTCON0(hpos, value); break;
	case 0x042: BLTCON1(hpos, value); break;

	case 0x044: BLTAFWM(hpos, value); break;
	case 0x046: BLTALWM(hpos, value); break;

	case 0x050: BLTAPTH(hpos, value); break;
	case 0x052: BLTAPTL(hpos, value); break;
	case 0x04C: BLTBPTH(hpos, value); break;
	case 0x04E: BLTBPTL(hpos, value); break;
	case 0x048: BLTCPTH(hpos, value); break;
	case 0x04A: BLTCPTL(hpos, value); break;
	case 0x054: BLTDPTH(hpos, value); break;
	case 0x056: BLTDPTL(hpos, value); break;

	case 0x058: BLTSIZE(hpos, value); break;

	case 0x064: BLTAMOD(hpos, value); break;
	case 0x062: BLTBMOD(hpos, value); break;
	case 0x060: BLTCMOD(hpos, value); break;
	case 0x066: BLTDMOD(hpos, value); break;

	case 0x070: BLTCDAT(hpos, value); break;
	case 0x072: BLTBDAT(hpos, value); break;
	case 0x074: BLTADAT(hpos, value); break;

	case 0x07E: DSKSYNC(hpos, value); break;

	case 0x080: COP1LCH(value); break;
	case 0x082: COP1LCL(value); break;
	case 0x084: COP2LCH(value); break;
	case 0x086: COP2LCL(value); break;

	case 0x088: COPJMP(1, 0); break;
	case 0x08A: COPJMP(2, 0); break;

	case 0x08E: DIWSTRT(hpos, value); break;
	case 0x090: DIWSTOP(hpos, value); break;
	case 0x092: DDFSTRT(hpos, value); break;
	case 0x094: DDFSTOP(hpos, value); break;

	case 0x096: DMACON(hpos, value); break;
	case 0x098: CLXCON(hpos, value); break;
	case 0x09A: INTENA(value); break;
	case 0x09C: INTREQ(value); break;
	case 0x09E: ADKCON(hpos, value); break;

	case 0x0A0: AUDxLCH(0, value); break;
	case 0x0A2: AUDxLCL(0, value); break;
	case 0x0A4: AUDxLEN(0, value); break;
	case 0x0A6: AUDxPER(0, value); break;
	case 0x0A8: AUDxVOL(0, value); break;
	case 0x0AA: AUDxDAT(0, value); break;

	case 0x0B0: AUDxLCH(1, value); break;
	case 0x0B2: AUDxLCL(1, value); break;
	case 0x0B4: AUDxLEN(1, value); break;
	case 0x0B6: AUDxPER(1, value); break;
	case 0x0B8: AUDxVOL(1, value); break;
	case 0x0BA: AUDxDAT(1, value); break;

	case 0x0C0: AUDxLCH(2, value); break;
	case 0x0C2: AUDxLCL(2, value); break;
	case 0x0C4: AUDxLEN(2, value); break;
	case 0x0C6: AUDxPER(2, value); break;
	case 0x0C8: AUDxVOL(2, value); break;
	case 0x0CA: AUDxDAT(2, value); break;

	case 0x0D0: AUDxLCH(3, value); break;
	case 0x0D2: AUDxLCL(3, value); break;
	case 0x0D4: AUDxLEN(3, value); break;
	case 0x0D6: AUDxPER(3, value); break;
	case 0x0D8: AUDxVOL(3, value); break;
	case 0x0DA: AUDxDAT(3, value); break;

	case 0x0E0: BPLxPTH(hpos, value, 0); break;
	case 0x0E2: BPLxPTL(hpos, value, 0); break;
	case 0x0E4: BPLxPTH(hpos, value, 1); break;
	case 0x0E6: BPLxPTL(hpos, value, 1); break;
	case 0x0E8: BPLxPTH(hpos, value, 2); break;
	case 0x0EA: BPLxPTL(hpos, value, 2); break;
	case 0x0EC: BPLxPTH(hpos, value, 3); break;
	case 0x0EE: BPLxPTL(hpos, value, 3); break;
	case 0x0F0: BPLxPTH(hpos, value, 4); break;
	case 0x0F2: BPLxPTL(hpos, value, 4); break;
	case 0x0F4: BPLxPTH(hpos, value, 5); break;
	case 0x0F6: BPLxPTL(hpos, value, 5); break;
	case 0x0F8: BPLxPTH(hpos, value, 6); break;
	case 0x0FA: BPLxPTL(hpos, value, 6); break;
	case 0x0FC: BPLxPTH(hpos, value, 7); break;
	case 0x0FE: BPLxPTL(hpos, value, 7); break;

	case 0x100: BPLCON0(hpos, value); break;
	case 0x102: BPLCON1(hpos, value); break;
	case 0x104: BPLCON2(hpos, value); break;
#ifdef ECS_DENISE
	case 0x106: BPLCON3(hpos, value); break;
#endif

	case 0x108: BPL1MOD(hpos, value); break;
	case 0x10A: BPL2MOD(hpos, value); break;
#ifdef AGA
	case 0x10E: CLXCON2(hpos, value); break;
#endif

	case 0x110: BPLxDAT(hpos, 0, value); break;
	case 0x112: BPLxDAT(hpos, 1, value); break;
	case 0x114: BPLxDAT(hpos, 2, value); break;
	case 0x116: BPLxDAT(hpos, 3, value); break;
	case 0x118: BPLxDAT(hpos, 4, value); break;
	case 0x11A: BPLxDAT(hpos, 5, value); break;
	case 0x11C: BPLxDAT(hpos, 6, value); break;
	case 0x11E: BPLxDAT(hpos, 7, value); break;

	case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
	case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
	case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
	case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
	case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
	case 0x1BC: case 0x1BE:
		COLOR_WRITE(hpos, value & 0x8FFF, (addr & 0x3E) / 2);
		break;
	case 0x120: case 0x124: case 0x128: case 0x12C:
	case 0x130: case 0x134: case 0x138: case 0x13C:
		SPRxPTH(hpos, value, (addr - 0x120) / 4);
		break;
	case 0x122: case 0x126: case 0x12A: case 0x12E:
	case 0x132: case 0x136: case 0x13A: case 0x13E:
		SPRxPTL(hpos, value, (addr - 0x122) / 4);
		break;
	case 0x140: case 0x148: case 0x150: case 0x158:
	case 0x160: case 0x168: case 0x170: case 0x178:
		SPRxPOS(hpos, value, (addr - 0x140) / 8);
		break;
	case 0x142: case 0x14A: case 0x152: case 0x15A:
	case 0x162: case 0x16A: case 0x172: case 0x17A:
		SPRxCTL(hpos, value, (addr - 0x142) / 8);
		break;
	case 0x144: case 0x14C: case 0x154: case 0x15C:
	case 0x164: case 0x16C: case 0x174: case 0x17C:
		SPRxDATA(hpos, value, (addr - 0x144) / 8);
		break;
	case 0x146: case 0x14E: case 0x156: case 0x15E:
	case 0x166: case 0x16E: case 0x176: case 0x17E:
		SPRxDATB(hpos, value, (addr - 0x146) / 8);
		break;

	case 0x36: JOYTEST(value); break;
	case 0x5A: BLTCON0L(hpos, value); break;
	case 0x5C: BLTSIZV(hpos, value); break;
	case 0x5E: BLTSIZH(hpos, value); break;
	case 0x1E4: DIWHIGH(hpos, value); break;
#ifdef AGA
	case 0x10C: BPLCON4(hpos, value); break;
#endif

	case 0x1DC: BEAMCON0(hpos, value); break;
	case 0x1C0:
		if (htotal != value) {
			sync_changes(hpos);
			htotal = value & (MAXHPOS_ROWS - 1);
			varsync(addr, 1, -1);
		}
		break;
	case 0x1C2:
		if (hsstop != value) {
			sync_changes(hpos);
			hsstop = value & (MAXHPOS_ROWS - 1);
			varsync(addr, 1, -1);
		}
		break;
	case 0x1C4:
		if (hbstrt != value) {
			sync_changes(hpos);
			hbstrt = value & 0x7ff;
			varsync(addr, 0, -1);
		}
		break;
	case 0x1C6:
		if (hbstop != value) {
			sync_changes(hpos);
			hbstop = value & 0x7ff;
			varsync(addr, 0, -1);
		}
		break;
	case 0x1C8:
		if (vtotal != value) {
			vtotal = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 1, -1);
		}
		break;
	case 0x1CA:
		if (vsstop != value) {
			sync_changes(hpos);
			vsstop = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 1, -1);
		}
		break;
	case 0x1CC:
		if (vbstrt != value) {
			sync_changes(hpos);
			uae_u16 old = vbstrt;
			vbstrt = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 0, old);
		}
		break;
	case 0x1CE:
		if (vbstop != value) {
			sync_changes(hpos);
			uae_u16 old = vbstop;
			vbstop = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 0, old);
		}
		break;
	case 0x1DE:
		if (hsstrt != value) {
			sync_changes(hpos);
			hsstrt = value & (MAXHPOS_ROWS - 1);
			varsync(addr, 1, -1);
		}
		break;
	case 0x1E0:
		if (vsstrt != value) {
			sync_changes(hpos);
			vsstrt = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 1, -1);
		}
		break;
	case 0x1E2:
		if (hcenter != value) {
			sync_changes(hpos);
			hcenter = value & (MAXHPOS_ROWS - 1);
			varsync(addr, 0, -1);
		}
		break;

	case 0x1D0: SPRHSTRT(hpos, value); break;
	case 0x1D2: SPRHSTOP(hpos, value); break;
	case 0x1D4: BPLHSTRT(hpos, value); break;
	case 0x1D6: BPLHSTOP(hpos, value); break;
	case 0x1D8: HHPOS(hpos, value); break;

#ifdef AGA
	case 0x1FC: FMODE(hpos, value); break;
#endif
	case 0x1FE: FNULL(value); break;

		/* writing to read-only register causes read access */
	default:
		if (!noget) {
#if CUSTOM_DEBUG > 0
			write_log(_T("%04X written %08x\n"), addr, M68K_GETPC);
#endif
			custom_wget_1(hpos, addr, 1, false);
		}
		return 1;
	}
	return 0;
}

static void REGPARAM2 custom_wput(uaecptr addr, uae_u32 value)
{
	int hpos = current_hpos ();

	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 2, value);
		return;
	}
#if CUSTOM_DEBUG > 2
	write_log (_T("%d:%d:wput: %04X %04X pc=%p\n"), hpos, vpos, addr & 0x01fe, value & 0xffff, m68k_getpc ());
#endif
	sync_copper(hpos);
	if (addr & 1) {
#ifdef DEBUGGER
		debug_invalid_reg(addr, -2, value);
#endif
		addr &= ~1;
		custom_wput_1(hpos, addr, (value >> 8) | (value & 0xff00), 0);
		custom_wput_1(hpos, addr + 2, (value << 8) | (value & 0x00ff), 0);
		return;
	}
	custom_wput_1(hpos, addr, value, 0);
}

static void REGPARAM2 custom_bput (uaecptr addr, uae_u32 value)
{
	uae_u16 rval;

	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 1, value);
		return;
	}
#ifdef DEBUGGER
	debug_invalid_reg(addr, -1, value);
#endif
	if (aga_mode) {
		if (addr & 1) {
			rval = value & 0xff;
		} else {
			rval = (value << 8) | (value & 0xff);
		}
	} else {
		rval = (value << 8) | (value & 0xff);
	}

	if (currprefs.cs_bytecustomwritebug) {
		if (addr & 1)
			custom_wput(addr & ~1, rval | (rval << 8));
		else
			custom_wput(addr, value << 8);
	} else {
		custom_wput(addr & ~1, rval);
	}
}

static void REGPARAM2 custom_lput (uaecptr addr, uae_u32 value)
{
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 4, value);
		return;
	}
	custom_wput (addr & 0xfffe, value >> 16);
	custom_wput ((addr + 2) & 0xfffe, (uae_u16)value);
}

#ifdef SAVESTATE

void custom_prepare_savestate(void)
{
	if (!currprefs.cpu_cycle_exact) {
		for (int i = 0; i < ev2_max; i++) {
			if (eventtab2[i].active) {
				eventtab2[i].active = 0;
				eventtab2[i].handler(eventtab2[i].data);
			}
		}
	}
}

void restore_custom_finish(void)
{
	if ((bplcon0 & 2) && !currprefs.genlock) {
		changed_prefs.genlock = currprefs.genlock = 1;
		write_log(_T("statefile with BPLCON0 ERSY set without Genlock. Enabling Genlock.\n"));
	}
}

void restore_custom_start(void)
{
	memset(&cop_state, 0, sizeof(cop_state));
	cop_state.state = COP_stop;
}

#define RB restore_u8()
#define SRB (uae_s8)restore_u8()
#define RBB restore_u8() != 0
#define RW restore_u16()
#define RL restore_u32()

uae_u8 *restore_custom(uae_u8 *src)
{
	uae_u16 dsklen, dskbytr;
	int dskpt;
	int i;

	audio_reset();

	changed_prefs.chipset_mask = currprefs.chipset_mask = RL & CSMASK_MASK;
	update_mirrors();
	blt_info.bltddat = RW;	/* 000 BLTDDAT */
	RW;						/* 002 DMACONR */
	RW;						/* 004 VPOSR */
	RW;						/* 006 VHPOSR */
	RW;						/* 008 DSKDATR (dummy register) */
	JOYSET(0, RW);		/* 00A JOY0DAT */
	JOYSET(1, RW);		/* 00C JOY1DAT */
	clxdat = RW;			/* 00E CLXDAT */
	RW;						/* 010 ADKCONR */
	RW;						/* 012 POT0DAT* */
	RW;						/* 014 POT1DAT* */
	RW;						/* 016 POTINP* */
	RW;						/* 018 SERDATR* */
	dskbytr = RW;			/* 01A DSKBYTR */
	RW;						/* 01C INTENAR */
	RW;						/* 01E INTREQR */
	dskpt = RL;				/* 020-022 DSKPT */
	dsklen = RW;			/* 024 DSKLEN */
	RW;						/* 026 DSKDAT */
	refptr = RW;			/* 028 REFPTR */
	i = RW; lof_store = lof_display = (i & 0x8000) ? 1 : 0; lol = (i & 0x0080) ? 1 : 0; /* 02A VPOSW */
	RW;						/* 02C VHPOSW */
	COPCON(RW);			/* 02E COPCON */
	RW;						/* 030 SERDAT* */
#ifdef SERIAL_PORT
	serper = RW;			/* 032 SERPER* */
#else
	RW;						/* 032 SERPER* */
#endif
	potgo_value = 0; POTGO(RW); /* 034 POTGO */
	RW;						/* 036 JOYTEST* */
	RW;						/* 038 STREQU */
	RW;						/* 03A STRVHBL */
	RW;						/* 03C STRHOR */
	RW;						/* 03E STRLONG */
	BLTCON0(0, RW);			/* 040 BLTCON0 */
	BLTCON1(0, RW);			/* 042 BLTCON1 */
	BLTAFWM(0, RW);			/* 044 BLTAFWM */
	BLTALWM(0, RW);			/* 046 BLTALWM */
	BLTCPTH(0, RW);BLTCPTL(0, RW);	/* 048-04B BLTCPT */
	BLTBPTH(0, RW);BLTBPTL(0, RW);	/* 04C-04F BLTBPT */
	BLTAPTH(0, RW);BLTAPTL(0, RW);	/* 050-053 BLTAPT */
	BLTDPTH(0, RW);BLTDPTL(0, RW);	/* 054-057 BLTDPT */
	RW;						/* 058 BLTSIZE */
	RW;						/* 05A BLTCON0L */
	blt_info.vblitsize = RW;/* 05C BLTSIZV */
	blt_info.hblitsize = RW;/* 05E BLTSIZH */
	BLTCMOD(0, RW);			/* 060 BLTCMOD */
	BLTBMOD(0, RW);			/* 062 BLTBMOD */
	BLTAMOD(0, RW);			/* 064 BLTAMOD */
	BLTDMOD(0, RW);			/* 066 BLTDMOD */
	RW;						/* 068 ? */
	RW;						/* 06A ? */
	RW;						/* 06C ? */
	RW;						/* 06E ? */
	BLTCDAT(0, RW);			/* 070 BLTCDAT */
	BLTBDAT(0, RW);			/* 072 BLTBDAT */
	BLTADAT(0, RW);			/* 074 BLTADAT */
	RW;						/* 076 ? */
	RW;						/* 078 ? */
	RW;						/* 07A ? */
	RW;						/* 07C LISAID */
	DSKSYNC(-1, RW);		/* 07E DSKSYNC */
	cop1lc = RL;			/* 080/082 COP1LC */
	cop2lc = RL;			/* 084/086 COP2LC */
	RW;						/* 088 ? */
	RW;						/* 08A ? */
	RW;						/* 08C ? */
	diwstrt = RW;			/* 08E DIWSTRT */
	diwstop = RW;			/* 090 DIWSTOP */
	ddfstrt = RW;			/* 092 DDFSTRT */
	ddfstop = RW;			/* 094 DDFSTOP */
	dmacon = RW & ~(0x2000|0x4000); /* 096 DMACON */
	CLXCON(-1, RW);			/* 098 CLXCON */
	intena = RW;			/* 09A INTENA */
	intreq = RW;			/* 09C INTREQ */
	adkcon = RW;			/* 09E ADKCON */
	for (i = 0; i < 8; i++)
		bplptx[i] = bplpt[i] = RL;
	bplcon0 = RW;			/* 100 BPLCON0 */
	bplcon1 = RW;			/* 102 BPLCON1 */
	bplcon2 = RW;			/* 104 BPLCON2 */
	bplcon3 = RW;			/* 106 BPLCON3 */
	bpl1mod = RW;			/* 108 BPL1MOD */
	bpl2mod = RW;			/* 10A BPL2MOD */
	bplcon4 = RW;			/* 10C BPLCON4 */
	clxcon2 = RW;			/* 10E CLXCON2* */
	for (i = 0; i < 8; i++) {
		fetched[i] = RW;	/*     BPLXDAT */
		fetched_aga[i] = fetched[i];
	}
	for (i = 0; i < 32; i++) {
		uae_u16 v = RW;
		color_regs_genlock[i] = (v & 0x8000) != 0;
		current_colors.color_regs_ecs[i] = v & 0xfff; /* 180 COLORxx */
	}
	htotal = RW;			/* 1C0 HTOTAL */
	hsstop = RW;			/* 1C2 HSTOP ? */
	hbstrt = RW;			/* 1C4 HBSTRT ? */
	hbstop = RW;			/* 1C6 HBSTOP ? */
	vtotal = RW;			/* 1C8 VTOTAL */
	vsstop = RW;			/* 1CA VSSTOP */
	vbstrt = RW;			/* 1CC VBSTRT */
	vbstop = RW;			/* 1CE VBSTOP */
	SPRHSTRT(-1, RW);		/* 1D0 SPRHSTART */
	SPRHSTOP(-1, RW);		/* 1D2 SPRHSTOP */
	BPLHSTRT(-1, RW);		/* 1D4 BPLHSTRT */
	BPLHSTOP(-1, RW);		/* 1D6 BPLHSTOP */
	hhpos = RW;				/* 1D8 HHPOSW */
	RW;						/* 1DA HHPOSR */
	new_beamcon0 = RW;		/* 1DC BEAMCON0 */
	hsstrt = RW;			/* 1DE HSSTRT */
	vsstrt = RW;			/* 1E0 VSSTT  */
	hcenter = RW;			/* 1E2 HCENTER */
	diwhigh = RW;			/* 1E4 DIWHIGH */
	diwhigh_written = (diwhigh & 0x8000) ? 1 : 0;
	hdiwstate = (diwhigh & 0x4000) ? diw_states::DIW_waiting_stop : diw_states::DIW_waiting_start;
	vdiwstate = (diwhigh & 0x0080) ? diw_states::DIW_waiting_start : diw_states::DIW_waiting_stop;
	diwhigh &= 0x3f3f;
	RW;						/* 1E6 ? */
	RW;						/* 1E8 ? */
	RW;						/* 1EA ? */
	RW;						/* 1EC ? */
	RW;						/* 1EE ? */
	RW;						/* 1F0 ? */
	RW;						/* 1F2 ? */
	RW;						/* 1F4 ? */
	RW;						/* 1F6 ? */
	RW;						/* 1F8 ? */
	i = RW;					/* 1FA ? */
	if (i & 0x8000) {
		currprefs.ntscmode = changed_prefs.ntscmode = i & 1;
	}
	fmode = RW;				/* 1FC FMODE */
	last_custom_value = RW; /* 1FE ? */
	refptr = RL;			/* full refresh pointer */

	bplcon0_saved = bplcon0;
	bplcon1_saved = bplcon1;
	bplcon2_saved = bplcon2;
	bplcon3_saved = bplcon3;
	bplcon4_saved = bplcon4;
	fmode_saved = fmode;
	beamcon0_saved = new_beamcon0;
	bplcon0d = bplcon0;
	bplcon0d_old = 0;
	bitplane_dma_change(dmacon);
	vdiw_change(vdiwstate == diw_states::DIW_waiting_stop);

	intreq2 = intreq;
	intena2 = intena;

	current_colors.extra = 0;
	if (isbrdblank(-1, bplcon0, bplcon3)) {
		current_colors.extra |= 1 << CE_BORDERBLANK;
	}
	if (issprbrd(-1, bplcon0, bplcon3)) {
		current_colors.extra |= 1 << CE_BORDERSPRITE;
	}
	if (ecs_denise && (bplcon0 & 1) && (bplcon3 & 0x10)) {
		current_colors.extra |= 1 << CE_BORDERNTRANS;
	}
	setextblank();

	lof_prev_lastline = lof_lastline = lof_store != 0;

	DISK_restore_custom(dskpt, dsklen, dskbytr);

	return src;
}

#endif /* SAVESTATE */

#if defined SAVESTATE || defined DEBUGGER

#define SB save_u8
#define SW save_u16
#define SL save_u32

uae_u8 *save_custom(size_t *len, uae_u8 *dstptr, int full)
{
	uae_u8 *dstbak, *dst;
	int i, dummy;
	uae_u32 dskpt;
	uae_u16 dsklen, dsksync, dskbytr;
	uae_u16 v;

	DISK_save_custom(&dskpt, &dsklen, &dsksync, &dskbytr);

	if (dstptr) {
		dstbak = dst = dstptr;
	} else {
		dstbak = dst = xmalloc(uae_u8, 8 + 256 * 2);
	}

	SL(currprefs.chipset_mask);
	SW(blt_info.bltddat);	/* 000 BLTDDAT */
	SW(dmacon);				/* 002 DMACONR */
	SW(VPOSR());			/* 004 VPOSR */
	SW(VHPOSR());			/* 006 VHPOSR */
	SW(0);					/* 008 DSKDATR */
	SW(JOYGET(0));			/* 00A JOY0DAT */
	SW(JOYGET(1));			/* 00C JOY1DAT */
	SW(clxdat | 0x8000);	/* 00E CLXDAT */
	SW(ADKCONR());			/* 010 ADKCONR */
	SW(POT0DAT());			/* 012 POT0DAT */
	SW(POT1DAT());			/* 014 POT1DAT */
	SW(0);					/* 016 POTINP * */
	SW(0);					/* 018 SERDATR * */
	SW(dskbytr);			/* 01A DSKBYTR */
	SW(INTENAR());			/* 01C INTENAR */
	SW(INTREQR());			/* 01E INTREQR */
	SL(dskpt);				/* 020-023 DSKPT */
	SW(dsklen);				/* 024 DSKLEN */
	SW(0);					/* 026 DSKDAT */
	SW(refptr);				/* 028 REFPTR */
	SW((lof_store ? 0x8001 : 0) | (lol ? 0x0080 : 0));/* 02A VPOSW */
	SW(0);					/* 02C VHPOSW */
	SW(copcon);				/* 02E COPCON */
#ifdef SERIAL_PORT
	SW(serdat);				/* 030 SERDAT * */
	SW(serper);				/* 032 SERPER * */
#else
	SW(0);					/* 030 SERDAT * */
	SW(0);					/* 032 SERPER * */
#endif
	SW(potgo_value);		/* 034 POTGO */
	SW(0);					/* 036 JOYTEST * */
	SW(0);					/* 038 STREQU */
	SW(0);					/* 03A STRVBL */
	SW(0);					/* 03C STRHOR */
	SW(0);					/* 03E STRLONG */
	SW(bltcon0);			/* 040 BLTCON0 */
	SW(bltcon1);			/* 042 BLTCON1 */
	SW(blt_info.bltafwm);	/* 044 BLTAFWM */
	SW(blt_info.bltalwm);	/* 046 BLTALWM */
	SL(bltcpt);				/* 048-04B BLTCPT */
	SL(bltbpt);				/* 04C-04F BLTCPT */
	SL(bltapt);				/* 050-053 BLTCPT */
	SL(bltdpt);				/* 054-057 BLTCPT */
	if (blt_info.vblitsize > 1024 || blt_info.hblitsize > 64) {
		v = 0;
	} else {
		v = (blt_info.vblitsize << 6) | (blt_info.hblitsize & 63);
	}
	SW(v);					/* 058 BLTSIZE */
	SW(bltcon0 & 0xff);		/* 05A BLTCON0L (use BLTCON0 instead) */
	SW(blt_info.vblitsize);	/* 05C BLTSIZV */
	SW(blt_info.hblitsize);	/* 05E BLTSIZH */
	SW(blt_info.bltcmod);	/* 060 BLTCMOD */
	SW(blt_info.bltbmod);	/* 062 BLTBMOD */
	SW(blt_info.bltamod);	/* 064 BLTAMOD */
	SW(blt_info.bltdmod);	/* 066 BLTDMOD */
	SW(0);					/* 068 ? */
	SW(0);					/* 06A ? */
	SW(0);					/* 06C ? */
	SW(0);					/* 06E ? */
	SW(blt_info.bltcdat);	/* 070 BLTCDAT */
	SW(blt_info.bltbdat);	/* 072 BLTBDAT */
	SW(blt_info.bltadat);	/* 074 BLTADAT */
	SW(0);					/* 076 ? */
	SW(0);					/* 078 ? */
	SW(0);					/* 07A ? */
	SW(DENISEID(&dummy));	/* 07C DENISEID/LISAID */
	SW(dsksync);			/* 07E DSKSYNC */
	SL(cop1lc);				/* 080-083 COP1LC */
	SL(cop2lc);				/* 084-087 COP2LC */
	SW(0);					/* 088 ? */
	SW(0);					/* 08A ? */
	SW(0);					/* 08C ? */
	SW(diwstrt);			/* 08E DIWSTRT */
	SW(diwstop);			/* 090 DIWSTOP */
	SW(ddfstrt);			/* 092 DDFSTRT */
	SW(ddfstop);			/* 094 DDFSTOP */
	SW(dmacon);				/* 096 DMACON */
	SW(clxcon);				/* 098 CLXCON */
	SW(intena);				/* 09A INTENA */
	SW(intreq);				/* 09C INTREQ */
	SW(adkcon);				/* 09E ADKCON */
	for (i = 0; full && i < 32; i++)
		SW(0);
	for (i = 0; i < 8; i++)
		SL(bplpt[i]);		/* 0E0-0FE BPLxPT */
	SW(bplcon0);			/* 100 BPLCON0 */
	SW(bplcon1);			/* 102 BPLCON1 */
	SW(bplcon2);			/* 104 BPLCON2 */
	SW(bplcon3);			/* 106 BPLCON3 */
	SW(bpl1mod);			/* 108 BPL1MOD */
	SW(bpl2mod);			/* 10A BPL2MOD */
	SW(bplcon4);			/* 10C BPLCON4 */
	SW(clxcon2);			/* 10E CLXCON2 */
	for (i = 0;i < 8; i++)
		SW (fetched[i]);	/* 110 BPLxDAT */
	if (full) {
		for (i = 0; i < 8; i++) {
			SL (spr[i].pt);	/* 120-13E SPRxPT */
		}
		for (i = 0; i < 8; i++) {
			struct sprite *s = &spr[i];
			SW(s->pos);	/* 1x0 SPRxPOS */
			SW(s->ctl);	/* 1x2 SPRxPOS */
			SW(s->data[0]);	/* 1x4 SPRxDATA */
			SW(s->datb[0]);	/* 1x6 SPRxDATB */
		}
	}
	for (i = 0; i < 32; i++) {
		if (aga_mode) {
			uae_u32 v = current_colors.color_regs_aga[i];
			uae_u16 v2;
			v &= 0x00f0f0f0;
			v2 = (v >> 4) & 15;
			v2 |= ((v >> 12) & 15) << 4;
			v2 |= ((v >> 20) & 15) << 8;
			SW(v2);
		} else {
			uae_u16 v = current_colors.color_regs_ecs[i];
			if (color_regs_genlock[i])
				v |= 0x8000;
			SW(v); /* 180-1BE COLORxx */
		}
	}
	SW(htotal);			/* 1C0 HTOTAL */
	SW(hsstop);			/* 1C2 HSTOP*/
	SW(hbstrt);			/* 1C4 HBSTRT */
	SW(hbstop);			/* 1C6 HBSTOP */
	SW(vtotal);			/* 1C8 VTOTAL */
	SW(vsstop);			/* 1CA VSSTOP */
	SW(vbstrt);			/* 1CC VBSTRT */
	SW(vbstop);			/* 1CE VBSTOP */
	SW(sprhstrt);		/* 1D0 SPRHSTRT */
	SW(sprhstop);		/* 1D2 SPRHSTOP */
	SW(bplhstrt);		/* 1D4 BPLHSTRT */
	SW(bplhstop);		/* 1D6 BPLHSTOP */
	SW(hhpos);			/* 1D8 HHPOSW */
	SW(HHPOSR());		/* 1DA HHPOSR */
	SW(new_beamcon0);	/* 1DC BEAMCON0 */
	SW(hsstrt);			/* 1DE HSSTRT */
	SW(vsstrt);			/* 1E0 VSSTRT */
	SW(hcenter);		/* 1E2 HCENTER */
	SW(diwhigh | (diwhigh_written ? 0x8000 : 0) | (hdiwstate == diw_states::DIW_waiting_stop ? 0x4000 : 0) | (vdiwstate == diw_states::DIW_waiting_start ? 0x0080 : 0)); /* 1E4 DIWHIGH */
	SW(0);				/* 1E6 */
	SW(0);				/* 1E8 */
	SW(0);				/* 1EA */
	SW(0);				/* 1EC */
	SW(0);				/* 1EE */
	SW(0);				/* 1F0 */
	SW(0);				/* 1F2 */
	SW(0);				/* 1F4 */
	SW(0);				/* 1F6 */
	SW(0);				/* 1F8 */
	SW(0x8000 | (currprefs.ntscmode ? 1 : 0));			/* 1FA (re-used for NTSC) */
	SW(fmode);			/* 1FC FMODE */
	SW(last_custom_value);	/* 1FE */
	SL(refptr);

	*len = dst - dstbak;
	return dstbak;
}

#endif /* SAVESTATE || DEBUGGER */

#ifdef SAVESTATE

uae_u8 *restore_custom_agacolors(uae_u8 *src)
{
	int i;

	for (i = 0; i < 256; i++) {
#ifdef AGA
		uae_u32 v = RL;
		color_regs_genlock[i] = 0;
		if (v & 0x80000000) {
			color_regs_genlock[i] = 1;
		}
		v &= 0xffffff;
		current_colors.color_regs_aga[i] = v | (color_regs_genlock[i] ? COLOR_CHANGE_GENLOCK : 0);
#else
		RL;
#endif
	}
	return src;
}

uae_u8 *save_custom_agacolors(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (!aga_mode) {
		int i;
		for (i = 0; i < 256; i++) {
			if (current_colors.color_regs_aga[i] || color_regs_genlock[i])
				break;
		}
		if (i == 256)
			return NULL;
	}

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 256 * 4);
	for (int i = 0; i < 256; i++)
#ifdef AGA
		SL ((current_colors.color_regs_aga[i] & 0xffffff) | (color_regs_genlock[i] ? 0x80000000 : 0));
#else
		SL (0);
#endif
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_custom_sprite(int num, uae_u8 *src)
{
	struct sprite *s = &spr[num];
	memset (s, 0, sizeof (struct sprite));
	s->pt = RL;		/* 120-13E SPRxPT */
	s->pos = RW;		/* 1x0 SPRxPOS */
	s->ctl = RW;		/* 1x2 SPRxPOS */
	s->data[0] = RW;	/* 1x4 SPRxDATA */
	s->datb[0] = RW;	/* 1x6 SPRxDATB */
	s->data[1] = RW;
	s->datb[1] = RW;
	s->data[2] = RW;
	s->datb[2] = RW;
	s->data[3] = RW;
	s->datb[3] = RW;
	s->armed = RB & 1;
	return src;
}

uae_u8 *save_custom_sprite(int num, size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	struct sprite *s = &spr[num];

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 30);
	SL(s->pt);		/* 120-13E SPRxPT */
	SW(s->pos);		/* 1x0 SPRxPOS */
	SW(s->ctl);		/* 1x2 SPRxPOS */
	SW(s->data[0]);	/* 1x4 SPRxDATA */
	SW(s->datb[0]);	/* 1x6 SPRxDATB */
	SW(s->data[1]);
	SW(s->datb[1]);
	SW(s->data[2]);
	SW(s->datb[2]);
	SW(s->data[3]);
	SW(s->datb[3]);
	SB(s->armed ? 1 : 0);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_custom_extra(uae_u8 *src)
{
	uae_u32 v = restore_u32();
	uae_u8 tmp = 0;

	if (!(v & 1))
		v = 0;
	currprefs.cs_compatible = changed_prefs.cs_compatible = v >> 24;
	cia_set_overlay((v & 2) != 0);

	currprefs.genlock = changed_prefs.genlock = RBB;
	currprefs.cs_rtc = changed_prefs.cs_rtc = RB;
	RL; // currprefs.cs_rtc_adjust = changed_prefs.cs_rtc_adjust = RL;

	currprefs.cs_a1000ram = changed_prefs.cs_a1000ram = RBB;

	//currprefs.a2091rom.enabled = changed_prefs.a2091rom.enabled = RBB;
	//currprefs.a4091rom.enabled = changed_prefs.a4091rom.enabled = RBB;
	tmp = RBB;
	tmp = RBB;
	tmp = RBB;

	currprefs.cs_pcmcia = changed_prefs.cs_pcmcia = RBB;
	currprefs.cs_ciaatod = changed_prefs.cs_ciaatod = RB;
	currprefs.cs_ciaoverlay = changed_prefs.cs_ciaoverlay = RBB;

	tmp = RBB;
	tmp = RBB;

	currprefs.cs_agnusrev = changed_prefs.cs_agnusrev = SRB;
	currprefs.cs_deniserev = changed_prefs.cs_deniserev = SRB;
	currprefs.cs_fatgaryrev = changed_prefs.cs_fatgaryrev = SRB;
	currprefs.cs_ramseyrev = changed_prefs.cs_ramseyrev = SRB;

	currprefs.cs_cd32c2p = changed_prefs.cs_cd32c2p = RBB;
	currprefs.cs_cd32cd = changed_prefs.cs_cd32cd = RBB;
	currprefs.cs_cd32nvram = changed_prefs.cs_cd32nvram = RBB;
	currprefs.cs_cdtvcd = changed_prefs.cs_cdtvcd = RBB;
	currprefs.cs_cdtvram = changed_prefs.cs_cdtvram = RBB;
	RB;

	currprefs.cs_df0idhw = changed_prefs.cs_df0idhw = RBB;
	tmp = RBB;
	currprefs.cs_ide = changed_prefs.cs_ide = RB;
	currprefs.cs_mbdmac = changed_prefs.cs_mbdmac = RB;
	currprefs.cs_ksmirror_a8 = changed_prefs.cs_ksmirror_a8 = RBB;
	currprefs.cs_ksmirror_e0 = changed_prefs.cs_ksmirror_e0 = RBB;
	currprefs.cs_resetwarning = changed_prefs.cs_resetwarning = RBB;
	currprefs.cs_z3autoconfig = changed_prefs.cs_z3autoconfig = RBB;
	currprefs.cs_1mchipjumper = changed_prefs.cs_1mchipjumper = RBB;
	currprefs.cs_bytecustomwritebug = changed_prefs.cs_bytecustomwritebug = RBB;
	currprefs.cs_color_burst = changed_prefs.cs_color_burst = RBB;
	currprefs.cs_toshibagary = changed_prefs.cs_toshibagary = RBB;
	currprefs.cs_romisslow = changed_prefs.cs_romisslow = RBB;

	currprefs.cs_ciatype[0] = changed_prefs.cs_ciatype[0] = RBB;
	currprefs.cs_ciatype[1] = changed_prefs.cs_ciatype[1] = RBB;

	currprefs.cs_memorypatternfill = changed_prefs.cs_memorypatternfill = RBB;

	currprefs.cs_agnusmodel = changed_prefs.cs_agnusmodel = RBB;
	currprefs.cs_agnussize = changed_prefs.cs_agnussize = RBB;
	currprefs.cs_denisemodel = changed_prefs.cs_denisemodel = RBB;

	return src;
}

uae_u8 *save_custom_extra(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);

	SL((currprefs.cs_compatible << 24) | (get_mem_bank_real(0) != &chipmem_bank ? 2 : 0) | 1);
	SB(currprefs.genlock ? 1 : 0);
	SB(currprefs.cs_rtc);
	SL(currprefs.cs_rtc_adjust);

	SB(currprefs.cs_a1000ram ? 1 : 0);

	SB(is_board_enabled(&currprefs, ROMTYPE_A2091, 0) ? 1 : 0);
	SB(is_board_enabled(&currprefs, ROMTYPE_A4091, 0) ? 1 : 0);
	SB(0);

	SB(currprefs.cs_pcmcia ? 1 : 0);
	SB(currprefs.cs_ciaatod);
	SB(currprefs.cs_ciaoverlay ? 1 : 0);

	SB(agnusa1000 ? 1 : 0);
	SB(denisea1000_noehb ? 1 : 0);

	SB(currprefs.cs_agnusrev);
	SB(currprefs.cs_deniserev);
	SB(currprefs.cs_fatgaryrev);
	SB(currprefs.cs_ramseyrev);

	SB(currprefs.cs_cd32c2p);
	SB(currprefs.cs_cd32cd);
	SB(currprefs.cs_cd32nvram);
	SB(currprefs.cs_cdtvcd ? 1 : 0);
	SB(currprefs.cs_cdtvram ? 1 : 0);
	SB(0);

	SB(currprefs.cs_df0idhw ? 1 : 0);
	SB(agnusa1000 ? 1 : 0);
	SB(currprefs.cs_ide);
	SB(currprefs.cs_mbdmac);
	SB(currprefs.cs_ksmirror_a8 ? 1 : 0);
	SB(currprefs.cs_ksmirror_e0 ? 1 : 0);
	SB(currprefs.cs_resetwarning ? 1 : 0);
	SB(currprefs.cs_z3autoconfig ? 1 : 0);
	SB(currprefs.cs_1mchipjumper ? 1 : 0);

	SB(currprefs.cs_bytecustomwritebug ? 1 : 0);
	SB(currprefs.cs_color_burst ? 1 : 0);
	SB(currprefs.cs_toshibagary ? 1 : 0);
	SB(currprefs.cs_romisslow ? 1 : 0);

	SB(currprefs.cs_ciatype[0]);
	SB(currprefs.cs_ciatype[1]);

	SB(currprefs.cs_memorypatternfill);

	SB(currprefs.cs_agnusmodel);
	SB(currprefs.cs_agnussize);
	SB(currprefs.cs_denisemodel);

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_custom_slots(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);

	uae_u32 v = 1;
	// copper final MOVE pending?
	if (cop_state.state == COP_read1) {
		v |= 2;
	} else if (cop_state.state == COP_read2) {
		v |= 4;
	}
	save_u32(v);
	save_u32(cop_state.ip);
	save_u16(cop_state.ir[0]);
	save_u16(cop_state.ir[1]);

	for (int i = 0; i < 4; i++) {
		save_u16(cycle_line_pipe[i]);
		save_u16(cycle_line_slot[i]);
	}

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_custom_slots(uae_u8 *src)
{
	uae_u32 v = restore_u32();
	if (!(v & 1))
		return src;

	cop_state.ip = restore_u32();
	cop_state.ir[0] = restore_u16();
	cop_state.ir[1] = restore_u16();
	cop_state.state = COP_stop;
	if (v & 2) {
		cop_state.state = COP_read1;
	} else if (v & 4) {
		cop_state.state = COP_read2;
	}

	for (int i = 0; i < 4; i++) {
		cycle_line_pipe[i] = restore_u16();
		cycle_line_slot[i] = (uae_u8)restore_u16();
	}

	return src;
}

uae_u8 *restore_custom_event_delay(uae_u8 *src)
{
	if (restore_u32() != 1)
		return src;
	int cnt = restore_u8();
	for (int i = 0; i < cnt; i++) {
		uae_u8 type = restore_u8();
		evt_t e = restore_u64();
		uae_u32 data = restore_u32();
		evfunc2 f = NULL;
		switch(type)
		{
			case 1:
				f = event_send_interrupt_do_ext;
				break;
			case 2:
				f = event_doint_delay_do_ext;
				break;
			case 3:
				f = event_audxdat_func;
				break;
			case 4:
				f = event_setdsr;
				break;
			case 5:
				f = event_CIA_synced_interrupt;
				break;
			case 6:
				f = event_doint_delay_do_intreq;
				break;
			case 7:
				f = event_doint_delay_do_intena;
				break;
			case 8:
				f = event_CIA_tod_inc_event;
				break;
			case 9:
				f = event_DISK_handler;
				break;
			case 10:
				f = bitplane_dma_change;
				break;
			case 0:
				write_log("ignored event type %d (%08x) restored\n", type, data);
				break;
			default:
				write_log("unknown event type %d (%08x) restored\n", type, data);
				break;
		}
		if (f) {
			event2_newevent_xx(-1, e, data, f);
		}
	}
	return src;
}
uae_u8 *save_custom_event_delay(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	int cnt = 0, cnt2 = 0;

	for (int i = ev2_misc; i < ev2_max; i++) {
		struct ev2 *e = &eventtab2[i];
		if (e->active) {
			cnt++;
		}
	}
	if (cnt == 0)
		return NULL;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);

	save_u32(1);
	save_u8(cnt);
	for (int i = ev2_misc; i < ev2_max; i++) {
		struct ev2 *e = &eventtab2[i];
		if (e->active) {
			evfunc2 f = e->handler;
			uae_u8 type = 0;
			if (f == event_send_interrupt_do_ext) {
				type = 1;
			} else if (f == event_doint_delay_do_ext) {
				type = 2;
			} else if (f == event_audxdat_func) {
				type = 3;
			} else if (f == event_setdsr) {
				type = 4;
			} else if (f == event_CIA_synced_interrupt) {
				type = 5;
			} else if (f == event_doint_delay_do_intreq) {
				type = 6;
			} else if (f == event_doint_delay_do_intena) {
				type = 7;
			} else if (f == event_CIA_tod_inc_event) {
				type = 8;
			} else if (f == event_DISK_handler) {
				type = 9;
			} else if (f == bitplane_dma_change) {
				type = 10;
			} else {
				write_log("unknown event2 handler %p\n", e->handler);
				e->active = false;
				f(e->data);
			}
			if (type) {
				cnt2++;
			}
			save_u8(type);
			save_u64(e->evtime - get_cycles());
			save_u32(e->data);
		}
	}
	write_log("%d pending events saved\n", cnt2);

	*len = dst - dstbak;
	return dstbak;
}


uae_u8 *save_cycles(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);
	save_u32(1);
	save_u32(CYCLE_UNIT);
	save_u64(get_cycles());
	save_u32(extra_cycle);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_cycles(uae_u8 *src)
{
	if (restore_u32() != 1)
		return src;
	restore_u32();
	start_cycles = restore_u64();
	extra_cycle = restore_u32();
	if (extra_cycle >= 2 * CYCLE_UNIT)
		extra_cycle = 0;
	set_cycles(start_cycles);
	clear_events();
	return src;
}

#endif /* SAVESTATE */

void check_prefs_changed_custom(void)
{
	bool syncchange = false;

	if (!config_changed)
		return;
	currprefs.gfx_framerate = changed_prefs.gfx_framerate;
	if (currprefs.turbo_emulation_limit != changed_prefs.turbo_emulation_limit) {
		currprefs.turbo_emulation_limit = changed_prefs.turbo_emulation_limit;
		if (changed_prefs.turbo_emulation) {
			warpmode(changed_prefs.turbo_emulation);
		}
	}
	if (currprefs.turbo_emulation != changed_prefs.turbo_emulation)
		warpmode(changed_prefs.turbo_emulation);
	if (inputdevice_config_change_test()) 
		inputdevice_copyconfig (&changed_prefs, &currprefs);
	currprefs.immediate_blits = changed_prefs.immediate_blits;
	currprefs.waiting_blits = changed_prefs.waiting_blits;
	currprefs.blitter_speed_throttle = changed_prefs.blitter_speed_throttle;
	currprefs.collision_level = changed_prefs.collision_level;
	currprefs.keyboard_nkro = changed_prefs.keyboard_nkro;
	if (currprefs.keyboard_mode != changed_prefs.keyboard_mode) {
		currprefs.keyboard_mode = changed_prefs.keyboard_mode;
		// send powerup sync
		keyboard_connected(true);
	}
	else if (currprefs.keyboard_mode >= 0 && changed_prefs.keyboard_mode < 0) {
		currprefs.keyboard_mode = changed_prefs.keyboard_mode;
		keyboard_connected(false);
	}

	currprefs.cs_ciaatod = changed_prefs.cs_ciaatod;
	currprefs.cs_rtc = changed_prefs.cs_rtc;
	currprefs.cs_cd32cd = changed_prefs.cs_cd32cd;
	currprefs.cs_cd32c2p = changed_prefs.cs_cd32c2p;
	currprefs.cs_cd32nvram = changed_prefs.cs_cd32nvram;
	currprefs.cs_cdtvcd = changed_prefs.cs_cdtvcd;
	currprefs.cs_ide = changed_prefs.cs_ide;
	currprefs.cs_pcmcia = changed_prefs.cs_pcmcia;
	currprefs.cs_fatgaryrev = changed_prefs.cs_fatgaryrev;
	currprefs.cs_ramseyrev = changed_prefs.cs_ramseyrev;
	currprefs.cs_agnusrev = changed_prefs.cs_agnusrev;
	currprefs.cs_deniserev = changed_prefs.cs_deniserev;
	currprefs.cs_mbdmac = changed_prefs.cs_mbdmac;
	currprefs.cs_df0idhw = changed_prefs.cs_df0idhw;
	currprefs.cs_agnussize = changed_prefs.cs_agnussize;
	currprefs.cs_z3autoconfig = changed_prefs.cs_z3autoconfig;
	currprefs.cs_bytecustomwritebug = changed_prefs.cs_bytecustomwritebug;
	currprefs.cs_color_burst = changed_prefs.cs_color_burst;
	currprefs.cs_romisslow = changed_prefs.cs_romisslow;
	currprefs.cs_toshibagary = changed_prefs.cs_toshibagary;
	currprefs.cs_unmapped_space = changed_prefs.cs_unmapped_space;
	currprefs.cs_eclockphase = changed_prefs.cs_eclockphase;
	currprefs.cs_eclocksync = changed_prefs.cs_eclocksync;
	currprefs.cs_ciatype[0] = changed_prefs.cs_ciatype[0];
	currprefs.cs_ciatype[1] = changed_prefs.cs_ciatype[1];
	currprefs.cs_memorypatternfill = changed_prefs.cs_memorypatternfill;

	if (currprefs.chipset_mask != changed_prefs.chipset_mask ||
		currprefs.picasso96_nocustom != changed_prefs.picasso96_nocustom ||
		currprefs.ntscmode != changed_prefs.ntscmode ||
		currprefs.cs_agnusmodel != changed_prefs.cs_agnusmodel ||
		currprefs.cs_denisemodel != changed_prefs.cs_denisemodel ||
		currprefs.cs_hvcsync != changed_prefs.cs_hvcsync
		) {
			currprefs.picasso96_nocustom = changed_prefs.picasso96_nocustom;
			if (currprefs.ntscmode != changed_prefs.ntscmode) {
				currprefs.ntscmode = changed_prefs.ntscmode;
				new_beamcon0 = currprefs.ntscmode ? 0x00 : BEAMCON0_PAL;
			}
			if ((changed_prefs.chipset_mask & CSMASK_ECS_AGNUS) && !(currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
				new_beamcon0 = beamcon0_saved;
			} else if (!(changed_prefs.chipset_mask & CSMASK_ECS_AGNUS) && (currprefs.chipset_mask & CSMASK_ECS_AGNUS)) {
				beamcon0_saved = beamcon0;
				beamcon0 = new_beamcon0 = currprefs.ntscmode ? 0x00 : BEAMCON0_PAL;
				diwhigh = 0;
				diwhigh_written = 0;
				bplcon0 &= ~(0x10 | 0x01);
			}
			if (currprefs.cs_hvcsync != changed_prefs.cs_hvcsync) {
				syncchange = true;
				nosignal_trigger = false;
				nosignal_status = 0;
			}
			currprefs.chipset_mask = changed_prefs.chipset_mask;
			currprefs.cs_agnusmodel = changed_prefs.cs_agnusmodel;
			currprefs.cs_denisemodel = changed_prefs.cs_denisemodel;
			currprefs.cs_hvcsync = changed_prefs.cs_hvcsync;
			init_custom();
	}

	if (currprefs.chipset_hr != changed_prefs.chipset_hr) {
		currprefs.chipset_hr = changed_prefs.chipset_hr;
		init_custom();
	}
	cia_set_eclockphase();
	if (syncchange) {
		varsync_changed = 2;
	}

#ifdef GFXFILTER
	for (int i = 0; i < 2; i++) {
		int idx = i == 0 ? 0 : 2;
		struct gfx_filterdata *fd = &currprefs.gf[idx];
		struct gfx_filterdata *fdcp = &changed_prefs.gf[idx];

		fd->gfx_filter_horiz_zoom = fdcp->gfx_filter_horiz_zoom;
		fd->gfx_filter_vert_zoom = fdcp->gfx_filter_vert_zoom;
		fd->gfx_filter_horiz_offset = fdcp->gfx_filter_horiz_offset;
		fd->gfx_filter_vert_offset = fdcp->gfx_filter_vert_offset;
		fd->gfx_filter_scanlines = fdcp->gfx_filter_scanlines;

		fd->gfx_filter_left_border = fdcp->gfx_filter_left_border;
		fd->gfx_filter_right_border = fdcp->gfx_filter_right_border;
		fd->gfx_filter_top_border = fdcp->gfx_filter_top_border;
		fd->gfx_filter_bottom_border = fdcp->gfx_filter_bottom_border;
	}
#endif
}

#ifdef CPUEMU_13

STATIC_INLINE void decide_fetch_ce(int hpos)
{
	if ((line_cyclebased || blt_info.blitter_dangerous_bpl) && vpos < current_maxvpos()) {
		decide_bpl_fetch(hpos);
	}
}

// blitter not in nasty mode = CPU gets one cycle if it has been waiting
// at least 4 cycles (all DMA cycles count, not just blitter cycles, even
// blitter idle cycles do count!)

extern int cpu_tracer;
static int dma_cycle(int *mode, int *ipl)
{
	int hpos_next, hpos_old;

	blt_info.nasty_cnt = 1;
	blt_info.wait_nasty = 0;
	if (cpu_tracer < 0) {
		return current_hpos_safe();
	}
	if (!currprefs.cpu_memory_cycle_exact) {
		return current_hpos_safe();
	}
	while (currprefs.cpu_memory_cycle_exact) {
		hpos_old = current_hpos_safe();
		hpos_next = hpos_old + 1;
		decide_line(hpos_next);
		sync_copper(hpos_next);
		decide_fetch_ce(hpos_next);
		int bpldma = bitplane_dma_access(hpos_old, 0);
		if (blt_info.blit_queued) {
			decide_blitter(hpos_next);
			// copper may have been waiting for the blitter
			sync_copper(hpos_next);
		}
		if ((cycle_line_slot[hpos_old] & CYCLE_MASK) == 0 && !bpldma) {
			alloc_cycle(hpos_old, CYCLE_CPU);
			break;
		}
		if (blt_info.nasty_cnt > 0) {
			blt_info.nasty_cnt++;
		}
		*ipl = regs.ipl_pin;
		do_cycles(1 * CYCLE_UNIT);
		/* bus was allocated to dma channel, wait for next cycle.. */
	}
	blt_info.nasty_cnt = 0;
	return hpos_old;
}

static void sync_cycles(void)
{
	if (extra_cycle) {
		do_cycles(extra_cycle);
		extra_cycle = 0;
	}
	evt_t c = get_cycles();
	int extra = c & (CYCLE_UNIT - 1);
	if (extra) {
		extra = CYCLE_UNIT - extra;
		do_cycles(extra);
	}
}

void do_copper(void)
{
	sync_cycles();
	int hpos = current_hpos();
	update_copper(hpos);
}

uae_u32 wait_cpu_cycle_read(uaecptr addr, int mode)
{
	uae_u32 v = 0;
	int hpos;
	int ipl = regs.ipl[0];
	evt_t now = get_cycles();

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	hpos = dma_cycle(&mode, &ipl);

#ifdef DEBUGGER
	if (debug_dma) {
		int reg = 0x1000;
		if (mode == -3) {
			reg |= 2;
			v = regs.chipset_latch_rw;
		} else if (mode < 0) {
			reg |= 4;
		} else if (mode > 0) {
			reg |= 2;
		} else {
			reg |= 1;
		}
		record_dma_read(reg, addr, hpos, vpos, DMARECORD_CPU, mode == -2 || mode == 2 ? 0 : 1);
	}
	peekdma_data.mask = 0;
#endif

	switch(mode)
	{
		case -1:
		v = get_long(addr);
		break;
		case -2:
		v = get_longi(addr);
		break;
		case 1:
		v = get_word(addr);
		break;
		case 2:
		v = get_wordi(addr);
		break;
		case 0:
		v = get_byte(addr);
		break;
	}

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read_value_pos(v, hpos, vpos);
	}
#endif

	regs.chipset_latch_rw = regs.chipset_latch_read = v;

	x_do_cycles_post(CYCLE_UNIT, v);

	// if IPL fetch was pending and CPU had wait states
	// Use ipl_pin value from previous cycle
	if (now == regs.ipl_evt) {
		regs.ipl[0] = ipl;
	}

	return v;
}

void wait_cpu_cycle_write(uaecptr addr, int mode, uae_u32 v)
{
	int hpos;
	int ipl = regs.ipl[0];
	evt_t now = get_cycles();

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	hpos = dma_cycle(&mode, &ipl);

#ifdef DEBUGGER
	if (debug_dma) {
		int reg = 0x1100;
		if (mode == -3) {
			reg |= 2;
		} else if (mode < 0) {
			reg |= 4;
		} else if (mode > 0) {
			reg |= 2;
		} else {
			reg |= 1;
		}
		record_dma_write(reg, v, addr, hpos, vpos, DMARECORD_CPU, 1);
	}
	peekdma_data.mask = 0;
#endif

	if (mode > -2) {
		if (mode < 0) {
			put_long(addr, v);
		} else if (mode > 0) {
			put_word(addr, v);
		} else if (mode == 0) {
			put_byte(addr, v);
		}
	}

	regs.chipset_latch_rw = regs.chipset_latch_write = v;

	x_do_cycles_post(CYCLE_UNIT, v);

	// if IPL fetch was pending and CPU had wait states:
	// Use ipl_pin value from previous cycle
	if (now == regs.ipl_evt) {
		regs.ipl[0] = ipl;
	}
}


uae_u32 wait_cpu_cycle_read_ce020(uaecptr addr, int mode)
{
	uae_u32 v = 0;
	int hpos, ipl;

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	hpos = dma_cycle(NULL, &ipl);

#ifdef DEBUGGER
	if (debug_dma) {
		int reg = 0x1000;
		if (mode < 0) {
			reg |= 4;
		} else if (mode > 0) {
			reg |= 2;
		} else {
			reg |= 1;
		}
		record_dma_read(reg, addr, hpos, vpos, DMARECORD_CPU, mode == -2 || mode == 2 ? 0 : 1);
	}
	peekdma_data.mask = 0;
#endif

	switch (mode) {
		case -1:
			v = get_long(addr);
			break;
		case -2:
			v = get_longi(addr);
			break;
		case 1:
			v = get_word(addr);
			break;
		case 2:
			v = get_wordi(addr);
			break;
		case 0:
			v = get_byte(addr);
			break;
	}

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read_value_pos(v, hpos, vpos);
	}
#endif

	regs.chipset_latch_rw = regs.chipset_latch_read = v;

	x_do_cycles_post(CYCLE_UNIT, v);

	return v;
}

void wait_cpu_cycle_write_ce020(uaecptr addr, int mode, uae_u32 v)
{
	int hpos, ipl;

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	hpos = dma_cycle(NULL, &ipl);

#ifdef DEBUGGER
	if (debug_dma) {
		int reg = 0x1100;
		if (mode < 0)
			reg |= 4;
		else if (mode > 0)
			reg |= 2;
		else
			reg |= 1;
		record_dma_write(reg, v, addr, hpos, vpos, DMARECORD_CPU, 1);
	}
	peekdma_data.mask = 0;
#endif

	if (mode < 0) {
		put_long(addr, v);
	} else if (mode > 0) {
		put_word(addr, v);
	} else if (mode == 0) {
		put_byte(addr, v);
	}

	regs.chipset_latch_rw = regs.chipset_latch_write = v;

	x_do_cycles_post(CYCLE_UNIT, v);
}

void do_cycles_ce(int cycles)
{
	cycles += extra_cycle;
	while (cycles >= CYCLE_UNIT) {
		int hpos = current_hpos() + 1;
		decide_line(hpos);
		sync_copper(hpos);
		decide_fetch_ce(hpos);
		if (blt_info.blit_queued) {
			decide_blitter(hpos);
		}
		do_cycles(1 * CYCLE_UNIT);
		cycles -= CYCLE_UNIT;
	}
	extra_cycle = cycles;
}

void do_cycles_ce020(int cycles)
{
	int c;
	evt_t cc;
	int extra;

	if (!cycles) {
		return;
	}
	cc = get_cycles();
	extra = cc & (CYCLE_UNIT - 1);
	if (extra) {
		extra = CYCLE_UNIT - extra;
		if (extra >= cycles) {
			do_cycles(cycles);
			return;
		}
		do_cycles(extra);
		cycles -= extra;
	}
	c = cycles;
	while (c) {
		int hpos = current_hpos() + 1;
		decide_line(hpos);
		sync_copper(hpos);
		decide_fetch_ce(hpos);
		if (blt_info.blit_queued) {
			decide_blitter(hpos);
		}
		if (c < CYCLE_UNIT) {
			break;
		}
		do_cycles(1 * CYCLE_UNIT);
		c -= CYCLE_UNIT;
	}
	if (c) {
		do_cycles(c);
	}
}

bool is_cycle_ce(uaecptr addr)
{
	addrbank *ab = get_mem_bank_real(addr);
	if (!ab || (ab->flags & ABFLAG_CHIPRAM) || ab == &custom_bank) {
		int hpos = current_hpos();
		return (cycle_line_slot[hpos] & CYCLE_MASK) != 0;
	}
	return 0;
}

#endif

bool isvga(void)
{
	if (programmedmode != 1) {
		return false;
	}
	if (hblank_hz >= 20000) {
		return true;
	}
	return false;
}

bool ispal(int *lines)
{
	if (lines) {
		*lines = maxvpos_display;
	}
	if (programmedmode == 1) {
		return currprefs.ntscmode == 0;
	}
	return maxvpos_display >= MAXVPOS_NTSC + (MAXVPOS_PAL - MAXVPOS_NTSC) / 2;
}

static void SET_LINE_CYCLEBASED(int hpos)
{
	line_cyclebased = 1;
	decide_line(hpos);
	decide_fetch_safe(hpos);
}
