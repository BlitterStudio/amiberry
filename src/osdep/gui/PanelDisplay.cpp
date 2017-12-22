#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#endif
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "include/memory.h"
#include "uae.h"
#include "custom.h"
#include "gui.h"
#include "gui_handling.h"

const int amigawidth_values[] = { 320, 352, 384, 640, 704, 768 };
const int amigaheight_values[] = { 200, 216, 240, 256, 262, 270 };

#ifdef USE_SDL1
const int FullscreenRatio[] = { 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
                                90, 91, 92, 93, 94, 95, 96, 97,98, 99,100 };
#endif

#ifdef USE_SDL2
static gcn::Window* grpScalingMethod;
static gcn::UaeRadioButton* optAuto;
static gcn::UaeRadioButton* optNearest;
static gcn::UaeRadioButton* optLinear;
#endif

static gcn::Window *grpAmigaScreen;
static gcn::Label* lblAmigaWidth;
static gcn::Label* lblAmigaWidthInfo;
static gcn::Slider* sldAmigaWidth;
static gcn::Label* lblAmigaHeight;
static gcn::Label* lblAmigaHeightInfo;
static gcn::Slider* sldAmigaHeight;

#ifdef PANDORA
static gcn::Label* lblVertPos;
static gcn::Label* lblVertPosInfo;
static gcn::Slider* sldVertPos;
#endif

static gcn::UaeCheckBox* chkLineDbl;
static gcn::UaeCheckBox* chkFrameskip;

#ifdef USE_SDL1
static gcn::Label*  lblFSRatio;
static gcn::Label*  lblFSRatioInfo;
static gcn::Slider* sldFSRatio;
static gcn::UaeCheckBox* chkAspect;
#endif


class AmigaScreenActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == sldAmigaWidth)
		{
			if (changed_prefs.gfx_size.width != amigawidth_values[int(sldAmigaWidth->getValue())])
			{
				changed_prefs.gfx_size.width = amigawidth_values[int(sldAmigaWidth->getValue())];
				RefreshPanelDisplay();
			}
		}
		else if (actionEvent.getSource() == sldAmigaHeight)
		{
			if (changed_prefs.gfx_size.height != amigaheight_values[int(sldAmigaHeight->getValue())])
			{
				changed_prefs.gfx_size.height = amigaheight_values[int(sldAmigaHeight->getValue())];
				RefreshPanelDisplay();
			}
		}
#ifdef PANDORA
		else if (actionEvent.getSource() == sldVertPos)
		{
			if (changed_prefs.pandora_vertical_offset != (int)(sldVertPos->getValue()) + OFFSET_Y_ADJUST)
			{
				changed_prefs.pandora_vertical_offset = (int)(sldVertPos->getValue()) + OFFSET_Y_ADJUST;
				RefreshPanelDisplay();
			}
		}
#endif
		else
			if (actionEvent.getSource() == chkFrameskip)
			{
				changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 1 : 0;
			}
			else if (actionEvent.getSource() == chkLineDbl)
			{
				changed_prefs.gfx_vresolution = chkLineDbl->isSelected() ? VRES_DOUBLE : VRES_NONDOUBLE;
			}
#ifdef USE_SDL1
			else if (actionEvent.getSource() == sldFSRatio)
			{
				if (changed_prefs.gfx_fullscreen_ratio != FullscreenRatio[int(sldFSRatio->getValue())])
				{
					changed_prefs.gfx_fullscreen_ratio = FullscreenRatio[int(sldFSRatio->getValue())];
					RefreshPanelDisplay();
				}
			}
			else if (actionEvent.getSource() == chkAspect)
				changed_prefs.gfx_correct_aspect = chkAspect->isSelected();
#endif
	}
};

AmigaScreenActionListener* amigaScreenActionListener;

#ifdef USE_SDL2
class ScalingMethodActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optAuto)
			changed_prefs.scaling_method = -1;
		else if (actionEvent.getSource() == optNearest)
			changed_prefs.scaling_method = 0;
		else if (actionEvent.getSource() == optLinear)
			changed_prefs.scaling_method = 1;
	}
};

