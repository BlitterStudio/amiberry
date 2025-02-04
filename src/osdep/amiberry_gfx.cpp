#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cmath>
#include <iostream>

#include "sysdeps.h"
#include "options.h"
#include "audio.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "traps.h"
#include "xwin.h"
#include "keyboard.h"
#include "drawing.h"
#include "picasso96.h"
#include "amiberry_gfx.h"
#include "sounddep/sound.h"
#include "inputdevice.h"
#ifdef WITH_MIDI
#include "midi.h"
#endif
#include "gui.h"
#include "serial.h"
#include "parallel.h"
#include "sampler.h"
#include "gfxboard.h"
#include "statusline.h"
#include "devices.h"

#include "threaddep/thread.h"
#include "vkbd/vkbd.h"
#include "fsdb_host.h"
#include "savestate.h"

#include <png.h>
#include <SDL_image.h>
#ifdef USE_OPENGL
#include <GL/glew.h>
#include <SDL_opengl.h>

#define CRTEMU_REPORT_SHADER_ERRORS
#define CRTEMU_IMPLEMENTATION
#include "crtemu.h"

#define CRT_FRAME_IMPLEMENTATION
#include "crt_frame.h"

#endif

#ifdef WITH_MIDIEMU
#include "midiemu.h"
#endif

#include "dpi_handler.hpp"

#ifdef AMIBERRY
static bool force_auto_crop = false;

/* SDL Surface for output of emulation */
SDL_DisplayMode sdl_mode;
SDL_Surface* amiga_surface = nullptr;

#ifdef USE_OPENGL
SDL_GLContext gl_context;
//crtemu_t* crtemu_lite = nullptr;
//crtemu_t* crtemu_pc = nullptr;
crtemu_t* crtemu_tv = nullptr;
#else
SDL_Texture* amiga_texture;
#endif

SDL_Rect renderQuad;
static int dx = 0, dy = 0;
SDL_Rect crop_rect;
const char* sdl_video_driver;
bool kmsdrm_detected = false;

static int display_width;
static int display_height;
static int display_depth;
Uint32 pixel_format;

static frame_time_t last_synctime;

static SDL_Surface* current_screenshot = nullptr;
std::string screenshot_filename;
FILE* screenshot_file = nullptr;
int delay_savestate_frame = 0;
#endif

static int deskhz;

struct MultiDisplay Displays[MAX_DISPLAYS];
struct AmigaMonitor AMonitors[MAX_AMIGAMONITORS];

static int display_change_requested;
static int wasfullwindow_a, wasfullwindow_p;

int vsync_activeheight, vsync_totalheight;
float vsync_vblank, vsync_hblank;
bool beamracer_debug;
bool gfx_hdr;

static SDL_mutex* screen_cs = nullptr;
static bool screen_cs_allocated;

/* If we have to lock the SDL surface, then we remember the address
 * of its pixel data - and recalculate the row maps only when this
 * address changes */
static void* old_pixels;

void gfx_lock(void)
{
	SDL_LockMutex(screen_cs);
}
void gfx_unlock(void)
{
	SDL_UnlockMutex(screen_cs);
}

#ifdef AMIBERRY
static void SetRect(SDL_Rect* rect, const int xLeft, const int yTop, const int xRight, const int yBottom)
{
	rect->x = xLeft;
	rect->w = xRight;
	rect->y = yTop;
	rect->h = yBottom;
}

static void OffsetRect(SDL_Rect* rect, const int dx, const int dy)
{
	rect->x += dx;
	rect->y += dy;
}

void GetWindowRect(SDL_Window* window, SDL_Rect* rect)
{
	SDL_GetWindowPosition(window, &rect->x, &rect->y);
	SDL_GetWindowSize(window, &rect->w, &rect->h);
}

// Check if the requested Amiga resolution can be displayed with the current Screen mode as a direct multiple
// Based on this we make the decision to use Linear (smooth) or Nearest Neighbor (pixelated) scaling
bool isModeAspectRatioExact(const SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}

// Set the scaling method based on the user's preferences
// Scaling methods available:
// -1: Auto select between Nearest Neighbor and Linear (default)
// 0: Nearest Neighbor
// 1: Linear
// 2: Integer Scaling (this uses Nearest Neighbor)
void set_scaling_option(const uae_prefs* p, const int width, const int height)
{
	if (p->scaling_method == -1)
	{
		if (isModeAspectRatioExact(&sdl_mode, width, height))
		{
#ifdef USE_OPENGL
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
			SDL_RenderSetIntegerScale(AMonitors[0].amiga_renderer, SDL_FALSE);
#endif
		}
		else
		{
#ifdef USE_OPENGL
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
			SDL_RenderSetIntegerScale(AMonitors[0].amiga_renderer, SDL_FALSE);
#endif
		}
	}
	else if (p->scaling_method == 0)
	{
#ifdef USE_OPENGL
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		SDL_RenderSetIntegerScale(AMonitors[0].amiga_renderer, SDL_FALSE);
#endif
	}
	else if (p->scaling_method == 1)
	{
#ifdef USE_OPENGL
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_RenderSetIntegerScale(AMonitors[0].amiga_renderer, SDL_FALSE);
#endif
	}
	else if (p->scaling_method == 2)
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		SDL_RenderSetIntegerScale(AMonitors[0].amiga_renderer, SDL_TRUE);
	}
#ifdef USE_OPENGL
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

static float SDL2_getrefreshrate(const int monid)
{
	SDL_DisplayMode mode;
	if (SDL_GetDisplayMode(monid, 0, &mode) != 0)
	{
		write_log("SDL_GetDisplayMode failed: %s\n", SDL_GetError());
		return 0;
	}
	return static_cast<float>(mode.refresh_rate);
}

static void SDL2_init()
{
	struct AmigaMonitor* mon = &AMonitors[0];
	write_log("Getting Current Video Driver...\n");
	sdl_video_driver = SDL_GetCurrentVideoDriver();
	if (sdl_video_driver != nullptr && strcmpi(sdl_video_driver, "KMSDRM") == 0)
	{
		kmsdrm_detected = true;
		if (!mon->amiga_window && mon->gui_window)
		{
			mon->amiga_window = mon->gui_window;
		}
		if (!mon->amiga_renderer && mon->gui_renderer)
		{
			mon->amiga_renderer = mon->gui_renderer;
		}
	}

	const auto should_be_zero = SDL_GetCurrentDisplayMode(0, &sdl_mode);
	if (should_be_zero == 0)
	{
		write_log("Current Display mode: bpp %i\t%s\t%i x %i\t%iHz\n", SDL_BITSPERPIXEL(sdl_mode.format), SDL_GetPixelFormatName(sdl_mode.format), sdl_mode.w, sdl_mode.h, sdl_mode.refresh_rate);
		vsync_vblank = static_cast<float>(sdl_mode.refresh_rate);

		mon->currentmode.native_depth = SDL_BITSPERPIXEL(sdl_mode.format);
		mon->currentmode.freq = sdl_mode.refresh_rate;
	}

	// If KMSDRM is detected, force Full-Window mode
	if (kmsdrm_detected)
	{
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = GFX_FULLWINDOW;
		currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = GFX_FULLWINDOW;
	}

	if (!mon->amiga_window)
	{
		write_log("Creating Amiberry window...\n");
		Uint32 mode;

		// Only enable Windowed mode if we're running under x11
		if (!kmsdrm_detected)
		{
			if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen == GFX_FULLWINDOW)
				mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
			else if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen == GFX_FULLSCREEN)
			{
				mode = SDL_WINDOW_FULLSCREEN;
				SDL_DisplayMode dm = {};
				dm.format = SDL_PIXELFORMAT_BGRA32;
				dm.w = currprefs.gfx_monitor[0].gfx_size.width;
				dm.h = currprefs.gfx_monitor[0].gfx_size.height;
				dm.refresh_rate = currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate;
				if (SDL_SetWindowDisplayMode(mon->amiga_window, &dm) != 0) {
					// Handle error
					write_log("Failed to set display mode: %s\n", SDL_GetError());
				}
			}
			else
				mode = SDL_WINDOW_RESIZABLE;
		}
		else
		{
			// otherwise go for Full-window
			mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		
		if (currprefs.borderless)
			mode |= SDL_WINDOW_BORDERLESS;
		if (currprefs.main_alwaysontop)
			mode |= SDL_WINDOW_ALWAYS_ON_TOP;
		if (currprefs.start_minimized)
			mode |= SDL_WINDOW_HIDDEN;
		else
			mode |= SDL_WINDOW_SHOWN;
		// Set Window allow high DPI by default
		mode |= SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef USE_OPENGL
		mode |= SDL_WINDOW_OPENGL;
#endif

		if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		{
			mon->amiga_window = SDL_CreateWindow("Amiberry",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				800,
				600,
				mode);
		}
		else
		{
			mon->amiga_window = SDL_CreateWindow("Amiberry",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				600,
				800,
				mode);
		}
		check_error_sdl(mon->amiga_window == nullptr, "Unable to create window:");

		auto* const icon_surface = IMG_Load(prefix_with_data_path("amiberry.png").c_str());
		if (icon_surface != nullptr)
		{
			SDL_SetWindowIcon(mon->amiga_window, icon_surface);
			SDL_FreeSurface(icon_surface);
		}
	}

#ifdef USE_OPENGL
	if (gl_context == nullptr)
		gl_context = SDL_GL_CreateContext(mon->amiga_window);

	glewInit();

	// Enable vsync
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		write_log("Warning: Failed to enable V-Sync in the current GL context!\n");
	}
#else
	if (mon->amiga_renderer == nullptr)
	{
		Uint32 flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
		mon->amiga_renderer = SDL_CreateRenderer(mon->amiga_window, -1, flags);
		check_error_sdl(mon->amiga_renderer == nullptr, "Unable to create a renderer:");
	}
	DPIHandler::set_render_scale(mon->amiga_renderer);
#endif

	if (SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1") != SDL_TRUE)
		write_log("SDL2: could not grab the keyboard!\n");

	if (SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0") == SDL_TRUE)
		write_log("SDL2: Set window not to minimize on focus loss\n");
}

static bool SDL2_alloctexture(int monid, int w, int h, const int depth)
{
	if (w == 0 || h == 0)
		return false;
#ifdef USE_OPENGL
	struct AmigaMonitor* mon = &AMonitors[monid];

	//crt_frame( (CRTEMU_U32*)amiga_surface->pixels ); // bezel - however, seems hardcoded to internal 1024x1024 size

	//TODO Check for option (which CRT filter to use: Lite/PC/TV)
	if (crtemu_tv)
		destroy_crtemu();
	if (crtemu_tv == nullptr)
		crtemu_tv = crtemu_create(CRTEMU_TYPE_TV, nullptr);
	if (crtemu_tv)
		crtemu_frame(crtemu_tv, (CRTEMU_U32*)amiga_surface->pixels, w, h);
	return crtemu_tv != nullptr;
#else
	if (w < 0 || h < 0)
	{
		if (amiga_texture)
		{
			int width, height;
			Uint32 format;
			SDL_QueryTexture(amiga_texture, &format, nullptr, &width, &height);
			if (width == -w && height == -h && (depth == 16 && format == SDL_PIXELFORMAT_RGB565) || (depth == 32 && format == SDL_PIXELFORMAT_BGRA32))
			{
				set_scaling_option(&currprefs, width, height);
				return true;
			}
		}
		return false;
	}

	if (amiga_texture != nullptr)
		SDL_DestroyTexture(amiga_texture);

	AmigaMonitor* mon = &AMonitors[0];
	amiga_texture = SDL_CreateTexture(mon->amiga_renderer, depth == 16 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, w, h);
	return amiga_texture != nullptr;
#endif
}

static void update_leds(const int monid)
{
	static uae_u32 rc[256], gc[256], bc[256], a[256];
	static int done;
	int osdx, osdy;

	if (!done) {
		for (int i = 0; i < 256; i++) {
			rc[i] = i << 16;
			gc[i] = i << 8;
			bc[i] = i << 0;
			a[i] = i << 24;
		}
		done = 1;
	}

	statusline_getpos(monid, &osdx, &osdy, crop_rect.w + crop_rect.x, crop_rect.h + crop_rect.y);
	int m = statusline_get_multiplier(monid) / 100;
	for (int y = 0; y < TD_TOTAL_HEIGHT * m; y++) {
		uae_u8 *buf = (uae_u8*)amiga_surface->pixels + (y + osdy) * amiga_surface->pitch;
		draw_status_line_single(monid, buf, 32 / 8, y, crop_rect.w + crop_rect.x, rc, gc, bc, a);
	}
}

bool vkbd_allowed(const int monid)
{
	struct AmigaMonitor *mon = &AMonitors[monid];
	return currprefs.vkbd_enabled && !mon->screen_is_picasso;
}

#endif //AMIBERRY

static int isfullscreen_2(struct uae_prefs* p)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	const auto idx = mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE;
	return p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLSCREEN ? 1 : p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLWINDOW ? -1 : 0;
}
int isfullscreen()
{
	return isfullscreen_2(&currprefs);
}

int default_freq = 60;

static struct MultiDisplay* getdisplay2(struct uae_prefs* p, const int index)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	static int max;
	int display = index < 0 ? p->gfx_apmode[mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display - 1 : index;

	if (!max || (max > 0 && Displays[max].monitorname != nullptr)) {
		max = 0;
		while (Displays[max].monitorname)
			max++;
		if (max == 0) {
			gui_message(_T("no display adapters! Exiting"));
			exit(0);
		}
	}
	if (index >= 0 && display >= max)
		return nullptr;
	if (display >= max)
		display = 0;
	display = std::max(display, 0);
	return &Displays[display];
}

struct MultiDisplay* getdisplay(struct uae_prefs* p, const int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	if (monid > 0 && mon->md)
		return mon->md;
	return getdisplay2(p, -1);
}

void desktop_coords(const int monid, int* dw, int* dh, int* ax, int* ay, int* aw, int* ah)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct MultiDisplay* md = getdisplay(&currprefs, monid);

	*dw = md->rect.w - md->rect.x;
	*dh = md->rect.h - md->rect.y;
	*ax = mon->amigawin_rect.x;
	*ay = mon->amigawin_rect.y;
	*aw = mon->amigawin_rect.w;
	*ah = mon->amigawin_rect.h;
}

int target_get_display(const TCHAR* name)
{
	return 0;
}

static volatile int waitvblankthread_mode;
static frame_time_t wait_vblank_timestamp;
static MultiDisplay* wait_vblank_display;
static volatile bool vsync_active;
static bool scanlinecalibrating;

