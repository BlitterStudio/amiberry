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

static std::vector<gcn::Label*> lblCustomField1{};
static std::vector<gcn::CheckBox*> chkCustomField1{};
static std::vector<gcn::DropDown*> cboCustomField1{};

static std::vector<gcn::Label*> lblCustomField2{};
static std::vector<gcn::CheckBox*> chkCustomField2{};
static std::vector<gcn::DropDown*> cboCustomField2{};

static std::vector<gcn::Label*> lblCustomField3{};
static std::vector<gcn::CheckBox*> chkCustomField3{};
static std::vector<gcn::DropDown*> cboCustomField3{};

static std::vector<gcn::Label*> lblCustomField4{};
static std::vector<gcn::CheckBox*> chkCustomField4{};
static std::vector<gcn::DropDown*> cboCustomField4{};

static std::vector<gcn::Label*> lblCustomField5{};
static std::vector<gcn::CheckBox*> chkCustomField5{};
static std::vector<gcn::DropDown*> cboCustomField5{};


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
	constexpr int textfield_width = 350;
	constexpr int pos_x1 = DISTANCE_BORDER;
	constexpr int pos_x2 = 150;
	int pos_y = DISTANCE_BORDER;

	for (auto& i : custom_list)
	{
		i.clear();
	}

	for (int i = 0; i < custom1_number; i++)
	{
		std::string id;

		lblCustomField1.emplace_back(new gcn::Label("Custom1:"));
		lblCustomField1[i]->setPosition(pos_x1, pos_y);

		wndShowCustomFields->add(lblCustomField1[i]);

		if (whdload_prefs.selected_slave.custom1.type == bit_type)
		{
			chkCustomField1.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom1.label_bit_pairs[i].first));
			// TODO not correct, it shouldn't use the value coming from the XML
			chkCustomField1[i]->setSelected(whdload_prefs.selected_slave.custom1.label_bit_pairs[i].second);

			id = "chkCustomField1_" + std::to_string(i);
			chkCustomField1[i]->setId(id);

			chkCustomField1[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField1[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField1[i]);
		}
		else if (whdload_prefs.selected_slave.custom1.type == bool_type)
		{
			chkCustomField1.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom1.caption));
			chkCustomField1[i]->setSelected(whdload_prefs.selected_slave.custom1.value);

			id = "chkCustomField1_" + std::to_string(i);
			chkCustomField1[i]->setId(id);

			chkCustomField1[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField1[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField1[i]);
		}
		else if (whdload_prefs.selected_slave.custom1.type == list_type)
		{
			lblCustomField1[i]->setCaption(whdload_prefs.selected_slave.custom1.caption);

			for (const auto& label : whdload_prefs.selected_slave.custom1.labels)
			{
				custom_list[i].add(label);
			}

			id = "cboCustomField1_" + std::to_string(i);
			cboCustomField1.emplace_back(new gcn::DropDown(&custom_list[i]));
			cboCustomField1[i]->setId(id);
			cboCustomField1[i]->setSize(textfield_width, cboCustomField1[i]->getHeight());
			cboCustomField1[i]->setBaseColor(gui_baseCol);
			cboCustomField1[i]->setBackgroundColor(colTextboxBackground);

			cboCustomField1[i]->setPosition(pos_x2, pos_y);
			pos_y += cboCustomField1[i]->getHeight() + 8;

			wndShowCustomFields->add(cboCustomField1[i]);
		}
		else
		{
			pos_y += lblCustomField1[i]->getHeight() + 8;
		}
	}

	for (int i = 0; i < custom2_number; i++)
	{
		std::string id;

		lblCustomField2.emplace_back(new gcn::Label("Custom2:"));
		lblCustomField2[i]->setPosition(pos_x1, pos_y);

		wndShowCustomFields->add(lblCustomField2[i]);

		if (whdload_prefs.selected_slave.custom2.type == bit_type)
		{
			chkCustomField2.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom2.label_bit_pairs[i].first));
			// TODO not correct, it shouldn't use the value coming from the XML
			chkCustomField2[i]->setSelected(whdload_prefs.selected_slave.custom2.label_bit_pairs[i].second);

			id = "chkCustomField2_" + std::to_string(i);
			chkCustomField2[i]->setId(id);

			chkCustomField2[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField2[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField2[i]);
		}
		else if (whdload_prefs.selected_slave.custom2.type == bool_type)
		{
			chkCustomField2.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom2.caption));
			chkCustomField2[i]->setSelected(whdload_prefs.selected_slave.custom2.value);

			id = "chkCustomField2_" + std::to_string(i);
			chkCustomField2[i]->setId(id);

			chkCustomField2[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField2[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField2[i]);
		}
		else if (whdload_prefs.selected_slave.custom2.type == list_type)
		{
			lblCustomField2[i]->setCaption(whdload_prefs.selected_slave.custom2.caption);

			for (const auto& label : whdload_prefs.selected_slave.custom2.labels)
			{
				custom_list[i].add(label);
			}

			id = "cboCustomField2_" + std::to_string(i);
			cboCustomField2.emplace_back(new gcn::DropDown(&custom_list[i]));
			cboCustomField2[i]->setId(id);
			cboCustomField2[i]->setSize(textfield_width, cboCustomField2[i]->getHeight());
			cboCustomField2[i]->setBaseColor(gui_baseCol);
			cboCustomField2[i]->setBackgroundColor(colTextboxBackground);

			cboCustomField2[i]->setPosition(pos_x2, pos_y);
			pos_y += cboCustomField2[i]->getHeight() + 8;

			wndShowCustomFields->add(cboCustomField2[i]);
		}
		else
		{
			pos_y += lblCustomField2[i]->getHeight() + 8;
		}
	}

	for (int i = 0; i < custom3_number; i++)
	{
		std::string id;

		lblCustomField3.emplace_back(new gcn::Label("Custom3:"));
		lblCustomField3[i]->setPosition(pos_x1, pos_y);

		wndShowCustomFields->add(lblCustomField3[i]);

		if (whdload_prefs.selected_slave.custom3.type == bit_type)
		{
			chkCustomField3.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom3.label_bit_pairs[i].first));
			// TODO not correct, it shouldn't use the value coming from the XML
			chkCustomField3[i]->setSelected(whdload_prefs.selected_slave.custom3.label_bit_pairs[i].second);

			id = "chkCustomField3_" + std::to_string(i);
			chkCustomField3[i]->setId(id);

			chkCustomField3[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField3[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField3[i]);
		}
		else if (whdload_prefs.selected_slave.custom3.type == bool_type)
		{
			chkCustomField3.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom3.caption));
			chkCustomField3[i]->setSelected(whdload_prefs.selected_slave.custom3.value);

			id = "chkCustomField3_" + std::to_string(i);
			chkCustomField3[i]->setId(id);

			chkCustomField3[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField3[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField3[i]);
		}
		else if (whdload_prefs.selected_slave.custom3.type == list_type)
		{
			lblCustomField3[i]->setCaption(whdload_prefs.selected_slave.custom3.caption);

			for (const auto& label : whdload_prefs.selected_slave.custom3.labels)
			{
				custom_list[i].add(label);
			}

			id = "cboCustomField3_" + std::to_string(i);
			cboCustomField3.emplace_back(new gcn::DropDown(&custom_list[i]));
			cboCustomField3[i]->setId(id);
			cboCustomField3[i]->setSize(textfield_width, cboCustomField3[i]->getHeight());
			cboCustomField3[i]->setBaseColor(gui_baseCol);
			cboCustomField3[i]->setBackgroundColor(colTextboxBackground);

			cboCustomField3[i]->setPosition(pos_x2, pos_y);
			pos_y += cboCustomField3[i]->getHeight() + 8;

			wndShowCustomFields->add(cboCustomField3[i]);
		}
		else
		{
			pos_y += lblCustomField3[i]->getHeight() + 8;
		}
	}

	for (int i = 0; i < custom4_number; i++)
	{
		std::string id;

		lblCustomField4.emplace_back(new gcn::Label("Custom4:"));
		lblCustomField4[i]->setPosition(pos_x1, pos_y);

		wndShowCustomFields->add(lblCustomField4[i]);

		if (whdload_prefs.selected_slave.custom4.type == bit_type)
		{
			chkCustomField4.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom4.label_bit_pairs[i].first));
			// TODO not correct, it shouldn't use the value coming from the XML
			chkCustomField4[i]->setSelected(whdload_prefs.selected_slave.custom4.label_bit_pairs[i].second);

			id = "chkCustomField4_" + std::to_string(i);
			chkCustomField4[i]->setId(id);

			chkCustomField4[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField4[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField4[i]);
		}
		else if (whdload_prefs.selected_slave.custom4.type == bool_type)
		{
			chkCustomField4.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom4.caption));
			chkCustomField4[i]->setSelected(whdload_prefs.selected_slave.custom4.value);

			id = "chkCustomField4_" + std::to_string(i);
			chkCustomField4[i]->setId(id);

			chkCustomField4[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField4[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField4[i]);
		}
		else if (whdload_prefs.selected_slave.custom4.type == list_type)
		{
			lblCustomField4[i]->setCaption(whdload_prefs.selected_slave.custom4.caption);

			for (const auto& label : whdload_prefs.selected_slave.custom4.labels)
			{
				custom_list[i].add(label);
			}

			id = "cboCustomField4_" + std::to_string(i);
			cboCustomField4.emplace_back(new gcn::DropDown(&custom_list[i]));
			cboCustomField4[i]->setId(id);
			cboCustomField4[i]->setSize(textfield_width, cboCustomField4[i]->getHeight());
			cboCustomField4[i]->setBaseColor(gui_baseCol);
			cboCustomField4[i]->setBackgroundColor(colTextboxBackground);

			cboCustomField4[i]->setPosition(pos_x2, pos_y);
			pos_y += cboCustomField4[i]->getHeight() + 8;

			wndShowCustomFields->add(cboCustomField4[i]);
		}
		else
		{
			pos_y += lblCustomField4[i]->getHeight() + 8;
		}
	}

	for (int i = 0; i < custom5_number; i++)
	{
		std::string id;

		lblCustomField5.emplace_back(new gcn::Label("Custom5:"));
		lblCustomField5[i]->setPosition(pos_x1, pos_y);

		wndShowCustomFields->add(lblCustomField5[i]);

		if (whdload_prefs.selected_slave.custom5.type == bit_type)
		{
			chkCustomField5.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom5.label_bit_pairs[i].first));
			// TODO not correct, it shouldn't use the value coming from the XML
			chkCustomField5[i]->setSelected(whdload_prefs.selected_slave.custom5.label_bit_pairs[i].second);

			id = "chkCustomField5_" + std::to_string(i);
			chkCustomField5[i]->setId(id);

			chkCustomField5[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField5[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField5[i]);
		}
		else if (whdload_prefs.selected_slave.custom5.type == bool_type)
		{
			chkCustomField5.emplace_back(new gcn::CheckBox(whdload_prefs.selected_slave.custom5.caption));
			chkCustomField5[i]->setSelected(whdload_prefs.selected_slave.custom5.value);

			id = "chkCustomField5_" + std::to_string(i);
			chkCustomField5[i]->setId(id);

			chkCustomField5[i]->setPosition(pos_x2, pos_y);
			pos_y += chkCustomField5[i]->getHeight() + 8;

			wndShowCustomFields->add(chkCustomField5[i]);
		}
		else if (whdload_prefs.selected_slave.custom5.type == list_type)
		{
			lblCustomField5[i]->setCaption(whdload_prefs.selected_slave.custom5.caption);

			for (const auto& label : whdload_prefs.selected_slave.custom5.labels)
			{
				custom_list[i].add(label);
			}

			id = "cboCustomField5_" + std::to_string(i);
			cboCustomField5.emplace_back(new gcn::DropDown(&custom_list[i]));
			cboCustomField5[i]->setId(id);
			cboCustomField5[i]->setSize(textfield_width, cboCustomField5[i]->getHeight());
			cboCustomField5[i]->setBaseColor(gui_baseCol);
			cboCustomField5[i]->setBackgroundColor(colTextboxBackground);

			cboCustomField5[i]->setPosition(pos_x2, pos_y);
			pos_y += cboCustomField5[i]->getHeight() + 8;

			wndShowCustomFields->add(cboCustomField5[i]);
		}
		else
		{
			pos_y += lblCustomField5[i]->getHeight() + 8;
		}
	}

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

	for (const auto label : lblCustomField1)
		delete label;
	lblCustomField1.clear();

	for (const auto checkbox : chkCustomField1)
		delete checkbox;
	chkCustomField1.clear();

	for (const auto dropdown : cboCustomField1)
		delete dropdown;
	cboCustomField1.clear();

	for (const auto label : lblCustomField2)
		delete label;
	lblCustomField2.clear();

	for (const auto checkbox : chkCustomField2)
		delete checkbox;
	chkCustomField2.clear();

	for (const auto dropdown : cboCustomField2)
		delete dropdown;
	cboCustomField2.clear();

	for (const auto label : lblCustomField3)
		delete label;
	lblCustomField3.clear();

	for (const auto checkbox : chkCustomField3)
		delete checkbox;
	chkCustomField3.clear();

	for (const auto dropdown : cboCustomField3)
		delete dropdown;
	cboCustomField3.clear();

	for (const auto label : lblCustomField4)
		delete label;
	lblCustomField4.clear();

	for (const auto checkbox : chkCustomField4)
		delete checkbox;
	chkCustomField4.clear();

	for (const auto dropdown : cboCustomField4)
		delete dropdown;
	cboCustomField4.clear();

	for (const auto label : lblCustomField5)
		delete label;
	lblCustomField5.clear();

	for (const auto checkbox : chkCustomField5)
		delete checkbox;
	chkCustomField5.clear();

	for (const auto dropdown : cboCustomField5)
		delete dropdown;
	cboCustomField5.clear();

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