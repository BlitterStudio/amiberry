#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

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
static gcn::UaeCheckBox* chkMouseHack;

class StringListModel : public gcn::ListModel
{
private:
	std::vector<std::string> values;
public:
	StringListModel(const char* entries[], const int count)
	{
		for (int i = 0; i < count; ++i)
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
		if (i < 0 || static_cast<unsigned int>(i) >= values.size())
			return "---";
		return values[i];
	}
};

static StringListModel ctrlPortList(nullptr, 0);
static int portListIDs[MAX_INPUT_DEVICES];

const char* autofireValues[] = {"Off", "Slow", "Medium", "Fast"};
StringListModel autofireList(autofireValues, 4);

const char* mousemapValues[] = {"None", "Left", "Right", "Both"};
StringListModel ctrlPortMouseModeList(mousemapValues, 4);

const char* joyportmodes[] = {"Mouse", "Joystick", "CD32", "Default"};
StringListModel ctrlPortModeList(joyportmodes, 4);

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
			RefreshPanelCustom();
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

	inputActionListener = new InputActionListener();
	const auto textFieldWidth = category.panel->getWidth() - (2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X
	);

	lblPort0 = new gcn::Label("Port 0 [Mouse]:");
	lblPort0->setAlignment(gcn::Graphics::RIGHT);
	cboPort0 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort0->setSize(textFieldWidth / 2, cboPort0->getHeight());
	cboPort0->setBaseColor(gui_baseCol);
	cboPort0->setBackgroundColor(colTextboxBackground);
	cboPort0->setId("cboPort0");
	cboPort0->addActionListener(inputActionListener);

	cboPort0mode = new gcn::UaeDropDown(&ctrlPortModeList);
	cboPort0mode->setSize(cboPort0mode->getWidth(), cboPort0mode->getHeight());
	cboPort0mode->setBaseColor(gui_baseCol);
	cboPort0mode->setBackgroundColor(colTextboxBackground);
	cboPort0mode->setId("cboPort0mode");
	cboPort0mode->addActionListener(inputActionListener);

	lblPort1 = new gcn::Label("Port 1 [Joystick]:");
	lblPort1->setAlignment(gcn::Graphics::RIGHT);
	lblPort0->setSize(lblPort1->getWidth(), lblPort0->getHeight());
	cboPort1 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort1->setSize(textFieldWidth / 2, cboPort1->getHeight());
	cboPort1->setBaseColor(gui_baseCol);
	cboPort1->setBackgroundColor(colTextboxBackground);
	cboPort1->setId("cboPort1");
	cboPort1->addActionListener(inputActionListener);

	cboPort1mode = new gcn::UaeDropDown(&ctrlPortModeList);
	cboPort1mode->setSize(cboPort1mode->getWidth(), cboPort1mode->getHeight());
	cboPort1mode->setBaseColor(gui_baseCol);
	cboPort1mode->setBackgroundColor(colTextboxBackground);
	cboPort1mode->setId("cboPort1mode");
	cboPort1mode->addActionListener(inputActionListener);

	lblPort2 = new gcn::Label("Port 2 [Parallel 1]:");
	lblPort2->setAlignment(gcn::Graphics::LEFT);
	cboPort2 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort2->setSize(textFieldWidth / 2, cboPort2->getHeight());
	cboPort2->setBaseColor(gui_baseCol);
	cboPort2->setBackgroundColor(colTextboxBackground);
	cboPort2->setId("cboPort2");
	cboPort2->addActionListener(inputActionListener);

	lblPort3 = new gcn::Label("Port 3 [Parallel 2]:");
	lblPort3->setAlignment(gcn::Graphics::LEFT);
	cboPort3 = new gcn::UaeDropDown(&ctrlPortList);
	cboPort3->setSize(textFieldWidth / 2, cboPort3->getHeight());
	cboPort3->setBaseColor(gui_baseCol);
	cboPort3->setBackgroundColor(colTextboxBackground);
	cboPort3->setId("cboPort3");
	cboPort3->addActionListener(inputActionListener);

	lblPort0mousemode = new gcn::Label("Mouse Stick 0:");
	lblPort0mousemode->setAlignment(gcn::Graphics::RIGHT);
	cboPort0mousemode = new gcn::UaeDropDown(&ctrlPortMouseModeList);
	cboPort0mousemode->setSize(68, cboPort0mousemode->getHeight());
	cboPort0mousemode->setBaseColor(gui_baseCol);
	cboPort0mousemode->setBackgroundColor(colTextboxBackground);
	cboPort0mousemode->setId("cboPort0mousemode");
	cboPort0mousemode->addActionListener(inputActionListener);

	lblPort1mousemode = new gcn::Label("Mouse Stick 1:");
	lblPort1mousemode->setAlignment(gcn::Graphics::RIGHT);
	cboPort1mousemode = new gcn::UaeDropDown(&ctrlPortMouseModeList);
	cboPort1mousemode->setSize(68, cboPort1mousemode->getHeight());
	cboPort1mousemode->setBaseColor(gui_baseCol);
	cboPort1mousemode->setBackgroundColor(colTextboxBackground);
	cboPort1mousemode->setId("cboPort1mousemode");
	cboPort1mousemode->addActionListener(inputActionListener);

	lblAutofire = new gcn::Label("Autofire Rate:");
	lblAutofire->setAlignment(gcn::Graphics::RIGHT);
	cboAutofire = new gcn::UaeDropDown(&autofireList);
	cboAutofire->setSize(80, cboAutofire->getHeight());
	cboAutofire->setBaseColor(gui_baseCol);
	cboAutofire->setBackgroundColor(colTextboxBackground);
	cboAutofire->setId("cboAutofire");
	cboAutofire->addActionListener(inputActionListener);

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

	chkMouseHack = new gcn::UaeCheckBox("Enable mousehack");
	chkMouseHack->setId("MouseHack");
	chkMouseHack->addActionListener(inputActionListener);

	auto posY = DISTANCE_BORDER;
	category.panel->add(lblPort0, DISTANCE_BORDER, posY);
	category.panel->add(cboPort0, DISTANCE_BORDER + lblPort0->getWidth() + 8, posY);
	category.panel->add(cboPort0mode, cboPort0->getX() + cboPort0->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboPort0->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort1, DISTANCE_BORDER, posY);
	category.panel->add(cboPort1, DISTANCE_BORDER + lblPort1->getWidth() + 8, posY);
	category.panel->add(cboPort1mode, cboPort1->getX() + cboPort1->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboPort1->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort2, DISTANCE_BORDER, posY);
	category.panel->add(cboPort2, DISTANCE_BORDER + lblPort2->getWidth() + 8, posY);
	posY += cboPort2->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblPort3, DISTANCE_BORDER, posY);
	category.panel->add(cboPort3, DISTANCE_BORDER + lblPort3->getWidth() + 8, posY);
	posY += cboPort3->getHeight() + DISTANCE_NEXT_Y * 2;

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

	category.panel->add(lblAutofire, DISTANCE_BORDER, posY);
	category.panel->add(cboAutofire, DISTANCE_BORDER + lblAutofire->getWidth() + 8, posY);
	category.panel->add(chkMouseHack, cboAutofire->getX() + cboAutofire->getWidth() + DISTANCE_NEXT_X, posY);
	posY += chkMouseHack->getHeight() + DISTANCE_NEXT_Y;

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

	delete chkMouseHack;

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

	for (auto i = 0; i < 5; ++i)
	{
		if (changed_prefs.input_joymouse_multiplier == mousespeed_values[i])
		{
			sldMouseSpeed->setValue(i);
			lblMouseSpeedInfo->setCaption(mousespeed_list[i]);
			break;
		}
	}

	chkMouseHack->setSelected(changed_prefs.input_tablet == TABLET_MOUSEHACK);
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
	helptext.emplace_back("\"Tap Delay\" specifies the time between taping the screen and an emulated ");
	helptext.emplace_back("mouse button click.");
	return true;
}
