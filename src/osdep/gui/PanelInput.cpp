#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>

#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#define MAX_PORTSUBMODES 16
static int portsubmodes[MAX_PORTSUBMODES];
static int retroarch_kb = 0;
static int joysticks = 0;
static int mice = 0;

static const int mousespeed_values[] = { 50, 75, 100, 125, 150 };
static const int digital_joymousespeed_values[] = { 2, 5, 10, 15, 20 };
static const int analog_joymousespeed_values[] = { 50, 75, 100, 125, 150 };

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

class StringListModel : public gcn::ListModel
{
private:
	std::vector<std::string> values;
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

	int add_element(const char* Elem)
	{
		values.emplace_back(Elem);
		return 0;
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

static StringListModel ctrlPortList(nullptr, 0);
static int portListIDs[MAX_INPUT_DEVICES + JSEM_LASTKBD + 1];

const char* autoFireValues[] = { "No autofire", "Autofire", "Autofire (toggle)", "Autofire (always)" };
StringListModel autoFireList(autoFireValues, 4);

const char* autoFireRateValues[] = { "Off", "Slow", "Medium", "Fast" };
StringListModel autoFireRateList(autoFireRateValues, 4);

const char* mousemapValues[] = { "None", "Left", "Right", "Both" };
StringListModel ctrlPortMouseModeList(mousemapValues, 4);

const char* joyportmodes[] = { "Default", "Wheel Mouse", "Mouse", "Joystick", "Gamepad", "Analog Joystick", "CDTV remote mouse", "CD32 pad"};
StringListModel ctrlPortModeList(joyportmodes, 8);

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
			inputdevice_validate_jports(&changed_prefs, changedport, NULL);

		inputdevice_updateconfig(NULL, &changed_prefs);
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
			changed_prefs.input_tablet = chkMouseHack->isSelected() ? TABLET_MOUSEHACK : TABLET_OFF;

		else if (actionEvent.getSource() == chkMagicMouseUntrap)
		{
			if (chkMagicMouseUntrap->isSelected())
			{
				changed_prefs.input_mouse_untrap |= MOUSEUNTRAP_MAGIC;
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
			else
			{
				changed_prefs.input_mouse_untrap &= ~MOUSEUNTRAP_MAGIC;
			}
		}

		else if (actionEvent.getSource() == optBoth)
			changed_prefs.input_magic_mouse_cursor = MAGICMOUSE_BOTH;
		else if (actionEvent.getSource() == optNative)
			changed_prefs.input_magic_mouse_cursor = MAGICMOUSE_NATIVE_ONLY;
		else if (actionEvent.getSource() == optHost)
			changed_prefs.input_magic_mouse_cursor = MAGICMOUSE_HOST_ONLY;
		
		else if (actionEvent.getSource() == chkInputAutoswitch)
			changed_prefs.input_autoswitch = chkInputAutoswitch->isSelected();

		else if (actionEvent.getSource() == cmdSwapPorts)
		{
			inputdevice_swap_compa_ports(&changed_prefs, 0);
			RefreshPanelInput();
			inputdevice_forget_unplugged_device(0);
			inputdevice_forget_unplugged_device(1);
		}
		
		RefreshPanelInput();
	}
};

static InputActionListener* inputActionListener;

void InitPanelInput(const struct _ConfigCategory& category)
{
	retroarch_kb = get_retroarch_kb_num();
	joysticks = inputdevice_get_device_total(IDTYPE_JOYSTICK);
	mice = inputdevice_get_device_total(IDTYPE_MOUSE);
	
	if (ctrlPortList.getNumberOfElements() == 0)
	{
		auto idx = 0;
		ctrlPortList.add_element("<none>");
		portListIDs[idx] = JPORT_NONE;
		
		idx++;
		ctrlPortList.add_element("Keyboard Layout A (Numpad, 0/5=Fire, Decimal/DEL=2nd Fire)");
		portListIDs[idx] = JSEM_KBDLAYOUT;
		
		idx++;
		ctrlPortList.add_element("Keyboard Layout B (Cursor, RCtrl/RAlt=Fire, RShift=2nd Fire)");
		portListIDs[idx] = JSEM_KBDLAYOUT + 1;
		
		idx++;
		ctrlPortList.add_element("Keyboard Layout C (WSAD, LAlt=Fire, LShift=2nd Fire)");
		portListIDs[idx] = JSEM_KBDLAYOUT + 2;

		idx++;
		ctrlPortList.add_element("Keyrah Layout (Cursor, Space/RAlt=Fire, RShift=2nd Fire)");
		portListIDs[idx] = JSEM_KBDLAYOUT + 3;

		for (auto j = 0; j < 4; j++)
		{
			auto element = "Retroarch KBD as Joystick Player" + std::to_string(j + 1);
			ctrlPortList.add_element(element.c_str());
			idx++;
			portListIDs[idx] = JSEM_KBDLAYOUT + j + 4;
		}
		
		for (auto j = 0; j < joysticks; j++)
		{
			ctrlPortList.add_element(inputdevice_get_device_name(IDTYPE_JOYSTICK, j));
			idx++;
			portListIDs[idx] = JSEM_JOYS + j;
		}

		for (auto j = 0; j < mice; j++)
		{
			ctrlPortList.add_element(inputdevice_get_device_name(IDTYPE_MOUSE, j));
			idx++;
			portListIDs[idx] = JSEM_MICE + j;
		}
	}

	inputPortsActionListener = new InputPortsActionListener();
	inputActionListener = new InputActionListener();
	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - 60;

	lblPort0 = new gcn::Label("Port 0:");
	lblPort0->setAlignment(gcn::Graphics::RIGHT);

	lblPort1 = new gcn::Label("Port 1:");
	lblPort1->setAlignment(gcn::Graphics::RIGHT);

	lblPort2 = new gcn::Label("Port 2:");
	lblPort2->setAlignment(gcn::Graphics::LEFT);

	lblPort3 = new gcn::Label("Port 3:");
	lblPort3->setAlignment(gcn::Graphics::LEFT);
	
	for (auto i = 0; i < MAX_JPORTS; i++)
	{
		joys[i] = new gcn::DropDown(&ctrlPortList);
		joys[i]->setSize(textFieldWidth, joys[i]->getHeight());
		joys[i]->setBaseColor(gui_baseCol);
		joys[i]->setBackgroundColor(colTextboxBackground);
		joys[i]->addActionListener(inputPortsActionListener);

		joysaf[i] = new gcn::DropDown(&autoFireList);
		joysaf[i]->setSize(150, joysaf[i]->getHeight());
		joysaf[i]->setBaseColor(gui_baseCol);
		joysaf[i]->setBackgroundColor(colTextboxBackground);
		joysaf[i]->addActionListener(inputPortsActionListener);

		if (i < 2)
		{
			joysm[i] = new gcn::DropDown(&ctrlPortModeList);
			joysm[i]->setSize(joysm[i]->getWidth(), joysm[i]->getHeight());
			joysm[i]->setBaseColor(gui_baseCol);
			joysm[i]->setBackgroundColor(colTextboxBackground);
			joysm[i]->addActionListener(inputPortsActionListener);

			joysmm[i] = new gcn::DropDown(&ctrlPortMouseModeList);
			joysmm[i]->setSize(95, joysmm[i]->getHeight());
			joysmm[i]->setBaseColor(gui_baseCol);
			joysmm[i]->setBackgroundColor(colTextboxBackground);
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

	cmdSwapPorts = new gcn::Button("Swap ports");
	cmdSwapPorts->setId("cmdSwapPorts");
	cmdSwapPorts->setSize(150, BUTTON_HEIGHT);
	cmdSwapPorts->setBaseColor(gui_baseCol);
	cmdSwapPorts->addActionListener(inputActionListener);

	lblPort0mousemode = new gcn::Label("Mouse Stick 0:");
	lblPort0mousemode->setAlignment(gcn::Graphics::RIGHT);

	lblPort1mousemode = new gcn::Label("Mouse Stick 1:");
	lblPort1mousemode->setAlignment(gcn::Graphics::RIGHT);

	lblAutofireRate = new gcn::Label("Autofire Rate:");
	lblAutofireRate->setAlignment(gcn::Graphics::RIGHT);
	cboAutofireRate = new gcn::DropDown(&autoFireRateList);
	cboAutofireRate->setSize(95, cboAutofireRate->getHeight());
	cboAutofireRate->setBaseColor(gui_baseCol);
	cboAutofireRate->setBackgroundColor(colTextboxBackground);
	cboAutofireRate->setId("cboAutofireRate");
	cboAutofireRate->addActionListener(inputActionListener);

	lblDigitalJoyMouseSpeed = new gcn::Label("Digital joy-mouse speed:");
	lblDigitalJoyMouseSpeed->setAlignment(gcn::Graphics::RIGHT);
	lblDigitalJoyMouseSpeedInfo = new gcn::Label("100");
	sldDigitalJoyMouseSpeed = new gcn::Slider(0, 4);
	sldDigitalJoyMouseSpeed->setSize(100, SLIDER_HEIGHT);
	sldDigitalJoyMouseSpeed->setBaseColor(gui_baseCol);
	sldDigitalJoyMouseSpeed->setMarkerLength(20);
	sldDigitalJoyMouseSpeed->setStepLength(1);
	sldDigitalJoyMouseSpeed->setId("sldDigitalJoyMouseSpeed");
	sldDigitalJoyMouseSpeed->addActionListener(inputActionListener);

	lblAnalogJoyMouseSpeed = new gcn::Label("Analog joy-mouse speed:");
	lblAnalogJoyMouseSpeed->setAlignment(gcn::Graphics::RIGHT);
	lblAnalogJoyMouseSpeedInfo = new gcn::Label("100");
	sldAnalogJoyMouseSpeed = new gcn::Slider(0, 4);
	sldAnalogJoyMouseSpeed->setSize(100, SLIDER_HEIGHT);
	sldAnalogJoyMouseSpeed->setBaseColor(gui_baseCol);
	sldAnalogJoyMouseSpeed->setMarkerLength(20);
	sldAnalogJoyMouseSpeed->setStepLength(1);
	sldAnalogJoyMouseSpeed->setId("sldAnalogJoyMouseSpeed");
	sldAnalogJoyMouseSpeed->addActionListener(inputActionListener);

	lblMouseSpeed = new gcn::Label("Mouse speed:");
	lblMouseSpeed->setAlignment(gcn::Graphics::RIGHT);
	lblMouseSpeedInfo = new gcn::Label("100");
	sldMouseSpeed = new gcn::Slider(0, 4);
	sldMouseSpeed->setSize(100, SLIDER_HEIGHT);
	sldMouseSpeed->setBaseColor(gui_baseCol);
	sldMouseSpeed->setMarkerLength(20);
	sldMouseSpeed->setStepLength(1);
	sldMouseSpeed->setId("sldMouseSpeed");
	sldMouseSpeed->addActionListener(inputActionListener);
	
	chkMouseHack = new gcn::CheckBox("Virtual mouse driver");
	chkMouseHack->setId("chkMouseHack");
	chkMouseHack->addActionListener(inputActionListener);

	chkMagicMouseUntrap = new gcn::CheckBox("Magic Mouse untrap");
	chkMagicMouseUntrap->setId("chkMagicMouseUntrap");
	chkMagicMouseUntrap->addActionListener(inputActionListener);

	optBoth = new gcn::RadioButton("Both", "radioCursorGroup");
	optBoth->setId("optBoth");
	optBoth->addActionListener(inputActionListener);
	optNative = new gcn::RadioButton("Native only", "radioCursorGroup");
	optNative->setId("optNative");
	optNative->addActionListener(inputActionListener);
	optHost = new gcn::RadioButton("Host only", "radioCursorGroup");
	optHost->setId("optHost");
	optHost->addActionListener(inputActionListener);
	
	chkInputAutoswitch = new gcn::CheckBox("Mouse/Joystick autoswitching");
	chkInputAutoswitch->setId("chkInputAutoswitch");
	chkInputAutoswitch->addActionListener(inputActionListener);

	auto posY = DISTANCE_BORDER;
	category.panel->add(lblPort0, DISTANCE_BORDER, posY);
	category.panel->add(joys[0], DISTANCE_BORDER + lblPort0->getWidth() + 8, posY);
	posY += joys[0]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(joysaf[0], joys[0]->getX(), posY);
	category.panel->add(joysm[0], joysaf[0]->getX() + joysaf[0]->getWidth() + DISTANCE_NEXT_X, posY);
	posY += joysaf[0]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort1, DISTANCE_BORDER, posY);
	category.panel->add(joys[1], DISTANCE_BORDER + lblPort1->getWidth() + 8, posY);
	posY += joys[1]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(joysaf[1], joys[1]->getX(), posY);
	category.panel->add(joysm[1], joysaf[1]->getX() + joysaf[1]->getWidth() + DISTANCE_NEXT_X, posY);
	posY += joysaf[1]->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cmdSwapPorts, joysaf[1]->getX(), posY);
	category.panel->add(chkInputAutoswitch, cmdSwapPorts->getX() + cmdSwapPorts->getWidth() + DISTANCE_NEXT_X, posY + BUTTON_HEIGHT/4);
	posY += chkInputAutoswitch->getHeight() + DISTANCE_NEXT_Y * 2;
	
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

	category.panel->add(lblDigitalJoyMouseSpeed, joysmm[0]->getX() + joysmm[0]->getWidth() + DISTANCE_NEXT_X * 2, posY);
	category.panel->add(sldDigitalJoyMouseSpeed, lblDigitalJoyMouseSpeed->getX() + lblDigitalJoyMouseSpeed->getWidth() + 8, posY);
	category.panel->add(lblDigitalJoyMouseSpeedInfo, sldDigitalJoyMouseSpeed->getX() + sldDigitalJoyMouseSpeed->getWidth() + 8, posY);
	posY += lblDigitalJoyMouseSpeed->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort1mousemode, DISTANCE_BORDER, posY);
	category.panel->add(joysmm[1], lblPort1mousemode->getX() + lblPort1mousemode->getWidth() + 8, posY);

