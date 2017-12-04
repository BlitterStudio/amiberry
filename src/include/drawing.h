/*
 * Data used for communication between custom.c and drawing.c.
 * 
 * Copyright 1996-1998 Bernd Schmidt
 */

#pragma once
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

#ifdef AMIBERRY
#define AMIGA_WIDTH_MAX (640 / 2)
#define AMIGA_HEIGHT_MAX (512 / 2)
#else
#define AMIGA_WIDTH_MAX (752 / 2)
#define AMIGA_HEIGHT_MAX (576 / 2)
#endif

 //#define NEWHSYNC

#ifdef NEWHSYNC
#define DIW_DDF_OFFSET 9
 /* this many cycles starting from hpos=0 are visible on right border */
#define HBLANK_OFFSET 13
#define DISPLAY_LEFT_SHIFT 0x40
#else
 /* According to the HRM, pixel data spends a couple of cycles somewhere in the chips
 before it appears on-screen. (TW: display emulation now does this automatically)  */
#define DIW_DDF_OFFSET 1
 /* this many cycles starting from hpos=0 are visible on right border */
#define HBLANK_OFFSET 9
 /* We ignore that many lores pixels at the start of the display. These are
 * invisible anyway due to hardware DDF limits. */
#define DISPLAY_LEFT_SHIFT 0x38
#endif

#define PIXEL_XPOS(HPOS) (((HPOS)*2 - DISPLAY_LEFT_SHIFT + DIW_DDF_OFFSET - 1) << lores_shift)

#define min_diwlastword (0)
#define max_diwlastword (PIXEL_XPOS(0x1d4 >> 1))

extern int lores_shift, interlace_seen;
extern bool aga_mode, direct_rgb;
extern int visible_left_border, visible_right_border;
extern int detected_screen_resolution;

STATIC_INLINE int coord_hw_to_window_x(int x)
{
	x -= DISPLAY_LEFT_SHIFT;
	return x << lores_shift;
}

STATIC_INLINE int coord_window_to_hw_x(int x)
{
	x >>= lores_shift;
	return x + DISPLAY_LEFT_SHIFT;
}

STATIC_INLINE int coord_diw_to_window_x(int x)
{
	return (x - DISPLAY_LEFT_SHIFT + DIW_DDF_OFFSET - 1) << lores_shift;
}

STATIC_INLINE int coord_window_to_diw_x(int x)
{
	x = coord_window_to_hw_x(x);
	return x - DIW_DDF_OFFSET;
}

extern int framecnt;
extern int custom_frame_redraw_necessary;

/* color values in two formats: 12 (OCS/ECS) or 24 (AGA) bit Amiga RGB (color_regs),
 * and the native color value; both for each Amiga hardware color register. 
 *
 * !!! See color_reg_xxx functions below before touching !!!
 */
#define CE_BORDERBLANK 0
#define CE_BORDERNTRANS 1
#define CE_BORDERSPRITE 2
#define CE_SHRES_DELAY 4

STATIC_INLINE bool ce_is_borderblank(uae_u8 data)
{
	return (data & (1 << CE_BORDERBLANK)) != 0;
}
STATIC_INLINE bool ce_is_bordersprite(uae_u8 data)
{
	return (data & (1 << CE_BORDERSPRITE)) != 0;
}
STATIC_INLINE bool ce_is_borderntrans(uae_u8 data)
{
	return (data & (1 << CE_BORDERNTRANS)) != 0;
}

struct color_entry {
	uae_u16 color_regs_ecs[32];
#ifndef AGA
	xcolnr acolors[32];
#else
	xcolnr acolors[256];
	uae_u32 color_regs_aga[256];
#endif
	uae_u8 extra;
};

#ifdef AGA
/* convert 24 bit AGA Amiga RGB to native color */
/* warning: this is still ugly, but now works with either byte order */
# define CONVERT_RGB(c) \
	( xbluecolors[((uae_u8*)(&c))[0]] | xgreencolors[((uae_u8*)(&c))[1]] | xredcolors[((uae_u8*)(&c))[2]] )
#endif

STATIC_INLINE xcolnr getxcolor(int c)
{
#ifdef AGA
	if (direct_rgb)
		return CONVERT_RGB(c);
	else
#endif
		return xcolors[c];
}