static int target_get_display_scanline2(int displayindex)
{
	//if (pD3DKMTGetScanLine) {
	//	D3DKMT_GETSCANLINE sl = { 0 };
	//	struct MultiDisplay* md = displayindex < 0 ? getdisplay(&currprefs, 0) : &Displays[displayindex];
	//	if (!md->HasAdapterData)
	//		return -11;
	//	sl.VidPnSourceId = md->VidPnSourceId;
	//	sl.hAdapter = md->AdapterHandle;
	//	NTSTATUS status = pD3DKMTGetScanLine(&sl);
	//	if (status == STATUS_SUCCESS) {
	//		if (sl.InVerticalBlank)
	//			return -1;
	//		return sl.ScanLine;
	//	}
	//	else {
	//		if ((int)status > 0)
	//			return -(int)status;
	//		return status;
	//	}
	//	return -12;
	//}
	//else if (D3D_getscanline) {
	//	int scanline;
	//	bool invblank;
	//	if (D3D_getscanline(&scanline, &invblank)) {
	//		if (invblank)
	//			return -1;
	//		return scanline;
	//	}
	//	return -14;
	//}
	return -13;
}

extern uae_s64 spincount;
bool calculated_scanline = true;

int target_get_display_scanline(const int displayindex)
{
	if (!scanlinecalibrating && calculated_scanline) {
		static int lastline;
		float diff = (float)(read_processor_time() - wait_vblank_timestamp);
		if (diff < 0)
			return -1;
		int sl = (int)(diff * (vsync_activeheight + (vsync_totalheight - vsync_activeheight) / 10) * vsync_vblank / syncbase);
		if (sl < 0)
			sl = -1;
		return sl;
	} else {
		static uae_s64 lastrdtsc;
		static int lastvpos;
		if (spincount == 0 || currprefs.m68k_speed >= 0) {
			lastrdtsc = 0;
			lastvpos = target_get_display_scanline2(displayindex);
			return lastvpos;
		}
		uae_s64 v = read_processor_time();
		if (lastrdtsc > v)
			return lastvpos;
		lastvpos = target_get_display_scanline2(displayindex);
		lastrdtsc = read_processor_time() + spincount * 4;
		return lastvpos;
	}
}

static bool get_display_vblank_params(int displayindex, int* activeheightp, int* totalheightp, float* vblankp, float* hblankp)
{
	bool ret = false;
	SDL_DisplayMode dm;
	SDL_Rect usable_bounds;
	SDL_Rect bounds;

	if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
	{
		write_log("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
		return ret;
	}
	if (SDL_GetDisplayUsableBounds(0, &usable_bounds) != 0)
	{
		write_log("SDL_GetDisplayUsableBounds failed: %s\n", SDL_GetError());
		return ret;
	}
	if (SDL_GetDisplayBounds(0, &bounds) != 0)
	{
		write_log("SDL_GetDisplayBounds failed: %s\n", SDL_GetError());
		return ret;
	}

	if (activeheightp)
		*activeheightp = usable_bounds.h;
	if (totalheightp)
		*totalheightp = bounds.h;
	auto vblank = (float)dm.refresh_rate;
	auto hblank = (float)31000; // faking hblank, since SDL2 doesn't provide a way to get the real one
	if (vblankp)
		*vblankp = vblank;
	if (hblankp)
		*hblankp = hblank;

	ret = true;
	return ret;
}

static void display_vblank_thread(struct AmigaMonitor* mon)
{
	struct amigadisplay* ad = &adisplays[mon->monitor_id];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];

	if (waitvblankthread_mode > 0)
		return;
	// It seems some Windows 7 drivers stall if D3DKMTWaitForVerticalBlankEvent()
	// and D3DKMTGetScanLine() is used simultaneously.
	//if ((calculated_scanline || os_win8) && ap->gfx_vsyncmode && pD3DKMTWaitForVerticalBlankEvent && wait_vblank_display && wait_vblank_display->HasAdapterData) {
	//	waitvblankevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//	waitvblankthread_mode = 1;
	//	unsigned int th;
	//	_beginthreadex(NULL, 0, waitvblankthread, 0, 0, &th);
	//}
	//else {
		calculated_scanline = false;
	//}
}

void target_cpu_speed()
{
	display_vblank_thread(&AMonitors[0]);
}

const TCHAR* target_get_display_name(const int num, const bool friendlyname)
{
	if (num <= 0)
		return nullptr;
	struct MultiDisplay* md = getdisplay2(nullptr, num - 1);
	if (!md)
		return nullptr;
	if (friendlyname)
		return md->monitorname;
	return md->monitorid;
}

void getgfxoffset(int monid, float* dxp, float* dyp, float* mxp, float* myp)
{
	float dx = 0, dy = 0, mx = 1.0, my = 1.0;

	if (currprefs.gfx_auto_crop)
	{
		dx -= float(crop_rect.x);
		dy -= float(crop_rect.y);
	}

	*dxp = dx;
	*dyp = dy;
	*mxp = 1.0f / mx;
	*myp = 1.0f / my;
}

static void addmode(struct MultiDisplay* md, SDL_DisplayMode* dm, const int rawmode)
{
	int ct;
	int i, j;
	int w = dm->w;
	int h = dm->h;
	int d = SDL_BITSPERPIXEL(dm->format);
	bool lace = false;
	int freq = 0;

	if (w > max_uae_width || h > max_uae_height) {
		write_log(_T("Ignored mode %d*%d\n"), w, h);
		return;
	}

	if (dm->refresh_rate) {
		freq = dm->refresh_rate;
		if (freq < 10)
			freq = 0;
	}
	//if (dm->dmFields & DM_DISPLAYFLAGS) {
	//	lace = (dm->dmDisplayFlags & DM_INTERLACED) != 0;
	//}

	ct = 0;
	if (d == 8)
		ct = RGBMASK_8BIT;
	if (d == 15)
		ct = RGBMASK_15BIT;
	if (d == 16)
		ct = RGBMASK_16BIT;
	if (d == 24)
		ct = RGBMASK_24BIT;
	if (d == 32)
		ct = RGBMASK_32BIT;
	if (ct == 0)
		return;
	d /= 8;
	i = 0;
	while (md->DisplayModes[i].depth >= 0) {
		if (md->DisplayModes[i].depth == d && md->DisplayModes[i].res.width == w && md->DisplayModes[i].res.height == h) {
			for (j = 0; j < MAX_REFRESH_RATES; j++) {
				if (md->DisplayModes[i].refresh[j] == 0 || md->DisplayModes[i].refresh[j] == freq)
					break;
			}
			if (j < MAX_REFRESH_RATES) {
				md->DisplayModes[i].refresh[j] = freq;
				md->DisplayModes[i].refreshtype[j] = (lace ? REFRESH_RATE_LACE : 0) | (rawmode ? REFRESH_RATE_RAW : 0);
				md->DisplayModes[i].refresh[j + 1] = 0;
				if (!lace)
					md->DisplayModes[i].lace = false;
				return;
			}
		}
		i++;
	}
	i = 0;
	while (md->DisplayModes[i].depth >= 0)
		i++;
	if (i >= MAX_PICASSO_MODES - 1)
		return;
	md->DisplayModes[i].rawmode = rawmode;
	md->DisplayModes[i].lace = lace;
	md->DisplayModes[i].res.width = w;
	md->DisplayModes[i].res.height = h;
	md->DisplayModes[i].depth = d;
	md->DisplayModes[i].refresh[0] = freq;
	md->DisplayModes[i].refreshtype[0] = (lace ? REFRESH_RATE_LACE : 0) | (rawmode ? REFRESH_RATE_RAW : 0);
	md->DisplayModes[i].refresh[1] = 0;
	md->DisplayModes[i].colormodes = ct;
	md->DisplayModes[i + 1].depth = -1;
	_sntprintf(md->DisplayModes[i].name, sizeof md->DisplayModes[i].name, _T("%dx%d%s, %d-bit"),
		md->DisplayModes[i].res.width, md->DisplayModes[i].res.height,
		lace ? _T("i") : _T(""),
		md->DisplayModes[i].depth * 8);
}

