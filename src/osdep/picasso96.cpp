/*
 * UAE - The U*nix Amiga Emulator
 *
 * Picasso96 Support Module
 *
* Copyright 1997-2001 Brian King <Brian_King@CodePoet.com>
* Copyright 2000-2001 Bernd Roesch <>
 *
 * Theory of operation:
 * On the Amiga side, a Picasso card consists mainly of a memory area that
 * contains the frame buffer.  On the UAE side, we allocate a block of memory
 * that will hold the frame buffer.  This block is in normal memory, it is
 * never directly on the graphics card. All graphics operations, which are
 * mainly reads and writes into this block and a few basic operations like
 * filling a rectangle, operate on this block of memory.
 * Since the memory is not on the graphics card, some work must be done to
 * synchronize the display with the data in the Picasso frame buffer.  There
 * are various ways to do this. One possibility is to allocate a second
 * buffer of the same size, and perform all write operations twice.  Since
 * we never read from the second buffer, it can actually be placed in video
 * memory.  The X11 driver could be made to use the Picasso frame buffer as
 * the data buffer of an XImage, which could then be XPutImage()d from time
 * to time.  Another possibility is to translate all Picasso accesses into
 * Xlib (or GDI, or whatever your graphics system is) calls.  This possibility
 * is a bit tricky, since there is a risk of generating very many single pixel
 * accesses which may be rather slow.
 *
 * TODO:
 * - we want to add a manual switch to override SetSwitch for hardware banging
 *   programs started from a Picasso workbench.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#if defined(PICASSO96)

#define NEWTRAP 1

#include "config.h"
#include "options.h"
#include "threaddep/thread.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "xwin.h"
#include "savestate.h"
#include "autoconf.h"
#include "traps.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif
#include "picasso96.h"
#include "gfxboard.h"
#include "devices.h"

#define NOBLITTER 0
#define NOBLITTER_BLIT 0
#define NOBLITTER_ALL 0

static const int defaultHz = 60;

static int picasso96_BT = BT_uaegfx;
static int picasso96_GCT = GCT_Unknown;
static int picasso96_PCT = PCT_Unknown;

static bool picasso_flushpixels(uae_u8 *src, int offset);

int p96refresh_active;

static int p96syncrate;
static smp_comm_pipe* render_pipe;
static volatile int render_thread_state;

#define PICASSO_STATE_SETDISPLAY 1
#define PICASSO_STATE_SETPANNING 2
#define PICASSO_STATE_SETGC 4
#define PICASSO_STATE_SETDAC 8
#define PICASSO_STATE_SETSWITCH 16

#ifdef PICASSO96

static uae_u8 all_ones_bitmap, all_zeros_bitmap; /* yuk */

struct picasso96_state_struct picasso96_state;
struct picasso_vidbuf_description picasso_vidinfo;
static struct PicassoResolution *newmodes;

/* These are the maximum resolutions... They are filled in by GetSupportedResolutions()
 have to fill this in, otherwise problems occur on the Amiga side P96 s/w which expects
 data here. */
static struct ScreenResolution planar = { 320, 240 };
static struct ScreenResolution chunky = { 640, 480 };
static struct ScreenResolution hicolour = { 640, 480 };
static struct ScreenResolution truecolour = { 640, 480 };
static struct ScreenResolution alphacolour = { 640, 480 };

uae_u32 p96_rgbx16[65536];
uae_u32 p96rc[256], p96gc[256], p96bc[256];

static uaecptr boardinfo, ABI_interrupt;
static int interrupt_enabled;
float p96vblank;

static int uaegfx_old, uaegfx_active;
static uae_u32 reserved_gfxmem;
static uaecptr uaegfx_resname,
	uaegfx_resid,
	uaegfx_init,
	uaegfx_base,
	uaegfx_rom;

typedef enum {
	BLIT_FALSE,
	BLIT_NOR,
	BLIT_ONLYDST,
	BLIT_NOTSRC,
	BLIT_ONLYSRC,
	BLIT_NOTDST,
	BLIT_EOR,
	BLIT_NAND,
	BLIT_AND,
	BLIT_NEOR,
	BLIT_DST,
	BLIT_NOTONLYSRC,
	BLIT_SRC,
	BLIT_NOTONLYDST,
	BLIT_OR,
	BLIT_TRUE,
	BLIT_SWAP = 30
} BLIT_OPCODE;

static void init_picasso_screen();
static uae_u32 p2ctab[256][2];
static int set_gc_called = 0, init_picasso_screen_called = 0;
//fastscreen
static uaecptr oldscr = 0;

extern addrbank gfxmem_bank;
extern addrbank* gfxmem_banks[MAX_RTG_BOARDS];

STATIC_INLINE void endianswap(uae_u32* vp, int bpp)
{
	uae_u32 v = *vp;
	switch (bpp)
	{
	case 2:
		*vp = bswap_16(v);
		break;
	case 4:
		*vp = bswap_32(v);
		break;
	}
}

static uae_u8 GetBytesPerPixel(uae_u32 RGBfmt)
{
	switch (RGBfmt)
	{
	case RGBFB_CLUT:
	case RGBFB_Y4U1V1:
		return 1;

	case RGBFB_A8R8G8B8:
	case RGBFB_A8B8G8R8:
	case RGBFB_R8G8B8A8:
	case RGBFB_B8G8R8A8:
		return 4;

	case RGBFB_B8G8R8:
	case RGBFB_R8G8B8:
		return 3;

	case RGBFB_R5G5B5:
	case RGBFB_R5G6B5:
	case RGBFB_R5G6B5PC:
	case RGBFB_R5G5B5PC:
	case RGBFB_B5G6R5PC:
	case RGBFB_B5G5R5PC:
	case RGBFB_Y4U2V2:
		return 2;
	}
	return 0;
}

STATIC_INLINE bool validatecoords2(TrapContext *ctx, struct RenderInfo *ri, uae_u32 *Xp, uae_u32 *Yp, uae_u32 *Widthp, uae_u32 *Heightp)
{
	uae_u32 Width = *Widthp;
	uae_u32 Height = *Heightp;
	uae_u32 X = *Xp;
	uae_u32 Y = *Yp;
	if (!Width || !Height)
		return true;
	if (ri) {
		int bpp = GetBytesPerPixel (ri->RGBFormat);
		if (X * bpp >= ri->BytesPerRow)
			return false;
		uae_u32 X2 = X + Width;
		if (X2 * bpp > ri->BytesPerRow) {
			X2 = ri->BytesPerRow / bpp;
			Width = X2 - X;
			*Widthp = Width;
		}
		if (!valid_address(ri->AMemory, (Y + Height - 1) * ri->BytesPerRow + (X + Width - 1) * bpp))
			return false;
	}
	return true;
}
static bool validatecoords(TrapContext *ctx, struct RenderInfo *ri, uae_u32 *X, uae_u32 *Y, uae_u32 *Width, uae_u32 *Height)
{
	if (validatecoords2(ctx, ri, X, Y, Width, Height))
		return true;
	write_log (_T("RTG invalid region: %08X:%d:%d (%dx%d)-(%dx%d)\n"), ri->AMemory, ri->BytesPerRow, ri->RGBFormat, *X, *Y, *Width, *Height);
	return false;
}

/*
 * Amiga <-> native structure conversion functions
 */

static int CopyRenderInfoStructureA2U(TrapContext* ctx, uaecptr amigamemptr, struct RenderInfo* ri)
{
	if (trap_valid_address(ctx, amigamemptr, PSSO_RenderInfo_sizeof)) {
		struct trapmd md[] =
		{
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_RenderInfo_Memory } },
			{ TRAPCMD_GET_WORD, { amigamemptr + PSSO_RenderInfo_BytesPerRow } },
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_RenderInfo_RGBFormat } }
		};
		trap_multi(ctx, md, sizeof md / sizeof(struct trapmd));
		uaecptr memp = md[0].params[0];
		ri->AMemory = memp;
		ri->Memory = get_real_address (memp);
		ri->BytesPerRow = md[1].params[0];
		ri->RGBFormat = (RGBFTYPE)md[2].params[0];
		// Can't really validate this better at this point, no height.
		if (trap_valid_address(ctx, memp, ri->BytesPerRow))
			return 1;
	}
	write_log (_T("ERROR - Invalid RenderInfo memory area...\n"));
	return 0;
}

static int CopyPatternStructureA2U(TrapContext* ctx, uaecptr amigamemptr, struct Pattern* pattern)
{
	if (trap_valid_address(ctx, amigamemptr, PSSO_Pattern_sizeof)) {
		struct trapmd md[] =
		{
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_Pattern_Memory } },
			{ TRAPCMD_GET_WORD, { amigamemptr + PSSO_Pattern_XOffset } },
			{ TRAPCMD_GET_WORD, { amigamemptr + PSSO_Pattern_YOffset } },
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_Pattern_FgPen } },
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_Pattern_BgPen } },
			{ TRAPCMD_GET_BYTE, { amigamemptr + PSSO_Pattern_Size } },
			{ TRAPCMD_GET_BYTE, { amigamemptr + PSSO_Pattern_DrawMode } }
		};
		trap_multi(ctx, md, sizeof md / sizeof(struct trapmd));
		uaecptr memp = md[0].params[0];
		pattern->Memory = get_real_address(memp);
		pattern->AMemory = memp;
		pattern->XOffset = md[1].params[0];
		pattern->YOffset = md[2].params[0];
		pattern->FgPen = md[3].params[0];
		pattern->BgPen = md[4].params[0];
		pattern->Size = md[5].params[0];
		pattern->DrawMode = md[6].params[0];
		if (trap_valid_address(ctx, memp, 2))
			return 1;
	}
	write_log (_T("ERROR - Invalid Pattern memory area...\n"));
	return 0;
}

static int CopyBitMapStructureA2U(TrapContext* ctx, uaecptr amigamemptr, struct BitMap* bm)
{
	int i;

	struct trapmd md[] =
	{
		{ TRAPCMD_GET_WORD, { amigamemptr + PSSO_BitMap_BytesPerRow } },
		{ TRAPCMD_GET_WORD, { amigamemptr + PSSO_BitMap_Rows } },
		{ TRAPCMD_GET_BYTE, { amigamemptr + PSSO_BitMap_Flags } },
		{ TRAPCMD_GET_BYTE, { amigamemptr + PSSO_BitMap_Depth } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 0 } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 4 } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 8 } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 12 } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 16 } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 20 } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 24 } },
		{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_BitMap_Planes + 28} }
	};
	trap_multi(ctx, md, sizeof md / sizeof(struct trapmd));
	bm->BytesPerRow = md[0].params[0];
	bm->Rows = md[1].params[0];
	bm->Flags = md[2].params[0];
	bm->Depth = md[3].params[0];

	/* ARGH - why is THIS happening? */
	if(bm->Depth > 8)
		bm->Depth = 8;

	for (i = 0; i < bm->Depth; i++) {
		uaecptr plane = md[4 + i].params[0];
		bm->APlanes[i] = plane;
		switch (plane) {
		case 0:
			bm->Planes[i] = &all_zeros_bitmap;
			break;
		case 0xFFFFFFFF:
			bm->Planes[i] = &all_ones_bitmap;
			break;
		default:
			if (trap_valid_address(ctx, plane, bm->BytesPerRow * bm->Rows))
				bm->Planes[i] = get_real_address(plane);
			else
				bm->Planes[i] = &all_zeros_bitmap;
			break;
		}
	}
	return 1;
}

static int CopyTemplateStructureA2U(TrapContext* ctx, uaecptr amigamemptr, struct Template* tmpl)
{
	if (trap_valid_address(ctx, amigamemptr, PSSO_Template_sizeof)) {
		struct trapmd md[] =
		{
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_Template_Memory } },
			{ TRAPCMD_GET_WORD, { amigamemptr + PSSO_Template_BytesPerRow } },
			{ TRAPCMD_GET_BYTE, { amigamemptr + PSSO_Template_XOffset } },
			{ TRAPCMD_GET_BYTE, { amigamemptr + PSSO_Template_DrawMode } },
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_Template_FgPen } },
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_Template_BgPen } }
		};
		trap_multi(ctx, md, sizeof md / sizeof(struct trapmd));

		uaecptr memp = md[0].params[0];
		tmpl->Memory = get_real_address(memp);
		tmpl->AMemory = memp;
		tmpl->BytesPerRow = md[1].params[0];
		tmpl->XOffset = md[2].params[0];
		tmpl->DrawMode = md[3].params[0];
		tmpl->FgPen = md[4].params[0];
		tmpl->BgPen = md[5].params[0];
		return 1;
	}
	write_log (_T("ERROR - Invalid Template memory area...\n"));
	return 0;
}

/* list is Amiga address of list, in correct endian format for UAE
* node is Amiga address of node, in correct endian format for UAE */
static void AmigaListAddTail(TrapContext *ctx, uaecptr l, uaecptr n)
{
	trap_put_long(ctx, n + 0, l + 4); // n->ln_Succ = (struct Node *)&l->lh_Tail;
	trap_put_long(ctx, n + 4, trap_get_long(ctx, l + 8)); // n->ln_Pred = l->lh_TailPred;
	trap_put_long(ctx, trap_get_long(ctx, l + 8) + 0, n); // l->lh_TailPred->ln_Succ = n;
	trap_put_long(ctx, l + 8, n); // l->lh_TailPred = n;
}

/*
* Fill a rectangle in the screen.
*/
static void do_fillrect_frame_buffer(struct RenderInfo *ri, int X, int Y, int Width, int Height, uae_u32 Pen, int Bpp)
{
	int cols;
	uae_u8 *dst;
	int bpr = ri->BytesPerRow;

	dst = ri->Memory + X * Bpp + Y * bpr;
	endianswap (&Pen, Bpp);
	switch (Bpp)
	{
	case 1:
		for (int lines = 0; lines < Height; lines++, dst += bpr) {
			memset (dst, Pen, Width);
		}
	break;
	case 2:
	{
		Pen |= Pen << 16;
		for (int lines = 0; lines < Height; lines++, dst += bpr) {
			uae_u32 *p = (uae_u32*)dst;
			for (cols = 0; cols < (Width & ~15); cols += 16) {
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
			}
			while (cols < (Width & ~1)) {
				*p++ = Pen;
				cols += 2;
			}
			if (Width & 1) {
				((uae_u16*)p)[0] = Pen;
			}
		}
	}
	break;
	case 3:
	{
		uae_u16 Pen1 = Pen & 0xffff;
		uae_u16 Pen2 = (Pen << 8) | ((Pen >> 16) & 0xff);
		uae_u16 Pen3 = Pen >> 8;
		bool same = (Pen & 0xff) == ((Pen >> 8) & 0xff) && (Pen & 0xff) == ((Pen >> 16) & 0xff);
		for (int lines = 0; lines < Height; lines++, dst += bpr) {
			uae_u16 *p = (uae_u16*)dst;
			if (same) {
				memset(p, Pen & 0xff, Width * 3);
			} else {
				for (cols = 0; cols < (Width & ~7); cols += 8) {
					*p++ = Pen1;
					*p++ = Pen2;
					*p++ = Pen3;
					*p++ = Pen1;
					*p++ = Pen2;
					*p++ = Pen3;
					*p++ = Pen1;
					*p++ = Pen2;
					*p++ = Pen3;
					*p++ = Pen1;
					*p++ = Pen2;
					*p++ = Pen3;
				}
				uae_u8 *p8 = (uae_u8*)p;
				while (cols < Width) {
					*p8++ = Pen >> 0;
					*p8++ = Pen >> 8;
					*p8++ = Pen >> 16;
					cols++;
				}
			}
		}
	}
	break;
	case 4:
	{
		for (int lines = 0; lines < Height; lines++, dst += bpr) {
			uae_u32 *p = (uae_u32*)dst;
			for (cols = 0; cols < (Width & ~7); cols += 8) {
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
				*p++ = Pen;
			}
			while (cols < Width) {
				*p++ = Pen;
				cols++;
			}
		}
	}
	break;
	}
}

static int p96_framecnt;
static int doskip (void)
{
	if (p96_framecnt >= currprefs.gfx_framerate)
		p96_framecnt = 0;
	return p96_framecnt > 0;
}

static void picasso_trigger_vblank (void)
{
	TrapContext* ctx = nullptr;
	if (!ABI_interrupt || !uaegfx_base || !interrupt_enabled)
		return;
	trap_put_long(ctx, uaegfx_base + CARD_IRQPTR, ABI_interrupt + PSSO_BoardInfo_SoftInterrupt);
	trap_put_byte(ctx, uaegfx_base + CARD_IRQFLAG, 1);
}

static bool is_uaegfx_active(void)
{
	if (!currprefs.rtgboards[0].rtgmem_size)
		return false;
	return true;
}

static bool rtg_render (void)
{
	bool flushed = false;
	bool uaegfx_active = is_uaegfx_active();
	struct picasso96_state_struct* state = &picasso96_state;

	if (uaegfx_active) {
		if (!doskip ())
			flushed = picasso_flushpixels (gfxmem_banks[0]->start + regs.natmem_offset, state->XYOffset - gfxmem_banks[0]->start);
	}

	return flushed;
}

enum
{
	/* DEST = RGBFB_B8G8R8A8,32 */
	RGBFB_A8R8G8B8_32 = 1,
	RGBFB_A8B8G8R8_32,
	RGBFB_R8G8B8A8_32,
	RGBFB_B8G8R8A8_32,
	RGBFB_R8G8B8_32,
	RGBFB_B8G8R8_32,
	RGBFB_R5G6B5PC_32,
	RGBFB_R5G5B5PC_32,
	RGBFB_R5G6B5_32,
	RGBFB_R5G5B5_32,
	RGBFB_B5G6R5PC_32,
	RGBFB_B5G5R5PC_32,
	RGBFB_CLUT_RGBFB_32,
	RGBFB_Y4U2V2_32,
	RGBFB_Y4U1V1_32,

	/* DEST = RGBFB_R5G6B5PC,16 */
	RGBFB_A8R8G8B8_16,
	RGBFB_A8B8G8R8_16,
	RGBFB_R8G8B8A8_16,
	RGBFB_B8G8R8A8_16,
	RGBFB_R8G8B8_16,
	RGBFB_B8G8R8_16,
	RGBFB_R5G6B5PC_16,
	RGBFB_R5G5B5PC_16,
	RGBFB_R5G6B5_16,
	RGBFB_R5G5B5_16,
	RGBFB_B5G6R5PC_16,
	RGBFB_B5G5R5PC_16,
	RGBFB_CLUT_RGBFB_16,
	RGBFB_Y4U2V2_16,
	RGBFB_Y4U1V1_16,

	/* DEST = RGBFB_CLUT,8 */
	RGBFB_CLUT_8
};

