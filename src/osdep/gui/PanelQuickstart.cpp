#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "disk.h"
#include "blkdev.h"
#include "gui_handling.h"

static gcn::Label* lblModel;
static gcn::DropDown* cboModel;
static gcn::Label* lblConfig;
static gcn::DropDown* cboConfig;
static gcn::CheckBox* chkNTSC;

static gcn::CheckBox* chkqsDFx[2];
static gcn::DropDown* cboqsDFxType[2];
static gcn::CheckBox* chkqsDFxWriteProtect[2];
static gcn::Button* cmdqsDFxInfo[2];
static gcn::Button* cmdqsDFxEject[2];
static gcn::Button* cmdqsDFxSelect[2];
static gcn::DropDown* cboqsDFxFile[2];

static gcn::CheckBox* chkCD;
static gcn::Button* cmdCDEject;
static gcn::Button* cmdCDSelect;
static gcn::DropDown* cboCDFile;

static gcn::CheckBox* chkQuickstartMode;
static gcn::Button* cmdSetConfiguration;

static gcn::Label* lblWhdload;
static gcn::DropDown* cboWhdload;
static gcn::Button* cmdWhdloadEject;
static gcn::Button* cmdWhdloadSelect;

static gcn::StringListModel qsDriveTypeList(floppy_drive_types);

struct amigamodels
{
	int compalevels;
	char name[32];
	char configs[8][128];
};

static amigamodels amodels[] = {
	{
		4, "Amiga 500", {
			"1.3 ROM, OCS, 512 KB Chip + 512 KB Slow RAM (most common)",
			"1.3 ROM, ECS Agnus, 512 KB Chip RAM + 512 KB Slow RAM",
			"1.3 ROM, ECS Agnus, 1 MB Chip RAM",
			"1.3 ROM, OCS Agnus, 512 KB Chip RAM",
			"1.2 ROM, OCS Agnus, 512 KB Chip RAM",
			"1.2 ROM, OCS Agnus, 512 KB Chip RAM + 512 KB Slow RAM",
			"\0"
		}
	},
	{
		4, "Amiga 500+", {
			"Basic non-expanded configuration",
			"2 MB Chip RAM expanded configuration",
			"4 MB Fast RAM expanded configuration",
			"\0"
		}
	},
	{
		4, "Amiga 600", {
			"Basic non-expanded configuration",
			"2 MB Chip RAM expanded configuration",
			"4 MB Fast RAM expanded configuration",
			"8 MB Fast RAM expanded configuration"
			"\0"
		}
	},
	{
		4, "Amiga 1000", {
			"512 KB Chip RAM",
			"ICS Denise without EHB support",
			"256 KB Chip RAM"
			"\0"
		}
	},
	{
		5, "Amiga 1200", {
			"Basic non-expanded configuration",
			"4 MB Fast RAM expanded configuration",
			"8 MB Fast RAM expanded configuration"
			"\0"
		}
	},
	{
		2, "Amiga 3000", {
			"1.4 ROM, 2MB Chip + 8MB Fast",
			"2.04 ROM, 2MB Chip + 8MB Fast",
			"3.1 ROM, 2MB Chip + 8MB Fast",
			"\0"
		}
	},
	{
		1, "Amiga 4000", {
			"68030, 3.1 ROM, 2MB Chip + 8MB Fast",
			"68040, 3.1 ROM, 2MB Chip + 8MB Fast",
			"\0"
		}
	},
	{
		1, "Amiga 4000T", {
			"68030, 3.1 ROM, 2MB Chip + 8MB Fast",
			"68040, 3.1 ROM, 2MB Chip + 8MB Fast",
			"\0"
		}
	},
	{
		4, "CD32", {
			"CD32",
			"CD32 with Full Motion Video cartridge",
			"Cubo CD32",
			"CD32, 8MB Fast"
			"\0"
		}
	},
	{
		4, "CDTV", {
			"CDTV",
			"\0"
		}
	},
	{-1}
};

static const int numModels = 10;
static int numModelConfigs = 0;
static bool bIgnoreListChange = true;

static gcn::StringListModel amigaModelList;
static gcn::StringListModel amigaConfigList;
static gcn::StringListModel diskfileList;
static gcn::StringListModel cdfileList;
static gcn::StringListModel whdloadFileList;

static void AdjustDropDownControls();

static void CountModelConfigs()
{
	amigaConfigList.clear();
	numModelConfigs = 0;
	if (quickstart_model >= 0 && quickstart_model < numModels)
	{
		for (auto& config : amodels[quickstart_model].configs)
		{
			if (config[0] == '\0')
				break;
			amigaConfigList.add(config);
			++numModelConfigs;
		}
	}
}

