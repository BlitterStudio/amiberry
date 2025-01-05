
/*
* UAE - The Un*x Amiga Emulator
*
* Screen drawing functions
*
* Copyright 1995-2000 Bernd Schmidt
* Copyright 1995 Alessandro Bissacco
* Copyright 2000-2008 Toni Wilen
*/

/* There are a couple of concepts of "coordinates" in this file.
- DIW coordinates
- DDF coordinates (essentially cycles, resolution lower than lores by a factor of 2)
- Pixel coordinates
* in the Amiga's resolution as determined by BPLCON0 ("Amiga coordinates")
* in the window resolution as determined by the preferences ("window coordinates").
* in the window resolution, and with the origin being the topmost left corner of
the window ("native coordinates")
One note about window coordinates.  The visible area depends on the width of the
window, and the centering code.  The first visible horizontal window coordinate is
often _not_ 0, but the value of VISIBLE_LEFT_BORDER instead.

One important thing to remember: DIW coordinates are in the lowest possible
resolution.

To prevent extremely bad things (think pixels cut in half by window borders) from
happening, all ports should restrict window widths to be multiples of 16 pixels.  */

#define SPRITE_DEBUG_HIDE 0
#define BG_COLOR_DEBUG 0
#define EXTBORDER_BLANK 0

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
#include "newcpu.h"
#include "xwin.h"
#include "autoconf.h"
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

//#define XLINECHECK

struct amigadisplay adisplays[MAX_AMIGADISPLAYS];

typedef enum
{
	CMODE_NORMAL,
	CMODE_DUALPF,
	CMODE_EXTRAHB,
	CMODE_EXTRAHB_ECS_KILLEHB,
	CMODE_HAM
} CMODE_T;

#ifdef AMIBERRY
#define RENDER_SIGNAL_PARTIAL 1
#define RENDER_SIGNAL_FRAME_DONE 2
#define RENDER_SIGNAL_QUIT 3
static uae_thread_id drawing_tid = nullptr;
static smp_comm_pipe *volatile drawing_pipe = nullptr;
static uae_sem_t drawing_sem = nullptr;
static bool volatile drawing_thread_busy = false;
#endif

extern int sprite_buffer_res;
static int lores_factor;
int lores_shift, shres_shift;

static void pfield_set_linetoscr(void);

int debug_bpl_mask = 0xff, debug_bpl_mask_one;

static struct decision *dp_for_drawing;
static struct draw_info *dip_for_drawing;

static void lores_set(int lores)
{
	int old = lores_shift;
	lores_shift = lores;
	if (lores_shift != old || shres_shift != RES_MAX - lores) {
		shres_shift = RES_MAX - lores;
		pfield_set_linetoscr();
	}
}

static void lores_reset (void)
{
	lores_factor = currprefs.gfx_resolution ? 2 : 1;
	lores_set(currprefs.gfx_resolution);
	if (doublescan > 0) {
		int ls = lores_shift;
		if (ls < 2) {
			ls++;
		}
		lores_factor = 2;
		lores_set(ls);
	}
	sprite_buffer_res = currprefs.gfx_resolution;
	if (doublescan > 0 && sprite_buffer_res < RES_SUPERHIRES)
		sprite_buffer_res++;
}

/* mirror of chipset_mask */
bool ecs_agnus;
bool ecs_denise;
bool aga_mode;
bool agnusa1000;
bool denisea1000_noehb;
bool denisea1000;

bool direct_rgb;

/* The shift factor to apply when converting between Amiga coordinates and window
coordinates.  Zero if the resolution is the same, positive if window coordinates
have a higher resolution (i.e. we're stretching the image), negative if window
coordinates have a lower resolution (i.e. we're shrinking the image).  */
static int res_shift;

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

static uae_u32 clxtab[256];

/* Video buffer description structure. Filled in by the graphics system
* dependent code. */

/* OCS/ECS color lookup table. */
xcolnr xcolors[4096];

struct spritepixelsbuf {
	uae_u8 flags;
	uae_u8 stdata;
	uae_u16 stfmdata;
	uae_u16 data;
};
static struct spritepixelsbuf spritepixels_buffer[MAX_PIXELS_PER_LINE];
static struct spritepixelsbuf *spritepixels;
static int sprite_first_x, sprite_last_x;
static bool sprite_visibility;

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

struct color_entry colors_for_drawing;
xcolnr fullblack;
static struct color_entry direct_colors_for_drawing;

static xcolnr *p_acolors;
static xcolnr *p_xcolors;

/* The size of these arrays is pretty arbitrary; it was chosen to be "more
than enough".  The coordinates used for indexing into these arrays are
almost, but not quite, Amiga coordinates (there's a constant offset).  */
static union {
	uae_u64 apixels_q[MAX_PIXELS_PER_LINE * 2 / sizeof(uae_u64)];
	uae_u32 apixels_l[MAX_PIXELS_PER_LINE * 2 / sizeof(uae_u32)];
	uae_u8  apixels[MAX_PIXELS_PER_LINE * 2];
} pixdata;

static uae_u8 *refresh_indicator_buffer;
static uae_u8 *refresh_indicator_changed, *refresh_indicator_changed_prev;
static int refresh_indicator_height;

#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
uae_u16 *spixels;
#else
uae_u16 spixels[2 * MAX_SPR_PIXELS];
#endif

struct sprite_stb spixstate;

static uae_u32 ham_linebuf[MAX_PIXELS_PER_LINE * 2];

static uae_u8 all_ones[MAX_PIXELS_PER_LINE];
static uae_u8 all_zeros[MAX_PIXELS_PER_LINE];

uae_u8 *xlinebuffer, *xlinebuffer_genlock;

static int *amiga2aspect_line_map, *native2amiga_line_map;
static int native2amiga_line_map_height;
static uae_u8 **row_map;
static uae_u8 *row_map_genlock_buffer;
static uae_u8 row_tmp[MAX_PIXELS_PER_LINE * 32 / 8];
static int max_drawn_amiga_line;
uae_u8 **row_map_genlock;
uae_u8 *row_map_color_burst_buffer;

/* line_draw_funcs: pfield_do_linetoscr, pfield_do_fill_line, decode_ham */
typedef void (*line_draw_func)(int, int, int);

#define LINE_UNDECIDED 1
#define LINE_DECIDED 2
#define LINE_DECIDED_DOUBLE 3
#define LINE_AS_PREVIOUS 4
#define LINE_BLACK 5
#define LINE_REMEMBERED_AS_BLACK 6
#define LINE_DONE 7
#define LINE_DONE_AS_PREVIOUS 8
#define LINE_REMEMBERED_AS_PREVIOUS 9

#define LINESTATE_SIZE ((MAXVPOS + MAXVPOS_WRAPLINES) * 2 + 1)

#ifdef AMIBERRY
static int linestate_first_undecided = 0;
#endif
static uae_u8 linestate[LINESTATE_SIZE];

uae_u8 line_data[(MAXVPOS + MAXVPOS_WRAPLINES) * 2][MAX_PLANES * MAX_WORDS_PER_LINE * 2];

/* Centering variables.  */
static int min_diwstart, max_diwstop;
/* The visible window: VISIBLE_LEFT_BORDER contains the left border of the visible
area, VISIBLE_RIGHT_BORDER the right border.  These are in window coordinates.  */
int visible_left_border, visible_right_border;
/* Pixels outside of visible_start and visible_stop are always black */
static int visible_left_start, visible_right_stop;
static int visible_top_start, visible_bottom_stop;
/* same for blank */
static int vblank_top_start, vblank_bottom_stop;
static int hblank_left_start, hblank_right_stop;
static int hblank_left_start_hard, hblank_right_stop_hard;
static bool extborder, exthblanken, exthblankon;
static int exthblank;
static bool exthblank_force;
static int exthblank_set;
static bool ehb_enable;
static bool syncdebug;

static int linetoscr_x_adjust_pixbytes, linetoscr_x_adjust_pixels;
static int thisframe_y_adjust;
static int thisframe_y_adjust_real, min_ypos_for_screen;
static int max_ypos_thisframe1;
int thisframe_first_drawn_line, thisframe_last_drawn_line;

/* A frame counter that forces a redraw after at least one skipped frame in
interlace mode.  */
static int last_redraw_point;

#define MAX_STOP 30000
static int first_drawn_line, last_drawn_line;

/* These are generated by the drawing code from the line_decisions array for
each line that needs to be drawn.  These are basically extracted out of
bit fields in the hardware registers.  */
static int bplmode, bplehb, bplham, bpldualpf, bpldualpfpri;
static int bpldualpf2of, bplplanecnt, bplmaxplanecnt, ecsshres;
static int bplbypass, bplcolorburst, bplcolorburst_field;
static int bplres;
static int plf1pri, plf2pri, bplxor, bplxorsp, bpland, bpldelay_sh;
static uae_u32 plf_sprite_mask;
static int sbasecol[2] = { 16, 16 };
static int hposblank;
static bool ecs_genlock_features_active;
static uae_u8 ecs_genlock_features_mask;
static bool ecs_genlock_features_colorkey;
static bool aga_genlock_features_zdclken;
static bool sprite_smaller_than_64, sprite_smaller_than_64_inuse;
static bool full_blank;
static bool hsync_debug, vsync_debug, hblank_debug, vblank_debug;
static int hcenter_debug;
static uae_u8 vb_state;

uae_sem_t gui_sem;

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

#ifdef XLINECHECK
static void xlinecheck (unsigned int start, unsigned int end)
{
	unsigned int xstart = (unsigned int)xlinebuffer + start * vidinfo->drawbuffer.pixbytes;
	unsigned int xend = (unsigned int)xlinebuffer + end * vidinfo->drawbuffer.pixbytes;
	unsigned int end1 = (unsigned int)vidinfo->drawbuffer.bufmem + vidinfo->drawbuffer.rowbytes * vidinfo->drawbuffer.height;
	int min = linetoscr_x_adjust_bytes / vidinfo->drawbuffer.pixbytes;
	int ok = 1;

	if (xstart >= vidinfo->drawbuffer.emergmem && xstart < vidinfo->drawbuffer.emergmem + 4096 * vidinfo->drawbuffer.pixbytes &&
		xend >= vidinfo->drawbuffer.emergmem && xend < vidinfo->drawbuffer.emergmem + 4096 * vidinfo->drawbuffer.pixbytes)
		return;

	if (xstart < (unsigned int)vidinfo->drawbuffer.bufmem || xend < (unsigned int)vidinfo->drawbuffer.bufmem)
		ok = 0;
	if (xend > end1 || xstart >= end1)
		ok = 0;
	xstart -= (unsigned int)vidinfo->drawbuffer.bufmem;
	xend -= (unsigned int)vidinfo->drawbuffer.bufmem;
	if ((xstart % vidinfo->drawbuffer.rowbytes) >= vidinfo->drawbuffer.width * vidinfo->drawbuffer.pixbytes)
		ok = 0;
	if ((xend % vidinfo->drawbuffer.rowbytes) >= vidinfo->drawbuffer.width * vidinfo->drawbuffer.pixbytes)
		ok = 0;
	if (xstart >= xend)
		ok = 0;
	if (xend - xstart > vidinfo->drawbuffer.width * vidinfo->drawbuffer.pixbytes)
		ok = 0;

	if (!ok) {
		write_log (_T("*** %d-%d (%dx%dx%d %d) %p\n"),
			start - min, end - min, vidinfo->drawbuffer.width, vidinfo->drawbuffer.height,
			vidinfo->drawbuffer.pixbytes, vidinfo->drawbuffer.rowbytes,
			xlinebuffer);
	}
}
#else
#define xlinecheck(start, end)
#endif

static void clearbuffer (struct vidbuffer *dst)
{
	if (!dst->bufmem_allocated)
		return;
	uae_u8 *p = dst->bufmem_allocated;
	for (int y = 0; y < dst->height_allocated; y++) {
		memset (p, 0, dst->width_allocated * dst->pixbytes);
		p += dst->rowbytes;
	}
}

