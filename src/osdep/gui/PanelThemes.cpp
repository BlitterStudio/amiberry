#include <cstring>
#include <cstdio>
#include <vector>

#include <guisan.hpp>
#include <guisan/sdl.hpp>

#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "StringListModel.h"

static gcn::Label* lblThemeFont;
static gcn::TextField* txtThemeFont;
static gcn::Button* cmdThemeFont;

static gcn::Label* lblThemeFontSize;
static gcn::TextField* txtThemeFontSize;

// Theme font color RGB values
static gcn::Window* grpThemeFontColor;
static gcn::Label* lblThemeFontColorR;
static gcn::Slider* sldThemeFontColorR;
static gcn::Label* lblThemeFontColorRValue;
static gcn::Label* lblThemeFontColorG;
static gcn::Slider* sldThemeFontColorG;
static gcn::Label* lblThemeFontColorGValue;
static gcn::Label* lblThemeFontColorB;
static gcn::Slider* sldThemeFontColorB;
static gcn::Label* lblThemeFontColorBValue;

// Theme base color RGB values
static gcn::Window* grpThemeBaseColor;
static gcn::Label* lblThemeBaseColorR;
static gcn::Slider* sldThemeBaseColorR;
static gcn::Label* lblThemeBaseColorRValue;
static gcn::Label* lblThemeBaseColorG;
static gcn::Slider* sldThemeBaseColorG;
static gcn::Label* lblThemeBaseColorGValue;
static gcn::Label* lblThemeBaseColorB;
static gcn::Slider* sldThemeBaseColorB;
static gcn::Label* lblThemeBaseColorBValue;

// Theme selector inactive color RGB values
static gcn::Window* grpThemeSelectorInactiveColor;
static gcn::Label* lblThemeSelectorInactiveColorR;
static gcn::Slider* sldThemeSelectorInactiveColorR;
static gcn::Label* lblThemeSelectorInactiveColorRValue;
static gcn::Label* lblThemeSelectorInactiveColorG;
static gcn::Slider* sldThemeSelectorInactiveColorG;
static gcn::Label* lblThemeSelectorInactiveColorGValue;
static gcn::Label* lblThemeSelectorInactiveColorB;
static gcn::Slider* sldThemeSelectorInactiveColorB;
static gcn::Label* lblThemeSelectorInactiveColorBValue;

// Theme selector active color RGB values
static gcn::Window* grpThemeSelectorActiveColor;
static gcn::Label* lblThemeSelectorActiveColorR;
static gcn::Slider* sldThemeSelectorActiveColorR;
static gcn::Label* lblThemeSelectorActiveColorRValue;
static gcn::Label* lblThemeSelectorActiveColorG;
static gcn::Slider* sldThemeSelectorActiveColorG;
static gcn::Label* lblThemeSelectorActiveColorGValue;
static gcn::Label* lblThemeSelectorActiveColorB;
static gcn::Slider* sldThemeSelectorActiveColorB;
static gcn::Label* lblThemeSelectorActiveColorBValue;

// Theme selection color RGB values
static gcn::Window* grpThemeSelectionColor;
static gcn::Label* lblThemeSelectionColorR;
static gcn::Slider* sldThemeSelectionColorR;
static gcn::Label* lblThemeSelectionColorRValue;
static gcn::Label* lblThemeSelectionColorG;
static gcn::Slider* sldThemeSelectionColorG;
static gcn::Label* lblThemeSelectionColorGValue;
static gcn::Label* lblThemeSelectionColorB;
static gcn::Slider* sldThemeSelectionColorB;
static gcn::Label* lblThemeSelectionColorBValue;

// Theme text background color RGB values
static gcn::Window* grpThemeTextBgColor;
static gcn::Label* lblThemeTextBgColorR;
static gcn::Slider* sldThemeTextBgColorR;
static gcn::Label* lblThemeTextBgColorRValue;
static gcn::Label* lblThemeTextBgColorG;
static gcn::Slider* sldThemeTextBgColorG;
static gcn::Label* lblThemeTextBgColorGValue;
static gcn::Label* lblThemeTextBgColorB;
static gcn::Slider* sldThemeTextBgColorB;
static gcn::Label* lblThemeTextBgColorBValue;

// Theme foreground color RGB values
static gcn::Window* grpThemeFgColor;
static gcn::Label* lblThemeFgColorR;
static gcn::Slider* sldThemeFgColorR;
static gcn::Label* lblThemeFgColorRValue;
static gcn::Label* lblThemeFgColorG;
static gcn::Slider* sldThemeFgColorG;
static gcn::Label* lblThemeFgColorGValue;
static gcn::Label* lblThemeFgColorB;
static gcn::Slider* sldThemeFgColorB;
static gcn::Label* lblThemeFgColorBValue;

// Save, Reset and Load buttons
static gcn::Button* cmdThemeSave;
static gcn::Button* cmdThemeReset;
static gcn::Button* cmdThemeLoad;

//TODO add a DropDown for the available preset themes

