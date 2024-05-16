#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "disk.h"
#include "gui_handling.h"
#include "floppybridge/floppybridge_lib.h"
#include "parser.h"

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

static gcn::Button* cmdCreateDDDisk;
static gcn::Button* cmdCreateHDDisk;
static gcn::Window* grpDrawBridge;
static gcn::Label* lblDBDriver;
static gcn::DropDown* cboDBDriver;
static gcn::CheckBox* chkDBSerialAuto;
static gcn::DropDown* cboDBSerialPort;
static gcn::CheckBox* chkDBSmartSpeed;
static gcn::CheckBox* chkDBAutoCache;
static gcn::CheckBox* chkDBCableDriveB;

static std::vector<FloppyBridgeAPI::DriverInformation> driver_list{};
static const char* drive_speed_list[] = {"Turbo", "100% (compatible)", "200%", "400%", "800%"};
static const int drive_speed_values[] = {0, 100, 200, 400, 800};

static void AdjustDropDownControls();
static bool bIgnoreListChange = false;

static gcn::StringListModel driveTypeList(floppy_drive_types);
static gcn::StringListModel driverNameList;
static gcn::StringListModel diskfileList;
static gcn::StringListModel serial_ports_list;

static void RefreshDiskListModel()
{
	diskfileList.clear();
	for(const auto & i : lstMRUDiskList)
	{
		const std::string full_path = i;
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		diskfileList.add(std::string(filename).append(" { ").append(full_path).append(" }"));
	}
}

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

class DFxCheckActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cboDBDriver)
		{
			changed_prefs.drawbridge_driver = cboDBDriver->getSelected();
		}
		else if (actionEvent.getSource() == chkDBSerialAuto)
		{
			changed_prefs.drawbridge_serial_auto = chkDBSerialAuto->isSelected();
		}
		else if (actionEvent.getSource() == cboDBSerialPort)
		{
			const auto selected = cboDBSerialPort->getSelected();
			if (selected == 0)
			{
				changed_prefs.drawbridge_serial_port[0] = 0;
			}
			else
			{
				const auto port_name = serial_ports_list.getElementAt(selected);
				_sntprintf(changed_prefs.drawbridge_serial_port, 256, "%s", port_name.c_str());
			}
		}
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
				std::string tmp;

				if (strlen(changed_prefs.floppyslots[i].df) > 0)
					tmp = std::string(changed_prefs.floppyslots[i].df);
				else
					tmp = get_floppy_path();

				tmp = SelectFile("Select disk image file", tmp, diskfile_filter);
				if (!tmp.empty())
				{
					if (strncmp(changed_prefs.floppyslots[i].df, tmp.c_str(), MAX_DPATH) != 0)
					{
						strncpy(changed_prefs.floppyslots[i].df, tmp.c_str(), MAX_DPATH);
						disk_insert(i, tmp.c_str());
						RefreshDiskListModel();

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
							DISK_history_add (changed_prefs.floppyslots[i].df, -1, HISTORY_FLOPPY, 0);
							disk_insert(i, changed_prefs.floppyslots[i].df);
							lstMRUDiskList.erase(lstMRUDiskList.begin() + idx);
							lstMRUDiskList.insert(lstMRUDiskList.begin(), changed_prefs.floppyslots[i].df);
							RefreshDiskListModel();
							bIgnoreListChange = true;
							cboDFxFile[i]->setSelected(0);
							bIgnoreListChange = false;
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
		std::string tmp;
		if (actionEvent.getSource() == cmdCreateDDDisk)
		{
			// Create 3.5" DD Disk
			char diskname[MAX_DPATH];
			tmp = SelectFile("Create 3.5\" DD disk file", current_dir, diskfile_filter, true);
			if (!tmp.empty())
			{
				extract_filename(tmp.c_str(), diskname);
				remove_file_extension(diskname);
				diskname[31] = '\0';
				disk_creatediskfile(&changed_prefs, tmp.c_str(), 0, DRV_35_DD, -1, diskname, false, false, nullptr);
				DISK_history_add (tmp.c_str(), -1, HISTORY_FLOPPY, 0);
				add_file_to_mru_list(lstMRUDiskList, tmp);
				RefreshDiskListModel();
				current_dir = extract_path(tmp);
			}
			cmdCreateDDDisk->requestFocus();
		}
		else if (actionEvent.getSource() == cmdCreateHDDisk)
		{
			// Create 3.5" HD Disk
			char diskname[MAX_DPATH];
			tmp = SelectFile("Create 3.5\" HD disk file", current_dir, diskfile_filter, true);
			if (!tmp.empty())
			{
				extract_filename(tmp.c_str(), diskname);
				remove_file_extension(diskname);
				diskname[31] = '\0';
				disk_creatediskfile(&changed_prefs, tmp.c_str(), 0, DRV_35_HD, -1, diskname, false, false, nullptr);
				DISK_history_add (tmp.c_str(), -1, HISTORY_FLOPPY, 0);
				add_file_to_mru_list(lstMRUDiskList, tmp);
				RefreshDiskListModel();
				current_dir = extract_path(tmp);
			}
			cmdCreateHDDisk->requestFocus();
		}
	}
};

