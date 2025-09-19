#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <dpi_handler.hpp>

#ifdef USE_GUISAN
#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"
#endif

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <array>
#endif

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "fsdb_host.h"
#include "autoconf.h"
#include "inputdevice.h"
#include "xwin.h"
#include "custom.h"
#include "disk.h"
#include "savestate.h"
#include "target.h"

bool ctrl_state = false, shift_state = false, alt_state = false, win_state = false;
int last_x = 0;
int last_y = 0;

bool gui_running = false;
static int last_active_panel = 3;
bool joystick_refresh_needed = false;

enum
{
	MAX_STARTUP_TITLE = 64,
	MAX_STARTUP_MESSAGE = 256
};

static TCHAR startup_title[MAX_STARTUP_TITLE] = _T("");
static TCHAR startup_message[MAX_STARTUP_MESSAGE] = _T("");

void target_startup_msg(const TCHAR* title, const TCHAR* msg)
{
	_tcsncpy(startup_title, title, MAX_STARTUP_TITLE);
	_tcsncpy(startup_message, msg, MAX_STARTUP_MESSAGE);
}

#ifdef USE_GUISAN
ConfigCategory categories[] = {
	{"About", "amigainfo.png", nullptr, nullptr, InitPanelAbout, ExitPanelAbout, RefreshPanelAbout,
		HelpPanelAbout
	},
	{"Paths", "paths.png", nullptr, nullptr, InitPanelPaths, ExitPanelPaths, RefreshPanelPaths, HelpPanelPaths},
	{"Quickstart", "quickstart.png", nullptr, nullptr, InitPanelQuickstart, ExitPanelQuickstart,
		RefreshPanelQuickstart, HelpPanelQuickstart
	},
	{"Configurations", "file.png", nullptr, nullptr, InitPanelConfig, ExitPanelConfig, RefreshPanelConfig,
		HelpPanelConfig
	},
	{"CPU and FPU", "cpu.png", nullptr, nullptr, InitPanelCPU, ExitPanelCPU, RefreshPanelCPU, HelpPanelCPU},
	{"Chipset", "cpu.png", nullptr, nullptr, InitPanelChipset, ExitPanelChipset, RefreshPanelChipset,
		HelpPanelChipset
	},
	{"ROM", "chip.png", nullptr, nullptr, InitPanelROM, ExitPanelROM, RefreshPanelROM, HelpPanelROM},
	{"RAM", "chip.png", nullptr, nullptr, InitPanelRAM, ExitPanelRAM, RefreshPanelRAM, HelpPanelRAM},
	{"Floppy drives", "35floppy.png", nullptr, nullptr, InitPanelFloppy, ExitPanelFloppy, RefreshPanelFloppy,
		HelpPanelFloppy
	},
	{"Hard drives/CD", "drive.png", nullptr, nullptr, InitPanelHD, ExitPanelHD, RefreshPanelHD, HelpPanelHD},
	{"Expansions", "expansion.png", nullptr, nullptr, InitPanelExpansions, ExitPanelExpansions,
		RefreshPanelExpansions, HelpPanelExpansions},
	{"RTG board", "expansion.png", nullptr, nullptr, InitPanelRTG, ExitPanelRTG,
		RefreshPanelRTG, HelpPanelRTG
	},
	{"Hardware info", "expansion.png", nullptr, nullptr, InitPanelHWInfo, ExitPanelHWInfo, RefreshPanelHWInfo, HelpPanelHWInfo},
	{"Display", "screen.png", nullptr, nullptr, InitPanelDisplay, ExitPanelDisplay, RefreshPanelDisplay,
		HelpPanelDisplay
	},
	{"Sound", "sound.png", nullptr, nullptr, InitPanelSound, ExitPanelSound, RefreshPanelSound, HelpPanelSound},
	{"Input", "joystick.png", nullptr, nullptr, InitPanelInput, ExitPanelInput, RefreshPanelInput, HelpPanelInput},
	{"IO Ports", "port.png", nullptr, nullptr, InitPanelIO, ExitPanelIO, RefreshPanelIO, HelpPanelIO},
	{"Custom controls", "controller.png", nullptr, nullptr, InitPanelCustom, ExitPanelCustom,
		RefreshPanelCustom, HelpPanelCustom
	},
	{"Disk swapper", "35floppy.png", nullptr, nullptr, InitPanelDiskSwapper, ExitPanelDiskSwapper, RefreshPanelDiskSwapper, HelpPanelDiskSwapper},
	{"Miscellaneous", "misc.png", nullptr, nullptr, InitPanelMisc, ExitPanelMisc, RefreshPanelMisc, HelpPanelMisc},
	{ "Priority", "misc.png", nullptr, nullptr, InitPanelPrio, ExitPanelPrio, RefreshPanelPrio, HelpPanelPrio},
	{"Savestates", "savestate.png", nullptr, nullptr, InitPanelSavestate, ExitPanelSavestate,
		RefreshPanelSavestate, HelpPanelSavestate
	},
	{"Virtual Keyboard", "keyboard.png", nullptr, nullptr, InitPanelVirtualKeyboard, 
		ExitPanelVirtualKeyboard, RefreshPanelVirtualKeyboard, HelpPanelVirtualKeyboard
	},
	{"WHDLoad", "drive.png", nullptr, nullptr, InitPanelWHDLoad, ExitPanelWHDLoad, RefreshPanelWHDLoad, HelpPanelWHDLoad},

	{"Themes", "amigainfo.png", nullptr, nullptr, InitPanelThemes, ExitPanelThemes, RefreshPanelThemes, HelpPanelThemes},

	{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
};
#elif USE_IMGUI
static bool show_message_box = false;
static char message_box_title[128] = "";
static char message_box_message[2048] = "";
static float gui_scale = 1.0f;

// About panel resources (ImGui backend)
static SDL_Texture* about_logo_texture = nullptr;
static int about_logo_tex_w = 0;
static int about_logo_tex_h = 0;

static void ensure_about_logo_texture()
{
	if (about_logo_texture)
		return;

	AmigaMonitor* mon = &AMonitors[0];
	const auto path = prefix_with_data_path("amiberry-logo.png");
	SDL_Surface* surf = IMG_Load(path.c_str());
	if (!surf)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "About panel: failed to load %s: %s", path.c_str(), SDL_GetError());
		return;
	}
	about_logo_tex_w = surf->w;
	about_logo_tex_h = surf->h;
	about_logo_texture = SDL_CreateTextureFromSurface(mon->gui_renderer, surf);
	SDL_FreeSurface(surf);
	if (!about_logo_texture)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "About panel: failed to create texture: %s", SDL_GetError());
	}
}

ConfigCategory categories[] = {
  {"About", "amigainfo.png"},
  {"Paths", "paths.png"},
  {"Quickstart", "quickstart.png"},
  {"Configurations", "file.png"},
  {"CPU and FPU", "cpu.png"},
  {"Chipset", "cpu.png"},
  {"ROM", "chip.png"},
  {"RAM", "chip.png"},
  {"Floppy drives", "35floppy.png"},
  {"Hard drives/CD", "drive.png"},
  {"Expansions", "expansion.png"},
  {"RTG board", "expansion.png"},
  {"Hardware info", "expansion.png"},
  {"Display", "screen.png"},
  {"Sound", "sound.png"},
  {"Input", "joystick.png"},
  {"IO Ports", "port.png"},
  {"Custom controls", "controller.png"},
  {"Disk swapper", "35floppy.png"},
  {"Miscellaneous", "misc.png"},
  {"Priority", "misc.png"},
  {"Savestates", "savestate.png"},
  {"Virtual Keyboard", "keyboard.png"},
  {"WHDLoad", "drive.png"},
  {"Themes", "amigainfo.png"},
  {nullptr, nullptr}
};
#endif

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
	PANEL_DISK_SWAPPER,
	PANEL_MISC,
	PANEL_PRIO,
	PANEL_SAVESTATES,
	PANEL_VIRTUAL_KEYBOARD,
	PANEL_WHDLOAD,
	PANEL_THEMES,
	NUM_PANELS
};

/*
* SDL Stuff we need
*/
SDL_Joystick* gui_joystick;
SDL_Surface* gui_screen;
SDL_Event gui_event;
SDL_Event touch_event;
SDL_Texture* gui_texture;
SDL_Rect gui_renderQuad;
SDL_Rect gui_window_rect{0, 0, GUI_WIDTH, GUI_HEIGHT};

#ifdef USE_GUISAN
/*
* Gui SDL stuff we need
*/
gcn::SDLInput* gui_input;
gcn::SDLGraphics* gui_graphics;
gcn::SDLImageLoader* gui_imageLoader;
gcn::SDLTrueTypeFont* gui_font;

/*
* Gui stuff we need
*/
gcn::Gui* uae_gui;
gcn::Container* gui_top;
gcn::Container* selectors;
gcn::ScrollArea* selectorsScrollArea;

// GUI Colors
gcn::Color gui_base_color;
gcn::Color gui_background_color;
gcn::Color gui_selector_inactive_color;
gcn::Color gui_selector_active_color;
gcn::Color gui_selection_color;
gcn::Color gui_foreground_color;
gcn::Color gui_font_color;

gcn::FocusHandler* focusHdl;
gcn::Widget* activeWidget;

// Main buttons
gcn::Button* cmdQuit;
gcn::Button* cmdReset;
gcn::Button* cmdRestart;
gcn::Button* cmdStart;
gcn::Button* cmdHelp;
gcn::Button* cmdShutdown;
#endif

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

int disk_in_drive(int entry)
{
	for (int i = 0; i < 4; i++) {
		if (_tcslen(changed_prefs.dfxlist[entry]) > 0 && !_tcscmp(changed_prefs.dfxlist[entry], changed_prefs.floppyslots[i].df))
			return i;
	}
	return -1;
}

int disk_swap(int entry, int mode)
{
	int drv, i, drvs[4] = { -1, -1, -1, -1 };

	for (i = 0; i < MAX_SPARE_DRIVES; i++) {
		drv = disk_in_drive(i);
		if (drv >= 0)
			drvs[drv] = i;
	}
	if ((drv = disk_in_drive(entry)) >= 0) {
		if (mode < 0) {
			changed_prefs.floppyslots[drv].df[0] = 0;
			return 1;
		}

		if (_tcscmp(changed_prefs.floppyslots[drv].df, currprefs.floppyslots[drv].df)) {
			_tcscpy(changed_prefs.floppyslots[drv].df, currprefs.floppyslots[drv].df);
			disk_insert(drv, changed_prefs.floppyslots[drv].df);
		}
		else {
			changed_prefs.floppyslots[drv].df[0] = 0;
		}
		if (drvs[0] < 0 || drvs[1] < 0 || drvs[2] < 0 || drvs[3] < 0) {
			drv++;
			while (drv < 4 && drvs[drv] >= 0)
				drv++;
			if (drv < 4 && changed_prefs.floppyslots[drv].dfxtype >= 0) {
				_tcscpy(changed_prefs.floppyslots[drv].df, changed_prefs.dfxlist[entry]);
				disk_insert(drv, changed_prefs.floppyslots[drv].df);
			}
		}
		return 1;
	}
	for (i = 0; i < 4; i++) {
		if (drvs[i] < 0 && changed_prefs.floppyslots[i].dfxtype >= 0) {
			_tcscpy(changed_prefs.floppyslots[i].df, changed_prefs.dfxlist[entry]);
			disk_insert(i, changed_prefs.floppyslots[i].df);
			return 1;
		}
	}
	_tcscpy(changed_prefs.floppyslots[0].df, changed_prefs.dfxlist[entry]);
	disk_insert(0, changed_prefs.floppyslots[0].df);
	return 1;
}

void gui_restart()
{
	gui_running = false;
}

#ifdef USE_GUISAN
void focus_bug_workaround(const gcn::Window* wnd)
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

