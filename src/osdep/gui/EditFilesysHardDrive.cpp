#include <cstdio>
#include <cstring>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "autoconf.h"
#include "filesys.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"
#include "rommgr.h"
#include "StringListModel.h"

enum
{
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 250
};

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;

static gcn::Window* wndEditFilesysHardDrive;
static gcn::Button* cmdHDDOk;
static gcn::Button* cmdHDDCancel;
static gcn::Label* lblHDPath;
static gcn::TextField* txtHDPath;

static gcn::Label* lblHDController;
static gcn::DropDown* cboHDController;
static gcn::DropDown* cboHDControllerUnit;
static gcn::DropDown* cboHDControllerType;
static gcn::DropDown* cboHDFeatureLevel;

static gcn::StringListModel controllerListModel;
static gcn::StringListModel unitListModel;
static gcn::StringListModel controllerTypeListModel;
static gcn::StringListModel hdFeatureLevelListModel;

static void sethardfiletypes()
{
	bool ide = current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST;
	bool scsi = current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_SCSI_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_SCSI_LAST;
	cboHDControllerType->setEnabled(ide);
	cboHDFeatureLevel->setEnabled(ide || scsi);
	if (!ide) {
		current_hfdlg.ci.controller_media_type = 0;
	}
	if (current_hfdlg.ci.controller_media_type && current_hfdlg.ci.unit_feature_level == 0)
		current_hfdlg.ci.unit_feature_level = 1;
	cboHDControllerType->setSelected(current_hfdlg.ci.controller_media_type);
	cboHDFeatureLevel->setSelected(current_hfdlg.ci.unit_feature_level);
}

static void sethd()
{
	bool rdb = is_hdf_rdb();
	bool physgeo = (rdb) || current_hfdlg.ci.chs;
	bool enablegeo = (!rdb || (physgeo && current_hfdlg.ci.geometry[0] == 0)) && !current_hfdlg.ci.chs;
	const struct expansionromtype* ert = get_unit_expansion_rom(current_hfdlg.ci.controller_type);
	if (ert && current_hfdlg.ci.controller_unit >= 8) {
		if (!_tcscmp(ert->name, _T("a2091"))) {
			current_hfdlg.ci.unit_feature_level = HD_LEVEL_SASI_CHS;
		}
		else if (!_tcscmp(ert->name, _T("a2090a"))) {
			current_hfdlg.ci.unit_feature_level = HD_LEVEL_SCSI_1;
		}
	}
	if (!physgeo)
		current_hfdlg.ci.physical_geometry = false;
}

static void sethardfilegeo()
{
	if (current_hfdlg.ci.geometry[0]) {
		current_hfdlg.ci.physical_geometry = true;
		get_hd_geometry(&current_hfdlg.ci);
	}
	else if (current_hfdlg.ci.chs) {
		current_hfdlg.ci.physical_geometry = true;
	}
}

static void setharddrive()
{
	sethardfilegeo();
	sethd();
	//txtHDPath->setText(current_hfdlg.ci.rootdir);
	auto selIndex = 0;
	for (auto i = 0; i < controller.size(); ++i) {
		if (controller[i].type == current_hfdlg.ci.controller_type)
			selIndex = i;
	}
	cboHDController->setSelected(selIndex);
	cboHDControllerUnit->setSelected(current_hfdlg.ci.controller_unit);
	sethardfiletypes();
}

class FilesysHardDriveActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent &action_event) override
	{
		if (action_event.getSource() == cmdHDDOk)
		{
			if (txtHDPath->getText().empty())
			{
				wndEditFilesysHardDrive->setCaption("Please select a device.");
				return;
			}
			_tcscpy(current_hfdlg.ci.rootdir, txtHDPath->getText().c_str());
			// Set RDB mode if IDE or SCSI
			if (current_hfdlg.ci.controller_type > 0) {
				current_hfdlg.ci.sectors = current_hfdlg.ci.reserved = current_hfdlg.ci.surfaces = 0;
			}

			dialogResult = true;
			dialogFinished = true;
		}
		else if (action_event.getSource() == cboHDController)
		{
			int posn = cboHDController->getSelected();
			current_hfdlg.ci.controller_type = posn % HD_CONTROLLER_NEXT_UNIT;
			current_hfdlg.ci.controller_type_unit = posn / HD_CONTROLLER_NEXT_UNIT;
			current_hfdlg.forcedcylinders = 0;
			current_hfdlg.ci.cyls = current_hfdlg.ci.highcyl = current_hfdlg.ci.sectors = current_hfdlg.ci.surfaces = 0;
			std::string txt1, txt2;
			updatehdfinfo(true, true, true, txt1, txt2);
			inithdcontroller(current_hfdlg.ci.controller_type, current_hfdlg.ci.controller_type_unit, UAEDEV_HDF, current_hfdlg.ci.rootdir[0] != 0);
			setharddrive();
		}
		else if (action_event.getSource() == cboHDControllerUnit)
		{
			current_hfdlg.ci.controller_unit = cboHDControllerUnit->getSelected();
			setharddrive();
		}
		else if (action_event.getSource() == cboHDControllerType)
		{
			current_hfdlg.ci.controller_media_type = cboHDControllerType->getSelected();
			setharddrive();
		}
		else if (action_event.getSource() == cboHDFeatureLevel)
		{
			current_hfdlg.ci.unit_feature_level = cboHDFeatureLevel->getSelected();
			setharddrive();
		}
		else if (action_event.getSource() == cmdHDDCancel)
		{
			dialogResult = false;
			dialogFinished = true;
		}
	}
};

