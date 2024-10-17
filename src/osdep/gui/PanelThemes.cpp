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

struct RGBColorComponents {
    gcn::Window* group;
    gcn::Label* labelR;
    gcn::Slider* sliderR;
    gcn::Label* valueR;
    gcn::Label* labelG;
    gcn::Slider* sliderG;
    gcn::Label* valueG;
    gcn::Label* labelB;
    gcn::Slider* sliderB;
    gcn::Label* valueB;
};

static RGBColorComponents themeFontColor;
static RGBColorComponents themeBaseColor;
static RGBColorComponents themeSelectorInactiveColor;
static RGBColorComponents themeSelectorActiveColor;
static RGBColorComponents themeSelectionColor;
static RGBColorComponents themeTextBgColor;
static RGBColorComponents themeFgColor;

// Save, Reset and Load buttons
static gcn::Button* cmdThemeSave;
static gcn::Button* cmdThemeReset;
static gcn::Button* cmdThemeLoad;

//TODO add a DropDown for the available preset themes

constexpr int slider_width = 150;
constexpr int marker_length = 20;

class ThemesActionListener : public gcn::ActionListener
{
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto source = actionEvent.getSource();
		if (source == cmdThemeFont)
		{
			const char* filter[] = { ".ttf", "\0" };
			const std::string font = SelectFile("Select font", "/usr/share/fonts/truetype/", filter, false);
			if (!font.empty())
			{
				txtThemeFont->setText(font);
				gui_theme.font_name = font;
			}
		}
        else if (source == cmdThemeReset)
        {
            ResetTheme();
        }
        else if (source == cmdThemeSave)
        {
            //TODO Save the values to amiberry.conf
        }
        else if (source == cmdThemeLoad)
        {
            //TODO Load the values from amiberry.conf
        }
        else 
        {
            HandleSliderAction(source);
        }

		RefreshPanelThemes();
	}

	static void HandleSliderAction(const gcn::Widget* source) {
        if (source == themeFontColor.sliderR) {
            UpdateColorComponent(gui_theme.font_color.r, themeFontColor.sliderR, themeFontColor.valueR);
        }
        else if (source == themeFontColor.sliderG) {
            UpdateColorComponent(gui_theme.font_color.g, themeFontColor.sliderG, themeFontColor.valueG);
        }
        else if (source == themeFontColor.sliderB) {
            UpdateColorComponent(gui_theme.font_color.b, themeFontColor.sliderB, themeFontColor.valueB);
        }
        else if (source == themeBaseColor.sliderR) {
            UpdateColorComponent(gui_theme.base_color.r, themeBaseColor.sliderR, themeBaseColor.valueR);
        }
        else if (source == themeBaseColor.sliderG) {
            UpdateColorComponent(gui_theme.base_color.g, themeBaseColor.sliderG, themeBaseColor.valueG);
        }
        else if (source == themeBaseColor.sliderB) {
            UpdateColorComponent(gui_theme.base_color.b, themeBaseColor.sliderB, themeBaseColor.valueB);
        }
        else if (source == themeSelectorInactiveColor.sliderR) {
            UpdateColorComponent(gui_theme.selector_inactive.r, themeSelectorInactiveColor.sliderR, themeSelectorInactiveColor.valueR);
        }
        else if (source == themeSelectorInactiveColor.sliderG) {
            UpdateColorComponent(gui_theme.selector_inactive.g, themeSelectorInactiveColor.sliderG, themeSelectorInactiveColor.valueG);
        }
        else if (source == themeSelectorInactiveColor.sliderB) {
            UpdateColorComponent(gui_theme.selector_inactive.b, themeSelectorInactiveColor.sliderB, themeSelectorInactiveColor.valueB);
        }
        else if (source == themeSelectorActiveColor.sliderR) {
            UpdateColorComponent(gui_theme.selector_active.r, themeSelectorActiveColor.sliderR, themeSelectorActiveColor.valueR);
        }
        else if (source == themeSelectorActiveColor.sliderG) {
            UpdateColorComponent(gui_theme.selector_active.g, themeSelectorActiveColor.sliderG, themeSelectorActiveColor.valueG);
        }
        else if (source == themeSelectorActiveColor.sliderB) {
            UpdateColorComponent(gui_theme.selector_active.b, themeSelectorActiveColor.sliderB, themeSelectorActiveColor.valueB);
        }
        else if (source == themeSelectionColor.sliderR) {
            UpdateColorComponent(gui_theme.selection_color.r, themeSelectionColor.sliderR, themeSelectionColor.valueR);
        }
        else if (source == themeSelectionColor.sliderG) {
            UpdateColorComponent(gui_theme.selection_color.g, themeSelectionColor.sliderG, themeSelectionColor.valueG);
        }
        else if (source == themeSelectionColor.sliderB) {
            UpdateColorComponent(gui_theme.selection_color.b, themeSelectionColor.sliderB, themeSelectionColor.valueB);
        }
        else if (source == themeTextBgColor.sliderR) {
            UpdateColorComponent(gui_theme.textbox_background.r, themeTextBgColor.sliderR, themeTextBgColor.valueR);
        }
        else if (source == themeTextBgColor.sliderG) {
            UpdateColorComponent(gui_theme.textbox_background.g, themeTextBgColor.sliderG, themeTextBgColor.valueG);
        }
        else if (source == themeTextBgColor.sliderB) {
            UpdateColorComponent(gui_theme.textbox_background.b, themeTextBgColor.sliderB, themeTextBgColor.valueB);
        }
        else if (source == themeFgColor.sliderR) {
            UpdateColorComponent(gui_theme.foreground_color.r, themeFgColor.sliderR, themeFgColor.valueR);
        }
        else if (source == themeFgColor.sliderG) {
            UpdateColorComponent(gui_theme.foreground_color.g, themeFgColor.sliderG, themeFgColor.valueG);
        }
        else if (source == themeFgColor.sliderB) {
            UpdateColorComponent(gui_theme.foreground_color.b, themeFgColor.sliderB, themeFgColor.valueB);
        }
    }

	static void UpdateColorComponent(int& colorComponent, const gcn::Slider* slider, gcn::Label* label) {
        colorComponent = static_cast<int>(slider->getValue());
        label->setCaption(std::to_string(slider->getValue()));
    }

	static void ResetTheme() {
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
};

