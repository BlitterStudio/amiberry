#include <iostream>
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "gui.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "autoconf.h"

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
	{ "Paths",            "data/paths.ico", nullptr, nullptr, InitPanelPaths,     ExitPanelPaths,     RefreshPanelPaths,      HelpPanelPaths },
	{ "Quickstart",       "data/quickstart.ico", nullptr, nullptr, InitPanelQuickstart,  ExitPanelQuickstart,  RefreshPanelQuickstart, HelpPanelQuickstart },
	{ "Configurations",   "data/file.ico", nullptr, nullptr, InitPanelConfig,    ExitPanelConfig,    RefreshPanelConfig,     HelpPanelConfig },
	{ "CPU and FPU",      "data/cpu.ico", nullptr, nullptr, InitPanelCPU,       ExitPanelCPU,       RefreshPanelCPU,        HelpPanelCPU },
	{ "Chipset",          "data/cpu.ico", nullptr, nullptr, InitPanelChipset,   ExitPanelChipset,   RefreshPanelChipset,    HelpPanelChipset },
	{ "ROM",              "data/chip.ico", nullptr, nullptr, InitPanelROM,       ExitPanelROM,       RefreshPanelROM,        HelpPanelROM },
	{ "RAM",              "data/chip.ico", nullptr, nullptr, InitPanelRAM,       ExitPanelRAM,       RefreshPanelRAM,        HelpPanelRAM },
	{ "Floppy drives",    "data/35floppy.ico", nullptr, nullptr, InitPanelFloppy,    ExitPanelFloppy,    RefreshPanelFloppy,     HelpPanelFloppy },
	{ "Hard drives/CD", "data/drive.ico", nullptr, nullptr, InitPanelHD,        ExitPanelHD,        RefreshPanelHD,         HelpPanelHD },
	{ "Display",          "data/screen.ico", nullptr, nullptr, InitPanelDisplay,   ExitPanelDisplay,   RefreshPanelDisplay,    HelpPanelDisplay },
	{ "Sound",            "data/sound.ico", nullptr, nullptr, InitPanelSound,     ExitPanelSound,     RefreshPanelSound,      HelpPanelSound },
	{ "Input",            "data/joystick.ico", nullptr, nullptr, InitPanelInput,     ExitPanelInput,     RefreshPanelInput,      HelpPanelInput },
	{ "Miscellaneous",    "data/misc.ico", nullptr, nullptr, InitPanelMisc,      ExitPanelMisc,      RefreshPanelMisc,       HelpPanelMisc },
	{ "Savestates",       "data/savestate.png", nullptr, nullptr, InitPanelSavestate, ExitPanelSavestate, RefreshPanelSavestate,  HelpPanelSavestate },
	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
};

enum {
	PANEL_PATHS, PANEL_QUICKSTART, PANEL_CONFIGURATIONS, PANEL_CPU, PANEL_CHIPSET, PANEL_ROM, PANEL_RAM,
	PANEL_FLOPPY, PANEL_HD, PANEL_DISPLAY, PANEL_SOUND, PANEL_INPUT, PANEL_MISC, PANEL_SAVESTATES,
	NUM_PANELS
};


/*
* SDL Stuff we need
*/
SDL_Surface* gui_screen;
SDL_Texture* gui_texture;
SDL_Event gui_event;
SDL_Cursor* cursor;
SDL_Surface* cursorSurface;

/*
* Guisan SDL stuff we need
*/
gcn::SDLInput* gui_input;
gcn::SDLGraphics* gui_graphics;
gcn::SDLImageLoader* gui_imageLoader;
gcn::SDLTrueTypeFont* gui_font;

