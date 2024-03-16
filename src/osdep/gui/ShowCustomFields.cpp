#include <cstdio>
#include <cstring>

#include <guisan.hpp>
#include <guisan/sdl.hpp>

#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"

#include "options.h"
#include "amiberry_input.h"

enum
{
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 600
};

static bool dialog_finished = false;

static int custom1_number = 1;
static int custom2_number = 1;
static int custom3_number = 1;
static int custom4_number = 1;
static int custom5_number = 1;

static gcn::StringListModel custom_list[5];

static gcn::Window* wndShowCustomFields;
static gcn::Button* cmdOK;

struct custom_field {
	std::vector<gcn::Label*> lbl;
	std::vector<gcn::CheckBox*> chk;
	std::vector<gcn::DropDown*> cbo;
};

custom_field customField1;
custom_field customField2;
custom_field customField3;
custom_field customField4;
custom_field customField5;

void create_custom_field(custom_field& customField, int custom_number, const std::string& customName, const whdload_custom& custom_field, int& pos_y)
{
	constexpr int textfield_width = 350;
	constexpr int pos_x1 = DISTANCE_BORDER;
	constexpr int pos_x2 = 150;

	for (int i = 0; i < custom_number; i++) {
		std::string id;

		customField.lbl.emplace_back(new gcn::Label(customName + ":"));
		customField.lbl[i]->setPosition(pos_x1, pos_y);

		wndShowCustomFields->add(customField.lbl[i]);

		if (custom_field.type == bit_type)
		{
			customField.chk.emplace_back(new gcn::CheckBox(custom_field.label_bit_pairs[i].first));
			// TODO not correct, it shouldn't use the value coming from the XML
			customField.chk[i]->setSelected(custom_field.label_bit_pairs[i].second);

			id = "chkCustomField_" + std::to_string(i);
			customField.chk[i]->setId(id);

			customField.chk[i]->setPosition(pos_x2, pos_y);
			pos_y += customField.chk[i]->getHeight() + 8;

			wndShowCustomFields->add(customField.chk[i]);
		}
		else if (custom_field.type == bool_type)
		{
			customField.chk.emplace_back(new gcn::CheckBox(custom_field.caption));
			customField.chk[i]->setSelected(custom_field.value);

			id = "chkCustomField_" + std::to_string(i);
			customField.chk[i]->setId(id);

			customField.chk[i]->setPosition(pos_x2, pos_y);
			pos_y += customField.chk[i]->getHeight() + 8;

			wndShowCustomFields->add(customField.chk[i]);
		}
		else if (custom_field.type == list_type)
		{
			customField.lbl[i]->setCaption(custom_field.caption);

			for (const auto& label : custom_field.labels)
			{
				custom_list[i].add(label);
			}

			id = "cboCustomField_" + std::to_string(i);
			customField.cbo.emplace_back(new gcn::DropDown(&custom_list[i]));
			customField.cbo[i]->setId(id);
			customField.cbo[i]->setSize(textfield_width, customField.cbo[i]->getHeight());
			customField.cbo[i]->setBaseColor(gui_baseCol);
			customField.cbo[i]->setBackgroundColor(colTextboxBackground);

			customField.cbo[i]->setPosition(pos_x2, pos_y);
			pos_y += customField.cbo[i]->getHeight() + 8;

			wndShowCustomFields->add(customField.cbo[i]);
		}
		else
		{
			pos_y += customField.lbl[i]->getHeight() + 8;
		}
	}
}

void delete_custom_field(custom_field& customField) {
	for (const auto& lbl : customField.lbl) {
		delete lbl;
	}
	customField.lbl.clear();

	for (const auto& chk : customField.chk) {
		delete chk;
	}
	customField.chk.clear();

	for (const auto& cbo : customField.cbo) {
		delete cbo;
	}
	customField.cbo.clear();
}

class ShowCustomFieldsActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
			dialog_finished = true;

	}
};

static ShowCustomFieldsActionListener* showCustomFieldsActionListener;


static void InitShowCustomFields()
{
	wndShowCustomFields = new gcn::Window("Custom Fields");
	wndShowCustomFields->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowCustomFields->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowCustomFields->setBaseColor(gui_baseCol);
	wndShowCustomFields->setTitleBarHeight(TITLEBAR_HEIGHT);

	showCustomFieldsActionListener = new ShowCustomFieldsActionListener();
	int pos_y = DISTANCE_BORDER;

	for (auto& i : custom_list)
	{
		i.clear();
	}

	create_custom_field(customField1, custom1_number, "Custom1", whdload_prefs.selected_slave.custom1, pos_y);
	create_custom_field(customField2, custom2_number, "Custom2", whdload_prefs.selected_slave.custom2, pos_y);
	create_custom_field(customField3, custom3_number, "Custom3", whdload_prefs.selected_slave.custom3, pos_y);
	create_custom_field(customField4, custom4_number, "Custom4", whdload_prefs.selected_slave.custom4, pos_y);
	create_custom_field(customField5, custom5_number, "Custom5", whdload_prefs.selected_slave.custom5, pos_y);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(showCustomFieldsActionListener);

	wndShowCustomFields->add(cmdOK);

	gui_top->add(wndShowCustomFields);

	wndShowCustomFields->requestModalFocus();
	focus_bug_workaround(wndShowCustomFields);
	cmdOK->requestFocus();
}

