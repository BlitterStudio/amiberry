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

enum
{
	DIALOG_WIDTH = 600,
	DIALOG_HEIGHT = 250
};

extern std::string volName;

static bool dialogResult = false;
static bool dialogFinished = false;

static gcn::Window* wndEditFilesysVirtual;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::Label* lblDevice;
static gcn::TextField* txtDevice;
static gcn::Label* lblVolume;
static gcn::TextField* txtVolume;
static gcn::Label* lblPath;
static gcn::TextField* txtPath;
static gcn::CheckBox* chkReadWrite;
static gcn::CheckBox* chkVirtBootable;
static gcn::Label* lblBootPri;
static gcn::TextField* txtBootPri;
static gcn::Button* cmdVirtSelectDir;
static gcn::Button* cmdVirtSelectFile;

class FilesysVirtualActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdVirtSelectDir)
		{
			wndEditFilesysVirtual->releaseModalFocus();
			std::string path;
			if (txtPath->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtPath->getText();
			}
			const std::string tmp = SelectFolder("Select folder", path);
			if (!tmp.empty())
			{
				txtPath->setText(tmp);
				txtVolume->setText(volName);
				default_fsvdlg(&current_fsvdlg);
				if (current_fsvdlg.ci.devname[0] == 0)
					CreateDefaultDevicename(current_fsvdlg.ci.devname);
				_tcscpy(current_fsvdlg.ci.volname, current_fsvdlg.ci.devname);
				_tcscpy(current_fsvdlg.ci.rootdir, tmp.c_str());
			}
			wndEditFilesysVirtual->requestModalFocus();
			cmdVirtSelectDir->requestFocus();
		}
		else if (actionEvent.getSource() == cmdVirtSelectFile)
		{
			wndEditFilesysVirtual->releaseModalFocus();
			std::string path;
			if (txtPath->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtPath->getText();
			}
			const std::string tmp = SelectFile("Select archive", path, archive_filter);
			if (!tmp.empty())
			{
				txtPath->setText(tmp);
				TCHAR* s = filesys_createvolname(nullptr, tmp.c_str(), nullptr, _T("Harddrive"));
				txtVolume->setText(std::string(s));
				default_fsvdlg(&current_fsvdlg);
				if (current_fsvdlg.ci.devname[0] == 0)
					CreateDefaultDevicename(current_fsvdlg.ci.devname);
				_tcscpy(current_fsvdlg.ci.volname, current_fsvdlg.ci.devname);
				_tcscpy(current_fsvdlg.ci.rootdir, tmp.c_str());
			}
			wndEditFilesysVirtual->requestModalFocus();
			cmdVirtSelectFile->requestFocus();
		}
		else if (actionEvent.getSource() == chkVirtBootable) {
			char tmp[32];
			if (chkVirtBootable->isSelected()) {
				current_fsvdlg.ci.bootpri = 0;
			}
			else {
				current_fsvdlg.ci.bootpri = BOOTPRI_NOAUTOBOOT;
			}
			snprintf(tmp, sizeof(tmp) - 1, "%d", current_fsvdlg.ci.bootpri);
			txtBootPri->setText(tmp);
		}
		else
		{
			if (actionEvent.getSource() == cmdOK)
			{
				if (txtDevice->getText().empty())
				{
					wndEditFilesysVirtual->setCaption("Please enter a device name.");
					return;
				}
				if (txtVolume->getText().empty())
				{
					wndEditFilesysVirtual->setCaption("Please enter a volume name.");
					return;
				}
				dialogResult = true;
			}
			dialogFinished = true;
		}
	}
};

static FilesysVirtualActionListener* filesysVirtualActionListener;

