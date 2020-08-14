#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cmath>

#include "sysdeps.h"
#include "uae.h"
#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "savestate.h"
#include "picasso96.h"
#include "amiberry_gfx.h"

#include <png.h>
#include <SDL_image.h>

#include "clipboard.h"
#include "devices.h"
#include "inputdevice.h"

#if 0
#ifdef ANDROID
#include <SDL_screenkeyboard.h>
#endif
#endif

#include "gfxboard.h"
#include "statusline.h"
#include "sounddep/sound.h"
#include "threaddep/thread.h"

static uae_thread_id display_tid = nullptr;
static smp_comm_pipe *volatile display_pipe = nullptr;
static uae_sem_t display_sem = nullptr;
static bool volatile display_thread_busy = false;
#ifdef USE_DISPMANX
static unsigned int current_vsync_frame = 0;
unsigned long time_per_frame = 20000; // Default for PAL (50 Hz): 20000 microsecs
static int vsync_modulo = 1;
bool volatile flip_in_progess = false;
#endif

/* SDL Surface for output of emulation */
SDL_DisplayMode sdlMode;
SDL_Surface* screen = nullptr;
SDL_Texture* texture;
SDL_Rect renderQuad;
SDL_Renderer* renderer;
const char* sdl_video_driver;

#ifdef ANDROID
#include <android/log.h>
#endif

static int display_width;
static int display_height;
static int display_depth;
Uint32 pixel_format;
bool can_have_linedouble;

static unsigned long last_synctime;
static int host_hz = 50;
static bool clipboard_initialized;

/* Possible screen modes (x and y resolutions) */
#define MAX_SCREEN_MODES 14
static int x_size_table[MAX_SCREEN_MODES] = { 640, 640, 720, 800, 800, 960, 1024, 1280, 1280, 1280, 1360, 1366, 1680, 1920 };
static int y_size_table[MAX_SCREEN_MODES] = { 400, 480, 400, 480, 600, 540, 768, 720, 800, 1024, 768, 768, 1050, 1080 };

struct PicassoResolution* DisplayModes;
struct MultiDisplay Displays[MAX_DISPLAYS];

static int display_change_requested;
int screen_is_picasso = 0;
static int wasfullwindow_a, wasfullwindow_p;

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

static int display_thread(void *unused)
{
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
			if (screen_is_picasso)
			{
				if (picasso96_state.RGBFormat == RGBFB_R5G6B5
					|| picasso96_state.RGBFormat == RGBFB_R5G6B5PC
					|| picasso96_state.RGBFormat == RGBFB_CLUT)
				{
					display_depth = 16;
					rgb_mode = VC_IMAGE_RGB565;
					pixel_format = SDL_PIXELFORMAT_RGB565;
				}
				else 
				{
					display_depth = 32;
					rgb_mode = VC_IMAGE_RGBA32;
					pixel_format = SDL_PIXELFORMAT_RGBA32;
				}	
			}
			else
			{
				//display_depth = 16;
				//rgb_mode = VC_IMAGE_RGB565;
				display_depth = 32;
				rgb_mode = VC_IMAGE_RGBA32;
				pixel_format = SDL_PIXELFORMAT_RGBA32;
			}

			if (!screen)
				screen = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, display_depth, pixel_format);

			displayHandle = vc_dispmanx_display_open(0);

			if (!amigafb_resource_1)
				amigafb_resource_1 = vc_dispmanx_resource_create(rgb_mode, display_width, display_height, &vc_image_ptr);
			if (!amigafb_resource_2)
				amigafb_resource_2 = vc_dispmanx_resource_create(rgb_mode, display_width, display_height, &vc_image_ptr);
			if (!blackfb_resource)
				blackfb_resource = vc_dispmanx_resource_create(rgb_mode, display_width, display_height, &vc_image_ptr);

			vc_dispmanx_rect_set(&blit_rect, 0, 0, display_width, display_height);
			vc_dispmanx_resource_write_data(amigafb_resource_1, rgb_mode, screen->pitch, screen->pixels, &blit_rect);
			vc_dispmanx_resource_write_data(blackfb_resource, rgb_mode, screen->pitch, screen->pixels, &blit_rect);
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
				if (screen_is_picasso)
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
#ifdef USE_DISPMANX
			if (current_resource_amigafb == 1)
			{
				current_resource_amigafb = 0;
				vc_dispmanx_resource_write_data(amigafb_resource_1,
					rgb_mode,
					screen->pitch,
					screen->pixels,
					&blit_rect);
				updateHandle = vc_dispmanx_update_start(0);
				vc_dispmanx_element_change_source(updateHandle, elementHandle, amigafb_resource_1);
			}
			else
			{
				current_resource_amigafb = 1;
				vc_dispmanx_resource_write_data(amigafb_resource_2,
					rgb_mode,
					screen->pitch,
					screen->pixels,
					&blit_rect);
				updateHandle = vc_dispmanx_update_start(0);
				vc_dispmanx_element_change_source(updateHandle, elementHandle, amigafb_resource_2);
			}
			vc_dispmanx_update_submit(updateHandle, nullptr, nullptr);
			flip_in_progess = false;