static ScalingMethodActionListener* scalingMethodActionListener;
#endif

void InitPanelDisplay(const struct _ConfigCategory& category)
{
	amigaScreenActionListener = new AmigaScreenActionListener();
	auto posY = DISTANCE_BORDER;

	lblAmigaWidth = new gcn::Label("Width:");
	lblAmigaWidth->setAlignment(gcn::Graphics::RIGHT);
	sldAmigaWidth = new gcn::Slider(0, 5);
	sldAmigaWidth->setSize(160, SLIDER_HEIGHT);
	sldAmigaWidth->setBaseColor(gui_baseCol);
	sldAmigaWidth->setMarkerLength(20);
	sldAmigaWidth->setStepLength(1);
	sldAmigaWidth->setId("sldWidth");
	sldAmigaWidth->addActionListener(amigaScreenActionListener);
	lblAmigaWidthInfo = new gcn::Label("320");

	lblAmigaHeight = new gcn::Label("Height:");
	lblAmigaHeight->setAlignment(gcn::Graphics::RIGHT);
	sldAmigaHeight = new gcn::Slider(0, 5);
	sldAmigaHeight->setSize(160, SLIDER_HEIGHT);
	sldAmigaHeight->setBaseColor(gui_baseCol);
	sldAmigaHeight->setMarkerLength(20);
	sldAmigaHeight->setStepLength(1);
	sldAmigaHeight->setId("sldHeight");
	sldAmigaHeight->addActionListener(amigaScreenActionListener);
	lblAmigaHeightInfo = new gcn::Label("200");

#ifdef PANDORA
	lblVertPos = new gcn::Label("Vert. offset:");
	lblVertPos->setAlignment(gcn::Graphics::RIGHT);
	sldVertPos = new gcn::Slider(-16, 16);
	sldVertPos->setSize(160, SLIDER_HEIGHT);
	sldVertPos->setBaseColor(gui_baseCol);
	sldVertPos->setMarkerLength(20);
	sldVertPos->setStepLength(1);
	sldVertPos->setId("sldVertPos");
	sldVertPos->addActionListener(amigaScreenActionListener);
	lblVertPosInfo = new gcn::Label("000");
#endif //PANDORA

#ifdef USE_SDL1
	lblFSRatio = new gcn::Label("Ratio:");
	lblFSRatio->setAlignment(gcn::Graphics::RIGHT);
	sldFSRatio = new gcn::Slider(0, 20);
	sldFSRatio->setSize(160, SLIDER_HEIGHT);
	sldFSRatio->setBaseColor(gui_baseCol);
	sldFSRatio->setMarkerLength(20);
	sldFSRatio->setStepLength(1);
	sldFSRatio->setId("FSRatio");
	sldFSRatio->addActionListener(amigaScreenActionListener);
	lblFSRatioInfo = new gcn::Label("100%%");

	chkAspect = new gcn::UaeCheckBox("4/3 ratio shrink");
	chkAspect->setId("4by3Ratio");
	chkAspect->addActionListener(amigaScreenActionListener);
#endif

	chkLineDbl = new gcn::UaeCheckBox("Line doubling");
  	chkLineDbl->addActionListener(amigaScreenActionListener);
	chkFrameskip = new gcn::UaeCheckBox("Frameskip");
	chkFrameskip->addActionListener(amigaScreenActionListener);

	grpAmigaScreen = new gcn::Window("Amiga Screen");
	grpAmigaScreen->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpAmigaScreen->add(lblAmigaWidth, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldAmigaWidth, lblAmigaWidth->getX() + lblAmigaWidth->getWidth() + DISTANCE_NEXT_Y, posY);
	grpAmigaScreen->add(lblAmigaWidthInfo, sldAmigaWidth->getX() + sldAmigaWidth->getWidth() + DISTANCE_NEXT_Y, posY);
	posY += sldAmigaWidth->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblAmigaHeight, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldAmigaHeight, lblAmigaHeight->getX() + lblAmigaHeight->getWidth() + DISTANCE_NEXT_Y, posY);
	grpAmigaScreen->add(lblAmigaHeightInfo, sldAmigaHeight->getX() + sldAmigaHeight->getWidth() + DISTANCE_NEXT_Y, posY);
	posY += sldAmigaHeight->getHeight() + DISTANCE_NEXT_Y;