static int getconvert (int rgbformat, int pixbytes)
{
	int v = 0;
	int d = pixbytes;

	switch (rgbformat)
	{
	case RGBFB_CLUT:
		if (d == 1)
			v = RGBFB_CLUT_8;
		else if (d == 2)
			v = RGBFB_CLUT_RGBFB_16;
		else if (d == 4)
			v = RGBFB_CLUT_RGBFB_32;
		break;

	case RGBFB_B5G6R5PC:
		if (d == 2)
			v = RGBFB_B5G6R5PC_16;
		else if (d == 4)
			v = RGBFB_B5G6R5PC_32;
		break;
	case RGBFB_R5G6B5PC:
		if (d == 2)
			v = RGBFB_R5G6B5PC_16;
		else if (d == 4)
			v = RGBFB_R5G6B5PC_32;
		break;

	case RGBFB_R5G5B5PC:
		if (d == 4)
			v = RGBFB_R5G5B5PC_32;
		else if (d == 2)
			v = RGBFB_R5G5B5PC_16;
		break;
	case RGBFB_R5G6B5:
		if (d == 4)
			v = RGBFB_R5G6B5_32;
		else
			v = RGBFB_R5G6B5_16;
		break;
	case RGBFB_R5G5B5:
		if (d == 4)
			v = RGBFB_R5G5B5_32;
		else
			v = RGBFB_R5G5B5_16;
		break;
	case RGBFB_B5G5R5PC:
		if (d == 4)
			v = RGBFB_B5G5R5PC_32;
		else
			v = RGBFB_B5G5R5PC_16;
		break;

	case RGBFB_A8R8G8B8:
		if (d == 2)
			v = RGBFB_A8R8G8B8_16;
		else if (d == 4)
			v = RGBFB_A8R8G8B8_32;
		break;
	case RGBFB_R8G8B8:
		if (d == 2)
			v = RGBFB_R8G8B8_16;
		else if (d == 4)
			v = RGBFB_R8G8B8_32;
		break;
	case RGBFB_B8G8R8:
		if (d == 2)
			v = RGBFB_B8G8R8_16;
		else if (d == 4)
			v = RGBFB_B8G8R8_32;
		break;
	case RGBFB_A8B8G8R8:
		if (d == 2)
			v = RGBFB_A8B8G8R8_16;
		else if (d == 4)
			v = RGBFB_A8B8G8R8_32;
		break;
	case RGBFB_B8G8R8A8:
		if (d == 2)
			v = RGBFB_B8G8R8A8_16;
		else if (d == 4)
			v = RGBFB_B8G8R8A8_32;
		break;
	case RGBFB_R8G8B8A8:
		if (d == 2)
			v = RGBFB_R8G8B8A8_16;
		else if (d == 4)
			v = RGBFB_R8G8B8A8_32;
		break;

	case RGBFB_Y4U2V2:
		if (d == 4)
			v = RGBFB_Y4U2V2_32;
		else
			v = RGBFB_Y4U2V2_16;
		break;
	case RGBFB_Y4U1V1:
		if (d == 4)
			v = RGBFB_Y4U1V1_32;
		else
			v = RGBFB_Y4U1V1_16;
		break;

	}
	return v;
}

static void setconvert()
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo;
	struct picasso96_state_struct* state = &picasso96_state;

	vidinfo->picasso_convert = getconvert (state->RGBFormat, picasso_vidinfo.pixbytes);
	vidinfo->host_mode = GetSurfacePixelFormat();
	if (picasso_vidinfo.pixbytes == 4)
		alloc_colors_rgb(8, 8, 8, 16, 8, 0, 0, 0, 0, 0, p96rc, p96gc, p96bc);
	else
		alloc_colors_rgb(5, 6, 5, 11, 5, 0, 0, 0, 0, 0, p96rc, p96gc, p96bc);
	gfx_set_picasso_colors(state->RGBFormat);
	picasso_palette(state->CLUT, vidinfo->clut);
	if (vidinfo->host_mode != vidinfo->ohost_mode || state->RGBFormat != vidinfo->orgbformat) {
		write_log(_T("RTG conversion: Depth=%d HostRGBF=%d P96RGBF=%d Mode=%d\n"),
			picasso_vidinfo.pixbytes, vidinfo->host_mode, state->RGBFormat, vidinfo->picasso_convert);
		vidinfo->ohost_mode = vidinfo->host_mode;
		vidinfo->orgbformat = state->RGBFormat;
	}
}

/* Clear our screen, since we've got a new Picasso screen-mode, and refresh with the proper contents
* This is called on several occasions:
* 1. Amiga-->Picasso transition, via SetSwitch()
* 2. Picasso-->Picasso transition, via SetPanning().
* 3. whenever the graphics code notifies us that the screen contents have been lost.
*/
void picasso_refresh()
{
	struct RenderInfo ri;
	struct amigadisplay* ad = &adisplays;
	struct picasso96_state_struct* state = &picasso96_state;

	if (!ad->picasso_on)
		return;
	setconvert();

	/* Make sure that the first time we show a Picasso video mode, we don't blit any crap.
 * We can do this by checking if we have an Address yet.
 */
	if (state->Address) {
		/* blit the stuff from our static frame-buffer to the gfx-card */
		ri.Memory = gfxmem_bank.baseaddr + (state->Address - gfxmem_bank.start);
		ri.BytesPerRow = state->BytesPerRow;
		ri.RGBFormat = state->RGBFormat;
	}
	else
	{
		write_log(_T("ERROR - picasso_refresh() can't refresh!\n"));
	}
}

static int p96hsync;
bool picasso_rendered = false;
void picasso_handle_vsync(void)
{
	struct amigadisplay* ad = &adisplays;
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo;

	if (currprefs.rtgboards[0].rtgmem_size == 0)
		return;

	if (!ad->picasso_on) {
		picasso_trigger_vblank();
		return;
	}

	int state = vidinfo->picasso_state_change;
	if (state & PICASSO_STATE_SETDAC) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETDAC);
	}
	if (state & PICASSO_STATE_SETGC) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETGC);
		set_gc_called = 1;
		vidinfo->picasso_changed = true;
		init_picasso_screen();
	}
	if (state & PICASSO_STATE_SETSWITCH) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETSWITCH);
		/* Do not switch immediately.  Tell the custom chip emulation about the
	* desired state, and wait for custom.c to call picasso_enablescreen
	* whenever it is ready to change the screen state.  */
		if (ad->picasso_on == ad->picasso_requested_on && ad->picasso_requested_on && vidinfo->picasso_changed) {
			ad->picasso_requested_forced_on = true;
		}
		vidinfo->picasso_changed = false;
		vidinfo->picasso_active = ad->picasso_requested_on;
	}
	if (state & PICASSO_STATE_SETPANNING) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETPANNING);
		vidinfo->set_panning_called = 1;
		init_picasso_screen();
		vidinfo->set_panning_called = 0;
	}
	if (state & PICASSO_STATE_SETDISPLAY) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETDISPLAY);
		// do nothing
	}

	p96_framecnt++;

	if (!ad->picasso_on)
		return;

	picasso_rendered = rtg_render();

	picasso_trigger_vblank();
}

#define BLT_SIZE 4
#define BLT_MULT 1
#define BLT_NAME BLIT_FALSE_32
#define BLT_FUNC(s,d) *d = 0
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_32
#define BLT_FUNC(s,d) *d = ~(*s | * d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_32
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_32
#define BLT_FUNC(s,d) *d = ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_32
#define BLT_FUNC(s,d) *d = (*s) & ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_32
#define BLT_FUNC(s,d) *d = ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_EOR_32
#define BLT_FUNC(s,d) *d = (*s) ^ (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NAND_32
#define BLT_FUNC(s,d) *d = ~((*s) & (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_AND_32
#define BLT_FUNC(s,d) *d = (*s) & (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NEOR_32
#define BLT_FUNC(s,d) *d = ~((*s) ^ (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYSRC_32
#define BLT_FUNC(s,d) *d = ~(*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYDST_32
#define BLT_FUNC(s,d) *d = ~(*d) | (*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_OR_32
#define BLT_FUNC(s,d) *d = (*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_TRUE_32
#ifdef AMIBERRY
#define BLT_FUNC(s,d) memset(d, 0xff, sizeof (*d))
#else
#define BLT_FUNC(s,d) *d = 0xffffffff
#endif
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SWAP_32
#define BLT_FUNC(s,d) tmp = *d ; *d = *s; *s = tmp;
#define BLT_TEMP
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define BLT_SIZE 3
#define BLT_MULT 1
#define BLT_NAME BLIT_FALSE_24
#define BLT_FUNC(s,d) *d = 0
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_24
#define BLT_FUNC(s,d) *d = ~(*s | * d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_24
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_24
#define BLT_FUNC(s,d) *d = ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_24
#define BLT_FUNC(s,d) *d = (*s) & ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_24
#define BLT_FUNC(s,d) *d = ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_EOR_24
#define BLT_FUNC(s,d) *d = (*s) ^ (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NAND_24
#define BLT_FUNC(s,d) *d = ~((*s) & (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_AND_24
#define BLT_FUNC(s,d) *d = (*s) & (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NEOR_24
#define BLT_FUNC(s,d) *d = ~((*s) ^ (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYSRC_24
#define BLT_FUNC(s,d) *d = ~(*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYDST_24
#define BLT_FUNC(s,d) *d = ~(*d) | (*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_OR_24
#define BLT_FUNC(s,d) *d = (*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_TRUE_24
#ifdef AMIBERRY
#define BLT_FUNC(s,d) memset(d, 0xff, sizeof (*d))
#else
#define BLT_FUNC(s,d) *d = 0xffffffff
#endif
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SWAP_24
#define BLT_FUNC(s,d) tmp = *d ; *d = *s; *s = tmp;
#define BLT_TEMP
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define BLT_SIZE 2
#define BLT_MULT 2
#define BLT_NAME BLIT_FALSE_16
#define BLT_FUNC(s,d) *d = 0
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_16
#define BLT_FUNC(s,d) *d = ~(*s | * d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_16
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_16
#define BLT_FUNC(s,d) *d = ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_16
#define BLT_FUNC(s,d) *d = (*s) & ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_16
#define BLT_FUNC(s,d) *d = ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_EOR_16
#define BLT_FUNC(s,d) *d = (*s) ^ (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NAND_16
#define BLT_FUNC(s,d) *d = ~((*s) & (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_AND_16
#define BLT_FUNC(s,d) *d = (*s) & (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NEOR_16
#define BLT_FUNC(s,d) *d = ~((*s) ^ (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYSRC_16
#define BLT_FUNC(s,d) *d = ~(*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYDST_16
#define BLT_FUNC(s,d) *d = ~(*d) | (*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_OR_16
#define BLT_FUNC(s,d) *d = (*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_TRUE_16
#ifdef AMIBERRY
#define BLT_FUNC(s,d) memset(d, 0xff, sizeof (*d))
#else
#define BLT_FUNC(s,d) *d = 0xffffffff
#endif
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SWAP_16
#define BLT_FUNC(s,d) tmp = *d ; *d = *s; *s = tmp;
#define BLT_TEMP
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define BLT_SIZE 1
#define BLT_MULT 4
#define BLT_NAME BLIT_FALSE_8
#define BLT_FUNC(s,d) *d = 0
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_8
#define BLT_FUNC(s,d) *d = ~(*s | * d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_8
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_8
#define BLT_FUNC(s,d) *d = ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_8
#define BLT_FUNC(s,d) *d = (*s) & ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_8
#define BLT_FUNC(s,d) *d = ~(*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_EOR_8
#define BLT_FUNC(s,d) *d = (*s) ^ (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NAND_8
#define BLT_FUNC(s,d) *d = ~((*s) & (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_AND_8
#define BLT_FUNC(s,d) *d = (*s) & (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NEOR_8
#define BLT_FUNC(s,d) *d = ~((*s) ^ (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYSRC_8
#define BLT_FUNC(s,d) *d = ~(*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYDST_8
#define BLT_FUNC(s,d) *d = ~(*d) | (*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_OR_8
#define BLT_FUNC(s,d) *d = (*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_TRUE_8
#ifdef AMIBERRY
#define BLT_FUNC(s,d) memset(d, 0xff, sizeof (*d))
#else
#define BLT_FUNC(s,d) *d = 0xffffffff
#endif
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SWAP_8
#define BLT_FUNC(s,d) tmp = *d ; *d = *s; *s = tmp;
#define BLT_TEMP
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define PARMS width, height, src, dst, ri->BytesPerRow, dstri->BytesPerRow

/*
* Functions to perform an action on the frame-buffer
*/
static int do_blitrect_frame_buffer(struct RenderInfo* ri, struct
                                    RenderInfo* dstri, unsigned long srcx, unsigned long srcy,
                                    unsigned long dstx, unsigned long dsty, unsigned long width, unsigned
                                    long height, uae_u8 mask, BLIT_OPCODE opcode)
{
	uae_u8 *src, *dst;
	uae_u8 Bpp = GetBytesPerPixel(ri->RGBFormat);
	unsigned long total_width = width * Bpp;

	src = ri->Memory + srcx * Bpp + srcy * ri->BytesPerRow;
	dst = dstri->Memory + dstx * Bpp + dsty * dstri->BytesPerRow;

	if (mask != 0xFF && Bpp > 1)
	{
		write_log (_T("WARNING - BlitRect() has mask 0x%x with Bpp %d.\n"), mask, Bpp);
	}

	if (mask == 0xFF || Bpp > 1)
	{
		if (opcode == BLIT_SRC)
		{
			/* handle normal case efficiently */
			if (ri->Memory == dstri->Memory && dsty == srcy)
			{
				unsigned long i;
				for (i = 0; i < height; i++, src += ri->BytesPerRow, dst += dstri->BytesPerRow)
					memmove(dst, src, total_width);
			}
			else if (dsty < srcy)
			{
				unsigned long i;
				for (i = 0; i < height; i++, src += ri->BytesPerRow, dst += dstri->BytesPerRow)
					memcpy(dst, src, total_width);
			}
			else
			{
				unsigned long i;
				src += (height - 1) * ri->BytesPerRow;
				dst += (height - 1) * dstri->BytesPerRow;
				for (i = 0; i < height; i++, src -= ri->BytesPerRow, dst -= dstri->BytesPerRow)
					memcpy (dst, src, total_width);
			}
			return 1;

		} else {

			if (Bpp == 4) {

				/* 32-bit optimized */
				switch (opcode)
				{
				case BLIT_FALSE: BLIT_FALSE_32 (PARMS); break;
				case BLIT_NOR: BLIT_NOR_32 (PARMS); break;
				case BLIT_ONLYDST: BLIT_ONLYDST_32 (PARMS); break;
				case BLIT_NOTSRC: BLIT_NOTSRC_32 (PARMS); break;
				case BLIT_ONLYSRC: BLIT_ONLYSRC_32 (PARMS); break;
				case BLIT_NOTDST: BLIT_NOTDST_32 (PARMS); break;
				case BLIT_EOR: BLIT_EOR_32 (PARMS); break;
				case BLIT_NAND: BLIT_NAND_32 (PARMS); break;
				case BLIT_AND: BLIT_AND_32 (PARMS); break;
				case BLIT_NEOR: BLIT_NEOR_32 (PARMS); break;
				case BLIT_NOTONLYSRC: BLIT_NOTONLYSRC_32 (PARMS); break;
				case BLIT_NOTONLYDST: BLIT_NOTONLYDST_32 (PARMS); break;
				case BLIT_OR: BLIT_OR_32 (PARMS); break;
				case BLIT_TRUE: BLIT_TRUE_32 (PARMS); break;
				case BLIT_SWAP: BLIT_SWAP_32 (PARMS); break;
				}
			} else if (Bpp == 3) {

				/* 24-bit (not very) optimized */
				switch (opcode)
				{
				case BLIT_FALSE: BLIT_FALSE_24 (PARMS); break;
				case BLIT_NOR: BLIT_NOR_24 (PARMS); break;
				case BLIT_ONLYDST: BLIT_ONLYDST_24 (PARMS); break;
				case BLIT_NOTSRC: BLIT_NOTSRC_24 (PARMS); break;
				case BLIT_ONLYSRC: BLIT_ONLYSRC_24 (PARMS); break;
				case BLIT_NOTDST: BLIT_NOTDST_24 (PARMS); break;
				case BLIT_EOR: BLIT_EOR_24 (PARMS); break;
				case BLIT_NAND: BLIT_NAND_24 (PARMS); break;
				case BLIT_AND: BLIT_AND_24 (PARMS); break;
				case BLIT_NEOR: BLIT_NEOR_24 (PARMS); break;
				case BLIT_NOTONLYSRC: BLIT_NOTONLYSRC_24 (PARMS); break;
				case BLIT_NOTONLYDST: BLIT_NOTONLYDST_24 (PARMS); break;
				case BLIT_OR: BLIT_OR_24 (PARMS); break;
				case BLIT_TRUE: BLIT_TRUE_24 (PARMS); break;
				case BLIT_SWAP: BLIT_SWAP_24 (PARMS); break;
				}

			} else if (Bpp == 2) {

				/* 16-bit optimized */
				switch (opcode)
				{
				case BLIT_FALSE: BLIT_FALSE_16 (PARMS); break;
				case BLIT_NOR: BLIT_NOR_16 (PARMS); break;
				case BLIT_ONLYDST: BLIT_ONLYDST_16 (PARMS); break;
				case BLIT_NOTSRC: BLIT_NOTSRC_16 (PARMS); break;
				case BLIT_ONLYSRC: BLIT_ONLYSRC_16 (PARMS); break;
				case BLIT_NOTDST: BLIT_NOTDST_16 (PARMS); break;
				case BLIT_EOR: BLIT_EOR_16 (PARMS); break;
				case BLIT_NAND: BLIT_NAND_16 (PARMS); break;
				case BLIT_AND: BLIT_AND_16 (PARMS); break;
				case BLIT_NEOR: BLIT_NEOR_16 (PARMS); break;
				case BLIT_NOTONLYSRC: BLIT_NOTONLYSRC_16 (PARMS); break;
				case BLIT_NOTONLYDST: BLIT_NOTONLYDST_16 (PARMS); break;
				case BLIT_OR: BLIT_OR_16 (PARMS); break;
				case BLIT_TRUE: BLIT_TRUE_16 (PARMS); break;
				case BLIT_SWAP: BLIT_SWAP_16 (PARMS); break;
				}

			} else if (Bpp == 1) {

				/* 8-bit optimized */
				switch (opcode)
				{
				case BLIT_FALSE: BLIT_FALSE_8 (PARMS); break;
				case BLIT_NOR: BLIT_NOR_8 (PARMS); break;
				case BLIT_ONLYDST: BLIT_ONLYDST_8 (PARMS); break;
				case BLIT_NOTSRC: BLIT_NOTSRC_8 (PARMS); break;
				case BLIT_ONLYSRC: BLIT_ONLYSRC_8 (PARMS); break;
				case BLIT_NOTDST: BLIT_NOTDST_8 (PARMS); break;
				case BLIT_EOR: BLIT_EOR_8 (PARMS); break;
				case BLIT_NAND: BLIT_NAND_8 (PARMS); break;
				case BLIT_AND: BLIT_AND_8 (PARMS); break;
				case BLIT_NEOR: BLIT_NEOR_8 (PARMS); break;
				case BLIT_NOTONLYSRC: BLIT_NOTONLYSRC_8 (PARMS); break;
				case BLIT_NOTONLYDST: BLIT_NOTONLYDST_8 (PARMS); break;
				case BLIT_OR: BLIT_OR_8 (PARMS); break;
				case BLIT_TRUE: BLIT_TRUE_8 (PARMS); break;
				case BLIT_SWAP: BLIT_SWAP_8 (PARMS); break;
				}
			}
		}
		return 1;
	}
	return 0;
}

/*
SetSpritePosition:
Synopsis: SetSpritePosition(bi, RGBFormat);
Inputs: a0: struct BoardInfo *bi
d7: RGBFTYPE RGBFormat

*/
static uae_u32 REGPARAM2 picasso_SetSpritePosition(TrapContext* ctx)
{
	return 0;
}


/*
SetSpriteColor:
Synopsis: SetSpriteColor(bi, index, red, green, blue, RGBFormat);
Inputs: a0: struct BoardInfo *bi
d0.b: index
d1.b: red
d2.b: green
d3.b: blue
d7: RGBFTYPE RGBFormat

This function changes one of the possible three colors of the hardware sprite.
*/
static uae_u32 REGPARAM2 picasso_SetSpriteColor(TrapContext* ctx)
{
		return 0;
}

