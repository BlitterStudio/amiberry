#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"

static gcn::Window* grpWhenActive;
static gcn::Label* lblActiveRunAtPrio;
static gcn::DropDown* cboActiveRunAtPrio;
static gcn::Label* lblActiveMouseUncaptured;
static gcn::CheckBox* chkActivePauseEmulation;
static gcn::CheckBox* chkActiveDisableSound;

static gcn::Window* grpWhenInactive;
static gcn::Label* lblInactiveRunAtPrio;
static gcn::DropDown* cboInactiveRunAtPrio;
static gcn::CheckBox* chkInactivePauseEmulation;
static gcn::CheckBox* chkInactiveDisableSound;
static gcn::CheckBox* chkInactiveDisableControllers;

static gcn::Window* grpWhenMinimized;
static gcn::Label* lblMinimizedRunAtPrio;
static gcn::DropDown* cboMinimizedRunAtPrio;
static gcn::CheckBox* chkMinimizedPauseEmulation;
static gcn::CheckBox* chkMinimizedDisableSound;
static gcn::CheckBox* chkMinimizedDisableControllers;

static const std::vector<std::string> prio_values = { "Low", "Normal", "High" };
static gcn::StringListModel prio_values_list(prio_values);

class PrioActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{
		if (action_event.getSource() == cboActiveRunAtPrio)
		{
			if (cboActiveRunAtPrio->getSelected() == 0)
			{
				// Low prio
				changed_prefs.active_capture_priority = 0;
			}
			else if (cboActiveRunAtPrio->getSelected() == 1)
			{
				// Normal prio
				changed_prefs.active_capture_priority = 1;
			}
			else if (cboActiveRunAtPrio->getSelected() == 2)
			{
				//High prio
				changed_prefs.active_capture_priority = 2;
			}
		}

		else if (action_event.getSource() == chkActivePauseEmulation)
		{
			// set pause emulation
			changed_prefs.active_nocapture_pause = chkActivePauseEmulation->isSelected();
		}
		else if (action_event.getSource() == chkActiveDisableSound)
		{
			// set disable sound
			changed_prefs.active_nocapture_nosound = chkActiveDisableSound->isSelected();
		}

		else if (action_event.getSource() == cboInactiveRunAtPrio)
		{
			if (cboInactiveRunAtPrio->getSelected() == 0)
			{
				// Low Prio
				changed_prefs.inactive_priority = 0;
			}
			else if (cboInactiveRunAtPrio->getSelected() == 1)
			{
				// Normal prio
				changed_prefs.inactive_priority = 1;
			}
			else if (cboInactiveRunAtPrio->getSelected() == 2)
			{
				// High prio
				changed_prefs.inactive_priority = 2;
			}
		}
		else if (action_event.getSource() == chkInactivePauseEmulation)
		{
			// set inactive pause emulation
			changed_prefs.inactive_pause = chkInactivePauseEmulation->isSelected();
		}
		else if (action_event.getSource() == chkInactiveDisableSound)
		{
			// set inactive disable sound
			changed_prefs.inactive_nosound = chkInactiveDisableSound->isSelected();
		}
		else if (action_event.getSource() == chkInactiveDisableControllers)
		{
			// set inactive disable controllers
			changed_prefs.inactive_input = chkInactiveDisableControllers->isSelected() ? 0 : 4;
		}

		else if (action_event.getSource() == cboMinimizedRunAtPrio)
		{
			if (cboMinimizedRunAtPrio->getSelected() == 0)
			{
				// Low Prio
				changed_prefs.minimized_priority = 0;
			}
			else if (cboMinimizedRunAtPrio->getSelected() == 1)
			{
				// Normal prio
				changed_prefs.minimized_priority = 1;
			}
			else if (cboMinimizedRunAtPrio->getSelected() == 2)
			{
				// High prio
				changed_prefs.minimized_priority = 2;
			}
		}
		else if (action_event.getSource() == chkMinimizedPauseEmulation)
		{
			changed_prefs.minimized_pause = chkMinimizedPauseEmulation->isSelected();
		}
		else if (action_event.getSource() == chkMinimizedDisableSound)
		{
			changed_prefs.minimized_nosound = chkMinimizedDisableSound->isSelected();
		}
		else if (action_event.getSource() == chkMinimizedDisableControllers)
		{
			changed_prefs.minimized_input = chkMinimizedDisableControllers->isSelected() ? 0 : 4;
		}
		
