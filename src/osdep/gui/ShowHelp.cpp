#include <cstdio>
#include <cstring>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"

#include "options.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"

enum
{
	DIALOG_WIDTH = 760,
	DIALOG_HEIGHT = 440
};

static bool dialog_finished = false;

static gcn::Window* wndShowHelp;
static gcn::Button* cmdOK;
static gcn::ListBox* lstHelp;
static gcn::ScrollArea* scrAreaHelp;

static gcn::StringListModel helpList;

class ShowHelpActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		dialog_finished = true;
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

	helpList.clear();
	for (const auto & i : helptext) {
		helpList.add(i);
	}

	lstHelp = new gcn::ListBox(&helpList);
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
	scrAreaHelp->setScrollbarWidth(SCROLLBAR_WIDTH);
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

	wndShowHelp->requestModalFocus();
	focus_bug_workaround(wndShowHelp);
	cmdOK->requestFocus();
}

static void ExitShowHelp()
{
	wndShowHelp->releaseModalFocus();
	gui_top->remove(wndShowHelp);

	delete lstHelp;
	delete scrAreaHelp;
	delete cmdOK;

	delete showHelpActionListener;
	delete wndShowHelp;
}

static void ShowHelpLoop()
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
			got_event = handle_keydown(event, dialog_finished, nav_left, nav_right);
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = handle_joybutton(&di_joystick[0], dialog_finished, nav_left, nav_right);
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

void ShowHelp(const char* title, const std::vector<std::string>& text)
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialog_finished = false;

	InitShowHelp(text);

	wndShowHelp->setCaption(title);
	cmdOK->setCaption("Ok");

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->sdl_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialog_finished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowHelpLoop();
		cap_fps(start);
	}

	ExitShowHelp();
}
