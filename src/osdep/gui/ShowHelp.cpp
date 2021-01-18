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

#include "options.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"

#ifdef ANDROID
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
	std::vector<std::string> lines;

public:
	HelpListModel(const std::vector<std::string>& helptext)
	{
		lines = helptext;
	}

	int getNumberOfElements() override
	{
		return lines.size();
	}

	std::string getElementAt(const int i) override
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
	lstHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4,
	                 DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	lstHelp->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	lstHelp->setBaseColor(gui_baseCol);
	lstHelp->setBackgroundColor(gui_baseCol);
	lstHelp->setWrappingEnabled(true);

	scrAreaHelp = new gcn::ScrollArea(lstHelp);
	scrAreaHelp->setBorderSize(1);
	scrAreaHelp->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaHelp->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4,
	                     DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	scrAreaHelp->setScrollbarWidth(20);
	scrAreaHelp->setBaseColor(gui_baseCol);
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
		SDL_RenderClear(gui_renderer);
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

void ShowHelp(const char* title, const std::vector<std::string>& text)
{
	dialogFinished = false;

	InitShowHelp(text);

	wndShowHelp->setCaption(title);
	cmdOK->setCaption("Ok");

	// Prepare the screen once
	uae_gui->logic();
	SDL_RenderClear(gui_renderer);
	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowHelpLoop();
		cap_fps(start);
	}

	ExitShowHelp();
}
