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

class string_list_model : public gcn::ListModel
{
	std::vector<std::string> values{};
public:
	string_list_model(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int add_element(const char* Elem) override
	{
		values.emplace_back(Elem);
		return 0;
	}

	void clear_elements() override
	{
		values.clear();
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
string_list_model fullscreen_resolutions_list(fullscreen_resolutions, 7);

const char* fullscreen_modes[] = { "Windowed", "Fullscreen", "Full-window" };
string_list_model fullscreen_modes_list(fullscreen_modes, 3);

const char* resolution[] = { "LowRes", "HighRes (normal)", "SuperHighRes" };
string_list_model resolution_list(resolution, 3);

const char* scaling_method[] = { "Auto", "Pixelated", "Smooth" };
string_list_model scaling_method_list(scaling_method, 3);

static gcn::Window* grpAmigaScreen;
static gcn::Label* lblAmigaWidth;
static gcn::TextField* txtAmigaWidth;
static gcn::Slider* sldAmigaWidth;

static gcn::Label* lblAmigaHeight;
static gcn::TextField* txtAmigaHeight;
static gcn::Slider* sldAmigaHeight;

static gcn::CheckBox* chkAutoCrop;
static gcn::CheckBox* chkBorderless;

static gcn::Label* lblHOffset;
static gcn::Slider* sldHOffset;
static gcn::Label* lblHOffsetValue;
static gcn::Label* lblVOffset;
static gcn::Slider* sldVOffset;
static gcn::Label* lblVOffsetValue;

static gcn::Label* lblScreenmode;
static gcn::DropDown* cboScreenmode;
static gcn::Label* lblFullscreen;
static gcn::DropDown* cboFullscreen;
static gcn::CheckBox* chkVsync;

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
static gcn::Label* lblFrameRate;
static gcn::CheckBox* chkAspect;
static gcn::CheckBox* chkBlackerThanBlack;

static gcn::Label* lblBrightness;
static gcn::Slider* sldBrightness;
static gcn::Label* lblBrightnessValue;

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

		else if (actionEvent.getSource() == chkAutoCrop)
		{
			changed_prefs.gfx_auto_crop = chkAutoCrop->isSelected();
#if !defined USE_DISPMANX
			changed_prefs.gfx_monitor[0].gfx_size_win.width = 720;
			changed_prefs.gfx_monitor[0].gfx_size_win.height = 568;
#endif
		}

		else if (actionEvent.getSource() == chkBorderless)
			changed_prefs.borderless = chkBorderless->isSelected();

		else if (actionEvent.getSource() == chkVsync)
			changed_prefs.gfx_apmode[0].gfx_vsync = chkVsync->isSelected();

		else if (actionEvent.getSource() == sldHOffset)
		{
			changed_prefs.gfx_horizontal_offset = static_cast<int>(sldHOffset->getValue());
			lblHOffsetValue->setCaption(std::to_string(changed_prefs.gfx_horizontal_offset));
			lblHOffsetValue->adjustSize();
		}
		else if (actionEvent.getSource() == sldVOffset)
		{
			changed_prefs.gfx_vertical_offset = static_cast<int>(sldVOffset->getValue());
			lblVOffsetValue->setCaption(std::to_string(changed_prefs.gfx_vertical_offset));
			lblVOffsetValue->adjustSize();
		}
		else if (actionEvent.getSource() == chkFrameskip)
		{
			changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 2 : 1;
			sldRefresh->setEnabled(chkFrameskip->isSelected());
			sldRefresh->setValue(changed_prefs.gfx_framerate);
			lblFrameRate->setCaption(std::to_string(changed_prefs.gfx_framerate));
			lblFrameRate->adjustSize();
		}
		else if (actionEvent.getSource() == sldBrightness)
		{
			changed_prefs.gfx_luminance = static_cast<int>(sldBrightness->getValue());
			lblBrightnessValue->setCaption(std::to_string(changed_prefs.gfx_luminance));
			lblBrightnessValue->adjustSize();
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

void InitPanelDisplay(const config_category& category)
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

#if defined USE_DISPMANX
	chkAutoCrop = new gcn::CheckBox("Auto Height");
#else
	chkAutoCrop = new gcn::CheckBox("Auto Crop");
#endif
	chkAutoCrop->setId("chkAutoCrop");
	chkAutoCrop->addActionListener(amigaScreenActionListener);

	chkBorderless = new gcn::CheckBox("Borderless");
	chkBorderless->setId("chkBorderless");
	chkBorderless->addActionListener(amigaScreenActionListener);

	chkVsync = new gcn::CheckBox("VSync");
	chkVsync->setId("chkVsync");
	chkVsync->addActionListener(amigaScreenActionListener);

	lblHOffset = new gcn::Label("H. Offset:");
	lblHOffset->setAlignment(gcn::Graphics::LEFT);
	sldHOffset = new gcn::Slider(-60, 60);
	sldHOffset->setSize(135, SLIDER_HEIGHT);
	sldHOffset->setBaseColor(gui_baseCol);
	sldHOffset->setMarkerLength(20);
	sldHOffset->setStepLength(1);
	sldHOffset->setId("sldHOffset");
	sldHOffset->addActionListener(amigaScreenActionListener);
	lblHOffsetValue = new gcn::Label("-60");
	lblHOffsetValue->setAlignment(gcn::Graphics::LEFT);

	lblVOffset = new gcn::Label("V. Offset:");
	lblVOffset->setAlignment(gcn::Graphics::LEFT);
	sldVOffset = new gcn::Slider(-20, 20);
	sldVOffset->setSize(135, SLIDER_HEIGHT);
	sldVOffset->setBaseColor(gui_baseCol);
	sldVOffset->setMarkerLength(20);
	sldVOffset->setStepLength(1);
	sldVOffset->setId("sldVOffset");
	sldVOffset->addActionListener(amigaScreenActionListener);
	lblVOffsetValue = new gcn::Label("-20");
	lblVOffsetValue->setAlignment(gcn::Graphics::LEFT);

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

	chkFrameskip = new gcn::CheckBox("Refresh:");
	chkFrameskip->setId("chkFrameskip");
	chkFrameskip->addActionListener(amigaScreenActionListener);

	sldRefresh = new gcn::Slider(1, 10);
	sldRefresh->setSize(100, SLIDER_HEIGHT);
	sldRefresh->setBaseColor(gui_baseCol);
	sldRefresh->setMarkerLength(20);
	sldRefresh->setStepLength(1);
	sldRefresh->setId("sldRefresh");
	sldRefresh->addActionListener(amigaScreenActionListener);
	lblFrameRate = new gcn::Label("50");
	lblFrameRate->setAlignment(gcn::Graphics::LEFT);

	lblBrightness = new gcn::Label("Brightness:");
	lblBrightness->setAlignment(gcn::Graphics::LEFT);
	sldBrightness = new gcn::Slider(-200, 200);
	sldBrightness->setSize(100, SLIDER_HEIGHT);
	sldBrightness->setBaseColor(gui_baseCol);
	sldBrightness->setMarkerLength(20);
	sldBrightness->setStepLength(1);
	sldBrightness->setId("sldBrightness");
	sldBrightness->addActionListener(amigaScreenActionListener);
	lblBrightnessValue = new gcn::Label("0.0");
	lblBrightnessValue->setAlignment(gcn::Graphics::LEFT);

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
	grpAmigaScreen->add(chkAutoCrop, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(chkBorderless, chkAutoCrop->getX() + chkAutoCrop->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(chkVsync, chkBorderless->getX() + chkBorderless->getWidth() + DISTANCE_NEXT_X, posY);
	posY += chkVsync->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblHOffset, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldHOffset, lblHOffset->getX() + lblHOffset->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(lblHOffsetValue, sldHOffset->getX() + sldHOffset->getWidth() + 8, posY + 2);
	posY += sldVOffset->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblVOffset, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldVOffset, lblVOffset->getX() + lblVOffset->getWidth() + DISTANCE_NEXT_X, posY);
	grpAmigaScreen->add(lblVOffsetValue, sldVOffset->getX() + sldVOffset->getWidth() + 8, posY + 2);

	grpAmigaScreen->setMovable(false);
	grpAmigaScreen->setSize(chkVsync->getX() + chkVsync->getWidth() + DISTANCE_BORDER, TITLEBAR_HEIGHT + lblVOffset->getY() + lblVOffset->getHeight() + DISTANCE_NEXT_Y);
	grpAmigaScreen->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpAmigaScreen->setBaseColor(gui_baseCol);
	category.panel->add(grpAmigaScreen);

	grpCentering = new gcn::Window("Centering");
	grpCentering->add(chkHorizontal, DISTANCE_BORDER, DISTANCE_BORDER);
	grpCentering->add(chkVertical, DISTANCE_BORDER, chkHorizontal->getY() + chkHorizontal->getHeight() + DISTANCE_NEXT_Y);
	grpCentering->setMovable(false);
	grpCentering->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCentering->setBaseColor(gui_baseCol);
	grpCentering->setSize(chkHorizontal->getX() + chkHorizontal->getWidth() + DISTANCE_BORDER * 5, TITLEBAR_HEIGHT + chkVertical->getY() + chkVertical->getHeight() + DISTANCE_NEXT_Y);
	grpCentering->setPosition(category.panel->getWidth() - DISTANCE_BORDER - grpCentering->getWidth(), DISTANCE_BORDER);
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
	category.panel->add(chkFilterLowRes, chkBlackerThanBlack->getX() + chkBlackerThanBlack->getWidth() + DISTANCE_NEXT_X, posY);
	posY += chkFilterLowRes->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkAspect, DISTANCE_BORDER, posY);
	posY += chkAspect->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkFlickerFixer, DISTANCE_BORDER, posY);
	posY += chkFlickerFixer->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(chkFrameskip, DISTANCE_BORDER, posY);
	category.panel->add(sldRefresh, chkFrameskip->getX() + chkFrameskip->getWidth() + DISTANCE_NEXT_X + 1, posY);
	category.panel->add(lblFrameRate, sldRefresh->getX() + sldRefresh->getWidth() + 8, posY + 2);
	posY += chkFrameskip->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblBrightness, DISTANCE_BORDER, posY);
	category.panel->add(sldBrightness, lblBrightness->getX() + lblBrightness->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(lblBrightnessValue, sldBrightness->getX() + sldBrightness->getWidth() + 8, posY);

	RefreshPanelDisplay();
}

void ExitPanelDisplay()
{
	delete chkFrameskip;
	delete sldRefresh;
	delete lblFrameRate;
	delete lblBrightness;
	delete sldBrightness;
	delete lblBrightnessValue;
	delete amigaScreenActionListener;
	delete lblAmigaWidth;
	delete sldAmigaWidth;
	delete txtAmigaWidth;
	delete lblAmigaHeight;
	delete sldAmigaHeight;
	delete txtAmigaHeight;
	delete chkAutoCrop;
	delete chkBorderless;
	delete lblHOffset;
	delete sldHOffset;
	delete lblHOffsetValue;
	delete lblVOffset;
	delete sldVOffset;
	delete lblVOffsetValue;
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
	delete chkVsync;

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
	lblFrameRate->setCaption(std::to_string(changed_prefs.gfx_framerate));
	lblFrameRate->adjustSize();

	sldBrightness->setValue(changed_prefs.gfx_luminance);
	lblBrightnessValue->setCaption(std::to_string(changed_prefs.gfx_luminance));
	lblBrightnessValue->adjustSize();
	
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
	chkAutoCrop->setSelected(changed_prefs.gfx_auto_crop);
#if !defined USE_DISPMANX
	if (changed_prefs.gfx_auto_crop)
	{
		changed_prefs.gfx_monitor[0].gfx_size_win.width = 720;
		changed_prefs.gfx_monitor[0].gfx_size_win.height = 568;
		changed_prefs.gfx_xcenter = 0;
		changed_prefs.gfx_ycenter = 0;
	}
#endif
	chkBorderless->setSelected(changed_prefs.borderless);
	chkVsync->setSelected(changed_prefs.gfx_apmode[0].gfx_vsync);

	sldHOffset->setValue(changed_prefs.gfx_horizontal_offset);
	lblHOffsetValue->setCaption(std::to_string(changed_prefs.gfx_horizontal_offset));
	lblHOffsetValue->adjustSize();

	sldVOffset->setValue(changed_prefs.gfx_vertical_offset);
	lblVOffsetValue->setCaption(std::to_string(changed_prefs.gfx_vertical_offset));
	lblVOffsetValue->adjustSize();

#if !defined USE_DISPMANX
	chkHorizontal->setEnabled(!changed_prefs.gfx_auto_crop);
	chkVertical->setEnabled(!changed_prefs.gfx_auto_crop);
#endif
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
#if !defined USE_DISPMANX
		lblAmigaWidth->setEnabled(!chkAutoCrop->isSelected());
		sldAmigaWidth->setEnabled(!chkAutoCrop->isSelected());
		txtAmigaWidth->setEnabled(!chkAutoCrop->isSelected());

		lblAmigaHeight->setEnabled(!chkAutoCrop->isSelected());
		sldAmigaHeight->setEnabled(!chkAutoCrop->isSelected());
		txtAmigaHeight->setEnabled(!chkAutoCrop->isSelected());
#endif
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
#if !defined USE_DISPMANX
		lblAmigaWidth->setEnabled(!chkAutoCrop->isSelected());
		sldAmigaWidth->setEnabled(!chkAutoCrop->isSelected());
		txtAmigaWidth->setEnabled(!chkAutoCrop->isSelected());

		lblAmigaHeight->setEnabled(!chkAutoCrop->isSelected());
		sldAmigaHeight->setEnabled(!chkAutoCrop->isSelected());
		txtAmigaHeight->setEnabled(!chkAutoCrop->isSelected());
#endif
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
	helptext.emplace_back("Select the required width, height and other options of displayed the Amiga screen.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Here you can select the screen mode to be used as well:");
	helptext.emplace_back(" ");
	helptext.emplace_back("- Windowed: the Amiga screen will be shown in a Window on your desktop");
	helptext.emplace_back("- Fullscreen: the monitor resolution will change to the selected one.");
	helptext.emplace_back("- Full-window: the Amiga screen will be scaled to the current resolution.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"Auto-Crop\" option will try to detect the width/height automatically,");
	helptext.emplace_back("and scale it up to the full area of the screen, eliminating any black borders.");
	helptext.emplace_back("When this option is disabled, the \"Centering\" options become active, and allow");
	helptext.emplace_back("you to enable automatic centering, for Horizontal and Vertical axes individually.");
	helptext.emplace_back("This can be useful but keep in mind that especially the vertical centering might");
	helptext.emplace_back("cause issues in some games/demos. If that happens, try disabling it and adjust");
	helptext.emplace_back("the Width/Height (in combination with H/V offset if you want), instead.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Borderless\" is only useful in a graphical environment (ie; not when running in");
	helptext.emplace_back("console). This will make the Amiberry window appear borderless, with no window");
	helptext.emplace_back("controls/decorations around it.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"VSync\" - enabling VSync will use the refresh rate of your monitor to do screen");
	helptext.emplace_back("updates. This might reduce tearing, but requires that your monitor supports the same");
	helptext.emplace_back("refresh rate that's being emulated (e.g. 50Hz for PAL).");
	helptext.emplace_back(" ");
	helptext.emplace_back("Also, you can use the \"Offset\" options, to manually shift the image horizontally");
	helptext.emplace_back("or vertically, to fine tune the image position.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can select the \"Scaling method\" used for the displayed Amiga screen.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Auto: will try to find the best looking scaling method depending on your");
	helptext.emplace_back("monitor resolution. This is the default option.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Pixelated: will give you a more pixelated and crisp image, but it may cause");
	helptext.emplace_back("some distortion if your resolution is not an exact multiple of what is displayed.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Linear: will give you a smoother scaling but some people might find it a bit");
	helptext.emplace_back("blurry depending on the Amiga software being used.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Blacker than black\" - when enabled, boosts the gamma for the black areas, which");
	helptext.emplace_back("makes the border around them look darker.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Filtered Low Res\" - This option enables filtering when LowRes is selected in the");
	helptext.emplace_back("Resolution option above. It will make the output look smoother instead of pixelated");
	helptext.emplace_back("when using the LowRes option.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"Correct Aspect Ratio\" - This allows you to choose if you want the correct");
	helptext.emplace_back("aspect ratio to be the default display method, or have the image stretched to fill");
	helptext.emplace_back("the screen instead.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"Remove interlace artifacts\" option enables a special feature internally, to");
	helptext.emplace_back("eliminate the artifacts produced by Interlace modes. It might cause issues in some");
	helptext.emplace_back("applications, so keep that in mind.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Refresh\" - If this option is enabled, it allows you to modify the refresh rate of");
	helptext.emplace_back("the emulation, from the full frame rate that it's normally running at. When this");
	helptext.emplace_back("option is disabled, the value will be set to 1, which represents that every 1 frame");
	helptext.emplace_back("will be drawn/shown. If you enable the option, the value will be changed to 2, which");
	helptext.emplace_back("means that 1 out of every 2 frames will be drawn (essentially what is known as being");
	helptext.emplace_back("'frame-skip'). Increasing this value further, will increase the frame-skipping to");
	helptext.emplace_back("draw one frame out of X (where X = the value you set it to). This can be useful in");
	helptext.emplace_back("situations where you are doing high computational work (like 3D-rendering) and you");
	helptext.emplace_back("don't care about the redraw on the screen. This option can also be useful to get");
	helptext.emplace_back("to get some more games playable.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Brightness\" - Allows adjustment of the output image brightness, from -200 to 200.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Line Mode\" - These options define how the Amiga screen is drawn.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Single: This will draw single lines only, which will be stretched vertically to");
	helptext.emplace_back("fill the screen when necessary (ie; showing a 640x256 resolution in a 640x512 window)");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Double: This option will draw every line twice, which will take up a little more");
	helptext.emplace_back("resources, but looks better since they don't have to be stretched vertically. It also");
	helptext.emplace_back("enables the options for the Interlace line mode, below.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Scanlines: This option will also draw double the lines, but try to replicate the");
	helptext.emplace_back("scanlines effect shown in CRT monitors. Please note that this is not a shader, so the");
	helptext.emplace_back("the screen output might not be perfect in all resolutions.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Interlaced line mode\" - These options further define how the Amiga screen is drawn");
	helptext.emplace_back("when using interlaced screenmodes.");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Single: Draw single lines, interlace modes will flicker like the real hardware");
	helptext.emplace_back(" ");
	helptext.emplace_back(" - Double, frames/fields/fields+: These options are only useful in Interlace modes.");
	helptext.emplace_back(" ");
	return true;
}
