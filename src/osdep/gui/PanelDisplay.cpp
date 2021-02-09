#include <cstdio>
#include <cstdlib>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "gui_handling.h"

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

	int add_element(const char* Elem)
	{
		values.emplace_back(Elem);
		return 0;
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

const int amigawidth_values[] = { 640, 704, 720 };
const int amigaheight_values[] = { 400, 480, 512, 568 };
#define AMIGAWIDTH_COUNT 3
#define AMIGAHEIGHT_COUNT 4

const int fullscreen_width_values[] = { 640, 720, 800, 1024, 1280, 1280, 1920 };
const int fullscreen_height_values[] = { 480, 576, 600, 768, 720, 1024, 1080 };
const char* fullscreen_resolutions[] = { "640x480", "720x576", "800x600", "1024x768", "1280x720", "1280x1024", "1920x1080" };
StringListModel fullscreen_resolutions_list(fullscreen_resolutions, 7);

const char* fullscreen_modes[] = { "Windowed", "Fullscreen", "Full-window" };
StringListModel fullscreen_modes_list(fullscreen_modes, 3);

const char* resolution[] = { "LowRes", "HighRes (normal)", "SuperHighRes" };
StringListModel resolution_list(resolution, 3);

const char* scaling_method[] = { "Auto", "Pixelated", "Smooth" };
StringListModel scaling_method_list(scaling_method, 3);

static gcn::Window* grpAmigaScreen;
static gcn::Label* lblAmigaWidth;
static gcn::TextField* txtAmigaWidth;
static gcn::Slider* sldAmigaWidth;

static gcn::Label* lblAmigaHeight;
static gcn::TextField* txtAmigaHeight;
static gcn::Slider* sldAmigaHeight;

static gcn::CheckBox* chkAutoHeight;
static gcn::CheckBox* chkBorderless;

static gcn::Label* lblScreenmode;
static gcn::DropDown* cboScreenmode;
static gcn::Label* lblFullscreen;
static gcn::DropDown* cboFullscreen;

static gcn::Window* grpCentering;
static gcn::CheckBox* chkHorizontal;
static gcn::CheckBox* chkVertical;
static gcn::CheckBox* chkFlickerFixer;

static gcn::Window* grpLineMode;
static gcn::RadioButton* optSingle;
static gcn::RadioButton* optDouble;
static gcn::RadioButton* optScanlines;
static gcn::RadioButton* optDouble2;
static gcn::RadioButton* optDouble3;

static gcn::Window* grpILineMode;
static gcn::RadioButton* optISingle;
static gcn::RadioButton* optIDouble;
static gcn::RadioButton* optIDouble2;
static gcn::RadioButton* optIDouble3;

static gcn::Label* lblScalingMethod;
static gcn::DropDown* cboScalingMethod;

static gcn::Label* lblResolution;
static gcn::DropDown* cboResolution;
static gcn::CheckBox* chkFilterLowRes;

static gcn::CheckBox* chkFrameskip;
static gcn::Slider* sldRefresh;
static gcn::CheckBox* chkAspect;
static gcn::CheckBox* chkBlackerThanBlack;

class AmigaScreenActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == sldAmigaWidth)
		{
			if (changed_prefs.gfx_monitor[0].gfx_size_win.width != amigawidth_values[static_cast<int>(sldAmigaWidth->getValue())])
				changed_prefs.gfx_monitor[0].gfx_size_win.width = amigawidth_values[static_cast<int>(sldAmigaWidth->getValue())];
		}
		else if (actionEvent.getSource() == sldAmigaHeight)
		{
			if (changed_prefs.gfx_monitor[0].gfx_size_win.height != amigaheight_values[static_cast<int>(sldAmigaHeight->getValue())])
				changed_prefs.gfx_monitor[0].gfx_size_win.height = amigaheight_values[static_cast<int>(sldAmigaHeight->getValue())];
		}
		else if (actionEvent.getSource() == txtAmigaWidth)
			changed_prefs.gfx_monitor[0].gfx_size_win.width = std::stoi(txtAmigaWidth->getText());
		else if (actionEvent.getSource() == txtAmigaHeight)
			changed_prefs.gfx_monitor[0].gfx_size_win.height = std::stoi(txtAmigaHeight->getText());

		else if (actionEvent.getSource() == chkAutoHeight)
			changed_prefs.gfx_auto_height = chkAutoHeight->isSelected();

		else if (actionEvent.getSource() == chkBorderless)
			changed_prefs.borderless = chkBorderless->isSelected();
		
		else if (actionEvent.getSource() == chkFrameskip)
		{
			changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 2 : 1;
			sldRefresh->setEnabled(chkFrameskip->isSelected());
			sldRefresh->setValue(changed_prefs.gfx_framerate);
		}

		else if (actionEvent.getSource() == sldRefresh)
			changed_prefs.gfx_framerate = static_cast<int>(sldRefresh->getValue());

		else if (actionEvent.getSource() == chkAspect)
			changed_prefs.gfx_correct_aspect = chkAspect->isSelected();

		else if (actionEvent.getSource() == chkBlackerThanBlack)
			changed_prefs.gfx_blackerthanblack = chkBlackerThanBlack->isSelected();

		else if (actionEvent.getSource() == cboScreenmode)
		{
			if (cboScreenmode->getSelected() == 0)
			{
				changed_prefs.gfx_apmode[0].gfx_fullscreen = GFX_WINDOW;
				changed_prefs.gfx_apmode[1].gfx_fullscreen = GFX_WINDOW;
			}
			else if (cboScreenmode->getSelected() == 1)
			{
				changed_prefs.gfx_apmode[0].gfx_fullscreen = GFX_FULLSCREEN;
				changed_prefs.gfx_apmode[1].gfx_fullscreen = GFX_FULLSCREEN;
			}
			else if (cboScreenmode->getSelected() == 2)
			{
				changed_prefs.gfx_apmode[0].gfx_fullscreen = GFX_FULLWINDOW;
				changed_prefs.gfx_apmode[1].gfx_fullscreen = GFX_FULLWINDOW;
			}
		}

		else if (actionEvent.getSource() == cboFullscreen)
		{
			const auto idx = cboFullscreen->getSelected();
			if (idx >= 0 && idx <= fullscreen_resolutions_list.getNumberOfElements())
			{
				auto* mon = &changed_prefs.gfx_monitor[0];
				mon->gfx_size_fs.width = fullscreen_width_values[idx];
				mon->gfx_size_fs.height = fullscreen_height_values[idx];
			}
		}

		else if (actionEvent.getSource() == chkHorizontal)
			changed_prefs.gfx_xcenter = chkHorizontal->isSelected() ? 2 : 0;

		else if (actionEvent.getSource() == chkVertical)
			changed_prefs.gfx_ycenter = chkVertical->isSelected() ? 2 : 0;

		else if (actionEvent.getSource() == chkFlickerFixer)
			changed_prefs.gfx_scandoubler = chkFlickerFixer->isSelected();

		else if (actionEvent.getSource() == cboResolution)
			changed_prefs.gfx_resolution = cboResolution->getSelected();

		else if (actionEvent.getSource() == chkFilterLowRes)
			changed_prefs.gfx_lores_mode = chkFilterLowRes->isSelected() ? 1 : 0;
		
		RefreshPanelDisplay();
	}
};

