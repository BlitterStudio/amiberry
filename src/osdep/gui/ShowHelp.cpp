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

#define DIALOG_WIDTH 760
#define DIALOG_HEIGHT 440

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
	auto got_event = 0;
	SDL_Event event;
	SDL_Event touch_event;
	didata* did = &di_joystick[0];
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
			break;

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
#ifdef USE_OPENGL
			touch_event.button.x = gui_graphics->getTargetPlaneWidth() * int(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTargetPlaneHeight() * int(event.tfinger.y);
#else
			touch_event.button.x = gui_graphics->getTarget()->w * int(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * int(event.tfinger.y);
#endif
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;
#ifdef USE_OPENGL
			touch_event.button.x = gui_graphics->getTargetPlaneWidth() * int(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTargetPlaneHeight() * int(event.tfinger.y);
#else
			touch_event.button.x = gui_graphics->getTarget()->w * int(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * int(event.tfinger.y);
#endif
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;
#ifdef USE_OPENGL
			touch_event.motion.x = gui_graphics->getTargetPlaneWidth() * int(event.tfinger.x);
			touch_event.motion.y = gui_graphics->getTargetPlaneHeight() * int(event.tfinger.y);
#else
			touch_event.motion.x = gui_graphics->getTarget()->w * int(event.tfinger.x);
			touch_event.motion.y = gui_graphics->getTarget()->h * int(event.tfinger.y);
#endif
			gui_input->pushInput(touch_event);
			break;

		case SDL_MOUSEWHEEL:
			got_event = 1;
			if (event.wheel.y > 0)
			{
				for (auto z = 0; z < event.wheel.y; ++z)
				{
					PushFakeKey(SDLK_UP);
				}
			}
			else if (event.wheel.y < 0)
			{
				for (auto z = 0; z > event.wheel.y; --z)
				{
					PushFakeKey(SDLK_DOWN);
				}
			}
			break;

		case SDL_KEYUP:
		case SDL_JOYBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
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
#ifndef USE_OPENGL
		SDL_RenderClear(sdl_renderer);
#endif
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

void ShowHelp(const char* title, const std::vector<std::string>& text)
{
	dialog_finished = false;

	InitShowHelp(text);

	wndShowHelp->setCaption(title);
	cmdOK->setCaption("Ok");

	// Prepare the screen once
	uae_gui->logic();
#ifndef USE_OPENGL
	SDL_RenderClear(sdl_renderer);
#endif
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