static void reset_decision_table (void)
{
	for (unsigned char& i : linestate)
	{
		i = LINE_UNDECIDED;
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

STATIC_INLINE int xshift (int x, int shift)
{
	if (shift < 0)
		return x >> (-shift);
	else
		return x << shift;
}

int coord_native_to_amiga_x (int x)
{
	x += visible_left_border;
	x = xshift (x, 1 - lores_shift);
	return x + 2 * DISPLAY_LEFT_SHIFT - 2 * DIW_DDF_OFFSET;
}

#ifdef AMIBERRY
int coord_native_to_amiga_y (int y)
{
	if (!native2amiga_line_map || y < 0)
		return -1;
	if (y >= native2amiga_line_map_height) 
		return native2amiga_line_map[native2amiga_line_map_height] + thisframe_y_adjust - minfirstline;
	return native2amiga_line_map[y] + thisframe_y_adjust - minfirstline;
}
#else
int coord_native_to_amiga_y(int y)
{
	if (!native2amiga_line_map || y < 0 || y >= native2amiga_line_map_height)
		return -1;
	return native2amiga_line_map[y] + thisframe_y_adjust - minfirstline;
}
#endif

STATIC_INLINE int res_shift_from_window (int x)
{
	if (res_shift >= 0)
		return x >> res_shift;
	return x << -res_shift;
}

STATIC_INLINE int res_shift_from_amiga (int x)
{
	if (res_shift >= 0)
		return x >> res_shift;
	return x << -res_shift;
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
	if (vidinfo->outbuffer == &vidinfo->drawbuffer)
		return true;
	return vidinfo->outbuffer->nativepositioning;
}

extern int plffirstline_total, plflastline_total;
extern int first_planes_vpos, last_planes_vpos;
extern int diwfirstword_total, diwlastword_total;
extern int ddffirstword_total, ddflastword_total;
extern bool vertical_changed, horizontal_changed;
extern int firstword_bplcon1;
extern int lof_display;

#define MIN_DISPLAY_W 256
#define MIN_DISPLAY_H 192
#define MAX_DISPLAY_W 362
#define MAX_DISPLAY_H 283

static int gclow, gcloh, gclox, gcloy, gclorealh;
static int stored_left_start, stored_top_start, stored_width, stored_height;

void get_custom_topedge (int *xp, int *yp, bool max)
{
	if (isnativevidbuf(0) && !max) {
		int x, y;
		x = visible_left_border + (DISPLAY_LEFT_SHIFT << currprefs.gfx_resolution);
		y = minfirstline << currprefs.gfx_vresolution;
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

void get_screen_blanking_limits(int *hbstop, int *hbstrt, int *vbstop, int *vbstrt)
{
	*vbstop = vblank_bottom_stop;
	*vbstrt = vblank_top_start;

	int hblank_left = exthblank ? hblank_left_start : hblank_left_start_hard;
	int hblank_right = exthblank ? hblank_right_stop : hblank_right_stop_hard;

	*hbstop = hblank_left - visible_left_border;
	*hbstrt = hblank_right - visible_left_border;
}

static void reset_custom_limits(void)
{
	gclow = gcloh = gclox = gcloy = 0;
	gclorealh = -1;
	center_reset = 1;
}

static void expand_vb_state(void)
{
	full_blank = vb_state == VB_PRGVB || (vb_state >= VB_XBLANK && vb_state < VB_XBORDER) || (vb_state == VB_XBORDER);
	vsync_debug = (vb_state & VB_VS) != 0;
	vblank_debug = (vb_state & VB_VB) != 0;
}

static void extblankcheck(void)
{
	exthblankon = dp_for_drawing && (dp_for_drawing->bplcon3 & 1) && (dp_for_drawing->bplcon0 & 1);
	if (exthblanken && exthblankon) {
		exthblank = exthblank_set;
	}
	if (exthblanken && !exthblankon) {
		exthblank = 0;
	}
}

static void reset_hblanking_limits(void)
{
	hblank_left_start = visible_left_start;
	hblank_right_stop = visible_right_stop;

	hblank_left_start = std::max(hblank_left_start, visible_left_border);
	hblank_right_stop = std::min(hblank_right_stop, visible_right_border);
}

static void get_vblanking_limits(int *vbstrtp, int *vbstopp, bool overscanonly)
{
	int vbstrt = vblank_firstline_hw;
	if (!ecs_denise) {
		vbstrt--;
	}
	int vbstop = maxvpos + lof_display;
	if (!ecs_denise && !ecs_agnus) {
		if (currprefs.gfx_overscanmode >= OVERSCANMODE_BROADCAST) {
			vbstop++;
		}
	} else if (ecs_agnus && !ecs_denise) {
		// hide hblank bug by faking vblank start 1 line earlier
		if (currprefs.gfx_overscanmode < OVERSCANMODE_BROADCAST) {
			vbstrt++;
			vbstop--;
		}
	}
	if (currprefs.gfx_overscanmode < OVERSCANMODE_OVERSCAN && !overscanonly) {
		int mult = (OVERSCANMODE_OVERSCAN - currprefs.gfx_overscanmode) * 5;
		vbstrt += mult;
		vbstop -= mult;
	}
	vbstrt <<= currprefs.gfx_vresolution;
	vbstop <<= currprefs.gfx_vresolution;
	vblank_top_start = std::max(vblank_top_start, vbstrt);
	vblank_bottom_stop = std::min(vblank_bottom_stop, vbstop);
	*vbstrtp = vbstrt;
	*vbstopp = vbstop;
}

// this handles hardwired vblank
// vb_state in do_color_changes() handles programmed vblank
static void set_vblanking_limits(void)
{
	vblank_top_start = visible_top_start;
	vblank_bottom_stop = visible_bottom_stop;

	vblank_top_start = std::max(vblank_top_start, visible_top_start);
	vblank_bottom_stop = std::min(vblank_bottom_stop, visible_bottom_stop);

	if (syncdebug) {
		return;
	}

	bool hardwired = true;
	if (ecs_agnus) {
		hardwired = (new_beamcon0 & BEAMCON0_VARVBEN) == 0;
		// ECS Denise with exthblank: always use thisline_decision.vb blanking method
		if (ecs_denise && !aga_mode && exthblank) {
			hardwired = false;
		}
	}
	if (hardwired) {
		int vbstrt, vbstop;
		get_vblanking_limits(&vbstrt, &vbstop, false);
		vblank_top_start = std::max(vblank_top_start, vbstrt);
		vblank_bottom_stop = std::min(vblank_bottom_stop, vbstop);
	}
}

int get_vertical_visible_height(bool useoldsize)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int h = vidinfo->drawbuffer.inheight;
	int vbstrt, vbstop;

	if (programmedmode <= 1) {
		h = maxvsize_display;
		if (useoldsize) {
			// 288/576 or 243/486
			if (h == 288 || h == 243) {
				h--;
			}
		}
		h <<= currprefs.gfx_vresolution;
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
			get_vblanking_limits(&vbstrt, &vbstop, true);
			int hh = vbstop - vbstrt;
			if (interlace_seen && lof_display) {
				hh -= 1 << currprefs.gfx_vresolution;
			}
			h = std::min(h, hh);
		}
	}
	return h;
}

static void set_hblanking_limits(void)
{
	if (syncdebug) {
		hblank_left_start_hard = visible_left_start;
		hblank_right_stop_hard = visible_right_stop;
		return;
	}

	hblank_left_start_hard = hblank_left_start;
	hblank_right_stop_hard = hblank_right_stop;

	// horizontal blanking
	bool hardwired = !dp_for_drawing || !ce_is_extblankset(colors_for_drawing.extra);
	bool doblank = false;
	int hbstrt = ((maxhpos_short + 8) << CCK_SHRES_SHIFT) - 3;
	if (!ecs_denise) {
		hbstrt -= 4;
	}
	int hbstop = (47 << CCK_SHRES_SHIFT) - 7;

	if (currprefs.gfx_overscanmode < OVERSCANMODE_OVERSCAN) {
		int mult = (OVERSCANMODE_OVERSCAN - currprefs.gfx_overscanmode) * 4;
		hbstrt -= mult << CCK_SHRES_SHIFT;
		hbstop += mult << CCK_SHRES_SHIFT;
		if (currprefs.gfx_overscanmode == 0) {
			hbstrt += 2 << CCK_SHRES_SHIFT;
			hbstop -= 2 << CCK_SHRES_SHIFT;
		}
	}

	// programmed mode with hardwired hblanking
	hblank_left_start_hard = std::max(hblank_left_start_hard, coord_hw_to_window_x_shres(hbstop));
	hblank_right_stop_hard = std::min(hblank_right_stop_hard, coord_hw_to_window_x_shres(hbstrt));

	if (hardwired) {
		doblank = true;
	} else if (currprefs.gfx_overscanmode <= OVERSCANMODE_OVERSCAN) {
		doblank = true;
	} else if (currprefs.gfx_overscanmode == OVERSCANMODE_BROADCAST) {
		hbstrt = ((maxhpos_short + 12) << CCK_SHRES_SHIFT) - 3;
		doblank = true;
	} else if (currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA) {
		doblank = true;
	}

	if (doblank && programmedmode != 1) {
		// programmed mode with programmed hblanking
		// reposition to sync
		// use hardwired hblank emulation as overscan blanking.
		if ((new_beamcon0 & bemcon0_hsync_mask) && !hardwired) {
			extern uae_u16 hsstrt;
			hbstrt += (hsstrt - 18) << CCK_SHRES_SHIFT;
			hbstop += (hsstrt - 18) << CCK_SHRES_SHIFT;
		}

		if (currprefs.chipset_hr) {
			hbstrt &= ~(3 >> currprefs.gfx_resolution);
			hbstop &= ~(3 >> currprefs.gfx_resolution);
		} else {
			hbstrt &= ~3;
			hbstop &= ~3;
		}
		hblank_left_start = std::max(hblank_left_start, coord_hw_to_window_x_shres(hbstop));
		hblank_right_stop = std::min(hblank_right_stop, coord_hw_to_window_x_shres(hbstrt));
	}
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

	// backwards compatibility, old 0x38 start is gone.
	if (left > 0) {
		left += (0x38 * 4) >> (RES_MAX - currprefs.gfx_resolution);
		right += (0x38 * 4) >> (RES_MAX - currprefs.gfx_resolution);
	}

	visible_left_start = std::max(left, visible_left_start);
	if (right > left && right < visible_right_stop)
		visible_right_stop = right;

	visible_top_start = std::max(top, visible_top_start);
	if (bottom > top && bottom < visible_bottom_stop)
		visible_bottom_stop = bottom;
}

void set_custom_limits (int w, int h, int dx, int dy, bool blank)
{
	struct amigadisplay *ad = &adisplays[0];
	struct gfx_filterdata *fd = &currprefs.gf[ad->gf_index];
	int vls = visible_left_start;
	int vrs = visible_right_stop;
	int vts = visible_top_start;
	int vbs = visible_bottom_stop;

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

	if (w <= 0 || dx < 0) {
		visible_left_start = 0;
		visible_right_stop = MAX_STOP;
	} else {
		visible_left_start = visible_left_border + dx;
		visible_right_stop = visible_left_start + w;
	}
	if (h <= 0 || dy < 0) {
		visible_top_start = 0;
		visible_bottom_stop = MAX_STOP;
	} else {
		visible_top_start = min_ypos_for_screen + dy;
		visible_bottom_stop = visible_top_start + h;
	}

	if ((w >= 0 && h >= 0) &&
		(vls != visible_left_start || vrs != visible_right_stop ||
		vts != visible_top_start || vbs != visible_bottom_stop))
		notice_screen_contents_lost(0);

	check_custom_limits();
}

void store_custom_limits (int w, int h, int x, int y)
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

int get_custom_limits (int *pw, int *ph, int *pdx, int *pdy, int *prealh)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int w, h, dx, dy, y1, y2, dbl1, dbl2;
	int ret = 0;

	if (!pw || !ph || !pdx || !pdy) {
		reset_custom_limits ();
		return 0;
	}

	if (!isnativevidbuf(0)) {
		*pw = vidinfo->outbuffer->outwidth;
		*ph = vidinfo->outbuffer->outheight;
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
		static int interlace_count;
		// interlace = only use long frames
		if (lof_display && (interlace_count & 1) == 0)
			interlace_count++;
		if (!lof_display && (interlace_count & 1) != 0)
			interlace_count++;
		if (interlace_count < 3)
			return ret;
		if (!lof_display)
			return ret;
		interlace_count = 0;
	}

	if (plflastline_total < 4)
		plflastline_total = last_planes_vpos;

	ddffirstword_total = coord_hw_to_window_x_lores(ddffirstword_total * 2 + DIW_DDF_OFFSET);
	ddflastword_total = coord_hw_to_window_x_lores(ddflastword_total * 2 + DIW_DDF_OFFSET);

	if (doublescan <= 0 && !programmedmode) {
		int min = coord_diw_lores_to_window_x(92);
		int max = coord_diw_lores_to_window_x(460);
		diwfirstword_total = std::max(diwfirstword_total, min);
		diwlastword_total = std::min(diwlastword_total, max);
		ddffirstword_total = std::max(ddffirstword_total, min);
		ddflastword_total = std::min(ddflastword_total, max);
		if (0 && !aga_mode) {
			diwfirstword_total = std::max(ddffirstword_total, diwfirstword_total);
			diwlastword_total = std::min(ddflastword_total, diwlastword_total);
		}
	}

	w = diwlastword_total - diwfirstword_total;
	dx = diwfirstword_total - visible_left_border;

	y2 = plflastline_total;
	y2 = std::min(y2, last_planes_vpos);
	y1 = plffirstline_total;
	y1 = std::max(first_planes_vpos, y1);
	y1 = std::max(minfirstline, y1);

	dbl2 = dbl1 = currprefs.gfx_vresolution;
	if (doublescan > 0 && interlace_seen <= 0) {
		dbl1--;
		dbl2--;
	}

	h = y2 - y1;
	dy = y1 - minfirstline;

	if (first_planes_vpos == 0) {
		// no planes enabled during frame
		if (ret < 0)
			return 1;
		h = currprefs.ntscmode ? 200 : 240;
		w = 320 << currprefs.gfx_resolution;
		dy = 36 / 2;
		dx = 58;
	}

	dx = std::max(dx, 0);

	*prealh = -1;
	if (programmedmode != 1 && first_planes_vpos) {
		int th = (maxvpos - minfirstline) * 95 / 100;
		if (th > h) {
			th = xshift (th, dbl1);
			*prealh = th;
		}
	}

	dy = xshift (dy, dbl2);
	h = xshift (h, dbl1);

	if (w == 0 || h == 0)
		return 0;

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

	if (gclow == w && gcloh == h && gclox == dx && gcloy == dy)
		return ret;

	if (w <= 0 || h <= 0 || dx < 0 || dy < 0)
		return ret;
	if (doublescan <= 0 && programmedmode != 1) {
		if (dx > vidinfo->outbuffer->inwidth / 3)
			return ret;
		if (dy > vidinfo->outbuffer->inheight / 3)
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
	write_log (_T("Display Size: %dx%d Offset: %dx%d\n"), w, h, dx, dy);
	write_log (_T("First: %d Last: %d MinV: %d MaxV: %d Min: %d\n"),
		plffirstline_total, plflastline_total,
		first_planes_vpos, last_planes_vpos, minfirstline);
#endif
	center_reset = 1;
	return 1;
}

void get_custom_mouse_limits (int *pw, int *ph, int *pdx, int *pdy, int dbl)
{
	int delay1, delay2;
	int w, h, dx, dy, dbl1, dbl2, y1, y2;

	w = diwlastword_total - diwfirstword_total;
	dx = diwfirstword_total - visible_left_border;

	y2 = plflastline_total;
	y2 = std::min(y2, last_planes_vpos);
	y1 = plffirstline_total;
	y1 = std::max(first_planes_vpos, y1);
	y1 = std::max(minfirstline, y1);

	h = y2 - y1;
	dy = y1 - minfirstline;

	if (*pw > 0)
		w = *pw;

	w = xshift (w, res_shift);

	if (*ph > 0)
		h = *ph;

	delay1 = (firstword_bplcon1 & 0x0f) | ((firstword_bplcon1 & 0x0c00) >> 6);
	delay2 = ((firstword_bplcon1 >> 4) & 0x0f) | (((firstword_bplcon1 >> 4) & 0x0c00) >> 6);
//	if (delay1 == delay2)
//		dx += delay1;

	dx = xshift (dx, res_shift);

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

	w = std::max(w, 1);
	h = std::max(h, 1);
	dx = std::max(dx, 0);
	dy = std::max(dy, 0);
	*pw = w; *ph = h;
	*pdx = dx; *pdy = dy;
}

/* Record DIW of the current line for use by centering code.  */
void record_diw_line (int plfstrt, int first, int last)
{
	max_diwstop = std::max(last, max_diwstop);
	min_diwstart = std::min(first, min_diwstart);
}

STATIC_INLINE int get_shdelay_add(void)
{
	if (bplres == RES_SUPERHIRES)
		return 0;
	if (currprefs.chipset_hr)
		return 0;
	int add = bpldelay_sh;
	add >>= RES_MAX - currprefs.gfx_resolution;
	return add;
}

/*
* Screen update macros/functions
*/

/* The important positions in the line: where do we start drawing the left border,
where do we start drawing the playfield, where do we start drawing the right border.
All of these are forced into the visible window (VISIBLE_LEFT_BORDER .. VISIBLE_RIGHT_BORDER).
PLAYFIELD_START and PLAYFIELD_END are in window coordinates.  */
static int playfield_start_pre, playfield_end_pre;
static int playfield_start, playfield_end;
static int real_playfield_start, real_playfield_end;
static int playfield_diff;
static int sprite_playfield_start, sprite_end;
static int may_require_hard_way;
static int linetoscr_diw_start, linetoscr_diw_end;
static int native_ddf_left, native_ddf_right;
#if 0
static int hamleftborderhidden;
#endif

static int pixels_offset;
static int src_pixel;
/* How many pixels in window coordinates which are to the left of the left border.  */
static int unpainted;

// blank = -1: force normal border color even if borderblank is active
static xcolnr getbgc(int blank)
{
#if BG_COLOR_DEBUG
	if (exthblank)
		return xcolors[0x888];
	else if (blank > 0)
		return xcolors[0x088];
	else if (blank < 0)
		return xcolors[0x0f8];
	else if (hposblank == 1)
		return xcolors[0xf00];
	else if (hposblank == 2)
		return xcolors[0x0f0];
	else if (hposblank == 3)
		return xcolors[0x00f];
	else if (ce_is_borderblank(colors_for_drawing.extra))
		return xcolors[0xc80];
	//return colors_for_drawing.acolors[0];
	return xcolors[0xf0f];
#endif
	if (exthblank > 0 || exthblank_force) {
		return fullblack;
	}
	bool extblken = ce_is_extblankset(colors_for_drawing.extra);
	// extblken=1: hblank and vblank = black
	if (!(vb_state & VB_NOVB) && extblken && aga_mode) {
		return fullblack;
	}
	bool brdblank = ce_is_borderblank(colors_for_drawing.extra);
	if (vb_state & VB_XBORDER) {
		if (brdblank)
			return fullblack;
		return colors_for_drawing.acolors[0];
	}
	if (hposblank) {
		return fullblack;
	}
#if 0
	if (brdblank && blank == 4) {
		return colors_for_drawing.acolors[0];
	}
#endif
	// borderblank = black (overrides extblken)
	if (brdblank && blank >= 0) {
		return fullblack;
	}
	if (blank > 0) {
		return fullblack;
	}
	return colors_for_drawing.acolors[0];
}


static void set_res_shift(void)
{
	int shift = lores_shift - bplres;
	int old = res_shift;
	res_shift = shift;
	if (res_shift != old)
		pfield_set_linetoscr();
}

/* Initialize the variables necessary for drawing a line.
* This involves setting up start/stop positions and display window
* borders.  */
static void pfield_init_linetoscr (bool border)
{
	/* First, get data fetch start/stop in DIW coordinates.  */
	int ddf_left = dp_for_drawing->plfleft + DIW_DDF_OFFSET - DDF_OFFSET;
	int ddf_right = dp_for_drawing->plfright + DIW_DDF_OFFSET - DDF_OFFSET;
	int native_ddf_left2;
	bool expanded = false;

	bplmaxplanecnt = dp_for_drawing->max_planes;
	
	if (border)
		ddf_left = DISPLAY_LEFT_SHIFT;

	/* Compute datafetch start/stop in pixels; native display coordinates.  */
	native_ddf_left = coord_hw_to_window_x_lores(ddf_left);
	native_ddf_right = coord_hw_to_window_x_lores(ddf_right);

	// Blerkenwiegel/Scoopex workaround
	native_ddf_left2 = native_ddf_left;

	native_ddf_left = std::max(native_ddf_left, 0);
	if (native_ddf_right > MAX_PIXELS_PER_LINE)
		native_ddf_right = MAX_PIXELS_PER_LINE;
	native_ddf_right = std::max(native_ddf_right, native_ddf_left);

	linetoscr_diw_start = dp_for_drawing->diwfirstword;
	linetoscr_diw_end = dp_for_drawing->diwlastword;
	linetoscr_diw_start = std::max(linetoscr_diw_start, 0);
	/* Perverse cases happen. */
	linetoscr_diw_end = std::max(linetoscr_diw_end, linetoscr_diw_start);

	set_res_shift();

	playfield_start = linetoscr_diw_start;
	playfield_end = linetoscr_diw_end;

	playfield_start = std::max(playfield_start, native_ddf_left);
	playfield_end = std::min(playfield_end, native_ddf_right);

	playfield_start = std::max(playfield_start, visible_left_border);
	playfield_start = std::min(playfield_start, visible_right_border);
	playfield_end = std::max(playfield_end, visible_left_border);
	playfield_end = std::min(playfield_end, visible_right_border);

	real_playfield_start = playfield_start;
	sprite_playfield_start = playfield_start;
	real_playfield_end = playfield_end;
	sprite_end = linetoscr_diw_end;

	// Sprite hpos don't include DIW_DDF_OFFSET and can appear 1 lores pixel
	// before first bitplane pixel appears.
	// This means "bordersprite" condition is possible under OCS/ECS too. Argh!

	if (dip_for_drawing->nr_sprites) {
		int gap = 0;
		// OCS Denise: no "bordersprite"
		if (aga_mode) {
			// If AGA: "bordersprite" starts 0.5 lores pixels earlier
			if (currprefs.chipset_hr) {
				// 1 lores pixel
				gap = 1 << lores_shift;
				// 1 lores -> 1 hires but don't round to zero.
				if (gap >= 2) {
					gap /= 2;
				}
			} else {
				gap = 1 << lores_shift;
			}
		} else if (ecs_denise) {
			// If ECS Denise: "bordersprite" starts 1 lores pixel earlier
			gap = 1 << lores_shift;
		}
		if (!ce_is_borderblank(colors_for_drawing.extra)) {
			/* bordersprite off or not supported: sprites are visible until diw_end */
			if (playfield_end < linetoscr_diw_end && hblank_right_stop > playfield_end) {
				playfield_end = linetoscr_diw_end;
			}
			// A1000 shows extra sprite pixel after hdiw end
			if (denisea1000) {
				sprite_end += 1 << lores_shift;
			}
			int plfleft = dp_for_drawing->plfleft - DDF_OFFSET;
			int left = coord_hw_to_window_x_lores(plfleft);
			int total = left - dp_for_drawing->diwfirstword;
			if (total > 0 && gap > 0) {
				gap = std::min(gap, total);
				left -= gap;
			}
			left = std::max(left, visible_left_border);
			if (left < playfield_start && left >= linetoscr_diw_start) {
				playfield_start = left;
			}
			sprite_playfield_start = playfield_start;
		} else {
			if (!ce_is_bordersprite(colors_for_drawing.extra)) {
				bool early = ((dp_for_drawing->plfleft >> 1) & 1) != 0;
				int plfleft = dp_for_drawing->plfleft - DDF_OFFSET + (early ? 1 * 2 : 0);
				sprite_playfield_start = coord_hw_to_window_x_lores(plfleft);
				sprite_playfield_start -= gap;
			}
			if (playfield_end < linetoscr_diw_end && hblank_right_stop > playfield_end) {
				playfield_end = linetoscr_diw_end;
			}
		}
	}

#ifdef AGA
	// if BPLCON4 is non-zero or borderblank: it can affect background color until end of DIW.
	if (dp_for_drawing->xor_seen || ce_is_borderblank(colors_for_drawing.extra)) {
		if (playfield_end < linetoscr_diw_end && hblank_right_stop > playfield_end) {
			playfield_end = linetoscr_diw_end;
			expanded = true;
		}
	}
	playfield_diff = 0;
	may_require_hard_way = 0;
	if (dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra) && dip_for_drawing->nr_sprites) {
		int min = visible_right_border, max = visible_left_border, i;
		for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
			int x;
			x = curr_sprite_entries[dip_for_drawing->first_sprite_entry + i].pos;
			min = std::min(x, min);
			// include max extra pixels, sprite may be 2x or 4x size: 4x - 1.
			x = curr_sprite_entries[dip_for_drawing->first_sprite_entry + i].max + (4 - 1);
			max = std::max(x, max);
		}
#if 0
		min = coord_hw_to_window_x(min >> sprite_buffer_res) + (DIW_DDF_OFFSET << lores_shift);
		if (min < playfield_start)
			playfield_start = min;
		if (playfield_start < visible_left_border)
			playfield_start = visible_left_border;
#endif
		max = coord_hw_to_window_x_lores(max >> sprite_buffer_res) + (DIW_DDF_OFFSET << lores_shift);
		playfield_end = std::max(max, playfield_end);
		playfield_end = std::min(playfield_end, visible_right_border);
		sprite_playfield_start = 0;
		may_require_hard_way = 1;
		sprite_end = max;
	}
#endif

	// AGA borderblank starts horizontally 1 shres pixel before bitplanes start
	playfield_start_pre = playfield_start;
	playfield_end_pre = playfield_end;
	if (currprefs.chipset_hr && aga_mode && bplres > 0) {
		if (currprefs.gfx_resolution == RES_SUPERHIRES) {
			playfield_start_pre -= 1;
			playfield_end_pre -= 1;
		} else {
			playfield_start_pre &= ~1;
			playfield_end_pre &= ~1;
		}
	}

	unpainted = visible_left_border < playfield_start ? 0 : visible_left_border - playfield_start;
	unpainted = res_shift_from_window (unpainted);

	int first_x = sprite_first_x;
	int last_x = sprite_last_x;
	if (first_x < last_x) {
		if (dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra)) {
			first_x = std::min(first_x, visible_left_border);
			last_x = std::max(last_x, visible_right_border);
		}
		first_x = std::max(first_x, 0);
		last_x = std::min(last_x, MAX_PIXELS_PER_LINE - 2);
		if (first_x < last_x)
			memset (spritepixels + first_x, 0, sizeof (struct spritepixelsbuf) * (last_x - first_x + 1));
	}

	sprite_last_x = 0;
	sprite_first_x = MAX_PIXELS_PER_LINE - 1;

	/* Now, compute some offsets.  */
	ddf_left -= DISPLAY_LEFT_SHIFT;
	ddf_left = std::max(ddf_left, 0);
	ddf_left <<= bplres;
	pixels_offset = MAX_PIXELS_PER_LINE - ddf_left;

	int leftborderhidden = playfield_start - native_ddf_left2;

#if 0
	hamleftborderhidden = 0;
	if (hblank_left_start > playfield_start) {
		leftborderhidden += hblank_left_start - playfield_start;
		hamleftborderhidden = hblank_left_start - playfield_start;
	}
#endif

	src_pixel = MAX_PIXELS_PER_LINE + res_shift_from_window(leftborderhidden);

	if (may_require_hard_way) {
		// must use "hard_way" rendering if negative leftborderhidden
		if (src_pixel < MAX_PIXELS_PER_LINE)
			may_require_hard_way = -1;
		if (playfield_start < real_playfield_start) {
			playfield_diff = res_shift_from_window(real_playfield_start - playfield_start);
		}
	}

	if (dip_for_drawing->nr_sprites == 0 && !expanded)
		return;

	if (dip_for_drawing->nr_sprites && aga_mode) {
		int add = get_shdelay_add();
		if (add) {
			if (sprite_playfield_start > 0) {
				sprite_playfield_start -= add;
			} else {
				;// this is most likely wrong: playfield_start -= add;
			}
		}
	}

	/* We need to clear parts of apixels.  */
	if (linetoscr_diw_start < native_ddf_left) {
		int size = res_shift_from_window (native_ddf_left - linetoscr_diw_start);
		linetoscr_diw_start = native_ddf_left;
		memset (pixdata.apixels + MAX_PIXELS_PER_LINE - size, 0, size);
	}
	if (linetoscr_diw_end > native_ddf_right) {
		int pos = res_shift_from_window(native_ddf_right - native_ddf_left);
		int size = res_shift_from_window(linetoscr_diw_end - native_ddf_right);
		if (pos + size > MAX_PIXELS_PER_LINE)
			size = MAX_PIXELS_PER_LINE - pos;
		if (size > 0)
			memset (pixdata.apixels + MAX_PIXELS_PER_LINE + pos, 0, size);
		linetoscr_diw_start = native_ddf_left;
	}
}

// erase sprite graphics in pixdata if they were outside of ddf
static void pfield_erase_hborder_sprites (void)
{
	if (sprite_first_x < native_ddf_left) {
		int size = res_shift_from_window (native_ddf_left - sprite_first_x);
		memset (pixdata.apixels + MAX_PIXELS_PER_LINE - size, 0, size);
	}
	if (sprite_last_x > native_ddf_right) {
		int pos = res_shift_from_window (native_ddf_right - native_ddf_left);
		int size = res_shift_from_window (sprite_last_x - native_ddf_right);
		if (pos + size > MAX_PIXELS_PER_LINE)
			size = MAX_PIXELS_PER_LINE - pos;
		if (size > 0)
			memset (pixdata.apixels + MAX_PIXELS_PER_LINE + pos, 0, size);
	}
}

// erase whole viewable area if sprite in upper or lower border
static void pfield_erase_vborder_sprites (void)
{
	if (visible_right_border <= visible_left_border)
		return;
	int pos = 0;
	int size = 0;
	if (visible_left_border < native_ddf_left) {
		size = res_shift_from_window (native_ddf_left - visible_left_border);
		pos = -size;
	}
	if (visible_right_border > native_ddf_left)
		size += res_shift_from_window (visible_right_border - native_ddf_left);
	memset (pixdata.apixels + MAX_PIXELS_PER_LINE - pos, 0, size);
}


