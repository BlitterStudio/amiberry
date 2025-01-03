#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>

#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "filesys.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"

enum
{
	DIALOG_WIDTH = 620,
	DIALOG_HEIGHT = 202
};

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;

static gcn::Window *wndCreateFilesysHardfile;
static gcn::Button *cmdOK;
static gcn::Button *cmdCancel;
static gcn::Label *lblPath;
static gcn::TextField *txtPath;
static gcn::Button *cmdPath;
static gcn::Label *lblSize;
static gcn::TextField *txtSize;
static gcn::CheckBox* chkDynamic;

class CreateFilesysHardfileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent &actionEvent) override
	{
		if (actionEvent.getSource() == cmdPath)
		{
			wndCreateFilesysHardfile->releaseModalFocus();
			std::string path;
			if (txtPath->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtPath->getText();
			}
			const std::string tmp = SelectFile("Create hard disk file", path, harddisk_filter, true);
			if (!tmp.empty())
			{
				txtPath->setText(tmp);
				fileSelected = true;
			}
			wndCreateFilesysHardfile->requestModalFocus();
			cmdPath->requestFocus();
		}
		else
		{
			if (actionEvent.getSource() == cmdOK)
			{
				if (!fileSelected)
				{
					wndCreateFilesysHardfile->setCaption("Please select a new filename.");
					return;
				}
				dialogResult = true;
			}
			dialogFinished = true;
		}
	}
};

static CreateFilesysHardfileActionListener *createFilesysHardfileActionListener;

static void InitCreateFilesysHardfile()
{
	wndCreateFilesysHardfile = new gcn::Window("Create");
	wndCreateFilesysHardfile->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndCreateFilesysHardfile->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndCreateFilesysHardfile->setBaseColor(gui_base_color);
	wndCreateFilesysHardfile->setForegroundColor(gui_foreground_color);
	wndCreateFilesysHardfile->setCaption("Create hardfile");
	wndCreateFilesysHardfile->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndCreateFilesysHardfile->setMovable(false);

	createFilesysHardfileActionListener = new CreateFilesysHardfileActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
					   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->setId("cmdCreateHdfOK");
	cmdOK->addActionListener(createFilesysHardfileActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
						   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_base_color);
	cmdCancel->setForegroundColor(gui_foreground_color);
	cmdCancel->setId("cmdCreateHdfCancel");
	cmdCancel->addActionListener(createFilesysHardfileActionListener);

	lblSize = new gcn::Label("Size (MB):");
	lblSize->setAlignment(gcn::Graphics::Right);
	txtSize = new gcn::TextField();
	txtSize->setSize(60, TEXTFIELD_HEIGHT);
	txtSize->setBaseColor(gui_base_color);
	txtSize->setBackgroundColor(gui_background_color);
	txtSize->setForegroundColor(gui_foreground_color);

	chkDynamic = new gcn::CheckBox("Dynamic VHD", true);
	chkDynamic->setBaseColor(gui_base_color);
	chkDynamic->setBackgroundColor(gui_background_color);
	chkDynamic->setForegroundColor(gui_foreground_color);
	chkDynamic->setId("chkDynamic");

	lblPath = new gcn::Label("Path:");
	lblPath->setAlignment(gcn::Graphics::Right);
	txtPath = new gcn::TextField();
	txtPath->setId("txtCreatePath");
	txtPath->setSize(500, TEXTFIELD_HEIGHT);
	txtPath->setBaseColor(gui_base_color);
	txtPath->setBackgroundColor(gui_background_color);
	txtPath->setForegroundColor(gui_foreground_color);

	cmdPath = new gcn::Button("...");
	cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdPath->setBaseColor(gui_base_color);
	cmdPath->setForegroundColor(gui_foreground_color);
	cmdPath->setId("cmdCreateHdfPath");
	cmdPath->addActionListener(createFilesysHardfileActionListener);

	int posY = DISTANCE_BORDER;

	wndCreateFilesysHardfile->add(lblPath, DISTANCE_BORDER, posY);
	wndCreateFilesysHardfile->add(txtPath, DISTANCE_BORDER + lblPath->getWidth() + 8, posY);
	wndCreateFilesysHardfile->add(cmdPath, wndCreateFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH,
								  posY);
	posY += txtPath->getHeight() + DISTANCE_NEXT_Y;

	wndCreateFilesysHardfile->add(lblSize, lblPath->getX(), posY);
	wndCreateFilesysHardfile->add(txtSize, lblSize->getX() + lblSize->getWidth() + 8, posY);
	wndCreateFilesysHardfile->add(chkDynamic, txtSize->getX() + txtSize->getWidth() + DISTANCE_NEXT_X, posY);

	wndCreateFilesysHardfile->add(cmdOK);
	wndCreateFilesysHardfile->add(cmdCancel);

	gui_top->add(wndCreateFilesysHardfile);

	wndCreateFilesysHardfile->requestModalFocus();
	focus_bug_workaround(wndCreateFilesysHardfile);
	cmdPath->requestFocus();
}

static void ExitCreateFilesysHardfile()
{
	wndCreateFilesysHardfile->releaseModalFocus();
	gui_top->remove(wndCreateFilesysHardfile);

	delete lblPath;
	delete txtPath;
	delete cmdPath;
	delete lblSize;
	delete txtSize;
	delete chkDynamic;

	delete cmdOK;
	delete cmdCancel;
	delete createFilesysHardfileActionListener;

	delete wndCreateFilesysHardfile;
}

static void CreateFilesysHardfileLoop()
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
					continue;
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
		// Send event to gui-controls
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

bool CreateFilesysHardfile()
{
	const AmigaMonitor* mon = &AMonitors[0];
	char zero = 0;

	dialogResult = false;
	dialogFinished = false;

	InitCreateFilesysHardfile();

	fileSelected = false;

	txtSize->setText("100");
	chkDynamic->setSelected(false);

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		CreateFilesysHardfileLoop();
		cap_fps(start);
	}

	if (dialogResult)
	{
		long long size = atoi(txtSize->getText().c_str());
		if (size < 1)
			size = 1;

		char init_path[MAX_DPATH];
		_tcsncpy(init_path, txtPath->getText().c_str(), MAX_DPATH - 1);
		if (chkDynamic->isSelected()) {
			if (_tcslen(init_path) > 4 && !_tcsicmp(init_path + _tcslen(init_path) - 4, _T(".hdf")))
				_tcscpy(init_path + _tcslen(init_path) - 4, _T(".vhd"));
			const bool result = vhd_create(init_path, size * 1024 * 1024, 0);
			if (!result) {
				ShowMessage("Create Hardfile", "Unable to create new VHD file.", "", "", "Ok", "");
				ExitCreateFilesysHardfile();
				dialogResult = false;
			}
		}
		else {
			FILE* newFile = fopen(init_path, "wb");
			if (!newFile)
			{
				ShowMessage("Create Hardfile", "Unable to create new file.", "", "", "Ok", "");
				ExitCreateFilesysHardfile();
				return false;
			}
			if (_fseeki64(newFile, size * 1024 * 1024 - 1, SEEK_SET) == 0)
			{
				fwrite(&zero, 1, 1, newFile);
				fclose(newFile);
			}
			else
			{
				fclose(newFile);
				ShowMessage("Create Hardfile", "Unable to create new file size.", "", "", "Ok", "");
				ExitCreateFilesysHardfile();
				dialogResult = false;
			}
		}
	}

	ExitCreateFilesysHardfile();
	return dialogResult;
}
