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
#include "td-sdl/thread.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "autoconf.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#ifdef JIT
#include "compemu.h"
#endif
#include "savestate.h"
#include <sys/time.h>
#include <time.h>


int lores_shift;

static void lores_reset (void)
{
    lores_shift = 0;
}

int aga_mode; /* mirror of chipset_mask & CSMASK_AGA */
int direct_rgb; 

#ifdef PANDORA
#define OFFSET_Y_ADJUST 15
#else
#define OFFSET_Y_ADJUST 0
#endif

/* The shift factor to apply when converting between Amiga coordinates and window
   coordinates.  Zero if the resolution is the same, positive if window coordinates
   have a higher resolution (i.e. we're stretching the image), negative if window
   coordinates have a lower resolution (i.e. we're shrinking the image).  */
static int res_shift;

extern SDL_Surface *prSDLScreen;

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
static int dblpfofs[] = { 0, 2, 4, 8, 16, 32, 64, 128 };

static int sprite_col_nat[65536];
static int sprite_col_at[65536];
static int sprite_bit[65536];
static uae_u32 clxtab[256];

/* Video buffer description structure. Filled in by the graphics system
 * dependent code. */
struct vidbuf_description gfxvidinfo;

/* OCS/ECS color lookup table. */
xcolnr xcolors[4096];

static uae_u8 spritepixels[MAX_PIXELS_PER_LINE * 2]; /* used when sprite resolution > lores */

#ifdef AGA
/* AGA mode color lookup tables */
unsigned int xredcolors[256], xgreencolors[256], xbluecolors[256];
static int dblpf_ind1_aga[256], dblpf_ind2_aga[256];
#else
static uae_u8 spriteagadpfpixels[1];
static int dblpf_ind1_aga[1], dblpf_ind2_aga[1];
#endif

struct color_entry colors_for_drawing;

/* The size of these arrays is pretty arbitrary; it was chosen to be "more
   than enough".  The coordinates used for indexing into these arrays are
   almost, but not quite, Amiga coordinates (there's a constant offset).  */
union pixdata_u {
    /* Let's try to align this thing. */
    double uupzuq;
    long int cruxmedo;
    uae_u8 apixels[MAX_PIXELS_PER_LINE * 2];
    uae_u16 apixels_w[MAX_PIXELS_PER_LINE * 2 / sizeof (uae_u16)];
    uae_u32 apixels_l[MAX_PIXELS_PER_LINE * 2 / sizeof (uae_u32)];
} pixdata;

#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
uae_u16 *spixels;
#else
uae_u16 spixels[MAX_SPR_PIXELS];
#endif

/* Eight bits for every pixel.  */
union sps_union spixstate;

static uae_u32 ham_linebuf[MAX_PIXELS_PER_LINE * 2];

uae_u8 *xlinebuffer;

static int *amiga2aspect_line_map, *native2amiga_line_map;
static uae_u8 *row_map[MAX_VIDHEIGHT + 1];
static uae_u8 row_tmp[MAX_PIXELS_PER_LINE * 32 / 8];
static int max_drawn_amiga_line;

/* line_draw_funcs: pfield_do_linetoscr, pfield_do_fill_line, decode_ham */
typedef void (*line_draw_func)(int, int);

#define LINE_UNDECIDED 1
#define LINE_DECIDED 2
#define LINE_DECIDED_DOUBLE 3
#define LINE_AS_PREVIOUS 4
#define LINE_BLACK 5
#define LINE_REMEMBERED_AS_BLACK 6
#define LINE_DONE 7
#define LINE_DONE_AS_PREVIOUS 8
#define LINE_REMEMBERED_AS_PREVIOUS 9

static int linestate_first_undecided = 0;

uae_u8 line_data[(MAXVPOS + 1) * 2][MAX_PLANES * MAX_WORDS_PER_LINE * 2];

/* The visible window: VISIBLE_LEFT_BORDER contains the left border of the visible
   area, VISIBLE_RIGHT_BORDER the right border.  These are in window coordinates.  */
static int visible_left_border, visible_right_border;
static int linetoscr_x_adjust_bytes;
static int thisframe_y_adjust;
static int thisframe_y_adjust_real, max_ypos_thisframe, min_ypos_for_screen;
static int extra_y_adjust;

/* These are generated by the drawing code from the line_decisions array for
   each line that needs to be drawn.  These are basically extracted out of
   bit fields in the hardware registers.  */
static int bplehb, bplham, bpldualpf, bpldualpfpri, bpldualpf2of, bplplanecnt, bplres;
static int plf1pri, plf2pri;
static uae_u32 plf_sprite_mask;
static int sbasecol[2] = { 16, 16 };

int picasso_requested_on;
int picasso_on;

int inhibit_frame;

int framecnt = 0, fs_framecnt = 0;
static int picasso_redraw_necessary;

/* Calculate idle time (time to wait for vsync) */
int idletime_frames = 0;
unsigned long idletime_time = 0;
int idletime_percent = 0;
#define IDLETIME_FRAMES 25
unsigned long time_per_frame = 20000; // Default for PAL (50 Hz): 20000 ns

void adjust_idletime(unsigned long ns_waited)
{
  idletime_frames++;
  idletime_time += ns_waited;
  if(idletime_frames >= IDLETIME_FRAMES)
  {
    unsigned long ns_per_frames = time_per_frame * idletime_frames * (1 + currprefs.gfx_framerate);
    idletime_percent = idletime_time * 100 / ns_per_frames;
    if(idletime_percent < 0)
      idletime_percent = 0;
    else if(idletime_percent > 100)
      idletime_percent = 100;
    idletime_time = 0;
    idletime_frames = 0;
  }
}

static __inline__ void count_frame (void)
{
	switch(currprefs.gfx_framerate)
	{
		case 0: // draw every frame (Limiting is done by waiting for vsync...)
			fs_framecnt = 0;
			break;
			
		case 1: // draw every second frame
			fs_framecnt++;
			if (fs_framecnt > 1)
				fs_framecnt = 0;
      break;  
  }
  if (inhibit_frame)
    fs_framecnt = 1;
}