void update_gui_screen()
{
	const AmigaMonitor* mon = &AMonitors[0];

	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		gui_renderQuad = { 0, 0, gui_screen->w, gui_screen->h };
	else
		gui_renderQuad = { -(GUI_WIDTH - GUI_HEIGHT) / 2, (GUI_WIDTH - GUI_HEIGHT) / 2, gui_screen->w, gui_screen->h };
	
	SDL_RenderCopyEx(mon->gui_renderer, gui_texture, nullptr, &gui_renderQuad, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
	SDL_RenderPresent(mon->gui_renderer);

	if (mon->amiga_window && !kmsdrm_detected)
		show_screen(0, 0);
}
#endif

void amiberry_gui_init()
{
	AmigaMonitor* mon = &AMonitors[0];
	sdl_video_driver = SDL_GetCurrentVideoDriver();

	if (sdl_video_driver != nullptr && strcmpi(sdl_video_driver, "KMSDRM") == 0)
	{
		kmsdrm_detected = true;
		if (!mon->gui_window && mon->amiga_window)
		{
			mon->gui_window = mon->amiga_window;
		}
		if (!mon->gui_renderer && mon->amiga_renderer)
		{
			mon->gui_renderer = mon->amiga_renderer;
		}
	}
	SDL_GetCurrentDisplayMode(0, &sdl_mode);

#ifdef USE_IMGUI
	// For ImGui, let's use a portion of the screen, not a fixed size
	gui_window_rect.w = sdl_mode.w * 4 / 5;
	gui_window_rect.h = sdl_mode.h * 4 / 5;
#endif
#ifdef USE_GUISAN
	//-------------------------------------------------
	// Create new screen for GUI
	//-------------------------------------------------
	if (!gui_screen)
	{
		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create GUI surface:");
	}
#endif

	if (!mon->gui_window)
	{
		write_log("Creating Amiberry GUI window...\n");
		regqueryint(nullptr, _T("GUIPosX"), &gui_window_rect.x);
		regqueryint(nullptr, _T("GUIPosY"), &gui_window_rect.y);

        Uint32 mode;
		if (!kmsdrm_detected)
		{
			// Only enable Windowed mode if we're running under x11
			mode = SDL_WINDOW_RESIZABLE;
		}
		else
		{
			// otherwise go for Full-window
			mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}

        if (currprefs.gui_alwaysontop)
            mode |= SDL_WINDOW_ALWAYS_ON_TOP;
        if (currprefs.start_minimized)
            mode |= SDL_WINDOW_HIDDEN;
        else
            mode |= SDL_WINDOW_SHOWN;
		// Set Window allow high DPI by default
		mode |= SDL_WINDOW_ALLOW_HIGHDPI;

        if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
        {
			mon->gui_window = SDL_CreateWindow("Amiberry GUI",
				gui_window_rect.x,
				gui_window_rect.y,
				gui_window_rect.w,
				gui_window_rect.h,
				mode);
        }
        else
        {
			mon->gui_window = SDL_CreateWindow("Amiberry GUI",
				gui_window_rect.y,
				gui_window_rect.x,
				gui_window_rect.h,
				gui_window_rect.w,
				mode);
        }
        check_error_sdl(mon->gui_window == nullptr, "Unable to create window:");

		auto* const icon_surface = IMG_Load(prefix_with_data_path("amiberry.png").c_str());
		if (icon_surface != nullptr)
		{
			SDL_SetWindowIcon(mon->gui_window, icon_surface);
			SDL_FreeSurface(icon_surface);
		}
	}
	else if (kmsdrm_detected)
	{
		SDL_SetWindowSize(mon->gui_window, GUI_WIDTH, GUI_HEIGHT);
	}

	if (mon->gui_renderer == nullptr)
	{
		mon->gui_renderer = SDL_CreateRenderer(mon->gui_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(mon->gui_renderer == nullptr, "Unable to create a renderer:");
	}
	DPIHandler::set_render_scale(mon->gui_renderer);
#ifdef USE_IMGUI
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Get DPI scale factor
	gui_scale = DPIHandler::get_scale();

	// Scale font size
	io.FontGlobalScale = gui_scale;

	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(gui_scale);

	// Setup Platform/Renderer backends
	ImGui_ImplSDLRenderer2_Init(AMonitors[0].gui_renderer);
	ImGui_ImplSDL2_InitForSDLRenderer(AMonitors[0].gui_window, AMonitors[0].gui_renderer);
#endif
#ifdef USE_GUISAN
	gui_texture = SDL_CreateTexture(mon->gui_renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING, gui_screen->w,
									gui_screen->h);
	check_error_sdl(gui_texture == nullptr, "Unable to create GUI texture:");

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_HEIGHT, GUI_WIDTH);
#endif

	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);

	SDL_RaiseWindow(mon->gui_window);

#ifdef USE_GUISAN
	//-------------------------------------------------
	// Create helpers for GUI framework
	//-------------------------------------------------

	gui_imageLoader = new gcn::SDLImageLoader();
	gui_imageLoader->setRenderer(mon->gui_renderer);

	// The ImageLoader in use is static and must be set to be
	// able to load images
	gcn::Image::setImageLoader(gui_imageLoader);
	gui_graphics = new gcn::SDLGraphics();
	// Set the target for the graphics object to be the screen.
	// In other words, we will draw to the screen.
	// Note, any surface will do, it doesn't have to be the screen.
	gui_graphics->setTarget(gui_screen);
	gui_input = new gcn::SDLInput();
#endif
}

void amiberry_gui_halt()
{
	AmigaMonitor* mon = &AMonitors[0];

#ifdef USE_GUISAN
	delete gui_imageLoader;
	gui_imageLoader = nullptr;
	delete gui_input;
	gui_input = nullptr;
	delete gui_graphics;
	gui_graphics = nullptr;
#elif USE_IMGUI
	// Release any About panel resources
	if (about_logo_texture) {
		SDL_DestroyTexture(about_logo_texture);
		about_logo_texture = nullptr;
	}
	// Properly shutdown ImGui backends/context
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
#endif

	if (gui_screen != nullptr)
	{
		SDL_FreeSurface(gui_screen);
		gui_screen = nullptr;
	}
	if (gui_texture != nullptr)
	{
		SDL_DestroyTexture(gui_texture);
		gui_texture = nullptr;
	}
	if (mon->gui_renderer && !kmsdrm_detected)
	{
		SDL_DestroyRenderer(mon->gui_renderer);
		mon->gui_renderer = nullptr;
	}

	if (mon->gui_window && !kmsdrm_detected) {
		regsetint(nullptr, _T("GUIPosX"), gui_window_rect.x);
		regsetint(nullptr, _T("GUIPosY"), gui_window_rect.y);
		SDL_DestroyWindow(mon->gui_window);
		mon->gui_window = nullptr;
	}
}

#ifdef USE_GUISAN
void check_input()
{
	const AmigaMonitor* mon = &AMonitors[0];

	auto got_event = 0;
	didata* did = &di_joystick[0];
	didata* existing_did;
	
	while (SDL_PollEvent(&gui_event))
	{
		switch (gui_event.type)
		{
		case SDL_WINDOWEVENT:
			if (gui_event.window.windowID == SDL_GetWindowID(mon->gui_window))
			{
				switch (gui_event.window.event)
				{
				case SDL_WINDOWEVENT_MOVED:
					gui_window_rect.x = gui_event.window.data1;
					gui_window_rect.y = gui_event.window.data2;
					break;
				case SDL_WINDOWEVENT_RESIZED:
					gui_window_rect.w = gui_event.window.data1;
					gui_window_rect.h = gui_event.window.data2;
					break;
				default: 
					break;
				}
			}
			got_event = 1;
			break;
		
		case SDL_QUIT:
			got_event = 1;
			//-------------------------------------------------
			// Quit entire program
			//-------------------------------------------------
			uae_quit();
			gui_running = false;
			break;

		case SDL_JOYDEVICEADDED:
			// Check if we need to re-import joysticks
			existing_did = &di_joystick[gui_event.jdevice.which];
			if (existing_did->guid.empty())
			{
				write_log("GUI: SDL2 Controller/Joystick added, re-running import joysticks...\n");
				import_joysticks();
				joystick_refresh_needed = true;
				RefreshPanelInput();
			}
			return;
		case SDL_JOYDEVICEREMOVED:
			write_log("GUI: SDL2 Controller/Joystick removed, re-running import joysticks...\n");
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
					if (handle_navigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || hat & SDL_HAT_DOWN)
				{
					if (handle_navigation(DIRECTION_DOWN))
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
					if (handle_navigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				else if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || hat & SDL_HAT_LEFT)
				{
					if (handle_navigation(DIRECTION_LEFT))
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
						if (handle_navigation(DIRECTION_RIGHT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_RIGHT);
					}
					else if (gui_event.jaxis.value < -joystick_dead_zone && last_x != -1)
					{
						last_x = -1;
						if (handle_navigation(DIRECTION_LEFT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_LEFT);
					}
					else if (gui_event.jaxis.value > -joystick_dead_zone && gui_event.jaxis.value < joystick_dead_zone)
					{
						last_x = 0;
					}
				}
				else if (gui_event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
				{
					if (gui_event.jaxis.value < -joystick_dead_zone && last_y != -1)
					{
						last_y = -1;
						if (handle_navigation(DIRECTION_UP))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_UP);
					}
					else if (gui_event.jaxis.value > joystick_dead_zone && last_y != 1)
					{
						last_y = 1;
						if (handle_navigation(DIRECTION_DOWN))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_DOWN);
					}
					else if (gui_event.jaxis.value > -joystick_dead_zone && gui_event.jaxis.value < joystick_dead_zone)
					{
						last_y = 0;
					}
				}
			}
			break;

		case SDL_KEYDOWN:
			got_event = 1;
			switch (gui_event.key.keysym.sym)
			{
			case SDLK_RCTRL:
			case SDLK_LCTRL:
				ctrl_state = true;
				break;
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
				shift_state = true;
				break;
			case SDLK_RALT:
			case SDLK_LALT:
				alt_state = true;
				break;
			case SDLK_RGUI:
			case SDLK_LGUI:
				win_state = true;
				break;
			default:
				break;
			}

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
					if (handle_navigation(DIRECTION_UP))
						continue; // Don't change value when enter ComboBox -> don't send event to control
					break;

				case VK_DOWN:
					if (handle_navigation(DIRECTION_DOWN))
						continue; // Don't change value when enter ComboBox -> don't send event to control
					break;

				case VK_LEFT:
					if (handle_navigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					break;

				case VK_RIGHT:
					if (handle_navigation(DIRECTION_RIGHT))
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
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &gui_event, sizeof gui_event);
			touch_event.type = (gui_event.type == SDL_FINGERDOWN) ? SDL_MOUSEBUTTONDOWN : (gui_event.type == SDL_FINGERUP) ? SDL_MOUSEBUTTONUP : SDL_MOUSEMOTION;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = (gui_event.type == SDL_FINGERDOWN) ? SDL_PRESSED : (gui_event.type == SDL_FINGERUP) ? SDL_RELEASED : 0;
			touch_event.button.x = gui_graphics->getTarget()->w * static_cast<int>(gui_event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * static_cast<int>(gui_event.tfinger.y);
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
			switch (gui_event.key.keysym.sym)
			{
			case SDLK_RCTRL:
			case SDLK_LCTRL:
				ctrl_state = false;
				break;
			case SDLK_RSHIFT:
			case SDLK_LSHIFT:
				shift_state = false;
				break;
			case SDLK_RALT:
			case SDLK_LALT:
				alt_state = false;
				break;
			case SDLK_RGUI:
			case SDLK_LGUI:
				win_state = false;
				break;
			default:
				break;
			}
			break;

		default:
			got_event = 1;
			break;
		}

		//-------------------------------------------------
		// Send event to gui-controls
		//-------------------------------------------------
		gui_input->pushInput(gui_event);
	}
	
	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();

		SDL_RenderClear(mon->gui_renderer);

		// Now we let the Gui object draw itself.
		uae_gui->draw();
		update_gui_screen();
	}
}

void amiberry_gui_run()
{
	const AmigaMonitor* mon = &AMonitors[0];

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

	SDL_RenderClear(mon->gui_renderer);

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
		const auto source = actionEvent.getSource();
		if (source == cmdShutdown)
		{
			shutdown_host();
		}
		else if (source == cmdQuit)
		{
			quit_program();
		}
		else if (source == cmdReset)
		{
			reset_amiga();
		}
		else if (source == cmdRestart)
		{
			restart_emulator();
		}
		else if (source == cmdStart)
		{
			start_emulation();
		}
		else if (source == cmdHelp)
		{
			show_help();
		}
	}
