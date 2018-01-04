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
#include <guisan/gui.hpp>
#endif
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "gui.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"

#include "inputdevice.h"


#ifdef ANDROIDSDL
#include "androidsdl_event.h"
#endif

int msg_done = 0;
gcn::Gui* msg_gui;
gcn::SDLGraphics* msg_graphics;
gcn::SDLInput* msg_input;
#ifdef USE_SDL1
gcn::contrib::SDLTrueTypeFont* msg_font;
#elif USE_SDL2
gcn::SDLTrueTypeFont* msg_font;
#endif
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

static DoneActionListener* doneActionListener;

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
	if (SDL_NumJoysticks() > 0)
		if (GUIjoy == nullptr)
			GUIjoy = SDL_JoystickOpen(0);

	while (SDL_PollEvent(&msg_event))
	{
		if (msg_event.type == SDL_KEYDOWN)
		{
			switch (msg_event.key.keysym.sym)
			{
			case VK_Blue:
			case VK_Green:
			case SDLK_RETURN:
				msg_done = 1;
				break;
			default:
				break;
			}
		}
		else if (msg_event.type == SDL_JOYBUTTONDOWN)
		{
			if (GUIjoy)
			{
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].east_button) ||
					SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].start_button) ||
					SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].east_button))

					msg_done = 1;
			}
			break;
		}

		//-------------------------------------------------
		// Send event to gui-controls
		//-------------------------------------------------
#ifdef ANDROIDSDL
		androidsdl_event(event, msg_input);
#else
		msg_input->pushInput(msg_event);
#endif 
	}
	if (GUIjoy)
		SDL_JoystickClose(GUIjoy);
}

void gui_init(const char* msg)
{
#ifdef USE_SDL1
	if (screen == nullptr)
	{
		auto dummy_screen = SDL_SetVideoMode(GUI_WIDTH, GUI_HEIGHT, 16, SDL_SWSURFACE | SDL_FULLSCREEN);
		screen = SDL_CreateRGBSurface(SDL_HWSURFACE, GUI_WIDTH, GUI_HEIGHT, 16,
			dummy_screen->format->Rmask, dummy_screen->format->Gmask, dummy_screen->format->Bmask, dummy_screen->format->Amask);
		SDL_FreeSurface(dummy_screen);
	}
#elif USE_SDL2
	if (sdlWindow == nullptr)
	{
		sdlWindow = SDL_CreateWindow("Amiberry-GUI",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			0,
			0,
			SDL_WINDOW_FULLSCREEN_DESKTOP);
		check_error_sdl(sdlWindow == nullptr, "Unable to create window");
	}

	if (screen == nullptr)
	{
		screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 16, 0, 0, 0, 0);
		check_error_sdl(screen == nullptr, "Unable to create SDL surface");
	}

	if (renderer == nullptr)
	{
		renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(renderer == nullptr, "Unable to create a renderer");
		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);
	}
		
	if (texture == nullptr)
	{
		texture = SDL_CreateTexture(renderer, screen->format->format, SDL_TEXTUREACCESS_STREAMING, screen->w, screen->h);
		check_error_sdl(renderer == nullptr, "Unable to create texture from Surface");
	}
#endif

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
#ifdef USE_SDL1
	msg_font = new gcn::contrib::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
#elif USE_SDL2
	msg_font = new gcn::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
#endif
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
	auto drawn = false;
	while (!msg_done)
	{
		// Poll input
		checkInput();

		// Now we let the Gui object perform its logic.
		msg_gui->logic();
		// Now we let the Gui object draw itself.
		msg_gui->draw();
		// Finally we update the screen.

#ifdef USE_SDL1
			SDL_Flip(screen);
#elif USE_SDL2
			if (cursor != nullptr)
				SDL_ShowCursor(SDL_ENABLE);

			SDL_UpdateTexture(texture, nullptr, screen->pixels, screen->pitch);

			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, nullptr, nullptr);
			SDL_RenderPresent(renderer);
#endif
	}

	gui_halt();
	SDL_ShowCursor(SDL_DISABLE);
}
