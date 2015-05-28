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


static const char *mousespeed_list[] = { ".25", ".5", "1x", "2x", "4x" };
static const int mousespeed_values[] = { 2, 5, 10, 20, 40 };
#ifndef RASPBERRY
static const char *stylusoffset_list[] = { "None", "1 px", "2 px", "3 px", "4 px", "5 px", "6 px", "7 px", "8 px", "9 px", "10 px" };
static const int stylusoffset_values[] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 };
#endif

static gcn::Label *lblCtrlConfig;
static gcn::UaeDropDown* cboCtrlConfig;
static gcn::Label *lblJoystick;
static gcn::UaeDropDown* cboJoystick;
static gcn::Label *lblAutofire;
static gcn::UaeDropDown* cboAutofire;
static gcn::Label* lblMouseSpeed;
static gcn::Label* lblMouseSpeedInfo;
static gcn::Slider* sldMouseSpeed;
#ifndef RASPBERRY
static gcn::Label *lblTapDelay;
static gcn::UaeDropDown* cboTapDelay;
static gcn::Label* lblStylusOffset;
static gcn::Label* lblStylusOffsetInfo;
static gcn::Slider* sldStylusOffset;
#endif
  
static gcn::UaeCheckBox* chkCustomCtrl;
static gcn::Label *lblDPAD;
static gcn::UaeDropDown* cboDPAD;
static gcn::Label *lblA;
static gcn::UaeDropDown* cboA;
static gcn::Label *lblB;
static gcn::UaeDropDown* cboB;
static gcn::Label *lblX;
static gcn::UaeDropDown* cboX;
static gcn::Label *lblY;
static gcn::UaeDropDown* cboY;
static gcn::Label *lblL;
static gcn::UaeDropDown* cboL;
static gcn::Label *lblR;
static gcn::UaeDropDown* cboR;
static gcn::Label *lblUp;
static gcn::UaeDropDown* cboUp;
static gcn::Label *lblDown;
static gcn::UaeDropDown* cboDown;
static gcn::Label *lblLeft;
static gcn::UaeDropDown* cboLeft;
static gcn::Label *lblRight;
static gcn::UaeDropDown* cboRight;


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

const char *ctrlConfigValues[] = {
  "A=Autofire, X=Fire, Y=Space, B=2nd",
  "A=Fire, X=Autofire, Y=Space, B=2nd",
  "A=Autofire, X=Jump, Y=Fire, B=2nd",
  "A=Fire, X=Jump, Y=Autofire, B=2nd"
};
StringListModel ctrlConfigList(ctrlConfigValues, 4);

const char *joystickValues[] = { "Port0", "Port1", "Both" };
StringListModel joystickList(joystickValues, 3);

const char *autofireValues[] = { "Light", "Medium", "Heavy" };
StringListModel autofireList(autofireValues, 3);
#ifndef RASPBERRY
const char *tapDelayValues[] = { "Normal", "Short", "None" };
StringListModel tapDelayList(tapDelayValues, 3);
#endif
const char *dPADValues[] = { "Joystick", "Mouse", "Custom" };
StringListModel dPADList(dPADValues, 3);

const char *mappingValues[] = {
  "Joystick Right", "Joystick Left", "Joystick Down", "Joystick Up", 
  "Joystick fire but.2", "Joystick fire but.1", "Mouse right button", "Mouse left button",
  "------------------",
  "Arrow Up", "Arrow Down", "Arrow Left", "Arrow Right", "Numpad 0", "Numpad 1", "Numpad 2",
  "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9",
  "Numpad Enter", "Numpad /", "Numpad *", "Numpad -", "Numpad +",
  "Numpad Delete", "Numpad (", "Numpad )",
  "Space", "Backspace", "Tab", "Return", "Escape", "Delete",
  "Left Shift", "Right Shift", "CAPS LOCK", "CTRL", "Left ALT", "Right ALT",
  "Left Amiga Key", "Right Amiga Key", "Help", "Left Bracket", "Right Bracket",
  "Semicolon", "Comma", "Period", "Slash", "Backslash", "Quote", "#", 
  "</>", "Backquote", "-", "=",
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", 
  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NULL"
};
StringListModel mappingList(mappingValues, 105);


class InputActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == cboCtrlConfig)
        changed_prefs.pandora_joyConf = cboCtrlConfig->getSelected();
        
      else if (actionEvent.getSource() == cboJoystick)
        changed_prefs.pandora_joyPort = (cboJoystick->getSelected() + 1) % 3;
      
      else if (actionEvent.getSource() == cboAutofire)
      {
        if(cboAutofire->getSelected() == 0)
          changed_prefs.input_autofire_framecnt = 12;
        else if (cboAutofire->getSelected() == 1)
          changed_prefs.input_autofire_framecnt = 8;
        else
          changed_prefs.input_autofire_framecnt = 4;
      }
      
 	    else if (actionEvent.getSource() == sldMouseSpeed)
 	    {
    		changed_prefs.input_joymouse_multiplier = mousespeed_values[(int)(sldMouseSpeed->getValue())];
    		RefreshPanelInput();
    	}
#ifndef RASPBERRY
      else if (actionEvent.getSource() == cboTapDelay)
      {
        if(cboTapDelay->getSelected() == 0)
          changed_prefs.pandora_tapDelay = 10;
        else if (cboTapDelay->getSelected() == 1)
          changed_prefs.pandora_tapDelay = 5;
        else
          changed_prefs.pandora_tapDelay = 2;
      }
 	    else if (actionEvent.getSource() == sldStylusOffset)
 	    {
    		changed_prefs.pandora_stylusOffset = stylusoffset_values[(int)(sldStylusOffset->getValue())];
    		RefreshPanelInput();
    	}
#endif
 	    else if (actionEvent.getSource() == chkCustomCtrl)
 	      changed_prefs.pandora_customControls = chkCustomCtrl->isSelected() ? 1 : 0;
 	        
 	    else if (actionEvent.getSource() == cboDPAD)
        changed_prefs.pandora_custom_dpad = cboDPAD->getSelected();

 	    else if (actionEvent.getSource() == cboA)
        changed_prefs.pandora_custom_A = cboA->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboB)
        changed_prefs.pandora_custom_B = cboB->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboX)
        changed_prefs.pandora_custom_X = cboX->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboY)
        changed_prefs.pandora_custom_Y = cboY->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboL)
        changed_prefs.pandora_custom_L = cboL->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboR)
        changed_prefs.pandora_custom_R = cboR->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboUp)
        changed_prefs.pandora_custom_up = cboUp->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboDown)
        changed_prefs.pandora_custom_down = cboDown->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboLeft)
        changed_prefs.pandora_custom_left = cboLeft->getSelected() - 8;

 	    else if (actionEvent.getSource() == cboRight)
        changed_prefs.pandora_custom_right = cboRight->getSelected() - 8;
    }
};
static InputActionListener* inputActionListener;


