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

#include <algorithm>
#include <cstdlib>

#include "uae.h"

#if defined(PICASSO96)

#define MULTIDISPLAY 0
#define WINCURSOR 1
#define NEWTRAP 1

#define USE_OVERLAY 1
#define USE_HARDWARESPRITE 1
#define USE_PALETTESWITCH 1
#define USE_VGASCREENSPLIT 1
#define USE_DACSWITCH 1

#define P96TRACING_ENABLED 0
#define P96TRACING_LEVEL 0
#define P96TRACING_SETUP_ENABLED 0
#define P96SPRTRACING_ENABLED 0

#include "options.h"
#include "threaddep/thread.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "xwin.h"
#include "savestate.h"
#include "autoconf.h"
#include "traps.h"
#include "native2amiga.h"
#include "drawing.h"
#include "inputdevice.h"
#include "debug.h"
#include "registry.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif
#include "picasso96.h"
#include "amiberry_gfx.h"
#include "clipboard.h"
#include "gfxboard.h"
#include "devices.h"
#include "statusline.h"

int debug_rtg_blitter = 3;

#define NOBLITTER (0 || !(debug_rtg_blitter & 1))
#define NOBLITTER_BLIT (0 || !(debug_rtg_blitter & 2))
#define NOBLITTER_ALL 0

#define CURSORMAXWIDTH 64
#define CURSORMAXHEIGHT 64

static int hwsprite = 0;
static int picasso96_BT = BT_uaegfx;
static int picasso96_GCT = GCT_Unknown;
static int picasso96_PCT = PCT_Unknown;

#ifdef _WIN32
int mman_GetWriteWatch (PVOID lpBaseAddress, SIZE_T dwRegionSize, PVOID *lpAddresses, PULONG_PTR lpdwCount, PULONG lpdwGranularity);
void mman_ResetWatch (PVOID lpBaseAddress, SIZE_T dwRegionSize);
#endif

static void picasso_flushpixels(int index, uae_u8 *src, int offset, bool render);
static int createwindowscursor(int monid, int set, int chipset);

int p96refresh_active;
bool have_done_picasso = true; /* For the JIT compiler */
int p96syncrate;
static int p96hsync_counter;

static uae_thread_id render_tid = nullptr;
static smp_comm_pipe *render_pipe = nullptr;
static volatile int render_thread_state;
static uae_sem_t render_cs = nullptr;

#define PICASSO_STATE_SETDISPLAY 1
#define PICASSO_STATE_SETPANNING 2
#define PICASSO_STATE_SETGC 4
#define PICASSO_STATE_SETDAC 8
#define PICASSO_STATE_SETSWITCH 16

#if defined(X86_MSVC_ASSEMBLY)
#define SWAPSPEEDUP
#endif
#ifdef PICASSO96
#ifdef _DEBUG // Change this to _DEBUG for debugging
#define P96TRACING_ENABLED 1
#define P96TRACING_LEVEL 1
#endif
#if P96TRACING_ENABLED
#define P96TRACE(x) do { write_log x; } while(0)
#else
#define P96TRACE(x)
#endif
#if P96TRACING_SETUP_ENABLED
#define P96TRACE_SETUP(x) do { write_log x; } while(0)
#else
#define P96TRACE_SETUP(x)
#endif
#if P96SPRTRACING_ENABLED
#define P96TRACE_SPR(x) do { write_log x; } while(0)
#else
#define P96TRACE_SPR(x)
#endif
#define P96TRACE2(x) do { write_log x; } while(0)

#define TAG_DONE   (0L)		/* terminates array of TagItems. ti_Data unused */
#define TAG_IGNORE (1L)		/* ignore this item, not end of array */
#define TAG_MORE   (2L)		/* ti_Data is pointer to another array of TagItems */
#define TAG_SKIP   (3L)		/* skip this and the next ti_Data items */

static uae_u8 all_ones_bitmap, all_zeros_bitmap; /* yuk */

struct picasso96_state_struct picasso96_state[MAX_AMIGAMONITORS];
struct picasso_vidbuf_description picasso_vidinfo[MAX_AMIGAMONITORS];
static struct PicassoResolution *newmodes;

/* These are the maximum resolutions... They are filled in by GetSupportedResolutions() */
/* have to fill this in, otherwise problems occur on the Amiga side P96 s/w which expects */
/* data here. */
static struct ScreenResolution planar = { 320, 240 };
static struct ScreenResolution chunky = { 640, 480 };
static struct ScreenResolution hicolour = { 640, 480 };
static struct ScreenResolution truecolour = { 640, 480 };
static struct ScreenResolution alphacolour = { 640, 480 };

uae_u32 p96_rgbx16[65536];
uae_u32 p96rc[256], p96gc[256], p96bc[256];

static int newcursor_x, newcursor_y;
static int cursorwidth, cursorheight, cursorok;
static uae_u8 *cursordata;
static uae_u32 cursorrgb[4], cursorrgbn[4];
static int cursordeactivate, setupcursor_needed;
static bool cursorvisible;
#ifdef _WIN32
static HCURSOR wincursor;
#else
SDL_Cursor* p96_cursor;
SDL_Surface* p96_cursor_surface;
#endif
static int wincursor_shown;
static uaecptr boardinfo, ABI_interrupt;
static int interrupt_enabled;
float p96vblank;

static int overlay_src_width_in, overlay_src_height_in;
static int overlay_src_height, overlay_src_width;
static uae_u32 overlay_format, overlay_color, overlay_color_unswapped;
static uae_u32 overlay_modeformat, overlay_modeinfo;
static uae_u32 overlay_brightness;
static int overlay_x, overlay_y;
static int overlay_w, overlay_h;
static int overlay_clipleft, overlay_cliptop;
static int overlay_clipwidth, overlay_clipheight;
static int overlay_pix;
static uaecptr overlay_bitmap, overlay_vram;
static int overlay_vram_offset;
static int overlay_active;
static int overlay_convert;
static int overlay_occlusion;
static struct MyCLUTEntry overlay_clutc[256 * 2];
static uae_u32 overlay_clut[256 * 2];
static uae_u32 *p96_rgbx16_ovl;
static bool delayed_set_switch;

static int uaegfx_old, uaegfx_active;
static uae_u32 reserved_gfxmem;
static uaecptr uaegfx_resname, uaegfx_prefix,
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

static void init_picasso_screen(int);
static uae_u32 p2ctab[256][2];
static int set_gc_called = 0, init_picasso_screen_called = 0;
//fastscreen
static uaecptr oldscr = 0;

extern addrbank gfxmem_bank;
extern addrbank *gfxmem_banks[MAX_RTG_BOARDS];
extern int rtg_index;

void lockrtg()
{
	if (currprefs.rtg_multithread && render_tid && render_cs)
#ifdef _WIN32
		EnterCriticalSection(&render_cs);
#else
		uae_sem_wait(&render_cs);
#endif
}

void unlockrtg()
{
	if (currprefs.rtg_multithread && render_tid && render_cs)
#ifdef _WIN32
		LeaveCriticalSection(&render_cs);
#else
		uae_sem_post(&render_cs);
#endif
}

STATIC_INLINE void endianswap (uae_u32 *vp, int bpp)
{
	uae_u32 v = *vp;
	switch (bpp)
	{
	case 2:
		*vp = uae_bswap_16(v);
		break;
	case 4:
		*vp = uae_bswap_32(v);
		break;
	default: // 1
		break;
	}
}

