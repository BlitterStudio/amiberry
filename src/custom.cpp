/*
* UAE - The Un*x Amiga Emulator
*
* Custom chip emulation
*
* Copyright 1995-2002 Bernd Schmidt
* Copyright 1995 Alessandro Bissacco
* Copyright 2000-2025 Toni Wilen
*
* Complete rewrite 2024-2025
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include <algorithm>
#include <cctype>
#include <cassert>
#include <cmath>

#include "options.h"
#include "uae.h"
#include "audio.h"
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
#include "devices.h"
#include "rommgr.h"
#ifdef WITH_SPECIALMONITORS
#include "specialmonitors.h"
#endif

#define VPOSW_DISABLED 0
#define VPOSW_DEBUG 0

#define FRAMEWAIT_MIN_MS 2
#define FRAMEWAIT_SPLIT 4

#define CYCLE_CONFLICT_LOGGING 0

#define CUSTOM_DEBUG 0
#define SPRITE_DEBUG 0

#define REFRESH_FIRST_HPOS 3
#define COPPER_CYCLE_POLARITY 1
#define HARDWIRED_DMA_TRIGGER_HPOS 1

#define REF_RAS_ADD_AGA 0x000
#define REF_RAS_ADD_ECS 0x200
#define REF_RAS_ADD_OCS 0x002

#define SPRBORDER 0

#define EXTRAWIDTH_BROADCAST 7
#define EXTRAHEIGHT_BROADCAST_TOP 0
#define EXTRAHEIGHT_BROADCAST_BOTTOM 0
#define EXTRAWIDTH_EXTREME 38
#define EXTRAHEIGHT_EXTREME 28
#define EXTRAWIDTH_ULTRA 77

#define LORES_TO_SHRES_SHIFT 2

#ifdef SERIAL_PORT
extern uae_u16 serper;
#endif

#define MAX_SCANDOUBLED_LINES 1200

struct pipeline_func
{
	evfunc2 func;
	uae_u16 v;
	uae_u16 cck;
};
#define MAX_PIPELINE_REG 3
struct pipeline_reg
{
	uae_u16 *p;
	uae_u16 v;
};
static uae_u32 displayresetcnt;
static int displayreset_delayed;
uae_u8 agnus_hpos;
int agnus_hpos_prev, agnus_hpos_next, agnus_vpos_next;
static int agnus_pos_change;
static uae_u32 dmal_shifter;
static uae_u16 pipelined_write_addr;
static uae_u16 pipelined_write_value;
static struct rgabuf rga_pipe[RGA_SLOT_TOTAL + 1];
struct denise_rga rga_denise[DENISE_RGA_SLOT_TOTAL];
static struct linestate *current_line_state;
static struct linestate lines[MAX_SCANDOUBLED_LINES + 1][2];
static int rga_denise_cycle, rga_denise_cycle_start, rga_denise_cycle_count_start, rga_denise_cycle_count_end;
static int draw_line_next_line, draw_line_wclks;
static uae_u32 rga_denise_cycle_line = 1;
static struct pipeline_reg preg;
static struct pipeline_func pfunc[MAX_PIPELINE_REG];
static uae_u16 prev_strobe;
static bool vb_fast;
static uae_u32 custom_state_flags;
static int not_safe_mode;
static bool dmal_next;
static int fast_lines_cnt;
static bool lineoptimizations_draw_always;

static uae_u32 scandoubled_bpl_ptr[MAX_SCANDOUBLED_LINES + 1][2][MAX_PLANES];
static bool scandoubled_bpl_ena[MAX_SCANDOUBLED_LINES + 1];

static evt_t blitter_dma_change_cycle, copper_dma_change_cycle, sprite_dma_change_cycle_on, sprite_dma_change_cycle_off;

static void empty_pipeline(void)
{
	if (preg.p) {
		*preg.p = preg.v;
		preg.p = NULL;
	}
}
static void push_pipeline(uae_u16 *p, uae_u16 v)
{
	if (preg.p) {
		// cpu or fast copper can cause this
		empty_pipeline();
	}
	preg.p = p;
	preg.v = v;
}
static void pipelined_custom_write(evfunc2 func, uae_u16 v, uae_u16 cck)
{
	if (!cck || isrestore()) {
		func(v);
		return;
	}
	for (int i = 0 ; i < MAX_PIPELINE_REG; i++) {
		struct pipeline_func *p = &pfunc[i];
		if (!p->func) {
			p->func = func;
			p->v = v;
			p->cck = cck;
			return;
		}
	}
	write_log("pipelined_custom_write overflow!\n");
}
static void handle_pipelined_custom_write(bool now)
{
	for (int i = 0 ; i < MAX_PIPELINE_REG; i++) {
		struct pipeline_func *p = &pfunc[i];
		if (p->func) {
			p->cck--;
			if (!p->cck || now) {
				auto f = p->func;
				p->func = NULL;
				f(p->v);
			}
		}
	}
}

static uae_u32 rga_slot_in_offset, rga_slot_first_offset, rga_slot_out_offset;
static evt_t last_rga_cycle;

static void write_drga_strobe(uae_u16 rga)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	r->rga = rga;
	r->flags = 0;
	r->line = rga_denise_cycle_line;
};
static void write_drga(uae_u16 rga, uaecptr pt, uae_u32 v)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	r->rga = rga;
	r->v = v;
	r->pt = pt;
	r->flags = 0;
	r->line = rga_denise_cycle_line;
};
static void write_drga_dat_spr(uae_u16 rga, uaecptr pt, uae_u32 v)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	r->rga = rga;
	r->v = v;
	r->pt = pt;
	r->flags = 0;
	r->line = rga_denise_cycle_line;
};
static void write_drga_dat_spr_wide(uae_u16 rga, uaecptr pt, uae_u64 v)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	r->rga = rga;
	r->v64 = v;
	r->pt = pt;
	r->flags = 0;
	r->line = rga_denise_cycle_line;
};

static void write_drga_dat_bpl16(uae_u16 rga, uaecptr pt, uae_u16 v, int plane)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	r->rga = rga;
	r->v = v;
	r->pt = pt;
	r->flags = 0;
	r->line = rga_denise_cycle_line;
};
static void write_drga_dat_bpl32(uae_u16 rga, uaecptr pt, uae_u32 v, int plane)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	r->rga = rga;
	r->v = v;
	r->pt = pt;
	r->flags = 0;
	r->line = rga_denise_cycle_line;
};
static void write_drga_dat_bpl64(uae_u16 rga, uaecptr pt, uae_u64 v, int plane)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	r->rga = rga;
	r->v64 = v;
	r->pt = pt;
	r->flags = 0;
	r->line = rga_denise_cycle_line;
};

static void write_drga_flag(uae_u32 flags, uae_u32 mask)
{
	struct denise_rga *r = &rga_denise[rga_denise_cycle];
	if (r->line != rga_denise_cycle_line) {
		r->line = rga_denise_cycle_line;
		r->rga = 0x1fe;
		r->flags = 0;
	}
	r->flags &= ~mask;
	r->flags |= flags;
}

static uae_u32 dummyrgaaddr;
struct rgabuf *write_rga(int slot, int type, uae_u16 v, uae_u32 *p)
{
	struct rgabuf *r = &rga_pipe[(slot + rga_slot_first_offset) & 3];

	bool strobe = (v >= 0x38 && v < 0x40) || v == 0x1fe;

	if (r->reg != 0x1fe && !strobe) {
		write_log("RGA conflict: %04x -> %04x, %08x | %08x -> %08x, %04x, %d\n",
			r->reg, v,
			p ? *p : 0, r->pv, (p ? *p : 0) | r->pv, 
			v,
			slot);
	}
	// RGA bus address conflict causes AND operation
	r->reg &= v;
	r->type |= type;
	r->alloc = 1;
	if (p && r->p) {
		// DMA address pointer conflict causes both old and new address to becomes old OR new.
		r->conflict = r->p;
		*r->p |= *p;
		*p = *r->p;
		r->pv |= *p;
	} else if (p) {
		r->p = p;
		r->pv = *p;
	}
	return r;
}

STATIC_INLINE rgabuf *read_rga_out(void)
{
	struct rgabuf *r = &rga_pipe[rga_slot_out_offset];
	return r;
}
STATIC_INLINE rgabuf *read_rga_in(void)
{
	struct rgabuf *r = &rga_pipe[rga_slot_in_offset];
	return r;
}

struct rgabuf *read_rga(int slot)
{
	struct rgabuf *r = &rga_pipe[(slot + rga_slot_first_offset) & 3];
	return r;
}
bool check_rga_free_slot_in(void)
{
	struct rgabuf *r = &rga_pipe[rga_slot_in_offset];
	return r->alloc <= 0;
}
STATIC_INLINE bool check_rga_out(void)
{
	struct rgabuf *r = &rga_pipe[rga_slot_out_offset];
	return r->alloc;
}

bool check_rga(int slot)
{
	struct rgabuf *r = &rga_pipe[(slot + rga_slot_first_offset) & 3];
	return r->alloc;
}
static void clear_rga(struct rgabuf *r)
{
	r->reg = 0x1fe;
	r->p = NULL;
	r->pv = 0;
	r->type = 0;
	r->alloc = 0;
	r->write = false;
	r->conflict = NULL;
}
static void shift_rga(void)
{
	rga_slot_first_offset--;
	rga_slot_in_offset--;
	rga_slot_out_offset--;
	rga_slot_first_offset &= 3;
	rga_slot_in_offset &= 3;
	rga_slot_out_offset &= 3;

	struct rgabuf *r = &rga_pipe[rga_slot_first_offset];
	clear_rga(r);
}


static void uae_abort (const TCHAR *format,...)
{
	static int nomore;
	va_list parms;
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

int vpos, vpos_prev;
bool lof_store; // real bit in custom registers
bool lof_display; // what display device thinks
static bool lof_detect, lof_pdetect;
static bool lof_detect_vsync;
static bool lol;
static bool linetoggle;
static int next_lineno;
int linear_vpos, linear_hpos, linear_vpos_prev[3], linear_hpos_prev[3];
static int linear_vpos_vsync;
static int linear_display_vpos;
int current_linear_vpos, current_linear_hpos;
int current_linear_vpos_nom, current_linear_hpos_short;
static int linear_vpos_visible, current_linear_vpos_visible;
static int current_linear_vpos_temp, current_linear_hpos_temp;
static int current_linear_temp_change;
static bool display_redraw;
static int display_hstart_cyclewait, display_hstart_cyclewait_cnt, display_hstart_cyclewait_end;
static int display_hstart_cyclewait_skip, display_hstart_cyclewait_skip2;
static bool display_hstart_cyclewait_start;
static int agnus_trigger_cck;
static int linear_vpos_changes;
static enum nln_how nextline_how;
static bool prevlofs[3];
static bool vsync_rendered, frame_rendered, frame_shown;
static frame_time_t vsynctimeperline;
static frame_time_t frameskiptime;
static bool genlockhtoggle;
static bool genlockvtoggle;
static bool graphicsbuffer_retry;
static int cia_hsync;
static int nosignal_cnt, nosignal_status;
static bool nosignal_trigger;
static bool syncs_stopped;
int display_reset;
static bool initial_frame;
static int custom_fastmode_exit;
static evt_t last_vsync_evt, last_hsync_evt;
static bool aexthblanken;
#if 0
static int custom_fastmode_bplextendmask;
#endif
static int plffirstline, plflastline;

/* Stupid genlock-detection prevention hack.
* We should stop calling vsync_handler() and
* hstop_handler() completely but it is not
* worth the trouble..
*/
static int vpos_lpen, hpos_lpen, hhpos_lpen, lightpen_triggered;
int lightpen_x[2], lightpen_y[2];
int lightpen_cx[2], lightpen_cy[2], lightpen_active, lightpen_enabled, lightpen_enabled2;

/*
* Hardware registers of all sorts.
*/

static uae_u16 cregs[256];

uae_u16 intena, intreq;
static uae_u16 intena2, intreq2;
uae_u16 dmacon;
static uae_u16 dmacon_next;
uae_u16 adkcon; /* used by audio code */
uae_u16 last_custom_value;
static bool dmacon_bpl;

static uae_u32 cop1lc, cop2lc, copcon;


/*
* Horizontal hardwired defaults
* 
* 0x00   0 HC0    (genlock handling)
* 0x01   1 HC1    (START), (VSY serration pulse start)
* 0x09   9 VR1    (LOF=0 -> VE start, LOF=1 -> VE stop)
* 0x12  18 SHS    ([HSSTRT] horizontal sync start), (VSY serration pulse end)
* 0x1a  26 VER1_P (HSY end of equalization pulse in PAL) 
* 0x1b  27 VER1_N (HSY end of equalization pulse in NTSC)
* 0x23  35 RHS    ([HSSTOP] horizontal sync end)
* 0x73 115 VR2    (LOF=1 -> VE start, LOF=0 -> VE stop),  (VSY serration pulse start)
* 0x84 132 CEN    ([HCENTER]), (VSY serration pulse end)
* 0x8c 140 VER2_P (CEN end of equalization pulse in PAL)
* 0x8d 141 VER2_N (CEN end of equalization pulse in NTSC)
* 0xe2 226 HC226  (LOL=0, [HTOTAL] PAL line, NTSC short line)
* 0xe3 227 HC227  (LOL=1, NTSC long line)
* 
* Vertical hardwired defaults
* 
* 0    SVB   (start for Vertical Equalization zone) 
* 2    VC2   (PAL/CEN/LOF=0 -> enable Vertical Sync zone)
* 3    VC3   (NTSC + PAL/SHS/LOF=1 enable Vertical Sync zone)
* 5    VC5   (PAL/SHS/LOF=0 + PAL/CEN/LOF=1 -> disable Vertical Sync zone)
* 6    VC6   (NTSC disable Vertical Sync zone)
* 7    VC7   (PAL LOF=0 stop for Vertical Equalization zone)
* 8    VC8   (PAL LOF=1 stop for Vertical Equalization zone)
* 9    VC9   (NTSC stop for Vertical Equalization zone)
* 20   RVB_N ([VBSTOP] NTSC)
* 25   RVB_P ([VBSTOP] PAL)
* 261  VC261 ([VBSTRT][VTOTAL] LOF=0, NTSC short frame)
* 262  VC262 ([VBSTRT] LOF=1, NTSC long frame)
* 311  VC311 ([VBSTRT][VTOTAL] LOF=0, PAL short frame)
* 312  VC312 ([VBSTRT] LOF=1, PAL long frame)
*/


/*

* Odd field:
*
* PAL
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

static bool agnus_phsync, agnus_phblank;
static uae_u32 agnus_phblank_start, agnus_phblank_end, agnus_phsync_start, agnus_phsync_end, agnus_hsync_start, agnus_hsync_end;
static bool agnus_pvsync, agnus_pcsync, agnus_csync;
static int agnus_vb, agnus_pvb;
static bool agnus_vb_active;
static bool agnus_vb_start_line, agnus_pvb_start_line, agnus_vb_active_start_line;
static bool agnus_vb_end_line, agnus_pvb_end_line, agnus_vb_active_end_line;
static bool agnus_equzone;
static bool agnus_hsync, agnus_vsync, agnus_ve, agnus_p_ve;
static bool agnus_bsvb, agnus_bsvb_prev;
static bool agnus_equdis;


int maxhpos = MAXHPOS_PAL;
static int maxhpos_long;
int maxhpos_short = MAXHPOS_PAL;
static bool maxhpos_lol;
int maxvpos = MAXVPOS_PAL;
int maxvpos_nom = MAXVPOS_PAL; // nominal value (same as maxvpos but "faked" maxvpos in fake 60hz modes)
static int maxvpos_long;
int maxvpos_display = MAXVPOS_PAL; // value used for display size
int maxhpos_display = AMIGA_WIDTH_MAX;
int maxvsize_display = AMIGA_HEIGHT_MAX;
int maxvpos_display_vsync;
int vsync_startline;
static bool maxvpos_display_vsync_next;
static int maxhposm1;
int maxhposm0 = MAXHPOS_PAL;
static bool maxhposeven;
static int hsyncendpos, hsyncstartpos;
int hsync_end_left_border, hdisplay_left_border;
static int hsyncstartpos_start, hsyncstartpos_start_cycles;

static int hsyncstartpos_start_hw;
int hsyncstartpos_hw;
int hsyncendpos_hw;

int denisehtotal;
static int maxvpos_total = 511;
int minfirstline = VBLANK_ENDLINE_PAL;
int minfirstline_linear = VBLANK_ENDLINE_PAL;
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
static bool beamcon0_dual, beamcon0_pal;
uae_u16 bemcon0_hsync_mask, bemcon0_vsync_mask;
bool beamcon0_has_hsync, beamcon0_has_vsync, beamcon0_has_csync;
static uae_u16 beamcon0_saved;
static uae_u16 bplcon0_saved, bplcon1_saved, bplcon2_saved;
static uae_u16 bplcon3_saved, bplcon4_saved;
static uae_u16 ddfstrt_saved, ddfstop_saved, diwhigh_saved;
static uae_u32 saved_color_regs_aga[32];
static struct color_entry agnus_colors;
static int varsync_changed, varsync_maybe_changed[2];
static int lines_after_beamcon_change;
static bool programmed_register_accessed_v, programmed_register_accessed_h;
static bool need_vdiw_check;
static int varhblank_lines, varhblank_val[2];
static int exthblank_lines[2];
static uae_u16 vt_old, ht_old, hs_old, vs_old;
uae_u16 vtotal, htotal;
static int maxvpos_stored, maxhpos_stored;
uae_u16 hsstop, hsstrt;
uae_u16 hbstop, hbstrt;
static int hbstop_cck, hbstrt_cck;
static int hsstop_detect, hsstop_detect2;
static uae_u16 vsstop, vsstrt;
static uae_u16 vbstop, vbstrt;
static uae_u16 hcenter;
static uae_u16 hsstrt_v2, hsstop_v2;
static bool ocs_blanked;
static uae_u16 sprhstrt, sprhstop, bplhstrt, bplhstop, hhpos;
static bool uhres_spr, uhres_bpl;
static int uhres_state;
static uae_u16 sprhstrt_v, sprhstop_v, bplhstrt_v, bplhstop_v;
static uaecptr hhbpl, hhspr;
static uae_s16 bplhmod;
static int ciavsyncmode;
static int diw_hstrt, diw_hstop;
static uae_u32 ref_ras_add;
static uaecptr refptr, refptr_p;
static uae_u32 refmask;
static int line_disabled;
static bool custom_disabled;
static int display_hstart_fastmode;
static int color_table_index;
static bool color_table_changed;
#define COLOR_TABLE_ENTRIES 2
static uae_u8 color_tables[COLOR_TABLE_ENTRIES * 512 * sizeof(uae_u32)];

#define HSYNCTIME (maxhpos * CYCLE_UNIT)

struct sprite {
	uaecptr pt;
	int vstart;
	int vstop;
	bool dblscan; /* AGA SSCAN2 */
	int dmastate;
	int dmacycle;
	uae_u16 ctl, pos;
};

static struct sprite spr[MAX_SPRITES];
static int plfstrt_sprite;
uaecptr sprite_0;
int sprite_0_width, sprite_0_height, sprite_0_doubled;
uae_u32 sprite_0_colors[4];
static uae_u8 magic_sprite_mask = 0xff;

static int sprite_width;
static int sprite_sprctlmask;
int sprite_buffer_res;

static uae_s16 bpl1mod, bpl2mod;
static uaecptr bplpt[MAX_PLANES];


uae_u16 bplcon0;
static uae_u16 bplcon1, bplcon2, bplcon3, bplcon4;
static uae_u32 bplcon0_res, bplcon0_planes, bplcon0_planes_limit;
static int diwstrt, diwstop, diwhigh;
static int diwhigh_written;
static uae_u16 ddfstrt, ddfstop, ddf_mask;
static int diw_change;

/* The display and data fetch windows */

enum class diw_states
{
	DIW_waiting_start, DIW_waiting_stop
};

int plffirstline_total, plflastline_total;
static int autoscale_bordercolors;
static int first_bpl_vpos;
static int last_decide_line_hpos;
static int last_fetch_hpos, last_decide_sprite_hpos;
static int diwfirstword, diwlastword;
static int last_diwlastword;
static int hb_last_diwlastword;
static int last_hdiw;
static diw_states vdiwstate, hdiwstate;
static int hdiwbplstart;
static bool exthblank;

int first_planes_vpos, last_planes_vpos;
static int first_bplcon0, first_bplcon0_old;
static int first_planes_vpos_old, last_planes_vpos_old;
int diwfirstword_total, diwlastword_total;
int ddffirstword_total, ddflastword_total;
static int diwfirstword_total_old, diwlastword_total_old;
static int ddffirstword_total_old, ddflastword_total_old;
bool vertical_changed, horizontal_changed;
int firstword_bplcon1;

static int copper_access;

uae_u16 clxdat;
static uae_u16 clxcon, clxcon2;

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
	COP_strobe_vbl_delay,
	COP_strobe_vbl_delay2,
	COP_strobe_vbl_delay_nodma,
	COP_strobe_vbl_extra_delay1,
	COP_strobe_vbl_extra_delay2,
	COP_strobe_vbl_extra_delay3,
	COP_strobe_delay_start,
	COP_strobe_delay_start_odd,
	COP_strobe_delay1,
	COP_strobe_delay1_odd,
	COP_strobe_delay2,
	COP_strobe_extra // just to skip current cycle when CPU wrote to COPJMP
};

struct copper {
	/* The current instruction words.  */
	uae_u16 ir[2];
	enum copper_states state, state_prev;
	/* Instruction pointer.  */
	uaecptr ip;
	uaecptr strobeip;
	// following move does not enable COPRS
	int ignore_next;
	int vcmp, hcmp;

	int strobe; /* COPJMP1 / COPJMP2 accessed */
	int strobetype;
	enum copper_states strobe_next;
	int moveaddr, movedata, movedelay;
	uaecptr moveptr;
	uaecptr vblankip;
	evt_t strobe_cycles;
};

static struct copper cop_state;
static int copper_enabled_thisline;

/*
* Statistics
*/
uae_u32 timeframes;
evt_t frametime;
frame_time_t lastframetime;
uae_u32 hsync_counter, vsync_counter;
frame_time_t idletime;
int bogusframe;
static int display_vsync_counter, display_hsync_counter;
static evt_t display_last_hsync, display_last_vsync;

static bool ddf_limit, ddfstrt_match, hwi_old;
static int ddf_stopping, ddf_enable_on;
static int bprun;
static evt_t bprun_end;
static int bprun_cycle;
static bool harddis_v, harddis_h;

struct custom_store custom_storage[256];

static uae_u16 dmal;

static int REGPARAM3 custom_wput_1(uaecptr, uae_u32, int) REGPARAM;

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
	if (ad->picasso_on) {
		custom_disabled = true;
		line_disabled |= 2;
	} else {
		custom_disabled = false;
		line_disabled &= ~2;
	}
}

STATIC_INLINE bool nodraw(void)
{
	struct amigadisplay *ad = &adisplays[0];
	bool nd = !currprefs.cpu_memory_cycle_exact && ad->framecnt != 0;
	return nd;
}

static bool doflickerfix_possible(void)
{
	return currprefs.gfx_vresolution && doublescan < 0 && vpos < MAXVPOS;
}
static bool doflickerfix_active(void)
{
	return interlace_seen > 0 && doflickerfix_possible();
}

static bool flickerfix_line_no_draw(int dvp)
{
	return doflickerfix_active() && dvp >= (maxvpos_nom - vsync_startline - 1) * 2;
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

// is last display line?
static bool is_last_line(void)
{
	return  vpos == vsync_startline || (vpos + 1 == vsync_startline && vpos_prev + 1 != vsync_startline - 1);
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

// 141 is largest hardwired hpos comparison position.
#define HW_HPOS_TABLE_MAX 142
static bool hw_hpos_table[HW_HPOS_TABLE_MAX];
static uae_u8 prg_hpos_table[256];

static void updateprghpostable(void)
{
	memset(prg_hpos_table, 0, sizeof prg_hpos_table);
	prg_hpos_table[hsstrt & 0xff] |= 1;
	prg_hpos_table[hsstop & 0xff] |= 2;
	prg_hpos_table[hbstrt & 0xff] |= 4;
	prg_hpos_table[hbstop & 0xff] |= 8;
	prg_hpos_table[hcenter & 0xff] |= 16;
}
static void updatehwhpostable(void)
{
	int isntsc = (beamcon0 & BEAMCON0_PAL) ? 0 : 1;
	hw_hpos_table[26] = false;
	hw_hpos_table[27] = false;
	hw_hpos_table[140] = false;
	hw_hpos_table[141] = false;
	if (isntsc) {
		hw_hpos_table[27] = true;
		hw_hpos_table[141] = true;
	} else {
		hw_hpos_table[26] = true;
		hw_hpos_table[140] = true;
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

	// hardwired horizontal positions
	hw_hpos_table[1] = true;
	hw_hpos_table[2] = true;
	hw_hpos_table[9] = true;
	hw_hpos_table[18] = true;
	hw_hpos_table[35] = true;
	hw_hpos_table[115] = true;
	hw_hpos_table[132] = true;
	hw_hpos_table[26] = true;
	hw_hpos_table[27] = true;
	hw_hpos_table[140] = true;
	hw_hpos_table[141] = true;
}

/* fetchstart_mask can be larger than fm_maxplane if FMODE > 0.
   This means that the remaining cycles are idle.
 */
static const uae_u8 bpl_sequence_8[32] = { 8, 4, 6, 2, 7, 3, 5, 1 };
static const uae_u8 bpl_sequence_4[32] = { 4, 2, 3, 1 };
static const uae_u8 bpl_sequence_2[32] = { 2, 1 };
static const uae_u8 *bpl_sequence;

/* set currently active Agnus bitplane DMA sequence */
static void setup_fmodes(uae_u16 con0)
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
	bplcon0_res = GET_RES_AGNUS(con0);
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
	fetchmode_size = 16 << fetchmode;
	fetchmode_bytes = 2 << fetchmode;
	fetchmode_mask = fetchmode_size - 1;
	fetchmode_fmode_bpl = fmode & 3;
	fetchmode_fmode_spr = (fmode >> 2) & 3;
	curr_diagram = cycle_diagram_table[fetchmode][bplcon0_res][bplcon0_planes_limit];
}


static void set_chipset_mode(bool imm)
{
	fmode = fmode_saved;
	bplcon0 = bplcon0_saved;
	bplcon1 = bplcon1_saved;
	bplcon2 = bplcon2_saved;
	bplcon3 = bplcon3_saved;
	bplcon4 = bplcon4_saved;
	ddfstrt = ddfstrt_saved & ddf_mask;
	ddfstop = ddfstop_saved & ddf_mask;
	diwhigh = diwhigh_saved;
	diwhigh_written = (diwhigh & 0x8000) != 0;
	diwhigh &= ~(0x8000 | 0x4000 | 0x0080 | 0x0040);
	if (!aga_mode) {
		fmode = 0;
		bplcon0 &= ~(0x0010 | 0x0020);
		bplcon1 &= ~(0xff00);
		bplcon2 &= ~(0x0100 | 0x0080);
		bplcon3 &= 0x003f;
		bplcon3 |= 0x0c00;
		bplcon4 = 0x0011;
		diwhigh &= ~(0x0010 | 0x1000);
		if (!ecs_agnus) {
			bplcon0 &= ~0x0080;
			diwhigh = 0;
			diwhigh_written = 0;
		}
		if (!ecs_denise) {
			bplcon0 &= ~0x0001;
			bplcon2 &= 0x007f;
			bplcon3 = 0x0c00;
		}
	}
	sprite_width = GET_SPRITEWIDTH(fmode);
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
	if (imm || bplcon0 != bplcon0_saved) {
		denise_update_reg_queue(0x100, bplcon0, rga_denise_cycle_line);
	}
	if (imm || bplcon1 != bplcon1_saved) {
		denise_update_reg_queue(0x102, bplcon1, rga_denise_cycle_line);
	}
	if (imm || bplcon2 != bplcon2_saved) {
		denise_update_reg_queue(0x104, bplcon2, rga_denise_cycle_line);
	}
	if (imm || bplcon3 != bplcon3_saved) {
		denise_update_reg_queue(0x106, bplcon3, rga_denise_cycle_line);
	}
	if (imm || bplcon4 != bplcon4_saved) {
		denise_update_reg_queue(0x10c, bplcon4, rga_denise_cycle_line);
	}
	if (imm) {
		denise_update_reg_queue(0x8e, diwstrt, rga_denise_cycle_line);
		if (diwhigh_written) {
			denise_update_reg_queue(0x1e4, diwhigh, rga_denise_cycle_line);
		}
	}
	if (imm || fmode != fmode_saved) {
		denise_update_reg_queue(0x1fc, fmode, rga_denise_cycle_line);
		setup_fmodes(bplcon0);
	}
}

static void update_mirrors(void)
{
	bool aga_mode_new = (currprefs.chipset_mask & CSMASK_AGA) != 0;
	if (aga_mode_new && !aga_mode) {
		memcpy(denise_colors.color_regs_aga, saved_color_regs_aga, sizeof(saved_color_regs_aga));
		memcpy(agnus_colors.color_regs_aga, saved_color_regs_aga, sizeof(saved_color_regs_aga));
	} else if (!aga_mode_new && aga_mode) {
		memcpy(saved_color_regs_aga, agnus_colors.color_regs_aga, sizeof(saved_color_regs_aga));
	}
	aga_mode = aga_mode_new;
	ecs_agnus = (currprefs.chipset_mask & CSMASK_ECS_AGNUS) != 0;
	ecs_denise = (currprefs.chipset_mask & CSMASK_ECS_DENISE) != 0;
	ecs_denise_only = ecs_denise && !aga_mode;
	agnusa1000 = (currprefs.chipset_mask & CSMASK_A1000) != 0 || currprefs.cs_agnusmodel == AGNUSMODEL_A1000 || currprefs.cs_agnusmodel == AGNUSMODEL_VELVET;
	if (agnusa1000) {
		ecs_agnus = false;
		aga_mode = false;
	}
	denisea1000_noehb = (currprefs.chipset_mask & CSMASK_A1000_NOEHB) != 0 || currprefs.cs_denisemodel == DENISEMODEL_VELVET || currprefs.cs_denisemodel == DENISEMODEL_A1000NOEHB;
	denisea1000 = (currprefs.chipset_mask & CSMASK_A1000) != 0 || currprefs.cs_denisemodel == DENISEMODEL_VELVET || currprefs.cs_denisemodel == DENISEMODEL_A1000NOEHB || currprefs.cs_denisemodel == DENISEMODEL_A1000;
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
			agnus_colors.acolors[i] = denise_colors.acolors[i] = getxcolor(denise_colors.color_regs_aga[i]);
		}
	}
	ddf_mask = ecs_agnus ? 0xfe : 0xfc;
	set_chipset_mode(true);
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	lineoptimizations_draw_always = drawing_can_lineoptimizations() == false;
	color_table_changed = true;
}

void notice_new_xcolors(void)
{
	update_mirrors();
	docols(&agnus_colors);
}

static uae_u16 get_strobe_reg(int slot)
{
	uae_u16 strobe = 0x1fe;
	if (slot == 0) {
		bool equ = ecs_agnus ? agnus_p_ve : agnus_ve;
		bool vb = (beamcon0 & BEAMCON0_VARVBEN) ? (agnus_pvb || agnus_pvb_end_line) && !agnus_pvb_start_line : (agnus_vb == 1 || agnus_vb_end_line);
/*
		A1000 Agnus:
		Line 0: STRHOR
		Line 1: STREQU

		OCS Agnus:
		Line 0: STRVBL
		Line 1: STREQU

		ECS Agnus:
		Line 0: STREQU
		Line 1: STREQU
*/
		strobe = 0x03c;
		if (vb) {
			strobe = 0x03a;
		}
		if (equ) {
			strobe = 0x038;
		}
	} else if (slot == 1 && lol) {
		strobe = 0x03e;
	}
	return strobe;
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

frame_time_t vsynctimebase_orig;

void compute_vsynctime(void)
{
	float svpos = current_linear_vpos_nom + 0.0f;
	float shpos = current_linear_hpos_short + 0.0f;
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
	if (!fake_vblank_hz) {
		fake_vblank_hz = vblank_hz;
	}

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
	cputimebase = syncbase / ((uae_u32)(svpos * shpos));

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
		minfirstline, maxvpos_display + vsync_startline,
		minfirstline, maxvpos_display + vsync_startline);
	write_log(_T("PC=%08x COP=%08x\n"), M68K_GETPC, cop_state.ip);
}

int current_maxvpos(void)
{
	return maxvpos + (lof_store ? 1 : 0);
}