class ThemesActionListener : public gcn::ActionListener
{
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto source = actionEvent.getSource();
		if (source == cmdThemeFont)
		{
			const char* filter[] = { ".ttf", "\0" };
			std::string font = SelectFile("Select font", "/usr/share/fonts/truetype/", filter, false);
			if (!font.empty())
			{
				txtThemeFont->setText(font);
				gui_theme.font_name = font;
			}
		}
        else if (source == cmdThemeReset)
        {
            gui_theme.font_name = "AmigaTopaz.ttf";
            gui_theme.font_size = 15;
            gui_theme.font_color = { 0, 0, 0 };
            gui_theme.base_color = { 170, 170, 170 };
            gui_theme.selector_inactive = { 170, 170, 170 };
            gui_theme.selector_active = { 103, 136, 187 };
            gui_theme.selection_color = { 195, 217, 217 };
            gui_theme.textbox_background = { 220, 220, 220 };
            gui_theme.foreground_color = { 0, 0, 0 };
        }
        else if (source == cmdThemeSave)
        {
            //TODO Save the values to amiberry.conf
        }
        else if (source == cmdThemeLoad)
        {
            //TODO Load the values from amiberry.conf
        }
        else if (source == sldThemeFontColorR)
        {
            gui_theme.font_color.r = static_cast<int>(sldThemeFontColorR->getValue());
            lblThemeFontColorRValue->setCaption(std::to_string(sldThemeFontColorR->getValue()));
        }
        else if (source == sldThemeFontColorG)
        {
            gui_theme.font_color.g = static_cast<int>(sldThemeFontColorG->getValue());
            lblThemeFontColorGValue->setCaption(std::to_string(sldThemeFontColorG->getValue()));
        }
        else if (source == sldThemeFontColorB)
        {
            gui_theme.font_color.b = static_cast<int>(sldThemeFontColorB->getValue());
            lblThemeFontColorBValue->setCaption(std::to_string(sldThemeFontColorB->getValue()));
        }
        else if (source == sldThemeBaseColorR)
        {
            gui_theme.base_color.r = static_cast<int>(sldThemeBaseColorR->getValue());
            lblThemeBaseColorRValue->setCaption(std::to_string(sldThemeBaseColorR->getValue()));
        }
        else if (source == sldThemeBaseColorG)
        {
            gui_theme.base_color.g = static_cast<int>(sldThemeBaseColorG->getValue());
            lblThemeBaseColorGValue->setCaption(std::to_string(sldThemeBaseColorG->getValue()));
        }
        else if (source == sldThemeBaseColorB)
        {
            gui_theme.base_color.b = static_cast<int>(sldThemeBaseColorB->getValue());
            lblThemeBaseColorBValue->setCaption(std::to_string(sldThemeBaseColorB->getValue()));
        }
        else if (source == sldThemeSelectorInactiveColorR)
        {
            gui_theme.selector_inactive.r = static_cast<int>(sldThemeSelectorInactiveColorR->getValue());
            lblThemeSelectorInactiveColorRValue->setCaption(std::to_string(sldThemeSelectorInactiveColorR->getValue()));
        }
        else if (source == sldThemeSelectorInactiveColorG)
        {
            gui_theme.selector_inactive.g = static_cast<int>(sldThemeSelectorInactiveColorG->getValue());
            lblThemeSelectorInactiveColorGValue->setCaption(std::to_string(sldThemeSelectorInactiveColorG->getValue()));
        }
        else if (source == sldThemeSelectorInactiveColorB)
        {
            gui_theme.selector_inactive.b = static_cast<int>(sldThemeSelectorInactiveColorB->getValue());
            lblThemeSelectorInactiveColorBValue->setCaption(std::to_string(sldThemeSelectorInactiveColorB->getValue()));
        }
		else if (source == sldThemeSelectorActiveColorR)
		{
			gui_theme.selector_active.r = static_cast<int>(sldThemeSelectorActiveColorR->getValue());
			lblThemeSelectorActiveColorRValue->setCaption(std::to_string(sldThemeSelectorActiveColorR->getValue()));
		}
		else if (source == sldThemeSelectorActiveColorG)
		{
			gui_theme.selector_active.g = static_cast<int>(sldThemeSelectorActiveColorG->getValue());
			lblThemeSelectorActiveColorGValue->setCaption(std::to_string(sldThemeSelectorActiveColorG->getValue()));
		}
		else if (source == sldThemeSelectorActiveColorB)
		{
			gui_theme.selector_active.b = static_cast<int>(sldThemeSelectorActiveColorB->getValue());
			lblThemeSelectorActiveColorBValue->setCaption(std::to_string(sldThemeSelectorActiveColorB->getValue()));
		}
		else if (source == sldThemeSelectionColorR)
		{
			gui_theme.selection_color.r = static_cast<int>(sldThemeSelectionColorR->getValue());
			lblThemeSelectionColorRValue->setCaption(std::to_string(sldThemeSelectionColorR->getValue()));
		}
		else if (source == sldThemeSelectionColorG)
		{
			gui_theme.selection_color.g = static_cast<int>(sldThemeSelectionColorG->getValue());
			lblThemeSelectionColorGValue->setCaption(std::to_string(sldThemeSelectionColorG->getValue()));
		}
		else if (source == sldThemeSelectionColorB)
		{
			gui_theme.selection_color.b = static_cast<int>(sldThemeSelectionColorB->getValue());
			lblThemeSelectionColorBValue->setCaption(std::to_string(sldThemeSelectionColorB->getValue()));
		}
		else if (source == sldThemeTextBgColorR)
		{
			gui_theme.textbox_background.r = static_cast<int>(sldThemeTextBgColorR->getValue());
			lblThemeTextBgColorRValue->setCaption(std::to_string(sldThemeTextBgColorR->getValue()));
		}
        else if (source == sldThemeTextBgColorG)
        {
            gui_theme.textbox_background.g = static_cast<int>(sldThemeTextBgColorG->getValue());
            lblThemeTextBgColorGValue->setCaption(std::to_string(sldThemeTextBgColorG->getValue()));
        }
		else if (source == sldThemeTextBgColorB)
		{
			gui_theme.textbox_background.b = static_cast<int>(sldThemeTextBgColorB->getValue());
			lblThemeTextBgColorBValue->setCaption(std::to_string(sldThemeTextBgColorB->getValue()));
		}
		else if (source == sldThemeFgColorR)
		{
			gui_theme.foreground_color.r = static_cast<int>(sldThemeFgColorR->getValue());
			lblThemeFgColorRValue->setCaption(std::to_string(sldThemeFgColorR->getValue()));
		}
		else if (source == sldThemeFgColorG)
		{
			gui_theme.foreground_color.g = static_cast<int>(sldThemeFgColorG->getValue());
			lblThemeFgColorGValue->setCaption(std::to_string(sldThemeFgColorG->getValue()));
		}
		else if (source == sldThemeFgColorB)
		{
			gui_theme.foreground_color.b = static_cast<int>(sldThemeFgColorB->getValue());
			lblThemeFgColorBValue->setCaption(std::to_string(sldThemeFgColorB->getValue()));
		}

		RefreshPanelThemes();
	}
};

static ThemesActionListener* themesActionListener;

