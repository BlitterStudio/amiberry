#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "inputdevice.h"
#include "amiberry_input.h"

enum
{
	MAX_PORTSUBMODES = 16
};

static int portsubmodes[MAX_PORTSUBMODES];
static int retroarch_kb = 0;
static int joysticks = 0;
static int mice = 0;

static const int mousespeed_values[] = { 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 75, 100, 125, 150 };
static const int digital_joymousespeed_values[] = { 2, 5, 10, 15, 20 };
static const int analog_joymousespeed_values[] = { 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 75, 100, 125, 150 };

static gcn::Label* lblPort0;
static gcn::DropDown* cboPort0;
static gcn::Label* lblPort1;
static gcn::DropDown* cboPort1;

static gcn::DropDown* cboPort0Autofire;
static gcn::DropDown* cboPort1Autofire;
static gcn::DropDown* cboPort2Autofire;
static gcn::DropDown* cboPort3Autofire;

static gcn::DropDown* cboPort0mode;
static gcn::DropDown* cboPort1mode;
static gcn::Button* cmdSwapPorts;

static gcn::Label* lblEmulatedParallelPort;

static gcn::Label* lblPort0mousemode;
static gcn::DropDown* cboPort0mousemode;
static gcn::Label* lblPort1mousemode;
static gcn::DropDown* cboPort1mousemode;

static gcn::Label* lblPort2;
static gcn::DropDown* cboPort2;
static gcn::Label* lblPort3;
static gcn::DropDown* cboPort3;

static gcn::Label* lblAutofireRate;
static gcn::DropDown* cboAutofireRate;

static gcn::Label* lblDigitalJoyMouseSpeed;
static gcn::Label* lblDigitalJoyMouseSpeedInfo;
static gcn::Slider* sldDigitalJoyMouseSpeed;
static gcn::Label* lblAnalogJoyMouseSpeed;
static gcn::Label* lblAnalogJoyMouseSpeedInfo;
static gcn::Slider* sldAnalogJoyMouseSpeed;
static gcn::Label* lblMouseSpeed;
static gcn::Label* lblMouseSpeedInfo;
static gcn::Slider* sldMouseSpeed;

static gcn::CheckBox* chkMouseHack;
static gcn::CheckBox* chkMagicMouseUntrap;
static gcn::CheckBox* chkInputAutoswitch;

static gcn::RadioButton* optBoth;
static gcn::RadioButton* optNative;
static gcn::RadioButton* optHost;

static gcn::DropDown* joys[] = { cboPort0, cboPort1, cboPort2, cboPort3 };
static gcn::DropDown* joysm[] = { cboPort0mode, cboPort1mode, nullptr, nullptr };
static gcn::DropDown* joysaf[] = { cboPort0Autofire, cboPort1Autofire, cboPort2Autofire, cboPort3Autofire };
static gcn::DropDown* joysmm[] = { cboPort0mousemode, cboPort1mousemode, nullptr, nullptr };

static gcn::Button* cmdRemap0;
static gcn::Button* cmdRemap1;

static gcn::CheckBox* chkSwapBackslashF11;
static gcn::CheckBox* chkSwapEndPgUp;

static gcn::StringListModel ctrlPortList;
static int portListIDs[MAX_INPUT_DEVICES + JSEM_LASTKBD + 1];

static const std::vector<std::string> autoFireValues = { "No autofire (normal)", "Autofire", "Autofire (toggle)", "Autofire (always)", "No autofire (toggle)"};
static gcn::StringListModel autoFireList(autoFireValues);

static const std::vector<std::string> autoFireRateValues = { "Off", "Slow", "Medium", "Fast" };
static gcn::StringListModel autoFireRateList(autoFireRateValues);

static const std::vector<std::string> mousemapValues = { "None", "LStick" };
static gcn::StringListModel ctrlPortMouseModeList(mousemapValues);

static const std::vector<std::string> joyportmodes = { "Default", "Wheel Mouse", "Mouse", "Joystick", "Gamepad", "Analog Joystick", "CDTV remote mouse", "CD32 pad"};
static gcn::StringListModel ctrlPortModeList(joyportmodes);

