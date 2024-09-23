/*
* Data used for communication between custom.c and drawing.c.
*
* Copyright 1996-1998 Bernd Schmidt
*/

#ifndef UAE_DRAWING_H
#define UAE_DRAWING_H

#include "uae/types.h"

#define SMART_UPDATE 1

#ifdef SUPPORT_PENGUINS
#undef SMART_UPDATE
#define SMART_UPDATE 1
#endif

#ifdef AGA
#define MAX_PLANES 8
#else
#define MAX_PLANES 6
#endif

extern int lores_shift, shres_shift, interlace_seen;
extern int visible_left_border, visible_right_border;
extern int detected_screen_resolution;
extern int hsync_end_left_border, denisehtotal;

#define AMIGA_WIDTH_MAX (754 / 2)
#define AMIGA_HEIGHT_MAX_PAL (576 / 2)
#define AMIGA_HEIGHT_MAX_NTSC (486 / 2)
#define AMIGA_HEIGHT_MAX (AMIGA_HEIGHT_MAX_PAL)

// Cycles * 2 from start of scanline to first refresh slot (hsync strobe slot)
#define DDF_OFFSET (2 * 4)

#define RECORDED_REGISTER_CHANGE_OFFSET 0x1000

/* According to the HRM, pixel data spends a couple of cycles somewhere in the chips
before it appears on-screen. (TW: display emulation now does this automatically)  */
#define DIW_DDF_OFFSET 1
#define DIW_DDF_OFFSET_SHRES (DIW_DDF_OFFSET << 2)
/* We ignore that many lores pixels at the start of the display. These are
* invisible anyway due to hardware DDF limits. */
#define DISPLAY_LEFT_SHIFT 0
#define DISPLAY_LEFT_SHIFT_SHRES (DISPLAY_LEFT_SHIFT << 2)

#define CCK_SHRES_SHIFT 3

STATIC_INLINE int shres_coord_hw_to_window_x(int x)
{
	x -= DISPLAY_LEFT_SHIFT_SHRES;
	x <<= lores_shift;
	x >>= 2;
	return x;
}

STATIC_INLINE int coord_hw_to_window_x_shres(int xx)
{
	int x = xx >> (CCK_SHRES_SHIFT - 1);
	x -= DISPLAY_LEFT_SHIFT;
	if (lores_shift == 1) {
		x <<= 1;
		x |= (xx >> 1) & 1;
	} else if (lores_shift == 2) {
		x <<= 2;
		x |= xx & 3;
	}
	return x;
}
STATIC_INLINE int coord_hw_to_window_x_lores(int x)
{
	x -= DISPLAY_LEFT_SHIFT;
	return x << lores_shift;
}

STATIC_INLINE int PIXEL_XPOS(int xx)
{
	int x = xx >> (CCK_SHRES_SHIFT - 1);
	x -= DISPLAY_LEFT_SHIFT;
	x += DIW_DDF_OFFSET;
	x -= 1;
	if (lores_shift == 1) {
		x <<= 1;
		x |= (xx >> 1) & 1;
	} else if (lores_shift == 2) {
		x <<= 2;
		x |= xx & 3;
	}
	return x;
}

#define min_diwlastword (PIXEL_XPOS(hsyncstartpos_start_cycles << CCK_SHRES_SHIFT))
#define max_diwlastword (PIXEL_XPOS(denisehtotal))

STATIC_INLINE int coord_window_to_hw_x(int x)
{
	x >>= lores_shift;
	return x + DISPLAY_LEFT_SHIFT;
}

STATIC_INLINE int coord_diw_lores_to_window_x(int x)
{
	return (x - DISPLAY_LEFT_SHIFT + DIW_DDF_OFFSET - 1) << lores_shift;
}

STATIC_INLINE int coord_diw_shres_to_window_x(int x)
{
	return (x - DISPLAY_LEFT_SHIFT_SHRES + DIW_DDF_OFFSET_SHRES - (1 << 2)) >> shres_shift;
}

STATIC_INLINE int coord_window_to_diw_x(int x)
{
	x = coord_window_to_hw_x (x);
	return x - DIW_DDF_OFFSET;
}

/* color values in two formats: 12 (OCS/ECS) or 24 (AGA) bit Amiga RGB (color_regs),
* and the native color value; both for each Amiga hardware color register.
*
* !!! See color_reg_xxx functions below before touching !!!
*/
#define CE_BORDERBLANK 0
#define CE_BORDERNTRANS 1
#define CE_BORDERSPRITE 2
#define CE_EXTBLANKSET 3
#define CE_SHRES_DELAY_SHIFT 8

STATIC_INLINE bool ce_is_borderblank(uae_u16 data)
{
	return (data & (1 << CE_BORDERBLANK)) != 0;
}
STATIC_INLINE bool ce_is_extblankset(uae_u16 data)
{
	return (data & (1 << CE_EXTBLANKSET)) != 0;
}
STATIC_INLINE bool ce_is_bordersprite(uae_u16 data)
{
	return (data & (1 << CE_BORDERSPRITE)) != 0;
}
STATIC_INLINE bool ce_is_borderntrans(uae_u16 data)
{
	return (data & (1 << CE_BORDERNTRANS)) != 0;
}