#else
			SDL_UpdateTexture(texture, nullptr, screen->pixels, screen->pitch);
			SDL_RenderClear(renderer);
			SDL_RenderCopyEx(renderer, texture, nullptr, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
#endif
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
	picasso_init_resolutions();
	InitPicasso96();
#endif
#ifdef USE_DISPMANX
	bcm_host_init();
	displayHandle = vc_dispmanx_display_open(0);
	vc_dispmanx_display_get_info(displayHandle, &modeInfo);
	can_have_linedouble = modeInfo.height >= 540;
	
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
				host_hz = static_cast<int>(frame_rate);
			}
			vc_vchi_tv_stop();
			vchi_disconnect(vchi_instance);
		}
	}

	if (sdl_window == nullptr)
	{
		sdl_window = SDL_CreateWindow("Amiberry",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			0,
			0,
			SDL_WINDOW_FULLSCREEN_DESKTOP);
		check_error_sdl(sdl_window == nullptr, "Unable to create window");
	}

#else
	write_log("Trying to get Current Video Driver...\n");
	sdl_video_driver = SDL_GetCurrentVideoDriver();
	
	SDL_DisplayMode current_mode;
	const auto should_be_zero = SDL_GetCurrentDisplayMode(0, &current_mode);
	if (should_be_zero == 0)
	{
		write_log("Current Display mode: bpp %i\t%s\t%i x %i\t%iHz\n", SDL_BITSPERPIXEL(current_mode.format), SDL_GetPixelFormatName(current_mode.format), current_mode.w, current_mode.h, current_mode.refresh_rate);
		host_hz = current_mode.refresh_rate;
		can_have_linedouble = current_mode.h >= 540;
	}

	Uint32 sdl_window_mode;
	if (sdl_video_driver != nullptr && strcmp(sdl_video_driver,"x11") == 0 
		&& current_mode.w >= 800 && current_mode.h >= 600)
	{
		// Only enable Windowed mode if we're running under x11 and the resolution is at least 800x600
		sdl_window_mode = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	}
	else
	{
		// otherwise go for Fullscreen
		sdl_window_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	write_log("Trying to create window...\n");

	if (sdl_window == nullptr)
	{
		if (amiberry_options.rotation_angle != 0 && amiberry_options.rotation_angle != 180)
		{
			sdl_window = SDL_CreateWindow("Amiberry",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_HEIGHT,
				GUI_WIDTH,
				sdl_window_mode);
		}
		else
		{
			sdl_window = SDL_CreateWindow("Amiberry",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_WIDTH,
				GUI_HEIGHT,
				sdl_window_mode);
		}
		check_error_sdl(sdl_window == nullptr, "Unable to create window:");		
	}

#endif

	auto* const icon_surface = IMG_Load("data/amiberry.png");
	if (icon_surface != nullptr)
	{
		SDL_SetWindowIcon(sdl_window, icon_surface);
		SDL_FreeSurface(icon_surface);
	}
	
	if (renderer == nullptr)
	{
		renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(renderer == nullptr, "Unable to create a renderer:");
	}
	
	if (SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1") != SDL_TRUE)
		write_log("SDL2: could not grab the keyboard!\n");

	if (SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0") == SDL_TRUE)
		write_log("SDL2: Set window not to minimize on focus loss\n");
	
	currprefs.gfx_apmode[1].gfx_refreshrate = host_hz;

#ifndef USE_DISPMANX
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

	return 1;
}

void update_win_fs_mode(struct uae_prefs* p)
{
	auto* avidinfo = &adisplays.gfxvidinfo;
#ifdef USE_DISPMANX
	// Dispmanx modes use configurable width/height and are fullwindow always
	p->gfx_monitor.gfx_size = p->gfx_monitor.gfx_size_win;
#else
	if (sdl_window && strcmp(sdl_video_driver, "x11") == 0)
	{
		const auto window_flags = SDL_GetWindowFlags(sdl_window);
		const bool is_fullwindow = window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
		const bool is_fullscreen = window_flags & SDL_WINDOW_FULLSCREEN;

		if (p->gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN)
		{
			p->gfx_monitor.gfx_size = p->gfx_monitor.gfx_size_fs;
			// Switch to Fullscreen mode, if we don't have it already
			if (!is_fullscreen)
				SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN);
		}
		else if (p->gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW)
		{
			p->gfx_monitor.gfx_size = p->gfx_monitor.gfx_size_win;
			if (!is_fullwindow)
				SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
			p->gfx_monitor.gfx_size = p->gfx_monitor.gfx_size_win;
			// Switch to Window mode, if we don't have it already
			if (is_fullscreen || is_fullwindow)
				SDL_SetWindowFullscreen(sdl_window, 0);
		}
		
		set_config_changed();
	}
	else
	{
		// KMSDRM is fullwindow always
		p->gfx_monitor.gfx_size = p->gfx_monitor.gfx_size_win;
	}
#endif
	if (screen_is_picasso)
	{
		display_width = picasso96_state.Width ? picasso96_state.Width : 640;
		display_height = picasso96_state.Height ? picasso96_state.Height : 480;
	}
	else
	{
		if (currprefs.gfx_resolution > avidinfo->gfx_resolution_reserved)
			avidinfo->gfx_resolution_reserved = currprefs.gfx_resolution;
		if (currprefs.gfx_vresolution > avidinfo->gfx_vresolution_reserved)
			avidinfo->gfx_vresolution_reserved = currprefs.gfx_vresolution;

		display_width = p->gfx_monitor.gfx_size.width / 2 << p->gfx_resolution;
		display_height = p->gfx_monitor.gfx_size.height / 2 << p->gfx_vresolution;
	}
}

void toggle_fullscreen(int mode)
{
#ifdef USE_DISPMANX
	// Dispmanx is full-window always
#else
	auto* const ad = &adisplays;
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
	update_win_fs_mode(&currprefs);
#endif
}

static int isfullscreen_2(struct uae_prefs* p)
{
	const auto idx = screen_is_picasso ? 1 : 0;
	return p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLSCREEN ? 1 : p->gfx_apmode[idx].gfx_fullscreen == GFX_FULLWINDOW ? -1 : 0;
}
int isfullscreen(void)
{
	return isfullscreen_2(&currprefs);
}

static void wait_for_display_thread(void)
{
	while (display_thread_busy)
		usleep(10);
}

void allocsoftbuffer(struct vidbuffer* buf, int width, int height, int depth)
{
	/* Initialize structure for Amiga video modes */
	buf->pixbytes = screen->format->BytesPerPixel;
	buf->width_allocated = (width + 7) & ~7;
	buf->height_allocated = height;

	buf->outwidth = buf->width_allocated;
	buf->outheight = buf->height_allocated;
	buf->inwidth = buf->width_allocated;
	buf->inheight = buf->height_allocated;
	
	buf->rowbytes = screen->pitch;
	buf->realbufmem = static_cast<uae_u8*>(screen->pixels);
	buf->bufmem_allocated = buf->bufmem = buf->realbufmem;
	buf->bufmem_lockable = true;
	
	if (screen->format->BytesPerPixel == 2)
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

#ifndef USE_DISPMANX
	if (texture != nullptr)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
#endif
	
	if (screen)
	{
		SDL_FreeSurface(screen);
		screen = nullptr;
	}
}

#if 0 // Disabled until we see how this is implemented in SDL2
#ifdef ANDROID
void update_onscreen()
{
	SDL_ANDROID_SetScreenKeyboardFloatingJoystick(changed_prefs.floatingJoystick);
	if (changed_prefs.onScreen==0)
	{
	  SDL_ANDROID_SetScreenKeyboardShown(0);
	}
	else
	{
	  SDL_ANDROID_SetScreenKeyboardShown(1);
		SDL_Rect pos_textinput, pos_dpad, pos_button1, pos_button2, pos_button3, pos_button4, pos_button5, pos_button6;
		pos_textinput.x = changed_prefs.pos_x_textinput*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_textinput.y = changed_prefs.pos_y_textinput*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_textinput.h=SDL_ListModes(NULL, 0)[0]->h / (float)10;
		pos_textinput.w=pos_textinput.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, &pos_textinput);
		pos_dpad.x = changed_prefs.pos_x_dpad*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_dpad.y = changed_prefs.pos_y_dpad*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_dpad.h=SDL_ListModes(NULL, 0)[0]->h / (float)2.5;
		pos_dpad.w=pos_dpad.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, &pos_dpad);
		pos_button1.x = changed_prefs.pos_x_button1*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_button1.y = changed_prefs.pos_y_button1*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_button1.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button1.w=pos_button1.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &pos_button1);
		pos_button2.x = changed_prefs.pos_x_button2*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_button2.y = changed_prefs.pos_y_button2*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_button2.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button2.w=pos_button2.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &pos_button2);
		pos_button3.x = changed_prefs.pos_x_button3*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_button3.y = changed_prefs.pos_y_button3*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_button3.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button3.w=pos_button3.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, &pos_button3);
		pos_button4.x = changed_prefs.pos_x_button4*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_button4.y = changed_prefs.pos_y_button4*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_button4.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button4.w=pos_button4.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &pos_button4);
		pos_button5.x = changed_prefs.pos_x_button5*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_button5.y = changed_prefs.pos_y_button5*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_button5.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button5.w=pos_button5.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, &pos_button5);
		pos_button6.x = changed_prefs.pos_x_button6*(SDL_ListModes(NULL, 0)[0]->w/(float)480);
		pos_button6.y = changed_prefs.pos_y_button6*(SDL_ListModes(NULL, 0)[0]->h/(float)360);
		pos_button6.h=SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button6.w=pos_button6.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_5, &pos_button6);

		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, changed_prefs.onScreen_textinput);
		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, changed_prefs.onScreen_dpad);
		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, changed_prefs.onScreen_button1);
		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, changed_prefs.onScreen_button2);
		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, changed_prefs.onScreen_button3);
		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, changed_prefs.onScreen_button4);
		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, changed_prefs.onScreen_button5);
		SDL_ANDROID_SetScreenKeyboardButtonShown(SDL_ANDROID_SCREENKEYBOARD_BUTTON_5, changed_prefs.onScreen_button6);
	}
}
#endif
#endif