STATIC_INLINE uae_u16 merge_2pixel16 (uae_u16 p1, uae_u16 p2)
{
	uae_u16 v = ((((p1 >> xredcolor_s) & xredcolor_m) + ((p2 >> xredcolor_s) & xredcolor_m)) / 2) << xredcolor_s;
	v |= ((((p1 >> xbluecolor_s) & xbluecolor_m) + ((p2 >> xbluecolor_s) & xbluecolor_m)) / 2) << xbluecolor_s;
	v |= ((((p1 >> xgreencolor_s) & xgreencolor_m) + ((p2 >> xgreencolor_s) & xgreencolor_m)) / 2) << xgreencolor_s;
	return v;
}
STATIC_INLINE uae_u32 merge_2pixel32 (uae_u32 p1, uae_u32 p2)
{
	uae_u32 v = ((((p1 >> 16) & 0xff) + ((p2 >> 16) & 0xff)) / 2) << 16;
	v |= ((((p1 >> 8) & 0xff) + ((p2 >> 8) & 0xff)) / 2) << 8;
	v |= ((((p1 >> 0) & 0xff) + ((p2 >> 0) & 0xff)) / 2) << 0;
	return v;
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
			if (aga_mode) {
				if (colors_for_drawing.color_regs_aga[v] & COLOR_CHANGE_GENLOCK)
					return false;
			} else {
				if (colors_for_drawing.color_regs_ecs[v] & 0x8000)
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
STATIC_INLINE bool get_genlock_transparency(uae_u8 v)
{
	if (!ecs_genlock_features_active) {
		if (v == 0)
			return false;
		return true;
	} else {
		return get_genlock_very_rare_and_complex_case(v);
	}
}

STATIC_INLINE bool get_genlock_transparency_border(void)
{
	if (!ecs_genlock_features_active) {
		return false;
	} else {
		// border color with BRDNTRAN bit set = not transparent
		if (ce_is_borderntrans(colors_for_drawing.extra))
			return true;
		return get_genlock_very_rare_and_complex_case(0);
	}
}

STATIC_INLINE void fill_line_16 (uae_u8 *buf, int start, int stop, int blank)
{
	uae_u16 *b = (uae_u16 *)buf;
	unsigned int i;
	unsigned int rem = 0;
	xcolnr col = getbgc (blank);
	if (((uintptr_t)&b[start]) & 1)
		b[start++] = (uae_u16) col;
	if (start >= stop)
		return;
	if (((uintptr_t)&b[stop]) & 1) {
		rem++;
		stop--;
	}
	for (i = start; i < stop; i += 2) {
		uae_u32 *b2 = (uae_u32 *)&b[i];
		*b2 = col;
	}
	if (rem)
		b[stop] = (uae_u16)col;
}

STATIC_INLINE void fill_line_32 (uae_u8 *buf, int start, int stop, int blank)
{
	uae_u32 *b = (uae_u32 *)buf;
	unsigned int i;
	xcolnr col = getbgc (blank);
	for (i = start; i < stop; i++)
		b[i] = col;
}

static void pfield_do_fill_line (int start, int stop, int blank)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	if (stop <= start)
		return;
	switch (vidinfo->drawbuffer.pixbytes) {
	case 2: fill_line_16 (xlinebuffer, start, stop, blank); break;
	case 4: fill_line_32 (xlinebuffer, start, stop, blank); break;
	}
	if (need_genlock_data) {
		memset(xlinebuffer_genlock + start, get_genlock_transparency_border(), stop - start);
	}
}

static void pfield_do_darken_line(int start, int stop, int vp)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	if (stop <= start)
		return;
	if (currprefs.gfx_resolution) {
		vp >>= 1;
	}
	bool brd = !vsync_debug && !hsync_debug && !vblank_debug && !hblank_debug && !hcenter_debug && ce_is_borderblank(colors_for_drawing.extra);
	bool vs = vsync_debug;
	if (hcenter_debug) {
		vs = false;

	}
	int c0 = 0x000;
	int c1 = 0x136;
	int c2 = 0x686;
	bool csync = currprefs.gfx_overscanmode == OVERSCANMODE_ULTRA + 2;
	bool trans = currprefs.gfx_overscanmode == OVERSCANMODE_ULTRA;
	if (!csync) {
		bool sync = hsync_debug || vs;
		if (sync && currprefs.gfx_overscanmode > OVERSCANMODE_ULTRA) {
			c0 = 0x831;
			c1 = 0x000;
		}
		if (hcenter_debug && vsync_debug && currprefs.gfx_overscanmode > OVERSCANMODE_ULTRA) {
			c0 = 0x381;
		}
	} else {
		if (hsync_debug) {
			c0 = 0x831;
			c1 = 0x000;
		}
		if (hcenter_debug && currprefs.gfx_overscanmode > OVERSCANMODE_ULTRA) {
			c0 = 0x381;
		}
	}
	if (!trans) {
		vp = 0;
		if (!c0) {
			c0 = c1;
		} else {
			c1 = c0;
		}
	}
	if (vidinfo->drawbuffer.pixbytes == 4) {
		uae_u32 *b = (uae_u32 *)xlinebuffer;
		for (int i = start; i < stop; i++) {
			int s = (i ^ vp) & 3;
			if (brd) {
				if (playfield_start_pre >= playfield_start && (i < playfield_start || i >= playfield_end)) {
					if (s == 1) {
						b[i] = xcolors[c2];
					} else if (s == 3) {
						b[i] = xcolors[c1];
					}
				} else if (i < playfield_start_pre || i >= playfield_end_pre) {
					if (s == 1) {
						b[i] = xcolors[c2];
					} else if (s == 3) {
						b[i] = xcolors[c1];
					}
				}
			} else {
				if (s == 1) {
					b[i] = xcolors[c0];
				} else if (s == 3) {
					b[i] = xcolors[c1];
				}
				if (!trans) {
					if (s == 0) {
						b[i] = xcolors[c0];
					} else if (s == 2) {
						b[i] = xcolors[c1];
					}
				}
			}
		}
	}
}

static void fill_line2(int startpos, int len)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int shift;
	int nints, nrem;
	int *start;
	xcolnr val;

	shift = 0;
	if (vidinfo->drawbuffer.pixbytes == 2)
		shift = 1;
	if (vidinfo->drawbuffer.pixbytes == 4)
		shift = 2;

	nints = len >> (2 - shift);
	nrem = nints & 7;
	nints &= ~7;
	start = (int *)(((uae_u8*)xlinebuffer) + (startpos << shift));
	val = getbgc(0);
	for (; nints > 0; nints -= 8, start += 8) {
		*start = val;
		*(start+1) = val;
		*(start+2) = val;
		*(start+3) = val;
		*(start+4) = val;
		*(start+5) = val;
		*(start+6) = val;
		*(start+7) = val;
	}

	switch (nrem) {
	case 7:
		*start++ = val;
	case 6:
		*start++ = val;
	case 5:
		*start++ = val;
	case 4:
		*start++ = val;
	case 3:
		*start++ = val;
	case 2:
		*start++ = val;
	case 1:
		*start = val;
	}
}

static void fill_line_border(int lineno, int bordertype)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int lastpos = visible_left_border;
	int endpos = visible_left_border + vidinfo->drawbuffer.inwidth;
	int w = endpos - lastpos;

	int hblank_left = exthblank ? hblank_left_start : hblank_left_start_hard;
	int hblank_right = exthblank ? hblank_right_stop : hblank_right_stop_hard;

	if (lineno < visible_top_start || lineno < vblank_top_start || lineno >= visible_bottom_stop || lineno >= vblank_bottom_stop || full_blank) {
		int b = hposblank;
		hposblank = 3;
		fill_line2(lastpos, w);
		if (syncdebug && bordertype >= 0) {
			pfield_do_darken_line(lastpos, endpos, lineno);
		}
		if (need_genlock_data) {
			memset(xlinebuffer_genlock + lastpos, get_genlock_transparency_border(), w);
		}
		hposblank = b;
		return;
	}

	// full hblank
	if (hposblank) {
		hposblank = 3;
		fill_line2(lastpos, w);
		if (syncdebug && bordertype >= 0) {
			pfield_do_darken_line(lastpos, endpos, lineno);
		}
		if (need_genlock_data) {
			memset(xlinebuffer_genlock + lastpos, get_genlock_transparency_border(), w);
		}
		return;
	}

	// hblank not visible
	if (hblank_left <= lastpos && hblank_right >= endpos) {
		fill_line2(lastpos, w);
		if (need_genlock_data) {
			memset(xlinebuffer_genlock + lastpos, get_genlock_transparency_border(), w);
		}
		return;
	}

	// left, right or both hblanks visible
	if (lastpos < hblank_left) {
		int t = hblank_left < endpos ? hblank_left : endpos;
		pfield_do_fill_line(lastpos, t, true);
		lastpos = t;
	}
	if (lastpos < hblank_right) {
		int t = hblank_right < endpos ? hblank_right : endpos;
		pfield_do_fill_line(lastpos, t, false);
		lastpos = t;
	}
	if (lastpos < endpos) {
		pfield_do_fill_line(lastpos, endpos, true);
	}
}

static int sprite_shdelay;
#define SPRITE_DEBUG 0
static uae_u8 render_sprites(int pos, int dualpf, uae_u8 apixel, int aga)
{
	struct spritepixelsbuf *spb = &spritepixels[pos];
	unsigned int v = spb->data;
	int *shift_lookup = dualpf ? (bpldualpfpri ? dblpf_ms2 : dblpf_ms1) : dblpf_ms;
	int maskshift, plfmask;

	if (!sprite_visibility) {
		return 0;
	}

	// If 64 pixel wide sprite and FMODE gets lowered when sprite's
	// first 32 pixels are being drawn: matching pixel(s) in second
	// 32 pixel part gets blanked.
	if (aga && spb->stfmdata && sprite_smaller_than_64_inuse && sprite_smaller_than_64) {
		spb[32 << currprefs.gfx_resolution].data &= ~spb->stfmdata;
	}

	// shdelay hack, above &spritepixels[pos] is correct. 
	pos += sprite_shdelay;
	/* The value in the shift lookup table is _half_ the shift count we
	need.  This is because we can't shift 32 bits at once (undefined
	behaviour in C).  */
	maskshift = shift_lookup[apixel];
	plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
	v &= ~plfmask;
	/* Extra 1 sprite pixel at DDFSTRT is only possible if at least 1 plane is active */
	if (pos >= sprite_playfield_start && pos < sprite_end && (v != 0 || SPRITE_DEBUG)) {
		unsigned int vlo, vhi, col;
		unsigned int v1 = v & 255;
		/* OFFS determines the sprite pair with the highest priority that has
		any bits set.  E.g. if we have 0xFF00 in the buffer, we have sprite
		pairs 01 and 23 cleared, and pairs 45 and 67 set, so OFFS will
		have a value of 4.
		2 * OFFS is the bit number in V of the sprite pair, and it also
		happens to be the color offset for that pair. 
		*/
		int offs;
		if (v1 == 0)
			offs = 4 + sprite_offs[v >> 8];
		else
			offs = sprite_offs[v1];

		/* Shift highest priority sprite pair down to bit zero.  */
		v >>= offs * 2;
		v &= 15;
#if SPRITE_DEBUG > 0
		v ^= 8;
#endif
		if ((spb->flags & 1) && (spb->stdata & (3 << offs))) {
			col = v;
			if (aga)
				col += sbasecol[1];
			else
				col += 16;
		} else {
			/* This sequence computes the correct color value.  We have to select
			either the lower-numbered or the higher-numbered sprite in the pair.
			We have to select the high one if the low one has all bits zero.
			If the lower-numbered sprite has any bits nonzero, (VLO - 1) is in
			the range of 0..2, and with the mask and shift, VHI will be zero.
			If the lower-numbered sprite is zero, (VLO - 1) is a mask of
			0xFFFFFFFF, and we select the bits of the higher numbered sprite
			in VHI.
			This is _probably_ more efficient than doing it with branches.  */
			vlo = v & 3;
			vhi = (v & (vlo - 1)) >> 2;
			col = (vlo | vhi);
			if (aga) {
				if (vhi > 0)
					col += sbasecol[1];
				else
					col += sbasecol[0];
			} else {
				col += 16;
			}
			col += offs * 2;
		}

#if SPRITE_DEBUG_HIDE
		col = 0;
#endif
		return col;
	}

	return 0;
}

#include "linetoscr.cpp"

#define LTPARMS src_pixel, start, stop

#ifdef ECS_DENISE
/* ECS SuperHires special cases */

#define PUTBPIX(x) buf[dpix] = (x);

static uae_u8 sh_render_sprites(int pos, int dualpf, uae_u8 apixel, int aga)
{
	struct spritepixelsbuf *spb = &spritepixels[pos];
	unsigned int v = spb->data;
	int *shift_lookup = dualpf ? (bpldualpfpri ? dblpf_ms2 : dblpf_ms1) : dblpf_ms;
	int maskshift, plfmask;

	if (exthblank || exthblank_force) {
		return 0;
	}
	if (extborder && ce_is_borderblank(colors_for_drawing.extra)) {
		return 0;
	}

	maskshift = shift_lookup[apixel];
	plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
	v &= ~plfmask;
	if (pos >= sprite_playfield_start && pos < sprite_end && v != 0) {
		uae_u32 col;
		uae_u32 v1 = v & 255;
		int offs;
		if (v1 == 0)
			offs = 4 + sprite_offs[v >> 8];
		else
			offs = sprite_offs[v1];
		v >>= offs * 2;
		v &= 15;
		col = (v >> 0) & 3;
		if (!col) {
			col = (v >> 2) & 3;
		}
		// not correct
		if ((spb->flags & 1) && (spb->stdata & (3 << offs))) {
			int col1 = (v >> 0) & 3;
			int col2 = (v >> 2) & 3;
			col = col1 | col2;
		}
		return col;
	}
	return 0;
}

static uae_u32 shsprite(int dpix, uae_u32 spix_val, uae_u32 v, int add, int spr)
{
	uae_u8 sprcol1, sprcol2, off;
	uae_u16 scol;
	if (!spr) {
		return v;
	}
	dpix &= ~1;
	struct spritepixelsbuf *spb = &spritepixels[dpix];
	int sdpix = dpix;
	if (spb->flags & 2) {
		sdpix -= add >> 1;
	}
	int mask = 3;
	sprcol1 = sh_render_sprites(sdpix, bpldualpf, spix_val, 0);
	sprcol2 = sh_render_sprites(sdpix + add, bpldualpf, spix_val, 0);
	off = (sprcol2 & mask) * 4 + (sprcol1 & mask) + 16;
	if ((dpix & add)) {
		if (!(sprcol2 & mask)) {
			return v;
		}
		scol = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
	} else {
		if (!(sprcol1 & mask)) {
			return v;
		}
		scol = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
	}
	scol |= scol >> 2;
	return xcolors[scol];
}

static int NOINLINE linetoscr_32_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u32 *buf = (uae_u32 *)xlinebuffer;

	while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		PUTBPIX(shsprite(dpix, spix_val1, xcolors[v], 2, spr));
		dpix++;
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		PUTBPIX(shsprite(dpix, spix_val2, xcolors[v], 2, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_32_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_32_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_32_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_32_sh_func(spix, dpix, stoppos, false);
}

static int NOINLINE linetoscr_16_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u16 *buf = (uae_u16 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], 2, spr));
		dpix++;
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val2, xcolors[v], 2, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_16_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_16_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_16_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_16_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_32_shrink1_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u32 *buf = (uae_u32 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], 1, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_32_shrink1_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink1_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_32_shrink1_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink1_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_32_shrink1f_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u32 *buf = (uae_u32 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2, dpix_val1, dpix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		PUTBPIX(shsprite (dpix, spix_val1, merge_2pixel32 (dpix_val1, dpix_val2), 1, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_32_shrink1f_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink1f_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_32_shrink1f_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink1f_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_16_shrink1_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u16 *buf = (uae_u16 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], 1, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_16_shrink1_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink1_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_16_shrink1_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink1_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_16_shrink1f_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u16 *buf = (uae_u16 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2, dpix_val1, dpix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		PUTBPIX(shsprite (dpix, spix_val1, merge_2pixel16 (dpix_val1, dpix_val2), 1, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_16_shrink1f_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink1f_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_16_shrink1f_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink1f_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_32_shrink2_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u32 *buf = (uae_u32 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], 1, spr));
		spix+=2;
		dpix++;
	}
	return spix;
}
static int linetoscr_32_shrink2_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink2_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_32_shrink2_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink2_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_32_shrink2f_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u32 *buf = (uae_u32 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u32 spix_val1, spix_val2, dpix_val1, dpix_val2, dpix_val3, dpix_val4;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val3 = merge_2pixel32 (dpix_val1, dpix_val2);
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val4 = merge_2pixel32 (dpix_val1, dpix_val2);
		PUTBPIX(shsprite (dpix, spix_val1, merge_2pixel32 (dpix_val3, dpix_val4), 1, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_32_shrink2f_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink2f_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_32_shrink2f_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_32_shrink2f_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_16_shrink2_sh_func(int spix, int dpix, int stoppos, int spr)
{
	uae_u16 *buf = (uae_u16 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], 1, spr));
		spix+=2;
		dpix++;
	}
	return spix;
}
static int linetoscr_16_shrink2_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink2_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_16_shrink2_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink2_sh_func(spix, dpix, stoppos, false);
}
static int NOINLINE linetoscr_16_shrink2f_sh_func (int spix, int dpix, int stoppos, int spr)
{
	uae_u16 *buf = (uae_u16 *) xlinebuffer;

	while (dpix < stoppos) {
		uae_u16 spix_val1, spix_val2, dpix_val1, dpix_val2, dpix_val3, dpix_val4;
		uae_u16 v;
		int off;
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val3 = merge_2pixel32 (dpix_val1, dpix_val2);
		spix_val1 = pixdata.apixels[spix++];
		spix_val2 = pixdata.apixels[spix++];
		off = ((spix_val2 & 3) * 4) + (spix_val1 & 3) + ((spix_val1 | spix_val2) & 16);
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc) << 0;
		v |= v >> 2;
		dpix_val1 = xcolors[v];
		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		dpix_val2 = xcolors[v];
		dpix_val4 = merge_2pixel32 (dpix_val1, dpix_val2);
		PUTBPIX(shsprite (dpix, spix_val1, merge_2pixel16 (dpix_val3, dpix_val4), 1, spr));
		dpix++;
	}
	return spix;
}
static int linetoscr_16_shrink2f_sh_spr(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink2f_sh_func(spix, dpix, stoppos, true);
}
static int linetoscr_16_shrink2f_sh(int spix, int dpix, int stoppos)
{
	return linetoscr_16_shrink2f_sh_func(spix, dpix, stoppos, false);
}
#endif

typedef int(*call_linetoscr)(int spix, int dpix, int dpix_end);
typedef int(*call_linetoscrb)(int spix, int dpix, int dpix_end, int blank);

static call_linetoscr pfield_do_linetoscr_normal, pfield_do_linetoscr_normal2;
static call_linetoscr pfield_do_linetoscr_sprite, pfield_do_linetoscr_sprite2;
static call_linetoscrb pfield_do_linetoscr_spriteonly;

static void pfield_do_linetoscr(int start, int stop, int blank)
{
	int pixel = pfield_do_linetoscr_normal(src_pixel, start, stop);
	if (exthblank || exthblank_force) {
		pfield_do_fill_line(start, stop, 1);
	} else if (extborder) {
#if EXTBORDER_BLANK
		bool bb = true;
#else
		bool bb = ce_is_borderblank(colors_for_drawing.extra);
#endif
		pfield_do_fill_line(start, stop, bb ? 1 : 0);
	}
	src_pixel = pixel;
}
static void pfield_do_linetoscr_spr(int start, int stop, int blank)
{
	int pixel;
	if (extborder) {
#if EXTBORDER_BLANK
		bool bb = true;
#else
		bool bb = ce_is_borderblank(colors_for_drawing.extra);
#endif
		pixel = pfield_do_linetoscr_sprite(src_pixel, start, stop);
		pfield_do_fill_line(start, stop, bb || exthblank > 0 || exthblank_force);
	} else {
		pixel = pfield_do_linetoscr_sprite(src_pixel, start, stop);
		if (exthblank || exthblank_force) {
			pfield_do_fill_line(start, stop, 1);
		}
	}
	src_pixel = pixel;
}
static int pfield_do_nothing(int a, int b, int c)
{
	return a;
}
static int pfield_do_nothingb(int a, int b, int c, int d)
{
	return a;
}

/* AGA subpixel delay hack */
static call_linetoscr pfield_do_linetoscr_shdelay_normal;
static call_linetoscr pfield_do_linetoscr_shdelay_sprite;

static int pfield_do_linetoscr_normal_shdelay(int spix, int dpix, int dpix_end)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int add = get_shdelay_add();
	int add2 = add * vidinfo->drawbuffer.pixbytes;
	if (add) {
		// Fill skipped pixel(s).
		pfield_do_linetoscr_shdelay_sprite(spix - 1, dpix, dpix + add);
	}
	xlinebuffer += add2;
	int out = pfield_do_linetoscr_shdelay_normal(spix, dpix, dpix_end);
	xlinebuffer -= add2;
	return out;
}
static int pfield_do_linetoscr_sprite_shdelay(int spix, int dpix, int dpix_end)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int out = spix;
	if (dpix < real_playfield_start && dpix_end > real_playfield_start) {
		// Crosses real_playfield_start.
		// Render only from dpix to real_playfield_start.
		int len = real_playfield_start - dpix;
		out = pfield_do_linetoscr_spriteonly(out, dpix, dpix + len, false);
		dpix = real_playfield_start;
	} else if (dpix_end <= real_playfield_start) {
		// Does not cross real_playfield_start, nothing special needed.
		out = pfield_do_linetoscr_spriteonly(out, dpix, dpix_end, false);
		return out;
	}
	// Render bitplane with subpixel scroll, from real_playfield_start to end.
	int add = get_shdelay_add();
	int add2 = add * vidinfo->drawbuffer.pixbytes;
	if (add) {
		pfield_do_linetoscr_shdelay_sprite(out - 1, dpix, dpix + add);
	}
	sprite_shdelay = add;
	spritepixels += add;
	xlinebuffer += add2;
	out = pfield_do_linetoscr_shdelay_sprite(out, dpix, dpix_end);
	xlinebuffer -= add2;
	spritepixels -= add;
	sprite_shdelay = 0;
	return out;
}