static CreateDiskActionListener* createDiskActionListener;

void InitPanelFloppy(const config_category& category)
{
	int posX;
	int posY = DISTANCE_BORDER;
	int i;
	const int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER;

	FloppyBridgeAPI::getDriverList(driver_list);
	driverNameList.clear();
	for (const auto &item : driver_list)
	{
		driverNameList.add(item.name);
	}

	serial_ports_list.clear();
	serial_ports_list.add("none");
	for(const auto& port : serial_ports) {
		serial_ports_list.add(port);
	}

	dfxCheckActionListener = new DFxCheckActionListener();
	driveTypeActionListener = new DriveTypeActionListener();
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
		chkDFx[i]->setBaseColor(gui_base_color);
		chkDFx[i]->setBackgroundColor(gui_textbox_background_color);
		chkDFx[i]->addActionListener(dfxCheckActionListener);

		cboDFxType[i] = new gcn::DropDown(&driveTypeList);
		cboDFxType[i]->setSize(150, cboDFxType[i]->getHeight());
		cboDFxType[i]->setBaseColor(gui_base_color);
		cboDFxType[i]->setBackgroundColor(gui_textbox_background_color);
		cboDFxType[i]->setSelectionColor(gui_selection_color);
		id_string = "cboType" + to_string(i);
		cboDFxType[i]->setId(id_string);
		cboDFxType[i]->addActionListener(driveTypeActionListener);

		chkDFxWriteProtect[i] = new gcn::CheckBox("Write-protected");
		id_string = "chkWP" + to_string(i);
		chkDFxWriteProtect[i]->setId(id_string);
		chkDFxWriteProtect[i]->setBaseColor(gui_base_color);
		chkDFxWriteProtect[i]->setBackgroundColor(gui_textbox_background_color);
		chkDFxWriteProtect[i]->addActionListener(dfxCheckActionListener);

		cmdDFxInfo[i] = new gcn::Button("?");
		id_string = "cmdInfo" + to_string(i);
		cmdDFxInfo[i]->setId(id_string);
		cmdDFxInfo[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdDFxInfo[i]->setBaseColor(gui_base_color);
		cmdDFxInfo[i]->addActionListener(dfxButtonActionListener);

		cmdDFxEject[i] = new gcn::Button("Eject");
		id_string = "cmdEject" + to_string(i);
		cmdDFxEject[i]->setId(id_string);
		cmdDFxEject[i]->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
		cmdDFxEject[i]->setBaseColor(gui_base_color);
		cmdDFxEject[i]->addActionListener(dfxButtonActionListener);

		cmdDFxSelect[i] = new gcn::Button("...");
		id_string = "cmdSel" + to_string(i);
		cmdDFxSelect[i]->setId(id_string);
		cmdDFxSelect[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdDFxSelect[i]->setBaseColor(gui_base_color);
		cmdDFxSelect[i]->addActionListener(dfxButtonActionListener);

		cboDFxFile[i] = new gcn::DropDown(&diskfileList);
		id_string = "cboDisk" + to_string(i);
		cboDFxFile[i]->setId(id_string);
		cboDFxFile[i]->setSize(textFieldWidth, cboDFxFile[i]->getHeight());
		cboDFxFile[i]->setBaseColor(gui_base_color);
		cboDFxFile[i]->setBackgroundColor(gui_textbox_background_color);
		cboDFxFile[i]->setSelectionColor(gui_selection_color);
		cboDFxFile[i]->addActionListener(diskFileActionListener);
	}

	lblDriveSpeed = new gcn::Label("Floppy Drive Emulation Speed:");
	sldDriveSpeed = new gcn::Slider(0, 4);
	sldDriveSpeed->setSize(110, SLIDER_HEIGHT);
	sldDriveSpeed->setBaseColor(gui_base_color);
	sldDriveSpeed->setMarkerLength(20);
	sldDriveSpeed->setStepLength(1);
	sldDriveSpeed->setId("sldDriveSpeed");
	sldDriveSpeed->addActionListener(driveSpeedSliderActionListener);
	lblDriveSpeedInfo = new gcn::Label(drive_speed_list[1]);

	cmdCreateDDDisk = new gcn::Button("Create 3.5\" DD disk");
	cmdCreateDDDisk->setSize(cmdCreateDDDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdCreateDDDisk->setBaseColor(gui_base_color);
	cmdCreateDDDisk->setId("cmdCreateDDDisk");
	cmdCreateDDDisk->addActionListener(createDiskActionListener);

	cmdCreateHDDisk = new gcn::Button("Create 3.5\" HD disk");
	cmdCreateHDDisk->setSize(cmdCreateHDDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdCreateHDDisk->setBaseColor(gui_base_color);
	cmdCreateHDDisk->setId("cmdCreateHDDisk");
	cmdCreateHDDisk->addActionListener(createDiskActionListener);

	lblDBDriver = new gcn::Label("DrawBridge driver:");
	cboDBDriver = new gcn::DropDown(&driverNameList);
	cboDBDriver->setId("cboDBDriver");
	cboDBDriver->setSize(350, cboDBDriver->getHeight());
	cboDBDriver->setBaseColor(gui_base_color);
	cboDBDriver->setBackgroundColor(gui_textbox_background_color);
	cboDBDriver->setSelectionColor(gui_selection_color);
	cboDBDriver->addActionListener(dfxCheckActionListener);

	chkDBSerialAuto = new gcn::CheckBox("DrawBridge: Auto-Detect serial port");
	chkDBSerialAuto->setId("chkDBSerialAuto");
	chkDBSerialAuto->setBaseColor(gui_base_color);
	chkDBSerialAuto->setBackgroundColor(gui_textbox_background_color);
	chkDBSerialAuto->addActionListener(dfxCheckActionListener);

	cboDBSerialPort = new gcn::DropDown(&serial_ports_list);
	cboDBSerialPort->setSize(200, cboDBSerialPort->getHeight());
	cboDBSerialPort->setBaseColor(gui_base_color);
	cboDBSerialPort->setBackgroundColor(gui_textbox_background_color);
	cboDBSerialPort->setSelectionColor(gui_selection_color);
	cboDBSerialPort->setId("cboDBSerialPort");
	cboDBSerialPort->addActionListener(dfxCheckActionListener);

	chkDBSmartSpeed = new gcn::CheckBox("DrawBridge: Smart Speed (Dynamically switch on Turbo)");
	chkDBSmartSpeed->setId("chkDBSmartSpeed");
	chkDBSmartSpeed->setBaseColor(gui_base_color);
	chkDBSmartSpeed->setBackgroundColor(gui_textbox_background_color);
	chkDBSmartSpeed->addActionListener(dfxCheckActionListener);

	chkDBAutoCache = new gcn::CheckBox("DrawBridge: Auto-Cache (Cache disk data while drive is idle)");
	chkDBAutoCache->setId("chkDBAutoCache");
	chkDBAutoCache->setBaseColor(gui_base_color);
	chkDBAutoCache->setBackgroundColor(gui_textbox_background_color);
	chkDBAutoCache->addActionListener(dfxCheckActionListener);

	chkDBCableDriveB = new gcn::CheckBox("DrawBridge: Connected as Drive B");
	chkDBCableDriveB->setId("chkDBCableDriveB");
	chkDBCableDriveB->setBaseColor(gui_base_color);
	chkDBCableDriveB->setBackgroundColor(gui_textbox_background_color);
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
		posY += cboDFxFile[i]->getHeight() + DISTANCE_NEXT_Y + 4;
	}

	posX = DISTANCE_BORDER;
	category.panel->add(lblDriveSpeed, posX, posY);
	posX += lblDriveSpeed->getWidth() + 8;
	category.panel->add(sldDriveSpeed, posX, posY);
	posX += sldDriveSpeed->getWidth() + DISTANCE_NEXT_X;
	category.panel->add(lblDriveSpeedInfo, posX, posY);
	posY += sldDriveSpeed->getHeight() + DISTANCE_NEXT_Y * 2;

	posX = DISTANCE_BORDER;

	grpDrawBridge = new gcn::Window("DrawBridge");
	grpDrawBridge->setPosition(posX, posY);
	grpDrawBridge->add(lblDBDriver, 10, 10);
	grpDrawBridge->add(cboDBDriver, lblDBDriver->getX() + lblDBDriver->getWidth() + 8, lblDBDriver->getY());
	grpDrawBridge->add(chkDBSerialAuto, lblDBDriver->getX(), lblDBDriver->getY() + lblDBDriver->getHeight() + DISTANCE_NEXT_Y);
	grpDrawBridge->add(cboDBSerialPort, chkDBSerialAuto->getX() + chkDBSerialAuto->getWidth() + DISTANCE_NEXT_X, chkDBSerialAuto->getY());
	grpDrawBridge->add(chkDBSmartSpeed, lblDBDriver->getX(), chkDBSerialAuto->getY() + chkDBSerialAuto->getHeight() + DISTANCE_NEXT_Y);
	grpDrawBridge->add(chkDBAutoCache, lblDBDriver->getX(), chkDBSmartSpeed->getY() + chkDBSmartSpeed->getHeight() + DISTANCE_NEXT_Y);
	grpDrawBridge->add(chkDBCableDriveB, lblDBDriver->getX(), chkDBAutoCache->getY() + chkDBAutoCache->getHeight() + DISTANCE_NEXT_Y);
	grpDrawBridge->setMovable(false);
	grpDrawBridge->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2, TITLEBAR_HEIGHT + chkDBCableDriveB->getY() + chkDBCableDriveB->getHeight() + DISTANCE_NEXT_Y);
	grpDrawBridge->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpDrawBridge->setBaseColor(gui_base_color);

	category.panel->add(grpDrawBridge);

	posY = category.panel->getHeight() - DISTANCE_BORDER - BUTTON_HEIGHT;
	category.panel->add(cmdCreateDDDisk, DISTANCE_BORDER, posY);
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
	delete lblDriveSpeed;
	delete sldDriveSpeed;
	delete lblDriveSpeedInfo;
	delete cmdCreateDDDisk;
	delete cmdCreateHDDisk;
	delete lblDBDriver;
	delete cboDBDriver;
	delete chkDBSerialAuto;
	delete cboDBSerialPort;
	delete chkDBSmartSpeed;
	delete chkDBAutoCache;
	delete chkDBCableDriveB;
	delete grpDrawBridge;

	delete dfxCheckActionListener;
	delete driveTypeActionListener;
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

	RefreshDiskListModel();
	AdjustDropDownControls();

	changed_prefs.nr_floppies = 0;
	for (i = 0; i < 4; ++i)
	{
		const auto driveEnabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
		const auto disk_in_drive = strlen(changed_prefs.floppyslots[i].df) > 0;
		chkDFx[i]->setSelected(driveEnabled);
		const int nn = fromdfxtype(i, changed_prefs.floppyslots[i].dfxtype, changed_prefs.floppyslots[i].dfxsubtype);
		cboDFxType[i]->setSelected(nn + 1);
		chkDFxWriteProtect[i]->setSelected(disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i));
		chkDFx[i]->setEnabled(prev_available);
		cboDFxType[i]->setEnabled(prev_available);

		chkDFxWriteProtect[i]->setEnabled(driveEnabled && !changed_prefs.floppy_read_only && nn < 5);
		cmdDFxInfo[i]->setEnabled(driveEnabled && nn < 5 && disk_in_drive);
		cmdDFxEject[i]->setEnabled(driveEnabled && nn < 5 && disk_in_drive);
		cmdDFxSelect[i]->setEnabled(driveEnabled && nn < 5);
		cboDFxFile[i]->setEnabled(driveEnabled && nn < 5);

		prev_available = driveEnabled;
		if (driveEnabled)
			changed_prefs.nr_floppies = i + 1;
	}

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
	chkDBSerialAuto->setEnabled(false);
	cboDBSerialPort->setEnabled(false);
	chkDBAutoCache->setEnabled(false);
	chkDBSmartSpeed->setEnabled(false);
	chkDBCableDriveB->setEnabled(false);
	for (i = 0; i < 4; ++i)
	{
		// is DrawBridge selected?
		if (changed_prefs.floppyslots[i].dfxtype == DRV_FB)
		{
			lblDBDriver->setEnabled(true);
			cboDBDriver->setEnabled(true);
			cboDBDriver->setSelected(changed_prefs.drawbridge_driver);
			chkDBSerialAuto->setSelected(changed_prefs.drawbridge_serial_auto);
			chkDBAutoCache->setSelected(changed_prefs.drawbridge_autocache);
			chkDBSmartSpeed->setSelected(changed_prefs.drawbridge_smartspeed);
			chkDBCableDriveB->setSelected(changed_prefs.drawbridge_connected_drive_b);

			if (!driver_list.empty())
			{
				const unsigned int config_options = driver_list[cboDBDriver->getSelected()].configOptions;

				if (config_options & FloppyBridgeAPI::ConfigOption_AutoDetectComport)
					chkDBSerialAuto->setEnabled(true);
				if (config_options & FloppyBridgeAPI::ConfigOption_ComPort)
					cboDBSerialPort->setEnabled(true);
				if (config_options & FloppyBridgeAPI::ConfigOption_AutoCache)
					chkDBAutoCache->setEnabled(true);
				if (config_options & FloppyBridgeAPI::ConfigOption_SmartSpeed)
					chkDBSmartSpeed->setEnabled(true);
				if (config_options & FloppyBridgeAPI::ConfigOption_DriveABCable)
					chkDBCableDriveB->setEnabled(true);

				cboDBSerialPort->setSelected(0);
				if (changed_prefs.drawbridge_serial_port[0])
				{
					const auto serial_name = std::string(changed_prefs.drawbridge_serial_port);
					for (int s = 0; s < serial_ports_list.getNumberOfElements(); s++)
					{
						if (serial_ports_list.getElementAt(s) == serial_name)
						{
							cboDBSerialPort->setSelected(s);
							break;
						}
					}
				}
				cboDBSerialPort->setEnabled(!chkDBSerialAuto->isSelected());
			}
			break;
		}
	}
}

