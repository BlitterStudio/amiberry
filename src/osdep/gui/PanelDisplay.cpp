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
#include "config.h"
#include "options.h"
#include "include/memory.h"
#include "uae.h"
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

class AmigaScreenActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent)
	{
		if (actionEvent.getSource() == sldAmigaWidth)
		{
			if (changed_prefs.gfx_size.width != amigawidth_values[(int)(sldAmigaWidth->getValue())])
			{
				changed_prefs.gfx_size.width = amigawidth_values[(int)(sldAmigaWidth->getValue())];
				RefreshPanelDisplay();
			}
		}
		else if (actionEvent.getSource() == sldAmigaHeight)
		{
			if (changed_prefs.gfx_size.height != amigaheight_values[(int)(sldAmigaHeight->getValue())])
			{
				changed_prefs.gfx_size.height = amigaheight_values[(int)(sldAmigaHeight->getValue())];
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

	chkFrameskip = new gcn::UaeCheckBox("Frameskip");
	chkFrameskip->addActionListener(amigaScreenActionListener);

	grpAmigaScreen = new gcn::Window("Amiga Screen");
	grpAmigaScreen->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);

	int posY = 10;
	grpAmigaScreen->add(lblAmigaWidth, 0, posY);
	grpAmigaScreen->add(sldAmigaWidth, 20, posY);
	grpAmigaScreen->add(lblAmigaWidthInfo, 20 + sldAmigaWidth->getWidth() + 12, posY);
	posY += sldAmigaWidth->getHeight() + DISTANCE_NEXT_Y;
	grpAmigaScreen->add(lblAmigaHeight, 0, posY);
	grpAmigaScreen->add(sldAmigaHeight, 20, posY);
	grpAmigaScreen->add(lblAmigaHeightInfo, 20 + sldAmigaHeight->getWidth() + 12, posY);
	posY += sldAmigaHeight->getHeight() + DISTANCE_NEXT_Y;

	grpAmigaScreen->setMovable(false);
	grpAmigaScreen->setSize(260, posY + DISTANCE_BORDER);
	grpAmigaScreen->setBaseColor(gui_baseCol);

	category.panel->add(grpAmigaScreen);
	category.panel->add(chkFrameskip, DISTANCE_BORDER, DISTANCE_BORDER + grpAmigaScreen->getHeight() + DISTANCE_NEXT_Y);

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
}
