#include <stdbool.h>
#include <stdio.h>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include <guisan/gui.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "inputdevice.h"

#ifdef ANDROID
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 600
#define DIALOG_HEIGHT 200

SDL_Surface* msg_screen;
#ifdef USE_DISPMANX
DISPMANX_RESOURCE_HANDLE_T message_resource;
DISPMANX_RESOURCE_HANDLE_T black_bg_resource;
DISPMANX_ELEMENT_HANDLE_T message_element;
int message_element_present = 0;
#else
SDL_Texture* msg_texture;
#endif
bool msg_done = false;
gcn::Gui* msg_gui;
gcn::SDLGraphics* msg_graphics;
gcn::SDLInput* msg_input;
gcn::SDLTrueTypeFont* msg_font;

gcn::Color msg_baseCol;
gcn::Container* msg_top;
gcn::Window* wndMsg;
gcn::Button* cmdDone;
gcn::Label* txtMsg;

class DoneActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdDone)
			msg_done = true;
	}
};

static DoneActionListener* doneActionListener;

void message_gui_halt()
{
	msg_top->remove(wndMsg);

	delete txtMsg;
	delete cmdDone;
	delete doneActionListener;
	delete wndMsg;

	delete msg_font;
	delete msg_top;

	delete msg_gui;
	delete msg_input;
	delete msg_graphics;

	if (msg_screen != nullptr)
	{
		SDL_FreeSurface(msg_screen);
		msg_screen = nullptr;
	}
#ifdef USE_DISPMANX
	if (message_element_present == 1)
	{
		message_element_present = 0;
		updateHandle = vc_dispmanx_update_start(0);
		vc_dispmanx_element_remove(updateHandle, message_element);
		message_element = 0;
		vc_dispmanx_element_remove(updateHandle, blackscreen_element);
		blackscreen_element = 0;
		vc_dispmanx_update_submit_sync(updateHandle);
	}

	if (message_resource)
	{
		vc_dispmanx_resource_delete(message_resource);
		message_resource = 0;
	}

	if (black_bg_resource)
	{
		vc_dispmanx_resource_delete(black_bg_resource);
		black_bg_resource = 0;
	}
	if (displayHandle)
		vc_dispmanx_display_close(displayHandle);
#else
	if (msg_texture != nullptr)
	{
		SDL_DestroyTexture(msg_texture);
		msg_texture = nullptr;
	}
	// Clear the screen
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
#endif
}

void message_UpdateScreen()
{
#ifdef USE_DISPMANX
	vc_dispmanx_resource_write_data(message_resource, rgb_mode, gui_screen->pitch, gui_screen->pixels, &blit_rect);
	updateHandle = vc_dispmanx_update_start(0);
	vc_dispmanx_element_change_source(updateHandle, message_element, message_resource);
	vc_dispmanx_update_submit_sync(updateHandle);
#else
	SDL_UpdateTexture(msg_texture, nullptr, msg_screen->pixels, msg_screen->pitch);
	SDL_RenderCopyEx(renderer, msg_texture, nullptr, nullptr, amiberry_options.rotation_angle, nullptr, SDL_FLIP_NONE);
	SDL_RenderPresent(renderer);
#endif
}