static void updateextblk(void)
{
	hsyncstartpos_start_hw = 13;
	hsyncstartpos_hw = maxhpos_short + hsyncstartpos_start_hw;
	hsyncendpos_hw = 24;

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
		if (hsyncstartpos_start < REFRESH_FIRST_HPOS + 1) {
			hsyncstartpos_start = REFRESH_FIRST_HPOS + 1;
		}

	} else {

		hsyncstartpos_start = hsyncstartpos_start_hw;
		hsyncstartpos = hsyncstartpos_hw;
		denisehtotal = maxhpos_short + 7;
		hsstop_detect2 = (35 + 9) * 2;

		hsyncendpos = hsyncendpos_hw;

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

void resetfulllinestate(void)
{
	displayreset_delayed |= 4 | 2 | 1;
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

	set_drawbuffer();
	struct vidbuffer *vb = vidinfo->inbuffer;

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
	vb->inxoffset = -1;
	vb->inyoffset = -1;
	updateextblk();

	hsync_end_left_border = display_hstart_cyclewait_end + display_hstart_cyclewait;

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

	if (vres2 < 0) {
		vres2 = 0;
	}
	if (vres2 > VRES_QUAD) {
		vres2 = VRES_QUAD;
	}

	if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
		vb->inwidth = current_linear_hpos_short << (res2 + 1);
	} else {
		vb->inwidth = (current_linear_hpos_short - (display_hstart_cyclewait_skip + display_hstart_cyclewait_skip2)) << (res2 + 1);
	}
	vb->inwidth2 = vb->inwidth;
	vb->extrawidth = -2;
	if (currprefs.gfx_extrawidth > 0) {
		vb->extrawidth = currprefs.gfx_extrawidth << res2;
	}
	vb->extraheight = -2;
	if (currprefs.gfx_extraheight > 0) {
		vb->extraheight = currprefs.gfx_extraheight << vres2;
	}
	if (currprefs.gfx_overscanmode >= OVERSCANMODE_EXTREME) {
		vb->extrawidth = -1;
	}
	int mfl = minfirstline_linear;
	if (currprefs.gfx_overscanmode < OVERSCANMODE_ULTRA) {
		mfl += 2;
	}
	int maxv = current_linear_vpos - mfl;
	vb->inheight = maxv << vres2;
	vb->inheight2 = vb->inheight;
	vb->inxoffset = 0;

	//write_log(_T("Width %d Height %d\n"), vb->inwidth, vb->inheight);

	if (vb->inwidth < 16)
		vb->inwidth = 16;
	if (vb->inwidth2 < 16)
		vb->inwidth2 = 16;
	if (vb->inheight < 1)
		vb->inheight = 1;
	if (vb->inheight2 < 1)
		vb->inheight2 = 1;

	if (!vb->hardwiredpositioning) {
		vb->outwidth = vb->inwidth;
		vb->outheight = vb->inheight;
	}

	check_nocustom();

	compute_vsynctime();

	hblank_hz = (currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL) / (maxhpos + (linetoggle ? 0.5f : 0.0f));

	write_log(_T("%s mode%s%s V=%.4fHz H=%0.4fHz (%dx%d+%d) IDX=%d (%s) D=%d RTG=%d/%d\n"),
		isntsc ? _T("NTSC") : _T("PAL"),
		islace ? _T(" lace") : _T(""),
		doublescan > 0 ? _T(" dblscan") : _T(""),
		vblank_hz,
		hblank_hz,
		maxhpos, maxvpos, lof_store ? 1 : 0,
		cr ? cr->index : -1,
		cr != NULL && cr->label != NULL ? cr->label : _T("<?>"),
		currprefs.gfx_apmode[ad->picasso_on ? 1 : 0].gfx_display, ad->picasso_on, ad->picasso_requested_on
	);

#ifdef PICASSO96
	init_hz_p96(0);
#endif

	set_config_changed();

	custom_end_drawing();

	if (currprefs.monitoremu_mon != 0) {
		target_graphics_buffer_update(currprefs.monitoremu_mon, false);
	}
	target_graphics_buffer_update(0, false);

	if (vb->inwidth > vb->width_allocated)
		vb->inwidth = vb->width_allocated;
	if (vb->inwidth2 > vb->width_allocated)
		vb->inwidth2 = vb->width_allocated;

	if (vb->inheight > vb->height_allocated)
		vb->inheight = vb->height_allocated;
	if (vb->inheight2 > vb->height_allocated)
		vb->inheight2 = vb->height_allocated;

	if (vb->outwidth > vb->width_allocated)
		vb->outwidth = vb->width_allocated;

	if (vb->outheight > vb->height_allocated)
		vb->outheight = vb->height_allocated;

	resetfulllinestate();
}

/* set PAL/NTSC or custom timing variables */
static void init_beamcon0(void)
{
	int isntsc, islace;
	int hpos = current_hpos();
	int oldmaxhpos = maxhpos;

	beamcon0 = new_beamcon0;

	doublescan = 0;
	programmedmode = 0;
	lines_after_beamcon_change = 5;

	isntsc = (beamcon0 & BEAMCON0_PAL) ? 0 : 1;
	islace = (interlace_seen) ? 1 : 0;
	if (!ecs_agnus) {
		isntsc = currprefs.ntscmode ? 1 : 0;
	}
	beamcon0_pal = !isntsc;

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
	beamcon0_has_hsync = (beamcon0 & bemcon0_hsync_mask) != 0;
	beamcon0_has_vsync = (beamcon0 & bemcon0_vsync_mask) != 0;
	beamcon0_has_csync = (beamcon0 & BEAMCON0_VARCSYEN) != 0;
	beamcon0_dual = (new_beamcon0 & BEAMCON0_DUAL) != 0;
	agnus_equdis = ecs_agnus && ((beamcon0 & BEAMCON0_VARCSYEN) || (beamcon0 & BEAMCON0_BLANKEN));

#ifdef AMIBERRY // Don't change vblank_hz when opening P96 screens
	if (picasso_is_active(0)) {
		isntsc = currprefs.ntscmode ? 1 : 0;
	}
#endif
	float clk = (float)(currprefs.ntscmode ? CHIPSET_CLOCK_NTSC : CHIPSET_CLOCK_PAL);

	int display_maxvpos = current_linear_vpos_nom;
	int display_maxhpos = current_linear_hpos_short;
	float halfhpos = 0;

	if (!isntsc) {
		maxvpos = MAXVPOS_PAL;
		maxhpos = MAXHPOS_PAL;
		minfirstline = VBLANK_STOP_PAL;
		vblank_hz_nom = vblank_hz = CHIPSET_CLOCK_PAL / ((float)display_maxvpos * display_maxhpos);
	} else {
		maxvpos = MAXVPOS_NTSC;
		maxhpos = MAXHPOS_NTSC;
		minfirstline = VBLANK_STOP_NTSC;
		vblank_hz_nom = vblank_hz = CHIPSET_CLOCK_NTSC / ((float)display_maxvpos * display_maxhpos);
		halfhpos = (beamcon0 & BEAMCON0_LOLDIS) ? 0 : 0.5f;
	}
	vblank_hz_shf = clk / ((display_maxvpos + 0.0f) * (display_maxhpos + halfhpos));
	vblank_hz_lof = clk / ((display_maxvpos + 1.0f) * (display_maxhpos + halfhpos));
	vblank_hz_lace = clk / ((display_maxvpos + 0.5f) * (display_maxhpos + halfhpos));

	display_hstart_cyclewait_end = 4;
	if (beamcon0_has_hsync) {
		display_hstart_cyclewait = 4;
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
			display_hstart_cyclewait_end = 0;
		} else {
			if (aexthblanken && !(beamcon0 & BEAMCON0_VARBEAMEN)) {
				int hp2 = maxhpos * 2;
				int hbstrtx, hbstopx;
				int hb = 1;
				hbstrtx = (hbstrt & 0xff) * 2;
				hbstopx = (hbstop & 0xff) * 2;
				if (aga_mode) {
					hbstrtx |= (hbstrt & 0x400) ? 1 : 0;
					hbstopx |= (hbstop & 0x400) ? 1 : 0;
				}
				if (hbstrtx > hp2) {
					hbstrtx = hp2;
				}
				if (hbstopx > hp2) {
					hbstopx = hp2;
				}
				if (hbstrtx > hp2 / 2 && hbstopx < hp2 / 2) {
					hb = (hp2 - hbstrtx) + hbstopx;
				} else if (hbstopx < hp2 / 2 && hbstrtx < hbstopx) {
					hb = hbstopx - hbstrtx;
				}
				if (hbstopx > hp2 / 2) {
					hbstopx = 0;
				}
				if (hb < 1) {
					hb = 1;
				}

				display_hstart_cyclewait = 4;
				if (hbstopx / 2 > hsstrt && (hbstopx / 2 - hsstrt < maxhpos / 2)) {
					display_hstart_cyclewait = hbstopx / 2 - hsstrt;
				} else if (hsstrt - hbstopx / 2 < maxhpos / 2) {
					display_hstart_cyclewait = hsstrt - hbstopx / 2;
				}
				if (hb) {
					display_hstart_cyclewait_end = hb / 2 - display_hstart_cyclewait;
					display_hstart_cyclewait += 1;
				}
			}
		}
	} else {
		display_hstart_cyclewait = 0;
		display_hstart_cyclewait_end = 0;
		if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
			display_hstart_cyclewait = 32;
			display_hstart_cyclewait_end = 9;
		} else if (currprefs.gfx_overscanmode <= OVERSCANMODE_OVERSCAN) {
			display_hstart_cyclewait = 32;
			display_hstart_cyclewait_end = 10;
		} else if (currprefs.gfx_overscanmode == OVERSCANMODE_EXTREME) {
			display_hstart_cyclewait = 22;
			display_hstart_cyclewait_end = 5;
		}
	}

	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		// programmable scanrates (ECS Agnus)
		if (vtotal >= MAXVPOS) {
			vtotal = MAXVPOS - 1;
		}
		maxvpos = vtotal + 1;
		maxhpos = htotal + 1;
	}

	// after vsync, it seems earlier possible visible line is vsync+3.
	vsync_startline = 3;
	if ((beamcon0 & BEAMCON0_VARVBEN) && (beamcon0 & bemcon0_vsync_mask)) {
		vsync_startline += vsstrt;
		if (vsync_startline >= maxvpos / 2) {
			vsync_startline = 3;
		}
	}

	maxvpos_nom = maxvpos;
	maxvpos_display = maxvpos;

	// calculate max possible display width in lores pixels
	if ((beamcon0 & BEAMCON0_VARBEAMEN) && beamcon0_has_hsync) {
		int hsstrt_delay = 2;
		int hb = 0;
		display_hstart_cyclewait = 0;
		display_hstart_cyclewait_end = 0;
		// assume VGA-like monitor if VARBEAMEN
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
			maxhpos_display = maxhpos + 7;
			if (beamcon0_has_hsync) {
				hsstop_detect = hsstrt * 2;
				if (hsstop_detect > maxhpos / 2 * 2 || hsstop_detect < 4 * 2) {
					hsstop_detect = 4 * 2;
				}
			} else {
				hsstop_detect = 4 * 2;
			}
			minfirstline = 0;
		} else {
			int hp2 = maxhpos * 2;
			int hbstrtx, hbstopx;

			if (aexthblanken) {

				hb = 1;
				hbstrtx = (hbstrt & 0xff) * 2;
				hbstopx = (hbstop & 0xff) * 2;
				if (aga_mode) {
					hbstrtx |= (hbstrt & 0x400) ? 1 : 0;
					hbstopx |= (hbstop & 0x400) ? 1 : 0;
				}
				if (hbstrtx > hp2) {
					hbstrtx = hp2;
				}
				if (hbstopx > hp2) {
					hbstopx = hp2;
				}
				if (hbstrtx > hp2 / 2 && hbstopx < hp2 / 2) {
					hb = (hp2 - hbstrtx) + hbstopx;
				} else if (hbstopx < hp2 / 2 && hbstrtx < hbstopx) {
					hb = hbstopx - hbstrtx;
				}
				if (hbstopx > hp2 / 2) {
					hbstopx = 0;
				}
				if (hb < 1) {
					hb = 1;
				}

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
				hsstop_detect = hbstopx / 2;
				display_hstart_cyclewait_end += hb / 2 - 4;


			} else {

				// hardwired
				hbstrtx = 0x10;
				hbstopx = ecs_denise ? 0x5d : 0x5e;
				hb = hbstopx - hbstrtx;
				maxhpos_display = maxhpos - (hb / 2);
				if (beamcon0_has_hsync) {
					hsstop_detect = hsstrt * 2;
				} else {
					hsstop_detect = hsstop_detect2;
				}
				hb = 0;
			}

			if (hbstopx / 2 > hsstrt && (hbstopx / 2 - hsstrt < maxhpos / 2)) {
				display_hstart_cyclewait = hbstopx / 2 - hsstrt;
			} else if (hsstrt - hbstopx / 2 < maxhpos / 2) {
				display_hstart_cyclewait = hsstrt - hbstopx / 2;
			}
			if (hb) {
				display_hstart_cyclewait_end = hb / 2 - display_hstart_cyclewait;
				display_hstart_cyclewait += 1;
			}
			display_hstart_cyclewait += hsstrt_delay;

			if (currprefs.gfx_overscanmode >= OVERSCANMODE_EXTREME) {
				int diff = (maxhpos - 2) - maxhpos_display;
				if (diff > 0) {
					hsstop_detect -= (diff / 2) * 2;
				}
				maxhpos_display = maxhpos - 2;
			}

			maxhpos_display *= 2;
			if (display_hstart_cyclewait_end < 0) {
				display_hstart_cyclewait_end = 0;
			}
		}

	} else if (!(beamcon0 & BEAMCON0_VARBEAMEN)) {

		maxhpos_display = AMIGA_WIDTH_MAX;
		hsstop_detect = hsstop_detect2;

		if (beamcon0 & bemcon0_hsync_mask) {
			if (currprefs.gfx_overscanmode < OVERSCANMODE_BROADCAST) {
				hsstop_detect += 7;
			} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
				hsstop_detect += 5;
			} else if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
				maxhpos_display += EXTRAWIDTH_ULTRA;
				minfirstline = 0;
				hsstop_detect = hsyncstartpos_start_cycles * 2 - 4;
			}
		} else {

			if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
				maxhpos_display += EXTRAWIDTH_ULTRA;
				hsstop_detect = 18 * 2;
				minfirstline = 1;
			}
		}

	}

	if (!(beamcon0 & BEAMCON0_VARBEAMEN) || !beamcon0_has_hsync) {
		if (currprefs.gfx_overscanmode < OVERSCANMODE_BROADCAST) {
			// one pixel row missing from right border if OCS
			if (!ecs_denise) {
				maxhpos_display--;
			}
			minfirstline++;
			if (maxvpos_display_vsync > 0) {
				maxvpos_display_vsync--;
			} else {
				maxvpos_display--;
			}
		} else if (currprefs.gfx_overscanmode == OVERSCANMODE_EXTREME) {
			maxhpos_display += EXTRAWIDTH_EXTREME;
			hsstop_detect -= 4;
		} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
			maxhpos_display += EXTRAWIDTH_BROADCAST;
			hsstop_detect -= 1;
		}
	}

	if (currprefs.gfx_extrawidth > 0) {
		maxhpos_display += currprefs.gfx_extrawidth;
	}

	if (hsstop_detect < 0) {
		hsstop_detect = 0;
	}
	if (minfirstline < 0) {
		minfirstline = 0;
	}

	int minfirstline_hw = minfirstline;
	maxvpos_display_vsync = 0;
	if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
		minfirstline = 0;
		minfirstline_hw = 0;
		maxvpos_display_vsync = vsync_startline;
	} else if (currprefs.gfx_overscanmode == OVERSCANMODE_EXTREME) {
		minfirstline_hw -= EXTRAHEIGHT_EXTREME / 2;
		minfirstline -= EXTRAHEIGHT_EXTREME / 2;
		maxvpos_display_vsync += 2;
	} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
		minfirstline_hw -= EXTRAHEIGHT_BROADCAST_TOP;
		maxvpos_display_vsync++;
	}
	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		minfirstline_hw = 0;
	}

	if ((beamcon0 & BEAMCON0_VARVBEN) && (beamcon0 & bemcon0_vsync_mask)) {
		minfirstline = vsync_startline;
		if (minfirstline > maxvpos / 2) {
			minfirstline = vsync_startline;
		}
		minfirstline++;
	} else if (beamcon0 & BEAMCON0_VARVBEN) {
		if (minfirstline > vbstop) {
			minfirstline = vbstop;
			if (minfirstline < 3) {
				minfirstline = 3;
			}
		}
	}
	if (beamcon0 & (BEAMCON0_VARVBEN | bemcon0_vsync_mask)) {
		programmedmode = 2;
	}

	int eh = currprefs.gfx_extraheight;
	if (eh > 0) {
		if (beamcon0 & bemcon0_vsync_mask) {
			minfirstline -= eh / 2;
		} else {
			minfirstline -= eh / 2;
		}
	}

	if (display_hstart_cyclewait_end < 0) {
		display_hstart_cyclewait_end = 0;
	}

	if (minfirstline < vsync_startline) {
		minfirstline = vsync_startline;
	}

	if (minfirstline >= maxvpos) {
		minfirstline = maxvpos - 1;
	}

	if (minfirstline < minfirstline_hw) {
		minfirstline = minfirstline_hw;
	}

	if (beamcon0 & bemcon0_vsync_mask) {
		if (minfirstline < vsync_startline) {
			minfirstline = vsync_startline;
		}
	}

	if (beamcon0 & BEAMCON0_VARBEAMEN) {
		float half = (beamcon0 & BEAMCON0_PAL) ? 0: ((beamcon0 & BEAMCON0_LOLDIS) ? 0 : 0.5f);
		vblank_hz_nom = vblank_hz = clk / (maxvpos * (maxhpos + half));
		vblank_hz_shf = vblank_hz;
		vblank_hz_lof = clk / ((maxvpos + 1.0f) * (maxhpos + half));
		vblank_hz_lace = clk / ((maxvpos + 0.5f) * (maxhpos + half));

		maxvpos_nom = maxvpos;
		maxvpos_display = maxvpos;

		programmedmode = 2;
		if ((htotal < 226 || htotal > 229) || (vtotal < 256 || vtotal > 320)) {
			doublescan = htotal <= 164 && vtotal >= 350 ? 1 : 0;
			// if superhires and wide enough: not doublescan
			if (doublescan && htotal >= 140 && (bplcon0 & 0x0040))
				doublescan = 0;
			programmedmode = 1;
		}
	}

	if (maxvpos_nom >= MAXVPOS) {
		maxvpos_nom = MAXVPOS;
	}
	maxvpos_long = (beamcon0 & BEAMCON0_VARBEAMEN) ? -1 : maxvpos + 1;
	if (maxvpos_display >= MAXVPOS) {
		maxvpos_display = MAXVPOS;
	}
	if (currprefs.gfx_scandoubler && doublescan == 0) {
		doublescan = -1;
	}
	/* limit to sane values */
	if (vblank_hz < 10) {
		vblank_hz = 10;
	}
	if (vblank_hz > 300) {
		vblank_hz = 300;
	}
	maxhpos_short = maxhpos;
	updateextblk();
	maxvpos_total = ecs_agnus ? (MAXVPOS_LINES_ECS - 1) : (MAXVPOS_LINES_OCS - 1);
	if (maxvpos_total > MAXVPOS) {
		maxvpos_total = MAXVPOS;
	}

	minfirstline_linear = minfirstline - (vsync_startline > 0 ? vsync_startline : 0);

	int size = currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA ? 0 : 4;
	display_hstart_cyclewait_skip2 = display_hstart_cyclewait_end;
	display_hstart_cyclewait_skip = display_hstart_cyclewait - size;
	display_hstart_cyclewait = size;
	resetfulllinestate();
	updatehwhpostable();
}

static void init_hz_reset(void)
{
	linear_vpos = currprefs.ntscmode ? MAXVPOS_NTSC : MAXVPOS_PAL;
	linear_hpos = currprefs.ntscmode ? MAXHPOS_NTSC : MAXHPOS_PAL;
	linear_vpos += lof_store;
	linear_vpos -= vsync_startline;
	linear_vpos_prev[0] = linear_vpos;
	linear_vpos_prev[1] = linear_vpos;
	linear_vpos_prev[2] = linear_vpos;
	linear_hpos_prev[0] = linear_hpos;
	linear_hpos_prev[1] = linear_hpos;
	linear_hpos_prev[2] = linear_hpos;
	current_linear_vpos = linear_vpos + vsync_startline - lof_store;
	current_linear_hpos = linear_hpos;
	current_linear_vpos_nom = current_linear_vpos;
	current_linear_hpos_short = linear_hpos;
	current_linear_hpos_temp = current_linear_hpos;
	current_linear_vpos_temp = current_linear_vpos;
	current_linear_temp_change = 0;
	current_linear_vpos_visible = 0;
	init_hz();
}

void init_hz(void)
{
	int isntsc, islace;
	int odbl = doublescan;
	double ovblank = vblank_hz;
	int hzc = 0;
	int omaxhpos = maxhpos;
	int omaxvpos = maxvpos;

	isntsc = (new_beamcon0 & BEAMCON0_PAL) ? 0 : 1;
	islace = (interlace_seen) ? 1 : 0;
	if (!ecs_agnus) {
		isntsc = currprefs.ntscmode ? 1 : 0;
	}

	if ((beamcon0 & (BEAMCON0_VARBEAMEN | BEAMCON0_PAL)) != (new_beamcon0 & (BEAMCON0_VARBEAMEN | BEAMCON0_PAL))) {
		hzc = 1;
	}
	if ((beamcon0 & (BEAMCON0_VARVBEN | BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN)) != (new_beamcon0 & (BEAMCON0_VARVBEN | BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN))) {
		hzc = 1;
	}

	init_beamcon0();

	if (doublescan != odbl || maxvpos != omaxvpos) {
		hzc = 1;
	}

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

	if (vblank_hz != ovblank) {
		updatedisplayarea(0);
	}
	inputdevice_tablet_strobe();

	if (varsync_changed > 0) {
		varsync_changed = 0;
		dumpsync();
	}
	resetfulllinestate();
}

static void calcvdiw(void)
{
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

	plffirstline = vstrt;
	plflastline = vstop;
	need_vdiw_check = true;
}

/* display mode changed (lores, doubling etc..), recalculate everything */
void init_custom(void)
{
	check_nocustom();
	update_mirrors();
	create_cycle_diagram_table();
	reset_drawing();
	init_hz();
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

static bool blit_busy(bool dmaconr)
{
	if (currprefs.blitter_cycle_exact) {
		// DMACONR latch load takes 1 cycle. Copper sees it immediately.
		if (dmaconr) {
			if (get_cycles() <= blt_info.finishcycle_dmacon) {
				return true;
			}
		} else {
			if (get_cycles() <= blt_info.finishcycle_copper) {
				return true;
			}
		}
	}
	if (!blt_info.blit_main) {
		return false;
	}
	// AGA apparently fixes both bugs.
	if (agnusa1000) {
		// Blitter busy bug: A1000 Agnus only sets busy-bit when blitter gets first DMA slot.
		if (!blt_info.got_cycle) {
			return false;
		}
	}
	if (blt_info.blit_pending) {
		return true;
	}
	return true;
}

STATIC_INLINE uae_u16 DMACONR(void)
{
	dmacon &= ~(0x4000 | 0x2000);
	dmacon |= (blit_busy(true) ? 0x4000 : 0x0000) | (blt_info.blitzero ? 0x2000 : 0);
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
STATIC_INLINE int issyncstopped(uae_u16 con0)
{
	return (con0 & 2) && (!currprefs.genlock || currprefs.genlock_effects);
}

static void setsyncstopped(void)
{
	syncs_stopped = true;
	resetfulllinestate();
}

static void checksyncstopped(uae_u16 con0)
{
	if (issyncstopped(con0)) {
		if (!currprefs.cpu_memory_cycle_exact) {
			setsyncstopped();
		}
	} else if (syncs_stopped) {
		syncs_stopped = false;
		resetfulllinestate();
	}
}

static int GETVPOS(void)
{
	return islightpentriggered() ? vpos_lpen : vpos;
}
static int GETHPOS(void)
{
	if (islightpentriggered()) {
		return hpos_lpen;
	}
	if (!eventtab[ev_sync].active || syncs_stopped) {
		return agnus_hpos;
	}
	evt_t c = get_cycles();
	int hp = ((int)(c - eventtab[ev_sync].oldcycles)) / CYCLE_UNIT;
	return hp;
}

static void setmaxhpos(void)
{
	maxhpos = maxhpos_short + lol;
	maxhpos_lol = lol;
	maxhpos_long = linetoggle ? maxhpos_short + 1 : -1;
	maxhposm0 = maxhpos;
	maxhposm1 = maxhpos - 1;
	maxhposeven = (maxhposm1 & 1) == 0;
}

#define CPU_ACCURATE (currprefs.cpu_model < 68020 || (currprefs.cpu_model == 68020 && currprefs.cpu_memory_cycle_exact))

static void check_line_enabled(void)
{
	line_disabled &= ~3;
	//line_disabled |= line_hidden() ? 1 : 0;
	line_disabled |= custom_disabled ? 2 : 0;
}

static bool canvhposw(void)
{
	return !custom_disabled && !eventtab[ev_sync].active && currprefs.cpu_memory_cycle_exact;
}

static void incpos(uae_u16 *hpp, uae_u16 *vpp)
{
	uae_u16 hp = *hpp;
	uae_u16 vp = *vpp;

	if (syncs_stopped) {
		return;
	}
	if (hp == 1) {
		vp++;
		if (vp == maxvpos + lof_store) {
			vp = 0;
		}
	}
	hp++;
	if (hp == maxhpos || hp == maxhpos_long) {
		hp = 0;
	}
	if (canvhposw()) {
		if (agnus_pos_change >= 1) {
			if (agnus_vpos_next >= 0) {
				vp = agnus_vpos_next;
			}
			if (agnus_hpos_next >= 0) {
				hp = agnus_hpos_next;
			}
		}
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

	incpos(&hp, &vp);

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

#if 0
	if (1 || (M68K_GETPC < 0x00f00000 || M68K_GETPC >= 0x10000000))
		write_log (_T("VPOSR %04x at %08x\n"), vp, M68K_GETPC);
#endif
	return vp;
}

static void VPOSW(uae_u16 v)
{
#if VPOSW_DISABLED
	return;
#endif

	int newvpos = vpos;

#if 0
	if (M68K_GETPC < 0xf00000 || 1)
		write_log (_T("VPOSW %04X PC=%08x\n"), v, M68K_GETPC);
#endif
#if 1
	if (lof_store != ((v & 0x8000) ? 1 : 0)) {
		lof_store = (v & 0x8000) ? 1 : 0;
	}
#endif
	// LOL is always reset when VPOSW is written to.
	// Implemented in all NTSC Agnus versions and ECS/AGA Agnus in NTSC mode.
	if (lol) {
		lol = false;
		setmaxhpos();
	}
	newvpos &= 0x00ff;
	v &= 7;
	if (!ecs_agnus) {
		v &= 1;
	}
	newvpos |= v << 8;

	if (canvhposw()) {
		agnus_vpos_next = newvpos;
		agnus_pos_change = 2;
		resetfulllinestate();
	} else if (!currprefs.cpu_memory_cycle_exact) {
		if (newvpos > vpos && newvpos > minfirstline && vpos > minfirstline && newvpos < maxvpos) {
			vpos = newvpos;
			resetfulllinestate();
		}
	}
}

static void VHPOSW(uae_u32 v)
{
#if VPOSW_DISABLED
	return;
#endif

	int newvpos = vpos;

#if VPOSW_DEBUG
	if (M68K_GETPC < 0xf00000 || 1)
		write_log (_T("VHPOSW %04X PC=%08x\n"), v, M68K_GETPC);
#endif

	bool cpu_accurate = currprefs.m68k_speed >= 0 && !currprefs.cachesize && currprefs.cpu_memory_cycle_exact;
	if (cpu_accurate || copper_access) {
		int hnew = v & 0xff;
		agnus_hpos_next = hnew;
	}

	newvpos &= 0xff00;
	newvpos |= v >> 8;

	if (canvhposw()) {
		agnus_vpos_next = newvpos;
		agnus_pos_change = 2;
		resetfulllinestate();
	} else if (!currprefs.cpu_memory_cycle_exact) {
		if (newvpos > vpos && newvpos > minfirstline && vpos > minfirstline && newvpos < maxvpos) {
			vpos = newvpos;
			resetfulllinestate();
		}
	}
}

// 80E1 -> 80E2 -> 8000 -> 8001 -> 8102 -> 8103
static uae_u16 VHPOSR(void)
{
	uae_u16 vp = GETVPOS();
	uae_u16 hp = GETHPOS();
	incpos(&hp, &vp);
	vp <<= 8;
	vp |= hp;

#if 0
	if (0 || M68K_GETPC < 0x00f00000 || M68K_GETPC >= 0x10000000)
		write_log (_T("VHPOSR %04x at %08x %04x\n"), vp, M68K_GETPC, bplcon0);
#endif
	return vp;
}

static uae_u16 HHPOSR(void)
{
	uae_u16 v;
	if (islightpentriggered()) {
		v = hhpos_lpen;
	} else {
		v = hhpos;
	}
	v &= 0xff;
	return v;
}

static void HHPOS(uae_u16 v)
{
	hhpos = v & (MAXHPOS_ROWS - 1);
}

static void SPRHSTRT(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		sprhstrt = v;
		sprhstrt_v = v & (MAXVPOS_LINES_ECS - 1);
	}
}
static void SPRHSTOP(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		sprhstop = v;
		sprhstop_v = v & (MAXVPOS_LINES_ECS - 1);
	}
}
static void BPLHSTRT(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		bplhstrt = v;
		bplhstrt_v = v & (MAXVPOS_LINES_ECS - 1);
	}
}
static void BPLHSTOP(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		bplhstop = v;
		bplhstop_v = v & (MAXVPOS_LINES_ECS - 1);
	}
}
static void SPRHPTH(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		hhbpl &= 0x0000ffff;
		hhbpl |= v;
	}
}
static void SPRHPTL(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		hhbpl &= 0xffff0000;
		hhbpl |= v << 16;
	}
}
static void BPLHPTH(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		hhspr &= 0x0000ffff;
		hhspr |= v;
	}
}
static void BPLHPTL(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		hhspr &= 0xffff0000;
		hhspr |= v << 16;
	}
}
static void BPLHMOD(uae_u16 v)
{
	if (ecs_agnus) {
		programmed_register_accessed_h = true;
		bplhmod = v;
	}
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
		if (slot > 4) {
			slot = 4;
		}
		//update_refptr(-1, slot, true, true);
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

static int test_copper_dangerous(uae_u16 reg, bool testonly)
{
	int addr = reg & 0x01fe;
	if (addr < ((copcon & 2) ? (ecs_agnus ? 0 : 0x40) : 0x80)) {
		if (!testonly) {
			cop_state.state = COP_stop;
			copper_enabled_thisline = 0;
		}
		return 1;
	}
	return 0;
}

// if DMA was changed during same cycle: previous value is used
static bool is_blitter_dma(void)
{
	bool dma = dmaen(DMA_BLITTER);
	if (get_cycles() <= blitter_dma_change_cycle) {
		return dma == false;
	}
	return dma;
}
static bool is_copper_dma(bool checksame)
{
	bool dma = dmaen(DMA_COPPER);
	if (checksame && get_cycles() <= copper_dma_change_cycle) {
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
		if (oldpos > pos) {
			pos = oldpos;
		}
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
			if (test_copper_dangerous(cop_state.ir[0], false)) {
				break;
			}
			custom_wput_1(cop_state.ir[0], cop_state.ir[1], 0);
		} else { // wait or skip
			if ((cop_state.ir[0] >> 8) > ((pos >> 5) & 0xff))
				pos = (((pos >> 5) & 0x100) | ((cop_state.ir[0] >> 8)) << 5) | ((cop_state.ir[0] & 0xff) >> 3);
			if (cop_state.ir[0] >= 0xffdf && cop_state.ir[1] == 0xfffe) {
				break;
			}
		}
	}
	cop_state.state = COP_stop;
}

STATIC_INLINE void COP1LCH(uae_u16 v)
{
	cop1lc = (cop1lc & 0xffff) | ((uae_u32)v << 16);

	if (agnus_hpos == 2 && vpos == 0 && safecpu() && !copper_access && is_copper_dma(false)) {
		if (cop_state.state == COP_strobe_vbl_delay) {
			cop_state.strobeip = cop1lc | ((regs.chipset_latch_rw & 0xffff) << 16);
		}
	}
}
STATIC_INLINE void COP1LCL(uae_u16 v)
{
	cop1lc = (cop1lc & ~0xffff) | (v & 0xfffe);

	// really strange chipset bug: if COP1LCL is written exactly at cycle 2, vpos 0,
	// vblank triggered COP1JMP loads to internal COPPTR COP1LC OR last data in chip bus!
	if (agnus_hpos == 2 && vpos == 0 && safecpu() && !copper_access && is_copper_dma(true)) {
		if (cop_state.state == COP_strobe_vbl_delay) {
			cop_state.strobeip = cop1lc | (regs.chipset_latch_rw & 0xfffe);
		}
	}
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

static uaecptr getstrobecopip(void)
{
	if (cop_state.strobe == 3) {
		return cop1lc | cop2lc;
	} else if (cop_state.strobe == 2) {
		return cop2lc;
	} else {
		return cop1lc;
	}

}
static void setstrobecopip(void)
{
	cop_state.ip = getstrobecopip();
	cop_state.strobe = 0;
}

static struct rgabuf *generate_copper_cycle_if_free(uae_u16 v)
{
	if (is_copper_dma(true) && check_rga_free_slot_in()) {
		struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_COPPER, 0x8c, &cop_state.ip);
		rga->copdat = v;
		return rga;
	}
	return NULL;
}

