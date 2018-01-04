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
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "include/memory.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "gui_handling.h"

static gcn::UaeCheckBox* chkRetroArchQuit;
static gcn::UaeCheckBox* chkRetroArchMenu;
static gcn::UaeCheckBox* chkRetroArchReset;
//static gcn::UaeCheckBox* chkRetroArchSaveState;

static gcn::UaeCheckBox* chkStatusLine;
static gcn::UaeCheckBox* chkHideIdleLed;
static gcn::UaeCheckBox* chkShowGUI;

static gcn::UaeCheckBox* chkBSDSocket;
static gcn::UaeCheckBox* chkMasterWP;

static gcn::Label* lblNumLock;
static gcn::UaeDropDown* cboKBDLed_num;
static gcn::Label* lblScrLock;
static gcn::UaeDropDown* cboKBDLed_scr;

static gcn::Label* lblOpenGUI;
static gcn::TextField* txtOpenGUI;
static gcn::Button* cmdOpenGUI;

static gcn::Label* lblKeyForQuit;
static gcn::TextField* txtKeyForQuit;
static gcn::Button* cmdKeyForQuit;

class StringListModel : public gcn::ListModel
{
	vector<string> values;
public:
	StringListModel(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	string getElementAt(int i) override
	{
		if (i < 0 || i >= values.size())
			return "---";
		return values[i];
	}
};

static const char* listValues[] = {"none", "POWER", "DF0", "DF1", "DF2", "DF3", "DF*", "HD", "CD"};
static StringListModel KBDLedList(listValues, 9);

class MiscActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkStatusLine)
			changed_prefs.leds_on_screen = chkStatusLine->isSelected();

		else if (actionEvent.getSource() == chkShowGUI)
			changed_prefs.start_gui = chkShowGUI->isSelected();

		else if (actionEvent.getSource() == chkRetroArchQuit)
			changed_prefs.use_retroarch_quit = chkRetroArchQuit->isSelected();

		else if (actionEvent.getSource() == chkRetroArchMenu)
			changed_prefs.use_retroarch_menu = chkRetroArchMenu->isSelected();

		else if (actionEvent.getSource() == chkRetroArchReset)
			changed_prefs.use_retroarch_reset = chkRetroArchReset->isSelected();

		//      else if (actionEvent.getSource() == chkRetroArchSavestate)
		//        changed_prefs.amiberry_use_retroarch_savestatebuttons = chkRetroArchSavestate->isSelected();

		else if (actionEvent.getSource() == chkBSDSocket)
			changed_prefs.socket_emu = chkBSDSocket->isSelected();

		else if (actionEvent.getSource() == chkMasterWP) {
			changed_prefs.floppy_read_only = chkMasterWP->isSelected();
			RefreshPanelQuickstart();
			RefreshPanelFloppy();
		}

		else if (actionEvent.getSource() == cboKBDLed_num)
			changed_prefs.kbd_led_num = cboKBDLed_num->getSelected();

		else if (actionEvent.getSource() == cboKBDLed_scr)
			changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected();

		else if (actionEvent.getSource() == cmdOpenGUI)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Open the GUI", "Cancel");
			if (key != nullptr)
			{
				txtOpenGUI->setText(key);
				strcpy(changed_prefs.open_gui, key);
			}
		}

		else if (actionEvent.getSource() == cmdKeyForQuit)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Quit the emulator", "Cancel");
			if (key != nullptr)
			{
				txtKeyForQuit->setText(key);
				strcpy(changed_prefs.quit_amiberry, key);
			}
		}
	}
};

MiscActionListener* miscActionListener;


