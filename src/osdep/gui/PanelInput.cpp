#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "gui_handling.h"
#include "inputdevice.h"

#if 0
#ifdef ANDROID
#include <SDL_android.h>
#endif
#endif

static const char* mousespeed_list[] = {".25", ".5", "1x", "2x", "4x"};
static const int mousespeed_values[] = {2, 5, 10, 20, 40};

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

static gcn::Label* lblPort0mousemode;
static gcn::DropDown* cboPort0mousemode;
static gcn::Label* lblPort1mousemode;
static gcn::DropDown* cboPort1mousemode;

static gcn::Label* lblParallelPortAdapter;
static gcn::Label* lblPort2;
static gcn::DropDown* cboPort2;
static gcn::Label* lblPort3;
static gcn::DropDown* cboPort3;

static gcn::Label* lblAutofireRate;
static gcn::DropDown* cboAutofireRate;
static gcn::Label* lblMouseSpeed;
static gcn::Label* lblMouseSpeedInfo;
static gcn::Slider* sldMouseSpeed;
static gcn::CheckBox* chkMouseHack;
static gcn::CheckBox* chkInputAutoswitch;

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

	int AddElement(const char* Elem)
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
static int portListIDs[MAX_INPUT_DEVICES];

const char* autoFireValues[] = { "No autofire", "Autofire", "Autofire (toggle)", "Autofire (always)" };
StringListModel autoFireList(autoFireValues, 4);

const char* autoFireRateValues[] = { "Off", "Slow", "Medium", "Fast" };
StringListModel autoFireRateList(autoFireRateValues, 4);

const char* mousemapValues[] = { "None", "Left", "Right", "Both" };
StringListModel ctrlPortMouseModeList(mousemapValues, 4);

const char* joyportmodes[] = { "Default", "Wheel Mouse", "Mouse", "Joystick", "Gamepad", "Analog Joystick", "CDTV remote mouse", "CD32 pad"};
StringListModel ctrlPortModeList(joyportmodes, 8);

class InputActionListener : public gcn::ActionListener
{
public:
	static void clear_ports(const int sel, const int current_port)
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

