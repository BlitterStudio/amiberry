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
#include "rommgr.h"
#include "StringListModel.h"
#include "uae.h"

enum
{
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 300
};

static bool dialogResult = false;
static bool dialogFinished = false;

static gcn::Window* wndEditCDDrive;
static gcn::Button* cmdCDDriveOK;
static gcn::Button* cmdCDDriveCancel;
static gcn::Label* lblCDDrivePath;
static gcn::TextField* txtCDDrivePath;

static gcn::Label* lblCDDriveController;
static gcn::DropDown* cboCDDriveController;
static gcn::DropDown* cboCDDriveUnit;

static gcn::StringListModel controllerListModel;
static gcn::StringListModel unitListModel;

class CDDriveActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto source = actionEvent.getSource();
		if (source == cmdCDDriveOK)
		{
			if (txtCDDrivePath->getText().empty())
			{
				wndEditCDDrive->setCaption("Path is empty!");
				return;
			}

			dialogResult = true;
			dialogFinished = true;
		}
		else if (source == cmdCDDriveCancel)
		{
			dialogResult = false;
			dialogFinished = true;
		}
		else if (source == cboCDDriveController)
		{
			const auto posn = controller[cboCDDriveController->getSelected()].type;
			current_cddlg.ci.controller_type = posn % HD_CONTROLLER_NEXT_UNIT;
			current_cddlg.ci.controller_type_unit = posn / HD_CONTROLLER_NEXT_UNIT;
			inithdcontroller(current_cddlg.ci.controller_type, current_cddlg.ci.controller_type_unit, UAEDEV_CD, current_cddlg.ci.rootdir[0] != 0);
		}
		else if (source == cboCDDriveUnit)
		{
			current_cddlg.ci.controller_unit = cboCDDriveUnit->getSelected();
		}
	}
};
static CDDriveActionListener* cdDriveActionListener;

static void InitEditCDDrive()
{
	auto cd_drives = get_cd_drives();

	wndEditCDDrive = new gcn::Window("CD Settings");
	wndEditCDDrive->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndEditCDDrive->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndEditCDDrive->setBaseColor(gui_base_color);
	wndEditCDDrive->setForegroundColor(gui_foreground_color);
	wndEditCDDrive->setCaption("CD Settings");
	wndEditCDDrive->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndEditCDDrive->setMovable(false);

	cdDriveActionListener = new CDDriveActionListener();

	cmdCDDriveOK = new gcn::Button("Add CD Drive");
	cmdCDDriveOK->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdCDDriveOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 4 * BUTTON_WIDTH - DISTANCE_NEXT_X,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCDDriveOK->setBaseColor(gui_base_color);
	cmdCDDriveOK->setForegroundColor(gui_foreground_color);
	cmdCDDriveOK->setId("cmdCDDriveOK");
	cmdCDDriveOK->addActionListener(cdDriveActionListener);

	cmdCDDriveCancel = new gcn::Button("Cancel");
	cmdCDDriveCancel->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdCDDriveCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCDDriveCancel->setBaseColor(gui_base_color);
	cmdCDDriveCancel->setForegroundColor(gui_foreground_color);
	cmdCDDriveCancel->setId("cmdCDDriveCancel");
	cmdCDDriveCancel->addActionListener(cdDriveActionListener);

	lblCDDrivePath = new gcn::Label("Path:");
	lblCDDrivePath->setAlignment(gcn::Graphics::Right);
	txtCDDrivePath = new gcn::TextField();
	txtCDDrivePath->setSize(490, TEXTFIELD_HEIGHT);
	txtCDDrivePath->setId("txtCDDrivePath");
	txtCDDrivePath->setBaseColor(gui_base_color);
	txtCDDrivePath->setBackgroundColor(gui_background_color);
	txtCDDrivePath->setForegroundColor(gui_foreground_color);

	lblCDDriveController = new gcn::Label("Controller:");
	lblCDDriveController->setAlignment(gcn::Graphics::Right);
	cboCDDriveController = new gcn::DropDown(&controllerListModel);
	cboCDDriveController->setSize(180, DROPDOWN_HEIGHT);
	cboCDDriveController->setBaseColor(gui_base_color);
	cboCDDriveController->setBackgroundColor(gui_background_color);
	cboCDDriveController->setForegroundColor(gui_foreground_color);
	cboCDDriveController->setSelectionColor(gui_selection_color);
	cboCDDriveController->setId("cboCDDriveController");
	cboCDDriveController->addActionListener(cdDriveActionListener);

	cboCDDriveUnit = new gcn::DropDown(&unitListModel);
	cboCDDriveUnit->setSize(60, DROPDOWN_HEIGHT);
	cboCDDriveUnit->setBaseColor(gui_base_color);
	cboCDDriveUnit->setBackgroundColor(gui_background_color);
	cboCDDriveUnit->setForegroundColor(gui_foreground_color);
	cboCDDriveUnit->setSelectionColor(gui_selection_color);
	cboCDDriveUnit->setId("cboCDDriveUnit");
	cboCDDriveUnit->addActionListener(cdDriveActionListener);

	int pos_y = DISTANCE_BORDER;

	wndEditCDDrive->add(lblCDDrivePath, DISTANCE_BORDER, pos_y);
	wndEditCDDrive->add(txtCDDrivePath, lblCDDrivePath->getX() + lblCDDrivePath->getWidth() + 8, pos_y);
	pos_y = txtCDDrivePath->getY() + txtCDDrivePath->getHeight() + DISTANCE_NEXT_Y;

	wndEditCDDrive->add(lblCDDriveController, DISTANCE_BORDER, pos_y);
	wndEditCDDrive->add(cboCDDriveController, lblCDDriveController->getX() + lblCDDriveController->getWidth() + 8, pos_y);
	wndEditCDDrive->add(cboCDDriveUnit, cboCDDriveController->getX() + cboCDDriveController->getWidth() + DISTANCE_NEXT_X, pos_y);

	wndEditCDDrive->add(cmdCDDriveOK);
	wndEditCDDrive->add(cmdCDDriveCancel);

	gui_top->add(wndEditCDDrive);

	wndEditCDDrive->requestModalFocus();
	focus_bug_workaround(wndEditCDDrive);

	// If we found CD Drives, auto-select the first one for now
	if (!cd_drives.empty())
	{
		txtCDDrivePath->setText(cd_drives.at(0));
	}
}