static ThemesActionListener* themesActionListener;

gcn::Slider* CreateSlider() {
    auto slider = new gcn::Slider(0, 255);
    slider->setSize(slider_width, SLIDER_HEIGHT);
    slider->setBaseColor(gui_base_color);
    slider->setBackgroundColor(gui_textbox_background_color);
    slider->setForegroundColor(gui_foreground_color);
    slider->setMarkerLength(marker_length);
    slider->setStepLength(1);
    slider->addActionListener(themesActionListener);
    return slider;
}

gcn::Button* CreateButton(const std::string& caption, const std::string& id) {
    auto button = new gcn::Button(caption);
    button->setId(id);
    button->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    button->setBaseColor(gui_base_color);
    button->setForegroundColor(gui_foreground_color);
    button->addActionListener(themesActionListener);
    return button;
}

void InitRGBColorComponents(RGBColorComponents& components, const std::string& title) {
    components.labelR = new gcn::Label("R:");
    components.sliderR = CreateSlider();
    components.valueR = new gcn::Label("255");

    components.labelG = new gcn::Label("G:");
    components.sliderG = CreateSlider();
    components.valueG = new gcn::Label("255");

    components.labelB = new gcn::Label("B:");
    components.sliderB = CreateSlider();
    components.valueB = new gcn::Label("255");

    components.group = new gcn::Window(title);
    components.group->setBaseColor(gui_base_color);
    components.group->setForegroundColor(gui_foreground_color);
    components.group->setMovable(false);
    components.group->setTitleBarHeight(TITLEBAR_HEIGHT);
    components.group->add(components.labelR, 10, 10);
    components.group->add(components.sliderR, components.labelR->getX() + components.labelR->getWidth() + 8, 10);
    components.group->add(components.valueR, components.sliderR->getX() + components.sliderR->getWidth() + 8, 10);
    components.group->add(components.labelG, 10, 40);
    components.group->add(components.sliderG, components.sliderR->getX(), 40);
    components.group->add(components.valueG, components.sliderG->getX() + components.sliderG->getWidth() + 8, 40);
    components.group->add(components.labelB, 10, 70);
    components.group->add(components.sliderB, components.sliderR->getX(), 70);
    components.group->add(components.valueB, components.sliderB->getX() + components.sliderB->getWidth() + 8, 70);
    const int grp_width = components.valueR->getX() + components.valueR->getWidth() + DISTANCE_BORDER;
    const int grp_height = components.labelB->getY() + components.labelB->getHeight() + DISTANCE_BORDER + 5;
    components.group->setSize(grp_width, TITLEBAR_HEIGHT + grp_height);
}