void InitPanelMisc(const struct _ConfigCategory& category)
{
	miscActionListener = new MiscActionListener();

	chkStatusLine = new gcn::UaeCheckBox("Status Line");
	chkStatusLine->setId("StatusLine");
	chkStatusLine->addActionListener(miscActionListener);

	chkHideIdleLed = new gcn::UaeCheckBox("Hide idle led");
	chkHideIdleLed->setId("HideIdle");
	chkHideIdleLed->addActionListener(miscActionListener);

	chkShowGUI = new gcn::UaeCheckBox("Show GUI on startup");
	chkShowGUI->setId("ShowGUI");
	chkShowGUI->addActionListener(miscActionListener);

	chkRetroArchQuit = new gcn::UaeCheckBox("Use RetroArch Quit Button");
	chkRetroArchQuit->setId("RetroArchQuit");
	chkRetroArchQuit->addActionListener(miscActionListener);

	chkRetroArchMenu = new gcn::UaeCheckBox("Use RetroArch Menu Button");
	chkRetroArchMenu->setId("RetroArchMenu");
	chkRetroArchMenu->addActionListener(miscActionListener);

	chkRetroArchReset = new gcn::UaeCheckBox("Use RetroArch Reset Button");
	chkRetroArchReset->setId("RetroArchReset");
	chkRetroArchReset->addActionListener(miscActionListener);

	//chkRetroArchSavestate = new gcn::UaeCheckBox("Use RetroArch State Controls");
	//chkRetroArchSavestate->setId("RetroArchState");
	//chkRetroArchSavestate->addActionListener(miscActionListener);

	chkBSDSocket = new gcn::UaeCheckBox("bsdsocket.library");
	chkBSDSocket->setId("BSDSocket");
	chkBSDSocket->addActionListener(miscActionListener);

	chkMasterWP = new gcn::UaeCheckBox("Master floppy write protection");
	chkMasterWP->setId("MasterWP");
	chkMasterWP->addActionListener(miscActionListener);

	lblNumLock = new gcn::Label("NumLock:");
	lblNumLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_num = new gcn::UaeDropDown(&KBDLedList);
	cboKBDLed_num->setBaseColor(gui_baseCol);
	cboKBDLed_num->setBackgroundColor(colTextboxBackground);
	cboKBDLed_num->setId("numlock");
	cboKBDLed_num->addActionListener(miscActionListener);

	lblScrLock = new gcn::Label("ScrollLock:");
	lblScrLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_scr = new gcn::UaeDropDown(&KBDLedList);
	cboKBDLed_scr->setBaseColor(gui_baseCol);
	cboKBDLed_scr->setBackgroundColor(colTextboxBackground);
	cboKBDLed_scr->setId("scrolllock");
	cboKBDLed_scr->addActionListener(miscActionListener);

	lblOpenGUI = new gcn::Label("Open GUI:");
	lblOpenGUI->setAlignment(gcn::Graphics::RIGHT);
	txtOpenGUI = new gcn::TextField();
	txtOpenGUI->setEnabled(false);
	txtOpenGUI->setSize(85, TEXTFIELD_HEIGHT);
	txtOpenGUI->setBackgroundColor(colTextboxBackground);
	cmdOpenGUI = new gcn::Button("...");
	cmdOpenGUI->setId("OpenGUI");
	cmdOpenGUI->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdOpenGUI->setBaseColor(gui_baseCol);
	cmdOpenGUI->addActionListener(miscActionListener);

	lblKeyForQuit = new gcn::Label("Quit Key:");
	lblKeyForQuit->setAlignment(gcn::Graphics::RIGHT);
	txtKeyForQuit = new gcn::TextField();
	txtKeyForQuit->setEnabled(false);
	txtKeyForQuit->setSize(85, TEXTFIELD_HEIGHT);
	txtKeyForQuit->setBackgroundColor(colTextboxBackground);
	cmdKeyForQuit = new gcn::Button("...");
	cmdKeyForQuit->setId("KeyForQuit");
	cmdKeyForQuit->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuit->setBaseColor(gui_baseCol);
	cmdKeyForQuit->addActionListener(miscActionListener);

	auto posY = DISTANCE_BORDER;
	category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
	posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkHideIdleLed, DISTANCE_BORDER, posY);
	posY += chkHideIdleLed->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);
	posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;

	posY = DISTANCE_BORDER;
	auto posX = 300;
	category.panel->add(chkRetroArchQuit, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchQuit->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkRetroArchMenu, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchMenu->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkRetroArchReset, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchReset->getHeight() + DISTANCE_NEXT_Y;
	//category.panel->add(chkRetroArchSavestate, posX + DISTANCE_BORDER, posY);

	category.panel->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y * 2;
	category.panel->add(chkMasterWP, DISTANCE_BORDER, posY);
	posY += chkMasterWP->getHeight() + DISTANCE_NEXT_Y * 2;


	category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
	category.panel->add(cboKBDLed_num, DISTANCE_BORDER + lblNumLock->getWidth() + 8, posY);

	category.panel->add(lblScrLock, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X * 2, posY);
	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);

	posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblOpenGUI, DISTANCE_BORDER, posY);
	category.panel->add(txtOpenGUI, lblOpenGUI->getX() + lblOpenGUI->getWidth() + 8, posY);
	category.panel->add(cmdOpenGUI, txtOpenGUI->getX() + txtOpenGUI->getWidth() + 8, posY);

	category.panel->add(lblKeyForQuit, cmdOpenGUI->getX() + cmdOpenGUI->getWidth() + DISTANCE_NEXT_X * 2, posY);
	category.panel->add(txtKeyForQuit, lblKeyForQuit->getX() + lblKeyForQuit->getWidth() + 8, posY);
	category.panel->add(cmdKeyForQuit, txtKeyForQuit->getX() + txtKeyForQuit->getWidth() + 8, posY);

	RefreshPanelMisc();
}

