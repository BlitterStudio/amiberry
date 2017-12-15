#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#endif
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "memory.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "gui.h"
#include "gui_handling.h"

#include "inputdevice.h"

#ifdef ANDROIDSDL
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 520
#define DIALOG_HEIGHT 202

static SDL_Joystick* GUIjoy;
extern struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];

extern string volName;

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
static gcn::UaeCheckBox* chkReadWrite;
static gcn::UaeCheckBox* chkAutoboot;
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
			strncpy(tmp, txtPath->getText().c_str(), MAX_PATH);
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
	txtDevice->setId("virtDev");

	lblVolume = new gcn::Label("Volume Label:");
	lblVolume->setAlignment(gcn::Graphics::RIGHT);
	txtVolume = new gcn::TextField();
	txtVolume->setSize(60, TEXTFIELD_HEIGHT);
	txtVolume->setId("virtVol");

	lblPath = new gcn::Label("Path:");
	lblPath->setAlignment(gcn::Graphics::RIGHT);
	txtPath = new gcn::TextField();
	txtPath->setSize(338, TEXTFIELD_HEIGHT);
	txtPath->setEnabled(false);
	cmdPath = new gcn::Button("...");
	cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdPath->setBaseColor(gui_baseCol);
	cmdPath->setId("virtPath");
	cmdPath->addActionListener(filesysVirtualActionListener);

	chkReadWrite = new gcn::UaeCheckBox("Read/Write", true);
	chkReadWrite->setId("virtRW");

	chkAutoboot = new gcn::UaeCheckBox("Bootable", true);
	chkAutoboot->setId("virtAutoboot");

	lblBootPri = new gcn::Label("Boot priority:");
	lblBootPri->setAlignment(gcn::Graphics::RIGHT);
	txtBootPri = new gcn::TextField();
	txtBootPri->setSize(40, TEXTFIELD_HEIGHT);
	txtBootPri->setId("virtBootpri");

	int posY = DISTANCE_BORDER;
	int posX = DISTANCE_BORDER;

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
	posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

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
	FocusBugWorkaround(wndEditFilesysVirtual);

	while (!dialogFinished)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN)
			{
#ifdef USE_SDL1
				switch (event.key.keysym.sym)
#elif USE_SDL2
				switch (event.key.keysym.scancode)
#endif
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

				case VK_Red:
				case VK_Green:
#ifdef USE_SDL1
					event.key.keysym.sym = SDLK_RETURN;
#elif USE_SDL2
					event.key.keysym.scancode = SDL_SCANCODE_RETURN;
#endif
					gui_input->pushInput(event); // Fire key down
					event.type = SDL_KEYUP; // and the key up
					break;
				default:
					break;
				}
			}
			else if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYHATMOTION)
			{
				gcn::FocusHandler* focusHdl;
				gcn::Widget* activeWidget;

				const int hat = SDL_JoystickGetHat(GUIjoy, 0);

				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_up) || (hat & SDL_HAT_UP)) // dpad
				{
					if (HandleNavigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_down) || (hat & SDL_HAT_DOWN)) // dpad
				{
					if (HandleNavigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_right) || (hat & SDL_HAT_RIGHT)) // dpad
				{
					if (HandleNavigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].dpad_left) || (hat & SDL_HAT_LEFT)) // dpad
				{
					if (HandleNavigation(DIRECTION_LEFT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_LEFT);
					break;
				}
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].south_button)) // need this to be X button
				{
					PushFakeKey(SDLK_RETURN);
					break;
				}
				if (SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].east_button) ||
					SDL_JoystickGetButton(GUIjoy, host_input_buttons[0].start_button)) // need this to be START button
				{
					dialogFinished = true;
					break;
				}
			}
			//-------------------------------------------------
			// Send event to guichan-controls
			//-------------------------------------------------
#ifdef ANDROIDSDL
			androidsdl_event(event, gui_input);
#else
			gui_input->pushInput(event);
#endif
		}

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
	struct mountedinfo mi;
	struct uaedev_config_data* uci;
	string strdevname, strvolname, strroot;
	char tmp[32];

	dialogResult = false;
	dialogFinished = false;

	InitEditFilesysVirtual();

	if (unit_no >= 0)
	{
		uci = &changed_prefs.mountconfig[unit_no];
		struct uaedev_config_info* ci = &uci->ci;
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);

		strdevname.assign(ci->devname);
		txtDevice->setText(strdevname);
		strvolname.assign(ci->volname);
		txtVolume->setText(strvolname);
		strroot.assign(ci->rootdir);
		txtPath->setText(strroot);
		chkReadWrite->setSelected(!ci->readonly);
		chkAutoboot->setSelected(ci->bootpri != BOOTPRI_NOAUTOBOOT);
		snprintf(tmp, sizeof tmp, "%d", ci->bootpri >= -127 ? ci->bootpri : -127);
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

	EditFilesysVirtualLoop();

	if (dialogResult)
	{
		struct uaedev_config_info ci;
		const int bp = tweakbootpri(atoi(txtBootPri->getText().c_str()), chkAutoboot->isSelected() ? 1 : 0, 0);
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
			struct hardfiledata* hfd = get_hardfile_data(uci->configoffset);
			hardfile_media_change(hfd, &ci, true, false);
		}
	}

	ExitEditFilesysVirtual();

	return dialogResult;
}
