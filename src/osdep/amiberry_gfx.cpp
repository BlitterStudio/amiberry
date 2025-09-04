/*
* Amiberry Amiga Emulator
*
* SDL2 interface
* 
* Copyright 2025 Dimitris Panokostas
* 
*/

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cmath>
#include <iostream>

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "events.h"
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
#include "registry.h"

#ifdef AMIBERRY
static bool force_auto_crop = false;
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

SDL_Rect render_quad;
static int dx = 0, dy = 0;
SDL_Rect crop_rect;
const char* sdl_video_driver;
bool kmsdrm_detected = false;

static int display_width;
static int display_height;
Uint32 pixel_format = SDL_PIXELFORMAT_BGR888;

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
int window_led_drives, window_led_drives_end;
int window_led_hd, window_led_hd_end;
int window_led_joys, window_led_joys_end, window_led_joy_start;
int window_led_msg, window_led_msg_end, window_led_msg_start;

static int wasfs[2];
static const TCHAR* wasfsname[2] = { _T("FullScreenMode"), _T("FullScreenModeRTG") };

int vsync_activeheight, vsync_totalheight;
float vsync_vblank, vsync_hblank;
bool beamracer_debug;
bool gfx_hdr;

int reopen(struct AmigaMonitor*, int, bool);

//static SDL_mutex* screen_cs = nullptr;
//static bool screen_cs_allocated;

void gfx_lock()
{
	//SDL_LockMutex(screen_cs);
}
void gfx_unlock()
{
	//SDL_UnlockMutex(screen_cs);
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

bool MonitorFromPoint(const SDL_Point pt)
{
	SDL_Rect display_bounds;
	bool point_in_display = false;
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
		if (SDL_GetDisplayBounds(i, &display_bounds) == 0) {
			if (SDL_PointInRect(&pt, &display_bounds)) {
				point_in_display = true;
				break;
			}
		}
	}
	return point_in_display;
}

// Check if the requested Amiga resolution can be displayed with the current Screen mode as a direct multiple
// Based on this we make the decision to use Linear (smooth) or Nearest Neighbor (pixelated) scaling
static bool ar_is_exact(const SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}

// Set the scaling method based on the user's preferences
// Scaling methods available:
// -1: Auto select between Nearest Neighbor and Linear (default)
// 0: Nearest Neighbor
// 1: Linear
// 2: Integer Scaling (this uses Nearest Neighbor)
void set_scaling_option(const int monid, const uae_prefs* p, const int width, const int height)
{
	if (p->scaling_method == -1)
	{
		if (ar_is_exact(&sdl_mode, width, height))
		{
#ifdef USE_OPENGL
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
			SDL_RenderSetIntegerScale(AMonitors[monid].amiga_renderer, SDL_FALSE);
#endif
		}
		else
		{
#ifdef USE_OPENGL
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
			SDL_RenderSetIntegerScale(AMonitors[monid].amiga_renderer, SDL_FALSE);
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
		SDL_RenderSetIntegerScale(AMonitors[monid].amiga_renderer, SDL_FALSE);
#endif
	}
	else if (p->scaling_method == 1)
	{
#ifdef USE_OPENGL
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_RenderSetIntegerScale(AMonitors[monid].amiga_renderer, SDL_FALSE);
#endif
	}
	else if (p->scaling_method == 2)
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		SDL_RenderSetIntegerScale(AMonitors[monid].amiga_renderer, SDL_TRUE);
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

static bool SDL2_alloctexture(int monid, int w, int h)
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
			if (width == -w && height == -h && format == pixel_format)
			{
				set_scaling_option(monid, &currprefs, width, height);
				return true;
			}
		}
		return false;
	}

	if (amiga_texture != nullptr)
		SDL_DestroyTexture(amiga_texture);

	AmigaMonitor* mon = &AMonitors[0];
	amiga_texture = SDL_CreateTexture(mon->amiga_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, amiga_surface->w, amiga_surface->h);
	return amiga_texture != nullptr;
#endif
}

static void update_leds(const int monid)
{
	if (!amiga_surface)
		return;
	static uae_u32 rc[256], gc[256], bc[256], a[256];
	static int done;
	int osdx, osdy;

	if (!done) {
		for (int i = 0; i < 256; i++) {
#ifdef AMIBERRY
			// RGBA
			rc[i] = i << 0;
			gc[i] = i << 8;
			bc[i] = i << 16;
#else
			// BGRA
			rc[i] = i << 16;
			gc[i] = i << 8;
			bc[i] = i << 0;
#endif
			a[i] = i << 24;
		}
		done = 1;
	}

	statusline_getpos(monid, &osdx, &osdy, crop_rect.w, crop_rect.h);
	int m = statusline_get_multiplier(monid) / 100;
	for (int y = 0; y < TD_TOTAL_HEIGHT * m; y++) {
		uae_u8* buf = (uae_u8*)amiga_surface->pixels + (y + osdy) * amiga_surface->pitch;
		draw_status_line_single(monid, buf, y, crop_rect.w, rc, gc, bc, a);
	}
}

bool vkbd_allowed(const int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	return currprefs.vkbd_enabled && !mon->screen_is_picasso;
}

