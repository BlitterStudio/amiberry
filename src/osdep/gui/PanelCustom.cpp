#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#endif
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

#include <iostream> // horace

static gcn::Window* grpPort;
static gcn::UaeRadioButton* optPort0;
static gcn::UaeRadioButton* optPort1;
static gcn::UaeRadioButton* optPort2;
static gcn::UaeRadioButton* optPort3;

static gcn::Window* grpFunction;
static gcn::UaeRadioButton* optMultiNone;
static gcn::UaeRadioButton* optMultiSelect;
static gcn::UaeRadioButton* optMultiLeft;
static gcn::UaeRadioButton* optMultiRight;

static gcn::Label* lblCustomAction[14];
static gcn::UaeDropDown* cboCustomAction[14];

static gcn::Label* lblPortInput;
static gcn::TextField* txtPortInput;
static gcn::Label* lblRetroarch;

static int SelectedPort = 1;
static int SelectedFunction = 0;


class StringListModel : public gcn::ListModel
{
private:
	vector<string> values;
public:
	StringListModel(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int AddElement(const char* Elem)
	{
		values.emplace_back(Elem);
		return 0;
	}

	int SwapFirstElement(const char* Elem)
	{
		values.erase(values.begin());
		values.emplace(values.begin(), Elem);
		return 0;
	}

	string getElementAt(int i) override
	{
		if (i < 0 || i >= values.size())
			return "---";
		return values[i];
	}
};

static StringListModel CustomEventList(nullptr, 0);
static StringListModel CustomEventList_HotKey(nullptr, 0);
static StringListModel CustomEventList_Menu(nullptr, 0);
static StringListModel CustomEventList_Quit(nullptr, 0);
static StringListModel CustomEventList_Reset(nullptr, 0);


const int RemapEventList[] = {
	// joystick port 1
	INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON, INPUTEVENT_JOY2_3RD_BUTTON,
	INPUTEVENT_JOY2_CD32_RED, INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW, INPUTEVENT_JOY2_CD32_PLAY,
	INPUTEVENT_MOUSE2_UP, INPUTEVENT_MOUSE2_DOWN, INPUTEVENT_MOUSE2_LEFT, INPUTEVENT_MOUSE2_RIGHT,
	// joystick port 0            
	INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN, INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT,
	INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON, INPUTEVENT_JOY1_3RD_BUTTON,
	INPUTEVENT_JOY1_CD32_RED, INPUTEVENT_JOY1_CD32_BLUE, INPUTEVENT_JOY1_CD32_GREEN, INPUTEVENT_JOY1_CD32_YELLOW,
	INPUTEVENT_JOY1_CD32_RWD, INPUTEVENT_JOY1_CD32_FFW, INPUTEVENT_JOY1_CD32_PLAY,
	INPUTEVENT_MOUSE1_UP, INPUTEVENT_MOUSE1_DOWN, INPUTEVENT_MOUSE1_LEFT, INPUTEVENT_MOUSE1_RIGHT,
	// joystick port 2 (parallel 1)
	INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN, INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT,
	INPUTEVENT_PAR_JOY1_FIRE_BUTTON, INPUTEVENT_PAR_JOY1_2ND_BUTTON,
	// joystick port 3 (parallel 2)
	INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN, INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT,
	INPUTEVENT_PAR_JOY2_FIRE_BUTTON, INPUTEVENT_PAR_JOY2_2ND_BUTTON,
	// keyboard
	INPUTEVENT_KEY_F1, INPUTEVENT_KEY_F2, INPUTEVENT_KEY_F3, INPUTEVENT_KEY_F4, INPUTEVENT_KEY_F5,
	INPUTEVENT_KEY_F6, INPUTEVENT_KEY_F7, INPUTEVENT_KEY_F8, INPUTEVENT_KEY_F9, INPUTEVENT_KEY_F10,
	INPUTEVENT_KEY_ESC, INPUTEVENT_KEY_TAB, INPUTEVENT_KEY_CTRL, INPUTEVENT_KEY_CAPS_LOCK, INPUTEVENT_KEY_LTGT,
	INPUTEVENT_KEY_SHIFT_LEFT, INPUTEVENT_KEY_ALT_LEFT, INPUTEVENT_KEY_AMIGA_LEFT,
	INPUTEVENT_KEY_AMIGA_RIGHT, INPUTEVENT_KEY_ALT_RIGHT, INPUTEVENT_KEY_SHIFT_RIGHT,
	INPUTEVENT_KEY_CURSOR_UP, INPUTEVENT_KEY_CURSOR_DOWN, INPUTEVENT_KEY_CURSOR_LEFT, INPUTEVENT_KEY_CURSOR_RIGHT,
	INPUTEVENT_KEY_SPACE, INPUTEVENT_KEY_HELP, INPUTEVENT_KEY_DEL, INPUTEVENT_KEY_BACKSPACE, INPUTEVENT_KEY_RETURN,
	INPUTEVENT_KEY_A, INPUTEVENT_KEY_B, INPUTEVENT_KEY_C, INPUTEVENT_KEY_D,
	INPUTEVENT_KEY_E, INPUTEVENT_KEY_F, INPUTEVENT_KEY_G, INPUTEVENT_KEY_H,
	INPUTEVENT_KEY_I, INPUTEVENT_KEY_J, INPUTEVENT_KEY_K, INPUTEVENT_KEY_L,
	INPUTEVENT_KEY_M, INPUTEVENT_KEY_N, INPUTEVENT_KEY_O, INPUTEVENT_KEY_P,
	INPUTEVENT_KEY_Q, INPUTEVENT_KEY_R, INPUTEVENT_KEY_S, INPUTEVENT_KEY_T,
	INPUTEVENT_KEY_U, INPUTEVENT_KEY_V, INPUTEVENT_KEY_W, INPUTEVENT_KEY_X,
	INPUTEVENT_KEY_Y, INPUTEVENT_KEY_Z,
	INPUTEVENT_KEY_ENTER,
	INPUTEVENT_KEY_NP_0, INPUTEVENT_KEY_NP_1, INPUTEVENT_KEY_NP_2,
	INPUTEVENT_KEY_NP_3, INPUTEVENT_KEY_NP_4, INPUTEVENT_KEY_NP_5,
	INPUTEVENT_KEY_NP_6, INPUTEVENT_KEY_NP_7, INPUTEVENT_KEY_NP_8,
	INPUTEVENT_KEY_NP_9, INPUTEVENT_KEY_NP_PERIOD, INPUTEVENT_KEY_NP_ADD,
	INPUTEVENT_KEY_NP_SUB, INPUTEVENT_KEY_NP_MUL, INPUTEVENT_KEY_NP_DIV,
	INPUTEVENT_KEY_NP_LPAREN, INPUTEVENT_KEY_NP_RPAREN,
	INPUTEVENT_KEY_BACKQUOTE,
	INPUTEVENT_KEY_1, INPUTEVENT_KEY_2, INPUTEVENT_KEY_3, INPUTEVENT_KEY_4,
	INPUTEVENT_KEY_5, INPUTEVENT_KEY_6, INPUTEVENT_KEY_7, INPUTEVENT_KEY_8,
	INPUTEVENT_KEY_9, INPUTEVENT_KEY_0,
	INPUTEVENT_KEY_SUB, INPUTEVENT_KEY_EQUALS, INPUTEVENT_KEY_BACKSLASH,
	INPUTEVENT_KEY_LEFTBRACKET, INPUTEVENT_KEY_RIGHTBRACKET,
	INPUTEVENT_KEY_SEMICOLON, INPUTEVENT_KEY_SINGLEQUOTE,
	INPUTEVENT_KEY_COMMA, INPUTEVENT_KEY_PERIOD, INPUTEVENT_KEY_DIV,
	// Emulator and machine events
	INPUTEVENT_SPC_ENTERGUI, INPUTEVENT_SPC_QUIT, INPUTEVENT_SPC_PAUSE,
	INPUTEVENT_SPC_SOFTRESET, INPUTEVENT_SPC_HARDRESET, INPUTEVENT_SPC_FREEZEBUTTON,
	INPUTEVENT_SPC_STATESAVEDIALOG, INPUTEVENT_SPC_STATERESTOREDIALOG,
	INPUTEVENT_SPC_SWAPJOYPORTS,
	INPUTEVENT_SPC_MOUSEMAP_PORT0_LEFT, INPUTEVENT_SPC_MOUSEMAP_PORT0_RIGHT,
	INPUTEVENT_SPC_MOUSEMAP_PORT1_LEFT, INPUTEVENT_SPC_MOUSEMAP_PORT1_RIGHT,
	INPUTEVENT_SPC_MOUSE_SPEED_DOWN, INPUTEVENT_SPC_MOUSE_SPEED_UP,

};

const int RemapEventListSize = sizeof RemapEventList / sizeof RemapEventList[0];

//int RemapEventListSize = 20;

class MultiActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
	}
};

