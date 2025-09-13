#include <cstdio>
#include <cstring>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

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

static gcn::Button* cmdSaveMapping;

static gcn::StringListModel CustomEventList;
static gcn::StringListModel CustomEventList_HotKey;
static gcn::StringListModel CustomEventList_Menu;
static gcn::StringListModel CustomEventList_Quit;
static gcn::StringListModel CustomEventList_Reset;
static gcn::StringListModel CustomEventList_Vkbd;

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
		else if (actionEvent.getSource()->getId() == "cmdSaveMapping")
		{
			//save the mapping to the controllers path with the controller name
			const std::string& controller_name = txtPortInput->getText();
			const std::string controller_path = get_controllers_path();
			const std::string controller_file = controller_path + controller_name + ".controller";
			const auto host_joy_id = changed_prefs.jports[SelectedPort].id - JSEM_JOYS;
			const didata* did = &di_joystick[host_joy_id];
			save_controller_mapping_to_file(did->mapping, controller_file);
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
		const auto host_joy_id = changed_prefs.jports[SelectedPort].id - JSEM_JOYS;
		didata* did = &di_joystick[host_joy_id];

		for (auto t = 0; t < SDL_CONTROLLER_BUTTON_MAX; t++)
		{
			if (actionEvent.getSource() == cboCustomButtonAction[t])
			{
				switch (SelectedFunction)
				{
				case 0:
					did->mapping.amiberry_custom_none[t] = remap_event_list[cboCustomButtonAction[t]->getSelected() - 1];
					break;
				case 1:
					did->mapping.amiberry_custom_hotkey[t] = remap_event_list[cboCustomButtonAction[t]->getSelected() - 1];
					break;
				default:
					break;
				}
			}
		}

		for (auto t = 0; t < SDL_CONTROLLER_AXIS_MAX; t++)
		{
			if (actionEvent.getSource() == cboCustomAxisAction[t])
			{
				switch (SelectedFunction)
				{
				case 0:
					did->mapping.amiberry_custom_axis_none[t] = remap_event_list[cboCustomAxisAction[t]->getSelected() - 1];
					break;
				case 1:
					did->mapping.amiberry_custom_axis_hotkey[t] = remap_event_list[cboCustomAxisAction[t]->getSelected() - 1];
					break;
				default:
					break;
				}
			}
		}

		inputdevice_updateconfig(nullptr, &changed_prefs);
		RefreshPanelCustom();
	}
};

static CustomActionListener* customActionListener;

