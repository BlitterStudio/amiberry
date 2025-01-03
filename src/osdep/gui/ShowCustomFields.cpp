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
static int custom_number[5];
static gcn::StringListModel custom_list[5];

static gcn::Window* wndShowCustomFields;
static gcn::Button* cmdOK;

struct custom_widget {
	std::vector<gcn::Label*> lbl;
	std::vector<gcn::CheckBox*> boolean;
	std::vector<gcn::CheckBox*> bit;
	std::vector<gcn::DropDown*> list;
};

custom_widget customWidget1;
custom_widget customWidget2;
custom_widget customWidget3;
custom_widget customWidget4;
custom_widget customWidget5;

constexpr std::array<custom_widget*, 5> customWidgets = { &customWidget1, &customWidget2, &customWidget3, &customWidget4, &customWidget5 };
constexpr std::array<whdload_custom*, 5> customFields = { &whdload_prefs.selected_slave.custom1, &whdload_prefs.selected_slave.custom2, &whdload_prefs.selected_slave.custom3, &whdload_prefs.selected_slave.custom4, &whdload_prefs.selected_slave.custom5 };

static int set_bit(const int value, const int bit_position) {
	return value | (1 << bit_position);
}

static int clear_bit(const int value, const int bit_position) {
	return value & ~(1 << bit_position);
}

static bool is_bit_set(const int num, const int bit) {
	return (num & (1 << bit)) != 0;
}

class ShowCustomFieldsActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto source = actionEvent.getSource();

		if (source == cmdOK)
			dialog_finished = true;
		else
		{
			for (int j = 0; j < 5; ++j)
			{
				for (int i = 0; i < custom_number[j]; i++)
				{
					if (!customWidgets[j]->list.empty() && customFields[j]->type == list_type)
					{
						if (source == customWidgets[j]->list[i])
						{
							customFields[j]->value = customWidgets[j]->list[i]->getSelected();
							write_log("Custom field %d value updated to: %d\n", i, customFields[j]->value);
						}
					}
					else if (!customWidgets[j]->boolean.empty() && customFields[j]->type == bool_type)
					{
						if (source == customWidgets[j]->boolean[i])
						{
							customFields[j]->value = customWidgets[j]->boolean[i]->isSelected();
							write_log("Custom field %d value updated to: %d\n", i, customFields[j]->value);
						}
					}
					else if (!customWidgets[j]->bit.empty() && customFields[j]->type == bit_type)
					{
						if (source == customWidgets[j]->bit[i])
						{
							customFields[j]->value = customWidgets[j]->bit[i]->isSelected()
							? set_bit(customFields[j]->value, customFields[j]->label_bit_pairs[i].second)
							: clear_bit(customFields[j]->value, customFields[j]->label_bit_pairs[i].second);
							write_log("Custom field %d value updated to: %d\n", i, customFields[j]->value);
						}
					}
				}
			}
		}
	}
};

static ShowCustomFieldsActionListener* showCustomFieldsActionListener;