static MultiActionListener* multiActionListener;


class GroupActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optPort0)
		{
			SelectedPort = 0;
		}
		else if (actionEvent.getSource() == optPort1)
		{
			SelectedPort = 1;
		}
		else if (actionEvent.getSource() == optPort2)
		{
			SelectedPort = 2;
		}
		else if (actionEvent.getSource() == optPort3)
		{
			SelectedPort = 3;
		}

		else if (actionEvent.getSource() == optMultiNone)
		{
			SelectedFunction = 0;
		}
		else if (actionEvent.getSource() == optMultiSelect)
		{
			SelectedFunction = 1;
		}
		else if (actionEvent.getSource() == optMultiLeft)
		{
			SelectedFunction = 2;
		}
		else if (actionEvent.getSource() == optMultiRight)
		{
			SelectedFunction = 3;
		}

		// you'll want to refresh the drop-down section here

		// inputdevice_updateconfig(nullptr, &changed_prefs);
		RefreshPanelCustom();
	}
};

static GroupActionListener* grpActionListener;


class CustomActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource())
		{
			struct joypad_map_layout tempmap;

			// get map
			switch (SelectedFunction)
			{
			case 0:
				tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_none;
				break;
			case 1:
				tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_hotkey;
				break;
			case 2:
				tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_left_trigger;
				break;
			case 3:
				tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_right_trigger;
				break;
			default: 
				break;
			}

			int sel;
			//     
			//  get the selected action from the drop-down, and 
			//    push it into the 'temp map' 
			//     

			if (actionEvent.getSource() == cboCustomAction[0])
			{
				sel = cboCustomAction[0]->getSelected();
				tempmap.dpad_up_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[1])
			{
				sel = cboCustomAction[1]->getSelected();
				tempmap.dpad_down_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[2])
			{
				sel = cboCustomAction[2]->getSelected();
				tempmap.dpad_left_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[3])
			{
				sel = cboCustomAction[3]->getSelected();
				tempmap.dpad_right_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[4])
			{
				sel = cboCustomAction[4]->getSelected();
				tempmap.select_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[5])
			{
				sel = cboCustomAction[5]->getSelected();
				tempmap.left_shoulder_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[6])
			{
				sel = cboCustomAction[6]->getSelected();
				tempmap.lstick_select_action = RemapEventList[sel - 1];
			}

			else if (actionEvent.getSource() == cboCustomAction[7])
			{
				sel = cboCustomAction[7]->getSelected();
				tempmap.north_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[8])
			{
				sel = cboCustomAction[8]->getSelected();
				tempmap.south_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[9])
			{
				sel = cboCustomAction[9]->getSelected();
				tempmap.east_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[10])
			{
				sel = cboCustomAction[10]->getSelected();
				tempmap.west_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[11])
			{
				sel = cboCustomAction[11]->getSelected();
				tempmap.start_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[12])
			{
				sel = cboCustomAction[12]->getSelected();
				tempmap.right_shoulder_action = RemapEventList[sel - 1];
			}
			else if (actionEvent.getSource() == cboCustomAction[13])
			{
				sel = cboCustomAction[13]->getSelected();
				tempmap.rstick_select_action = RemapEventList[sel - 1];
			}


			// push map back into changed_pre
			switch (SelectedFunction)
			{
			case 0:
				changed_prefs.jports[SelectedPort].amiberry_custom_none = tempmap;
				break;
			case 1:
				changed_prefs.jports[SelectedPort].amiberry_custom_hotkey = tempmap;
				break;
			case 2:
				changed_prefs.jports[SelectedPort].amiberry_custom_left_trigger = tempmap;
				break;
			case 3:
				changed_prefs.jports[SelectedPort].amiberry_custom_right_trigger = tempmap;
				break;
			default: 
				break;
			}


			// and here, we will scroll through the custom-map and 
			// push it into the currprefs config file

			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelCustom();
		}
	}
};

