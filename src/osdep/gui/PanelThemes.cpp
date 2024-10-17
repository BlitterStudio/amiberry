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

static gcn::Label* lblThemeFontColorR;
static gcn::TextField* txtThemeFontColorR;
static gcn::Label* lblThemeFontColorG;
static gcn::TextField* txtThemeFontColorG;
static gcn::Label* lblThemeFontColorB;
static gcn::TextField* txtThemeFontColorB;

static gcn::Label* lblThemeBaseColor;
static gcn::Label* lblThemeBaseColorR;
static gcn::TextField* txtThemeBaseColorR;
static gcn::Label* lblThemeBaseColorG;
static gcn::TextField* txtThemeBaseColorG;
static gcn::Label* lblThemeBaseColorB;
static gcn::TextField* txtThemeBaseColorB;

static gcn::Label* lblThemeSelectorInactiveColor;
static gcn::Label* lblThemeSelectorInactiveColorR;
static gcn::TextField* txtThemeSelectorInactiveColorR;
static gcn::Label* lblThemeSelectorInactiveColorG;
static gcn::TextField* txtThemeSelectorInactiveColorG;
static gcn::Label* lblThemeSelectorInactiveColorB;
static gcn::TextField* txtThemeSelectorInactiveColorB;

static gcn::Label* lblThemeSelectorActiveColor;
static gcn::Label* lblThemeSelectorActiveColorR;
static gcn::TextField* txtThemeSelectorActiveColorR;
static gcn::Label* lblThemeSelectorActiveColorG;
static gcn::TextField* txtThemeSelectorActiveColorG;
static gcn::Label* lblThemeSelectorActiveColorB;
static gcn::TextField* txtThemeSelectorActiveColorB;

static gcn::Label* lblThemeSelectionColor;
static gcn::Label* lblThemeSelectionColorR;
static gcn::TextField* txtThemeSelectionColorR;
static gcn::Label* lblThemeSelectionColorG;
static gcn::TextField* txtThemeSelectionColorG;
static gcn::Label* lblThemeSelectionColorB;
static gcn::TextField* txtThemeSelectionColorB;

static gcn::Label* lblThemeTextBgColor;
static gcn::Label* lblThemeTextBgColorR;
static gcn::TextField* txtThemeTextBgColorR;
static gcn::Label* lblThemeTextBgColorG;
static gcn::TextField* txtThemeTextBgColorG;
static gcn::Label* lblThemeTextBgColorB;
static gcn::TextField* txtThemeTextBgColorB;

static gcn::Label* lblThemeFgColor;
static gcn::Label* lblThemeFgColorR;
static gcn::TextField* txtThemeFgColorR;
static gcn::Label* lblThemeFgColorG;
static gcn::TextField* txtThemeFgColorG;
static gcn::Label* lblThemeFgColorB;
static gcn::TextField* txtThemeFgColorB;

static gcn::Button* cmdThemeSave;
static gcn::Button* cmdThemeReset;
static gcn::Button* cmdThemeLoad;

//TODO add a DropDown for the available preset themes

class ThemesButtonActionListener : public gcn::ActionListener
{
	void action(const gcn::ActionEvent& actionEvent) override
	{
        auto source = actionEvent.getSource();
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

		RefreshPanelThemes();
	}
};

static ThemesButtonActionListener* themesButtonActionListener;