int coord_native_to_amiga_x (int x)
{
  if(gfxvidinfo.width > 600)
  {
    x += (visible_left_border << 1);
    return x + 2*DISPLAY_LEFT_SHIFT - 2*DIW_DDF_OFFSET;
  }
  else
  {
    x += visible_left_border;
    x <<= (1 - lores_shift);
    return x + 2*DISPLAY_LEFT_SHIFT - 2*DIW_DDF_OFFSET;
  }
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

void notice_screen_contents_lost (void)
{
  picasso_redraw_necessary = 1;
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
static int pixels_offset;
static int src_pixel;
/* How many pixels in window coordinates which are to the left of the left border.  */
static int unpainted;

#include "linetoscr.c"

static void pfield_do_linetoscr_0_640 (int start, int stop)
{
	int local_res_shift = 1 - bplres; // stretch LORES, nothing for HIRES, shrink for SUPERHIRES
	start = start << 1;
	stop = stop << 1;

	if (local_res_shift == 0)
		src_pixel = linetoscr_16 (src_pixel, start, stop);
	else if (local_res_shift == 1)
		src_pixel = linetoscr_16_stretch1 (src_pixel, start, stop);
	else //if (local_res_shift == -1)
		src_pixel = linetoscr_16_shrink1 (src_pixel, start, stop);
}

static void pfield_do_linetoscr_0_640_AGA (int start, int stop)
{
	int local_res_shift = 1 - bplres; // stretch LORES, nothing for HIRES, shrink for SUPERHIRES
	start = start << 1;
	stop = stop << 1;

  if (local_res_shift == 0)
	  src_pixel = linetoscr_16_aga (src_pixel, start, stop);
  else if (local_res_shift == 1)
	  src_pixel = linetoscr_16_stretch1_aga (src_pixel, start, stop);
  else //if (local_res_shift == -1)
	  src_pixel = linetoscr_16_shrink1_aga (src_pixel, start, stop);
}

static void pfield_do_linetoscr_0 (int start, int stop)
{
	if (res_shift == 0)
		src_pixel = linetoscr_16 (src_pixel, start, stop);
	else if (res_shift == 1)
		src_pixel = linetoscr_16_stretch1 (src_pixel, start, stop);
	else //if (res_shift == -1)
		src_pixel = linetoscr_16_shrink1 (src_pixel, start, stop);
}

static void pfield_do_linetoscr_0_AGA (int start, int stop)
{
  if (res_shift == 0)
	  src_pixel = linetoscr_16_aga (src_pixel, start, stop);
  else if (res_shift == 1)
	  src_pixel = linetoscr_16_stretch1_aga (src_pixel, start, stop);
  else //if (res_shift == -1)
	  src_pixel = linetoscr_16_shrink1_aga (src_pixel, start, stop);
}

static void pfield_do_fill_line_0_640(int start, int stop)
{
	register uae_u16 *b = &(((uae_u16 *)xlinebuffer)[start << 1]);
	register xcolnr col = colors_for_drawing.acolors[0];
	register int i;
	register int max=(stop-start) << 1;
	for (i = 0; i < max; i++,b++)
		*b = col;
}

static void pfield_do_fill_line_0(int start, int stop)
{
	register uae_u16 *b = &(((uae_u16 *)xlinebuffer)[start]);
	register xcolnr col = colors_for_drawing.acolors[0];
	register int i;
	register int max=(stop-start);
	for (i = 0; i < max; i++,b++)
		*b = col;
}

static line_draw_func pfield_do_linetoscr=(line_draw_func)pfield_do_linetoscr_0;
static line_draw_func pfield_do_fill_line=(line_draw_func)pfield_do_fill_line_0;

/* Initialize the variables necessary for drawing a line.
 * This involves setting up start/stop positions and display window
 * borders.  */
static void pfield_init_linetoscr (void)
{
	/* First, get data fetch start/stop in DIW coordinates.  */
	int ddf_left = dp_for_drawing->plfleft * 2 + DIW_DDF_OFFSET;
	int ddf_right = dp_for_drawing->plfright * 2 + DIW_DDF_OFFSET;

	/* Compute datafetch start/stop in pixels; native display coordinates.  */
	int native_ddf_left = coord_hw_to_window_x (ddf_left);
	int native_ddf_right = coord_hw_to_window_x (ddf_right);

	int linetoscr_diw_start = dp_for_drawing->diwfirstword;
	int linetoscr_diw_end = dp_for_drawing->diwlastword;

	if (dip_for_drawing->nr_sprites == 0) {
		if (linetoscr_diw_start < native_ddf_left)
			linetoscr_diw_start = native_ddf_left;
		if (linetoscr_diw_end > native_ddf_right)
			linetoscr_diw_end = native_ddf_right;
	}

	/* Perverse cases happen. */
	if (linetoscr_diw_end < linetoscr_diw_start)
		linetoscr_diw_end = linetoscr_diw_start;

	playfield_start = linetoscr_diw_start;
	playfield_end = linetoscr_diw_end;

	if (playfield_start < visible_left_border)
		playfield_start = visible_left_border;
	if (playfield_start > visible_right_border)
		playfield_start = visible_right_border;
	if (playfield_end < visible_left_border)
		playfield_end = visible_left_border;
	if (playfield_end > visible_right_border)
		playfield_end = visible_right_border;

	/* Now, compute some offsets.  */

  res_shift = lores_shift - bplres;
	ddf_left -= DISPLAY_LEFT_SHIFT;
	ddf_left <<= bplres;
	pixels_offset = MAX_PIXELS_PER_LINE - ddf_left;

	unpainted = visible_left_border < playfield_start ? 0 : visible_left_border - playfield_start;
	src_pixel = MAX_PIXELS_PER_LINE + res_shift_from_window (playfield_start - native_ddf_left);
   
	if (dip_for_drawing->nr_sprites == 0)
		return;
	/* Must clear parts of apixels.  */
	if (linetoscr_diw_start < native_ddf_left) {
		int size = res_shift_from_window (native_ddf_left - linetoscr_diw_start);
		linetoscr_diw_start = native_ddf_left;
		memset (pixdata.apixels + MAX_PIXELS_PER_LINE - size, 0, size);
	}
	if (linetoscr_diw_end > native_ddf_right) {
		int pos = res_shift_from_window (native_ddf_right - native_ddf_left);
		int size = res_shift_from_window (linetoscr_diw_end - native_ddf_right);
		linetoscr_diw_start = native_ddf_left;
		memset (pixdata.apixels + MAX_PIXELS_PER_LINE + pos, 0, size);
	}
}

void drawing_adjust_mousepos(int *xp, int *yp)
{
}

STATIC_INLINE void fill_line (void)
{
	int nints;
  int *start;
  xcolnr val;

	nints = gfxvidinfo.width >> 1;
	if(gfxvidinfo.width > 600)
		start = (int *)(((char *)xlinebuffer) + (visible_left_border << 2));
	else
		start = (int *)(((char *)xlinebuffer) + (visible_left_border << 1));

	val = colors_for_drawing.acolors[0];
	val |= val << 16;
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
}

static void dummy_worker (int start, int stop)
{
}

static unsigned int ham_lastcolor;
static int ham_decode_pixel;

/* Decode HAM in the invisible portion of the display (left of VISIBLE_LEFT_BORDER),
   but don't draw anything in.  This is done to prepare HAM_LASTCOLOR for later,
   when decode_ham runs.  */
static void init_ham_decoding (void)
{
	int unpainted_amiga = res_shift_from_window (unpainted);
	ham_decode_pixel = src_pixel;
	ham_lastcolor = color_reg_get (&colors_for_drawing, 0);
	
	if (! bplham || (bplplanecnt != 6 && ((currprefs.chipset_mask & CSMASK_AGA) == 0 || bplplanecnt != 8))) {
		if (unpainted_amiga > 0) {
			int pv = pixdata.apixels[ham_decode_pixel + unpainted_amiga - 1];
			if (currprefs.chipset_mask & CSMASK_AGA)
				ham_lastcolor = colors_for_drawing.color_regs_aga[pv];
			else
				ham_lastcolor = colors_for_drawing.color_regs_ecs[pv];
		}
	} else if (currprefs.chipset_mask & CSMASK_AGA) {
		if (bplplanecnt == 8) { /* AGA mode HAM8 */
			while (unpainted_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel++];
				switch (pv & 0x3) {
					case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pv >> 2]; break;
					case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
					case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
					case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
				}
			}
		} else if (bplplanecnt == 6) { /* AGA mode HAM6 */
			while (unpainted_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel++];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_regs_aga[pv]; break;
					case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pv & 0xF) << 4; break;
					case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pv & 0xF) << 20; break;
					case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pv & 0xF) << 12; break;
				}
			}
		}
	} else {
		if (bplplanecnt == 6) { /* OCS/ECS mode HAM6 */
			while (unpainted_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel++];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_regs_ecs[pv]; break;
					case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
					case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
					case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
				}
			}
		}
	}
}