// normal COPJMP write: takes 2 more cycles
static void COPJMP(int num, int vblank)
{
	cop_state.strobe_next = COP_stop;

	if (!cop_state.strobe) {
		cop_state.state_prev = cop_state.state;
	}
	cop_state.strobetype = 0;
	if ((cop_state.state == COP_wait1 || cop_state.state == COP_waitforever || cop_state.state == COP_stop ||
		(cop_state.state == COP_read2 && ((cop_state.ir[0] & 1) || test_copper_dangerous(cop_state.ir[0], true))) ||
		cop_state.state == COP_wait_in2 || cop_state.state == COP_skip_in2) &&
		!vblank && is_copper_dma(false)) {
		// no copper request for next copper cycle
		if (blt_info.blit_main) {
			static int warned = 100;
			if (warned > 0) {
				write_log(_T("Potential buggy copper cycle conflict with blitter PC=%08x, COP=%08x\n"), M68K_GETPC, cop_state.ip);
				warned--;
				//activate_debugger();
			}
		}
		int hp = current_hpos();
		if ((hp & 1) && safecpu()) {
			// CPU unaligned COPJMP while waiting
			cop_state.strobe_next = COP_strobe_delay_start_odd;
			cop_state.strobetype = -1;
		} else {
			cop_state.state = COP_strobe_delay_start;
		}
		cop_state.strobeip = 0xffffffff;
	} else {
		// copper request done for next cycle
		if (vblank) {
			if (!is_copper_dma(false)) {
				cop_state.state = COP_strobe_vbl_delay_nodma;
			} else {
				cop_state.state = COP_strobe_vbl_delay;
				cop_state.strobeip = cop1lc;
				cop_state.strobe = 0;
				struct rgabuf *r = read_rga_in();
				r->type |= CYCLE_COPPER;
				r->copdat = 0;
				if (!r->alloc) {
					r->alloc = -1;
				}
#if 1
				switch (cop_state.state_prev)
				{
					case copper_states::COP_read1:
						// Wake up is delayed by 1 copper cycle if copper is currently loading words
						cop_state.state = COP_strobe_vbl_extra_delay1;
						break;
					case copper_states::COP_read2:
						// Wake up is delayed by 1 copper cycle if copper is currently loading words
						cop_state.state = COP_strobe_vbl_extra_delay2;
						break;
				}
#endif
			}
		} else {
			if (copper_access) {
				cop_state.state = COP_strobe_delay1;
			} else {
				cop_state.strobe_next = COP_strobe_delay1;
			}
			cop_state.strobetype = 1;
			if (cop_state.state == COP_read2 && !(cop_state.ir[0] & 1)) {
				cop_state.strobetype = 2;
			}
		}
	}
	cop_state.vblankip = cop1lc;
	cop_state.strobe_cycles = get_cycles() + CYCLE_UNIT;

	if (!vblank) {
		cop_state.strobe |= num;
	}

	if (custom_disabled) {
		copper_enabled_thisline = 0;
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
	if (copper_enabled_thisline < 0 && !is_copper_dma(false)) {
		copper_enabled_thisline = 0;
	}
}

static void copper_stop(void)
{
	copper_enabled_thisline = 0;
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

	setclr(&dmacon, v);
	dmacon &= 0x07FF;

	changed = dmacon ^ oldcon;

	if (0) {
		write_log("%04x: %04x -> %04x %08x %08x\n", changed, oldcon, dmacon, M68K_GETPC, cop_state.ip);
	}

	int oldcop = (oldcon & DMA_COPPER) != 0 && (oldcon & DMA_MASTER) != 0;
	int newcop = (dmacon & DMA_COPPER) != 0 && (dmacon & DMA_MASTER) != 0;
	if (!oldcop && newcop) {
		bootwarpmode();
	}
	if (newcop && !oldcop) {
		if (safecpu()) {
			if (copper_access) {
				copper_dma_change_cycle = get_cycles();
			} else {
				copper_dma_change_cycle = get_cycles() + CYCLE_UNIT;
			}
		}
		copper_enabled_thisline = 1;
	} else if (!newcop && oldcop) {
		if (safecpu()) {
			if (copper_access) {
				copper_dma_change_cycle = get_cycles();
			} else {
				copper_dma_change_cycle = get_cycles() + CYCLE_UNIT;
			}
		}
		copper_enabled_thisline = 1;
	}

	int oldblt = (oldcon & DMA_BLITTER) != 0 && (oldcon & DMA_MASTER) != 0;
	int newblt = (dmacon & DMA_BLITTER) != 0 && (dmacon & DMA_MASTER) != 0;
	if (oldblt != newblt && (copper_access || safecpu())) {
		if (copper_access) {
			blitter_dma_change_cycle = get_cycles();
		} else {
			// because of CPU vs blitter emulation side-effect
			blitter_dma_change_cycle = get_cycles() + CYCLE_UNIT;
		}
	}

	int oldspr = (oldcon & DMA_SPRITE) != 0 && (oldcon & DMA_MASTER) != 0;
	int newspr = (dmacon & DMA_SPRITE) != 0 && (dmacon & DMA_MASTER) != 0;
	if (!oldspr && newspr) {
		if (copper_access || safecpu()) {
			sprite_dma_change_cycle_on = get_cycles() + CYCLE_UNIT;
		}
	} else if (oldspr && !newspr) {
		if (copper_access || safecpu()) {
			sprite_dma_change_cycle_off = get_cycles();
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

	if ((dmacon & DMA_BLITPRI) > (oldcon & DMA_BLITPRI) && (blt_info.blit_main || blt_info.blit_queued)) {
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
	serial_rbf_change((newreq & 0x0800) ? 1 : 0);
}

static void event_doint_delay_do_ext_old(uae_u32 v)
{
	uae_u16 old = intreq2;
	setclr(&intreq, (1 << v) | 0x8000);
	setclr(&intreq2, (1 << v) | 0x8000);

	doint();
}

void event_doint_delay_do_ext(uae_u32 v)
{
	uae_u16 old = intreq2;
	setclr(&intreq, v | 0x8000);
	setclr(&intreq2, v | 0x8000);

	doint();
}

static void event_send_interrupt_do_ext(uae_u32 v)
{
	event2_newevent_xx(-1, 1 * CYCLE_UNIT, 1 << v, event_doint_delay_do_ext);
}

// external delayed interrupt
void INTREQ_INT(int num, int delay)
{
	if (m68k_interrupt_delay) {
		if (delay < CYCLE_UNIT) {
			delay *= CYCLE_UNIT;
		}
		event2_newevent_xx(-1, delay + CYCLE_UNIT, 1 << num, event_doint_delay_do_ext);
	} else {
		event_doint_delay_do_ext(1 << num);
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
#if 0
		if (!(v & 0x8000) && (v & (0x400 | 0x200 | 0x100 | 0x80))) {
			// same cycle interrupt set? override it
			evt_t c = get_cycles() + 1 * CYCLE_UNIT;
			for (int i = 0; i < ev2_max; i++) {
				struct ev2 *ev = &eventtab2[i];
				if (ev->active && ev->evtime == c && ev->handler == event_doint_delay_do_ext) {
					if (ev->data == 7 && (v & 0x0080) ||
						ev->data == 8 && (v & 0x0100) ||
						ev->data == 9 && (v & 0x0200) ||
						ev->data == 10 && (v & 0x0400)) {
						ev->active = false;
					}
				}
			}
		}
#endif
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

static void update_lof_detect(void)
{
	if (lof_detect_vsync != agnus_vb_active) {
		lof_detect_vsync = agnus_vb_active;
	}
}

static void update_agnus_vb(void)
{
	if (new_beamcon0 & BEAMCON0_VARVBEN) {
		agnus_vb_active = agnus_pvb;
		agnus_vb_active_end_line = agnus_pvb_end_line;
		agnus_vb_active_start_line = agnus_pvb_start_line;
		if (agnus_pvb_start_line) {
			current_linear_vpos_visible = linear_vpos_visible;
		}
		if (agnus_pvb_end_line) {
			linear_vpos_visible = 1;
		}
	} else {
		agnus_vb_active = agnus_vb == 1;
		agnus_vb_active_end_line = agnus_vb_end_line;
		agnus_vb_active_start_line = agnus_vb_start_line;
		if (agnus_vb_start_line) {
			current_linear_vpos_visible = linear_vpos_visible;
		}
		if (agnus_vb_end_line) {
			linear_vpos_visible = 1;
		}
	}
}

static void BEAMCON0(uae_u16 v)
{
	if (ecs_agnus) {
		bool beamcon0_changed = false;
		if (v != new_beamcon0) {
			new_beamcon0 = v;
			write_log(_T("BEAMCON0 = %04X, PC=%08X\n"), v, M68K_GETPC);
			// not LPENDIS, LOLDIS, PAL, BLANKEN, SYNC INVERT
			if (v & ~(BEAMCON0_LPENDIS | BEAMCON0_LOLDIS | BEAMCON0_PAL | BEAMCON0_BLANKEN | BEAMCON0_CSYTRUE | BEAMCON0_VSYTRUE | BEAMCON0_HSYTRUE)) {
				dumpsync();
			}
			beamcon0_changed = true;
			beamcon0_saved = v;
			check_harddis();
			update_agnus_vb();
		}
		if (beamcon0_changed) {
			init_beamcon0();
		}
		need_vdiw_check = true;
	}
}

static void check_exthblank(void)
{
	aexthblanken = (bplcon0 & 1) && (bplcon3 & 1) && ecs_denise;
	resetfulllinestate();
	// Recalculate beamcon0 settings if exthblank was set after BEAMCON0 modification (hblank is used for programmed mode centering)
	if (lines_after_beamcon_change > 0) {
		init_beamcon0();
		lines_after_beamcon_change = 0;
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
		init_beamcon0();
	}
}

#ifdef PICASSO96
void set_picasso_hack_rate(int hz)
{
	struct amigadisplay *ad = &adisplays[0];
	if (!ad->picasso_on) {
		return;
	}
	p96refresh_active = (int)(maxvpos_stored * vblank_hz_stored / hz);
	if (!currprefs.cs_ciaatod) {
		changed_prefs.cs_ciaatod = currprefs.cs_ciaatod = currprefs.ntscmode ? 2 : 1;
	}
	if (p96refresh_active > 0) {
		new_beamcon0 |= BEAMCON0_VARBEAMEN;
	}
	varsync_changed = 1;
}
#endif

static void BPLxPTH(uae_u16 v, int num)
{
	bplpt[num] = (bplpt[num] & 0x0000ffff) | ((uae_u32)v << 16);
}
static void BPLxPTL(uae_u16 v, int num)
{
	bplpt[num] = (bplpt[num] & 0xffff0000) | (v & 0x0000fffe);
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

static void BPLCON0_delayed(uae_u32 va)
{
	if ((bplcon0 & 1) != (va & 1) && (bplcon3 & 1)) {
		bplcon0 = va;
		check_exthblank();
	}

	bplcon0 = va;

	check_harddis();

	setup_fmodes(bplcon0);

	if ((va & 8) && !lightpen_triggered && agnus_vb_active) {
		// setting lightpen bit immediately freezes VPOSR if inside vblank and not already frozen
		hhpos_lpen = HHPOSR();
		lightpen_triggered = 1;
		vpos_lpen = vpos;
		hpos_lpen = agnus_hpos;
	}
	if (!(va & 8)) {
		// clearing lightpen bit immediately returns VPOSR back to normal
		lightpen_triggered = 0;
	}

	checksyncstopped(va);

	color_table_changed = true;
}

static void BPLCON0(uae_u16 v)
{
	uae_u16 old = bplcon0;
	bplcon0_saved = v;
	uae_u16 va = BPLCON0_Agnus_mask(v);

#if SPRBORDER
	v |= 1;
#endif
	if (bplcon0 == va) {
		return;
	}

	// UHRES, BYPASS
	if (va & (0x0080 | 0x0020)) {
		not_safe_mode |= 1;
	} else {
		not_safe_mode &= ~1;
	}

	if (copper_access || (!copper_access && currprefs.cpu_memory_cycle_exact)) {
		pipelined_custom_write(BPLCON0_delayed, v, 2);
	} else {
		BPLCON0_delayed(v);
	}
}

static void BPLCON1(uae_u16 v)
{
#if DISABLE_BPLCON1
	v = 0;
#endif
	bplcon1_saved = v;
	bplcon1 = v;
}
static void BPLCON2(uae_u16 v)
{
	bplcon2_saved = v;
	bplcon2 = v;
}
static void BPLCON3(uae_u16 v)
{
	if ((bplcon0 & 1) && (bplcon3 & 1) != (v & 1)) {
		bplcon3 = v;
		check_exthblank();
	}
	bplcon3_saved = v;
	bplcon3 = v;
	if (aga_mode) {
		color_table_changed = true;
	}
}
static void BPLCON4(uae_u16 v)
{
	bplcon4_saved = v;
	bplcon4 = v;
}

static void BPL1MOD(uae_u16 v)
{
	v &= ~1;
	bpl1mod = v;
}
static void BPL2MOD(uae_u16 v)
{
	v &= ~1;
	bpl2mod = v;
}

static void BPLxDAT(int num, uae_u16 data)
{
	write_drga(0x110 + num * 2, 0xffffffff, data);
}

static void DIWSTRT(uae_u16 v)
{
	diwhigh_saved &= ~0x8000;
	if (diwstrt == v && !diwhigh_written) {
		return;
	}
	diwhigh_written = false;
	diwstrt = v;
	calcvdiw();
}

static void DIWSTOP(uae_u16 v)
{
	diwhigh_saved &= ~0x8000;
	if (diwstop == v && !diwhigh_written) {
		return;
	}
	diwhigh_written = false;
	diwstop = v;
	calcvdiw();
}

static void DIWHIGH(uae_u16 v)
{
	diwhigh_saved = v | 0x8000;
	if (!ecs_agnus) {
		return;
	}
	if (!aga_mode) {
		v &= ~(0x0010 | 0x1000);
	}
	v &= ~(0x8000 | 0x4000 | 0x0080 | 0x0040);
	if (diwhigh_written && diwhigh == v) {
		return;
	}
	diwhigh_written = true;
	diwhigh = v;
	calcvdiw();
}

static void DDFSTRT(uae_u16 v)
{
	// DDFSTRT modified when DDFSTRT==hpos: neither value matches
	ddfstrt_saved = v;
	v &= ddf_mask;
	ddfstrt = 0xffff;
	push_pipeline(&ddfstrt, v);
}

static void DDFSTOP(uae_u16 v)
{
	// DDFSTOP modified when DDFSTOP==hpos: old value matches
	ddfstop_saved = v;
	v &= ddf_mask;
	push_pipeline(&ddfstop, v);
}

static void FMODE(uae_u16 v)
{
	if (!aga_mode) {
		if (currprefs.monitoremu) {
			specialmonitor_store_fmode(vpos, agnus_hpos, v);
		}
		fmode_saved = v;
		v = 0;
	} else {
		fmode_saved = v;
	}
	v &= 0xC00F;
	if (fmode == v) {
		return;
	}
	set_chipset_mode(false);
	setup_fmodes(bplcon0);
}

static void FNULL(uae_u16 v)
{

}

static void BLTADAT(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltadat = v;
}
/*
* "Loading data shifts it immediately" says the HRM. Well, that may
* be true for BLTBDAT, but not for BLTADAT - it appears the A data must be
* loaded for every word so that AFWM and ALWM can be applied.
*/
static void BLTBDAT(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltbdat = v;
	blitter_loadbdat(v);
}
static void BLTCDAT(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltcdat = v;
	blitter_loadcdat(v);
	reset_blit(0);
}
static void BLTAMOD(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltamod_next = (uae_s16)(v & 0xFFFE);
	// this delay due to copper/cpu vs blitter timing is slightly differently implemented
	blt_info.blt_mod_cycles[0] = get_cycles() + 2 * CYCLE_UNIT;
	if (!copper_access) {
		blt_info.bltamod = blt_info.bltamod_next;
	}
	blitter_delayed_update = true;
}
static void BLTBMOD(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltbmod_next = (uae_s16)(v & 0xFFFE);
	blt_info.blt_mod_cycles[1] = get_cycles() + 2 * CYCLE_UNIT;
	if (!copper_access) {
		blt_info.bltbmod = blt_info.bltbmod_next;
	}
	blitter_delayed_update = true;
}
static void BLTCMOD(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltcmod_next = (uae_s16)(v & 0xFFFE);
	blt_info.blt_mod_cycles[2] = get_cycles() + 2 * CYCLE_UNIT;
	if (!copper_access) {
		blt_info.bltcmod = blt_info.bltcmod_next;
	}
	blitter_delayed_update = true;
}
static void BLTDMOD(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltdmod_next = (uae_s16)(v & 0xFFFE);
	blt_info.blt_mod_cycles[3] = get_cycles() + 2 * CYCLE_UNIT;
	if (!copper_access) {
		blt_info.bltdmod = blt_info.bltdmod_next;
	}
	blitter_delayed_update = true;
}
static void BLTCON0(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltcon0 = v;
	reset_blit(1);
}
/* The next category is "Most useless hardware register".
* And the winner is... */
static void BLTCON0L(uae_u16 v)
{
	if (!ecs_agnus)
		return; // ei voittoa.
	maybe_blit(0);
	blt_info.bltcon0 = (blt_info.bltcon0 & 0xFF00) | (v & 0xFF);
	reset_blit(0);
}
static void BLTCON1(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltcon1 = v;
	reset_blit(2);
}
static void BLTAFWM(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltafwm = v;
}
static void BLTALWM(uae_u16 v)
{
	maybe_blit(0);
	blt_info.bltalwm = v;
}
static void BLTAPTH(uae_u16 v)
{
	maybe_blit(1);
	blt_info.bltapt_prev = blt_info.bltapt;
	blt_info.blt_ch_cycles[0] = get_cycles();
	blt_info.bltapt = (blt_info.bltapt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTAPTL(uae_u16 v)
{
	maybe_blit(1);
	blt_info.bltapt_prev = blt_info.bltapt;
	blt_info.blt_ch_cycles[0] = get_cycles();
	blt_info.bltapt = (blt_info.bltapt & ~0xffff) | (v & 0xfffe);
}
static void BLTBPTH(uae_u16 v)
{
	maybe_blit(1);
	blt_info.bltbpt_prev = blt_info.bltbpt;
	blt_info.blt_ch_cycles[1] = get_cycles();
	blt_info.bltbpt = (blt_info.bltbpt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTBPTL(uae_u16 v)
{
	maybe_blit(1);
	blt_info.bltbpt_prev = blt_info.bltbpt;
	blt_info.blt_ch_cycles[1] = get_cycles();
	blt_info.bltbpt = (blt_info.bltbpt & ~0xffff) | (v & 0xfffe);
}
static void BLTCPTH(uae_u16 v)
{
	maybe_blit(1);
	blt_info.bltcpt_prev = blt_info.bltcpt;
	blt_info.blt_ch_cycles[2] = get_cycles();
	blt_info.bltcpt = (blt_info.bltcpt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTCPTL(uae_u16 v)
{
	maybe_blit(1);
	blt_info.bltcpt_prev = blt_info.bltcpt;
	blt_info.blt_ch_cycles[2] = get_cycles();
	blt_info.bltcpt = (blt_info.bltcpt & ~0xffff) | (v & 0xfffe);
}
static void BLTDPTH(uae_u16 v)
{
#if 0
	if (blt_info.blit_finald && copper_access) {
		static int warned = 100;
		if (warned > 0) {
			warned--;
			write_log("Possible Copper Blitter wait bug detected COP=%08x\n", cop_state.ip);
		}
	}
#endif
	maybe_blit(1);
	blt_info.bltdpt_prev = blt_info.bltdpt;
	blt_info.blt_ch_cycles[3] = get_cycles();
	blt_info.bltdpt = (blt_info.bltdpt & 0xffff) | ((uae_u32)v << 16);
}
static void BLTDPTL(uae_u16 v)
{
#if 0
	if (blt_info.blit_finald && copper_access) {
		static int warned = 100;
		if (warned > 0) {
			warned--;
			write_log("Possible Copper Blitter wait bug detected COP=%08x\n", cop_state.ip);
		}
	}
#endif
	maybe_blit(1);
	blt_info.bltdpt_prev = blt_info.bltdpt;
	blt_info.blt_ch_cycles[3] = get_cycles();
	blt_info.bltdpt = (blt_info.bltdpt & ~0xffff) | (v & 0xfffe);
}

static void BLTSIZE_func(uae_u32 v)
{
	maybe_blit(1);
	blt_info.vblitsize = v >> 6;
	blt_info.hblitsize = v & 0x3F;
	if (!blt_info.vblitsize) {
		blt_info.vblitsize = 1024;
	}
	if (!blt_info.hblitsize) {
		blt_info.hblitsize = 64;
	}
	do_blitter(copper_access, copper_access ? cop_state.ip : M68K_GETPC);
}
static void BLTSIZV_func(uae_u32 v)
{
	maybe_blit(0);
	blt_info.vblitsize = v & 0x7FFF;
	if (!blt_info.vblitsize) {
		blt_info.vblitsize = 0x8000;
	}
}

static void BLTSIZH_func(uae_u32 v)
{
	maybe_blit(0);
	blt_info.hblitsize = v & 0x7FF;
	if (!blt_info.vblitsize) {
		blt_info.vblitsize = 0x8000;
	}
	if (!blt_info.hblitsize) {
		blt_info.hblitsize = 0x0800;
	}
	do_blitter(copper_access, copper_access ? cop_state.ip : M68K_GETPC);
}

static void BLTSIZE(int hpos, uae_u16 v)
{
	int wait = currprefs.m68k_speed == 0 && currprefs.blitter_cycle_exact;
	pipelined_custom_write(BLTSIZE_func, v, wait);
}
static void BLTSIZV(int hpos, uae_u16 v)
{
	if (ecs_agnus) {
		int wait = currprefs.m68k_speed == 0 && currprefs.blitter_cycle_exact;
		pipelined_custom_write(BLTSIZV_func, v, wait);
	}
}
static void BLTSIZH(int hpos, uae_u16 v)
{
	if (ecs_agnus) {
		int wait = currprefs.m68k_speed == 0 && currprefs.blitter_cycle_exact;
		pipelined_custom_write(BLTSIZH_func, v, wait);
	}
}

static void sprstartstop(struct sprite *s)
{
	if (agnus_vb_active) {
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
	struct sprite *s = &spr[num];

	s->vstart = s->pos >> 8;
	s->vstart |= (s->ctl & 0x04) ? 0x0100 : 0;
	s->vstop = s->ctl >> 8;
	s->vstop |= (s->ctl & 0x02) ? 0x100 : 0;
	if (ecs_agnus) {
		s->vstart |= (s->ctl & 0x40) ? 0x0200 : 0;
		s->vstop |= (s->ctl & 0x20) ? 0x0200 : 0;
	}
	s->dblscan = false;
#ifdef AGA
	if (aga_mode) {
		s->dblscan = (s->pos & 0x80) != 0;
	}
#endif
}

static void SPRxCTL(uae_u16 v, int num)
{
	struct sprite *s = &spr[num];

	s->ctl = v;
	sprstartstop(s);
	SPRxCTLPOS(num);
	sprstartstop(s);
}
static void SPRxCTL_DMA(uae_u16 v, int num)
{
	struct sprite *s = &spr[num];

	s->ctl = v;
	sprstartstop(s);
	SPRxCTLPOS(num);
	// This is needed to disarm previous field's sprite.
	// It can be seen on OCS Agnus + ECS Denise combination where
	// this cycle is disabled due to weird DDFTSTR=$18 copper list
	// which causes corrupted sprite to "wrap around" the display.
	s->dmastate = 0;
	sprstartstop(s);
}
static void SPRxPOS(uae_u16 v, int num)
{
	struct sprite *s = &spr[num];

	s->pos = v;
	sprstartstop(s);
	SPRxCTLPOS(num);
	sprstartstop(s);
}


// Undocumented AGA feature: if sprite is 64 pixel wide, SPRxDATx is written and next
// cycle is DMA fetch: sprite's first 32 pixels get replaced with bitplane data.
#if 0
static void sprite_get_bpl_data(int hpos, struct sprite *s, uae_u16 *dat)
{
	int nr = get_bitplane_dma_rel(hpos, 1);
	uae_u32 v = (uae_u32)((fmode & 3) ? fetched_aga[nr] : fetched_aga_spr[nr]);
	dat[0] = v >> 16;
	dat[1] = (uae_u16)v;
}
#endif

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

#if 0
static void SPRxDATA(uae_u16 v, int num)
{
	struct sprite *s = &spr[num];
	SPRxDATA_1(v, num);
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

static void SPRxDATB(uae_u16 v, int num)
{
	struct sprite *s = &spr[num];
	SPRxDATB_1(v, num);
	// See above
	if (fmode & 8) {
		if ((fmode & 4) && get_bitplane_dma_rel(hpos, -1)) {
			sprite_get_bpl_data(hpos, s, &s->datb[0]);
		} else {
			s->datb[0] = last_custom_value;
		}
	}
}
#endif

static void SPRxPTH(uae_u16 v, int num)
{
	spr[num].pt &= 0xffff;
	spr[num].pt |= (uae_u32)v << 16;
}

static void SPRxPTL(uae_u16 v, int num)
{
	spr[num].pt &= ~0xffff;
	spr[num].pt |= v & ~1;
}

static void CLXCON(uae_u16 v)
{
	clxcon = v;
}

static void CLXCON2(uae_u16 v)
{
	clxcon2 = v;
}

static void draw_line(int, bool);
static uae_u16 CLXDAT(void)
{
	// draw line up to current horizontal position to get accurate collision state
	if (currprefs.cpu_memory_cycle_exact && currprefs.m68k_speed >= 0 && !doflickerfix_active()) {
		int ldvpos = linear_display_vpos + draw_line_next_line;
		draw_line(ldvpos, false);
	}
	draw_denise_line_queue_flush();
	uae_u16 v = clxdat | 0x8000;
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
		rgb1 = agnus_colors.color_regs_aga[c1];
		rgb2 = agnus_colors.color_regs_aga[c2];
		rgb3 = agnus_colors.color_regs_aga[c3];
		rgb4 = agnus_colors.color_regs_aga[c4];
		console_out_f (_T("%3d %08X %3d %08X %3d %08X %3d %08X\n"),
			c1, rgb1, c2, rgb2, c3, rgb3, c4, rgb4);
	}
}

static uae_u16 COLOR_READ(int num)
{
	if (!aga_mode || !(bplcon2 & 0x0100)) {
		return 0xffff;
	}
	uae_u16 cval;
	int colreg = ((bplcon3 >> 13) & 7) * 32 + num;
	int cr = (agnus_colors.color_regs_aga[colreg] >> 16) & 0xFF;
	int cg = (agnus_colors.color_regs_aga[colreg] >> 8) & 0xFF;
	int cb = agnus_colors.color_regs_aga[colreg] & 0xFF;

	if (bplcon3 & 0x200) {
		cval = ((cr & 15) << 8) | ((cg & 15) << 4) | ((cb & 15) << 0);
	} else {
		cval = ((cr >> 4) << 8) | ((cg >> 4) << 4) | ((cb >> 4) << 0);
		if (agnus_colors.color_regs_genlock[num]) {
			cval |= 0x8000;
		}
	}
	return cval;
}

static void COLOR_WRITE(uae_u16 v, int num)
{
	if (aga_mode) {
		if (bplcon2 & 0x0100) {
			return; // RDRAM=1
		}

		int colreg = ((bplcon3 >> 13) & 7) * 32 + num;
		int r = (v & 0xF00) >> 8;
		int g = (v & 0xF0) >> 4;
		int b = (v & 0xF) >> 0;
		int cr = (agnus_colors.color_regs_aga[colreg] >> 16) & 0xFF;
		int cg = (agnus_colors.color_regs_aga[colreg] >> 8) & 0xFF;
		int cb = agnus_colors.color_regs_aga[colreg] & 0xFF;

		if (bplcon3 & 0x200) {
			cr &= 0xF0; cr |= r;
			cg &= 0xF0; cg |= g;
			cb &= 0xF0; cb |= b;
		} else {
			cr = r + (r << 4);
			cg = g + (g << 4);
			cb = b + (b << 4);
			agnus_colors.color_regs_genlock[colreg] = v >> 15;
		}
		uae_u32 cval = (cr << 16) | (cg << 8) | cb;

		agnus_colors.color_regs_aga[colreg] = cval;
		agnus_colors.acolors[colreg] = getxcolor(cval);

	} else {

		if (!ecs_denise) {
			v &= 0xfff;
		}
		agnus_colors.color_regs_ecs[num] = v & 0xfff;
		agnus_colors.color_regs_genlock[num] = v >> 15;
		agnus_colors.acolors[num] = getxcolor(v);
		agnus_colors.color_regs_aga[num] = agnus_colors.acolors[num];

	}

	color_table_changed = true;
}

#endif

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
		*r = (agnus_colors.color_regs_aga[colreg] >> 16) & 0xFF;
		*g = (agnus_colors.color_regs_aga[colreg] >> 8) & 0xFF;
		*b = agnus_colors.color_regs_aga[colreg] & 0xFF;
	} else {
		*r = (agnus_colors.color_regs_ecs[colreg] >> 8) & 15;
		*r |= (*r) << 4;
		*g = (agnus_colors.color_regs_ecs[colreg] >> 4) & 15;
		*g |= (*g) << 4;
		*b = (agnus_colors.color_regs_ecs[colreg] >> 0) & 15;
		*b |= (*b) << 4;
	}
	return true;
}

bool blitter_cant_access(void)
{
	if (!is_blitter_dma()) {
		return true;
	}
	return false;
}

// return if register is in Agnus or Denise (or both)
static int get_reg_chip(int reg)
{
#if VPOSW_DISABLED
	if (reg == 0x2a || reg == 0x2c) {
		return 0;
	}
#endif
	if (reg == 0x100 || reg == 0x1fc) {
		return 1 | 2;
	} else if (reg >= 0x102 && reg < 0x108) {
		return 1 | 2;
	} else if (reg == 0x10c) {
		return 1 | 2;
	} else if (reg == 0x8e || reg == 0x90 || reg == 0x1e4) {
		return 1 | 2;
	} else if (reg >= 0x180 && reg < 0x180 + 32 * 2) {
		return 1 | 2;
	} else if (reg >= 0x140 && reg < 0x140 + 8 * 8) {
		if (reg & 4) {
			return 2 + 4;
		}
		return 1 | 2;
	} else if (reg >= 0x110 && reg < 0x110 + 8 * 2) {
		return 2;
	} else if (reg == 0x02c) {
		if (currprefs.cpu_memory_cycle_exact) {
			return 1 | 2;
		}
		return 1;
	} else if (reg == 0x1c4 || reg == 0x1c6) {
		return 1 | 2;
	} else if (reg == 0x098 || reg == 0x10e) {
		return 1 | 2;
	} else if (reg >= 0x38 && reg < 0x40) {
		return 2;
	}
	return 1;
}

static void custom_wput_pipelined(uaecptr pt, uae_u16 v)
{
	pipelined_write_addr = pt;
	pipelined_write_value = v;
}

static void custom_wput_copper(uaecptr pt, uaecptr addr, uae_u32 value, int noget)
{
#ifdef DEBUGGER
	value = debug_putpeekdma_chipset(0xdff000 + addr, value, MW_MASK_COPPER, 0x08c);
#endif
	copper_access = 1;
	int reg = addr & 0x1fe;
	int c = get_reg_chip(reg);
	if (c & 1) {
		//custom_wput_pipelined(addr, value);
		custom_wput_1(addr, value, 1 | 0x8000);
	}
	if (c & 2) {
		if (c & 4) {
			value <<= 16;
		}
		write_drga(reg, pt, value);
	}
	copper_access = 0;
}

static void bpl_autoscale(void)
{
	if (bplcon0_planes > 0 && linear_vpos > plflastline_total) {
		plflastline_total = linear_vpos;
	}
	if (bplcon0_planes > 0 && plffirstline_total > linear_vpos) {
		plffirstline_total = linear_vpos;
	}
}

static void bprun_start(int hpos)
{
	if (bplcon0_planes > 0 && ddffirstword_total > hpos) {
		ddffirstword_total = hpos + 4 + 8;
	}

	bpl_autoscale();
}

/*
	CPU write COPJMP wakeup sequence when copper is waiting:
	- Idle cycle (can be used by other DMA channel)
	- Read word from current copper pointer (next word after wait instruction) to 1FE
	  This cycle can conflict with blitter DMA.
	Normal copper cycles resume
	- Write word from new copper pointer to 8C
*/

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
		if (blit_busy(false)) {
			if (blitwait) {
				/* We need to wait for the blitter.  */
				cop_state.state = COP_bltwait;
				return -2;
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

static void compute_spcflag_copper(void)
{
	copper_enabled_thisline = 0;
	if (cop_state.strobe_next == COP_stop) {
		if (!is_copper_dma(true) || (cop_state.state == COP_stop || cop_state.state == COP_waitforever || cop_state.state == COP_bltwait || cop_state.state == COP_bltwait2) || custom_disabled)
			return;
		if (cop_state.state == COP_wait1 && is_copper_dma(false)) {
			int vp = vpos & (((cop_state.ir[1] >> 8) & 0x7F) | 0x80);
			if (vp < cop_state.vcmp) {
				return;
			}
		}
	}
	copper_enabled_thisline = 1;
}

void blitter_done_notify(void)
{
	if (cop_state.state != COP_bltwait) {
		return;
	}
	cop_state.state = COP_wait1;
	compute_spcflag_copper();
#ifdef DEBUGGER
	if (copper_enabled_thisline) {
		if (debug_dma) {
			record_dma_event(DMA_EVENT_COPPERWAKE);
		}
		if (debug_copper) {
			int hpos = current_hpos();
			record_copper_blitwait(cop_state.ip - 4, hpos, vpos);
		}
	}
#endif
}

static void cursorsprite(struct sprite *s)
{
	if (!dmaen(DMA_SPRITE) || first_planes_vpos == 0) {
		return;
	}
	sprite_0 = s->pt;
	sprite_0_height = s->vstop - s->vstart;
	sprite_0_colors[0] = 0;
	sprite_0_doubled = 0;
	int sprres = !aga_mode || !(bplcon3 & 0x80) ? 0 : 1;
	if (sprres == 0) {
		sprite_0_doubled = 1;
	}
	if (spr[0].dblscan) {
		sprite_0_height /= 2;
	}
	if (aga_mode) {
		int sbasecol = ((bplcon4 >> 4) & 15) << 4;
		sprite_0_colors[1] = agnus_colors.color_regs_aga[sbasecol + 1] & 0xffffff;
		sprite_0_colors[2] = agnus_colors.color_regs_aga[sbasecol + 2] & 0xffffff;
		sprite_0_colors[3] = agnus_colors.color_regs_aga[sbasecol + 3] & 0xffffff;
	} else {
		sprite_0_colors[1] = xcolors[agnus_colors.color_regs_ecs[17] & 0xfff];
		sprite_0_colors[2] = xcolors[agnus_colors.color_regs_ecs[18] & 0xfff];
		sprite_0_colors[3] = xcolors[agnus_colors.color_regs_ecs[19] & 0xfff];
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

static int calculate_linetype(int vp)
{
	int lineno = vp;
	nextline_how = nln_normal;
	if (doflickerfix_active()) {
		lineno *= 2;
		if (!lof_display) {
			lineno++;
			nextline_how = nln_lower;
		} else {
			nextline_how = nln_upper;
		}
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
	if (currprefs.gfx_vresolution && interlace_seen > 0 && currprefs.gfx_iscanlines == 0) {
		// if non-fields interlace mode: render only complete frames
		if (!lof_display) {
			return true;
		}
	}
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
		if (t > legacy_avg) {
			legacy_avg = t;
		}
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
		if (t > vsynctimebase * 2 / 3) {
			t = vsynctimebase * 2 / 3;
		}

		if (currprefs.m68k_speed < 0) {
			vsynctimeperline = (vsynctimebase - t) / (maxvpos_display + 1);
		} else {
			vsynctimeperline = (vsynctimebase - t) / 3;
		}

		if (vsynctimeperline < 1) {
			vsynctimeperline = 1;
		}

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
				rtg_vsynccheck ();
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
				rtg_vsynccheck();
				maybe_process_pull_audio();
				if (cpu_sleep_millis(1) < 0)
					break;
			}

			while (rpt_vsync(clockadjust) < 0) {
				rtg_vsynccheck();
#if 0
				if (audio_is_pull_event()) {
					maybe_process_pull_audio();
					break;
				}
#endif
			}

#if 0
			static evt_t ttt;
			write_log("%lld\n", read_processor_time() - ttt);
			ttt = read_processor_time();
#endif
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
		gui_data.lace = interlace_seen != 0;
		if (!interlace_seen) {
			gui_data.lines = linear_vpos_prev[0];
		} else {
			gui_data.lines = linear_vpos_prev[0] + linear_vpos_prev[1];
		}
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
		vsync_handle_redraw(lof_store, bplcon0, bplcon3, isvsync_chipset() >= 0, initial_frame);
		initial_frame = false;
		vsync_rendered = true;
		end = read_processor_time();
		frameskiptime += end - start;
	}

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

	if (vblank_hz_mult > 0) {
		vblank_hz_state ^= 1;
	} else {
		vblank_hz_state = 1;
	}

	nextline_how = nln_normal;
}

static bool vsync_display_rendered;

static void vsync_display_render(void)
{
	if (!vsync_display_rendered) {
		vsyncmintimepre = read_processor_time();

		if (!custom_disabled) {
			draw_denise_vsync_queue(display_redraw);
			display_redraw = false;
		}

		draw_denise_line_queue_flush();
		end_draw_denise();
		vsync_handler_render();
		if (!custom_disabled) {
			start_draw_denise();
		}
		vsync_display_rendered = true;
	}
}

static void vsync_check_vsyncmode(void)
{
	if (linear_vpos_prev[2] == linear_vpos_prev[0] || linear_vpos_prev[1] == linear_vpos_prev[0]) {
		if (linear_hpos_prev[2] == linear_hpos_prev[0] || linear_hpos_prev[1] == linear_hpos_prev[0]) {
			if (linear_hpos_prev[2] == linear_hpos_prev[0] || linear_hpos_prev[1] == linear_hpos_prev[0]) {
				if (linear_hpos_prev[2] == linear_hpos_prev[0] || linear_hpos_prev[1] == linear_hpos_prev[0]) {
					int hp = linear_hpos_prev[0] > linear_hpos_prev[2] ? linear_hpos_prev[0] : linear_hpos_prev[2];
					int vp = linear_vpos_prev[0] > linear_vpos_prev[2] ? linear_vpos_prev[0] : linear_vpos_prev[2];
					int ydiff = (prevlofs[0] != prevlofs[1] && prevlofs[0] == prevlofs[2]) ? 2 : 1;
					if (abs(hp - current_linear_hpos_temp) >= 2 || abs(vp - current_linear_vpos_temp) >= ydiff) {
						current_linear_hpos_temp = hp;
						current_linear_vpos_temp = vp;
						current_linear_temp_change = 2;
					}
				}
			}
		}
	}

	if (current_linear_temp_change > 0) {
		current_linear_temp_change--;
		if (current_linear_temp_change == 0) {
			if (current_linear_hpos != current_linear_hpos_temp ||
				current_linear_vpos != current_linear_vpos_temp) {
				current_linear_hpos = current_linear_hpos_temp;
				current_linear_vpos = current_linear_vpos_temp;
				current_linear_hpos_short = current_linear_hpos - maxhpos_lol;
				current_linear_vpos_nom = current_linear_vpos - lof_store;
				init_beamcon0();
				compute_framesync();
				devices_syncchange();
				display_redraw = true;
			}
		}
	}

	if (varsync_changed > 0) {
		varsync_changed--;
		if (varsync_changed == 0) {
			dumpsync();
		}
	}
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

static void check_interlace(void)
{
	struct amigadisplay *ad = &adisplays[0];
	int is = interlace_seen;
	int nis = 0;
	bool ld = lof_display != 0;
	bool changed = false;

	if (prevlofs[0] == prevlofs[2] && prevlofs[1] == ld && lof_display != prevlofs[0]) {
		is = 1;
	} else if (prevlofs[2] != lof_display && prevlofs[1] == lof_display && prevlofs[0] == ld) {
		nis = 1;
	} else {
		is = 0;
	}

	if (is != interlace_seen) {
		interlace_seen = is;
		init_hz();
		changed = true;
		display_redraw = true;
	} else if (nis) {
		interlace_seen = 0;
		init_hz();
	}

	if (changed) {
		if (currprefs.gf[GF_INTERLACE].enable && memcmp(&currprefs.gf[GF_NORMAL], &currprefs.gf[GF_INTERLACE], sizeof(struct gfx_filterdata))) {
			changed_prefs.gf[GF_NORMAL].changed = true;
			changed_prefs.gf[GF_INTERLACE].changed = true;
			if (ad->interlace_on != (interlace_seen != 0)) {
				ad->interlace_on = interlace_seen != 0;
				set_config_changed();
			}
		}
	}
	if (!ad->picasso_on) {
		if (ad->interlace_on && currprefs.gf[GF_INTERLACE].enable) {
			ad->gf_index = GF_INTERLACE;
		} else {
			ad->gf_index = GF_NORMAL;
		}
	} else {
		ad->gf_index = GF_RTG;
	}

	prevlofs[2] = prevlofs[1];
	prevlofs[1] = prevlofs[0];
	prevlofs[0] = lof_display;
}

static void check_no_signal(void)
{
	evt_t c = get_cycles();
	if (issyncstopped(bplcon0)) {
		nosignal_trigger = true;
	}
	// Missing vsync
	if (c > display_last_vsync + (CHIPSET_CLOCK_PAL / 10) * CYCLE_UNIT) {
		nosignal_trigger = true;
	}
	// Missing hsync
	if (c > display_last_hsync + (CHIPSET_CLOCK_PAL / 100) * CYCLE_UNIT) {
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
}

static void handle_nosignal(void)
{
	if (nosignal_status < 0) {
		nosignal_status = 0;
		resetfulllinestate();
	}
	if (nosignal_cnt) {
		nosignal_cnt--;
		if (nosignal_cnt == 0) {
			nosignal_status = -1;
			resetfulllinestate();
		}
	}
	if (nosignal_trigger) {
		struct amigadisplay *ad = &adisplays[0];
		nosignal_trigger = false;
		resetfulllinestate();
		if (!ad->specialmonitoron) {
			if (currprefs.gfx_monitorblankdelay > 0) {
				nosignal_status = 1;
				nosignal_cnt = (int)(currprefs.gfx_monitorblankdelay / (1000.0f / vblank_hz));
				if (nosignal_cnt <= 0) {
					nosignal_cnt = 1;
				}
			} else {
				nosignal_status = 2;
				nosignal_cnt = 2;
			}
		}
	}
}

static void virtual_vsync_check(void)
{
	check_display_mode_change();
	check_interlace();
	handle_nosignal();
	vsync_check_vsyncmode();
	color_table_changed = true;
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

	check_no_signal();

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
//		vpos_count = p96refresh_active;
//		vpos_count_diff = p96refresh_active;
//		vtotal = vpos_count;
	}
#endif

	devices_vsync_post();

	if (bogusframe > 0) {
		bogusframe--;
	}

	config_check_vsync();
	if (timehack_alive > 0) {
		timehack_alive--;
	}

	vsync_handle_check();

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

static void dmal_emu_disk(struct rgabuf *rga, int slot, bool w)
{
	uae_u16 dat = 0;
	// disk_fifostatus() needed in >100% disk speed modes
	if (w) {
		// write to disk
		if (disk_fifostatus() <= 0) {
			uaecptr pt = rga->pv;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(rga->reg, pt, DMARECORD_DISK, slot);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(pt, MW_MASK_DISK, rga->reg);
			}
#endif
			dat = chipmem_wget_indirect(pt);
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read_value(dat);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(dat);
			}
#endif
			regs.chipset_latch_rw = last_custom_value = dat;
			DSKDAT(dat);
		}
	} else {
		// read from disk
		if (disk_fifostatus() >= 0) {
			uaecptr pt = rga->pv;
			dat = DSKDATR(slot);
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_write(rga->reg, dat, pt, DMARECORD_DISK, slot);
			}
			if (memwatch_enabled) {
				debug_putpeekdma_chipram(pt, dat, MW_MASK_DISK, rga->reg);
			}
#endif
			chipmem_wput_indirect(pt, dat);
			regs.chipset_latch_rw = last_custom_value = dat;
		}
	}
}

static void dmal_emu_audio(struct rgabuf *rga, int nr)
{
	uaecptr pt = rga->pv;
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read(0xaa + nr * 16, pt, DMARECORD_AUDIO, nr);
	}
	if (memwatch_enabled) {
		debug_getpeekdma_chipram(pt, MW_MASK_AUDIO_0 << nr, rga->reg);
	}
#endif
	uae_u16 dat = chipmem_wget_indirect(pt);
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read_value(dat);
	}
	if (memwatch_enabled) {
		debug_getpeekdma_value(dat);
	}
#endif
	regs.chipset_latch_rw = last_custom_value = dat;
	AUDxDAT(nr, dat, pt);
}

static void lightpen_trigger_func(uae_u32 v)
{
	vpos_lpen = vpos;
	hpos_lpen = v;
	hhpos_lpen = HHPOSR();
	lightpen_triggered = 1;
}

static bool do_render_slice(int mode, int slicecnt, int lastline)
{
	draw_denise_line_queue_flush();
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

	//write_log("%4d %4d %4d %4d %4d %4d\n", diwfirstword_total, diwlastword_total, ddffirstword_total, ddflastword_total, plffirstline_total, plflastline_total);

	first_planes_vpos = 0;
	last_planes_vpos = 0;
	diwfirstword_total = 30000;
	diwlastword_total = 0;
	ddffirstword_total = 30000;
	ddflastword_total = 0;
	plflastline_total = 0;
	plffirstline_total = 30000;
	first_bplcon0 = 0;
	autoscale_bordercolors = 0;
}

static void hautoscale_check(void)
{
	int vp = linear_vpos;
	// border detection/autoscale
	if (bplcon0_planes > 0 && dmaen(DMA_BITPLANE) && !agnus_vb_active) {
		if (first_bplcon0 == 0) {
			first_bplcon0 = bplcon0;
		}
		if (vp > last_planes_vpos) {
			last_planes_vpos = vp;
		}
		if (vp >= minfirstline_linear && first_planes_vpos == 0) {
			first_planes_vpos = vp;
		} else if (vp > current_linear_vpos) {
			last_planes_vpos = current_linear_vpos;
		}
	}
}

// this finishes current line
static void hsync_handler_pre(bool onvsync)
{
	if (!custom_disabled) {

		// make sure decisions are done to end of scanline
		//finish_partial_decision(maxhpos);
		//clear_bitplane_pipeline(0);

		/* reset light pen latch */
		if (agnus_vb_active_end_line) {
			lightpen_triggered = 0;
			sprite_0 = 0;
		}

		if (!lightpen_triggered && (bplcon0 & 8)) {
			// lightpen always triggers at the beginning of the last line
			if (agnus_pvb_start_line) {
				vpos_lpen = vpos;
				hpos_lpen = 1;
				hhpos_lpen = HHPOSR();
				lightpen_triggered = 1;
			} else if (lightpen_enabled && !agnus_vb_active) {
				int lpnum = inputdevice_get_lightpen_id();
				if (lpnum < 0) {
					lpnum = 0;
				}
				if (lightpen_cx[lpnum] > 0 && lightpen_cy[lpnum] == vpos) {
					event2_newevent_xx(-1, lightpen_cx[lpnum] * CYCLE_UNIT, lightpen_cx[lpnum], lightpen_trigger_func);
				}
			}
		}

		if (GET_PLANES(bplcon0)) {
			notice_resolution_seen(GET_RES_AGNUS(bplcon0), interlace_seen != 0);
		}
	}

	devices_hsync();

	hsync_counter++;

	check_line_enabled();

#ifdef DEBUGGER
	debug_hsync();
#endif
}

// low latency vsync

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

static void cia_hsync_do(void)
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
		uae_s32 tick = read_system_time(); // milliseconds
		int ms = 1000 / (currprefs.cs_ciaatod == 2 ? 60 : 50);
		if (tick - oldtick > 2000 || tick - oldtick < -2000) {
			oldtick = tick - ms;
			write_log(_T("RESET\n"));
		}
		if (tick - oldtick >= ms) {
			CIA_vsync_posthandler(1);
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
}

// this prepares for new line
static void hsync_handler_post(bool onvsync)
{
	cia_hsync_do();


#if 0
	if (!custom_disabled) {
		if (!currprefs.blitter_cycle_exact && blt_info.blit_main && dmaen (DMA_BITPLANE) && vdiwstate == diw_states::DIW_waiting_stop) {
			blitter_slowdown(thisline_decision.plfleft, thisline_decision.plfright - (16 << fetchmode),
				cycle_diagram_total_cycles[fetchmode][GET_RES_AGNUS (bplcon0)][GET_PLANES_LIMIT (bplcon0)],
				cycle_diagram_free_cycles[fetchmode][GET_RES_AGNUS (bplcon0)][GET_PLANES_LIMIT (bplcon0)]);
		}
	}
#endif

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
		if (is_last_line()) {
			sleeps_remaining = (165 - currprefs.cpu_idle) / 6;
			if (sleeps_remaining < 0)
				sleeps_remaining = 0;
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
			int maxlc = 1;
			vsyncmintime += vsynctimeperline;
			linecounter++;
			events_reset_syncline();
			if (vsync_isdone(NULL) <= 0 && !currprefs.turbo_emulation && (linecounter & (maxlc - 1)) == 0) {
				if (vsyncmaxtime - vsyncmintime > 0) {
					frame_time_t rpt = read_processor_time();
					if (vsyncwaittime - vsyncmintime > 0) {
						/* Extra time left? Do some extra CPU emulation */
						if (vsyncmintime > rpt) {
							if (regs.stopped && currprefs.cpu_idle && sleeps_remaining > 0) {
								// STOP STATE: sleep.
								cpu_sleep_millis(1);
								sleeps_remaining--;
								maybe_process_pull_audio();
							} else {
								is_syncline = -11;
								/* limit extra time */
								is_syncline_end = rpt + vsynctimeperline * maxlc;
								linecounter = 0;
							}
						}
					}
					if (!isvsync()) {
						// extra cpu emulation time if previous 10 lines without extra time.
						if (!is_syncline && linecounter >= 10 && (!regs.stopped || !currprefs.cpu_idle)) {
							is_syncline = -10;
							is_syncline_end = rpt + vsynctimeperline;
							linecounter = 0;
						}
					}
				}
			}
			maybe_process_pull_audio();
		}

	} else if (!currprefs.cpu_thread) {

		// the rest
		static int linecounter;
		static int nextwaitvpos;
		int maxlc = 8, maxlcm = 4;
		linecounter++;
		if (linear_vpos == 0)
			nextwaitvpos = maxlc * 4;
		if (audio_is_pull() > 0 && !currprefs.turbo_emulation && (linecounter & (maxlc - 1)) == 0) {
			maybe_process_pull_audio();
			frame_time_t rpt = read_processor_time();
			while (audio_pull_buffer() > 1 && (!isvsync() || (vsync_isdone(NULL) <= 0 && vsyncmintime - (rpt + vsynctimebase / 10) > 0 && vsyncmintime - rpt < vsynctimebase))) {
				cpu_sleep_millis(1);
				maybe_process_pull_audio();
				rpt = read_processor_time();
			}
		}
		if (linear_vpos + maxlc * maxlcm < current_linear_vpos && linear_vpos >= nextwaitvpos && (audio_is_pull() <= 0 || (audio_is_pull() > 0 && audio_pull_buffer()))) {
			nextwaitvpos += maxlc * maxlcm;
			vsyncmintime += vsynctimeperline * maxlc * maxlcm;
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

}

static void vsync_start_check(void)
{
	if (displayreset_delayed) {
		if (displayreset_delayed & 1) {
			displayresetcnt++;
		}
		displayreset_delayed >>= 1;
	}
	if (lines_after_beamcon_change > 0) {
		lines_after_beamcon_change--;
	}
}

static bool uae_quit_check(void)
{
	if (quit_program < 0) {
#ifdef SAVESTATE
		if (!savestate_state && quit_program == -UAE_QUIT && currprefs.quitstatefile[0]) {
			savestate_initsave(currprefs.quitstatefile, 1, 1, true);
			save_state(currprefs.quitstatefile, _T(""));
		}
#endif
		quit_program = -quit_program;
		set_inhibit_frame(0, IHF_QUIT_PROGRAM);
		set_special(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
		return true;
	}
	if (quit_program > 0) {
		struct amigadisplay *ad = &adisplays[0];
		/* prevent possible infinite loop at wait_cycles().. */
		ad->framecnt = 0;
		return true;
	}
	return false;
}

// executed at start of scanline
static void hsync_handler(bool vs)
{
	display_last_hsync = get_cycles();

	hsync_handler_pre(vs);
	if (vs) {
		devices_vsync_pre();
		if (savestate_check()) {
			uae_reset(0, 0);
			return;
		}
		if (uae_quit_check()) {
			return;
		}
	}
	if (vpos == vsync_startline + 1 && !maxvpos_display_vsync_next) {
		inputdevice_read_msg(true);
		vsync_display_render();
		vsync_display_rendered = false;
		if (currprefs.cs_hvcsync == 0) {
			if (beamcon0 & (BEAMCON0_VARVSYEN | BEAMCON0_VARCSYEN)) {
				lof_display = lof_pdetect;
			} else {
				lof_display = lof_detect;
			}
		} else if (currprefs.cs_hvcsync == 1) {
			if (beamcon0 & BEAMCON0_VARCSYEN) {
				lof_display = lof_pdetect;
			} else {
				lof_display = lof_detect;
			}
		} else if (currprefs.cs_hvcsync == 2) {
			if (beamcon0 & BEAMCON0_VARVSYEN) {
				lof_display = lof_pdetect;
			} else {
				lof_display = lof_detect;
			}
		}
		reset_autoscale();
		display_vsync_counter++;
		maxvpos_display_vsync_next = true;
		display_hsync_counter = 0;
		display_last_vsync = get_cycles();
		vsync_start_check();
	} else if (vpos != vsync_startline + 1 && maxvpos_display_vsync_next) {
		// protect against weird VPOSW writes causing continuous vblanks
		maxvpos_display_vsync_next = false;
		vsync_start_check();
	} else {
		display_hsync_counter++;
		if (display_hsync_counter > maxvpos) {
			display_hsync_counter = 0;
			inputdevice_read_msg(true);
			vsync_display_render();
			vsync_display_rendered = false;
			vsync_start_check();
		}

	}
	hsync_handler_post(vs);
}

static void audio_evhandler2(void)
{
	audio_evhandler();
}

void init_eventtab(void)
{
	if (!savestate_state) {
		clear_events();
	}

	eventtab[ev_cia].handler = CIA_handler;
	eventtab[ev_misc].handler = MISC_handler;
	eventtab[ev_audio].handler = audio_evhandler2;

	eventtab2[ev2_blitter].handler = blitter_handler;

	events_schedule();
}

void custom_prepare(void)
{
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
	custom_end_drawing();

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

	for(int i = 0; i < DENISE_RGA_SLOT_TOTAL; i++) {
		struct denise_rga *r = &rga_denise[i];
		memset(r, 0, sizeof(struct denise_rga));
	}
	preg.p = NULL;
	for (int i = 0 ; i < MAX_PIPELINE_REG; i++) {
		struct pipeline_func *p = &pfunc[i];
		memset(p, 0, sizeof(struct pipeline_func));
	}
	rga_denise_cycle = 0;
	rga_denise_cycle_start = 0;
	rga_denise_cycle_count_end = 0;
	rga_denise_cycle_count_start = 0;
	rga_denise_cycle_line = 1;

	vsync_startline = 3;
	copper_dma_change_cycle = 0;
	blitter_dma_change_cycle = 0;
	sprite_dma_change_cycle_on = 0;

	pipelined_write_addr = 0x1fe;
	prev_strobe = 0x3c;
	dmal_next = false;
	syncs_stopped = false;

	bool ntsc = currprefs.ntscmode;

	lightpen_active = 0;
	lightpen_triggered = 0;
	lightpen_cx[0] = lightpen_cy[0] = -1;
	lightpen_cx[1] = lightpen_cy[1] = -1;
	lightpen_x[0] = -1;
	lightpen_y[0] = -1;
	lightpen_x[1] = -1;
	lightpen_y[1] = -1;
	memset(custom_storage, 0, sizeof(custom_storage));
	exthblank = false;
	display_reset = 1;
	vt_old = 0;
	ht_old = 0;
	maxvpos_display_vsync_next = false;
	programmed_register_accessed_h = false;
	programmed_register_accessed_v = false;
	aexthblanken = false;
	denise_reset(false);

	agnus_hpos_next = -1;
	agnus_vpos_next = -1;
	agnus_pos_change = -2;
	hsync_counter = 0;
	vsync_counter = 0;
	display_vsync_counter = 0;
	display_hsync_counter = 0;
	display_last_hsync = get_cycles();
	display_last_vsync = get_cycles();
	next_lineno = 0;

	agnus_hpos = 0;
	agnus_hpos_prev = 0;
	vpos_prev = 0;
	vpos_lpen = -1;

	uhres_state = 0;
	uhres_bpl = false;
	uhres_spr = false;

	dmal_shifter = 0;
	dmal = 0;

	nosignal_cnt = 0;
	nosignal_status = 0;
	nosignal_trigger = false;

	irq_forced_delay = 0;
	irq_forced = 0;

	agnus_phsync = agnus_phblank = false;
	agnus_pvsync = agnus_pcsync = agnus_csync = false;
	agnus_vb = agnus_pvb = 0;
	agnus_vb_active = false;
	agnus_equzone = false;
	agnus_hsync = agnus_vsync = agnus_ve = agnus_p_ve = false;
	agnus_equdis = false;
	agnus_bsvb = true;

	if (hardreset || savestate_state) {
		maxhpos = ntsc ? MAXHPOS_NTSC : MAXHPOS_PAL;
		maxhpos_short = ntsc ? MAXHPOS_NTSC : MAXHPOS_PAL;
		maxvpos = ntsc ? MAXVPOS_NTSC : MAXVPOS_PAL;
		maxvpos_nom = ntsc ? MAXVPOS_NTSC : MAXVPOS_PAL;
		maxvpos_display = ntsc ? MAXVPOS_NTSC : MAXVPOS_PAL;
		maxhpos_lol = false;
	}

	if (!savestate_state) {
		rga_slot_first_offset = 0;
		rga_slot_in_offset = 1;
		rga_slot_out_offset = 2;
		cia_hsync = 0;
		extra_cycle = 0;
		currprefs.chipset_mask = changed_prefs.chipset_mask;
		vdiwstate = diw_states::DIW_waiting_start;
		blitter_reset();
		denise_reset(true);

		lol = false;

		for(int i = 0; i < RGA_SLOT_TOTAL + 1; i++) {
			struct rgabuf *r = &rga_pipe[i];
			memset(r, 0, sizeof(struct rgabuf));
			r->reg = 0x1fe;
		}

		if (hardreset) {

			vtotal = MAXVPOS_LINES_ECS - 1;
			htotal = MAXHPOS_ROWS - 1;
			hbstrt = 0xffff;
			hbstop = 0xffff;
			hbstrt_cck = 0xff;
			hbstop_cck = 0xff;
			hsstrt = 0; // jtxrules / illusion assumes HSSTRT==0!
			hsstop = 0;
			vbstrt = 0xffff;
			vbstop = 0xffff;
			vsstrt = 0xffff;
			vsstop = 0xffff;
			hcenter = 0xffff;
			bplhstop = 0xffff;
			bplhstrt = 0xffff;
			sprhstop = 0xffff;
			sprhstrt = 0xffff;

			for (int i = 0; i < 32; i++) {
				uae_u16 c;
				if (i == 0) {
					c = ((ecs_denise && !aga_mode) || denisea1000) ? 0xfff : 0x000;
				} else {
					c |= uaerand();
					c |= uaerand();
				}
				c &= 0xfff;
				agnus_colors.color_regs_ecs[i] = denise_colors.color_regs_ecs[i] = c;
			}
			for (int i = 0; i < 256; i++) {
				uae_u32 c = 0;
				if (i > 0) {
					c |= uaerand();
					c |= uaerand();
				}
				c &= 0xffffff;
				denise_colors.color_regs_aga[i] = c;
				agnus_colors.color_regs_aga[i] = c;
			}
			if (!aga_mode) {
				for (int i = 0; i < 32; i++) {
					agnus_colors.acolors[i] = denise_colors.acolors[i] = getxcolor(denise_colors.color_regs_ecs[i]);
				}
#ifdef AGA
			} else {
				for (int i = 0; i < 256; i++) {
					agnus_colors.acolors[i] = denise_colors.acolors[i] = getxcolor(denise_colors.color_regs_aga[i]);
				}
#endif
			}

			lof_store = lof_display = 0;
		}

		clxdat = 0;

		/* Clear the armed flags of all sprites.  */
		memset(spr, 0, sizeof spr);

		dmacon = 0;
		intreq = intreq2 = 0;
		intena = intena2 = 0;

		copcon = 0;
		DSKLEN(0, 0);

		bplcon0 = 0;
		bplcon4 = 0x0011; /* Get AGA chipset into ECS compatibility mode */
		bplcon3 = 0x0C00;

		diwhigh = 0;
		diwhigh_written = 0;
		hdiwstate = diw_states::DIW_waiting_start; // this does not reset at vblank

		refptr = 0;
		if (aga_mode) {
			refptr = 0x1ffffe;
		}
		refptr_p = refptr;
		FMODE(0);
		CLXCON(0);
		CLXCON2(0);
		setup_fmodes(bplcon0);
		beamcon0 = new_beamcon0 = beamcon0_saved = currprefs.ntscmode ? 0x00 : BEAMCON0_PAL;
		blt_info.blit_main = 0;
		blt_info.blit_pending = 0;
		blt_info.blit_interrupt = 1;
		blt_info.blit_queued = 0;
		//init_sprites();

		maxhpos = ntsc ? MAXHPOS_NTSC : MAXHPOS_PAL;
		maxhpos_short = maxhpos;
		maxhpos_lol = lol;
		updateextblk();

		bplcon0_saved = bplcon0;
		bplcon1_saved = bplcon1;
		bplcon2_saved = bplcon2;
		bplcon3_saved = bplcon3;
		bplcon4_saved = bplcon4;
		ddfstrt_saved = ddfstrt;
		ddfstop_saved = ddfstop;
		diwhigh_saved = diwhigh;
		fmode_saved = fmode;
		beamcon0_saved = new_beamcon0;

		if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
			// fake draco interrupts
			INTENA(0x8000 | 0x4000 | 0x1000 | 0x2000 | 0x0080 | 0x0010 | 0x0008 | 0x0001);
		}
	}
#ifdef WITH_SPECIALMONITORS
	specialmonitor_reset();
#endif
	update_mirrors();

	unset_special(~(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE | SPCFLAG_CHECK));

	inputdevice_reset();
	timehack_alive = 0;

	//vdiw_change(0);
	check_harddis();

	init_hz_reset();

	audio_reset();
	cop_state.strobe_next = COP_stop;
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

	//init_hardware_frame();
	drawing_init();

	//reset_decisions_scanline_start();
	//reset_decisions_hsync_start();

	bogusframe = 1;
	vsync_rendered = false;
	frame_shown = false;
	frame_rendered = false;

	if (isrestore()) {
		uae_u16 v;
		uae_u32 vv;

		// preset position to last end of last line
		// so that first VB and COPJMP happens correctly
		vpos = maxvpos + lof_store - 1;
		vpos_prev = vpos - 1;
		agnus_hpos = 0;
		agnus_vb = agnusa1000 ? false : true;

		audio_update_adkmasks();
		INTENA(0);
		INTREQ(0);
		v = bplcon0;
		BPLCON0(0);
		BPLCON0(v);
		FMODE(fmode);
		setup_fmodes(bplcon0);
		if (!aga_mode) {
			for(int i = 0 ; i < 32 ; i++)  {
				vv = denise_colors.color_regs_ecs[i];
				denise_colors.color_regs_ecs[i] = -1;
				denise_colors.color_regs_ecs[i] = vv;
				agnus_colors.acolors[i] = denise_colors.acolors[i] = xcolors[vv];
			}
#ifdef AGA
		} else {
			for(int i = 0 ; i < 256 ; i++)  {
				vv = denise_colors.color_regs_aga[i];
				denise_colors.color_regs_aga[i] = -1;
				denise_colors.color_regs_aga[i] = vv;
				if (i < 32) {
					saved_color_regs_aga[i] = vv;
				}
				agnus_colors.acolors[i] = denise_colors.acolors[i] = CONVERT_RGB(vv);
			}
#endif
		}
		CLXCON(clxcon);
		CLXCON2(clxcon2);
#ifdef SERIAL_PORT
		v = serper;
		serper = 0;
		SERPER(v);
#endif
		if (beamcon0 & (BEAMCON0_VARVBEN | BEAMCON0_CSCBEN | BEAMCON0_VARVSYEN | BEAMCON0_VARHSYEN | BEAMCON0_VARCSYEN | BEAMCON0_VARBEAMEN | BEAMCON0_DUAL | BEAMCON0_BLANKEN)) {
			programmed_register_accessed_h = true;
			programmed_register_accessed_v = true;
			dumpsync();
		}

		for (int i = 0; i < 8; i++) {
			SPRxCTLPOS(i);
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

	sprite_width = GET_SPRITEWIDTH(fmode);
	setup_fmodes(bplcon0);
	setmaxhpos();
	resetfulllinestate();
	updateprghpostable();
	start_draw_denise();

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
		DMACONR(),
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

	build_blitfilltable();

	drawing_init();

	update_mirrors();
	create_cycle_diagram_table();

	return 1;
}

/* Custom chip memory bank */

static uae_u32 REGPARAM3 custom_lget(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_wget(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_bget(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_lgeti(uaecptr) REGPARAM;
static uae_u32 REGPARAM3 custom_wgeti(uaecptr) REGPARAM;
static void REGPARAM3 custom_lput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 custom_wput(uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 custom_bput(uaecptr, uae_u32) REGPARAM;

addrbank custom_bank = {
	custom_lget, custom_wget, custom_bget,
	custom_lput, custom_wput, custom_bput,
	default_xlate, default_check, NULL, NULL, _T("Custom chipset"),
	custom_lgeti, custom_wgeti,
	ABFLAG_IO, S_READ, S_WRITE, NULL, 0x1ff, 0xdff000
};

static uae_u32 REGPARAM2 custom_wgeti(uaecptr addr)
{
	if (currprefs.cpu_model >= 68020 && !currprefs.cpu_compatible)
		return dummy_wgeti(addr);
	return custom_wget(addr);
}
static uae_u32 REGPARAM2 custom_lgeti(uaecptr addr)
{
	if (currprefs.cpu_model >= 68020 && !currprefs.cpu_compatible)
		return dummy_lgeti(addr);
	return custom_lget(addr);
}

static uae_u32 REGPARAM2 custom_wget_1(int hpos, uaecptr addr, int noput, bool isbyte)
{
	uae_u16 v = regs.chipset_latch_rw;
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
	case 0x002: v = DMACONR(); break;
	case 0x004: v = VPOSR(); break;
	case 0x006: v = VHPOSR(); break;

	case 0x00A: v = JOY0DAT(); break;
	case 0x00C: v = JOY1DAT(); break;
	case 0x00E: v = CLXDAT(); break;
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

	case 0x088: COPJMP(1, 0); break;
	case 0x08a: COPJMP(2, 0); break;

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
		if (!noput) {
			int r;
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
#ifdef DEBUGGER
			debug_wputpeek(0xdff000 + addr, l);
#endif
			r = custom_wput_1(addr, l, 1);
			v = last_custom_value;

			// Don't do this:
			// 000400c8 08f9 000a 00df f096      bset.b #$000a,$00dff096

			evt_t cycs = get_cycles();
			if (cycs - CYCLE_UNIT == last_rga_cycle) {
				v = regs.chipset_latch_rw;
			} else {
				if (aga_mode) {
					v = regs.chipset_latch_rw >> ((addr & 2) ? 0 : 16);
				} else {
					v = 0xffff;
				}
			}

#if 0
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
#endif
#if CUSTOM_DEBUG > 0
			write_log(_T("%08X read = %04X. Value written=%04X PC=%08x\n"), 0xdff000 | addr, v, l, M68K_GETPC);
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

	v = custom_wget_1(hpos, addr, 0, byte);
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

static uae_u32 REGPARAM2 custom_lget(uaecptr addr)
{
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0)
		return dummy_get(addr, 4, false, 0);
	return ((uae_u32)custom_wget(addr) << 16) | custom_wget(addr + 2);
}

static int custom_wput_agnus(int addr, uae_u32 value, int noget)
{
	int hpos = agnus_hpos;

	switch (addr) {
	case 0x00E: CLXDAT(); break;

	case 0x020: DSKPTH(value); break;
	case 0x022: DSKPTL(value); break;
	case 0x024: DSKLEN(value, hpos); break;
	case 0x026: /* DSKDAT(value). Writing to DMA write registers won't do anything */; break;
	case 0x028: REFPTR(hpos, value); break;
	case 0x02A: VPOSW(value); break;
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

	case 0x040: BLTCON0(value); break;
	case 0x042: BLTCON1(value); break;

	case 0x044: BLTAFWM(value); break;
	case 0x046: BLTALWM(value); break;

	case 0x050: BLTAPTH(value); break;
	case 0x052: BLTAPTL(value); break;
	case 0x04C: BLTBPTH(value); break;
	case 0x04E: BLTBPTL(value); break;
	case 0x048: BLTCPTH(value); break;
	case 0x04A: BLTCPTL(value); break;
	case 0x054: BLTDPTH(value); break;
	case 0x056: BLTDPTL(value); break;

	case 0x058: BLTSIZE(hpos, value); break;

	case 0x064: BLTAMOD(value); break;
	case 0x062: BLTBMOD(value); break;
	case 0x060: BLTCMOD(value); break;
	case 0x066: BLTDMOD(value); break;

	case 0x070: BLTCDAT(value); break;
	case 0x072: BLTBDAT(value); break;
	case 0x074: BLTADAT(value); break;

	case 0x07E: DSKSYNC(hpos, value); break;

	case 0x080: COP1LCH(value); break;
	case 0x082: COP1LCL(value); break;
	case 0x084: COP2LCH(value); break;
	case 0x086: COP2LCL(value); break;

	case 0x088: COPJMP(1, 0); break;
	case 0x08A: COPJMP(2, 0); break;

	case 0x08E: DIWSTRT(value); break;
	case 0x090: DIWSTOP(value); break;
	case 0x092: DDFSTRT(value); break;
	case 0x094: DDFSTOP(value); break;

	case 0x096: DMACON(hpos, value); break;
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

	case 0x0E0: BPLxPTH(value, 0); break;
	case 0x0E2: BPLxPTL(value, 0); break;
	case 0x0E4: BPLxPTH(value, 1); break;
	case 0x0E6: BPLxPTL(value, 1); break;
	case 0x0E8: BPLxPTH(value, 2); break;
	case 0x0EA: BPLxPTL(value, 2); break;
	case 0x0EC: BPLxPTH(value, 3); break;
	case 0x0EE: BPLxPTL(value, 3); break;
	case 0x0F0: BPLxPTH(value, 4); break;
	case 0x0F2: BPLxPTL(value, 4); break;
	case 0x0F4: BPLxPTH(value, 5); break;
	case 0x0F6: BPLxPTL(value, 5); break;
	case 0x0F8: BPLxPTH(value, 6); break;
	case 0x0FA: BPLxPTL(value, 6); break;
	case 0x0FC: BPLxPTH(value, 7); break;
	case 0x0FE: BPLxPTL(value, 7); break;

	case 0x100: BPLCON0(value); break;
	case 0x102: BPLCON1(value); break;
	case 0x104: BPLCON2(value); break;
	case 0x106: BPLCON3(value); break;
#ifdef AGA
	case 0x10c: BPLCON4(value); break;
#endif

	case 0x108: BPL1MOD(value); break;
	case 0x10A: BPL2MOD(value); break;

#if 0
	case 0x110: BPLxDAT(0, value); break;
	case 0x112: BPLxDAT(1, value); break;
	case 0x114: BPLxDAT(2, value); break;
	case 0x116: BPLxDAT(3, value); break;
	case 0x118: BPLxDAT(4, value); break;
	case 0x11A: BPLxDAT(5, value); break;
	case 0x11C: BPLxDAT(6, value); break;
	case 0x11E: BPLxDAT(7, value); break;
#endif

	case 0x180: case 0x182: case 0x184: case 0x186: case 0x188: case 0x18A:
	case 0x18C: case 0x18E: case 0x190: case 0x192: case 0x194: case 0x196:
	case 0x198: case 0x19A: case 0x19C: case 0x19E: case 0x1A0: case 0x1A2:
	case 0x1A4: case 0x1A6: case 0x1A8: case 0x1AA: case 0x1AC: case 0x1AE:
	case 0x1B0: case 0x1B2: case 0x1B4: case 0x1B6: case 0x1B8: case 0x1BA:
	case 0x1BC: case 0x1BE:
		COLOR_WRITE(value & 0x8FFF, (addr & 0x3E) / 2);
		break;

	case 0x120: case 0x124: case 0x128: case 0x12C:
	case 0x130: case 0x134: case 0x138: case 0x13C:
		SPRxPTH(value, (addr - 0x120) / 4);
		break;
	case 0x122: case 0x126: case 0x12A: case 0x12E:
	case 0x132: case 0x136: case 0x13A: case 0x13E:
		SPRxPTL(value, (addr - 0x122) / 4);
		break;
	case 0x140: case 0x148: case 0x150: case 0x158:
	case 0x160: case 0x168: case 0x170: case 0x178:
		SPRxPOS(value, (addr - 0x140) / 8);
		break;
	case 0x142: case 0x14A: case 0x152: case 0x15A:
	case 0x162: case 0x16A: case 0x172: case 0x17A:
		SPRxCTL(value, (addr - 0x142) / 8);
		break;
#if 0
	case 0x144: case 0x14C: case 0x154: case 0x15C:
	case 0x164: case 0x16C: case 0x174: case 0x17C:
		SPRxDATA(value, (addr - 0x144) / 8);
		break;
	case 0x146: case 0x14E: case 0x156: case 0x15E:
	case 0x166: case 0x16E: case 0x176: case 0x17E:
		SPRxDATB(value, (addr - 0x146) / 8);
		break;
#endif
	case 0x36: JOYTEST(value); break;
	case 0x5A: BLTCON0L(value); break;
	case 0x5C: BLTSIZV(hpos, value); break;
	case 0x5E: BLTSIZH(hpos, value); break;
	case 0x1E4: DIWHIGH(value); break;

	case 0x098: CLXCON(value); break;
	case 0x10e: CLXCON2(value); break;

	case 0x1DC: BEAMCON0(value); break;
	case 0x1C0:
		if (htotal != value) {
			htotal = value & (MAXHPOS_ROWS - 1);
			varsync(addr, 1, -1);
			programmed_register_accessed_h = true;
		}
		break;
	case 0x1C2:
		if (hsstop != value) {
			prg_hpos_table[hsstop & 0xff] &= ~2;
			hsstop = value & (MAXHPOS_ROWS - 1);
			prg_hpos_table[hsstop & 0xff] |= 2;
			varsync(addr, 1, -1);
			programmed_register_accessed_h = true;
		}
		break;
	case 0x1C4:
		if (hbstrt != value) {
			prg_hpos_table[hbstrt & 0xff] &= ~4;
			hbstrt = value & 0x7ff;
			prg_hpos_table[hbstrt & 0xff] |= 4;
			hbstrt_cck = hbstrt & 0xff;
			varsync(addr, 0, -1);
			programmed_register_accessed_h = true;
		}
		break;
	case 0x1C6:
		if (hbstop != value) {
			prg_hpos_table[hbstop & 0xff] &= ~8;
			hbstop = value & 0x7ff;
			prg_hpos_table[hbstop & 0xff] |= 8;
			hbstop_cck = hbstop & 0xff;
			varsync(addr, 0, -1);
			programmed_register_accessed_h = true;
		}
		break;
	case 0x1C8:
		if (vtotal != value) {
			vtotal = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 1, -1);
			programmed_register_accessed_v = true;
		}
		break;
	case 0x1CA:
		if (vsstop != value) {
			vsstop = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 1, -1);
			programmed_register_accessed_v = true;
		}
		break;
	case 0x1CC:
		if (vbstrt != value) {
			uae_u16 old = vbstrt;
			vbstrt = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 0, old);
			programmed_register_accessed_v = true;
		}
		break;
	case 0x1CE:
		if (vbstop != value) {
			uae_u16 old = vbstop;
			vbstop = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 0, old);
			programmed_register_accessed_v = true;
		}
		break;
	case 0x1DE:
		if (hsstrt != value) {
			prg_hpos_table[hsstrt & 0xff] &= ~1;
			hsstrt = value & (MAXHPOS_ROWS - 1);
			prg_hpos_table[hsstrt & 0xff] |= 1;
			varsync(addr, 1, -1);
			programmed_register_accessed_h = true;
		}
		break;
	case 0x1E0:
		if (vsstrt != value) {
			vsstrt = value & (MAXVPOS_LINES_ECS - 1);
			varsync(addr, 1, -1);
			programmed_register_accessed_v = true;
		}
		break;
	case 0x1E2:
		if (hcenter != value) {
			prg_hpos_table[hcenter & 0xff] &= ~16;
			hcenter = value & (MAXHPOS_ROWS - 1);
			prg_hpos_table[hcenter & 0xff] |= 16;
			varsync(addr, 0, -1);
			programmed_register_accessed_h = true;
		}
		break;

	case 0x1D0: SPRHSTRT(value); break;
	case 0x1D2: SPRHSTOP(value); break;
	case 0x1D4: BPLHSTRT(value); break;
	case 0x1D6: BPLHSTOP(value); break;
	case 0x1D8: HHPOS(value); break;
	case 0x1E6: BPLHMOD(value); break;
	case 0x1E8: SPRHPTH(value); break;
	case 0x1EA: SPRHPTL(value); break;
	case 0x1EC: BPLHPTH(value); break;
	case 0x1EE: BPLHPTL(value); break;

#ifdef AGA
	case 0x1FC: FMODE(value); break;
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

static int REGPARAM2 custom_wput_1(uaecptr addr, uae_u32 value, int noget)
{
	int hpos = agnus_hpos;
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
	int c = get_reg_chip(addr);
	int ret = 0;
	if (c & 1) {
		ret = custom_wput_agnus(addr, value, noget);
	}
	if (!(noget & 0x8000) && (c & 2)) {
		uae_u32 v = value;
		if (c & 4) {
			v <<= 16;
		}
		if (!currprefs.cpu_memory_cycle_exact || custom_fastmode > 0) {
			// fast CPU RGA pipeline, allow multiple register writes per CCK
			if (!denise_update_reg_queued(addr, v, rga_denise_cycle_line)) {
				// if full: direct write
				denise_update_reg_queue(addr, value, rga_denise_cycle_line);
			}
		} else {
			write_drga(addr, NULL, v);
		}
	}
	return ret;
}

static void REGPARAM2 custom_wput(uaecptr addr, uae_u32 value)
{
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 2, value);
		return;
	}
#if CUSTOM_DEBUG > 2
	write_log (_T("%d:%d:wput: %04X %04X pc=%p\n"), hpos, vpos, addr & 0x01fe, value & 0xffff, m68k_getpc ());
#endif
	if (addr & 1) {
#ifdef DEBUGGER
		debug_invalid_reg(addr, -2, value);
#endif
		addr &= ~1;
		custom_wput_1(addr, (value >> 8) | (value & 0xff00), 0);
		custom_wput_1(addr + 2, (value << 8) | (value & 0x00ff), 0);
		return;
	}
	custom_wput_1(addr, value, 0);
}

static void REGPARAM2 custom_bput(uaecptr addr, uae_u32 value)
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

static void REGPARAM2 custom_lput(uaecptr addr, uae_u32 value)
{
	if ((addr & 0xffff) < 0x8000 && currprefs.cs_fatgaryrev >= 0) {
		dummy_put(addr, 4, value);
		return;
	}
	custom_wput(addr & 0xfffe, value >> 16);
	custom_wput((addr + 2) & 0xfffe, (uae_u16)value);
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
	denise_reset(true);
	rga_slot_first_offset = 0;
	rga_slot_in_offset = 1;
	rga_slot_out_offset = 2;
}

#define RB restore_u8()
#define SRB restore_s8()
#define RBB (restore_u8() != 0)
#define RW restore_u16()
#define RL restore_u32()

uae_u8 *restore_custom(uae_u8 *src)
{
	uae_u16 dsklen, dskbytr, tmp;
	int dskpt;
	int i;

	audio_reset();

	custom_state_flags = RL;
	changed_prefs.chipset_mask = currprefs.chipset_mask = custom_state_flags & CSMASK_MASK;
	update_mirrors();
	blt_info.bltddat = RW;	/* 000 BLTDDAT */
	RW;						/* 002 DMACONR */
	RW;						/* 004 VPOSR */
	RW;						/* 006 VHPOSR */
	RW;						/* 008 DSKDATR (dummy register) */
	JOYSET(0, RW);			/* 00A JOY0DAT */
	JOYSET(1, RW);			/* 00C JOY1DAT */
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
	i = RW; lof_store = (i & 0x8000) != 0; lol = (i & 0x0080) != 0; /* 02A VPOSW */
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
	BLTCON0(RW);			/* 040 BLTCON0 */
	BLTCON1(RW);			/* 042 BLTCON1 */
	BLTAFWM(RW);			/* 044 BLTAFWM */
	BLTALWM(RW);			/* 046 BLTALWM */
	blt_info.bltcpt = RL;	/* 048-04B BLTCPT */	
	blt_info.bltbpt = RL;	/* 04C-04F BLTBPT */
	blt_info.bltapt = RL;	/* 050-053 BLTAPT */
	blt_info.bltdpt = RL;	/* 054-057 BLTDPT */
	RW;						/* 058 BLTSIZE */
	RW;						/* 05A BLTCON0L */
	blt_info.vblitsize = RW;/* 05C BLTSIZV */
	blt_info.hblitsize = RW;/* 05E BLTSIZH */
	blt_info.bltcmod_next = blt_info.bltcmod = RW;	/* 060 BLTCMOD */
	blt_info.bltbmod_next = blt_info.bltbmod = RW;	/* 062 BLTBMOD */
	blt_info.bltamod_next = blt_info.bltamod = RW;	/* 064 BLTAMOD */
	blt_info.bltdmod_next = blt_info.bltdmod = RW;	/* 066 BLTDMOD */
	RW;						/* 068 ? */
	RW;						/* 06A ? */
	RW;						/* 06C ? */
	RW;						/* 06E ? */
	BLTCDAT(RW);			/* 070 BLTCDAT */
	BLTBDAT(RW);			/* 072 BLTBDAT */
	BLTADAT(RW);			/* 074 BLTADAT */
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
	CLXCON(RW);				/* 098 CLXCON */
	intena = RW;			/* 09A INTENA */
	intreq = RW;			/* 09C INTREQ */
	adkcon = RW;			/* 09E ADKCON */
	for (i = 0; i < 8; i++) {
		bplpt[i] = RL;
	}
	bplcon0 = RW;			/* 100 BPLCON0 */
	bplcon1 = RW;			/* 102 BPLCON1 */
	bplcon2 = RW;			/* 104 BPLCON2 */
	bplcon3 = RW;			/* 106 BPLCON3 */
	bpl1mod = RW;			/* 108 BPL1MOD */
	bpl2mod = RW;			/* 10A BPL2MOD */
	bplcon4 = RW;			/* 10C BPLCON4 */
	clxcon2 = RW;			/* 10E CLXCON2* */
	for (i = 0; i < 8; i++) {
		tmp = RW;			/*     BPLXDAT */
		restore_custom_bpl_dat(i, tmp);
	}
	for (i = 0; i < 32; i++) {
		uae_u16 v = RW;
		agnus_colors.color_regs_genlock[i] = denise_colors.color_regs_genlock[i] = (v & 0x8000) != 0;
		agnus_colors.color_regs_ecs[i] = denise_colors.color_regs_ecs[i] = v & 0xfff; /* 180 COLORxx */
		agnus_colors.color_regs_aga[i] = denise_colors.color_regs_aga[i] = getxcolor(v);
		saved_color_regs_aga[i] = getxcolor(v);
	}
	htotal = RW;			/* 1C0 HTOTAL */
	hsstop = RW;			/* 1C2 HSTOP ? */
	hbstrt = RW;			/* 1C4 HBSTRT ? */
	hbstop = RW;			/* 1C6 HBSTOP ? */
	vtotal = RW;			/* 1C8 VTOTAL */
	vsstop = RW;			/* 1CA VSSTOP */
	vbstrt = RW;			/* 1CC VBSTRT */
	vbstop = RW;			/* 1CE VBSTOP */
	SPRHSTRT(RW);			/* 1D0 SPRHSTART */
	SPRHSTOP(RW);			/* 1D2 SPRHSTOP */
	BPLHSTRT(RW);			/* 1D4 BPLHSTRT */
	BPLHSTOP(RW);			/* 1D6 BPLHSTOP */
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
	BPLHMOD(RW);			/* 1E6 ? */
	SPRHPTH(RW);			/* 1E8 ? */
	SPRHPTL(RW);			/* 1EA ? */
	BPLHPTH(RW);			/* 1EC ? */
	BPLHPTL(RW);			/* 1EE ? */
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
	ddfstrt_saved = ddfstrt;
	ddfstop_saved = ddfstop;
	diwhigh_saved = diwhigh;

	bitplane_dma_change(dmacon);
	calcvdiw();
	denise_update_reg_queue(0x100, bplcon0, rga_denise_cycle_line);
	denise_update_reg_queue(0x102, bplcon1, rga_denise_cycle_line);
	denise_update_reg_queue(0x104, bplcon2, rga_denise_cycle_line);
	denise_update_reg_queue(0x106, bplcon3, rga_denise_cycle_line);
	denise_update_reg_queue(0x10c, bplcon4, rga_denise_cycle_line);
	denise_update_reg_queue(0x1e4, diwhigh, rga_denise_cycle_line);
	denise_update_reg_queue(0x08e, diwstrt, rga_denise_cycle_line);
	denise_update_reg_queue(0x090, diwstop, rga_denise_cycle_line);
	if (diwhigh_written) {
		denise_update_reg_queue(0x1e4, diwhigh, rga_denise_cycle_line);
	}
	denise_update_reg_queue(0x1fc, fmode, rga_denise_cycle_line);
	denise_update_reg_queue(0x098, clxcon, rga_denise_cycle_line);
	denise_update_reg_queue(0x10e, clxcon2, rga_denise_cycle_line);
	denise_update_reg_queue(0x1c4, hbstrt, rga_denise_cycle_line);
	denise_update_reg_queue(0x1c6, hbstop, rga_denise_cycle_line);
	draw_denise_line_queue_flush();
	docols(&denise_colors);
	docols(&agnus_colors);

	intreq2 = intreq;
	intena2 = intena;

#if 0
	// pre-6.0 versions stored statefile with LOF already toggled
	if ((bplcon0 & 4) && !(custom_state_flags & 0x100)) {
		lof_store = 1 - lof_store;
	}
#endif
	lof_display = lof_detect = lof_store;

	DISK_restore_custom(dskpt, dsklen, dskbytr);
	restore_blitter_start();

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
	int lof_mod = lof_store;

	DISK_save_custom(&dskpt, &dsklen, &dsksync, &dskbytr);

	if (dstptr) {
		dstbak = dst = dstptr;
	} else {
		dstbak = dst = xmalloc(uae_u8, 4 + 256 * 2 + 4);
	}

	SL(currprefs.chipset_mask | 0x100);
	SW(blt_info.bltddat);	/* 000 BLTDDAT */
	SW(DMACONR());			/* 002 DMACONR */
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
	SW((lof_mod ? 0x8001 : 0) | (lol ? 0x0080 : 0));/* 02A VPOSW */
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
	SW(blt_info.bltcon0);	/* 040 BLTCON0 */
	SW(blt_info.bltcon1);	/* 042 BLTCON1 */
	SW(blt_info.bltafwm);	/* 044 BLTAFWM */
	SW(blt_info.bltalwm);	/* 046 BLTALWM */
	SL(blt_info.bltcpt);	/* 048-04B BLTCPT */
	SL(blt_info.bltbpt);	/* 04C-04F BLTCPT */
	SL(blt_info.bltapt);	/* 050-053 BLTCPT */
	SL(blt_info.bltdpt);	/* 054-057 BLTCPT */
	if (blt_info.vblitsize > 1024 || blt_info.hblitsize > 64) {
		v = 0;
	} else {
		v = (blt_info.vblitsize << 6) | (blt_info.hblitsize & 63);
	}
	SW(v);					/* 058 BLTSIZE */
	SW(blt_info.bltcon0 & 0xff);/* 05A BLTCON0L (use BLTCON0 instead) */
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
	for (i = 0; i < 8; i++) {
		SW(save_custom_bpl_dat(i));	/* 110 BPLxDAT */
	}
	if (full) {
		for (i = 0; i < 8; i++) {
			SL (spr[i].pt);	/* 120-13E SPRxPT */
		}
		for (i = 0; i < 8; i++) {
			struct sprite *s = &spr[i];
			SW(s->pos);	/* 1x0 SPRxPOS */
			SW(s->ctl);	/* 1x2 SPRxPOS */
			SW(0);
			SW(0);
		}
	}
	for (i = 0; i < 32; i++) {
		if (aga_mode) {
			uae_u32 v = agnus_colors.color_regs_aga[i];
			uae_u16 v2;
			v &= 0x00f0f0f0;
			v2 = (v >> 4) & 15;
			v2 |= ((v >> 12) & 15) << 4;
			v2 |= ((v >> 20) & 15) << 8;
			SW(v2);
		} else {
			uae_u16 v = agnus_colors.color_regs_ecs[i];
			if (agnus_colors.color_regs_genlock[i])
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
	SW(bplhmod);				/* 1E6 */
	SW(hhspr >> 16);	/* 1E8 */
	SW(hhspr & 0xffff);	/* 1EA */
	SW(hhbpl >> 16);	/* 1E8 */
	SW(hhbpl & 0xffff);	/* 1EA */
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
		denise_colors.color_regs_genlock[i] = 0;
		if (v & 0x80000000) {
			denise_colors.color_regs_genlock[i] = 1;
		}
		agnus_colors.color_regs_genlock[i] = 0;
		v &= 0xffffff;
		denise_colors.color_regs_aga[i] = v;
		agnus_colors.color_regs_aga[i] = v;
		if (i < 32) {
			saved_color_regs_aga[i] = v;
		}
#else
		RL;
#endif
	}
	docols(&denise_colors);
	docols(&agnus_colors);
	return src;
}

uae_u8 *save_custom_agacolors(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (!aga_mode) {
		int i;
		for (i = 0; i < 256; i++) {
			if (agnus_colors.color_regs_aga[i] || agnus_colors.color_regs_genlock[i])
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
		SL ((agnus_colors.color_regs_aga[i] & 0xffffff) | (agnus_colors.color_regs_genlock[i] ? 0x80000000 : 0));
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
	s->pos = RW;	/* 1x0 SPRxPOS */
	s->ctl = RW;	/* 1x2 SPRxPOS */
	return restore_custom_sprite_denise(num, src, s->pos, s->ctl);
}

uae_u8 *save_custom_sprite(int num, size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	struct sprite *s = &spr[num];

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 100);
	SL(s->pt);		/* 120-13E SPRxPT */
	SW(s->pos);		/* 1x0 SPRxPOS */
	SW(s->ctl);		/* 1x2 SPRxPOS */
	dst = save_custom_sprite_denise(num, dst);
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

static int getregfrompt(uaecptr *pt)
{
	if (pt == &blt_info.bltdpt)
		return 0x000;
	if (pt == &blt_info.bltcpt)
		return 0x070;
	if (pt == &blt_info.bltbpt)
		return 0x072;
	if (pt == &blt_info.bltapt)
		return 0x074;
	if (pt == &bplpt[0])
		return 0x110;
	if (pt == &bplpt[1])
		return 0x112;
	if (pt == &bplpt[2])
		return 0x114;
	if (pt == &bplpt[3])
		return 0x116;
	if (pt == &bplpt[4])
		return 0x118;
	if (pt == &bplpt[5])
		return 0x11a;
	if (pt == &bplpt[6])
		return 0x11c;
	if (pt == &bplpt[7])
		return 0x11e;
	if (pt == &spr[0].pt)
		return 0x140;
	if (pt == &spr[1].pt)
		return 0x142;
	if (pt == &spr[2].pt)
		return 0x144;
	if (pt == &spr[3].pt)
		return 0x146;
	if (pt == &spr[4].pt)
		return 0x148;
	if (pt == &spr[5].pt)
		return 0x14a;
	if (pt == &spr[6].pt)
		return 0x14c;
	if (pt == &spr[7].pt)
		return 0x14e;
	if (pt == &cop_state.ip)
		return 0x8c;
	return -1;
}
static uaecptr *getptfromreg(int reg)
{
	switch (reg)
	{
	case -1:
	case 0xffff:
		return NULL;
	case 0x000:
		return &blt_info.bltdpt;
	case 0x070:
		return &blt_info.bltcpt;
	case 0x072:
		return &blt_info.bltbpt;
	case 0x074:
		return &blt_info.bltapt;
	case 0x110:
		return &bplpt[0];
	case 0x112:
		return &bplpt[1];
	case 0x114:
		return &bplpt[2];
	case 0x116:
		return &bplpt[3];
	case 0x118:
		return &bplpt[4];
	case 0x11a:
		return &bplpt[5];
	case 0x11c:
		return &bplpt[6];
	case 0x11e:
		return &bplpt[7];
	case 0x140:
		return &spr[0].pt;
	case 0x142:
		return &spr[1].pt;
	case 0x144:
		return &spr[2].pt;
	case 0x146:
		return &spr[3].pt;
	case 0x148:
		return &spr[4].pt;
	case 0x14a:
		return &spr[5].pt;
	case 0x14c:
		return &spr[6].pt;
	case 0x14e:
		return &spr[7].pt;
	case 0x8c:
		return &cop_state.ip;
	}
	return &dummyrgaaddr;
}

uae_u8 *save_custom_slots(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 1000);

	uae_u32 v = 1 | 8;
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
		save_u16(0);//save_u16(cycle_line_pipe[i]);
		save_u16(0);//save_u16(cycle_line_slot[i]);
	}

	// RGA pipeline
	for (int i = 0; i < RGA_SLOT_TOTAL; i++) {
		struct rgabuf *r = &rga_pipe[i];
		int regidx;
		save_u32(r->type);
		save_u16(r->reg);
		save_u32(r->pv);
		save_u32(r->alloc);
		save_u32(r->write);
		save_u32(r->bpldat);
		save_u32(r->sprdat);
		save_u32(r->bltdat);
		save_u32(r->auddat);
		save_u32(r->refdat);
		save_u32(r->dskdat);
		save_u32(r->copdat);
		save_u16(r->bplmod);
		save_u16(r->bltmod);
		regidx = getregfrompt(r->p);
		save_u16(regidx);
		regidx = getregfrompt(r->conflict);
		save_u16(regidx);
	}

	save_u8(rga_slot_first_offset);

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
		//cycle_line_pipe[i] = restore_u16();
		//cycle_line_slot[i] = (uae_u8)restore_u16();
		restore_u16();
		restore_u16();
	}

	// RGA pipeline
	if (v & 8) {
		for (int i = 0; i < RGA_SLOT_TOTAL; i++) {
			struct rgabuf *r = &rga_pipe[i];
			int regidx;
			r->type = restore_u32();
			r->reg = restore_u16();
			r->pv = restore_u32();
			r->alloc = restore_u32();
			r->write = restore_u32();
			r->bpldat = restore_u32();
			r->sprdat = restore_u32();
			r->bltdat = restore_u32();
			r->auddat = restore_u32();
			r->refdat = restore_u32();
			r->dskdat = restore_u32();
			r->copdat = restore_u32();
			r->bplmod = restore_u16();
			r->bltmod = restore_u16();
			regidx = restore_u16();
			r->p = getptfromreg(regidx);
			regidx = restore_u16();
			r->conflict = getptfromreg(regidx);
		}
		rga_slot_first_offset = restore_u8();
		rga_slot_in_offset = (rga_slot_first_offset + 1) & 3;
		rga_slot_out_offset = (rga_slot_in_offset + 1) & 3;
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
				f = event_doint_delay_do_ext_old;
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
			case 11:
				f = event_doint_delay_do_ext;
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
			} else if (f == event_doint_delay_do_ext_old) {
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
			} else if (f == event_doint_delay_do_ext) {
				type = 11;
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
		dstbak = dst = xmalloc(uae_u8, 128);
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
	if (currprefs.turbo_emulation != changed_prefs.turbo_emulation) {
		warpmode(changed_prefs.turbo_emulation);
	}
	if (inputdevice_config_change_test()) {
		inputdevice_copyconfig (&changed_prefs, &currprefs);
	}
	currprefs.immediate_blits = changed_prefs.immediate_blits;
	currprefs.waiting_blits = changed_prefs.waiting_blits;
	currprefs.blitter_speed_throttle = changed_prefs.blitter_speed_throttle;
	currprefs.collision_level = changed_prefs.collision_level;
	currprefs.keyboard_nkro = changed_prefs.keyboard_nkro;
	if (currprefs.keyboard_mode != changed_prefs.keyboard_mode) {
		currprefs.keyboard_mode = changed_prefs.keyboard_mode;
		// send powerup sync
		keyboard_connected(true);
	} else if (currprefs.keyboard_mode >= 0 && changed_prefs.keyboard_mode < 0) {
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
	currprefs.cs_optimizations = changed_prefs.cs_optimizations;

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

	cia_set_eclockphase();
	if (syncchange) {
		varsync_changed = 2;
	}
	if (beamcon0 & (BEAMCON0_VARVBEN | BEAMCON0_CSCBEN | BEAMCON0_VARVSYEN | BEAMCON0_VARHSYEN | BEAMCON0_VARCSYEN | BEAMCON0_VARBEAMEN | BEAMCON0_DUAL | BEAMCON0_BLANKEN)) {
		if (!programmed_register_accessed_h || !programmed_register_accessed_v) {
			programmed_register_accessed_h = true;
			programmed_register_accessed_v = true;
			dumpsync();
		}
	}
	resetfulllinestate();

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

static uae_u16 fetch16(struct rgabuf *r)
{
	uaecptr p = r->pv;
	uae_u16 v;
	if (aga_mode) {
		// AGA always does 32-bit fetches, this is needed
		// to emulate 64 pixel wide sprite side-effects.
		uae_u32 vv = chipmem_lget_indirect(p & ~3);
		if (p & 2) {
			v = (uae_u16)vv;
		} else {
			v = vv >> 16;
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

static uae_u32 fetch32_bpl(struct rgabuf *r)
{
	uae_u32 v;
	uaecptr p = r->pv;
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

static uae_u32 fetch32_spr(struct rgabuf *r)
{
	uae_u32 v;
	uaecptr p = r->pv;
	uaecptr pm = p & ~3;
	if (p & 2) {
		v = chipmem_lget_indirect(pm) & 0x0000ffff;
		v |= v << 16;
	} else if (fetchmode_fmode_spr & 2) { // optimized (fetchmode_fmode & 3) == 2
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

static uae_u64 fetch64(struct rgabuf *r)
{
	uae_u64 v;
	uaecptr p = r->pv;
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


static void process_copper(struct rgabuf *r)
{
	uae_u16 id = r->copdat;
	int hpos = agnus_hpos;

	if ((id & 0xf) == 0xf) {
		// last cycle was even: cycle 1 got allocated which is unusable for copper
		// reads from address zero!
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x1fe, 0, DMARECORD_COPPER, 7);
		}
#endif
		uae_u16 tmp = chipmem_wget_indirect(0);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(tmp);
			record_dma_event(DMA_EVENT_COPPERUSE);
		}
#endif
		regs.chipset_latch_rw = tmp;
		return;
	}

	switch (cop_state.state)
	{

	case COP_stop:
		break;
	case COP_strobe_vbl_delay:
	{
		cop_state.state = COP_strobe_vbl_delay2;
		cop_state.strobeip = getstrobecopip();
		cop_state.strobe = 0;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event(DMA_EVENT_COPPERUSE);
		}
#endif
	}
	break;
	case COP_strobe_vbl_delay2:
	{
	// fake MOVE phase 2
#ifdef DEBUGGER
		// inhibited $8c access
		if (debug_dma) {
			record_dma_read(0x1fe, cop_state.ip, DMARECORD_COPPER, 6);
		}
#endif
		cop_state.ir[1] = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[1]);
			record_dma_event(DMA_EVENT_COPPERUSE);
		}
#endif
		cop_state.ip = cop_state.strobeip;
		cop_state.state = COP_read1;
		regs.chipset_latch_rw = cop_state.ir[1];
	}
	break;
	case COP_strobe_vbl_delay_nodma:
		cop_state.ip = getstrobecopip();
		cop_state.state = COP_read1;
		break;

	case COP_strobe_vbl_extra_delay1:
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x8c, cop_state.ip, DMARECORD_COPPER, 6);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c);
		}
#endif
		cop_state.ir[0] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
		cop_state.ip = cop_state.strobeip;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[0]);
		}
#endif
		cop_state.state = COP_read1;
	break;
	case COP_strobe_vbl_extra_delay2:
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x8c, cop_state.ip, DMARECORD_COPPER, 6);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c);
		}
#endif
		cop_state.ir[0] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
		cop_state.ip += 2;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[0]);
		}
#endif
		cop_state.state = COP_strobe_vbl_extra_delay3;
	break;
	case COP_strobe_vbl_extra_delay3:
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x1fe, cop_state.ip, DMARECORD_COPPER, 6);
		}
#endif
		cop_state.ir[1] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
		cop_state.ip = cop_state.strobeip;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[1]);
		}
#endif
		cop_state.state = COP_read1;
	break;

	// cycle after odd strobe write is normal IR2
	case COP_strobe_delay_start_odd:
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(r->reg, cop_state.ip, DMARECORD_COPPER, 6);
		}
#endif
		cop_state.ir[1] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[1]);
		}
#endif
		cop_state.ip += 2;
		cop_state.strobeip = getstrobecopip();
		cop_state.strobe = 0;
		cop_state.state = COP_strobe_delay1_odd;
		cop_state.ignore_next = 0;
	break;

	case COP_strobe_delay1_odd:
	{
		// odd cycle strobe blitter conflict possibility
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event(DMA_EVENT_COPPERUSE);
		}
#endif
		// conflict with blitter: blitter pointer becomes old copper pointer OR blitter pointer
		// after the blit cycle, blitter pointer becomes new copper pointer. This is done in blitter emulation.
		// copper pointer is not affected by this.
		if (r->type & CYCLE_BLITTER) {
			r->conflict = r->p;
			r->conflictaddr = cop_state.strobeip;
			r->p = &dummyrgaaddr;
			r->pv |= cop_state.ip;
			cop_state.strobeip |= r->bltmod;
		}
		cop_state.ip = cop_state.strobeip;
		cop_state.state = COP_read1;
	}
	break;

	case COP_strobe_delay1:
	{
		if (cop_state.strobetype >= 0) {
			uae_u16 reg = cop_state.ir[0] & 0x1FE;
			// fake MOVE phase 1
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(r->reg, cop_state.ip, DMARECORD_COPPER, 6);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, r->reg);
			}
#endif
			cop_state.ir[0] = regs.chipset_latch_rw = last_custom_value = chipmem_wget_indirect(cop_state.ip);
			if (cop_state.strobetype == 0) {
				cop_state.ip = cop_state.strobeip;
				cop_state.state = COP_read1;
			} else {
				cop_state.ip += 2;
				cop_state.strobeip = getstrobecopip();
				cop_state.strobe = 0;
				cop_state.state = COP_strobe_delay2;
				if (cop_state.strobetype >= 2) {
					if (!test_copper_dangerous(reg, true) && !cop_state.ignore_next) {
						custom_wput_copper(cop_state.ip, reg, cop_state.ir[0], 0);
					}
				}
				cop_state.ignore_next = 0;
			}
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read_value(cop_state.ir[0]);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(cop_state.ir[0]);
			}
#endif
		} else {
			// fake and hidden MOVE phase 1
			cop_state.ip = cop_state.strobeip;
			cop_state.state = COP_read1;
		}
	}
	break;
	case COP_strobe_delay2:
	{
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(r->reg, cop_state.ip, DMARECORD_COPPER, 6);
		}
#endif
		cop_state.ir[1] = chipmem_wget_indirect(cop_state.ip);
		regs.chipset_latch_rw = cop_state.ir[1];
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read_value(cop_state.ir[1]);
		}
