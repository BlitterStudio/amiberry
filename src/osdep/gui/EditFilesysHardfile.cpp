#include <cstdio>
#include <cstring>

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
#include "filesys.h"
#include "gui_handling.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"

#ifdef ANDROID
#include "androidsdl_event.h"
#endif

#define DIALOG_WIDTH 620
#define DIALOG_HEIGHT 280

static const char *harddisk_filter[] = {".hdf", ".hdz", ".lha", "zip", ".vhd", "\0"};

struct controller_map
{
	int type;
	char display[64];
};

static struct controller_map controller[] = {
	{HD_CONTROLLER_TYPE_UAE, "UAE"},
	{HD_CONTROLLER_TYPE_IDE_FIRST, "A600/A1200/A4000 IDE"},
	{-1}};

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;

static gcn::Window *wndEditFilesysHardfile;
static gcn::Button *cmdOK;
static gcn::Button *cmdCancel;
static gcn::Label *lblDevice;
static gcn::TextField *txtDevice;
static gcn::CheckBox *chkReadWrite;
static gcn::CheckBox *chkAutoboot;
static gcn::Label *lblBootPri;
static gcn::TextField *txtBootPri;
static gcn::Label *lblPath;
static gcn::TextField *txtPath;
static gcn::Button *cmdPath;
static gcn::Label *lblSurfaces;
static gcn::TextField *txtSurfaces;
static gcn::Label *lblReserved;
static gcn::TextField *txtReserved;
static gcn::Label *lblSectors;
static gcn::TextField *txtSectors;
static gcn::Label *lblBlocksize;
static gcn::TextField *txtBlocksize;
static gcn::Label *lblController;
static gcn::DropDown *cboController;
static gcn::DropDown *cboUnit;

static void sethd(void)
{
	bool rdb = is_hdf_rdb();
	bool enablegeo = !rdb;
	txtSectors->setEnabled(enablegeo);
	txtSurfaces->setEnabled(enablegeo);
	txtReserved->setEnabled(enablegeo);
	txtBlocksize->setEnabled(enablegeo);
}

static void sethardfile(void)
{
	std::string strdevname, strroot;
	char tmp[32];
	auto ide = current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST;
	auto rdb = is_hdf_rdb();
	auto disables = !rdb || (rdb && current_hfdlg.ci.controller_type == HD_CONTROLLER_TYPE_UAE);

	sethd();
	if (!disables)
		current_hfdlg.ci.bootpri = 0;
	strroot.assign(current_hfdlg.ci.rootdir);
	txtPath->setText(strroot);
	strdevname.assign(current_hfdlg.ci.devname);
	txtDevice->setText(strdevname);
	snprintf(tmp, sizeof(tmp) - 1, "%d", rdb ? current_hfdlg.ci.psecs : current_hfdlg.ci.sectors);
	txtSectors->setText(tmp);
	snprintf(tmp, sizeof(tmp) - 1, "%d", rdb ? current_hfdlg.ci.pheads : current_hfdlg.ci.surfaces);
	txtSurfaces->setText(tmp);
	snprintf(tmp, sizeof(tmp) - 1, "%d", rdb ? current_hfdlg.ci.pcyls : current_hfdlg.ci.reserved);
	txtReserved->setText(tmp);
	snprintf(tmp, sizeof(tmp) - 1, "%d", current_hfdlg.ci.blocksize);
	txtBlocksize->setText(tmp);
	snprintf(tmp, sizeof(tmp) - 1, "%d", current_hfdlg.ci.bootpri);
	txtBootPri->setText(tmp);
	chkReadWrite->setSelected(!current_hfdlg.ci.readonly);
	chkAutoboot->setSelected(ISAUTOBOOT(&current_hfdlg.ci));
	chkAutoboot->setEnabled(disables);

	auto selIndex = 0;
	for (auto i = 0; i < 2; ++i) {
		if (controller[i].type == current_hfdlg.ci.controller_type)
			selIndex = i;
	}
	cboController->setSelected(selIndex);
	cboUnit->setSelected(current_hfdlg.ci.controller_unit);
}