void InitPanelInput(const struct _ConfigCategory& category)
{
  inputActionListener = new InputActionListener();

  lblCtrlConfig = new gcn::Label("Control Config:");
  lblCtrlConfig->setSize(100, LABEL_HEIGHT);
  lblCtrlConfig->setAlignment(gcn::Graphics::RIGHT);
	cboCtrlConfig = new gcn::UaeDropDown(&ctrlConfigList);
  cboCtrlConfig->setSize(280, DROPDOWN_HEIGHT);
  cboCtrlConfig->setBaseColor(gui_baseCol);
  cboCtrlConfig->setId("cboCtrlConfig");
  cboCtrlConfig->addActionListener(inputActionListener);

  lblJoystick = new gcn::Label("Joystick:");
  lblJoystick->setSize(100, LABEL_HEIGHT);
  lblJoystick->setAlignment(gcn::Graphics::RIGHT);
	cboJoystick = new gcn::UaeDropDown(&joystickList);
  cboJoystick->setSize(80, DROPDOWN_HEIGHT);
  cboJoystick->setBaseColor(gui_baseCol);
  cboJoystick->setId("cboJoystick");
  cboJoystick->addActionListener(inputActionListener);

  lblAutofire = new gcn::Label("Autofire Rate:");
  lblAutofire->setSize(100, LABEL_HEIGHT);
  lblAutofire->setAlignment(gcn::Graphics::RIGHT);
	cboAutofire = new gcn::UaeDropDown(&autofireList);
  cboAutofire->setSize(80, DROPDOWN_HEIGHT);
  cboAutofire->setBaseColor(gui_baseCol);
  cboAutofire->setId("cboAutofire");
  cboAutofire->addActionListener(inputActionListener);

	lblMouseSpeed = new gcn::Label("Mouse Speed:");
  lblMouseSpeed->setSize(100, LABEL_HEIGHT);
  lblMouseSpeed->setAlignment(gcn::Graphics::RIGHT);
  sldMouseSpeed = new gcn::Slider(0, 4);
  sldMouseSpeed->setSize(110, SLIDER_HEIGHT);
  sldMouseSpeed->setBaseColor(gui_baseCol);
	sldMouseSpeed->setMarkerLength(20);
	sldMouseSpeed->setStepLength(1);
	sldMouseSpeed->setId("MouseSpeed");
  sldMouseSpeed->addActionListener(inputActionListener);
  lblMouseSpeedInfo = new gcn::Label(".25");
#ifndef RASPBERRY
  lblTapDelay = new gcn::Label("Tap Delay:");
  lblTapDelay->setSize(100, LABEL_HEIGHT);
  lblTapDelay->setAlignment(gcn::Graphics::RIGHT);
	cboTapDelay = new gcn::UaeDropDown(&tapDelayList);
  cboTapDelay->setSize(80, DROPDOWN_HEIGHT);
  cboTapDelay->setBaseColor(gui_baseCol);
  cboTapDelay->setId("cboTapDelay");
  cboTapDelay->addActionListener(inputActionListener);

	lblStylusOffset = new gcn::Label("Stylus Offset:");
  lblStylusOffset->setSize(100, LABEL_HEIGHT);
  lblStylusOffset->setAlignment(gcn::Graphics::RIGHT);
  sldStylusOffset = new gcn::Slider(0, 10);
  sldStylusOffset->setSize(110, SLIDER_HEIGHT);
  sldStylusOffset->setBaseColor(gui_baseCol);
	sldStylusOffset->setMarkerLength(20);
	sldStylusOffset->setStepLength(1);
	sldStylusOffset->setId("StylusOffset");
  sldStylusOffset->addActionListener(inputActionListener);
  lblStylusOffsetInfo = new gcn::Label("10 px");
#endif
	chkCustomCtrl = new gcn::UaeCheckBox("Custom Control");
	chkCustomCtrl->setId("CustomCtrl");
  chkCustomCtrl->addActionListener(inputActionListener);

  lblDPAD = new gcn::Label("DPAD/Port1:");
  lblDPAD->setSize(100, LABEL_HEIGHT);
  lblDPAD->setAlignment(gcn::Graphics::RIGHT);
	cboDPAD = new gcn::UaeDropDown(&dPADList);
  cboDPAD->setSize(80, DROPDOWN_HEIGHT);
  cboDPAD->setBaseColor(gui_baseCol);
  cboDPAD->setId("cboDPAD");
  cboDPAD->addActionListener(inputActionListener);

  lblA = new gcn::Label("<A>:");
  lblA->setSize(100, LABEL_HEIGHT);
  lblA->setAlignment(gcn::Graphics::RIGHT);
	cboA = new gcn::UaeDropDown(&mappingList);
  cboA->setSize(150, DROPDOWN_HEIGHT);
  cboA->setBaseColor(gui_baseCol);
  cboA->setId("cboA");
  cboA->addActionListener(inputActionListener);

  lblB = new gcn::Label("<B>:");
  lblB->setSize(100, LABEL_HEIGHT);
  lblB->setAlignment(gcn::Graphics::RIGHT);
	cboB = new gcn::UaeDropDown(&mappingList);
  cboB->setSize(150, DROPDOWN_HEIGHT);
  cboB->setBaseColor(gui_baseCol);
  cboB->setId("cboB");
  cboB->addActionListener(inputActionListener);

  lblX = new gcn::Label("<X>:");
  lblX->setSize(100, LABEL_HEIGHT);
  lblX->setAlignment(gcn::Graphics::RIGHT);
	cboX = new gcn::UaeDropDown(&mappingList);
  cboX->setSize(150, DROPDOWN_HEIGHT);
  cboX->setBaseColor(gui_baseCol);
  cboX->setId("cboX");
  cboX->addActionListener(inputActionListener);

  lblY = new gcn::Label("<Y>:");
  lblY->setSize(100, LABEL_HEIGHT);
  lblY->setAlignment(gcn::Graphics::RIGHT);
	cboY = new gcn::UaeDropDown(&mappingList);
  cboY->setSize(150, DROPDOWN_HEIGHT);
  cboY->setBaseColor(gui_baseCol);
  cboY->setId("cboY");
  cboY->addActionListener(inputActionListener);

  lblL = new gcn::Label("<L>:");
  lblL->setSize(100, LABEL_HEIGHT);
  lblL->setAlignment(gcn::Graphics::RIGHT);
	cboL = new gcn::UaeDropDown(&mappingList);
  cboL->setSize(150, DROPDOWN_HEIGHT);
  cboL->setBaseColor(gui_baseCol);
  cboL->setId("cboL");
  cboL->addActionListener(inputActionListener);

  lblR = new gcn::Label("<R>:");
  lblR->setSize(100, LABEL_HEIGHT);
  lblR->setAlignment(gcn::Graphics::RIGHT);
	cboR = new gcn::UaeDropDown(&mappingList);
  cboR->setSize(150, DROPDOWN_HEIGHT);
  cboR->setBaseColor(gui_baseCol);
  cboR->setId("cboR");
  cboR->addActionListener(inputActionListener);

  lblUp = new gcn::Label("Up:");
  lblUp->setSize(100, LABEL_HEIGHT);
  lblUp->setAlignment(gcn::Graphics::RIGHT);
	cboUp = new gcn::UaeDropDown(&mappingList);
  cboUp->setSize(150, DROPDOWN_HEIGHT);
  cboUp->setBaseColor(gui_baseCol);
  cboUp->setId("cboUp");
  cboUp->addActionListener(inputActionListener);

  lblDown = new gcn::Label("Down:");
  lblDown->setSize(100, LABEL_HEIGHT);
  lblDown->setAlignment(gcn::Graphics::RIGHT);
	cboDown = new gcn::UaeDropDown(&mappingList);
  cboDown->setSize(150, DROPDOWN_HEIGHT);
  cboDown->setBaseColor(gui_baseCol);
  cboDown->setId("cboDown");
  cboDown->addActionListener(inputActionListener);

  lblLeft = new gcn::Label("Left:");
  lblLeft->setSize(100, LABEL_HEIGHT);
  lblLeft->setAlignment(gcn::Graphics::RIGHT);
	cboLeft = new gcn::UaeDropDown(&mappingList);
  cboLeft->setSize(150, DROPDOWN_HEIGHT);
  cboLeft->setBaseColor(gui_baseCol);
  cboLeft->setId("cboLeft");
  cboLeft->addActionListener(inputActionListener);

  lblRight = new gcn::Label("Right:");
  lblRight->setSize(100, LABEL_HEIGHT);
  lblRight->setAlignment(gcn::Graphics::RIGHT);
	cboRight = new gcn::UaeDropDown(&mappingList);
  cboRight->setSize(150, DROPDOWN_HEIGHT);
  cboRight->setBaseColor(gui_baseCol);
  cboRight->setId("cboRight");
  cboRight->addActionListener(inputActionListener);

  int posY = DISTANCE_BORDER;
  category.panel->add(lblCtrlConfig, DISTANCE_BORDER, posY);
  category.panel->add(cboCtrlConfig, DISTANCE_BORDER + lblCtrlConfig->getWidth() + 8, posY);
  posY += cboCtrlConfig->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(lblJoystick, DISTANCE_BORDER, posY);
  category.panel->add(cboJoystick, DISTANCE_BORDER + lblJoystick->getWidth() + 8, posY);
  category.panel->add(lblAutofire, 300, posY);
  category.panel->add(cboAutofire, 300 + lblAutofire->getWidth() + 8, posY);
  posY += cboAutofire->getHeight() + DISTANCE_NEXT_Y;
#ifndef RASPBERRY
  category.panel->add(lblTapDelay, DISTANCE_BORDER, posY);
  category.panel->add(cboTapDelay, DISTANCE_BORDER + lblTapDelay->getWidth() + 8, posY);
  posY += cboTapDelay->getHeight() + DISTANCE_NEXT_Y;
#endif
  category.panel->add(lblMouseSpeed, DISTANCE_BORDER, posY);
  category.panel->add(sldMouseSpeed, DISTANCE_BORDER + lblMouseSpeed->getWidth() + 8, posY);
  category.panel->add(lblMouseSpeedInfo, sldMouseSpeed->getX() + sldMouseSpeed->getWidth() + 12, posY);
  posY += sldMouseSpeed->getHeight() + DISTANCE_NEXT_Y;
#ifndef RASPBERRY
  category.panel->add(lblStylusOffset, DISTANCE_BORDER, posY);
  category.panel->add(sldStylusOffset, DISTANCE_BORDER + lblStylusOffset->getWidth() + 8, posY);
  category.panel->add(lblStylusOffsetInfo, sldStylusOffset->getX() + sldStylusOffset->getWidth() + 12, posY);
  posY += sldStylusOffset->getHeight() + DISTANCE_NEXT_Y;
#endif
  category.panel->add(lblDPAD, DISTANCE_BORDER, posY);
  category.panel->add(cboDPAD, DISTANCE_BORDER + lblDPAD->getWidth() + 8, posY);
  category.panel->add(chkCustomCtrl, 320, posY);
  posY += cboDPAD->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(lblA, DISTANCE_BORDER, posY);
  category.panel->add(cboA, DISTANCE_BORDER + lblA->getWidth() + 8, posY);
  category.panel->add(lblB, 300, posY);
  category.panel->add(cboB, 300 + lblB->getWidth() + 8, posY);
  posY += cboA->getHeight() + 4;
  category.panel->add(lblX, DISTANCE_BORDER, posY);
  category.panel->add(cboX, DISTANCE_BORDER + lblX->getWidth() + 8, posY);
  category.panel->add(lblY, 300, posY);
  category.panel->add(cboY, 300 + lblY->getWidth() + 8, posY);
  posY += cboX->getHeight() + 4;
  category.panel->add(lblL, DISTANCE_BORDER, posY);
  category.panel->add(cboL, DISTANCE_BORDER + lblL->getWidth() + 8, posY);
  category.panel->add(lblR, 300, posY);
  category.panel->add(cboR, 300 + lblR->getWidth() + 8, posY);
  posY += cboL->getHeight() + 4;
  category.panel->add(lblUp, DISTANCE_BORDER, posY);
  category.panel->add(cboUp, DISTANCE_BORDER + lblUp->getWidth() + 8, posY);
  category.panel->add(lblDown, 300, posY);
  category.panel->add(cboDown, 300 + lblDown->getWidth() + 8, posY);
  posY += cboUp->getHeight() + 4;
  category.panel->add(lblLeft, DISTANCE_BORDER, posY);
  category.panel->add(cboLeft, DISTANCE_BORDER + lblLeft->getWidth() + 8, posY);
  category.panel->add(lblRight, 300, posY);
  category.panel->add(cboRight, 300 + lblRight->getWidth() + 8, posY);
  posY += cboLeft->getHeight() + 4;
  
  RefreshPanelInput();
}


