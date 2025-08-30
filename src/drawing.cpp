
/*
* UAE - The Un*x Amiga Emulator
*
* Screen drawing functions
*
* Copyright 1995-2000 Bernd Schmidt
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

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "xwin.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "savestate.h"
#include "statusline.h"
#include "inputdevice.h"
#include "debug.h"
#ifdef CD32
#include "cd32_fmv.h"
#endif
#ifdef WITH_SPECIALMONITORS
#include "specialmonitors.h"
#endif
#include "devices.h"
#include "gfxboard.h"

#define ENABLE_MULTITHREADED_DENISE 1
extern int multithread_enabled;
#define MULTITHREADED_DENISE (ENABLE_MULTITHREADED_DENISE && multithread_enabled != 0)

#define BLANK_COLOR 0x000000

#if 0
#define DEBUG_TVOVERSCAN_H_GRAYSCALE 0x22
#define DEBUG_TVOVERSCAN_V_GRAYSCALE 0x44
#define DEBUG_LOL_COLOR 0x006600
#else
#define DEBUG_TVOVERSCAN_H_GRAYSCALE 0x0
#define DEBUG_TVOVERSCAN_V_GRAYSCALE 0x0
#define DEBUG_LOL_COLOR 0x000000
#endif

#define DEBUG_ALWAYS_UNALIGNED_DRAWING 0

#define AUTOSCALE_SPRITES 1
#define LOL_SHIFT_COLORS 0
#define GENLOCK_EXTRA_CLEAR 8
#define MAX_RGA_OVERLAPPING_CYCLES 5

uae_u8 *xlinebuffer, *xlinebuffer2;
uae_u8 *xlinebuffer_genlock;
static uae_u8* xlinebuffer_start, * xlinebuffer_end;
static uae_u8* xlinebuffer2_start, * xlinebuffer2_end;
static uae_u8 *xlinebuffer_genlock_start, *xlinebuffer_genlock_end;

static int *amiga2aspect_line_map, *native2amiga_line_map;
static int native2amiga_line_map_height;
static uae_u8 **row_map;
static uae_u8 *row_map_genlock_buffer;
static uae_u8 row_tmp8[MAX_PIXELS_PER_LINE * 32 / 8];
static uae_u8 row_tmp8g[MAX_PIXELS_PER_LINE * 32 / 8];
static int max_drawn_amiga_line;
uae_u8 **row_map_genlock;
uae_u8 *row_map_color_burst_buffer;

static uae_sem_t write_sem, read_sem;

struct denise_rga_queue
{
	int type;
	int vpos, linear_vpos;
	int gfx_ypos;
	nln_how how;
	uae_u32 linecnt;
	int startpos, endpos;
	int startcycle, endcycle;
	int skip;
	int skip2;
	int dtotal;
	int calib_start;
	int calib_len;
	bool lol, lof;
	int hdelay;
	bool blanked;
	uae_u16 strobe;
	int strobe_pos;
	int erase;
	bool finalseg;
	struct linestate *ls;
	uae_u16 reg, val;
};

static volatile uae_atomic rga_queue_read, rga_queue_write;
static int denise_thread_state;
static struct denise_rga_queue rga_queue[DENISE_RGA_SLOT_CHUNKS];
static struct denise_rga_queue temp_line;
static struct denise_rga_queue *this_line;
static volatile bool thread_debug_lock;

static void denise_handle_quick_strobe(uae_u16 strobe, int offset, int vpos);
static void draw_denise_vsync(int);
static void denise_update_reg(uae_u16 reg, uae_u16 v, uae_u32 linecnt);
static void draw_denise_line(int gfx_ypos, nln_how how, uae_u32 linecnt, int startpos, int startcycle, int endcycle, int skip, int skip2, int dtotal, int calib_start, int calib_len, bool lol, int hdelay, bool blanked, bool finalseg, struct linestate *ls);

static void quick_denise_rga(uae_u32 linecnt, int startpos, int endpos)
{
	int pos = startpos;
	endpos++;
	endpos &= DENISE_RGA_SLOT_MASK;
	while (pos != endpos) {
		struct denise_rga *rd = &rga_denise[pos];
		if (rd->line == linecnt && rd->rga != 0x1fe && (rd->rga < 0x38 || rd->rga >= 0x40)) {
			denise_update_reg(rd->rga, rd->v, linecnt);
		}
		pos++;
		pos &= DENISE_RGA_SLOT_MASK;
	}
}

static void update_overlapped_cycles(int endpos)
{
	for (int i = 0; i <= MAX_RGA_OVERLAPPING_CYCLES; i++) {
		struct denise_rga *rga = &rga_denise[(endpos - i) & DENISE_RGA_SLOT_MASK];
		rga->line++;
	}
}

static void read_denise_line_queue(void)
{
	bool nolock = false;

	while (rga_queue_read == rga_queue_write) {
		uae_sem_wait(&write_sem);
	}

	struct denise_rga_queue *q = &rga_queue[rga_queue_read & DENISE_RGA_SLOT_CHUNKS_MASK];
	this_line = q;
	bool next = false;

	//evt_t t1 = read_processor_time();

#if 0
	sleep_millis(1);
#endif


	if (q->type == 0) {
		draw_denise_line(q->gfx_ypos, q->how, q->linecnt, q->startpos, q->startcycle, q->endcycle, q->skip, q->skip2, q->dtotal, q->calib_start, q->calib_len, q->lol, q->hdelay, q->blanked, q->finalseg, q->ls);
		next = q->finalseg;
	} else if (q->type == 1) {
		draw_denise_bitplane_line_fast(q->gfx_ypos, q->how, q->ls);
	} else if (q->type == 2) {
		draw_denise_border_line_fast(q->gfx_ypos, q->how, q->ls);
	} else if (q->type == 3) {
		quick_denise_rga(q->linecnt, q->startpos, q->endpos);
	} else if (q->type == 4) {
		denise_handle_quick_strobe(q->strobe, q->strobe_pos, q->vpos);
		next = true;
		nolock = true;
	} else if (q->type == 5) {
		draw_denise_vsync(q->erase);
		nolock = true;
	} else if (q->type == 6) {
		denise_update_reg(q->reg, q->val, q->linecnt);
		nolock = true;
	} else if (q->type == 7) {
		if (q->val) {
			denise_store_registers();
		} else {
			denise_restore_registers();
		}
	}

	if (!nolock) {
		if (!thread_debug_lock) {
			write_log("read_denise_line_queue: queue processed without lock!\n");
		}
	}

	//evt_t t2 = read_processor_time();

	//write_log("%lld ", (t2 - t1));

	if (next) {
		update_overlapped_cycles(q->endpos);
	}

	if (!nolock) {
		if (!thread_debug_lock) {
			write_log("read_denise_line_queue: queue lock was released during draw!\n");
		}
	}

#if 0
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = vidinfo->inbuffer;
	if (!vb->locked || !vb->bufmem || row_map[0] == NULL) {
		write_log("read_denise_line_queue: buffer cleared!\n");
	}
	for (int i = 0; i < vb->inheight; i++) {
		uae_u8 *p = row_map[i];
		*p = 0x12;
	}
#endif

	atomic_inc(&rga_queue_read);

	uae_sem_post(&read_sem);
}

static int denise_thread(void *v)
{
	denise_thread_state = 1;
	while(denise_thread_state) {
		read_denise_line_queue();
	}
	denise_thread_state = -1;
	return 0;
}

static bool denise_locked;

static bool denise_lock(void)
{
	draw_denise_line_queue_flush();

	if (denise_locked) {
		return true;
	}

	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;
	struct vidbuffer *vb = &vidinfo->drawbuffer;

	if (!vb->locked) {
		if (!lockscr(vb, false, display_reset > 0)) {
			notice_screen_contents_lost(monid);
			return false;
		}
	}
	denise_locked = true;
	return true;
}

int scandoubled_line;

struct amigadisplay adisplays[MAX_AMIGADISPLAYS];

typedef enum
{
	CMODE_NORMAL,
	CMODE_DUALPF,
	CMODE_EXTRAHB,
	CMODE_HAM,
	CMODE_EXTRAHB_ECS_KILLEHB
} CMODE_T;

static void select_lts(void);

static uae_u32 ham_lastcolor;

int debug_bpl_mask = 0xff, debug_bpl_mask_one;

/* mirror of chipset_mask */
bool ecs_agnus;
bool ecs_denise, ecs_denise_only;
bool aga_mode;
bool agnusa1000;
bool denisea1000_noehb;
bool denisea1000;

bool direct_rgb;

static int linedbl, linedbld;

int interlace_seen;
int detected_screen_resolution;

#define AUTO_LORES_FRAMES 10
static int can_use_lores = 0, frame_res, frame_res_lace;
static int resolution_count[RES_MAX + 1], lines_count;
static int center_reset;
static bool init_genlock_data;
bool need_genlock_data;

/* Lookup tables for dual playfields.  The dblpf_*1 versions are for the case
that playfield 1 has the priority, dbplpf_*2 are used if playfield 2 has
priority.  If we need an array for non-dual playfield mode, it has no number.  */
/* The dbplpf_ms? arrays contain a shift value.  plf_spritemask is initialized
to contain two 16 bit words, with the appropriate mask if pf1 is in the
foreground being at bit offset 0, the one used if pf2 is in front being at
offset 16.  */

static int dblpf_ms1[256], dblpf_ms2[256], dblpf_ms[256];
static int dblpf_ind1[256], dblpf_ind2[256];

static int dblpf_2nd1[256], dblpf_2nd2[256];

static const int dblpfofs[] = { 0, 2, 4, 8, 16, 32, 64, 128 };

static int sprite_offs[256];

/* Video buffer description structure. Filled in by the graphics system
* dependent code. */

/* OCS/ECS color lookup table. */
xcolnr xcolors[4096];

static uae_u32 chunky_out[4096], dpf_chunky_out[4096];

#ifdef AGA
/* AGA mode color lookup tables */
unsigned int xredcolors[256], xgreencolors[256], xbluecolors[256];
static int dblpf_ind1_aga[256], dblpf_ind2_aga[256];
#else
static uae_u8 spriteagadpfpixels[1];
static int dblpf_ind1_aga[1], dblpf_ind2_aga[1];
#endif
int xredcolor_s, xredcolor_b, xredcolor_m;
int xgreencolor_s, xgreencolor_b, xgreencolor_m;
int xbluecolor_s, xbluecolor_b, xbluecolor_m;

xcolnr fullblack;
static struct color_entry direct_colors_for_drawing_bypass;

static xcolnr *p_acolors;
static xcolnr *p_xcolors;

static uae_u8 *refresh_indicator_buffer;
static uae_u8 *refresh_indicator_changed, *refresh_indicator_changed_prev;
static int refresh_indicator_height;

static int center_horizontal_offset;
/* The visible window: VISIBLE_LEFT_BORDER contains the left border of the visible
area, VISIBLE_RIGHT_BORDER the right border.  These are in window coordinates.  */
int visible_left_border, visible_right_border;
/* Pixels outside of visible_start and visible_stop are always black */
static int visible_left_start, visible_right_stop;
static int visible_top_start, visible_bottom_stop;
static bool exthblanken;
static int exthblank;
static bool syncdebug;

static int linetoscr_x_adjust;
static int thisframe_y_adjust, thisframe_y_adjust_real, prev_thisframe_y_adjust_real;
static bool center_y_erase, erase_next_draw;
static int min_ypos_for_screen;
static int max_ypos_thisframe1;
static int thisframe_first_drawn_line, thisframe_last_drawn_line;

/* A frame counter that forces a redraw after at least one skipped frame in
interlace mode.  */
static int last_redraw_point;

#define MAX_STOP 30000
static int first_drawn_line, last_drawn_line;

/* These are generated by the drawing code from the line_decisions array for
each line that needs to be drawn.  These are basically extracted out of
bit fields in the hardware registers.  */
static int bplmode, bplmode_new;
static bool bplehb_eke, bplham, bplehb, bpldualpf, bpldualpfpri;
static uae_u8 bplehb_mask;
static int bpldualpf2of, bplplanecnt, bplmaxplanecnt, ecsshres;
static int bplbypass, bplcolorburst, bplcolorburst_field;
static int bplres;
static int plf1pri, plf2pri, bpland;
static uae_u32 plf_sprite_mask;
static int sbasecol[2], sbasecol2[2];
static bool ecs_genlock_features_active;
static uae_u8 ecs_genlock_features_mask;
static bool ecs_genlock_features_colorkey;
static bool aga_genlock_features_zdclken;

uae_sem_t gui_sem;

static volatile int rga_denise_fast_read, rga_denise_fast_write;
#define DENISE_RGA_SLOT_FAST_TOTAL 1024
static struct denise_rga rga_denise_fast[DENISE_RGA_SLOT_FAST_TOTAL];

typedef void (*LINETOSRC_FUNC)(void);
static LINETOSRC_FUNC lts;
static bool lts_changed, lts_request;
typedef void (*LINETOSRC_FUNCF)(int,int,int,int,int,int,int,int,int,uae_u32,uae_u8*,uae_u8*,int,int*,int,struct linestate*);

static int denise_hcounter, denise_hcounter_next, denise_hcounter_new, denise_hcounter_prev, denise_hcounter_cmp;
static bool denise_accurate_mode;
static uae_u32 bplxdat[MAX_PLANES], bplxdat2[MAX_PLANES], bplxdat3[MAX_PLANES];
static uae_u64 bplxdat_64[MAX_PLANES], bplxdat2_64[MAX_PLANES], bplxdat3_64[MAX_PLANES];
static uae_u16 bplcon0_denise, bplcon1_denise, bplcon2_denise, bplcon3_denise, bplcon4_denise;
static uae_u8 bplcon4_denise_xor_val, bplcon4_denise_sbase, bplcon4_denise_xor_val2;
static int bplcon1_shift[2], bplcon1_shift_full[2], bplcon1_shift_full_masked[2], bplshiftcnt[2];
static int bplcon1_shift_mask, bplcon1_shift_mask_full;
static int denise_res, denise_res_size;
static uae_u16 denise_diwstrt, denise_diwstop;
static int denise_hstrt, denise_hstop, denise_diwhigh, denise_diwhigh2;
static int denise_brdstrt, denise_brdstop;
static int denise_brdstrt_lores, denise_brdstop_lores;
static bool denise_brdstrt_unalign, denise_brdstop_unalign;
static int denise_hstrt_lores, denise_hstop_lores;
static bool denise_hstrt_unalign, denise_hstop_unalign, denise_phbstrt_unalign, denise_phbstop_unalign;
static int denise_hbstrt_lores, denise_hbstop_lores;
static int denise_strlong_lores, denise_strlong_hd;
static bool denise_strlong_unalign, strlong_emulation;
static int denise_phbstrt, denise_phbstop, denise_phbstrt_lores, denise_phbstop_lores;
static int linear_denise_vbstrt, linear_denise_vbstop;
static int linear_denise_hbstrt, linear_denise_hbstop;
static int linear_denise_frame_hbstrt, linear_denise_frame_hbstop;
static int denise_visible_lines, denise_visible_lines_counted;
static uae_u16 hbstrt_denise_reg, hbstop_denise_reg;
static uae_u16 fmode_denise, denise_bplfmode, denise_sprfmode;
static bool denise_sprfmode64, denise_bplfmode64;
static int bpldat_fmode;
static int fetchmode_size_denise, fetchmode_mask_denise;
static int delayed_vblank_ecs;
static bool denise_hdiw, denise_hblank, denise_phblank, denise_vblank, denise_pvblank;
static bool denise_blank_active, denise_blank_active2, denise_hblank_active, denise_vblank_active;
static bool debug_special_csync, debug_special_hvsync;
static bool exthblankon_ecs, exthblankon_ecsonly, exthblankon_aga;
static bool denise_csync, denise_vsync, denise_hsync, denise_blank_enabled;
static bool denise_csync_blanken, denise_csync_blanken2;
static bool diwhigh_written;
struct color_entry denise_colors;
static bool bpl1dat_trigger, bpl1dat_copy;
static uae_u32 bordercolor, bordercolor_ecs_shres;
static int sprites_hidden, sprites_hidden2, sprite_hidden_mask;
static bool bordersprite, borderblank, bordertrans;
static bool bpldat_copy[2];
static int denise_planes, denise_max_planes;
static bool denise_odd_even, denise_max_odd_even;
static int pix_prev;
static int last_bpl_pix;
static int previous_strobe;
static bool denise_strlong, denise_strlong_fast, agnus_lol, extblank;
static int lol, lol_fast;
static int denise_lol_shift_prev;
static bool denise_lol_shift_enable;
static int decode_specials, decode_specials_debug;
static int *dpf_lookup, *dpf_lookup_no;
static int denise_sprres, denise_spr_add, denise_spr_shiftsize;
static int denise_xposmask, denise_xposmask_lores, denise_xposmask_mask_lores;
static uae_u16 clxcon, clxcon2;
static int clxcon_bpl_enable_o, clxcon_bpl_match_o;
static int clxcon_bpl_enable, clxcon_bpl_match;
static uae_u16 clxcon_bpl_enable_55, clxcon_bpl_enable_aa;
static uae_u16 clxcon_bpl_match_55, clxcon_bpl_match_aa;
static int aga_delayed_color_idx;
static uae_u16 aga_delayed_color_val, aga_delayed_color_con2, aga_delayed_color_con3;
static int aga_unalign0, aga_unalign1, bpl1dat_unalign, reswitch_unalign;
static uae_u8 loaded_pix, loaded_pixs[4];
static int hresolution, hresolution_add;
static bool denise_sprite_blank_active;
static int delayed_sprite_vblank_ecs;
static bool denise_burst;
static int *debug_dma_dhpos_odd;
static struct dma_rec *debug_dma_ptr;
static int denise_cycle_half;
static int denise_vblank_extra_top, denise_vblank_extra_bottom;
static int denise_hblank_extra_left, denise_hblank_extra_right;
static uae_u32 dtbuf[2][4];
static uae_u8 dtgbuf[2][4];

struct denise_spr
{
	int num;
	uae_u16 pos, ctl;
	uae_u32 dataa, datab;
	uae_u64 dataa64, datab64;
	int xpos, xpos_lores;
	int armed, armeds;
	uae_u32 dataas, databs;
	uae_u64 dataas64, databs64;
	bool attached;
	bool shiftercopydone;
	int fmode;
	int shift;
	int pix;
};
static struct denise_spr dspr[MAX_SPRITES];
static struct denise_spr *dprspt[MAX_SPRITES + 1], *dprspts[MAX_SPRITES + 1];
static int denise_spr_nr_armed, denise_spr_nr_armeds;
static uae_u16 bplcoltable[256];
static uae_u16 sprcoltable[256];
static uae_u16 sprbplcoltable[256];
static uae_u8 sprcolmask;
static int denise_spr_nearestcnt;

static int denise_y_start, denise_y_end;

static int denise_pixtotal, denise_pixtotalv, denise_linecnt, denise_startpos, denise_cck, denise_endcycle;
static int denise_pixtotalskip, denise_pixtotalskip2, denise_hdelay;
static int denise_pixtotal_max;
static uae_u32 *buf1, *buf2, *buf_d;
static uae_u8 *gbuf;
static uae_u8 pixx0, pixx1, pixx2, pixx3;
static uae_u32 debug_buf[256 * 2 * 4], debug_bufx[256 * 2 * 4];
static int hbstrt_offset, hbstop_offset;
static int hstrt_offset, hstop_offset;
static int bpl1dat_trigger_offset;
static int internal_pixel_cnt, internal_pixel_start_cnt;
static bool no_denise_lol, denise_strlong_seen;
#define STRLONG_SEEN_DELAY 2
static int denise_strlong_seen_delay;

void set_inhibit_frame(int monid, int bit)
{
	struct amigadisplay *ad = &adisplays[monid];
	ad->inhibit_frame |= 1 << bit;
}
void clear_inhibit_frame(int monid, int bit)
{
	struct amigadisplay *ad = &adisplays[monid];
	ad->inhibit_frame &= ~(1 << bit);
}
void toggle_inhibit_frame(int monid, int bit)
{
	struct amigadisplay *ad = &adisplays[monid];
	ad->inhibit_frame ^= 1 << bit;
}

static void clearbuffer(struct vidbuffer *dst)
{
	if (!dst->bufmem_allocated)
		return;
	uae_u8 *p = dst->bufmem_allocated;
	for (int y = 0; y < dst->height_allocated; y++) {
		memset (p, 0, dst->width_allocated * dst->pixbytes);
		p += dst->rowbytes;
	}
}

static void count_frame(int monid)
{
	struct amigadisplay *ad = &adisplays[monid];
	ad->framecnt++;
	if (ad->framecnt >= currprefs.gfx_framerate || currprefs.monitoremu == MONITOREMU_A2024)
		ad->framecnt = 0;
	if (ad->inhibit_frame)
		ad->framecnt = 1;
}

STATIC_INLINE int xshift(int x, int shift)
{
	if (shift < 0)
		return x >> (-shift);
	else
		return x << shift;
}

int coord_native_to_amiga_x(int x)
{
	x += visible_left_border;
	return x;
}

int coord_native_to_amiga_y(int y)
{
#ifdef AMIBERRY
	if (!native2amiga_line_map || y < 0) {
		return 0;
	}
	if (y >= native2amiga_line_map_height) {
		y = native2amiga_line_map_height + thisframe_y_adjust - 1;
	}
	return native2amiga_line_map[y] - minfirstline;
#else
	if (!native2amiga_line_map || y < 0) {
		return 0;
	}
	if (y >= native2amiga_line_map_height) {
		y = native2amiga_line_map_height - 1;
	}
	return native2amiga_line_map[y] - minfirstline;
#endif
}

void notice_screen_contents_lost(int monid)
{
	struct amigadisplay *ad = &adisplays[monid];
	ad->picasso_redraw_necessary = 1;
	ad->frame_redraw_necessary = 2;
}

bool isnativevidbuf(int monid)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	if (vidinfo->outbuffer == NULL)
		return false;
	if (vidinfo->outbuffer == vidinfo->inbuffer)
		return true;
	return !vidinfo->outbuffer->hardwiredpositioning;
}

extern int plffirstline_total, plflastline_total;
extern int diwfirstword_total, diwlastword_total;
extern int ddffirstword_total, ddflastword_total;
extern bool vertical_changed, horizontal_changed;
extern int firstword_bplcon1;
extern bool lof_display;

#define MIN_DISPLAY_W 256
#define MIN_DISPLAY_H 192
#define MAX_DISPLAY_W 362
#define MAX_DISPLAY_H 283

static int gclow, gcloh, gclox, gcloy, gclorealh;
static int stored_left_start, stored_top_start, stored_width, stored_height;

void get_custom_topedge (int *xp, int *yp, bool max)
{
	if (isnativevidbuf(0) && !max) {
		int x = visible_left_border;
		int y = minfirstline << currprefs.gfx_vresolution;

		x -= 1 << currprefs.gfx_resolution;
#if 0
		int dbl1, dbl2;
		dbl2 = dbl1 = currprefs.gfx_vresolution;
		if (doublescan > 0 && interlace_seen <= 0) {
			dbl1--;
			dbl2--;
		}
		x = -(visible_left_border + (DISPLAY_LEFT_SHIFT << currprefs.gfx_resolution));
		y = -minfirstline << currprefs.gfx_vresolution;
		y = xshift (y, dbl2);
#endif
		*xp = x;
		*yp = y;
	} else {
		*xp = 0;
		*yp = 0;
	}
}

static void reset_custom_limits(void)
{
	gclow = gcloh = gclox = gcloy = 0;
	gclorealh = -1;
	center_reset = 1;
}

int get_vertical_visible_height(bool useoldsize)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int h = vidinfo->inbuffer->inheight;

	if (programmedmode <= 1) {
		h = maxvsize_display;
		if (useoldsize) {
			// 288/576 or 243/486
			if (h == 288 || h == 243) {
				h--;
			}
		}
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_BROADCAST) {
			h += 2;
		}
		h <<= currprefs.gfx_vresolution;
		if (h > vidinfo->inbuffer->inheight) {
			h = vidinfo->inbuffer->inheight;
		}
	}
	if (interlace_seen && currprefs.gfx_vresolution > 0) {
		h -= 1 << (currprefs.gfx_vresolution - 1);
	}
	if (!syncdebug) {
		bool hardwired = true;
		if (ecs_agnus) {
			hardwired = (new_beamcon0 & BEAMCON0_VARVBEN) == 0;
		}
		if (hardwired) {
			int hh = denise_visible_lines_counted;
			if (currprefs.gfx_overscanmode >= OVERSCANMODE_BROADCAST) {
				hh += 2;
			}
			hh <<= currprefs.gfx_vresolution;
			if (h > hh) {
				h = hh;
			}
		}
	}
	return h;
}

void get_custom_raw_limits(int *pw, int *ph, int *pdx, int *pdy)
{
	if (stored_width > 0) {
		*pw = stored_width;
		*ph = stored_height;
		*pdx = stored_left_start;
		*pdy = stored_top_start;
	} else {
		int x = visible_left_border;
		x = std::max(x, visible_left_start);
		*pdx = x;
		int x2 = visible_right_border;
		x2 = std::min(x2, visible_right_stop);
		*pw = x2 - x;
		int y = min_ypos_for_screen;
		y = std::max(y, visible_top_start);
		*pdy = y;
		int y2 = max_ypos_thisframe1;
		y2 = std::min(y2, visible_bottom_stop);
		*ph = y2 - y;
	}
}

void check_custom_limits(void)
{
	struct amigadisplay *ad = &adisplays[0];
	struct gfx_filterdata *fd = &currprefs.gf[ad->gf_index];

	int vls = visible_left_start;
	int vrs = visible_right_stop;
	int vts = visible_top_start;
	int vbs = visible_bottom_stop;

	int left = fd->gfx_filter_left_border < 0 ? 0 : fd->gfx_filter_left_border >> (RES_MAX - currprefs.gfx_resolution);
	int right = fd->gfx_filter_right_border < 0 ? 0 : fd->gfx_filter_right_border >> (RES_MAX - currprefs.gfx_resolution);
	int top = fd->gfx_filter_top_border < 0 ? 0 : fd->gfx_filter_top_border;
	int bottom = fd->gfx_filter_bottom_border < 0 ? 0 : fd->gfx_filter_bottom_border;

	denise_vblank_extra_top = -1;
	denise_vblank_extra_bottom = 30000;

	if (left > 0) {
		left += (0x38 * 4) >> (RES_MAX - currprefs.gfx_resolution);
	}
	if (right > 0) {
		right += (0x38 * 4) >> (RES_MAX - currprefs.gfx_resolution);
	}

	if (left > visible_left_start) {
		visible_left_start = left;
	}
	if (right > left && right < visible_right_stop) {
		visible_right_stop = right;
	}

	if (top > visible_top_start) {
		visible_top_start = top;
	}
	if (bottom > top && bottom < visible_bottom_stop) {
		visible_bottom_stop = bottom;
	}

	int vshift = currprefs.gfx_vresolution;
	if (doublescan == 1 && vshift > 0) {
		vshift--;
	}
	int ydiff = minfirstline - minfirstline_linear;
	denise_vblank_extra_top = (visible_top_start - ydiff) >> vshift;
	denise_vblank_extra_bottom = (visible_bottom_stop - ydiff) >> vshift;
	denise_hblank_extra_left = visible_left_start;
	denise_hblank_extra_right = visible_right_stop;

	//write_log("%d %d %d %d\n", denise_vblank_extra_top, denise_vblank_extra_bottom, visible_top_start, visible_bottom_stop);
}

void set_custom_limits (int w, int h, int dx, int dy, bool blank)
{
	struct amigadisplay *ad = &adisplays[0];
	struct gfx_filterdata *fd = &currprefs.gf[ad->gf_index];
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = vidinfo->inbuffer;

	if (dx > 0 && w > 0) {
		dx -= 1 << currprefs.gfx_resolution;
	}

	if (fd->gfx_filter_left_border == 0) {
		w = 0;
		dx = 0;
	}
	if (fd->gfx_filter_top_border == 0) {
		h = 0;
		dy = 0;
	}
#ifdef WITH_SPECIALMONITORS
	if (specialmonitor_uses_control_lines() || !blank) {
		w = -1;
		h = -1;
	}
#endif

	int wwadd = 0;
	int hhadd = 0, hhadd2 = 0;
	if (currprefs.gfx_overscanmode < OVERSCANMODE_OVERSCAN) {
		int addw = (OVERSCANMODE_OVERSCAN - currprefs.gfx_overscanmode) * 8;
		int addh = (OVERSCANMODE_OVERSCAN - currprefs.gfx_overscanmode) * 5;
		if (currprefs.gfx_overscanmode == 0) {
			addw -= 6;
		}
		wwadd = addw << hresolution;
		hhadd = addh << currprefs.gfx_vresolution;
		hhadd2 = hhadd + (1 << currprefs.gfx_vresolution);
	}

	int vshift = currprefs.gfx_vresolution;
	if (doublescan == 1) {
		vshift--;
	}
	if (vshift < 0) {
		vshift = 0;
	}

	if (w <= 0 || dx < 0) {
		visible_left_start = 0;
		visible_right_stop = MAX_STOP;
	} else {
		if (dx < wwadd) {
			w -= 2 * (wwadd - dx);
			dx = wwadd;
		}
		visible_left_start = visible_left_border + dx;
		visible_right_stop = visible_left_start + w;
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_BROADCAST) {
			visible_right_stop += 2 << currprefs.gfx_resolution;
		}
	}

	if (h <= 0 || dy < 0) {
		visible_top_start = 0;
		visible_bottom_stop = MAX_STOP;
	} else {
		int startypos = min_ypos_for_screen;
		visible_top_start = startypos + dy;
		visible_bottom_stop = startypos + dy + h;
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_BROADCAST) {
			visible_top_start -= 1 << currprefs.gfx_resolution;
			visible_bottom_stop += 1 << currprefs.gfx_resolution;
		}
		if (visible_top_start < hhadd + startypos) {
			visible_top_start = hhadd + startypos;
		}
		if ((current_linear_vpos << currprefs.gfx_vresolution) - hhadd2 < visible_bottom_stop) {
			visible_bottom_stop = (current_linear_vpos << currprefs.gfx_vresolution) - hhadd2;
		}
	}

	check_custom_limits();
}

void store_custom_limits(int w, int h, int x, int y)
{
	stored_left_start = x;
	stored_top_start = y;
	stored_width = w;
	stored_height = h;
#if 0
	write_log (_T("%dx%d %dx%d %dx%d %dx%d %d\n"), x, y, w, h,
	   	currprefs.gfx_xcenter_pos,
		currprefs.gfx_ycenter_pos,
		currprefs.gfx_xcenter_size,
		currprefs.gfx_ycenter_size,
		currprefs.gfx_filter_autoscale);
#endif
}

