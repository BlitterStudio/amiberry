#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"

#ifdef ANDROID
#include "androidsdl_event.h"
#endif

#include "amiberry_filesys.hpp"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"

#define DIALOG_WIDTH 600
#define DIALOG_HEIGHT 140

static bool dialogResult = false;
static bool dialogFinished = false;
static const char* dialogControlPressed;
static bool halt_gui = false;

static gcn::Window* wndShowMessage;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::Label* lblText1;
static gcn::Label* lblText2;

class ShowMessageActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
			dialogResult = true;
		dialogFinished = true;
	}
};

static ShowMessageActionListener* showMessageActionListener;

static void InitShowMessage()
{
	if (sdl_window == nullptr)
	{
		sdl_window = SDL_CreateWindow("Amiberry",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			0,
			0,
			SDL_WINDOW_FULLSCREEN_DESKTOP);
		check_error_sdl(sdl_window == nullptr, "Unable to create window:");
	}
	if (gui_screen == nullptr)
	{
		gui_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(gui_screen == nullptr, "Unable to create SDL surface:");
	}
	if (renderer == nullptr)
	{
		renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(renderer == nullptr, "Unable to create a renderer:");
		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);
	}

#ifdef USE_DISPMANX
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

		if (!blackscreen_element)
			blackscreen_element = vc_dispmanx_element_add(updateHandle, displayHandle, 0,
				&black_rect, black_gui_resource, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
				nullptr, DISPMANX_NO_ROTATE);

		if (!gui_element)
			gui_element = vc_dispmanx_element_add(updateHandle, displayHandle, 1,
				&dst_rect, gui_resource, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
				nullptr, DISPMANX_NO_ROTATE);

		vc_dispmanx_update_submit_sync(updateHandle);
}
#else

	if (sdl_window)
	{
		if (amiberry_options.rotation_angle != 0 && amiberry_options.rotation_angle != 180)
			SDL_SetWindowSize(sdl_window, GUI_HEIGHT, GUI_WIDTH);
		else
			SDL_SetWindowSize(sdl_window, GUI_WIDTH, GUI_HEIGHT);
	}
	
	// make the scaled rendering look smoother (linear scaling).
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (gui_texture == nullptr)
	{
		gui_texture = SDL_CreateTexture(renderer, gui_screen->format->format, SDL_TEXTUREACCESS_STREAMING,
			gui_screen->w, gui_screen->h);
		check_error_sdl(gui_texture == nullptr, "Unable to create texture from Surface");
	}
#endif

	if (amiberry_options.rotation_angle == 0 || amiberry_options.rotation_angle == 180)
		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);
	else
		SDL_RenderSetLogicalSize(renderer, GUI_HEIGHT, GUI_WIDTH);

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
		gui_baseCol = gcn::Color(170, 170, 170);
		gui_top->setBaseColor(gui_baseCol);
		uae_gui->setTop(gui_top);
	}
	if (gui_font == nullptr)
	{
		TTF_Init();
		gui_font = new gcn::SDLTrueTypeFont(prefix_with_application_directory_path("data/AmigaTopaz.ttf"), 15);
		gcn::Widget::setGlobalFont(gui_font);
		gui_font->setAntiAlias(false);
	}
	
	wndShowMessage = new gcn::Window("Message");
	wndShowMessage->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowMessage->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowMessage->setBaseColor(gui_baseCol);
	wndShowMessage->setTitleBarHeight(TITLEBAR_HEIGHT);

	showMessageActionListener = new ShowMessageActionListener();

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

	wndShowMessage->add(lblText1, DISTANCE_BORDER, DISTANCE_BORDER);
	wndShowMessage->add(lblText2, DISTANCE_BORDER, DISTANCE_BORDER + lblText1->getHeight() + 4);
	wndShowMessage->add(cmdOK);
	wndShowMessage->add(cmdCancel);

	gui_top->add(wndShowMessage);

	cmdOK->requestFocus();
	wndShowMessage->requestModalFocus();
}

