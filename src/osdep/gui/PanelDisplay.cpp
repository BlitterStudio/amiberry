#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
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
#include "gui.h"
#include "gui_handling.h"

static gcn::UaeCheckBox* chkFrameskip;
static gcn::Window* grpScalingMethod;
static gcn::UaeRadioButton* optAuto;
static gcn::UaeRadioButton* optNearest;
static gcn::UaeRadioButton* optLinear;
#ifdef PANDORA
static gcn::Window *grpAmigaScreen;
static gcn::Label* lblVertPos;
static gcn::Label* lblVertPosInfo;
static gcn::Slider* sldVertPos;
#endif // PANDORA

class AmigaScreenActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkFrameskip)
		{
			changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 1 : 0;
		}
#ifdef PANDORA
		else if (actionEvent.getSource() == sldVertPos) 
      {
        if(changed_prefs.pandora_vertical_offset != (int)(sldVertPos->getValue()) + OFFSET_Y_ADJUST)
        {
      		changed_prefs.pandora_vertical_offset = (int)(sldVertPos->getValue()) + OFFSET_Y_ADJUST;
      		RefreshPanelDisplay();
    	  }
      }
#endif // PANDORA
	}
};

AmigaScreenActionListener* amigaScreenActionListener;

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

void InitPanelDisplay(const struct _ConfigCategory& category)
{
	amigaScreenActionListener = new AmigaScreenActionListener();

	int posY = DISTANCE_BORDER;

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
	grpScalingMethod->setSize(240, optLinear->getY() + optLinear->getHeight() + 30);
	grpScalingMethod->setBaseColor(gui_baseCol);

	category.panel->add(grpScalingMethod);

	posY += DISTANCE_BORDER + grpScalingMethod->getHeight() + DISTANCE_NEXT_Y;

#ifdef PANDORA
	lblVertPos = new gcn::Label("Vert. offset:");
	lblVertPos->setSize(150, LABEL_HEIGHT);
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

	chkFrameskip = new gcn::UaeCheckBox("Frameskip");
	chkFrameskip->addActionListener(amigaScreenActionListener);

#ifdef PANDORA
	grpAmigaScreen = new gcn::Window("Amiga Screen");
	grpAmigaScreen->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpAmigaScreen->add(lblVertPos, 0, posY);
	grpAmigaScreen->add(sldVertPos, 160, posY);
	grpAmigaScreen->add(lblVertPosInfo, 160 + sldVertPos->getWidth() + 12, posY);
	posY += sldVertPos->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->setMovable(false);
	grpAmigaScreen->setSize(460, posY + DISTANCE_BORDER);
	grpAmigaScreen->setBaseColor(gui_baseCol);

	category.panel->add(grpAmigaScreen);
#endif //PANDORA

	category.panel->add(chkFrameskip, DISTANCE_BORDER, posY);

	RefreshPanelDisplay();
}


void ExitPanelDisplay()
{
	delete chkFrameskip;
	delete amigaScreenActionListener;
#ifdef PANDORA
	delete lblVertPos;
	delete sldVertPos;
	delete lblVertPosInfo;
	delete grpAmigaScreen;
#endif //PANDORA
	delete optAuto;
	delete optNearest;
	delete optLinear;
	delete grpScalingMethod;
	delete scalingMethodActionListener;
}


void RefreshPanelDisplay()
{
	chkFrameskip->setSelected(changed_prefs.gfx_framerate);

	if (changed_prefs.scaling_method == -1)
		optAuto->setSelected(true);
	else if (changed_prefs.scaling_method == 0)
		optNearest->setSelected(true);
	else if (changed_prefs.scaling_method == 1)
		optLinear->setSelected(true);
#ifdef PANDORA
	sldVertPos->setValue(changed_prefs.pandora_vertical_offset - OFFSET_Y_ADJUST);
	snprintf(tmp, 32, "%d", changed_prefs.pandora_vertical_offset - OFFSET_Y_ADJUST);
	lblVertPosInfo->setCaption(tmp);
#endif //PANDORA
}

bool HelpPanelDisplay(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.push_back("Select the scaling method for the Amiga screen. The default option \"Auto\", will try to find the best looking");
	helptext.push_back("scaling method depending on your monitor's resolution. \"Nearest Neighbor\" will give you a more pixelated");
	helptext.push_back("and crisp image, but it may come with some distortion if your resolution is not an exact multiple.");
	helptext.push_back("\"Linear\" will give you a smoother scaling but some people might find it a bit blurry.");
	helptext.push_back("");
#ifdef PANDORA
	helptext.push_back("With \"Vert. offset\" you can adjust the position of the first drawn line of the Amiga screen. You can also change");
	helptext.push_back("this during emulation with left and right shoulder button and dpad up/down.");
	helptext.push_back("");
#endif //PANDORA
	helptext.push_back("When you activate \"Frameskip\", only every second frame is drawn. This will improve performance and some");
	helptext.push_back("more games are playable.");
	return true;
}