void InitPanelThemes(const config_category& category)
{
	themesActionListener = new ThemesActionListener();
	constexpr int slider_width = 150;
	constexpr int marker_length = 20;

	lblThemeFont = new gcn::Label("Font:");
	txtThemeFont = new gcn::TextField();
	txtThemeFont->setSize(350, TEXTFIELD_HEIGHT);
	txtThemeFont->setBaseColor(gui_base_color);
	txtThemeFont->setBackgroundColor(gui_textbox_background_color);
	txtThemeFont->setForegroundColor(gui_foreground_color);

	cmdThemeFont = new gcn::Button("...");
	cmdThemeFont->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdThemeFont->setBaseColor(gui_base_color);
	cmdThemeFont->setForegroundColor(gui_foreground_color);
	cmdThemeFont->addActionListener(themesActionListener);

    lblThemeFontSize = new gcn::Label("Size:");
    txtThemeFontSize = new gcn::TextField();
    txtThemeFontSize->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFontSize->setId("txtThemeFontSize");
    txtThemeFontSize->setBaseColor(gui_base_color);
    txtThemeFontSize->setBackgroundColor(gui_textbox_background_color);
    txtThemeFontSize->setForegroundColor(gui_foreground_color);

	// Note: this really should be extracted into a new type of Guisan widget (RGB Slider)

    // Theme font color RGB values
    lblThemeFontColorR = new gcn::Label("R:");
    sldThemeFontColorR = new gcn::Slider(0, 255);
    sldThemeFontColorR->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeFontColorR->setId("sldThemeFontColorR");
    sldThemeFontColorR->setBaseColor(gui_base_color);
    sldThemeFontColorR->setBackgroundColor(gui_textbox_background_color);
    sldThemeFontColorR->setForegroundColor(gui_foreground_color);
    sldThemeFontColorR->setMarkerLength(marker_length);
    sldThemeFontColorR->setStepLength(1);
    sldThemeFontColorR->addActionListener(themesActionListener);
    lblThemeFontColorRValue = new gcn::Label("255");

    lblThemeFontColorG = new gcn::Label("G:");
    sldThemeFontColorG = new gcn::Slider(0, 255);
    sldThemeFontColorG->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeFontColorG->setId("sldThemeFontColorG");
    sldThemeFontColorG->setBaseColor(gui_base_color);
    sldThemeFontColorG->setBackgroundColor(gui_textbox_background_color);
    sldThemeFontColorG->setForegroundColor(gui_foreground_color);
    sldThemeFontColorG->setMarkerLength(marker_length);
    sldThemeFontColorG->setStepLength(1);
    sldThemeFontColorG->addActionListener(themesActionListener);
    lblThemeFontColorGValue = new gcn::Label("255");

    lblThemeFontColorB = new gcn::Label("B:");
    sldThemeFontColorB = new gcn::Slider(0, 255);
    sldThemeFontColorB->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeFontColorB->setId("sldThemeFontColorB");
    sldThemeFontColorB->setBaseColor(gui_base_color);
    sldThemeFontColorB->setBackgroundColor(gui_textbox_background_color);
    sldThemeFontColorB->setForegroundColor(gui_foreground_color);
    sldThemeFontColorB->setMarkerLength(marker_length);
    sldThemeFontColorB->setStepLength(1);
    sldThemeFontColorB->addActionListener(themesActionListener);
    lblThemeFontColorBValue = new gcn::Label("255");

    grpThemeFontColor = new gcn::Window("Font color");
    grpThemeFontColor->setBaseColor(gui_base_color);
    grpThemeFontColor->setForegroundColor(gui_foreground_color);
    grpThemeFontColor->setMovable(false);
    grpThemeFontColor->setTitleBarHeight(TITLEBAR_HEIGHT);
    grpThemeFontColor->add(lblThemeFontColorR, 10, 10);
    grpThemeFontColor->add(sldThemeFontColorR, lblThemeFontColorR->getX() + lblThemeFontColorR->getWidth() + 8, 10);
    grpThemeFontColor->add(lblThemeFontColorRValue, sldThemeFontColorR->getX() + sldThemeFontColorR->getWidth() + 8, 10);
    grpThemeFontColor->add(lblThemeFontColorG, 10, 40);
    grpThemeFontColor->add(sldThemeFontColorG, sldThemeFontColorR->getX(), 40);
    grpThemeFontColor->add(lblThemeFontColorGValue,sldThemeFontColorG->getX() + sldThemeFontColorG->getWidth() + 8, 40);
    grpThemeFontColor->add(lblThemeFontColorB, 10, 70);
    grpThemeFontColor->add(sldThemeFontColorB, sldThemeFontColorR->getX(), 70);
    grpThemeFontColor->add(lblThemeFontColorBValue, sldThemeFontColorB->getX() + sldThemeFontColorB->getWidth() + 8, 70);
    int grp_width = lblThemeFontColorRValue->getX() + lblThemeFontColorRValue->getWidth() + DISTANCE_BORDER;
    int grp_height = lblThemeFontColorB->getY() + lblThemeFontColorB->getHeight() + DISTANCE_BORDER + 5;
    grpThemeFontColor->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);

    // Theme base color RGB values
    lblThemeBaseColorR = new gcn::Label("R:");
    sldThemeBaseColorR = new gcn::Slider(0, 255);
    sldThemeBaseColorR->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeBaseColorR->setId("sldThemeBaseColorR");
    sldThemeBaseColorR->setBaseColor(gui_base_color);
    sldThemeBaseColorR->setBackgroundColor(gui_textbox_background_color);
    sldThemeBaseColorR->setForegroundColor(gui_foreground_color);
    sldThemeBaseColorR->setMarkerLength(marker_length);
	sldThemeBaseColorR->setStepLength(1);
	sldThemeBaseColorR->addActionListener(themesActionListener);
	lblThemeBaseColorRValue = new gcn::Label("255");

    lblThemeBaseColorG = new gcn::Label("G:");
    sldThemeBaseColorG = new gcn::Slider(0, 255);
    sldThemeBaseColorG->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeBaseColorG->setId("sldThemeBaseColorG");
    sldThemeBaseColorG->setBaseColor(gui_base_color);
    sldThemeBaseColorG->setBackgroundColor(gui_textbox_background_color);
    sldThemeBaseColorG->setForegroundColor(gui_foreground_color);
	sldThemeBaseColorG->setMarkerLength(marker_length);
	sldThemeBaseColorG->setStepLength(1);
	sldThemeBaseColorG->addActionListener(themesActionListener);
	lblThemeBaseColorGValue = new gcn::Label("255");

    lblThemeBaseColorB = new gcn::Label("B:");
    sldThemeBaseColorB = new gcn::Slider(0, 255);
    sldThemeBaseColorB->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeBaseColorB->setId("sldThemeBaseColorB");
    sldThemeBaseColorB->setBaseColor(gui_base_color);
    sldThemeBaseColorB->setBackgroundColor(gui_textbox_background_color);
    sldThemeBaseColorB->setForegroundColor(gui_foreground_color);
	sldThemeBaseColorB->setMarkerLength(marker_length);
	sldThemeBaseColorB->setStepLength(1);
	sldThemeBaseColorB->addActionListener(themesActionListener);
	lblThemeBaseColorBValue = new gcn::Label("255");

    grpThemeBaseColor = new gcn::Window("Base color");
    grpThemeBaseColor->setBaseColor(gui_base_color);
    grpThemeBaseColor->setForegroundColor(gui_foreground_color);
    grpThemeBaseColor->setMovable(false);
    grpThemeBaseColor->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpThemeBaseColor->add(lblThemeBaseColorR, 10, 10);
	grpThemeBaseColor->add(sldThemeBaseColorR, lblThemeBaseColorR->getX() + lblThemeBaseColorR->getWidth() + 8, 10);
	grpThemeBaseColor->add(lblThemeBaseColorRValue, sldThemeBaseColorR->getX() + sldThemeBaseColorR->getWidth() + 8, 10);
	grpThemeBaseColor->add(lblThemeBaseColorG, 10, 40);
	grpThemeBaseColor->add(sldThemeBaseColorG, sldThemeBaseColorR->getX(), 40);
	grpThemeBaseColor->add(lblThemeBaseColorGValue, sldThemeBaseColorG->getX() + sldThemeBaseColorG->getWidth() + 8, 40);
	grpThemeBaseColor->add(lblThemeBaseColorB, 10, 70);
	grpThemeBaseColor->add(sldThemeBaseColorB, sldThemeBaseColorR->getX(), 70);
	grpThemeBaseColor->add(lblThemeBaseColorBValue, sldThemeBaseColorB->getX() + sldThemeBaseColorB->getWidth() + 8, 70);
	grpThemeBaseColor->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);

    // Theme selector inactive color RGB values
    lblThemeSelectorInactiveColorR = new gcn::Label("R:");
    sldThemeSelectorInactiveColorR = new gcn::Slider(0, 255);
    sldThemeSelectorInactiveColorR->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectorInactiveColorR->setId("sldThemeSelectorInactiveColorR");
    sldThemeSelectorInactiveColorR->setBaseColor(gui_base_color);
    sldThemeSelectorInactiveColorR->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectorInactiveColorR->setForegroundColor(gui_foreground_color);
	sldThemeSelectorInactiveColorR->setMarkerLength(marker_length);
	sldThemeSelectorInactiveColorR->setStepLength(1);
	sldThemeSelectorInactiveColorR->addActionListener(themesActionListener);
	lblThemeSelectorInactiveColorRValue = new gcn::Label("255");

    lblThemeSelectorInactiveColorG = new gcn::Label("G:");
    sldThemeSelectorInactiveColorG = new gcn::Slider(0, 255);
    sldThemeSelectorInactiveColorG->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectorInactiveColorG->setId("sldThemeSelectorInactiveColorG");
    sldThemeSelectorInactiveColorG->setBaseColor(gui_base_color);
    sldThemeSelectorInactiveColorG->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectorInactiveColorG->setForegroundColor(gui_foreground_color);
	sldThemeSelectorInactiveColorG->setMarkerLength(marker_length);
	sldThemeSelectorInactiveColorG->setStepLength(1);
	sldThemeSelectorInactiveColorG->addActionListener(themesActionListener);
	lblThemeSelectorInactiveColorGValue = new gcn::Label("255");

    lblThemeSelectorInactiveColorB = new gcn::Label("B:");
    sldThemeSelectorInactiveColorB = new gcn::Slider(0, 255);
    sldThemeSelectorInactiveColorB->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectorInactiveColorB->setId("sldThemeSelectorInactiveColorB");
    sldThemeSelectorInactiveColorB->setBaseColor(gui_base_color);
    sldThemeSelectorInactiveColorB->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectorInactiveColorB->setForegroundColor(gui_foreground_color);
	sldThemeSelectorInactiveColorB->setMarkerLength(marker_length);
	sldThemeSelectorInactiveColorB->setStepLength(1);
	sldThemeSelectorInactiveColorB->addActionListener(themesActionListener);
	lblThemeSelectorInactiveColorBValue = new gcn::Label("255");

    grpThemeSelectorInactiveColor = new gcn::Window("Selector inactive color");
    grpThemeSelectorInactiveColor->setBaseColor(gui_base_color);
    grpThemeSelectorInactiveColor->setForegroundColor(gui_foreground_color);
    grpThemeSelectorInactiveColor->setMovable(false);
    grpThemeSelectorInactiveColor->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpThemeSelectorInactiveColor->add(lblThemeSelectorInactiveColorR, 10, 10);
	grpThemeSelectorInactiveColor->add(sldThemeSelectorInactiveColorR, lblThemeSelectorInactiveColorR->getX() + lblThemeSelectorInactiveColorR->getWidth() + 8, 10);
	grpThemeSelectorInactiveColor->add(lblThemeSelectorInactiveColorRValue, sldThemeSelectorInactiveColorR->getX() + sldThemeSelectorInactiveColorR->getWidth() + 8, 10);
	grpThemeSelectorInactiveColor->add(lblThemeSelectorInactiveColorG, 10, 40);
	grpThemeSelectorInactiveColor->add(sldThemeSelectorInactiveColorG, sldThemeSelectorInactiveColorR->getX(), 40);
	grpThemeSelectorInactiveColor->add(lblThemeSelectorInactiveColorGValue, sldThemeSelectorInactiveColorG->getX() + sldThemeSelectorInactiveColorG->getWidth() + 8, 40);
	grpThemeSelectorInactiveColor->add(lblThemeSelectorInactiveColorB, 10, 70);
	grpThemeSelectorInactiveColor->add(sldThemeSelectorInactiveColorB, sldThemeSelectorInactiveColorR->getX(), 70);
	grpThemeSelectorInactiveColor->add(lblThemeSelectorInactiveColorBValue, sldThemeSelectorInactiveColorB->getX() + sldThemeSelectorInactiveColorB->getWidth() + 8, 70);
	grpThemeSelectorInactiveColor->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);

    // Theme selector active color RGB values
    lblThemeSelectorActiveColorR = new gcn::Label("R:");
    sldThemeSelectorActiveColorR = new gcn::Slider(0, 255);
    sldThemeSelectorActiveColorR->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectorActiveColorR->setId("sldThemeSelectorActiveColorR");
    sldThemeSelectorActiveColorR->setBaseColor(gui_base_color);
    sldThemeSelectorActiveColorR->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectorActiveColorR->setForegroundColor(gui_foreground_color);
	sldThemeSelectorActiveColorR->setMarkerLength(marker_length);
	sldThemeSelectorActiveColorR->setStepLength(1);
	sldThemeSelectorActiveColorR->addActionListener(themesActionListener);
	lblThemeSelectorActiveColorRValue = new gcn::Label("255");

    lblThemeSelectorActiveColorG = new gcn::Label("G:");
    sldThemeSelectorActiveColorG = new gcn::Slider(0, 255);
    sldThemeSelectorActiveColorG->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectorActiveColorG->setId("sldThemeSelectorActiveColorG");
    sldThemeSelectorActiveColorG->setBaseColor(gui_base_color);
    sldThemeSelectorActiveColorG->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectorActiveColorG->setForegroundColor(gui_foreground_color);
	sldThemeSelectorActiveColorG->setMarkerLength(marker_length);
	sldThemeSelectorActiveColorG->setStepLength(1);
	sldThemeSelectorActiveColorG->addActionListener(themesActionListener);
	lblThemeSelectorActiveColorGValue = new gcn::Label("255");

    lblThemeSelectorActiveColorB = new gcn::Label("B:");
    sldThemeSelectorActiveColorB = new gcn::Slider(0, 255);
    sldThemeSelectorActiveColorB->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectorActiveColorB->setId("sldThemeSelectorActiveColorB");
    sldThemeSelectorActiveColorB->setBaseColor(gui_base_color);
    sldThemeSelectorActiveColorB->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectorActiveColorB->setForegroundColor(gui_foreground_color);
	sldThemeSelectorActiveColorB->setMarkerLength(marker_length);
	sldThemeSelectorActiveColorB->setStepLength(1);
	sldThemeSelectorActiveColorB->addActionListener(themesActionListener);
	lblThemeSelectorActiveColorBValue = new gcn::Label("255");

    grpThemeSelectorActiveColor = new gcn::Window("Selector active color");
    grpThemeSelectorActiveColor->setBaseColor(gui_base_color);
    grpThemeSelectorActiveColor->setForegroundColor(gui_foreground_color);
    grpThemeSelectorActiveColor->setMovable(false);
    grpThemeSelectorActiveColor->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpThemeSelectorActiveColor->add(lblThemeSelectorActiveColorR, 10, 10);
	grpThemeSelectorActiveColor->add(sldThemeSelectorActiveColorR, lblThemeSelectorActiveColorR->getX() + lblThemeSelectorActiveColorR->getWidth() + 8, 10);
	grpThemeSelectorActiveColor->add(lblThemeSelectorActiveColorRValue, sldThemeSelectorActiveColorR->getX() + sldThemeSelectorActiveColorR->getWidth() + 8, 10);
	grpThemeSelectorActiveColor->add(lblThemeSelectorActiveColorG, 10, 40);
	grpThemeSelectorActiveColor->add(sldThemeSelectorActiveColorG, sldThemeSelectorActiveColorR->getX(), 40);
	grpThemeSelectorActiveColor->add(lblThemeSelectorActiveColorGValue, sldThemeSelectorActiveColorG->getX() + sldThemeSelectorActiveColorG->getWidth() + 8, 40);
	grpThemeSelectorActiveColor->add(lblThemeSelectorActiveColorB, 10, 70);
	grpThemeSelectorActiveColor->add(sldThemeSelectorActiveColorB, sldThemeSelectorActiveColorR->getX(), 70);
	grpThemeSelectorActiveColor->add(lblThemeSelectorActiveColorBValue, sldThemeSelectorActiveColorB->getX() + sldThemeSelectorActiveColorB->getWidth() + 8, 70);
	grpThemeSelectorActiveColor->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);

    // Theme selection color RGB values
    lblThemeSelectionColorR = new gcn::Label("R:");
    sldThemeSelectionColorR = new gcn::Slider(0, 255);
    sldThemeSelectionColorR->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectionColorR->setId("sldThemeSelectionColorR");
    sldThemeSelectionColorR->setBaseColor(gui_base_color);
    sldThemeSelectionColorR->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectionColorR->setForegroundColor(gui_foreground_color);
	sldThemeSelectionColorR->setMarkerLength(marker_length);
	sldThemeSelectionColorR->setStepLength(1);
	sldThemeSelectionColorR->addActionListener(themesActionListener);
	lblThemeSelectionColorRValue = new gcn::Label("255");

    lblThemeSelectionColorG = new gcn::Label("G:");
    sldThemeSelectionColorG = new gcn::Slider(0, 255);
    sldThemeSelectionColorG->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectionColorG->setId("sldThemeSelectionColorG");
    sldThemeSelectionColorG->setBaseColor(gui_base_color);
    sldThemeSelectionColorG->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectionColorG->setForegroundColor(gui_foreground_color);
	sldThemeSelectionColorG->setMarkerLength(marker_length);
	sldThemeSelectionColorG->setStepLength(1);
	sldThemeSelectionColorG->addActionListener(themesActionListener);
	lblThemeSelectionColorGValue = new gcn::Label("255");

    lblThemeSelectionColorB = new gcn::Label("B:");
    sldThemeSelectionColorB = new gcn::Slider(0, 255);
    sldThemeSelectionColorB->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeSelectionColorB->setId("sldThemeSelectionColorB");
    sldThemeSelectionColorB->setBaseColor(gui_base_color);
    sldThemeSelectionColorB->setBackgroundColor(gui_textbox_background_color);
    sldThemeSelectionColorB->setForegroundColor(gui_foreground_color);
	sldThemeSelectionColorB->setMarkerLength(marker_length);
	sldThemeSelectionColorB->setStepLength(1);
	sldThemeSelectionColorB->addActionListener(themesActionListener);
	lblThemeSelectionColorBValue = new gcn::Label("255");

    grpThemeSelectionColor = new gcn::Window("Selection color");
    grpThemeSelectionColor->setBaseColor(gui_base_color);
    grpThemeSelectionColor->setForegroundColor(gui_foreground_color);
    grpThemeSelectionColor->setMovable(false);
    grpThemeSelectionColor->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpThemeSelectionColor->add(lblThemeSelectionColorR, 10, 10);
	grpThemeSelectionColor->add(sldThemeSelectionColorR, lblThemeSelectionColorR->getX() + lblThemeSelectionColorR->getWidth() + 8, 10);
	grpThemeSelectionColor->add(lblThemeSelectionColorRValue, sldThemeSelectionColorR->getX() + sldThemeSelectionColorR->getWidth() + 8, 10);
	grpThemeSelectionColor->add(lblThemeSelectionColorG, 10, 40);
	grpThemeSelectionColor->add(sldThemeSelectionColorG, sldThemeSelectionColorR->getX(), 40);
	grpThemeSelectionColor->add(lblThemeSelectionColorGValue, sldThemeSelectionColorG->getX() + sldThemeSelectionColorG->getWidth() + 8, 40);
	grpThemeSelectionColor->add(lblThemeSelectionColorB, 10, 70);
	grpThemeSelectionColor->add(sldThemeSelectionColorB, sldThemeSelectionColorR->getX(), 70);
	grpThemeSelectionColor->add(lblThemeSelectionColorBValue, sldThemeSelectionColorB->getX() + sldThemeSelectionColorB->getWidth() + 8, 70);
	grpThemeSelectionColor->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);

    // Theme text background color RGB values
    lblThemeTextBgColorR = new gcn::Label("R:");
    sldThemeTextBgColorR = new gcn::Slider(0, 255);
    sldThemeTextBgColorR->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeTextBgColorR->setId("sldThemeTextBgColorR");
    sldThemeTextBgColorR->setBaseColor(gui_base_color);
    sldThemeTextBgColorR->setBackgroundColor(gui_textbox_background_color);
    sldThemeTextBgColorR->setForegroundColor(gui_foreground_color);
	sldThemeTextBgColorR->setMarkerLength(marker_length);
	sldThemeTextBgColorR->setStepLength(1);
	sldThemeTextBgColorR->addActionListener(themesActionListener);
	lblThemeTextBgColorRValue = new gcn::Label("255");

    lblThemeTextBgColorG = new gcn::Label("G:");
    sldThemeTextBgColorG = new gcn::Slider(0, 255);
    sldThemeTextBgColorG->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeTextBgColorG->setId("sldThemeTextBgColorG");
    sldThemeTextBgColorG->setBaseColor(gui_base_color);
    sldThemeTextBgColorG->setBackgroundColor(gui_textbox_background_color);
    sldThemeTextBgColorG->setForegroundColor(gui_foreground_color);
	sldThemeTextBgColorG->setMarkerLength(marker_length);
	sldThemeTextBgColorG->setStepLength(1);
	sldThemeTextBgColorG->addActionListener(themesActionListener);
	lblThemeTextBgColorGValue = new gcn::Label("255");

    lblThemeTextBgColorB = new gcn::Label("B:");
    sldThemeTextBgColorB = new gcn::Slider(0, 255);
    sldThemeTextBgColorB->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeTextBgColorB->setId("sldThemeTextBgColorB");
    sldThemeTextBgColorB->setBaseColor(gui_base_color);
    sldThemeTextBgColorB->setBackgroundColor(gui_textbox_background_color);
    sldThemeTextBgColorB->setForegroundColor(gui_foreground_color);
	sldThemeTextBgColorB->setMarkerLength(marker_length);
	sldThemeTextBgColorB->setStepLength(1);
	sldThemeTextBgColorB->addActionListener(themesActionListener);
	lblThemeTextBgColorBValue = new gcn::Label("255");

    grpThemeTextBgColor = new gcn::Window("Text background color");
    grpThemeTextBgColor->setBaseColor(gui_base_color);
    grpThemeTextBgColor->setForegroundColor(gui_foreground_color);
    grpThemeTextBgColor->setMovable(false);
    grpThemeTextBgColor->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpThemeTextBgColor->add(lblThemeTextBgColorR, 10, 10);
	grpThemeTextBgColor->add(sldThemeTextBgColorR, lblThemeTextBgColorR->getX() + lblThemeTextBgColorR->getWidth() + 8, 10);
	grpThemeTextBgColor->add(lblThemeTextBgColorRValue, sldThemeTextBgColorR->getX() + sldThemeTextBgColorR->getWidth() + 8, 10);
	grpThemeTextBgColor->add(lblThemeTextBgColorG, 10, 40);
	grpThemeTextBgColor->add(sldThemeTextBgColorG, sldThemeTextBgColorR->getX(), 40);
	grpThemeTextBgColor->add(lblThemeTextBgColorGValue, sldThemeTextBgColorG->getX() + sldThemeTextBgColorG->getWidth() + 8, 40);
	grpThemeTextBgColor->add(lblThemeTextBgColorB, 10, 70);
	grpThemeTextBgColor->add(sldThemeTextBgColorB, sldThemeTextBgColorR->getX(), 70);
	grpThemeTextBgColor->add(lblThemeTextBgColorBValue, sldThemeTextBgColorB->getX() + sldThemeTextBgColorB->getWidth() + 8, 70);
	grpThemeTextBgColor->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);

    // Theme foreground color RGB values
    lblThemeFgColorR = new gcn::Label("R:");
    sldThemeFgColorR = new gcn::Slider(0, 255);
    sldThemeFgColorR->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeFgColorR->setId("sldThemeFgColorR");
    sldThemeFgColorR->setBaseColor(gui_base_color);
    sldThemeFgColorR->setBackgroundColor(gui_textbox_background_color);
    sldThemeFgColorR->setForegroundColor(gui_foreground_color);
	sldThemeFgColorR->setMarkerLength(marker_length);
	sldThemeFgColorR->setStepLength(1);
	sldThemeFgColorR->addActionListener(themesActionListener);
	lblThemeFgColorRValue = new gcn::Label("255");

    lblThemeFgColorG = new gcn::Label("G:");
    sldThemeFgColorG = new gcn::Slider(0, 255);
    sldThemeFgColorG->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeFgColorG->setId("sldThemeFgColorG");
    sldThemeFgColorG->setBaseColor(gui_base_color);
    sldThemeFgColorG->setBackgroundColor(gui_textbox_background_color);
    sldThemeFgColorG->setForegroundColor(gui_foreground_color);
	sldThemeFgColorG->setMarkerLength(marker_length);
	sldThemeFgColorG->setStepLength(1);
	sldThemeFgColorG->addActionListener(themesActionListener);
	lblThemeFgColorGValue = new gcn::Label("255");

    lblThemeFgColorB = new gcn::Label("B:");
    sldThemeFgColorB = new gcn::Slider(0, 255);
    sldThemeFgColorB->setSize(slider_width, SLIDER_HEIGHT);
    sldThemeFgColorB->setId("sldThemeFgColorB");
    sldThemeFgColorB->setBaseColor(gui_base_color);
    sldThemeFgColorB->setBackgroundColor(gui_textbox_background_color);
    sldThemeFgColorB->setForegroundColor(gui_foreground_color);
	sldThemeFgColorB->setMarkerLength(marker_length);
	sldThemeFgColorB->setStepLength(1);
	sldThemeFgColorB->addActionListener(themesActionListener);
	lblThemeFgColorBValue = new gcn::Label("255");

    grpThemeFgColor = new gcn::Window("Foreground color");
    grpThemeFgColor->setBaseColor(gui_base_color);
    grpThemeFgColor->setForegroundColor(gui_foreground_color);
    grpThemeFgColor->setMovable(false);
    grpThemeFgColor->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpThemeFgColor->add(lblThemeFgColorR, 10, 10);
	grpThemeFgColor->add(sldThemeFgColorR, lblThemeFgColorR->getX() + lblThemeFgColorR->getWidth() + 8, 10);
	grpThemeFgColor->add(lblThemeFgColorRValue, sldThemeFgColorR->getX() + sldThemeFgColorR->getWidth() + 8, 10);
	grpThemeFgColor->add(lblThemeFgColorG, 10, 40);
	grpThemeFgColor->add(sldThemeFgColorG, sldThemeFgColorR->getX(), 40);
	grpThemeFgColor->add(lblThemeFgColorGValue, sldThemeFgColorG->getX() + sldThemeFgColorG->getWidth() + 8, 40);
	grpThemeFgColor->add(lblThemeFgColorB, 10, 70);
	grpThemeFgColor->add(sldThemeFgColorB, sldThemeFgColorR->getX(), 70);
	grpThemeFgColor->add(lblThemeFgColorBValue, sldThemeFgColorB->getX() + sldThemeFgColorB->getWidth() + 8, 70);
	grpThemeFgColor->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);

    // Save button
    cmdThemeSave = new gcn::Button("Save");
    cmdThemeSave->setId("cmdThemeSave");
    cmdThemeSave->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    cmdThemeSave->setBaseColor(gui_base_color);
    cmdThemeSave->setForegroundColor(gui_foreground_color);
    cmdThemeSave->addActionListener(themesActionListener);
    // Reset button
    cmdThemeReset = new gcn::Button("Reset");
    cmdThemeReset->setId("cmdThemeReset");
    cmdThemeReset->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    cmdThemeReset->setBaseColor(gui_base_color);
    cmdThemeReset->setForegroundColor(gui_foreground_color);
    cmdThemeReset->addActionListener(themesActionListener);
    // Load button
    cmdThemeLoad = new gcn::Button("Load");
    cmdThemeLoad->setId("cmdThemeLoad");
    cmdThemeLoad->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    cmdThemeLoad->setBaseColor(gui_base_color);
    cmdThemeLoad->setForegroundColor(gui_foreground_color);
    cmdThemeLoad->addActionListener(themesActionListener);

    // Positioning
	int pos_x = DISTANCE_BORDER;
	int pos_y = DISTANCE_BORDER;

	category.panel->add(lblThemeFont, pos_x, pos_x);
	pos_x += lblThemeFont->getWidth() + 8;
	category.panel->add(txtThemeFont, pos_x, pos_y);
	pos_x += txtThemeFont->getWidth() + 8;
	category.panel->add(cmdThemeFont, pos_x, pos_y);
	pos_x += cmdThemeFont->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeFontSize, pos_x, pos_y);
	pos_x += lblThemeFontSize->getWidth() + 8;
    category.panel->add(txtThemeFontSize, pos_x, pos_y);
    pos_y += txtThemeFont->getHeight() + DISTANCE_NEXT_Y;

    pos_x = DISTANCE_BORDER;
    category.panel->add(grpThemeFontColor, pos_x, pos_y);
	pos_x = grpThemeFontColor->getX() + grpThemeFontColor->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(grpThemeBaseColor, pos_x, pos_y);

    pos_y += grpThemeFontColor->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(grpThemeSelectorInactiveColor, DISTANCE_BORDER, pos_y);
	pos_x = grpThemeSelectorInactiveColor->getX() + grpThemeSelectorInactiveColor->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(grpThemeSelectorActiveColor, pos_x, pos_y);

	pos_y += grpThemeSelectorInactiveColor->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(grpThemeSelectionColor, DISTANCE_BORDER, pos_y);
	pos_x = grpThemeSelectionColor->getX() + grpThemeSelectionColor->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(grpThemeTextBgColor, pos_x, pos_y);

	pos_y += grpThemeSelectionColor->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(grpThemeFgColor, DISTANCE_BORDER, pos_y);

    // Bottom buttons
    pos_x = DISTANCE_BORDER;
    pos_y = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;

    category.panel->add(cmdThemeSave, pos_x, pos_y);
    pos_x += cmdThemeSave->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(cmdThemeReset, pos_x, pos_y);
    pos_x += cmdThemeReset->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(cmdThemeLoad, pos_x, pos_y);

	RefreshPanelThemes();
}

