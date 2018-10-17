#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "savestate.h"
#include "picasso96.h"
#include "amiberry_gfx.h"

#include <png.h>
#include <SDL.h>
#include <cmath>
#ifdef USE_SDL1
#include <SDL_image.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_ttf.h>
#endif
#ifdef ANDROIDSDL
#include <SDL_screenkeyboard.h>
#endif
#ifdef USE_DISPMANX
#include "bcm_host.h"
#include "threaddep/thread.h"

#define DISPLAY_SIGNAL_SETUP 				1
#define DISPLAY_SIGNAL_SUBSHUTDOWN 			2
#define DISPLAY_SIGNAL_OPEN 				3
#define DISPLAY_SIGNAL_SHOW 				4
#define DISPLAY_SIGNAL_QUIT 				5
static uae_thread_id display_tid = nullptr;
static smp_comm_pipe *volatile display_pipe = nullptr;
static uae_sem_t display_sem = nullptr;
static bool volatile display_thread_busy = false;
#endif

#ifdef ANDROIDSDL
#include <android/log.h>
#endif

static int display_width;
static int display_height;
bool can_have_linedouble;

/* SDL Surface for output of emulation */
SDL_Surface* screen = nullptr;
#if (defined USE_SDL2) && (defined USE_RENDER_THREAD)
SDL_Thread * renderthread = nullptr;
#endif
#ifdef USE_SDL2
const char* sdl_video_driver;
#endif

#ifdef USE_DISPMANX
static unsigned int current_vsync_frame = 0;
#endif
unsigned long time_per_frame = 20000; // Default for PAL (50 Hz): 20000 microsecs
static unsigned long last_synctime;
static int vsync_modulo = 1;
static int host_hz = 50;

/* Possible screen modes (x and y resolutions) */
#define MAX_SCREEN_MODES 14
static int x_size_table[MAX_SCREEN_MODES] = { 640, 640, 720, 800, 800, 960, 1024, 1280, 1280, 1280, 1360, 1366, 1680, 1920 };
static int y_size_table[MAX_SCREEN_MODES] = { 400, 480, 400, 480, 600, 540, 768, 720, 800, 1024, 768, 768, 1050, 1080 };

struct PicassoResolution* DisplayModes;
struct MultiDisplay Displays[MAX_DISPLAYS];

int screen_is_picasso = 0;

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

DISPMANX_DISPLAY_HANDLE_T   dispmanxdisplay;
DISPMANX_MODEINFO_T         dispmanxdinfo;
DISPMANX_RESOURCE_HANDLE_T  dispmanxresource_amigafb_1 = 0;
DISPMANX_RESOURCE_HANDLE_T  dispmanxresource_amigafb_2 = 0;
DISPMANX_RESOURCE_HANDLE_T  dispmanxresource_blackfb = 0;
DISPMANX_ELEMENT_HANDLE_T   dispmanxelement;
DISPMANX_ELEMENT_HANDLE_T	blackscreenelement;
DISPMANX_UPDATE_HANDLE_T    dispmanxupdate;
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