#if P96TRACING_ENABLED
/*
* Debugging dumps
*/
static void DumpModeInfoStructure(TrapContext *ctx, uaecptr amigamodeinfoptr)
{
	write_log (_T("ModeInfo Structure Dump:\n"));
	write_log (_T("  Node.ln_Succ  = 0x%x\n"), trap_get_long(ctx, amigamodeinfoptr));
	write_log (_T("  Node.ln_Pred  = 0x%x\n"), trap_get_long(ctx, amigamodeinfoptr + 4));
	write_log (_T("  Node.ln_Type  = 0x%x\n"), trap_get_byte(ctx, amigamodeinfoptr + 8));
	write_log (_T("  Node.ln_Pri   = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + 9));
	/*write_log (_T("  Node.ln_Name  = %s\n"), uaememptr->Node.ln_Name); */
	write_log (_T("  OpenCount     = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_OpenCount));
	write_log (_T("  Active        = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + PSSO_ModeInfo_Active));
	write_log (_T("  Width         = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_Width));
	write_log (_T("  Height        = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_Height));
	write_log (_T("  Depth         = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + PSSO_ModeInfo_Depth));
	write_log (_T("  Flags         = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + PSSO_ModeInfo_Flags));
	write_log (_T("  HorTotal      = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_HorTotal));
	write_log (_T("  HorBlankSize  = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_HorBlankSize));
	write_log (_T("  HorSyncStart  = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_HorSyncStart));
	write_log (_T("  HorSyncSize   = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_HorSyncSize));
	write_log (_T("  HorSyncSkew   = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + PSSO_ModeInfo_HorSyncSkew));
	write_log (_T("  HorEnableSkew = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + PSSO_ModeInfo_HorEnableSkew));
	write_log (_T("  VerTotal      = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_VerTotal));
	write_log (_T("  VerBlankSize  = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_VerBlankSize));
	write_log (_T("  VerSyncStart  = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_VerSyncStart));
	write_log (_T("  VerSyncSize   = %d\n"), trap_get_word(ctx, amigamodeinfoptr + PSSO_ModeInfo_VerSyncSize));
	write_log (_T("  Clock         = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + PSSO_ModeInfo_first_union));
	write_log (_T("  ClockDivide   = %d\n"), trap_get_byte(ctx, amigamodeinfoptr + PSSO_ModeInfo_second_union));
	write_log (_T("  PixelClock    = %d\n"), trap_get_long(ctx, amigamodeinfoptr + PSSO_ModeInfo_PixelClock));
}

static void DumpLibResolutionStructure(TrapContext *ctx, uaecptr amigalibresptr)
{
	int i;
	uaecptr amigamodeinfoptr;
	struct LibResolution *uaememptr = (struct LibResolution *)get_mem_bank(amigalibresptr).xlateaddr(amigalibresptr);

	write_log (_T("LibResolution Structure Dump:\n"));

	if (trap_get_long(ctx, amigalibresptr + PSSO_LibResolution_DisplayID) == 0xFFFFFFFF) {
		write_log (_T("  Finished With LibResolutions...\n"));
	} else {
		write_log (_T("  Name      = %s\n"), uaememptr->P96ID);
		write_log (_T("  DisplayID = 0x%x\n"), trap_get_long(ctx, amigalibresptr + PSSO_LibResolution_DisplayID));
		write_log (_T("  Width     = %d\n"), trap_get_word(ctx, amigalibresptr + PSSO_LibResolution_Width));
		write_log (_T("  Height    = %d\n"), trap_get_word(ctx, amigalibresptr + PSSO_LibResolution_Height));
		write_log (_T("  Flags     = %d\n"), trap_get_word(ctx, amigalibresptr + PSSO_LibResolution_Flags));
		for (i = 0; i < MAXMODES; i++) {
			amigamodeinfoptr = trap_get_long(ctx, amigalibresptr + PSSO_LibResolution_Modes + i*4);
			write_log (_T("  ModeInfo[%d] = 0x%x\n"), i, amigamodeinfoptr);
			if (amigamodeinfoptr)
				DumpModeInfoStructure(ctx, amigamodeinfoptr);
		}
		write_log (_T("  BoardInfo = 0x%x\n"), trap_get_long(ctx, amigalibresptr + PSSO_LibResolution_BoardInfo));
	}
}

static TCHAR binary_byte[9] = { 0,0,0,0,0,0,0,0,0 };

static TCHAR *BuildBinaryString (uae_u8 value)
{
	int i;
	for (i = 0; i < 8; i++) {
		binary_byte[i] = (value & (1 << (7 - i))) ? '#' : '.';
	}
	return binary_byte;
}

static void DumpPattern (struct Pattern *patt)
{
	uae_u8 *mem;
	int row, col;

	if (!patt->Memory)
		return;

	for (row = 0; row < (1 << patt->Size); row++) {
		mem = patt->Memory + row * 2;
		for (col = 0; col < 2; col++) {
			write_log (_T("%s "), BuildBinaryString (*mem++));
		}
		write_log (_T("\n"));
	}
}

static void DumpTemplate (struct Template *tmp, uae_u32 w, uae_u32 h)
{
	uae_u8 *mem = tmp->Memory;
	unsigned int row, col, width;
	
	if (!mem)
		return;
	width = (w + 7) >> 3;
	write_log (_T("xoffset = %d, bpr = %d\n"), tmp->XOffset, tmp->BytesPerRow);
	for (row = 0; row < h; row++) {
		mem = tmp->Memory + row * tmp->BytesPerRow;
		for (col = 0; col < width; col++) {
			write_log (_T("%s "), BuildBinaryString (*mem++));
		}
		write_log (_T("\n"));
	}
}

static void DumpLine(struct Line *line)
{
	if (line) {
		write_log (_T("Line->X = %d\n"), line->X);
		write_log (_T("Line->Y = %d\n"), line->Y);
		write_log (_T("Line->Length = %d\n"), line->Length);
		write_log (_T("Line->dX = %d\n"), line->dX);
		write_log (_T("Line->dY = %d\n"), line->dY);
		write_log (_T("Line->sDelta = %d\n"), line->sDelta);
		write_log (_T("Line->lDelta = %d\n"), line->lDelta);
		write_log (_T("Line->twoSDminusLD = %d\n"), line->twoSDminusLD);
		write_log (_T("Line->LinePtrn = %d\n"), line->LinePtrn);
		write_log (_T("Line->PatternShift = %d\n"), line->PatternShift);
		write_log (_T("Line->FgPen = 0x%x\n"), line->FgPen);
		write_log (_T("Line->BgPen = 0x%x\n"), line->BgPen);
		write_log (_T("Line->Horizontal = %d\n"), line->Horizontal);
		write_log (_T("Line->DrawMode = %d\n"), line->DrawMode);
		write_log (_T("Line->Xorigin = %d\n"), line->Xorigin);
		write_log (_T("Line->Yorigin = %d\n"), line->Yorigin);
	}
}

static void ShowSupportedResolutions (void)
{
	int i = 0;

	write_log (_T("-----------------\n"));
	while (newmodes[i].depth >= 0) {
		write_log (_T("%s\n"), newmodes[i].name);
		i++;
	}
	write_log (_T("-----------------\n"));
}

#endif

static void **gwwbuf[MAX_RTG_BOARDS];
static int gwwbufsize[MAX_RTG_BOARDS], gwwpagesize[MAX_RTG_BOARDS], gwwpagemask[MAX_RTG_BOARDS];
extern uae_u8 *natmem_offset;

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
	default: // RGBFB_NONE
		return 0;
	}
	return 0;
}

static constexpr uae_u32 rgbfmasks[] =
{
	0x00000000, // RGBFF_NONE
	0xffffffff, // RGBFF_CLUT
	0x00ffffff, // RGBFF_R8G8B8
	0x00ffffff, // RGBFB_B8G8R8
	0xffffffff, // RGBFB_R5G6B5PC
	0x7fff7fff, // RGBFB_R5G6B5PC
	0xffffff00, // RGBFB_A8R8G8B8
	0xffffff00, // RGBFB_A8B8G8R8
	0x00ffffff, // RGBFB_R8G8B8A8
	0x00ffffff, // RGBFB_B8G8R8A8
	0xffffffff, // RGBFB_R5G6B5
	0xff7fff7f, // RGBFB_R5G5B5
	0xffffffff, // RGBFB_B5G6R5PC
	0x7fff7fff, // RGBFB_B5G5R5PC
	0xffffffff, // RGBFB_Y4U2V2
	0xffffffff  // RGBFB_Y4U1V1
};

static bool validatecoords2(TrapContext *ctx, const struct RenderInfo *ri, uae_u8 RGBFmt, const uae_u32 *Xp, const uae_u32 *Yp, uae_u32 *Widthp, const uae_u32 *Heightp)
{
	uae_u32 Width = *Widthp;
	const uae_u32 Height = *Heightp;
	const uae_u32 X = *Xp;
	const uae_u32 Y = *Yp;
	if (!Width || !Height) {
		return true;
	}
	if (Width > 32767 || Height > 32767 || X > 32767 || Y > 32767) {
		return false;
	}
	if (ri) {
		int bpr = ri->BytesPerRow;
		if (bpr < 0) {
			return false;
		}
		const int bpp = GetBytesPerPixel(RGBFmt);
		// zero is allowed (repeats same line * height)
		if (!bpr) {
			if (X) {
				return false;
			}
			bpr = Width * bpp;
		}
		if (X * bpp >= bpr) {
			return false;
		}
		uae_u32 X2 = X + Width;
		if (X2 * bpp > bpr) {
			X2 = bpr / bpp;
			Width = X2 - X;
			*Widthp = Width;
		}
		const uaecptr start = gfxmem_banks[0]->start;
		const uae_u32 size = gfxmem_banks[0]->allocated_size;
		uaecptr mem = ri->AMemory;
		if (mem < start || mem >= start + size) {
			return false;
		}
		mem += (Y + Height - 1) * ri->BytesPerRow + (X + Width - 1) * bpp;
		if (mem < start || mem >= start + size) {
			return false;
		}
	}
	return true;
}
static bool validatecoords(TrapContext *ctx, const struct RenderInfo *ri, uae_u8 RGBFmt, const uae_u32 *X, const uae_u32 *Y, uae_u32 *Width, const uae_u32 *Height)
{
	if (validatecoords2(ctx, ri, RGBFmt, X, Y, Width, Height))
		return true;
	write_log (_T("RTG invalid region: %08X:%d:%d (%dx%d)-(%dx%d)\n"), ri->AMemory, ri->BytesPerRow, ri->RGBFormat, *X, *Y, *Width, *Height);
	return false;
}

/*
* Amiga <-> native structure conversion functions
*/

static int CopyRenderInfoStructureA2U(TrapContext *ctx, uaecptr amigamemptr, struct RenderInfo *ri)
{
	if (trap_valid_address(ctx, amigamemptr, PSSO_RenderInfo_sizeof)) {
		struct trapmd md[] =
		{
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_RenderInfo_Memory } },
			{ TRAPCMD_GET_WORD, { amigamemptr + PSSO_RenderInfo_BytesPerRow } },
			{ TRAPCMD_GET_LONG, { amigamemptr + PSSO_RenderInfo_RGBFormat } }
		};
		trap_multi(ctx, md, sizeof md / sizeof(struct trapmd));
		const uaecptr memp = md[0].params[0];
		ri->AMemory = memp;
		ri->BytesPerRow = static_cast<uae_s16>(md[1].params[0]);
		ri->RGBFormat = static_cast<RGBFTYPE>(md[2].params[0]);
		// Can't really validate this better at this point, no height.
		if (trap_valid_address(ctx, memp, ri->BytesPerRow)) {
			ri->Memory = get_real_address(memp);
			return 1;
		}
	}
	write_log (_T("ERROR - Invalid RenderInfo memory area...\n"));
	return 0;
}

static int CopyPatternStructureA2U(TrapContext *ctx, uaecptr amigamemptr, struct Pattern *pattern)
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
		const uaecptr memp = md[0].params[0];
		pattern->AMemory = memp;
		pattern->XOffset = md[1].params[0];
		pattern->YOffset = md[2].params[0];
		pattern->FgPen = md[3].params[0];
		pattern->BgPen = md[4].params[0];
		pattern->Size = md[5].params[0];
		pattern->DrawMode = md[6].params[0];
		if (trap_valid_address(ctx, memp, 2)) {
			if (trap_is_indirect())
				pattern->Memory = nullptr;
			else
				pattern->Memory = get_real_address(memp);
			return 1;
		}
	}
	write_log (_T("ERROR - Invalid Pattern memory area...\n"));
	return 0;
}

static int CopyBitMapStructureA2U(TrapContext *ctx, uaecptr amigamemptr, struct BitMap *bm)
{
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
	bm->Depth = std::min<int>(bm->Depth, 8);

	for (int i = 0; i < bm->Depth; i++) {
		const uaecptr plane = md[4 + i].params[0];
		bm->APlanes[i] = plane;
		switch (plane) {
		case 0x00000000:
			bm->Planes[i] = &all_zeros_bitmap;
			break;
		case 0xFFFFFFFF:
			bm->Planes[i] = &all_ones_bitmap;
			break;
		default:
			if (!trap_is_indirect() && trap_valid_address(ctx, plane, bm->BytesPerRow * bm->Rows))
				bm->Planes[i] = get_real_address(plane);
			else
				bm->Planes[i] = &all_zeros_bitmap;
			break;
		}
	}
	return 1;
}

static int CopyTemplateStructureA2U(TrapContext *ctx, uaecptr amigamemptr, struct Template *tmpl)
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

		const uaecptr memp = md[0].params[0];
		if (trap_is_indirect()) {
			tmpl->Memory = nullptr;
		} else {
			if (!trap_valid_address(ctx, memp, 1)) {
				write_log(_T("ERROR - Invalid Template memory region %08x...\n"), memp);
				return 0;
			}
			tmpl->Memory = get_real_address(memp);
		}
		tmpl->AMemory = memp;
		tmpl->BytesPerRow = static_cast<uae_s16>(md[1].params[0]);
		tmpl->XOffset = md[2].params[0];
		tmpl->DrawMode = md[3].params[0];
		tmpl->FgPen = md[4].params[0];
		tmpl->BgPen = md[5].params[0];
		return 1;
	}
	write_log (_T("ERROR - Invalid Template memory area...\n"));
	return 0;
}

static int CopyLineStructureA2U(TrapContext *ctx, uaecptr amigamemptr, struct Line *line)
{
	if (trap_valid_address(ctx, amigamemptr, sizeof(struct Line))) {
		line->X = trap_get_word(ctx, amigamemptr + PSSO_Line_X);
		line->Y = trap_get_word(ctx, amigamemptr + PSSO_Line_Y);
		line->Length = trap_get_word(ctx, amigamemptr + PSSO_Line_Length);
		line->dX = static_cast<uae_s16>(trap_get_word(ctx, amigamemptr + PSSO_Line_dX));
		line->dY = static_cast<uae_s16>(trap_get_word(ctx, amigamemptr + PSSO_Line_dY));
		line->lDelta = static_cast<uae_s16>(trap_get_word(ctx, amigamemptr + PSSO_Line_lDelta));
		line->sDelta = static_cast<uae_s16>(trap_get_word(ctx, amigamemptr + PSSO_Line_sDelta));
		line->twoSDminusLD = static_cast<uae_s16>(trap_get_word(ctx, amigamemptr + PSSO_Line_twoSDminusLD));
		line->LinePtrn = trap_get_word(ctx, amigamemptr + PSSO_Line_LinePtrn);
		line->PatternShift = trap_get_word(ctx, amigamemptr + PSSO_Line_PatternShift);
		line->FgPen = trap_get_long(ctx, amigamemptr + PSSO_Line_FgPen);
		line->BgPen = trap_get_long(ctx, amigamemptr + PSSO_Line_BgPen);
		line->Horizontal = trap_get_word(ctx, amigamemptr + PSSO_Line_Horizontal);
		line->DrawMode = trap_get_byte(ctx, amigamemptr + PSSO_Line_DrawMode);
		line->Xorigin = trap_get_word(ctx, amigamemptr + PSSO_Line_Xorigin);
		line->Yorigin = trap_get_word(ctx, amigamemptr + PSSO_Line_Yorigin);
		return 1;
	}
	write_log (_T("ERROR - Invalid Line structure...\n"));
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
static void do_fillrect_frame_buffer(const struct RenderInfo *ri, int X, int Y, int Width, int Height, uae_u32 Pen, int Bpp)
{
	int cols;
	uae_u8 *dst;
	const int bpr = ri->BytesPerRow;

	dst = ri->Memory + X * Bpp + Y * bpr;
	endianswap (&Pen, Bpp);
	switch (Bpp)
	{
	case 1:
		for (int lines = 0; lines < Height; lines++, dst += bpr) {
			memset (dst, static_cast<int>(Pen), Width);
		}
	break;
	case 2:
	{
		Pen |= Pen << 16;
		for (int lines = 0; lines < Height; lines++, dst += bpr) {
			auto p = reinterpret_cast<uae_u32*>(dst);
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
				reinterpret_cast<uae_u16*>(p)[0] = Pen;
			}
		}
	}
	break;
	case 3:
	{
		const uae_u16 Pen1 = Pen & 0xffff;
		const uae_u16 Pen2 = (Pen << 8) | ((Pen >> 16) & 0xff);
		const uae_u16 Pen3 = Pen >> 8;
		const bool same = (Pen & 0xff) == ((Pen >> 8) & 0xff) && (Pen & 0xff) == ((Pen >> 16) & 0xff);
		for (int lines = 0; lines < Height; lines++, dst += bpr) {
			auto *p = reinterpret_cast<uae_u16*>(dst);
			if (same) {
				memset(p, static_cast<int>(Pen) & 0xff, Width * 3);
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
				auto p8 = reinterpret_cast<uae_u8*>(p);
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
			auto p = reinterpret_cast<uae_u32*>(dst);
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
	default:
		break;
	}
}

static void setupcursor()
{
#ifdef AMIBERRY
	struct rtgboardconfig *rbc = &currprefs.rtgboards[0];

	if (rbc->rtgmem_type >= GFXBOARD_HARDWARE)
		return;

	gfx_lock ();
	setupcursor_needed = 1;
	if (cursordata && cursorwidth && cursorheight) {
		p96_cursor_surface = SDL_CreateRGBSurfaceWithFormat(0, cursorwidth, cursorheight, 32, SDL_PIXELFORMAT_BGRA32);

		for (int y = 0; y < cursorheight; y++) {
			uae_u8 *p1 = cursordata + cursorwidth * y;
			auto *p2 = reinterpret_cast<uae_u32*>(static_cast<Uint8*>(p96_cursor_surface->pixels) + p96_cursor_surface->pitch * y);
			for (int x = 0; x < cursorwidth; x++) {
				uae_u8 c = *p1++;
				if (c < 4) {
					*p2 = cursorrgbn[c];
				}
				p2++;
			}
		}

		auto* p96_formatted_cursor_surface = SDL_ConvertSurfaceFormat(p96_cursor_surface, SDL_PIXELFORMAT_BGRA32, 0);
		if (p96_formatted_cursor_surface != nullptr) {
			SDL_FreeSurface(p96_cursor_surface);
			if (p96_cursor != nullptr) {
				SDL_FreeCursor(p96_cursor);
				p96_cursor = nullptr;
			}
			p96_cursor = SDL_CreateColorCursor(p96_formatted_cursor_surface, 0, 0);
			SDL_FreeSurface(p96_formatted_cursor_surface);

			SDL_SetCursor(p96_cursor);
			setupcursor_needed = 0;
			P96TRACE_SPR((_T("cursorsurface3d updated\n")));
		}
	} else {
		P96TRACE_SPR((_T("cursorsurfaced3d LockRect() failed %08x\n"), hr));
	}
	gfx_unlock ();
#else
	uae_u8 *dptr;
	int bpp = 4;
	int pitch;
	struct rtgboardconfig *rbc = &currprefs.rtgboards[0];

	if (rbc->rtgmem_type >= GFXBOARD_HARDWARE || D3D_setcursor == NULL)
		return;
	gfx_lock ();
	setupcursor_needed = 1;
	dptr = D3D_setcursorsurface(rbc->monitor_id, false, &pitch);
	if (dptr) {
		for (int y = 0; y < CURSORMAXHEIGHT; y++) {
			uae_u8 *p2 = dptr + pitch * y;
			memset(p2, 0, CURSORMAXWIDTH * bpp);
		}
		if (cursordata && cursorwidth && cursorheight) {
			for (int y = 0; y < cursorheight; y++) {
				uae_u8 *p1 = cursordata + cursorwidth * y;
				uae_u32 *p2 = (uae_u32*)(dptr + pitch * y);
				for (int x = 0; x < cursorwidth; x++) {
					uae_u8 c = *p1++;
					if (c < 4) {
						*p2 = cursorrgbn[c];
					}
					p2++;
				}
			}
		}
		D3D_setcursorsurface(rbc->monitor_id, false, NULL);
		setupcursor_needed = 0;
		P96TRACE_SPR((_T("cursorsurface3d updated\n")));
	} else {
		P96TRACE_SPR((_T("cursorsurfaced3d LockRect() failed %08x\n"), hr));
	}
	gfx_unlock ();
#endif
}

static void disablemouse ()
{
	cursorok = FALSE;
	cursordeactivate = 0;
	if (!hwsprite)
		return;
#ifdef AMIBERRY
	if (p96_cursor)
		SDL_FreeCursor(p96_cursor);
#else
	D3D_setcursor(0, 0, 0, 0, 0, 0, 0, false, true);
#endif
}

static void mouseupdate(struct AmigaMonitor *mon)
{
	struct picasso96_state_struct *state = &picasso96_state[mon->monitor_id];
	int x = newcursor_x;
	int y = newcursor_y;
	float mx = currprefs.gf[GF_RTG].gfx_filter_horiz_zoom_mult;
	float my = currprefs.gf[GF_RTG].gfx_filter_vert_zoom_mult;
	int forced = 0;

	if (!hwsprite)
		return;
	if (cursordeactivate > 0) {
		cursordeactivate--;
		if (cursordeactivate == 0) {
			disablemouse ();
			cursorvisible = false;
		}
	}

#ifdef AMIBERRY
	if (p96_cursor == nullptr)
		setupcursor_needed = 1;
#else
	if (D3D_setcursor) {
		if (!D3D_setcursorsurface(mon->monitor_id, true, NULL)) {
			setupcursor_needed = 1;
		}
		if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER) {
			D3D_setcursor(mon->monitor_id, x, y, WIN32GFX_GetWidth(mon), WIN32GFX_GetHeight(mon), mx, my, cursorvisible, mon->scalepicasso == 2);
		} else {
			D3D_setcursor(mon->monitor_id, x, y, state->Width, state->Height, mx, my, cursorvisible, false);
		}
	}
#endif
}

static int p96_framecnt;
int p96skipmode = -1;
static int doskip ()
{
	if (p96_framecnt >= currprefs.gfx_framerate)
		p96_framecnt = 0;
	return p96_framecnt > 0;
}

void picasso_trigger_vblank()
{
	TrapContext *ctx = nullptr;
	if (!ABI_interrupt || !uaegfx_base || !interrupt_enabled || !currprefs.rtg_hardwareinterrupt)
		return;
	trap_put_long(ctx, uaegfx_base + CARD_IRQPTR, ABI_interrupt + PSSO_BoardInfo_SoftInterrupt);
	trap_put_byte(ctx, uaegfx_base + CARD_IRQFLAG, 1);
	if (currprefs.rtgvblankrate != 0)
		INTREQ (0x8000 | 0x0008);
}

static bool is_uaegfx_active()
{
	if (currprefs.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE || !currprefs.rtgboards[0].rtgmem_size)
		return false;
	return true;
}

static void rtg_render()
{
	int monid = currprefs.rtgboards[0].monitor_id;
	const bool uaegfx_active = is_uaegfx_active();
	const int uaegfx_index = 0;
	const struct AmigaMonitor *mon = &AMonitors[monid];
	const struct picasso96_state_struct *state = &picasso96_state[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	struct amigadisplay *ad = &adisplays[monid];

#ifdef _WIN32
	if (D3D_restore)
		D3D_restore(monid, true);
#endif
	if (doskip () && p96skipmode == 0) {
		;
	} else {
		const bool full = vidinfo->full_refresh > 0;
		if (uaegfx_active) {
			if (!currprefs.rtg_multithread) {
				picasso_flushpixels(0, gfxmem_banks[uaegfx_index]->start + natmem_offset, static_cast<int>(state->XYOffset - gfxmem_banks[uaegfx_index]->start), true);
			}
		} else {
			vidinfo->full_refresh = std::max(vidinfo->full_refresh, 0);
			if (vidinfo->full_refresh > 0)
				vidinfo->full_refresh--;
		}
#ifdef GFXBOARD
		gfxboard_vsync_handler(full, true);
#endif
		if (currprefs.rtg_multithread && uaegfx_active && quit_program == 0) {
			if (ad->pending_render) {
				ad->pending_render = false;
				gfx_unlock_picasso(mon->monitor_id, true);
			}
			write_comm_pipe_int(render_pipe, uaegfx_index, 0);
		}
	}
}
static void rtg_clear(int monid)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	vidinfo->rtg_clear_flag = 2;
}

enum {

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

int getconvert(int rgbformat, int pixbytes)
{
	int v = 0;
	const int d = pixbytes;

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
	default: // RGBFB_R5G6B5PC
		if (d == 2)
			v = RGBFB_R5G6B5PC_16;
		else if (d == 4)
			v = RGBFB_R5G6B5PC_32;
		break;
	}
	return v;
}

static void setconvert(int monid)
{
	lockrtg();
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	struct picasso96_state_struct *state = &picasso96_state[monid];

	if (state->advDragging) {
		vidinfo->picasso_convert[0] = getconvert(static_cast<int>(vidinfo->dacrgbformat[0]), picasso_vidinfo[monid].pixbytes);
		vidinfo->picasso_convert[1] = getconvert(static_cast<int>(vidinfo->dacrgbformat[1]), picasso_vidinfo[monid].pixbytes);
	} else {
		vidinfo->picasso_convert[0] = vidinfo->picasso_convert[1] = getconvert(state->RGBFormat, picasso_vidinfo[monid].pixbytes);
	}
	vidinfo->host_mode = picasso_vidinfo[monid].pixbytes == 4 ? RGBFB_B8G8R8A8 : RGBFB_B5G6R5PC;
	if (picasso_vidinfo[monid].pixbytes == 4)
		alloc_colors_rgb(8, 8, 8, 16, 8, 0, 0, 0, 0, 0, p96rc, p96gc, p96bc); // BGRA
	else
		alloc_colors_rgb(5, 6, 5, 11, 5, 0, 0, 0, 0, 0, p96rc, p96gc, p96bc); // BGR
	gfx_set_picasso_colors(monid, state->RGBFormat);
	picasso_palette(state->CLUT, vidinfo->clut);
	if (vidinfo->host_mode != vidinfo->ohost_mode || state->RGBFormat != vidinfo->orgbformat) {
		write_log (_T("RTG conversion: Depth=%d HostRGBF=%d P96RGBF=%d Mode=%d/%d\n"),
			picasso_vidinfo[monid].pixbytes, vidinfo->host_mode, state->RGBFormat, vidinfo->picasso_convert[0], vidinfo->picasso_convert[1]);
		if (vidinfo->host_mode != vidinfo->ohost_mode && isfullscreen() > 0 && currprefs.rtgmatchdepth) {
			state->ModeChanged = true;
		}
		vidinfo->ohost_mode = vidinfo->host_mode;
		vidinfo->orgbformat = state->RGBFormat;
	}
	vidinfo->full_refresh = 1;
	setupcursor_needed = 1;
	unlockrtg();
}

bool picasso_is_active(int monid)
{
	const struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	return vidinfo->picasso_active;
}

/* Clear our screen, since we've got a new Picasso screen-mode, and refresh with the proper contents
* This is called on several occasions:
* 1. Amiga-->Picasso transition, via SetSwitch()
* 2. Picasso-->Picasso transition, via SetPanning().
* 3. whenever the graphics code notifies us that the screen contents have been lost.
*/
void picasso_refresh(int monid)
{
	struct RenderInfo ri{};
	struct AmigaMonitor *mon = &AMonitors[monid];
	const struct amigadisplay *ad = &adisplays[monid];
	const struct picasso96_state_struct *state = &picasso96_state[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];

	if (!ad->picasso_on)
		return;
	lockrtg();
	vidinfo->full_refresh = 1;
	setconvert(monid);
	setupcursor();
	rtg_clear(monid);

	if (currprefs.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE) {
#ifdef GFXBOARD
		gfxboard_refresh(monid);
#endif
		unlockrtg();
		return;
	}

	/* Make sure that the first time we show a Picasso video mode, we don't blit any crap.
	* We can do this by checking if we have an Address yet.
	*/
	if (state->Address) {
		unsigned int width, height;

		/* blit the stuff from our static frame-buffer to the gfx-card */
		ri.Memory = gfxmem_bank.baseaddr + (state->Address - gfxmem_bank.start);
		ri.BytesPerRow = static_cast<uae_s16>(state->BytesPerRow);
		ri.RGBFormat = state->RGBFormat;

		if (vidinfo->set_panning_called) {
			width = (state->VirtualWidth < state->Width) ?
				state->VirtualWidth : state->Width;
			height = (state->VirtualHeight < state->Height) ?
				state->VirtualHeight : state->Height;
		} else {
			width = state->Width;
			height = state->Height;
		}
	} else {
		write_log (_T("ERROR - picasso_refresh() can't refresh!\n"));
	}
	unlockrtg();
}

static void picasso_handle_vsync2(struct AmigaMonitor *mon)
{
	int monid = mon->monitor_id;
	struct amigadisplay *ad = &adisplays[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const struct picasso96_state_struct *p96state = &picasso96_state[monid];
	static int vsynccnt;
	int thisisvsync = 1;
	int vsync = isvsync_rtg();
	int mult;
	bool rendered = false;
	const bool uaegfx = currprefs.rtgboards[0].rtgmem_type < GFXBOARD_HARDWARE;
	const bool uaegfx_active = is_uaegfx_active();

	const int state = vidinfo->picasso_state_change;
	if (state)
		lockrtg();
	if (state & PICASSO_STATE_SETDAC) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETDAC);
		if (p96state->advDragging) {
			vidinfo->picasso_convert[0] = getconvert(static_cast<int>(vidinfo->dacrgbformat[0]), picasso_vidinfo[monid].pixbytes);
			vidinfo->picasso_convert[1] = getconvert(static_cast<int>(vidinfo->dacrgbformat[1]), picasso_vidinfo[monid].pixbytes);
		}
		rtg_clear(mon->monitor_id);
	}
	if (state & PICASSO_STATE_SETGC) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETGC);
		set_gc_called = 1;
		vidinfo->picasso_changed = true;
		init_picasso_screen(monid);
		init_hz_p96(monid);
		if (delayed_set_switch) {
			delayed_set_switch = false;
			atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETSWITCH);
			ad->picasso_requested_on = true;
			set_config_changed();
		}
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
		vidinfo->full_refresh = 1;
		vidinfo->set_panning_called = 1;
		init_picasso_screen(monid);
		vidinfo->set_panning_called = 0;
	}
	if (state & PICASSO_STATE_SETDISPLAY) {
		atomic_and(&vidinfo->picasso_state_change, ~PICASSO_STATE_SETDISPLAY);
		// do nothing
	}
	if (state)
		unlockrtg();

	if (ad->picasso_on) {
#if 0
		if (vsync < 0) {
			vsync_busywait_end(NULL);
			vsync_busywait_do(NULL, false, false);
		}
#endif
	}

	getvsyncrate(monid, (float)currprefs.chipset_refreshrate, &mult);
	if (vsync && mult < 0) {
		vsynccnt++;
		if (vsynccnt < 2)
			thisisvsync = 0;
		else
			vsynccnt = 0;
	}

	p96_framecnt++;

	if (!uaegfx && !ad->picasso_on) {
		rtg_render();
		return;
	}

	if (!ad->picasso_on)
		return;

	if (uaegfx && uaegfx_active) {
		if (setupcursor_needed)
			setupcursor();
		mouseupdate(mon);
	}

	if (thisisvsync) {
		rtg_render();
#ifdef AVIOUTPUT
		frame_drawn(monid);
#endif
	}

	if (uaegfx) {
		if (thisisvsync)
			picasso_trigger_vblank();
	}

#if 0
	if (vsync < 0) {
		vsync_busywait_start();
	}
#endif

}

static int p96hsync;

void picasso_handle_vsync()
{
	struct AmigaMonitor *mon = &AMonitors[currprefs.rtgboards[0].monitor_id];
	const struct amigadisplay *ad = &adisplays[currprefs.rtgboards[0].monitor_id];
	const bool uaegfx = currprefs.rtgboards[0].rtgmem_type < GFXBOARD_HARDWARE;
	const bool uaegfx_active = is_uaegfx_active();

	if (currprefs.rtgboards[0].rtgmem_size == 0)
		return;

	if (!ad->picasso_on && uaegfx) {
		if (uaegfx_active) {
			createwindowscursor(mon->monitor_id, 0, 1);
		}
		picasso_trigger_vblank();
		if (!delayed_set_switch)
			return;
	}

	const int vsync = isvsync_rtg();
	if (vsync < 0) {
		p96hsync = 0;
		picasso_handle_vsync2(mon);
	} else if (currprefs.rtgvblankrate == 0) {
		picasso_handle_vsync2(mon);
	}
}

static void picasso_handle_hsync()
{
	struct AmigaMonitor *mon = &AMonitors[currprefs.rtgboards[0].monitor_id];
	struct amigadisplay *ad = &adisplays[currprefs.rtgboards[0].monitor_id];
	const bool uaegfx = currprefs.rtgboards[0].rtgmem_type < GFXBOARD_HARDWARE;
	const bool uaegfx_active = is_uaegfx_active();

	if (currprefs.rtgboards[0].rtgmem_size == 0)
		return;

	if (ad->pending_render) {
		ad->pending_render = false;
		gfx_unlock_picasso(mon->monitor_id, true);
	}

	const int vsync = isvsync_rtg();
	if (vsync < 0) {
		p96hsync++;
		if (p96hsync >= p96syncrate * 3) {
			p96hsync = 0;
			// kickstart vblank vsync_busywait stuff
			picasso_handle_vsync();
		}
		return;
	}

	if (currprefs.rtgvblankrate == 0)
		return;

	p96hsync++;
	if (p96hsync >= p96syncrate) {
		if (!ad->picasso_on) {
			if (uaegfx) {
				if (uaegfx_active) {
					createwindowscursor(mon->monitor_id, 0, 1);
				}
				picasso_trigger_vblank();
			}
#ifdef GFXBOARD
			gfxboard_vsync_handler(false, false);
#endif
		} else {
			picasso_handle_vsync2(mon);
		}
		p96hsync = 0;
	}
}

#define BLT_SIZE 4
#define BLT_MULT 1
#define BLT_NAME BLIT_FALSE_32
#define BLT_FUNC(s,d) *d = 0
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_32
#define BLT_FUNC(s,d) *d = ~((*s) | (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_32
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_32
#define BLT_FUNC(s,d) *d = ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_32
#define BLT_FUNC(s,d) *d = (*s) & ((~(*d)) & rgbmask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_32
#define BLT_FUNC(s,d) *d = (~(*d)) & rgbmask
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
#define BLT_FUNC(s,d) *d = ((~(*d)) & rgbmask) | (*s)
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
#define BLT_FUNC(s,d) { uae_u16 tmp = *d ; *d = *s; *s = tmp; }
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define BLT_SIZE 3
#define BLT_MULT 1
#define BLT_NAME BLIT_FALSE_24
#define BLT_FUNC(s,d) *d = 0
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_24
#define BLT_FUNC(s,d) *d = ~((*s) | (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_24
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_24
#define BLT_FUNC(s,d) *d = ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_24
#define BLT_FUNC(s,d) *d = (*s) & (~(*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_24
#define BLT_FUNC(s,d) *d = (~(*d))
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
#define BLT_FUNC(s,d) *d = (~(*d)) | (*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_OR_24
#define BLT_FUNC(s,d) *d = (*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_TRUE_24
#define BLT_FUNC(s,d) *d = 0xffffffff
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SWAP_24
#define BLT_FUNC(s,d) { uae_u32 tmp = *d; *d = *s; *s = tmp; }
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define BLT_SIZE 2
#define BLT_MULT 2
#define BLT_NAME BLIT_FALSE_16
#define BLT_FUNC(s,d) *d = 0
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_16
#define BLT_FUNC(s,d) *d = ~((*s) | (*d))
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_16
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_16
#define BLT_FUNC(s,d) *d = ~(*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_16
#define BLT_FUNC(s,d) *d = (*s) & ((~(*d)) & rgbmask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_16
#define BLT_FUNC(s,d) *d = ((~(*d)) & rgbmask)
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
#define BLT_FUNC(s,d) *d = ((~(*d)) & rgbmask) | (*s)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_OR_16
#define BLT_FUNC(s,d) *d = (*s) | (*d)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_TRUE_16
#define BLT_FUNC(s,d) *d = 0xffff
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SWAP_16
#define BLT_FUNC(s,d) { uae_u16 tmp = *d; *d = *s; *s = tmp; }
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define BLT_SIZE 1
#define BLT_MULT 4
#define BLT_NAME BLIT_FALSE_8
#define BLT_NAME_MASK BLIT_FALSE_MASK_8
#define BLT_FUNC(s,d) *d = 0
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((0) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOR_8
#define BLT_NAME_MASK BLIT_NOR_MASK_8
#define BLT_FUNC(s,d) *d = ~((*s) | (*d))
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((~((*s) | (*d))) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYDST_8
#define BLT_NAME_MASK BLIT_ONLYDST_MASK_8
#define BLT_FUNC(s,d) *d = (*d) & ~(*s)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | (((*d) & ~(*s)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTSRC_8
#define BLT_NAME_MASK BLIT_NOTSRC_MASK_8
#define BLT_FUNC(s,d) *d = ~(*s)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((~(*s)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_ONLYSRC_8
#define BLT_NAME_MASK BLIT_ONLYSRC_MASK_8
#define BLT_FUNC(s,d) *d = (*s) & ~(*d)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | (((*s) & ~(*d)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTDST_8
#define BLT_NAME_MASK BLIT_NOTDST_MASK_8
#define BLT_FUNC(s,d) *d = ~(*d)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((~(*d)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_EOR_8
#define BLT_NAME_MASK BLIT_EOR_MASK_8
#define BLT_FUNC(s,d) *d = (*s) ^ (*d)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | (((*s) ^ (*d)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NAND_8
#define BLT_NAME_MASK BLIT_NAND_MASK_8
#define BLT_FUNC(s,d) *d = ~((*s) & (*d))
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((~((*s) & (*d))) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_AND_8
#define BLT_NAME_MASK BLIT_AND_MASK_8
#define BLT_FUNC(s,d) *d = (*s) & (*d)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | (((*s) & (*d)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NEOR_8
#define BLT_NAME_MASK BLIT_NEOR_MASK_8
#define BLT_FUNC(s,d) *d = ~((*s) ^ (*d))
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((~((*s) ^ (*d))) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYSRC_8
#define BLT_NAME_MASK BLIT_NOTONLYSRC_MASK_8
#define BLT_FUNC(s,d) *d = ~(*s) | (*d)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((~(*s) | (*d)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SRC_8
#define BLT_NAME_MASK BLIT_SRC_MASK_8
#define BLT_FUNC(s,d) *d = *s
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((*s) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_NOTONLYDST_8
#define BLT_NAME_MASK BLIT_NOTONLYDST_MASK_8
#define BLT_FUNC(s,d) *d = ~(*d) | (*s)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((~(*d) | (*s)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_OR_8
#define BLT_NAME_MASK BLIT_OR_MASK_8
#define BLT_FUNC(s,d) *d = (*s) | (*d)
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | (((*s) | (*d)) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_TRUE_8
#define BLT_NAME_MASK BLIT_TRUE_MASK_8
#define BLT_FUNC(s,d) *d = 0xff
#define BLT_FUNC_MASK(s,d,mask) *d = ((*d) & ~mask) | ((0xff) & mask)
#include "p96_blit.cpp.in"
#define BLT_NAME BLIT_SWAP_8
#define BLT_NAME_MASK BLIT_SWAP_MASK_8
#define BLT_FUNC(s,d) { uae_u8 tmp = *d; *d = *s; *s = tmp; }
#define BLT_FUNC_MASK(s,d,mask) { uae_u8 tmp = *d; *d = ((*d) & ~mask) | ((*s) & mask); *s = ((*s) & ~mask) | ((tmp) & mask); }
#include "p96_blit.cpp.in"
#undef BLT_SIZE
#undef BLT_MULT

#define PARMS width, height, src, dst, ri->BytesPerRow, dstri->BytesPerRow, rgbmask
#define PARMSM width, height, src, dst, ri->BytesPerRow, dstri->BytesPerRow, mask

/*
* Functions to perform an action on the frame-buffer
*/
static void do_blitrect_frame_buffer (const struct RenderInfo *ri, const struct
	RenderInfo *dstri, uae_u32 srcx, uae_u32 srcy,
	uae_u32 dstx, uae_u32 dsty, uae_u32 width, uae_u32 height,
	uae_u8 mask, uae_u32 RGBFmt, BLIT_OPCODE opcode)
{
	uae_u8 *src, *dst;
	const uae_u8 Bpp = GetBytesPerPixel(RGBFmt);
	const uae_u32 total_width = width * Bpp;
	const uae_u32 rgbmask = rgbfmasks[RGBFmt];

	src = ri->Memory + srcx * Bpp + srcy * ri->BytesPerRow;
	dst = dstri->Memory + dstx * Bpp + dsty * dstri->BytesPerRow;

	P96TRACE ((_T("(%dx%d)=(%dx%d)=(%dx%d)=%d\n"), srcx, srcy, dstx, dsty, width, height, opcode));
	if (Bpp == 1 && mask != 0xff) {

		switch (opcode)
		{
		case BLIT_FALSE: BLIT_FALSE_MASK_8(PARMSM); break;
		case BLIT_NOR: BLIT_NOR_MASK_8(PARMSM); break;
		case BLIT_ONLYDST: BLIT_ONLYDST_MASK_8(PARMSM); break;
		case BLIT_NOTSRC: BLIT_NOTSRC_8(PARMSM); break;
		case BLIT_ONLYSRC: BLIT_ONLYSRC_MASK_8(PARMSM); break;
		case BLIT_NOTDST: BLIT_NOTDST_MASK_8(PARMSM); break;
		case BLIT_EOR: BLIT_EOR_MASK_8(PARMSM); break;
		case BLIT_NAND: BLIT_NAND_MASK_8(PARMSM); break;
		case BLIT_AND: BLIT_AND_MASK_8(PARMSM); break;
		case BLIT_NEOR: BLIT_NEOR_MASK_8(PARMSM); break;
		case BLIT_NOTONLYSRC: BLIT_NOTONLYSRC_MASK_8(PARMSM); break;
		case BLIT_SRC: BLIT_SRC_MASK_8(PARMSM); break;
		case BLIT_NOTONLYDST: BLIT_NOTONLYDST_MASK_8(PARMSM); break;
		case BLIT_OR: BLIT_OR_MASK_8(PARMSM); break;
		case BLIT_TRUE: BLIT_TRUE_MASK_8(PARMSM); break;
		case BLIT_SWAP: BLIT_SWAP_MASK_8(PARMSM); break;
		default: write_log (_T("Unsupported opcode %d\n"), opcode); break;
		}

	} else {

		if(opcode == BLIT_SRC) {

			/* handle normal case efficiently */
			if (ri->Memory == dstri->Memory && dsty == srcy) {
				for (int i = 0; i < height; i++, src += ri->BytesPerRow, dst += dstri->BytesPerRow)
					memmove (dst, src, total_width);
			} else if (dsty < srcy) {
				for (int i = 0; i < height; i++, src += ri->BytesPerRow, dst += dstri->BytesPerRow)
					memcpy (dst, src, total_width);
			} else {
				src += (height - 1) * ri->BytesPerRow;
				dst += (height - 1) * dstri->BytesPerRow;
				for (int i = 0; i < height; i++, src -= ri->BytesPerRow, dst -= dstri->BytesPerRow)
					memcpy (dst, src, total_width);
			}

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
				default: write_log (_T("Unsupported opcode %d\n"), opcode); break;
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
				default: write_log (_T("Unsupported opcode %d\n"), opcode); break;
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
				default: write_log (_T("Unsupported opcode %d\n"), opcode); break;
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
				default: write_log (_T("Unsupported opcode %d\n"), opcode); break;
				}

			}
		}

	}
}

/*
SetSpritePosition:
Synopsis: SetSpritePosition(bi, RGBFormat);
Inputs: a0: struct BoardInfo *bi
d7: RGBFTYPE RGBFormat

*/
static uae_u32 REGPARAM2 picasso_SetSpritePosition (TrapContext *ctx)
{
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	const struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const uaecptr bi = trap_get_areg(ctx, 0);
	boardinfo = bi;
#if 1
	int x = static_cast<uae_s16>(trap_get_word(ctx, bi + PSSO_BoardInfo_MouseX)) - static_cast<int>(state->XOffset);
	int y = static_cast<uae_s16>(trap_get_word(ctx, bi + PSSO_BoardInfo_MouseY)) - static_cast<int>(state->YOffset);
#else
	int x = (uae_s16)trap_get_dreg(ctx, 0) - state->XOffset;
	int y = (uae_s16)trap_get_dreg(ctx, 1) - state->YOffset;
#endif
	if (vidinfo->splitypos >= 0) {
		y += vidinfo->splitypos;
	}
	newcursor_x = x;
	newcursor_y = y;
	if (!hwsprite)
		return 0;
	return 1;
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
static uae_u32 REGPARAM2 picasso_SetSpriteColor (TrapContext *ctx)
{
	const uaecptr bi = trap_get_areg(ctx, 0);
	uae_u8 idx = trap_get_dreg(ctx, 0);
	const uae_u8 red = trap_get_dreg(ctx, 1);
	const uae_u8 green = trap_get_dreg(ctx, 2);
	const uae_u8 blue = trap_get_dreg(ctx, 3);
	boardinfo = bi;
	idx++;
	if (!hwsprite)
		return 0;
	if (idx >= 4)
		return 0;
	const uae_u32 oc = cursorrgb[idx];
	cursorrgb[idx] = (red << 16) | (green << 8) | (blue << 0);
	if (oc != cursorrgb[idx]) {
		setupcursor_needed = 1;
	}
	P96TRACE_SPR ((_T("SetSpriteColor(%08x,%d:%02X%02X%02X). %x\n"), bi, idx, red, green, blue, bi + PSSO_BoardInfo_MousePens));
	return 1;
}

STATIC_INLINE uae_u16 rgb32torgb16pc (uae_u32 rgb)
{
	return (((rgb >> (16 + 3)) & 0x1f) << 11) | (((rgb >> (8 + 2)) & 0x3f) << 5) | (((rgb >> (0 + 3)) & 0x1f) << 0);
}

static void updatesprcolors (int bpp)
{
	int i;
	for (i = 0; i < 4; i++) {
		uae_u32 v = cursorrgb[i];
		switch (bpp)
		{
		case 2:
			cursorrgbn[i] = rgb32torgb16pc (v);
			break;
		case 4:
			if (i > 0)
				v |= 0xff000000;
			else
				v &= 0x00ffffff;
			cursorrgbn[i] = v;
			break;
		default: // 1
			cursorrgbn[i] = (v & 0x80) ? 1 : 0;
			break;
		}
	}
}

#ifdef AMIBERRY
static void putmousepixel(const SDL_Surface* cursor_surface, const int x, const int y, const int c, const uae_u32 *ct)
{
	if (c == 0) {
		auto* const target_pixel = reinterpret_cast<Uint32*>(static_cast<Uint8*>(cursor_surface->pixels) + y * cursor_surface->pitch + x * cursor_surface->
			format->BytesPerPixel);
		*target_pixel = 0;
	} else {
		const uae_u32 val = ct[c];
		auto* pixels = static_cast<unsigned char*>(cursor_surface->pixels);
		pixels[4 * (y * cursor_surface->pitch + x) + 0] = (val >> 16); //Red
		pixels[4 * (y * cursor_surface->pitch + x) + 1] = (val >> 8); //Green
		pixels[4 * (y * cursor_surface->pitch + x) + 2] = val; //Blue
		pixels[4 * (y * cursor_surface->pitch + x) + 3] = 255; //Alpha
	}
}
#else
static void putwinmousepixel(HDC andDC, HDC xorDC, int x, int y, int c, uae_u32 *ct)
{
	if (c == 0) {
		SetPixel(andDC, x, y, RGB (255, 255, 255));
		SetPixel(xorDC, x, y, RGB (0, 0, 0));
	} else {
		uae_u32 val = ct[c];
		SetPixel(andDC, x, y, RGB (0, 0, 0));
		SetPixel(xorDC, x, y, RGB ((val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff));
	}
}
#endif

//static int wincursorcnt;
static int tmp_sprite_w, tmp_sprite_h;
static uae_u8 tmp_sprite_data[CURSORMAXWIDTH * CURSORMAXHEIGHT];
static uae_u32 tmp_sprite_colors[4];

extern uaecptr sprite_0;
extern int sprite_0_width, sprite_0_height, sprite_0_doubled;
extern uae_u32 sprite_0_colors[4];

static int createwindowscursor(int monid, int set, int chipset)
{
#ifdef AMIBERRY
	SDL_Surface* cursor_surface = nullptr;
	SDL_Surface* formatted_cursor_surface = nullptr;
	int ret = 0;
	bool isdata = false;
	SDL_Cursor* old_cursor = p96_cursor;
	uae_u32 *ct;
	TrapContext *ctx = nullptr;
	int w, h;
	uae_u8 *image;
	uae_u8 tmp_sprite[CURSORMAXWIDTH * CURSORMAXHEIGHT];
	int datasize;

	wincursor_shown = 0;

	if (isfullscreen() > 0 || currprefs.input_tablet == 0 || !(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		goto exit;
	}
	if (currprefs.input_magic_mouse_cursor != MAGICMOUSE_HOST_ONLY) {
		goto exit;
	}

	if (chipset) {
		w = sprite_0_width;
		h = sprite_0_height;
		uaecptr src = sprite_0;
		int doubledsprite = sprite_0_doubled;
		if (doubledsprite) {
			w *= 2;
			h *= 2;
		}
		int hiressprite = sprite_0_width / 16;
		int ds = h * ((w + 15) / 16) * 4;
		if (!sprite_0 || !mousehack_alive() || w > CURSORMAXWIDTH || h > CURSORMAXHEIGHT || !valid_address(src, ds)) {
			goto exit;
		}
		int yy = 0;
		for (int y = 0, yy = 0; y < h; yy++) {
			uae_u8 *dst = tmp_sprite + y * w;
			int dbl;
			uaecptr img = src + yy * 4 * hiressprite;
			for (dbl = 0; dbl < (doubledsprite ? 2 : 1); dbl++) {
				int x = 0;
				while (x < w) {
					uae_u32 d1 = trap_get_long(ctx, img);
					uae_u32 d2 = trap_get_long(ctx, img + 2 * hiressprite);
					int bits;
					int maxbits = w - x;

					maxbits = std::min(maxbits, 16 * hiressprite);
					for (bits = 0; bits < maxbits && x < w; bits++) {
						uae_u8 c = ((d2 & 0x80000000) ? 2 : 0) + ((d1 & 0x80000000) ? 1 : 0);
						d1 <<= 1;
						d2 <<= 1;
						*dst++ = c;
						x++;
						if (doubledsprite && x < w) {
							*dst++ = c;
							x++;
						}
					}
				}
				if (y <= h) {
					y++;
				}
			}
		}
		ct = sprite_0_colors;
		image = tmp_sprite;
	} else {
		w = cursorwidth;
		h = cursorheight;
		ct = cursorrgbn;
		image = cursordata;
	}

	datasize = h * ((w + 15) / 16) * 16;

	if (p96_cursor) {
		if (w == tmp_sprite_w && h == tmp_sprite_h && !memcmp(tmp_sprite_data, image, datasize) && !memcmp(tmp_sprite_colors, ct, sizeof (uae_u32)*4)) {
			if (SDL_GetCursor() == p96_cursor) {
				wincursor_shown = 1;
				return 1;
			}
		}
	}

	p96_cursor = nullptr;

	write_log(_T("p96_cursor: %dx%d\n"), w, h);

	tmp_sprite_w = tmp_sprite_h = 0;

end:
	if (isdata) {
		p96_cursor = SDL_CreateColorCursor(formatted_cursor_surface, 0, 0);
		tmp_sprite_w = w;
		tmp_sprite_h = h;
		memcpy(tmp_sprite_data, image, datasize);
		memcpy(tmp_sprite_colors, ct, sizeof (uae_u32) * 4);
	}

	if (cursor_surface != nullptr)
	{
		SDL_FreeSurface(cursor_surface);
		cursor_surface = nullptr;
	}

	if (formatted_cursor_surface != nullptr)
	{
		SDL_FreeSurface(formatted_cursor_surface);
		formatted_cursor_surface = nullptr;
	}

	if (p96_cursor) {
		SDL_SetCursor(p96_cursor);
		wincursor_shown = 1;
	}

	if (!ret) {
		write_log(_T("RTG Windows color cursor creation failed\n"));
	}

	if (old_cursor) {
		SDL_FreeCursor(old_cursor);
		old_cursor = nullptr;
	}

	return ret;

exit:
	if (p96_cursor) {
		if (SDL_GetCursor() == p96_cursor) {
			SDL_SetCursor(normalcursor);
		}
		SDL_FreeCursor(p96_cursor);
		p96_cursor = nullptr;
	}

	return ret;
#else
	HBITMAP andBM, xorBM;
	HBITMAP andoBM, xoroBM;
	HDC andDC, xorDC, DC, mainDC;
	ICONINFO ic;
	int ret = 0;
	bool isdata;
	HCURSOR oldwincursor = wincursor;
	uae_u32 *ct;
	TrapContext *ctx = NULL;
	int w, h;
	uae_u8 *image;
	uae_u8 tmp_sprite[CURSORMAXWIDTH * CURSORMAXHEIGHT];
	int datasize = 0;

	wincursor_shown = 0;

	if (isfullscreen() > 0 || currprefs.input_tablet == 0 || !(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		goto exit;
	}
	if (currprefs.input_magic_mouse_cursor != MAGICMOUSE_HOST_ONLY) {
		goto exit;
	}

	if (chipset) {
		w = sprite_0_width;
		h = sprite_0_height;
		uaecptr src = sprite_0;
		int doubledsprite = sprite_0_doubled;
		if (doubledsprite) {
			w *= 2;
			h *= 2;
		}
		int hiressprite = sprite_0_width / 16;
		int ds = h * ((w + 15) / 16) * 4;
		if (!sprite_0 || !mousehack_alive() || w > CURSORMAXWIDTH || h > CURSORMAXHEIGHT || !valid_address(src, ds)) {
			if (wincursor) {
				SetCursor(normalcursor);
			}
			goto exit;
		}
		int yy = 0;
		for (int y = 0, yy = 0; y < h; yy++) {
			uae_u8 *dst = tmp_sprite + y * w;
			int dbl;
			uaecptr img = src + yy * 4 * hiressprite;
			for (dbl = 0; dbl < (doubledsprite ? 2 : 1); dbl++) {
				int x = 0;
				while (x < w) {
					uae_u32 d1 = trap_get_long(ctx, img);
					uae_u32 d2 = trap_get_long(ctx, img + 2 * hiressprite);
					int bits;
					int maxbits = w - x;

					if (maxbits > 16 * hiressprite) {
						maxbits = 16 * hiressprite;
					}
					for (bits = 0; bits < maxbits && x < w; bits++) {
						uae_u8 c = ((d2 & 0x80000000) ? 2 : 0) + ((d1 & 0x80000000) ? 1 : 0);
						d1 <<= 1;
						d2 <<= 1;
						*dst++ = c;
						x++;
						if (doubledsprite && x < w) {
							*dst++ = c;
							x++;
						}
					}
				}
				if (y <= h) {
					y++;
				}
			}
		}
		ct = sprite_0_colors;
		image = tmp_sprite;
	} else {
		w = cursorwidth;
		h = cursorheight;
		ct = cursorrgbn;
		image = cursordata;
	}

	datasize = h * ((w + 15) / 16) * 16;

	if (wincursor) {
		if (w == tmp_sprite_w && h == tmp_sprite_h && !memcmp(tmp_sprite_data, image, datasize) && !memcmp(tmp_sprite_colors, ct, sizeof (uae_u32)*4)) {
			if (GetCursor() == wincursor) {
				wincursor_shown = 1;
				return 1;
			}
		}
	}

	write_log(_T("wincursor: %dx%d\n"), w, h);

	tmp_sprite_w = tmp_sprite_h = 0;

	DC = mainDC = andDC = xorDC = NULL;
	andBM = xorBM = NULL;
	DC = GetDC(NULL);
	if (!DC)
		goto end;
	mainDC = CreateCompatibleDC(DC);
	andDC = CreateCompatibleDC(DC);
	xorDC = CreateCompatibleDC(DC);
	if (!mainDC || !andDC || !xorDC)
		goto end;
	andBM = CreateCompatibleBitmap(DC, w, h);
	xorBM = CreateCompatibleBitmap(DC, w, h);
	if (!andBM || !xorBM)
		goto end;
	andoBM = (HBITMAP)SelectObject(andDC, andBM);
	xoroBM = (HBITMAP)SelectObject(xorDC, xorBM);

	isdata = false;
	for (int y = 0; y < h; y++) {
		uae_u8 *s = image + y * w;
		for (int x = 0; x < w; x++) {
			int c = *s++;
			putwinmousepixel(andDC, xorDC, x, y, c, ct);
			if (c > 0) {
				isdata = true;
			}
		}
	}
	ret = 1;

	SelectObject(andDC, andoBM);
	SelectObject(xorDC, xoroBM);

end:
	DeleteDC(xorDC);
	DeleteDC(andDC);
	DeleteDC(mainDC);
	ReleaseDC(NULL, DC);

	if (!isdata) {
		wincursor = LoadCursor(NULL, IDC_ARROW);
	} else if (ret) {
		memset(&ic, 0, sizeof ic);
		ic.hbmColor = xorBM;
		ic.hbmMask = andBM;
		wincursor = CreateIconIndirect(&ic);
		tmp_sprite_w = w;
		tmp_sprite_h = h;
		memcpy(tmp_sprite_data, image, datasize);
		memcpy(tmp_sprite_colors, ct, sizeof (uae_u32) * 4);
	}

	DeleteObject(andBM);
	DeleteObject(xorBM);

	if (wincursor) {
		SetCursor(wincursor);
		wincursor_shown = 1;
	}

	if (!ret) {
		write_log(_T("RTG Windows color cursor creation failed\n"));
	}

exit:
	if (currprefs.input_tablet && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) && currprefs.input_magic_mouse_cursor == MAGICMOUSE_NATIVE_ONLY) {
		if (GetCursor() != NULL)
			SetCursor(NULL);
	} else {
		if (wincursor == oldwincursor && normalcursor != NULL) {
			SetCursor(normalcursor);
		}
	}
	if (oldwincursor) {
		DestroyIcon(oldwincursor);
	}
	oldwincursor = NULL;

	return ret;
#endif
}

int picasso_setwincursor(int monid)
{
	struct amigadisplay *ad = &adisplays[monid];
#ifdef AMIBERRY
	if (p96_cursor) {
		SDL_SetCursor(p96_cursor);
		return 1;
	} else if (!ad->picasso_on) {
		if (createwindowscursor(monid, 0, 1))
			return 1;
	}
#else
	if (wincursor) {
		SetCursor(wincursor);
		return 1;
	} else if (!ad->picasso_on) {
		if (createwindowscursor(monid, 0, 1))
			return 1;
	}
#endif
	return 0;
}

static uae_u32 setspriteimage(TrapContext *ctx, uaecptr bi)
{
	uae_u32 flags;
	int x, y, yy, bits, bpp;
	int hiressprite, doubledsprite;
	int ret = 0;
	int w, h;

	cursordeactivate = 0;
	if (!hwsprite)
		return 0;
	xfree (cursordata);
	cursordata = nullptr;
	bpp = 4;
	w = trap_get_byte(ctx, bi + PSSO_BoardInfo_MouseWidth);
	h = trap_get_byte(ctx, bi + PSSO_BoardInfo_MouseHeight);
	flags = trap_get_long(ctx, bi + PSSO_BoardInfo_Flags);
	hiressprite = 1;
	doubledsprite = 0;
	if (flags & BIF_HIRESSPRITE)
		hiressprite = 2;
	if (flags & BIF_BIGSPRITE)
		doubledsprite = 1;
	updatesprcolors (bpp);

	P96TRACE_SPR ((_T("SetSpriteImage(%08x,%08x,w=%d,h=%d,%d/%d,%08x)\n"),
		bi, trap_get_long(ctx, bi + PSSO_BoardInfo_MouseImage), w, h,
		hiressprite - 1, doubledsprite, bi + PSSO_BoardInfo_MouseImage));

	const uaecptr iptr = trap_get_long(ctx, bi + PSSO_BoardInfo_MouseImage);
	const int datasize = 4 * hiressprite + h * 4 * hiressprite;

	if (!w || !h || iptr == 0 || !valid_address(iptr, datasize)) {
		cursordeactivate = 1;
		ret = 1;
		goto end;
	}

	cursordata = xmalloc (uae_u8, w * h);
	for (y = 0, yy = 0; y < h; y++, yy++) {
		uae_u8 *p = cursordata + w * y;
		const uae_u8 *pprev = p;
		const uaecptr img = iptr + 4 * hiressprite + yy * 4 * hiressprite;
		x = 0;
		while (x < w) {
			uae_u32 d1 = trap_get_long(ctx, img);
			uae_u32 d2 = trap_get_long(ctx, img + 2 * hiressprite);
			int maxbits = w - x;
			maxbits = std::min(maxbits, 16 * hiressprite);
			for (bits = 0; bits < maxbits && x < w; bits++) {
				const uae_u8 c = ((d2 & 0x80000000) ? 2 : 0) + ((d1 & 0x80000000) ? 1 : 0);
				d1 <<= 1;
				d2 <<= 1;
				*p++ = c;
				x++;
				if (doubledsprite && x < w) {
					*p++ = c;
					x++;
				}
			}
		}
		if (doubledsprite && y < h) {
			y++;
			memcpy(p, pprev, w);
		}
	}

	cursorwidth = w;
	if (cursorwidth > CURSORMAXWIDTH)
		cursorwidth = CURSORMAXWIDTH;
	cursorheight = h;
	if (cursorheight > CURSORMAXHEIGHT)
		cursorheight = CURSORMAXHEIGHT;

	createwindowscursor(0, 1, 0);

	setupcursor ();
	ret = 1;
	cursorok = TRUE;
	P96TRACE_SPR ((_T("hardware sprite created\n")));
end:
	return ret;
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
	const uaecptr bi = trap_get_areg(ctx, 0);
	boardinfo = bi;
	return setspriteimage(ctx, bi);
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
	uae_u32 result = 0;
	const uae_u32 activate = trap_get_dreg(ctx, 0);
	if (!hwsprite)
		return 0;
	if (activate) {
		picasso_SetSpriteImage (ctx);
		cursorvisible = true;
	} else {
		cursordeactivate = 2;
	}
	result = 1;
	P96TRACE_SPR ((_T("SetSprite: %d\n"), activate));

	return result;
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
	const uaecptr AmigaBoardInfo = trap_get_areg(ctx, 0);
	struct picasso96_state_struct *state = &picasso96_state[currprefs.rtgboards[0].monitor_id];
	/* NOTES: See BoardInfo struct definition in Picasso96 dev info */
	if (!uaegfx_active || !(gfxmem_bank.flags & ABFLAG_MAPPED))
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

	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_Active, 1);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_Width, width);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_Height, height);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_Depth, depth);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_Flags, 0);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorTotal, width + 8);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorBlankSize, 8);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorSyncStart, 2);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_HorSyncSize, 2);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_HorSyncSkew, 0);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_HorEnableSkew, 0);

	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerTotal, height + 8);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerBlankSize, 8);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerSyncStart, 2);
	trap_put_word(ctx, amigamemptr + PSSO_ModeInfo_VerSyncSize, 2);

	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_first_union, 98);
	trap_put_byte(ctx, amigamemptr + PSSO_ModeInfo_second_union, 14);

	trap_put_long(ctx, amigamemptr + PSSO_ModeInfo_PixelClock,
		width * height * (currprefs.gfx_apmode[1].gfx_refreshrate ? abs (currprefs.gfx_apmode[1].gfx_refreshrate) : default_freq));
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
	2560,1920, 176,
	3440,1440, 177,
	3840,1600, 178,
	3840,2160, 179,
	5120,1440, 180,
	5120,2160, 181,
	1280, 600, 182,
	1024, 600, 183,
	-1,-1,0
};

static int AssignModeID (int w, int h, int *unkcnt)
{
	int i;

#ifdef NEWMODES2
	return 0x40000000 | (w << 14) | h;
#endif
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


static void CopyLibResolutionStructureU2A(TrapContext *ctx, const struct LibResolution *libres, uaecptr amigamemptr)
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


void picasso_allocatewritewatch (int index, int gfxmemsize)
{
#ifdef _WIN32
	SYSTEM_INFO si;

	xfree (gwwbuf[index]);
	GetSystemInfo (&si);
	gwwpagesize[index] = si.dwPageSize;
	gwwbufsize[index] = gfxmemsize / gwwpagesize[index] + 1;
	gwwpagemask[index] = gwwpagesize[index] - 1;
	gwwbuf[index] = xmalloc (void*, gwwbufsize[index]);
#endif
}

#ifdef _WIN32
static ULONG_PTR writewatchcount[MAX_RTG_BOARDS];
static int watch_offset[MAX_RTG_BOARDS];
#endif
int picasso_getwritewatch (int index, int offset, uae_u8 ***gwwbufp, uae_u8 **startp)
{
#ifdef _WIN32
	ULONG ps;
	writewatchcount[index] = gwwbufsize[index];
	watch_offset[index] = offset;
	if (gfxmem_banks[index]->start + offset >= max_physmem) {
		writewatchcount[index] = 0;
		return -1;
	}
	uae_u8 *start = gfxmem_banks[index]->start + natmem_offset + offset;
	if (GetWriteWatch (WRITE_WATCH_FLAG_RESET, start, (gwwbufsize[index] - 1) * gwwpagesize[index], gwwbuf[index], &writewatchcount[index], &ps)) {
		write_log (_T("picasso_getwritewatch %d\n"), GetLastError ());
		writewatchcount[index] = 0;
		return -1;
	}
	if (gwwbufp)
		*gwwbufp = (uae_u8**)gwwbuf[index];
	if (startp)
		*startp = start;
	return (int)writewatchcount[index];
#else
	return -1;
#endif
}
bool picasso_is_vram_dirty (int index, uaecptr addr, int size)
{
#ifdef _WIN32
	static ULONG_PTR last;
	uae_u8 *a = addr + natmem_offset + watch_offset[index];
	int s = size;
	int ms = gwwpagesize[index];

	for (;;) {
		for (ULONG_PTR i = last; i < writewatchcount[index]; i++) {
			uae_u8 *ma = (uae_u8*)gwwbuf[index][i];
			if (
				(a < ma && a + s >= ma) ||
				(a < ma + ms && a + s >= ma + ms) ||
				(a >= ma && a < ma + ms)) {
				last = i;
				return true;
			}
		}
		if (last == 0)
			break;
		last = 0;
	}
	return false;
#else
	return true;
#endif
}

static void init_alloc (TrapContext *ctx, int size)
{
	picasso96_amem = picasso96_amemend = 0;
	if (uaegfx_base) {
		int size = static_cast<int>(trap_get_long(ctx, uaegfx_base + CARD_RESLISTSIZE));
		picasso96_amem = trap_get_long(ctx, uaegfx_base + CARD_RESLIST);
	} else if (uaegfx_active) {
		reserved_gfxmem = size;
		picasso96_amem = gfxmem_bank.start + gfxmem_bank.allocated_size - size;
	}
	picasso96_amemend = picasso96_amem + size;
	write_log (_T("P96 RESINFO: %08X-%08X (%d,%d)\n"), picasso96_amem, picasso96_amemend, size / PSSO_ModeInfo_sizeof, size);
#ifdef _WIN32
	picasso_allocatewritewatch (0, gfxmem_bank.allocated_size);
#endif
}

static int p96depth (int depth)
{
	const uae_u32 f = currprefs.picasso96_modeflags;
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
	const auto ma = (struct PicassoResolution *)a;
	const auto mb = (struct PicassoResolution *)b;
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

static int missmodes[] = { 320, 200, 320, 240, 320, 256, 640, 400, 640, 480, 640, 512, 800, 600, 1024, 600, 1024, 768, 1280, 1024, -1 };

static uaecptr uaegfx_card_install (TrapContext *ctx, uae_u32 size);

static void picasso96_alloc2 (TrapContext *ctx)
{
	int i, j, size, cnt;
	int misscnt, depths;

	xfree (newmodes);
	newmodes = nullptr;
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

	for (const auto & Display : Displays) {
		const struct PicassoResolution *DisplayModes = Display.DisplayModes;
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
	}

	cnt = 0;
	for (const auto & Display : Displays) {
		const struct PicassoResolution *DisplayModes = Display.DisplayModes;
		i = 0;
		while (DisplayModes[i].depth >= 0) {
			if (DisplayModes[i].rawmode) {
				i++;
				continue;
			}
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
				const int w = static_cast<int>(DisplayModes[i].res.width);
				const int h = static_cast<int>(DisplayModes[i].res.height);
				if (w > missmodes[misscnt * 2 + 0] || (w == missmodes[misscnt * 2 + 0] && h > missmodes[misscnt * 2 + 1])) {
					struct PicassoResolution *pr = &newmodes[cnt];
					memcpy (pr, &DisplayModes[i], sizeof (struct PicassoResolution));
					pr->res.width = missmodes[misscnt * 2 + 0];
					pr->res.height = missmodes[misscnt * 2 + 1];
					_sntprintf (pr->name, sizeof pr->name, _T("%dx%d FAKE"), pr->res.width, pr->res.height);
					size += PSSO_ModeInfo_sizeof * depths;
					cnt++;
					misscnt++;
					continue;
				}
			}
			int k;
			for (k = 0; k < cnt; k++) {
				if (newmodes[k].res.width == DisplayModes[i].res.width &&
					newmodes[k].res.height == DisplayModes[i].res.height &&
					newmodes[k].depth == DisplayModes[i].depth)
					break;
			}
			if (k >= cnt) {
				memcpy (&newmodes[cnt], &DisplayModes[i], sizeof (struct PicassoResolution));
				size += PSSO_ModeInfo_sizeof * depths;
				cnt++;
			}
			i++;
		}
	}
	qsort (newmodes, cnt, sizeof (struct PicassoResolution), resolution_compare);


#if MULTIDISPLAY
	for (i = 0; Displays[i].name; i++) {
		size += PSSO_LibResolution_sizeof;
		size += PSSO_ModeInfo_sizeof * depths;
	}
#endif
	newmodes[cnt].depth = -1;

	for (i = 0; i < cnt; i++) {
		int depth;
		for (depth = 1; depth <= 4; depth++) {
			switch (depth) {
			case 1:
				chunky.width = std::max(newmodes[i].res.width, chunky.width);
				chunky.height = std::max(newmodes[i].res.height, chunky.height);
				break;
			case 2:
				hicolour.width = std::max(newmodes[i].res.width, hicolour.width);
				hicolour.height = std::max(newmodes[i].res.height, hicolour.height);
				break;
			case 3:
				truecolour.width = std::max(newmodes[i].res.width, truecolour.width);
				truecolour.height = std::max(newmodes[i].res.height, truecolour.height);
				break;
			case 4:
				alphacolour.width = std::max(newmodes[i].res.width, alphacolour.width);
				alphacolour.height = std::max(newmodes[i].res.height, alphacolour.height);
				break;
			default: // never
				break;
			}
		}
	}
	uaegfx_card_install (ctx, size);
	init_alloc (ctx, size);
}

void picasso96_alloc (TrapContext *ctx)
{
	if (currprefs.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE)
		return;
	uaegfx_resname = ds(_T("uaegfx.card"));
	uaegfx_prefix = ds(_T("UAE"));
	if (uaegfx_old)
		return;
	picasso96_alloc2 (ctx);
}

static void inituaegfxfuncs (TrapContext *ctx, uaecptr start, uaecptr ABI);
static void inituaegfx(TrapContext *ctx, uaecptr ABI)
{
	cursorvisible = false;
	cursorok = 0;
	cursordeactivate = 0;
	write_log (_T("RTG mode mask: %x BI=%08x\n"), currprefs.picasso96_modeflags, ABI);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_BitsPerCannon, 8);
	trap_put_word(ctx, ABI + PSSO_BoardInfo_RGBFormats, currprefs.picasso96_modeflags);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_BoardType, picasso96_BT);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_GraphicsControllerType, picasso96_GCT);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_PaletteChipType, picasso96_PCT);
	trap_put_long(ctx, ABI + PSSO_BoardInfo_BoardName, uaegfx_prefix);

	trap_put_long(ctx, ABI + PSSO_BoardInfo_MemoryClock, 200000000);

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

	uae_u32 flags = trap_get_long(ctx, ABI + PSSO_BoardInfo_Flags);
	flags &= 0xffff0000;
	if (flags & BIF_NOBLITTER)
		write_log (_T("P96: Blitter disabled in devs:monitors/uaegfx!\n"));
	if constexpr (NOBLITTER_ALL) {
		flags |= BIF_NOBLITTER;
		flags &= ~BIF_BLITTER;
	} else {
		flags |= BIF_BLITTER;
	}
	flags |= BIF_NOMEMORYMODEMIX;
	flags |= BIF_GRANTDIRECTACCESS;
	flags &= ~BIF_HARDWARESPRITE;

#ifdef AMIBERRY
	if (USE_HARDWARESPRITE && currprefs.rtg_hardwaresprite) {
#else
	if (D3D_setcursor && D3D_setcursor(0, -1, -1, -1, -1, 0, 0, false, false) && USE_HARDWARESPRITE && currprefs.rtg_hardwaresprite) {
#endif
		hwsprite = 1;
		flags |= BIF_HARDWARESPRITE;
		write_log (_T("P96: Hardware sprite support enabled\n"));
	} else {
		hwsprite = 0;
		write_log (_T("P96: Hardware sprite support disabled\n"));
	}
	if (currprefs.rtg_hardwareinterrupt && !uaegfx_old)
		flags |= BIF_VBLANKINTERRUPT;
	// force INDISPLAYCHAIN if no multiple monitors emulated
	if (!(flags & BIF_INDISPLAYCHAIN)) {
		if (currprefs.rtgboards[0].monitor_id == 0) {
			write_log(_T("P96: BIF_INDISPLAYCHAIN force-enabled!\n"));
			flags |= BIF_INDISPLAYCHAIN;
		}
	}

	if (USE_OVERLAY && currprefs.rtg_overlay) {
		flags |= BIF_VIDEOWINDOW;
	}
	if (USE_VGASCREENSPLIT && currprefs.rtg_vgascreensplit) {
		flags |= BIF_VGASCREENSPLIT;
	}
	if (USE_PALETTESWITCH && currprefs.rtg_paletteswitch) {
		flags |= BIF_PALETTESWITCH;
	}
	if (USE_DACSWITCH && currprefs.rtg_dacswitch) {
		flags |= BIF_DACSWITCH;
	}

	trap_put_long(ctx, ABI + PSSO_BoardInfo_Flags, flags);
	if (debug_rtg_blitter != 3)
		write_log (_T("P96: Blitter mode = %x!\n"), debug_rtg_blitter);

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
		res->DisplayID = AssignModeID (w, h, unkcnt);
	}
	res->BoardInfo = AmigaBoardInfo;
	res->Width = w;
	res->Height = h;
	res->Flags = P96F_PUBLIC;
	memcpy (res->P96ID, "P96-0:", 6);
	if (name) {
		char *n2 = ua (name);
		strcpy (res->Name, n2);
		xfree (n2);
	} else {
		_sntprintf (res->Name, sizeof res->Name, "UAE:%4dx%4d", w, h);
	}

	for (depth = 8; depth <= 32; depth++) {
		if (!p96depth (depth))
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
static uae_u32 REGPARAM2 picasso_InitCard (TrapContext *ctx)
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
		struct LibResolution res = { 0 };
		const int j = i;
		if (addmode(ctx, AmigaBoardInfo, &amem, &res, static_cast<int>(newmodes[i].res.width), static_cast<int>(newmodes[i].res.height), nullptr, 0, &unkcnt)) {
			TCHAR *s;
			s = au (res.Name);
			write_log (_T("%2d: %08X %4dx%4d %s\n"), ++cnt, res.DisplayID, res.Width, res.Height, s);
			xfree (s);
			while (newmodes[i].depth >= 0
				&& newmodes[i].res.width == newmodes[j].res.width
				&& newmodes[i].res.height == newmodes[j].res.height)
				i++;

			LibResolutionStructureCount++;
			CopyLibResolutionStructureU2A(ctx, &res, amem);
#if P96TRACING_ENABLED && P96TRACING_LEVEL > 1
			DumpLibResolutionStructure(ctx, amem);
#endif
			AmigaListAddTail(ctx, AmigaBoardInfo + PSSO_BoardInfo_ResolutionsList, amem);
			amem += PSSO_LibResolution_sizeof;
		} else {
			write_log (_T("--: %08X %4dx%4d Not enough VRAM\n"), res.DisplayID, res.Width, res.Height);
			i++;
		}
	}
#if MULTIDISPLAY
	for (i = 0; Displays[i].name; i++) {
		struct LibResolution res = { 0 };
		struct MultiDisplay *md = &Displays[i];
		int w = md->rect.right - md->rect.left;
		int h = md->rect.bottom - md->rect.top;
		TCHAR tmp[100];
		if (md->primary)
			strcpy (tmp, "UAE:Primary");
		else
			_stprintf (tmp, "UAE:Display#%d", i);
		addmode (AmigaBoardInfo, &amem, &res, w, h, tmp, i + 1, &unkcnt);
		write_log (_T("%08X %4dx%4d %s\n"), res.DisplayID, res.Width + 16, res.Height, res.Name);
		LibResolutionStructureCount++;
		CopyLibResolutionStructureU2A (&res, amem);
#if P96TRACING_ENABLED && P96TRACING_LEVEL > 1
		DumpLibResolutionStructure(ctx, amem);
#endif
		AmigaListAddTail (AmigaBoardInfo + PSSO_BoardInfo_ResolutionsList, amem);
		amem += PSSO_LibResolution_sizeof;
	}
#endif

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
static uae_u32 REGPARAM2 picasso_SetSwitch (TrapContext *ctx)
{
	lockrtg();
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	struct amigadisplay *ad = &adisplays[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const uae_u16 flag = trap_get_dreg(ctx, 0) & 0xFFFF;

	TCHAR p96text[100];
	p96text[0] = 0;
	if (flag && (state->BytesPerPixel == 0 || state->Width == 0 || state->Height == 0) && monid > 0) {
		state->Width = 640;
		state->VirtualWidth = state->Width;
		state->Height = 480;
		state->VirtualHeight = state->Height;
		state->GC_Depth = 8;
		state->GC_Flags = 0;
		state->BytesPerPixel = 1;
		state->HLineDBL = 1;
		state->VLineDBL = 1;
		state->HostAddress = nullptr;
		delayed_set_switch = true;
		atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETGC);
	} else {
		delayed_set_switch = false;
		atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETSWITCH);
		ad->picasso_requested_on = flag != 0;
		set_config_changed();
	}
	if (flag)
		_sntprintf(p96text, sizeof p96text, _T("Picasso96 %dx%dx%d (%dx%dx%d)"),
			state->Width, state->Height, state->BytesPerPixel * 8,
			vidinfo->width, vidinfo->height, vidinfo->pixbytes * 8);
	write_log(_T("SetSwitch() - %s - %s. Monitor=%d\n"), flag ? p96text : _T("amiga"), delayed_set_switch ? _T("delayed") : _T("immediate"), monid);

	/* Put old switch-state in D0 */
	unlockrtg();
	return !flag;
}


void picasso_enablescreen(int monid, int on)
{
	const bool uaegfx = currprefs.rtgboards[0].rtgmem_type < GFXBOARD_HARDWARE && currprefs.rtgboards[0].rtgmem_size;
	const bool uaegfx_active = is_uaegfx_active();

	if (uaegfx_active && uaegfx) {
		if (!init_picasso_screen_called)
			init_picasso_screen(monid);
	}
	setconvert(monid);
	picasso_refresh(0);
}

static void resetpalette(struct picasso96_state_struct *state)
{
	for (auto & i : state->CLUT) {
		i.Pad = 0xff;
	}
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
static int updateclut(TrapContext *ctx, uaecptr clut, int start, int count, int offset)
{
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	uae_u8 clutbuf[256 * 3];
	int i, changed = 0;
	clut += start * 3;
	trap_get_bytes(ctx, clutbuf + start * 3, clut, count * 3);
	for (i = start; i < start + count; i++) {
		const int coffset = i + offset;
		const int r = clutbuf[i * 3 + 0];
		const int g = clutbuf[i * 3 + 1];
		const int b = clutbuf[i * 3 + 2];
		//write_log(_T("%d: %02x%02x%02x\n"), i, r, g, b);
		changed |= state->CLUT[coffset].Red != r
			|| state->CLUT[coffset].Green != g
			|| state->CLUT[coffset].Blue != b;
		if (state->CLUT[coffset].Pad) {
			changed = 1;
			state->CLUT[coffset].Pad = 0;
		}
		state->CLUT[coffset].Red = r;
		state->CLUT[coffset].Green = g;
		state->CLUT[coffset].Blue = b;
	}
	if (offset && currprefs.rtg_paletteswitch) {
		state->dualclut = true;
	}
	changed |= picasso_palette(state->CLUT, vidinfo->clut);
	return changed;
}
static uae_u32 REGPARAM2 picasso_SetColorArray (TrapContext *ctx)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[currprefs.rtgboards[0].monitor_id];
	/* Fill in some static UAE related structure about this new CLUT setting
	* We need this for CLUT-based displays, and for mapping CLUT to hi/true colour */
	uae_u16 start = trap_get_dreg (ctx, 0);
	const uae_u16 count = trap_get_dreg (ctx, 1);
	const uaecptr boardinfo = trap_get_areg (ctx, 0);
	uaecptr clut = boardinfo + PSSO_BoardInfo_CLUT;
	int offset = 0;
	if (start > 512 || count > 512 || start + count > 512)
		return 0;
	if (start >= 256) {
		clut = boardinfo + PSSO_BoardInfo_SecondaryCLUT;
		start -= 256;
		offset = 256;
	}
	lockrtg();
	if (updateclut(ctx, clut, start, count, offset))
		vidinfo->full_refresh = 1;
	unlockrtg();
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
static uae_u32 REGPARAM2 picasso_SetDAC (TrapContext *ctx)
{
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const struct picasso96_state_struct* state = &picasso96_state[monid];
	const uae_u16 idx = trap_get_dreg(ctx, 0);
	const uae_u32 mode = trap_get_dreg(ctx, 7);
	/* Fill in some static UAE related structure about this new DAC setting
	* Lets us keep track of what pixel format the Amiga is thinking about in our frame-buffer */

	if (state->advDragging) {
		vidinfo->dacrgbformat[idx ? 1 : 0] = mode;
	} else {
		vidinfo->dacrgbformat[0] = mode;
		vidinfo->dacrgbformat[1] = mode;
	}
	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETDAC);
	P96TRACE_SETUP((_T("SetDAC()\n")));
	return 1;
}

static uae_u32 REGPARAM2 picasso_CoerceMode(struct TrapContext *ctx)
{
	const uae_u16 bw = trap_get_dreg(ctx, 2);
	const uae_u16 fw = trap_get_dreg(ctx, 3);
	return bw > fw ? bw : fw;
}

static uae_u32 REGPARAM2 picasso_GetCompatibleDACFormats(struct TrapContext *ctx)
{
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	const auto type = static_cast<RGBFTYPE>(trap_get_dreg(ctx, 7));
	switch (type)
	{
	case RGBFB_CLUT:
	case RGBFB_R8G8B8:
	case RGBFB_B8G8R8:
	case RGBFB_R5G6B5PC:
	case RGBFB_R5G5B5PC:
	case RGBFB_A8R8G8B8:
	case RGBFB_A8B8G8R8:
	case RGBFB_R8G8B8A8:
	case RGBFB_B8G8R8A8:
	case RGBFB_R5G6B5:
	case RGBFB_R5G5B5:
	case RGBFB_B5G6R5PC:
	case RGBFB_B5G5R5PC:
		state->advDragging = true;
		return RGBMASK_8BIT | RGBMASK_15BIT | RGBMASK_16BIT | RGBMASK_24BIT | RGBMASK_32BIT;
	default: // never
		return 0;
	}
	return 0;
}

static void init_picasso_screen(int monid)
{
	const struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	struct picasso96_state_struct *state = &picasso96_state[monid];
	if(vidinfo->set_panning_called) {
		state->Extent = state->Address + state->BytesPerRow * state->VirtualHeight;
	}
	if (set_gc_called) {
		gfx_set_picasso_modeinfo(monid, state->RGBFormat);
		set_gc_called = 0;
	}

	if((vidinfo->width == state->Width) &&
		(vidinfo->height == state->Height) &&
		(vidinfo->depth == (state->GC_Depth >> 3)) &&
		(vidinfo->selected_rgbformat == state->RGBFormat))
	{
		picasso_refresh(monid);
	}
	init_picasso_screen_called = 1;
#ifdef _WIN32
	mman_ResetWatch (gfxmem_bank.start + natmem_offset, gfxmem_bank.allocated_size);
#endif

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
static uae_u32 REGPARAM2 picasso_SetGC (TrapContext *ctx)
{
	lockrtg();
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	/* Fill in some static UAE related structure about this new ModeInfo setting */
	const uaecptr AmigaBoardInfo = trap_get_areg(ctx, 0);
	const uae_u32 border   = trap_get_dreg(ctx, 0);
	const uaecptr modeinfo = trap_get_areg(ctx, 1);

	trap_put_long(ctx, AmigaBoardInfo + PSSO_BoardInfo_ModeInfo, modeinfo);
	trap_put_word(ctx, AmigaBoardInfo + PSSO_BoardInfo_Border, border);

	const uae_u16 w = trap_get_word(ctx, modeinfo + PSSO_ModeInfo_Width);
	if (w != state->Width) {
		state->ModeChanged = true;
	}
	state->Width = w;
	state->VirtualWidth = state->Width; /* in case SetPanning doesn't get called */

	const uae_u16 h = trap_get_word(ctx, modeinfo + PSSO_ModeInfo_Height);
	if (h != state->Height) {
		state->ModeChanged = true;
	}
	state->Height = h;
	state->VirtualHeight = state->Height; /* in case SetPanning doesn't get called */

	const uae_u8 d = trap_get_byte(ctx, modeinfo + PSSO_ModeInfo_Depth);
	if (d != state->GC_Depth && isfullscreen() > 0 && currprefs.rtgmatchdepth) {
		state->ModeChanged = true;
	}
	state->GC_Depth = d;
	state->GC_Flags = trap_get_byte(ctx, modeinfo + PSSO_ModeInfo_Flags);

	state->HLineDBL = 1;
	state->VLineDBL = 1;

	P96TRACE_SETUP((_T("SetGC(%d,%d,%d,%d)\n"), state->Width, state->Height, state->GC_Depth, border));

	state->HostAddress = nullptr;

	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETGC);
	unlockrtg();
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
	if(state->VirtualWidth > state->Width || state->VirtualHeight > state->Height)
		state->BigAssBitmap = 1;
	else
		state->BigAssBitmap = 0;
}

static uae_u32 REGPARAM2 picasso_SetPanning (TrapContext *ctx)
{
	lockrtg();
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	uae_u16 Width = trap_get_dreg(ctx, 0);
	const uaecptr start_of_screen = trap_get_areg(ctx, 1);
	const uaecptr bi = trap_get_areg(ctx, 0);
	const uaecptr bmeptr = trap_get_long(ctx, bi + PSSO_BoardInfo_BitMapExtra);  /* Get our BoardInfo ptr's BitMapExtra ptr */
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
	state->XOffset = static_cast<uae_s16>(trap_get_dreg(ctx, 1) & 0xFFFF);
	state->YOffset = static_cast<uae_s16>(trap_get_dreg(ctx, 2) & 0xFFFF);
	trap_put_word(ctx, bi + PSSO_BoardInfo_XOffset, static_cast<uae_u16>(state->XOffset));
	trap_put_word(ctx, bi + PSSO_BoardInfo_YOffset, static_cast<uae_u16>(state->YOffset));
	state->VirtualWidth = bme_width;
	state->VirtualHeight = bme_height;
	state->RGBFormat = static_cast<RGBFTYPE>(trap_get_dreg(ctx, 7));
	state->BytesPerPixel = GetBytesPerPixel (state->RGBFormat);
	state->BytesPerRow = state->VirtualWidth * state->BytesPerPixel;
	picasso_SetPanningInit(state);

	if (rgbf != state->RGBFormat) {
		setconvert(monid);
	}

	P96TRACE_SETUP((_T("SetPanning(%d, %d, %d) (%dx%d) Start 0x%x, BPR %d Bpp %d RGBF %d\n"),
		Width, state->XOffset, state->YOffset,
		bme_width, bme_height,
		start_of_screen, state->BytesPerRow, state->BytesPerPixel, state->RGBFormat));

	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETPANNING);

	unlockrtg();
	return 1;
}


static uae_u32 picasso_SetSplitPosition(TrapContext *ctx)
{
	lockrtg();
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const uaecptr bi = trap_get_areg(ctx, 0);

	auto pos = static_cast<uae_s16>(trap_get_dreg(ctx, 0));
	trap_put_word(ctx, bi + PSSO_BoardInfo_YSplit, pos);
	pos--;
	if (pos != vidinfo->splitypos) {
		vidinfo->splitypos = pos;
		vidinfo->full_refresh = 1;
	}
	unlockrtg();
	return 1;
}

#ifdef CPU_64_BIT
static void do_xor8(uae_u8 *p, int w, uae_u32 v)
{
	while (ALIGN_POINTER_TO32(p) != 7 && w) {
		*p ^= v;
		p++;
		w--;
	}
	const uae_u64 vv = v | (static_cast<uae_u64>(v) << 32);
	while (w >= 2 * 8) {
		*reinterpret_cast<uae_u64*>(p) ^= vv;
		p += 8;
		*reinterpret_cast<uae_u64*>(p) ^= vv;
		p += 8;
		w -= 2 * 8;
	}
	while (w) {
		*p ^= v;
		p++;
		w--;
	}
}
#else
static void do_xor8(uae_u8 *p, int w, uae_u32 v)
{
	while (ALIGN_POINTER_TO32(p) != 3 && w) {
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
#endif
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
	const uaecptr renderinfo = trap_get_areg(ctx, 1);
	const uae_u32 X = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 Y = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	uae_u32 Width = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	const uae_u32 Height = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	auto mask = static_cast<uae_u8>(trap_get_dreg(ctx, 4));
	const uae_u8 RGBFmt = trap_get_dreg(ctx, 7);
	const int Bpp = GetBytesPerPixel(RGBFmt);
	uae_u32 xorval;
	struct RenderInfo ri{};
	uae_u8 *uae_mem, *rectstart;
	uae_u32 width_in_bytes;
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;

	if (CopyRenderInfoStructureA2U(ctx, renderinfo, &ri)) {
		P96TRACE((_T("InvertRect %dbpp 0x%02x\n"), Bpp, mask));

		if (!validatecoords(ctx, &ri, ri.RGBFormat, &X, &Y, &Width, &Height))
			return 1;

		if (Bpp > 1)
			mask = 0xFF;

		if (!mask) {
			return 1;
		}

		xorval = (mask << 24) | (mask << 16) | (mask << 8) | mask;
		width_in_bytes = Bpp * Width;
		rectstart = uae_mem = ri.Memory + Y * ri.BytesPerRow + X * Bpp;

		for (int lines = 0; lines < Height; lines++, uae_mem += ri.BytesPerRow) {
			do_xor8(uae_mem, static_cast<int>(width_in_bytes), xorval);
		}

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
static uae_u32 REGPARAM2 picasso_FillRect(TrapContext *ctx)
{
	const uaecptr renderinfo = trap_get_areg(ctx, 1);
	const uae_u32 X = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 Y = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	uae_u32 Width = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	const uae_u32 Height = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	uae_u32 Pen = trap_get_dreg(ctx, 4);
	auto Mask = static_cast<uae_u8>(trap_get_dreg(ctx, 5));
	const auto RGBFmt = static_cast<uae_u8>(trap_get_dreg(ctx, 7));
	uae_u8 *oldstart;
	int Bpp;
	struct RenderInfo ri{};
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;

	if (CopyRenderInfoStructureA2U(ctx, renderinfo, &ri)) {
		if (!validatecoords(ctx, &ri, RGBFmt, &X, &Y, &Width, &Height))
			return 1;

		Bpp = GetBytesPerPixel(RGBFmt);

		P96TRACE((_T("FillRect(%d, %d, %d, %d) Pen 0x%x BPP %d BPR %d Mask 0x%02x\n"),
			X, Y, Width, Height, Pen, Bpp, ri.BytesPerRow, Mask));

		if (Bpp > 1 || Mask == 0xFF) {

			/* Do the fill-rect in the frame-buffer */
			do_fillrect_frame_buffer(&ri, static_cast<int>(X), static_cast<int>(Y), static_cast<int>(Width), static_cast<int>(Height), Pen, Bpp);

		} else {

			/* We get here only if Mask != 0xFF */
			Pen &= Mask;
			Mask = ~Mask;
			oldstart = ri.Memory + Y * ri.BytesPerRow + X * Bpp;
			{
				uae_u8 *start = oldstart;
				const uae_u8 *end = start + Height * ri.BytesPerRow;
				for (; start != end; start += ri.BytesPerRow) {
					uae_u8 *p = start;
					for (int cols = 0; cols < Width; cols++) {
						const uae_u32 tmpval = do_get_mem_byte(p + cols) & Mask;
						do_put_mem_byte(p + cols, static_cast<uae_u8>(Pen | tmpval));
					}
				}
			}
		}
	}
	return 1;
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
	uae_u32 srcx;
	uae_u32 srcy;
	uae_u32 dstx;
	uae_u32 dsty;
	uae_u32 width;
	uae_u32 height;
	uae_u8 mask;
	uae_u8 RGBFmt;
	BLIT_OPCODE opcode;
} blitrectdata;

static int BlitRectHelper(TrapContext *ctx)
{
	struct RenderInfo *ri = blitrectdata.ri;
	const struct RenderInfo *dstri = blitrectdata.dstri;
	const uae_u32 srcx = blitrectdata.srcx;
	const uae_u32 srcy = blitrectdata.srcy;
	const uae_u32 dstx = blitrectdata.dstx;
	const uae_u32 dsty = blitrectdata.dsty;
	uae_u32 width = blitrectdata.width;
	const uae_u32 height = blitrectdata.height;
	const uae_u8 RGBFmt = blitrectdata.RGBFmt;
	const uae_u8 mask = blitrectdata.mask;
	const BLIT_OPCODE opcode = blitrectdata.opcode;

	if (!validatecoords(ctx, ri, RGBFmt, &srcx, &srcy, &width, &height))
		return 1;
	if (!validatecoords(ctx, dstri, RGBFmt, &dstx, &dsty, &width, &height))
		return 1;

	uae_u8 Bpp = GetBytesPerPixel(RGBFmt);

	/*
	* If we have no destination RenderInfo, then we're dealing with a single-buffer action, called
	* from picasso_BlitRect().  The code in do_blitrect_frame_buffer() deals with the frame-buffer,
	* while the do_blit() code deals with the visible screen.
	*
	* If we have a destination RenderInfo, then we've been called from picasso_BlitRectNoMaskComplete()
	* and we need to put the results on the screen from the frame-buffer.
	*/
	if (dstri == nullptr || dstri->Memory == ri->Memory) {
		dstri = ri;
	}
	/* Do our virtual frame-buffer memory first */
	do_blitrect_frame_buffer(ri, dstri, srcx, srcy, dstx, dsty, width, height, mask, RGBFmt, opcode);
	return 1;
}

static int BlitRect(TrapContext *ctx, uaecptr ri, uaecptr dstri,
	uae_u32 srcx, uae_u32 srcy, uae_u32 dstx, uae_u32 dsty,
	uae_u32 width, uae_u32 height, uae_u8 mask, uae_u8 RGBFmt, BLIT_OPCODE opcode)
{
	/* Set up the params */
	CopyRenderInfoStructureA2U(ctx, ri, &blitrectdata.ri_struct);
	blitrectdata.ri = &blitrectdata.ri_struct;
	if(dstri) {
		CopyRenderInfoStructureA2U(ctx, dstri, &blitrectdata.dstri_struct);
		blitrectdata.dstri = &blitrectdata.dstri_struct;
	} else {
		blitrectdata.dstri = nullptr;
	}
	blitrectdata.srcx = srcx;
	blitrectdata.srcy = srcy;
	blitrectdata.dstx = dstx;
	blitrectdata.dsty = dsty;
	blitrectdata.width = width;
	blitrectdata.height = height;
	blitrectdata.mask = mask;
	blitrectdata.opcode = opcode;
	blitrectdata.RGBFmt = RGBFmt;

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
	const uaecptr renderinfo = trap_get_areg(ctx, 1);
	const uae_u32 srcx = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 srcy = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	const uae_u32 dstx = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	const uae_u32 dsty = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	const uae_u32 width = static_cast<uae_u16>(trap_get_dreg(ctx, 4));
	const uae_u32 height = static_cast<uae_u16>(trap_get_dreg(ctx, 5));
	const auto  Mask = static_cast<uae_u8>(trap_get_dreg(ctx, 6));
	const auto  RGBFmt = static_cast<uae_u8>(trap_get_dreg(ctx, 7));
	uae_u32 result = 0;

	if (NOBLITTER_BLIT)
		return 0;
	P96TRACE((_T("BlitRect(%d, %d, %d, %d, %d, %d, 0x%02x)\n"), srcx, srcy, dstx, dsty, width, height, Mask));
	result = BlitRect(ctx, renderinfo, 0, srcx, srcy, dstx, dsty, width, height, Mask, RGBFmt, BLIT_SRC);
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
	const uaecptr srcri = trap_get_areg(ctx, 1);
	const uaecptr dstri = trap_get_areg(ctx, 2);
	const uae_u32 srcx = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 srcy = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	const uae_u32 dstx = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	const uae_u32 dsty = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	const uae_u32 width = static_cast<uae_u16>(trap_get_dreg(ctx, 4));
	const uae_u32 height = static_cast<uae_u16>(trap_get_dreg(ctx, 5));
	const auto OpCode = static_cast<BLIT_OPCODE>(trap_get_dreg(ctx, 6) & 0xff);
	const uae_u32 RGBFmt = trap_get_dreg(ctx, 7);
	uae_u32 result = 0;

	if (NOBLITTER_BLIT)
		return 0;

	P96TRACE((_T("BlitRectNoMaskComplete() op 0x%02x, %08x:(%4d,%4d) --> %08x:(%4d,%4d), wh(%4d,%4d)\n"),
		OpCode, trap_get_long(ctx, srcri + PSSO_RenderInfo_Memory), srcx, srcy,
		trap_get_long(ctx, dstri + PSSO_RenderInfo_Memory), dstx, dsty, width, height));
	result = BlitRect(ctx, srcri, dstri, srcx, srcy, dstx, dsty, width, height, 0xFF, RGBFmt, OpCode);
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
		mem[bits] = static_cast<uae_u8>(fgpen);
		break;
	case 2:
		reinterpret_cast<uae_u16*>(mem)[bits] = static_cast<uae_u16>(fgpen);
		break;
	case 3:
		mem[bits * 3 + 0] = fgpen >> 0;
		mem[bits * 3 + 1] = fgpen >> 8;
		mem[bits * 3 + 2] = fgpen >> 16;
		break;
	case 4:
		reinterpret_cast<uae_u32*>(mem)[bits] = fgpen;
		break;
	default: // never
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
static uae_u32 REGPARAM2 picasso_BlitPattern(TrapContext *ctx)
{
	const uaecptr rinf = trap_get_areg(ctx, 1);
	const uaecptr pinf = trap_get_areg(ctx, 2);
	const uae_u32 X = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 Y = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	uae_u32 W = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	const uae_u32 H = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	const auto Mask = static_cast<uae_u8>(trap_get_dreg(ctx, 4));
	const auto RGBFmt = static_cast<uae_u8>(trap_get_dreg(ctx, 7));
	int Bpp = GetBytesPerPixel (RGBFmt);
	int inversion = 0;
	struct RenderInfo ri{};
	struct Pattern pattern;
	uae_u8 *uae_mem;
	int xshift;
	uae_u32 ysize_mask;
	uae_u32 result = 0;
	const uae_u32 rgbmask = rgbfmasks[RGBFmt];

	if (NOBLITTER)
		return 0;

	if(CopyRenderInfoStructureA2U(ctx, rinf, &ri) && CopyPatternStructureA2U(ctx, pinf, &pattern)) {
		if (!validatecoords(ctx, &ri, RGBFmt, &X, &Y, &W, &H))
			return 1;
		if (pattern.Size > 16) {
			return 1;
		}

		Bpp = GetBytesPerPixel(ri.RGBFormat);
		uae_mem = ri.Memory + Y * ri.BytesPerRow + X * Bpp; /* offset with address */

		if (pattern.DrawMode & INVERS)
			inversion = 1;

		pattern.DrawMode &= 0x03;

		const bool indirect = trap_is_indirect();
		uae_u32 fgpen, bgpen;
		P96TRACE((_T("BlitPattern() xy(%d,%d), wh(%d,%d) draw 0x%x, off(%d,%d), ph %d\n"),
			X, Y, W, H, pattern.DrawMode, pattern.XOffset, pattern.YOffset, 1 << pattern.Size));

#if P96TRACING_ENABLED
		DumpPattern(&pattern);
#endif
		ysize_mask = (1 << pattern.Size) - 1;
		xshift = pattern.XOffset & 15;

		fgpen = pattern.FgPen;
		endianswap (&fgpen, Bpp);
		bgpen = pattern.BgPen;
		endianswap (&bgpen, Bpp);

		uae_u16 *tmplbuf = nullptr;
		if (indirect) {
			const int size = 1 << pattern.Size;
			tmplbuf = xcalloc(uae_u16, size);
			trap_get_words(ctx, tmplbuf, pattern.AMemory, 1 << pattern.Size);
		}

		for (int rows = 0; rows < H; rows++, uae_mem += ri.BytesPerRow) {
			const uae_u32 prow = (rows + pattern.YOffset) & ysize_mask;
			unsigned int d;
			uae_u8 *uae_mem2 = uae_mem;

			if (indirect) {
				d = do_get_mem_word(tmplbuf + prow);
			} else {
				d = do_get_mem_word(reinterpret_cast<uae_u16*>(pattern.Memory) + prow);
			}

			if (xshift != 0)
				d = (d << xshift) | (d >> (16 - xshift));

			for (int cols = 0; cols < W; cols += 16, uae_mem2 += Bpp * 16) {
				int bits;
				int max = static_cast<int>(W) - cols;
				uae_u32 data = d;

				max = std::min(max, 16);

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
								PixelWrite(uae_mem2, bits, fgpen, Bpp, Mask);
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
							PixelWrite(uae_mem2, bits, bit_set ? fgpen : bgpen, Bpp, Mask);
						}
						break;
					}
				case COMP:
					{
						for (bits = 0; bits < max; bits++) {
							const int bit_set = data & 0x8000;
							data <<= 1;
							if (bit_set) {
								switch (Bpp)
								{
								case 1:
									{
										uae_mem2[bits] ^= rgbmask & Mask;
									}
									break;
								case 2:
									{
										auto *addr = reinterpret_cast<uae_u16*>(uae_mem2);
										addr[bits] ^= rgbmask;
									}
									break;
								case 3:
									{
										auto *addr = reinterpret_cast<uae_u32*>(uae_mem2 + bits * 3);
										do_put_mem_long (addr, do_get_mem_long (addr) ^ 0x00ffffff);
									}
									break;
								case 4:
									{
										const auto addr = reinterpret_cast<uae_u32*>(uae_mem2);
										addr[bits] ^= rgbmask;
									}
									break;
								default: // never
									break;
								}
							}
						}
						break;
					}
				default: // never
					break;
				}
			}
		}
		xfree(tmplbuf);
	}

	return 1;
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
static uae_u32 REGPARAM2 picasso_BlitTemplate(TrapContext *ctx)
{
	uae_u8 inversion = 0;
	const uaecptr rinf = trap_get_areg(ctx, 1);
	const uaecptr tmpl = trap_get_areg(ctx, 2);
	const uae_u32 X = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 Y = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	uae_u32 W = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	uae_u32 H = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	const auto Mask = static_cast<uae_u16>(trap_get_dreg(ctx, 4));
	const auto RGBFmt = static_cast<uae_u8>(trap_get_dreg(ctx, 7));
	struct Template tmp{};
	struct RenderInfo ri{};
	int bitoffset;
	uae_u8* uae_mem;
	uae_u8 *tmpl_base;
	uae_u32 rgbmask;
	int Bpp;

	if (NOBLITTER)
		return 0;

	if (CopyRenderInfoStructureA2U(ctx, rinf, &ri) && CopyTemplateStructureA2U(ctx, tmpl, &tmp)) {
		if (!validatecoords(ctx, &ri, RGBFmt, &X, &Y, &W, &H))
			return 1;

		rgbmask = rgbfmasks[RGBFmt];
		Bpp = GetBytesPerPixel(RGBFmt);
		uae_mem = ri.Memory + Y * ri.BytesPerRow + X * Bpp; /* offset into address */

		if (tmp.DrawMode & INVERS)
			inversion = 1;

		tmp.DrawMode &= 3;

		uae_u32 fgpen, bgpen;
		const bool indirect = trap_is_indirect();

		P96TRACE((_T("BlitTemplate() xy(%d,%d), wh(%d,%d) draw 0x%x fg 0x%x bg 0x%x \n"),
			X, Y, W, H, tmp.DrawMode, tmp.FgPen, tmp.BgPen));

		bitoffset = tmp.XOffset % 8;

#if P96TRACING_ENABLED && P96TRACING_LEVEL > 0
		DumpTemplate(&tmp, W, H);
#endif

		fgpen = tmp.FgPen;
		endianswap (&fgpen, Bpp);
		bgpen = tmp.BgPen;
		endianswap (&bgpen, Bpp);

		uae_u8 *tmpl_buffer = nullptr;
		if (indirect) {
			const int tmpl_size = H * tmp.BytesPerRow * Bpp;
			tmpl_buffer = xcalloc(uae_u8, tmpl_size + 1);
			trap_get_bytes(ctx, tmpl_buffer, tmp.AMemory, tmpl_size);
			tmpl_base = tmpl_buffer + tmp.XOffset / 8;
		} else {
			tmpl_base = tmp.Memory + tmp.XOffset / 8;
		}

		for (int rows = 0; rows < H; rows++, uae_mem += ri.BytesPerRow, tmpl_base += tmp.BytesPerRow) {
			uae_u8 *uae_mem2 = uae_mem;
			const uae_u8 *tmpl_mem = tmpl_base;
			unsigned int data;

			data = *tmpl_mem;

			for (int cols = 0; cols < W; cols += 8, uae_mem2 += Bpp * 8) {
				uae_u32 byte;
				int max = W - cols;

				max = std::min(max, 8);

				data <<= 8;
				data |= *++tmpl_mem;

				byte = data >> (8 - bitoffset);

				switch (tmp.DrawMode)
				{
				case JAM1:
					{
						for (int bits = 0; bits < max; bits++) {
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
						for (int bits = 0; bits < max; bits++) {
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
						for (int bits = 0; bits < max; bits++) {
							const int bit_set = (byte & 0x80);
							byte <<= 1;
							if (bit_set) {
								switch (Bpp)
								{
								case 1:
									{
										uae_u8 *addr = uae_mem2;
										addr[bits] ^= rgbmask & Mask;
									}
									break;
								case 2:
									{
										const auto addr = reinterpret_cast<uae_u16*>(uae_mem2);
										addr[bits] ^= rgbmask;
									}
									break;
								case 3:
									{
										const auto addr = reinterpret_cast<uae_u32*>(uae_mem2 + bits * 3);
										do_put_mem_long(addr, do_get_mem_long(addr) ^ 0xffffff);
									}
									break;
								case 4:
									{
										const auto addr = reinterpret_cast<uae_u32*>(uae_mem2);
										addr[bits] ^= rgbmask;
									}
									break;
								default: // never
									break;
								}
							}
						}
						break;
					}
				default: // never
					break;
				}
			}
		}
		xfree(tmpl_buffer);
	}

	return 1;
}

/*
* CalculateBytesPerRow:
* a0:	struct BoardInfo
* d0:	uae_u16 Width
* d7:	RGBFTYPE RGBFormat
* This function calculates the amount of bytes needed for a line of
* "Width" pixels in the given RGBFormat.
*/
static uae_u32 REGPARAM2 picasso_CalculateBytesPerRow(TrapContext *ctx)
{
	uae_u16 width = trap_get_dreg(ctx, 0);
	const uae_u32 type = trap_get_dreg(ctx, 7);
	const int bpp = GetBytesPerPixel(type);
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
	const int monid = currprefs.rtgboards[0].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const uae_u32 setstate = trap_get_dreg(ctx, 0);
	P96TRACE_SETUP((_T("SetDisplay(%d)\n"), setstate));
	resetpalette(state);
	atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETDISPLAY);
	return !setstate;
}

void init_hz_p96(int monid)
{
	if (currprefs.rtgvblankrate < 0 || isvsync_rtg()) {
		const float rate = target_getcurrentvblankrate(monid);
		if (rate < 0)
			p96vblank = vblank_hz;
		else
			p96vblank = target_getcurrentvblankrate(monid);
	} else if (currprefs.rtgvblankrate == 0) {
		p96vblank = vblank_hz;
	} else {
		p96vblank = static_cast<float>(currprefs.rtgvblankrate);
	}
	if (p96vblank <= 0)
		p96vblank = 60;
	p96vblank = std::min<float>(p96vblank, 300);
	p96syncrate = static_cast<int>(static_cast<float>(maxvpos_nom) * vblank_hz / p96vblank);
	write_log(_T("RTGFREQ: %d*%.4f = %.4f / %.1f = %d\n"), maxvpos_nom, vblank_hz, static_cast<float>(maxvpos_nom) * vblank_hz, p96vblank, p96syncrate);
}

/* NOTE: Watch for those planeptrs of 0x00000000 and 0xFFFFFFFF for all zero / all one bitmaps !!!! */
static void PlanarToChunky(TrapContext *ctx, struct RenderInfo *ri, struct BitMap *bm,
	uae_u32 srcx, uae_u32 srcy, uae_u32 dstx, uae_u32 dsty,
	uae_u32 width, uae_u32 height, uae_u8 minterm, uae_u8 mask)
{
	uae_u8 *PLANAR[8];
	uaecptr APLANAR[8];
	bool specialplane[8];
	uae_u8 *image = ri->Memory + dstx * GetBytesPerPixel(ri->RGBFormat) + dsty * ri->BytesPerRow;
	int Depth = bm->Depth;
	int bitoffset = srcx & 7;
	bool indirect = trap_is_indirect();

	/* Set up our bm->Planes[] pointers to the right horizontal offset */
	for (int j = 0; j < Depth; j++) {
		specialplane[j] = false;
		if (indirect) {
			uaecptr ap = bm->APlanes[j];
			if (ap != 0 && ap != 0xffffffff) {
				ap += srcx / 8 + srcy * bm->BytesPerRow;
			} else {
				specialplane[j] = true;
			}
			APLANAR[j] = ap;
		} else {
			uae_u8 *p = bm->Planes[j];
			if (p != &all_zeros_bitmap && p != &all_ones_bitmap) {
				p += srcx / 8 + srcy * bm->BytesPerRow;
			} else {
				specialplane[j] = true;
			}
			PLANAR[j] = p;
		}
	}
	uae_u32 mask32 = (mask << 24) | (mask << 16) | (mask << 8) | mask;
	int eol_offset = bm->BytesPerRow - ((width + 7) >> 3);
	bool needin = false;
	if ((minterm != BLIT_FALSE && minterm != BLIT_TRUE && minterm != BLIT_NOTSRC && minterm != BLIT_SRC) || mask != 0xff) {
		needin = true;
	}
	for (int rows = 0; rows < height; rows++, image += ri->BytesPerRow) {

		for (int cols = 0; cols < width; cols += 8) {
			uae_u32 a = 0, b = 0;
			uae_u32 amask = 0, bmask = 0;
			uae_u32 msk = 0xFF;
			int tmp = cols + 8 - width;
			if (tmp > 0) {
				msk <<= tmp;
				if (tmp < 4) {
					bmask = 0xffffffff >> (32 - tmp * 8);
					needin = true;
				} else if (tmp > 4) {
					amask = 0xffffffff >> (64 - tmp * 8);
					bmask = 0xffffffff;
					needin = true;
				} else {
					bmask = 0xffffffff;
					needin = true;
				}
			}
			for (int k = 0; k < Depth; k++) {
				uae_u32 data;
				if (indirect) {
					if (APLANAR[k] == 0) {
						data = 0x00;
					} else if (APLANAR[k] == 0xffffffff) {
						data = 0xFF;
					} else {
						data = static_cast<uae_u8>(trap_get_word(ctx, APLANAR[k]) >> (8 - bitoffset));
						APLANAR[k]++;
					}
				} else {
					if (PLANAR[k] == &all_zeros_bitmap) {
						data = 0x00;
					} else if (PLANAR[k] == &all_ones_bitmap) {
						data = 0xFF;
					} else {
						data = static_cast<uae_u8>(do_get_mem_word(reinterpret_cast<uae_u16*>(PLANAR[k])) >> (8 - bitoffset));
						PLANAR[k]++;
					}
				}
				data &= msk;
				a |= p2ctab[data][0] << k;
				b |= p2ctab[data][1] << k;
			}

			uae_u32 inval0 = 0, inval1 = 0;
			if (needin) {
				inval0 = do_get_mem_long(reinterpret_cast<uae_u32*>(image + cols));
				if (bmask != 0xffffffff) {
					inval1 = do_get_mem_long(reinterpret_cast<uae_u32*>(image + cols + 4));
				}
			}
			uae_u32 invali0 = inval0 ^ rgbfmasks[ri->RGBFormat];
			uae_u32 invali1 = inval1 ^ rgbfmasks[ri->RGBFormat];
			uae_u32 out0 = 0, out1 = 0;

			switch (minterm)
			{
			case BLIT_SRC:
				out0 = a;
				out1 = b;
				break;

			case BLIT_FALSE:
				out0 = 0;
				out1 = 0;
				break;
			case BLIT_TRUE:
				out0 = 0xffffffff;
				out1 = 0xffffffff;
				break;

			case BLIT_NOR:
				out0 = ~(a | inval0);
				out1 = ~(b | inval1);
				break;
			case BLIT_ONLYDST:
				out0 = inval0 & ~a;
				out1 = inval1 & ~b;
				break;
			case BLIT_NOTSRC:
				out0 = ~a;
				out1 = ~b;
				break;
			case BLIT_ONLYSRC:
				out0 = a & invali0;
				out1 = b & invali1;
				break;
			case BLIT_NOTDST:
				out0 = invali0;
				out1 = invali1;
				break;
			case BLIT_EOR:
				out0 = a ^ inval0;
				out1 = b ^ inval1;
				break;
			case BLIT_NAND:
				out0 = ~(a & inval0);
				out1 = ~(b & inval1);
				break;
			case BLIT_AND:
				out0 = a & inval0;
				out1 = b & inval1;
				break;
			case BLIT_NEOR:
				out0 = ~(a ^ inval0);
				out1 = ~(b ^ inval1);
				break;
			case BLIT_DST:
				out0 = inval0;
				out1 = inval1;
				break;
			case BLIT_NOTONLYSRC:
				out0 = (~a) | inval0;
				out1 = (~b) | inval1;
				break;
			case BLIT_NOTONLYDST:
				out0 = a | invali0;
				out1 = b | invali1;
				break;
			case BLIT_OR:
				out0 = a | inval0;
				out1 = b | inval1;
				break;
			default: // never
				break;
			}

			if (mask != 0xff) {
				out0 = (out0 & mask32) | (inval0 & ~mask32);
				out1 = (out1 & mask32) | (inval1 & ~mask32);
			}

			out0 = (out0 & ~amask) | (inval0 & amask);
			out1 = (out1 & ~bmask) | (inval1 & bmask);

			do_put_mem_long(reinterpret_cast<uae_u32*>(image + cols), out0);
			if (bmask != 0xffffffff) {
				do_put_mem_long(reinterpret_cast<uae_u32*>(image + cols + 4), out1);
			}
		}
		for (int j = 0; j < Depth; j++) {
			if (!specialplane[j]) {
				APLANAR[j] += eol_offset;
				PLANAR[j] += eol_offset;
			}
		}
	}
}

static uae_u32 getcim(uae_u8 v, int bpp, int *maxcp, uaecptr acim, uae_u32 *cim, TrapContext *ctx)
{
	// most operations use only low palette values
	// do not fetch and convert whole palette unless needed
	const int maxc = *maxcp;
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
		*maxcp = vc;
	}
	return cim[v];
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
* d6.b: MinTerm
* d7.b: Mask
*
* This function is currently used to blit from planar bitmaps within system memory to chunky bitmaps
* on the board. Watch out for plane pointers that are 0x00000000 (represents a plane with all bits "0")
* or 0xffffffff (represents a plane with all bits "1").
*/
static uae_u32 REGPARAM2 picasso_BlitPlanar2Chunky (TrapContext *ctx)
{
	const uaecptr bm = trap_get_areg(ctx, 1);
	const uaecptr ri = trap_get_areg(ctx, 2);
	const uae_u32 srcx = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 srcy = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	const uae_u32 dstx = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	const uae_u32 dsty = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	const uae_u32 width = static_cast<uae_u16>(trap_get_dreg(ctx, 4));
	const uae_u32 height = static_cast<uae_u16>(trap_get_dreg(ctx, 5));
	const uae_u8 minterm = static_cast<uae_u8>(trap_get_dreg(ctx, 6)) & 0xFF;
	const uae_u8 mask = static_cast<uae_u8>(trap_get_dreg(ctx, 7)) & 0xFF;
	struct RenderInfo local_ri{};
	struct BitMap local_bm{};
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;

	if (CopyRenderInfoStructureA2U(ctx, ri, &local_ri) && CopyBitMapStructureA2U(ctx, bm, &local_bm)) {
		P96TRACE((_T("BlitPlanar2Chunky(%d, %d, %d, %d, %d, %d) Minterm 0x%x, Mask 0x%x, Depth %d\n"),
			srcx, srcy, dstx, dsty, width, height, minterm, mask, local_bm.Depth));
		P96TRACE((_T("P2C - BitMap has %d BPR, %d rows\n"), local_bm.BytesPerRow, local_bm.Rows));
		PlanarToChunky(ctx, &local_ri, &local_bm, srcx, srcy, dstx, dsty, width, height, minterm, mask);
		result = 1;
	}
	return result;
}

/* NOTE: Watch for those planeptrs of 0x00000000 and 0xFFFFFFFF for all zero / all one bitmaps !!!! */
static void PlanarToDirect(TrapContext *ctx, const struct RenderInfo *ri, const struct BitMap *bm,
	uae_u32 srcx, uae_u32 srcy, uae_u32 dstx, uae_u32 dsty,
	uae_u32 width, uae_u32 height, uae_u8 minterm, uae_u8 mask, uaecptr acim)
{
	const int bpp = GetBytesPerPixel(ri->RGBFormat);
	uae_u8 *PLANAR[8];
	uaecptr APLANAR[8];
	bool specialplane[8];
	uae_u8 *image = ri->Memory + dstx * bpp + dsty * ri->BytesPerRow;
	const int Depth = bm->Depth;
	const bool indirect = trap_is_indirect();
	int maxc = -1;
	uae_u32 cim[256];
	const uae_u8 depthmask = (1 << Depth) - 1;

	if(!bpp)
		return;

	/* Set up our bm->Planes[] pointers to the right horizontal offset */
	for (int j = 0; j < Depth; j++) {
		specialplane[j] = false;
		if (indirect) {
			uaecptr ap = bm->APlanes[j];
			if (ap != 0 && ap != 0xffffffff) {
				ap += srcx / 8 + srcy * bm->BytesPerRow;
			} else if (ap == 0) {
				specialplane[j] = true;
				PLANAR[j] = &all_zeros_bitmap;
			} else {
				specialplane[j] = true;
				PLANAR[j] = &all_ones_bitmap;
			}
			APLANAR[j] = ap;
		} else {
			uae_u8 *p = bm->Planes[j];
			if (p != &all_zeros_bitmap && p != &all_ones_bitmap) {
				p += srcx / 8 + srcy * bm->BytesPerRow;
			} else {
				specialplane[j] = true;
			}
			PLANAR[j] = p;
		}
	}

	uae_u8 *planebuf = nullptr;
	const int planebuf_width = (width + 1) & ~1;
	if (indirect) {
		planebuf = xmalloc(uae_u8, planebuf_width * Depth);
	}

	const int eol_offset = bm->BytesPerRow - ((width + (srcx & 7)) >> 3);
	for (int rows = 0; rows < height; rows++, image += ri->BytesPerRow) {
		uae_u8 *image2 = image;
		uae_u32 bitoffs = 7 - (srcx & 7);

		if (indirect) {
			for (int k = 0; k < Depth; k++) {
				if (!specialplane[k]) {
					PLANAR[k] = planebuf + planebuf_width * k;
					trap_get_bytes(ctx, PLANAR[k], APLANAR[k], planebuf_width);
				}
			}
		}

		for (int cols = 0; cols < width; cols ++) {
			uae_u8 v = 0;
			for (int k = 0; k < Depth; k++) {
				if (PLANAR[k] == &all_ones_bitmap)
					v |= 1 << k;
				else if (PLANAR[k] != &all_zeros_bitmap) {
					v |= ((*PLANAR[k] >> bitoffs) & 1) << k;
				}
			}
			v &= depthmask;
			const uae_u8 vi = (v ^ mask) & depthmask;

			uae_u32 inval = 0;
			if (minterm != BLIT_FALSE && minterm != BLIT_TRUE && minterm != BLIT_NOTSRC && minterm != BLIT_SRC) {
				switch (bpp)
				{
				case 2:
					inval = reinterpret_cast<uae_u16*>(image2)[0];
					break;
				case 3:
					inval = image2[0] | (image2[1] << 8) | (image2[2] << 16);
					break;
				case 4:
					inval = reinterpret_cast<uae_u32*>(image2)[0];
					break;
				default: // never
					break;
				}
			}
			const uae_u32 invali = inval ^ rgbfmasks[ri->RGBFormat];

			uae_u32 out = 0;

			switch (minterm)
			{
			// CIM[B]
			case BLIT_SRC:
				out = getcim(v, bpp, &maxc, acim, cim, ctx);
				break;

			// CIM[0]
			case BLIT_FALSE:
				out = getcim(0, bpp, &maxc, acim, cim, ctx);
				break;
			// CIM[~0]
			case BLIT_TRUE:
				out = getcim((1 << Depth) - 1, bpp, &maxc, acim, cim, ctx);
				break;

			case BLIT_NOR: // ~(C | CIM[B])
				out = ~(getcim(v, bpp, &maxc, acim, cim, ctx) | inval);
				break;
			case BLIT_ONLYDST: // C & CIM[~B]
				out = getcim(vi, bpp, &maxc, acim, cim, ctx) & inval;
				break;
			case BLIT_NOTSRC: // CIM[~B]
				out = getcim(vi, bpp, &maxc, acim, cim, ctx);
				break;
			case BLIT_ONLYSRC: // (~C) & CIM[B]
				out = getcim(v, bpp, &maxc, acim, cim, ctx) & invali;
				break;
			case BLIT_NOTDST: // ~C
				out = invali;
				break;
			case BLIT_EOR: // C ^ CIM[B]
				out = getcim(v, bpp, &maxc, acim, cim, ctx) ^ inval;
				break;
			case BLIT_NAND: // ~(C & CIM[B])
				out = ~(getcim(v, bpp, &maxc, acim, cim, ctx) & inval);
				break;
			case BLIT_AND: // C & CIM[B]
				out = getcim(v, bpp, &maxc, acim, cim, ctx) & inval;
				break;
			case BLIT_NEOR: // ~(C ^ CIM[B])
				out = ~(getcim(v, bpp, &maxc, acim, cim, ctx) ^ inval);
				break;
			case BLIT_DST: // C
				out = inval;
				break;
			case BLIT_NOTONLYSRC: // C | CIM[~B]
				out = getcim(vi, bpp, &maxc, acim, cim, ctx) | inval;
				break;
			case BLIT_NOTONLYDST: // (~C) | CIM[B]
				out = getcim(v, bpp, &maxc, acim, cim, ctx) | invali;
				break;
			case BLIT_OR: // C | CIM[B]
				out = getcim(v, bpp, &maxc, acim, cim, ctx) | inval;
				break;
			default: // never
				break;
			}

			switch (bpp)
			{
			case 2:
				reinterpret_cast<uae_u16*>(image2)[0] = static_cast<uae_u16>(out);
				image2 += 2;
				break;
			case 3:
				image2[0] = out >> 0;
				image2[1] = out >> 8;
				image2[2] = out >> 16;
				image2 += 3;
				break;
			case 4:
				reinterpret_cast<uae_u32*>(image2)[0] = out;
				image2 += 4;
				break;
			default: // never
				break;
			}

			bitoffs--;
			bitoffs &= 7;
			if (bitoffs == 7) {
				for (int k = 0; k < Depth; k++) {
					if (!specialplane[k]) {
						PLANAR[k]++;
						APLANAR[k]++;
					}
				}
			}
		}

		for (int i = 0; i < Depth; i++) {
			if (!specialplane[i]) {
				PLANAR[i] += eol_offset;
				APLANAR[i] += eol_offset;
			}
		}
	}
	if (planebuf) {
		xfree(planebuf);
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
static uae_u32 REGPARAM2 picasso_BlitPlanar2Direct(TrapContext *ctx)
{
	const uaecptr bm = trap_get_areg(ctx, 1);
	const uaecptr ri = trap_get_areg(ctx, 2);
	const uaecptr cim = trap_get_areg(ctx, 3);
	const uae_u32 srcx = static_cast<uae_u16>(trap_get_dreg(ctx, 0));
	const uae_u32 srcy = static_cast<uae_u16>(trap_get_dreg(ctx, 1));
	const uae_u32 dstx = static_cast<uae_u16>(trap_get_dreg(ctx, 2));
	const uae_u32 dsty = static_cast<uae_u16>(trap_get_dreg(ctx, 3));
	const uae_u32 width = static_cast<uae_u16>(trap_get_dreg(ctx, 4));
	const uae_u32 height = static_cast<uae_u16>(trap_get_dreg(ctx, 5));
	const auto minterm = static_cast<uae_u8>(trap_get_dreg(ctx, 6));
	const auto Mask = static_cast<uae_u8>(trap_get_dreg(ctx, 7));
	struct RenderInfo local_ri{};
	struct BitMap local_bm{};
	uae_u32 result = 0;

	if (NOBLITTER)
		return 0;

	if (CopyRenderInfoStructureA2U(ctx, ri, &local_ri) && CopyBitMapStructureA2U(ctx, bm, &local_bm)) {
		P96TRACE((_T("BlitPlanar2Direct(%d, %d, %d, %d, %d, %d) Minterm 0x%x, Mask 0x%x, Depth %d\n"),
			srcx, srcy, dstx, dsty, width, height, minterm, Mask, local_bm.Depth));
		PlanarToDirect(ctx, &local_ri, &local_bm, srcx, srcy, dstx, dsty, width, height, minterm, Mask, cim);
		result = 1;
	}
	return result;
}

void picasso_statusline(int monid, uae_u8 *dst)
{
	const struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const struct picasso96_state_struct *state = &picasso96_state[monid];
	int y, slx, sly;
	int dst_height, dst_width, pitch;

	dst_height = state->Height;
	dst_height = std::min(dst_height, vidinfo->height);
	dst_width = state->Width;
	dst_width = std::min(dst_width, vidinfo->width);
	pitch = vidinfo->rowbytes;
	statusline_getpos(monid, &slx, &sly, state->Width, dst_height);
	statusline_render(monid, dst + sly * pitch, vidinfo->pixbytes, pitch, dst_width, dst_height, p96rc, p96gc, p96bc, nullptr);
	const int m = statusline_get_multiplier(monid) / 100;
	for (y = 0; y < TD_TOTAL_HEIGHT * m; y++) {
		uae_u8 *buf = dst + (y + sly) * pitch;
		draw_status_line_single(monid, buf, vidinfo->pixbytes, y, dst_width, p96rc, p96gc, p96bc, nullptr);
	}
}

static void copyrow(int monid, uae_u8 *src, uae_u8 *dst, int x, int y, int width, int srcbytesperrow, int srcpixbytes, int dx, int dy, int dstbytesperrow, int dstpixbytes, const int *convert_modep, const uae_u32 *p96_rgbx16p)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	struct picasso96_state_struct *state = &picasso96_state[monid];
	int endx = x + width, endx4;
	int dstpix = dstpixbytes;
	int srcpix = srcpixbytes;
	uae_u32 *clut = vidinfo->clut;
	int convert_mode = convert_modep[0];

	if (y >= vidinfo->splitypos && vidinfo->splitypos >= 0) {
		src = gfxmem_banks[monid]->start + natmem_offset;
		if (state->dualclut) {
			clut += 256;
		}
		y -= vidinfo->splitypos;
		if (convert_mode != convert_modep[1]) {
			int bpp1 = GetBytesPerPixel(vidinfo->dacrgbformat[0]);
			int bpp2 = GetBytesPerPixel(vidinfo->dacrgbformat[1]);
			srcbytesperrow = srcbytesperrow * bpp2 / bpp1;
			convert_mode = convert_modep[1];
		}
	}

	if (dy < 0 || dy >= state->Height) {
		return;
	}

	uae_u8 *src2 = src + y * srcbytesperrow;
	uae_u8 *dst2 = dst + dy * dstbytesperrow;

#ifdef AMIBERRY
	// In Amiberry, we only use these two modes, so we can optimize this as much as possible
	// Use memcpy for copying memory
	if (convert_mode == RGBFB_B8G8R8A8_32 || convert_mode == RGBFB_R5G6B5PC_16) {
		std::memcpy(dst2 + dx * dstpix, src2 + x * srcpix, width * dstpix);
		return;
	}
#else
	// native match?
	//if (currprefs.gfx_api) {
		switch (convert_mode) {

			case RGBFB_B8G8R8A8_32:
			case RGBFB_R5G6B5PC_16:
				memcpy(dst2 + dx * dstpix, src2 + x * srcpix, width * dstpix);
				return;
		}
	//} else {
#endif
		endx4 = endx & ~3;

		switch (convert_mode) {
			/* 24bit->32bit */
			case RGBFB_R8G8B8_32:
				while (x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = (src2[x * 3 + 0] << 16) | (src2[x * 3 + 1] << 8) | (src2[x * 3 + 2] << 0);
					x++;
					dx++;
				}
				break;
			case RGBFB_B8G8R8_32:
				while (x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = reinterpret_cast<uae_u32*>(src2 + x * 3)[0] & 0x00ffffff;
					x++;
					dx++;
				}
				break;

				/* 32bit->32bit */
			case RGBFB_R8G8B8A8_32:
				while (x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = (src2[x * 4 + 0] << 16) | (src2[x * 4 + 1] << 8) | (src2[x * 4 + 2] << 0);
					x++;
					dx++;
				}
				break;
			case RGBFB_A8R8G8B8_32:
				while (x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = (src2[x * 4 + 1] << 16) | (src2[x * 4 + 2] << 8) | (src2[x * 4 + 3] << 0);
					x++;
					dx++;
				}
				break;
			case RGBFB_A8B8G8R8_32:
				while (x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = reinterpret_cast<uae_u32*>(src2)[x] >> 8;
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
			case RGBFB_B5G5R5PC_32: {
				while ((x & 3) && x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
					x++;
					dx++;
				}
				while (x < endx4) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
					x++;
					dx++;
				}
				while (x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
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
			case RGBFB_R5G6B5PC_16: {
				while ((x & 3) && x < endx) {
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
					x++;
					dx++;
				}
				while (x < endx4) {
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
					x++;
					dx++;
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
					x++;
					dx++;
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
					x++;
					dx++;
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
					x++;
					dx++;
				}
				while (x < endx) {
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
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
					reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[(((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) |
					                                     (((b >> 3) & 0x1f) << 0)];
					x++;
					dx++;
				}
				break;
			case RGBFB_B8G8R8_16:
				while (x < endx) {
					uae_u32 v;
					v = reinterpret_cast<uae_u32*>(&src2[x * 3])[0] >> 8;
					reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[(((v >> (8 + 3)) & 0x1f) << 11) |
					                                     (((v >> (0 + 2)) & 0x3f) << 5) |
					                                     (((v >> (16 + 3)) & 0x1f) << 0)];
					x++;
					dx++;
				}
				break;

				/* 32bit->16bit */
			case RGBFB_R8G8B8A8_16:
				while (x < endx) {
					uae_u32 v;
					v = reinterpret_cast<uae_u32*>(src2)[x];
					reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[(((v >> (0 + 3)) & 0x1f) << 11) |
					                                     (((v >> (8 + 2)) & 0x3f) << 5) |
					                                     (((v >> (16 + 3)) & 0x1f) << 0)];
					x++;
					dx++;
				}
				break;
			case RGBFB_A8R8G8B8_16:
				while (x < endx) {
					uae_u32 v;
					v = reinterpret_cast<uae_u32*>(src2)[x];
					reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[(((v >> (8 + 3)) & 0x1f) << 11) |
					                                     (((v >> (16 + 2)) & 0x3f) << 5) |
					                                     (((v >> (24 + 3)) & 0x1f) << 0)];
					x++;
					dx++;
				}
				break;
			case RGBFB_A8B8G8R8_16:
				while (x < endx) {
					uae_u32 v;
					v = reinterpret_cast<uae_u32*>(src2)[x];
					reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[(((v >> (24 + 3)) & 0x1f) << 11) |
					                                     (((v >> (16 + 2)) & 0x3f) << 5) |
					                                     (((v >> (8 + 3)) & 0x1f) << 0)];
					x++;
					dx++;
				}
				break;
			case RGBFB_B8G8R8A8_16:
				while (x < endx) {
					uae_u32 v;
					v = reinterpret_cast<uae_u32*>(src2)[x];
					reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[(((v >> (16 + 3)) & 0x1f) << 11) |
					                                     (((v >> (8 + 2)) & 0x3f) << 5) |
					                                     (((v >> (0 + 3)) & 0x1f) << 0)];
					x++;
					dx++;
				}
				break;

				/* 8bit->32bit */
			case RGBFB_CLUT_RGBFB_32: {
				while ((x & 3) && x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
				}
				while (x < endx4) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
				}
				while (x < endx) {
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
				}
			}
				break;

				/* 8bit->16bit */
			case RGBFB_CLUT_RGBFB_16: {
				while ((x & 3) && x < endx) {
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
				}
				while (x < endx4) {
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
				}
				while (x < endx) {
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
					x++;
					dx++;
				}
			}
				break;
			default: // never
				break;
		}
	//}
}

static uae_u16 yuvtorgb(uae_u8 yx, uae_u8 ux, uae_u8 vx)
{
	const int y = yx - 16;
	const int u = ux - 128;
	const int v = vx - 128;
	int r = (298 * y + 409 * v + 128) >> (8 + 3);
	int g = (298 * y - 100 * u - 208 * v + 128) >> (8 + 3);
	int b = (298 * y + 516 * u + 128) >> (8 + 3);
	r = std::max(r, 0);
	r = std::min(r, 31);
	g = std::max(g, 0);
	g = std::min(g, 31);
	b = std::max(b, 0);
	b = std::min(b, 31);
	return (r << 10) | (g << 5) | b;
}

#define CKCHECK if (!ck \
	|| (screenpixbytes == 4 && colorkey == ((uae_u32*)srcs)[dx]) \
	|| (screenpixbytes == 2 && (uae_u16)colorkey == ((uae_u16*)srcs)[dx]) \
	|| (screenpixbytes == 1 && (uae_u8)colorkey == ((uae_u8*)srcs)[dx]) \
	|| (screenpixbytes == 3 && ckbytes[0] == srcs[dx * 3 + 0] && ckbytes[1] == srcs[dx * 3 + 1] && ckbytes[2] == srcs[dx * 3 + 2]))

void copyrow_scale(int monid, uae_u8 *src, uae_u8 *src_screen, uae_u8 *dst,
	int sx, int sy, int sxadd, int width, int srcbytesperrow, int srcpixbytes,
	int screenbytesperrow, int screenpixbytes,
	int dx, int dy, int dstwidth, int dstheight, int dstbytesperrow, int dstpixbytes,
	bool ck, uae_u32 colorkey,
	int convert_mode, uae_u32 *p96_rgbx16p, uae_u32 *clut, bool yuv_swap)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
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
	default: break;
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
					reinterpret_cast<uae_u32*>(dst2)[dx] = (src2[x * 3 + 0] << 16) | (src2[x * 3 + 1] << 8) | (src2[x * 3 + 2] << 0);
				dx++;
			}
			break;
		case RGBFB_B8G8R8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = reinterpret_cast<uae_u32*>(src2 + x * 3)[0] & 0x00ffffff;
				dx++;
			}
			break;

		/* 32bit->32bit */
		case RGBFB_R8G8B8A8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = (src2[x * 4 + 0] << 16) | (src2[x * 4 + 1] << 8) | (src2[x * 4 + 2] << 0);
				dx++;
			}
			break;
		case RGBFB_A8R8G8B8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = (src2[x * 4 + 1] << 16) | (src2[x * 4 + 2] << 8) | (src2[x * 4 + 3] << 0);
				dx++;
			}
			break;
		case RGBFB_A8B8G8R8_32:
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = reinterpret_cast<uae_u32*>(src2)[x] >> 8;
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
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]];
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
			uae_u32 val = reinterpret_cast<uae_u32*>(src2)[sx >> 8];
			uae_u32 oldval = val ^ 1;
			while (sx < endx) {
				x = sx >> 8;
				if (oldsx != x) {
					uae_u32 val = reinterpret_cast<uae_u32*>(src2)[x];
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
						reinterpret_cast<uae_u32*>(dst2)[dx] = outval1;
					}
				} else {
					CKCHECK
					{
						if (docalc2) {
							uae_u16 out = yuvtorgb(y1, u, v);
							outval2 = p96_rgbx16p[out];
							docalc2 = false;
						}
						reinterpret_cast<uae_u32*>(dst2)[dx] = outval2;
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
				uae_u32 val = reinterpret_cast<uae_u32*>(src2)[x];
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
						reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 128) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y2, u, v);
						reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 64) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y1, u, v);
						reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[out];
					}
				} else {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y0, u, v);
						reinterpret_cast<uae_u32*>(dst2)[dx] = p96_rgbx16p[out];
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
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = static_cast<uae_u16>(p96_rgbx16p[reinterpret_cast<uae_u16*>(src2)[x]]);
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
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u16*>(dst2)[dx] = clut[src2[x]];
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
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx4) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
				dx++;
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
				dx++;
			}
			while (sx < endx) {
				x = sx >> 8;
				sx += sxadd;
				CKCHECK
					reinterpret_cast<uae_u32*>(dst2)[dx] = clut[src2[x]];
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
			uae_u32 val = reinterpret_cast<uae_u32*>(src2)[sx >> 8];
			uae_u32 oldval = val ^ 1;
			while (sx < endx) {
				x = sx >> 8;
				if (x != oldsx) {
					val = reinterpret_cast<uae_u32*>(src2)[x];
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
						reinterpret_cast<uae_u16*>(dst2)[dx] = outval1;
					}
					CKCHECK
					{
						if (docalc2) {
							uae_u16 out = yuvtorgb(y1, u, v);
							outval2 = p96_rgbx16p[out];
							docalc2 = false;
						}
						reinterpret_cast<uae_u16*>(dst2)[dx] = outval2;
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
				uae_u32 val = reinterpret_cast<uae_u32*>(src2)[x];
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
						reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 128) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y2, u, v);
						reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[out];
					}
				} else if (fr >= 64) {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y1, u, v);
						reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[out];
					}
				} else {
					CKCHECK
					{
						uae_u16 out = yuvtorgb(y0, u, v);
						reinterpret_cast<uae_u16*>(dst2)[dx] = p96_rgbx16p[out];
					}
				}
				sx += sxadd;
				dx++;
			}
		}
		break;
		default: // never
			break;
	}
}