void message_checkInput()
{
	//-------------------------------------------------
	// Check user input
	//-------------------------------------------------
	//
	const auto start = SDL_GetPerformanceCounter();
	int gotEvent = 0;
	SDL_Event msg_event;
	SDL_Event touch_event;
	while (SDL_PollEvent(&msg_event))
	{
		switch(msg_event.type)
		{
		case SDL_KEYDOWN:
			gotEvent = 1;
			switch (msg_event.key.keysym.sym)
			{
			case VK_ESCAPE:
				msg_done = true;
				break;
				
			case VK_Blue:
			case VK_Green:
			case SDLK_RETURN:
				msg_event.key.keysym.sym = SDLK_RETURN;
				msg_input->pushInput(msg_event);
				msg_event.type = SDL_KEYUP;
				//msg_done = true;
				break;
			default:
				break;
			}
			break;

		case SDL_JOYBUTTONDOWN:
			if (gui_joystick)
			{
				gotEvent = 1;
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button) ||
					SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].start_button) ||
					SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button))

					msg_done = true;
			}
			break;

		case SDL_FINGERDOWN:
			gotEvent = 1;
			memcpy(&touch_event, &msg_event, sizeof msg_event);
			touch_event.type = SDL_MOUSEBUTTONDOWN;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_PRESSED;
			touch_event.button.x = msg_graphics->getTarget()->w * msg_event.tfinger.x;
			touch_event.button.y = msg_graphics->getTarget()->h * msg_event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			gotEvent = 1;
			memcpy(&touch_event, &msg_event, sizeof msg_event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;
			touch_event.button.x = msg_graphics->getTarget()->w * msg_event.tfinger.x;
			touch_event.button.y = msg_graphics->getTarget()->h * msg_event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			gotEvent = 1;
			memcpy(&touch_event, &msg_event, sizeof msg_event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;
			touch_event.motion.x = msg_graphics->getTarget()->w * msg_event.tfinger.x;
			touch_event.motion.y = msg_graphics->getTarget()->h * msg_event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_KEYUP:
		case SDL_JOYBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
			gotEvent = 1;
			break;
			
		default:
			break;
		}

		//-------------------------------------------------
		// Send event to gui-controls
		//-------------------------------------------------
#ifdef ANDROID
		androidsdl_event(msg_event, msg_input);
#else
		msg_input->pushInput(msg_event);
#endif

		cap_fps(start, 60);
	}
	
	if (gotEvent)
	{
		// Now we let the Gui object perform its logic.
		msg_gui->logic();
		// Now we let the Gui object draw itself.
		msg_gui->draw();
		message_UpdateScreen();
	}
	cap_fps(start, 60);
}

void message_gui_init(const char* msg)
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
	if (msg_screen == nullptr)
	{
		msg_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(msg_screen == nullptr, "Unable to create SDL surface:");
	}
	if (renderer == nullptr)
	{
		renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(renderer == nullptr, "Unable to create a renderer:");
		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);
	}
	SDL_ShowCursor(SDL_ENABLE);
#ifdef USE_DISPMANX
	displayHandle = vc_dispmanx_display_open(0);
	uint32_t vc_gui_image_ptr;

	if (!message_resource)
		message_resource = vc_dispmanx_resource_create(rgb_mode, GUI_WIDTH, GUI_HEIGHT, &vc_gui_image_ptr);
	if (!black_bg_resource)
		black_bg_resource = vc_dispmanx_resource_create(rgb_mode, GUI_WIDTH, GUI_HEIGHT, &vc_gui_image_ptr);
	vc_dispmanx_rect_set(&blit_rect, 0, 0, GUI_WIDTH, GUI_HEIGHT);
	vc_dispmanx_resource_write_data(message_resource, rgb_mode, gui_screen->pitch, gui_screen->pixels, &blit_rect);
	vc_dispmanx_resource_write_data(black_bg_resource, rgb_mode, gui_screen->pitch, gui_screen->pixels, &blit_rect);
	vc_dispmanx_rect_set(&src_rect, 0, 0, GUI_WIDTH << 16, GUI_HEIGHT << 16);
	vc_dispmanx_rect_set(&black_rect, 0, 0, modeInfo.width, modeInfo.height);

	// Scaled display with correct Aspect Ratio
	const auto want_aspect = float(GUI_WIDTH) / float(GUI_HEIGHT);
	const auto real_aspect = float(modeInfo.width) / float(modeInfo.height);

	SDL_Rect viewport;
	if (want_aspect > real_aspect)
	{
		const auto scale = float(modeInfo.width) / float(GUI_WIDTH);
		viewport.x = 0;
		viewport.w = modeInfo.width;
		viewport.h = int(std::ceil(GUI_HEIGHT * scale));
		viewport.y = (modeInfo.height - viewport.h) / 2;
	}
	else
	{
		const auto scale = float(modeInfo.height) / float(GUI_HEIGHT);
		viewport.y = 0;
		viewport.h = modeInfo.height;
		viewport.w = int(std::ceil(GUI_WIDTH * scale));
		viewport.x = (modeInfo.width - viewport.w) / 2;
	}
	vc_dispmanx_rect_set(&dst_rect, viewport.x, viewport.y, viewport.w, viewport.h);

	if (!message_element_present)
	{
		message_element_present = 1;
		updateHandle = vc_dispmanx_update_start(0);

		VC_DISPMANX_ALPHA_T alpha;
		alpha.flags = DISPMANX_FLAGS_ALPHA_FROM_SOURCE;
		alpha.opacity = 255;
		alpha.mask = 0;

		blackscreen_element = vc_dispmanx_element_add(updateHandle, displayHandle, 0,
			&black_rect, black_bg_resource, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
			nullptr, DISPMANX_NO_ROTATE);

		message_element = vc_dispmanx_element_add(updateHandle, displayHandle, 1,
			&dst_rect, message_resource, &src_rect, DISPMANX_PROTECTION_NONE, &alpha,
			nullptr,             // clamp
			DISPMANX_NO_ROTATE);

		vc_dispmanx_update_submit_sync(updateHandle);
	}