static FilesysHardDriveActionListener *filesysHardDriveActionListener;

static void InitEditFilesysHardDrive()
{
	wndEditFilesysHardDrive = new gcn::Window("Edit");
	wndEditFilesysHardDrive->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndEditFilesysHardDrive->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndEditFilesysHardDrive->setBaseColor(gui_base_color);
	wndEditFilesysHardDrive->setForegroundColor(gui_foreground_color);
	wndEditFilesysHardDrive->setCaption("Hard Drive settings");
	wndEditFilesysHardDrive->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndEditFilesysHardDrive->setMovable(false);

	filesysHardDriveActionListener = new FilesysHardDriveActionListener();

	cmdHDDOk = new gcn::Button("Ok");
	cmdHDDOk->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdHDDOk->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdHDDOk->setBaseColor(gui_base_color);
	cmdHDDOk->setForegroundColor(gui_foreground_color);
	cmdHDDOk->setId("cmdHDDOk");
	cmdHDDOk->addActionListener(filesysHardDriveActionListener);

	cmdHDDCancel = new gcn::Button("Cancel");
	cmdHDDCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdHDDCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
		DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdHDDCancel->setBaseColor(gui_base_color);
	cmdHDDCancel->setForegroundColor(gui_foreground_color);
	cmdHDDCancel->setId("cmdHDDCancel");
	cmdHDDCancel->addActionListener(filesysHardDriveActionListener);

	lblHDPath = new gcn::Label("Path:");
	lblHDPath->setAlignment(gcn::Graphics::Right);
	txtHDPath = new gcn::TextField();
	txtHDPath->setSize(500, TEXTFIELD_HEIGHT);
	txtHDPath->setId("txtHDDPath");
	txtHDPath->setBaseColor(gui_base_color);
	txtHDPath->setBackgroundColor(gui_background_color);
	txtHDPath->setForegroundColor(gui_foreground_color);

	lblHDController = new gcn::Label("Controller:");
	lblHDController->setAlignment(gcn::Graphics::Right);
	cboHDController = new gcn::DropDown(&controllerListModel);
	cboHDController->setSize(200, DROPDOWN_HEIGHT);
	cboHDController->setBaseColor(gui_base_color);
	cboHDController->setBackgroundColor(gui_background_color);
	cboHDController->setForegroundColor(gui_foreground_color);
	cboHDController->setSelectionColor(gui_selection_color);
	cboHDController->setId("cboHDController");
	cboHDController->addActionListener(filesysHardDriveActionListener);

	cboHDControllerUnit = new gcn::DropDown(&unitListModel);
	cboHDControllerUnit->setSize(60, DROPDOWN_HEIGHT);
	cboHDControllerUnit->setBaseColor(gui_base_color);
	cboHDControllerUnit->setBackgroundColor(gui_background_color);
	cboHDControllerUnit->setForegroundColor(gui_foreground_color);
	cboHDControllerUnit->setSelectionColor(gui_selection_color);
	cboHDControllerUnit->setId("cboHDControllerUnit");
	cboHDControllerUnit->addActionListener(filesysHardDriveActionListener);

	cboHDControllerType = new gcn::DropDown(&controllerTypeListModel);
	cboHDControllerType->setSize(60, DROPDOWN_HEIGHT);
	cboHDControllerType->setBaseColor(gui_base_color);
	cboHDControllerType->setBackgroundColor(gui_background_color);
	cboHDControllerType->setForegroundColor(gui_foreground_color);
	cboHDControllerType->setSelectionColor(gui_selection_color);
	cboHDControllerType->setId("cboHDControllerType");
	cboHDControllerType->addActionListener(filesysHardDriveActionListener);

	cboHDFeatureLevel = new gcn::DropDown(&hdFeatureLevelListModel);
	cboHDFeatureLevel->setSize(80, DROPDOWN_HEIGHT);
	cboHDFeatureLevel->setBaseColor(gui_base_color);
	cboHDFeatureLevel->setBackgroundColor(gui_background_color);
	cboHDFeatureLevel->setForegroundColor(gui_foreground_color);
	cboHDFeatureLevel->setSelectionColor(gui_selection_color);
	cboHDFeatureLevel->setId("cboHDFeatureLevel");
	cboHDFeatureLevel->addActionListener(filesysHardDriveActionListener);

	int posY = DISTANCE_BORDER;

	wndEditFilesysHardDrive->add(lblHDPath, DISTANCE_BORDER, posY);
	wndEditFilesysHardDrive->add(txtHDPath, DISTANCE_BORDER + lblHDPath->getWidth() + 8, posY);
	posY += txtHDPath->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardDrive->add(lblHDController, DISTANCE_BORDER, posY);
	wndEditFilesysHardDrive->add(cboHDController, DISTANCE_BORDER + lblHDController->getWidth() + 8, posY);
	wndEditFilesysHardDrive->add(cboHDControllerUnit, cboHDController->getX() + cboHDController->getWidth() + DISTANCE_NEXT_X, posY);
	wndEditFilesysHardDrive->add(cboHDControllerType, cboHDControllerUnit->getX() + cboHDControllerUnit->getWidth() + DISTANCE_NEXT_X, posY);
	wndEditFilesysHardDrive->add(cboHDFeatureLevel, cboHDControllerType->getX() + cboHDControllerType->getWidth() + DISTANCE_NEXT_X, posY);

	wndEditFilesysHardDrive->add(cmdHDDOk);
	wndEditFilesysHardDrive->add(cmdHDDCancel);

	gui_top->add(wndEditFilesysHardDrive);

	wndEditFilesysHardDrive->requestModalFocus();
	focus_bug_workaround(wndEditFilesysHardDrive);
	cmdHDDCancel->requestFocus();

	setharddrive();
}

