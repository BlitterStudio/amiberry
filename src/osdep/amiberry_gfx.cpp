#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cmath>

#include "sysdeps.h"
#include "options.h"
#include "audio.h"
#include "uae.h"
#include "custom.h"
#include "events.h"
#include "traps.h"
#include "xwin.h"
#include "drawing.h"
#include "picasso96.h"
#include "amiberry_gfx.h"
#include "sounddep/sound.h"
#include "inputdevice.h"
#include "gui.h"
#include "serial.h"
#include "savestate.h"
#include "statusline.h"
#include "devices.h"
#include "gfxboard.h"
#include "threaddep/thread.h"

#include <png.h>
#include <SDL_image.h>

static uae_thread_id display_tid = nullptr;
static smp_comm_pipe *volatile display_pipe = nullptr;
static uae_sem_t display_sem = nullptr;
static bool volatile display_thread_busy = false;
#ifdef USE_DISPMANX
static unsigned int current_vsync_frame = 0;
unsigned long time_per_frame = 20000; // Default for PAL (50 Hz): 20000 microsecs
static int vsync_modulo = 1;
#endif
bool volatile flip_in_progess = false;

/* SDL Surface for output of emulation */
SDL_DisplayMode sdlMode;
SDL_Surface* sdl_surface = nullptr;
SDL_Texture* amiga_texture;
SDL_Rect renderQuad;
SDL_Renderer* sdl_renderer;
const char* sdl_video_driver;

#ifdef ANDROID
#include <android/log.h>
#endif

static int display_width;
static int display_height;
static int display_depth;
Uint32 pixel_format;

static unsigned long last_synctime;
static int deskhz;

#ifdef USE_DISPMANX
/* Possible screen modes (x and y resolutions) */
#define MAX_SCREEN_MODES 14
static int x_size_table[MAX_SCREEN_MODES] = { 640, 640, 720, 800, 800, 960, 1024, 1280, 1280, 1280, 1360, 1366, 1680, 1920 };
static int y_size_table[MAX_SCREEN_MODES] = { 400, 480, 400, 480, 600, 540, 768, 720, 800, 1024, 768, 768, 1050, 1080 };
#endif

// Check if the requested Amiga resolution can be displayed with the current Screen mode as a direct multiple
// Based on this we make the decision to use Linear (smooth) or Nearest Neighbor (pixelated) scaling
bool isModeAspectRatioExact(SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}