static void SetControlState(const int model)
{
	auto df1_visible = true;
	auto cd_visible = false;
	auto df0_editable = true;

	switch (model)
	{
	case 0: // A500
	case 1: // A500+
	case 2: // A600
	case 3: // A1000
	case 4: // A1200
	case 5: // A3000
	case 6: // A4000
	case 7: // A4000T
		break;

	case 8: // CD32
	case 9: // CDTV
		// No floppy drive available, CD available
		df0_editable = false;
		df1_visible = false;
		cd_visible = true;
		break;
	default:
		break;
	}

	chkqsDFxWriteProtect[0]->setEnabled(df0_editable && !changed_prefs.floppy_read_only);
	cmdqsDFxInfo[0]->setEnabled(df0_editable);
	cmdqsDFxEject[0]->setEnabled(df0_editable);
	cmdqsDFxSelect[0]->setEnabled(df0_editable);
	cboqsDFxFile[0]->setEnabled(df0_editable);

	chkqsDFx[1]->setVisible(df1_visible);
	cboqsDFxType[1]->setVisible(df1_visible);
	chkqsDFxWriteProtect[1]->setVisible(df1_visible);
	cmdqsDFxInfo[1]->setVisible(df1_visible);
	cmdqsDFxEject[1]->setVisible(df1_visible);
	cmdqsDFxSelect[1]->setVisible(df1_visible);
	cboqsDFxFile[1]->setVisible(df1_visible);

	chkCD->setVisible(cd_visible);
	cmdCDEject->setVisible(cd_visible);
	cmdCDSelect->setVisible(cd_visible);
	cboCDFile->setVisible(cd_visible);
}

static void AdjustPrefs()
{
	built_in_prefs(&changed_prefs, quickstart_model, quickstart_conf, 0, 0);
	switch (quickstart_model)
	{
	case 0: // A500
	case 1: // A500+
	case 2: // A600
	case 3: // A1000
	case 4: // A1200
	case 5: // A3000
		// df0 always active
		changed_prefs.floppyslots[0].dfxtype = DRV_35_DD;
		changed_prefs.floppyslots[1].dfxtype = DRV_NONE;

		// No CD available
		changed_prefs.cdslots[0].inuse = false;
		changed_prefs.cdslots[0].type = SCSI_UNIT_DISABLED;
		break;
	case 6: // A4000
	case 7: // A4000T
		// df0 always active
		changed_prefs.floppyslots[0].dfxtype = DRV_35_HD;
		changed_prefs.floppyslots[1].dfxtype = DRV_NONE;

		// No CD available
		changed_prefs.cdslots[0].inuse = false;
		changed_prefs.cdslots[0].type = SCSI_UNIT_DISABLED;
		break;

	case 8: // CD32
	case 9: // CDTV
		// No floppy drive available, CD available
		changed_prefs.floppyslots[0].dfxtype = DRV_NONE;
		changed_prefs.floppyslots[1].dfxtype = DRV_NONE;
		changed_prefs.cdslots[0].inuse = true;
		changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
		changed_prefs.gfx_monitor[0].gfx_size.width = 720;
		changed_prefs.gfx_monitor[0].gfx_size.height = 568;
		break;
	default:
		break;
	}
}

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

static void RefreshCDListModel()
{
	cdfileList.clear();
	for(const auto & i : lstMRUCDList)
	{
		const std::string full_path = i;
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		cdfileList.add(std::string(filename).append(" { ").append(full_path).append(" }"));
	}
}

static void RefreshWhdListModel()
{
	whdloadFileList.clear();
	for(const auto & i : lstMRUWhdloadList)
	{
		const std::string full_path = i;
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		whdloadFileList.add(std::string(filename).append(" { ").append(full_path).append(" }"));
	}
}

class QSCDButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdCDEject)
		{
			//---------------------------------------
			// Eject CD from drive
			//---------------------------------------
			strncpy(changed_prefs.cdslots[0].name, "", MAX_DPATH);
			AdjustDropDownControls();
		}
		else if (actionEvent.getSource() == cmdCDSelect)
		{
			std::string tmp;
			if (strlen(changed_prefs.cdslots[0].name) > 0)
				tmp = std::string(changed_prefs.cdslots[0].name);
			else
				tmp = current_dir;

			tmp = SelectFile("Select CD image file", tmp, cdfile_filter);
			{
				if (strncmp(changed_prefs.cdslots[0].name, tmp.c_str(), MAX_DPATH) != 0)
				{
					strncpy(changed_prefs.cdslots[0].name, tmp.c_str(), MAX_DPATH);
					changed_prefs.cdslots[0].inuse = true;
					changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
					add_file_to_mru_list(lstMRUCDList, tmp);
					current_dir = extract_path(tmp);

					RefreshCDListModel();
					AdjustDropDownControls();
				}
			}
			cmdCDSelect->requestFocus();
		}

		RefreshPanelHD();
		RefreshPanelQuickstart();
	}
};