static void picasso_flushoverlay(const int index, uae_u8 *src, const int scr_offset, uae_u8 *dst)
{
	const int monid = currprefs.rtgboards[index].monitor_id;
	const struct picasso96_state_struct *state = &picasso96_state[monid];
	const struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];

	if (overlay_w <= 0)
		return;
	if (overlay_h <= 0)
		return;

	const uae_u8 *vram_end = src + gfxmem_banks[0]->allocated_size;
	const uae_u8 *dst_end = dst + vidinfo->height * vidinfo->rowbytes;
	uae_u8 *s = src + overlay_vram_offset;
	uae_u8 *ss = src + scr_offset;
	const int mx = overlay_src_width_in * 256 / overlay_w;
	const int my = overlay_src_height_in * 256 / overlay_h;
	int y = 0;

	int split = 0;
	if (vidinfo->splitypos >= 0) {
		split = vidinfo->splitypos;
	}

	for (int dy = 0; dy < overlay_h; dy++) {
		if (s + (y >> 8) * overlay_src_width_in * overlay_pix > vram_end) {
			break;
		}
		if (ss + (overlay_y + dy + split) * state->BytesPerRow > vram_end) {
			break;
		}
		if (dst + (overlay_y + dy + split) * vidinfo->rowbytes > dst_end) {
			break;
		}
		copyrow_scale(monid, s, ss, dst,
			0, (y >> 8), mx, overlay_src_width_in, overlay_src_width * overlay_pix, overlay_pix,
			state->BytesPerRow, state->BytesPerPixel,
			overlay_x, overlay_y + dy + split, vidinfo->width, vidinfo->height, vidinfo->rowbytes, vidinfo->pixbytes,
			overlay_occlusion != 0, overlay_color,
			overlay_convert, p96_rgbx16_ovl, overlay_clut, true);
		y += my;
	}
}