void ExitPanelThemes()
{
	delete lblThemeFont;
	delete txtThemeFont;
	delete cmdThemeFont;

	delete lblThemeFontSize;
	delete txtThemeFontSize;

    delete grpThemeFontColor;
    delete lblThemeFontColorR;
    delete sldThemeFontColorR;
    delete lblThemeFontColorRValue;
    delete lblThemeFontColorG;
    delete sldThemeFontColorG;
    delete lblThemeFontColorGValue;
    delete lblThemeFontColorB;
    delete sldThemeFontColorB;
    delete lblThemeFontColorBValue;

    delete grpThemeBaseColor;
	delete lblThemeBaseColorR;
	delete sldThemeBaseColorR;
	delete lblThemeBaseColorRValue;
	delete lblThemeBaseColorG;
	delete sldThemeBaseColorG;
	delete lblThemeBaseColorGValue;
	delete lblThemeBaseColorB;
	delete sldThemeBaseColorB;
	delete lblThemeBaseColorBValue;

    delete grpThemeSelectorInactiveColor;
    delete lblThemeSelectorInactiveColorR;
	delete sldThemeSelectorInactiveColorR;
	delete lblThemeSelectorInactiveColorRValue;
	delete lblThemeSelectorInactiveColorG;
	delete sldThemeSelectorInactiveColorG;
	delete lblThemeSelectorInactiveColorGValue;
	delete lblThemeSelectorInactiveColorB;
	delete sldThemeSelectorInactiveColorB;
	delete lblThemeSelectorInactiveColorBValue;

    delete grpThemeSelectorActiveColor;
	delete lblThemeSelectorActiveColorR;
	delete sldThemeSelectorActiveColorR;
    delete lblThemeSelectorActiveColorRValue;
	delete lblThemeSelectorActiveColorG;
	delete sldThemeSelectorActiveColorG;
	delete lblThemeSelectorActiveColorGValue;
	delete lblThemeSelectorActiveColorB;
	delete sldThemeSelectorActiveColorB;
	delete lblThemeSelectorActiveColorBValue;

    delete grpThemeSelectionColor;
	delete lblThemeSelectionColorR;
	delete sldThemeSelectionColorR;
	delete lblThemeSelectionColorRValue;
	delete lblThemeSelectionColorG;
	delete sldThemeSelectionColorG;
	delete lblThemeSelectionColorGValue;
	delete lblThemeSelectionColorB;
	delete sldThemeSelectionColorB;
	delete lblThemeSelectionColorBValue;

    delete grpThemeTextBgColor;
	delete lblThemeTextBgColorR;
	delete sldThemeTextBgColorR;
	delete lblThemeTextBgColorRValue;
	delete lblThemeTextBgColorG;
	delete sldThemeTextBgColorG;
	delete lblThemeTextBgColorGValue;
	delete lblThemeTextBgColorB;
	delete sldThemeTextBgColorB;
	delete lblThemeTextBgColorBValue;

    delete grpThemeFgColor;
    delete lblThemeFgColorR;
    delete sldThemeFgColorR;
	delete lblThemeFgColorRValue;
    delete lblThemeFgColorG;
    delete sldThemeFgColorG;
	delete lblThemeFgColorGValue;
    delete lblThemeFgColorB;
    delete sldThemeFgColorB;
	delete lblThemeFgColorBValue;

    delete cmdThemeSave;
    delete cmdThemeReset;
    delete cmdThemeLoad;

	delete themesActionListener;
}