int get_custom_limits(int *pw, int *ph, int *pdx, int *pdy, int *prealh, int *hres, int *vres)
{
	static int interlace_count;
	static int interlace_lof[2];
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int w, h, dx, dy, y1, y2, dbl1, dbl2;
	int ret = 0;

	if (!pw || !ph || !pdx || !pdy) {
		reset_custom_limits();
		return 0;
	}

	*hres = hresolution;
	*vres = currprefs.gfx_vresolution;

	if (!isnativevidbuf(0)) {
		*pw = vidinfo->inbuffer->outwidth;
		*ph = vidinfo->inbuffer->outheight;
		*pdx = 0;
		*pdy = 0;
		*prealh = -1;
		return 1;
	}

	*pw = gclow;
	*ph = gcloh;
	*pdx = gclox;
	*pdy = gcloy;
	*prealh = gclorealh;

	if (gclow > 0 && gcloh > 0)
		ret = -1;

	if (interlace_seen) {
		for (;;) {
			if (!currprefs.gfx_iscanlines) {
				// if more than 1 long or short frames only: accept it, we may have double (non field) mode.
				if (interlace_lof[0] >= 2 || interlace_lof[1] >= 2) {
					break;
				}
			}
			// wait for long frame
			if (interlace_lof[0] && interlace_lof[1]) {
				if (!lof_display) {
					return ret;
				}
				break;
			}
			interlace_lof[lof_display]++;
			return ret;
		}
	} else {
		interlace_count = 0;
		interlace_lof[0] = 0;
		interlace_lof[1] = 0;
	}

	int skip = denise_hdelay << (RES_MAX + 1);
	int diwfirst = diwfirstword_total + skip;
	int diwlast = diwlastword_total + skip;

	int ddffirst = ddffirstword_total << (RES_MAX + 1);
	int ddflast = ddflastword_total << (RES_MAX + 1);

	if (doublescan <= 0 && !programmedmode) {
		int min = 92 << RES_MAX;
		int max = 460 << RES_MAX;
		if (diwfirst < min)
			diwfirst = min;
		if (diwlast > max)
			diwlast = max;
		if (ddffirstword_total < 30000) {
			if (ddffirst < min)
				ddffirst = min;
			if (ddflast > max)
				ddflast = max;
			if (0 && !aga_mode) {
				if (ddffirst > diwfirst)
					diwfirst = ddffirst;
				if (ddflast < diwlast)
					diwlast = ddflast;
			}
		}
	}

	w = diwlast - diwfirst;
	dx = diwfirst - (hdisplay_left_border << (RES_MAX + 1)) + (1 << RES_MAX);

	w >>= (RES_MAX - hresolution);
	dx >>= (RES_MAX - hresolution);

	y2 = plflastline_total;
	y1 = plffirstline_total;
	if (exthblankon_ecs) {
		y1--;
	}

	if (minfirstline_linear > y1) {
		y1 = minfirstline_linear;
	}

	dbl2 = dbl1 = currprefs.gfx_vresolution;
	if (doublescan > 0 && interlace_seen <= 0) {
		dbl1--;
		dbl2--;
	}

	h = y2 - y1 + 1;
	dy = y1 - minfirstline_linear;

	if (plffirstline_total >= 30000) {
		// no planes enabled during frame
		if (ret < 0)
			return 1;
		h = currprefs.ntscmode ? 200 : 240;
		w = 320 << hresolution;
		dy = 36 / 2;
		dx = 58;
	}

	if (dx < 0)
		dx = 0;

	*prealh = -1;
	if (programmedmode != 1 && plffirstline_total < 30000) {
		int th = (current_linear_vpos - minfirstline_linear) * 95 / 100;
		if (th > h) {
			th = xshift(th, dbl1);
			*prealh = th;
		}
	}

	dy = xshift(dy, dbl2);
	h = xshift(h, dbl1);

	if (w == 0 || h == 0)
		return 0;

#if 1
	if (doublescan <= 0 && programmedmode != 1) {
		if ((w >> currprefs.gfx_resolution) < MIN_DISPLAY_W) {
			dx += (w - (MIN_DISPLAY_W << currprefs.gfx_resolution)) / 2;
			w = MIN_DISPLAY_W << currprefs.gfx_resolution;
		}
		if ((h >> dbl1) < MIN_DISPLAY_H) {
			dy += (h - (MIN_DISPLAY_H << dbl1)) / 2;
			h = MIN_DISPLAY_H << dbl1;
		}
		if ((w >> currprefs.gfx_resolution) > MAX_DISPLAY_W) {
			dx += (w - (MAX_DISPLAY_W << currprefs.gfx_resolution)) / 2;
			w = MAX_DISPLAY_W << currprefs.gfx_resolution;
		}
		if ((h >> dbl1) > MAX_DISPLAY_H) {
			dy += (h - (MAX_DISPLAY_H << dbl1)) / 2;
			h = MAX_DISPLAY_H << dbl1;
		}
	}
#endif

	if (gclow == w && gcloh == h && gclox == dx && gcloy == dy)
		return ret;

	if (w <= 0 || h <= 0 || dx < 0 || dy < 0)
		return ret;
	if (doublescan <= 0 && programmedmode != 1) {
		if (dx > vidinfo->inbuffer->inwidth / 2)
			return ret;
		if (dy > vidinfo->inbuffer->inheight / 2)
			return ret;
	}

	gclow = w;
	gcloh = h;
	gclox = dx;
	gcloy = dy;
	gclorealh = *prealh;
	*pw = w;
	*ph = h;
	*pdx = dx;
	*pdy = dy;
#if 1
	write_log(_T("Display Size: %dx%d Offset: %dx%d\n"), w, h, dx, dy);
	write_log(_T("First: %d Last: %d Min: %d\n"),
		plffirstline_total, plflastline_total,
		minfirstline);
#endif
	center_reset = 1;
	resetfulllinestate();
	return 1;
}

void get_custom_mouse_limits (int *pw, int *ph, int *pdx, int *pdy, int dbl)
{
	int delay1, delay2;
	int w, h, dx, dy, dbl1, dbl2, y1, y2;

	w = diwlastword_total - diwfirstword_total;
	dx = diwfirstword_total - visible_left_border;

	y2 = plflastline_total;
	y1 = plffirstline_total;
	if (minfirstline > y1)
		y1 = minfirstline;

	h = y2 - y1;
	dy = y1 - minfirstline;

	if (*pw > 0)
		w = *pw;

	if (*ph > 0)
		h = *ph;

	delay1 = (firstword_bplcon1 & 0x0f) | ((firstword_bplcon1 & 0x0c00) >> 6);
	delay2 = ((firstword_bplcon1 >> 4) & 0x0f) | (((firstword_bplcon1 >> 4) & 0x0c00) >> 6);

	dbl2 = dbl1 = currprefs.gfx_vresolution;
	if ((doublescan > 0 || interlace_seen > 0) && !dbl) {
		dbl1--;
		dbl2--;
	}
	if (interlace_seen > 0)
		dbl2++;
	if (interlace_seen <= 0 && dbl)
		dbl2--;
	h = xshift (h, dbl1);
	dy = xshift (dy, dbl2);

	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;
	if (dx < 0)
		dx = 0;
	if (dy < 0)
		dy = 0;
	*pw = w; *ph = h;
	*pdx = dx; *pdy = dy;
}

static bool get_genlock_very_rare_and_complex_case(uae_u8 v)
{
	if (ecs_genlock_features_colorkey) {
		if (currprefs.genlock_effects) {
			if (v < 64 && (currprefs.ecs_genlock_features_colorkey_mask[0] & (1LL << v))) {
				return false;
			}
			if (v >= 64 && v < 128 && (currprefs.ecs_genlock_features_colorkey_mask[1] & (1LL << (v - 64)))) {
				return false;
			}
			if (v >= 128 && v < 192 && (currprefs.ecs_genlock_features_colorkey_mask[2] & (1LL << (v - 128)))) {
				return false;
			}
			if (v >= 192 && v < 256 && (currprefs.ecs_genlock_features_colorkey_mask[3] & (1LL << (v - 192)))) {
				return false;
			}
		} else {
			// color key match?
			if (denise_colors.color_regs_genlock[v]) {
				return false;
			}
		}
	}
	// plane mask match?
	if (currprefs.genlock_effects) {
		if (v & currprefs.ecs_genlock_features_plane_mask)
			return false;
	} else {
		if (v & ecs_genlock_features_mask)
			return false;
	}
	return true;
}
// false = transparent
static bool get_genlock_transparency(uae_u8 v)
{
	if (!ecs_genlock_features_active) {
		if (v == 0)
			return false;
		return true;
	} else {
		return get_genlock_very_rare_and_complex_case(v);
	}
}
static bool get_genlock_transparency_fast(uae_u8 v)
{
	if (!ecs_genlock_features_active) {
		if (v == 0)
			return false;
		return true;
	} else {
		return get_genlock_very_rare_and_complex_case(v);
	}
}

static bool get_genlock_transparency_border(void)
{
	if (!ecs_genlock_features_active) {
		return false;
	} else {
		// border color with BRDNTRAN bit set = not transparent
		if (bplcon3_denise & 0x0010)
			return true;
		return get_genlock_very_rare_and_complex_case(0);
	}
}

static bool get_genlock_transparency_border_fast(uae_u16 bplcon3)
{
	if (!ecs_genlock_features_active) {
		return false;
	} else {
		// border color with BRDNTRAN bit set = not transparent
		if (bplcon3 & 0x0010)
			return true;
		return get_genlock_very_rare_and_complex_case(0);
	}
}


static void gen_pfield_tables(void)
{
	for (int i = 0; i < 256; i++) {
		int plane1 = ((i >> 0) & 1) | ((i >> 1) & 2) | ((i >> 2) & 4) | ((i >> 3) & 8);
		int plane2 = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4) | ((i >> 4) & 8);

		dblpf_2nd1[i] = plane1 == 0 && plane2 != 0;
		dblpf_2nd2[i] = plane2 != 0;

#ifdef AGA
		dblpf_ind1_aga[i] = plane1 == 0 ? plane2 : plane1;
		dblpf_ind2_aga[i] = plane2 == 0 ? plane1 : plane2;
#endif

		dblpf_ms1[i] = plane1 == 0 ? (plane2 == 0 ? 16 : 8) : 0;
		dblpf_ms2[i] = plane2 == 0 ? (plane1 == 0 ? 16 : 0) : 8;
		dblpf_ms[i] = i == 0 ? 16 : 8;

		if (plane2 > 0)
			plane2 += 8;
		// use OCS/ECS unused plane bits 6 and 7 for 
		// dualplayfield BPLCON2 invalid value emulation.
		int plane1x = (i & 0x40) ? 0 : plane1;
		int plane2x = (i & 0x80) ? 0 : plane2;
		dblpf_ind1[i] = plane1 == 0 ? plane2x : plane1x;
		dblpf_ind2[i] = plane2 == 0 ? plane1x : plane2x;

		sprite_offs[i] = (i & 15) ? 0 : 2;
	}
}

static void init_aspect_maps(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = vidinfo->inbuffer;
	int i, maxl, h;

	linedbld = linedbl = currprefs.gfx_vresolution;
	if (doublescan > 0 && interlace_seen <= 0) {
		linedbl = 0;
		linedbld = 1;
	}
	maxl = (MAXVPOS + 1) << linedbld;
	min_ypos_for_screen = minfirstline << linedbl;
	max_drawn_amiga_line = -1;

	vidinfo->xchange = 1 << (RES_MAX - currprefs.gfx_resolution);
	vidinfo->ychange = linedbl ? 1 : 2;

	visible_left_start = 0;
	visible_right_stop = MAX_STOP;
	visible_top_start = 0;
	visible_bottom_stop = MAX_STOP;

	h = vb->height_allocated;
	if (h == 0)
		/* Do nothing if the gfx driver hasn't initialized the screen yet */
		return;

	if (native2amiga_line_map)
		xfree (native2amiga_line_map);
	if (amiga2aspect_line_map)
		xfree (amiga2aspect_line_map);

	/* At least for this array the +1 is necessary. */
	native2amiga_line_map_height = h;
	amiga2aspect_line_map = xmalloc (int, (MAXVPOS + 1) * 2 + 1);
	native2amiga_line_map = xmalloc (int, native2amiga_line_map_height);

	for (i = 0; i < maxl; i++) {
		int v = i - min_ypos_for_screen;
		if (v >= h && max_drawn_amiga_line < 0)
			max_drawn_amiga_line = v;
		if (i < min_ypos_for_screen || v >= native2amiga_line_map_height)
			v = -1;
		amiga2aspect_line_map[i] = v;
	}
	if (max_drawn_amiga_line < 0)
		max_drawn_amiga_line = maxl - min_ypos_for_screen;

	for (i = 0; i < native2amiga_line_map_height; i++)
		native2amiga_line_map[i] = -1;

	for (i = maxl - 1; i >= min_ypos_for_screen; i--) {
		int j;
		if (amiga2aspect_line_map[i] == -1)
			continue;
		for (j = amiga2aspect_line_map[i]; j < native2amiga_line_map_height && native2amiga_line_map[j] == -1; j++)
			native2amiga_line_map[j] = i >> linedbl;
	}
}

void init_row_map(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = vidinfo->inbuffer;
	static uae_u8 *oldbufmem;
	static struct vidbuffer *oldvb;
	static int oldheight, oldpitch;
	static bool oldgenlock, oldburst;
	int i, j;

	if (vb->height_allocated > max_uae_height) {
		write_log(_T("Resolution too high, aborting\n"));
		abort();
	}
	if (!row_map) {
		row_map = xmalloc(uae_u8 *, max_uae_height + 1);
		row_map_genlock = xmalloc(uae_u8 *, max_uae_height + 1);
	}

	if (oldbufmem && oldbufmem == vb->bufmem &&
		oldvb == vb &&
		oldheight == vb->height_allocated &&
		oldpitch == vb->rowbytes &&
		oldgenlock == init_genlock_data &&
		oldburst == (row_map_color_burst_buffer ? 1 : 0)) {
			return;
	}
	xfree(row_map_genlock_buffer);
	row_map_genlock_buffer = NULL;
	if (init_genlock_data) {
		row_map_genlock_buffer = xcalloc(uae_u8, vb->width_allocated * (vb->height_allocated + 2));
	}
	xfree(row_map_color_burst_buffer);
	row_map_color_burst_buffer = NULL;
	if (currprefs.cs_color_burst) {
		row_map_color_burst_buffer = xcalloc(uae_u8, vb->height_allocated + 2);
	}
	for (i = 0, j = 0; i < vb->height_allocated; i++, j += vb->rowbytes) {
		if (i < vb->outheight) {
			row_map[i] = vb->bufmem + j;
		} else {
			row_map[i] = row_tmp8;
		}
		if (init_genlock_data) {
			row_map_genlock[i] = row_map_genlock_buffer + vb->width_allocated * (i + 1);
		} else {
			row_map_genlock[i] = NULL;
		}
	}
	while (i < max_uae_height + 1) {
		row_map[i] = row_tmp8;
		row_map_genlock[i] = row_tmp8g;
		i++;
	}
	oldvb = vb;
	oldbufmem = vb->bufmem;
	oldheight = vb->height_allocated;
	oldpitch = vb->rowbytes;
	oldgenlock = init_genlock_data;
	oldburst = row_map_color_burst_buffer ? 1 : 0;

	init_aspect_maps();
}

static bool cancenter(void)
{
	struct amigadisplay* ad = &adisplays[0];
	struct gfx_filterdata* fd = &currprefs.gf[ad->gf_index];

	return !currprefs.genlock_image &&
		(fd->gfx_filter_autoscale != AUTOSCALE_NORMAL &&
			fd->gfx_filter_autoscale != AUTOSCALE_RESIZE &&
			fd->gfx_filter_autoscale != AUTOSCALE_MANUAL &&
			fd->gfx_filter_autoscale != AUTOSCALE_INTEGER_AUTOSCALE &&
			fd->gfx_filter_autoscale != AUTOSCALE_CENTER);
}

static void center_image (void)
{
	struct amigadisplay *ad = &adisplays[0];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;
	int prev_x_adjust = center_horizontal_offset;
	int prev_y_adjust = thisframe_y_adjust;

	if (!vidinfo->inbuffer) {
		return;
	}

	int w = vidinfo->inbuffer->inwidth;
	int ew = vidinfo->inbuffer->extrawidth;
	int maxdiw = denisehtotal >> (RES_MAX - currprefs.gfx_resolution);
	int xoffset = 0;

	if (currprefs.gfx_overscanmode <= OVERSCANMODE_OVERSCAN && diwlastword_total > 0 && diwlastword_total > diwfirstword_total  && cancenter()) {

		visible_left_border = maxdiw - w;
		visible_left_border &= ~((xshift(1, 0)) - 1);

		int ww = (diwlastword_total - diwfirstword_total) >> (RES_MAX - hresolution);
		int wx = ((diwfirstword_total) >> (RES_MAX - hresolution)) - visible_left_border / 2;

		if (ww < w && currprefs.gfx_xcenter == 2) {
			/* Try to center. */
			xoffset = (w - ww) / 2 - wx / 2;
		}

		if (!center_reset && !vertical_changed) {
			/* Would the old value be good enough? If so, leave it as it is if we want to be clever. */
			if (currprefs.gfx_xcenter == 2) {
				if (abs(xoffset - prev_x_adjust) <= 32) {
					xoffset = prev_x_adjust;
				}
			}
		}

		if (xoffset < -maxdiw / 4 || xoffset > maxdiw / 4) {
			xoffset = 0;
		}

	} else if (ew == -1) {
		// wide mode
		int hs = 0;
		visible_left_border = hs << currprefs.gfx_resolution;
		if (visible_left_border + w > maxdiw) {
			visible_left_border += (maxdiw - (visible_left_border + w) - 1) / 2;
		}
		if (visible_left_border < (hs << currprefs.gfx_resolution)) {
			visible_left_border = hs << currprefs.gfx_resolution;
		}
	} else if (ew < -1) {
		// normal
		visible_left_border = maxdiw - w;
	} else {
		if (vidinfo->inbuffer->inxoffset < 0) {
			visible_left_border = 0;
		} else {
			visible_left_border = vidinfo->inbuffer->inxoffset << currprefs.gfx_resolution;
		}
	}

	if (visible_left_border > maxdiw - 32) {
		visible_left_border = maxdiw - 32;
	}
	if (visible_left_border < 0) {
		visible_left_border = 0;
	}
	if (currprefs.gfx_overscanmode <= OVERSCANMODE_OVERSCAN) {
		if (ecs_denise) {
			visible_left_border += 1 << currprefs.gfx_resolution;
		} else {
			visible_left_border += 2 << currprefs.gfx_resolution;
		}
	} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
		if (ecs_denise) {
			visible_left_border += 3 << currprefs.gfx_resolution;
		} else {
			visible_left_border += 4 << currprefs.gfx_resolution;
		}
	}
	visible_left_border &= ~((1 << currprefs.gfx_resolution) - 1);

	//write_log (_T("%d %d %d %d\n"), max_diwlastword, vidinfo->inbuffer->width, currprefs.gfx_resolution, visible_left_border);

	linetoscr_x_adjust = xoffset;
	center_horizontal_offset = xoffset;

	visible_right_border = maxdiw + w + ((ew > 0 ? ew : 0) << currprefs.gfx_resolution);
	if (visible_right_border > maxdiw + ((ew > 0 ? ew : 0) << currprefs.gfx_resolution)) {
		visible_right_border = maxdiw + ((ew > 0 ? ew : 0) << currprefs.gfx_resolution);
	}

	int max_drawn_amiga_line_tmp = max_drawn_amiga_line;
	if (max_drawn_amiga_line_tmp > vidinfo->inbuffer->inheight) {
		max_drawn_amiga_line_tmp = vidinfo->inbuffer->inheight;
	}
	max_drawn_amiga_line_tmp >>= linedbl;
	max_drawn_amiga_line_tmp -= vsync_startline;
	
	thisframe_y_adjust = minfirstline;
	if (currprefs.gfx_ycenter && cancenter()) {

		if (plffirstline_total >= 0 && (plflastline_total + 1) > plffirstline_total) {

			if (plflastline_total - plffirstline_total < max_drawn_amiga_line_tmp && currprefs.gfx_ycenter == 2) {
				thisframe_y_adjust = ((plflastline_total + 1) - plffirstline_total - max_drawn_amiga_line_tmp) / 2 + plffirstline_total;
			} else {
				thisframe_y_adjust = plffirstline_total;
			}
			thisframe_y_adjust++;

			/* Would the old value be good enough? If so, leave it as it is if we want to be clever. */
			if (!center_reset && !horizontal_changed) {
				if (currprefs.gfx_ycenter == 2 && thisframe_y_adjust != prev_y_adjust && abs(thisframe_y_adjust - prev_y_adjust) < 100) {
					if (prev_y_adjust <= plffirstline_total && prev_y_adjust + max_drawn_amiga_line_tmp > plflastline_total + 1) {
						thisframe_y_adjust = prev_y_adjust;
					}
				}
			}

		} else {

			center_reset = 2;

		}
	}

	/* Make sure the value makes sense */
	thisframe_y_adjust_real = (minfirstline - thisframe_y_adjust) << linedbl;
	if (thisframe_y_adjust_real + max_drawn_amiga_line_tmp > maxvpos_display + maxvpos_display / 2) {
		thisframe_y_adjust_real = maxvpos_display + maxvpos_display / 2 - max_drawn_amiga_line_tmp;
	}
	if (thisframe_y_adjust_real < -maxvpos_display / 2) {
		thisframe_y_adjust_real = -maxvpos_display / 2;
	}
	if (thisframe_y_adjust_real != prev_thisframe_y_adjust_real) {
		center_y_erase = true;
		prev_thisframe_y_adjust_real = thisframe_y_adjust_real;
	}
	if (prev_x_adjust != center_horizontal_offset) {
		center_y_erase = true;
	}

	max_ypos_thisframe1 = (maxvpos_display - minfirstline + maxvpos_display_vsync) << linedbl;

	if (prev_x_adjust != center_horizontal_offset || prev_y_adjust != thisframe_y_adjust) {
		int redraw = interlace_seen > 0 && linedbl ? 2 : 1;
		if (redraw > ad->frame_redraw_necessary) {
			ad->frame_redraw_necessary = redraw;
		}
	}

	vidinfo->inbuffer->xoffset = visible_left_border << (RES_MAX - currprefs.gfx_resolution);
	vidinfo->inbuffer->yoffset = thisframe_y_adjust << VRES_MAX;

	if (center_reset > 0) {
		center_reset--;
	}
	horizontal_changed = false;
	vertical_changed = false;
}

static int frame_res_cnt;
static int autoswitch_old_resolution;

void notice_resolution_seen(int res, bool lace)
{
	if (res > frame_res) {
		frame_res = res;
	}
	if (res > 0) {
		can_use_lores = 0;
	}
	if (!frame_res_lace && lace) {
		frame_res_lace = lace;
	}
}

static void init_drawing_frame(void)
{
	struct amigadisplay *ad = &adisplays[0];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;
	static int frame_res_old;

	int largest_res = 0;
	int largest_count = 0;
	int largest_count_res = 0;
	for (int i = 0; i <= RES_MAX; i++) {
		if (resolution_count[i]) {
			largest_res = i;
		}
		if (resolution_count[i] >= largest_count) {
			largest_count = resolution_count[i];
			largest_count_res = i;
		}
	}
	if (currprefs.gfx_resolution == changed_prefs.gfx_resolution && lines_count > 0) {
		detected_screen_resolution = largest_res;
	}

	if (currprefs.gfx_resolution == changed_prefs.gfx_resolution && lines_count > 0) {

		if (currprefs.gfx_autoresolution_vga && programmedmode == 1 && vidinfo->gfx_resolution_reserved >= RES_HIRES && vidinfo->gfx_vresolution_reserved >= VRES_DOUBLE) {
			if (largest_res == RES_SUPERHIRES && (vidinfo->gfx_resolution_reserved < RES_SUPERHIRES || vidinfo->gfx_vresolution_reserved < 1)) {
				// enable full doubling/superhires support if programmed mode. It may be "half-width" only and may fit in normal display window.
				vidinfo->gfx_resolution_reserved = RES_SUPERHIRES;
				vidinfo->gfx_vresolution_reserved = VRES_DOUBLE;
				graphics_reset(false);
			}
			int newres = RES_HIRES;
			int hres = (2 * htotal) << largest_res;
			if (hres > 1150) {
				newres = RES_SUPERHIRES;
			} else {
				newres = RES_HIRES;
			}
			if (newres < RES_HIRES) {
				newres = RES_HIRES;
			}
			if (newres > RES_MAX) {
				newres = RES_MAX;
			}
			if (changed_prefs.gfx_resolution != newres) {
				autoswitch_old_resolution = RES_HIRES;
				write_log(_T("Programmed mode autores = %d -> %d (%d)\n"), changed_prefs.gfx_resolution, newres, largest_res);
				changed_prefs.gfx_resolution = newres;
				set_config_changed();
				return;
			}
		} else if (autoswitch_old_resolution == RES_HIRES) {
			autoswitch_old_resolution = 0;
			if (changed_prefs.gfx_resolution != RES_HIRES) {
				changed_prefs.gfx_resolution = RES_HIRES;
				set_config_changed();
				return;
			}
		}

		if (currprefs.gfx_autoresolution) {
			int frame_res_detected;
			int frame_res_lace_detected = frame_res_lace;

			if (currprefs.gfx_autoresolution == 1 || currprefs.gfx_autoresolution >= 100)
				frame_res_detected = largest_res;
			else if (largest_count * 100 / lines_count >= currprefs.gfx_autoresolution)
				frame_res_detected = largest_count_res;
			else
				frame_res_detected = largest_count_res - 1;
			if (frame_res_detected < 0)
				frame_res_detected = 0;
	#if 0
			static int delay;
			delay--;
			if (delay < 0) {
				delay = 50;
				write_log (_T("%d %d, %d %d %d, %d %d, %d %d\n"), currprefs.gfx_autoresolution, lines_count, resolution_count[0], resolution_count[1], resolution_count[2],
					largest_count, largest_count_res, frame_res_detected, frame_res_lace_detected);
			}
	#endif
			if (frame_res_detected >= 0 && frame_res_lace_detected >= 0) {
				if (frame_res_cnt > 0 && frame_res_old == frame_res_detected * 2 + frame_res_lace_detected) {
					frame_res_cnt--;
					if (frame_res_cnt == 0) {
						int m = frame_res_detected * 2 + frame_res_lace_detected;
						struct wh *dst = currprefs.gfx_apmode[0].gfx_fullscreen ? &changed_prefs.gfx_monitor[0].gfx_size_fs : &changed_prefs.gfx_monitor[0].gfx_size_win;
						struct wh *src = currprefs.gfx_apmode[0].gfx_fullscreen ? &currprefs.gfx_monitor[0].gfx_size_fs_xtra[m] : &currprefs.gfx_monitor[0].gfx_size_win_xtra[m];
						int nr = m >> 1;
						int nl = (m & 1) == 0 ? 0 : 1;
						int nr_o = nr;
						int nl_o = nl;

						if (currprefs.gfx_autoresolution >= 100 && nl == 0 && nr > 0) {
							nl = 1;
						}
						if (currprefs.gfx_autoresolution_minh < 0) {
							if (nr < nl)
								nr = nl;
						} else if (nr < currprefs.gfx_autoresolution_minh) {
							nr = currprefs.gfx_autoresolution_minh;
						}
						if (currprefs.gfx_autoresolution_minv < 0) {
							if (nl < nr)
								nl = nr;
						} else if (nl < currprefs.gfx_autoresolution_minv) {
							nl = currprefs.gfx_autoresolution_minv;
						}

						if (nr > vidinfo->gfx_resolution_reserved)
							nr = vidinfo->gfx_resolution_reserved;
						if (nl > vidinfo->gfx_vresolution_reserved)
							nl = vidinfo->gfx_vresolution_reserved;

						if (changed_prefs.gfx_resolution != nr || changed_prefs.gfx_vresolution != nl) {
							changed_prefs.gfx_resolution = nr;
							changed_prefs.gfx_vresolution = nl;

							write_log(_T("RES -> %d (%d) LINE -> %d (%d) (%d - %d, %d - %d)\n"), nr, nr_o, nl, nl_o,
								currprefs.gfx_autoresolution_minh, currprefs.gfx_autoresolution_minv,
								vidinfo->gfx_resolution_reserved, vidinfo->gfx_vresolution_reserved);
							set_config_changed();
						}
						if (src->width > 0 && src->height > 0) {
							if (memcmp(dst, src, sizeof(*dst))) {
								*dst = *src;
								set_config_changed();
							}
						}
						frame_res_cnt = currprefs.gfx_autoresolution_delay;
					}
				} else {
					frame_res_old = frame_res_detected * 2 + frame_res_lace_detected;
					frame_res_cnt = currprefs.gfx_autoresolution_delay;
					if (frame_res_cnt <= 0)
						frame_res_cnt = 1;
				}
			}
		}
	}
	for (int i = 0; i <= RES_MAX; i++) {
		resolution_count[i] = 0;
	}
	lines_count = 0;
	frame_res = -1;
	frame_res_lace = 0;

	if (thisframe_first_drawn_line < 0) {
		thisframe_first_drawn_line = minfirstline;
	}
	if (thisframe_first_drawn_line > thisframe_last_drawn_line) {
		thisframe_last_drawn_line = thisframe_first_drawn_line;
	}

	last_drawn_line = 0;
	first_drawn_line = 32767;

	if (ad->frame_redraw_necessary) {
		ad->custom_frame_redraw_necessary = 1;
		ad->frame_redraw_necessary--;
	} else {
		ad->custom_frame_redraw_necessary = 0;
	}

	center_image ();

	thisframe_first_drawn_line = -1;
	thisframe_last_drawn_line = -1;
}

static int lightpen_y1[2], lightpen_y2[2];

void putpixel(uae_u8 *buf, uae_u8 *genlockbuf, int x, xcolnr c8)
{
	if (x <= 0)
		return;

	if (genlockbuf) {
		genlockbuf[x] = 1;
	}

	uae_u32 *p = (uae_u32*)buf + x;
	*p = c8;
}

static void setxlinebuffer(int monid, int line)
{
	struct vidbuf_description* vidinfo = &adisplays[monid].gfxvidinfo;

	line += thisframe_y_adjust_real;
	if (line < 0 || line >= max_uae_height) {
		xlinebuffer = row_map[max_uae_height - 1];
		xlinebuffer_genlock = NULL;

		xlinebuffer_start = xlinebuffer;
		xlinebuffer_end = xlinebuffer + (vidinfo->inbuffer->outwidth * sizeof(uae_u32));

	} else {
		xlinebuffer = row_map[line];

		xlinebuffer_start = xlinebuffer;
		xlinebuffer_end = xlinebuffer + (vidinfo->inbuffer->outwidth * sizeof(uae_u32));

		xlinebuffer_genlock = row_map_genlock[line];
		if (xlinebuffer_genlock) {
			xlinebuffer_genlock_start = xlinebuffer_genlock;
			xlinebuffer_genlock_end = xlinebuffer_genlock + (vidinfo->inbuffer->outwidth);
		}

	}
}

static uae_u8 *status_line_ptr(int monid, int line)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	int y;

	y = line - (vidinfo->inbuffer->outheight - TD_TOTAL_HEIGHT);
	xlinebuffer = row_map[line];
	xlinebuffer_genlock = row_map_genlock[line];
	return xlinebuffer;
}

