#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "disk.h"
#include "gui_handling.h"
#include "floppybridge/floppybridge_lib.h"

static gcn::CheckBox* chkDFx[4];
static gcn::DropDown* cboDFxType[4];
static gcn::CheckBox* chkDFxWriteProtect[4];
static gcn::Button* cmdDFxInfo[4];
static gcn::Button* cmdDFxEject[4];
static gcn::Button* cmdDFxSelect[4];
static gcn::DropDown* cboDFxFile[4];
static gcn::Label* lblDriveSpeed;
static gcn::Label* lblDriveSpeedInfo;
static gcn::Slider* sldDriveSpeed;
static gcn::CheckBox* chkLoadConfig;
static gcn::Button* cmdSaveForDisk;
static gcn::Button* cmdCreateDDDisk;
static gcn::Button* cmdCreateHDDisk;
static gcn::Label* lblDBDriver;
static gcn::DropDown* cboDBDriver;
static gcn::CheckBox* chkDBSmartSpeed;
static gcn::CheckBox* chkDBAutoCache;
static gcn::CheckBox* chkDBCableDriveB;

static std::vector<FloppyBridgeAPI::DriverInformation> driver_list{};
static const char* drive_speed_list[] = {"Turbo", "100% (compatible)", "200%", "400%", "800%"};
static const int drive_speed_values[] = {0, 100, 200, 400, 800};

static void AdjustDropDownControls();
static bool bLoadConfigForDisk = false;
static bool bIgnoreListChange = false;

class DriveTypeListModel : public gcn::ListModel
{
	std::vector<std::string> types{};

public:
	DriveTypeListModel()
	{
		types.emplace_back("Disabled");
		types.emplace_back("3.5\" DD");
		types.emplace_back("3.5\" HD");
		types.emplace_back("5.25\" (40)");
		types.emplace_back("5.25\" (80)");
		types.emplace_back("3.5\" ESCOM");
		types.emplace_back("FB: Fast");
		types.emplace_back("FB: Compatible");
		types.emplace_back("FB: Turbo");
		types.emplace_back("FB: Accurate");
	}

	int add_element(const char* elem) override
	{
		types.emplace_back(elem);
		return 0;
	}

	void clear_elements() override
	{
		types.clear();
	}
	
	int getNumberOfElements() override
	{
		return static_cast<int>(types.size());
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(types.size()))
			return "---";
		return types[i];
	}
};

static DriveTypeListModel driveTypeList;

class DriverListModel : public gcn::ListModel
{
	std::vector<std::string> types{};

public:
	DriverListModel()
	{
		FloppyBridgeAPI::getDriverList(driver_list);
		for (auto &i : driver_list)
		{
			types.emplace_back(i.name);
		}
	}

	int add_element(const char* elem) override
	{
		types.emplace_back(elem);
		return 0;
	}

	void clear_elements() override
	{
		types.clear();
	}

	int getNumberOfElements() override
	{
		return static_cast<int>(types.size());
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(types.size()))
			return "---";
		return types[i];
	}
};

static DriverListModel driverNameList;

class DiskfileListModel : public gcn::ListModel
{
public:
	DiskfileListModel()
	= default;

	int getNumberOfElements() override
	{
		return lstMRUDiskList.size();
	}

	int add_element(const char* elem) override
	{
		return 0;
	}

	void clear_elements() override
	{
	}
	
	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(lstMRUDiskList.size()))
			return "---";
		const std::string full_path = lstMRUDiskList[i];
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		return filename + " { " + full_path + " }";
	}
};

static DiskfileListModel diskfileList;

class DriveTypeActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		int sub;
		for (auto i = 0; i < 4; ++i)
		{
			if (actionEvent.getSource() == cboDFxType[i])
			{
				auto selectedType = cboDFxType[i]->getSelected();
				const int dfxtype = todfxtype(i, selectedType - 1, &sub);
				changed_prefs.floppyslots[i].dfxtype = dfxtype;
				changed_prefs.floppyslots[i].dfxsubtype = sub;
				if (dfxtype == DRV_FB)
				{
					TCHAR tmp[32];
					_stprintf(tmp, _T("%d:%s"), selectedType - 5, drivebridgeModes[selectedType - 6].data());
					_tcscpy(changed_prefs.floppyslots[i].dfxsubtypeid, tmp);
				}
				else
				{
					changed_prefs.floppyslots[i].dfxsubtypeid[0] = 0;
				}
			}
		}
		RefreshPanelFloppy();
		RefreshPanelQuickstart();
	}
};

