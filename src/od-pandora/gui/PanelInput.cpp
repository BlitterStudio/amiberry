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
#include "keyboard.h"
#include "inputdevice.h"


static const char *mousespeed_list[] = { ".25", ".5", "1x", "2x", "4x" };
static const int mousespeed_values[] = { 2, 5, 10, 20, 40 };

static gcn::Label *lblPort0;
static gcn::UaeDropDown* cboPort0;
static gcn::Label *lblPort1;
static gcn::UaeDropDown* cboPort1;

static gcn::Label *lblAutofire;
static gcn::UaeDropDown* cboAutofire;
static gcn::Label* lblMouseSpeed;
static gcn::Label* lblMouseSpeedInfo;
static gcn::Slider* sldMouseSpeed;
#ifndef RASPBERRY
static gcn::Label *lblTapDelay;
static gcn::UaeDropDown* cboTapDelay;
static gcn::UaeCheckBox* chkMouseHack;
#endif
  
static gcn::UaeCheckBox* chkCustomCtrl;
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
static gcn::Label *lblKeyForMenu;
static gcn::UaeDropDown* KeyForMenu;


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

    int AddElement(const char * Elem)
    {
      values.push_back(Elem);
      return 0;
    }

    std::string getElementAt(int i)
    {
      if(i < 0 || i >= values.size())
        return "---";
      return values[i];
    }
};

static const char *inputport_list[] = { "Nubs as mouse", "dPad as mouse", "dPad as joystick", "dPad as CD32 contr.", "none" };
StringListModel ctrlPortList(inputport_list, 5);

const char *autofireValues[] = { "Off", "Slow", "Medium", "Fast" };
StringListModel autofireList(autofireValues, 4);

#ifndef RASPBERRY
const char *tapDelayValues[] = { "Normal", "Short", "None" };
StringListModel tapDelayList(tapDelayValues, 3);
#endif

static const int ControlKey_SDLKeyValues[] = { SDLK_F11 , SDLK_F12, SDLK_LALT , SDLK_LCTRL };

const char *ControlKeyValues[] = { "F11", "F12", "LeftAlt", "LeftCtrl" };
StringListModel ControlKeyList(ControlKeyValues, 4);