static void draw_status_line(int monid, int line, int statusy)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	uae_u8 *buf = status_line_ptr(monid, line);
	if (!buf)
		return;
	if (statusy < 0)
		statusline_render(monid, buf, vidinfo->inbuffer->rowbytes, vidinfo->inbuffer->outwidth, TD_TOTAL_HEIGHT, xredcolors, xgreencolors, xbluecolors, NULL);
	else
		draw_status_line_single(monid, buf, statusy, vidinfo->inbuffer->outwidth, xredcolors, xgreencolors, xbluecolors, NULL);
}

static void draw_debug_status_line(int monid, int line)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	xlinebuffer = row_map[line];
	xlinebuffer_genlock = row_map_genlock[line];
#ifdef DEBUGGER
	debug_draw(xlinebuffer, xlinebuffer_genlock, line, vidinfo->inbuffer->outwidth, vidinfo->inbuffer->outheight, xredcolors, xgreencolors, xbluecolors);
#endif
}

#define LIGHTPEN_HEIGHT 12
#define LIGHTPEN_WIDTH 17

static const char *lightpen_cursor = {
	"------.....------"
	"------.xxx.------"
	"------.xxx.------"
	"------.xxx.------"
	".......xxx......."
	".xxxxxxxxxxxxxxx."
	".xxxxxxxxxxxxxxx."
	".......xxx......."
	"------.xxx.------"
	"------.xxx.------"
	"------.xxx.------"
	"------.....------"
};

static void draw_lightpen_cursor(int monid, int x, int y, int line, int onscreen, int lpnum)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	const char *p;
	int color1 = onscreen ? (lpnum ? 0x0ff : 0xff0) : (lpnum ? 0x0f0 : 0xf00);
	int color2 = (color1 & 0xeee) >> 1;

	setxlinebuffer(monid, line);

	p = lightpen_cursor + y * LIGHTPEN_WIDTH;
	for (int i = 0; i < LIGHTPEN_WIDTH; i++) {
		int xx = x + i - LIGHTPEN_WIDTH / 2;
		if (*p != '-' && xx >= 0 && xx < vidinfo->inbuffer->outwidth && (xx * 4 + xlinebuffer >= xlinebuffer_start && xx * 4 + xlinebuffer < xlinebuffer_end)) {
			putpixel(xlinebuffer, xlinebuffer_genlock, xx, *p == 'x' ? xcolors[color1] : xcolors[color2]);
		}
		p++;
	}
}

static void lightpen_update(struct vidbuffer *vb, int lpnum)
{
	struct vidbuf_description *vidinfo = &adisplays[vb->monitor_id].gfxvidinfo;
	if (lightpen_x[lpnum] < 0 && lightpen_y[lpnum] < 0)
		return;

	bool out = false;
	int extra = 2;

	if (lightpen_x[lpnum] < -extra)
		lightpen_x[lpnum] = -extra;
	if (lightpen_x[lpnum] >= vidinfo->inbuffer->inwidth + extra)
		lightpen_x[lpnum] = vidinfo->inbuffer->inwidth + extra;
	if (lightpen_y[lpnum] < -extra)
		lightpen_y[lpnum] = -extra;
	if (lightpen_y[lpnum] >= vidinfo->inbuffer->inheight + extra)
		lightpen_y[lpnum] = vidinfo->inbuffer->inheight + extra;
	if (lightpen_y[lpnum] >= max_ypos_thisframe1)
		lightpen_y[lpnum] = max_ypos_thisframe1;

	if (lightpen_x[lpnum] < 0 || lightpen_y[lpnum] < 0) {
		out = true;
	}
	if (lightpen_x[lpnum] >= vidinfo->inbuffer->inwidth) {
		out = true;
	}
	if (lightpen_y[lpnum] >= max_ypos_thisframe1) {
		out = true;
	}

	int cx = ((lightpen_x[lpnum] + visible_left_border) >> (1 + currprefs.gfx_resolution)) + 29;

	int cy = lightpen_y[lpnum];
	cy >>= linedbl;
	cy += minfirstline;

	cx += currprefs.lightpen_offset[0];
	cy += currprefs.lightpen_offset[1];

	if (cx <= 0x18 - 1) {
		cx = 0x18 - 1;
		out = true;
	}
	if (cy <= minfirstline_linear - 1) {
		cy = minfirstline_linear - 1;
		out = true;
	}
	if (cy >= current_linear_vpos) {
		cy = current_linear_vpos;
		out = true;
	}

	if (currprefs.lightpen_crosshair && lightpen_active) {
		for (int i = 0; i < LIGHTPEN_HEIGHT; i++) {
			int line = lightpen_y[lpnum] + i - LIGHTPEN_HEIGHT / 2;
			if (line >= 0 && line < max_ypos_thisframe1) {
				if (lightpen_active & (1 << lpnum)) {
					if (denise_lock()) {
						draw_lightpen_cursor(vb->monitor_id, lightpen_x[lpnum], i, line, cx > 0, lpnum);
					}
				}
			}
		}
	}

	lightpen_y1[lpnum] = lightpen_y[lpnum] - LIGHTPEN_HEIGHT / 2 - 1 + (thisframe_y_adjust_real >> linedbl);
	lightpen_y2[lpnum] = lightpen_y1[lpnum] + LIGHTPEN_HEIGHT + 1 + (thisframe_y_adjust_real >> linedbl);

	lightpen_cx[lpnum] = out ? -1 : cx;
	lightpen_cy[lpnum] = out ? -1 : cy;
}

static void refresh_indicator_init(void)
{
	xfree(refresh_indicator_buffer);
	refresh_indicator_buffer = NULL;
	xfree(refresh_indicator_changed);
	refresh_indicator_changed = NULL;
	xfree(refresh_indicator_changed_prev);
	refresh_indicator_changed_prev = NULL;

	if (!currprefs.refresh_indicator)
		return;

	refresh_indicator_height = 724;
	refresh_indicator_buffer = xcalloc(uae_u8, MAX_PIXELS_PER_LINE * sizeof(uae_u32) * refresh_indicator_height);
	refresh_indicator_changed = xcalloc(uae_u8, refresh_indicator_height);
	refresh_indicator_changed_prev = xcalloc(uae_u8, refresh_indicator_height);
}

bool drawing_can_lineoptimizations(void)
{
	if (currprefs.monitoremu || currprefs.cs_cd32fmv || ((currprefs.genlock || currprefs.genlock_effects) && currprefs.genlock_image) ||
		currprefs.cs_color_burst || currprefs.gfx_grayscale || currprefs.monitoremu) {
		return false;
	}
	if (lightpen_active || debug_dma >= 2 || debug_heatmap >= 2) {
		return false;
	}
	return true;
}

static void draw_frame_extras(struct vidbuffer *vb, int y_start, int y_end)
{
#ifdef DEBUGGER
	if (debug_dma > 1 || debug_heatmap > 1) {
		if (denise_lock()) {
			for (int i = 0; i < vb->outheight; i++) {
				int line = i;
				draw_debug_status_line(vb->monitor_id, line);
			}
		}
	}
#endif

	if (lightpen_active) {
		if (lightpen_active & 1) {
			lightpen_update(vb, 0);
		}
		if (inputdevice_get_lightpen_id() >= 0 && (lightpen_active & 2)) {
			lightpen_update(vb, 1);
		}
	}
}

extern bool beamracer_debug;

static void setnativeposition(struct vidbuffer *vb)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	vb->inwidth = vidinfo->inbuffer->inwidth;
	vb->inheight = vidinfo->inbuffer->inheight;
	vb->inwidth2 = vidinfo->inbuffer->inwidth2;
	vb->inheight2 = vidinfo->inbuffer->inheight2;
	vb->outwidth = vidinfo->inbuffer->outwidth;
	vb->outheight = vidinfo->inbuffer->outheight;
}

static void setspecialmonitorpos(struct vidbuffer *vb)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	vb->extrawidth = vidinfo->inbuffer->extrawidth;
	vb->xoffset = vidinfo->inbuffer->xoffset;
	vb->yoffset = vidinfo->inbuffer->yoffset;
	vb->inxoffset = vidinfo->inbuffer->inxoffset;
	vb->inyoffset = vidinfo->inbuffer->inyoffset;
}

static void vbcopy(struct vidbuffer *vbout, struct vidbuffer *vbin)
{
	if (vbout->locked) {
		for (int h = 0; h < vbout->height_allocated && h < vbin->height_allocated; h++) {
			memcpy(vbout->bufmem + h * vbout->rowbytes, vbin->bufmem + h * vbin->rowbytes, (vbin->width_allocated > vbout->width_allocated ? vbout->width_allocated : vbin->width_allocated) * vbout->pixbytes);
		}
	}
}

static void finish_drawing_frame(bool drawlines)
{
	int monid = 0;
	bool vbcopied = false;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;
	struct vidbuffer *vbout = vidinfo->outbuffer;
	struct vidbuffer *vbin = vidinfo->inbuffer;

	if (!drawlines || !vbout || !vbin) {
		return;
	}

	vbout->last_drawn_line = 0;
	vbout->hardwiredpositioning = false;

	draw_frame_extras(vbin, -1, -1);

#ifdef WITH_SPECIALMONITORS
	// video port adapters
	if (currprefs.monitoremu) {
		if (!denise_lock()) {
			return;
		}
		struct vidbuf_description *outvi = &adisplays[currprefs.monitoremu_mon].gfxvidinfo;
		struct vidbuffer *m_out = &outvi->drawbuffer;
		struct vidbuffer *m_in = &outvi->tempbuffer;
		if (init_genlock_data != specialmonitor_need_genlock()) {
			init_genlock_data = specialmonitor_need_genlock();
			init_row_map();
		}
		bool locked = true;
		bool multimon = currprefs.monitoremu_mon != 0;
		if (multimon) {
			locked = lockscr(m_out, false, display_reset > 0);
			outvi->xchange = vidinfo->xchange;
			outvi->ychange = vidinfo->ychange;
		} else {
			m_out = vbout;
		}
		setspecialmonitorpos(m_out);
		if (locked && emulate_specialmonitors(m_in, m_out)) {
			if (!m_in->hardwiredpositioning) {
				setnativeposition(m_out);
			}
			if (!ad->specialmonitoron) {
				need_genlock_data = specialmonitor_need_genlock();
				ad->specialmonitoron = true;
				compute_framesync();
			}
		} else {
			need_genlock_data = false;
			if (ad->specialmonitoron) {
				ad->specialmonitoron = false;
				m_in->hardwiredpositioning = false;
				compute_framesync();
			}
			vbcopy(vbout, vbin);
			vbcopied = true;
		}
		if (multimon && locked) {
			unlockscr(m_out, -1, -1);
			render_screen(m_out->monitor_id, 1, true);
			show_screen(m_out->monitor_id, 0);
		}
		vbcopied = true;
	}

	// genlock
	if (currprefs.genlock_image && (currprefs.genlock || currprefs.genlock_effects) && !currprefs.monitoremu && vidinfo->tempbuffer.bufmem_allocated) {
		if (!denise_lock()) {
			return;
		}
		setspecialmonitorpos(vbout);
		if (init_genlock_data != specialmonitor_need_genlock()) {
			need_genlock_data = init_genlock_data = specialmonitor_need_genlock();
			init_row_map();
			lts_request = true;
		}
		emulate_genlock(vbin, vbout, aga_genlock_features_zdclken);
		if (!vbout->hardwiredpositioning) {
			setnativeposition(vbout);
		}
		vbcopied = true;
	}
#endif

#ifdef CD32
	// cd32 fmv
	if (!currprefs.monitoremu && vidinfo->tempbuffer.bufmem_allocated && currprefs.cs_cd32fmv) {
		if (!denise_lock()) {
			return;
		}
		if (cd32_fmv_active) {
			cd32_fmv_genlock(vbin, vbout);
			setnativeposition(vbout);
		} else {
			vbcopy(vbout, vbin);
		}
		vbcopied = true;
	}
#endif

	// grayscale
#ifdef WITH_SPECIALMONITORS
	if (!currprefs.monitoremu && vidinfo->tempbuffer.bufmem_allocated &&
		((!currprefs.genlock && (!bplcolorburst_field && currprefs.cs_color_burst)) || currprefs.gfx_grayscale)) {
		if (!denise_lock()) {
			return;
		}
		setspecialmonitorpos(vbout);
		emulate_grayscale(vbin, vbout);
		if (!vbout->hardwiredpositioning) {
			setnativeposition(vbout);
		}
		vbcopied = true;
	}
#endif

	if (denise_locked) {
		if (!vbcopied) {
			vbcopy(vbout, vbin);
		}
		unlockscr(vbout, display_reset ? -2 : -1, -1);
		denise_locked = false;
	}
#ifdef AMIBERRY
	if (currprefs.gfx_auto_crop)
		auto_crop_image();
#endif
}

void check_prefs_picasso(void)
{
#ifdef PICASSO96
	for (int monid = 0; monid < MAX_AMIGAMONITORS; monid++) {
		struct amigadisplay *ad = &adisplays[monid];

		if (ad->picasso_on && ad->picasso_redraw_necessary) {
			picasso_refresh(monid);
		}
		ad->picasso_redraw_necessary = 0;

		if (ad->picasso_requested_on == ad->picasso_on && !ad->picasso_requested_forced_on) {
			continue;
		}
		 
		devices_unsafeperiod();

		if (!ad->picasso_requested_on && monid > 0) {
			ad->picasso_requested_on = ad->picasso_on;
			ad->picasso_requested_forced_on = false;
			continue;
		}

		if (ad->picasso_requested_on) {
			if (!toggle_rtg(monid, -2)) {
				ad->picasso_requested_forced_on = false;
				ad->picasso_on = false;
				ad->picasso_requested_on = false;
				continue;
			}
		}

		ad->picasso_requested_forced_on = false;
		ad->picasso_on = ad->picasso_requested_on;

		if (!ad->picasso_on) {
			clear_inhibit_frame(monid, IHF_PICASSO);
			ad->gf_index = ad->interlace_on ? GF_INTERLACE : GF_NORMAL;
		} else {
			set_inhibit_frame(monid, IHF_PICASSO);
			ad->gf_index = GF_RTG;
		}

		gfx_set_picasso_state(monid, ad->picasso_on);
		picasso_enablescreen(monid, ad->picasso_requested_on);

		notice_screen_contents_lost(monid);
		notice_new_xcolors();
		count_frame(monid);
		compute_framesync();
	}
#endif
}

void redraw_frame(void)
{
	last_drawn_line = 0;
	first_drawn_line = 32767;
	// do not do full refresh if lagless mode
	// because last line would appear as bright line
	if (isvsync_chipset() < 0)
		return;
	finish_drawing_frame(true);
}

void full_redraw_all(void)
{
	int monid = 0;
	bool redraw = false;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	if (vidinfo->inbuffer && vidinfo->inbuffer->height_allocated && amiga2aspect_line_map) {
		notice_screen_contents_lost(monid);
		if (!ad->picasso_on) {
			redraw_frame();
			redraw = true;
		}
	}
	if (ad->picasso_on) {
		gfxboard_refresh(monid);
		redraw = true;
	}
	if (redraw) {
		render_screen(0, 1, true);
		show_screen(0, 0);
	}
}

bool vsync_handle_check(void)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	int changed = check_prefs_changed_gfx();
	if (changed) {
		custom_end_drawing();
	}
	if (changed > 0) {
		reset_drawing();
		notice_screen_contents_lost(monid);
		notice_new_xcolors();
	} else if (changed < 0) {
		reset_drawing();
		notice_screen_contents_lost(monid);
		notice_new_xcolors();
	}
	if (config_changed) {
		device_check_config();
	}
	return changed != 0;
}

void vsync_handle_redraw(int long_field, uae_u16 bplcon0p, uae_u16 bplcon3p, bool drawlines, bool initial)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];

	check_prefs_picasso();

	last_redraw_point++;
	if (interlace_seen <= 0 || (currprefs.gfx_iscanlines && interlace_seen > 0) || last_redraw_point >= 2 || long_field || doublescan < 0) {
		last_redraw_point = 0;

		if (!initial) {
			if (ad->framecnt == 0) {
				finish_drawing_frame(drawlines);
#ifdef AVIOUTPUT
				if (!ad->picasso_on) {
					frame_drawn(monid);
				}
#endif
			}
			count_frame(monid);
		}
#if 0
		if (interlace_seen > 0) {
			interlace_seen = -1;
		}
		else if (interlace_seen == -1) {
			interlace_seen = 0;
			if (currprefs.gfx_scandoubler && currprefs.gfx_vresolution)
				notice_screen_contents_lost();
		}
#endif

		if (ad->framecnt == 0) {
			init_drawing_frame();
		}
	} else {
#if 0
		if (isvsync_chipset()) {
			flush_screen(vidinfo->inbuffer, 0, 0); /* vsync mode */
		}
#endif
	}

	gui_flicker_led(-1, 0, 0);
	denise_accurate_mode = currprefs.cpu_memory_cycle_exact || currprefs.cs_optimizations >= DISPLAY_OPTIMIZATIONS_PARTIAL || (currprefs.cpu_model <= 68020 && currprefs.m68k_speed >= 0 && currprefs.cpu_compatible);
}

static int  dummy_lock(struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
	return 1;
}

static void dummy_unlock(struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
}

static void gfxbuffer_reset(int monid)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	vidinfo->drawbuffer.lockscr = dummy_lock;
	vidinfo->drawbuffer.unlockscr = dummy_unlock;
}

void allocvidbuffer(int monid, struct vidbuffer *buf, int width, int height, int depth)
{
	memset(buf, 0, sizeof (struct vidbuffer));
	buf->monitor_id = monid;
	buf->pixbytes = (depth + 7) / 8;
	buf->width_allocated = (width + 7) & ~7;
	buf->height_allocated = height;

	buf->outwidth = buf->width_allocated;
	buf->outheight = buf->height_allocated;
	buf->inwidth = buf->width_allocated;
	buf->inheight = buf->height_allocated;

	buf->rowbytes = buf->width_allocated * buf->pixbytes;
	int size = buf->rowbytes * buf->height_allocated;
	buf->realbufmem = xcalloc(uae_u8, size + 2 * buf->rowbytes);
	buf->bufmem_allocated = buf->bufmem = buf->realbufmem + buf->rowbytes;
	buf->bufmemend = buf->realbufmem + size - buf->rowbytes;
}

void freevidbuffer(int monid, struct vidbuffer *buf)
{
	if (!buf->vram_buffer) {
		xfree(buf->realbufmem);
		memset(buf, 0, sizeof (struct vidbuffer));
	}
}

void reset_drawing(void)
{
	custom_end_drawing();

	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;

	syncdebug = currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA;
	exthblank = 0;
	exthblanken = false;
	display_reset = 1;
	denise_sprite_blank_active = false;
	delayed_sprite_vblank_ecs = false;

	last_redraw_point = 0;

	notice_screen_contents_lost(monid);
	init_drawing_frame();

	frame_res_cnt = currprefs.gfx_autoresolution_delay;
	lightpen_y1[0] = lightpen_y2[0] = -1;
	lightpen_y1[1] = lightpen_y2[1] = -1;

	reset_custom_limits();

	clearbuffer(&vidinfo->drawbuffer);
	clearbuffer(&vidinfo->tempbuffer);

	center_reset = 1;
	ad->specialmonitoron = false;
	bplcolorburst_field = 1;
	ecs_genlock_features_active = false;
	aga_genlock_features_zdclken = false;
	ecs_genlock_features_colorkey = false;

	denise_reset(false);
	select_lts();

	no_denise_lol = !currprefs.cpu_memory_cycle_exact;
}

static void gen_direct_drawing_table(void)
{
#ifdef AGA
	// BYPASS grayscale color table
	for (int i = 0; i < 256; i++) {
		uae_u32 v = (i << 16) | (i << 8) | i;
		direct_colors_for_drawing_bypass.acolors[i] = CONVERT_RGB(v);
	}
#endif
}

void drawing_init(void)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;

	if (!denise_thread_state) {
		uae_sem_init(&read_sem, 0, 1);
		uae_sem_init(&write_sem, 0, 1);
		denise_thread_state = 1;
		uae_start_thread(_T("denise"), denise_thread, NULL, NULL);
	}

	refresh_indicator_init();

	gen_pfield_tables();

	gen_direct_drawing_table();

	uae_sem_init(&gui_sem, 0, 1);
#ifdef PICASSO96
	if (!isrestore()) {
		ad->picasso_on = 0;
		ad->picasso_requested_on = 0;
		ad->gf_index = GF_NORMAL;
		gfx_set_picasso_state(0, 0);
	}
#endif
	xlinebuffer = NULL;
	xlinebuffer2 = NULL;
	xlinebuffer_genlock = NULL;

	ad->inhibit_frame = 0;

	gfxbuffer_reset(0);
	reset_drawing();
}

int isvsync_chipset(void)
{
	struct amigadisplay *ad = &adisplays[0];
	if (ad->picasso_on || currprefs.gfx_apmode[0].gfx_vsync <= 0)
		return 0;
	if (currprefs.gfx_apmode[0].gfx_vsyncmode == 0)
		return 1;
	if (currprefs.m68k_speed >= 0)
		return -1;
	return currprefs.cachesize ? -3 : -2;
}

int isvsync_rtg(void)
{
	struct amigadisplay *ad = &adisplays[0];
	if (!ad->picasso_on || currprefs.gfx_apmode[1].gfx_vsync <= 0)
		return 0;
	if (currprefs.gfx_apmode[1].gfx_vsyncmode == 0)
		return 1;
	if (currprefs.m68k_speed >= 0)
		return -1;
	return currprefs.cachesize ? -3 : -2;
}

int isvsync(void)
{
	struct amigadisplay *ad = &adisplays[0];
	if (ad->picasso_on)
		return isvsync_rtg();
	else
		return isvsync_chipset();
}

void get_mode_blanking_limits(int *phbstop, int *phbstrt, int *pvbstop, int *pvbstrt)
{
	*pvbstop = linear_denise_vbstrt;
	*pvbstrt = linear_denise_vbstop;
	*phbstop = linear_denise_frame_hbstop;
	*phbstrt = linear_denise_frame_hbstrt;
}

static void setup_brdblank(void)
{
	denise_brdstrt_unalign = false;
	denise_brdstop_unalign = false;
	if (aga_mode && currprefs.gfx_resolution == RES_SUPERHIRES && borderblank && denise_accurate_mode) {
		denise_brdstrt = denise_hstop - 1;
		denise_brdstop = denise_hstrt - 1;
		denise_brdstrt_lores = denise_brdstrt >> 2;
		denise_brdstop_lores = denise_brdstop >> 2;
		if (!denise_hstop_unalign) {
			denise_brdstrt_unalign = true;
		}
		if (!denise_brdstop) {
			denise_brdstop_unalign = true;
		}
	} else {
		denise_brdstrt = -1;
		denise_brdstop = -1;
	}
}

static void calchdiw(void)
{
	int hbmask = (1 << (RES_SUPERHIRES - hresolution)) - 1;

	denise_hstrt = (denise_diwstrt & 0xFF) << 2;
	denise_hstop = (denise_diwstop & 0xFF) << 2;

	// ECS Denise/AGA: horizontal DIWHIGH high bit.
	if (diwhigh_written && ecs_denise) {
		denise_hstrt |= ((denise_diwhigh >> 5) & 1) << (8 + 2);
		denise_hstop |= ((denise_diwhigh >> 13) & 1) << (8 + 2);
	} else {
		denise_hstop |= 0x100 << 2;
	}

	// AGA only: horizontal DIWHIGH hires/shres bits.
	if (diwhigh_written && aga_mode) {
		denise_hstrt |= (denise_diwhigh >> 3) & 3;
		denise_hstop |= (denise_diwhigh >> 11) & 3;
	}

	denise_hstrt_lores = denise_hstrt >> 2;
	denise_hstop_lores = denise_hstop >> 2;

	denise_hstrt &= ~hbmask;
	denise_hstop &= ~hbmask;

	denise_hstrt_unalign = (denise_hstrt & 3) != 0;
	denise_hstop_unalign = (denise_hstop & 3) != 0;

	setup_brdblank();
}

static void spr_nearest(void)
{
	int min = 0x7fffffff;
	int cnt1 = denise_hcounter_new;
	int cnt2 = denise_hcounter;
	for (int i = 0; i < MAX_SPRITES; i++) {
		struct denise_spr *s = &dspr[i];
		if (s->armed) {
			if (denise_xposmask_mask_lores) {
				int diff = (s->xpos_lores & denise_xposmask_lores) - cnt1;
				if (diff < 0) {
					diff = (s->xpos_lores | denise_xposmask_mask_lores) - cnt1;
				}
				if (diff < min && diff >= 0) {
					min = diff;
				}
				diff = (s->xpos_lores & denise_xposmask_lores) - cnt2;
				if (diff < 0) {
					diff = (s->xpos_lores | denise_xposmask_mask_lores) - cnt2;
				}
				if (diff < min && diff >= 0) {
					min = diff;
				}
			} else {
				int diff = s->xpos_lores - cnt1;
				if (diff < min && diff >= 0) {
					min = diff;
				}
				diff = s->xpos_lores - cnt2;
				if (diff < min && diff >= 0) {
					min = diff;
				}
			}
		}
	}
	denise_spr_nearestcnt = min - 2;
	if (aga_mode && denise_spr_nearestcnt > 0 && denise_spr_nearestcnt < 0x7fffffff) {
		denise_spr_nearestcnt <<= hresolution;
	}
}

// arm/disarm sprite
static void spr_arm(struct denise_spr *s, int state)
{
	if (state) {
		if (!s->armed) {
			for (int i = 0; i < MAX_SPRITES; i++) {
				if (dprspt[i] == NULL) {
					dprspt[i] = s;
					break;
				}
			}
			denise_spr_nr_armed++;
			if (denise_spr_nr_armed == 1) {
				select_lts();
			}
			s->armed = 1;
			spr_nearest();
		}
	} else {
		for (int i = 0; i < MAX_SPRITES; i++) {
			if (dprspt[i] == s) {
				dprspt[i] = NULL;
				for (int j = i + 1; j < MAX_SPRITES; j++) {
					if (!dprspt[j]) {
						break;
					}
					dprspt[j - 1] = dprspt[j];
					dprspt[j] = NULL;
				}
				break;
			}
		}
		if (s->armed) {
			denise_spr_nr_armed--;
			if (denise_spr_nr_armed == 0) {
				select_lts();
			}
			s->armed = 0;
			s->shiftercopydone = false;
		}
	}
}

// arm/disarm sprite horizontal match
static void spr_arms(struct denise_spr *s, int state)
{
	// ECS Denise + superhires: sprites 4 to 7 are disabled
	if (ecs_denise_only && denise_res == RES_SUPERHIRES) {
		size_t num = s - dspr;
		if (num >= 4) {
			state = 0;
		}
	}
	if (state) {
		if (!s->armeds) {
			for (int i = 0; i < MAX_SPRITES; i++) {
				if (dprspts[i] == NULL) {
					dprspts[i] = s;
					break;
				}
			}
			denise_spr_nr_armeds++;
			s->armeds = 1;
			// Sprite start always has 1 lores pixel delay
			s->shift = 1 << RES_SUPERHIRES;
		}
	} else {
		for (int i = 0; i < MAX_SPRITES; i++) {
			if (dprspts[i] == s) {
				dprspts[i] = NULL;
				for (int j = i + 1; j < MAX_SPRITES; j++) {
					if (!dprspts[j]) {
						break;
					}
					dprspts[j - 1] = dprspts[j];
					dprspts[j] = NULL;
				}
				break;
			}
		}
		if (s->armeds) {
			denise_spr_nr_armeds--;
			s->armeds = 0;
		}
	}
}

static void sprwrite_64(int reg, uae_u64 v)
{
	int num = reg / 8;
	struct denise_spr *s = &dspr[num];
	bool second = (reg & 2) != 0;

	if (second) {
		s->datab64 = v;
	} else {
		s->dataa64 = v;
		spr_arm(s, 1);
#if AUTOSCALE_SPRITES
		/* get upper and lower sprite position if brdsprt enabled */
		if ((s->dataa64 || s->datab64) && bordersprite) {
			if (this_line->linear_vpos < plffirstline_total) {
				plffirstline_total = this_line->linear_vpos;
			}
			if (this_line->linear_vpos > plflastline_total) {
				plflastline_total = this_line->linear_vpos;
			}
			int x = s->xpos;;
			if (diwfirstword_total > x && x >= (48 << RES_MAX)) {
				diwfirstword_total = x;
				diwfirstword_total <<= 2;
			}
			if (diwlastword_total < x + 16 && x <= (448 << RES_MAX)) {
				diwlastword_total = x + 16;
				diwlastword_total <<= 2;
			}
#endif
		}
	}
}

static void sprwrite(int reg, uae_u32 v)
{
	int num = reg / 8;
	struct denise_spr *s = &dspr[num];
	bool dat = (reg & 4) != 0;
	bool second = (reg & 2) != 0;

	// If sprite X matches and sprite was already armed,
	// old value matches and shifter copy is done first.
	// (For example Hybris score board)
	if (s->armed && s->xpos_lores == denise_hcounter) {
		s->dataas = s->dataa;
		s->databs = s->datab;
		s->dataas64 = s->dataa64;
		s->databs64 = s->datab64;
		spr_arms(s, 1);
		s->fmode = denise_sprfmode;
		s->shiftercopydone = true;
	}

	if (dat) {
		uae_u16 oa = s->dataa;
		uae_u16 ob = s->datab;
		if (second) {
			s->datab = v;
		} else {
			s->dataa = v;
			// if same cycle would arm the sprite and match it, match is missed
			if (!s->armed && (s->xpos & (1 << 2)) && s->xpos - (1 << 2) == (denise_hcounter << 2)) {
				return;
			}
			spr_arm(s, 1);
#if AUTOSCALE_SPRITES
			/* get upper and lower sprite position if brdsprt enabled */
			if ((s->dataa || s->datab) && bordersprite) {
				if (this_line->linear_vpos < plffirstline_total) {
					plffirstline_total = this_line->linear_vpos;
				}
				if (this_line->linear_vpos > plflastline_total) {
					plflastline_total = this_line->linear_vpos;
				}
				int x = s->xpos;;
				if (diwfirstword_total > x && x >= (48 << RES_MAX)) {
					diwfirstword_total = x;
					diwfirstword_total <<= 2;
				}
				if (diwlastword_total < x + 16 && x <= (448 << RES_MAX)) {
					diwlastword_total = x + 16;
					diwlastword_total <<= 2;
				}
			}
#endif
		}
	} else {
		if (second) {
			s->ctl = v;
			spr_arm(s, 0);
		} else {
			s->pos = v;
		}
		int sprxp = (s->pos & 0xff) * 2 + (s->ctl & 1);
		s->xpos_lores = sprxp;
		sprxp <<= 2;
		if (aga_mode) {
			if (s->ctl & 0x10) {
				sprxp |= 2;
			}
			if (s->ctl & 0x08) {
				sprxp |= 1;
			}
		} else if (ecs_denise) {
			if (s->ctl & 0x10) {
				sprxp |= 2;
			}
		}
		s->xpos = sprxp;
		dspr[num & ~1].attached = (dspr[num | 1].ctl & 0x80) != 0;
		if (s->armed) {
			spr_nearest();
		}
	}

}