void fb_copyrow(int monid, uae_u8 *src, uae_u8 *dst, int x, int y, int width, int srcpixbytes, int dy)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	struct picasso96_state_struct *state = &picasso96_state[monid];
	copyrow(monid, src, dst, x, y, width, 0, srcpixbytes,
		x, dy, picasso_vidinfo[monid].rowbytes, picasso_vidinfo[monid].pixbytes,
		vidinfo->picasso_convert, p96_rgbx16);
}

static void copyallinvert(int monid, uae_u8 *src, uae_u8 *dst, int pwidth, int pheight, int srcbytesperrow, int srcpixbytes, int dstbytesperrow, int dstpixbytes, const int *mode_convert)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];

	const int w = pwidth * dstpixbytes;
	uae_u8 *src2 = src;
	for (int y = 0; y < pheight; y++) {
		for (int x = 0; x < w; x++)
			src2[x] ^= 0xff;
		copyrow(monid, src, dst, 0, y, pwidth, srcbytesperrow, srcpixbytes, 0, y, dstbytesperrow, dstpixbytes, mode_convert, p96_rgbx16);
		for (int x = 0; x < w; x++)
			src2[x] ^= 0xff;
		src2 += srcbytesperrow;
	}
}

static void copyall(int monid, uae_u8 *src, uae_u8 *dst, int pwidth, int pheight, int srcbytesperrow, int srcpixbytes, int dstbytesperrow, int dstpixbytes, const int *mode_convert)
{
	for (int y = 0; y < pheight; y++) {
		copyrow(monid, src, dst, 0, y, pwidth, srcbytesperrow, srcpixbytes, 0, y, dstbytesperrow, dstpixbytes, mode_convert, p96_rgbx16);
	}
}