static void pfield_set_linetoscr (void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	xlinecheck(start, stop);
	p_acolors = colors_for_drawing.acolors;
	p_xcolors = xcolors;
	bpland = 0xff;
	if (bplbypass) {
		p_acolors = direct_colors_for_drawing.acolors;
	}
	spritepixels = spritepixels_buffer;
	pfield_do_linetoscr_spriteonly = pfield_do_nothingb;
#ifdef AGA
	if (aga_mode) {
		if (res_shift == 0) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_aga_genlock : linetoscr_16_aga;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_aga_spr_genlock : linetoscr_16_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_16_aga_spronly;
				break;
				case 4:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_aga_genlock : linetoscr_32_aga;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_aga_spr_genlock : linetoscr_32_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_32_aga_spronly;
				break;
			}
		} else if (res_shift == 2) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_stretch2_aga_genlock : linetoscr_16_stretch2_aga;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_stretch2_aga_spr_genlock : linetoscr_16_stretch2_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_16_stretch2_aga_spronly;
				break;
				case 4:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_stretch2_aga_genlock : linetoscr_32_stretch2_aga;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_stretch2_aga_spr_genlock : linetoscr_32_stretch2_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_32_stretch2_aga_spronly;
				break;
			}
		} else if (res_shift == 1) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_stretch1_aga_genlock : linetoscr_16_stretch1_aga;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_stretch1_aga_spr_genlock : linetoscr_16_stretch1_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_16_stretch1_aga_spronly;
				break;
				case 4:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_stretch1_aga_genlock : linetoscr_32_stretch1_aga;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_stretch1_aga_spr_genlock : linetoscr_32_stretch1_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_32_stretch1_aga_spronly;
				break;
			}
		} else if (res_shift == -1) {
			if (currprefs.gfx_lores_mode) {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_shrink1f_aga_genlock : linetoscr_16_shrink1f_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_shrink1f_aga_spr_genlock : linetoscr_16_shrink1f_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink1f_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_shrink1f_aga_genlock : linetoscr_32_shrink1f_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_shrink1f_aga_spr_genlock : linetoscr_32_shrink1f_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink1f_aga_spronly;
					break;
				}
			} else {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_shrink1_aga_genlock : linetoscr_16_shrink1_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_shrink1_aga_spr_genlock : linetoscr_16_shrink1_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink1_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_shrink1_aga_genlock : linetoscr_32_shrink1_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_shrink1_aga_spr_genlock : linetoscr_32_shrink1_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink1_aga_spronly;
					break;
				}
			}
		} else if (res_shift == -2) {
			if (currprefs.gfx_lores_mode) {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_shrink2f_aga_genlock : linetoscr_16_shrink2f_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_shrink2f_aga_spr_genlock : linetoscr_16_shrink2f_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink2f_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_shrink2f_aga_genlock : linetoscr_32_shrink2f_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_shrink2f_aga_spr_genlock : linetoscr_32_shrink2f_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink2f_aga_spronly;
					break;
				}
			} else {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_shrink2_aga_genlock : linetoscr_16_shrink2_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_shrink2_aga_spr_genlock : linetoscr_16_shrink2_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink2_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_shrink2_aga_genlock : linetoscr_32_shrink2_aga;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_shrink2_aga_spr_genlock : linetoscr_32_shrink2_aga_spr;
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink2_aga_spronly;
					break;
				}
			}
		}
		if (get_shdelay_add()) {
			pfield_do_linetoscr_shdelay_normal = pfield_do_linetoscr_normal;
			pfield_do_linetoscr_shdelay_sprite = pfield_do_linetoscr_sprite;
			pfield_do_linetoscr_normal = pfield_do_linetoscr_normal_shdelay;
			pfield_do_linetoscr_sprite = pfield_do_linetoscr_sprite_shdelay;
		}
	}
#endif
	// A1000 Denise right border "bordersprite" bug
	if (denisea1000) {
		if (res_shift == 0) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_spriteonly = linetoscr_16_aga_spronly;
				break;
				case 4:
				pfield_do_linetoscr_spriteonly = linetoscr_32_aga_spronly;
				break;
			}
		} else if (res_shift == 2) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_spriteonly = linetoscr_16_stretch2_aga_spronly;
				break;
				case 4:
				pfield_do_linetoscr_spriteonly = linetoscr_32_stretch2_aga_spronly;
				break;
			}
		} else if (res_shift == 1) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_spriteonly = linetoscr_16_stretch1_aga_spronly;
				break;
				case 4:
				pfield_do_linetoscr_spriteonly = linetoscr_32_stretch1_aga_spronly;
				break;
			}
		} else if (res_shift == -1) {
			if (currprefs.gfx_lores_mode) {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink1f_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink1f_aga_spronly;
					break;
				}
			} else {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink1_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink1_aga_spronly;
					break;
				}
			}
		} else if (res_shift == -2) {
			if (currprefs.gfx_lores_mode) {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink2f_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink2f_aga_spronly;
					break;
				}
			} else {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_spriteonly = linetoscr_16_shrink2_aga_spronly;
					break;
					case 4:
					pfield_do_linetoscr_spriteonly = linetoscr_32_shrink2_aga_spronly;
					break;
				}
			}
		}
	}
#ifdef ECS_DENISE
	if (!aga_mode && ecsshres) {
		// TODO: genlock support
		if (res_shift == 0) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_normal = linetoscr_16_sh;
				pfield_do_linetoscr_sprite = linetoscr_16_sh_spr;
				break;
				case 4:
				pfield_do_linetoscr_normal = linetoscr_32_sh;
				pfield_do_linetoscr_sprite = linetoscr_32_sh_spr;
				break;
			}
		} else if (res_shift == -1) {
			if (currprefs.gfx_lores_mode) {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = linetoscr_16_shrink1f_sh;
					pfield_do_linetoscr_sprite = linetoscr_16_shrink1f_sh_spr;
					break;
					case 4:
					pfield_do_linetoscr_normal = linetoscr_32_shrink1f_sh;
					pfield_do_linetoscr_sprite = linetoscr_32_shrink1f_sh_spr;
					break;
				}
			} else {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = linetoscr_16_shrink1_sh;
					pfield_do_linetoscr_sprite = linetoscr_16_shrink1_sh_spr;
					break;
					case 4:
					pfield_do_linetoscr_normal = linetoscr_32_shrink1_sh;
					pfield_do_linetoscr_sprite = linetoscr_32_shrink1_sh_spr;
					break;
				}
			}
		} else if (res_shift == -2) {
			if (currprefs.gfx_lores_mode) {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = linetoscr_16_shrink2f_sh;
					pfield_do_linetoscr_sprite = linetoscr_16_shrink2f_sh_spr;
					break;
					case 4:
					pfield_do_linetoscr_normal = linetoscr_32_shrink2f_sh;
					pfield_do_linetoscr_sprite = linetoscr_32_shrink2f_sh_spr;
					break;
				}
			} else {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = linetoscr_16_shrink2_sh;
					pfield_do_linetoscr_sprite = linetoscr_16_shrink2_sh_spr;
					break;
					case 4:
					pfield_do_linetoscr_normal = linetoscr_32_shrink2_sh;
					pfield_do_linetoscr_sprite = linetoscr_32_shrink2_sh_spr;
					break;
				}
			}
		}
	}
#endif
	if (!aga_mode && !ecsshres) {
		if (res_shift == 0) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_genlock : linetoscr_16;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_spr_genlock : linetoscr_16_spr;
				break;
				case 4:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_genlock : linetoscr_32;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_spr_genlock : linetoscr_32_spr;
				break;
			}
		} else if (res_shift == 2) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_stretch2_genlock : linetoscr_16_stretch2;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_stretch2_spr_genlock : linetoscr_16_stretch2_spr;
				break;
				case 4:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_stretch2_genlock : linetoscr_32_stretch2;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_stretch2_spr_genlock : linetoscr_32_stretch2_spr;
				break;
			}
		} else if (res_shift == 1) {
			switch (vidinfo->drawbuffer.pixbytes) {
				case 2:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_stretch1_genlock : linetoscr_16_stretch1;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_stretch1_spr_genlock : linetoscr_16_stretch1_spr;
				break;
				case 4:
				pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_stretch1_genlock : linetoscr_32_stretch1;
				pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_stretch1_spr_genlock : linetoscr_32_stretch1_spr;
				break;
			}
		} else if (res_shift == -1) {
				if (currprefs.gfx_lores_mode) {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_shrink1f_genlock : linetoscr_16_shrink1f;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_shrink1f_spr_genlock : linetoscr_16_shrink1f_spr;
					break;
					case 4:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_shrink1f_genlock : linetoscr_32_shrink1f;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_shrink1f_spr_genlock : linetoscr_32_shrink1f_spr;
					break;
				}
			} else {
				switch (vidinfo->drawbuffer.pixbytes) {
					case 2:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_16_shrink1_genlock : linetoscr_16_shrink1;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_16_shrink1_spr_genlock : linetoscr_16_shrink1_spr;
					break;
					case 4:
					pfield_do_linetoscr_normal = need_genlock_data ? linetoscr_32_shrink1_genlock : linetoscr_32_shrink1;
					pfield_do_linetoscr_sprite = need_genlock_data ? linetoscr_32_shrink1_spr_genlock : linetoscr_32_shrink1_spr;
					break;
				}
			}
		}
	}
	pfield_do_linetoscr_normal2 = pfield_do_linetoscr_normal;
	pfield_do_linetoscr_sprite2 = pfield_do_linetoscr_sprite2;
}

// A1000 Denise right border bug: sprites have 1 extra lores pixel visible
static void pfield_do_linetoscr_bordersprite_a1000(int start, int stop, int blank)
{
	if (blank || exthblank > 0 || exthblank_force || extborder) {
		pfield_do_fill_line(start, stop, blank);
		return;
	}
	pfield_do_fill_line(start, stop, blank);
	if (start < sprite_playfield_start) {
		return;
	}
	if (stop > sprite_end) {
		stop = sprite_end;
		if (start >= stop) {
			return;
		}
	}
	pfield_do_linetoscr_spriteonly(src_pixel, start, stop, false);
}

// left or right AGA border sprite
static void pfield_do_linetoscr_bordersprite_aga(int start, int stop, int blank)
{
	if (blank || exthblank > 0 || exthblank_force || extborder) {
		pfield_do_fill_line(start, stop, blank);
		return;
	}
	pfield_do_linetoscr_spriteonly(src_pixel, start, stop, false);
}

static void dummy_worker (int start, int stop, int blank)
{
}

static int ham_decode_pixel;
static uae_u32 ham_lastcolor;

static void decode_ham_pixel(int hdp)
{
	if (!bplham) {
		int pv = pixdata.apixels[hdp];
#ifdef AGA
		if (aga_mode)
			ham_lastcolor = colors_for_drawing.color_regs_aga[pv ^ bplxor] & 0xffffff;
		else
#endif
			ham_lastcolor = colors_for_drawing.color_regs_ecs[pv] & 0xfff;
	} else if (aga_mode) {
		if (bplplanecnt >= 7) { /* AGA mode HAM8 */
			int pw = pixdata.apixels[hdp];
			int pv = pw ^ bplxor;
			int pc = pv >> 2;
			switch (pv & 0x3)
			{
				case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pc] & 0xffffff; break;
				case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pw & 0xFC); break;
				case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pw & 0xFC) << 16; break;
				case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pw & 0xFC) << 8; break;
			}
		} else { /* AGA mode HAM6 */
			int pw = pixdata.apixels[hdp];
			int pv = pw ^ bplxor;
			uae_u32 pc = ((pw & 0xf) << 0) | ((pw & 0xf) << 4);
			switch (pv & 0x30)
			{
				case 0x00: ham_lastcolor = colors_for_drawing.color_regs_aga[pv & 0x0f] & 0xffffff; break;
				case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= pc << 0; break;
				case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= pc << 16; break;
				case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= pc << 8; break;
			}
		}
	} else {
		if (!bpldualpf) {
			/* OCS/ECS mode HAM6 */
			int pv = pixdata.apixels[ham_decode_pixel];
			switch (pv & 0x30)
			{
				case 0x00: ham_lastcolor = colors_for_drawing.color_regs_ecs[pv] & 0xfff; break;
				case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
				case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
				case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
			}
		} else {
			/* OCS/ECS mode HAM6 + DPF */
			int pv = pixdata.apixels[ham_decode_pixel];
			int *lookup = bpldualpfpri ? dblpf_ind2 : dblpf_ind1;
			int idx = lookup[pv];
			switch (pv & 0x30)
			{
				case 0x00: ham_lastcolor = colors_for_drawing.color_regs_ecs[idx] & 0xfff; break;
				case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (idx & 0xF); break;
				case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (idx & 0xF) << 8; break;
				case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (idx & 0xF) << 4; break;
			}
		}
	}
}


/* Decode HAM in the invisible portion of the display (left of VISIBLE_LEFT_BORDER),
 * but don't draw anything in.  This is done to prepare HAM_LASTCOLOR for later,
 * when decode_ham runs.
 *
 */
static void init_ham_decoding(void)
{
	int unpainted_amiga = unpainted;

	ham_decode_pixel = src_pixel;
#if 0
	ham_decode_pixel = -hamleftborderhidden;
#endif
	ham_lastcolor = color_reg_get(&colors_for_drawing, 0);
	while (unpainted_amiga-- > 0) {
		decode_ham_pixel(ham_decode_pixel++);
	}
}

static void decode_ham(int pix, int stoppos, int blank)
{
	int todraw_amiga = res_shift_from_window(stoppos - pix);
	while (todraw_amiga-- > 0) {
		decode_ham_pixel(ham_decode_pixel);
		ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
	}
}

static void decode_ham_border(int pix, int stoppos, int blank)
{
	ham_lastcolor = color_reg_get(&colors_for_drawing, 0);
}

static void erase_ham_right_border(int pix, int stoppos, bool blank)
{
	if (stoppos < playfield_end)
		return;
	// erase right border in HAM modes or old HAM data may be visible
	// if DDFSTOP < DIWSTOP (Uridium II title screen)
	int todraw_amiga = res_shift_from_window (stoppos - pix);
	while (todraw_amiga-- > 0)
		ham_linebuf[ham_decode_pixel++] = 0;
}

static void gen_pfield_tables(void)
{
	int i;

	for (i = 0; i < 256; i++) {
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

		clxtab[i] = ((((i & 3) && (i & 12)) << 9)
			| (((i & 3) && (i & 48)) << 10)
			| (((i & 3) && (i & 192)) << 11)
			| (((i & 12) && (i & 48)) << 12)
			| (((i & 12) && (i & 192)) << 13)
			| (((i & 48) && (i & 192)) << 14));

	}

	memset(all_ones, 0xff, MAX_PIXELS_PER_LINE);

}

/* When looking at this function and the ones that inline it, bear in mind
what an optimizing compiler will do with this code.  All callers of this
function only pass in constant arguments (except for E).  This means
that many of the if statements will go away completely after inlining.  */
STATIC_INLINE void draw_sprites_1(struct sprite_entry *e, int dualpf, int has_attach)
{
	uae_u16 *buf = spixels + e->first_pixel;
	uae_u8 *stbuf = spixstate.stb + e->first_pixel;
	uae_u16 *stfmbuf = spixstate.stbfm + e->first_pixel;
	int spr_pos, pos;
	int epos = e->pos;
	int emax = e->max;

	buf -= epos;
	stbuf -= epos;
	stfmbuf -= epos;

	spr_pos = epos - ((DISPLAY_LEFT_SHIFT - DIW_DDF_OFFSET) << sprite_buffer_res);

	sprite_first_x = std::min(spr_pos, sprite_first_x);

	for (pos = epos; pos < emax; pos++, spr_pos++) {
		if (spr_pos >= 0 && spr_pos < MAX_PIXELS_PER_LINE) {
			struct spritepixelsbuf *sp = &spritepixels[spr_pos];
			sp->data = buf[pos];
			sp->stdata = stbuf[pos];
			sp->stfmdata = stfmbuf[pos];
			sp->flags = has_attach ? 1 : 0;
			// ECS superhires sprite odd/even bit
			sp->flags |= (pos & 2) ? 2 : 0;
		}
	}

	sprite_last_x = std::max(spr_pos, sprite_last_x);
}

/* See comments above.  Do not touch if you don't know what's going on.
* (We do _not_ want the following to be inlined themselves).  */
/* lores bitplane, lores sprites */
static void NOINLINE draw_sprites_normal_sp_nat(struct sprite_entry *e) { draw_sprites_1(e, 0, 0); }
static void NOINLINE draw_sprites_normal_dp_nat(struct sprite_entry *e) { draw_sprites_1(e, 1, 0); }
static void NOINLINE draw_sprites_normal_sp_at(struct sprite_entry *e) { draw_sprites_1(e, 0, 1); }
static void NOINLINE draw_sprites_normal_dp_at(struct sprite_entry *e) { draw_sprites_1(e, 1, 1); }

#ifdef AGA
/* not very optimized */
STATIC_INLINE void draw_sprites_aga(struct sprite_entry *e)
{
	draw_sprites_1(e, bpldualpf, e->has_attached);
}
#endif

STATIC_INLINE void draw_sprites_ecs(struct sprite_entry *e)
{
	if (e->has_attached) {
		if (bpldualpf)
			draw_sprites_normal_dp_at(e);
		else
			draw_sprites_normal_sp_at(e);
	} else {
		if (bpldualpf)
			draw_sprites_normal_dp_nat(e);
		else
			draw_sprites_normal_sp_nat(e);
	}
}

#ifdef AGA
/* clear possible bitplane data outside DIW area */
static void clear_bitplane_border_aga(void)
{
	int len, shift = res_shift;
	uae_u8 v = 0;

	if (shift < 0) {
		shift = -shift;
		len = (real_playfield_start - playfield_start) << shift;
		int offset = playfield_start << shift;
		memset(pixdata.apixels + pixels_offset + offset, v, len);
		if (bplham)
			memset(ham_linebuf + pixels_offset + offset, v, len * sizeof(uae_u32));

		len = (playfield_end - real_playfield_end) << shift;
		offset = real_playfield_end << shift;
		memset(pixdata.apixels + pixels_offset + offset, v, len);
		if (bplham)
			memset(ham_linebuf + pixels_offset + offset, v, len * sizeof(uae_u32));
	} else {
		len = (real_playfield_start - playfield_start) >> shift;
		int offset = playfield_start >> shift;
		memset(pixdata.apixels + pixels_offset + offset, v, len);
		if (bplham)
			memset(ham_linebuf + pixels_offset + offset, v, len * sizeof(uae_u32));

		len = (playfield_end - real_playfield_end) >> shift;
		offset = real_playfield_end >> shift;
		memset(pixdata.apixels + pixels_offset + offset, v, len);
		if (bplham)
			memset(ham_linebuf + pixels_offset + offset, v, len * sizeof(uae_u32));
	}
}
#endif

static void weird_bitplane_fix(int start, int end)
{
	uae_u8 *p = pixdata.apixels + pixels_offset;

	start = res_shift_from_window(start);
	end = res_shift_from_window(end);

	if (!bpldualpf) {
		// HAM is unaffected (probably because plane 5 is HAM control bit)
		if (bplham)
			return;
		if (bplplanecnt >= 5 && plf2pri >= 5) {
			// Emulate OCS/ECS only undocumented "SWIV" hardware feature:
			// PF2 >= 5 and bit in plane 5 set: other planes are ignored in color selection.
			for (int i = start; i < end; i++) {
				if (p[i] & 0x10)
					p[i] = 0x10;
			}
		}
	} else if (plf1pri >= 5 || plf2pri >= 5) {
		// If PFx is invalid (>=5), matching playfield's color becomes transparent
		// (COLOR00). Priorities keep working normally: "transparent" playfield
		// will still hide lower priority playfield behind it.
		// Logo in Running man / Scoopex
		uae_u8 mask1 = 0x01 | 0x04 | 0x10;
		uae_u8 mask2 = 0x02 | 0x08 | 0x20;
		for (int i = start; i < end; i++) {
			if (plf1pri >= 5 && (p[i] & mask1))
				p[i] |= 0x40;
			if (plf2pri >= 5 && (p[i] & mask2))
				p[i] |= 0x80;
		}
	}
}

/* We use the compiler's inlining ability to ensure that PLANES is in effect a compile time
constant.  That will cause some unnecessary code to be optimized away.
Don't touch this if you don't know what you are doing.  */

#define MERGE(a,b,mask,shift) do {\
	uae_u32 tmp = mask & (a ^ (b >> shift)); \
	a ^= tmp; \
	b ^= (tmp << shift); \
} while (0)

#define MERGE64(a,b,mask,shift) do {\
	uae_u64 tmp = mask & (a ^ (b >> shift)); \
	a ^= tmp; \
	b ^= (tmp << shift); \
} while (0)