static void ExitEditCDDrive()
{
	wndEditCDDrive->releaseModalFocus();
	gui_top->remove(wndEditCDDrive);

	delete cmdCDDriveOK;
	delete cmdCDDriveCancel;
	delete lblCDDrivePath;
	delete txtCDDrivePath;
	delete lblCDDriveController;
	delete cboCDDriveController;
	delete cboCDDriveUnit;
	delete cdDriveActionListener;
	delete wndEditCDDrive;
}

static void EditCDDriveLoop()
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

bool EditCDDrive(const int unit_no)
{
	const AmigaMonitor* mon = &AMonitors[0];

	mountedinfo mi{};
	uaedev_config_data* uci;

	dialogResult = false;
	dialogFinished = false;

	if (current_cddlg.ci.controller_type == HD_CONTROLLER_TYPE_UAE)
		current_cddlg.ci.controller_type = (is_board_enabled(&changed_prefs, ROMTYPE_A2091, 0) ||
			is_board_enabled(&changed_prefs, ROMTYPE_GVPS2, 0) || is_board_enabled(&changed_prefs, ROMTYPE_A4091, 0) ||
			(changed_prefs.cs_mbdmac & 3)) ? HD_CONTROLLER_TYPE_SCSI_AUTO : HD_CONTROLLER_TYPE_IDE_AUTO;
	inithdcontroller(current_cddlg.ci.controller_type, current_cddlg.ci.controller_type_unit, UAEDEV_CD, current_cddlg.ci.rootdir[0] != 0);

	controllerListModel.clear();
	for (const auto& [type, display] : controller) {
		controllerListModel.add(display);
	}

	unitListModel.clear();
	for (const auto& controller_unit_item : controller_unit) {
		unitListModel.add(controller_unit_item);
	}

	InitEditCDDrive();

	if (unit_no >= 0)
	{
		uci = &changed_prefs.mountconfig[unit_no];
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);
		memcpy(&current_cddlg.ci, uci, sizeof(uaedev_config_info));

		txtCDDrivePath->setText(current_cddlg.ci.rootdir);
		cboCDDriveController->setSelected(current_cddlg.ci.controller_type);
		cboCDDriveUnit->setSelected(current_cddlg.ci.controller_unit);
	}
	else
	{
		//TODO default_cddlg ?
	}

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		EditCDDriveLoop();
		cap_fps(start);
	}

	if (dialogResult)
	{
		strncpy(current_cddlg.ci.rootdir, txtCDDrivePath->getText().c_str(), sizeof(current_cddlg.ci.rootdir) - 1);

		auto posn = controller[cboCDDriveController->getSelected()].type;
		current_cddlg.ci.controller_type = posn % HD_CONTROLLER_NEXT_UNIT;
		current_cddlg.ci.controller_type_unit = posn / HD_CONTROLLER_NEXT_UNIT;
		inithdcontroller(current_cddlg.ci.controller_type, current_cddlg.ci.controller_type_unit, UAEDEV_CD, current_cddlg.ci.rootdir[0] != 0);

		current_cddlg.ci.controller_unit = cboCDDriveUnit->getSelected();

		new_cddrive(unit_no);
	}

	ExitEditCDDrive();

	return dialogResult;
}