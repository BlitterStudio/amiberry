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

#ifdef RASPBERRY
class StringListModel : public gcn::ListModel
{
  private:
    std::vector<std::string> values;
  public:
    StringListModel(const char *entries[], int count)
    {
      for(int i=0; i<count; ++i)
      values.push_back(entries[i]);
    }

    int getNumberOfElements()
    {
      return values.size();
    }

    std::string getElementAt(int i)
    {
      if(i < 0 || i >= values.size())
        return "---";
      return values[i];
    }
};


static gcn::Label *lblNumLock;
static gcn::UaeDropDown* cboKBDLed_num;
static gcn::Label *lblScrLock;
static gcn::UaeDropDown* cboKBDLed_scr;
static gcn::Label *lblCapLock;
static gcn::UaeDropDown* cboKBDLed_cap;

const char *listValues[] = { "", "POWER", "DF0", "DF1", "DF2", "DF3", "DF*", "HD" };
StringListModel KBDLedList(listValues, 8);
#endif

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

#ifdef RASPBERRY
     else if (actionEvent.getSource() == cboKBDLed_num)
     {
       if (cboKBDLed_num->getSelected() == 0) changed_prefs.kbd_led_num = -1; // Nothing
       if (cboKBDLed_num->getSelected() == 7) changed_prefs.kbd_led_num = 5;  // HD
       if (cboKBDLed_num->getSelected() == 6) changed_prefs.kbd_led_num = -2; // Any DFs
       if (cboKBDLed_num->getSelected() >= 1 && cboKBDLed_num->getSelected() <= 4) changed_prefs.kbd_led_num = cboKBDLed_num->getSelected() - 1; // Specific DF#
     }
     else if (actionEvent.getSource() == cboKBDLed_cap)
     {
       if (cboKBDLed_cap->getSelected() == 0) changed_prefs.kbd_led_cap = -1;
       if (cboKBDLed_cap->getSelected() == 7) changed_prefs.kbd_led_cap = 5;
       if (cboKBDLed_cap->getSelected() == 6) changed_prefs.kbd_led_cap = -2;
       if (cboKBDLed_cap->getSelected() >= 1 && cboKBDLed_cap->getSelected() <= 4) changed_prefs.kbd_led_cap = cboKBDLed_cap->getSelected() - 1;
     }
     else if (actionEvent.getSource() == cboKBDLed_scr)
     {
       if (cboKBDLed_scr->getSelected() == 0) changed_prefs.kbd_led_scr = -1;
       if (cboKBDLed_scr->getSelected() == 7) changed_prefs.kbd_led_scr = 5;
       if (cboKBDLed_scr->getSelected() == 6) changed_prefs.kbd_led_scr = -2;
       if (cboKBDLed_scr->getSelected() >= 1 && cboKBDLed_scr->getSelected() <= 4) changed_prefs.kbd_led_scr = cboKBDLed_scr->getSelected() - 1;
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

#ifdef RASPBERRY
  lblNumLock = new gcn::Label("NumLock LED");
  lblNumLock->setSize(150, LABEL_HEIGHT);
  lblNumLock->setAlignment(gcn::Graphics::LEFT);
  cboKBDLed_num = new gcn::UaeDropDown(&KBDLedList);
  cboKBDLed_num->setSize(150, DROPDOWN_HEIGHT);
  cboKBDLed_num->setBaseColor(gui_baseCol);
  cboKBDLed_num->setId("numlock");
  cboKBDLed_num->addActionListener(miscActionListener);

  lblCapLock = new gcn::Label("CapsLock LED");
  lblCapLock->setSize(150, LABEL_HEIGHT);
  lblCapLock->setAlignment(gcn::Graphics::LEFT);
  cboKBDLed_cap = new gcn::UaeDropDown(&KBDLedList);
  cboKBDLed_cap->setSize(150, DROPDOWN_HEIGHT);
  cboKBDLed_cap->setBaseColor(gui_baseCol);
  cboKBDLed_cap->setId("capslock");
  cboKBDLed_cap->addActionListener(miscActionListener);

  lblScrLock = new gcn::Label("ScrollLock LED");
  lblScrLock->setSize(150, LABEL_HEIGHT);
  lblScrLock->setAlignment(gcn::Graphics::LEFT);
  cboKBDLed_scr = new gcn::UaeDropDown(&KBDLedList);
  cboKBDLed_scr->setSize(150, DROPDOWN_HEIGHT);
  cboKBDLed_scr->setBaseColor(gui_baseCol);
  cboKBDLed_scr->setId("scrolllock");
  cboKBDLed_scr->addActionListener(miscActionListener);

  category.panel->add(lblNumLock, DISTANCE_BORDER, posY);
//  category.panel->add(lblCapLock, lblNumLock->getX() + lblNumLock->getWidth() + DISTANCE_NEXT_X, posY);
  category.panel->add(lblScrLock, lblCapLock->getX() + lblCapLock->getWidth() + DISTANCE_NEXT_X, posY);
  posY += lblNumLock->getHeight() + 4;

  category.panel->add(cboKBDLed_num, DISTANCE_BORDER, posY);
//  category.panel->add(cboKBDLed_cap, cboKBDLed_num->getX() + cboKBDLed_num->getWidth() + DISTANCE_NEXT_X, posY);
  category.panel->add(cboKBDLed_scr, cboKBDLed_cap->getX() + cboKBDLed_cap->getWidth() + DISTANCE_NEXT_X, posY);

  posY += cboKBDLed_scr->getHeight() + DISTANCE_NEXT_Y;
#endif  
 
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

#ifdef RASPBERRY
  delete lblCapLock;
  delete lblScrLock;
  delete lblNumLock;
  delete cboKBDLed_num;
  delete cboKBDLed_cap;
  delete cboKBDLed_scr;
#endif
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
#ifdef RASPBERRY
  if (changed_prefs.kbd_led_num == -1) cboKBDLed_num->setSelected(0);
  if (changed_prefs.kbd_led_num == -2) cboKBDLed_num->setSelected(6);
  if (changed_prefs.kbd_led_num == 5) cboKBDLed_num->setSelected(7);
  if (changed_prefs.kbd_led_num >= 1 && changed_prefs.kbd_led_num <= 4) cboKBDLed_num->setSelected(changed_prefs.kbd_led_num + 1);
  if (changed_prefs.kbd_led_cap == -1) cboKBDLed_cap->setSelected(0);
  if (changed_prefs.kbd_led_cap == -2) cboKBDLed_cap->setSelected(6);
  if (changed_prefs.kbd_led_cap == 5) cboKBDLed_cap->setSelected(7);
  if (changed_prefs.kbd_led_cap >= 1 && changed_prefs.kbd_led_cap <= 4) cboKBDLed_cap->setSelected(changed_prefs.kbd_led_cap + 1);
  if (changed_prefs.kbd_led_scr == -1) cboKBDLed_scr->setSelected(0);
  if (changed_prefs.kbd_led_scr == -2) cboKBDLed_scr->setSelected(6);
  if (changed_prefs.kbd_led_scr == 5) cboKBDLed_scr->setSelected(7);
  if (changed_prefs.kbd_led_scr >= 1 && changed_prefs.kbd_led_scr <= 4) cboKBDLed_scr->setSelected(changed_prefs.kbd_led_scr + 1);
#endif
}