#define GETLONG32(P) (*(uae_u32*)P)
#define GETLONG64(P) (*(uae_u64*)P)

STATIC_INLINE void pfield_doline32_1(uae_u32 *pixels, int wordcount, int planes, uae_u8 *real_bplpt[8])
{
	while (wordcount-- > 0) {
		uae_u32 b0, b1, b2, b3, b4, b5, b6, b7;

		b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;
		switch (planes) {
#ifdef AGA
		case 8: b0 = GETLONG32(real_bplpt[7]); real_bplpt[7] += 4;
		case 7: b1 = GETLONG32(real_bplpt[6]); real_bplpt[6] += 4;
#endif
		case 6: b2 = GETLONG32(real_bplpt[5]); real_bplpt[5] += 4;
		case 5: b3 = GETLONG32(real_bplpt[4]); real_bplpt[4] += 4;
		case 4: b4 = GETLONG32(real_bplpt[3]); real_bplpt[3] += 4;
		case 3: b5 = GETLONG32(real_bplpt[2]); real_bplpt[2] += 4;
		case 2: b6 = GETLONG32(real_bplpt[1]); real_bplpt[1] += 4;
		case 1: b7 = GETLONG32(real_bplpt[0]); real_bplpt[0] += 4;
		}

		MERGE(b0, b1, 0x55555555, 1);
		MERGE(b2, b3, 0x55555555, 1);
		MERGE(b4, b5, 0x55555555, 1);
		MERGE(b6, b7, 0x55555555, 1);

		MERGE(b0, b2, 0x33333333, 2);
		MERGE(b1, b3, 0x33333333, 2);
		MERGE(b4, b6, 0x33333333, 2);
		MERGE(b5, b7, 0x33333333, 2);

		MERGE(b0, b4, 0x0f0f0f0f, 4);
		MERGE(b1, b5, 0x0f0f0f0f, 4);
		MERGE(b2, b6, 0x0f0f0f0f, 4);
		MERGE(b3, b7, 0x0f0f0f0f, 4);

		MERGE(b0, b1, 0x00ff00ff, 8);
		MERGE(b2, b3, 0x00ff00ff, 8);
		MERGE(b4, b5, 0x00ff00ff, 8);
		MERGE(b6, b7, 0x00ff00ff, 8);

		MERGE(b0, b2, 0x0000ffff, 16);
		do_put_mem_long(pixels + 0, b0);
		do_put_mem_long(pixels + 4, b2);
		MERGE(b1, b3, 0x0000ffff, 16);
		do_put_mem_long(pixels + 2, b1);
		do_put_mem_long(pixels + 6, b3);
		MERGE(b4, b6, 0x0000ffff, 16);
		do_put_mem_long(pixels + 1, b4);
		do_put_mem_long(pixels + 5, b6);
		MERGE(b5, b7, 0x0000ffff, 16);
		do_put_mem_long(pixels + 3, b5);
		do_put_mem_long(pixels + 7, b7);
		pixels += 8;
	}
}


STATIC_INLINE void pfield_doline64_1(uae_u64 *pixels, int wordcount, int planes, uae_u8 *real_bplpt[8])
{
	while (wordcount-- > 0) {
		uae_u64 b0, b1, b2, b3, b4, b5, b6, b7;

		b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;
		switch (planes) {
#ifdef AGA
		case 8: b0 = GETLONG64(real_bplpt[7]); real_bplpt[7] += 8;
		case 7: b1 = GETLONG64(real_bplpt[6]); real_bplpt[6] += 8;
#endif
		case 6: b2 = GETLONG64(real_bplpt[5]); real_bplpt[5] += 8;
		case 5: b3 = GETLONG64(real_bplpt[4]); real_bplpt[4] += 8;
		case 4: b4 = GETLONG64(real_bplpt[3]); real_bplpt[3] += 8;
		case 3: b5 = GETLONG64(real_bplpt[2]); real_bplpt[2] += 8;
		case 2: b6 = GETLONG64(real_bplpt[1]); real_bplpt[1] += 8;
		case 1: b7 = GETLONG64(real_bplpt[0]); real_bplpt[0] += 8;
		}

		MERGE64(b0, b1, 0x5555555555555555, 1);
		MERGE64(b2, b3, 0x5555555555555555, 1);
		MERGE64(b4, b5, 0x5555555555555555, 1);
		MERGE64(b6, b7, 0x5555555555555555, 1);

		MERGE64(b0, b2, 0x3333333333333333, 2);
		MERGE64(b1, b3, 0x3333333333333333, 2);
		MERGE64(b4, b6, 0x3333333333333333, 2);
		MERGE64(b5, b7, 0x3333333333333333, 2);

		MERGE64(b0, b4, 0x0f0f0f0f0f0f0f0f, 4);
		MERGE64(b1, b5, 0x0f0f0f0f0f0f0f0f, 4);
		MERGE64(b2, b6, 0x0f0f0f0f0f0f0f0f, 4);
		MERGE64(b3, b7, 0x0f0f0f0f0f0f0f0f, 4);

		MERGE64(b0, b1, 0x00ff00ff00ff00ff, 8);
		MERGE64(b2, b3, 0x00ff00ff00ff00ff, 8);
		MERGE64(b4, b5, 0x00ff00ff00ff00ff, 8);
		MERGE64(b6, b7, 0x00ff00ff00ff00ff, 8);

		MERGE64(b0, b2, 0x0000ffff0000ffff, 16);
		do_put_mem_quad(pixels + 0, b0);
		do_put_mem_quad(pixels + 4, b2);
		MERGE64(b1, b3, 0x0000ffff0000ffff, 16);
		do_put_mem_quad(pixels + 2, b1);
		do_put_mem_quad(pixels + 6, b3);
		MERGE64(b4, b6, 0x0000ffff0000ffff, 16);
		do_put_mem_quad(pixels + 1, b4);
		do_put_mem_quad(pixels + 5, b6);
		MERGE64(b5, b7, 0x0000ffff0000ffff, 16);
		do_put_mem_quad(pixels + 3, b5);
		do_put_mem_quad(pixels + 7, b7);
		pixels += 8;
	}
}

/* See above for comments on inlining.  These functions should _not_
be inlined themselves.  */
static void NOINLINE pfield_doline32_n1(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_1(data, count, 1, real_bplpt); }
static void NOINLINE pfield_doline32_n2(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_1(data, count, 2, real_bplpt); }
static void NOINLINE pfield_doline32_n3(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_1(data, count, 3, real_bplpt); }
static void NOINLINE pfield_doline32_n4(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_1(data, count, 4, real_bplpt); }
static void NOINLINE pfield_doline32_n5(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_1(data, count, 5, real_bplpt); }
static void NOINLINE pfield_doline32_n6(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_1(data, count, 6, real_bplpt); }
#ifdef AGA
static void NOINLINE pfield_doline32_n7(uae_u32 *data, int count, uae_u8* real_bplpt[8]) { pfield_doline32_1(data, count, 7, real_bplpt); }
static void NOINLINE pfield_doline32_n8(uae_u32 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline32_1(data, count, 8, real_bplpt); }
#endif

static void NOINLINE pfield_doline64_n1(uae_u64 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline64_1(data, count, 1, real_bplpt); }
static void NOINLINE pfield_doline64_n2(uae_u64 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline64_1(data, count, 2, real_bplpt); }
static void NOINLINE pfield_doline64_n3(uae_u64 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline64_1(data, count, 3, real_bplpt); }
static void NOINLINE pfield_doline64_n4(uae_u64 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline64_1(data, count, 4, real_bplpt); }
static void NOINLINE pfield_doline64_n5(uae_u64 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline64_1(data, count, 5, real_bplpt); }
static void NOINLINE pfield_doline64_n6(uae_u64 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline64_1(data, count, 6, real_bplpt); }
#ifdef AGA
static void NOINLINE pfield_doline64_n7(uae_u64 *data, int count, uae_u8 *real_bplpt[8]) { pfield_doline64_1(data, count, 7, real_bplpt); }
static void NOINLINE pfield_doline64_n8(uae_u64 *data, int count, uae_u8* real_bplpt[8]) { pfield_doline64_1(data, count, 8, real_bplpt); }
#endif

static void pfield_doline(int lineno)
{
	uae_u8 *real_bplpt[8];
	int offset = 0; // currprefs.chipset_hr ? 8 : 0;

#if 0
	int wordcount = (dp_for_drawing->plflinelen + 1) / 2;
	uae_u64 *data = pixdata.apixels_q + MAX_PIXELS_PER_LINE / sizeof(uae_u64);

#define DATA_POINTER(n) ((debug_bpl_mask & (1 << n)) ? (line_data[lineno] + (n) * MAX_WORDS_PER_LINE * 2) : (debug_bpl_mask_one ? all_ones : all_zeros))
	real_bplpt[0] = DATA_POINTER(0);
	real_bplpt[1] = DATA_POINTER(1);
	real_bplpt[2] = DATA_POINTER(2);
	real_bplpt[3] = DATA_POINTER(3);
	real_bplpt[4] = DATA_POINTER(4);
	real_bplpt[5] = DATA_POINTER(5);
#ifdef AGA
	real_bplpt[6] = DATA_POINTER(6);
	real_bplpt[7] = DATA_POINTER(7);
#endif

	switch (bplmaxplanecnt) {
	default: break;
	case 0: memset(data, 0, wordcount * 64); break;
	case 1: pfield_doline64_n1(data, wordcount, real_bplpt); break;
	case 2: pfield_doline64_n2(data, wordcount, real_bplpt); break;
	case 3: pfield_doline64_n3(data, wordcount, real_bplpt); break;
	case 4: pfield_doline64_n4(data, wordcount, real_bplpt); break;
	case 5: pfield_doline64_n5(data, wordcount, real_bplpt); break;
	case 6: pfield_doline64_n6(data, wordcount, real_bplpt); break;
#ifdef AGA
	case 7: pfield_doline64_n7(data, wordcount, real_bplpt); break;
	case 8: pfield_doline64_n8(data, wordcount, real_bplpt); break;
#endif
	}
#else
	int wordcount = dp_for_drawing->plflinelen;
	uae_u32 *data = pixdata.apixels_l + MAX_PIXELS_PER_LINE / sizeof(uae_u32);

#define DATA_POINTER(n) ((debug_bpl_mask & (1 << n)) ? (line_data[lineno] + (n) * MAX_WORDS_PER_LINE * 2 + offset) : (debug_bpl_mask_one ? all_ones : all_zeros))
	real_bplpt[0] = DATA_POINTER(0);
	real_bplpt[1] = DATA_POINTER(1);
	real_bplpt[2] = DATA_POINTER(2);
	real_bplpt[3] = DATA_POINTER(3);
	real_bplpt[4] = DATA_POINTER(4);
	real_bplpt[5] = DATA_POINTER(5);
#ifdef AGA
	real_bplpt[6] = DATA_POINTER(6);
	real_bplpt[7] = DATA_POINTER(7);
#endif

	switch (bplmaxplanecnt) {
	default: break;
	case 0: memset(data, 0, wordcount * 32); break;
	case 1: pfield_doline32_n1(data, wordcount, real_bplpt); break;
	case 2: pfield_doline32_n2(data, wordcount, real_bplpt); break;
	case 3: pfield_doline32_n3(data, wordcount, real_bplpt); break;
	case 4: pfield_doline32_n4(data, wordcount, real_bplpt); break;
	case 5: pfield_doline32_n5(data, wordcount, real_bplpt); break;
	case 6: pfield_doline32_n6(data, wordcount, real_bplpt); break;
#ifdef AGA
	case 7: pfield_doline32_n7(data, wordcount, real_bplpt); break;
	case 8: pfield_doline32_n8(data, wordcount, real_bplpt); break;
#endif
	}
#endif

	if (refresh_indicator_buffer && refresh_indicator_height > lineno) {
		uae_u8 *opline = refresh_indicator_buffer + lineno * MAX_PIXELS_PER_LINE * 2;
		wordcount *= 32;
		if (!memcmp(opline, data, wordcount)) {
			if (refresh_indicator_changed[lineno] != 0xff) {
				refresh_indicator_changed[lineno]++;
				refresh_indicator_changed_prev[lineno] = std::max(refresh_indicator_changed[lineno],
				                                                  refresh_indicator_changed_prev[lineno]);
			}
		} else {
			memcpy(opline, data, wordcount);
			if (refresh_indicator_changed[lineno] != refresh_indicator_changed_prev[lineno])
				refresh_indicator_changed_prev[lineno] = 0;
			refresh_indicator_changed[lineno] = 0;
		}
	}
}

void init_row_map(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	static uae_u8 *oldbufmem;
	static int oldheight, oldpitch;
	static bool oldgenlock, oldburst;
	int i, j;

	if (vidinfo->drawbuffer.height_allocated > max_uae_height) {
		write_log (_T("Resolution too high, aborting\n"));
		abort ();
	}
	if (!row_map) {
		row_map = xmalloc(uae_u8*, max_uae_height + 1);
		row_map_genlock = xmalloc(uae_u8*, max_uae_height + 1);
	}

	if (oldbufmem && oldbufmem == vidinfo->drawbuffer.bufmem &&
		oldheight == vidinfo->drawbuffer.height_allocated &&
		oldpitch == vidinfo->drawbuffer.rowbytes &&
		oldgenlock == init_genlock_data &&
		oldburst == (row_map_color_burst_buffer ? 1 : 0))
		return;
	xfree(row_map_genlock_buffer);
	row_map_genlock_buffer = NULL;
	if (init_genlock_data) {
		row_map_genlock_buffer = xcalloc(uae_u8, vidinfo->drawbuffer.width_allocated * (vidinfo->drawbuffer.height_allocated + 2));
	}
	xfree(row_map_color_burst_buffer);
	row_map_color_burst_buffer = NULL;
	if (currprefs.cs_color_burst) {
		row_map_color_burst_buffer = xcalloc(uae_u8, vidinfo->drawbuffer.height_allocated + 2);
	}
	j = oldheight == 0 ? max_uae_height : oldheight;
	for (i = vidinfo->drawbuffer.height_allocated; i < max_uae_height + 1 && i < j + 1; i++) {
		row_map[i] = row_tmp;
		row_map_genlock[i] = row_tmp;
	}
	for (i = 0, j = 0; i < vidinfo->drawbuffer.height_allocated; i++, j += vidinfo->drawbuffer.rowbytes) {
		row_map[i] = vidinfo->drawbuffer.bufmem + j;
		if (init_genlock_data) {
			row_map_genlock[i] = row_map_genlock_buffer + vidinfo->drawbuffer.width_allocated * (i + 1);
		} else {
			row_map_genlock[i] = NULL;
		}
	}
	oldbufmem = vidinfo->drawbuffer.bufmem;
	oldheight = vidinfo->drawbuffer.height_allocated;
	oldpitch = vidinfo->drawbuffer.rowbytes;
	oldgenlock = init_genlock_data;
	oldburst = row_map_color_burst_buffer ? 1 : 0;
}

static void init_aspect_maps(void)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
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

	h = vidinfo->drawbuffer.height_allocated;
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

static void setbplmode(void)
{
	if (bplham)
		bplmode = CMODE_HAM;
	else if (bpldualpf)
		bplmode = CMODE_DUALPF;
	else if (bplehb > 0)
		bplmode = CMODE_EXTRAHB;
	else if (bplehb < 0)
		bplmode = CMODE_EXTRAHB_ECS_KILLEHB;
	else
		bplmode = CMODE_NORMAL;
}

static void set_sprite_visibility(void)
{
	sprite_visibility = true;
	if (exthblank || exthblank_force) {
		sprite_visibility = false;
	}
	if (extborder && (ce_is_borderblank(colors_for_drawing.extra) || !ce_is_bordersprite(colors_for_drawing.extra))) {
		sprite_visibility = false;
	}
}