void create_custom_field(custom_widget& widget, const int number, const std::string& caption, const whdload_custom& custom_field, int& pos_y, const int custom_list_index)
{
	constexpr int pos_x1 = DISTANCE_BORDER;

	for (int i = 0; i < number; i++) {
		std::string id;

		auto label = new gcn::Label(caption + ":");
		label->setPosition(pos_x1, pos_y);
		widget.lbl.emplace_back(label);
		wndShowCustomFields->add(label);

		int pos_x2 = label->getWidth() + 15;

		switch (custom_field.type) {
		case bit_type: {
			auto checkbox = new gcn::CheckBox(custom_field.label_bit_pairs[i].first);
			checkbox->setSelected(is_bit_set(custom_field.value, custom_field.label_bit_pairs[i].second));
			checkbox->addActionListener(showCustomFieldsActionListener);
			checkbox->setId("chkCustomFieldBit_" + std::to_string(i));
			checkbox->setForegroundColor(gui_foreground_color);
			checkbox->setBaseColor(gui_base_color);
			checkbox->setBackgroundColor(gui_background_color);
			checkbox->setPosition(pos_x2, pos_y);
			widget.bit.emplace_back(checkbox);
			wndShowCustomFields->add(checkbox);
			pos_y += checkbox->getHeight() + 8;
			break;
		}
		case bool_type: {
			auto checkbox = new gcn::CheckBox(custom_field.caption);
			checkbox->setSelected(custom_field.value);
			checkbox->addActionListener(showCustomFieldsActionListener);
			checkbox->setId("chkCustomFieldBool_" + std::to_string(i));
			checkbox->setForegroundColor(gui_foreground_color);
			checkbox->setBaseColor(gui_base_color);
			checkbox->setBackgroundColor(gui_background_color);
			checkbox->setPosition(pos_x2, pos_y);
			widget.boolean.emplace_back(checkbox);
			wndShowCustomFields->add(checkbox);
			pos_y += checkbox->getHeight() + 8;
			break;
		}
		case list_type: {
			constexpr int textfield_width = 300;
			label->setCaption(custom_field.caption);
			label->adjustSize();
			pos_x2 = textfield_width + 15;
			label->setPosition(pos_x2, pos_y);

			for (const auto& item : custom_field.labels)
			{
				custom_list[custom_list_index].add(item);
			}
			auto dropdown = new gcn::DropDown(&custom_list[custom_list_index]);
			dropdown->setId("cboCustomFieldList_" + std::to_string(i));
			dropdown->setSize(textfield_width, dropdown->getHeight());
			dropdown->setBaseColor(gui_base_color);
			dropdown->setBackgroundColor(gui_background_color);
			dropdown->setForegroundColor(gui_foreground_color);
			dropdown->setSelectionColor(gui_selection_color);
			dropdown->addActionListener(showCustomFieldsActionListener);
			dropdown->setPosition(pos_x1, pos_y);
			widget.list.emplace_back(dropdown);
			wndShowCustomFields->add(dropdown);
			pos_y += dropdown->getHeight() + 8;
			break;
		}
		default:
			pos_y += label->getHeight() + 8;
			break;
		}
	}
}

void delete_custom_field(custom_widget& customField) {
	for (const auto& lbl : customField.lbl) {
		delete lbl;
	}
	customField.lbl.clear();

	for (const auto& chk : customField.boolean) {
		delete chk;
	}
	customField.boolean.clear();

	for (const auto& chk : customField.bit) {
		delete chk;
	}
	customField.bit.clear();

	for (const auto& cbo : customField.list) {
		delete cbo;
	}
	customField.list.clear();
}

static void InitShowCustomFields()
{
	wndShowCustomFields = new gcn::Window("Custom Fields");
	wndShowCustomFields->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndShowCustomFields->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndShowCustomFields->setBaseColor(gui_base_color);
	wndShowCustomFields->setForegroundColor(gui_foreground_color);
	wndShowCustomFields->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndShowCustomFields->setMovable(false);

	showCustomFieldsActionListener = new ShowCustomFieldsActionListener();
	int pos_y = DISTANCE_BORDER;

	for (auto& i : custom_list)
	{
		i.clear();
	}

	for (int i = 0; i < 5; ++i) {
		create_custom_field(*customWidgets[i], custom_number[i], "Custom" + std::to_string(i + 1), *customFields[i], pos_y, i);
	}
	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
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

	delete_custom_field(customWidget1);
	delete_custom_field(customWidget2);
	delete_custom_field(customWidget3);
	delete_custom_field(customWidget4);
	delete_custom_field(customWidget5);

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

    // Initialize custom_number to 1
    std::fill(std::begin(custom_number), std::end(custom_number), 1);

    // If the custom field is a bit field, set the custom_number to the number of pairs
    if (whdload_prefs.selected_slave.custom1.type == bit_type)
        custom_number[0] = static_cast<int>(whdload_prefs.selected_slave.custom1.label_bit_pairs.size());
    if (whdload_prefs.selected_slave.custom2.type == bit_type)
        custom_number[1] = static_cast<int>(whdload_prefs.selected_slave.custom2.label_bit_pairs.size());
    if (whdload_prefs.selected_slave.custom3.type == bit_type)
        custom_number[2] = static_cast<int>(whdload_prefs.selected_slave.custom3.label_bit_pairs.size());
    if (whdload_prefs.selected_slave.custom4.type == bit_type)
        custom_number[3] = static_cast<int>(whdload_prefs.selected_slave.custom4.label_bit_pairs.size());
    if (whdload_prefs.selected_slave.custom5.type == bit_type)
        custom_number[4] = static_cast<int>(whdload_prefs.selected_slave.custom5.label_bit_pairs.size());

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

	create_startup_sequence();
	ExitShowCustomFields();
}