#include <cstring>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "statusline.h"

static gcn::CheckBox* chkRetroArchQuit;
static gcn::CheckBox* chkRetroArchMenu;
static gcn::CheckBox* chkRetroArchReset;
//static gcn::CheckBox* chkRetroArchSaveState;

static gcn::CheckBox* chkStatusLine;
static gcn::CheckBox* chkStatusLineRtg;
static gcn::CheckBox* chkShowGUI;
static gcn::CheckBox* chkMouseUntrap;
static gcn::CheckBox* chkBSDSocket;
static gcn::CheckBox* chkMasterWP;
static gcn::CheckBox* chkClipboardSharing;
static gcn::CheckBox* chkAllowHostRun;

static gcn::Label* lblNumLock;
static gcn::DropDown* cboKBDLed_num;
static gcn::Label* lblScrLock;
static gcn::DropDown* cboKBDLed_scr;

static gcn::Label* lblOpenGUI;
static gcn::TextField* txtOpenGUI;
static gcn::Button* cmdOpenGUI;

static gcn::Label* lblKeyForQuit;
static gcn::TextField* txtKeyForQuit;
static gcn::Button* cmdKeyForQuit;

static gcn::Label* lblKeyActionReplay;
static gcn::TextField* txtKeyActionReplay;
static gcn::Button* cmdKeyActionReplay;

static gcn::Label* lblKeyFullScreen;
static gcn::TextField* txtKeyFullScreen;
static gcn::Button* cmdKeyFullScreen;

#ifdef SERIAL_PORT
static gcn::Window* grpSerialDevice;
static gcn::TextField* txtSerialDevice;
static gcn::CheckBox* chkSerialDirect;
static gcn::CheckBox* chkRTSCTS;
static gcn::CheckBox* chkUaeSerial;
#endif

