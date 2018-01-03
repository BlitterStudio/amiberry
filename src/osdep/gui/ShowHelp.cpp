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

#include "options.h"
#include "inputdevice.h"

#ifdef ANDROIDSDL
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 760
#define DIALOG_HEIGHT 420

static bool dialogFinished = false;

static gcn::Window* wndShowHelp;
static gcn::Button* cmdOK;
static gcn::ListBox* lstHelp;
static gcn::ScrollArea* scrAreaHelp;


class HelpListModel : public gcn::ListModel
{
	vector<string> lines;

public:
	HelpListModel(const vector<string>& helptext)
	{
		lines = helptext;
	}

	int getNumberOfElements() override
	{
		return lines.size();
	}

	string getElementAt(const int i) override
	{
		if (i >= 0 && i < lines.size())
			return lines[i];
		return "";
	}
};

static HelpListModel* helpList;


class ShowHelpActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		dialogFinished = true;
	}
};

static ShowHelpActionListener* showHelpActionListener;


static void InitShowHelp(const vector<string>& helptext)
{
	wndShowHelp = new gcn::Window("Help");
	wndShowHelp->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowHelp->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowHelp->setBaseColor(gui_baseCol);
	wndShowHelp->setTitleBarHeight(TITLEBAR_HEIGHT);

	showHelpActionListener = new ShowHelpActionListener();

	helpList = new HelpListModel(helptext);

	lstHelp = new gcn::ListBox(helpList);
	lstHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4,
	                 DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	lstHelp->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	lstHelp->setBaseColor(gui_baseCol + 0x202020);
	lstHelp->setBackgroundColor(gui_baseCol);
	lstHelp->setWrappingEnabled(true);

	scrAreaHelp = new gcn::ScrollArea(lstHelp);
#ifdef USE_SDL1
	scrAreaHelp->setFrameSize(1);
#elif USE_SDL2
	scrAreaHelp->setBorderSize(1);
#endif
	scrAreaHelp->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4,
	                     DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	scrAreaHelp->setScrollbarWidth(20);
	scrAreaHelp->setBaseColor(gui_baseCol + 0x202020);
	scrAreaHelp->setBackgroundColor(gui_baseCol);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(showHelpActionListener);

	wndShowHelp->add(scrAreaHelp, DISTANCE_BORDER, DISTANCE_BORDER);
	wndShowHelp->add(cmdOK);

	gui_top->add(wndShowHelp);

	cmdOK->requestFocus();
	wndShowHelp->requestModalFocus();
}


static void ExitShowHelp(void)
{
	wndShowHelp->releaseModalFocus();
	gui_top->remove(wndShowHelp);

	delete lstHelp;
	delete scrAreaHelp;
	delete cmdOK;

	delete helpList;
	delete showHelpActionListener;

	delete wndShowHelp;
}


static void ShowHelpLoop(void)
{
	FocusBugWorkaround(wndShowHelp);

	while (!dialogFinished)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case VK_ESCAPE:
					dialogFinished = true;
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
			}
			else if (event.type == SDL_JOYBUTTONDOWN)
			{
				if (GUIjoy)
				{
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
				}
				break;
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


void ShowHelp(const char* title, const vector<string>& text)
{
	dialogFinished = false;

	InitShowHelp(text);

	wndShowHelp->setCaption(title);
	cmdOK->setCaption("Ok");
	ShowHelpLoop();
	ExitShowHelp();
}
