#include <cstring>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "fsdb_host.h"
#include "statusline.h"

static gcn::ScrollArea* scrollArea;
static gcn::Window* grpMiscOptions;

static gcn::CheckBox* chkAltTabRelease;
static gcn::CheckBox* chkRetroArchQuit;
static gcn::CheckBox* chkRetroArchMenu;
static gcn::CheckBox* chkRetroArchReset;
static gcn::CheckBox* chkStatusLine;
static gcn::CheckBox* chkStatusLineRtg;
static gcn::CheckBox* chkShowGUI;
static gcn::CheckBox* chkMouseUntrap;
static gcn::CheckBox* chkBSDSocket;
static gcn::CheckBox* chkMasterWP;
static gcn::CheckBox* chkHDReadOnly;
static gcn::CheckBox* chkClipboardSharing;
static gcn::CheckBox* chkAllowNativeCode;
static gcn::CheckBox* chkRCtrlIsRAmiga;
static gcn::CheckBox* chkSyncClock;
static gcn::CheckBox* chkResetDelay;
static gcn::CheckBox* chkFasterRTG;
static gcn::CheckBox* chkIllegalMem;
static gcn::CheckBox* chkMinimizeInactive;
static gcn::CheckBox* chkCaptureAlways;
static gcn::CheckBox* chkHideAutoconfig;

static gcn::Label* lblNumLock;
static gcn::DropDown* cboKBDLed_num;
static gcn::Label* lblScrLock;
static gcn::DropDown* cboKBDLed_scr;

static gcn::Label* lblOpenGUI;
static gcn::TextField* txtOpenGUI;
static gcn::Button* cmdKeyOpenGUI;
static gcn::ImageButton* cmdKeyOpenGUIClear;

static gcn::Label* lblKeyForQuit;
static gcn::TextField* txtKeyForQuit;
static gcn::Button* cmdKeyForQuit;
static gcn::ImageButton* cmdKeyForQuitClear;

static gcn::Label* lblKeyActionReplay;
static gcn::TextField* txtKeyActionReplay;
static gcn::Button* cmdKeyActionReplay;
static gcn::ImageButton* cmdKeyActionReplayClear;

static gcn::Label* lblKeyFullScreen;
static gcn::TextField* txtKeyFullScreen;
static gcn::Button* cmdKeyFullScreen;
static gcn::ImageButton* cmdKeyFullScreenClear;

static gcn::Label* lblKeyMinimize;
static gcn::TextField* txtKeyMinimize;
static gcn::Button* cmdKeyMinimize;
static gcn::ImageButton* cmdKeyMinimizeClear;

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

static const char* listValues[] = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD" };
static StringListModel KBDLedList(listValues, 8);