class StringListModel : public gcn::ListModel
{
	std::vector<std::string> values;
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

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

static const char* listValues[] = {"none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD"};
static StringListModel KBDLedList(listValues, 8);

#ifdef SERIAL_PORT
class MiscKeyListener : public gcn::KeyListener
{
public:
	void keyPressed(gcn::KeyEvent& keyEvent) override
	{
		if (keyEvent.getSource() == txtSerialDevice)
		{
			snprintf (changed_prefs.sername, 256, "%s", txtSerialDevice->getText().c_str());
			if( changed_prefs.sername[0] )
				changed_prefs.use_serial = 1;
			else
				changed_prefs.use_serial = 0;
		}
	}
};
#endif

class MiscActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkStatusLine)
		{
			if (chkStatusLine->isSelected())
				changed_prefs.leds_on_screen = changed_prefs.leds_on_screen | STATUSLINE_CHIPSET;
			else
				changed_prefs.leds_on_screen = changed_prefs.leds_on_screen & ~STATUSLINE_CHIPSET;
		}
		else if (actionEvent.getSource() == chkStatusLineRtg)
		{
			if (chkStatusLineRtg->isSelected())
				changed_prefs.leds_on_screen = changed_prefs.leds_on_screen | STATUSLINE_RTG;
			else
				changed_prefs.leds_on_screen = changed_prefs.leds_on_screen & ~STATUSLINE_RTG;
		}

		else if (actionEvent.getSource() == chkShowGUI)
			changed_prefs.start_gui = chkShowGUI->isSelected();

		else if (actionEvent.getSource() == chkMouseUntrap)
			changed_prefs.input_mouse_untrap = chkMouseUntrap->isSelected();

		else if (actionEvent.getSource() == chkRetroArchQuit)
		{
			changed_prefs.use_retroarch_quit = chkRetroArchQuit->isSelected();
			RefreshPanelCustom();
		}

		else if (actionEvent.getSource() == chkRetroArchMenu)
		{
			changed_prefs.use_retroarch_menu = chkRetroArchMenu->isSelected();
			RefreshPanelCustom();
		}

		else if (actionEvent.getSource() == chkRetroArchReset)
		{
			changed_prefs.use_retroarch_reset = chkRetroArchReset->isSelected();
			RefreshPanelCustom();
		}

		//else if (actionEvent.getSource() == chkRetroArchSavestate)
		//   changed_prefs.amiberry_use_retroarch_savestatebuttons = chkRetroArchSavestate->isSelected();

		else if (actionEvent.getSource() == chkBSDSocket)
			changed_prefs.socket_emu = chkBSDSocket->isSelected();

		else if (actionEvent.getSource() == chkMasterWP)
		{
			changed_prefs.floppy_read_only = chkMasterWP->isSelected();
			RefreshPanelQuickstart();
			RefreshPanelFloppy();
		}

		else if (actionEvent.getSource() == chkClipboardSharing)
			changed_prefs.clipboard_sharing = chkClipboardSharing->isSelected();

		else if (actionEvent.getSource() == chkAllowHostRun)
			changed_prefs.allow_host_run = chkAllowHostRun->isSelected();

		else if (actionEvent.getSource() == cboKBDLed_num)
			changed_prefs.kbd_led_num = cboKBDLed_num->getSelected() - 1;

		else if (actionEvent.getSource() == cboKBDLed_scr)
			changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected() - 1;

		else if (actionEvent.getSource() == cmdOpenGUI)
		{
			const auto* const key = ShowMessageForInput("Press a key", "Press a key to map to Open the GUI, or ESC to cancel...", "Cancel");
			if (key != nullptr)
			{
				txtOpenGUI->setText(key);
				strcpy(changed_prefs.open_gui, key);
				strcpy(currprefs.open_gui, key);
			}
		}

		else if (actionEvent.getSource() == cmdKeyForQuit)
		{
			const auto* const key = ShowMessageForInput("Press a key", "Press a key to map to Quit the emulator, or ESC to cancel...", "Cancel");
			if (key != nullptr)
			{
				txtKeyForQuit->setText(key);
				strcpy(changed_prefs.quit_amiberry, key);
				strcpy(currprefs.quit_amiberry, key);
			}
		}

		else if (actionEvent.getSource() == cmdKeyActionReplay)
		{
			const auto* const key = ShowMessageForInput("Press a key", "Press a key to map to Action Replay, or ESC to cancel...", "Cancel");
			if (key != nullptr)
			{
				txtKeyActionReplay->setText(key);
				strcpy(changed_prefs.action_replay, key);
				strcpy(currprefs.action_replay, key);
			}
		}

		else if (actionEvent.getSource() == cmdKeyFullScreen)
		{
			const auto* const key = ShowMessageForInput("Press a key", "Press a key to map to toggle FullScreen, or ESC to cancel...", "Cancel");
			if (key != nullptr)
			{
				txtKeyFullScreen->setText(key);
				strcpy(changed_prefs.fullscreen_toggle, key);
				strcpy(currprefs.fullscreen_toggle, key);
			}
		}

#ifdef SERIAL_PORT
		else if (actionEvent.getSource() == txtSerialDevice)
			snprintf (changed_prefs.sername, 256, "%s", txtSerialDevice->getText().c_str());

		else if (actionEvent.getSource() == chkSerialDirect)
			changed_prefs.serial_direct = chkSerialDirect->isSelected();

		else if (actionEvent.getSource() == chkRTSCTS)
			changed_prefs.serial_hwctsrts = chkRTSCTS->isSelected();

		else if (actionEvent.getSource() == chkUaeSerial)
			changed_prefs.uaeserial = chkUaeSerial->isSelected();
#endif
	}
};

MiscActionListener* miscActionListener;
#ifdef SERIAL_PORT
MiscKeyListener* miscKeyListener;
#endif