/*
SetSpriteImage:
Synopsis: SetSpriteImage(bi, RGBFormat);
Inputs: a0: struct BoardInfo *bi
d7: RGBFTYPE RGBFormat

This function gets new sprite image data from the MouseImage field of the BoardInfo structure and writes
it to the board.

There are three possible cases:

BIB_HIRESSPRITE is set:
skip the first two long words and the following sprite data is arranged as an array of two longwords. Those form the
two bit planes for one image line respectively.

BIB_HIRESSPRITE and BIB_BIGSPRITE are not set:
skip the first two words and the following sprite data is arranged as an array of two words. Those form the two
bit planes for one image line respectively.

BIB_HIRESSPRITE is not set and BIB_BIGSPRITE is set:
skip the first two words and the following sprite data is arranged as an array of two words. Those form the two bit
planes for one image line respectively. You have to double each pixel horizontally and vertically. All coordinates
used in this case already assume a zoomed sprite, only the sprite data is not zoomed yet. You will have to
compensate for this when accounting for hotspot offsets and sprite dimensions.
*/
static uae_u32 REGPARAM2 picasso_SetSpriteImage(TrapContext *ctx)
{
  return 0;
}

/*
SetSprite:
Synopsis: SetSprite(bi, activate, RGBFormat);
Inputs: a0: struct BoardInfo *bi
d0: BOOL activate
d7: RGBFTYPE RGBFormat

This function activates or deactivates the hardware sprite.
*/
static uae_u32 REGPARAM2 picasso_SetSprite (TrapContext *ctx)
{
		return 0;
}

/*
* BOOL FindCard(struct BoardInfo *bi);      and
*
* FindCard is called in the first stage of the board initialisation and
* configuration and is used to look if there is a free and unconfigured
* board of the type the driver is capable of managing. If it finds one,
* it immediately reserves it for use by Picasso96, usually by clearing
* the CDB_CONFIGME bit in the flags field of the ConfigDev struct of
* this expansion card. But this is only a common example, a driver can
* do whatever it wants to mark this card as used by the driver. This
* mechanism is intended to ensure that a board is only configured and
* used by one driver. FindBoard also usually fills some fields of the
* BoardInfo struct supplied by the caller, the rtg.library, for example
* the MemoryBase, MemorySize and RegisterBase fields.
*/
static void picasso96_alloc2 (TrapContext *ctx);
static uae_u32 REGPARAM2 picasso_FindCard (TrapContext *ctx)
{
	uaecptr AmigaBoardInfo = trap_get_areg(ctx, 0);
	struct picasso96_state_struct *state = &picasso96_state;
	/* NOTES: See BoardInfo struct definition in Picasso96 dev info */
	if (!uaegfx_active || !gfxmem_bank.start)
		return 0;
	if (uaegfx_base) {
		trap_put_long(ctx, uaegfx_base + CARD_BOARDINFO, AmigaBoardInfo);
	} else if (uaegfx_old) {
		picasso96_alloc2 (ctx);
	}
	boardinfo = AmigaBoardInfo;
	if (gfxmem_bank.allocated_size && !state->CardFound) {
		/* Fill in MemoryBase, MemorySize */
		trap_put_long(ctx, AmigaBoardInfo + PSSO_BoardInfo_MemoryBase, gfxmem_bank.start);
		trap_put_long(ctx, AmigaBoardInfo + PSSO_BoardInfo_MemorySize, gfxmem_bank.allocated_size - reserved_gfxmem);
		state->CardFound = 1; /* mark our "card" as being found */
		return -1;
	} else
		return 0;
}

static void FillBoardInfo(TrapContext *ctx, uaecptr amigamemptr, struct LibResolution *res, int width, int height, int depth)
{
	switch (depth)
	{
	case 8:
		res->Modes[CHUNKY] = amigamemptr;
		break;
	case 15:
	case 16:
		res->Modes[HICOLOR] = amigamemptr;
		break;
	case 24:
		res->Modes[TRUECOLOR] = amigamemptr;
		break;
	default:
		res->Modes[TRUEALPHA] = amigamemptr;
		break;
	}
	trap_set_bytes(ctx, amigamemptr, 0, PSSO_ModeInfo_sizeof);

	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_Width, width);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_Height, height);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_Depth, depth);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_Flags, 0);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorTotal, width);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorBlankSize, 1);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorSyncStart, 0);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorSyncSize, 1);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_HorSyncSkew, 0);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_HorEnableSkew, 0);

	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerTotal, height);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerBlankSize, 1);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerSyncStart, 0);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerSyncSize, 1);

	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_first_union, 98);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_second_union, 14);

	trap_put_long(ctx, amigamemptr + PSSO_ModeInfo_PixelClock,
		width * height * defaultHz);
}

struct modeids {
	int width, height;
	int id;
};
static struct modeids mi[] =
{
	/* "original" modes */

	320, 200, 0,
	320, 240, 1,
	640, 400, 2,
	640, 480, 3,
	800, 600, 4,
	1024, 768, 5,
	1152, 864, 6,
	1280,1024, 7,
	1600,1280, 8,
	320, 256, 9,
	640, 512,10,

	/* new modes */

	704, 480, 129,
	704, 576, 130,
	720, 480, 131,
	720, 576, 132,
	768, 483, 133,
	768, 576, 134,
	800, 480, 135,
	848, 480, 136,
	854, 480, 137,
	948, 576, 138,
	1024, 576, 139,
	1152, 768, 140,
	1152, 864, 141,
	1280, 720, 142,
	1280, 768, 143,
	1280, 800, 144,
	1280, 854, 145,
	1280, 960, 146,
	1366, 768, 147,
	1440, 900, 148,
	1440, 960, 149,
	1600,1200, 150,
	1680,1050, 151,
	1920,1080, 152,
	1920,1200, 153,
	2048,1152, 154,
	2048,1536, 155,
	2560,1600, 156,
	2560,2048, 157,
	400, 300, 158,
	512, 384, 159,
	640, 432, 160,
	1360, 768, 161,
	1360,1024, 162,
	1400,1050, 163,
	1792,1344, 164,
	1800,1440, 165,
	1856,1392, 166,
	1920,1440, 167,
	480, 360, 168,
	640, 350, 169,
	1600, 900, 170,
	960, 600, 171,
	1088, 612, 172,
	1152, 648, 173,
	1776,1000, 174,
	2560,1440, 175,
	-1,-1,0
};

static int AssignModeID (int w, int h, int *unkcnt)
{
	int i;

	for (i = 0; mi[i].width > 0; i++) {
		if (w == mi[i].width && h == mi[i].height)
			return 0x50001000 | (mi[i].id * 0x10000);
	}
	(*unkcnt)++;
	write_log (_T("P96: Non-unique mode %dx%d"), w, h);
	if (i > 0 && 256 - (*unkcnt) == mi[i - 1].id + 1) {
		(*unkcnt) = 256 - 127;
		write_log(_T(" (Skipped reserved)"));
	} else if (256 - (*unkcnt) == 11) {
		(*unkcnt) = 511;
		write_log(_T(" (Using extra)"));
	}
	write_log(_T("\n"));
	return 0x51001000 - (*unkcnt) * 0x10000;
}

static uaecptr picasso96_amem, picasso96_amemend;


static void CopyLibResolutionStructureU2A(TrapContext *ctx, struct LibResolution *libres, uaecptr amigamemptr)
{
	int i;

	trap_set_bytes(ctx, amigamemptr, 0, PSSO_LibResolution_sizeof);
	for (i = 0; i < strlen (libres->P96ID); i++)
		trap_put_byte(ctx, amigamemptr + PSSO_LibResolution_P96ID + i, libres->P96ID[i]);
	trap_put_long(ctx, amigamemptr + PSSO_LibResolution_DisplayID, libres->DisplayID);
	trap_put_word(ctx, amigamemptr + PSSO_LibResolution_Width, libres->Width);
	trap_put_word(ctx, amigamemptr + PSSO_LibResolution_Height, libres->Height);
	trap_put_word(ctx, amigamemptr + PSSO_LibResolution_Flags, libres->Flags);
	for (i = 0; i < MAXMODES; i++)
		trap_put_long(ctx, amigamemptr + PSSO_LibResolution_Modes + i * 4, libres->Modes[i]);
	trap_put_long(ctx, amigamemptr + 10, amigamemptr + PSSO_LibResolution_P96ID);
	trap_put_long(ctx, amigamemptr + PSSO_LibResolution_BoardInfo, libres->BoardInfo);
}

static void init_alloc (TrapContext *ctx, int size)
{
	picasso96_amem = picasso96_amemend = 0;
	if (uaegfx_base) {
		int size = trap_get_long(ctx, uaegfx_base + CARD_RESLISTSIZE);
		picasso96_amem = trap_get_long(ctx, uaegfx_base + CARD_RESLIST);
	} else if (uaegfx_active) {
		reserved_gfxmem = size;
		picasso96_amem = gfxmem_bank.start + gfxmem_bank.allocated_size - size;
	}
	picasso96_amemend = picasso96_amem + size;
	write_log (_T("P96 RESINFO: %08X-%08X (%d,%d)\n"), picasso96_amem, picasso96_amemend, size / PSSO_ModeInfo_sizeof, size);
}

static int p96depth (int depth)
{
	uae_u32 f = currprefs.picasso96_modeflags;
	int ok = 0;

	if (depth == 8 && (f & RGBFF_CLUT))
		ok = 1;
	if (depth == 15 && (f & (RGBFF_R5G5B5PC | RGBFF_R5G5B5 | RGBFF_B5G5R5PC)))
		ok = 2;
	if (depth == 16 && (f & (RGBFF_R5G6B5PC | RGBFF_R5G6B5 | RGBFF_B5G6R5PC)))
		ok = 2;
	if (depth == 24 && (f & (RGBFF_R8G8B8 | RGBFF_B8G8R8)))
		ok = 3;
	if (depth == 32 && (f & (RGBFF_A8R8G8B8 | RGBFF_A8B8G8R8 | RGBFF_R8G8B8A8 | RGBFF_B8G8R8A8)))
		ok = 4;
	return ok;
}

static int resolution_compare (const void *a, const void *b)
{
	struct PicassoResolution *ma = (struct PicassoResolution *)a;
	struct PicassoResolution *mb = (struct PicassoResolution *)b;
	if (ma->res.width < mb->res.width)
		return -1;
	if (ma->res.width > mb->res.width)
		return 1;
	if (ma->res.height < mb->res.height)
		return -1;
	if (ma->res.height > mb->res.height)
		return 1;
	return ma->depth - mb->depth;
}

static int missmodes[] = { 320, 200, 320, 240, 320, 256, 640, 400, 640, 480, 640, 512, 800, 600, 1024, 768, 1280, 1024, -1 };

static uaecptr uaegfx_card_install (TrapContext *ctx, uae_u32 size);

static void picasso96_alloc2 (TrapContext *ctx)
{
	int i, j, size, cnt;
	int misscnt, depths;

	xfree (newmodes);
	newmodes = NULL;
	picasso96_amem = picasso96_amemend = 0;
	if (gfxmem_bank.allocated_size == 0)
		return;
	misscnt = 0;
	newmodes = xmalloc (struct PicassoResolution, MAX_PICASSO_MODES);
	size = 0;

	depths = 0;
	if (p96depth (8))
		depths++;
	if (p96depth (15))
		depths++;
	if (p96depth (16))
		depths++;
	if (p96depth (24))
		depths++;
	if (p96depth (32))
		depths++;

	i = 0;
	while (DisplayModes[i].depth >= 0) {
		for (j = 0; missmodes[j * 2] >= 0; j++) {
			if (DisplayModes[i].res.width == missmodes[j * 2 + 0] && DisplayModes[i].res.height == missmodes[j * 2 + 1]) {
				missmodes[j * 2 + 0] = 0;
				missmodes[j * 2 + 1] = 0;
			}
		}
		i++;
	}

	cnt = 0;
	i = 0;
	while (DisplayModes[i].depth >= 0) {
		// Not even 256 color mode fits in VRAM? Ignore it completely.
		if (DisplayModes[i].res.width * DisplayModes[i].res.height > gfxmem_bank.allocated_size - 256) {
			i++;
			continue;
		}
		j = i;
		size += PSSO_LibResolution_sizeof;
		while (missmodes[misscnt * 2] == 0)
			misscnt++;
		if (missmodes[misscnt * 2] >= 0) {
			int w = DisplayModes[i].res.width;
			int h = DisplayModes[i].res.height;
			if (w > missmodes[misscnt * 2 + 0] || (w == missmodes[misscnt * 2 + 0] && h > missmodes[misscnt * 2 + 1])) {	
				struct PicassoResolution *pr = &newmodes[cnt];
				memcpy (pr, &DisplayModes[i], sizeof (struct PicassoResolution));
				pr->res.width = missmodes[misscnt * 2 + 0];
				pr->res.height = missmodes[misscnt * 2 + 1];
				_stprintf (pr->name, _T("%dx%d FAKE"), pr->res.width, pr->res.height);
				size += PSSO_ModeInfo_sizeof * depths;
				cnt++;
				misscnt++;
				continue;
			}
		}
			memcpy (&newmodes[cnt], &DisplayModes[i], sizeof (struct PicassoResolution));
			size += PSSO_ModeInfo_sizeof * depths;
			cnt++;
		i++;
	}
	qsort (newmodes, cnt, sizeof (struct PicassoResolution), resolution_compare);

	newmodes[cnt].depth = -1;

	for (i = 0; i < cnt; i++) {
		int depth;
		for (depth = 1; depth <= 4; depth++) {
			switch (depth) {
			case 1:
				if (newmodes[i].res.width > chunky.width)
					chunky.width = newmodes[i].res.width;
				if (newmodes[i].res.height > chunky.height)
					chunky.height = newmodes[i].res.height;
				break;
			case 2:
				if (newmodes[i].res.width > hicolour.width)
					hicolour.width = newmodes[i].res.width;
				if (newmodes[i].res.height > hicolour.height)
					hicolour.height = newmodes[i].res.height;
				break;
			case 3:
				if (newmodes[i].res.width > truecolour.width)
					truecolour.width = newmodes[i].res.width;
				if (newmodes[i].res.height > truecolour.height)
					truecolour.height = newmodes[i].res.height;
				break;
			case 4:
				if (newmodes[i].res.width > alphacolour.width)
					alphacolour.width = newmodes[i].res.width;
				if (newmodes[i].res.height > alphacolour.height)
					alphacolour.height = newmodes[i].res.height;
				break;
			}
		}
	}
	uaegfx_card_install (ctx, size);
	init_alloc (ctx, size);
}

void picasso96_alloc (TrapContext *ctx)
{
	uaegfx_resname = ds (_T("uaegfx.card"));
	if (uaegfx_old)
		return;
	picasso96_alloc2 (ctx);
}

static void inituaegfxfuncs (TrapContext *ctx, uaecptr start, uaecptr ABI);
static void inituaegfx(TrapContext *ctx, uaecptr ABI)
{
	uae_u32 flags;

	write_log (_T("RTG mode mask: %x BI=%08x\n"), currprefs.picasso96_modeflags, ABI);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_BitsPerCannon, 8);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_RGBFormats, currprefs.picasso96_modeflags);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_BoardType, picasso96_BT);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_GraphicsControllerType, picasso96_GCT);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_PaletteChipType, picasso96_PCT);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_BoardName, uaegfx_resname);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_BoardType, 1);

	trap_put_long(ctx, ABI + PSSO_BoardInfo_MemoryClock, 100000000);

	/* only 1 clock */
	trap_put_long(ctx, ABI + PSSO_BoardInfo_PixelClockCount + PLANAR * 4, 1);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_PixelClockCount + CHUNKY * 4, 1);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_PixelClockCount + HICOLOR * 4, 1);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_PixelClockCount + TRUECOLOR * 4, 1);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_PixelClockCount + TRUEALPHA * 4, 1);

	/* we have 16 bits for horizontal and vertical timings - hack */
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorValue + PLANAR * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorValue + CHUNKY * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorValue + HICOLOR * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorValue + TRUECOLOR * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorValue + TRUEALPHA * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerValue + PLANAR * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerValue + CHUNKY * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerValue + HICOLOR * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerValue + TRUECOLOR * 2, 0x4000);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerValue + TRUEALPHA * 2, 0x4000);

	flags = trap_get_long(ctx, ABI + PSSO_BoardInfo_Flags);
	flags &= 0xffff0000;
	if (flags & BIF_NOBLITTER)
		write_log (_T("P96: Blitter disabled in devs:monitors/uaegfx!\n"));
	if (NOBLITTER_ALL) {
		flags |= BIF_NOBLITTER;
		flags &= ~BIF_BLITTER;
	} else {
		flags |= BIF_BLITTER;
	}
	flags |= BIF_NOMEMORYMODEMIX;
	if (!uaegfx_old)
		flags |= BIF_VBLANKINTERRUPT;
	if (!(flags & BIF_INDISPLAYCHAIN)) {
		write_log (_T("P96: BIF_INDISPLAYCHAIN force-enabled!\n"));
		flags |= BIF_INDISPLAYCHAIN;
	}
	trap_put_long(ctx, ABI + PSSO_BoardInfo_Flags, flags);

	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorResolution + 0, planar.width);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorResolution + 2, chunky.width);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorResolution + 4, hicolour.width);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorResolution + 6, truecolour.width);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxHorResolution + 8, alphacolour.width);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerResolution + 0, planar.height);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerResolution + 2, chunky.height);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerResolution + 4, hicolour.height);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerResolution + 6, truecolour.height);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_MaxVerResolution + 8, alphacolour.height);

	inituaegfxfuncs(ctx, uaegfx_rom, ABI);
}

static bool addmode(TrapContext *ctx, uaecptr AmigaBoardInfo, uaecptr *amem, struct LibResolution *res, int w, int h, const TCHAR *name, int display, int *unkcnt)
{
	int depth;
	bool added = false;

	if (display > 0) {
		res->DisplayID = 0x51000000 + display * 0x100000;
	} else {
		res->DisplayID = AssignModeID(w, h, unkcnt);
	}
	res->BoardInfo = AmigaBoardInfo;
	res->Width = w;
	res->Height = h;
	res->Flags = P96F_PUBLIC;
	memcpy(res->P96ID, "P96-0:", 6);
	if (name) {
		char* n2 = ua(name);
		strcpy (res->Name, n2);
		xfree (n2);
	} else {
		sprintf(res->Name, "UAE:%4dx%4d", w, h);
	}

	for (depth = 8; depth <= 32; depth++) {
		if (!p96depth(depth))
			continue;
		if(gfxmem_bank.allocated_size >= w * h * ((depth + 7) / 8)) {
			FillBoardInfo(ctx, *amem, res, w, h, depth);
			*amem += PSSO_ModeInfo_sizeof;
			added = true;
		}
	}
	return added;
}

