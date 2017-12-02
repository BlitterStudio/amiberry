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
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "gui_handling.h"
#include "keyboard.h"
#include "inputdevice.h"

#ifdef ANDROIDSDL
#include <SDL_android.h>
#endif

static const char* mousespeed_list[] = {".25", ".5", "1x", "2x", "4x"};
static const int mousespeed_values[] = {2, 5, 10, 20, 40};

static gcn::Label* lblPort0;
static gcn::UaeDropDown* cboPort0;
static gcn::Label* lblPort1;
static gcn::UaeDropDown* cboPort1;

static gcn::UaeDropDown* cboPort0mode;
static gcn::UaeDropDown* cboPort1mode;

static gcn::Label* lblPort0mousemode;
static gcn::UaeDropDown* cboPort0mousemode;
static gcn::Label* lblPort1mousemode;
static gcn::UaeDropDown* cboPort1mousemode;

static gcn::Label* lblPort2;
static gcn::UaeDropDown* cboPort2;
static gcn::Label* lblPort3;
static gcn::UaeDropDown* cboPort3;

static gcn::UaeDropDown* cboPort2mode;
static gcn::UaeDropDown* cboPort3mode;



static gcn::Label* lblAutofire;
static gcn::UaeDropDown* cboAutofire;
static gcn::Label* lblMouseSpeed;
static gcn::Label* lblMouseSpeedInfo;
static gcn::Slider* sldMouseSpeed;
#ifdef PANDORA
static gcn::Label *lblTapDelay;
static gcn::UaeDropDown* cboTapDelay;
#endif
static gcn::UaeCheckBox* chkMouseHack;
  
static gcn::UaeCheckBox* chkCustomCtrl;
static gcn::Label* lblA;
static gcn::TextField* txtA;
static gcn::Button* cmdA;

static gcn::Label* lblB;
static gcn::TextField* txtB;
static gcn::Button* cmdB;

static gcn::Label* lblX;
static gcn::TextField* txtX;
static gcn::Button* cmdX;

static gcn::Label* lblY;
static gcn::TextField* txtY;
static gcn::Button* cmdY;

static gcn::Label* lblL;
static gcn::TextField* txtL;
static gcn::Button* cmdL;

static gcn::Label* lblR;
static gcn::TextField* txtR;
static gcn::Button* cmdR;

static gcn::Label* lblUp;
static gcn::TextField* txtUp;
static gcn::Button* cmdUp;

static gcn::Label* lblDown;
static gcn::TextField* txtDown;
static gcn::Button* cmdDown;

static gcn::Label* lblLeft;
static gcn::TextField* txtLeft;
static gcn::Button* cmdLeft;

static gcn::Label* lblRight;
static gcn::TextField* txtRight;
static gcn::Button* cmdRight;

static gcn::Label* lblPlay;
static gcn::TextField* txtPlay;
static gcn::Button* cmdPlay;