#ifdef SERIAL_PORT
class MiscKeyListener : public gcn::KeyListener
{
public:
	void keyPressed(gcn::KeyEvent& keyEvent) override
	{
		if (keyEvent.getSource() == txtSerialDevice)
		{
			snprintf(changed_prefs.sername, 256, "%s", txtSerialDevice->getText().c_str());
			if (changed_prefs.sername[0])
				changed_prefs.use_serial = true;
			else
				changed_prefs.use_serial = false;
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
		{
			if (chkMouseUntrap->isSelected())
				changed_prefs.input_mouse_untrap |= MOUSEUNTRAP_MIDDLEBUTTON;
			else
				changed_prefs.input_mouse_untrap &= ~MOUSEUNTRAP_MIDDLEBUTTON;
		}

		else if (actionEvent.getSource() == chkAltTabRelease)
		{
			changed_prefs.alt_tab_release = chkAltTabRelease->isSelected();
		}
		
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

		else if (actionEvent.getSource() == chkBSDSocket)
			changed_prefs.socket_emu = chkBSDSocket->isSelected();

		else if (actionEvent.getSource() == chkMasterWP)
		{
			changed_prefs.floppy_read_only = chkMasterWP->isSelected();
			RefreshPanelQuickstart();
			RefreshPanelFloppy();
		}

		else if (actionEvent.getSource() == chkHDReadOnly)
		{
			changed_prefs.harddrive_read_only = chkHDReadOnly->isSelected();
		}

		else if (actionEvent.getSource() == chkClipboardSharing)
			changed_prefs.clipboard_sharing = chkClipboardSharing->isSelected();

		else if (actionEvent.getSource() == chkRCtrlIsRAmiga)
			changed_prefs.right_control_is_right_win_key = chkRCtrlIsRAmiga->isSelected();

		else if (actionEvent.getSource() == chkSyncClock)
			changed_prefs.tod_hack = chkSyncClock->isSelected();

		else if (actionEvent.getSource() == chkResetDelay)
			changed_prefs.reset_delay = chkResetDelay->isSelected();

		else if (actionEvent.getSource() == chkFasterRTG)
			changed_prefs.picasso96_nocustom = chkFasterRTG->isSelected();

		else if (actionEvent.getSource() == chkAllowNativeCode)
			changed_prefs.native_code = chkAllowNativeCode->isSelected();

		else if (actionEvent.getSource() == chkIllegalMem)
			changed_prefs.illegal_mem = chkIllegalMem->isSelected();

		else if (actionEvent.getSource() == chkMinimizeInactive)
			changed_prefs.minimize_inactive = chkMinimizeInactive->isSelected();

		else if (actionEvent.getSource() == chkCaptureAlways)
			changed_prefs.capture_always = chkCaptureAlways->isSelected();

		else if (actionEvent.getSource() == chkHideAutoconfig)
			changed_prefs.uae_hide_autoconfig = chkHideAutoconfig->isSelected();
		
		else if (actionEvent.getSource() == cboKBDLed_num)
			changed_prefs.kbd_led_num = cboKBDLed_num->getSelected() - 1;

		else if (actionEvent.getSource() == cboKBDLed_scr)
			changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected() - 1;

		else if (actionEvent.getSource() == cmdKeyOpenGUI)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Open the GUI, or ESC to cancel...", "Cancel");
			if (!key.key_name.empty())
			{
				std::string hotkey = key.modifiers_string + key.key_name;
				txtOpenGUI->setText(hotkey);
				strcpy(changed_prefs.open_gui, hotkey.c_str());
				strcpy(currprefs.open_gui, hotkey.c_str());
			}
		}
		else if (actionEvent.getSource() == cmdKeyOpenGUIClear)
		{
			std::string hotkey = "";
			strcpy(changed_prefs.open_gui, hotkey.c_str());
			strcpy(currprefs.open_gui, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyForQuit)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Quit the emulator, or ESC to cancel...", "Cancel");
			if (!key.key_name.empty())
			{
				std::string hotkey = key.modifiers_string + key.key_name;
				txtKeyForQuit->setText(hotkey);
				strcpy(changed_prefs.quit_amiberry, hotkey.c_str());
				strcpy(currprefs.quit_amiberry, hotkey.c_str());
			}
		}
		else if (actionEvent.getSource() == cmdKeyForQuitClear)
		{
			std::string hotkey = "";
			strcpy(changed_prefs.quit_amiberry, hotkey.c_str());
			strcpy(currprefs.quit_amiberry, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyActionReplay)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Action Replay, or ESC to cancel...", "Cancel");
			if (!key.key_name.empty())
			{
				std::string hotkey = key.modifiers_string + key.key_name;
				txtKeyActionReplay->setText(hotkey);
				strcpy(changed_prefs.action_replay, hotkey.c_str());
				strcpy(currprefs.action_replay, hotkey.c_str());
			}
		}
		else if (actionEvent.getSource() == cmdKeyActionReplayClear)
		{
			std::string hotkey = "";
			strcpy(changed_prefs.action_replay, hotkey.c_str());
			strcpy(currprefs.action_replay, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyFullScreen)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to toggle FullScreen, or ESC to cancel...", "Cancel");
			if (!key.key_name.empty())
			{
				std::string hotkey = key.modifiers_string + key.key_name;
				txtKeyFullScreen->setText(hotkey);
				strcpy(changed_prefs.fullscreen_toggle, hotkey.c_str());
				strcpy(currprefs.fullscreen_toggle, hotkey.c_str());
			}
		}
		else if (actionEvent.getSource() == cmdKeyFullScreenClear)
		{
			std::string hotkey = "";
			strcpy(changed_prefs.fullscreen_toggle, hotkey.c_str());
			strcpy(currprefs.fullscreen_toggle, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyMinimize)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Minimize or ESC to cancel...", "Cancel");
			if (!key.key_name.empty())
			{
				std::string hotkey = key.modifiers_string + key.key_name;
				txtKeyMinimize->setText(hotkey);
				strcpy(changed_prefs.minimize, hotkey.c_str());
				strcpy(currprefs.minimize, hotkey.c_str());
			}
		}
		else if (actionEvent.getSource() == cmdKeyMinimizeClear)
		{
			std::string hotkey = "";
			strcpy(changed_prefs.minimize, hotkey.c_str());
			strcpy(currprefs.minimize, hotkey.c_str());
			RefreshPanelMisc();
		}
		
#ifdef SERIAL_PORT
		else if (actionEvent.getSource() == txtSerialDevice)
			snprintf(changed_prefs.sername, 256, "%s", txtSerialDevice->getText().c_str());

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

	grpMiscOptions = new gcn::Window();
	grpMiscOptions->setId("grpMiscOptions");
	
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

	chkAltTabRelease = new gcn::CheckBox("Alt-Tab releases control");
	chkAltTabRelease->setId("chkAltTabRelease");
	chkAltTabRelease->addActionListener(miscActionListener);
	
	chkRetroArchQuit = new gcn::CheckBox("Use RetroArch Quit Button");
	chkRetroArchQuit->setId("chkRetroArchQuit");
	chkRetroArchQuit->addActionListener(miscActionListener);

	chkRetroArchMenu = new gcn::CheckBox("Use RetroArch Menu Button");
	chkRetroArchMenu->setId("chkRetroArchMenu");
	chkRetroArchMenu->addActionListener(miscActionListener);

	chkRetroArchReset = new gcn::CheckBox("Use RetroArch Reset Button");
	chkRetroArchReset->setId("chkRetroArchReset");
	chkRetroArchReset->addActionListener(miscActionListener);

	chkBSDSocket = new gcn::CheckBox("bsdsocket.library");
	chkBSDSocket->setId("chkBSDSocket");
	chkBSDSocket->addActionListener(miscActionListener);

	chkMasterWP = new gcn::CheckBox("Master floppy write protection");
	chkMasterWP->setId("chkMasterWP");
	chkMasterWP->addActionListener(miscActionListener);

	chkHDReadOnly = new gcn::CheckBox("Master harddrive write protection");
	chkHDReadOnly->setId("chkHDRO");
	chkHDReadOnly->addActionListener(miscActionListener);
	
	chkClipboardSharing = new gcn::CheckBox("Clipboard sharing");
	chkClipboardSharing->setId("chkClipboardSharing");
	chkClipboardSharing->addActionListener(miscActionListener);

	chkRCtrlIsRAmiga = new gcn::CheckBox("RCtrl = RAmiga");
	chkRCtrlIsRAmiga->setId("chkRCtrlIsRAmiga");
	chkRCtrlIsRAmiga->addActionListener(miscActionListener);

	chkSyncClock = new gcn::CheckBox("Synchronize clock");
	chkSyncClock->setId("chkSyncClock");
	chkSyncClock->addActionListener(miscActionListener);

	chkResetDelay = new gcn::CheckBox("One second reboot pause");
	chkResetDelay->setId("chkResetDelay");
	chkResetDelay->addActionListener(miscActionListener);

	chkFasterRTG = new gcn::CheckBox("Faster RTG");
	chkFasterRTG->setId("chkFasterRTG");
	chkFasterRTG->addActionListener(miscActionListener);

	chkAllowNativeCode = new gcn::CheckBox("Allow native code");
	chkAllowNativeCode->setId("chkAllowNativeCode");
	chkAllowNativeCode->addActionListener(miscActionListener);

	chkIllegalMem = new gcn::CheckBox("Log illegal memory accesses");
	chkIllegalMem->setId("chkIllegalMem");
	chkIllegalMem->addActionListener(miscActionListener);

	chkMinimizeInactive = new gcn::CheckBox("Minimize when focus is lost");
	chkMinimizeInactive->setId("chkMinimizeInactive");
	chkMinimizeInactive->addActionListener(miscActionListener);

	chkCaptureAlways = new gcn::CheckBox("Capture mouse when window is activated");
	chkCaptureAlways->setId("chkCaptureAlways");
	chkCaptureAlways->addActionListener(miscActionListener);

	chkHideAutoconfig = new gcn::CheckBox("Hide all UAE autoconfig boards");
	chkHideAutoconfig->setId("chkHideAutoconfig");
	chkHideAutoconfig->addActionListener(miscActionListener);
	
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
	txtOpenGUI->setSize(120, TEXTFIELD_HEIGHT);
	txtOpenGUI->setBackgroundColor(colTextboxBackground);
	cmdKeyOpenGUI = new gcn::Button("...");
	cmdKeyOpenGUI->setId("cmdKeyOpenGUI");
	cmdKeyOpenGUI->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyOpenGUI->setBaseColor(gui_baseCol);
	cmdKeyOpenGUI->addActionListener(miscActionListener);
	cmdKeyOpenGUIClear = new gcn::ImageButton(prefix_with_application_directory_path("data/delete.png"));
	cmdKeyOpenGUIClear->setBaseColor(gui_baseCol);
	cmdKeyOpenGUIClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyOpenGUIClear->setId("cmdKeyOpenGUIClear");
	cmdKeyOpenGUIClear->addActionListener(miscActionListener);

	lblKeyForQuit = new gcn::Label("Quit Key:");
	lblKeyForQuit->setAlignment(gcn::Graphics::RIGHT);
	txtKeyForQuit = new gcn::TextField();
	txtKeyForQuit->setEnabled(false);
	txtKeyForQuit->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyForQuit->setBackgroundColor(colTextboxBackground);
	cmdKeyForQuit = new gcn::Button("...");
	cmdKeyForQuit->setId("cmdKeyForQuit");
	cmdKeyForQuit->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuit->setBaseColor(gui_baseCol);
	cmdKeyForQuit->addActionListener(miscActionListener);
	cmdKeyForQuitClear = new gcn::ImageButton(prefix_with_application_directory_path("data/delete.png"));
	cmdKeyForQuitClear->setBaseColor(gui_baseCol);
	cmdKeyForQuitClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuitClear->setId("cmdKeyForQuitClear");
	cmdKeyForQuitClear->addActionListener(miscActionListener);

	lblKeyActionReplay = new gcn::Label("Action Replay:");
	lblKeyActionReplay->setAlignment(gcn::Graphics::RIGHT);
	txtKeyActionReplay = new gcn::TextField();
	txtKeyActionReplay->setEnabled(false);
	txtKeyActionReplay->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyActionReplay->setBackgroundColor(colTextboxBackground);
	cmdKeyActionReplay = new gcn::Button("...");
	cmdKeyActionReplay->setId("cmdKeyActionReplay");
	cmdKeyActionReplay->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyActionReplay->setBaseColor(gui_baseCol);
	cmdKeyActionReplay->addActionListener(miscActionListener);
	cmdKeyActionReplayClear = new gcn::ImageButton(prefix_with_application_directory_path("data/delete.png"));
	cmdKeyActionReplayClear->setBaseColor(gui_baseCol);
	cmdKeyActionReplayClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyActionReplayClear->setId("cmdKeyActionReplayClear");
	cmdKeyActionReplayClear->addActionListener(miscActionListener);

	lblKeyFullScreen = new gcn::Label("FullScreen:");
	lblKeyFullScreen->setAlignment(gcn::Graphics::RIGHT);
	txtKeyFullScreen = new gcn::TextField();
	txtKeyFullScreen->setEnabled(false);
	txtKeyFullScreen->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyFullScreen->setBackgroundColor(colTextboxBackground);
	cmdKeyFullScreen = new gcn::Button("...");
	cmdKeyFullScreen->setId("cmdKeyFullScreen");
	cmdKeyFullScreen->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyFullScreen->setBaseColor(gui_baseCol);
	cmdKeyFullScreen->addActionListener(miscActionListener);
	cmdKeyFullScreenClear = new gcn::ImageButton(prefix_with_application_directory_path("data/delete.png"));
	cmdKeyFullScreenClear->setBaseColor(gui_baseCol);
	cmdKeyFullScreenClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyFullScreenClear->setId("cmdKeyFullScreenClear");
	cmdKeyFullScreenClear->addActionListener(miscActionListener);

	lblKeyMinimize = new gcn::Label("Minimize:");
	lblKeyMinimize->setAlignment(gcn::Graphics::RIGHT);
	txtKeyMinimize = new gcn::TextField();
	txtKeyMinimize->setEnabled(false);
	txtKeyMinimize->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyMinimize->setBackgroundColor(colTextboxBackground);
	cmdKeyMinimize = new gcn::Button("...");
	cmdKeyMinimize->setId("cmdKeyMinimize");
	cmdKeyMinimize->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyMinimize->setBaseColor(gui_baseCol);
	cmdKeyMinimize->addActionListener(miscActionListener);
	cmdKeyMinimizeClear = new gcn::ImageButton(prefix_with_application_directory_path("data/delete.png"));
	cmdKeyMinimizeClear->setBaseColor(gui_baseCol);
	cmdKeyMinimizeClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyMinimizeClear->setId("cmdKeyMinimizeClear");
	cmdKeyMinimizeClear->addActionListener(miscActionListener);

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
	grpMiscOptions->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpMiscOptions->setSize(category.panel->getWidth() - category.panel->getWidth() / 3 - DISTANCE_BORDER, 700);
	grpMiscOptions->setBaseColor(gui_baseCol);
	grpMiscOptions->add(chkMouseUntrap, DISTANCE_BORDER, posY);
	posY += chkMouseUntrap->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkShowGUI, DISTANCE_BORDER, posY);
	posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkBSDSocket, DISTANCE_BORDER, posY);
	posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;
	// Use CTRL-F11 to quit
	// Don't show taskbar button
	// Don't show notification icon
	// Main window always on top
	// GUI window always on top
	// Disable screensaver
	grpMiscOptions->add(chkSyncClock, DISTANCE_BORDER, posY);
	posY += chkSyncClock->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkResetDelay, DISTANCE_BORDER, posY);
	posY += chkResetDelay->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkFasterRTG, DISTANCE_BORDER, posY);
	posY += chkFasterRTG->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkClipboardSharing, DISTANCE_BORDER, posY);
	posY += chkClipboardSharing->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkAllowNativeCode, DISTANCE_BORDER, posY);
	posY += chkAllowNativeCode->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkStatusLine, DISTANCE_BORDER, posY);
	posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkStatusLineRtg, DISTANCE_BORDER, posY);
	posY += chkStatusLineRtg->getHeight() + DISTANCE_NEXT_Y;
	// Create winuaelog.txt log
	grpMiscOptions->add(chkIllegalMem, DISTANCE_BORDER, posY);
	posY += chkIllegalMem->getHeight() + DISTANCE_NEXT_Y;
	// Blank unused displays
	// Start mouse uncaptured
	// Start minimized
	grpMiscOptions->add(chkMinimizeInactive, DISTANCE_BORDER, posY);
	posY += chkMinimizeInactive->getHeight() + DISTANCE_NEXT_Y;
	// 100/120Hz VSync black frame insertion
	grpMiscOptions->add(chkMasterWP, DISTANCE_BORDER, posY);
	posY += chkMasterWP->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkHDReadOnly, DISTANCE_BORDER, posY);
	posY += chkHDReadOnly->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkHideAutoconfig, DISTANCE_BORDER, posY);
	posY += chkHideAutoconfig->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkRCtrlIsRAmiga, DISTANCE_BORDER, posY);
	posY += chkRCtrlIsRAmiga->getHeight() + DISTANCE_NEXT_Y;
	// Windows shutdown/logoff notification
	// Warn when attempting to close window
	// Power led dims when audio filter is disabled
	grpMiscOptions->add(chkCaptureAlways, DISTANCE_BORDER, posY);
	posY += chkCaptureAlways->getHeight() + DISTANCE_NEXT_Y;
	// Debug memory space
	grpMiscOptions->add(chkAltTabRelease, DISTANCE_BORDER, posY);
	posY += chkAltTabRelease->getHeight() + DISTANCE_NEXT_X;
	grpMiscOptions->add(chkRetroArchQuit, DISTANCE_BORDER, posY);
	posY += chkRetroArchQuit->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkRetroArchMenu, DISTANCE_BORDER, posY);
	posY += chkRetroArchMenu->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkRetroArchReset, DISTANCE_BORDER, posY);
	
	scrollArea = new gcn::ScrollArea(grpMiscOptions);
	scrollArea->setBackgroundColor(gui_baseCol);
	scrollArea->setBaseColor(gui_baseCol);
	scrollArea->setWidth(category.panel->getWidth() - (category.panel->getWidth() / 3) - SMALL_BUTTON_WIDTH - 8);
	scrollArea->setHeight(300);
	scrollArea->setBorderSize(1);

	category.panel->add(scrollArea, DISTANCE_BORDER, DISTANCE_BORDER);

	const auto column2_x = scrollArea->getWidth() + DISTANCE_NEXT_X * 2;
	posY = DISTANCE_BORDER;
	
	category.panel->add(lblOpenGUI, column2_x, posY);
	posY += lblOpenGUI->getHeight() + 8;
	category.panel->add(txtOpenGUI, lblOpenGUI->getX(), posY);
	category.panel->add(cmdKeyOpenGUI, txtOpenGUI->getX() + txtOpenGUI->getWidth() + 8, posY);
	category.panel->add(cmdKeyOpenGUIClear, cmdKeyOpenGUI->getX() + cmdKeyOpenGUI->getWidth() + 8, posY);
	posY += cmdKeyOpenGUI->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(lblKeyForQuit, column2_x, posY);
	posY += lblKeyForQuit->getHeight() + 8;
	category.panel->add(txtKeyForQuit, lblKeyForQuit->getX(), posY);
	category.panel->add(cmdKeyForQuit, txtKeyForQuit->getX() + txtKeyForQuit->getWidth() + 8, posY);
	category.panel->add(cmdKeyForQuitClear, cmdKeyForQuit->getX() + cmdKeyForQuit->getWidth() + 8, posY);
	posY += cmdKeyForQuit->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblKeyActionReplay, column2_x, posY);
	posY += lblKeyActionReplay->getHeight() + 8;
	category.panel->add(txtKeyActionReplay, lblKeyActionReplay->getX(), posY);
	category.panel->add(cmdKeyActionReplay, txtKeyActionReplay->getX() + txtKeyActionReplay->getWidth() + 8, posY);
	category.panel->add(cmdKeyActionReplayClear, cmdKeyActionReplay->getX() + cmdKeyActionReplay->getWidth() + 8, posY);
	posY += cmdKeyActionReplay->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(lblKeyFullScreen, column2_x, posY);
	posY += lblKeyFullScreen->getHeight() + 8;
	category.panel->add(txtKeyFullScreen, lblKeyFullScreen->getX(), posY);
	category.panel->add(cmdKeyFullScreen, txtKeyFullScreen->getX() + txtKeyFullScreen->getWidth() + 8, posY);
	category.panel->add(cmdKeyFullScreenClear, cmdKeyFullScreen->getX() + cmdKeyFullScreen->getWidth() + 8, posY);
	posY += cmdKeyFullScreen->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblKeyMinimize, column2_x, posY);
	posY += lblKeyMinimize->getHeight() + 8;
	category.panel->add(txtKeyMinimize, lblKeyMinimize->getX(), posY);
	category.panel->add(cmdKeyMinimize, txtKeyMinimize->getX() + txtKeyMinimize->getWidth() + 8, posY);
	category.panel->add(cmdKeyMinimizeClear, cmdKeyMinimize->getX() + cmdKeyMinimize->getWidth() + 8, posY);
	
	posY = scrollArea->getHeight() + DISTANCE_NEXT_Y * 2;	

	category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
	category.panel->add(cboKBDLed_num, DISTANCE_BORDER + lblNumLock->getWidth() + 8, posY);

	category.panel->add(lblScrLock, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);

	posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y * 2;