void update_joystick_list()
{
	retroarch_kb = get_retroarch_kb_num();
	joysticks = inputdevice_get_device_total(IDTYPE_JOYSTICK);
	mice = inputdevice_get_device_total(IDTYPE_MOUSE);

	auto idx = 0;
	ctrlPortList.add("<none>");
	portListIDs[idx] = JPORT_NONE;

	idx++;
	ctrlPortList.add("Keyboard Layout A (Numpad, 0/5=Fire, Decimal/DEL=2nd Fire)");
	portListIDs[idx] = JSEM_KBDLAYOUT;

	idx++;
	ctrlPortList.add("Keyboard Layout B (Cursor, RCtrl/RAlt=Fire, RShift=2nd Fire)");
	portListIDs[idx] = JSEM_KBDLAYOUT + 1;

	idx++;
	ctrlPortList.add("Keyboard Layout C (WSAD, LAlt=Fire, LShift=2nd Fire)");
	portListIDs[idx] = JSEM_KBDLAYOUT + 2;

	idx++;
	ctrlPortList.add("Keyrah Layout (Cursor, Space/RAlt=Fire, RShift=2nd Fire)");
	portListIDs[idx] = JSEM_KBDLAYOUT + 3;

	for (auto j = 0; j < 4; j++)
	{
		auto element = "Retroarch KBD as Joystick Player" + std::to_string(j + 1);
		ctrlPortList.add(element);
		idx++;
		portListIDs[idx] = JSEM_KBDLAYOUT + j + 4;
	}

	for (auto j = 0; j < joysticks; j++)
	{
		ctrlPortList.add(inputdevice_get_device_name(IDTYPE_JOYSTICK, j));
		idx++;
		portListIDs[idx] = JSEM_JOYS + j;
	}

	for (auto j = 0; j < mice; j++)
	{
		ctrlPortList.add(inputdevice_get_device_name(IDTYPE_MOUSE, j));
		idx++;
		portListIDs[idx] = JSEM_MICE + j;
	}
}

class InputPortsActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		auto changed = 0;
		auto changedport = -1;
		for (auto i = 0; i < MAX_JPORTS; i++)
		{
			if (actionEvent.getSource() == joys[i]
				|| actionEvent.getSource() == joysm[i])
			{
				changedport = i;
				inputdevice_compa_clear(&changed_prefs, changedport);
			}

			auto* port = &changed_prefs.jports[i].id;
			auto* portm = &changed_prefs.jports[i].mode;
			auto* portsm = &changed_prefs.jports[i].submode;
			auto prevport = *port;
			auto* id = joys[i];
			auto* idm = joysm[i];

			auto v = id->getSelected();
			if (v >= 0)
			{
				auto max = JSEM_LASTKBD + joysticks;
				if (i < 2)
					max += mice;
				v -= 1;
				if (v < 0 || v >= max + MAX_JPORTS_CUSTOM) {
					*port = JPORT_NONE;
				}
				else if (v >= max) {
					*port = JSEM_CUSTOM + v - max;
				}
				else if (v < JSEM_LASTKBD) {
					*port = JSEM_KBDLAYOUT + (int)v;
				}
				else if (v >= JSEM_LASTKBD + joysticks) {
					*port = JSEM_MICE + (int)v - joysticks - JSEM_LASTKBD;
				}
				else {
					*port = JSEM_JOYS + (int)v - JSEM_LASTKBD;
				}
			}
			if (idm != nullptr) {
				v = idm->getSelected();
				if (v >= 0)
				{
					auto vcnt = 0;
					*portsm = 0;
					for (auto portsubmode : portsubmodes)
					{
						if (v <= 0)
							break;
						if (portsubmode > 0) {
							if (v <= portsubmode) {
								*portsm = v;
							}
							v -= portsubmode;
						}
						else {
							v--;
							vcnt++;
						}
					}
					*portm = vcnt;
				}
			}
			if (joysaf[i] != nullptr) {
				const auto af = joysaf[i]->getSelected();
				changed_prefs.jports[i].autofire = af;
			}
			if (i < 2 && joysmm[i] != nullptr)
			{
				v = joysmm[i]->getSelected();
				changed_prefs.jports[i].mousemap = v;
			}
			if (*port != prevport)
				changed = 1;
		}
		if (changed)
			inputdevice_validate_jports(&changed_prefs, changedport, nullptr);

		inputdevice_updateconfig(nullptr, &changed_prefs);
		inputdevice_config_change();
		RefreshPanelInput();
		RefreshPanelCustom();
	}
};

static InputPortsActionListener* inputPortsActionListener;

class InputActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cboAutofireRate)
		{
			if (cboAutofireRate->getSelected() == 0)
				changed_prefs.input_autofire_linecnt = 0;
			else if (cboAutofireRate->getSelected() == 1)
				changed_prefs.input_autofire_linecnt = 12 * 312;
			else if (cboAutofireRate->getSelected() == 2)
				changed_prefs.input_autofire_linecnt = 8 * 312;
			else
				changed_prefs.input_autofire_linecnt = 4 * 312;
		}

		else if (actionEvent.getSource() == sldDigitalJoyMouseSpeed)
			changed_prefs.input_joymouse_speed = digital_joymousespeed_values[static_cast<int>(sldDigitalJoyMouseSpeed->getValue())];

		else if (actionEvent.getSource() == sldAnalogJoyMouseSpeed)
			changed_prefs.input_joymouse_multiplier = analog_joymousespeed_values[static_cast<int>(sldAnalogJoyMouseSpeed->getValue())];

		else if (actionEvent.getSource() == sldMouseSpeed)
			changed_prefs.input_mouse_speed = mousespeed_values[static_cast<int>(sldMouseSpeed->getValue())];

		else if (actionEvent.getSource() == chkMouseHack)
			currprefs.input_tablet = changed_prefs.input_tablet = chkMouseHack->isSelected() ? TABLET_MOUSEHACK : TABLET_OFF;

		else if (actionEvent.getSource() == chkMagicMouseUntrap)
		{
			if (chkMagicMouseUntrap->isSelected())
			{
				currprefs.input_mouse_untrap = changed_prefs.input_mouse_untrap |= MOUSEUNTRAP_MAGIC;
			}
			else
			{
				currprefs.input_mouse_untrap = changed_prefs.input_mouse_untrap &= ~MOUSEUNTRAP_MAGIC;
			}
		}

		else if (actionEvent.getSource() == optBoth)
			currprefs.input_magic_mouse_cursor = changed_prefs.input_magic_mouse_cursor = MAGICMOUSE_BOTH;
		else if (actionEvent.getSource() == optNative)
			currprefs.input_magic_mouse_cursor = changed_prefs.input_magic_mouse_cursor = MAGICMOUSE_NATIVE_ONLY;
		else if (actionEvent.getSource() == optHost)
			currprefs.input_magic_mouse_cursor = changed_prefs.input_magic_mouse_cursor = MAGICMOUSE_HOST_ONLY;
		
		else if (actionEvent.getSource() == chkInputAutoswitch)
			changed_prefs.input_autoswitch = chkInputAutoswitch->isSelected();

		else if (actionEvent.getSource() == cmdSwapPorts)
		{
			inputdevice_swap_compa_ports(&changed_prefs, 0);
			RefreshPanelInput();
			inputdevice_forget_unplugged_device(0);
			inputdevice_forget_unplugged_device(1);
		}

		else if (actionEvent.getSource() == cmdRemap0)
		{
			const auto host_joy_id = changed_prefs.jports[0].id - JSEM_JOYS;
			const auto mapping = show_controller_map(host_joy_id, false);
			if (!mapping.empty())
			{
				SDL_GameControllerAddMapping(mapping.c_str());
				save_mapping_to_file(mapping);
			}
		}

		else if (actionEvent.getSource() == cmdRemap1)
		{
			const auto host_joy_id = changed_prefs.jports[1].id - JSEM_JOYS;
			const auto mapping = show_controller_map(host_joy_id, false);
			if (!mapping.empty())
			{
				SDL_GameControllerAddMapping(mapping.c_str());
				save_mapping_to_file(mapping);
			}
		}

		else if (actionEvent.getSource() == chkSwapBackslashF11)
		{
			key_swap_hack = chkSwapBackslashF11->isSelected();
			regsetint(NULL, _T("KeySwapBackslashF11"), key_swap_hack);
		}

		else if (actionEvent.getSource() == chkSwapEndPgUp)
		{
			key_swap_end_pgup = chkSwapEndPgUp->isSelected();
			regsetint(NULL, _T("KeyEndPageUp"), key_swap_end_pgup);
		}
		
		RefreshPanelInput();
		RefreshPanelRTG(); // needed to enable the Hardware RTG sprite
	}
};

static InputActionListener* inputActionListener;

