#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "gui_handling.h"
#include "fsdb_host.h"

static gcn::Label* lblEmulatorVersion;
static gcn::Label* lblCopyright;
static gcn::Label* lblSDL_compiled_version;
static gcn::Label* lblSDL_driver;
static gcn::Icon* icon;
static gcn::Image* amiberryLogoImage;
static gcn::TextBox* textBox;
static gcn::ScrollArea* textBoxScrollArea;

void InitPanelAbout(const config_category& category)
{
	amiberryLogoImage = gcn::Image::load(prefix_with_data_path("amiberry-logo.png"));
	icon = new gcn::Icon(amiberryLogoImage);
	lblEmulatorVersion = new gcn::Label(get_version_string());
	lblCopyright = new gcn::Label(get_copyright_notice());
	lblSDL_compiled_version = new gcn::Label(get_sdl2_version_string());
#ifdef USE_DISPMANX
	lblSDL_driver = new gcn::Label("SDL2 video driver: DispmanX");
#else
	lblSDL_driver = new gcn::Label("SDL2 video driver: " + std::string(sdl_video_driver));
#endif
	
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
	textBox->setBackgroundColor(gui_base_color);

	textBoxScrollArea = new gcn::ScrollArea(textBox);
	textBoxScrollArea->setBackgroundColor(gui_base_color);
	textBoxScrollArea->setBaseColor(gui_base_color);
	textBoxScrollArea->setWidth(category.panel->getWidth() - DISTANCE_BORDER * 2);

	textBoxScrollArea->setBorderSize(1);
	
	int pos_y = DISTANCE_BORDER;
	category.panel->add(icon, DISTANCE_BORDER, pos_y);
	pos_y += icon->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblEmulatorVersion, DISTANCE_BORDER, pos_y);
	pos_y += lblEmulatorVersion->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(lblCopyright, DISTANCE_BORDER, pos_y);
	pos_y += lblCopyright->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(lblSDL_compiled_version, DISTANCE_BORDER, pos_y);
	pos_y += lblSDL_compiled_version->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(lblSDL_driver, DISTANCE_BORDER, pos_y);
	pos_y += lblSDL_driver->getHeight() + DISTANCE_NEXT_Y;
	textBoxScrollArea->setHeight(category.panel->getHeight() - DISTANCE_BORDER - pos_y);
	category.panel->add(textBoxScrollArea, DISTANCE_BORDER, pos_y);

	RefreshPanelAbout();
}

void ExitPanelAbout()
{
	delete lblEmulatorVersion;
	delete lblCopyright;
	delete lblSDL_compiled_version;
	delete lblSDL_driver;
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
	helptext.emplace_back("This panel contains information about the version of Amiberry, when it was changed,");
	helptext.emplace_back("which version of SDL2 it was compiled against and currently using.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Furthermore, you can also find the GPLv3 license notice here, and if you scroll down");
	helptext.emplace_back("all the credits to the people behind the development of this emulator as well.");
	helptext.emplace_back(" ");
	helptext.emplace_back("At the bottom of the screen, there are a few buttons available regardless of which");
	helptext.emplace_back("panel you have selected. Those are: ");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Shutdown\": allows you to shutdown the whole system Amiberry is running on. This");
	helptext.emplace_back(" option can be disabled if you wish, by setting \'disable_shutdown_button=yes\' in");
	helptext.emplace_back(" in your amiberry.conf file.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Quit\": This quits Amiberry, as you'd expect.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Restart\": This button will stop emulation (if running), reload Amiberry and reset");
	helptext.emplace_back(" the currently loaded configuration. This has a similar effect as if you Quit and start");
	helptext.emplace_back(" Amiberry again.");
	helptext.emplace_back(" It can be useful if you want to change a setting that cannot be changed on-the-fly,");
	helptext.emplace_back(" and you don't want to quit and start the Amiberry again to do that.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Help\": This will display some on-screen help/documentation, relating to the Panel");
	helptext.emplace_back(" you are currently in.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Reset\": This button will trigger a hard reset of the emulation, which will reboot");
	helptext.emplace_back(" with the current settings. ");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Start\": This button starts the emulation, using the current settings you have set.");
	helptext.emplace_back(" ");
	return true;
}