void ExitPanelInput(void)
{
  delete lblCtrlConfig;
  delete cboCtrlConfig;
  delete lblJoystick;
  delete cboJoystick;
  delete lblAutofire;
  delete cboAutofire;
  delete lblMouseSpeed;
  delete sldMouseSpeed;
  delete lblMouseSpeedInfo;
#ifndef RASPBERRY
  delete lblTapDelay;
  delete cboTapDelay;
  delete lblStylusOffset;
  delete sldStylusOffset;
  delete lblStylusOffsetInfo;
#endif
  delete chkCustomCtrl;
  delete lblDPAD;
  delete cboDPAD;
  delete lblA;
  delete cboA;
  delete lblB;
  delete cboB;
  delete lblX;
  delete cboX;
  delete lblY;
  delete cboY;
  delete lblL;
  delete cboL;
  delete lblR;
  delete cboR;
  delete lblUp;
  delete cboUp;
  delete lblDown;
  delete cboDown;
  delete lblLeft;
  delete cboLeft;
  delete lblRight;
  delete cboRight;

  delete inputActionListener;
}


void RefreshPanelInput(void)
{
  int i;

  cboCtrlConfig->setSelected(changed_prefs.pandora_joyConf);
  cboJoystick->setSelected((changed_prefs.pandora_joyPort + 2) % 3);

	if (changed_prefs.input_autofire_framecnt == 12)
    cboAutofire->setSelected(0);
  else if (changed_prefs.input_autofire_framecnt == 8)
    cboAutofire->setSelected(1);
  else
    cboAutofire->setSelected(2);

  for(i=0; i<5; ++i)
  {
    if(changed_prefs.input_joymouse_multiplier == mousespeed_values[i])
    {
      sldMouseSpeed->setValue(i);
      lblMouseSpeedInfo->setCaption(mousespeed_list[i]);
      break;
    }
  }
#ifndef RASPBERRY
	if (changed_prefs.pandora_tapDelay == 10)
    cboTapDelay->setSelected(0);
  else if (changed_prefs.pandora_tapDelay == 5)
    cboTapDelay->setSelected(1);
  else
    cboTapDelay->setSelected(2);

  for(i=0; i<11; ++i)
  {
    if(changed_prefs.pandora_stylusOffset == stylusoffset_values[i])
    {
      sldStylusOffset->setValue(i);
      lblStylusOffsetInfo->setCaption(stylusoffset_list[i]);
      break;
    }
  }
#endif
  chkCustomCtrl->setSelected(changed_prefs.pandora_customControls);
  cboDPAD->setSelected(changed_prefs.pandora_custom_dpad);
  cboA->setSelected(changed_prefs.pandora_custom_A + 8);
  cboB->setSelected(changed_prefs.pandora_custom_B + 8);
  cboX->setSelected(changed_prefs.pandora_custom_X + 8);
  cboY->setSelected(changed_prefs.pandora_custom_Y + 8);
  cboL->setSelected(changed_prefs.pandora_custom_L + 8);
  cboR->setSelected(changed_prefs.pandora_custom_R + 8);
  cboUp->setSelected(changed_prefs.pandora_custom_up + 8);
  cboDown->setSelected(changed_prefs.pandora_custom_down + 8);
  cboLeft->setSelected(changed_prefs.pandora_custom_left + 8);
  cboRight->setSelected(changed_prefs.pandora_custom_right + 8);
}