static int resolution_compare(const void* a, const void* b)
{
	auto* ma = (struct PicassoResolution*)a;
	auto* mb = (struct PicassoResolution*)b;
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

static void sortmodes(struct MultiDisplay* md)
{
	auto i = 0, idx = -1;
	unsigned int pw = -1, ph = -1;
	while (md->DisplayModes[i].depth >= 0)
		i++;
	qsort(md->DisplayModes, i, sizeof(struct PicassoResolution), resolution_compare);
	for (i = 0; md->DisplayModes[i].depth >= 0; i++)
	{
		int j, k;
		for (j = 0; md->DisplayModes[i].refresh[j]; j++) {
			for (k = j + 1; md->DisplayModes[i].refresh[k]; k++) {
				if (md->DisplayModes[i].refresh[j] > md->DisplayModes[i].refresh[k]) {
					int t = md->DisplayModes[i].refresh[j];
					md->DisplayModes[i].refresh[j] = md->DisplayModes[i].refresh[k];
					md->DisplayModes[i].refresh[k] = t;
					t = md->DisplayModes[i].refreshtype[j];
					md->DisplayModes[i].refreshtype[j] = md->DisplayModes[i].refreshtype[k];
					md->DisplayModes[i].refreshtype[k] = t;
				}
			}
		}
		if (md->DisplayModes[i].res.height != ph || md->DisplayModes[i].res.width != pw)
		{
			ph = md->DisplayModes[i].res.height;
			pw = md->DisplayModes[i].res.width;
			idx++;
		}
		md->DisplayModes[i].residx = idx;
	}
}

static void modesList(struct MultiDisplay* md)
{
	int i, j;

	i = 0;
	while (md->DisplayModes[i].depth >= 0)
	{
		write_log(_T("%d: %s%s ("), i, md->DisplayModes[i].rawmode ? _T("!") : _T(""), md->DisplayModes[i].name);
		j = 0;
		while (md->DisplayModes[i].refresh[j] > 0)
		{
			if (j > 0)
				write_log(_T(","));
			if (md->DisplayModes[i].refreshtype[j] & REFRESH_RATE_RAW)
				write_log(_T("!"));
			write_log(_T("%d"), md->DisplayModes[i].refresh[j]);
			if (md->DisplayModes[i].refreshtype[j] & REFRESH_RATE_LACE)
				write_log(_T("i"));
			j++;
		}
		write_log(_T(")\n"));
		i++;
	}
}

void reenumeratemonitors(void)
{
	for (auto& Display : Displays)
	{
		struct MultiDisplay* md = &Display;
		memcpy(&md->workrect, &md->rect, sizeof(SDL_Rect));
	}
	enumeratedisplays();
}

static bool enumeratedisplays2(bool selectall)
{
	struct MultiDisplay *md = Displays;

	SDL_Rect bounds;
	if (SDL_GetDisplayUsableBounds(0, &bounds) != 0)
	{
		write_log("SDL_GetDisplayUsableBounds failed: %s\n", SDL_GetError());
		return false;
	}

	md->adaptername = my_strdup_trim ("Display adapter");
	md->adapterid = my_strdup ("AdapterID");
	md->adapterkey = my_strdup ("AdapterKey");
	md->monitorname = my_strdup_trim ("Monitor");
	md->monitorid = my_strdup ("MonitorID");
	md->primary = true;

	Displays[0].rect.x = bounds.x;
	Displays[0].rect.y = bounds.y;
	Displays[0].rect.w = bounds.w;
	Displays[0].rect.h = bounds.h;

	if (!md->fullname)
		md->fullname = my_strdup (md->adapterid);

	return true;
}

void enumeratedisplays()
{
	MultiDisplay* md = Displays;
	SDL_GetDisplayBounds(0, &md->rect);
	SDL_GetDisplayBounds(0, &md->workrect);

	if (!enumeratedisplays2 (false))
		enumeratedisplays2(true);
}

void sortdisplays()
{
	struct MultiDisplay* md;
	int i, idx;

	SDL_DisplayMode desktop_dm;
	if (SDL_GetDesktopDisplayMode(0, &desktop_dm) != 0) {
		write_log("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
		return;
	}

	int w = desktop_dm.w;
	int h = desktop_dm.h;
	int wv = w;
	int hv = h;
	int b = SDL_BITSPERPIXEL(desktop_dm.format);

	deskhz = 0;

	md = Displays;
	md->DisplayModes = xmalloc(struct PicassoResolution, MAX_PICASSO_MODES);
	md->DisplayModes[0].depth = -1;

	int numDispModes = SDL_GetNumDisplayModes(0);
	for (int mode = 0; mode < 2; mode++)
	{
		SDL_DisplayMode dm;
		for (idx = 0; idx < numDispModes; idx++)
		{
			if (SDL_GetDisplayMode(0, idx, &dm) != 0) {
				write_log("SDL_GetDisplayMode failed: %s\n", SDL_GetError());
				return;
			}
			int found = 0;
			int idx2 = 0;
			while (md->DisplayModes[idx2].depth >= 0 && !found)
			{
				struct PicassoResolution* pr = &md->DisplayModes[idx2];
				if (dm.w == w && dm.h == h && SDL_BITSPERPIXEL(dm.format) == b) {
					deskhz = std::max(dm.refresh_rate, deskhz);
				}
				if (pr->res.width == dm.w && pr->res.height == dm.h && pr->depth == SDL_BITSPERPIXEL(dm.format) / 8) {
					for (i = 0; pr->refresh[i]; i++) {
						if (pr->refresh[i] == dm.refresh_rate) {
							found = 1;
							break;
						}
					}
				}
				idx2++;
			}
			if (!found && SDL_BITSPERPIXEL(dm.format) > 8) {
				addmode(md, &dm, mode);
			}
		}

	}
	sortmodes(md);
	modesList(md);
	i = 0;
	while (md->DisplayModes[i].depth > 0)
		i++;
	write_log(_T("%d display modes.\n"), i);
	write_log(_T("Desktop: W=%d H=%d B=%d HZ=%d. CXVS=%d CYVS=%d\n"), w, h, b, deskhz, wv, hv);
}

int gfx_adjust_screenmode(MultiDisplay *md, int *pwidth, int *pheight, int *ppixbits)
{
	struct PicassoResolution* best;
	uae_u32 selected_mask = (*ppixbits == 8 ? RGBMASK_8BIT
		: *ppixbits == 15 ? RGBMASK_15BIT
		: *ppixbits == 16 ? RGBMASK_16BIT
		: *ppixbits == 24 ? RGBMASK_24BIT
		: RGBMASK_32BIT);
	int pass, i = 0, index = 0;

	for (pass = 0; pass < 2; pass++) {
		struct PicassoResolution* dm;
		uae_u32 mask = (pass == 0
			? selected_mask
			: RGBMASK_8BIT | RGBMASK_15BIT | RGBMASK_16BIT | RGBMASK_24BIT | RGBMASK_32BIT); /* %%% - BERND, were you missing 15-bit here??? */
		i = 0;
		index = 0;

		best = &md->DisplayModes[0];
		dm = &md->DisplayModes[1];

		while (dm->depth >= 0) {

			/* do we already have supported resolution? */
			if (dm->res.width == *pwidth && dm->res.height == *pheight && dm->depth == (*ppixbits / 8))
				return i;

			if ((dm->colormodes & mask) != 0) {
				if (dm->res.width <= best->res.width && dm->res.height <= best->res.height
					&& dm->res.width >= *pwidth && dm->res.height >= *pheight)
				{
					best = dm;
					index = i;
				}
				if (dm->res.width >= best->res.width && dm->res.height >= best->res.height
					&& dm->res.width <= *pwidth && dm->res.height <= *pheight)
				{
					best = dm;
					index = i;
				}
			}
			dm++;
			i++;
		}
		if (best->res.width == *pwidth && best->res.height == *pheight) {
			selected_mask = mask; /* %%% - BERND, I added this - does it make sense?  Otherwise, I'd specify a 16-bit display-mode for my
								  Workbench (using -H 2, but SHOULD have been -H 1), and end up with an 8-bit mode instead*/
			break;
		}
	}
	*pwidth = best->res.width;
	*pheight = best->res.height;
	if (best->colormodes & selected_mask)
		return index;

	/* Ordering here is done such that 16-bit is preferred, followed by 15-bit, 8-bit, 32-bit and 24-bit */
	if (best->colormodes & RGBMASK_16BIT)
		*ppixbits = 16;
	else if (best->colormodes & RGBMASK_15BIT) /* %%% - BERND, this possibility was missing? */
		*ppixbits = 15;
	else if (best->colormodes & RGBMASK_8BIT)
		*ppixbits = 8;
	else if (best->colormodes & RGBMASK_32BIT)
		*ppixbits = 32;
	else if (best->colormodes & RGBMASK_24BIT)
		*ppixbits = 24;
	else
		index = -1;

	return index;
}

bool render_screen(int monid, int mode, bool immediate)
{
	return true;
}

bool show_screen_maybe(const int monid, const bool show)
{
	struct amigadisplay* ad = &adisplays[monid];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	if (!ap->gfx_vflip || ap->gfx_vsyncmode == 0 || ap->gfx_vsync <= 0) {
		if (show)
			show_screen(monid, 0);
		return false;
	}
	return false;
}

float target_adjust_vblank_hz(const int monid, float hz)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	int maxrate;
	if (!currprefs.lightboost_strobo)
		return hz;
	if (isfullscreen() > 0) {
		maxrate = mon->currentmode.freq;
	} else {
		maxrate = deskhz;
	}
	double nhz = hz * 2.0;
	if (nhz >= maxrate - 1 && nhz < maxrate + 1)
		hz -= 0.5;
	return hz;
}

void show_screen(const int monid, int mode)
{
	AmigaMonitor* mon = &AMonitors[monid];
	const amigadisplay* ad = &adisplays[monid];
	const bool rtg = ad->picasso_on;

	const auto start = read_processor_time();

	// RTG status line is handled in P96 code, this is for native modes only
	if ((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !rtg)
	{
		update_leds(monid);
	}

#ifdef USE_OPENGL
	auto time = SDL_GetTicks();
	glViewport(0, 0, renderQuad.w, renderQuad.h);
	if (crtemu_tv) {
		crtemu_present(crtemu_tv, time, (CRTEMU_U32 const*)amiga_surface->pixels,
			crop_rect.w, crop_rect.h, 0xffffffff, 0x000000);
	}

	SDL_GL_SwapWindow(mon->amiga_window);
#else
	if (amiga_texture && amiga_surface)
	{
		SDL_RenderClear(mon->amiga_renderer);
		SDL_UpdateTexture(amiga_texture, nullptr, amiga_surface->pixels, amiga_surface->pitch);
		SDL_RenderCopyEx(mon->amiga_renderer, amiga_texture, &crop_rect, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
		if (vkbd_allowed(monid))
		{
			vkbd_redraw();
		}
		SDL_RenderPresent(mon->amiga_renderer);
	}
#endif // USE_OPENGL

	last_synctime = read_processor_time();
	idletime += last_synctime - start;
}

int lockscr(struct vidbuffer* vb, bool fullupdate, bool first, bool skip)
{
	gfx_lock();
	//if (amiga_surface && SDL_MUSTLOCK(amiga_surface))
	//	SDL_LockSurface(amiga_surface);
	//int pitch;
	//SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&vb->bufmem), &pitch);
	if (amiga_surface->pixels != old_pixels) {
	//if (first)
		init_row_map();
		old_pixels = amiga_surface->pixels;
	}
	gfx_unlock();
	return 1;
}

void unlockscr(struct vidbuffer* vb, int y_start, int y_end)
{
	//if (amiga_surface && SDL_MUSTLOCK(amiga_surface))
	//	SDL_UnlockSurface(amiga_surface);
	//SDL_UnlockTexture(texture);
	gfx_unlock();
}

uae_u8* gfx_lock_picasso(const int monid, bool fullupdate)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[monid];
	static uae_u8* p;
	if (amiga_surface == nullptr || mon->screen_is_picasso == 0)
		return nullptr;
	if (mon->rtg_locked) {
		return p;
	}
	//gfx_lock();
	vidinfo->pixbytes = amiga_surface->format->BytesPerPixel;
	vidinfo->rowbytes = amiga_surface->pitch;
	vidinfo->maxwidth = amiga_surface->w;
	vidinfo->maxheight = amiga_surface->h;
	p = static_cast<uae_u8*>(amiga_surface->pixels);
	if (!p)
	{
		//gfx_unlock();
	}
	else
	{
		mon->rtg_locked = true;
	}
	return p;
}

void gfx_unlock_picasso(const int monid, const bool dorender)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	//if (!mon->rtg_locked)
		//gfx_lock();
	mon->rtg_locked = false;
	if (dorender)
	{
		render_screen(0, 0, true);
		show_screen(0, 0);
	}
	//gfx_unlock();
}

static bool canmatchdepth(void)
{
	if (!currprefs.rtgmatchdepth)
		return false;
	if (currprefs.gfx_api >= 2)
		return false;
	return true;
}

static void updatemodes(struct AmigaMonitor* mon)
{
	struct uae_filter* usedfilter = mon->usedfilter;
	Uint32 flags = 0;

	mon->currentmode.fullfill = 0;
	if (isfullscreen() > 0)
		flags |= SDL_WINDOW_FULLSCREEN;
	else if (isfullscreen() < 0)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#ifdef AMIBERRY
	if (mon->currentmode.current_depth < 15)
		mon->currentmode.current_depth = 16;
	else if (mon->currentmode.current_depth < 32)
		mon->currentmode.current_depth = 32;
#else
#if defined (GFXFILTER)
	if (usedfilter) {
		flags |= DM_SWSCALE;
		if (mon->currentmode.current_depth < 15)
			mon->currentmode.current_depth = 16;
	}
#endif
#endif
	mon->currentmode.flags = flags;
	mon->currentmode.fullfill = 1;
	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		const SDL_Rect rc = getdisplay(&currprefs, mon->monitor_id)->rect;
		mon->currentmode.native_width = rc.w;
		mon->currentmode.native_height = rc.h;
		mon->currentmode.current_width = mon->currentmode.native_width;
		mon->currentmode.current_height = mon->currentmode.native_height;
	} else {
		mon->currentmode.native_width = mon->currentmode.current_width;
		mon->currentmode.native_height = mon->currentmode.current_height;
	}
}

static void update_gfxparams(struct AmigaMonitor* mon)
{
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];

	updatewinfsmode(mon->monitor_id, &currprefs);
#ifdef PICASSO96
	mon->currentmode.vsync = 0;
	if (mon->screen_is_picasso) {
		float mx = 1.0;
		float my = 1.0;
		if (currprefs.gf[GF_RTG].gfx_filter_horiz_zoom_mult > 0) {
			mx *= currprefs.gf[GF_RTG].gfx_filter_horiz_zoom_mult;
		}
		if (currprefs.gf[GF_RTG].gfx_filter_vert_zoom_mult > 0) {
			my *= currprefs.gf[GF_RTG].gfx_filter_vert_zoom_mult;
		}
		mon->currentmode.current_width = static_cast<int>(state->Width * currprefs.rtg_horiz_zoom_mult * mx);
		mon->currentmode.current_height = static_cast<int>(state->Height * currprefs.rtg_vert_zoom_mult * my);
		currprefs.gfx_apmode[APMODE_RTG].gfx_interlaced = false;
		if (currprefs.rtgvblankrate == 0) {
			currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate = currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate;
			if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced) {
				currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate *= 2;
			}
		} else if (currprefs.rtgvblankrate < 0) {
			currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate = 0;
		} else {
			currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate = currprefs.rtgvblankrate;
		}
		if (currprefs.gfx_apmode[APMODE_RTG].gfx_vsync)
			mon->currentmode.vsync = 1 + currprefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode;
	} else {
#endif
		mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
		mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
		if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync)
			mon->currentmode.vsync = 1 + currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode;
#ifdef PICASSO96
	}
#endif
#if FORCE16BIT
	mon->currentmode.current_depth = 16;
#else
	mon->currentmode.current_depth = currprefs.color_mode < 5 && currprefs.gfx_api == 1 ? 16 : 32;
#endif
	if (mon->screen_is_picasso && canmatchdepth() && isfullscreen() > 0) {
		int pbits = state->BytesPerPixel * 8;
		if (pbits <= 8) {
			if (mon->currentmode.current_depth == 32)
				pbits = 32;
			else
				pbits = 16;
		}
		if (pbits == 24)
			pbits = 32;
		mon->currentmode.current_depth = pbits;
	}
	mon->currentmode.amiga_width = mon->currentmode.current_width;
	mon->currentmode.amiga_height = mon->currentmode.current_height;

	mon->scalepicasso = 0;
	if (mon->screen_is_picasso) {
		bool diff = state->Width != mon->currentmode.native_width || state->Height != mon->currentmode.native_height;
		if (isfullscreen() < 0) {
			if ((currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER || currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_SCALE || currprefs.rtgallowscaling) && diff) {
				mon->scalepicasso = RTG_MODE_SCALE;
			}
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE && diff) {
				mon->scalepicasso = RTG_MODE_INTEGER_SCALE;
			}
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER && diff) {
				mon->scalepicasso = currprefs.gf[GF_RTG].gfx_filter_autoscale;
			}
			if (!mon->scalepicasso && currprefs.rtgscaleaspectratio) {
				mon->scalepicasso = -1;
			}
		} else if (isfullscreen() > 0) {
			if (!canmatchdepth()) { // can't scale to different color depth
				if (mon->currentmode.native_width > state->Width && mon->currentmode.native_height > state->Height) {
					if (currprefs.gf[GF_RTG].gfx_filter_autoscale)
						mon->scalepicasso = RTG_MODE_SCALE;
					if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE) {
						mon->scalepicasso = RTG_MODE_INTEGER_SCALE;
					}
				}
				if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER)
					mon->scalepicasso = currprefs.gf[GF_RTG].gfx_filter_autoscale;
				if (!mon->scalepicasso && currprefs.rtgscaleaspectratio)
					mon->scalepicasso = -1;
			}
		} else if (isfullscreen() == 0) {
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE) {
				mon->scalepicasso = RTG_MODE_INTEGER_SCALE;
				mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
				mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
			} else if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER) {
				if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width < state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height < state->Height) {
					if (!currprefs.rtgallowscaling) {
						;
					} else if (currprefs.rtgscaleaspectratio) {
						mon->scalepicasso = -1;
						mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
						mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
					}
				} else {
					mon->scalepicasso = RTG_MODE_CENTER;
					mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
					mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
				}
			} else if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_SCALE) {
				if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width > state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height > state->Height)
					mon->scalepicasso = RTG_MODE_SCALE;
				if ((currprefs.gfx_monitor[mon->monitor_id].gfx_size.width != state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height != state->Height) && currprefs.rtgallowscaling) {
					mon->scalepicasso = RTG_MODE_SCALE;
				} else if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width < state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height < state->Height) {
					// no always scaling and smaller? Back to normal size and set new configured max size
					mon->currentmode.current_width = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.width = state->Width;
					mon->currentmode.current_height = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.height = state->Height;
				} else if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width == state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height == state->Height) {
					;
				} else if (!mon->scalepicasso && currprefs.rtgscaleaspectratio) {
					mon->scalepicasso = -1;
				}
			} else {
				if ((currprefs.gfx_monitor[mon->monitor_id].gfx_size.width != state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height != state->Height) && currprefs.rtgallowscaling)
					mon->scalepicasso = RTG_MODE_SCALE;
				if (!mon->scalepicasso && currprefs.rtgscaleaspectratio)
					mon->scalepicasso = -1;
			}
		}

		if (mon->scalepicasso > 0 && (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width != state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height != state->Height)) {
			mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
			mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
		}
	}
}

void graphics_reset(const bool forced)
{
	if (forced) {
		display_change_requested = 2;
	} else {
		// full reset if display size can't be changed.
		if (currprefs.gfx_api) {
			display_change_requested = 3;
		} else {
			display_change_requested = 2;
		}
	}
}

static void open_screen(struct uae_prefs* p);

