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

#define DIALOG_WIDTH 620
#define DIALOG_HEIGHT 272

static SDL_Joystick* GUIjoy;
extern struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];

static const char* harddisk_filter[] = {".hdf", "\0"};

struct controller_map
{
	int type;
	char display[64];
};

static struct controller_map controller[] = {
	{HD_CONTROLLER_TYPE_UAE, "UAE"},
	{HD_CONTROLLER_TYPE_IDE_FIRST, "A600/A1200/A4000 IDE"},
	{-1}
};

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;


static gcn::Window* wndEditFilesysHardfile;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::Label* lblDevice;
static gcn::TextField* txtDevice;
static gcn::UaeCheckBox* chkReadWrite;
static gcn::UaeCheckBox* chkAutoboot;
static gcn::Label* lblBootPri;
static gcn::TextField* txtBootPri;
static gcn::Label* lblPath;
static gcn::TextField* txtPath;
static gcn::Button* cmdPath;
static gcn::Label* lblSurfaces;
static gcn::TextField* txtSurfaces;
static gcn::Label* lblReserved;
static gcn::TextField* txtReserved;
static gcn::Label* lblSectors;
static gcn::TextField* txtSectors;
static gcn::Label* lblBlocksize;
static gcn::TextField* txtBlocksize;
static gcn::Label* lblController;
static gcn::UaeDropDown* cboController;
static gcn::UaeDropDown* cboUnit;

static void check_rdb(const TCHAR* filename)
{
	const bool isrdb = hardfile_testrdb(filename);
	if (isrdb)
	{
		txtSectors->setText("0");
		txtSurfaces->setText("0");
		txtReserved->setText("0");
		txtBootPri->setText("0");
	}
	txtSectors->setEnabled(!isrdb);
	txtSurfaces->setEnabled(!isrdb);
	txtReserved->setEnabled(!isrdb);
	txtBootPri->setEnabled(!isrdb);
}


class ControllerListModel : public gcn::ListModel
{
public:
	ControllerListModel()
	{
	}

	int getNumberOfElements() override
	{
		return 2;
	}

	string getElementAt(const int i) override
	{
		if (i < 0 || i >= 2)
			return "---";
		return controller[i].display;
	}
};

static ControllerListModel controllerListModel;


class UnitListModel : public gcn::ListModel
{
public:
	UnitListModel()
	{
	}

	int getNumberOfElements() override
	{
		return 4;
	}

	string getElementAt(const int i) override
	{
		char num[8];

		if (i < 0 || i >= 4)
			return "---";
		snprintf(num, 8, "%d", i);
		return num;
	}
};

static UnitListModel unitListModel;

class FilesysHardfileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdPath)
		{
			char tmp[MAX_DPATH];
			strncpy(tmp, txtPath->getText().c_str(), MAX_PATH);
			wndEditFilesysHardfile->releaseModalFocus();
			if (SelectFile("Select harddisk file", tmp, harddisk_filter))
			{
				txtPath->setText(tmp);
				fileSelected = true;
				check_rdb(tmp);
			}
			wndEditFilesysHardfile->requestModalFocus();
			cmdPath->requestFocus();
		}
		else if (actionEvent.getSource() == cboController)
		{
			switch (controller[cboController->getSelected()].type)
			{
			case HD_CONTROLLER_TYPE_UAE:
				cboUnit->setSelected(0);
				cboUnit->setEnabled(false);
				break;
			default:
				cboUnit->setEnabled(true);
				break;
			}
		}
		else
		{
			if (actionEvent.getSource() == cmdOK)
			{
				if (txtDevice->getText().length() <= 0)
				{
					wndEditFilesysHardfile->setCaption("Please enter a device name.");
					return;
				}
				if (!fileSelected)
				{
					wndEditFilesysHardfile->setCaption("Please select a filename.");
					return;
				}
				dialogResult = true;
			}
			dialogFinished = true;
		}
	}
};

static FilesysHardfileActionListener* filesysHardfileActionListener;