void updatehdfinfo(bool force, bool defaults)
{
	uae_u8 id[512] = { 0 };
	uae_u32 i;

	uae_u64 bsize = 0;
	if (force) {
		auto gotrdb = false;
		auto blocksize = 512;
		struct hardfiledata hfd{};
		memset(id, 0, sizeof id);
		memset(&hfd, 0, sizeof hfd);
		hfd.ci.readonly = true;
		hfd.ci.blocksize = blocksize;
		current_hfdlg.size = 0;
		current_hfdlg.dostype = 0;
		if (hdf_open(&hfd, current_hfdlg.ci.rootdir) > 0) {
			for (i = 0; i < 16; i++) {
				hdf_read(&hfd, id, i * 512, 512);
				bsize = hfd.virtsize;
				current_hfdlg.size = hfd.virtsize;
				if (!memcmp(id, "RDSK", 4) || !memcmp(id, "CDSK", 4)) {
					blocksize = (id[16] << 24) | (id[17] << 16) | (id[18] << 8) | (id[19] << 0);
					gotrdb = true;
					break;
				}
			}
			if (i == 16) {
				hdf_read(&hfd, id, 0, 512);
				current_hfdlg.dostype = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | (id[3] << 0);
			}
		}
		if (defaults) {
			if (blocksize > 512) {
				hfd.ci.blocksize = blocksize;
			}
		}
		if (current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST) {
			getchspgeometry(bsize, &current_hfdlg.ci.pcyls, &current_hfdlg.ci.pheads, &current_hfdlg.ci.psecs, true);
		}
		else {
			getchspgeometry(bsize, &current_hfdlg.ci.pcyls, &current_hfdlg.ci.pheads, &current_hfdlg.ci.psecs, false);
		}
		if (defaults && !gotrdb) {
			gethdfgeometry(bsize, &current_hfdlg.ci);
		}
		hdf_close(&hfd);
	}

	if (current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST) {
		if (current_hfdlg.ci.unit_feature_level == HD_LEVEL_ATA_1 && bsize >= 4 * (uae_u64)0x40000000)
			current_hfdlg.ci.unit_feature_level = HD_LEVEL_ATA_2;
	}
}

class ControllerListModel : public gcn::ListModel
{
public:
	ControllerListModel() = default;

	int getNumberOfElements() override
	{
		return 2;
	}

	std::string getElementAt(const int i) override
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
	UnitListModel() = default;

	int getNumberOfElements() override
	{
		return 4;
	}