#endif
		if (cop_state.strobetype >= 1) {
			cop_state.ip = cop_state.strobeip;
		} else {
			cop_state.ip += 2;
		}
		cop_state.state = COP_read1;
	}
	break;

	case COP_strobe_delay_start:
	{
		cop_state.state = COP_strobe_delay1_odd;
	}
	break;

	case COP_read1:
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_read(0x8c, cop_state.ip, DMARECORD_COPPER, 5);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c);
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
		cop_state.state = COP_read2;
		break;
	case COP_read2:
		if (cop_state.ir[0] & 1) {
			// WAIT or SKIP
#ifdef DEBUGGER
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c);
			}
#endif
			cop_state.ir[1] = chipmem_wget_indirect(cop_state.ip);
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(0x8c, cop_state.ip, DMARECORD_COPPER, (cop_state.ir[1] & 1) ? 3 : 2);
				record_dma_read_value(cop_state.ir[1]);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(cop_state.ir[1]);
			}
#endif
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
			if (debug_copper) {
				record_copper(debugip - 4, debugip, cop_state.ir[0], cop_state.ir[1], hpos, vpos);
			}
#endif

		} else {
			// MOVE
			uae_u16 reg = cop_state.ir[0] & 0x1FE;

#ifdef DEBUGGER
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c);
			}
