#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

#include <guisan.hpp>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#ifdef USE_OPENGL
#include <guisan/opengl.hpp>
#include <guisan/opengl/openglsdlimageloader.hpp>
#endif
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "fsdb_host.h"
#include "autoconf.h"
#include "amiberry_input.h"
#include "inputdevice.h"

#if defined(ANDROID)
#include "androidsdl_event.h"
#include <android/log.h>
#endif

bool ctrl_state = false, shift_state = false, alt_state = false, win_state = false;
int last_x = 0;
int last_y = 0;

bool gui_running = false;
static int last_active_panel = 3;
bool joystick_refresh_needed = false;

#define MAX_STARTUP_TITLE 64
#define MAX_STARTUP_MESSAGE 256
static TCHAR startup_title[MAX_STARTUP_TITLE] = _T("");
static TCHAR startup_message[MAX_STARTUP_MESSAGE] = _T("");

void target_startup_msg(const TCHAR* title, const TCHAR* msg)
{
	_tcsncpy(startup_title, title, MAX_STARTUP_TITLE);
	_tcsncpy(startup_message, msg, MAX_STARTUP_MESSAGE);
}

ConfigCategory categories[] = {
	{"About", "amigainfo.ico", nullptr, nullptr, InitPanelAbout, ExitPanelAbout, RefreshPanelAbout,
		HelpPanelAbout
	},
	{"Paths", "paths.ico", nullptr, nullptr, InitPanelPaths, ExitPanelPaths, RefreshPanelPaths, HelpPanelPaths},
	{"Quickstart", "quickstart.ico", nullptr, nullptr, InitPanelQuickstart, ExitPanelQuickstart,
		RefreshPanelQuickstart, HelpPanelQuickstart
	},
	{"Configurations", "file.ico", nullptr, nullptr, InitPanelConfig, ExitPanelConfig, RefreshPanelConfig,
		HelpPanelConfig
	},
	{"CPU and FPU", "cpu.ico", nullptr, nullptr, InitPanelCPU, ExitPanelCPU, RefreshPanelCPU, HelpPanelCPU},
	{"Chipset", "cpu.ico", nullptr, nullptr, InitPanelChipset, ExitPanelChipset, RefreshPanelChipset,
		HelpPanelChipset
	},
	{"ROM", "chip.ico", nullptr, nullptr, InitPanelROM, ExitPanelROM, RefreshPanelROM, HelpPanelROM},
	{"RAM", "chip.ico", nullptr, nullptr, InitPanelRAM, ExitPanelRAM, RefreshPanelRAM, HelpPanelRAM},
	{"Floppy drives", "35floppy.ico", nullptr, nullptr, InitPanelFloppy, ExitPanelFloppy, RefreshPanelFloppy,
		HelpPanelFloppy
	},
	{"Hard drives/CD", "drive.ico", nullptr, nullptr, InitPanelHD, ExitPanelHD, RefreshPanelHD, HelpPanelHD},
	{"Expansions", "expansion.ico", nullptr, nullptr, InitPanelExpansions, ExitPanelExpansions,
		RefreshPanelExpansions, HelpPanelExpansions},
	{"RTG board", "expansion.ico", nullptr, nullptr, InitPanelRTG, ExitPanelRTG,
		RefreshPanelRTG, HelpPanelRTG
	},
	{"Hardware info", "expansion.ico", nullptr, nullptr, InitPanelHWInfo, ExitPanelHWInfo, RefreshPanelHWInfo, HelpPanelHWInfo},
	{"Display", "screen.ico", nullptr, nullptr, InitPanelDisplay, ExitPanelDisplay, RefreshPanelDisplay,
		HelpPanelDisplay
	},
	{"Sound", "sound.ico", nullptr, nullptr, InitPanelSound, ExitPanelSound, RefreshPanelSound, HelpPanelSound},
	{"Input", "joystick.ico", nullptr, nullptr, InitPanelInput, ExitPanelInput, RefreshPanelInput, HelpPanelInput},
	{"IO Ports", "port.ico", nullptr, nullptr, InitPanelIO, ExitPanelIO, RefreshPanelIO, HelpPanelIO},
	{"Custom controls", "controller.png", nullptr, nullptr, InitPanelCustom, ExitPanelCustom,
		RefreshPanelCustom, HelpPanelCustom
	},
	{"Disk swapper", "35floppy.ico", nullptr, nullptr, InitPanelDiskSwapper, ExitPanelDiskSwapper, RefreshPanelDiskSwapper, HelpPanelDiskSwapper},
	{"Miscellaneous", "misc.ico", nullptr, nullptr, InitPanelMisc, ExitPanelMisc, RefreshPanelMisc, HelpPanelMisc},
	{ "Priority", "misc.ico", nullptr, nullptr, InitPanelPrio, ExitPanelPrio, RefreshPanelPrio, HelpPanelPrio},
	{"Savestates", "savestate.png", nullptr, nullptr, InitPanelSavestate, ExitPanelSavestate,
		RefreshPanelSavestate, HelpPanelSavestate
	},
	{"Virtual Keyboard", "keyboard.png", nullptr, nullptr, InitPanelVirtualKeyboard, 
		ExitPanelVirtualKeyboard, RefreshPanelVirtualKeyboard, HelpPanelVirtualKeyboard
	},
#ifdef ANDROID
	{ "OnScreen",         "screen.ico",    NULL, NULL, InitPanelOnScreen,  ExitPanelOnScreen, RefreshPanelOnScreen,  HelpPanelOnScreen },
#endif
	{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
};

enum
{
	PANEL_ABOUT,
	PANEL_PATHS,
	PANEL_QUICKSTART,
	PANEL_CONFIGURATIONS,
	PANEL_CPU,
	PANEL_CHIPSET,
	PANEL_ROM,
	PANEL_RAM,
	PANEL_FLOPPY,
	PANEL_HD,
	PANEL_EXPANSIONS,
	PANEL_RTG,
	PANEL_HWINFO,
	PANEL_DISPLAY,
	PANEL_SOUND,
	PANEL_INPUT,
	PANEL_IO,
	PANEL_CUSTOM,
	PANEL_MISC,
	PANEL_SAVESTATES,
	PANEL_VIRTUAL_KEYBOARD,
#ifdef ANDROID
	PANEL_ONSCREEN,
#endif
	NUM_PANELS
};

/*
* SDL Stuff we need
*/
SDL_Joystick* gui_joystick;
SDL_Surface* gui_screen;
SDL_Event gui_event;
SDL_Event touch_event;
#ifdef USE_DISPMANX
DISPMANX_RESOURCE_HANDLE_T gui_resource;
DISPMANX_RESOURCE_HANDLE_T black_gui_resource;
DISPMANX_ELEMENT_HANDLE_T gui_element;
DISPMANX_ELEMENT_HANDLE_T gui_black_element;
int element_present = 0;
#else
#ifdef USE_OPENGL
#else
SDL_Texture* gui_texture;
#endif
SDL_Cursor* cursor;
SDL_Surface* cursor_surface;
#endif

/*
* Gui SDL stuff we need
*/
gcn::SDLInput* gui_input;
#ifdef USE_OPENGL
gcn::OpenGLGraphics* gui_graphics;		   // Graphics driver
gcn::OpenGLSDLImageLoader* gui_imageLoader;  // For loading images
gcn::ImageFont* gui_font;
#else
gcn::SDLGraphics* gui_graphics;
gcn::SDLImageLoader* gui_imageLoader;
gcn::SDLTrueTypeFont* gui_font;
#endif

/*
* Gui stuff we need
*/
gcn::Gui* uae_gui;
gcn::Container* gui_top;
gcn::Container* selectors;
gcn::ScrollArea* selectorsScrollArea;

// GUI Colors
gcn::Color gui_baseCol;
gcn::Color colTextboxBackground;
gcn::Color colSelectorInactive;
gcn::Color colSelectorActive;

gcn::FocusHandler* focusHdl;
gcn::Widget* activeWidget;

// Main buttons
gcn::Button* cmdQuit;
gcn::Button* cmdReset;
gcn::Button* cmdRestart;
gcn::Button* cmdStart;
gcn::Button* cmdHelp;
gcn::Button* cmdShutdown;


/* Flag for changes in rtarea:
  Bit 0: any HD in config?
  Bit 1: force because add/remove HD was clicked or new config loaded
  Bit 2: socket_emu on
  Bit 3: mousehack on
  Bit 4: rtgmem on
  Bit 5: chipmem larger than 2MB
  
  gui_rtarea_flags_onenter is set before GUI is shown, bit 1 may change during GUI display.
*/
static int gui_rtarea_flags_onenter;

static int gui_create_rtarea_flag(uae_prefs* p)
{
	auto flag = 0;

	if (count_HDs(p) > 0)
		flag |= 1;

	// We allow on-the-fly switching of this in Amiberry
#ifndef AMIBERRY
	if (p->input_tablet > 0)
		flag |= 8;
#endif

	return flag;
}

void gui_force_rtarea_hdchange()
{
	gui_rtarea_flags_onenter |= 2;
}

void gui_restart()
{
	gui_running = false;
}

static void (*refresh_func_after_draw)() = nullptr;

void register_refresh_func(void (*func)())
{
	refresh_func_after_draw = func;
}

void focus_bug_workaround(gcn::Window* wnd)
{
	// When modal dialog opens via mouse, the dialog will not
	// have the focus unless there is a mouse click. We simulate the click...
	SDL_Event event{};
	event.type = SDL_MOUSEBUTTONDOWN;
	event.button.button = SDL_BUTTON_LEFT;
	event.button.state = SDL_PRESSED;
	event.button.x = wnd->getX() + 2;
	event.button.y = wnd->getY() + 2;
	gui_input->pushInput(event);
	event.type = SDL_MOUSEBUTTONUP;
	gui_input->pushInput(event);
}

static void show_help_requested()
{
	vector<string> helptext;
	if (categories[last_active_panel].HelpFunc != nullptr && categories[last_active_panel].HelpFunc(helptext))
	{
		//------------------------------------------------
		// Show help for current panel
		//------------------------------------------------
		char title[128];
		snprintf(title, 128, "Help for %s", categories[last_active_panel].category);
		ShowHelp(title, helptext);
	}
}

void cap_fps(Uint64 start)
{
	int refresh_rate;
	const auto end = SDL_GetPerformanceCounter();
	const auto elapsed_ms = static_cast<float>(end - start) / static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.0f;
#ifdef USE_DISPMANX
	refresh_rate = 60;
#else
	refresh_rate = sdl_mode.refresh_rate;
	if (refresh_rate < 50) refresh_rate = 50;
	if (refresh_rate > 60) refresh_rate = 60;
#endif
	float d = 0.0f;
	if (refresh_rate == 60)
		d = floor(16.666f - elapsed_ms);
	else
		d = floor(20.000f - elapsed_ms);

	if( d > 0.0f ) SDL_Delay( d );
}

void update_gui_screen()
{
#ifdef USE_DISPMANX
	vc_dispmanx_resource_write_data(gui_resource, rgb_mode, gui_screen->pitch, gui_screen->pixels, &blit_rect);
	updateHandle = vc_dispmanx_update_start(0);
	vc_dispmanx_element_change_source(updateHandle, gui_element, gui_resource);
	vc_dispmanx_update_submit_sync(updateHandle);
#elif USE_OPENGL
	const AmigaMonitor* mon = &AMonitors[0];
	SDL_GL_SwapWindow(mon->sdl_window);
#else
	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		renderQuad = { 0, 0, gui_screen->w, gui_screen->h };
	else
		renderQuad = { -(GUI_WIDTH - GUI_HEIGHT) / 2, (GUI_WIDTH - GUI_HEIGHT) / 2, gui_screen->w, gui_screen->h };
	
	SDL_RenderCopyEx(sdl_renderer, gui_texture, nullptr, &renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
	SDL_RenderPresent(sdl_renderer);
#endif
}

#ifdef USE_DISPMANX
#else
void setup_cursor()
{
	cursor_surface = SDL_LoadBMP(prefix_with_data_path("cursor.bmp").c_str());
	if (!cursor_surface)
	{
		// Load failed. Log error.
		write_log("Could not load cursor bitmap: %s\n", SDL_GetError());
		return;
	}
	
	auto* formatted_surface = SDL_ConvertSurfaceFormat(cursor_surface, SDL_PIXELFORMAT_RGBA8888, 0);
	if (formatted_surface != nullptr)
	{
		SDL_FreeSurface(cursor_surface);

		if (cursor != nullptr)
		{
			SDL_FreeCursor(cursor);
			cursor = nullptr;
		}
		
		// Create new cursor with surface
		cursor = SDL_CreateColorCursor(formatted_surface, 0, 0);
		SDL_FreeSurface(formatted_surface);
	}

	if (!cursor)
	{
		// Cursor creation failed. Log error and free surface
		write_log("Could not create color cursor: %s\n", SDL_GetError());
		cursor_surface = nullptr;
		formatted_surface = nullptr;
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
		return;
	}

	SDL_SetCursor(cursor);	
}
#endif

void init_dispmanx_gui()
{
#ifdef USE_DISPMANX
	if (!displayHandle)
		displayHandle = vc_dispmanx_display_open(0);
	rgb_mode = VC_IMAGE_RGB565;
	uint32_t vc_gui_image_ptr;
	if (!gui_resource)
		gui_resource = vc_dispmanx_resource_create(rgb_mode, GUI_WIDTH, GUI_HEIGHT, &vc_gui_image_ptr);
	if (!black_gui_resource)
		black_gui_resource = vc_dispmanx_resource_create(rgb_mode, GUI_WIDTH, GUI_HEIGHT, &vc_gui_image_ptr);
	
	vc_dispmanx_rect_set(&blit_rect, 0, 0, GUI_WIDTH, GUI_HEIGHT);
	vc_dispmanx_resource_write_data(gui_resource, rgb_mode, gui_screen->pitch, gui_screen->pixels, &blit_rect);
	vc_dispmanx_resource_write_data(black_gui_resource, rgb_mode, gui_screen->pitch, gui_screen->pixels, &blit_rect);
	vc_dispmanx_rect_set(&src_rect, 0, 0, GUI_WIDTH << 16, GUI_HEIGHT << 16);
	vc_dispmanx_rect_set(&black_rect, 0, 0, modeInfo.width, modeInfo.height);
	// Full screen destination rectangle
	//vc_dispmanx_rect_set(&dst_rect, 0, 0, modeInfo.width, modeInfo.height);

	// Scaled display with correct Aspect Ratio
	const auto want_aspect = static_cast<float>(GUI_WIDTH) / static_cast<float>(GUI_HEIGHT);
	const auto real_aspect = static_cast<float>(modeInfo.width) / static_cast<float>(modeInfo.height);

	SDL_Rect viewport;
	if (want_aspect > real_aspect)
	{
		const auto scale = static_cast<float>(modeInfo.width) / static_cast<float>(GUI_WIDTH);
		viewport.x = 0;
		viewport.w = modeInfo.width;
		viewport.h = static_cast<int>(std::ceil(GUI_HEIGHT * scale));
		viewport.y = (modeInfo.height - viewport.h) / 2;
	}
	else
	{
		const auto scale = static_cast<float>(modeInfo.height) / static_cast<float>(GUI_HEIGHT);
		viewport.y = 0;
		viewport.h = modeInfo.height;
		viewport.w = static_cast<int>(std::ceil(GUI_WIDTH * scale));
		viewport.x = (modeInfo.width - viewport.w) / 2;
	}
	vc_dispmanx_rect_set(&dst_rect, viewport.x, viewport.y, viewport.w, viewport.h);

	if (!element_present)
	{
		element_present = 1;
		updateHandle = vc_dispmanx_update_start(0);

		VC_DISPMANX_ALPHA_T alpha;
		alpha.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE;
		alpha.opacity = 255;
		alpha.mask = 0;

		if (!gui_black_element)
			gui_black_element = vc_dispmanx_element_add(updateHandle, displayHandle, 0,
				&black_rect, black_gui_resource, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
				nullptr, DISPMANX_NO_ROTATE);

		if (!gui_element)
			gui_element = vc_dispmanx_element_add(updateHandle, displayHandle, 1,
				&dst_rect, gui_resource, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
				nullptr, DISPMANX_NO_ROTATE);

		vc_dispmanx_update_submit_sync(updateHandle);
	}
#endif
}

void amiberry_gui_init()
{
	AmigaMonitor* mon = &AMonitors[0];
#ifndef USE_DISPMANX
	sdl_video_driver = SDL_GetCurrentVideoDriver();
	SDL_GetCurrentDisplayMode(0, &sdl_mode);
#endif

// we will grab the surface from the window directly, for OpenGL
#ifndef USE_OPENGL
	//-------------------------------------------------
	// Create new screen for GUI
	//-------------------------------------------------
	if (!gui_screen)
	{
		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create GUI surface:");
	}
#endif
#ifdef USE_DISPMANX
	init_dispmanx_gui();
	
	if (!mon->sdl_window)
	{
		mon->sdl_window = SDL_CreateWindow("Amiberry GUI",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			GUI_WIDTH,
			GUI_HEIGHT,
			SDL_WINDOW_FULLSCREEN_DESKTOP);
		check_error_sdl(mon->sdl_window == nullptr, "Unable to create window:");
	}
	if (sdl_renderer == nullptr)
	{
		sdl_renderer = SDL_CreateRenderer(mon->sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(sdl_renderer == nullptr, "Unable to create a renderer:");
	}
	SDL_RenderSetLogicalSize(sdl_renderer, GUI_WIDTH, GUI_HEIGHT);
#else
	//setup_cursor();

	if (!mon->sdl_window)
	{
        Uint32 sdl_window_mode;
        if (sdl_mode.w >= 800 && sdl_mode.h >= 600 && strcmpi(sdl_video_driver, "KMSDRM") != 0)
        {
            // Only enable Windowed mode if we're running under x11 and the resolution is at least 800x600
            if (currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW)
                sdl_window_mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
            else if (currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN)
                sdl_window_mode = SDL_WINDOW_FULLSCREEN;
            else
                sdl_window_mode =  SDL_WINDOW_RESIZABLE;
            if (currprefs.borderless)
                sdl_window_mode |= SDL_WINDOW_BORDERLESS;
            if (currprefs.main_alwaysontop)
                sdl_window_mode |= SDL_WINDOW_ALWAYS_ON_TOP;
            if (currprefs.start_minimized)
                sdl_window_mode |= SDL_WINDOW_HIDDEN;
            else
                sdl_window_mode |= SDL_WINDOW_SHOWN;
            // Set Window allow high DPI by default
            sdl_window_mode |= SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef USE_OPENGL
            sdl_window_mode |= SDL_WINDOW_OPENGL;
#endif
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


		auto* const icon_surface = IMG_Load(prefix_with_data_path("amiberry.png").c_str());
		if (icon_surface != nullptr)
		{
			SDL_SetWindowIcon(mon->sdl_window, icon_surface);
			SDL_FreeSurface(icon_surface);
		}
	}
	else if (mon->sdl_window)
	{
		const auto window_flags = SDL_GetWindowFlags(mon->sdl_window);
		const bool is_maximized = window_flags & SDL_WINDOW_MAXIMIZED;
		const bool is_fullscreen = window_flags & SDL_WINDOW_FULLSCREEN;
		if (!is_maximized && !is_fullscreen)
		{
			if (amiberry_options.rotation_angle != 0 && amiberry_options.rotation_angle != 180)
			{
				SDL_SetWindowSize(mon->sdl_window, GUI_HEIGHT, GUI_WIDTH);
			}
			else
			{
				SDL_SetWindowSize(mon->sdl_window, GUI_WIDTH, GUI_HEIGHT);
			}
		}
	}
#ifdef USE_OPENGL
	// Grab the window surface
	gui_screen = SDL_GetWindowSurface(mon->sdl_window);
	if (gl_context == nullptr)
		gl_context = SDL_GL_CreateContext(mon->sdl_window);

	// Enable vsync
	if (SDL_GL_SetSwapInterval(-1) < 0)
	{
		write_log("Warning: Adaptive V-Sync not supported on this platform, trying normal V-Sync\n");
		if (SDL_GL_SetSwapInterval(1) < 0)
		{
			write_log("Warning: Failed to enable V-Sync in the current GL context!\n");
		}
	}

	// Setup OpenGL
	glViewport(0, 0, GUI_WIDTH, GUI_HEIGHT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
#else
	if (sdl_renderer == nullptr)
	{
		sdl_renderer = SDL_CreateRenderer(mon->sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(sdl_renderer == nullptr, "Unable to create a renderer:");
	}

	// make the scaled rendering look smoother (linear scaling).
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	gui_texture = SDL_CreateTexture(sdl_renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING, gui_screen->w,
									gui_screen->h);
	check_error_sdl(gui_texture == nullptr, "Unable to create GUI texture:");

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(sdl_renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(sdl_renderer, GUI_HEIGHT, GUI_WIDTH);
#endif
#endif

	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);

	//-------------------------------------------------
	// Create helpers for GUI framework
	//-------------------------------------------------
#ifdef USE_OPENGL
	gui_imageLoader = new gcn::OpenGLSDLImageLoader();
#else
	gui_imageLoader = new gcn::SDLImageLoader();
	gui_imageLoader->setRenderer(sdl_renderer);
#endif
	// The ImageLoader in use is static and must be set to be
	// able to load images
	gcn::Image::setImageLoader(gui_imageLoader);
#ifdef USE_OPENGL
	gui_graphics = new gcn::OpenGLGraphics();
	gui_graphics->setTargetPlane(GUI_WIDTH, GUI_HEIGHT);
#else
	gui_graphics = new gcn::SDLGraphics();
	// Set the target for the graphics object to be the screen.
	// In other words, we will draw to the screen.
	// Note, any surface will do, it doesn't have to be the screen.
	gui_graphics->setTarget(gui_screen);
#endif
	gui_input = new gcn::SDLInput();
	uae_gui = new gcn::Gui();
	uae_gui->setGraphics(gui_graphics);
	uae_gui->setInput(gui_input);
}

void amiberry_gui_halt()
{
	AmigaMonitor* mon = &AMonitors[0];
	
	delete uae_gui;
	uae_gui = nullptr;
	delete gui_imageLoader;
    gui_imageLoader = nullptr;
	delete gui_input;
	gui_input = nullptr;
	delete gui_graphics;
	gui_graphics = nullptr;
#ifndef USE_OPENGL
	if (gui_screen != nullptr)
	{
		SDL_FreeSurface(gui_screen);
		gui_screen = nullptr;
	}
#endif
#ifdef USE_DISPMANX
	if (element_present == 1)
	{
		updateHandle = vc_dispmanx_update_start(0);
		vc_dispmanx_element_remove(updateHandle, gui_element);
		vc_dispmanx_element_remove(updateHandle, gui_black_element);
		vc_dispmanx_update_submit_sync(updateHandle);
		gui_element = 0;
		gui_black_element = 0;
		element_present = 0;
	}
	
	if (gui_resource)
	{
		vc_dispmanx_resource_delete(gui_resource);
		gui_resource = 0;
	}

	if (black_gui_resource)
	{
		vc_dispmanx_resource_delete(black_gui_resource);
		black_gui_resource = 0;
	}
	//if (displayHandle)
	//{
	//	vc_dispmanx_display_close(displayHandle);
	//	displayHandle = 0;
	//}
#elif USE_OPENGL
	if (cursor != nullptr)
	{
		SDL_FreeCursor(cursor);
		cursor = nullptr;
	}
#else
	if (gui_texture != nullptr)
	{
		SDL_DestroyTexture(gui_texture);
		gui_texture = nullptr;
	}

	if (cursor != nullptr)
	{
		SDL_FreeCursor(cursor);
		cursor = nullptr;
	}
#endif	
}

void check_input()
{
	auto got_event = 0;
	didata* did = &di_joystick[0];
	
	while (SDL_PollEvent(&gui_event))
	{
		switch (gui_event.type)
		{
		case SDL_QUIT:
			got_event = 1;
			//-------------------------------------------------
			// Quit entire program via SDL-Quit
			//-------------------------------------------------
			uae_quit();
			gui_running = false;
			break;

		case SDL_JOYDEVICEADDED:
		//case SDL_CONTROLLERDEVICEADDED:
		case SDL_JOYDEVICEREMOVED:
		//case SDL_CONTROLLERDEVICEREMOVED:
			write_log("GUI: SDL2 Controller/Joystick added or removed, re-running import joysticks...\n");
			if (inputdevice_devicechange(&currprefs))
			{
				import_joysticks();
				joystick_refresh_needed = true;
				RefreshPanelInput();
			}
			return;

		case SDL_JOYHATMOTION:
		case SDL_JOYBUTTONDOWN:
			if (gui_joystick)
			{
				got_event = 1;
				const int hat = SDL_JoystickGetHat(gui_joystick, 0);
				
				if (gui_event.jbutton.button == static_cast<Uint8>(enter_gui_button) || (
					SDL_JoystickGetButton(gui_joystick, did->mapping.menu_button) &&
					SDL_JoystickGetButton(gui_joystick, did->mapping.hotkey_button)))
				{
					if (emulating && cmdStart->isEnabled())
					{
						//------------------------------------------------
						// Continue emulation
						//------------------------------------------------
						gui_running = false;
					}
					else
					{
						//------------------------------------------------
						// First start of emulator -> reset Amiga
						//------------------------------------------------
						uae_reset(0, 1);
						gui_running = false;
					}
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_UP]) || hat & SDL_HAT_UP)
				{
					if (HandleNavigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || hat & SDL_HAT_DOWN)
				{
					if (HandleNavigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}

				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])))
				{
					for (auto z = 0; z < 10; ++z)
					{
						PushFakeKey(SDLK_UP);
					}
				}
				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])))
				{
					for (auto z = 0; z < 10; ++z)
					{
						PushFakeKey(SDLK_DOWN);
					}
				}

				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || hat & SDL_HAT_RIGHT)
				{
					if (HandleNavigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || hat & SDL_HAT_LEFT)
				{
					if (HandleNavigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_LEFT);
					break;
				}
				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_A])
						|| SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_B]))
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_A]))
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_B])))
				{
					PushFakeKey(SDLK_RETURN);
					continue;
				}

				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.quit_button) &&
					SDL_JoystickGetButton(gui_joystick, did->mapping.hotkey_button))
				{
					// use the HOTKEY button
					uae_quit();
					gui_running = false;
					break;
				}

				else if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_GUIDE])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_GUIDE])))
				{
					// use the HOTKEY button
					gui_running = false;
				}
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				if (gui_event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
				{
					if (gui_event.jaxis.value > joystick_dead_zone && last_x != 1)
					{
						last_x = 1;
						if (HandleNavigation(DIRECTION_RIGHT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_RIGHT);
						break;
					}
					if (gui_event.jaxis.value < -joystick_dead_zone && last_x != -1)
					{
						last_x = -1;
						if (HandleNavigation(DIRECTION_LEFT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_LEFT);
						break;
					}
					if (gui_event.jaxis.value > -joystick_dead_zone && gui_event.jaxis.value < joystick_dead_zone)
						last_x = 0;
				}
				else if (gui_event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
				{
					if (gui_event.jaxis.value < -joystick_dead_zone && last_y != -1)
					{
						last_y = -1;
						if (HandleNavigation(DIRECTION_UP))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_UP);
						break;
					}
					if (gui_event.jaxis.value > joystick_dead_zone && last_y != 1)
					{
						last_y = 1;
						if (HandleNavigation(DIRECTION_DOWN))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_DOWN);
						break;
					}
					if (gui_event.jaxis.value > -joystick_dead_zone && gui_event.jaxis.value < joystick_dead_zone)
						last_y = 0;
				}
			}
			break;

		case SDL_KEYDOWN:
			got_event = 1;
			if (gui_event.key.keysym.sym == SDLK_RCTRL || gui_event.key.keysym.sym == SDLK_LCTRL) ctrl_state = true;
			else if (gui_event.key.keysym.sym == SDLK_RSHIFT || gui_event.key.keysym.sym == SDLK_LSHIFT) shift_state = true;
			else if (gui_event.key.keysym.sym == SDLK_RALT || gui_event.key.keysym.sym == SDLK_LALT) alt_state = true;
			else if (gui_event.key.keysym.sym == SDLK_RGUI || gui_event.key.keysym.sym == SDLK_LGUI) win_state = true;

			if (gui_event.key.keysym.scancode == enter_gui_key.scancode)
			{
				if ((enter_gui_key.modifiers.lctrl || enter_gui_key.modifiers.rctrl) == ctrl_state
					&& (enter_gui_key.modifiers.lshift || enter_gui_key.modifiers.rshift) == shift_state
					&& (enter_gui_key.modifiers.lalt || enter_gui_key.modifiers.ralt) == alt_state
					&& (enter_gui_key.modifiers.lgui || enter_gui_key.modifiers.rgui) == win_state)
				{
					if (emulating && cmdStart->isEnabled())
					{
						//------------------------------------------------
						// Continue emulation
						//------------------------------------------------
						gui_running = false;
					}
					else
					{
						//------------------------------------------------
						// First start of emulator -> reset Amiga
						//------------------------------------------------
						uae_reset(0, 1);
						gui_running = false;
					}
				}
			}
			else
			{
				switch (gui_event.key.keysym.sym)
				{
				case SDLK_q:
					//-------------------------------------------------
					// Quit entire program via Q on keyboard
					//-------------------------------------------------
					focusHdl = gui_top->_getFocusHandler();
					activeWidget = focusHdl->getFocused();
					if (dynamic_cast<gcn::TextField*>(activeWidget) == nullptr)
					{
						// ...but only if we are not in a Textfield...
						uae_quit();
						gui_running = false;
					}
					break;

				case VK_ESCAPE:
				case VK_Red:
					gui_running = false;
					break;

				case VK_Green:
				case VK_Blue:
					//------------------------------------------------
					// Simulate press of enter when 'X' pressed
					//------------------------------------------------
					gui_event.key.keysym.sym = SDLK_RETURN;

					gui_input->pushInput(gui_event); // Fire key down
					gui_event.type = SDL_KEYUP; // and the key up
					break;

				case VK_UP:
					if (HandleNavigation(DIRECTION_UP))
						continue; // Don't change value when enter ComboBox -> don't send event to control
					break;

				case VK_DOWN:
					if (HandleNavigation(DIRECTION_DOWN))
						continue; // Don't change value when enter ComboBox -> don't send event to control
					break;

				case VK_LEFT:
					if (HandleNavigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					break;

				case VK_RIGHT:
					if (HandleNavigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					break;

				case SDLK_F1:
					show_help_requested();
					cmdHelp->requestFocus();
					break;

				default:
					break;
				}
			}
			break;

		case SDL_FINGERDOWN:
			got_event = 1;
			memcpy(&touch_event, &gui_event, sizeof gui_event);
			touch_event.type = SDL_MOUSEBUTTONDOWN;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_PRESSED;
#ifdef USE_OPENGL
			touch_event.button.x = gui_graphics->getTargetPlaneWidth() * int(gui_event.tfinger.x);
			touch_event.button.y = gui_graphics->getTargetPlaneHeight() * int(gui_event.tfinger.y);
#else
			touch_event.button.x = gui_graphics->getTarget()->w * int(gui_event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * int(gui_event.tfinger.y);
#endif
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &gui_event, sizeof gui_event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;
#ifdef USE_OPENGL
			touch_event.button.x = gui_graphics->getTargetPlaneWidth() * int(gui_event.tfinger.x);
			touch_event.button.y = gui_graphics->getTargetPlaneHeight() * int(gui_event.tfinger.y);
#else
			touch_event.button.x = gui_graphics->getTarget()->w * int(gui_event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * int(gui_event.tfinger.y);
#endif
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &gui_event, sizeof gui_event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;
#ifdef USE_OPENGL
			touch_event.motion.x = gui_graphics->getTargetPlaneWidth() * int(gui_event.tfinger.x);
			touch_event.motion.y = gui_graphics->getTargetPlaneHeight() * int(gui_event.tfinger.y);
#else
			touch_event.motion.x = gui_graphics->getTarget()->w * int(gui_event.tfinger.x);
			touch_event.motion.y = gui_graphics->getTarget()->h * int(gui_event.tfinger.y);
#endif
			gui_input->pushInput(touch_event);
			break;

		case SDL_MOUSEWHEEL:
			got_event = 1;
			if (gui_event.wheel.y > 0)
			{
				for (auto z = 0; z < gui_event.wheel.y; ++z)
				{
					PushFakeKey(SDLK_UP);
				}
			}
			else if (gui_event.wheel.y < 0)
			{
				for (auto z = 0; z > gui_event.wheel.y; --z)
				{
					PushFakeKey(SDLK_DOWN);
				}
			}
			break;

		case SDL_KEYUP:
			got_event = 1;
			if (gui_event.key.keysym.sym == SDLK_RCTRL || gui_event.key.keysym.sym == SDLK_LCTRL) ctrl_state = false;
			else if (gui_event.key.keysym.sym == SDLK_RSHIFT || gui_event.key.keysym.sym == SDLK_LSHIFT) shift_state = false;
			else if (gui_event.key.keysym.sym == SDLK_RALT || gui_event.key.keysym.sym == SDLK_LALT) alt_state = false;
			else if (gui_event.key.keysym.sym == SDLK_RGUI || gui_event.key.keysym.sym == SDLK_LGUI) win_state = false;
			break;
		case SDL_JOYBUTTONUP:
		case SDL_CONTROLLERBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_RENDER_TARGETS_RESET:
		case SDL_RENDER_DEVICE_RESET:
		case SDL_WINDOWEVENT:
		case SDL_DISPLAYEVENT:
		case SDL_SYSWMEVENT:
			got_event = 1;
			break;
			
		default:
			break;
		}

		//-------------------------------------------------
		// Send event to gui-controls
		//-------------------------------------------------
#ifdef ANDROID
		androidsdl_event(gui_event, gui_input);
#else
		gui_input->pushInput(gui_event);
#endif
	}
	
	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
#ifndef USE_OPENGL
		SDL_RenderClear(sdl_renderer);
#endif
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		update_gui_screen();
	}
}

void amiberry_gui_run()
{
	if (amiberry_options.gui_joystick_control)
	{
		for (auto j = 0; j < SDL_NumJoysticks(); j++)
		{
			gui_joystick = SDL_JoystickOpen(j);
			// Some controllers (e.g. PS4) report a second instance with only axes and no buttons.
			// We ignore these and move on.
			if (SDL_JoystickNumButtons(gui_joystick) < 1)
			{
				SDL_JoystickClose(gui_joystick);
				continue;
			}
			if (gui_joystick)
				break;
		}
	}

	// Prepare the screen once
	uae_gui->logic();
#ifndef USE_OPENGL
	SDL_RenderClear(sdl_renderer);
#endif
	uae_gui->draw();
	update_gui_screen();
	
	//-------------------------------------------------
	// The main loop
	//-------------------------------------------------
	while (gui_running)
	{
		const auto start = SDL_GetPerformanceCounter();
		check_input();

		if (gui_rtarea_flags_onenter != gui_create_rtarea_flag(&changed_prefs))
			disable_resume();

		if (refresh_func_after_draw != nullptr)
		{
			void (*currFunc)() = refresh_func_after_draw;
			refresh_func_after_draw = nullptr;
			currFunc();
		}

		cap_fps(start);
	}

	if (gui_joystick)
	{
		SDL_JoystickClose(gui_joystick);
		gui_joystick = nullptr;
	}
}

class MainButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdShutdown)
		{
			// ------------------------------------------------
			// Shutdown the host (power off)
			// ------------------------------------------------
			uae_quit();
			gui_running = false;
			host_poweroff = true;
		}

		if (actionEvent.getSource() == cmdQuit)
		{
			//-------------------------------------------------
			// Quit entire program via click on Quit-button
			//-------------------------------------------------
			uae_quit();
			gui_running = false;
		}
		else if (actionEvent.getSource() == cmdReset)
		{
			//-------------------------------------------------
			// Reset Amiga via click on Reset-button
			//-------------------------------------------------
			uae_reset(1, 1);
			gui_running = false;
		}
		else if (actionEvent.getSource() == cmdRestart)
		{
			//-------------------------------------------------
			// Restart emulator
			//-------------------------------------------------
			char tmp[MAX_DPATH];
			get_configuration_path(tmp, sizeof tmp);
			if (strlen(last_loaded_config) > 0)
				strncat(tmp, last_loaded_config, MAX_DPATH - 1);
			else
			{
				strncat(tmp, OPTIONSFILENAME, MAX_DPATH - 1);
				strncat(tmp, ".uae", MAX_DPATH - 10);
			}
			uae_restart(-1, tmp);
			gui_running = false;
		}
		else if (actionEvent.getSource() == cmdStart)
		{
			if (emulating && cmdStart->isEnabled())
			{
				//------------------------------------------------
				// Continue emulation
				//------------------------------------------------
				gui_running = false;
			}
			else
			{
				//------------------------------------------------
				// First start of emulator -> reset Amiga
				//------------------------------------------------
				uae_reset(0, 1);
				gui_running = false;
			}
		}
		else if (actionEvent.getSource() == cmdHelp)
		{
			show_help_requested();
			cmdHelp->requestFocus();
		}
	}
};