void InitPanelInput(const config_category& category)
{
	if (ctrlPortList.getNumberOfElements() == 0)
	{
		update_joystick_list();
	}

	inputPortsActionListener = new InputPortsActionListener();
	inputActionListener = new InputActionListener();
	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - 60;

	lblPort0 = new gcn::Label("Port 0:");
	lblPort0->setAlignment(gcn::Graphics::Right);

	lblPort1 = new gcn::Label("Port 1:");
	lblPort1->setAlignment(gcn::Graphics::Right);

	lblPort2 = new gcn::Label("Port 2:");
	lblPort2->setAlignment(gcn::Graphics::Left);

	lblPort3 = new gcn::Label("Port 3:");
	lblPort3->setAlignment(gcn::Graphics::Left);
	
	for (auto i = 0; i < MAX_JPORTS; i++)
	{
		joys[i] = new gcn::DropDown(&ctrlPortList);
		joys[i]->setSize(textFieldWidth, joys[i]->getHeight());
		joys[i]->setBaseColor(gui_base_color);
		joys[i]->setBackgroundColor(gui_background_color);
		joys[i]->setForegroundColor(gui_foreground_color);
		joys[i]->setSelectionColor(gui_selection_color);
		joys[i]->addActionListener(inputPortsActionListener);

		joysaf[i] = new gcn::DropDown(&autoFireList);
		joysaf[i]->setSize(200, joysaf[i]->getHeight());
		joysaf[i]->setBaseColor(gui_base_color);
		joysaf[i]->setBackgroundColor(gui_background_color);
		joysaf[i]->setForegroundColor(gui_foreground_color);
		joysaf[i]->setSelectionColor(gui_selection_color);
		joysaf[i]->addActionListener(inputPortsActionListener);

		if (i < 2)
		{
			joysm[i] = new gcn::DropDown(&ctrlPortModeList);
			joysm[i]->setSize(150, joysm[i]->getHeight());
			joysm[i]->setBaseColor(gui_base_color);
			joysm[i]->setBackgroundColor(gui_background_color);
			joysm[i]->setForegroundColor(gui_foreground_color);
			joysm[i]->setSelectionColor(gui_selection_color);
			joysm[i]->addActionListener(inputPortsActionListener);

			joysmm[i] = new gcn::DropDown(&ctrlPortMouseModeList);
			joysmm[i]->setSize(95, joysmm[i]->getHeight());
			joysmm[i]->setBaseColor(gui_base_color);
			joysmm[i]->setBackgroundColor(gui_background_color);
			joysmm[i]->setForegroundColor(gui_foreground_color);
			joysmm[i]->setSelectionColor(gui_selection_color);
			joysmm[i]->addActionListener(inputPortsActionListener);
		}
		
		switch (i)
		{
		case 0: 
			joys[i]->setId("cboPort0");
			joysaf[i]->setId("cboPort0Autofire");
			joysm[i]->setId("cboPort0mode");
			joysmm[i]->setId("cboPort0mousemode");
			break;
		case 1: 
			joys[i]->setId("cboPort1");
			joysaf[i]->setId("cboPort1Autofire");
			joysm[i]->setId("cboPort1mode");
			joysmm[i]->setId("cboPort1mousemode");
			break;
		case 2: 
			joys[i]->setId("cboPort2");
			joysaf[i]->setId("cboPort2Autofire");
			break;
		case 3: 
			joys[i]->setId("cboPort3");
			joysaf[i]->setId("cboPort3Autofire");
			break;
		default: 
			break;
		}
	}

	cmdRemap0 = new gcn::Button("Remap");
	cmdRemap0->setId("cmdRemap0");
	cmdRemap0->setSize(BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdRemap0->setBaseColor(gui_base_color);
	cmdRemap0->setForegroundColor(gui_foreground_color);
	cmdRemap0->addActionListener(inputActionListener);

	cmdRemap1 = new gcn::Button("Remap");
	cmdRemap1->setId("cmdRemap1");
	cmdRemap1->setSize(BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdRemap1->setBaseColor(gui_base_color);
	cmdRemap1->setForegroundColor(gui_foreground_color);
	cmdRemap1->addActionListener(inputActionListener);
	
	cmdSwapPorts = new gcn::Button("Swap ports");
	cmdSwapPorts->setId("cmdSwapPorts");
	cmdSwapPorts->setSize(150, BUTTON_HEIGHT);
	cmdSwapPorts->setBaseColor(gui_base_color);
	cmdSwapPorts->setForegroundColor(gui_foreground_color);
	cmdSwapPorts->addActionListener(inputActionListener);

	lblEmulatedParallelPort = new gcn::Label("Emulated Parallel Port joystick adapter");

	lblPort0mousemode = new gcn::Label("Mouse Map Port0:");
	lblPort0mousemode->setAlignment(gcn::Graphics::Right);

	lblPort1mousemode = new gcn::Label("Mouse Map Port1:");
	lblPort1mousemode->setAlignment(gcn::Graphics::Right);

	lblAutofireRate = new gcn::Label("Autofire Rate:");
	lblAutofireRate->setAlignment(gcn::Graphics::Right);
	cboAutofireRate = new gcn::DropDown(&autoFireRateList);
	cboAutofireRate->setSize(95, cboAutofireRate->getHeight());
	cboAutofireRate->setBaseColor(gui_base_color);
	cboAutofireRate->setBackgroundColor(gui_background_color);
	cboAutofireRate->setForegroundColor(gui_foreground_color);
	cboAutofireRate->setSelectionColor(gui_selection_color);
	cboAutofireRate->setId("cboAutofireRate");
	cboAutofireRate->addActionListener(inputActionListener);

	lblDigitalJoyMouseSpeed = new gcn::Label("Digital joy-mouse speed:");
	lblDigitalJoyMouseSpeed->setAlignment(gcn::Graphics::Right);
	lblDigitalJoyMouseSpeedInfo = new gcn::Label("100");
	sldDigitalJoyMouseSpeed = new gcn::Slider(0, 4);
	sldDigitalJoyMouseSpeed->setSize(100, SLIDER_HEIGHT);
	sldDigitalJoyMouseSpeed->setBaseColor(gui_base_color);
	sldDigitalJoyMouseSpeed->setBackgroundColor(gui_background_color);
	sldDigitalJoyMouseSpeed->setForegroundColor(gui_foreground_color);
	sldDigitalJoyMouseSpeed->setMarkerLength(20);
	sldDigitalJoyMouseSpeed->setStepLength(1);
	sldDigitalJoyMouseSpeed->setId("sldDigitalJoyMouseSpeed");
	sldDigitalJoyMouseSpeed->addActionListener(inputActionListener);

	lblAnalogJoyMouseSpeed = new gcn::Label("Analog joy-mouse speed:");
	lblAnalogJoyMouseSpeed->setAlignment(gcn::Graphics::Right);
	lblAnalogJoyMouseSpeedInfo = new gcn::Label("100");
	sldAnalogJoyMouseSpeed = new gcn::Slider(0, 13);
	sldAnalogJoyMouseSpeed->setSize(100, SLIDER_HEIGHT);
	sldAnalogJoyMouseSpeed->setBaseColor(gui_base_color);
	sldAnalogJoyMouseSpeed->setBackgroundColor(gui_background_color);
	sldAnalogJoyMouseSpeed->setForegroundColor(gui_foreground_color);
	sldAnalogJoyMouseSpeed->setMarkerLength(20);
	sldAnalogJoyMouseSpeed->setStepLength(1);
	sldAnalogJoyMouseSpeed->setId("sldAnalogJoyMouseSpeed");
	sldAnalogJoyMouseSpeed->addActionListener(inputActionListener);

	lblMouseSpeed = new gcn::Label("Mouse speed:");
	lblMouseSpeed->setAlignment(gcn::Graphics::Right);
	lblMouseSpeedInfo = new gcn::Label("100");
	sldMouseSpeed = new gcn::Slider(0, 13);
	sldMouseSpeed->setSize(100, SLIDER_HEIGHT);
	sldMouseSpeed->setBaseColor(gui_base_color);
	sldMouseSpeed->setBackgroundColor(gui_background_color);
	sldMouseSpeed->setForegroundColor(gui_foreground_color);
	sldMouseSpeed->setMarkerLength(20);
	sldMouseSpeed->setStepLength(1);
	sldMouseSpeed->setId("sldMouseSpeed");
	sldMouseSpeed->addActionListener(inputActionListener);
	
	chkMouseHack = new gcn::CheckBox("Virtual mouse driver");
	chkMouseHack->setId("chkMouseHack");
	chkMouseHack->setBaseColor(gui_base_color);
	chkMouseHack->setBackgroundColor(gui_background_color);
	chkMouseHack->setForegroundColor(gui_foreground_color);
	chkMouseHack->addActionListener(inputActionListener);

	chkMagicMouseUntrap = new gcn::CheckBox("Magic Mouse untrap");
	chkMagicMouseUntrap->setId("chkMagicMouseUntrap");
	chkMagicMouseUntrap->setBaseColor(gui_base_color);
	chkMagicMouseUntrap->setBackgroundColor(gui_background_color);
	chkMagicMouseUntrap->setForegroundColor(gui_foreground_color);
	chkMagicMouseUntrap->addActionListener(inputActionListener);

	optBoth = new gcn::RadioButton("Both", "radioCursorGroup");
	optBoth->setId("optBoth");
	optBoth->setBaseColor(gui_base_color);
	optBoth->setBackgroundColor(gui_background_color);
	optBoth->setForegroundColor(gui_foreground_color);
	optBoth->addActionListener(inputActionListener);
	optNative = new gcn::RadioButton("Native only", "radioCursorGroup");
	optNative->setId("optNative");
	optNative->setBaseColor(gui_base_color);
	optNative->setBackgroundColor(gui_background_color);
	optNative->setForegroundColor(gui_foreground_color);
	optNative->addActionListener(inputActionListener);
	optHost = new gcn::RadioButton("Host only", "radioCursorGroup");
	optHost->setId("optHost");
	optHost->setBaseColor(gui_base_color);
	optHost->setBackgroundColor(gui_background_color);
	optHost->setForegroundColor(gui_foreground_color);
	optHost->addActionListener(inputActionListener);
	
	chkInputAutoswitch = new gcn::CheckBox("Mouse/Joystick autoswitching");
	chkInputAutoswitch->setId("chkInputAutoswitch");
	chkInputAutoswitch->setBaseColor(gui_base_color);
	chkInputAutoswitch->setBackgroundColor(gui_background_color);
	chkInputAutoswitch->setForegroundColor(gui_foreground_color);
	chkInputAutoswitch->addActionListener(inputActionListener);

	chkSwapBackslashF11 = new gcn::CheckBox("Swap Backslash/F11");
	chkSwapBackslashF11->setId("chkSwapBackslashF11");
	chkSwapBackslashF11->setBaseColor(gui_base_color);
	chkSwapBackslashF11->setBackgroundColor(gui_background_color);
	chkSwapBackslashF11->setForegroundColor(gui_foreground_color);
	chkSwapBackslashF11->addActionListener(inputActionListener);

	chkSwapEndPgUp = new gcn::CheckBox("Page Up = End");
	chkSwapEndPgUp->setId("chkSwapEndPgUp");
	chkSwapEndPgUp->setBaseColor(gui_base_color);
	chkSwapEndPgUp->setBackgroundColor(gui_background_color);
	chkSwapEndPgUp->setForegroundColor(gui_foreground_color);
	chkSwapEndPgUp->addActionListener(inputActionListener);

	int posY = DISTANCE_BORDER;
	category.panel->add(lblPort0, DISTANCE_BORDER, posY);
	category.panel->add(joys[0], DISTANCE_BORDER + lblPort0->getWidth() + 8, posY);
	posY += joys[0]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(joysaf[0], joys[0]->getX(), posY);
	category.panel->add(joysm[0], joysaf[0]->getX() + joysaf[0]->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cmdRemap0, joysm[0]->getX() + joysm[0]->getWidth() + DISTANCE_NEXT_X, posY);
	posY += joysaf[0]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort1, DISTANCE_BORDER, posY);
	category.panel->add(joys[1], DISTANCE_BORDER + lblPort1->getWidth() + 8, posY);
	posY += joys[1]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(joysaf[1], joys[1]->getX(), posY);
	category.panel->add(joysm[1], joysaf[1]->getX() + joysaf[1]->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cmdRemap1, joysm[1]->getX() + joysm[1]->getWidth() + DISTANCE_NEXT_X, posY);
	posY += joysaf[1]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cmdSwapPorts, joysaf[1]->getX(), posY);
	category.panel->add(chkInputAutoswitch, cmdSwapPorts->getX() + cmdSwapPorts->getWidth() + DISTANCE_NEXT_X, posY + BUTTON_HEIGHT/4);
	posY += chkInputAutoswitch->getHeight() + DISTANCE_NEXT_Y * 3;

	category.panel->add(lblEmulatedParallelPort, DISTANCE_BORDER, posY);
	posY += lblEmulatedParallelPort->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort2, DISTANCE_BORDER, posY);
	category.panel->add(joys[2], DISTANCE_BORDER + lblPort2->getWidth() + 8, posY);
	posY += joys[2]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(joysaf[2], joys[2]->getX(), posY);
	posY += joysaf[2]->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(lblPort3, DISTANCE_BORDER, posY);
	category.panel->add(joys[3], DISTANCE_BORDER + lblPort3->getWidth() + 8, posY);
	posY += joys[3]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(joysaf[3], joys[3]->getX(), posY);
	posY += joysaf[3]->getHeight() + DISTANCE_NEXT_Y * 2;
	
	category.panel->add(lblPort0mousemode, DISTANCE_BORDER, posY);
	category.panel->add(joysmm[0], lblPort0mousemode->getX() + lblPort0mousemode->getWidth() + 8, posY);

	category.panel->add(lblDigitalJoyMouseSpeed, joysmm[0]->getX() + joysmm[0]->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(sldDigitalJoyMouseSpeed, lblDigitalJoyMouseSpeed->getX() + lblDigitalJoyMouseSpeed->getWidth() + 8, posY);
	category.panel->add(lblDigitalJoyMouseSpeedInfo, sldDigitalJoyMouseSpeed->getX() + sldDigitalJoyMouseSpeed->getWidth() + 8, posY);
	posY += lblDigitalJoyMouseSpeed->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort1mousemode, DISTANCE_BORDER, posY);
	category.panel->add(joysmm[1], lblPort1mousemode->getX() + lblPort1mousemode->getWidth() + 8, posY);

	category.panel->add(lblAnalogJoyMouseSpeed, joysmm[1]->getX() + joysmm[1]->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(sldAnalogJoyMouseSpeed, sldDigitalJoyMouseSpeed->getX(), posY);
	category.panel->add(lblAnalogJoyMouseSpeedInfo, sldAnalogJoyMouseSpeed->getX() + sldAnalogJoyMouseSpeed->getWidth() + 8, posY);
	posY += lblAnalogJoyMouseSpeed->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblAutofireRate, DISTANCE_BORDER, posY);
	category.panel->add(cboAutofireRate, DISTANCE_BORDER + lblAutofireRate->getWidth() + 8, posY);

	category.panel->add(lblMouseSpeed, joysmm[1]->getX() + joysmm[1]->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(sldMouseSpeed, sldAnalogJoyMouseSpeed->getX(), posY);
	category.panel->add(lblMouseSpeedInfo, sldMouseSpeed->getX() + sldMouseSpeed->getWidth() + 8, posY);
	posY += lblMouseSpeed->getHeight() + DISTANCE_NEXT_Y * 2;
	
	category.panel->add(chkMouseHack, DISTANCE_BORDER, posY);
	category.panel->add(chkMagicMouseUntrap, lblMouseSpeed->getX(), posY);
	posY += chkMouseHack->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(optBoth, DISTANCE_BORDER, posY);
	category.panel->add(optNative, optBoth->getX() + optBoth->getWidth() + 8, posY);
	category.panel->add(optHost, optNative->getX() + optNative->getWidth() + 8, posY);
	posY += optBoth->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkSwapBackslashF11, DISTANCE_BORDER, posY);
	posY += chkSwapBackslashF11->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkSwapEndPgUp, chkSwapBackslashF11->getX(), posY);
	
	for (auto& portsubmode : portsubmodes)
	{
		portsubmode = 0;
	}
	portsubmodes[8] = 2;
	portsubmodes[9] = -1;

	inputdevice_updateconfig(nullptr, &changed_prefs);
	RefreshPanelInput();
}