void RefreshPanelThemes()
{
	txtThemeFont->setText(gui_theme.font_name);
    txtThemeFontSize->setText(std::to_string(gui_theme.font_size));

	// Theme font color RGB values
    sldThemeFontColorR->setValue(gui_theme.font_color.r);
    sldThemeFontColorG->setValue(gui_theme.font_color.g);
    sldThemeFontColorB->setValue(gui_theme.font_color.b);
    lblThemeFontColorRValue->setCaption(std::to_string(gui_theme.font_color.r));
    lblThemeFontColorGValue->setCaption(std::to_string(gui_theme.font_color.g));
    lblThemeFontColorBValue->setCaption(std::to_string(gui_theme.font_color.b));

	// Theme base color RGB values
    sldThemeBaseColorR->setValue(gui_theme.base_color.r);
    sldThemeBaseColorG->setValue(gui_theme.base_color.g);
    sldThemeBaseColorB->setValue(gui_theme.base_color.b);
	lblThemeBaseColorRValue->setCaption(std::to_string(gui_theme.base_color.r));
	lblThemeBaseColorGValue->setCaption(std::to_string(gui_theme.base_color.g));
	lblThemeBaseColorBValue->setCaption(std::to_string(gui_theme.base_color.b));

	// Theme selector inactive color RGB values
    sldThemeSelectorInactiveColorR->setValue(gui_theme.selector_inactive.r);
    sldThemeSelectorInactiveColorG->setValue(gui_theme.selector_inactive.g);
    sldThemeSelectorInactiveColorB->setValue(gui_theme.selector_inactive.b);
	lblThemeSelectorInactiveColorRValue->setCaption(std::to_string(gui_theme.selector_inactive.r));
	lblThemeSelectorInactiveColorGValue->setCaption(std::to_string(gui_theme.selector_inactive.g));
	lblThemeSelectorInactiveColorBValue->setCaption(std::to_string(gui_theme.selector_inactive.b));

	// Theme selector active color RGB values
    sldThemeSelectorActiveColorR->setValue(gui_theme.selector_active.r);
    sldThemeSelectorActiveColorG->setValue(gui_theme.selector_active.g);
    sldThemeSelectorActiveColorB->setValue(gui_theme.selector_active.b);
	lblThemeSelectorActiveColorRValue->setCaption(std::to_string(gui_theme.selector_active.r));
	lblThemeSelectorActiveColorGValue->setCaption(std::to_string(gui_theme.selector_active.g));
	lblThemeSelectorActiveColorBValue->setCaption(std::to_string(gui_theme.selector_active.b));

	// Theme selection color RGB values
    sldThemeSelectionColorR->setValue(gui_theme.selection_color.r);
    sldThemeSelectionColorG->setValue(gui_theme.selection_color.g);
    sldThemeSelectionColorB->setValue(gui_theme.selection_color.b);
	lblThemeSelectionColorRValue->setCaption(std::to_string(gui_theme.selection_color.r));
	lblThemeSelectionColorGValue->setCaption(std::to_string(gui_theme.selection_color.g));
	lblThemeSelectionColorBValue->setCaption(std::to_string(gui_theme.selection_color.b));

	// Theme text background color RGB values
    sldThemeTextBgColorR->setValue(gui_theme.textbox_background.r);
    sldThemeTextBgColorG->setValue(gui_theme.textbox_background.g);
    sldThemeTextBgColorB->setValue(gui_theme.textbox_background.b);
	lblThemeTextBgColorRValue->setCaption(std::to_string(gui_theme.textbox_background.r));
	lblThemeTextBgColorGValue->setCaption(std::to_string(gui_theme.textbox_background.g));
	lblThemeTextBgColorBValue->setCaption(std::to_string(gui_theme.textbox_background.b));

	// Theme foreground color RGB values
    sldThemeFgColorR->setValue(gui_theme.foreground_color.r);
    sldThemeFgColorG->setValue(gui_theme.foreground_color.g);
    sldThemeFgColorB->setValue(gui_theme.foreground_color.b);
	lblThemeFgColorRValue->setCaption(std::to_string(gui_theme.foreground_color.r));
	lblThemeFgColorGValue->setCaption(std::to_string(gui_theme.foreground_color.g));
	lblThemeFgColorBValue->setCaption(std::to_string(gui_theme.foreground_color.b));
}

bool HelpPanelThemes(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("The themes panel allows you to customize the look of the GUI.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Font: The font to use for the GUI.");
	helptext.emplace_back("Font size: The size of the font.");
	helptext.emplace_back("Font color: The color of the font.");
	helptext.emplace_back("Base color: The base color of the GUI.");
	helptext.emplace_back("Selector inactive color: The color of the inactive selector.");
	helptext.emplace_back("Selector active color: The color of the active selector.");
	helptext.emplace_back("Selection color: The color of the selection.");
	helptext.emplace_back("Text background color: The color of the text background.");
    helptext.emplace_back("Foreground color: The color of the foreground.");
	return true;
}