static void check_lts_request(void)
{
	if (lts_request) {
		select_lts();
	}
}

static void setbplmode(void)
{
	bplham = (bplcon0_denise & 0x800) != 0;
	bpldualpf = (bplcon0_denise & 0x400) == 0x400;
	bplehb = denise_planes == 6 && !bplham && !bpldualpf && (!ecs_denise || !(bplcon2_denise & 0x200));

	// BYPASS: HAM and EHB select bits are ignored
	bpland = 0xff;
	bplbypass = (bplcon0_denise & 0x20) != 0 && aga_mode;
	p_xcolors = xcolors;
	p_acolors = denise_colors.acolors;
	if (bplbypass) {
		if (bplham && denise_planes == 6) {
			bpland = 0x0f;
		}
		if (bplham && denise_planes == 8) {
			bpland = 0xfc;
		}
		bplham = 0;
		if (bplehb) {
			bpland = 0x1f;
		}
		bplehb = 0;
		p_acolors = direct_colors_for_drawing_bypass.acolors;
	}

	int bplmodeo = bplmode;
	int bplmode_next;
	if (bplham) {
		bplmode_next = CMODE_HAM;
	} else if (bpldualpf) {
		bplmode_next = CMODE_DUALPF;
	} else if (bplehb && bplehb_eke) {
		bplmode_next = CMODE_EXTRAHB_ECS_KILLEHB;
	} else if (bplehb) {
		bplmode_next = CMODE_EXTRAHB;
	} else {
		bplmode_next = CMODE_NORMAL;
	}

	// AGA EHB switch has extra hires pixel delay
	if (aga_mode && bplmode_next == CMODE_EXTRAHB && bplmodeo != CMODE_EXTRAHB) {
		bplmode_new = bplmode_next;
		aga_unalign0++;
		lts_request = true;
	} else if (aga_mode && bplmode_next != CMODE_EXTRAHB && bplmodeo == CMODE_EXTRAHB) {
		bplmode_new = bplmode_next;
		aga_unalign0++;
		lts_request = true;
	} else {
		bplmode = bplmode_new = bplmode_next;
		if (bplmodeo != bplmode) {
			lts_request = true;
		}
	}
}

static void update_specials(void)
{
	decode_specials = 0;
	if (!aga_mode) {
		// OCS/ECS feature if plf2pri>=5 and plane 5 bit is set: value is always 16 (SWIV scoreboard)
		if ((bplmode == CMODE_NORMAL || bplmode == CMODE_EXTRAHB || bplmode == CMODE_EXTRAHB_ECS_KILLEHB) && denise_planes >= 5 && plf2pri >= 5) {
			decode_specials = 1;
		}
		// OCS/ECS DPF feature: if matching plf2pri>=5: value is always 0 (Running man / Scoopex logo)
		if (bplmode == CMODE_DUALPF && (plf1pri >= 5 || plf2pri >= 5)) {
			decode_specials = 2;
		}
	}
}

static void update_genlock(void)
{
	ecs_genlock_features_active = (ecs_denise && ((bplcon2_denise & 0x0c00) || bordertrans)) ||
		(currprefs.genlock_effects ? 1 : 0) || (aga_mode && (bplcon3_denise & 0x004) && (bplcon0_denise & 1));
	if (ecs_genlock_features_active) {
		ecs_genlock_features_colorkey = currprefs.ecs_genlock_features_colorkey_mask[0] || currprefs.ecs_genlock_features_colorkey_mask[1] ||
			currprefs.ecs_genlock_features_colorkey_mask[2] || currprefs.ecs_genlock_features_colorkey_mask[3];
		ecs_genlock_features_mask = currprefs.ecs_genlock_features_plane_mask;
		aga_genlock_features_zdclken = false;
		if (bplcon2_denise & 0x0800) {
			ecs_genlock_features_mask = 1 << ((bplcon2_denise >> 12) & 7);
		}
		if (bplcon2_denise & 0x0400) {
			ecs_genlock_features_colorkey = true;
		}
		if ((bplcon3_denise & 0x0004) && (bplcon0_denise & 1)) {
			aga_genlock_features_zdclken = true;
		}
	}
}

static void update_bplcon1(void)
{
	if (!aga_mode) {
		if (ecs_denise && (reswitch_unalign == 1 || reswitch_unalign < 0)) {
			return;
		}
		if (!ecs_denise && (reswitch_unalign == 1 || reswitch_unalign == 2 || reswitch_unalign < 0)) {
			return;
		}
	}

	int bplcon1_hr[2];
	int delay1 = (bplcon1_denise & 0x0f) | ((bplcon1_denise & 0x0c00) >> 6);
	int delay2 = ((bplcon1_denise >> 4) & 0x0f) | (((bplcon1_denise >> 4) & 0x0c00) >> 6);
	int mask = 3 >> hresolution;
	bool wasoddeven = bplcon1_shift_full[0] != bplcon1_shift_full[1];

	bplcon1_shift_mask = fetchmode_mask_denise >> denise_res;

	bplcon1_shift[0] = delay1;
	bplcon1_shift[1] = delay2;

	bplcon1_shift[0] &= bplcon1_shift_mask;
	bplcon1_shift[1] &= bplcon1_shift_mask;

	bplcon1_hr[0] = (bplcon1_denise >> 8) & 3;
	bplcon1_hr[1] = (bplcon1_denise >> 12) & 3;
	
	bplcon1_shift_full[0] = (bplcon1_shift[0] << 2) | bplcon1_hr[0];
	bplcon1_shift_full[1] = (bplcon1_shift[1] << 2) | bplcon1_hr[1];

	bplcon1_shift_mask_full = (bplcon1_shift_mask << 2) | 3;

	int bplcon1_hr_mask = 0;//3 >> denise_res;

	bplcon1_shift_full[0] &= ~mask;
	bplcon1_shift_full[1] &= ~mask;

	bplcon1_shift_full_masked[0] = bplcon1_shift_full[0] & ~bplcon1_hr_mask;
	bplcon1_shift_full_masked[1] = bplcon1_shift_full[1] & ~bplcon1_hr_mask;

	if (wasoddeven != (bplcon1_shift_full[0] != bplcon1_shift_full[1])) {
		lts_request = true;
	}
}

STATIC_INLINE void get_shres_pix(uae_u8 p0, uae_u8 p1, uae_u32 *dpix0, uae_u32 *dpix1)
{
	uae_u16 v;
	uae_u8 off = ((p1 & 3) * 4) + (p0 & 3) + ((p0 | p1) & 16);
	v = (denise_colors.color_regs_ecs[off] & 0xccc) << 0;
	v |= v >> 2;
	*dpix0 = xcolors[v];
	v = (denise_colors.color_regs_ecs[off] & 0x333) << 2;
	v |= v >> 2;
	*dpix1 = xcolors[v];
}
STATIC_INLINE void get_shres_pix_genlock(uae_u8 p0, uae_u8 p1, uae_u32 *dpix0, uae_u32 *dpix1, uae_u8 *gpix0, uae_u8 *gpix1)
{
	uae_u8 v;
	uae_u8 off = ((p1 & 3) * 4) + (p0 & 3) + ((p0 | p1) & 16);
	v = (denise_colors.color_regs_ecs[off] & 0xccc) << 0;
	v |= v >> 2;
	*dpix0 = xcolors[v];
	*gpix0 = v;
	v = (denise_colors.color_regs_ecs[off] & 0x333) << 2;
	v |= v >> 2;
	*gpix1 = v;
	*dpix1 = xcolors[v];
}

static void update_bordercolor(void)
{
	if (borderblank) {
		bordercolor = fullblack;
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
			bordercolor = denise_colors.acolors[0];
			bordercolor |= 0x01000000;
		}
		bordercolor_ecs_shres = 0;
	} else {
		bordercolor = denise_colors.acolors[0];
		get_shres_pix(0, 0, &bordercolor_ecs_shres, &bordercolor_ecs_shres);
	}
	setup_brdblank();
}

static void update_hblank(void)
{
	if (exthblankon_aga) {
		int hbmask = (1 << (RES_SUPERHIRES - hresolution)) - 1;

		denise_phbstrt = hbstrt_denise_reg & 0xff;
		denise_phbstop = hbstop_denise_reg & 0xff;
		denise_phbstrt_lores = (denise_phbstrt << 1) | ((hbstrt_denise_reg >> 10) & 1);
		denise_phbstop_lores = (denise_phbstop << 1) | ((hbstop_denise_reg >> 10) & 1);
		denise_phbstrt <<= 3;
		denise_phbstop <<= 3;
		denise_phbstrt |= (hbstrt_denise_reg >> 8) & 7;
		denise_phbstop |= (hbstop_denise_reg >> 8) & 7;

		denise_phbstrt &= ~hbmask;
		denise_phbstop &= ~hbmask;

		denise_phbstrt_unalign = (denise_phbstrt & 3) != 0;
		denise_phbstop_unalign = (denise_phbstop & 3) != 0;

	} else {

		denise_phbstrt = -1;
		denise_phbstop = -1;
		denise_phbstrt_lores = -1;
		denise_phbstop_lores = -1;
		denise_phbstrt_unalign = false;
		denise_phbstop_unalign = false;

	}

	denise_hbstrt_lores = ecs_denise ? 0x10 : 0x0f;
	denise_hbstop_lores = ecs_denise ? 0x5d : 0x5d;

	denise_strlong_lores = ecs_denise || denisea1000 ? 0x0f : 0x11;
	denise_strlong_hd = denise_strlong_lores << 2;
	denise_strlong_unalign = false;
	if (aga_mode && hresolution == RES_SUPERHIRES) {
		denise_strlong_hd += 1;
		denise_strlong_unalign = true;
	}
}

static void update_sprres_set(void)
{
	denise_spr_add = 1 << (RES_MAX - hresolution);
	denise_spr_shiftsize = 1 << (RES_SUPERHIRES - denise_sprres);
}

static void update_sprres(void)
{
	int sr = (bplcon3_denise >> 6) & 3;
	if (!aga_mode) {
		sr = 0;
	}
	int odenise_sprres = denise_sprres;
	switch (sr)
	{
		case 0:
		default:
		denise_sprres = denise_res == RES_SUPERHIRES ? RES_HIRES : RES_LORES;
		break;
		case 1:
		denise_sprres = RES_LORES;
		break;
		case 2:
		denise_sprres = RES_HIRES;
		break;
		case 3:
		denise_sprres = RES_SUPERHIRES;
		break;
	}
	update_sprres_set();
	if (odenise_sprres != denise_sprres) {
		lts_request = true;
	}
}

static void update_ecs_features(void)
{
	bool ecsena = ecs_denise && (bplcon0_denise & 1) != 0;
	exthblankon_aga = aga_mode && (bplcon3_denise & 1) && ecsena;
	exthblankon_ecs = (bplcon3_denise & 1) && ecsena;
	exthblankon_ecsonly = exthblankon_ecs && !exthblankon_aga;
	borderblank = (bplcon3_denise & 0x20) && ecsena;
	bordertrans = (bplcon3_denise & 0x10) && ecsena;
	bordersprite = (bplcon3_denise & 0x02) && (!borderblank || currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) && ecsena && aga_mode;
	sprite_hidden_mask = 1;
	if (exthblankon_ecsonly) {
		denise_vblank_active = false;
		denise_blank_active2 = denise_hblank_active || denise_vblank_active;
	}
	if (bordersprite) {
		sprites_hidden2 = 0;
		sprites_hidden = 0;
		sprite_hidden_mask = 0;
	} else {
		if (!bpl1dat_trigger) {
			sprites_hidden2 |= 2;
		}
		if (!denise_hdiw) {
			sprites_hidden2 |= 1;
		}
	}
	update_hblank();
	update_bordercolor();
	exthblanken = exthblankon_aga || exthblankon_ecs;
}

static void update_fmode(void)
{
	int oplanes = denise_planes;
	denise_planes = GET_PLANES(bplcon0_denise);
	if (denise_bplfmode == 0 && denise_res > 0 && denise_planes > 4) {
		denise_planes = 0;
	}
	if (denise_bplfmode == 0 && denise_res > 1 && denise_planes > 2) {
		denise_planes = 0;
	}
	if (denise_bplfmode == 1 && denise_res > 1 && denise_planes > 4) {
		denise_planes = 0;
	}
	int osize = fetchmode_size_denise;
	fetchmode_size_denise = 16 << bpldat_fmode;
	fetchmode_mask_denise = fetchmode_size_denise - 1;
	update_bplcon1();
	if (oplanes != denise_planes || osize != fetchmode_size_denise) {
		lts_request = true;
	}
}

static void expand_bplcon4_spr(uae_u16 v)
{
	uae_u16 o = bplcon4_denise;
	bplcon4_denise &= 0xff00;
	bplcon4_denise |= v & 0x00ff;
	// Sprite bank change is 1 hires pixel faster than bitplane XOR change
	if (aga_mode && bplcon4_denise_sbase != (bplcon4_denise & 0x00ff)) {
		bplcon4_denise_sbase = bplcon4_denise & 0xff;
		sbasecol2[0] = ((bplcon4_denise_sbase >> 4) & 15) << 4;
		sbasecol2[1] = ((bplcon4_denise_sbase >> 0) & 15) << 4;
		aga_unalign1++;
	}
}

static void expand_bplcon4_bm(uae_u16 v)
{
	bplcon4_denise &= 0x00ff;
	bplcon4_denise |= v & 0xff00;
	// Sprite bank change is 1 hires pixel faster than bitplane XOR change
	if (aga_mode && (bplcon4_denise >> 8) != bplcon4_denise_xor_val2) {
		bplcon4_denise_xor_val2 = bplcon4_denise >> 8;
		aga_unalign1++;
	}
}

static void expand_bplcon3(uae_u16 v)
{
	bplcon3_denise = v;
	bpldualpf2of = aga_mode ? (bplcon3_denise >> 10) & 7 : 8;
	update_ecs_features();
	update_sprres();
	update_genlock();
	check_lts_request();
}

static void expand_bplcon2(uae_u16 v)
{
	bplcon2_denise = v;
	plf1pri = bplcon2_denise & 7;
	plf2pri = (bplcon2_denise >> 3) & 7;
	plf_sprite_mask = 0xFFFF0000 << (4 * plf2pri);
	plf_sprite_mask |= (0x0000FFFF << (4 * plf1pri)) & 0xFFFF;
	bpldualpfpri = (bplcon2_denise & 0x40) == 0x40;

	if (aga_mode) {
		dpf_lookup = bpldualpfpri ? dblpf_ind2_aga : dblpf_ind1_aga;
	} else {
		dpf_lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
	}
	dpf_lookup_no = bpldualpfpri ? dblpf_2nd2 : dblpf_2nd1;

	bool bplehb_eke_o = bplehb_eke;
	bplehb_eke = false;
	if (ecs_denise) {
		// Check for KillEHB bit in ECS/AGA
		bplehb_eke = (bplcon2_denise & 0x0200) != 0;
	}
	bplehb_mask = denisea1000_noehb ? 0x1f : 0x3f;
	if (bplehb_eke_o != bplehb_eke) {
		setbplmode();
	}

	update_specials();
	check_lts_request();
}

static void expand_bplcon1(uae_u16 v)
{
#if DISABLE_BPLCON1
	v = 0;
#endif

	if (!aga_mode) {
		v &= 0x00ff;
	}
	bplcon1_denise = v;
	update_bplcon1();
	check_lts_request();
}

static void expand_bplcon0_early(uae_u16 v)
{
	if (!aga_mode) {
		int ores = denise_res;
		int nres = GET_RES_DENISE(v);
		if (ores == RES_LORES && nres == RES_HIRES) {
			if (ecs_denise) {
				reswitch_unalign = 3;
			} else {
				reswitch_unalign = 3;
			}
		}
	}
}

int gethresolution(void)
{
	return hresolution;
}

static void sethresolution(void)
{
	hresolution = currprefs.gfx_resolution;
	if (doublescan == 1) {
		hresolution++;
		if (hresolution > RES_SUPERHIRES) {
			hresolution = RES_SUPERHIRES;
		}
	}
}

static void setlasthamcolor(void)
{
	if (aga_mode) {
		ham_lastcolor = denise_colors.color_regs_aga[last_bpl_pix];
	} else {
		ham_lastcolor = denise_colors.color_regs_ecs[last_bpl_pix];
	}
}

static void expand_bplcon0(uae_u16 v)
{
	uae_u16 old = bplcon0_denise;
	if (!ecs_denise) {
		v &= ~0x00F1;
	} else if (!aga_mode) {
		v &= ~0x00B0;
	}
	v &= ~((currprefs.cs_color_burst ? 0x0000 : 0x0200) | 0x0100 | 0x0080 | 0x0020);

	if ((v & 0x800) && !(bplcon0_denise & 0x800)) {
		setlasthamcolor();
	}

	bplcon0_denise = v;

	if (denise_res != GET_RES_DENISE(bplcon0_denise) || denise_planes != GET_PLANES(bplcon0_denise)) {
		lts_request = true;
	}

	int ores = denise_res;
	denise_res = GET_RES_DENISE(bplcon0_denise);
	denise_res_size = 1 << denise_res;
	sethresolution();
	denise_planes = GET_PLANES(bplcon0_denise);
	bplcolorburst = (bplcon0_denise & 0x200) != 0;
	if (!bplcolorburst) {
		bplcolorburst_field = 0;
	}

#if 0
	if (!aga_mode) {
		if (ores == RES_HIRES && denise_res == RES_LORES) {
			//reswitch_unalign = -1;
		}
	}
#endif

	setbplmode();
	update_fmode();
	update_specials();
	update_sprres();
	if ((old & 1) != (bplcon0_denise & 1)) {
		update_ecs_features();
		update_genlock();
	}
	check_lts_request();
}

static void expand_fmode(uae_u16 v)
{
	if (!aga_mode) {
		v = 0;
	}
	fmode_denise = v;
	denise_xposmask = (v & 0x8000) ? 0x0ff : 0x1ff;
	denise_xposmask_mask_lores = (v & 0x8000) ? 0x100 : 0x000;
	denise_xposmask_lores = denise_xposmask;
	denise_xposmask <<= 2;
	denise_xposmask |= hresolution == RES_SUPERHIRES ? 3 : (hresolution == RES_HIRES ? 2 : 0);

	denise_bplfmode = (v & 3) == 3 ? 2 : (v & 3) == 0 ? 0 : 1;
	v >>= 2;
	denise_sprfmode = (v & 3) == 3 ? 2 : (v & 3) == 0 ? 0 : 1;
	denise_sprfmode64 = denise_sprfmode == 2;
	denise_bplfmode64 = denise_bplfmode == 2;
	update_fmode();
	check_lts_request();
}


static void expand_colmask(void)
{
	sprcolmask = 0x40 | 0x10 | 0x04 | 0x01;
	if (clxcon & (1 << 15)) {
		sprcolmask |= 0x80;
	}
	if (clxcon & (1 << 14)) {
		sprcolmask |= 0x20;
	}
	if (clxcon & (1 << 13)) {
		sprcolmask |= 0x08;
	}
	if (clxcon & (1 << 12)) {
		sprcolmask |= 0x02;
	}

	clxcon_bpl_enable = (clxcon >> 6) & 63;
	clxcon_bpl_match = clxcon & 63;
	if (aga_mode) {
		clxcon_bpl_match |= (clxcon2 & (0x01 | 0x02)) << 6;
		clxcon_bpl_enable |= clxcon2 & (0x40 | 0x80);
	}
	clxcon_bpl_match &= clxcon_bpl_enable;
	clxcon_bpl_enable_55 = clxcon_bpl_enable & 0x55;
	clxcon_bpl_enable_aa = clxcon_bpl_enable & 0xaa;
	clxcon_bpl_match_55 = clxcon_bpl_match & 0x55;
	clxcon_bpl_match_aa = clxcon_bpl_match & 0xaa;

	// create bpl to bpl (CLXDAT bit 0) collision table
	int clxcon_bpl_enable2 = clxcon_bpl_enable;
	int clxcon_bpl_match2 = clxcon_bpl_match;
	if (currprefs.collision_level < 3) {
		clxcon_bpl_enable2 = 0;
		clxcon_bpl_match2 = 0;
	}
	bool bplalwayson = currprefs.collision_level >= 3 && clxcon_bpl_enable2 == 0;
	if (clxcon_bpl_enable_o != clxcon_bpl_enable2 || clxcon_bpl_match_o != clxcon_bpl_match2) {
		for (int i = 0; i < (aga_mode ? 256 : 64); i++) {
			uae_u8 m = i & clxcon_bpl_enable;
			uae_u8 odd = m & 0x55;
			uae_u8 even = m & 0xaa;
			if (((odd && even) && m == (clxcon_bpl_enable2 & clxcon_bpl_match2)) || bplalwayson) {
				bplcoltable[i] = 0x0001;
			} else {
				bplcoltable[i] = 0x0000;
			}
		}
		clxcon_bpl_enable_o = clxcon_bpl_enable2;
		clxcon_bpl_match_o = clxcon_bpl_match2;
	}
}

static void expand_clxcon(uae_u16 v)
{
	clxcon = v;
	clxcon2 = 0;
	expand_colmask();
}
static void expand_clxcon2(uae_u16 v)
{
	clxcon2 = v;
	expand_colmask();
}

static void set_strlong(void)
{
	denise_strlong_seen = true;
	denise_strlong_seen_delay = STRLONG_SEEN_DELAY;
	if (no_denise_lol) {
		int add = 1 << hresolution;
		denise_strlong = false;
		denise_strlong_fast = true;
		denise_lol_shift_enable = true;
		denise_lol_shift_prev = add;
		return;
	}
	denise_strlong = true;
	if (!strlong_emulation) {
		write_log("STRLONG strobe emulation activated.\n");
		strlong_emulation = true;
		select_lts();
	}
}
static void copy_strlong(void)
{
	lol = denise_strlong ? 1 : 0;
	lol_fast = denise_strlong_fast ? 1 : 0;
	if (denisea1000) {
		memset(dtbuf, 0, sizeof(dtbuf));
	}
}
static void reset_strlong(void)
{
	denise_strlong = false;
	denise_strlong_fast = false;
}

void denise_reset(bool hard)
{
	static int dummyint = 0;
	static struct dma_rec dummydrec = { 0 };

	custom_end_drawing();
	dummyint = 0;
	memset(&dummydrec, 0, sizeof(dummydrec));
	memset(chunky_out, 0, sizeof(chunky_out));
	memset(dpf_chunky_out, 0, sizeof(dpf_chunky_out));
	debug_dma_dhpos_odd = &dummyint;
	debug_dma_ptr = &dummydrec;
	denise_cycle_half = 0;
	denise_strlong = false;
	denise_strlong_fast = false;
	rga_denise_fast_read = rga_denise_fast_write = 0;
	rga_queue_write = rga_queue_read = 0;
	denise_lol_shift_enable = false;
	denise_lol_shift_prev = 0;
	if (hard) {
		strlong_emulation = false;
		denise_res = 0;
		denise_planes = 0;
		fmode_denise = 0;
		bplcon0_denise = 0;
		bplcon3_denise = 0x0c00;
		bplcon4_denise = 0x0011;
		clxcon_bpl_enable = -1;
		clxcon_bpl_match = -1;
		memset(bplcoltable, 0, sizeof(bplcoltable));
		denise_xposmask = 0xffff;
		sbasecol[0] = 16;
		sbasecol[1] = 16;
		sbasecol2[1] = 16;
		sbasecol2[0] = 16;
		for (int i = 0; i < MAX_SPRITES; i++) {
			struct denise_spr *s = &dspr[i];
			memset(s, 0, sizeof(struct denise_spr));
			s->num = i;
			s->armeds = 0;
			dprspt[i] = NULL;
			dprspts[i] = NULL;
		}
		denise_spr_nr_armed = 0;
		denise_spr_nr_armeds = 0;
		denise_diwhigh = 0;
		denise_diwhigh2 = -1;
		diwhigh_written = false;
	}
	bplcon4_denise_sbase = -1;
	update_sprres_set();
	denise_bplfmode64 = denise_sprfmode64 = false;
	denise_hdiw = false;
	denise_blank_active = false;
	denise_hblank = false;
	denise_phblank = false;
	denise_vblank = false;
	denise_hblank_active = false;
	denise_vblank_active = false;
	denise_blank_active = denise_blank_active2 = false;
	denise_blank_enabled = currprefs.gfx_overscanmode < OVERSCANMODE_ULTRA;
	extblank = false;
	last_bpl_pix = 0;
	decode_specials = 0;
	decode_specials_debug = currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA ? 2 : (currprefs.display_calibration ? 1 : 0);
	debug_special_hvsync = currprefs.gfx_overscanmode == OVERSCANMODE_ULTRA + 1;
	debug_special_csync = currprefs.gfx_overscanmode == OVERSCANMODE_ULTRA + 2;
	denise_csync_blanken = false;
	aga_delayed_color_idx = -1;
	for (int i = 0; i < 256; i++) {
		uae_u16 v = 0;
		if (i & (0x01 | 0x02)) { // 0/1
			if (i & (0x04 | 0x08)) { // 2/3
				v |= 0x0200;
			}
			if (i & (0x10 | 0x20)) { // 4/5
				v |= 0x0400;
			}
			if (i & (0x40 | 0x80)) { // 6/7
				v |= 0x0800;
			}
		}
		if (i & (0x04 | 0x08)) { // 2/3
			if (i & (0x10 | 0x20)) { // 4/5
				v |= 0x1000;
			}
			if (i & (0x40 | 0x80)) { // 6/7
				v |= 0x2000;
			}
		}
		if (i & (0x10 | 0x20)) { // 4/5
			if (i & (0x40 | 0x80)) { // 6/7
				v |= 0x4000;
			}
		}
		sprcoltable[i] = v;
	}
	for (int i = 0; i < 256; i++) {
		uae_u16 v = 0;
		if (i & (0x01 | 0x02)) { // 0/1
			v |= 0x02;
		}
		if (i & (0x04 | 0x08)) { // 2/3
			v |= 0x04;
		}
		if (i & (0x10 | 0x20)) { // 4/5
			v |= 0x08;
		}
		if (i & (0x40 | 0x80)) { // 6/7
			v |= 0x10;
		}
		sprbplcoltable[i] = v;
	}
	clxcon_bpl_enable_o = -1;
	clxcon_bpl_match_o = -1;
	expand_bplcon0(bplcon0_denise);
	expand_bplcon1(bplcon1_denise);
	expand_bplcon2(bplcon2_denise);
	expand_bplcon3(bplcon3_denise);
	expand_bplcon4_spr(bplcon4_denise);
	expand_bplcon4_bm(bplcon4_denise);
	expand_fmode(fmode_denise);
	expand_colmask();
	sethresolution();
}

// COLORxx write
static void update_color(int idx, uae_u16 v, uae_u16 con2, uae_u16 con3)
{
	if (aga_mode) {

		/* writing is disabled when RDRAM=1 */
		if (con2 & 0x0100)
			return;

		int colreg = ((con3 >> 13) & 7) * 32 + idx;
		int r = (v & 0xF00) >> 8;
		int g = (v & 0xF0) >> 4;
		int b = (v & 0xF) >> 0;
		int cr = (denise_colors.color_regs_aga[colreg] >> 16) & 0xFF;
		int cg = (denise_colors.color_regs_aga[colreg] >> 8) & 0xFF;
		int cb = denise_colors.color_regs_aga[colreg] & 0xFF;

		if (con3 & 0x200) {
			cr &= 0xF0; cr |= r;
			cg &= 0xF0; cg |= g;
			cb &= 0xF0; cb |= b;
		} else {
			cr = r + (r << 4);
			cg = g + (g << 4);
			cb = b + (b << 4);
			denise_colors.color_regs_genlock[colreg] = v >> 15;
		}
		uae_u32 cval = (cr << 16) | (cg << 8) | cb;

		denise_colors.color_regs_aga[colreg] = cval;
		denise_colors.acolors[colreg] = getxcolor(cval);
		if (idx < 32) {
			denise_colors.color_regs_ecs[idx] = ((cr >> 4) << 8) | ((cg >> 4) << 4) | (cb >> 4);
		}

		if (!colreg) {
			update_bordercolor();
		}

	} else {

		if (!ecs_denise) {
			v &= 0xfff;
		}
		denise_colors.color_regs_ecs[idx] = v & 0xfff;
		denise_colors.color_regs_genlock[idx] = v >> 15;
		denise_colors.acolors[idx] = getxcolor(v);
		denise_colors.color_regs_aga[idx] = denise_colors.acolors[idx];

		if (!idx) {
			update_bordercolor();
		}

	}
}

static void hstart_new(void)
{
	// inside vblank: does not happen
	if (!denise_vblank_active) {
		bpl1dat_trigger = false;
		sprites_hidden2 = bordersprite ? 0 : 3;
		if (denise_hdiw) {
			sprites_hidden2 &= ~1;
		}
		sprites_hidden = sprites_hidden2;
		bplshiftcnt[0] = bplshiftcnt[1] = 0;
		last_bpl_pix = 0;
		setlasthamcolor();
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_BPL1DAT_HDIW, false);
		}
#endif
	}
}

static void do_exthblankon_ecs(void)
{
	// ECS Denise CSYNC programmed HBLANK
	if (!denise_blank_active2 && denise_csync_blanken2) {
		hstart_new();
	}
	denise_blank_active2 = denise_csync_blanken2;
	if (extblank != denise_csync_blanken2) {
		extblank = denise_csync_blanken2;
		if (!extblank) {
			copy_strlong();
			memset(dtbuf, 0, sizeof(dtbuf));
		}
	}
	if (delayed_sprite_vblank_ecs > 0 && denise_blank_active2) {
		denise_sprite_blank_active = true;
		delayed_sprite_vblank_ecs = 0;
	} else if (delayed_sprite_vblank_ecs < 0 && denise_blank_active2) {
		denise_sprite_blank_active = false;
		delayed_sprite_vblank_ecs = 0;
	} else {
		denise_sprite_blank_active = false;
	}
	denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
}

static void do_exthblankon_aga(void)
{
	// AGA programmed HBLANK
	denise_blank_active2 = denise_phblank || denise_pvblank;
	denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
}

// BPL1DAT allows sprites 1 lores pixel before bitplanes
static void bpl1dat_enable_sprites(void)
{
	sprites_hidden2 &= ~2;
	if (denise_hdiw) {
		sprites_hidden2 &= ~1;
	}
}
static void bpl1dat_enable_bpls(void)
{
	// A1000/OCS Denise: BPL1DAT won't open HDIW if BURST is active
	if (ecs_denise || !denise_burst) {
		bpl1dat_trigger = true;
#ifdef DEBUGGER
		if (debug_dma) {
			record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_BPL1DAT_HDIW, true);
		}
#endif
	}
}