void InitPanelMisc(const struct _ConfigCategory& category)
{
	miscActionListener = new MiscActionListener();
#ifdef SERIAL_PORT
	miscKeyListener = new MiscKeyListener();
#endif

	chkStatusLine = new gcn::CheckBox("Status Line native");
	chkStatusLine->setId("chkStatusLineNative");
	chkStatusLine->addActionListener(miscActionListener);

	chkStatusLineRtg = new gcn::CheckBox("Status Line RTG");
	chkStatusLineRtg->setId("chkStatusLineRtg");
	chkStatusLineRtg->addActionListener(miscActionListener);

	chkShowGUI = new gcn::CheckBox("Show GUI on startup");
	chkShowGUI->setId("chkShowGUI");
	chkShowGUI->addActionListener(miscActionListener);

	chkMouseUntrap = new gcn::CheckBox("Untrap = middle button");
	chkMouseUntrap->setId("chkMouseUntrap");
	chkMouseUntrap->addActionListener(miscActionListener);

	chkRetroArchQuit = new gcn::CheckBox("Use RetroArch Quit Button");
	chkRetroArchQuit->setId("chkRetroArchQuit");
	chkRetroArchQuit->addActionListener(miscActionListener);

	chkRetroArchMenu = new gcn::CheckBox("Use RetroArch Menu Button");
	chkRetroArchMenu->setId("chkRetroArchMenu");
	chkRetroArchMenu->addActionListener(miscActionListener);

	chkRetroArchReset = new gcn::CheckBox("Use RetroArch Reset Button");
	chkRetroArchReset->setId("chkRetroArchReset");
	chkRetroArchReset->addActionListener(miscActionListener);

	//chkRetroArchSavestate = new gcn::CheckBox("Use RetroArch State Controls");
	//chkRetroArchSavestate->setId("RetroArchState");
	//chkRetroArchSavestate->addActionListener(miscActionListener);

	chkBSDSocket = new gcn::CheckBox("bsdsocket.library");
	chkBSDSocket->setId("chkBSDSocket");
	chkBSDSocket->addActionListener(miscActionListener);

	chkMasterWP = new gcn::CheckBox("Master floppy write protection");
	chkMasterWP->setId("chkMasterWP");
	chkMasterWP->addActionListener(miscActionListener);

	chkClipboardSharing = new gcn::CheckBox("Clipboard sharing");
	chkClipboardSharing->setId("chkClipboardSharing");
	chkClipboardSharing->addActionListener(miscActionListener);

	chkAllowHostRun = new gcn::CheckBox("Allow host-run");
	chkAllowHostRun->setId("chkAllowHostRun");
	chkAllowHostRun->addActionListener(miscActionListener);

	lblNumLock = new gcn::Label("NumLock:");
	lblNumLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_num = new gcn::DropDown(&KBDLedList);
	cboKBDLed_num->setBaseColor(gui_baseCol);
	cboKBDLed_num->setBackgroundColor(colTextboxBackground);
	cboKBDLed_num->setId("cboNumlock");
	cboKBDLed_num->addActionListener(miscActionListener);

	lblScrLock = new gcn::Label("ScrollLock:");
	lblScrLock->setAlignment(gcn::Graphics::RIGHT);
	cboKBDLed_scr = new gcn::DropDown(&KBDLedList);
	cboKBDLed_scr->setBaseColor(gui_baseCol);
	cboKBDLed_scr->setBackgroundColor(colTextboxBackground);
	cboKBDLed_scr->setId("cboScrolllock");
	cboKBDLed_scr->addActionListener(miscActionListener);

	lblOpenGUI = new gcn::Label("Open GUI:");
	lblOpenGUI->setAlignment(gcn::Graphics::RIGHT);
	txtOpenGUI = new gcn::TextField();
	txtOpenGUI->setEnabled(false);
	txtOpenGUI->setSize(105, TEXTFIELD_HEIGHT);
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
	txtKeyForQuit->setSize(105, TEXTFIELD_HEIGHT);
	txtKeyForQuit->setBackgroundColor(colTextboxBackground);
	cmdKeyForQuit = new gcn::Button("...");
	cmdKeyForQuit->setId("KeyForQuit");
	cmdKeyForQuit->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuit->setBaseColor(gui_baseCol);
	cmdKeyForQuit->addActionListener(miscActionListener);

	lblKeyActionReplay = new gcn::Label("Action Replay:");
	lblKeyActionReplay->setAlignment(gcn::Graphics::RIGHT);
	txtKeyActionReplay = new gcn::TextField();
	txtKeyActionReplay->setEnabled(false);
	txtKeyActionReplay->setSize(105, TEXTFIELD_HEIGHT);
	txtKeyActionReplay->setBackgroundColor(colTextboxBackground);
	cmdKeyActionReplay = new gcn::Button("...");
	cmdKeyActionReplay->setId("KeyActionReplay");
	cmdKeyActionReplay->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyActionReplay->setBaseColor(gui_baseCol);
	cmdKeyActionReplay->addActionListener(miscActionListener);

	lblKeyFullScreen = new gcn::Label("FullScreen:");
	lblKeyFullScreen->setAlignment(gcn::Graphics::RIGHT);
	txtKeyFullScreen = new gcn::TextField();
	txtKeyFullScreen->setEnabled(false);
	txtKeyFullScreen->setSize(105, TEXTFIELD_HEIGHT);
	txtKeyFullScreen->setBackgroundColor(colTextboxBackground);
	cmdKeyFullScreen = new gcn::Button("...");
	cmdKeyFullScreen->setId("KeyFullScreen");
	cmdKeyFullScreen->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyFullScreen->setBaseColor(gui_baseCol);
	cmdKeyFullScreen->addActionListener(miscActionListener);

#ifdef SERIAL_PORT
	grpSerialDevice = new gcn::Window("Serial Port");
	grpSerialDevice->setId("grpSerialDevice");

	txtSerialDevice = new gcn::TextField();
	txtSerialDevice->setId("txtSerialDevice");
	txtSerialDevice->addActionListener(miscActionListener);
	txtSerialDevice->addKeyListener(miscKeyListener);

	chkSerialDirect = new gcn::CheckBox("Direct");
	chkSerialDirect->setId("chkSerialDirect");
	chkSerialDirect->addActionListener(miscActionListener);

	chkRTSCTS = new gcn::CheckBox("RTS/CTS");
	chkRTSCTS->setId("chkRTSCTS");
	chkRTSCTS->addActionListener(miscActionListener);

	chkUaeSerial = new gcn::CheckBox("uaeserial.device");
	chkUaeSerial->setId("chkUaeSerial");
	chkUaeSerial->addActionListener(miscActionListener);
#endif

	auto posY = DISTANCE_BORDER;
	category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
	posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkStatusLineRtg, DISTANCE_BORDER, posY);
	posY += chkStatusLineRtg->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);

	posY = DISTANCE_BORDER;
	auto posX = 300;
	category.panel->add(chkRetroArchQuit, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchQuit->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkRetroArchMenu, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchMenu->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkRetroArchReset, posX + DISTANCE_BORDER, posY);
	posY += chkRetroArchReset->getHeight() + DISTANCE_NEXT_Y;
	//category.panel->add(chkRetroArchSavestate, posX + DISTANCE_BORDER, posY);
	
	category.panel->add(chkMouseUntrap, DISTANCE_BORDER, posY);
	category.panel->add(chkClipboardSharing, posX + DISTANCE_BORDER, posY);
	posY += chkMouseUntrap->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkBSDSocket, DISTANCE_BORDER, posY);
	category.panel->add(chkAllowHostRun, posX + DISTANCE_BORDER, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkMasterWP, DISTANCE_BORDER, posY);
	posY += chkMasterWP->getHeight() + DISTANCE_NEXT_Y * 2;

	const auto column2_x = DISTANCE_BORDER + 290;

	category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
	category.panel->add(cboKBDLed_num, DISTANCE_BORDER + lblNumLock->getWidth() + 8, posY);

	category.panel->add(lblScrLock, column2_x, posY);
	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);

	posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblOpenGUI, DISTANCE_BORDER, posY);
	category.panel->add(txtOpenGUI, lblOpenGUI->getX() + lblKeyActionReplay->getWidth() + 8, posY);
	category.panel->add(cmdOpenGUI, txtOpenGUI->getX() + txtOpenGUI->getWidth() + 8, posY);

	category.panel->add(lblKeyForQuit, column2_x, posY);
	category.panel->add(txtKeyForQuit, lblKeyForQuit->getX() + lblKeyFullScreen->getWidth() + 8, posY);
	category.panel->add(cmdKeyForQuit, txtKeyForQuit->getX() + txtKeyForQuit->getWidth() + 8, posY);

	posY += cmdOpenGUI->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblKeyActionReplay, DISTANCE_BORDER, posY);
	category.panel->add(txtKeyActionReplay, lblKeyActionReplay->getX() + lblKeyActionReplay->getWidth() + 8, posY);
	category.panel->add(cmdKeyActionReplay, txtKeyActionReplay->getX() + txtKeyActionReplay->getWidth() + 8, posY);

	category.panel->add(lblKeyFullScreen, column2_x, posY);
	category.panel->add(txtKeyFullScreen, lblKeyFullScreen->getX() + lblKeyFullScreen->getWidth() + 8, posY);
	category.panel->add(cmdKeyFullScreen, txtKeyFullScreen->getX() + txtKeyFullScreen->getWidth() + 8, posY);
	
