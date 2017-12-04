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
#ifdef PANDORA
static gcn::Label* lblPandoraSpeed;
static gcn::Label* lblPandoraSpeedInfo;
static gcn::Slider* sldPandoraSpeed;
#endif
static gcn::UaeCheckBox* chkBSDSocket;
static gcn::UaeCheckBox* chkMasterWP;

static gcn::Label* lblOpenGUI;
static gcn::TextField* txtOpenGUI;
static gcn::Button* cmdOpenGUI;

static gcn::Label* lblKeyForQuit;
static gcn::TextField* txtKeyForQuit;
static gcn::Button* cmdKeyForQuit;

static gcn::Label* lblNumLock;
static gcn::UaeDropDown* cboKBDLed_num;
static gcn::Label* lblScrLock;
static gcn::UaeDropDown* cboKBDLed_scr;

class StringListModel : public gcn::ListModel
{
	vector<string> values;
public:
	StringListModel(const char* entries[], const int count)
	{
		for (int i = 0; i < count; ++i)
			values.push_back(entries[i]);
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
#ifdef PANDORA
		else if (actionEvent.getSource() == chkHideIdleLed)
			changed_prefs.pandora_hide_idle_led = chkHideIdleLed->isSelected();
#endif
		else if (actionEvent.getSource() == chkShowGUI)
			changed_prefs.start_gui = chkShowGUI->isSelected();

		else if (actionEvent.getSource() == chkRetroArchQuit)
			changed_prefs.amiberry_use_retroarch_quit = chkRetroArchQuit->isSelected();

		else if (actionEvent.getSource() == chkRetroArchMenu)
			changed_prefs.amiberry_use_retroarch_menu = chkRetroArchMenu->isSelected();

		else if (actionEvent.getSource() == chkRetroArchReset)
			changed_prefs.amiberry_use_retroarch_reset = chkRetroArchReset->isSelected();

		//      else if (actionEvent.getSource() == chkRetroArchSavestate)
		//        changed_prefs.amiberry_use_retroarch_savestatebuttons = chkRetroArchSavestate->isSelected();

		else if (actionEvent.getSource() == chkBSDSocket)
			changed_prefs.socket_emu = chkBSDSocket->isSelected();

		else if (actionEvent.getSource() == chkMasterWP) {
			changed_prefs.floppy_read_only = chkMasterWP->isSelected();
			RefreshPanelQuickstart();
			RefreshPanelFloppy();
		}

		else if (actionEvent.getSource() == cmdOpenGUI)
		{
			const char* key = ShowMessageForInput("Press a key", "Press a key to map to Open the GUI", "Cancel");
			if (key != nullptr)
			{
				txtOpenGUI->setText(key);
				strcpy(changed_prefs.open_gui, key);
			}
		}

		else if (actionEvent.getSource() == cmdKeyForQuit)
		{
			const char* key = ShowMessageForInput("Press a key", "Press a key to map to Quit the emulator", "Cancel");
			if (key != nullptr)
			{
				txtKeyForQuit->setText(key);
				strcpy(changed_prefs.quit_amiberry, key);
			}
		}
#ifdef PANDORA
		else if (actionEvent.getSource() == sldPandoraSpeed)
		{
			int newspeed = (int)sldPandoraSpeed->getValue();
			newspeed = newspeed - (newspeed % 20);
			if (changed_prefs.pandora_cpu_speed != newspeed)
			{
				changed_prefs.pandora_cpu_speed = newspeed;
				RefreshPanelMisc();
			}
		}
#endif

		else if (actionEvent.getSource() == cboKBDLed_num)
			changed_prefs.kbd_led_num = cboKBDLed_num->getSelected();

		else if (actionEvent.getSource() == cboKBDLed_scr)
			changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected();
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


        
#ifdef PANDORA
	lblPandoraSpeed = new gcn::Label("Pandora Speed:");
	lblPandoraSpeed->setSize(110, LABEL_HEIGHT);
	lblPandoraSpeed->setAlignment(gcn::Graphics::RIGHT);
	sldPandoraSpeed = new gcn::Slider(500, 1260);
	sldPandoraSpeed->setSize(200, SLIDER_HEIGHT);
	sldPandoraSpeed->setBaseColor(gui_baseCol);
	sldPandoraSpeed->setMarkerLength(20);
	sldPandoraSpeed->setStepLength(20);
	sldPandoraSpeed->setId("PandSpeed");
	sldPandoraSpeed->addActionListener(miscActionListener);
	lblPandoraSpeedInfo = new gcn::Label("1000 MHz");
#endif
	chkBSDSocket = new gcn::UaeCheckBox("bsdsocket.library");
	chkBSDSocket->setId("BSDSocket");
	chkBSDSocket->addActionListener(miscActionListener);

	chkMasterWP = new gcn::UaeCheckBox("Master floppy write protection");
	chkMasterWP->setId("MasterWP");
	chkMasterWP->addActionListener(miscActionListener);

	lblNumLock = new gcn::Label("NumLock:");
	lblNumLock->setSize(85, LABEL_HEIGHT);
	lblNumLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_num = new gcn::UaeDropDown(&KBDLedList);
	cboKBDLed_num->setBaseColor(gui_baseCol);
	cboKBDLed_num->setBackgroundColor(colTextboxBackground);
	cboKBDLed_num->setId("numlock");
	cboKBDLed_num->addActionListener(miscActionListener);

	lblScrLock = new gcn::Label("ScrollLock:");
	lblScrLock->setSize(85, LABEL_HEIGHT);
	lblScrLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_scr = new gcn::UaeDropDown(&KBDLedList);
	cboKBDLed_scr->setBaseColor(gui_baseCol);
	cboKBDLed_scr->setBackgroundColor(colTextboxBackground);
	cboKBDLed_scr->setId("scrolllock");
	cboKBDLed_scr->addActionListener(miscActionListener);

	lblOpenGUI = new gcn::Label("Open GUI:");
	lblOpenGUI->setSize(85, LABEL_HEIGHT);
	lblOpenGUI->setAlignment(gcn::Graphics::RIGHT);
	txtOpenGUI = new gcn::TextField();
	txtOpenGUI->setEnabled(false);
	txtOpenGUI->setSize(85, txtOpenGUI->getHeight());
	txtOpenGUI->setBackgroundColor(colTextboxBackground);
	cmdOpenGUI = new gcn::Button("...");
	cmdOpenGUI->setId("OpenGUI");
	cmdOpenGUI->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdOpenGUI->setBaseColor(gui_baseCol);
	cmdOpenGUI->addActionListener(miscActionListener);

	lblKeyForQuit = new gcn::Label("Quit Key:");
	lblKeyForQuit->setSize(85, LABEL_HEIGHT);
	lblKeyForQuit->setAlignment(gcn::Graphics::RIGHT);
	txtKeyForQuit = new gcn::TextField();
	txtKeyForQuit->setEnabled(false);
	txtKeyForQuit->setSize(85, txtKeyForQuit->getHeight());
	txtKeyForQuit->setBackgroundColor(colTextboxBackground);
	cmdKeyForQuit = new gcn::Button("...");
	cmdKeyForQuit->setId("KeyForQuit");
	cmdKeyForQuit->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuit->setBaseColor(gui_baseCol);
	cmdKeyForQuit->addActionListener(miscActionListener);

	int posY = DISTANCE_BORDER;
	category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
	posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkHideIdleLed, DISTANCE_BORDER, posY);
	posY += chkHideIdleLed->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);
	posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;

	posY = DISTANCE_BORDER;
	int posX = 300;
	category.panel->add(chkRetroArchQuit, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchQuit->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkRetroArchMenu, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchMenu->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkRetroArchReset, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchReset->getHeight() + DISTANCE_NEXT_Y;
	//category.panel->add(chkRetroArchSavestate, posX + DISTANCE_BORDER, posY);

#ifdef PANDORA
	category.panel->add(lblPandoraSpeed, DISTANCE_BORDER, posY);
	category.panel->add(sldPandoraSpeed, DISTANCE_BORDER + lblPandoraSpeed->getWidth() + 8, posY);
	category.panel->add(lblPandoraSpeedInfo, sldPandoraSpeed->getX() + sldPandoraSpeed->getWidth() + 12, posY);
	posY += sldPandoraSpeed->getHeight() + DISTANCE_NEXT_Y;
#endif
	category.panel->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y * 2;
	category.panel->add(chkMasterWP, DISTANCE_BORDER, posY);
	posY += chkMasterWP->getHeight() + DISTANCE_NEXT_Y;


	category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
	category.panel->add(cboKBDLed_num, DISTANCE_BORDER + lblNumLock->getWidth() + 8, posY);

	category.panel->add(lblScrLock, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);

	posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblOpenGUI, DISTANCE_BORDER, posY);
	category.panel->add(txtOpenGUI, DISTANCE_BORDER + lblOpenGUI->getWidth() + 8, posY);
	category.panel->add(cmdOpenGUI, txtOpenGUI->getX() + txtOpenGUI->getWidth() + DISTANCE_NEXT_X, posY);

	category.panel->add(lblKeyForQuit, cmdOpenGUI->getX() + cmdOpenGUI->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(txtKeyForQuit, lblKeyForQuit->getX() + lblKeyForQuit->getWidth() + 8, posY);
	category.panel->add(cmdKeyForQuit, txtKeyForQuit->getX() + txtKeyForQuit->getWidth() + DISTANCE_NEXT_X, posY);

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
#ifdef PANDORA
	delete lblPandoraSpeed;
	delete sldPandoraSpeed;
	delete lblPandoraSpeedInfo;
#endif
	delete chkBSDSocket;
	delete chkMasterWP;

	delete lblScrLock;
	delete lblNumLock;
	delete cboKBDLed_num;
	delete cboKBDLed_scr;

	delete miscActionListener;
	delete lblOpenGUI;
	delete txtOpenGUI;

	delete lblKeyForQuit;
	delete txtKeyForQuit;
	delete cmdKeyForQuit;
}

