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

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "autoconf.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "savestate.h"
#include "statusline.h"
#include "cd32_fmv.h"
#include "audio.h"
#include "devices.h"

#define RENDER_SIGNAL_PARTIAL 1
#define RENDER_SIGNAL_FRAME_DONE 2
#define RENDER_SIGNAL_QUIT 3
static uae_thread_id render_tid = 0;
static smp_comm_pipe *volatile render_pipe = 0;
static uae_sem_t render_sem = 0;
static bool volatile render_thread_busy = false;

extern int sprite_buffer_res;
int lores_shift;

static void pfield_set_linetoscr(void);

static void lores_set(int lores)
{
	int old = lores_shift;
	lores_shift = lores;
	if (lores_shift != old)
		pfield_set_linetoscr();
}

static void lores_reset (void)
{
	lores_set(currprefs.gfx_resolution);
	sprite_buffer_res = currprefs.gfx_resolution;
}

bool aga_mode; /* mirror of chipset_mask & CSMASK_AGA */

/* The shift factor to apply when converting between Amiga coordinates and window
   coordinates.  Zero if the resolution is the same, positive if window coordinates
   have a higher resolution (i.e. we're stretching the image), negative if window
   coordinates have a lower resolution (i.e. we're shrinking the image).  */
static int res_shift;

static int linedbl;

int interlace_seen;

/* Lookup tables for dual playfields.  The dblpf_*1 versions are for the case
   that playfield 1 has the priority, dbplpf_*2 are used if playfield 2 has
   priority.  If we need an array for non-dual playfield mode, it has no number.  */
/* The dbplpf_ms? arrays contain a shift value.  plf_spritemask is initialized
   to contain two 16 bit words, with the appropriate mask if pf1 is in the
   foreground being at bit offset 0, the one used if pf2 is in front being at
   offset 16.  */

static int dblpf_ms1[256], dblpf_ms2[256];
static int dblpf_ind1[256], dblpf_ind2[256];

static int dblpf_2nd1[256], dblpf_2nd2[256];

static const int dblpfofs[] = { 0, 2, 4, 8, 16, 32, 64, 128 };

static int sprite_col_nat[65536];
static int sprite_col_at[65536];
static int sprite_bit[65536];
static uae_u32 clxtab[256];

/* Video buffer description structure. Filled in by the graphics system
 * dependent code. */
struct vidbuf_description gfxvidinfo;

/* OCS/ECS color lookup table. */
xcolnr xcolors[4096];

struct spritepixelsbuf {
	uae_u8 attach;
	uae_u8 stdata;
	uae_u16 data;
};
static struct spritepixelsbuf spritepixels_buffer[MAX_PIXELS_PER_LINE];
static struct spritepixelsbuf *spritepixels;
static int sprite_first_x, sprite_last_x;

/* AGA mode color lookup tables */
#ifndef ARMV6_ASSEMBLY
unsigned int xredcolors[256], xgreencolors[256], xbluecolors[256];
#endif
static int dblpf_ind1_aga[256], dblpf_ind2_aga[256];

struct color_entry colors_for_drawing;
static struct color_entry direct_colors_for_drawing;

static xcolnr *p_acolors;
static xcolnr *p_xcolors;

/* The size of these arrays is pretty arbitrary; it was chosen to be "more
   than enough".  The coordinates used for indexing into these arrays are
   almost, but not quite, Amiga coordinates (there's a constant offset).  */
static union pixdata_u {
  uae_u8 apixels[MAX_PIXELS_PER_LINE * 2];
  uae_u16 apixels_w[MAX_PIXELS_PER_LINE * 2 / sizeof (uae_u16)];
  uae_u32 apixels_l[MAX_PIXELS_PER_LINE * 2 / sizeof (uae_u32)];
} pixdata;

uae_u16 spixels[2 * MAX_SPR_PIXELS];

/* Eight bits for every pixel.  */
union sps_union spixstate;

static uae_u16 ham_linebuf[MAX_PIXELS_PER_LINE * 2];

static uae_u8 *xlinebuffer;

static int *amiga2aspect_line_map, *native2amiga_line_map;
static uae_u8 **row_map;
static uae_u8 row_tmp[MAX_PIXELS_PER_LINE * 32 / 8];
static int max_drawn_amiga_line;

/* line_draw_funcs: pfield_do_linetoscr, pfield_do_fill_line, decode_ham */
typedef void (*line_draw_func)(int, int, bool);

static bool screenlocked = false;
static int next_line_to_render = 0;
static int linestate_first_undecided = 0;
static bool nextline_as_previous = false;

uae_u8 line_data[(MAXVPOS + 2) * 2][MAX_PLANES * MAX_WORDS_PER_LINE * 2];

/* The visible window: VISIBLE_LEFT_BORDER contains the left border of the visible
   area, VISIBLE_RIGHT_BORDER the right border.  These are in window coordinates.  */
static int visible_left_border, visible_right_border;

static int linetoscr_x_adjust_pixbytes, linetoscr_x_adjust_pixels;
static int thisframe_y_adjust;
static int thisframe_y_adjust_real, max_ypos_thisframe, min_ypos_for_screen;

#define MAX_STOP 30000

/* These are generated by the drawing code from the line_decisions array for
   each line that needs to be drawn.  These are basically extracted out of
   bit fields in the hardware registers.  */
static int bplehb, bplham, bpldualpf, bpldualpfpri, bpldualpf2of, bplplanecnt, ecsshres;
static int bplbypass;
static int bplres;
static int plf1pri, plf2pri, bplxor, bpland;
static uae_u32 plf_sprite_mask;
static uae_u32 plf_sprite_mask_n16;
static int sbasecol[2] = { 16, 16 };
static int hposblank;

bool picasso_requested_on, picasso_requested_forced_on, picasso_on;

int inhibit_frame;

int framecnt = 0;

STATIC_INLINE void count_frame (void)
{
  framecnt++;
  if (framecnt > currprefs.gfx_framerate)
		framecnt = 0;
  if (inhibit_frame)
    framecnt = 1;
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
  return x + 2*DISPLAY_LEFT_SHIFT - 2*DIW_DDF_OFFSET;
}

int coord_native_to_amiga_y (int y)
{
	return native2amiga_line_map[y] + thisframe_y_adjust - minfirstline;
}

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

static struct decision *dp_for_drawing;
static struct draw_info *dip_for_drawing;

/*
 * Screen update macros/functions
 */

/* The important positions in the line: where do we start drawing the left border,
   where do we start drawing the playfield, where do we start drawing the right border.
   All of these are forced into the visible window (VISIBLE_LEFT_BORDER .. VISIBLE_RIGHT_BORDER).
   PLAYFIELD_START and PLAYFIELD_END are in window coordinates.  */
static int playfield_start, playfield_end;
static int real_playfield_start, real_playfield_end;
static int sprite_playfield_start;
static bool may_require_hard_way;
static int native_ddf_left, native_ddf_right;

static int pixels_offset;
static int src_pixel;
/* How many pixels in window coordinates which are to the left of the left border.  */
static int unpainted;

STATIC_INLINE xcolnr getbgc (bool blank)
{
	return (blank || hposblank || ce_is_borderblank(colors_for_drawing.extra)) ? 0 : colors_for_drawing.acolors[0];
}