#ifdef SERIAL_PORT
	posY += cmdKeyActionReplay->getHeight() + DISTANCE_NEXT_Y * 2;

	grpSerialDevice->setPosition(DISTANCE_BORDER, posY);
	grpSerialDevice->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + chkSerialDirect->getHeight() * 5);
	grpSerialDevice->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSerialDevice->setBaseColor(gui_baseCol);
	txtSerialDevice->setSize(grpSerialDevice->getWidth() - chkSerialDirect->getWidth() - chkRTSCTS->getWidth() - DISTANCE_BORDER * 4, TEXTFIELD_HEIGHT);
	txtSerialDevice->setBackgroundColor(colTextboxBackground);
	grpSerialDevice->add(txtSerialDevice, DISTANCE_BORDER, DISTANCE_BORDER);
	grpSerialDevice->add(chkRTSCTS, DISTANCE_BORDER, DISTANCE_BORDER * 3);
	grpSerialDevice->add(chkSerialDirect,chkRTSCTS->getWidth() + chkRTSCTS->getX() + DISTANCE_NEXT_X, DISTANCE_BORDER * 3);
	grpSerialDevice->add(chkUaeSerial, chkSerialDirect->getWidth() + chkSerialDirect->getX() + DISTANCE_NEXT_X, DISTANCE_BORDER * 3);
	category.panel->add(grpSerialDevice);
