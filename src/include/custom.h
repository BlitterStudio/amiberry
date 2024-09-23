/*
* UAE - The Un*x Amiga Emulator
*
* custom chip support
*
* (c) 1995 Bernd Schmidt
*/

#ifndef UAE_CUSTOM_H
#define UAE_CUSTOM_H

#include "uae/types.h"
#include "machdep/rpt.h"

#define BEAMCON0_HARDDIS	0x4000
#define BEAMCON0_LPENDIS	0x2000
#define BEAMCON0_VARVBEN	0x1000
#define BEAMCON0_LOLDIS		0x0800
#define BEAMCON0_CSCBEN		0x0400
#define BEAMCON0_VARVSYEN	0x0200
#define BEAMCON0_VARHSYEN	0x0100
#define BEAMCON0_VARBEAMEN	0x0080
#define BEAMCON0_DUAL		0x0040
#define BEAMCON0_PAL		0x0020
#define BEAMCON0_VARCSYEN	0x0010
#define BEAMCON0_BLANKEN	0x0008
#define BEAMCON0_CSYTRUE	0x0004
#define BEAMCON0_VSYTRUE	0x0002
#define BEAMCON0_HSYTRUE	0x0001

extern bool aga_mode, ecs_agnus, ecs_denise;
extern bool agnusa1000, denisea1000_noehb, denisea1000;
extern bool direct_rgb;

/* These are the masks that are ORed together in the chipset_mask option.
* If CSMASK_AGA is set, the ECS bits are guaranteed to be set as well.  */
#define CSMASK_ECS_AGNUS 1
#define CSMASK_ECS_DENISE 2
#define CSMASK_AGA 4
#define CSMASK_MASK (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA)

#define CHIPSET_CLOCK_PAL  3546895
#define CHIPSET_CLOCK_NTSC 3579545

#define MAXHPOS_ROWS 256
#define MAXVPOS_LINES_ECS 2048
#define MAXVPOS_LINES_OCS 512

#define BLIT_NASTY_CPU_STEAL_CYCLE_COUNT 3

uae_u32 get_copper_address(int copno);

extern int custom_init(void);
extern void custom_prepare(void);
extern void custom_reset(bool hardreset, bool keyboardreset);
extern int intlev(void);
extern void intlev_ack(int);
extern void dumpcustom(void);

extern void do_copper(void);

extern void notice_new_xcolors(void);
extern void notice_screen_contents_lost(int monid);
extern void init_row_map(void);
extern void init_hz_normal(void);
extern void init_custom(void);

extern void set_picasso_hack_rate(int hz);

/* Set to 1 to leave out the current frame in average frame time calculation.
* Useful if the debugger was active.  */
extern int bogusframe;
extern uae_u32 hsync_counter, vsync_counter;

extern uae_u16 dmacon;
extern uae_u16 intena, intreq, intreqr;

extern int vpos, lof_store, lof_display;
extern int scandoubled_line;

extern int n_frames;

STATIC_INLINE int dmaen(unsigned int dmamask)
{
	return (dmamask & dmacon) && (dmacon & 0x200);
}

extern uae_u16 adkcon;

extern unsigned int joy0dir, joy1dir;
extern int joy0button, joy1button;

extern void INTREQ(uae_u16);
extern bool INTREQ_0(uae_u16);
extern void INTREQ_f(uae_u16);
extern void INTREQ_INT(int num, int delay);
extern void rethink_uae_int(void);
extern uae_u16 INTREQR(void);

/* maximums for statically allocated tables */
#ifdef UAE_MINI
/* absolute minimums for basic A500/A1200-emulation */
#define MAXHPOS 227
#define MAXVPOS 312
#else
#define MAXHPOS 256
#define MAXVPOS 800
#endif

/* PAL/NTSC values */