#endif
			cop_state.ir[1] = chipmem_wget_indirect(cop_state.ip);

			uae_u16 data = cop_state.ir[1];
			cop_state.state = COP_read1;

			uae_u16 preg = reg;

			// Previous instruction was SKIP that skipped
			if (cop_state.ignore_next > 0) {
				reg = 0x08c;
			}

			bool dang = test_copper_dangerous(preg, false);

#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(reg, cop_state.ip, DMARECORD_COPPER, dang ? 4 : 1);
				record_dma_read_value(cop_state.ir[1]);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_value(cop_state.ir[1]);
			}
#endif
			cop_state.ip += 2;

#ifdef DEBUGGER
			uaecptr debugip = cop_state.ip;
#endif
			// was "dangerous" register -> copper stopped
			if (dang) {
				return;
			}

			if (reg == 0x100) {
				// BPLCON0 new value is needed early
				//bplcon0_denise_change_early(hpos, data);
#if 0
				cop_state.moveaddr = reg;
				cop_state.movedata = data;
				cop_state.movedelay = 1;
				cop_state.moveptr = cop_state.ip;
#else
				custom_wput_copper(cop_state.ip, reg, data, 0);
#endif
			} else {
				custom_wput_copper(cop_state.ip, reg, data, 0);
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
			record_dma_read(0x8c, cop_state.ip, DMARECORD_COPPER, 0);
		}
		if (memwatch_enabled) {
			debug_getpeekdma_chipram(cop_state.ip, MW_MASK_COPPER, 0x8c);
		}