bool HelpPanelFloppy(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("You can enable/disable each drive by clicking the checkbox next to DFx or by selecting");
	helptext.emplace_back(R"(the drive type in the dropdown control. The 3.5" DD drive type is the right choice)");
	helptext.emplace_back("for nearly all ADF and ADZ floppy image files.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The option \"Write-protected\" indicates if the emulator can write to the ADF or not.");
	helptext.emplace_back("Changing the write protection of the disk file may fail because of missing rights");
	helptext.emplace_back("on the host filesystem.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"...\" button opens a dialog to select the desired floppy disk file to load.");
	helptext.emplace_back("With the dropdown control, you can select one of the disk files most recently used.");
	helptext.emplace_back("Details of the currently loaded floppy file can be displayed with the \"?\" button.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can reduce the loading time for lot of games by increasing the floppy drive");
	helptext.emplace_back("emulation speed. A few games will not load with higher drive speed and you have");
	helptext.emplace_back("to select 100%.");
	helptext.emplace_back(" ");
	helptext.emplace_back("If you select an \"FB\" floppy type as one of the drives, that will activate the");
	helptext.emplace_back("DrawBridge-related options below. You can use these options to select which DrawBridge");
	helptext.emplace_back("driver to use, as well as optionally enable some of the extra features the driver offers.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(You can also use the "Create 3.5" DD disk" and "Create 3.5" HD disk" buttons, to make)");
	helptext.emplace_back("a new and empty disk image, for use with the emulator.");
	return true;
}