/* We only save hardware registers during the hardware frame. Now, when
* drawing the frame, we expand the data into a slightly more useful
* form. */
static void pfield_expand_dp_bplcon(void)
{
	bool pfield_mode_changed = false;

	bplres = dp_for_drawing->bplres;
	bplplanecnt = dp_for_drawing->nr_planes;
	bplham = dp_for_drawing->ham_seen;
	bplehb = dp_for_drawing->ehb_seen && ehb_enable;
	if (ecs_denise) {
		// Check for KillEHB bit in ECS/AGA
		if (dp_for_drawing->bplcon2 & 0x0200) {
			bplehb = 0;
			if (!aga_mode)
				bplehb = -1;
		}
	} else if (denisea1000_noehb) {
		bplehb = -1;
	}
	bplcolorburst = (dp_for_drawing->bplcon0 & 0x200) != 0;
	if (!bplcolorburst)
		bplcolorburst_field = 0;
#ifdef ECS_DENISE
	int oecsshres = ecsshres;
	ecsshres = bplres == RES_SUPERHIRES && ecs_denise && !aga_mode && (dp_for_drawing->bplcon0 & 0x40);
	pfield_mode_changed = oecsshres != ecsshres;
#endif

	plf1pri = dp_for_drawing->bplcon2 & 7;
	plf2pri = (dp_for_drawing->bplcon2 >> 3) & 7;
	plf_sprite_mask = 0xFFFF0000 << (4 * plf2pri);
	plf_sprite_mask |= (0x0000FFFF << (4 * plf1pri)) & 0xFFFF;
	bpldualpf = (dp_for_drawing->bplcon0 & 0x400) == 0x400;
	bpldualpfpri = (dp_for_drawing->bplcon2 & 0x40) == 0x40;

#ifdef AGA
	// BYPASS: HAM and EHB select bits are ignored
	if (bplbypass != (dp_for_drawing->bplcon0 & 0x20) != 0) {
		bpland = 0xff;
		bplbypass = (dp_for_drawing->bplcon0 & 0x20) != 0;
		pfield_mode_changed = true;
	}
	if (bplbypass) {
		if (bplham && bplplanecnt == 6)
			bpland = 0x0f;
		if (bplham && bplplanecnt == 8)
			bpland = 0xfc;
		bplham = 0;
		if (bplehb)
			bpland = 31;
		bplehb = 0;
	}
	bpldualpf2of = (dp_for_drawing->bplcon3 >> 10) & 7;
	sbasecol[0] = ((dp_for_drawing->bplcon4sp >> 4) & 15) << 4;
	sbasecol[1] = ((dp_for_drawing->bplcon4sp >> 0) & 15) << 4;
	bplxor = dp_for_drawing->bplcon4bm >> 8;
	int sh = (colors_for_drawing.extra >> CE_SHRES_DELAY_SHIFT) & 3;
	if (sh != bpldelay_sh) {
		bpldelay_sh = sh;
		pfield_mode_changed = true;
	}
	if (sprite_smaller_than_64 && (dp_for_drawing->fmode & 0x0c) == 0x0c)
		sprite_smaller_than_64_inuse = true;
	sprite_smaller_than_64 = (dp_for_drawing->fmode & 0x0c) != 0x0c;
#endif
	ecs_genlock_features_active = (ecs_denise && ((dp_for_drawing->bplcon2 & 0x0c00) || ce_is_borderntrans(colors_for_drawing.extra))) ||
		(currprefs.genlock_effects ? 1 : 0) || (aga_mode && (dp_for_drawing->bplcon3 & 0x004) && (dp_for_drawing->bplcon0 & 1));
	if (ecs_genlock_features_active) {
		ecs_genlock_features_colorkey = currprefs.ecs_genlock_features_colorkey_mask[0] || currprefs.ecs_genlock_features_colorkey_mask[1] ||
			currprefs.ecs_genlock_features_colorkey_mask[2] || currprefs.ecs_genlock_features_colorkey_mask[3];
		ecs_genlock_features_mask = currprefs.ecs_genlock_features_plane_mask;
		aga_genlock_features_zdclken = false;
		if (dp_for_drawing->bplcon2 & 0x0800) {
			ecs_genlock_features_mask = 1 << ((dp_for_drawing->bplcon2 >> 12) & 7);
		}
		if (dp_for_drawing->bplcon2 & 0x0400) {
			ecs_genlock_features_colorkey = true;
		}
		if ((dp_for_drawing->bplcon3 & 0x0004) && (dp_for_drawing->bplcon0 & 1)) {
			aga_genlock_features_zdclken = true;
		}
	}

	if (pfield_mode_changed)
		pfield_set_linetoscr();
	
	setbplmode();
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

static void pfield_expand_dp_bplconx (int regno, int v, int hp, int vp)
{
	regno -= RECORDED_REGISTER_CHANGE_OFFSET;
	switch (regno)
	{
	case 0xffff - RECORDED_REGISTER_CHANGE_OFFSET:
		return;
	case 0x100: // BPLCON0
		dp_for_drawing->bplcon0 = v;
		dp_for_drawing->bplres = GET_RES_DENISE(v);
		dp_for_drawing->nr_planes = GET_PLANES(v);
		dp_for_drawing->ham_seen = isham(v);
		if (currprefs.chipset_hr && dp_for_drawing->bplres < currprefs.gfx_resolution)
			dp_for_drawing->bplres = currprefs.gfx_resolution;
		extblankcheck();
		break;
	case 0x101: // BPLCON0 partial
		dp_for_drawing->bplcon0 &= ~(0x0800 | 0x0400 | 0x0080 | 0x0001);
		dp_for_drawing->bplcon0 |= v & (0x0800 | 0x0400 | 0x0080 | 0x0001);
		dp_for_drawing->ham_seen = isham(v);
		extblankcheck();
		break;
	case 0x201: // AGA EHB immediate change
		ehb_enable = (v & 0x7010) == 0x6000;
		break;
	case 0x104: // BPLCON2
		dp_for_drawing->bplcon2 = v;
		break;
#ifdef ECS_DENISE
	case 0x106: // BPLCON3
		dp_for_drawing->bplcon3 = v;
		extblankcheck();
		break;
#endif
#ifdef AGA
	case 0x10c: // BPLCON4 bitplane xor (and sprite if sprite change is not visible)
		dp_for_drawing->bplcon4bm = v;
		dp_for_drawing->bplcon4sp = v;
		break;
	case 0x10c+1: // BPLCON4 sprite bank
		dp_for_drawing->bplcon4sp = v;
		break;
	case 0x1fc: // FMODE
		dp_for_drawing->fmode = v;
		break;
	case 0x200: // hblank
		if (v) {
			exthblanken = true;
			if (vp >= 0) {
				extblankcheck();
			} else {
				exthblank = exthblank_set;
			}
		} else {
			exthblanken = false;
			exthblank = 0;
		}
		set_sprite_visibility();
		return;
	case 0x208: // forced hblank
		if (v) {
			exthblanken = true;
			exthblankon = true;
		} else {
			exthblanken = false;
			exthblank = 0;
		}
		return;
	case 0x202: // hsync (debug)
		hsync_debug = v;
		return;
	case 0x204: // hblank (debug)
		hblank_debug = v;
		return;
	case 0x206: // hcenter (denug)
		hcenter_debug = v;
		return;
#endif
	}
	pfield_expand_dp_bplcon();
	set_res_shift();
}

static int drawing_color_matches;
static enum { color_match_acolors, color_match_full } color_match_type;

/* Set up colors_for_drawing to the state at the beginning of the currently drawn
line.  Try to avoid copying color tables around whenever possible.  */
static void adjust_drawing_colors (int ctable, int need_full, bool blankcheck)
{
	uae_u16 oe = colors_for_drawing.extra;
	if (drawing_color_matches != ctable || need_full < 0) {
		if (need_full) {
			color_reg_cpy (&colors_for_drawing, curr_color_tables + ctable);
			color_match_type = color_match_full;
		} else {
			if (aga_mode) {
				memcpy(colors_for_drawing.acolors, curr_color_tables[ctable].acolors, sizeof(xcolnr) * 256);
			} else {
				memcpy(colors_for_drawing.acolors, curr_color_tables[ctable].acolors, sizeof(xcolnr) * 32);
			}
			colors_for_drawing.extra = curr_color_tables[ctable].extra;
			color_match_type = color_match_acolors;
		}
		drawing_color_matches = ctable;
	} else if (need_full && color_match_type != color_match_full) {
		color_reg_cpy (&colors_for_drawing, &curr_color_tables[ctable]);
		color_match_type = color_match_full;
	}
	if (colors_for_drawing.extra != oe) {
		reset_hblanking_limits();
		set_hblanking_limits();
		expand_vb_state();
	}
}

static void playfield_hard_way(line_draw_func worker_pfield, int first, int last)
{
	int stop = last < real_playfield_end ? last : real_playfield_end;

	src_pixel += playfield_diff;
	ham_decode_pixel += playfield_diff;

	if (first < real_playfield_start)  {
		int next = last < real_playfield_start ? last : real_playfield_start;
		// left border sprite
		pfield_do_linetoscr_bordersprite_aga(first, next, false);
		// bitplanes
		if (stop > real_playfield_start) {
			(*worker_pfield)(real_playfield_start, stop, false);
			// right border sprite
			if (last > real_playfield_end) {
				int sfirst = first > real_playfield_end ? first : real_playfield_end;
				pfield_do_linetoscr_bordersprite_aga(sfirst, last, false);
			}
		}
	} else {
		// bitplanes
		if (stop > real_playfield_start) {
			(*worker_pfield)(first, stop, false);
			// right border sprite
			if (last > real_playfield_end) {
				int sfirst = first > real_playfield_end ? first : real_playfield_end;
				pfield_do_linetoscr_bordersprite_aga(sfirst, last, false);
			}
		}
	}

	src_pixel -= playfield_diff;
	ham_decode_pixel -= playfield_diff;
}

static void do_color_changes(line_draw_func worker_border, line_draw_func worker_pfield, int vp)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	int lastpos = visible_left_border;
	int lastpos2 = lastpos;
	int endpos = visible_left_border + vidinfo->drawbuffer.inwidth;
	bool vbarea = vp < vblank_top_start || vp >= vblank_bottom_stop;
	bool playfield_first = true;

	extborder = false; // reset here because it always have start and end in same scanline
	hcenter_debug = 0;
	if (!ecs_denise) {
		// used for OCS Denise blanking bug when not ECS Denise or AGA.
		exthblank = 0;
	}
	ehb_enable = true;
	set_sprite_visibility();

	for (int i = dip_for_drawing->first_color_change; i <= dip_for_drawing->last_color_change; i++) {
		int regno = curr_color_changes[i].regno;
		uae_u32 value = curr_color_changes[i].value;
		int nextpos, nextpos_in_range;

		if (i == dip_for_drawing->last_color_change) {
			nextpos = endpos;
		} else {
			nextpos = shres_coord_hw_to_window_x(curr_color_changes[i].linepos);
		}

		nextpos_in_range = nextpos;
		nextpos_in_range = std::min(nextpos_in_range, endpos);

		if (vp >= 0) {

			if (full_blank || vbarea) {
				// vblank + programmed vblank / hardwired vblank

				hposblank = 3;
				if (nextpos_in_range > lastpos) {
					int t = nextpos_in_range;
					(*worker_border)(lastpos, t, 1);
					lastpos = t;
				}

			} else {
				// non-vblank scanline

				int hblank_left = exthblankon ? hblank_left_start : hblank_left_start_hard;

				// left hblank (left edge to hblank end)
				if (nextpos_in_range > lastpos && lastpos < hblank_left) {
					int t = nextpos_in_range <= hblank_left ? nextpos_in_range : hblank_left;
					(*worker_border)(lastpos, t, 1);
					lastpos = t;
				}

				if (playfield_start_pre >= playfield_start || !ce_is_borderblank(colors_for_drawing.extra)) {

					// normal left border (hblank end to playfield start)
					if (nextpos_in_range > lastpos && lastpos < playfield_start) {
						int t = nextpos_in_range <= playfield_start ? nextpos_in_range : playfield_start;
						(*worker_border)(lastpos, t, 0);
						lastpos = t;
					}

					// playfield
					if (nextpos_in_range > lastpos && lastpos >= playfield_start && lastpos < playfield_end) {
						int t = nextpos_in_range <= playfield_end ? nextpos_in_range : playfield_end;
						// always draw bitplanes from start position, bitplanes can start during hblank
						if (playfield_first && lastpos > playfield_start) {
							lastpos = playfield_start;
						}
						playfield_first = false;
						if ((plf2pri >= 5 || plf1pri >= 5) && !aga_mode) {
							weird_bitplane_fix(lastpos, t);
						}
						if (may_require_hard_way && (may_require_hard_way < 0 || (bplxor && may_require_hard_way && worker_pfield != pfield_do_linetoscr_bordersprite_aga))) {
							playfield_hard_way(worker_pfield, lastpos, t);
						} else {
							(*worker_pfield)(lastpos, t, 0);
						}
						// playfield started inside hblank? Overwrite it with blank.
						if (playfield_start < hblank_left) {
							(*worker_border)(playfield_start, hblank_left, 1);
						}
						lastpos = t;
					}

				} else {
					// special AGA borderblank 1 shres pixel delay

					// borderblank left border (hblank end to playfield_start_pre)
					if (nextpos_in_range > lastpos && lastpos < playfield_start_pre) {
						int t = nextpos_in_range <= playfield_start_pre ? nextpos_in_range : playfield_start_pre;
						(*worker_border)(lastpos, t, 0);
						lastpos = t;
					}
					// AGA "buggy" borderblank, real background color visible, single shres pixel wide.
					if (nextpos_in_range > lastpos && lastpos < playfield_start) {
						int t = nextpos_in_range <= playfield_start ? nextpos_in_range : playfield_start;
						(*worker_border)(lastpos, t, -1);
						lastpos = t;
					}

					// playfield with last shres pixel not drawn.
					if (nextpos_in_range > lastpos && lastpos >= playfield_start && lastpos < playfield_end_pre) {
						int t = nextpos_in_range <= playfield_end_pre ? nextpos_in_range : playfield_end_pre;
						if (may_require_hard_way && (may_require_hard_way < 0 || (bplxor && may_require_hard_way && worker_pfield != pfield_do_linetoscr_bordersprite_aga))) {
							playfield_hard_way(worker_pfield, lastpos, t);
						} else {
							(*worker_pfield)(lastpos, t, 0);
						}
						lastpos = t;
					}

					// last shres pixel of playfield blanked
					if (nextpos_in_range > lastpos && lastpos >= playfield_end_pre && lastpos < playfield_end) {
						int t = nextpos_in_range <= playfield_end ? nextpos_in_range : playfield_end;
						(*worker_border)(lastpos, t, 0);
						lastpos = t;
					}
				}

				int hblank_right = exthblankon ? hblank_right_stop : hblank_right_stop_hard;

				// right border (playfield end to hblank start)
				if (nextpos_in_range > lastpos && lastpos >= playfield_end_pre && lastpos < hblank_right) {
					int t = nextpos_in_range <= hblank_right ? nextpos_in_range : hblank_right;
					(*worker_border)(lastpos, t, 0);
					lastpos = t;
				}

				// right hblank (hblank start to right edge, hblank start may be earlier than playfield end)
				if (nextpos_in_range > hblank_right) {
					(*worker_border)(hblank_right, nextpos_in_range, 1);
					lastpos = nextpos_in_range;
				}
			}

			if (syncdebug) {
				if (hblank_debug || vblank_debug || hsync_debug || vsync_debug || hcenter_debug || exthblank || ce_is_borderblank(colors_for_drawing.extra)) {
					pfield_do_darken_line(lastpos2, nextpos_in_range, vp);
				}
				lastpos2 = std::max(nextpos_in_range, lastpos2);
			}
		}

		if (i != dip_for_drawing->last_color_change) {
			if (regno >= RECORDED_REGISTER_CHANGE_OFFSET) {
				pfield_expand_dp_bplconx(regno, value, nextpos, vp);
			} else if (regno >= 0 && !(value & COLOR_CHANGE_MASK)) {
				color_reg_set(&colors_for_drawing, regno, value);
				colors_for_drawing.acolors[regno] = getxcolor(value);
			} else if (regno == 0 && (value & COLOR_CHANGE_MASK)) {
				if ((value & COLOR_CHANGE_MASK) == COLOR_CHANGE_ACTBORDER) {
					if (value & 2) {
						if (value & 1) {
							exthblank_force = true;
						} else {
							exthblank_force = false;
						}
					} else {
						if (value & 1) {
							extborder = true;
						} else {
							extborder = false;
						}
					}
					set_sprite_visibility();
				} else if (value & COLOR_CHANGE_BLANK) {
					if (value & 1) {
						exthblank = exthblank_set;
					} else {
						exthblank = 0;
						// so that OCS Denise blanking bug is marked in debug mode
						hblank_debug = false;
						vblank_debug = false;
					}
				} else if (value & COLOR_CHANGE_BRDBLANK) {
					colors_for_drawing.extra &= ~(1 << CE_BORDERBLANK);
					colors_for_drawing.extra &= ~(1 << CE_BORDERNTRANS);
					colors_for_drawing.extra &= ~(1 << CE_BORDERSPRITE);
					colors_for_drawing.extra &= ~(1 << CE_EXTBLANKSET);
					colors_for_drawing.extra |= (value & 1) != 0 ? (1 << CE_BORDERBLANK) : 0;
					colors_for_drawing.extra |= (value & 3) == 2 ? (1 << CE_BORDERSPRITE) : 0;
					colors_for_drawing.extra |= (value & 5) == 4 ? (1 << CE_BORDERNTRANS) : 0;
					colors_for_drawing.extra |= (value & 8) == 8 ? (1 << CE_EXTBLANKSET) : 0;
					set_sprite_visibility();
				} else if (value & COLOR_CHANGE_SHRES_DELAY) {
					colors_for_drawing.extra &= ~(1 << CE_SHRES_DELAY_SHIFT);
					colors_for_drawing.extra &= ~(1 << (CE_SHRES_DELAY_SHIFT + 1));
					colors_for_drawing.extra |= (value & 3) << CE_SHRES_DELAY_SHIFT;
					pfield_expand_dp_bplcon();
				}
			}
		}
	}
}

STATIC_INLINE bool is_color_changes(struct draw_info *di)
{
	int regno = curr_color_changes[di->first_color_change].regno;
	int changes = di->nr_color_changes;
	return changes > 1 || (changes == 1 && regno != 0xffff && regno != -1);
}

enum double_how {
	dh_buf,
	dh_line,
	dh_emerg
};

static void pfield_draw_line(struct vidbuffer *vb, int lineno, int gfx_ypos, int follow_ypos)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	static int warned = 0;
	int border = 0;
	int do_double = 0;
	bool have_color_changes;
	enum double_how dh;
	int ls = linestate[lineno];

	dp_for_drawing = line_decisions + lineno;
	dip_for_drawing = curr_drawinfo + lineno;

	if (dp_for_drawing->plfleft >= 0) {
		lines_count++;
		resolution_count[dp_for_drawing->bplres]++;
	}

	switch (ls)
	{
	case LINE_REMEMBERED_AS_PREVIOUS:
//		if (!warned) // happens when program messes up with VPOSW
//			write_log (_T("Shouldn't get here... this is a bug.\n")), warned++;
		return;

	case LINE_BLACK:
		linestate[lineno] = LINE_REMEMBERED_AS_BLACK;
		border = -1;
		break;

	case LINE_REMEMBERED_AS_BLACK:
		return;

	case LINE_AS_PREVIOUS:
		dp_for_drawing--;
		dip_for_drawing--;
		linestate[lineno] = LINE_DONE_AS_PREVIOUS;
		if (dp_for_drawing->plfleft < 0)
			border = 1;
		break;

	case LINE_DONE_AS_PREVIOUS:
		/* fall through */
	case LINE_DONE:
		return;

	case LINE_DECIDED_DOUBLE:
		if (follow_ypos >= 0) {
			do_double = 1;
			linestate[lineno + 1] = LINE_DONE_AS_PREVIOUS;
		}

		/* fall through */
	default:
		if (dp_for_drawing->plfleft < 0) {
			border = 1;
		}
		linestate[lineno] = LINE_DONE;
		break;
	}

	have_color_changes = is_color_changes(dip_for_drawing);
	if (vb_state != dp_for_drawing->vb) {
		vb_state = dp_for_drawing->vb;
		expand_vb_state();
	}
	sprite_smaller_than_64_inuse = false;

	dh = dh_line;
	xlinebuffer = vidinfo->drawbuffer.linemem;
	if (xlinebuffer == 0 && do_double
		&& (border == 0 || have_color_changes))
		xlinebuffer = vidinfo->drawbuffer.emergmem, dh = dh_emerg;
	if (xlinebuffer == 0)
		xlinebuffer = row_map[gfx_ypos], dh = dh_buf;
	xlinebuffer -= linetoscr_x_adjust_pixbytes;
	xlinebuffer_genlock = row_map_genlock[gfx_ypos] - linetoscr_x_adjust_pixels;

	if (row_map_color_burst_buffer)
		row_map_color_burst_buffer[gfx_ypos] = bplcolorburst;

	if (border == 0) {

		pfield_expand_dp_bplcon();
		// must be after pfield_expand_dp_bplcon
		adjust_drawing_colors(dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb || ecsshres, true);
		pfield_init_linetoscr(false);
		pfield_doline(lineno);

		/* The problem is that we must call decode_ham() BEFORE we do the sprites. */
		if (dp_for_drawing->ham_seen) {
			int ohposblank = hposblank;
			uae_u16 b0 = dp_for_drawing->bplcon0;
			uae_u16 b2 = dp_for_drawing->bplcon2;
			uae_u16 b3 = dp_for_drawing->bplcon3;
			uae_u16 b4bm = dp_for_drawing->bplcon4bm;
			uae_u16 b4sp = dp_for_drawing->bplcon4sp;
			uae_u16 fm = dp_for_drawing->fmode;
			init_ham_decoding();
			int oham_decode_pixel = ham_decode_pixel;
			do_color_changes(decode_ham_border, decode_ham, lineno);
			if (have_color_changes) {
				// do_color_changes() did color changes and register changes, restore them.
				adjust_drawing_colors(dp_for_drawing->ctable, -1, false);
				dp_for_drawing->bplcon0 = b0;
				dp_for_drawing->bplcon2 = b2;
				dp_for_drawing->bplcon3 = b3;
				dp_for_drawing->bplcon4bm = b4bm;
				dp_for_drawing->bplcon4bm = b4sp;
				dp_for_drawing->fmode = fm;
				dp_for_drawing->bplres = GET_RES_DENISE(dp_for_drawing->bplcon0);
				dp_for_drawing->nr_planes = GET_PLANES(dp_for_drawing->bplcon0);
				dp_for_drawing->ham_seen = isham(dp_for_drawing->bplcon0);
				if (currprefs.chipset_hr && dp_for_drawing->bplres < currprefs.gfx_resolution) {
					dp_for_drawing->bplres = currprefs.gfx_resolution;
				}
				pfield_expand_dp_bplcon();
				set_res_shift();
			}
			hposblank = ohposblank;
			ham_decode_pixel = oham_decode_pixel;
			bplham = dp_for_drawing->ham_at_start;
			setbplmode();
		}

		if (dip_for_drawing->nr_sprites) {
			int i;
#ifdef AGA
			if (ce_is_bordersprite(colors_for_drawing.extra) && dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra))
				clear_bitplane_border_aga();
#endif

			for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
#ifdef AGA
				if (aga_mode)
					draw_sprites_aga(curr_sprite_entries + dip_for_drawing->first_sprite_entry + i);
				else
#endif
					draw_sprites_ecs(curr_sprite_entries + dip_for_drawing->first_sprite_entry + i);
			}
		}

		if (dip_for_drawing->nr_sprites) {
			if (ce_is_bordersprite(colors_for_drawing.extra) && !ce_is_borderblank(colors_for_drawing.extra) && dp_for_drawing->bordersprite_seen) {
				do_color_changes(pfield_do_linetoscr_bordersprite_aga, pfield_do_linetoscr_spr, lineno);
			} else if (agnusa1000) {
				do_color_changes(pfield_do_linetoscr_bordersprite_a1000, pfield_do_linetoscr_spr, lineno);
			} else {
				do_color_changes(pfield_do_fill_line, dip_for_drawing->nr_sprites ? pfield_do_linetoscr_spr : pfield_do_linetoscr, lineno);
			}
		} else {
			do_color_changes(pfield_do_fill_line, dip_for_drawing->nr_sprites ? pfield_do_linetoscr_spr : pfield_do_linetoscr, lineno);
		}

		if (dh == dh_emerg)
			memcpy(row_map[gfx_ypos], xlinebuffer + linetoscr_x_adjust_pixbytes, vidinfo->drawbuffer.pixbytes * vidinfo->drawbuffer.inwidth);

		if (do_double) {
			if (dh == dh_emerg)
				memcpy(row_map[follow_ypos], xlinebuffer + linetoscr_x_adjust_pixbytes, vidinfo->drawbuffer.pixbytes * vidinfo->drawbuffer.inwidth);
			else if (dh == dh_buf)
				memcpy(row_map[follow_ypos], row_map[gfx_ypos], vidinfo->drawbuffer.pixbytes * vidinfo->drawbuffer.inwidth);
			if (need_genlock_data)
				memcpy(row_map_genlock[follow_ypos], row_map_genlock[gfx_ypos], vidinfo->drawbuffer.inwidth);
		}

		if (dip_for_drawing->nr_sprites) {
			pfield_erase_hborder_sprites();
		}

	} else if (border > 0) { // border > 0: top or bottom border

		bool dosprites = false;

		adjust_drawing_colors(dp_for_drawing->ctable, 0, true);

#ifdef AGA /* this makes things complex.. */
		if (dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra) && dip_for_drawing->nr_sprites) {
			dosprites = true;
			pfield_expand_dp_bplcon();
			pfield_init_linetoscr(true);
			pfield_erase_vborder_sprites();
		}
#endif
		if (!syncdebug && !dosprites && !have_color_changes) {
			if (dp_for_drawing->plfleft < -1) {
				// blanked border line
				int tmp = hposblank;
				hposblank = 1;
				fill_line_border(lineno, border);
				hposblank = tmp;
			} else {
				// normal border line
				fill_line_border(lineno, border);
			}

			if (do_double) {
				if (dh == dh_buf) {
					xlinebuffer = row_map[follow_ypos] - linetoscr_x_adjust_pixbytes;
					xlinebuffer_genlock = row_map_genlock[follow_ypos] - linetoscr_x_adjust_pixels;
					fill_line_border(lineno, border);
				}
				/* If dh == dh_line, do_flush_line will re-use the rendered line
				* from linemem.  */
			}
			return;
		}