	static void set_port(const int sel, const int current_port)
	{
		changed_prefs.jports[current_port].id = portListIDs[sel];
		if (changed_prefs.jports[current_port].id == JPORT_NONE)
		{
			changed_prefs.jports[current_port].idc.configname[0] = 0;
			changed_prefs.jports[current_port].idc.name[0] = 0;
			changed_prefs.jports[current_port].idc.shortid[0] = 0;
		}
		inputdevice_updateconfig(nullptr, &changed_prefs);
		RefreshPanelInput();
		RefreshPanelCustom();
	}

	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cboPort0)
		{
			// Handle new device in port 0
			const auto sel = cboPort0->getSelected();
			const auto current_port = 0;

			// clear 
			clear_ports(sel, current_port);

			// set
			set_port(sel, current_port);
		}

		else if (actionEvent.getSource() == cboPort1)
		{
			// Handle new device in port 1
			const auto sel = cboPort1->getSelected();
			const auto current_port = 1;

			// clear 
			clear_ports(sel, current_port);

			// set
			set_port(sel, current_port);
		}

		else if (actionEvent.getSource() == cboPort2)
		{
			// Handle new device in port 2
			const auto sel = cboPort2->getSelected();
			const auto current_port = 2;

			// clear 
			clear_ports(sel, current_port);

			// set
			set_port(sel, current_port);
		}

		else if (actionEvent.getSource() == cboPort3)
		{
			// Handle new device in port 3
			const auto sel = cboPort3->getSelected();
			const auto current_port = 3;

			// clear 
			clear_ports(sel, current_port);

			// set
			set_port(sel, current_port);
		}

		else if (actionEvent.getSource() == cboPort0Autofire)
			changed_prefs.jports[0].autofire = cboPort0Autofire->getSelected();
		else if (actionEvent.getSource() == cboPort1Autofire)
			changed_prefs.jports[1].autofire = cboPort1Autofire->getSelected();
		else if (actionEvent.getSource() == cboPort2Autofire)
			changed_prefs.jports[2].autofire = cboPort2Autofire->getSelected();
		else if (actionEvent.getSource() == cboPort3Autofire)
			changed_prefs.jports[3].autofire = cboPort3Autofire->getSelected();
		
		else if (actionEvent.getSource() == cboPort0mode)
		{
			changed_prefs.jports[0].mode = cboPort0mode->getSelected();
			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
			RefreshPanelCustom();
		}
		else if (actionEvent.getSource() == cboPort1mode)
		{
			changed_prefs.jports[1].mode = cboPort1mode->getSelected();
			inputdevice_updateconfig(nullptr, &changed_prefs);
			RefreshPanelInput();
			RefreshPanelCustom();
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

		else if (actionEvent.getSource() == cboAutofireRate)
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

		else if (actionEvent.getSource() == sldMouseSpeed)
		{
			changed_prefs.input_joymouse_multiplier = mousespeed_values[static_cast<int>(sldMouseSpeed->getValue())];
			RefreshPanelInput();
		}

		else if (actionEvent.getSource() == chkMouseHack)
		{
#if 0
#ifdef ANDROID
			if (chkMouseHack->isSelected())
				SDL_ANDROID_SetMouseEmulationMode(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
			else
				SDL_ANDROID_SetMouseEmulationMode(1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
#endif
#endif
			changed_prefs.input_tablet = chkMouseHack->isSelected() ? TABLET_MOUSEHACK : TABLET_OFF;
		}

		else if (actionEvent.getSource() == chkInputAutoswitch)
		{
			changed_prefs.input_autoswitch = chkInputAutoswitch->isSelected();
		}
	}
};

static InputActionListener* inputActionListener;

void InitPanelInput(const struct _ConfigCategory& category)
{
	if (ctrlPortList.getNumberOfElements() == 0)
	{
		auto idx = 0;
		ctrlPortList.AddElement("None");
		portListIDs[idx] = JPORT_NONE;

		int i;
		for (i = 0; i < inputdevice_get_device_total(IDTYPE_MOUSE); i++)
		{
			const auto* const device_name = inputdevice_get_device_name(IDTYPE_MOUSE, i);
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

	inputActionListener = new InputActionListener();
	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - 60;

	lblPort0 = new gcn::Label("Port 0:");
	lblPort0->setAlignment(gcn::Graphics::RIGHT);
	cboPort0 = new gcn::DropDown(&ctrlPortList);
	cboPort0->setSize(textFieldWidth, cboPort0->getHeight());
	cboPort0->setBaseColor(gui_baseCol);
	cboPort0->setBackgroundColor(colTextboxBackground);
	cboPort0->setId("cboPort0");
	cboPort0->addActionListener(inputActionListener);

	cboPort0Autofire = new gcn::DropDown(&autoFireList);
	cboPort0Autofire->setSize(150, cboPort0Autofire->getHeight());
	cboPort0Autofire->setBaseColor(gui_baseCol);
	cboPort0Autofire->setBackgroundColor(colTextboxBackground);
	cboPort0Autofire->setId("cboPort0Autofire");
	cboPort0Autofire->addActionListener(inputActionListener);

	cboPort1Autofire = new gcn::DropDown(&autoFireList);
	cboPort1Autofire->setSize(150, cboPort1Autofire->getHeight());
	cboPort1Autofire->setBaseColor(gui_baseCol);
	cboPort1Autofire->setBackgroundColor(colTextboxBackground);
	cboPort1Autofire->setId("cboPort1Autofire");
	cboPort1Autofire->addActionListener(inputActionListener);

	cboPort2Autofire = new gcn::DropDown(&autoFireList);
	cboPort2Autofire->setSize(150, cboPort2Autofire->getHeight());
	cboPort2Autofire->setBaseColor(gui_baseCol);
	cboPort2Autofire->setBackgroundColor(colTextboxBackground);
	cboPort2Autofire->setId("cboPort2Autofire");
	cboPort2Autofire->addActionListener(inputActionListener);

	cboPort3Autofire = new gcn::DropDown(&autoFireList);
	cboPort3Autofire->setSize(150, cboPort3Autofire->getHeight());
	cboPort3Autofire->setBaseColor(gui_baseCol);
	cboPort3Autofire->setBackgroundColor(colTextboxBackground);
	cboPort3Autofire->setId("cboPort3Autofire");
	cboPort3Autofire->addActionListener(inputActionListener);
	
	cboPort0mode = new gcn::DropDown(&ctrlPortModeList);
	cboPort0mode->setSize(cboPort0mode->getWidth(), cboPort0mode->getHeight());
	cboPort0mode->setBaseColor(gui_baseCol);
	cboPort0mode->setBackgroundColor(colTextboxBackground);
	cboPort0mode->setId("cboPort0mode");
	cboPort0mode->addActionListener(inputActionListener);

	lblPort1 = new gcn::Label("Port 1:");
	lblPort1->setAlignment(gcn::Graphics::RIGHT);
	lblPort0->setSize(lblPort1->getWidth(), lblPort0->getHeight());
	cboPort1 = new gcn::DropDown(&ctrlPortList);
	cboPort1->setSize(textFieldWidth, cboPort1->getHeight());
	cboPort1->setBaseColor(gui_baseCol);
	cboPort1->setBackgroundColor(colTextboxBackground);
	cboPort1->setId("cboPort1");
	cboPort1->addActionListener(inputActionListener);

	cboPort1mode = new gcn::DropDown(&ctrlPortModeList);
	cboPort1mode->setSize(cboPort1mode->getWidth(), cboPort1mode->getHeight());
	cboPort1mode->setBaseColor(gui_baseCol);
	cboPort1mode->setBackgroundColor(colTextboxBackground);
	cboPort1mode->setId("cboPort1mode");
	cboPort1mode->addActionListener(inputActionListener);

	lblParallelPortAdapter = new gcn::Label("Emulated parallel port joystick adapter");
	lblParallelPortAdapter->setAlignment(gcn::Graphics::LEFT);
	
	lblPort2 = new gcn::Label("Port 2:");
	lblPort2->setAlignment(gcn::Graphics::LEFT);
	cboPort2 = new gcn::DropDown(&ctrlPortList);
	cboPort2->setSize(textFieldWidth, cboPort2->getHeight());
	cboPort2->setBaseColor(gui_baseCol);
	cboPort2->setBackgroundColor(colTextboxBackground);
	cboPort2->setId("cboPort2");
	cboPort2->addActionListener(inputActionListener);

	lblPort3 = new gcn::Label("Port 3:");
	lblPort3->setAlignment(gcn::Graphics::LEFT);
	cboPort3 = new gcn::DropDown(&ctrlPortList);
	cboPort3->setSize(textFieldWidth, cboPort3->getHeight());
	cboPort3->setBaseColor(gui_baseCol);
	cboPort3->setBackgroundColor(colTextboxBackground);
	cboPort3->setId("cboPort3");
	cboPort3->addActionListener(inputActionListener);

	lblPort0mousemode = new gcn::Label("Mouse Stick 0:");
	lblPort0mousemode->setAlignment(gcn::Graphics::RIGHT);
	cboPort0mousemode = new gcn::DropDown(&ctrlPortMouseModeList);
	cboPort0mousemode->setSize(68, cboPort0mousemode->getHeight());
	cboPort0mousemode->setBaseColor(gui_baseCol);
	cboPort0mousemode->setBackgroundColor(colTextboxBackground);
	cboPort0mousemode->setId("cboPort0mousemode");
	cboPort0mousemode->addActionListener(inputActionListener);

	lblPort1mousemode = new gcn::Label("Mouse Stick 1:");
	lblPort1mousemode->setAlignment(gcn::Graphics::RIGHT);
	cboPort1mousemode = new gcn::DropDown(&ctrlPortMouseModeList);
	cboPort1mousemode->setSize(68, cboPort1mousemode->getHeight());
	cboPort1mousemode->setBaseColor(gui_baseCol);
	cboPort1mousemode->setBackgroundColor(colTextboxBackground);
	cboPort1mousemode->setId("cboPort1mousemode");
	cboPort1mousemode->addActionListener(inputActionListener);

	lblAutofireRate = new gcn::Label("Autofire Rate:");
	lblAutofireRate->setAlignment(gcn::Graphics::RIGHT);
	cboAutofireRate = new gcn::DropDown(&autoFireRateList);
	cboAutofireRate->setSize(80, cboAutofireRate->getHeight());
	cboAutofireRate->setBaseColor(gui_baseCol);
	cboAutofireRate->setBackgroundColor(colTextboxBackground);
	cboAutofireRate->setId("cboAutofireRate");
	cboAutofireRate->addActionListener(inputActionListener);

	lblMouseSpeed = new gcn::Label("Mouse Speed:");
	lblMouseSpeed->setAlignment(gcn::Graphics::RIGHT);
	sldMouseSpeed = new gcn::Slider(0, 4);
	sldMouseSpeed->setSize(110, SLIDER_HEIGHT);
	sldMouseSpeed->setBaseColor(gui_baseCol);
	sldMouseSpeed->setMarkerLength(20);
	sldMouseSpeed->setStepLength(1);
	sldMouseSpeed->setId("MouseSpeed");
	sldMouseSpeed->addActionListener(inputActionListener);
	lblMouseSpeedInfo = new gcn::Label(".25");

	chkMouseHack = new gcn::CheckBox("Virtual mouse driver");
	chkMouseHack->setId("chkMouseHack");
	chkMouseHack->addActionListener(inputActionListener);

	chkInputAutoswitch = new gcn::CheckBox("Mouse/Joystick autoswitching");
	chkInputAutoswitch->setId("chkInputAutoswitch");
	chkInputAutoswitch->addActionListener(inputActionListener);

	auto posY = DISTANCE_BORDER;
	category.panel->add(lblPort0, DISTANCE_BORDER, posY);
	category.panel->add(cboPort0, DISTANCE_BORDER + lblPort0->getWidth() + 8, posY);
	posY += cboPort0->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboPort0Autofire, cboPort0->getX(), posY);
	category.panel->add(cboPort0mode, cboPort0Autofire->getX() + cboPort0Autofire->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboPort0Autofire->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort1, DISTANCE_BORDER, posY);
	category.panel->add(cboPort1, DISTANCE_BORDER + lblPort1->getWidth() + 8, posY);
	posY += cboPort1->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboPort1Autofire, cboPort1->getX(), posY);
	category.panel->add(cboPort1mode, cboPort1Autofire->getX() + cboPort1Autofire->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboPort1Autofire->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(chkInputAutoswitch, cboPort1Autofire->getX(), posY);
	posY += chkInputAutoswitch->getHeight() + DISTANCE_NEXT_Y * 2;
	
	category.panel->add(lblParallelPortAdapter, DISTANCE_BORDER, posY);
	posY += lblParallelPortAdapter->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(lblPort2, DISTANCE_BORDER, posY);
	category.panel->add(cboPort2, DISTANCE_BORDER + lblPort2->getWidth() + 8, posY);
	posY += cboPort2->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboPort2Autofire, cboPort2->getX(), posY);
	posY += cboPort2Autofire->getHeight() + DISTANCE_NEXT_Y;
	
	category.panel->add(lblPort3, DISTANCE_BORDER, posY);
	category.panel->add(cboPort3, DISTANCE_BORDER + lblPort3->getWidth() + 8, posY);
	posY += cboPort3->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboPort3Autofire, cboPort3->getX(), posY);
	posY += cboPort3Autofire->getHeight() + DISTANCE_NEXT_Y * 2;
	
	category.panel->add(lblPort0mousemode, DISTANCE_BORDER, posY);
	category.panel->add(cboPort0mousemode, lblPort0mousemode->getX() + lblPort0mousemode->getWidth() + 8, posY);

	category.panel->add(lblMouseSpeed,
	                    cboPort0mousemode->getX() + cboPort0mousemode->getWidth() + (DISTANCE_NEXT_X * 2), posY);
	category.panel->add(sldMouseSpeed, lblMouseSpeed->getX() + lblMouseSpeed->getWidth() + 8, posY);
	category.panel->add(lblMouseSpeedInfo, sldMouseSpeed->getX() + sldMouseSpeed->getWidth() + 8, posY);
	posY += lblMouseSpeed->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort1mousemode, DISTANCE_BORDER, posY);
	category.panel->add(cboPort1mousemode, lblPort1mousemode->getX() + lblPort1mousemode->getWidth() + 8, posY);
	posY += lblPort1mousemode->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblAutofireRate, DISTANCE_BORDER, posY);
	category.panel->add(cboAutofireRate, DISTANCE_BORDER + lblAutofireRate->getWidth() + 8, posY);
	category.panel->add(chkMouseHack, cboAutofireRate->getX() + cboAutofireRate->getWidth() + DISTANCE_NEXT_X, posY);
	posY += chkMouseHack->getHeight() + DISTANCE_NEXT_Y;

	RefreshPanelInput();
}