/****************************************
* InitCard()
*
* a2: BoardInfo structure ptr - Amiga-based address in Intel endian-format
*
*/
static uae_u32 REGPARAM2 picasso_InitCard(TrapContext* ctx)
{
	int LibResolutionStructureCount = 0;
	int i, unkcnt, cnt;
	uaecptr amem;
	uaecptr AmigaBoardInfo = trap_get_areg(ctx, 0);

	if (!picasso96_amem) {
		write_log (_T("P96: InitCard() but no resolution memory!\n"));
		return 0;
	}
	amem = picasso96_amem;

	inituaegfx(ctx, AmigaBoardInfo);

	i = 0;
	unkcnt = cnt = 0;
	while (newmodes[i].depth >= 0) {
		struct LibResolution res = {0};
		int j = i;
		if (addmode(ctx, AmigaBoardInfo, &amem, &res, newmodes[i].res.width, newmodes[i].res.height, NULL, 0, &unkcnt)) {
			TCHAR* s;
			s = au(res.Name);
			write_log (_T("%2d: %08X %4dx%4d %s\n"), ++cnt, res.DisplayID, res.Width, res.Height, s);
			xfree (s);
			while (newmodes[i].depth >= 0
				&& newmodes[i].res.width == newmodes[j].res.width
				&& newmodes[i].res.height == newmodes[j].res.height)
				i++;

			LibResolutionStructureCount++;
			CopyLibResolutionStructureU2A(ctx, &res, amem);
			AmigaListAddTail(ctx, AmigaBoardInfo + PSSO_BoardInfo_ResolutionsList, amem);
			amem += PSSO_LibResolution_sizeof;
		}
		else
		{
			write_log (_T("--: %08X %4dx%4d Not enough VRAM\n"), res.DisplayID, res.Width, res.Height);
			i++;
		}
	}

	if (amem > picasso96_amemend)
		write_log (_T("P96: display resolution list corruption %08x<>%08x (%d)\n"), amem, picasso96_amemend, i);

	return -1;
}

/*
* SetSwitch:
* a0: struct BoardInfo
* d0.w: BOOL state
* this function should set a board switch to let the Amiga signal pass
* through when supplied with a 0 in d0 and to show the board signal if
* a 1 is passed in d0. You should remember the current state of the
* switch to avoid unneeded switching. If your board has no switch, then
* simply supply a function that does nothing except a RTS.
*
* NOTE: Return the opposite of the switch-state. BDK
*/
static uae_u32 REGPARAM2 picasso_SetSwitch(TrapContext* ctx)
{
	struct picasso96_state_struct *state = &picasso96_state;
	struct amigadisplay *ad = &adisplays;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u16 flag = trap_get_dreg(ctx, 0) & 0xFFFF;

	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETSWITCH);
	ad->picasso_requested_on = flag != 0;
	set_config_changed();

	TCHAR p96text[100];
	p96text[0] = 0;
	if (flag)
		_stprintf(p96text, _T("Picasso96 %dx%dx%d (%dx%dx%d)"),
			state->Width, state->Height, state->BytesPerPixel * 8,
			vidinfo->width, vidinfo->height, vidinfo->pixbytes * 8);
	write_log (_T("SetSwitch() - %s.\n"), flag ? p96text : _T("amiga"));

	/* Put old switch-state in D0 */
	return !flag;
}


void picasso_enablescreen(int on)
{
	bool uaegfx = currprefs.rtgboards[0].rtgmem_size;
	bool uaegfx_active = is_uaegfx_active();

	if (uaegfx_active && uaegfx) {
		if (!init_picasso_screen_called)
			init_picasso_screen();
	}
	setconvert();
	picasso_refresh();
}

static void resetpalette(struct picasso96_state_struct *state)
{
	for (int i = 0; i < 256; i++)
		state->CLUT[i].Pad = 0xff;
}

/*
* SetColorArray:
* a0: struct BoardInfo
* d0.w: startindex
* d1.w: count
* when this function is called, your driver has to fetch "count" color
* values starting at "startindex" from the CLUT field of the BoardInfo
* structure and write them to the hardware. The color values are always
* between 0 and 255 for each component regardless of the number of bits
* per cannon your board has. So you might have to shift the colors
* before writing them to the hardware.
*/
static int updateclut(TrapContext* ctx, uaecptr clut, int start, int count)
{
	struct picasso96_state_struct *state = &picasso96_state;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u8 clutbuf[256 * 3];
	int i, changed = 0;
	clut += start * 3;
	trap_get_bytes(ctx, clutbuf + start * 3, clut, count * 3);
	for (i = start; i < start + count; i++) {
		int r = clutbuf[i * 3 + 0];
		int g = clutbuf[i * 3 + 1];
		int b = clutbuf[i * 3 + 2];
		changed |= state->CLUT[i].Red != r
			|| state->CLUT[i].Green != g
			|| state->CLUT[i].Blue != b;
		if (state->CLUT[i].Pad) {
			changed = 1;
			state->CLUT[i].Pad = 0;
		}
		state->CLUT[i].Red = r;
		state->CLUT[i].Green = g;
		state->CLUT[i].Blue = b;
}

	changed |= picasso_palette(state->CLUT, vidinfo->clut);
	return changed;
}
static uae_u32 REGPARAM2 picasso_SetColorArray(TrapContext* ctx)
{
	/* Fill in some static UAE related structure about this new CLUT setting
	* We need this for CLUT-based displays, and for mapping CLUT to hi/true colour */
	uae_u16 start = trap_get_dreg(ctx, 0);
	uae_u16 count = trap_get_dreg(ctx, 1);
	uaecptr boardinfo = trap_get_areg(ctx, 0);
	uaecptr clut = boardinfo + PSSO_BoardInfo_CLUT;
	if (start > 256 || count > 256 || start + count > 256)
		return 0;
  updateclut(ctx, clut, start, count);
	return 1;
}

/*
* SetDAC:
* a0: struct BoardInfo
* d7: RGBFTYPE RGBFormat
* This function is called whenever the RGB format of the display changes,
* e.g. from chunky to TrueColor. Usually, all you have to do is to set
* the RAMDAC of your board accordingly.
*/
static uae_u32 REGPARAM2 picasso_SetDAC(TrapContext* ctx)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo;
	/* Fill in some static UAE related structure about this new DAC setting
	* Lets us keep track of what pixel format the Amiga is thinking about in our frame-buffer */

	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETDAC);
	return 1;
}

static void init_picasso_screen()
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	struct picasso96_state_struct *state = &picasso96_state;
	if(vidinfo->set_panning_called) {
		state->Extent = state->Address + state->BytesPerRow * state->VirtualHeight;
	}
	if (set_gc_called) {
		gfx_set_picasso_modeinfo(state->Width, state->Height, state->GC_Depth, state->RGBFormat);
		set_gc_called = 0;
	}
	if((vidinfo->width == state->Width) &&
		(vidinfo->height == state->Height) &&
		(vidinfo->depth == (state->GC_Depth >> 3)) &&
		(vidinfo->selected_rgbformat == state->RGBFormat))
	{
		picasso_refresh();
	}
	init_picasso_screen_called = 1;
}

/*
* SetGC:
* a0: struct BoardInfo
* a1: struct ModeInfo
* d0: BOOL Border
* This function is called whenever another ModeInfo has to be set. This
* function simply sets up the CRTC and TS registers to generate the
* timing used for that screen mode. You should not set the DAC, clocks
* or linear start address. They will be set when appropriate by their
* own functions.
*/
static uae_u32 REGPARAM2 picasso_SetGC(TrapContext* ctx)
{
	struct picasso96_state_struct *state = &picasso96_state;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	/* Fill in some static UAE related structure about this new ModeInfo setting */
	uaecptr AmigaBoardInfo = trap_get_areg(ctx, 0);
	uae_u32 border = trap_get_dreg(ctx, 0);
	uaecptr modeinfo = trap_get_areg(ctx, 1);

	trap_put_long(ctx, AmigaBoardInfo + PSSO_BoardInfo_ModeInfo, modeinfo);
	trap_put_word(ctx, AmigaBoardInfo + PSSO_BoardInfo_Border, border);

	state->Width = trap_get_word(ctx, modeinfo + PSSO_ModeInfo_Width);
	state->VirtualWidth = state->Width; /* in case SetPanning doesn't get called */

	state->Height = trap_get_word(ctx, modeinfo + PSSO_ModeInfo_Height);
	state->VirtualHeight = state->Height; /* in case SetPanning doesn't get called */

	state->GC_Depth = trap_get_byte(ctx, modeinfo + PSSO_ModeInfo_Depth);
	state->GC_Flags = trap_get_byte(ctx, modeinfo + PSSO_ModeInfo_Flags);

	state->HostAddress = NULL;

	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETGC);
	return 1;
}

/*
* SetPanning:
* a0: struct BoardInfo
* a1: UBYTE *Memory
* d0: uae_u16 Width
* d1: WORD XOffset
* d2: WORD YOffset
* d7: RGBFTYPE RGBFormat
* This function sets the view origin of a display which might also be
* overscanned. In register a1 you get the start address of the screen
* bitmap on the Amiga side. You will have to subtract the starting
* address of the board memory from that value to get the memory start
* offset within the board. Then you get the offset in pixels of the
* left upper edge of the visible part of an overscanned display. From
* these values you will have to calculate the LinearStartingAddress
* fields of the CRTC registers.

* NOTE: SetPanning() can be used to know when a Picasso96 screen is
* being opened.  Better to do the appropriate clearing of the
* background here than in SetSwitch() derived functions,
* because SetSwitch() is not called for subsequent Picasso screens.
*/

static void picasso_SetPanningInit (struct picasso96_state_struct *state)
{
	state->XYOffset = state->Address + (state->XOffset * state->BytesPerPixel)
		+ (state->YOffset * state->BytesPerRow);
}

static uae_u32 REGPARAM2 picasso_SetPanning(TrapContext* ctx)
{
	struct picasso96_state_struct *state = &picasso96_state;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u16 Width = trap_get_dreg(ctx, 0);
	uaecptr start_of_screen = trap_get_areg(ctx, 1);
	uaecptr bi = trap_get_areg(ctx, 0);
	uaecptr bmeptr = trap_get_long(ctx, bi + PSSO_BoardInfo_BitMapExtra);  /* Get our BoardInfo ptr's BitMapExtra ptr */
	uae_u16 bme_width, bme_height;
	RGBFTYPE rgbf;

	if (oldscr == 0) {
		oldscr = start_of_screen;
	}
	if (oldscr != start_of_screen) {
		oldscr = start_of_screen;
	}

	bme_width = trap_get_word(ctx, bmeptr + PSSO_BitMapExtra_Width);
	bme_height = trap_get_word(ctx, bmeptr + PSSO_BitMapExtra_Height);
	rgbf = state->RGBFormat;

	state->Address = start_of_screen; /* Amiga-side address */
	state->XOffset = (uae_s16)(trap_get_dreg(ctx, 1) & 0xFFFF);
	state->YOffset = (uae_s16)(trap_get_dreg(ctx, 2) & 0xFFFF);
	trap_put_word(ctx, bi + PSSO_BoardInfo_XOffset, state->XOffset);
	trap_put_word(ctx, bi + PSSO_BoardInfo_YOffset, state->YOffset);
	state->VirtualWidth = bme_width;
	state->VirtualHeight = bme_height;
	state->RGBFormat = (RGBFTYPE)trap_get_dreg(ctx, 7);
	state->BytesPerPixel = GetBytesPerPixel (state->RGBFormat);
	state->BytesPerRow = state->VirtualWidth * state->BytesPerPixel;
	picasso_SetPanningInit(state);

	if (rgbf != state->RGBFormat) {
		setconvert();
	}

	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETPANNING);

	return 1;
}

static void do_xor8 (uae_u8 *p, int w, uae_u32 v)
{
	while (ALIGN_POINTER_TO32 (p) != 3 && w) {
		*p ^= v;
		p++;
		w--;
	}
	while (w >= 2 * 4) {
		*((uae_u32*)p) ^= v;
		p += 4;
		*((uae_u32*)p) ^= v;
		p += 4;
		w -= 2 * 4;
	}
	while (w) {
		*p ^= v;
		p++;
		w--;
	}
}

/*
* InvertRect:
*
* Inputs:
* a0:struct BoardInfo *bi
* a1:struct RenderInfo *ri
* d0.w:X
* d1.w:Y
* d2.w:Width
* d3.w:Height
* d4.l:Mask
* d7.l:RGBFormat
*
* This function is used to invert a rectangular area on the board. It is called by BltBitMap,
* BltPattern and BltTemplate.
*/
static uae_u32 REGPARAM2 picasso_InvertRect (TrapContext *ctx)
{
	uaecptr renderinfo = trap_get_areg(ctx, 1);
	uae_u32 X = (uae_u16)trap_get_dreg(ctx, 0);
	uae_u32 Y = (uae_u16)trap_get_dreg(ctx, 1);
	uae_u32 Width = (uae_u16)trap_get_dreg(ctx, 2);
	uae_u32 Height = (uae_u16)trap_get_dreg(ctx, 3);
	uae_u8 mask = (uae_u8)trap_get_dreg(ctx, 4);
	int Bpp = GetBytesPerPixel (trap_get_dreg(ctx, 7));
	uae_u32 xorval;
	unsigned int lines;
	struct RenderInfo ri;
	uae_u8 *uae_mem, *rectstart;
	unsigned long width_in_bytes;
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;

	if (CopyRenderInfoStructureA2U(ctx, renderinfo, &ri)) {
		if (!validatecoords(ctx, &ri, &X, &Y, &Width, &Height))
			return 1;

		if (mask != 0xFF && Bpp > 1)
			mask = 0xFF;

		xorval = 0x01010101 * (mask & 0xFF);
		width_in_bytes = Bpp * Width;
		rectstart = uae_mem = ri.Memory + Y * ri.BytesPerRow + X * Bpp;

		for (lines = 0; lines < Height; lines++, uae_mem += ri.BytesPerRow)
			do_xor8 (uae_mem, width_in_bytes, xorval);
		result = 1;
	}

	return result; /* 1 if supported, 0 otherwise */
}

/***********************************************************
FillRect:
***********************************************************
* a0: struct BoardInfo *
* a1: struct RenderInfo *
* d0: WORD X
* d1: WORD Y
* d2: WORD Width
* d3: WORD Height
* d4: uae_u32 Pen
* d5: UBYTE Mask
* d7: uae_u32 RGBFormat
***********************************************************/
static uae_u32 REGPARAM2 picasso_FillRect(TrapContext* ctx)
{
	uaecptr renderinfo = trap_get_areg(ctx, 1);
	uae_u32 X = (uae_u16)trap_get_dreg(ctx, 0);
	uae_u32 Y = (uae_u16)trap_get_dreg(ctx, 1);
	uae_u32 Width = (uae_u16)trap_get_dreg(ctx, 2);
	uae_u32 Height = (uae_u16)trap_get_dreg(ctx, 3);
	uae_u32 Pen = trap_get_dreg(ctx, 4);
	uae_u8 Mask = (uae_u8)trap_get_dreg(ctx, 5);
	RGBFTYPE RGBFormat = (RGBFTYPE)trap_get_dreg(ctx, 7);
	uae_u8* oldstart;
	int Bpp;
	struct RenderInfo ri;
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;
  if (CopyRenderInfoStructureA2U (ctx, renderinfo, &ri) && Y != 0xFFFF) {
		if (!validatecoords(ctx, &ri, &X, &Y, &Width, &Height))
			return 1;

		Bpp = GetBytesPerPixel(RGBFormat);

		if (Bpp > 1)
			Mask = 0xFF;

		if (Mask == 0xFF) {

			/* Do the fill-rect in the frame-buffer */
			do_fillrect_frame_buffer(&ri, X, Y, Width, Height, Pen, Bpp);
			result = 1;

		} else {

			/* We get here only if Mask != 0xFF */
			if (Bpp != 1) {
				write_log (_T("WARNING - FillRect() has unhandled mask 0x%x with Bpp %d. Using fall-back routine.\n"), Mask, Bpp);
			} else {
				Pen &= Mask;
				Mask = ~Mask;
				oldstart = ri.Memory + Y * ri.BytesPerRow + X * Bpp;
				{
					uae_u8 *start = oldstart;
					uae_u8 *end = start + Height * ri.BytesPerRow;
					for (; start != end; start += ri.BytesPerRow) {
						uae_u8 *p = start;
						unsigned long cols;
						for (cols = 0; cols < Width; cols++) {
							uae_u32 tmpval = do_get_mem_byte (p + cols) & Mask;
							do_put_mem_byte (p + cols, (uae_u8)(Pen | tmpval));
						}
					}
				}
				result = 1;
			}
		}
	}
	return result;
}

/*
* BlitRect() is a generic (any chunky pixel format) rectangle copier
* NOTE: If dstri is NULL, then we're only dealing with one RenderInfo area, and called from picasso_BlitRect()
*
* OpCodes:
* 0 = FALSE:	dst = 0
* 1 = NOR:	dst = ~(src | dst)
* 2 = ONLYDST:	dst = dst & ~src
* 3 = NOTSRC:	dst = ~src
* 4 = ONLYSRC:	dst = src & ~dst
* 5 = NOTDST:	dst = ~dst
* 6 = EOR:	dst = src^dst
* 7 = NAND:	dst = ~(src & dst)
* 8 = AND:	dst = (src & dst)
* 9 = NEOR:	dst = ~(src ^ dst)
*10 = DST:	dst = dst
*11 = NOTONLYSRC: dst = ~src | dst
*12 = SRC:	dst = src
*13 = NOTONLYDST: dst = ~dst | src
*14 = OR:	dst = src | dst
*15 = TRUE:	dst = 0xFF
*/
struct blitdata
{
	struct RenderInfo ri_struct;
	struct RenderInfo dstri_struct;
	struct RenderInfo *ri; /* Self-referencing pointers */
	struct RenderInfo *dstri;
	unsigned long srcx;
	unsigned long srcy;
	unsigned long dstx;
	unsigned long dsty;
	unsigned long width;
	unsigned long height;
	uae_u8 mask;
	BLIT_OPCODE opcode;
} blitrectdata;

STATIC_INLINE int BlitRectHelper(TrapContext *ctx)
{
	struct RenderInfo *ri = blitrectdata.ri;
	struct RenderInfo *dstri = blitrectdata.dstri;
	uae_u32 srcx = blitrectdata.srcx;
	uae_u32 srcy = blitrectdata.srcy;
	uae_u32 dstx = blitrectdata.dstx;
	uae_u32 dsty = blitrectdata.dsty;
	uae_u32 width = blitrectdata.width;
	uae_u32 height = blitrectdata.height;
	uae_u8 mask = blitrectdata.mask;
	BLIT_OPCODE opcode = blitrectdata.opcode;

	if (!validatecoords(ctx, ri, &srcx, &srcy, &width, &height))
		return 1;
	if (!validatecoords(ctx, dstri, &dstx, &dsty, &width, &height))
		return 1;

	uae_u8 Bpp = GetBytesPerPixel (ri->RGBFormat);

	if (opcode == BLIT_DST) {
		write_log ( _T("WARNING: BlitRect() being called with opcode of BLIT_DST\n") );
		return 1;
	}

	/*
	* If we have no destination RenderInfo, then we're dealing with a single-buffer action, called
	* from picasso_BlitRect().  The code in do_blitrect_frame_buffer() deals with the frame-buffer,
	* while the do_blit() code deals with the visible screen.
	*
	* If we have a destination RenderInfo, then we've been called from picasso_BlitRectNoMaskComplete()
	* and we need to put the results on the screen from the frame-buffer.
	*/
	if (dstri == NULL || dstri->Memory == ri->Memory) {
		if (mask != 0xFF && Bpp > 1)
			mask = 0xFF;
		dstri = ri;
	}
	/* Do our virtual frame-buffer memory first */
	return do_blitrect_frame_buffer(ri, dstri, srcx, srcy, dstx, dsty, width, height, mask, opcode);
}