class StringListModel : public gcn::ListModel
{
  private:
    std::vector<std::string> values;
public:
	StringListModel(const char* entries[], int count)
	{
		for (int i = 0; i < count; ++i)
			values.push_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int AddElement(const char* Elem)
	{
		values.push_back(Elem);
		return 0;
	}

	string getElementAt(int i) override
	{
		if (i < 0 || i >= values.size())
			return "---";
		return values[i];
	}
};

#ifdef PANDORA
static const char* inputport_list[] = {"Mouse", "Arrows as mouse", "Arrows as joystick", "Arrows as CD32 contr.", "none"};
StringListModel ctrlPortList(inputport_list, 5);
#else
static StringListModel ctrlPortList(nullptr, 0);
static int portListIDs[MAX_INPUT_DEVICES];
#endif

const char* autofireValues[] = {"Off", "Slow", "Medium", "Fast"};
StringListModel autofireList(autofireValues, 4);

const char* mousemapValues[] = {"None", "Left", "Right", "Both"};
StringListModel ctrlPortMouseModeList(mousemapValues, 4);

const char* joyportmodes[] = {"Mouse", "Joystick", "CD32", "Default"};
StringListModel ctrlPortModeList(joyportmodes, 4);
const char *tapDelayValues[] = { "Normal", "Short", "None" };
StringListModel tapDelayList(tapDelayValues, 3);

#ifdef PANDORA
const char* mappingValues[] =
{
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
{
	REMAP_CD32_RWD, REMAP_CD32_FFW, REMAP_CD32_PLAY, REMAP_CD32_YELLOW, REMAP_CD32_GREEN,
	REMAP_JOY_RIGHT, REMAP_JOY_LEFT, REMAP_JOY_DOWN, REMAP_JOY_UP, REMAP_JOYBUTTON_TWO, REMAP_JOYBUTTON_ONE, REMAP_MOUSEBUTTON_RIGHT, REMAP_MOUSEBUTTON_LEFT,
	0, AK_UP, AK_DN, AK_LF, AK_RT, AK_NP0, AK_NP1, AK_NP2, /*  13 -  20 */
	AK_NP3, AK_NP4, AK_NP5, AK_NP6, AK_NP7, AK_NP8, AK_NP9, AK_ENT, /*  21 -  28 */
	AK_NPDIV, AK_NPMUL, AK_NPSUB, AK_NPADD, AK_NPDEL, AK_NPLPAREN, AK_NPRPAREN, AK_SPC, /*  29 -  36 */
	AK_BS, AK_TAB, AK_RET, AK_ESC, AK_DEL, AK_LSH, AK_RSH, AK_CAPSLOCK, /*  37 -  44 */
	AK_CTRL, AK_LALT, AK_RALT, AK_LAMI, AK_RAMI, AK_HELP, AK_LBRACKET, AK_RBRACKET, /*  45 -  52 */
	AK_SEMICOLON, AK_COMMA, AK_PERIOD, AK_SLASH, AK_BACKSLASH, AK_QUOTE, AK_NUMBERSIGN, AK_LTGT, /*  53 -  60 */
	AK_BACKQUOTE, AK_MINUS, AK_EQUAL, AK_A, AK_B, AK_C, AK_D, AK_E, /*  61 -  68 */
	AK_F, AK_G, AK_H, AK_I, AK_J, AK_K, AK_L, AK_M, /*  69 -  76 */
	AK_N, AK_O, AK_P, AK_Q, AK_R, AK_S, AK_T, AK_U, /*  77 -  84 */
	AK_V, AK_W, AK_X, AK_Y, AK_Z, AK_1, AK_2, AK_3, /*  85 -  92 */
	AK_4, AK_5, AK_6, AK_7, AK_8, AK_9, AK_0, AK_F1, /*  93 - 100 */
	AK_F2, AK_F3, AK_F4, AK_F5, AK_F6, AK_F7, AK_F8, AK_F9, /* 101 - 108 */
	AK_F10, 0
}; /*  109 - 110 */

static int GetAmigaKeyIndex(int key)
{
	for (int i = 0; i < 110; ++i)
	{
		if (amigaKey[i] == key)
			return i;
	}
	return 13; // Default: no key
}
#endif

class InputActionListener : public gcn::ActionListener
{
public:
	void clear_ports(const int sel, const int current_port)
	{
		for (auto i = 0; i < MAX_JPORTS; i++)
		{
			if (i == current_port)
				continue;
			if (changed_prefs.jports[i].id == portListIDs[sel])
			{
				changed_prefs.jports[i].id = JPORT_NONE; 
				changed_prefs.jports[i].idc.configname[0] = 0;
				changed_prefs.jports[i].idc.name[0] = 0;
				changed_prefs.jports[i].idc.shortid[0] = 0;
			}
		}
	}

	void action(const gcn::ActionEvent& actionEvent) override
	{
#ifdef PANDORA
      if (actionEvent.getSource() == cboPort0) {
        // Handle new device in port 0
        switch(cboPort0->getSelected()) {
          case 0: changed_prefs.jports[0].id = JSEM_MICE;     changed_prefs.jports[0].mode = JSEM_MODE_MOUSE; break;
          case 1: changed_prefs.jports[0].id = JSEM_MICE + 1; changed_prefs.jports[0].mode = JSEM_MODE_MOUSE; break;
          case 2: changed_prefs.jports[0].id = JSEM_JOYS;     changed_prefs.jports[0].mode = JSEM_MODE_JOYSTICK; break;
          case 3: changed_prefs.jports[0].id = JSEM_JOYS;     changed_prefs.jports[0].mode = JSEM_MODE_JOYSTICK_CD32; break;
          case 4: changed_prefs.jports[0].id = -1;            changed_prefs.jports[0].mode = JSEM_MODE_DEFAULT; break;
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
        }
        inputdevice_updateconfig(NULL, &changed_prefs);
      }
#else
		if (actionEvent.getSource() == cboPort0)
		{
			// Handle new device in port 0
			const int sel = cboPort0->getSelected();
			const int current_port = 0;

			// clear 
			clear_ports(sel, current_port);

			// set
			changed_prefs.jports[current_port].id = portListIDs[sel];
			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
		}
		else if (actionEvent.getSource() == cboPort1)
		{
			// Handle new device in port 1
			const int sel = cboPort1->getSelected();
			const int current_port = 1;

			// clear 
			clear_ports(sel, current_port);

			// set
			changed_prefs.jports[current_port].id = portListIDs[sel];
			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
		}
#endif

		else if (actionEvent.getSource() == cboPort2)
		{
			// Handle new device in port 2
			const int sel = cboPort2->getSelected();
			const int current_port = 2;
			
			// clear 
			clear_ports(sel, current_port);
			
			// set
			changed_prefs.jports[current_port].id = portListIDs[sel];
			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
		}

		else if (actionEvent.getSource() == cboPort3)
		{
			// Handle new device in port 3
			const int sel = cboPort3->getSelected();
			const int current_port = 3;

			// clear 
			clear_ports(sel, current_port);

			// set
			changed_prefs.jports[current_port].id = portListIDs[sel];
			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
		}

		else if (actionEvent.getSource() == cboPort0mode)
		{
			if (cboPort0mode->getSelected() == 0)
				changed_prefs.jports[0].mode = JSEM_MODE_MOUSE;
			else if (cboPort0mode->getSelected() == 1)
				changed_prefs.jports[0].mode = JSEM_MODE_JOYSTICK;
			else if (cboPort0mode->getSelected() == 2)
				changed_prefs.jports[0].mode = JSEM_MODE_JOYSTICK_CD32;
			else
				changed_prefs.jports[0].mode = JSEM_MODE_DEFAULT;

			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
		}
		else if (actionEvent.getSource() == cboPort1mode)
		{
			if (cboPort1mode->getSelected() == 0)
				changed_prefs.jports[1].mode = JSEM_MODE_MOUSE;
			else if (cboPort1mode->getSelected() == 1)
				changed_prefs.jports[1].mode = JSEM_MODE_JOYSTICK;
			else if (cboPort1mode->getSelected() == 2)
				changed_prefs.jports[1].mode = JSEM_MODE_JOYSTICK_CD32;
			else
				changed_prefs.jports[1].mode = JSEM_MODE_DEFAULT;

			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
		}

		// mousemap drop-down change
		else if (actionEvent.getSource() == cboPort0mousemode)
		{
			changed_prefs.jports[0].mousemap = cboPort0mousemode->getSelected();
			inputdevice_updateconfig(nullptr, &changed_prefs);
		}
		else if (actionEvent.getSource() == cboPort1mousemode)
		{
			changed_prefs.jports[1].mousemap = cboPort1mousemode->getSelected();
			inputdevice_updateconfig(nullptr, &changed_prefs);
		}

		else if (actionEvent.getSource() == cboAutofire)
		{
			if (cboAutofire->getSelected() == 0)
				changed_prefs.input_autofire_linecnt = 0;
			else if (cboAutofire->getSelected() == 1)
				changed_prefs.input_autofire_linecnt = 12 * 312;
			else if (cboAutofire->getSelected() == 2)
				changed_prefs.input_autofire_linecnt = 8 * 312;
			else
				changed_prefs.input_autofire_linecnt = 4 * 312;
		}

		else if (actionEvent.getSource() == sldMouseSpeed)
		{
			changed_prefs.input_joymouse_multiplier = mousespeed_values[int(sldMouseSpeed->getValue())];
			RefreshPanelInput();
		}
#ifdef PANDORA
	else if (actionEvent.getSource() == cboTapDelay)
      {
        if(cboTapDelay->getSelected() == 0)
          changed_prefs.pandora_tapDelay = 10;
        else if (cboTapDelay->getSelected() == 1)
          changed_prefs.pandora_tapDelay = 5;
        else
          changed_prefs.pandora_tapDelay = 2;
      }
#endif
    	else if (actionEvent.getSource() == chkMouseHack)
  	  {
#ifdef ANDROIDSDL
        if (chkMouseHack->isSelected())
             SDL_ANDROID_SetMouseEmulationMode(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        else
             SDL_ANDROID_SetMouseEmulationMode(1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
#endif
  	    changed_prefs.input_tablet = chkMouseHack->isSelected() ? TABLET_MOUSEHACK : TABLET_OFF;
  	  }
		else if (actionEvent.getSource() == chkCustomCtrl)
			changed_prefs.customControls = chkCustomCtrl->isSelected();

		else if (actionEvent.getSource() == cmdA)
			//changed_prefs.custom_a = amigaKey[txtA->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to A", "Cancel");
			if (key != nullptr)
			{
				txtA->setText(key);
				strcpy(changed_prefs.custom_a, key);
			}
		}

		else if (actionEvent.getSource() == cmdB)
			//changed_prefs.custom_b = amigaKey[txtB->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to B", "Cancel");
			if (key != nullptr)
			{
				txtB->setText(key);
				strcpy(changed_prefs.custom_b, key);
			}
		}

		else if (actionEvent.getSource() == cmdX)
			//changed_prefs.custom_x = amigaKey[txtX->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to X", "Cancel");
			if (key != nullptr)
			{
				txtX->setText(key);
				strcpy(changed_prefs.custom_x, key);
			}
		}

		else if (actionEvent.getSource() == cmdY)
			//changed_prefs.custom_y = amigaKey[txtY->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to Y", "Cancel");
			if (key != nullptr)
			{
				txtY->setText(key);
				strcpy(changed_prefs.custom_y, key);
			}
		}

		else if (actionEvent.getSource() == cmdL)
			//changed_prefs.custom_l = amigaKey[txtL->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to LShoulder", "Cancel");
			if (key != nullptr)
			{
				txtL->setText(key);
				strcpy(changed_prefs.custom_l, key);
			}
		}

