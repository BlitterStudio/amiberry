#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "autoconf.h"
#include "gui_handling.h"

#include "amiberry_gfx.h"
#include "amiberry_input.h"
#include "StringListModel.h"

enum
{
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 250
};

static bool dialogResult = false;
static bool dialogFinished = false;

static gcn::Window* wndEditTapeDrive;
static gcn::Button* cmdTapeDriveOK;
static gcn::Button* cmdTapeDriveCancel;
static gcn::Label* lblTapeDrivePath;
static gcn::TextField* txtTapeDrivePath;

static gcn::Button* cmdTapeDriveSelectDir;
static gcn::Button* cmdTapeDriveSelectFile;

static gcn::Label* lblTapeDriveController;
static gcn::DropDown* cboTapeDriveController;
static gcn::DropDown* cboTapeDriveUnit;

static gcn::StringListModel controllerListModel;
static gcn::StringListModel unitListModel;

class TapeDriveActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		auto source = actionEvent.getSource();
		if (source == cmdTapeDriveOK)
		{
			if (txtTapeDrivePath->getText().empty())
			{
				wndEditTapeDrive->setCaption("Path is empty!");
				return;
			}
			dialogResult = true;
			dialogFinished = true;
		}
		else if (source == cmdTapeDriveCancel)
		{
			dialogResult = false;
			dialogFinished = true;
		}
		else if (source == cmdTapeDriveSelectDir)
		{
			wndEditTapeDrive->releaseModalFocus();
			std::string path;
			if (txtTapeDrivePath->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtTapeDrivePath->getText();
			}
			std::string tmp = SelectFolder("Select Directory", path);
			if (!tmp.empty())
			{
				txtTapeDrivePath->setText(tmp);
				default_tapedlg(&current_tapedlg);
				strncpy(current_tapedlg.ci.rootdir, tmp.c_str(), sizeof(current_tapedlg.ci.rootdir) - 1);
			}
			wndEditTapeDrive->requestModalFocus();
		}
		else if (source == cmdTapeDriveSelectFile)
		{
			wndEditTapeDrive->releaseModalFocus();
			std::string path;
			if (txtTapeDrivePath->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtTapeDrivePath->getText();
			}
			std::string tmp = SelectFile("Select Tape Image", path, harddisk_filter);
			if (!tmp.empty())
			{
				txtTapeDrivePath->setText(tmp);
				default_tapedlg(&current_tapedlg);
				strncpy(current_tapedlg.ci.rootdir, tmp.c_str(), sizeof(current_tapedlg.ci.rootdir) - 1);
			}
			wndEditTapeDrive->requestModalFocus();
		}
		else if (source == cboTapeDriveController)
		{
			auto posn = controller[cboTapeDriveController->getSelected()].type;
			current_tapedlg.ci.controller_type = posn % HD_CONTROLLER_NEXT_UNIT;
			current_tapedlg.ci.controller_type_unit = posn / HD_CONTROLLER_NEXT_UNIT;
			inithdcontroller(current_tapedlg.ci.controller_type, current_tapedlg.ci.controller_type_unit, UAEDEV_TAPE, current_tapedlg.ci.rootdir[0] != 0);
		}
		else if (source == cboTapeDriveUnit)
		{
			current_tapedlg.ci.controller_unit = cboTapeDriveUnit->getSelected();
		}
	}
};
static TapeDriveActionListener* tapeDriveActionListener;