static int BlitRect(TrapContext *ctx, uaecptr ri, uaecptr dstri,
	unsigned long srcx, unsigned long srcy, unsigned long dstx, unsigned long dsty,
	unsigned long width, unsigned long height, uae_u8 mask, BLIT_OPCODE opcode)
{
	/* Set up the params */
	CopyRenderInfoStructureA2U(ctx, ri, &blitrectdata.ri_struct);
	blitrectdata.ri = &blitrectdata.ri_struct;
	if(dstri) {
		CopyRenderInfoStructureA2U(ctx, dstri, &blitrectdata.dstri_struct);
		blitrectdata.dstri = &blitrectdata.dstri_struct;
	} else {
		blitrectdata.dstri = NULL;
	}
	blitrectdata.srcx = srcx;
	blitrectdata.srcy = srcy;
	blitrectdata.dstx = dstx;
	blitrectdata.dsty = dsty;
	blitrectdata.width = width;
	blitrectdata.height = height;
	blitrectdata.mask = mask;
	blitrectdata.opcode = opcode;

	return BlitRectHelper(ctx);
}

/***********************************************************
BlitRect:
***********************************************************
* a0:	struct BoardInfo
* a1:	struct RenderInfo
* d0:	WORD SrcX
* d1:	WORD SrcY
* d2:	WORD DstX
* d3:	WORD DstY
* d4:	WORD Width
* d5:	WORD Height
* d6:	UBYTE Mask
* d7:	uae_u32 RGBFormat
***********************************************************/
static uae_u32 REGPARAM2 picasso_BlitRect (TrapContext *ctx)
{
	uaecptr renderinfo = trap_get_areg(ctx, 1);
	unsigned long srcx = (uae_u16)trap_get_dreg(ctx, 0);
	unsigned long srcy = (uae_u16)trap_get_dreg(ctx, 1);
	unsigned long dstx = (uae_u16)trap_get_dreg(ctx, 2);
	unsigned long dsty = (uae_u16)trap_get_dreg(ctx, 3);
	unsigned long width = (uae_u16)trap_get_dreg(ctx, 4);
	unsigned long height = (uae_u16)trap_get_dreg(ctx, 5);
	uae_u8 Mask = (uae_u8)trap_get_dreg(ctx, 6);
	uae_u32 result = 0;

	if (NOBLITTER_BLIT)
		return 0;
	result = BlitRect(ctx, renderinfo, (uaecptr)NULL, srcx, srcy, dstx, dsty, width, height, Mask, BLIT_SRC);
	return result;
}

/***********************************************************
BlitRectNoMaskComplete:
***********************************************************
* a0:	struct BoardInfo
* a1:	struct RenderInfo (src)
* a2:	struct RenderInfo (dst)
* d0:	WORD SrcX
* d1:	WORD SrcY
* d2:	WORD DstX
* d3:	WORD DstY
* d4:	WORD Width
* d5:	WORD Height
* d6:	UBYTE OpCode
* d7:	uae_u32 RGBFormat
* NOTE:	MUST return 0 in D0 if we're not handling this operation
*	because the RGBFormat or opcode aren't supported.
*	OTHERWISE return 1
***********************************************************/
static uae_u32 REGPARAM2 picasso_BlitRectNoMaskComplete (TrapContext *ctx)
{
	uaecptr srcri = trap_get_areg(ctx, 1);
	uaecptr dstri = trap_get_areg(ctx, 2);
	unsigned long srcx = (uae_u16)trap_get_dreg(ctx, 0);
	unsigned long srcy = (uae_u16)trap_get_dreg(ctx, 1);
	unsigned long dstx = (uae_u16)trap_get_dreg(ctx, 2);
	unsigned long dsty = (uae_u16)trap_get_dreg(ctx, 3);
	unsigned long width = (uae_u16)trap_get_dreg(ctx, 4);
	unsigned long height = (uae_u16)trap_get_dreg(ctx, 5);
	BLIT_OPCODE OpCode = (BLIT_OPCODE)(trap_get_dreg(ctx, 6) & 0xff);
	uae_u32 RGBFmt = trap_get_dreg(ctx, 7);
	uae_u32 result = 0;

	if (NOBLITTER_BLIT)
		return 0;

	result = BlitRect(ctx, srcri, dstri, srcx, srcy, dstx, dsty, width, height, 0xFF, OpCode);
	return result;
}

/* NOTE: fgpen MUST be in host byte order */
STATIC_INLINE void PixelWrite(uae_u8 *mem, int bits, uae_u32 fgpen, int Bpp, uae_u32 mask)
{
	switch (Bpp)
	{
	case 1:
		if (mask != 0xFF)
			fgpen = (fgpen & mask) | (mem[bits] & ~mask);
		mem[bits] = (uae_u8)fgpen;
		break;
	case 2:
		((uae_u16 *)mem)[bits] = (uae_u16)fgpen;
		break;
	case 3:
		mem[bits * 3 + 0] = fgpen >> 0;
		mem[bits * 3 + 1] = fgpen >> 8;
		mem[bits * 3 + 2] = fgpen >> 16;
		break;
	case 4:
		((uae_u32 *)mem)[bits] = fgpen;
		break;
	}
}

/*
* BlitPattern:
*
* Synopsis:BlitPattern(bi, ri, pattern, X, Y, Width, Height, Mask, RGBFormat);
* Inputs:
* a0:struct BoardInfo *bi
* a1:struct RenderInfo *ri
* a2:struct Pattern *pattern
* d0.w:X
* d1.w:Y
* d2.w:Width
* d3.w:Height
* d4.w:Mask
* d7.l:RGBFormat
*
* This function is used to paint a pattern on the board memory using the blitter. It is called by
* BltPattern, if a AreaPtrn is used with positive AreaPtSz. The pattern consists of a b/w image
* using a single plane of image data which will be expanded repeatedly to the destination RGBFormat
* using ForeGround and BackGround pens as well as draw modes. The width of the pattern data is
* always 16 pixels (one word) and the height is calculated as 2^Size. The data must be shifted up
* and to the left by XOffset and YOffset pixels at the beginning.
*/
static uae_u32 REGPARAM2 picasso_BlitPattern (TrapContext *ctx)
{
	uaecptr rinf = trap_get_areg(ctx, 1);
	uaecptr pinf = trap_get_areg(ctx, 2);
	uae_u32 X = (uae_u16)trap_get_dreg(ctx, 0);
	uae_u32 Y = (uae_u16)trap_get_dreg(ctx, 1);
	uae_u32 W = (uae_u16)trap_get_dreg(ctx, 2);
	uae_u32 H = (uae_u16)trap_get_dreg(ctx, 3);
	uae_u8 Mask = (uae_u8)trap_get_dreg(ctx, 4);
	uae_u32 RGBFmt = trap_get_dreg(ctx, 7);
	uae_u8 Bpp = GetBytesPerPixel (RGBFmt);
	int inversion = 0;
	struct RenderInfo ri;
	struct Pattern pattern;
	unsigned long rows;
	uae_u8 *uae_mem;
	int xshift;
	unsigned long ysize_mask;
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;

	if(CopyRenderInfoStructureA2U(ctx, rinf, &ri) && CopyPatternStructureA2U(ctx, pinf, &pattern)) {
		if (!validatecoords(ctx, &ri, &X, &Y, &W, &H))
			return 0;

		Bpp = GetBytesPerPixel(ri.RGBFormat);
		uae_mem = ri.Memory + Y * ri.BytesPerRow + X * Bpp; /* offset with address */

		if (pattern.DrawMode & INVERS)
			inversion = 1;

		pattern.DrawMode &= 0x03;
		if (Mask != 0xFF) {
			if(Bpp > 1)
				Mask = 0xFF;
			result = 1;
		} else {
			result = 1;
		}

		if (pattern.Size >= 16)
			result = 0;

  	if (result) {
			uae_u32 fgpen, bgpen;

			ysize_mask = (1 << pattern.Size) - 1;
			xshift = pattern.XOffset & 15;

			fgpen = pattern.FgPen;
			endianswap(&fgpen, Bpp);
			bgpen = pattern.BgPen;
			endianswap(&bgpen, Bpp);

			for (rows = 0; rows < H; rows++, uae_mem += ri.BytesPerRow) {
				unsigned long prow = (rows + pattern.YOffset) & ysize_mask;
				unsigned int d;
				uae_u8* uae_mem2 = uae_mem;
				unsigned long cols;

				d = do_get_mem_word(((uae_u16 *)pattern.Memory) + prow);

				if (xshift != 0)
					d = (d << xshift) | (d >> (16 - xshift));

				for (cols = 0; cols < W; cols += 16, uae_mem2 += Bpp * 16) {
					long bits;
					long max = W - cols;
					unsigned int data = d;

					if (max > 16)
						max = 16;

					switch (pattern.DrawMode)
					{
					case JAM1:
						{
							for (bits = 0; bits < max; bits++) {
								int bit_set = data & 0x8000;
								data <<= 1;
								if (inversion)
									bit_set = !bit_set;
								if (bit_set)
									PixelWrite (uae_mem2, bits, fgpen, Bpp, Mask);
							}
							break;
						}
					case JAM2:
						{
							for (bits = 0; bits < max; bits++) {
								int bit_set = data & 0x8000;
								data <<= 1;
								if (inversion)
									bit_set = !bit_set;
								PixelWrite (uae_mem2, bits, bit_set ? fgpen : bgpen, Bpp, Mask);
							}
							break;
						}
					case COMP:
						{
							for (bits = 0; bits < max; bits++) {
								int bit_set = data & 0x8000;
								data <<= 1;
								if (bit_set) {
									switch (Bpp)
									{
									case 1:
										{
											uae_mem2[bits] ^= 0xff & Mask;
										}
										break;
									case 2:
										{
											uae_u16 *addr = (uae_u16 *)uae_mem2;
											addr[bits] ^= 0xffff;
										}
										break;
									case 3:
										{
											uae_u32 *addr = (uae_u32 *)(uae_mem2 + bits * 3);
											do_put_mem_long (addr, do_get_mem_long (addr) ^ 0x00ffffff);
										}
										break;
									case 4:
										{
											uae_u32 *addr = (uae_u32 *)uae_mem2;
											addr[bits] ^= 0xffffffff;
										}
										break;
									}
								}
							}
							break;
						}
					}
				}
			}
			result = 1;
		}
	}

	return result;
}

/*************************************************
BlitTemplate:
**************************************************
* Synopsis: BlitTemplate(bi, ri, template, X, Y, Width, Height, Mask, RGBFormat);
* a0: struct BoardInfo *bi
* a1: struct RenderInfo *ri
* a2: struct Template *template
* d0.w: X
* d1.w: Y
* d2.w: Width
* d3.w: Height
* d4.w: Mask
* d7.l: RGBFormat
*
* This function is used to paint a template on the board memory using the blitter.
* It is called by BltPattern and BltTemplate. The template consists of a b/w image
* using a single plane of image data which will be expanded to the destination RGBFormat
* using ForeGround and BackGround pens as well as draw modes.
***********************************************************************************/
static uae_u32 REGPARAM2 picasso_BlitTemplate(TrapContext* ctx)
{
	uae_u8 inversion = 0;
	uaecptr rinf = trap_get_areg(ctx, 1);
	uaecptr tmpl = trap_get_areg(ctx, 2);
	uae_u32 X = (uae_u16)trap_get_dreg(ctx, 0);
	uae_u32 Y = (uae_u16)trap_get_dreg(ctx, 1);
	uae_u32 W = (uae_u16)trap_get_dreg(ctx, 2);
	uae_u32 H = (uae_u16)trap_get_dreg(ctx, 3);
	uae_u16 Mask = (uae_u16)trap_get_dreg(ctx, 4);
	struct Template tmp;
	struct RenderInfo ri;
	unsigned long rows;
	int bitoffset;
	uae_u8 *uae_mem, Bpp;
	uae_u8 *tmpl_base;
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;
	if (CopyRenderInfoStructureA2U(ctx, rinf, &ri) && CopyTemplateStructureA2U(ctx, tmpl, &tmp)) {
		if (!validatecoords(ctx, &ri, &X, &Y, &W, &H))
			return 1;

		Bpp = GetBytesPerPixel (ri.RGBFormat);
		uae_mem = ri.Memory + Y * ri.BytesPerRow + X * Bpp; /* offset into address */

		if (tmp.DrawMode & INVERS)
			inversion = 1;

		tmp.DrawMode &= 0x03;

		if (Mask != 0xFF) {
			if(Bpp > 1)
				Mask = 0xFF;
			if(tmp.DrawMode == COMP) {
				write_log (_T("WARNING - BlitTemplate() has unhandled mask 0x%x with COMP DrawMode. Using fall-back routine.\n"), Mask);
				return 0;
			} else {
				result = 1;
			}
		} else {
			result = 1;
		}

		if(result) {
			uae_u32 fgpen, bgpen;

			bitoffset = tmp.XOffset % 8;

			fgpen = tmp.FgPen;
			endianswap(&fgpen, Bpp);
			bgpen = tmp.BgPen;
			endianswap(&bgpen, Bpp);

			tmpl_base = tmp.Memory + tmp.XOffset / 8;

			for (rows = 0; rows < H; rows++, uae_mem += ri.BytesPerRow, tmpl_base += tmp.BytesPerRow) {
				unsigned long cols;
				uae_u8 *uae_mem2 = uae_mem;
				uae_u8 *tmpl_mem = tmpl_base;
				unsigned int data;
				
				data = *tmpl_mem;

				for (cols = 0; cols < W; cols += 8, uae_mem2 += Bpp * 8) {
					unsigned int byte;
					long bits;
					long max = W - cols;

					if (max > 8)
						max = 8;

					data <<= 8;
					data |= *++tmpl_mem;

					byte = data >> (8 - bitoffset);

					switch (tmp.DrawMode)
					{
					case JAM1:
						{
							for (bits = 0; bits < max; bits++) {
								int bit_set = (byte & 0x80);
								byte <<= 1;
								if (inversion)
									bit_set = !bit_set;
								if (bit_set)
									PixelWrite(uae_mem2, bits, fgpen, Bpp, Mask);
							}
							break;
						}
					case JAM2:
						{
							for (bits = 0; bits < max; bits++) {
								int bit_set = (byte & 0x80);
								byte <<= 1;
								if (inversion)
									bit_set = !bit_set;
								PixelWrite(uae_mem2, bits, bit_set ? fgpen : bgpen, Bpp, Mask);
							}
							break;
						}
					case COMP:
						{
							for (bits = 0; bits < max; bits++) {
								int bit_set = (byte & 0x80);
								byte <<= 1;
								if (bit_set) {
									switch (Bpp)
									{
									case 1:
										{
											uae_u8 *addr = uae_mem2;
											addr[bits] ^= 0xff;
										}
										break;
									case 2:
										{
											uae_u16 *addr = (uae_u16 *)uae_mem2;
											addr[bits] ^= 0xffff;
										}
										break;
									case 3:
										{
											uae_u32 *addr = (uae_u32 *)(uae_mem2 + bits * 3);
											do_put_mem_long (addr, do_get_mem_long (addr) ^ 0x00FFFFFF);
										}
										break;
									case 4:
										{
											uae_u32 *addr = (uae_u32 *)uae_mem2;
											addr[bits] ^= 0xffffffff;
										}
										break;
									}
								}
							}
							break;
						}
					}
				}
			}
			result = 1;
		}
	}

	return 1;
}

/*
* CalculateBytesPerRow:
* a0: 	struct BoardInfo
* d0: 	uae_u16 Width
* d7:	RGBFTYPE RGBFormat
* This function calculates the amount of bytes needed for a line of
* "Width" pixels in the given RGBFormat.
*/
static uae_u32 REGPARAM2 picasso_CalculateBytesPerRow (TrapContext *ctx)
{
	uae_u16 width = trap_get_dreg(ctx, 0);
	uae_u32 type = trap_get_dreg(ctx, 7);
	int bpp = GetBytesPerPixel(type);
	width = bpp * width;
	return width;
}

/*
* SetDisplay:
* a0: struct BoardInfo
* d0: BOOL state
* This function enables and disables the video display.
*
* NOTE: return the opposite of the state
*/
static uae_u32 REGPARAM2 picasso_SetDisplay(TrapContext* ctx)
{
	struct picasso96_state_struct *state = &picasso96_state;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u32 setstate = trap_get_dreg(ctx, 0);
	resetpalette(state);
	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETDISPLAY);
	return !setstate;
}

void init_hz_p96(int monid)
{
	if (isvsync_rtg()) {
		float rate = target_getcurrentvblankrate();
		if (rate < 0)
			p96vblank = vblank_hz;
		else
			p96vblank = target_getcurrentvblankrate();
	}
	//else if (currprefs.win32_rtgvblankrate == 0) {
	//	p96vblank = vblank_hz;
	//}
	//else {
	//	p96vblank = currprefs.win32_rtgvblankrate;
	//}
	if (p96vblank <= 0)
		p96vblank = 60;
	if (p96vblank >= 300)
		p96vblank = 300;
	p96syncrate = maxvpos_nom * vblank_hz / p96vblank;
	write_log(_T("RTGFREQ: %d*%.4f = %.4f / %.1f = %d\n"), maxvpos_nom, vblank_hz, maxvpos_nom * vblank_hz, p96vblank, p96syncrate);
}

/* NOTE: Watch for those planeptrs of 0x00000000 and 0xFFFFFFFF for all zero / all one bitmaps !!!! */
static void PlanarToChunky(TrapContext* ctx, struct RenderInfo* ri, struct BitMap* bm,
                           unsigned long srcx, unsigned long srcy,
                           unsigned long dstx, unsigned long dsty,
                           unsigned long width, unsigned long height,
                           uae_u8 mask)
{
	int j;

	uae_u8 *PLANAR[8];
	uae_u8 *image = ri->Memory + dstx * GetBytesPerPixel(ri->RGBFormat) + dsty * ri->BytesPerRow;
	int Depth = bm->Depth;
	unsigned long rows, bitoffset = srcx & 7;
	long eol_offset;

	/* Set up our bm->Planes[] pointers to the right horizontal offset */
	for (j = 0; j < Depth; j++) {
		uae_u8 *p = bm->Planes[j];
		if (p != &all_zeros_bitmap && p != &all_ones_bitmap)
			p += srcx / 8 + srcy * bm->BytesPerRow;
		PLANAR[j] = p;
		if ((mask & (1 << j)) == 0)
			PLANAR[j] = &all_zeros_bitmap;
	}
	eol_offset = (long)bm->BytesPerRow - (long)((width + 7) >> 3);
	for (rows = 0; rows < height; rows++, image += ri->BytesPerRow) {
		unsigned long cols;

		for (cols = 0; cols < width; cols += 8) {
			int k;
			uae_u32 a = 0, b = 0;
			unsigned int msk = 0xFF;
			long tmp = cols + 8 - width;
			if (tmp > 0) {
				msk <<= tmp;
				b = do_get_mem_long ((uae_u32 *)(image + cols + 4));
				if (tmp < 4)
					b &= 0xFFFFFFFF >> (32 - tmp * 8);
				else if (tmp > 4) {
					a = do_get_mem_long ((uae_u32 *)(image + cols));
					a &= 0xFFFFFFFF >> (64 - tmp * 8);
				}
			}
			for (k = 0; k < Depth; k++) {
				unsigned int data;
				if (PLANAR[k] == &all_zeros_bitmap)
					data = 0;
				else if (PLANAR[k] == &all_ones_bitmap)
					data = 0xFF;
				else {
					data = (uae_u8)(do_get_mem_word((uae_u16 *)PLANAR[k]) >> (8 - bitoffset));
					PLANAR[k]++;
				}
				data &= msk;
				a |= p2ctab[data][0] << k;
				b |= p2ctab[data][1] << k;
			}
			do_put_mem_long((uae_u32 *)(image + cols), a);
			do_put_mem_long((uae_u32 *)(image + cols + 4), b);
		}
		for (j = 0; j < Depth; j++) {
			if (PLANAR[j] != &all_zeros_bitmap && PLANAR[j] != &all_ones_bitmap) {
				PLANAR[j] += eol_offset;
			}
	}
}
}