static QSCDButtonActionListener* cdButtonActionListener;

class QSCDFileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		//---------------------------------------
		// CD image from list selected
		//---------------------------------------
		if (!bIgnoreListChange)
		{
			if (actionEvent.getSource() == cboCDFile)
			{
				const auto idx = cboCDFile->getSelected();

				if (idx < 0)
				{
					strncpy(changed_prefs.cdslots[0].name, "", MAX_DPATH);
					AdjustDropDownControls();
				}
				else
				{
					const auto element = get_full_path_from_disk_list(cdfileList.getElementAt(idx));
					if (element != changed_prefs.cdslots[0].name)
					{
						strncpy(changed_prefs.cdslots[0].name, element.c_str(), MAX_DPATH);
						DISK_history_add (changed_prefs.cdslots[0].name, -1, HISTORY_CD, 0);
						changed_prefs.cdslots[0].inuse = true;
						changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
						lstMRUCDList.erase(lstMRUCDList.begin() + idx);
						lstMRUCDList.insert(lstMRUCDList.begin(), changed_prefs.cdslots[0].name);
						bIgnoreListChange = true;
						cboCDFile->setSelected(0);
						bIgnoreListChange = false;
					}
				}
				RefreshPanelHD();
				RefreshPanelQuickstart();
			}
		}
	}
};

static QSCDFileActionListener* cdFileActionListener;

class QSWHDLoadActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		//---------------------------------------
		// WHDLoad file from list selected
		//---------------------------------------
		if (!bIgnoreListChange)
		{
			if (actionEvent.getSource() == cboWhdload)
			{
				const auto idx = cboWhdload->getSelected();

				if (idx < 0)
				{
					whdload_prefs.whdload_filename = "";
				}
				else
				{
					const auto element = get_full_path_from_disk_list(whdloadFileList.getElementAt(idx));
					if (element != whdload_prefs.whdload_filename)
					{
						whdload_prefs.whdload_filename.assign(element);
						lstMRUWhdloadList.erase(lstMRUWhdloadList.begin() + idx);
						lstMRUWhdloadList.insert(lstMRUWhdloadList.begin(), whdload_prefs.whdload_filename);
						bIgnoreListChange = true;
						cboWhdload->setSelected(0);
						bIgnoreListChange = false;
					}
					whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
				}
				refresh_all_panels();
			}
		}
	}
};

static QSWHDLoadActionListener* whdloadActionListener;

class QSWHDLoadButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdWhdloadEject)
		{
			//---------------------------------------
			// Eject WHDLoad file
			//---------------------------------------
			whdload_prefs.whdload_filename = "";
		}
		else if (actionEvent.getSource() == cmdWhdloadSelect)
		{
			std::string tmp;
			if (!whdload_prefs.whdload_filename.empty())
				tmp = whdload_prefs.whdload_filename;
			else
				tmp = get_whdload_arch_path();

			tmp = SelectFile("Select WHDLoad LHA file", tmp, whdload_filter);
			{
				whdload_prefs.whdload_filename = tmp;
				add_file_to_mru_list(lstMRUWhdloadList, whdload_prefs.whdload_filename);
				RefreshWhdListModel();
				whdload_auto_prefs(&changed_prefs, whdload_prefs.whdload_filename.c_str());
				
				AdjustDropDownControls();
				SetLastActiveConfig(whdload_prefs.whdload_filename.c_str());
			}
			cmdWhdloadSelect->requestFocus();
		}
		refresh_all_panels();
	}
};

static QSWHDLoadButtonActionListener* whdloadButtonActionListener;

class AmigaModelActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (!bIgnoreListChange)
		{
			if (actionEvent.getSource() == cboModel)
			{
				//---------------------------------------
				// Amiga model selected
				//---------------------------------------
				if (quickstart_model != cboModel->getSelected())
				{
					quickstart_model = cboModel->getSelected();
					CountModelConfigs();
					cboConfig->setSelected(0);
					SetControlState(quickstart_model);
					AdjustPrefs();
					disable_resume();
				}
			}
			else if (actionEvent.getSource() == cboConfig)
			{
				//---------------------------------------
				// Model configuration selected
				//---------------------------------------
				if (quickstart_conf != cboConfig->getSelected())
				{
					quickstart_conf = cboConfig->getSelected();
					AdjustPrefs();
				}
			}
			refresh_all_panels();
		}

		if (actionEvent.getSource() == cmdSetConfiguration)
		{
			AdjustPrefs();
			refresh_all_panels();
		}
	}
};