void RefreshPanelMisc()
{
	chkStatusLine->setSelected(changed_prefs.leds_on_screen);
#ifdef PANDORA
	chkHideIdleLed->setSelected(changed_prefs.pandora_hide_idle_led);
#endif
	chkShowGUI->setSelected(changed_prefs.start_gui);

	chkRetroArchQuit->setSelected(changed_prefs.amiberry_use_retroarch_quit);
	chkRetroArchMenu->setSelected(changed_prefs.amiberry_use_retroarch_menu);
	chkRetroArchReset->setSelected(changed_prefs.amiberry_use_retroarch_reset);
	//chkRetroArchSavestate->setSelected(changed_prefs.amiberry_use_retroarch_statebuttons);  
#ifdef PANDORA
	sldPandoraSpeed->setValue(changed_prefs.pandora_cpu_speed);
	snprintf(tmp, 20, "%d MHz", changed_prefs.pandora_cpu_speed);
	lblPandoraSpeedInfo->setCaption(tmp);
#endif
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	chkMasterWP->setSelected(changed_prefs.floppy_read_only);

	cboKBDLed_num->setSelected(changed_prefs.kbd_led_num);
	cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr);

	txtOpenGUI->setText(changed_prefs.open_gui != "" ? changed_prefs.open_gui : "Click to map");
	txtKeyForQuit->setText(changed_prefs.quit_amiberry != "" ? changed_prefs.quit_amiberry : "Click to map");
}

bool HelpPanelMisc(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.push_back("\"Status Line\" shows/hides the status line indicator. During emulation, you can show/hide this by pressing left");
	helptext.push_back("shoulder and 'd'. The first value in the status line shows the idle time of the Pandora CPU in %, the second value");
	helptext.push_back("is the current frame rate. When you have a HDD in your Amiga emulation, the HD indicator shows read (blue) and");
	helptext.push_back("write (red) access to the HDD. The next values are showing the track number for each disk drive and indicates");
	helptext.push_back("disk access.");
	helptext.push_back("");
	helptext.push_back("When you deactivate the option \"Show GUI on startup\" and use this configuration by specifying it with the");
	helptext.push_back("command line parameter \"-config=<file>\", the emulation starts directly without showing the GUI.");
	helptext.push_back("");
#ifdef PANDORA
	helptext.push_back("Set the speed for the Pandora CPU to overclock it for games which need more power. Be careful with this");
	helptext.push_back("parameter.");
	helptext.push_back("");
#endif	
	helptext.push_back("\"bsdsocket.library\" enables network functions (i.e. for web browsers in OS3.9).");
	helptext.push_back("");
	helptext.push_back("\"Master floppy drive protection\" will disable all write access to floppy disks.");
	return true;
}
