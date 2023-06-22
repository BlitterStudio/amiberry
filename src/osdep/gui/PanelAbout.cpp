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
static gcn::Label* lblCopyright;
static gcn::Label* lblSDL_compiled_version;
static gcn::Icon* icon;
static gcn::Image* amiberryLogoImage;
static gcn::TextBox* textBox;
static gcn::ScrollArea* textBoxScrollArea;

void InitPanelAbout(const config_category& category)
{
	amiberryLogoImage = gcn::Image::load(prefix_with_data_path("amiberry-logo.png"));
	icon = new gcn::Icon(amiberryLogoImage);
	lblEmulatorVersion = new gcn::Label(get_version_string());
	lblCopyright = new gcn::Label("Copyright (C) 2016-2023 Dimitris Panokostas");
	lblSDL_compiled_version = new gcn::Label(get_sdl2_version_string());
	
	textBox = new gcn::TextBox(
		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"any later version.\n\n"

		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the\n"
		"GNU General Public License for more details.\n\n"

		"You should have received a copy of the GNU General Public License\n"
		"along with this program. If not, see https://www.gnu.org/licenses\n\n"

		"Credits:\n"
		"Dimitris Panokostas (MiDWaN) - Amiberry author\n"
		"Toni Wilen - WinUAE author\n"
		"TomB - Original ARM port of UAE, JIT ARM updates\n"
		"Rob Smith, Drawbridge floppy controller\n"
		"Gareth Fare - Original Serial port implementation\n"
		"Dom Cresswell (Horace & The Spider) - Controller and WHDBooter updates\n"
		"Christer Solskogen - Makefile and testing\n"
		"Gunnar Kristjansson - Amibian and inspiration\n"
		"Thomas Navarro Garcia - Original Amiberry logo\n"
		"Chips - Original RPI port\n"
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
	textBoxScrollArea->setHeight(210);
	textBoxScrollArea->setBorderSize(1);
	
	auto pos_y = DISTANCE_BORDER;
	category.panel->add(icon, DISTANCE_BORDER, pos_y);
	pos_y += icon->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblEmulatorVersion, DISTANCE_BORDER, pos_y);
	pos_y += lblEmulatorVersion->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblCopyright, DISTANCE_BORDER, pos_y);
	pos_y += lblCopyright->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblSDL_compiled_version, DISTANCE_BORDER, pos_y);
	pos_y += lblSDL_compiled_version->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(textBoxScrollArea, DISTANCE_BORDER, pos_y);

	RefreshPanelAbout();
}

void ExitPanelAbout()
{
	delete lblEmulatorVersion;
	delete lblCopyright;
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