#ifdef SERIAL_PORT
	grpSerialDevice->setPosition(DISTANCE_BORDER, posY);
	grpSerialDevice->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + chkSerialDirect->getHeight() * 5);
	grpSerialDevice->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpSerialDevice->setBaseColor(gui_baseCol);
	txtSerialDevice->setSize(grpSerialDevice->getWidth() - chkSerialDirect->getWidth() - chkRTSCTS->getWidth() - DISTANCE_BORDER * 4, TEXTFIELD_HEIGHT);
	txtSerialDevice->setBackgroundColor(colTextboxBackground);
	grpSerialDevice->add(txtSerialDevice, DISTANCE_BORDER, DISTANCE_BORDER);
	grpSerialDevice->add(chkRTSCTS, DISTANCE_BORDER, DISTANCE_BORDER * 3);
	grpSerialDevice->add(chkSerialDirect, chkRTSCTS->getWidth() + chkRTSCTS->getX() + DISTANCE_NEXT_X, DISTANCE_BORDER * 3);
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
	delete chkAltTabRelease;
	delete chkRetroArchQuit;
	delete chkRetroArchMenu;
	delete chkRetroArchReset;
	delete chkBSDSocket;
	delete chkMasterWP;
	delete chkHDReadOnly;
	delete chkClipboardSharing;
	delete chkRCtrlIsRAmiga;
	delete chkSyncClock;
	delete chkResetDelay;
	delete chkFasterRTG;
	delete chkAllowNativeCode;
	delete chkIllegalMem;
	delete chkMinimizeInactive;
	delete chkCaptureAlways;
	delete chkHideAutoconfig;
	
	delete lblScrLock;
	delete lblNumLock;
	delete cboKBDLed_num;
	delete cboKBDLed_scr;

	delete lblOpenGUI;
	delete txtOpenGUI;
	delete cmdKeyOpenGUI;
	delete cmdKeyOpenGUIClear;

	delete lblKeyForQuit;
	delete txtKeyForQuit;
	delete cmdKeyForQuit;
	delete cmdKeyForQuitClear;

	delete lblKeyActionReplay;
	delete txtKeyActionReplay;
	delete cmdKeyActionReplay;
	delete cmdKeyActionReplayClear;

	delete lblKeyFullScreen;
	delete txtKeyFullScreen;
	delete cmdKeyFullScreen;
	delete cmdKeyFullScreenClear;

	delete lblKeyMinimize;
	delete txtKeyMinimize;
	delete cmdKeyMinimize;
	delete cmdKeyMinimizeClear;

	delete miscActionListener;
