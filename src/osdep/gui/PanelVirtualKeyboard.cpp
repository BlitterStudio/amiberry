#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_image.h>
#include <guisan/sdl.hpp>

#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "fsdb_host.h"
#include "vkbd/vkbd.h"

static gcn::CheckBox* chkVkEnabled;
static gcn::CheckBox* chkVkHires;
static gcn::CheckBox* chkVkExit;
static gcn::Slider* sldVkTransparency;
static gcn::Label* lblVkTransparency;
static gcn::Label* lblVkTransparencyValue;
static gcn::Label* lblVkLanguage;
static gcn::DropDown* cboVkLanguage;
static gcn::Label* lblVkStyle;
static gcn::DropDown* cboVkStyle;

static gcn::Label* lblVkSetHotkey;
static gcn::TextField* txtVkSetHotkey;
static gcn::Button* cmdVkSetHotkey;
static gcn::ImageButton* cmdVkSetHotkeyClear;

static gcn::CheckBox* chkRetroArchVkbd;

class TransparencySliderActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.vkbd_transparency = static_cast<int>(sldVkTransparency->getValue());
		RefreshPanelVirtualKeyboard();
	}
};

class ExitCheckboxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.vkbd_exit = chkVkExit->isSelected();
		RefreshPanelVirtualKeyboard();
	}
};

class HiresCheckboxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.vkbd_hires = chkVkHires->isSelected();
		RefreshPanelVirtualKeyboard();
	}
};

class VkEnabledCheckboxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.vkbd_enabled = chkVkEnabled->isSelected();
		RefreshPanelVirtualKeyboard();
	}
};

const std::vector<std::pair<VkbdLanguage,std::string>> languages {
	std::make_pair(VKBD_LANGUAGE_US, "US"),
	std::make_pair(VKBD_LANGUAGE_FR, "FR"),
	std::make_pair(VKBD_LANGUAGE_UK, "UK"),
	std::make_pair(VKBD_LANGUAGE_GER, "DE")
};

const std::vector<std::pair<VkbdStyle, std::string>> styles {
	std::make_pair(VKBD_STYLE_ORIG, "Original"),
	std::make_pair(VKBD_STYLE_WARM, "Warm"),
	std::make_pair(VKBD_STYLE_COOL, "Cool"),
	std::make_pair(VKBD_STYLE_DARK, "Dark"),
};

template<class T> 
class EnumListModel : public gcn::ListModel
{
private:
	std::vector<std::pair<T,std::string>> m_values{};
public:
	explicit EnumListModel(const std::vector<std::pair<T,std::string>>& values)
		: m_values(values)
	{
	}

	int getNumberOfElements() override
	{
		return m_values.size();
	}

	void add(const std::string& elem) override
	{
	}

	void clear() override
	{
	}
	
	std::string getElementAt(const int i) override
	{
		return m_values.at(i).second;
	}

	T getValueAt(int i)
	{
		return m_values.at(i).first;
	}

	T getValue(const std::string &str)
	{
		return getValueAt(getIndex(str));
	}

	int getIndex(const std::string &str)
	{
		for(int i = 0; i < m_values.size(); ++i)
		{
			if (str == m_values.at(i).second)
			{
				return i;
			}
		}
		return 0;
	}
};

static EnumListModel<VkbdLanguage>* languageListModel;
static EnumListModel<VkbdStyle>* styleListModel;

class LanguageDropDownActionListener : public gcn::ActionListener
{
public:

	void action(const gcn::ActionEvent& actionEvent) override
	{
		const int selected = cboVkLanguage->getSelected();
		_tcscpy(changed_prefs.vkbd_language, languageListModel->getElementAt(selected).c_str());
		RefreshPanelVirtualKeyboard();
	}
};

class StyleDropDownActionListener : public gcn::ActionListener
{
public:

	void action(const gcn::ActionEvent& actionEvent) override
	{
		const int selected = cboVkStyle->getSelected();
		_tcscpy(changed_prefs.vkbd_style, styleListModel->getElementAt(selected).c_str());
		RefreshPanelVirtualKeyboard();
	}

};

