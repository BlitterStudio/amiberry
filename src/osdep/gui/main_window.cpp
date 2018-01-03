#include <iostream>
#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#endif
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "autoconf.h"

#include "inputdevice.h"

#if defined(ANDROIDSDL)
#include "androidsdl_event.h"
#include <SDL_screenkeyboard.h>
#include <SDL_android.h>
#include <android/log.h>
#endif

SDL_Joystick* GUIjoy;

bool gui_running = false;
static int last_active_panel = 2;

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
	{ "About",            "data/amigainfo.ico", nullptr, nullptr, InitPanelAbout, ExitPanelAbout,     RefreshPanelAbout,      HelpPanelAbout },
	{ "Paths",            "data/paths.ico", nullptr, nullptr, InitPanelPaths,     ExitPanelPaths,     RefreshPanelPaths,      HelpPanelPaths },
	{ "Quickstart",       "data/quickstart.ico", nullptr, nullptr, InitPanelQuickstart,  ExitPanelQuickstart,  RefreshPanelQuickstart, HelpPanelQuickstart },
	{ "Configurations",   "data/file.ico", nullptr, nullptr, InitPanelConfig,    ExitPanelConfig,    RefreshPanelConfig,     HelpPanelConfig },
	{ "CPU and FPU",      "data/cpu.ico", nullptr, nullptr, InitPanelCPU,       ExitPanelCPU,       RefreshPanelCPU,        HelpPanelCPU },
	{ "Chipset",          "data/cpu.ico", nullptr, nullptr, InitPanelChipset,   ExitPanelChipset,   RefreshPanelChipset,    HelpPanelChipset },
	{ "ROM",              "data/chip.ico", nullptr, nullptr, InitPanelROM,       ExitPanelROM,       RefreshPanelROM,        HelpPanelROM },
	{ "RAM",              "data/chip.ico", nullptr, nullptr, InitPanelRAM,       ExitPanelRAM,       RefreshPanelRAM,        HelpPanelRAM },
	{ "Floppy drives",    "data/35floppy.ico", nullptr, nullptr, InitPanelFloppy,    ExitPanelFloppy,    RefreshPanelFloppy,     HelpPanelFloppy },
	{ "Hard drives/CD",   "data/drive.ico", nullptr, nullptr, InitPanelHD,        ExitPanelHD,        RefreshPanelHD,         HelpPanelHD },
	{ "Display",          "data/screen.ico", nullptr, nullptr, InitPanelDisplay,   ExitPanelDisplay,   RefreshPanelDisplay,    HelpPanelDisplay },
	{ "Sound",            "data/sound.ico", nullptr, nullptr, InitPanelSound,     ExitPanelSound,     RefreshPanelSound,      HelpPanelSound },
	{ "Input",            "data/joystick.ico", nullptr, nullptr, InitPanelInput,     ExitPanelInput,     RefreshPanelInput,      HelpPanelInput },
	{ "Custom controls",  "data/controller.png",  nullptr, nullptr, InitPanelCustom,     ExitPanelCustom,     RefreshPanelCustom,      HelpPanelCustom },
	{ "Miscellaneous",    "data/misc.ico",      nullptr, nullptr, InitPanelMisc,      ExitPanelMisc,      RefreshPanelMisc,       HelpPanelMisc },
	{ "Savestates",       "data/savestate.png", nullptr, nullptr, InitPanelSavestate, ExitPanelSavestate, RefreshPanelSavestate,  HelpPanelSavestate },
#ifdef ANDROIDSDL  
	{ "OnScreen",         "data/screen.ico",    NULL, NULL, InitPanelOnScreen,  ExitPanelOnScreen, RefreshPanelOnScreen,  HelpPanelOnScreen },
#endif
	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
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
	PANEL_DISPLAY,
	PANEL_SOUND,
	PANEL_INPUT,
	PANEL_CUSTOM,
	PANEL_MISC,
	PANEL_SAVESTATES,
#ifdef ANDROIDSDL
	PANEL_ONSCREEN,
#endif
	NUM_PANELS
};


/*
* SDL Stuff we need
*/
SDL_Surface* gui_screen;
#ifdef USE_SDL2
SDL_Texture* gui_texture;
SDL_Cursor* cursor;
SDL_Surface* cursor_surface;
#endif

/*
* Gui SDL stuff we need
*/
gcn::SDLInput* gui_input;
gcn::SDLGraphics* gui_graphics;
gcn::SDLImageLoader* gui_imageLoader;
#ifdef USE_SDL1
gcn::contrib::SDLTrueTypeFont* gui_font;
#elif USE_SDL2
gcn::SDLTrueTypeFont* gui_font;
#endif