static void decode_ham (int pix, int stoppos)
{
	int todraw_amiga = res_shift_from_window (stoppos - pix);
	
	if (! bplham || (bplplanecnt != 6 && ((currprefs.chipset_mask & CSMASK_AGA) == 0 || bplplanecnt != 8))) {
		while (todraw_amiga-- > 0) {
			int pv = pixdata.apixels[ham_decode_pixel];
			if (currprefs.chipset_mask & CSMASK_AGA)
				ham_lastcolor = colors_for_drawing.color_regs_aga[pv];
			else
				ham_lastcolor = colors_for_drawing.color_regs_ecs[pv];
			
			ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
		}
	} else if (currprefs.chipset_mask & CSMASK_AGA) {
		if (bplplanecnt == 8) { /* AGA mode HAM8 */
			while (todraw_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel];
				switch (pv & 0x3) {
					case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pv >> 2]; break;
					case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pv & 0xFC); break;
					case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pv & 0xFC) << 16; break;
					case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pv & 0xFC) << 8; break;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		} else if (bplplanecnt == 6) { /* AGA mode HAM6 */
			while (todraw_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_regs_aga[pv]; break;
					case 0x10: ham_lastcolor &= 0xFFFF00; ham_lastcolor |= (pv & 0xF) << 4; break;
					case 0x20: ham_lastcolor &= 0x00FFFF; ham_lastcolor |= (pv & 0xF) << 20; break;
					case 0x30: ham_lastcolor &= 0xFF00FF; ham_lastcolor |= (pv & 0xF) << 12; break;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		}
	} else {
		if (bplplanecnt == 6) { /* OCS/ECS mode HAM6 */
			while (todraw_amiga-- > 0) {
				int pv = pixdata.apixels[ham_decode_pixel];
				switch (pv & 0x30) {
					case 0x00: ham_lastcolor = colors_for_drawing.color_regs_ecs[pv]; break;
					case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;
					case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;
					case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;
				}
				ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
			}
		}
	}
}

static void gen_pfield_tables (void)
{
	int i;

    /* For now, the AGA stuff is broken in the dual playfield case. We encode
     * sprites in dpf mode by ORing the pixel value with 0x80. To make dual
     * playfield rendering easy, the lookup tables contain are made linear for
     * values >= 128. That only works for OCS/ECS, though. */

	for (i = 0; i < 256; i++) {
		int plane1 = (i & 1) | ((i >> 1) & 2) | ((i >> 2) & 4) | ((i >> 3) & 8);
		int plane2 = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4) | ((i >> 4) & 8);

		dblpf_2nd1[i] = plane1 == 0 ? (plane2 == 0 ? 0 : 2) : 1;
		dblpf_2nd2[i] = plane2 == 0 ? (plane1 == 0 ? 0 : 1) : 2;
		
#ifdef AGA
		dblpf_ind1_aga[i] = plane1 == 0 ? plane2 : plane1;
		dblpf_ind2_aga[i] = plane2 == 0 ? plane1 : plane2;
#endif
		
		dblpf_ms1[i] = plane1 == 0 ? (plane2 == 0 ? 16 : 8) : 0;
		dblpf_ms2[i] = plane2 == 0 ? (plane1 == 0 ? 16 : 0) : 8;
		dblpf_ms[i] = i == 0 ? 16 : 8;
		if (plane2 > 0)
			plane2 += 8;
		dblpf_ind1[i] = i >= 128 ? i & 0x7F : (plane1 == 0 ? plane2 : plane1);
		dblpf_ind2[i] = i >= 128 ? i & 0x7F : (plane2 == 0 ? plane1 : plane2);

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

static void draw_sprites_normal_sp_lo_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        col = sprite_col_nat[v] + 16;
        pixdata.apixels[window_pos] = col;
      }
      window_pos++;
   }
}

static void draw_sprites_normal_ham_lo_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        col = sprite_col_nat[v] + 16;
		    ham_linebuf[window_pos] = color_reg_get (&colors_for_drawing, col);
      }
      window_pos++;
   }
}


static void draw_sprites_normal_dp_lo_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        col = sprite_col_nat[v] + 16 + 128;
        pixdata.apixels[window_pos] = col;
      }
      window_pos++;
   }
}

static void draw_sprites_normal_sp_lo_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        int offs;
        offs = sprite_bit[v];
        if (stbuf[pos] & offs) {
          col = sprite_col_at[v] + 16;
    		} else {
          col = sprite_col_nat[v] + 16;
        }
        pixdata.apixels[window_pos] = col;
      }
      window_pos++;
   }
}

static void draw_sprites_normal_ham_lo_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        int offs;
        offs = sprite_bit[v];
        if (stbuf[pos] & offs) {
          col = sprite_col_at[v] + 16;
    		} else {
          col = sprite_col_nat[v] + 16;
        }
		    ham_linebuf[window_pos] = color_reg_get (&colors_for_drawing, col);
      }
      window_pos++;
   }
}

static void draw_sprites_normal_dp_lo_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        int offs;
        offs = sprite_bit[v];
        if (stbuf[pos] & offs) {
          col = sprite_col_at[v] + 16 + 128;
    		} else {
          col = sprite_col_nat[v] + 16 + 128;
        }
        pixdata.apixels[window_pos] = col;
      }
      window_pos++;
   }
}

static void draw_sprites_normal_sp_hi_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos ++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        col = sprite_col_nat[v] + 16;
        pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}

static void draw_sprites_normal_ham_hi_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos ++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        col = sprite_col_nat[v] + 16;
        col = col | (col << 8);
        ham_linebuf[window_pos >> 1] = color_reg_get (&colors_for_drawing, col);
      }
      window_pos += 2;
   }
}


static void draw_sprites_normal_dp_hi_nat(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos ++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        col = sprite_col_nat[v] + 16 + 128;
        pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}

static void draw_sprites_normal_sp_hi_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        int offs;
        offs = sprite_bit[v];
        if (stbuf[pos] & offs) {
          col = sprite_col_at[v] + 16;
    		} else {
          col = sprite_col_nat[v] + 16;
        }
        pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}

static void draw_sprites_normal_ham_hi_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;
   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        int offs;
        offs = sprite_bit[v];
        if (stbuf[pos] & offs) {
          col = sprite_col_at[v] + 16;
    		} else {
          col = sprite_col_nat[v] + 16;
        }
        col = col | (col << 8);
        ham_linebuf[window_pos >> 1] = color_reg_get (&colors_for_drawing, col);
      }
      window_pos += 2;
   }
}

static void draw_sprites_normal_dp_hi_at(struct sprite_entry *_GCCRES_ e)
{
   int *shift_lookup = (bpldualpfpri ? dblpf_ms2 : dblpf_ms1);
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) );
   window_pos <<= 1;
   window_pos += pixels_offset;

   unsigned max=e->max;
   for (pos = e->pos; pos < max; pos++) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        int offs;
        offs = sprite_bit[v];
        if (stbuf[pos] & offs) {
          col = sprite_col_at[v] + 16 + 128;
    		} else {
          col = sprite_col_nat[v] + 16 + 128;
        }
        pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
      }
      window_pos += 2;
   }
}

typedef void (*draw_sprites_func)(struct sprite_entry *_GCCRES_ e);
static draw_sprites_func draw_sprites_dp_hi[2]={
	draw_sprites_normal_dp_hi_nat, draw_sprites_normal_dp_hi_at };
static draw_sprites_func draw_sprites_sp_hi[2]={
	draw_sprites_normal_sp_hi_nat, draw_sprites_normal_sp_hi_at };
static draw_sprites_func draw_sprites_ham_hi[2]={
	draw_sprites_normal_ham_hi_nat, draw_sprites_normal_ham_hi_at };
