#include <stdbool.h>
#include <string.h>
#include <stdio.h>

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

#include "inputdevice.h"
#include "amiberry_gfx.h"

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
			}
			wndEditFilesysVirtual->requestModalFocus();
			cmdPath->requestFocus();
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

	int gotEvent = 0;
	SDL_Event event;
	SDL_Event touch_event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
			gotEvent = 1;
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
		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				gotEvent = 1;
				const int hat = SDL_JoystickGetHat(gui_joystick, 0);

				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_up) || (hat & SDL_HAT_UP) || SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_y) == -32768) // dpad
				{
					if (HandleNavigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_down) || (hat & SDL_HAT_DOWN) || SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_y) == 32767) // dpad
				{
					if (HandleNavigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_right) || (hat & SDL_HAT_RIGHT) || SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_x) == 32767) // dpad
				{
					if (HandleNavigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_left) || (hat & SDL_HAT_LEFT) || SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_x) == -32768) // dpad
				{
					if (HandleNavigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_LEFT);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].south_button)) // need this to be X button
				{
					PushFakeKey(SDLK_RETURN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button) ||
					SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].start_button)) // need this to be START button
				{
					dialogFinished = true;
					break;
				}
			}
			break;

		case SDL_FINGERDOWN:
			gotEvent = 1;
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
			gotEvent = 1;
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
			gotEvent = 1;
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
			gotEvent = 1;
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
	
	if (gotEvent)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		UpdateGuiScreen();
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
		const auto ci = &uci->ci;
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);

		strdevname.assign(ci->devname);
		txtDevice->setText(strdevname);
		strvolname.assign(ci->volname);
		txtVolume->setText(strvolname);
		strroot.assign(ci->rootdir);
		txtPath->setText(strroot);
		chkReadWrite->setSelected(!ci->readonly);
		chkAutoboot->setSelected(ci->bootpri != BOOTPRI_NOAUTOBOOT);
		snprintf(tmp, 32, "%d", ci->bootpri >= -127 ? ci->bootpri : -127);
		txtBootPri->setText(tmp);
	}
	else
	{
		CreateDefaultDevicename(tmp);
		txtDevice->setText(tmp);
		txtVolume->setText(tmp);
		strroot.assign(currentDir);
		txtPath->setText(strroot);
		chkReadWrite->setSelected(true);
		txtBootPri->setText("0");
	}

	// Prepare the screen once
	uae_gui->logic();
	uae_gui->draw();
	UpdateGuiScreen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		EditFilesysVirtualLoop();
		cap_fps(start, 60);
	}

	if (dialogResult)
	{
		struct uaedev_config_info ci{};
		const auto bp = tweakbootpri(atoi(txtBootPri->getText().c_str()), chkAutoboot->isSelected() ? 1 : 0, 0);
		extractPath(const_cast<char *>(txtPath->getText().c_str()), currentDir);

		uci_set_defaults(&ci, true);
		strncpy(ci.devname, const_cast<char *>(txtDevice->getText().c_str()), MAX_DPATH);
		strncpy(ci.volname, const_cast<char *>(txtVolume->getText().c_str()), MAX_DPATH);
		strncpy(ci.rootdir, const_cast<char *>(txtPath->getText().c_str()), MAX_DPATH);
		ci.type = UAEDEV_DIR;
		ci.readonly = !chkReadWrite->isSelected();
		ci.bootpri = bp;

		uci = add_filesys_config(&changed_prefs, unit_no, &ci);
		if (uci)
		{
			filesys_media_change (ci.rootdir, 1, uci);
		}
	}

	ExitEditFilesysVirtual();

	return dialogResult;
}