#ifdef PANDORA
	grpAmigaScreen->add(lblVertPos, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldVertPos, lblVertPos->getX() + lblVertPos->getWidth() + DISTANCE_NEXT_Y, posY);
	grpAmigaScreen->add(lblVertPosInfo, sldVertPos->getX() + sldVertPos->getWidth() + 12, posY);
	posY += sldVertPos->getHeight() + DISTANCE_NEXT_Y;
#endif

#ifdef USE_SDL1
	grpAmigaScreen->add(lblFSRatio, DISTANCE_BORDER, posY);
	grpAmigaScreen->add(sldFSRatio, lblFSRatio->getX() + lblFSRatio->getWidth() + DISTANCE_NEXT_Y, posY);
	grpAmigaScreen->add(lblFSRatioInfo, sldFSRatio->getX() + sldFSRatio->getWidth() + DISTANCE_NEXT_Y, posY);
	posY += sldFSRatio->getHeight() + DISTANCE_NEXT_Y;
#endif

	grpAmigaScreen->setMovable(false);
	grpAmigaScreen->setSize(lblAmigaHeightInfo->getX() + lblAmigaHeightInfo->getWidth() + DISTANCE_BORDER, posY + DISTANCE_BORDER);
	grpAmigaScreen->setBaseColor(gui_baseCol);

	category.panel->add(grpAmigaScreen);
	posY = DISTANCE_BORDER + grpAmigaScreen->getHeight() + DISTANCE_NEXT_Y;

#ifdef USE_SDL1
	category.panel->add(chkAspect, DISTANCE_BORDER, posY);
	posY += chkAspect->getHeight() + DISTANCE_NEXT_Y;
#endif

#ifdef USE_SDL2
	scalingMethodActionListener = new ScalingMethodActionListener();

	optAuto = new gcn::UaeRadioButton("Auto", "radioscalingmethodgroup");
	optAuto->addActionListener(scalingMethodActionListener);

	optNearest = new gcn::UaeRadioButton("Nearest Neighbor (pixelated)", "radioscalingmethodgroup");
	optNearest->addActionListener(scalingMethodActionListener);

	optLinear = new gcn::UaeRadioButton("Linear (smooth)", "radioscalingmethodgroup");
	optLinear->addActionListener(scalingMethodActionListener);

	grpScalingMethod = new gcn::Window("Scaling method");
	grpScalingMethod->setPosition(DISTANCE_BORDER, posY);
	grpScalingMethod->add(optAuto, 5, 10);
	grpScalingMethod->add(optNearest, 5, 40);
	grpScalingMethod->add(optLinear, 5, 70);
	grpScalingMethod->setMovable(false);
	grpScalingMethod->setSize(optNearest->getWidth() + DISTANCE_BORDER, optLinear->getY() + optLinear->getHeight() + 30);
	grpScalingMethod->setBaseColor(gui_baseCol);

	category.panel->add(grpScalingMethod);

	posY += DISTANCE_BORDER + grpScalingMethod->getHeight() + DISTANCE_NEXT_Y;
#endif

	category.panel->add(chkLineDbl, DISTANCE_BORDER, posY);
	posY += chkLineDbl->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkFrameskip, DISTANCE_BORDER, posY);

	RefreshPanelDisplay();
}


void ExitPanelDisplay()
{
	delete chkFrameskip;
	delete amigaScreenActionListener;
	delete lblAmigaWidth;
	delete sldAmigaWidth;
	delete lblAmigaWidthInfo;
	delete lblAmigaHeight;
	delete sldAmigaHeight;
	delete lblAmigaHeightInfo;

#ifdef PANDORA
	delete lblVertPos;
	delete sldVertPos;
	delete lblVertPosInfo;
#endif

	delete grpAmigaScreen;

#ifdef USE_SDL1
	delete lblFSRatio;
	delete sldFSRatio;
	delete lblFSRatioInfo;
	delete chkAspect;
#endif

#ifdef USE_SDL2
	delete optAuto;
	delete optNearest;
	delete optLinear;
	delete grpScalingMethod;
	delete scalingMethodActionListener;
#endif
}


