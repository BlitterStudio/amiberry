#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"

extern SDL_Surface* screen;

int msg_done = 0;
gcn::Gui* msg_gui;
gcn::SDLGraphics* msg_graphics;
gcn::SDLInput* msg_input;
gcn::SDLTrueTypeFont* msg_font;
SDL_Event msg_event;

gcn::Color msg_baseCol;
gcn::Container* msg_top;
gcn::Window* wndMsg;
gcn::Button* cmdDone;
gcn::TextBox* txtMsg;

int msgWidth = 260;
int msgHeight = 110;
int borderSize = 6;

class DoneActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdDone)
			msg_done = 1;
	}
};

DoneActionListener* doneActionListener;

void gui_halt()
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
}

void checkInput()
{
	//-------------------------------------------------
	// Check user input
	//-------------------------------------------------	
	while (SDL_PollEvent(&msg_event))
	{
		if (msg_event.type == SDL_KEYDOWN)
		{
			switch (msg_event.key.keysym.sym)
			{
			case VK_Red:
			case VK_Green:
			case SDLK_RETURN:
				msg_done = 1;
				break;
			default: 
				break;
			}
		}

		//-------------------------------------------------
		// Send event to guisan-controls
		//-------------------------------------------------
		msg_input->pushInput(msg_event);
	}
}

void gui_init(const char* msg)
{
	msg_graphics = new gcn::SDLGraphics();
	msg_graphics->setTarget(screen);
	msg_input = new gcn::SDLInput();
	msg_gui = new gcn::Gui();
	msg_gui->setGraphics(msg_graphics);
	msg_gui->setInput(msg_input);

	msg_baseCol.r = 160;
	msg_baseCol.g = 160;
	msg_baseCol.b = 160;

	msg_top = new gcn::Container();
	msg_top->setDimension(gcn::Rectangle((screen->w - msgWidth + borderSize * 4) / 2, (screen->h - msgHeight + borderSize * 4) / 4, msgWidth + (borderSize * 2), msgHeight + (borderSize * 2) + BUTTON_HEIGHT));
	msg_top->setBaseColor(msg_baseCol);
	msg_gui->setTop(msg_top);

	TTF_Init();
	msg_font = new gcn::SDLTrueTypeFont("data/Topaznew.ttf", 14);
	gcn::Widget::setGlobalFont(msg_font);

	doneActionListener = new DoneActionListener();

	wndMsg = new gcn::Window("Load");
	wndMsg->setSize(msgWidth + (borderSize * 2), msgHeight + (borderSize * 2) + BUTTON_HEIGHT);
	wndMsg->setPosition(0, 0);
	wndMsg->setBaseColor(msg_baseCol + 0x202020);
	wndMsg->setCaption("Information");
	wndMsg->setTitleBarHeight(12);

	cmdDone = new gcn::Button("Ok");
	cmdDone->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdDone->setBaseColor(msg_baseCol + 0x202020);
	cmdDone->setId("Done");
	cmdDone->addActionListener(doneActionListener);

	txtMsg = new gcn::TextBox(msg);
	txtMsg->setPosition(0, 0);
	txtMsg->setSize(msgWidth, msgHeight);

	wndMsg->add(txtMsg, borderSize, borderSize);
	wndMsg->add(cmdDone, (wndMsg->getWidth() - cmdDone->getWidth()) / 2, wndMsg->getHeight() - (borderSize * 2) - BUTTON_HEIGHT);

	msg_top->add(wndMsg);
	cmdDone->requestFocus();
}

void InGameMessage(const char* msg)
{
	gui_init(msg);

	msg_done = 0;
	bool drawn = false;
	while (!msg_done)
	{
		// Poll input
		checkInput();

		// Now we let the Gui object perform its logic.
		msg_gui->logic();
		// Now we let the Gui object draw itself.
		msg_gui->draw();
		// Finally we update the screen.
		if (!drawn)
		{
			SDL_ShowCursor(SDL_ENABLE);
			updatedisplayarea();
		}
		drawn = true;
	}

	gui_halt();
	SDL_ShowCursor(SDL_DISABLE);
}