static bool SDL2_renderframe(const int monid, int mode, int immediate)
{
	const AmigaMonitor* mon = &AMonitors[monid];
	const amigadisplay* ad = &adisplays[monid];
	// RTG status line is handled in P96 code, this is for native modes only
	if ((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on)
	{
		update_leds(monid);
	}

	if (amiga_texture && amiga_surface)
	{
		SDL_RenderClear(mon->amiga_renderer);
		SDL_UpdateTexture(amiga_texture, nullptr, amiga_surface->pixels, amiga_surface->pitch);
		SDL_RenderCopyEx(mon->amiga_renderer, amiga_texture, &crop_rect, &render_quad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
		if (vkbd_allowed(monid))
		{
			vkbd_redraw();
		}
		return true;
	}
	return false;
}

static void SDL2_showframe(const int monid)
{
	const AmigaMonitor* mon = &AMonitors[monid];
	SDL_RenderPresent(mon->amiga_renderer);
}

static void SDL2_refresh(const int monid)
{
	SDL2_renderframe(monid, 1, 1);
	SDL2_showframe(monid);
}
#endif //AMIBERRY

int gfx_IsPicassoScreen(const struct AmigaMonitor* mon)
{
	return mon->screen_is_picasso ? 1 : 0;
}

static int isfullscreen_2(const struct uae_prefs* p)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	const auto idx = mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE;
	return p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLSCREEN ? 1 : p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLWINDOW ? -1 : 0;
}
int isfullscreen()
{
	return isfullscreen_2(&currprefs);
}

int gfx_GetWidth(const struct AmigaMonitor* mon)
{
	return mon->currentmode.current_width;
}

int gfx_GetHeight(const struct AmigaMonitor* mon)
{
	return mon->currentmode.current_height;
}

static bool doInit(AmigaMonitor*);

int default_freq = 60;

static struct MultiDisplay* getdisplay2(const struct uae_prefs* p, const int index)
{
	const struct AmigaMonitor* mon = &AMonitors[0];
	static int max;
	int display = index < 0 ? p->gfx_apmode[mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display - 1 : index;

	if (!max || (max < MAX_DISPLAYS && max > 0 && Displays[max].monitorname != nullptr)) {
		max = 0;
		while (max < MAX_DISPLAYS && Displays[max].monitorname)
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

struct MultiDisplay* getdisplay(const struct uae_prefs* p, const int monid)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	if (monid > 0 && mon->md)
		return mon->md;
	return getdisplay2(p, 0);
}

void desktop_coords(const int monid, int* dw, int* dh, int* ax, int* ay, int* aw, int* ah)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	const struct MultiDisplay* md = getdisplay(&currprefs, monid);

	*dw = md->rect.w;
	*dh = md->rect.h;
	*ax = mon->amigawin_rect.x;
	*ay = mon->amigawin_rect.y;
	*aw = mon->amigawin_rect.w;
	*ah = mon->amigawin_rect.h;
}

static int target_get_display2(const TCHAR* name, const int mode)
{
	int found, found2;

	found = -1;
	found2 = -1;
	for (int i = 0; Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->monitorid[0] == '\\')
			continue;
		if (mode == 2 && md->monitorid[0] != '\\')
			continue;
		if (!_tcscmp(md->monitorid, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;

	found = -1;
	for (int i = 0; Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->adapterid[0] == '\\')
			continue;
		if (mode == 2 && md->adapterid[0] != '\\')
			continue;
		if (!_tcscmp(md->adapterid, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				if (found2 < 0)
					found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;

	for (int i = 0; Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->adaptername[0] == '\\')
			continue;
		if (mode == 2 && md->adaptername[0] != '\\')
			continue;
		if (!_tcscmp(md->adaptername, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				if (found2 < 0)
					found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;

	for (int i = 0; Displays[i].monitorname; i++) {
		const struct MultiDisplay* md = &Displays[i];
		if (mode == 1 && md->monitorname[0] == '\\')
			continue;
		if (mode == 2 && md->monitorname[0] != '\\')
			continue;
		if (!_tcscmp(md->monitorname, name)) {
			if (found < 0) {
				found = i + 1;
			} else {
				if (found2 < 0)
					found2 = found;
				found = -1;
				break;
			}
		}
	}
	if (found >= 0)
		return found;
	if (mode == 3) {
		if (found2 >= 0)
			return found2;
	}

	return -1;
}

int target_get_display(const TCHAR* name)
{
	int disp;

	//write_log(_T("target_get_display '%s'\n"), name);
	disp = target_get_display2(name, 0);
	//write_log(_T("Scan 0: %d\n"), disp);
	if (disp >= 0)
		return disp;
	disp = target_get_display2(name, 1);
	//write_log(_T("Scan 1: %d\n"), disp);
	if (disp >= 0)
		return disp;
	disp = target_get_display2(name, 2);
	//write_log(_T("Scan 2: %d\n"), disp);
	if (disp >= 0)
		return disp;
	disp = target_get_display2(name, 3);
	//write_log(_T("Scan 3: %d\n"), disp);
	if (disp >= 0)
		return disp;
	return -1;
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
		float diff = static_cast<float>(read_processor_time() - wait_vblank_timestamp);
		if (diff < 0)
			return -1;
		int sl = static_cast<int>(diff * (vsync_activeheight + (vsync_totalheight - vsync_activeheight) / 10) * vsync_vblank / syncbase);
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
		const uae_s64 v = read_processor_time();
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

	if (SDL_GetDesktopDisplayMode(displayindex, &dm) != 0)
	{
		write_log("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
		return ret;
	}
	if (SDL_GetDisplayUsableBounds(displayindex, &usable_bounds) != 0)
	{
		write_log("SDL_GetDisplayUsableBounds failed: %s\n", SDL_GetError());
		return ret;
	}
	if (SDL_GetDisplayBounds(displayindex, &bounds) != 0)
	{
		write_log("SDL_GetDisplayBounds failed: %s\n", SDL_GetError());
		return ret;
	}

	if (activeheightp)
		*activeheightp = usable_bounds.h;
	if (totalheightp)
		*totalheightp = bounds.h;
	const auto vblank = static_cast<float>(dm.refresh_rate);
	const auto hblank = static_cast<float>(31000); // faking hblank, since SDL2 doesn't provide a way to get the real one
	if (vblankp)
		*vblankp = vblank;
	if (hblankp)
		*hblankp = hblank;

	ret = true;
	return ret;
}

static void display_vblank_thread(struct AmigaMonitor* mon)
{
	const struct amigadisplay* ad = &adisplays[mon->monitor_id];
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

extern void target_calibrate_spin();
static void display_param_init(struct AmigaMonitor* mon)
{
	const struct amigadisplay* ad = &adisplays[mon->monitor_id];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];

	vsync_activeheight = mon->currentmode.current_height;
	vsync_totalheight = vsync_activeheight * 1125 / 1080;
	vsync_vblank = 0;
	vsync_hblank = 0;
	get_display_vblank_params(0, &vsync_activeheight, &vsync_totalheight, &vsync_vblank, &vsync_hblank);
	if (vsync_vblank <= 0)
		vsync_vblank = static_cast<float>(mon->currentmode.freq);
	// GPU scaled mode?
	if (vsync_activeheight > mon->currentmode.current_height) {
		const float m = static_cast<float>(vsync_activeheight) / mon->currentmode.current_height;
		vsync_hblank = vsync_hblank / m + 0.5f;
		vsync_activeheight = mon->currentmode.current_height;
	}

	wait_vblank_display = getdisplay(&currprefs, mon->monitor_id);
	if (!wait_vblank_display || !wait_vblank_display->HasAdapterData) {
		write_log(_T("Selected display mode does not have adapter data!\n"));
	}
	scanlinecalibrating = true;
	target_calibrate_spin();
	scanlinecalibrating = false;
	display_vblank_thread(mon);
}

const TCHAR* target_get_display_name(const int num, const bool friendlyname)
{
	if (num <= 0)
		return nullptr;
	const struct MultiDisplay* md = getdisplay2(nullptr, 0);
	if (!md)
		return nullptr;
	if (friendlyname)
		return md->monitorname;
	return md->monitorid;
}

void centerdstrect(struct AmigaMonitor* mon, SDL_Rect* dr)
{
	if (!(mon->currentmode.flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)))
		OffsetRect(dr, mon->amigawin_rect.x, mon->amigawin_rect.y);
	if (mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		if (mon->scalepicasso && mon->screen_is_picasso)
			return;
		if (mon->currentmode.fullfill && (mon->currentmode.current_width > mon->currentmode.native_width || mon->currentmode.current_height > mon->currentmode.native_height))
			return;
		OffsetRect(dr, (mon->currentmode.native_width - mon->currentmode.current_width) / 2,
			(mon->currentmode.native_height - mon->currentmode.current_height) / 2);
	}
}

//static int picasso_offset_x, picasso_offset_y;
//static float picasso_offset_mx, picasso_offset_my;

void getgfxoffset(const int monid, float* dxp, float* dyp, float* mxp, float* myp)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	const struct amigadisplay* ad = &adisplays[monid];
	float dx = 0, dy = 0, mx = 1.0, my = 1.0;
#ifdef AMIBERRY
	if (currprefs.gfx_auto_crop)
	{
		dx -= static_cast<float>(crop_rect.x);
		dy -= static_cast<float>(crop_rect.y);
	}
#endif
	//if (ad->picasso_on) {
	//	dx = picasso_offset_x * picasso_offset_mx;
	//	dy = picasso_offset_y * picasso_offset_my;
	//	mx = picasso_offset_mx;
	//	my = picasso_offset_my;
	//}

	if (mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		while (!(mon->scalepicasso && mon->screen_is_picasso)) {
			if (mon->currentmode.fullfill && (mon->currentmode.current_width > mon->currentmode.native_width || mon->currentmode.current_height > mon->currentmode.native_height))
				break;
			dx += (mon->currentmode.native_width - mon->currentmode.current_width) / 2;
			dy += (mon->currentmode.native_height - mon->currentmode.current_height) / 2;
			break;
		}
	}

	*dxp = dx;
	*dyp = dy;
	*mxp = 1.0f / mx;
	*myp = 1.0f / my;
}

//static int rgbformat_bits (RGBFTYPE t)
//{
//	unsigned long f = 1 << t;
//	return ((f & RGBMASK_8BIT) != 0 ? 8
//		: (f & RGBMASK_15BIT) != 0 ? 15
//		: (f & RGBMASK_16BIT) != 0 ? 16
//		: (f & RGBMASK_24BIT) != 0 ? 24
//		: (f & RGBMASK_32BIT) != 0 ? 32
//		: 0);
//}

int getrefreshrate(const int monid, const int width, const int height)
{
	const struct amigadisplay* ad = &adisplays[monid];
	const struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	int freq = 0;

	if (ap->gfx_refreshrate <= 0)
		return 0;

	const struct MultiDisplay* md = getdisplay(&currprefs, monid);
	for (int i = 0; md->DisplayModes[i].inuse; i++) {
		const struct PicassoResolution* pr = &md->DisplayModes[i];
		if (pr->res.width == width && pr->res.height == height) {
			for (int j = 0; pr->refresh[j] > 0; j++) {
				if (pr->refresh[j] == ap->gfx_refreshrate)
					return ap->gfx_refreshrate;
				if (pr->refresh[j] > freq && pr->refresh[j] < ap->gfx_refreshrate)
					freq = pr->refresh[j];
			}
		}
	}
	write_log(_T("Refresh rate %d not supported, using %d\n"), ap->gfx_refreshrate, freq);
	return freq;
}

static void addmode(const struct MultiDisplay* md, const SDL_DisplayMode* dm, const int rawmode)
{
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

	// SDL2 reports 24-bit here, but we can ignore it and use 32-bit modes
#ifndef AMIBERRY
	if (d != 32) {
		return;
	}
#endif

	if (dm->refresh_rate) {
		freq = dm->refresh_rate;
		if (freq < 10)
			freq = 0;
	}
	//if (dm->dmFields & DM_DISPLAYFLAGS) {
	//	lace = (dm->dmDisplayFlags & DM_INTERLACED) != 0;
	//}

	i = 0;
	while (md->DisplayModes[i].inuse) {
		if (md->DisplayModes[i].res.width == w && md->DisplayModes[i].res.height == h) {
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
	while (md->DisplayModes[i].inuse)
		i++;
	if (i >= MAX_PICASSO_MODES - 1)
		return;
	md->DisplayModes[i].inuse = true;
	md->DisplayModes[i].rawmode = rawmode;
	md->DisplayModes[i].lace = lace;
	md->DisplayModes[i].res.width = w;
	md->DisplayModes[i].res.height = h;
	md->DisplayModes[i].refresh[0] = freq;
	md->DisplayModes[i].refreshtype[0] = (lace ? REFRESH_RATE_LACE : 0) | (rawmode ? REFRESH_RATE_RAW : 0);
	md->DisplayModes[i].refresh[1] = 0;
	_sntprintf(md->DisplayModes[i].name, sizeof md->DisplayModes[i].name, _T("%dx%d%s"),
		md->DisplayModes[i].res.width, md->DisplayModes[i].res.height,
		lace ? _T("i") : _T(""));
}

static int resolution_compare(const void* a, const void* b)
{
	const PicassoResolution* ma = (struct PicassoResolution*)a;
	const PicassoResolution* mb = (struct PicassoResolution*)b;
	if (ma->res.width < mb->res.width)
		return -1;
	if (ma->res.width > mb->res.width)
		return 1;
	if (ma->res.height < mb->res.height)
		return -1;
	if (ma->res.height > mb->res.height)
		return 1;
	return 0;
}

static void sortmodes(const struct MultiDisplay* md)
{
	int i = 0, idx = -1;
	unsigned int pw = -1, ph = -1;

	while (md->DisplayModes[i].inuse)
		i++;
	qsort(md->DisplayModes, i, sizeof(struct PicassoResolution), resolution_compare);
	for (i = 0; md->DisplayModes[i].inuse; i++)
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
	while (md->DisplayModes[i].inuse) {
		write_log(_T("%d: %s%s ("), i, md->DisplayModes[i].rawmode ? _T("!") : _T(""), md->DisplayModes[i].name);
		j = 0;
		while (md->DisplayModes[i].refresh[j] > 0) {
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

void reenumeratemonitors()
{
	for (int i = 0; i < MAX_DISPLAYS; i++) {
		struct MultiDisplay* md = &Displays[i];
		memcpy(&md->workrect, &md->rect, sizeof(SDL_Rect));
	}
	enumeratedisplays();
}

static bool enumeratedisplays2(bool selectall)
{
	struct MultiDisplay *md = Displays;
	const int num_displays = SDL_GetNumVideoDisplays();
	if (num_displays < 1) {
		write_log("No video displays found\n");
		return false;
	}

	for (int i = 0; i < num_displays; i++)
	{
		if (md - Displays >= MAX_DISPLAYS)
			break;
		const char *display_name = SDL_GetDisplayName(i);
		if (!display_name)
			continue;

		if (SDL_GetDisplayBounds(i, &md->rect) != 0)
			continue;
		SDL_GetDisplayBounds(i, &md->workrect);

		md->adaptername = my_strdup_trim(display_name);
		md->adapterid = my_strdup(display_name);
		md->adapterkey = my_strdup(display_name);
		md->monitorname = my_strdup_trim(display_name);
		md->monitorid = my_strdup(display_name);
		md->primary = i == 0; // Assuming the first display is the primary display

		int num_modes = SDL_GetNumDisplayModes(i);
		if (num_modes < 1)
			continue;

		md->DisplayModes = static_cast<PicassoResolution*>(malloc((num_modes + 1) * sizeof(PicassoResolution)));
		if (!md->DisplayModes)
			continue;

		for (int j = 0; j < num_modes; j++) {
			SDL_DisplayMode mode;
			if (SDL_GetDisplayMode(i, j, &mode) != 0)
				continue;

			md->DisplayModes[j].res.width = mode.w;
			md->DisplayModes[j].res.height = mode.h;
			md->DisplayModes[j].refresh[0] = mode.refresh_rate;
			md->DisplayModes[j].refresh[1] = 0;
		}

		md++;
	}

	if (md == Displays)
		return false;

	for (auto& display : Displays) {
		if (display.monitorname == nullptr) {
			break;
		}
		if (display.fullname == nullptr) {
			display.fullname = my_strdup(display.adapterid);
		}
	}

	return true;
}

void enumeratedisplays()
{
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

	const int w = desktop_dm.w;
	const int h = desktop_dm.h;
	const int wv = w;
	const int hv = h;
	const int b = SDL_BITSPERPIXEL(desktop_dm.format);

	deskhz = 0;

	md = Displays;
	while (md->monitorname) {
		md->DisplayModes = xmalloc(struct PicassoResolution, MAX_PICASSO_MODES);

		write_log(_T("%s '%s' [%s]\n"), md->adaptername, md->adapterid, md->adapterkey);
		write_log(_T("-: %s [%s]\n"), md->fullname, md->monitorid);
		for (int mode = 0; mode < 2; mode++)
		{
			SDL_DisplayMode dm;
			const int num_disp_modes = SDL_GetNumDisplayModes(md->monitor);
			for (idx = 0; idx < num_disp_modes; idx++)
			{
				if (SDL_GetDisplayMode(md->monitor, idx, &dm) != 0) {
					write_log("SDL_GetDisplayMode failed: %s\n", SDL_GetError());
					return;
				}
				int found = 0;
				int idx2 = 0;
				while (md->DisplayModes[idx2].inuse && !found)
				{
					struct PicassoResolution* pr = &md->DisplayModes[idx2];
					if (dm.w == w && dm.h == h && SDL_BITSPERPIXEL(dm.format) == b) {
						deskhz = std::max(dm.refresh_rate, deskhz);
					}
					if (pr->res.width == dm.w && pr->res.height == dm.h) {
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
		while (md->DisplayModes[i].inuse)
			i++;
		write_log(_T("%d display modes.\n"), i);
		md++;
	}
	write_log(_T("Desktop: W=%d H=%d B=%d HZ=%d. CXVS=%d CYVS=%d\n"), w, h, b, deskhz, wv, hv);
}

int gfx_adjust_screenmode(const MultiDisplay *md, int *pwidth, int *pheight)
{
	struct PicassoResolution* best;
	int pass, i = 0, index = 0;

	for (pass = 0; pass < 2; pass++) {
		struct PicassoResolution* dm;
		i = 0;
		index = 0;

		best = &md->DisplayModes[0];
		dm = &md->DisplayModes[1];

		while (dm->inuse) {

			/* do we already have supported resolution? */
			if (dm->res.width == *pwidth && dm->res.height == *pheight)
				return i;

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
			dm++;
			i++;
		}
		if (best->res.width == *pwidth && best->res.height == *pheight) {
			break;
		}
	}
	*pwidth = best->res.width;
	*pheight = best->res.height;

	return index;
}

bool render_screen(const int monid, const int mode, const bool immediate)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	const struct amigadisplay* ad = &adisplays[monid];
	int cnt;

	mon->render_ok = false;
	if (ad->picasso_on || monitor_off) {
		return mon->render_ok;
	}
	cnt = 0;
	while (mon->wait_render) {
		sleep_millis(1);
		cnt++;
		if (cnt > 500) {
			return mon->render_ok;
		}
	}
	//gfx_lock();
	mon->render_ok = SDL2_renderframe(monid, mode, immediate);
	//gfx_unlock();
	return mon->render_ok;
}

bool show_screen_maybe(const int monid, const bool show)
{
	const struct amigadisplay* ad = &adisplays[monid];
	const struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	if (!ap->gfx_vflip || ap->gfx_vsyncmode == 0 || ap->gfx_vsync <= 0) {
		if (show)
			show_screen(monid, 0);
		return false;
	}
	return false;
}

float target_adjust_vblank_hz(const int monid, float hz)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
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
		hz -= 0.5f;
	return hz;
}

void show_screen(const int monid, int mode)
{
	AmigaMonitor* mon = &AMonitors[monid];
	const amigadisplay* ad = &adisplays[monid];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];

	//gfx_lock();
	//if (mode == 2 || mode == 3 || mode == 4) {
	//	if ((mon->currentmode.flags & DM_D3D) && D3D_showframe_special && ap->gfx_strobo) {
	//		if (mode == 4) {
	//			// erase + render
	//			D3D_showframe_special(0, 2);
	//			D3D_showframe_special(0, 1);
	//		} else {
	//			// erase or render
	//			D3D_showframe_special(0, mode == 3 ? 2 : 1);
	//		}
	//	}
	//	gfx_unlock();
	//	return;
	//}
	if (mode >= 0 && !mon->render_ok) {
		//gfx_unlock();
		return;
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
	SDL2_showframe(monid);
#endif
	//gfx_unlock();
	mon->render_ok = false;
}

int lockscr(struct vidbuffer* vb, bool fullupdate, bool skip)
{
	const struct AmigaMonitor* mon = &AMonitors[vb->monitor_id];
	int ret = 0;

	if (!mon->amiga_window || !amiga_surface)
		return ret;

	//gfx_lock();
	if (vb->vram_buffer) {
		// Benchmarks have shown that Locking and Unlocking the Texture is slower than just calling UpdateTexture
		// Therefore, this is disabled in Amiberry.
		//
		//SDL_LockTexture(amiga_texture, nullptr, reinterpret_cast<void**>(&vb->bufmem), &vb->rowbytes);

		vb->bufmem = static_cast<uae_u8*>(amiga_surface->pixels);
		vb->rowbytes = amiga_surface->pitch;
		vb->width_allocated = amiga_surface->w;
		vb->height_allocated = amiga_surface->h;
		if (vb->bufmem) {
			ret = 1;
		}
	} else {
		ret = 1;
	}
	if (ret) {
		vb->locked = true;
	}
	//gfx_unlock();
	return ret;
}

void unlockscr(struct vidbuffer* vb, int y_start, int y_end)
{
	//gfx_lock();
	vb->locked = false;
	if (vb->vram_buffer) {
		vb->bufmem = nullptr;
		// Not using SDL2_UnlockTexture due to performance reasons, see lockscr for details
		//SDL_UnlockTexture(amiga_texture);
	}
	//gfx_unlock();
}

void flush_clear_screen(struct vidbuffer* vb)
{
	if (!vb)
		return;
	if (lockscr(vb, true, false)) {
		int y;
		for (y = 0; y < vb->height_allocated; y++) {
			memset(vb->bufmem + y * vb->rowbytes, 0, vb->width_allocated * vb->pixbytes);
		}
		unlockscr(vb, -1, -1);
	}
}

float filterrectmult(int v1, float v2, int dmode)
{
	const float v = v1 / v2;
	const int vv = static_cast<int>(v + 0.5f);
	if (v > 1.5f && vv * v2 <= v1 && vv * (v2 + vv - 1) >= v1) {
		return static_cast<float>(vv);
	}
	if (!dmode) {
		return v;
	}
	if (v > 0.2f && v < 0.3f) {
		return 0.25f;
	}
	if (v > 0.4f && v < 0.6f) {
		return 0.5f;
	}
	return static_cast<float>(static_cast<int>(v + 0.5f));
}

static uae_u8* gfx_lock_picasso2(int monid, bool fullupdate)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[monid];
	uae_u8* p;
	//SDL_LockTexture(amiga_texture, nullptr, reinterpret_cast<void**>(&p), &vidinfo->rowbytes);
	//SDL_QueryTexture(amiga_texture,
	//	nullptr, nullptr,
	//	&vidinfo->maxwidth, &vidinfo->maxheight);
	vidinfo->pixbytes = amiga_surface->format->BytesPerPixel;
	vidinfo->rowbytes = amiga_surface->pitch;
	vidinfo->maxwidth = amiga_surface->w;
	vidinfo->maxheight = amiga_surface->h;
	p = static_cast<uae_u8*>(amiga_surface->pixels);
	return p;
}
uae_u8* gfx_lock_picasso(const int monid, bool fullupdate)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	static uae_u8* p;
	if (mon->rtg_locked) {
		return p;
	}
	//gfx_lock();
	p = gfx_lock_picasso2(monid, fullupdate);
	if (!p) {
		//gfx_unlock();
	} else {
		mon->rtg_locked = true;
	}
	return p;
}

void gfx_unlock_picasso(const int monid, const bool dorender)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	//if (!mon->rtg_locked)
	//	gfx_lock();
	mon->rtg_locked = false;
	if (dorender) {
		if (mon->p96_double_buffer_needs_flushing) {
			//D3D_flushtexture(monid, mon->p96_double_buffer_first, mon->p96_double_buffer_last);
			mon->p96_double_buffer_needs_flushing = 0;
		}
	}
	//SDL_UnlockTexture(amiga_texture);
	if (dorender) {
		if (SDL2_renderframe(monid, true, false)) {
			//gfx_unlock();
			mon->render_ok = true;
			show_screen_maybe(monid, true);
		} else {
			//gfx_unlock();
		}
	} else {
		//gfx_unlock();
	}
}

static void close_hwnds(struct AmigaMonitor* mon)
{
	if (mon->screen_is_initialized)
		releasecapture(mon);
	mon->screen_is_initialized = 0;
	if (!mon->monitor_id) {
#ifdef AVIOUTPUT
		AVIOutput_Restart(true);
#endif
#ifdef RETROPLATFORM
		rp_set_hwnd(NULL);
#endif
	}
	if (mon->monitor_id > 0 && mon->amiga_window)
		setmouseactive(mon->monitor_id, 0);

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

	gfx_hdr = false;
}

static void updatemodes(struct AmigaMonitor* mon)
{
	Uint32 flags = 0;

	mon->currentmode.fullfill = 0;
	if (isfullscreen() > 0)
		flags |= SDL_WINDOW_FULLSCREEN;
	else if (isfullscreen() < 0)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	mon->currentmode.flags = flags;
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
	const struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];

	updatewinfsmode(mon->monitor_id, &currprefs);
#ifdef PICASSO96
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
	} else {
#endif
		mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
		mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
#ifdef PICASSO96
	}
#endif
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

static int open_windows(AmigaMonitor* mon, bool mousecapture, bool started)
{
	bool recapture = false;
	int ret;

	mon->screen_is_initialized = 0;

	if (mon->monitor_id && mouseactive)
		recapture = true;

	inputdevice_unacquire();
	reset_sound();
	if (mon->amiga_window == nullptr)
		wait_keyrelease();

	mon->in_sizemove = 0;

	updatewinfsmode(mon->monitor_id, &currprefs);

	int init_round = 0;
	ret = -2;
	do {
		if (ret < -1) {
			updatemodes(mon);
			update_gfxparams(mon);
		}
		ret = doInit(mon);
		init_round++;
		if (ret < -9) {
			return 0;
		}
	} while (ret < 0);

	if (!ret) {
		return ret;
	}

	bool startactive = (started && mouseactive) || (!started && !currprefs.start_uncaptured && !currprefs.start_minimized);
	bool startpaused = !started && ((currprefs.start_minimized && currprefs.minimized_pause) || (currprefs.start_uncaptured && currprefs.inactive_pause && isfullscreen() <= 0));
	bool startminimized = !started && currprefs.start_minimized && isfullscreen() <= 0;
	int input = 0;

	if ((mousecapture && startactive) || recapture)
		setmouseactive(mon->monitor_id, -1);

	int upd = 0;
	if (startactive) {
		setpriority(currprefs.active_capture_priority);
		upd = 2;
	} else if (startminimized) {
		setpriority(currprefs.minimized_priority);
		setminimized(mon->monitor_id);
		input = currprefs.inactive_input;
		upd = 1;
	} else {
		setpriority(currprefs.inactive_priority);
		input = currprefs.inactive_input;
		upd = 2;
	}
	if (upd > 1) {
		for (int i = 0; i < NUM_LEDS; i++)
			gui_flicker_led(i, -1, -1);
		gui_led(LED_POWER, gui_data.powerled, gui_data.powerled_brightness);
		gui_fps(0, 0, 0, 0, 0);
		if (gui_data.md >= 0)
			gui_led(LED_MD, 0, -1);
		for (int i = 0; i < 4; i++) {
			if (currprefs.floppyslots[i].dfxtype >= 0)
				gui_led(LED_DF0 + i, 0, -1);
		}
	}
	if (upd > 0) {
		inputdevice_acquire(TRUE);
		if (!isfocus())
			inputdevice_unacquire(input);
	}

	if (startpaused)
		setpaused(1);

	statusline_updated(mon->monitor_id);
	refreshtitle();

	return ret;
}

static void reopen_gfx(struct AmigaMonitor* mon)
{
	open_windows(mon, false, true);
	render_screen(mon->monitor_id, 1, true);
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

void gfx_DisplayChangeRequested(const int mode)
{
	display_change_requested = mode;
}

int check_prefs_changed_gfx()
{
	int c = 0;
	bool monitors[MAX_AMIGAMONITORS]{};

	if (!config_changed && !display_change_requested)
		return 0;

	c |= config_changed_flags;
	config_changed_flags = 0;

	//c |= currprefs.win32_statusbar != changed_prefs.win32_statusbar ? 512 : 0;

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
	c |= currprefs.genlock_offset_x != changed_prefs.genlock_offset_x ? (1 | 256) : 0;
	c |= currprefs.genlock_offset_y != changed_prefs.genlock_offset_y ? (1 | 256) : 0;
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
	c |= currprefs.rtgallowscaling != changed_prefs.rtgallowscaling ? (2 | 8 | 64) : 0;
	c |= currprefs.rtgscaleaspectratio != changed_prefs.rtgscaleaspectratio ? (8 | 64) : 0;
	c |= currprefs.rtgvblankrate != changed_prefs.rtgvblankrate ? 8 : 0;

#ifdef AMIBERRY
	// These are missing from WinUAE
	c |= currprefs.rtg_hardwareinterrupt != changed_prefs.rtg_hardwareinterrupt ? 32 : 0;
	c |= currprefs.rtg_hardwaresprite != changed_prefs.rtg_hardwaresprite ? 32 : 0;
	c |= currprefs.rtg_multithread != changed_prefs.rtg_multithread ? 32 : 0;
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
		currprefs.lightboost_strobo = changed_prefs.lightboost_strobo;

		if (currprefs.gfx_api != changed_prefs.gfx_api) {
			display_change_requested = 1;
		}

#ifdef RETROPLATFORM
		if (c & 128) {
			// hres/vres change
			rp_screenmode_changed();
		}
#endif

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
		currprefs.genlock_offset_x = changed_prefs.genlock_offset_x;
		currprefs.genlock_offset_y = changed_prefs.genlock_offset_y;
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
#endif
				}
			}
			if (c & 1024) {
				target_graphics_buffer_update(mon->monitor_id, true);
			}
			if (c & 512) {
				reopen_gfx(mon);
			}
			if ((c & 16) || ((c & 8) && keepfsmode)) {
				if (reopen(mon, c & 2, unacquired == false)) {
					c |= 2;
				} else {
					unacquired = true;
				}
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
		init_hz();
	}
	if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate) {
		currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
		init_hz();
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

		get_custom_limits(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		fixup_prefs_dimensions(&changed_prefs);

		return 1;
	}

	//currprefs.win32_norecyclebin = changed_prefs.win32_norecyclebin;
	currprefs.filesys_limit = changed_prefs.filesys_limit;
	currprefs.harddrive_read_only = changed_prefs.harddrive_read_only;

	//if (currprefs.win32_logfile != changed_prefs.win32_logfile) {
	//	currprefs.win32_logfile = changed_prefs.win32_logfile;
	//	if (currprefs.win32_logfile)
	//		logging_open (0, 1);
	//	else
	//		logging_cleanup ();
	//}

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
		//refreshtitle();
		inputdevice_acquire(TRUE);
#ifndef	_DEBUG
		setpriority(currprefs.active_capture_priority);
#endif
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
	AmigaMonitor* mon = &AMonitors[monid];
	/* init colors */

	red_bits = green_bits = blue_bits = 8;
	red_shift = 0; green_shift = 8; blue_shift = 16;

	alloc_colors64k(monid, red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, alpha_bits, alpha_shift, alpha, 0);
	notice_new_xcolors();
#ifdef AVIOUTPUT
	AVIOutput_RGBinfo(red_bits, green_bits, blue_bits, alpha_bits, red_shift, green_shift, blue_shift, alpha_shift);
#endif
	//Screenshot_RGBinfo (red_bits, green_bits, blue_bits, alpha_bits, red_shift, green_shift, blue_shift, alpha_shift);
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
	const struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
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

static void open_screen(struct AmigaMonitor* mon)
{
	close_windows(mon);
	open_windows(mon, true, true);
}

int reopen(struct AmigaMonitor* mon, int full, bool unacquire)
{
	const struct amigadisplay* ad = &adisplays[mon->monitor_id];
	int quick = 0;
	int idx = mon->screen_is_picasso ? 1 : 0;
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];

	updatewinfsmode(mon->monitor_id, &changed_prefs);

	if (changed_prefs.gfx_apmode[0].gfx_fullscreen != currprefs.gfx_apmode[0].gfx_fullscreen && !mon->screen_is_picasso)
		full = 1;
	if (changed_prefs.gfx_apmode[1].gfx_fullscreen != currprefs.gfx_apmode[1].gfx_fullscreen && mon->screen_is_picasso)
		full = 1;

	/* fullscreen to fullscreen? */
	if (isfullscreen() > 0 && currprefs.gfx_apmode[0].gfx_fullscreen == changed_prefs.gfx_apmode[0].gfx_fullscreen &&
		currprefs.gfx_apmode[1].gfx_fullscreen == changed_prefs.gfx_apmode[1].gfx_fullscreen && currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN) {
		quick = 1;
	}
	/* windowed to windowed */
	if (isfullscreen() <= 0 && currprefs.gfx_apmode[0].gfx_fullscreen == changed_prefs.gfx_apmode[0].gfx_fullscreen &&
		currprefs.gfx_apmode[1].gfx_fullscreen == changed_prefs.gfx_apmode[1].gfx_fullscreen) {
		quick = 1;
	}

	currprefs.gfx_monitor[mon->monitor_id].gfx_size_fs.width = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.width;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.width = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.width;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.height = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.height;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.x = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.x;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.y = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.y;

	currprefs.gfx_apmode[0].gfx_fullscreen = changed_prefs.gfx_apmode[0].gfx_fullscreen;
	currprefs.gfx_apmode[1].gfx_fullscreen = changed_prefs.gfx_apmode[1].gfx_fullscreen;
	currprefs.gfx_apmode[0].gfx_vsync = changed_prefs.gfx_apmode[0].gfx_vsync;
	currprefs.gfx_apmode[1].gfx_vsync = changed_prefs.gfx_apmode[1].gfx_vsync;
	currprefs.gfx_apmode[0].gfx_vsyncmode = changed_prefs.gfx_apmode[0].gfx_vsyncmode;
	currprefs.gfx_apmode[1].gfx_vsyncmode = changed_prefs.gfx_apmode[1].gfx_vsyncmode;
	currprefs.gfx_apmode[0].gfx_refreshrate = changed_prefs.gfx_apmode[0].gfx_refreshrate;

	currprefs.rtg_horiz_zoom_mult = changed_prefs.rtg_horiz_zoom_mult;
	currprefs.rtg_vert_zoom_mult = changed_prefs.rtg_vert_zoom_mult;

	set_config_changed();

	if (!quick)
		return 1;

	if (unacquire) {
		inputdevice_unacquire();
	}

	reopen_gfx(mon);

	return 0;
}

bool vsync_switchmode(const int monid, int hz)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	static struct PicassoResolution* oldmode;
	static int oldhz;
	int w = mon->currentmode.native_width;
	int h = mon->currentmode.native_height;
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
				for (int i = 0; md->DisplayModes[i].inuse && !found; i++) {
					struct PicassoResolution* r = &md->DisplayModes[i];
					if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt)) {
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

static int modeswitchneeded(const struct AmigaMonitor* mon, const struct winuae_currentmode* wc)
{
	struct vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];

	if (isfullscreen() > 0) {
		/* fullscreen to fullscreen */
		if (mon->screen_is_picasso) {
			if (state->Width < wc->current_width && state->Height < wc->current_height) {
				if (currprefs.gf[GF_RTG].gfx_filter_autoscale == 1 || (currprefs.gf[GF_RTG].gfx_filter_autoscale == 2 && currprefs.rtgallowscaling))
					return 0;
			}
			if (state->Width != wc->current_width ||
				state->Height != wc->current_height)
				return 1;
			if (state->Width == wc->current_width &&
				state->Height == wc->current_height) {
			}
			return 1;
		} else {
			if (mon->currentmode.current_width != wc->current_width ||
				mon->currentmode.current_height != wc->current_height)
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
		}
		return -1;
	}
	return 0;
}

void gfx_set_picasso_state(const int monid, const int on)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct winuae_currentmode wc;
	struct apmode* newmode, * oldmode;
	struct gfx_filterdata* newf, * oldf;
	int mode;

	if (mon->screen_is_picasso == on)
		return;
	mon->screen_is_picasso = on;
#ifdef RETROPLATFORM
	rp_rtg_switch();
#endif
	memcpy(&wc, &mon->currentmode, sizeof(wc));

	newmode = &currprefs.gfx_apmode[on ? 1 : 0];
	oldmode = &currprefs.gfx_apmode[on ? 0 : 1];

	newf = &currprefs.gf[on ? 1 : 0];
	oldf = &currprefs.gf[on ? 0 : 1];

	updatemodes(mon);
	update_gfxparams(mon);

	// if filter changes, need to reset
	mode = 0;
	if (newf->gfx_filter != oldf->gfx_filter)
		mode = -1;
	for (int i = 0; i <= 2 * MAX_FILTERSHADERS; i++) {
		if (_tcscmp(newf->gfx_filtershader[i], oldf->gfx_filtershader[i]))
			mode = -1;
		if (_tcscmp(newf->gfx_filtermask[i], oldf->gfx_filtermask[i]))
			mode = -1;
	}
	const bool differentmonitor = getdisplay(&currprefs, newmode->gfx_display) != getdisplay(&currprefs, oldmode->gfx_display);
	// if screen parameter changes, need to reopen window
	if (newmode->gfx_fullscreen != oldmode->gfx_fullscreen ||
		(newmode->gfx_fullscreen && (
			newmode->gfx_backbuffers != oldmode->gfx_backbuffers ||
			differentmonitor ||
			newmode->gfx_refreshrate != oldmode->gfx_refreshrate ||
			newmode->gfx_strobo != oldmode->gfx_strobo ||
			newmode->gfx_vflip != oldmode->gfx_vflip ||
			newmode->gfx_vsync != oldmode->gfx_vsync))) {
		mode = 1;
	}
	if (mode <= 0) {
		const int m = modeswitchneeded(mon, &wc);
		if (m > 0)
			mode = m;
		if (m < 0 && !mode)
			mode = m;
		if (!mode)
			return;
	}
	if (mode < 0) {
		open_windows(mon, true, true);
	} else {
		open_screen(mon); // reopen everything
	}

#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif
}

static void updatepicasso96(struct AmigaMonitor* mon)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	vidinfo->rowbytes = 0;
	vidinfo->pixbytes = 32 / 8;
	vidinfo->rgbformat = 0;
	vidinfo->extra_mem = 1;
	vidinfo->height = mon->currentmode.current_height;
	vidinfo->width = mon->currentmode.current_width;
	vidinfo->offset = 0;
	vidinfo->splitypos = -1;
}

void gfx_set_picasso_modeinfo(const int monid, const RGBFTYPE rgbfmt)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];
	if (!mon->screen_is_picasso)
		return;
	gfx_set_picasso_colors(monid, rgbfmt);
	const int need = modeswitchneeded(mon, &mon->currentmode);
	update_gfxparams(mon);
	updatemodes(mon);
	if (need > 0) {
		open_screen(mon);
	} else if (need < 0) {
		open_windows(mon, true, true);
	}
	state->ModeChanged = false;
#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif
	target_graphics_buffer_update(monid, false);
}

void gfx_set_picasso_colors(int monid, const RGBFTYPE rgbfmt)
{
	alloc_colors_picasso(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt, p96_rgbx16);
}

static void gfxmode_reset(int monid)
{
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
#ifdef LOGITECHLCD
	lcd_open();
#endif
	return 1;
}

void machdep_free()
{
#ifdef LOGITECHLCD
	lcd_close();
#endif
}

int graphics_init(bool mousecapture)
{
	gfxmode_reset(0);
	if (open_windows(&AMonitors[0], mousecapture, false)) {
		if (currprefs.monitoremu_mon > 0 && currprefs.monitoremu) {
			gfxmode_reset(currprefs.monitoremu_mon);
			open_windows(&AMonitors[currprefs.monitoremu_mon], mousecapture, false);
		}
		return true;
	}
	return false;
}

int graphics_setup()
{
	//if (!screen_cs_allocated) {
	//	screen_cs = SDL_CreateMutex();
	//	if (screen_cs == nullptr) {
	//		write_log(_T("Couldn't create screen_cs: %s\n"), SDL_GetError());
	//		return 0;
	//	}
	//	screen_cs_allocated = true;
	//}
#ifdef PICASSO96
	InitPicasso96(0);
#endif
	return 1;
}

void graphics_leave()
{
	for (int i = 0; i < MAX_AMIGAMONITORS; i++) 
	{
		close_windows(&AMonitors[i]);
	}

	//SDL_DestroyMutex(screen_cs);
	//screen_cs = nullptr;
	//screen_cs_allocated = false;
}

void close_windows(struct AmigaMonitor* mon)
{
	vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;

	reset_sound();
#if 0
	S2X_free(mon->monitor_id);
#endif
#ifdef AMIBERRY
	SDL_FreeSurface(amiga_surface);
	amiga_surface = nullptr;
#endif
	freevidbuffer(mon->monitor_id, &avidinfo->drawbuffer);
	freevidbuffer(mon->monitor_id, &avidinfo->tempbuffer);
	close_hwnds(mon);
}

static int getbestmode(struct AmigaMonitor* mon, int nextbest)
{
	int i, startidx;
	struct MultiDisplay* md;
	int ratio;
	int index = -1;

	for (;;) {
		md = getdisplay2(&currprefs, index);
		if (!md)
			return 0;
		ratio = mon->currentmode.native_width > mon->currentmode.native_height ? 1 : 0;
		for (i = 0; md->DisplayModes[i].inuse; i++) {
			struct PicassoResolution* pr = &md->DisplayModes[i];
			if (pr->res.width == mon->currentmode.native_width && pr->res.height == mon->currentmode.native_height)
				break;
		}
		if (md->DisplayModes[i].inuse) {
			if (!nextbest)
				break;
			while (md->DisplayModes[i].res.width == mon->currentmode.native_width && md->DisplayModes[i].res.height == mon->currentmode.native_height)
				i++;
		} else {
			i = 0;
		}
		// first iterate only modes that have similar aspect ratio
		startidx = i;
		for (; md->DisplayModes[i].inuse; i++) {
			struct PicassoResolution* pr = &md->DisplayModes[i];
			int r = pr->res.width > pr->res.height ? 1 : 0;
			if (pr->res.width >= mon->currentmode.native_width && pr->res.height >= mon->currentmode.native_height && r == ratio) {
				write_log(_T("FS: %dx%d -> %dx%d %d %d\n"), mon->currentmode.native_width, mon->currentmode.native_height,
					pr->res.width, pr->res.height, ratio, index);
				mon->currentmode.native_width = pr->res.width;
				mon->currentmode.native_height = pr->res.height;
				mon->currentmode.current_width = mon->currentmode.native_width;
				mon->currentmode.current_height = mon->currentmode.native_height;
				goto end;
			}
		}
		// still not match? check all modes
		i = startidx;
		for (; md->DisplayModes[i].inuse; i++) {
			struct PicassoResolution* pr = &md->DisplayModes[i];
			int r = pr->res.width > pr->res.height ? 1 : 0;
			if (pr->res.width >= mon->currentmode.native_width && pr->res.height >= mon->currentmode.native_height) {
				write_log(_T("FS: %dx%d -> %dx%d\n"), mon->currentmode.native_width, mon->currentmode.native_height,
					pr->res.width, pr->res.height);
				mon->currentmode.native_width = pr->res.width;
				mon->currentmode.native_height = pr->res.height;
				mon->currentmode.current_width = mon->currentmode.native_width;
				mon->currentmode.current_height = mon->currentmode.native_height;
				goto end;
			}
		}
		index++;
	}
end:
	if (index >= 0) {
		currprefs.gfx_apmode[mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display =
			changed_prefs.gfx_apmode[mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display = index;
		write_log(reinterpret_cast<const TCHAR*>(L"Can't find mode %dx%d ->\n"), mon->currentmode.native_width, mon->currentmode.native_height);
		write_log(reinterpret_cast<const TCHAR*>(L"Monitor switched to '%s'\n"), md->adaptername);
	}
	return 1;
}

float target_getcurrentvblankrate(const int monid)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	float vb;
	if (currprefs.gfx_variable_sync)
		return static_cast<float>(mon->currentmode.freq);
	if (get_display_vblank_params(0, nullptr, nullptr, &vb, nullptr)) {
		return vb;
	}

	return SDL2_getrefreshrate(0);
}

static void movecursor(const int x, const int y)
{
	write_log(_T("SetCursorPos %dx%d\n"), x, y);
	SDL_WarpMouseGlobal(x, y);
}

static void getextramonitorpos(const struct AmigaMonitor* mon, SDL_Rect* r)
{
	TCHAR buf[100];
	SDL_Rect r1, r2;
	int x, y;
	bool got = true;

	_sntprintf(buf, sizeof buf, _T("MainPosX_%d"), mon->monitor_id);
	if (!regqueryint(nullptr, buf, &x)) {
		got = false;
	}
	_sntprintf(buf, sizeof buf, _T("MainPosY_%d"), mon->monitor_id);
	if (!regqueryint(nullptr, buf, &y)) {
		got = false;
	}
	if (got) {
		SDL_Point pt;
		pt.x = x;
		pt.y = y;
		if (!MonitorFromPoint(pt)) {
			got = false;
		}
	}
	// find rightmost window edge
	int monid = MAX_AMIGAMONITORS - 1;
	int rightmon = -1;
	int rightedge = 0;
	HWND hwnd = NULL;
	while (monid >= 1) {
		monid--;
		hwnd = AMonitors[monid].amiga_window;
		if (!hwnd)
			continue;
		GetWindowRect(hwnd, &r1);
		if (r1.w > rightedge) {
			rightedge = r1.w;
			rightmon = monid;
		}
	}
	if (rightmon < 0 && !got)
		return;
	hwnd = AMonitors[rightmon].amiga_window;
	GetWindowRect(hwnd, &r1);
	r2 = r1;

	//getextendedframebounds(hwnd, &r2);
	const int width = r->w;
	const int height = r->h;

	if (got) {
		r->x = x;
		r->y = y;
	} else {
		r->x = r1.w - ((r2.x - r1.x) + (r1.w - r2.w));
		r->y = r1.y;
	}
	r->h = height;
	r->w = width;
}

static int create_windows(struct AmigaMonitor* mon)
{
	const Uint32 fullscreen = mon->currentmode.flags & SDL_WINDOW_FULLSCREEN;
	Uint32 fullwindow = mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
	Uint32 flags = 0;
	const int borderless = currprefs.borderless;
	int x, y, w, h;
	struct MultiDisplay* md;

#ifdef AMIBERRY
	// Detect KMSDRM driver
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
	// If KMSDRM is detected, force Full-Window mode
	if (kmsdrm_detected)
	{
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = GFX_FULLWINDOW;
		currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = GFX_FULLWINDOW;
		fullwindow = 1;
	}
#endif

	md = getdisplay(&currprefs, mon->monitor_id);
	if (mon->monitor_id && fullwindow) {
		struct MultiDisplay* md2 = nullptr;
		int idx = 0;
		for (;;) {
			md2 = getdisplay2(&currprefs, idx);
			if (md2 == md)
				break;
			if (!md2)
				break;
			idx++;
		}
		for (int i = 0; i <= mon->monitor_id; i++) {
			md2 = getdisplay2(&currprefs, idx);
			if (!md2)
				idx = 0;
			else
				idx++;
		}
		if (md2)
			md = md2;
	}
	mon->md = md;

	if (mon->amiga_window) {
		if (minimized) {
			minimized = -1;
			return 1;
		}

		updatewinrect(mon, false);
		GetWindowRect(mon->amiga_window, &mon->amigawin_rect);
		write_log(_T("window already open (%dx%d %dx%d)\n"),
			mon->amigawin_rect.x, mon->amigawin_rect.y, mon->amigawin_rect.w, mon->amigawin_rect.h);
		updatemouseclip(mon);
#ifdef RETROPLATFORM
		rp_screenmode_changed();
#endif
		mon->window_extra_height_bar = 0;
		return 1;
	}

	window_led_drives = 0;
	window_led_drives_end = 0;
	x = 0; y = 0;

	SDL_Rect rc;
	int stored_x = 1, stored_y = 0;
	int oldx, oldy;

	regqueryint(nullptr, _T("MainPosX"), &stored_x);
	regqueryint(nullptr, _T("MainPosY"), &stored_y);

	if (borderless) {
		stored_x = currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.x;
		stored_y = currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.y;
	}

	stored_x = std::max(stored_x, 0);
	stored_y = std::max(stored_y, 0);

	SDL_Rect displayBounds;
	SDL_GetDisplayBounds(0, &displayBounds);

	if (stored_x > displayBounds.w)
		rc.x = 1;
	else
		rc.x = stored_x;

	if (stored_y > displayBounds.h)
		rc.y = 1;
	else
		rc.y = stored_y;

	rc.w = mon->currentmode.current_width;
	rc.h = mon->currentmode.current_height;

	oldx = rc.x;
	oldy = rc.y;

	mon->win_x_diff = rc.x - oldx;
	mon->win_y_diff = rc.y - oldy;

	if (mon->monitor_id > 0) {
		getextramonitorpos(mon, &rc);
	}

	if (fullwindow) {
		rc = md->rect;
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI;
		mon->currentmode.native_width = rc.w;
		mon->currentmode.native_height = rc.h;
	}
	else if (fullscreen)
	{
		flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI;
		getbestmode(mon, 0);
		w = mon->currentmode.native_width;
		h = mon->currentmode.native_height;
		rc = md->rect;
		if (rc.x >= 0)
			x = rc.x;
		else
			x = rc.x + (rc.w - w);
		if (rc.y >= 0)
			y = rc.y;
		else
			y = rc.y + (rc.h - h);
	} else {
		flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	}
	if (currprefs.main_alwaysontop) {
		flags |= SDL_WINDOW_ALWAYS_ON_TOP;
	}
	if (borderless)
		flags |= SDL_WINDOW_BORDERLESS;
	if (currprefs.start_minimized || currprefs.headless)
		flags |= SDL_WINDOW_HIDDEN;

	mon->amiga_window = SDL_CreateWindow(_T("Amiberry"),
		rc.x, rc.y,
		rc.w, rc.h,
		flags);
	if (!mon->amiga_window) {
		write_log(_T("main window creation failed\n"));
		return 0;
	}

	SDL_Rect rc2;
	GetWindowRect(mon->amiga_window, &rc2);
	mon->window_extra_width = rc2.w - mon->currentmode.current_width;
	mon->window_extra_height = rc2.h - mon->currentmode.current_height;

	w = mon->currentmode.native_width;
	h = mon->currentmode.native_height;

	if (!mon->amiga_window) {
		write_log(_T("creation of amiga window failed\n"));
		close_hwnds(mon);
		return 0;
	}

	auto* const icon_surface = IMG_Load(prefix_with_data_path("amiberry.png").c_str());
	if (icon_surface != nullptr)
	{
		SDL_SetWindowIcon(mon->amiga_window, icon_surface);
		SDL_FreeSurface(icon_surface);
	}

	if (mon->amiga_renderer == nullptr)
	{
		Uint32 renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
		mon->amiga_renderer = SDL_CreateRenderer(mon->amiga_window, -1, renderer_flags);
		check_error_sdl(mon->amiga_renderer == nullptr, "Unable to create a renderer:");
	}
	DPIHandler::set_render_scale(mon->amiga_renderer);

	updatewinrect(mon, true);
	GetWindowRect(mon->amiga_window, &mon->mainwin_rect);
	if (fullscreen || fullwindow)
		movecursor(x + w / 2, y + h / 2);

	mon->window_extra_height_bar = 0;
	//mon->dpi = getdpiforwindow(mon->monitor_id);

	if (SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1") != SDL_TRUE)
		write_log("SDL2: could not grab the keyboard!\n");

	if (SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0") == SDL_TRUE)
		write_log("SDL2: Set window not to minimize on focus loss\n");
	return 1;
}

static void allocsoftbuffer(int monid, const TCHAR* name, struct vidbuffer* buf, int flags, int width, int height)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	int depth = 32;

	if (buf->initialized && buf->vram_buffer) {
		return;
	}

	buf->monitor_id = monid;
	buf->pixbytes = (depth + 7) / 8;
	buf->width_allocated = (width + 7) & ~7;
	buf->height_allocated = height;
	buf->initialized = true;
	buf->hardwiredpositioning = false;

	if (buf == &vidinfo->drawbuffer) {

		buf->bufmem = NULL;
		buf->bufmemend = NULL;
		buf->realbufmem = NULL;
		buf->bufmem_allocated = NULL;
		buf->vram_buffer = true;

	} else {

		xfree(buf->realbufmem);
		const int w = buf->width_allocated;
		const int h = buf->height_allocated;
		const int size = (w * 2) * (h * 2) * buf->pixbytes;
		buf->rowbytes = w * 2 * buf->pixbytes;
		buf->realbufmem = xcalloc(uae_u8, size);
		buf->bufmem_allocated = buf->bufmem = buf->realbufmem + (h / 2) * buf->rowbytes + (w / 2) * buf->pixbytes;
		buf->bufmemend = buf->realbufmem + size - buf->rowbytes;

	}
}

static int oldtex_w[MAX_AMIGAMONITORS], oldtex_h[MAX_AMIGAMONITORS], oldtex_rtg[MAX_AMIGAMONITORS];

static bool doInit(AmigaMonitor* mon)
{
	int ret = 0;
	bool modechanged;

	struct vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;
	struct amigadisplay* ad = &adisplays[mon->monitor_id];
	avidinfo->gfx_resolution_reserved = RES_MAX;
	avidinfo->gfx_vresolution_reserved = VRES_MAX;

	modechanged = true;
	if (wasfs[0] == 0)
		regqueryint(NULL, wasfsname[0], &wasfs[0]);
	if (wasfs[1] == 0)
		regqueryint(NULL, wasfsname[1], &wasfs[1]);

	gfxmode_reset(mon->monitor_id);

	for (;;) {
		updatemodes(mon);

		if (mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
			const SDL_Rect rc = getdisplay(&currprefs, mon->monitor_id)->rect;
			mon->currentmode.native_width = rc.w;
			mon->currentmode.native_height = rc.h;
		}
		if (!create_windows(mon))
		{
			close_hwnds(mon);
			return ret;
		}
#ifdef PICASSO96
		if (mon->screen_is_picasso) {
			display_width = picasso96_state[0].Width ? picasso96_state[0].Width : 640;
			display_height = picasso96_state[0].Height ? picasso96_state[0].Height : 480;
			break;
		} else {
#endif
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

			display_width = mon->currentmode.amiga_width;
			display_height = mon->currentmode.amiga_height;
			break;
#ifdef PICASSO96
		}
#endif
	}

	updatepicasso96(mon);

	if (!mon->screen_is_picasso) {

		allocsoftbuffer(mon->monitor_id, _T("draw"), &avidinfo->drawbuffer, mon->currentmode.flags,
			1920, 1280);

		allocsoftbuffer(mon->monitor_id, _T("monemu"), &avidinfo->tempbuffer, mon->currentmode.flags,
			mon->currentmode.amiga_width > 2048 ? mon->currentmode.amiga_width : 2048,
			mon->currentmode.amiga_height > 2048 ? mon->currentmode.amiga_height : 2048);
	}

	avidinfo->outbuffer = &avidinfo->drawbuffer;
	avidinfo->inbuffer = &avidinfo->tempbuffer;

	if (amiga_surface)
	{
		SDL_FreeSurface(amiga_surface);
		amiga_surface = nullptr;
	}
	amiga_surface = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, 32, pixel_format);

	updatewinrect(mon, true);
	mon->screen_is_initialized = 1;

	if (modechanged) {
		init_colors(mon->monitor_id);
		display_param_init(mon);
		//createstatusline(mon->hAmigaWnd, mon->monitor_id);
	}
	target_graphics_buffer_update(mon->monitor_id, false);

	picasso_refresh(mon->monitor_id);
#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif

	if (isfullscreen() != 0) {
		setmouseactive(mon->monitor_id, -1);
	}

	//osk_setup(mon->monitor_id, -2);
	if (vkbd_allowed(mon->monitor_id))
	{
		vkbd_set_transparency(static_cast<double>(currprefs.vkbd_transparency) / 100.0);
		vkbd_set_hires(currprefs.vkbd_hires);
		vkbd_set_keyboard_has_exit_button(currprefs.vkbd_exit);
		vkbd_set_language(string(currprefs.vkbd_language));
		vkbd_set_style(string(currprefs.vkbd_style));
		vkbd_init();
	}

	return true;
}

bool target_graphics_buffer_update(const int monid, const bool force)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	struct vidbuf_description* avidinfo = &adisplays[monid].gfxvidinfo;
	const struct picasso96_state_struct* state = &picasso96_state[monid];
	struct vidbuffer *vb = NULL, *vbout = NULL;

	//gfx_lock();

	int w, h;

	if (mon->screen_is_picasso) {
		w = state->Width;
		h = state->Height;
	} else {
		vb = avidinfo->inbuffer;
		vbout = avidinfo->outbuffer;
		if (!vb) {
			//gfx_unlock();
			return false;
		}
		w = vb->outwidth;
		h = vb->outheight;
	}

	if (!force && oldtex_w[monid] == w && oldtex_h[monid] == h && oldtex_rtg[monid] == mon->screen_is_picasso) {
		if (SDL2_alloctexture(mon->monitor_id, -w, -h))
		{
			//osk_setup(monid, -2);
			if (vbout) {
				vbout->width_allocated = w;
				vbout->height_allocated = h;
			}
			//gfx_unlock();
			return false;
		}
	}

	if (!w || !h) {
		oldtex_w[monid] = w;
		oldtex_h[monid] = h;
		oldtex_rtg[monid] = mon->screen_is_picasso;
		//gfx_unlock();
		return false;
	}

	if (!SDL2_alloctexture(mon->monitor_id, w, h)) {
		//gfx_unlock();
		return false;
	}

	if (vbout) {
		vbout->width_allocated = w;
		vbout->height_allocated = h;
	}

	if (avidinfo->inbuffer != avidinfo->outbuffer) {
		avidinfo->outbuffer->inwidth = w;
		avidinfo->outbuffer->inheight = h;
		avidinfo->outbuffer->width_allocated = w;
		avidinfo->outbuffer->height_allocated = h;
	}

	oldtex_w[monid] = w;
	oldtex_h[monid] = h;
	oldtex_rtg[monid] = mon->screen_is_picasso;
	//osk_setup(monid, -2);

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
				render_quad = {dx, dy, w, h};
				crop_rect = {dx, dy, w, h};
			}
			else
			{
				SDL_RenderSetLogicalSize(mon->amiga_renderer, h, w);
				render_quad = { -(w - h) / 2, (w - h) / 2, w, h };
				crop_rect = { -(w - h) / 2, (w - h) / 2, w, h };
			}
			set_scaling_option(monid, &currprefs, w, h);
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

		if (currprefs.ntscmode)
			scaled_height = scaled_height * 6 / 5;

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
					render_quad = {dx, dy, scaled_width, scaled_height};
					crop_rect = {dx, dy, w, h};
				}
				else if (currprefs.gfx_manual_crop)
				{
					render_quad = { dx, dy, scaled_width, scaled_height };
					crop_rect = { currprefs.gfx_horizontal_offset, currprefs.gfx_vertical_offset, currprefs.gfx_manual_crop_width, currprefs.gfx_manual_crop_height };
				}
			}
			else
			{
				SDL_RenderSetLogicalSize(mon->amiga_renderer, scaled_height, scaled_width);
				if (!currprefs.gfx_auto_crop && !currprefs.gfx_manual_crop) {
					render_quad = { -(scaled_width - scaled_height) / 2, (scaled_width - scaled_height) / 2, scaled_width, scaled_height };
					crop_rect = { -(w - h) / 2, (w - h) / 2, w, h };
				}
			}
			set_scaling_option(monid, &currprefs, scaled_width, scaled_height);
		}
		else
		{
			return false;
		}
#endif
	}

	//gfx_unlock();

	return true;
}