/*
* BlitPlanar2Chunky:
* a0: struct BoardInfo *bi
* a1: struct BitMap *bm - source containing planar information and assorted details
* a2: struct RenderInfo *ri - dest area and its details
* d0.w: SrcX
* d1.w: SrcY
* d2.w: DstX
* d3.w: DstY
* d4.w: SizeX
* d5.w: SizeY
* d6.b: MinTerm - uh oh!
* d7.b: Mask - uh oh!
*
* This function is currently used to blit from planar bitmaps within system memory to chunky bitmaps
* on the board. Watch out for plane pointers that are 0x00000000 (represents a plane with all bits "0")
* or 0xffffffff (represents a plane with all bits "1").
*/
static uae_u32 REGPARAM2 picasso_BlitPlanar2Chunky(TrapContext* ctx)
{
	uaecptr bm = trap_get_areg(ctx, 1);
	uaecptr ri = trap_get_areg(ctx, 2);
	unsigned long srcx = (uae_u16)trap_get_dreg(ctx, 0);
	unsigned long srcy = (uae_u16)trap_get_dreg(ctx, 1);
	unsigned long dstx = (uae_u16)trap_get_dreg(ctx, 2);
	unsigned long dsty = (uae_u16)trap_get_dreg(ctx, 3);
	unsigned long width = (uae_u16)trap_get_dreg(ctx, 4);
	unsigned long height = (uae_u16)trap_get_dreg(ctx, 5);
	uae_u8 minterm = trap_get_dreg(ctx, 6) & 0xFF;
	uae_u8 mask = trap_get_dreg(ctx, 7) & 0xFF;
	struct RenderInfo local_ri;
	struct BitMap local_bm;
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;
	if (minterm != 0x0C) {
		write_log (_T("ERROR - BlitPlanar2Chunky() has minterm 0x%x, which I don't handle. Using fall-back routine.\n"),
			minterm);
	}
	else if (CopyRenderInfoStructureA2U(ctx, ri, &local_ri) && CopyBitMapStructureA2U(ctx, bm, &local_bm))
	{
		PlanarToChunky(ctx, &local_ri, &local_bm, srcx, srcy, dstx, dsty, width, height, mask);
		result = 1;
	}
	return result;
}

/* NOTE: Watch for those planeptrs of 0x00000000 and 0xFFFFFFFF for all zero / all one bitmaps !!!! */
static void PlanarToDirect(TrapContext* ctx, struct RenderInfo* ri, struct BitMap* bm,
                           unsigned long srcx, unsigned long srcy,
                           unsigned long dstx, unsigned long dsty,
                           unsigned long width, unsigned long height, uae_u8 mask, uaecptr acim)
{
	int j;
	int bpp = GetBytesPerPixel(ri->RGBFormat);
	uae_u8* PLANAR[8];
	uaecptr APLANAR[8];
	uae_u8* image = ri->Memory + dstx * bpp + dsty * ri->BytesPerRow;
	int Depth = bm->Depth;
	unsigned long rows;
	long eol_offset;
	//bool indirect = trap_is_indirect();
	int maxc = -1;
	uae_u32 cim[256];

	if(!bpp)
		return;

	/* Set up our bm->Planes[] pointers to the right horizontal offset */
	for (j = 0; j < Depth; j++) {
		uae_u8 *p = bm->Planes[j];
		if (p != &all_zeros_bitmap && p != &all_ones_bitmap)
			p += srcx / 8 + srcy * bm->BytesPerRow;
		PLANAR[j] = p;
		if ((mask & (1 << j)) == 0)
			PLANAR[j] = &all_zeros_bitmap;
	}

  eol_offset = (uae_s32) bm->BytesPerRow - (uae_s32) ((width + (srcx & 7)) >> 3);
	for (rows = 0; rows < height; rows++, image += ri->BytesPerRow) {
		unsigned long cols;
		uae_u8 *image2 = image;
		unsigned int bitoffs = 7 - (srcx & 7);
		int i;

		for (cols = 0; cols < width; cols ++) {
			int v = 0, k;
			for (k = 0; k < Depth; k++) {
				if (PLANAR[k] == &all_ones_bitmap)
					v |= 1 << k;
				else if (PLANAR[k] != &all_zeros_bitmap) {
					v |= ((*PLANAR[k] >> bitoffs) & 1) << k;
				}
			}

			// most operations use only low palette values
			// do not fetch and convert whole palette unless needed
			if (v > maxc) {
				int vc = v;
				if (vc < 3)
					vc = 3;
				else if (vc < 7)
					vc = 7;
				else if (vc < 15)
					vc = 15;
				else if (vc < 31)
					vc = 32;
				else if (vc < 63)
					vc = 63;
				else
					vc = 255;
				trap_get_longs(ctx, &cim[maxc + 1], acim + 4 + (maxc + 1) * 4, vc - maxc);
				for (int i = maxc + 1; i <= vc; i++) {
					endianswap(&cim[i], bpp);
				}
				maxc = vc;
			}

			switch (bpp)
			{
			case 2:
				((uae_u16 *)image2)[0] = (uae_u16)(cim[v]);
				image2 += 2;
				break;
			case 3:
				image2[0] = cim[v] >> 0;
				image2[1] = cim[v] >> 8;
				image2[2] = cim[v] >> 16;
				image2 += 3;
				break;
			case 4:
				((uae_u32 *)image2)[0] = cim[v];
				image2 += 4;
				break;
			}
			bitoffs--;
			bitoffs &= 7;
			if (bitoffs == 7) {
				int k;
				for (k = 0; k < Depth; k++) {
					if (PLANAR[k] != &all_zeros_bitmap && PLANAR[k] != &all_ones_bitmap) {
						PLANAR[k]++;
					}
				}
			}
		}

		for (i = 0; i < Depth; i++) {
			if (PLANAR[i] != &all_zeros_bitmap && PLANAR[i] != &all_ones_bitmap) {
				PLANAR[i] += eol_offset;
			}
		}
	}
}

/*
* BlitPlanar2Direct:
*
* Synopsis:
* BlitPlanar2Direct(bi, bm, ri, cim, SrcX, SrcY, DstX, DstY, SizeX, SizeY, MinTerm, Mask);
* Inputs:
* a0:struct BoardInfo *bi
* a1:struct BitMap *bm
* a2:struct RenderInfo *ri
* a3:struct ColorIndexMapping *cmi
* d0.w:SrcX
* d1.w:SrcY
* d2.w:DstX
* d3.w:DstY
* d4.w:SizeX
* d5.w:SizeY
* d6.b:MinTerm
* d7.b:Mask
*
* This function is currently used to blit from planar bitmaps within system memory to direct color
* bitmaps (15, 16, 24 or 32 bit) on the board. Watch out for plane pointers that are 0x00000000 (represents
* a plane with all bits "0") or 0xffffffff (represents a plane with all bits "1"). The ColorIndexMapping is
* used to map the color index of each pixel formed by the bits in the bitmap's planes to a direct color value
* which is written to the destination RenderInfo. The color mask and all colors within the mapping are words,
* triple bytes or longwords respectively similar to the color values used in FillRect(), BlitPattern() or
* BlitTemplate().
*/
static uae_u32 REGPARAM2 picasso_BlitPlanar2Direct (TrapContext *ctx)
{
	uaecptr bm = trap_get_areg(ctx, 1);
	uaecptr ri = trap_get_areg(ctx, 2);
	uaecptr cim = trap_get_areg(ctx, 3);
	unsigned long srcx = (uae_u16)trap_get_dreg(ctx, 0);
	unsigned long srcy = (uae_u16)trap_get_dreg(ctx, 1);
	unsigned long dstx = (uae_u16)trap_get_dreg(ctx, 2);
	unsigned long dsty = (uae_u16)trap_get_dreg(ctx, 3);
	unsigned long width = (uae_u16)trap_get_dreg(ctx, 4);
	unsigned long height = (uae_u16)trap_get_dreg(ctx, 5);
	uae_u8 minterm = trap_get_dreg(ctx, 6);
	uae_u8 Mask = trap_get_dreg(ctx, 7);
	struct RenderInfo local_ri;
	struct BitMap local_bm;
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;

	if (minterm != 0x0C) {
		write_log (_T("WARNING - BlitPlanar2Direct() has unhandled op-code 0x%x. Using fall-back routine.\n"), minterm);
		return 0;
	}
	if (CopyRenderInfoStructureA2U(ctx, ri, &local_ri) && CopyBitMapStructureA2U(ctx, bm, &local_bm)) {
		Mask = 0xFF;
		PlanarToDirect(ctx, &local_ri, &local_bm, srcx, srcy, dstx, dsty, width, height, Mask, cim);
		result = 1;
	}
	return result;
}

#include "statusline.h"
void picasso_statusline( uae_u8 *dst)
{
  struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	struct picasso96_state_struct *state = &picasso96_state;
	int y;
	int dst_height, dst_width, pitch;

	dst_height = state->Height;
	if (dst_height > vidinfo->height)
		dst_height = vidinfo->height;
	dst_width = state->Width;
	if (dst_width > vidinfo->width)
		dst_width = vidinfo->width;
	pitch = vidinfo->rowbytes;
	for (y = 0; y < TD_TOTAL_HEIGHT; y++)
	{
		int line = dst_height - TD_TOTAL_HEIGHT + y;
		uae_u8* buf = dst + line * pitch;
		draw_status_line_single(buf, vidinfo->pixbytes, y, dst_width, p96rc, p96gc, p96bc, nullptr);
	}
}