		else if (actionEvent.getSource() == cmdR)
			//changed_prefs.custom_r = amigaKey[txtR->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to RShoulder", "Cancel");
			if (key != nullptr)
			{
				txtR->setText(key);
				strcpy(changed_prefs.custom_r, key);
			}
		}

		else if (actionEvent.getSource() == cmdUp)
			//changed_prefs.custom_up = amigaKey[txtUp->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to Up", "Cancel");
			if (key != nullptr)
			{
				txtUp->setText(key);
				strcpy(changed_prefs.custom_up, key);
			}
		}

		else if (actionEvent.getSource() == cmdDown)
			//changed_prefs.custom_down = amigaKey[txtDown->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to Down", "Cancel");
			if (key != nullptr)
			{
				txtDown->setText(key);
				strcpy(changed_prefs.custom_down, key);
			}
		}

		else if (actionEvent.getSource() == cmdLeft)
			//changed_prefs.custom_left = amigaKey[txtLeft->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to Left", "Cancel");
			if (key != nullptr)
			{
				txtLeft->setText(key);
				strcpy(changed_prefs.custom_left, key);
			}
		}

		else if (actionEvent.getSource() == cmdRight)
			//changed_prefs.custom_right = amigaKey[txtRight->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to Right", "Cancel");
			if (key != nullptr)
			{
				txtRight->setText(key);
				strcpy(changed_prefs.custom_right, key);
			}
		}

		else if (actionEvent.getSource() == txtPlay)
			//changed_prefs.custom_play = amigaKey[txtPlay->getSelected()];
		{
			const char* key = ShowMessageForInput("Press a key/button", "Press a key or button to map to Play", "Cancel");
			if (key != nullptr)
			{
				txtPlay->setText(key);
				strcpy(changed_prefs.custom_play, key);
			}
		}
	}
};

