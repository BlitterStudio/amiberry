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
static gcn::Window* grpMiscExpansions;

static gcn::CheckBox* chkBSDSocket;

class ExpansionsActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{	
		if (action_event.getSource() == chkBSDSocket)
			changed_prefs.socket_emu = chkBSDSocket->isSelected();
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

	auto posY = DISTANCE_BORDER;
	
	grpExpansionBoard = new gcn::Window("Expansion Board Settings");
	grpExpansionBoard->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	//TODO add items here
	grpExpansionBoard->setMovable(false);
	grpExpansionBoard->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 300);
	grpExpansionBoard->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpExpansionBoard->setBaseColor(gui_baseCol);
	category.panel->add(grpExpansionBoard);

	grpMiscExpansions = new gcn::Window("Miscellaneous Expansions");
	grpMiscExpansions->setPosition(DISTANCE_BORDER, grpExpansionBoard->getX() + grpExpansionBoard->getHeight() + DISTANCE_NEXT_Y);
	grpMiscExpansions->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;
	//TODO add items here
	grpMiscExpansions->setMovable(false);
	grpMiscExpansions->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, 150);
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
	delete grpExpansionBoard;
	delete grpMiscExpansions;
	delete expansions_action_listener;
}

void RefreshPanelExpansions()
{
	chkBSDSocket->setEnabled(!emulating);
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
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