static void InitEditFilesysVirtual()
{
	wndEditFilesysVirtual = new gcn::Window("Edit");
	wndEditFilesysVirtual->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndEditFilesysVirtual->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndEditFilesysVirtual->setBaseColor(gui_base_color);
	wndEditFilesysVirtual->setForegroundColor(gui_foreground_color);
	wndEditFilesysVirtual->setCaption("Volume settings");
	wndEditFilesysVirtual->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndEditFilesysVirtual->setMovable(false);

	filesysVirtualActionListener = new FilesysVirtualActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->setId("cmdVirtOK");
	cmdOK->addActionListener(filesysVirtualActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                       DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_base_color);
	cmdCancel->setForegroundColor(gui_foreground_color);
	cmdCancel->setId("cmdVirtCancel");
	cmdCancel->addActionListener(filesysVirtualActionListener);

	lblDevice = new gcn::Label("Device Name:");
	lblDevice->setAlignment(gcn::Graphics::Right);
	txtDevice = new gcn::TextField();
	txtDevice->setSize(120, TEXTFIELD_HEIGHT);
	txtDevice->setId("txtVirtDevice");
	txtDevice->setBaseColor(gui_base_color);
	txtDevice->setBackgroundColor(gui_background_color);
	txtDevice->setForegroundColor(gui_foreground_color);

	lblVolume = new gcn::Label("Volume Label:");
	lblVolume->setAlignment(gcn::Graphics::Right);
	txtVolume = new gcn::TextField();
	txtVolume->setSize(120, TEXTFIELD_HEIGHT);
	txtVolume->setId("txtVirtVolume");
	txtVolume->setBaseColor(gui_base_color);
	txtVolume->setBackgroundColor(gui_background_color);
	txtVolume->setForegroundColor(gui_foreground_color);

	lblPath = new gcn::Label("Path:");
	lblPath->setWidth(lblVolume->getWidth());
	lblPath->setAlignment(gcn::Graphics::Right);
	txtPath = new gcn::TextField();
	txtPath->setSize(450, TEXTFIELD_HEIGHT);
	txtPath->setId("txtVirtPath");
	txtPath->setBaseColor(gui_base_color);
	txtPath->setBackgroundColor(gui_background_color);
	txtPath->setForegroundColor(gui_foreground_color);
	
	cmdVirtSelectDir = new gcn::Button("Select Directory");
	cmdVirtSelectDir->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdVirtSelectDir->setBaseColor(gui_base_color);
	cmdVirtSelectDir->setForegroundColor(gui_foreground_color);
	cmdVirtSelectDir->setId("cmdVirtSelectDir");
	cmdVirtSelectDir->addActionListener(filesysVirtualActionListener);

	cmdVirtSelectFile = new gcn::Button("Select Archive");
	cmdVirtSelectFile->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdVirtSelectFile->setBaseColor(gui_base_color);
	cmdVirtSelectFile->setForegroundColor(gui_foreground_color);
	cmdVirtSelectFile->setId("cmdVirtSelectFile");
	cmdVirtSelectFile->addActionListener(filesysVirtualActionListener);

	chkReadWrite = new gcn::CheckBox("Read/Write", true);
	chkReadWrite->setBaseColor(gui_base_color);
	chkReadWrite->setForegroundColor(gui_foreground_color);
	chkReadWrite->setBackgroundColor(gui_background_color);
	chkReadWrite->setId("chkVirtRW");

	chkVirtBootable = new gcn::CheckBox("Bootable", true);
	chkVirtBootable->setId("chkVirtBootable");
	chkVirtBootable->setBaseColor(gui_base_color);
	chkVirtBootable->setBackgroundColor(gui_background_color);
	chkVirtBootable->setForegroundColor(gui_foreground_color);
	chkVirtBootable->addActionListener(filesysVirtualActionListener);

	lblBootPri = new gcn::Label("Boot priority:");
	lblBootPri->setAlignment(gcn::Graphics::Right);
	txtBootPri = new gcn::TextField();
	txtBootPri->setSize(45, TEXTFIELD_HEIGHT);
	txtBootPri->setBaseColor(gui_base_color);
	txtBootPri->setBackgroundColor(gui_background_color);
	txtBootPri->setForegroundColor(gui_foreground_color);

	int posY = DISTANCE_BORDER;
	int posX = DISTANCE_BORDER;

	wndEditFilesysVirtual->add(lblDevice, DISTANCE_BORDER, posY);
	posX += lblVolume->getWidth() + 8;

	wndEditFilesysVirtual->add(txtDevice, posX, posY);
	posX += txtDevice->getWidth() + DISTANCE_NEXT_X * 2;

	wndEditFilesysVirtual->add(chkReadWrite, posX, posY + 1);
	posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysVirtual->add(lblVolume, DISTANCE_BORDER, posY);
	wndEditFilesysVirtual->add(txtVolume, txtDevice->getX(), posY);

	wndEditFilesysVirtual->add(chkVirtBootable, chkReadWrite->getX(), posY + 1);
	posX += chkVirtBootable->getWidth() + DISTANCE_NEXT_Y * 3;

	wndEditFilesysVirtual->add(lblBootPri, posX, posY + 1);
	wndEditFilesysVirtual->add(txtBootPri, posX + lblBootPri->getWidth() + 8, posY);
	posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysVirtual->add(lblPath, DISTANCE_BORDER, posY);
	wndEditFilesysVirtual->add(txtPath, lblVolume->getX() + lblVolume->getWidth() + 8, posY);
	posY += txtPath->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysVirtual->add(cmdVirtSelectDir, txtPath->getX(), posY);
	wndEditFilesysVirtual->add(cmdVirtSelectFile, cmdVirtSelectDir->getX() + cmdVirtSelectDir->getWidth() + DISTANCE_NEXT_X, posY);

	wndEditFilesysVirtual->add(cmdOK);
	wndEditFilesysVirtual->add(cmdCancel);

	gui_top->add(wndEditFilesysVirtual);

	wndEditFilesysVirtual->requestModalFocus();
	focus_bug_workaround(wndEditFilesysVirtual);
	cmdVirtSelectDir->requestFocus();
}

