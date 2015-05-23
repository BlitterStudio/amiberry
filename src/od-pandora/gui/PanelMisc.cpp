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
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "target.h"
#include "gui_handling.h"


static gcn::UaeCheckBox* chkStatusLine;
static gcn::UaeCheckBox* chkShowGUI;
#ifdef RASPBERRY
static gcn::UaeCheckBox* chkAspect;
#else
static gcn::Label* lblPandoraSpeed;
static gcn::Label* lblPandoraSpeedInfo;
static gcn::Slider* sldPandoraSpeed;
#endif

class MiscActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == chkStatusLine)
        changed_prefs.leds_on_screen = chkStatusLine->isSelected();
      
      else if (actionEvent.getSource() == chkShowGUI)
        changed_prefs.start_gui = chkShowGUI->isSelected();

#ifdef RASPBERRY
      else if (actionEvent.getSource() == chkAspect)
        changed_prefs.gfx_correct_aspect = chkAspect->isSelected();
#else
      else if (actionEvent.getSource() == sldPandoraSpeed)
      {
        int newspeed = (int) sldPandoraSpeed->getValue();
        newspeed = newspeed - (newspeed % 20);
        if(changed_prefs.pandora_cpu_speed != newspeed)
        {
          changed_prefs.pandora_cpu_speed = newspeed;
          RefreshPanelMisc();
        }
      }
#endif
    }
};
MiscActionListener* miscActionListener;


void InitPanelMisc(const struct _ConfigCategory& category)
{
  miscActionListener = new MiscActionListener();

  chkStatusLine = new gcn::UaeCheckBox("Status Line");
  chkStatusLine->addActionListener(miscActionListener);

  chkShowGUI = new gcn::UaeCheckBox("Show GUI on startup");
  chkShowGUI->setId("ShowGUI");
  chkShowGUI->addActionListener(miscActionListener);
  
#ifdef RASPBERRY
  chkAspect = new gcn::UaeCheckBox("4/3 ratio shrink");
  chkAspect->addActionListener(miscActionListener);
#else
  lblPandoraSpeed = new gcn::Label("Pandora Speed:");
  lblPandoraSpeed->setSize(110, LABEL_HEIGHT);
  lblPandoraSpeed->setAlignment(gcn::Graphics::RIGHT);
  sldPandoraSpeed = new gcn::Slider(500, 1260);
  sldPandoraSpeed->setSize(200, SLIDER_HEIGHT);
  sldPandoraSpeed->setBaseColor(gui_baseCol);
  sldPandoraSpeed->setMarkerLength(20);
  sldPandoraSpeed->setStepLength(20);
  sldPandoraSpeed->setId("PandSpeed");
  sldPandoraSpeed->addActionListener(miscActionListener);
  lblPandoraSpeedInfo = new gcn::Label("1000 MHz");
#endif
  int posY = DISTANCE_BORDER;
  category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
  posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);
  posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;
#ifdef RASPBERRY
  category.panel->add(chkAspect, DISTANCE_BORDER, posY);
  posY += chkAspect->getHeight() + DISTANCE_NEXT_Y;
#else
  category.panel->add(lblPandoraSpeed, DISTANCE_BORDER, posY);
  category.panel->add(sldPandoraSpeed, DISTANCE_BORDER + lblPandoraSpeed->getWidth() + 8, posY);
  category.panel->add(lblPandoraSpeedInfo, sldPandoraSpeed->getX() + sldPandoraSpeed->getWidth() + 12, posY);
  posY += sldPandoraSpeed->getHeight() + DISTANCE_NEXT_Y;
#endif
  RefreshPanelMisc();
}


void ExitPanelMisc(void)
{
  delete chkStatusLine;
  delete chkShowGUI;
#ifdef RASPBERRY
  delete chkAspect;
#else
  delete lblPandoraSpeed;
  delete sldPandoraSpeed;
  delete lblPandoraSpeedInfo;
#endif
  delete miscActionListener;
}


void RefreshPanelMisc(void)
{
  char tmp[20];

  chkStatusLine->setSelected(changed_prefs.leds_on_screen);
  chkShowGUI->setSelected(changed_prefs.start_gui);
#ifdef RASPBERRY
  chkAspect->setSelected(changed_prefs.gfx_correct_aspect);
#else
  sldPandoraSpeed->setValue(changed_prefs.pandora_cpu_speed);
  snprintf(tmp, 20, "%d MHz", changed_prefs.pandora_cpu_speed);
  lblPandoraSpeedInfo->setCaption(tmp);
#endif
}
