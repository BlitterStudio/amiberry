#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "gui.h"
#include "savestate.h"
#include "gui_handling.h"
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

class TransparencySliderActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		update();
		RefreshPanelVirtualKeyboard();
	}

	static void update()
	{
		const auto value = static_cast<float>(sldVkTransparency->getValue());
		changed_prefs.vkbd_transparency = value;
		vkbd_set_transparency(value);
	}
};

class ExitCheckboxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		update();
		RefreshPanelVirtualKeyboard();
	}

	static void update()
	{
		if (chkVkExit->isSelected())
		{
			changed_prefs.vkbd_exit = true;
			vkbd_set_keyboard_has_exit_button(true);
		}
		else
		{
			changed_prefs.vkbd_exit = false;
			vkbd_set_keyboard_has_exit_button(false);
		}
	}
};

class HiresCheckboxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		update();
		RefreshPanelVirtualKeyboard();
	}

	static void update()
	{
		if (chkVkHires->isSelected())
		{
			changed_prefs.vkbd_hires = true;
			vkbd_set_hires(true);
		}
		else
		{
			changed_prefs.vkbd_hires = false;
			vkbd_set_hires(false);
		}
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
	std::make_pair(VKBD_LANGUAGE_US, "United States"),
	std::make_pair(VKBD_LANGUAGE_FR, "French"),
	std::make_pair(VKBD_LANGUAGE_UK, "United Kingdom"),
	std::make_pair(VKBD_LANGUAGE_GER, "German")
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

static EnumListModel<VkbdLanguage> * languageListModel;
static EnumListModel<VkbdStyle> * styleListModel;

class LanguageDropDownActionListener : public gcn::ActionListener
{
	public:

		void action(const gcn::ActionEvent& actionEvent) override
		{
			update();
		}

		static void update()
		{
			const int selected = cboVkLanguage->getSelected();
			vkbd_set_language(languageListModel->getValueAt(selected));
			_tcscpy(changed_prefs.vkbd_language, languageListModel->getElementAt(selected).c_str());
		}
};

class StyleDropDownActionListener : public gcn::ActionListener
{
	public:

		void action(const gcn::ActionEvent& actionEvent) override
		{
			update();
		}

		static void update()
		{
			const int selected = cboVkStyle->getSelected();
			vkbd_set_style(styleListModel->getValueAt(selected));
			_tcscpy(changed_prefs.vkbd_style, styleListModel->getElementAt(selected).c_str());
		}
};

static VkEnabledCheckboxActionListener* vkEnabledActionListener;
static HiresCheckboxActionListener* hiresChkActionListener;
static ExitCheckboxActionListener* exitChkActionListener;
static TransparencySliderActionListener* transparencySldActionListener;
static LanguageDropDownActionListener* languageDrpActionListener;
static StyleDropDownActionListener* styleDrpActionListener;

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
	sldVkTransparency = new gcn::Slider(0.0, 1.0);
	sldVkTransparency->setSize(100, SLIDER_HEIGHT);
	sldVkTransparency->setBaseColor(gui_baseCol);
	sldVkTransparency->setMarkerLength(20);
	sldVkTransparency->setStepLength(0.01);
	sldVkTransparency->setId("sldVkTransparency");
	sldVkTransparency->addActionListener(transparencySldActionListener);
	lblVkTransparencyValue = new gcn::Label("1.00");
	lblVkTransparencyValue->setAlignment(gcn::Graphics::LEFT);


	lblVkLanguage = new gcn::Label(_T("Keyboard Layout:"));

	languageListModel = new EnumListModel<VkbdLanguage>(languages);
	languageDrpActionListener = new LanguageDropDownActionListener();
	cboVkLanguage = new gcn::DropDown();
	cboVkLanguage->setSize(150, cboVkLanguage->getHeight());
	cboVkLanguage->setBaseColor(gui_baseCol);
	cboVkLanguage->setBackgroundColor(colTextboxBackground);
	cboVkLanguage->setId("cboVkLanguage");
	cboVkLanguage->setListModel(languageListModel);
	cboVkLanguage->addActionListener(languageDrpActionListener);

	lblVkStyle = new gcn::Label(_T("Style:"));

	styleListModel = new EnumListModel<VkbdStyle>(styles);
	styleDrpActionListener = new StyleDropDownActionListener();
	cboVkStyle = new gcn::DropDown();
	cboVkStyle->setSize(150, cboVkStyle->getHeight());
	cboVkStyle->setBaseColor(gui_baseCol);
	cboVkStyle->setBackgroundColor(colTextboxBackground);
	cboVkStyle->setId("cboVkStyle");
	cboVkStyle->setListModel(styleListModel);
	cboVkStyle->addActionListener(styleDrpActionListener);


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

	delete transparencySldActionListener;
	delete hiresChkActionListener;
	delete languageListModel;
	delete languageDrpActionListener;
	delete styleListModel;
	delete styleDrpActionListener;
}

void RefreshPanelVirtualKeyboard(void)
{
	chkVkEnabled->setSelected(changed_prefs.vkbd_enabled);
	chkVkHires->setSelected(changed_prefs.vkbd_hires);
	chkVkExit->setSelected(changed_prefs.vkbd_exit);
	sldVkTransparency->setValue(changed_prefs.vkbd_transparency);
	lblVkTransparencyValue->setCaption(std::to_string(static_cast<int>(changed_prefs.vkbd_transparency * 100)) + "%");
	lblVkTransparencyValue->adjustSize();

	cboVkLanguage->setSelected(languageListModel->getIndex(changed_prefs.vkbd_language));
	cboVkStyle->setSelected(styleListModel->getIndex(changed_prefs.vkbd_style));

	if (changed_prefs.vkbd_enabled)
	{
		chkVkHires->setEnabled(true);
		chkVkExit->setEnabled(true);
		sldVkTransparency->setEnabled(true);
		lblVkTransparencyValue->setEnabled(true);
		cboVkLanguage->setEnabled(true);
		cboVkStyle->setEnabled(true);

		HiresCheckboxActionListener::update();
		ExitCheckboxActionListener::update();
		TransparencySliderActionListener::update();
		LanguageDropDownActionListener::update();
		StyleDropDownActionListener::update();
	}
	else
	{
		chkVkHires->setEnabled(false);
		chkVkExit->setEnabled(false);
		sldVkTransparency->setEnabled(false);
		lblVkTransparencyValue->setEnabled(false);
		cboVkLanguage->setEnabled(false);
		cboVkStyle->setEnabled(false);
	}
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