static void set_res_shift(int shift)
{
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
	int ddf_left = dp_for_drawing->plfleft * 2 + DIW_DDF_OFFSET;
	int ddf_right = dp_for_drawing->plfright * 2 + DIW_DDF_OFFSET;
	int leftborderhidden;
	bool expanded = false;

	if (border)
		ddf_left = DISPLAY_LEFT_SHIFT;

	/* Compute datafetch start/stop in pixels; native display coordinates.  */
	native_ddf_left = coord_hw_to_window_x (ddf_left);
	native_ddf_right = coord_hw_to_window_x (ddf_right);

	if (native_ddf_left < 0)
		native_ddf_left = 0;

	if (native_ddf_right < native_ddf_left)
		native_ddf_right = native_ddf_left;

	int linetoscr_diw_start = dp_for_drawing->diwfirstword;
	int linetoscr_diw_end = dp_for_drawing->diwlastword;

	/* Perverse cases happen. */
	if (linetoscr_diw_end < linetoscr_diw_start)
		linetoscr_diw_end = linetoscr_diw_start;

	set_res_shift(lores_shift - bplres);

	playfield_start = linetoscr_diw_start;
	playfield_end = linetoscr_diw_end;

	if (playfield_start < native_ddf_left)
		playfield_start = native_ddf_left;
	if (playfield_end > native_ddf_right)
		playfield_end = native_ddf_right;

	if (playfield_start < visible_left_border)
		playfield_start = visible_left_border;
	if (playfield_start > visible_right_border)
		playfield_start = visible_right_border;
	if (playfield_end < visible_left_border)
		playfield_end = visible_left_border;
	if (playfield_end > visible_right_border)
		playfield_end = visible_right_border;

	real_playfield_start = playfield_start;
	sprite_playfield_start = playfield_start;
	real_playfield_end = playfield_end;

	// Sprite hpos don't include DIW_DDF_OFFSET and can appear 1 lores pixel
	// before first bitplane pixel appears.
	// This means "bordersprite" condition is possible under OCS/ECS too. Argh!
	if (dip_for_drawing->nr_sprites) {
		if (!ce_is_borderblank(colors_for_drawing.extra)) {
			/* bordersprite off or not supported: sprites are visible until diw_end */
			if (playfield_end < linetoscr_diw_end && MAX_STOP > playfield_end) {
				playfield_end = linetoscr_diw_end;
			}
			int left = coord_hw_to_window_x (dp_for_drawing->plfleft * 2);
			if (left < visible_left_border)
				left = visible_left_border;
			if (left < playfield_start && left >= linetoscr_diw_start) {
				playfield_start = left;
			}
		} else {
			sprite_playfield_start = 0;
			if (playfield_end < linetoscr_diw_end && MAX_STOP > playfield_end) {
				playfield_end = linetoscr_diw_end;
			}
		}
	}

	// if BPLCON4 is non-zero: it will affect background color until end of DIW.
	if (dp_for_drawing->xor_seen) {
		if (playfield_end < linetoscr_diw_end && MAX_STOP > playfield_end) {
			playfield_end = linetoscr_diw_end;
			expanded = true;
		}
	}
	may_require_hard_way = false;
	if (dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra) && dip_for_drawing->nr_sprites) {
		int min = visible_right_border, max = visible_left_border, i;
		for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
			int x;
			x = curr_sprite_entries[dip_for_drawing->first_sprite_entry + i].pos;
			if (x < min)
				min = x;
			// include max extra pixels, sprite may be 2x or 4x size: 4x - 1.
			x = curr_sprite_entries[dip_for_drawing->first_sprite_entry + i].max + (4 - 1);
			if (x > max)
				max = x;
		}
		min = coord_hw_to_window_x (min >> sprite_buffer_res) + (DIW_DDF_OFFSET << lores_shift);
		max = coord_hw_to_window_x (max >> sprite_buffer_res) + (DIW_DDF_OFFSET << lores_shift);

		if (min < playfield_start)
			playfield_start = min;
		if (playfield_start < visible_left_border)
			playfield_start = visible_left_border;
		if (max > playfield_end)
			playfield_end = max;
		if (playfield_end > visible_right_border)
			playfield_end = visible_right_border;
		sprite_playfield_start = 0;
		may_require_hard_way = true;
	}

	unpainted = visible_left_border < playfield_start ? 0 : visible_left_border - playfield_start;
  unpainted = res_shift_from_window (unpainted);

	int first_x = sprite_first_x;
	int last_x = sprite_last_x;
	if (first_x < last_x) {
		if (dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra)) {
			if (first_x > visible_left_border)
				first_x = visible_left_border;
			if (last_x < visible_right_border)
				last_x = visible_right_border;
		}
		if (first_x < 0)
			first_x = 0;
		if (last_x > MAX_PIXELS_PER_LINE - 2)
			last_x = MAX_PIXELS_PER_LINE - 2;
		if (first_x < last_x)
			memset (spritepixels + first_x, 0, sizeof (struct spritepixelsbuf) * (last_x - first_x + 1));
	}

  sprite_last_x = 0;
  sprite_first_x = MAX_PIXELS_PER_LINE - 1;

	/* Now, compute some offsets.  */
	ddf_left -= DISPLAY_LEFT_SHIFT;
	if (ddf_left < 0)
		ddf_left = 0;
	pixels_offset = MAX_PIXELS_PER_LINE - (ddf_left << bplres);
	ddf_left <<= bplres;

  leftborderhidden = playfield_start - native_ddf_left;
	src_pixel = MAX_PIXELS_PER_LINE + res_shift_from_window (leftborderhidden);

	if (dip_for_drawing->nr_sprites == 0 && !expanded)
		return;

	/* We need to clear parts of apixels.  */
	if (linetoscr_diw_start < native_ddf_left) {
		int size = res_shift_from_window (native_ddf_left - linetoscr_diw_start);
		linetoscr_diw_start = native_ddf_left;
		memset (pixdata.apixels + MAX_PIXELS_PER_LINE - size, 0, size);
	}
	if (linetoscr_diw_end > native_ddf_right) {
		int pos = res_shift_from_window (native_ddf_right - native_ddf_left);
		int size = res_shift_from_window (linetoscr_diw_end - native_ddf_right);
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


STATIC_INLINE void fill_line_16 (uae_u8 *buf, int start, int stop, bool blank)
{
	uae_u16 *b = (uae_u16 *)buf;
	unsigned int i;
	unsigned int rem = 0;
	xcolnr col = getbgc (blank);
	if (((uintptr_t)&b[start]) & 3)
		b[start++] = (uae_u16) col;
	if (start >= stop)
		return;
	if (((uintptr_t)&b[stop]) & 3) {
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

static void pfield_do_fill_line (int start, int stop, bool blank)
{
	if (stop <= start)
		return;
	fill_line_16 (xlinebuffer, start, stop, blank);
}

static void fill_line2 (int startpos, int len)
{
	int shift;
	int nints, nrem;
	int *start;
	xcolnr val;

		shift = 1;

	nints = len >> (2 - shift);
	nrem = nints & 7;
	nints &= ~7;
	start = (int *)(((uae_u8*)xlinebuffer) + (startpos << shift));
	val = getbgc (false);
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

STATIC_INLINE void fill_line_border (int lineno)
{
	int lastpos = visible_left_border;
	int endpos = visible_left_border + gfxvidinfo.drawbuffer.outwidth;

	// full hblank
	if (hposblank) {
		hposblank = 3;
		fill_line2(lastpos, gfxvidinfo.drawbuffer.outwidth);
		return;
	}

	// hblank not visible
	if (0 <= lastpos && MAX_STOP >= endpos) {
		fill_line2(lastpos, gfxvidinfo.drawbuffer.outwidth);
		return;
	}

	if (lastpos < endpos) {
		pfield_do_fill_line(lastpos, endpos, true);
	}
}

static uae_u8 render_sprites (int pos, int dualpf, uae_u8 apixel, int aga)
{
	struct spritepixelsbuf *spb = &spritepixels[pos];
	unsigned int v = spb->data;
	int *shift_lookup = bpldualpfpri ? dblpf_ms2 : dblpf_ms1;
	int maskshift, plfmask;

	/* The value in the shift lookup table is _half_ the shift count we
	need.  This is because we can't shift 32 bits at once (undefined
	behaviour in C).  */

  if(dualpf) {
	  maskshift = shift_lookup[apixel];
	  plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
	  v &= ~plfmask;
  } else {
    if(apixel)
      v &= plf_sprite_mask_n16;
  }

	/* Extra 1 sprite pixel at DDFSTRT is only possible if at least 1 plane is active */
	if ((bplplanecnt > 0 || pos >= sprite_playfield_start) && (v != 0)) {
		unsigned int col;

    if (spb->attach) {
      int offs = sprite_bit[v];
      if (spb->stdata & offs) {
        if (aga)
          col = sprite_col_at[v] + sbasecol[1];
        else
          col = sprite_col_at[v] + 16;
        return col;
      }
    }

		if (aga) {
      int offs = sprite_bit[v];
      if (offs & 0x55)
				col = sprite_col_nat[v] + sbasecol[0];
			else
				col = sprite_col_nat[v] + sbasecol[1];
		} else {
			col = sprite_col_nat[v] + 16;
		}

		return col;
	}

	return 0;
}

#include "linetoscr.cpp"

/* ECS SuperHires special cases */

#define PUTBPIX(x) buf[dpix] = (x);

STATIC_INLINE uae_u32 shsprite (int dpix, uae_u32 spix_val, uae_u32 v, int spr)
{
	uae_u8 sprcol;
	uae_u16 scol;
	if (!spr)
		return v;
	sprcol = render_sprites (dpix, 0, spix_val, 0);
	if (!sprcol)
		return v;
	/* good enough for now.. */ 
	scol = colors_for_drawing.color_regs_ecs[sprcol] & 0xccc;
	scol |= scol >> 2;
	return xcolors[scol];
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
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], spr));
		dpix++;
 		v = (colors_for_drawing.color_regs_ecs[off] & 0x333) << 2;
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val2, xcolors[v], spr));
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
	  v = (colors_for_drawing.color_regs_ecs[off] & 0xccc);
	  v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], spr));
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
		v = (colors_for_drawing.color_regs_ecs[off] & 0xccc);
		v |= v >> 2;
		PUTBPIX(shsprite (dpix, spix_val1, xcolors[v], spr));
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

typedef int(*call_linetoscr)(int spix, int dpix, int dpix_end);

static call_linetoscr pfield_do_linetoscr_normal;
static call_linetoscr pfield_do_linetoscr_sprite;
static call_linetoscr pfield_do_linetoscr_spriteonly;