static void copyrow (uae_u8 *src, uae_u8 *dst, int x, int y, int width, int srcbytesperrow, int srcpixbytes, int dx, int dy, int dstbytesperrow, int dstpixbytes, bool direct, int convert_mode, uae_u32 *p96_rgbx16p)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u8* src2 = src + y * srcbytesperrow;
	uae_u8* dst2 = dst + dy * dstbytesperrow;
	int endx = x + width, endx4;
	int dstpix = dstpixbytes;
	int srcpix = srcpixbytes;

	if (direct) {
		memcpy(dst2 + x * dstpix, src2 + x * srcpix, width * dstpix);
		return;
	}
	// native match?
	if (currprefs.gfx_api) {
		switch (convert_mode)
		{
#ifdef WORDS_BIGENDIAN
		case RGBFB_A8R8G8B8_32:
		case RGBFB_R5G6B5_16:
#else
		case RGBFB_B8G8R8A8_32:
		case RGBFB_R5G6B5PC_16:
#endif
				memcpy (dst2 + dx * dstpix, src2 + x * srcpix, width * dstpix);
			return;
		}
	} else {
		switch (convert_mode)
		{
#ifdef WORDS_BIGENDIAN
		case RGBFB_A8R8G8B8_32:
		case RGBFB_R5G6B5_16:
#else
		case RGBFB_B8G8R8A8_32:
		case RGBFB_R5G6B5PC_16:
#endif
				memcpy (dst2 + dx * dstpix, src2 + x * srcpix, width * dstpix);
			return;
		}
	}

	endx4 = endx & ~3;

	switch (convert_mode)
	{
		/* 24bit->32bit */
	case RGBFB_R8G8B8_32:
		while (x < endx) {
			((uae_u32*)dst2)[dx] = (src2[x * 3 + 0] << 16) | (src2[x * 3 + 1] << 8) | (src2[x * 3 + 2] << 0);
			x++;
			dx++;
		}
		break;
	case RGBFB_B8G8R8_32:
		while (x < endx) {
			((uae_u32*)dst2)[dx] = ((uae_u32*)(src2 + x * 3))[0] & 0x00ffffff;
			x++;
			dx++;
		}
		break;

		/* 32bit->32bit */
	case RGBFB_R8G8B8A8_32:
		while (x < endx) {
			((uae_u32*)dst2)[dx] = (src2[x * 4 + 0] << 16) | (src2[x * 4 + 1] << 8) | (src2[x * 4 + 2] << 0);
			x++;
			dx++;
		}
		break;
	case RGBFB_A8R8G8B8_32:
		while (x < endx) {
			((uae_u32*)dst2)[dx] = (src2[x * 4 + 1] << 16) | (src2[x * 4 + 2] << 8) | (src2[x * 4 + 3] << 0);
			x++;
			dx++;
		}
		break;
	case RGBFB_A8B8G8R8_32:
		while (x < endx) {
			((uae_u32*)dst2)[dx] = ((uae_u32*)src2)[x] >> 8;
			x++;
			dx++;
		}
		break;

		/* 15/16bit->32bit */
	case RGBFB_R5G6B5PC_32:
	case RGBFB_R5G5B5PC_32:
	case RGBFB_R5G6B5_32:
	case RGBFB_R5G5B5_32:
	case RGBFB_B5G6R5PC_32:
	case RGBFB_B5G5R5PC_32:
		{
			while ((x & 3) && x < endx) {
				((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
			}
			while (x < endx4) {
				((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
				((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
				((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
				((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
			}
			while (x < endx) {
				((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
			}
		}
		break;

		/* 16/15bit->16bit */
	case RGBFB_R5G5B5PC_16:
	case RGBFB_R5G6B5_16:
	case RGBFB_R5G5B5_16:
	case RGBFB_B5G5R5PC_16:
	case RGBFB_B5G6R5PC_16:
	case RGBFB_R5G6B5PC_16:
		{
			while ((x & 3) && x < endx) {
				((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
			}
			while (x < endx4) {
				((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
				((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
				((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
				((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
			}
			while (x < endx) {
				((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				x++;
				dx++;
			}
		}
		break;

		/* 24bit->16bit */
	case RGBFB_R8G8B8_16:
		while (x < endx) {
			uae_u8 r, g, b;
			r = src2[x * 3 + 0];
			g = src2[x * 3 + 1];
			b = src2[x * 3 + 2];
			((uae_u16*)dst2)[dx] = p96_rgbx16p[(((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | (((b >> 3) & 0x1f) << 0)];
			x++;
			dx++;
		}
		break;
	case RGBFB_B8G8R8_16:
		while (x < endx) {
			uae_u32 v;
			v = ((uae_u32*)(&src2[x * 3]))[0] >> 8;
			((uae_u16*)dst2)[dx] = p96_rgbx16p[(((v >> (8 + 3)) & 0x1f) << 11) | (((v >> (0 + 2)) & 0x3f) << 5) | (((v >> (16 + 3)) & 0x1f) << 0)];
			x++;
			dx++;
		}
		break;

		/* 32bit->16bit */
	case RGBFB_R8G8B8A8_16:
		while (x < endx) {
			uae_u32 v;
			v = ((uae_u32*)src2)[x];
			((uae_u16*)dst2)[dx] = p96_rgbx16p[(((v >> (0 + 3)) & 0x1f) << 11) | (((v >> (8 + 2)) & 0x3f) << 5) | (((v >> (16 + 3)) & 0x1f) << 0)];
			x++;
			dx++;
		}
		break;
	case RGBFB_A8R8G8B8_16:
		while (x < endx) {
			uae_u32 v;
			v = ((uae_u32*)src2)[x];
			((uae_u16*)dst2)[dx] = p96_rgbx16p[(((v >> (8 + 3)) & 0x1f) << 11) | (((v >> (16 + 2)) & 0x3f) << 5) | (((v >> (24 + 3)) & 0x1f) << 0)];
			x++;
			dx++;
		}
		break;
	case RGBFB_A8B8G8R8_16:
		while (x < endx) {
			uae_u32 v;
			v = ((uae_u32*)src2)[x];
			((uae_u16*)dst2)[dx] = p96_rgbx16p[(((v >> (24 + 3)) & 0x1f) << 11) | (((v >> (16 + 2)) & 0x3f) << 5) | (((v >> (8 + 3)) & 0x1f) << 0)];
			x++;
			dx++;
		}
		break;
	case RGBFB_B8G8R8A8_16:
		while (x < endx) {
			uae_u32 v;
			v = ((uae_u32*)src2)[x];
			((uae_u16*)dst2)[dx] = p96_rgbx16p[(((v >> (16 + 3)) & 0x1f) << 11) | (((v >> (8 + 2)) & 0x3f) << 5) | (((v >> (0 + 3)) & 0x1f) << 0)];
			x++;
			dx++;
		}
		break;

		/* 8bit->32bit */
	case RGBFB_CLUT_RGBFB_32:
		{
			while ((x & 3) && x < endx) {
				((uae_u32*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
			}
			while (x < endx4) {
				((uae_u32*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
				((uae_u32*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
				((uae_u32*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
				((uae_u32*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
			}
			while (x < endx) {
				((uae_u32*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
			}
		}
		break;

		/* 8bit->16bit */
	case RGBFB_CLUT_RGBFB_16:
		{
			while ((x & 3) && x < endx) {
				((uae_u16*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
			}
			while (x < endx4) {
				((uae_u16*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
				((uae_u16*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
				((uae_u16*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
				((uae_u16*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
			}
			while (x < endx) {
				((uae_u16*)dst2)[dx] = vidinfo->clut[src2[x]];
				x++;
				dx++;
			}
		}
		break;
	}
}

static uae_u16 yuvtorgb(uae_u8 yx, uae_u8 ux, uae_u8 vx)
{
	int y = yx - 16;
	int u = ux - 128;
	int v = vx - 128;
	int r = (298 * y + 409 * v + 128) >> (8 + 3);
	int g = (298 * y - 100 * u - 208 * v + 128) >> (8 + 3);
	int b = (298 * y + 516 * u + 128) >> (8 + 3);
	if (r < 0)
		r = 0;
	if (r > 31)
		r = 31;
	if (g < 0)
		g = 0;
	if (g > 31)
		g = 31;
	if (b < 0)
		b = 0;
	if (b > 31)
		b = 31;
	return (r << 10) | (g << 5) | b;
}

#define CKCHECK if (!ck \
	|| (screenpixbytes == 4 && colorkey == ((uae_u32*)srcs)[dx]) \
	|| (screenpixbytes == 2 && (uae_u16)colorkey == ((uae_u16*)srcs)[dx]) \
	|| (screenpixbytes == 1 && (uae_u8)colorkey == ((uae_u8*)srcs)[dx]) \
	|| (screenpixbytes == 3 && ckbytes[0] == srcs[dx * 3 + 0] && ckbytes[1] == srcs[dx * 3 + 1] && ckbytes[2] == srcs[dx * 3 + 2]))

void copyrow_scale(uae_u8 *src, uae_u8 *src_screen, uae_u8 *dst,
	int sx, int sy, int sxadd, int width, int srcbytesperrow, int srcpixbytes,
	int screenbytesperrow, int screenpixbytes,
	int dx, int dy, int dstwidth, int dstheight, int dstbytesperrow, int dstpixbytes,
	bool ck, uae_u32 colorkey,
	int convert_mode, uae_u32 *p96_rgbx16p, uae_u32 *clut, bool yuv_swap)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u8 *src2 = src + sy * srcbytesperrow;
	uae_u8 *dst2 = dst + dy * dstbytesperrow;
	uae_u8 *srcs = src_screen + dy * screenbytesperrow;
	int endx, endx4;
	int dstpix = dstpixbytes;
	int srcpix = srcpixbytes;
	int x;
	uae_u8 ckbytes[3];

	if (dx < 0 || dx >= dstwidth)
		return;
	if (dy < 0 || dy >= dstheight)
		return;

	if (dx + (width * 256) / sxadd > dstwidth) {
		width = ((dstwidth - dx) * sxadd) / 256;
		if (width <= 0)
			return;
	}

	endx = (sx + width) << 8;

	switch (convert_mode)
	{
		case RGBFB_Y4U2V2_32:
		case RGBFB_Y4U2V2_16:
		endx /= 2;
		sxadd /= 2;
		break;
		case RGBFB_Y4U1V1_32:
		case RGBFB_Y4U1V1_16:
		endx /= 4;
		sxadd /= 4;
		break;
	}

	endx4 = endx & ~(3 << 8);

	ckbytes[0] = colorkey >> 16;
	ckbytes[1] = colorkey >> 8;
	ckbytes[2] = colorkey >> 0;

	switch (convert_mode)
	{
		/* 24bit->32bit */
		case RGBFB_R8G8B8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = (src2[x * 3 + 0] << 16) | (src2[x * 3 + 1] << 8) | (src2[x * 3 + 2] << 0);
				dx++;
			}
			break;
		case RGBFB_B8G8R8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = ((uae_u32*)(src2 + x * 3))[0] & 0x00ffffff;
				dx++;
			}
			break;

		/* 32bit->32bit */
		case RGBFB_R8G8B8A8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = (src2[x * 4 + 0] << 16) | (src2[x * 4 + 1] << 8) | (src2[x * 4 + 2] << 0);
				dx++;
			}
			break;
		case RGBFB_A8R8G8B8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = (src2[x * 4 + 1] << 16) | (src2[x * 4 + 2] << 8) | (src2[x * 4 + 3] << 0);
				dx++;
			}
			break;
		case RGBFB_A8B8G8R8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = ((uae_u32*)src2)[x] >> 8;
				dx++;
			}
			break;

		/* 15/16bit->32bit */
		case RGBFB_R5G6B5PC_32:
		case RGBFB_R5G5B5PC_32:
		case RGBFB_R5G6B5_32:
		case RGBFB_R5G5B5_32:
		case RGBFB_B5G6R5PC_32:
		case RGBFB_B5G5R5PC_32:
		{
			while ((sx & (3 << 8)) && sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
			}
		}
		break;

		case RGBFB_Y4U2V2_32:
		{
			uae_u32 outval1, outval2;
			uae_u8 y0, y1, u, v;
			bool docalc1 = false;
			bool docalc2 = false;
			int oldsx = -1;
			uae_u32 val = ((uae_u32*)src2)[sx >> 8];
			uae_u32 oldval = val ^ 1;
			while (sx < endx) {
				x = sx >> 8;
				if (oldsx != x) {
					uae_u32 val = ((uae_u32*)src2)[x];
					if (val != oldval) {
						oldval = val;
						if (yuv_swap)
							val = ((val & 0xff00ff00) >> 8) | ((val & 0x00ff00ff) << 8);
						y0 = val >> 8;
						y1 = val >> 24;
						u = val >> 0;
						v = val >> 16;
						if (y0 == y1) {
							uae_u16 out = yuvtorgb(y0, u, v);
							outval1 = p96_rgbx16p[out];
							outval2 = outval1;
						} else {
							docalc1 = true;
							docalc2 = true;
						}
					}
					oldsx = x;
				}
				if ((sx & 255) < 128) {
					CKCHECK
					{
						if (docalc1) {
							uae_u16 out = yuvtorgb(y0, u, v);
							outval1 = p96_rgbx16p[out];
							docalc1 = false;
						}
						((uae_u32*)dst2)[dx] = outval1;
					}
				} else {
					CKCHECK
					{
						if (docalc2) {
							uae_u16 out = yuvtorgb(y1, u, v);
							outval2 = p96_rgbx16p[out];
							docalc2 = false;
						}
						((uae_u32*)dst2)[dx] = outval2;
					}
				}
				sx += sxadd;
				dx++;
			}
		}
		break;

		case RGBFB_Y4U1V1_32:
		{
			while (sx < endx) {
				x = sx >> 8;
				uae_u32 val = ((uae_u32*)src2)[x];
				uae_u8 y0 = ((val >> 12) & 31) * 8;
				uae_u8 y1 = ((val >> 17) & 31) * 8;
				uae_u8 y2 = ((val >> 22) & 31) * 8;
				uae_u8 y3 = ((val >> 27) & 31) * 8;
				uae_s8 u = ((val >> 0) & 63) * 4;
				uae_s8 v = ((val >> 6) & 63) * 4;
				int fr = sx & 255;
				if (fr >= 192) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y3, u, v);
						((uae_u32*)dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 128) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y2, u, v);
						((uae_u32*)dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 64) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y1, u, v);
						((uae_u32*)dst2)[dx] = p96_rgbx16p[out];
					}
				} else {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y0, u, v);
						((uae_u32*)dst2)[dx] = p96_rgbx16p[out];
					}
				}
				sx += sxadd;
				dx++;
			}
		}
		break;

		/* 16/15bit->16bit */
		case RGBFB_R5G5B5PC_16:
		case RGBFB_R5G6B5_16:
		case RGBFB_R5G5B5_16:
		case RGBFB_B5G5R5PC_16:
		case RGBFB_B5G6R5PC_16:
		case RGBFB_R5G6B5PC_16:
		{
			while ((sx & (3 << 8)) && sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = (uae_u16)p96_rgbx16p[((uae_u16*)src2)[x]];
				dx++;
			}
		}
		break;

		/* 8bit->16bit */
		case RGBFB_CLUT_RGBFB_16:
		{
			while ((sx & (3 << 8)) && sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u16*)dst2)[dx] = clut[src2[x]];
				dx++;
			}
		}
		break;

		/* 8bit->32bit */
		case RGBFB_CLUT_RGBFB_32:
		{
			while ((sx & (3 << 8)) && sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					((uae_u32*)dst2)[dx] = clut[src2[x]];
				dx++;
			}
		}
		break;

		case RGBFB_Y4U2V2_16:
		{
			uae_u16 outval1, outval2;
			uae_u8 y0, y1, u, v;
			bool docalc1 = false;
			bool docalc2 = false;
			int oldsx = -1;
			uae_u32 val = ((uae_u32*)src2)[sx >> 8];
			uae_u32 oldval = val ^ 1;
			while (sx < endx) {
				x = sx >> 8;
				if (x != oldsx) {
					val = ((uae_u32*)src2)[x];
					if (val != oldval) {
						oldval = val;
						if (yuv_swap)
							val = ((val & 0xff00ff00) >> 8) | ((val & 0x00ff00ff) << 8);
						y0 = val >> 8;
						y1 = val >> 24;
						u = val >> 0;
						v = val >> 16;
						if (y0 == y1) {
							uae_u16 out = yuvtorgb(y0, u, v);
							outval1 = p96_rgbx16p[out];
							outval2 = outval1;
						} else {
							docalc1 = true;
							docalc2 = true;
						}
					}
					oldsx = x;
				}
				if ((sx & 255) < 128) {
					CKCHECK
					{
						if (docalc1) {
							uae_u16 out = yuvtorgb(y0, u, v);
							outval1 = p96_rgbx16p[out];
							docalc1 = false;
						}
						((uae_u16*)dst2)[dx] = outval1;
					}
					CKCHECK
					{
						if (docalc2) {
							uae_u16 out = yuvtorgb(y1, u, v);
							outval2 = p96_rgbx16p[out];
							docalc2 = false;
						}
						((uae_u16*)dst2)[dx] = outval2;
					}
				}
				sx += sxadd;
				dx++;
			}
		}
		break;

		case RGBFB_Y4U1V1_16:
		{
			while (sx < endx) {
				x = sx >> 8;
				uae_u32 val = ((uae_u32*)src2)[x];
				uae_u8 y0 = ((val >> 12) & 31) * 8;
				uae_u8 y1 = ((val >> 17) & 31) * 8;
				uae_u8 y2 = ((val >> 22) & 31) * 8;
				uae_u8 y3 = ((val >> 27) & 31) * 8;
				uae_s8 u = ((val >> 0) & 63) * 4;
				uae_s8 v = ((val >> 6) & 63) * 4;
				int fr = sx & 255;
				if (fr >= 192) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y3, u, v);
						((uae_u16*)dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 128) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y2, u, v);
						((uae_u16*)dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 64) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y1, u, v);
						((uae_u16*)dst2)[dx] = p96_rgbx16p[out];
					}
				} else {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y0, u, v);
						((uae_u16*)dst2)[dx] = p96_rgbx16p[out];
					}
				}
				sx += sxadd;
				dx++;
			}
		}
		break;

	}
}

void fb_copyrow(uae_u8 *src, uae_u8 *dst, int x, int y, int width, int srcpixbytes, int dy)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	struct picasso96_state_struct *state = &picasso96_state;
	copyrow(src, dst, x, y, width, 0, srcpixbytes,
		x, dy, picasso_vidinfo.rowbytes, picasso_vidinfo.pixbytes,
		state->RGBFormat == vidinfo->host_mode, vidinfo->picasso_convert, p96_rgbx16);
}

static void copyallinvert(uae_u8 *src, uae_u8 *dst, int pwidth, int pheight, int srcbytesperrow, int srcpixbytes, int dstbytesperrow, int dstpixbytes, bool direct, int mode_convert)
{
	int x, y, w;

	w = pwidth * dstpixbytes;
	if (direct) {
		for (y = 0; y < pheight; y++) {
			for (x = 0; x < w; x++)
				dst[x] = src[x] ^ 0xff;
			dst += dstbytesperrow;
			src += srcbytesperrow;
		}
	} else {
		uae_u8 *src2 = src;
		for (y = 0; y < pheight; y++) {
			for (x = 0; x < w; x++)
				src2[x] ^= 0xff;
			copyrow(src, dst, 0, y, pwidth, srcbytesperrow, srcpixbytes, 0, y, dstbytesperrow, dstpixbytes, direct, mode_convert, p96_rgbx16);
			for (x = 0; x < w; x++)
				src2[x] ^= 0xff;
			src2 += srcbytesperrow;
		}
	}
}

static void copyall (uae_u8 *src, uae_u8 *dst, int pwidth, int pheight, int srcbytesperrow, int srcpixbytes, int dstbytesperrow, int dstpixbytes, bool direct, int mode_convert)
{
  struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	int y, bytes;
	if (direct)
	{
		auto w = pwidth * vidinfo->pixbytes;
		for (y = 0; y < pheight; y++)
		{
			memcpy(dst, src, w);
			dst += dstbytesperrow;
			src += srcbytesperrow;
		}
	}
	else
	{
		for (y = 0; y < pheight; y++)
			copyrow (src, dst, 0, y, pwidth, srcbytesperrow, srcpixbytes, 0, y, dstbytesperrow, dstpixbytes, direct, mode_convert, p96_rgbx16);
	}
}

uae_u8 *getrtgbuffer(int *widthp, int *heightp, int *pitch, int *depth, uae_u8 *palette)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	struct picasso96_state_struct *state = &picasso96_state;
	uae_u8 *src = gfxmem_bank.start + regs.natmem_offset;
	int off = state->XYOffset - gfxmem_bank.start;
	int width, height, pixbytes;
	uae_u8 *dst;
	int convert;
	int hmode;

	if (!vidinfo->extra_mem)
		return NULL;

	width = state->VirtualWidth;
	height = state->VirtualHeight;
	pixbytes = state->BytesPerPixel == 1 && palette ? 1 : 4;
	if (!width || !height || !pixbytes)
		return NULL;

	dst = xmalloc (uae_u8, width * height * pixbytes);
	if (!dst)
		return NULL;
	hmode = pixbytes == 1 ? RGBFB_CLUT : RGBFB_B8G8R8A8;
	convert = getconvert (state->RGBFormat, pixbytes);
	alloc_colors_picasso(8, 8, 8, 16, 8, 0, state->RGBFormat, p96_rgbx16);

	if (pixbytes > 1 && hmode != convert) {
		copyall (src + off, dst, width, height, state->BytesPerRow, state->BytesPerPixel, width * pixbytes, pixbytes, false, convert);
	} else {
		uae_u8 *dstp = dst;
		uae_u8 *srcp = src;
		for (int y = 0; y < height; y++) {
			memcpy (dstp, srcp, width * pixbytes);
			dstp += width * pixbytes;
			srcp += state->BytesPerRow;
		}
	}
	if (pixbytes == 1) {
		for (int i = 0; i < 256; i++) {
			palette[i * 3 + 0] = state->CLUT[i].Red;
			palette[i * 3 + 1] = state->CLUT[i].Green;
			palette[i * 3 + 2] = state->CLUT[i].Blue;
		}
	}

	gfx_set_picasso_colors(state->RGBFormat);

	*widthp = width;
	*heightp = height;
	*pitch = width * pixbytes;
	*depth = pixbytes * 8;

	return dst;
}
void freertgbuffer(uae_u8 *dst)
{
	xfree (dst);
}

void picasso_invalidate(int x, int y, int w, int h)
{

}

static bool picasso_flushpixels (uae_u8 *src, int off)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	struct picasso96_state_struct *state = &picasso96_state;
	uae_u8* src_start;
	uae_u8* src_end;
	uae_u8* dst = nullptr;
	int pwidth = state->Width > state->VirtualWidth ? state->VirtualWidth : state->Width;
	int pheight = state->Height > state->VirtualHeight ? state->VirtualHeight : state->Height;

	src_start = src + off;
	src_end = src + off + state->BytesPerRow * pheight - 1;
	if (!vidinfo->extra_mem || src_start >= src_end) {
		return false;
	}

	dst = gfx_lock_picasso();
	if (dst == nullptr)
		return false;

	copyall(src + off, dst, pwidth, pheight,
			state->BytesPerRow, state->BytesPerPixel,
			vidinfo->rowbytes, vidinfo->pixbytes,
			state->RGBFormat == vidinfo->host_mode, vidinfo->picasso_convert);

	if (currprefs.leds_on_screen)
		picasso_statusline(dst);

	gfx_unlock_picasso(true);

	return true;
}

static int render_thread(void* v)
{
	render_thread_state = 1;
	for (;;) {
		int idx = read_comm_pipe_int_blocking(render_pipe);
		if (idx == -1)
			break;
		idx &= 0xff;

		struct amigadisplay *ad = &adisplays;
		if (ad->picasso_on && ad->picasso_requested_on) {
			if (ad->picasso_requested_on) {
				struct picasso96_state_struct *state = &picasso96_state;
				picasso_flushpixels(gfxmem_banks[idx]->start + regs.natmem_offset, static_cast<int>(state->XYOffset - gfxmem_banks[idx]->start));
				ad->pending_render = true;
			}
		}
	}
	render_thread_state = -1;
	return 0;
}

extern addrbank gfxmem_bank;
MEMORY_FUNCTIONS(gfxmem);
addrbank gfxmem_bank = {
	gfxmem_lget, gfxmem_wget, gfxmem_bget,
	gfxmem_lput, gfxmem_wput, gfxmem_bput,
	gfxmem_xlate, gfxmem_check, nullptr, nullptr, _T("RTG RAM"),
	dummy_wgeti,
	ABFLAG_RAM | ABFLAG_RTG, 0, 0
};
addrbank* gfxmem_banks[MAX_RTG_BOARDS];

/* Call this function first, near the beginning of code flow
* Place in InitGraphics() which seems reasonable...
* Also put it in reset_drawing() for safe-keeping.  */
void InitPicasso96()
{
	struct picasso96_state_struct *state = &picasso96_state;
	int i;

	gfxmem_banks[0] = &gfxmem_bank;

	//fastscreen
	oldscr = 0;
	//fastscreen
	memset (state, 0, sizeof (struct picasso96_state_struct));

	for (int i = 0; i < 256; i++)
	{
		p2ctab[i][0] = (((i & 128) ? 0x01000000 : 0)
			| ((i & 64) ? 0x010000 : 0)
			| ((i & 32) ? 0x0100 : 0)
			| ((i & 16) ? 0x01 : 0));
		p2ctab[i][1] = (((i & 8) ? 0x01000000 : 0)
			| ((i & 4) ? 0x010000 : 0)
			| ((i & 2) ? 0x0100 : 0)
			| ((i & 1) ? 0x01 : 0));
	}
}

#endif

static uae_u32 REGPARAM2 picasso_SetInterrupt(TrapContext* ctx)
{
	uaecptr bi = trap_get_areg(ctx, 0);
	uae_u32 onoff = trap_get_dreg(ctx, 0);
	interrupt_enabled = onoff;
	//write_log (_T("Picasso_SetInterrupt(%08x,%d)\n"), bi, onoff);
	return onoff;
}

static uaecptr uaegfx_vblankname, uaegfx_portsname;

static void initvblankABI(TrapContext* ctx, uaecptr base, uaecptr ABI)
{
	for (int i = 0; i < 22; i++)
		trap_put_byte(ctx, ABI + PSSO_BoardInfo_HardInterrupt + i, trap_get_byte(ctx, base + CARD_PORTSIRQ + i));
	ABI_interrupt = ABI;
}

static void initvblankirq(TrapContext* ctx, uaecptr base)
{
	uaecptr p1 = base + CARD_VBLANKIRQ;
	uaecptr p2 = base + CARD_PORTSIRQ;
	uaecptr c = base + CARD_IRQCODE;

	trap_put_word(ctx, p1 + 8, 0x0205);
	trap_put_long(ctx, p1 + 10, uaegfx_vblankname);
	trap_put_long(ctx, p1 + 14, base + CARD_IRQFLAG);
	trap_put_long(ctx, p1 + 18, c);

	trap_put_word(ctx, p2 + 8, 0x0205);
	trap_put_long(ctx, p2 + 10, uaegfx_portsname);
	trap_put_long(ctx, p2 + 14, base + CARD_IRQFLAG);
	trap_put_long(ctx, p2 + 18, c);

	trap_put_word(ctx, c, 0x4a11); c += 2;		// tst.b (a1) CARD_IRQFLAG
	trap_put_word(ctx, c, 0x670e); c += 2;		// beq.s label
	trap_put_word(ctx, c, 0x4211); c += 2;		// clr.b (a1)
	trap_put_long(ctx, c, 0x2c690008); c += 4;	// move.l 8(a1),a6 CARD_IRQEXECBASE
	trap_put_long(ctx, c, 0x22690004); c += 4;	// move.l 4(a1),a1 CARD_IRQPTR
	trap_put_long(ctx, c, 0x4eaeff4c); c += 4;	// jsr Cause(a6)
	trap_put_word(ctx, c, 0x7000); c += 2;		// label: moveq #0,d0
	trap_put_word(ctx, c, RTS);					// rts

#if NEWTRAP
	trap_call_add_areg(ctx, 1, p1);
	trap_call_add_dreg(ctx, 0, 5);
	trap_call_lib(ctx, trap_get_long(ctx, 4), -168); 	/* AddIntServer */

	trap_call_add_areg(ctx, 1, p2);
	trap_call_add_dreg(ctx, 0, 3);
	trap_call_lib(ctx, trap_get_long(ctx, 4), -168); 	/* AddIntServer */
#else
	trap_set_areg(ctx, 1, p1);
	trap_set_dreg(ctx, 0, 5);			/* VERTB */
	CallLib (ctx, trap_get_long(ctx, 4), -168); 	/* AddIntServer */
	trap_set_areg(ctx, 1, p2);
	trap_set_dreg(ctx, 0, 3);			/* PORTS */
	CallLib (ctx, trap_get_long(ctx, 4), -168);	/* AddIntServer */
#endif
}

static uae_u32 REGPARAM2 picasso_SetClock(TrapContext* ctx)
{
	uaecptr bi = trap_get_areg(ctx, 0);
	return 0;
}

static uae_u32 REGPARAM2 picasso_SetMemoryMode(TrapContext* ctx)
{
	uaecptr bi = trap_get_areg(ctx, 0);
	uae_u32 rgbformat = trap_get_dreg(ctx, 7);
	return 0;
}

#define PUTABI(func) \
  if (ABI) \
		trap_put_long(ctx, ABI + func, here ());

#define RTGCALL(func,funcdef,call) \
  PUTABI (func); \
  dl (0x48e78000); \
  calltrap (deftrap (call)); \
  dw (0x4a80); \
  dl (0x4cdf0001);\
  dw (0x6604); \
  dw (0x2f28); \
  dw (funcdef); \
  dw (RTS);

#define RTGCALL2(func,call) \
  PUTABI (func); \
  calltrap (deftrap (call)); \
  dw (RTS);

#define RTGCALLDEFAULT(func,funcdef) \
	PUTABI (func); \
	dw (0x2f28); \
	dw (funcdef); \
	dw (RTS);

#define RTGNONE(func) \
  if (ABI) \
		trap_put_long(ctx, ABI + func, start);

static void inituaegfxfuncs(TrapContext* ctx, uaecptr start, uaecptr ABI)
{
	if (uaegfx_old || !ABI)
		return;
	org(start);

	dw(RTS);
	/* ResolvePixelClock
	move.l	D0,gmi_PixelClock(a1)	; pass the pixelclock through
	moveq	#0,D0					; index is 0
	move.b	#98,gmi_Numerator(a1)	; whatever
	move.b	#14,gmi_Denominator(a1)	; whatever
	rts
	*/
	PUTABI (PSSO_BoardInfo_ResolvePixelClock);
	dl (0x2340002c);
	dw (0x7000);
	dl (0x137c0062); dw (0x002a);
	dl (0x137c000e); dw (0x002b);
	dw (RTS);

	/* GetPixelClock
	move.l #CLOCK,D0 ; fill in D0 with our one true pixel clock
	rts
	*/
	PUTABI (PSSO_BoardInfo_GetPixelClock);
	dw (0x203c);
	dl (100227260);
	dw (RTS);

	/* CalculateMemory
	; this is simple, because we're not supporting planar modes in UAE
	move.l	a1,d0
	rts
	*/
	PUTABI (PSSO_BoardInfo_CalculateMemory);
	dw (0x2009);
	dw (RTS);

	/* GetCompatibleFormats
; all formats can coexist without any problems, since we don't support planar stuff in UAE
move.l	#RGBMASK_8BIT | RGBMASK_15BIT | RGBMASK_16BIT | RGBMASK_24BIT | RGBMASK_32BIT,d0
rts
*/
	PUTABI(PSSO_BoardInfo_GetCompatibleFormats);
	dw(0x203c);
  dl (RGBMASK_8BIT | RGBMASK_15BIT | RGBMASK_16BIT | RGBMASK_24BIT | RGBMASK_32BIT);
	dw(RTS);

	/* CalculateBytesPerRow (optimized) */
	PUTABI(PSSO_BoardInfo_CalculateBytesPerRow);
	dl(0x0c400140); // cmp.w #320,d0
	uaecptr addr = here();
	dw(0);
	calltrap(deftrap(picasso_CalculateBytesPerRow));
	uaecptr addr2 = here();
	org(addr);
	dw(0x6500 | (addr2 - addr)); // bcs.s .l1
	org(addr2);
	dw(RTS);
	dw(0x0c87); dl (0x00000010); // l1: cmp.l #$10,d7
	dw(0x640a); // bcc.s .l2
	dw(0x7200); // moveq #0,d1
	dl(0x123b7010); // move.b table(pc,d7.w),d1
	dw(0x6b04); // bmi.s l3
	dw(0xe368); // lsl.w d1,d0
	dw(RTS); // .l2
	dw(0x3200); // .l3 move.w d0,d1
	dw(0xd041);  // add.w d1,d0
	dw(0xd041); // add.w d1,d0
	dw(RTS);
	dl(0x0000ffff); // table
	dl(0x01010202);
	dl(0x02020101);
	dl(0x01010100);

	//RTGCALL2(PSSO_BoardInfo_SetClock, picasso_SetClock);
	//RTGCALL2(PSSO_BoardInfo_SetMemoryMode, picasso_SetMemoryMode);
	RTGNONE(PSSO_BoardInfo_SetClock);
	RTGNONE(PSSO_BoardInfo_SetMemoryMode);
	RTGNONE(PSSO_BoardInfo_SetWriteMask);
	RTGNONE(PSSO_BoardInfo_SetClearMask);
	RTGNONE(PSSO_BoardInfo_SetReadPlane);

	RTGNONE(PSSO_BoardInfo_WaitVerticalSync);
	RTGNONE(PSSO_BoardInfo_WaitBlitter);

	RTGCALL(PSSO_BoardInfo_BlitPlanar2Direct, PSSO_BoardInfo_BlitPlanar2DirectDefault, picasso_BlitPlanar2Direct);
	RTGCALL(PSSO_BoardInfo_FillRect, PSSO_BoardInfo_FillRectDefault, picasso_FillRect);
	RTGCALL(PSSO_BoardInfo_BlitRect, PSSO_BoardInfo_BlitRectDefault, picasso_BlitRect);
	RTGCALL(PSSO_BoardInfo_BlitPlanar2Chunky, PSSO_BoardInfo_BlitPlanar2ChunkyDefault, picasso_BlitPlanar2Chunky);
	RTGCALL(PSSO_BoardInfo_BlitTemplate, PSSO_BoardInfo_BlitTemplateDefault, picasso_BlitTemplate);
	RTGCALL(PSSO_BoardInfo_InvertRect, PSSO_BoardInfo_InvertRectDefault, picasso_InvertRect);
	RTGCALL(PSSO_BoardInfo_BlitRectNoMaskComplete, PSSO_BoardInfo_BlitRectNoMaskCompleteDefault, picasso_BlitRectNoMaskComplete);
	RTGCALL(PSSO_BoardInfo_BlitPattern, PSSO_BoardInfo_BlitPatternDefault, picasso_BlitPattern);

	RTGCALL2(PSSO_BoardInfo_SetSwitch, picasso_SetSwitch);
	RTGCALL2(PSSO_BoardInfo_SetColorArray, picasso_SetColorArray);
	RTGCALL2(PSSO_BoardInfo_SetDAC, picasso_SetDAC);
	RTGCALL2(PSSO_BoardInfo_SetGC, picasso_SetGC);
	RTGCALL2(PSSO_BoardInfo_SetPanning, picasso_SetPanning);
	RTGCALL2(PSSO_BoardInfo_SetDisplay, picasso_SetDisplay);

	RTGCALL2(PSSO_BoardInfo_SetSprite, picasso_SetSprite);
	RTGCALL2(PSSO_BoardInfo_SetSpritePosition, picasso_SetSpritePosition);
	RTGCALL2(PSSO_BoardInfo_SetSpriteImage, picasso_SetSpriteImage);
	RTGCALL2(PSSO_BoardInfo_SetSpriteColor, picasso_SetSpriteColor);

	RTGCALLDEFAULT(PSSO_BoardInfo_ScrollPlanar, PSSO_BoardInfo_ScrollPlanarDefault);
	RTGCALLDEFAULT(PSSO_BoardInfo_UpdatePlanar, PSSO_BoardInfo_UpdatePlanarDefault);
	RTGCALLDEFAULT(PSSO_BoardInfo_DrawLine, PSSO_BoardInfo_DrawLineDefault);

	RTGCALL2(PSSO_BoardInfo_SetInterrupt, picasso_SetInterrupt);

	write_log (_T("uaegfx.card magic code: %08X-%08X BI=%08X\n"), start, here (), ABI);

	if (ABI)
		initvblankABI(ctx, uaegfx_base, ABI);
}

static void picasso_reset(int hardreset)
{
	struct picasso96_state_struct *state = &picasso96_state;
	if (currprefs.rtg_multithread) {
		if (!render_pipe) {
			//InitializeCriticalSection(&render_cs);
			render_pipe = xmalloc(smp_comm_pipe, 1);
			init_comm_pipe(render_pipe, 10, 1);
		}
		if (render_thread_state <= 0)
		{
			render_thread_state = 0;
			uae_start_thread(_T("rtg"), render_thread, nullptr, nullptr);
		}
	}

	if (savestate_state != STATE_RESTORE) {
		uaegfx_base = 0;
		uaegfx_old = 0;
		uaegfx_active = 0;
		interrupt_enabled = 0;
		reserved_gfxmem = 0;
		resetpalette(state);
		InitPicasso96();
	}

	struct amigadisplay *ad = &adisplays;
	ad->picasso_requested_on = false;
}

static void picasso_free(void)
{
	if (render_thread_state > 0) {
		write_comm_pipe_int(render_pipe, -1, 0);
		while (render_thread_state >= 0) {
			Sleep(10);
		}
		render_thread_state = 0;
	}
}

void uaegfx_install_code(uaecptr start)
{
	uaegfx_rom = start;
	org(start);
	inituaegfxfuncs(nullptr, start, 0);

	device_add_reset(picasso_reset);
	//device_add_hsync(picasso_handle_hsync);
	device_add_exit(picasso_free);
}

#define UAEGFX_VERSION 3
#define UAEGFX_REVISION 4

static uae_u32 REGPARAM2 gfx_open(TrapContext* ctx)
{
	trap_put_word(ctx, uaegfx_base + 32, trap_get_word(ctx, uaegfx_base + 32) + 1);
	return uaegfx_base;
}

static uae_u32 REGPARAM2 gfx_close(TrapContext* ctx)
{
	trap_put_word(ctx, uaegfx_base + 32, trap_get_word(ctx, uaegfx_base + 32) - 1);
	return 0;
}

static uae_u32 REGPARAM2 gfx_expunge(TrapContext* ctx)
{
	return 0;
}

static uaecptr uaegfx_card_install(TrapContext* ctx, uae_u32 extrasize)
{
	uae_u32 functable, datatable, a2;
	uaecptr openfunc, closefunc, expungefunc;
	uaecptr findcardfunc, initcardfunc;
	uaecptr exec = trap_get_long(ctx, 4);

	if (uaegfx_old || !gfxmem_bank.start)
		return 0;

	uaegfx_resid = ds (_T("UAE Graphics Card 3.4"));
	uaegfx_vblankname = ds(_T("UAE Graphics Card VBLANK"));
	uaegfx_portsname = ds(_T("UAE Graphics Card PORTS"));

	/* Open */
	openfunc = here ();
	calltrap (deftrap (gfx_open)); dw (RTS);

	/* Close */
	closefunc = here ();
	calltrap (deftrap (gfx_close)); dw (RTS);

	/* Expunge */
	expungefunc = here ();
	calltrap (deftrap (gfx_expunge)); dw (RTS);

	/* FindCard */
	findcardfunc = here ();
	calltrap (deftrap (picasso_FindCard)); dw (RTS);

	/* InitCard */
	initcardfunc = here ();
	calltrap (deftrap (picasso_InitCard)); dw (RTS);

	functable = here ();
	dl (openfunc);
	dl (closefunc);
	dl (expungefunc);
	dl (EXPANSION_nullfunc);
	dl (findcardfunc);
	dl (initcardfunc);
	dl (0xFFFFFFFF); /* end of table */

	datatable = makedatatable (uaegfx_resid, uaegfx_resname, 0x09, -50, UAEGFX_VERSION, UAEGFX_REVISION);

	a2 = trap_get_areg(ctx, 2);

#if NEWTRAP
	trap_call_add_areg(ctx, 0, functable);
	trap_call_add_areg(ctx, 1, datatable);
	trap_call_add_areg(ctx, 2, 0);
	trap_call_add_dreg(ctx, 0, CARD_SIZEOF + extrasize);
	trap_call_add_dreg(ctx, 1, 0);
	uaegfx_base = trap_call_lib(ctx, exec, -0x54); /* MakeLibrary */
#else
	trap_set_areg(ctx, 0, functable);
	trap_set_areg(ctx, 1, datatable);
	trap_set_areg(ctx, 2, 0);
	trap_set_dreg(ctx, 0, CARD_SIZEOF + extrasize);
	trap_set_dreg(ctx, 1, 0);
	uaegfx_base = CallLib(ctx, exec, -0x54); /* MakeLibrary */
#endif

	trap_set_areg(ctx, 2, a2);
	if (!uaegfx_base)
		return 0;

#if NEWTRAP
	trap_call_add_areg(ctx, 1, uaegfx_base);
	trap_call_lib(ctx, exec, -0x18c); /* AddLibrary */
#else
	trap_set_areg(ctx, 1, uaegfx_base);
	CallLib(ctx, exec, -0x18c); /* AddLibrary */
#endif

#if NEWTRAP
	trap_call_add_areg(ctx, 1, EXPANSION_explibname);
	trap_call_add_dreg(ctx, 0, 0);
	uae_u32 lib = trap_call_lib(ctx, exec, -0x228); /* OpenLibrary */
#else
	trap_set_areg(ctx, 1, EXPANSION_explibname);
	trap_set_dreg(ctx, 0, 0);
	uae_u32 lib = CallLib(ctx, exec, -0x228); /* OpenLibrary */
#endif

	trap_put_long(ctx, uaegfx_base + CARD_EXPANSIONBASE, lib); /* OpenLibrary */
	trap_put_long(ctx, uaegfx_base + CARD_EXECBASE, exec);
	trap_put_long(ctx, uaegfx_base + CARD_IRQEXECBASE, exec);
	trap_put_long(ctx, uaegfx_base + CARD_NAME, uaegfx_resname);
	trap_put_long(ctx, uaegfx_base + CARD_RESLIST, uaegfx_base + CARD_SIZEOF);
	trap_put_long(ctx, uaegfx_base + CARD_RESLISTSIZE, extrasize);

	initvblankirq(ctx, uaegfx_base);

	write_log(_T("uaegfx.card %d.%d init @%08X\n"), UAEGFX_VERSION, UAEGFX_REVISION, uaegfx_base);
	uaegfx_active = 1;

	return uaegfx_base;
}

uae_u32 picasso_demux(uae_u32 arg, TrapContext* ctx)
{
	uae_u32 num = trap_get_long(ctx, trap_get_areg(ctx, 7) + 4);

	if (uaegfx_base) {
		if (num >= 16 && num <= 39) {
			write_log (_T("uaelib: obsolete Picasso96 uaelib hook called, call ignored\n"));
			return 0;
		}
	}
	if (!uaegfx_old) {
		write_log (_T("uaelib: uaelib hook in use\n"));
		uaegfx_old = 1;
		uaegfx_active = 1;
	}
	switch (num)
	{
     case 16: return picasso_FindCard (ctx);
     case 17: return picasso_FillRect (ctx);
     case 18: return picasso_SetSwitch (ctx);
     case 19: return picasso_SetColorArray (ctx);
     case 20: return picasso_SetDAC (ctx);
     case 21: return picasso_SetGC (ctx);
     case 22: return picasso_SetPanning (ctx);
     case 23: return picasso_CalculateBytesPerRow (ctx);
     case 24: return picasso_BlitPlanar2Chunky (ctx);
     case 25: return picasso_BlitRect (ctx);
     case 26: return picasso_SetDisplay (ctx);
     case 27: return picasso_BlitTemplate (ctx);
     case 28: return picasso_BlitRectNoMaskComplete (ctx);
     case 29: return picasso_InitCard (ctx);
     case 30: return picasso_BlitPattern (ctx);
     case 31: return picasso_InvertRect (ctx);
     case 32: return picasso_BlitPlanar2Direct (ctx);
     //case 34: return picasso_WaitVerticalSync (ctx);
     case 35: return gfxmem_bank.allocated_size ? 1 : 0;
	 case 36: return picasso_SetSprite (ctx);
	 case 37: return picasso_SetSpritePosition (ctx);
	 case 38: return picasso_SetSpriteImage (ctx);
	 case 39: return picasso_SetSpriteColor (ctx);
	}

	return 0;
}

static uae_u32 p96_restored_flags;

void restore_p96_finish(void)
{
	struct amigadisplay *ad = &adisplays;
	struct picasso96_state_struct *state = &picasso96_state;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;

	init_alloc (NULL, 0);
	if (uaegfx_rom && boardinfo) {
		inituaegfxfuncs(NULL, uaegfx_rom, boardinfo);
		ad->picasso_requested_on = !!(p96_restored_flags & 1);
		vidinfo->picasso_active = ad->picasso_requested_on;
		set_config_changed();
	}
}

uae_u8* restore_p96(uae_u8* src)
{
	struct amigadisplay *ad = &adisplays;
	struct picasso96_state_struct *state = &picasso96_state;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u32 flags;
	int i;

	if (restore_u32() != 2)
		return src;
	InitPicasso96();
	p96_restored_flags = flags = restore_u32();
	init_picasso_screen_called = 0;
	set_gc_called = !!(flags & 2);
	vidinfo->set_panning_called = !!(flags & 4);
	interrupt_enabled = !!(flags & 32);
	changed_prefs.rtgboards[0].rtgmem_size = restore_u32();
	state->Address = restore_u32 ();
	state->RGBFormat = (RGBFTYPE)restore_u32 ();
	state->Width = restore_u16 ();
	state->Height = restore_u16 ();
	state->VirtualWidth = restore_u16 ();
	state->VirtualHeight = restore_u16 ();
	state->XOffset = restore_u16 ();
	state->YOffset = restore_u16 ();
	state->GC_Depth = restore_u8 ();
	state->GC_Flags = restore_u8 ();
	state->BytesPerRow = restore_u16 ();
	state->BytesPerPixel = restore_u8 ();
	uaegfx_base = restore_u32();
	uaegfx_rom = restore_u32();
	boardinfo = restore_u32();
	if (flags & 64) {
		for (i = 0; i < 256; i++) {
			state->CLUT[i].Red = restore_u8 ();
			state->CLUT[i].Green = restore_u8 ();
			state->CLUT[i].Blue = restore_u8 ();
		}
	}
	state->HostAddress = NULL;
	picasso_SetPanningInit(state);
	state->Extent = state->Address + state->BytesPerRow * state->VirtualHeight;
	return src;
}

uae_u8* save_p96(int* len, uae_u8* dstptr)
{
	struct amigadisplay *ad = &adisplays;
	struct picasso96_state_struct *state = &picasso96_state;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo;
	uae_u8 *dstbak, *dst;
	int i;

	if (currprefs.rtgboards[0].rtgmem_size == 0)
		return nullptr;
	if (dstptr)
		dstbak = dst = dstptr;
	else
  	dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32(2);
	save_u32 ((ad->picasso_on ? 1 : 0) | (set_gc_called ? 2 : 0) | (vidinfo->set_panning_called ? 4 : 0) |
    (interrupt_enabled ? 32 : 0) | 64);
	save_u32(currprefs.rtgboards[0].rtgmem_size);
	save_u32 (state->Address);
	save_u32 (state->RGBFormat);
	save_u16 (state->Width);
	save_u16 (state->Height);
	save_u16 (state->VirtualWidth);
	save_u16 (state->VirtualHeight);
	save_u16 (state->XOffset);
	save_u16 (state->YOffset);
	save_u8 (state->GC_Depth);
	save_u8 (state->GC_Flags);
	save_u16 (state->BytesPerRow);
	save_u8 (state->BytesPerPixel);
	save_u32(uaegfx_base);
	save_u32(uaegfx_rom);
	save_u32(boardinfo);
	for (i = 0; i < 256; i++) {
		save_u8 (state->CLUT[i].Red);
		save_u8 (state->CLUT[i].Green);
		save_u8 (state->CLUT[i].Blue);
	}
	*len = dst - dstbak;
	return dstbak;
}

#endif
