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

#define DIALOG_WIDTH 600
#define DIALOG_HEIGHT 200

SDL_Surface* msg_screen;
SDL_Event msg_event;
#ifdef USE_SDL2
SDL_Texture* msg_texture;
#endif
bool msg_done = false;
gcn::Gui* msg_gui;
gcn::SDLGraphics* msg_graphics;
gcn::SDLInput* msg_input;
#ifdef USE_SDL1
gcn::contrib::SDLTrueTypeFont* msg_font;
#elif USE_SDL2
gcn::SDLTrueTypeFont* msg_font;
#endif

gcn::Color msg_baseCol;
gcn::Container* msg_top;
gcn::Window* wndMsg;
gcn::Button* cmdDone;
gcn::Label* txtMsg;

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

	if (msg_screen != nullptr)
	{
		SDL_FreeSurface(msg_screen);
		msg_screen = nullptr;
	}
#ifdef USE_SDL2
	if (msg_texture != nullptr)
	{
		SDL_DestroyTexture(msg_texture);
		msg_texture = nullptr;
	}
	// Clear the screen
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
#endif
}

void UpdateScreen()
{
#ifdef USE_SDL1
	wait_for_vsync();
	SDL_Flip(msg_screen);
#elif USE_SDL2
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, msg_texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
#endif
}

void checkInput()
{
	//-------------------------------------------------
	// Check user input
	//-------------------------------------------------	

	int gotEvent = 0;
	while (SDL_PollEvent(&msg_event))
	{
		gotEvent = 1;
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
	if (gotEvent)
	{
		// Now we let the Gui object perform its logic.
		msg_gui->logic();
		// Now we let the Gui object draw itself.
		msg_gui->draw();
#ifdef USE_SDL2
		SDL_UpdateTexture(msg_texture, nullptr, msg_screen->pixels, msg_screen->pitch);
#endif
	}
	// Finally we update the screen.
	UpdateScreen();
}

void gui_init(const char* msg)
{
#ifdef USE_SDL1
	if (msg_screen == nullptr)
	{
		auto dummy_screen = SDL_SetVideoMode(GUI_WIDTH, GUI_HEIGHT, 16, SDL_SWSURFACE | SDL_FULLSCREEN);
		msg_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, GUI_WIDTH, GUI_HEIGHT, 16,
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
	// make the scaled rendering look smoother (linear scaling).
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (msg_screen == nullptr)
	{
		msg_screen = SDL_CreateRGBSurface(0, GUI_WIDTH, GUI_HEIGHT, 32, 0, 0, 0, 0);
		check_error_sdl(msg_screen == nullptr, "Unable to create SDL surface");
	}

	if (renderer == nullptr)
	{
		renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		check_error_sdl(renderer == nullptr, "Unable to create a renderer");
		SDL_RenderSetLogicalSize(renderer, GUI_WIDTH, GUI_HEIGHT);
	}

	if (msg_texture == nullptr)
	{
		msg_texture = SDL_CreateTexture(renderer, msg_screen->format->format, SDL_TEXTUREACCESS_STREAMING, msg_screen->w,
		                                msg_screen->h);
		check_error_sdl(msg_texture == nullptr, "Unable to create texture from Surface");
	}
	SDL_ShowCursor(SDL_ENABLE);
#endif

	msg_graphics = new gcn::SDLGraphics();
	msg_graphics->setTarget(msg_screen);
	msg_input = new gcn::SDLInput();
	msg_gui = new gcn::Gui();
	msg_gui->setGraphics(msg_graphics);
	msg_gui->setInput(msg_input);
}

void widgets_init(const char* msg)
{
	msg_baseCol = gcn::Color(170, 170, 170);

	msg_top = new gcn::Container();
	msg_top->setDimension(gcn::Rectangle(0, 0, GUI_WIDTH, GUI_HEIGHT));
	msg_top->setBaseColor(msg_baseCol);
	msg_gui->setTop(msg_top);

	TTF_Init();
#ifdef USE_SDL1
	msg_font = new gcn::contrib::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
#elif USE_SDL2
	msg_font = new gcn::SDLTrueTypeFont("data/AmigaTopaz.ttf", 15);
#endif
	gcn::Widget::setGlobalFont(msg_font);

	wndMsg = new gcn::Window("InGameMessage");
	wndMsg->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndMsg->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndMsg->setBaseColor(msg_baseCol);
	wndMsg->setCaption("Information");
	wndMsg->setTitleBarHeight(TITLEBAR_HEIGHT);

	doneActionListener = new DoneActionListener();

	cmdDone = new gcn::Button("Ok");
	cmdDone->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdDone->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdDone->setBaseColor(msg_baseCol);
	cmdDone->setId("Done");
	cmdDone->addActionListener(doneActionListener);

	txtMsg = new gcn::Label(msg);
	txtMsg->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER, LABEL_HEIGHT);

	wndMsg->add(txtMsg, DISTANCE_BORDER, DISTANCE_BORDER);
	wndMsg->add(cmdDone);

	msg_top->add(wndMsg);
	cmdDone->requestFocus();
	wndMsg->requestModalFocus();
}

void gui_run()
{
	if (SDL_NumJoysticks() > 0)
	{
		GUIjoy = SDL_JoystickOpen(0);
	}

	// Prepare the screen once
	msg_gui->logic();
	msg_gui->draw();
#ifdef USE_SDL2
	SDL_UpdateTexture(msg_texture, nullptr, msg_screen->pixels, msg_screen->pitch);
#endif
	UpdateScreen();

	while (!msg_done)
	{
		// Poll input
		checkInput();
		UpdateScreen();
	}

	if (GUIjoy)
	{
		SDL_JoystickClose(GUIjoy);
		GUIjoy = nullptr;
	}
}

void InGameMessage(const char* msg)
{
	gui_init(msg);
	widgets_init(msg);

	gui_run();

	gui_halt();
	SDL_ShowCursor(SDL_DISABLE);
}