static draw_sprites_func draw_sprites_dp_lo[2]={
	draw_sprites_normal_dp_lo_nat, draw_sprites_normal_dp_lo_at };
static draw_sprites_func draw_sprites_sp_lo[2]={
	draw_sprites_normal_sp_lo_nat, draw_sprites_normal_sp_lo_at };
static draw_sprites_func draw_sprites_ham_lo[2]={
	draw_sprites_normal_ham_lo_nat, draw_sprites_normal_ham_lo_at };

static draw_sprites_func *draw_sprites_punt = draw_sprites_sp_lo;

/* When looking at this function and the ones that inline it, bear in mind
   what an optimizing compiler will do with this code.  All callers of this
   function only pass in constant arguments (except for E).  This means
   that many of the if statements will go away completely after inlining.  */

/* NOTE: This function is called for AGA modes *only* */
STATIC_INLINE void draw_sprites_aga_1 (struct sprite_entry *e, int ham, int dualpf,
				   int doubling, int skip, int has_attach)
{
   int *shift_lookup = dualpf ? (bpldualpfpri ? dblpf_ms2 : dblpf_ms1) : dblpf_ms;
   uae_u16 *buf = spixels + e->first_pixel;
   uae_u8 *stbuf = spixstate.bytes + e->first_pixel;
   int pos, window_pos;
   uae_u8 xor_val = (uae_u8)(dp_for_drawing->bplcon4 >> 8);

   buf -= e->pos;
   stbuf -= e->pos;

   window_pos = e->pos + ((DIW_DDF_OFFSET - DISPLAY_LEFT_SHIFT) << 1);
   if (skip)
      window_pos >>= 1;
   else if (doubling)
      window_pos <<= 1;
   window_pos += pixels_offset;
   for (pos = e->pos; pos < e->max; pos += 1 << skip) {
      int maskshift, plfmask;
      unsigned int v = buf[pos];

      /* The value in the shift lookup table is _half_ the shift count we
	   need.  This is because we can't shift 32 bits at once (undefined
	   behaviour in C).  */
      maskshift = shift_lookup[pixdata.apixels[window_pos]];
      plfmask = (plf_sprite_mask >> maskshift) >> maskshift;
      v &= ~plfmask;
      if (v != 0) {
        unsigned int col;
        int offs;
        offs = sprite_bit[v];
        if (has_attach && (stbuf[pos] & offs)) {
          col = sprite_col_at[v] + sbasecol[1];
    		} else {
          if(offs & 0x55)
            col = sprite_col_nat[v] + sbasecol[0];
          else
            col = sprite_col_nat[v] + sbasecol[1];
        }

      if (dualpf) {
			  spritepixels[window_pos] = col;
				if (doubling)
					spritepixels[window_pos + 1] = col;
			} else if (ham) {
				col = color_reg_get (&colors_for_drawing, col);
				col ^= xor_val;
				ham_linebuf[window_pos] = col;
				if (doubling)
					ham_linebuf[window_pos + 1] = col;
			} else {
				col ^= xor_val;
				if (doubling)
					pixdata.apixels_w[window_pos >> 1] = col | (col << 8);
				else
					pixdata.apixels[window_pos] = col;
			}
		}
		
		window_pos += 1 << doubling;
	}
}

// ENTRY, ham, dualpf, doubling, skip, has_attach
STATIC_INLINE void draw_sprites_aga_sp_lo_nat(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 0, 0, 0, 1, 0); }
STATIC_INLINE void draw_sprites_aga_dp_lo_nat(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 0, 1, 0, 1, 0); }
STATIC_INLINE void draw_sprites_aga_ham_lo_nat(struct sprite_entry *e)  { draw_sprites_aga_1 (e, 1, 0, 0, 1, 0); }

STATIC_INLINE void draw_sprites_aga_sp_lo_at(struct sprite_entry *e)    { draw_sprites_aga_1 (e, 0, 0, 0, 1, 1); }
STATIC_INLINE void draw_sprites_aga_dp_lo_at(struct sprite_entry *e)    { draw_sprites_aga_1 (e, 0, 1, 0, 1, 1); }
STATIC_INLINE void draw_sprites_aga_ham_lo_at(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 1, 0, 0, 1, 1); }

STATIC_INLINE void draw_sprites_aga_sp_hi_nat(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 0, 0, 0, 0, 0); }
STATIC_INLINE void draw_sprites_aga_dp_hi_nat(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 0, 1, 0, 0, 0); }
STATIC_INLINE void draw_sprites_aga_ham_hi_nat(struct sprite_entry *e)  { draw_sprites_aga_1 (e, 1, 0, 0, 0, 0); }

STATIC_INLINE void draw_sprites_aga_sp_hi_at(struct sprite_entry *e)    { draw_sprites_aga_1 (e, 0, 0, 0, 0, 1); }
STATIC_INLINE void draw_sprites_aga_dp_hi_at(struct sprite_entry *e)    { draw_sprites_aga_1 (e, 0, 1, 0, 0, 1); }
STATIC_INLINE void draw_sprites_aga_ham_hi_at(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 1, 0, 0, 0, 1); }

STATIC_INLINE void draw_sprites_aga_sp_shi_nat(struct sprite_entry *e)  { draw_sprites_aga_1 (e, 0, 0, 1, 0, 0); }
STATIC_INLINE void draw_sprites_aga_dp_shi_nat(struct sprite_entry *e)  { draw_sprites_aga_1 (e, 0, 1, 1, 0, 0); }
STATIC_INLINE void draw_sprites_aga_ham_shi_nat(struct sprite_entry *e) { draw_sprites_aga_1 (e, 1, 0, 1, 0, 0); }

STATIC_INLINE void draw_sprites_aga_sp_shi_at(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 0, 0, 1, 0, 1); }
STATIC_INLINE void draw_sprites_aga_dp_shi_at(struct sprite_entry *e)   { draw_sprites_aga_1 (e, 0, 1, 1, 0, 1); }
STATIC_INLINE void draw_sprites_aga_ham_shi_at(struct sprite_entry *e)  { draw_sprites_aga_1 (e, 1, 0, 1, 0, 1); }

static draw_sprites_func draw_sprites_aga_sp_lo[2]={
	draw_sprites_aga_sp_lo_nat, draw_sprites_aga_sp_lo_at };
static draw_sprites_func draw_sprites_aga_sp_hi[2]={
	draw_sprites_aga_sp_hi_nat, draw_sprites_aga_sp_hi_at };
static draw_sprites_func draw_sprites_aga_sp_shi[2]={
	draw_sprites_aga_sp_shi_nat, draw_sprites_aga_sp_shi_at };

static draw_sprites_func draw_sprites_aga_dp_lo[2]={
	draw_sprites_aga_dp_lo_nat, draw_sprites_aga_dp_lo_at };
static draw_sprites_func draw_sprites_aga_dp_hi[2]={
	draw_sprites_aga_dp_hi_nat, draw_sprites_aga_dp_hi_at };
static draw_sprites_func draw_sprites_aga_dp_shi[2]={
	draw_sprites_aga_dp_shi_nat, draw_sprites_aga_dp_shi_at };

static draw_sprites_func draw_sprites_aga_ham_lo[2]={
	draw_sprites_aga_ham_lo_nat, draw_sprites_aga_ham_lo_at };
static draw_sprites_func draw_sprites_aga_ham_hi[2]={
	draw_sprites_aga_ham_hi_nat, draw_sprites_aga_ham_hi_at };
static draw_sprites_func draw_sprites_aga_ham_shi[2]={
	draw_sprites_aga_ham_shi_nat, draw_sprites_aga_ham_shi_at };

