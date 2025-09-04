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

typedef int (*allocfunc_type)(int, int, int, xcolnr*);

extern xcolnr xcolors[4096];
extern uae_u32 p96_rgbx16[65536];
extern xcolnr fullblack;

extern int graphics_setup(void);
extern int graphics_init(bool);
extern void graphics_leave(void);
extern void graphics_reset(bool);
extern bool handle_events(void);
extern int handle_msgpump(bool);
extern void setup_brkhandler(void);
extern int isfullscreen(void);
extern void toggle_fullscreen(int monid, int);
extern bool toggle_rtg(int monid, int);
extern void close_rtg(int monid, bool reset);

extern void toggle_mousegrab(void);
void setmouseactivexy(int monid, int x, int y, int dir);

extern void desktop_coords(int monid, int* dw, int* dh, int* x, int* y, int* w, int* h);
extern bool vsync_switchmode(int monid, int hz);
extern void vsync_clear(void);
extern int vsync_isdone(frame_time_t*);
extern void doflashscreen(void);
extern int flashscreen;
extern void updatedisplayarea(int monid);
extern int isvsync_chipset(void);
extern int isvsync_rtg(void);
extern int isvsync(void);

extern void flush_line(struct vidbuffer*, int);
extern void flush_block(struct vidbuffer*, int, int);
extern void flush_screen(struct vidbuffer*, int, int);
extern void flush_clear_screen(struct vidbuffer*);
extern bool render_screen(int monid, int, bool);
extern void show_screen(int monid, int mode);
extern bool show_screen_maybe(int monid, bool);

extern int lockscr(struct vidbuffer*, bool, bool);
extern void unlockscr(struct vidbuffer*, int, int);
extern bool target_graphics_buffer_update(int monid, bool force);
extern float target_adjust_vblank_hz(int monid, float);
extern int target_get_display_scanline(int displayindex);
extern void target_spin(int);

void getgfxoffset(int monid, float *dxp, float *dyp, float *mxp, float *myp);
float target_getcurrentvblankrate(int monid);

extern int debuggable(void);
extern void screenshot(int monid, int, int);
void refreshtitle(void);

extern int bits_in_mask(unsigned long mask);
extern int mask_shift(unsigned long mask);
extern uae_u32 doMask(uae_u32 p, int bits, int shift);
extern uae_u32 doMask256(int p, int bits, int shift);
extern void alloc_colors64k(int monid, int, int, int, int, int, int, int, int, int, int);
extern void alloc_colors_rgb(int rw, int gw, int bw, int rs, int gs, int bs, int aw, int as, int alpha, int byte_swap,
	uae_u32* rc, uae_u32* gc, uae_u32* bc);
extern void alloc_colors_picasso(int rw, int gw, int bw, int rs, int gs, int bs, int rgbfmt, uae_u32* rgbx16);
extern float getvsyncrate(int monid, float hz, int* mult);


struct vidbuffer
{
	/* Function implemented by graphics driver */
	int  (*lockscr)            (struct vidbuf_description* gfxinfo, struct vidbuffer* vb);
	void (*unlockscr)          (struct vidbuf_description* gfxinfo, struct vidbuffer* vb);

	uae_u8* bufmem, * bufmemend;
	uae_u8* realbufmem;
	uae_u8* bufmem_allocated;
	bool initialized;
	bool locked;
	bool vram_buffer;
	int rowbytes; /* Bytes per row in the memory pointed at by bufmem. */
	int pixbytes; /* Bytes per pixel. */
	/* size of this buffer */
	int width_allocated;
	int height_allocated;
	/* size of max visible image */
	int outwidth;
	int outheight;
	/* nominal size of image for centering */
	int inwidth;
	int inheight;
	/* same but doublescan multiplier included */
	int inwidth2;
	int inheight2;
	/* static, hardwired screen position and size (A2024) */
	bool hardwiredpositioning;
	/* extra width, chipset hpos extra in right border */
	int extrawidth, extraheight;

	int xoffset; /* superhires pixels from left edge */
	int yoffset; /* lines from top edge */

	int inxoffset; /* sync positioning */
	int inyoffset;

	int monitor_id;
	int last_drawn_line;
};

extern bool isnativevidbuf(int monid);
extern int max_uae_width, max_uae_height;
extern bool gfx_hdr;

struct vidbuf_description
{
	struct vidbuffer drawbuffer;
	/* output buffer when using A2024 emulation */
	struct vidbuffer tempbuffer;

	struct vidbuffer* inbuffer;
	struct vidbuffer* outbuffer;

	int gfx_resolution_reserved; // reserved space for currprefs.gfx_resolution
	int gfx_vresolution_reserved; // reserved space for currprefs.gfx_resolution
	int xchange; /* how many superhires pixels in one pixel in buffer */
	int ychange; /* how many interlaced lines in one line in buffer */
};

struct amigadisplay
{
	volatile bool picasso_requested_on;
	bool picasso_requested_forced_on;
	bool picasso_on;
	bool interlace_on;
	int gf_index;
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