// bpl1dat write -> copy all bplxdats to internal registers
// (must copy all, not just current plane count because if planecount
// is decreased mid line, old higher planes must be still shifted out)
static void bpldat_docopy(void)
{
	if (aga_mode) {
		if (denise_bplfmode64) {
			bplxdat2_64[0] = bplxdat_64[0];
			bplxdat2_64[1] = bplxdat_64[1];
			bplxdat2_64[2] = bplxdat_64[2];
			bplxdat2_64[3] = bplxdat_64[3];
			bplxdat2_64[4] = bplxdat_64[4];
			bplxdat2_64[5] = bplxdat_64[5];
			bplxdat2_64[6] = bplxdat_64[6];
			bplxdat2_64[7] = bplxdat_64[7];
		} else {
			bplxdat2[0] = bplxdat[0];
			bplxdat2[1] = bplxdat[1];
			bplxdat2[2] = bplxdat[2];
			bplxdat2[3] = bplxdat[3];
			bplxdat2[4] = bplxdat[4];
			bplxdat2[5] = bplxdat[5];
			bplxdat2[6] = bplxdat[6];
			bplxdat2[7] = bplxdat[7];
		}
	} else {
		bplxdat2[0] = bplxdat[0];
		bplxdat2[1] = bplxdat[1];
		bplxdat2[2] = bplxdat[2];
		bplxdat2[3] = bplxdat[3];
		bplxdat2[4] = bplxdat[4];
		bplxdat2[5] = bplxdat[5];
	}

	bpldat_copy[0] = true;
	bpldat_copy[1] = true;

	if (bpl1dat_trigger_offset < 0) {
		bpl1dat_trigger_offset = internal_pixel_cnt;
	}

	if (debug_bpl_mask != 0xff) {
		for (int i = 0; i < MAX_PLANES; i++) {
			if (!(debug_bpl_mask & (1 << i))) {
				bplxdat2[i] = 0;
				bplxdat2_64[i] = 0;
			}
		}
	}
}

static void expand_drga_early2x(struct denise_rga *rd)
{
	switch (rd->rga)
	{
		case 0x10c:
		expand_bplcon4_spr(rd->v);
		break;
	}
}

static void expand_drga_early(struct denise_rga *rd)
{
	if (rd->rga >= 0x180 && rd->rga < 0x180 + 32 * 2) {
		int idx = (rd->rga - 0x180) / 2;
		if (aga_delayed_color_idx >= 0) {
			update_color(aga_delayed_color_idx, aga_delayed_color_val, aga_delayed_color_con2, aga_delayed_color_con3);
			aga_delayed_color_idx = -1;
		}
		if (aga_mode && (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA || !denise_vblank_active)) {
			// AGA color changes are 1 hires pixel delayed
			aga_delayed_color_idx = idx;
			aga_delayed_color_val = rd->v;
			aga_delayed_color_con2 = bplcon2_denise;
			aga_delayed_color_con3 = bplcon3_denise;
			aga_unalign0++;
		} else {
			update_color(idx, rd->v, bplcon2_denise, bplcon3_denise);
		}
	}

	switch (rd->rga)
	{
		case 0x144: case 0x146:
		case 0x14c: case 0x14e:
		case 0x154: case 0x156:
		case 0x15c: case 0x15e:
		case 0x164: case 0x166:
		case 0x16c: case 0x16e:
		case 0x174: case 0x176:
		case 0x17c: case 0x17e:
		if (denise_sprfmode64) {
			sprwrite_64(rd->rga - 0x140, rd->v64);
		} else {
			sprwrite(rd->rga - 0x140, rd->v);
		}
		break;

		case 0x100:
		expand_bplcon0_early(rd->v);
		break;

		case 0x10c:
		expand_bplcon4_bm(rd->v);
		break;

		// BPL1DAT sprite enable is 2 lores pixels earlier
		// BPL1DAT bitplane enable is 1 lores pixel earlier (AGA has extra 1 hires pixel)
		case 0x110:
		bpl1dat_enable_sprites();
		if (!bpl1dat_trigger) {
			// need 1 lores pixel delay
			// skip delay if DIW is not open
			if (!denise_hdiw) {
				bpl1dat_enable_bpls();
			} else {
				bpl1dat_unalign = true;
				aga_unalign0 += 2;
				aga_unalign1 += 2;
			}
		}
		break;
	}

}

static void expand_drga_blanken(struct denise_rga *rd)
{
	if (rd->flags) {
		if (rd->flags & DENISE_RGA_FLAG_BLANKEN_CSYNC) {
			// + 0.5CCK extra delay
			if (rd->flags & DENISE_RGA_FLAG_BLANKEN_CSYNC_ON) {
				if (!denise_csync_blanken) {
					memset(dtbuf, 0, sizeof(dtbuf));
				}
				denise_csync_blanken = true;
			} else {
				denise_csync_blanken = false;
			}
		}
	}
}

bool denise_is_vb(void)
{
	if (delayed_vblank_ecs > 0) {
		return true;
	} else if (delayed_vblank_ecs < 0) {
		return false;
	}
	return denise_vblank;
}

static void handle_strobes(struct denise_rga *rd)
{
	if (ecs_denise) {

		if (rd->rga == 0x03c && previous_strobe != 0x03c) {
			delayed_vblank_ecs = -1;
			delayed_sprite_vblank_ecs = -1;
		} else if (rd->rga != 0x03c && previous_strobe == 0x03c) {
			delayed_vblank_ecs = 1;
			delayed_sprite_vblank_ecs = 1;
		}

		denise_hcounter_new = 1 * 2;

	} else {

		if (rd->rga == 0x03c && previous_strobe != 0x03c) {
			denise_vblank = false;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_VB, false);
			}
#endif
		} else if (rd->rga == 0x038 && previous_strobe != 0x038) {
			denise_vblank = true;
#ifdef DEBUGGER
			if (debug_dma) {
				record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_VB, true);
			}
#endif
		}
		denise_vblank_active = denise_vblank;
		denise_blank_active2 = denise_hblank_active || denise_vblank_active;
		denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
		// OCS Denise hcounter only resets if STRHOR or STRVBL.
		// STREQU does not reset it. 9-bit counter is free-running.
		if (rd->rga == 0x03a || rd->rga == 0x03c) {
			denise_hcounter_new = 1 * 2;
		}
	}
	if (rd->rga == 0x03c && previous_strobe != 0x03c) {
		linear_denise_vbstrt = this_line->linear_vpos;
	} else if (rd->rga != 0x03c && previous_strobe == 0x03c) {
		linear_denise_vbstop = this_line->linear_vpos;
		denise_visible_lines++;
		denise_visible_lines_counted = denise_visible_lines;
		denise_visible_lines = 0;
	}
	if (rd->rga == 0x03c && previous_strobe == 0x03c) {
		denise_visible_lines++;
	}
	previous_strobe = rd->rga;
	reset_strlong();
	spr_nearest();
}

static void expand_drga(struct denise_rga *rd)
{
	if (rd->flags) {
		if (rd->flags & DENISE_RGA_FLAG_SYNC) {
			denise_csync = (rd->flags & DENISE_RGA_FLAG_CSYNC) != 0;
			denise_vsync = (rd->flags & DENISE_RGA_FLAG_VSYNC) != 0;
			denise_hsync = (rd->flags & DENISE_RGA_FLAG_HSYNC) != 0;
		}
		if (rd->flags & DENISE_RGA_FLAG_LOL) {
			agnus_lol = (rd->flags & DENISE_RGA_FLAG_LOL_ON) != 0;
			if (no_denise_lol) {
				int add = 1 << hresolution;
				if (denise_strlong_seen) {
					agnus_lol = false;
					denise_lol_shift_enable = true;
					denise_lol_shift_prev = add;
				} else {
					agnus_lol = false;
					denise_lol_shift_enable = false;
					denise_lol_shift_prev = add;
				}
			} else {
				if (!agnus_lol && (!denise_lol_shift_prev || denise_lol_shift_enable)) {
					int add = 1 << hresolution;
					buf1 += add;
					buf2 += add;
					buf_d += add;
					if (gbuf) {
						gbuf += add;
					}
					denise_lol_shift_prev = add;
					denise_lol_shift_enable = true;
				} else if (agnus_lol && denise_lol_shift_prev > 0) {
					buf1 -= denise_lol_shift_prev;
					buf2 -= denise_lol_shift_prev;
					buf_d -= denise_lol_shift_prev;
					if (gbuf) {
						gbuf -= denise_lol_shift_prev;
					}
					denise_lol_shift_prev = 0;
					denise_lol_shift_enable = true;
				}
			}
		}
	}

	switch (rd->rga)
	{
		case 0x02c:
		denise_hcounter_new = (rd->v & 0xff) * 2;
		spr_nearest();
		break;
		case 0x038: // STREQU
		case 0x03a: // STRVBL
		case 0x03c: // STRHOR
		handle_strobes(rd);
		break;
		case 0x03e: // STRLONG
		set_strlong();
		break;

		case 0x140: case 0x142:
		case 0x148: case 0x14a:
		case 0x150: case 0x152:
		case 0x158: case 0x15a:
		case 0x160: case 0x162:
		case 0x168: case 0x16a:
		case 0x170: case 0x172:
		case 0x178: case 0x17a:
		sprwrite(rd->rga - 0x140, rd->v);
		break;

		case 0x110:
		if (denise_bplfmode64) {
			bplxdat_64[0] = rd->v64;
		} else {
			bplxdat[0] = rd->v;
		}
		bpldat_docopy();
		if (bpldat_fmode != denise_bplfmode) {
			bpldat_fmode = denise_bplfmode;
			update_fmode();
			select_lts();
		}
		break;
		case 0x112:
		case 0x114:
		case 0x116:
		case 0x118:
		case 0x11a:
		case 0x11c:
		case 0x11e:
		if (denise_bplfmode64) {
			bplxdat_64[(rd->rga - 0x110) / 2] = rd->v64;
		} else {
			bplxdat[(rd->rga - 0x110) / 2] = rd->v;
		}
		break;

		case 0x100:
		expand_bplcon0(rd->v);
		break;
		case 0x102:
		expand_bplcon1(rd->v);
		break;
		case 0x104:
		expand_bplcon2(rd->v);
		break;
		case 0x106:
		expand_bplcon3(rd->v);
		break;
		case 0x08e:
		denise_diwstrt = rd->v;
		diwhigh_written = false;
		calchdiw();
		break;
		case 0x090:
		denise_diwstop = rd->v;
		diwhigh_written = false;
		calchdiw();
		break;
		case 0x1e4:
		if (rd->v != denise_diwhigh2 || rd->v != denise_diwhigh || !diwhigh_written) {
			// ecs denise superhires check is a hack until lts_unaligned_ecs_shres() is implemented
			if (!aga_mode && hresolution < RES_SUPERHIRES) {
				reswitch_unalign++;
				denise_diwhigh2 = rd->v;
			} else {
				denise_diwhigh = rd->v;
				diwhigh_written = true;
				calchdiw();
			}
		}
		break;
		case 0x1fc:
		expand_fmode(rd->v);
		break;
		case 0x098:
		expand_clxcon(rd->v);
		break;
		case 0x10e:
		expand_clxcon2(rd->v);
		break;
		case 0x1c4:
		hbstrt_denise_reg = rd->v;
		update_hblank();
		break;
		case 0x1c6:
		hbstop_denise_reg = rd->v;
		update_hblank();
		break;

	}
}

static void flush_fast_rga(uae_u32 linecnt)
{
	// extract fast CPU RGA pipeline
	while (rga_denise_fast_write != rga_denise_fast_read) {
		int rp = rga_denise_fast_read;
		struct denise_rga *rd = &rga_denise_fast[rp];
		// Any RGA queued accesses must be processed first
		if (linecnt <= rd->line) {
			break;
		}
		expand_drga(rd);
		expand_drga_early(rd);
		expand_drga_early2x(rd);
		rp++;
		rp &= DENISE_RGA_SLOT_FAST_TOTAL - 1;
		rga_denise_fast_read = rp;
	}
}

static void denise_update_reg(uae_u16 reg, uae_u16 v, uae_u32 linecnt)
{
	// makes sure fast queue is flushed first
	if (rga_denise_fast_write != rga_denise_fast_read) {
		flush_fast_rga(linecnt);
	}

	struct denise_rga dr = { 0 };
	dr.rga = reg;
	dr.v = v;
	expand_drga_early2x(&dr);
	expand_drga_early(&dr);
	expand_drga(&dr);
	// handle all queued writes immediately
	reswitch_unalign = 0;
	update_bplcon1();
	if (denise_diwhigh2 >= 0) {
		denise_diwhigh = denise_diwhigh2;
		denise_diwhigh2 = -1;
		diwhigh_written = true;
		calchdiw();
	}
	if (aga_delayed_color_idx >= 0) {
		update_color(aga_delayed_color_idx, aga_delayed_color_val, aga_delayed_color_con2, aga_delayed_color_con3);
		aga_delayed_color_idx = -1;
	}
	bplmode = bplmode_new;
	sbasecol[0] = sbasecol2[0];
	sbasecol[1] = sbasecol2[1];
	aga_unalign0 = 0;
	aga_unalign1 = 0;
	check_lts_request();
}

// AGA HAM
static uae_u32 decode_ham_pixel_aga_fast(uint8_t pw, int planes, uae_u8 bxor, uae_u32 *colors_aga)
{
	if (planes >= 7) { /* AGA mode HAM8 */
		int pv = pw ^ bxor;
		int pc = pv >> 2;
		switch (pv & 0x3)
		{
			case 0x0: ham_lastcolor = colors_aga[pc]; break;
			case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pw & 0xFC); break;
			case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pw & 0xFC) << 16; break;
			case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pw & 0xFC) << 8; break;
		}
	} else { /* AGA mode HAM6 */
		int pv = pw ^ bxor;
		uae_u32 pc = ((pw & 0xf) << 0) | ((pw & 0xf) << 4);
		switch (pv & 0x30)
		{
			case 0x00: ham_lastcolor = colors_aga[pv & 0x0f]; break;
			case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= pc << 0; break;
			case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= pc << 16; break;
			case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= pc << 8; break;
		}
	}
	return CONVERT_RGB(ham_lastcolor);
}

// OCS/ECS HAM
static uae_u32 decode_ham_pixel_fast(uint8_t pv, uae_u16 *colors_ocs)
{
	/* OCS/ECS mode HAM6 */
	switch (pv & 0x30)
	{
		case 0x00: ham_lastcolor = colors_ocs[pv]; break;
		case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
		case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
		case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
	}
	return xcolors[ham_lastcolor];
}

// AGA HAM
static uae_u32 decode_ham_pixel_aga(uint8_t pw)
{
	if (denise_planes >= 7) { /* AGA mode HAM8 */
		int pv = pw ^ bplcon4_denise_xor_val;
		int pc = pv >> 2;
		switch (pv & 0x3)
		{
			case 0x0: ham_lastcolor = denise_colors.color_regs_aga[pc]; break;
			case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pw & 0xFC); break;
			case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pw & 0xFC) << 16; break;
			case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pw & 0xFC) << 8; break;
		}
	} else { /* AGA mode HAM6 */
		int pv = pw ^ bplcon4_denise_xor_val;
		uae_u32 pc = ((pw & 0xf) << 0) | ((pw & 0xf) << 4);
		switch (pv & 0x30)
		{
			case 0x00: ham_lastcolor = denise_colors.color_regs_aga[pv & 0x0f]; break;
			case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= pc << 0; break;
			case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= pc << 16; break;
			case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= pc << 8; break;
		}
	}
	return CONVERT_RGB(ham_lastcolor);
}

// OCS/ECS HAM
static uae_u32 decode_ham_pixel(uint8_t pv)
{
	if (!bpldualpf) {
		/* OCS/ECS mode HAM6 */
		switch (pv & 0x30)
		{
			case 0x00: ham_lastcolor = denise_colors.color_regs_ecs[pv]; break;
			case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
			case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
			case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
		}
	} else {
		/* OCS/ECS mode HAM6 + DPF */
		int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
		int idx = lookup[pv];
		switch (pv & 0x30)
		{
			case 0x00: ham_lastcolor = denise_colors.color_regs_ecs[idx]; break;
			case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (idx & 0xF); break;
			case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (idx & 0xF) << 8; break;
			case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (idx & 0xF) << 4; break;
		}
	}
	return p_xcolors[ham_lastcolor];
}

static uint8_t decode_denise_specials(uint8_t pix)
{
	if (decode_specials == 1) {
		// Emulate OCS/ECS only undocumented "SWIV" hardware feature:
		// PF2 >= 5 and bit in plane 5 set: other planes are ignored in color selection.
		if (pix & 0x10) {
			pix = 0x10;
		}
	} else if (decode_specials == 2) {
		// If PFx is invalid (>=5), matching playfield's color becomes transparent
		// (COLOR00). Priorities keep working normally: "transparent" playfield
		// will still hide lower priority playfield behind it.
		// Logo in Running man / Scoopex
		uae_u8 mask1 = 0x01 | 0x04 | 0x10;
		uae_u8 mask2 = 0x02 | 0x08 | 0x20;
		if (plf1pri >= 5 && (pix & mask1)) {
			pix |= 0x40;
		}
		if (plf2pri >= 5 && (pix & mask2)) {
			pix |= 0x80;
		}
	}
	return pix;
}

static uae_u32 decode_pixel_aga(uint8_t pix)
{
	switch (bplmode)
	{
		default:
		return denise_colors.acolors[pix ^ bplcon4_denise_xor_val];
		case CMODE_DUALPF:
		{
			uae_u8 val = dpf_lookup[pix];
			if (dpf_lookup_no[pix]) {
				val += dblpfofs[bpldualpf2of];
			}
			val ^= bplcon4_denise_xor_val;
			return denise_colors.acolors[val];
		}
		case CMODE_EXTRAHB:
		{
			pix ^= bplcon4_denise_xor_val;
			if (pix & 0x20) {
				uae_u32 v = (denise_colors.color_regs_aga[pix & 0x1f] >> 1) & 0x7f7f7f;
				return CONVERT_RGB(v);
			} else {
				return denise_colors.acolors[pix];
			}
		}
		case CMODE_HAM:
		{
			return decode_ham_pixel_aga(pix);
		}
		case CMODE_EXTRAHB_ECS_KILLEHB:
		{
			pix ^= bplcon4_denise_xor_val;
			return denise_colors.acolors[pix & 31];
		}
	}
}

static uae_u32 decode_pixel(uint8_t pix)
{
	switch (bplmode)
	{
		default:
		return denise_colors.acolors[pix];
		case CMODE_DUALPF:
		{
			uae_u8 val = dpf_lookup[pix];
			return denise_colors.acolors[val];
		}
		case CMODE_EXTRAHB:
		{
			pix &= bplehb_mask;
			if (pix <= 31) {
				return denise_colors.acolors[pix];
			} else {
				return p_xcolors[(denise_colors.color_regs_ecs[pix - 32] >> 1) & 0x777];
			}
		}
		case CMODE_HAM:
		{
			return decode_ham_pixel(pix);
		}
	}
}

static uae_u8 denise_render_sprites2(uae_u8 apixel, uae_u32 vs)
{
	uae_u8 c = vs >> 16;
	uae_u16 v = (uae_u16)vs;
	if (currprefs.collision_level) {
		// Sprite to sprite collisiomns
		clxdat |= sprcoltable[c & sprcolmask];
		// Sprite to bitplane collisions
		if ((c & sprcolmask) && currprefs.collision_level > 1) {
			if (bpldualpf) {
				if ((apixel & clxcon_bpl_enable_aa) == clxcon_bpl_match_aa) {
					clxdat |= sprbplcoltable[c & sprcolmask] << 4;
				}
				if ((apixel & clxcon_bpl_enable_55) == clxcon_bpl_match_55) {
					clxdat |= sprbplcoltable[c & sprcolmask] << 0;
				}
			} else {
				if ((apixel & clxcon_bpl_enable_aa) == clxcon_bpl_match_aa) {
					clxdat |= sprbplcoltable[c & sprcolmask] << 4;
					if ((apixel & clxcon_bpl_enable_55) == clxcon_bpl_match_55) {
						clxdat |= sprbplcoltable[c & sprcolmask] << 0;
					}
				}
			}
		}
	}
	int *shift_lookup = bpldualpf ? (bpldualpfpri ? dblpf_ms2 : dblpf_ms1) : dblpf_ms;
	int maskshift, plfmask;
	maskshift = shift_lookup[apixel];
	// The value in the shift lookup table is _half_ the shift count we
	// need. This is because we can't shift 32 bits at once (undefined
	// behaviour in C)
	plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
	v &= ~plfmask;
	v &= debug_sprite_mask;
	if (v) {
		int vlo, vhi, col;
		int v1 = v & 255;
		// OFFS determines the sprite pair with the highest priority that has
		// any bits set.  E.g. if we have 0xFF00 in the buffer, we have sprite
		// pairs 01 and 23 cleared, and pairs 45 and 67 set, so OFFS will
		// have a value of 4.
		// 2 * OFFS is the bit number in V of the sprite pair, and it also
		// happens to be the color offset for that pair. 
		int offs;
		if (v1 == 0)
			offs = 4 + sprite_offs[v >> 8];
		else
			offs = sprite_offs[v1];
		v >>= offs * 2;
		v &= 15;
		if (dspr[offs].attached) {
			col = v;
			if (aga_mode)
				col += sbasecol[1];
			else
				col += 16;
		} else {
			// This sequence computes the correct color value.  We have to select
			// either the lower-numbered or the higher-numbered sprite in the pair.
			// We have to select the high one if the low one has all bits zero.
			// If the lower-numbered sprite has any bits nonzero, (VLO - 1) is in
			// the range of 0..2, and with the mask and shift, VHI will be zero.
			// If the lower-numbered sprite is zero, (VLO - 1) is a mask of
			// 0xFFFFFFFF, and we select the bits of the higher numbered sprite
			// in VHI.
			// This is _probably_ more efficient than doing it with branches.
			vlo = v & 3;
			vhi = (v & (vlo - 1)) >> 2;
			col = (vlo | vhi);
			if (aga_mode) {
				if (vhi > 0)
					col += sbasecol[1];
				else
					col += sbasecol[0];
			} else {
				col += 16;
			}
			col += offs * 2;
		}
		return col;
	}
	return 0;
}

static void get_shres_spr_pix(uae_u32 sv0, uae_u32 sv1, uae_u32 *dpix0, uae_u32 *dpix1)
{
	uae_u16 v;
	uae_u8 p0 = denise_render_sprites2(0, sv0);
	uae_u8 p1 = denise_render_sprites2(0, sv1);
	uae_u8 off = ((p1 & 3) * 4) + (p0 & 3) + ((p0 | p1) & 16);
	if (p0) {
		v = (denise_colors.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		*dpix0 = xcolors[v];
	}
	if (p1) {
		v = (denise_colors.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		*dpix1 = xcolors[v];
	}
}

static void spr_disarms(uae_u32 mask)
{
	int num = 0;
	while (mask && num < MAX_SPRITES) {
		if (mask & (1 << num)) {
			struct denise_spr *sp = &dspr[num];
			spr_arms(sp, 0);
		}
		num++;
	}
}

static void denise_shift_sprites_aga(int shift)
{
	int sidx = 0;
	while (dprspts[sidx]) {
		struct denise_spr *sp = dprspts[sidx];
		if (denise_sprfmode64) {
			sp->dataas64 <<= shift;
			sp->databs64 <<= shift;
		} else {
			sp->dataas <<= shift;
			sp->databs <<= shift;
		}
		sidx++;
	}
}

static uae_u32 denise_render_sprites_aga(int add)
{
	uae_u32 v = 0;
	uae_u32 d = 0;
	int sidx = 0;
	while (dprspts[sidx]) {
		struct denise_spr *sp = dprspts[sidx];
		int num = sp->num;
		int pix = sp->pix;
		v |= pix << (num * 2);
		v |= pix ? (1 << (num + 16)) : 0;
		sp->shift -= add;
		if (sp->shift <= 0) {
			int shift = 1;
			if (sp->shift < 0) {
				// skip pixels if sprite res > display res
				shift = sp->shift < -denise_spr_shiftsize ? 4 : 2;
			}
			sp->shift = denise_spr_shiftsize;
			if (denise_sprfmode64) {
				if (!sp->dataas64 && !sp->databs64) {
					d |= 1 << num;
				}
				sp->pix = ((sp->dataas64 >> 63) & 1) << 0;
				sp->pix |= ((sp->databs64 >> 63) & 1) << 1;
				sp->dataas64 <<= shift;
				sp->databs64 <<= shift;
			} else {
				if (!sp->dataas && !sp->databs) {
					d |= 1 << num;
				}
				sp->pix = ((sp->dataas >> 31) & 1) << 0;
				sp->pix |= ((sp->databs >> 31) & 1) << 1;
				sp->dataas <<= shift;
				sp->databs <<= shift;
			}
		}
		sidx++;
	}
	if (d) {
		spr_disarms(d);
	}
	return v;
}

static uae_u32 denise_render_sprites(void)
{
	uae_u32 v = 0;
	uae_u32 d = 0;
	int sidx = 0;
	while (dprspts[sidx]) {
		struct denise_spr *sp = dprspts[sidx];
		int num = sp->num;
		int pix = sp->pix;
		v |= pix << (num * 2);
		v |= pix ? (1 << (num + 16)) : 0;
		sp->shift--;
		if (sp->shift <= 0) {
			sp->shift = denise_spr_shiftsize;
			if (!sp->dataas && !sp->databs) {
				d |= 1 << num;
			}
			sp->pix = ((sp->dataas >> 31) & 1) << 0;
			sp->pix |= ((sp->databs >> 31) & 1) << 1;
			sp->dataas <<= 1;
			sp->databs <<= 1;
		}
		sidx++;
	}
	if (d) {
		spr_disarms(d);
	}
	return v;
}
static uae_u32 denise_render_sprites_lores(void)
{
	uae_u32 v = 0;
	uae_u32 d = 0;
	int sidx = 0;
	while (dprspts[sidx]) {
		struct denise_spr *sp = dprspts[sidx];
		int num = sp->num;
		int pix = sp->pix;
		v |= pix << (num * 2);
		v |= pix ? (1 << (num + 16)) : 0;
		if (!sp->dataas && !sp->databs) {
			d |= 1 << num;
		}
		sp->pix = ((sp->dataas >> 31) & 1) << 0;
		sp->pix |= ((sp->databs >> 31) & 1) << 1;
		sp->dataas <<= 1;
		sp->databs <<= 1;
		sidx++;
	}
	if (d) {
		spr_disarms(d);
	}
	return v;
}

static uae_u32 denise_render_sprites_ecs_shres(void)
{
	uae_u32 v = 0;
	uae_u32 d = 0;
	for (int s = 0; s < 4; s++) {
		struct denise_spr *sp = &dspr[s];
		int num = sp->num;
		int pix = sp->pix;
		v |= pix << (num * 2);
		v |= pix ? (1 << (num + 16)) : 0;
		sp->shift -= 2;
		if (sp->shift <= 0) {
			sp->shift = denise_spr_shiftsize;
			if (!sp->dataas && !sp->databs) {
				d |= 1 << num;
			}
			sp->pix = ((sp->dataas >> 31) & 1) << 0;
			sp->pix |= ((sp->databs >> 31) & 1) << 1;
			sp->dataas <<= 1;
			sp->databs <<= 1;
		}
	}
	return v;
}

static void do_hb(void)
{
	denise_hblank_active = denise_hblank;
	denise_vblank_active = denise_vblank;
	denise_blank_active2 = denise_hblank_active || denise_vblank_active;
	denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
	denise_sprite_blank_active = denise_blank_active;
}

static void do_hbstrt(int cnt)
{
	// move denise hcounter used to match BPLCON1 reset in HB start to hide the glitch in right edge
	if (!denise_accurate_mode) {
		denise_hcounter_cmp = denise_hcounter;
	}
	denise_hblank = true;
	if (!exthblankon_ecs) {
		hbstrt_offset = internal_pixel_cnt;
		if (delayed_vblank_ecs > 0) {
#ifdef DEBUGGER
			if (debug_dma && !denise_vblank) {
				record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_VB, true);
			}
#endif
			denise_vblank = true;
			delayed_vblank_ecs = 0;
		}
		do_hb();
		hstart_new();
		linear_denise_hbstrt = internal_pixel_cnt;
		// hstop was not matched?
		if (!diwlastword_total && bpl1dat_trigger_offset >= 0) {
			diwlastword_total = internal_pixel_cnt;
		}
	}
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_HB, true);
	}
#endif
}
static void do_hbstop(int cnt)
{
	denise_hblank = false;
	if (!exthblankon_ecs) {
		hbstop_offset = internal_pixel_cnt;
		if (delayed_vblank_ecs < 0) {
#ifdef DEBUGGER
			if (debug_dma && denise_vblank) {
				record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_VB, false);
			}
#endif
			denise_vblank = false;
			delayed_vblank_ecs = 0;
		}
		do_hb();
		linear_denise_hbstop = internal_pixel_cnt;
	}
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_HB, false);
	}
#endif
}

static void do_phbstrt_aga(int cnt)
{
	denise_phblank = true;
	if (exthblankon_aga) {
		hbstrt_offset = internal_pixel_cnt;
		if (delayed_vblank_ecs > 0) {
			denise_pvblank = true;
			delayed_vblank_ecs = 0;
			denise_vblank_active = denise_pvblank;
			denise_blank_active2 = denise_hblank_active || denise_vblank_active;
			denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
		}
		hstart_new();
		linear_denise_hbstrt = internal_pixel_cnt;
		do_exthblankon_aga();
		copy_strlong();
		denise_sprite_blank_active = denise_blank_active;
	}
}
static void do_phbstop_aga(int cnt)
{
	denise_phblank = false;
	if (exthblankon_aga) {
		hbstop_offset = internal_pixel_cnt;
		if (delayed_vblank_ecs < 0) {
			denise_pvblank = false;
			delayed_vblank_ecs = 0;
			denise_vblank_active = denise_pvblank;
			denise_blank_active2 = denise_hblank_active || denise_vblank_active;
			denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
		}
		do_exthblankon_aga();
		denise_sprite_blank_active = denise_blank_active;
		linear_denise_hbstop = internal_pixel_cnt;
	}
}

static void do_phbstrt_ecs(int cnt)
{
	denise_phblank = true;
	if (exthblankon_ecs) {
		hbstrt_offset = internal_pixel_cnt;
		if (delayed_vblank_ecs > 0) {
			denise_pvblank = true;
			delayed_vblank_ecs = 0;
			denise_vblank_active = denise_pvblank;
			denise_blank_active2 = denise_hblank_active || denise_vblank_active;
			denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
			denise_sprite_blank_active = denise_blank_active;
		}
		hstart_new();
		do_exthblankon_ecs();
		linear_denise_hbstrt = internal_pixel_cnt;
	}
}
static void do_phbstop_ecs(int cnt)
{
	denise_phblank = false;
	if (exthblankon_ecs) {
		hbstop_offset = internal_pixel_cnt;
		if (delayed_vblank_ecs < 0) {
			denise_pvblank = false;
			delayed_vblank_ecs = 0;
			denise_vblank_active = denise_pvblank;
			denise_blank_active2 = denise_hblank_active || denise_vblank_active;
			denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
			denise_sprite_blank_active = denise_blank_active;
		}
		do_exthblankon_ecs();
		linear_denise_hbstop = internal_pixel_cnt;
	}
}

