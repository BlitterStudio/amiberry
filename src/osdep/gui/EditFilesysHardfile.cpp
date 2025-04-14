#include <cstdio>
#include <cstring>

#include <guisan.hpp>

#include <string>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

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

enum
{
	DIALOG_WIDTH = 650,
	DIALOG_HEIGHT = 500
};

static bool dialogResult = false;
static bool dialogFinished = false;
static bool fileSelected = false;

static gcn::Window *wndEditFilesysHardfile;

static gcn::Label* lblHfPath;
static gcn::TextField* txtHfPath;
static gcn::Button* cmdHfPath;

static gcn::Label* lblHfGeometry;
static gcn::TextField* txtHfGeometry;
static gcn::Button* cmdHfGeometry;

static gcn::Label* lblHfFilesys;
static gcn::TextField* txtHfFilesys;
static gcn::Button* cmdHfFilesys;

static gcn::Label* lblDevice;
static gcn::TextField* txtDevice;

static gcn::Label* lblBootPri;
static gcn::TextField* txtBootPri;

static gcn::CheckBox* chkReadWrite;
static gcn::CheckBox* chkVirtBootable;

static gcn::CheckBox* chkDoNotMount;
static gcn::CheckBox* chkRdbMode;

static gcn::Label* lblController;
static gcn::DropDown* cboController;
static gcn::DropDown* cboUnit;
static gcn::DropDown* cboHdfControllerType;
static gcn::DropDown* cboHdfFeatureLevel;

static gcn::TextField* txtHdfInfo;
static gcn::TextField* txtHdfInfo2;

static gcn::Button *cmdOK;
static gcn::Button *cmdCancel;

static gcn::CheckBox* chkManualGeometry;
static gcn::Label *lblSurfaces;
static gcn::TextField *txtSurfaces;
static gcn::Label *lblReserved;
static gcn::TextField *txtReserved;
static gcn::Label *lblSectors;
static gcn::TextField *txtSectors;
static gcn::Label *lblBlocksize;
static gcn::TextField *txtBlocksize;

std::string txt1, txt2;

static void sethardfilegeo()
{
	if (current_hfdlg.ci.geometry[0]) {
		current_hfdlg.ci.physical_geometry = true;
		chkManualGeometry->setSelected(true);
		chkManualGeometry->setEnabled(false);
		get_hd_geometry(&current_hfdlg.ci);
	} else if (current_hfdlg.ci.chs) {
		current_hfdlg.ci.physical_geometry = true;
		chkManualGeometry->setSelected(true);
		chkManualGeometry->setEnabled(false);
		txtSectors->setEnabled(false);
		txtSurfaces->setEnabled(false);
		txtReserved->setEnabled(false);
		txtBlocksize->setEnabled(false);
	} else {
		chkManualGeometry->setEnabled(true);
	}
}

static void sethd()
{
	const bool rdb = is_hdf_rdb();
	const bool physgeo = (rdb && chkManualGeometry->isSelected()) || current_hfdlg.ci.chs;
	const bool enablegeo = (!rdb || (physgeo && current_hfdlg.ci.geometry[0] == 0)) && !current_hfdlg.ci.chs;
	const struct expansionromtype *ert = get_unit_expansion_rom(current_hfdlg.ci.controller_type);
	if (ert && current_hfdlg.ci.controller_unit >= 8) {
		if (!_tcscmp(ert->name, _T("a2091"))) {
			current_hfdlg.ci.unit_feature_level = HD_LEVEL_SASI_CHS;
		} else if (!_tcscmp(ert->name, _T("a2090a"))) {
			current_hfdlg.ci.unit_feature_level = HD_LEVEL_SCSI_1;
		}
	}
	if (!physgeo)
		current_hfdlg.ci.physical_geometry = false;
	txtSectors->setEnabled(enablegeo);
	txtSurfaces->setEnabled(enablegeo);
	txtReserved->setEnabled(enablegeo);
	txtBlocksize->setEnabled(enablegeo);
}

static void sethardfiletypes()
{
	bool ide = current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST;
	bool scsi = current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_SCSI_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_SCSI_LAST;
	cboHdfControllerType->setEnabled(ide);
	cboHdfFeatureLevel->setEnabled(ide || scsi);
	if (!ide) {
		current_hfdlg.ci.controller_media_type = 0;
	}
	if (current_hfdlg.ci.controller_media_type && current_hfdlg.ci.unit_feature_level == 0)
		current_hfdlg.ci.unit_feature_level = 1;
	cboHdfControllerType->setSelected(current_hfdlg.ci.controller_media_type);
	cboHdfFeatureLevel->setSelected(current_hfdlg.ci.unit_feature_level);
}