static DriveTypeActionListener* driveTypeActionListener;

class DriverActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cboDBDriver)
		{
			changed_prefs.drawbridge_driver = cboDBDriver->getSelected();
			drawbridge_update_profiles(&changed_prefs);
		}
		RefreshPanelFloppy();
		RefreshPanelQuickstart();
	}
};

static DriverActionListener* driverActionListener;

class DFxCheckActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkLoadConfig)
			bLoadConfigForDisk = chkLoadConfig->isSelected();
		else if (actionEvent.getSource() == chkDBSmartSpeed)
		{
			changed_prefs.drawbridge_smartspeed = chkDBSmartSpeed->isSelected();
		}
		else if (actionEvent.getSource() == chkDBAutoCache)
		{
			changed_prefs.drawbridge_autocache = chkDBAutoCache->isSelected();
		}
		else if (actionEvent.getSource() == chkDBCableDriveB)
		{
			changed_prefs.drawbridge_connected_drive_b = chkDBCableDriveB->isSelected();
		}
		else
		{
			for (auto i = 0; i < 4; ++i)
			{
				if (actionEvent.getSource() == chkDFx[i])
				{
					//---------------------------------------
					// Drive enabled/disabled
					//---------------------------------------
					if (chkDFx[i]->isSelected())
						changed_prefs.floppyslots[i].dfxtype = DRV_35_DD;
					else
						changed_prefs.floppyslots[i].dfxtype = DRV_NONE;
				}
				else if (actionEvent.getSource() == chkDFxWriteProtect[i])
				{
					//---------------------------------------
					// Write-protect changed
					//---------------------------------------
					disk_setwriteprotect(&changed_prefs, i, changed_prefs.floppyslots[i].df,
					                     chkDFxWriteProtect[i]->isSelected());
					if (disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i) != chkDFxWriteProtect[i]->
						isSelected())
					{
						// Failed to change write protection -> maybe filesystem doesn't support this
						chkDFxWriteProtect[i]->setSelected(!chkDFxWriteProtect[i]->isSelected());
						ShowMessage("Set/Clear write protect", "Failed to change write permission.",
						            "Maybe underlying filesystem doesn't support this.", "", "Ok", "");
						chkDFxWriteProtect[i]->requestFocus();
					}
					DISK_reinsert(i);
				}
			}
		}
		RefreshPanelFloppy();
		RefreshPanelQuickstart();
	}
};

static DFxCheckActionListener* dfxCheckActionListener;


class DFxButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < 4; ++i)
		{
			if (actionEvent.getSource() == cmdDFxInfo[i])
			{
				//---------------------------------------
				// Show info about current disk-image
				//---------------------------------------
				if (changed_prefs.floppyslots[i].dfxtype != DRV_NONE && strlen(changed_prefs.floppyslots[i].df) > 0)
					DisplayDiskInfo(i);
			}
			else if (actionEvent.getSource() == cmdDFxEject[i])
			{
				//---------------------------------------
				// Eject disk from drive
				//---------------------------------------
				disk_eject(i);
				strncpy(changed_prefs.floppyslots[i].df, "", MAX_DPATH);
				AdjustDropDownControls();
			}
			else if (actionEvent.getSource() == cmdDFxSelect[i])
			{
				//---------------------------------------
				// Select disk for drive
				//---------------------------------------
				char tmp[MAX_DPATH];

				if (strlen(changed_prefs.floppyslots[i].df) > 0)
					strncpy(tmp, changed_prefs.floppyslots[i].df, MAX_DPATH);
				else
					strncpy(tmp, current_dir, MAX_DPATH);
				if (SelectFile("Select disk image file", tmp, diskfile_filter))
				{
					if (strncmp(changed_prefs.floppyslots[i].df, tmp, MAX_DPATH) != 0)
					{
						strncpy(changed_prefs.floppyslots[i].df, tmp, MAX_DPATH);
						disk_insert(i, tmp);
						AddFileToDiskList(tmp, 1);
						extract_path(tmp, current_dir);

						if (i == 0 && chkLoadConfig->isSelected())
						{
							// Search for config of disk
							extract_filename(changed_prefs.floppyslots[i].df, tmp);
							remove_file_extension(tmp);
							LoadConfigByName(tmp);
						}
						AdjustDropDownControls();
					}
				}
				cmdDFxSelect[i]->requestFocus();
			}
		}
		RefreshPanelFloppy();
		RefreshPanelQuickstart();
	}
};