#endif
	RefreshPanelMisc();
}

void ExitPanelMisc()
{
	delete chkStatusLine;
	delete chkStatusLineRtg;
	delete chkShowGUI;
	delete chkMouseUntrap;

	delete chkRetroArchQuit;
	delete chkRetroArchMenu;
	delete chkRetroArchReset;
	//delete chkRetroArchSaveState;

	delete chkBSDSocket;
	delete chkMasterWP;
	delete chkClipboardSharing;
	delete chkAllowHostRun;

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

	delete lblKeyActionReplay;
	delete txtKeyActionReplay;
	delete cmdKeyActionReplay;

	delete lblKeyFullScreen;
	delete txtKeyFullScreen;
	delete cmdKeyFullScreen;

	delete miscActionListener;
#ifdef SERIAL_PORT
	delete miscKeyListener;

	delete grpSerialDevice;
	delete txtSerialDevice;
	delete chkRTSCTS;
	delete chkSerialDirect;
	delete chkUaeSerial;
#endif
}

void RefreshPanelMisc()
{
	chkStatusLine->setSelected(changed_prefs.leds_on_screen & STATUSLINE_CHIPSET);
	chkStatusLineRtg->setSelected(changed_prefs.leds_on_screen & STATUSLINE_RTG);
	chkShowGUI->setSelected(changed_prefs.start_gui);
	chkMouseUntrap->setSelected(changed_prefs.input_mouse_untrap);

	chkRetroArchQuit->setSelected(changed_prefs.use_retroarch_quit);
	chkRetroArchMenu->setSelected(changed_prefs.use_retroarch_menu);
	chkRetroArchReset->setSelected(changed_prefs.use_retroarch_reset);
	//chkRetroArchSavestate->setSelected(changed_prefs.use_retroarch_statebuttons);

	chkBSDSocket->setEnabled(!emulating);
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	chkMasterWP->setSelected(changed_prefs.floppy_read_only);
	chkClipboardSharing->setSelected(changed_prefs.clipboard_sharing);
	chkAllowHostRun->setSelected(changed_prefs.allow_host_run);

	cboKBDLed_num->setSelected(changed_prefs.kbd_led_num + 1);
	cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr + 1);

	txtOpenGUI->setText(strncmp(changed_prefs.open_gui, "", 1) != 0 ? changed_prefs.open_gui : "Click to map");
	txtKeyForQuit->setText(strncmp(changed_prefs.quit_amiberry, "", 1) != 0
		                       ? changed_prefs.quit_amiberry
		                       : "Click to map");
	txtKeyActionReplay->setText(strncmp(changed_prefs.action_replay, "", 1) != 0
		                            ? changed_prefs.action_replay
		                            : "Click to map");
	txtKeyFullScreen->setText(strncmp(changed_prefs.fullscreen_toggle, "", 1) != 0
		                          ? changed_prefs.fullscreen_toggle
		                          : "Click to map");