static void *display_thread(void *unused)
{
	VC_DISPMANX_ALPHA_T alpha = {
		DISPMANX_FLAGS_ALPHA_T(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
		255 /*alpha 0->255*/ , 	0
	};
	uint32_t vc_image_ptr;
#ifdef USE_SDL1
	SDL_Surface *dummy_screen;
#endif
	int width, height, depth;
	float want_aspect, real_aspect, scale;
	SDL_Rect viewport;

	for (;;) {
		display_thread_busy = false;
		auto signal = read_comm_pipe_u32_blocking(display_pipe);
		display_thread_busy = true;
		
		switch (signal) {
		case DISPLAY_SIGNAL_SETUP:
			bcm_host_init();
			dispmanxdisplay = vc_dispmanx_display_open(0);
			vc_dispmanx_vsync_callback(dispmanxdisplay, vsync_callback, nullptr);
			break;

		case DISPLAY_SIGNAL_SUBSHUTDOWN:
			if (DispManXElementpresent == 1)
			{
				DispManXElementpresent = 0;
				dispmanxupdate = vc_dispmanx_update_start(0);
				vc_dispmanx_element_remove(dispmanxupdate, dispmanxelement);
				vc_dispmanx_element_remove(dispmanxupdate, blackscreenelement);
				vc_dispmanx_update_submit_sync(dispmanxupdate);
			}

			if (dispmanxresource_amigafb_1 != 0) {
				vc_dispmanx_resource_delete(dispmanxresource_amigafb_1);
				dispmanxresource_amigafb_1 = 0;
			}
			if (dispmanxresource_amigafb_2 != 0) {
				vc_dispmanx_resource_delete(dispmanxresource_amigafb_2);
				dispmanxresource_amigafb_2 = 0;
			}
			if (dispmanxresource_blackfb != 0)
			{
				vc_dispmanx_resource_delete(dispmanxresource_blackfb);
				dispmanxresource_blackfb = 0;
			}

			if (screen != nullptr)
			{
				SDL_FreeSurface(screen);
				screen = nullptr;
			}

			uae_sem_post(&display_sem);
			break;

		case DISPLAY_SIGNAL_OPEN:
			width = display_width;
			height = display_height;
			if (screen_is_picasso)
			{
				if (picasso96_state.RGBFormat == RGBFB_R5G6B5 || picasso96_state.RGBFormat == RGBFB_CLUT)
				{
					depth = 16;
					rgb_mode = VC_IMAGE_RGB565;
				}
				else 
				{
					depth = 32;
					rgb_mode = VC_IMAGE_RGBA32;
				}	
			}
			else
			{
				depth = 16;
				rgb_mode = VC_IMAGE_RGB565;
			}

#ifdef USE_SDL1
			dummy_screen = SDL_SetVideoMode(width, height, depth, SDL_SWSURFACE | SDL_FULLSCREEN);
			screen = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, depth,
				dummy_screen->format->Rmask, dummy_screen->format->Gmask, dummy_screen->format->Bmask, dummy_screen->format->Amask);
			SDL_FreeSurface(dummy_screen);
#elif USE_SDL2
			screen = SDL_CreateRGBSurface(0, display_width, display_height, depth, 0, 0, 0, 0);
#endif
			vc_dispmanx_display_get_info(dispmanxdisplay, &dispmanxdinfo);

			dispmanxresource_amigafb_1 = vc_dispmanx_resource_create(rgb_mode, width, height, &vc_image_ptr);
			dispmanxresource_amigafb_2 = vc_dispmanx_resource_create(rgb_mode, width, height, &vc_image_ptr);
			dispmanxresource_blackfb = vc_dispmanx_resource_create(rgb_mode, width, height, &vc_image_ptr);

			vc_dispmanx_rect_set(&blit_rect, 0, 0, width, height);
			vc_dispmanx_resource_write_data(dispmanxresource_amigafb_1, rgb_mode, screen->pitch, screen->pixels, &blit_rect);
			vc_dispmanx_resource_write_data(dispmanxresource_blackfb, rgb_mode, screen->pitch, screen->pixels, &blit_rect);
			vc_dispmanx_rect_set(&src_rect, 0, 0, width << 16, height << 16);

			// Use the full screen size for the black frame
			vc_dispmanx_rect_set(&black_rect, 0, 0, dispmanxdinfo.width, dispmanxdinfo.height);

			// Correct Aspect Ratio
			if (changed_prefs.gfx_correct_aspect == 0)
			{
				// Fullscreen.
				vc_dispmanx_rect_set(&dst_rect, 0, 0, dispmanxdinfo.width, dispmanxdinfo.height);
			}
			else
			{
				if (screen_is_picasso)
					height = display_height;
				else
					height = display_height * 2 >> changed_prefs.gfx_vresolution;

				want_aspect = float(width) / float(height);
				real_aspect = float(dispmanxdinfo.width) / float(dispmanxdinfo.height);

				if (want_aspect > real_aspect)
				{
					scale = float(dispmanxdinfo.width) / float(display_width);
					viewport.x = 0;
					viewport.w = dispmanxdinfo.width;
					viewport.h = int(std::ceil(height * scale));
					viewport.y = (dispmanxdinfo.height - viewport.h) / 2;
				}
				else
				{
					scale = float(dispmanxdinfo.height) / float(height);
					viewport.y = 0;
					viewport.h = dispmanxdinfo.height;
					viewport.w = int(std::ceil(display_width * scale));
					viewport.x = (dispmanxdinfo.width - viewport.w) / 2;
				}

				vc_dispmanx_rect_set(&dst_rect, viewport.x, viewport.y, viewport.w, viewport.h);
			}

			if (DispManXElementpresent == 0)
			{
				DispManXElementpresent = 1;
				dispmanxupdate = vc_dispmanx_update_start(0);
				blackscreenelement = vc_dispmanx_element_add(dispmanxupdate, dispmanxdisplay, 0,
					&black_rect, dispmanxresource_blackfb, &src_rect, DISPMANX_PROTECTION_NONE, &alpha, 
					nullptr, DISPMANX_NO_ROTATE);

				dispmanxelement = vc_dispmanx_element_add(dispmanxupdate, dispmanxdisplay, 10,               // layer
					&dst_rect, dispmanxresource_amigafb_1, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
					nullptr,             // clamp
					DISPMANX_NO_ROTATE);

				vc_dispmanx_update_submit(dispmanxupdate, nullptr, nullptr);
			}
			uae_sem_post(&display_sem);
			break;

		case DISPLAY_SIGNAL_SHOW:
			if (current_resource_amigafb == 1)
			{
				current_resource_amigafb = 0;
				vc_dispmanx_resource_write_data(dispmanxresource_amigafb_1,
					rgb_mode,
					screen->pitch,
					screen->pixels,
					&blit_rect);
				dispmanxupdate = vc_dispmanx_update_start(0);
				vc_dispmanx_element_change_source(dispmanxupdate, dispmanxelement, dispmanxresource_amigafb_1);
			}
			else
			{
				current_resource_amigafb = 1;
				vc_dispmanx_resource_write_data(dispmanxresource_amigafb_2,
					rgb_mode,
					screen->pitch,
					screen->pixels,
					&blit_rect);
				dispmanxupdate = vc_dispmanx_update_start(0);
				vc_dispmanx_element_change_source(dispmanxupdate, dispmanxelement, dispmanxresource_amigafb_2);
			}
			vc_dispmanx_update_submit(dispmanxupdate, nullptr, nullptr);
			break;

		case DISPLAY_SIGNAL_QUIT:
			vc_dispmanx_vsync_callback(dispmanxdisplay, nullptr, nullptr);
			vc_dispmanx_display_close(dispmanxdisplay);
			bcm_host_deinit();
			display_tid = nullptr;
			return nullptr;
		default: 
			break;
		}
	}
}
#endif