#define VB_VB 0x20 // vblank
#define VB_VS 0x10 // vsync
#define VB_XBORDER 0x08 // forced border color or bblank
#define VB_XBLANK 0x04 // forced bblank
#define VB_PRGVB 0x02 // programmed vblank
#define VB_NOVB 0x01 // normal

struct color_entry {
	uae_u16 color_regs_ecs[32];
#ifndef AGA
	xcolnr acolors[32];
#else
	xcolnr acolors[256];
	uae_u32 color_regs_aga[256];
#endif
	uae_u16 extra;
};

#ifdef AGA
/* convert 24 bit AGA Amiga RGB to native color */
/* warning: this is still ugly, but now works with either byte order */
#ifdef WORDS_BIGENDIAN
# define CONVERT_RGB(c) \
	( xbluecolors[((uae_u8*)(&c))[3]] | xgreencolors[((uae_u8*)(&c))[2]] | xredcolors[((uae_u8*)(&c))[1]] )
#else
# define CONVERT_RGB(c) \
	( xbluecolors[((uae_u8*)(&c))[0]] | xgreencolors[((uae_u8*)(&c))[1]] | xredcolors[((uae_u8*)(&c))[2]] )
#endif
#else
#define CONVERT_RGB(c) 0
#endif

STATIC_INLINE xcolnr getxcolor(int c)
{
#ifdef AGA
	if (direct_rgb)
		return CONVERT_RGB(c);
	else
#endif
		return xcolors[c & 0xfff];
}

/* functions for reading, writing, copying and comparing struct color_entry */
STATIC_INLINE int color_reg_get (struct color_entry *ce, int c)
{
#ifdef AGA
	if (aga_mode)
		return ce->color_regs_aga[c];
	else
#endif
		return ce->color_regs_ecs[c];
}
STATIC_INLINE void color_reg_set (struct color_entry *ce, int c, int v)
{
#ifdef AGA
	if (aga_mode)
		ce->color_regs_aga[c] = v;
	else
#endif
		ce->color_regs_ecs[c] = v;
}
STATIC_INLINE int color_reg_cmp (struct color_entry *ce1, struct color_entry *ce2)
{
	int v;
#ifdef AGA
	if (aga_mode)
		v = memcmp (ce1->color_regs_aga, ce2->color_regs_aga, sizeof (uae_u32) * 256);
	else
#endif
		v = memcmp (ce1->color_regs_ecs, ce2->color_regs_ecs, sizeof (uae_u16) * 32);
	if (!v && ce1->extra == ce2->extra)
		return 0;
	return 1;
}
/* ugly copy hack, is there better solution? */
STATIC_INLINE void color_reg_cpy (struct color_entry *dst, struct color_entry *src)
{
	dst->extra = src->extra;
#ifdef AGA
	if (aga_mode)
		/* copy acolors and color_regs_aga */
		memcpy (dst->acolors, src->acolors, sizeof(struct color_entry) - sizeof(uae_u16) * 32);
	else
#endif
		/* copy first 32 acolors and color_regs_ecs */
		memcpy (dst->color_regs_ecs, src->color_regs_ecs, sizeof(struct color_entry));
}

/*
* The idea behind this code is that at some point during each horizontal
* line, we decide how to draw this line. There are many more-or-less
* independent decisions, each of which can be taken at a different horizontal
* position.
* Sprites and color changes are handled specially: There isn't a single decision,
* but a list of structures containing information on how to draw the line.
*/

#define COLOR_CHANGE_BRDBLANK 0x80000000
#define COLOR_CHANGE_SHRES_DELAY 0x40000000
#define COLOR_CHANGE_BLANK 0x20000000
#define COLOR_CHANGE_ACTBORDER (COLOR_CHANGE_BLANK | COLOR_CHANGE_BRDBLANK)
#define COLOR_CHANGE_MASK 0xf0000000
#define COLOR_CHANGE_GENLOCK 0x01000000
struct color_change {
	int linepos;
	int regno;
	uae_u32 value;
};

/* 440 rather than 880, since sprites are always lores.  */
#ifdef UAE_MINI
#define MAX_PIXELS_PER_LINE 880
#else
#define MAX_PIXELS_PER_LINE 2304
#endif

#define MAXVPOS_WRAPLINES 10

/* No divisors for MAX_PIXELS_PER_LINE; we support AGA and SHRES sprites */
#define MAX_SPR_PIXELS ((((MAXVPOS + MAXVPOS_WRAPLINES) * 2 + 1) * MAX_PIXELS_PER_LINE) / 4)

struct sprite_entry
{
	uae_u16 pos;
	uae_u16 max;
	uae_u32 first_pixel;
	bool has_attached;
};