static bool SDL2_renderframe(int monid, int mode, bool immediate)
{
	SDL_RenderCopyEx(sdl_renderer, amiga_texture, nullptr, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
	return true;
}

static void SDL2_free()
{
	if (amiga_texture)
	{
		SDL_DestroyTexture(amiga_texture);
		amiga_texture = nullptr;
	}
}

static float SDL2_getrefreshrate(int monid)
{
	SDL_DisplayMode mode;
	if (SDL_GetDisplayMode(monid, 0, &mode) != 0)
	{
		write_log("SDL_GetDisplayMode failed: %s\n", SDL_GetError());
		return 0;
	}
	return static_cast<float>(mode.refresh_rate);
}

static int SDL2_GetCurrentDepth()
{
	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
		SDL_Log("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
		return 1;
	}
	return SDL_BITSPERPIXEL(dm.format);
}

static int SDL2_init(SDL_Window* ahwnd, int monid, int width, int height, int depth, int* freq, int mmulth, int mmultv)
{
	struct amigadisplay* ad = &adisplays[monid];
	struct apmode* apm = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	SDL_DisplayMode sdlMode;

	write_log(_T("SDL2 init start. (%d*%d) (%d*%d) RTG=%d Depth=%d.\n"), width, height, width, height, ad->picasso_on, depth);

	pixel_format = depth == 32 ? SDL_PIXELFORMAT_BGRA32 : SDL_PIXELFORMAT_RGB565;

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
	{
		SDL_RenderSetLogicalSize(sdl_renderer, width, height);
		renderQuad = { 0, 0, width, height };
	}
	else
	{
		SDL_RenderSetLogicalSize(sdl_renderer, height, width);
		renderQuad = { -(width - height) / 2, (width - height) / 2, width, height };
	}

	if (isfullscreen() == 0)
		SDL_SetWindowSize(ahwnd, width, height);

	SDL_GetCurrentDisplayMode(0, &sdlMode);
	if (currprefs.scaling_method == -1)
	{
		if (isModeAspectRatioExact(&sdlMode, width, height))
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		else
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	}
	else if (currprefs.scaling_method == 0)
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	else if (currprefs.scaling_method == 1)
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	return 0;
}

static bool SDL2_alloctexture(int monid, int w, int h)
{
	if (amiga_texture == nullptr)
		amiga_texture = SDL_CreateTexture(sdl_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (amiga_texture == nullptr)
	{
		write_log("SDL_CreateTexture failed!");
		return false;
	}
	return true;
}

static void SDL2_refresh(int monid)
{
	SDL2_renderframe(monid, true, true);
	SDL_RenderPresent(sdl_renderer);
}

void SDL2_guimode(int monid, int guion)
{
	struct AmigaMonitor* mon = &AMonitors[monid];

	if (isfullscreen() <= 0)
		return;

	if (guion > 0)
	{
		SDL2_free();
		SDL_HideWindow(mon->sdl_window);
	}
}

void SDL2_getpixelformat(int depth, int* rb, int* gb, int* bb, int* rs, int* gs, int* bs, int* ab, int* as, int* a)
{
	switch (depth)
	{
	case 32:
		*rb = 8;
		*gb = 8;
		*bb = 8;
		*ab = 8;
		*rs = 16;
		*gs = 8;
		*bs = 0;
		*as = 24;
		*a = 0;
		break;
	case 15:
		*rb = 5;
		*gb = 5;
		*bb = 5;
		*ab = 1;
		*rs = 10;
		*gs = 5;
		*bs = 0;
		*as = 15;
		*a = 0;
		break;
	case 16:
		*rb = 5;
		*gb = 6;
		*bb = 5;
		*ab = 0;
		*rs = 11;
		*gs = 5;
		*bs = 0;
		*as = 0;
		*a = 0;
		break;
	}
}

struct PicassoResolution* DisplayModes;
struct MultiDisplay Displays[MAX_DISPLAYS];

struct AmigaMonitor AMonitors[MAX_AMIGAMONITORS];

static int display_change_requested;
static int wasfullwindow_a, wasfullwindow_p;

int vsync_vblank, vsync_hblank;

static SDL_Surface* current_screenshot = nullptr;
static char screenshot_filename_default[MAX_DPATH] =
{
	'/', 't', 'm', 'p', '/', 'n', 'u', 'l', 'l', '.', 'p', 'n', 'g', '\0'
};
char* screenshot_filename = static_cast<char *>(&screenshot_filename_default[0]);
FILE* screenshot_file = nullptr;
static void create_screenshot();
static int save_thumb(char* path);
int delay_savestate_frame = 0;
static volatile bool vsync_active;

#ifdef USE_DISPMANX
static unsigned long next_synctime = 0;

DISPMANX_DISPLAY_HANDLE_T   displayHandle;
DISPMANX_MODEINFO_T         modeInfo;
DISPMANX_RESOURCE_HANDLE_T  amigafb_resource_1 = 0;
DISPMANX_RESOURCE_HANDLE_T  amigafb_resource_2 = 0;
DISPMANX_RESOURCE_HANDLE_T  blackfb_resource = 0;
DISPMANX_ELEMENT_HANDLE_T   elementHandle;
DISPMANX_ELEMENT_HANDLE_T	blackscreen_element;
DISPMANX_UPDATE_HANDLE_T    updateHandle;
VC_RECT_T       src_rect;
VC_RECT_T       dst_rect;
VC_RECT_T       blit_rect;
VC_RECT_T		black_rect;

VC_IMAGE_TYPE_T rgb_mode = VC_IMAGE_RGB565;

static int DispManXElementpresent = 0;
static unsigned char current_resource_amigafb = 0;

static volatile uae_atomic vsync_counter = 0;
void vsync_callback(unsigned int a, void* b)
{
	atomic_inc(&vsync_counter);
}
#endif

static int display_thread(void* unused)
{
	struct AmigaMonitor* mon = &AMonitors[0];
#ifdef USE_DISPMANX
	VC_DISPMANX_ALPHA_T alpha = {
		DISPMANX_FLAGS_ALPHA_T(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
		255, 0
	};
	uint32_t vc_image_ptr;
	SDL_Rect viewport;
#endif
	for (;;) {
		display_thread_busy = false;
		auto signal = read_comm_pipe_u32_blocking(display_pipe);
		display_thread_busy = true;

		switch (signal) {
		case DISPLAY_SIGNAL_SETUP:
#ifdef USE_DISPMANX
			vc_dispmanx_vsync_callback(displayHandle, vsync_callback, nullptr);
#else

#endif
			break;

		case DISPLAY_SIGNAL_SUBSHUTDOWN:
#ifdef USE_DISPMANX
			if (DispManXElementpresent == 1)
			{
				DispManXElementpresent = 0;
				updateHandle = vc_dispmanx_update_start(0);
				vc_dispmanx_element_remove(updateHandle, elementHandle);
				elementHandle = 0;
				vc_dispmanx_update_submit_sync(updateHandle);
			}

			if (amigafb_resource_1) {
				vc_dispmanx_resource_delete(amigafb_resource_1);
				amigafb_resource_1 = 0;
			}
			if (amigafb_resource_2) {
				vc_dispmanx_resource_delete(amigafb_resource_2);
				amigafb_resource_2 = 0;
			}
			if (blackfb_resource)
			{
				vc_dispmanx_resource_delete(blackfb_resource);
				blackfb_resource = 0;
			}
#else

#endif
			uae_sem_post(&display_sem);
			break;

		case DISPLAY_SIGNAL_OPEN:
#ifdef USE_DISPMANX			
			if (mon->screen_is_picasso)
			{
				if (picasso96_state[0].RGBFormat == RGBFB_R5G6B5
					|| picasso96_state[0].RGBFormat == RGBFB_R5G6B5PC
					|| picasso96_state[0].RGBFormat == RGBFB_CLUT)
				{
					display_depth = 16;
					rgb_mode = VC_IMAGE_RGB565;
					pixel_format = SDL_PIXELFORMAT_RGB565;
				}
				else
				{
					display_depth = 32;
					rgb_mode = VC_IMAGE_ARGB8888;
					pixel_format = SDL_PIXELFORMAT_BGRA32;
				}
			}
			else
			{
				//display_depth = 16;
				//rgb_mode = VC_IMAGE_RGB565;
				display_depth = 32;
				rgb_mode = VC_IMAGE_ARGB8888;
				pixel_format = SDL_PIXELFORMAT_BGRA32;
			}

			if (!sdl_surface)
				sdl_surface = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, display_depth, pixel_format);

			displayHandle = vc_dispmanx_display_open(0);

			if (!amigafb_resource_1)
				amigafb_resource_1 = vc_dispmanx_resource_create(rgb_mode, display_width, display_height, &vc_image_ptr);
			if (!amigafb_resource_2)
				amigafb_resource_2 = vc_dispmanx_resource_create(rgb_mode, display_width, display_height, &vc_image_ptr);
			if (!blackfb_resource)
				blackfb_resource = vc_dispmanx_resource_create(rgb_mode, display_width, display_height, &vc_image_ptr);

			vc_dispmanx_rect_set(&blit_rect, 0, 0, display_width, display_height);
			vc_dispmanx_resource_write_data(amigafb_resource_1, rgb_mode, sdl_surface->pitch, sdl_surface->pixels, &blit_rect);
			vc_dispmanx_resource_write_data(blackfb_resource, rgb_mode, sdl_surface->pitch, sdl_surface->pixels, &blit_rect);
			vc_dispmanx_rect_set(&src_rect, 0, 0, display_width << 16, display_height << 16);

			// Use the full screen size for the black frame
			vc_dispmanx_rect_set(&black_rect, 0, 0, modeInfo.width, modeInfo.height);

			// Correct Aspect Ratio
			if (changed_prefs.gfx_correct_aspect == 0)
			{
				// Fullscreen.
				vc_dispmanx_rect_set(&dst_rect, 0, 0, modeInfo.width, modeInfo.height);
			}
			else
			{
				int width, height;
				if (mon->screen_is_picasso)
				{
					width = display_width;
					height = display_height;
				}
				else
				{
					width = display_width * 2 >> changed_prefs.gfx_resolution;
					height = display_height * 2 >> changed_prefs.gfx_vresolution;
				}

				const auto want_aspect = static_cast<float>(width) / static_cast<float>(height);
				const auto real_aspect = static_cast<float>(modeInfo.width) / static_cast<float>(modeInfo.height);

				if (want_aspect > real_aspect)
				{
					const auto scale = static_cast<float>(modeInfo.width) / static_cast<float>(width);
					viewport.x = 0;
					viewport.w = modeInfo.width;
					viewport.h = static_cast<int>(std::ceil(height * scale));
					viewport.y = (modeInfo.height - viewport.h) / 2;
				}
				else
				{
					const auto scale = static_cast<float>(modeInfo.height) / static_cast<float>(height);
					viewport.y = 0;
					viewport.h = modeInfo.height;
					viewport.w = static_cast<int>(std::ceil(width * scale));
					viewport.x = (modeInfo.width - viewport.w) / 2;
				}

				vc_dispmanx_rect_set(&dst_rect, viewport.x, viewport.y, viewport.w, viewport.h);
			}

			if (DispManXElementpresent == 0)
			{
				DispManXElementpresent = 1;
				updateHandle = vc_dispmanx_update_start(0);
				if (!blackscreen_element)
					blackscreen_element = vc_dispmanx_element_add(updateHandle, displayHandle, 0,
						&black_rect, blackfb_resource, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
						nullptr, DISPMANX_NO_ROTATE);
				if (!elementHandle)
					elementHandle = vc_dispmanx_element_add(updateHandle, displayHandle, 1,
						&dst_rect, amigafb_resource_1, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
						nullptr, DISPMANX_NO_ROTATE);

				vc_dispmanx_update_submit(updateHandle, nullptr, nullptr);
			}
#else

#endif

			uae_sem_post(&display_sem);
			break;

		case DISPLAY_SIGNAL_SHOW:
			vsync_active = true;
#ifdef USE_DISPMANX
			if (current_resource_amigafb == 1)
			{
				current_resource_amigafb = 0;
				vc_dispmanx_resource_write_data(amigafb_resource_1,
					rgb_mode,
					sdl_surface->pitch,
					sdl_surface->pixels,
					&blit_rect);
				updateHandle = vc_dispmanx_update_start(0);
				vc_dispmanx_element_change_source(updateHandle, elementHandle, amigafb_resource_1);
			}
			else
			{
				current_resource_amigafb = 1;
				vc_dispmanx_resource_write_data(amigafb_resource_2,
					rgb_mode,
					sdl_surface->pitch,
					sdl_surface->pixels,
					&blit_rect);
				updateHandle = vc_dispmanx_update_start(0);
				vc_dispmanx_element_change_source(updateHandle, elementHandle, amigafb_resource_2);
			}
			vc_dispmanx_update_submit(updateHandle, nullptr, nullptr);
#else
			SDL_RenderClear(sdl_renderer);
			SDL_UpdateTexture(amiga_texture, nullptr, sdl_surface->pixels, sdl_surface->pitch);
			SDL_RenderCopyEx(sdl_renderer, amiga_texture, nullptr, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
#endif
			flip_in_progess = false;
			break;

		case DISPLAY_SIGNAL_QUIT:
#ifdef USE_DISPMANX
			updateHandle = vc_dispmanx_update_start(0);
			vc_dispmanx_element_remove(updateHandle, blackscreen_element);
			blackscreen_element = 0;
			vc_dispmanx_update_submit_sync(updateHandle);

			vc_dispmanx_vsync_callback(displayHandle, nullptr, nullptr);
			vc_dispmanx_display_close(displayHandle);
#else

#endif
			display_tid = nullptr;
			return 0;
		default:
			break;
		}
	}
	return 0;
}

#ifdef USE_DISPMANX
void change_layer_number(int layer)
{
	updateHandle = vc_dispmanx_update_start(0);
	vc_dispmanx_element_change_layer(updateHandle, blackscreen_element, layer - 1);
	vc_dispmanx_element_change_layer(updateHandle, elementHandle, layer);
	vc_dispmanx_update_submit_sync(updateHandle);
}
#endif

int graphics_setup(void)
{
#ifdef PICASSO96
	//sortdisplays(); //we already run this earlier on startup
	InitPicasso96(0);
#endif

#ifdef USE_DISPMANX
	bcm_host_init();
	displayHandle = vc_dispmanx_display_open(0);
	vc_dispmanx_display_get_info(displayHandle, &modeInfo);

	VCHI_INSTANCE_T vchi_instance;
	VCHI_CONNECTION_T* vchi_connection;
	TV_DISPLAY_STATE_T tvstate;

	if (vchi_initialise(&vchi_instance) == 0) {
		if (vchi_connect(nullptr, 0, vchi_instance) == 0)
		{
			vc_vchi_tv_init(vchi_instance, &vchi_connection, 1);
			if (vc_tv_get_display_state(&tvstate) == 0)
			{
				HDMI_PROPERTY_PARAM_T property;
				property.property = HDMI_PROPERTY_PIXEL_CLOCK_TYPE;
				vc_tv_hdmi_get_property(&property);
				const auto frame_rate = property.param1 == HDMI_PIXEL_CLOCK_TYPE_NTSC ? static_cast<float>(tvstate.display.hdmi.frame_rate) * (1000.0f / 1001.0f) : static_cast<float>(tvstate.display.hdmi.frame_rate);
				vsync_vblank = static_cast<int>(frame_rate);
			}
			vc_vchi_tv_stop();
			vchi_disconnect(vchi_instance);
		}
	}
#endif
	return 1;
}

void updatedisplayarea(int monid)
{
	show_screen(monid, 0);
}

void update_win_fs_mode(int monid, struct uae_prefs* p)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	auto* avidinfo = &adisplays[0].gfxvidinfo;
#ifdef USE_DISPMANX
	// Dispmanx modes use configurable width/height and are fullwindow always
	p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_win;
#else
	if (mon->sdl_window)
	{
		const auto window_flags = SDL_GetWindowFlags(mon->sdl_window);
		const bool is_fullwindow = window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
		const bool is_fullscreen = window_flags & SDL_WINDOW_FULLSCREEN;

		if (p->gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN)
		{
			p->gfx_monitor[0].gfx_size = p->gfx_monitor[0].gfx_size_win;
			// Switch to Fullscreen mode, if we don't have it already
			if (!is_fullscreen)
			{
				SDL_SetWindowFullscreen(mon->sdl_window, SDL_WINDOW_FULLSCREEN);
				SDL_SetWindowSize(mon->sdl_window, p->gfx_monitor[0].gfx_size_fs.width, p->gfx_monitor[0].gfx_size_fs.height);
			}
		}
		else if (p->gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW)
		{
			p->gfx_monitor[0].gfx_size = p->gfx_monitor[0].gfx_size_win;
			if (!is_fullwindow)
				SDL_SetWindowFullscreen(mon->sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
			p->gfx_monitor[0].gfx_size = p->gfx_monitor[0].gfx_size_win;
			// Switch to Window mode, if we don't have it already - but not for KMSDRM
			if ((is_fullscreen || is_fullwindow) && strcmpi(sdl_video_driver, "KMSDRM") != 0)
				SDL_SetWindowFullscreen(mon->sdl_window, 0);
		}
		
		set_config_changed();
	}

#endif
	if (mon->screen_is_picasso)
	{
		display_width = picasso96_state[0].Width ? picasso96_state[0].Width : 640;
		display_height = picasso96_state[0].Height ? picasso96_state[0].Height : 480;
	}
	else
	{
		if (currprefs.gfx_resolution > avidinfo->gfx_resolution_reserved)
			avidinfo->gfx_resolution_reserved = currprefs.gfx_resolution;
		if (currprefs.gfx_vresolution > avidinfo->gfx_vresolution_reserved)
			avidinfo->gfx_vresolution_reserved = currprefs.gfx_vresolution;

		display_width = p->gfx_monitor[0].gfx_size.width / 2 << p->gfx_resolution;
		display_height = p->gfx_monitor[0].gfx_size.height / 2 << p->gfx_vresolution;
	}
}

void toggle_fullscreen(int monid, int mode)
{
#ifdef USE_DISPMANX
	// Dispmanx is full-window always
#else
	auto* const ad = &adisplays[monid];
	auto* p = ad->picasso_on ? &changed_prefs.gfx_apmode[1].gfx_fullscreen : &changed_prefs.gfx_apmode[0].gfx_fullscreen;
	const auto wfw = ad->picasso_on ? wasfullwindow_p : wasfullwindow_a;
	auto v = *p;

	if (mode < 0) {
		// fullscreen <> window (if in fullwindow: fullwindow <> fullscreen)
		if (v == GFX_FULLWINDOW)
			v = GFX_FULLSCREEN;
		else if (v == GFX_WINDOW)
			v = GFX_FULLSCREEN;
		else if (v == GFX_FULLSCREEN)
		{
			if (wfw > 0)
				v = GFX_FULLWINDOW;
			else
				v = GFX_WINDOW;
		}
	}
	else if (mode == 0) {
		// fullscreen <> window
		if (v == GFX_FULLSCREEN)
			v = GFX_WINDOW;
		else
			v = GFX_FULLSCREEN;
	}
	else if (mode == 1) {
		// fullscreen <> fullwindow
		if (v == GFX_FULLSCREEN)
			v = GFX_FULLWINDOW;
		else
			v = GFX_FULLSCREEN;
	}
	else if (mode == 2) {
		// window <> fullwindow
		if (v == GFX_FULLWINDOW)
			v = GFX_WINDOW;
		else
			v = GFX_FULLWINDOW;
	}
	else if (mode == 10) {
		v = GFX_WINDOW;
	}
	*p = v;
	devices_unsafeperiod();
	update_win_fs_mode(monid, &currprefs);
#endif
}

static int isfullscreen_2(struct uae_prefs* p)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	const auto idx = mon->screen_is_picasso ? 1 : 0;
	return p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLSCREEN ? 1 : p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLWINDOW ? -1 : 0;
}
int isfullscreen(void)
{
	return isfullscreen_2(&currprefs);
}

int default_freq = 60;

static struct MultiDisplay* getdisplay2(struct uae_prefs* p, int index)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	static int max;
	int display = index < 0 ? p->gfx_apmode[mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE].gfx_display - 1 : index;

	if (!max || (max > 0 && Displays[max].monitorname != NULL)) {
		max = 0;
		while (Displays[max].monitorname)
			max++;
		if (max == 0) {
			gui_message(_T("no display adapters! Exiting"));
			exit(0);
		}
	}
	if (index >= 0 && display >= max)
		return NULL;
	if (display >= max)
		display = 0;
	if (display < 0)
		display = 0;
	return &Displays[display];
}

struct MultiDisplay* getdisplay(struct uae_prefs* p, int monid)
{
	return getdisplay2(p, -1);
}

void desktop_coords(int monid, int* dw, int* dh, int* ax, int* ay, int* aw, int* ah)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct MultiDisplay* md = getdisplay(&currprefs, monid);

	*dw = md->rect.w - md->rect.x;
	*dh = md->rect.h - md->rect.y;
	*ax = 0;
	*ay = 0;
	*aw = sdl_surface->w - *ax;
	*ah = sdl_surface->h - *ay;
}