void ExitPanelInput()
{
	for (auto& joy : joys)
		delete joy;

	for (auto& joyaf : joysaf)
		delete joyaf;

	for (auto& joym : joysm)
		delete joym;

	for (auto& joymm : joysmm)
		delete joymm;
	
	delete lblPort0;
	delete lblPort1;
	delete lblPort2;
	delete lblPort3;

	delete lblPort0mousemode;
	delete lblPort1mousemode;
	delete cmdSwapPorts;

	delete lblEmulatedParallelPort;

	delete lblAutofireRate;
	delete cboAutofireRate;
	
	delete lblDigitalJoyMouseSpeed;
	delete sldDigitalJoyMouseSpeed;
	delete lblDigitalJoyMouseSpeedInfo;
	delete lblAnalogJoyMouseSpeed;
	delete sldAnalogJoyMouseSpeed;
	delete lblAnalogJoyMouseSpeedInfo;
	delete lblMouseSpeed;
	delete sldMouseSpeed;
	delete lblMouseSpeedInfo;

	delete chkMouseHack;
	delete chkMagicMouseUntrap;
	delete chkInputAutoswitch;

	delete inputPortsActionListener;
	delete inputActionListener;

	delete optBoth;
	delete optNative;
	delete optHost;

	delete cmdRemap0;
	delete cmdRemap1;
	delete chkSwapBackslashF11;
	delete chkSwapEndPgUp;
}

