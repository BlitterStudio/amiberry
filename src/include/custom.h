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

extern bool aga_mode, ecs_agnus, ecs_denise, ecs_denise_only;
extern bool agnusa1000, denisea1000_noehb, denisea1000;
extern bool direct_rgb;

/* These are the masks that are ORed together in the chipset_mask option.
* If CSMASK_AGA is set, the ECS bits are guaranteed to be set as well.  */
#define CSMASK_OCS 0
#define CSMASK_ECS_AGNUS 1
#define CSMASK_ECS_DENISE 2
#define CSMASK_AGA 4
#define CSMASK_A1000_NOEHB 0x10
#define CSMASK_A1000 0x20
#define CSMASK_MASK (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA | CSMASK_A1000)

#define CHIPSET_CLOCK_PAL  3546895
#define CHIPSET_CLOCK_NTSC 3579545

#define MAXHPOS_ROWS 256
#define MAXVPOS_LINES_ECS 2048
#define MAXVPOS_LINES_OCS 512

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
extern void init_hz(void);
extern void init_custom(void);

extern void set_picasso_hack_rate(int hz);

/* Set to 1 to leave out the current frame in average frame time calculation.
* Useful if the debugger was active.  */
extern int bogusframe;
extern uae_u32 hsync_counter, vsync_counter;

extern uae_u16 dmacon;
extern uae_u16 intena, intreq, intreqr;

extern int vpos, linear_vpos;
extern uae_u8 agnus_hpos;
extern bool lof_store, lof_display;
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
extern int minfirstline, minfirstline_linear, vblank_endline, numscrlines, minfirstline_linear;
extern float vblank_hz, fake_vblank_hz;
extern float hblank_hz;
extern int vblank_skip, doublescan;
extern int programmedmode;
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

#define CYCLE_BITPLANE  (1 << 0)
#define CYCLE_REFRESH	(1 << 1)
#define CYCLE_STROBE	(1 << 2)
#define CYCLE_DISK		(1 << 3)
#define CYCLE_AUDIO		(1 << 4)
#define CYCLE_SPRITE	(1 << 5)
#define CYCLE_COPPER	(1 << 6)
#define CYCLE_UHRESBPL	(1 << 7)
#define CYCLE_UHRESSPR	(1 << 8)
#define CYCLE_BLITTER	(1 << 9)
#define CYCLE_CPU		(1 << 10)

extern uae_u32 timeframes;
extern evt_t frametime;
extern uae_u16 htotal, vtotal, beamcon0, new_beamcon0;
extern uae_u16 bemcon0_hsync_mask, bemcon0_vsync_mask;

#define MAX_WORDS_PER_LINE 112

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

extern bool ispal(int *lines);
extern bool isvga(void);
extern int current_maxvpos(void);
extern struct chipset_refresh *get_chipset_refresh(struct uae_prefs*);
extern void compute_framesync(void);
extern void getsyncregisters(uae_u16 *phsstrt, uae_u16 *phsstop, uae_u16 *pvsstrt, uae_u16 *pvsstop);
bool blitter_cant_access(void);
void custom_cpuchange(void);
void custom_dumpstate(int);
bool get_ras_cas(uaecptr, int*, int*);
void get_mode_blanking_limits(int *phbstop, int *phbstrt, int *pvbstop, int *pvbstrt);

#define CYCLE_PIPE_CPUFREE 0x4000
#define CYCLE_PIPE_BLITTER 0x100
#define CYCLE_PIPE_COPPER 0x80
#define CYCLE_PIPE_SPRITE 0x40
#define CYCLE_PIPE_BITPLANE 0x20
#define CYCLE_PIPE_MODULO 0x10

#define RGA_PIPELINE_MASK 255

#define RGA_PIPELINE_OFFSET_BLITTER 1

struct custom_store
{
	uae_u16 value;
	uae_u32 pc;
};
extern struct custom_store custom_storage[256];

#define DENISE_RGA_FLAG_SYNC 0x01
#define DENISE_RGA_FLAG_CSYNC 0x02
#define DENISE_RGA_FLAG_VSYNC 0x04
#define DENISE_RGA_FLAG_HSYNC 0x08
#define DENISE_RGA_FLAG_BLANKEN_CSYNC 0x10
#define DENISE_RGA_FLAG_BLANKEN_CSYNC_ON 0x20
#define DENISE_RGA_FLAG_LOL 0x40
#define DENISE_RGA_FLAG_LOL_ON 0x80

#define DENISE_RGA_FLAG_NEXTLAST_VB 0x10

struct denise_rga
{
	union {
		uae_u64 v64;
		uae_u32 v;
	};
	uae_u16 rga;
	uae_u32 line;
	uae_u32 flags;
	uaecptr pt;
#ifdef DEBUGGER
	struct dma_rec *dr;
#endif
};

#define DENISE_RGA_SLOT_CHUNKS 8
#define DENISE_RGA_SLOT_CHUNKS_MASK (DENISE_RGA_SLOT_CHUNKS - 1)
#define DENISE_RGA_SLOT_TOTAL (512 * DENISE_RGA_SLOT_CHUNKS)
#define DENISE_RGA_SLOT_MASK (DENISE_RGA_SLOT_TOTAL - 1)
extern struct denise_rga rga_denise[DENISE_RGA_SLOT_TOTAL];

struct denise_fastsprite
{
	union {
		uae_u64 data64[2];
		uae_u32 data[2];
	};
	uae_u16 pos, ctl;
	bool active;
};

struct rgabuf {
	uae_u32 *p;
	uae_u32 pv;
	uae_u16 reg;
	uae_u16 value;
	uae_u32 type;
	uae_u32 mwmask;
	int alloc;
	bool write;
	uae_u32 *conflict;
	uaecptr conflictaddr;
	uae_u32 bpldat, sprdat, bltdat, auddat, refdat, dskdat, copdat;
	uae_s16 bplmod, bltmod, bltadd;
};

#define RGA_SLOT_BPL 0
#define RGA_SLOT_IN 1
#define RGA_SLOT_OUT 2
#define RGA_SLOT_TOTAL 3

bool check_rga(int slot);
bool check_rga_free_slot_in(void);
struct rgabuf *read_rga(int slot);
struct rgabuf *write_rga(int slot, int type, uae_u16 v, uae_u32 *p);
extern uae_u16 clxdat;

void custom_end_drawing(void);
void resetfulllinestate(void);

extern int current_linear_vpos, current_linear_hpos;
extern uae_u8 agnus_hpos;

void vsync_event_done(void);
bool get_custom_color_reg(int colreg, uae_u8 *r, uae_u8 *g, uae_u8 *b);
void event_doint_delay_do_ext(uae_u32 v);

#endif /* UAE_CUSTOM_H */