static void ExitEditFilesysVirtual()
{
	wndEditFilesysVirtual->releaseModalFocus();
	gui_top->remove(wndEditFilesysVirtual);

	delete lblDevice;
	delete txtDevice;
	delete lblVolume;
	delete txtVolume;
	delete lblPath;
	delete txtPath;
	delete cmdVirtSelectDir;
	delete cmdVirtSelectFile;
	delete chkReadWrite;
	delete chkVirtBootable;
	delete lblBootPri;
	delete txtBootPri;

	delete cmdOK;
	delete cmdCancel;
	delete filesysVirtualActionListener;

	delete wndEditFilesysVirtual;
}

static void EditFilesysVirtualLoop()
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

bool EditFilesysVirtual(const int unit_no)
{
	const AmigaMonitor* mon = &AMonitors[0];

	mountedinfo mi{};
	std::string strdevname, strvolname;
	char tmp[32];

	dialogResult = false;
	dialogFinished = false;

	InitEditFilesysVirtual();

	if (unit_no >= 0)
	{
		uaedev_config_data* uci = &changed_prefs.mountconfig[unit_no];
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);
		memcpy(&current_fsvdlg.ci, uci, sizeof(uaedev_config_info));
	}
	else
	{
		default_fsvdlg(&current_fsvdlg);
		CreateDefaultDevicename(current_fsvdlg.ci.devname);
		_tcscpy(current_fsvdlg.ci.volname, current_fsvdlg.ci.devname);
	}
	strdevname.assign(current_fsvdlg.ci.devname);
	txtDevice->setText(strdevname);

	strvolname.assign(current_fsvdlg.ci.volname);
	txtVolume->setText(strvolname);

	txtPath->setText(std::string(current_fsvdlg.ci.rootdir));
	chkReadWrite->setSelected(!current_fsvdlg.ci.readonly);
	chkVirtBootable->setSelected(current_fsvdlg.ci.bootpri != BOOTPRI_NOAUTOBOOT);
	snprintf(tmp, sizeof(tmp) - 1, "%d", current_fsvdlg.ci.bootpri >= -127 ? current_fsvdlg.ci.bootpri : -127);
	txtBootPri->setText(tmp);
	
	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		EditFilesysVirtualLoop();
		cap_fps(start);
	}

	if (dialogResult)
	{
		strncpy(current_fsvdlg.ci.devname, txtDevice->getText().c_str(), MAX_DPATH - 1);
		strncpy(current_fsvdlg.ci.volname, txtVolume->getText().c_str(), MAX_DPATH - 1);
		current_fsvdlg.ci.readonly = !chkReadWrite->isSelected();
		current_fsvdlg.ci.bootpri = tweakbootpri(atoi(txtBootPri->getText().c_str()), chkVirtBootable->isSelected() ? 1 : 0, 0);

		new_filesys(unit_no);
	}

	ExitEditFilesysVirtual();

	return dialogResult;
}