#ifdef AGA
		if (dosprites) {

			for (int i = 0; i < dip_for_drawing->nr_sprites; i++)
				draw_sprites_aga(curr_sprite_entries + dip_for_drawing->first_sprite_entry + i);
			do_color_changes(pfield_do_linetoscr_bordersprite_aga, pfield_do_linetoscr_bordersprite_aga, lineno);
#else
		if (0) {
#endif

		} else {

			playfield_start = visible_right_border;
			playfield_end = visible_right_border;
			playfield_start_pre = playfield_start;
			playfield_end_pre = playfield_end;
			do_color_changes(pfield_do_fill_line, pfield_do_fill_line, lineno);

		}

		if (dh == dh_emerg)
			memcpy(row_map[gfx_ypos], xlinebuffer + linetoscr_x_adjust_pixbytes, vidinfo->drawbuffer.pixbytes * vidinfo->drawbuffer.inwidth);
		if (do_double) {
			if (dh == dh_emerg)
				memcpy(row_map[follow_ypos], xlinebuffer + linetoscr_x_adjust_pixbytes, vidinfo->drawbuffer.pixbytes * vidinfo->drawbuffer.inwidth);
			else if (dh == dh_buf)
				memcpy(row_map[follow_ypos], row_map[gfx_ypos], vidinfo->drawbuffer.pixbytes * vidinfo->drawbuffer.inwidth);
			if (need_genlock_data)
				memcpy(row_map_genlock[follow_ypos], row_map_genlock[gfx_ypos], vidinfo->drawbuffer.inwidth);
		}

	} else {

		// top or bottom blanking region
		int tmp = hposblank;
		hposblank = 1;
		fill_line_border(lineno, border);
		hposblank = tmp;

	}
}

static void center_image (void)
{
	struct amigadisplay *ad = &adisplays[0];
	struct gfx_filterdata *fd = &currprefs.gf[ad->gf_index];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;
	int prev_x_adjust = visible_left_border;
	int prev_y_adjust = thisframe_y_adjust;

	int w = vidinfo->drawbuffer.inwidth;
	int ew = vidinfo->drawbuffer.extrawidth;
	int maxdiw = max_diwlastword;

	if (currprefs.gfx_overscanmode <= OVERSCANMODE_OVERSCAN && currprefs.gfx_xcenter && !fd->gfx_filter_autoscale && max_diwstop > 0) {

		if (max_diwstop - min_diwstart < w && currprefs.gfx_xcenter == 2)
			/* Try to center. */
			visible_left_border = (max_diwstop - min_diwstart - w) / 2 + min_diwstart;
		else
			visible_left_border = max_diwstop - w - (max_diwstop - min_diwstart - w) / 2;
		visible_left_border &= ~((xshift(1, lores_shift)) - 1);

		if (!center_reset && !vertical_changed) {
			/* Would the old value be good enough? If so, leave it as it is if we want to be clever. */
			if (currprefs.gfx_xcenter == 2) {
				if (visible_left_border < prev_x_adjust && prev_x_adjust < min_diwstart && min_diwstart - visible_left_border <= 32)
					visible_left_border = prev_x_adjust;
			}
		}

	} else if (ew == -1) {
		// wide mode
		int hs = hsync_end_left_border;
		visible_left_border = hs << currprefs.gfx_resolution;
		if (visible_left_border + w > maxdiw) {
			visible_left_border += (maxdiw - (visible_left_border + w) - 1) / 2;
		}
		visible_left_border = std::max(visible_left_border, hs << currprefs.gfx_resolution);
	} else if (ew < -1) {
		// normal
		visible_left_border = maxdiw - w;
	} else {
		if (vidinfo->drawbuffer.inxoffset < 0) {
			visible_left_border = 0;
		} else {
			visible_left_border = (vidinfo->drawbuffer.inxoffset - DISPLAY_LEFT_SHIFT) << currprefs.gfx_resolution;
		}
	}

	visible_left_border = std::min(visible_left_border, max_diwlastword - 32);
	visible_left_border = std::max(visible_left_border, 0);
	visible_left_border &= ~((xshift (1, lores_shift)) - 1);

	//write_log (_T("%d %d %d %d %d\n"), max_diwlastword, vidinfo->drawbuffer.width, lores_shift, currprefs.gfx_resolution, visible_left_border);

	linetoscr_x_adjust_pixels = visible_left_border;
	linetoscr_x_adjust_pixbytes = linetoscr_x_adjust_pixels * vidinfo->drawbuffer.pixbytes;

	visible_right_border = maxdiw + w + ((ew > 0 ? ew : 0) << currprefs.gfx_resolution);
	visible_right_border = std::min(visible_right_border, maxdiw + ((ew > 0 ? ew : 0) << currprefs.gfx_resolution));

	int max_drawn_amiga_line_tmp = max_drawn_amiga_line;
	max_drawn_amiga_line_tmp = std::min(max_drawn_amiga_line_tmp, vidinfo->drawbuffer.inheight);
	max_drawn_amiga_line_tmp >>= linedbl;

	thisframe_y_adjust = minfirstline;
	if (currprefs.gfx_ycenter && !fd->gfx_filter_autoscale) {

		if (thisframe_first_drawn_line >= 0 && thisframe_last_drawn_line > thisframe_first_drawn_line) {

			if (thisframe_last_drawn_line - thisframe_first_drawn_line < max_drawn_amiga_line_tmp && currprefs.gfx_ycenter == 2)
				thisframe_y_adjust = (thisframe_last_drawn_line - thisframe_first_drawn_line - max_drawn_amiga_line_tmp) / 2 + thisframe_first_drawn_line;
			else
				thisframe_y_adjust = thisframe_first_drawn_line;

			/* Would the old value be good enough? If so, leave it as it is if we want to be clever. */
			if (!center_reset && !horizontal_changed) {
				if (currprefs.gfx_ycenter == 2 && thisframe_y_adjust != prev_y_adjust && abs(thisframe_y_adjust - prev_y_adjust) < 100) {
					if (prev_y_adjust <= thisframe_first_drawn_line && prev_y_adjust + max_drawn_amiga_line_tmp > thisframe_last_drawn_line)
						thisframe_y_adjust = prev_y_adjust;
				}
			}

		} else {

			center_reset = 2;

		}
	}

	/* Make sure the value makes sense */
	if (thisframe_y_adjust + max_drawn_amiga_line_tmp > maxvpos + maxvpos / 2)
		thisframe_y_adjust = maxvpos + maxvpos / 2 - max_drawn_amiga_line_tmp;
	thisframe_y_adjust = std::max(thisframe_y_adjust, 0);

	thisframe_y_adjust_real = thisframe_y_adjust << linedbl;
	max_ypos_thisframe1 = (maxvpos_display - minfirstline + maxvpos_display_vsync) << linedbl;

	if (prev_x_adjust != visible_left_border || prev_y_adjust != thisframe_y_adjust) {
		int redraw = interlace_seen > 0 && linedbl ? 2 : 1;
		ad->frame_redraw_necessary = std::max(redraw, ad->frame_redraw_necessary);
	}

	max_diwstop = 0;
	min_diwstart = MAX_STOP;

	vidinfo->drawbuffer.xoffset = (DISPLAY_LEFT_SHIFT << RES_MAX) + (visible_left_border << (RES_MAX - currprefs.gfx_resolution));
	vidinfo->drawbuffer.yoffset = thisframe_y_adjust << VRES_MAX;

	if (center_reset > 0) {
		center_reset--;
	}
	horizontal_changed = false;
	vertical_changed = false;
}

static int frame_res_cnt;
static int autoswitch_old_resolution;
static void init_drawing_frame (void)
{
	struct amigadisplay *ad = &adisplays[0];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;
	static int frame_res_old;

	int largest_res = 0;
	int largest_count = 0;
	int largest_count_res = 0;
	for (int i = 0; i <= RES_MAX; i++) {
		if (resolution_count[i])
			largest_res = i;
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
			newres = std::max(newres, RES_HIRES);
			newres = std::min(newres, RES_MAX);
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
			frame_res_detected = std::max(frame_res_detected, 0);
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
							nr = std::max(nr, nl);
						} else if (nr < currprefs.gfx_autoresolution_minh) {
							nr = currprefs.gfx_autoresolution_minh;
						}
						if (currprefs.gfx_autoresolution_minv < 0) {
							nl = std::max(nl, nr);
						} else if (nl < currprefs.gfx_autoresolution_minv) {
							nl = currprefs.gfx_autoresolution_minv;
						}

						nr = std::min(nr, vidinfo->gfx_resolution_reserved);
						nl = std::min(nl, vidinfo->gfx_vresolution_reserved);

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
	for (int i = 0; i <= RES_MAX; i++)
		resolution_count[i] = 0;
	lines_count = 0;
	frame_res = -1;
	frame_res_lace = 0;

	if (can_use_lores > AUTO_LORES_FRAMES && 0) {
		lores_factor = 1;
		lores_set(0);
	} else {
		can_use_lores++;
		lores_reset();
	}

	init_hardware_for_drawing_frame ();

#ifdef AMIBERRY
	linestate_first_undecided = 0;
#endif

	if (thisframe_first_drawn_line < 0)
		thisframe_first_drawn_line = minfirstline;
	thisframe_last_drawn_line = std::max(thisframe_first_drawn_line, thisframe_last_drawn_line);

	int maxline = ((maxvpos_display + maxvpos_display_vsync + 1) << linedbl) + 2;
	for (int i = 0; i < maxline; i++) {
		int ls = linestate[i];
		switch (ls) {
		case LINE_DONE_AS_PREVIOUS:
			linestate[i] = LINE_REMEMBERED_AS_PREVIOUS;
			break;
		case LINE_REMEMBERED_AS_BLACK:
			break;
		default:
			linestate[i] = LINE_UNDECIDED;
			break;
		}
	}
	last_drawn_line = 0;
	first_drawn_line = 32767;

	if (ad->frame_redraw_necessary) {
		reset_decision_table();
		ad->custom_frame_redraw_necessary = 1;
		ad->frame_redraw_necessary--;
	} else {
		ad->custom_frame_redraw_necessary = 0;
	}

	center_image ();

	thisframe_first_drawn_line = -1;
	thisframe_last_drawn_line = -1;

	drawing_color_matches = -1;
}

static int lightpen_y1[2], lightpen_y2[2];

void putpixel(uae_u8 *buf, uae_u8 *genlockbuf, int bpp, int x, xcolnr c8)
{
	if (x <= 0)
		return;

	if (genlockbuf)
		genlockbuf[x] = 0xff;

	switch (bpp) {
	case 1:
		buf[x] = (uae_u8)c8;
		break;
	case 2:
		{
			uae_u16 *p = (uae_u16*)buf + x;
			*p = (uae_u16)c8;
			break;
		}
	case 3:
		/* no 24 bit yet */
		break;
	case 4:
		{
			uae_u32 *p = (uae_u32*)buf + x;
			*p = c8;
			break;
		}
	}
}

static uae_u8 *status_line_ptr(int monid, int line)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	int y;

	y = line - (vidinfo->drawbuffer.outheight - TD_TOTAL_HEIGHT);
	xlinebuffer = vidinfo->drawbuffer.linemem;
	if (xlinebuffer == 0)
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
		statusline_render(monid, buf, vidinfo->drawbuffer.pixbytes, vidinfo->drawbuffer.rowbytes, vidinfo->drawbuffer.outwidth, TD_TOTAL_HEIGHT, xredcolors, xgreencolors, xbluecolors, NULL);
	else
		draw_status_line_single(monid, buf, vidinfo->drawbuffer.pixbytes, statusy, vidinfo->drawbuffer.outwidth, xredcolors, xgreencolors, xbluecolors, NULL);
}

static void draw_debug_status_line(int monid, int line)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	xlinebuffer = vidinfo->drawbuffer.linemem;
	if (xlinebuffer == 0)
		xlinebuffer = row_map[line];
	xlinebuffer_genlock = row_map_genlock[line];