static void do_hstrt_aga(int cnt)
{
	if (!denise_hdiw) {
		sprites_hidden2 &= ~1;
		sprites_hidden = sprites_hidden2;
		last_bpl_pix = 0;
		setlasthamcolor();
	}
	denise_hdiw = true;
	hstrt_offset = internal_pixel_cnt;
	if (internal_pixel_cnt < diwfirstword_total && bpl1dat_trigger_offset >= 0) {
		diwfirstword_total = internal_pixel_cnt;
	}
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_HDIW, true);
	}
#endif
}
static void do_hstop_aga(int cnt)
{
	hstop_offset = internal_pixel_cnt;
	sprites_hidden2 |= sprite_hidden_mask;
	sprites_hidden = sprites_hidden2;
	denise_hdiw = false;
	if (internal_pixel_cnt > diwlastword_total && bpl1dat_trigger_offset >= 0) {
		diwlastword_total = internal_pixel_cnt;
	}
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_HDIW, false);
	}
#endif
}
static void do_hstrt_ecs(int cnt)
{
	if (!denise_hdiw) {
		sprites_hidden2 &= ~1;
		sprites_hidden = sprites_hidden2;
		last_bpl_pix = 0;
		setlasthamcolor();
	}
	hstrt_offset = internal_pixel_cnt;
	denise_hdiw = true;
	if (internal_pixel_cnt < diwfirstword_total && bpl1dat_trigger_offset >= 0) {
		diwfirstword_total = internal_pixel_cnt;
	}
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_HDIW, true);
	}
#endif
}
static void do_hstop_ecs(int cnt)
{
	hstop_offset = internal_pixel_cnt;
	denise_hdiw = false;
	if (denisea1000) {
		// A1000 sprite ends 1 lores pixel later
		sprites_hidden2 |= sprite_hidden_mask;
	} else {
		sprites_hidden2 |= sprite_hidden_mask;
		sprites_hidden = sprites_hidden2;
	}
	if (internal_pixel_cnt > diwlastword_total && bpl1dat_trigger_offset >= 0) {
		diwlastword_total = internal_pixel_cnt;
	}
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_HDIW, false);
	}
#endif
}

// fast mode hblank handling
static void check_fast_hb(void)
{
	if (!exthblankon_aga && !exthblankon_ecs) {
		if (delayed_vblank_ecs > 0) {
			denise_vblank = true;
			delayed_vblank_ecs = 0;
		} else if (delayed_vblank_ecs < 0) {
			denise_vblank = false;
			delayed_vblank_ecs = 0;
		}
		denise_hblank = true;
		do_hb();
	} else {
		if (delayed_vblank_ecs > 0) {
			denise_pvblank = true;
			delayed_vblank_ecs = 0;
			denise_vblank_active = denise_pvblank;
			denise_blank_active2 = denise_hblank_active || denise_vblank_active;
			denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
		}
		if (delayed_vblank_ecs < 0) {
			denise_pvblank = false;
			delayed_vblank_ecs = 0;
			denise_vblank_active = denise_pvblank;
			denise_blank_active2 = denise_hblank_active || denise_vblank_active;
			denise_blank_active = denise_blank_enabled ? denise_blank_active2 : false;
		}
		if (exthblankon_aga) {
			do_exthblankon_aga();
		}
		denise_phblank = true;
	}
	hstart_new();
}

// fix strobe position after fast mode
static void denise_handle_quick_strobe(uae_u16 strobe, int offset, int vpos)
{
	struct denise_rga rd = { 0 };
	rd.rga = strobe;
	denise_hcounter_new += maxhpos * 2;
	denise_hcounter_new &= 511;
	denise_hcounter = denise_hcounter_new;
	denise_hcounter_cmp = denise_hcounter;

	//write_log("%d %04x %d %d\n", vpos, strobe, offset, denise_hcounter_new);

	handle_strobes(&rd);
	if (denise_hcounter_new == 1 * 2) {
		// 3 = refresh offset, 2 = pipeline delay
		denise_hcounter_new = (offset - 3 - 2) * 2 + 2;
		denise_hcounter = denise_hcounter_new;
		denise_hcounter_cmp = denise_hcounter;
		check_fast_hb();
	}
}

bool denise_update_reg_queued(uae_u16 reg, uae_u16 v, uae_u32 linecnt)
{
	int wp = rga_denise_fast_write;
	if (((wp + 1) & (DENISE_RGA_SLOT_FAST_TOTAL - 1)) == rga_denise_fast_read) {
		return false;
	}
	struct denise_rga *r = &rga_denise_fast[wp];
	r->rga = reg;
	r->v = v;
	r->line = linecnt;
	wp++;
	wp &= DENISE_RGA_SLOT_FAST_TOTAL - 1;
	rga_denise_fast_write = wp;
	return true;
}

static void do_denise_cck(int linecnt, int startpos, int i)
{
	int idxp = (i + startpos + 1) & DENISE_RGA_SLOT_MASK;
	int idx0 = (i + startpos - 0) & DENISE_RGA_SLOT_MASK;
	int idx1 = (i + startpos - 1) & DENISE_RGA_SLOT_MASK;
	int idx2 = (i + startpos - 1) & DENISE_RGA_SLOT_MASK;
	int idx3 = (i + startpos - 2) & DENISE_RGA_SLOT_MASK;

	denise_hcounter_new = denise_hcounter + 2;
	denise_hcounter_new &= 511;

	if (rga_denise_fast_write != rga_denise_fast_read) {
		flush_fast_rga(linecnt);
	}

	struct denise_rga *rd;

	rd = &rga_denise[idx1];
	if (rd->line == linecnt) {
		expand_drga(rd);
	}

	rd = &rga_denise[idx0];
	if (rd->line == linecnt) {
		expand_drga_early(rd);
	}

#ifdef DEBUGGER
	if (rd->dr) {
		uae_u32 f0;
		if (debug_dma_ptr->denise_evt[1] & DENISE_EVENT_COPIED)  {
			f0 = debug_dma_ptr->denise_evt[1];
		} else {
			f0 = debug_dma_ptr->denise_evt[0];
			debug_dma_ptr->denise_evt[1] = f0;
			debug_dma_ptr->denise_evt_changed[1] = 0;
		}
		f0 &= ~(DENISE_EVENT_COPIED | DENISE_EVENT_UNKNOWN);
		debug_dma_ptr = rd->dr;
		debug_dma_ptr->dhpos[0] = denise_hcounter;
		debug_dma_ptr->denise_evt[0] = f0;
		debug_dma_ptr->denise_evt_changed[0] = 0;
		debug_dma_ptr->denise_evt[1] = 0;
		debug_dma_ptr->denise_evt_changed[1] = 0;
		debug_dma_dhpos_odd = &debug_dma_ptr->dhpos[1];
	}
#endif

	rd = &rga_denise[idxp];
	if (rd->line == linecnt) {
		expand_drga_early2x(rd);
	}

	if (!aga_mode && ecs_denise) {
		int idbe = (i + startpos - 3) & DENISE_RGA_SLOT_MASK;
		rd = &rga_denise[idbe];
		if (rd->line == linecnt) {
			expand_drga_blanken(rd);
		}
	}

}

static void burst_enable(void)
{
	denise_burst = true;
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_BURST, true);
	}
#endif
}
static void burst_disable(void)
{
	denise_burst = false;
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_event_denise(debug_dma_ptr, denise_cycle_half, DENISE_EVENT_BURST, false);
	}
#endif
}

static void lts_unaligned_ecs(int, int, int);
static void lts_unaligned_aga(int, int, int);

static bool checkhorizontal1_ecs(int cnt, int cnt_next, int h)
{
	denise_cycle_half = h;
	if (reswitch_unalign) {
		lts_unaligned_ecs(cnt, cnt_next, h);
#if 0
		if (reswitch_unalign == 1) {
			bplcon1_shift_mask &= 7;
			bplcon1_shift[0] &= 7;
			bplcon1_shift[1] &= 7;
		}
#endif
		if (reswitch_unalign > 0) {
			reswitch_unalign--;
		} else {
			reswitch_unalign++;
		}
		if (reswitch_unalign == 0) {
			update_bplcon1();
		}
		return true;
	}
#if DEBUG_ALWAYS_UNALIGNED_DRAWING
	lts_unaligned_ecs(cnt, cnt_next, h);
	return true;
#endif
	if (cnt == denise_hstrt_lores) {
		do_hstrt_ecs(cnt);
	}
	if (cnt == denise_hstop_lores) {
		do_hstop_ecs(cnt);
	}
	if (cnt_next == denise_hbstrt_lores) {
		do_hbstrt(cnt);
	}
	if (cnt_next == denise_hbstop_lores) {
		do_hbstop(cnt);
	}
	if (!exthblankon_ecs && cnt_next == denise_strlong_lores) {
		copy_strlong();
	}
	if (cnt == 0x40) { // BURST signal activation
		burst_enable();
	}
	if (cnt == 0x52) { // BURST signal deactivation
		burst_disable();
	}
	if (exthblankon_ecsonly) {
		do_exthblankon_ecs();
	}
	if (bpl1dat_unalign && h) {
		bpl1dat_enable_bpls();
		bpl1dat_unalign = false;
	}
	// Delay by 1 lores pixel
	denise_csync_blanken2 = denise_csync_blanken;
	return false;
}

static bool checkhorizontal1_aga(int cnt, int cnt_next, int h)
{
	denise_cycle_half = h;
#if DEBUG_ALWAYS_UNALIGNED_DRAWING
	lts_unaligned_aga(cnt, cnt_next, h);
	return true;
#endif
	if (aga_unalign0 > 0 && !h) {
		aga_unalign0--;
		lts_unaligned_aga(cnt, cnt_next, h);
		return true;
	}
	if (aga_unalign1 > 0 && h) {
		aga_unalign1--;
		lts_unaligned_aga(cnt, cnt_next, h);
		return true;
	}
	if (borderblank) {
		if (cnt == denise_brdstrt_lores) {
			lts_unaligned_aga(cnt, cnt_next, h);
			return true;
		}
		if (cnt == denise_brdstop_lores) {
			lts_unaligned_aga(cnt, cnt_next, h);
			return true;
		}
	}
	if (cnt == denise_hstrt_lores) {
		if (denise_hstrt_unalign) {
			lts_unaligned_aga(cnt, cnt_next, h);
			return true;
		}
		do_hstrt_aga(cnt);
	}
	if (cnt == denise_hstop_lores) {
		if (denise_hstop_unalign) {
			lts_unaligned_aga(cnt, cnt_next, h);
			return true;
		}
		do_hstop_aga(cnt);
	}
	if (cnt_next == denise_hbstrt_lores) {
		do_hbstrt(cnt);
	}
	if (cnt_next == denise_hbstop_lores) {
		do_hbstop(cnt);
	}
	if (cnt_next == denise_phbstrt_lores) {
		if (denise_phbstrt_unalign) {
			lts_unaligned_aga(cnt, cnt_next, h);
			return true;
		}
		do_phbstrt_aga(cnt_next);
	}
	if (cnt_next == denise_phbstop_lores) {
		if (denise_phbstop_unalign) {
			lts_unaligned_aga(cnt, cnt_next, h);
			return true;
		}
		do_phbstop_aga(cnt_next);
	}
	if (!exthblankon_aga && cnt_next == denise_strlong_lores) {
		if (!denise_strlong_unalign) {
			copy_strlong();
		} else {
			lts_unaligned_aga(cnt, cnt_next, h);
			return true;
		}
	}
	return false;
}

#define SPRITE_NEAREST_MIN -3

static void matchsprites2(int cnt)
{
	int sidx = 0;
	while (dprspt[sidx]) {
		struct denise_spr *sp = dprspt[sidx];
		if (sp->armed) {
			if  (cnt == (sp->xpos & ~3)) {
				if (sp->shiftercopydone) {
					sp->shiftercopydone = false;
				} else {
					sp->dataas = sp->dataa;
					sp->databs = sp->datab;

					// ECS Denise + hires sprite bit and not superhires: leftmost pixel is missing
					if (ecs_denise_only && denise_res < RES_SUPERHIRES && (sp->ctl & 0x10)) {
						sp->dataas &= 0x7fffffff;
						sp->databs &= 0x7fffffff;
					}

					if (sp->dataas || sp->databs) {
						spr_arms(sp, 1);
						sp->fmode = denise_sprfmode;
					}
				}
			}
		}
		sidx++;
	}
}

static void matchsprites2_ecs_shres(int cnt)
{
	int sidx = 0;
	while (dprspt[sidx]) {
		struct denise_spr *sp = dprspt[sidx];
		if (sp->armed) {
			if  (cnt == sp->xpos) {
				if (sp->shiftercopydone) {
					sp->shiftercopydone = false;
				} else {
					sp->dataas = sp->dataa;
					sp->databs = sp->datab;
					if (sp->dataas || sp->databs) {
						spr_arms(sp, 1);
						sp->fmode = denise_sprfmode;
					}
				}
			}
		}
		sidx++;
	}
}
static void matchsprites_ecs_shres(int cnt)
{
	denise_spr_nearestcnt--;
	if (denise_spr_nearestcnt < 0 && denise_spr_nearestcnt >= SPRITE_NEAREST_MIN) {
		matchsprites2_ecs_shres(cnt);
		spr_nearest();
	}
}

static void matchsprites(int cnt)
{
	denise_spr_nearestcnt--;
	if (denise_spr_nearestcnt < 0 && denise_spr_nearestcnt >= SPRITE_NEAREST_MIN) {
		matchsprites2(cnt);
		spr_nearest();
	}
}

static void matchsprites_aga(int cnt)
{
	denise_spr_nearestcnt--;
	if (denise_spr_nearestcnt < 0 && denise_spr_nearestcnt >= SPRITE_NEAREST_MIN) {
		int sidx = 0;
		while (dprspt[sidx]) {
			struct denise_spr *sp = dprspt[sidx];
			if (sp->armed) {
				if ((cnt & denise_xposmask) == (sp->xpos & denise_xposmask)) {
					if (sp->shiftercopydone) {
						sp->shiftercopydone = false;
					} else {
						if (denise_sprfmode64) {
							sp->dataas64 = sp->dataa64;
							sp->databs64 = sp->datab64;
							if (sp->dataas64 || sp->databs64) {
								spr_arms(sp, 1);
								sp->fmode = denise_sprfmode;
							}
						} else {
							sp->dataas = sp->dataa;
							sp->databs = sp->datab;
							if (sp->dataas || sp->databs) {
								spr_arms(sp, 1);
								sp->fmode = denise_sprfmode;
							}
						}
					}
				}
			}
			sidx++;
		}
		spr_nearest();
	}
}

static uae_u16 s_bplcon0, s_bplcon1, s_bplcon2, s_bplcon3, s_bplcon4, s_fmode;
static int denise_hcounter_s;
void denise_store_registers(void)
{
	denise_hcounter_s = denise_hcounter; 
	s_bplcon0 = bplcon0_denise;
	s_bplcon1 = bplcon1_denise;
	s_bplcon2 = bplcon2_denise;
	s_bplcon3 = bplcon3_denise;
	s_bplcon4 = bplcon4_denise;
	s_fmode = fmode_denise;
}
void denise_restore_registers(void)
{
	denise_hcounter = denise_hcounter_s;
	expand_bplcon0(s_bplcon0);
	expand_bplcon1(s_bplcon1);
	expand_bplcon2(s_bplcon2);
	expand_bplcon3(s_bplcon3);
	expand_bplcon4_spr(s_bplcon4);
	expand_bplcon4_bm(s_bplcon4);
	expand_fmode(s_fmode);
}

void set_drawbuffer(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;

	if (!drawing_can_lineoptimizations()) {
		vidinfo->inbuffer = &vidinfo->tempbuffer;
		struct vidbuffer *vb = &vidinfo->tempbuffer;
		if (!vb->outwidth || !vb->outheight) {
			struct vidbuffer *vb2 = &vidinfo->drawbuffer;
			vb->outwidth = vb2->outwidth;
			vb->outheight = vb2->outheight;
			vb->inwidth = vb2->inwidth;
			vb->inheight = vb2->inheight;
		}
	} else {
		vidinfo->inbuffer = &vidinfo->drawbuffer;
	}
}

bool start_draw_denise(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = &vidinfo->drawbuffer;

	if (thread_debug_lock) {
		return true;
	}

	vidinfo->outbuffer = vb;

	if (!lockscr(vb, false, display_reset > 0)) {
		return false;
	}

	thread_debug_lock = true;

	set_drawbuffer();
	if (vidinfo->outbuffer != vidinfo->inbuffer) {
		vidinfo->inbuffer->locked = vidinfo->outbuffer->locked;
	}
	init_row_map();

	denise_y_start = 0;
	denise_y_end = -1;

	return true;
}

void end_draw_denise(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = &vidinfo->drawbuffer;

	if (thread_debug_lock) {
		draw_denise_line_queue_flush();

		thread_debug_lock = false;

		unlockscr(vb, denise_y_start, denise_y_end);

		if (vidinfo->outbuffer != vidinfo->inbuffer) {
			vidinfo->inbuffer->locked = vidinfo->outbuffer->locked;
		}
	}
}

// emulate black level calibration (vb and hb)
static uae_u8 blc_prev[3];
static void emulate_black_level_calibration(uae_u32 *b1, uae_u32 *b2, uae_u32 *db, int dtotal, int cstart, int clen)
{
	int shift = hresolution + 1;
	int off;

	if (cstart < 0) {
		off = dtotal + cstart;
	} else {
		off = cstart;
	}
	uae_u32 vv[3] = { 0 };
	int cnt = 0;
	for (int i = 0; i < clen; i++) {
		for (int ii = 0; ii  < (1 << shift); ii++) {
			int j = (off << shift) + (i << shift) + ii;
			uae_u32 v = db[j];
			uae_u8 vi;
			vi = (v >> 16) & 0xff;
			vv[0] += vi;
			vi = (v >> 8) & 0xff;
			vv[1] += vi;
			vi = (v >> 0) & 0xff;
			vv[2] += vi;
			cnt++;
		}
	}
	db[off << shift] = 0xff0000;
	db[(off << shift) + (clen << shift) - 1] = 0xff0000;

	int outc[3];
	for (int i = 0; i < 3; i++) {
		outc[i] = vv[i] / cnt;
	}

//	if (outc[0] > 10 || outc[1] > 10 || outc[2] > 10)
//		write_log("%02x %02x %02x\n", outc[0], outc[1], outc[2]);

	if (outc[0] <= 3 && outc[1] <= 3 && outc[2] <= 3) {
		memcpy(b1, db, (dtotal * sizeof(uae_u32)) << shift);
		if (b1 != b2) {
			memcpy(b2, db, (dtotal * sizeof(uae_u32)) << shift);
		}
	} else {
		for (int i = 0; i < (dtotal << shift); i++) {
			uae_u32 v = *db++;
			if (i >= (clen << shift)) {
				uae_u8 c[3];
				c[0] = v >> 16;
				c[1] = v >> 8;
				c[2] = v >> 0;
				for (int j = 0; j < 3; j++) {
					if (outc[j] > 0) {
						if (c[j] <= outc[j]) {
							c[j] = 0;
						} else {
							c[j] = (c[j] - outc[j]) * 256 / outc[j];
						}
					}
				}
				v = (c[0] << 16) | (c[1] << 8) | (c[2] << 0);
			}
			*b1++ = v;
			*b2++ = v;
		}
	}
}

static uae_u8 filter_pixel_genlock(uae_u8 p1, uae_u8 p2)
{
	return p1 | p2;
}

static uae_u32 filter_pixel(uae_u32 p1, uae_u32 p2)
{
	uae_u32 v = 0;
	v |= ((((p1 >> 16) & 0xff) + ((p2 >> 16) & 0xff)) / 2) << 16;
	v |= ((((p1 >>  8) & 0xff) + ((p2 >>  8) & 0xff)) / 2) << 8;
	v |= ((((p1 >>  0) & 0xff) + ((p2 >>  0) & 0xff)) / 2) << 0;
	return v;
}

// ultra extreme debug pixel patterns
static uint32_t decode_denise_specials_debug(uint32_t v, int inc)
{
	*buf_d++ = v;
	if (decode_specials_debug > 1) {
		int t = ((inc >> 1) + this_line->linear_vpos + 1) & 3;
		if (denise_blank_active2) {
			if (t == 0) {
				v = 0;
			} else if (t == 2) {
				v = xcolors[0x111];
			}
		}
		if (v & 0x01000000) {
			if (t == 0) {
				v = xcolors[0x686];
			}
		}
		if (debug_special_csync) {
			if (new_beamcon0 & BEAMCON0_BLANKEN) {
				if (denise_csync_blanken) {
					v = xcolors[0x006];
				} else if (denise_csync) {
					v = xcolors[0x060];
				}
			} else if (denise_csync) {
				v = xcolors[0x060];
			}
		} else if (debug_special_hvsync) {
			uae_u16 c = 0;
			if (denise_hsync) {
				c |= 0x006;
			}
			if (denise_vsync) {
				c |= 0x060;
			}
			if (c) {
				v = xcolors[c];
			}
		}
	}
	return v;
}

static void flush_null(void)
{
	if (aga_mode) {
		if (aga_delayed_color_idx >= 0) {
			update_color(aga_delayed_color_idx, aga_delayed_color_val, aga_delayed_color_con2, aga_delayed_color_con3);
			aga_delayed_color_idx = -1;
		}
		sbasecol[0] = sbasecol2[0];
		sbasecol[1] = sbasecol2[1];
		bplcon4_denise_xor_val = bplcon4_denise_xor_val2;
	}

	bplmode = bplmode_new;
}

static void lts_null(void)
{
	while (denise_cck < denise_endcycle) {
		do_denise_cck(denise_linecnt, denise_startpos, denise_cck);
		if (lts_changed) return;
		for (int h = 0; h < 2; h++) {
			denise_cycle_half = h;
			if (h) {
				denise_hcounter_next = denise_hcounter_new;
			}

			if (aga_mode) {
				checkhorizontal1_aga(denise_hcounter, denise_hcounter_prev, h);
			} else {
				int cnt = denise_hcounter << 2;
				checkhorizontal1_ecs(cnt, denise_hcounter_next, h);
			}
			flush_null();

#ifdef DEBUGGER
			*debug_dma_dhpos_odd = denise_hcounter;
#endif
			denise_hcounter_cmp++;
			denise_hcounter_cmp &= 511;
			denise_hcounter++;
			denise_hcounter &= 511;
			denise_hcounter_next++;
			denise_hcounter_next &= 511;
		}
		denise_pixtotal++;
		if (denise_pixtotal == 0) {
			internal_pixel_start_cnt = internal_pixel_cnt;
		}
		denise_hcounter = denise_hcounter_new;
		if (denise_accurate_mode) {
			denise_hcounter_cmp = denise_hcounter;
		}
		denise_cck++;
	}
}

static void get_line(int gfx_ypos, enum nln_how how)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = vidinfo->inbuffer;
	int eraselines = 0;
	int yadjust = currprefs.gfx_overscanmode < OVERSCANMODE_ULTRA ? minfirstline_linear << currprefs.gfx_vresolution : 0;
	int xshift = 0;

	xlinebuffer = NULL;
	xlinebuffer2 = NULL;
	xlinebuffer_genlock = NULL;

	denise_pixtotal_max = denise_pixtotalv - denise_pixtotalskip2;
	denise_pixtotal = -denise_pixtotalskip;

	if (!vb->locked) {
		denise_pixtotal_max = -0x7fffffff;
		return;
	}

	if (erase_next_draw) {
		// clear lines if mode height changed
		struct vidbuf_description* vidinfo = &adisplays[0].gfxvidinfo;
		int l = 0;
		while (l < vb->inheight) {
			uae_u8* b = row_map[l];
			memset(b, 0, vb->inwidth * vb->pixbytes);
			l++;
		}
		erase_next_draw = false;
	}

	gfx_ypos -= yadjust;

	if (how == nln_none) {
		return;
	}

	if (gfx_ypos >= 0 && gfx_ypos < vb->inheight) {
		denise_y_end = gfx_ypos + 1;
		switch (how)
		{
			case nln_lower_black_always:
			if (gfx_ypos + 1 < vb->inheight) {
				setxlinebuffer(0, gfx_ypos + 1);
				memset(xlinebuffer, 0, vb->inwidth * vb->pixbytes);
			}
			break;
			case nln_upper_black_always:
			if (gfx_ypos > 0) {
				setxlinebuffer(0, gfx_ypos - 1);
				memset(xlinebuffer, 0, vb->inwidth * vb->pixbytes);
			}
			break;
			case nln_doubled:
			if (gfx_ypos + 1 < vb->inheight) {
				setxlinebuffer(0, gfx_ypos + 1);
				xlinebuffer2 = xlinebuffer;
				xlinebuffer2_start = xlinebuffer_start;
				xlinebuffer2_end = xlinebuffer_end;
				denise_y_end++;
			}
			break;
			case nln_nblack:
			if (gfx_ypos + 1 < vb->inheight) {
				setxlinebuffer(0, gfx_ypos + 1);
				memset(xlinebuffer, 0, vb->inwidth * vb->pixbytes);
				denise_y_end++;
			}
			break;
		}
		setxlinebuffer(0, gfx_ypos);
		xshift = linetoscr_x_adjust >> hresolution;
		denise_pixtotal -= xshift;
	}

	buf1 = (uae_u32*)xlinebuffer;
	if (!xlinebuffer2) {
		xlinebuffer2 = xlinebuffer;
		xlinebuffer2_start = xlinebuffer_start;
		xlinebuffer2_end = xlinebuffer_end;
	}
	buf2 = (uae_u32*)xlinebuffer2;
	buf_d = debug_bufx;
	gbuf = xlinebuffer_genlock;

	if (buf1) {
		for (int i = 0; i < denise_lol_shift_prev; i++) {
			if (buf1 >= (uae_u32*)xlinebuffer_start && buf1 < (uae_u32*)xlinebuffer_end) {
				*buf1++ = DEBUG_LOL_COLOR;
				*buf2++ = DEBUG_LOL_COLOR;
				*buf_d++ = 0;
				if (gbuf) {
					*gbuf++ = 0xff;
				}
			} else {
				buf1++;
				buf2++;
				buf_d++;
				if (gbuf) {
					gbuf++;
				}
			}
		}
	}
	
	if ((denise_pixtotal_max << (1 + hresolution)) > vb->inwidth) {
		denise_pixtotal_max = vb->inwidth >> (1 + hresolution);
	}
	if (xshift > 0) {
		denise_pixtotal_max -= xshift;
	}
	if (!buf1) {
		denise_pixtotal_max = -0x7fffffff;
	}

}

static void draw_denise_vsync(int erase)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	struct vidbuffer *vb = vidinfo->inbuffer;

	if (denise_strlong_seen_delay <= 0) {
		if (!denise_strlong_seen && strlong_emulation) {
			int add = 1 << hresolution;
			strlong_emulation = false;
			write_log("STRLONG strobe emulation deactivated.\n");
			select_lts();
			denise_lol_shift_enable = false;
			denise_lol_shift_prev = add;
		}
		denise_strlong_seen = false;
	} else {
		denise_strlong_seen_delay--;
	}

	if (erase || center_y_erase) {
		// delay until next frame to prevent single black frame
		erase_next_draw = true;
		center_y_erase = false;
	}
}

static void denise_draw_update(void)
{
	if (denise_max_planes != denise_planes) {
		int mp = denise_max_planes;
		denise_max_planes = denise_planes;
		if (mp > 4 && denise_bplfmode == 0 && denise_res > 0) {
			select_lts();
		}
		if (mp > 2 && denise_bplfmode == 0 && denise_res > 1) {
			select_lts();
		}
	}
	denise_max_odd_even = denise_odd_even;
}

static uae_u32 *buf1t;
static uae_u32 *buf2t;
static uae_u32 *bufdt;
static uae_u8 *bufg;