static InputActionListener* inputActionListener;


void InitPanelInput(const struct _ConfigCategory& category)
{
	
#ifndef PANDORA
	if (ctrlPortList.getNumberOfElements() == 0)
	{
		int idx = 0;
		ctrlPortList.AddElement("None");
		portListIDs[idx] = JPORT_NONE;

		int i;
		for (i = 0; i < inputdevice_get_device_total(IDTYPE_MOUSE); i++)
		{
			const auto device_name = inputdevice_get_device_name(IDTYPE_MOUSE, i);
			if (device_name && device_name[0])
			{
				ctrlPortList.AddElement(inputdevice_get_device_name(IDTYPE_MOUSE, i));
				idx++;
				portListIDs[idx] = JSEM_MICE + i;
			}
		}

		for (i = 0; i < inputdevice_get_device_total(IDTYPE_JOYSTICK); i++)
		{
			ctrlPortList.AddElement(inputdevice_get_device_name(IDTYPE_JOYSTICK, i));
			idx++;
			portListIDs[idx] = JSEM_JOYS + i;
		}
	}
#endif

	inputActionListener = new InputActionListener();
	int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X * 2;
	const int mode_length = 80;
	const int port_lbl_length = 120;

	lblPort0 = new gcn::Label("Port 0 [Mouse]:");
	lblPort0->setSize(port_lbl_length, LABEL_HEIGHT);
	lblPort0->setAlignment(gcn::Graphics::RIGHT);
	cboPort0 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort0->setSize(textFieldWidth, DROPDOWN_HEIGHT);
	cboPort0->setBaseColor(gui_baseCol);
	cboPort0->setBackgroundColor(colTextboxBackground);
	cboPort0->setId("cboPort0");
	cboPort0->addActionListener(inputActionListener);

	cboPort0mode = new gcn::UaeDropDown(&ctrlPortModeList);
	cboPort0mode->setSize(mode_length, DROPDOWN_HEIGHT);
	cboPort0mode->setBaseColor(gui_baseCol);
	cboPort0mode->setId("cboPort0mode");
	cboPort0mode->addActionListener(inputActionListener);

	lblPort1 = new gcn::Label("Port 1 [Joystick]:");
	lblPort1->setSize(port_lbl_length, LABEL_HEIGHT);
	lblPort1->setAlignment(gcn::Graphics::RIGHT);
	cboPort1 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort1->setSize(textFieldWidth, DROPDOWN_HEIGHT);
	cboPort1->setBaseColor(gui_baseCol);
	cboPort1->setBackgroundColor(colTextboxBackground);
	cboPort1->setId("cboPort1");
	cboPort1->addActionListener(inputActionListener);

	cboPort1mode = new gcn::UaeDropDown(&ctrlPortModeList);
	cboPort1mode->setSize(mode_length, DROPDOWN_HEIGHT);
	cboPort1mode->setBaseColor(gui_baseCol);
	cboPort1mode->setId("cboPort1mode");
	cboPort1mode->addActionListener(inputActionListener);

	lblPort2 = new gcn::Label("Port 2 [Parallel 1]:");
	lblPort2->setSize(port_lbl_length, LABEL_HEIGHT);
	lblPort2->setAlignment(gcn::Graphics::LEFT);
	cboPort2 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort2->setSize(textFieldWidth, DROPDOWN_HEIGHT);
	cboPort2->setBaseColor(gui_baseCol);
	cboPort2->setId("cboPort2");
	cboPort2->addActionListener(inputActionListener);

	lblPort3 = new gcn::Label("Port 3 [Parallel 2]:");
	lblPort3->setSize(port_lbl_length, LABEL_HEIGHT);
	lblPort3->setAlignment(gcn::Graphics::LEFT);
	cboPort3 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort3->setSize(textFieldWidth, DROPDOWN_HEIGHT);
	cboPort3->setBaseColor(gui_baseCol);
	cboPort3->setId("cboPort3");
	cboPort3->addActionListener(inputActionListener);

	lblPort0mousemode = new gcn::Label("Mouse Stick 0:");
	lblPort0mousemode->setSize(100, LABEL_HEIGHT);
	lblPort0mousemode->setAlignment(gcn::Graphics::RIGHT);
	cboPort0mousemode = new gcn::UaeDropDown(&ctrlPortMouseModeList);
	cboPort0mousemode->setSize(68, DROPDOWN_HEIGHT);
	cboPort0mousemode->setBaseColor(gui_baseCol);
	cboPort0mousemode->setId("cboPort0mousemode");
	cboPort0mousemode->addActionListener(inputActionListener);

	lblPort1mousemode = new gcn::Label("Mouse Stick 1:");
	lblPort1mousemode->setSize(100, LABEL_HEIGHT);
	lblPort1mousemode->setAlignment(gcn::Graphics::RIGHT);
	cboPort1mousemode = new gcn::UaeDropDown(&ctrlPortMouseModeList);
	cboPort1mousemode->setSize(68, DROPDOWN_HEIGHT);
	cboPort1mousemode->setBaseColor(gui_baseCol);
	cboPort1mousemode->setId("cboPort1mousemode");
	cboPort1mousemode->addActionListener(inputActionListener);

	lblAutofire = new gcn::Label("Autofire Rate:");
	lblAutofire->setSize(100, LABEL_HEIGHT);
	lblAutofire->setAlignment(gcn::Graphics::RIGHT);
	cboAutofire = new gcn::UaeDropDown(&autofireList);
	cboAutofire->setBaseColor(gui_baseCol);
	cboAutofire->setBackgroundColor(colTextboxBackground);
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

#ifdef PANDORA
	lblTapDelay = new gcn::Label("Tap Delay:");
  	lblTapDelay->setSize(100, LABEL_HEIGHT);
  	lblTapDelay->setAlignment(gcn::Graphics::RIGHT);
	cboTapDelay = new gcn::UaeDropDown(&tapDelayList);
  	cboTapDelay->setSize(80, DROPDOWN_HEIGHT);
  	cboTapDelay->setBaseColor(gui_baseCol);
  	cboTapDelay->setId("cboTapDelay");
  	cboTapDelay->addActionListener(inputActionListener);
#endif

  	chkMouseHack = new gcn::UaeCheckBox("Enable mousehack");
  	chkMouseHack->setId("MouseHack");
  	chkMouseHack->addActionListener(inputActionListener);
	chkCustomCtrl = new gcn::UaeCheckBox("Custom Control");
	chkCustomCtrl->setId("CustomCtrl");
	chkCustomCtrl->addActionListener(inputActionListener);

	lblA = new gcn::Label("Green:");
	lblA->setSize(80, LABEL_HEIGHT);
	lblA->setAlignment(gcn::Graphics::RIGHT);
	txtA = new gcn::TextField();
	txtA->setEnabled(false);
	txtA->setSize(85, txtA->getHeight());
	txtA->setBackgroundColor(colTextboxBackground);
	cmdA = new gcn::Button("...");
	cmdA->setId("cmdA");
	cmdA->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdA->setBaseColor(gui_baseCol);
	cmdA->addActionListener(inputActionListener);

	lblB = new gcn::Label("Blue:");
	lblB->setSize(80, LABEL_HEIGHT);
	lblB->setAlignment(gcn::Graphics::RIGHT);
	txtB = new gcn::TextField();
	txtB->setEnabled(false);
	txtB->setSize(85, txtB->getHeight());
	txtB->setBackgroundColor(colTextboxBackground);
	cmdB = new gcn::Button("...");
	cmdB->setId("cmdB");
	cmdB->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdB->setBaseColor(gui_baseCol);
	cmdB->addActionListener(inputActionListener);

	lblX = new gcn::Label("Red:");
	lblX->setSize(80, LABEL_HEIGHT);
	lblX->setAlignment(gcn::Graphics::RIGHT);
	txtX = new gcn::TextField();
	txtX->setEnabled(false);
	txtX->setSize(85, txtX->getHeight());
	txtX->setBackgroundColor(colTextboxBackground);
	cmdX = new gcn::Button("...");
	cmdX->setId("cmdX");
	cmdX->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdX->setBaseColor(gui_baseCol);
	cmdX->addActionListener(inputActionListener);

	lblY = new gcn::Label("Yellow:");
	lblY->setSize(80, LABEL_HEIGHT);
	lblY->setAlignment(gcn::Graphics::RIGHT);
	txtY = new gcn::TextField();
	txtY->setEnabled(false);
	txtY->setSize(85, txtY->getHeight());
	txtY->setBackgroundColor(colTextboxBackground);
	cmdY = new gcn::Button("...");
	cmdY->setId("cmdY");
	cmdY->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdY->setBaseColor(gui_baseCol);
	cmdY->addActionListener(inputActionListener);

	lblL = new gcn::Label("LShoulder:");
	lblL->setSize(80, LABEL_HEIGHT);
	lblL->setAlignment(gcn::Graphics::RIGHT);
	txtL = new gcn::TextField();
	txtL->setEnabled(false);
	txtL->setSize(85, txtL->getHeight());
	txtL->setBackgroundColor(colTextboxBackground);
	cmdL = new gcn::Button("...");
	cmdL->setId("cmdL");
	cmdL->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdL->setBaseColor(gui_baseCol);
	cmdL->addActionListener(inputActionListener);

	lblR = new gcn::Label("RShoulder:");
	lblR->setSize(80, LABEL_HEIGHT);
	lblR->setAlignment(gcn::Graphics::RIGHT);
	txtR = new gcn::TextField();
	txtR->setEnabled(false);
	txtR->setSize(85, txtR->getHeight());
	txtR->setBackgroundColor(colTextboxBackground);
	cmdR = new gcn::Button("...");
	cmdR->setId("cmdR");
	cmdR->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdR->setBaseColor(gui_baseCol);
	cmdR->addActionListener(inputActionListener);

	lblUp = new gcn::Label("Up:");
	lblUp->setSize(80, LABEL_HEIGHT);
	lblUp->setAlignment(gcn::Graphics::RIGHT);
	txtUp = new gcn::TextField();
	txtUp->setEnabled(false);
	txtUp->setSize(85, txtUp->getHeight());
	txtUp->setBackgroundColor(colTextboxBackground);
	cmdUp = new gcn::Button("...");
	cmdUp->setId("cmdUp");
	cmdUp->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdUp->setBaseColor(gui_baseCol);
	cmdUp->addActionListener(inputActionListener);

	lblDown = new gcn::Label("Down:");
	lblDown->setSize(80, LABEL_HEIGHT);
	lblDown->setAlignment(gcn::Graphics::RIGHT);
	txtDown = new gcn::TextField();
	txtDown->setEnabled(false);
	txtDown->setSize(85, txtDown->getHeight());
	txtDown->setBackgroundColor(colTextboxBackground);
	cmdDown = new gcn::Button("...");
	cmdDown->setId("cmdDown");
	cmdDown->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdDown->setBaseColor(gui_baseCol);
	cmdDown->addActionListener(inputActionListener);

	lblLeft = new gcn::Label("Left:");
	lblLeft->setSize(80, LABEL_HEIGHT);
	lblLeft->setAlignment(gcn::Graphics::RIGHT);
	txtLeft = new gcn::TextField();
	txtLeft->setEnabled(false);
	txtLeft->setSize(85, txtLeft->getHeight());
	txtLeft->setBackgroundColor(colTextboxBackground);
	cmdLeft = new gcn::Button("...");
	cmdLeft->setId("cmdLeft");
	cmdLeft->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdLeft->setBaseColor(gui_baseCol);
	cmdLeft->addActionListener(inputActionListener);

	lblRight = new gcn::Label("Right:");
	lblRight->setSize(80, LABEL_HEIGHT);
	lblRight->setAlignment(gcn::Graphics::RIGHT);
	txtRight = new gcn::TextField();
	txtRight->setEnabled(false);
	txtRight->setSize(85, txtRight->getHeight());
	txtRight->setBackgroundColor(colTextboxBackground);
	cmdDown = new gcn::Button("...");
	cmdDown->setId("cmdDown");
	cmdDown->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdDown->setBaseColor(gui_baseCol);
	cmdDown->addActionListener(inputActionListener);

	lblPlay = new gcn::Label("Play:");
	lblPlay->setSize(80, LABEL_HEIGHT);
	lblPlay->setAlignment(gcn::Graphics::RIGHT);
	txtPlay = new gcn::TextField();
	txtPlay->setEnabled(false);
	txtPlay->setSize(85, txtPlay->getHeight());
	txtPlay->setBackgroundColor(colTextboxBackground);
	cmdDown = new gcn::Button("...");
	cmdDown->setId("cmdDown");
	cmdDown->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdDown->setBaseColor(gui_baseCol);
	cmdDown->addActionListener(inputActionListener);

	int posY = DISTANCE_BORDER;
	category.panel->add(lblPort0, DISTANCE_BORDER, posY);
	category.panel->add(cboPort0, DISTANCE_BORDER + lblPort0->getWidth() + 8, posY);
	category.panel->add(cboPort0mode, +lblPort0->getWidth() + cboPort0->getWidth() + 30, posY);

	posY += cboPort0->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblPort1, DISTANCE_BORDER, posY);
	category.panel->add(cboPort1, DISTANCE_BORDER + lblPort1->getWidth() + 8, posY);
	category.panel->add(cboPort1mode, +lblPort1->getWidth() + cboPort1->getWidth() + 30, posY);

	posY += cboPort1->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblPort2, DISTANCE_BORDER, posY);
	category.panel->add(cboPort2, DISTANCE_BORDER + lblPort2->getWidth() + 8, posY);

	posY += cboPort2->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblPort3, DISTANCE_BORDER, posY);
	category.panel->add(cboPort3, DISTANCE_BORDER + lblPort3->getWidth() + 8, posY);

	posY += cboPort3->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblMouseSpeed, DISTANCE_BORDER, posY);
	category.panel->add(sldMouseSpeed, DISTANCE_BORDER + lblMouseSpeed->getWidth() + 8, posY);
	category.panel->add(lblMouseSpeedInfo, sldMouseSpeed->getX() + sldMouseSpeed->getWidth() + 12, posY);
	
	category.panel->add(lblPort0mousemode, 320, posY);
	category.panel->add(cboPort0mousemode, 320 + lblPort0mousemode->getWidth() + 8, posY);

	posY += sldMouseSpeed->getHeight() + DISTANCE_NEXT_Y * 2;
	category.panel->add(chkMouseHack, DISTANCE_BORDER + lblA->getWidth() + 8, posY);
	category.panel->add(lblPort1mousemode, 320, posY);
	category.panel->add(cboPort1mousemode, 320 + lblPort1mousemode->getWidth() + 8, posY);

	posY += cboAutofire->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblAutofire, DISTANCE_BORDER, posY);
	category.panel->add(cboAutofire, DISTANCE_BORDER + lblAutofire->getWidth() + 8, posY);
	posY += cboAutofire->getHeight() + DISTANCE_NEXT_Y;
  