static void pfield_do_linetoscr(int start, int stop, bool blank)
{
	src_pixel = pfield_do_linetoscr_normal(src_pixel, start, stop);
}
static void pfield_do_linetoscr_spr(int start, int stop, bool blank)
{
	src_pixel = pfield_do_linetoscr_sprite(src_pixel, start, stop);
}
static int pfield_do_nothing(int a, int b, int c)
{
	return a;
}

static void pfield_set_linetoscr (void)
{
	p_acolors = colors_for_drawing.acolors;
	p_xcolors = xcolors;
	bpland = 0xff;
	if (bplbypass) {
		p_acolors = direct_colors_for_drawing.acolors;
	}
	spritepixels = spritepixels_buffer;
	pfield_do_linetoscr_spriteonly = pfield_do_nothing;

	if (currprefs.chipset_mask & CSMASK_AGA) {
		if (res_shift == 0) {
				pfield_do_linetoscr_normal = linetoscr_16_aga;
				pfield_do_linetoscr_sprite = linetoscr_16_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_16_aga_spronly;
		} else if (res_shift == 2) {
		} else if (res_shift == 1) {
				pfield_do_linetoscr_normal = linetoscr_16_stretch1_aga;
				pfield_do_linetoscr_sprite = linetoscr_16_stretch1_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_16_stretch1_aga_spronly;
		} else if (res_shift == -1) {
				pfield_do_linetoscr_normal = linetoscr_16_shrink1_aga;
				pfield_do_linetoscr_sprite = linetoscr_16_shrink1_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_16_shrink1_aga_spronly;
		} else if (res_shift == -2) {
				pfield_do_linetoscr_normal = linetoscr_16_shrink2_aga;
				pfield_do_linetoscr_sprite = linetoscr_16_shrink2_aga_spr;
				pfield_do_linetoscr_spriteonly = linetoscr_16_shrink2_aga_spronly;
		}
	}

	if (!(currprefs.chipset_mask & CSMASK_AGA) && ecsshres) {
		if (res_shift == 0) {
				pfield_do_linetoscr_normal = linetoscr_16_sh;
				pfield_do_linetoscr_sprite = linetoscr_16_sh_spr;
		} else if (res_shift == -1) {
				pfield_do_linetoscr_normal = linetoscr_16_shrink1_sh;
				pfield_do_linetoscr_sprite = linetoscr_16_shrink1_sh_spr;
		} else if (res_shift == -2) {
				pfield_do_linetoscr_normal = linetoscr_16_shrink2_sh;
				pfield_do_linetoscr_sprite = linetoscr_16_shrink2_sh_spr;
		}
	}

	if (!(currprefs.chipset_mask & CSMASK_AGA) && !ecsshres) {
		if (res_shift == 0) {
				pfield_do_linetoscr_normal = linetoscr_16;
				pfield_do_linetoscr_sprite = linetoscr_16_spr;
		} else if (res_shift == 2) {
		} else if (res_shift == 1) {
				pfield_do_linetoscr_normal = linetoscr_16_stretch1;
				pfield_do_linetoscr_sprite = linetoscr_16_stretch1_spr;
		} else if (res_shift == -1) {
				pfield_do_linetoscr_normal = linetoscr_16_shrink1;
				pfield_do_linetoscr_sprite = linetoscr_16_shrink1_spr;
		}
	}
}

// left or right AGA border sprite
static void pfield_do_linetoscr_bordersprite_aga (int start, int stop, bool blank)
{
	if (blank) {
		pfield_do_fill_line (start, stop, blank);
		return;
	}
	pfield_do_linetoscr_spriteonly(src_pixel, start, stop);
}

static void dummy_worker (int start, int stop, bool blank)
{
}

#ifdef ARMV6T2
STATIC_INLINE int DECODE_HAM8_1(int col, int pv)
{
  __asm__ (
    "ubfx    %[pv], %[pv], #3, #5      \n\t"
    "bfi     %[col], %[pv], #0, #5     \n\t"
    : [col] "+r" (col) , [pv] "+r" (pv) );
  return col;
}
STATIC_INLINE int DECODE_HAM8_2(int col, int pv)
{
  __asm__ (
    "ubfx    %[pv], %[pv], #3, #5      \n\t"
    "bfi     %[col], %[pv], #11, #5    \n\t"
    : [col] "+r" (col) , [pv] "+r" (pv) );
  return col;
}
STATIC_INLINE int DECODE_HAM8_3(int col, int pv)
{
  __asm__ (
    "ubfx    %[pv], %[pv], #2, #6      \n\t"
    "bfi     %[col], %[pv], #5, #6     \n\t"
    : [col] "+r" (col) , [pv] "+r" (pv) );
  return col;
}

STATIC_INLINE int DECODE_HAM6_1(int col, int pv)
{
  __asm__ (
    "bfi     %[col], %[pv], #1, #4     \n\t"
    : [col] "+r" (col) : [pv] "r" (pv) );
  return (col);
}
STATIC_INLINE int DECODE_HAM6_2(int col, int pv)
{
  __asm__ (
    "bfi     %[col], %[pv], #12, #4     \n\t"
    : [col] "+r" (col) : [pv] "r" (pv) );
  return (col);
}
STATIC_INLINE int DECODE_HAM6_3(int col, int pv)
{
  __asm__ (
    "bfi     %[col], %[pv], #7, #4     \n\t"
    : [col] "+r" (col) : [pv] "r" (pv) );
  return (col);
}
#endif

static int ham_decode_pixel;
static uae_u16 ham_lastcolor;

/* Decode HAM in the invisible portion of the display (left of VISIBLE_LEFT_BORDER),
 * but don't draw anything in.  This is done to prepare HAM_LASTCOLOR for later,
 * when decode_ham runs.
 *
 */
static void init_ham_decoding (void)
{
  int unpainted_amiga = unpainted;

  ham_decode_pixel = src_pixel;
	ham_lastcolor = colors_for_drawing.acolors[0];
	
	if (!bplham) {
		if (unpainted_amiga > 0) {
			int pv = pixdata.apixels[ham_decode_pixel + unpainted_amiga - 1];
			if (aga_mode)
				ham_lastcolor = colors_for_drawing.acolors[pv ^ bplxor];
			else
				ham_lastcolor = colors_for_drawing.acolors[pv];
		}
	} else if (aga_mode) {
		if (bplplanecnt >= 7) { /* AGA mode HAM8 */
			while (unpainted_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel++] ^ bplxor;
				switch (pv & 0x3) 
        {
					case 0x0: ham_lastcolor = colors_for_drawing.acolors[pv >> 2]; break;
#ifdef ARMV6T2
					case 0x1: ham_lastcolor = DECODE_HAM8_1(ham_lastcolor, pv); break;
					case 0x2: ham_lastcolor = DECODE_HAM8_2(ham_lastcolor, pv); break;
					case 0x3: ham_lastcolor = DECODE_HAM8_3(ham_lastcolor, pv); break;
#else
				  case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
				  case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
				  case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
#endif
				}
			}
		} else { /* AGA mode HAM6 */
			while (unpainted_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel++] ^ bplxor;
				uae_u32 pc = ((pv & 0xf) << 0) | ((pv & 0xf) << 4);
				switch (pv & 0x30) 
        {
					case 0x00: ham_lastcolor = colors_for_drawing.acolors[pv]; break;
#ifdef ARMV6T2
					case 0x10: ham_lastcolor = DECODE_HAM8_1(ham_lastcolor, pc); break;
					case 0x20: ham_lastcolor = DECODE_HAM8_2(ham_lastcolor, pc); break;
					case 0x30: ham_lastcolor = DECODE_HAM8_3(ham_lastcolor, pc); break;
#else
				  case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pc & 0xF) << 4; break;
				  case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pc & 0xF) << 20; break;
				  case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pc & 0xF) << 12; break;
#endif
				}
			}
		}
	} else {
		/* OCS/ECS mode HAM6 */
		while (unpainted_amiga-- > 0) {
			int pv = pixdata.apixels[ham_decode_pixel++];
			switch (pv & 0x30) 
      {
				case 0x00: ham_lastcolor = colors_for_drawing.acolors[pv]; break;
#ifdef ARMV6T2
				case 0x10: ham_lastcolor = DECODE_HAM6_1(ham_lastcolor, pv); break;
				case 0x20: ham_lastcolor = DECODE_HAM6_2(ham_lastcolor, pv); break;
				case 0x30: ham_lastcolor = DECODE_HAM6_3(ham_lastcolor, pv); break;
#else
			  case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
			  case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
			  case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
#endif
			}
		}
	}
}