uae_u8 *uaegfx_getrtgbuffer(const int monid, int *widthp, int *heightp, int *pitch, int *depth, uae_u8 *palette)
{
	const struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	const struct picasso96_state_struct *state = &picasso96_state[monid];
	uae_u8 *src = gfxmem_banks[monid]->start + natmem_offset;
	const int off = state->XYOffset - gfxmem_banks[monid]->start;
	int width, height, pixbytes;
	uae_u8 *dst;
	int convert[2];

	if (!vidinfo->extra_mem)
		return nullptr;

	width = state->VirtualWidth;
	height = state->VirtualHeight;
	pixbytes = state->BytesPerPixel == 1 && palette ? 1 : 4;
	if (!width || !height || !pixbytes)
		return nullptr;

	dst = xmalloc (uae_u8, width * height * pixbytes);
	if (!dst)
		return nullptr;
	convert[0] = getconvert (state->RGBFormat, pixbytes);
	convert[1] = convert[0];
	alloc_colors_picasso(8, 8, 8, 16, 8, 0, state->RGBFormat, p96_rgbx16); // BGR

	copyall (monid, src + off, dst, width, height, state->BytesPerRow, state->BytesPerPixel, width * pixbytes, pixbytes, convert);
	if (pixbytes == 1) {
		for (int i = 0; i < 256; i++) {
			palette[i * 3 + 0] = state->CLUT[i].Red;
			palette[i * 3 + 1] = state->CLUT[i].Green;
			palette[i * 3 + 2] = state->CLUT[i].Blue;
		}
	}

	gfx_set_picasso_colors(monid, state->RGBFormat);

	*widthp = width;
	*heightp = height;
	*pitch = width * pixbytes;
	*depth = pixbytes * 8;

	return dst;
}
void uaegfx_freertgbuffer(int monid, uae_u8 *dst)
{
	xfree (dst);
}