void RefreshPanelDisplay()
{
	chkLineDbl->setSelected(changed_prefs.gfx_vresolution != VRES_NONDOUBLE);
	chkFrameskip->setSelected(changed_prefs.gfx_framerate);

	int i;
	char tmp[32];

	for (i = 0; i<6; ++i)
	{
		if (changed_prefs.gfx_size.width == amigawidth_values[i])
		{
			sldAmigaWidth->setValue(i);
			snprintf(tmp, 32, "%d", changed_prefs.gfx_size.width);
			lblAmigaWidthInfo->setCaption(tmp);
			break;
		}
	}

	for (i = 0; i<6; ++i)
	{
		if (changed_prefs.gfx_size.height == amigaheight_values[i])
		{
			sldAmigaHeight->setValue(i);
			snprintf(tmp, 32, "%d", changed_prefs.gfx_size.height);
			lblAmigaHeightInfo->setCaption(tmp);
			break;
		}
	}

#ifdef USE_SDL1
	for (i = 0; i<21; ++i)
	{
		if (changed_prefs.gfx_fullscreen_ratio == FullscreenRatio[i])
		{
			sldFSRatio->setValue(i);
			snprintf(tmp, 32, "%d%%", changed_prefs.gfx_fullscreen_ratio);
			lblFSRatioInfo->setCaption(tmp);
			break;
		}
	}
	chkAspect->setSelected(changed_prefs.gfx_correct_aspect);
#endif

#ifdef USE_SDL2
	if (changed_prefs.scaling_method == -1)
		optAuto->setSelected(true);
	else if (changed_prefs.scaling_method == 0)
		optNearest->setSelected(true);
	else if (changed_prefs.scaling_method == 1)
		optLinear->setSelected(true);
#endif

#ifdef PANDORA
	sldVertPos->setValue(changed_prefs.pandora_vertical_offset - OFFSET_Y_ADJUST);
	snprintf(tmp, 32, "%d", changed_prefs.pandora_vertical_offset - OFFSET_Y_ADJUST);
	lblVertPosInfo->setCaption(tmp);
#endif //PANDORA
}

bool HelpPanelDisplay(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required width and height of the Amiga screen. If you select \"NTSC\"");
	helptext.emplace_back("in Chipset, a value greater than 240 for \"Height\" makes no sense. When the game,");
	helptext.emplace_back("Demo or Workbench uses HiRes mode and you selected a value for \"Width\" lower than 640,");
	helptext.emplace_back("you will only see half of the pixels.");
	helptext.emplace_back("");
	helptext.emplace_back("Select the scaling method for the Amiga screen. The default option \"Auto\", ");
	helptext.emplace_back("will try to find the best looking scaling method depending on your monitor's resolution. ");
	helptext.emplace_back("\"Nearest Neighbor\" will give you a more pixelated and crisp image, but it may come with ");
	helptext.emplace_back("some distortion if your resolution is not an exact multiple. ");
	helptext.emplace_back("\"Linear\" will give you a smoother scaling but some people might find it a bit blurry.");
	helptext.emplace_back("");
#ifdef PANDORA
	helptext.emplace_back("With \"Vert. offset\" you can adjust the position of the first drawn line of the Amiga ");
	helptext.emplace_back("screen. You can also change this during emulation with left and right shoulder button ");
	helptext.emplace_back("and dpad up/down.");
	helptext.emplace_back("");
#endif //PANDORA
	helptext.emplace_back("Activate line doubling to remove flicker in interlace modes.");
	helptext.emplace_back("");
	helptext.emplace_back("When you activate \"Frameskip\", only every second frame is drawn.");
	helptext.emplace_back("This will improve performance and some more games are playable.");
	return true;
}