static void updatedisplayarea2(const int monid)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->screen_is_initialized)
		return;
	SDL2_refresh(monid);
}

void updatedisplayarea(const int monid)
{
	set_custom_limits(-1, -1, -1, -1, false);
	if (monid >= 0) {
		updatedisplayarea2(monid);
	} else {
		for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
			updatedisplayarea2(i);
		}
	}
}

void updatewinfsmode(const int monid, struct uae_prefs* p)
{
	struct MultiDisplay* md;
	struct amigadisplay* ad = &adisplays[monid];

	fixup_prefs_dimensions(p);
	int fs = isfullscreen_2(p);
	if (fs != 0) {
		p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_fs;
	} else {
		p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_win;
	}

	int* wfw = &wasfs[ad->picasso_on ? 1 : 0];
	const TCHAR* wfwname = wasfsname[ad->picasso_on ? 1 : 0];
	if (fs != *wfw && fs != 0) {
		*wfw = fs;
		regsetint(NULL, wfwname, *wfw);
	}

	md = getdisplay(p, monid);
	set_config_changed();

#ifdef AMIBERRY
	const AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->screen_is_picasso)
		force_auto_crop = true;
#endif
}

bool toggle_3d_debug(void)
{
	if (isvsync_chipset() < 0) {
		beamracer_debug = !beamracer_debug;
		//if (D3D_debug) {
		//	D3D_debug(0, beamracer_debug);
		//}
		reset_drawing();
		return true;
	}
	return false;
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
	//return false;
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
	const amigadisplay* ad = &adisplays[monid];
	auto* p = ad->picasso_on ? &changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen : &changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen;
	int* wfw = &wasfs[ad->picasso_on ? 1 : 0];
	auto v = *p;
	//static int prevmode = -1;

	if (mode < 0) {
		// fullwindow->window->fullwindow.
		// fullscreen->window->fullscreen.
		// window->fullscreen->window.
		if (v == GFX_FULLWINDOW) {
			//prevmode = v;
			*wfw = -1;
			v = GFX_WINDOW;
		} else if (v == GFX_WINDOW) {
			if (*wfw >= 0) {
				v = GFX_FULLSCREEN;
			} else {
				v = GFX_FULLWINDOW;
			}
		} else if (v == GFX_FULLSCREEN) {
			//prevmode = v;
			*wfw = 1;
			v = GFX_WINDOW;
		}
	} else if (mode == 0) {
		//prevmode = v;
		// fullscreen <> window
		if (v == GFX_FULLSCREEN)
			v = GFX_WINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 1) {
		//prevmode = v;
		// fullscreen <> fullwindow
		if (v == GFX_FULLSCREEN)
			v = GFX_FULLWINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 2) {
		//prevmode = v;
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

#ifdef AMIBERRY
void auto_crop_image()
{
	const AmigaMonitor* mon = &AMonitors[0];
	static bool last_autocrop;

	if (currprefs.gfx_auto_crop)
	{
		static int last_cw = 0, last_ch = 0, last_cx = 0, last_cy = 0;
		int cw, ch, cx, cy, crealh = 0;
		int hres = currprefs.gfx_resolution;
		int vres = currprefs.gfx_vresolution;
		get_custom_limits(&cw, &ch, &cx, &cy, &crealh, &hres, &vres);

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

		if (currprefs.ntscmode)
			height = height * 6 / 5;

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
			render_quad = { dx, dy, width, height };
			crop_rect = { cx, cy, cw, ch };
		}
		else
		{
			SDL_RenderSetLogicalSize(mon->amiga_renderer, height, width);
			render_quad = { -(width - height) / 2, (width - height) / 2, width, height };
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