int graphics_setup(void)
{
#ifdef PICASSO96
	picasso_init_resolutions();
	InitPicasso96();
#endif
#ifdef USE_SDL1
	const SDL_VideoInfo* sdl_video_info = SDL_GetVideoInfo();
	can_have_linedouble = sdl_video_info->current_h >= 540;

	VCHI_INSTANCE_T vchi_instance;
	VCHI_CONNECTION_T *vchi_connection;
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
				const float frame_rate = property.param1 == HDMI_PIXEL_CLOCK_TYPE_NTSC ? tvstate.display.hdmi.frame_rate * (1000.0f / 1001.0f) : tvstate.display.hdmi.frame_rate;
				host_hz = int(frame_rate);
			}
			vc_vchi_tv_stop();
			vchi_disconnect(vchi_instance);
		}
	}
#elif USE_SDL2
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		abort();
	}

	sdl_video_driver = SDL_GetCurrentVideoDriver();
	Uint32 sdl_window_mode;

	if (strcmp(sdl_video_driver,"x11") == 0)
	{
		// Only enable Windowed mode if we're running under x11
		sdl_window_mode = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	}
	else
	{
		// otherwise go for Fullscreen
		sdl_window_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	if (sdlWindow == nullptr)
	{
		sdlWindow = SDL_CreateWindow("Amiberry",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			800,
			480,
			sdl_window_mode);
		check_error_sdl(sdlWindow == nullptr, "Unable to create window");		
	}
	
	if (SDL_GetWindowDisplayMode(sdlWindow, &sdlMode) != 0)
	{
		SDL_Log("Could not get information about SDL Mode! SDL_Error: %s\n", SDL_GetError());
	}
	can_have_linedouble = sdlMode.h >= 540;

	if (renderer == nullptr)
	{
		renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(renderer == nullptr, "Unable to create a renderer");
	}

	if (SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1") != SDL_TRUE)
		SDL_Log("SDL could not grab the keyboard");

	SDL_ShowCursor(SDL_DISABLE);

	SDL_DisplayMode current;
	const auto should_be_zero = SDL_GetCurrentDisplayMode(0, &current);
	if (should_be_zero == 0)
		host_hz = current.refresh_rate;
#endif

	currprefs.gfx_apmode[1].gfx_refreshrate = host_hz;

#ifdef USE_DISPMANX
	if (display_pipe == nullptr) {
		display_pipe = xmalloc(smp_comm_pipe, 1);
		init_comm_pipe(display_pipe, 20, 1);
	}
	if (display_sem == nullptr) {
		uae_sem_init(&display_sem, 0, 0);
	}
	if (display_tid == nullptr && display_pipe != nullptr && display_sem != nullptr) {
		uae_start_thread(_T("render"), display_thread, nullptr, &display_tid);
	}
	write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SETUP, 1);
#endif

	return 1;
}