void PositionComponents(const config_category& category) {
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
    category.panel->add(themeFontColor.group, pos_x, pos_y);
    pos_x = themeFontColor.group->getX() + themeFontColor.group->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(themeBaseColor.group, pos_x, pos_y);

    pos_y += themeFontColor.group->getHeight() + DISTANCE_NEXT_Y;

    category.panel->add(themeSelectorInactiveColor.group, DISTANCE_BORDER, pos_y);
    pos_x = themeSelectorInactiveColor.group->getX() + themeSelectorInactiveColor.group->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(themeSelectorActiveColor.group, pos_x, pos_y);

    pos_y += themeSelectorInactiveColor.group->getHeight() + DISTANCE_NEXT_Y;

    category.panel->add(themeSelectionColor.group, DISTANCE_BORDER, pos_y);
    pos_x = themeSelectionColor.group->getX() + themeSelectionColor.group->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(themeTextBgColor.group, pos_x, pos_y);

    pos_y += themeSelectionColor.group->getHeight() + DISTANCE_NEXT_Y;

    category.panel->add(themeFgColor.group, DISTANCE_BORDER, pos_y);

    // Bottom buttons
    pos_x = DISTANCE_BORDER;
    pos_y = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;

    category.panel->add(cmdThemeSave, pos_x, pos_y);
    pos_x += cmdThemeSave->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(cmdThemeReset, pos_x, pos_y);
    pos_x += cmdThemeReset->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(cmdThemeLoad, pos_x, pos_y);
}

void InitPanelThemes(const config_category& category)
{
	themesActionListener = new ThemesActionListener();


	lblThemeFont = new gcn::Label("Font:");
	txtThemeFont = new gcn::TextField();
	txtThemeFont->setSize(380, TEXTFIELD_HEIGHT);
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

    InitRGBColorComponents(themeFontColor, "Font color");
    InitRGBColorComponents(themeBaseColor, "Base color");
    InitRGBColorComponents(themeSelectorInactiveColor, "Selector inactive color");
    InitRGBColorComponents(themeSelectorActiveColor, "Selector active color");
    InitRGBColorComponents(themeSelectionColor, "Selection color");
    InitRGBColorComponents(themeTextBgColor, "Text background color");
    InitRGBColorComponents(themeFgColor, "Foreground color");

    cmdThemeSave = CreateButton("Save", "cmdThemeSave");
    cmdThemeReset = CreateButton("Reset", "cmdThemeReset");
    cmdThemeLoad = CreateButton("Load", "cmdThemeLoad");

    PositionComponents(category);

	RefreshPanelThemes();
}

void DeleteRGBColorComponents(const RGBColorComponents& components) {
    delete components.group;
    delete components.labelR;
    delete components.sliderR;
    delete components.valueR;
    delete components.labelG;
    delete components.sliderG;
    delete components.valueG;
    delete components.labelB;
    delete components.sliderB;
    delete components.valueB;
}

void ExitPanelThemes()
{
	delete lblThemeFont;
	delete txtThemeFont;
	delete cmdThemeFont;

	delete lblThemeFontSize;
	delete txtThemeFontSize;

    DeleteRGBColorComponents(themeFontColor);
    DeleteRGBColorComponents(themeBaseColor);
    DeleteRGBColorComponents(themeSelectorInactiveColor);
    DeleteRGBColorComponents(themeSelectorActiveColor);
    DeleteRGBColorComponents(themeSelectionColor);
    DeleteRGBColorComponents(themeTextBgColor);
    DeleteRGBColorComponents(themeFgColor);

    delete cmdThemeSave;
    delete cmdThemeReset;
    delete cmdThemeLoad;

	delete themesActionListener;
}

void RefreshRGBColorComponents(const RGBColorComponents& components, const gcn::Color& color) {
    components.sliderR->setValue(color.r);
    components.sliderG->setValue(color.g);
    components.sliderB->setValue(color.b);
    components.valueR->setCaption(std::to_string(color.r));
    components.valueG->setCaption(std::to_string(color.g));
    components.valueB->setCaption(std::to_string(color.b));
}

void RefreshPanelThemes()
{
	txtThemeFont->setText(gui_theme.font_name);
    txtThemeFontSize->setText(std::to_string(gui_theme.font_size));

    RefreshRGBColorComponents(themeFontColor, gui_theme.font_color);
    RefreshRGBColorComponents(themeBaseColor, gui_theme.base_color);
    RefreshRGBColorComponents(themeSelectorInactiveColor, gui_theme.selector_inactive);
    RefreshRGBColorComponents(themeSelectorActiveColor, gui_theme.selector_active);
    RefreshRGBColorComponents(themeSelectionColor, gui_theme.selection_color);
    RefreshRGBColorComponents(themeTextBgColor, gui_theme.textbox_background);
    RefreshRGBColorComponents(themeFgColor, gui_theme.foreground_color);
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