static AmigaModelActionListener* amigaModelActionListener;

class QSNTSCButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (chkNTSC->isSelected())
		{
			changed_prefs.ntscmode = true;
			changed_prefs.chipset_refreshrate = 60;
		}
		else
		{
			changed_prefs.ntscmode = false;
			changed_prefs.chipset_refreshrate = 50;
		}
		RefreshPanelChipset();
	}
};

static QSNTSCButtonActionListener* ntscButtonActionListener;

class QSDFxCheckActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < 2; ++i)
		{
			if (actionEvent.getSource() == chkqsDFx[i])
			{
				//---------------------------------------
				// Drive enabled/disabled
				//---------------------------------------
				if (chkqsDFx[i]->isSelected())
					changed_prefs.floppyslots[i].dfxtype = DRV_35_DD;
				else
					changed_prefs.floppyslots[i].dfxtype = DRV_NONE;
			}
			else if (actionEvent.getSource() == chkqsDFxWriteProtect[i])
			{
				//---------------------------------------
				// Write-protect changed
				//---------------------------------------
				if (strlen(changed_prefs.floppyslots[i].df) <= 0)
					return;
				disk_setwriteprotect(&changed_prefs, i, changed_prefs.floppyslots[i].df,
									 chkqsDFxWriteProtect[i]->isSelected());
				if (disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i) != chkqsDFxWriteProtect[i]->
					isSelected())
				{
					// Failed to change write protection -> maybe filesystem doesn't support this
					ShowMessage("Set/Clear write protect", "Failed to change write permission.",
								"Maybe underlying filesystem doesn't support this.", "", "Ok", "");
					chkqsDFxWriteProtect[i]->requestFocus();
				}
				DISK_reinsert(i);
			}
		}

		RefreshPanelFloppy();
		RefreshPanelQuickstart();
	}
};

static QSDFxCheckActionListener* qsdfxCheckActionListener;

class QSDriveTypeActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		int sub;
		for (auto i = 0; i < 2; ++i)
		{
			if (actionEvent.getSource() == cboqsDFxType[i])
			{
				const auto selectedType = cboqsDFxType[i]->getSelected();
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

static QSDriveTypeActionListener* qsDriveTypeActionListener;

class QSDFxButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < 2; ++i)
		{
			if (actionEvent.getSource() == cmdqsDFxInfo[i])
			{
				//---------------------------------------
				// Show info about current disk-image
				//---------------------------------------
				if (changed_prefs.floppyslots[i].dfxtype != DRV_NONE && strlen(changed_prefs.floppyslots[i].df) > 0)
					DisplayDiskInfo(i);
			}
			else if (actionEvent.getSource() == cmdqsDFxEject[i])
			{
				//---------------------------------------
				// Eject disk from drive
				//---------------------------------------
				disk_eject(i);
				strncpy(changed_prefs.floppyslots[i].df, "", MAX_DPATH);
				AdjustDropDownControls();
			}
			else if (actionEvent.getSource() == cmdqsDFxSelect[i])
			{
				//---------------------------------------
				// Select disk for drive
				//---------------------------------------
				std::string tmp;

				if (strlen(changed_prefs.floppyslots[i].df) > 0)
					tmp = std::string(changed_prefs.floppyslots[i].df);
				else
					tmp = current_dir;
				tmp = SelectFile("Select disk image file", tmp, diskfile_filter);
				{
					if (strncmp(changed_prefs.floppyslots[i].df, tmp.c_str(), MAX_DPATH) != 0)
					{
						strncpy(changed_prefs.floppyslots[i].df, tmp.c_str(), MAX_DPATH);
						disk_insert(i, tmp.c_str());
						RefreshDiskListModel();
						current_dir = extract_path(tmp);

						AdjustDropDownControls();
						SetLastActiveConfig(tmp.c_str());
					}
				}
				cmdqsDFxSelect[i]->requestFocus();
			}
		}

		refresh_all_panels();
	}
};

static QSDFxButtonActionListener* qsdfxButtonActionListener;

class QSDiskFileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < 2; ++i)
		{
			if (actionEvent.getSource() == cboqsDFxFile[i])
			{
				//---------------------------------------
				// Disk image from list selected
				//---------------------------------------
				if (!bIgnoreListChange)
				{
					const auto idx = cboqsDFxFile[i]->getSelected();

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
							bIgnoreListChange = true;
							cboqsDFxFile[i]->setSelected(0);
							bIgnoreListChange = false;
						}
					}
					RefreshPanelFloppy();
					RefreshPanelQuickstart();
				}
			}
		}
	}
};