static void InitEditTapeDrive()
{
	wndEditTapeDrive = new gcn::Window("Edit Tape Drive");
	wndEditTapeDrive->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndEditTapeDrive->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndEditTapeDrive->setBaseColor(gui_base_color);
	wndEditTapeDrive->setForegroundColor(gui_foreground_color);
	wndEditTapeDrive->setCaption("Tape Drive Settings");
	wndEditTapeDrive->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndEditTapeDrive->setMovable(false);

	tapeDriveActionListener = new TapeDriveActionListener();

	cmdTapeDriveOK = new gcn::Button("Add Tape Drive");
	cmdTapeDriveOK->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdTapeDriveOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 4 * BUTTON_WIDTH - DISTANCE_NEXT_X,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdTapeDriveOK->setBaseColor(gui_base_color);
	cmdTapeDriveOK->setForegroundColor(gui_foreground_color);
	cmdTapeDriveOK->setId("cmdTapeDriveOK");
	cmdTapeDriveOK->addActionListener(tapeDriveActionListener);

	cmdTapeDriveCancel = new gcn::Button("Cancel");
	cmdTapeDriveCancel->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdTapeDriveCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdTapeDriveCancel->setBaseColor(gui_base_color);
	cmdTapeDriveCancel->setForegroundColor(gui_foreground_color);
	cmdTapeDriveCancel->setId("cmdTapeDriveCancel");
	cmdTapeDriveCancel->addActionListener(tapeDriveActionListener);

	lblTapeDrivePath = new gcn::Label("Path:");
	lblTapeDrivePath->setAlignment(gcn::Graphics::Right);
	txtTapeDrivePath = new gcn::TextField();
	txtTapeDrivePath->setSize(490, TEXTFIELD_HEIGHT);
	txtTapeDrivePath->setId("txtTapeDrivePath");
	txtTapeDrivePath->setBaseColor(gui_base_color);
	txtTapeDrivePath->setBackgroundColor(gui_background_color);
	txtTapeDrivePath->setForegroundColor(gui_foreground_color);

	cmdTapeDriveSelectDir = new gcn::Button("Select Directory");
	cmdTapeDriveSelectDir->setSize(BUTTON_WIDTH * 3, BUTTON_HEIGHT);
	cmdTapeDriveSelectDir->setBaseColor(gui_base_color);
	cmdTapeDriveSelectDir->setForegroundColor(gui_foreground_color);
	cmdTapeDriveSelectDir->setId("cmdTapeDriveSelectDir");
	cmdTapeDriveSelectDir->addActionListener(tapeDriveActionListener);

	cmdTapeDriveSelectFile = new gcn::Button("Select File");
	cmdTapeDriveSelectFile->setSize(BUTTON_WIDTH * 3, BUTTON_HEIGHT);
	cmdTapeDriveSelectFile->setBaseColor(gui_base_color);
	cmdTapeDriveSelectFile->setForegroundColor(gui_foreground_color);
	cmdTapeDriveSelectFile->setId("cmdTapeDriveSelectFile");
	cmdTapeDriveSelectFile->addActionListener(tapeDriveActionListener);

	lblTapeDriveController = new gcn::Label("Controller:");
	lblTapeDriveController->setAlignment(gcn::Graphics::Right);
	cboTapeDriveController = new gcn::DropDown(&controllerListModel);
	cboTapeDriveController->setSize(180, DROPDOWN_HEIGHT);
	cboTapeDriveController->setBaseColor(gui_base_color);
	cboTapeDriveController->setBackgroundColor(gui_background_color);
	cboTapeDriveController->setForegroundColor(gui_foreground_color);
	cboTapeDriveController->setSelectionColor(gui_selection_color);
	cboTapeDriveController->setId("cboTapeDriveController");
	cboTapeDriveController->addActionListener(tapeDriveActionListener);

	cboTapeDriveUnit = new gcn::DropDown(&unitListModel);
	cboTapeDriveUnit->setSize(60, DROPDOWN_HEIGHT);
	cboTapeDriveUnit->setBaseColor(gui_base_color);
	cboTapeDriveUnit->setBackgroundColor(gui_background_color);
	cboTapeDriveUnit->setForegroundColor(gui_foreground_color);
	cboTapeDriveUnit->setSelectionColor(gui_selection_color);
	cboTapeDriveUnit->setId("cboTapeDriveUnit");
	cboTapeDriveUnit->addActionListener(tapeDriveActionListener);

	int posY = DISTANCE_BORDER;

	wndEditTapeDrive->add(lblTapeDrivePath, DISTANCE_BORDER, posY);
	wndEditTapeDrive->add(txtTapeDrivePath, lblTapeDrivePath->getX() + lblTapeDrivePath->getWidth() + 8, posY);
	posY = txtTapeDrivePath->getY() + txtTapeDrivePath->getHeight() + DISTANCE_NEXT_Y;

	wndEditTapeDrive->add(cmdTapeDriveSelectDir, DISTANCE_BORDER, posY);
	wndEditTapeDrive->add(cmdTapeDriveSelectFile, cmdTapeDriveSelectDir->getX() + cmdTapeDriveSelectDir->getWidth() + DISTANCE_NEXT_X, posY);
	posY = cmdTapeDriveSelectDir->getY() + cmdTapeDriveSelectDir->getHeight() + DISTANCE_NEXT_Y;

	wndEditTapeDrive->add(lblTapeDriveController, DISTANCE_BORDER, posY);
	wndEditTapeDrive->add(cboTapeDriveController, lblTapeDriveController->getX() + lblTapeDriveController->getWidth() + 8, posY);
	wndEditTapeDrive->add(cboTapeDriveUnit, cboTapeDriveController->getX() + cboTapeDriveController->getWidth() + DISTANCE_NEXT_X, posY);

	wndEditTapeDrive->add(cmdTapeDriveOK);
	wndEditTapeDrive->add(cmdTapeDriveCancel);

	gui_top->add(wndEditTapeDrive);

	wndEditTapeDrive->requestModalFocus();
	focus_bug_workaround(wndEditTapeDrive);
	cmdTapeDriveSelectDir->requestFocus();
}