struct sprite_stb
{
	/* Eight bits for every pixel for attachment
	 * Another eight for 64/32 status
	 */
	uae_u8 stb[2 * MAX_SPR_PIXELS];
	uae_u16 stbfm[2 * MAX_SPR_PIXELS];
};
extern struct sprite_stb spixstate;

#ifdef OS_WITHOUT_MEMORY_MANAGEMENT
extern uae_u16 *spixels;
#else
extern uae_u16 spixels[MAX_SPR_PIXELS * 2];
#endif

/* Way too much... */
#define MAX_REG_CHANGE ((MAXVPOS + MAXVPOS_WRAPLINES) * 2 * MAXHPOS / 2)

extern struct color_entry *curr_color_tables, *prev_color_tables;

extern struct sprite_entry *curr_sprite_entries, *prev_sprite_entries;
extern struct color_change *curr_color_changes, *prev_color_changes;
extern struct draw_info *curr_drawinfo, *prev_drawinfo;

/* struct decision contains things we save across drawing frames for
* comparison (smart update stuff). */
struct decision {
	/* Records the leftmost access of BPL1DAT.  */
	int plfleft, plfright, plflinelen;
	/* Display window: native coordinates, depend on lores state.  */
	int diwfirstword, diwlastword;
	int ctable;

	uae_u16 bplcon0, bplcon2;
#ifdef AGA
	uae_u16 bplcon3, bplcon4bm, bplcon4sp;
	uae_u16 fmode;
#endif
	uae_u8 nr_planes, max_planes;
	uae_u8 bplres;
	bool ehb_seen;
	bool ham_seen;
	bool ham_at_start;
#ifdef AGA
	bool bordersprite_seen;
	bool xor_seen;
	uae_u8 vb;
#endif
};

/* Anything related to changes in hw registers during the DDF for one
* line. */
struct draw_info {
	int first_sprite_entry, last_sprite_entry;
	int first_color_change, last_color_change;
	int nr_color_changes, nr_sprites;
};

extern struct decision line_decisions[2 * (MAXVPOS + MAXVPOS_WRAPLINES) + 1];

extern uae_u8 line_data[(MAXVPOS + MAXVPOS_WRAPLINES) * 2][MAX_PLANES * MAX_WORDS_PER_LINE * 2];

/* Functions in drawing.c.  */
extern int coord_native_to_amiga_y (int);
extern int coord_native_to_amiga_x (int);

extern void record_diw_line (int plfstrt, int first, int last);

/* Determine how to draw a scan line.  */
enum nln_how {
	/* All lines on a non-doubled display. */
	nln_normal,
	/* Non-interlace, doubled display.  */
	nln_doubled,
	/* Interlace, doubled display, upper line.  */
	nln_upper,
	/* Interlace, doubled display, lower line.  */
	nln_lower,
	/* This line normal, next one black.  */
	nln_nblack,
	nln_upper_black,
	nln_lower_black,
	nln_upper_black_always,
	nln_lower_black_always
};

extern void hsync_record_line_state(int lineno, enum nln_how, int changed);
extern void hsync_record_line_state_last(int lineno, enum nln_how, int changed);
extern void vsync_handle_redraw (int long_field, int lof_changed, uae_u16, uae_u16, bool drawlines, bool initial);
extern bool vsync_handle_check (void);
extern void draw_lines(int end, int section);
extern void init_hardware_for_drawing_frame (void);
extern void reset_drawing (void);
extern void drawing_init (void);
extern bool notice_interlace_seen(int, bool);
extern void notice_resolution_seen(int, bool);
extern bool frame_drawn (int monid);
extern void redraw_frame(void);
extern void full_redraw_all(void);
extern bool draw_frame (struct vidbuffer*);
extern int get_custom_limits (int *pw, int *ph, int *pdx, int *pdy, int *prealh);
extern void store_custom_limits (int w, int h, int dx, int dy);
extern void set_custom_limits (int w, int h, int dx, int dy, bool blank);
extern void check_custom_limits (void);
extern void get_custom_topedge (int *x, int *y, bool max);
extern void get_custom_raw_limits (int *pw, int *ph, int *pdx, int *pdy);
void get_custom_mouse_limits (int *pw, int *ph, int *pdx, int *pdy, int dbl);
extern void putpixel (uae_u8 *buf, uae_u8 *genlockbuf, int bpp, int x, xcolnr c8);
extern void allocvidbuffer(int monid, struct vidbuffer *buf, int width, int height, int depth);
extern void freevidbuffer(int monid, struct vidbuffer *buf);
extern void check_prefs_picasso(void);
extern int get_vertical_visible_height(bool);
extern void get_screen_blanking_limits(int*, int*, int*, int*);

/* Finally, stuff that shouldn't really be shared.  */

extern int thisframe_first_drawn_line, thisframe_last_drawn_line;

#define IHF_SCROLLLOCK 0
#define IHF_QUIT_PROGRAM 1
#define IHF_PICASSO 2

void set_inhibit_frame(int monid, int bit);
void clear_inhibit_frame(int monid, int bit);
void toggle_inhibit_frame(int monid, int bit);

#endif /* UAE_DRAWING_H */