void InitPanelThemes(const config_category& category)
{
	themesButtonActionListener = new ThemesButtonActionListener();

	lblThemeFont = new gcn::Label("Font:");
	txtThemeFont = new gcn::TextField();
	txtThemeFont->setSize(480, TEXTFIELD_HEIGHT);
	txtThemeFont->setBaseColor(gui_base_color);
	txtThemeFont->setBackgroundColor(gui_textbox_background_color);
	txtThemeFont->setForegroundColor(gui_foreground_color);

	cmdThemeFont = new gcn::Button("...");
	cmdThemeFont->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdThemeFont->setBaseColor(gui_base_color);
	cmdThemeFont->setForegroundColor(gui_foreground_color);
	cmdThemeFont->addActionListener(themesButtonActionListener);

    lblThemeFontSize = new gcn::Label("Font size:");
    txtThemeFontSize = new gcn::TextField();
    txtThemeFontSize->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFontSize->setId("txtThemeFontSize");
    txtThemeFontSize->setBaseColor(gui_base_color);
    txtThemeFontSize->setBackgroundColor(gui_textbox_background_color);
    txtThemeFontSize->setForegroundColor(gui_foreground_color);

    // Theme font color RGB values
    lblThemeFontColorR = new gcn::Label("Red:");
    txtThemeFontColorR = new gcn::TextField();
    txtThemeFontColorR->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFontColorR->setId("txtThemeFontColorR");
    txtThemeFontColorR->setBaseColor(gui_base_color);
    txtThemeFontColorR->setBackgroundColor(gui_textbox_background_color);
    txtThemeFontColorR->setForegroundColor(gui_foreground_color);

    lblThemeFontColorG = new gcn::Label("Green:");
    txtThemeFontColorG = new gcn::TextField();
    txtThemeFontColorG->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFontColorG->setId("txtThemeFontColorG");
    txtThemeFontColorG->setBaseColor(gui_base_color);
    txtThemeFontColorG->setBackgroundColor(gui_textbox_background_color);
    txtThemeFontColorG->setForegroundColor(gui_foreground_color);

    lblThemeFontColorB = new gcn::Label("Blue:");
    txtThemeFontColorB = new gcn::TextField();
    txtThemeFontColorB->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFontColorB->setId("txtThemeFontColorB");
    txtThemeFontColorB->setBaseColor(gui_base_color);
    txtThemeFontColorB->setBackgroundColor(gui_textbox_background_color);
    txtThemeFontColorB->setForegroundColor(gui_foreground_color);

    lblThemeBaseColor = new gcn::Label("Base color");
    // Theme base color RGB values
    lblThemeBaseColorR = new gcn::Label("Red:");
    txtThemeBaseColorR = new gcn::TextField();
    txtThemeBaseColorR->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeBaseColorR->setId("txtThemeBaseColorR");
    txtThemeBaseColorR->setBaseColor(gui_base_color);
    txtThemeBaseColorR->setBackgroundColor(gui_textbox_background_color);
    txtThemeBaseColorR->setForegroundColor(gui_foreground_color);

    lblThemeBaseColorG = new gcn::Label("Green:");
    txtThemeBaseColorG = new gcn::TextField();
    txtThemeBaseColorG->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeBaseColorG->setId("txtThemeBaseColorG");
    txtThemeBaseColorG->setBaseColor(gui_base_color);
    txtThemeBaseColorG->setBackgroundColor(gui_textbox_background_color);
    txtThemeBaseColorG->setForegroundColor(gui_foreground_color);

    lblThemeBaseColorB = new gcn::Label("Blue:");
    txtThemeBaseColorB = new gcn::TextField();
    txtThemeBaseColorB->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeBaseColorB->setId("txtThemeBaseColorB");
    txtThemeBaseColorB->setBaseColor(gui_base_color);
    txtThemeBaseColorB->setBackgroundColor(gui_textbox_background_color);
    txtThemeBaseColorB->setForegroundColor(gui_foreground_color);

    lblThemeSelectorInactiveColor = new gcn::Label("Selector inactive color");
    // Theme selector inactive color RGB values
    lblThemeSelectorInactiveColorR = new gcn::Label("Red:");
    txtThemeSelectorInactiveColorR = new gcn::TextField();
    txtThemeSelectorInactiveColorR->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectorInactiveColorR->setId("txtThemeSelectorInactiveColorR");
    txtThemeSelectorInactiveColorR->setBaseColor(gui_base_color);
    txtThemeSelectorInactiveColorR->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectorInactiveColorR->setForegroundColor(gui_foreground_color);

    lblThemeSelectorInactiveColorG = new gcn::Label("Green:");
    txtThemeSelectorInactiveColorG = new gcn::TextField();
    txtThemeSelectorInactiveColorG->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectorInactiveColorG->setId("txtThemeSelectorInactiveColorG");
    txtThemeSelectorInactiveColorG->setBaseColor(gui_base_color);
    txtThemeSelectorInactiveColorG->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectorInactiveColorG->setForegroundColor(gui_foreground_color);

    lblThemeSelectorInactiveColorB = new gcn::Label("Blue:");
    txtThemeSelectorInactiveColorB = new gcn::TextField();
    txtThemeSelectorInactiveColorB->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectorInactiveColorB->setId("txtThemeSelectorInactiveColorB");
    txtThemeSelectorInactiveColorB->setBaseColor(gui_base_color);
    txtThemeSelectorInactiveColorB->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectorInactiveColorB->setForegroundColor(gui_foreground_color);

    lblThemeSelectorActiveColor = new gcn::Label("Selector active color");
    // Theme selector active color RGB values
    lblThemeSelectorActiveColorR = new gcn::Label("Red:");
    txtThemeSelectorActiveColorR = new gcn::TextField();
    txtThemeSelectorActiveColorR->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectorActiveColorR->setId("txtThemeSelectorActiveColorR");
    txtThemeSelectorActiveColorR->setBaseColor(gui_base_color);
    txtThemeSelectorActiveColorR->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectorActiveColorR->setForegroundColor(gui_foreground_color);

    lblThemeSelectorActiveColorG = new gcn::Label("Green:");
    txtThemeSelectorActiveColorG = new gcn::TextField();
    txtThemeSelectorActiveColorG->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectorActiveColorG->setId("txtThemeSelectorActiveColorG");
    txtThemeSelectorActiveColorG->setBaseColor(gui_base_color);
    txtThemeSelectorActiveColorG->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectorActiveColorG->setForegroundColor(gui_foreground_color);

    lblThemeSelectorActiveColorB = new gcn::Label("Blue:");
    txtThemeSelectorActiveColorB = new gcn::TextField();
    txtThemeSelectorActiveColorB->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectorActiveColorB->setId("txtThemeSelectorActiveColorB");
    txtThemeSelectorActiveColorB->setBaseColor(gui_base_color);
    txtThemeSelectorActiveColorB->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectorActiveColorB->setForegroundColor(gui_foreground_color);

    lblThemeSelectionColor = new gcn::Label("Selection color");
    // Theme selection color RGB values
    lblThemeSelectionColorR = new gcn::Label("Red:");
    txtThemeSelectionColorR = new gcn::TextField();
    txtThemeSelectionColorR->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectionColorR->setId("txtThemeSelectionColorR");
    txtThemeSelectionColorR->setBaseColor(gui_base_color);
    txtThemeSelectionColorR->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectionColorR->setForegroundColor(gui_foreground_color);

    lblThemeSelectionColorG = new gcn::Label("Green:");
    txtThemeSelectionColorG = new gcn::TextField();
    txtThemeSelectionColorG->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectionColorG->setId("txtThemeSelectionColorG");
    txtThemeSelectionColorG->setBaseColor(gui_base_color);
    txtThemeSelectionColorG->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectionColorG->setForegroundColor(gui_foreground_color);

    lblThemeSelectionColorB = new gcn::Label("Blue:");
    txtThemeSelectionColorB = new gcn::TextField();
    txtThemeSelectionColorB->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeSelectionColorB->setId("txtThemeSelectionColorB");
    txtThemeSelectionColorB->setBaseColor(gui_base_color);
    txtThemeSelectionColorB->setBackgroundColor(gui_textbox_background_color);
    txtThemeSelectionColorB->setForegroundColor(gui_foreground_color);

    lblThemeTextBgColor = new gcn::Label("Text background color");
    // Theme text background color RGB values
    lblThemeTextBgColorR = new gcn::Label("Red:");
    txtThemeTextBgColorR = new gcn::TextField();
    txtThemeTextBgColorR->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeTextBgColorR->setId("txtThemeTextBgColorR");
    txtThemeTextBgColorR->setBaseColor(gui_base_color);
    txtThemeTextBgColorR->setBackgroundColor(gui_textbox_background_color);
    txtThemeTextBgColorR->setForegroundColor(gui_foreground_color);

    lblThemeTextBgColorG = new gcn::Label("Green:");
    txtThemeTextBgColorG = new gcn::TextField();
    txtThemeTextBgColorG->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeTextBgColorG->setId("txtThemeTextBgColorG");
    txtThemeTextBgColorG->setBaseColor(gui_base_color);
    txtThemeTextBgColorG->setBackgroundColor(gui_textbox_background_color);
    txtThemeTextBgColorG->setForegroundColor(gui_foreground_color);

    lblThemeTextBgColorB = new gcn::Label("Blue:");
    txtThemeTextBgColorB = new gcn::TextField();
    txtThemeTextBgColorB->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeTextBgColorB->setId("txtThemeTextBgColorB");
    txtThemeTextBgColorB->setBaseColor(gui_base_color);
    txtThemeTextBgColorB->setBackgroundColor(gui_textbox_background_color);
    txtThemeTextBgColorB->setForegroundColor(gui_foreground_color);

    lblThemeFgColor = new gcn::Label("Foreground color");
    // Theme foreground color RGB values
    lblThemeFgColorR = new gcn::Label("Red:");
    txtThemeFgColorR = new gcn::TextField();
    txtThemeFgColorR->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFgColorR->setId("txtThemeFgColorR");
    txtThemeFgColorR->setBaseColor(gui_base_color);
    txtThemeFgColorR->setBackgroundColor(gui_textbox_background_color);
    txtThemeFgColorR->setForegroundColor(gui_foreground_color);

    lblThemeFgColorG = new gcn::Label("Green:");
    txtThemeFgColorG = new gcn::TextField();
    txtThemeFgColorG->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFgColorG->setId("txtThemeFgColorG");
    txtThemeFgColorG->setBaseColor(gui_base_color);
    txtThemeFgColorG->setBackgroundColor(gui_textbox_background_color);
    txtThemeFgColorG->setForegroundColor(gui_foreground_color);

    lblThemeFgColorB = new gcn::Label("Blue:");
    txtThemeFgColorB = new gcn::TextField();
    txtThemeFgColorB->setSize(50, TEXTFIELD_HEIGHT);
    txtThemeFgColorB->setId("txtThemeFgColorB");
    txtThemeFgColorB->setBaseColor(gui_base_color);
    txtThemeFgColorB->setBackgroundColor(gui_textbox_background_color);
    txtThemeFgColorB->setForegroundColor(gui_foreground_color);

    // Save button
    cmdThemeSave = new gcn::Button("Save");
    cmdThemeSave->setId("cmdThemeSave");
    cmdThemeSave->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    cmdThemeSave->setBaseColor(gui_base_color);
    cmdThemeSave->setForegroundColor(gui_foreground_color);
    cmdThemeSave->addActionListener(themesButtonActionListener);
    // Reset button
    cmdThemeReset = new gcn::Button("Reset");
    cmdThemeReset->setId("cmdThemeReset");
    cmdThemeReset->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    cmdThemeReset->setBaseColor(gui_base_color);
    cmdThemeReset->setForegroundColor(gui_foreground_color);
    cmdThemeReset->addActionListener(themesButtonActionListener);
    // Load button
    cmdThemeLoad = new gcn::Button("Load");
    cmdThemeLoad->setId("cmdThemeLoad");
    cmdThemeLoad->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    cmdThemeLoad->setBaseColor(gui_base_color);
    cmdThemeLoad->setForegroundColor(gui_foreground_color);
    cmdThemeLoad->addActionListener(themesButtonActionListener);


	int pos_x = DISTANCE_BORDER;
	int pos_y = DISTANCE_BORDER;

	category.panel->add(lblThemeFont, pos_x, pos_x);
	pos_x += lblThemeFont->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(txtThemeFont, pos_x, pos_y);
	pos_x += txtThemeFont->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(cmdThemeFont, pos_x, pos_y);
    pos_y += txtThemeFont->getHeight() + DISTANCE_NEXT_Y;

    category.panel->add(lblThemeFontSize, DISTANCE_BORDER, pos_y);
    pos_x = lblThemeFontSize->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(txtThemeFontSize, pos_x, pos_y);

    pos_x += txtThemeFontSize->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeFontColorR, pos_x, pos_y);
    pos_x += lblThemeFontColorR->getWidth() + 8;
    category.panel->add(txtThemeFontColorR, pos_x, pos_y);
    pos_x += txtThemeFontColorR->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeFontColorG, pos_x, pos_y);
    pos_x += lblThemeFontColorG->getWidth() + 8;
    category.panel->add(txtThemeFontColorG, pos_x, pos_y);
    pos_x += txtThemeFontColorG->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeFontColorB, pos_x, pos_y);
    pos_x += lblThemeFontColorB->getWidth() + 8;
    category.panel->add(txtThemeFontColorB, pos_x, pos_y);

    pos_y += txtThemeFontSize->getHeight() + DISTANCE_NEXT_Y * 2;

    pos_x = DISTANCE_BORDER;
    category.panel->add(lblThemeBaseColor, pos_x, pos_y);
    pos_y += lblThemeBaseColor->getHeight() + DISTANCE_NEXT_Y;
    category.panel->add(lblThemeBaseColorR, pos_x, pos_y);
    pos_x += lblThemeBaseColorR->getWidth() + 8;
    category.panel->add(txtThemeBaseColorR, pos_x, pos_y);
    pos_x += txtThemeBaseColorR->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeBaseColorG, pos_x, pos_y);
    pos_x += lblThemeBaseColorG->getWidth() + 8;
    category.panel->add(txtThemeBaseColorG, pos_x, pos_y);
    pos_x += txtThemeBaseColorG->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeBaseColorB, pos_x, pos_y);
    pos_x += lblThemeBaseColorB->getWidth() + 8;
    category.panel->add(txtThemeBaseColorB, pos_x, pos_y);

    pos_y += txtThemeBaseColorR->getHeight() + DISTANCE_NEXT_Y * 2;

    pos_x = DISTANCE_BORDER;
    category.panel->add(lblThemeSelectorInactiveColor, pos_x, pos_y);
    pos_y += lblThemeSelectorInactiveColor->getHeight() + DISTANCE_NEXT_Y;
    category.panel->add(lblThemeSelectorInactiveColorR, pos_x, pos_y);
    pos_x += lblThemeSelectorInactiveColorR->getWidth() + 8;
    category.panel->add(txtThemeSelectorInactiveColorR, pos_x, pos_y);
    pos_x += txtThemeSelectorInactiveColorR->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeSelectorInactiveColorG, pos_x, pos_y);
    pos_x += lblThemeSelectorInactiveColorG->getWidth() + 8;
    category.panel->add(txtThemeSelectorInactiveColorG, pos_x, pos_y);
    pos_x += txtThemeSelectorInactiveColorG->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeSelectorInactiveColorB, pos_x, pos_y);
    pos_x += lblThemeSelectorInactiveColorB->getWidth() + 8;
    category.panel->add(txtThemeSelectorInactiveColorB, pos_x, pos_y);

    pos_y += txtThemeSelectorInactiveColorR->getHeight() + DISTANCE_NEXT_Y * 2;

    pos_x = DISTANCE_BORDER;
    category.panel->add(lblThemeSelectorActiveColor, pos_x, pos_y);
    pos_y += lblThemeSelectorActiveColor->getHeight() + DISTANCE_NEXT_Y;
    category.panel->add(lblThemeSelectorActiveColorR, pos_x, pos_y);
    pos_x += lblThemeSelectorActiveColorR->getWidth() + 8;
    category.panel->add(txtThemeSelectorActiveColorR, pos_x, pos_y);
    pos_x += txtThemeSelectorActiveColorR->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeSelectorActiveColorG, pos_x, pos_y);
    pos_x += lblThemeSelectorActiveColorG->getWidth() + 8;
    category.panel->add(txtThemeSelectorActiveColorG, pos_x, pos_y);
    pos_x += txtThemeSelectorActiveColorG->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeSelectorActiveColorB, pos_x, pos_y);
    pos_x += lblThemeSelectorActiveColorB->getWidth() + 8;
    category.panel->add(txtThemeSelectorActiveColorB, pos_x, pos_y);

    pos_y += txtThemeSelectorActiveColorR->getHeight() + DISTANCE_NEXT_Y * 2;

    pos_x = DISTANCE_BORDER;
    category.panel->add(lblThemeSelectionColor, pos_x, pos_y);
    pos_y += lblThemeSelectionColor->getHeight() + DISTANCE_NEXT_Y;
    category.panel->add(lblThemeSelectionColorR, pos_x, pos_y);
    pos_x += lblThemeSelectionColorR->getWidth() + 8;
    category.panel->add(txtThemeSelectionColorR, pos_x, pos_y);
    pos_x += txtThemeSelectionColorR->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeSelectionColorG, pos_x, pos_y);
    pos_x += lblThemeSelectionColorG->getWidth() + 8;
    category.panel->add(txtThemeSelectionColorG, pos_x, pos_y);
    pos_x += txtThemeSelectionColorG->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeSelectionColorB, pos_x, pos_y);
    pos_x += lblThemeSelectionColorB->getWidth() + 8;
    category.panel->add(txtThemeSelectionColorB, pos_x, pos_y);

    pos_y += txtThemeSelectionColorR->getHeight() + DISTANCE_NEXT_Y * 2;

    pos_x = DISTANCE_BORDER;
    category.panel->add(lblThemeTextBgColor, pos_x, pos_y);
    pos_y += lblThemeTextBgColor->getHeight() + DISTANCE_NEXT_Y;
    category.panel->add(lblThemeTextBgColorR, pos_x, pos_y);
    pos_x += lblThemeTextBgColorR->getWidth() + 8;
    category.panel->add(txtThemeTextBgColorR, pos_x, pos_y);
    pos_x += txtThemeTextBgColorR->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeTextBgColorG, pos_x, pos_y);
    pos_x += lblThemeTextBgColorG->getWidth() + 8;
    category.panel->add(txtThemeTextBgColorG, pos_x, pos_y);
    pos_x += txtThemeTextBgColorG->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeTextBgColorB, pos_x, pos_y);
    pos_x += lblThemeTextBgColorB->getWidth() + 8;
    category.panel->add(txtThemeTextBgColorB, pos_x, pos_y);

    pos_y += txtThemeTextBgColorR->getHeight() + DISTANCE_NEXT_Y * 2;

    pos_x = DISTANCE_BORDER;
    category.panel->add(lblThemeFgColor, pos_x, pos_y);
    pos_y += lblThemeFgColor->getHeight() + DISTANCE_NEXT_Y;
    category.panel->add(lblThemeFgColorR, pos_x, pos_y);
    pos_x += lblThemeFgColorR->getWidth() + 8;
    category.panel->add(txtThemeFgColorR, pos_x, pos_y);
    pos_x += txtThemeFgColorR->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeFgColorG, pos_x, pos_y);
    pos_x += lblThemeFgColorG->getWidth() + 8;
    category.panel->add(txtThemeFgColorG, pos_x, pos_y);
    pos_x += txtThemeFgColorG->getWidth() + DISTANCE_NEXT_X;
    category.panel->add(lblThemeFgColorB, pos_x, pos_y);
    pos_x += lblThemeFgColorB->getWidth() + 8;
    category.panel->add(txtThemeFgColorB, pos_x, pos_y);

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

    delete lblThemeFontColorR;
    delete txtThemeFontColorR;
    delete lblThemeFontColorG;
    delete txtThemeFontColorG;
    delete lblThemeFontColorB;
    delete txtThemeFontColorB;

    delete lblThemeBaseColor;
	delete lblThemeBaseColorR;
	delete txtThemeBaseColorR;
	delete lblThemeBaseColorG;
	delete txtThemeBaseColorG;
	delete lblThemeBaseColorB;
	delete txtThemeBaseColorB;

    delete lblThemeSelectorInactiveColor;
    delete lblThemeSelectorInactiveColorR;
	delete txtThemeSelectorInactiveColorR;
	delete lblThemeSelectorInactiveColorG;
	delete txtThemeSelectorInactiveColorG;
	delete lblThemeSelectorInactiveColorB;
	delete txtThemeSelectorInactiveColorB;

    delete lblThemeSelectorActiveColor;
	delete lblThemeSelectorActiveColorR;
	delete txtThemeSelectorActiveColorR;
	delete lblThemeSelectorActiveColorG;
	delete txtThemeSelectorActiveColorG;
	delete lblThemeSelectorActiveColorB;
	delete txtThemeSelectorActiveColorB;

    delete lblThemeSelectionColor;
	delete lblThemeSelectionColorR;
	delete txtThemeSelectionColorR;
	delete lblThemeSelectionColorG;
	delete txtThemeSelectionColorG;
	delete lblThemeSelectionColorB;
	delete txtThemeSelectionColorB;

    delete lblThemeTextBgColor;
	delete lblThemeTextBgColorR;
	delete txtThemeTextBgColorR;
	delete lblThemeTextBgColorG;
	delete txtThemeTextBgColorG;
	delete lblThemeTextBgColorB;
	delete txtThemeTextBgColorB;

    delete lblThemeFgColor;
    delete lblThemeFgColorR;
    delete txtThemeFgColorR;
    delete lblThemeFgColorG;
    delete txtThemeFgColorG;
    delete lblThemeFgColorB;
    delete txtThemeFgColorB;

    delete cmdThemeSave;
    delete cmdThemeReset;
    delete cmdThemeLoad;

	delete themesButtonActionListener;
}