void toggle_fullscreen()
{
#ifdef USE_SDL2
	const Uint32 fullscreen_flag = SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (sdlWindow)
	{
		const bool is_fullscreen = SDL_GetWindowFlags(sdlWindow) & fullscreen_flag;
		SDL_SetWindowFullscreen(sdlWindow, is_fullscreen ? 0 : fullscreen_flag);
		SDL_ShowCursor(is_fullscreen);
	}
#endif
}

#ifdef USE_DISPMANX
static void wait_for_display_thread(void)
{
	while(display_thread_busy)
		usleep(10);
}
#endif

void InitAmigaVidMode(struct uae_prefs* p)
{
	/* Initialize structure for Amiga video modes */
	gfxvidinfo.drawbuffer.pixbytes = screen->format->BytesPerPixel;
	gfxvidinfo.drawbuffer.bufmem = static_cast<uae_u8*>(screen->pixels);
	gfxvidinfo.drawbuffer.outwidth = p->gfx_size.width;
	gfxvidinfo.drawbuffer.outheight = p->gfx_size.height << p->gfx_vresolution;
	gfxvidinfo.drawbuffer.rowbytes = screen->pitch;
}

void graphics_subshutdown()
{
#ifdef USE_DISPMANX
	if (display_tid != nullptr) {
		wait_for_display_thread();
		write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SUBSHUTDOWN, 1);
		uae_sem_wait(&display_sem);
	}
#elif USE_SDL2
#if (defined USE_RENDER_THREAD)
	if (renderthread)
	{
		SDL_WaitThread(renderthread, NULL); 
		renderthread = NULL;
	}
#endif

	if (screen != nullptr)
	{
		SDL_FreeSurface(screen);
		screen = nullptr;
	}
	if (texture != nullptr)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
#endif
}

#ifdef ANDROIDSDL
void update_onscreen()
{
	SDL_ANDROID_SetScreenKeyboardFloatingJoystick(changed_prefs.floatingJoystick);
	if (changed_prefs.onScreen == 0)
	{
		SDL_ANDROID_SetScreenKeyboardShown(0);
	}
	else
	{
		SDL_ANDROID_SetScreenKeyboardShown(1);
		SDL_Rect pos_textinput, pos_dpad, pos_button1, pos_button2, pos_button3, pos_button4, pos_button5, pos_button6;
		pos_textinput.x = changed_prefs.pos_x_textinput*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_textinput.y = changed_prefs.pos_y_textinput*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_textinput.h = SDL_ListModes(NULL, 0)[0]->h / (float)10;
		pos_textinput.w = pos_textinput.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, &pos_textinput);
		pos_dpad.x = changed_prefs.pos_x_dpad*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_dpad.y = changed_prefs.pos_y_dpad*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_dpad.h = SDL_ListModes(NULL, 0)[0]->h / (float)2.5;
		pos_dpad.w = pos_dpad.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, &pos_dpad);
		pos_button1.x = changed_prefs.pos_x_button1*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_button1.y = changed_prefs.pos_y_button1*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_button1.h = SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button1.w = pos_button1.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &pos_button1);
		pos_button2.x = changed_prefs.pos_x_button2*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_button2.y = changed_prefs.pos_y_button2*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_button2.h = SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button2.w = pos_button2.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &pos_button2);
		pos_button3.x = changed_prefs.pos_x_button3*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_button3.y = changed_prefs.pos_y_button3*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_button3.h = SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button3.w = pos_button3.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, &pos_button3);
		pos_button4.x = changed_prefs.pos_x_button4*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_button4.y = changed_prefs.pos_y_button4*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_button4.h = SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button4.w = pos_button4.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &pos_button4);
		pos_button5.x = changed_prefs.pos_x_button5*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_button5.y = changed_prefs.pos_y_button5*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_button5.h = SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button5.w = pos_button5.h;
		SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, &pos_button5);
		pos_button6.x = changed_prefs.pos_x_button6*(SDL_ListModes(NULL, 0)[0]->w / (float)480);
		pos_button6.y = changed_prefs.pos_y_button6*(SDL_ListModes(NULL, 0)[0]->h / (float)360);
		pos_button6.h = SDL_ListModes(NULL, 0)[0]->h / (float)5;
		pos_button6.w = pos_button6.h;
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

