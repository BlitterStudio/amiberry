#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "gui.h"
#include "gui_handling.h"


const int amigawidth_values[] = {320, 352, 384, 640, 704, 768};
const int amigaheight_values[] = {200, 216, 240, 256, 262, 270};

static gcn::Window* grpAmigaScreen;
static gcn::Label* lblAmigaWidth;
static gcn::Label* lblAmigaWidthInfo;
static gcn::Slider* sldAmigaWidth;
static gcn::Label* lblAmigaHeight;
static gcn::Label* lblAmigaHeightInfo;
static gcn::Slider* sldAmigaHeight;
static gcn::UaeCheckBox* chkFrameskip;
static gcn::Window* grpScalingMethod;
static gcn::UaeRadioButton* optAuto;
static gcn::UaeRadioButton* optNearest;
static gcn::UaeRadioButton* optLinear;

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
		else if (actionEvent.getSource() == chkFrameskip)
		{
			changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 1 : 0;
		}
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

	lblAmigaWidth = new gcn::Label("Width:");
	lblAmigaWidth->setSize(150, LABEL_HEIGHT);
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
	lblAmigaHeight->setSize(150, LABEL_HEIGHT);
	lblAmigaHeight->setAlignment(gcn::Graphics::RIGHT);
	sldAmigaHeight = new gcn::Slider(0, 5);
	sldAmigaHeight->setSize(160, SLIDER_HEIGHT);
	sldAmigaHeight->setBaseColor(gui_baseCol);
	sldAmigaHeight->setMarkerLength(20);
	sldAmigaHeight->setStepLength(1);
	sldAmigaHeight->setId("sldHeight");
	sldAmigaHeight->addActionListener(amigaScreenActionListener);
	lblAmigaHeightInfo = new gcn::Label("200");

	grpAmigaScreen = new gcn::Window("Amiga Screen");
	grpAmigaScreen->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	int posY = DISTANCE_BORDER;
	grpAmigaScreen->add(lblAmigaWidth, 0, posY);
	grpAmigaScreen->add(sldAmigaWidth, 20, posY);
	grpAmigaScreen->add(lblAmigaWidthInfo, 20 + sldAmigaWidth->getWidth() + 12, posY);
	posY += sldAmigaWidth->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblAmigaHeight, 0, posY);
	grpAmigaScreen->add(sldAmigaHeight, 20, posY);
	grpAmigaScreen->add(lblAmigaHeightInfo, 20 + sldAmigaHeight->getWidth() + 12, posY);
	posY += DISTANCE_BORDER + sldAmigaHeight->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->setMovable(false);
	grpAmigaScreen->setSize(240, posY);
	grpAmigaScreen->setBaseColor(gui_baseCol);

	category.panel->add(grpAmigaScreen);
	posY += DISTANCE_BORDER + DISTANCE_NEXT_Y;

	chkFrameskip = new gcn::UaeCheckBox("Frameskip");
	chkFrameskip->addActionListener(amigaScreenActionListener);

	category.panel->add(chkFrameskip, DISTANCE_BORDER, posY);
	posY += DISTANCE_BORDER + chkFrameskip->getHeight() + DISTANCE_NEXT_Y;

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

	RefreshPanelDisplay();
}


void ExitPanelDisplay()
{
	delete lblAmigaWidth;
	delete sldAmigaWidth;
	delete lblAmigaWidthInfo;
	delete lblAmigaHeight;
	delete sldAmigaHeight;
	delete lblAmigaHeightInfo;
	delete grpAmigaScreen;
	delete chkFrameskip;
	delete amigaScreenActionListener;

	delete optAuto;
	delete optNearest;
	delete optLinear;
	delete grpScalingMethod;
	delete scalingMethodActionListener;
}


void RefreshPanelDisplay()
{
	int i;
	char tmp[32];

	for (i = 0; i < 6; ++i)
	{
		if (changed_prefs.gfx_size.width == amigawidth_values[i])
		{
			sldAmigaWidth->setValue(i);
			snprintf(tmp, 32, "%d", changed_prefs.gfx_size.width);
			lblAmigaWidthInfo->setCaption(tmp);
			break;
		}
	}

	for (i = 0; i < 6; ++i)
	{
		if (changed_prefs.gfx_size.height == amigaheight_values[i])
		{
			sldAmigaHeight->setValue(i);
			snprintf(tmp, 32, "%d", changed_prefs.gfx_size.height);
			lblAmigaHeightInfo->setCaption(tmp);
			break;
		}
	}

	chkFrameskip->setSelected(changed_prefs.gfx_framerate);

	if (changed_prefs.scaling_method == -1)
		optAuto->setSelected(true);
	else if (changed_prefs.scaling_method == 0)
		optNearest->setSelected(true);
	else if (changed_prefs.scaling_method == 1)
		optLinear->setSelected(true);
}