static void decode_ham (int pix, int stoppos, bool blank)
{
	int todraw_amiga = res_shift_from_window (stoppos - pix);
	
	if (!bplham) {
		while (todraw_amiga-- > 0) {
			int pv = pixdata.apixels[ham_decode_pixel];
			if (aga_mode)
				ham_lastcolor = colors_for_drawing.acolors[pv ^ bplxor];
			else
				ham_lastcolor = colors_for_drawing.acolors[pv];
			
			ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		}
	} else if (aga_mode) {
		if (bplplanecnt >= 7) { /* AGA mode HAM8 */
			while (todraw_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel] ^ bplxor;
				switch (pv & 0x3) 
        {
					case 0x0: ham_lastcolor = colors_for_drawing.acolors[pv >> 2]; break;
#ifdef ARMV6T2
					case 0x1: ham_lastcolor = DECODE_HAM8_1(ham_lastcolor, pv); break;
					case 0x2: ham_lastcolor = DECODE_HAM8_2(ham_lastcolor, pv); break;
					case 0x3: ham_lastcolor = DECODE_HAM8_3(ham_lastcolor, pv); break;
#else
				  case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
				  case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
				  case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
#endif
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		} else { /* AGA mode HAM6 */
			while (todraw_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel] ^ bplxor;
				uae_u32 pc = ((pv & 0xf) << 0) | ((pv & 0xf) << 4);
				switch (pv & 0x30) 
        {
					case 0x00: ham_lastcolor = colors_for_drawing.acolors[pv]; break;
#ifdef ARMV6T2
					case 0x10: ham_lastcolor = DECODE_HAM8_1(ham_lastcolor, pc); break;
					case 0x20: ham_lastcolor = DECODE_HAM8_2(ham_lastcolor, pc); break;
					case 0x30: ham_lastcolor = DECODE_HAM8_3(ham_lastcolor, pc); break;
#else
				  case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pc & 0xF) << 4; break;
				  case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pc & 0xF) << 20; break;
				  case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pc & 0xF) << 12; break;
#endif
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		}
	} else {
		/* OCS/ECS mode HAM6 */
		while (todraw_amiga-- > 0) {
			int pv = pixdata.apixels[ham_decode_pixel];
			switch (pv & 0x30) 
      {
				case 0x00: ham_lastcolor = colors_for_drawing.acolors[pv]; break;
#ifdef ARMV6T2
				case 0x10: ham_lastcolor = DECODE_HAM6_1(ham_lastcolor, pv); break;
				case 0x20: ham_lastcolor = DECODE_HAM6_2(ham_lastcolor, pv); break;
				case 0x30: ham_lastcolor = DECODE_HAM6_3(ham_lastcolor, pv); break;
#else
			  case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
			  case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
			  case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
#endif
			}
			ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		}
	}
}

static void gen_pfield_tables (void)
{
	int i;

	for (i = 0; i < 256; i++) {
		int plane1 = ((i >> 0) & 1) | ((i >> 1) & 2) | ((i >> 2) & 4) | ((i >> 3) & 8);
		int plane2 = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4) | ((i >> 4) & 8);

		dblpf_2nd1[i] = plane1 == 0 && plane2 != 0;
		dblpf_2nd2[i] = plane2 != 0;
		
		dblpf_ind1_aga[i] = plane1 == 0 ? plane2 : plane1;
		dblpf_ind2_aga[i] = plane2 == 0 ? plane1 : plane2;
		
		dblpf_ms1[i] = plane1 == 0 ? (plane2 == 0 ? 16 : 8) : 0;
		dblpf_ms2[i] = plane2 == 0 ? (plane1 == 0 ? 16 : 0) : 8;
		if (plane2 > 0)
			plane2 += 8;
		dblpf_ind1[i] = i >= 128 ? i & 0x7F : (plane1 == 0 ? plane2 : plane1);
		dblpf_ind2[i] = i >= 128 ? i & 0x7F : (plane2 == 0 ? plane1 : plane2);

		// Hack for OCS/ECS-only dualplayfield chipset bug.
		// If PF2P2 is invalid (>5), playfield color becomes transparent but
		// playfield still hides playfield under it! (if plfpri is set)
		if (i & 64) {
			dblpf_ind2[i] = 0;
			dblpf_ind1[i] = 0;
		}

		clxtab[i] = ((((i & 3) && (i & 12)) << 9)
				| (((i & 3) && (i & 48)) << 10)
				| (((i & 3) && (i & 192)) << 11)
				| (((i & 12) && (i & 48)) << 12)
				| (((i & 12) && (i & 192)) << 13)
				| (((i & 48) && (i & 192)) << 14));
	}

  for(i=0; i<65536; ++i)
  {
    sprite_col_nat[i] = 
      (i & 0x0003) ? ((i >> 0) & 3) + 0 :
      (i & 0x000C) ? ((i >> 2) & 3) + 0 :
      (i & 0x0030) ? ((i >> 4) & 3) + 4 :
      (i & 0x00C0) ? ((i >> 6) & 3) + 4 :
      (i & 0x0300) ? ((i >> 8) & 3) + 8 :
      (i & 0x0C00) ? ((i >> 10) & 3) + 8 :
      (i & 0x3000) ? ((i >> 12) & 3) + 12 :
      (i & 0xC000) ? ((i >> 14) & 3) + 12 : 0;
    sprite_col_at[i] =
      (i & 0x000F) ? ((i >> 0) & 0x000F) :
      (i & 0x00F0) ? ((i >> 4) & 0x000F) :
      (i & 0x0F00) ? ((i >> 8) & 0x000F) :
      (i & 0xF000) ? ((i >> 12) & 0x000F) : 0;
    sprite_bit[i] = 
      (i & 0x0003) ? 0x01 : 
      (i & 0x000C) ? 0x02 : 
      (i & 0x0030) ? 0x04 :
      (i & 0x00C0) ? 0x08 :
      (i & 0x0300) ? 0x10 :
      (i & 0x0C00) ? 0x20 :
      (i & 0x3000) ? 0x40 :
      (i & 0xC000) ? 0x80 : 0;
  }
}

/* When looking at this function and the ones that inline it, bear in mind
   what an optimizing compiler will do with this code.  All callers of this
   function only pass in constant arguments (except for E).  This means
   that many of the if statements will go away completely after inlining.  */
STATIC_INLINE void draw_sprites_1 (struct sprite_entry *e, const int dualpf, const int has_attach)
{
  uae_u16 *buf = spixels + e->first_pixel;
  uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
  int spr_pos, pos;
	int epos = e->pos;
	int emax = e->max;

  buf -= epos;
  stbuf -= epos;

  spr_pos = epos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) << sprite_buffer_res);

	if (spr_pos < sprite_first_x)
		sprite_first_x = spr_pos;

  for (pos = epos; pos < emax; pos++, spr_pos++) {
		if (spr_pos >= 0 && spr_pos < MAX_PIXELS_PER_LINE) {
			spritepixels[spr_pos].data = buf[pos];
			spritepixels[spr_pos].stdata = stbuf[pos];
			spritepixels[spr_pos].attach = has_attach;
		}
  }	 

	if (spr_pos > sprite_last_x)
		sprite_last_x = spr_pos;
}

/* See comments above.  Do not touch if you don't know what's going on.
* (We do _not_ want the following to be inlined themselves).  */
/* lores bitplane, lores sprites */
static void NOINLINE draw_sprites_normal_sp_nat (struct sprite_entry *e) { draw_sprites_1 (e, 0, 0); }
static void NOINLINE draw_sprites_normal_dp_nat (struct sprite_entry *e) { draw_sprites_1 (e, 1, 0); }
static void NOINLINE draw_sprites_normal_sp_at (struct sprite_entry *e) { draw_sprites_1 (e, 0, 1); }
static void NOINLINE draw_sprites_normal_dp_at (struct sprite_entry *e) { draw_sprites_1 (e, 1, 1); }

/* not very optimized */
STATIC_INLINE void draw_sprites_aga (struct sprite_entry *e, int aga)
{
	draw_sprites_1 (e, bpldualpf, e->has_attached);
}

STATIC_INLINE void draw_sprites_ecs (struct sprite_entry *e)
{
	if (e->has_attached) {
		if (bpldualpf)
			draw_sprites_normal_dp_at (e);
		else
			draw_sprites_normal_sp_at (e);
	} else {
		if (bpldualpf)
			draw_sprites_normal_dp_nat (e);
		else
			draw_sprites_normal_sp_nat (e);
	}
}

/* clear possible bitplane data outside DIW area */
static void clear_bitplane_border_aga (void)
{
	int len, shift = res_shift;
	uae_u8 v = 0;

	if (shift < 0) {
		shift = -shift;
		len = (real_playfield_start - playfield_start) << shift;
		memset (pixdata.apixels + pixels_offset + (playfield_start << shift), v, len);
		len = (playfield_end - real_playfield_end) << shift;
		memset (pixdata.apixels + pixels_offset + (real_playfield_end << shift), v, len);
	} else {
		len = (real_playfield_start - playfield_start) >> shift;
		memset (pixdata.apixels + pixels_offset + (playfield_start >> shift), v, len);
		len = (playfield_end - real_playfield_end) >> shift;
		memset (pixdata.apixels + pixels_offset + (real_playfield_end >> shift), v, len);
	}
}

