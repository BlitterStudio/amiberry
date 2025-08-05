#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

#include <guisan.hpp>
#include <SDL_image.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"

#include "options.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"
#include "fsdb_host.h"
#include "xwin.h"

enum
{
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 200
};

static bool dialogResult = false;
static bool dialogFinished = false;
static amiberry_hotkey hotkey = {};
static bool halt_gui = false;
static bool modifier_only_pressed = false;

static gcn::Window* wndShowMessage;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::TextBox* txtMessageText;
static gcn::Label* lblText1;
static gcn::Label* lblText2;

class ShowMessageActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
			dialogResult = true;
		else if (actionEvent.getSource() == cmdCancel)
			dialogResult = false;
		dialogFinished = true;
	}
};

static ShowMessageActionListener* showMessageActionListener;

static void InitShowMessage(const std::string& message)
{
	AmigaMonitor* mon = &AMonitors[0];
	sdl_video_driver = SDL_GetCurrentVideoDriver();
	if (sdl_video_driver != nullptr && strcmpi(sdl_video_driver, "KMSDRM") == 0)
		kmsdrm_detected = true;
	SDL_GetCurrentDisplayMode(0, &sdl_mode);

	if (!gui_screen)
	{
		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create SDL surface:");
	}

	if (!mon->gui_window)
	{
		Uint32 mode;
		if (sdl_mode.w >= 800 && sdl_mode.h >= 600 && !kmsdrm_detected)
		{
			mode = SDL_WINDOW_RESIZABLE;
			if (currprefs.gui_alwaysontop)
				mode |= SDL_WINDOW_ALWAYS_ON_TOP;
			if (currprefs.start_minimized)
				mode |= SDL_WINDOW_HIDDEN;
			else
				mode |= SDL_WINDOW_SHOWN;
			// Set Window allow high DPI by default
			mode |= SDL_WINDOW_ALLOW_HIGHDPI;
		}
		else
		{
			// otherwise go for Full-window
			mode = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}

		if (amiberry_options.rotation_angle != 0 && amiberry_options.rotation_angle != 180)
		{
			mon->gui_window = SDL_CreateWindow("Amiberry GUI",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_HEIGHT,
				GUI_WIDTH,
				mode);
		}
		else
		{
			mon->gui_window = SDL_CreateWindow("Amiberry GUI",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_WIDTH,
				GUI_HEIGHT,
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

	if (mon->gui_renderer == nullptr)
	{
		mon->gui_renderer = SDL_CreateRenderer(mon->gui_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(mon->gui_renderer == nullptr, "Unable to create a renderer:");
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_WIDTH, GUI_HEIGHT);
	}

	if (mon->gui_window)
	{
		const auto window_flags = SDL_GetWindowFlags(mon->gui_window);
		const bool is_maximized = window_flags & SDL_WINDOW_MAXIMIZED;
		const bool is_fullscreen = window_flags & SDL_WINDOW_FULLSCREEN;
		if (!is_maximized && !is_fullscreen)
		{
			if (amiberry_options.rotation_angle != 0 && amiberry_options.rotation_angle != 180)
				SDL_SetWindowSize(mon->gui_window, GUI_HEIGHT, GUI_WIDTH);
			else
				SDL_SetWindowSize(mon->gui_window, GUI_WIDTH, GUI_HEIGHT);
		}
	}

	// make the scaled rendering look smoother (linear scaling).
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (gui_texture == nullptr)
	{
		gui_texture = SDL_CreateTexture(mon->gui_renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING,
		                                gui_screen->w, gui_screen->h);
		check_error_sdl(gui_texture == nullptr, "Unable to create texture from Surface");
	}

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(mon->gui_renderer, GUI_HEIGHT, GUI_WIDTH);

	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);

	if (gui_graphics == nullptr)
	{
		gui_graphics = new gcn::SDLGraphics();
		gui_graphics->setTarget(gui_screen);
	}
	if (gui_input == nullptr)
	{
		gui_input = new gcn::SDLInput();
	}
	if (uae_gui == nullptr)
	{
		halt_gui = true;
		uae_gui = new gcn::Gui();
		uae_gui->setGraphics(gui_graphics);
		uae_gui->setInput(gui_input);
	}
	if (gui_top == nullptr)
	{
		gui_top = new gcn::Container();
		gui_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
		gui_base_color = gui_theme.base_color;
		gui_foreground_color = gui_theme.foreground_color;
		gui_font_color = gui_theme.font_color;
		gui_top->setBaseColor(gui_base_color);
		gui_top->setBackgroundColor(gui_base_color);
		gui_top->setForegroundColor(gui_foreground_color);
		uae_gui->setTop(gui_top);
	}
	if (gui_font == nullptr)
	{
		TTF_Init();

		load_theme(amiberry_options.gui_theme);
		apply_theme();
	}

	wndShowMessage = new gcn::Window("Message");
	wndShowMessage->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowMessage->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowMessage->setBaseColor(gui_base_color);
	wndShowMessage->setBackgroundColor(gui_base_color);
	wndShowMessage->setForegroundColor(gui_foreground_color);
	wndShowMessage->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndShowMessage->setMovable(false);

	showMessageActionListener = new ShowMessageActionListener();

	txtMessageText = new gcn::TextBox(message);
	txtMessageText->setBackgroundColor(gui_base_color);
	txtMessageText->setForegroundColor(gui_foreground_color);
	txtMessageText->setEditable(false);
	txtMessageText->setFrameSize(0);
	lblText1 = new gcn::Label("");
	lblText1->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);
	lblText2 = new gcn::Label("");
	lblText2->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->addActionListener(showMessageActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                       DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_base_color);
	cmdCancel->setForegroundColor(gui_foreground_color);
	cmdCancel->addActionListener(showMessageActionListener);

	wndShowMessage->add(txtMessageText, DISTANCE_BORDER, DISTANCE_BORDER);
	wndShowMessage->add(lblText1, DISTANCE_BORDER, DISTANCE_BORDER + txtMessageText->getHeight() + 4);
	wndShowMessage->add(lblText2, DISTANCE_BORDER, lblText1->getY() + lblText1->getHeight() + 4);
	wndShowMessage->add(cmdOK);
	wndShowMessage->add(cmdCancel);

	gui_top->add(wndShowMessage);

	wndShowMessage->requestModalFocus();
	focus_bug_workaround(wndShowMessage);
	cmdOK->requestFocus();
}

static void ExitShowMessage()
{
	const AmigaMonitor* mon = &AMonitors[0];

	wndShowMessage->releaseModalFocus();
	gui_top->remove(wndShowMessage);

	delete txtMessageText;
	delete lblText1;
	delete lblText2;
	delete cmdOK;
	delete cmdCancel;

	delete showMessageActionListener;

	delete wndShowMessage;

	if (halt_gui)
	{
		delete uae_gui;
		uae_gui = nullptr;
		delete gui_input;
		gui_input = nullptr;
		delete gui_graphics;
		gui_graphics = nullptr;
		delete gui_font;
		gui_font = nullptr;
		delete gui_top;
		gui_top = nullptr;

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

		// Clear the screen
		SDL_RenderClear(mon->gui_renderer);
		SDL_RenderPresent(mon->gui_renderer);
	}
}

static std::string BuildCurrentHotkeyDisplay()
{
	std::string display;

	// Add modifiers in a consistent order
	if (hotkey.modifiers.lctrl) display += "LCtrl+";
	if (hotkey.modifiers.rctrl) display += "RCtrl+";
	if (hotkey.modifiers.lalt) display += "LAlt+";
	if (hotkey.modifiers.ralt) display += "RAlt+";
	if (hotkey.modifiers.lshift) display += "LShift+";
	if (hotkey.modifiers.rshift) display += "RShift+";
	if (hotkey.modifiers.lgui) display += "LGUI+";
	if (hotkey.modifiers.rgui) display += "RGUI+";

	// Add the main key if present
	if (!hotkey.key_name.empty()) {
		display += hotkey.key_name;
	} else if (!display.empty() && display.back() == '+') {
		// Remove trailing + if only modifiers
		display.pop_back();
	}

	return display;
}

static void ResetHotkey()
{
	hotkey.modifiers.lctrl = hotkey.modifiers.rctrl =
		hotkey.modifiers.lalt = hotkey.modifiers.ralt =
		hotkey.modifiers.lshift = hotkey.modifiers.rshift =
		hotkey.modifiers.lgui = hotkey.modifiers.rgui = false;
	hotkey.scancode = 0;
	hotkey.button = -1;
	hotkey.key_name.clear();
	hotkey.modifiers_string.clear();
}

static bool HandleModifierKey(SDL_Keycode keycode, bool pressed)
{
	bool is_modifier = true;

	switch (keycode)
	{
	case SDLK_LCTRL:
		hotkey.modifiers.lctrl = pressed;
		break;
	case SDLK_RCTRL:
		hotkey.modifiers.rctrl = pressed;
		break;
	case SDLK_LALT:
		hotkey.modifiers.lalt = pressed;
		break;
	case SDLK_RALT:
		hotkey.modifiers.ralt = pressed;
		break;
	case SDLK_LSHIFT:
		hotkey.modifiers.lshift = pressed;
		break;
	case SDLK_RSHIFT:
		hotkey.modifiers.rshift = pressed;
		break;
	case SDLK_LGUI:
		hotkey.modifiers.lgui = pressed;
		break;
	case SDLK_RGUI:
		hotkey.modifiers.rgui = pressed;
		break;
	default:
		is_modifier = false;
		break;
	}

	return is_modifier;
}

static void UpdateHotkeyDisplay()
{
	std::string display = BuildCurrentHotkeyDisplay();
	if (display.empty()) {
		lblText1->setCaption("Press a key combination...");
	} else {
		lblText1->setCaption(display);
	}
}

static void ShowMessageWaitInputLoop()
{
	AmigaMonitor* mon = &AMonitors[0];

	auto got_event = 0;
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			if (event.key.repeat == 0)
			{
				got_event = 1;

				// Handle escape key to cancel
				if (event.key.keysym.sym == VK_ESCAPE)
				{
					dialogFinished = true;
					break;
				}

				// Handle backspace to clear current hotkey
				if (event.key.keysym.sym == SDLK_BACKSPACE)
				{
					ResetHotkey();
					UpdateHotkeyDisplay();
					break;
				}

				bool is_modifier = HandleModifierKey(event.key.keysym.sym, true);

				if (!is_modifier)
				{
					// Regular key pressed - complete the hotkey
					hotkey.scancode = event.key.keysym.scancode;
					hotkey.key_name = SDL_GetKeyName(event.key.keysym.sym);
					// Build the complete hotkey string now
					hotkey.modifiers_string = BuildCurrentHotkeyDisplay();
					UpdateHotkeyDisplay();
					dialogFinished = true;
				}
				else
				{
					// Modifier key pressed - update display but don't finish
					modifier_only_pressed = true;
					UpdateHotkeyDisplay();
				}
			}
			break;

		case SDL_KEYUP:
			if (event.key.repeat == 0)
			{
				got_event = 1;

				// Check if this is a modifier key before clearing it
				bool is_modifier = false;
				switch (event.key.keysym.sym)
				{
				case SDLK_LCTRL:
				case SDLK_RCTRL:
				case SDLK_LALT:
				case SDLK_RALT:
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
				case SDLK_LGUI:
				case SDLK_RGUI:
					is_modifier = true;
					break;
				default:
					is_modifier = false;
					break;
				}

				if (is_modifier && modifier_only_pressed)
				{
					// Only modifier(s) were pressed and released - complete the hotkey
					// Build the string BEFORE clearing the modifiers
					hotkey.modifiers_string = BuildCurrentHotkeyDisplay();
					UpdateHotkeyDisplay();
					dialogFinished = true;
				}

				// Now clear the modifier flag
				HandleModifierKey(event.key.keysym.sym, false);

				if (!is_modifier)
				{
					modifier_only_pressed = false;
				}
			}
			break;

		case SDL_CONTROLLERBUTTONDOWN:
			got_event = 1;
			hotkey.button = event.cbutton.button;
			hotkey.key_name = SDL_GameControllerGetStringForButton(
				static_cast<SDL_GameControllerButton>(event.cbutton.button));
			UpdateHotkeyDisplay();
			dialogFinished = true;
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
		// Send event to guisan-controls
		//-------------------------------------------------
		gui_input->pushInput(event);
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();

		SDL_RenderClear(mon->gui_renderer);

		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally, we update the screen.
		update_gui_screen();
	}
}

