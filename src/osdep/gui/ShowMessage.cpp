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

#include "options.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"

#define DIALOG_WIDTH 600
#define DIALOG_HEIGHT 140

static bool dialogResult = false;
static bool dialogFinished = false;
static const char* dialogControlPressed;

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
}

static void ShowMessageWaitInputLoop()
{
	FocusBugWorkaround(wndShowMessage);

	while (!dialogFinished)
	{
		auto time = SDL_GetTicks();
		int gotEvent = 0;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				gotEvent = 1;
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

			if (event.type == SDL_CONTROLLERBUTTONDOWN)
			{
				gotEvent = 1;
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
		if (gotEvent)
		{
			// Now we let the Gui object perform its logic.
			uae_gui->logic();
			// Now we let the Gui object draw itself.
			uae_gui->draw();
#ifdef USE_DISPMANX
			UpdateGuiScreen();
#else
			SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
#endif
			// Finally we update the screen.
			UpdateGuiScreen();
		}
		if (SDL_GetTicks() - time < 10) {
			SDL_Delay(10);
		}
	}
}

static void navigate_left_right()
{
	const auto focusHdl = gui_top->_getFocusHandler();
	const auto activeWidget = focusHdl->getFocused();
	if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
}

static void ShowMessageLoop()
{
	FocusBugWorkaround(wndShowMessage);

	int gotEvent = 0;
	while (!dialogFinished)
	{
		auto time = SDL_GetTicks();
		SDL_Event event;
		SDL_Event touch_event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				gotEvent = 1;
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
					gotEvent = 1;
					const int hat = SDL_JoystickGetHat(gui_joystick, 0);

					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].south_button))
					{
						PushFakeKey(SDLK_RETURN);
						break;
					}
					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button) ||
						SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].start_button))
					{
						dialogFinished = true;
						break;
					}
					if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_left) ||
						SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_right) ||
						(hat & SDL_HAT_LEFT) ||
						(hat & SDL_HAT_RIGHT))

					{
						navigate_left_right();
						break;
					}
				}
				break;

			case SDL_FINGERDOWN:
				gotEvent = 1;
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
				gotEvent = 1;
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
				gotEvent = 1;
				memcpy(&touch_event, &event, sizeof event);
				touch_event.type = SDL_MOUSEMOTION;
				touch_event.motion.which = 0;
				touch_event.motion.state = 0;
				touch_event.motion.x = gui_graphics->getTarget()->w * event.tfinger.x;
				touch_event.motion.y = gui_graphics->getTarget()->h * event.tfinger.y;
				gui_input->pushInput(touch_event);
				break;

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
			// Send event to guisan-controls
			//-------------------------------------------------
#ifdef ANDROID
			androidsdl_event(event, gui_input);
#else
			gui_input->pushInput(event);
#endif
		}
		if (gotEvent)
		{
			// Now we let the Gui object perform its logic.
			uae_gui->logic();
			// Now we let the Gui object draw itself.
			uae_gui->draw();
#ifdef USE_DISPMANX
#else
			SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
#endif
			// Finally we update the screen.
			UpdateGuiScreen();
		}
		if (SDL_GetTicks() - time < 10) {
			SDL_Delay(10);
		}
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
		cmdOK->setPosition(cmdCancel->getX(), cmdCancel->getY());
	}
	cmdOK->setEnabled(true);

	// Prepare the screen once
	uae_gui->logic();
	uae_gui->draw();
#ifdef USE_DISPMANX
#else
	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
#endif
	UpdateGuiScreen();

	ShowMessageLoop();

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
#ifdef USE_DISPMANX
#else
	SDL_UpdateTexture(gui_texture, nullptr, gui_screen->pixels, gui_screen->pitch);
#endif
	UpdateGuiScreen();

	ShowMessageWaitInputLoop();

	ExitShowMessage();

	return dialogControlPressed;
}