void RefreshPanelInput()
{
	if (joystick_refresh_needed)
	{
		ctrlPortList.clear();
		update_joystick_list();
	}

	for (auto i = 0; i < MAX_JPORTS; i++) 
	{
		const auto* id = joys[i];
		const auto* idm = joysm[i];
		const auto* iaf = joysaf[i];
		const auto v = changed_prefs.jports[i].id;
		const auto vm = changed_prefs.jports[i].mode + changed_prefs.jports[i].submode;
		const auto vaf = changed_prefs.jports[i].autofire;

		if (idm != nullptr)
			idm->setSelected(vm);

		if (iaf != nullptr)
			iaf->setSelected(vaf);
		
		for (auto j = 0; j < ctrlPortList.getNumberOfElements(); ++j)
		{
			if (v == portListIDs[j])
				id->setSelected(j);
		}
	}

	if (changed_prefs.jports[0].id >= JSEM_JOYS && changed_prefs.jports[0].id < JSEM_MICE - 1)
		cmdRemap0->setEnabled(true);
	else
		cmdRemap0->setEnabled(false);

	if (changed_prefs.jports[1].id >= JSEM_JOYS && changed_prefs.jports[1].id < JSEM_MICE - 1)
		cmdRemap1->setEnabled(true);
	else
		cmdRemap1->setEnabled(false);
	
	if (changed_prefs.input_autofire_linecnt == 0)
		cboAutofireRate->setSelected(0);
	else if (changed_prefs.input_autofire_linecnt > 10 * 312)
		cboAutofireRate->setSelected(1);
	else if (changed_prefs.input_autofire_linecnt > 6 * 312)
		cboAutofireRate->setSelected(2);
	else
		cboAutofireRate->setSelected(3);

	// changed mouse map
	for (int i = 0; i < 2; ++i) {
		joysmm[i]->setSelected(changed_prefs.jports[i].mousemap);
	}

	for (int i = 0; i < 2; i++) {
		const bool is_enabled = joysm[i]->getSelected() != 0;
		joysmm[i]->setEnabled(is_enabled);
		if (i == 0) {
			lblPort0mousemode->setEnabled(is_enabled);
		}
		else {
			lblPort1mousemode->setEnabled(is_enabled);
		}
	}

	for (auto i = 0; i < 5; ++i)
	{
		if (changed_prefs.input_joymouse_speed == digital_joymousespeed_values[i])
		{
			lblDigitalJoyMouseSpeedInfo->setCaption(to_string(changed_prefs.input_joymouse_speed));
			sldDigitalJoyMouseSpeed->setValue(i);
			break;
		}
	}

	for (auto i = 0; i < 14; ++i)
	{
		if (changed_prefs.input_joymouse_multiplier == analog_joymousespeed_values[i])
		{
			lblAnalogJoyMouseSpeedInfo->setCaption(to_string(changed_prefs.input_joymouse_multiplier));
			sldAnalogJoyMouseSpeed->setValue(i);
			break;
		}
	}

	for (auto i = 0; i < 14; ++i)
	{
		if (changed_prefs.input_mouse_speed == mousespeed_values[i])
		{
			lblMouseSpeedInfo->setCaption(to_string(changed_prefs.input_mouse_speed));
			sldMouseSpeed->setValue(i);
			break;
		}
	}

	chkMouseHack->setSelected(changed_prefs.input_tablet > 0);
	optBoth->setEnabled(changed_prefs.input_tablet > 0);
	optNative->setEnabled(changed_prefs.input_tablet > 0);
	optHost->setEnabled(changed_prefs.input_tablet > 0);

	chkMagicMouseUntrap->setSelected(changed_prefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC);
	chkInputAutoswitch->setSelected(changed_prefs.input_autoswitch);
	switch (changed_prefs.input_magic_mouse_cursor)
	{
		case MAGICMOUSE_BOTH:
			optBoth->setSelected(true);	
			break;
		case MAGICMOUSE_NATIVE_ONLY:
			optNative->setSelected(true);
			break;
		case MAGICMOUSE_HOST_ONLY:
			optHost->setSelected(true);
			break;
		default:
			break;
	}
	chkSwapBackslashF11->setSelected(key_swap_hack);
	chkSwapEndPgUp->setSelected(key_swap_end_pgup);
}

