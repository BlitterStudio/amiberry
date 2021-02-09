#include <guisan.hpp>
#include <SDL_ttf.h>
#include <sstream>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "gui_handling.h"
#include "fsdb_host.h"

static gcn::Label* lblEmulatorVersion;
static gcn::Label* lblSDL_compiled_version;
static gcn::Icon* icon;
static gcn::Image* amiberryLogoImage;
static gcn::TextBox* textBox;
static gcn::ScrollArea* textBoxScrollArea;
static SDL_version compiled;
static SDL_version linked;

void InitPanelAbout(const struct _ConfigCategory& category)
{
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	
	amiberryLogoImage = gcn::Image::load(prefix_with_application_directory_path("data/amiberry-logo.png"));
	icon = new gcn::Icon(amiberryLogoImage);
	lblEmulatorVersion = new gcn::Label(get_version_string());
	std::ostringstream sdl_compiled;
	sdl_compiled << "Compiled against SDL2 v" << int(compiled.major) << "." << int(compiled.minor) << "." << int(compiled.patch);
	sdl_compiled << ", Linked against SDL2 v" << int(linked.major) << "." << int(linked.minor) << "." << int(linked.patch);
	lblSDL_compiled_version = new gcn::Label(sdl_compiled.str());
	
	textBox = new gcn::TextBox(
		"Dimitris Panokostas (MiDWaN) - Amiberry author\n"
		"Toni Wilen - WinUAE author\n"
		"TomB - Original ARM port of UAE, JIT ARM updates\n"
		"Chips - Original RPI port\n"
		"Gareth Fare - Serial port implementation\n"
		"Dom Cresswell (Horace & The Spider) - Controller and WHDBooter updates\n"
		"Christer Solskogen - Makefile and testing\n"
		"Gunnar Kristjansson - Amibian and inspiration\n"
		"Thomas Navarro Garcia - Original Amiberry logo\n"
		"Vasiliki Soufi - Amiberry name\n"
		"\n"
		"Dedicated to HeZoR - R.I.P. little brother (1978-2017)\n"
	);
	textBox->setEditable(false);
	textBox->setBackgroundColor(gui_baseCol);

	textBoxScrollArea = new gcn::ScrollArea(textBox);
	textBoxScrollArea->setBackgroundColor(gui_baseCol);
	textBoxScrollArea->setBaseColor(gui_baseCol);
	textBoxScrollArea->setWidth(category.panel->getWidth() - DISTANCE_BORDER * 2);
	textBoxScrollArea->setHeight(220);
	textBoxScrollArea->setBorderSize(1);
	
	auto pos_y = DISTANCE_BORDER;
	category.panel->add(icon, DISTANCE_BORDER, pos_y);
	pos_y += icon->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblEmulatorVersion, DISTANCE_BORDER, pos_y);
	pos_y += lblEmulatorVersion->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblSDL_compiled_version, DISTANCE_BORDER, pos_y);
	pos_y += lblSDL_compiled_version->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(textBoxScrollArea, DISTANCE_BORDER, pos_y);

	RefreshPanelAbout();
}

void ExitPanelAbout()
{
	delete lblEmulatorVersion;
	delete lblSDL_compiled_version;
	delete icon;
	delete amiberryLogoImage;
	delete textBoxScrollArea;
	delete textBox;
}

void RefreshPanelAbout()
{
}

bool HelpPanelAbout(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Here you can see the version information as well as");
	helptext.emplace_back("the people behind the development of this emulator.");
	return true;
}
