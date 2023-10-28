#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>

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
	EnumListModel(const std::vector<std::pair<T,std::string>> values)
		: m_values(values)
	{

	}

	int getNumberOfElements() override
	{
		return m_values.size();
	}

	int add_element(const char* elem) override
	{
		return 0;
	}

	void clear_elements() override
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
			const std::string button = "";
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
	chkVkEnabled->addActionListener(vkEnabledActionListener);

	hiresChkActionListener = new HiresCheckboxActionListener();
	chkVkHires = new gcn::CheckBox(_T("High-Resolution"));
	chkVkHires->setId("chkVkHires");
	chkVkHires->addActionListener(hiresChkActionListener);

	exitChkActionListener = new ExitCheckboxActionListener();
	chkVkExit = new gcn::CheckBox(_T("Quit button on keyboard"));
	chkVkExit->setId("chkVkExit");
	chkVkExit->addActionListener(exitChkActionListener);

	transparencySldActionListener = new TransparencySliderActionListener();

	lblVkTransparency = new gcn::Label(_T("Transparency:"));
	sldVkTransparency = new gcn::Slider(0, 100);
	sldVkTransparency->setSize(100, SLIDER_HEIGHT);
	sldVkTransparency->setBaseColor(gui_baseCol);
	sldVkTransparency->setMarkerLength(20);
	sldVkTransparency->setStepLength(1);
	sldVkTransparency->setId("sldVkTransparency");
	sldVkTransparency->addActionListener(transparencySldActionListener);
	lblVkTransparencyValue = new gcn::Label("100%");
	lblVkTransparencyValue->setAlignment(gcn::Graphics::LEFT);

	lblVkLanguage = new gcn::Label(_T("Keyboard Layout:"));

	languageListModel = new EnumListModel<VkbdLanguage>(languages);
	languageDrpActionListener = new LanguageDropDownActionListener();
	cboVkLanguage = new gcn::DropDown();
	cboVkLanguage->setSize(120, cboVkLanguage->getHeight());
	cboVkLanguage->setBaseColor(gui_baseCol);
	cboVkLanguage->setBackgroundColor(colTextboxBackground);
	cboVkLanguage->setId("cboVkLanguage");
	cboVkLanguage->setListModel(languageListModel);
	cboVkLanguage->addActionListener(languageDrpActionListener);

	lblVkStyle = new gcn::Label(_T("Style:"));

	styleListModel = new EnumListModel<VkbdStyle>(styles);
	styleDrpActionListener = new StyleDropDownActionListener();
	cboVkStyle = new gcn::DropDown();
	cboVkStyle->setSize(120, cboVkStyle->getHeight());
	cboVkStyle->setBaseColor(gui_baseCol);
	cboVkStyle->setBackgroundColor(colTextboxBackground);
	cboVkStyle->setId("cboVkStyle");
	cboVkStyle->setListModel(styleListModel);
	cboVkStyle->addActionListener(styleDrpActionListener);

	vkHotkeyActionListener = new VkHotkeyActionListener();

	lblVkSetHotkey = new gcn::Label("Toggle button:");
	lblVkSetHotkey->setAlignment(gcn::Graphics::RIGHT);
	txtVkSetHotkey = new gcn::TextField();
	txtVkSetHotkey->setEnabled(false);
	txtVkSetHotkey->setSize(120, TEXTFIELD_HEIGHT);
	txtVkSetHotkey->setBackgroundColor(colTextboxBackground);
	cmdVkSetHotkey = new gcn::Button("...");
	cmdVkSetHotkey->setId("cmdVkSetHotkey");
	cmdVkSetHotkey->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdVkSetHotkey->setBaseColor(gui_baseCol);
	cmdVkSetHotkey->addActionListener(vkHotkeyActionListener);
	cmdVkSetHotkeyClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdVkSetHotkeyClear->setBaseColor(gui_baseCol);
	cmdVkSetHotkeyClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdVkSetHotkeyClear->setId("cmdVkSetHotkeyClear");
	cmdVkSetHotkeyClear->addActionListener(vkHotkeyActionListener);

	chkRetroArchVkbd = new gcn::CheckBox("Use RetroArch Vkbd button");
	chkRetroArchVkbd->setId("chkRetroArchVkbd");
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

void ExitPanelVirtualKeyboard(void)
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

void RefreshPanelVirtualKeyboard(void)
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
	helptext.emplace_back("Virtual keyboard can be used to input key presses using a controller.");
    helptext.emplace_back(" ");
    helptext.emplace_back("Options for the virtual keyboard are configured here:");
    helptext.emplace_back("To show the virtual keyboard, first Enable it here and then map the");
	helptext.emplace_back("'Toggle Virtual Keyboard' custom event to a controller button.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can select a normal or a High-resolution version of the on-screen keyboard.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can optionally have a button on the on-screen keyboard, that Quits Amiberry.");
	helptext.emplace_back("Additionally, you can change the transparency amount, the language and the ");
	helptext.emplace_back("look it will have.");
	return true;
}