static QSDiskFileActionListener* diskFileActionListener;

class QuickstartModeActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		amiberry_options.quickstart_start = chkQuickstartMode->isSelected();
	}
};

static QuickstartModeActionListener* quickstartModeActionListener;

void InitPanelQuickstart(const config_category& category)
{
	int posX;
	int posY = DISTANCE_BORDER;

	amigaModelList.clear();
	for (int i = 0; i < numModels; ++i)
		amigaModelList.add(amodels[i].name);

	amigaConfigList.clear();
	diskfileList.clear();
	for(const auto & i : lstMRUDiskList)
	{
		const std::string full_path = i;
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		diskfileList.add(std::string(filename).append(" { ").append(full_path).append(" }"));
	}

	cdfileList.clear();
	for(const auto & i : lstMRUCDList)
	{
		const std::string full_path = i;
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		cdfileList.add(std::string(filename).append(" { ").append(full_path).append(" }"));
	}

	amigaModelActionListener = new AmigaModelActionListener();
	ntscButtonActionListener = new QSNTSCButtonActionListener();
	qsdfxCheckActionListener = new QSDFxCheckActionListener();
	qsDriveTypeActionListener = new QSDriveTypeActionListener();
	qsdfxButtonActionListener = new QSDFxButtonActionListener();
	diskFileActionListener = new QSDiskFileActionListener();
	cdButtonActionListener = new QSCDButtonActionListener();
	cdFileActionListener = new QSCDFileActionListener();
	quickstartModeActionListener = new QuickstartModeActionListener();
	whdloadActionListener = new QSWHDLoadActionListener();
	whdloadButtonActionListener = new QSWHDLoadButtonActionListener();

	lblModel = new gcn::Label("Amiga model:");
	lblModel->setAlignment(gcn::Graphics::RIGHT);
	cboModel = new gcn::DropDown(&amigaModelList);
	cboModel->setSize(160, cboModel->getHeight());
	cboModel->setBaseColor(gui_baseCol);
	cboModel->setBackgroundColor(colTextboxBackground);
	cboModel->setSelectionColor(gui_selection_color);
	cboModel->setId("cboAModel");
	cboModel->addActionListener(amigaModelActionListener);

	lblConfig = new gcn::Label("Config:");
	lblConfig->setAlignment(gcn::Graphics::RIGHT);
	cboConfig = new gcn::DropDown(&amigaConfigList);
	cboConfig->setSize(category.panel->getWidth() - lblConfig->getWidth() - 8 - 2 * DISTANCE_BORDER,
					   cboConfig->getHeight());
	cboConfig->setBaseColor(gui_baseCol);
	cboConfig->setBackgroundColor(colTextboxBackground);
	cboConfig->setSelectionColor(gui_selection_color);
	cboConfig->setId("cboAConfig");
	cboConfig->addActionListener(amigaModelActionListener);

	chkNTSC = new gcn::CheckBox("NTSC");
	chkNTSC->setId("qsNTSC");
	chkNTSC->addActionListener(ntscButtonActionListener);

	for (auto i = 0; i < 2; ++i)
	{
		char tmp[20];
		snprintf(tmp, 20, "DF%d:", i);
		chkqsDFx[i] = new gcn::CheckBox(tmp);
		snprintf(tmp, 20, "qsDF%d", i);
		chkqsDFx[i]->setId(tmp);
		chkqsDFx[i]->addActionListener(qsdfxCheckActionListener);

		cboqsDFxType[i] = new gcn::DropDown(&qsDriveTypeList);
		cboqsDFxType[i]->setBaseColor(gui_baseCol);
		cboqsDFxType[i]->setBackgroundColor(colTextboxBackground);
		cboqsDFxType[i]->setSelectionColor(gui_selection_color);
		snprintf(tmp, 20, "cboqsType%d", i);
		cboqsDFxType[i]->setId(tmp);
		cboqsDFxType[i]->addActionListener(qsDriveTypeActionListener);

		chkqsDFxWriteProtect[i] = new gcn::CheckBox("Write-protected");
		snprintf(tmp, 20, "qsWP%d", i);
		chkqsDFxWriteProtect[i]->setId(tmp);
		chkqsDFxWriteProtect[i]->addActionListener(qsdfxCheckActionListener);


		cmdqsDFxInfo[i] = new gcn::Button("?");
		snprintf(tmp, 20, "qsInfo%d", i);
		cmdqsDFxInfo[i]->setId(tmp);
		cmdqsDFxInfo[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdqsDFxInfo[i]->setBaseColor(gui_baseCol);
		cmdqsDFxInfo[i]->addActionListener(qsdfxButtonActionListener);

		cmdqsDFxEject[i] = new gcn::Button("Eject");
		snprintf(tmp, 20, "qscmdEject%d", i);
		cmdqsDFxEject[i]->setId(tmp);
		cmdqsDFxEject[i]->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
		cmdqsDFxEject[i]->setBaseColor(gui_baseCol);
		cmdqsDFxEject[i]->addActionListener(qsdfxButtonActionListener);

		cmdqsDFxSelect[i] = new gcn::Button("Select file");
		snprintf(tmp, 20, "qscmdSel%d", i);
		cmdqsDFxSelect[i]->setId(tmp);
		cmdqsDFxSelect[i]->setSize(BUTTON_WIDTH + 10, SMALL_BUTTON_HEIGHT);
		cmdqsDFxSelect[i]->setBaseColor(gui_baseCol);
		cmdqsDFxSelect[i]->addActionListener(qsdfxButtonActionListener);

		cboqsDFxFile[i] = new gcn::DropDown(&diskfileList);
		snprintf(tmp, 20, "cboqsDisk%d", i);
		cboqsDFxFile[i]->setId(tmp);
		cboqsDFxFile[i]->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboqsDFxFile[i]->getHeight());
		cboqsDFxFile[i]->setBaseColor(gui_baseCol);
		cboqsDFxFile[i]->setBackgroundColor(colTextboxBackground);
		cboqsDFxFile[i]->setSelectionColor(gui_selection_color);
		cboqsDFxFile[i]->addActionListener(diskFileActionListener);
	}

	chkCD = new gcn::CheckBox("CD drive");
	chkCD->setId("qsCD drive");
	chkCD->setEnabled(false);

	cmdCDEject = new gcn::Button("Eject");
	cmdCDEject->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
	cmdCDEject->setBaseColor(gui_baseCol);
	cmdCDEject->setId("qscdEject");
	cmdCDEject->addActionListener(cdButtonActionListener);

	cmdCDSelect = new gcn::Button("Select image");
	cmdCDSelect->setSize(BUTTON_WIDTH + 10, SMALL_BUTTON_HEIGHT);
	cmdCDSelect->setBaseColor(gui_baseCol);
	cmdCDSelect->setId("qsCDSelect");
	cmdCDSelect->addActionListener(cdButtonActionListener);

	cboCDFile = new gcn::DropDown(&cdfileList);
	cboCDFile->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboCDFile->getHeight());
	cboCDFile->setBaseColor(gui_baseCol);
	cboCDFile->setBackgroundColor(colTextboxBackground);
	cboCDFile->setSelectionColor(gui_selection_color);
	cboCDFile->setId("cboCD");
	cboCDFile->addActionListener(cdFileActionListener);

	chkQuickstartMode = new gcn::CheckBox("Start in Quickstart mode");
	chkQuickstartMode->setId("qsMode");
	chkQuickstartMode->addActionListener(quickstartModeActionListener);

	cmdSetConfiguration = new gcn::Button("Set configuration");
	cmdSetConfiguration->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdSetConfiguration->setBaseColor(gui_baseCol);
	cmdSetConfiguration->setId("cmdSetConfig");
	cmdSetConfiguration->addActionListener(amigaModelActionListener);

	lblWhdload = new gcn::Label("WHDLoad auto-config:");
	cboWhdload = new gcn::DropDown(&whdloadFileList);
	cboWhdload->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboWhdload->getHeight());
	cboWhdload->setBaseColor(gui_baseCol);
	cboWhdload->setBackgroundColor(colTextboxBackground);
	cboWhdload->setSelectionColor(gui_selection_color);
	cboWhdload->setId("cboQsWhdload");
	cboWhdload->addActionListener(whdloadActionListener);

	cmdWhdloadEject = new gcn::Button("Eject");
	cmdWhdloadEject->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
	cmdWhdloadEject->setBaseColor(gui_baseCol);
	cmdWhdloadEject->setId("cmdQsWhdloadEject");
	cmdWhdloadEject->addActionListener(whdloadButtonActionListener);

	cmdWhdloadSelect = new gcn::Button("Select file");
	cmdWhdloadSelect->setSize(BUTTON_WIDTH + 10, SMALL_BUTTON_HEIGHT);
	cmdWhdloadSelect->setBaseColor(gui_baseCol);
	cmdWhdloadSelect->setId("cmdQsWhdloadSelect");
	cmdWhdloadSelect->addActionListener(whdloadButtonActionListener);

	category.panel->add(lblModel, DISTANCE_BORDER, posY);
	category.panel->add(cboModel, DISTANCE_BORDER + lblModel->getWidth() + 8, posY);
	category.panel->add(chkNTSC, cboModel->getX() + cboModel->getWidth() + 8, posY);
	posY += cboModel->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblConfig, DISTANCE_BORDER, posY);
	category.panel->add(cboConfig, DISTANCE_BORDER + lblConfig->getWidth() + 8, posY);
	posY += cboConfig->getHeight() + DISTANCE_NEXT_Y * 2;

	for (auto i = 0; i < 2; ++i)
	{
		posX = DISTANCE_BORDER;
		category.panel->add(chkqsDFx[i], posX, posY);
		posX += chkqsDFx[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cboqsDFxType[i], posX, posY);
		posX += cboqsDFxType[i]->getWidth() + DISTANCE_NEXT_X * 2;
		category.panel->add(chkqsDFxWriteProtect[i], posX, posY);
		posX += chkqsDFxWriteProtect[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cmdqsDFxInfo[i], posX, posY);
		posX += cmdqsDFxInfo[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cmdqsDFxEject[i], posX, posY);
		posX += cmdqsDFxEject[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cmdqsDFxSelect[i], posX, posY);
		posY += cmdqsDFxEject[i]->getHeight() + 8;

		category.panel->add(cboqsDFxFile[i], DISTANCE_BORDER, posY);
		posY += cboqsDFxFile[i]->getHeight() + DISTANCE_NEXT_Y + 4;
	}

	category.panel->add(chkCD, chkqsDFx[1]->getX(), chkqsDFx[1]->getY());
	category.panel->add(cmdCDEject, cmdqsDFxEject[1]->getX(), cmdqsDFxEject[1]->getY());
	category.panel->add(cmdCDSelect, cmdqsDFxSelect[1]->getX(), cmdqsDFxSelect[1]->getY());
	category.panel->add(cboCDFile, cboqsDFxFile[1]->getX(), cboqsDFxFile[1]->getY());

	category.panel->add(chkQuickstartMode, category.panel->getWidth() - chkQuickstartMode->getWidth() - DISTANCE_BORDER,
						posY);
	posY += chkQuickstartMode->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cmdSetConfiguration, DISTANCE_BORDER, posY);

	posY += cmdSetConfiguration->getHeight() + DISTANCE_NEXT_Y * 3;
	category.panel->add(lblWhdload, DISTANCE_BORDER, posY);
	category.panel->add(cmdWhdloadEject, lblWhdload->getX() + lblWhdload->getWidth() + DISTANCE_NEXT_X * 16, posY);
	category.panel->add(cmdWhdloadSelect, cmdWhdloadEject->getX() + cmdWhdloadEject->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cmdWhdloadSelect->getHeight() + 8;
	category.panel->add(cboWhdload, DISTANCE_BORDER, posY);

	chkCD->setVisible(false);
	cmdCDEject->setVisible(false);
	cmdCDSelect->setVisible(false);
	cboCDFile->setVisible(false);

	bIgnoreListChange = false;

	cboModel->setSelected(quickstart_model);
	CountModelConfigs();
	cboConfig->setSelected(quickstart_conf);
	SetControlState(quickstart_model);

	// Only change the current prefs if we're not already emulating
	if (!emulating && !config_loaded)
		AdjustPrefs();

	RefreshPanelQuickstart();
}

