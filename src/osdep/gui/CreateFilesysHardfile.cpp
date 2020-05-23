#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>

#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "filesys.h"
#include "gui_handling.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"

#ifdef ANDROID
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 620
#define DIALOG_HEIGHT 202

static const char *harddisk_filter[] = {".hdf", "\0"};

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;

static gcn::Window *wndCreateFilesysHardfile;
static gcn::Button *cmdOK;
static gcn::Button *cmdCancel;
static gcn::Label *lblDevice;
static gcn::TextField *txtDevice;
static gcn::CheckBox *chkAutoboot;
static gcn::Label *lblBootPri;
static gcn::TextField *txtBootPri;
static gcn::Label *lblPath;
static gcn::TextField *txtPath;
static gcn::Button *cmdPath;
static gcn::Label *lblSize;
static gcn::TextField *txtSize;

class CreateFilesysHardfileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent &actionEvent) override
	{
		if (actionEvent.getSource() == cmdPath)
		{
			char tmp[MAX_DPATH];
			strncpy(tmp, txtPath->getText().c_str(), MAX_DPATH);
			wndCreateFilesysHardfile->releaseModalFocus();
			if (SelectFile("Create harddisk file", tmp, harddisk_filter, true))
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
				if (txtDevice->getText().length() <= 0)
				{
					wndCreateFilesysHardfile->setCaption("Please enter a device name.");
					return;
				}
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
	wndCreateFilesysHardfile->setBaseColor(gui_baseCol);
	wndCreateFilesysHardfile->setCaption("Create hardfile");
	wndCreateFilesysHardfile->setTitleBarHeight(TITLEBAR_HEIGHT);

	createFilesysHardfileActionListener = new CreateFilesysHardfileActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
					   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->setId("createHdfOK");
	cmdOK->addActionListener(createFilesysHardfileActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
						   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
	cmdCancel->setId("createHdfCancel");
	cmdCancel->addActionListener(createFilesysHardfileActionListener);

	lblDevice = new gcn::Label("Device Name:");
	lblDevice->setAlignment(gcn::Graphics::RIGHT);
	txtDevice = new gcn::TextField();
	txtDevice->setSize(80, TEXTFIELD_HEIGHT);

	chkAutoboot = new gcn::CheckBox("Bootable", true);
	chkAutoboot->setId("createHdfAutoboot");

	lblBootPri = new gcn::Label("Boot priority:");
	lblBootPri->setAlignment(gcn::Graphics::RIGHT);
	txtBootPri = new gcn::TextField();
	txtBootPri->setSize(40, TEXTFIELD_HEIGHT);

	lblSize = new gcn::Label("Size (MB):");
	lblSize->setAlignment(gcn::Graphics::RIGHT);
	txtSize = new gcn::TextField();
	txtSize->setSize(60, TEXTFIELD_HEIGHT);

	lblPath = new gcn::Label("Path:");
	lblPath->setAlignment(gcn::Graphics::RIGHT);
	txtPath = new gcn::TextField();
	txtPath->setSize(500, TEXTFIELD_HEIGHT);

	cmdPath = new gcn::Button("...");
	cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdPath->setBaseColor(gui_baseCol);
	cmdPath->setId("createHdfPath");
	cmdPath->addActionListener(createFilesysHardfileActionListener);

	int posY = DISTANCE_BORDER;
	int posX = DISTANCE_BORDER;

	wndCreateFilesysHardfile->add(lblDevice, DISTANCE_BORDER, posY);
	posX += lblDevice->getWidth() + 8;

	wndCreateFilesysHardfile->add(txtDevice, posX, posY);
	posX += txtDevice->getWidth() + DISTANCE_BORDER * 2;

	wndCreateFilesysHardfile->add(chkAutoboot, posX, posY + 1);
	posX += chkAutoboot->getWidth() + DISTANCE_BORDER;

	wndCreateFilesysHardfile->add(lblBootPri, posX, posY);
	wndCreateFilesysHardfile->add(txtBootPri, posX + lblBootPri->getWidth() + 8, posY);
	posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

	wndCreateFilesysHardfile->add(lblPath, DISTANCE_BORDER, posY);
	wndCreateFilesysHardfile->add(txtPath, DISTANCE_BORDER + lblPath->getWidth() + 8, posY);
	wndCreateFilesysHardfile->add(cmdPath, wndCreateFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH,
								  posY);
	posY += txtPath->getHeight() + DISTANCE_NEXT_Y;

	wndCreateFilesysHardfile->add(lblSize, lblDevice->getX(), posY);
	wndCreateFilesysHardfile->add(txtSize, txtDevice->getX(), posY);

	wndCreateFilesysHardfile->add(cmdOK);
	wndCreateFilesysHardfile->add(cmdCancel);

	gui_top->add(wndCreateFilesysHardfile);

	txtDevice->requestFocus();
	wndCreateFilesysHardfile->requestModalFocus();
}

static void ExitCreateFilesysHardfile()
{
	wndCreateFilesysHardfile->releaseModalFocus();
	gui_top->remove(wndCreateFilesysHardfile);

	delete lblDevice;
	delete txtDevice;
	delete chkAutoboot;
	delete lblBootPri;
	delete txtBootPri;
	delete lblPath;
	delete txtPath;
	delete cmdPath;
	delete lblSize;
	delete txtSize;

	delete cmdOK;
	delete cmdCancel;
	delete createFilesysHardfileActionListener;

	delete wndCreateFilesysHardfile;
}

static void CreateFilesysHardfileLoop()
{
	//FocusBugWorkaround(wndCreateFilesysHardfile);

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
				event.type = SDL_KEYUP;		 // and the key up
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

				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_up) || (hat & SDL_HAT_UP) ||
					SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_y) == -32768) // dpad
				{
					if (HandleNavigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_down) || (hat & SDL_HAT_DOWN) ||
					SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_y) == 32767) // dpad
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
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].dpad_left) || (hat & SDL_HAT_LEFT) ||
					SDL_JoystickGetAxis(gui_joystick, host_input_buttons[0].lstick_axis_x) == -32768) // dpad
				{
					if (HandleNavigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_LEFT);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].south_button))
					// need this to be X button
				{
					PushFakeKey(SDLK_RETURN);
					continue;
				}
				if (SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].east_button) ||
					SDL_JoystickGetButton(gui_joystick, host_input_buttons[0].start_button))
					// need this to be START button
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
		// Send event to gui-controls
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