// Check if the requested Amiga resolution can be displayed with the current Screen mode as a direct multiple
// Based on this we make the decision to use Linear (smooth) or Nearest Neighbor (pixelated) scaling
bool isModeAspectRatioExact(SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}

static void updatepicasso96()
{
#ifdef PICASSO96
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo;
	vidinfo->rowbytes = 0;
	vidinfo->pixbytes = screen->format->BytesPerPixel;
	vidinfo->rgbformat = 0;
	vidinfo->extra_mem = 1;
	vidinfo->height = screen->h;
	vidinfo->width = screen->w;
	vidinfo->depth = screen->format->BytesPerPixel * 8;
	vidinfo->offset = 0;
#endif
}
static void open_screen(struct uae_prefs* p)
{
	auto* avidinfo = &adisplays.gfxvidinfo;
	graphics_subshutdown();

	if (max_uae_width == 0 || max_uae_height == 0)
	{
		max_uae_width = 1920;
		max_uae_height = 1080;
	}
	
	if (wasfullwindow_a == 0)
		wasfullwindow_a = currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW ? 1 : -1;
	if (wasfullwindow_p == 0)
		wasfullwindow_p = currprefs.gfx_apmode[1].gfx_fullscreen == GFX_FULLWINDOW ? 1 : -1;
	
#if 0
#ifdef ANDROID
	update_onscreen();
#endif
#endif

	update_win_fs_mode(p);
	
#ifdef USE_DISPMANX
	next_synctime = 0;
	current_resource_amigafb = 0;

	write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_OPEN, 1);
	uae_sem_wait(&display_sem);

	vsync_counter = 0;
	current_vsync_frame = 2;
