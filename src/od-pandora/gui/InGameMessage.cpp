#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "gui.h"
#include "gui_handling.h"
#include "pandora_gfx.h"

//extern SDL_Surface *amigaSurface;
extern void flush_screen();

static int msg_done = 0;
class DoneActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent)
	{
		msg_done = 1;
	}
};
static DoneActionListener* doneActionListener;


void InGameMessage(const char *msg)
{
	gcn::Gui* msg_gui;
	gcn::SDLGraphics* msg_graphics;
	gcn::SDLInput* msg_input;
	gcn::SDLTrueTypeFont* msg_font;
	gcn::Color msg_baseCol;

	gcn::Container* msg_top;
	gcn::Window *wndMsg;
	gcn::Button* cmdDone;
	gcn::TextBox* txtMsg;

	int msgWidth = 260;
	int msgHeight = 100;

	msg_graphics = new gcn::SDLGraphics();
	msg_graphics->setTarget(amigaSurface);
	msg_input = new gcn::SDLInput();
	msg_gui = new gcn::Gui();
	msg_gui->setGraphics(msg_graphics);
	msg_gui->setInput(msg_input);

	msg_baseCol.r = 192;
	msg_baseCol.g = 192;
	msg_baseCol.b = 208;

	msg_top = new gcn::Container();
	msg_top->setDimension(gcn::Rectangle((amigaSurface->w - msgWidth) / 2, (amigaSurface->h - msgHeight) / 2, msgWidth, msgHeight));
	msg_top->setBaseColor(msg_baseCol);
	msg_gui->setTop(msg_top);

	TTF_Init();
	msg_font = new gcn::SDLTrueTypeFont("data/FreeSans.ttf", 10);
	gcn::Widget::setGlobalFont(msg_font);

	doneActionListener = new DoneActionListener();

	wndMsg = new gcn::Window("Load");
	wndMsg->setSize(msgWidth, msgHeight);
	wndMsg->setPosition(0, 0);
	wndMsg->setBaseColor(msg_baseCol + 0x202020);
	wndMsg->setCaption("Information");
	wndMsg->setTitleBarHeight(12);

	cmdDone = new gcn::Button("Ok");
	cmdDone->setSize(40, 20);
	cmdDone->setBaseColor(msg_baseCol + 0x202020);
	cmdDone->addActionListener(doneActionListener);

	txtMsg = new gcn::TextBox(msg);
	txtMsg->setEnabled(false);
	txtMsg->setPosition(6, 6);
	txtMsg->setSize(wndMsg->getWidth() - 16, 46);
	txtMsg->setOpaque(false);

	wndMsg->add(txtMsg, 6, 6);
	wndMsg->add(cmdDone, (wndMsg->getWidth() - cmdDone->getWidth()) / 2, wndMsg->getHeight() - 38);

	msg_top->add(wndMsg);
	cmdDone->requestFocus();

	msg_done = 0;
	bool drawn = false;
	while (!msg_done)
	{
	    //-------------------------------------------------
	    // Check user input
	    //-------------------------------------------------
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_RETURN:
					msg_done = 1;
					break;
				}
			}

			//-------------------------------------------------
			// Send event to guichan-controls
			//-------------------------------------------------
			msg_input->pushInput(event);
		}

		// Now we let the Gui object perform its logic.
		msg_gui->logic();
		// Now we let the Gui object draw itself.
		msg_gui->draw();
		// Finally we update the screen.
		if (!drawn)
		{
			// Update the texture from the surface
			SDL_UpdateTexture(texture, NULL, gui_screen->pixels, gui_screen->pitch);
			// Copy the texture on the renderer
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			// Update the window surface (show the renderer)
			SDL_RenderPresent(renderer);
		}			
		drawn = true;
	}

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