int target_get_display(const TCHAR* name)
{
	return 0;
}

int target_get_display_scanline(int displayindex)
{
	return -2;
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

static void wait_for_display_thread(void)
{
	while (display_thread_busy)
		usleep(10);
}

static void allocsoftbuffer(int monid, const TCHAR* name, struct vidbuffer* buf, int flags, int width, int height, int depth)
{
	/* Initialize structure for Amiga video modes */
	buf->monitor_id = monid;
	buf->pixbytes = sdl_surface->format->BytesPerPixel;
	buf->width_allocated = (width + 7) & ~7;
	buf->height_allocated = height;

	buf->outwidth = buf->width_allocated;
	buf->outheight = buf->height_allocated;
	buf->inwidth = buf->width_allocated;
	buf->inheight = buf->height_allocated;
	
	buf->rowbytes = sdl_surface->pitch;
	buf->realbufmem = static_cast<uae_u8*>(sdl_surface->pixels);
	buf->bufmem_allocated = buf->bufmem = buf->realbufmem;
	buf->bufmem_lockable = true;
	
	if (sdl_surface->format->BytesPerPixel == 2)
		currprefs.color_mode = changed_prefs.color_mode = 2;
	else
		currprefs.color_mode = changed_prefs.color_mode = 5;
}

void graphics_subshutdown()
{
	if (display_tid != nullptr) {
		wait_for_display_thread();
		write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SUBSHUTDOWN, 1);
		uae_sem_wait(&display_sem);
	}
	reset_sound();

	auto* avidinfo = &adisplays[0].gfxvidinfo;
	avidinfo->drawbuffer.realbufmem = NULL;
	avidinfo->drawbuffer.bufmem = NULL;
	avidinfo->drawbuffer.bufmem_allocated = NULL;
	avidinfo->drawbuffer.bufmem_lockable = false;

	avidinfo->outbuffer = &avidinfo->drawbuffer;
	avidinfo->inbuffer = &avidinfo->drawbuffer;
	
#ifndef USE_DISPMANX
	if (amiga_texture != nullptr)
	{
		SDL_DestroyTexture(amiga_texture);
		amiga_texture = nullptr;
	}
#endif
	
	if (sdl_surface)
	{
		SDL_FreeSurface(sdl_surface);
		sdl_surface = nullptr;
	}
}

static void updatepicasso96(struct AmigaMonitor* mon)
{
#ifdef PICASSO96
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	vidinfo->rowbytes = 0;
	vidinfo->pixbytes = sdl_surface->format->BytesPerPixel;
	vidinfo->rgbformat = 0;
	vidinfo->extra_mem = 1;
	vidinfo->height = sdl_surface->h;
	vidinfo->width = sdl_surface->w;
	vidinfo->depth = sdl_surface->format->BytesPerPixel * 8;
	vidinfo->offset = 0;
	vidinfo->splitypos = -1;
#endif
}