		RefreshPanelPrio();
	}
};

PrioActionListener* prioActionListener;

void InitPanelPrio(const config_category& category)
{
	prioActionListener = new PrioActionListener();

	int posY = DISTANCE_BORDER;

	lblActiveRunAtPrio = new gcn::Label("Run at priority:");
	lblActiveRunAtPrio->setAlignment(gcn::Graphics::CENTER);
	cboActiveRunAtPrio = new gcn::DropDown(&prio_values_list);
	cboActiveRunAtPrio->setSize(150, cboActiveRunAtPrio->getHeight());
	cboActiveRunAtPrio->setBaseColor(gui_baseCol);
	cboActiveRunAtPrio->setBackgroundColor(colTextboxBackground);
	cboActiveRunAtPrio->setSelectionColor(gui_selection_color);
	cboActiveRunAtPrio->setId("cboActiveRunAtPrio");
	cboActiveRunAtPrio->addActionListener(prioActionListener);

	lblActiveMouseUncaptured = new gcn::Label("Mouse uncaptured:");
	lblActiveMouseUncaptured->setAlignment(gcn::Graphics::CENTER);

	chkActivePauseEmulation = new gcn::CheckBox("Pause emulation");
	chkActivePauseEmulation->setId("chkActivePauseEmulation");
	chkActivePauseEmulation->setBaseColor(gui_baseCol);
	chkActivePauseEmulation->setBackgroundColor(colTextboxBackground);
	chkActivePauseEmulation->addActionListener(prioActionListener);

	chkActiveDisableSound = new gcn::CheckBox("Disable sound");
	chkActiveDisableSound->setId("chkActiveDisableSound");
	chkActiveDisableSound->setBaseColor(gui_baseCol);
	chkActiveDisableSound->setBackgroundColor(colTextboxBackground);
	chkActiveDisableSound->addActionListener(prioActionListener);

	grpWhenActive = new gcn::Window("When Active");
	grpWhenActive->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpWhenActive->add(lblActiveRunAtPrio, DISTANCE_BORDER, posY);
	posY += lblActiveRunAtPrio->getHeight() + DISTANCE_NEXT_Y;
	grpWhenActive->add(cboActiveRunAtPrio, DISTANCE_BORDER, posY);
	posY += cboActiveRunAtPrio->getHeight() + DISTANCE_NEXT_Y;
	grpWhenActive->add(lblActiveMouseUncaptured, DISTANCE_BORDER, posY);
	posY += lblActiveMouseUncaptured->getHeight() + DISTANCE_NEXT_Y;
	grpWhenActive->add(chkActivePauseEmulation, DISTANCE_BORDER, posY);
	posY += chkActivePauseEmulation->getHeight() + DISTANCE_NEXT_Y;
	grpWhenActive->add(chkActiveDisableSound, DISTANCE_BORDER, posY);
	grpWhenActive->setMovable(false);
	grpWhenActive->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpWhenActive->setSize(cboActiveRunAtPrio->getWidth() + DISTANCE_BORDER * 3, TITLEBAR_HEIGHT + chkActiveDisableSound->getY() + chkActiveDisableSound->getHeight() + DISTANCE_NEXT_Y * 3);
	grpWhenActive->setBaseColor(gui_baseCol);
	category.panel->add(grpWhenActive);
	
	lblInactiveRunAtPrio = new gcn::Label("Run at priority:");
	lblInactiveRunAtPrio->setAlignment(gcn::Graphics::CENTER);
	cboInactiveRunAtPrio = new gcn::DropDown(&prio_values_list);
	cboInactiveRunAtPrio->setSize(150, cboInactiveRunAtPrio->getHeight());
	cboInactiveRunAtPrio->setBaseColor(gui_baseCol);
	cboInactiveRunAtPrio->setBackgroundColor(colTextboxBackground);
	cboInactiveRunAtPrio->setSelectionColor(gui_selection_color);
	cboInactiveRunAtPrio->setId("cboInactiveRunAtPrio");
	cboInactiveRunAtPrio->addActionListener(prioActionListener);

	chkInactivePauseEmulation = new gcn::CheckBox("Pause emulation");
	chkInactivePauseEmulation->setId("chkInactivePauseEmulation");
	chkInactivePauseEmulation->setBaseColor(gui_baseCol);
	chkInactivePauseEmulation->setBackgroundColor(colTextboxBackground);
	chkInactivePauseEmulation->addActionListener(prioActionListener);

	chkInactiveDisableSound = new gcn::CheckBox("Disable sound");
	chkInactiveDisableSound->setId("chkInactiveDisableSound");
	chkInactiveDisableSound->setBaseColor(gui_baseCol);
	chkInactiveDisableSound->setBackgroundColor(colTextboxBackground);
	chkInactiveDisableSound->addActionListener(prioActionListener);

	chkInactiveDisableControllers = new gcn::CheckBox("Disable input");
	chkInactiveDisableControllers->setId("chkInactiveDisableControllers");
	chkInactiveDisableControllers->setBaseColor(gui_baseCol);
	chkInactiveDisableControllers->setBackgroundColor(colTextboxBackground);
	chkInactiveDisableControllers->addActionListener(prioActionListener);

	grpWhenInactive = new gcn::Window("When Inactive");
	grpWhenInactive->setPosition(grpWhenActive->getX() + grpWhenActive->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpWhenInactive->add(lblInactiveRunAtPrio, DISTANCE_BORDER, DISTANCE_BORDER);
	grpWhenInactive->add(cboInactiveRunAtPrio, DISTANCE_BORDER, cboActiveRunAtPrio->getY());
	grpWhenInactive->add(chkInactivePauseEmulation, DISTANCE_BORDER, chkActivePauseEmulation->getY());
	grpWhenInactive->add(chkInactiveDisableSound, DISTANCE_BORDER, chkActiveDisableSound->getY());
	posY += chkInactiveDisableSound->getHeight() + DISTANCE_NEXT_Y;
	grpWhenInactive->add(chkInactiveDisableControllers, DISTANCE_BORDER, posY);
	grpWhenInactive->setMovable(false);
	grpWhenInactive->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpWhenInactive->setSize(grpWhenActive->getWidth(), grpWhenActive->getHeight());
	grpWhenInactive->setBaseColor(gui_baseCol);
	category.panel->add(grpWhenInactive);

	lblMinimizedRunAtPrio = new gcn::Label("Run at priority:");
	lblMinimizedRunAtPrio->setAlignment(gcn::Graphics::CENTER);
	cboMinimizedRunAtPrio = new gcn::DropDown(&prio_values_list);
	cboMinimizedRunAtPrio->setSize(150, cboInactiveRunAtPrio->getHeight());
	cboMinimizedRunAtPrio->setBaseColor(gui_baseCol);
	cboMinimizedRunAtPrio->setBackgroundColor(colTextboxBackground);
	cboMinimizedRunAtPrio->setSelectionColor(gui_selection_color);
	cboMinimizedRunAtPrio->setId("cboMinimizedRunAtPrio");
	cboMinimizedRunAtPrio->addActionListener(prioActionListener);

	chkMinimizedPauseEmulation = new gcn::CheckBox("Pause emulation");
	chkMinimizedPauseEmulation->setId("chkMinimizedPauseEmulation");
	chkMinimizedPauseEmulation->setBaseColor(gui_baseCol);
	chkMinimizedPauseEmulation->setBackgroundColor(colTextboxBackground);
	chkMinimizedPauseEmulation->addActionListener(prioActionListener);

	chkMinimizedDisableSound = new gcn::CheckBox("Disable sound");
	chkMinimizedDisableSound->setId("chkMinimizedDisableSound");
	chkMinimizedDisableSound->setBaseColor(gui_baseCol);
	chkMinimizedDisableSound->setBackgroundColor(colTextboxBackground);
	chkMinimizedDisableSound->addActionListener(prioActionListener);

	chkMinimizedDisableControllers = new gcn::CheckBox("Disable input");
	chkMinimizedDisableControllers->setId("chkMinimizedDisableControllers");
	chkMinimizedDisableControllers->setBaseColor(gui_baseCol);
	chkMinimizedDisableControllers->setBackgroundColor(colTextboxBackground);
	chkMinimizedDisableControllers->addActionListener(prioActionListener);

	grpWhenMinimized = new gcn::Window("When Minimized");
	grpWhenMinimized->setPosition(grpWhenInactive->getX() + grpWhenInactive->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpWhenMinimized->add(lblMinimizedRunAtPrio, DISTANCE_BORDER, DISTANCE_BORDER);
	grpWhenMinimized->add(cboMinimizedRunAtPrio, DISTANCE_BORDER, cboInactiveRunAtPrio->getY());
	grpWhenMinimized->add(chkMinimizedPauseEmulation, DISTANCE_BORDER, chkInactivePauseEmulation->getY());
	grpWhenMinimized->add(chkMinimizedDisableSound, DISTANCE_BORDER, chkInactiveDisableSound->getY());
	grpWhenMinimized->add(chkMinimizedDisableControllers, DISTANCE_BORDER, chkInactiveDisableControllers->getY());
	grpWhenMinimized->setMovable(false);
	grpWhenMinimized->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpWhenMinimized->setSize(grpWhenActive->getWidth(), grpWhenInactive->getHeight());
	grpWhenMinimized->setBaseColor(gui_baseCol);
	category.panel->add(grpWhenMinimized);
	
	RefreshPanelPrio();
}

void ExitPanelPrio()
{
	delete lblActiveRunAtPrio;
	delete cboActiveRunAtPrio;
	delete lblActiveMouseUncaptured;
	delete chkActivePauseEmulation;
	delete chkActiveDisableSound;
	delete grpWhenActive;

	delete lblInactiveRunAtPrio;
	delete cboInactiveRunAtPrio;
	delete chkInactivePauseEmulation;
	delete chkInactiveDisableSound;
	delete chkInactiveDisableControllers;
	delete grpWhenInactive;

	delete lblMinimizedRunAtPrio;
	delete cboMinimizedRunAtPrio;
	delete chkMinimizedPauseEmulation;
	delete chkMinimizedDisableSound;
	delete chkMinimizedDisableControllers;
	delete grpWhenMinimized;
	
	delete prioActionListener;
}

void RefreshPanelPrio()
{
	cboActiveRunAtPrio->setSelected(changed_prefs.active_capture_priority);
	chkActivePauseEmulation->setSelected(changed_prefs.active_nocapture_pause);
	chkActiveDisableSound->setSelected(changed_prefs.active_nocapture_nosound);

	cboInactiveRunAtPrio->setSelected(changed_prefs.inactive_priority);
	chkInactivePauseEmulation->setSelected(changed_prefs.inactive_pause);
	chkInactiveDisableSound->setSelected(changed_prefs.inactive_nosound);
	chkInactiveDisableControllers->setSelected(changed_prefs.inactive_input == 0);

	cboMinimizedRunAtPrio->setSelected(changed_prefs.minimized_priority);
	chkMinimizedPauseEmulation->setSelected(changed_prefs.minimized_pause);
	chkMinimizedDisableSound->setSelected(changed_prefs.minimized_nosound);
	chkMinimizedDisableControllers->setSelected(changed_prefs.minimized_input == 0);
}

bool HelpPanelPrio(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back(" ");
	helptext.emplace_back("In this panel you can configure the task priority for Amiberry, under different usage");
	helptext.emplace_back("scenarios. In most cases, the default settings work fine here.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"When Active\" - Set the priority options for when Amiberry is currently active.");
	helptext.emplace_back("     |");
	helptext.emplace_back("  \"Mouse uncaptured\" - Set what Amiberry does when active and mouse capture is lost.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"When Inactive\" - Set the priority options for when Amiberry is currently inactive.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"When Minimized\" - Set the priority options for when Amiberry is minimized.");
	helptext.emplace_back(" ");
	return true;
}