#endif
		break;
	default:
		write_log(_T("copper_fetch invalid state %d! %02x\n"), cop_state.state, id);
		break;
	}
}

static void generate_copper(void)
{
	int hpos = agnus_hpos;

	if ((hpos & 1) != COPPER_CYCLE_POLARITY) {
		// copper does not advance if hpos bit 0 didn't toggle
		if ((hpos & 1) == (agnus_hpos_prev & 1)) {
			switch (cop_state.state)
			{
				case COP_read1:
				case COP_read2:
				case COP_wait:
				case COP_skip:
				case COP_strobe_delay1:
				{
					generate_copper_cycle_if_free(CYCLE_PIPE_COPPER | 0xf);
				}
				break;
			}
		}
		return;
	}

	if ((hpos & 1) == (agnus_hpos_prev & 1)) {
		return;
	}

	bool bus_allocated = !check_rga_free_slot_in();

	switch (cop_state.state)
	{
		case COP_strobe_vbl_delay_nodma:
		generate_copper_cycle_if_free(CYCLE_PIPE_COPPER);
		break;
		case COP_strobe_vbl_delay2:
		// Second cycle after COPJMP does basically skipped MOVE (MOVE to 1FE)
		// Cycle is used and needs to be free.
		generate_copper_cycle_if_free(CYCLE_PIPE_COPPER);
		break;

		case COP_strobe_delay_start_odd:
		{
			cop_state.strobeip = getstrobecopip();
			cop_state.strobe = 0;
			cop_state.ignore_next = 0;
			if (hpos == 1 && !bus_allocated) {
				// if COP_strobe_delay2 crossed scanlines, it will be skipped!
				struct rgabuf *rga = generate_copper_cycle_if_free(CYCLE_PIPE_COPPER);
				rga->reg = 0x1fe;
				cop_state.state = COP_strobe_delay1;
				cop_state.strobetype = 0;
			} else {
				cop_state.state = COP_strobe_delay1_odd;
				// not allocated
				struct rgabuf *rga = read_rga_in();
				if (rga) {
					rga->type |= CYCLE_COPPER;
					rga->copdat = 0;
					if (!rga->alloc) {
						rga->alloc = -1;
					}
				}
			}
		}
		break;

		case COP_strobe_delay1_odd:
		{
			cop_state.state = COP_strobe_delay1_odd;
			if (cop_state.strobe) {
				cop_state.strobeip = getstrobecopip();
				cop_state.strobe = 0;
				cop_state.ignore_next = 0;
			}
			// not allocated
			struct rgabuf *rga = read_rga_in();
			if (rga) {
				rga->type |= CYCLE_COPPER;
				rga->copdat = 0;
				if (!rga->alloc) {
					rga->alloc = -1;
				}
			}
		}
		break;

		case COP_strobe_delay_start:
		if (hpos == 1 && get_cycles() > cop_state.strobe_cycles) {
			cop_state.state = COP_strobe_delay1_odd;
			cop_state.strobeip = getstrobecopip();
			cop_state.strobe = 0;
			cop_state.ignore_next = 0;
			// not allocated
			struct rgabuf *rga = read_rga_in();
			if (rga) {
				rga->type |= CYCLE_COPPER;
				rga->copdat = 0;
				if (!rga->alloc) {
					rga->alloc = -1;
				}
			}
		} else {
			cop_state.state = COP_strobe_delay1;
		}
		break;

		case COP_strobe_delay1:
		// First cycle after COPJMP. This is the strange one.
		// This cycle does not need to be free
		// But it still gets allocated by copper if it is free = CPU and blitter can't use it.
		if (bus_allocated) {
			cop_state.state = COP_strobe_delay2;
			if (cop_state.strobeip == 0xffffffff) {
				cop_state.strobetype = 1;
			}
			cop_state.strobeip = getstrobecopip();
			cop_state.strobe = 0;
			cop_state.ignore_next = 0;
		} else {
			if (hpos == 1 && get_cycles() > cop_state.strobe_cycles) {
				// if COP_strobe_delay2 crossed scanlines, it will be skipped!
				cop_state.strobetype = 0;
			}
			if (cop_state.strobetype == 0) {
				cop_state.strobeip = getstrobecopip();
				cop_state.strobe = 0;
				cop_state.ignore_next = 0;
			}
			cop_state.state = COP_strobe_delay1;
			// allocated and $1fe, not $08c
			struct rgabuf *rga = generate_copper_cycle_if_free(0x08);
			if (cop_state.strobetype == 0) {
				if (rga) {
					rga->reg = 0x1fe;
				}
			}
		}
		break;
		case COP_strobe_delay2:
		{
			struct rgabuf *rga = generate_copper_cycle_if_free(CYCLE_PIPE_COPPER);
			if (cop_state.strobe && cop_state.strobetype >= 1) {
				cop_state.strobe = 0;
				if (rga) {
					rga->reg = 0x1fe;
				}
			}
		}
		break;

		case COP_strobe_vbl_extra_delay3:
		{
			struct rgabuf *rga = generate_copper_cycle_if_free(CYCLE_PIPE_COPPER);
			if (cop_state.strobe && cop_state.strobetype >= 1) {
				cop_state.strobe = 0;
				if (rga) {
					rga->reg = 0x1fe;
				}
			}
		}
		break;

			// Request IR1
		case COP_read1:
			generate_copper_cycle_if_free(CYCLE_PIPE_COPPER | 0x02);
			break;

			// Request IR2
		case COP_read2:
			generate_copper_cycle_if_free(CYCLE_PIPE_COPPER | 0x03);
			break;

			// WAIT: Got IR2, first idle cycle.
			// Need free cycle, cycle not allocated.
		case COP_wait_in2:
			{
				if (bus_allocated) {
					break;
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
					if (cop_state.strobe_next == COP_stop) {
						copper_enabled_thisline = 0;
					}
#ifdef DEBUGGER
					if (debug_dma && comp == -2) {
						record_dma_event(DMA_EVENT_COPPERWAKE2);
					}
#endif
					break;
				}

				if (comp) {
					break;
				}

#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_COPPERWAKE2);
					record_dma_event(DMA_EVENT_COPPERWAKE);
				}
#endif
				if (bus_allocated) {
					break;
				}

				cop_state.state = COP_wait;
			}
			break;

			// Wait finished, request IR1.
		case COP_wait:
			{
				if (!generate_copper_cycle_if_free(CYCLE_PIPE_COPPER | 0x04)) {
					break;
				}
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_COPPERWAKE);
				}
				if (debug_copper) {
					record_copper(cop_state.ip - 4, 0xffffffff, cop_state.ir[0], cop_state.ir[1], hpos, vpos);
				}
#endif
				cop_state.state = COP_read1;
			}
			break;

			// SKIP: Got IR2. First idle cycle.
			// Free cycle needed, cycle not allocated.
		case COP_skip_in2:

			if (bus_allocated) {
				break;
			}
			cop_state.state = COP_skip1;
			break;

			// SKIP: Second idle cycle. Do nothing.
			// Free cycle needed, cycle not allocated.
		case COP_skip1:

			if (bus_allocated) {
				break;
			}

			cop_state.state = COP_skip;
			break;

			// Check comparison. SKIP finished. Request IR1.
		case COP_skip:
			if (!generate_copper_cycle_if_free(CYCLE_PIPE_COPPER | 0x005)) {
				break;
			}

			if (!coppercomp(hpos, false)) {
				cop_state.ignore_next = 1;
			} else {
				cop_state.ignore_next = -1;
			}

#ifdef DEBUGGER
			if (debug_dma) {
				if (cop_state.ignore_next > 0) {
					record_dma_event(DMA_EVENT_COPPERSKIP);
				} else {
					record_dma_event(DMA_EVENT_COPPERWAKE2);
				}
			}
			if (debug_copper) {
				record_copper(cop_state.ip - 4, 0xffffffff, cop_state.ir[0], cop_state.ir[1], hpos, vpos);
			}
#endif

			cop_state.state = COP_read1;
			break;

		case COP_strobe_extra:
			// Wait 1 copper cycle doing nothing
			cop_state.state = COP_strobe_delay1;
			break;

		default:
			break;
		}

		if (cop_state.strobe_next != COP_stop) {
			cop_state.state = cop_state.strobe_next;
			cop_state.strobe_next = COP_stop;
		}
}

// Because BPL and SPR DMA is decided 1 CCK earlier than others
// PT and MOD need to be loaded in following cycle, cycle
// when request goes to DMA addressing logic.
static void bitplane_rga_ptmod(void)
{
	struct rgabuf *r = read_rga_in();
	if (r->alloc > 0) {
		if (r->type == CYCLE_BITPLANE) {
			int bpl = r->bpldat & 7;
			bool domod = (r->bpldat & 8) != 0;
			int mod = 0;
			if (domod) {
				if (fmode & 0x4000) {
					if (((diwstrt >> 8) ^ (vpos ^ 1)) & 1) {
						mod = bpl1mod;
					} else {
						mod = bpl2mod;
					}
				} else {
					mod = (bpl & 1) ? bpl2mod : bpl1mod;
				}
			}
			r->p = &bplpt[bpl];
			r->pv = *r->p;
			r->bplmod = mod;
		} else if (r->type == CYCLE_SPRITE) {
			int num = r->sprdat & 7;
			struct sprite *s = &spr[num];
			r->p = &s->pt;
			r->pv = *r->p;
		}
	}
}

// Generate UHRES (0x78, 0x7A) RGA slots
static void generate_uhres(void)
{
	if ((dmacon & DMA_MASTER) && check_rga_free_slot_in())  {
		if (uhres_state > 0) {
			uhres_state++;
		}
		if (agnus_hpos == 1 && !beamcon0_dual) {
			// !DUAL: trigger is START
			uhres_state = 2;
		} else if (hhpos == 0 && beamcon0_dual) {
			// DUAL: trigger is HHPOS reset
			uhres_state = 1;
		}
		if (uhres_spr && (uhres_state == 4 || uhres_state == 6)) {
			struct rgabuf *r = write_rga(RGA_SLOT_IN, CYCLE_UHRESSPR, 0x078, NULL);
			r->p = &hhspr;
			r->pv = *r->p;
		}
		if (uhres_bpl && (uhres_state == 5)) {
			struct rgabuf *r = write_rga(RGA_SLOT_IN, CYCLE_UHRESBPL, 0x07a, NULL);
			r->bplmod = bplhmod;
			r->p = &hhbpl;
			r->pv = *r->p;
		}
	}
}

static void bpl_dma_normal_stop(int hpos)
{
#ifdef DEBUGGER
	if (debug_dma && bprun) {
		record_dma_event_agnus(AGNUS_EVENT_BPRUN, false);
		record_dma_event_agnus(AGNUS_EVENT_BPRUN2, false);
	}
#endif
	ddf_stopping = 0;
	bprun = 0;
	bprun_end = get_cycles();
	plfstrt_sprite = 0x100;
	if (!ecs_agnus) {
		ddf_limit = true;
	}
	if (bplcon0_planes > 0 && hpos > ddflastword_total) {
		ddflastword_total = hpos;
	}
	bpl_autoscale();
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

static void generate_bpl(bool clock)
{
	if (bprun > 0) {
		int hpos = agnus_hpos;
		bool last = islastbplseq();
		int cycle_pos = bprun_cycle & fetchstart_mask;
		if (dmacon_bpl) {
			bool domod = false;
			if (ddf_stopping == 2) {
				int cycle = bprun_cycle & 7;
				if (fm_maxplane == 8 || (fm_maxplane == 4 && cycle >= 4) || (fm_maxplane == 2 && cycle >= 6)) {
					domod = true;
				}
			}
			int plane = bpl_sequence[cycle_pos];
			if (plane >= 1 && plane <= bplcon0_planes_limit) {
				int bpl = plane - 1;
				struct rgabuf *rga = write_rga(RGA_SLOT_BPL, CYCLE_BITPLANE, 0x110 + bpl * 2, NULL);
				rga->bpldat = bpl | (domod ? 8 : 0);
			}
		}
		if (clock) {
			bprun_cycle++;
		}
		if (last) {
			if (ddf_stopping == 2) {
				bpl_dma_normal_stop(hpos);
			}
			if (ddf_stopping == 1) {
				ddf_stopping = 2;
			}
		}
	}
}

static void update_bpl_scandoubler(void)
{
	for (int i = 0; i < MAX_PLANES; i++) {
		scandoubled_bpl_ptr[linear_vpos][lof_store][i] = bplpt[i];
	}
}

static void decide_bpl(int hpos)
{
	bool dma = dmacon_bpl;
	bool diw = vdiwstate == diw_states::DIW_waiting_stop;

	if (ecs_agnus) {
		// ECS/AGA


		if (bprun < 0 && (hpos & 1)) {
			bprun = 1;
			bprun_cycle = 0;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_BPRUN, true);
			}
#endif
		}

#if 0
		// BPRUN latched: off
		if (bprun == 3) {
			if (ddf_stopping == 1) {
				// If bpl sequencer counter was all ones (last cycle of block): ddf passed jumps to last step.
				if (islastbplseq()) {
					ddf_stopping = 2;
				}
			}
			bprun = 0;
			bprun_end = hpos;
		}
#endif

		// DDFSTRT == DDFSTOP: BPRUN gets enabled and DDF passed state in next cycle.
		if (ddf_enable_on < 0) {
			ddf_enable_on = 0;
			if (bprun && !ddf_stopping) {
				ddf_stopping = 1;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_BPRUN2, true);
				}
#endif
			}
		}

		// Hard start limit
		if (hpos == (0x18 | 0)) {
			ddf_limit = false;
		}

		// DDFSTRT
		if (hpos == ddfstrt) {
			ddf_enable_on = 1;
			if (currprefs.gfx_scandoubler && linear_vpos < MAX_SCANDOUBLED_LINES) {
				update_bpl_scandoubler();
			}
		}

		// Hard stop limit
		if (hpos == (0xd7 + 0)) {
			// Triggers DDFSTOP condition if hard limits are not disabled.
			ddf_limit = true;
			if (bprun && !ddf_stopping) {
				if (!harddis_h) {
					ddf_stopping = 1;
#ifdef DEBUGGER
					if (debug_dma) {
						record_dma_event_agnus(AGNUS_EVENT_BPRUN2, true);
					}
#endif
				}
			}
		}

		// DDFSTOP
		// Triggers DDFSTOP condition.
		// Clears DDF allowed flag.
		if (hpos == (ddfstop | 0)) {
			if (bprun && !ddf_stopping) {
				ddf_stopping = 1;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_BPRUN2, true);
				}
#endif
			}
			if (ddfstop != ddfstrt) {
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
				// Bitplane sequencer activated
				bprun = -1;
				if (plfstrt_sprite > hpos + 1) {
					plfstrt_sprite = hpos + 1;
				}
				bprun_start(hpos);
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event(DMA_EVENT_DDFSTRT);
				}
#endif
			}
			hwi_old = hwi;
		}

		if (bprun == 2) {
			bprun = 0;
			// If DDF has passed, jumps to last step.
			// (For example Scoopex Crash landing crack intro)
			if (ddf_stopping == 1) {
				ddf_stopping = 2;
			} else if (ddf_stopping == 0) {
				// If DDF has not passed, set it as passed.
				ddf_stopping = 1;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_BPRUN2, true);
				}
#endif
			}
			plfstrt_sprite = 0x100;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_BPRUN, false);
			}
#endif
		}

		// DIW or DMA switched off: clear BPRUN
		if ((!dma || !diw) && bprun == 1) {
			bprun = 2;
			if (ddf_stopping == 1) {
				ddf_stopping = 2;
				bprun = 0;
				plfstrt_sprite = 0x100;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_BPRUN, false);
				}
#endif
			}
		}

	} else {

		// OCS

		// DDFSTOP
		// Triggers DDFSTOP condition.
		if (hpos == (ddfstop | 0)) {
			if (bprun > 0 && !ddf_stopping) {
				ddf_stopping = 1;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_BPRUN2, true);
				}
#endif
			}
		}

		// BPRUN latched: On
		if (bprun < 0 && (hpos & 1)) {
			bprun = 1;
			bprun_cycle = 0;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_BPRUN, true);
			}
#endif
		}
#if 0
		// BPRUN latched: off
		if (bprun == 3) {
			bprun = 0;
			bprun_end = hpos;
			plfstrt_sprite = 0x100;
		}
#endif

		// Hard start limit
		if (hpos == 0x18) {
			ddf_limit = false;
		}

		// Hard stop limit
		// Triggers DDFSTOP condition. Last cycle of bitplane DMA resets DDFSTRT limit.
		if (hpos == (0xd7 + 0)) {
			if (bprun && !ddf_stopping) {
				ddf_stopping = 1;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_BPRUN2, true);
				}
#endif
			}
		}

		// DDFSTRT
		if (hpos == ddfstrt) {
			ddfstrt_match = true;
			if (currprefs.gfx_scandoubler && linear_vpos < MAX_SCANDOUBLED_LINES) {
				update_bpl_scandoubler();
			}
		} else {
			ddfstrt_match = false;
		}

		if (!ddf_limit && ddfstrt_match && !bprun && dma && diw) {
			// Bitplane sequencer activated
			bprun = -1;
			if (plfstrt_sprite > hpos) {
				plfstrt_sprite = hpos;
			}
			bprun_start(hpos);
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event(DMA_EVENT_DDFSTRT);
			}
#endif
		}

		if (bprun == 2) {
			// If DDF has passed, jumps to last step.
			// (For example Scoopex Crash landing crack intro)
			if (ddf_stopping == 1) {
				ddf_stopping = 2;
			}
			bprun = 0;
			plfstrt_sprite = 0x100;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_BPRUN, false);
			}
#endif
		}

		// DMA or DIW off: clear BPRUN
		if ((!dma || !diw) && bprun == 1) {
			bprun = 2;
			if (ddf_stopping == 1) {
				ddf_stopping = 2;
				bprun = 0;
				plfstrt_sprite = 0x100;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_BPRUN, false);
				}
#endif
			}
		}
	}
}

static void check_bpl_vdiw(void)
{
	bool forceoff = agnus_bsvb && !harddis_v;
	bool start = vpos == plffirstline && !forceoff;
	// VB start line forces vertical display window off (if HARDDIS=0)
	bool end = vpos == plflastline || forceoff;

	if (start && !end) {
		vdiwstate = diw_states::DIW_waiting_stop;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_VDIW, true);
		}
#endif
	} else if (end) {
		vdiwstate = diw_states::DIW_waiting_start;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_VDIW, false);
		}
#endif
	}
	if (linear_vpos < MAX_SCANDOUBLED_LINES) {
		scandoubled_bpl_ena[linear_vpos] = vdiwstate != diw_states::DIW_waiting_start;
	}
}

static void generate_sprites(int num, int slot)
{
	int hp = agnus_hpos;

	if (slot == 0 || slot == 2) {
		struct sprite *s = &spr[num];
		if (slot == 0) {
			if (!s->dmacycle && s->dmastate) {
				s->dmacycle = 1;
			}
			if (vpos == s->vstart) {
				s->dmastate = 1;
				s->dmacycle = 1;
				if (num == 0 && slot == 0) {
					cursorsprite(s);
				}
#if 0
				if (num == 0)
					write_log("START %04x %04x %08x\n", s->vstart, s->vstop, s->pt);
#endif
			}
			if (vpos == s->vstop || agnus_vb_active_end_line) {
				s->dmastate = 0;
				s->dmacycle = 1;
#if 0
				if (num == 0)
					write_log("STOP %04x %04x %08x\n", s->vstart, s->vstop, s->pt);
#endif
			}
		}
		if (dmaen(DMA_SPRITE) && s->dmacycle) {
			bool dodma = false;

			if (hp <= plfstrt_sprite) {
				dodma = true;
#ifdef AGA
				if (dodma && s->dblscan && (fmode & 0x8000) && (vpos & 1) != (s->vstart & 1) && s->dmastate) {
					dodma = false;
				}
#endif
				if (dodma) {
					uae_u32 dat = CYCLE_PIPE_SPRITE | (s->dmastate ? 0x10 : 0x00) | (s->dmacycle == 1 ? 0 : 8) | num;
					int reg = 0x140 + slot + num * 8 + (s->dmastate ? 4 : 0);
					struct rgabuf *rga = write_rga(RGA_SLOT_BPL, CYCLE_SPRITE, reg, &s->pt);
					if (get_cycles() == sprite_dma_change_cycle_on) {
						// If sprite DMA is switched on just when sprite DMA is decided, channel is still decided but it is not allocated!
						// Blitter can use this cycle, causing a conflict.
						rga->alloc = 0;
						rga->conflict = &s->pt;
					} else if (get_cycles() == bprun_end) {
						// last bitplane cycle is available for sprites (if bitplane ends before all sprites)
					}
					rga->sprdat = dat;
				}
			}
		}
		if (s->dmacycle) {
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



#define DMAL_REFRESH0 (1 << 1)
#define DMAL_REFRESH1 (1 << 2)
#define DMAL_REFRESH2 (1 << 3)
#define DMAL_REFRESH3 (1 << 4)
#define DMAL_DSK0 (1 << 5)
#define DMAL_DSK1 (1 << 6)
#define DMAL_DSK2 (1 << 7)
#define DMAL_AUD0 (1 << 8)
#define DMAL_AUD1 (1 << 9)
#define DMAL_AUD2 (1 << 10)
#define DMAL_AUD3 (1 << 11)
// sprites have earlier DMA decisions
#define DMAL_SPR0A (1 << 11)
#define DMAL_SPR0B (1 << 12)
#define DMAL_SPR1A (1 << 13)
#define DMAL_SPR1B (1 << 14)
#define DMAL_SPR2A (1 << 15)
#define DMAL_SPR2B (1 << 16)
#define DMAL_SPR3A (1 << 17)
#define DMAL_SPR3B (1 << 18)
#define DMAL_SPR4A (1 << 19)
#define DMAL_SPR4B (1 << 20)
#define DMAL_SPR5A (1 << 21)
#define DMAL_SPR5B (1 << 22)
#define DMAL_SPR6A (1 << 23)
#define DMAL_SPR6B (1 << 24)
#define DMAL_SPR7A (1 << 25)
#define DMAL_SPR7B (1 << 26)

static void process_dmal(uae_u32 v)
{
	uae_u16 dmalt = audio_dmal();
	dmalt <<= (3 * 2);
	dmalt |= disk_dmal();
	dmal = dmalt;
	inputdevice_hsync_strobe();
}

static void start_dmal(void)
{
	dmal_shifter |= 2;
}

static void shift_dmal(void)
{
	dmal_shifter <<= 1;
}
static void handle_dmal(void)
{
	if (!dmal_shifter) {
		return;
	}

	if (agnus_hpos & 1) {
		if (!custom_disabled && !agnus_vb_active && !agnus_bsvb && (dmal_shifter & (DMAL_SPR0A | DMAL_SPR1A | DMAL_SPR2A | DMAL_SPR3A | DMAL_SPR4A | DMAL_SPR5A | DMAL_SPR6A | DMAL_SPR7A |
			DMAL_SPR0B | DMAL_SPR1B | DMAL_SPR2B | DMAL_SPR3B | DMAL_SPR4B | DMAL_SPR5B | DMAL_SPR6B | DMAL_SPR7B))) {
			for (int nr = 0; nr < 8; nr++) {
				if (dmal_shifter & (DMAL_SPR0A << (nr * 2))) {
					generate_sprites(nr, 0);
				}
				if (dmal_shifter & (DMAL_SPR0A << (nr * 2 + 1))) {
					generate_sprites(nr, 2);
				}
			}
		}
	}

	if (agnus_hpos & 1) {
		return;
	}

	if (dmaen(DMA_AUD0 | DMA_AUD1 | DMA_AUD2 | DMA_AUD3) && ((dmal >> (2 * 3)) & 255) && (dmal_shifter & (DMAL_AUD0 | DMAL_AUD1 | DMAL_AUD2 | DMAL_AUD3))) {
		for (int nr = 0; nr < 4; nr++) {
			if (dmal_shifter & (DMAL_AUD0 << nr)) {
				uae_u32 dmalbits = (dmal >> ((3 + nr) * 2)) & 3;
				if (dmalbits) {
					uaecptr *pt = audio_getpt(nr); //, (dmalbits & 1) != 0);
					struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_AUDIO, 0xaa + nr * 16, pt);
					rga->auddat =  dmalbits | (((3 + nr) * 2) << 8);
				}
			}
		}
	}

	if (dmaen(DMA_DISK) && (((dmal >> 0) & 63) && (dmal_shifter & (DMAL_DSK0 | DMAL_DSK1 | DMAL_DSK2)))) {
		for (int nr = 0; nr < 3; nr++) {
			if (dmal_shifter & (DMAL_DSK0 << nr)) {
				uae_u32 dmalbits = (dmal >> (0 + nr * 2)) & 3;
				int w = (dmalbits & 3) == 3;
				if (dmalbits) {
					uaecptr *pt = disk_getpt();
					struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_DISK, w ? 0x26 : 0x08, pt);
					rga->dskdat = nr;
				}
			}
		}
	}

	if (dmal_shifter & (DMAL_REFRESH0 | DMAL_REFRESH1 | DMAL_REFRESH2 | DMAL_REFRESH3)) {
		if (dmal_shifter & DMAL_REFRESH0) {
			uae_u16 reg = get_strobe_reg(0);
			refptr &= refmask;
			struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_STROBE, reg, &refptr);
			rga->refdat = 0;
		}
		if (dmal_shifter & DMAL_REFRESH1) {
			uae_u16 reg = get_strobe_reg(1);
			refptr &= refmask;
			struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_REFRESH, reg, &refptr);
			rga->refdat = 1;
		}
		if (dmal_shifter & DMAL_REFRESH2) {
			refptr &= refmask;
			struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_REFRESH, 0x1fe, &refptr);
			rga->refdat = 2;
		}
		if (dmal_shifter & DMAL_REFRESH3) {
			refptr &= refmask;
			struct rgabuf *rga = write_rga(RGA_SLOT_IN, CYCLE_REFRESH, 0x1fe, &refptr);
			rga->refdat = 3;
		}
	}
}