const char *mappingValues[] = {
  "CD32 rwd", "CD32 ffw", "CD32 play", "CD32 yellow", "CD32 green",
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
StringListModel mappingList(mappingValues, 110);
static int amigaKey[] = 
 { REMAP_CD32_RWD,  REMAP_CD32_FFW, REMAP_CD32_PLAY, REMAP_CD32_YELLOW, REMAP_CD32_GREEN,
   REMAP_JOY_RIGHT, REMAP_JOY_LEFT, REMAP_JOY_DOWN,  REMAP_JOY_UP,      REMAP_JOYBUTTON_TWO, REMAP_JOYBUTTON_ONE, REMAP_MOUSEBUTTON_RIGHT, REMAP_MOUSEBUTTON_LEFT,
   0,             AK_UP,    AK_DN,      AK_LF,    AK_RT,        AK_NP0,       AK_NP1,         AK_NP2,       /*  13 -  20 */
   AK_NP3,        AK_NP4,   AK_NP5,     AK_NP6,   AK_NP7,       AK_NP8,       AK_NP9,         AK_ENT,       /*  21 -  28 */
   AK_NPDIV,      AK_NPMUL, AK_NPSUB,   AK_NPADD, AK_NPDEL,     AK_NPLPAREN,  AK_NPRPAREN,    AK_SPC,       /*  29 -  36 */
   AK_BS,         AK_TAB,   AK_RET,     AK_ESC,   AK_DEL,       AK_LSH,       AK_RSH,         AK_CAPSLOCK,  /*  37 -  44 */
   AK_CTRL,       AK_LALT,  AK_RALT,    AK_LAMI,  AK_RAMI,      AK_HELP,      AK_LBRACKET,    AK_RBRACKET,  /*  45 -  52 */
   AK_SEMICOLON,  AK_COMMA, AK_PERIOD,  AK_SLASH, AK_BACKSLASH, AK_QUOTE,     AK_NUMBERSIGN,  AK_LTGT,      /*  53 -  60 */
   AK_BACKQUOTE,  AK_MINUS, AK_EQUAL,   AK_A,     AK_B,         AK_C,         AK_D,           AK_E,         /*  61 -  68 */
   AK_F,          AK_G,     AK_H,       AK_I,     AK_J,         AK_K,         AK_L,           AK_M,         /*  69 -  76 */
   AK_N,          AK_O,     AK_P,       AK_Q,     AK_R,         AK_S,         AK_T,           AK_U,         /*  77 -  84 */
   AK_V,          AK_W,     AK_X,       AK_Y,     AK_Z,         AK_1,         AK_2,           AK_3,         /*  85 -  92 */
   AK_4,          AK_5,     AK_6,       AK_7,     AK_8,         AK_9,         AK_0,           AK_F1,        /*  93 - 100 */
   AK_F2,         AK_F3,    AK_F4,      AK_F5,    AK_F6,        AK_F7,        AK_F8,          AK_F9,        /* 101 - 108 */
   AK_F10,        0 }; /*  109 - 110 */
extern int customControlMap[SDLK_LAST];

static int GetAmigaKeyIndex(int key)
{
  for(int i=0; i < 110; ++i) {
    if(amigaKey[i] == key)
      return i;
  }
  return 13; // Default: no key
}


class InputActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      if (actionEvent.getSource() == cboPort0) {
        // Handle new device in port 0
        switch(cboPort0->getSelected()) {
          case 0: changed_prefs.jports[0].id = JSEM_MICE;     changed_prefs.jports[0].mode = JSEM_MODE_MOUSE; break;
          case 1: changed_prefs.jports[0].id = JSEM_MICE + 1; changed_prefs.jports[0].mode = JSEM_MODE_MOUSE; break;
          case 2: changed_prefs.jports[0].id = JSEM_JOYS;     changed_prefs.jports[0].mode = JSEM_MODE_JOYSTICK; break;
          case 3: changed_prefs.jports[0].id = JSEM_JOYS;     changed_prefs.jports[0].mode = JSEM_MODE_JOYSTICK_CD32; break;
          case 4: changed_prefs.jports[0].id = -1;            changed_prefs.jports[0].mode = JSEM_MODE_DEFAULT; break;
          default:changed_prefs.jports[0].id = JSEM_JOYS + cboPort0->getSelected() - 4;
                  changed_prefs.jports[0].mode = JSEM_MODE_JOYSTICK;
                  break;
        }
        inputdevice_updateconfig(NULL, &changed_prefs);
      }
      
      else if (actionEvent.getSource() == cboPort1) {
        // Handle new device in port 1
        switch(cboPort1->getSelected()) {
          case 0: changed_prefs.jports[1].id = JSEM_MICE;     changed_prefs.jports[1].mode = JSEM_MODE_MOUSE; break;
          case 1: changed_prefs.jports[1].id = JSEM_MICE + 1; changed_prefs.jports[1].mode = JSEM_MODE_MOUSE; break;
          case 2: changed_prefs.jports[1].id = JSEM_JOYS;     changed_prefs.jports[1].mode = JSEM_MODE_JOYSTICK; break;
          case 3: changed_prefs.jports[1].id = JSEM_JOYS;     changed_prefs.jports[1].mode = JSEM_MODE_JOYSTICK_CD32; break;
          case 4: changed_prefs.jports[1].id = -1;            changed_prefs.jports[1].mode = JSEM_MODE_DEFAULT; break;
          default:changed_prefs.jports[1].id = JSEM_JOYS + cboPort1->getSelected() - 4;
                  changed_prefs.jports[1].mode = JSEM_MODE_JOYSTICK;
                  break;
        }
        inputdevice_updateconfig(NULL, &changed_prefs);
      }
      
      else if (actionEvent.getSource() == cboAutofire)
      {
        if(cboAutofire->getSelected() == 0)
          changed_prefs.input_autofire_linecnt = 0;
        else if(cboAutofire->getSelected() == 1)
          changed_prefs.input_autofire_linecnt = 12 * 312;
        else if (cboAutofire->getSelected() == 2)
          changed_prefs.input_autofire_linecnt = 8 * 312;
        else
          changed_prefs.input_autofire_linecnt = 4 * 312;
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

    	else if (actionEvent.getSource() == chkMouseHack)
  	  {
  	    changed_prefs.input_tablet = chkMouseHack->isSelected() ? TABLET_MOUSEHACK : TABLET_OFF;
  	  }
#endif
 	    else if (actionEvent.getSource() == chkCustomCtrl)
 	      changed_prefs.pandora_customControls = chkCustomCtrl->isSelected() ? 1 : 0;
 	        
 	    else if (actionEvent.getSource() == cboA)
        customControlMap[SDLK_HOME] = amigaKey[cboA->getSelected()];

 	    else if (actionEvent.getSource() == cboB)
        customControlMap[SDLK_END] = amigaKey[cboB->getSelected()];

 	    else if (actionEvent.getSource() == cboX)
        customControlMap[SDLK_PAGEDOWN] = amigaKey[cboX->getSelected()];

 	    else if (actionEvent.getSource() == cboY)
        customControlMap[SDLK_PAGEUP] = amigaKey[cboY->getSelected()];

 	    else if (actionEvent.getSource() == cboL)
        customControlMap[SDLK_RSHIFT] = amigaKey[cboL->getSelected()];

 	    else if (actionEvent.getSource() == cboR)
        customControlMap[SDLK_RCTRL] = amigaKey[cboR->getSelected()];

 	    else if (actionEvent.getSource() == cboUp)
        customControlMap[SDLK_UP] = amigaKey[cboUp->getSelected()];

 	    else if (actionEvent.getSource() == cboDown)
        customControlMap[SDLK_DOWN] = amigaKey[cboDown->getSelected()];

 	    else if (actionEvent.getSource() == cboLeft)
        customControlMap[SDLK_LEFT] = amigaKey[cboLeft->getSelected()];

 	    else if (actionEvent.getSource() == cboRight)
        customControlMap[SDLK_RIGHT] = amigaKey[cboRight->getSelected()];

 	    else if (actionEvent.getSource() == KeyForMenu)
        changed_prefs.key_for_menu = ControlKey_SDLKeyValues[KeyForMenu->getSelected()] ;

    }
};
static InputActionListener* inputActionListener;