private:
	static void shutdown_host()
	{
		uae_quit();
		gui_running = false;
		host_poweroff = true;
	}

	static void quit_program()
	{
		uae_quit();
		gui_running = false;
	}

	static void reset_amiga()
	{
		uae_reset(1, 1);
		gui_running = false;
	}

	static void restart_emulator()
	{
		char tmp[MAX_DPATH];
		get_configuration_path(tmp, sizeof tmp);
		if (strlen(last_loaded_config) > 0)
			strncat(tmp, last_loaded_config, MAX_DPATH - 1);
		else
		{
			strncat(tmp, OPTIONSFILENAME, MAX_DPATH - 1);
			strncat(tmp, ".uae", MAX_DPATH - 10);
		}
		uae_restart(&changed_prefs, -1, tmp);
		gui_running = false;
	}

	static void start_emulation()
	{
		if (emulating && cmdStart->isEnabled())
		{
			gui_running = false;
		}
		else
		{
			uae_reset(0, 1);
			gui_running = false;
		}
	}

	static void show_help()
	{
		show_help_requested();
		cmdHelp->requestFocus();
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
	// Create GUI
	//-------------------------------------------------
	uae_gui = new gcn::Gui();
	uae_gui->setGraphics(gui_graphics);
	uae_gui->setInput(gui_input);

	//-------------------------------------------------
	// Initialize fonts
	//-------------------------------------------------
	TTF_Init();

	load_theme(amiberry_options.gui_theme);
	apply_theme();

	//-------------------------------------------------
	// Create container for main page
	//-------------------------------------------------
	gui_top = new gcn::Container();
	gui_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
	gui_top->setBaseColor(gui_base_color);
	gui_top->setBackgroundColor(gui_base_color);
	gui_top->setForegroundColor(gui_foreground_color);
	uae_gui->setTop(gui_top);

	//--------------------------------------------------
	// Create main buttons
	//--------------------------------------------------
	mainButtonActionListener = new MainButtonActionListener();

	cmdQuit = new gcn::Button("Quit");
	cmdQuit->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdQuit->setBaseColor(gui_base_color);
	cmdQuit->setForegroundColor(gui_foreground_color);
	cmdQuit->setId("Quit");
	cmdQuit->addActionListener(mainButtonActionListener);

	cmdShutdown = new gcn::Button("Shutdown");
	cmdShutdown->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdShutdown->setBaseColor(gui_base_color);
	cmdShutdown->setForegroundColor(gui_foreground_color);
	cmdShutdown->setId("Shutdown");
	cmdShutdown->addActionListener(mainButtonActionListener);

	cmdReset = new gcn::Button("Reset");
	cmdReset->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdReset->setBaseColor(gui_base_color);
	cmdReset->setForegroundColor(gui_foreground_color);
	cmdReset->setId("Reset");
	cmdReset->addActionListener(mainButtonActionListener);

	cmdRestart = new gcn::Button("Restart");
	cmdRestart->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdRestart->setBaseColor(gui_base_color);
	cmdRestart->setForegroundColor(gui_foreground_color);
	cmdRestart->setId("Restart");
	cmdRestart->addActionListener(mainButtonActionListener);

	cmdStart = new gcn::Button("Start");
	if (emulating)
		cmdStart->setCaption("Resume");
	cmdStart->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdStart->setBaseColor(gui_base_color);
	cmdStart->setForegroundColor(gui_foreground_color);
	cmdStart->setId("Start");
	cmdStart->addActionListener(mainButtonActionListener);

	cmdHelp = new gcn::Button("Help");
	cmdHelp->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdHelp->setBaseColor(gui_base_color);
	cmdHelp->setForegroundColor(gui_foreground_color);
	cmdHelp->setId("Help");
	cmdHelp->addActionListener(mainButtonActionListener);

	//--------------------------------------------------
	// Create selector entries
	//--------------------------------------------------
	constexpr auto workAreaHeight = GUI_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y;
	selectors = new gcn::Container();
	selectors->setFrameSize(0);
	selectors->setBaseColor(gui_base_color);
	selectors->setBackgroundColor(gui_base_color);
	selectors->setForegroundColor(gui_foreground_color);

	constexpr auto selectorScrollAreaWidth = SELECTOR_WIDTH + 2;
	selectorsScrollArea = new gcn::ScrollArea();
	selectorsScrollArea->setContent(selectors);
	selectorsScrollArea->setBaseColor(gui_base_color);
	selectorsScrollArea->setBackgroundColor(gui_background_color);
	selectorsScrollArea->setForegroundColor(gui_foreground_color);
	selectorsScrollArea->setSize(selectorScrollAreaWidth, workAreaHeight);
	selectorsScrollArea->setFrameSize(1);
	
	const auto panelStartX = DISTANCE_BORDER + selectorsScrollArea->getWidth() + DISTANCE_BORDER;

	int selectorsHeight = 0;
	panelFocusListener = new PanelFocusListener();
	for (i = 0; categories[i].category != nullptr; ++i)
	{
		categories[i].selector = new gcn::SelectorEntry(categories[i].category, prefix_with_data_path(categories[i].imagepath));
		categories[i].selector->setActiveColor(gui_selector_active_color);
		categories[i].selector->setInactiveColor(gui_selector_inactive_color);
		categories[i].selector->setSize(SELECTOR_WIDTH, SELECTOR_HEIGHT);
		categories[i].selector->addFocusListener(panelFocusListener);

		categories[i].panel = new gcn::Container();
		categories[i].panel->setId(categories[i].category);
		categories[i].panel->setSize(GUI_WIDTH - panelStartX - DISTANCE_BORDER, workAreaHeight);
		categories[i].panel->setBaseColor(gui_base_color);
		categories[i].panel->setForegroundColor(gui_foreground_color);
		categories[i].panel->setFrameSize(1);
		categories[i].panel->setVisible(false);
			
		selectorsHeight += categories[i].selector->getHeight();
	}

	selectors->setSize(SELECTOR_WIDTH, selectorsHeight);

	// These need to be called after the selectors have been created
	apply_theme_extras();

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
	gui_top->add(cmdShutdown, DISTANCE_BORDER, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
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

	for (i = 0, yPos = 0; categories[i].category != nullptr; ++i, yPos += SELECTOR_HEIGHT)
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
	delete uae_gui;
	uae_gui = nullptr;
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
#endif

#ifdef USE_GUISAN
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
		std::cout << e.getMessage() << '\n';
		uae_quit();
	}

	// Catch all Std exceptions.
	catch (exception& e)
	{
		std::cout << "Std exception: " << e.what() << '\n';
		uae_quit();
	}

	// Catch all unknown exceptions.
	catch (...)
	{
		std::cout << "Unknown exception" << '\n';
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
			quit_program = -UAE_RESET_HARD; // Hard reset required...
	}

	// Reset counter for access violations
	init_max_signals();
}
#elif USE_IMGUI
void ShowMessageBox(const char* title, const char* message)
{
	strncpy(message_box_title, title, sizeof(message_box_title) - 1);
	strncpy(message_box_message, message, sizeof(message_box_message) - 1);
	show_message_box = true;
}

