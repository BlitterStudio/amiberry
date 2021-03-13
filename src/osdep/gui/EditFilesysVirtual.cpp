#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "autoconf.h"
#include "gui_handling.h"

#include "amiberry_gfx.h"
#include "amiberry_input.h"

#ifdef ANDROID
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 520
#define DIALOG_HEIGHT 202

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
static gcn::Button* cmdPath;
static gcn::CheckBox* chkReadWrite;
static gcn::CheckBox* chkAutoboot;
static gcn::Label* lblBootPri;
static gcn::TextField* txtBootPri;


class FilesysVirtualActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdPath)
		{
			char tmp[MAX_DPATH];
			strncpy(tmp, txtPath->getText().c_str(), MAX_DPATH);
			wndEditFilesysVirtual->releaseModalFocus();
			if (SelectFolder("Select folder", tmp))
			{
				txtPath->setText(tmp);
				txtVolume->setText(volName);
				default_fsvdlg(&current_fsvdlg);
				CreateDefaultDevicename(current_fsvdlg.ci.devname);
				_tcscpy(current_fsvdlg.ci.volname, current_fsvdlg.ci.devname);
				_tcscpy(current_fsvdlg.ci.rootdir, tmp);
			}
			wndEditFilesysVirtual->requestModalFocus();
			cmdPath->requestFocus();
		}
		else if (actionEvent.getSource() == chkAutoboot) {
			char tmp[32];
			if (chkAutoboot->isSelected()) {
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
				if (txtDevice->getText().length() <= 0)
				{
					wndEditFilesysVirtual->setCaption("Please enter a device name.");
					return;
				}
				if (txtVolume->getText().length() <= 0)
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
	wndEditFilesysVirtual->setBaseColor(gui_baseCol);
	wndEditFilesysVirtual->setCaption("Volume settings");
	wndEditFilesysVirtual->setTitleBarHeight(TITLEBAR_HEIGHT);

	filesysVirtualActionListener = new FilesysVirtualActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->setId("virtOK");
	cmdOK->addActionListener(filesysVirtualActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                       DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
	cmdCancel->setId("virtCancel");
	cmdCancel->addActionListener(filesysVirtualActionListener);

	lblDevice = new gcn::Label("Device Name:");
	lblDevice->setAlignment(gcn::Graphics::RIGHT);
	txtDevice = new gcn::TextField();
	txtDevice->setSize(60, TEXTFIELD_HEIGHT);

	lblVolume = new gcn::Label("Volume Label:");
	lblVolume->setAlignment(gcn::Graphics::RIGHT);
	txtVolume = new gcn::TextField();
	txtVolume->setSize(60, TEXTFIELD_HEIGHT);

	lblPath = new gcn::Label("Path:");
	lblPath->setAlignment(gcn::Graphics::RIGHT);
	txtPath = new gcn::TextField();
	txtPath->setSize(338, TEXTFIELD_HEIGHT);
	
	cmdPath = new gcn::Button("...");
	cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdPath->setBaseColor(gui_baseCol);
	cmdPath->setId("virtPath");
	cmdPath->addActionListener(filesysVirtualActionListener);

	chkReadWrite = new gcn::CheckBox("Read/Write", true);
	chkReadWrite->setId("virtRW");

	chkAutoboot = new gcn::CheckBox("Bootable", true);
	chkAutoboot->setId("virtAutoboot");
	chkAutoboot->addActionListener(filesysVirtualActionListener);

	lblBootPri = new gcn::Label("Boot priority:");
	lblBootPri->setAlignment(gcn::Graphics::RIGHT);
	txtBootPri = new gcn::TextField();
	txtBootPri->setSize(40, TEXTFIELD_HEIGHT);

	auto posY = DISTANCE_BORDER;
	auto posX = DISTANCE_BORDER;

	wndEditFilesysVirtual->add(lblDevice, DISTANCE_BORDER, posY);
	posX += lblDevice->getWidth() + 8;

	wndEditFilesysVirtual->add(txtDevice, posX, posY);
	posX += txtDevice->getWidth() + DISTANCE_BORDER * 2;

	wndEditFilesysVirtual->add(chkReadWrite, posX, posY + 1);
	posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysVirtual->add(lblVolume, DISTANCE_BORDER, posY);
	wndEditFilesysVirtual->add(txtVolume, txtDevice->getX(), posY);

	wndEditFilesysVirtual->add(chkAutoboot, chkReadWrite->getX(), posY + 1);
	posX += chkAutoboot->getWidth() + DISTANCE_BORDER * 2;

	wndEditFilesysVirtual->add(lblBootPri, posX, posY);
	wndEditFilesysVirtual->add(txtBootPri, posX + lblBootPri->getWidth() + 8, posY);
	posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysVirtual->add(lblPath, DISTANCE_BORDER, posY);
	wndEditFilesysVirtual->add(txtPath, DISTANCE_BORDER + lblDevice->getWidth() + 8, posY);
	wndEditFilesysVirtual->add(cmdPath, wndEditFilesysVirtual->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);

	wndEditFilesysVirtual->add(cmdOK);
	wndEditFilesysVirtual->add(cmdCancel);

	gui_top->add(wndEditFilesysVirtual);

	txtDevice->requestFocus();
	wndEditFilesysVirtual->requestModalFocus();
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
	delete cmdPath;
	delete chkReadWrite;
	delete chkAutoboot;
	delete lblBootPri;
	delete txtBootPri;

	delete cmdOK;
	delete cmdCancel;
	delete filesysVirtualActionListener;

	delete wndEditFilesysVirtual;
}


static void EditFilesysVirtualLoop()
{
	//FocusBugWorkaround(wndEditFilesysVirtual);

	int got_event = 0;
	SDL_Event event;
	SDL_Event touch_event;
	struct didata* did = &di_joystick[0];
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
				if (HandleNavigation(DIRECTION_UP))
					continue; // Don't change value when enter ComboBox -> don't send event to control
				break;

			case VK_DOWN:
				if (HandleNavigation(DIRECTION_DOWN))
					continue; // Don't change value when enter ComboBox -> don't send event to control
				break;

			case VK_LEFT:
				if (HandleNavigation(DIRECTION_LEFT))
					continue; // Don't change value when enter Slider -> don't send event to control
				break;

			case VK_RIGHT:
				if (HandleNavigation(DIRECTION_RIGHT))
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
					if (HandleNavigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || (hat & SDL_HAT_DOWN)) // dpad
				{
					if (HandleNavigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || (hat & SDL_HAT_RIGHT)) // dpad
				{
					if (HandleNavigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || (hat & SDL_HAT_LEFT)) // dpad
				{
					if (HandleNavigation(DIRECTION_LEFT))
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
						if (HandleNavigation(DIRECTION_RIGHT))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_RIGHT);
						break;
					}
					if (event.jaxis.value < -joystick_dead_zone && last_x != -1)
					{
						last_x = -1;
						if (HandleNavigation(DIRECTION_LEFT))
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
						if (HandleNavigation(DIRECTION_UP))
							continue; // Don't change value when enter Slider -> don't send event to control
						PushFakeKey(SDLK_UP);
						break;
					}
					if (event.jaxis.value > joystick_dead_zone && last_y != 1)
					{
						last_y = 1;
						if (HandleNavigation(DIRECTION_DOWN))
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
			touch_event.button.x = gui_graphics->getTarget()->w * event.tfinger.x;
			touch_event.button.y = gui_graphics->getTarget()->h * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERUP:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEBUTTONUP;
			touch_event.button.which = 0;
			touch_event.button.button = SDL_BUTTON_LEFT;
			touch_event.button.state = SDL_RELEASED;
			touch_event.button.x = gui_graphics->getTarget()->w * event.tfinger.x;
			touch_event.button.y = gui_graphics->getTarget()->h * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_FINGERMOTION:
			got_event = 1;
			memcpy(&touch_event, &event, sizeof event);
			touch_event.type = SDL_MOUSEMOTION;
			touch_event.motion.which = 0;
			touch_event.motion.state = 0;
			touch_event.motion.x = gui_graphics->getTarget()->w * event.tfinger.x;
			touch_event.motion.y = gui_graphics->getTarget()->h * event.tfinger.y;
			gui_input->pushInput(touch_event);
			break;

		case SDL_KEYUP:
		case SDL_JOYBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
			got_event = 1;
			break;
			
		default:
			break;
		}

		//-------------------------------------------------
		// Send event to guisan-controls
		//-------------------------------------------------
#ifdef ANDROID
		androidsdl_event(event, gui_input);
#else
		gui_input->pushInput(event);
#endif
	}
	
	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		SDL_RenderClear(sdl_renderer);
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}


bool EditFilesysVirtual(const int unit_no)
{
	struct mountedinfo mi{};
	struct uaedev_config_data* uci;
	std::string strdevname, strvolname, strroot;
	char tmp[32];

	dialogResult = false;
	dialogFinished = false;

	InitEditFilesysVirtual();

	if (unit_no >= 0)
	{
		uci = &changed_prefs.mountconfig[unit_no];
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);
		memcpy(&current_fsvdlg.ci, uci, sizeof(struct uaedev_config_info));
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
	strroot.assign(current_fsvdlg.ci.rootdir);
	txtPath->setText(strroot);
	chkReadWrite->setSelected(!current_fsvdlg.ci.readonly);
	chkAutoboot->setSelected(current_fsvdlg.ci.bootpri != BOOTPRI_NOAUTOBOOT);
	snprintf(tmp, sizeof(tmp) - 1, "%d", current_fsvdlg.ci.bootpri >= -127 ? current_fsvdlg.ci.bootpri : -127);
	txtBootPri->setText(tmp);
	
	// Prepare the screen once
	uae_gui->logic();
	SDL_RenderClear(sdl_renderer);
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
		struct uaedev_config_info ci{};
		
		strncpy(current_fsvdlg.ci.devname, (char*)txtDevice->getText().c_str(), MAX_DPATH - 1);
		strncpy(current_fsvdlg.ci.volname, (char*)txtVolume->getText().c_str(), MAX_DPATH - 1);
		current_fsvdlg.ci.readonly = !chkReadWrite->isSelected();
		current_fsvdlg.ci.bootpri = tweakbootpri(atoi(txtBootPri->getText().c_str()), chkAutoboot->isSelected() ? 1 : 0, 0);

		memcpy(&ci, &current_fsvdlg.ci, sizeof(struct uaedev_config_info));
		uci = add_filesys_config(&changed_prefs, unit_no, &ci);
		if (uci)
		{
			if (uci->ci.rootdir[0])
				filesys_media_change(uci->ci.rootdir, unit_no, uci);
			else if (uci->configoffset >= 0)
				filesys_eject(uci->configoffset);
		}

		extract_path((char*)txtPath->getText().c_str(), current_dir);
	}

	ExitEditFilesysVirtual();

	return dialogResult;
}