MainButtonActionListener* mainButtonActionListener;

class PanelFocusListener : public gcn::FocusListener
{
public:
	void focusGained(const gcn::Event& event) override
	{
		for (auto i = 0; categories[i].category != nullptr; ++i)
		{
			if (event.getSource() == categories[i].selector)
			{
				categories[i].selector->setActive(true);
				categories[i].panel->setVisible(true);
				last_active_panel = i;
				cmdHelp->setVisible(categories[last_active_panel].HelpFunc != nullptr);
			}
			else
			{
				categories[i].selector->setActive(false);
				categories[i].panel->setVisible(false);
			}
		}
	}
};

PanelFocusListener* panelFocusListener;

void gui_widgets_init()
{
	int i;
	int yPos;

	//-------------------------------------------------
	// Define base colors
	//-------------------------------------------------
	gui_baseCol = gui_theme.base_color;
	colSelectorInactive = gui_theme.selector_inactive;
	colSelectorActive = gui_theme.selector_active;
	colTextboxBackground = gui_theme.textbox_background;

	//-------------------------------------------------
	// Create container for main page
	//-------------------------------------------------
	gui_top = new gcn::Container();
	gui_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
	gui_top->setBaseColor(gui_baseCol);
	uae_gui->setTop(gui_top);

#ifndef USE_OPENGL
	//-------------------------------------------------
	// Initialize fonts
	//-------------------------------------------------
	TTF_Init();
#endif

	try
	{
#ifdef USE_OPENGL
		gui_font = new gcn::ImageFont(prefix_with_data_path("rpgfont.png"), " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&`'*#=[]\"");
#else
		gui_font = new gcn::SDLTrueTypeFont(prefix_with_data_path(gui_theme.font_name), gui_theme.font_size);
		gui_font->setAntiAlias(false);
#endif
	}
	catch (exception& ex)
	{
		cout << ex.what() << '\n';
		write_log("An error occurred while trying to open the GUI font! Exception: %s\n", ex.what());
		abort();
	}

	gcn::Widget::setGlobalFont(gui_font);

	//--------------------------------------------------
	// Create main buttons
	//--------------------------------------------------
	mainButtonActionListener = new MainButtonActionListener();

	cmdQuit = new gcn::Button("Quit");
	cmdQuit->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdQuit->setBaseColor(gui_baseCol);
	cmdQuit->setId("Quit");
	cmdQuit->addActionListener(mainButtonActionListener);

	cmdShutdown = new gcn::Button("Shutdown");
	cmdShutdown->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdShutdown->setBaseColor(gui_baseCol);
	cmdShutdown->setId("Shutdown");
	cmdShutdown->addActionListener(mainButtonActionListener);

	cmdReset = new gcn::Button("Reset");
	cmdReset->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdReset->setBaseColor(gui_baseCol);
	cmdReset->setId("Reset");
	cmdReset->addActionListener(mainButtonActionListener);

	cmdRestart = new gcn::Button("Restart");
	cmdRestart->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdRestart->setBaseColor(gui_baseCol);
	cmdRestart->setId("Restart");
	cmdRestart->addActionListener(mainButtonActionListener);

	cmdStart = new gcn::Button("Start");
	if (emulating)
		cmdStart->setCaption("Resume");
	cmdStart->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdStart->setBaseColor(gui_baseCol);
	cmdStart->setId("Start");
	cmdStart->addActionListener(mainButtonActionListener);

	cmdHelp = new gcn::Button("Help");
	cmdHelp->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdHelp->setBaseColor(gui_baseCol);
	cmdHelp->setId("Help");
	cmdHelp->addActionListener(mainButtonActionListener);

	//--------------------------------------------------
	// Create selector entries
	//--------------------------------------------------
	const auto workAreaHeight = GUI_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10;
	const auto selectorWidth = 150;
	const auto selectorHeight = 24;
	selectors = new gcn::Container();
	selectors->setBorderSize(0);
	selectors->setBaseColor(colSelectorInactive);

	const auto selectorScrollAreaWidth = selectorWidth + 12;
	selectorsScrollArea = new gcn::ScrollArea();
	selectorsScrollArea->setContent(selectors);
	selectorsScrollArea->setBaseColor(colSelectorInactive);
	selectorsScrollArea->setSize(selectorScrollAreaWidth, workAreaHeight);
	selectorsScrollArea->setBorderSize(1);
	
	const auto panelStartX = DISTANCE_BORDER + selectorsScrollArea->getWidth() + 2 + 11;

	double selectorsHeight = 0.0;
	panelFocusListener = new PanelFocusListener();
	for (i = 0; categories[i].category != nullptr; ++i)
	{
		categories[i].selector = new gcn::SelectorEntry(categories[i].category, prefix_with_data_path(categories[i].imagepath));
		categories[i].selector->setActiveColor(colSelectorActive);
		categories[i].selector->setInactiveColor(colSelectorInactive);
		categories[i].selector->setSize(selectorWidth, selectorHeight);
		categories[i].selector->addFocusListener(panelFocusListener);

		categories[i].panel = new gcn::Container();
		categories[i].panel->setId(categories[i].category);
		categories[i].panel->setSize(GUI_WIDTH - panelStartX - DISTANCE_BORDER, workAreaHeight);
		categories[i].panel->setBaseColor(gui_baseCol);
		categories[i].panel->setBorderSize(1);
		categories[i].panel->setVisible(false);
			
		selectorsHeight += categories[i].selector->getHeight();
	}

	selectors->setSize(150, selectorsHeight);

	//--------------------------------------------------
	// Initialize panels
	//--------------------------------------------------
	for (i = 0; categories[i].category != nullptr; ++i)
	{
		if (categories[i].InitFunc != nullptr)
			(*categories[i].InitFunc)(categories[i]);
	}

	//--------------------------------------------------
	// Place everything on main form
	//--------------------------------------------------
#ifndef ANDROID
	gui_top->add(cmdShutdown, DISTANCE_BORDER, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
#endif
	gui_top->add(cmdQuit, DISTANCE_BORDER + BUTTON_WIDTH + DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdRestart, DISTANCE_BORDER + 2 * BUTTON_WIDTH + 2 * DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdHelp, DISTANCE_BORDER + 3 * BUTTON_WIDTH + 3 * DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdReset, DISTANCE_BORDER + 5 * BUTTON_WIDTH + 5 * DISTANCE_NEXT_X,
				 GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
	gui_top->add(cmdStart, GUI_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);

	gui_top->add(selectorsScrollArea, DISTANCE_BORDER + 1, DISTANCE_BORDER + 1);

	for (i = 0, yPos = 0; categories[i].category != nullptr; ++i, yPos += 24)
	{
		selectors->add(categories[i].selector, 0, yPos);
		gui_top->add(categories[i].panel, panelStartX, DISTANCE_BORDER + 1);
	}

	//--------------------------------------------------
	// Activate last active panel
	//--------------------------------------------------
	if (!emulating && amiberry_options.quickstart_start)
		last_active_panel = 2;
	if (amiberry_options.disable_shutdown_button)
		cmdShutdown->setEnabled(false);
	categories[last_active_panel].selector->requestFocus();
	cmdHelp->setVisible(categories[last_active_panel].HelpFunc != nullptr);
}

void gui_widgets_halt()
{
	for (auto i = 0; categories[i].category != nullptr; ++i)
	{
		if (categories[i].ExitFunc != nullptr)
			(*categories[i].ExitFunc)();

		delete categories[i].selector;
		delete categories[i].panel;
	}

	delete panelFocusListener;
	delete selectors;
	delete selectorsScrollArea;
	
	delete cmdQuit;
	delete cmdShutdown;
	delete cmdReset;
	delete cmdRestart;
	delete cmdStart;
	delete cmdHelp;

	delete mainButtonActionListener;

	delete gui_font;
	gui_font = nullptr;
	delete gui_top;
	gui_top = nullptr;
}

void refresh_all_panels()
{
	for (auto i = 0; categories[i].category != nullptr; ++i)
	{
		if (categories[i].RefreshFunc != nullptr)
			(*categories[i].RefreshFunc)();
	}
}

void disable_resume()
{
	if (emulating)
	{
		cmdStart->setEnabled(false);
	}
}

void run_gui()
{
	gui_running = true;
	gui_rtarea_flags_onenter = gui_create_rtarea_flag(&currprefs);

	expansion_generate_autoconfig_info(&changed_prefs);

	try
	{
		amiberry_gui_init();
		gui_widgets_init();
		if (_tcslen(startup_message) > 0)
		{
			ShowMessage(startup_title, startup_message, "", "", "Ok", "");
			_tcscpy(startup_title, _T(""));
			_tcscpy(startup_message, _T(""));
			cmdStart->requestFocus();
		}
		amiberry_gui_run();
		gui_widgets_halt();
		amiberry_gui_halt();
	}

	// Catch all GUI framework exceptions.
	catch (gcn::Exception& e)
	{
		std::cout << e.getMessage() << std::endl;
		uae_quit();
	}

	// Catch all Std exceptions.
	catch (exception& e)
	{
		std::cout << "Std exception: " << e.what() << std::endl;
		uae_quit();
	}

	// Catch all unknown exceptions.
	catch (...)
	{
		std::cout << "Unknown exception" << std::endl;
		uae_quit();
	}

	expansion_generate_autoconfig_info(&changed_prefs);
	cfgfile_compatibility_romtype(&changed_prefs);

	if (quit_program > UAE_QUIT || quit_program < -UAE_QUIT)
	{
		//--------------------------------------------------
		// Prepare everything for Reset of Amiga
		//--------------------------------------------------
		currprefs.nr_floppies = changed_prefs.nr_floppies;

		if (gui_rtarea_flags_onenter != gui_create_rtarea_flag(&changed_prefs))
			quit_program = -UAE_RESET_HARD; // Hardreset required...
	}

	// Reset counter for access violations
	init_max_signals();
}
