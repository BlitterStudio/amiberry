#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "gui.h"
#include "gui_handling.h"


const int amigawidth_values[] = { 320, 352, 384, 640, 704, 768 };
const int amigaheight_values[] = { 200, 216, 240, 256, 262, 270 };
#ifdef RASPBERRY
const int FullscreenRatio[] = { 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
                     90, 91, 92, 93, 94, 95, 96, 97 ,98, 99 ,100 };
#endif

static gcn::Window *grpAmigaScreen;
static gcn::Label* lblAmigaWidth;
static gcn::Label* lblAmigaWidthInfo;
static gcn::Slider* sldAmigaWidth;
static gcn::Label* lblAmigaHeight;
static gcn::Label* lblAmigaHeightInfo;
static gcn::Slider* sldAmigaHeight;
static gcn::Label* lblVertPos;
static gcn::Label* lblVertPosInfo;
static gcn::Slider* sldVertPos;
static gcn::UaeCheckBox* chkFrameskip;
#ifdef RASPBERRY
static gcn::Label*  lblFSRatio;
static gcn::Label*  lblFSRatioInfo;
static gcn::Slider* sldFSRatio;

static gcn::UaeCheckBox* chkAspect;
#endif


class AmigaScreenActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == sldAmigaWidth) 
      {
        if(changed_prefs.gfx_size.width != amigawidth_values[(int)(sldAmigaWidth->getValue())])
        {
      		changed_prefs.gfx_size.width = amigawidth_values[(int)(sldAmigaWidth->getValue())];
      		RefreshPanelDisplay();
    	  }
      }
      else if (actionEvent.getSource() == sldAmigaHeight) 
      {
        if(changed_prefs.gfx_size.height != amigaheight_values[(int)(sldAmigaHeight->getValue())])
        {
      		changed_prefs.gfx_size.height = amigaheight_values[(int)(sldAmigaHeight->getValue())];
      		RefreshPanelDisplay();
    	  }
      }
      else if (actionEvent.getSource() == sldVertPos) 
      {
        if(changed_prefs.pandora_vertical_offset != (int)(sldVertPos->getValue()))
        {
      		changed_prefs.pandora_vertical_offset = (int)(sldVertPos->getValue());
      		RefreshPanelDisplay();
    	  }
      }
      else if (actionEvent.getSource() == chkFrameskip) 
      {
        changed_prefs.gfx_framerate = chkFrameskip->isSelected() ? 1 : 0;
      }
#ifdef RASPBERRY
      else if (actionEvent.getSource() == sldFSRatio) 
      {
        if(changed_prefs.gfx_fullscreen_ratio != FullscreenRatio[(int)(sldFSRatio->getValue())])
        {
      		changed_prefs.gfx_fullscreen_ratio = FullscreenRatio[(int)(sldFSRatio->getValue())];
      		RefreshPanelDisplay();
    	  }
      }
      else if (actionEvent.getSource() == chkAspect)
        changed_prefs.gfx_correct_aspect = chkAspect->isSelected();
#endif
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


#ifdef RASPBERRY
  lblFSRatio = new gcn::Label("Fullscreen Ratio:");
  lblFSRatio->setSize(150, LABEL_HEIGHT);
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

  chkFrameskip = new gcn::UaeCheckBox("Frameskip");
  chkFrameskip->addActionListener(amigaScreenActionListener);

  grpAmigaScreen = new gcn::Window("Amiga Screen");
  grpAmigaScreen->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
  
  int posY = 10;
  grpAmigaScreen->add(lblAmigaWidth, 0, posY);
  grpAmigaScreen->add(sldAmigaWidth, 160, posY);
  grpAmigaScreen->add(lblAmigaWidthInfo, 160 + sldAmigaWidth->getWidth() + 12, posY);
  posY += sldAmigaWidth->getHeight() + DISTANCE_NEXT_Y;
  grpAmigaScreen->add(lblAmigaHeight, 0, posY);
  grpAmigaScreen->add(sldAmigaHeight, 160, posY);
  grpAmigaScreen->add(lblAmigaHeightInfo, 160 + sldAmigaHeight->getWidth() + 12, posY);
  posY += sldAmigaHeight->getHeight() + DISTANCE_NEXT_Y;
  grpAmigaScreen->add(lblVertPos, 0, posY);
  grpAmigaScreen->add(sldVertPos, 160, posY);
  grpAmigaScreen->add(lblVertPosInfo, 160 + sldVertPos->getWidth() + 12, posY);
  posY += sldVertPos->getHeight() + DISTANCE_NEXT_Y;