void ExitPanelInput()
{
	delete lblPort0;
	delete cboPort0;
	delete cboPort0Autofire;
	delete cboPort0mode;
	delete lblPort0mousemode;
	delete cboPort0mousemode;

	delete lblPort1;
	delete cboPort1;
	delete cboPort1Autofire;
	delete cboPort1mode;

	delete lblPort1mousemode;
	delete cboPort1mousemode;

	delete lblParallelPortAdapter;
	delete lblPort2;
	delete cboPort2;
	delete cboPort2Autofire;
	
	delete lblPort3;
	delete cboPort3;
	delete cboPort3Autofire;
	
	delete lblAutofireRate;
	delete cboAutofireRate;
	delete lblMouseSpeed;
	delete sldMouseSpeed;
	delete lblMouseSpeedInfo;

	delete chkMouseHack;
	delete chkInputAutoswitch;

	delete inputActionListener;
}

void RefreshPanelInput()
{
	// Set current device in port 0
	auto idx = 0;
	for (auto i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
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
	for (auto i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
	{
		if (changed_prefs.jports[1].id == portListIDs[i])
		{
			idx = i;
			break;
		}
	}
	cboPort1->setSelected(idx);

	// Set current device in port 2
	idx = 0;
	for (auto i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
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
	for (auto i = 0; i < ctrlPortList.getNumberOfElements(); ++i)
	{
		if (changed_prefs.jports[3].id == portListIDs[i])
		{
			idx = i;
			break;
		}
	}
	cboPort3->setSelected(idx);

	if (changed_prefs.input_autofire_linecnt == 0)
		cboAutofireRate->setSelected(0);
	else if (changed_prefs.input_autofire_linecnt > 10 * 312)
		cboAutofireRate->setSelected(1);
	else if (changed_prefs.input_autofire_linecnt > 6 * 312)
		cboAutofireRate->setSelected(2);
	else
		cboAutofireRate->setSelected(3);

	cboPort0mode->setSelected(changed_prefs.jports[0].mode);
	cboPort1mode->setSelected(changed_prefs.jports[1].mode);
	
	cboPort0Autofire->setSelected(changed_prefs.jports[0].autofire);
	cboPort1Autofire->setSelected(changed_prefs.jports[1].autofire);
	cboPort2Autofire->setSelected(changed_prefs.jports[2].autofire);
	cboPort3Autofire->setSelected(changed_prefs.jports[3].autofire);

	// changed mouse map
	cboPort0mousemode->setSelected(changed_prefs.jports[0].mousemap);
	cboPort1mousemode->setSelected(changed_prefs.jports[1].mousemap);

	if (cboPort0mode->getSelected() == 0)
	{
		cboPort0mousemode->setEnabled(false);
		lblPort0mousemode->setEnabled(false);
	}
	else
	{
		cboPort0mousemode->setEnabled(true);
		lblPort0mousemode->setEnabled(true);
	}

	if (cboPort1mode->getSelected() == 0)
	{
		cboPort1mousemode->setEnabled(false);
		lblPort1mousemode->setEnabled(false);
	}
	else
	{
		cboPort1mousemode->setEnabled(true);
		lblPort1mousemode->setEnabled(true);
	}

	for (auto i = 0; i < 5; ++i)
	{
		if (changed_prefs.input_joymouse_multiplier == mousespeed_values[i])
		{
			sldMouseSpeed->setValue(i);
			lblMouseSpeedInfo->setCaption(mousespeed_list[i]);
			break;
		}
	}

	chkMouseHack->setSelected(changed_prefs.input_tablet > 0);
	chkInputAutoswitch->setSelected(changed_prefs.input_autoswitch);
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