static void open_screen(struct uae_prefs* p)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	auto* avidinfo = &adisplays[0].gfxvidinfo;
	graphics_subshutdown();

	if (max_uae_width == 0 || max_uae_height == 0)
	{
		max_uae_width = 8192;
		max_uae_height = 8192;
	}
	
	if (wasfullwindow_a == 0)
		wasfullwindow_a = currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW ? 1 : -1;
	if (wasfullwindow_p == 0)
		wasfullwindow_p = currprefs.gfx_apmode[1].gfx_fullscreen == GFX_FULLWINDOW ? 1 : -1;
	
	update_win_fs_mode(0, p);
	
#ifdef USE_DISPMANX
	next_synctime = 0;
	current_resource_amigafb = 0;

	write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_OPEN, 1);
	uae_sem_wait(&display_sem);

	vsync_counter = 0;
	current_vsync_frame = 2;
#else
	
	int width, height;
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

		if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		{
			SDL_RenderSetLogicalSize(sdl_renderer, display_width, display_height);
			renderQuad = { 0, 0, display_width, display_height };
		}
		else
		{
			SDL_RenderSetLogicalSize(sdl_renderer, display_height, display_width);
			renderQuad = { -(display_width - display_height) / 2, (display_width - display_height) / 2, display_width, display_height };
		}
		
		if (isfullscreen() == 0)
			SDL_SetWindowSize(mon->sdl_window, display_width, display_height);
	}
	else
	{
		//display_depth = 16;
		//pixel_format = SDL_PIXELFORMAT_RGB565;
		display_depth = 32;
		pixel_format = SDL_PIXELFORMAT_BGRA32;

		if (changed_prefs.gfx_correct_aspect == 0)
		{
			width = sdlMode.w;
			height = sdlMode.h;
		}
		else
		{
			width = display_width * 2 >> p->gfx_resolution;
			height = display_height * 2 >> p->gfx_vresolution;
		}

		if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		{
			SDL_RenderSetLogicalSize(sdl_renderer, width, height);
			renderQuad = { 0, 0, width, height };
		}
		else
		{
			SDL_RenderSetLogicalSize(sdl_renderer, height, width);
			renderQuad = { -(width - height) / 2, (width - height) / 2, width, height };
		}
		
		if (isfullscreen() == 0)
			SDL_SetWindowSize(mon->sdl_window, width, height);
	}

	if (p->scaling_method == -1)
	{
		if (isModeAspectRatioExact(&sdlMode, display_width, display_height))
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		else
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	}
	else if (p->scaling_method == 0)
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	else if (p->scaling_method == 1)
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	
	sdl_surface = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, display_depth, pixel_format);
	check_error_sdl(sdl_surface == nullptr, "Unable to create a surface");

	amiga_texture = SDL_CreateTexture(sdl_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, sdl_surface->w, sdl_surface->h);
	check_error_sdl(amiga_texture == nullptr, "Unable to create texture");

#endif

	setpriority(currprefs.active_capture_priority);
	updatepicasso96(mon);

	if (sdl_surface != nullptr)
	{
		allocsoftbuffer(mon->monitor_id, _T("draw"), &avidinfo->drawbuffer, 0, display_width, display_height, display_depth);
		notice_screen_contents_lost(0);
		if (!mon->screen_is_picasso)
		{
			init_row_map();
		}
	}
	init_colors(mon->monitor_id);

	updatewinrect(mon, true);

	mon->screen_is_initialized = 1;
	
	picasso_refresh(mon->monitor_id);

	setmouseactive(mon->monitor_id, -1);
}

extern int vstrt; // vertical start
extern int vstop; // vertical stop
void flush_screen(struct vidbuffer* vidbuffer, int ystart, int ystop)
{
	if (vidbuffer->bufmem == nullptr) return; // no buffer allocated return

	static int last_autoheight;
	if (currprefs.gfx_auto_height)
	{
		static int last_vstrt, last_vstop, new_height;
		if (last_autoheight != currprefs.gfx_auto_height || last_vstrt != vstrt || last_vstop != vstop)
		{
			last_vstrt = vstrt;
			last_vstop = vstop;

			auto start_y = minfirstline;  // minfirstline = first line to be written to screen buffer
			auto stop_y = 274 + minfirstline; // last line to be written to screen buffer
			if (vstrt > minfirstline)
				start_y = vstrt;		// if vstrt > minfirstline then there is a black border
			if (start_y > 200)
				start_y = minfirstline; // shouldn't happen but does for donkey kong
			if (vstop < stop_y)
				stop_y = vstop;			// if vstop < stop_y then there is a black border

			new_height = stop_y - start_y;
			
			if (new_height < 200)
				new_height = 200;
			if (new_height * 2 != currprefs.gfx_monitor[0].gfx_size_win.height)
			{
				display_height = new_height * 2;
				currprefs.gfx_monitor[0].gfx_size_win.height = new_height * 2;
				copy_prefs(&currprefs, &changed_prefs);
				open_screen(&currprefs);
				init_custom();
			}
		}
	}

	last_autoheight = currprefs.gfx_auto_height;
}

void update_display(struct uae_prefs* p)
{
	open_screen(p);
}

void graphics_reset(bool forced)
{
	if (forced) {
		display_change_requested = 2;
	}
	else {
		// full reset if display size can't changed.
		if (currprefs.gfx_api) {
			display_change_requested = 3;
		}
		else {
			display_change_requested = 2;
		}
	}
}