#ifdef USE_SDL2
// Check if the requested Amiga resolution can be displayed with the current Screen mode as a direct multiple
// Based on this we make the decision to use Linear (smooth) or Nearest Neighbor (pixelated) scaling
bool isModeAspectRatioExact(SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}
#endif

static void open_screen(struct uae_prefs* p)
{
	graphics_subshutdown();

	if (max_uae_width == 0 || max_uae_height == 0)
	{
		max_uae_width = 1920;
		max_uae_height = 1080;
	}

#ifdef ANDROIDSDL
	update_onscreen();
#endif
	if (screen_is_picasso)
	{
		display_width = picasso_vidinfo.width ? picasso_vidinfo.width : 640;
		display_height = picasso_vidinfo.height ? picasso_vidinfo.height : 256;
#ifdef USE_SDL2
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"); // we always use linear for Picasso96 modes
#endif
	}
	else
	{
		p->gfx_resolution = p->gfx_size.width ? (p->gfx_size.width > 600 ? 1 : 0) : 1;
		display_width = p->gfx_size.width ? p->gfx_size.width : 640;
		display_height = (p->gfx_size.height ? p->gfx_size.height : 256) << p->gfx_vresolution;

#ifdef USE_SDL2
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
#endif
	}

#ifdef USE_DISPMANX
	next_synctime = 0;
	current_resource_amigafb = 0;
	write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_OPEN, 1);
	uae_sem_wait(&display_sem);

	vsync_counter = 0;
	current_vsync_frame = 2;

#elif USE_SDL2
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer);

	if (sdlWindow && strcmp(sdl_video_driver, "x11") == 0)
	{
		const bool is_fullscreen = SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP;
		if (p->gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN)
		{
			// Switch to Fullscreen mode, if we don't have it already
			if (!is_fullscreen)
				SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
			// Switch to Window mode, if we don't have it already
			if (is_fullscreen)
				SDL_SetWindowFullscreen(sdlWindow, 0);
		}

		if (!is_fullscreen)
			if ((SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_MAXIMIZED) == 0)
			{
				if (screen_is_picasso)
					SDL_SetWindowSize(sdlWindow, display_width, display_height);
				else
					SDL_SetWindowSize(sdlWindow, display_width, display_height * 2 >> p->gfx_vresolution);
			}	
	}

	int depth;
	Uint32 pixel_format;

	if (screen_is_picasso)
	{
		if (picasso96_state.RGBFormat == RGBFB_R5G6B5 || picasso96_state.RGBFormat == RGBFB_CLUT)
		{
			depth = 16;
			pixel_format = SDL_PIXELFORMAT_RGB565;
		}
		else
		{
			depth = 32;
			pixel_format = SDL_PIXELFORMAT_RGBA32;
		}
	}
	else
	{
		depth = 16;
		pixel_format = SDL_PIXELFORMAT_RGB565;
	}

	screen = SDL_CreateRGBSurface(0, display_width, display_height, depth, 0, 0, 0, 0);
	check_error_sdl(screen == nullptr, "Unable to create a surface");

	if (screen_is_picasso)
		SDL_RenderSetLogicalSize(renderer, display_width, display_height);
	else
		SDL_RenderSetLogicalSize(renderer, display_width, (display_height * 2) >> p->gfx_vresolution);

	texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, screen->w, screen->h);
	check_error_sdl(texture == nullptr, "Unable to create texture");

#endif

	if (screen != nullptr)
	{
		InitAmigaVidMode(p);
		init_row_map();
	}
}

void update_display(struct uae_prefs* p)
{
	open_screen(p);
#ifdef USE_SDL2
	SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
	SDL_ShowCursor(SDL_DISABLE);
	framecnt = 1; // Don't draw frame before reset done
}