static void sethardfile()
{
	std::string rootdir, filesys, strdevname;
	char tmp[32];

	sethardfilegeo();

	auto ide = current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_IDE_LAST;
	bool scsi = current_hfdlg.ci.controller_type >= HD_CONTROLLER_TYPE_SCSI_FIRST && current_hfdlg.ci.controller_type <= HD_CONTROLLER_TYPE_SCSI_LAST;
	auto rdb = is_hdf_rdb();
	bool physgeo = rdb && chkManualGeometry->isSelected();
	auto disables = !rdb || current_hfdlg.ci.controller_type == HD_CONTROLLER_TYPE_UAE;
	bool rdsk = current_hfdlg.rdb;

	sethd();
	if (!disables)
		current_hfdlg.ci.bootpri = 0;
	rootdir.assign(current_hfdlg.ci.rootdir);
	txtHfPath->setText(rootdir);
	filesys.assign(current_hfdlg.ci.filesys);
	txtHfFilesys->setText(filesys);
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
	chkVirtBootable->setSelected(ISAUTOBOOT(&current_hfdlg.ci));
	chkDoNotMount->setSelected(!ISAUTOMOUNT(&current_hfdlg.ci));
	chkVirtBootable->setEnabled(disables);
	chkDoNotMount->setEnabled(disables);
	chkVirtBootable->setVisible(disables);
	chkDoNotMount->setVisible(disables);
	txtBootPri->setVisible(disables);
	chkManualGeometry->setVisible(!rdb);
	if (rdb)
	{
		chkRdbMode->setEnabled(!rdsk);
		chkRdbMode->setSelected(true);
	}
	else
	{
		chkRdbMode->setEnabled(true);
		chkRdbMode->setSelected(false);
	}
	if (!rdb) {
		chkManualGeometry->setSelected(false);
	}
	auto selIndex = 0;
	for (auto i = 0; i < controller.size(); ++i) {
		if (controller[i].type == current_hfdlg.ci.controller_type)
			selIndex = i;
	}
	cboController->setSelected(selIndex);
	cboUnit->setSelected(current_hfdlg.ci.controller_unit);
	sethardfiletypes();
}

static void set_phys_cyls()
{
	if (chkManualGeometry->isSelected()) {
		int v = (current_hfdlg.ci.pheads * current_hfdlg.ci.psecs * current_hfdlg.ci.blocksize);
		current_hfdlg.ci.pcyls = (int)(v ? current_hfdlg.size / v : 0);
		current_hfdlg.ci.physical_geometry = true;
		txtReserved->setText(std::to_string(current_hfdlg.ci.pcyls));
	}
}

static gcn::StringListModel controllerListModel;
static gcn::StringListModel unitListModel;
static gcn::StringListModel hdfTypeListModel;
static gcn::StringListModel hdfFeatureLevelListModel;

class FilesysHardfileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent &actionEvent) override
	{
		if (actionEvent.getSource() == cmdHfGeometry)
		{
			wndEditFilesysHardfile->releaseModalFocus();
			std::string path;
			if (txtHfGeometry->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtHfGeometry->getText();
			}
			const std::string tmp = SelectFile("Select hard disk geometry file", path, nullptr);
			if (!tmp.empty())
			{
				txtHfGeometry->setText(tmp);
                strncpy(current_hfdlg.ci.geometry, tmp.c_str(), sizeof(current_hfdlg.ci.geometry) - 1);
                current_hfdlg.ci.geometry[sizeof(current_hfdlg.ci.geometry) - 1] = '\0';
				sethardfile();
				updatehdfinfo(true, false, false, txt1, txt2);
				txtHdfInfo->setText(txt1);
				txtHdfInfo2->setText(txt2);
			}
			wndEditFilesysHardfile->requestModalFocus();
		}
		else if (actionEvent.getSource() == cmdHfFilesys)
		{
			wndEditFilesysHardfile->releaseModalFocus();
			std::string path;
			if (txtHfFilesys->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtHfFilesys->getText();
			}
			const std::string tmp = SelectFile("Select hard disk filesystem file", path, nullptr);
			if (!tmp.empty())
			{
				txtHfFilesys->setText(tmp);
				strncpy(current_hfdlg.ci.filesys, tmp.c_str(), sizeof(current_hfdlg.ci.filesys) - 1);
				current_hfdlg.ci.filesys[sizeof(current_hfdlg.ci.filesys) - 1] = '\0';
				sethardfile();
				updatehdfinfo(true, false, false, txt1, txt2);
				txtHdfInfo->setText(txt1);
				txtHdfInfo2->setText(txt2);
			}
			wndEditFilesysHardfile->requestModalFocus();
		}
		else if (actionEvent.getSource() == cmdHfPath)
		{
			wndEditFilesysHardfile->releaseModalFocus();
			std::string path;
			if (txtHfPath->getText().empty())
			{
				path = get_harddrive_path();
			}
			else
			{
				path = txtHfPath->getText();
			}
			const std::string tmp = SelectFile("Select hard disk file", path, harddisk_filter);
			if (!tmp.empty())
			{
				txtHfPath->setText(tmp);
				default_hfdlg(&current_hfdlg, false);
				strncpy(current_hfdlg.ci.rootdir, tmp.c_str(), sizeof(current_hfdlg.ci.rootdir) - 1);
				fileSelected = true;
				
				if (current_hfdlg.ci.devname[0] == 0)
					CreateDefaultDevicename(current_hfdlg.ci.devname);

				hardfile_testrdb(&current_hfdlg);
				updatehdfinfo (true, true, false, txt1, txt2);
				get_hd_geometry (&current_hfdlg.ci);
				updatehdfinfo (false, false, false, txt1, txt2);
				txtHdfInfo->setText(txt1);
				txtHdfInfo2->setText(txt2);
				sethardfile();
			}
			wndEditFilesysHardfile->requestModalFocus();
			cmdHfPath->requestFocus();
		}
		else if (actionEvent.getSource() == cboController)
		{
			auto posn = controller[cboController->getSelected()].type;
			current_hfdlg.ci.controller_type = posn % HD_CONTROLLER_NEXT_UNIT;
			current_hfdlg.ci.controller_type_unit = posn / HD_CONTROLLER_NEXT_UNIT;
			inithdcontroller(current_hfdlg.ci.controller_type, current_hfdlg.ci.controller_type_unit, UAEDEV_HDF, current_hfdlg.ci.rootdir[0] != 0);
			sethardfile();
		}
		else if (actionEvent.getSource() == cboUnit) {
			current_hfdlg.ci.controller_unit = cboUnit->getSelected();
			sethardfile();
		}
		else if (actionEvent.getSource() == cboHdfControllerType)
		{
			current_hfdlg.ci.controller_media_type = cboHdfControllerType->getSelected();
			sethardfile();
		}
		else if (actionEvent.getSource() == cboHdfFeatureLevel)
		{
			current_hfdlg.ci.unit_feature_level = cboHdfFeatureLevel->getSelected();
			sethardfile();
		}
		else if (actionEvent.getSource() == chkReadWrite) {
			current_hfdlg.ci.readonly = !chkReadWrite->isSelected();
		}
		else if (actionEvent.getSource() == chkVirtBootable) {
			char tmp[32];
			if (chkVirtBootable->isSelected()) {
				current_hfdlg.ci.bootpri = 0;
				chkDoNotMount->setSelected(false);
			}
			else {
				current_hfdlg.ci.bootpri = BOOTPRI_NOAUTOBOOT;
			}
			snprintf(tmp, sizeof(tmp) - 1, "%d", current_hfdlg.ci.bootpri);
			txtBootPri->setText(tmp);
		}
		else if (actionEvent.getSource() == chkDoNotMount) {
			if (chkDoNotMount->isSelected()) {
				current_hfdlg.ci.bootpri = BOOTPRI_NOAUTOMOUNT;
				chkVirtBootable->setSelected(false);
			}
			else
			{
				current_hfdlg.ci.bootpri = BOOTPRI_NOAUTOBOOT;
				chkVirtBootable->setSelected(true);
			}
		}
		else if (actionEvent.getSource() == chkRdbMode) {
			if (chkRdbMode->isSelected())
			{
				txtHfFilesys->setText("");
				txtDevice->setText("");
				current_hfdlg.ci.sectors = current_hfdlg.ci.reserved = current_hfdlg.ci.surfaces = 0;
				current_hfdlg.ci.bootpri = 0;
			}
			else
			{
				TCHAR tmp[MAX_DPATH];
				_tcscpy(tmp, current_hfdlg.ci.rootdir);
				default_hfdlg(&current_hfdlg, false);
				_tcscpy(current_hfdlg.ci.rootdir, tmp);
			}
			sethardfile();
		}
		else if (actionEvent.getSource() == chkManualGeometry)
		{
			current_hfdlg.ci.physical_geometry = chkManualGeometry->isSelected();
			updatehdfinfo(true, false, false, txt1, txt2);
			txtHdfInfo->setText(txt1);
			txtHdfInfo2->setText(txt2);
			sethardfile();
		}
		else
		{
			if (actionEvent.getSource() == cmdOK)
			{
				if (txtDevice->getText().empty())
				{
					wndEditFilesysHardfile->setCaption("Please enter a device name.");
					return;
				}
				if (txtHfPath->getText().empty())
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
	void focusGained(const gcn::Event& event) override
	{
	}

	void focusLost(const gcn::Event& event) override
	{
		int v;
		int* p;
		if (event.getSource() == txtDevice) {
			strncpy(current_hfdlg.ci.devname, txtDevice->getText().c_str(), MAX_DPATH - 1);

		}
		else if (event.getSource() == txtBootPri) {
			current_hfdlg.ci.bootpri = atoi(txtBootPri->getText().c_str());
			if (current_hfdlg.ci.bootpri < -127)
				current_hfdlg.ci.bootpri = -127;
			if (current_hfdlg.ci.bootpri > 127)
				current_hfdlg.ci.bootpri = 127;

		}
		else if (event.getSource() == txtSurfaces) {
			p = chkManualGeometry->isSelected() ? &current_hfdlg.ci.pheads : &current_hfdlg.ci.surfaces;
			v = *p;
			*p = atoi(txtSurfaces->getText().c_str());
			if (v != *p) {
				set_phys_cyls();
				updatehdfinfo(true, false, false, txt1, txt2);
				txtHdfInfo->setText(txt1);
				txtHdfInfo2->setText(txt2);
				chkRdbMode->setSelected(!is_hdf_rdb());
			}

		}
		else if (event.getSource() == txtReserved) {
			p = chkManualGeometry->isSelected() ? &current_hfdlg.ci.pcyls : &current_hfdlg.ci.reserved;
			v = *p;
			*p = atoi(txtReserved->getText().c_str());
			if (v != *p) {
				if (chkManualGeometry->isSelected())
				{
					current_hfdlg.ci.physical_geometry = true;
				}
				updatehdfinfo(true, false, false, txt1, txt2);
				txtHdfInfo->setText(txt1);
				txtHdfInfo2->setText(txt2);
				chkRdbMode->setSelected(!is_hdf_rdb());
			}

		}
		else if (event.getSource() == txtSectors) {
			p = chkManualGeometry->isSelected() ? &current_hfdlg.ci.psecs : &current_hfdlg.ci.sectors;
			v = *p;
			*p = atoi(txtSectors->getText().c_str());
			if (v != *p) {
				set_phys_cyls();
				updatehdfinfo(true, false, false, txt1, txt2);
				txtHdfInfo->setText(txt1);
				txtHdfInfo2->setText(txt2);
				chkRdbMode->setSelected(!is_hdf_rdb());
			}

		}
		else if (event.getSource() == txtBlocksize) {
			v = current_hfdlg.ci.blocksize;
			current_hfdlg.ci.blocksize = atoi(txtBlocksize->getText().c_str());
			if (v != current_hfdlg.ci.blocksize)
			{
				updatehdfinfo(true, false, false, txt1, txt2);
				txtHdfInfo->setText(txt1);
				txtHdfInfo2->setText(txt2);
			}
		}
	}
};
static FilesysHardfileFocusListener* filesysHardfileFocusListener;

static void InitEditFilesysHardfile()
{
	wndEditFilesysHardfile = new gcn::Window("Edit");
	wndEditFilesysHardfile->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndEditFilesysHardfile->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndEditFilesysHardfile->setBaseColor(gui_base_color);
	wndEditFilesysHardfile->setForegroundColor(gui_foreground_color);
	wndEditFilesysHardfile->setCaption("Hardfile settings");
	wndEditFilesysHardfile->setTitleBarHeight(TITLEBAR_HEIGHT);
	wndEditFilesysHardfile->setMovable(false);

	filesysHardfileActionListener = new FilesysHardfileActionListener();
	filesysHardfileFocusListener = new FilesysHardfileFocusListener();

	lblHfPath = new gcn::Label("Path:");
	lblHfPath->setAlignment(gcn::Graphics::Right);
	txtHfPath = new gcn::TextField();
	txtHfPath->setSize(500, TEXTFIELD_HEIGHT);
	txtHfPath->setId("txtHdfPath");
	txtHfPath->setBaseColor(gui_base_color);
	txtHfPath->setBackgroundColor(gui_background_color);
	txtHfPath->setForegroundColor(gui_foreground_color);

	cmdHfPath = new gcn::Button("...");
	cmdHfPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdHfPath->setBaseColor(gui_base_color);
	cmdHfPath->setForegroundColor(gui_foreground_color);
	cmdHfPath->setId("cmdHdfPath");
	cmdHfPath->addActionListener(filesysHardfileActionListener);

	lblHfGeometry = new gcn::Label("Geometry:");
	lblHfGeometry->setAlignment(gcn::Graphics::Right);
	txtHfGeometry = new gcn::TextField();
	txtHfGeometry->setSize(500, TEXTFIELD_HEIGHT);
	txtHfGeometry->setId("txtHdfGeometry");
	txtHfGeometry->setBaseColor(gui_base_color);
	txtHfGeometry->setBackgroundColor(gui_background_color);
	txtHfGeometry->setForegroundColor(gui_foreground_color);

	cmdHfGeometry = new gcn::Button("...");
	cmdHfGeometry->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdHfGeometry->setBaseColor(gui_base_color);
	cmdHfGeometry->setForegroundColor(gui_foreground_color);
	cmdHfGeometry->setId("cmdHdfGeometry");
	cmdHfGeometry->addActionListener(filesysHardfileActionListener);

	lblHfFilesys = new gcn::Label("Filesys:");
	lblHfFilesys->setAlignment(gcn::Graphics::Right);
	txtHfFilesys = new gcn::TextField();
	txtHfFilesys->setSize(500, TEXTFIELD_HEIGHT);
	txtHfFilesys->setId("txtHdfFilesys");
	txtHfFilesys->setBaseColor(gui_base_color);
	txtHfFilesys->setBackgroundColor(gui_background_color);
	txtHfFilesys->setForegroundColor(gui_foreground_color);

	cmdHfFilesys = new gcn::Button("...");
	cmdHfFilesys->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdHfFilesys->setBaseColor(gui_base_color);
	cmdHfFilesys->setForegroundColor(gui_foreground_color);
	cmdHfFilesys->setId("cmdHdfFilesys");
	cmdHfFilesys->addActionListener(filesysHardfileActionListener);

	lblDevice = new gcn::Label("Device:");
	lblDevice->setAlignment(gcn::Graphics::Right);
	txtDevice = new gcn::TextField();
	txtDevice->setId("txtHdfDev");
	txtDevice->setSize(100, TEXTFIELD_HEIGHT);
	txtDevice->setBaseColor(gui_base_color);
	txtDevice->setBackgroundColor(gui_background_color);
	txtDevice->setForegroundColor(gui_foreground_color);
	txtDevice->addFocusListener(filesysHardfileFocusListener);

	lblBootPri = new gcn::Label("Boot priority:");
	lblBootPri->setAlignment(gcn::Graphics::Right);
	txtBootPri = new gcn::TextField();
	txtBootPri->setId("txtHdfBootPri");
	txtBootPri->setSize(40, TEXTFIELD_HEIGHT);
	txtBootPri->setBaseColor(gui_base_color);
	txtBootPri->setBackgroundColor(gui_background_color);
	txtBootPri->setForegroundColor(gui_foreground_color);
	txtBootPri->addFocusListener(filesysHardfileFocusListener);

	chkReadWrite = new gcn::CheckBox("Read/Write", true);
	chkReadWrite->setId("chkHdfRW");
	chkReadWrite->setBaseColor(gui_base_color);
	chkReadWrite->setBackgroundColor(gui_background_color);
	chkReadWrite->setForegroundColor(gui_foreground_color);
	chkReadWrite->addActionListener(filesysHardfileActionListener);

	chkVirtBootable = new gcn::CheckBox("Bootable", true);
	chkVirtBootable->setId("hdfAutoboot");
	chkVirtBootable->setBaseColor(gui_base_color);
	chkVirtBootable->setBackgroundColor(gui_background_color);
	chkVirtBootable->setForegroundColor(gui_foreground_color);
	chkVirtBootable->addActionListener(filesysHardfileActionListener);

	chkManualGeometry = new gcn::CheckBox("Manual geometry", false);
	chkManualGeometry->setId("chkHdfManualGeometry");
	chkManualGeometry->setBaseColor(gui_base_color);
	chkManualGeometry->setBackgroundColor(gui_background_color);
	chkManualGeometry->setForegroundColor(gui_foreground_color);
	chkManualGeometry->addActionListener(filesysHardfileActionListener);

	chkDoNotMount = new gcn::CheckBox("Do not mount");
	chkDoNotMount->setId("chkHdfDoNotMount");
	chkDoNotMount->setBaseColor(gui_base_color);
	chkDoNotMount->setBackgroundColor(gui_background_color);
	chkDoNotMount->setForegroundColor(gui_foreground_color);
	chkDoNotMount->addActionListener(filesysHardfileActionListener);

	chkRdbMode = new gcn::CheckBox("Full drive/RDB mode", false);
	chkRdbMode->setId("chkHdfRDB");
	chkRdbMode->setBaseColor(gui_base_color);
	chkRdbMode->setBackgroundColor(gui_background_color);
	chkRdbMode->setForegroundColor(gui_foreground_color);
	chkRdbMode->addActionListener(filesysHardfileActionListener);

	lblController = new gcn::Label("Controller:");
	lblController->setAlignment(gcn::Graphics::Right);
	cboController = new gcn::DropDown(&controllerListModel);
	cboController->setSize(250, DROPDOWN_HEIGHT);
	cboController->setBaseColor(gui_base_color);
	cboController->setBackgroundColor(gui_background_color);
	cboController->setForegroundColor(gui_foreground_color);
	cboController->setSelectionColor(gui_selection_color);
	cboController->setId("cboHdfController");
	cboController->addActionListener(filesysHardfileActionListener);

	cboUnit = new gcn::DropDown(&unitListModel);
	cboUnit->setSize(80, DROPDOWN_HEIGHT);
	cboUnit->setBaseColor(gui_base_color);
	cboUnit->setBackgroundColor(gui_background_color);
	cboUnit->setForegroundColor(gui_foreground_color);
	cboUnit->setSelectionColor(gui_selection_color);
	cboUnit->setId("cboHdfUnit");
	cboUnit->addActionListener(filesysHardfileActionListener);

	cboHdfControllerType = new gcn::DropDown(&hdfTypeListModel);
	cboHdfControllerType->setSize(80, DROPDOWN_HEIGHT);
	cboHdfControllerType->setBaseColor(gui_base_color);
	cboHdfControllerType->setBackgroundColor(gui_background_color);
	cboHdfControllerType->setForegroundColor(gui_foreground_color);
	cboHdfControllerType->setSelectionColor(gui_selection_color);
	cboHdfControllerType->setId("cboHdfControllerType");
	cboHdfControllerType->addActionListener(filesysHardfileActionListener);

	cboHdfFeatureLevel = new gcn::DropDown(&hdfFeatureLevelListModel);
	cboHdfFeatureLevel->setSize(168, DROPDOWN_HEIGHT);
	cboHdfFeatureLevel->setBaseColor(gui_base_color);
	cboHdfFeatureLevel->setBackgroundColor(gui_background_color);
	cboHdfFeatureLevel->setForegroundColor(gui_foreground_color);
	cboHdfFeatureLevel->setSelectionColor(gui_selection_color);
	cboHdfFeatureLevel->setId("cboHdfFeatureLevel");
	cboHdfFeatureLevel->addActionListener(filesysHardfileActionListener);

	txtHdfInfo = new gcn::TextField();
	txtHdfInfo->setSize(DIALOG_WIDTH - DISTANCE_BORDER * 2, TEXTFIELD_HEIGHT);
	txtHdfInfo->setBaseColor(gui_base_color);
	txtHdfInfo->setBackgroundColor(gui_background_color);
	txtHdfInfo->setForegroundColor(gui_foreground_color);
	txtHdfInfo->setEnabled(false);

	txtHdfInfo2 = new gcn::TextField();
	txtHdfInfo2->setSize(DIALOG_WIDTH - DISTANCE_BORDER * 2, TEXTFIELD_HEIGHT);
	txtHdfInfo2->setBaseColor(gui_base_color);
	txtHdfInfo2->setBackgroundColor(gui_background_color);
	txtHdfInfo2->setForegroundColor(gui_foreground_color);
	txtHdfInfo2->setEnabled(false);

	lblSurfaces = new gcn::Label("Surfaces:");
	lblSurfaces->setAlignment(gcn::Graphics::Right);
	txtSurfaces = new gcn::TextField();
	txtSurfaces->setSize(60, TEXTFIELD_HEIGHT);
	txtSurfaces->setBaseColor(gui_base_color);
	txtSurfaces->setBackgroundColor(gui_background_color);
	txtSurfaces->setForegroundColor(gui_foreground_color);
	txtSurfaces->addFocusListener(filesysHardfileFocusListener);

	lblReserved = new gcn::Label("Reserved:");
	lblReserved->setAlignment(gcn::Graphics::Right);
	txtReserved = new gcn::TextField();
	txtReserved->setSize(60, TEXTFIELD_HEIGHT);
	txtReserved->setBaseColor(gui_base_color);
	txtReserved->setBackgroundColor(gui_background_color);
	txtReserved->setForegroundColor(gui_foreground_color);
	txtReserved->addFocusListener(filesysHardfileFocusListener);

	lblSectors = new gcn::Label("Sectors:");
	lblSectors->setAlignment(gcn::Graphics::Right);
	txtSectors = new gcn::TextField();
	txtSectors->setSize(60, TEXTFIELD_HEIGHT);
	txtSectors->setBaseColor(gui_base_color);
	txtSectors->setBackgroundColor(gui_background_color);
	txtSectors->setForegroundColor(gui_foreground_color);
	txtSectors->addFocusListener(filesysHardfileFocusListener);

	lblBlocksize = new gcn::Label("Blocksize:");
	lblBlocksize->setAlignment(gcn::Graphics::Right);
	txtBlocksize = new gcn::TextField();
	txtBlocksize->setSize(60, TEXTFIELD_HEIGHT);
	txtBlocksize->setBaseColor(gui_base_color);
	txtBlocksize->setBackgroundColor(gui_background_color);
	txtBlocksize->setForegroundColor(gui_foreground_color);
	txtBlocksize->addFocusListener(filesysHardfileFocusListener);

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
					   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_base_color);
	cmdOK->setForegroundColor(gui_foreground_color);
	cmdOK->setId("cmdHdfOK");
	cmdOK->addActionListener(filesysHardfileActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
						   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_base_color);
	cmdCancel->setForegroundColor(gui_foreground_color);
	cmdCancel->setId("cmdHdfCancel");
	cmdCancel->addActionListener(filesysHardfileActionListener);

	int posX = DISTANCE_BORDER + lblHfGeometry->getWidth() + 8;
	int posY = DISTANCE_BORDER;

	wndEditFilesysHardfile->add(lblHfPath, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtHfPath, posX, posY);
	wndEditFilesysHardfile->add(cmdHfPath, wndEditFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);
	posY += txtHfPath->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(lblHfGeometry, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtHfGeometry, DISTANCE_BORDER + lblHfGeometry->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(cmdHfGeometry, wndEditFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);
	posY += txtHfGeometry->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(lblHfFilesys, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtHfFilesys, DISTANCE_BORDER + lblHfGeometry->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(cmdHfFilesys, wndEditFilesysHardfile->getWidth() - DISTANCE_BORDER - SMALL_BUTTON_WIDTH, posY);
	posY += txtHfFilesys->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(lblDevice, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(txtDevice, posX, posY);
	posX = txtDevice->getX() + txtDevice->getWidth() + DISTANCE_NEXT_X * 4;

	wndEditFilesysHardfile->add(lblBootPri, posX, posY);
	wndEditFilesysHardfile->add(txtBootPri, posX + lblBootPri->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(chkManualGeometry, txtBootPri->getX() + txtBootPri->getWidth() + DISTANCE_NEXT_X * 4, posY);
	posY += txtBootPri->getHeight() + DISTANCE_NEXT_Y;

	posX = txtDevice->getX();
	wndEditFilesysHardfile->add(chkReadWrite, posX, posY + 1);
	posX = chkReadWrite->getX() + chkReadWrite->getWidth() + DISTANCE_NEXT_X * 3;
	wndEditFilesysHardfile->add(chkVirtBootable, posX, posY + 1);
	posY = chkReadWrite->getY() + chkReadWrite->getHeight() + DISTANCE_NEXT_Y;
	wndEditFilesysHardfile->add(chkDoNotMount, chkReadWrite->getX(), posY + 1);
	posY = chkDoNotMount->getY() + chkDoNotMount->getHeight() + DISTANCE_NEXT_Y;
	wndEditFilesysHardfile->add(chkRdbMode, chkReadWrite->getX(), posY + 1);

	posY = chkRdbMode->getY() + chkRdbMode->getHeight() + DISTANCE_NEXT_Y;
	wndEditFilesysHardfile->add(lblController, DISTANCE_BORDER, posY);
	posY = lblController->getY() + lblController->getHeight() + 8;
	wndEditFilesysHardfile->add(cboController, DISTANCE_BORDER, posY);
	wndEditFilesysHardfile->add(cboUnit, cboController->getX() + cboController->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(cboHdfControllerType, cboUnit->getX() + cboUnit->getWidth() + 8, posY);
	wndEditFilesysHardfile->add(cboHdfFeatureLevel, cboUnit->getX(), lblController->getY());

	posX = chkVirtBootable->getX() + chkVirtBootable->getWidth() + DISTANCE_NEXT_X * 10;
	posY = chkVirtBootable->getY();
	wndEditFilesysHardfile->add(lblSurfaces, posX, posY);
	wndEditFilesysHardfile->add(txtSurfaces, lblSurfaces->getX() + lblBlocksize->getWidth() + 8, posY);
	posY = txtSurfaces->getY() + txtSurfaces->getHeight() + DISTANCE_NEXT_Y;
	wndEditFilesysHardfile->add(lblSectors, posX, posY);
	wndEditFilesysHardfile->add(txtSectors, txtSurfaces->getX(), posY);
	posY = txtSectors->getY() + txtSectors->getHeight() + DISTANCE_NEXT_Y;
	wndEditFilesysHardfile->add(lblReserved, posX, posY);
	wndEditFilesysHardfile->add(txtReserved, txtSurfaces->getX(), posY);
	posY = txtReserved->getY() + txtReserved->getHeight() + DISTANCE_NEXT_Y;
	wndEditFilesysHardfile->add(lblBlocksize, posX, posY);
	wndEditFilesysHardfile->add(txtBlocksize, txtSurfaces->getX(), posY);

	posY = cboController->getY() + cboController->getHeight() + DISTANCE_NEXT_Y;

	wndEditFilesysHardfile->add(txtHdfInfo, DISTANCE_BORDER, posY);
	posY += txtHdfInfo->getHeight() + DISTANCE_NEXT_Y;
	wndEditFilesysHardfile->add(txtHdfInfo2, DISTANCE_BORDER, posY);

	wndEditFilesysHardfile->add(cmdOK);
	wndEditFilesysHardfile->add(cmdCancel);

	gui_top->add(wndEditFilesysHardfile);

	wndEditFilesysHardfile->requestModalFocus();
	focus_bug_workaround(wndEditFilesysHardfile);
	cmdHfPath->requestFocus();
}

static void ExitEditFilesysHardfile()
{
	wndEditFilesysHardfile->releaseModalFocus();
	gui_top->remove(wndEditFilesysHardfile);

	delete lblHfPath;
	delete txtHfPath;
	delete cmdHfPath;

	delete lblHfGeometry;
	delete txtHfGeometry;
	delete cmdHfGeometry;

	delete lblHfFilesys;
	delete txtHfFilesys;
	delete cmdHfFilesys;

	delete lblDevice;
	delete txtDevice;

	delete lblBootPri;
	delete txtBootPri;

	delete chkReadWrite;
	delete chkVirtBootable;
	delete chkDoNotMount;
	delete chkRdbMode;
	delete chkManualGeometry;

	delete lblController;
	delete cboController;
	delete cboUnit;
	delete cboHdfControllerType;
	delete cboHdfFeatureLevel;
	delete txtHdfInfo;
	delete txtHdfInfo2;

	delete lblSurfaces;
	delete txtSurfaces;
	delete lblReserved;
	delete txtReserved;
	delete lblSectors;
	delete txtSectors;
	delete lblBlocksize;
	delete txtBlocksize;

	delete cmdOK;
	delete cmdCancel;
	delete filesysHardfileActionListener;
	delete filesysHardfileFocusListener;

	delete wndEditFilesysHardfile;
}

static void EditFilesysHardfileLoop()
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
				if (cboController->isDroppedDown() || cboUnit->isDroppedDown())
				{
					cboController->foldUp();
					cboController->releaseModalFocus();
					cboController->releaseModalMouseInputFocus();
					cboUnit->foldUp();
					cboUnit->releaseModalFocus();
					cboUnit->releaseModalMouseInputFocus();
				}
				else
				{
					dialogFinished = true;
				}
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

bool EditFilesysHardfile(const int unit_no)
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
	for (const auto& controller_unit_item : controller_unit)	 {
		unitListModel.add(controller_unit_item);
	}

	hdfTypeListModel.clear();
	for (const auto& hdf_type_item : controller_type) {
		hdfTypeListModel.add(hdf_type_item);
	}

	hdfFeatureLevelListModel.clear();
	for (const auto& hdf_feature_level_item : controller_feature_level) {
		hdfFeatureLevelListModel.add(hdf_feature_level_item);
	}

	InitEditFilesysHardfile();

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
		default_hfdlg(&current_hfdlg, false);
		fileSelected = false;
	}

	chkManualGeometry->setSelected(current_hfdlg.ci.physical_geometry);
	updatehdfinfo(true, false, false, txt1, txt2);
	txtHdfInfo->setText(txt1);
	txtHdfInfo2->setText(txt2);
	sethardfile();

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->gui_renderer);

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
		new_hardfile(unit_no);
	}

	ExitEditFilesysHardfile();

	return dialogResult;
}