int check_prefs_changed_gfx()
{
	int c = 0;
	bool monitors[MAX_AMIGAMONITORS];
	
	if (!config_changed && !display_change_requested)
		return 0;

	for (int i = 0; i < MAX_AMIGADISPLAYS; i++) {
		int c2 = 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_fs.width != changed_prefs.gfx_monitor[i].gfx_size_fs.width ? 16 : 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_fs.height != changed_prefs.gfx_monitor[i].gfx_size_fs.height ? 16 : 0;
		c2 |= ((currprefs.gfx_monitor[i].gfx_size_win.width + 7) & ~7) != ((changed_prefs.gfx_monitor[i].gfx_size_win.width + 7) & ~7) ? 16 : 0;
		c2 |= currprefs.gfx_monitor[i].gfx_size_win.height != changed_prefs.gfx_monitor[i].gfx_size_win.height ? 16 : 0;
#ifdef AMIBERRY
		c2 |= currprefs.gfx_auto_height != changed_prefs.gfx_auto_height ? 16 : 0;
		c2 |= currprefs.gfx_correct_aspect != changed_prefs.gfx_correct_aspect ? 16 : 0;
		c2 |= currprefs.scaling_method != changed_prefs.scaling_method ? 16 : 0;
#endif
		if (c2) {
			c |= c2;
			monitors[i] = true;
		}
	}
	monitors[0] = true;

	c |= currprefs.color_mode != changed_prefs.color_mode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[0].gfx_fullscreen != changed_prefs.gfx_apmode[0].gfx_fullscreen ? 16 : 0;
	c |= currprefs.gfx_apmode[1].gfx_fullscreen != changed_prefs.gfx_apmode[1].gfx_fullscreen ? 16 : 0;
	c |= currprefs.gfx_apmode[0].gfx_vsync != changed_prefs.gfx_apmode[0].gfx_vsync ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[1].gfx_vsync != changed_prefs.gfx_apmode[1].gfx_vsync ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[0].gfx_vsyncmode != changed_prefs.gfx_apmode[0].gfx_vsyncmode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[1].gfx_vsyncmode != changed_prefs.gfx_apmode[1].gfx_vsyncmode ? 2 | 16 : 0;
	c |= currprefs.gfx_apmode[0].gfx_refreshrate != changed_prefs.gfx_apmode[0].gfx_refreshrate ? 2 | 16 : 0;
	c |= currprefs.gfx_autoresolution != changed_prefs.gfx_autoresolution ? (2 | 8 | 16) : 0;
	c |= currprefs.gfx_autoresolution_vga != changed_prefs.gfx_autoresolution_vga ? (2 | 8 | 16) : 0;
	c |= currprefs.gfx_api != changed_prefs.gfx_api ? (1 | 8 | 32) : 0;
	c |= currprefs.gfx_api_options != changed_prefs.gfx_api_options ? (1 | 8 | 32) : 0;
	c |= currprefs.lightboost_strobo != changed_prefs.lightboost_strobo ? (2 | 16) : 0;

	for (int j = 0; j < 2; j++) {
		struct gfx_filterdata* gf = &currprefs.gf[j];
		struct gfx_filterdata* gfc = &changed_prefs.gf[j];

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
	c |= currprefs.gfx_scandoubler != changed_prefs.gfx_scandoubler ? (2 | 8) : 0;
	c |= currprefs.gfx_threebitcolors != changed_prefs.gfx_threebitcolors ? (256) : 0;
	c |= currprefs.gfx_grayscale != changed_prefs.gfx_grayscale ? (512) : 0;

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
	
	if (display_change_requested || c)
	{
		bool setpause = false;
		bool dontcapture = false;
		int keepfsmode =
			currprefs.gfx_apmode[0].gfx_fullscreen == changed_prefs.gfx_apmode[0].gfx_fullscreen &&
			currprefs.gfx_apmode[1].gfx_fullscreen == changed_prefs.gfx_apmode[1].gfx_fullscreen;

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
			}
			else if (display_change_requested == 2) {
				c = 512;
			}
			else {
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

		for (int j = 0; j < 2; j++) {
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
		
		currprefs.gfx_apmode[0].gfx_fullscreen = changed_prefs.gfx_apmode[0].gfx_fullscreen;
		currprefs.gfx_apmode[1].gfx_fullscreen = changed_prefs.gfx_apmode[1].gfx_fullscreen;
		currprefs.gfx_apmode[0].gfx_vsync = changed_prefs.gfx_apmode[0].gfx_vsync;
		currprefs.gfx_apmode[1].gfx_vsync = changed_prefs.gfx_apmode[1].gfx_vsync;
		currprefs.gfx_apmode[0].gfx_vsyncmode = changed_prefs.gfx_apmode[0].gfx_vsyncmode;
		currprefs.gfx_apmode[1].gfx_vsyncmode = changed_prefs.gfx_apmode[1].gfx_vsyncmode;
		currprefs.gfx_apmode[0].gfx_refreshrate = changed_prefs.gfx_apmode[0].gfx_refreshrate;

		currprefs.gfx_auto_height = changed_prefs.gfx_auto_height;
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
		currprefs.gfx_scandoubler = changed_prefs.gfx_scandoubler;
		currprefs.gfx_threebitcolors = changed_prefs.gfx_threebitcolors;
		currprefs.gfx_grayscale = changed_prefs.gfx_grayscale;

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
				clearscreen();
			}
			if (c & 256) {
				init_colors(mon->monitor_id);
				reset_drawing();
			}
			if (c & 128) {
				if (currprefs.gfx_autoresolution) {
					c |= 2 | 8;
				}
				else {
					c |= 16;
					reset_drawing();
					//S2X_reset();
				}
			}
			if (c & 1024) {
				target_graphics_buffer_update(mon->monitor_id);
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
				graphics_init(dontcapture ? false : true);
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

	if (currprefs.gf[0].gfx_filter_autoscale != changed_prefs.gf[0].gfx_filter_autoscale ||
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

		get_custom_limits(NULL, NULL, NULL, NULL, NULL);
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
		currprefs.active_capture_priority != changed_prefs.active_capture_priority ||
		currprefs.inactive_priority != changed_prefs.inactive_priority ||
		currprefs.active_nocapture_nosound != changed_prefs.active_nocapture_nosound ||
		currprefs.active_nocapture_pause != changed_prefs.active_nocapture_pause ||
		currprefs.inactive_nosound != changed_prefs.inactive_nosound ||
		currprefs.inactive_pause != changed_prefs.inactive_pause ||
		currprefs.inactive_input != changed_prefs.inactive_input ||
		currprefs.minimized_priority != changed_prefs.minimized_priority ||
		currprefs.minimized_nosound != changed_prefs.minimized_nosound ||
		currprefs.minimized_pause != changed_prefs.minimized_pause ||
		currprefs.minimized_input != changed_prefs.minimized_input ||
		currprefs.native_code != changed_prefs.native_code ||
		currprefs.alt_tab_release != changed_prefs.alt_tab_release ||
		currprefs.use_retroarch_quit != changed_prefs.use_retroarch_quit ||
		currprefs.use_retroarch_menu != changed_prefs.use_retroarch_menu ||
		currprefs.use_retroarch_reset != changed_prefs.use_retroarch_reset ||
		currprefs.sound_pullmode != changed_prefs.sound_pullmode ||
		currprefs.input_analog_remap != changed_prefs.input_analog_remap ||
		currprefs.kbd_led_num != changed_prefs.kbd_led_num ||
		currprefs.kbd_led_scr != changed_prefs.kbd_led_scr ||
		currprefs.right_control_is_right_win_key != changed_prefs.right_control_is_right_win_key)
	{
		currprefs.leds_on_screen = changed_prefs.leds_on_screen;
		currprefs.keyboard_leds[0] = changed_prefs.keyboard_leds[0];
		currprefs.keyboard_leds[1] = changed_prefs.keyboard_leds[1];
		currprefs.keyboard_leds[2] = changed_prefs.keyboard_leds[2];
		currprefs.input_mouse_untrap = changed_prefs.input_mouse_untrap;
		currprefs.input_magic_mouse_cursor = changed_prefs.input_magic_mouse_cursor;
		currprefs.active_capture_priority = changed_prefs.active_capture_priority;
		currprefs.inactive_priority = changed_prefs.inactive_priority;
		currprefs.active_nocapture_nosound = changed_prefs.active_nocapture_nosound;
		currprefs.active_nocapture_pause = changed_prefs.active_nocapture_pause;
		currprefs.inactive_nosound = changed_prefs.inactive_nosound;
		currprefs.inactive_pause = changed_prefs.inactive_pause;
		currprefs.inactive_input = changed_prefs.inactive_input;
		currprefs.minimized_priority = changed_prefs.minimized_priority;
		currprefs.minimized_nosound = changed_prefs.minimized_nosound;
		currprefs.minimized_pause = changed_prefs.minimized_pause;
		currprefs.minimized_input = changed_prefs.minimized_input;
		currprefs.native_code = changed_prefs.native_code;
		currprefs.alt_tab_release = changed_prefs.alt_tab_release;
		currprefs.use_retroarch_quit = changed_prefs.use_retroarch_quit;
		currprefs.use_retroarch_menu = changed_prefs.use_retroarch_menu;
		currprefs.use_retroarch_reset = changed_prefs.use_retroarch_reset;
		currprefs.sound_pullmode = changed_prefs.sound_pullmode;
		currprefs.input_analog_remap = changed_prefs.input_analog_remap;
		currprefs.kbd_led_num = changed_prefs.kbd_led_num;
		currprefs.kbd_led_scr = changed_prefs.kbd_led_scr;
		currprefs.right_control_is_right_win_key = changed_prefs.right_control_is_right_win_key;
		inputdevice_unacquire();
		currprefs.keyboard_leds_in_use = changed_prefs.keyboard_leds_in_use = (currprefs.keyboard_leds[0] | currprefs.keyboard_leds[1] | currprefs.keyboard_leds[2]) != 0;
		pause_sound();
		resume_sound();
		inputdevice_acquire(TRUE);
		setpriority(currprefs.active_capture_priority);
		return 1;
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

	return 0;
}

float target_getcurrentvblankrate(int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	float vb;
	if (currprefs.gfx_variable_sync)
		return (float)mon->currentmode.freq;
	if (get_display_vblank_params(-1, NULL, NULL, &vb, NULL)) {
		return vb;
	}

	return SDL2_getrefreshrate(0);
}

int lockscr(struct vidbuffer* vb, bool fullupdate, bool first)
{
	if (sdl_surface && SDL_MUSTLOCK(sdl_surface))
		SDL_LockSurface(sdl_surface);
	//int pitch;
	//SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&vb->bufmem), &pitch);
	init_row_map();
	return 1;
}

void unlockscr(struct vidbuffer* vb, int y_start, int y_end)
{
	if (sdl_surface && SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);
	//SDL_UnlockTexture(texture);
}

bool render_screen(int monid, int mode, bool immediate)
{
	if (savestate_state == STATE_DOSAVE)
	{
		if (delay_savestate_frame > 0)
			--delay_savestate_frame;
		else
		{
			create_screenshot();
			save_thumb(screenshot_filename);
			savestate_state = 0;
		}
	}

	return true;
}

float target_adjust_vblank_hz(float hz)
{
	int maxrate;
	if (!currprefs.lightboost_strobo)
		return hz;
	maxrate = deskhz;
	double nhz = hz * 2.0;
	if (nhz >= maxrate - 1 && nhz < maxrate + 1)
		hz -= 0.5;
	return hz;
}

void show_screen(int monid, int mode)
{
	const auto start = read_processor_time();
		
#ifdef USE_DISPMANX
	const auto wait_till = current_vsync_frame;
	if (vsync_modulo == 1)
	{
		// Amiga framerate is equal to host framerate
		do
		{
			usleep(10);
			current_vsync_frame = vsync_counter;
		} while (wait_till >= current_vsync_frame && read_processor_time() - start < 40000);
		if (wait_till + 1 != current_vsync_frame)
		{
			// We missed a vsync...
			next_synctime = 0;
		}
	}
	else
	{
		// Amiga framerate differs from host framerate
		const auto wait_till_time = next_synctime != 0 ? next_synctime : last_synctime + time_per_frame;
		if (current_vsync_frame % vsync_modulo == 0)
		{
			// Real vsync
			if (start < wait_till_time)
			{
				// We are in time, wait for vsync
				atomic_set(&vsync_counter, current_vsync_frame);
				do
				{
					usleep(10);
					current_vsync_frame = vsync_counter;
				} while (wait_till >= current_vsync_frame && read_processor_time() - start < 40000);
			}
			else
			{
				// Too late for vsync
			}
		}
		else
		{
			// Estimate vsync by time
			while (wait_till_time > read_processor_time())
			{
				usleep(10);
			}
			++current_vsync_frame;
		}
	}

	if (currprefs.gfx_framerate == 2)
		current_vsync_frame++;
	
#endif

#ifdef USE_DISPMANX
	wait_for_display_thread();
	flip_in_progess = true;
	write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SHOW, 1);
#else
	if (amiberry_options.use_sdl2_render_thread)
	{
		wait_for_display_thread();
		flip_in_progess = true;
		// RenderPresent must be done in the main thread.
		SDL_RenderPresent(sdl_renderer);
		write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SHOW, 1);	
	}
	else 
	{
		flip_in_progess = true;
		SDL_RenderClear(sdl_renderer);
		SDL_UpdateTexture(amiga_texture, nullptr, sdl_surface->pixels, sdl_surface->pitch);
		SDL_RenderCopyEx(sdl_renderer, amiga_texture, nullptr, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
		SDL_RenderPresent(sdl_renderer);
		flip_in_progess = false;
	}
#endif

	last_synctime = read_processor_time();
	idletime += last_synctime - start;

#ifdef USE_DISPMANX
	if (last_synctime - next_synctime > time_per_frame - 5000)
		next_synctime = last_synctime + time_per_frame * currprefs.gfx_framerate;
	else
		next_synctime = next_synctime + time_per_frame * currprefs.gfx_framerate;
#endif
}

unsigned long target_lastsynctime()
{
	return last_synctime;
}

bool show_screen_maybe(int monid, bool show)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct amigadisplay* ad = &adisplays[monid];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];
	if (!ap->gfx_vflip || ap->gfx_vsyncmode == 0 || ap->gfx_vsync <= 0) {
		if (show)
			show_screen(0, 0);
		return false;
	}
	return false;
}

float target_adjust_vblank_hz(int monid, float hz)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	int maxrate;
	if (!currprefs.lightboost_strobo)
		return hz;
	if (isfullscreen() > 0) {
		maxrate = mon->currentmode.freq;
	}
	else {
		maxrate = deskhz;
	}
	double nhz = hz * 2.0;
	if (nhz >= maxrate - 1 && nhz < maxrate + 1)
		hz -= 0.5;
	return hz;
}