class VkHotkeyActionListener : public gcn::ActionListener
{
public:

	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdVkSetHotkey)
		{
			const auto button = ShowMessageForInput("Press a button", "Press a button on your controller to set it as the Hotkey button", "Cancel");
			if (!button.key_name.empty())
			{
				txtVkSetHotkey->setText(button.key_name);
				_tcscpy(changed_prefs.vkbd_toggle, button.key_name.c_str());
				for (int port = 0; port < 2; port++)
				{
					const auto host_joy_id = changed_prefs.jports[port].id - JSEM_JOYS;
					didata* did = &di_joystick[host_joy_id];
					did->mapping.vkbd_button = button.button;
				}
			}
		}
		else if (actionEvent.getSource() == cmdVkSetHotkeyClear)
		{
			const std::string button;
			txtVkSetHotkey->setText(button);
			strcpy(changed_prefs.vkbd_toggle, button.c_str());
			for (int port = 0; port < 2; port++)
			{
				const auto host_joy_id = changed_prefs.jports[port].id - JSEM_JOYS;
				didata* did = &di_joystick[host_joy_id];
				did->mapping.vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;
			}
		}
		else if (actionEvent.getSource() == chkRetroArchVkbd)
		{
			changed_prefs.use_retroarch_vkbd = chkRetroArchVkbd->isSelected();
		}
		RefreshPanelVirtualKeyboard();
		RefreshPanelCustom();
	}
};

static VkEnabledCheckboxActionListener* vkEnabledActionListener;
static HiresCheckboxActionListener* hiresChkActionListener;
static ExitCheckboxActionListener* exitChkActionListener;
static TransparencySliderActionListener* transparencySldActionListener;
static LanguageDropDownActionListener* languageDrpActionListener;
static StyleDropDownActionListener* styleDrpActionListener;
static VkHotkeyActionListener* vkHotkeyActionListener;