static __inline__ void decide_draw_sprites(void) 
{
  if (currprefs.chipset_mask & CSMASK_AGA)
  {
    int diff = RES_HIRES - bplres;
    if (diff > 0)
    { // doubling = 0, skip = 1
      if(bpldualpf)
      { // ham = 0, dualpf = 1
        draw_sprites_punt = draw_sprites_aga_dp_lo;
      }
      else if(dp_for_drawing->ham_seen)
      { // ham = 1, dualpf = 0
        draw_sprites_punt = draw_sprites_aga_ham_lo;
      }
      else
      { // ham = 0, dualpf = 0
        draw_sprites_punt = draw_sprites_aga_sp_lo;
      }
    }
    else if(diff == 0)
    { // doubling = 0, skip = 0
      if(bpldualpf)
      { // ham = 0, dualpf = 1
        draw_sprites_punt = draw_sprites_aga_dp_hi;
      }
      else if(dp_for_drawing->ham_seen)
      { // ham = 1, dualpf = 0
        draw_sprites_punt = draw_sprites_aga_ham_hi;
      }
      else
      { // ham = 0, dualpf = 0
        draw_sprites_punt = draw_sprites_aga_sp_hi;
      }
    }
    else
    { // doubling = 1, skip = 0
      if(bpldualpf)
      { // ham = 0, dualpf = 1
        draw_sprites_punt = draw_sprites_aga_dp_shi;
      }
      else if(dp_for_drawing->ham_seen)
      { // ham = 1, dualpf = 0
        draw_sprites_punt = draw_sprites_aga_ham_shi;
      }
      else
      { // ham = 0, dualpf = 0
        draw_sprites_punt = draw_sprites_aga_sp_shi;
      }
    }
  }
  else
  {
    if (bplres == RES_LORES)
  		if (bpldualpf)
  			draw_sprites_punt=draw_sprites_dp_lo;
      else if(dp_for_drawing->ham_seen)  		
  			draw_sprites_punt=draw_sprites_ham_lo;
  		else
  			draw_sprites_punt=draw_sprites_sp_lo;
  	else
  		if (bpldualpf)
  			draw_sprites_punt=draw_sprites_dp_hi;
      else if(dp_for_drawing->ham_seen)  		
  			draw_sprites_punt=draw_sprites_ham_hi;
      else
  			draw_sprites_punt=draw_sprites_sp_hi;
	}
}

#define MERGE(a,b,mask,shift) do {\
    uae_u32 tmp = mask & (a ^ (b >> shift)); \
    a ^= tmp; \
    b ^= (tmp << shift); \
} while (0)

#define GETLONG(P) (*(uae_u32 *)P)

#define DATA_POINTER(n) (line_data[lineno] + (n) * MAX_WORDS_PER_LINE * 2)

#ifdef USE_ARMNEON

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

static void pfield_doline_n0 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
{
	memset(pixels, 0, wordcount << 5);
}

#define MERGE_0(a,b,mask,shift) {\
   uae_u32 tmp = mask & (b>>shift); \
   a = tmp; \
   b ^= (tmp << shift); \
}

static void pfield_doline_n5 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
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

static void pfield_doline_n7 (uae_u32 *_GCCRES_ pixels, int wordcount, int lineno)
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

typedef void (*pfield_doline_func)(uae_u32 *_GCCRES_, int, int);

static pfield_doline_func pfield_doline_n[9]={
	pfield_doline_n0, ARM_doline_n1, NEON_doline_n2, NEON_doline_n3,
	NEON_doline_n4, pfield_doline_n5, NEON_doline_n6, pfield_doline_n7,
	NEON_doline_n8
};

#else

uae_u8 *real_bplpt[8];

/* We use the compiler's inlining ability to ensure that PLANES is in effect a compile time
   constant.  That will cause some unnecessary code to be optimized away.
   Don't touch this if you don't know what you are doing.  */
