#include <algorithm>
#include <iostream>
#include <sstream>
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
#include "gui.h"
#include "gui_handling.h"

#ifdef ANDROIDSDL
#include "androidsdl_event.h"
#endif

#include "options.h"
#include "inputdevice.h"

#define DIALOG_WIDTH 340
#define DIALOG_HEIGHT 140

static bool dialogResult = false;
static bool dialogFinished = false;
static SDL_Joystick* GUIjoy;
extern struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];
static const char* dialogControlPressed;
static Uint8 dialogButtonPressed;

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

#ifdef ANDROIDSDL
#include "androidsdl_event.h"
#endif

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
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.scancode)
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
// This only works in SDL2 for now
#ifdef USE_SDL2
			if (event.type == SDL_CONTROLLERBUTTONDOWN)
			{
				dialogControlPressed = SDL_GameControllerGetStringForButton(SDL_GameControllerButton(event.cbutton.button));
				dialogFinished = true;
				break;
			}
#endif
			//-------------------------------------------------
			// Send event to guisan-controls
			//-------------------------------------------------
			gui_input->pushInput(event);
		}

		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.

		UpdateGuiScreen();
	}
}

static void navigate_left_right()
{
	gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
	gcn::Widget* activeWidget = focusHdl->getFocused();
	if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
}

static void ShowMessageLoop()
{
	FocusBugWorkaround(wndShowMessage);

	GUIjoy = SDL_JoystickOpen(0);

	while (!dialogFinished)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
#ifdef USE_SDL1
				switch (event.key.keysym.sym)
#elif USE_SDL2
				switch (event.key.keysym.scancode)
#endif
				{
				case VK_ESCAPE:
					dialogFinished = true;
					break;

				case VK_LEFT:
				case VK_RIGHT:
					navigate_left_right();
					break;

				case VK_Red:
				case VK_Green:
#ifdef USE_SDL1
				case SDLK_RETURN:
					event.key.keysym.sym = SDLK_RETURN;
#elif USE_SDL2
				case SDL_SCANCODE_RETURN:
					event.key.keysym.scancode = SDL_SCANCODE_RETURN;
#endif
					gui_input->pushInput(event); // Fire key down
					event.type = SDL_KEYUP; // and the key up
					break;
				default:
					break;
				}
			}
			else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYHATMOTION)
			{
				const int hat = SDL_JoystickGetHat(GUIjoy, 0);

				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].south_button))
				{
					PushFakeKey(SDLK_RETURN);
					break;
				}
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].east_button) ||
					SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].start_button))
				{
					dialogFinished = true;
					break;
				}
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_left) ||
					SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_right) ||
					(hat & SDL_HAT_LEFT) ||
					(hat & SDL_HAT_RIGHT))

				{
					navigate_left_right();
					break;
				}
			}

			//-------------------------------------------------
			// Send event to guisan-controls
			//-------------------------------------------------
#ifdef ANDROIDSDL
			androidsdl_event(event, gui_input);
#else
			gui_input->pushInput(event);
#endif
		}

		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.

		UpdateGuiScreen();
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

	ShowMessageWaitInputLoop();
	ExitShowMessage();

	return dialogControlPressed;
}