void ExitPanelQuickstart()
{
	delete lblModel;
	delete cboModel;
	delete lblConfig;
	delete cboConfig;
	delete chkNTSC;
	for (auto i = 0; i < 2; ++i)
	{
		delete chkqsDFx[i];
		delete cboqsDFxType[i];
		delete chkqsDFxWriteProtect[i];
		delete cmdqsDFxInfo[i];
		delete cmdqsDFxEject[i];
		delete cmdqsDFxSelect[i];
		delete cboqsDFxFile[i];
	}
	delete chkCD;
	delete cmdCDEject;
	delete cmdCDSelect;
	delete cboCDFile;
	delete chkQuickstartMode;
	delete cmdSetConfiguration;

	delete lblWhdload;
	delete cboWhdload;
	delete cmdWhdloadEject;
	delete cmdWhdloadSelect;

	delete amigaModelActionListener;
	delete ntscButtonActionListener;
	delete qsdfxCheckActionListener;
	delete qsDriveTypeActionListener;
	delete qsdfxButtonActionListener;
	delete diskFileActionListener;
	delete cdButtonActionListener;
	delete cdFileActionListener;
	delete quickstartModeActionListener;
	delete whdloadActionListener;
	delete whdloadButtonActionListener;
}

static void AdjustDropDownControls()
{
	bIgnoreListChange = true;

	for (auto i = 0; i < 2; ++i)
	{
		cboqsDFxFile[i]->clearSelected();

		if (changed_prefs.floppyslots[i].dfxtype != DRV_NONE && strlen(changed_prefs.floppyslots[i].df) > 0)
		{
			for (auto j = 0; j < static_cast<int>(lstMRUDiskList.size()); ++j)
			{
				if (strcmp(lstMRUDiskList[j].c_str(), changed_prefs.floppyslots[i].df) == 0)
				{
					cboqsDFxFile[i]->setSelected(j);
					break;
				}
			}
		}
	}

	cboCDFile->clearSelected();
	if (changed_prefs.cdslots[0].inuse && strlen(changed_prefs.cdslots[0].name) > 0)
	{
		for (auto i = 0; i < static_cast<int>(lstMRUCDList.size()); ++i)
		{
			if (lstMRUCDList[i].c_str() != changed_prefs.cdslots[0].name)
			{
				cboCDFile->setSelected(i);
				break;
			}
		}
	}

	cboWhdload->clearSelected();
	if (!whdload_prefs.whdload_filename.empty())
	{
		for (auto i = 0; i < static_cast<int>(lstMRUWhdloadList.size()); ++i)
		{
			if (lstMRUWhdloadList[i] == whdload_prefs.whdload_filename)
			{
				cboWhdload->setSelected(i);
				break;
			}
		}
	}

	bIgnoreListChange = false;
}