#ifdef RASPBERRY
  grpAmigaScreen->add(lblFSRatio, 0, posY);
  grpAmigaScreen->add(sldFSRatio, 160, posY);
  grpAmigaScreen->add(lblFSRatioInfo, 160 + sldFSRatio->getWidth() + 12, posY);
  posY += sldFSRatio->getHeight() + DISTANCE_NEXT_Y;
#endif

  
  grpAmigaScreen->setMovable(false);
  grpAmigaScreen->setSize(460, posY + DISTANCE_BORDER);
  grpAmigaScreen->setBaseColor(gui_baseCol);

  category.panel->add(grpAmigaScreen);

#ifdef RASPBERRY
  category.panel->add(chkAspect   , DISTANCE_BORDER, DISTANCE_BORDER + grpAmigaScreen->getHeight() + DISTANCE_NEXT_Y);
  category.panel->add(chkFrameskip, DISTANCE_BORDER, DISTANCE_BORDER + grpAmigaScreen->getHeight() + chkAspect->getHeight() + 2*DISTANCE_NEXT_Y);
#else
  category.panel->add(chkFrameskip, DISTANCE_BORDER, DISTANCE_BORDER + grpAmigaScreen->getHeight() + DISTANCE_NEXT_Y);
#endif

  RefreshPanelDisplay();
}


void ExitPanelDisplay(void)
{
  delete lblAmigaWidth;
  delete sldAmigaWidth;
  delete lblAmigaWidthInfo;
  delete lblAmigaHeight;
  delete sldAmigaHeight;
  delete lblAmigaHeightInfo;
  delete lblVertPos;
  delete sldVertPos;
  delete lblVertPosInfo;
  delete grpAmigaScreen;
  delete chkFrameskip;
  delete amigaScreenActionListener;
#ifdef RASPBERRY
  delete lblFSRatio;
  delete sldFSRatio;
  delete lblFSRatioInfo;

  delete chkAspect;
#endif
}


void RefreshPanelDisplay(void)
{
  int i;
  char tmp[32];
  
  for(i=0; i<6; ++i)
  {
    if(changed_prefs.gfx_size.width == amigawidth_values[i])
    {
      sldAmigaWidth->setValue(i);
      snprintf(tmp, 32, "%d", changed_prefs.gfx_size.width);
      lblAmigaWidthInfo->setCaption(tmp);
      break;
    }
  }

  for(i=0; i<6; ++i)
  {
    if(changed_prefs.gfx_size.height == amigaheight_values[i])
    {
      sldAmigaHeight->setValue(i);
      snprintf(tmp, 32, "%d", changed_prefs.gfx_size.height);
      lblAmigaHeightInfo->setCaption(tmp);
      break;
    }
  }
  
#ifdef RASPBERRY
  for(i=0; i<21; ++i)
  {
    if(changed_prefs.gfx_fullscreen_ratio == FullscreenRatio[i])
    {
      sldFSRatio->setValue(i);
      snprintf(tmp, 32, "%d%%", changed_prefs.gfx_fullscreen_ratio);
      lblFSRatioInfo->setCaption(tmp);
      break;
    }
  }

  chkAspect->setSelected(changed_prefs.gfx_correct_aspect);
#endif

  sldVertPos->setValue(changed_prefs.pandora_vertical_offset);
  snprintf(tmp, 32, "%d", changed_prefs.pandora_vertical_offset);
  lblVertPosInfo->setCaption(tmp);
  
  chkFrameskip->setSelected(changed_prefs.gfx_framerate);
}