static void weird_bitplane_fix (int start, int end)
{
	int sh = lores_shift;
	uae_u8 *p = pixdata.apixels + pixels_offset;

	start >>= sh;
	end >>= sh;
	if (bplplanecnt == 5 && !bpldualpf) {
		/* emulate OCS/ECS only undocumented "SWIV" hardware feature */
		for (int i = start; i < end; i++) {
			if (p[i] & 16)
				p[i] = 16;
		}
	} else if (bpldualpf && bpldualpfpri) {
		/* in dualplayfield mode this feature is even more strange.. */
		for (int i = start; i < end; i++) {
			if (p[i] & (2 | 8 | 32))
				p[i] |= 0x40;
		}
	} else if (bpldualpf && !bpldualpfpri) {
		for (int i = start; i < end; i++) {
			p[i] &= ~(2 | 8 | 32);
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

#define GETLONG(P) (*(uae_u32 *)P)

#define DATA_POINTER(n) (line_data[lineno] + (n) * MAX_WORDS_PER_LINE * 2)

#if defined(USE_ARMNEON) && !defined(ANDROID)
// FIXME: these neon helper functions caused text rel  problem on android 

#ifdef __cplusplus
  extern "C" {
#endif
    void ARM_doline_n1(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n2(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n3(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n4(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n6(uae_u32 *pixels, int wordcount, int lineno);
    void NEON_doline_n8(uae_u32 *pixels, int wordcount, int lineno);
#ifdef __cplusplus
  }
#endif

static void pfield_doline_n0 (uae_u32 *pixels, int wordcount, int lineno)
{
	memset(pixels, 0, wordcount << 5);
}

#define MERGE_0(a,b,mask,shift) {\
   uae_u32 tmp = mask & (b>>shift); \
   a = tmp; \
   b ^= (tmp << shift); \
}

static void pfield_doline_n5 (uae_u32 *pixels, int wordcount, int lineno)
{
  uae_u8 *real_bplpt[5];

   real_bplpt[0] = DATA_POINTER (0);
   real_bplpt[1] = DATA_POINTER (1);
   real_bplpt[2] = DATA_POINTER (2);
   real_bplpt[3] = DATA_POINTER (3);
   real_bplpt[4] = DATA_POINTER (4);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
      b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE_0(b2, b3, 0x55555555, 1);
      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE_0(b0, b2, 0x33333333, 2);
      MERGE_0(b1, b3, 0x33333333, 2);
      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE (b0, b4, 0x0f0f0f0f, 4);
      MERGE (b1, b5, 0x0f0f0f0f, 4);
      MERGE (b2, b6, 0x0f0f0f0f, 4);
      MERGE (b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      do_put_mem_long(pixels, b0);
      do_put_mem_long(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      do_put_mem_long(pixels + 2, b1);
      do_put_mem_long(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      do_put_mem_long(pixels + 1, b4);
      do_put_mem_long(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      do_put_mem_long(pixels + 3, b5);
      do_put_mem_long(pixels + 7, b7);
      pixels += 8;
   }
}

static void pfield_doline_n7 (uae_u32 *pixels, int wordcount, int lineno)
{
  uae_u8 *real_bplpt[7];
   real_bplpt[0] = DATA_POINTER (0);
   real_bplpt[1] = DATA_POINTER (1);
   real_bplpt[2] = DATA_POINTER (2);
   real_bplpt[3] = DATA_POINTER (3);
   real_bplpt[4] = DATA_POINTER (4);
   real_bplpt[5] = DATA_POINTER (5);
   real_bplpt[6] = DATA_POINTER (6);

   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;
      b1 = GETLONG ((uae_u32 *)real_bplpt[6]); real_bplpt[6] += 4;
      b2 = GETLONG ((uae_u32 *)real_bplpt[5]); real_bplpt[5] += 4;
      b3 = GETLONG ((uae_u32 *)real_bplpt[4]); real_bplpt[4] += 4;
      b4 = GETLONG ((uae_u32 *)real_bplpt[3]); real_bplpt[3] += 4;
      b5 = GETLONG ((uae_u32 *)real_bplpt[2]); real_bplpt[2] += 4;
      b6 = GETLONG ((uae_u32 *)real_bplpt[1]); real_bplpt[1] += 4;
      b7 = GETLONG ((uae_u32 *)real_bplpt[0]); real_bplpt[0] += 4;

      MERGE_0(b0, b1, 0x55555555, 1);
      MERGE (b2, b3, 0x55555555, 1);
      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE (b0, b2, 0x33333333, 2);
      MERGE (b1, b3, 0x33333333, 2);
      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE (b0, b4, 0x0f0f0f0f, 4);
      MERGE (b1, b5, 0x0f0f0f0f, 4);
      MERGE (b2, b6, 0x0f0f0f0f, 4);
      MERGE (b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      do_put_mem_long(pixels, b0);
      do_put_mem_long(pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      do_put_mem_long(pixels + 2, b1);
      do_put_mem_long(pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      do_put_mem_long(pixels + 1, b4);
      do_put_mem_long(pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      do_put_mem_long(pixels + 3, b5);
      do_put_mem_long(pixels + 7, b7);
      pixels += 8;
   }
}

typedef void (*pfield_doline_func)(uae_u32 *, int, int);

static pfield_doline_func pfield_doline_n[9]={
	pfield_doline_n0, ARM_doline_n1, NEON_doline_n2, NEON_doline_n3,
	NEON_doline_n4, pfield_doline_n5, NEON_doline_n6, pfield_doline_n7,
	NEON_doline_n8
};

#else

static uae_u8 *real_bplpt[8];

STATIC_INLINE void pfield_doline_1 (uae_u32 *pixels, int wordcount, int planes)
{
   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;

	    b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;
	    switch (planes) {
	      case 8: b0 = GETLONG (real_bplpt[7]); real_bplpt[7] += 4;
	      case 7: b1 = GETLONG (real_bplpt[6]); real_bplpt[6] += 4;
	      case 6: b2 = GETLONG (real_bplpt[5]); real_bplpt[5] += 4;
	      case 5: b3 = GETLONG (real_bplpt[4]); real_bplpt[4] += 4;
	      case 4: b4 = GETLONG (real_bplpt[3]); real_bplpt[3] += 4;
	      case 3: b5 = GETLONG (real_bplpt[2]); real_bplpt[2] += 4;
	      case 2: b6 = GETLONG (real_bplpt[1]); real_bplpt[1] += 4;
	      case 1: b7 = GETLONG (real_bplpt[0]); real_bplpt[0] += 4;
    	}

      MERGE (b0, b1, 0x55555555, 1);
      MERGE (b2, b3, 0x55555555, 1);
      MERGE (b4, b5, 0x55555555, 1);
      MERGE (b6, b7, 0x55555555, 1);

      MERGE (b0, b2, 0x33333333, 2);
      MERGE (b1, b3, 0x33333333, 2);
      MERGE (b4, b6, 0x33333333, 2);
      MERGE (b5, b7, 0x33333333, 2);

      MERGE (b0, b4, 0x0f0f0f0f, 4);
      MERGE (b1, b5, 0x0f0f0f0f, 4);
      MERGE (b2, b6, 0x0f0f0f0f, 4);
      MERGE (b3, b7, 0x0f0f0f0f, 4);

      MERGE (b0, b1, 0x00ff00ff, 8);
      MERGE (b2, b3, 0x00ff00ff, 8);
      MERGE (b4, b5, 0x00ff00ff, 8);
      MERGE (b6, b7, 0x00ff00ff, 8);

      MERGE (b0, b2, 0x0000ffff, 16);
      do_put_mem_long (pixels, b0);
      do_put_mem_long (pixels + 4, b2);
      MERGE (b1, b3, 0x0000ffff, 16);
      do_put_mem_long (pixels + 2, b1);
      do_put_mem_long (pixels + 6, b3);
      MERGE (b4, b6, 0x0000ffff, 16);
      do_put_mem_long (pixels + 1, b4);
      do_put_mem_long (pixels + 5, b6);
      MERGE (b5, b7, 0x0000ffff, 16);
      do_put_mem_long (pixels + 3, b5);
      do_put_mem_long (pixels + 7, b7);
      pixels += 8;
   }
}

/* See above for comments on inlining.  These functions should _not_
   be inlined themselves.  */
static void NOINLINE pfield_doline_n1 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 1); }
static void NOINLINE pfield_doline_n2 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 2); }
static void NOINLINE pfield_doline_n3 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 3); }
static void NOINLINE pfield_doline_n4 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 4); }
static void NOINLINE pfield_doline_n5 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 5); }
static void NOINLINE pfield_doline_n6 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 6); }
static void NOINLINE pfield_doline_n7 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 7); }
static void NOINLINE pfield_doline_n8 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 8); }

#endif /* USE_ARMNEON */