void target_cpu_speed(void)
{
	//display_vblank_thread(&AMonitors[0]);
}

const TCHAR* target_get_display_name(int num, bool friendlyname)
{
	if (num <= 0)
		return NULL;
	struct MultiDisplay* md = getdisplay2(NULL, num - 1);
	if (!md)
		return NULL;
	if (friendlyname)
		return md->monitorname;
	return md->monitorid;
}

void getgfxoffset(int monid, float* dxp, float* dyp, float* mxp, float* myp)
{
	float dx = 0, dy = 0, mx = 1.0, my = 1.0;

	*dxp = dx;
	*dyp = dy;
	*mxp = 1.0f / mx;
	*myp = 1.0f / my;
}

void DX_Fill(struct AmigaMonitor* mon, int dstx, int dsty, int width, int height, uae_u32 color)
{
	SDL_Rect dstrect;
	if (width < 0)
		width = sdl_surface->w;
	if (height < 0)
		height = sdl_surface->h;
	dstrect.x = dstx;
	dstrect.y = dsty;
	dstrect.w = width;
	dstrect.h = height;
	SDL_FillRect(sdl_surface, &dstrect, color);
}

void clearscreen()
{
#ifndef USE_DISPMANX
	if (amiberry_options.use_sdl2_render_thread)
#endif
		wait_for_display_thread();

	if (sdl_surface != nullptr)
	{
		SDL_FillRect(sdl_surface, nullptr, 0);
		render_screen(0, 0, true);
		show_screen(0, 0);
	}
}

static void graphics_subinit()
{
	if (sdl_surface == nullptr)
	{
		open_screen(&currprefs);
		if (sdl_surface == nullptr)
			write_log("Unable to set video mode: %s\n", SDL_GetError());
	}
}

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

void init_colors(int monid)
{
	/* Truecolor: */
	red_bits = bits_in_mask(sdl_surface->format->Rmask);
	green_bits = bits_in_mask(sdl_surface->format->Gmask);
	blue_bits = bits_in_mask(sdl_surface->format->Bmask);
	red_shift = mask_shift(sdl_surface->format->Rmask);
	green_shift = mask_shift(sdl_surface->format->Gmask);
	blue_shift = mask_shift(sdl_surface->format->Bmask);
	alpha_bits = bits_in_mask(sdl_surface->format->Amask);
	alpha_shift = mask_shift(sdl_surface->format->Amask);

	alloc_colors64k(monid, red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, alpha_bits, alpha_shift, alpha, 0, false);
	notice_new_xcolors();
}

/*
* Find the colour depth of the display
*/
static int get_display_depth()
{
	return sdl_surface->format->BytesPerPixel * 8;
}

int machdep_init(void)
{
	for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
		struct AmigaMonitor* mon = &AMonitors[i];
		struct amigadisplay* ad = &adisplays[i];
		mon->monitor_id = i;
		ad->picasso_requested_on = false;
		ad->picasso_on = false;
		mon->screen_is_picasso = 0;
		memset(&mon->currentmode, 0, sizeof(*&mon->currentmode));
	}

	return 1;
}