int check_prefs_changed_gfx()
{
	auto changed = 0;

	if (currprefs.gfx_size.height != changed_prefs.gfx_size.height ||
		currprefs.gfx_size.width != changed_prefs.gfx_size.width ||
		currprefs.gfx_resolution != changed_prefs.gfx_resolution ||
		currprefs.gfx_vresolution != changed_prefs.gfx_vresolution ||
		currprefs.gfx_correct_aspect != changed_prefs.gfx_correct_aspect)
	{
		cfgfile_configuration_change(1);
		currprefs.gfx_size.height = changed_prefs.gfx_size.height;
		currprefs.gfx_size.width = changed_prefs.gfx_size.width;
		currprefs.gfx_resolution = changed_prefs.gfx_resolution;
		currprefs.gfx_vresolution = changed_prefs.gfx_vresolution;
		currprefs.gfx_correct_aspect = changed_prefs.gfx_correct_aspect;
		update_display(&currprefs);
		changed = 1;
	}
	if (currprefs.leds_on_screen != changed_prefs.leds_on_screen ||
		currprefs.hide_idle_led != changed_prefs.hide_idle_led || 
		currprefs.vertical_offset != changed_prefs.vertical_offset)
	{
		currprefs.leds_on_screen = changed_prefs.leds_on_screen;
		currprefs.hide_idle_led = changed_prefs.hide_idle_led;
		currprefs.vertical_offset = changed_prefs.vertical_offset;
		changed = 1;
	}
	if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate)
	{
		currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
		changed = 1;
	}

	currprefs.filesys_limit = changed_prefs.filesys_limit;
	currprefs.harddrive_read_only = changed_prefs.harddrive_read_only;

	if (changed)
		init_custom();

	return changed;
}


int lockscr()
{
	return 1;
}


void unlockscr()
{

}

#ifdef USE_DISPMANX
void wait_for_vsync()
{
	const auto start = read_processor_time();
	const auto wait_till = current_vsync_frame;
	do
	{
		usleep(10);
		current_vsync_frame = vsync_counter;
	} while (wait_till >= current_vsync_frame && read_processor_time() - start < 20000);
}
#endif

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