static void ExitEditFilesysHardDrive()
{
	wndEditFilesysHardDrive->releaseModalFocus();
	gui_top->remove(wndEditFilesysHardDrive);

	delete lblHDPath;
	delete txtHDPath;
	delete lblHDController;
	delete cboHDController;
	delete cboHDControllerUnit;
	delete cboHDControllerType;
	delete cboHDFeatureLevel;
	delete cmdHDDCancel;
	delete cmdHDDOk;

	delete filesysHardDriveActionListener;
	delete wndEditFilesysHardDrive;
}

static void EditFilesysHardDriveLoop()
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
				event.type = SDL_KEYUP;		 // and the key up
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

				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_UP]) || hat & SDL_HAT_UP)
				{
					if (handle_navigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || hat & SDL_HAT_DOWN)
				{
					if (handle_navigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || hat & SDL_HAT_RIGHT)
				{
					if (handle_navigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || hat & SDL_HAT_LEFT)
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

bool EditFilesysHardDrive(const int unit_no)
{
	const AmigaMonitor* mon = &AMonitors[0];

	mountedinfo mi{};

	dialogResult = false;
	dialogFinished = false;

	inithdcontroller(current_hfdlg.ci.controller_type, current_hfdlg.ci.controller_type_unit, UAEDEV_HDF, current_hfdlg.ci.rootdir[0] != 0);

	controllerListModel.clear();
	for (const auto& [type, display] : controller) {
		controllerListModel.add(display);
	}

	unitListModel.clear();
	for (const auto& controller_unit_item : controller_unit) {
		unitListModel.add(controller_unit_item);
	}

	controllerTypeListModel.clear();
	for (const auto& type : controller_type) {
		controllerTypeListModel.add(type);
	}

	hdFeatureLevelListModel.clear();
	for (const auto& feature_level : controller_feature_level) {
		hdFeatureLevelListModel.add(feature_level);
	}

	InitEditFilesysHardDrive();

	if (unit_no >= 0)
	{
		uaedev_config_data* uci = &changed_prefs.mountconfig[unit_no];
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);

		current_hfdlg.forcedcylinders = uci->ci.highcyl;
		memcpy(&current_hfdlg.ci, uci, sizeof(uaedev_config_info));
		fileSelected = true;
	}
	else
	{
		default_hfdlg(&current_hfdlg, true);
		fileSelected = false;
	}

	std::string txt1, txt2;
	updatehdfinfo(true, false, true, txt1, txt2);

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		EditFilesysHardDriveLoop();
		cap_fps(start);
	}

	if (dialogResult)
	{
		current_dir = extract_path(txtHDPath->getText());
		new_harddrive(unit_no);
	}

	ExitEditFilesysHardDrive();

	return dialogResult;
}
