/*
* Data used for communication between custom.c and drawing.c.
*
* Copyright 1996-1998 Bernd Schmidt
*/

#ifndef UAE_DRAWING_H
#define UAE_DRAWING_H

#define DISABLE_BPLCON1 0

#include "uae/types.h"

#ifdef AGA
#define MAX_PLANES 8
#else
#define MAX_PLANES 6
#endif

#define MAX_SPRITES 8

extern int interlace_seen;
extern int visible_left_border, visible_right_border;
extern int detected_screen_resolution;
extern int hsync_end_left_border, hdisplay_left_border, denisehtotal;
extern int vsync_startline;

#define AMIGA_WIDTH_MAX (754 / 2)
#define AMIGA_HEIGHT_MAX_PAL (576 / 2)
#define AMIGA_HEIGHT_MAX_NTSC (486 / 2)
#define AMIGA_HEIGHT_MAX (AMIGA_HEIGHT_MAX_PAL)

#define CCK_SHRES_SHIFT 3

/* color values in two formats: 12 (OCS/ECS) or 24 (AGA) bit Amiga RGB (color_regs),
* and the native color value; both for each Amiga hardware color register.
*
* !!! See color_reg_xxx functions below before touching !!!
*/


struct color_entry {
	uae_u16 color_regs_ecs[32];
#ifndef AGA
	xcolnr acolors[32];
#else
	xcolnr acolors[256];
	uae_u32 color_regs_aga[256];
#endif
	bool color_regs_genlock[256];
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
STATIC_INLINE int color_reg_get(struct color_entry *ce, int c)
{
#ifdef AGA
	if (aga_mode)
		return ce->color_regs_aga[c];
	else
#endif
		return ce->color_regs_ecs[c];
}

#define MAX_PIXELS_PER_LINE 2304

/* Functions in drawing.c.  */
extern int coord_native_to_amiga_y(int);
extern int coord_native_to_amiga_x(int);

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
	nln_lower_black_always,
	nln_none
};

extern void vsync_handle_redraw(int long_field, uae_u16, uae_u16, bool drawlines, bool initial);
extern bool vsync_handle_check(void);
extern void reset_drawing(void);
extern void drawing_init(void);
extern bool frame_drawn(int monid);
extern void redraw_frame(void);
extern void full_redraw_all(void);
extern int get_custom_limits(int *pw, int *ph, int *pdx, int *pdy, int *prealh, int *hres, int *vres);
extern void store_custom_limits(int w, int h, int dx, int dy);
extern void set_custom_limits(int w, int h, int dx, int dy, bool blank);
extern void check_custom_limits(void);
extern void get_custom_topedge(int *x, int *y, bool max);
extern void get_custom_raw_limits(int *pw, int *ph, int *pdx, int *pdy);
extern void get_custom_mouse_limits(int *pw, int *ph, int *pdx, int *pdy, int dbl);
extern void putpixel(uae_u8 *buf, uae_u8 *genlockbuf, int x, xcolnr c8);
extern void allocvidbuffer(int monid, struct vidbuffer *buf, int width, int height, int depth);
extern void freevidbuffer(int monid, struct vidbuffer *buf);
extern void check_prefs_picasso(void);
extern int get_vertical_visible_height(bool);
extern void get_mode_blanking_limits(int *phbstop, int *phbstrt, int *pvbstop, int *pvbstrt);
extern void notice_resolution_seen(int res, bool lace);

/* Finally, stuff that shouldn't really be shared.  */

#define IHF_SCROLLLOCK 0
#define IHF_QUIT_PROGRAM 1
#define IHF_PICASSO 2

void set_inhibit_frame(int monid, int bit);
void clear_inhibit_frame(int monid, int bit);
void toggle_inhibit_frame(int monid, int bit);

#define LINE_DRAW_COUNT 3
#define LINETYPE_BLANK 1
#define LINETYPE_BORDER 2
#define LINETYPE_BPL 3
struct linestate
{
	int type;
	uae_u32 cnt;
	uae_u16 ddfstrt, ddfstop;
	uae_u16 diwstrt, diwstop, diwhigh;
	uae_u16 bplcon0, bplcon1, bplcon2, bplcon3, bplcon4;
	uae_u16 fmode;
	uae_u32 color0;
	bool brdblank;
	uae_u8 *linecolorstate;
	int bpllen;
	int colors;
	uae_u8 *bplpt[MAX_PLANES];
	int hbstrt_offset, hbstop_offset;
	int hstrt_offset, hstop_offset;
	int bpl1dat_trigger_offset;
	int internal_pixel_cnt;
	int internal_pixel_start_cnt;
	bool lol;
	bool blankedline;
	bool vb;
	bool strlong_seen;
	int lol_shift_prev;
	int fetchmode_size, fetchstart_mask;
	uae_u16 strobe;
	int strobe_pos;
};

extern struct color_entry denise_colors;
void draw_denise_line_queue(int gfx_ypos, nln_how how, uae_u32 linecnt, int startpos, int endpos, int startcycle, int endcycle, int skip, int skip2, int dtotal, int calib_start, int calib_len, bool lof, bool lol, int hdelay, bool blanked, bool finalseg, struct linestate *ls);
void draw_denise_bitplane_line_fast(int gfx_ypos, enum nln_how how, struct linestate *ls);
void draw_denise_bitplane_line_fast_queue(int gfx_ypos, enum nln_how how, struct linestate *ls);
void draw_denise_border_line_fast(int gfx_ypos, enum nln_how how, struct linestate *ls);
void draw_denise_border_line_fast_queue(int gfx_ypos, enum nln_how how, struct linestate *ls);
bool start_draw_denise(void);
void end_draw_denise(void);
void denise_reset(bool);
bool denise_update_reg_queued(uae_u16 reg, uae_u16 v, uae_u32 linecnt);
void denise_store_registers(void);
void denise_restore_registers(void);
bool denise_is_vb(void);
void draw_denise_vsync_queue(int);
void draw_denise_line_queue_flush(void);
void quick_denise_rga_queue(uae_u32 linecnt, int startpos, int endpos);
void denise_handle_quick_strobe_queue(uae_u16 strobe, int strobe_pos, int endpos);
bool drawing_can_lineoptimizations(void);
void set_drawbuffer(void);
int gethresolution(void);
void denise_update_reg_queue(uae_u16 reg, uae_u16 v, uae_u32 linecnt);
void denise_store_restore_registers_queue(bool store, uae_u32 linecnt);
void denise_clearbuffers(void);
uae_u8 *get_row_genlock(int monid, int line);

#endif /* UAE_DRAWING_H */