static void pfield_doline (int lineno)
{
  int wordcount = dp_for_drawing->plflinelen;
  uae_u32 *data = pixdata.apixels_l + MAX_PIXELS_PER_LINE / 4;
      
#if defined(USE_ARMNEON) && !defined(ANDROID)
  pfield_doline_n[bplplanecnt](data, wordcount, lineno);
#else
  real_bplpt[0] = DATA_POINTER (0);
  real_bplpt[1] = DATA_POINTER (1);
  real_bplpt[2] = DATA_POINTER (2);
  real_bplpt[3] = DATA_POINTER (3);
  real_bplpt[4] = DATA_POINTER (4);
  real_bplpt[5] = DATA_POINTER (5);
  real_bplpt[6] = DATA_POINTER (6);
  real_bplpt[7] = DATA_POINTER (7);

  switch (bplplanecnt) {
    default: break;
    case 0: memset (data, 0, wordcount * 32); break;
    case 1: pfield_doline_n1 (data, wordcount); break;
    case 2: pfield_doline_n2 (data, wordcount); break;
    case 3: pfield_doline_n3 (data, wordcount); break;
    case 4: pfield_doline_n4 (data, wordcount); break;
    case 5: pfield_doline_n5 (data, wordcount); break;
    case 6: pfield_doline_n6 (data, wordcount); break;
    case 7: pfield_doline_n7 (data, wordcount); break;
    case 8: pfield_doline_n8 (data, wordcount); break;
  }
#endif /* USE_ARMNEON */
}

void init_row_map (void)
{
	static uae_u8 *oldbufmem;
	static int oldheight, oldpitch;
	int i;

	if (!row_map) {
		row_map = xmalloc(uae_u8*, max_uae_height + 1);
	}

	if (oldbufmem && oldbufmem == gfxvidinfo.drawbuffer.bufmem &&
		oldheight == gfxvidinfo.drawbuffer.outheight &&
		oldpitch == gfxvidinfo.drawbuffer.rowbytes)
		return;
	int j = oldheight == 0 ? max_uae_height : oldheight;
	for (i = gfxvidinfo.drawbuffer.outheight; i < max_uae_height + 1 && i < j + 1; i++) {
		row_map[i] = row_tmp;
	}
	for (i = 0, j = 0; i < gfxvidinfo.drawbuffer.outheight; i++, j += gfxvidinfo.drawbuffer.rowbytes) {
		row_map[i] = gfxvidinfo.drawbuffer.bufmem + j;
	}
	oldbufmem = gfxvidinfo.drawbuffer.bufmem;
	oldheight = gfxvidinfo.drawbuffer.outheight;
	oldpitch = gfxvidinfo.drawbuffer.rowbytes;
}

static void init_aspect_maps(void)
{
	int i, h;

	linedbl = currprefs.gfx_vresolution;
	int maxl = (MAXVPOS + 1) << linedbl;
	min_ypos_for_screen = minfirstline << linedbl;
	max_drawn_amiga_line = -1;

	h = gfxvidinfo.drawbuffer.outheight;
	if (h == 0)
		/* Do nothing if the gfx driver hasn't initialized the screen yet */
		return;

	if (native2amiga_line_map)
		xfree(native2amiga_line_map);
	if (amiga2aspect_line_map)
		xfree(amiga2aspect_line_map);

	/* At least for this array the +1 is necessary. */
	amiga2aspect_line_map = xmalloc(int, (MAXVPOS + 1) * 2 + 1);
	native2amiga_line_map = xmalloc(int, h);

	for (i = 0; i < maxl; i++) {
		int v = i - min_ypos_for_screen;
		if (v >= h && max_drawn_amiga_line < 0)
			max_drawn_amiga_line = v;
		if (i < min_ypos_for_screen || v >= h)
			v = -1;
		amiga2aspect_line_map[i] = v;
	}
	if (max_drawn_amiga_line < 0)
		max_drawn_amiga_line = maxl - min_ypos_for_screen;

	for (i = 0; i < h; i++)
		native2amiga_line_map[i] = -1;

	for (i = maxl - 1; i >= min_ypos_for_screen; i--) {
		if (amiga2aspect_line_map[i] == -1)
			continue;
		for (int j = amiga2aspect_line_map[i]; j < h && native2amiga_line_map[j] == -1; j++)
			native2amiga_line_map[j] = i + currprefs.vertical_offset >> linedbl;
	}
}

/*
 * One drawing frame has been finished. Tell the graphics code about it.
 */
STATIC_INLINE void do_flush_screen ()
{
  unlockscr ();
  screenlocked = false;
}

/* We only save hardware registers during the hardware frame. Now, when
 * drawing the frame, we expand the data into a slightly more useful
 * form. */
static void pfield_expand_dp_bplcon (void)
{
	bool pfield_mode_changed = false;

  bplres = dp_for_drawing->bplres;
  bplplanecnt = dp_for_drawing->nr_planes;
  bplham = dp_for_drawing->ham_seen;
  if (aga_mode) {
  	bplehb = (dp_for_drawing->bplcon0 & 0x7010) == 0x6000;
    bpldualpf2of = (dp_for_drawing->bplcon3 >> 10) & 7;
    sbasecol[0] = ((dp_for_drawing->bplcon4 >> 4) & 15) << 4;
    sbasecol[1] = ((dp_for_drawing->bplcon4 >> 0) & 15) << 4;
    bplxor = dp_for_drawing->bplcon4 >> 8;
  } else
  	bplehb = (dp_for_drawing->bplcon0 & 0xFC00) == 0x6000 || (dp_for_drawing->bplcon0 & 0xFC00) == 0x7000;
	if ((currprefs.chipset_mask & CSMASK_ECS_DENISE) && (dp_for_drawing->bplcon2 & 0x0200))
		bplehb = 0;
	int oecsshres = ecsshres;
	ecsshres = bplres == RES_SUPERHIRES && (currprefs.chipset_mask & CSMASK_ECS_DENISE) && !(currprefs.chipset_mask & CSMASK_AGA);
	pfield_mode_changed = oecsshres != ecsshres;
    
  plf1pri = dp_for_drawing->bplcon2 & 7;
  plf2pri = (dp_for_drawing->bplcon2 >> 3) & 7;
  plf_sprite_mask = 0xFFFF0000 << (4 * plf2pri);
  plf_sprite_mask |= (0x0000FFFF << (4 * plf1pri)) & 0xFFFF;
  plf_sprite_mask_n16 = ~(plf_sprite_mask >> 16);
  bpldualpf = (dp_for_drawing->bplcon0 & 0x400) == 0x400;
  bpldualpfpri = (dp_for_drawing->bplcon2 & 0x40) == 0x40;

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

	if (pfield_mode_changed)
		pfield_set_linetoscr();
}

static bool isham (uae_u16 bplcon0)
{
  int p = GET_PLANES (bplcon0);
  if (!(bplcon0 & 0x800))
	  return 0;
  if (aga_mode) {
	  // AGA only has 6 or 8 plane HAM
	  if (p == 6 || p == 8)
	    return 1;
  } else {
	  // OCS/ECS also supports 5 plane HAM
	  if (GET_RES_DENISE (bplcon0) > 0)
	    return 0;
	  if (p >= 5)
	    return 1;
  }
  return 0;
}

static void pfield_expand_dp_bplconx (int regno, int v)
{
	if (regno == 0xffff) {
		hposblank = 1;
		return;
	}
  regno -= 0x1000;
  switch (regno)
  {
	  case 0x100: // BPLCON0
      dp_for_drawing->bplcon0 = v;
      dp_for_drawing->bplres = GET_RES_DENISE(v);
      dp_for_drawing->nr_planes = GET_PLANES(v);
	    dp_for_drawing->ham_seen = isham (v);
      break;
	  case 0x104: // BPLCON2
      dp_for_drawing->bplcon2 = v;
      break;
  	case 0x106: // BPLCON3
      dp_for_drawing->bplcon3 = v;
      break;
    case 0x10c: // BPLCON4
      dp_for_drawing->bplcon4 = v;
      break;
  }
  pfield_expand_dp_bplcon();
	set_res_shift(lores_shift - bplres);
}

static int drawing_color_matches;
static enum { color_match_acolors, color_match_full } color_match_type;

/* Set up colors_for_drawing to the state at the beginning of the currently drawn
   line.  Try to avoid copying color tables around whenever possible.  */
static void adjust_drawing_colors (int ctable, int need_full)
{
	if (drawing_color_matches != ctable || need_full < 0) {
  	if (need_full) {
			color_reg_cpy (&colors_for_drawing, curr_color_tables + ctable);
			color_match_type = color_match_full;
  	} else {
			memcpy (colors_for_drawing.acolors, curr_color_tables[ctable].acolors,
			sizeof colors_for_drawing.acolors);
			colors_for_drawing.extra = curr_color_tables[ctable].extra;
			color_match_type = color_match_acolors;
  	}
		drawing_color_matches = ctable;
  } else if (need_full && color_match_type != color_match_full) {
		color_reg_cpy (&colors_for_drawing, &curr_color_tables[ctable]);
		color_match_type = color_match_full;
  }
}

