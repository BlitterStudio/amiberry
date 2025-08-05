#include <cstring>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "fsdb_host.h"
#include "statusline.h"

static gcn::ScrollArea* scrlMisc;
static gcn::Window* grpMiscOptions;

static gcn::CheckBox* chkAltTabRelease;
static gcn::CheckBox* chkRetroArchQuit;
static gcn::CheckBox* chkRetroArchMenu;
static gcn::CheckBox* chkRetroArchReset;
static gcn::CheckBox* chkStatusLine;
static gcn::CheckBox* chkStatusLineRtg;
static gcn::CheckBox* chkShowGUI;
static gcn::CheckBox* chkMouseUntrap;
static gcn::CheckBox* chkMasterWP;
static gcn::CheckBox* chkHDReadOnly;
static gcn::CheckBox* chkClipboardSharing;
static gcn::CheckBox* chkAllowNativeCode;
static gcn::CheckBox* chkRCtrlIsRAmiga;
static gcn::CheckBox* chkMainAlwaysOnTop;
static gcn::CheckBox* chkGuiAlwaysOnTop;
static gcn::CheckBox* chkSyncClock;
static gcn::CheckBox* chkResetDelay;
static gcn::CheckBox* chkFasterRTG;
static gcn::CheckBox* chkIllegalMem;
static gcn::CheckBox* chkMinimizeInactive;
static gcn::CheckBox* chkCaptureAlways;
static gcn::CheckBox* chkHideAutoconfig;
static gcn::CheckBox* chkScsiDisable;
static gcn::CheckBox* chkWarpModeReset;

static gcn::Label* lblNumLock;
static gcn::DropDown* cboKBDLed_num;
static gcn::Label* lblScrLock;
static gcn::DropDown* cboKBDLed_scr;
static gcn::Label* lblCapLock;
static gcn::DropDown* cboKBDLed_cap;

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

static gcn::Label* lblKeyRAmiga;
static gcn::TextField* txtKeyRAmiga;
static gcn::Button* cmdKeyRAmiga;
static gcn::ImageButton* cmdKeyRAmigaClear;

static const std::vector<std::string> listValues = { "none", "POWER", "DF0", "DF1", "DF2", "DF3", "HD", "CD" };
static gcn::StringListModel KBDLedList(listValues);

void setHotkeyFieldAndPrefs(const amiberry_hotkey& key, gcn::TextField* field, char* changed_pref, char* curr_pref) {
	// Only set hotkey if we actually got a valid key (not cancelled)
	if (!key.modifiers_string.empty())
	{
		field->setText(key.modifiers_string);
		strcpy(changed_pref, key.modifiers_string.c_str());
		strcpy(curr_pref, key.modifiers_string.c_str());
	}
}

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

		else if (actionEvent.getSource() == chkMainAlwaysOnTop)
			changed_prefs.main_alwaysontop = chkMainAlwaysOnTop->isSelected();

		else if (actionEvent.getSource() == chkGuiAlwaysOnTop)
			changed_prefs.gui_alwaysontop = chkGuiAlwaysOnTop->isSelected();

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

		else if (actionEvent.getSource() == chkScsiDisable)
			changed_prefs.scsidevicedisable = chkScsiDisable->isSelected();

		else if (actionEvent.getSource() == chkWarpModeReset)
			changed_prefs.turbo_boot = chkWarpModeReset->isSelected();

		else if (actionEvent.getSource() == cboKBDLed_num)
			changed_prefs.kbd_led_num = cboKBDLed_num->getSelected() - 1;

		else if (actionEvent.getSource() == cboKBDLed_scr)
			changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected() - 1;

		else if (actionEvent.getSource() == cboKBDLed_cap)
			changed_prefs.kbd_led_cap = cboKBDLed_cap->getSelected() - 1;

		else if (actionEvent.getSource() == cmdKeyOpenGUI)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Open the GUI, or ESC to cancel...", "Cancel");
			setHotkeyFieldAndPrefs(key, txtOpenGUI, changed_prefs.open_gui, currprefs.open_gui);
		}
		else if (actionEvent.getSource() == cmdKeyOpenGUIClear)
		{
			std::string hotkey;
			strcpy(changed_prefs.open_gui, hotkey.c_str());
			strcpy(currprefs.open_gui, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyForQuit)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Quit the emulator, or ESC to cancel...", "Cancel");
			setHotkeyFieldAndPrefs(key, txtKeyForQuit, changed_prefs.quit_amiberry, currprefs.quit_amiberry);
		}
		else if (actionEvent.getSource() == cmdKeyForQuitClear)
		{
			std::string hotkey;
			strcpy(changed_prefs.quit_amiberry, hotkey.c_str());
			strcpy(currprefs.quit_amiberry, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyActionReplay)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Action Replay, or ESC to cancel...", "Cancel");
			setHotkeyFieldAndPrefs(key, txtKeyActionReplay, changed_prefs.action_replay, currprefs.action_replay);
		}
		else if (actionEvent.getSource() == cmdKeyActionReplayClear)
		{
			std::string hotkey;
			strcpy(changed_prefs.action_replay, hotkey.c_str());
			strcpy(currprefs.action_replay, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyFullScreen)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to toggle FullScreen, or ESC to cancel...", "Cancel");
			setHotkeyFieldAndPrefs(key, txtKeyFullScreen, changed_prefs.fullscreen_toggle, currprefs.fullscreen_toggle);
		}
		else if (actionEvent.getSource() == cmdKeyFullScreenClear)
		{
			std::string hotkey;
			strcpy(changed_prefs.fullscreen_toggle, hotkey.c_str());
			strcpy(currprefs.fullscreen_toggle, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyMinimize)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Minimize or ESC to cancel...", "Cancel");
			setHotkeyFieldAndPrefs(key, txtKeyMinimize, changed_prefs.minimize, currprefs.minimize);
		}
		else if (actionEvent.getSource() == cmdKeyMinimizeClear)
		{
			std::string hotkey;
			strcpy(changed_prefs.minimize, hotkey.c_str());
			strcpy(currprefs.minimize, hotkey.c_str());
			RefreshPanelMisc();
		}

		else if (actionEvent.getSource() == cmdKeyRAmiga)
		{
			const auto key = ShowMessageForInput("Press a key", "Press a key to map to Right Amiga or ESC to cancel...", "Cancel");
			setHotkeyFieldAndPrefs(key, txtKeyRAmiga, changed_prefs.right_amiga, currprefs.right_amiga);
		}
		else if (actionEvent.getSource() == cmdKeyRAmigaClear)
		{
			std::string hotkey;
			strcpy(changed_prefs.right_amiga, hotkey.c_str());
			strcpy(currprefs.right_amiga, hotkey.c_str());
			RefreshPanelMisc();
		}
	}
};

