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
#include "gui_handling.h"


static gcn::UaeCheckBox* chkStatusLine;
static gcn::UaeCheckBox* chkHideIdleLed;
static gcn::UaeCheckBox* chkShowGUI;
#ifdef PANDORA_SPECIFIC
static gcn::Label* lblPandoraSpeed;
static gcn::Label* lblPandoraSpeedInfo;
static gcn::Slider* sldPandoraSpeed;
#endif
static gcn::UaeCheckBox* chkBSDSocket;


class MiscActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == chkStatusLine)
        changed_prefs.leds_on_screen = chkStatusLine->isSelected();
      
      else if (actionEvent.getSource() == chkHideIdleLed)
        changed_prefs.pandora_hide_idle_led = chkHideIdleLed->isSelected();

      else if (actionEvent.getSource() == chkShowGUI)
        changed_prefs.start_gui = chkShowGUI->isSelected();

      else if (actionEvent.getSource() == chkBSDSocket)
        changed_prefs.socket_emu = chkBSDSocket->isSelected();


#ifdef PANDORA_SPECIFIC
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
  chkStatusLine->setId("StatusLine");
  chkStatusLine->addActionListener(miscActionListener);

  chkHideIdleLed = new gcn::UaeCheckBox("Hide idle led");
  chkHideIdleLed->setId("HideIdle");
  chkHideIdleLed->addActionListener(miscActionListener);

  chkShowGUI = new gcn::UaeCheckBox("Show GUI on startup");
  chkShowGUI->setId("ShowGUI");
  chkShowGUI->addActionListener(miscActionListener);
  
#ifdef PANDORA_SPECIFIC
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

  chkBSDSocket = new gcn::UaeCheckBox("bsdsocket.library");
  chkBSDSocket->setId("BSDSocket");
  chkBSDSocket->addActionListener(miscActionListener);

  int posY = DISTANCE_BORDER;
  category.panel->add(chkStatusLine, DISTANCE_BORDER, posY);
  posY += chkStatusLine->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(chkHideIdleLed, DISTANCE_BORDER, posY);
  posY += chkHideIdleLed->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(chkShowGUI, DISTANCE_BORDER, posY);
  posY += chkShowGUI->getHeight() + DISTANCE_NEXT_Y;
#ifdef PANDORA_SPECIFIC
  category.panel->add(lblPandoraSpeed, DISTANCE_BORDER, posY);
  category.panel->add(sldPandoraSpeed, DISTANCE_BORDER + lblPandoraSpeed->getWidth() + 8, posY);
  category.panel->add(lblPandoraSpeedInfo, sldPandoraSpeed->getX() + sldPandoraSpeed->getWidth() + 12, posY);
  posY += sldPandoraSpeed->getHeight() + DISTANCE_NEXT_Y;
#endif
  category.panel->add(chkBSDSocket, DISTANCE_BORDER, posY);
  posY += chkBSDSocket->getHeight() + DISTANCE_NEXT_Y;
  
  RefreshPanelMisc();
}


void ExitPanelMisc(void)
{
  delete chkStatusLine;
  delete chkHideIdleLed;
  delete chkShowGUI;
#ifdef PANDORA_SPECIFIC
  delete lblPandoraSpeed;
  delete sldPandoraSpeed;
  delete lblPandoraSpeedInfo;
#endif
  delete chkBSDSocket;
  delete miscActionListener;
}


void RefreshPanelMisc(void)
{
  char tmp[20];

  chkStatusLine->setSelected(changed_prefs.leds_on_screen);
  chkHideIdleLed->setSelected(changed_prefs.pandora_hide_idle_led);
  chkShowGUI->setSelected(changed_prefs.start_gui);
#ifdef PANDORA_SPECIFIC
  sldPandoraSpeed->setValue(changed_prefs.pandora_cpu_speed);
  snprintf(tmp, 20, "%d MHz", changed_prefs.pandora_cpu_speed);
  lblPandoraSpeedInfo->setCaption(tmp);
#endif
  
  chkBSDSocket->setSelected(changed_prefs.socket_emu);
}
