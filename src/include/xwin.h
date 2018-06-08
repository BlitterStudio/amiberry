 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the graphics system (X, SVGAlib)
  *
  * Copyright 1995-1997 Bernd Schmidt
  */

#ifndef UAE_XWIN_H
#define UAE_XWIN_H

#include "uae/types.h"
#include "machdep/rpt.h"

typedef uae_u32 xcolnr;

typedef int (*allocfunc_type)(int, int, int, xcolnr *);

extern xcolnr xcolors[4096];
extern uae_u32 p96_rgbx16[65536];

extern int graphics_setup (void);
extern int graphics_init (bool);
extern void graphics_leave (void);
STATIC_INLINE void handle_events (void)
{
}
extern int handle_msgpump (void);

extern bool vsync_switchmode (int);
extern int isvsync_chipset(void);
extern int isvsync_rtg(void);
extern int isvsync(void);

extern bool render_screen (bool);
extern void show_screen (int);
extern bool show_screen_maybe (bool);

extern int lockscr (void);
extern void unlockscr (void);
extern bool target_graphics_buffer_update (void);

extern void screenshot (int);

extern int bits_in_mask (unsigned long mask);
extern int mask_shift (unsigned long mask);
extern unsigned int doMask (int p, int bits, int shift);
extern unsigned int doMask256 (int p, int bits, int shift);
extern void alloc_colors64k (int, int, int, int, int, int, int);
extern void alloc_colors_rgb(int rw, int gw, int bw, int rs, int gs, int bs, int byte_swap,
	uae_u32 *rc, uae_u32 *gc, uae_u32 *bc);
extern void alloc_colors_picasso (int rw, int gw, int bw, int rs, int gs, int bs, int rgbfmt);

/* The graphics code has a choice whether it wants to use a large buffer
* for the whole display, or only a small buffer for a single line.
* If you use a large buffer:
*   - set bufmem to point at it
*   - set linemem to 0
*   - if memcpy within bufmem would be very slow, i.e. because bufmem is
*     in graphics card memory, also set emergmem to point to a buffer
*     that is large enough to hold a single line.
*   - implement flush_line to be a no-op.
* If you use a single line buffer:
*   - set bufmem and emergmem to 0
*   - set linemem to point at your buffer
*   - implement flush_line to copy a single line to the screen
*/
struct vidbuffer
{
  uae_u8 *bufmem;
  int rowbytes; /* Bytes per row in the memory pointed at by bufmem. */
  int pixbytes; /* Bytes per pixel. */
	/* size of max visible image */
  int outwidth;
  int outheight;
};

extern int max_uae_width, max_uae_height;

struct vidbuf_description
{
  struct vidbuffer drawbuffer;
};

extern struct vidbuf_description gfxvidinfo;

struct amigadisplay
{
	bool picasso_requested_on;
	bool picasso_requested_forced_on;
	bool picasso_on;
	int picasso_redraw_necessary;
	int custom_frame_redraw_necessary;
	int frame_redraw_necessary;
	int framecnt;
	bool specialmonitoron;
	int inhibit_frame;
	bool pending_render;

	struct vidbuf_description gfxvidinfo;
};

extern struct amigadisplay adisplays[MAX_AMIGADISPLAYS];

#endif /* UAE_XWIN_H */