bool CreateFilesysHardfile()
{
	std::string strroot;
	char tmp[32];
	char zero = 0;

	dialogResult = false;
	dialogFinished = false;

	InitCreateFilesysHardfile();

	CreateDefaultDevicename(tmp);
	txtDevice->setText(tmp);
	strroot.assign(currentDir);
	txtPath->setText(strroot);
	fileSelected = false;

	txtBootPri->setText("0");
	txtSize->setText("100");

	// Prepare the screen once
	uae_gui->logic();
	uae_gui->draw();
	UpdateGuiScreen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		CreateFilesysHardfileLoop();
		cap_fps(start, 60);
	}

	if (dialogResult)
	{
		long long size = atoi(txtSize->getText().c_str());
		if (size < 1)
			size = 1;

		const auto bp = tweakbootpri(atoi(txtBootPri->getText().c_str()), 1, 0);

		auto* const newFile = fopen(txtPath->getText().c_str(), "wbe");
		if (!newFile)
		{
			write_log("Unable to create new file.");
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
			write_log("Unable to create new file size.");
			fclose(newFile);
			ExitCreateFilesysHardfile();
			return false;
		}
		
		struct uaedev_config_info ci
		{
		};

		uci_set_defaults(&ci, false);
		strncpy(ci.devname, const_cast<char *>(txtDevice->getText().c_str()), MAX_DPATH);
		strncpy(ci.rootdir, const_cast<char *>(txtPath->getText().c_str()), MAX_DPATH);
		ci.type = UAEDEV_HDF;
		ci.surfaces = (size / 1024) + 1;
		ci.bootpri = bp;

		ci.controller_type = 0;
		ci.controller_type_unit = 0;
		ci.controller_unit = 0;
		ci.controller_media_type = 0;
		ci.unit_feature_level = 1;
		ci.readonly = false;
		auto* const uci = add_filesys_config(&changed_prefs, -1, &ci);
		if (uci)
		{
			auto* const hfd = get_hardfile_data(uci->configoffset);
			hardfile_media_change(hfd, &ci, true, false);
		}
	}

	ExitCreateFilesysHardfile();
	return dialogResult;
}