STATIC_INLINE void pfield_doline_1 (uae_u32 *pixels, int wordcount, int planes)
{
   while (wordcount-- > 0) {
      uae_u32 b0,b1,b2,b3,b4,b5,b6,b7;

	    b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;
	    switch (planes) {
#ifdef AGA
	      case 8: b0 = GETLONG (real_bplpt[7]); real_bplpt[7] += 4;
	      case 7: b1 = GETLONG (real_bplpt[6]); real_bplpt[6] += 4;
#endif
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
#ifdef AGA
static void NOINLINE pfield_doline_n7 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 7); }
static void NOINLINE pfield_doline_n8 (uae_u32 *data, int count) { pfield_doline_1 (data, count, 8); }
#endif

#endif /* USE_ARMNEON */

static __inline__ void pfield_doline (int lineno)
{
  int wordcount = dp_for_drawing->plflinelen;
  uae_u32 *data = pixdata.apixels_l + MAX_PIXELS_PER_LINE / 4;
  
#ifdef USE_ARMNEON
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
  int i, j;

  j = 0;  
  for (i = gfxvidinfo.height; i < MAX_VIDHEIGHT + 1; i++)
    row_map[i] = row_tmp;
  for (i = 0; i < gfxvidinfo.height; i++, j += gfxvidinfo.rowbytes)
		row_map[i] = gfxvidinfo.bufmem + j;
}

static void init_aspect_maps (void)
{
    int i, maxl;

    if (gfxvidinfo.height == 0)
	/* Do nothing if the gfx driver hasn't initialized the screen yet */
	return;

    if (native2amiga_line_map)
	free (native2amiga_line_map);
    if (amiga2aspect_line_map)
	free (amiga2aspect_line_map);

    /* At least for this array the +1 is necessary. */
    amiga2aspect_line_map = (int *)malloc (sizeof (int) * (MAXVPOS + 1) * 2 + 1);
    native2amiga_line_map = (int *)malloc (sizeof (int) * gfxvidinfo.height);

    maxl = (MAXVPOS + 1);
    min_ypos_for_screen = minfirstline;
    max_drawn_amiga_line = -1;
    for (i = 0; i < maxl; i++) {
	int v = (int) (i - min_ypos_for_screen);
	if (v >= gfxvidinfo.height && max_drawn_amiga_line == -1)
	    max_drawn_amiga_line = i - min_ypos_for_screen;
	if (i < min_ypos_for_screen || v >= gfxvidinfo.height)
	    v = -1;
	amiga2aspect_line_map[i] = v;
    }

    for (i = 0; i < gfxvidinfo.height; i++)
	native2amiga_line_map[i] = -1;

    for (i = maxl-1; i >= min_ypos_for_screen; i--) {
	int j;
	if (amiga2aspect_line_map[i] == -1)
	    continue;
	for (j = amiga2aspect_line_map[i]; j < gfxvidinfo.height && native2amiga_line_map[j] == -1; j++)
	    native2amiga_line_map[j] = i;
    }
}

/*
 * One drawing frame has been finished. Tell the graphics code about it.
 */
STATIC_INLINE void do_flush_screen ()
{
  unlockscr ();
	flush_screen (); /* vsync mode */
}

/* We only save hardware registers during the hardware frame. Now, when
 * drawing the frame, we expand the data into a slightly more useful
 * form. */
static void pfield_expand_dp_bplcon (void)
{
    bplres = dp_for_drawing->bplres;
    bplplanecnt = dp_for_drawing->nr_planes;
    bplham = dp_for_drawing->ham_seen;

    if (currprefs.chipset_mask & CSMASK_AGA) {
    	bplehb = (dp_for_drawing->bplcon0 & 0x7010) == 0x6000 && !(dp_for_drawing->bplcon2 & 0x200);
      bpldualpf2of = (dp_for_drawing->bplcon3 >> 10) & 7;
      sbasecol[0] = ((dp_for_drawing->bplcon4 >> 4) & 15) << 4;
      sbasecol[1] = ((dp_for_drawing->bplcon4 >> 0) & 15) << 4;
    } else if (currprefs.chipset_mask & CSMASK_ECS_DENISE)
    	bplehb = (dp_for_drawing->bplcon0 & 0xFC00) == 0x6000 && !(dp_for_drawing->bplcon2 & 0x200);
    else
    	bplehb = (dp_for_drawing->bplcon0 & 0xFC00) == 0x6000;

    plf1pri = dp_for_drawing->bplcon2 & 7;
    plf2pri = (dp_for_drawing->bplcon2 >> 3) & 7;
    plf_sprite_mask = 0xFFFF0000 << (4 * plf2pri);
    plf_sprite_mask |= (0xFFFF << (4 * plf1pri)) & 0xFFFF;
    bpldualpf = (dp_for_drawing->bplcon0 & 0x400) == 0x400;
    bpldualpfpri = (dp_for_drawing->bplcon2 & 0x40) == 0x40;
}

static void pfield_expand_dp_bplcon2(int regno, int v)
{
  regno -= 0x1000;
  switch (regno)
  {
    case 0x100:
      dp_for_drawing->bplcon0 = v;
      dp_for_drawing->bplres = GET_RES(v);
      dp_for_drawing->nr_planes = GET_PLANES(v);
	    dp_for_drawing->ham_seen = !! (v & 0x800);
      break;
    case 0x104:
      dp_for_drawing->bplcon2 = v;
      break;
  	case 0x106:
      dp_for_drawing->bplcon3 = v;
      break;
    case 0x108:
      dp_for_drawing->bplcon4 = v;
      break;
  }
  pfield_expand_dp_bplcon();
//  res_shift = lores_shift - bplres;
}

static int drawing_color_matches;
static enum { color_match_acolors, color_match_full } color_match_type;

/* Set up colors_for_drawing to the state at the beginning of the currently drawn
   line.  Try to avoid copying color tables around whenever possible.  */
static __inline__ void adjust_drawing_colors (int ctable, int need_full)
{
    if (drawing_color_matches != ctable) {
	if (need_full) {
			color_reg_cpy (&colors_for_drawing, curr_color_tables + ctable);
			color_match_type = color_match_full;
	} else {
			memcpy (colors_for_drawing.acolors, curr_color_tables[ctable].acolors,
			sizeof colors_for_drawing.acolors);
			color_match_type = color_match_acolors;
	}
		drawing_color_matches = ctable;
    } else if (need_full && color_match_type != color_match_full) {
		color_reg_cpy (&colors_for_drawing, &curr_color_tables[ctable]);
		color_match_type = color_match_full;
    }
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
			nextpos = coord_hw_to_window_x (curr_color_changes[i].linepos << 1);

		nextpos_in_range = nextpos;
	  if (nextpos > endpos)
	    nextpos_in_range = endpos;

	if (nextpos_in_range > lastpos) {
	    if (lastpos < playfield_start) {
			int t = nextpos_in_range <= playfield_start ? nextpos_in_range : playfield_start;
			(*worker_border) (lastpos, t);
			lastpos = t;
			}
		}
	if (nextpos_in_range > lastpos) {
	    if (lastpos >= playfield_start && lastpos < playfield_end) {
			int t = nextpos_in_range <= playfield_end ? nextpos_in_range : playfield_end;
			(*worker_pfield) (lastpos, t);
			lastpos = t;
			}
		}
	if (nextpos_in_range > lastpos) {
			if (lastpos >= playfield_end)
			(*worker_border) (lastpos, nextpos_in_range);
			lastpos = nextpos_in_range;
		}
	if (i != dip_for_drawing->last_color_change) {
	    if (regno >= 0x1000) {
		    pfield_expand_dp_bplcon2(regno, value);
	    } else {
			  color_reg_set (&colors_for_drawing, regno, value);
			  colors_for_drawing.acolors[regno] = getxcolor (value);
			}
		}
	if (lastpos >= endpos)
			break;
    }
}

static void pfield_draw_line (int lineno, int gfx_ypos)
{
   dp_for_drawing = line_decisions + lineno;
   dip_for_drawing = curr_drawinfo + lineno;
   
   xlinebuffer = row_map[gfx_ypos];
	 if(gfxvidinfo.width > 600)
	 		xlinebuffer -= (linetoscr_x_adjust_bytes << 1);
	 else
   		xlinebuffer -= linetoscr_x_adjust_bytes;

	if (dp_for_drawing->plfleft != -1) {
		pfield_expand_dp_bplcon ();
		pfield_init_linetoscr ();
    pfield_doline (lineno);

    adjust_drawing_colors (dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb);
   
    /* The problem is that we must call decode_ham() BEFORE we do the
      sprites. */
    if (dp_for_drawing->ham_seen) {
      init_ham_decoding ();
	    if (dip_for_drawing->nr_color_changes == 0) {
         /* The easy case: need to do HAM decoding only once for the
          * full line. */
         decode_ham (visible_left_border, visible_right_border);
	    } else /* Argh. */ {
         do_color_changes (dummy_worker, decode_ham);
         adjust_drawing_colors (dp_for_drawing->ctable, dp_for_drawing->ham_seen || bplehb);
      }
      bplham = dp_for_drawing->ham_at_start;
    }
      
		if (dip_for_drawing->nr_sprites) {
			int i;
			decide_draw_sprites();
			for (i = 0; i < dip_for_drawing->nr_sprites; i++) {
      	struct sprite_entry *e = curr_sprite_entries + dip_for_drawing->first_sprite_entry + i;
      	draw_sprites_punt[e->has_attached](e);
			}
		}
   
		do_color_changes (pfield_do_fill_line, pfield_do_linetoscr);
	} else {
		adjust_drawing_colors (dp_for_drawing->ctable, 0);
		if (dip_for_drawing->nr_color_changes == 0)	{
			fill_line ();
		}	else {
			playfield_start = visible_right_border;
			playfield_end = visible_right_border;
			do_color_changes(pfield_do_fill_line, pfield_do_fill_line);
		}
	}
}

void center_image (void)
{
	int deltaToBorder;
	if(gfxvidinfo.width > 600)
	  deltaToBorder = (gfxvidinfo.width >> 1) - 320;
	else
	  deltaToBorder = gfxvidinfo.width - 320;
	  
	visible_left_border = 73 - (deltaToBorder >> 1);
	visible_right_border = 393 + (deltaToBorder >> 1);

	linetoscr_x_adjust_bytes = visible_left_border * gfxvidinfo.pixbytes;

  thisframe_y_adjust = minfirstline;

  thisframe_y_adjust_real = thisframe_y_adjust + currprefs.pandora_vertical_offset + OFFSET_Y_ADJUST;
  max_ypos_thisframe = (maxvpos - thisframe_y_adjust);
}

static void init_drawing_frame (void)
{
    init_hardware_for_drawing_frame ();

    linestate_first_undecided = 0;

    center_image ();

    drawing_color_matches = -1;
}

/*
 * Some code to put status information on the screen.
 */
#define TD_PADX 10
#define TD_PADY 2
#define TD_WIDTH 32
#define TD_LED_WIDTH 24
#define TD_LED_HEIGHT 4

#define TD_RIGHT 1
#define TD_BOTTOM 2

static int td_pos = (TD_RIGHT|TD_BOTTOM);

#define TD_NUM_WIDTH 7
#define TD_NUM_HEIGHT 7