AmigaScreenActionListener* amigaScreenActionListener;

class ScalingMethodActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cboScalingMethod)
		{
			changed_prefs.scaling_method = cboScalingMethod->getSelected() - 1;
		}
	}
};

static ScalingMethodActionListener* scalingMethodActionListener;

void disable_idouble_modes()
{
	optISingle->setEnabled(true);
	optISingle->setSelected(true);
	changed_prefs.gfx_iscanlines = 0;
	
	optIDouble->setEnabled(false);
	optIDouble2->setEnabled(false);
	optIDouble3->setEnabled(false);
}

void enable_idouble_modes()
{
	if (optISingle->isSelected())
	{
		optISingle->setSelected(false);
		optIDouble->setSelected(true);
	}
	optISingle->setEnabled(false);
	
	optIDouble->setEnabled(true);
	optIDouble2->setEnabled(true);
	optIDouble3->setEnabled(true);
}

class LineModeActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& action_event) override
	{
		if (action_event.getSource() == optSingle)
		{
			changed_prefs.gfx_vresolution = VRES_NONDOUBLE;
			changed_prefs.gfx_pscanlines = 0;

			disable_idouble_modes();
		}
		else if (action_event.getSource() == optDouble)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 0;
			
			enable_idouble_modes();
		}
		else if (action_event.getSource() == optScanlines)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 1;

			enable_idouble_modes();
		}
		else if (action_event.getSource() == optDouble2)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 2;

			enable_idouble_modes();
		}
		else if (action_event.getSource() == optDouble3)
		{
			changed_prefs.gfx_vresolution = VRES_DOUBLE;
			changed_prefs.gfx_pscanlines = 3;

			enable_idouble_modes();
		}
		else if (action_event.getSource() == optISingle || action_event.getSource() == optIDouble)
		{
			changed_prefs.gfx_iscanlines = 0;
		}
		else if (action_event.getSource() == optIDouble2)
		{
			changed_prefs.gfx_iscanlines = 1;
		}
		else if (action_event.getSource() == optIDouble3)
		{
			changed_prefs.gfx_iscanlines = 2;
		}
	}
};