static void ExitEditTapeDrive()
{
	wndEditTapeDrive->releaseModalFocus();
	gui_top->remove(wndEditTapeDrive);

	delete cmdTapeDriveOK;
	delete cmdTapeDriveCancel;
	delete lblTapeDrivePath;
	delete txtTapeDrivePath;
	delete cmdTapeDriveSelectDir;
	delete cmdTapeDriveSelectFile;
	delete lblTapeDriveController;
	delete cboTapeDriveController;
	delete cboTapeDriveUnit;
	delete tapeDriveActionListener;
	delete wndEditTapeDrive;
}

static void EditTapeDriveLoop()
{
	const AmigaMonitor* mon = &AMonitors[0];

	int got_event = 0;
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
				dialogFinished = true;
				break;

			case VK_UP:
				if (handle_navigation(DIRECTION_UP))
					continue; // Don't change value when enter ComboBox -> don't send event to control
				break;

			case VK_DOWN:
				if (handle_navigation(DIRECTION_DOWN))
					continue; // Don't change value when enter ComboBox -> don't send event to control
				break;

			case VK_LEFT:
				if (handle_navigation(DIRECTION_LEFT))
					continue; // Don't change value when enter Slider -> don't send event to control
				break;

			case VK_RIGHT:
				if (handle_navigation(DIRECTION_RIGHT))
					continue; // Don't change value when enter Slider -> don't send event to control
				break;

			case VK_Blue:
			case VK_Green:
				event.key.keysym.sym = SDLK_RETURN;
				gui_input->pushInput(event); // Fire key down
				event.type = SDL_KEYUP; // and the key up
				break;
			default:
				break;
			}
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				const int hat = SDL_JoystickGetHat(gui_joystick, 0);

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_UP]) || (hat & SDL_HAT_UP)) // dpad
				{
					if (handle_navigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || (hat & SDL_HAT_DOWN)) // dpad
				{
					if (handle_navigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || (hat & SDL_HAT_RIGHT)) // dpad
				{
					if (handle_navigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || (hat & SDL_HAT_LEFT)) // dpad
				{
					if (handle_navigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_LEFT);
					break;
				}
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
					dialogFinished = true;
					break;
				}
				if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])))
				{
					for (auto z = 0; z < 10; ++z)
					{
						PushFakeKey(SDLK_UP);
					}
				}
				if ((did->mapping.is_retroarch || !did->is_controller)
					&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])
					|| SDL_GameControllerGetButton(did->controller,
						static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])))
				{
					for (auto z = 0; z < 10; ++z)
					{
						PushFakeKey(SDLK_DOWN);
					}
				}
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = 1;
				if (event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
				{
					if (event.jaxis.value > joystick_dead_zone && last_x != 1)
					{
						last_x = 1;
						if (handle_navigation(DIRECTION_RIGHT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_RIGHT);
						break;
					}
					if (event.jaxis.value < -joystick_dead_zone && last_x != -1)
					{
						last_x = -1;
						if (handle_navigation(DIRECTION_LEFT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_LEFT);
						break;
					}
					if (event.jaxis.value > -joystick_dead_zone && event.jaxis.value < joystick_dead_zone)
						last_x = 0;
				}
				else if (event.jaxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
				{
					if (event.jaxis.value < -joystick_dead_zone && last_y != -1)
					{
						last_y = -1;
						if (handle_navigation(DIRECTION_UP))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_UP);
						break;
					}
					if (event.jaxis.value > joystick_dead_zone && last_y != 1)
					{
						last_y = 1;
						if (handle_navigation(DIRECTION_DOWN))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_DOWN);
						break;
					}
					if (event.jaxis.value > -joystick_dead_zone && event.jaxis.value < joystick_dead_zone)
						last_y = 0;
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

bool EditTapeDrive(const int unit_no)
{
	const AmigaMonitor* mon = &AMonitors[0];

	mountedinfo mi{};

	dialogResult = false;
	dialogFinished = false;

	inithdcontroller(current_tapedlg.ci.controller_type, current_tapedlg.ci.controller_type_unit, UAEDEV_TAPE, current_tapedlg.ci.rootdir[0] != 0);

	controllerListModel.clear();
	for (const auto& [type, display] : controller) {
		controllerListModel.add(display);
	}

	unitListModel.clear();
	for (const auto& controller_unit_item : controller_unit) {
		unitListModel.add(controller_unit_item);
	}

	InitEditTapeDrive();

	if (unit_no >= 0)
	{
		uaedev_config_data* uci = &changed_prefs.mountconfig[unit_no];
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);
		memcpy(&current_tapedlg.ci, uci, sizeof(uaedev_config_info));

		txtTapeDrivePath->setText(current_tapedlg.ci.rootdir);
		cboTapeDriveController->setSelected(current_tapedlg.ci.controller_type);
		cboTapeDriveUnit->setSelected(current_tapedlg.ci.controller_unit);
	}
	else
	{
		default_tapedlg(&current_tapedlg);
	}

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		EditTapeDriveLoop();
		cap_fps(start);
	}

	if (dialogResult)
	{
		new_tapedrive(unit_no);
	}

	ExitEditTapeDrive();

	return dialogResult;
}