/*
* Gui stuff we need
*/
gcn::Gui* uae_gui;
gcn::Container* gui_top;
gcn::Container* selectors;
gcn::Color gui_baseCol;
gcn::Color colTextboxBackground;
gcn::Color colSelectorInactive;
gcn::Color colSelectorActive;

namespace widgets
{
	// Main buttons
	gcn::Button* cmdQuit;
	gcn::Button* cmdReset;
	gcn::Button* cmdRestart;
	gcn::Button* cmdStart;
	gcn::Button* cmdHelp;
	gcn::Button* cmdShutdown;
}


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

static int gui_create_rtarea_flag(struct uae_prefs* p)
{
	auto flag = 0;

	if (count_HDs(p) > 0)
		flag |= 1;

	if (p->socket_emu)
		flag |= 4;

	if (p->input_tablet > 0)
		flag |= 8;

	if (p->rtgboards[0].rtgmem_size)
		flag |= 16;

	if (p->chipmem_size > 2 * 1024 * 1024)
		flag |= 32;

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

static void (*refreshFuncAfterDraw)(void) = nullptr;

void RegisterRefreshFunc(void (*func)(void))
{
	refreshFuncAfterDraw = func;
}

void FocusBugWorkaround(gcn::Window* wnd)
{
	// When modal dialog opens via mouse, the dialog will not
	// have the focus unless there is a mouse click. We simulate the click...
	SDL_Event event;
	event.type = SDL_MOUSEBUTTONDOWN;
	event.button.button = SDL_BUTTON_LEFT;
	event.button.state = SDL_PRESSED;
	event.button.x = wnd->getX() + 2;
	event.button.y = wnd->getY() + 2;
	gui_input->pushInput(event);
	event.type = SDL_MOUSEBUTTONUP;
	gui_input->pushInput(event);
}

static void ShowHelpRequested()
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

void UpdateGuiScreen()
{
#ifdef USE_SDL1
	wait_for_vsync();
	SDL_Flip(gui_screen);
#elif USE_SDL2
	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
	
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, gui_texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
#endif
}

namespace sdl
{
#ifdef USE_SDL2
	// Sets the cursor image up
	void setup_cursor() 
	{
		// Detect resolution and load appropiate cursor image
		if (sdlMode.w > 1280)
		{
			cursor_surface = SDL_LoadBMP("data/cursor-x2.bmp");
		}
		else
		{
			cursor_surface = SDL_LoadBMP("data/cursor.bmp");
		}
		
		if (cursor_surface == nullptr)
		{
			// Load failed. Log error.
			cout << "Could not load cursor bitmap: " << SDL_GetError() << endl;
			return;
		}

		// Create new cursor with surface
		cursor = SDL_CreateColorCursor(cursor_surface, 0, 0);
		if (cursor == nullptr)
		{
			// Cursor creation failed. Log error and free surface
			cout << "Could not create color cursor: " << SDL_GetError() << endl;
			SDL_FreeSurface(cursor_surface);
			cursor_surface = nullptr;
			return;
		}

		SDL_SetCursor(cursor);
	}
#endif

	void gui_init()
	{
		//-------------------------------------------------
		// Create new screen for GUI
		//-------------------------------------------------
#ifdef USE_SDL1
#ifdef DEBUG
		const auto videoInfo = SDL_GetVideoInfo();
		printf("Current resolution: %d x %d %d bpp\n", videoInfo->current_w, videoInfo->current_h,
			videoInfo->vfmt->BitsPerPixel);
#endif //DEBUG
		gui_screen = SDL_SetVideoMode(GUI_WIDTH, GUI_HEIGHT, 16, SDL_SWSURFACE | SDL_FULLSCREEN);
		check_error_sdl(gui_screen == nullptr, "Unable to create GUI surface");
		SDL_EnableUNICODE(1);
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		SDL_ShowCursor(SDL_ENABLE);
#endif
#ifdef USE_SDL2

		setup_cursor();

		// make the scaled rendering look smoother (linear scaling).
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create GUI surface");

		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);

		gui_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, gui_screen->w, gui_screen->h);
		check_error_sdl(gui_texture == nullptr, "Unable to create GUI texture");

		if (cursor)
			SDL_ShowCursor(SDL_ENABLE);
#endif
#ifdef ANDROIDSDL
		// Enable Android multitouch
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
		SDL_JoystickOpen(0);
#endif
		//-------------------------------------------------
		// Create helpers for GUI framework
		//-------------------------------------------------
		gui_imageLoader = new gcn::SDLImageLoader();
		// The ImageLoader in use is static and must be set to be
		// able to load images
		gcn::Image::setImageLoader(gui_imageLoader);
		gui_graphics = new gcn::SDLGraphics();
		// Set the target for the graphics object to be the screen.
		// In other words, we will draw to the screen.
		// Note, any surface will do, it doesn't have to be the screen.
		gui_graphics->setTarget(gui_screen);
		gui_input = new gcn::SDLInput();
		uae_gui = new gcn::Gui();
		uae_gui->setGraphics(gui_graphics);
		uae_gui->setInput(gui_input);
	}

	void gui_halt()
	{
		delete uae_gui;
		delete gui_imageLoader;
		delete gui_input;
		delete gui_graphics;

		if (gui_screen != nullptr)
		{
			SDL_FreeSurface(gui_screen);
			gui_screen = nullptr;
		}

#ifdef USE_SDL2
		if (gui_texture != nullptr)
		{
			SDL_DestroyTexture(gui_texture);
			gui_texture = nullptr;
		}

		if (cursor)
		{
			SDL_FreeCursor(cursor);
			cursor = nullptr;
		}

		if (cursor_surface)
		{
			SDL_FreeSurface(cursor_surface);
			cursor_surface = nullptr;
		}

		// Clear the screen
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
#endif
	}

	void checkInput()
	{
#ifdef USE_SDL1
		const auto key_for_gui = SDLK_F12;
#elif USE_SDL2
		const auto key_for_gui = SDL_GetKeyFromName(currprefs.open_gui);
#endif
		SDL_Event gui_event;
		while (SDL_PollEvent(&gui_event))
		{
			if (gui_event.type == SDL_QUIT)
			{
				//-------------------------------------------------
				// Quit entire program via SQL-Quit
				//-------------------------------------------------
				uae_quit();
				gui_running = false;
				break;
			}

			// Horace's start at implementing Joystick control
			if (gui_event.type == SDL_JOYBUTTONDOWN || gui_event.type == SDL_JOYHATMOTION)
			{
				gcn::FocusHandler* focusHdl;
				gcn::Widget* activeWidget;

				if (GUIjoy)
				{
					const int hat = SDL_JoystickGetHat(GUIjoy, 0);

					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_up) || (hat & SDL_HAT_UP)) // dpad
					{
						if (HandleNavigation(DIRECTION_UP))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_UP);
						break;
					}
					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_down) || (hat & SDL_HAT_DOWN)) // dpad
					{
						if (HandleNavigation(DIRECTION_DOWN))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_DOWN);
						break;
					}


					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].left_shoulder)) // dpad
					{
						for (int z = 0; z<10; ++z)
						{
							PushFakeKey(SDLK_UP);
						}
					}
					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].right_shoulder)) // dpad
					{
						for (int z = 0; z<10; ++z)
						{
							PushFakeKey(SDLK_DOWN);
						}
					}


					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_right) || (hat & SDL_HAT_RIGHT)) // dpad
					{
						if (HandleNavigation(DIRECTION_RIGHT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_RIGHT);
						break;
					}
					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_left) || (hat & SDL_HAT_LEFT)) // dpad
					{
						if (HandleNavigation(DIRECTION_LEFT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_LEFT);
						break;
					}
					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].south_button)) // need this to be X button
					{
						PushFakeKey(SDLK_RETURN);
						continue;
					}

					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].quit_button) &&
						SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].hotkey_button)) // use the HOTKEY button
					{
						uae_quit();
						gui_running = false;
						break;
					}
					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].left_trigger))
					{
						ShowHelpRequested();
						widgets::cmdHelp->requestFocus();
						break;
					}
					if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].menu_button)) // use the HOTKEY button
					{
						gui_running = false;
					}
				}
				break;
			}

			if (gui_event.type == SDL_KEYDOWN)
			{
				gcn::FocusHandler* focusHdl;
				gcn::Widget* activeWidget;

				if (gui_event.key.keysym.sym == key_for_gui)
				{
					if (emulating && widgets::cmdStart->isEnabled())
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
				else
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
						ShowHelpRequested();
						widgets::cmdHelp->requestFocus();
						break;

					default:
						break;
					}
			}

			//-------------------------------------------------
			// Send event to gui-controls
			//-------------------------------------------------