#if (defined USE_SDL2) && (defined USE_RENDER_THREAD)
// All the moving and copying of data, happens here.
int sdl2_render_thread(void *ptr) {
	if (texture == NULL || renderer == NULL || screen == NULL) {
		return 0;
	}

	SDL_UpdateTexture(texture, nullptr, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	return 0;
}
#endif

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

	current_vsync_frame += currprefs.gfx_framerate;
#endif

#ifdef USE_DISPMANX
	wait_for_display_thread();
	write_comm_pipe_u32(display_pipe, DISPLAY_SIGNAL_SHOW, 1);
#elif USE_SDL2
#if (defined USE_RENDER_THREAD)
	// Wait for the last thread to finish before rendering it.
	SDL_WaitThread(renderthread, NULL); 
	renderthread = NULL;
	// RenderPresent must be done in the main thread.
	SDL_RenderPresent(renderer);
	// Then start the next render thread.
	renderthread = SDL_CreateThread(sdl2_render_thread, "AmigaScreen", nullptr);
#else
	SDL_UpdateTexture(texture, nullptr, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
#endif //USE_RENDER_THREAD
	
#endif //USE_SDL2

	last_synctime = read_processor_time();
	idletime += last_synctime - start;

#ifdef USE_DISPMANX
	if (last_synctime - next_synctime > time_per_frame - 5000)
		next_synctime = last_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
	else
		next_synctime = next_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
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

void black_screen_now()
{
#if (defined USE_SDL2) && (defined USE_RENDER_THREAD)
	if (renderthread)
	{
		SDL_WaitThread(renderthread, NULL); 
		renderthread = NULL;
	}
#endif
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
			fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
	}
	else
	{
		SDL_ShowCursor(SDL_DISABLE);
		InitAmigaVidMode(&currprefs);
	}
}

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

static int init_colors()
{
	/* Truecolor: */
	red_bits = bits_in_mask(screen->format->Rmask);
	green_bits = bits_in_mask(screen->format->Gmask);
	blue_bits = bits_in_mask(screen->format->Bmask);
	red_shift = mask_shift(screen->format->Rmask);
	green_shift = mask_shift(screen->format->Gmask);
	blue_shift = mask_shift(screen->format->Bmask);
	alloc_colors64k(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, 0);
	notice_new_xcolors();

	return 1;
}

/*
* Find the colour depth of the display
*/
static int get_display_depth()
{
#ifdef USE_SDL1
	const SDL_VideoInfo *vid_info;
	auto depth = 0;

	if ((vid_info = SDL_GetVideoInfo()))
	{
		depth = vid_info->vfmt->BitsPerPixel;

		/* Don't trust the answer if it's 16 bits; the display
		* could actually be 15 bits deep. We'll count the bits
		* ourselves */
		if (depth == 16)
			depth = bits_in_mask(vid_info->vfmt->Rmask) + bits_in_mask(vid_info->vfmt->Gmask) + bits_in_mask(vid_info->vfmt->Bmask);
	}
#elif USE_SDL2
	const int depth = screen->format->BytesPerPixel == 4 ? 32 : 16;
#endif
	return depth;
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
		? RGBFB_B8G8R8
		: unit == 32
		? RGBFB_R8G8B8A8
		: RGBFB_NONE;
}

int graphics_init(bool mousecapture)
{
	graphics_subinit();

	if (!init_colors())
		return 0;

	return 1;
}

void graphics_leave()
{
	graphics_subshutdown();

#ifdef USE_DISPMANX
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
#endif
#ifdef  USE_SDL2
	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}

	if (renderer)
	{
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}

	if (sdlWindow)
	{
		SDL_DestroyWindow(sdlWindow);
		sdlWindow = nullptr;
	}
#endif

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
	const auto pix = static_cast<unsigned char *>(surface->pixels);
	unsigned char writeBuffer[1024 * 3];
	const auto f = fopen(path, "wbe");
	if (!f) return 0;
	auto png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		nullptr,
		nullptr,
		nullptr);
	if (!png_ptr)
	{
		fclose(f);
		return 0;
	}

	auto info_ptr = png_create_info_struct(png_ptr);

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

	auto b = writeBuffer;

	const auto sizeX = w;
	const auto sizeY = h;

	auto p = reinterpret_cast<unsigned short *>(pix);
	for (auto y = 0; y < sizeY; y++)
	{
		for (auto x = 0; x < sizeX; x++)
		{
			auto v = p[x];

			*b++ = ((v & SYSTEM_RED_MASK) >> SYSTEM_RED_SHIFT) << 3; // R
			*b++ = ((v & SYSTEM_GREEN_MASK) >> SYSTEM_GREEN_SHIFT) << 2; // G
			*b++ = ((v & SYSTEM_BLUE_MASK) >> SYSTEM_BLUE_SHIFT) << 3; // B
		}
		p += surface->pitch / 2;
		png_write_row(png_ptr, writeBuffer);
		b = writeBuffer;
	}

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(f);
	return 1;
}

static void create_screenshot()
{
#if (defined USE_SDL2) && (defined USE_RENDER_THREAD)
	if (renderthread)
	{
		SDL_WaitThread(renderthread, NULL);
		renderthread = NULL;
	}
#endif
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
#if (defined USE_SDL2) && (defined USE_RENDER_THREAD)
	if (renderthread)
	{
		SDL_WaitThread(renderthread, NULL);
		renderthread = NULL;
	}
#endif
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
	auto changed_height = changed_prefs.gfx_size.height;

	if (hz >= 55)
		hz = 60;
	else
		hz = 50;

	if (hz == 50 && currVSyncRate == 60)
	{
		// Switch from NTSC -> PAL
		switch (changed_height)
		{
		case 200: changed_height = 240; break;
		case 216: changed_height = 262; break;
		case 240: changed_height = 270; break;
		case 256: changed_height = 270; break;
		case 262: changed_height = 270; break;
		case 270: changed_height = 270; break;
		default: break;
		}
	}
	else if (hz == 60 && currVSyncRate == 50)
	{
		// Switch from PAL -> NTSC
		switch (changed_height)
		{
		case 200: changed_height = 200; break;
		case 216: changed_height = 200; break;
		case 240: changed_height = 200; break;
		case 256: changed_height = 216; break;
		case 262: changed_height = 216; break;
		case 270: changed_height = 240; break;
		default: break;
		}
	}

	if (hz != currVSyncRate)
	{
		currVSyncRate = hz;
#ifdef USE_SDL1
		black_screen_now();
#endif
		fpscounter_reset();
		time_per_frame = 1000 * 1000 / (hz);
		
		if (hz == host_hz)
			vsync_modulo = 1;
		else if (hz > host_hz)
			vsync_modulo = 6; // Amiga draws 6 frames while host has 5 vsyncs -> sync every 6th Amiga frame
		else
			vsync_modulo = 5; // Amiga draws 5 frames while host has 6 vsyncs -> sync every 5th Amiga frame
	}

	if (!picasso_on && !picasso_requested_on)
		changed_prefs.gfx_size.height = changed_height;

	return true;
}