static void check_vidsyncs(void)
{
	uae_u32 flags = DENISE_RGA_FLAG_SYNC;
	if (ecs_denise && !aga_mode) {
		flags |= DENISE_RGA_FLAG_BLANKEN_CSYNC;
	}
	if (ecs_denise && !aga_mode && (beamcon0 & BEAMCON0_BLANKEN)) {
		// BLANKEN: programmed VB and HB -> CSYNC
		bool on = agnus_phblank || agnus_pvb;
		// VARCSYEN: both VARCSY and BLANKEN must be active.
		if (beamcon0 & BEAMCON0_VARCSYEN) {
			if (!agnus_pcsync) {
				on = false;
			} else {
				flags |= DENISE_RGA_FLAG_CSYNC;
			}
		}
		flags |= on ? DENISE_RGA_FLAG_BLANKEN_CSYNC_ON : 0;
	} else if ((beamcon0 & BEAMCON0_VARCSYEN)) {
		// VARCSYEN
		bool on = agnus_pcsync;
		flags |= on ? (DENISE_RGA_FLAG_BLANKEN_CSYNC_ON | DENISE_RGA_FLAG_CSYNC) : 0;
	} else {
		bool on = agnus_csync;
		flags |= on ? (DENISE_RGA_FLAG_BLANKEN_CSYNC_ON | DENISE_RGA_FLAG_CSYNC) : 0;
	}
	if (beamcon0 & BEAMCON0_VARVSYEN) {
		flags |= agnus_pvsync ? DENISE_RGA_FLAG_VSYNC : 0;
	} else {
		flags |= agnus_vsync ? DENISE_RGA_FLAG_VSYNC : 0;
	}
	if (beamcon0 & BEAMCON0_VARHSYEN) {
		flags |= agnus_phsync ? DENISE_RGA_FLAG_HSYNC : 0;
	} else {
		flags |= agnus_hsync ? DENISE_RGA_FLAG_HSYNC : 0;
	}
	if (beamcon0 & BEAMCON0_CSYTRUE) {
		flags ^= DENISE_RGA_FLAG_CSYNC;
	}
	if (beamcon0 & BEAMCON0_VSYTRUE) {
		flags ^= DENISE_RGA_FLAG_VSYNC;
	}
	if (beamcon0 & BEAMCON0_HSYTRUE) {
		flags ^= DENISE_RGA_FLAG_HSYNC;
	}
	write_drga_flag(flags, DENISE_RGA_FLAG_SYNC | DENISE_RGA_FLAG_CSYNC | DENISE_RGA_FLAG_VSYNC | DENISE_RGA_FLAG_HSYNC |
		DENISE_RGA_FLAG_BLANKEN_CSYNC_ON | DENISE_RGA_FLAG_BLANKEN_CSYNC);
}

static void update_fast_vb(void)
{
	vb_fast = get_strobe_reg(0) != 0x3c;
}

static void check_vsyncs_fast(void)
{
	if (agnus_vb == 2) {
		agnus_vb = 1;
		update_agnus_vb();
	}
	// P_VE
	if (ecs_agnus) {
		if (agnus_bsvb) {
			agnus_p_ve = true;
		}
		if ((vpos == 9 && lof_store && !beamcon0_pal) || (vpos == 8) || (vpos == 7 && lof_store && beamcon0_pal) || agnus_equdis) {
			agnus_p_ve = false;
		}
	}
	// VE
	if (vpos == 0) {
		agnus_ve = true;
	}
	if ((vpos == 9 && lof_store && !beamcon0_pal) || (vpos == 8) || (vpos == 7 && lof_store && beamcon0_pal) || agnus_equdis) {
		agnus_ve = false;
	}
	// VSYNC
	if (vpos == 3 && lof_store) {
		agnus_vsync = true;
		lof_detect = 1;
		update_lof_detect();
	}
	if (vpos == 5 && !lof_store) {
		agnus_vsync = false;
	}
	if (vpos == 2 && !lof_store) {
		agnus_vsync = true;
		lof_detect = 0;
		update_lof_detect();
	}
	if (vpos == 5 && lof_store) {
		agnus_vsync = false;
	}

	// Programmed VSYNC
	if (programmed_register_accessed_v) {
		if (!lof_store && vpos == vsstrt) {
			agnus_pvsync = true;
			lof_pdetect = 0;
		}
		if (!lof_store && vpos == vsstop) {
			agnus_pvsync = false;
		}
		if (lof_store && vpos == vsstrt) {
			agnus_pvsync = true;
			lof_pdetect = 1;
		}
		if (lof_store && vpos == vsstop) {
			agnus_pvsync = false;
		}
		if (vpos == vbstrt) {
			agnus_pvb = true;
			agnus_pvb_start_line = true;
			update_agnus_vb();
			update_lof_detect();
		} else if (agnus_pvb_start_line) {
			agnus_pvb_start_line = false;
			update_agnus_vb();
		}
		if (vpos == vbstop) {
			agnus_pvb = false;
			agnus_pvb_end_line = true;
			update_agnus_vb();
		} else if (agnus_pvb_end_line) {
			agnus_pvb_end_line = false;
			update_agnus_vb();
		}
	}

	if (programmed_register_accessed_h) {
		if (hcenter < maxhpos) {
			if (lof_store && vpos == vsstrt) {
				agnus_pvsync = true;
				lof_pdetect = 1;
			}
			if (lof_store && vpos == vsstop) {
				agnus_pvsync = false;
			}
		}
	}

	check_vidsyncs();
	update_fast_vb();
}

static void check_vsyncs(void)
{
	bool pal = beamcon0_pal;

	agnus_bsvb_prev = agnus_bsvb;
	agnus_bsvb = false;
	need_vdiw_check = true;

	if ((agnusa1000 && vpos == 0) || (!agnusa1000 && vpos == maxvpos + lof_store - 1)) {
		agnus_bsvb = true;
		agnus_vb = 2;
		agnus_vb_start_line = true;
		update_agnus_vb();
	}

	if (programmed_register_accessed_v) {
		if (vpos == vbstrt) {
			agnus_pvb = true;
			agnus_pvb_start_line = true;
			update_agnus_vb();
			update_lof_detect();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_PRG_VB, true);
			}
#endif
		} else if (agnus_pvb_start_line) {
			agnus_pvb_start_line = false;
			update_agnus_vb();
		}
		if (vpos == vbstop) {
			agnus_pvb = false;
			agnus_pvb_end_line = true;
			update_agnus_vb();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_PRG_VB, false);
			}
#endif
		} else if (agnus_pvb_end_line) {
			agnus_pvb_end_line = false;
			update_agnus_vb();
		}
		if (vpos == bplhstop) {
			uhres_bpl = false;
		}
		if (vpos == bplhstrt) {
			uhres_bpl = true;
		}
		if (vpos == sprhstop) {
			uhres_spr = false;
		}
		if (vpos == sprhstrt) {
			uhres_spr = true;
		}
	}

	if (pal) {

		if (agnus_bsvb_prev) {
			agnus_equzone = true;
		}

		// A1000 PAL Agnus has only PAL line count and VB end
		// All others use NTSC timings.
		if (agnusa1000) {
			if (vpos == 9) {
				agnus_equzone = false;
			}
		} else {
			if (vpos == 7 && !lof_store) {
				agnus_equzone = false;
			}
			if (vpos == 8 && lof_store) {
				agnus_equzone = false;
			}
		}
		
		// VB end
		if (vpos == 25) {
			agnus_vb = 0;
			agnus_vb_end_line = true;
			update_agnus_vb();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_HW_VB, false);
			}
#endif
		} else if (agnus_vb_end_line) {
			agnus_vb_end_line = false;
			update_agnus_vb();
		}

	} else {

		if (agnus_bsvb_prev) {
			agnus_equzone = true;
		}
		if (vpos == 9) {
			agnus_equzone = false;
		}

		// VB end
		if (vpos == 20) {
			agnus_vb = 0;
			agnus_vb_end_line = true;
			update_agnus_vb();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_PRG_VB, false);
			}
#endif
		} else if (agnus_vb_end_line) {
			agnus_vb_end_line = false;
			update_agnus_vb();
		}
	}
}

static void do_scandouble(void)
{
	if (linear_vpos >= MAX_SCANDOUBLED_LINES) {
		return;
	}
	int l = lof_store;
	int vp = linear_vpos;
	struct rgabuf rga = { 0 };
	for (int i = 0; i < rga_denise_cycle_count_end; i++) {
		int idx = (i + rga_denise_cycle_start) & DENISE_RGA_SLOT_MASK;
		struct denise_rga *rd = &rga_denise[idx];
		if (rd->rga >= 0x110 && rd->rga < 0x120) {
			int plane = (rd->rga - 0x110) / 2;
			uaecptr l1 = scandoubled_bpl_ptr[vp][l][plane];
			uaecptr l2 = scandoubled_bpl_ptr[vp][l ^ 1][plane];
			rga.pv = rd->pt - l1 + l2;
			if (fetchmode_fmode_bpl == 3) {
				rd->v64 = fetch64(&rga);
			} else if (fetchmode_fmode_bpl > 0) {
				rd->v = fetch32_bpl(&rga);
			} else {
				rd->v = fetch16(&rga);
			}
		}
	}
}

static void next_denise_rga(void)
{
	rga_denise_cycle_start = rga_denise_cycle;
	rga_denise_cycle_count_end = 0;
	rga_denise_cycle_count_start = 0;
#if 0
	for (int i = 0; i <= 5; i++) {
		struct denise_rga *rga = &rga_denise[(rga_denise_cycle - i) & DENISE_RGA_SLOT_MASK];
		if (rga->line == rga_denise_cycle_line && rga->rga != 0x1fe) {
			fast_mode_ce_not = true;
		}
		rga->line++;
	}
#endif
	rga_denise_cycle_line++;
}

static void decide_line_end(void)
{
	linear_hpos_prev[2] = linear_hpos_prev[1];
	linear_hpos_prev[1] = linear_hpos_prev[0];
	linear_hpos_prev[0] = linear_hpos;
	linear_hpos = 0;
	hautoscale_check();
	display_hstart_cyclewait_cnt = display_hstart_cyclewait;
	if (currprefs.display_calibration) {
		display_hstart_cyclewait_cnt = 4;
	}
	display_hstart_cyclewait_start = false;
}

static int getlinetype(void)
{
	int type = 0;

	if (vb_fast || nosignal_status) {
		type = LINETYPE_BLANK;
	} else if (vdiwstate == diw_states::DIW_waiting_start || GET_PLANES(bplcon0) == 0 || !dmaen(DMA_BITPLANE)) {
		type = LINETYPE_BORDER;
	} else if (ddfstop > ddfstrt && ddfstrt >= 0x14 && GET_RES_AGNUS(bplcon0) == GET_RES_DENISE(bplcon0) && dmaen(DMA_BITPLANE)) {
		type = LINETYPE_BPL;
	}
	return type;
}

static int getcolorcount(int planes)
{
	bool ham = (bplcon0 & 0x800) != 0;
	bool dpf = (bplcon0 & 0x400) != 0;
	bool shres = (bplcon0 & 0x40) != 0;
	if (aga_mode) {
		int num = 1 << planes;
		if (ham) {
			if (planes < 8) {
				num = 16;
			} else {
				num = 64;
			}
		} else if (dpf) {
			int pf2of2 = (bplcon3 >> 10) & 7;
			if (!pf2of2) {
				num = 16;
			} else {
				num = (1 << pf2of2) + 16;
			}
		}
		num += bplcon4 >> 8;
		if (num > 256) {
			num = 256;
		}
		return num;
	}
	if (shres) {
		return 32;
	}
	if (ham) {
		return 16;
	}
	if (dpf) {
		return 16;
	}
	if (planes > 5) {
		planes = 5;
	}
	return 1 << planes;
}

static int getbplmod(int plane)
{
	uae_s16 mod;
	if (fmode & 0x4000) {
		if (((diwstrt >> 8) ^ (vpos ^ 1)) & 1) {
			mod = bpl1mod;
		} else {
			mod = bpl2mod;
		}
	} else {
		mod = (plane & 1) ? bpl2mod : bpl1mod;
	}
	return mod;
}

static int checkprevfieldlinestateequalbpl(struct linestate *l)
{
	if (l->bplcon0 == bplcon0 &&
		l->bplcon2 == bplcon2 &&
		l->ddfstrt == ddfstrt &&
		l->ddfstop == ddfstop &&
		l->diwstrt == diwstrt &&
		l->diwstop == diwstop &&
		l->bplcon3 == bplcon3 &&
		l->bplcon4 == bplcon4 &&
		l->diwhigh == diwhigh &&
		l->fmode == fmode &&
		l->bpllen > 0)
	{
		return -1;
	}
	return 0;
}

// draw border line quickly (no copper, no sprites, no weird things, normal mode)
static bool draw_border_fast(struct linestate *l, int ldv)
{
	if (l->hbstrt_offset < 0 || l->hbstop_offset < 0) {
		return false;
	}
	bool brdblank = (bplcon0 & 1) && (bplcon3 & 0x20);
	l->color0 = aga_mode ? agnus_colors.color_regs_aga[0] : agnus_colors.color_regs_ecs[0];
	l->brdblank = brdblank;
	int dvp = calculate_linetype(ldv);
	draw_denise_border_line_fast_queue(dvp, nextline_how, l);
	return true;
}

// draw bitplane line quickly (no copper, no sprites, no weird things, normal mode)
static bool draw_line_fast(struct linestate *l, int ldv, uaecptr bplptp[8], bool addbpl)
{
	int dvp = calculate_linetype(ldv);
	if (flickerfix_line_no_draw(dvp)) {
		return draw_border_fast(l, ldv);
	}

	if (l->hbstrt_offset < 0 || l->hbstop_offset < 0) {
		return false;
	}
	if (l->bpl1dat_trigger_offset < 0) {
		return false;
	}
	// no HAM+DPF
	if ((l->bplcon0 & (0x800 | 0x400)) == (0x800 | 0x400)) {
		return false;
	}
	int planes = GET_PLANES(l->bplcon0);
	int bc1mask = aga_mode ? 0xffff : 0x00ff;
	// no odd/even scroll if not DPF
	if (!(l->bplcon0 & 0x400) && ((l->bplcon1 & bc1mask) & 0x0f0f) != (((l->bplcon1 & bc1mask) >> 4) & 0x0f0f)) {
		return false;
	}
	int colors = getcolorcount(planes);
	int len = l->bpllen;
	for (int i = 0; i < planes; i++) {
		uaecptr pt = bplptp[i];
		if (!currprefs.z3chipmem.size) {
			pt &= chipmem_bank.mask;
		}
		if (!valid_address(pt, len)) {
			return false;
		}
		l->bplpt[i] = get_real_address(pt);
	}
	if (color_table_changed) {
		draw_denise_line_queue_flush();
		color_table_index++;
		if (color_table_index >= COLOR_TABLE_ENTRIES) {
			color_table_index = 0;
		}
		l->linecolorstate = color_tables + color_table_index * 512 * sizeof(uae_u32);
		uae_u8 *dpt = l->linecolorstate;
		memcpy(dpt, agnus_colors.acolors, colors * sizeof(uae_u32));
		dpt += 256 * sizeof(uae_u32);
		if (aga_mode) {
			memcpy(dpt, agnus_colors.color_regs_aga, colors * sizeof(uae_u32));
		} else {
			memcpy(dpt, agnus_colors.color_regs_ecs, colors * sizeof(uae_u16));
		}
		color_table_changed = false;
	} else {
		l->linecolorstate = color_tables + color_table_index * 512 * sizeof(uae_u32);
	}

	l->color0 = aga_mode ? agnus_colors.color_regs_aga[0] : agnus_colors.color_regs_ecs[0];
	l->bplcon1 = bplcon1 & bc1mask;
	l->fetchmode_size = fetchmode_size;
	l->fetchstart_mask = fetchstart_mask;
	draw_denise_bitplane_line_fast_queue(dvp, nextline_how, l);
	if (addbpl) {
		// advance bpl pointers
		for (int i = 0; i < planes; i++) {
			int mod = getbplmod(i);
			scandoubled_bpl_ptr[linear_vpos][lof_store][i] = bplpt[i];
			bplpt[i] += mod + len;
		}
	}
	return true;
}

static bool draw_always(void)
{
	if (nextline_how == nln_lower_black_always || nextline_how == nln_upper_black_always) {
		return true;
	}
	if (lineoptimizations_draw_always) {
		return true;
	}
	return false;
}

static void resetlinestate(void)
{
	int lvpos = linear_vpos + 1;
	if (lvpos >= MAX_SCANDOUBLED_LINES) {
		return;
	}
	struct linestate *l = &lines[lvpos][lof_display];
	l->type = 0;
	l->cnt = displayresetcnt - 1;
}

#define MAX_STORED_BPL_DATA 204

static void storelinestate(void)
{
	int lvpos = linear_vpos + 1;
	if (lvpos >= MAX_SCANDOUBLED_LINES) {
		return;
	}
	struct linestate *l = &lines[lvpos][lof_display];

	l->type = getlinetype();
	if (!l->type) {
		l->cnt = displayresetcnt - 1;
		return;
	}
	int bc1mask = aga_mode ? 0xffff : 0x00ff;
	l->bpllen = -1;
	l->cnt = displayresetcnt;
	l->bplcon0 = bplcon0;
	l->bplcon1 = bplcon1 & bc1mask;
	l->bplcon2 = bplcon2;
	l->ddfstrt = ddfstrt;
	l->ddfstop = ddfstop;
	l->diwstrt = diwstrt;
	l->diwstop = diwstop;
	bool brdblank = (bplcon0 & 1) && (bplcon3 & 0x20);
	l->color0 = aga_mode ? agnus_colors.color_regs_aga[0] : agnus_colors.color_regs_ecs[0];
	l->brdblank = brdblank;

	l->bplcon3 = bplcon3;
	l->bplcon4 = bplcon4;
	l->diwhigh = diwhigh;
	l->fmode = fmode;

	if (l->type == LINETYPE_BPL) {
		int stop = !harddis_h && ddfstop > 0xd8 ? 0xd8 : ddfstop;
		int len = ((stop - ddfstrt) + fetchunit - 1) / fetchunit + 1;
		len = len * fetchunit / fetchstart;
		len <<= 1;
		if (len < MAX_STORED_BPL_DATA) {
			len <<= fetchmode;
			l->bpllen = len;
		}
	}
}

static bool checkprevfieldlinestateequal(void)
{
	int lvpos = linear_vpos + 1;
	if (lvpos >= MAX_SCANDOUBLED_LINES + 1 || linear_display_vpos + 1 >= MAX_SCANDOUBLED_LINES) {
		return false;
	}
	bool ret = false;
	bool always = draw_always();
	struct linestate *l = &lines[lvpos][lof_display];

	int type = getlinetype();
	if (type && type == l->type && displayresetcnt == l->cnt) {
		if (type == LINETYPE_BLANK) {
			if (1) {
				ret = true;
			}
		} else if (type == LINETYPE_BORDER) {
			if (1) {
				bool brdblank = (bplcon0 & 1) && (bplcon3 & 0x20);
				uae_u32 c = aga_mode ? agnus_colors.color_regs_aga[0] : agnus_colors.color_regs_ecs[0];
				if (!always && c == l->color0 && brdblank == l->brdblank) {
					ret = true;
				} else if (always || currprefs.cs_optimizations == DISPLAY_OPTIMIZATIONS_FULL) {
					storelinestate();
					ret = draw_border_fast(l, linear_display_vpos + 1);
				}
			}
		} else if (type == LINETYPE_BPL && !l->blankedline) {
			if (1) {
				int r = checkprevfieldlinestateequalbpl(l);
				if ((r && always) || (r && currprefs.cs_optimizations == DISPLAY_OPTIMIZATIONS_FULL)) {
					// no match but same parameters: do quick BPL emulation
					storelinestate();
					int planes = GET_PLANES(l->bplcon0);

					// first line after accurate -> fast switch
					if (doflickerfix_active() && custom_fastmode == 1 && lvpos >= 2 && scandoubled_bpl_ena[lvpos]) {
						scandoubled_line = 1;
						lof_display ^= 1;
						int lvpos2 = lvpos - 1;
						struct linestate *l2 = &lines[lvpos2][lof_display];
						uaecptr bplptx[MAX_PLANES];
						bool skip = false;
						for (int i = 0; i < planes; i++) {
							uaecptr li1 = scandoubled_bpl_ptr[lvpos2][lof_store][i];
							uaecptr li2 = scandoubled_bpl_ptr[lvpos2][lof_store ^ 1][i];
							skip = !li1 || !li2 || li1 == 0xffffffff || li2 == 0xffffffff;
							bplptx[i] = scandoubled_bpl_ptr[lvpos2 - 1][lof_store][i] - li1 + li2;
						}
						if (skip) {
							draw_border_fast(l2, linear_display_vpos + 0);
						} else {
							draw_line_fast(l2, linear_display_vpos + 0, bplptx, false);
						}
						lof_display ^= 1;
						scandoubled_line = 0;
					}

					r = draw_line_fast(l, linear_display_vpos + 1, bplpt, true);
					if (doflickerfix_active()) {
						if (scandoubled_bpl_ena[lvpos]) {
							scandoubled_line = 1;
							lof_display ^= 1;
							struct linestate *l2 = &lines[lvpos][lof_display];
							uaecptr bplptx[MAX_PLANES];
							bool skip = false;
							for (int i = 0; i < planes; i++) {
								uaecptr li1 = scandoubled_bpl_ptr[lvpos][lof_store][i];
								uaecptr li2 = scandoubled_bpl_ptr[lvpos][lof_store ^ 1][i];
								skip = !li1 || !li2 || li1 == 0xffffffff || li2 == 0xffffffff;
								bplptx[i] = bplpt[i] - li1 + li2;
							}
							if (skip) {
								draw_border_fast(l2, linear_display_vpos + 2);
							} else {
								draw_line_fast(l2, linear_display_vpos + 2, bplptx, false);
							}
							lof_display ^= 1;
							scandoubled_line = 0;
						}
					}
				}
				ret = r > 0;
			}
		}
	}
	return ret;
}

static void draw_line(int ldvpos, bool finalseg)
{
	int dvp = calculate_linetype(ldvpos);
	int wclks = draw_line_wclks;
	int maxv = maxvpos_display + maxvpos_display_vsync - vsync_startline + lof_store;
	if (ldvpos >= maxv || ldvpos + 1 < minfirstline - vsync_startline) {
		wclks = -1;
	}
	if (flickerfix_line_no_draw(dvp)) {
		wclks = -1;
	}

	struct linestate *l = NULL;
	if (linear_vpos < MAX_SCANDOUBLED_LINES) {
		l = &lines[linear_vpos][lof_display];
	}

	int cs = 0;// (beamcon0 & BEAMCON0_VARHSYEN) ? agnus_phsync_end - agnus_phsync_start : agnus_hsync_end - agnus_hsync_start;
	int cslen = 10;
	draw_denise_line_queue(dvp, nextline_how, rga_denise_cycle_line, rga_denise_cycle_start, rga_denise_cycle, rga_denise_cycle_count_start, rga_denise_cycle_count_end,
		display_hstart_cyclewait_skip, display_hstart_cyclewait_skip2,
		wclks, cs, cslen, lof_store, lol, display_hstart_fastmode - display_hstart_cyclewait, nosignal_status != 0, finalseg, l);
	rga_denise_cycle_count_start = rga_denise_cycle_count_end;
}

static void dmal_fast(void)
{
	process_dmal(0);
	for (int nr = 0; nr < 4; nr++) {
		uae_u32 dmalbits = (dmal >> ((3 + nr) * 2)) & 3;
		if (dmalbits) {
			struct rgabuf r = { 0 };
			r.p = audio_getpt(nr);
			r.pv = *r.p;
			dmal_emu_audio(&r, nr);
			r.pv += 2;
			if (dmalbits & 1) {
				r.pv = audio_getloadpt(nr);
			}
			*r.p = r.pv;
		}
	}
	for (int nr = 0; nr < 3; nr++) {
		uae_u32 dmalbits = (dmal >> (0 + nr * 2)) & 3;
		int w = (dmalbits & 3) == 3;
		if (dmalbits) {
			struct rgabuf r = { 0 };
			r.p = disk_getpt();
			r.pv = *r.p;
			dmal_emu_disk(&r, nr, w);
			r.pv += 2;
			*r.p = r.pv;
		}
	}
}

static void do_draw_line(void)
{
	start_draw_denise();

	if (custom_fastmode_exit) {
		custom_fastmode_exit = 0;
		quick_denise_rga_queue(rga_denise_cycle_line, rga_denise_cycle_start, rga_denise_cycle);
		next_denise_rga();
		decide_line_end();
		return;
	}
	if (!custom_disabled) {
		draw_line_wclks = linear_hpos - (display_hstart_cyclewait_skip - display_hstart_cyclewait_skip2);
		if (custom_fastmode >= 0) {
			if (doflickerfix_active() && scandoubled_bpl_ena[linear_vpos]) {
				denise_store_restore_registers_queue(true, rga_denise_cycle_line);
				draw_line(linear_display_vpos, true);
				draw_denise_line_queue_flush();
				do_scandouble();
				denise_store_restore_registers_queue(false, rga_denise_cycle_line);
				lof_display ^= 1;
				scandoubled_line = 1;
				rga_denise_cycle_count_start = 0;
				draw_line(linear_display_vpos, true);
				scandoubled_line = 0;
				lof_display ^= 1;
			} else {
				draw_line(linear_display_vpos, true);
			}
		}

		next_denise_rga();

	}

	decide_line_end();
}

static void decide_hsync(void)
{
	if (display_hstart_cyclewait_start) {
		if (display_hstart_cyclewait_cnt > 0) {
			display_hstart_cyclewait_cnt--;
		} else {
			display_hstart_fastmode = agnus_hpos;
			hdisplay_left_border = (get_cck_cycles() - agnus_trigger_cck) - REFRESH_FIRST_HPOS + display_hstart_cyclewait_skip;
			do_draw_line();
			draw_line_next_line = 1;
		}
	}
}

static void handle_pipelined_write(void)
{
	if (pipelined_write_addr == 0x1fe) {
		return;
	}
	custom_wput_1(pipelined_write_addr, pipelined_write_value, 1 | 0x8000);
	pipelined_write_addr = 0x1fe;
}

#if 0
static bool can_fast_copper(void)
{
	// if copper is waiting, wake up is inside blank/border and
	// it is followed by only writes to color registers:
	// emulate it "immediately"
	int h = cop_state.hcmp;
	if (cop_state.state == COP_wait1 && cop_state.ir[1] == 0xfffe) {
		uaecptr ip = cop_state.ip;
		int h = cop_state.hcmp + 4;
		bool ok = true;
		int cnt = 0;
		bool vb = get_strobe_reg(0) != 0x3c;
		while (ok && vb && vdiwstate == diw_states::DIW_waiting_start && cop_state.state == COP_read1) {
			h += 4;
			if (h >= maxhpos) {
				break;
			}
			uae_u16 ir1 = chipmem_wget_indirect(ip);
			uae_u16 ir2 = chipmem_wget_indirect(ip + 2);
			if (!(ir1 & 1)) {
				// MOVE
				ir1 &= 0x1fe;
				if (vb) {
					if (ir1 == 0x106) {
						// BPLCON3 allowed but only safe bits
						if ((bplcon3 ^ ir2) & 0x003f) {
							ok = false;
							break;
						}
					} else if (ir1 < 0x180 || ir1 >= 0x180 + 32 * 2) {
						// not color register?
						ok = false;
						break;
					} else if (ir1 == 0x8a) {
						h += 4;
						cop_state.ip = cop2lc;
						continue;
					} else if (ir1 == 0x88) {
						h += 4;
						cop_state.ip = cop1lc;
						continue;
					}
					custom_wput_copper(cop_state.ip, ir1, ir2, 0);
				}
			} else if ((ir2 & 1) && (ir2 & 1)) {
				// SKIP
				ok = false;
				break;
			} else {
				// WAIT
				if (ir2 != 0xfffe) {
					ok = false;
					break;
				}
				int vp = (ir1 >> 8) & 0x7F;
				if (vp > vpos) {
					cop_state.ip = ip;
					cop_state.ir[0] = ir1;
					cop_state.ir[1] = ir2;
					cop_state.vcmp = (cop_state.ir[0] & (cop_state.ir[1] | 0x8000)) >> 8;
					cop_state.hcmp = (cop_state.ir[0] & cop_state.ir[1] & 0xFE);
					break;
				}
			}
			ip += 4;
		}
		if (ok) {
			return true;
		}
	}
	return false;
}
#endif

static int can_fast_custom(void)
{
	if (currprefs.cs_optimizations >= DISPLAY_OPTIMIZATIONS_NONE) {
		return 0;
	}
	if (not_safe_mode || syncs_stopped || agnus_pos_change > -2) {
		return 0;
	}
	compute_spcflag_copper();
	if (copper_enabled_thisline) {
#if 0
		if (!can_fast_copper()) {
			return 0;
		}
#else
		return 0;
#endif
	}
	if (!display_hstart_fastmode) {
		return 0;
	}
	if (dmaen(DMA_SPRITE)) {
		if (agnus_vb_active_end_line) {
			return 0;
		}
		int type = getlinetype();
		if (type != LINETYPE_BLANK) {
			for (int i = 0; i < MAX_SPRITES; i++) {
				struct sprite *s = &spr[i];
				if (s->vstart == vpos || s->vstop == vpos) {
					return 0;
				}
				if (s->dmastate) {
					return 0;
				}
			}
		}
	}
	if (0 || 1)
		return 1;
	return 0;
}

static void do_imm_dmal(void)
{
	process_dmal(0);
	uae_u32 dmal_d = dmal;

	// "immediate" audio DMA
	if (dmaen(DMA_AUD0 | DMA_AUD1 | DMA_AUD2 | DMA_AUD3) && ((dmal_d >> (3 * 2)) & 255)) {
		for (int nr = 0; nr < 4; nr++) {
			uae_u32 dmalbits = (dmal_d >> ((3 + nr) * 2)) & 3;
			if (dmalbits) {
				struct rgabuf r = { 0 };
				uaecptr *pt = audio_getpt(nr);
				r.pv = *pt;
				dmal_emu_audio(&r, nr);
				*pt += 2;
				if (dmalbits & 1) {
					*pt = audio_getloadpt(nr);
				}
			}
		}
	}

	// "immediate" disk DMA
	if (dmaen(DMA_DISK) && (((dmal_d >> 0) & 63))) {
		for (int nr = 0; nr < 3; nr++) {
			uae_u32 dmalbits = (dmal_d >> (0 + nr * 2)) & 3;
			if (dmalbits) {
				int w = (dmalbits & 3) == 3;
				struct rgabuf r = { 0 };
				uaecptr *pt = disk_getpt();
				r.pv = *pt;
				dmal_emu_disk(&r, nr, w);
				*pt += 2;
			}
		}
	}

	dmal = 0;
	dmal_shifter = 0;
}

static void sync_equalline_handler(void);
static void start_sync_equalline_handler(void)
{
	eventtab[ev_sync].active = 1;
	eventtab[ev_sync].oldcycles = get_cycles();
	eventtab[ev_sync].evtime = get_cycles() + maxhpos * CYCLE_UNIT;
	eventtab[ev_sync].handler = sync_equalline_handler;
	events_schedule();
}
static void sync_imm_evhandler(void);
static void start_sync_imm_handler(void)
{
	eventtab[ev_sync].active = 1;
	eventtab[ev_sync].oldcycles = get_cycles();
	eventtab[ev_sync].evtime = get_cycles() + maxhpos * CYCLE_UNIT;
	eventtab[ev_sync].handler = sync_imm_evhandler;
	events_schedule();
}


static void vsync_nosync(void)
{
	nosignal_trigger = true;
	linear_vpos = 0;
	vsync_handler_post();
	devices_vsync_pre();
	inputdevice_read_msg(true);
	vsync_display_render();
	vsync_display_rendered = false;
	virtual_vsync_check();
	uae_quit_check();
}

static void custom_trigger_start_nosync(void)
{
	linear_display_vpos = linear_vpos;
	linear_vpos++;
	linear_vpos_visible++;
	if (linear_vpos >= maxvpos + lof_store) {
		vsync_nosync();
	}
}