#ifdef DEBUGGER
	debug_draw(xlinebuffer, vidinfo->drawbuffer.pixbytes, line, vidinfo->drawbuffer.outwidth, vidinfo->drawbuffer.outheight, xredcolors, xgreencolors, xbluecolors);
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

	xlinebuffer = vidinfo->drawbuffer.linemem;
	if (xlinebuffer == 0)
		xlinebuffer = row_map[line];
	xlinebuffer_genlock = row_map_genlock[line];

	p = lightpen_cursor + y * LIGHTPEN_WIDTH;
	for (int i = 0; i < LIGHTPEN_WIDTH; i++) {
		int xx = x + i - LIGHTPEN_WIDTH / 2;
		if (*p != '-' && xx >= 0 && xx < vidinfo->drawbuffer.outwidth) {
			putpixel(xlinebuffer, xlinebuffer_genlock, vidinfo->drawbuffer.pixbytes, xx, *p == 'x' ? xcolors[color1] : xcolors[color2]);
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

	lightpen_x[lpnum] = std::max(lightpen_x[lpnum], -extra);
	lightpen_x[lpnum] = std::min(lightpen_x[lpnum], vidinfo->drawbuffer.inwidth + extra);
	lightpen_y[lpnum] = std::max(lightpen_y[lpnum], -extra);
	lightpen_y[lpnum] = std::min(lightpen_y[lpnum], vidinfo->drawbuffer.inheight + extra);
	lightpen_y[lpnum] = std::min(lightpen_y[lpnum], max_ypos_thisframe1);

	if (lightpen_x[lpnum] < 0 || lightpen_y[lpnum] < 0) {
		out = true;
	}
	if (lightpen_x[lpnum] >= vidinfo->drawbuffer.inwidth) {
		out = true;
	}
	if (lightpen_y[lpnum] >= max_ypos_thisframe1) {
		out = true;
	}

	int cx = (((lightpen_x[lpnum] + visible_left_border) >> lores_shift) >> 1) + 29;

	int cy = lightpen_y[lpnum];
	cy >>= linedbl;
	cy += minfirstline;

	cx += currprefs.lightpen_offset[0];
	cy += currprefs.lightpen_offset[1];

	if (cx <= 0x18 - 1) {
		cx = 0x18 - 1;
		out = true;
	}
	if (cy <= minfirstline - 1) {
		cy = minfirstline - 1;
		out = true;
	}
	if (cy >= maxvpos) {
		cy = maxvpos;
		out = true;
	}

	if (currprefs.lightpen_crosshair && lightpen_active) {
		for (int i = 0; i < LIGHTPEN_HEIGHT; i++) {
			int line = lightpen_y[lpnum] + i - LIGHTPEN_HEIGHT / 2;
			if (line >= 0 && line < max_ypos_thisframe1) {
				if (lightpen_active & (1 << lpnum)) {
					draw_lightpen_cursor(vb->monitor_id, lightpen_x[lpnum], i, line, cx > 0, lpnum);
				}
			}
		}
	}

	lightpen_y1[lpnum] = lightpen_y[lpnum] - LIGHTPEN_HEIGHT / 2 - 1 + thisframe_y_adjust;
	lightpen_y2[lpnum] = lightpen_y1[lpnum] + LIGHTPEN_HEIGHT + 1 + thisframe_y_adjust;

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

	refresh_indicator_height = 600;
	refresh_indicator_buffer = xcalloc(uae_u8, MAX_PIXELS_PER_LINE * 2 * refresh_indicator_height);
	refresh_indicator_changed = xcalloc(uae_u8, refresh_indicator_height);
	refresh_indicator_changed_prev = xcalloc(uae_u8, refresh_indicator_height);
}

static const int refresh_indicator_colors[] = { 0x777, 0x0f0, 0x00f, 0xff0, 0xf0f };

static void refresh_indicator_update(struct vidbuffer *vb)
{
	struct vidbuf_description *vidinfo = &adisplays[vb->monitor_id].gfxvidinfo;
	for (int i = 0; i < max_ypos_thisframe1; i++) {
		int i1 = i + min_ypos_for_screen;
		int line = i + thisframe_y_adjust_real;
		int whereline = amiga2aspect_line_map[i1];
		int wherenext = amiga2aspect_line_map[i1 + 1];

		if (whereline >= vb->inheight)
			break;
		if (whereline < 0)
			continue;
		if (line >= refresh_indicator_height)
			break;

		xlinebuffer = row_map[whereline];
		uae_u8 pixel = refresh_indicator_changed_prev[line];
		if (wherenext >= 0) {
			pixel = refresh_indicator_changed_prev[line & ~1];
		}

		int color1 = 0;
		int color2 = 0;
		if (pixel <= 4) {
			color1 = color2 = refresh_indicator_colors[pixel];
		} else if (pixel <= 8) {
			color2 = refresh_indicator_colors[pixel - 5];
		}
		for (int x = 0; x < 8; x++) {
			putpixel(xlinebuffer, NULL, vidinfo->drawbuffer.pixbytes, x, xcolors[color1]);
		}
		for (int x = 8; x < 16; x++) {
			putpixel(xlinebuffer, NULL, vidinfo->drawbuffer.pixbytes, x, xcolors[color2]);
		}
	}
}

#define LARGEST_LINE_DEBUG 0

static void draw_frame2(struct vidbuffer *vbin, struct vidbuffer *vbout)
{
#if LARGEST_LINE_DEBUG
	int largest = 0;
#endif

	set_vblanking_limits();
	reset_hblanking_limits();
	set_hblanking_limits();
	extblankcheck();
	expand_vb_state();

	bool firstline = true;
	int lastline = thisframe_y_adjust_real - (1 << linedbl);
	for (int i = 0; i < max_ypos_thisframe1; i++) {
		int i1 = i + min_ypos_for_screen;
		int line = i + thisframe_y_adjust_real;
		int whereline = amiga2aspect_line_map[i1];
		int wherenext = amiga2aspect_line_map[i1 + 1];

#ifdef AMIBERRY
		if (whereline >= vbin->inheight || line >= linestate_first_undecided)
#else
		if (whereline >= vbin->inheight)
#endif
			break;
		if (whereline < 0) {
			lastline = line;
			continue;
		}

		if (firstline) {
			if (lastline >= 0) {
				// scan line - 1 events, it might have hblank enable for next line.
				for (int j = 0; j < 2; j++) {
					dip_for_drawing = curr_drawinfo + lastline;
					dp_for_drawing = line_decisions + lastline;
					do_color_changes(NULL, NULL, -1);
					lastline++;
				}
			}
			firstline = false;
		}

#if LARGEST_LINE_DEBUG
		if (largest < whereline)
			largest = whereline;
#endif

		if (ecs_denise) {
			reset_hblanking_limits();
			set_hblanking_limits();
		}

		hposblank = 0;
		pfield_draw_line(vbout, line, whereline, wherenext);
	}

#if LARGEST_LINE_DEBUG
	write_log (_T("%d\n"), largest);
#endif
}

static void draw_frame_extras(struct vidbuffer *vb, int y_start, int y_end)
{
#ifdef DEBUGGER
	if (debug_dma > 1 || debug_heatmap > 1) {
		for (int i = 0; i < vb->outheight; i++) {
			int line = i;
			draw_debug_status_line(vb->monitor_id, line);
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
	if (refresh_indicator_buffer)
		refresh_indicator_update(vb);
}

#ifdef WITH_BEAMRACER
extern bool beamracer_debug;
#endif

void draw_lines(int end, int section)
{
	int monid = 0;
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	struct vidbuffer *vb = &vidinfo->drawbuffer;
	int y_start = -1;
	int y_end = -1;

	static bool section_toggle;

	if (section == 0)
		section_toggle = !section_toggle;

	end -= minfirstline;
	if (end < 0)
		return;
	end <<= linedbl;
	if (min_ypos_for_screen > 0 && thisframe_y_adjust_real > 0) {
		end += min_ypos_for_screen;
		end -= thisframe_y_adjust_real;
		if (end < 0)
			return;
	}

#ifdef WITH_BEAMRACER
	int section_color_cnt = 4;
#endif

	vidinfo->outbuffer = vb;
	if (!lockscr(vb, false, vb->last_drawn_line ? false : true, display_reset > 0))
		return;

	set_vblanking_limits();
	reset_hblanking_limits();
	set_hblanking_limits();

	bool firstline = true;
	int lastline = thisframe_y_adjust_real - (1 << linedbl);
	while (vb->last_drawn_line < end) {
		int i = vb->last_drawn_line;
		int i1 = i + min_ypos_for_screen;
		int line = i + thisframe_y_adjust_real;
		int whereline = amiga2aspect_line_map[i1];
		int wherenext = amiga2aspect_line_map[i1 + 1];

#ifdef AMIBERRY
		if (whereline >= vb->inheight || line >= linestate_first_undecided) {
#else
		if (whereline >= vb->inheight) {
#endif
			y_end = vb->inheight - 1;
			break;
		}
		if (whereline < 0) {
			lastline = line;
			continue;
		}
		if (y_start < 0) {
			y_start = whereline;
		}

		if (firstline) {
			if (lastline >= 0) {
				// scan line - 1 events, it might have hblank enable for next line.
				for (int j = 0; j < 2; j++) {
					dip_for_drawing = curr_drawinfo + lastline;
					do_color_changes(NULL, NULL, -1);
					lastline++;
				}
			}
			firstline = false;
		}

		hposblank = 0;
		pfield_draw_line(vb, line, whereline, wherenext);

#ifdef WITH_BEAMRACER
#if 1
		if (beamracer_debug) {
			if (vb->last_drawn_line == end - 4) {
				section_color_cnt = 4;
			}
			if (section_color_cnt > 0) {
				section_color_cnt--;
				static const int section_colors[] = { 0x777, 0xf00, 0x0f0, 0x00f };
				int color = section_toggle ? section_colors[section & 3] : 0;
				xlinebuffer = row_map[whereline];
				for (int x = 0; x < 4; x++) {
					putpixel(xlinebuffer, NULL, vidinfo->drawbuffer.pixbytes, x, xcolors[color]);
				}
			}
		}
#endif
#endif

		vb->last_drawn_line++;
		if (vb->last_drawn_line == end) {
			y_end = whereline;
		}
	}
	draw_frame_extras(vb, y_start, y_end + 1);
	unlockscr(vb, y_start, y_end + 1);
}

bool draw_frame (struct vidbuffer *vb)
{
	struct vidbuf_description *vidinfo = &adisplays[vb->monitor_id].gfxvidinfo;
	uae_u8 oldstate[LINESTATE_SIZE];
	struct vidbuffer oldvb{};

	memcpy (&oldvb, &vidinfo->drawbuffer, sizeof (struct vidbuffer));
	memcpy (&vidinfo->drawbuffer, vb, sizeof (struct vidbuffer));
	clearbuffer (vb);
	init_row_map ();
	memcpy (oldstate, linestate, LINESTATE_SIZE);
	for (int i = 0; i < LINESTATE_SIZE; i++) {
		uae_u8 v = linestate[i];
		if (v == LINE_REMEMBERED_AS_PREVIOUS) {
			if (i > 0)
				linestate[i - 1] = LINE_DECIDED_DOUBLE;
			v = LINE_AS_PREVIOUS;
		} else if (v == LINE_DONE_AS_PREVIOUS) {
			if (i > 0)
				linestate[i - 1] = LINE_DECIDED_DOUBLE;
			v = LINE_AS_PREVIOUS;
		} else if (v == LINE_REMEMBERED_AS_BLACK) {
			v = LINE_BLACK;
		} else if (v == LINE_DONE) {
			v = LINE_DECIDED;
		}
//		if (i < maxvpos)
//			write_log (_T("%d: %d -> %d\n"), i, linestate[i], v);
		linestate[i] = v;
	}
	last_drawn_line = 0;
	first_drawn_line = 32767;
	drawing_color_matches = -1;
	draw_frame2 (vb, NULL);
	last_drawn_line = 0;
	first_drawn_line = 32767;
	drawing_color_matches = -1;
	memcpy (linestate, oldstate, LINESTATE_SIZE);
	memcpy (&vidinfo->drawbuffer, &oldvb, sizeof (struct vidbuffer));
	init_row_map ();
	return true;
}

static void setnativeposition(struct vidbuffer *vb)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	vb->inwidth = vidinfo->drawbuffer.inwidth;
	vb->inheight = vidinfo->drawbuffer.inheight;
	vb->inwidth2 = vidinfo->drawbuffer.inwidth2;
	vb->inheight2 = vidinfo->drawbuffer.inheight2;
	vb->outwidth = vidinfo->drawbuffer.outwidth;
	vb->outheight = vidinfo->drawbuffer.outheight;
}

static void setspecialmonitorpos(struct vidbuffer *vb)
{
	struct vidbuf_description *vidinfo = &adisplays[0].gfxvidinfo;
	vb->extrawidth = vidinfo->drawbuffer.extrawidth;
	vb->xoffset = vidinfo->drawbuffer.xoffset;
	vb->yoffset = vidinfo->drawbuffer.yoffset;
	vb->inxoffset = vidinfo->drawbuffer.inxoffset;
	vb->inyoffset = vidinfo->drawbuffer.inyoffset;
}

static void finish_drawing_frame(bool drawlines)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;
	struct vidbuffer *vb = &vidinfo->drawbuffer;

	vidinfo->outbuffer = vb;
	vb->last_drawn_line = 0;

	if (!drawlines) {
		return;
	}

	if (!lockscr(vb, false, true, display_reset > 0)) {
		notice_screen_contents_lost(monid);
		return;
	}

	draw_frame2(vb, vb);

	draw_frame_extras(vb, -1, -1);

#ifdef WITH_SPECIALMONITORS
	// video port adapters
	if (currprefs.monitoremu) {
		struct vidbuf_description *outvi = &adisplays[currprefs.monitoremu_mon].gfxvidinfo;
		struct vidbuffer *out = &outvi->drawbuffer;
		if (init_genlock_data != specialmonitor_need_genlock()) {
			init_genlock_data = specialmonitor_need_genlock();
			init_row_map();
		}
		bool locked = true;
		bool multimon = currprefs.monitoremu_mon != 0;
		if (multimon) {
			locked = lockscr(out, false, true, display_reset > 0);
			outvi->xchange = vidinfo->xchange;
			outvi->ychange = vidinfo->ychange;
		} else {
			out = &vidinfo->tempbuffer;
		}
		setspecialmonitorpos(out);
		if (locked && emulate_specialmonitors(vb, out)) {
			if (!multimon) {
				vb->tempbufferinuse = true;
				vb = vidinfo->outbuffer = out;
			}
			if (out->nativepositioning) {
				setnativeposition(out);
			}
			if (!ad->specialmonitoron) {
				need_genlock_data = specialmonitor_need_genlock();
				ad->specialmonitoron = true;
				compute_framesync();
				pfield_set_linetoscr();
			}
		} else {
			pfield_set_linetoscr();
			need_genlock_data = false;
			if (ad->specialmonitoron || out->tempbufferinuse) {
				out->tempbufferinuse = false;
				ad->specialmonitoron = false;
				compute_framesync();
			}
		}
		if (multimon && locked) {
			unlockscr(out, -1, -1);
			render_screen(out->monitor_id, 1, true);
			show_screen(out->monitor_id, 0);
		}
	}

	// genlock
	if (currprefs.genlock_image && (currprefs.genlock || currprefs.genlock_effects) && !currprefs.monitoremu && vidinfo->tempbuffer.bufmem_allocated) {
		setspecialmonitorpos(&vidinfo->tempbuffer);
		if (init_genlock_data != specialmonitor_need_genlock()) {
			need_genlock_data = init_genlock_data = specialmonitor_need_genlock();
			init_row_map();
			pfield_set_linetoscr();
		}
		emulate_genlock(vb, &vidinfo->tempbuffer, aga_genlock_features_zdclken);
		vb = vidinfo->outbuffer = &vidinfo->tempbuffer;
		if (vb->nativepositioning)
			setnativeposition(vb);
		vidinfo->drawbuffer.tempbufferinuse = true;
	}
#endif

#ifdef CD32
	// cd32 fmv
	if (!currprefs.monitoremu && vidinfo->tempbuffer.bufmem_allocated && currprefs.cs_cd32fmv) {
		if (cd32_fmv_active) {
			cd32_fmv_genlock(vb, &vidinfo->tempbuffer);
			vb = vidinfo->outbuffer = &vidinfo->tempbuffer;
			setnativeposition(vb);
			vidinfo->drawbuffer.tempbufferinuse = true;
		} else {
			vidinfo->drawbuffer.tempbufferinuse = false;
		}
	}
#endif

	// grayscale
#ifdef WITH_SPECIALMONITORS
	if (!currprefs.monitoremu && vidinfo->tempbuffer.bufmem_allocated &&
		((!currprefs.genlock && (!bplcolorburst_field && currprefs.cs_color_burst)) || currprefs.gfx_grayscale)) {
		setspecialmonitorpos(&vidinfo->tempbuffer);
		emulate_grayscale(vb, &vidinfo->tempbuffer);
		vb = vidinfo->outbuffer = &vidinfo->tempbuffer;
		if (vb->nativepositioning)
			setnativeposition(vb);
		vidinfo->drawbuffer.tempbufferinuse = true;
	}
#endif

	unlockscr(vb, display_reset ? -2 : -1, -1);
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

		if (ad->picasso_on && ad->picasso_redraw_necessary)
			picasso_refresh(monid);
		ad->picasso_redraw_necessary = 0;

		if (ad->picasso_requested_on == ad->picasso_on && !ad->picasso_requested_forced_on)
			continue;
		 
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
	if (vidinfo->drawbuffer.height_allocated && amiga2aspect_line_map) {
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

bool vsync_handle_check (void)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	int changed = check_prefs_changed_gfx ();
	if (changed > 0) {
		reset_drawing ();
		init_row_map ();
		init_aspect_maps ();
		notice_screen_contents_lost(monid);
		notice_new_xcolors ();
	} else if (changed < 0) {
		reset_drawing ();
		init_row_map ();
		init_aspect_maps ();
		notice_screen_contents_lost(monid);
		notice_new_xcolors ();
	}
	if (config_changed) {
		device_check_config();
	}
	return changed != 0;
}

#ifdef AMIBERRY
void quit_drawing_thread()
{
	while (drawing_thread_busy)
		sleep_micros(1);
	if (drawing_pipe)
		write_comm_pipe_u32(drawing_pipe, RENDER_SIGNAL_QUIT, 1);
}
#endif

void vsync_handle_redraw(int long_field, int lof_changed, uae_u16 bplcon0p, uae_u16 bplcon3p, bool drawlines, bool initial)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	last_redraw_point++;
	if (lof_changed || interlace_seen <= 0 || (currprefs.gfx_iscanlines && interlace_seen > 0) || last_redraw_point >= 2 || long_field || doublescan < 0) {
		last_redraw_point = 0;

		if (!initial) {
			if (ad->framecnt == 0) {
#ifdef AMIBERRY
				if (currprefs.multithreaded_drawing && drawing_tid)
				{
					if (quit_program < 0)
					{
						quit_drawing_thread();
					}
					else
					{
						while (drawing_thread_busy)
							sleep_micros(10);
						write_comm_pipe_u32(drawing_pipe, RENDER_SIGNAL_FRAME_DONE, 1);
						uae_sem_wait(&drawing_sem);
					}
				}
				else
				{
					finish_drawing_frame(drawlines);
				}
#else
				finish_drawing_frame(drawlines);
#endif
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

		if (quit_program < 0) {
#ifdef AMIBERRY
			if (currprefs.multithreaded_drawing && drawing_tid)
			{
				quit_drawing_thread();
			}
#endif
#ifdef SAVESTATE
			if (!savestate_state && quit_program == -UAE_QUIT && currprefs.quitstatefile[0]) {
				savestate_initsave(currprefs.quitstatefile, 1, 1, true);
				save_state(currprefs.quitstatefile, _T(""));
			}
#endif
			quit_program = -quit_program;
			set_inhibit_frame(monid, IHF_QUIT_PROGRAM);
			set_special(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
			return;
		}

		if (ad->framecnt == 0) {
			init_drawing_frame();
		} else if (currprefs.cpu_memory_cycle_exact) {
			init_hardware_for_drawing_frame();
		}
	} else {
#if 0
		if (isvsync_chipset()) {
			flush_screen(vidinfo->inbuffer, 0, 0); /* vsync mode */
		}
#endif
	}

	gui_flicker_led (-1, 0, 0);
}

void hsync_record_line_state_last(int lineno, enum nln_how how, int changed)
{
	uae_u8 *state = linestate + lineno;

	switch (how) {
		case nln_upper_black:
		case nln_upper_black_always:
		case nln_lower_black:
		case nln_lower_black_always:
		hsync_record_line_state(lineno, how, 0);
		break;
	}
}

void hsync_record_line_state(int lineno, enum nln_how how, int changed)
{
	struct amigadisplay *ad = &adisplays[0];
	uae_u8 *state;

	if (ad->framecnt != 0)
		return;

	//write_log("%d:%d:%d ", lineno, how, lof_display);

	state = linestate + lineno;
	changed |= ad->frame_redraw_necessary != 0 || refresh_indicator_buffer != NULL ||
		((lineno >= lightpen_y1[0] && lineno < lightpen_y2[0]) ||
		(lineno >= lightpen_y1[1] && lineno < lightpen_y2[1]));

	switch (how) {
	case nln_normal:
		*state = changed ? LINE_DECIDED : LINE_DONE;
		break;
	case nln_doubled:
		*state = changed ? LINE_DECIDED_DOUBLE : LINE_DONE;
		changed |= state[1] != LINE_REMEMBERED_AS_PREVIOUS;
		state[1] = changed ? LINE_AS_PREVIOUS : LINE_DONE_AS_PREVIOUS;
		break;
	case nln_nblack:
		*state = changed ? LINE_DECIDED : LINE_DONE;
		if (state[1] != LINE_REMEMBERED_AS_BLACK) {
			state[1] = LINE_BLACK;
		}
		break;
	case nln_lower:
		if (lineno > 0 && state[-1] == LINE_UNDECIDED) {
			state[-1] = LINE_DECIDED; //LINE_BLACK;
		}
		*state = changed ? LINE_DECIDED : LINE_DONE;
		break;
	case nln_upper:
		*state = changed ? LINE_DECIDED : LINE_DONE;
		if (state[1] == LINE_UNDECIDED
			|| state[1] == LINE_REMEMBERED_AS_PREVIOUS
			|| state[1] == LINE_AS_PREVIOUS)
			state[1] = LINE_DECIDED; //LINE_BLACK;
		break;
	case nln_lower_black_always:
		state[1] = LINE_BLACK;
		*state = LINE_DECIDED;
		break;
	case nln_lower_black:
		changed |= state[0] != LINE_DONE;
		*state = changed ? LINE_DECIDED : LINE_DONE;
		state[1] = LINE_DONE;
		break;
	case nln_upper_black_always:
		*state = LINE_DECIDED;
		if (lineno > 0) {
			state[-1] = LINE_BLACK;
		}
		break;
	case nln_upper_black:
		changed |= state[0] != LINE_DONE;
		*state = changed ? LINE_DECIDED : LINE_DONE;
		if (lineno > 0) {
			state[-1] = LINE_DONE;
		}
		break;
	}
#ifdef AMIBERRY
	linestate_first_undecided = lineno + 1;
	if (currprefs.multithreaded_drawing)
	{
		if (drawing_tid && linestate_first_undecided > 3 && !drawing_thread_busy) {
			if (currprefs.gfx_vresolution) {
				if (!(linestate_first_undecided & 0x3e))
					write_comm_pipe_u32(drawing_pipe, RENDER_SIGNAL_PARTIAL, 1);
			}
			else if (!(linestate_first_undecided & 0x1f))
				write_comm_pipe_u32(drawing_pipe, RENDER_SIGNAL_PARTIAL, 1);
		}
	}
#endif
}

static void dummy_flush_line(struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int line_no)
{
}

static void dummy_flush_block(struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line)
{
}

static void dummy_flush_screen(struct vidbuf_description *gfxinfo, struct vidbuffer *vb, int first_line, int last_line)
{
}

static void dummy_flush_clear_screen(struct vidbuf_description *gfxinfo, struct vidbuffer *vb)
{
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
	vidinfo->drawbuffer.flush_line         = dummy_flush_line;
	vidinfo->drawbuffer.flush_block        = dummy_flush_block;
	vidinfo->drawbuffer.flush_screen       = dummy_flush_screen;
	vidinfo->drawbuffer.flush_clear_screen = dummy_flush_clear_screen;
	vidinfo->drawbuffer.lockscr            = dummy_lock;
	vidinfo->drawbuffer.unlockscr          = dummy_unlock;
}

void notice_resolution_seen (int res, bool lace)
{
	frame_res = std::max(res, frame_res);
	if (res > 0)
		can_use_lores = 0;
	if (!frame_res_lace && lace)
		frame_res_lace = lace;
}

bool notice_interlace_seen (int monid, bool lace)
{
	struct amigadisplay *ad = &adisplays[0];
	bool changed = false;
	bool interlace_on = false;

	// non-lace to lace switch (non-lace active at least one frame)?
	if (lace) {
		if (interlace_seen == 0) {
			changed = true;
			interlace_on = true;
			//write_log (_T("->lace PC=%x\n"), m68k_getpc ());
		}
		interlace_seen = currprefs.gfx_vresolution ? 1 : -1;
	} else {
		if (interlace_seen) {
			changed = true;
			//write_log (_T("->non-lace PC=%x\n"), m68k_getpc ());
		}
		interlace_seen = 0;
	}

	if (changed) {
		if (currprefs.gf[GF_INTERLACE].enable && memcmp(&currprefs.gf[GF_NORMAL], &currprefs.gf[GF_INTERLACE], sizeof(struct gfx_filterdata))) {
			changed_prefs.gf[GF_NORMAL].changed = true;
			changed_prefs.gf[GF_INTERLACE].changed = true;
			if (ad->interlace_on != interlace_on) {
				ad->interlace_on = interlace_on;
				set_config_changed();
			}
		} else {
			ad->interlace_on = false;
		}
	}

	if (!ad->picasso_on) {
		if (ad->interlace_on) {
			ad->gf_index = GF_INTERLACE;
		} else {
			ad->gf_index = GF_NORMAL;
		}
	} else {
		ad->gf_index = GF_RTG;
	}

	return changed;
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
	buf->realbufmem = xcalloc(uae_u8, size);
	buf->bufmem_allocated = buf->bufmem = buf->realbufmem;
	buf->bufmemend = buf->realbufmem + size - buf->rowbytes;
	buf->bufmem_lockable = true;
}

void freevidbuffer(int monid, struct vidbuffer *buf)
{
	xfree (buf->realbufmem);
	memset (buf, 0, sizeof (struct vidbuffer));
}

void reset_drawing(void)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;

	syncdebug = currprefs.gfx_overscanmode >= OVERSCANMODE_ULTRA;
	max_diwstop = 0;
	vb_state = 0;
	exthblank = 0;
	exthblank_force = false;
	exthblank_set = syncdebug ? -1 : 1;
	exthblanken = false;
	exthblankon = false;
	extborder = false;
	display_reset = 1;
	ehb_enable = true;
	hblank_debug = vblank_debug = hsync_debug = vsync_debug = false;
	hcenter_debug = 0;

	lores_reset ();

#ifdef AMIBERRY
	linestate_first_undecided = 0;
#endif

	reset_decision_table();

	init_aspect_maps ();

	init_row_map ();

	last_redraw_point = 0;

	memset(spixels, 0, sizeof spixels);
	memset(&spixstate, 0, sizeof spixstate);
	memset(spritepixels_buffer, 0, sizeof(spritepixels_buffer));
	memset(line_data, 0, sizeof(line_data));
	memset(ham_linebuf, 0, sizeof(ham_linebuf));

	init_hardware_for_drawing_frame();
		
	notice_screen_contents_lost(monid);
	init_drawing_frame ();
	pfield_set_linetoscr();

	frame_res_cnt = currprefs.gfx_autoresolution_delay;
	lightpen_y1[0] = lightpen_y2[0] = -1;
	lightpen_y1[1] = lightpen_y2[1] = -1;

	reset_custom_limits ();

	clearbuffer (&vidinfo->drawbuffer);
	clearbuffer (&vidinfo->tempbuffer);

	center_reset = 1;
	ad->specialmonitoron = false;
	bplcolorburst_field = 1;
	ecs_genlock_features_active = false;
	aga_genlock_features_zdclken = false;
	ecs_genlock_features_colorkey = false;
}

static void gen_direct_drawing_table(void)
{
#ifdef AGA
	// BYPASS color table
	for (int i = 0; i < 256; i++) {
		uae_u32 v = (i << 16) | (i << 8) | i;
		direct_colors_for_drawing.acolors[i] = CONVERT_RGB(v);
	}
#endif
}

#ifdef AMIBERRY
static int drawing_thread(void *unused)
{
	for (;;) {
		drawing_thread_busy = false;
		const auto signal = read_comm_pipe_u32_blocking(drawing_pipe);
		drawing_thread_busy = true;
		switch (signal) {

			case RENDER_SIGNAL_PARTIAL:
				draw_lines(0, 0);
				break;

			case RENDER_SIGNAL_FRAME_DONE:
				finish_drawing_frame(true);
				uae_sem_post(&drawing_sem);
				break;

			case RENDER_SIGNAL_QUIT:
				drawing_tid = nullptr;
				if (drawing_pipe)
				{
					destroy_comm_pipe(drawing_pipe);
					xfree(drawing_pipe);
					drawing_pipe = nullptr;
				}
				if (drawing_sem)
				{
					uae_sem_destroy(&drawing_sem);
					drawing_sem = nullptr;
				}
				drawing_thread_busy = false;
				return 0;
			default:
				break;
		}
	}
}

void start_drawing_thread()
{
	if (drawing_pipe == nullptr) {
		drawing_pipe = xmalloc(smp_comm_pipe, 1);
		init_comm_pipe(drawing_pipe, 20, 1);
	}
	if (drawing_sem == nullptr) {
		uae_sem_init(&drawing_sem, 0, 0);
	}
	if (drawing_tid == nullptr && drawing_pipe != nullptr && drawing_sem != nullptr) {
		uae_start_thread(_T("drawing"), drawing_thread, nullptr, &drawing_tid);
	}
}
#endif

void drawing_init (void)
{
	int monid = 0;
	struct amigadisplay *ad = &adisplays[monid];
	struct vidbuf_description *vidinfo = &ad->gfxvidinfo;

	refresh_indicator_init();

	gen_pfield_tables();

	gen_direct_drawing_table();

	uae_sem_init (&gui_sem, 0, 1);
#ifdef AMIBERRY
	if (currprefs.multithreaded_drawing)
	{
		start_drawing_thread();
	}
#endif

#ifdef PICASSO96
	if (!isrestore ()) {
		ad->picasso_on = 0;
		ad->picasso_requested_on = 0;
		ad->gf_index = GF_NORMAL;
		gfx_set_picasso_state(0, 0);
	}
#endif
	xlinebuffer = vidinfo->drawbuffer.bufmem;
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
		return isvsync_rtg ();
	else
		return isvsync_chipset ();
}