void InitPanelVirtualKeyboard(const struct config_category& category)
{
	vkEnabledActionListener = new VkEnabledCheckboxActionListener();
	chkVkEnabled = new gcn::CheckBox(_T("Virtual Keyboard enabled"));
	chkVkEnabled->setId("chkVkEnabled");
	chkVkEnabled->setBaseColor(gui_base_color);
	chkVkEnabled->setBackgroundColor(gui_background_color);
	chkVkEnabled->setForegroundColor(gui_foreground_color);
	chkVkEnabled->addActionListener(vkEnabledActionListener);

	hiresChkActionListener = new HiresCheckboxActionListener();
	chkVkHires = new gcn::CheckBox(_T("High-Resolution"));
	chkVkHires->setId("chkVkHires");
	chkVkHires->setBaseColor(gui_base_color);
	chkVkHires->setBackgroundColor(gui_background_color);
	chkVkHires->setForegroundColor(gui_foreground_color);
	chkVkHires->addActionListener(hiresChkActionListener);

	exitChkActionListener = new ExitCheckboxActionListener();
	chkVkExit = new gcn::CheckBox(_T("Quit button on keyboard"));
	chkVkExit->setId("chkVkExit");
	chkVkExit->setBaseColor(gui_base_color);
	chkVkExit->setBackgroundColor(gui_background_color);
	chkVkExit->setForegroundColor(gui_foreground_color);
	chkVkExit->addActionListener(exitChkActionListener);

	transparencySldActionListener = new TransparencySliderActionListener();

	lblVkTransparency = new gcn::Label(_T("Transparency:"));
	sldVkTransparency = new gcn::Slider(0, 100);
	sldVkTransparency->setSize(100, SLIDER_HEIGHT);
	sldVkTransparency->setBaseColor(gui_base_color);
	sldVkTransparency->setBackgroundColor(gui_background_color);
	sldVkTransparency->setForegroundColor(gui_foreground_color);
	sldVkTransparency->setMarkerLength(20);
	sldVkTransparency->setStepLength(1);
	sldVkTransparency->setId("sldVkTransparency");
	sldVkTransparency->addActionListener(transparencySldActionListener);
	lblVkTransparencyValue = new gcn::Label("100%");
	lblVkTransparencyValue->setAlignment(gcn::Graphics::Left);

	lblVkLanguage = new gcn::Label(_T("Keyboard Layout:"));

	languageListModel = new EnumListModel<VkbdLanguage>(languages);
	languageDrpActionListener = new LanguageDropDownActionListener();
	cboVkLanguage = new gcn::DropDown();
	cboVkLanguage->setSize(120, cboVkLanguage->getHeight());
	cboVkLanguage->setBaseColor(gui_base_color);
	cboVkLanguage->setBackgroundColor(gui_background_color);
	cboVkLanguage->setForegroundColor(gui_foreground_color);
	cboVkLanguage->setSelectionColor(gui_selection_color);
	cboVkLanguage->setId("cboVkLanguage");
	cboVkLanguage->setListModel(languageListModel);
	cboVkLanguage->addActionListener(languageDrpActionListener);

	lblVkStyle = new gcn::Label(_T("Style:"));

	styleListModel = new EnumListModel<VkbdStyle>(styles);
	styleDrpActionListener = new StyleDropDownActionListener();
	cboVkStyle = new gcn::DropDown();
	cboVkStyle->setSize(120, cboVkStyle->getHeight());
	cboVkStyle->setBaseColor(gui_base_color);
	cboVkStyle->setBackgroundColor(gui_background_color);
	cboVkStyle->setForegroundColor(gui_foreground_color);
	cboVkStyle->setSelectionColor(gui_selection_color);
	cboVkStyle->setId("cboVkStyle");
	cboVkStyle->setListModel(styleListModel);
	cboVkStyle->addActionListener(styleDrpActionListener);

	vkHotkeyActionListener = new VkHotkeyActionListener();

	lblVkSetHotkey = new gcn::Label("Toggle button:");
	lblVkSetHotkey->setAlignment(gcn::Graphics::Right);
	txtVkSetHotkey = new gcn::TextField();
	txtVkSetHotkey->setEnabled(false);
	txtVkSetHotkey->setSize(120, TEXTFIELD_HEIGHT);
	txtVkSetHotkey->setBaseColor(gui_base_color);
	txtVkSetHotkey->setBackgroundColor(gui_background_color);
	txtVkSetHotkey->setForegroundColor(gui_foreground_color);
	cmdVkSetHotkey = new gcn::Button("...");
	cmdVkSetHotkey->setId("cmdVkSetHotkey");
	cmdVkSetHotkey->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdVkSetHotkey->setBaseColor(gui_base_color);
	cmdVkSetHotkey->setForegroundColor(gui_foreground_color);
	cmdVkSetHotkey->addActionListener(vkHotkeyActionListener);
	cmdVkSetHotkeyClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdVkSetHotkeyClear->setBaseColor(gui_base_color);
	cmdVkSetHotkeyClear->setForegroundColor(gui_foreground_color);
	cmdVkSetHotkeyClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdVkSetHotkeyClear->setId("cmdVkSetHotkeyClear");
	cmdVkSetHotkeyClear->addActionListener(vkHotkeyActionListener);

	chkRetroArchVkbd = new gcn::CheckBox("Use RetroArch Vkbd button");
	chkRetroArchVkbd->setId("chkRetroArchVkbd");
	chkRetroArchVkbd->setBaseColor(gui_base_color);
	chkRetroArchVkbd->setBackgroundColor(gui_background_color);
	chkRetroArchVkbd->setForegroundColor(gui_foreground_color);
	chkRetroArchVkbd->addActionListener(vkHotkeyActionListener);

	int x = DISTANCE_BORDER;
	int y = DISTANCE_BORDER;
	category.panel->add(chkVkEnabled, x, y);
	y += chkVkEnabled->getHeight() + DISTANCE_NEXT_Y;
    category.panel->add(chkVkHires, x, y);

	y += chkVkHires->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkVkExit, x, y);

	y += chkVkExit->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblVkTransparency, x, y);
	x += lblVkLanguage->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(sldVkTransparency, x, y);
	category.panel->add(lblVkTransparencyValue, sldVkTransparency->getX() + sldVkTransparency->getWidth() + 8, y);

	y += sldVkTransparency->getHeight() + DISTANCE_NEXT_Y;
	x = DISTANCE_BORDER;
	category.panel->add(lblVkLanguage, x, y);
	x += lblVkLanguage->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(cboVkLanguage, x, y);

	y += cboVkLanguage->getHeight() + DISTANCE_NEXT_Y;
	x = DISTANCE_BORDER;
	category.panel->add(lblVkStyle, x, y);
	x += lblVkLanguage->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(cboVkStyle, x, y);

	y += cboVkStyle->getHeight() + DISTANCE_NEXT_Y;
	x = DISTANCE_BORDER;
	category.panel->add(lblVkSetHotkey, x, y);
	x += lblVkLanguage->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(txtVkSetHotkey, x, y);
	category.panel->add(cmdVkSetHotkey, txtVkSetHotkey->getX() + txtVkSetHotkey->getWidth() + 8, y);
	category.panel->add(cmdVkSetHotkeyClear, cmdVkSetHotkey->getX() + cmdVkSetHotkey->getWidth() + 8, y);

	y += txtVkSetHotkey->getHeight() + DISTANCE_NEXT_Y;
	x = DISTANCE_BORDER;
	category.panel->add(chkRetroArchVkbd, x, y);

	RefreshPanelVirtualKeyboard();
}

void ExitPanelVirtualKeyboard()
{
	delete chkVkEnabled;
    delete chkVkHires;
	delete chkVkExit;
	delete sldVkTransparency;
	delete lblVkTransparency;
	delete lblVkTransparencyValue;
	delete lblVkLanguage;
	delete cboVkLanguage;
	delete lblVkStyle;
	delete cboVkStyle;
	delete lblVkSetHotkey;
	delete txtVkSetHotkey;
	delete cmdVkSetHotkey;
	delete cmdVkSetHotkeyClear;
	delete chkRetroArchVkbd;

	delete vkEnabledActionListener;
	delete hiresChkActionListener;
	delete exitChkActionListener;
	delete transparencySldActionListener;
	delete languageDrpActionListener;
	delete styleDrpActionListener;
	delete vkHotkeyActionListener;
	
	delete languageListModel;
	delete styleListModel;
}