	category.panel->add(lblAnalogJoyMouseSpeed, joysmm[1]->getX() + joysmm[1]->getWidth() + DISTANCE_NEXT_X * 2, posY);
	category.panel->add(sldAnalogJoyMouseSpeed, sldDigitalJoyMouseSpeed->getX(), posY);
	category.panel->add(lblAnalogJoyMouseSpeedInfo, sldAnalogJoyMouseSpeed->getX() + sldAnalogJoyMouseSpeed->getWidth() + 8, posY);
	posY += lblAnalogJoyMouseSpeed->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblAutofireRate, DISTANCE_BORDER, posY);
	category.panel->add(cboAutofireRate, DISTANCE_BORDER + lblAutofireRate->getWidth() + 8, posY);

	category.panel->add(lblMouseSpeed, joysmm[1]->getX() + joysmm[1]->getWidth() + DISTANCE_NEXT_X * 2, posY);
	category.panel->add(sldMouseSpeed, sldAnalogJoyMouseSpeed->getX(), posY);
	category.panel->add(lblMouseSpeedInfo, sldMouseSpeed->getX() + sldMouseSpeed->getWidth() + 8, posY);
	posY += lblMouseSpeed->getHeight() + DISTANCE_NEXT_Y * 2;
	
	category.panel->add(chkMouseHack, DISTANCE_BORDER, posY);
	category.panel->add(chkMagicMouseUntrap, lblMouseSpeed->getX(), posY);
	posY += chkMouseHack->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(optBoth, DISTANCE_BORDER, posY);
	category.panel->add(optNative, optBoth->getX() + optBoth->getWidth() + 8, posY);
	category.panel->add(optHost, optNative->getX() + optNative->getWidth() + 8, posY);
	
	for (auto& portsubmode : portsubmodes)
	{
		portsubmode = 0;
	}
	portsubmodes[8] = 2;
	portsubmodes[9] = -1;

	inputdevice_updateconfig(NULL, &changed_prefs);
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
}