void InitPanelInput(const struct _ConfigCategory& category)
{
  inputActionListener = new InputActionListener();

  if (ctrlPortList.getNumberOfElements() < (4 + inputdevice_get_device_total (IDTYPE_JOYSTICK)))
  {
    int i;
    for(i=0; i<(inputdevice_get_device_total (IDTYPE_JOYSTICK) - 1); i++)
    {
       ctrlPortList.AddElement(inputdevice_get_device_name(IDTYPE_JOYSTICK,i + 1));
    }
  }


  lblPort0 = new gcn::Label("Port0:");
  lblPort0->setSize(100, LABEL_HEIGHT);
  lblPort0->setAlignment(gcn::Graphics::RIGHT);
	cboPort0 = new gcn::UaeDropDown(&ctrlPortList);
  cboPort0->setSize(435, DROPDOWN_HEIGHT);
  cboPort0->setBaseColor(gui_baseCol);
  cboPort0->setId("cboPort0");
  cboPort0->addActionListener(inputActionListener);

  lblPort1 = new gcn::Label("Port1:");
  lblPort1->setSize(100, LABEL_HEIGHT);
  lblPort1->setAlignment(gcn::Graphics::RIGHT);
	cboPort1 = new gcn::UaeDropDown(&ctrlPortList);
  cboPort1->setSize(435, DROPDOWN_HEIGHT);
  cboPort1->setBaseColor(gui_baseCol);
  cboPort1->setId("cboPort1");
  cboPort1->addActionListener(inputActionListener);

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
  
  chkMouseHack = new gcn::UaeCheckBox("Enable mousehack");
  chkMouseHack->setId("MouseHack");
  chkMouseHack->addActionListener(inputActionListener);
#endif
	chkCustomCtrl = new gcn::UaeCheckBox("Custom Control");
	chkCustomCtrl->setId("CustomCtrl");
  chkCustomCtrl->addActionListener(inputActionListener);

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

  lblKeyForMenu = new gcn::Label("Key for Menu:");
  lblKeyForMenu->setSize(100, LABEL_HEIGHT);
  lblKeyForMenu->setAlignment(gcn::Graphics::RIGHT);
  KeyForMenu = new gcn::UaeDropDown(&ControlKeyList);
  KeyForMenu->setSize(150, DROPDOWN_HEIGHT);
  KeyForMenu->setBaseColor(gui_baseCol);
  KeyForMenu->setId("CKeyMenu");
  KeyForMenu->addActionListener(inputActionListener);

  int posY = DISTANCE_BORDER;
  category.panel->add(lblPort0, DISTANCE_BORDER, posY);
  category.panel->add(cboPort0, DISTANCE_BORDER + lblPort0->getWidth() + 8, posY);
  posY += cboPort0->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(lblPort1, DISTANCE_BORDER, posY);
  category.panel->add(cboPort1, DISTANCE_BORDER + lblPort1->getWidth() + 8, posY);

  posY += cboPort1->getHeight() + DISTANCE_NEXT_Y;
  category.panel->add(lblAutofire, DISTANCE_BORDER, posY);
  category.panel->add(cboAutofire, DISTANCE_BORDER + lblAutofire->getWidth() + 8, posY);
  posY += cboAutofire->getHeight() + DISTANCE_NEXT_Y;

  category.panel->add(lblMouseSpeed, DISTANCE_BORDER, posY);
  category.panel->add(sldMouseSpeed, DISTANCE_BORDER + lblMouseSpeed->getWidth() + 8, posY);
  category.panel->add(lblMouseSpeedInfo, sldMouseSpeed->getX() + sldMouseSpeed->getWidth() + 12, posY);
  posY += sldMouseSpeed->getHeight() + DISTANCE_NEXT_Y;
#ifndef RASPBERRY
  category.panel->add(chkMouseHack, DISTANCE_BORDER + lblA->getWidth() + 8, posY);
  category.panel->add(lblTapDelay, 300, posY);
  category.panel->add(cboTapDelay, 300 + lblTapDelay->getWidth() + 8, posY);
  posY += cboTapDelay->getHeight() + DISTANCE_NEXT_Y;
#endif
  category.panel->add(chkCustomCtrl, DISTANCE_BORDER + lblA->getWidth() + 8, posY);
  posY += chkCustomCtrl->getHeight() + DISTANCE_NEXT_Y;
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
  posY += cboLeft->getHeight() + DISTANCE_NEXT_Y;
  
  category.panel->add(lblKeyForMenu, DISTANCE_BORDER, posY);
  category.panel->add(KeyForMenu, DISTANCE_BORDER + lblLeft->getWidth() + 8, posY);
  posY += KeyForMenu->getHeight() + 4;

  RefreshPanelInput();
}