bool target_graphics_buffer_update()
{
	auto rate_changed = false;

	if (currprefs.gfx_size.height != changed_prefs.gfx_size.height)
	{
		update_display(&changed_prefs);
		rate_changed = true;
	}

	if (rate_changed)
	{
#ifdef USE_SDL1
		black_screen_now();
#endif
		fpscounter_reset();
		time_per_frame = 1000 * 1000 / currprefs.chipset_refreshrate;
	}

	return true;
}

#ifdef PICASSO96

int picasso_palette(struct MyCLUTEntry *clut)
{
	auto changed = 0;
	for (auto i = 0; i < 256; i++)
	{
		int r = clut[i].Red;
		int g = clut[i].Green;
		int b = clut[i].Blue;
		//auto v = CONVERT_RGB(r << 16 | g << 8 | b);
		uae_u32 v = (doMask256(r, red_bits, red_shift)
			| doMask256(g, green_bits, green_shift)
			| doMask256(b, blue_bits, blue_shift))
			| doMask256(0xff, alpha_bits, alpha_shift);
		if (v != picasso_vidinfo.clut[i])
		{
			picasso_vidinfo.clut[i] = v;
			changed = 1;
		}
	}
	return changed;
}

static int resolution_compare(const void* a, const void* b)
{
	auto ma = (struct PicassoResolution *)a;
	auto mb = (struct PicassoResolution *)b;
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
	Displays[0].disabled = 0;
	Displays[0].rect.left = 0;
	Displays[0].rect.top = 0;
	Displays[0].rect.right = 800;
	Displays[0].rect.bottom = 480;
	sprintf(tmp, "%s (%d*%d)", "Display", Displays[0].rect.right, Displays[0].rect.bottom);
	Displays[0].name = my_strdup(tmp);
	Displays[0].name2 = my_strdup("Display");

	const auto md1 = Displays;
	DisplayModes = md1->DisplayModes = xmalloc(struct PicassoResolution, MAX_PICASSO_MODES);
	for (auto i = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; i++)
	{
		for (auto bitdepth : bits)
		{
			const auto bit_unit = bitdepth + 1 & 0xF8;
			const auto rgbFormat = bitdepth == 8 ? RGBFB_CLUT : bitdepth == 16 ? RGBFB_R5G6B5 : RGBFB_R8G8B8A8;
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
	if (on == screen_is_picasso)
		return;

	screen_is_picasso = on;
	open_screen(&currprefs);
	if (screen != nullptr)
		picasso_vidinfo.rowbytes = screen->pitch;
}

void gfx_set_picasso_modeinfo(uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{
	depth >>= 3;
	if (unsigned(picasso_vidinfo.width) == w &&
		unsigned(picasso_vidinfo.height) == h &&
		unsigned(picasso_vidinfo.depth) == depth &&
		picasso_vidinfo.selected_rgbformat == rgbfmt)
		return;

	picasso_vidinfo.selected_rgbformat = rgbfmt;
	picasso_vidinfo.width = w;
	picasso_vidinfo.height = h;
	picasso_vidinfo.depth = screen->format->BitsPerPixel; // Native depth
	picasso_vidinfo.extra_mem = 1;
	picasso_vidinfo.pixbytes = screen->format->BytesPerPixel; // Native bytes

	if (screen_is_picasso)
	{
		open_screen(&currprefs);
		if(screen != nullptr) {
			picasso_vidinfo.rowbytes = screen->pitch;
			picasso_vidinfo.rgbformat = screen->format->BytesPerPixel == 4 ? RGBFB_R8G8B8A8 : RGBFB_R5G6B5;
		}
	}
}

void gfx_set_picasso_colors(RGBFTYPE rgbfmt)
{
	alloc_colors_picasso(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt);
}

uae_u8* gfx_lock_picasso()
{
	if (screen == nullptr || screen_is_picasso == 0)
		return nullptr;
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

	picasso_vidinfo.rowbytes = screen->pitch;
	return static_cast<uae_u8 *>(screen->pixels);
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
