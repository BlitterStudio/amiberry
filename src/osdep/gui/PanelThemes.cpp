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

static gcn::Label* lblThemeBaseColorR;
static gcn::TextField* txtThemeBaseColorR;
static gcn::Label* lblThemeBaseColorG;
static gcn::TextField* txtThemeBaseColorG;
static gcn::Label* lblThemeBaseColorB;
static gcn::TextField* txtThemeBaseColorB;

static gcn::Label* lblThemeSelectorInactiveColorR;
static gcn::TextField* txtThemeSelectorInactiveColorR;
static gcn::Label* lblThemeSelectorInactiveColorG;
static gcn::TextField* txtThemeSelectorInactiveColorG;
static gcn::Label* lblThemeSelectorInactiveColorB;
static gcn::TextField* txtThemeSelectorInactiveColorB;

static gcn::Label* lblThemeSelectorActiveColorR;
static gcn::TextField* txtThemeSelectorActiveColorR;
static gcn::Label* lblThemeSelectorActiveColorG;
static gcn::TextField* txtThemeSelectorActiveColorG;
static gcn::Label* lblThemeSelectorActiveColorB;
static gcn::TextField* txtThemeSelectorActiveColorB;

static gcn::Label* lblThemeSelectionColorR;
static gcn::TextField* txtThemeSelectionColorR;
static gcn::Label* lblThemeSelectionColorG;
static gcn::TextField* txtThemeSelectionColorG;
static gcn::Label* lblThemeSelectionColorB;
static gcn::TextField* txtThemeSelectionColorB;

static gcn::Label* lblThemeTextBgColorR;
static gcn::TextField* txtThemeTextBgColorR;
static gcn::Label* lblThemeTextBgColorG;
static gcn::TextField* txtThemeTextBgColorG;
static gcn::Label* lblThemeTextBgColorB;
static gcn::TextField* txtThemeTextBgColorB;

//TODO add a Save button here, to save the settings to amiberry.conf
static gcn::Button* cmdSaveAmiberryConfig

class ThemesButtonActionListener : public gcn::ActionListener
{
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdThemeFont)
		{
			const char* filter[] = { ".ttf", "\0" };
			std::string font = SelectFile("Select font", "/usr/share/fonts/truetype/", filter, false);
			if (!font.empty())
			{
				txtThemeFont->setText(font);
				gui_theme.font_name = font;
			}
		}

		//TODO Expand with more widgets handling as necessary. Note: textfields will need focus listeners to update the values

		RefreshPanelThemes();
	}
};

static ThemesButtonActionListener* themesButtonActionListener;

void InitPanelThemes(const config_category& category)
{
	themesButtonActionListener = new ThemesButtonActionListener();

	lblThemeFont = new gcn::Label("Font:");
  lblThemeFontSize = new gcn::Label("Font Size");
	txtThemeFont = new gcn::TextField();
  txtThemeFontSize = new gnc::TextField();
	txtThemeFont->setSize(480, TEXTFIELD_HEIGHT);
	txtThemeFont->setBaseColor(gui_base_color);
	txtThemeFont->setBackgroundColor(gui_textbox_background_color);
	txtThemeFont->setForegroundColor(gui_foreground_color);
  txtThemeFontSize->setSize(480, TEXTFIELD_HEIGHT);
  txtThemeFontSize->SetBaseColor(gui_base_color);
  txtThemeFontSize->setBackgroundColor(gui_textbox_background_color);
	txtThemeFontSize->setForegroundColor(gui_foreground_color);

	cmdThemeFont = new gcn::Button("...");
	cmdThemeFont->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdThemeFont->setBaseColor(gui_base_color);
	cmdThemeFont->setForegroundColor(gui_foreground_color);
	cmdThemeFont->addActionListener(themesButtonActionListener);

	// TODO add the rest of the widgets here
  cmdThemeFontSize = new gcn::Button("...");
  cmdThemeFontSize->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdThemeFontSize->setBaseColor(gui_base_color);
	cmdThemeFontSize->setForegroundColor(gui_foreground_color);
	cmdThemeFontSize->addActionListener(themesButtonActionListener);

	int pos_x = DISTANCE_BORDER;
	int pos_y = DISTANCE_BORDER;

	category.panel->add(lblThemeFont, pos_x, pos_x);
	pos_x += lblThemeFont->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(txtThemeFont, pos_x, pos_y);
	pos_x += txtThemeFont->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(cmdThemeFont, pos_x, pos_y);
	pos_x += cmdThemeFont->getWidth() + DISTANCE_NEXT_X;
  category.panel->add(lblThemeFontSize, pos_x, pos_y);
  pos_x += lblThemeFontSize->getWidth() + DISTANCE_NEXT_X;
  category.panel->add(txtThemeFontSize, pos_x, pos_y);
  pos_x += txtThemeFontSize->getWidth() + DISTANCE_NEXT_X;
  category.panel->add(cmdThemeFontSize, pos_x, pos_y);
  pos_x += cmdThemeFontSize->getWidth() + DISTANCE_NEXT_X;
  //lblThemeFontColorR;
	//txtThemeFontColorR;
	//lblThemeFontColorG;
	//txtThemeFontColorG;
	//lblThemeFontColorB;
	//txtThemeFontColorB;
	//lblThemeBaseColorR;
	//txtThemeBaseColorR;
	//lblThemeBaseColorG;
	//txtThemeBaseColorG;
	//lblThemeBaseColorB;
	//txtThemeBaseColorB;
	//lblThemeSelectorInactiveColorR;
	//txtThemeSelectorInactiveColorR;
	//lblThemeSelectorInactiveColorG;
	//txtThemeSelectorInactiveColorG;
	//lblThemeSelectorInactiveColorB;
	//txtThemeSelectorInactiveColorB;
	//lblThemeSelectorActiveColorR;
	//txtThemeSelectorActiveColorR;
	//lblThemeSelectorActiveColorG;
	//txtThemeSelectorActiveColorG;
	//lblThemeSelectorActiveColorB;
	//txtThemeSelectorActiveColorB;
	//lblThemeSelectionColorR;
	//txtThemeSelectionColorR;
	//lblThemeSelectionColorG;
	//txtThemeSelectionColorG;
	//lblThemeSelectionColorB;
	//txtThemeSelectionColorB;
	//lblThemeTextBgColorR;
	//txtThemeTextBgColorR;
	//lblThemeTextBgColorG;
	//txtThemeTextBgColorG;
	//lblThemeTextBgColorB;
	//txtThemeTextBgColorB;

	RefreshPanelThemes();
}