#ifdef SERIAL_PORT
	chkRTSCTS->setSelected(changed_prefs.serial_hwctsrts);
	chkSerialDirect->setSelected(changed_prefs.serial_direct);
	txtSerialDevice->setText(changed_prefs.sername);
	chkUaeSerial->setSelected(changed_prefs.uaeserial);
#endif
}

bool HelpPanelMisc(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("\"Status Line\" Shows/Hides the status line indicator.");
	helptext.emplace_back("The first value in the status line shows the idle time of the CPU in %,");
	helptext.emplace_back("the second value is the current frame rate.");
	helptext.emplace_back("When you have a HDD in your Amiga emulation, the HD indicator shows read (blue) and write");
	helptext.emplace_back("(red) access to the HDD. The next values are showing the track number for each disk drive");
	helptext.emplace_back("and indicates disk access.");
	helptext.emplace_back(" ");
	helptext.emplace_back("When you deactivate the option \"Show GUI on startup\" and use this configuration");
	helptext.emplace_back("by specifying it with the command line parameter \"-config=<file>\", ");
	helptext.emplace_back("the emulation starts directly without showing the GUI.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"bsdsocket.library\" enables network functions (i.e. for web browsers in OS3.9).");
	helptext.emplace_back("You don't need to use a TCP stack (e.g. AmiTCP/Genesis/Roadshow) when this option is enabled.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Master floppy drive protection\" will disable all write access to floppy disks.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can set some of the keyboard LEDs to react on drive activity, using the relevant options.");
	helptext.emplace_back(" ");
#ifndef SERIAL_PORT
	helptext.emplace_back("Finally, you can assign the desired hotkeys to Open the GUI, Quit the emulator,");
	helptext.emplace_back("open Action Replay/HRTMon or toggle Fullscreen mode ON/OFF.");
#else
	helptext.emplace_back("You can assign the desired hotkeys to Open the GUI, Quit the emulator,");
	helptext.emplace_back("open Action Replay/HRTMon or toggle Fullscreen mode ON/OFF.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Serial Port emulates the Amiga UART through a hardware based UART device on the host." );
	helptext.emplace_back("For example /dev/ttyUSB0. For use on emulator to emulator use Direct ON and RTS/CTS OFF.");
	helptext.emplace_back("For emulator to physical device configurations the opposite should apply.");
#endif
	return true;
}