static DFxButtonActionListener* dfxButtonActionListener;


class DiskFileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < 4; ++i)
		{
			if (actionEvent.getSource() == cboDFxFile[i])
			{
				//---------------------------------------
				// Disk image from list selected
				//---------------------------------------
				if (!bIgnoreListChange)
				{
					const auto idx = cboDFxFile[i]->getSelected();

					if (idx < 0)
					{
						disk_eject(i);
						strncpy(changed_prefs.floppyslots[i].df, "", MAX_DPATH);
						AdjustDropDownControls();
					}
					else
					{
						auto element = get_full_path_from_disk_list(diskfileList.getElementAt(idx));
						if (element != changed_prefs.floppyslots[i].df)
						{
							strncpy(changed_prefs.floppyslots[i].df, element.c_str(), MAX_DPATH);
							disk_insert(i, changed_prefs.floppyslots[i].df);
							lstMRUDiskList.erase(lstMRUDiskList.begin() + idx);
							lstMRUDiskList.insert(lstMRUDiskList.begin(), changed_prefs.floppyslots[i].df);
							bIgnoreListChange = true;
							cboDFxFile[i]->setSelected(0);
							bIgnoreListChange = false;

							if (i == 0 && chkLoadConfig->isSelected())
							{
								// Search for config of disk
								char tmp[MAX_DPATH];

								extract_filename(changed_prefs.floppyslots[i].df, tmp);
								remove_file_extension(tmp);
								LoadConfigByName(tmp);
							}
						}
					}
				}
			}
		}
		RefreshPanelFloppy();
		RefreshPanelQuickstart();
	}
};

static DiskFileActionListener* diskFileActionListener;


class DriveSpeedSliderActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.floppy_speed = drive_speed_values[static_cast<int>(sldDriveSpeed->getValue())];
		RefreshPanelFloppy();
	}
};

static DriveSpeedSliderActionListener* driveSpeedSliderActionListener;


class SaveForDiskActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		//---------------------------------------
		// Save configuration for current disk
		//---------------------------------------
		if (strlen(changed_prefs.floppyslots[0].df) > 0)
		{
			char filename[MAX_DPATH];
			char diskname[MAX_DPATH];

			extract_filename(changed_prefs.floppyslots[0].df, diskname);
			remove_file_extension(diskname);

			get_configuration_path(filename, MAX_DPATH);
			strncat(filename, diskname, MAX_DPATH - 1);
			strncat(filename, ".uae", MAX_DPATH - 1);

			snprintf(changed_prefs.description, 256, "Configuration for disk '%s'", diskname);
			if (cfgfile_save(&changed_prefs, filename, 0))
				RefreshPanelConfig();
		}
	}
};

static SaveForDiskActionListener* saveForDiskActionListener;


class CreateDiskActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdCreateDDDisk)
		{
			// Create 3.5" DD Disk
			char tmp[MAX_DPATH];
			char diskname[MAX_DPATH];
			strncpy(tmp, current_dir, MAX_DPATH);
			if (SelectFile("Create 3.5\" DD disk file", tmp, diskfile_filter, true))
			{
				extract_filename(tmp, diskname);
				remove_file_extension(diskname);
				diskname[31] = '\0';
				disk_creatediskfile(&changed_prefs, tmp, 0, DRV_35_DD, -1, diskname, false, false, nullptr);
				AddFileToDiskList(tmp, 1);
				extract_path(tmp, current_dir);
			}
			cmdCreateDDDisk->requestFocus();
		}
		else if (actionEvent.getSource() == cmdCreateHDDisk)
		{
			// Create 3.5" HD Disk
			char tmp[MAX_DPATH];
			char diskname[MAX_DPATH];
			strcpy(tmp, current_dir);
			if (SelectFile("Create 3.5\" HD disk file", tmp, diskfile_filter, true))
			{
				extract_filename(tmp, diskname);
				remove_file_extension(diskname);
				diskname[31] = '\0';
				disk_creatediskfile(&changed_prefs, tmp, 0, DRV_35_HD, -1, diskname, false, false, nullptr);
				AddFileToDiskList(tmp, 1);
				extract_path(tmp, current_dir);
			}
			cmdCreateHDDisk->requestFocus();
		}
	}
};

static CreateDiskActionListener* createDiskActionListener;