#else
	
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer);

	if (screen_is_picasso)
	{
		if (picasso96_state.RGBFormat == RGBFB_R5G6B5
			|| picasso96_state.RGBFormat == RGBFB_R5G6B5PC
			|| picasso96_state.RGBFormat == RGBFB_CLUT)
		{
			display_depth = 16;
			pixel_format = SDL_PIXELFORMAT_RGB565;
		}
		else
		{
			display_depth = 32;
			pixel_format = SDL_PIXELFORMAT_RGBA32;
		}

		if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		{
			SDL_RenderSetLogicalSize(renderer, display_width, display_height);
			renderQuad = { 0, 0, display_width, display_height };
		}
		else
		{
			SDL_RenderSetLogicalSize(renderer, display_height, display_width);
			renderQuad = { -(display_width - display_height) / 2, (display_width - display_height) / 2, display_width, display_height };
		}
		
		if (isfullscreen() == 0)
			SDL_SetWindowSize(sdl_window, display_width, display_height);
	}
	else
	{
		//display_depth = 16;
		//pixel_format = SDL_PIXELFORMAT_RGB565;
		display_depth = 32;
		pixel_format = SDL_PIXELFORMAT_RGBA32;
		const auto width = display_width * 2 >> p->gfx_resolution;
		const auto height = display_height * 2 >> p->gfx_vresolution;

		if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		{
			SDL_RenderSetLogicalSize(renderer, width, height);
			renderQuad = { 0, 0, width, height };
		}
		else
		{
			SDL_RenderSetLogicalSize(renderer, height, width);
			renderQuad = { -(width - height) / 2, (width - height) / 2, width, height };
		}
		
		if (isfullscreen() == 0)
			SDL_SetWindowSize(sdl_window, width, height);
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
	
	screen = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, display_depth, pixel_format);
	check_error_sdl(screen == nullptr, "Unable to create a surface");

	texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, screen->w, screen->h);
	check_error_sdl(texture == nullptr, "Unable to create texture");