static void draw_denise_line(int gfx_ypos, enum nln_how how, uae_u32 linecnt, int startpos, int startcycle, int endcycle, int skip, int skip2, int dtotal, int calib_start, int calib_len, bool lol, int hdelay, bool blanked, bool finalseg, struct linestate *ls)
{
	bool fullline = false;

	if (startcycle == 0) {
		denise_pixtotalv = dtotal;
		denise_pixtotalskip = skip;
		denise_pixtotalskip2 = skip2;
		denise_linecnt = linecnt;
		denise_hdelay = hdelay;
		denise_startpos = startpos;
	}

	denise_cck = startcycle;
	denise_endcycle = endcycle;

	denise_draw_update();

	if (fullline) {
		denise_pixtotalv = denise_endcycle;
		buf1 = debug_buf;
		buf2 = debug_buf;
	}

	if (!row_map) {
		return;
	}

	if (startcycle == 0) {
		get_line(gfx_ypos, how);

		//write_log("# %d %d\n", gfx_ypos, vpos);

		denise_hcounter_prev = -1;
		hbstrt_offset = -1;
		hbstop_offset = -1;
		hstrt_offset = -1;
		hstop_offset = -1;
		bpl1dat_trigger_offset = -1;
		internal_pixel_cnt = 0;
		internal_pixel_start_cnt = 0;

		buf1t = buf1;
		buf2t = buf2;
		bufdt = buf_d;
		bufg = gbuf;
	}

	bool blankedline = (this_line->linear_vpos >= denise_vblank_extra_bottom || this_line->linear_vpos < denise_vblank_extra_top) && currprefs.gfx_overscanmode < OVERSCANMODE_EXTREME && !programmedmode;
	bool line_is_blanked = false;

	if (denise_pixtotal_max == -0x7fffffff || blankedline || blanked) {

		// don't draw vertical blanking if not ultra extreme overscan
		internal_pixel_cnt = -1;
		line_is_blanked = true;
		while (denise_cck < denise_endcycle) {
			while (denise_cck < denise_endcycle) {
				do_denise_cck(denise_linecnt, denise_startpos, denise_cck);
				if (lts_changed) {
					break;
				}
				if (aga_mode) {
					for (int h = 0; h < 2 ;h++) {
						denise_cycle_half = h;
						if (h) {
							denise_hcounter_next = denise_hcounter_new;
						}
						if (checkhorizontal1_aga(denise_hcounter, denise_hcounter_next, h)) continue;
						flush_null();
#ifdef DEBUGGER
						*debug_dma_dhpos_odd = denise_hcounter;
#endif
						denise_hcounter_cmp++;
						denise_hcounter_cmp &= 511;
						denise_hcounter++;
						denise_hcounter &= 511;
						denise_hcounter_next++;
						denise_hcounter_next &= 511;
					}
				} else {
					for (int h = 0; h < 2 ;h++) {
						denise_cycle_half = h;
						if (h) {
							denise_hcounter_next = denise_hcounter_new;
						}
						if (checkhorizontal1_ecs(denise_hcounter, denise_hcounter_next, h)) continue;
						flush_null();
#ifdef DEBUGGER
						*debug_dma_dhpos_odd = denise_hcounter;
#endif
						denise_hcounter_cmp++;
						denise_hcounter++;
						denise_hcounter &= 511;
						denise_hcounter_next++;
						denise_hcounter_next &= 511;
					}
				}
				denise_pixtotal++;
				denise_hcounter = denise_hcounter_new;
				if (denise_accurate_mode) {
					denise_hcounter_cmp = denise_hcounter;
				}
				denise_cck++;
			}
			lts_changed = false;
		}

		if (xlinebuffer) {
			memset(xlinebuffer_start, DEBUG_TVOVERSCAN_V_GRAYSCALE, xlinebuffer_end - xlinebuffer_start);
			if (xlinebuffer2 && xlinebuffer != xlinebuffer2) {
				memset(xlinebuffer2_start, DEBUG_TVOVERSCAN_V_GRAYSCALE, xlinebuffer2_end - xlinebuffer2_start);
			}
			if (xlinebuffer_genlock) {
				memset(xlinebuffer_genlock_start, 0, xlinebuffer_genlock_end - xlinebuffer_genlock_start);
			}
		}

		last_bpl_pix = 0;
		setlasthamcolor();

	} else {

		// visible line
		if (denise_spr_nr_armed) {
			spr_nearest();
		}

		//write_log("- %d\n", vpos);

		while (denise_cck < denise_endcycle) {
			lts();
			lts_changed = false;
		}

		if (!finalseg) {
			return;
		}

#if 0
		static int testtoggle[1000];
		testtoggle[gfx_ypos]++;
		if (testtoggle[gfx_ypos] >= 100) {
			testtoggle[gfx_ypos] = 0;
		}
		uae_u32 cc = testtoggle[gfx_ypos] < 30 ? 0xff00 : (testtoggle[gfx_ypos] < 60 ? 0xff : 0xff0000);
		for (int i = 0; i < 8; i++) {
			if (buf1) {
				buf1t[i] ^= cc;
				if (buf1t != buf2t) {
					buf2t[i] ^= cc;
				}
			}
		}
#endif
		int rshift = RES_MAX - hresolution;
		int hbstrt_offset2 = (hbstrt_offset - internal_pixel_start_cnt) >> rshift;
		int hbstop_offset2 = (hbstop_offset - internal_pixel_start_cnt) >> rshift;
		uae_u32 *hbstrt_ptr1 = hbstrt_offset2 >= 0 ? buf1t + hbstrt_offset2 : NULL;
		uae_u32 *hbstop_ptr1 = hbstop_offset2 >= 0 ? buf1t + hbstop_offset2 : NULL;
		uae_u32 *hbstrt_ptr2 = buf2 && hbstrt_offset2 >= 0 ? buf2t + hbstrt_offset2 : NULL;
		uae_u32 *hbstop_ptr2 = buf2 && hbstop_offset2 >= 0 ? buf2t + hbstop_offset2 : NULL;
		// blank last pixel row if normal overscan mode, it might have NTSC artifacts
		if (denise_strlong_seen && denise_pixtotal_max != -0x7fffffff && hbstrt_ptr1) {
			int add = 1 << hresolution;
			uae_u32 *ptre1 = buf1;
			uae_u32 *ptre2 = buf2;
			if (no_denise_lol) {
				int padd = lol ? 0 : 2;
				uae_u32 *p1 = hbstrt_ptr1 - padd * add;
				uae_u32 *p2 = hbstrt_ptr2 - padd * add;
				for (int i = 0; i < add * 2; i++) {
					if (hbstrt_ptr1 && p1 < ptre1) {
						*p1++ = BLANK_COLOR;
					}
					if (hbstrt_ptr2 && p2 < ptre2) {
						*p2++ = BLANK_COLOR;
					}
				}
				if (!lol) {
					hbstrt_offset -= (1 << RES_MAX) * 2;
				}
			} else if (!ecs_denise && currprefs.gfx_overscanmode <= OVERSCANMODE_OVERSCAN) {
				uae_u32 *p1 = hbstrt_ptr1 - denise_lol_shift_prev;
				uae_u32 *p2 = hbstrt_ptr2 - denise_lol_shift_prev;
				for (int i = 0; i < add * 2; i++) {
					if (hbstrt_ptr1 && p1 < ptre1) {
						*p1++ = BLANK_COLOR;
					}
					if (hbstrt_ptr2 && p2 < ptre2) {
						*p2++ = BLANK_COLOR;
					}
				}
			}
		}
		int right = denise_strlong_seen ? denise_hblank_extra_right - (1 << currprefs.gfx_resolution) : denise_hblank_extra_right;
		if (!programmedmode && (denise_hblank_extra_left > visible_left_border || visible_right_border > right) && currprefs.gfx_overscanmode < OVERSCANMODE_EXTREME) {
			int ww1 = denise_hblank_extra_left > visible_left_border ? (denise_hblank_extra_left - visible_left_border) << 0 : 0;
			int ww2 = visible_right_border > right ? (visible_right_border - right) << 0 : 0;
			for (int i = 0; i < 4; i++) {
				int add = 1 << hresolution;
				uae_u32 *ptr, *ptrs, *ptre;
				int lolshift = 0;
				int w = 0;
				switch (i)
				{
					case 0:
						ptr = hbstrt_ptr1;
						ptrs = buf1t;
						ptre = buf1;
						lolshift = no_denise_lol ? (lol ? add : -add) : (lol ? add : 0);
						w = ww2;
						break;
					case 1:
						ptr = hbstrt_ptr2;
						ptrs = buf2t;
						ptre = buf2;
						lolshift = no_denise_lol ? (lol ? add : -add) : (lol ? add : 0);
						w = ww2;
						break;
					case 2:
						ptr = hbstop_ptr1;
						ptrs = buf1t;
						ptre = buf1;
						lolshift = lol || no_denise_lol ? 0 : add;
						w = ww1;
						break;
					case 3:
						ptr = hbstop_ptr2;
						ptrs = buf2t;
						ptre = buf2;
						lolshift = lol || no_denise_lol ? 0 : add;
						w = ww1;
						break;
				}
				if (ptr && w) {
					int lolshift2 = denise_strlong_seen ? lolshift : 0;
					uae_u32 *p1 = ptr + lolshift2;
					if (i >= 2) {
						if (p1 + w > ptrs && p1 < ptre) {
							int wxadd = 0;
							if (p1 < ptrs) {
								wxadd = addrdiff(p1, ptrs);
								w -= wxadd;
								p1 += wxadd;
							}
							if (p1 + w > ptre) {
								int wadd = addrdiff(p1 + w, ptre);
								w -= wadd;
							}
							if (w > 0) {
								memset(p1, DEBUG_TVOVERSCAN_H_GRAYSCALE, w * sizeof(uae_u32));
								if (bufg) {
									uae_u8 *gp1 = (p1 - ptrs) + bufg + wxadd;
									memset(gp1, 0, w);
								}
							}
						}
					} else {
						if (p1 - w < ptre && p1 >= ptrs) {
							int wxadd = 0;
							p1 -= w;
							if (p1 < ptrs) {
								wxadd = addrdiff(ptrs, p1);
								p1 += wxadd;
								w -= wxadd;
							}
							if (p1 + w > ptre) {
								int wadd = addrdiff(p1 + w, ptre);
								w -= wadd;
							}
							if (w > 0) {
								memset(p1, DEBUG_TVOVERSCAN_H_GRAYSCALE, w * sizeof(uae_u32));
								if (bufg) {
									uae_u8 *gp1 = (p1 - ptrs) + bufg + wxadd;
									memset(gp1, 0, w);
								}
							}
						}
					}
				}
			}
		}

		if (currprefs.display_calibration && xlinebuffer) {
			emulate_black_level_calibration(buf1t, buf2t, bufdt, denise_endcycle, calib_start, calib_len);
		}
	}

	if (ls) {
		ls->hstrt_offset = hstrt_offset;
		ls->hstop_offset = hstop_offset;
		ls->hbstrt_offset = hbstrt_offset;
		ls->hbstop_offset = hbstop_offset;
		ls->bpl1dat_trigger_offset = bpl1dat_trigger_offset;
		ls->internal_pixel_cnt = internal_pixel_cnt;
		ls->internal_pixel_start_cnt = internal_pixel_start_cnt;
		ls->blankedline = blankedline;
		ls->strlong_seen = denise_strlong_seen;
		ls->vb = denise_vblank_active;
		ls->lol = lol;
	}

	if (linear_denise_hbstrt >= 0 && linear_denise_hbstop >= 0 && !denise_vblank_active) {
		linear_denise_frame_hbstrt = linear_denise_hbstrt & ~15;
		linear_denise_frame_hbstop = linear_denise_hbstop & ~15;
	}

	if (refresh_indicator_buffer && buf1t) {
		static const int refresh_indicator_colors[] = { 0x777, 0x0f0, 0x00f, 0xff0, 0xf0f };
		const int indicator_width = 16;
		int yadjust = currprefs.gfx_overscanmode < OVERSCANMODE_ULTRA ? minfirstline_linear << currprefs.gfx_vresolution : 0;
		int lineno = gfx_ypos;
		int w = addrdiff(buf1, buf1t) - indicator_width;
		if (w > 0) {
			if (w > MAX_PIXELS_PER_LINE) {
				w = MAX_PIXELS_PER_LINE;
			}
			if (lineno >= 0 && lineno < refresh_indicator_height) {
				void *p = refresh_indicator_buffer + lineno * MAX_PIXELS_PER_LINE * sizeof(uae_u32);
				void *ps = buf1t + indicator_width;
				if (!memcmp(p, ps, w * sizeof(uae_u32))) {
					if (refresh_indicator_changed[lineno] != 0xff) {
						refresh_indicator_changed[lineno]++;
						if (refresh_indicator_changed[lineno] > refresh_indicator_changed_prev[lineno]) {
							refresh_indicator_changed_prev[lineno] = refresh_indicator_changed[lineno];
						}
					}
				} else {
					memcpy(p, ps, w * sizeof(uae_u32));
					if (refresh_indicator_changed[lineno] != refresh_indicator_changed_prev[lineno]) {
						refresh_indicator_changed_prev[lineno] = 0;
					}
					refresh_indicator_changed[lineno] = 0;
				}
			}

			uae_u8 pixel = refresh_indicator_changed_prev[lineno];
			int color1 = 0;
			int color2 = 0;
			if (pixel <= 4) {
				color1 = color2 = refresh_indicator_colors[pixel];
			} else if (pixel <= 8) {
				color2 = refresh_indicator_colors[pixel - 5];
			}
			for (int x = 0; x < 8; x++) {
				buf1t[x] = xcolors[color1];
				if (buf2t) {
					buf2t[x] = xcolors[color1];
				}
			}
			for (int x = 8; x < 16; x++) {
				buf1t[x] = xcolors[color2];
				if (buf2t) {
					buf2t[x] = xcolors[color2];
				}
			}
		}
	}

	if (!line_is_blanked && denise_planes > 0) {
		resolution_count[denise_res]++;
	}
	lines_count++;
}

// optimized drawing routines
#include "linetoscr_common.cpp"
#include "linetoscr_ocs_ecs.cpp"
#include "linetoscr_ocs_ecs_genlock.cpp"
#include "linetoscr_ocs_ecs_ntsc.cpp"
#include "linetoscr_ocs_ecs_ntsc_genlock.cpp"
#include "linetoscr_aga_fm0.cpp"
#include "linetoscr_aga_fm1.cpp"
#include "linetoscr_aga_fm2.cpp"
#include "linetoscr_aga_fm0_genlock.cpp"
#include "linetoscr_aga_fm1_genlock.cpp"
#include "linetoscr_aga_fm2_genlock.cpp"
#include "linetoscr_ecs_shres.cpp"
#include "linetoscr_ecs_fast.cpp"
#include "linetoscr_aga_fast.cpp"
#include "linetoscr_ecs_genlock_fast.cpp"
#include "linetoscr_aga_genlock_fast.cpp"

// select linetoscr routine
static void select_lts(void)
{
	LINETOSRC_FUNC lts_old = lts;

	bool samecycle = denise_hcounter == denise_hcounter_prev;
	int bm = bplmode_new;
	// if plane count decreases mid-scanline, old data in higher plane count shifters must be still shifted out.
	if (denise_planes > denise_max_planes) {
		denise_max_planes = denise_planes;
	}
	if (denise_bplfmode == 0 && denise_res > 0 && denise_max_planes > 4) {
		denise_max_planes = 4;
	}
	if (denise_bplfmode == 0 && denise_res > 1 && denise_max_planes > 2) {
		denise_max_planes = 2;
	}
	if (denise_bplfmode == 1 && denise_res > 1 && denise_max_planes > 4) {
		denise_max_planes = 4;
	}
	if (denise_max_planes > 6 && (bm == CMODE_EXTRAHB || bm == CMODE_EXTRAHB_ECS_KILLEHB)) {
		denise_max_planes = 6;
	}
	if (denise_max_planes == 7 && bm == CMODE_HAM) {
		denise_max_planes = 6;
	}
	denise_odd_even = bplcon1_shift_full[0] != bplcon1_shift_full[1];
	if (denise_odd_even) {
		denise_max_odd_even = denise_odd_even;
	}
	if (!denise_odd_even) {
		bplshiftcnt[1] = bplshiftcnt[0];
	}
	hresolution_add = 1 << hresolution;

	if (denise_max_planes <= 4 && bm == CMODE_HAM) {
		bm = CMODE_NORMAL;
	}
	if (denise_max_planes <= 5 && (bm == CMODE_EXTRAHB || bm == CMODE_EXTRAHB_ECS_KILLEHB)) {
		bm = CMODE_NORMAL;
	}

	if (aga_mode) {

		int spr = 0;
		if (denise_spr_nr_armed || samecycle) {
			spr = 1;
		}
		if (need_genlock_data) {
			int planes = denise_max_planes > 4 ? 1 : 0;
			int oddeven = denise_max_odd_even ? 1 : 0;
			int idx = (oddeven) + (bm * 2) + (planes * 2 * 5) + (spr * 2 * 5 * 2) + (denise_res * 2 * 5 * 2 * 2) + (hresolution * 2 * 5 * 2 * 2 * 3) + (bpldat_fmode * 2 * 5 * 2 * 2 * 3 * 3);
			if (currprefs.gfx_lores_mode) {
				lts = linetoscr_aga_genlock_funcs_filtered[idx];
				if (!lts) {
					lts = linetoscr_aga_genlock_funcs[idx];
				}
			} else {
				lts = linetoscr_aga_genlock_funcs[idx];
			}
		} else {
			int planes = denise_max_planes > 0 ? (denise_max_planes - 1) / 2 : 0;
			int oddeven = denise_max_odd_even ? 1 : 0;
			int idx = (oddeven) + (bm * 2) + (planes * 2 * 5) + (spr * 2 * 5 * 4) + (denise_res * 2 * 5 * 4 * 2) + (hresolution * 2 * 5 * 4 * 2 * 3) + (bpldat_fmode * 2 * 5 * 4 * 2 * 3 * 3);
			if (currprefs.gfx_lores_mode) {
				lts = linetoscr_aga_funcs_filtered[idx];
				if (!lts) {
					lts = linetoscr_aga_funcs[idx];
				}
			} else {
				lts = linetoscr_aga_funcs[idx];
			}
		}

	} else if (ecs_denise && denise_res == RES_SUPERHIRES) {

		if (hresolution == RES_LORES) {
			lts = lts_null;
		} else {
			int idx = hresolution;
			if (idx == RES_HIRES && currprefs.gfx_lores_mode) {
				if (need_genlock_data) {
					lts = lts_ecs_shres_dhires_genlock_filtered;
				} else {
					lts = lts_ecs_shres_dhires_filtered;
				}
			} else {
				if (need_genlock_data) {
					lts = linetoscr_ecs_shres_genlock_funcs[idx];
				} else {
					lts = linetoscr_ecs_shres_funcs[idx];
				}
			}
		}

	} else {

		int spr = 0;
		if (denise_spr_nr_armed || samecycle) {
			spr = 1;
		}
		if (need_genlock_data) {
			int oddeven = denise_max_odd_even;
			int idx = (oddeven) + (bm * 2) + (spr * 2 * 4) + (denise_res * 2 * 4 * 2) + (hresolution * 2 * 4 * 2 * 2);
			lts = strlong_emulation ? linetoscr_ecs_ntsc_genlock_funcs[idx] : linetoscr_ecs_genlock_funcs[idx];
		} else {
			int planes;
			switch (denise_max_planes)
			{
				case 0:
				case 1:
				case 2:
				planes = 0;
				break;
				case 3:
				case 4:
				planes = 1;
				break;
				case 5:
				planes = 2;
				break;
				default:
				if (bplehb_eke) {
					planes = 2;
				} else {
					planes = 3;
				}
				break;
			}
			int oddeven = denise_max_odd_even;
			int idx = (oddeven) + (bm * 2) + (planes * 2 * 4) + (spr * 2 * 4 * 4) + (denise_res * 2 * 4 * 4 * 2) + (hresolution * 2 * 4 * 4 * 2 * 2);
			if (currprefs.gfx_lores_mode) {
				lts = strlong_emulation ? linetoscr_ecs_ntsc_funcs_filtered[idx] : linetoscr_ecs_funcs_filtered[idx];
				if (!lts) {
					lts = strlong_emulation ? linetoscr_ecs_ntsc_funcs[idx] : linetoscr_ecs_funcs[idx];
				}
			} else {
				lts = strlong_emulation ? linetoscr_ecs_ntsc_funcs[idx] : linetoscr_ecs_funcs[idx];
			}
		}

	}

	if (lts_old != lts) {
		lts_changed = true;
	}
	lts_request = false;
	denise_hcounter_prev = denise_hcounter;
}

STATIC_INLINE uae_u8 getbpl(void)
{
	uae_u8 pix;
	if (denise_bplfmode == 2) {
		pix = getbpl8_64();
	} else if (denise_bplfmode == 1) {
		pix = getbpl8_32();
	} else {
		pix = getbpl8();
	}
	return pix;
}

// Generic and unoptimized shres based (outputs 4xshres,2xhires or 1xlores pixels) display emulation,
// only used when subpixel support is needed
// (lores pixel has hires or shres color change or hires pixel has shres color change)
// For example:
// HDIW or HB is not lores-aligned
// or BPLCON4 changes or AGA color register is modified.
static void lts_unaligned_aga(int cnt, int cnt_next, int h)
{
	if (!denise_odd_even) {
		bplshiftcnt[1] = bplshiftcnt[0];
	}

	// hardwired hblank
	if (cnt_next == denise_hbstrt_lores) {
		do_hbstrt(cnt);
	}
	if (cnt_next == denise_hbstop_lores) {
		do_hbstop(cnt);
	}

	cnt <<= 2;
	cnt_next <<= 2;

	int dpixcnt = 0;

	int xshift = RES_MAX - hresolution;
	int xadd = 1 << xshift;
	int denise_res_size2 = denise_res_size << xshift;
	if (denise_res > hresolution) {
		xshift = RES_SUPERHIRES - denise_res;
		xadd = 1 << xshift;
		denise_res_size2 = denise_res_size;
	}
	int ipix = 0;
	for (int i = 0; i < (1 << RES_MAX); i += xadd, ipix += xadd) {

		if (i == 2 || !hresolution) {

			if (!h) {
				if (aga_delayed_color_idx >= 0) {
					update_color(aga_delayed_color_idx, aga_delayed_color_val, aga_delayed_color_con2, aga_delayed_color_con3);
					aga_delayed_color_idx = -1;
				}
				bplmode = bplmode_new;

			}
			if (h) {
				bplcon4_denise_xor_val = bplcon4_denise_xor_val2;
				sbasecol[0] = sbasecol2[0];
				sbasecol[1] = sbasecol2[1];

				if (bpl1dat_unalign) {
					bpl1dat_enable_bpls();
					bpl1dat_unalign = false;
				}

			}

		}

		if (cnt == denise_hstrt) {
			do_hstrt_aga(cnt);
		}
		if (cnt == denise_hstop) {
			do_hstop_aga(cnt);
		}
		if (cnt_next == denise_phbstrt) {
			do_phbstrt_aga(cnt);
		}
		if (cnt_next == denise_phbstop) {
			do_phbstop_aga(cnt);
		}
		if (!exthblankon_aga && cnt_next == denise_strlong_hd) {
			copy_strlong();
		}
		
		matchsprites_aga(cnt);

		// bitplanes
		uae_u8 pix = 0;
		uae_u32 dpix_val = BLANK_COLOR;
		uae_u8 gpix = 0xff;
		if (!denise_blank_active) {
			// borderblank ends 1 shres pixel early
			dpix_val = cnt == denise_brdstop && (denise_hdiw || cnt + 1 == denise_hstrt) ? denise_colors.acolors[0] : bordercolor;
			gpix = 0;
			if (denise_hdiw && bpl1dat_trigger) {
				pix = loaded_pixs[ipix];
				// odd to even bitplane collision
				clxdat |= bplcoltable[pix];
				dpix_val = decode_pixel_aga(pix);
				gpix = pix;
				// borderblank starts 1 shres pixel early
				if (cnt == denise_brdstrt) {
					dpix_val = fullblack;
				}
			}
			last_bpl_pix = pix;
		}

		if (!denise_odd_even) {
			// both even and odd shifting
			bplshiftcnt[0] += denise_res_size2;
			if (bplshiftcnt[0] >= 4) {
				bplshiftcnt[0] = 0;
				if (denise_bplfmode64) {
					shiftbpl8_64();
				} else {
					shiftbpl8();
				}
				loaded_pix = getbpl();
			}
		} else {
			// even planes shifting
			bplshiftcnt[0] += denise_res_size2;
			if (bplshiftcnt[0] >= 4) {
				bplshiftcnt[0] = 0;
				if (denise_bplfmode64) {
					shiftbpl8e_64();
				} else {
					shiftbpl8e();
				}
				loaded_pix = getbpl();
			}
			// odd planes shifting
			bplshiftcnt[1] += denise_res_size2;
			if (bplshiftcnt[1] >= 4) {
				bplshiftcnt[1] = 0;
				if (denise_bplfmode64) {
					shiftbpl8o_64();
				} else {
					shiftbpl8o();
				}
				loaded_pix = getbpl();
			}
		}

		int dhv = cnt & bplcon1_shift_mask_full;
		if (!denise_odd_even) {
			// both even and odd planes copy
			if (bpldat_copy[0] && dhv == bplcon1_shift_full[0]) {
				if (denise_bplfmode64) {
					copybpl8_64();
				} else {
					copybpl8();
				}
				bplshiftcnt[0] = 0;
				loaded_pix = getbpl();
			}
		} else {
			// even planes copy
			if (bpldat_copy[0] && dhv == bplcon1_shift_full[0]) {
				if (denise_bplfmode64) {
					copybpl8e_64();
				} else {
					copybpl8e();
				}
				bplshiftcnt[0] = 0;
				loaded_pix = getbpl();
			}
			// odd planes copy
			if (bpldat_copy[1] && dhv == bplcon1_shift_full[1]) {
				if (denise_bplfmode64) {
					copybpl8o_64();
				} else {
					copybpl8o();
				}
				bplshiftcnt[1] = 0;
				loaded_pix = getbpl();
			}
		}

		loaded_pixs[ipix] = loaded_pix;

		// sprite rendering
		uae_u32 sv = 0;
		if (denise_spr_nr_armeds) {
			uae_u32 svt = denise_render_sprites_aga(xadd);
			if (!denise_blank_active && !sprites_hidden) {
				sv = svt;
			}
		}

		if (sv) {
			uae_u32 spix = denise_render_sprites2(pix, sv);
			if (spix) {
				dpix_val = denise_colors.acolors[spix];
				gpix = spix;
			}
		}
		dtbuf[h][ipix] = dpix_val;
		dtgbuf[h][ipix] = gpix;

		// bitplane and sprite merge & output
		if (!dpixcnt && buf1 && denise_pixtotal >= 0 && denise_pixtotal < denise_pixtotal_max) {

			uae_u32 t = dtbuf[h ^ lol][ipix]; 
#ifdef DEBUGGER
			if (decode_specials_debug) {
				t = decode_denise_specials_debug(t, cnt);
			}
#endif
			*buf1++ = t;
			*buf2++ = t;
			if (gbuf) {
				*gbuf++ = dtgbuf[h ^ lol][ipix];
			}
		}

		dpixcnt += hresolution_add << xshift;
		dpixcnt &= (1 << RES_SUPERHIRES) - 1;

		cnt += xadd;
		cnt_next += xadd;
		internal_pixel_cnt += xadd;
	}

	sprites_hidden = sprites_hidden2;

#ifdef DEBUGGER
	*debug_dma_dhpos_odd = denise_hcounter;
#endif

	denise_hcounter_cmp++;
	denise_hcounter_cmp &= 511;
	denise_hcounter++;
	denise_hcounter &= 511;
	denise_hcounter_next++;
	denise_hcounter_next &= 511;

	if (!denise_odd_even) {
		bplshiftcnt[1] = bplshiftcnt[0];
	}
}


static void lts_unaligned_ecs(int cnt, int cnt_next, int h)
{
	if (!denise_odd_even) {
		bplshiftcnt[1] = bplshiftcnt[0];
	}

	// hardwired hblank (both aga and ocs/ecs)
	if (cnt_next == denise_hbstrt_lores) {
		do_hbstrt(cnt);
	}
	if (cnt_next == denise_hbstop_lores) {
		do_hbstop(cnt);
	}

	// ocs/ecs hdiw
	if (cnt == denise_hstrt_lores) {
		do_hstrt_ecs(cnt);
	}
	if (cnt == denise_hstop_lores) {
		do_hstop_ecs(cnt);
	}
	if (cnt == 0x40) { // BURST signal activation
		burst_enable();
	}
	if (cnt == 0x52) { // BURST signal deactivation
		burst_disable();
	}
	if (exthblankon_ecsonly) {
		do_exthblankon_ecs();
		if (!denise_csync_blanken2 && denise_csync_blanken && !denise_vblank_active) {
			// When ECS Denise CSYNC programmed blank activates: clear bpl1dat border state and sprite visible state
			hstart_new();
		}
	}

	cnt <<= 2;
	cnt_next <<= 2;

	matchsprites(cnt);

	int dpixcnt = 0;

	int ipix = 0;
	for (int i = 0; i < (1 << RES_SUPERHIRES); i++, ipix++) {

		if (i == 2) {

			if (h) {
				if (bpl1dat_unalign) {
					bpl1dat_enable_bpls();
					bpl1dat_unalign = false;
				}
			}

			// DIWHIGH has extra 0.5 CCK delay
			if (denise_diwhigh2 >= 0) {
				denise_diwhigh = denise_diwhigh2;
				denise_diwhigh2 = -1;
				diwhigh_written = true;
				calchdiw();
			}

		}

		// bitplanes
		uae_u32 dpix_val = BLANK_COLOR;
		uae_u8 pix = 0;
		uae_u8 gpix = 0xff;
		if (!denise_blank_active) {
			// borderblank ends 1 shres pixel early
			dpix_val = cnt == denise_brdstop && (denise_hdiw || cnt + 1 == denise_hstrt) ? denise_colors.acolors[0] : bordercolor;
			gpix = 0;
			if (denise_hdiw && bpl1dat_trigger) {
				pix = getbpl6();

#if 0
				if (reswitch_unalign == 2 && i >= 2) {
					pix = 0;
				}
				if (reswitch_unalign == 3 && i >= 2) {
					pix = 0;
				}
#endif

				// odd to even bitplane collision
				clxdat |= bplcoltable[pix];
				if (decode_specials) {
					pix = decode_denise_specials(pix);
				}
				dpix_val = decode_pixel(pix);
				gpix = pix;
				// borderblank starts 1 shres pixel early
				if (cnt == denise_brdstrt) {
					dpix_val = fullblack;
				}
				last_bpl_pix = pix;
			}
		}

		if (bplcon1_shift[0] == bplcon1_shift[1]) {
			// both even and odd shifting
			bplshiftcnt[0] += denise_res_size;
			if (bplshiftcnt[0] >= 4) {
				bplshiftcnt[0] = 0;
				shiftbpl6();
			}
		} else {
			// even planes shifting
			bplshiftcnt[0] += denise_res_size;
			if (bplshiftcnt[0] >= 4) {
				bplshiftcnt[0] = 0;
				shiftbpl6e();
			}
			// odd planes shifting
			bplshiftcnt[1] += denise_res_size;
			if (bplshiftcnt[1] >= 4) {
				bplshiftcnt[1] = 0;
				shiftbpl6o();
			}
		}

		// sprite rendering
		uae_u32 sv = 0;
		if (denise_spr_nr_armeds) {
			uae_u32 svt = denise_render_sprites();
			if (!denise_blank_active && !sprites_hidden) {
				sv = svt;
			}
		}

		if (sv) {
			uae_u32 spix = denise_render_sprites2(pix, sv);
			if (spix) {
				dpix_val = denise_colors.acolors[spix];
				gpix = spix;
			}
		}
		dtbuf[h][ipix] = dpix_val;
		dtgbuf[h][ipix] = gpix;

		// bitplane and sprite merge & output
		if (!dpixcnt && buf1 && denise_pixtotal >= 0 && denise_pixtotal < denise_pixtotal_max) {

			uae_u32 t = dtbuf[h ^ lol][ipix];

#ifdef DEBUGGER
			if (decode_specials_debug) {
				t = decode_denise_specials_debug(t, cnt);
			}
#endif

#if 0
			if (reswitch_unalign < 0) {
				t |= 0x0000ff;
			} else if (reswitch_unalign == 1) {
				t |= 0xffff00;
			}
#endif

			*buf1++ = t;
			*buf2++ = t;
			if (gbuf) {
				*gbuf++ = dtgbuf[h ^ lol][ipix];
			}

		}

		dpixcnt += hresolution_add;
		dpixcnt &= (1 << RES_SUPERHIRES) - 1;

		cnt++;
		cnt_next++;
		internal_pixel_cnt++;
	}

	int dhv = denise_hcounter_cmp & bplcon1_shift_mask;
	if (bplcon1_shift[0] == bplcon1_shift[1]) {
		// both even and odd planes copy
		if (bpldat_copy[0] && dhv == bplcon1_shift[0]) {
			copybpl6();
		}
	} else {
		// even planes copy
		if (bpldat_copy[0] && dhv == bplcon1_shift[0]) {
			copybpl6e();
		}
		// odd planes copy
		if (bpldat_copy[1] && dhv == bplcon1_shift[1]) {
			copybpl6o();
		}
	}

	sprites_hidden = sprites_hidden2;

#ifdef DEBUGGER
	*debug_dma_dhpos_odd = denise_hcounter;
#endif

	denise_hcounter_cmp++;
	denise_hcounter++;
	denise_hcounter &= 511;
	denise_hcounter_next++;
	denise_hcounter_next &= 511;

	if (!denise_odd_even) {
		bplshiftcnt[1] = bplshiftcnt[0];
	}
}

/* We use the compiler's inlining ability to ensure that PLANES is in effect a compile time
constant.  That will cause some unnecessary code to be optimized away.
Don't touch this if you don't know what you are doing.  */

#define MERGE32(a,b,mask,shift) do {\
	uae_u32 tmp = mask & (a ^ (b >> shift)); \
	a ^= tmp; \
	b ^= (tmp << shift); \
} while (0)