void picasso_invalidate(int monid, int x, int y, int w, int h)
{
	DX_Invalidate(&AMonitors[monid], x, y, w, h);
}

static void picasso_flushpixels(int index, uae_u8 *src, int off, bool render)
{
	int monid = currprefs.rtgboards[index].monitor_id;
	struct picasso96_state_struct *state = &picasso96_state[monid];
	uae_u8 *src_start[2];
	uae_u8 *src_end[2];
#ifdef _WIN32
	ULONG_PTR gwwcnt;
#endif
	int pwidth = state->Width > state->VirtualWidth ? state->VirtualWidth : state->Width;
	int pheight = state->Height > state->VirtualHeight ? state->VirtualHeight : state->Height;
	int maxy = -1;
	int miny = pheight - 1;
	int flushlines = 0, matchcount = 0;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	bool overlay_updated = false;

#ifdef _WIN32
	src_start[0] = src + (off & ~gwwpagemask[index]);
	src_end[0] = src + ((off + state->BytesPerRow * pheight + gwwpagesize[index] - 1) & ~gwwpagemask[index]);
	if (vidinfo->splitypos >= 0) {
		src_end[0] = src + ((off + state->BytesPerRow * vidinfo->splitypos + gwwpagesize[index] - 1) & ~gwwpagemask[index]);
		src_start[1] = src;
		src_end[1] = src + ((state->BytesPerRow * (pheight - vidinfo->splitypos) + gwwpagesize[index] - 1) & ~gwwpagemask[index]);
#else
	src_start[0] = src + off;
	src_end[0] = src + (off + state->BytesPerRow * pheight);
	if (vidinfo->splitypos >= 0) {
		src_end[0] = src + (off + state->BytesPerRow * vidinfo->splitypos);
		src_start[1] = src;
		src_end[1] = src + state->BytesPerRow * (pheight - vidinfo->splitypos);
#endif
	} else {
		src_start[1] = src_end[1] = nullptr;
	}
#ifdef _WIN32
	if (!vidinfo->extra_mem || !gwwbuf[index] || (src_start[0] >= src_end[0] && src_start[1] >= src_end[1])) {
#else
	if (!vidinfo->extra_mem || (src_start[0] >= src_end[0] && src_start[1] >= src_end[1])) {
#endif
		return;
	}

	if (flashscreen) {
		vidinfo->full_refresh = 1;
	}
	if (vidinfo->full_refresh || vidinfo->rtg_clear_flag)
		vidinfo->full_refresh = -1;

	uae_u8 *dstp = nullptr;
	for (;;) {
		uae_u8 *dst = nullptr;
		bool dofull;
#ifdef _WIN32
		gwwcnt = 0;
#endif
		if (doskip() && p96skipmode == 1) {
			break;
		}
#ifdef _WIN32
		if (!index && overlay_vram && overlay_active) {
			ULONG ps;
			gwwcnt = gwwbufsize[index];
			uae_u8 *ovr_start = src + (overlay_vram_offset & ~gwwpagemask[index]);
			uae_u8 *ovr_end = src + ((overlay_vram_offset + overlay_src_width * overlay_src_height * overlay_pix + gwwpagesize[index] - 1) & ~gwwpagemask[index]);
			mman_GetWriteWatch(ovr_start, ovr_end - ovr_start, gwwbuf[index], &gwwcnt, &ps);
			overlay_updated = gwwcnt > 0;
		}
#endif

		for (int split = 0; split < 2; split++) {
			auto regionsize = static_cast<uae_u32>(src_end[split] - src_start[split]);
			if (src_start[split] == nullptr && src_end[split] == nullptr) {
				break;
			}

			if (vidinfo->full_refresh < 0 || overlay_updated) {
#ifdef _WIN32
				gwwcnt = regionsize / gwwpagesize[index] + 1;
#endif
				vidinfo->full_refresh = 1;
#ifdef _WIN32
				for (int i = 0; i < gwwcnt; i++)
					gwwbuf[index][i] = src_start[split] + i * gwwpagesize[index];
#endif
			} else {
#ifdef _WIN32
				ULONG ps;
				gwwcnt = gwwbufsize[index];
				if (mman_GetWriteWatch(src_start[split], regionsize, gwwbuf[index], &gwwcnt, &ps))
					continue;
#endif
			}
#ifdef _WIN32
			matchcount += (int)gwwcnt;

			if (gwwcnt == 0) {
				continue;
			}

			dofull = gwwcnt >= (regionsize / gwwpagesize[index]) * 80 / 100;
#else
			dofull = true;
#endif

			if (!dstp) {
				dstp = gfx_lock_picasso(monid, dofull);
			}
			if (dstp == nullptr) {
				continue;
			}
			dst = dstp;

#ifndef AMIBERRY // we never set maxwidth/maxheight, so this won't work
			// safety check
			if (pwidth > vidinfo->maxwidth) {
				pwidth = vidinfo->maxwidth;
			}
			if (pheight > vidinfo->maxheight) {
				pheight = vidinfo->maxheight;
			}
#endif

			if (!split && vidinfo->rtg_clear_flag) {
				uae_u8 *p2 = dst;
				for (int h = 0; h < vidinfo->maxheight; h++) {
					memset(p2, 0, vidinfo->maxwidth * vidinfo->pixbytes);
					p2 += vidinfo->rowbytes;
				}
				vidinfo->rtg_clear_flag--;
			}

			dst += vidinfo->offset;

			if (doskip() && p96skipmode == 2) {
				continue;
			}

			if (dofull) {
				if (flashscreen != 0) {
					copyallinvert(monid, src + off, dst, pwidth, pheight,
						state->BytesPerRow, state->BytesPerPixel,
						vidinfo->rowbytes, vidinfo->pixbytes,
						vidinfo->picasso_convert);
				} else {
					copyall(monid, src + off, dst, pwidth, pheight,
						state->BytesPerRow, state->BytesPerPixel,
						vidinfo->rowbytes, vidinfo->pixbytes,
						vidinfo->picasso_convert);
				}
				miny = 0;
				maxy = pheight;
				flushlines = -1;
				break;
			}

			if (split) {
				off = 0;
			}
#ifdef _WIN32
			for (int i = 0; i < gwwcnt; i++) {
				uae_u8 *p = (uae_u8 *)gwwbuf[index][i];

				if (p >= src_start[split] && p < src_end[split]) {
					int y, x, realoffset;

					if (p >= src + off) {
						realoffset = (int)(p - (src + off));
					} else {
						realoffset = 0;
					}

					y = realoffset / state->BytesPerRow;
					if (split) {
						y += vidinfo->splitypos;
					}
					if (y < pheight) {
						int w = (gwwpagesize[index] + state->BytesPerPixel - 1) / state->BytesPerPixel;
						x = (realoffset % state->BytesPerRow) / state->BytesPerPixel;
						if (x < pwidth) {
							copyrow(monid, src + off, dst, x, y, pwidth - x,
								state->BytesPerRow, state->BytesPerPixel,
								x, y, vidinfo->rowbytes, vidinfo->pixbytes,
								vidinfo->picasso_convert, p96_rgbx16);
							flushlines++;
						}
						w = (gwwpagesize[index] - (state->BytesPerRow - x * state->BytesPerPixel) + state->BytesPerPixel - 1) / state->BytesPerPixel;
						if (y < miny) {
							miny = y;
						}
						y++;
						while (y < pheight && w > 0) {
							int maxw = w > pwidth ? pwidth : w;
							copyrow(monid, src + off, dst, 0, y, maxw,
								state->BytesPerRow, state->BytesPerPixel,
								0, y, vidinfo->rowbytes, vidinfo->pixbytes,
								vidinfo->picasso_convert, p96_rgbx16);
							w -= maxw;
							y++;
							flushlines++;
						}
						if (y > maxy) {
							maxy = y;
						}
					}

				}

			}
#endif
		}
		break;
	}

	if (!index && overlay_vram && overlay_active && (flushlines || overlay_updated)) {
		if (dstp == nullptr) {
			dstp = gfx_lock_picasso(monid, false);
		}
		if (dstp) {
			picasso_flushoverlay(index, src, off, dstp);
		}
	}

	//if (0 && flushlines) {
	//	write_log (_T("%d:%d\n"), flushlines, matchcount);
	//}

	if (currprefs.leds_on_screen & STATUSLINE_RTG) {
		if (dstp == nullptr) {
			dstp = gfx_lock_picasso(monid, false);
		}
		if (dstp) {
			maxy = vidinfo->height;
			if (miny > vidinfo->height - TD_TOTAL_HEIGHT)
				miny = vidinfo->height - TD_TOTAL_HEIGHT;
#ifdef AMIBERRY
			picasso_statusline(monid, dstp);
#endif
		}
	}
	if (maxy >= 0) {
		if (doskip () && p96skipmode == 4) {
			;
		} else {
			picasso_invalidate(monid, 0, miny, pwidth, maxy - miny);
		}
	}

	if (dstp || render) {
		gfx_unlock_picasso(monid, render);
	}

#ifdef _WIN32
	if (dstp && gwwcnt) {
#else
	if (dstp) {
#endif
		vidinfo->full_refresh = 0;
	}
}

static int render_thread(void *v)
{
	render_thread_state = 1;
	for (;;) {
		int idx = read_comm_pipe_int_blocking(render_pipe);
		if (idx == -1)
			break;
		idx &= 0xff;
		const int monid = currprefs.rtgboards[idx].monitor_id;
		struct amigadisplay *ad = &adisplays[monid];
		if (ad->picasso_on && ad->picasso_requested_on) {
			lockrtg();
			if (ad->picasso_requested_on) {
				const struct picasso96_state_struct *state = &picasso96_state[monid];
				picasso_flushpixels(idx, gfxmem_banks[idx]->start + natmem_offset, state->XYOffset - gfxmem_banks[idx]->start, false);
				ad->pending_render = true;
			}
			unlockrtg();
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
	dummy_lgeti, dummy_wgeti,
	ABFLAG_RAM | ABFLAG_RTG | ABFLAG_DIRECTACCESS, 0, 0
};
extern addrbank gfxmem2_bank;
MEMORY_FUNCTIONS(gfxmem2);
addrbank gfxmem2_bank = {
	gfxmem2_lget, gfxmem2_wget, gfxmem2_bget,
	gfxmem2_lput, gfxmem2_wput, gfxmem2_bput,
	gfxmem2_xlate, gfxmem2_check, nullptr, nullptr, _T("RTG RAM #2"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_RAM | ABFLAG_RTG | ABFLAG_DIRECTACCESS, 0, 0
};
extern addrbank gfxmem3_bank;
MEMORY_FUNCTIONS(gfxmem3);
addrbank gfxmem3_bank = {
	gfxmem3_lget, gfxmem3_wget, gfxmem3_bget,
	gfxmem3_lput, gfxmem3_wput, gfxmem3_bput,
	gfxmem3_xlate, gfxmem3_check, nullptr, nullptr, _T("RTG RAM #3"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_RAM | ABFLAG_RTG | ABFLAG_DIRECTACCESS, 0, 0
};
extern addrbank gfxmem4_bank;
MEMORY_FUNCTIONS(gfxmem4);
addrbank gfxmem4_bank = {
	gfxmem4_lget, gfxmem4_wget, gfxmem4_bget,
	gfxmem4_lput, gfxmem4_wput, gfxmem4_bput,
	gfxmem4_xlate, gfxmem4_check, nullptr, nullptr, _T("RTG RAM #4"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_RAM | ABFLAG_RTG | ABFLAG_DIRECTACCESS, 0, 0
};
addrbank *gfxmem_banks[MAX_RTG_BOARDS];

/* Call this function first, near the beginning of code flow
* Place in InitGraphics() which seems reasonable...
* Also put it in reset_drawing() for safe-keeping.  */
void InitPicasso96(int monid)
{
	struct picasso96_state_struct *state = &picasso96_state[monid];
	gfxmem_banks[0] = &gfxmem_bank;
	gfxmem_banks[1] = &gfxmem2_bank;
	gfxmem_banks[2] = &gfxmem3_bank;
	gfxmem_banks[3] = &gfxmem4_bank;

	//fastscreen
	oldscr = 0;
	//fastscreen
	memset (state, 0, sizeof (struct picasso96_state_struct));

	for (int i = 0; i < 256; i++) {
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

static uae_u32 REGPARAM2 picasso_SetInterrupt (TrapContext *ctx)
{
	uaecptr bi = trap_get_areg(ctx, 0);
	const uae_u32 onoff = trap_get_dreg(ctx, 0);
	interrupt_enabled = static_cast<int>(onoff);
	//write_log (_T("Picasso_SetInterrupt(%08x,%d)\n"), bi, onoff);
	return onoff;
}

static uaecptr uaegfx_vblankname, uaegfx_portsname;
static void initvblankABI (TrapContext *ctx, uaecptr base, uaecptr ABI)
{
	for (int i = 0; i < 22; i++)
		trap_put_byte(ctx, ABI + PSSO_BoardInfo_HardInterrupt + i, trap_get_byte(ctx, base + CARD_PORTSIRQ + i));
	ABI_interrupt = ABI;
}
static void initvblankirq (TrapContext *ctx, uaecptr base)
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

static uae_u32 REGPARAM2 picasso_SetClock(TrapContext *ctx)
{
	uaecptr bi = trap_get_areg(ctx, 0);
	P96TRACE_SETUP((_T("SetClock\n")));
	return 0;
}

static uae_u32 REGPARAM2 picasso_SetMemoryMode(TrapContext *ctx)
{
	uaecptr bi = trap_get_areg(ctx, 0);
	uae_u32 rgbformat = trap_get_dreg(ctx, 7);
	P96TRACE_SETUP((_T("SetMemoryMode\n")));
	return 0;
}

#if USE_OVERLAY

#define OVERLAY_COOKIE 0x12345678

static uaecptr gettag(TrapContext *ctx, uaecptr p, uae_u32 *tagp, uae_u32 *valp)
{
	for (;;) {
		uae_u32 tag = trap_get_long(ctx, p);
		uae_u32 val = trap_get_long(ctx, p + 4);
#if OVERLAY_DEBUG > 2
		write_log(_T("TAG %08x: %08x %08x\n"), p, tag, val);
#endif
		if (tag == TAG_DONE)
			return 0;
		if (tag == TAG_IGNORE) {
			p += 8;
			continue;
		}
		if (tag == TAG_SKIP) {
			p += (val + 1) * 8;
			continue;
		}
		if (tag == TAG_MORE) {
			p = val;
			continue;
		}
		*tagp = tag;
		*valp = val;
		return p + 8;
	}
}
static void settag(TrapContext *ctx, uaecptr p, uae_u32 v)
{
	trap_put_long(ctx, p, v);
#if OVERLAY_DEBUG > 2
	write_log(_T(" - %08x -> %08x\n"), p, v);
#endif
}

static void overlaygettag(TrapContext *ctx, uae_u32 tag, uae_u32 val)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[0];

	switch (tag)
	{
	case FA_Active:
		overlay_active = static_cast<int>(val);
		break;
	case FA_Occlusion:
		overlay_occlusion = static_cast<int>(val);
		break;
	case FA_Left:
		overlay_x = static_cast<int>(val);
		break;
	case FA_Top:
		overlay_y = static_cast<int>(val);
		break;
	case FA_Width:
		overlay_w = static_cast<int>(val);
		break;
	case FA_Height:
		overlay_h = static_cast<int>(val);
		break;
	case FA_SourceWidth:
		overlay_src_width_in = static_cast<int>(val);
		break;
	case FA_SourceHeight:
		overlay_src_height_in = static_cast<int>(val);
		break;
	case FA_Format:
		overlay_format = val;
		break;
	case FA_ModeFormat:
		overlay_modeformat = val;
		break;
	case FA_Brightness:
		overlay_brightness = val;
		break;
	case FA_ModeInfo:
		overlay_modeinfo = val;
		break;
	case FA_Color:
		overlay_color = val;
		overlay_color_unswapped = val;
		endianswap(&overlay_color, picasso96_state[0].BytesPerPixel);
		break;
	case FA_ClipLeft:
		overlay_clipleft = static_cast<int>(val);
		break;
	case FA_ClipTop:
		overlay_cliptop = static_cast<int>(val);
		break;
	case FA_ClipWidth:
		overlay_clipwidth = static_cast<int>(val);
		break;
	case FA_ClipHeight:
		overlay_clipheight = static_cast<int>(val);
		break;
	case FA_Colors32:
		{
			while(val) {
				uae_u8 tmpbuf[256 * 3 * 4 + 4];
				const uae_u32 v = trap_get_long(ctx, val);
				val += 4;
				const uae_u16 num = v >> 16;
				if (num > 256)
					break;
				const uae_u16 first = v & 0xffff;
				trap_get_bytes(ctx, tmpbuf, val, num * 3 * 4 + 4);
				for (int i = 0; i < num && (first + i) < 256; i++) {
					overlay_clutc[first + i].Red = tmpbuf[i * 3 * 4 + 0];
					overlay_clutc[first + i].Green = tmpbuf[i * 3 * 4 + 4];
					overlay_clutc[first + i].Blue = tmpbuf[i * 3 * 4 + 8];
				}
				if (!tmpbuf[num])
					break;
				val += num * 3 * 4;
			}
			if (picasso_palette(overlay_clutc, overlay_clut))
				vidinfo->full_refresh = 1;
		}
		break;
	case FA_Colors:
		{
			while (val) {
				uae_u8 tmpbuf[4 * 2];
				trap_get_bytes(ctx, tmpbuf, val, 4 * 2);
				const uae_u16 idx = (tmpbuf[0] << 8) | tmpbuf[1];
				if (idx >= 256)
					break;
				overlay_clutc[idx].Red = ((tmpbuf[3] & 15) << 4) | (tmpbuf[3] & 15);
				overlay_clutc[idx].Green = ((tmpbuf[5] & 15) << 4) | (tmpbuf[5] & 15);
				overlay_clutc[idx].Blue = ((tmpbuf[7] & 15) << 4) | (tmpbuf[7] & 15);
				val += 4 * 2;
			}
			if (picasso_palette(overlay_clutc, overlay_clut))
				vidinfo->full_refresh = 1;
		}
		break;
	default: break;
	}
}

static void overlaysettag(TrapContext *ctx, uae_u32 tag, uae_u32 val)
{
	switch (tag)
	{
	case FA_Active:
		settag(ctx, val, overlay_active);
		break;
	case FA_Occlusion:
		settag(ctx, val, overlay_occlusion);
		break;
	case FA_MinWidth:
	case FA_MinHeight:
		settag(ctx, val, 16);
		break;
	case FA_MaxWidth:
	case FA_MaxHeight:
		settag(ctx, val, 4096);
		break;
	case FA_Left:
		settag(ctx, val, overlay_x);
		break;
	case FA_Top:
		settag(ctx, val, overlay_y);
		break;
	case FA_Width:
		settag(ctx, val, overlay_w);
		break;
	case FA_Height:
		settag(ctx, val, overlay_h);
		break;
	case FA_SourceWidth:
		settag(ctx, val, overlay_src_width_in);
		break;
	case FA_SourceHeight:
		settag(ctx, val, overlay_src_height_in);
		break;
	case FA_BitMap:
		settag(ctx, val, overlay_bitmap);
		break;
	case FA_Format:
		settag(ctx, val, overlay_format);
		break;
	case FA_ModeFormat:
		settag(ctx, val, overlay_modeformat);
		break;
	case FA_Brightness:
		settag(ctx, val, overlay_brightness);
		break;
	case FA_ModeInfo:
		settag(ctx, val, overlay_modeinfo);
		break;
	case FA_Color:
		settag(ctx, val, overlay_color_unswapped);
		break;
	case FA_ClipLeft:
		settag(ctx, val, overlay_clipleft);
		break;
	case FA_ClipTop:
		settag(ctx, val, overlay_cliptop);
		break;
	case FA_ClipWidth:
		settag(ctx, val, overlay_clipwidth);
		break;
	case FA_ClipHeight:
		settag(ctx, val, overlay_clipheight);
		break;
	default: break;
	}
}

static uae_u32 REGPARAM2 picasso_SetFeatureAttrs(TrapContext *ctx)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[0];
	uaecptr bi = trap_get_areg(ctx, 0);
	uaecptr featuredata = trap_get_areg(ctx, 1);
	uaecptr tagp = trap_get_areg(ctx, 2);
	uae_u32 type = trap_get_dreg(ctx, 0);

#if OVERLAY_DEBUG > 1
	write_log(_T("picasso_SetFeatureAttrs %08x %d\n"), tagp, type);
#endif
	for (;;) {
		uae_u32 tag, val;
		tagp = gettag(ctx, tagp, &tag, &val);
		if (!tagp)
			break;
#if OVERLAY_DEBUG > 1
		write_log(_T("picasso_SetFeatureAttrs %08x tag %08x (%d) %08x\n"), tagp - 8, tag, tag & 0x7fffffff, val);
#endif
		overlaygettag(ctx, tag, val);
	}
#if OVERLAY_DEBUG > 1
	write_log(_T("RTG Overlay: X=%d Y=%d W=%d H=%d Act=%d SrcW=%d SrcH=%d BPP=%d VRAM=%08x\n"),
		overlay_x, overlay_y, overlay_w, overlay_h,
		overlay_active, overlay_src_width_in, overlay_src_height_in,
		overlay_pix, overlay_vram);
#endif
	vidinfo->full_refresh = 1;
	return 0;
}

static uae_u32 REGPARAM2 picasso_GetFeatureAttrs(TrapContext *ctx)
{
	uaecptr bi = trap_get_areg(ctx, 0);
	uaecptr featuredata = trap_get_areg(ctx, 1);
	uaecptr tagp = trap_get_areg(ctx, 2);
	uae_u32 type = trap_get_dreg(ctx, 0);

#if OVERLAY_DEBUG > 1
	write_log(_T("picasso_GetFeatureAttrs %08x %d\n"), tagp, type);
#endif
	for (;;) {
		uae_u32 tag, val;
		tagp = gettag(ctx, tagp, &tag, &val);
		if (!tagp)
			break;
		overlaysettag(ctx, tag, val);
#if OVERLAY_DEBUG > 1
		write_log(_T("picasso_GetFeatureAttrs %08x tag %08x (%d) %08x\n"), tagp, tag, tag & 0x7fffffff, val);
#endif
	}
	return 0;
}

// Tags from picassoiv driver..
static constexpr uae_u32 ovltags[] = {
	ABMA_RGBFormat, 0,
	ABMA_Clear, 1,
	ABMA_Displayable, 1,
	ABMA_Visible, 1,
	ABMA_Colors, 0,
	ABMA_Colors32, 0,
	ABMA_ConstantBytesPerRow, 0,
	ABMA_NoMemory, 0,
	ABMA_RenderFunc, 0,
	ABMA_SaveFunc, 0,
	ABMA_UserData, 0,
	ABMA_Alignment, 0,
	TAG_DONE, 0
};
#define ALLOC_TAG_SIZE (13 * 8)

// "VBInt PicassoIV (0)" -0x14
// AllocCardMem=274 AllocBitMap=506 SetColorArray=286
// CLUTEntry=578

static uae_u32 REGPARAM2 picasso_CreateFeature(TrapContext *ctx)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[0];
	const struct picasso96_state_struct *state = &picasso96_state[0];
	uaecptr bi = trap_get_areg(ctx, 0);
	uae_u32 type = trap_get_dreg(ctx, 0);
	uaecptr tagp = trap_get_areg(ctx, 1);

#if OVERLAY_DEBUG
	write_log(_T("picasso_CreateFeature type %d %08x %08x\n"), type, bi, tagp);
#endif
	if (type != SFT_MEMORYWINDOW)
		return 0;
	if (overlay_vram)
		return 0;
	memcpy(overlay_clutc, state->CLUT, sizeof(struct MyCLUTEntry) * 256);
	overlay_src_width_in = -1;
	overlay_src_height_in = -1;
	overlay_format = 0;
	for (;;) {
		uae_u32 tag, val;
		tagp = gettag(ctx, tagp, &tag, &val);
		if (!tagp)
			break;
#if OVERLAY_DEBUG
		write_log(_T("picasso_CreateFeature tag %08x (%d) %08x\n"), tag, tag & 0x7fffffff, val);
#endif
		overlaygettag(ctx, tag, val);
	}
	if (overlay_src_width_in < 16 || overlay_src_height_in < 16) {
#if OVERLAY_DEBUG
		write_log(_T("picasso_CreateFeature overlay width or height too small (%d*%d)\n"), overlay_src_width, overlay_src_height);
#endif
		return 0;
	}
	if (overlay_src_width_in > vidinfo->width || overlay_src_height_in > vidinfo->height) {
#if OVERLAY_DEBUG
		write_log(_T("picasso_CreateFeature overlay width or height too large (%d*%d)\n"), overlay_src_width, overlay_src_height);
#endif
		return 0;
	}
	if (overlay_format < RGBFB_CLUT || overlay_format >= RGBFB_MaxFormats) {
#if OVERLAY_DEBUG
		write_log(_T("picasso_CreateFeature overlay unknown format %d\n"), overlay_format);
#endif
		return 0;
	}
	overlay_pix = GetBytesPerPixel(overlay_format);
	if (!overlay_pix) {
#if OVERLAY_DEBUG
		write_log(_T("picasso_CreateFeature overlay bytes per bit is zero (format %d)\n"), overlay_format);
#endif
		return 0;
	}
	const uaecptr overlay_tagmem = uae_AllocMem(ctx, ALLOC_TAG_SIZE, 65536, trap_get_long(ctx, 4));
	if (!overlay_tagmem)
		return 0;
	const uaecptr func = trap_get_long(ctx, bi + PSSO_BoardInfo_AllocBitMap);
	for (int i = 0; ovltags[i]; i += 2) {
		trap_put_long(ctx, overlay_tagmem + i * 4 + 0, ovltags[i + 0]);
		if (i == 0) {
			trap_put_long(ctx, overlay_tagmem + i * 4 + 4, overlay_format);
		} else {
			trap_put_long(ctx, overlay_tagmem + i * 4 + 4, ovltags[i + 1]);
		}
	}
	trap_call_add_areg(ctx, 0, bi);
	trap_call_add_dreg(ctx, 0, overlay_src_width_in);
	trap_call_add_dreg(ctx, 1, overlay_src_height_in);
	trap_call_add_areg(ctx, 1, overlay_tagmem);
	overlay_bitmap = trap_call_func(ctx, func);
	uae_FreeMem(ctx, overlay_tagmem, ALLOC_TAG_SIZE, trap_get_long(ctx, 4));
	if (!overlay_bitmap) {
		return 0;
	}
	// probably should use GetBitMapAttr() but function is not documented enough.
	overlay_src_width = trap_get_word(ctx, overlay_bitmap + 0) / overlay_pix;
	overlay_src_height = trap_get_word(ctx, overlay_bitmap + 2);
	overlay_vram = trap_get_long(ctx, overlay_bitmap + 8);
	overlay_vram_offset = static_cast<int>(overlay_vram - gfxmem_banks[0]->start);
	overlay_convert = getconvert(static_cast<int>(overlay_format), picasso_vidinfo[0].pixbytes);
	if (!p96_rgbx16_ovl)
		p96_rgbx16_ovl = xcalloc(uae_u32, 65536);
	int of = static_cast<int>(overlay_format);
	if (of == RGBFB_Y4U2V2 || of == RGBFB_Y4U1V1)
		of = RGBFB_R5G5B5PC;
	alloc_colors_picasso(8, 8, 8, 16, 8, 0, of, p96_rgbx16_ovl); // BGR
#if OVERLAY_DEBUG
	write_log(_T("picasso_CreateFeature overlay bitmap %08x, vram %08x (%dx%d)\n"),
		overlay_bitmap, overlay_vram, overlay_src_width, overlay_src_height);
#endif
	picasso_palette(overlay_clutc, overlay_clut);
	vidinfo->full_refresh = 1;
	return OVERLAY_COOKIE;
}

static uae_u32 REGPARAM2 picasso_DeleteFeature(TrapContext *ctx)
{
	const uaecptr bi = trap_get_areg(ctx, 0);
	uae_u32 type = trap_get_dreg(ctx, 0);
	uaecptr featuredata = trap_get_areg(ctx, 1);

#if OVERLAY_DEBUG
	write_log(_T("picasso_DeleteFeature type %d data %08x\n"), type, featuredata);
#endif
	if (type != SFT_MEMORYWINDOW)
		return 0;
	if (featuredata != OVERLAY_COOKIE)
		return 0;
	if (!overlay_bitmap)
		return 0;
	const uaecptr func = trap_get_long(ctx, bi + PSSO_BoardInfo_FreeBitMap);
	trap_call_add_areg(ctx, 0, bi);
	trap_call_add_areg(ctx, 1, overlay_bitmap);
	trap_call_add_areg(ctx, 2, 0);
	trap_call_func(ctx, func);
	overlay_bitmap = 0;
	overlay_vram = 0;
	overlay_active = 0;
	return 1;
}

#endif

#define PUTABI(func) \
	if (ABI) \
		trap_put_long(ctx, ABI + func, here ()); \
		save_rom_absolute(ABI + func);

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

#define RTGCALL2X(func,call) \
	PUTABI (func); \
	calltrap (deftrap2 (call, TRAPFLAG_EXTRA_STACK, NULL)); \
	dw (RTS);

#define RTGCALLDEFAULT(func,funcdef) \
	PUTABI (func); \
	dw (0x2f28); \
	dw (funcdef); \
	dw (RTS);

#define RTGCALLONLYDEFAULT(func,funcdef,unused) \
	PUTABI (func); \
	dw (0x2f28); \
	dw (funcdef); \
	dw (RTS);

#define RTGNONE(func) \
	if (ABI) \
		trap_put_long(ctx, ABI + func, start); \
		save_rom_absolute(ABI + func);

static void inituaegfxfuncs(TrapContext *ctx, uaecptr start, uaecptr ABI)
{
	if (uaegfx_old || !ABI)
		return;
	org (start);

	dw (RTS);
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
	PUTABI (PSSO_BoardInfo_GetCompatibleFormats);
	dw (0x203c);
	dl (RGBMASK_8BIT | RGBMASK_15BIT | RGBMASK_16BIT | RGBMASK_24BIT | RGBMASK_32BIT | (USE_OVERLAY ? (RGBFF_Y4U2V2 | RGBFF_Y4U1V1) : 0));
	dw (RTS);

	/* CalculateBytesPerRow (optimized) */
	PUTABI (PSSO_BoardInfo_CalculateBytesPerRow);
	dl(0x0c400140); // cmp.w #320,d0
	const uaecptr addr = here();
	dw(0);
	calltrap (deftrap (picasso_CalculateBytesPerRow));
	const uaecptr addr2 = here();
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

	if (USE_HARDWARESPRITE && currprefs.rtg_hardwaresprite) {
		RTGCALL2(PSSO_BoardInfo_SetSprite, picasso_SetSprite);
		RTGCALL2(PSSO_BoardInfo_SetSpritePosition, picasso_SetSpritePosition);
		RTGCALL2(PSSO_BoardInfo_SetSpriteImage, picasso_SetSpriteImage);
		RTGCALL2(PSSO_BoardInfo_SetSpriteColor, picasso_SetSpriteColor);
	}

	RTGCALLDEFAULT(PSSO_BoardInfo_ScrollPlanar, PSSO_BoardInfo_ScrollPlanarDefault);
	RTGCALLDEFAULT(PSSO_BoardInfo_UpdatePlanar, PSSO_BoardInfo_UpdatePlanarDefault);
	RTGCALLDEFAULT(PSSO_BoardInfo_DrawLine, PSSO_BoardInfo_DrawLineDefault);

	if (USE_OVERLAY && currprefs.rtg_overlay) {
		RTGCALL2(PSSO_BoardInfo_GetFeatureAttrs, picasso_GetFeatureAttrs);
		RTGCALL2(PSSO_BoardInfo_SetFeatureAttrs, picasso_SetFeatureAttrs);
		RTGCALL2X(PSSO_BoardInfo_CreateFeature, picasso_CreateFeature);
		RTGCALL2X(PSSO_BoardInfo_DeleteFeature, picasso_DeleteFeature);
	}

	if (USE_VGASCREENSPLIT && currprefs.rtg_vgascreensplit) {
		RTGCALL2(PSSO_SetSplitPosition, picasso_SetSplitPosition);
	}

	if (USE_DACSWITCH && currprefs.rtg_dacswitch) {
		RTGCALL2(PSSO_BoardInfo_GetCompatibleDACFormats, picasso_GetCompatibleDACFormats);
		RTGCALL2(PSSO_BoardInfo_CoerceMode, picasso_CoerceMode);
	}

	if (currprefs.rtg_hardwareinterrupt)
		RTGCALL2(PSSO_BoardInfo_SetInterrupt, picasso_SetInterrupt);

	write_log (_T("uaegfx.card magic code: %08X-%08X BI=%08X\n"), start, here (), ABI);

	if (ABI && currprefs.rtg_hardwareinterrupt)
		initvblankABI(ctx, uaegfx_base, ABI);
}

static void picasso_reset2(int monid)
{
	struct picasso96_state_struct *state = &picasso96_state[monid];
	if (!monid && currprefs.rtg_multithread) {
		if (!render_pipe) {
			render_pipe = xmalloc(smp_comm_pipe, 1);
			init_comm_pipe(render_pipe, 10, 1);
		}
#ifdef AMIBERRY
		if (render_cs == nullptr) {
			uae_sem_init(&render_cs, 0, -1);
		}
#endif
		if (render_thread_state <= 0) {
			render_thread_state = 0;
			uae_start_thread(_T("rtg"), render_thread, nullptr, &render_tid);
		}
	}

	lockrtg();

	rtg_index = -1;
	if (savestate_state != STATE_RESTORE) {
		uaegfx_base = 0;
		uaegfx_old = 0;
		uaegfx_active = 0;
		overlay_bitmap = 0;
		overlay_vram = 0;
		overlay_active = 0;
		interrupt_enabled = 0;
		reserved_gfxmem = 0;
		resetpalette(state);
		state->dualclut = false;
		state->advDragging = false;
		InitPicasso96(monid);
	}

	if (is_uaegfx_active() && currprefs.rtgboards[0].monitor_id > 0) {
		close_rtg(currprefs.rtgboards[0].monitor_id, true);
	}

	for (auto & adisplay : adisplays) {
		struct amigadisplay *ad = &adisplay;
		ad->picasso_requested_on = false;
	}

	unlockrtg();
}

static void picasso_reset(int hardreset)
{
	for (int i = 0; i < MAX_AMIGADISPLAYS; i++) {
		picasso_reset2(i);
	}
}

static void picasso_free()
{
	if (render_thread_state > 0) {
		write_comm_pipe_int(render_pipe, -1, 0);
		while (render_thread_state >= 0) {
			Sleep(10);
		}
#ifdef AMIBERRY
		destroy_comm_pipe(render_pipe);
		xfree(render_pipe);
		render_pipe = nullptr;
		uae_sem_destroy(&render_cs);
		render_cs = nullptr;
#endif
		render_thread_state = 0;
	}
}

void uaegfx_install_code (uaecptr start)
{
	uaegfx_rom = start;
	org (start);
	inituaegfxfuncs(nullptr, start, 0);

	device_add_reset(picasso_reset);
	device_add_hsync(picasso_handle_hsync);
	device_add_exit(picasso_free, nullptr);
}

#define UAEGFX_VERSION 3
#define UAEGFX_REVISION 4

static uae_u32 REGPARAM2 gfx_open(TrapContext *ctx)
{
	trap_put_word(ctx, uaegfx_base + 32, trap_get_word(ctx, uaegfx_base + 32) + 1);
	return uaegfx_base;
}
static uae_u32 REGPARAM2 gfx_close(TrapContext *ctx)
{
	trap_put_word(ctx, uaegfx_base + 32, trap_get_word(ctx, uaegfx_base + 32) - 1);
	return 0;
}
static uae_u32 REGPARAM2 gfx_expunge(TrapContext *ctx)
{
	return 0;
}

static uaecptr uaegfx_card_install (TrapContext *ctx, uae_u32 extrasize)
{
	uae_u32 functable, datatable, a2;
	uaecptr openfunc, closefunc, expungefunc;
	uaecptr findcardfunc, initcardfunc;
	uaecptr exec = trap_get_long(ctx, 4);

	if (uaegfx_old || !(gfxmem_bank.flags & ABFLAG_MAPPED))
		return 0;

	uaegfx_resid = ds (_T("UAE Graphics Card 4.0"));
	uaegfx_vblankname = ds (_T("UAE Graphics Card VBLANK"));
	uaegfx_portsname = ds (_T("UAE Graphics Card PORTS"));

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
	CallLib (ctx, exec, -0x18c); /* AddLibrary */
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

	if (currprefs.rtg_hardwareinterrupt)
		initvblankirq(ctx, uaegfx_base);

	write_log (_T("uaegfx.card %d.%d init @%08X\n"), UAEGFX_VERSION, UAEGFX_REVISION, uaegfx_base);
	uaegfx_active = 1;

	return uaegfx_base;
}

uae_u32 picasso_demux (uae_u32 arg, TrapContext *ctx)
{
	const uae_u32 num = trap_get_long(ctx, trap_get_areg(ctx, 7) + 4);

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
	default: return 0;
	}

	return 0;
}

static uae_u32 p96_restored_flags;

void restore_p96_finish ()
{
	struct amigadisplay *ad = &adisplays[0];
	struct picasso96_state_struct *state = &picasso96_state[0];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[0];

	init_alloc (nullptr, 0);
	if (uaegfx_rom && boardinfo) {
		inituaegfxfuncs(nullptr, uaegfx_rom, boardinfo);
		ad->picasso_requested_on = !!(p96_restored_flags & 1);
		vidinfo->picasso_active = ad->picasso_requested_on;

		if (overlay_vram) {
			overlay_vram_offset = static_cast<int>(overlay_vram - gfxmem_banks[0]->start);
			overlay_convert = getconvert(static_cast<int>(overlay_format), picasso_vidinfo[0].pixbytes);
			if (!p96_rgbx16_ovl)
				p96_rgbx16_ovl = xcalloc(uae_u32, 65536);
			alloc_colors_picasso(8, 8, 8, 16, 8, 0, overlay_format, p96_rgbx16_ovl); // BGR
			picasso_palette(overlay_clutc, overlay_clut);
			overlay_color = overlay_color_unswapped;
			overlay_pix = GetBytesPerPixel(overlay_format);
			endianswap(&overlay_color, picasso96_state[0].BytesPerPixel);
		}
		if (cursorvisible) {
			setspriteimage(nullptr, boardinfo);
		}

		set_config_changed();
	}
}

uae_u8 *restore_p96 (uae_u8 *src)
{
	struct amigadisplay *ad = &adisplays[0];
	struct picasso96_state_struct *state = &picasso96_state[0];
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[0];
	uae_u32 flags;

	if (restore_u32 () != 2)
		return src;
	InitPicasso96(0);
	p96_restored_flags = flags = restore_u32 ();
	hwsprite = !!(flags & 8);
	cursorvisible = !!(flags & 16);
	init_picasso_screen_called = 0;
	set_gc_called = !!(flags & 2);
	vidinfo->set_panning_called = !!(flags & 4);
	interrupt_enabled = !!(flags & 32);
	changed_prefs.rtgboards[0].rtgmem_size = restore_u32 ();
	state->Address = restore_u32 ();
	state->RGBFormat = static_cast<RGBFTYPE>(restore_u32());
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
	for (auto& i : cursorrgb)
		i = restore_u32();
	if (flags & 64) {
		for (auto& i : state->CLUT)
		{
			i.Red = restore_u8 ();
			i.Green = restore_u8 ();
			i.Blue = restore_u8 ();
		}
	}
	if (flags & 128) {
		overlay_bitmap = restore_u32();
		overlay_vram = restore_u32();
		overlay_format = restore_u32();
		overlay_modeformat = restore_u32();
		overlay_modeinfo = restore_u32();
		overlay_color_unswapped = restore_u32();
		overlay_active = restore_u8();
		overlay_occlusion = restore_u8();
		overlay_x = restore_u16();
		overlay_y = restore_u16();
		overlay_w = restore_u16();
		overlay_h = restore_u16();
		overlay_src_width_in = restore_u16();
		overlay_src_height_in = restore_u16();
		overlay_src_width = restore_u16();
		overlay_src_height = restore_u16();
		overlay_clipleft = restore_u16();
		overlay_cliptop = restore_u16();
		overlay_clipwidth = restore_u16();
		overlay_clipheight = restore_u16();
		overlay_brightness = restore_u16();
		for (auto& i : overlay_clutc)
		{
			i.Red = restore_u8();
			i.Green = restore_u8();
			i.Blue = restore_u8();
		}
	}
	state->HostAddress = nullptr;
	state->HLineDBL = 1;
	state->VLineDBL = 1;
	picasso_SetPanningInit(state);
	state->Extent = state->Address + state->BytesPerRow * state->VirtualHeight;
	return src;
}

uae_u8 *save_p96 (size_t *len, uae_u8 *dstptr)
{
	const struct amigadisplay *ad = &adisplays[0];
	const struct picasso96_state_struct *state = &picasso96_state[0];
	const struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[0];
	uae_u8 *dstbak, *dst;
	int i;

	if (currprefs.rtgboards[0].rtgmem_size == 0)
		return nullptr;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 2 * 3 * 256 + 1000);
	save_u32 (2);
	save_u32 ((ad->picasso_on ? 1 : 0) | (set_gc_called ? 2 : 0) | (vidinfo->set_panning_called ? 4 : 0) |
		(hwsprite ? 8 : 0) | (cursorvisible ? 16 : 0) | (interrupt_enabled ? 32 : 0) | 64 | 128);
	save_u32 (currprefs.rtgboards[0].rtgmem_size);
	save_u32 (state->Address);
	save_u32 (state->RGBFormat);
	save_u16 (state->Width);
	save_u16 (state->Height);
	save_u16 (state->VirtualWidth);
	save_u16 (state->VirtualHeight);
	save_u16 (static_cast<uae_u16>(state->XOffset));
	save_u16 (static_cast<uae_u16>(state->YOffset));
	save_u8 (state->GC_Depth);
	save_u8 (state->GC_Flags);
	save_u16 (state->BytesPerRow);
	save_u8 (state->BytesPerPixel);
	save_u32 (uaegfx_base);
	save_u32 (uaegfx_rom);
	save_u32 (boardinfo);
	for (i = 0; i < 4; i++)
		save_u32 (cursorrgb[i]);
	for (i = 0; i < 256; i++) {
		save_u8 (state->CLUT[i].Red);
		save_u8 (state->CLUT[i].Green);
		save_u8 (state->CLUT[i].Blue);
	}
	// overlay
	save_u32(overlay_bitmap);
	save_u32(overlay_vram);
	save_u32(overlay_format);
	save_u32(overlay_modeformat);
	save_u32(overlay_modeinfo);
	save_u32(overlay_color_unswapped);
	save_u8(overlay_active);
	save_u8(overlay_occlusion);
	save_u16(overlay_x);
	save_u16(overlay_y);
	save_u16(overlay_w);
	save_u16(overlay_h);
	save_u16(overlay_src_width_in);
	save_u16(overlay_src_height_in);
	save_u16(overlay_src_width);
	save_u16(overlay_src_height);
	save_u16(overlay_clipleft);
	save_u16(overlay_cliptop);
	save_u16(overlay_clipwidth);
	save_u16(overlay_clipheight);
	save_u16(overlay_brightness);
	for (i = 0; i < 256; i++) {
		save_u8(overlay_clutc[i].Red);
		save_u8(overlay_clutc[i].Green);
		save_u8(overlay_clutc[i].Blue);
	}

	*len = dst - dstbak;
	return dstbak;
}

#endif