void InitPanelCustom(const config_category& category)
{
	if (CustomEventList.getNumberOfElements() == 0)
	{
		CustomEventList.add("None");
		for (int idx = 0; idx < remap_event_list_size; idx++)
		{
			const auto* const ie = inputdevice_get_eventinfo(remap_event_list[idx]);
			CustomEventList.add(ie->name);
		}
	}

	CustomEventList_HotKey = CustomEventList;
	CustomEventList_Menu = CustomEventList;
	CustomEventList_Quit = CustomEventList;
	CustomEventList_Reset = CustomEventList;
	CustomEventList_Vkbd = CustomEventList;

	CustomEventList_HotKey.swap_first("In-Use (HotKey)");
	CustomEventList_Menu.swap_first("In-Use (Menu)");
	CustomEventList_Quit.swap_first("In-Use (Quit)");
	CustomEventList_Reset.swap_first("In-Use (Reset)");
	CustomEventList_Vkbd.swap_first("In-Use (VKBD)");

	customActionListener = new CustomActionListener();
	grpActionListener = new GroupActionListener();

	optPort0 = new gcn::RadioButton("0: Mouse", "radioportgroup");
	optPort0->setId("0: Mouse");
	optPort0->setBaseColor(gui_base_color);
	optPort0->setBackgroundColor(gui_background_color);
	optPort0->setForegroundColor(gui_foreground_color);
	optPort0->addActionListener(grpActionListener);
	optPort1 = new gcn::RadioButton("1: Joystick", "radioportgroup");
	optPort1->setId("1: Joystick");
	optPort1->setBaseColor(gui_base_color);
	optPort1->setBackgroundColor(gui_background_color);
	optPort1->setForegroundColor(gui_foreground_color);
	optPort1->addActionListener(grpActionListener);
	optPort2 = new gcn::RadioButton("2: Parallel 1", "radioportgroup");
	optPort2->setId("2: Parallel 1");
	optPort2->setBaseColor(gui_base_color);
	optPort2->setBackgroundColor(gui_background_color);
	optPort2->setForegroundColor(gui_foreground_color);
	optPort2->addActionListener(grpActionListener);
	optPort3 = new gcn::RadioButton("3: Parallel 2", "radioportgroup");
	optPort3->setId("3: Parallel 2");
	optPort3->setBaseColor(gui_base_color);
	optPort3->setBackgroundColor(gui_background_color);
	optPort3->setForegroundColor(gui_foreground_color);
	optPort3->addActionListener(grpActionListener);

	optMultiNone = new gcn::RadioButton("None", "radiomultigroup");
	optMultiNone->setId("None");
	optMultiNone->setBaseColor(gui_base_color);
	optMultiNone->setBackgroundColor(gui_background_color);
	optMultiNone->setForegroundColor(gui_foreground_color);
	optMultiNone->addActionListener(grpActionListener);
	optMultiSelect = new gcn::RadioButton("HotKey", "radiomultigroup");
	optMultiSelect->setId("HotKey");
	optMultiSelect->setBaseColor(gui_base_color);
	optMultiSelect->setBackgroundColor(gui_background_color);
	optMultiSelect->setForegroundColor(gui_foreground_color);
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
	grpPort->setBaseColor(gui_base_color);
	grpPort->setForegroundColor(gui_foreground_color);

	category.panel->add(grpPort);

	lblSetHotkey = new gcn::Label("Set Hotkey:");
	lblSetHotkey->setAlignment(gcn::Graphics::Right);
	txtSetHotkey = new gcn::TextField();
	txtSetHotkey->setEnabled(false);
	txtSetHotkey->setSize(120, TEXTFIELD_HEIGHT);
	txtSetHotkey->setBaseColor(gui_base_color);
	txtSetHotkey->setBackgroundColor(gui_background_color);
	txtSetHotkey->setForegroundColor(gui_foreground_color);
	cmdSetHotkey = new gcn::Button("...");
	cmdSetHotkey->setId("cmdSetHotkey");
	cmdSetHotkey->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSetHotkey->setBaseColor(gui_base_color);
	cmdSetHotkey->setForegroundColor(gui_foreground_color);
	cmdSetHotkey->addActionListener(grpActionListener);
	cmdSetHotkeyClear = new gcn::ImageButton(prefix_with_data_path("delete.png"));
	cmdSetHotkeyClear->setBaseColor(gui_base_color);
	cmdSetHotkeyClear->setForegroundColor(gui_foreground_color);
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
	lblPortInput->setAlignment(gcn::Graphics::Right);

	txtPortInput = new gcn::TextField();
	txtPortInput->setEnabled(false);
	txtPortInput->setBaseColor(gui_base_color);
	txtPortInput->setBackgroundColor(gui_background_color);
	txtPortInput->setForegroundColor(gui_foreground_color);

	lblRetroarch = new gcn::Label("[-]");
	lblRetroarch->setAlignment(gcn::Graphics::Left);

	cmdSaveMapping = new gcn::Button("Save as default mapping");
	cmdSaveMapping->setId("cmdSaveMapping");
	cmdSaveMapping->setSize(BUTTON_WIDTH * 3, BUTTON_HEIGHT);
	cmdSaveMapping->setBaseColor(gui_base_color);
	cmdSaveMapping->setForegroundColor(gui_foreground_color);
	cmdSaveMapping->addActionListener(grpActionListener);

	txtPortInput->setSize(grpPort->getWidth() - (lblPortInput->getWidth() + DISTANCE_NEXT_X * 2 + lblRetroarch->getWidth()),
		TEXTFIELD_HEIGHT);
	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
		lblCustomButtonAction[i] = new gcn::Label(label_button_list[i]);
	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
	{
		lblCustomButtonAction[i]->setSize(lblCustomButtonAction[14]->getWidth(), lblCustomButtonAction[14]->getHeight());
		lblCustomButtonAction[i]->setAlignment(gcn::Graphics::Right);

		cboCustomButtonAction[i] = new gcn::DropDown(&CustomEventList);
		cboCustomButtonAction[i]->setSize(cboCustomButtonAction[i]->getWidth() * 2, cboCustomButtonAction[i]->getHeight());
		cboCustomButtonAction[i]->setBaseColor(gui_base_color);
		cboCustomButtonAction[i]->setBackgroundColor(gui_background_color);
		cboCustomButtonAction[i]->setForegroundColor(gui_foreground_color);
		cboCustomButtonAction[i]->setSelectionColor(gui_selection_color);

		std::string cbo_id = "cboCustomButtonAction" + std::to_string(i);
		cboCustomButtonAction[i]->setId(cbo_id);
		cboCustomButtonAction[i]->addActionListener(customActionListener);
	}

	for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
	{
		lblCustomAxisAction[i] = new gcn::Label(label_axis_list[i]);
		lblCustomAxisAction[i]->setSize(lblCustomButtonAction[14]->getWidth(), lblCustomButtonAction[14]->getHeight());
		lblCustomAxisAction[i]->setAlignment(gcn::Graphics::Right);

		cboCustomAxisAction[i] = new gcn::DropDown(&CustomEventList);
		cboCustomAxisAction[i]->setSize(cboCustomAxisAction[i]->getWidth() * 2, cboCustomAxisAction[i]->getHeight());
		cboCustomAxisAction[i]->setBaseColor(gui_base_color);
		cboCustomAxisAction[i]->setBackgroundColor(gui_background_color);
		cboCustomAxisAction[i]->setForegroundColor(gui_foreground_color);
		cboCustomAxisAction[i]->setSelectionColor(gui_selection_color);

		std::string cbo_id = "cboCustomAxisAction" + std::to_string(i);
		cboCustomAxisAction[i]->setId(cbo_id);
		cboCustomAxisAction[i]->addActionListener(customActionListener);
	}


	auto posY = lblFunction->getY() + lblFunction->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblPortInput, DISTANCE_BORDER, posY);
	category.panel->add(txtPortInput, lblPortInput->getX() + lblPortInput->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(lblRetroarch, txtPortInput->getX() + txtPortInput->getWidth() + DISTANCE_NEXT_X, posY);
	posY = txtPortInput->getY() + txtPortInput->getHeight() + DISTANCE_NEXT_Y;

	// Column 1
	constexpr auto column1 = 5;
	for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX / 2; i++)
	{
		category.panel->add(lblCustomButtonAction[i], column1, posY);
		category.panel->add(cboCustomButtonAction[i], lblCustomButtonAction[i]->getX() + lblCustomButtonAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

	for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX / 2 + 1; i++)
	{
		category.panel->add(lblCustomAxisAction[i], column1, posY);
		category.panel->add(cboCustomAxisAction[i], lblCustomAxisAction[i]->getX() + lblCustomAxisAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

	// Column 2
	posY = txtPortInput->getY() + txtPortInput->getHeight() + DISTANCE_NEXT_Y;
	const auto column2 = cboCustomButtonAction[0]->getX() + cboCustomButtonAction[0]->getWidth() + 4;
	for (int i = SDL_CONTROLLER_BUTTON_MAX / 2; i < SDL_CONTROLLER_BUTTON_MAX; i++)
	{
		category.panel->add(lblCustomButtonAction[i], column2, posY);
		category.panel->add(cboCustomButtonAction[i], lblCustomButtonAction[i]->getX() + lblCustomButtonAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

	for (int i = SDL_CONTROLLER_AXIS_MAX / 2 + 1; i < SDL_CONTROLLER_AXIS_MAX; i++)
	{
		category.panel->add(lblCustomAxisAction[i], column2, posY);
		category.panel->add(cboCustomAxisAction[i], lblCustomAxisAction[i]->getX() + lblCustomAxisAction[i]->getWidth() + 4, posY);
		posY = posY + DROPDOWN_HEIGHT + 6;
	}

	category.panel->add(cmdSaveMapping, DISTANCE_BORDER, category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT);

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
	delete cmdSaveMapping;

	delete grpActionListener;
	delete customActionListener;
}

gcn::StringListModel* determine_list_model(const didata* did, const int button, const bool is_button_mapped)
{
	gcn::StringListModel* list_model;
	// set hotkey/quit/reset/menu on NONE field (and disable hotkey)
	if (button == did->mapping.hotkey_button && is_button_mapped)
	{
		list_model = &CustomEventList_HotKey;
	}
	else if (button == did->mapping.quit_button && is_button_mapped && SelectedFunction == 1 && changed_prefs.use_retroarch_quit)
	{
		list_model = &CustomEventList_Quit;
	}
	else if (button == did->mapping.menu_button && is_button_mapped && SelectedFunction == 1 && changed_prefs.use_retroarch_menu)
	{
		list_model = &CustomEventList_Menu;
	}
	else if (button == did->mapping.reset_button && is_button_mapped && SelectedFunction == 1 && changed_prefs.use_retroarch_reset)
	{
		list_model = &CustomEventList_Reset;
	}
	else if (button == did->mapping.vkbd_button && is_button_mapped && ((SelectedFunction == 1 && changed_prefs.use_retroarch_vkbd) || (SelectedFunction == 0 && !did->mapping.is_retroarch)))
	{
		list_model = &CustomEventList_Vkbd;
	}
	else
	{
		list_model = &CustomEventList;
	}
	return list_model;
}

void disable_all_controls()
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

	cmdSaveMapping->setEnabled(false);
}

void process_axis(const didata* did, const int n)
{
	std::array<int, SDL_CONTROLLER_AXIS_MAX> axis_map{};
	switch (SelectedFunction)
	{
	case 0:
		axis_map = did->mapping.amiberry_custom_axis_none;
		break;
	case 1:
		axis_map = did->mapping.amiberry_custom_axis_hotkey;
		break;
	default:
		break;
	}
	const auto axis = did->mapping.axis[n];

	// disable unmapped axes
	cboCustomAxisAction[n]->setEnabled(axis > -1);
	lblCustomAxisAction[n]->setEnabled(axis > -1);

	cboCustomAxisAction[n]->setListModel(&CustomEventList);

	if (axis_map[n] > 0)
	{
		// Custom mapping found
		const auto x = find_in_array(remap_event_list, remap_event_list_size, axis_map[n]);
		cboCustomAxisAction[n]->setSelected(x + 1);
	}
	else if (did->mapping.is_retroarch)
	{
		cboCustomAxisAction[n]->setSelected(0);
	}
	else
	{
		// Default mapping
		const auto evt = default_axis_mapping[axis];
		const auto x = find_in_array(remap_event_list, remap_event_list_size, evt);
		cboCustomAxisAction[n]->setSelected(x + 1);
	}
}

void update_button_action_selection(const didata* did, const int n, const array<int, 21>::value_type button)
{
	std::array<int, SDL_CONTROLLER_BUTTON_MAX> button_map{};
	switch (SelectedFunction)
	{
	case 0:
		button_map = did->mapping.amiberry_custom_none;
		break;
	case 1:
		button_map = did->mapping.amiberry_custom_hotkey;
		break;
	default:
		break;
	}

	if (button_map[n] > 0)
	{
		// Custom mapping found
		const auto x = find_in_array(remap_event_list, remap_event_list_size, button_map[n]);
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
		const auto evt = default_button_mapping[button];
		const auto x = find_in_array(remap_event_list, remap_event_list_size, evt);
		cboCustomButtonAction[n]->setSelected(x + 1);
	}
}

void update_selected_port()
{
	optPort0->setSelected(SelectedPort == 0);
	optPort1->setSelected(SelectedPort == 1);
	optPort2->setSelected(SelectedPort == 2);
	optPort3->setSelected(SelectedPort == 3);

	optMultiNone->setSelected(SelectedFunction == 0);
	optMultiSelect->setSelected(SelectedFunction == 1);
}

bool is_valid_joystick(const int id)
{
	return id >= JSEM_JOYS && id < JSEM_MICE - 1;
}

void update_hotkey_button(didata* did)
{
	if (did->mapping.hotkey_button)
	{
		// Check if the controller is not using retroarch mapping
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
}

void process_button(const didata* did, const int n)
{
	const auto button = did->mapping.button[n];

	// disable unmapped buttons
	const bool is_button_mapped = button > -1;
	cboCustomButtonAction[n]->setEnabled(is_button_mapped);
	lblCustomButtonAction[n]->setEnabled(is_button_mapped);

	gcn::StringListModel* list_model = determine_list_model(did, button, is_button_mapped);
	cboCustomButtonAction[n]->setListModel(list_model);
	if (list_model != &CustomEventList)
	{
		cboCustomButtonAction[n]->setEnabled(false);
		lblCustomButtonAction[n]->setEnabled(false);
	}

	update_button_action_selection(did, n, button);
}

void RefreshPanelCustom()
{
	update_selected_port();

	const auto host_joy_id = changed_prefs.jports[SelectedPort].id - JSEM_JOYS;
	didata* did = &di_joystick[host_joy_id];

	// Check if the selected port is a valid joystick
	if (is_valid_joystick(changed_prefs.jports[SelectedPort].id))
	{
		// Check if the hotkey button is set
		update_hotkey_button(did);

		// Process each button
		for (auto n = 0; n < SDL_CONTROLLER_BUTTON_MAX; ++n)
		{
			process_button(did, n);
		}

		// Process each axis
		for (auto n = 0; n < SDL_CONTROLLER_AXIS_MAX; ++n)
		{
			process_axis(did, n);
		}

		// Enable DPad buttons if hats are present
		if (did->mapping.number_of_hats > 0)
		{
			for (int i = SDL_CONTROLLER_BUTTON_DPAD_UP; i <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; i++)
			{
				cboCustomButtonAction[i]->setEnabled(true);
				lblCustomButtonAction[i]->setEnabled(true);
			}
		}

		// Set controller name and enable save button if not using retroarch mapping
		if (did->mapping.is_retroarch)
		{
			lblRetroarch->setCaption("[R]");
			txtPortInput->setText(did->joystick_name);
		}
		else
		{
			lblRetroarch->setCaption("[N]");
			txtPortInput->setText(did->name);
			cmdSaveMapping->setEnabled(true);
		}
	}
	else
	{
		disable_all_controls();
	}
}

bool HelpPanelCustom(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("In this panel you can customize the controls for your gamepad/joystick and assign any");
	helptext.emplace_back("of the several available input events to each button. Of course, this panel requires");
	helptext.emplace_back("a joystick or controller to be connected, in order for the dropdowns to be enabled.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The available customization options include functions such as:");
	helptext.emplace_back(" ");
	helptext.emplace_back("- Standard joystick inputs");
	helptext.emplace_back("- Parallel port Joystick inputs");
	helptext.emplace_back("- CD32 gamepad inputs");
	helptext.emplace_back("- All keyboard keys");
	helptext.emplace_back("- Various emulator functions (i.e. Autocrop, Quit, Reset, Enter GUI)");
	helptext.emplace_back(" ");
	helptext.emplace_back("Furthermore, this panel will display different information depending on if you are using");
	helptext.emplace_back("a RetroArch mapping (ie; like RetroPie) or not. If this is a RetroArch mapping, then a");
	helptext.emplace_back("\"[R]\" indicator will appear next to the controller's name to show that.");
	helptext.emplace_back(" ");
	helptext.emplace_back("When Retroarch mapping is used, the already mapped/assigned buttons will be disabled and");
	helptext.emplace_back("you cannot change them from this panel. On standard (non-Retroarch) mappings, a \"[N]\"");
	helptext.emplace_back("indicator will appear, and you can change the mappings to any of the options available.");
	helptext.emplace_back(" ");
	helptext.emplace_back("To change a mapping option, select the port which you wish to re-map, and the functions");
	helptext.emplace_back("of the individual buttons are selectable via the marked drop-down boxes.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can also assign a Hotkey which can be used in combination with the buttons, which");
	helptext.emplace_back("provides an extra set of custom controls. The controls used while the Hotkey button is");
	helptext.emplace_back(R"(pressed can be seen if you switch from "None" to the "Hotkey" option. The Hotkey itself)");
	helptext.emplace_back("can also be assigned from here as well.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can also remap the game controller D-Pad to act like the Left Analog joystick.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(You can use the "Save as default mapping" button, which will save the current mapping)");
	helptext.emplace_back("as the default for the current controller. It will be automatically applied on startup");
	helptext.emplace_back("for this controller. This option is only available in non-Retroarch environments.");
	return true;
}