#define MAXHPOS_PAL 227
#define MAXHPOS_NTSC 227
// short field maxvpos
#define MAXVPOS_PAL 312
#define MAXVPOS_NTSC 262
// following endlines = first visible line
#define VBLANK_ENDLINE_PAL 26
#define VBLANK_ENDLINE_NTSC 21
// line when sprite DMA fetches first control words
#define VBLANK_STOP_PAL 25
#define VBLANK_STOP_NTSC 20
#define VBLANK_HZ_PAL 50
#define VBLANK_HZ_NTSC 60
#define VSYNC_ENDLINE_PAL 5
#define VSYNC_ENDLINE_NTSC 6
#define EQU_ENDLINE_PAL 8
#define EQU_ENDLINE_NTSC 9

#define OCS_DENISE_HBLANK_DISABLE_HPOS 0x2e

extern int maxhpos, maxhposm0, maxhpos_short;
extern int maxvpos, maxvpos_nom, maxvpos_display, maxvpos_display_vsync, maxhpos_display;
extern int maxvsize_display;
extern int hsyncstartpos_hw, hsyncendpos_hw;
extern int minfirstline, minfirstline_linear, vblank_endline, numscrlines;
extern float vblank_hz, fake_vblank_hz;
extern float hblank_hz;
extern int vblank_skip, doublescan;
extern int programmedmode;
extern int vblank_firstline_hw;
extern int display_reset;

#define DMA_AUD0      0x0001
#define DMA_AUD1      0x0002
#define DMA_AUD2      0x0004
#define DMA_AUD3      0x0008
#define DMA_DISK      0x0010
#define DMA_SPRITE    0x0020
#define DMA_BLITTER   0x0040
#define DMA_COPPER    0x0080
#define DMA_BITPLANE  0x0100
#define DMA_MASTER    0x0200
#define DMA_BLITPRI   0x0400

#define CYCLE_BITPLANE  1
#define CYCLE_REFRESH	2
#define CYCLE_STROBE	3
#define CYCLE_MISC		4
#define CYCLE_SPRITE	5
#define CYCLE_COPPER	6
#define CYCLE_BLITTER	7
#define CYCLE_CPU		8

#define CYCLE_MASK 0x0f

extern uae_u32 timeframes;
extern evt_t frametime;
extern uae_u16 htotal, vtotal, beamcon0, new_beamcon0;
extern uae_u16 bemcon0_hsync_mask, bemcon0_vsync_mask;

// 100 words give you 1600 horizontal pixels. Should be more than enough for superhires. 
// Extreme overscan superhires needs more.
// must be divisible by 8
#ifdef CUSTOM_SIMPLE
#define MAX_WORDS_PER_LINE 56
#else
#define MAX_WORDS_PER_LINE 112
#endif

extern uae_u32 hirestab_h[256][2];
extern uae_u32 lorestab_h[256][4];

extern uae_u32 hirestab_l[256][1];
extern uae_u32 lorestab_l[256][2];

#ifdef AGA
/* AGA mode color lookup tables */
extern unsigned int xredcolors[256], xgreencolors[256], xbluecolors[256];
#endif
extern int xredcolor_s, xredcolor_b, xredcolor_m;
extern int xgreencolor_s, xgreencolor_b, xgreencolor_m;
extern int xbluecolor_s, xbluecolor_b, xbluecolor_m;

#define RES_LORES 0
#define RES_HIRES 1
#define RES_SUPERHIRES 2
#define RES_MAX 2
#define VRES_NONDOUBLE 0
#define VRES_DOUBLE 1
#define VRES_QUAD 2
#define VRES_MAX 1

/* calculate shift depending on resolution (replaced "decided_hires ? 4 : 8") */
#define RES_SHIFT(res) ((res) == RES_LORES ? 8 : (res) == RES_HIRES ? 4 : 2)