void RefreshPanelThemes()
{
	txtThemeFont->setText(gui_theme.font_name);
    txtThemeFontSize->setText(std::to_string(gui_theme.font_size));
    txtThemeFontColorR->setText(std::to_string(gui_theme.font_color.r));
    txtThemeFontColorG->setText(std::to_string(gui_theme.font_color.g));
    txtThemeFontColorB->setText(std::to_string(gui_theme.font_color.b));

    txtThemeBaseColorR->setText(std::to_string(gui_theme.base_color.r));
    txtThemeBaseColorG->setText(std::to_string(gui_theme.base_color.g));
    txtThemeBaseColorB->setText(std::to_string(gui_theme.base_color.b));

    txtThemeSelectorInactiveColorR->setText(std::to_string(gui_theme.selector_inactive.r));
    txtThemeSelectorInactiveColorG->setText(std::to_string(gui_theme.selector_inactive.g));
    txtThemeSelectorInactiveColorB->setText(std::to_string(gui_theme.selector_inactive.b));

    txtThemeSelectorActiveColorR->setText(std::to_string(gui_theme.selector_active.r));
    txtThemeSelectorActiveColorG->setText(std::to_string(gui_theme.selector_active.g));
    txtThemeSelectorActiveColorB->setText(std::to_string(gui_theme.selector_active.b));

    txtThemeSelectionColorR->setText(std::to_string(gui_theme.selection_color.r));
    txtThemeSelectionColorG->setText(std::to_string(gui_theme.selection_color.g));
    txtThemeSelectionColorB->setText(std::to_string(gui_theme.selection_color.b));

    txtThemeTextBgColorR->setText(std::to_string(gui_theme.textbox_background.r));
    txtThemeTextBgColorG->setText(std::to_string(gui_theme.textbox_background.g));
    txtThemeTextBgColorB->setText(std::to_string(gui_theme.textbox_background.b));

    txtThemeFgColorR->setText(std::to_string(gui_theme.foreground_color.r));
    txtThemeFgColorG->setText(std::to_string(gui_theme.foreground_color.g));
    txtThemeFgColorB->setText(std::to_string(gui_theme.foreground_color.b));
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