static void render_panel_about()
{
	// Ensure logo texture is available
	ensure_about_logo_texture();

	// Draw centered logo banner, scaled to content width
	if (about_logo_texture)
	{
		const float region_w = ImGui::GetContentRegionAvail().x;
		auto draw_w = static_cast<float>(about_logo_tex_w);
		auto draw_h = static_cast<float>(about_logo_tex_h);
		if (draw_w > region_w * 0.9f)
		{
			const float scale = (region_w * 0.9f) / draw_w;
			draw_w *= scale;
			draw_h *= scale;
		}
		// center horizontally
		const float pos_x = (ImGui::GetContentRegionAvail().x - draw_w) * 0.5f;
		if (pos_x > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pos_x);
		ImGui::Image((ImTextureID)about_logo_texture, ImVec2(draw_w, draw_h));
	}

	ImGui::Spacing();

	// Version and environment info
	ImGui::TextUnformatted(get_version_string().c_str());
	ImGui::TextUnformatted(get_copyright_notice().c_str());
	ImGui::TextUnformatted(get_sdl2_version_string().c_str());
	ImGui::Text("SDL2 video driver: %s", sdl_video_driver ? sdl_video_driver : "unknown");

	ImGui::Separator();

	// License and credits text inside a bordered, scrollable region
	static auto about_long_text =
		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"any later version.\n\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"GNU General Public License for more details.\n\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program. If not, see https://www.gnu.org/licenses\n\n"
		"Credits:\n"
		"Dimitris Panokostas (MiDWaN) - Amiberry author\n"
		"Toni Wilen - WinUAE author\n"
		"TomB - Original ARM port of UAE, JIT ARM updates\n"
		"Rob Smith, Drawbridge floppy controller\n"
		"Gareth Fare - Original Serial port implementation\n"
		"Dom Cresswell (Horace & The Spider) - Controller and WHDBooter updates\n"
		"Christer Solskogen - Makefile and testing\n"
		"Gunnar Kristjansson - Amibian and inspiration\n"
		"Thomas Navarro Garcia - Original Amiberry logo\n"
		"Chips - Original RPI port\n"
		"Vasiliki Soufi - Amiberry name\n\n"
		"Dedicated to HeZoR - R.I.P. little brother (1978-2017)\n";

	// Use a child region with border to mimic a textbox look
	ImGui::BeginChild("AboutScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::TextUnformatted(about_long_text);
	ImGui::EndChild();
}

static void render_panel_paths()
{
	char tmp[MAX_DPATH];
	ImGui::Text("System ROMs:");
	get_rom_path(tmp, sizeof tmp);
	ImGui::InputText("##SystemROMs", tmp, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##SystemROMs"))
	{
		//std::string path = SelectFolder("Folder for System ROMs", changed_prefs.path_rom.path[0]);
		//if (!path.empty())
		//	set_rom_path(path);
	}
	ImGui::Text("Configuration files:");
	get_configuration_path(tmp, sizeof tmp);
	ImGui::InputText("##ConfigPath", tmp, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##ConfigPath"))
	{
		//std::string path = SelectFolder("Folder for configuration files", tmp);
		//if (!path.empty())
		//	set_configuration_path(path);
	}
}

static void render_panel_quickstart()
{
	const char* models[] = { "Amiga 500", "Amiga 500+", "Amiga 600", "Amiga 1000", "Amiga 1200", "Amiga 3000", "Amiga 4000", "Amiga 4000T", "CD32", "CDTV", "American Laser Games / Picmatic", "Arcadia Multi Select system", "Macrosystem" };
	ImGui::Combo("Amiga model", &quickstart_model, models, IM_ARRAYSIZE(models));

	const char* configs[] = { "1.3 ROM, OCS, 512 KB Chip + 512 KB Slow RAM (most common)", "1.3 ROM, ECS Agnus, 512 KB Chip RAM + 512 KB Slow RAM", "1.3 ROM, ECS Agnus, 1 MB Chip RAM", "1.3 ROM, OCS Agnus, 512 KB Chip RAM", "1.2 ROM, OCS Agnus, 512 KB Chip RAM", "1.2 ROM, OCS Agnus, 512 KB Chip RAM + 512 KB Slow RAM" };
	ImGui::Combo("Config", &quickstart_conf, configs, IM_ARRAYSIZE(configs));

	ImGui::Checkbox("NTSC", &changed_prefs.ntscmode);

	for (int i = 0; i < 2; ++i)
	{
		char label[10];
		snprintf(label, 10, "DF%d:", i);
		ImGui::Checkbox(label, (bool*)&changed_prefs.floppyslots[i].dfxtype);
		ImGui::SameLine();
		ImGui::Checkbox("Write-protected", &changed_prefs.floppy_read_only);
	}

	ImGui::Checkbox("CD drive", &changed_prefs.cdslots[0].inuse);

	if (ImGui::Button("Set configuration"))
		built_in_prefs(&changed_prefs, quickstart_model, quickstart_conf, 0, 0);
}

static void render_panel_configurations()
{
	static int selected = -1;
	ImGui::BeginChild("ConfigList", ImVec2(0, -100), true);
	for (int i = 0; i < ConfigFilesList.size(); ++i)
	{
		if (ImGui::Selectable(ConfigFilesList[i]->Name, selected == i))
			selected = i;
	}
	ImGui::EndChild();

	static char name[MAX_DPATH] = "";
	static char desc[MAX_DPATH] = "";
	if (selected != -1)
	{
		strncpy(name, ConfigFilesList[selected]->Name, MAX_DPATH);
		strncpy(desc, ConfigFilesList[selected]->Description, MAX_DPATH);
	}
	ImGui::InputText("Name", name, MAX_DPATH);
	ImGui::InputText("Description", desc, MAX_DPATH);

	if (ImGui::Button("Load"))
	{
		if (selected != -1)
			target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		char filename[MAX_DPATH];
		get_configuration_path(filename, MAX_DPATH);
		strncat(filename, name, MAX_DPATH - 1);
		strncat(filename, ".uae", MAX_DPATH - 10);
		strncpy(changed_prefs.description, desc, 256);
		if (cfgfile_save(&changed_prefs, filename, 0))
		{
			strncpy(last_active_config, name, MAX_DPATH);
			ReadConfigFileList();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		if (selected != -1)
		{
			remove(ConfigFilesList[selected]->FullPath);
			ReadConfigFileList();
			selected = -1;
		}
	}
}

static void render_panel_cpu()
{
	ImGui::RadioButton("68000", &changed_prefs.cpu_model, 68000);
	ImGui::RadioButton("68010", &changed_prefs.cpu_model, 68010);
	ImGui::RadioButton("68020", &changed_prefs.cpu_model, 68020);
	ImGui::RadioButton("68030", &changed_prefs.cpu_model, 68030);
	ImGui::RadioButton("68040", &changed_prefs.cpu_model, 68040);
	ImGui::RadioButton("68060", &changed_prefs.cpu_model, 68060);
	ImGui::Checkbox("24-bit addressing", &changed_prefs.address_space_24);
	ImGui::Checkbox("More compatible", &changed_prefs.cpu_compatible);
	ImGui::Checkbox("Data cache", &changed_prefs.cpu_data_cache);
	ImGui::Checkbox("JIT", (bool*)&changed_prefs.cachesize);

	ImGui::Separator();
	ImGui::Text("MMU");
	ImGui::RadioButton("None##MMU", &changed_prefs.mmu_model, 0);
	ImGui::RadioButton("MMU", &changed_prefs.mmu_model, changed_prefs.cpu_model);
	ImGui::Checkbox("EC", &changed_prefs.mmu_ec);

	ImGui::Separator();
	ImGui::Text("FPU");
	ImGui::RadioButton("None##FPU", &changed_prefs.fpu_model, 0);
	ImGui::RadioButton("68881", &changed_prefs.fpu_model, 68881);
	ImGui::RadioButton("68882", &changed_prefs.fpu_model, 68882);
	ImGui::RadioButton("CPU internal", &changed_prefs.fpu_model, changed_prefs.cpu_model);
	ImGui::Checkbox("More compatible##FPU", &changed_prefs.fpu_strict);

	ImGui::Separator();
	ImGui::Text("CPU Speed");
	ImGui::RadioButton("Fastest Possible", &changed_prefs.m68k_speed, -1);
	ImGui::RadioButton("A500/A1200 or cycle exact", &changed_prefs.m68k_speed, 0);
	ImGui::SliderInt("CPU Speed", (int*)&changed_prefs.m68k_speed_throttle, 0, 5000);
	ImGui::SliderInt("CPU Idle", &changed_prefs.cpu_idle, 0, 120);

	ImGui::Separator();
	ImGui::Text("Cycle-Exact CPU Emulation Speed");
	const char* freq_items[] = { "1x", "2x (A500)", "4x (A1200)", "8x", "16x" };
	ImGui::Combo("CPU Frequency", &changed_prefs.cpu_clock_multiplier, freq_items, IM_ARRAYSIZE(freq_items));
	ImGui::Checkbox("Multi-threaded CPU", &changed_prefs.cpu_thread);

	ImGui::Separator();
	ImGui::Text("PowerPC CPU Options");
	ImGui::Checkbox("PPC emulation", (bool*)&changed_prefs.ppc_mode);
	ImGui::SliderInt("Stopped M68K CPU Idle", &changed_prefs.ppc_cpu_idle, 0, 10);

	ImGui::Separator();
	ImGui::Text("Advanced JIT Settings");
	ImGui::SliderInt("Cache size", &changed_prefs.cachesize, 0, 8192);
	ImGui::Checkbox("FPU Support##JIT", &changed_prefs.compfpu);
	ImGui::Checkbox("Constant jump", &changed_prefs.comp_constjump);
	ImGui::Checkbox("Hard flush", &changed_prefs.comp_hardflush);
	ImGui::RadioButton("Direct##memaccess", &changed_prefs.comptrustbyte, 0);
	ImGui::RadioButton("Indirect##memaccess", &changed_prefs.comptrustbyte, 1);
	ImGui::Checkbox("No flags", &changed_prefs.compnf);
	ImGui::Checkbox("Catch unexpected exceptions", &changed_prefs.comp_catchfault);
}

static void render_panel_chipset()
{
	ImGui::RadioButton("OCS", (int*)&changed_prefs.chipset_mask, 0);
	ImGui::RadioButton("ECS Agnus", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS);
	ImGui::RadioButton("ECS Denise", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_DENISE);
	ImGui::RadioButton("Full ECS", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE);
	ImGui::RadioButton("AGA", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA);
	ImGui::Checkbox("NTSC", &changed_prefs.ntscmode);
	ImGui::Checkbox("Cycle Exact (Full)", &changed_prefs.cpu_cycle_exact);
	ImGui::Checkbox("Cycle Exact (DMA/Memory)", &changed_prefs.cpu_memory_cycle_exact);
	const char* chipset_items[] = { "Generic", "CDTV", "CDTV-CR", "CD32", "A500", "A500+", "A600", "A1000", "A1200", "A2000", "A3000", "A3000T", "A4000", "A4000T", "Velvet", "Casablanca", "DraCo" };
	ImGui::Combo("Chipset Extra", &changed_prefs.cs_compatible, chipset_items, IM_ARRAYSIZE(chipset_items));

	ImGui::Separator();
	ImGui::Text("Options");
	ImGui::Checkbox("Immediate Blitter", &changed_prefs.immediate_blits);
	ImGui::Checkbox("Wait for Blitter", (bool*)&changed_prefs.waiting_blits);
	ImGui::Checkbox("Multithreaded Drawing", (bool*)&multithread_enabled);
	const char* monitor_items[] = { "-", "Autodetect" };
	ImGui::Combo("Video port display hardware", &changed_prefs.monitoremu, monitor_items, IM_ARRAYSIZE(monitor_items));

	ImGui::Separator();
	ImGui::Text("Keyboard");
	const char* keyboard_items[] = { "Keyboard disconnected", "UAE High level emulation", "A500 / A500 + (6500 - 1 MCU)", "A600 (6570 - 036 MCU)", "A1000 (6500 - 1 MCU. ROM not yet dumped)", "A1000 (6570 - 036 MCU)", "A1200 (68HC05C MCU)", "A2000 (Cherry, 8039 MCU)", "A2000/A3000/A4000 (6570-036 MCU)" };
	ImGui::Combo("Keyboard Layout", &changed_prefs.keyboard_mode, keyboard_items, IM_ARRAYSIZE(keyboard_items));
	ImGui::Checkbox("Keyboard N-key rollover", &changed_prefs.keyboard_nkro);

	ImGui::Separator();
	ImGui::Text("Collision Level");
	ImGui::RadioButton("None##Collision", &changed_prefs.collision_level, 0);
	ImGui::RadioButton("Sprites only", &changed_prefs.collision_level, 1);
	ImGui::RadioButton("Sprites and Sprites vs. Playfield", &changed_prefs.collision_level, 2);
	ImGui::RadioButton("Full (rarely needed)", &changed_prefs.collision_level, 3);
}

static void render_panel_rom()
{
	ImGui::Text("Main ROM File:");
	ImGui::InputText("##MainROM", changed_prefs.romfile, MAX_DPATH);
	ImGui::Text("Extended ROM File:");
	ImGui::InputText("##ExtROM", changed_prefs.romextfile, MAX_DPATH);
	ImGui::Text("Cartridge ROM File:");
	ImGui::InputText("##CartROM", changed_prefs.cartfile, MAX_DPATH);
	ImGui::Checkbox("MapROM emulation", (bool*)&changed_prefs.maprom);
	ImGui::Checkbox("ShapeShifter support", &changed_prefs.kickshifter);
	const char* uae_items[] = { "ROM disabled", "Original UAE (FS + F0 ROM)", "New UAE (64k + F0 ROM)", "New UAE (128k, ROM, Direct)", "New UAE (128k, ROM, Indirect)" };
	ImGui::Combo("Advanced UAE expansion board/Boot ROM", &changed_prefs.uaeboard, uae_items, IM_ARRAYSIZE(uae_items));
}

static void render_panel_ram()
{
    // Use temporary ints to avoid aliasing issues with ImGui writing into uae_u32 fields
    int chip = static_cast<int>(changed_prefs.chipmem.size);
    int slow = static_cast<int>(changed_prefs.bogomem.size);
    int z2fast0 = static_cast<int>(changed_prefs.fastmem[0].size);
    int z3fast0 = static_cast<int>(changed_prefs.z3fastmem[0].size);
    int z3chip = static_cast<int>(changed_prefs.z3chipmem.size);
    int mb_low = static_cast<int>(changed_prefs.mbresmem_low.size);
    int mb_high = static_cast<int>(changed_prefs.mbresmem_high.size);

    if (ImGui::SliderInt("Chip", &chip, 0, 0x800000))
        changed_prefs.chipmem.size = static_cast<uae_u32>(chip);
    if (ImGui::SliderInt("Slow", &slow, 0, 0x180000))
        changed_prefs.bogomem.size = static_cast<uae_u32>(slow);
    if (ImGui::SliderInt("Z2 Fast", &z2fast0, 0, 0x800000))
        changed_prefs.fastmem[0].size = static_cast<uae_u32>(z2fast0);
    if (ImGui::SliderInt("Z3 Fast", &z3fast0, 0, 0x40000000))
        changed_prefs.z3fastmem[0].size = static_cast<uae_u32>(z3fast0);
    if (ImGui::SliderInt("32-bit Chip RAM", &z3chip, 0, 0x40000000))
        changed_prefs.z3chipmem.size = static_cast<uae_u32>(z3chip);
    if (ImGui::SliderInt("Motherboard Fast RAM", &mb_low, 0, 0x8000000))
        changed_prefs.mbresmem_low.size = static_cast<uae_u32>(mb_low);
    if (ImGui::SliderInt("Processor slot Fast RAM", &mb_high, 0, 0x8000000))
        changed_prefs.mbresmem_high.size = static_cast<uae_u32>(mb_high);

    const char* z3_mapping_items[] = { "Automatic (*)", "UAE (0x10000000)", "Real (0x40000000)" };
    ImGui::Combo("Z3 Mapping Mode", &changed_prefs.z3_mapping_mode, z3_mapping_items, IM_ARRAYSIZE(z3_mapping_items));
}

static void render_panel_floppy()
{
    for (int i = 0; i < 4; ++i)
    {
        ImGui::PushID(i);
        char label[16];
        snprintf(label, sizeof(label), "DF%d:", i);
        ImGui::Checkbox(label, (bool*)&changed_prefs.floppyslots[i].dfxtype);
        ImGui::SameLine();
        ImGui::Checkbox("Write-protected", &changed_prefs.floppy_read_only);
        ImGui::SameLine();
        // Use a unique ID for each input to avoid ImGui ID collisions
        if (ImGui::InputText("##FloppyPath", changed_prefs.floppyslots[i].df, MAX_DPATH)) {
            // Optionally sanitize path here if needed
        }
        ImGui::PopID();
    }
    ImGui::SliderInt("Floppy Drive Emulation Speed", &changed_prefs.floppy_speed, 0, 800);
    if (ImGui::Button("Create 3.5\" DD disk"))
    {
        // Create 3.5" DD Disk
    }
    if (ImGui::Button("Create 3.5\" HD disk"))
    {
        // Create 3.5" HD Disk
    }
    if (ImGui::Button("Save config for disk"))
    {
        // Save configuration for current disk
    }
}

static void render_panel_hd()
{
	for (int i = 0; i < changed_prefs.mountitems; ++i)
	{
		ImGui::Text("Device: %s, Volume: %s, Path: %s", changed_prefs.mountconfig[i].ci.devname, changed_prefs.mountconfig[i].ci.volname, changed_prefs.mountconfig[i].ci.rootdir);
	}
	if (ImGui::Button("Add Directory/Archive"))
	{
		// Add Directory/Archive
	}
	if (ImGui::Button("Add Hardfile"))
	{
		// Add Hardfile
	}
	if (ImGui::Button("Add Hard Drive"))
	{
		// Add Hard Drive
	}
	if (ImGui::Button("Add CD Drive"))
	{
		// Add CD Drive
	}
	if (ImGui::Button("Add Tape Drive"))
	{
		// Add Tape Drive
	}
	if (ImGui::Button("Create Hardfile"))
	{
		// Create Hardfile
	}
	ImGui::Checkbox("CDFS automount CD/DVD drives", &changed_prefs.automount_cddrives);
	ImGui::Checkbox("CD drive/image", &changed_prefs.cdslots[0].inuse);
	ImGui::InputText("##CDFile", changed_prefs.cdslots[0].name, MAX_DPATH);
	if (ImGui::Button("Eject"))
	{
		// Eject CD
	}
	if (ImGui::Button("Select image file"))
	{
		// Select CD image file
	}
	ImGui::Checkbox("CDTV/CDTV-CR/CD32 turbo CD read speed", (bool*)&changed_prefs.cd_speed);
}

void render_panel_expansions()
{
	ImGui::Text("Expansion Board Settings");
	// TODO: Implement Expansion Board Settings

	ImGui::Separator();
	ImGui::Text("Accelerator Board Settings");
	// TODO: Implement Accelerator Board Settings

	ImGui::Separator();
	ImGui::Text("Miscellaneous Expansions");
	ImGui::Checkbox("bsdsocket.library", &changed_prefs.socket_emu);
	ImGui::Checkbox("uaescsi.device", (bool*)&changed_prefs.scsi);
	ImGui::Checkbox("CD32 Full Motion Video cartridge", &changed_prefs.cs_cd32fmv);
	ImGui::Checkbox("uaenet.device", &changed_prefs.sana2);
}

void render_panel_rtg()
{
	const char* rtg_boards[] = { "-", "UAE Zorro II", "UAE Zorro III", "PCI bridgeboard" };
	ImGui::Combo("RTG Graphics Board", &changed_prefs.rtgboards[0].rtgmem_type, rtg_boards, IM_ARRAYSIZE(rtg_boards));

    int vram = static_cast<int>(changed_prefs.rtgboards[0].rtgmem_size);
	if (ImGui::SliderInt("VRAM size", &vram, 0, 0x10000000))
        changed_prefs.rtgboards[0].rtgmem_size = static_cast<uae_u32>(vram);
	ImGui::Checkbox("Scale if smaller than display size setting", (bool*)&changed_prefs.gf[1].gfx_filter_autoscale);
	ImGui::Checkbox("Always scale in windowed mode", &changed_prefs.rtgallowscaling);
	ImGui::Checkbox("Always center", (bool*)&changed_prefs.gf[1].gfx_filter_autoscale);
	ImGui::Checkbox("Hardware vertical blank interrupt", &changed_prefs.rtg_hardwareinterrupt);
	ImGui::Checkbox("Hardware sprite emulation", &changed_prefs.rtg_hardwaresprite);
	ImGui::Checkbox("Multithreaded", &changed_prefs.rtg_multithread);
	const char* rtg_refreshrates[] = { "Chipset", "Default", "50", "60", "70", "75" };
	ImGui::Combo("Refresh rate", &changed_prefs.rtgvblankrate, rtg_refreshrates, IM_ARRAYSIZE(rtg_refreshrates));
	const char* rtg_buffermodes[] = { "Double buffering", "Triple buffering" };
	ImGui::Combo("Buffer mode", &changed_prefs.gfx_apmode[1].gfx_backbuffers, rtg_buffermodes, IM_ARRAYSIZE(rtg_buffermodes));
	const char* rtg_aspectratios[] = { "Disabled", "Automatic" };
	ImGui::Combo("Aspect Ratio", &changed_prefs.rtgscaleaspectratio, rtg_aspectratios, IM_ARRAYSIZE(rtg_aspectratios));
	const char* rtg_16bit_modes[] = { "(15/16bit)", "All", "R5G6B5PC (*)", "R5G5B5PC", "R5G6B5", "R5G5B5", "B5G6R5PC", "B5G5R5PC" };
	ImGui::Combo("16-bit modes", (int*)&changed_prefs.picasso96_modeflags, rtg_16bit_modes, IM_ARRAYSIZE(rtg_16bit_modes));
	const char* rtg_32bit_modes[] = { "(32bit)", "All", "A8R8G8B8", "A8B8G8R8", "R8G8B8A8 (*)", "B8G8R8A8" };
	ImGui::Combo("32-bit modes", (int*)&changed_prefs.picasso96_modeflags, rtg_32bit_modes, IM_ARRAYSIZE(rtg_32bit_modes));
}

static void render_panel_hwinfo()
{
	ImGui::BeginChild("HWInfoList", ImVec2(0, -50), true);
	ImGui::Columns(6, "HWInfoColumns");
	ImGui::Separator();
	ImGui::Text("Type"); ImGui::NextColumn();
	ImGui::Text("Name"); ImGui::NextColumn();
	ImGui::Text("Start"); ImGui::NextColumn();
	ImGui::Text("End"); ImGui::NextColumn();
	ImGui::Text("Size"); ImGui::NextColumn();
	ImGui::Text("ID"); ImGui::NextColumn();
	ImGui::Separator();
	for (int i = 0; i < MAX_INFOS; ++i)
	{
		struct autoconfig_info* aci = expansion_get_autoconfig_data(&changed_prefs, i);
		if (aci)
		{
			ImGui::Text("%s", aci->zorro >= 1 && aci->zorro <= 3 ? std::to_string(aci->zorro).c_str() : "-"); ImGui::NextColumn();
			ImGui::Text("%s", aci->name); ImGui::NextColumn();
			ImGui::Text("0x%08x", aci->start); ImGui::NextColumn();
			ImGui::Text("0x%08x", aci->start + aci->size - 1); ImGui::NextColumn();
			ImGui::Text("0x%08x", aci->size); ImGui::NextColumn();
			ImGui::Text("0x%04x/0x%02x", (aci->autoconfig_bytes[4] << 8) | aci->autoconfig_bytes[5], aci->autoconfig_bytes[1]); ImGui::NextColumn();
		}
	}
	ImGui::Columns(1);
	ImGui::EndChild();

	ImGui::Checkbox("Custom board order", &changed_prefs.autoconfig_custom_sort);
	if (ImGui::Button("Move up"))
	{
		// Move up
	}
	ImGui::SameLine();
	if (ImGui::Button("Move down"))
	{
		// Move down
	}
}

static void render_panel_display()
{
	ImGui::Text("Amiga Screen");
	const char* screenmode_items[] = { "Windowed", "Fullscreen", "Full-window" };
	ImGui::Combo("Screen mode", &changed_prefs.gfx_apmode[0].gfx_fullscreen, screenmode_items, IM_ARRAYSIZE(screenmode_items));
	ImGui::Checkbox("Manual Crop", &changed_prefs.gfx_manual_crop);
	ImGui::SliderInt("Width", &changed_prefs.gfx_manual_crop_width, 0, 800);
	ImGui::SliderInt("Height", &changed_prefs.gfx_manual_crop_height, 0, 600);
	ImGui::Checkbox("Auto Crop", &changed_prefs.gfx_auto_crop);
	ImGui::Checkbox("Borderless", &changed_prefs.borderless);
	const char* vsync_items[] = { "-", "Lagless", "Lagless 50/60Hz", "Standard", "Standard 50/60Hz" };
	ImGui::Combo("VSync Native", &changed_prefs.gfx_apmode[0].gfx_vsync, vsync_items, IM_ARRAYSIZE(vsync_items));
	ImGui::Combo("VSync RTG", &changed_prefs.gfx_apmode[1].gfx_vsync, vsync_items, IM_ARRAYSIZE(vsync_items));
	ImGui::SliderInt("H. Offset", &changed_prefs.gfx_horizontal_offset, -80, 80);
	ImGui::SliderInt("V. Offset", &changed_prefs.gfx_vertical_offset, -80, 80);

	ImGui::Separator();
	ImGui::Text("Centering");
	ImGui::Checkbox("Horizontal", (bool*)&changed_prefs.gfx_xcenter);
	ImGui::Checkbox("Vertical", (bool*)&changed_prefs.gfx_ycenter);

	ImGui::Separator();
	ImGui::Text("Line mode");
	ImGui::RadioButton("Single", &changed_prefs.gfx_vresolution, 0);
	ImGui::RadioButton("Double", &changed_prefs.gfx_vresolution, 1);
	ImGui::RadioButton("Scanlines", &changed_prefs.gfx_pscanlines, 1);
	ImGui::RadioButton("Double, fields", &changed_prefs.gfx_pscanlines, 2);
	ImGui::RadioButton("Double, fields+", &changed_prefs.gfx_pscanlines, 3);

	ImGui::Separator();
	ImGui::Text("Interlaced line mode");
	ImGui::RadioButton("Single##Interlaced", &changed_prefs.gfx_iscanlines, 0);
	ImGui::RadioButton("Double, frames", &changed_prefs.gfx_iscanlines, 0);
	ImGui::RadioButton("Double, fields##Interlaced", &changed_prefs.gfx_iscanlines, 1);
	ImGui::RadioButton("Double, fields+##Interlaced", &changed_prefs.gfx_iscanlines, 2);

	ImGui::Separator();
	const char* scaling_items[] = { "Auto", "Pixelated", "Smooth", "Integer" };
	ImGui::Combo("Scaling method", &changed_prefs.scaling_method, scaling_items, IM_ARRAYSIZE(scaling_items));
	const char* resolution_items[] = { "LowRes", "HighRes (normal)", "SuperHighRes" };
	ImGui::Combo("Resolution", &changed_prefs.gfx_resolution, resolution_items, IM_ARRAYSIZE(resolution_items));
	ImGui::Checkbox("Filtered Low Res", (bool*)&changed_prefs.gfx_lores_mode);
	const char* res_autoswitch_items[] = { "Disabled", "Always On", "10%", "33%", "66%" };
	ImGui::Combo("Res. autoswitch", &changed_prefs.gfx_autoresolution, res_autoswitch_items, IM_ARRAYSIZE(res_autoswitch_items));
	ImGui::Checkbox("Frameskip", (bool*)&changed_prefs.gfx_framerate);
	ImGui::SliderInt("Refresh", &changed_prefs.gfx_framerate, 1, 10);
	ImGui::Checkbox("FPS Adj:", (bool*)&changed_prefs.cr[changed_prefs.cr_selected].locked);
	ImGui::SliderFloat("##FPSAdj", &changed_prefs.cr[changed_prefs.cr_selected].rate, 1, 100);
	ImGui::Checkbox("Correct Aspect Ratio", (bool*)&changed_prefs.gfx_correct_aspect);
	ImGui::Checkbox("Blacker than black", &changed_prefs.gfx_blackerthanblack);
	ImGui::Checkbox("Remove interlace artifacts", &changed_prefs.gfx_scandoubler);
	ImGui::SliderInt("Brightness", &changed_prefs.gfx_luminance, -200, 200);
}

static void render_panel_sound()
{
	ImGui::Text("Sound Emulation");
	ImGui::RadioButton("Disabled", &changed_prefs.produce_sound, 0);
	ImGui::RadioButton("Disabled, but emulated", &changed_prefs.produce_sound, 1);
	ImGui::RadioButton("Enabled", &changed_prefs.produce_sound, 2);
	ImGui::Checkbox("Automatic switching", &changed_prefs.sound_auto);

	ImGui::Separator();
	ImGui::Text("Volume");
	ImGui::SliderInt("Paula Volume", &changed_prefs.sound_volume_paula, 0, 100);
	ImGui::SliderInt("CD Volume", &changed_prefs.sound_volume_cd, 0, 100);
	ImGui::SliderInt("AHI Volume", &changed_prefs.sound_volume_board, 0, 100);
	ImGui::SliderInt("MIDI Volume", &changed_prefs.sound_volume_midi, 0, 100);

	ImGui::Separator();
	ImGui::Text("Floppy Drive Sound Emulation");
	ImGui::Checkbox("Enable floppy drive sound", (bool*)&changed_prefs.floppyslots[0].dfxclick);
	ImGui::SliderInt("Empty drive", &changed_prefs.dfxclickvolume_empty[0], 0, 100);
	ImGui::SliderInt("Disk in drive", &changed_prefs.dfxclickvolume_disk[0], 0, 100);

	ImGui::Separator();
	ImGui::Text("Sound Buffer Size");
	ImGui::SliderInt("##SoundBufferSize", &changed_prefs.sound_maxbsiz, 0, 65536);
	ImGui::RadioButton("Pull audio", &changed_prefs.sound_pullmode, 1);
	ImGui::RadioButton("Push audio", &changed_prefs.sound_pullmode, 0);

	ImGui::Separator();
	ImGui::Text("Options");
	const char* soundcard_items[] = { "-" };
	ImGui::Combo("Device", &changed_prefs.soundcard, soundcard_items, IM_ARRAYSIZE(soundcard_items));
	ImGui::Checkbox("System default", &changed_prefs.soundcard_default);
	const char* channel_mode_items[] = { "Mono", "Stereo", "Cloned stereo (4 channels)", "4 Channels", "Cloned stereo (5.1)", "5.1 Channels", "Cloned stereo (7.1)", "7.1 channels" };
	ImGui::Combo("Channel mode", &changed_prefs.sound_stereo, channel_mode_items, IM_ARRAYSIZE(channel_mode_items));
	const char* frequency_items[] = { "11025", "22050", "32000", "44100", "48000" };
	ImGui::Combo("Frequency", &changed_prefs.sound_freq, frequency_items, IM_ARRAYSIZE(frequency_items));
	const char* interpolation_items[] = { "Disabled", "Anti", "Sinc", "RH", "Crux" };
	ImGui::Combo("Interpolation", &changed_prefs.sound_interpol, interpolation_items, IM_ARRAYSIZE(interpolation_items));
	const char* filter_items[] = { "Always off", "Emulated (A500)", "Emulated (A1200)", "Always on (A500)", "Always on (A1200)", "Always on (Fixed only)" };
	ImGui::Combo("Filter", (int*)&changed_prefs.sound_filter, filter_items, IM_ARRAYSIZE(filter_items));
	const char* separation_items[] = { "100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%" };
	ImGui::Combo("Stereo separation", &changed_prefs.sound_stereo_separation, separation_items, IM_ARRAYSIZE(separation_items));
	const char* stereo_delay_items[] = { "-", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
	ImGui::Combo("Stereo delay", &changed_prefs.sound_mixed_stereo_delay, stereo_delay_items, IM_ARRAYSIZE(stereo_delay_items));
	const char* swap_channels_items[] = { "-", "Paula only", "AHI only", "Both" };
	ImGui::Combo("Swap channels", (int*)&changed_prefs.sound_stereo_swap_paula, swap_channels_items, IM_ARRAYSIZE(swap_channels_items));
}

static void render_panel_input()
{
	ImGui::Text("Port 0:");
	ImGui::Combo("##Port0", &changed_prefs.jports[0].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port0Mode", &changed_prefs.jports[0].mode, "Default\0Wheel Mouse\0Mouse\0Joystick\0Gamepad\0Analog Joystick\0CDTV remote mouse\0CD32 pad\0");
	ImGui::Combo("##Port0Autofire", &changed_prefs.jports[0].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");
	ImGui::Combo("##Port0MouseMode", &changed_prefs.jports[0].mousemap, "None\0LStick\0");
	ImGui::Button("Remap");

	ImGui::Text("Port 1:");
	ImGui::Combo("##Port1", &changed_prefs.jports[1].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port1Mode", &changed_prefs.jports[1].mode, "Default\0Wheel Mouse\0Mouse\0Joystick\0Gamepad\0Analog Joystick\0CDTV remote mouse\0CD32 pad\0");
	ImGui::Combo("##Port1Autofire", &changed_prefs.jports[1].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");
	ImGui::Combo("##Port1MouseMode", &changed_prefs.jports[1].mousemap, "None\0LStick\0");
	ImGui::Button("Remap");

	ImGui::Button("Swap ports");
	ImGui::Checkbox("Mouse/Joystick autoswitching", &changed_prefs.input_autoswitch);

	ImGui::Text("Emulated Parallel Port joystick adapter");
	ImGui::Text("Port 2:");
	ImGui::Combo("##Port2", &changed_prefs.jports[2].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port2Autofire", &changed_prefs.jports[2].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");

	ImGui::Text("Port 3:");
	ImGui::Combo("##Port3", &changed_prefs.jports[3].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port3Autofire", &changed_prefs.jports[3].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");

	const char* autofire_rate_items[] = { "Off", "Slow", "Medium", "Fast" };
	ImGui::Combo("Autofire Rate", &changed_prefs.input_autofire_linecnt, autofire_rate_items, IM_ARRAYSIZE(autofire_rate_items));

	ImGui::SliderInt("Digital joy-mouse speed", &changed_prefs.input_joymouse_speed, 2, 20);
	ImGui::SliderInt("Analog joy-mouse speed", &changed_prefs.input_joymouse_multiplier, 5, 150);
	ImGui::SliderInt("Mouse speed", &changed_prefs.input_mouse_speed, 5, 150);

	ImGui::Checkbox("Virtual mouse driver", (bool*)&changed_prefs.input_tablet);
	ImGui::Checkbox("Magic Mouse untrap", (bool*)&changed_prefs.input_mouse_untrap);

	ImGui::RadioButton("Both", &changed_prefs.input_magic_mouse_cursor, 0);
	ImGui::RadioButton("Native only", &changed_prefs.input_magic_mouse_cursor, 1);
	ImGui::RadioButton("Host only", &changed_prefs.input_magic_mouse_cursor, 2);

	ImGui::Checkbox("Swap Backslash/F11", (bool*)&key_swap_hack);
	ImGui::Checkbox("Page Up = End", (bool*)&key_swap_end_pgup);
}

static void render_panel_io()
{
	ImGui::Text("Parallel Port");
	const char* sampler_items[] = { "none" };
	ImGui::Combo("Sampler", &changed_prefs.samplersoundcard, sampler_items, IM_ARRAYSIZE(sampler_items));
	ImGui::Checkbox("Stereo sampler", &changed_prefs.sampler_stereo);

	ImGui::Separator();
	ImGui::Text("Serial Port");
	const char* serial_port_items[] = { "none" };
	ImGui::Combo("##SerialPort", (int*)(void*)changed_prefs.sername, serial_port_items, IM_ARRAYSIZE(serial_port_items));
	ImGui::Checkbox("Shared", &changed_prefs.serial_demand);
	ImGui::Checkbox("Direct", &changed_prefs.serial_direct);
	ImGui::Checkbox("Host RTS/CTS", &changed_prefs.serial_hwctsrts);
	ImGui::Checkbox("uaeserial.device", &changed_prefs.uaeserial);
	ImGui::Checkbox("Serial status (RTS/CTS/DTR/DTE/CD)", &changed_prefs.serial_rtsctsdtrdtecd);
	ImGui::Checkbox("Serial status: Ring Indicator", &changed_prefs.serial_ri);

	ImGui::Separator();
	ImGui::Text("MIDI");
	const char* midi_out_items[] = { "none" };
	ImGui::Combo("Out", (int*)(void*)changed_prefs.midioutdev, midi_out_items, IM_ARRAYSIZE(midi_out_items));
	const char* midi_in_items[] = { "none" };
	ImGui::Combo("In", (int*)(void*)changed_prefs.midiindev, midi_in_items, IM_ARRAYSIZE(midi_in_items));
	ImGui::Checkbox("Route MIDI In to MIDI Out", &changed_prefs.midirouter);

	ImGui::Separator();
	ImGui::Text("Protection Dongle");
	const char* dongle_items[] = { "none", "RoboCop 3", "Leader Board", "B.A.T. II", "Italy '90 Soccer", "Dames Grand-Maitre", "Rugby Coach", "Cricket Captain", "Leviathan", "Music Master", "Logistics/SuperBase", "Scala MM (Red)", "Scala MM (Green)", "Striker Manager", "Multi-player Soccer Manager", "Football Director 2" };
	ImGui::Combo("##ProtectionDongle", &changed_prefs.dongle, dongle_items, IM_ARRAYSIZE(dongle_items));
}

static void render_panel_custom()
{
    // Ensure SelectedPort stays within valid range to avoid OOB on jports
    if (SelectedPort < 0) SelectedPort = 0;
    if (SelectedPort > 3) SelectedPort = 3;

    // Persistent temporary buffers for safe text input (avoid using string literals)
    static char set_hotkey_buf[64] = {0};
    static char input_device_buf[256] = {0};

    ImGui::RadioButton("Port 0: Mouse", &SelectedPort, 0);
    ImGui::RadioButton("Port 1: Joystick", &SelectedPort, 1);
    ImGui::RadioButton("Port 2: Parallel 1", &SelectedPort, 2);
    ImGui::RadioButton("Port 3: Parallel 2", &SelectedPort, 3);

    ImGui::RadioButton("None", &SelectedFunction, 0);
    ImGui::RadioButton("HotKey", &SelectedFunction, 1);

    // Use writable buffers instead of string literals
    ImGui::InputText("##SetHotkey", set_hotkey_buf, sizeof(set_hotkey_buf));
    ImGui::SameLine();
    ImGui::Button("...");
    ImGui::SameLine();
    ImGui::Button("X");

    ImGui::InputText("Input Device", input_device_buf, sizeof(input_device_buf));

    const char* items[] = { "None" };

    // Buttons mapping list
    int bindex = 0;
    for (const auto& label : label_button_list) {
        ImGui::PushID(bindex);
        ImGui::Text("%s", label.c_str());
        ImGui::SameLine();
        ImGui::Combo("##btnmap", &changed_prefs.jports[SelectedPort].autofire, items, 1);
        ImGui::PopID();
        ++bindex;
    }

    // Axes mapping list
    int aindex = 0;
    for (const auto& label : label_axis_list) {
        ImGui::PushID(1000 + aindex);
        ImGui::Text("%s", label.c_str());
        ImGui::SameLine();
        ImGui::Combo("##axmap", &changed_prefs.jports[SelectedPort].autofire, items, 1);
        ImGui::PopID();
        ++aindex;
    }

    if (ImGui::Button("Save as default mapping")) {
        // TODO: Save mapping defaults
    }
}

static void render_panel_diskswapper()
{
    ImGui::BeginChild("DiskSwapperList", ImVec2(0, -50), true);
    ImGui::Columns(3, "DiskSwapperColumns");
    ImGui::Separator();
    ImGui::Text("#"); ImGui::NextColumn();
    ImGui::Text("Disk Image"); ImGui::NextColumn();
    ImGui::Text("Drive"); ImGui::NextColumn();
    ImGui::Separator();
    for (int i = 0; i < MAX_SPARE_DRIVES; ++i)
    {
        ImGui::PushID(i);
        ImGui::Text("%d", i + 1); ImGui::NextColumn();
        // Unique IDs per row to avoid collisions and potential crashes
        bool edited = ImGui::InputText("##DiskPath", changed_prefs.dfxlist[i], MAX_DPATH);
        ImGui::SameLine();
        if (ImGui::Button("...##Browse"))
        {
            // Select file (TODO)
        }
        ImGui::SameLine();
        if (ImGui::Button("X##Clear"))
        {
            changed_prefs.dfxlist[i][0] = 0;
        }
        ImGui::NextColumn();
        const int drive = disk_in_drive(i);
        char drive_btn[16];
        if (drive < 0)
            snprintf(drive_btn, sizeof(drive_btn), "-##%d", i);
        else
            snprintf(drive_btn, sizeof(drive_btn), "%d##%d", drive, i);
        if (ImGui::Button(drive_btn))
        {
            disk_swap(i, 1);
        }
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    if (ImGui::Button("Remove All"))
    {
        for (int row = 0; row < MAX_SPARE_DRIVES; ++row)
            changed_prefs.dfxlist[row][0] = 0;
    }
}

static void render_panel_misc()
{
	ImGui::Checkbox("Status Line native", (bool*)&changed_prefs.leds_on_screen);
	ImGui::Checkbox("Status Line RTG", (bool*)&changed_prefs.leds_on_screen);
	ImGui::Checkbox("Show GUI on startup", &changed_prefs.start_gui);
	ImGui::Checkbox("Untrap = middle button", (bool*)&changed_prefs.input_mouse_untrap);
	ImGui::Checkbox("Alt-Tab releases control", &changed_prefs.alt_tab_release);
	ImGui::Checkbox("Use RetroArch Quit Button", &changed_prefs.use_retroarch_quit);
	ImGui::Checkbox("Use RetroArch Menu Button", &changed_prefs.use_retroarch_menu);
	ImGui::Checkbox("Use RetroArch Reset Button", &changed_prefs.use_retroarch_reset);
	ImGui::Checkbox("Master floppy write protection", &changed_prefs.floppy_read_only);
	ImGui::Checkbox("Master harddrive write protection", &changed_prefs.harddrive_read_only);
	ImGui::Checkbox("Clipboard sharing", &changed_prefs.clipboard_sharing);
	ImGui::Checkbox("RCtrl = RAmiga", &changed_prefs.right_control_is_right_win_key);
	ImGui::Checkbox("Always on top", &changed_prefs.main_alwaysontop);
	ImGui::Checkbox("GUI Always on top", &changed_prefs.gui_alwaysontop);
	ImGui::Checkbox("Synchronize clock", &changed_prefs.tod_hack);
	ImGui::Checkbox("One second reboot pause", &changed_prefs.reset_delay);
	ImGui::Checkbox("Faster RTG", &changed_prefs.picasso96_nocustom);
	ImGui::Checkbox("Allow native code", &changed_prefs.native_code);
	ImGui::Checkbox("Log illegal memory accesses", &changed_prefs.illegal_mem);
	ImGui::Checkbox("Minimize when focus is lost", &changed_prefs.minimize_inactive);
	ImGui::Checkbox("Capture mouse when window is activated", &changed_prefs.capture_always);
	ImGui::Checkbox("Hide all UAE autoconfig boards", &changed_prefs.uae_hide_autoconfig);
	ImGui::Checkbox("A600/A1200/A4000 IDE scsi.device disable", &changed_prefs.scsidevicedisable);
	ImGui::Checkbox("Warp mode reset", &changed_prefs.turbo_boot);

	const char* led_items[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD" };
	ImGui::Combo("NumLock", &changed_prefs.kbd_led_num, led_items, IM_ARRAYSIZE(led_items));
	ImGui::Combo("ScrollLock", &changed_prefs.kbd_led_scr, led_items, IM_ARRAYSIZE(led_items));
	ImGui::Combo("CapsLock", &changed_prefs.kbd_led_cap, led_items, IM_ARRAYSIZE(led_items));

	ImGui::InputText("Open GUI", changed_prefs.open_gui, 256);
	ImGui::InputText("Quit Key", changed_prefs.quit_amiberry, 256);
	ImGui::InputText("Action Replay", changed_prefs.action_replay, 256);
	ImGui::InputText("FullScreen", changed_prefs.fullscreen_toggle, 256);
	ImGui::InputText("Minimize", changed_prefs.minimize, 256);
	ImGui::InputText("Right Amiga", changed_prefs.right_amiga, 256);
}

static void render_panel_prio()
{
	ImGui::Text("When Active");
	const char* prio_items[] = { "Low", "Normal", "High" };
	ImGui::Combo("Run at priority##Active", &changed_prefs.active_capture_priority, prio_items, IM_ARRAYSIZE(prio_items));
	ImGui::Checkbox("Pause emulation##Active", &changed_prefs.active_nocapture_pause);
	ImGui::Checkbox("Disable sound##Active", &changed_prefs.active_nocapture_nosound);

	ImGui::Separator();
	ImGui::Text("When Inactive");
	ImGui::Combo("Run at priority##Inactive", &changed_prefs.inactive_priority, prio_items, IM_ARRAYSIZE(prio_items));
	ImGui::Checkbox("Pause emulation##Inactive", &changed_prefs.inactive_pause);
	ImGui::Checkbox("Disable sound##Inactive", &changed_prefs.inactive_nosound);
	ImGui::Checkbox("Disable input##Inactive", (bool*)&changed_prefs.inactive_input);

	ImGui::Separator();
	ImGui::Text("When Minimized");
	ImGui::Combo("Run at priority##Minimized", &changed_prefs.minimized_priority, prio_items, IM_ARRAYSIZE(prio_items));
	ImGui::Checkbox("Pause emulation##Minimized", &changed_prefs.minimized_pause);
	ImGui::Checkbox("Disable sound##Minimized", &changed_prefs.minimized_nosound);
	ImGui::Checkbox("Disable input##Minimized", (bool*)&changed_prefs.minimized_input);
}

static void render_panel_savestates()
{
	for (int i = 0; i < 15; ++i)
	{
		ImGui::RadioButton(std::to_string(i).c_str(), &current_state_num, i);
	}
	ImGui::Separator();
	ImGui::Text("Filename: %s", savestate_fname);
	ImGui::Text("Timestamp: %s", "");
	if (ImGui::Button("Load from Slot"))
	{
		// Load state from selected slot
	}
	ImGui::SameLine();
	if (ImGui::Button("Save to Slot"))
	{
		// Save current state to selected slot
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete Slot"))
	{
		// Delete state from selected slot
	}
	if (ImGui::Button("Load state..."))
	{
		// Load state from file
	}
	ImGui::SameLine();
	if (ImGui::Button("Save state..."))
	{
		// Save current state to file
	}
}

static void render_panel_virtual_keyboard()
{
    ImGui::Checkbox("Virtual Keyboard enabled", &changed_prefs.vkbd_enabled);

    if (!changed_prefs.vkbd_enabled)
        ImGui::BeginDisabled();

    ImGui::Checkbox("High-Resolution", &changed_prefs.vkbd_hires);
    ImGui::Checkbox("Quit button on keyboard", &changed_prefs.vkbd_exit);

    ImGui::Text("Transparency:");
    ImGui::SameLine();
    ImGui::SliderInt("##VkTransparency", &changed_prefs.vkbd_transparency, 0, 100, "%d%%");

    const char* languages[] = { "US", "FR", "UK", "DE" };
    int current_lang = 0;
    if (strcmp(changed_prefs.vkbd_language, "FR") == 0) current_lang = 1;
    else if (strcmp(changed_prefs.vkbd_language, "UK") == 0) current_lang = 2;
    else if (strcmp(changed_prefs.vkbd_language, "DE") == 0) current_lang = 3;

    if (ImGui::Combo("Keyboard Layout", &current_lang, languages, IM_ARRAYSIZE(languages))) {
        strcpy(changed_prefs.vkbd_language, languages[current_lang]);
    }

    const char* styles[] = { "Original", "Warm", "Cool", "Dark" };
    int current_style = 0;
    if (strcmp(changed_prefs.vkbd_style, "Warm") == 0) current_style = 1;
    else if (strcmp(changed_prefs.vkbd_style, "Cool") == 0) current_style = 2;
    else if (strcmp(changed_prefs.vkbd_style, "Dark") == 0) current_style = 3;

    if (ImGui::Combo("Style", &current_style, styles, IM_ARRAYSIZE(styles))) {
        strcpy(changed_prefs.vkbd_style, styles[current_style]);
    }

    ImGui::Checkbox("Use RetroArch Vkbd button", &changed_prefs.use_retroarch_vkbd);

    if (changed_prefs.use_retroarch_vkbd)
        ImGui::BeginDisabled();

    ImGui::Text("Toggle button:");
    ImGui::SameLine();
    ImGui::InputText("##VkSetHotkey", changed_prefs.vkbd_toggle, sizeof(changed_prefs.vkbd_toggle), ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("...##VkSetHotkey"))
    {
        // TODO: Implement input selection dialog
    }
    ImGui::SameLine();
    if (ImGui::Button("X##VkSetHotkeyClear"))
    {
        changed_prefs.vkbd_toggle[0] = 0;
        for (int port = 0; port < 2; port++)
        {
            const auto host_joy_id = changed_prefs.jports[port].id - JSEM_JOYS;
            if (host_joy_id >= 0 && host_joy_id < MAX_INPUT_DEVICES)
            {
                didata* did = &di_joystick[host_joy_id];
                did->mapping.vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;
            }
        }
    }

    if (changed_prefs.use_retroarch_vkbd)
        ImGui::EndDisabled();

    if (!changed_prefs.vkbd_enabled)
        ImGui::EndDisabled();
}

static void render_panel_whdload()
{
    ImGui::Text("WHDLoad auto-config:");
    ImGui::SameLine(ImGui::GetWindowWidth() - 180);
    if (ImGui::Button("Eject")) {
        whdload_prefs.whdload_filename = "";
        clear_whdload_prefs();
    }

    ImGui::SameLine();
    if (ImGui::Button("Select file")) {
        // TODO: Implement file selector.
    }

    // WHDLoad file dropdown
    std::vector<const char*> whdload_display_items;
    std::vector<std::string> whdload_filenames;
    for (const auto& full_path : lstMRUWhdloadList) {
        whdload_filenames.push_back(full_path.substr(full_path.find_last_of("/\\") + 1));
    }
    for (const auto& filename : whdload_filenames) {
        whdload_display_items.push_back(filename.c_str());
    }

    int current_whd = -1;
    if (!whdload_prefs.whdload_filename.empty()) {
        for (size_t i = 0; i < lstMRUWhdloadList.size(); ++i) {
            if (lstMRUWhdloadList[i] == whdload_prefs.whdload_filename) {
                current_whd = static_cast<int>(i);
                break;
            }
        }
    }
    ImGui::PushItemWidth(-1);
    if (ImGui::Combo("##WHDLoadFile", &current_whd, whdload_display_items.data(), static_cast<int>(whdload_display_items.size()))) {
        if (current_whd >= 0) {
            const auto& selected_path = lstMRUWhdloadList[static_cast<size_t>(current_whd)];
            if (selected_path != whdload_prefs.whdload_filename) {
                whdload_prefs.whdload_filename = selected_path;
                add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
                whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
                if (!last_loaded_config[0])
                    set_last_active_config(whdload_prefs.whdload_filename.c_str());
            }
        }
    }
    ImGui::PopItemWidth();

    ImGui::Separator();

    // WHDLoad game options
    ImGui::InputText("Game Name", whdload_prefs.game_name.data(), whdload_prefs.game_name.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputText("UUID", whdload_prefs.variant_uuid.data(), whdload_prefs.variant_uuid.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputText("Slave Default", whdload_prefs.slave_default.data(), whdload_prefs.slave_default.size(), ImGuiInputTextFlags_ReadOnly);

    ImGui::Checkbox("Slave Libraries", &whdload_prefs.slave_libraries);

    // Slaves dropdown
    std::vector<const char*> slave_items;
    for (const auto& slave : whdload_prefs.slaves) {
        slave_items.push_back(slave.filename.c_str());
    }
    int current_slave = -1;
    if (!whdload_prefs.selected_slave.filename.empty()) {
        for (size_t i = 0; i < whdload_prefs.slaves.size(); ++i) {
            if (whdload_prefs.slaves[i].filename == whdload_prefs.selected_slave.filename) {
                current_slave = static_cast<int>(i);
                break;
            }
        }
    }
    if (ImGui::Combo("Slaves", &current_slave, slave_items.data(), static_cast<int>(slave_items.size()))) {
        if (current_slave >= 0) {
            whdload_prefs.selected_slave = whdload_prefs.slaves[static_cast<size_t>(current_slave)];
            create_startup_sequence();
        }
    }

    char data_path_buf[1024];
    strncpy(data_path_buf, whdload_prefs.selected_slave.data_path.c_str(), sizeof(data_path_buf));
    data_path_buf[sizeof(data_path_buf) - 1] = 0;
    ImGui::InputText("Slave Data path", data_path_buf, sizeof(data_path_buf), ImGuiInputTextFlags_ReadOnly);

    if (ImGui::InputText("Custom", whdload_prefs.custom.data(), whdload_prefs.custom.size())) {
        create_startup_sequence();
    }

    if (whdload_prefs.whdload_filename.empty()) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Custom Fields"))
    {
        // TODO: Implement custom fields dialog
    }
    if (whdload_prefs.whdload_filename.empty()) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();
    ImGui::Text("Global options");

    if (ImGui::Checkbox("Button Wait", &whdload_prefs.button_wait)) {
        create_startup_sequence();
    }
    if (ImGui::Checkbox("Show Splash", &whdload_prefs.show_splash)) {
        create_startup_sequence();
    }

    if (ImGui::InputInt("Config Delay", &whdload_prefs.config_delay)) {
        create_startup_sequence();
    }

    if (ImGui::Checkbox("Write Cache", &whdload_prefs.write_cache)) {
        create_startup_sequence();
    }
    if (ImGui::Checkbox("Quit on Exit", &whdload_prefs.quit_on_exit)) {
        create_startup_sequence();
    }
}

static void render_panel_themes()
{

}

void run_gui()
{
	gui_running = true;
	AmigaMonitor* mon = &AMonitors[0];

	amiberry_gui_init();

	// Main loop
	while (gui_running)
	{
		while (SDL_PollEvent(&gui_event))
		{
			// Make sure ImGui sees all events
			ImGui_ImplSDL2_ProcessEvent(&gui_event);

			if (gui_event.type == SDL_QUIT)
				gui_running = false;
			if (gui_event.type == SDL_WINDOWEVENT && gui_event.window.event == SDL_WINDOWEVENT_CLOSE && gui_event.window.windowID == SDL_GetWindowID(mon->gui_window))
				gui_running = false;
		}

		// Start the Dear ImGui frame
		ImGui_ImplSDL2_NewFrame();
		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui::NewFrame();

		ImGuiStyle& style = ImGui::GetStyle();
		const auto sidebar_width = ImGui::GetIO().DisplaySize.x / 5.0f;
		const auto button_bar_height = ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("Amiberry", &gui_running, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

		// Sidebar
		ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, 0), true);
		for (int i = 0; categories[i].category != nullptr; ++i) {
			if (ImGui::Selectable(categories[i].category, last_active_panel == i)) {
				last_active_panel = i;
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginGroup();
		// Content
		ImGui::BeginChild("Content", ImVec2(0, -button_bar_height));

		if (last_active_panel == PANEL_ABOUT)
		{
			render_panel_about();
		}
		else if (last_active_panel == PANEL_PATHS)
		{
			render_panel_paths();
		}
		else if (last_active_panel == PANEL_QUICKSTART)
		{
			render_panel_quickstart();
		}
		else if (last_active_panel == PANEL_CONFIGURATIONS)
		{
			render_panel_configurations();
		}
		else if (last_active_panel == PANEL_CPU)
		{
			render_panel_cpu();
		}
		else if (last_active_panel == PANEL_CHIPSET)
		{
			render_panel_chipset();
		}
		else if (last_active_panel == PANEL_ROM)
		{
			render_panel_rom();
		}
		else if (last_active_panel == PANEL_RAM)
		{
			render_panel_ram();
		}
		else if (last_active_panel == PANEL_FLOPPY)
		{
			render_panel_floppy();
		}
		else if (last_active_panel == PANEL_HD)
		{
			render_panel_hd();
		}
		else if (last_active_panel == PANEL_EXPANSIONS)
		{
			render_panel_expansions();
		}
		else if (last_active_panel == PANEL_RTG)
		{
			render_panel_rtg();
		}
		else if (last_active_panel == PANEL_HWINFO)
		{
			render_panel_hwinfo();
		}
		else if (last_active_panel == PANEL_DISPLAY)
		{
			render_panel_display();
		}
		else if (last_active_panel == PANEL_SOUND)
		{
			render_panel_sound();
		}
		else if (last_active_panel == PANEL_INPUT)
		{
			render_panel_input();
		}
		else if (last_active_panel == PANEL_IO)
		{
			render_panel_io();
		}
		else if (last_active_panel == PANEL_CUSTOM)
		{
			render_panel_custom();
		}
		else if (last_active_panel == PANEL_DISK_SWAPPER)
		{
			render_panel_diskswapper();
		}
		else if (last_active_panel == PANEL_MISC)
		{
			render_panel_misc();
		}
		else if (last_active_panel == PANEL_PRIO)
		{
			render_panel_prio();
		}
		else if (last_active_panel == PANEL_SAVESTATES)
		{
			render_panel_savestates();
		}
		else if (last_active_panel == PANEL_VIRTUAL_KEYBOARD)
		{
			render_panel_virtual_keyboard();
		}
		else if (last_active_panel == PANEL_WHDLOAD)
		{
			render_panel_whdload();
		}
		else if (last_active_panel == PANEL_THEMES)
		{
			render_panel_themes();
		}
		ImGui::EndChild();

		// Button bar
		ImGui::BeginChild("ButtonBar", ImVec2(0, 0), true);
		if (ImGui::Button("Quit"))
		{
			uae_quit();
			gui_running = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Restart"))
		{
			uae_reset(1, 1);
			gui_running = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Help"))
		{
			std::string help_str;
			switch(last_active_panel)
			{
				case PANEL_ABOUT:
				{
					help_str = "This panel contains information about the version of Amiberry, when it was changed,\nwhich version of SDL2 it was compiled against and currently using.\n \nFurthermore, you can also find the GPLv3 license notice here, and if you scroll down\nall the credits to the people behind the development of this emulator as well.\n \nAt the bottom of the screen, there are a few buttons available regardless of which\npanel you have selected. Those are: \n \n\"Shutdown\": allows you to shutdown the whole system Amiberry is running on. This\n option can be disabled if you wish, by setting \'disable_shutdown_button=yes\' in\n in your amiberry.conf file.\n \n\"Quit\": This quits Amiberry, as you'd expect.\n \n\"Restart\": This button will stop emulation (if running), reload Amiberry and reset\n the currently loaded configuration. This has a similar effect as if you Quit and start\n Amiberry again.\n It can be useful if you want to change a setting that cannot be changed on-the-fly,\n and you don't want to quit and start the Amiberry again to do that.\n \n\"Help\": This will display some on-screen help/documentation, relating to the Panel\n you are currently in.\n \n\"Reset\": This button will trigger a hard reset of the emulation, which will reboot\n with the current settings. \n \n\"Start\": This button starts the emulation, using the current settings you have set.\n ";
				}
				break;
				case PANEL_PATHS:
				{
					help_str = "Here you can configure the various paths for Amiberry resources. In normal usage,\nthe default paths should work fine, however if you wish to change any path, you\ncan use the \"...\" button, to select the folder/path of your choosing. Details\nfor each path resource appear below.\n \nYou can enable/disable logging and specify the location of the logfile by using\nthe relevant options. A logfile is useful when trying to troubleshoot something,\nbut otherwise this option should be off, as it will incur some extra overhead.\nYou can also redirect the log output to console, by enabling that logging option.\nYou can alternatively enable log output to console if you pass the --log option\nto Amiberry on startup.\n \nThe \"Rescan Paths\" button will rescan the paths specified above and refresh the\nlocal cache. This should be done if you added kickstart ROMs for example, in order\nfor Amiberry to pick them up. This button will regenerate the amiberry.conf file\nif it's missing, and will be populated with the default values.\n \nThe \"Update WHDBooter files\" button will attempt to download the latest XML used for\nthe WHDLoad-booter functionality of Amiberry, along with all related files in the\n\"whdboot\" directory. It requires an internet connection and write permissions in the\ndestination directory. The downloaded XML file will be stored in the default location\n(whdboot/game-data/whdload_db.xml). Once the file is successfully downloaded, you\nwill also get a dialog box informing you about the details. A backup copy of the\nexisting whdload_db.xml is made (whdboot/game-data/whdload_db.bak), to preserve any\ncustom edits that may have been made. The rest of the files will be updated with the\nlatest version from the repository.\n \nThe \"Update Controllers DB\" button will attempt to download the latest version of\nthe bundled gamecontrollerdb.txt file, to be stored in the Controllers files path.\nThe file contains the \"official\" mappings for recognized controllers by SDL2 itself.\nPlease note that this is separate from the user-configurable gamecontrollerdb_user.txt\nfile, which is contained in the Controllers path. That file is never overwritten, and\nit will be loaded after the official one, so any entries contained there will take a \nhigher priority. Once the file is successfully downloaded, you will also get a dialog\nbox informing you about the details. A backup copy of the existing gamecontrollerdb.txt\n(conf/gamecontrollerdb.bak) is created, to preserve any custom edits it may contain.\n \nThe paths for Amiberry resources include;\n \n- System ROMs: The Amiga Kickstart files are by default located under 'roms'.\n  After changing the location of the Kickstart ROMs, or adding any additional ROMs, \n  click on the \"Rescan\" button to refresh the list of the available ROMs. Please\n  note that MT-32 ROM files may also reside here, or in a \"mt32-roms\" directory\n  at this location, if you wish to use the MT-32 MIDI emulation feature in Amiberry.\n \n- Configuration files: These are located under \"conf\" by default. This is where your\n  configurations will be stored, but also where Amiberry keeps the special amiberry.conf\n  file, which contains the default settings the emulator uses when it starts up. This\n  is also where the bundled gamecontrollersdb.txt file is located, which contains the\n  community-maintained mappings for various controllers that SDL2 recognizes.\n \n- NVRAM files: the location where CDTV/CD2 modes will store their NVRAM files.\n \n- Plugins path: the location where external plugins (such as the CAPSimg or the\n  floppybridge plugin) are stored.\n \n- Screenshots: any screenshots you take will be saved by default in this location.\n \n- Save state files: if you use them, they will be saved in the specified location.\n \n- Controller files: any custom (user-generated) controller mapping files will be saved\n  in this location. This location is also used in RetroArch environments (ie; such as\n  RetroPie) to point to the directory containing the controller mappings.\n \n- RetroArch configuration file (retroarch.cfg): only useful if you are using RetroArch\n  (ie; in RetroPie). Amiberry can pick-up the configuration file from the path specified\n  here, and load it automatically, applying any mappings it contains. You can ignore this\n  path if you're not using RetroArch.\n \n- WHDboot files: This directory contains the files required by the whd-booter process\n  to launch WHDLoad game archives. In normal usage you should not need to change this.\n \n- Below that are 4 additional paths, that can be used to better organize your various\n  Amiga files, and streamline GUI operations when it comes to selecting the different\n  types of Amiga media. The file selector buttons in Amiberry associated with each of\n  the media types, will open these path locations. The defaults are shown, but these\n  can be changed to better suit your requirements.\n \nThese settings are saved automatically when you click Rescan, or exit the emulator.\n ";
				}
				break;
				case PANEL_QUICKSTART:
				{
					help_str = "Simplified start of emulation by just selecting the Amiga model and the disk/CD\nyou want to use.\n \nAfter selecting the Amiga model, you can choose from a small list of standard\nconfigurations for this model to start with. Depending on the model selected,\nthe floppy or CD drive options will be enabled for you, which you can use to\ninsert any floppy disk or CD images, accordingly.\nIf you need more advanced control over the hardware you want to emulate, you\ncan always use the rest of the GUI for that.\n \nYou can reset the current configuration to your selected model, by clicking the\nSet Configuration button.\n \nWhen you activate \"Start in Quickstart mode\", the next time you run the emulator,\nit  will start with the QuickStart panel. Otherwise you start in the configurations panel.\n \nYou can optionally select a WHDLoad LHA title, and have Amiberry auto-configure\nall settings, based on the WHDLoad XML (assuming the title is found there).\nThen you can just click on Start to begin!";
				}
				break;
				case PANEL_CONFIGURATIONS:
				{
					help_str = "In this panel, you can see a list of all your previously saved configurations. The\nConfiguration file (.uae) contains all the emulator settings available in it. Loading\nsuch a file, will apply those settings to Amiberry immediately. Accordingly, you can\nSave your current settings in a file here, for future use.\n \nPlease note the \"default\" config name is special for Amiberry, since if it exists,\nit will be loaded automatically on startup. This will override the emulator options\nAmiberry sets internally at startup, and this may impact on compatibility when using\nthe Quickstart panel.\n \nTo load a configuration, select the entry in the list, and then click on the \"Load\"\nbutton. Note that if you double-click on an entry in the list, the emulation starts\nimmediately using that configuration.\n \nTo create/save a new configuration, set all emulator options as required, then enter\na new \"Name\", optionally provide a short description, and then click on the \"Save\"\nbutton. When trying to Save a configuration, if the supplied filename already exists,\nit will be automatically renamed to \"configuration.backup\", to keep as a backup.\n \nPlease note a special case exists when creating/saving a configuration file for use \nwith floppy disk images and whdload archives. The auto-config logic in Amiberry will\nscan for a configuration file of the same \"Name\" as the disk image or .lha archive\nbeing loaded. After you load a floppy disk image or whdload archive, and Start the \nemulation, you can use the \"F12\" key to show the GUI, and in this panel the \"Name\"\nfield for the configuration will be filled correctly. Do not change this, as it will\nstop auto-config from working. You may change the description if you desire.\n \nTo delete the currently selected configuration file from the disk (and the list),\nclick on the \"Delete\" button.";
				}
				break;
				case PANEL_CPU:
				{
					help_str = "Select the required Amiga CPU (68000 - 68060). If you select 68020, you can choose\nbetween 24-bit (68EC020) or 32-bit (68020) addressing.\n \nThe option \"More compatible\" will emulate prefetch (68000) or prefetch and\ninstruction cache. It's more compatible but slower, but not required\nfor most games and demos.\n \nJIT/JIT FPU enables the Just-in-time compiler. This may break compatibility in some games.\nNote: Not available in all platforms currently\n \nThe available FPU models depend on the selected CPU type. The option \"More compatible\"\nactivates a more accurate rounding method and compare of two floats.\n \nThe CPU Speed slider allows you to set the CPU speed. The fastest possible setting\nwill run the CPU as fast as possible. The A500/A1200 setting will run the CPU at\nthe speed of an A500 or A1200. The slider allows you to set the CPU speed in\npercentages of the maximum speed.\n \nThe CPU Idle slider allows you to set how much the CPU should sleep when idle.\nThis is useful to keep the system temperature down.\n \nThe MMU option allows you to enable the Memory Management Unit. This is only available\nfor the 68030, 68040 and 68060 CPUs.\n \nThe FPU option allows you to enable the FPU. This is only available for the 68020, 68030,\n68040 and 68060 CPUs.\n \nThe PPC emulation option allows you to enable the PowerPC emulation. This is only available\nfor the 68040 and 68060 CPUs and requires an extra plugin (qemu-uae) to be available.";
				}
				break;
				case PANEL_CHIPSET:
				{
					help_str = "If you want to emulate an Amiga 1200, select AGA. For most Amiga 500 games,\nselect \"Full ECS\" instead. Some older Amiga games require \"OCS\" or \"ECS Agnus\".\nYou have to play with these options if a game won't work as expected. By selecting\nan entry in \"Extra\", all internal Chipset settings will change to the required values\nfor the specified Amiga model. For some games, you have to switch to \"NTSC\"\n(60 Hz instead of 50 Hz) for correct timing.\n \nThe \"Multithreaded drawing\" option, will enable some Amiberry optimizations\nthat will help the performance when drawing lines on native screen modes.\nIn some cases, this might cause screen tearing artifacts, so you can choose to\ndisable this option when needed. Note that it cannot be changed once emulation has\nstarted.\n \nIf you see graphic issues in a game, try the \"Immediate\" or \"Wait for blitter\"\nBlitter options.\n \nFor \"Collision Level\", select \"Sprites and Sprites vs. Playfield\" which is fine\nfor nearly all games.";
				}
				break;
				case PANEL_ROM:
				{
					help_str = "Select the required Kickstart ROM for the Amiga you want to emulate in \"Main ROM File\".\n \nIn \"Extended ROM File\", you can only select the required ROM for CD32 emulation.\n \nYou can use the ShapeShifter support checkbox to patch the system ROM for ShapeShifter\ncompatibility. You do not need to run PrepareEmul on startup with this enabled.\n \nIn \"Cartridge ROM File\", you can select the CD32 FMV module to activate video\nplayback in CD32. There are also some Action Replay and Freezer cards and the built-in\nHRTMon available.\n \nThe Advanced UAE Expansion/Boot ROM option allows you to set the following:\nRom Disabled: All UAE expansions are disabled. Only needed if you want to force it.\nOriginal UAE: Autoconfig board + F0 ROM.\nNew UAE: 64k + F0 ROM - not very useful (per Toni Wilen).";
				}
				break;
			}

			if (!help_str.empty())
				ShowMessageBox("Help", help_str.c_str());
		}
		ImGui::SameLine();
		if (ImGui::Button("Start"))
			gui_running = false;
		ImGui::EndChild();
		ImGui::EndGroup();

		if (show_message_box)
		{
			ImGui::OpenPopup(message_box_title);
			if (ImGui::BeginPopupModal(message_box_title, &show_message_box, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextWrapped("%s", message_box_message);
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					show_message_box = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		
		ImGui::End();

		// Rendering
		ImGui::Render();
		SDL_SetRenderDrawColor(mon->gui_renderer, (Uint8)(0.45f * 255), (Uint8)(0.55f * 255),
							   (Uint8)(0.60f * 255), (Uint8)(1.00f * 255));
		SDL_RenderClear(mon->gui_renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), mon->gui_renderer);
		SDL_RenderPresent(mon->gui_renderer);
	}

	amiberry_gui_halt();

	// Reset counter for access violations
	init_max_signals();
}
#endif