void RefreshPanelInput()
{
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

	if (changed_prefs.input_autofire_linecnt == 0)
		cboAutofireRate->setSelected(0);
	else if (changed_prefs.input_autofire_linecnt > 10 * 312)
		cboAutofireRate->setSelected(1);
	else if (changed_prefs.input_autofire_linecnt > 6 * 312)
		cboAutofireRate->setSelected(2);
	else
		cboAutofireRate->setSelected(3);

	// changed mouse map
	joysmm[0]->setSelected(changed_prefs.jports[0].mousemap);
	joysmm[1]->setSelected(changed_prefs.jports[1].mousemap);

	if (joysm[0]->getSelected() == 0)
	{
		joysmm[0]->setEnabled(false);
		lblPort0mousemode->setEnabled(false);
	}
	else
	{
		joysmm[0]->setEnabled(true);
		lblPort0mousemode->setEnabled(true);
	}

	if (joysm[1]->getSelected() == 0)
	{
		joysmm[1]->setEnabled(false);
		lblPort1mousemode->setEnabled(false);
	}
	else
	{
		joysmm[1]->setEnabled(true);
		lblPort1mousemode->setEnabled(true);
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

	for (auto i = 0; i < 5; ++i)
	{
		if (changed_prefs.input_joymouse_multiplier == analog_joymousespeed_values[i])
		{
			lblAnalogJoyMouseSpeedInfo->setCaption(to_string(changed_prefs.input_joymouse_multiplier));
			sldAnalogJoyMouseSpeed->setValue(i);
			break;
		}
	}

	for (auto i = 0; i < 5; ++i)
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
}

bool HelpPanelInput(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("You can select the control type for both ports and the rate for autofire.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Set the emulated mouse speed to .25x, .5x, 1x, 2x and 4x to slow down or ");
	helptext.emplace_back("speed up the mouse.");
	helptext.emplace_back(" ");
	helptext.emplace_back("When \"Enable mousehack\" is activated, you can use touch input to set");
	helptext.emplace_back("the mouse pointer to the exact position. This works very well on Workbench, ");
	helptext.emplace_back("but many games using their own mouse handling and will not profit from this mode.");
	helptext.emplace_back(" ");
	return true;
}