MiscActionListener* miscActionListener;

void InitPanelMisc(const config_category& category)
{
	miscActionListener = new MiscActionListener();

	grpMiscOptions = new gcn::Window();
	grpMiscOptions->setId("grpMiscOptions");
	
	chkStatusLine = new gcn::CheckBox("Status Line native");
	chkStatusLine->setId("chkStatusLineNative");
	chkStatusLine->setBaseColor(gui_base_color);
	chkStatusLine->setBackgroundColor(gui_background_color);
	chkStatusLine->setForegroundColor(gui_foreground_color);
	chkStatusLine->addActionListener(miscActionListener);

	chkStatusLineRtg = new gcn::CheckBox("Status Line RTG");
	chkStatusLineRtg->setId("chkStatusLineRtg");
	chkStatusLineRtg->setBaseColor(gui_base_color);
	chkStatusLineRtg->setBackgroundColor(gui_background_color);
	chkStatusLineRtg->setForegroundColor(gui_foreground_color);
	chkStatusLineRtg->addActionListener(miscActionListener);

	chkShowGUI = new gcn::CheckBox("Show GUI on startup");
	chkShowGUI->setId("chkShowGUI");
	chkShowGUI->setBaseColor(gui_base_color);
	chkShowGUI->setBackgroundColor(gui_background_color);
	chkShowGUI->setForegroundColor(gui_foreground_color);
	chkShowGUI->addActionListener(miscActionListener);

	chkMouseUntrap = new gcn::CheckBox("Untrap = middle button");
	chkMouseUntrap->setId("chkMouseUntrap");
	chkMouseUntrap->setBaseColor(gui_base_color);
	chkMouseUntrap->setBackgroundColor(gui_background_color);
	chkMouseUntrap->setForegroundColor(gui_foreground_color);
	chkMouseUntrap->addActionListener(miscActionListener);

	chkAltTabRelease = new gcn::CheckBox("Alt-Tab releases control");
	chkAltTabRelease->setId("chkAltTabRelease");
	chkAltTabRelease->setBaseColor(gui_base_color);
	chkAltTabRelease->setBackgroundColor(gui_background_color);
	chkAltTabRelease->setForegroundColor(gui_foreground_color);
	chkAltTabRelease->addActionListener(miscActionListener);
	
	chkRetroArchQuit = new gcn::CheckBox("Use RetroArch Quit Button");
	chkRetroArchQuit->setId("chkRetroArchQuit");
	chkRetroArchQuit->setBaseColor(gui_base_color);
	chkRetroArchQuit->setBackgroundColor(gui_background_color);
	chkRetroArchQuit->setForegroundColor(gui_foreground_color);
	chkRetroArchQuit->addActionListener(miscActionListener);

	chkRetroArchMenu = new gcn::CheckBox("Use RetroArch Menu Button");
	chkRetroArchMenu->setId("chkRetroArchMenu");
	chkRetroArchMenu->setBaseColor(gui_base_color);
	chkRetroArchMenu->setBackgroundColor(gui_background_color);
	chkRetroArchMenu->setForegroundColor(gui_foreground_color);
	chkRetroArchMenu->addActionListener(miscActionListener);

	chkRetroArchReset = new gcn::CheckBox("Use RetroArch Reset Button");
	chkRetroArchReset->setId("chkRetroArchReset");
	chkRetroArchReset->setBaseColor(gui_base_color);
	chkRetroArchReset->setBackgroundColor(gui_background_color);
	chkRetroArchReset->setForegroundColor(gui_foreground_color);
	chkRetroArchReset->addActionListener(miscActionListener);

	chkMasterWP = new gcn::CheckBox("Master floppy write protection");
	chkMasterWP->setId("chkMasterWP");
	chkMasterWP->setBaseColor(gui_base_color);
	chkMasterWP->setBackgroundColor(gui_background_color);
	chkMasterWP->setForegroundColor(gui_foreground_color);
	chkMasterWP->addActionListener(miscActionListener);

	chkHDReadOnly = new gcn::CheckBox("Master harddrive write protection");
	chkHDReadOnly->setId("chkHDReadOnly");
	chkHDReadOnly->setBaseColor(gui_base_color);
	chkHDReadOnly->setBackgroundColor(gui_background_color);
	chkHDReadOnly->setForegroundColor(gui_foreground_color);
	chkHDReadOnly->addActionListener(miscActionListener);
	
	chkClipboardSharing = new gcn::CheckBox("Clipboard sharing");
	chkClipboardSharing->setId("chkClipboardSharing");
	chkClipboardSharing->setBaseColor(gui_base_color);
	chkClipboardSharing->setBackgroundColor(gui_background_color);
	chkClipboardSharing->setForegroundColor(gui_foreground_color);
	chkClipboardSharing->addActionListener(miscActionListener);

	chkRCtrlIsRAmiga = new gcn::CheckBox("RCtrl = RAmiga");
	chkRCtrlIsRAmiga->setId("chkRCtrlIsRAmiga");
	chkRCtrlIsRAmiga->setBaseColor(gui_base_color);
	chkRCtrlIsRAmiga->setBackgroundColor(gui_background_color);
	chkRCtrlIsRAmiga->setForegroundColor(gui_foreground_color);
	chkRCtrlIsRAmiga->addActionListener(miscActionListener);

	chkMainAlwaysOnTop = new gcn::CheckBox("Always on top");
	chkMainAlwaysOnTop->setId("chkMainAlwaysOnTop");
	chkMainAlwaysOnTop->setBaseColor(gui_base_color);
	chkMainAlwaysOnTop->setBackgroundColor(gui_background_color);
	chkMainAlwaysOnTop->setForegroundColor(gui_foreground_color);
	chkMainAlwaysOnTop->addActionListener(miscActionListener);

	chkGuiAlwaysOnTop = new gcn::CheckBox("GUI Always on top");
	chkGuiAlwaysOnTop->setId("chkGuiAlwaysOnTop");
	chkGuiAlwaysOnTop->setBaseColor(gui_base_color);
	chkGuiAlwaysOnTop->setBackgroundColor(gui_background_color);
	chkGuiAlwaysOnTop->setForegroundColor(gui_foreground_color);
	chkGuiAlwaysOnTop->addActionListener(miscActionListener);

	chkSyncClock = new gcn::CheckBox("Synchronize clock");
	chkSyncClock->setId("chkSyncClock");
	chkSyncClock->setBaseColor(gui_base_color);
	chkSyncClock->setBackgroundColor(gui_background_color);
	chkSyncClock->setForegroundColor(gui_foreground_color);
	chkSyncClock->addActionListener(miscActionListener);

	chkResetDelay = new gcn::CheckBox("One second reboot pause");
	chkResetDelay->setId("chkResetDelay");
	chkResetDelay->setBaseColor(gui_base_color);
	chkResetDelay->setBackgroundColor(gui_background_color);
	chkResetDelay->setForegroundColor(gui_foreground_color);
	chkResetDelay->addActionListener(miscActionListener);

	chkFasterRTG = new gcn::CheckBox("Faster RTG");
	chkFasterRTG->setId("chkFasterRTG");
	chkFasterRTG->setBaseColor(gui_base_color);
	chkFasterRTG->setBackgroundColor(gui_background_color);
	chkFasterRTG->setForegroundColor(gui_foreground_color);
	chkFasterRTG->addActionListener(miscActionListener);

	chkAllowNativeCode = new gcn::CheckBox("Allow native code");
	chkAllowNativeCode->setId("chkAllowNativeCode");
	chkAllowNativeCode->setBaseColor(gui_base_color);
	chkAllowNativeCode->setBackgroundColor(gui_background_color);
	chkAllowNativeCode->setForegroundColor(gui_foreground_color);
	chkAllowNativeCode->addActionListener(miscActionListener);

	chkIllegalMem = new gcn::CheckBox("Log illegal memory accesses");
	chkIllegalMem->setId("chkIllegalMem");
	chkIllegalMem->setBaseColor(gui_base_color);
	chkIllegalMem->setBackgroundColor(gui_background_color);
	chkIllegalMem->setForegroundColor(gui_foreground_color);
	chkIllegalMem->addActionListener(miscActionListener);

	chkMinimizeInactive = new gcn::CheckBox("Minimize when focus is lost");
	chkMinimizeInactive->setId("chkMinimizeInactive");
	chkMinimizeInactive->setBaseColor(gui_base_color);
	chkMinimizeInactive->setBackgroundColor(gui_background_color);
	chkMinimizeInactive->setForegroundColor(gui_foreground_color);
	chkMinimizeInactive->addActionListener(miscActionListener);

	chkCaptureAlways = new gcn::CheckBox("Capture mouse when window is activated");
	chkCaptureAlways->setId("chkCaptureAlways");
	chkCaptureAlways->setBaseColor(gui_base_color);
	chkCaptureAlways->setBackgroundColor(gui_background_color);
	chkCaptureAlways->setForegroundColor(gui_foreground_color);
	chkCaptureAlways->addActionListener(miscActionListener);

	chkHideAutoconfig = new gcn::CheckBox("Hide all UAE autoconfig boards");
	chkHideAutoconfig->setId("chkHideAutoconfig");
	chkHideAutoconfig->setBaseColor(gui_base_color);
	chkHideAutoconfig->setBackgroundColor(gui_background_color);
	chkHideAutoconfig->setForegroundColor(gui_foreground_color);
	chkHideAutoconfig->addActionListener(miscActionListener);

	chkScsiDisable = new gcn::CheckBox("A600/A1200/A4000 IDE scsi.device disable");
	chkScsiDisable->setId("chkScsiDisable");
	chkScsiDisable->setBaseColor(gui_base_color);
	chkScsiDisable->setBackgroundColor(gui_background_color);
	chkScsiDisable->setForegroundColor(gui_foreground_color);
	chkScsiDisable->addActionListener(miscActionListener);

	chkWarpModeReset = new gcn::CheckBox("Warp mode reset");
	chkWarpModeReset->setId("chkWarpModeReset");
	chkWarpModeReset->setBaseColor(gui_base_color);
	chkWarpModeReset->setBackgroundColor(gui_background_color);
	chkWarpModeReset->setForegroundColor(gui_foreground_color);
	chkWarpModeReset->addActionListener(miscActionListener);
	
	lblNumLock = new gcn::Label("NumLock:");
	lblNumLock->setAlignment(gcn::Graphics::Right);
	cboKBDLed_num = new gcn::DropDown(&KBDLedList);
	cboKBDLed_num->setBaseColor(gui_base_color);
	cboKBDLed_num->setBackgroundColor(gui_background_color);
	cboKBDLed_num->setForegroundColor(gui_foreground_color);
	cboKBDLed_num->setSelectionColor(gui_selection_color);
	cboKBDLed_num->setId("cboNumlock");
	cboKBDLed_num->addActionListener(miscActionListener);

	lblScrLock = new gcn::Label("ScrollLock:");
	lblScrLock->setAlignment(gcn::Graphics::Right);
	cboKBDLed_scr = new gcn::DropDown(&KBDLedList);
	cboKBDLed_scr->setBaseColor(gui_base_color);
	cboKBDLed_scr->setBackgroundColor(gui_background_color);
	cboKBDLed_scr->setForegroundColor(gui_foreground_color);
	cboKBDLed_scr->setSelectionColor(gui_selection_color);
	cboKBDLed_scr->setId("cboScrolllock");
	cboKBDLed_scr->addActionListener(miscActionListener);

	lblCapLock = new gcn::Label("CapsLock:");
	lblCapLock->setAlignment(gcn::Graphics::Left);
	cboKBDLed_cap = new gcn::DropDown(&KBDLedList);
	cboKBDLed_cap->setBaseColor(gui_base_color);
	cboKBDLed_cap->setBackgroundColor(gui_background_color);
	cboKBDLed_cap->setForegroundColor(gui_foreground_color);
	cboKBDLed_cap->setSelectionColor(gui_selection_color);
	cboKBDLed_cap->setId("cboCapsLock");
	cboKBDLed_cap->addActionListener(miscActionListener);

	lblOpenGUI = new gcn::Label("Open GUI:");
	lblOpenGUI->setAlignment(gcn::Graphics::Right);
	txtOpenGUI = new gcn::TextField();
	txtOpenGUI->setEnabled(false);
	txtOpenGUI->setSize(120, TEXTFIELD_HEIGHT);
	txtOpenGUI->setBaseColor(gui_base_color);
	txtOpenGUI->setBackgroundColor(gui_background_color);
	txtOpenGUI->setForegroundColor(gui_foreground_color);
	cmdKeyOpenGUI = new gcn::Button("...");
	cmdKeyOpenGUI->setId("cmdKeyOpenGUI");
	cmdKeyOpenGUI->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyOpenGUI->setBaseColor(gui_base_color);
	cmdKeyOpenGUI->setForegroundColor(gui_foreground_color);
	cmdKeyOpenGUI->addActionListener(miscActionListener);
	cmdKeyOpenGUIClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdKeyOpenGUIClear->setBaseColor(gui_base_color);
	cmdKeyOpenGUIClear->setForegroundColor(gui_foreground_color);
	cmdKeyOpenGUIClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyOpenGUIClear->setId("cmdKeyOpenGUIClear");
	cmdKeyOpenGUIClear->addActionListener(miscActionListener);

	lblKeyForQuit = new gcn::Label("Quit Key:");
	lblKeyForQuit->setAlignment(gcn::Graphics::Right);
	txtKeyForQuit = new gcn::TextField();
	txtKeyForQuit->setEnabled(false);
	txtKeyForQuit->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyForQuit->setBaseColor(gui_base_color);
	txtKeyForQuit->setBackgroundColor(gui_background_color);
	txtKeyForQuit->setForegroundColor(gui_foreground_color);
	cmdKeyForQuit = new gcn::Button("...");
	cmdKeyForQuit->setId("cmdKeyForQuit");
	cmdKeyForQuit->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuit->setBaseColor(gui_base_color);
	cmdKeyForQuit->setForegroundColor(gui_foreground_color);
	cmdKeyForQuit->addActionListener(miscActionListener);
	cmdKeyForQuitClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdKeyForQuitClear->setBaseColor(gui_base_color);
	cmdKeyForQuitClear->setForegroundColor(gui_foreground_color);
	cmdKeyForQuitClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyForQuitClear->setId("cmdKeyForQuitClear");
	cmdKeyForQuitClear->addActionListener(miscActionListener);

	lblKeyActionReplay = new gcn::Label("Action Replay:");
	lblKeyActionReplay->setAlignment(gcn::Graphics::Right);
	txtKeyActionReplay = new gcn::TextField();
	txtKeyActionReplay->setEnabled(false);
	txtKeyActionReplay->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyActionReplay->setBaseColor(gui_base_color);
	txtKeyActionReplay->setBackgroundColor(gui_background_color);
	txtKeyActionReplay->setForegroundColor(gui_foreground_color);
	cmdKeyActionReplay = new gcn::Button("...");
	cmdKeyActionReplay->setId("cmdKeyActionReplay");
	cmdKeyActionReplay->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyActionReplay->setBaseColor(gui_base_color);
	cmdKeyActionReplay->setForegroundColor(gui_foreground_color);
	cmdKeyActionReplay->addActionListener(miscActionListener);
	cmdKeyActionReplayClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdKeyActionReplayClear->setBaseColor(gui_base_color);
	cmdKeyActionReplayClear->setForegroundColor(gui_foreground_color);
	cmdKeyActionReplayClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyActionReplayClear->setId("cmdKeyActionReplayClear");
	cmdKeyActionReplayClear->addActionListener(miscActionListener);

	lblKeyFullScreen = new gcn::Label("FullScreen:");
	lblKeyFullScreen->setAlignment(gcn::Graphics::Right);
	txtKeyFullScreen = new gcn::TextField();
	txtKeyFullScreen->setEnabled(false);
	txtKeyFullScreen->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyFullScreen->setBaseColor(gui_base_color);
	txtKeyFullScreen->setBackgroundColor(gui_background_color);
	txtKeyFullScreen->setForegroundColor(gui_foreground_color);
	cmdKeyFullScreen = new gcn::Button("...");
	cmdKeyFullScreen->setId("cmdKeyFullScreen");
	cmdKeyFullScreen->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyFullScreen->setBaseColor(gui_base_color);
	cmdKeyFullScreen->setForegroundColor(gui_foreground_color);
	cmdKeyFullScreen->addActionListener(miscActionListener);
	cmdKeyFullScreenClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdKeyFullScreenClear->setBaseColor(gui_base_color);
	cmdKeyFullScreenClear->setForegroundColor(gui_foreground_color);
	cmdKeyFullScreenClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyFullScreenClear->setId("cmdKeyFullScreenClear");
	cmdKeyFullScreenClear->addActionListener(miscActionListener);

	lblKeyMinimize = new gcn::Label("Minimize:");
	lblKeyMinimize->setAlignment(gcn::Graphics::Right);
	txtKeyMinimize = new gcn::TextField();
	txtKeyMinimize->setEnabled(false);
	txtKeyMinimize->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyMinimize->setBaseColor(gui_base_color);
	txtKeyMinimize->setBackgroundColor(gui_background_color);
	txtKeyMinimize->setForegroundColor(gui_foreground_color);
	cmdKeyMinimize = new gcn::Button("...");
	cmdKeyMinimize->setId("cmdKeyMinimize");
	cmdKeyMinimize->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyMinimize->setBaseColor(gui_base_color);
	cmdKeyMinimize->setForegroundColor(gui_foreground_color);
	cmdKeyMinimize->addActionListener(miscActionListener);
	cmdKeyMinimizeClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdKeyMinimizeClear->setBaseColor(gui_base_color);
	cmdKeyMinimizeClear->setForegroundColor(gui_foreground_color);
	cmdKeyMinimizeClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyMinimizeClear->setId("cmdKeyMinimizeClear");
	cmdKeyMinimizeClear->addActionListener(miscActionListener);

	lblKeyRAmiga = new gcn::Label("Right Amiga:");
	lblKeyRAmiga->setAlignment(gcn::Graphics::Right);
	txtKeyRAmiga = new gcn::TextField();
	txtKeyRAmiga->setEnabled(false);
	txtKeyRAmiga->setSize(120, TEXTFIELD_HEIGHT);
	txtKeyRAmiga->setBaseColor(gui_base_color);
	txtKeyRAmiga->setBackgroundColor(gui_background_color);
	txtKeyRAmiga->setForegroundColor(gui_foreground_color);
	cmdKeyRAmiga = new gcn::Button("...");
	cmdKeyRAmiga->setId("cmdKeyRAmiga");
	cmdKeyRAmiga->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyRAmiga->setBaseColor(gui_base_color);
	cmdKeyRAmiga->setForegroundColor(gui_foreground_color);
	cmdKeyRAmiga->addActionListener(miscActionListener);
	cmdKeyRAmigaClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdKeyRAmigaClear->setBaseColor(gui_base_color);
	cmdKeyRAmigaClear->setForegroundColor(gui_foreground_color);
	cmdKeyRAmigaClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdKeyRAmigaClear->setId("cmdKeyRAmigaClear");
	cmdKeyRAmigaClear->addActionListener(miscActionListener);

	int posY = DISTANCE_BORDER;
	grpMiscOptions->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpMiscOptions->setBaseColor(gui_base_color);
	grpMiscOptions->setForegroundColor(gui_foreground_color);
	grpMiscOptions->add(chkMouseUntrap, DISTANCE_BORDER, posY);
	posY += chkMouseUntrap->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkShowGUI, DISTANCE_BORDER, posY);
	posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;
	// Use CTRL-F11 to quit
	// Don't show taskbar button
	// Don't show notification icon
	grpMiscOptions->add(chkMainAlwaysOnTop, DISTANCE_BORDER, posY);
	posY += chkMainAlwaysOnTop->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkGuiAlwaysOnTop, DISTANCE_BORDER, posY);
	posY += chkGuiAlwaysOnTop->getHeight() + DISTANCE_NEXT_Y;
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
	grpMiscOptions->add(chkScsiDisable, DISTANCE_BORDER, posY);
	posY += chkScsiDisable->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkAltTabRelease, DISTANCE_BORDER, posY);
	posY += chkAltTabRelease->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkWarpModeReset, DISTANCE_BORDER, posY);
	posY += chkWarpModeReset->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkRetroArchQuit, DISTANCE_BORDER, posY);
	posY += chkRetroArchQuit->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkRetroArchMenu, DISTANCE_BORDER, posY);
	posY += chkRetroArchMenu->getHeight() + DISTANCE_NEXT_Y;
	grpMiscOptions->add(chkRetroArchReset, DISTANCE_BORDER, posY);
	grpMiscOptions->setTitleBarHeight(1);
	grpMiscOptions->setSize(category.panel->getWidth() - category.panel->getWidth() / 3 - 40, 800);

	scrlMisc = new gcn::ScrollArea(grpMiscOptions);
	scrlMisc->setId("scrlMisc");
	scrlMisc->setBackgroundColor(gui_base_color);
	scrlMisc->setBaseColor(gui_base_color);
	scrlMisc->setForegroundColor(gui_foreground_color);
	scrlMisc->setWidth(category.panel->getWidth() - (category.panel->getWidth() / 3) - 27);
	scrlMisc->setHeight(600);
	scrlMisc->setFrameSize(1);
	scrlMisc->setFocusable(true);
	category.panel->add(scrlMisc, DISTANCE_BORDER, DISTANCE_BORDER);

	const auto column2_x = scrlMisc->getWidth() + 20;
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
	posY += cmdKeyMinimizeClear->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblKeyRAmiga, column2_x, posY);
	posY += lblKeyRAmiga->getHeight() + 8;
	category.panel->add(txtKeyRAmiga, lblKeyRAmiga->getX(), posY);
	category.panel->add(cmdKeyRAmiga, txtKeyRAmiga->getX() + txtKeyRAmiga->getWidth() + 8, posY);
	category.panel->add(cmdKeyRAmigaClear, cmdKeyRAmiga->getX() + cmdKeyRAmiga->getWidth() + 8, posY);
	posY += cmdKeyRAmiga->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblNumLock, column2_x, posY);
	category.panel->add(cboKBDLed_num, lblNumLock->getX() + lblScrLock->getWidth() + 8, posY);
	posY += cboKBDLed_num->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblScrLock, column2_x, posY);
	category.panel->add(cboKBDLed_scr, lblScrLock->getX() + lblScrLock->getWidth() + 8, posY);
	posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblCapLock, column2_x, posY);
	category.panel->add(cboKBDLed_cap, lblCapLock->getX() + lblScrLock->getWidth() + 8, posY);

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
	delete chkMasterWP;
	delete chkHDReadOnly;
	delete chkClipboardSharing;
	delete chkRCtrlIsRAmiga;
	delete chkMainAlwaysOnTop;
	delete chkGuiAlwaysOnTop;
	delete chkSyncClock;
	delete chkResetDelay;
	delete chkFasterRTG;
	delete chkAllowNativeCode;
	delete chkIllegalMem;
	delete chkMinimizeInactive;
	delete chkCaptureAlways;
	delete chkHideAutoconfig;
	delete chkScsiDisable;
	delete chkWarpModeReset;
	
	delete lblScrLock;
	delete lblNumLock;
	delete lblCapLock;
	delete cboKBDLed_num;
	delete cboKBDLed_scr;
	delete cboKBDLed_cap;

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

	delete lblKeyRAmiga;
	delete txtKeyRAmiga;
	delete cmdKeyRAmiga;
	delete cmdKeyRAmigaClear;

	delete miscActionListener;

	delete grpMiscOptions;
	delete scrlMisc;
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
	chkMasterWP->setSelected(changed_prefs.floppy_read_only);
	chkHDReadOnly->setSelected(changed_prefs.harddrive_read_only);
	chkClipboardSharing->setSelected(changed_prefs.clipboard_sharing);
	chkRCtrlIsRAmiga->setSelected(changed_prefs.right_control_is_right_win_key);
	chkMainAlwaysOnTop->setSelected(changed_prefs.main_alwaysontop);
	chkGuiAlwaysOnTop->setSelected(changed_prefs.gui_alwaysontop);
	chkSyncClock->setSelected(changed_prefs.tod_hack);
	chkResetDelay->setSelected(changed_prefs.reset_delay);
	chkFasterRTG->setSelected(changed_prefs.picasso96_nocustom);
	chkAllowNativeCode->setSelected(changed_prefs.native_code);
	chkIllegalMem->setSelected(changed_prefs.illegal_mem);
	chkMinimizeInactive->setSelected(changed_prefs.minimize_inactive);
	chkCaptureAlways->setSelected(changed_prefs.capture_always);
	chkHideAutoconfig->setSelected(changed_prefs.uae_hide_autoconfig);
	chkScsiDisable->setSelected(changed_prefs.scsidevicedisable);
	chkWarpModeReset->setSelected(changed_prefs.turbo_boot);
	
	cboKBDLed_num->setSelected(changed_prefs.kbd_led_num + 1);
	cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr + 1);
	cboKBDLed_cap->setSelected(changed_prefs.kbd_led_cap + 1);

	txtOpenGUI->setText(strncmp(changed_prefs.open_gui, "", 1) != 0 
		? changed_prefs.open_gui
		: "Click to map");
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
	txtKeyRAmiga->setText(strncmp(changed_prefs.right_amiga, "", 1) != 0
		? changed_prefs.right_amiga
		: "Click to map");
}