#ifdef PANDORA
  category.panel->add(lblTapDelay, 300, posY);
  category.panel->add(cboTapDelay, 300 + lblTapDelay->getWidth() + 8, posY);
  posY += cboTapDelay->getHeight() + DISTANCE_NEXT_Y;
#else
  posY += chkMouseHack->getHeight() + DISTANCE_NEXT_Y;
#endif
	category.panel->add(chkCustomCtrl, DISTANCE_BORDER + lblA->getWidth() + 8, posY);
	posY += chkCustomCtrl->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblA, DISTANCE_BORDER, posY);
	category.panel->add(txtA, DISTANCE_BORDER + lblA->getWidth() + 8, posY);

	int posColumn2 = txtA->getX() + txtA->getWidth() + 12;

	category.panel->add(lblB, posColumn2, posY);
	category.panel->add(txtB, posColumn2 + lblB->getWidth() + 8, posY);
	posY += txtA->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblX, DISTANCE_BORDER, posY);
	category.panel->add(txtX, DISTANCE_BORDER + lblX->getWidth() + 8, posY);
	category.panel->add(lblY, posColumn2, posY);
	category.panel->add(txtY, posColumn2 + lblY->getWidth() + 8, posY);
	posY += txtX->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblL, DISTANCE_BORDER, posY);
	category.panel->add(txtL, DISTANCE_BORDER + lblL->getWidth() + 8, posY);
	category.panel->add(lblR, posColumn2, posY);
	category.panel->add(txtR, posColumn2 + lblR->getWidth() + 8, posY);
	posY += txtL->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblUp, DISTANCE_BORDER, posY);
	category.panel->add(txtUp, DISTANCE_BORDER + lblUp->getWidth() + 8, posY);
	category.panel->add(lblDown, posColumn2, posY);
	category.panel->add(txtDown, posColumn2 + lblDown->getWidth() + 8, posY);
	posY += txtUp->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblLeft, DISTANCE_BORDER, posY);
	category.panel->add(txtLeft, DISTANCE_BORDER + lblLeft->getWidth() + 8, posY);
	category.panel->add(lblRight, posColumn2, posY);
	category.panel->add(txtRight, posColumn2 + lblRight->getWidth() + 8, posY);
	posY += txtLeft->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblPlay, DISTANCE_BORDER, posY);
	category.panel->add(txtPlay, DISTANCE_BORDER + lblPlay->getWidth() + 8, posY);

	RefreshPanelInput();
}


