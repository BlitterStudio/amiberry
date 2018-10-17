/*
 * Data used for communication between custom.c and drawing.c.
 * 
 * Copyright 1996-1998 Bernd Schmidt
 */

#ifndef UAE_DRAWING_H
#define UAE_DRAWING_H

#include "uae/types.h"

#define SMART_UPDATE 1

#define MAX_PLANES 8

/* According to the HRM, pixel data spends a couple of cycles somewhere in the chips
before it appears on-screen. (TW: display emulation now does this automatically)  */
#define DIW_DDF_OFFSET 1
/* this many cycles starting from hpos=0 are visible on right border */
#define HBLANK_OFFSET 9
/* We ignore that many lores pixels at the start of the display. These are
 * invisible anyway due to hardware DDF limits. */
#define DISPLAY_LEFT_SHIFT 0x38

#define PIXEL_XPOS(HPOS) (((HPOS)*2 - DISPLAY_LEFT_SHIFT + DIW_DDF_OFFSET - 1) << lores_shift)

#define min_diwlastword (0)
#define max_diwlastword   (PIXEL_XPOS(0x1d4>> 1))

extern int lores_shift, interlace_seen;
extern bool aga_mode;

STATIC_INLINE int shres_coord_hw_to_window_x (int x)
{
	x -= DISPLAY_LEFT_SHIFT << 2;
	x <<= lores_shift;
	x >>= 2;
	return x;
}

STATIC_INLINE int coord_hw_to_window_x (int x)
{
  x -= DISPLAY_LEFT_SHIFT;
	return x << lores_shift;
}

STATIC_INLINE int coord_window_to_hw_x (int x)
{
	x >>= lores_shift;
  return x + DISPLAY_LEFT_SHIFT;
}

STATIC_INLINE int coord_diw_to_window_x (int x)
{
	return (x - DISPLAY_LEFT_SHIFT + DIW_DDF_OFFSET - 1) << lores_shift;
}

STATIC_INLINE int coord_window_to_diw_x (int x)
{
  x = coord_window_to_hw_x (x);
  return x - DIW_DDF_OFFSET;
}

extern int framecnt;

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
  xcolnr acolors[256];
  uae_u32 color_regs_aga[256];
	uae_u8 extra;
};

/* convert 24 bit AGA Amiga RGB to native color */
#ifdef WORDS_BIGENDIAN
# define CONVERT_RGB(c) \
	( xbluecolors[((uae_u8*)(&c))[3]] | xgreencolors[((uae_u8*)(&c))[2]] | xredcolors[((uae_u8*)(&c))[1]] )
#else
#define CONVERT_RGB(c) \
    ( xbluecolors[((uae_u8*)(&c))[0]] | xgreencolors[((uae_u8*)(&c))[1]] | xredcolors[((uae_u8*)(&c))[2]] )
#define CONVERT_RGB_16(c) \
    ( xbluecolors[((uae_u8*)(&c))[0]] | xgreencolors[((uae_u8*)(&c))[1]] | xredcolors[((uae_u8*)(&c))[2]] )
#endif

STATIC_INLINE xcolnr getxcolor (int c)
{
	if (aga_mode)
		return CONVERT_RGB(c);
	else
		return xcolors[c];
}

/* functions for reading, writing, copying and comparing struct color_entry */
STATIC_INLINE int color_reg_get (struct color_entry *ce, int c)
{
	if (aga_mode)
		return ce->color_regs_aga[c];
	else
		return ce->color_regs_ecs[c];
}

STATIC_INLINE void color_reg_set (struct color_entry *ce, int c, int v)
{
	if (aga_mode)
		ce->color_regs_aga[c] = v;
	else
		ce->color_regs_ecs[c] = v;
}

/* ugly copy hack, is there better solution? */
STATIC_INLINE void color_reg_cpy (struct color_entry *dst, struct color_entry *src)
{
	dst->extra = src->extra;
  if (aga_mode)
  	/* copy acolors and color_regs_aga */
  	memcpy (dst->acolors, src->acolors, sizeof(struct color_entry) - sizeof(uae_u16) * 32);
  else
  	/* copy first 32 acolors and color_regs_ecs */
  	memcpy(dst->color_regs_ecs, src->color_regs_ecs, sizeof(uae_u16) * 32 + sizeof(xcolnr) * 32);
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
#define MAX_PIXELS_PER_LINE 1760

/* No divisors for MAX_PIXELS_PER_LINE; we support AGA and SHRES sprites  */
#define MAX_SPR_PIXELS (((MAXVPOS + 1)*2 + 1) * MAX_PIXELS_PER_LINE)

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
#define MAX_REG_CHANGE ((MAXVPOS + 1) * MAXHPOS)
#define COLOR_TABLE_SIZE (MAXVPOS + 2) * 2

extern struct color_entry curr_color_tables[COLOR_TABLE_SIZE];

extern struct sprite_entry *curr_sprite_entries;
extern struct color_change *curr_color_changes;
extern struct draw_info curr_drawinfo[2 * (MAXVPOS + 2) + 1];

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
#ifdef AGA
	bool bordersprite_seen;
	bool xor_seen;
#endif
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
extern int coord_native_to_amiga_y (int);
extern int coord_native_to_amiga_x (int);

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

extern void hsync_record_line_state (int lineno);
extern void halt_draw_frame(void);
extern void vsync_handle_redraw (void);
extern bool vsync_handle_check (void);
extern void init_hardware_for_drawing_frame (void);
extern void reset_drawing (void);
extern void drawing_init (void);
extern bool notice_interlace_seen (bool);
extern void putpixel(uae_u8* buf, uae_u8* genlockbuf, int bpp, int x, xcolnr c8, int opaq);
extern void check_prefs_picasso(void);

/* Finally, stuff that shouldn't really be shared.  */

#define IHF_SCROLLLOCK 0
#define IHF_QUIT_PROGRAM 1
#define IHF_PICASSO 2

extern int inhibit_frame;

STATIC_INLINE void set_inhibit_frame (int bit)
{
  inhibit_frame |= 1 << bit;
}
STATIC_INLINE void clear_inhibit_frame (int bit)
{
  inhibit_frame &= ~(1 << bit);
}

#endif /* UAE_DRAWING_H */