int check_prefs_changed_gfx()
{
	int c = 0;
	bool monitors[MAX_AMIGAMONITORS]{};

	if (!config_changed && !display_change_requested)
		return 0;

	c |= config_changed_flags;
	config_changed_flags = 0;

	for (int i = 0; i < MAX_AMIGADISPLAYS; i++) {
		monitors[i] = false;
		int c2 = 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_fs.width != changed_prefs.gfx_monitor[i].gfx_size_fs.width ? 16 : 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_fs.height != changed_prefs.gfx_monitor[i].gfx_size_fs.height ? 16 : 0;
		c2 |= ((currprefs.gfx_monitor[i].gfx_size_win.width + 7) & ~7) != ((changed_prefs.gfx_monitor[i].gfx_size_win.width + 7) & ~7) ? 16 : 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_win.height != changed_prefs.gfx_monitor[i].gfx_size_win.height ? 16 : 0;
#ifdef AMIBERRY
		c2 |= currprefs.gfx_horizontal_offset != changed_prefs.gfx_horizontal_offset ? 16 : 0;
		c2 |= currprefs.gfx_vertical_offset != changed_prefs.gfx_vertical_offset ? 16 : 0;
		c2 |= currprefs.gfx_auto_crop != changed_prefs.gfx_auto_crop ? 16 : 0;
		c2 |= currprefs.gfx_manual_crop != changed_prefs.gfx_manual_crop ? 16 : 0;
		c2 |= currprefs.gfx_manual_crop_width != changed_prefs.gfx_manual_crop_width ? 16 : 0;
		c2 |= currprefs.gfx_manual_crop_height != changed_prefs.gfx_manual_crop_height ? 16 : 0;
		c2 |= currprefs.gfx_correct_aspect != changed_prefs.gfx_correct_aspect ? 16 : 0;
		c2 |= currprefs.scaling_method != changed_prefs.scaling_method ? 16 : 0;
#endif
		if (c2) {
			if (i > 0) {
				for (auto& rtgboard : changed_prefs.rtgboards) {
					struct rtgboardconfig* rbc = &rtgboard;
					if (rbc->monitor_id == i) {
						c |= c2;
						monitors[i] = true;
					}
				}
				if (!monitors[i]) {
					currprefs.gfx_monitor[i].gfx_size_fs.width = changed_prefs.gfx_monitor[i].gfx_size_fs.width;
					currprefs.gfx_monitor[i].gfx_size_fs.height = changed_prefs.gfx_monitor[i].gfx_size_fs.height;
					currprefs.gfx_monitor[i].gfx_size_win.width = changed_prefs.gfx_monitor[i].gfx_size_win.width;
					currprefs.gfx_monitor[i].gfx_size_win.height = changed_prefs.gfx_monitor[i].gfx_size_win.height;
				}
			} else {
				c |= c2;
				monitors[i] = true;
			}
		}
		if (AMonitors[i].screen_is_picasso) {
			struct gfx_filterdata* gfc = &changed_prefs.gf[1];
			if (gfc->changed) {
				gfc->changed = false;
				c |= 16;
			}
		} else {
			struct gfx_filterdata* gfc1 = &changed_prefs.gf[0];
			struct gfx_filterdata* gfc2 = &changed_prefs.gf[2];
			if (gfc1->changed || gfc2->changed) {
				gfc1->changed = false;
				gfc2->changed = false;
				c |= 16;
			}
		}
	}
	if (currprefs.gf[2].enable != changed_prefs.gf[2].enable) {
		currprefs.gf[2].enable = changed_prefs.gf[2].enable;
		c |= 512;
	}

	monitors[0] = true;

	c |= currprefs.color_mode != changed_prefs.color_mode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen ? 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen != changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen ? 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_vsync != changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsync ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode != changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate ? 2 | 16 : 0;
	c |= currprefs.gfx_autoresolution != changed_prefs.gfx_autoresolution ? (2 | 8 | 16) : 0;
	c |= currprefs.gfx_autoresolution_vga != changed_prefs.gfx_autoresolution_vga ? (2 | 8 | 16) : 0;
	c |= currprefs.gfx_api != changed_prefs.gfx_api ? (1 | 8 | 32) : 0;
	c |= currprefs.gfx_api_options != changed_prefs.gfx_api_options ? (1 | 8 | 32) : 0;
	c |= currprefs.lightboost_strobo != changed_prefs.lightboost_strobo ? (2 | 16) : 0;

	for (int j = 0; j < MAX_FILTERDATA; j++) {
		struct gfx_filterdata* gf = &currprefs.gf[j];
		struct gfx_filterdata* gfc = &changed_prefs.gf[j];

		c |= gf->gfx_filter != gfc->gfx_filter ? (2 | 8) : 0;
		c |= gf->gfx_filter != gfc->gfx_filter ? (2 | 8) : 0;

		for (int i = 0; i <= 2 * MAX_FILTERSHADERS; i++) {
			c |= _tcscmp(gf->gfx_filtershader[i], gfc->gfx_filtershader[i]) ? (2 | 8) : 0;
			c |= _tcscmp(gf->gfx_filtermask[i], gfc->gfx_filtermask[i]) ? (2 | 8) : 0;
		}
		c |= _tcscmp(gf->gfx_filteroverlay, gfc->gfx_filteroverlay) ? (2 | 8) : 0;

		c |= gf->gfx_filter_scanlines != gfc->gfx_filter_scanlines ? (1 | 8) : 0;
		c |= gf->gfx_filter_scanlinelevel != gfc->gfx_filter_scanlinelevel ? (1 | 8) : 0;
		c |= gf->gfx_filter_scanlineratio != gfc->gfx_filter_scanlineratio ? (1 | 8) : 0;
		c |= gf->gfx_filter_scanlineoffset != gfc->gfx_filter_scanlineoffset ? (1 | 8) : 0;

		c |= gf->gfx_filter_horiz_zoom_mult != gfc->gfx_filter_horiz_zoom_mult ? (1) : 0;
		c |= gf->gfx_filter_vert_zoom_mult != gfc->gfx_filter_vert_zoom_mult ? (1) : 0;

		c |= gf->gfx_filter_filtermodeh != gfc->gfx_filter_filtermodeh ? (2 | 8) : 0;
		c |= gf->gfx_filter_filtermodev != gfc->gfx_filter_filtermodev ? (2 | 8) : 0;
		c |= gf->gfx_filter_bilinear != gfc->gfx_filter_bilinear ? (2 | 8 | 16) : 0;
		c |= gf->gfx_filter_noise != gfc->gfx_filter_noise ? (1) : 0;
		c |= gf->gfx_filter_blur != gfc->gfx_filter_blur ? (1) : 0;

		c |= gf->gfx_filter_aspect != gfc->gfx_filter_aspect ? (1) : 0;
		c |= gf->gfx_filter_rotation != gfc->gfx_filter_rotation ? (1) : 0;
		c |= gf->gfx_filter_keep_aspect != gfc->gfx_filter_keep_aspect ? (1) : 0;
		c |= gf->gfx_filter_keep_autoscale_aspect != gfc->gfx_filter_keep_autoscale_aspect ? (1) : 0;
		c |= gf->gfx_filter_luminance != gfc->gfx_filter_luminance ? (1) : 0;
		c |= gf->gfx_filter_contrast != gfc->gfx_filter_contrast ? (1) : 0;
		c |= gf->gfx_filter_saturation != gfc->gfx_filter_saturation ? (1) : 0;
		c |= gf->gfx_filter_gamma != gfc->gfx_filter_gamma ? (1) : 0;
		c |= gf->gfx_filter_integerscalelimit != gfc->gfx_filter_integerscalelimit ? (1) : 0;
		if (j && gf->gfx_filter_autoscale != gfc->gfx_filter_autoscale)
			c |= 8 | 64;
	}

	c |= currprefs.rtg_horiz_zoom_mult != changed_prefs.rtg_horiz_zoom_mult ? 16 : 0;
	c |= currprefs.rtg_vert_zoom_mult != changed_prefs.rtg_vert_zoom_mult ? 16 : 0;

	c |= currprefs.gfx_luminance != changed_prefs.gfx_luminance ? (1 | 256) : 0;
	c |= currprefs.gfx_contrast != changed_prefs.gfx_contrast ? (1 | 256) : 0;
	c |= currprefs.gfx_gamma != changed_prefs.gfx_gamma ? (1 | 256) : 0;

	c |= currprefs.gfx_resolution != changed_prefs.gfx_resolution ? (128) : 0;
	c |= currprefs.gfx_vresolution != changed_prefs.gfx_vresolution ? (128) : 0;
	c |= currprefs.gfx_autoresolution_minh != changed_prefs.gfx_autoresolution_minh ? (128) : 0;
	c |= currprefs.gfx_autoresolution_minv != changed_prefs.gfx_autoresolution_minv ? (128) : 0;
	c |= currprefs.gfx_iscanlines != changed_prefs.gfx_iscanlines ? (2 | 8) : 0;
	c |= currprefs.gfx_pscanlines != changed_prefs.gfx_pscanlines ? (2 | 8) : 0;

	c |= currprefs.monitoremu != changed_prefs.monitoremu ? (2 | 8) : 0;
	c |= currprefs.genlock_image != changed_prefs.genlock_image ? (2 | 8) : 0;
	c |= currprefs.genlock != changed_prefs.genlock ? (2 | 8) : 0;
	c |= currprefs.genlock_alpha != changed_prefs.genlock_alpha ? (1 | 8) : 0;
	c |= currprefs.genlock_mix != changed_prefs.genlock_mix ? (1 | 256) : 0;
	c |= currprefs.genlock_aspect != changed_prefs.genlock_aspect ? (1 | 256) : 0;
	c |= currprefs.genlock_scale != changed_prefs.genlock_scale ? (1 | 256) : 0;
	c |= _tcsicmp(currprefs.genlock_image_file, changed_prefs.genlock_image_file) ? (2 | 8) : 0;
	c |= _tcsicmp(currprefs.genlock_video_file, changed_prefs.genlock_video_file) ? (2 | 8) : 0;

	c |= currprefs.gfx_lores_mode != changed_prefs.gfx_lores_mode ? (2 | 8) : 0;
	c |= currprefs.gfx_overscanmode != changed_prefs.gfx_overscanmode ? (2 | 8) : 0;
	c |= currprefs.gfx_scandoubler != changed_prefs.gfx_scandoubler ? (2 | 8) : 0;
	c |= currprefs.gfx_threebitcolors != changed_prefs.gfx_threebitcolors ? (256) : 0;
	c |= currprefs.gfx_grayscale != changed_prefs.gfx_grayscale ? (512) : 0;
	c |= currprefs.gfx_monitorblankdelay != changed_prefs.gfx_monitorblankdelay ? (512) : 0;

	c |= currprefs.gfx_display_sections != changed_prefs.gfx_display_sections ? (512) : 0;
	c |= currprefs.gfx_variable_sync != changed_prefs.gfx_variable_sync ? 1 : 0;
	c |= currprefs.gfx_windowed_resize != changed_prefs.gfx_windowed_resize ? 1 : 0;

	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_display != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_display ? (2 | 4 | 8) : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_display != changed_prefs.gfx_apmode[APMODE_RTG].gfx_display ? (2 | 4 | 8) : 0;
	c |= currprefs.gfx_blackerthanblack != changed_prefs.gfx_blackerthanblack ? (2 | 8) : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers ? (2 | 16) : 0;
	c |= currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced ? (2 | 8) : 0;
	c |= currprefs.gfx_apmode[APMODE_RTG].gfx_backbuffers != changed_prefs.gfx_apmode[APMODE_RTG].gfx_backbuffers ? (2 | 16) : 0;

	c |= currprefs.main_alwaysontop != changed_prefs.main_alwaysontop ? 32 : 0;
	c |= currprefs.gui_alwaysontop != changed_prefs.gui_alwaysontop ? 2 : 0;
	c |= currprefs.borderless != changed_prefs.borderless ? 32 : 0;
	c |= currprefs.blankmonitors != changed_prefs.blankmonitors ? 32 : 0;
	c |= currprefs.rtgmatchdepth != changed_prefs.rtgmatchdepth ? 2 : 0;
	c |= currprefs.rtgallowscaling != changed_prefs.rtgallowscaling ? (2 | 8 | 64) : 0;
	c |= currprefs.rtgscaleaspectratio != changed_prefs.rtgscaleaspectratio ? (8 | 64) : 0;
	c |= currprefs.rtgvblankrate != changed_prefs.rtgvblankrate ? 8 : 0;

#ifdef AMIBERRY
	// These are missing from WinUAE
	c |= currprefs.rtg_hardwareinterrupt != changed_prefs.rtg_hardwareinterrupt ? 32 : 0;
	c |= currprefs.rtg_hardwaresprite != changed_prefs.rtg_hardwaresprite ? 32 : 0;
	c |= currprefs.rtg_multithread != changed_prefs.rtg_multithread ? 32 : 0;
#endif

#ifdef AMIBERRY
	c |= currprefs.multithreaded_drawing != changed_prefs.multithreaded_drawing ? (512) : 0;
#endif

	if (display_change_requested || c)
	{
		bool setpause = false;
		bool dontcapture = false;
		int keepfsmode =
			currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen == changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen &&
			currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen == changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen;

		currprefs.gfx_autoresolution = changed_prefs.gfx_autoresolution;
		currprefs.gfx_autoresolution_vga = changed_prefs.gfx_autoresolution_vga;
		currprefs.color_mode = changed_prefs.color_mode;
		currprefs.lightboost_strobo = changed_prefs.lightboost_strobo;

		if (currprefs.gfx_api != changed_prefs.gfx_api) {
			display_change_requested = 1;
		}

		if (display_change_requested) {
			if (display_change_requested == 3) {
				c = 1024;
			} else if (display_change_requested == 2) {
				c = 512;
			} else if (display_change_requested == 4) {
				c = 32;
			} else {
				c = 2;
				keepfsmode = 0;
				if (display_change_requested <= -1) {
					dontcapture = true;
					if (display_change_requested == -2)
						setpause = true;
					if (pause_emulation)
						setpause = true;
				}
			}
			display_change_requested = 0;
		}

		for (int j = 0; j < MAX_FILTERDATA; j++) {
			struct gfx_filterdata* gf = &currprefs.gf[j];
			struct gfx_filterdata* gfc = &changed_prefs.gf[j];
			memcpy(gf, gfc, sizeof(struct gfx_filterdata));
		}

#ifdef AMIBERRY
		currprefs.gfx_monitor[0].gfx_size_fs.width = changed_prefs.gfx_monitor[0].gfx_size_fs.width;
		currprefs.gfx_monitor[0].gfx_size_fs.height = changed_prefs.gfx_monitor[0].gfx_size_fs.height;
		currprefs.gfx_monitor[0].gfx_size_win.width = changed_prefs.gfx_monitor[0].gfx_size_win.width;
		currprefs.gfx_monitor[0].gfx_size_win.height = changed_prefs.gfx_monitor[0].gfx_size_win.height;
		currprefs.gfx_monitor[0].gfx_size.width = changed_prefs.gfx_monitor[0].gfx_size.width;
		currprefs.gfx_monitor[0].gfx_size.height = changed_prefs.gfx_monitor[0].gfx_size.height;
		currprefs.gfx_monitor[0].gfx_size_win.x = changed_prefs.gfx_monitor[0].gfx_size_win.x;
		currprefs.gfx_monitor[0].gfx_size_win.y = changed_prefs.gfx_monitor[0].gfx_size_win.y;

		currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen;
		currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen;
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync;
		currprefs.gfx_apmode[APMODE_RTG].gfx_vsync = changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsync;
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsyncmode;
		currprefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode = changed_prefs.gfx_apmode[APMODE_RTG].gfx_vsyncmode;
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate;

		currprefs.multithreaded_drawing = changed_prefs.multithreaded_drawing;
		currprefs.gfx_horizontal_offset = changed_prefs.gfx_horizontal_offset;
		currprefs.gfx_vertical_offset = changed_prefs.gfx_vertical_offset;
		currprefs.gfx_manual_crop_width = changed_prefs.gfx_manual_crop_width;
		currprefs.gfx_manual_crop_height = changed_prefs.gfx_manual_crop_height;
		currprefs.gfx_auto_crop = changed_prefs.gfx_auto_crop;
		currprefs.gfx_manual_crop = changed_prefs.gfx_manual_crop;

		if (currprefs.gfx_auto_crop)
		{
			changed_prefs.gfx_xcenter = changed_prefs.gfx_ycenter = 0;
		}
		currprefs.gfx_correct_aspect = changed_prefs.gfx_correct_aspect;
		currprefs.scaling_method = changed_prefs.scaling_method;
#endif
		currprefs.rtg_horiz_zoom_mult = changed_prefs.rtg_horiz_zoom_mult;
		currprefs.rtg_vert_zoom_mult = changed_prefs.rtg_vert_zoom_mult;

		currprefs.gfx_luminance = changed_prefs.gfx_luminance;
		currprefs.gfx_contrast = changed_prefs.gfx_contrast;
		currprefs.gfx_gamma = changed_prefs.gfx_gamma;

		currprefs.gfx_resolution = changed_prefs.gfx_resolution;
		currprefs.gfx_vresolution = changed_prefs.gfx_vresolution;
		currprefs.gfx_autoresolution_minh = changed_prefs.gfx_autoresolution_minh;
		currprefs.gfx_autoresolution_minv = changed_prefs.gfx_autoresolution_minv;
		currprefs.gfx_iscanlines = changed_prefs.gfx_iscanlines;
		currprefs.gfx_pscanlines = changed_prefs.gfx_pscanlines;
		currprefs.monitoremu = changed_prefs.monitoremu;

		currprefs.genlock_image = changed_prefs.genlock_image;
		currprefs.genlock = changed_prefs.genlock;
		currprefs.genlock_mix = changed_prefs.genlock_mix;
		currprefs.genlock_alpha = changed_prefs.genlock_alpha;
		currprefs.genlock_aspect = changed_prefs.genlock_aspect;
		currprefs.genlock_scale = changed_prefs.genlock_scale;
		_tcscpy(currprefs.genlock_image_file, changed_prefs.genlock_image_file);
		_tcscpy(currprefs.genlock_video_file, changed_prefs.genlock_video_file);

		currprefs.gfx_lores_mode = changed_prefs.gfx_lores_mode;
		currprefs.gfx_overscanmode = changed_prefs.gfx_overscanmode;
		currprefs.gfx_scandoubler = changed_prefs.gfx_scandoubler;
		currprefs.gfx_threebitcolors = changed_prefs.gfx_threebitcolors;
		currprefs.gfx_grayscale = changed_prefs.gfx_grayscale;
		currprefs.gfx_monitorblankdelay = changed_prefs.gfx_monitorblankdelay;

		currprefs.gfx_display_sections = changed_prefs.gfx_display_sections;
		currprefs.gfx_variable_sync = changed_prefs.gfx_variable_sync;
		currprefs.gfx_windowed_resize = changed_prefs.gfx_windowed_resize;

		currprefs.gfx_apmode[APMODE_NATIVE].gfx_display = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_display;
		currprefs.gfx_apmode[APMODE_RTG].gfx_display = changed_prefs.gfx_apmode[APMODE_RTG].gfx_display;
		currprefs.gfx_blackerthanblack = changed_prefs.gfx_blackerthanblack;
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_backbuffers;
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced;
		currprefs.gfx_apmode[APMODE_RTG].gfx_backbuffers = changed_prefs.gfx_apmode[APMODE_RTG].gfx_backbuffers;

		currprefs.main_alwaysontop = changed_prefs.main_alwaysontop;
		currprefs.gui_alwaysontop = changed_prefs.gui_alwaysontop;
		currprefs.borderless = changed_prefs.borderless;
		currprefs.blankmonitors = changed_prefs.blankmonitors;
		currprefs.rtgmatchdepth = changed_prefs.rtgmatchdepth;
		currprefs.rtgallowscaling = changed_prefs.rtgallowscaling;
		currprefs.rtgscaleaspectratio = changed_prefs.rtgscaleaspectratio;
		currprefs.rtgvblankrate = changed_prefs.rtgvblankrate;

#ifdef AMIBERRY
		// These are missing from WinUAE
		currprefs.rtg_hardwareinterrupt = changed_prefs.rtg_hardwareinterrupt;
		currprefs.rtg_hardwaresprite = changed_prefs.rtg_hardwaresprite;
		currprefs.rtg_multithread = changed_prefs.rtg_multithread;
#endif

		bool unacquired = false;
		for (int monid = MAX_AMIGAMONITORS - 1; monid >= 0; monid--) {
			if (!monitors[monid])
				continue;
			struct AmigaMonitor* mon = &AMonitors[monid];

			if (c & 64) {
				if (!unacquired) {
					inputdevice_unacquire();
					unacquired = true;
				}
			}
			if (c & 256) {
				init_colors(mon->monitor_id);
				reset_drawing();
			}
			if (c & 128) {
				if (currprefs.gfx_autoresolution) {
					c |= 2 | 8;
				} else {
					c |= 16;
					reset_drawing();
#ifdef AMIBERRY
					// Trigger auto-crop recalculations if needed
					force_auto_crop = true;
#else
					//S2X_reset(mon->monitor_id);
#endif
				}
			}
			if (c & 1024) {
				target_graphics_buffer_update(mon->monitor_id, true);
			}
			if (c & 512) {
				open_screen(&currprefs);
			}
			if ((c & 16) || ((c & 8) && keepfsmode)) {
				open_screen(&currprefs);
				c |= 2;
			}
			if ((c & 32) || ((c & 2) && !keepfsmode)) {
				if (!unacquired) {
					inputdevice_unacquire();
					unacquired = true;
				}
				close_windows(mon);
				if (currprefs.gfx_api != changed_prefs.gfx_api || currprefs.gfx_api_options != changed_prefs.gfx_api_options) {
					currprefs.gfx_api = changed_prefs.gfx_api;
					currprefs.gfx_api_options = changed_prefs.gfx_api_options;
				}
				graphics_init(!dontcapture);
			}
		}
		init_custom();
		if (c & 4) {
			pause_sound();
			reset_sound();
			resume_sound();
		}

		if (setpause || dontcapture) {
			if (!unacquired)
				inputdevice_unacquire();
			unacquired = false;
		}

		if (unacquired)
			inputdevice_acquire(TRUE);

		if (setpause)
			setpaused(1);

		return 1;
	}

	bool changed = false;
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
		if (currprefs.cr[i].rate != changed_prefs.cr[i].rate ||
			currprefs.cr[i].locked != changed_prefs.cr[i].locked) {
			memcpy(&currprefs.cr[i], &changed_prefs.cr[i], sizeof(struct chipset_refresh));
			changed = true;
		}
	}
	if (changed) {
		init_hz_normal();
	}

	if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate) {
		currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
		init_hz_normal();
		return 1;
	}

	if (
		currprefs.gf[0].gfx_filter_autoscale != changed_prefs.gf[0].gfx_filter_autoscale ||
		currprefs.gf[2].gfx_filter_autoscale != changed_prefs.gf[2].gfx_filter_autoscale ||
		currprefs.gfx_xcenter_pos != changed_prefs.gfx_xcenter_pos ||
		currprefs.gfx_ycenter_pos != changed_prefs.gfx_ycenter_pos ||
		currprefs.gfx_xcenter_size != changed_prefs.gfx_xcenter_size ||
		currprefs.gfx_ycenter_size != changed_prefs.gfx_ycenter_size ||
		currprefs.gfx_xcenter != changed_prefs.gfx_xcenter ||
		currprefs.gfx_ycenter != changed_prefs.gfx_ycenter)
	{
		currprefs.gfx_xcenter_pos = changed_prefs.gfx_xcenter_pos;
		currprefs.gfx_ycenter_pos = changed_prefs.gfx_ycenter_pos;
		currprefs.gfx_xcenter_size = changed_prefs.gfx_xcenter_size;
		currprefs.gfx_ycenter_size = changed_prefs.gfx_ycenter_size;
		currprefs.gfx_xcenter = changed_prefs.gfx_xcenter;
		currprefs.gfx_ycenter = changed_prefs.gfx_ycenter;
		currprefs.gf[0].gfx_filter_autoscale = changed_prefs.gf[0].gfx_filter_autoscale;
		currprefs.gf[2].gfx_filter_autoscale = changed_prefs.gf[2].gfx_filter_autoscale;

		get_custom_limits(nullptr, nullptr, nullptr, nullptr, nullptr);
		fixup_prefs_dimensions(&changed_prefs);

		return 1;
	}

	currprefs.filesys_limit = changed_prefs.filesys_limit;
	currprefs.harddrive_read_only = changed_prefs.harddrive_read_only;

	if (currprefs.leds_on_screen != changed_prefs.leds_on_screen ||
		currprefs.keyboard_leds[0] != changed_prefs.keyboard_leds[0] ||
		currprefs.keyboard_leds[1] != changed_prefs.keyboard_leds[1] ||
		currprefs.keyboard_leds[2] != changed_prefs.keyboard_leds[2] ||
		currprefs.input_mouse_untrap != changed_prefs.input_mouse_untrap ||
		currprefs.input_magic_mouse_cursor != changed_prefs.input_magic_mouse_cursor ||
		currprefs.minimize_inactive != changed_prefs.minimize_inactive ||
		currprefs.active_capture_priority != changed_prefs.active_capture_priority ||
		currprefs.inactive_priority != changed_prefs.inactive_priority ||
		currprefs.minimized_priority != changed_prefs.minimized_priority ||
		currprefs.active_nocapture_nosound != changed_prefs.active_nocapture_nosound ||
		currprefs.active_nocapture_pause != changed_prefs.active_nocapture_pause ||
		currprefs.active_input != changed_prefs.active_input ||
		currprefs.inactive_nosound != changed_prefs.inactive_nosound ||
		currprefs.inactive_pause != changed_prefs.inactive_pause ||
		currprefs.inactive_input != changed_prefs.inactive_input ||
		currprefs.minimized_nosound != changed_prefs.minimized_nosound ||
		currprefs.minimized_pause != changed_prefs.minimized_pause ||
		currprefs.minimized_input != changed_prefs.minimized_input ||
		currprefs.capture_always != changed_prefs.capture_always ||
		currprefs.native_code != changed_prefs.native_code ||
		currprefs.alt_tab_release != changed_prefs.alt_tab_release ||
		currprefs.use_retroarch_quit != changed_prefs.use_retroarch_quit ||
		currprefs.use_retroarch_menu != changed_prefs.use_retroarch_menu ||
		currprefs.use_retroarch_reset != changed_prefs.use_retroarch_reset ||
		currprefs.use_retroarch_vkbd != changed_prefs.use_retroarch_vkbd ||
		currprefs.sound_pullmode != changed_prefs.sound_pullmode ||
		currprefs.kbd_led_num != changed_prefs.kbd_led_num ||
		currprefs.kbd_led_scr != changed_prefs.kbd_led_scr ||
		currprefs.kbd_led_cap != changed_prefs.kbd_led_cap ||
		currprefs.turbo_boot != changed_prefs.turbo_boot ||
		currprefs.right_control_is_right_win_key != changed_prefs.right_control_is_right_win_key)
	{
		currprefs.leds_on_screen = changed_prefs.leds_on_screen;
		currprefs.keyboard_leds[0] = changed_prefs.keyboard_leds[0];
		currprefs.keyboard_leds[1] = changed_prefs.keyboard_leds[1];
		currprefs.keyboard_leds[2] = changed_prefs.keyboard_leds[2];
		currprefs.input_mouse_untrap = changed_prefs.input_mouse_untrap;
		currprefs.input_magic_mouse_cursor = changed_prefs.input_magic_mouse_cursor;
		currprefs.minimize_inactive = changed_prefs.minimize_inactive;
		currprefs.active_capture_priority = changed_prefs.active_capture_priority;
		currprefs.inactive_priority = changed_prefs.inactive_priority;
		currprefs.minimized_priority = changed_prefs.minimized_priority;
		currprefs.active_nocapture_nosound = changed_prefs.active_nocapture_nosound;
		currprefs.active_input = changed_prefs.active_input;
		currprefs.active_nocapture_pause = changed_prefs.active_nocapture_pause;
		currprefs.inactive_nosound = changed_prefs.inactive_nosound;
		currprefs.inactive_pause = changed_prefs.inactive_pause;
		currprefs.inactive_input = changed_prefs.inactive_input;
		currprefs.minimized_nosound = changed_prefs.minimized_nosound;
		currprefs.minimized_pause = changed_prefs.minimized_pause;
		currprefs.minimized_input = changed_prefs.minimized_input;
		currprefs.capture_always = changed_prefs.capture_always;
		currprefs.native_code = changed_prefs.native_code;
		currprefs.alt_tab_release = changed_prefs.alt_tab_release;
		currprefs.use_retroarch_quit = changed_prefs.use_retroarch_quit;
		currprefs.use_retroarch_menu = changed_prefs.use_retroarch_menu;
		currprefs.use_retroarch_reset = changed_prefs.use_retroarch_reset;
		currprefs.use_retroarch_vkbd = changed_prefs.use_retroarch_vkbd;
		currprefs.sound_pullmode = changed_prefs.sound_pullmode;
		currprefs.kbd_led_num = changed_prefs.kbd_led_num;
		currprefs.kbd_led_scr = changed_prefs.kbd_led_scr;
		currprefs.kbd_led_cap = changed_prefs.kbd_led_cap;
		currprefs.turbo_boot = changed_prefs.turbo_boot;
		currprefs.right_control_is_right_win_key = changed_prefs.right_control_is_right_win_key;
		inputdevice_unacquire();
		currprefs.keyboard_leds_in_use = changed_prefs.keyboard_leds_in_use = (currprefs.keyboard_leds[0] | currprefs.keyboard_leds[1] | currprefs.keyboard_leds[2]) != 0;
		pause_sound();
		resume_sound();
		inputdevice_acquire(TRUE);
		setpriority(currprefs.active_capture_priority);
		return 1;
	}

	if (currprefs.samplersoundcard != changed_prefs.samplersoundcard ||
		currprefs.sampler_stereo != changed_prefs.sampler_stereo) {
		currprefs.samplersoundcard = changed_prefs.samplersoundcard;
		currprefs.sampler_stereo = changed_prefs.sampler_stereo;
		sampler_free();
	}

	if (_tcscmp(currprefs.prtname, changed_prefs.prtname) ||
		currprefs.parallel_autoflush_time != changed_prefs.parallel_autoflush_time ||
		currprefs.parallel_matrix_emulation != changed_prefs.parallel_matrix_emulation ||
		currprefs.parallel_postscript_emulation != changed_prefs.parallel_postscript_emulation ||
		currprefs.parallel_postscript_detection != changed_prefs.parallel_postscript_detection ||
		_tcscmp(currprefs.ghostscript_parameters, changed_prefs.ghostscript_parameters)) {
		_tcscpy(currprefs.prtname, changed_prefs.prtname);
		currprefs.parallel_autoflush_time = changed_prefs.parallel_autoflush_time;
		currprefs.parallel_matrix_emulation = changed_prefs.parallel_matrix_emulation;
		currprefs.parallel_postscript_emulation = changed_prefs.parallel_postscript_emulation;
		currprefs.parallel_postscript_detection = changed_prefs.parallel_postscript_detection;
		_tcscpy(currprefs.ghostscript_parameters, changed_prefs.ghostscript_parameters);
#ifdef PARALLEL_PORT
		//closeprinter();
#endif
	}

	if (_tcscmp(currprefs.sername, changed_prefs.sername) ||
		currprefs.serial_hwctsrts != changed_prefs.serial_hwctsrts ||
		currprefs.serial_direct != changed_prefs.serial_direct ||
		currprefs.serial_demand != changed_prefs.serial_demand) {
		_tcscpy(currprefs.sername, changed_prefs.sername);
		currprefs.serial_hwctsrts = changed_prefs.serial_hwctsrts;
		currprefs.serial_demand = changed_prefs.serial_demand;
		currprefs.serial_direct = changed_prefs.serial_direct;
#ifdef SERIAL_PORT
		serial_exit();
		serial_init();
#endif
	}

	if (_tcscmp(currprefs.midiindev, changed_prefs.midiindev) ||
		_tcscmp(currprefs.midioutdev, changed_prefs.midioutdev) ||
		currprefs.midirouter != changed_prefs.midirouter)
	{
		_tcscpy(currprefs.midiindev, changed_prefs.midiindev);
		_tcscpy(currprefs.midioutdev, changed_prefs.midioutdev);
		currprefs.midirouter = changed_prefs.midirouter;

#ifdef SERIAL_PORT
		serial_exit();
		serial_init();
#ifdef WITH_MIDI
		Midi_Reopen();
#endif
#endif
#ifdef WITH_MIDIEMU
		midi_emu_reopen();
#endif
	}