void ExitPanelInput()
{
	delete lblPort0;
	delete cboPort0;
	delete cboPort0mode;
	delete lblPort0mousemode;
	delete cboPort0mousemode;

	delete lblPort1;
	delete cboPort1;
	delete cboPort1mode;

	delete lblPort1mousemode;
	delete cboPort1mousemode;

	delete lblPort2;
	delete cboPort2;
	delete lblPort3;
	delete cboPort3;

	delete lblAutofire;
	delete cboAutofire;
	delete lblMouseSpeed;
	delete sldMouseSpeed;
	delete lblMouseSpeedInfo;
#ifdef PANDORA
  	delete lblTapDelay;
  	delete cboTapDelay;
#endif
  	delete chkMouseHack;

	delete chkCustomCtrl;
	delete lblA;
	delete txtA;
	delete lblB;
	delete txtB;
	delete lblX;
	delete txtX;
	delete lblY;
	delete txtY;
	delete lblL;
	delete txtL;
	delete lblR;
	delete txtR;
	delete lblUp;
	delete txtUp;
	delete lblDown;
	delete txtDown;
	delete lblLeft;
	delete txtLeft;
	delete lblRight;
	delete txtRight;
	delete txtPlay;

	delete inputActionListener;
}


void RefreshPanelInput()
{
#ifdef PANDORA
  int i;
	// Set current device in port 0
	switch (changed_prefs.jports[0].id)
	{
	case JSEM_MICE:
		cboPort0->setSelected(0);
		break;
	case JSEM_MICE + 1:
		cboPort0->setSelected(1);
		break;
	case JSEM_JOYS:
		if (changed_prefs.jports[0].mode != JSEM_MODE_JOYSTICK_CD32)
			cboPort0->setSelected(2);
		else
			cboPort0->setSelected(3);
		break;
	case -1:
		cboPort0->setSelected(4);
		break;
	default:
		cboPort0->setSelected(changed_prefs.jports[0].id - JSEM_JOYS + 4);
		break;
	}

	// Set current device in port 1
	switch (changed_prefs.jports[1].id)
	{
	case JSEM_MICE:
		cboPort1->setSelected(0);
		break;
	case JSEM_MICE + 1:
		cboPort1->setSelected(1);
		break;
	case JSEM_JOYS:
		if (changed_prefs.jports[1].mode != JSEM_MODE_JOYSTICK_CD32)
			cboPort1->setSelected(2);
		else
			cboPort1->setSelected(3);
		break;
	case -1:
		cboPort1->setSelected(4);
		break;
	default:
		cboPort1->setSelected(changed_prefs.jports[1].id - JSEM_JOYS + 4);
		break;
	}
#else

	// Set current device in port 0
	int idx = 0;
	for (int i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
	{
		if (changed_prefs.jports[0].id == portListIDs[i])
		{
			idx = i;
			break;
		}
	}
	cboPort0->setSelected(idx);

	// Set current device in port 1
	idx = 0;
	for (int i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
	{
		if (changed_prefs.jports[1].id == portListIDs[i])
		{
			idx = i;
			break;
		}
	}
	cboPort1->setSelected(idx);
#endif

	// Set current device in port 2
	idx = 0;
	for (int i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
	{
		if (changed_prefs.jports[2].id == portListIDs[i])
		{
			idx = i;
			break;
		}
	}
	cboPort2->setSelected(idx);

	// Set current device in port 3
	idx = 0;
	for (int i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
	{
		if (changed_prefs.jports[3].id == portListIDs[i])
		{
			idx = i;
			break;
		}
	}
	cboPort3->setSelected(idx);

	if (changed_prefs.input_autofire_linecnt == 0)
		cboAutofire->setSelected(0);
	else if (changed_prefs.input_autofire_linecnt > 10 * 312)
		cboAutofire->setSelected(1);
	else if (changed_prefs.input_autofire_linecnt > 6 * 312)
		cboAutofire->setSelected(2);
	else
		cboAutofire->setSelected(3);


	if (changed_prefs.jports[0].mode == JSEM_MODE_MOUSE)
		cboPort0mode->setSelected(0);
	else if (changed_prefs.jports[0].mode == JSEM_MODE_JOYSTICK)
		cboPort0mode->setSelected(1);
	else if (changed_prefs.jports[0].mode == JSEM_MODE_JOYSTICK_CD32)
		cboPort0mode->setSelected(2);
	else
		cboPort0mode->setSelected(3);


	if (changed_prefs.jports[1].mode == JSEM_MODE_MOUSE)
		cboPort1mode->setSelected(0);
	else if (changed_prefs.jports[1].mode == JSEM_MODE_JOYSTICK)
		cboPort1mode->setSelected(1);
	else if (changed_prefs.jports[1].mode == JSEM_MODE_JOYSTICK_CD32)
		cboPort1mode->setSelected(2);
	else
		cboPort1mode->setSelected(3);

	// changed mouse map
	cboPort0mousemode->setSelected(changed_prefs.jports[0].mousemap);
	cboPort1mousemode->setSelected(changed_prefs.jports[1].mousemap);

	if (cboPort0mode->getSelected() == 0)
	{
		cboPort0mousemode->setEnabled(false);
	}
	else
	{
		cboPort0mousemode->setEnabled(true);
	}

	if (cboPort1mode->getSelected() == 0)
	{
		cboPort1mousemode->setEnabled(false);
	}
	else
	{
		cboPort1mousemode->setEnabled(true);
	}

	for (int i = 0; i < 5; ++i)
	{
		if (changed_prefs.input_joymouse_multiplier == mousespeed_values[i])
		{
			sldMouseSpeed->setValue(i);
			lblMouseSpeedInfo->setCaption(mousespeed_list[i]);
			break;
		}
	}
#ifdef PANDORA
	if (changed_prefs.pandora_tapDelay == 10)
		cboTapDelay->setSelected(0);
	else if (changed_prefs.pandora_tapDelay == 5)
		cboTapDelay->setSelected(1);
	else
		cboTapDelay->setSelected(2);
#endif
	chkMouseHack->setSelected(changed_prefs.input_tablet == TABLET_MOUSEHACK);

	chkCustomCtrl->setSelected(changed_prefs.customControls);

	txtA->setText(changed_prefs.custom_a);
	txtB->setText(changed_prefs.custom_b);
	txtX->setText(changed_prefs.custom_x);
	txtY->setText(changed_prefs.custom_y);
	txtL->setText(changed_prefs.custom_l);
	txtR->setText(changed_prefs.custom_r);
	txtUp->setText(changed_prefs.custom_up);
	txtDown->setText(changed_prefs.custom_down);
	txtLeft->setText(changed_prefs.custom_left);
	txtRight->setText(changed_prefs.custom_right);
	txtPlay->setText(changed_prefs.custom_play);
}

bool HelpPanelInput(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.push_back("You can select the control type for both ports and the rate for autofire.");
	helptext.push_back("");
	helptext.push_back("Set the emulated mouse speed to .25x, .5x, 1x, 2x and 4x to slow down or speed up the mouse.");
	helptext.push_back("");
  	helptext.push_back("When \"Enable mousehack\" is activated, you can use the stylus to set the mouse pointer to the exact position.");
  	helptext.push_back("This works very well on Workbench, but many games using there own mouse handling and will not profit from");
  	helptext.push_back("this code.");
  	helptext.push_back("");
  	helptext.push_back("\"Tap Delay\" specifies the time between taping the screen and an emulated mouse button click.");
  	helptext.push_back("");
	helptext.push_back("When you activate \"Custom Control\", you can define which Amiga key should be emulated by pressing one of the");
	helptext.push_back("ABXY- or D-pad buttons. Useful to setup controls for pinball games. During emulation, you can switch between");
	helptext.push_back("regular buttons and custom settings by pressing left shoulder button and 'c'.");
	return true;
}