static void InitEditFilesysHardfile()
{
	for (int i = 0; expansionroms[i].name; i++)
	{
		const struct expansionromtype* erc = &expansionroms[i];
		if (erc->deviceflags & EXPANSIONTYPE_IDE)
		{
			for (int j = 0; controller[j].type >= 0; ++j)
			{
				if (!strcmp(erc->friendlyname, controller[j].display))
				{
					controller[j].type = HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST + i;
					break;
				}
			}
		}
	}

	wndEditFilesysHardfile = new gcn::Window("Edit");
	wndEditFilesysHardfile->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndEditFilesysHardfile->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndEditFilesysHardfile->setBaseColor(gui_baseCol);
	wndEditFilesysHardfile->setCaption("Volume settings");
	wndEditFilesysHardfile->setTitleBarHeight(TITLEBAR_HEIGHT);

	filesysHardfileActionListener = new FilesysHardfileActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
	                   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->setId("hdfOK");
	cmdOK->addActionListener(filesysHardfileActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
	                       DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
	cmdCancel->setId("hdfCancel");
	cmdCancel->addActionListener(filesysHardfileActionListener);

	lblDevice = new gcn::Label("Device Name:");
	lblDevice->setAlignment(gcn::Graphics::RIGHT);
	txtDevice = new gcn::TextField();
	txtDevice->setSize(60, TEXTFIELD_HEIGHT);
	txtDevice->setId("hdfDev");

	chkReadWrite = new gcn::UaeCheckBox("Read/Write", true);
	chkReadWrite->setId("hdfRW");

	chkAutoboot = new gcn::UaeCheckBox("Bootable", true);
	chkAutoboot->setId("hdfAutoboot");

	lblBootPri = new gcn::Label("Boot priority:");
	lblBootPri->setAlignment(gcn::Graphics::RIGHT);
	txtBootPri = new gcn::TextField();
	txtBootPri->setSize(40, TEXTFIELD_HEIGHT);
	txtBootPri->setId("hdfBootPri");

	lblSurfaces = new gcn::Label("Surfaces:");
	lblSurfaces->setAlignment(gcn::Graphics::RIGHT);
	txtSurfaces = new gcn::TextField();
	txtSurfaces->setSize(40, TEXTFIELD_HEIGHT);
	txtSurfaces->setId("hdfSurface");

	lblReserved = new gcn::Label("Reserved:");
	lblReserved->setAlignment(gcn::Graphics::RIGHT);
	txtReserved = new gcn::TextField();
	txtReserved->setSize(40, TEXTFIELD_HEIGHT);
	txtReserved->setId("hdfReserved");

	lblSectors = new gcn::Label("Sectors:");
	lblSectors->setAlignment(gcn::Graphics::RIGHT);
	txtSectors = new gcn::TextField();
	txtSectors->setSize(40, TEXTFIELD_HEIGHT);
	txtSectors->setId("hdfSectors");

	lblBlocksize = new gcn::Label("Blocksize:");
	lblBlocksize->setAlignment(gcn::Graphics::RIGHT);
	txtBlocksize = new gcn::TextField();
	txtBlocksize->setSize(40, TEXTFIELD_HEIGHT);
	txtBlocksize->setId("hdfBlocksize");

	lblPath = new gcn::Label("Path:");
	lblPath->setAlignment(gcn::Graphics::RIGHT);
	txtPath = new gcn::TextField();
	txtPath->setSize(338, TEXTFIELD_HEIGHT);
	txtPath->setEnabled(false);
	cmdPath = new gcn::Button("...");
	cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdPath->setBaseColor(gui_baseCol);
	cmdPath->setId("hdfPath");
	cmdPath->addActionListener(filesysHardfileActionListener);

	lblController = new gcn::Label("Controller:");
	lblController->setAlignment(gcn::Graphics::RIGHT);
	cboController = new gcn::UaeDropDown(&controllerListModel);
	cboController->setSize(180, DROPDOWN_HEIGHT);
	cboController->setBaseColor(gui_baseCol);
	cboController->setId("hdfController");
	cboController->addActionListener(filesysHardfileActionListener);
	cboUnit = new gcn::UaeDropDown(&unitListModel);
	cboUnit->setSize(60, DROPDOWN_HEIGHT);
	cboUnit->setBaseColor(gui_baseCol);
	cboUnit->setId("hdfUnit");

	int posY = DISTANCE_BORDER;
	int posX = DISTANCE_BORDER;

	wndEditFilesysHardfile->add(lblDevice, DISTANCE_BORDER, posY);
	posX += lblDevice->getWidth() + 8;

	wndEditFilesysHardfile->add(txtDevice, posX, posY);
	posX += txtDevice->getWidth() + DISTANCE_BORDER * 2;

	wndEditFilesysHardfile->add(chkReadWrite, posX, posY + 1);
	posY += txtDevice->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(chkAutoboot, chkReadWrite->getX(), posY + 1);
	posX += chkAutoboot->getWidth() + DISTANCE_BORDER;

	wndEditFilesysHardfile->add(lblBootPri, posX, posY);
	wndEditFilesysHardfile->add(txtBootPri, posX + lblBootPri->getWidth() + 8, posY);
	posY += txtBootPri->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(lblPath, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtPath, DISTANCE_BORDER + lblPath->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(cmdPath, wndEditFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);
	posY += txtPath->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(lblSurfaces, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtSurfaces, DISTANCE_BORDER + lblSurfaces->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(lblReserved, txtSurfaces->getX() + txtSurfaces->getWidth() + DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtReserved, lblReserved->getX() + lblReserved->getWidth() + 8, posY);
	posY += txtSurfaces->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(lblSectors, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtSectors, DISTANCE_BORDER + lblSectors->getWidth() + 8, posY);

	wndEditFilesysHardfile->add(lblBlocksize, txtSectors->getX() + txtSectors->getWidth() + DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtBlocksize, lblBlocksize->getX() + lblBlocksize->getWidth() + 8, posY);
	posY += txtSectors->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(lblController, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(cboController, DISTANCE_BORDER + lblController->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(cboUnit, cboController->getX() + cboController->getWidth() + 8, posY);
	posY += cboController->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(cmdOK);
	wndEditFilesysHardfile->add(cmdCancel);

	gui_top->add(wndEditFilesysHardfile);

	txtDevice->requestFocus();
	wndEditFilesysHardfile->requestModalFocus();
}


static void ExitEditFilesysHardfile()
{
	wndEditFilesysHardfile->releaseModalFocus();
	gui_top->remove(wndEditFilesysHardfile);

	delete lblDevice;
	delete txtDevice;
	delete chkReadWrite;
	delete chkAutoboot;
	delete lblBootPri;
	delete txtBootPri;
	delete lblPath;
	delete txtPath;
	delete cmdPath;
	delete lblSurfaces;
	delete txtSurfaces;
	delete lblReserved;
	delete txtReserved;
	delete lblSectors;
	delete txtSectors;
	delete lblBlocksize;
	delete txtBlocksize;
	delete lblController;
	delete cboController;
	delete cboUnit;

	delete cmdOK;
	delete cmdCancel;
	delete filesysHardfileActionListener;

	delete wndEditFilesysHardfile;
}


static void EditFilesysHardfileLoop()
{
	FocusBugWorkaround(wndEditFilesysHardfile);

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


bool EditFilesysHardfile(const int unit_no)
{
	struct mountedinfo mi;
	struct uaedev_config_data* uci;
	string strdevname, strroot;
	char tmp[32];

	dialogResult = false;
	dialogFinished = false;

	InitEditFilesysHardfile();

	if (unit_no >= 0)
	{
		uci = &changed_prefs.mountconfig[unit_no];
		struct uaedev_config_info* ci = &uci->ci;
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);

		strdevname.assign(ci->devname);
		txtDevice->setText(strdevname);
		strroot.assign(ci->rootdir);
		txtPath->setText(strroot);
		fileSelected = true;

		chkReadWrite->setSelected(!ci->readonly);
		chkAutoboot->setSelected(ci->bootpri != BOOTPRI_NOAUTOBOOT);
		snprintf(tmp, sizeof tmp, "%d", ci->bootpri >= -127 ? ci->bootpri : -127);
		txtBootPri->setText(tmp);
		snprintf(tmp, sizeof tmp, "%d", ci->surfaces);
		txtSurfaces->setText(tmp);
		snprintf(tmp, sizeof tmp, "%d", ci->reserved);
		txtReserved->setText(tmp);
		snprintf(tmp, sizeof tmp, "%d", ci->sectors);
		txtSectors->setText(tmp);
		snprintf(tmp, sizeof tmp, "%d", ci->blocksize);
		txtBlocksize->setText(tmp);
		int selIndex = 0;
		for (int i = 0; i < 2; ++i)
		{
			if (controller[i].type == ci->controller_type)
				selIndex = i;
		}
		cboController->setSelected(selIndex);
		cboUnit->setSelected(ci->controller_unit);

		check_rdb(strroot.c_str());
	}
	else
	{
		CreateDefaultDevicename(tmp);
		txtDevice->setText(tmp);
		strroot.assign(currentDir);
		txtPath->setText(strroot);
		fileSelected = false;

		chkReadWrite->setSelected(true);
		txtBootPri->setText("0");
		txtSurfaces->setText("1");
		txtReserved->setText("2");
		txtSectors->setText("32");
		txtBlocksize->setText("512");
		cboController->setSelected(0);
		cboUnit->setSelected(0);
	}

	EditFilesysHardfileLoop();

	if (dialogResult)
	{
		struct uaedev_config_info ci;
		const int bp = tweakbootpri(atoi(txtBootPri->getText().c_str()), chkAutoboot->isSelected() ? 1 : 0, 0);
		extractPath(const_cast<char *>(txtPath->getText().c_str()), currentDir);

		uci_set_defaults(&ci, false);
		strncpy(ci.devname, const_cast<char *>(txtDevice->getText().c_str()), MAX_DPATH);
		strncpy(ci.rootdir, const_cast<char *>(txtPath->getText().c_str()), MAX_DPATH);
		ci.type = UAEDEV_HDF;
		ci.controller_type = controller[cboController->getSelected()].type;
		ci.controller_type_unit = 0;
		ci.controller_unit = cboUnit->getSelected();
		ci.controller_media_type = 0;
		ci.unit_feature_level = 1;
		ci.unit_special_flags = 0;
		ci.readonly = !chkReadWrite->isSelected();
		ci.sectors = atoi(txtSectors->getText().c_str());
		ci.surfaces = atoi(txtSurfaces->getText().c_str());
		ci.reserved = atoi(txtReserved->getText().c_str());
		ci.blocksize = atoi(txtBlocksize->getText().c_str());
		ci.bootpri = bp;
		ci.physical_geometry = hardfile_testrdb(ci.rootdir);

		uci = add_filesys_config(&changed_prefs, unit_no, &ci);
		if (uci)
		{
			struct hardfiledata* hfd = get_hardfile_data(uci->configoffset);
			if (hfd)
				hardfile_media_change(hfd, &ci, true, false);
			else
				hardfile_added(&ci);
		}
	}

	ExitEditFilesysHardfile();

	return dialogResult;
}