#ifdef AMIBERRY
	// Virtual keyboard
	if (currprefs.vkbd_enabled != changed_prefs.vkbd_enabled ||
		currprefs.vkbd_hires != changed_prefs.vkbd_hires ||
		currprefs.vkbd_transparency != changed_prefs.vkbd_transparency ||
		currprefs.vkbd_exit != changed_prefs.vkbd_exit ||
		_tcscmp(currprefs.vkbd_language, changed_prefs.vkbd_language) ||
		_tcscmp(currprefs.vkbd_style, changed_prefs.vkbd_style) ||
		_tcscmp(currprefs.vkbd_toggle, changed_prefs.vkbd_toggle))
	{
		currprefs.vkbd_enabled = changed_prefs.vkbd_enabled;
		currprefs.vkbd_hires = changed_prefs.vkbd_hires;
		currprefs.vkbd_transparency = changed_prefs.vkbd_transparency;
		currprefs.vkbd_exit = changed_prefs.vkbd_exit;
		_tcscpy(currprefs.vkbd_language, changed_prefs.vkbd_language);
		_tcscpy(currprefs.vkbd_style, changed_prefs.vkbd_style);
		_tcscpy(currprefs.vkbd_toggle, changed_prefs.vkbd_toggle);

		if (currprefs.vkbd_enabled)
		{
			vkbd_set_transparency(static_cast<double>(currprefs.vkbd_transparency) / 100.0);
			vkbd_set_hires(currprefs.vkbd_hires);
			vkbd_set_keyboard_has_exit_button(currprefs.vkbd_exit);
			vkbd_set_language(string(currprefs.vkbd_language));
			vkbd_set_style(string(currprefs.vkbd_style));
			vkbd_button = SDL_GameControllerGetButtonFromString(currprefs.vkbd_toggle);
			if (vkbd_allowed(0))
				vkbd_init();
		}
		else
		{
			vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;
			vkbd_quit();
		}
	}
