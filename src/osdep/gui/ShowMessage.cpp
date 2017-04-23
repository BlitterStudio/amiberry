#include <algorithm>
#include <guisan.hpp>
#include <iostream>
#include <sstream>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "gui.h"
#include "gui_handling.h"

#define DIALOG_WIDTH 340
#define DIALOG_HEIGHT 140

static bool dialogResult = false;
static bool dialogFinished = false;
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
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(showMessageActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
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

			if (event.type == SDL_CONTROLLERBUTTONDOWN)
			{
				dialogControlPressed = SDL_GameControllerGetStringForButton(SDL_GameControllerButton(event.cbutton.button));
				dialogFinished = true;
				break;
			}

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

static void ShowMessageLoop()
{
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

				case VK_LEFT:
				case VK_RIGHT:
					{
						gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
						gcn::Widget* activeWidget = focusHdl->getFocused();
						if (activeWidget == cmdCancel)
							cmdOK->requestFocus();
						else if (activeWidget == cmdOK)
							cmdCancel->requestFocus();
					}
					break;

				case VK_Red:
				case VK_Green:
				case SDL_SCANCODE_RETURN:
					event.key.keysym.scancode = SDL_SCANCODE_RETURN;
					gui_input->pushInput(event); // Fire key down
					event.type = SDL_KEYUP; // and the key up
					break;
				default: 
					break;
				}
			}

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