#ifdef ANDROIDSDL
			androidsdl_event(event, gui_input);
#else
			gui_input->pushInput(gui_event);
#endif
		}
	}

	void gui_run()
	{
		if (SDL_NumJoysticks() > 0)
			GUIjoy = SDL_JoystickOpen(0);

		//-------------------------------------------------
		// The main loop
		//-------------------------------------------------
		while (gui_running)
		{
			// Poll input
			checkInput();

			if (gui_rtarea_flags_onenter != gui_create_rtarea_flag(&changed_prefs))
				DisableResume();

			// Now we let the Gui object perform its logic.
			uae_gui->logic();
			// Now we let the Gui object draw itself.
			uae_gui->draw();

			// Finally we update the screen.
			UpdateGuiScreen();

			if (refreshFuncAfterDraw != nullptr)
			{
				void(*currFunc)() = refreshFuncAfterDraw;
				refreshFuncAfterDraw = nullptr;
				currFunc();
			}
		}

		if (GUIjoy)
			SDL_JoystickClose(GUIjoy);
	}
}

namespace widgets
{
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
				fetch_configurationpath(tmp, sizeof tmp);
				if (strlen(last_loaded_config) > 0)
					strncat(tmp, last_loaded_config, MAX_DPATH - 1);
				else
				{
					strncat(tmp, OPTIONSFILENAME, MAX_DPATH);
					strncat(tmp, ".uae", MAX_DPATH);
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
				ShowHelpRequested();
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


	void gui_init()
	{
		int i;
		int yPos;

		//-------------------------------------------------
		// Define base colors
		//-------------------------------------------------
		gui_baseCol = gcn::Color(170, 170, 170);
		colSelectorInactive = gcn::Color(170, 170, 170);
		colSelectorActive = gcn::Color(103, 136, 187);
		colTextboxBackground = gcn::Color(220, 220, 220);

		//-------------------------------------------------
		// Create container for main page
		//-------------------------------------------------
		gui_top = new gcn::Container();
#ifdef USE_SDL1
		gui_top->setDimension(gcn::Rectangle((gui_screen->w - GUI_WIDTH) / 2, (gui_screen->h - GUI_HEIGHT) / 2, GUI_WIDTH,
			GUI_HEIGHT));
#elif USE_SDL2
		gui_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
#endif
		gui_top->setBaseColor(gui_baseCol);
		uae_gui->setTop(gui_top);

		//-------------------------------------------------
		// Initialize fonts
		//-------------------------------------------------
		TTF_Init();
#ifdef USE_SDL1
		try
		{
			gui_font = new gcn::contrib::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
		}
		catch (const std::exception& ex)
		{
			write_log("Could not open data/AmigaTopaz.ttf!\n");
			abort();
		}
		catch (...)
		{
			write_log("An error occurred while trying to open data/AmigaTopaz.ttf!\n");
			abort();
		}
#elif USE_SDL2
		try
		{
			gui_font = new gcn::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
		}
		catch(const std::exception& ex)
		{
			write_log("Could not open data/AmigaTopaz.ttf!\n");
			abort();
		}
		catch (...)
		{
			write_log("An error occurred while trying to open data/AmigaTopaz.ttf!\n");
			abort();
		}		
#endif
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
		const auto workAreaHeight = GUI_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y;
		selectors = new gcn::Container();
		selectors->setSize(150, workAreaHeight - 2);
		selectors->setBaseColor(colSelectorInactive);
#ifdef USE_SDL1
		selectors->setFrameSize(1);
#elif USE_SDL2
		selectors->setBorderSize(1);
#endif
		const auto panelStartX = DISTANCE_BORDER + selectors->getWidth() + 2 + 11;

		panelFocusListener = new PanelFocusListener();
		for (i = 0; categories[i].category != nullptr; ++i)
		{
			categories[i].selector = new gcn::SelectorEntry(categories[i].category, categories[i].imagepath);
			categories[i].selector->setActiveColor(colSelectorActive);
			categories[i].selector->setInactiveColor(colSelectorInactive);
			categories[i].selector->setSize(150, 24);
			categories[i].selector->addFocusListener(panelFocusListener);

			categories[i].panel = new gcn::Container();
			categories[i].panel->setId(categories[i].category);
			categories[i].panel->setSize(GUI_WIDTH - panelStartX - DISTANCE_BORDER - 1, workAreaHeight - 2);
			categories[i].panel->setBaseColor(gui_baseCol);
#ifdef USE_SDL1
			categories[i].panel->setFrameSize(1);
#elif USE_SDLL2
			categories[i].panel->setBorderSize(1);
#endif
			categories[i].panel->setVisible(false);
		}

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
		gui_top->add(cmdReset, DISTANCE_BORDER, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
		gui_top->add(cmdQuit, DISTANCE_BORDER + BUTTON_WIDTH + DISTANCE_NEXT_X, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
		gui_top->add(cmdShutdown, DISTANCE_BORDER + 2 * BUTTON_WIDTH + 2 * DISTANCE_NEXT_X, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
		gui_top->add(cmdHelp, DISTANCE_BORDER + 3 * BUTTON_WIDTH + 3 * DISTANCE_NEXT_X, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);
		gui_top->add(cmdStart, GUI_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, GUI_HEIGHT - DISTANCE_BORDER - BUTTON_HEIGHT);

		gui_top->add(selectors, DISTANCE_BORDER + 1, DISTANCE_BORDER + 1);
		for (i = 0, yPos = 0; categories[i].category != nullptr; ++i, yPos += 24)
		{
			selectors->add(categories[i].selector, 0, yPos);
			gui_top->add(categories[i].panel, panelStartX, DISTANCE_BORDER + 1);
		}

		//--------------------------------------------------
		// Activate last active panel
		//--------------------------------------------------
		if (!emulating && quickstart_start)
			last_active_panel = 2;
		categories[last_active_panel].selector->requestFocus();
		cmdHelp->setVisible(categories[last_active_panel].HelpFunc != nullptr);
	}


	void gui_halt()
	{
		int i;

		for (i = 0; categories[i].category != nullptr; ++i)
		{
			if (categories[i].ExitFunc != nullptr)
				(*categories[i].ExitFunc)();
		}

		for (i = 0; categories[i].category != nullptr; ++i)
			delete categories[i].selector;
		delete panelFocusListener;
		delete selectors;

		delete cmdQuit;
		delete cmdShutdown;
		delete cmdReset;
		delete cmdRestart;
		delete cmdStart;
		delete cmdHelp;

		delete mainButtonActionListener;

		delete gui_font;
		delete gui_top;
	}
}


void RefreshAllPanels()
{
	for (auto i = 0; categories[i].category != nullptr; ++i)
	{
		if (categories[i].RefreshFunc != nullptr)
			(*categories[i].RefreshFunc)();
	}
}


void DisableResume()
{
	if (emulating)
	{
		widgets::cmdStart->setEnabled(false);
		gcn::Color backCol;
		backCol.r = 128;
		backCol.g = 128;
		backCol.b = 128;
		widgets::cmdStart->setForegroundColor(backCol);
	}
}


void run_gui()
{
#ifdef ANDROIDSDL
	SDL_ANDROID_SetScreenKeyboardShown(0);
	SDL_ANDROID_SetSystemMousePointerVisible(1);
#endif
	gui_running = true;
	gui_rtarea_flags_onenter = gui_create_rtarea_flag(&currprefs);

	expansion_generate_autoconfig_info(&changed_prefs);

	try
	{
		sdl::gui_init();
		widgets::gui_init();
		if (_tcslen(startup_message) > 0)
		{
			ShowMessage(startup_title, startup_message, _T(""), _T("Ok"), _T(""));
			_tcscpy(startup_title, _T(""));
			_tcscpy(startup_message, _T(""));
			widgets::cmdStart->requestFocus();
		}
		sdl::gui_run();
		widgets::gui_halt();
		sdl::gui_halt();
#ifdef ANDROIDSDL
		if (currprefs.onScreen != 0)
		{
			SDL_ANDROID_SetScreenKeyboardShown(1);
			SDL_ANDROID_SetSystemMousePointerVisible(0);
		}
#endif 
	}

	// Catch all GUI framework exceptions.
	catch (gcn::Exception &e)
	{
		cout << e.getMessage() << endl;
		uae_quit();
	}

	// Catch all Std exceptions.
	catch (exception &e)
	{
		cout << "Std exception: " << e.what() << endl;
		uae_quit();
	}

	// Catch all unknown exceptions.
	catch (...)
	{
		cout << "Unknown exception" << endl;
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
		screen_is_picasso = 0;

		if (gui_rtarea_flags_onenter != gui_create_rtarea_flag(&changed_prefs))
			quit_program = -UAE_RESET_HARD; // Hardreset required...
	}

	// Reset counter for access violations
	init_max_signals();
}