#define TD_TOTAL_HEIGHT (TD_PADY * 2 + TD_NUM_HEIGHT)

#define NUMBERS_NUM 16

static const char *numbers = { /* ugly  0123456789CHD%+- */
"+++++++--++++-+++++++++++++++++-++++++++++++++++++++++++++++++++++++++++++++-++++++-++++----++---+--------------"
"+xxxxx+--+xx+-+xxxxx++xxxxx++x+-+x++xxxxx++xxxxx++xxxxx++xxxxx++xxxxx++xxxx+-+x++x+-+xxx++-+xx+-+x---+----------"
"+x+++x+--++x+-+++++x++++++x++x+++x++x++++++x++++++++++x++x+++x++x+++x++x++++-+x++x+-+x++x+--+x++x+--+x+----+++--"
"+x+-+x+---+x+-+xxxxx++xxxxx++xxxxx++xxxxx++xxxxx+--++x+-+xxxxx++xxxxx++x+----+xxxx+-+x++x+----+x+--+xxx+--+xxx+-"
"+x+++x+---+x+-+x++++++++++x++++++x++++++x++x+++x+--+x+--+x+++x++++++x++x++++-+x++x+-+x++x+---+x+x+--+x+----+++--"
"+xxxxx+---+x+-+xxxxx++xxxxx+----+x++xxxxx++xxxxx+--+x+--+xxxxx++xxxxx++xxxx+-+x++x+-+xxx+---+x++xx--------------"
"+++++++---+++-++++++++++++++----+++++++++++++++++--+++--++++++++++++++++++++-++++++-++++------------------------"
};

static const char *letters = { /* ugly */
"------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ "
"-xxxxx -xxxxx -xxxxx -xxxx- -xxxxx -xxxxx -xxxxx -x---x --xx-- -----x -x--x- -x---- -x---x -x---x --xxx- -xxxx- -xxxx- -xxxx- -xxxxx -xxxxx -x---x -x---x -x---x -x---x -x---x -xxxxx "
"-x---x -x---x -x---- -x---x -x---- -x---- -x---- -x---x --xx-- -----x -x-x-- -x---- -xxxxx -xx--x -x---x -x---x -x---- -x---x -x---- ---x-- -x---x -x---x --x-x- --x-x- -x---x ----x- "
"-xxxxx -xxxxx -x---- -x---x -xxxxx -xxxx- -xxxxx -xxxxx --xx-- -----x -xx--- -x---- -x---x -x-x-x -x---x -xxxx- -x---- -xxxx- -xxxxx ---x-- -x---x -x---x ---x-- ---x-- -x-x-x ---x-- "
"-x---x -x---x -x---- -x---x -x---- -x---- -x---x -x---x --xx-- -x---x -x-x-- -x---- -x---x -x--xx -x---x -x---- -x---- -x-x-- -----x ---x-- -x---x -xx-xx --x-x- ---x-- -xx-xx --x--- "
"-x---x -xxxxx -xxxxx -xxxx- -xxxxx -x---- -xxxxx -x---x --xx-- -xxxxx -x--x- -xxxx- -x---x -x---x --xxx- -x---- -xxxx- -x--xx -xxxxx ---x-- --xxx- ---x-- -x---x ---x-- -x---x -xxxxx "
"------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ ------ "
};


STATIC_INLINE void putpixel (int x, xcolnr c8)
{
	uae_u16 *p = (uae_u16 *)xlinebuffer + x;
	*p = (uae_u16)c8;
}

static void write_tdnumber (int x, int y, int num)
{
    int j;
    const char *numptr;

    numptr = numbers + num * TD_NUM_WIDTH + NUMBERS_NUM * TD_NUM_WIDTH * y;
    for (j = 0; j < TD_NUM_WIDTH; j++) {
    	if (*numptr == 'x')
	      putpixel (x + j, xcolors[0xfff]);
	    else if (*numptr == '+')
	      putpixel (x + j, xcolors[0x000]);
		  numptr++;
    }
}

static void write_tdletter (int x, int y, char ch)
{
    int j;
    uae_u8 *numptr;
	
    numptr = (uae_u8 *)(letters + (ch-65) * TD_NUM_WIDTH + 26 * TD_NUM_WIDTH * y);
  
	for (j = 0; j < TD_NUM_WIDTH; j++) {
	putpixel (x + j, *numptr == 'x' ? xcolors[0xfff] : xcolors[0x000]);
	numptr++;
    }
}

static void draw_status_line (int line)
{
  int x, y, i, j, led, on;
  int on_rgb, off_rgb, c;
  uae_u8 *buf;

#ifdef PICASSO96
  if(picasso_on)
  {
#ifdef RASPBERRY
    // It should be thoses lines for everyones... 
    x = picasso_vidinfo.width - TD_PADX - 6 * TD_WIDTH;
    y = line - (picasso_vidinfo.height - TD_TOTAL_HEIGHT);
    xlinebuffer = (uae_u8*)prSDLScreen->pixels + picasso_vidinfo.rowbytes * line;
#else
    x = prSDLScreen->w - TD_PADX - 6 * TD_WIDTH;
    y = line - (prSDLScreen->h - TD_TOTAL_HEIGHT);
    xlinebuffer = (uae_u8*)prSDLScreen->pixels + prSDLScreen->pitch * line;
#endif
  }
  else
#endif
  {
    x = gfxvidinfo.width - TD_PADX - 6 * TD_WIDTH;
    y = line - (gfxvidinfo.height - TD_TOTAL_HEIGHT);
    xlinebuffer = row_map[line];
  }

	x+=100 - (TD_WIDTH*(currprefs.nr_floppies-1)) - TD_WIDTH;
#ifdef PICASSO96
  if(picasso_on)
#ifdef RASPBERRY
    memset (xlinebuffer + (x - 4) * 2, 0, (picasso_vidinfo.width - x + 4) * 2);
#else
    memset (xlinebuffer + (x - 4) * 2, 0, (prSDLScreen->w - x + 4) * 2);
#endif
  else
#endif
    memset (xlinebuffer + (x - 4) * gfxvidinfo.pixbytes, 0, (gfxvidinfo.width - x + 4) * gfxvidinfo.pixbytes);

	for (led = -2; led < (currprefs.nr_floppies+1); led++) {
		int track;
		if (led > 0) {
			/* Floppy */
			track = gui_data.drive_track[led-1];
			on = gui_data.drive_motor[led-1];
			on_rgb = 0x0c0;
			off_rgb = 0x030;
      if (gui_data.drive_writing[led-1])
		    on_rgb = 0xc00;
		} else if (led < -1) {
			/* Idle time */
			track = idletime_percent;
			on = 1;
			on_rgb = 0x666;
			off_rgb = 0x666;
		} else if (led < 0) {
			/* Power */
			track = gui_data.fps;
			on = gui_data.powerled;
			on_rgb = 0xc00;
			off_rgb = 0x300;
		} else {
			/* Hard disk */
			track = -2;
			
			switch (gui_data.hd) {
				case HDLED_OFF:
					on = 0;
					off_rgb = 0x003;
					break;
				case HDLED_READ:
					on = 1;
					on_rgb = 0x00c;
					off_rgb = 0x003;
					break;
				case HDLED_WRITE:
					on = 1;
					on_rgb = 0xc00;
					off_rgb = 0x300;
					break;
			}
		}
	c = xcolors[on ? on_rgb : off_rgb];

	for (j = 0; j < TD_LED_WIDTH; j++) 
	    putpixel (x + j, c);

	if (y >= TD_PADY && y - TD_PADY < TD_NUM_HEIGHT) {
	    if (track >= 0) {
        int tn = track >= 100 ? 3 : 2;
	      int offs = (TD_LED_WIDTH - tn * TD_NUM_WIDTH) / 2;
        if(track >= 100)
        {
        	write_tdnumber (x + offs, y - TD_PADY, track / 100);
        	offs += TD_NUM_WIDTH;
        }
	      write_tdnumber (x + offs, y - TD_PADY, (track / 10) % 10);
	      write_tdnumber (x + offs + TD_NUM_WIDTH, y - TD_PADY, track % 10);
	    }
		  else if (nr_units() > 0) {
    		int offs = (TD_LED_WIDTH - 2 * TD_NUM_WIDTH) / 2;
			  write_tdletter(x + offs, y - TD_PADY, 'H');
			  write_tdletter(x + offs + TD_NUM_WIDTH, y - TD_PADY, 'D');
	    }
	}
	x += TD_WIDTH;
    }

}