#endif

	setpriority(currprefs.active_capture_priority);
	updatepicasso96();

	if (screen != nullptr)
	{
		allocsoftbuffer(&avidinfo->drawbuffer, display_width, display_height, display_depth);
		notice_screen_contents_lost();
		if (!screen_is_picasso)
		{
			init_row_map();
		}
	}
	init_colors();
	picasso_refresh();

	if (isfullscreen() != 0)
		setmouseactive(-1);
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
			if (new_height * 2 != currprefs.gfx_monitor.gfx_size_win.height)
			{
				display_height = new_height * 2;
				currprefs.gfx_monitor.gfx_size_win.height = new_height * 2;
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
	set_mouse_grab(true);
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
	
	if (!config_changed && !display_change_requested)
		return 0;

	int c2 = 0;
	c2 |= currprefs.gfx_monitor.gfx_size_fs.width != changed_prefs.gfx_monitor.gfx_size_fs.width ? 16 : 0;
	c2 |= currprefs.gfx_monitor.gfx_size_fs.height != changed_prefs.gfx_monitor.gfx_size_fs.height ? 16 : 0;
	c2 |= ((currprefs.gfx_monitor.gfx_size_win.width + 7) & ~7) != ((changed_prefs.gfx_monitor.gfx_size_win.width + 7) & ~7) ? 16 : 0;
	c2 |= currprefs.gfx_monitor.gfx_size_win.height != changed_prefs.gfx_monitor.gfx_size_win.height ? 16 : 0;
#ifdef AMIBERRY
	c2 |= currprefs.gfx_auto_height != changed_prefs.gfx_auto_height ? 16 : 0;
	c2 |= currprefs.gfx_correct_aspect != changed_prefs.gfx_correct_aspect ? 16 : 0;
	c2 |= currprefs.scaling_method != changed_prefs.scaling_method ? 16 : 0;
#endif
	if (c2) {
		c |= c2;
	}

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
		currprefs.gfx_monitor.gfx_size_fs.width = changed_prefs.gfx_monitor.gfx_size_fs.width;
		currprefs.gfx_monitor.gfx_size_fs.height = changed_prefs.gfx_monitor.gfx_size_fs.height;
		currprefs.gfx_monitor.gfx_size_win.width = changed_prefs.gfx_monitor.gfx_size_win.width;
		currprefs.gfx_monitor.gfx_size_win.height = changed_prefs.gfx_monitor.gfx_size_win.height;
		currprefs.gfx_monitor.gfx_size.width = changed_prefs.gfx_monitor.gfx_size.width;
		currprefs.gfx_monitor.gfx_size.height = changed_prefs.gfx_monitor.gfx_size.height;
		
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

		bool unacquired = false;
		//for (int monid = MAX_AMIGAMONITORS - 1; monid >= 0; monid--) {
			//if (!monitors[monid])
				//continue;
			//struct AmigaMonitor* mon = &AMonitors[monid];

			if (c & 64) {
				if (!unacquired) {
					inputdevice_unacquire();
					unacquired = true;
				}
				black_screen_now();
			}
			if (c & 256) {
				init_colors();
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
				target_graphics_buffer_update();
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
				graphics_subshutdown();
				if (currprefs.gfx_api != changed_prefs.gfx_api || currprefs.gfx_api_options != changed_prefs.gfx_api_options) {
					currprefs.gfx_api = changed_prefs.gfx_api;
					currprefs.gfx_api_options = changed_prefs.gfx_api_options;
				}
				graphics_init(dontcapture ? false : true);
			}
		//}

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
		currprefs.allow_host_run != changed_prefs.allow_host_run ||
		currprefs.use_retroarch_quit != changed_prefs.use_retroarch_quit ||
		currprefs.use_retroarch_menu != changed_prefs.use_retroarch_menu ||
		currprefs.use_retroarch_reset != changed_prefs.use_retroarch_reset ||
		currprefs.input_analog_remap != changed_prefs.input_analog_remap ||
		currprefs.kbd_led_num != changed_prefs.kbd_led_num ||
		currprefs.kbd_led_scr != changed_prefs.kbd_led_scr)
	{
		currprefs.leds_on_screen = changed_prefs.leds_on_screen;
		currprefs.keyboard_leds[0] = changed_prefs.keyboard_leds[0];
		currprefs.keyboard_leds[1] = changed_prefs.keyboard_leds[1];
		currprefs.keyboard_leds[2] = changed_prefs.keyboard_leds[2];
		currprefs.input_mouse_untrap = changed_prefs.input_mouse_untrap;
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
		currprefs.allow_host_run = changed_prefs.allow_host_run;
		currprefs.use_retroarch_quit = changed_prefs.use_retroarch_quit;
		currprefs.use_retroarch_menu = changed_prefs.use_retroarch_menu;
		currprefs.use_retroarch_reset = changed_prefs.use_retroarch_reset;
		currprefs.input_analog_remap = changed_prefs.input_analog_remap;
		currprefs.kbd_led_num = changed_prefs.kbd_led_num;
		currprefs.kbd_led_scr = changed_prefs.kbd_led_scr;
		inputdevice_unacquire();
		currprefs.keyboard_leds_in_use = changed_prefs.keyboard_leds_in_use = (currprefs.keyboard_leds[0] | currprefs.keyboard_leds[1] | currprefs.keyboard_leds[2]) != 0;
		pause_sound();
		resume_sound();
		inputdevice_acquire(TRUE);
		setpriority(currprefs.active_capture_priority);
		return 1;
	}

	return 0;
}


