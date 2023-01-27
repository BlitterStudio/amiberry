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

static gcn::CheckBox * chkHires;
static gcn::Slider * sldTransparency;
static gcn::Label * lblTransparency;
static gcn::Label * lblLanguage;
static gcn::DropDown * drpLanguage;
static gcn::Label * lblStyle;
static gcn::DropDown * drpStyle;

class TransparencySliderActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		update();
	}

	void update()
	{
		auto value = sldTransparency->getValue();
		changed_prefs.vkbd_transparency = value;
		vkbd_set_transparency(value);
	}
};

class HiresCheckboxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		update();
	}

	void update()
	{
		if (chkHires->isSelected())
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

const std::vector<std::pair<VkbdLanguage,std::string>> languages {
	std::make_pair(VKBD_LANGUAGE_US, "United States"),
	std::make_pair(VKBD_LANGUAGE_FR, "French"),
	std::make_pair(VKBD_LANGUAGE_UK, "United Kingdom"),
	std::make_pair(VKBD_LANGUAGE_GER, "German")
};

const std::vector<std::pair<VkbdStyle, std::string>> styles {
	std::make_pair(VKBD_STYLE_ORIG, "Orig"),
	std::make_pair(VKBD_STYLE_WARM, "Warm"),
	std::make_pair(VKBD_STYLE_COOL, "Cool"),
	std::make_pair(VKBD_STYLE_DARK, "Dark"),
};

template<class T> 
class EnumListModel : public gcn::ListModel
{
private:
	std::vector<std::pair<T,std::string>> m_values;
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

		void update()
		{
			int selected = drpLanguage->getSelected();
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

		void update()
		{
			int selected = drpStyle->getSelected();
			vkbd_set_style(styleListModel->getValueAt(selected));
			_tcscpy(changed_prefs.vkbd_style, styleListModel->getElementAt(selected).c_str());
		}
};

static HiresCheckboxActionListener * hiresChkActionListener;
static TransparencySliderActionListener * transparencySldActionListener;
static LanguageDropDownActionListener * languageDrpActionListener;
static StyleDropDownActionListener * styleDrpActionListener;

void InitPanelVirtualKeyboard(const struct config_category& category)
{
	hiresChkActionListener = new HiresCheckboxActionListener();
	chkHires = new gcn::CheckBox(_T("High-Resolution"));
	chkHires->addActionListener(hiresChkActionListener);

	transparencySldActionListener = new TransparencySliderActionListener();
	sldTransparency = new gcn::Slider(0.0, 1.0);
	sldTransparency->setSize(70, SLIDER_HEIGHT);
	sldTransparency->setBaseColor(gui_baseCol);
	sldTransparency->setMarkerLength(20);
	sldTransparency->setStepLength(0.01);
	sldTransparency->addActionListener(transparencySldActionListener);

	lblTransparency = new gcn::Label(_T("Transparency"));

	lblLanguage = new gcn::Label(_T("Keyboard Layout"));

	languageListModel = new EnumListModel<VkbdLanguage>(languages);
	languageDrpActionListener = new LanguageDropDownActionListener();
	drpLanguage = new gcn::DropDown();
	drpLanguage->setListModel(languageListModel);
	drpLanguage->addActionListener(languageDrpActionListener);

	lblStyle = new gcn::Label(_T("Style"));

	styleListModel = new EnumListModel<VkbdStyle>(styles);
	styleDrpActionListener = new StyleDropDownActionListener();
	drpStyle = new gcn::DropDown();
	drpStyle->setListModel(styleListModel);
	drpStyle->addActionListener(styleDrpActionListener);


	// First row:
	int x = DISTANCE_BORDER;
	int y = DISTANCE_BORDER;
    category.panel->add(chkHires, x, y);

	// Second row
	y += chkHires->getHeight() + DISTANCE_NEXT_Y;
	x = DISTANCE_BORDER;
	category.panel->add(lblTransparency, x, y);
	x += lblTransparency->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(sldTransparency, x, y);

	// Third row
	y += sldTransparency->getHeight() + DISTANCE_NEXT_Y;
	x = DISTANCE_BORDER;
	category.panel->add(lblLanguage, x, y);
	x += lblLanguage->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(drpLanguage, x, y);

	// Fourth row
	y += drpLanguage->getHeight() + DISTANCE_NEXT_Y;
	x = DISTANCE_BORDER;
	category.panel->add(lblStyle, x, y);
	x += lblLanguage->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(drpStyle, x, y);

	RefreshPanelVirtualKeyboard();
}

void ExitPanelVirtualKeyboard(void)
{
    delete chkHires;
	delete sldTransparency;
	delete lblTransparency;
	delete lblLanguage;
	delete drpLanguage;
	delete lblStyle;
	delete drpStyle;

	delete transparencySldActionListener;
	delete hiresChkActionListener;
	delete languageListModel;
	delete languageDrpActionListener;
	delete styleListModel;
	delete styleDrpActionListener;
}

void RefreshPanelVirtualKeyboard(void)
{
	chkHires->setSelected(changed_prefs.vkbd_hires);
	sldTransparency->setValue(changed_prefs.vkbd_transparency);
	drpLanguage->setSelected(languageListModel->getIndex(changed_prefs.vkbd_language));
	drpStyle->setSelected(styleListModel->getIndex(changed_prefs.vkbd_style));

	hiresChkActionListener->update();
	transparencySldActionListener->update();
	languageDrpActionListener->update();
	styleDrpActionListener->update();
	
	gui_update();
}

bool HelpPanelVirtualKeyboard(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Virtual keyboard can be used to input key presses using a controller.");
    helptext.emplace_back(" ");
    helptext.emplace_back("Options for the virtual keyboard are configured here");
    helptext.emplace_back("To enable the virtual keyboard map the 'Toggle Virtual Keyboard' custom event to a controller button");
	return true;
}