static void ExitShowMessage()
{
	wndShowMessage->releaseModalFocus();
	gui_top->remove(wndShowMessage);

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

		if (cursor != nullptr)
		{
			SDL_FreeCursor(cursor);
			cursor = nullptr;
		}

		// Clear the screen
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
#endif
	}
}

static void ShowMessageWaitInputLoop()
{
	auto got_event = 0;
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_KEYDOWN)
		{
			got_event = 1;
			switch (event.key.keysym.sym)
			{
			case VK_ESCAPE:
				dialogFinished = true;
				break;

			default:
				dialogControlPressed = SDL_GetKeyName(event.key.keysym.sym);
				dialogFinished = true;
				break;
			}
		}

		if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_JOYBUTTONDOWN)
		{
			got_event = 1;
			dialogControlPressed = SDL_GameControllerGetStringForButton(
				SDL_GameControllerButton(event.cbutton.button));
			dialogFinished = true;
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
	auto got_event = 0;
	SDL_Event event;
	SDL_Event touch_event;
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
		case SDL_CONTROLLERBUTTONDOWN:
			if (gui_controller)
			{
				got_event = 1;
				if (SDL_GameControllerGetButton(gui_controller, SDL_CONTROLLER_BUTTON_A) ||
					SDL_GameControllerGetButton(gui_controller, SDL_CONTROLLER_BUTTON_B))
				{
					PushFakeKey(SDLK_RETURN);
					break;
				}
				if (SDL_GameControllerGetButton(gui_controller, SDL_CONTROLLER_BUTTON_X) ||
					SDL_GameControllerGetButton(gui_controller, SDL_CONTROLLER_BUTTON_Y) ||
					SDL_GameControllerGetButton(gui_controller, SDL_CONTROLLER_BUTTON_START))
				{
					dialogFinished = true;
					break;
				}
				if (SDL_GameControllerGetButton(gui_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
					SDL_GameControllerGetButton(gui_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
				{
					navigate_left_right();
					break;
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
			touch_event.button.x = gui_graphics->getTarget()->w * event.tfinger.x;
			touch_event.button.y = gui_graphics->getTarget()->h * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;
			touch_event.button.x = gui_graphics->getTarget()->w * event.tfinger.x;
			touch_event.button.y = gui_graphics->getTarget()->h * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;
			touch_event.motion.x = gui_graphics->getTarget()->w * event.tfinger.x;
			touch_event.motion.y = gui_graphics->getTarget()->h * event.tfinger.y;
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
#ifdef ANDROID
		androidsdl_event(event, gui_input);
#else
		gui_input->pushInput(event);
#endif
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

bool ShowMessage(const char* title, const char* line1, const char* line2, const char* button1, const char* button2)
{
	dialogResult = false;
	dialogFinished = false;

	InitShowMessage();

	wndShowMessage->setCaption(title);
	lblText1->setCaption(line1);
	lblText2->setCaption(line2);
	cmdOK->setCaption(button1);
	cmdCancel->setCaption(button2);
	if (strlen(button2) == 0)
	{
		cmdCancel->setVisible(false);
		cmdCancel->setEnabled(false);
		cmdOK->setPosition(cmdCancel->getX(), cmdCancel->getY());
	}
	cmdOK->setEnabled(true);

	// Prepare the screen once
	uae_gui->logic();
	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowMessageLoop();
		cap_fps(start, 60);
	}

	ExitShowMessage();
	return dialogResult;
}

const char* ShowMessageForInput(const char* title, const char* line1, const char* button1)
{
	dialogResult = false;
	dialogFinished = false;

	InitShowMessage();
	
	wndShowMessage->setCaption(title);
	lblText1->setCaption(line1);
	cmdOK->setVisible(false);
	cmdCancel->setCaption(button1);

	// Prepare the screen once
	uae_gui->logic();
	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowMessageWaitInputLoop();
		cap_fps(start, 60);
	}

	ExitShowMessage();
	return dialogControlPressed;
}
