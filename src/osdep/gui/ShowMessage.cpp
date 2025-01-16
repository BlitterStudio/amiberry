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

static void ShowMessageWaitInputLoop()
{
	AmigaMonitor* mon = &AMonitors[0];

	auto got_event = 0;
	std::string caption;
	std::string delimiter;
	size_t pos;
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			if (event.key.repeat == 0)
			{
				got_event = 1;
				switch (event.key.keysym.sym)
				{
				case VK_ESCAPE:
					dialogFinished = true;
					break;

				case SDLK_LCTRL:
					hotkey.modifiers.lctrl = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				case SDLK_RCTRL:
					hotkey.modifiers.rctrl = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				case SDLK_LALT:
					hotkey.modifiers.lalt = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				case SDLK_RALT:
					hotkey.modifiers.ralt = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				case SDLK_LSHIFT:
					hotkey.modifiers.lshift = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				case SDLK_RSHIFT:
					hotkey.modifiers.rshift = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				case SDLK_LGUI:
					hotkey.modifiers.lgui = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				case SDLK_RGUI:
					hotkey.modifiers.rgui = true;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;

				default:
					hotkey.scancode = event.key.keysym.scancode;
					hotkey.key_name = delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					lblText1->setCaption(caption.append(" ").append(delimiter));
					break;
				}
			}
			break;

		case SDL_CONTROLLERBUTTONDOWN:
			//case SDL_JOYBUTTONDOWN:
			got_event = 1;
			hotkey.button = event.cbutton.button;
			hotkey.key_name = SDL_GameControllerGetStringForButton(
				static_cast<SDL_GameControllerButton>(event.cbutton.button));
			dialogFinished = true;
			break;

		case SDL_KEYUP:
			if (event.key.repeat == 0)
			{
				got_event = 1;
				switch (event.key.keysym.sym)
				{
				case SDLK_LCTRL:
					hotkey.modifiers.lctrl = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				case SDLK_RCTRL:
					hotkey.modifiers.rctrl = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				case SDLK_LALT:
					hotkey.modifiers.lalt = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				case SDLK_RALT:
					hotkey.modifiers.ralt = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				case SDLK_LSHIFT:
					hotkey.modifiers.lshift = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				case SDLK_RSHIFT:
					hotkey.modifiers.rshift = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				case SDLK_LGUI:
					hotkey.modifiers.lgui = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				case SDLK_RGUI:
					hotkey.modifiers.rgui = false;
					delimiter = SDL_GetKeyName(event.key.keysym.sym);
					caption = lblText1->getCaption();
					if ((pos = caption.find(delimiter)) != std::string::npos)
						caption.erase(0, pos + delimiter.length());
					lblText1->setCaption(caption);
					break;
				default:
					dialogFinished = true;
					break;
				}
			}
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
		// Finally we update the screen.
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

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	hotkey.modifiers.lctrl = hotkey.modifiers.rctrl =
		hotkey.modifiers.lalt = hotkey.modifiers.ralt =
		hotkey.modifiers.lshift = hotkey.modifiers.rshift =
		hotkey.modifiers.lgui = hotkey.modifiers.rgui = false;

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowMessageWaitInputLoop();
		cap_fps(start);
	}

	ExitShowMessage();

	hotkey.modifiers_string = "";
	if (hotkey.modifiers.lctrl)
		hotkey.modifiers_string = "LCtrl+";
	if (hotkey.modifiers.rctrl)
		hotkey.modifiers_string = "RCtrl+";
	if (hotkey.modifiers.lalt)
		hotkey.modifiers_string = hotkey.modifiers_string + "LAlt+";
	if (hotkey.modifiers.ralt)
		hotkey.modifiers_string = hotkey.modifiers_string + "RAlt+";
	if (hotkey.modifiers.lshift)
		hotkey.modifiers_string = hotkey.modifiers_string + "LShift+";
	if (hotkey.modifiers.rshift)
		hotkey.modifiers_string = hotkey.modifiers_string + "RShift+";
	if (hotkey.modifiers.lgui)
		hotkey.modifiers_string = hotkey.modifiers_string + "LGUI+";
	if (hotkey.modifiers.rgui)
		hotkey.modifiers_string = hotkey.modifiers_string + "RGUI+";

	return hotkey;
}