#define MERGE64(a,b,mask,shift) do {\
	uae_u64 tmp = mask & (a ^ (b >> shift)); \
	a ^= tmp; \
	b ^= (tmp << shift); \
} while (0)

#define DOLINE_SWAP 0

STATIC_INLINE uae_u32 GETLONG32_16(uae_u8 *P)
{
	uae_u32 v = (*(uae_u32 *)P);
	v = (v >> 16) | (v << 16);
	return v;
}
#define GETLONG32_8(P) (do_get_mem_long((uae_u32*)P))
#define GETLONG32_32(P) (*((uae_u32*)P))
#define GETLONG32_64(P) (*((uae_u32*)P))
#define GETLONG64(P) (*(uae_u64*)P)


STATIC_INLINE void pfield_doline32_8(uae_u32 *pixels, int wordcount, int planes, uae_u8 *real_bplpt[8])
{
	while (wordcount-- > 0) {
		uae_u32 b0, b1, b2, b3, b4, b5, b6, b7;

		b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;
		switch (planes) {
#ifdef AGA
			case 8: b0 = GETLONG32_8(real_bplpt[7]); real_bplpt[7] += 4;
			case 7: b1 = GETLONG32_8(real_bplpt[6]); real_bplpt[6] += 4;
#endif
			case 6: b2 = GETLONG32_8(real_bplpt[5]); real_bplpt[5] += 4;
			case 5: b3 = GETLONG32_8(real_bplpt[4]); real_bplpt[4] += 4;
			case 4: b4 = GETLONG32_8(real_bplpt[3]); real_bplpt[3] += 4;
			case 3: b5 = GETLONG32_8(real_bplpt[2]); real_bplpt[2] += 4;
			case 2: b6 = GETLONG32_8(real_bplpt[1]); real_bplpt[1] += 4;
			case 1: b7 = GETLONG32_8(real_bplpt[0]); real_bplpt[0] += 4;
		}

		MERGE32(b0, b1, 0x55555555, 1);
		MERGE32(b2, b3, 0x55555555, 1);
		MERGE32(b4, b5, 0x55555555, 1);
		MERGE32(b6, b7, 0x55555555, 1);

		MERGE32(b0, b2, 0x33333333, 2);
		MERGE32(b1, b3, 0x33333333, 2);
		MERGE32(b4, b6, 0x33333333, 2);
		MERGE32(b5, b7, 0x33333333, 2);

		MERGE32(b0, b4, 0x0f0f0f0f, 4);
		MERGE32(b1, b5, 0x0f0f0f0f, 4);
		MERGE32(b2, b6, 0x0f0f0f0f, 4);
		MERGE32(b3, b7, 0x0f0f0f0f, 4);

		MERGE32(b0, b1, 0x00ff00ff, 8);
		MERGE32(b2, b3, 0x00ff00ff, 8);
		MERGE32(b4, b5, 0x00ff00ff, 8);
		MERGE32(b6, b7, 0x00ff00ff, 8);

		MERGE32(b0, b2, 0x0000ffff, 16);
#if DOLINE_SWAP
		pixels[0] = b0;
		pixels[4] = b2;
#else
		do_put_mem_long(pixels + 0, b0);
		do_put_mem_long(pixels + 4, b2);
#endif
		MERGE32(b1, b3, 0x0000ffff, 16);
#if DOLINE_SWAP
		pixels[2] = b1;
		pixels[6] = b3;
#else
		do_put_mem_long(pixels + 2, b1);
		do_put_mem_long(pixels + 6, b3);
#endif
		MERGE32(b4, b6, 0x0000ffff, 16);
#if DOLINE_SWAP
		pixels[1] = b4;
		pixels[5] = b6;
#else
		do_put_mem_long(pixels + 1, b4);
		do_put_mem_long(pixels + 5, b6);
#endif
		MERGE32(b5, b7, 0x0000ffff, 16);
#if DOLINE_SWAP
		pixels[3] = b5;
		pixels[7] = b7;
#else
		do_put_mem_long(pixels + 3, b5);
		do_put_mem_long(pixels + 7, b7);
#endif
		pixels += 8;
	}
}

/* See above for comments on inlining.  These functions should _not_
be inlined themselves.  */
static void NOINLINE pfield_doline32_n1_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 1, real_bplpt); }
static void NOINLINE pfield_doline32_n2_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 2, real_bplpt); }
static void NOINLINE pfield_doline32_n3_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 3, real_bplpt); }
static void NOINLINE pfield_doline32_n4_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 4, real_bplpt); }
static void NOINLINE pfield_doline32_n5_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 5, real_bplpt); }
static void NOINLINE pfield_doline32_n6_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 6, real_bplpt); }
#ifdef AGA
static void NOINLINE pfield_doline32_n7_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 7, real_bplpt); }
static void NOINLINE pfield_doline32_n8_8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_8(data, count, 8, real_bplpt); }
#endif
static void pfield_doline_8(int planecnt, int wordcount, uae_u8 *datap, struct linestate *ls)
{
	uae_u8 **real_bplpt = ls->bplpt;
	uae_u32 *data = (uae_u32 *)datap;
	switch (planecnt) {
		default: break;
		case 0: memset(data, 0, wordcount * 32); break;
		case 1: pfield_doline32_n1_8(data, wordcount, real_bplpt); break;
		case 2: pfield_doline32_n2_8(data, wordcount, real_bplpt); break;
		case 3: pfield_doline32_n3_8(data, wordcount, real_bplpt); break;
		case 4: pfield_doline32_n4_8(data, wordcount, real_bplpt); break;
		case 5: pfield_doline32_n5_8(data, wordcount, real_bplpt); break;
		case 6: pfield_doline32_n6_8(data, wordcount, real_bplpt); break;
#ifdef AGA
		case 7: pfield_doline32_n7_8(data, wordcount, real_bplpt); break;
		case 8: pfield_doline32_n8_8(data, wordcount, real_bplpt); break;
#endif
	}
}

static void tvadjust(int *hbstrt_offset, int *hbstop_offset, struct linestate *ls)
{
	if (!programmedmode && currprefs.gfx_overscanmode < OVERSCANMODE_EXTREME) {
		int right = denise_strlong_seen ? denise_hblank_extra_right - (1 << currprefs.gfx_resolution) : denise_hblank_extra_right;
		int ww1 = denise_hblank_extra_left > visible_left_border ? (denise_hblank_extra_left - visible_left_border) << 0 : 0;
		int ww2 = visible_right_border > right ? (visible_right_border - right) << 0 : 0;
	
		if (hbstrt_offset && ww2 > 0) {
			*hbstrt_offset -= ww2;
		}
		if (hbstop_offset && ww1 > 0) {
			*hbstop_offset += ww1;
		}
	}
}

static int r_shift(int v, int shift)
{
	if (shift >= 0) {
		return v >> shift;
	} else {
		return v << (-shift);
	}
}
static int l_shift(int v, int shift)
{
	if (shift >= 0) {
		return v << shift;
	} else {
		return v >> (-shift);
	}
}

static void fill_border(int total, uae_u32 bgcol)
{
	if (buf2) {
		while (total >= 8) {
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;

			*buf2++ = bgcol;
			*buf2++ = bgcol;
			*buf2++ = bgcol;
			*buf2++ = bgcol;
			*buf2++ = bgcol;
			*buf2++ = bgcol;
			*buf2++ = bgcol;
			*buf2++ = bgcol;

			total -= 8;
		}
		while (total > 0) {
			*buf1++ = bgcol;
			*buf2++ = bgcol;
			total--;
		}
	} else {
		while (total >= 8) {
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			*buf1++ = bgcol;
			total -= 8;
		}
		while (total > 0) {
			*buf1++ = bgcol;
			total--;
		}
	}
}

// draw border from hb to hb
void draw_denise_border_line_fast(int gfx_ypos, enum nln_how how, struct linestate *ls)
{
	if (ls->strlong_seen) {
		set_strlong();
	}

	get_line(gfx_ypos, how);

	if (!buf1 && !ls->blankedline && denise_planes > 0) {
		resolution_count[denise_res]++;
	}
	lines_count++;

	if (!buf1) {
		return;
	}

	if (ls->blankedline) {
		return;
	}

	uae_u32 *buf1p = buf1;
	uae_u32 *buf2p = buf2 != buf1 ? buf2 : NULL;
	uae_u8 *gbufp = gbuf;

	int rshift = RES_MAX - hresolution;

	bool ecsena = ecs_denise && (ls->bplcon0 & 1) != 0;
	bool brdblank = (ls->bplcon3 & 0x20) && ecsena;

	uae_u32 bgcol = brdblank ? 0x000000 : getxcolor(ls->color0);

	int hbstrt_offset = ls->hbstrt_offset >> rshift;
	int hbstop_offset = ls->hbstop_offset >> rshift;

	tvadjust(&hbstrt_offset, &hbstop_offset, ls);

	int draw_end = ls->internal_pixel_cnt >> rshift;
	int draw_startoffset = ls->internal_pixel_start_cnt >> rshift;

	//write_log("%d %08x %03d %03d %03d %03d\n", vpos, ls->color0, hbstrt_offset, hbstop_offset, draw_startoffset, draw_end);

	buf1 = buf1p;
	buf2 = buf2p;

	int start = draw_startoffset;
	if (start < hbstop_offset) {
		int diff = hbstop_offset - start;
		buf1 += diff;
		if (buf2) {
			buf2 += diff;
		}
		if (gbufp) {
			gbufp += diff;
		}
		start = hbstop_offset;
	}
	int end = draw_end > hbstrt_offset ? hbstrt_offset : draw_end;
	int total = end - start;

	fill_border(total, bgcol);

	total = end - start;
	if (need_genlock_data && gbuf && total) {
		int max = addrdiff(xlinebuffer_genlock_end, gbufp);
		total += GENLOCK_EXTRA_CLEAR;
		if (total > max) {
			total = max;
		}
		memset(gbufp, 0, total);
	}

}

void draw_denise_bitplane_line_fast(int gfx_ypos, enum nln_how how, struct linestate *ls)
{
	if (ls->strlong_seen) {
		set_strlong();
	}

	get_line(gfx_ypos, how);
	
	//write_log("* %d %d\n", gfx_ypos, vpos);

	if (!buf1 && !ls->blankedline && denise_planes > 0) {
		resolution_count[denise_res]++;
	}
	lines_count++;


	if (!buf1) {
		return;
	}

	if (ls->blankedline) {
		return;
	}

	uae_u32 *buf1p = buf1;
	uae_u32 *buf2p = buf2 != buf1 ? buf2 : NULL;
	int planecnt = GET_PLANES(ls->bplcon0);
	int res = GET_RES_DENISE(ls->bplcon0);
	bool dpf = (ls->bplcon0 & 0x400) != 0;
	bool ham = (ls->bplcon0 & 0x800) != 0;
	int fmode = 16 << (((ls->fmode & 3) == 3 ? 2 : (ls->fmode & 3)));

	bool ehb = planecnt == 6 && !ham && !dpf && (!ecs_denise || !(ls->bplcon2 & 0x200));
	int mode = CMODE_NORMAL;
	if (ham) {
		mode = CMODE_HAM;
		last_bpl_pix = 0;
		setlasthamcolor();
	} else if (dpf) {
		mode = CMODE_DUALPF;
	} else if (ehb) {
		mode = CMODE_EXTRAHB;
	}
	int ltsidx = mode + 5 * res + 5 * 3 * hresolution;
	if (buf2p) {
		ltsidx += 5 * 3 * 3;
	}

	LINETOSRC_FUNCF ltsf;
	if (aga_mode) {
		if (need_genlock_data) {
			ltsf = linetoscr_aga_genlock_fast_funcs[ltsidx];
			if (currprefs.gfx_lores_mode && linetoscr_aga_genlock_fast_funcs_filtered[ltsidx]) {
				ltsf = linetoscr_aga_genlock_fast_funcs_filtered[ltsidx];
			}
		} else {
			ltsf = linetoscr_aga_fast_funcs[ltsidx];
			if (currprefs.gfx_lores_mode && linetoscr_aga_fast_funcs_filtered[ltsidx]) {
				ltsf = linetoscr_aga_fast_funcs_filtered[ltsidx];
			}
		}
	} else {
		if (need_genlock_data) {
			ltsf = linetoscr_ecs_genlock_fast_funcs[ltsidx];
			if (currprefs.gfx_lores_mode && linetoscr_ecs_genlock_fast_funcs_filtered[ltsidx]) {
				ltsf = linetoscr_ecs_genlock_fast_funcs_filtered[ltsidx];
			}
		} else {
			ltsf = linetoscr_ecs_fast_funcs[ltsidx];
			if (currprefs.gfx_lores_mode && linetoscr_ecs_fast_funcs_filtered[ltsidx]) {
				ltsf = linetoscr_ecs_fast_funcs_filtered[ltsidx];
			}
		}
	}

	uae_u32 *cstart = chunky_out + 1024;
	int len = (ls->bpllen + 3) / 4;
	pfield_doline_8(planecnt, len, (uae_u8*)cstart, ls);

	bool ecsena = ecs_denise && (ls->bplcon0 & 1) != 0;
	bool brdblank = (ls->bplcon3 & 0x20) && ecsena;

	uae_u32 bgcol;
	if (aga_mode) {
		bgcol = brdblank ? 0x000000 : getxcolor(ls->color0);
	} else if (res == 2) {
		bgcol = brdblank ? 0x000000 : bordercolor_ecs_shres;
	} else {
		bgcol = brdblank ? 0x000000 : getxcolor(ls->color0);
	}
	
	//bgcol = 0xff00;

	uae_u8 *cp = (uae_u8*)cstart;
	uae_u8 *cp2 = cp;

	if (!aga_mode) {
		// OCS/ECS PF2 >= 5 and bit in plane 5 set: other planes are ignored in color selection.
		int plf1pri = (ls->bplcon2 >> 0) & 7;
		int plf2pri = (ls->bplcon2 >> 3) & 7;
		if (!ham && !dpf && planecnt >= 5 && plf2pri >= 5) {
			for (int i = 0; i < len * 4 * 8; i++) {
				if (cp[i] & 0x10) {
					cp[i] = 0x10;
				}
			}
		}
		// OCS/ECS DPF feature: if matching plf2pri>=5: value is always 0
		if (dpf && (plf1pri >= 5 || plf2pri >= 5)) {
			for (int i = 0; i < len * 4 * 8; i++) {
				uae_u8 pix = cp[i];
				uae_u8 mask1 = 0x01 | 0x04 | 0x10;
				uae_u8 mask2 = 0x02 | 0x08 | 0x20;
				if (plf1pri >= 5 && (pix & mask1)) {
					pix |= 0x40;
				}
				if (plf2pri >= 5 && (pix & mask2)) {
					pix |= 0x80;
				}
				cp[i] = pix;
			}
		}
	}

	int doubling = hresolution - res;
	int rshift = RES_MAX - hresolution;

	int delay1 = (ls->bplcon1 & 0x0f) | ((ls->bplcon1 & 0x0c00) >> 6);
	int delaymask = (fmode >> res) - 1;
	int delayoffset = ls->fetchmode_size - (((ls->ddfstrt - 0x18) & ls->fetchstart_mask) << 1);
	delay1 += delayoffset;
	delay1 &= delaymask;
	delay1 <<= 2;
	int byteshift = r_shift(delay1, RES_MAX - res);
	cp -= byteshift;
	if (dpf) {
		int delay2 = ((ls->bplcon1 & 0xf0) >> 4) | ((ls->bplcon1 & 0xc000) >> 10);
		delay2 += delayoffset;
		delay2 &= delaymask;
		delay2 <<= 2;
		byteshift = r_shift(delay2, RES_MAX - res);
		cp2 -= byteshift;
		// different bitplane delay in DPF? Merge them.
		if (cp != cp2) {
			uae_u8 *dpout = (uae_u8*)(dpf_chunky_out + 1024);
			for (int i = 0; i < len * 8; i++) {
				uae_u32 pix0 = ((uae_u32*)cp)[i];
				uae_u32 pix1 = ((uae_u32*)cp2)[i];
				uae_u32 c = (pix0 & 0x55555555) | (pix1 & 0xaaaaaaaa);
				((uae_u32*)dpout)[i] = c;
			}
			cp = dpout;
		}
	}

	int hbstrt_offset = ls->hbstrt_offset >> rshift;
	int hbstop_offset = ls->hbstop_offset >> rshift;

	// if HAM: need to clear left hblank after line has been drawn
	tvadjust(&hbstrt_offset, ham ? NULL : &hbstop_offset, ls);

	// negative checks are needed to handle always-on HDIW
	int hstop_offset_adjusted = ls->hstop_offset;
	if (ls->bpl1dat_trigger_offset >= 0) {
		int bpl_end = ls->bpl1dat_trigger_offset + (1 << RES_MAX) + ((ls->bpllen * 32 + byteshift * 4) >> denise_res);
		if (hstop_offset_adjusted < 0 || hstop_offset_adjusted > bpl_end) {
			hstop_offset_adjusted = bpl_end;
		}
	}
	int hstrt_offset = ls->hstrt_offset < 0 || ls->hstop_offset < 0 || (ls->hstrt_offset >> rshift) >= hbstrt_offset ? hbstop_offset : ls->hstrt_offset >> rshift;
	int hstop_offset = hstop_offset_adjusted < 0 || (hstop_offset_adjusted >> rshift) >= hbstrt_offset ? hbstrt_offset : hstop_offset_adjusted >> rshift;
	int bpl1dat_trigger_offset = (ls->bpl1dat_trigger_offset + (1 << RES_MAX)) >> rshift;
	int draw_start = 0;
	int draw_end = ls->internal_pixel_cnt >> rshift;
	int draw_startoffset = ls->internal_pixel_start_cnt >> rshift;


	//write_log("%03d %03d %03d %03d %03d %03d %03d\n", vpos, hbstop_offset, hbstrt_offset, hstrt_offset, hstop_offset, bpl1dat_trigger_offset, delayoffset);

	uae_u8 bxor = ls->bplcon4 >> 8;
	buf1 = buf1p;
	buf2 = buf2p;

	int cpadd = doubling < 0 ? (doubling < -1 ? 2 : 1) : 0;
	int bufadd = doubling > 0 ? (doubling > 1 ? 2 : 1) : 0;

	// subpixel handling
	int subpix = (ls->bplcon1 & 0x0300) >> 8;
	int cpadds[4] = { 0, 0, 0, 0 };
	if (doubling < 0) {
		cpadds[0] = 1 << cpadd;
		subpix = 0;
	} else if (doubling == 0) {
		cpadds[0] = 1 << cpadd;
		subpix >>= RES_MAX - hresolution;
		cp -= subpix;
		cp2 -= subpix;
	} else if (doubling == 1) {
		if (subpix & 2) {
			cp--;
			cpadds[1] = 1 << cpadd;
		} else {
			cpadds[1] = 1 << cpadd;
		}
		if (subpix & 1) {
			cpadds[0] = 1 << cpadd;
			cpadds[1] = 0;
		}
	} else {
		if (subpix & 2) {
			cp--;
			cpadds[3] = 1 << cpadd;
		} else {
			cpadds[3] = 1 << cpadd;
		}
		if (subpix & 1) {
			cpadds[1] = 1 << cpadd;
			cpadds[3] = 0;
		}
	}

#if 1
	
	ltsf(draw_start, draw_end, draw_startoffset, hbstrt_offset, hbstop_offset, hstrt_offset, hstop_offset, bpl1dat_trigger_offset,
		planecnt, bgcol, cp, cp2, cpadd, cpadds, bufadd, ls);

#if 0
	*buf1++ = 0;
	*buf1++ = 0;
	*buf2++ = 0;
	*buf2++ = 0;
#endif

	if (!programmedmode && ham) {
		int ww1 = visible_left_start > visible_left_border ? (visible_left_start - visible_left_border) << 0 : 0;
		if (ww1 > 0) {
			hbstop_offset += ww1;
			int w = hbstop_offset - draw_startoffset - (1 << hresolution);
			if (w > 0) {
				memset(buf1p, DEBUG_TVOVERSCAN_H_GRAYSCALE, w * sizeof(uae_u32));
				if (buf2p) {
					memset(buf2p, DEBUG_TVOVERSCAN_H_GRAYSCALE, w * sizeof(uae_u32));
				}
			}
		}
	}

	// clear some more bytes to clear possible lightpen cursor graphics
	if (need_genlock_data && gbuf) {
		int max = addrdiff(xlinebuffer_genlock_end, gbuf);
		int total = GENLOCK_EXTRA_CLEAR;
		if (total > max) {
			total = max;
		}
		memset(gbuf, 0, total);
	}

#else

	int cnt = draw_start;

	if (hstrt_offset > bpl1dat_trigger_offset) {
		int d = hstrt_offset - bpl1dat_trigger_offset;
		cp += cpadd * d;
	}
	cnt = draw_startoffset;
	if (cnt < hbstop_offset) {
		int d = hbstop_offset - cnt;
		buf1 += bufadd * d;
		buf2 += bufadd * d;
		cnt = hbstop_offset;
	}
	if (draw_end > hbstrt_offset) {
		draw_end = hbstrt_offset;
	}

	while (cnt < draw_end) {
		if (cnt < bpl1dat_trigger_offset || cnt < hstrt_offset || cnt >= hstop_offset) {
			*buf1++ = bgcol;
			*buf2++ = bgcol;
		}
		if (cnt >= bpl1dat_trigger_offset && cnt >= hstrt_offset && cnt < hstop_offset) {
			uae_u8 c;
			uae_u32 col;
			c = *cp;
			cp += cpadds[0];
			c ^= bxor;
			col = colors_aga[c];
			*buf1++ = col;
			*buf2++ = col;
		}
		cnt += bufadd;
	}
#endif

}

#define RB restore_u8()
#define SRB restore_s8()
#define RW restore_u16()
#define RL restore_u32()
#define RQ restore_u64()
#define SB save_u8
#define SW save_u16
#define SL save_u32
#define SQ save_u64

uae_u16 save_custom_bpl_dat(int num)
{
	return bplxdat[num];
}
void restore_custom_bpl_dat(int num, uae_u16 dat)
{
	bplxdat[num] = dat;
}

uae_u8 *save_custom_bpl(size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 8 * 64);

	SL(1);
	for (int i = 0; i < 8; i++) {
		SL(bplxdat[i]);
		SL(bplxdat2[i]);
		SL(bplxdat3[i]);
		SQ(bplxdat_64[i]);
		SQ(bplxdat2_64[i]);
		SQ(bplxdat3_64[i]);
	}

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_custom_bpl(uae_u8 *src)
{
	uae_u32 v = RL;

	for (int i = 0; i < 8; i++) {
		bplxdat[i] = RL;
		bplxdat2[i] = RL;
		bplxdat3[i] = RL;
		bplxdat_64[i] = RQ;
		bplxdat2_64[i] = RQ;
		bplxdat3_64[i] = RQ;
	}

	return src;
}

uae_u8 *save_custom_sprite_denise(int num, uae_u8 *dst)
{
	struct denise_spr *s = &dspr[num];

	if (denise_sprfmode64) {
		SW(s->dataa64 >> 48);
		SW(s->datab64 >> 48);
		SW((uae_u16)(s->dataa64 >> 32));
		SW((uae_u16)(s->datab64 >> 32));
		SW((uae_u16)(s->dataa64 >> 16));
		SW((uae_u16)(s->datab64 >> 16));
		SW((uae_u16)(s->dataa64 >>  0));
		SW((uae_u16)(s->datab64 >>  0));
	} else {
		SW(s->dataa >> 16);
		SW(s->datab >> 16);
		SW(0);
		SW(0);
		SW(0);
		SW(0);
		SW(0);
		SW(0);
	}
	uae_u8 f = (s->armed ? 1 : 0) | (s->fmode << 1);
	SB(f);

	return dst;
}

uae_u8 *restore_custom_sprite_denise(int num, uae_u8 *src, uae_u16 pos, uae_u16 ctl)
{
	struct denise_spr *s = &dspr[num];
	uae_u16 data[4], datb[4];

	sprwrite(num * 8 + 0, pos);
	sprwrite(num * 8 + 2, ctl);

	data[0] = RW;
	datb[0] = RW;
	data[1] = RW;
	datb[1] = RW;
	data[2] = RW;
	datb[2] = RW;
	data[3] = RW;
	datb[3] = RW;

	uae_u8 f = RB;
	int armed = f & 1;
	s->fmode = (f >> 1) & 3;
	s->dataa = data[0] << 16;
	s->datab = datb[0] << 16;
	s->dataa64 = (data[0] << 16) | (data[1]);
	s->datab64 = (datb[0] << 16) | (datb[1]);
	spr_arm(s, armed);

	if (denise_sprfmode64) {
		s->dataa64 <<= 32;
		s->datab64 <<= 32;
		s->dataa64 |= (data[2] << 16) | (data[3]);
		s->datab64 |= (datb[2] << 16) | (datb[3]);
	}

	return src;
}

static void updatelinedata(void)
{
	this_line = &temp_line;
	this_line->vpos = vpos;
	this_line->lof = lof_store;
	this_line->linear_vpos = linear_vpos;
}

static bool waitqueue_nolock(void)
{
	while (((rga_queue_write + 1) & DENISE_RGA_SLOT_CHUNKS_MASK) == (rga_queue_read & DENISE_RGA_SLOT_CHUNKS_MASK)) {
		uae_sem_trywait_delay(&read_sem, 500);
	}
	return true;
}
static bool waitqueue(int id)
{
	if (quit_program) {
		return false;
	}
	if (!thread_debug_lock) {
		write_log("Denise queue without lock! id=%d\n", id);
		return false;
	}
	waitqueue_nolock();
	return true;
}
static void addtowritequeue(void)
{
	atomic_inc(&rga_queue_write);

	uae_sem_post(&write_sem);
}

void draw_denise_border_line_fast_queue(int gfx_ypos, enum nln_how how, struct linestate *ls)
{
	if (MULTITHREADED_DENISE) {
		
		if (!waitqueue(2)) {
			return;
		}

		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->gfx_ypos = gfx_ypos;
		q->how = how;
		q->ls = ls;
		q->type = 2;
		q->vpos = vpos;
		q->linear_vpos = linear_vpos;

		addtowritequeue();

	} else {
	
		updatelinedata();
		draw_denise_border_line_fast(gfx_ypos, how, ls);
	
	}
}

void draw_denise_bitplane_line_fast_queue(int gfx_ypos, enum nln_how how, struct linestate *ls)
{
	if (MULTITHREADED_DENISE) {
		
		if (!waitqueue(1)) {
			return;
		}

		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->gfx_ypos = gfx_ypos;
		q->how = how;
		q->ls = ls;
		q->type = 1;
		q->vpos = vpos;
		q->linear_vpos = linear_vpos;

		addtowritequeue();

	} else {
	
		updatelinedata();
		draw_denise_bitplane_line_fast(gfx_ypos, how, ls);
	
	}
}

void quick_denise_rga_queue(uae_u32 linecnt, int startpos, int endpos)
{
	if (MULTITHREADED_DENISE) {

		if (!waitqueue(3)) {
			return;
		}

		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->linecnt = linecnt;
		q->startpos = startpos;
		q->endpos = endpos;
		q->type = 3;
		q->vpos = vpos;
		q->linear_vpos = linear_vpos;

		addtowritequeue();
	
	} else {

		updatelinedata();
		quick_denise_rga(linecnt, startpos, endpos);
	
	}
}

void denise_handle_quick_strobe_queue(uae_u16 strobe, int strobe_pos, int endpos)
{
	if (MULTITHREADED_DENISE) {

		if (!waitqueue_nolock()) {
			return;
		}

		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->strobe = strobe;
		q->strobe_pos = strobe_pos;
		q->endpos = endpos;
		q->type = 4;
		q->vpos = vpos;
		q->linear_vpos = linear_vpos;

		addtowritequeue();

	} else {

		updatelinedata();
		denise_handle_quick_strobe(strobe, strobe_pos, endpos);
		update_overlapped_cycles(endpos);

	}
}

void draw_denise_line_queue(int gfx_ypos, nln_how how, uae_u32 linecnt, int startpos, int endpos, int startcycle, int endcycle, int skip, int skip2, int dtotal, int calib_start, int calib_len, bool lof, bool lol, int hdelay, bool blanked, bool finalseg, struct linestate *ls)
{
	if (MULTITHREADED_DENISE) {

		if (!waitqueue(0)) {
			return;
		}

		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->type = 0;
		q->gfx_ypos = gfx_ypos;
		q->how = how;
		q->linecnt = linecnt;
		q->startpos = startpos;
		q->endpos = endpos;
		q->startcycle = startcycle;
		q->endcycle = endcycle;
		q->skip = skip;
		q->skip2 = skip2;
		q->dtotal = dtotal;
		q->calib_start = calib_start;
		q->calib_len = calib_len;
		q->lof = lof;
		q->lol = lol;
		q->ls = ls;
		q->vpos = vpos;
		q->hdelay = hdelay;
		q->blanked = blanked;
		q->finalseg = finalseg;
		q->linear_vpos = linear_vpos;

		addtowritequeue();

	} else {

		updatelinedata();
		draw_denise_line(gfx_ypos, how, linecnt, startpos, startcycle, endcycle, skip, skip2, dtotal, calib_start, calib_len, lol, hdelay, blanked, finalseg, ls);
		if (finalseg) {
			update_overlapped_cycles(endpos);
		}

	}
}

void draw_denise_vsync_queue(int erase)
{
	if (MULTITHREADED_DENISE) {

		if (!waitqueue_nolock()) {
			return;
		}
		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->type = 5;
		q->erase = erase;
		q->vpos = vpos;
		q->linear_vpos = linear_vpos;

		addtowritequeue();

	} else {
	
		updatelinedata();
		draw_denise_vsync(erase);

	}
}

void denise_update_reg_queue(uae_u16 reg, uae_u16 v, uae_u32 linecnt)
{
	if (MULTITHREADED_DENISE) {

		if (!waitqueue_nolock()) {
			return;
		}
		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->type = 6;
		q->vpos = vpos;
		q->linear_vpos = linear_vpos;
		q->reg = reg;
		q->val = v;
		q->linecnt = linecnt;

		addtowritequeue();

	} else {

		updatelinedata();
		denise_update_reg(reg, v, linecnt);

	}
}

void denise_store_restore_registers_queue(bool store, uae_u32 linecnt)
{
	if (MULTITHREADED_DENISE) {

		if (!waitqueue(7)) {
			return;
		}
		struct denise_rga_queue *q = &rga_queue[rga_queue_write & DENISE_RGA_SLOT_CHUNKS_MASK];
		q->type = 7;
		q->vpos = vpos;
		q->linear_vpos = linear_vpos;
		q->val = store ? 1 : 0;
		q->linecnt = linecnt;

		addtowritequeue();

	} else {

		updatelinedata();
		if (store) {
			denise_store_registers();
		} else {
			denise_restore_registers();
		}
	}
}

void draw_denise_line_queue_flush(void)
{
	if (MULTITHREADED_DENISE) {

		for (;;) {
			if (rga_queue_read == rga_queue_write) {
				return;
			}
			uae_sem_trywait_delay(&read_sem, 500);
		}

	}
}