#endif

	return 0;
}

/* Color management */

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

void init_colors(const int monid)
{
	/* Truecolor: */
	red_bits = bits_in_mask(amiga_surface->format->Rmask);
	green_bits = bits_in_mask(amiga_surface->format->Gmask);
	blue_bits = bits_in_mask(amiga_surface->format->Bmask);
	red_shift = mask_shift(amiga_surface->format->Rmask);
	green_shift = mask_shift(amiga_surface->format->Gmask);
	blue_shift = mask_shift(amiga_surface->format->Bmask);
	alpha_bits = bits_in_mask(amiga_surface->format->Amask);
	alpha_shift = mask_shift(amiga_surface->format->Amask);

	alloc_colors64k(monid, red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, alpha_bits, alpha_shift, alpha, 0, false);
	notice_new_xcolors();
}

#ifdef PICASSO96

int picasso_palette(struct MyCLUTEntry *CLUT, uae_u32 *clut)
{
	int changed = 0;

	for (int i = 0; i < 256 * 2; i++) {
		const int r = CLUT[i].Red;
		const int g = CLUT[i].Green;
		const int b = CLUT[i].Blue;
		const uae_u32 v = (doMask256 (r, red_bits, red_shift)
			| doMask256(g, green_bits, green_shift)
			| doMask256(b, blue_bits, blue_shift))
			| doMask256 ((1 << alpha_bits) - 1, alpha_bits, alpha_shift);
		if (v != clut[i]) {
			//write_log (_T("%d:%08x\n"), i, v);
			clut[i] = v;
			changed = 1;
		}
	}
	return changed;
}

void DX_Invalidate(struct AmigaMonitor* mon, int x, int y, int width, int height)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	int last, lastx;

	if (width == 0 || height == 0)
		return;
	if (y < 0 || height < 0) {
		y = 0;
		height = vidinfo->height;
	}
	if (x < 0 || width < 0) {
		x = 0;
		width = vidinfo->width;
	}
	last = y + height - 1;
	lastx = x + width - 1;
	mon->p96_double_buffer_first = y;
	mon->p96_double_buffer_last = last;
	mon->p96_double_buffer_firstx = x;
	mon->p96_double_buffer_lastx = lastx;
	mon->p96_double_buffer_needs_flushing = 1;
}

#endif
static void updatepicasso96(struct AmigaMonitor* mon);
static void allocsoftbuffer(int monid, const TCHAR* name, struct vidbuffer* buf, int flags, int width, int height, int depth);

static void open_screen(struct uae_prefs* p)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	auto* avidinfo = &adisplays[0].gfxvidinfo;

	updatemodes(mon);
	update_gfxparams(mon);

	graphics_subshutdown();

	if (!mon->amiga_window)
		SDL2_init();

	if (max_uae_width == 0 || max_uae_height == 0)
	{
		max_uae_width = 8192;
		max_uae_height = 8192;
	}

	if (wasfullwindow_a == 0)
		wasfullwindow_a = p->gfx_apmode[APMODE_NATIVE].gfx_fullscreen == GFX_FULLWINDOW ? 1 : -1;
	if (wasfullwindow_p == 0)
		wasfullwindow_p = p->gfx_apmode[APMODE_RTG].gfx_fullscreen == GFX_FULLWINDOW ? 1 : -1;

	updatewinfsmode(0, p);

	if (mon->screen_is_picasso)
	{
		if (picasso96_state[0].RGBFormat == RGBFB_R5G6B5
			|| picasso96_state[0].RGBFormat == RGBFB_R5G6B5PC
			|| picasso96_state[0].RGBFormat == RGBFB_CLUT)
		{
			display_depth = 16;
			pixel_format = SDL_PIXELFORMAT_RGB565;
		}
		else
		{
			display_depth = 32;
			pixel_format = SDL_PIXELFORMAT_BGRA32;
		}
		display_width = picasso96_state[0].Width ? picasso96_state[0].Width : 640;
		display_height = picasso96_state[0].Height ? picasso96_state[0].Height : 480;
	}
	else // Native screen mode
	{
		mon->currentmode.native_depth = mon->currentmode.current_depth;

		avidinfo->gfx_resolution_reserved = std::max(currprefs.gfx_resolution, avidinfo->gfx_resolution_reserved);
		avidinfo->gfx_vresolution_reserved = std::max(currprefs.gfx_vresolution, avidinfo->gfx_vresolution_reserved);

		if (!currprefs.gfx_autoresolution) {
			mon->currentmode.amiga_width = AMIGA_WIDTH_MAX << currprefs.gfx_resolution;
			mon->currentmode.amiga_height = AMIGA_HEIGHT_MAX << currprefs.gfx_vresolution;
		} else {
			mon->currentmode.amiga_width = AMIGA_WIDTH_MAX << avidinfo->gfx_resolution_reserved;
			mon->currentmode.amiga_height = AMIGA_HEIGHT_MAX << avidinfo->gfx_vresolution_reserved;
		}
		if (avidinfo->gfx_resolution_reserved == RES_SUPERHIRES)
			mon->currentmode.amiga_height *= 2;
		mon->currentmode.amiga_height = std::min(mon->currentmode.amiga_height, 1280);

		avidinfo->drawbuffer.inwidth = avidinfo->drawbuffer.outwidth = mon->currentmode.amiga_width;
		avidinfo->drawbuffer.inheight = avidinfo->drawbuffer.outheight = mon->currentmode.amiga_height;

		mon->currentmode.pitch = mon->currentmode.amiga_width * mon->currentmode.current_depth >> 3;

		display_depth = 32;
		pixel_format = SDL_PIXELFORMAT_BGRA32;

		display_width = mon->currentmode.amiga_width;
		display_height = mon->currentmode.amiga_height;

		// Force recalculation of row maps - if we're locking
		old_pixels = (void*)-1;
	}

	amiga_surface = SDL_CreateRGBSurfaceWithFormat(0, mon->screen_is_picasso ? display_width : 1920, mon->screen_is_picasso ? display_height : 1280, display_depth, pixel_format);
	check_error_sdl(amiga_surface == nullptr, "Unable to create a surface");

	statusline_set_multiplier(mon->monitor_id, display_width, display_height);
	setpriority(p->active_capture_priority);

	updatepicasso96(mon);

	avidinfo->outbuffer = &avidinfo->drawbuffer;
	avidinfo->inbuffer = &avidinfo->drawbuffer;

	if (!mon->screen_is_picasso)
	{
		allocsoftbuffer(mon->monitor_id, _T("draw"), &avidinfo->drawbuffer, 0, 1920, 1280, display_depth);
		if (currprefs.monitoremu || currprefs.cs_cd32fmv || ((currprefs.genlock || currprefs.genlock_effects) && currprefs.genlock_image) || currprefs.cs_color_burst || currprefs.gfx_grayscale) {
			allocsoftbuffer(mon->monitor_id, _T("monemu"), &avidinfo->tempbuffer, mon->currentmode.flags,
			                mon->currentmode.amiga_width > 1024 ? mon->currentmode.amiga_width : 1024,
			                mon->currentmode.amiga_height > 1024 ? mon->currentmode.amiga_height : 1024,
			                mon->currentmode.current_depth);
		}
		init_row_map();
	}

	updatewinrect(mon, true);
	mon->screen_is_initialized = 1;

	init_colors(mon->monitor_id);
	target_graphics_buffer_update(mon->monitor_id, false);
	picasso_refresh(mon->monitor_id);
	setmouseactive(mon->monitor_id, -1);

	if (vkbd_allowed(0))
	{
		vkbd_set_transparency(static_cast<double>(currprefs.vkbd_transparency) / 100.0);
		vkbd_set_hires(currprefs.vkbd_hires);
		vkbd_set_keyboard_has_exit_button(currprefs.vkbd_exit);
		vkbd_set_language(string(currprefs.vkbd_language));
		vkbd_set_style(string(currprefs.vkbd_style));
		vkbd_init();
	}
}