void ExitPanelThemes()
{
	delete lblThemeFont;
	delete txtThemeFont;
	delete cmdThemeFont;

	//TODO uncomment these, when the widgets are added
	delete lblThemeFontSize;
	delete txtThemeFontSize;
	//delete lblThemeFontColorR;
	//delete txtThemeFontColorR;
	//delete lblThemeFontColorG;
	//delete txtThemeFontColorG;
	//delete lblThemeFontColorB;
	//delete txtThemeFontColorB;
	//delete lblThemeBaseColorR;
	//delete txtThemeBaseColorR;
	//delete lblThemeBaseColorG;
	//delete txtThemeBaseColorG;
	//delete lblThemeBaseColorB;
	//delete txtThemeBaseColorB;
	//delete lblThemeSelectorInactiveColorR;
	//delete txtThemeSelectorInactiveColorR;
	//delete lblThemeSelectorInactiveColorG;
	//delete txtThemeSelectorInactiveColorG;
	//delete lblThemeSelectorInactiveColorB;
	//delete txtThemeSelectorInactiveColorB;
	//delete lblThemeSelectorActiveColorR;
	//delete txtThemeSelectorActiveColorR;
	//delete lblThemeSelectorActiveColorG;
	//delete txtThemeSelectorActiveColorG;
	//delete lblThemeSelectorActiveColorB;
	//delete txtThemeSelectorActiveColorB;
	//delete lblThemeSelectionColorR;
	//delete txtThemeSelectionColorR;
	//delete lblThemeSelectionColorG;
	//delete txtThemeSelectionColorG;
	//delete lblThemeSelectionColorB;
	//delete txtThemeSelectionColorB;
	//delete lblThemeTextBgColorR;
	//delete txtThemeTextBgColorR;
	//delete lblThemeTextBgColorG;
	//delete txtThemeTextBgColorG;
	//delete lblThemeTextBgColorB;
	//delete txtThemeTextBgColorB;
	delete themesButtonActionListener;
}

void RefreshPanelThemes()
{
	txtThemeFont->setText(gui_theme.font_name);
	//TODO add the rest of the widgets here
}

bool HelpPanelThemes(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.push_back("The themes panel allows you to customize the look of the GUI.");
	helptext.push_back(" ");
	helptext.push_back("Font: The font to use for the GUI.");
	helptext.push_back("Font size: The size of the font.");
	helptext.push_back("Font color: The color of the font.");
	helptext.push_back("Base color: The base color of the GUI.");
	helptext.push_back("Selector inactive color: The color of the inactive selector.");
	helptext.push_back("Selector active color: The color of the active selector.");
	helptext.push_back("Selection color: The color of the selection.");
	helptext.push_back("Text background color: The color of the text background.");
	return true;
}