void ExitPanelMisc()
{
	delete chkStatusLine;
	delete chkHideIdleLed;
	delete chkShowGUI;

	delete chkRetroArchQuit;
	delete chkRetroArchMenu;
	delete chkRetroArchReset;
	//delete chkRetroArchSaveState;

	delete chkBSDSocket;
	delete chkMasterWP;

	delete lblScrLock;
	delete lblNumLock;
	delete cboKBDLed_num;
	delete cboKBDLed_scr;

	delete lblOpenGUI;
	delete txtOpenGUI;
	delete cmdOpenGUI;

	delete lblKeyForQuit;
	delete txtKeyForQuit;
	delete cmdKeyForQuit;

	delete miscActionListener;
}

void RefreshPanelMisc()
{
	chkStatusLine->setSelected(changed_prefs.leds_on_screen);
	chkShowGUI->setSelected(changed_prefs.start_gui);
	chkRetroArchQuit->setSelected(changed_prefs.use_retroarch_quit);
	chkRetroArchMenu->setSelected(changed_prefs.use_retroarch_menu);
	chkRetroArchReset->setSelected(changed_prefs.use_retroarch_reset);
	//chkRetroArchSavestate->setSelected(changed_prefs.use_retroarch_statebuttons);  

	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	chkMasterWP->setSelected(changed_prefs.floppy_read_only);

	cboKBDLed_num->setSelected(changed_prefs.kbd_led_num);
	cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr);

	txtOpenGUI->setText(strncmp(changed_prefs.open_gui, "", 1) != 0 ? changed_prefs.open_gui : "Click to map");
	txtKeyForQuit->setText(strncmp(changed_prefs.quit_amiberry, "", 1) != 0 ? changed_prefs.quit_amiberry : "Click to map");

}

bool HelpPanelMisc(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.emplace_back("\"Status Line\" Shows/Hides the status line indicator.");
	helptext.emplace_back("The first value in the status line shows the idle time of the CPU in %,");
	helptext.emplace_back("the second value is the current frame rate.");
	helptext.emplace_back("When you have a HDD in your Amiga emulation, the HD indicator shows read (blue) and write");
	helptext.emplace_back("(red) access to the HDD. The next values are showing the track number for each disk drive");
	helptext.emplace_back("and indicates disk access.");
	helptext.emplace_back("");
	helptext.emplace_back("When you deactivate the option \"Show GUI on startup\" and use this configuration ");
	helptext.emplace_back("by specifying it with the command line parameter \"-config=<file>\", ");
	helptext.emplace_back("the emulation starts directly without showing the GUI.");
	helptext.emplace_back("");
	helptext.emplace_back("\"bsdsocket.library\" enables network functions (i.e. for web browsers in OS3.9).");
	helptext.emplace_back("");
	helptext.emplace_back("\"Master floppy drive protection\" will disable all write access to floppy disks.");
	return true;
}