static void playfield_hard_way(line_draw_func worker_pfield, int first, int last)
{
	if (first < real_playfield_start)  {
		int next = last < real_playfield_start ? last : real_playfield_start;
		int diff = next - first;
		pfield_do_linetoscr_bordersprite_aga(first, next, false);
		if (res_shift >= 0)
			diff >>= res_shift;
		else
			diff <<= res_shift;
		src_pixel += diff;
		first = next;
	}
	(*worker_pfield)(first, last < real_playfield_end ? last : real_playfield_end, false);
	if (last > real_playfield_end)
		pfield_do_linetoscr_bordersprite_aga(real_playfield_end, last, false);
}

STATIC_INLINE void do_color_changes (line_draw_func worker_border, line_draw_func worker_pfield)
{
  int i;
  int lastpos = visible_left_border;
  int endpos = visible_right_border;

  for (i = dip_for_drawing->first_color_change; i <= dip_for_drawing->last_color_change; i++) {
		int regno = curr_color_changes[i].regno;
		unsigned int value = curr_color_changes[i].value;
	  int nextpos, nextpos_in_range;

	  if (i == dip_for_drawing->last_color_change)
      nextpos = endpos;
	  else
			nextpos = shres_coord_hw_to_window_x (curr_color_changes[i].linepos);

	  nextpos_in_range = nextpos;
    if (nextpos > endpos)
      nextpos_in_range = endpos;

		// left border (hblank end to playfield start)
	  if (nextpos_in_range > lastpos && lastpos < playfield_start) {
		  int t = nextpos_in_range <= playfield_start ? nextpos_in_range : playfield_start;
		  (*worker_border) (lastpos, t, false);
		  lastpos = t;
	  }

		// playfield
	  if (nextpos_in_range > lastpos && lastpos >= playfield_start && lastpos < playfield_end) {
		  int t = nextpos_in_range <= playfield_end ? nextpos_in_range : playfield_end;
			if (plf2pri > 5 && !(currprefs.chipset_mask & CSMASK_AGA))
				weird_bitplane_fix (lastpos, t);
			if (bplxor && may_require_hard_way && worker_pfield != pfield_do_linetoscr_bordersprite_aga)
				playfield_hard_way(worker_pfield, lastpos, t);
			else
		  (*worker_pfield) (lastpos, t, false);
		  lastpos = t;
	  }

		// right border (playfield end to hblank start)
	  if (nextpos_in_range > lastpos && lastpos >= playfield_end) {
			(*worker_border) (lastpos, nextpos_in_range, false);
		  lastpos = nextpos_in_range;
	  }

    if (regno >= 0x1000) {
      pfield_expand_dp_bplconx (regno, value);
		} else if (regno >= 0 && !(value & COLOR_CHANGE_MASK)) {
      color_reg_set (&colors_for_drawing, regno, value);
      colors_for_drawing.acolors[regno] = getxcolor (value);
		} else if (regno == 0 && (value & COLOR_CHANGE_MASK)) {
			if (value & COLOR_CHANGE_BRDBLANK) {
				colors_for_drawing.extra &= ~(1 << CE_BORDERBLANK);
				colors_for_drawing.extra &= ~(1 << CE_BORDERSPRITE);
				colors_for_drawing.extra |= (value & 1) != 0 ? (1 << CE_BORDERBLANK) : 0;
				colors_for_drawing.extra |= (value & 3) == 2 ? (1 << CE_BORDERSPRITE) : 0;
      }
	  }
	  if (lastpos >= endpos)
		  break;
  }
}

STATIC_INLINE bool is_color_changes(struct draw_info *di)
{
	int regno = curr_color_changes[di->first_color_change].regno;
	int changes = di->nr_color_changes;
	return changes > 1 || (changes == 1 && regno != 0xffff && regno != -1);
}

static void pfield_draw_line(int lineno, int gfx_ypos, int follow_ypos)
{
	int border = 0;
	int do_double = 0;
	bool have_color_changes;

	dp_for_drawing = line_decisions + lineno;
	dip_for_drawing = curr_drawinfo + lineno;

	if (currprefs.gfx_vresolution && !interlace_seen) {
		if (nextline_as_previous) {
			nextline_as_previous = false;
			return;
		}
		nextline_as_previous = true;
		if (follow_ypos >= 0)
			do_double = 1;
	}
	if (dp_for_drawing->plfleft < 0)
		border = 1;

	have_color_changes = is_color_changes(dip_for_drawing);

	xlinebuffer = row_map[gfx_ypos];
	xlinebuffer -= linetoscr_x_adjust_pixbytes;

	if (border == 0) {

		pfield_expand_dp_bplcon();
		pfield_init_linetoscr(false);
		pfield_doline(lineno);

		adjust_drawing_colors(dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb || ecsshres);

		/* The problem is that we must call decode_ham() BEFORE we do the sprites. */
		if (dp_for_drawing->ham_seen) {
			int ohposblank = hposblank;
			uae_u8 b0 = dp_for_drawing->bplcon0;
			uae_u8 b2 = dp_for_drawing->bplcon2;
			uae_u8 b3 = dp_for_drawing->bplcon3;
			uae_u8 b4 = dp_for_drawing->bplcon4;
			init_ham_decoding();
			do_color_changes(dummy_worker, decode_ham);
			if (have_color_changes) {
				// do_color_changes() did color changes and register changes, restore them.
				adjust_drawing_colors(dp_for_drawing->ctable, -1);
				dp_for_drawing->bplcon0 = b0;
				dp_for_drawing->bplcon2 = b2;
				dp_for_drawing->bplcon3 = b3;
				dp_for_drawing->bplcon4 = b4;
				pfield_expand_dp_bplcon();
			}
			hposblank = ohposblank;
			ham_decode_pixel = src_pixel;
			bplham = dp_for_drawing->ham_at_start;
		}

		if (dip_for_drawing->nr_sprites) {
			int i;
			if (ce_is_bordersprite(colors_for_drawing.extra) && dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra))
				clear_bitplane_border_aga();

			for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
				if (currprefs.chipset_mask & CSMASK_AGA)
					draw_sprites_aga(curr_sprite_entries + dip_for_drawing->first_sprite_entry + i, 1);
				else
					draw_sprites_ecs(curr_sprite_entries + dip_for_drawing->first_sprite_entry + i);
			}
		}

		if (dip_for_drawing->nr_sprites && ce_is_bordersprite(colors_for_drawing.extra) && !ce_is_borderblank(colors_for_drawing.extra) && dp_for_drawing->bordersprite_seen)
			do_color_changes(pfield_do_linetoscr_bordersprite_aga, pfield_do_linetoscr_spr);
		else
			do_color_changes(pfield_do_fill_line, dip_for_drawing->nr_sprites ? pfield_do_linetoscr_spr : pfield_do_linetoscr);

		if (do_double) {
			memcpy(row_map[follow_ypos], row_map[gfx_ypos], gfxvidinfo.drawbuffer.pixbytes * gfxvidinfo.drawbuffer.outwidth);
		}

		if (dip_for_drawing->nr_sprites)
			pfield_erase_hborder_sprites();

	}
	else { // border > 0: top or bottom border

		bool dosprites = false;

		adjust_drawing_colors(dp_for_drawing->ctable, 0);

		if (dp_for_drawing->bordersprite_seen && !ce_is_borderblank(colors_for_drawing.extra) && dip_for_drawing->nr_sprites) {
			dosprites = true;
			pfield_expand_dp_bplcon();
			pfield_init_linetoscr(true);
			pfield_erase_vborder_sprites();
		}

		if (!dosprites && !have_color_changes) {
			if (dp_for_drawing->plfleft < -1) {
				// blanked border line
				int tmp = hposblank;
				hposblank = 1;
				fill_line_border(lineno);
				hposblank = tmp;
			}
			else {
				// normal border line
				fill_line_border(lineno);
			}

			if (do_double) {
				xlinebuffer = row_map[follow_ypos] - linetoscr_x_adjust_pixbytes;
				fill_line_border(lineno);
			}
			return;
		}

		if (dosprites) {

			for (int i = 0; i < dip_for_drawing->nr_sprites; i++)
				draw_sprites_aga(curr_sprite_entries + dip_for_drawing->first_sprite_entry + i, 1);
			do_color_changes(pfield_do_linetoscr_bordersprite_aga, pfield_do_linetoscr_bordersprite_aga);

		}
		else {

			playfield_start = visible_right_border;
			playfield_end = visible_right_border;
			do_color_changes(pfield_do_fill_line, pfield_do_fill_line);

		}

		if (do_double) {
			memcpy(row_map[follow_ypos], row_map[gfx_ypos], gfxvidinfo.drawbuffer.pixbytes * gfxvidinfo.drawbuffer.outwidth);
		}

	}
}

