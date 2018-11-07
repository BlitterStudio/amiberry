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
#include "options.h"
#include "uae.h"
#include "gui.h"
#include "gui_handling.h"

static gcn::Label* lblEmulatorVersion;
static gcn::Icon* icon;
static gcn::Image* amiberryLogoImage;
static gcn::TextBox* textBox;
static gcn::ScrollArea* textBoxScrollArea;

void InitPanelAbout(const struct _ConfigCategory& category)
{
	amiberryLogoImage = gcn::Image::load("data/amiberry-logo.png");
	icon = new gcn::Icon(amiberryLogoImage);
	lblEmulatorVersion = new gcn::Label("Amiberry v2.24b5 (2018-11-07)");

	textBox = new gcn::TextBox(
		"Dimitris Panokostas (MiDWaN) - Amiberry author\n"
		"Toni Wilen - WinUAE author\n"
		"TomB - Original ARM port of UAE, core emulation updates\n"
		"Alynna Trypnotk - Tinker Board port\n"
		"Chips - Original RPI port\n"
		"Dom Cresswell (Horace & The Spider) - Controller and WHDBooter updates\n"
		"Christer Solskogen - Makefile improvements\n"
		"Gunnar Kristjansson - Amibian and inspiration\n"
		"Thomas Navarro Garcia - Amiberry logo\n"
		"Vasiliki Soufi - Amiberry name\n"
		"\n"
		"Dedicated to HeZoR - R.I.P. (1978-2017)\n"
		);

	textBox->setBackgroundColor(gui_baseCol);
	textBoxScrollArea = new gcn::ScrollArea(textBox);
	textBoxScrollArea->setBackgroundColor(gui_baseCol);
	textBoxScrollArea->setBaseColor(gui_baseCol);
	textBoxScrollArea->setWidth(category.panel->getWidth() - DISTANCE_BORDER * 2);
	textBoxScrollArea->setHeight(100);
#ifdef USE_SDL1
	textBoxScrollArea->setFrameSize(1);
#elif USE_SDL2
	textBoxScrollArea->setBorderSize(1);
#endif

	auto pos_y = DISTANCE_BORDER;
	category.panel->add(icon, DISTANCE_BORDER, pos_y);
	pos_y += icon->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblEmulatorVersion, DISTANCE_BORDER, pos_y);
	pos_y += lblEmulatorVersion->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(textBoxScrollArea, DISTANCE_BORDER, pos_y);

	RefreshPanelAbout();
}

void ExitPanelAbout()
{
	delete lblEmulatorVersion;
	delete icon;
	delete amiberryLogoImage;
	delete textBoxScrollArea;
	delete textBox;
}

void RefreshPanelAbout()
{
	
}

bool HelpPanelAbout(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.emplace_back("Here you can see the version information as well as");
	helptext.emplace_back("the people behind the development of this emulator.");
	return true;
}