bool vsync_switchmode(const int monid, int hz)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	static struct PicassoResolution* oldmode;
	static int oldhz;
	int w = mon->currentmode.native_width;
	int h = mon->currentmode.native_height;
	int d = mon->currentmode.native_depth / 8;
	struct MultiDisplay* md = getdisplay(&currprefs, monid);
	struct PicassoResolution* found;
	int newh, cnt;
	bool preferdouble = false, preferlace = false;
	bool lace = false;

	if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate > 85) {
		preferdouble = true;
	} else if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced) {
		preferlace = true;
	}

	if (hz >= 55)
		hz = 60;
	else
		hz = 50;

	newh = h * (currprefs.ntscmode ? 60 : 50) / hz;

	found = nullptr;
	for (cnt = 0; cnt <= abs(newh - h) + 1 && !found; cnt++) {
		for (int dbl = 0; dbl < 2 && !found; dbl++) {
			bool doublecheck = false;
			bool lacecheck = false;
			if (preferdouble && dbl == 0)
				doublecheck = true;
			else if (preferlace && dbl == 0)
				lacecheck = true;

			for (int extra = 1; extra >= -1 && !found; extra--) {
				for (int i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
					struct PicassoResolution* r = &md->DisplayModes[i];
					if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt) && r->depth == d) {
						for (int j = 0; r->refresh[j] > 0; j++) {
							if (doublecheck) {
								if (r->refreshtype[j] & REFRESH_RATE_LACE)
									continue;
								if (r->refresh[j] == hz * 2 + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							} else if (lacecheck) {
								if (!(r->refreshtype[j] & REFRESH_RATE_LACE))
									continue;
								if (r->refresh[j] * 2 == hz + extra) {
									found = r;
									lace = true;
									hz = r->refresh[j];
									break;
								}
							} else {
								if (r->refresh[j] == hz + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	if (found == oldmode && hz == oldhz)
		return true;
	oldmode = found;
	oldhz = hz;
	if (!found) {
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync = 0;
		if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync) {
			set_config_changed();
		}
		write_log(_T("refresh rate changed to %d%s but no matching screenmode found, vsync disabled\n"), hz, lace ? _T("i") : _T("p"));
		return false;
	} else {
		newh = static_cast<int>(found->res.height);
		changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height = newh;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = hz;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = lace;
		if (changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height != currprefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height ||
			changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate != currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate) {
			write_log(_T("refresh rate changed to %d%s, new screenmode %dx%d\n"), hz, lace ? _T("i") : _T("p"), w, newh);
			set_config_changed();
		}
		return true;
	}
}

void vsync_clear()
{
	vsync_active = false;
}

int vsync_isdone(frame_time_t* dt)
{
	if (isvsync() == 0)
		return -1;
	if (waitvblankthread_mode <= 0)
		return -2;
	if (dt)
		*dt = wait_vblank_timestamp;
	return vsync_active ? 1 : 0;
}

#ifdef PICASSO96

static int modeswitchneeded(struct AmigaMonitor* mon, struct winuae_currentmode* wc)
{
	struct vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];

	if (isfullscreen() > 0) {
		/* fullscreen to fullscreen */
		if (mon->screen_is_picasso) {
			if (state->BytesPerPixel > 1 && state->BytesPerPixel * 8 != wc->current_depth && canmatchdepth())
				return -1;
			if (state->Width < wc->current_width && state->Height < wc->current_height) {
				if ((currprefs.gf[GF_RTG].gfx_filter_autoscale == 1 || (currprefs.gf[GF_RTG].gfx_filter_autoscale == 2 && currprefs.rtgallowscaling)) && !canmatchdepth())
					return 0;
			}
			if (state->Width != wc->current_width ||
				state->Height != wc->current_height)
				return 1;
			if (state->Width == wc->current_width &&
				state->Height == wc->current_height) {
				if (state->BytesPerPixel * 8 == wc->current_depth || state->BytesPerPixel == 1)
					return 0;
				if (!canmatchdepth())
					return 0;
			}
			return 1;
		} else {
			if (mon->currentmode.current_width != wc->current_width ||
				mon->currentmode.current_height != wc->current_height ||
				mon->currentmode.current_depth != wc->current_depth)
				return -1;
			if (!avidinfo->outbuffer->bufmem_lockable)
				return -1;
		}
	} else if (isfullscreen () == 0) {
		/* windowed to windowed */
		return -2;
	} else {
		/* fullwindow to fullwindow */
		if (mon->screen_is_picasso) {
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale && ((wc->native_width > state->Width && wc->native_height >= state->Height) || (wc->native_height > state->Height && wc->native_width >= state->Width)))
				return -1;
			if (currprefs.rtgallowscaling && (state->Width != wc->native_width || state->Height != wc->native_height))
				return -1;
#if 0
			if (wc->native_width < state->Width || wc->native_height < state->Height)
				return 1;
#endif
		}
		return -1;
	}
	return 0;
}

void gfx_set_picasso_state(const int monid, const int on)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	if (mon->screen_is_picasso == on)
		return;
	mon->screen_is_picasso = on;

	updatemodes(mon);
	update_gfxparams(mon);

	open_screen(&currprefs);
}

static void updatepicasso96(struct AmigaMonitor* mon)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	vidinfo->rowbytes = 0;
	vidinfo->pixbytes = amiga_surface->format->BytesPerPixel;
	vidinfo->rgbformat = 0;
	vidinfo->extra_mem = 1;
	vidinfo->height = amiga_surface->h;
	vidinfo->width = amiga_surface->w;
	vidinfo->depth = amiga_surface->format->BytesPerPixel * 8;
	vidinfo->offset = 0;
	vidinfo->splitypos = -1;
}

void gfx_set_picasso_modeinfo(const int monid, const RGBFTYPE rgbfmt)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];
	int need;
	if (!mon->screen_is_picasso)
		return;
	gfx_set_picasso_colors(monid, rgbfmt);
	need = modeswitchneeded(mon, &mon->currentmode);
	update_gfxparams(mon);
	updatemodes(mon);

	if (need != 0)
		open_screen(&currprefs);

	state->ModeChanged = false;
	target_graphics_buffer_update(monid, false);
}

void gfx_set_picasso_colors(int monid, const RGBFTYPE rgbfmt)
{
	alloc_colors_picasso(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt, p96_rgbx16);
}
#endif

int machdep_init()
{
	for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
		struct AmigaMonitor* mon = &AMonitors[i];
		struct amigadisplay* ad = &adisplays[i];
		mon->monitor_id = i;
		ad->picasso_requested_on = false;
		ad->picasso_on = false;
		mon->screen_is_picasso = 0;
		memset(&mon->currentmode, 0, sizeof(mon->currentmode));
	}

	return 1;
}

static void graphics_subinit()
{
	if (amiga_surface == nullptr)
	{
		open_screen(&currprefs);
		if (amiga_surface == nullptr)
			write_log("Unable to set video mode: %s\n", SDL_GetError());
	}
}

int graphics_init(bool mousecapture)
{
	SDL2_init();

	inputdevice_unacquire();
	graphics_subinit();

	inputdevice_acquire(TRUE);
	return 1;
}

int graphics_setup()
{
	if (!screen_cs_allocated) {
		screen_cs = SDL_CreateMutex();
		if (screen_cs == NULL) {
			write_log(_T("Couldn't create screen_cs: %s\n"), SDL_GetError());
			return 0;
		}
		screen_cs_allocated = true;
	}
#ifdef PICASSO96
	InitPicasso96(0);
#endif
	return 1;
}

void graphics_leave()
{
	struct AmigaMonitor* mon = &AMonitors[0];
	close_windows(mon);

	if (kmsdrm_detected)
	{
		if (mon->amiga_renderer)
		{
			SDL_DestroyRenderer(mon->amiga_renderer);
			mon->amiga_renderer = nullptr;
		}
		if (mon->gui_renderer)
		{
			SDL_DestroyRenderer(mon->gui_renderer);
			mon->gui_renderer = nullptr;
		}
		if (mon->amiga_window)
		{
			SDL_DestroyWindow(mon->amiga_window);
			mon->amiga_window = nullptr;
		}
		if (mon->gui_window)
		{
			SDL_DestroyWindow(mon->gui_window);
			mon->gui_window = nullptr;
		}
	}

	SDL_DestroyMutex(screen_cs);
	screen_cs = nullptr;
	screen_cs_allocated = false;
}

void close_windows(struct AmigaMonitor* mon)
{
	reset_sound();
	graphics_subshutdown();

#ifdef USE_OPENGL
	destroy_crtemu();
#else
	if (amiga_texture)
	{
		SDL_DestroyTexture(amiga_texture);
		amiga_texture = nullptr;
	}
#endif

#ifdef USE_OPENGL
	if (gl_context != nullptr)
	{
		SDL_GL_DeleteContext(gl_context);
		gl_context = nullptr;
	}
#else
	if (mon->amiga_renderer && !kmsdrm_detected)
	{
		SDL_DestroyRenderer(mon->amiga_renderer);
		mon->amiga_renderer = nullptr;
	}
#endif
	if (mon->amiga_window && !kmsdrm_detected)
	{
		SDL_DestroyWindow(mon->amiga_window);
		mon->amiga_window = nullptr;
	}

	if (currprefs.vkbd_enabled)
		vkbd_quit();
}

float target_getcurrentvblankrate(const int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	float vb;
	if (currprefs.gfx_variable_sync)
		return (float)mon->currentmode.freq;
	if (get_display_vblank_params(-1, nullptr, nullptr, &vb, nullptr)) {
		return vb;
	}

	return SDL2_getrefreshrate(0);
}

static void allocsoftbuffer(const int monid, const TCHAR* name, struct vidbuffer* buf, int flags, const int width, const int height, const int depth)
{
	/* Initialize structure for Amiga video modes */
	buf->monitor_id = monid;
	buf->pixbytes = amiga_surface->format->BytesPerPixel;
	buf->width_allocated = (width + 7) & ~7;
	buf->height_allocated = height;

	buf->rowbytes = amiga_surface->pitch;
	buf->realbufmem = static_cast<uae_u8*>(amiga_surface->pixels);
	buf->bufmem_allocated = buf->bufmem = buf->realbufmem;
	buf->bufmem_lockable = true;

	if (amiga_surface->format->BytesPerPixel == 2)
		currprefs.color_mode = changed_prefs.color_mode = 2;
	else
		currprefs.color_mode = changed_prefs.color_mode = 5;

	write_log(_T("Mon %d allocated %s temp buffer (%d*%d*%d) = %p\n"), monid, name, width, height, depth, buf->realbufmem);
}

static int oldtex_w[MAX_AMIGAMONITORS], oldtex_h[MAX_AMIGAMONITORS], oldtex_rtg[MAX_AMIGAMONITORS];
bool target_graphics_buffer_update(const int monid, const bool force)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct vidbuf_description* avidinfo = &adisplays[monid].gfxvidinfo;
	struct picasso96_state_struct* state = &picasso96_state[monid];

	int w, h, depth;

	if (mon->screen_is_picasso) {
		w = state->Width;
		h = state->Height;
		depth = state->RGBFormat == RGBFB_R5G6B5
			|| state->RGBFormat == RGBFB_R5G6B5PC
			|| state->RGBFormat == RGBFB_B5G6R5PC
			|| state->RGBFormat == RGBFB_CLUT
			? 16 : 32;
	} else {
		struct vidbuffer* vb = avidinfo->drawbuffer.tempbufferinuse ? &avidinfo->tempbuffer : &avidinfo->drawbuffer;
		avidinfo->outbuffer = vb;
		w = vb->outwidth;
		h = vb->outheight;
		depth = 32;
	}

	if (!force && oldtex_w[monid] == w && oldtex_h[monid] == h && oldtex_rtg[monid] == mon->screen_is_picasso && SDL2_alloctexture(mon->monitor_id, -w, -h, depth)) {
		//osk_setup(monid, -2);
		return false;
	}

	if (!w || !h) {
		oldtex_w[monid] = w;
		oldtex_h[monid] = h;
		oldtex_rtg[monid] = mon->screen_is_picasso;
		return false;
	}

	if (!SDL2_alloctexture(mon->monitor_id, w, h, depth)) {
		return false;
	}

	oldtex_w[monid] = w;
	oldtex_h[monid] = h;
	oldtex_rtg[monid] = mon->screen_is_picasso;

	write_log(_T("Buffer %d size (%d*%d) %s\n"), monid, w, h, mon->screen_is_picasso ? _T("RTG") : _T("Native"));

	if (mon->screen_is_picasso)
	{
		if (mon->amiga_window && isfullscreen() == 0)
		{
			if (mon->amigawin_rect.w > w || mon->amigawin_rect.h > h)
			{
				SDL_SetWindowSize(mon->amiga_window, mon->amigawin_rect.w, mon->amigawin_rect.h);
			}
			else
			{
				SDL_SetWindowSize(mon->amiga_window, w, h);
			}
		}
#ifdef USE_OPENGL
		renderQuad = { dx, dy, w, h };
		crop_rect = { dx, dy, w, h };
		set_scaling_option(&currprefs, w, h);
#else
		if (mon->amiga_renderer) {
			if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180) {
				SDL_RenderSetLogicalSize(mon->amiga_renderer, w, h);
				renderQuad = {dx, dy, w, h};
				crop_rect = {dx, dy, w, h};
			}
			else
			{
				SDL_RenderSetLogicalSize(mon->amiga_renderer, h, w);
				renderQuad = { -(w - h) / 2, (w - h) / 2, w, h };
				crop_rect = { -(w - h) / 2, (w - h) / 2, w, h };
			}
			set_scaling_option(&currprefs, w, h);
		}
		else
			return false;
#endif
	}
	else
	{
		int scaled_width = w;
		int scaled_height = h;
		if (currprefs.gfx_vresolution == VRES_NONDOUBLE)
		{
			if (currprefs.gfx_resolution == RES_HIRES || currprefs.gfx_resolution == RES_SUPERHIRES)
				scaled_height *= 2;
//			else if (currprefs.gfx_resolution == RES_SUPERHIRES)
//				scaled_width /= 2;
		}
		else
		{
			if (currprefs.gfx_resolution == RES_LORES)
				scaled_width *= 2;
//			else if (currprefs.gfx_resolution == RES_SUPERHIRES)
//				scaled_width /= 2;
		}

		if (mon->amiga_window && isfullscreen() == 0)
		{
			if (mon->amigawin_rect.w > 800 && mon->amigawin_rect.h != 600)
			{
				SDL_SetWindowSize(mon->amiga_window, mon->amigawin_rect.w, mon->amigawin_rect.h);
			}
			else
			{
				SDL_SetWindowSize(mon->amiga_window, scaled_width, scaled_height);
			}
		}
#ifdef USE_OPENGL
		if (!currprefs.gfx_auto_crop && !currprefs.gfx_manual_crop) {
			renderQuad = { dx, dy, scaled_width, scaled_height };
			crop_rect = { dx, dy, scaled_width, scaled_height };
		}
		else if (currprefs.gfx_manual_crop)
		{
			renderQuad = { dx, dy, scaled_width, scaled_height };
			crop_rect = { currprefs.gfx_horizontal_offset, currprefs.gfx_vertical_offset, currprefs.gfx_manual_crop_width, currprefs.gfx_manual_crop_height };
		}
		set_scaling_option(&currprefs, scaled_width, scaled_height);
#else
		if (mon->amiga_renderer)
		{
			if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180) {
				SDL_RenderSetLogicalSize(mon->amiga_renderer, scaled_width, scaled_height);
				if (!currprefs.gfx_auto_crop && !currprefs.gfx_manual_crop) {
					renderQuad = {dx, dy, scaled_width, scaled_height};
					crop_rect = {dx, dy, w, h};
				}
				else if (currprefs.gfx_manual_crop)
				{
					renderQuad = { dx, dy, scaled_width, scaled_height };
					crop_rect = { currprefs.gfx_horizontal_offset, currprefs.gfx_vertical_offset, currprefs.gfx_manual_crop_width, currprefs.gfx_manual_crop_height };
				}
			}
			else
			{
				SDL_RenderSetLogicalSize(mon->amiga_renderer, scaled_height, scaled_width);
				if (!currprefs.gfx_auto_crop && !currprefs.gfx_manual_crop) {
					renderQuad = { -(scaled_width - scaled_height) / 2, (scaled_width - scaled_height) / 2, scaled_width, scaled_height };
					crop_rect = { -(w - h) / 2, (w - h) / 2, w, h };
				}
			}
			set_scaling_option(&currprefs, scaled_width, scaled_height);
		}
		else
		{
			return false;
		}