void RefreshPanelQuickstart()
{
	auto prev_available = true;

	chkNTSC->setSelected(changed_prefs.ntscmode);

	RefreshWhdListModel();
	AdjustDropDownControls();

	changed_prefs.nr_floppies = 0;
	for (auto i = 0; i < 2; ++i)
	{
		const auto drive_enabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
		const auto disk_in_drive = strlen(changed_prefs.floppyslots[i].df) > 0;

		chkqsDFx[i]->setSelected(drive_enabled);
		const int nn = fromdfxtype(i, changed_prefs.floppyslots[i].dfxtype, changed_prefs.floppyslots[i].dfxsubtype);
		cboqsDFxType[i]->setSelected(nn + 1);
		chkqsDFxWriteProtect[i]->setSelected(disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df, i));
		if (i == 0)
			chkqsDFx[i]->setEnabled(false);
		else
			chkqsDFx[i]->setEnabled(prev_available);

		cmdqsDFxInfo[i]->setEnabled(drive_enabled && nn < 5 && disk_in_drive);
		chkqsDFxWriteProtect[i]->setEnabled(drive_enabled && !changed_prefs.floppy_read_only && nn < 5);
		cmdqsDFxEject[i]->setEnabled(drive_enabled && nn < 5 && disk_in_drive);
		cmdqsDFxSelect[i]->setEnabled(drive_enabled && nn < 5);
		cboqsDFxFile[i]->setEnabled(drive_enabled && nn < 5);

		prev_available = drive_enabled;
		if (drive_enabled)
			changed_prefs.nr_floppies = i + 1;
	}

	chkCD->setSelected(changed_prefs.cdslots[0].inuse);
	cmdCDEject->setEnabled(changed_prefs.cdslots[0].inuse);
	cmdCDSelect->setEnabled(changed_prefs.cdslots[0].inuse);
	cboCDFile->setEnabled(changed_prefs.cdslots[0].inuse);

	chkQuickstartMode->setSelected(amiberry_options.quickstart_start);
}