int graphics_init(bool mousecapture)
{
	struct AmigaMonitor* mon = &AMonitors[0];
#ifdef USE_DISPMANX
	if (!mon->sdl_window)
	{
		mon->sdl_window = SDL_CreateWindow("Amiberry",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			0,
			0,
			SDL_WINDOW_FULLSCREEN_DESKTOP);
		check_error_sdl(mon->sdl_window == nullptr, "Unable to create window");
	}
#else
	write_log("Getting Current Video Driver...\n");
	sdl_video_driver = SDL_GetCurrentVideoDriver();

	const auto should_be_zero = SDL_GetCurrentDisplayMode(0, &sdlMode);
	if (should_be_zero == 0)
	{
		write_log("Current Display mode: bpp %i\t%s\t%i x %i\t%iHz\n", SDL_BITSPERPIXEL(sdlMode.format), SDL_GetPixelFormatName(sdlMode.format), sdlMode.w, sdlMode.h, sdlMode.refresh_rate);
		vsync_vblank = sdlMode.refresh_rate;
	}

	write_log("Creating Amiberry window...\n");

	if (!mon->sdl_window)
	{
		Uint32 sdl_window_mode;
		if (sdlMode.w >= 800 && sdlMode.h >= 600 && strcmpi(sdl_video_driver, "KMSDRM") != 0)
		{
			// Only enable Windowed mode if we're running under x11 and the resolution is at least 800x600
			if (currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW)
				sdl_window_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
			else if (currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN)
				sdl_window_mode = SDL_WINDOW_FULLSCREEN;
			else
				sdl_window_mode = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
			if (currprefs.borderless)
				sdl_window_mode |= SDL_WINDOW_BORDERLESS;
			if (currprefs.main_alwaysontop)
				sdl_window_mode |= SDL_WINDOW_ALWAYS_ON_TOP;
		}
		else
		{
			// otherwise go for Full-window
			sdl_window_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		
		if (amiberry_options.rotation_angle != 0 && amiberry_options.rotation_angle != 180)
		{
			mon->sdl_window = SDL_CreateWindow("Amiberry",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_HEIGHT,
				GUI_WIDTH,
				sdl_window_mode);
		}
		else
		{
			mon->sdl_window = SDL_CreateWindow("Amiberry",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_WIDTH,
				GUI_HEIGHT,
				sdl_window_mode);
		}
		check_error_sdl(mon->sdl_window == nullptr, "Unable to create window:");

		auto* const icon_surface = IMG_Load("data/amiberry.png");
		if (icon_surface != nullptr)
		{
			SDL_SetWindowIcon(mon->sdl_window, icon_surface);
			SDL_FreeSurface(icon_surface);
		}
	}

#endif

	if (sdl_renderer == nullptr)
	{
		sdl_renderer = SDL_CreateRenderer(mon->sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(sdl_renderer == nullptr, "Unable to create a renderer:");
	}

	if (SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1") != SDL_TRUE)
		write_log("SDL2: could not grab the keyboard!\n");

	if (SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0") == SDL_TRUE)
		write_log("SDL2: Set window not to minimize on focus loss\n");

	if (currprefs.rtgvblankrate == 0) {
		currprefs.gfx_apmode[1].gfx_refreshrate = currprefs.gfx_apmode[0].gfx_refreshrate;
		if (currprefs.gfx_apmode[0].gfx_interlaced) {
			currprefs.gfx_apmode[1].gfx_refreshrate *= 2;
		}
	}
	else if (currprefs.rtgvblankrate < 0) {
		currprefs.gfx_apmode[1].gfx_refreshrate = 0;
	}
	else {
		currprefs.gfx_apmode[1].gfx_refreshrate = currprefs.rtgvblankrate;
	}

#ifndef USE_DISPMANX
	if (strcmpi(sdl_video_driver, "KMSDRM") == 0)
	{
		// Disable the render thread under KMSDRM (not supported)
		amiberry_options.use_sdl2_render_thread = false;
	}
	
	if (amiberry_options.use_sdl2_render_thread)
	{
#endif
		if (display_pipe == nullptr) {
			display_pipe = xmalloc(smp_comm_pipe, 1);
			init_comm_pipe(display_pipe, 20, 1);
		}
		if (display_sem == nullptr) {
			uae_sem_init(&display_sem, 0, 0);
		}
		if (display_tid == nullptr && display_pipe != nullptr && display_sem != nullptr) {
			uae_start_thread(_T("display thread"), display_thread, nullptr, &display_tid);
		}
		write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SETUP, 1);
#ifndef USE_DISPMANX
	}
#endif
	
	inputdevice_unacquire();
	graphics_subinit();

	inputdevice_acquire(TRUE);
	return 1;
}

void graphics_leave()
{
	struct AmigaMonitor* mon = &AMonitors[0];
	graphics_subshutdown();

#ifndef USE_DISPMANX
	if (amiberry_options.use_sdl2_render_thread)
	{
#endif
		if (display_tid != nullptr) {
			write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_QUIT, 1);
			while (display_tid != nullptr) {
				sleep_millis(10);
			}
			destroy_comm_pipe(display_pipe);
			xfree(display_pipe);
			display_pipe = nullptr;
			uae_sem_destroy(&display_sem);
			display_sem = nullptr;
		}
#ifndef USE_DISPMANX
	}
#endif
#ifdef USE_DISPMANX
	bcm_host_deinit();
#else
	if (amiga_texture)
	{
		SDL_DestroyTexture(amiga_texture);
		amiga_texture = nullptr;
	}
#endif
	
	if (sdl_renderer)
	{
		SDL_DestroyRenderer(sdl_renderer);
		sdl_renderer = nullptr;
	}

	if (mon->sdl_window)
	{
		SDL_DestroyWindow(mon->sdl_window);
		mon->sdl_window = nullptr;
	}
}

void close_windows(struct AmigaMonitor* mon)
{
	reset_sound();
	graphics_subshutdown();
}

static int save_png(SDL_Surface* surface, char* path)
{
	const auto w = surface->w;
	const auto h = surface->h;
	auto* const pix = static_cast<unsigned char *>(surface->pixels);
	unsigned char writeBuffer[1920 * 3];
	auto* const f = fopen(path, "wbe");
	if (!f) return 0;
	auto* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
											nullptr,
											nullptr,
											nullptr);
	if (!png_ptr)
	{
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

static void create_screenshot()
{
	if (amiberry_options.use_sdl2_render_thread)
		wait_for_display_thread();

	if (current_screenshot != nullptr)
	{
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}

	if (sdl_surface != nullptr) {
	current_screenshot = SDL_CreateRGBSurfaceFrom(sdl_surface->pixels,
		sdl_surface->w,
		sdl_surface->h,
		sdl_surface->format->BitsPerPixel,
		sdl_surface->pitch,
		sdl_surface->format->Rmask,
		sdl_surface->format->Gmask,
		sdl_surface->format->Bmask,
		sdl_surface->format->Amask);
	}
}

static int save_thumb(char* path)
{
	if (amiberry_options.use_sdl2_render_thread)
		wait_for_display_thread();

	auto ret = 0;
	if (current_screenshot != nullptr)
	{
		ret = save_png(current_screenshot, path);
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}
	return ret;
}

#ifdef USE_DISPMANX	
static int currVSyncRate = 0;
#endif
bool vsync_switchmode(int monid, int hz)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	static struct PicassoResolution* oldmode;
	static int oldhz;
	int w = sdl_surface->w;
	int h = sdl_surface->h;
	struct PicassoResolution* found;
	int newh, i, cnt;
	bool preferdouble = false, preferlace = false;
	bool lace = false;

	if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate > 85) {
		preferdouble = true;
	}
	else if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced) {
		preferlace = true;
	}
	
	if (hz >= 55)
		hz = 60;
	else
		hz = 50;
	
#ifdef USE_DISPMANX	
	if (hz != currVSyncRate)
	{
		currVSyncRate = hz;
		fpscounter_reset();

		time_per_frame = 1000 * 1000 / (hz);

		if (hz == vsync_vblank)
			vsync_modulo = 1;
		else if (hz > vsync_vblank)
			vsync_modulo = 6; // Amiga draws 6 frames while host has 5 vsyncs -> sync every 6th Amiga frame
		else
			vsync_modulo = 5; // Amiga draws 5 frames while host has 6 vsyncs -> sync every 5th Amiga frame
	}
	return true;
#else
	newh = h * (currprefs.ntscmode ? 60 : 50) / hz;

	found = NULL;

	for (cnt = 0; cnt <= abs(newh - h) + 1 && !found; cnt++) {
		for (int dbl = 0; dbl < 2 && !found; dbl++) {
			bool doublecheck = false;
			bool lacecheck = false;
			if (preferdouble && dbl == 0)
				doublecheck = true;
			else if (preferlace && dbl == 0)
				lacecheck = true;

			for (int extra = 1; extra >= -1 && !found; extra--) {
				for (i = 0; DisplayModes[i].depth >= 0 && !found; i++) {
					struct PicassoResolution* r = &DisplayModes[i];
					if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt)) {
						int j;
						for (j = 0; r->refresh[j] > 0; j++) {
							if (doublecheck) {
								if (r->refreshtype[j] & REFRESH_RATE_LACE)
									continue;
								if (r->refresh[j] == hz * 2 + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							}
							else if (lacecheck) {
								if (!(r->refreshtype[j] & REFRESH_RATE_LACE))
									continue;
								if (r->refresh[j] * 2 == hz + extra) {
									found = r;
									lace = true;
									hz = r->refresh[j];
									break;
								}
							}
							else {
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
	}
	else {
		newh = found->res.height;
		changed_prefs.gfx_monitor[0].gfx_size_fs.height = newh;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = hz;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = lace;
		if (changed_prefs.gfx_monitor[0].gfx_size_fs.height != currprefs.gfx_monitor[0].gfx_size_fs.height ||
			changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate != currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate) {
			write_log(_T("refresh rate changed to %d%s, new screenmode %dx%d\n"), hz, lace ? _T("i") : _T("p"), w, newh);
			set_config_changed();
		}
		return true;
	}
#endif
}

void vsync_clear(void)
{
	vsync_active = false;
	//if (waitvblankevent)
	//	ResetEvent(waitvblankevent);
}

int vsync_isdone(frame_time_t* dt)
{
	if (isvsync() == 0)
		return -1;
	//if (waitvblankthread_mode <= 0)
	//	return -2;
	//if (dt)
	//	*dt = wait_vblank_timestamp;
	return vsync_active ? 1 : 0;
}

static int oldtex_w[MAX_AMIGAMONITORS], oldtex_h[MAX_AMIGAMONITORS], oldtex_rtg[MAX_AMIGAMONITORS];
bool target_graphics_buffer_update(int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[monid];
	struct vidbuf_description* avidinfo = &adisplays[monid].gfxvidinfo;
	struct picasso96_state_struct* state = &picasso96_state[monid];

	int w, h;
	auto rate_changed = false;

	if (mon->screen_is_picasso) {
		w = state->Width > vidinfo->width ? state->Width : vidinfo->width;
		h = state->Height > vidinfo->height ? state->Height : vidinfo->height;
	}
	else {
		struct vidbuffer* vb = avidinfo->drawbuffer.tempbufferinuse ? &avidinfo->tempbuffer : &avidinfo->drawbuffer;
		avidinfo->outbuffer = vb;
		w = vb->outwidth;
		h = vb->outheight;
	}
	
	if (currprefs.gfx_monitor[monid].gfx_size.height != changed_prefs.gfx_monitor[monid].gfx_size.height)
	{
		update_display(&changed_prefs);
		rate_changed = true;
	}

	if (rate_changed)
	{
		fpscounter_reset();
#ifdef USE_DISPMANX
		time_per_frame = 1000 * 1000 / currprefs.chipset_refreshrate;
#endif
	}

	if (oldtex_w[monid] == w && oldtex_h[monid] == h && oldtex_rtg[monid] == mon->screen_is_picasso)
		return false;

	if (!w || !h) {
		oldtex_w[monid] = w;
		oldtex_h[monid] = h;
		oldtex_rtg[monid] = mon->screen_is_picasso;
		return false;
	}

	oldtex_w[monid] = w;
	oldtex_h[monid] = h;
	oldtex_rtg[monid] = mon->screen_is_picasso;

	write_log(_T("Buffer %d size (%d*%d) %s\n"), monid, w, h, mon->screen_is_picasso ? _T("RTG") : _T("Native"));

	return true;
}

#ifdef PICASSO96

int picasso_palette(struct MyCLUTEntry *CLUT, uae_u32 *clut)
{
	auto changed = 0;

	for (auto i = 0; i < 256 * 2; i++) {
		int r = CLUT[i].Red;
		int g = CLUT[i].Green;
		int b = CLUT[i].Blue;
		auto v = (doMask256 (r, red_bits, red_shift)
			| doMask256 (g, green_bits, green_shift)
			| doMask256 (b, blue_bits, blue_shift))
			| doMask256 (0xff, alpha_bits, alpha_shift);
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

static void addmode(struct MultiDisplay* md, SDL_DisplayMode* dm, int rawmode)
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
	_stprintf(md->DisplayModes[i].name, _T("%dx%d%s, %d-bit"),
		md->DisplayModes[i].res.width, md->DisplayModes[i].res.height,
		lace ? _T("i") : _T(""),
		md->DisplayModes[i].depth * 8);
}

static int resolution_compare(const void* a, const void* b)
{
	auto* ma = (struct PicassoResolution *)a;
	auto* mb = (struct PicassoResolution *)b;
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

void sortdisplays()
{
	struct MultiDisplay* md;
	int i, idx;
	char tmp[200];
	
#ifdef USE_DISPMANX	
	int w = 800;
	int h = 600;
	int wv = w;
	int hv = h;
	int b = 24;
	
	Displays[0].primary = 1;
	Displays[0].rect.x = 0;
	Displays[0].rect.y = 0;
	Displays[0].rect.w = 800;
	Displays[0].rect.h = 600;
#else
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

	deskhz = desktop_dm.refresh_rate;

	SDL_Rect bounds;
	if (SDL_GetDisplayUsableBounds(0, &bounds) != 0)
	{
		write_log("SDL_GetDisplayUsableBounds failed: %s\n", SDL_GetError());
		return;
	}

	Displays[0].primary = 1;
	Displays[0].rect.x = bounds.x;
	Displays[0].rect.y = bounds.y;
	Displays[0].rect.w = bounds.w;
	Displays[0].rect.h = bounds.h;
#endif
	sprintf(tmp, "%s (%d*%d)", "Display", Displays[0].rect.w, Displays[0].rect.h);
	Displays[0].fullname = my_strdup(tmp);
	Displays[0].monitorname = my_strdup("Display");

	md = Displays;

	md->DisplayModes = xmalloc(struct PicassoResolution, MAX_PICASSO_MODES);
	md->DisplayModes[0].depth = -1;
#ifdef USE_DISPMANX
	auto count = 0;
	int bits[] = { 8, 16, 32 };
	for (auto i = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; i++)
	{
		for (auto bitdepth : bits)
		{
			const auto bit_unit = bitdepth + 1 & 0xF8;
			const auto rgbFormat =
				bitdepth == 8 ? RGBFB_CLUT :
				bitdepth == 16 ? RGBFB_R5G6B5 : RGBFB_R8G8B8A8;
			auto pixelFormat = 1 << rgbFormat;
			pixelFormat |= RGBFF_CHUNKY;
			md->DisplayModes[count].rawmode = 0;
			md->DisplayModes[count].lace = false;
			md->DisplayModes[count].res.width = x_size_table[i];
			md->DisplayModes[count].res.height = y_size_table[i];
			md->DisplayModes[count].depth = bit_unit >> 3;
			md->DisplayModes[count].refresh[0] = 50;
			md->DisplayModes[count].refreshtype[0] = 0;
			md->DisplayModes[count].refresh[1] = 60;
			md->DisplayModes[count].refreshtype[1] = 0;
			md->DisplayModes[count].refresh[2] = 0;
			md->DisplayModes[count].colormodes = pixelFormat;
			sprintf(md->DisplayModes[count].name, "%dx%d, %d-bit",
				md->DisplayModes[count].res.width, md->DisplayModes[count].res.height, md->DisplayModes[count].depth * 8);

			count++;
		}
	}
	md->DisplayModes[count].depth = -1;
#else
	int numDispModes = SDL_GetNumDisplayModes(0);
	for (int mode = 0; mode < 2; mode++) 
	{
		SDL_DisplayMode dm;
		for (idx = 0; idx <= numDispModes; idx++)
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
					if (dm.refresh_rate > deskhz)
						deskhz = dm.refresh_rate;
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
			idx++;
		}
		
	}
#endif	
	sortmodes(md);
	modesList(md);
	i = 0;
	while (md->DisplayModes[i].depth > 0)
		i++;
	write_log(_T("%d display modes.\n"), i);
	write_log(_T("Desktop: W=%d H=%d B=%d HZ=%d. CXVS=%d CYVS=%d\n"), w, h, b, deskhz, wv, hv);
}
#endif

#ifdef PICASSO96
void gfx_set_picasso_state(int monid, int on)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	if (mon->screen_is_picasso == on)
		return;
	mon->screen_is_picasso = on;

	clearscreen();
	open_screen(&currprefs);
}

void gfx_set_picasso_modeinfo(int monid, uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	if (!mon->screen_is_picasso)
		return;
	clearscreen();
	gfx_set_picasso_colors(0, rgbfmt);

	if (static_cast<unsigned>(picasso_vidinfo[0].width) == w &&
		static_cast<unsigned>(picasso_vidinfo[0].height) == h &&
		static_cast<unsigned>(picasso_vidinfo[0].depth) == depth &&
		picasso_vidinfo[0].selected_rgbformat == rgbfmt)
		return;

	picasso_vidinfo[0].selected_rgbformat = rgbfmt;
	picasso_vidinfo[0].width = w;
	picasso_vidinfo[0].height = h;
	picasso_vidinfo[0].depth = depth;
	picasso_vidinfo[0].extra_mem = 1;
	picasso_vidinfo[0].rowbytes = sdl_surface->pitch;
	picasso_vidinfo[0].pixbytes = sdl_surface->format->BytesPerPixel;
	picasso_vidinfo[0].offset = 0;

	if (mon->screen_is_picasso)
		open_screen(&currprefs);
}

void gfx_set_picasso_colors(int monid, RGBFTYPE rgbfmt)
{
	alloc_colors_picasso(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt, p96_rgbx16);
}

uae_u8* gfx_lock_picasso(int monid, bool fullupdate, bool doclear)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[0];
	static uae_u8* p;
	if (sdl_surface == nullptr || mon->screen_is_picasso == 0)
		return nullptr;
	if (SDL_MUSTLOCK(sdl_surface))
		SDL_LockSurface(sdl_surface);

	vidinfo->pixbytes = sdl_surface->format->BytesPerPixel;
	vidinfo->rowbytes = sdl_surface->pitch;
	p = static_cast<uae_u8*>(sdl_surface->pixels);
	if (!p)
	{
		if (SDL_MUSTLOCK(sdl_surface))
			SDL_UnlockSurface(sdl_surface);
	}
	else
	{
		if (doclear)
		{
			auto* p2 = p;
			for (auto h = 0; h < vidinfo->height; h++)
			{
				memset(p2, 0, vidinfo->width * vidinfo->pixbytes);
				p2 += vidinfo->rowbytes;
			}
		}
	}
	return p;
}

void gfx_unlock_picasso(int monid, const bool dorender)
{
	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);

	if (dorender)
	{
		render_screen(0, 0, true);
		show_screen(0, 0);
	}
}

#endif // PICASSO96

float target_getcurrentvblankrate()
{
	return static_cast<float>(vsync_vblank);
}

int rtg_index = -1;

// -2 = default
// -1 = prev
// 0 = chipset
// 1..4 = rtg
// 5 = next
bool toggle_rtg(int monid, int mode)
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
		}
		else if (mode >= 0 && mode <= MAX_RTG_BOARDS) {
			rtg_index = mode - 1;
		}
		else {
			rtg_index++;
		}
		if (rtg_index >= MAX_RTG_BOARDS) {
			rtg_index = -1;
		}
		else if (rtg_index < -1) {
			rtg_index = MAX_RTG_BOARDS - 1;
		}
		if (rtg_index < 0) {
			if (ad->picasso_on) {
				//gfxboard_rtg_disable(monid, old_index);
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
				int idx = 0; // gfxboard_toggle(r->monitor_id, rtg_index, mode >= -1);
				if (idx >= 0) {
					rtg_index = idx;
					return true;
				}
				if (idx < -1) {
					rtg_index = -1;
					return false;
				}
			}
			else {
				//gfxboard_toggle(r->monitor_id, -1, -1);
				if (mode < -1)
					return true;
				devices_unsafeperiod();
				//gfxboard_rtg_disable(monid, old_index);
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

void close_rtg(int monid)
{
	close_windows(&AMonitors[monid]);
}