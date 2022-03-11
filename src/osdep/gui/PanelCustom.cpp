#include <cstdio>
#include <cstring>

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
#include "fsdb_host.h"

static gcn::Window* grpPort;
static gcn::RadioButton* optPort0;
static gcn::RadioButton* optPort1;
static gcn::RadioButton* optPort2;
static gcn::RadioButton* optPort3;

static gcn::Label* lblFunction;
static gcn::RadioButton* optMultiNone;
static gcn::RadioButton* optMultiSelect;

static gcn::Label* lblSetHotkey;
static gcn::TextField* txtSetHotkey;
static gcn::Button* cmdSetHotkey;
static gcn::ImageButton* cmdSetHotkeyClear;

static gcn::Label* lblCustomButtonAction[SDL_CONTROLLER_BUTTON_MAX];
static gcn::DropDown* cboCustomButtonAction[SDL_CONTROLLER_BUTTON_MAX];

static gcn::Label* lblCustomAxisAction[SDL_CONTROLLER_AXIS_MAX];
static gcn::DropDown* cboCustomAxisAction[SDL_CONTROLLER_AXIS_MAX];

static gcn::Label* lblPortInput;
static gcn::TextField* txtPortInput;
static gcn::Label* lblRetroarch;

static int SelectedPort = 1;
static int SelectedFunction = 0;


class string_list_model : public gcn::ListModel
{
private:
	std::vector<std::string> values{};
public:
	string_list_model(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int add_element(const char* elem) override
	{
		values.emplace_back(elem);
		return 0;
	}

	void clear_elements() override
	{
		values.clear();
	}

	int swap_first_element(const char* Elem)
	{
		values.erase(values.begin());
		values.emplace(values.begin(), Elem);
		return 0;
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

static string_list_model CustomEventList(nullptr, 0);
static string_list_model CustomEventList_HotKey(nullptr, 0);
static string_list_model CustomEventList_Menu(nullptr, 0);
static string_list_model CustomEventList_Quit(nullptr, 0);
static string_list_model CustomEventList_Reset(nullptr, 0);

const string label_button_list[] = {
	"South:", "East:", "West:", "North:", "Select:", "Guide:", "Start:", "L.Stick:", "R.Stick:",
	"L.Shoulder:", "R.Shoulder:", "DPad Up:", "DPad Down:", "DPad Left:", "DPad Right:",
	"Misc1:", "Paddle1:", "Paddle2:", "Paddle3:", "Paddle4:", "Touchpad:"
};

const string label_axis_list[] = {
	"Left X:", "Left Y:", "Right X:", "Right Y:", "L.Trigger:", "R.Trigger:"
};

class GroupActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optPort0)
			SelectedPort = 0;
		else if (actionEvent.getSource() == optPort1)
			SelectedPort = 1;
		else if (actionEvent.getSource() == optPort2)
			SelectedPort = 2;
		else if (actionEvent.getSource() == optPort3)
			SelectedPort = 3;

		else if (actionEvent.getSource() == optMultiNone)
			SelectedFunction = 0;
		else if (actionEvent.getSource() == optMultiSelect)
			SelectedFunction = 1;

		else if (actionEvent.getSource() == cmdSetHotkey)
		{
			const auto button = ShowMessageForInput("Press a button", "Press a button on your controller to set it as the Hotkey button", "Cancel");
			if (!button.key_name.empty())
			{
				txtSetHotkey->setText(button.key_name);
				const auto host_joy_id = changed_prefs.jports[SelectedPort].id - JSEM_JOYS;
				didata* did = &di_joystick[host_joy_id];
				did->mapping.hotkey_button = button.button;
			}
		}
		else if (actionEvent.getSource() == cmdSetHotkeyClear)
		{
			txtSetHotkey->setText("");
			const auto host_joy_id = changed_prefs.jports[SelectedPort].id - JSEM_JOYS;
			didata* did = &di_joystick[host_joy_id];
			did->mapping.hotkey_button = SDL_CONTROLLER_BUTTON_INVALID;
		}

		RefreshPanelCustom();
	}
};

static GroupActionListener* grpActionListener;

class CustomActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto t = 0; t < SDL_CONTROLLER_BUTTON_MAX; t++)
		{
			if (actionEvent.getSource() == cboCustomButtonAction[t])
			{
				std::array<int, SDL_CONTROLLER_BUTTON_MAX> temp_button_map{};

				// get map
				switch (SelectedFunction)
				{
				case 0:
					temp_button_map = changed_prefs.jports[SelectedPort].amiberry_custom_none;
					break;
				case 1:
					temp_button_map = changed_prefs.jports[SelectedPort].amiberry_custom_hotkey;
					break;
				default:
					break;
				}

				// get the selected action from the drop-down, and 
				// push it into the 'temp map'
				temp_button_map[t] = remap_event_list[cboCustomButtonAction[t]->getSelected() - 1];

				// push map back into changed_prefs
				switch (SelectedFunction)
				{
				case 0:
					changed_prefs.jports[SelectedPort].amiberry_custom_none = temp_button_map;
					break;
				case 1:
					changed_prefs.jports[SelectedPort].amiberry_custom_hotkey = temp_button_map;
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

		for (auto t = 0; t < SDL_CONTROLLER_AXIS_MAX; t++)
		{
			if (actionEvent.getSource() == cboCustomAxisAction[t])
			{
				std::array<int, SDL_CONTROLLER_AXIS_MAX> temp_axis_map{};

				// get map
				switch (SelectedFunction)
				{
				case 0:
					temp_axis_map = changed_prefs.jports[SelectedPort].amiberry_custom_axis_none;
					break;
				case 1:
					temp_axis_map = changed_prefs.jports[SelectedPort].amiberry_custom_axis_hotkey;
					break;
				default:
					break;
				}

				// get the selected action from the drop-down
				// and push it into the temp map
				temp_axis_map[t] = remap_event_list[cboCustomAxisAction[t]->getSelected() - 1];

				// push map back to changed_prefs
				switch (SelectedFunction)
				{
				case 0:
					changed_prefs.jports[SelectedPort].amiberry_custom_axis_none = temp_axis_map;
					break;
				case 1:
					changed_prefs.jports[SelectedPort].amiberry_custom_axis_hotkey = temp_axis_map;
					break;
				default:
					break;
				}

				inputdevice_updateconfig(nullptr, &changed_prefs);
				RefreshPanelCustom();
			}
		}
	}
};

static CustomActionListener* customActionListener;

void InitPanelCustom(const config_category& category)
{
	int i;
	char tmp[255];

	if (CustomEventList.getNumberOfElements() == 0)
	{
		CustomEventList.add_element("None");

		for (int idx = 0; idx < remap_event_list_size; idx++)
		{
			const auto* const ie = inputdevice_get_eventinfo(remap_event_list[idx]);
			snprintf(tmp, 255, "%s", ie->name);
			CustomEventList.add_element(tmp);
		}
	}

	CustomEventList_HotKey = CustomEventList;
	CustomEventList_Menu = CustomEventList;
	CustomEventList_Quit = CustomEventList;
	CustomEventList_Reset = CustomEventList;

	CustomEventList_HotKey.swap_first_element("None (HotKey)");
	CustomEventList_Menu.swap_first_element("None (Menu)");
	CustomEventList_Quit.swap_first_element("None (Quit)");
	CustomEventList_Reset.swap_first_element("None (Reset)");

	customActionListener = new CustomActionListener();
	grpActionListener = new GroupActionListener();

	optPort0 = new gcn::RadioButton("0: Mouse", "radioportgroup");
	optPort0->setId("0: Mouse");
	optPort0->addActionListener(grpActionListener);
	optPort1 = new gcn::RadioButton("1: Joystick", "radioportgroup");
	optPort1->setId("1: Joystick");
	optPort1->addActionListener(grpActionListener);
	optPort2 = new gcn::RadioButton("2: Parallel 1", "radioportgroup");
	optPort2->setId("2: Parallel 1");
	optPort2->addActionListener(grpActionListener);
	optPort3 = new gcn::RadioButton("3: Parallel 2", "radioportgroup");
	optPort3->setId("3: Parallel 2");
	optPort3->addActionListener(grpActionListener);

	optMultiNone = new gcn::RadioButton("None", "radiomultigroup");
	optMultiNone->setId("None");
	optMultiNone->addActionListener(grpActionListener);
	optMultiSelect = new gcn::RadioButton("HotKey", "radiomultigroup");
	optMultiSelect->setId("HotKey");
	optMultiSelect->addActionListener(grpActionListener);

	grpPort = new gcn::Window("Joystick Port");
	grpPort->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpPort->setMovable(false);
	grpPort->add(optPort0, 10, 10);
	grpPort->add(optPort1, optPort0->getX() + optPort0->getWidth() + DISTANCE_NEXT_X, optPort0->getY());
	grpPort->add(optPort2, optPort1->getX() + optPort1->getWidth() + DISTANCE_NEXT_X, optPort0->getY());
	grpPort->add(optPort3, optPort2->getX() + optPort2->getWidth() + DISTANCE_NEXT_X, optPort0->getY());
	grpPort->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + optPort0->getHeight() * 2 + 5);
	grpPort->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpPort->setBaseColor(gui_baseCol);

	category.panel->add(grpPort);

	lblSetHotkey = new gcn::Label("Set Hotkey:");
	lblSetHotkey->setAlignment(gcn::Graphics::RIGHT);
	txtSetHotkey = new gcn::TextField();
	txtSetHotkey->setEnabled(false);
	txtSetHotkey->setSize(120, TEXTFIELD_HEIGHT);
	txtSetHotkey->setBackgroundColor(colTextboxBackground);
	cmdSetHotkey = new gcn::Button("...");
	cmdSetHotkey->setId("cmdSetHotkey");
	cmdSetHotkey->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSetHotkey->setBaseColor(gui_baseCol);
	cmdSetHotkey->addActionListener(grpActionListener);
	cmdSetHotkeyClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdSetHotkeyClear->setBaseColor(gui_baseCol);
	cmdSetHotkeyClear->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSetHotkeyClear->setId("cmdSetHotkeyClear");
	cmdSetHotkeyClear->addActionListener(grpActionListener);

	lblFunction = new gcn::Label("Function Key:");
	category.panel->add(lblFunction, DISTANCE_BORDER, grpPort->getY() + grpPort->getHeight() + DISTANCE_NEXT_Y);
	category.panel->add(optMultiNone, lblFunction->getX() + lblFunction->getWidth() + 8, lblFunction->getY());
	category.panel->add(optMultiSelect, optMultiNone->getX() + optMultiNone->getWidth() + DISTANCE_NEXT_X, optMultiNone->getY());
	category.panel->add(lblSetHotkey, optMultiSelect->getX() + optMultiSelect->getWidth() + DISTANCE_NEXT_X * 2, optMultiSelect->getY());
	category.panel->add(txtSetHotkey, lblSetHotkey->getX() + lblSetHotkey->getWidth() + 8, lblSetHotkey->getY());
	category.panel->add(cmdSetHotkey, txtSetHotkey->getX() + txtSetHotkey->getWidth() + 8, txtSetHotkey->getY());
	category.panel->add(cmdSetHotkeyClear, cmdSetHotkey->getX() + cmdSetHotkey->getWidth() + 8, cmdSetHotkey->getY());

	// the input-device should be listed
	lblPortInput = new gcn::Label("Input Device:");
	lblPortInput->setAlignment(gcn::Graphics::RIGHT);

	txtPortInput = new gcn::TextField();
	txtPortInput->setEnabled(false);
	txtPortInput->setBaseColor(gui_baseCol);
	txtPortInput->setBackgroundColor(colTextboxBackground);

	lblRetroarch = new gcn::Label("[-]");
	lblRetroarch->setAlignment(gcn::Graphics::LEFT);

	txtPortInput->setSize(grpPort->getWidth() - (lblPortInput->getWidth() + DISTANCE_NEXT_X * 2 + lblRetroarch->getWidth()),
		TEXTFIELD_HEIGHT);
	for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
		lblCustomButtonAction[i] = new gcn::Label(label_button_list[i]);
	for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
	{
		lblCustomButtonAction[i]->setSize(lblCustomButtonAction[14]->getWidth(), lblCustomButtonAction[14]->getHeight());
		lblCustomButtonAction[i]->setAlignment(gcn::Graphics::RIGHT);

		cboCustomButtonAction[i] = new gcn::DropDown(&CustomEventList);
		cboCustomButtonAction[i]->setSize(cboCustomButtonAction[i]->getWidth() * 2, cboCustomButtonAction[i]->getHeight());
		cboCustomButtonAction[i]->setBaseColor(gui_baseCol);
		cboCustomButtonAction[i]->setBackgroundColor(colTextboxBackground);

		std::string cbo_id = "cboCustomButtonAction" + to_string(i);
		cboCustomButtonAction[i]->setId(cbo_id);
		cboCustomButtonAction[i]->addActionListener(customActionListener);
	}

	for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
	{
		lblCustomAxisAction[i] = new gcn::Label(label_axis_list[i]);
		lblCustomAxisAction[i]->setSize(lblCustomButtonAction[14]->getWidth(), lblCustomButtonAction[14]->getHeight());
		lblCustomAxisAction[i]->setAlignment(gcn::Graphics::RIGHT);

		cboCustomAxisAction[i] = new gcn::DropDown(&CustomEventList);
		cboCustomAxisAction[i]->setSize(cboCustomAxisAction[i]->getWidth() * 2, cboCustomAxisAction[i]->getHeight());
		cboCustomAxisAction[i]->setBaseColor(gui_baseCol);
		cboCustomAxisAction[i]->setBackgroundColor(colTextboxBackground);

		std::string cbo_id = "cboCustomAxisAction" + to_string(i);
		cboCustomAxisAction[i]->setId(cbo_id);
		cboCustomAxisAction[i]->addActionListener(customActionListener);
	}


	auto posY = lblFunction->getY() + lblFunction->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblPortInput, DISTANCE_BORDER, posY);
	category.panel->add(txtPortInput, lblPortInput->getX() + lblPortInput->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(lblRetroarch, txtPortInput->getX() + txtPortInput->getWidth() + DISTANCE_NEXT_X, posY);
	posY = txtPortInput->getY() + txtPortInput->getHeight() + DISTANCE_NEXT_Y;

	// Column 1
	const auto column1 = 5;
	for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX / 2; i++)
	{
		category.panel->add(lblCustomButtonAction[i], column1, posY);
		category.panel->add(cboCustomButtonAction[i], lblCustomButtonAction[i]->getX() + lblCustomButtonAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

	for (i = 0; i < SDL_CONTROLLER_AXIS_MAX / 2 + 1; i++)
	{
		category.panel->add(lblCustomAxisAction[i], column1, posY);
		category.panel->add(cboCustomAxisAction[i], lblCustomAxisAction[i]->getX() + lblCustomAxisAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

	// Column 2
	posY = txtPortInput->getY() + txtPortInput->getHeight() + DISTANCE_NEXT_Y;
	const auto column2 = cboCustomButtonAction[0]->getX() + cboCustomButtonAction[0]->getWidth() + 4;
	for (i = SDL_CONTROLLER_BUTTON_MAX / 2; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		category.panel->add(lblCustomButtonAction[i], column2, posY);
		category.panel->add(cboCustomButtonAction[i], lblCustomButtonAction[i]->getX() + lblCustomButtonAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

	for (i = SDL_CONTROLLER_AXIS_MAX / 2 + 1; i < SDL_CONTROLLER_AXIS_MAX; i++)
	{
		category.panel->add(lblCustomAxisAction[i], column2, posY);
		category.panel->add(cboCustomAxisAction[i], lblCustomAxisAction[i]->getX() + lblCustomAxisAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

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
	delete lblFunction;

	delete lblSetHotkey;
	delete txtSetHotkey;
	delete cmdSetHotkey;
	delete cmdSetHotkeyClear;

	for (auto& i : lblCustomButtonAction)
		delete i;

	for (auto& i : cboCustomButtonAction)
		delete i;

	for (auto& i : lblCustomAxisAction)
		delete i;

	for (auto& i : cboCustomAxisAction)
		delete i;

	delete lblPortInput;
	delete txtPortInput;
	delete lblRetroarch;

	delete grpActionListener;
	delete customActionListener;
}

void RefreshPanelCustom()
{
	optPort0->setSelected(SelectedPort == 0);
	optPort1->setSelected(SelectedPort == 1);
	optPort2->setSelected(SelectedPort == 2);
	optPort3->setSelected(SelectedPort == 3);

	optMultiNone->setSelected(SelectedFunction == 0);
	optMultiSelect->setSelected(SelectedFunction == 1);

	// Refresh the drop-down section here
	// get map
	std::array<int, SDL_CONTROLLER_BUTTON_MAX> temp_button_map{};
	switch (SelectedFunction)
	{
	case 0:
		temp_button_map = changed_prefs.jports[SelectedPort].amiberry_custom_none;
		break;
	case 1:
		temp_button_map = changed_prefs.jports[SelectedPort].amiberry_custom_hotkey;
		break;
	default:
		break;
	}

	std::array<int, SDL_CONTROLLER_AXIS_MAX> temp_axis_map{};
	switch (SelectedFunction)
	{
	case 0:
		temp_axis_map = changed_prefs.jports[SelectedPort].amiberry_custom_axis_none;
		break;
	case 1:
		temp_axis_map = changed_prefs.jports[SelectedPort].amiberry_custom_axis_hotkey;
		break;
	default:
		break;
	}

	// update the joystick port and disable those not available
	if (changed_prefs.jports[SelectedPort].id >= JSEM_JOYS
		&& changed_prefs.jports[SelectedPort].id < JSEM_MICE - 1)
	{
		const auto host_joy_id = changed_prefs.jports[SelectedPort].id - JSEM_JOYS;
		didata* did = &di_joystick[host_joy_id];

		if (did->mapping.hotkey_button)
		{
			if (did->is_controller && !did->mapping.is_retroarch)
			{
				const auto hotkey_text = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(did->mapping.hotkey_button));
				if (hotkey_text != nullptr) txtSetHotkey->setText(hotkey_text);
				cmdSetHotkey->setEnabled(true);
				cmdSetHotkeyClear->setEnabled(true);
			}
			else
			{
				const std::string hotkey_text = to_string(did->mapping.hotkey_button);
				if (!hotkey_text.empty()) txtSetHotkey->setText(hotkey_text);
				if (did->mapping.is_retroarch)
				{
					// Disable editing buttons, as these are controlled by the retroarch config anyway
					cmdSetHotkey->setEnabled(false);
					cmdSetHotkeyClear->setEnabled(false);
				}
			}
		}

		for (auto n = 0; n < SDL_CONTROLLER_BUTTON_MAX; ++n)
		{
			const auto temp_button = did->mapping.button[n];

			// disable unmapped buttons
			cboCustomButtonAction[n]->setEnabled(temp_button > -1);
			lblCustomButtonAction[n]->setEnabled(temp_button > -1);

			// set hotkey/quit/reset/menu on NONE field (and disable hotkey)
			if (temp_button == did->mapping.hotkey_button
				&& temp_button != -1)
			{
				cboCustomButtonAction[n]->setListModel(&CustomEventList_HotKey);
				cboCustomButtonAction[n]->setEnabled(false);
				lblCustomButtonAction[n]->setEnabled(false);
			}

			else if (temp_button == did->mapping.quit_button
				&& temp_button != -1
				&& SelectedFunction == 1
				&& changed_prefs.use_retroarch_quit)
			{
				cboCustomButtonAction[n]->setListModel(&CustomEventList_Quit);
				cboCustomButtonAction[n]->setEnabled(false);
				lblCustomButtonAction[n]->setEnabled(false);
			}

			else if (temp_button == did->mapping.menu_button
				&& temp_button != -1
				&& SelectedFunction == 1
				&& changed_prefs.use_retroarch_menu)
			{
				cboCustomButtonAction[n]->setListModel(&CustomEventList_Menu);
				cboCustomButtonAction[n]->setEnabled(false);
				lblCustomButtonAction[n]->setEnabled(false);
			}

			else if (temp_button == did->mapping.reset_button
				&& temp_button != -1
				&& SelectedFunction == 1
				&& changed_prefs.use_retroarch_reset)
			{
				cboCustomButtonAction[n]->setListModel(&CustomEventList_Reset);
				cboCustomButtonAction[n]->setEnabled(false);
				lblCustomButtonAction[n]->setEnabled(false);
			}

			else
				cboCustomButtonAction[n]->setListModel(&CustomEventList);

			if (temp_button_map[n] > 0)
			{
				// Custom mapping found
				const auto x = find_in_array(remap_event_list, remap_event_list_size, temp_button_map[n]);
				cboCustomButtonAction[n]->setSelected(x + 1);
			}
			// Retroarch mapping is not the same, so we skip this to avoid incorrect actions showing in the GUI
			else if (did->mapping.is_retroarch)
			{
				cboCustomButtonAction[n]->setSelected(0);
			}
			else
			{
				// Default mapping
				const auto evt = default_button_mapping[temp_button];
				const auto x = find_in_array(remap_event_list, remap_event_list_size, evt);
				cboCustomButtonAction[n]->setSelected(x + 1);
			}
		}

		for (auto n = 0; n < SDL_CONTROLLER_AXIS_MAX; ++n)
		{
			const auto temp_axis = did->mapping.axis[n];

			// disable unmapped axes
			cboCustomAxisAction[n]->setEnabled(temp_axis > -1);
			lblCustomAxisAction[n]->setEnabled(temp_axis > -1);

			cboCustomAxisAction[n]->setListModel(&CustomEventList);

			if (temp_axis_map[n] > 0)
			{
				// Custom mapping found
				const auto x = find_in_array(remap_event_list, remap_event_list_size, temp_axis_map[n]);
				cboCustomAxisAction[n]->setSelected(x + 1);
			}
			else if (did->mapping.is_retroarch)
			{
				cboCustomAxisAction[n]->setSelected(0);
			}
			else
			{
				// Default mapping
				const auto evt = default_axis_mapping[temp_axis];
				const auto x = find_in_array(remap_event_list, remap_event_list_size, evt);
				cboCustomAxisAction[n]->setSelected(x + 1);
			}
		}

		if (did->mapping.number_of_hats > 0)
		{
			for (int i = SDL_CONTROLLER_BUTTON_DPAD_UP; i <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; i++)
			{
				cboCustomButtonAction[i]->setEnabled(true);
				lblCustomButtonAction[i]->setEnabled(true);
			}
		}

		if (did->mapping.is_retroarch)
		{
			lblRetroarch->setCaption("[R]");
			txtPortInput->setText(did->joystick_name);
		}
		else
		{
			lblRetroarch->setCaption("[N]");
			txtPortInput->setText(did->name);
		}
	}

	else
	{
		for (auto& n : cboCustomButtonAction)
		{
			n->setListModel(&CustomEventList);
			n->setEnabled(false);
		}
		for (auto& n : lblCustomButtonAction)
		{
			n->setEnabled(false);
		}
		for (auto& n : cboCustomAxisAction)
		{
			n->setListModel(&CustomEventList);
			n->setEnabled(false);
		}
		for (auto& n : lblCustomAxisAction)
		{
			n->setEnabled(false);
		}

		lblRetroarch->setCaption("[_]");
		const std::string text = "Not a valid Input Controller for Joystick Emulation.";
		txtPortInput->setText(text);
	}
}

bool HelpPanelCustom(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Set up Custom input actions for each Amiga port, such as Keyboard remapping,");
	helptext.emplace_back("or emulator functions.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Select the port which you wish to re-map with 'Joystick Port'.");
	helptext.emplace_back("The currently selected Input Device will then be displayed under 'Input Device'.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Buttons which are not available on this device (detected with RetroArch ");
	helptext.emplace_back("configuration files) are unavailable to remap.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The HotKey button (used for secondary functions) is also unavailable for custom options. ");
	helptext.emplace_back("The actions performed by pressing the HotKey with other buttons can also be remapped.");
	helptext.emplace_back("Pre-defined functions such as Quit/Reset/Menu will be displayed as the 'default' option.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The Function of the individual buttons are selectable via the marked drop-down boxes.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can also remap the game controllers D-Pad to act like the Left Analog stick.");
	return true;
}