/*
* Guisan stuff we need
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
	gcn::Button* cmdShutdown;
	gcn::Button* cmdHelp;
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
	int flag = 0;

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

static void (*refreshFuncAfterDraw)() = nullptr;

void RegisterRefreshFunc(void (*func)())
{
	refreshFuncAfterDraw = func;
}

void FocusBugWorkaround(gcn::Window *wnd)
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
	std::vector<std::string> helptext;
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
	// Update the texture from the surface
	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
	// Copy the texture on the renderer
	SDL_RenderCopy(renderer, gui_texture, nullptr, nullptr);
	// Update the window surface (show the renderer)
	SDL_RenderPresent(renderer);
}

namespace sdl
{
	// Sets the cursor image up
	void setup_cursor() 
	{
		// Detect resolution and load appropiate cursor image
		if (sdlMode.w > 1280)
		{
			cursorSurface = SDL_LoadBMP("data/cursor-x2.bmp");
		}
		else
		{
			cursorSurface = SDL_LoadBMP("data/cursor.bmp");
		}
		
		if (cursorSurface == nullptr)
		{
			// Load failed. Log error.
			cout << "Could not load cursor bitmap: " << SDL_GetError() << endl;
			return;
		}

		// Create new cursor with surface
		cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
		if (cursor == nullptr)
		{
			// Cursor creation failed. Log error and free surface
			cout << "Could not create color cursor: " << SDL_GetError() << endl;
			SDL_FreeSurface(cursorSurface);
			cursorSurface = nullptr;
			return;
		}

		SDL_SetCursor(cursor);
	}

	void gui_init()
	{
		//-------------------------------------------------
		// Create new screen for GUI
		//-------------------------------------------------
		setup_cursor();

		// make the scaled rendering look smoother (linear scaling).
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 32, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create a surface");

		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);

		gui_texture = SDL_CreateTextureFromSurface(renderer, gui_screen);
		check_error_sdl(gui_texture == nullptr, "Unable to create texture");

		if (cursor)
		{
			SDL_ShowCursor(SDL_ENABLE);
		}

		//-------------------------------------------------
		// Create helpers for guisan
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

		SDL_FreeSurface(gui_screen);
		SDL_DestroyTexture(gui_texture);
		if (cursor)
		{
			SDL_FreeCursor(cursor);
			cursor = nullptr;
		}
		if (cursorSurface)
		{
			SDL_FreeSurface(cursorSurface);
			cursorSurface = nullptr;
		}
		gui_screen = nullptr;
	}

	void checkInput()
	{
		while (SDL_PollEvent(&gui_event))
		{
			if (gui_event.type == SDL_KEYDOWN)
			{
				gcn::FocusHandler* focusHdl;
				gcn::Widget* activeWidget;

				if (gui_event.key.keysym.sym == SDL_GetKeyFromName(currprefs.open_gui))
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
					switch (gui_event.key.keysym.scancode)
					{
					case SDL_SCANCODE_Q:
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
						gui_running = false;
						break;

					case VK_Red:
					case VK_Green:
						//------------------------------------------------
						// Simulate press of enter when 'X' pressed
						//------------------------------------------------
						gui_event.key.keysym.scancode = SDL_SCANCODE_RETURN;
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
					default:
						break;
					}
			}
			else if (gui_event.type == SDL_QUIT)
			{
				//-------------------------------------------------
				// Quit entire program via SQL-Quit
				//-------------------------------------------------
				uae_quit();
				gui_running = false;
			}
			//-------------------------------------------------
			// Send event to guichan-controls
			//-------------------------------------------------
			gui_input->pushInput(gui_event);
		}
	}

	void gui_run()
	{
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
				void (*currFunc)() = refreshFuncAfterDraw;
				refreshFuncAfterDraw = nullptr;
				currFunc();
			}
		}
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
				host_shutdown();
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
					strcat(tmp, last_loaded_config);
				else
				{
					strcat(tmp, OPTIONSFILENAME);
					strcat(tmp, ".uae");
				}
				uae_restart(0, tmp);
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
		}
	};

	MainButtonActionListener* mainButtonActionListener;


	class PanelFocusListener : public gcn::FocusListener
	{
	public:
		void focusGained(const gcn::Event& event) override
		{
			for (int i = 0; categories[i].category != nullptr; ++i)
			{
				if (event.getSource() == categories[i].selector)
				{
					categories[i].selector->setActive(true);
					categories[i].panel->setVisible(true);
					last_active_panel = i;
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
		gui_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
		gui_top->setBaseColor(gui_baseCol);
		uae_gui->setTop(gui_top);

		//-------------------------------------------------
		// Initialize fonts
		//-------------------------------------------------
		TTF_Init();
		gui_font = new gcn::SDLTrueTypeFont("data/Topaznew.ttf", 14);
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
		int workAreaHeight = GUI_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y;
		selectors = new gcn::Container();
		selectors->setSize(150, workAreaHeight - 2);
		selectors->setBaseColor(colSelectorInactive);
		selectors->setBorderSize(1);
		int panelStartX = DISTANCE_BORDER + selectors->getWidth() + 2 + 11;

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
			categories[i].panel->setBorderSize(1);
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
		for (i = 0 , yPos = 0; categories[i].category != nullptr; ++i , yPos += 24)
		{
			selectors->add(categories[i].selector, 0, yPos);
			gui_top->add(categories[i].panel, panelStartX, DISTANCE_BORDER + 1);
		}

		//--------------------------------------------------
		// Activate last active panel
		//--------------------------------------------------
		if (!emulating && quickstart_start)
			last_active_panel = 1;
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
	for (int i = 0; categories[i].category != nullptr; ++i)
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
	gui_running = true;
	gui_rtarea_flags_onenter = gui_create_rtarea_flag(&currprefs);

	expansion_generate_autoconfig_info(&changed_prefs);

	try
	{
		sdl::gui_init();
		widgets::gui_init();
		if (_tcslen(startup_message) > 0) {
			ShowMessage(startup_title, startup_message, _T(""), _T("Ok"), _T(""));
			_tcscpy(startup_title, _T(""));
			_tcscpy(startup_message, _T(""));
			widgets::cmdStart->requestFocus();
		}
		sdl::gui_run();
		widgets::gui_halt();
		sdl::gui_halt();
	}
	
	// Catch all guisan exceptions.
	catch (gcn::Exception e)
	{
		cout << e.getMessage() << endl;
		uae_quit();
	}

	// Catch all Std exceptions.
	catch (exception e)
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
