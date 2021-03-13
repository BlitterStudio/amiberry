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

static const char* drive_speed_list[] = {"Turbo", "100% (compatible)", "200%", "400%", "800%"};
static const int drive_speed_values[] = {0, 100, 200, 400, 800};

static void AdjustDropDownControls();
static bool bLoadConfigForDisk = false;
static bool bIgnoreListChange = false;

class DriveTypeListModel : public gcn::ListModel
{
private:
	std::vector<std::string> types{};

public:
	DriveTypeListModel()
	{
		types.emplace_back("Disabled");
		types.emplace_back("3.5'' DD");
		types.emplace_back("3.5'' HD");
		types.emplace_back("5.25'' SD");
		types.emplace_back("3.5'' ESCOM");
	}

	int getNumberOfElements() override
	{
		return types.size();
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(types.size()))
			return "---";
		return types[i];
	}
};

static DriveTypeListModel driveTypeList;

class DiskfileListModel : public gcn::ListModel
{
public:
	DiskfileListModel()
	= default;

	int getNumberOfElements() override
	{
		return lstMRUDiskList.size();
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= static_cast<int>(lstMRUDiskList.size()))
			return "---";
		return lstMRUDiskList[i];
	}
};

static DiskfileListModel diskfileList;

static void DisplayDiskInfo(int num)
{
	struct diskinfo di{};
	char tmp1[MAX_DPATH];
	std::vector<std::string> infotext;
	char title[MAX_DPATH];
	char nameonly[MAX_DPATH];
	char linebuffer[512];

	DISK_examine_image(&changed_prefs, num, &di, true);
	DISK_validate_filename(&changed_prefs, changed_prefs.floppyslots[num].df, tmp1, 0, NULL, NULL, NULL);
	extract_filename(tmp1, nameonly);
	snprintf(title, MAX_DPATH - 1, "Info for %s", nameonly);

	snprintf(linebuffer, sizeof(linebuffer) - 1, "Disk readable: %s", di.unreadable ? _T("No") : _T("Yes"));
	infotext.push_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Disk CRC32: %08X", di.imagecrc32);
	infotext.push_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Boot block CRC32: %08X", di.bootblockcrc32);
	infotext.push_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Boot block checksum valid: %s", di.bb_crc_valid ? _T("Yes") : _T("No"));
	infotext.push_back(linebuffer);
	snprintf(linebuffer, sizeof(linebuffer) - 1, "Boot block type: %s", di.bootblocktype == 0 ? _T("Custom") : (di.bootblocktype == 1 ? _T("Standard 1.x") : _T("Standard 2.x+")));
	infotext.push_back(linebuffer);
	if (di.diskname[0]) {
		snprintf(linebuffer, sizeof(linebuffer) - 1, "Label: '%s'", di.diskname);
		infotext.push_back(linebuffer);
	}
	infotext.push_back("");

	if (di.bootblockinfo[0]) {
		infotext.push_back("Amiga Bootblock Reader database detected:");
		snprintf(linebuffer, sizeof(linebuffer) - 1, "Name: '%s'", di.bootblockinfo);
		infotext.push_back(linebuffer);
		if (di.bootblockclass[0]) {
			snprintf(linebuffer, sizeof(linebuffer) - 1, "Class: '%s'", di.bootblockclass);
			infotext.push_back(linebuffer);
		}
		infotext.push_back("");
	}
	
	int w = 16;
	for (int i = 0; i < 1024; i += w) {
		for (int j = 0; j < w; j++) {
			uae_u8 b = di.bootblock[i + j];
			sprintf(linebuffer + j * 3, _T("%02X "), b);
			if (b >= 32 && b < 127)
				linebuffer[w * 3 + 1 + j] = (char)b;
			else
				linebuffer[w * 3 + 1 + j] = '.';
		}
		linebuffer[w * 3] = ' ';
		linebuffer[w * 3 + 1 + w] = 0;
		infotext.push_back(linebuffer);
	}

	ShowDiskInfo(title, infotext);
}

class DriveTypeActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		//---------------------------------------
		// New drive type selected
		//---------------------------------------
		for (auto i = 0; i < 4; ++i)
		{
			if (actionEvent.getSource() == cboDFxType[i])
				changed_prefs.floppyslots[i].dfxtype = cboDFxType[i]->getSelected() - 1;
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
		if (actionEvent.getSource() == chkLoadConfig)
			bLoadConfigForDisk = chkLoadConfig->isSelected();
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
					if (disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df) != chkDFxWriteProtect[i]->
						isSelected())
					{
						// Failed to change write protection -> maybe filesystem doesn't support this
						chkDFxWriteProtect[i]->setSelected(!chkDFxWriteProtect[i]->isSelected());
						ShowMessage("Set/Clear write protect", "Failed to change write permission.",
						            "Maybe underlying filesystem doesn't support this.", "Ok", "");
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
						if (diskfileList.getElementAt(idx) != changed_prefs.floppyslots[i].df)
						{
							strncpy(changed_prefs.floppyslots[i].df, diskfileList.getElementAt(idx).c_str(), MAX_DPATH);
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
			// Create 3.5'' DD Disk
			char tmp[MAX_DPATH];
			char diskname[MAX_DPATH];
			strncpy(tmp, current_dir, MAX_DPATH);
			if (SelectFile("Create 3.5'' DD disk file", tmp, diskfile_filter, true))
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
			// Create 3.5'' HD Disk
			char tmp[MAX_DPATH];
			char diskname[MAX_DPATH];
			strcpy(tmp, current_dir);
			if (SelectFile("Create 3.5'' HD disk file", tmp, diskfile_filter, true))
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


void InitPanelFloppy(const struct _ConfigCategory& category)
{
	int posX;
	auto posY = DISTANCE_BORDER;
	int i;
	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER;

	dfxCheckActionListener = new DFxCheckActionListener();
	driveTypeActionListener = new DriveTypeActionListener();
	dfxButtonActionListener = new DFxButtonActionListener();
	diskFileActionListener = new DiskFileActionListener();
	driveSpeedSliderActionListener = new DriveSpeedSliderActionListener();
	saveForDiskActionListener = new SaveForDiskActionListener();
	createDiskActionListener = new CreateDiskActionListener();

	for (i = 0; i < 4; ++i)
	{
		char tmp[20];
		snprintf(tmp, 20, "DF%d:", i);
		chkDFx[i] = new gcn::CheckBox(tmp);
		chkDFx[i]->setId(tmp);
		chkDFx[i]->addActionListener(dfxCheckActionListener);

		cboDFxType[i] = new gcn::DropDown(&driveTypeList);
		cboDFxType[i]->setBaseColor(gui_baseCol);
		cboDFxType[i]->setBackgroundColor(colTextboxBackground);
		snprintf(tmp, 20, "cboType%d", i);
		cboDFxType[i]->setId(tmp);
		cboDFxType[i]->addActionListener(driveTypeActionListener);

		chkDFxWriteProtect[i] = new gcn::CheckBox("Write-protected");
		chkDFxWriteProtect[i]->addActionListener(dfxCheckActionListener);
		snprintf(tmp, 20, "chkWP%d", i);
		chkDFxWriteProtect[i]->setId(tmp);

		cmdDFxInfo[i] = new gcn::Button("?");
		cmdDFxInfo[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdDFxInfo[i]->setBaseColor(gui_baseCol);
		snprintf(tmp, 20, "cmdInfo%d", i);
		cmdDFxInfo[i]->setId(tmp);
		cmdDFxInfo[i]->addActionListener(dfxButtonActionListener);

		cmdDFxEject[i] = new gcn::Button("Eject");
		cmdDFxEject[i]->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
		cmdDFxEject[i]->setBaseColor(gui_baseCol);
		snprintf(tmp, 20, "cmdEject%d", i);
		cmdDFxEject[i]->setId(tmp);
		cmdDFxEject[i]->addActionListener(dfxButtonActionListener);

		cmdDFxSelect[i] = new gcn::Button("...");
		cmdDFxSelect[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdDFxSelect[i]->setBaseColor(gui_baseCol);
		snprintf(tmp, 20, "cmdSel%d", i);
		cmdDFxSelect[i]->setId(tmp);
		cmdDFxSelect[i]->addActionListener(dfxButtonActionListener);

		cboDFxFile[i] = new gcn::DropDown(&diskfileList);
		cboDFxFile[i]->setSize(textFieldWidth, cboDFxFile[i]->getHeight());
		cboDFxFile[i]->setBaseColor(gui_baseCol);
		cboDFxFile[i]->setBackgroundColor(colTextboxBackground);
		snprintf(tmp, 20, "cboDisk%d", i);
		cboDFxFile[i]->setId(tmp);
		cboDFxFile[i]->addActionListener(diskFileActionListener);

		if (i == 0)
		{
			chkLoadConfig = new gcn::CheckBox("Load config with same name as disk");
			chkLoadConfig->setId("LoadDiskCfg");
			chkLoadConfig->addActionListener(dfxCheckActionListener);
		}
	}

	lblDriveSpeed = new gcn::Label("Floppy Drive Emulation Speed:");
	sldDriveSpeed = new gcn::Slider(0, 4);
	sldDriveSpeed->setSize(110, SLIDER_HEIGHT);
	sldDriveSpeed->setBaseColor(gui_baseCol);
	sldDriveSpeed->setMarkerLength(20);
	sldDriveSpeed->setStepLength(1);
	sldDriveSpeed->setId("DriveSpeed");
	sldDriveSpeed->addActionListener(driveSpeedSliderActionListener);
	lblDriveSpeedInfo = new gcn::Label(drive_speed_list[1]);

	cmdSaveForDisk = new gcn::Button("Save config for disk");
	cmdSaveForDisk->setSize(cmdSaveForDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdSaveForDisk->setBaseColor(gui_baseCol);
	cmdSaveForDisk->setId("SaveForDisk");
	cmdSaveForDisk->addActionListener(saveForDiskActionListener);

	cmdCreateDDDisk = new gcn::Button("Create 3.5'' DD disk");
	cmdCreateDDDisk->setSize(cmdCreateDDDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdCreateDDDisk->setBaseColor(gui_baseCol);
	cmdCreateDDDisk->setId("CreateDD");
	cmdCreateDDDisk->addActionListener(createDiskActionListener);

	cmdCreateHDDisk = new gcn::Button("Create 3.5'' HD disk");
	cmdCreateHDDisk->setSize(cmdCreateHDDisk->getWidth() + 10, BUTTON_HEIGHT);
	cmdCreateHDDisk->setBaseColor(gui_baseCol);
	cmdCreateHDDisk->setId("CreateHD");
	cmdCreateHDDisk->addActionListener(createDiskActionListener);

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

	AdjustDropDownControls();

	changed_prefs.nr_floppies = 0;
	for (i = 0; i < 4; ++i)
	{
		const auto driveEnabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
		chkDFx[i]->setSelected(driveEnabled);
		cboDFxType[i]->setSelected(changed_prefs.floppyslots[i].dfxtype + 1);
		chkDFxWriteProtect[i]->setSelected(disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df));
		chkDFx[i]->setEnabled(prev_available);
		cboDFxType[i]->setEnabled(prev_available);

		chkDFxWriteProtect[i]->setEnabled(driveEnabled && !changed_prefs.floppy_read_only);
		cmdDFxInfo[i]->setEnabled(driveEnabled);
		cmdDFxEject[i]->setEnabled(driveEnabled);
		cmdDFxSelect[i]->setEnabled(driveEnabled);
		cboDFxFile[i]->setEnabled(driveEnabled);

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
}

bool HelpPanelFloppy(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("You can enable/disable each drive by clicking the checkbox next to DFx or select");
	helptext.emplace_back("the drive type in the dropdown control. \"3.5'' DD\" is the right choice for nearly");
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
	helptext.emplace_back(R"(With the buttons "Create 3.5'' DD disk" and "Create 3.5'' HD disk" you can)");
	helptext.emplace_back("create a new and empty disk.");
	return true;
}