#else
	// make the scaled rendering look smoother (linear scaling).
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (msg_texture == nullptr)
	{
		msg_texture = SDL_CreateTexture(renderer, msg_screen->format->format, SDL_TEXTUREACCESS_STREAMING,
		                                msg_screen->w, msg_screen->h);
		check_error_sdl(msg_texture == nullptr, "Unable to create texture from Surface");
	}
#endif

	msg_graphics = new gcn::SDLGraphics();
	msg_graphics->setTarget(msg_screen);
	msg_input = new gcn::SDLInput();
	msg_gui = new gcn::Gui();
	msg_gui->setGraphics(msg_graphics);
	msg_gui->setInput(msg_input);
}

void message_widgets_init(const char* msg)
{
	msg_baseCol = gcn::Color(170, 170, 170);

	msg_top = new gcn::Container();
	msg_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
	msg_top->setBaseColor(msg_baseCol);
	msg_gui->setTop(msg_top);

	TTF_Init();
	msg_font = new gcn::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
	gcn::Widget::setGlobalFont(msg_font);

	wndMsg = new gcn::Window("InGameMessage");
	wndMsg->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndMsg->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndMsg->setBaseColor(msg_baseCol);
	wndMsg->setCaption("Information");
	wndMsg->setTitleBarHeight(TITLEBAR_HEIGHT);

	doneActionListener = new DoneActionListener();

	cmdDone = new gcn::Button("Ok");
	cmdDone->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdDone->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
	                     DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdDone->setBaseColor(msg_baseCol);
	cmdDone->setId("Done");
	cmdDone->addActionListener(doneActionListener);

	txtMsg = new gcn::Label(msg);
	txtMsg->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);

	wndMsg->add(txtMsg, DISTANCE_BORDER, DISTANCE_BORDER);
	wndMsg->add(cmdDone);

	msg_top->add(wndMsg);
	cmdDone->requestFocus();
	wndMsg->requestModalFocus();
}

void message_gui_run()
{
	if (SDL_NumJoysticks() > 0)
	{
		gui_joystick = SDL_JoystickOpen(0);
	}

	// Prepare the screen once
	msg_gui->logic();
	msg_gui->draw();
	message_UpdateScreen();

	while (!msg_done)
	{
		// Poll input
		message_checkInput();
	}

	if (gui_joystick)
	{
		SDL_JoystickClose(gui_joystick);
		gui_joystick = nullptr;
	}
}

void InGameMessage(const char* msg)
{
	message_gui_init(msg);
	message_widgets_init(msg);

	message_gui_run();

	message_gui_halt();
	SDL_ShowCursor(SDL_DISABLE);
}
