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
#include "amiberry_input.h"
#include "fsdb.h"
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
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// Setup Platform/Renderer backends
	ImGui_ImplSDLRenderer2_Init(AMonitors[0].gui_renderer);
	ImGui_ImplSDL2_InitForSDLRenderer(AMonitors[0].gui_window, AMonitors[0].gui_renderer);
#endif
#ifdef USE_GUISAN
	gui_texture = SDL_CreateTexture(mon->gui_renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING, gui_screen->w,
									gui_screen->h);
	check_error_sdl(gui_texture == nullptr, "Unable to create GUI texture:");
#endif

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_HEIGHT, GUI_WIDTH);

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
	selectorsScrollArea->setBackgroundColor(gui_base_color);
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

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("Amiberry", &gui_running, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

		// Sidebar
		ImGui::BeginChild("Sidebar", ImVec2(150, 0), true);
		for (int i = 0; categories[i].category != nullptr; ++i) {
			if (ImGui::Selectable(categories[i].category, last_active_panel == i)) {
				last_active_panel = i;
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Content
		ImGui::BeginChild("Content", ImVec2(0, -50));

		if (last_active_panel == PANEL_ABOUT)
		{
			ImGui::Text("Amiberry - The Amiga Emulator for ARM-based devices");
			ImGui::Text("Version: %s", get_version_string().c_str());
			ImGui::Separator();
			ImGui::Text("Ported from WinUAE, which was made by Toni Wilen and contributors.");
			ImGui::Text("WinUAE is a port of the original UAE, which was made by Bernd Schmidt.");
			ImGui::Text("Guisan GUI library by Olof Naessen.");
			ImGui::Text("ImGui GUI library by Omar Cornut.");
		}
		else if (last_active_panel == PANEL_PATHS)
		{
			// --- 5) Temporary stub to avoid TCHAR[8][4096] to char* mismatch and unknown symbols ---
			// Re-enable gradually. When editing arrays like refs.path_rom.path, index them: path[i]
			// not the whole 2D array.
			ImGui::Text("Paths panel (ImGui) is WIP.");
			ImGui::Separator();
			ImGui::TextWrapped("Tip: edit multi-path arrays by indexing, e.g. refs.path_rom.path[i], "
							   "not refs.path_rom.path. Use MAX_DPATH for element size.");
			if (ImGui::Button("Rescan Paths"))
				scan_roms(true);
			if (ImGui::Button("Update WHDBooter files"))
			{
				//download_whdbooter_files();
			}
			if (ImGui::Button("Update Controllers DB"))
			{
				//download_controllers_db();
			}
		}
		else if (last_active_panel == PANEL_CONFIGURATIONS)
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
				strncat(filename, ".uae", MAX_DPATH - 1);
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
		else if (last_active_panel == PANEL_QUICKSTART)
		{
			const char* models[] = { "Amiga 500", "Amiga 500+", "Amiga 600", "Amiga 1000", "Amiga 1200", "Amiga 3000", "Amiga 4000", "Amiga 4000T", "CD32", "CDTV", "American Laser Games / Picmatic", "Arcadia Multi Select system", "Macrosystem" };
			ImGui::Combo("Amiga model", &quickstart_model, models, IM_ARRAYSIZE(models));

			const char* configs[] = { "1.3 ROM, OCS, 512 KB Chip + 512 KB Slow RAM (most common)", "1.3 ROM, ECS Agnus, 512 KB Chip RAM + 512 KB Slow RAM", "1.3 ROM, ECS Agnus, 1 MB Chip RAM", "1.3 ROM, OCS Agnus, 512 KB Chip RAM", "1.2 ROM, OCS Agnus, 512 KB Chip RAM", "1.2 ROM, OCS Agnus, 512 KB Chip RAM + 512 KB Slow RAM" };
			ImGui::Combo("Config", &quickstart_conf, configs, IM_ARRAYSIZE(configs));

			ImGui::Checkbox("NTSC", &changed_prefs.ntscmode);

			for (int i = 0; i < 2; ++i)
			{
				char label[10];
				sprintf(label, "DF%d:", i);
				ImGui::Checkbox(label, (bool*)&changed_prefs.floppyslots[i].dfxtype);
				ImGui::SameLine();
				ImGui::Checkbox("Write-protected", &changed_prefs.floppy_read_only);
			}

			ImGui::Checkbox("CD drive", &changed_prefs.cdslots[0].inuse);

			if (ImGui::Button("Set configuration"))
				built_in_prefs(&changed_prefs, quickstart_model, quickstart_conf, 0, 0);
		}
		else if (last_active_panel == PANEL_CPU)
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
		else if (last_active_panel == PANEL_CHIPSET)
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
		else if (last_active_panel == PANEL_ROM)
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
		else if (last_active_panel == PANEL_RAM)
		{
			ImGui::SliderInt("Chip", (int*)&changed_prefs.chipmem.size, 0, 0x800000);
			ImGui::SliderInt("Slow", (int*)&changed_prefs.bogomem.size, 0, 0x180000);
			ImGui::SliderInt("Z2 Fast", (int*)&changed_prefs.fastmem[0].size, 0, 0x800000);
			ImGui::SliderInt("Z3 Fast", (int*)&changed_prefs.z3fastmem[0].size, 0, 0x40000000);
			ImGui::SliderInt("32-bit Chip RAM", (int*)&changed_prefs.z3chipmem.size, 0, 0x40000000);
			ImGui::SliderInt("Motherboard Fast RAM", (int*)&changed_prefs.mbresmem_low.size, 0, 0x8000000);
			ImGui::SliderInt("Processor slot Fast RAM", (int*)&changed_prefs.mbresmem_high.size, 0, 0x8000000);
			const char* z3_mapping_items[] = { "Automatic (*)", "UAE (0x10000000)", "Real (0x40000000)" };
			ImGui::Combo("Z3 Mapping Mode", &changed_prefs.z3_mapping_mode, z3_mapping_items, IM_ARRAYSIZE(z3_mapping_items));
		}
		else if (last_active_panel == PANEL_FLOPPY)
		{
			for (int i = 0; i < 4; ++i)
			{
				char label[10];
				sprintf(label, "DF%d:", i);
				ImGui::Checkbox(label, (bool*)&changed_prefs.floppyslots[i].dfxtype);
				ImGui::SameLine();
				ImGui::Checkbox("Write-protected", &changed_prefs.floppy_read_only);
				ImGui::SameLine();
				ImGui::InputText("", changed_prefs.floppyslots[i].df, MAX_DPATH);
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
		else if (last_active_panel == PANEL_HD)
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
		else if (last_active_panel == PANEL_EXPANSIONS)
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
		else if (last_active_panel == PANEL_RTG)
		{
			const char* rtg_boards[] = { "-", "UAE Zorro II", "UAE Zorro III", "PCI bridgeboard" };
			ImGui::Combo("RTG Graphics Board", &changed_prefs.rtgboards[0].rtgmem_type, rtg_boards, IM_ARRAYSIZE(rtg_boards));
			ImGui::SliderInt("VRAM size", (int*)&changed_prefs.rtgboards[0].rtgmem_size, 0, 0x10000000);
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
		else if (last_active_panel == PANEL_HWINFO)
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
		else if (last_active_panel == PANEL_DISPLAY)
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
		else if (last_active_panel == PANEL_SOUND)
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
		else if (last_active_panel == PANEL_INPUT)
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
		else if (last_active_panel == PANEL_IO)
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
		else if (last_active_panel == PANEL_CUSTOM)
		{
			ImGui::RadioButton("Port 0: Mouse", &SelectedPort, 0);
			ImGui::RadioButton("Port 1: Joystick", &SelectedPort, 1);
			ImGui::RadioButton("Port 2: Parallel 1", &SelectedPort, 2);
			ImGui::RadioButton("Port 3: Parallel 2", &SelectedPort, 3);

			ImGui::RadioButton("None", &SelectedFunction, 0);
			ImGui::RadioButton("HotKey", &SelectedFunction, 1);

			ImGui::InputText("##SetHotkey", "", 120);
			ImGui::Button("...");
			ImGui::Button("X");

			ImGui::InputText("Input Device", "", 256);

			for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
			{
				ImGui::Text("%s", label_button_list[i].c_str());
				const char* items[] = { "None" };
				ImGui::Combo("", &changed_prefs.jports[SelectedPort].autofire, items, IM_ARRAYSIZE(items));
			}

			for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
			{
				ImGui::Text("%s", label_axis_list[i].c_str());
				const char* items[] = { "None" };
				ImGui::Combo("", &changed_prefs.jports[SelectedPort].autofire, items, IM_ARRAYSIZE(items));
			}

			if (ImGui::Button("Save as default mapping"))
			{
				// Save mapping
			}
		}
		else if (last_active_panel == PANEL_DISK_SWAPPER)
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
				ImGui::Text("%d", i + 1); ImGui::NextColumn();
				ImGui::InputText("", changed_prefs.dfxlist[i], MAX_DPATH);
				ImGui::SameLine();
				if (ImGui::Button("..."))
				{
					// Select file
				}
				ImGui::SameLine();
				if (ImGui::Button("X"))
				{
					changed_prefs.dfxlist[i][0] = 0;
				}
				ImGui::NextColumn();
				int drive = disk_in_drive(i);
				if (ImGui::Button(drive == -1 ? "-" : std::to_string(drive).c_str()))
				{
					disk_swap(i, 1);
				}
				ImGui::NextColumn();
			}
			ImGui::Columns(1);
			ImGui::EndChild();

			if (ImGui::Button("Remove All"))
			{
				for (auto& row : changed_prefs.dfxlist)
					row[0] = 0;
			}
		}
		else if (last_active_panel == PANEL_MISC)
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
		else if (last_active_panel == PANEL_PRIO)
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
		else if (last_active_panel == PANEL_SAVESTATES)
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
		ImGui::EndChild();

		// Button bar
		ImGui::BeginChild("ButtonBar", ImVec2(0, 0), true);
		if (ImGui::Button("Quit"))
			uae_quit();
		ImGui::SameLine();
		if (ImGui::Button("Restart"))
			uae_reset(1, 1);
		ImGui::SameLine();
		if (ImGui::Button("Start"))
			gui_running = false;
		ImGui::EndChild();

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
}
#endif