	std::string getElementAt(const int i) override
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
	void action(const gcn::ActionEvent &actionEvent) override
	{
		if (actionEvent.getSource() == cmdPath)
		{
			char tmp[MAX_DPATH];
			strncpy(tmp, txtPath->getText().c_str(), MAX_DPATH);
			wndEditFilesysHardfile->releaseModalFocus();
			if (SelectFile("Select harddisk file", tmp, harddisk_filter))
			{
				txtPath->setText(tmp);
				fileSelected = true;
				default_hfdlg(&current_hfdlg);
				CreateDefaultDevicename(current_hfdlg.ci.devname);
				_tcscpy(current_hfdlg.ci.rootdir, tmp);
				// Set RDB mode if IDE or SCSI
				if (current_hfdlg.ci.controller_type > 0) {
					current_hfdlg.ci.sectors = current_hfdlg.ci.reserved = current_hfdlg.ci.surfaces = 0;
				}
				hardfile_testrdb(&current_hfdlg);
				updatehdfinfo(true, true);
				updatehdfinfo(false, false);
				sethardfile();
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
			current_hfdlg.ci.controller_type = controller[cboController->getSelected()].type;
			sethardfile();

		}
		else if (actionEvent.getSource() == cboUnit) {
			current_hfdlg.ci.controller_unit = cboUnit->getSelected();

		}
		else if (actionEvent.getSource() == chkReadWrite) {
			current_hfdlg.ci.readonly = !chkReadWrite->isSelected();

		}
		else if (actionEvent.getSource() == chkAutoboot) {
			char tmp[32];
			if (chkAutoboot->isSelected()) {
				current_hfdlg.ci.bootpri = 0;
			}
			else {
				current_hfdlg.ci.bootpri = BOOTPRI_NOAUTOBOOT;
			}
			snprintf(tmp, sizeof(tmp) - 1, "%d", current_hfdlg.ci.bootpri);
			txtBootPri->setText(tmp);
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

static FilesysHardfileActionListener *filesysHardfileActionListener;

class FilesysHardfileFocusListener : public gcn::FocusListener
{
public:
	void focusGained(const gcn::Event& event)
	{
	}

	void focusLost(const gcn::Event& event)
	{
		int v;
		int* p;
		if (event.getSource() == txtDevice) {
			strncpy(current_hfdlg.ci.devname, (char*)txtDevice->getText().c_str(), MAX_DPATH - 1);

		}
		else if (event.getSource() == txtBootPri) {
			current_hfdlg.ci.bootpri = atoi(txtBootPri->getText().c_str());
			if (current_hfdlg.ci.bootpri < -127)
				current_hfdlg.ci.bootpri = -127;
			if (current_hfdlg.ci.bootpri > 127)
				current_hfdlg.ci.bootpri = 127;

		}
		else if (event.getSource() == txtSurfaces) {
			p = &current_hfdlg.ci.surfaces;
			v = *p;
			*p = atoi(txtSurfaces->getText().c_str());
			if (v != *p) {
				updatehdfinfo(true, false);
			}

		}
		else if (event.getSource() == txtReserved) {
			p = &current_hfdlg.ci.reserved;
			v = *p;
			*p = atoi(txtReserved->getText().c_str());
			if (v != *p) {
				updatehdfinfo(true, false);
			}

		}
		else if (event.getSource() == txtSectors) {
			p = &current_hfdlg.ci.sectors;
			v = *p;
			*p = atoi(txtSectors->getText().c_str());
			if (v != *p) {
				updatehdfinfo(true, false);
			}

		}
		else if (event.getSource() == txtBlocksize) {
			v = current_hfdlg.ci.blocksize;
			current_hfdlg.ci.blocksize = atoi(txtBlocksize->getText().c_str());
			if (v != current_hfdlg.ci.blocksize)
				updatehdfinfo(true, false);
		}
	}
};
static FilesysHardfileFocusListener* filesysHardfileFocusListener;


static void InitEditFilesysHardfile()
{
	for (auto i = 0; expansionroms[i].name; i++)
	{
		const auto* const erc = &expansionroms[i];
		if (erc->deviceflags & EXPANSIONTYPE_IDE)
		{
			for (auto j = 0; controller[j].type >= 0; ++j)
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
	filesysHardfileFocusListener = new FilesysHardfileFocusListener();

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

	txtDevice->addFocusListener(filesysHardfileFocusListener);

	chkReadWrite = new gcn::CheckBox("Read/Write", true);
	chkReadWrite->setId("hdfRW");

	chkReadWrite->addActionListener(filesysHardfileActionListener);

	chkAutoboot = new gcn::CheckBox("Bootable", true);
	chkAutoboot->setId("hdfAutoboot");

	chkAutoboot->addActionListener(filesysHardfileActionListener);

	lblBootPri = new gcn::Label("Boot priority:");
	lblBootPri->setAlignment(gcn::Graphics::RIGHT);
	txtBootPri = new gcn::TextField();
	txtBootPri->setSize(40, TEXTFIELD_HEIGHT);

	txtBootPri->addFocusListener(filesysHardfileFocusListener);
	
	lblSurfaces = new gcn::Label("Surfaces:");
	lblSurfaces->setAlignment(gcn::Graphics::RIGHT);
	txtSurfaces = new gcn::TextField();
	txtSurfaces->setSize(40, TEXTFIELD_HEIGHT);

	txtSurfaces->addFocusListener(filesysHardfileFocusListener);
	
	lblReserved = new gcn::Label("Reserved:");
	lblReserved->setAlignment(gcn::Graphics::RIGHT);
	txtReserved = new gcn::TextField();
	txtReserved->setSize(40, TEXTFIELD_HEIGHT);

	txtReserved->addFocusListener(filesysHardfileFocusListener);
	
	lblSectors = new gcn::Label("Sectors:");
	lblSectors->setAlignment(gcn::Graphics::RIGHT);
	txtSectors = new gcn::TextField();
	txtSectors->setSize(40, TEXTFIELD_HEIGHT);

	txtSectors->addFocusListener(filesysHardfileFocusListener);
	
	lblBlocksize = new gcn::Label("Blocksize:");
	lblBlocksize->setAlignment(gcn::Graphics::RIGHT);
	txtBlocksize = new gcn::TextField();
	txtBlocksize->setSize(40, TEXTFIELD_HEIGHT);

	txtBlocksize->addFocusListener(filesysHardfileFocusListener);
	
	lblPath = new gcn::Label("Path:");
	lblPath->setAlignment(gcn::Graphics::RIGHT);
	txtPath = new gcn::TextField();
	txtPath->setSize(500, TEXTFIELD_HEIGHT);
	
	cmdPath = new gcn::Button("...");
	cmdPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdPath->setBaseColor(gui_baseCol);
	cmdPath->setId("hdfPath");
	cmdPath->addActionListener(filesysHardfileActionListener);

	lblController = new gcn::Label("Controller:");
	lblController->setAlignment(gcn::Graphics::RIGHT);
	cboController = new gcn::DropDown(&controllerListModel);
	cboController->setSize(180, DROPDOWN_HEIGHT);
	cboController->setBaseColor(gui_baseCol);
	cboController->setId("hdfController");
	cboController->addActionListener(filesysHardfileActionListener);
	
	cboUnit = new gcn::DropDown(&unitListModel);
	cboUnit->setSize(60, DROPDOWN_HEIGHT);
	cboUnit->setBaseColor(gui_baseCol);
	cboUnit->setId("hdfUnit");

	cboUnit->addActionListener(filesysHardfileActionListener);
	
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
	delete filesysHardfileFocusListener;

	delete wndEditFilesysHardfile;
}

static void EditFilesysHardfileLoop()
{
	//FocusBugWorkaround(wndEditFilesysHardfile);

	char lastActiveWidget[128];
	strcpy(lastActiveWidget, "");
	
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
					if (HandleNavigation(DIRECTION_UP))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_UP);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || hat & SDL_HAT_DOWN)
				{
					if (HandleNavigation(DIRECTION_DOWN))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_DOWN);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || hat & SDL_HAT_RIGHT)
				{
					if (HandleNavigation(DIRECTION_RIGHT))
						continue; // Don't change value when enter Slider -> don't send event to control
					PushFakeKey(SDLK_RIGHT);
					break;
				}
				if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || hat & SDL_HAT_LEFT)
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

bool EditFilesysHardfile(const int unit_no)
{
	struct mountedinfo mi
	{
	};
	struct uaedev_config_data *uci;
	std::string strdevname, strroot;
	char tmp[32];

	dialogResult = false;
	dialogFinished = false;

	InitEditFilesysHardfile();

	if (unit_no >= 0)
	{
		uci = &changed_prefs.mountconfig[unit_no];
		get_filesys_unitconfig(&changed_prefs, unit_no, &mi);

		current_hfdlg.forcedcylinders = uci->ci.highcyl;
		memcpy(&current_hfdlg.ci, uci, sizeof(struct uaedev_config_info));
		fileSelected = true;
	}
	else
	{
		default_hfdlg(&current_hfdlg);
		CreateDefaultDevicename(current_hfdlg.ci.devname);
		fileSelected = false;
	}

	updatehdfinfo(true, false);
	sethardfile();
	
	// Prepare the screen once
	uae_gui->logic();
	SDL_RenderClear(sdl_renderer);
	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		EditFilesysHardfileLoop();
		cap_fps(start);
	}

	if (dialogResult)
	{
		extract_path(const_cast<char *>(txtPath->getText().c_str()), current_dir);

		struct uaedev_config_info ci{};
		memcpy(&ci, &current_hfdlg.ci, sizeof(struct uaedev_config_info));
		uci = add_filesys_config(&changed_prefs, unit_no, &ci);
		if (uci) {
			auto* const hfd = get_hardfile_data(uci->configoffset);
			if (hfd)
				hardfile_media_change(hfd, &ci, true, false);
		}
	}

	ExitEditFilesysHardfile();

	return dialogResult;
}