bool HelpPanelQuickstart(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Simplified start of emulation by just selecting the Amiga model and the disk/CD");
	helptext.emplace_back("you want to use.");
	helptext.emplace_back(" ");
	helptext.emplace_back("After selecting the Amiga model, you can choose from a small list of standard");
	helptext.emplace_back("configurations for this model to start with. Depending on the model selected,");
	helptext.emplace_back("the floppy or CD drive options will be enabled for you, which you can use to");
	helptext.emplace_back("insert any floppy disk or CD images, accordingly.");
	helptext.emplace_back("If you need more advanced control over the hardware you want to emulate, you");
	helptext.emplace_back("can always use the rest of the GUI for that.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can reset the current configuration to your selected model, by clicking the");
	helptext.emplace_back("Set Configuration button.");
	helptext.emplace_back(" ");
	helptext.emplace_back("When you activate \"Start in Quickstart mode\", the next time you run the emulator,");
	helptext.emplace_back("it  will start with the QuickStart panel. Otherwise you start in the configurations panel.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can optionally select a WHDLoad LHA title, and have Amiberry auto-configure");
	helptext.emplace_back("all settings, based on the WHDLoad XML (assuming the title is found there).");
	helptext.emplace_back("Then you can just click on Start to begin!");
	return true;
}