static void ExitShowCustomFields()
{
	wndShowCustomFields->releaseModalFocus();
	gui_top->remove(wndShowCustomFields);

	delete cmdOK;

	delete_custom_field(customField1);
	delete_custom_field(customField2);
	delete_custom_field(customField3);
	delete_custom_field(customField4);
	delete_custom_field(customField5);

	delete showCustomFieldsActionListener;
	delete wndShowCustomFields;
}

static void ShowCustomFieldsLoop()
{
	const AmigaMonitor* mon = &AMonitors[0];

	auto got_event = 0;
	SDL_Event event;
	SDL_Event touch_event;
	didata* did = &di_joystick[0];
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			got_event = 1;
			switch (event.key.keysym.sym)
			{
			case VK_ESCAPE:
				dialog_finished = true;
				break;

			case VK_Blue:
			case VK_Green:
			case SDLK_RETURN:
				event.key.keysym.sym = SDLK_RETURN;
				gui_input->pushInput(event); // Fire key down
				event.type = SDL_KEYUP; // and the key up
				break;
			default:
				break;
			}
			break;

		case SDL_JOYBUTTONDOWN:
			if (gui_joystick)
			{
				got_event = 1;
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_A]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_B]))
				{
					PushFakeKey(SDLK_RETURN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_X]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_Y]) ||
					SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_START]))
				{
					dialog_finished = true;
					break;
				}
			}
			break;

		case SDL_FINGERDOWN:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONDOWN;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_PRESSED;

			touch_event.button.x = gui_graphics->getTarget()->w * static_cast<int>(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * static_cast<int>(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;

			touch_event.button.x = gui_graphics->getTarget()->w * static_cast<int>(event.tfinger.x);
			touch_event.button.y = gui_graphics->getTarget()->h * static_cast<int>(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;

			touch_event.motion.x = gui_graphics->getTarget()->w * static_cast<int>(event.tfinger.x);
			touch_event.motion.y = gui_graphics->getTarget()->h * static_cast<int>(event.tfinger.y);

			gui_input->pushInput(touch_event);
			break;

		case SDL_MOUSEWHEEL:
			got_event = 1;
			if (event.wheel.y > 0)
			{
				for (auto z = 0; z < event.wheel.y; ++z)
				{
					PushFakeKey(SDLK_UP);
				}
			}
			else if (event.wheel.y < 0)
			{
				for (auto z = 0; z > event.wheel.y; --z)
				{
					PushFakeKey(SDLK_DOWN);
				}
			}
			break;

		case SDL_KEYUP:
		case SDL_JOYBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_RENDER_TARGETS_RESET:
		case SDL_RENDER_DEVICE_RESET:
		case SDL_WINDOWEVENT:
		case SDL_DISPLAYEVENT:
		case SDL_SYSWMEVENT:
			got_event = 1;
			break;

		default:
			break;
		}
		//-------------------------------------------------
		// Send event to guisan-controls
		//-------------------------------------------------
		gui_input->pushInput(event);
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();

		SDL_RenderClear(mon->gui_renderer);

		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

void ShowCustomFields()
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialog_finished = false;

	if (whdload_prefs.selected_slave.custom1.type == bit_type)
		custom1_number = static_cast<int>(whdload_prefs.selected_slave.custom1.label_bit_pairs.size());
	if (whdload_prefs.selected_slave.custom2.type == bit_type)
		custom2_number = static_cast<int>(whdload_prefs.selected_slave.custom2.label_bit_pairs.size());
	if (whdload_prefs.selected_slave.custom3.type == bit_type)
		custom3_number = static_cast<int>(whdload_prefs.selected_slave.custom3.label_bit_pairs.size());
	if (whdload_prefs.selected_slave.custom4.type == bit_type)
		custom4_number = static_cast<int>(whdload_prefs.selected_slave.custom4.label_bit_pairs.size());
	if (whdload_prefs.selected_slave.custom5.type == bit_type)
		custom5_number = static_cast<int>(whdload_prefs.selected_slave.custom5.label_bit_pairs.size());

	InitShowCustomFields();

	wndShowCustomFields->setCaption("Custom Fields");
	cmdOK->setCaption("Ok");

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialog_finished)
	{
		const auto start = SDL_GetPerformanceCounter();
		ShowCustomFieldsLoop();
		cap_fps(start);
	}

	ExitShowCustomFields();
}