#ifdef SERIAL_PORT
	delete miscKeyListener;

	delete grpSerialDevice;
	delete txtSerialDevice;
	delete chkRTSCTS;
	delete chkSerialDirect;
	delete chkUaeSerial;
#endif
	delete grpMiscOptions;
	delete scrollArea;
}

void RefreshPanelMisc()
{
	chkStatusLine->setSelected(changed_prefs.leds_on_screen & STATUSLINE_CHIPSET);
	chkStatusLineRtg->setSelected(changed_prefs.leds_on_screen & STATUSLINE_RTG);
	chkShowGUI->setSelected(changed_prefs.start_gui);
	chkMouseUntrap->setSelected(changed_prefs.input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON);
	chkAltTabRelease->setSelected(changed_prefs.alt_tab_release);
	chkRetroArchQuit->setSelected(changed_prefs.use_retroarch_quit);
	chkRetroArchMenu->setSelected(changed_prefs.use_retroarch_menu);
	chkRetroArchReset->setSelected(changed_prefs.use_retroarch_reset);
	chkBSDSocket->setEnabled(!emulating);
	chkBSDSocket->setSelected(changed_prefs.socket_emu);
	chkMasterWP->setSelected(changed_prefs.floppy_read_only);
	chkHDReadOnly->setSelected(changed_prefs.harddrive_read_only);
	chkClipboardSharing->setSelected(changed_prefs.clipboard_sharing);
	chkRCtrlIsRAmiga->setSelected(changed_prefs.right_control_is_right_win_key);
	chkSyncClock->setSelected(changed_prefs.tod_hack);
	chkResetDelay->setSelected(changed_prefs.reset_delay);
	chkFasterRTG->setSelected(changed_prefs.picasso96_nocustom);
	chkAllowNativeCode->setSelected(changed_prefs.native_code);
	chkIllegalMem->setSelected(changed_prefs.illegal_mem);
	chkMinimizeInactive->setSelected(changed_prefs.minimize_inactive);
	chkCaptureAlways->setSelected(changed_prefs.capture_always);
	chkHideAutoconfig->setSelected(changed_prefs.uae_hide_autoconfig);
	
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
	txtKeyMinimize->setText(strncmp(changed_prefs.minimize, "", 1) != 0
		? changed_prefs.minimize
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
	helptext.emplace_back("Serial Port emulates the Amiga UART through a hardware based UART device on the host.");
	helptext.emplace_back("For example /dev/ttyUSB0. For use on emulator to emulator use Direct ON and RTS/CTS OFF.");
	helptext.emplace_back("For emulator to physical device configurations the opposite should apply.");
#endif
	return true;
}