void InitPanelFloppy(const config_category& category)
{
	int posX;
	auto posY = DISTANCE_BORDER;
	int i;
	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER;

	dfxCheckActionListener = new DFxCheckActionListener();
	driveTypeActionListener = new DriveTypeActionListener();
	driverActionListener = new DriverActionListener();
	dfxButtonActionListener = new DFxButtonActionListener();
	diskFileActionListener = new DiskFileActionListener();
	driveSpeedSliderActionListener = new DriveSpeedSliderActionListener();
	saveForDiskActionListener = new SaveForDiskActionListener();
	createDiskActionListener = new CreateDiskActionListener();

	for (i = 0; i < 4; ++i)
	{
		std::string id_string;

		id_string = "DF" + to_string(i) + ":";
		chkDFx[i] = new gcn::CheckBox(id_string);
		chkDFx[i]->setId(id_string);
		chkDFx[i]->addActionListener(dfxCheckActionListener);

		cboDFxType[i] = new gcn::DropDown(&driveTypeList);
		cboDFxType[i]->setBaseColor(gui_baseCol);
		cboDFxType[i]->setBackgroundColor(colTextboxBackground);
		id_string = "cboType" + to_string(i);
		cboDFxType[i]->setId(id_string);
		cboDFxType[i]->addActionListener(driveTypeActionListener);

		chkDFxWriteProtect[i] = new gcn::CheckBox("Write-protected");
		id_string = "chkWP" + to_string(i);
		chkDFxWriteProtect[i]->setId(id_string);
		chkDFxWriteProtect[i]->addActionListener(dfxCheckActionListener);
		

		cmdDFxInfo[i] = new gcn::Button("?");
		id_string = "cmdInfo" + to_string(i);
		cmdDFxInfo[i]->setId(id_string);
		cmdDFxInfo[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdDFxInfo[i]->setBaseColor(gui_baseCol);
		cmdDFxInfo[i]->addActionListener(dfxButtonActionListener);

		cmdDFxEject[i] = new gcn::Button("Eject");
		id_string = "cmdEject" + to_string(i);
		cmdDFxEject[i]->setId(id_string);
		cmdDFxEject[i]->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
		cmdDFxEject[i]->setBaseColor(gui_baseCol);
		cmdDFxEject[i]->addActionListener(dfxButtonActionListener);

		cmdDFxSelect[i] = new gcn::Button("...");
		id_string = "cmdSel" + to_string(i);
		cmdDFxSelect[i]->setId(id_string);
		cmdDFxSelect[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdDFxSelect[i]->setBaseColor(gui_baseCol);
		cmdDFxSelect[i]->addActionListener(dfxButtonActionListener);

		cboDFxFile[i] = new gcn::DropDown(&diskfileList);
		id_string = "cboDisk" + to_string(i);
		cboDFxFile[i]->setId(id_string);
		cboDFxFile[i]->setSize(textFieldWidth, cboDFxFile[i]->getHeight());
		cboDFxFile[i]->setBaseColor(gui_baseCol);
		cboDFxFile[i]->setBackgroundColor(colTextboxBackground);
		cboDFxFile[i]->addActionListener(diskFileActionListener);

		if (i == 0)
		{
			chkLoadConfig = new gcn::CheckBox("Load config with same name as disk");
			chkLoadConfig->setId("chkLoadDiskCfg");
			chkLoadConfig->addActionListener(dfxCheckActionListener);
		}
	}

	lblDriveSpeed = new gcn::Label("Floppy Drive Emulation Speed:");
	sldDriveSpeed = new gcn::Slider(0, 4);
	sldDriveSpeed->setSize(110, SLIDER_HEIGHT);
	sldDriveSpeed->setBaseColor(gui_baseCol);
	sldDriveSpeed->setMarkerLength(20);
	sldDriveSpeed->setStepLength(1);
	sldDriveSpeed->setId("sldDriveSpeed");
	sldDriveSpeed->addActionListener(driveSpeedSliderActionListener);
	lblDriveSpeedInfo = new gcn::Label(drive_speed_list[1]);

	cmdSaveForDisk = new gcn::Button("Save config for disk");
	cmdSaveForDisk->setSize(cmdSaveForDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdSaveForDisk->setBaseColor(gui_baseCol);
	cmdSaveForDisk->setId("cmdSaveForDisk");
	cmdSaveForDisk->addActionListener(saveForDiskActionListener);

	cmdCreateDDDisk = new gcn::Button("Create 3.5\" DD disk");
	cmdCreateDDDisk->setSize(cmdCreateDDDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdCreateDDDisk->setBaseColor(gui_baseCol);
	cmdCreateDDDisk->setId("cmdCreateDDDisk");
	cmdCreateDDDisk->addActionListener(createDiskActionListener);

	cmdCreateHDDisk = new gcn::Button("Create 3.5\" HD disk");
	cmdCreateHDDisk->setSize(cmdCreateHDDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdCreateHDDisk->setBaseColor(gui_baseCol);
	cmdCreateHDDisk->setId("cmdCreateHDDisk");
	cmdCreateHDDisk->addActionListener(createDiskActionListener);

	lblDBDriver = new gcn::Label("DrawBridge driver:");
	cboDBDriver = new gcn::DropDown(&driverNameList);
	cboDBDriver->setId("cboDBDriver");
	cboDBDriver->setSize(350, cboDBDriver->getHeight());
	cboDBDriver->setBaseColor(gui_baseCol);
	cboDBDriver->setBackgroundColor(colTextboxBackground);
	cboDBDriver->addActionListener(driverActionListener);

	chkDBSmartSpeed = new gcn::CheckBox("DrawBridge: Smart Speed (Dynamically switch on Turbo)");
	chkDBSmartSpeed->setId("chkDBSmartSpeed");
	chkDBSmartSpeed->addActionListener(dfxCheckActionListener);

	chkDBAutoCache = new gcn::CheckBox("DrawBridge: Auto-Cache (Cache disk data while drive is idle)");
	chkDBAutoCache->setId("chkDBAutoCache");
	chkDBAutoCache->addActionListener(dfxCheckActionListener);

	chkDBCableDriveB = new gcn::CheckBox("DrawBridge: Connected as Drive B");
	chkDBCableDriveB->setId("chkDBCableDriveB");
	chkDBCableDriveB->addActionListener(dfxCheckActionListener);

	for (i = 0; i < 4; ++i)
	{
		posX = DISTANCE_BORDER;
		category.panel->add(chkDFx[i], posX, posY);
		posX += chkDFx[i]->getWidth() + DISTANCE_NEXT_X * 2;
		category.panel->add(cboDFxType[i], posX, posY);
		posX += cboDFxType[i]->getWidth() + DISTANCE_NEXT_X * 2;
		category.panel->add(chkDFxWriteProtect[i], posX, posY);
		posX += 3 + chkDFxWriteProtect[i]->getWidth() + 4 * DISTANCE_NEXT_X;
		category.panel->add(cmdDFxInfo[i], posX, posY);
		posX += cmdDFxInfo[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cmdDFxEject[i], posX, posY);
		posX += cmdDFxEject[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cmdDFxSelect[i], posX, posY);
		posY += cmdDFxEject[i]->getHeight() + 8;

		category.panel->add(cboDFxFile[i], DISTANCE_BORDER, posY);
		if (i == 0)
		{
			posY += cboDFxFile[i]->getHeight() + 8;
			category.panel->add(chkLoadConfig, DISTANCE_BORDER, posY);
		}
		posY += cboDFxFile[i]->getHeight() + DISTANCE_NEXT_Y + 4;
	}

	posX = DISTANCE_BORDER;
	category.panel->add(lblDriveSpeed, posX, posY);
	posX += lblDriveSpeed->getWidth() + 8;
	category.panel->add(sldDriveSpeed, posX, posY);
	posX += sldDriveSpeed->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(lblDriveSpeedInfo, posX, posY);
	posY += sldDriveSpeed->getHeight() + DISTANCE_NEXT_Y;

	posX = DISTANCE_BORDER;
	category.panel->add(lblDBDriver, posX, posY);
	category.panel->add(cboDBDriver, lblDBDriver->getX() + lblDBDriver->getWidth() + 8, posY);
	posY += cboDBDriver->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkDBSmartSpeed, posX, posY);
	posY += chkDBSmartSpeed->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkDBAutoCache, posX, posY);
	posY += chkDBAutoCache->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkDBCableDriveB, posX, posY);

	posY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
	category.panel->add(cmdSaveForDisk, DISTANCE_BORDER, posY);
	category.panel->add(cmdCreateDDDisk, cmdSaveForDisk->getX() + cmdSaveForDisk->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cmdCreateHDDisk, cmdCreateDDDisk->getX() + cmdCreateDDDisk->getWidth() + DISTANCE_NEXT_X, posY);

	RefreshPanelFloppy();
}


void ExitPanelFloppy()
{
	for (auto i = 0; i < 4; ++i)
	{
		delete chkDFx[i];
		delete cboDFxType[i];
		delete chkDFxWriteProtect[i];
		delete cmdDFxInfo[i];
		delete cmdDFxEject[i];
		delete cmdDFxSelect[i];
		delete cboDFxFile[i];
	}
	delete chkLoadConfig;
	delete lblDriveSpeed;
	delete sldDriveSpeed;
	delete lblDriveSpeedInfo;
	delete cmdSaveForDisk;
	delete cmdCreateDDDisk;
	delete cmdCreateHDDisk;
	delete lblDBDriver;
	delete cboDBDriver;
	delete chkDBSmartSpeed;
	delete chkDBAutoCache;
	delete chkDBCableDriveB;

	delete dfxCheckActionListener;
	delete driveTypeActionListener;
	delete driverActionListener;
	delete dfxButtonActionListener;
	delete diskFileActionListener;
	delete driveSpeedSliderActionListener;
	delete saveForDiskActionListener;
	delete createDiskActionListener;
}


static void AdjustDropDownControls()
{
	bIgnoreListChange = true;

	for (auto i = 0; i < 4; ++i)
	{
		cboDFxFile[i]->clearSelected();

		if (changed_prefs.floppyslots[i].dfxtype != DRV_NONE && strlen(changed_prefs.floppyslots[i].df) > 0)
		{
			for (auto j = 0; j < static_cast<int>(lstMRUDiskList.size()); ++j)
			{
				if (strcmp(lstMRUDiskList[j].c_str(), changed_prefs.floppyslots[i].df) == 0)
				{
					cboDFxFile[i]->setSelected(j);
					break;
				}
			}
		}
	}

	bIgnoreListChange = false;
}


void RefreshPanelFloppy()
{
	int i;
	auto prev_available = true;

	AdjustDropDownControls();

	changed_prefs.nr_floppies = 0;
	for (i = 0; i < 4; ++i)
	{
		const auto driveEnabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
		chkDFx[i]->setSelected(driveEnabled);
		const int nn = fromdfxtype(i, changed_prefs.floppyslots[i].dfxtype, changed_prefs.floppyslots[i].dfxsubtype);
		cboDFxType[i]->setSelected(nn + 1);
		chkDFxWriteProtect[i]->setSelected(disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i));
		chkDFx[i]->setEnabled(prev_available);
		cboDFxType[i]->setEnabled(prev_available);

		chkDFxWriteProtect[i]->setEnabled(driveEnabled && !changed_prefs.floppy_read_only && nn < 5);
		cmdDFxInfo[i]->setEnabled(driveEnabled && nn < 5);
		cmdDFxEject[i]->setEnabled(driveEnabled && nn < 5);
		cmdDFxSelect[i]->setEnabled(driveEnabled && nn < 5);
		cboDFxFile[i]->setEnabled(driveEnabled && nn < 5);

		prev_available = driveEnabled;
		if (driveEnabled)
			changed_prefs.nr_floppies = i + 1;
	}

	chkLoadConfig->setSelected(bLoadConfigForDisk);

	for (i = 0; i <= 4; ++i)
	{
		if (changed_prefs.floppy_speed == drive_speed_values[i])
		{
			sldDriveSpeed->setValue(i);
			lblDriveSpeedInfo->setCaption(drive_speed_list[i]);
			break;
		}
	}

	lblDBDriver->setEnabled(false);
	cboDBDriver->setEnabled(false);
	chkDBAutoCache->setEnabled(false);
	chkDBSmartSpeed->setEnabled(false);
	chkDBCableDriveB->setEnabled(false);
	for (i = 0; i <= 4; ++i)
	{
		// is DrawBridge selected?
		if (changed_prefs.floppyslots[i].dfxtype == DRV_FB)
		{
			lblDBDriver->setEnabled(true);
			cboDBDriver->setEnabled(true);
			cboDBDriver->setSelected(changed_prefs.drawbridge_driver);
			chkDBAutoCache->setSelected(changed_prefs.drawbridge_autocache);
			chkDBSmartSpeed->setSelected(changed_prefs.drawbridge_smartspeed);
			chkDBCableDriveB->setSelected(changed_prefs.drawbridge_connected_drive_b);

			if (!driver_list.empty())
			{
				unsigned int config_options = driver_list[cboDBDriver->getSelected()].configOptions;

				if (config_options & FloppyBridgeAPI::ConfigOption_AutoCache)
					chkDBAutoCache->setEnabled(true);
				if (config_options & FloppyBridgeAPI::ConfigOption_SmartSpeed)
					chkDBSmartSpeed->setEnabled(true);
				if (config_options & FloppyBridgeAPI::ConfigOption_DriveABCable)
					chkDBCableDriveB->setEnabled(true);
			}
			break;
		}
	}
}

bool HelpPanelFloppy(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("You can enable/disable each drive by clicking the checkbox next to DFx or select");
	helptext.emplace_back("the drive type in the dropdown control. \"3.5\" DD\" is the right choice for nearly");
	helptext.emplace_back("all ADF and ADZ files.");
	helptext.emplace_back("The option \"Write-protected\" indicates if the emulator can write to the ADF.");
	helptext.emplace_back("Changing the write protection of the disk file may fail because of missing rights");
	helptext.emplace_back("on the host filesystem.");
	helptext.emplace_back("The button \"...\" opens a dialog to select the required disk file.");
	helptext.emplace_back("With the dropdown control, you can select one of the disks you recently used.");
	helptext.emplace_back("Details of the current floppy can be displayed with \"?\".");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can reduce the loading time for lot of games by increasing the floppy drive");
	helptext.emplace_back("emulation speed. A few games will not load with higher drive speed and you have");
	helptext.emplace_back("to select 100%.");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Save config for disk\" will create a new configuration file with the name of");
	helptext.emplace_back("the disk in DF0. This configuration will be loaded each time you select the disk");
	helptext.emplace_back("and have the option \"Load config with same name as disk\" enabled.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(With the buttons "Create 3.5\" DD disk" and "Create 3.5\" HD disk" you can)");
	helptext.emplace_back("create a new and empty disk.");
	return true;
}
