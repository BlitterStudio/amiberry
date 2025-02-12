#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <dpi_handler.hpp>

#include <guisan.hpp>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
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
#include "fsdb.h"
#include "inputdevice.h"
#include "xwin.h"

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
	{"WHDLoad", "drive.ico", nullptr, nullptr, InitPanelWHDLoad, ExitPanelWHDLoad, RefreshPanelWHDLoad, HelpPanelWHDLoad},

	{"Themes", "amigainfo.ico", nullptr, nullptr, InitPanelThemes, ExitPanelThemes, RefreshPanelThemes, HelpPanelThemes},

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

	//-------------------------------------------------
	// Create new screen for GUI
	//-------------------------------------------------
	if (!gui_screen)
	{
		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create GUI surface:");
	}

	if (!mon->gui_window)
	{
		write_log("Creating Amiberry GUI window...\n");
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
				gui_window_rect.x != 0 ? gui_window_rect.x : SDL_WINDOWPOS_CENTERED,
				gui_window_rect.y != 0 ? gui_window_rect.y : SDL_WINDOWPOS_CENTERED,
				gui_window_rect.w,
				gui_window_rect.h,
				mode);
        }
        else
        {
			mon->gui_window = SDL_CreateWindow("Amiberry GUI",
				gui_window_rect.y != 0 ? gui_window_rect.y : SDL_WINDOWPOS_CENTERED,
				gui_window_rect.x != 0 ? gui_window_rect.x : SDL_WINDOWPOS_CENTERED,
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

	auto render_scale_quality = "linear";
	bool integer_scale = false;

#ifdef __MACH__
	render_scale_quality = "nearest";
	integer_scale = true;
#endif

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, render_scale_quality);
	SDL_RenderSetIntegerScale(mon->gui_renderer, integer_scale ? SDL_TRUE : SDL_FALSE);

	gui_texture = SDL_CreateTexture(mon->gui_renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING, gui_screen->w,
									gui_screen->h);
	check_error_sdl(gui_texture == nullptr, "Unable to create GUI texture:");

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_HEIGHT, GUI_WIDTH);

	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);

	SDL_RaiseWindow(mon->gui_window);

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
}

void amiberry_gui_halt()
{
	AmigaMonitor* mon = &AMonitors[0];

	delete gui_imageLoader;
	gui_imageLoader = nullptr;
	delete gui_input;
	gui_input = nullptr;
	delete gui_graphics;
	gui_graphics = nullptr;

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
		SDL_DestroyWindow(mon->gui_window);
		mon->gui_window = nullptr;
	}
}

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
