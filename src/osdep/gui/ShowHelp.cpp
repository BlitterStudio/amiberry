#include <algorithm>
#include <guisan.hpp>
#include <iostream>
#include <sstream>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
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

static SDL_Joystick *GUIjoy;
extern struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];


#define DIALOG_WIDTH 760
#define DIALOG_HEIGHT 420

static bool dialogFinished = false;

static gcn::Window *wndShowHelp;
static gcn::Button* cmdOK;
static gcn::ListBox* lstHelp;
static gcn::ScrollArea* scrAreaHelp;


class HelpListModel : public gcn::ListModel
{
	std::vector<std::string> lines;

public:
	HelpListModel(std::vector<std::string> helptext)
	{
		lines = helptext;
	}

	int getNumberOfElements() override
	{
		return lines.size();
	}

	std::string getElementAt(int i) override
	{
		if (i >= 0 && i < lines.size())
			return lines[i];
		return "";
	}
};
static HelpListModel *helpList;


class ShowHelpActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		dialogFinished = true;
	}
};
static ShowHelpActionListener* showHelpActionListener;


static void InitShowHelp(const std::vector<std::string>& helptext)
{
	wndShowHelp = new gcn::Window("Help");
	wndShowHelp->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowHelp->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowHelp->setBaseColor(gui_baseCol);
	wndShowHelp->setTitleBarHeight(TITLEBAR_HEIGHT);

	showHelpActionListener = new ShowHelpActionListener();

	helpList = new HelpListModel(helptext);

	lstHelp = new gcn::ListBox(helpList);
	lstHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	lstHelp->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	lstHelp->setBaseColor(gui_baseCol + 0x202020);
	lstHelp->setBackgroundColor(gui_baseCol);
	lstHelp->setWrappingEnabled(true);

	scrAreaHelp = new gcn::ScrollArea(lstHelp);
	scrAreaHelp->setBorderSize(1);
	scrAreaHelp->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	scrAreaHelp->setScrollbarWidth(20);
	scrAreaHelp->setBaseColor(gui_baseCol + 0x202020);
	scrAreaHelp->setBackgroundColor(gui_baseCol);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH, DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
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
//TODO: Check if this is needed in guisan?
	//FocusBugWorkaround(wndShowHelp);
	GUIjoy = SDL_JoystickOpen(0);

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

				case VK_Red:
				case VK_Green:
				case SDL_SCANCODE_RETURN:
					event.key.keysym.scancode = SDL_SCANCODE_RETURN;
					gui_input->pushInput(event); // Fire key down
					event.type = SDL_KEYUP;  // and the key up
					break;
				default:
					break;
				}
			}
else if (event.type == SDL_JOYBUTTONDOWN)
      {
            if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].south_button))
                {PushFakeKey(SDLK_RETURN);
                 break;}

            else if (SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].east_button) || 
                     SDL_JoystickGetButton(GUIjoy,host_input_buttons[0].start_button))
                {dialogFinished = true;
                     break;}    
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


void ShowHelp(const char *title, const std::vector<std::string>& text)
{
	dialogFinished = false;

	InitShowHelp(text);

	wndShowHelp->setCaption(title);
	cmdOK->setCaption("Ok");
	ShowHelpLoop();
	ExitShowHelp();
}