void RefreshPanelVirtualKeyboard()
{
	chkVkEnabled->setSelected(changed_prefs.vkbd_enabled);
	chkVkHires->setSelected(changed_prefs.vkbd_hires);
	chkVkExit->setSelected(changed_prefs.vkbd_exit);
	sldVkTransparency->setValue(changed_prefs.vkbd_transparency);
	lblVkTransparencyValue->setCaption(std::to_string(changed_prefs.vkbd_transparency) + "%");
	lblVkTransparencyValue->adjustSize();

	cboVkLanguage->setSelected(languageListModel->getIndex(changed_prefs.vkbd_language));
	cboVkStyle->setSelected(styleListModel->getIndex(changed_prefs.vkbd_style));

	txtVkSetHotkey->setText(changed_prefs.vkbd_toggle);
	chkRetroArchVkbd->setSelected(changed_prefs.use_retroarch_vkbd);

	if (changed_prefs.vkbd_enabled)
	{
		chkVkHires->setEnabled(true);
		chkVkExit->setEnabled(true);
		lblVkTransparency->setEnabled(true);
		sldVkTransparency->setEnabled(true);
		lblVkTransparencyValue->setEnabled(true);
		lblVkLanguage->setEnabled(true);
		cboVkLanguage->setEnabled(true);
		lblVkStyle->setEnabled(true);
		cboVkStyle->setEnabled(true);
		lblVkSetHotkey->setEnabled(true);
		cmdVkSetHotkey->setEnabled(true);
		cmdVkSetHotkeyClear->setEnabled(true);
		chkRetroArchVkbd->setEnabled(true);
	}
	else
	{
		chkVkHires->setEnabled(false);
		chkVkExit->setEnabled(false);
		lblVkTransparency->setEnabled(false);
		sldVkTransparency->setEnabled(false);
		lblVkTransparencyValue->setEnabled(false);
		lblVkLanguage->setEnabled(false);
		cboVkLanguage->setEnabled(false);
		lblVkStyle->setEnabled(false);
		cboVkStyle->setEnabled(false);
		lblVkSetHotkey->setEnabled(false);
		cmdVkSetHotkey->setEnabled(false);
		cmdVkSetHotkeyClear->setEnabled(false);
		chkRetroArchVkbd->setEnabled(false);
	}

	lblVkSetHotkey->setEnabled(!chkRetroArchVkbd->isSelected());
	cmdVkSetHotkey->setEnabled(!chkRetroArchVkbd->isSelected());
	cmdVkSetHotkeyClear->setEnabled(!chkRetroArchVkbd->isSelected());
}

bool HelpPanelVirtualKeyboard(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("In this panel, you can configure the various options for Amiberry's On-screen (virtual)");
	helptext.emplace_back("keyboard. The virtual keyboard can be used to input key presses using a game controller.");
	helptext.emplace_back("This feature is only useful on native screen modes modes (e.g. ECS/AGA), which games use.");
	helptext.emplace_back("For example, you can use it when a game requires you to press a keyboard key, and you only");
	helptext.emplace_back("a controller connected.");
	helptext.emplace_back("The Virtual Keyboard is not usable on RTG screen modes.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Any options you change here can be saved to the currently loaded configuration .uae file.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Virtual Keyboard enabled\": controls if the on-screen keyboard will be enabled or not.");
	helptext.emplace_back(" All the options below this, depend on this being enabled first.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"High-Resolution\": If enabled, Amiberry will use a larger keyboard image when showing");
	helptext.emplace_back(" the on-screen keyboard, which can be useful for high resolution displays.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"("Quit button on keyboard": If enabled, an extra "Quit" button will be shown on the)");
	helptext.emplace_back(" on-screen keyboard, allowing you to Quit the emulator from there.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Transparency\": The amount of transparency the on-screen keyboard will have when shown.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Keyboard Layout\": You can choose the desired keyboard layout, from a range of options.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Style\": You can choose from a few different themes available for the on-screen keyboard.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Toggle button\": You can choose which controller button will be used, to toggle the on-");
	helptext.emplace_back(" screen keyboard off or on.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Please note; In cases where you want Amiberry to always start with the virtual on-screen");
	helptext.emplace_back("keyboard enabled, you can set the above options as defaults in your amiberry.conf file.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Use Retroarch Vkbd button\": This option allows you choose to use the RetroArch mapping");
	helptext.emplace_back(" of the \"vkbd\" button, if found. This is only useful in RetroArch environments.");
	helptext.emplace_back(" ");
	return true;
}
