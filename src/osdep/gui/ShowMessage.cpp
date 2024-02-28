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

#define DIALOG_WIDTH 600
#define DIALOG_HEIGHT 200

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
#ifdef USE_DISPMANX
	if (!displayHandle)
		init_dispmanx_gui();

	if (!mon->sdl_window)
	{
		mon->sdl_window = SDL_CreateWindow("Amiberry",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			GUI_WIDTH,
			GUI_HEIGHT,
			SDL_WINDOW_FULLSCREEN_DESKTOP);
		check_error_sdl(mon->sdl_window == nullptr, "Unable to create window:");
	}
	if (mon->sdl_renderer == nullptr)
	{
		mon->sdl_renderer = SDL_CreateRenderer(mon->sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(mon->sdl_renderer == nullptr, "Unable to create a renderer:");
		SDL_RenderSetLogicalSize(mon->sdl_renderer, GUI_WIDTH, GUI_HEIGHT);
	}
#else
	if (!mon->sdl_window)
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
			mon->sdl_window = SDL_CreateWindow("Amiberry GUI",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_HEIGHT,
				GUI_WIDTH,
				mode);
		}
		else
		{
			mon->sdl_window = SDL_CreateWindow("Amiberry GUI",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				GUI_WIDTH,
				GUI_HEIGHT,
				mode);
		}
		check_error_sdl(mon->sdl_window == nullptr, "Unable to create window:");

		auto* const icon_surface = IMG_Load(prefix_with_data_path("amiberry.png").c_str());
		if (icon_surface != nullptr)
		{
			SDL_SetWindowIcon(mon->sdl_window, icon_surface);
			SDL_FreeSurface(icon_surface);
		}
	}

	if (mon->sdl_renderer == nullptr)
	{
		mon->sdl_renderer = SDL_CreateRenderer(mon->sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(mon->sdl_renderer == nullptr, "Unable to create a renderer:");
		SDL_RenderSetLogicalSize(mon->sdl_renderer, GUI_WIDTH, GUI_HEIGHT);
	}

	if (mon->sdl_window)
	{
		const auto window_flags = SDL_GetWindowFlags(mon->sdl_window);
		const bool is_maximized = window_flags & SDL_WINDOW_MAXIMIZED;
		const bool is_fullscreen = window_flags & SDL_WINDOW_FULLSCREEN;
		if (!is_maximized && !is_fullscreen)
		{
			if (amiberry_options.rotation_angle != 0 && amiberry_options.rotation_angle != 180)
				SDL_SetWindowSize(mon->sdl_window, GUI_HEIGHT, GUI_WIDTH);
			else
				SDL_SetWindowSize(mon->sdl_window, GUI_WIDTH, GUI_HEIGHT);
		}
	}

	// make the scaled rendering look smoother (linear scaling).
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (gui_texture == nullptr)
	{
		gui_texture = SDL_CreateTexture(mon->sdl_renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING,
		                                gui_screen->w, gui_screen->h);
		check_error_sdl(gui_texture == nullptr, "Unable to create texture from Surface");
	}

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(mon->sdl_renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(mon->sdl_renderer, GUI_HEIGHT, GUI_WIDTH);
#endif

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
		gui_baseCol = gui_theme.base_color;
		gui_top->setBaseColor(gui_baseCol);
		uae_gui->setTop(gui_top);
	}
	if (gui_font == nullptr)
	{
		TTF_Init();

		try
		{
			gui_font = new gcn::SDLTrueTypeFont(prefix_with_data_path(gui_theme.font_name), gui_theme.font_size);
			gui_font->setAntiAlias(false);
		}
		catch (exception& ex)
		{
			cout << ex.what() << '\n';
			write_log("An error occurred while trying to open the GUI font! Exception: %s\n", ex.what());
			abort();
		}

		gcn::Widget::setGlobalFont(gui_font);
	}

	wndShowMessage = new gcn::Window("Message");
	wndShowMessage->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowMessage->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowMessage->setBaseColor(gui_baseCol);
	wndShowMessage->setTitleBarHeight(TITLEBAR_HEIGHT);

	showMessageActionListener = new ShowMessageActionListener();

	txtMessageText = new gcn::TextBox(message);
	txtMessageText->setBackgroundColor(gui_baseCol);
	txtMessageText->setEditable(false);
	lblText1 = new gcn::Label("");
	lblText1->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);
	lblText2 = new gcn::Label("");
	lblText2->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(showMessageActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                       DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
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
	AmigaMonitor* mon = &AMonitors[0];

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
#ifdef USE_DISPMANX
		if (element_present == 1)
		{
			element_present = 0;
			updateHandle = vc_dispmanx_update_start(0);
			vc_dispmanx_element_remove(updateHandle, gui_element);
			gui_element = 0;
			vc_dispmanx_element_remove(updateHandle, blackscreen_element);
			blackscreen_element = 0;
			vc_dispmanx_update_submit_sync(updateHandle);
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
		if (displayHandle)
			vc_dispmanx_display_close(displayHandle);
#else
		if (gui_texture != nullptr)
		{
			SDL_DestroyTexture(gui_texture);
			gui_texture = nullptr;
		}

		// Clear the screen
		SDL_RenderClear(mon->sdl_renderer);
		SDL_RenderPresent(mon->sdl_renderer);
#endif
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

		SDL_RenderClear(mon->sdl_renderer);

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
	AmigaMonitor* mon = &AMonitors[0];

	auto got_event = 0;
	SDL_Event event;
	SDL_Event touch_event;
	didata* did = &di_joystick[0];
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			got_event = 1;
			switch (event.key.keysym.sym)
			{
			case VK_ESCAPE:
				dialogFinished = true;
				break;

			case VK_LEFT:
			case VK_RIGHT:
				navigate_left_right();
				break;

			case VK_Blue:
			case VK_Green:
			case SDLK_RETURN:
				event.key.keysym.sym = SDLK_RETURN;
				gui_input->pushInput(event); // Fire key down
				event.type = SDL_KEYUP; // and the key up
				break;
			default:
				break;
			}
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				const int hat = SDL_JoystickGetHat(gui_joystick, 0);

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_A]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_B]))
				{
					PushFakeKey(SDLK_RETURN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_X]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_Y]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_START]))
				{
					dialogFinished = true;
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) ||
					hat & SDL_HAT_LEFT ||
					hat & SDL_HAT_RIGHT)
				{
					navigate_left_right();
					break;
				}
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				if (event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
				{
					if (event.jaxis.value > joystick_dead_zone && last_x != 1)
					{
						last_x = 1;
						navigate_left_right();
						break;
					}
					if (event.jaxis.value<-joystick_dead_zone && last_x != -1)
					                      {
						                      last_x = -1;
						                      navigate_left_right();
						break;
					
					                      }
					if (event.jaxis.value > -joystick_dead_zone && event.jaxis.value < joystick_dead_zone)
					                      last_x = 0;
				
				}
			}
			break;

		case SDL_FINGERDOWN:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONDOWN;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_PRESSED;

			touch_event.button.x = gui_graphics->getTarget()->w * int(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * int(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;

			touch_event.button.x = gui_graphics->getTarget()->w * int(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * int(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;

			touch_event.motion.x = gui_graphics->getTarget()->w * int(event.tfinger.x);
			touch_event.motion.y = gui_graphics->getTarget()->h * int(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_KEYUP:
		case SDL_JOYBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
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

		SDL_RenderClear(mon->sdl_renderer);

		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

bool ShowMessage(const std::string& title, const std::string& line1, const std::string& line2, const std::string& line3,
                 const std::string& button1, const std::string& button2)
{
	AmigaMonitor* mon = &AMonitors[0];

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

	SDL_RenderClear(mon->sdl_renderer);

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
	AmigaMonitor* mon = &AMonitors[0];

	dialogResult = false;
	dialogFinished = false;

	InitShowMessage(line1);

	wndShowMessage->setCaption(title);
	txtMessageText->setText(line1);
	cmdOK->setVisible(false);
	cmdCancel->setCaption(button1);

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->sdl_renderer);

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