/* functions for reading, writing, copying and comparing struct color_entry */
STATIC_INLINE int color_reg_get(struct color_entry *ce, int c)
{
#ifdef AGA
	if (aga_mode)
		return ce->color_regs_aga[c];
	else
#endif
		return ce->color_regs_ecs[c];
}
STATIC_INLINE void color_reg_set(struct color_entry *ce, int c, int v)
{
#ifdef AGA
	if (aga_mode)
		ce->color_regs_aga[c] = v;
	else
#endif
		ce->color_regs_ecs[c] = v;
}
STATIC_INLINE int color_reg_cmp(struct color_entry *ce1, struct color_entry *ce2)
{
	int v;
#ifdef AGA
	if (aga_mode)
		v = memcmp(ce1->color_regs_aga, ce2->color_regs_aga, sizeof(uae_u32) * 256);
	else
#endif
		v = memcmp(ce1->color_regs_ecs, ce2->color_regs_ecs, sizeof(uae_u16) * 32);
	if (!v && ce1->extra == ce2->extra)
		return 0;
	return 1;
}
/* ugly copy hack, is there better solution? */
STATIC_INLINE void color_reg_cpy(struct color_entry *dst, struct color_entry *src)
{
	dst->extra = src->extra;
#ifdef AGA
	if (aga_mode)
		/* copy acolors and color_regs_aga */
		memcpy(dst->acolors, src->acolors, sizeof(struct color_entry) - sizeof(uae_u16) * 32);
	else
#endif
		/* copy first 32 acolors and color_regs_ecs */
		memcpy(dst->color_regs_ecs, src->color_regs_ecs, sizeof(struct color_entry));
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
#define COLOR_CHANGE_HSYNC_HACK 0x20000000
#define COLOR_CHANGE_MASK 0xf0000000
struct color_change {
	int linepos;
	int regno;
	unsigned int value;
};

/* 440 rather than 880, since sprites are always lores.  */
#ifdef UAE_MINI
#define MAX_PIXELS_PER_LINE 880
#else
#define MAX_PIXELS_PER_LINE 1760
#endif

/* No divisors for MAX_PIXELS_PER_LINE; we support AGA and SHRES sprites */
#define MAX_SPR_PIXELS (((MAXVPOS + 1) * 2 + 1) * MAX_PIXELS_PER_LINE)

struct sprite_entry
{
	unsigned short pos;
	unsigned short max;
	unsigned int first_pixel;
	bool has_attached;
};

union sps_union {
	uae_u8 bytes[2 * MAX_SPR_PIXELS];
	uae_u32 words[2 * MAX_SPR_PIXELS / 4];
};
extern union sps_union spixstate;
extern uae_u16 spixels[MAX_SPR_PIXELS * 2];

/* Way too much... */
#define MAX_REG_CHANGE ((MAXVPOS + 1) * 2 * MAXHPOS)

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
	uae_u16 bplcon3, bplcon4;
#endif
	uae_u8 nr_planes;
	uae_u8 bplres;
	bool ehb_seen;
	bool ham_seen;
	bool ham_at_start;
	bool bordersprite_seen;
};

/* Anything related to changes in hw registers during the DDF for one
 * line. */
struct draw_info {
	int first_sprite_entry, last_sprite_entry;
	int first_color_change, last_color_change;
	int nr_color_changes, nr_sprites;
};

extern struct decision line_decisions[2 * (MAXVPOS + 2) + 1];

extern uae_u8 line_data[(MAXVPOS + 2) * 2][MAX_PLANES * MAX_WORDS_PER_LINE * 2];

/* Functions in drawing.c.  */
extern int coord_native_to_amiga_y(int);
extern int coord_native_to_amiga_x(int);

extern void record_diw_line(int plfstrt, int first, int last);
extern void hardware_line_completed(int lineno);

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
extern void vsync_handle_redraw(int long_field, int lof_changed, uae_u16, uae_u16);
extern bool vsync_handle_check();
extern void init_hardware_for_drawing_frame();
extern void reset_drawing();
extern void drawing_init();
extern bool notice_interlace_seen(bool);
extern void notice_resolution_seen(int, bool);
extern void frame_drawn(void);
extern void redraw_frame(void);
extern bool draw_frame();
extern int get_custom_limits(int *pw, int *ph, int *pdx, int *pdy, int *prealh);
extern void store_custom_limits(int w, int h, int dx, int dy);
extern void set_custom_limits(int w, int h, int dx, int dy);
extern void get_custom_topedge(int *x, int *y, bool max);
extern void get_custom_raw_limits(int *pw, int *ph, int *pdx, int *pdy);

extern long time_per_frame;

/* Finally, stuff that shouldn't really be shared.  */

extern int thisframe_first_drawn_line, thisframe_last_drawn_line;

#define IHF_SCROLLLOCK 0
#define IHF_QUIT_PROGRAM 1
#define IHF_PICASSO 2

extern int inhibit_frame;

STATIC_INLINE void set_inhibit_frame(int bit)
{
	inhibit_frame |= 1 << bit;
}

STATIC_INLINE void clear_inhibit_frame(int bit)
{
	inhibit_frame &= ~(1 << bit);
}

STATIC_INLINE void toggle_inhibit_frame(int bit)
{
	inhibit_frame ^= 1 << bit;
}