bool HelpPanelInput(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Here you can select the devices connected to the various input ports.");
	helptext.emplace_back("Port 0 is normally used by a Mouse, while Port 1 is usually for a Joystick.");
	helptext.emplace_back("Ports 2 and 3 are emulating the Parallel port adapter, and are only supported");
	helptext.emplace_back("in some games.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can use the Swap Ports button to swap the devices between port 0 and 1.");
	helptext.emplace_back("Auto-switching enables you to switch Port 0 between Mouse-Joystick based on");
	helptext.emplace_back("which device is being used.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Mouse Map: This allows you to use the Left analog stick on your controller");
	helptext.emplace_back("to emulate a mouse in Port 0, with the Shoulder buttons acting as Mouse buttons.");
	helptext.emplace_back("Useful if you don't have a real mouse connected and only a controller with");
	helptext.emplace_back("analog sticks (e.g. PS4, XBox).");
	helptext.emplace_back("Only works when type is set to something other than Default.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can set the Autofire Rate, and choose if a device should have Autofire");
	helptext.emplace_back("as well as what kind of autofire (normal, toggle, always-on).");
	helptext.emplace_back(" ");
	helptext.emplace_back("If you enable Virtual Mouse driver, mouse input events will be read as absolute");
	helptext.emplace_back("instead of relative. This may not work with all apps. Below that you can select");
	helptext.emplace_back("which mouse pointer(s) will be visible during emulation.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Magic Mouse untrap will free the mouse if it's moved outside the emulator window");
	helptext.emplace_back("and re-capture it when it's moved back in. Only useful when running under a");
	helptext.emplace_back("windowed environment.");
	return true;
}
