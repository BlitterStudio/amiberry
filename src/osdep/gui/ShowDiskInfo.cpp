#include <algorithm>
#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include <iostream>
#include <sstream>

#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"
#include "amiberry_input.h"

enum
{
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 460
};

static bool dialog_finished = false;

static gcn::Window* wndShowDiskInfo;
static gcn::Button* cmdOK;
static gcn::ListBox* lstInfo;
static gcn::ScrollArea* scrAreaInfo;

static gcn::StringListModel infoList;

class ShowDiskInfoActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		dialog_finished = true;
	}
};

static ShowDiskInfoActionListener* showDiskInfoActionListener;

static void InitShowDiskInfo(const std::vector<std::string>& infotext)
{
	wndShowDiskInfo = new gcn::Window("DiskInfo");
	wndShowDiskInfo->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowDiskInfo->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowDiskInfo->setBaseColor(gui_base_color);
	wndShowDiskInfo->setForegroundColor(gui_foreground_color);
	wndShowDiskInfo->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndShowDiskInfo->setMovable(false);

	showDiskInfoActionListener = new ShowDiskInfoActionListener();

	infoList.clear();
	for (const auto & i : infotext) {
		infoList.add(i);
	}

	lstInfo = new gcn::ListBox(&infoList);
	lstInfo->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4 - 20,
	                 DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	lstInfo->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	lstInfo->setBaseColor(gui_base_color);
	lstInfo->setBackgroundColor(gui_background_color);
	lstInfo->setForegroundColor(gui_foreground_color);
	lstInfo->setSelectionColor(gui_selection_color);
	lstInfo->setWrappingEnabled(true);

	scrAreaInfo = new gcn::ScrollArea(lstInfo);
	scrAreaInfo->setFrameSize(1);
	scrAreaInfo->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaInfo->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4,
	                     DIALOG_HEIGHT - 3 * DISTANCE_BORDER - BUTTON_HEIGHT - DISTANCE_NEXT_Y - 10);
	scrAreaInfo->setScrollbarWidth(SCROLLBAR_WIDTH);
	scrAreaInfo->setBaseColor(gui_base_color);
	scrAreaInfo->setBackgroundColor(gui_background_color);
	scrAreaInfo->setForegroundColor(gui_foreground_color);
	scrAreaInfo->setSelectionColor(gui_selection_color);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->addActionListener(showDiskInfoActionListener);

	wndShowDiskInfo->add(scrAreaInfo, DISTANCE_BORDER, DISTANCE_BORDER);
	wndShowDiskInfo->add(cmdOK);

	gui_top->add(wndShowDiskInfo);

	wndShowDiskInfo->requestModalFocus();
	focus_bug_workaround(wndShowDiskInfo);
	cmdOK->requestFocus();
}

static void ExitShowDiskInfo()
{
	wndShowDiskInfo->releaseModalFocus();
	gui_top->remove(wndShowDiskInfo);

	delete lstInfo;
	delete scrAreaInfo;
	delete cmdOK;

	delete showDiskInfoActionListener;
	delete wndShowDiskInfo;
}

static void ShowDiskInfoLoop()
{
	const AmigaMonitor* mon = &AMonitors[0];

	auto got_event = 0;
	SDL_Event event;
	SDL_Event touch_event;
	const didata* did = &di_joystick[0];

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			got_event = 1;
			switch (event.key.keysym.sym)
			{
			case VK_ESCAPE:
				dialog_finished = true;
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

		case SDL_JOYBUTTONDOWN:
			if (gui_joystick)
			{
				got_event = 1;
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
					dialog_finished = true;
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

			touch_event.button.x = gui_graphics->getTarget()->w * static_cast<int>(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * static_cast<int>(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;

			touch_event.button.x = gui_graphics->getTarget()->w * static_cast<int>(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * static_cast<int>(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;

			touch_event.motion.x = gui_graphics->getTarget()->w * static_cast<int>(event.tfinger.x);
			touch_event.motion.y = gui_graphics->getTarget()->h * static_cast<int>(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_KEYUP:
		case SDL_JOYBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
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

		SDL_RenderClear(mon->gui_renderer);

		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

void ShowDiskInfo(const char* title, const std::vector<std::string>& text)
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialog_finished = false;

	InitShowDiskInfo(text);

	wndShowDiskInfo->setCaption(title);
	cmdOK->setCaption("Ok");

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialog_finished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowDiskInfoLoop();
		cap_fps(start);
	}

	ExitShowDiskInfo();
}