void ExitPanelInput(void)
{
  delete lblPort0;
  delete cboPort0;
  delete lblPort1;
  delete cboPort1;
  
  delete lblAutofire;
  delete cboAutofire;
  delete lblMouseSpeed;
  delete sldMouseSpeed;
  delete lblMouseSpeedInfo;
#ifndef RASPBERRY
  delete lblTapDelay;
  delete cboTapDelay;
  delete chkMouseHack;
#endif
  delete chkCustomCtrl;
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

  delete lblKeyForMenu;
  delete KeyForMenu;

  delete inputActionListener;
}


void RefreshPanelInput(void)
{
  int i;

  // Set current device in port 0
  switch(changed_prefs.jports[0].id) {
    case JSEM_MICE:
      cboPort0->setSelected(0); 
      break;
    case JSEM_MICE + 1: 
      cboPort0->setSelected(1); 
      break;
    case JSEM_JOYS:
      if(changed_prefs.jports[0].mode != JSEM_MODE_JOYSTICK_CD32)
        cboPort0->setSelected(2);
      else
        cboPort0->setSelected(3);
      break;
    case -1:
      cboPort0->setSelected(4); 
      break;
    default:
      cboPort0->setSelected(changed_prefs.jports[0].id-JSEM_JOYS + 4); 
      break;
  }
  
  // Set current device in port 1
  switch(changed_prefs.jports[1].id) {
    case JSEM_MICE:
      cboPort1->setSelected(0); 
      break;
    case JSEM_MICE + 1: 
      cboPort1->setSelected(1); 
      break;
    case JSEM_JOYS:
      if(changed_prefs.jports[1].mode != JSEM_MODE_JOYSTICK_CD32)
        cboPort1->setSelected(2);
      else
        cboPort1->setSelected(3); 
      break;
    case -1:
      cboPort1->setSelected(4); 
      break;
    default:
      cboPort1->setSelected(changed_prefs.jports[1].id-JSEM_JOYS + 4); 
      break;
  } 

	if (changed_prefs.input_autofire_linecnt == 0)
	  cboAutofire->setSelected(0);
	else if (changed_prefs.input_autofire_linecnt > 10 * 312)
    cboAutofire->setSelected(1);
  else if (changed_prefs.input_autofire_linecnt > 6 * 312)
    cboAutofire->setSelected(2);
  else
    cboAutofire->setSelected(3);

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
  
  chkMouseHack->setSelected(changed_prefs.input_tablet == TABLET_MOUSEHACK);
#endif
  chkCustomCtrl->setSelected(changed_prefs.pandora_customControls);
  cboA->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_HOME]));
  cboB->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_END]));
  cboX->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_PAGEDOWN]));
  cboY->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_PAGEUP]));
  cboL->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_RSHIFT]));
  cboR->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_RCTRL]));
  cboUp->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_UP]));
  cboDown->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_DOWN]));
  cboLeft->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_LEFT]));
  cboRight->setSelected(GetAmigaKeyIndex(customControlMap[SDLK_RIGHT]));

  for(i=0; i<4; ++i)
  {
    if(changed_prefs.key_for_menu == ControlKey_SDLKeyValues[i])
    {
      KeyForMenu->setSelected(i);
      break;
    }
  }
}