static CustomActionListener* customActionListener;


void InitPanelCustom(const struct _ConfigCategory& category)
{
	int i;
	char tmp[255];
	static TCHAR* myname;

	//int num_elements = sizeof( RemapEventList ) / sizeof( RemapEventList[0] );

	if (CustomEventList.getNumberOfElements() == 0)
	{
		CustomEventList.AddElement("None");

		for (auto idx : RemapEventList)
		{
			const auto ie = inputdevice_get_eventinfo(idx);
			snprintf(tmp, 255, "%s", ie->name);
			CustomEventList.AddElement(tmp);
		}
	}

	CustomEventList_HotKey = CustomEventList;
	CustomEventList_Menu = CustomEventList;
	CustomEventList_Quit = CustomEventList;
	CustomEventList_Reset = CustomEventList;

	CustomEventList_HotKey.SwapFirstElement("None (HotKey)");
	CustomEventList_Menu.SwapFirstElement("None (Menu)");
	CustomEventList_Quit.SwapFirstElement("None (Quit)");
	CustomEventList_Reset.SwapFirstElement("None (Reset)");

	customActionListener = new CustomActionListener();
	grpActionListener = new GroupActionListener();

	optPort0 = new gcn::UaeRadioButton("0: Mouse", "radioportgroup");
	optPort0->addActionListener(grpActionListener);
	optPort1 = new gcn::UaeRadioButton("1: Joystick", "radioportgroup");
	optPort1->addActionListener(grpActionListener);
	optPort2 = new gcn::UaeRadioButton("2: Parallel 1", "radioportgroup");
	optPort2->addActionListener(grpActionListener);
	optPort3 = new gcn::UaeRadioButton("3: Parallel 2", "radioportgroup");
	optPort3->addActionListener(grpActionListener);

	optMultiNone = new gcn::UaeRadioButton("None", "radiomultigroup");
	optMultiNone->addActionListener(grpActionListener);
	optMultiSelect = new gcn::UaeRadioButton("HotKey", "radiomultigroup");
	optMultiSelect->addActionListener(grpActionListener);
	//	optMultiLeft = new gcn::UaeRadioButton("Left Trigger", "radiomultigroup");
	//	optMultiLeft->addActionListener(grpActionListener);
	//	optMultiRight = new gcn::UaeRadioButton("Right Trigger", "radiomultigroup");
	//	optMultiRight->addActionListener(grpActionListener);

	grpPort = new gcn::Window("Joystick Port");
	grpPort->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpPort->add(optPort0, 10, 5);
	grpPort->add(optPort1, 150, 5);
	grpPort->add(optPort2, 290, 5);
	grpPort->add(optPort3, 430, 5);
	grpPort->setSize(580, 50);
	grpPort->setBaseColor(gui_baseCol);

	category.panel->add(grpPort);

	grpFunction = new gcn::Window("Function Key");
	grpFunction->setPosition(DISTANCE_BORDER, 75);
	grpFunction->add(optMultiNone, 10, 5);
	grpFunction->add(optMultiSelect, 150, 5);
	//	grpFunction->add(optMultiLeft,   290, 5);
	//	grpFunction->add(optMultiRight,  430, 5);
	grpFunction->setSize(580, 50);
	grpFunction->setBaseColor(gui_baseCol);

	category.panel->add(grpFunction);

	// the input-device should be listed
	lblPortInput = new gcn::Label("Input Device:");
	lblPortInput->setAlignment(gcn::Graphics::RIGHT);

	txtPortInput = new gcn::TextField();
	txtPortInput->setEnabled(false);
	txtPortInput->setBaseColor(gui_baseCol);
	txtPortInput->setBackgroundColor(colTextboxBackground);

	lblRetroarch = new gcn::Label("[-]");
	lblRetroarch->setAlignment(gcn::Graphics::LEFT);

	txtPortInput->setSize(grpFunction->getWidth() - (lblPortInput->getWidth() + DISTANCE_NEXT_X * 2 + lblRetroarch->getWidth()), TEXTFIELD_HEIGHT);

	lblCustomAction[0] = new gcn::Label("D-Pad Up:");
	lblCustomAction[1] = new gcn::Label("D-Pad Down:");
	lblCustomAction[2] = new gcn::Label("D-Pad Left:");
	lblCustomAction[3] = new gcn::Label("D-Pad Right:");
	lblCustomAction[4] = new gcn::Label("Select:");
	lblCustomAction[5] = new gcn::Label("Left Shoulder:");
	lblCustomAction[6] = new gcn::Label("Left Analog:");

	lblCustomAction[7] = new gcn::Label("North:");
	lblCustomAction[8] = new gcn::Label("South:");
	lblCustomAction[9] = new gcn::Label("East:");
	lblCustomAction[10] = new gcn::Label("West:");
	lblCustomAction[11] = new gcn::Label("Start:");
	lblCustomAction[12] = new gcn::Label("Right Shoulder:");
	lblCustomAction[13] = new gcn::Label("Right Analog:");

	for (i = 0; i < 14; ++i)
	{
		lblCustomAction[i]->setSize(lblCustomAction[12]->getWidth(), lblCustomAction[12]->getHeight());
		lblCustomAction[i]->setAlignment(gcn::Graphics::RIGHT);

		cboCustomAction[i] = new gcn::UaeDropDown(&CustomEventList);
		cboCustomAction[i]->setSize(cboCustomAction[i]->getWidth(), DROPDOWN_HEIGHT);
		cboCustomAction[i]->setBaseColor(gui_baseCol);
		cboCustomAction[i]->setBackgroundColor(colTextboxBackground);

		snprintf(tmp, 20, "cboCustomAction%d", i);
		cboCustomAction[i]->setId(tmp);
		cboCustomAction[i]->addActionListener(customActionListener);
	}

	auto posY = 144 + 40;
	for (i = 0; i < 7; ++i)
	{
		category.panel->add(lblCustomAction[i], DISTANCE_BORDER, posY);
		category.panel->add(cboCustomAction[i], DISTANCE_BORDER + lblCustomAction[i]->getWidth() + 8, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}


	posY = 144 + 40;
	for (i = 7; i < 14; ++i)
	{
		category.panel->add(lblCustomAction[i], DISTANCE_BORDER + 290, posY);
		category.panel->add(cboCustomAction[i], DISTANCE_BORDER + lblCustomAction[i]->getWidth() + 290 + 8, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}


	category.panel->add(lblPortInput, DISTANCE_BORDER, 144);
	category.panel->add(txtPortInput, lblPortInput->getX() + lblPortInput->getWidth() + DISTANCE_NEXT_X, 144);
	category.panel->add(lblRetroarch, txtPortInput->getX() + txtPortInput->getWidth() + DISTANCE_NEXT_X, 144);

	// optMultiLeft->setEnabled(false);
	// optMultiRight->setEnabled(false);

	RefreshPanelCustom();
}


void ExitPanelCustom()
{
	delete optPort0;
	delete optPort1;
	delete optPort2;
	delete optPort3;
	delete grpPort;

	delete optMultiNone;
	delete optMultiSelect;
	delete optMultiLeft;
	delete optMultiRight;
	delete grpFunction;

	for (auto & i : lblCustomAction)
	{
		delete i;
		//        delete cboCustomAction[i];                   
	}


	delete lblPortInput;
	delete txtPortInput;
	delete lblRetroarch;

	delete grpActionListener;
	delete customActionListener;
}


void RefreshPanelCustom(void)
{
	optPort0->setSelected(SelectedPort == 0);
	optPort1->setSelected(SelectedPort == 1);
	optPort2->setSelected(SelectedPort == 2);
	optPort3->setSelected(SelectedPort == 3);

	optMultiNone->setSelected(SelectedFunction == 0);
	optMultiSelect->setSelected(SelectedFunction == 1);
	// optMultiLeft->setSelected(SelectedFunction == 2);
	//  optMultiRight->setSelected(SelectedFunction == 3);


	// you'll want to refresh the drop-down section here
	// get map
	struct joypad_map_layout tempmap;
	switch (SelectedFunction)
	{
	case 0:
		tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_none;
		break;
	case 1:
		tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_hotkey;
		break;
	case 2:
		tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_left_trigger;
		break;
	case 3:
		tempmap = changed_prefs.jports[SelectedPort].amiberry_custom_right_trigger;
		break;
	default: 
		break;
	}


	// update the joystick port  , and disable those which are not available
	char tmp[255];

	if (changed_prefs.jports[SelectedPort].id > JSEM_JOYS && changed_prefs.jports[SelectedPort].id < JSEM_MICE - 1)
	{
		const auto hostjoyid = changed_prefs.jports[SelectedPort].id - JSEM_JOYS - 1;
#ifdef USE_SDL1
		strncpy(tmp, SDL_JoystickName(hostjoyid), 255);
#elif USE_SDL2
		strncpy(tmp, SDL_JoystickNameForIndex(hostjoyid), 255);
#endif

		for (auto n = 0; n < 14; ++n)
		{
			auto temp_button = 0;

			switch (n)
			{
			case 0:
				{
					temp_button = host_input_buttons[hostjoyid].dpad_up;
					break;
				}
			case 1:
				{
					temp_button = host_input_buttons[hostjoyid].dpad_down;
					break;
				}
			case 2:
				{
					temp_button = host_input_buttons[hostjoyid].dpad_left;
					break;
				}
			case 3:
				{
					temp_button = host_input_buttons[hostjoyid].dpad_right;
					break;
				}
			case 4:
				{
					temp_button = host_input_buttons[hostjoyid].select_button;
					break;
				}
			case 5:
				{
					temp_button = host_input_buttons[hostjoyid].left_shoulder;
					break;
				}
			case 6:
				{
					temp_button = host_input_buttons[hostjoyid].lstick_button;
					break;
				}
			case 7:
				{
					temp_button = host_input_buttons[hostjoyid].north_button;
					break;
				}
			case 8:
				{
					temp_button = host_input_buttons[hostjoyid].south_button;
					break;
				}
			case 9:
				{
					temp_button = host_input_buttons[hostjoyid].east_button;
					break;
				}
			case 10:
				{
					temp_button = host_input_buttons[hostjoyid].west_button;
					break;
				}
			case 11:
				{
					temp_button = host_input_buttons[hostjoyid].start_button;
					break;
				}
			case 12:
				{
					temp_button = host_input_buttons[hostjoyid].right_shoulder;
					break;
				}
			case 13:
				{
					temp_button = host_input_buttons[hostjoyid].rstick_button;
					break;
				}
			default: 
				break;
			}

			// disable unmapped buttons
			cboCustomAction[n]->setEnabled((temp_button + 1) != 0);

			// set hotkey/quit/reset/menu on NONE field (and disable hotkey)
			if (temp_button == host_input_buttons[hostjoyid].hotkey_button && temp_button != -1)
			{
				cboCustomAction[n]->setListModel(&CustomEventList_HotKey);
				cboCustomAction[n]->setEnabled(false);
				cboCustomAction[n]->setSelected(0);
			}

			else if (temp_button == host_input_buttons[hostjoyid].quit_button && temp_button != -1 && SelectedFunction == 1 &&
				changed_prefs.use_retroarch_quit)
			{
				cboCustomAction[n]->setListModel(&CustomEventList_Quit);
				cboCustomAction[n]->setEnabled(false);
				cboCustomAction[n]->setSelected(0);
			}

			else if (temp_button == host_input_buttons[hostjoyid].menu_button && temp_button != -1 && SelectedFunction == 1 &&
				changed_prefs.use_retroarch_menu)
			{
				cboCustomAction[n]->setListModel(&CustomEventList_Menu);
				cboCustomAction[n]->setEnabled(false);
				cboCustomAction[n]->setSelected(0);
			}

			else if (temp_button == host_input_buttons[hostjoyid].reset_button && temp_button != -1 && SelectedFunction == 1 &&
				changed_prefs.use_retroarch_reset)
			{
				cboCustomAction[n]->setListModel(&CustomEventList_Reset);
				cboCustomAction[n]->setEnabled(false);
				cboCustomAction[n]->setSelected(0);
			}

			else
			{
				cboCustomAction[n]->setListModel(&CustomEventList);
			}
		}

		if (host_input_buttons[hostjoyid].number_of_hats > 0)
		{
			cboCustomAction[ 0]->setEnabled(true);
			cboCustomAction[ 1]->setEnabled(true);
			cboCustomAction[ 2]->setEnabled(true);
			cboCustomAction[ 3]->setEnabled(true);
		}

		if (host_input_buttons[hostjoyid].is_retroarch)
			lblRetroarch->setCaption("[R]");
		else
			lblRetroarch->setCaption("[N]");
	}

	else
	{
		_stprintf(tmp, _T("%s"), "Not a valid Input Controller for Joystick Emulation.");
		for (auto & n : cboCustomAction)
		{
			n->setListModel(&CustomEventList);
			n->setEnabled(false);
		}

		lblRetroarch->setCaption("[_]");
	}

	txtPortInput->setText(tmp);


	// now select which items in drop-down are 'done'
	auto eventnum = 0;

	for (auto z = 0; z < 14; ++z)
	{
		switch (z)
		{
		case 0:
			{
				eventnum = tempmap.dpad_up_action;
				break;
			}
		case 1:
			{
				eventnum = tempmap.dpad_down_action;
				break;
			}
		case 2:
			{
				eventnum = tempmap.dpad_left_action;
				break;
			}
		case 3:
			{
				eventnum = tempmap.dpad_right_action;
				break;
			}
		case 4:
			{
				eventnum = tempmap.select_action;
				break;
			}
		case 5:
			{
				eventnum = tempmap.left_shoulder_action;
				break;
			}
		case 6:
			{
				eventnum = tempmap.lstick_select_action;
				break;
			}
		case 7:
			{
				eventnum = tempmap.north_action;
				break;
			}
		case 8:
			{
				eventnum = tempmap.south_action;
				break;
			}
		case 9:
			{
				eventnum = tempmap.east_action;
				break;
			}
		case 10:
			{
				eventnum = tempmap.west_action;
				break;
			}
		case 11:
			{
				eventnum = tempmap.start_action;
				break;
			}
		case 12:
			{
				eventnum = tempmap.right_shoulder_action;
				break;
			}
		case 13:
			{
				eventnum = tempmap.rstick_select_action;
				break;
			}
		default: 
			break;
		}
		const auto x = find_in_array(RemapEventList, RemapEventListSize, eventnum);
		if (cboCustomAction[z]->getEnabled())
		{
			cboCustomAction[z]->setSelected(x + 1);
		}
	}
}


bool HelpPanelCustom(vector<string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Set up Custom input actions for each Amiga port, such as Keyboard remapping,");
	helptext.emplace_back("or emulator functions.");
	helptext.emplace_back("");
	helptext.emplace_back("Select the port which you wish to re-map with 'Joystick Port'.");
	helptext.emplace_back("The currently selected Input Device will then be displayed under 'Input Device'.");
	helptext.emplace_back("");
	helptext.emplace_back("Buttons which are not available on this device (detected with RetroArch ");
	helptext.emplace_back("configuration files) are unavailable to remap.");
	helptext.emplace_back("");
	helptext.emplace_back("The HotKey button (used for secondary functions) is also unavailable for custom options. ");
	helptext.emplace_back("The actions performed by pressing the HotKey with other buttons can also be remapped.");
	helptext.emplace_back("Pre-defined functions such as Quit/Reset/Menu will be displayed as the 'default' option.");
	helptext.emplace_back("");
	helptext.emplace_back("The Function of the individual buttons are selectable via the marked drop-down boxes.");
	return true;
}