static LineModeActionListener* lineModeActionListener;

void InitPanelDisplay(const struct _ConfigCategory& category)
{
	amigaScreenActionListener = new AmigaScreenActionListener();
	scalingMethodActionListener = new ScalingMethodActionListener();
	lineModeActionListener = new LineModeActionListener();
	
	auto posY = DISTANCE_BORDER;

	lblFullscreen = new gcn::Label("Fullscreen:");
	lblFullscreen->setAlignment(gcn::Graphics::LEFT);
	cboFullscreen = new gcn::DropDown(&fullscreen_resolutions_list);
	cboFullscreen->setSize(150, cboFullscreen->getHeight());
	cboFullscreen->setBaseColor(gui_baseCol);
	cboFullscreen->setBackgroundColor(colTextboxBackground);
	cboFullscreen->setId("cboFullscreen");
	cboFullscreen->addActionListener(amigaScreenActionListener);
	
	lblAmigaWidth = new gcn::Label("Width:");
	lblAmigaWidth->setAlignment(gcn::Graphics::LEFT);
	sldAmigaWidth = new gcn::Slider(0, AMIGAWIDTH_COUNT - 1);
	sldAmigaWidth->setSize(135, SLIDER_HEIGHT);
	sldAmigaWidth->setBaseColor(gui_baseCol);
	sldAmigaWidth->setMarkerLength(20);
	sldAmigaWidth->setStepLength(1);
	sldAmigaWidth->setId("sldWidth");
	sldAmigaWidth->addActionListener(amigaScreenActionListener);
	txtAmigaWidth = new gcn::TextField();
	txtAmigaWidth->setSize(35, TEXTFIELD_HEIGHT);
	txtAmigaWidth->setBackgroundColor(colTextboxBackground);
	txtAmigaWidth->addActionListener(amigaScreenActionListener);

	lblAmigaHeight = new gcn::Label("Height:");
	lblAmigaHeight->setAlignment(gcn::Graphics::LEFT);
	sldAmigaHeight = new gcn::Slider(0, AMIGAHEIGHT_COUNT - 1);
	sldAmigaHeight->setSize(135, SLIDER_HEIGHT);
	sldAmigaHeight->setBaseColor(gui_baseCol);
	sldAmigaHeight->setMarkerLength(20);
	sldAmigaHeight->setStepLength(1);
	sldAmigaHeight->setId("sldHeight");
	sldAmigaHeight->addActionListener(amigaScreenActionListener);
	txtAmigaHeight = new gcn::TextField();
	txtAmigaHeight->setSize(35, TEXTFIELD_HEIGHT);
	txtAmigaHeight->setBackgroundColor(colTextboxBackground);
	txtAmigaHeight->addActionListener(amigaScreenActionListener);

	chkAutoHeight = new gcn::CheckBox("Auto Height");
	chkAutoHeight->setId("chkAutoHeight");
	chkAutoHeight->addActionListener(amigaScreenActionListener);

	chkBorderless = new gcn::CheckBox("Borderless");
	chkBorderless->setId("chkBorderless");
	chkBorderless->addActionListener(amigaScreenActionListener);
	
	chkHorizontal = new gcn::CheckBox("Horizontal");
	chkHorizontal->setId("chkHorizontal");
	chkHorizontal->addActionListener(amigaScreenActionListener);
	chkVertical = new gcn::CheckBox("Vertical");
	chkVertical->setId("chkVertical");
	chkVertical->addActionListener(amigaScreenActionListener);

	chkFlickerFixer = new gcn::CheckBox("Remove interlace artifacts");
	chkFlickerFixer->setId("chkFlickerFixer");
	chkFlickerFixer->addActionListener(amigaScreenActionListener);

	chkFilterLowRes = new gcn::CheckBox("Filtered Low Res");
	chkFilterLowRes->setId("chkFilterLowRes");
	chkFilterLowRes->addActionListener(amigaScreenActionListener);

	chkBlackerThanBlack = new gcn::CheckBox("Blacker than black");
	chkBlackerThanBlack->setId("chkBlackerThanBlack");
	chkBlackerThanBlack->addActionListener(amigaScreenActionListener);
	
	chkAspect = new gcn::CheckBox("Correct Aspect Ratio");
	chkAspect->setId("chkAspect");
	chkAspect->addActionListener(amigaScreenActionListener);

	chkFrameskip = new gcn::CheckBox("Frameskip");
	chkFrameskip->setId("chkFrameskip");
	chkFrameskip->addActionListener(amigaScreenActionListener);

	sldRefresh = new gcn::Slider(1, 10);
	sldRefresh->setSize(100, SLIDER_HEIGHT);
	sldRefresh->setBaseColor(gui_baseCol);
	sldRefresh->setMarkerLength(20);
	sldRefresh->setStepLength(1);
	sldRefresh->setId("sldRefresh");
	sldRefresh->addActionListener(amigaScreenActionListener);
	
	lblScreenmode = new gcn::Label("Screen mode:");
	lblScreenmode->setAlignment(gcn::Graphics::RIGHT);
	cboScreenmode = new gcn::DropDown(&fullscreen_modes_list);
	cboScreenmode->setSize(150, cboScreenmode->getHeight());
	cboScreenmode->setBaseColor(gui_baseCol);
	cboScreenmode->setBackgroundColor(colTextboxBackground);
	cboScreenmode->setId("cboScreenmode");
	cboScreenmode->addActionListener(amigaScreenActionListener);

	lblResolution = new gcn::Label("Resolution:");
	lblResolution->setAlignment(gcn::Graphics::RIGHT);
	cboResolution = new gcn::DropDown(&resolution_list);
	cboResolution->setSize(150, cboResolution->getHeight());
	cboResolution->setBaseColor(gui_baseCol);
	cboResolution->setBackgroundColor(colTextboxBackground);
	cboResolution->setId("cboResolution");
	cboResolution->addActionListener(amigaScreenActionListener);
	
	grpAmigaScreen = new gcn::Window("Amiga Screen");
	grpAmigaScreen->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	grpAmigaScreen->add(lblFullscreen, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(cboFullscreen, lblFullscreen->getX() + lblFullscreen->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboFullscreen->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->add(lblScreenmode, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(cboScreenmode, lblScreenmode->getX() + lblScreenmode->getWidth() + 8, posY);
	posY += cboScreenmode->getHeight() + DISTANCE_NEXT_Y;
	
	grpAmigaScreen->add(lblAmigaWidth, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldAmigaWidth, lblAmigaWidth->getX() + lblAmigaHeight->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(txtAmigaWidth, sldAmigaWidth->getX() + sldAmigaWidth->getWidth() + DISTANCE_NEXT_X, posY);
	posY += sldAmigaWidth->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->add(lblAmigaHeight, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldAmigaHeight, lblAmigaHeight->getX() + lblAmigaHeight->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(txtAmigaHeight, sldAmigaHeight->getX() + sldAmigaHeight->getWidth() + DISTANCE_NEXT_X,
		posY);
	
	posY += sldAmigaHeight->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(chkAutoHeight, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(chkBorderless, chkAutoHeight->getX() + chkAutoHeight->getWidth() + DISTANCE_NEXT_X, posY);
	posY += chkAutoHeight->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->setMovable(false);
	grpAmigaScreen->setSize(lblAmigaWidth->getX() + lblAmigaWidth->getWidth() + sldAmigaWidth->getWidth() + lblAmigaWidth->getWidth() + txtAmigaHeight->getWidth() + DISTANCE_BORDER, TITLEBAR_HEIGHT + chkAutoHeight->getY() + chkAutoHeight->getHeight() + DISTANCE_NEXT_Y);
	grpAmigaScreen->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpAmigaScreen->setBaseColor(gui_baseCol);
	category.panel->add(grpAmigaScreen);

	grpCentering = new gcn::Window("Centering");
	grpCentering->setPosition(DISTANCE_BORDER + grpAmigaScreen->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpCentering->add(chkHorizontal, DISTANCE_BORDER, DISTANCE_BORDER);
	grpCentering->add(chkVertical, DISTANCE_BORDER, chkHorizontal->getY() + chkHorizontal->getHeight() + DISTANCE_NEXT_Y);
	grpCentering->setMovable(false);
	grpCentering->setSize(chkHorizontal->getX() + chkHorizontal->getWidth() + DISTANCE_BORDER * 5, TITLEBAR_HEIGHT + chkVertical->getY() + chkVertical->getHeight() + DISTANCE_NEXT_Y);
	grpCentering->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCentering->setBaseColor(gui_baseCol);
	category.panel->add(grpCentering);	
	posY = DISTANCE_BORDER + grpAmigaScreen->getHeight() + DISTANCE_NEXT_Y;

	lblScalingMethod = new gcn::Label("Scaling method:");
	lblScalingMethod->setAlignment(gcn::Graphics::RIGHT);
	cboScalingMethod = new gcn::DropDown(&scaling_method_list);
	cboScalingMethod->setSize(150, cboScalingMethod->getHeight());
	cboScalingMethod->setBaseColor(gui_baseCol);
	cboScalingMethod->setBackgroundColor(colTextboxBackground);
	cboScalingMethod->setId("cboScalingMethod");
	cboScalingMethod->addActionListener(scalingMethodActionListener);
	category.panel->add(lblScalingMethod, DISTANCE_BORDER, posY);
	category.panel->add(cboScalingMethod, lblScalingMethod->getX() + lblScalingMethod->getWidth() + 8, posY);
	posY += cboScalingMethod->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblResolution, DISTANCE_BORDER, posY);
	category.panel->add(cboResolution, cboScalingMethod->getX(), posY);
	posY += cboResolution->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkFilterLowRes, DISTANCE_BORDER, posY);
	posY += chkFilterLowRes->getHeight() + DISTANCE_NEXT_Y;
	
	optSingle = new gcn::RadioButton("Single", "linemodegroup");
	optSingle->setId("optSingle");
	optSingle->addActionListener(lineModeActionListener);

	optDouble = new gcn::RadioButton("Double", "linemodegroup");
	optDouble->setId("optDouble");
	optDouble->addActionListener(lineModeActionListener);

	optScanlines = new gcn::RadioButton("Scanlines", "linemodegroup");
	optScanlines->setId("optScanlines");
	optScanlines->addActionListener(lineModeActionListener);

	optDouble2 = new gcn::RadioButton("Double, fields", "linemodegroup");
	optDouble2->setId("optDouble2");
	optDouble2->addActionListener(lineModeActionListener);

	optDouble3 = new gcn::RadioButton("Double, fields+", "linemodegroup");
	optDouble3->setId("optDouble3");
	optDouble3->addActionListener(lineModeActionListener);
	
	grpLineMode = new gcn::Window("Line mode");
	grpLineMode->setPosition(grpCentering->getX(), grpCentering->getY() + grpCentering->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optSingle, 10, 10);
	grpLineMode->add(optDouble, optSingle->getX(), optSingle->getY() + optSingle->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optScanlines, optSingle->getX(), optDouble->getY() + optDouble->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optDouble2, optSingle->getX(), optScanlines->getY() + optScanlines->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->add(optDouble3, optSingle->getX(), optDouble2->getY() + optDouble2->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->setMovable(false);
	grpLineMode->setSize(grpCentering->getWidth(), TITLEBAR_HEIGHT + optDouble3->getY() + optDouble3->getHeight() + DISTANCE_NEXT_Y);
	grpLineMode->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpLineMode->setBaseColor(gui_baseCol);	
	category.panel->add(grpLineMode);

	optISingle = new gcn::RadioButton("Single", "ilinemodegroup");
	optISingle->setId("optISingle");
	optISingle->addActionListener(lineModeActionListener);

	optIDouble = new gcn::RadioButton("Double, frames", "ilinemodegroup");
	optIDouble->setId("optIDouble");
	optIDouble->addActionListener(lineModeActionListener);

	optIDouble2 = new gcn::RadioButton("Double, fields", "ilinemodegroup");
	optIDouble2->setId("optIDouble2");
	optIDouble2->addActionListener(lineModeActionListener);
	
	optIDouble3 = new gcn::RadioButton("Double, fields+", "ilinemodegroup");
	optIDouble3->setId("optIDouble3");
	optIDouble3->addActionListener(lineModeActionListener);

	grpILineMode = new gcn::Window("Interlaced line mode");
	grpILineMode->setPosition(grpLineMode->getX(), grpLineMode->getY() + grpLineMode->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->add(optISingle, 10, 10);
	grpILineMode->add(optIDouble, optISingle->getX(), optISingle->getY() + optISingle->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->add(optIDouble2, optISingle->getX(), optIDouble->getY() + optIDouble->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->add(optIDouble3, optISingle->getX(), optIDouble2->getY() + optIDouble2->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->setMovable(false);
	grpILineMode->setSize(grpCentering->getWidth(), TITLEBAR_HEIGHT + optIDouble3->getY() + optIDouble3->getHeight() + DISTANCE_NEXT_Y);
	grpILineMode->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpILineMode->setBaseColor(gui_baseCol);
	category.panel->add(grpILineMode);

	category.panel->add(chkBlackerThanBlack, DISTANCE_BORDER, posY);
	posY += chkBlackerThanBlack->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkAspect, DISTANCE_BORDER, posY);
	posY += chkAspect->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkFlickerFixer, DISTANCE_BORDER, posY);
	posY += chkFlickerFixer->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkFrameskip, DISTANCE_BORDER, posY);
	category.panel->add(sldRefresh, chkFrameskip->getX() + chkFrameskip->getWidth() + DISTANCE_NEXT_X, posY);

	RefreshPanelDisplay();
}

void ExitPanelDisplay()
{
	delete chkFrameskip;
	delete sldRefresh;
	delete amigaScreenActionListener;
	delete lblAmigaWidth;
	delete sldAmigaWidth;
	delete txtAmigaWidth;
	delete lblAmigaHeight;
	delete sldAmigaHeight;
	delete txtAmigaHeight;
	delete chkAutoHeight;
	delete chkBorderless;
	delete grpAmigaScreen;

	delete chkHorizontal;
	delete chkVertical;
	delete chkFlickerFixer;
	delete grpCentering;

	delete chkBlackerThanBlack;
	delete chkAspect;
	delete lblScreenmode;
	delete cboScreenmode;
	delete lblFullscreen;
	delete cboFullscreen;

	delete optSingle;
	delete optDouble;
	delete optScanlines;
	delete optDouble2;
	delete optDouble3;
	delete grpLineMode;
	delete optISingle;
	delete optIDouble;
	delete optIDouble2;
	delete optIDouble3;
	delete grpILineMode;
	delete lineModeActionListener;

	delete lblScalingMethod;
	delete cboScalingMethod;
	delete scalingMethodActionListener;

	delete lblResolution;
	delete cboResolution;
	delete chkFilterLowRes;
}

void RefreshPanelDisplay()
{
	chkFrameskip->setSelected(changed_prefs.gfx_framerate > 1);
	sldRefresh->setEnabled(chkFrameskip->isSelected());
	sldRefresh->setValue(changed_prefs.gfx_framerate);
	
	int i;

	for (i = 0; i < AMIGAWIDTH_COUNT; ++i)
	{
		if (changed_prefs.gfx_monitor[0].gfx_size_win.width == amigawidth_values[i])
		{
			sldAmigaWidth->setValue(i);
			txtAmigaWidth->setText(std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.width));
			break;
		}
		if (i == AMIGAWIDTH_COUNT - 1)
		{
			txtAmigaWidth->setText(std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.width));
			break;
		}
	}

	for (i = 0; i < AMIGAHEIGHT_COUNT; ++i)
	{
		if (changed_prefs.gfx_monitor[0].gfx_size_win.height == amigaheight_values[i])
		{
			sldAmigaHeight->setValue(i);
			txtAmigaHeight->setText(std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.height));
			break;
		}
		if (i == AMIGAHEIGHT_COUNT - 1)
		{
			txtAmigaHeight->setText(std::to_string(changed_prefs.gfx_monitor[0].gfx_size_win.height));
			break;
		}
	}
	chkAutoHeight->setSelected(changed_prefs.gfx_auto_height);
	chkBorderless->setSelected(changed_prefs.borderless);
	
	chkHorizontal->setSelected(changed_prefs.gfx_xcenter == 2);
	chkVertical->setSelected(changed_prefs.gfx_ycenter == 2);

	chkFlickerFixer->setSelected(changed_prefs.gfx_scandoubler);
	chkBlackerThanBlack->setSelected(changed_prefs.gfx_blackerthanblack);
	
	chkAspect->setSelected(changed_prefs.gfx_correct_aspect);
	chkFilterLowRes->setSelected(changed_prefs.gfx_lores_mode);

	if (changed_prefs.gfx_apmode[0].gfx_fullscreen == GFX_WINDOW)
	{
		cboScreenmode->setSelected(0);
		cboFullscreen->setEnabled(false);

		lblAmigaHeight->setEnabled(!chkAutoHeight->isSelected());
		sldAmigaHeight->setEnabled(!chkAutoHeight->isSelected());
		txtAmigaHeight->setEnabled(!chkAutoHeight->isSelected());
	}
	else if (changed_prefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN)
	{
		cboScreenmode->setSelected(1);
		cboFullscreen->setEnabled(true);
	}
	else if (changed_prefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLWINDOW)
	{
		cboScreenmode->setSelected(2);
		cboFullscreen->setEnabled(false);

		lblAmigaHeight->setEnabled(!chkAutoHeight->isSelected());
		sldAmigaHeight->setEnabled(!chkAutoHeight->isSelected());
		txtAmigaHeight->setEnabled(!chkAutoHeight->isSelected());
	}

	//Disable Borderless checkbox in non-Windowed modes
	chkBorderless->setEnabled(cboScreenmode->getSelected() == 0);

	if (changed_prefs.gfx_monitor[0].gfx_size_fs.width && changed_prefs.gfx_monitor[0].gfx_size_fs.height)
	{
		auto found = false;
		for (auto idx = 0; idx <= fullscreen_resolutions_list.getNumberOfElements(); idx++)
		{
			if (changed_prefs.gfx_monitor[0].gfx_size_fs.width == fullscreen_width_values[idx]
				&& changed_prefs.gfx_monitor[0].gfx_size_fs.height == fullscreen_height_values[idx])
			{
				cboFullscreen->setSelected(idx);
				found = true;
			}
		}
		if (!found)
		{
			cboFullscreen->setSelected(2);
			changed_prefs.gfx_monitor[0].gfx_size_fs.width = fullscreen_width_values[2];
			changed_prefs.gfx_monitor[0].gfx_size_fs.height = fullscreen_height_values[2];
		}
	}
	
#ifdef USE_DISPMANX
	lblScalingMethod->setEnabled(false);
	cboScalingMethod->setEnabled(false);
#endif

	cboScalingMethod->setSelected(changed_prefs.scaling_method + 1);
	
	if (changed_prefs.gfx_vresolution == VRES_NONDOUBLE && changed_prefs.gfx_pscanlines == 0)
	{
		optSingle->setSelected(true);
		disable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 0)
	{
		optDouble->setSelected(true);
		enable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 1)
	{
		optScanlines->setSelected(true);
		enable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 2)
	{
		optDouble2->setSelected(true);
		enable_idouble_modes();
	}
	else if (changed_prefs.gfx_vresolution >= VRES_DOUBLE && changed_prefs.gfx_pscanlines == 3)
	{
		optDouble3->setSelected(true);
		enable_idouble_modes();
	}

	if (changed_prefs.gfx_iscanlines == 0)
	{
		if (changed_prefs.gfx_vresolution >= VRES_DOUBLE)
			optIDouble->setSelected(true);
		else
			optISingle->setSelected(true);
	}
	else if (changed_prefs.gfx_iscanlines == 1)
		optIDouble2->setSelected(true);
	else if (changed_prefs.gfx_iscanlines == 2)
		optIDouble3->setSelected(true);
	
	cboResolution->setSelected(changed_prefs.gfx_resolution);
}

bool HelpPanelDisplay(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required width and height of the Amiga screen.");
	helptext.emplace_back("You can select the screen mode to be used as well:");
	helptext.emplace_back("- Windowed: the Amiga screen will be shown in a Window on your desktop");
	helptext.emplace_back("- Fullscreen: the monitor resolution will change to the selected one.");
	helptext.emplace_back("- Full-window: the Amiga screen will be scaled to the current resolution.");
	helptext.emplace_back("The Auto-height option will have the emulator try to detect the height automatically.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can use the Horizontal/Vertical centering options to center the image automatically.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The Aspect Ratio option allows you to choose if you want the correct Aspect Ratio");
	helptext.emplace_back("to be kept always (default), or have the image stretched to fill the screen instead.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can select the scaling method for the Amiga screen. The default option \"Auto\", ");
	helptext.emplace_back("will try to find the best looking scaling method depending on your monitor's resolution. ");
	helptext.emplace_back("\"Nearest Neighbor\" will give you a more pixelated and crisp image, but it may come with ");
	helptext.emplace_back("some distortion if your resolution is not an exact multiple of what is displayed.");
	helptext.emplace_back("\"Linear\" will give you a smoother scaling but some people might find it a bit blurry.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Activate line doubling to remove flicker in interlace modes, or Scanlines for the CRT effect.");
	helptext.emplace_back("You can additionally use the Remove interlace artifacts option, to enable the flicker-fixer feature.");
	helptext.emplace_back(" ");
	helptext.emplace_back("When you activate \"Frameskip\", only every second frame is drawn.");
	helptext.emplace_back("You can use the slider to further fine-tune how many frames should be skipped.");
	helptext.emplace_back("This will improve performance and some more games are playable.");
	return true;
}