static void center_image(void)
{
	const int deltaToBorder = (gfxvidinfo.drawbuffer.outwidth >> currprefs.gfx_resolution) - 320;

	visible_left_border = 73 - (deltaToBorder >> 1);
	visible_right_border = 393 + (deltaToBorder >> 1);
	visible_left_border <<= lores_shift;
	visible_right_border <<= lores_shift;

	linetoscr_x_adjust_pixels = visible_left_border;
	linetoscr_x_adjust_pixbytes = linetoscr_x_adjust_pixels * gfxvidinfo.drawbuffer.pixbytes;

	int max_drawn_amiga_line_tmp = max_drawn_amiga_line;
	if (max_drawn_amiga_line_tmp > gfxvidinfo.drawbuffer.outheight)
		max_drawn_amiga_line_tmp = gfxvidinfo.drawbuffer.outheight;
	max_drawn_amiga_line_tmp >>= linedbl;

	thisframe_y_adjust = minfirstline + currprefs.vertical_offset;

	/* Make sure the value makes sense */
	if (thisframe_y_adjust + max_drawn_amiga_line_tmp > maxvpos + maxvpos / 2)
		thisframe_y_adjust = maxvpos + maxvpos / 2 - max_drawn_amiga_line_tmp;
	if (thisframe_y_adjust < 0)
		thisframe_y_adjust = 0;

	thisframe_y_adjust_real = thisframe_y_adjust << linedbl;
	max_ypos_thisframe = (maxvpos_display - minfirstline + 1) << linedbl;
}

static void init_drawing_frame (void)
{
	lores_reset();

	init_hardware_for_drawing_frame();

	linestate_first_undecided = 0;
	nextline_as_previous = false;

	center_image();

	drawing_color_matches = -1;
}

static void draw_status_line(int line, int statusy)
{
	xlinebuffer = row_map[line];
	uae_u8 *buf = xlinebuffer;
	draw_status_line_single(buf, statusy, gfxvidinfo.drawbuffer.outwidth);
}

static void partial_draw_frame(void)
{
	if (framecnt == 0) {
		if (!screenlocked) {
			if (!lockscr())
				return;
			screenlocked = true;
		}

		struct vidbuffer *vb = &gfxvidinfo.drawbuffer;
		for (; next_line_to_render < max_ypos_thisframe; ++next_line_to_render) {
			const int i1 = next_line_to_render + min_ypos_for_screen;
			const int line = next_line_to_render + thisframe_y_adjust_real;
			const int whereline = amiga2aspect_line_map[i1];
			const int wherenext = amiga2aspect_line_map[i1 + 1];

			if (whereline >= vb->outheight || line >= linestate_first_undecided)
				break;
			if (whereline < 0)
				continue;

			hposblank = 0;
			pfield_draw_line(line, whereline, wherenext);
		}
	}
}

void halt_draw_frame(void)
{
  if(screenlocked) {
    unlockscr();
    screenlocked = false;
  }
}

static void finish_drawing_frame (void)
{
	int i;
	struct vidbuffer *vb = &gfxvidinfo.drawbuffer;

	if (!screenlocked) {
		if (!lockscr())
			return;
		screenlocked = true;
	}

	for (i = next_line_to_render; i < max_ypos_thisframe; i++) {
		const int i1 = i + min_ypos_for_screen;
		const int line = i + thisframe_y_adjust_real;
		const int whereline = amiga2aspect_line_map[i1];
		const int wherenext = amiga2aspect_line_map[i1 + 1];

		if (whereline >= vb->outheight || line >= linestate_first_undecided)
			break;
		if (whereline < 0)
			continue;

		hposblank = 0;
		pfield_draw_line(line, whereline, wherenext);
	}

	if (currprefs.leds_on_screen) {
		for (i = 0; i < TD_TOTAL_HEIGHT; i++) {
			int line = gfxvidinfo.drawbuffer.outheight - TD_TOTAL_HEIGHT + i;
			draw_status_line(line, i);
		}
	}

	if (currprefs.cs_cd32fmv) {
		if (cd32_fmv_active) {
			cd32_fmv_genlock(vb, &gfxvidinfo.drawbuffer);
		}
	}

	do_flush_screen();
	next_line_to_render = 0;
}

void check_prefs_picasso(void)
{
#ifdef PICASSO96
	if (picasso_on)
		picasso_refresh();

	if (picasso_requested_on == picasso_on && !picasso_requested_forced_on)
		return;

	picasso_requested_forced_on = false;
	picasso_on = picasso_requested_on;

	if (!picasso_on)
		clear_inhibit_frame(IHF_PICASSO);
	else
		set_inhibit_frame(IHF_PICASSO);

	gfx_set_picasso_state(picasso_on);
	picasso_enablescreen(picasso_requested_on);

	notice_new_xcolors();
	count_frame();
#endif
}

bool vsync_handle_check(void)
{
	const int changed = check_prefs_changed_gfx();
	if (changed) {
		reset_drawing();
		notice_new_xcolors();
	}
	device_check_config();
	return changed != 0;
}

void vsync_handle_redraw(void)
{
	if (framecnt == 0) {
#ifdef USE_SDL1
		if (render_tid) {
			while (render_thread_busy)
				sleep_millis(1);
			write_comm_pipe_u32(render_pipe, RENDER_SIGNAL_FRAME_DONE, 1);
			uae_sem_wait(&render_sem);
		}
#elif USE_SDL2
		finish_drawing_frame();
#endif
	}

	if (quit_program < 0) {
#ifdef USE_SDL1
		if (render_tid) {
			while (render_thread_busy)
				sleep_millis(1);
			write_comm_pipe_u32(render_pipe, RENDER_SIGNAL_QUIT, 1);
			while (render_tid != 0) {
				sleep_millis(10);
			}
			destroy_comm_pipe(render_pipe);
			xfree(render_pipe);
			render_pipe = 0;
			uae_sem_destroy(&render_sem);
			render_sem = 0;
		}
#endif

		quit_program = -quit_program;
		set_inhibit_frame(IHF_QUIT_PROGRAM);
		set_special(SPCFLAG_BRK | SPCFLAG_MODE_CHANGE);
		return;
	}

	count_frame();

	if (framecnt == 0)
		init_drawing_frame();

	gui_flicker_led(-1, 0, 0);
}

void hsync_record_line_state(int lineno)
{
	if (framecnt != 0)
		return;

	linestate_first_undecided = lineno + 1;
#ifdef USE_SDL1
	if (render_tid && linestate_first_undecided > 3 && !render_thread_busy) {
		if (currprefs.gfx_vresolution) {
			if (!(linestate_first_undecided & 0x3e))
				write_comm_pipe_u32(render_pipe, RENDER_SIGNAL_PARTIAL, 1);
		}
		else if (!(linestate_first_undecided & 0x1f))
			write_comm_pipe_u32(render_pipe, RENDER_SIGNAL_PARTIAL, 1);
	}
#endif
}

bool notice_interlace_seen(bool lace)
{
	bool changed = false;
	// non-lace to lace switch (non-lace active at least one frame)?
	if (lace) {
		if (interlace_seen == 0) {
			changed = true;
		}
		interlace_seen = currprefs.gfx_vresolution ? 1 : -1;
	}
	else {
		if (interlace_seen) {
			changed = true;
		}
		interlace_seen = 0;
	}
	return changed;
}

void reset_drawing(void)
{
	lores_reset();

	linestate_first_undecided = 0;
	nextline_as_previous = false;

	init_aspect_maps();

	init_row_map();

	memset(spixels, 0, sizeof spixels);
	memset(&spixstate, 0, sizeof spixstate);

	init_drawing_frame();
	pfield_set_linetoscr();
}

static void gen_direct_drawing_table(void)
{
	// BYPASS color table
	for (int i = 0; i < 256; i++) {
		uae_u32 v = (i << 16) | (i << 8) | i;
		direct_colors_for_drawing.acolors[i] = CONVERT_RGB(v);
	}
}
#ifdef USE_SDL1
static void *render_thread(void *unused)
{
	for (;;) {
		render_thread_busy = false;
		uae_u32 signal = read_comm_pipe_u32_blocking(render_pipe);
		render_thread_busy = true;
		switch (signal) {
		case RENDER_SIGNAL_PARTIAL:
			partial_draw_frame();
			break;

		case RENDER_SIGNAL_FRAME_DONE:
			finish_drawing_frame();
			uae_sem_post(&render_sem);
			break;

		case RENDER_SIGNAL_QUIT:
			render_tid = nullptr;
			return nullptr;
		}
	}
}
#endif
void drawing_init(void)
{
	gen_pfield_tables();

	gen_direct_drawing_table();
#ifdef USE_SDL1
	if (render_pipe == 0) {
		render_pipe = xmalloc(smp_comm_pipe, 1);
		init_comm_pipe(render_pipe, 20, 1);
	}
	if (render_sem == 0) {
		uae_sem_init(&render_sem, 0, 0);
	}
	if (render_tid == 0 && render_pipe != 0 && render_sem != 0) {
		uae_start_thread(_T("render"), render_thread, NULL, &render_tid);
	}
#endif
#ifdef PICASSO96
	if (!isrestore()) {
		picasso_on = 0;
		picasso_requested_on = 0;
		gfx_set_picasso_state(0);
	}
#endif
	xlinebuffer = gfxvidinfo.drawbuffer.bufmem;

	inhibit_frame = 0;

	reset_drawing();
}