int lockscr(struct vidbuffer* vb, bool fullupdate, bool first)
{
	if (screen && SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
	//int pitch;
	//SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&vb->bufmem), &pitch);
	init_row_map();
	return 1;
}


void unlockscr(struct vidbuffer* vb, int y_start, int y_end)
{
	if (screen && SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
	//SDL_UnlockTexture(texture);
}

bool render_screen(bool immediate)
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

void show_screen(int mode)
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
		// RenderPresent must be done in the main thread.
		SDL_RenderPresent(renderer);
		write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SHOW, 1);
	}
	else 
	{
		SDL_UpdateTexture(texture, nullptr, screen->pixels, screen->pitch);
		SDL_RenderClear(renderer);
		SDL_RenderCopyEx(renderer, texture, nullptr, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
		SDL_RenderPresent(renderer);
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

bool show_screen_maybe(const bool show)
{
	if (show)
		show_screen(0);
	return false;
}

void DX_Fill(int dstx, int dsty, int width, int height, uae_u32 color)
{
	SDL_Rect dstrect;
	if (width < 0)
		width = screen->w;
	if (height < 0)
		height = screen->h;
	dstrect.x = dstx;
	dstrect.y = dsty;
	dstrect.w = width;
	dstrect.h = height;
	SDL_FillRect(screen, &dstrect, color);
}

void black_screen_now()
{
#ifndef USE_DISPMANX
	if (amiberry_options.use_sdl2_render_thread)
#endif
		wait_for_display_thread();

	if (screen != nullptr)
	{
		SDL_FillRect(screen, nullptr, 0);
		render_screen(true);
		show_screen(0);
	}
}

static void graphics_subinit()
{
	if (screen == nullptr)
	{
		open_screen(&currprefs);
		if (screen == nullptr)
			write_log("Unable to set video mode: %s\n", SDL_GetError());
	}
}

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

void init_colors()
{
	/* Truecolor: */
	red_bits = bits_in_mask(screen->format->Rmask);
	green_bits = bits_in_mask(screen->format->Gmask);
	blue_bits = bits_in_mask(screen->format->Bmask);
	red_shift = mask_shift(screen->format->Rmask);
	green_shift = mask_shift(screen->format->Gmask);
	blue_shift = mask_shift(screen->format->Bmask);
	alpha_bits = bits_in_mask(screen->format->Amask);
	alpha_shift = mask_shift(screen->format->Amask);

	alloc_colors64k(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, alpha_bits, alpha_shift, alpha, 0, false);
	notice_new_xcolors();
}

/*
* Find the colour depth of the display
*/
static int get_display_depth()
{
	return screen->format->BytesPerPixel * 8;
}

int GetSurfacePixelFormat()
{
	const auto depth = get_display_depth();
	const auto unit = depth + 1 & 0xF8;

	return unit == 8
		? RGBFB_CHUNKY
		: depth == 15 && unit == 16
		? RGBFB_R5G5B5
		: depth == 16 && unit == 16
		? RGBFB_R5G6B5
		: unit == 24
		? RGBFB_R8G8B8
		: unit == 32
		? RGBFB_R8G8B8A8
		: RGBFB_NONE;
}

int graphics_init(bool mousecapture)
{
	inputdevice_unacquire();
	graphics_subinit();
	init_colors();

	inputdevice_acquire(TRUE);
	return 1;
}

void graphics_leave()
{
	graphics_subshutdown();

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
#ifdef USE_DISPMANX
	bcm_host_deinit();
#else
	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
#endif
	
	if (renderer)
	{
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}

	if (sdl_window)
	{
		SDL_DestroyWindow(sdl_window);
		sdl_window = nullptr;
	}

	SDL_VideoQuit();
}

#define  SYSTEM_RED_SHIFT      (screen->format->Rshift)
#define  SYSTEM_GREEN_SHIFT    (screen->format->Gshift)
#define  SYSTEM_BLUE_SHIFT     (screen->format->Bshift)
#define  SYSTEM_RED_MASK       (screen->format->Rmask)
#define  SYSTEM_GREEN_MASK     (screen->format->Gmask)
#define  SYSTEM_BLUE_MASK      (screen->format->Bmask)

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

	if (screen != nullptr) {
	current_screenshot = SDL_CreateRGBSurfaceFrom(screen->pixels,
		screen->w,
		screen->h,
		screen->format->BitsPerPixel,
		screen->pitch,
		screen->format->Rmask,
		screen->format->Gmask,
		screen->format->Bmask,
		screen->format->Amask);
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

static int currVSyncRate = 0;
bool vsync_switchmode(int hz)
{
	if (hz >= 55)
		hz = 60;
	else
		hz = 50;

	if (hz != currVSyncRate)
	{
		currVSyncRate = hz;
		fpscounter_reset();
#ifdef USE_DISPMANX		
		time_per_frame = 1000 * 1000 / (hz);

		if (hz == host_hz)
			vsync_modulo = 1;
		else if (hz > host_hz)
			vsync_modulo = 6; // Amiga draws 6 frames while host has 5 vsyncs -> sync every 6th Amiga frame
		else
			vsync_modulo = 5; // Amiga draws 5 frames while host has 6 vsyncs -> sync every 5th Amiga frame
#endif
	}

	return true;
}

bool target_graphics_buffer_update()
{
	auto rate_changed = false;

	if (currprefs.gfx_monitor.gfx_size.height != changed_prefs.gfx_monitor.gfx_size.height)
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

	return true;
}

#ifdef PICASSO96

int picasso_palette(struct MyCLUTEntry *CLUT, uae_u32 *clut)
{
	auto changed = 0;

	for (auto i = 0; i < 256; i++) {
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

static void sortmodes()
{
	auto i = 0, idx = -1;
	unsigned int pw = -1, ph = -1;
	while (DisplayModes[i].depth >= 0)
		i++;
	qsort(DisplayModes, i, sizeof(struct PicassoResolution), resolution_compare);
	for (i = 0; DisplayModes[i].depth >= 0; i++)
	{
		if (DisplayModes[i].res.height != ph || DisplayModes[i].res.width != pw)
		{
			ph = DisplayModes[i].res.height;
			pw = DisplayModes[i].res.width;
			idx++;
		}
		DisplayModes[i].residx = idx;
	}
}

static void modes_list()
{
	auto i = 0;
	while (DisplayModes[i].depth >= 0)
	{
		write_log("%d: %s (", i, DisplayModes[i].name);
		auto j = 0;
		while (DisplayModes[i].refresh[j] > 0)
		{
			if (j > 0)
				write_log(",");
			write_log("%d", DisplayModes[i].refresh[j]);
			j++;
		}
		write_log(")\n");
		i++;
	}
}

void picasso_init_resolutions()
{
	auto count = 0;
	char tmp[200];
	int bits[] = { 8, 16, 32 };

	Displays[0].primary = 1;
	Displays[0].rect.left = 0;
	Displays[0].rect.top = 0;
	Displays[0].rect.right = 800;
	Displays[0].rect.bottom = 600;
	sprintf(tmp, "%s (%d*%d)", "Display", Displays[0].rect.right, Displays[0].rect.bottom);
	Displays[0].fullname = my_strdup(tmp);
	Displays[0].monitorname = my_strdup("Display");

	auto* const md1 = Displays;
	DisplayModes = md1->DisplayModes = xmalloc(struct PicassoResolution, MAX_PICASSO_MODES);
	for (auto i = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; i++)
	{
		for (auto bitdepth : bits)
		{
			const auto bit_unit = bitdepth + 1 & 0xF8;
			const auto rgbFormat =
				bitdepth == 8 ? RGBFB_CLUT :
				bitdepth == 16 ? RGBFB_R5G6B5 :
				bitdepth == 24 ? RGBFB_R8G8B8 : RGBFB_R8G8B8A8;
			auto pixelFormat = 1 << rgbFormat;
			pixelFormat |= RGBFF_CHUNKY;
			DisplayModes[count].res.width = x_size_table[i];
			DisplayModes[count].res.height = y_size_table[i];
			DisplayModes[count].depth = bit_unit >> 3;
			DisplayModes[count].refresh[0] = 50;
			DisplayModes[count].refresh[1] = 60;
			DisplayModes[count].refresh[2] = 0;
			DisplayModes[count].colormodes = pixelFormat;
			sprintf(DisplayModes[count].name, "%dx%d, %d-bit",
				DisplayModes[count].res.width, DisplayModes[count].res.height, DisplayModes[count].depth * 8);

			count++;
		}
	}
	DisplayModes[count].depth = -1;
	sortmodes();
	modes_list();
	DisplayModes = Displays[0].DisplayModes;
}
#endif

#ifdef PICASSO96
void gfx_set_picasso_state(int on)
{
	if (screen_is_picasso == on)
		return;
	screen_is_picasso = on;

	black_screen_now();
	open_screen(&currprefs);
}

void gfx_set_picasso_modeinfo(uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{
	if (!screen_is_picasso)
		return;
	black_screen_now();
	gfx_set_picasso_colors(rgbfmt);

	if (static_cast<unsigned>(picasso_vidinfo.width) == w &&
		static_cast<unsigned>(picasso_vidinfo.height) == h &&
		static_cast<unsigned>(picasso_vidinfo.depth) == depth &&
		picasso_vidinfo.selected_rgbformat == rgbfmt)
		return;

	picasso_vidinfo.selected_rgbformat = rgbfmt;
	picasso_vidinfo.width = w;
	picasso_vidinfo.height = h;
	picasso_vidinfo.depth = depth;
	picasso_vidinfo.extra_mem = 1;
	picasso_vidinfo.rowbytes = screen->pitch;
	picasso_vidinfo.pixbytes = screen->format->BytesPerPixel;
	picasso_vidinfo.offset = 0;

	if (screen_is_picasso)
		open_screen(&currprefs);
}

void gfx_set_picasso_colors(RGBFTYPE rgbfmt)
{
	alloc_colors_picasso(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt, p96_rgbx16);
}

uae_u8* gfx_lock_picasso(bool fullupdate, bool doclear)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo;
	static uae_u8* p;
	if (screen == nullptr || screen_is_picasso == 0)
		return nullptr;
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	vidinfo->pixbytes = screen->format->BytesPerPixel;
	vidinfo->rowbytes = screen->pitch;
	p = static_cast<uae_u8*>(screen->pixels);
	if (!p)
	{
		if (SDL_MUSTLOCK(screen))
			SDL_UnlockSurface(screen);
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

void gfx_unlock_picasso(const bool dorender)
{
	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	if (dorender)
	{
		render_screen(true);
		show_screen(0);
	}
}

#endif // PICASSO96

float target_getcurrentvblankrate()
{
	return static_cast<float>(host_hz);
}