static void finish_drawing_frame (void)
{
	int i;

	lockscr();

	if(gfxvidinfo.width > 600)
	{
    if(currprefs.chipset_mask & CSMASK_AGA)
  		pfield_do_linetoscr=(line_draw_func)pfield_do_linetoscr_0_640_AGA;
    else
  		pfield_do_linetoscr=(line_draw_func)pfield_do_linetoscr_0_640;
		pfield_do_fill_line=(line_draw_func)pfield_do_fill_line_0_640;
	}
	else
	{
    if(currprefs.chipset_mask & CSMASK_AGA)
  		pfield_do_linetoscr=(line_draw_func)pfield_do_linetoscr_0_AGA;
    else
  		pfield_do_linetoscr=(line_draw_func)pfield_do_linetoscr_0;
		pfield_do_fill_line=(line_draw_func)pfield_do_fill_line_0;
	}

	for (i = 0; i < max_ypos_thisframe; i++) {
		int where,i1;
		int line = i + thisframe_y_adjust_real;

    if(line >= linestate_first_undecided)
			break;

		i1 = i + min_ypos_for_screen;
		where = amiga2aspect_line_map[i1];
		if (where >= gfxvidinfo.height)
			break;
		if (where < 0)
			continue;

		pfield_draw_line (line, where);
	}

	if (currprefs.leds_on_screen) {
		for (i = 0; i < TD_TOTAL_HEIGHT; i++) {
			int line = gfxvidinfo.height - TD_TOTAL_HEIGHT + i;
			draw_status_line (line);
		}
	}
	do_flush_screen ();

#ifdef DEBUG_M68K
extern int debug_frame_counter;
extern int debug_frame_start;
extern int debug_frame_end;
  ++debug_frame_counter;
  if(debug_frame_counter >= debug_frame_start && debug_frame_counter < debug_frame_end)
  {
    write_log("FRAME_SYNC ------------------------ FRAME_SYNC\n");
    write_log("FRAME_SYNC --- start frame %4d --- FRAME_SYNC\n", debug_frame_counter);
    write_log("FRAME_SYNC ------------------------ FRAME_SYNC\n");
  }
#endif  
}

STATIC_INLINE void check_picasso (void)
{
#ifdef PICASSO96
    if (picasso_on && picasso_redraw_necessary)
	picasso_refresh (1);
    picasso_redraw_necessary = 0;

    if (picasso_requested_on == picasso_on)
	return;

    picasso_on = picasso_requested_on;

    if (!picasso_on)
    	clear_inhibit_frame (IHF_PICASSO);
    else
	    set_inhibit_frame (IHF_PICASSO);

    gfx_set_picasso_state (picasso_on);
    picasso_enablescreen (picasso_requested_on);

    notice_screen_contents_lost ();
    notice_new_xcolors ();
    count_frame ();
#endif
}

#ifdef RASPBERRY
int wait_for_vsync = 1;
extern uae_sem_t vsync_wait_sem;
#endif

void vsync_handle_redraw (int long_frame, int lof_changed)
{

	count_frame ();

		if (framecnt == 0)
		{
			#ifdef RASPBERRY
			if (wait_for_vsync == 1)
				uae_sem_wait (&vsync_wait_sem);
			wait_for_vsync = 1;
			#endif
			finish_drawing_frame ();
		}
#ifdef PICASSO96
    else if(picasso_on && currprefs.leds_on_screen)
    {
      int i;
  		for (i = 0; i < TD_TOTAL_HEIGHT; i++) {
#ifdef RASPBERRY
  			int line = picasso_vidinfo.height - TD_TOTAL_HEIGHT + i;
#else
  			int line = prSDLScreen->h - TD_TOTAL_HEIGHT + i;
#endif
  			draw_status_line (line);
  		}
    }
#endif
		/* At this point, we have finished both the hardware and the
		 * drawing frame. Essentially, we are outside of all loops and
		 * can do some things which would cause confusion if they were
		 * done at other times.
		 */
#ifdef SAVESTATE
	  if (savestate_state == STATE_DORESTORE) {
			savestate_state = STATE_RESTORE;
	    reset_drawing ();
			uae_reset (0);
	  }
#endif

		if (quit_program < 0) {
			quit_program = -quit_program;
	    set_inhibit_frame (IHF_QUIT_PROGRAM);
			set_special (&regs, SPCFLAG_BRK);
			return;
		}

	  check_picasso ();

	  if (check_prefs_changed_gfx ()) {
	    reset_drawing ();
	    notice_new_xcolors ();
	  }

	  check_prefs_changed_audio ();
	  check_prefs_changed_custom ();
	  check_prefs_changed_cpu ();

		framecnt = fs_framecnt;
      
		if (framecnt == 0)
			init_drawing_frame ();

  gui_hd_led (0);
  gui_cd_led (0);
}

void hsync_record_line_state (int lineno)
{
  if (framecnt != 0)
  	return;

  linestate_first_undecided = lineno + 1;
}

static void dummy_flush_line (struct vidbuf_description *gfxinfo, int line_no)
{
}

static void dummy_flush_block (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
}

static void dummy_flush_screen (struct vidbuf_description *gfxinfo, int first_line, int last_line)
{
}

static void dummy_flush_clear_screen (struct vidbuf_description *gfxinfo)
{
}

static int  dummy_lock (struct vidbuf_description *gfxinfo)
{
    return 1;
}

static void dummy_unlock (struct vidbuf_description *gfxinfo)
{
}

static void gfxbuffer_reset (void)
{
    gfxvidinfo.flush_line         = dummy_flush_line;
    gfxvidinfo.flush_block        = dummy_flush_block;
    gfxvidinfo.flush_screen       = dummy_flush_screen;
    gfxvidinfo.flush_clear_screen = dummy_flush_clear_screen;
    gfxvidinfo.lockscr            = dummy_lock;
    gfxvidinfo.unlockscr          = dummy_unlock;
}

void reset_drawing (void)
{
    lores_reset ();

    linestate_first_undecided = 0;
    
    init_aspect_maps ();

    init_row_map();

    memset(spixels, 0, sizeof spixels);
    memset(&spixstate, 0, sizeof spixstate);

    init_drawing_frame ();
    notice_screen_contents_lost ();
}

void drawing_init (void)
{
    gen_pfield_tables();

#ifdef PICASSO96
    if (savestate_state != STATE_RESTORE) {
      InitPicasso96 ();
      picasso_on = 0;
      picasso_requested_on = 0;
      picasso_redraw_necessary = 0;
      gfx_set_picasso_state (0);
    }
#endif
    xlinebuffer = gfxvidinfo.bufmem;
    inhibit_frame = 0;

    gfxbuffer_reset ();
    reset_drawing ();
}