static void navigate_left_right()
{
	const gcn::FocusHandler* focus_hdl = gui_top->_getFocusHandler();
	const gcn::Widget* active_widget = focus_hdl->getFocused();
	if (active_widget == cmdCancel)
		cmdOK->requestFocus();
	else if (active_widget == cmdOK)
		cmdCancel->requestFocus();
}

static void ShowMessageLoop()
{
	const AmigaMonitor* mon = &AMonitors[0];

	bool got_event = false;
	SDL_Event event;
	SDL_Event touch_event;
	bool nav_left, nav_right;
	while (SDL_PollEvent(&event))
	{
		nav_left = nav_right = false;
		switch (event.type)
		{
		case SDL_KEYDOWN:
			got_event = handle_keydown(event, dialogFinished, nav_left, nav_right);
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = handle_joybutton(&di_joystick[0], dialogFinished, nav_left, nav_right);
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = handle_joyaxis(event, nav_left, nav_right);
			}
			break;

		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			got_event = handle_finger(event, touch_event);
			gui_input->pushInput(touch_event);
			break;

		case SDL_MOUSEWHEEL:
			got_event = handle_mousewheel(event);
			break;

		default:
			got_event = true;
			break;
		}

		if (nav_left || nav_right)
			navigate_left_right();

		//-------------------------------------------------
		// Send event to guisan-controls
		//-------------------------------------------------
		gui_input->pushInput(event);
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		SDL_RenderClear(mon->gui_renderer);
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

bool ShowMessage(const std::string& title, const std::string& line1, const std::string& line2, const std::string& line3,
                 const std::string& button1, const std::string& button2)
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialogResult = false;
	dialogFinished = false;

	InitShowMessage(line1);

	wndShowMessage->setCaption(title);
	txtMessageText->setText(line1);
	lblText1->setCaption(line2);
	lblText2->setCaption(line3);
	cmdOK->setCaption(button1);
	cmdCancel->setCaption(button2);
	if (button2.empty())
	{
		cmdCancel->setVisible(false);
		cmdCancel->setEnabled(false);
		cmdOK->setPosition(cmdCancel->getX(), cmdCancel->getY());
	}
	cmdOK->setEnabled(true);

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowMessageLoop();
		cap_fps(start);
	}

	ExitShowMessage();
	return dialogResult;
}

amiberry_hotkey ShowMessageForInput(const char* title, const char* line1, const char* button1)
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialogResult = false;
	dialogFinished = false;

	InitShowMessage(line1);

	wndShowMessage->setCaption(title);
	txtMessageText->setText(line1);
	cmdOK->setVisible(false);
	cmdCancel->setCaption(button1);

	// Initialize hotkey and display
	ResetHotkey();
	lblText1->setCaption("Press a key combination...");
	lblText2->setCaption("(Backspace to clear, Escape to cancel)");

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowMessageWaitInputLoop();
		cap_fps(start);
	}

	ExitShowMessage();
	
	return hotkey;
}