bool HelpPanelMisc(std::vector<std::string>& helptext)
{
	helptext.clear();
        helptext.emplace_back("In this panel you can control various emulator options that aren't covered by any of");
        helptext.emplace_back("the previous panels. Changes made here can be saved to a .uae config file and will");
        helptext.emplace_back("override any defaults that may exist in the amiberry.conf file. The naming of these");
        helptext.emplace_back("options makes their usage reasonably apparent, however they are detailed in full");
        helptext.emplace_back("below for further clarity.");
        helptext.emplace_back(" ");
        helptext.emplace_back("On the right of this panel you can assign some common keys or buttons, for use with");
        helptext.emplace_back("the emulation control:");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Open GUI: The key to open the main Amiberry GUI. The F12 key is the default.");
        helptext.emplace_back("- Quit Key: A key to quit Amiberry immediately. No default is set.");
        helptext.emplace_back("- Action Replay: A key to open Action Replay/HRTmon. The Pause key is the default.");
        helptext.emplace_back("- FullScreen: A key to toggle Fullscreen/Fullwindow/Windowed mode. No default is set.");
        helptext.emplace_back("- Minimize: A key to minimize Amiberry. No default is set.");
        helptext.emplace_back(" ");
        helptext.emplace_back("Below that, you can assign your keyboard's LEDs to blink and indicate the activity");
        helptext.emplace_back("and status of various functions within the emulation. The available functions are:");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Power: Power LED (on/off)");
        helptext.emplace_back("- DF0-DF3: drive activity (will blink when the drive is reading/writing)");
        helptext.emplace_back("- HD: Hard Drive activity (will blink when the drive is reading/writing)");
        helptext.emplace_back("- CD: CD Drive activity (will blink when the CD drive is reading)");
        helptext.emplace_back(" ");
        helptext.emplace_back("The descriptions of the various emulator options available here are;");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Untrap = middle button: This option enables the use of the mouse middle button to");
        helptext.emplace_back("  release control of the mouse pointer in Amiberry. If enabled, the middle mouse");
        helptext.emplace_back("  button cannot be used under AmigaOS.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Show GUI on startup: This option controls if the GUI should be shown on startup,");
        helptext.emplace_back("  when a configuration file is loaded. If you want to start emulating immediately");
        helptext.emplace_back("  when loading a config file, you can disable this option.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Always on top: Set Amiberry window to be always on top of other windows.");
        helptext.emplace_back(" ");
		helptext.emplace_back("- GUI Always on top: Set Amiberry GUI window to be always on top of other windows.");
		helptext.emplace_back("  When using Fullscreen mode in x11, enable this option to promote the GUI window.");
		helptext.emplace_back(" ");
        helptext.emplace_back("- Synchronize clock: Syncs the Amiga clock to the host time.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- One second reboot pause: This option inserts a one-second delay, during reboots.");
        helptext.emplace_back("  Useful if the emulation is too fast, and you want to enter the Early Startup Menu");
        helptext.emplace_back("  by holding down both mouse buttons.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Faster RTG: This option will skip parts of the custom chipset emulation, when RTG");
        helptext.emplace_back("  modes are active. This will help performance with RTG modes, and is the default.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Clipboard sharing: This option allows you to share the clipboard between AmigaOS");
        helptext.emplace_back("  and the host OS, and you can copy/paste text between the two if this is enabled.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Allow native code: This option enables the support for using \"host-run\" and other");
        helptext.emplace_back("  similar tools, to launch host commands from inside the emulated system. Since this");
        helptext.emplace_back("  can also be a security risk, it's disabled by default.");
        helptext.emplace_back(" ");
        helptext.emplace_back("The Status Line displays information about the emulation activity like sound buffer");
        helptext.emplace_back("and CPU usage, FPS, and disk activity, and on some platforms the temperature of the");
        helptext.emplace_back("SoC/board. You can use the following two options to enable/disable this feature;");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Status Line native: Shows the status line when native (PAL/NTSC) screens are open.");
        helptext.emplace_back("- Status Line RTG: Same as the native option above, this will show the status line");
        helptext.emplace_back("  in RTG modes. If you want it always visible, you can enable both options.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Log illegal memory accesses: If enabled, then illegal memory accesses from the");
        helptext.emplace_back("  emulated environment will be logged in the logfile (if that is enabled).");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Minimize when focus is lost: If enabled, when Amiberry loses focus it will then");
        helptext.emplace_back("  automatically minimize it's window.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Master floppy write protection: This option allows you to centrally write-protect");
        helptext.emplace_back("  all floppy images, to avoid accidentally writing something to them.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Master harddrive write protection: Same as the above option, but for hard drives.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Hide all UAE autoconfig boards: If enabled all UAE-specific boards will be hidden");
        helptext.emplace_back("  from the system in emulation.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- RCtrl = RAmiga: Use the RCtrl as the RAmiga key, in lieu of Right Windows key");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Capture mouse when window is activated: If this option is enabled, the mouse will");
        helptext.emplace_back("  be automatically captured if the Amiberry window is selected.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- A600/A1200/A4000 IDE scsi.device disabled: This option will disable the internal");
        helptext.emplace_back("  IDE scsi.device for those Amiga models, making it faster to boot up.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Alt-Tab releases control: When enabled, you can use Alt-Tab to release control");
        helptext.emplace_back("  from Amiberry. If enabled, that key combination cannot be used under AmigaOS.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Warp mode reset: If enabled, \"Warp mode\" is used to speed-up the reset process.");
        helptext.emplace_back(" ");
        helptext.emplace_back("If a valid RetroArch config is found by Amiberry, the following options apply;");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Use RetroArch Quit button: If enabled, a retroarch mapping for Quit will be valid.");
        helptext.emplace_back("- Use RetroArch Menu button: Same as above for retroarch mapping for Opening the GUI");
        helptext.emplace_back("- Use RetroArch Reset button: Same as above for the retroarch mapping for Resetting");
        helptext.emplace_back("  the emulation.");
        helptext.emplace_back(" ");

	return true;
}