static void custom_trigger_start(void)
{
	if (vdiwstate == diw_states::DIW_waiting_stop && dmaen(DMA_BITPLANE)) {
		bpl_autoscale();
	}

	vpos_prev = vpos;
	vpos++;
	vpos &= 2047;
	if (!ecs_agnus) {
		vpos &= 511;
	}
	linear_display_vpos = linear_vpos;
	linear_vpos++;
	linear_vpos_visible++;
	draw_line_next_line = 0;

	linear_vpos_vsync++;
	if (beamcon0_has_vsync) {
		if (vpos == vsstrt) {
			linear_vpos_vsync = 0;
		}
	} else {
		if (beamcon0_pal && (vpos == 3 && lof_store) || (vpos == 2 && !lof_store)) {
			linear_vpos_vsync = 0;
		}
		if (!beamcon0_pal && vpos == 3) {
			linear_vpos_vsync = 0;
		}
	}

	if (linear_vpos < MAX_SCANDOUBLED_LINES) {
		current_line_state = &lines[linear_vpos][lof_display];
	}

	if (vpos == vsync_startline) {

		linear_vpos_prev[2] = linear_vpos_prev[1];
		linear_vpos_prev[1] = linear_vpos_prev[0];
		linear_vpos_prev[0] = linear_vpos;
		linear_vpos = 0;

		virtual_vsync_check();

		last_vsync_evt = get_cycles() + (maxvpos * maxhpos * 3) * CYCLE_UNIT;
	}

	bool vposzero = false;
	// LOF=1 is always matched, even when LOF=0 but only in PAL/NTSC modes
	if ((vpos == maxvpos + lof_store) || (vpos == maxvpos_long)) {
		vpos = 0;
		check_vsyncs();

		hsync_handler(true);
		vsync_handler_post();

		vsync_counter++;
		COPJMP(1, 1);

		if (bplcon0 & 4) {
			lof_store = lof_store ? 0 : 1;
		}
		if ((bplcon0 & 2) && currprefs.genlock) {
			genlockvtoggle = lof_store ? 1 : 0;
		}

		vposzero = true;

	} else if (agnus_vb_start_line) {

		agnus_vb_start_line = false;
		update_agnus_vb();
	}

	if (!vposzero) {
		if (vpos == 0) {

			hsync_handler(true);
			vsync_handler_post();

			vsync_counter++;

		} else {

			check_vsyncs();

			hsync_handler(false);

		}
	}

	if (!(new_beamcon0 & BEAMCON0_PAL) && !(new_beamcon0 & BEAMCON0_LOLDIS)) {
		lol = lol ? false : true;
		linetoggle = true;
	} else {
		lol = false;
		linetoggle = false;
	}

	setmaxhpos();
	agnus_trigger_cck = get_cck_cycles();

	start_dmal();
	check_bpl_vdiw();

	int custom_fastmode_prev = custom_fastmode;

	if (custom_disabled) {
		if (!eventtab[ev_sync].active && !currprefs.cpu_memory_cycle_exact && currprefs.cs_optimizations < DISPLAY_OPTIMIZATIONS_NONE) {
			custom_fastmode = 0;
			start_sync_imm_handler();
			write_log("Chipset emulation inactive\n");
		}
		linear_hpos_prev[2] = linear_hpos_prev[1];
		linear_hpos_prev[1] = linear_hpos_prev[0];
		linear_hpos_prev[0] = maxhpos_short;
		linear_hpos = maxhpos_short;
	}

	if (!canvhposw()) {
		// ignore pending V(H)POSW writes if in normal mode
		if (agnus_pos_change > -2) {
			agnus_pos_change = -2;
			agnus_vpos_next = -1;
			agnus_hpos_next = -1;
		}
	}

	if (vpos == 0 && fast_lines_cnt) {
		static int pctx, pctxcnt;
		int pct = fast_lines_cnt * 100 / (maxvpos > 0 ? maxvpos : 1);
		if (pct > 100) {
			pct = 100;
		}
		pctx += pct;
		pctxcnt++;
		if (pctxcnt == 50) {
			int v = pctx / pctxcnt;
			write_log("%03d%%%%\n", v);
			pctxcnt = 0;
			pctx = 0;

		}
		fast_lines_cnt = 0;
	}

	if (!custom_disabled && !currprefs.cpu_memory_cycle_exact && currprefs.cs_optimizations < DISPLAY_OPTIMIZATIONS_NONE) {
#if 0
		if (bpl_active_this_line) {
			custom_fastmode_bplextendmask = 0;
			for (int i = 0; i < RGA_SLOT_TOTAL + 1; i++) {
				struct rgabuf *r = &rga_pipe[i];
				if (r->reg >= 0x110 && r->reg < 0x120 && r->alloc) {
					int num = (r->reg - 0x110) / 2;
					custom_fastmode_bplextendmask |= 1 << num;
				}
			}
		}
#endif
		update_fast_vb();
		int canline = can_fast_custom();
		if (canline) {
			calculate_linetype(linear_display_vpos + 1);
			bool same = checkprevfieldlinestateequal();
			if (same) {
				start_sync_equalline_handler();
				if (custom_fastmode >= 1) {
					custom_fastmode++;
				} else {
					custom_fastmode = 1;
					do_imm_dmal();
				}
			} else {
				storelinestate();
			}
		} else {
			resetlinestate();
		}
	}

	if (!eventtab[ev_sync].active) {
		custom_fastmode = 0;
	} else {
		check_vsyncs_fast();
	}

#if 0
	if (1 && !can_fast_custom() && custom_fastmode) {
		custom_fastmode = 0;
	}
#endif

}

// syncs stopped: generate fake hsyncs to keep everything running
static void fakehsync_handler(uae_u32 v)
{
	if (syncs_stopped) {
		hsync_handler(false);
		next_denise_rga();
		event2_newevent_xx(-1, CYCLE_UNIT * maxhpos, 0, fakehsync_handler);
		custom_trigger_start_nosync();
	}
}

static void set_fakehsync_handler(void)
{
	if (event2_newevent_x_exists(fakehsync_handler)) {
		return;
	}
	event2_newevent_xx(-1, CYCLE_UNIT * maxhpos, 0, fakehsync_handler);
}

static bool cck_clock;

static void get_cck_clock(void)
{
	int h = agnus_hpos + 1;
	if (h == maxhpos || h == maxhpos_long) {
		h = 0;
	}
	if (agnus_pos_change == 1 && agnus_hpos_next >= 0) {
		h = agnus_hpos_next;
	}
	cck_clock = (h & 1) != (agnus_hpos & 1);
}

static void inc_cck(void)
{
#ifdef DEBUGGER
	if (debug_dma) {
		struct dma_rec *dr = record_dma_next_cycle(agnus_hpos, vpos, linear_vpos_vsync);
		rga_denise[rga_denise_cycle].dr = dr;
		if (dr) {
			dr->cs = (beamcon0 & BEAMCON0_VARCSYEN) ? agnus_pcsync : agnus_csync;
			dr->hs = (beamcon0 & BEAMCON0_VARHSYEN) ? agnus_phsync : agnus_hsync;
			dr->vs = (beamcon0 & BEAMCON0_VARVSYEN) ? agnus_pvsync : agnus_vsync;
		}
	}
#endif

	agnus_hpos_prev = agnus_hpos;
	agnus_hpos++;
	hhpos++;
	linear_hpos++;
	currcycle_cck++;

	// must check end of line first
	if (agnus_hpos == maxhpos || agnus_hpos == maxhpos_long) {
		agnus_hpos = 0;
		if (issyncstopped(bplcon0) && !syncs_stopped) {
			if (!lol) {
				setsyncstopped();
			}
		}
	}
	if (syncs_stopped) {
		agnus_hpos = 0;
		hhpos = 0;
		linear_hpos = 0;
		dmal_shifter = 0; // fast CPU fix
		set_fakehsync_handler();
	} else {
		rga_denise_cycle++;
		rga_denise_cycle &= DENISE_RGA_SLOT_MASK;
		rga_denise_cycle_count_end++;
	}
	if (beamcon0_dual) {
		if (hhpos == maxhpos || hhpos == maxhpos_long) {
			hhpos = 0;
		}
	} else {
		hhpos = agnus_hpos;
	}

	// before VPOSW write modifications
	// (write $xxE3 to VPOSW won't match end of line)
	if (agnus_pos_change > -2) {
		agnus_pos_change--;
		if (agnus_pos_change == 0) {
			if (agnus_hpos_next >= 0) {
				if (agnus_hpos_next) {
					int hnewglitch = agnus_hpos_next & ~1;
					decide_bpl(hnewglitch);
				}
				agnus_hpos = agnus_hpos_next;
				agnus_hpos_next = -1;
			}
			if (agnus_vpos_next >= 0) {
				vpos = agnus_vpos_next;
				agnus_vpos_next = -1;
				check_vsyncs();
			}
			compute_spcflag_copper();
		}
	}

}

static void update_agnus_pcsync(int hp, bool prevsy)
{
	bool pcsync = agnus_pcsync;

	bool enable1 = false;
	if ((prevsy && hp == hbstop_cck) ||
		(prevsy && hp == hbstrt_cck) ||
		(!prevsy && hp == hsstop)) {
		enable1 = true;
	}
	bool enable2 = true;
	if ((prevsy && hp == hcenter) || (hp == hsstrt) || (agnus_pcsync)) {
		enable2 = false;
	}
	agnus_pcsync = !enable1 && !enable2;
	if (pcsync != agnus_pcsync) {
		if (agnus_pcsync) {
			agnus_phsync_start = get_cck_cycles();
		}
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_PRG_CS, agnus_pcsync);
		}
#endif
	}
}

static void check_hsyncs_hardwired(void)
{
	int hp = agnus_hpos;
	bool pal = beamcon0_pal;
	bool realpal = pal && !agnusa1000;
	int hhp = beamcon0_dual ? hhpos : agnus_hpos;

	bool is_hsstrt = hp == 18;
	bool is_hsstop = hp == 35;
	bool is_shs = hp == 18;
	bool is_rhs = hp == 35;
	bool is_cen = hp == 132;
	bool is_vr1 = hp == 9;
	bool is_vr2 = hp == 115;
	bool is_ver1 = (realpal ? hp == 26 : hp == 27);
	bool is_ver2 = (realpal ? hp == 140 : hp == 141);
	bool is_rsvs = (lof_store && is_shs) || is_cen;
	bool is_rsve_n = (lof_store && is_vr1) || is_vr2;
	bool is_rsve_p = (lof_store && is_vr2);

	if (is_hsstrt) { // HSSTRT
		agnus_hsync = true;
		agnus_hsync_start = get_cck_cycles();
		check_vidsyncs();
		if (!beamcon0_has_hsync) {
			display_hstart_cyclewait_start = true;
			write_drga_flag(DENISE_RGA_FLAG_LOL | (lol ? DENISE_RGA_FLAG_LOL_ON : 0), DENISE_RGA_FLAG_LOL | DENISE_RGA_FLAG_LOL_ON);
		}
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_HW_HS, true);
		}
#endif
	}
	if (is_hsstop) { // HSSTOP
		agnus_hsync = false;
		agnus_hsync_end = get_cck_cycles();
		check_vidsyncs();
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_HW_HS, false);
		}
#endif
	}

	if (realpal) {

		// Vertical SYNC
		if (is_shs) { // SHS
			if (vpos == 3 && lof_store) {
				agnus_vsync = true;
				lof_detect = 1;
				update_lof_detect();
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, true);
				}
#endif
			}
			if (vpos == 5 && !lof_store) {
				agnus_vsync = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, false);
				}
#endif
			}
			check_vidsyncs();
		}
		if (is_cen) { // HCENTER
			if (vpos == 2 && !lof_store) {
				agnus_vsync = true;
				lof_detect = 0;
				update_lof_detect();
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, true);
				}
#endif
			}
			if (vpos == 5 && lof_store) {
				agnus_vsync = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, false);
				}
#endif
			}
			check_vidsyncs();
		}

	} else {

		// Vertical SYNC
		if (is_shs) { // SHS
			if (vpos == 3 && lof_store) {
				agnus_vsync = true;
				lof_detect = 1;
				update_lof_detect();
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, true);
				}
#endif
			}
			if (vpos == 6 && lof_store) {
				agnus_vsync = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, false);
				}
#endif
			}
			check_vidsyncs();
		}
		if (is_cen) { // HCENTER
			if (vpos == 3 && !lof_store) {
				agnus_vsync = true;
				lof_detect = 0;
				update_lof_detect();
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, true);
				}
#endif
			}
			if (vpos == 6 && !lof_store) {
				agnus_vsync = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_HW_VS, false);
				}
#endif
			}
			check_vidsyncs();
		}
	}

	if (1) {
		bool is_ve = agnus_ve;
		bool is_hc1 = hp == 1;
		bool is_vsy = agnus_vsync;

		// CSYNC
		if (agnus_csync && (is_hc1 || is_vr2 || (is_rhs && !is_vsy) || (is_ver2 && !is_vsy) || (is_ver1 && !is_vsy && is_ve))) {
			agnus_csync = false;
			check_vidsyncs();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_HW_CS, false);
			}
#endif
		} else if (!agnus_csync && ((is_cen && is_ve) || is_shs)) {
			agnus_csync = true;
			check_vidsyncs();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_HW_CS, true);
			}
#endif
		}

		// P_VE
		if (ecs_agnus) {
			if (agnus_bsvb) {
				agnus_p_ve = true;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_P_VE, true);
				}
#endif
			}
			if ((vpos == 9 && is_rsve_n) || (vpos == 8 && is_vr1) || (vpos == 7 && is_rsve_p) || agnus_equdis) {
				agnus_p_ve = false;
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_P_VE, false);
				}
#endif
			}
		}
		// VE
		if (vpos == 0 && is_rsve_n) {
			agnus_ve = true;
			check_vidsyncs();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_VE, true);
			}
#endif
		}
		if ((vpos == 9 && is_rsve_n) || (vpos == 8 && is_vr1) || (vpos == 7 && is_rsve_p) || agnus_equdis) {
			if (agnus_ve) {
				agnus_ve = false;
				check_vidsyncs();
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_event_agnus(AGNUS_EVENT_VE, false);
				}
#endif
			}
		}
	}

	if (hp == 2) {
		if (agnus_vb > 1) {
			agnus_vb--;
			update_agnus_vb();
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_HW_VB, true);
			}
#endif
		}
	}
}

static void check_hsyncs_programmed(void)
{
	int hp = agnus_hpos;
	int hhp = beamcon0_dual ? hhpos : agnus_hpos;
	bool prevsy = agnus_pvsync;

	if (hhp == hsstrt) {
		agnus_phsync = true;
		if (!lof_store && vpos == vsstrt) {
			agnus_pvsync = true;
			lof_pdetect = 0;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_PRG_VS, true);
			}
#endif
		}
		if (!lof_store && vpos == vsstop) {
			agnus_pvsync = false;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_PRG_VS, false);
			}
#endif
		}
		if (beamcon0_has_hsync) {
			display_hstart_cyclewait_start = true;
			if (hsstrt > 8) {
				// LOL info must be send after STRLONG cycyle
				write_drga_flag(DENISE_RGA_FLAG_LOL | (lol ? DENISE_RGA_FLAG_LOL_ON : 0), DENISE_RGA_FLAG_LOL | DENISE_RGA_FLAG_LOL_ON);
			}
		}
		update_agnus_pcsync(hhp, prevsy);
		update_agnus_vb();
		check_vidsyncs();
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_PRG_HS, true);
		}
#endif
	}
	if (hhp == hsstop) {
		agnus_phsync = false;
		update_agnus_pcsync(hhp, prevsy);
		agnus_phsync_end = get_cck_cycles();
		check_vidsyncs();
		if (beamcon0_has_hsync && hsstrt <= 8) {
			// LOL info must be send after STRLONG cycyle
			write_drga_flag(DENISE_RGA_FLAG_LOL | (lol ? DENISE_RGA_FLAG_LOL_ON : 0), DENISE_RGA_FLAG_LOL | DENISE_RGA_FLAG_LOL_ON);
		}
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_PRG_HS, false);
		}
#endif
	}
	if (hhp == hbstrt_cck) {
		agnus_phblank = true;
		agnus_phblank_start = get_cck_cycles();
		update_agnus_pcsync(hhp, prevsy);
		check_vidsyncs();
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_HB, true);
		}
#endif
	}
	if (hhp == hbstop_cck) {
		agnus_phblank = false;
		agnus_phblank_end = get_cck_cycles();
		update_agnus_pcsync(hhp, prevsy);
		check_vidsyncs();
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_agnus(AGNUS_EVENT_HB, false);
		}
#endif
	}
	if (hhp == hcenter) {
		if (lof_store && vpos == vsstrt) {
			agnus_pvsync = true;
			lof_pdetect = 1;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_PRG_VS, true);
			}
#endif
		}
		if (lof_store && vpos == vsstop) {
			agnus_pvsync = false;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_agnus(AGNUS_EVENT_PRG_VS, false);
			}
#endif
		}
		update_agnus_pcsync(hhp, prevsy);
		check_vidsyncs();
	}
}

static void check_hsyncs(void)
{
	int hp = agnus_hpos;

	if (hp < HW_HPOS_TABLE_MAX && hw_hpos_table[hp]) {
		check_hsyncs_hardwired();
	}
	
	if (programmed_register_accessed_h) {
		int hhp = beamcon0_dual ? hhpos : agnus_hpos;
		if (prg_hpos_table[hhp]) {
			check_hsyncs_programmed();
		}
	}
}

static void handle_rga_out(void)
{
	if (dmal_next) {
		dmal_next = false;
		process_dmal(0);
	}

	if (check_rga_out()) {
		int hp = agnus_hpos;
		struct rgabuf *r = read_rga_out();
		bool done = false;
		bool disinc = false;

		last_rga_cycle = get_cycles();

		if (r->type & (CYCLE_REFRESH | CYCLE_STROBE)) {
			*r->p += ref_ras_add;
			*r->p &= refmask;
			refptr = *r->p;
			disinc = true;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(r->reg, *r->p, DMARECORD_REFRESH, (hp - 3) / 2);
				record_dma_read_value(0xffff);
				int num = r->refdat;
				if (num == 1 && lof_store) {
					record_dma_event(DMA_EVENT_LOF);
				}
				if (num == 1 && lol) {
					record_dma_event(DMA_EVENT_LOL);
				}
			}
#endif
			done = true;
		}

		switch (r->reg)
		{
			// STROBES
			case 0x38:
			case 0x3a:
			case 0x3c:
			{
				// VERTB = STRHOR -> !STRHOR
				if (prev_strobe == 0x3c && r->reg != 0x3c) {
					INTREQ_INT(5, 0);
				}
				prev_strobe = r->reg;
				if (!custom_disabled) {
					write_drga_strobe(r->reg);
				}
				// load DMAL in next cycle
				dmal_next = true;
				done = true;
			}
			break;
			case 0x03e: // STRLONG
			if (!custom_disabled) {
				write_drga_strobe(r->reg);
			}
			done = true;
			break;
		}

		if (r->type & CYCLE_COPPER) {
			process_copper(r);
			done = true;
		}

		// SPR
		if (r->reg >= 0x140 && r->reg < 0x180) {
			int num = r->sprdat & 7;
			bool slot = (r->sprdat & 8) != 0;
			bool dmastate = (r->sprdat & 0x10) != 0;
			struct sprite *s = &spr[num];
			uae_u16 sdat = 0;
			uaecptr pt = s->pt;

#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(r->reg, *r->p, DMARECORD_SPRITE, num);
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(*r->p, MW_MASK_SPR_0 + num, r->reg);
			}
#endif
			if (fetchmode_fmode_spr == 0) {
				uae_u16 dat = fetch16(r);
				if (!dmastate) {
					write_drga_dat_spr(r->reg, pt, dat);
				} else {
					write_drga_dat_spr(r->reg, pt, dat << 16);
				}
				sdat = dat;
			} else if (fetchmode_fmode_spr == 1) {
				uae_u32 dat = fetch32_spr(r);
				sdat = dat >> 16;
				if (!dmastate) {
					write_drga_dat_spr(r->reg, pt, sdat);
				} else {
					write_drga_dat_spr(r->reg, pt, dat);
				}
			} else {
				uae_u64 dat = fetch64(r);
				sdat = dat >> 48;
				if (!dmastate) {
					write_drga_dat_spr(r->reg, pt, sdat);
				} else {
					write_drga_dat_spr_wide(r->reg, pt, dat);
				}
			}

			if (!dmastate) {
				if (slot) {
					SPRxCTL_DMA(sdat, num);
				} else {
					SPRxPOS(sdat, num);
				}
			}
			if (!disinc) {
				r->pv += sprite_width / 8;
			}
			regs.chipset_latch_rw = sdat;
			s->pt = r->pv;
			done = true;
		} else if (r->type & CYCLE_SPRITE) {
			int num = r->sprdat & 7;
			struct sprite *s = &spr[num];
			*r->p += sprite_width / 8;
			s->pt = *r->p;
		}

		// BPL
		if (r->reg >= 0x110 && r->reg < 0x120) {

			int num = r->bpldat & 7;
			uaecptr pt = r->pv;

#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(r->reg, pt, DMARECORD_BITPLANE, num);
				if (r->bplmod) {
					record_dma_event(DMA_EVENT_MODADD);
				}
			}
			if (memwatch_enabled) {
				debug_getpeekdma_chipram(pt, MW_MASK_BPL_0 + num, r->reg);
			}
#endif
			if (!aga_mode) {
				uae_u32 dat = fetch16(r);
				write_drga_dat_bpl16(r->reg, pt, dat, num);
				regs.chipset_latch_rw = (uae_u16)dat;
			} else {
				if (fetchmode_fmode_bpl == 0) {
					uae_u32 dat = fetch16(r);
					write_drga_dat_bpl16(r->reg, pt, dat, num);
					regs.chipset_latch_rw = (uae_u16)dat;
				} else if (fetchmode_fmode_bpl == 1) {
					uae_u32 dat = fetch32_bpl(r);
					write_drga_dat_bpl32(r->reg, pt, dat, num);
					regs.chipset_latch_rw = (uae_u16)dat;
				} else {
					uae_u64 dat64 = fetch64(r);
					write_drga_dat_bpl64(r->reg, pt, dat64, num);
					regs.chipset_latch_rw = (uae_u16)dat64;
				}
			}
			if (!disinc) {
				r->pv += fetchmode_bytes + r->bplmod;
			}
			bplpt[num] = r->pv;
			done = true;
		} else if (r->type & CYCLE_BITPLANE) {
			int num = r->bpldat & 7;
			r->pv += fetchmode_bytes + r->bplmod;
			bplpt[num] = r->pv;
		}

		if (r->type & CYCLE_BLITTER) {
			process_blitter(r);
			done = true;
		}

		// AUDIO
		if (r->reg == 0xaa || r->reg == 0xba || r->reg == 0xca || r->reg == 0xda) {
			int num = (r->reg - 0xaa) / 0x10;
			uaecptr pt = r->pv;
			dmal_emu_audio(r, num);
			if (!disinc) {
				pt += 2;
			}
			if (r->auddat & 1) {
				pt = audio_getloadpt(num);
			}
			*r->p = pt;
			r->pv = pt;
			done = true;
		}

		// DISK
		if (r->reg == 0x08 || r->reg == 0x26) {
			uaecptr pt = r->pv;
			dmal_emu_disk(r, r->dskdat, r->reg == 0x26);
			if (!disinc) {
				pt += 2;
				*r->p = pt;
				r->pv = pt;
			}
			done = true;
		}

		// UHRES BPL
		if (r->reg == 0x7a) {
			uaecptr pt = r->pv;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(r->reg, pt, DMARECORD_UHRESBPL, 0);
			}
#endif
			pt += 2 + r->bplmod;
			*r->p = pt;
			hhbpl = pt;
			done = true;
		}
		// UHRES SPR
		if (r->reg == 0x78) {
			uaecptr pt = r->pv;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_read(r->reg, pt, DMARECORD_UHRESSPR, 0);
			}
#endif
			pt += 2;
			*r->p = pt;
			hhspr = pt;
			done = true;
		}

		if (!done) {
			if (r->write) {
#ifdef DEBUGGER
				debug_putpeekdma_chipram(*r->p, r->value, r->mwmask, r->reg);
#endif
				chipmem_wput_indirect(*r->p, r->value);
			} else {
#ifdef DEBUGGER
				if (debug_dma) {
					record_dma_read(r->reg, *r->p, DMARECORD_REFRESH, 0);
					record_dma_read_value(0xffff);
				}
				if (memwatch_enabled) {
					debug_getpeekdma_chipram(*r->p, r->mwmask, r->reg);
				}
#endif
			}
			regs.chipset_latch_rw = r->value;
		}

#ifdef DEBUGGER
		if (r->conflict) {
			record_dma_event(DMA_EVENT_SPECIAL);
		}
#endif
	}
}

static void generate_dmal(void)
{
	handle_dmal();

	// even cycles only
	if (!(agnus_hpos_prev & 1) && (agnus_hpos & 1)) {
		shift_dmal();
	}
}

static void generate_dma_requests(void)
{
	if (!custom_disabled) {
		generate_bpl(cck_clock);
		if (bplcon0 & 0x0080) {
			generate_uhres();
		}
		if (copper_enabled_thisline && !custom_fastmode) {
			generate_copper();
		}
	}

	if (blt_info.blit_queued || blitter_delayed_update) {
		generate_blitter();
	}
}

static void do_cck(bool docycles)
{
	get_cck_clock();

	if (!custom_disabled) {
		bitplane_rga_ptmod();
	}

	handle_rga_out();

	generate_dma_requests();
	if (!custom_disabled) {
		decide_bpl(agnus_hpos);
	}

	if (need_vdiw_check) {
		need_vdiw_check = false;
		check_bpl_vdiw();
	}

	check_hsyncs();

	if (agnus_hpos == HARDWIRED_DMA_TRIGGER_HPOS) {
		if (custom_fastmode < 0) {
			custom_fastmode = 0;
		} else {
			if (!syncs_stopped) {
				custom_trigger_start();
				if (custom_fastmode > 0) {
					return;
				}
			}
		}
	}

	decide_hsync();
	empty_pipeline();

	inc_cck();
	if (docycles) {
		do_cycles_normal(1 * CYCLE_UNIT);
	}

	dmacon_bpl = dmaen(DMA_BITPLANE);
	dmacon_next = dmacon;

	handle_pipelined_write();
	handle_pipelined_custom_write(false);

	shift_rga();

	if (custom_fastmode <= 0) {
		generate_dmal();
	}

}

// horizontal sync callback when line not changed + fast cpu
static void sync_equalline_handler(void)
{
	int hpos_delta = 1;

	eventtab[ev_sync].active = 0;

	handle_pipelined_write();
	handle_pipelined_custom_write(false);

	int rdc_offset = REFRESH_FIRST_HPOS - hpos_delta;

	rga_denise_cycle += rdc_offset;
	rga_denise_cycle &= DENISE_RGA_SLOT_MASK;

	uae_u16 str = get_strobe_reg(0);
	write_drga_strobe(str);
	if (prev_strobe == 0x3c && str != 0x3c) {
		INTREQ_INT(5, 0);
	}
	prev_strobe = str;

	int diff = display_hstart_fastmode - hpos_delta;
	linear_hpos += diff;
	currcycle_cck += diff;
	rga_denise_cycle_count_end += diff;
	rga_denise_cycle += diff - rdc_offset;
	rga_denise_cycle &= DENISE_RGA_SLOT_MASK;

	if (custom_fastmode == 1) {
		int rdc = rga_denise_cycle;
		do_draw_line();
		rga_denise_cycle = rdc;
	} else {
		denise_handle_quick_strobe_queue(str, display_hstart_fastmode, rga_denise_cycle);
		next_denise_rga();
		decide_line_end();
	}

	for (int i = 0; i < RGA_SLOT_TOTAL + 1; i++) {
		struct rgabuf *r = &rga_pipe[i];
		clear_rga(r);
	}

	agnus_hpos = hpos_delta;
	linear_hpos = 0;

	diff = ((int)(get_cycles() - eventtab[ev_sync].oldcycles)) / CYCLE_UNIT;
	diff -= display_hstart_fastmode - hpos_delta;

	linear_hpos += diff;
	currcycle_cck += diff;
	rga_denise_cycle_count_end += diff;
	rga_denise_cycle += diff;
	rga_denise_cycle &= DENISE_RGA_SLOT_MASK;

	fast_lines_cnt++;

	custom_trigger_start();

	check_vsyncs_fast();

	if (eventtab[ev_sync].active) {
		check_bpl_vdiw();
		do_imm_dmal();
	} else {
		custom_fastmode = -1;
		custom_fastmode_exit = 1;
	}
}

// horizontal sync callback when in RTG mode + fast cpu
static void sync_imm_evhandler(void)
{
	if (!custom_disabled) {
		eventtab[ev_sync].active = 0;
		write_log("Chipset emulation active\n");
		rga_denise_cycle_line++;
		return;
	}

	uae_u16 str = get_strobe_reg(0);
	if (prev_strobe == 0x3c && str != 0x3c) {
		INTREQ_INT(5, 0);
	}
	prev_strobe = str;

	custom_trigger_start();

	do_imm_dmal();

	start_sync_imm_handler();
}


int do_cycles_cck(int cycles)
{
	int c = 0;
	while (cycles >= CYCLE_UNIT) {
		do_cck(true);
		c += CYCLE_UNIT;
		cycles -= CYCLE_UNIT;
		if (custom_fastmode > 0) {
			return c;
		}
	}
	return c;
#if 0
	int c = cycles;
	while (c >= CYCLE_UNIT) {
		do_cck();
		c -= CYCLE_UNIT;
	}
	if (c > 0) {
		evt_t t = get_cycles();
		do_cycles(c);
		evt_t t2 = get_cycles();
		if ((t & ~(CYCLE_UNIT - 1)) != (t2 & ~(CYCLE_UNIT - 1))) {
			do_cck_noce();
		}
	}
#endif
}










#ifdef CPUEMU_13

// blitter not in nasty mode = CPU gets one cycle if it has been waiting
// at least 4 cycles (all DMA cycles count, not just blitter cycles, even
// blitter idle cycles do count!)

extern int cpu_tracer;
static int dma_cycle(int *mode, int *ipl)
{
	if (cpu_tracer < 0) {
		return current_hpos_safe();
	}
	if (!currprefs.cpu_memory_cycle_exact) {
		return current_hpos_safe();
	}
	blt_info.nasty_cnt = 0;
	while (currprefs.cpu_memory_cycle_exact) {
		struct rgabuf *r = read_rga_out();
		if (r->alloc <= 0 || quit_program > 0) {
			break;
		}
		blt_info.nasty_cnt++;
		*ipl = regs.ipl_pin;
		do_cck(true);
		/* bus was allocated to dma channel, wait for next cycle.. */
	}
	blt_info.nasty_cnt = 0;
	return agnus_hpos;
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
		x_do_cycles(extra);
	}
}

uae_u32 wait_cpu_cycle_read(uaecptr addr, int mode)
{
	uae_u32 v = 0, vd = 0;
	int ipl = regs.ipl[0];
	evt_t now = get_cycles();

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	dma_cycle(&mode, &ipl);

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
		record_dma_read(reg, addr, DMARECORD_CPU, mode == -2 || mode == 2 ? 0 : 1);
	}
	peekdma_data.mask = 0;
#else
	if (mode == -3) {
		v = regs.chipset_latch_rw;
	}
#endif

	switch(mode)
	{
		case -1:
		v = vd = get_long(addr);
		break;
		case -2:
		v = vd = get_longi(addr);
		break;
		case 1:
		v = vd = get_word(addr);
		break;
		case 2:
		v = vd = get_wordi(addr);
		break;
		case 0:
		v = vd = get_word(addr & ~1);
		v >>= (addr & 1) ? 0 : 8;
		break;
	}

#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_read_value_pos(vd);
	}
#endif

	x_do_cycles_post(CYCLE_UNIT, 0);

	regs.chipset_latch_rw = regs.chipset_latch_read = v;

	// if IPL fetch was pending and CPU had wait states
	// Use ipl_pin value from previous cycle
	if (now == regs.ipl_evt) {
		regs.ipl[0] = ipl;
	}

	return v;
}

void wait_cpu_cycle_write(uaecptr addr, int mode, uae_u32 v)
{
	int ipl = regs.ipl[0];
	evt_t now = get_cycles();

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	dma_cycle(&mode, &ipl);

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
		record_dma_write(reg, v, addr, DMARECORD_CPU, 1);
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

	x_do_cycles_post(CYCLE_UNIT, v);

	regs.chipset_latch_rw = regs.chipset_latch_write = v;

	// if IPL fetch was pending and CPU had wait states:
	// Use ipl_pin value from previous cycle
	if (now == regs.ipl_evt) {
		regs.ipl[0] = ipl;
	}
}


uae_u32 wait_cpu_cycle_read_ce020(uaecptr addr, int mode)
{
	uae_u32 v = 0;
	int ipl;

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	dma_cycle(NULL, &ipl);

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
		record_dma_read(reg, addr, DMARECORD_CPU, mode == -2 || mode == 2 ? 0 : 1);
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
		record_dma_read_value_pos(v);
	}
#endif

	regs.chipset_latch_rw = regs.chipset_latch_read = v;

	x_do_cycles_post(CYCLE_UNIT, v);

	return v;
}

void wait_cpu_cycle_write_ce020(uaecptr addr, int mode, uae_u32 v)
{
	int ipl;

	sync_cycles();

	x_do_cycles_pre(CYCLE_UNIT);

	dma_cycle(NULL, &ipl);

#ifdef DEBUGGER
	if (debug_dma) {
		int reg = 0x1100;
		if (mode < 0)
			reg |= 4;
		else if (mode > 0)
			reg |= 2;
		else
			reg |= 1;
		record_dma_write(reg, v, addr, DMARECORD_CPU, 1);
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
		do_cck(true);
		cycles -= CYCLE_UNIT;
	}
	extra_cycle = cycles;
}

void do_cycles_ce020(int cycles)
{
	evt_t cc;
	static int extra;

	cycles += extra;
	extra = 0;
	if (!cycles) {
		return;
	}
	cc = get_cycles();
	while (cycles >= CYCLE_UNIT) {
		do_cck(true);
		cycles -= CYCLE_UNIT;
	}
	extra += cycles;
#if 0
	if (cycles > 0) {
		cc = get_cycles();
		evt_t cc2 = cc + cycles;
		if ((cc & ~(CYCLE_UNIT - 1)) != (cc2 & ~(CYCLE_UNIT - 1))) {
			do_cck();
		}
	}
#endif
}

bool is_cycle_ce(uaecptr addr)
{
	addrbank *ab = get_mem_bank_real(addr);
	if (!ab || (ab->flags & ABFLAG_CHIPRAM) || ab == &custom_bank) {
		struct rgabuf *r = read_rga_out();
		if (r->alloc <= 0) {
			return false;
		}
		return true;
	}
	return false;
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
		*lines = current_linear_vpos_visible;
	}
	if (programmedmode == 1) {
		return currprefs.ntscmode == 0;
	}
	return current_linear_vpos_nom >= MAXVPOS_NTSC + (MAXVPOS_PAL - MAXVPOS_NTSC) / 2;
}

void custom_end_drawing(void)
{
	draw_denise_line_queue_flush();
	end_draw_denise();
}
