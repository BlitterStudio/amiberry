#include <cstdio>
#include <cstdlib>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"

static gcn::Window* grpExpansionBoard;
static gcn::Window* grpAcceleratorBoard;
static gcn::Window* grpMiscExpansions;

static gcn::CheckBox* chkBSDSocket;
static gcn::CheckBox* chkScsi;
static gcn::CheckBox* chkCD32Fmv;
static gcn::CheckBox* chkSana2;

class ExpansionsActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{	
		if (action_event.getSource() == chkBSDSocket)
		{
			changed_prefs.socket_emu = chkBSDSocket->isSelected();
		}
		else if (action_event.getSource() == chkScsi)
		{
			changed_prefs.scsi = chkScsi->isSelected();
		}
		else if (action_event.getSource() == chkCD32Fmv)
		{
			changed_prefs.cs_cd32fmv = chkCD32Fmv->isSelected();
		}
		else if (action_event.getSource() == chkSana2)
		{
			changed_prefs.sana2 = chkSana2->isSelected();
		}
		//TODO
		
		RefreshPanelExpansions();
	}
};

ExpansionsActionListener* expansions_action_listener;

void InitPanelExpansions(const struct _ConfigCategory& category)
{
	expansions_action_listener = new ExpansionsActionListener();

	chkBSDSocket = new gcn::CheckBox("bsdsocket.library");
	chkBSDSocket->setId("chkBSDSocket");
	chkBSDSocket->addActionListener(expansions_action_listener);

	chkScsi = new gcn::CheckBox("uaescsi.device");
	chkScsi->setId("chkSCSI");
	chkScsi->addActionListener(expansions_action_listener);

	chkCD32Fmv = new gcn::CheckBox("CD32 Full Motion Video cartridge");
	chkCD32Fmv->setId("chkCD32Fmv");
	chkCD32Fmv->addActionListener(expansions_action_listener);

	chkSana2 = new gcn::CheckBox("uaenet.device");
	chkSana2->setId("chkSana2");
	chkSana2->addActionListener(expansions_action_listener);
	chkSana2->setEnabled(false); //TODO enable this when SANA2 support is implemented
	
	auto posY = DISTANCE_BORDER;
	auto posX = DISTANCE_BORDER;
	
	grpExpansionBoard = new gcn::Window("Expansion Board Settings");
	grpExpansionBoard->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	//TODO add items here
	grpExpansionBoard->setMovable(false);
	grpExpansionBoard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 200);
	grpExpansionBoard->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpExpansionBoard->setBaseColor(gui_baseCol);
	category.panel->add(grpExpansionBoard);

	grpAcceleratorBoard = new gcn::Window("Accelerator Board Settings");
	grpAcceleratorBoard->setPosition(DISTANCE_BORDER, grpExpansionBoard->getY() + grpExpansionBoard->getHeight() + DISTANCE_NEXT_Y);
	//TODO add items here
	grpAcceleratorBoard->setMovable(false);
	grpAcceleratorBoard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 150);
	grpAcceleratorBoard->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpAcceleratorBoard->setBaseColor(gui_baseCol);
	category.panel->add(grpAcceleratorBoard);
	
	grpMiscExpansions = new gcn::Window("Miscellaneous Expansions");
	grpMiscExpansions->setPosition(DISTANCE_BORDER, grpAcceleratorBoard->getY() + grpAcceleratorBoard->getHeight() + DISTANCE_NEXT_Y);
	grpMiscExpansions->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posX += chkScsi->getX() + chkScsi->getWidth() + DISTANCE_NEXT_X * 5;
	grpMiscExpansions->add(chkSana2, posX, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;
	grpMiscExpansions->add(chkScsi, DISTANCE_BORDER, posY);
	grpMiscExpansions->add(chkCD32Fmv, posX, posY);
	posY += chkScsi->getHeight() + DISTANCE_NEXT_Y;
	//TODO add items here
	grpMiscExpansions->setMovable(false);
	grpMiscExpansions->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 110);
	grpMiscExpansions->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpMiscExpansions->setBaseColor(gui_baseCol);
	category.panel->add(grpMiscExpansions);
	
	//TODO
	RefreshPanelExpansions();
}

void ExitPanelExpansions()
{
	//TODO
	delete chkBSDSocket;
	delete chkScsi;
	delete chkCD32Fmv;
	delete chkSana2;
	
	delete grpExpansionBoard;
	delete grpAcceleratorBoard;
	delete grpMiscExpansions;
	
	delete expansions_action_listener;
}

void RefreshPanelExpansions()
{
	chkBSDSocket->setEnabled(!emulating);
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	chkScsi->setSelected(changed_prefs.scsi);
	chkCD32Fmv->setSelected(changed_prefs.cs_cd32fmv);
	chkSana2->setSelected(changed_prefs.sana2);
	//TODO
}

bool HelpPanelExpansions(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("\"bsdsocket.library\" enables network functions (i.e. for web browsers in OS3.9).");
	helptext.emplace_back("You don't need to use a TCP stack (e.g. AmiTCP/Genesis/Roadshow) when this option is enabled.");
	helptext.emplace_back(" ");
	//TODO
	return true;
}