#endif
	}

	return true;
}

void updatedisplayarea(const int monid)
{
	set_custom_limits(-1, -1, -1, -1, false);
	show_screen(monid, 0);
}

void updatewinfsmode(const int monid, struct uae_prefs* p)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	auto* const ad = &adisplays[monid];
	const int id = ad->picasso_on ? APMODE_RTG : APMODE_NATIVE;
	auto* avidinfo = &adisplays[monid].gfxvidinfo;
	const bool borderless = p->borderless;

	if (mon->amiga_window)
	{
		const auto window_flags = SDL_GetWindowFlags(mon->amiga_window);
		const bool is_fullwindow = window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
		const bool is_fullscreen = window_flags & SDL_WINDOW_FULLSCREEN;
		const bool is_borderless = window_flags & SDL_WINDOW_BORDERLESS;

		if (p->gfx_apmode[id].gfx_fullscreen == GFX_FULLSCREEN && !is_fullscreen)
		{
			p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_fs;
			SDL_DisplayMode dm = {};
			dm.format = SDL_PIXELFORMAT_BGRA32;
			dm.w = p->gfx_monitor[monid].gfx_size.width;
			dm.h = p->gfx_monitor[monid].gfx_size.height;
			dm.refresh_rate = p->gfx_apmode[id].gfx_refreshrate;
			if (SDL_SetWindowDisplayMode(mon->amiga_window, &dm) != 0) {
				// Handle error
				write_log("Failed to set display mode: %s\n", SDL_GetError());
			}
			if (SDL_SetWindowFullscreen(mon->amiga_window, SDL_WINDOW_FULLSCREEN) != 0) {
				// Handle error
				write_log("Failed to set window to fullscreen: %s\n", SDL_GetError());
			}
			set_config_changed();
		}
		else if (p->gfx_apmode[id].gfx_fullscreen == GFX_FULLWINDOW && !is_fullwindow)
		{
			p->gfx_monitor[id].gfx_size = p->gfx_monitor[monid].gfx_size_win;
			SDL_SetWindowFullscreen(mon->amiga_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			set_config_changed();
		}
		else if (p->gfx_apmode[id].gfx_fullscreen == GFX_WINDOW && (is_fullscreen || is_fullwindow) && !kmsdrm_detected)
		{
			p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_win;
			SDL_SetWindowFullscreen(mon->amiga_window, 0);
			set_config_changed();
		}

		if (borderless != is_borderless)
		{
			SDL_SetWindowBordered(mon->amiga_window, borderless ? SDL_FALSE : SDL_TRUE);
			set_config_changed();
		}
	}

	if (!mon->screen_is_picasso)
		force_auto_crop = true;
}

int rtg_index = -1;

// -2 = default
// -1 = prev
// 0 = chipset
// 1..4 = rtg
// 5 = next
bool toggle_rtg(const int monid, const int mode)
{
	struct amigadisplay* ad = &adisplays[monid];

	int old_index = rtg_index;

	if (monid > 0) {
		return true;
	}

	if (mode < -1 && rtg_index >= 0)
		return true;

	for (;;) {
		if (mode == -1) {
			rtg_index--;
		} else if (mode >= 0 && mode <= MAX_RTG_BOARDS) {
			rtg_index = mode - 1;
		} else {
			rtg_index++;
		}
		if (rtg_index >= MAX_RTG_BOARDS) {
			rtg_index = -1;
		} else if (rtg_index < -1) {
			rtg_index = MAX_RTG_BOARDS - 1;
		}
		if (rtg_index < 0) {
			if (ad->picasso_on) {
				gfxboard_rtg_disable(monid, old_index);
				ad->picasso_requested_on = false;
				statusline_add_message(STATUSTYPE_DISPLAY, _T("Chipset display"));
				set_config_changed();
				return false;
			}
			return false;
		}
		struct rtgboardconfig* r = &currprefs.rtgboards[rtg_index];
		if (r->rtgmem_size > 0 && r->monitor_id == monid) {
			if (r->rtgmem_type >= GFXBOARD_HARDWARE) {
				int idx = gfxboard_toggle(r->monitor_id, rtg_index, mode >= -1);
				if (idx >= 0) {
					rtg_index = idx;
					return true;
				}
				if (idx < -1) {
					rtg_index = -1;
					return false;
				}
			} else {
				gfxboard_toggle(r->monitor_id, -1, -1);
				if (mode < -1)
					return true;
				devices_unsafeperiod();
				gfxboard_rtg_disable(monid, old_index);
				// can always switch from RTG to custom
				if (ad->picasso_requested_on && ad->picasso_on) {
					ad->picasso_requested_on = false;
					rtg_index = -1;
					set_config_changed();
					return true;
				}
				if (ad->picasso_on)
					return false;
				// can only switch from custom to RTG if there is some mode active
				if (picasso_is_active(r->monitor_id)) {
					picasso_enablescreen(r->monitor_id, 1);
					ad->picasso_requested_on = true;
					statusline_add_message(STATUSTYPE_DISPLAY, _T("RTG %d: %s"), rtg_index + 1, _T("UAEGFX"));
					set_config_changed();
					return true;
				}
			}
		}
		if (mode >= 0 && mode <= MAX_RTG_BOARDS) {
			rtg_index = old_index;
			return false;
		}
	}
	return false;
}

void close_rtg(const int monid, const bool reset)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	close_windows(mon);
	if (reset) {
		struct amigadisplay* ad = &adisplays[monid];
		mon->screen_is_picasso = false;
		ad->picasso_on = false;
		ad->picasso_requested_on = false;
		ad->picasso_requested_forced_on = false;
	}
}

void toggle_fullscreen(const int monid, const int mode)
{
	auto* const ad = &adisplays[monid];
	auto* p = ad->picasso_on ? &changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen : &changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen;
	const auto wfw = ad->picasso_on ? wasfullwindow_p : wasfullwindow_a;
	auto v = *p;
	static int prevmode = -1;

	if (mode < 0) {
		// fullwindow->window->fullwindow.
		// fullscreen->window->fullscreen.
		// window->fullscreen->window.
		if (v == GFX_FULLWINDOW) {
			prevmode = v;
			v = GFX_WINDOW;
		} else if (v == GFX_WINDOW) {
			if (prevmode < 0) {
				v = GFX_FULLSCREEN;
				prevmode = v;
			} else {
				v = prevmode;
			}
		} else if (v == GFX_FULLSCREEN) {
			if (wfw > 0)
				v = GFX_FULLWINDOW;
			else
				v = GFX_WINDOW;
		}
	} else if (mode == 0) {
		prevmode = v;
		// fullscreen <> window
		if (v == GFX_FULLSCREEN)
			v = GFX_WINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 1) {
		prevmode = v;
		// fullscreen <> fullwindow
		if (v == GFX_FULLSCREEN)
			v = GFX_FULLWINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 2) {
		prevmode = v;
		// window <> fullwindow
		if (v == GFX_FULLWINDOW)
			v = GFX_WINDOW;
		else
			v = GFX_FULLWINDOW;
	} else if (mode == 10) {
		v = GFX_WINDOW;
	}
	*p = v;
	devices_unsafeperiod();
	updatewinfsmode(monid, &changed_prefs);
}

void destroy_crtemu()
{
#ifdef USE_OPENGL
	if (crtemu_tv != nullptr)
	{
		crtemu_destroy(crtemu_tv);
		crtemu_tv = nullptr;
	}
#endif
}

void graphics_subshutdown()
{
	reset_sound();

	SDL_FreeSurface(amiga_surface);
	amiga_surface = nullptr;

	auto* avidinfo = &adisplays[0].gfxvidinfo;
	avidinfo->drawbuffer.realbufmem = nullptr;
	avidinfo->drawbuffer.bufmem = nullptr;
	avidinfo->drawbuffer.bufmem_allocated = nullptr;
	avidinfo->drawbuffer.bufmem_lockable = false;

#ifdef USE_OPENGL
	destroy_crtemu();
#else
	if (amiga_texture != nullptr)
	{
		SDL_DestroyTexture(amiga_texture);
		amiga_texture = nullptr;
	}
#endif
}

#ifdef AMIBERRY
void auto_crop_image()
{
	const AmigaMonitor* mon = &AMonitors[0];
	static bool last_autocrop;

	if (currprefs.gfx_auto_crop)
	{
		static int last_cw = 0, last_ch = 0, last_cx = 0, last_cy = 0;
		int cw, ch, cx, cy, crealh = 0;
		get_custom_limits(&cw, &ch, &cx, &cy, &crealh);

		if (!force_auto_crop && last_autocrop == currprefs.gfx_auto_crop && last_cw == cw && last_ch == ch && last_cx == cx && last_cy == cy)
		{
			return;
		}

		last_cw = cw;
		last_ch = ch;
		last_cx = cx;
		last_cy = cy;
		force_auto_crop = false;

		int width = cw;
		int height = ch;
		if (currprefs.gfx_vresolution == VRES_NONDOUBLE)
		{
			if (currprefs.gfx_resolution == RES_HIRES || currprefs.gfx_resolution == RES_SUPERHIRES)
				height *= 2;
		}
		if (currprefs.gfx_correct_aspect == 0)
		{
			width = sdl_mode.w;
			height = sdl_mode.h;
		}
#ifdef USE_OPENGL
		renderQuad = { dx, dy, width, height };
		crop_rect = { cx, cy, cw, ch };
#else

		if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		{
			SDL_RenderSetLogicalSize(mon->amiga_renderer, width, height);
			renderQuad = { dx, dy, width, height };
			crop_rect = { cx, cy, cw, ch };
		}
		else
		{
			SDL_RenderSetLogicalSize(mon->amiga_renderer, height, width);
			renderQuad = { -(width - height) / 2, (width - height) / 2, width, height };
		}

		if (vkbd_allowed(0))
		{
			vkbd_update_position_from_texture();
		}
#endif
	}

	last_autocrop = currprefs.gfx_auto_crop;
}
#endif

unsigned long target_lastsynctime()
{
	return last_synctime;
}

/*
* Find the colour depth of the display
*/
static int get_display_depth()
{
	return amiga_surface->format->BytesPerPixel * 8;
}

void update_display(struct uae_prefs* p)
{
	open_screen(p);
}

static int save_png(const SDL_Surface* surface, const std::string& path)
{
	const auto w = surface->w;
	const auto h = surface->h;
	auto* const pix = static_cast<unsigned char*>(surface->pixels);
	unsigned char writeBuffer[1920 * 3]{};

	// Open the file for writing
	auto* const f = fopen(path.c_str(), "wbe");
	if (!f)
	{
		write_log(_T("Failed to open file for writing: %s\n"), path.c_str());
		return 0;
	}

	// Create a PNG write structure
	auto* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr)
	{
		write_log(_T("Failed to create PNG write structure\n"));
		fclose(f);
		return 0;
	}

	auto* info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, nullptr);
		fclose(f);
		return 0;
	}

	png_init_io(png_ptr, f);
	png_set_IHDR(png_ptr,
		info_ptr,
		w,
		h,
		8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	auto* b = writeBuffer;
	const auto sizeX = w;
	const auto sizeY = h;
	const auto depth = get_display_depth();

	if (depth <= 16)
	{
		auto* p = reinterpret_cast<unsigned short*>(pix);
		for (auto y = 0; y < sizeY; y++)
		{
			for (auto x = 0; x < sizeX; x++)
			{
				const auto v = p[x];
				*b++ = ((v & SYSTEM_RED_MASK) >> SYSTEM_RED_SHIFT) << 3; // R
				*b++ = ((v & SYSTEM_GREEN_MASK) >> SYSTEM_GREEN_SHIFT) << 2; // G
				*b++ = ((v & SYSTEM_BLUE_MASK) >> SYSTEM_BLUE_SHIFT) << 3; // B
			}
			p += surface->pitch / 2;
			png_write_row(png_ptr, writeBuffer);
			b = writeBuffer;
		}
	}
	else
	{
		auto* p = reinterpret_cast<unsigned int*>(pix);
		for (auto y = 0; y < sizeY; y++) {
			for (auto x = 0; x < sizeX; x++) {
				auto v = p[x];
				*b++ = ((v & SYSTEM_RED_MASK) >> SYSTEM_RED_SHIFT); // R
				*b++ = ((v & SYSTEM_GREEN_MASK) >> SYSTEM_GREEN_SHIFT); // G 
				*b++ = ((v & SYSTEM_BLUE_MASK) >> SYSTEM_BLUE_SHIFT); // B
			}
			p += surface->pitch / 4;
			png_write_row(png_ptr, writeBuffer);
			b = writeBuffer;
		}
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(f);
	return 1;
}

bool create_screenshot()
{
	if (current_screenshot != nullptr)
	{
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}

	if (amiga_surface != nullptr) {
		current_screenshot = SDL_CreateRGBSurfaceFrom(amiga_surface->pixels,
			AMIGA_WIDTH_MAX << currprefs.gfx_resolution,
			AMIGA_HEIGHT_MAX << currprefs.gfx_vresolution,
			amiga_surface->format->BitsPerPixel,
			amiga_surface->pitch,
			amiga_surface->format->Rmask,
			amiga_surface->format->Gmask,
			amiga_surface->format->Bmask,
			amiga_surface->format->Amask);
	}
	return current_screenshot != nullptr;
}

int save_thumb(const std::string& path)
{
	auto ret = 0;
	if (current_screenshot != nullptr)
	{
		ret = save_png(current_screenshot, path);
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}
	return ret;
}

void screenshot(int monid, int mode, int doprepare)
{
	std::string filename;

	if (!create_screenshot())
	{
		write_log(_T("Failed to create screenshot\n"));
		return;
	}

	if (strlen(currprefs.floppyslots[0].df) > 0)
		filename = extract_filename(currprefs.floppyslots[0].df);
	else if (currprefs.cdslots[0].inuse && strlen(currprefs.cdslots[0].name) > 0)
		filename = extract_filename(currprefs.cdslots[0].name);
	else
		filename = "default.uae";

	screenshot_filename = get_screenshot_path();
	screenshot_filename += filename;
	screenshot_filename = remove_file_extension(screenshot_filename);
	screenshot_filename += ".png";

	save_thumb(screenshot_filename);
}