/* get resolution from bplcon0 */
STATIC_INLINE int GET_RES_DENISE(uae_u16 con0)
{
	if (!ecs_denise) {
		con0 &= ~0x40; // SUPERHIRES
	}
	return ((con0) & 0x40) ? RES_SUPERHIRES : ((con0) & 0x8000) ? RES_HIRES : RES_LORES;
}
STATIC_INLINE int GET_RES_AGNUS(uae_u16 con0)
{
	if (!ecs_agnus) {
		con0 &= ~0x40; // SUPERHIRES
	}
	return ((con0) & 0x40) ? RES_SUPERHIRES : ((con0) & 0x8000) ? RES_HIRES : RES_LORES;
}
/* get sprite width from FMODE */
#define GET_SPRITEWIDTH(FMODE) ((((FMODE) >> 2) & 3) == 3 ? 64 : (((FMODE) >> 2) & 3) == 0 ? 16 : 32)
/* Compute the number of bitplanes from a value written to BPLCON0  */
STATIC_INLINE int GET_PLANES(uae_u16 bplcon0)
{
	if ((bplcon0 & 0x0010) && (bplcon0 & 0x7000))
		return 0; // >8 planes = 0 planes
	if (bplcon0 & 0x0010)
		return 8; // AGA 8-planes bit
	return (bplcon0 >> 12) & 7; // normal planes bits
}

extern void fpscounter_reset(void);
extern frame_time_t idletime;
extern int lightpen_x[2], lightpen_y[2];
extern int lightpen_cx[2], lightpen_cy[2], lightpen_active, lightpen_enabled, lightpen_enabled2;

struct customhack {
	uae_u16 v;
	int vpos, hpos;
};
extern void alloc_cycle_ext(int, int);
extern bool alloc_cycle_blitter(int hpos, uaecptr *ptr, int, int);
extern uaecptr alloc_cycle_blitter_conflict_or(int, int, bool*);
extern bool ispal(int *lines);
extern bool isvga(void);
extern int current_maxvpos(void);
extern struct chipset_refresh *get_chipset_refresh(struct uae_prefs*);
extern void compute_framesync(void);
extern void getsyncregisters(uae_u16 *phsstrt, uae_u16 *phsstop, uae_u16 *pvsstrt, uae_u16 *pvsstop);
bool blitter_cant_access(int hpos);
void custom_cpuchange(void);
bool bitplane_dma_access(int hpos, int offset);
void custom_dumpstate(int);
bool get_ras_cas(uaecptr, int*, int*);
void get_mode_blanking_limits(int *phbstop, int *phbstrt, int *pvbstop, int *pvbstrt);

#define RGA_PIPELINE_ADJUST 4
#define MAX_CHIPSETSLOTS 256
#define MAX_CHIPSETSLOTS_EXTRA 12
extern uae_u8 cycle_line_slot[MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST + MAX_CHIPSETSLOTS_EXTRA];
extern uae_u16 cycle_line_pipe[MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST + MAX_CHIPSETSLOTS_EXTRA];
extern uae_u16 blitter_pipe[MAX_CHIPSETSLOTS + RGA_PIPELINE_ADJUST + MAX_CHIPSETSLOTS_EXTRA];

#define CYCLE_PIPE_CPUSTEAL 0x8000
#define CYCLE_PIPE_NONE 0x4000
#define CYCLE_PIPE_BLITTER 0x100
#define CYCLE_PIPE_COPPER 0x80
#define CYCLE_PIPE_SPRITE 0x40
#define CYCLE_PIPE_BITPLANE 0x20
#define CYCLE_PIPE_MODULO 0x10

#define RGA_PIPELINE_MASK 255

#define RGA_PIPELINE_OFFSET_BLITTER 1

extern int rga_pipeline_blitter;

STATIC_INLINE int get_rga_pipeline(int hpos, int off)
{
	return (hpos + off) % maxhposm0;
}

struct custom_store
{
	uae_u16 value;
	uae_u32 pc;
};
extern struct custom_store custom_storage[256];

#endif /* UAE_CUSTOM_H */
