#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "disk.h"
#include "memory.h"
#include "autoconf.h"
#include "filesys.h"
#include "blkdev.h"
#include "gui_handling.h"
#include "fsdb_host.h"
#include "rommgr.h"
#include "uae.h"

enum
{
	COL_DEVICE,
	COL_VOLUME,
	COL_PATH,
	COL_READWRITE,
	COL_SIZE,
	COL_BOOTPRI,
	COL_COUNT
};

static const char* column_caption[] =
{
	"Device", "Volume", "Path", "R/W", "Size", "Bootpri"
};
static const int column_size[] =
{
	55, // Device
	65, // Volume
	260, // Path
	30, // R/W
	55, // Size
	40 // Bootpri
};

static void AdjustDropDownControls();
static bool bIgnoreListChange = false;

static gcn::Label* lblList[COL_COUNT];
static gcn::Container* listEntry[MAX_HD_DEVICES];
static gcn::TextField* listCells[MAX_HD_DEVICES][COL_COUNT];
static gcn::Button* listCmdProps[MAX_HD_DEVICES];
static gcn::ImageButton* listCmdDelete[MAX_HD_DEVICES];
static gcn::Button* cmdAddDirectory;
static gcn::Button* cmdAddHardfile;
static gcn::Button* cmdAddHardDrive;
static gcn::Button* cmdCreateHardfile;
static gcn::Button* cmdAddCDDrive;
static gcn::Button* cmdAddTapeDrive;

static gcn::CheckBox* chkCDFSAutomount;
static gcn::CheckBox* chkCD;
static gcn::DropDown* cboCDFile;
static gcn::Button* cmdCDEject;
static gcn::Button* cmdCDSelectFile;
static gcn::CheckBox* chkCDTurbo;

static void harddisktype(TCHAR* s, const struct uaedev_config_info* ci)
{
	switch (ci->type)
	{
	case UAEDEV_CD:
		_tcscpy(s, _T("CD"));
		break;
	case UAEDEV_TAPE:
		_tcscpy(s, _T("TAPE"));
		break;
	case UAEDEV_HDF:
		_tcscpy(s, _T("HDF"));
		break;
	default:
		_tcscpy(s, _T("n/a"));
		break;
	}
}

static gcn::StringListModel cdfileList;

static void RefreshCDListModel()
{
	cdfileList.clear();
	auto cd_drives = get_cd_drives();
	if (!cd_drives.empty())
	{
		for (const auto& drive : cd_drives)
		{
			cdfileList.add(drive);
		}
	}
	for(const auto & i : lstMRUCDList)
	{
		const std::string full_path = i;
		const std::string filename = full_path.substr(full_path.find_last_of("/\\") + 1);
		cdfileList.add(std::string(filename).append(" { ").append(full_path).append(" }"));
	}
}

class HDRemoveActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < MAX_HD_DEVICES; ++i)
		{
			if (actionEvent.getSource() == listCmdDelete[i])
			{
				kill_filesys_unitconfig(&changed_prefs, i);
				gui_force_rtarea_hdchange();
				break;
			}
		}
		cmdAddDirectory->requestFocus();
		RefreshPanelHD();
	}
};

static HDRemoveActionListener* hdRemoveActionListener;

class HDEditActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		int type;
		struct uaedev_config_data* uci;
		struct mountedinfo mi;

		for (auto i = 0; i < MAX_HD_DEVICES; ++i)
		{
			if (actionEvent.getSource() == listCmdProps[i])
			{
				if (i >= changed_prefs.mountitems)
					return;

				uci = &changed_prefs.mountconfig[i];

				type = get_filesys_unitconfig(&changed_prefs, i, &mi);
				if (type < 0)
				{
					type = uci->ci.type == UAEDEV_HDF ? FILESYS_HARDFILE : FILESYS_VIRTUAL;
				}

				if (uci->ci.type == UAEDEV_CD)
				{
					memcpy(&current_cddlg.ci, uci, sizeof(struct uaedev_config_info));
					if (EditCDDrive(i))
					{
						new_cddrive(i);
						gui_force_rtarea_hdchange();
					}
				}
				else if (uci->ci.type == UAEDEV_TAPE)
				{
					memcpy(&current_tapedlg.ci, uci, sizeof(struct uaedev_config_info));
					if (EditTapeDrive(i))
					{
						new_tapedrive(i);
						gui_force_rtarea_hdchange();
					}
				}
				else if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB)
				{
					current_hfdlg.forcedcylinders = uci->ci.highcyl;
					memcpy(&current_hfdlg.ci, uci, sizeof(struct uaedev_config_info));
					if (EditFilesysHardfile(i))
					{
						new_hardfile(i);
						gui_force_rtarea_hdchange();
					}
				}
				else if (type == FILESYS_HARDDRIVE)
				{
					memcpy(&current_hfdlg.ci, uci, sizeof(struct uaedev_config_info));
					if (EditFilesysHardDrive(i))
					{
						new_harddrive(i);
						gui_force_rtarea_hdchange();
					}
				}
				else /* Filesystem */
				{
					memcpy(&current_fsvdlg.ci, uci, sizeof(struct uaedev_config_info));
					if (EditFilesysVirtual(i))
					{
						new_filesys(i);
						gui_force_rtarea_hdchange();
					}
				}
				
				listCmdProps[i]->requestFocus();
				break;
			}
		}
		RefreshPanelHD();
	}
};

static HDEditActionListener* hdEditActionListener;

class HDAddActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdAddDirectory)
		{
			if (EditFilesysVirtual(-1))
			{
				new_filesys(-1);
				gui_force_rtarea_hdchange();
			}
			cmdAddDirectory->requestFocus();
		}
		else if (actionEvent.getSource() == cmdAddHardfile)
		{
			if (EditFilesysHardfile(-1))
			{
				new_hardfile(-1);
				gui_force_rtarea_hdchange();
			}
			cmdAddHardfile->requestFocus();
		}
		else if (actionEvent.getSource() == cmdAddHardDrive)
		{
			if (EditFilesysHardDrive(-1))
			{
				new_harddrive(-1);
				gui_force_rtarea_hdchange();
			}
			cmdAddHardDrive->requestFocus();
		}
		else if (actionEvent.getSource() == cmdAddCDDrive)
		{
			if (EditCDDrive(-1))
			{
				new_cddrive(-1);
				gui_force_rtarea_hdchange();
			}
			cmdAddCDDrive->requestFocus();
		}
		else if (actionEvent.getSource() == cmdAddTapeDrive)
		{
			if (EditTapeDrive(-1))
			{
				new_tapedrive(-1);
				gui_force_rtarea_hdchange();
			}
			cmdAddTapeDrive->requestFocus();
		}
		else if (actionEvent.getSource() == cmdCreateHardfile)
		{
			if (CreateFilesysHardfile())
				gui_force_rtarea_hdchange();
			cmdCreateHardfile->requestFocus();
		}
		RefreshPanelHD();
	}
};

HDAddActionListener* hdAddActionListener;

class CDCheckActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == chkCD)
		{
			if (changed_prefs.cdslots[0].inuse)
			{
				changed_prefs.cdslots[0].inuse = false;
				changed_prefs.cdslots[0].type = SCSI_UNIT_DISABLED;
				changed_prefs.cdslots[0].name[0] = 0;
				AdjustDropDownControls();
			}
			else
			{
				changed_prefs.cdslots[0].inuse = true;
				changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
			}
		}
		else if (actionEvent.getSource() == chkCDTurbo)
			changed_prefs.cd_speed = chkCDTurbo->isSelected() ? 0 : 100;
		else if (actionEvent.getSource() == chkCDFSAutomount)
			changed_prefs.automount_cddrives = chkCDFSAutomount->isSelected();

		RefreshPanelHD();
		RefreshPanelQuickstart();
		RefreshPanelExpansions();
	}
};

CDCheckActionListener* cdCheckActionListener;

class CDButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdCDEject)
		{
			//---------------------------------------
			// Eject CD from drive
			//---------------------------------------
			changed_prefs.cdslots[0].name[0] = 0;
			changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
			cboCDFile->clearSelected();
			AdjustDropDownControls();
		}
		else if (actionEvent.getSource() == cmdCDSelectFile)
		{
			//---------------------------------------
			// Insert CD into drive
			//---------------------------------------
			std::string tmp;
			if (strlen(changed_prefs.cdslots[0].name) > 0)
				tmp = std::string(changed_prefs.cdslots[0].name);
			else
				tmp = get_cdrom_path();

			tmp = SelectFile("Select CD image file", tmp, cdfile_filter);
			if (!tmp.empty())
			{
				if (strncmp(changed_prefs.cdslots[0].name, tmp.c_str(), MAX_DPATH) != 0)
				{
					strncpy(changed_prefs.cdslots[0].name, tmp.c_str(), MAX_DPATH);
					changed_prefs.cdslots[0].inuse = true;
					changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
					add_file_to_mru_list(lstMRUCDList, tmp);

					RefreshCDListModel();
					AdjustDropDownControls();
					if (!last_loaded_config[0])
						set_last_active_config(tmp.c_str());
				}
			}
			cmdCDSelectFile->requestFocus();
		}

		RefreshPanelHD();
		RefreshPanelQuickstart();
	}
};

CDButtonActionListener* cdButtonActionListener;

class CDFileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		//---------------------------------------
		// CD image from list selected
		//---------------------------------------
		if (!bIgnoreListChange)
		{
			const auto idx = cboCDFile->getSelected();

			if (idx < 0)
			{
				changed_prefs.cdslots[0].name[0] = 0;
				AdjustDropDownControls();
			}
			else
			{
				const auto selected = cdfileList.getElementAt(idx);
				// if selected starts with /dev/sr, it's a CD drive
				// TODO: Check this on MacOS, it might be different there
				if (selected.find("/dev/") == 0)
				{
					strncpy(changed_prefs.cdslots[0].name, selected.c_str(), MAX_DPATH);
					changed_prefs.cdslots[0].inuse = true;
					changed_prefs.cdslots[0].type = SCSI_UNIT_IOCTL;
				}
				else
				{
					const auto element = get_full_path_from_disk_list(cdfileList.getElementAt(idx));
					if (element != changed_prefs.cdslots[0].name)
					{
						strncpy(changed_prefs.cdslots[0].name, element.c_str(), MAX_DPATH);
						DISK_history_add(changed_prefs.cdslots[0].name, -1, HISTORY_CD, 0);
						changed_prefs.cdslots[0].inuse = true;
						changed_prefs.cdslots[0].type = SCSI_UNIT_DEFAULT;
						lstMRUCDList.erase(lstMRUCDList.begin() + idx);
						lstMRUCDList.insert(lstMRUCDList.begin(), changed_prefs.cdslots[0].name);
						RefreshCDListModel();
						bIgnoreListChange = true;
						cboCDFile->setSelected(0);
						bIgnoreListChange = false;
						if (!last_loaded_config[0])
							set_last_active_config(element.c_str());
					}
				}
			}
		}
		RefreshPanelHD();
	}
};

static CDFileActionListener* cdFileActionListener;

void InitPanelHD(const config_category& category)
{
	int row, col;
	auto posY = DISTANCE_BORDER / 2;

	RefreshCDListModel();

	hdRemoveActionListener = new HDRemoveActionListener();
	hdEditActionListener = new HDEditActionListener();
	hdAddActionListener = new HDAddActionListener();

	for (col = 0; col < COL_COUNT; ++col)
		lblList[col] = new gcn::Label(column_caption[col]);

	for (row = 0; row < MAX_HD_DEVICES; ++row)
	{
		listEntry[row] = new gcn::Container();
		listEntry[row]->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, SMALL_BUTTON_HEIGHT + 4);
		listEntry[row]->setBaseColor(gui_base_color);
		listEntry[row]->setBackgroundColor(gui_background_color);
		listEntry[row]->setForegroundColor(gui_foreground_color);
		listEntry[row]->setFrameSize(0);

		listCmdProps[row] = new gcn::Button("...");
		listCmdProps[row]->setBaseColor(gui_base_color);
		listCmdProps[row]->setForegroundColor(gui_foreground_color);
		listCmdProps[row]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		std::string id_string = "cmdProp" + to_string(row);
		listCmdProps[row]->setId(id_string);
		listCmdProps[row]->addActionListener(hdEditActionListener);

		listCmdDelete[row] = new gcn::ImageButton(prefix_with_data_path("delete.png"));
		listCmdDelete[row]->setBaseColor(gui_base_color);
		listCmdDelete[row]->setForegroundColor(gui_foreground_color);
		listCmdDelete[row]->setSize(SMALL_BUTTON_HEIGHT, SMALL_BUTTON_HEIGHT);
		id_string = "cmdDel" + to_string(row);
		listCmdDelete[row]->setId(id_string);
		listCmdDelete[row]->addActionListener(hdRemoveActionListener);

		for (col = 0; col < COL_COUNT; ++col)
		{
			listCells[row][col] = new gcn::TextField();
			listCells[row][col]->setSize(column_size[col], SMALL_BUTTON_HEIGHT);
			listCells[row][col]->setEnabled(false);
			listCells[row][col]->setBaseColor(gui_base_color);
			listCells[row][col]->setBackgroundColor(gui_background_color);
			listCells[row][col]->setForegroundColor(gui_foreground_color);
		}
	}

	cmdAddDirectory = new gcn::Button("Add Directory/Archive");
	cmdAddDirectory->setBaseColor(gui_base_color);
	cmdAddDirectory->setForegroundColor(gui_foreground_color);
	cmdAddDirectory->setSize(cmdAddDirectory->getWidth() + 8, BUTTON_HEIGHT);
	cmdAddDirectory->setId("cmdAddDir");
	cmdAddDirectory->addActionListener(hdAddActionListener);

	cmdAddHardfile = new gcn::Button("Add Hardfile");
	cmdAddHardfile->setBaseColor(gui_base_color);
	cmdAddHardfile->setForegroundColor(gui_foreground_color);
	cmdAddHardfile->setSize(cmdAddDirectory->getWidth(), BUTTON_HEIGHT);
	cmdAddHardfile->setId("cmdAddHDF");
	cmdAddHardfile->addActionListener(hdAddActionListener);

	cmdAddHardDrive = new gcn::Button("Add Hard Drive");
	cmdAddHardDrive->setBaseColor(gui_base_color);
	cmdAddHardDrive->setForegroundColor(gui_foreground_color);
	cmdAddHardDrive->setSize(cmdAddDirectory->getWidth(), BUTTON_HEIGHT);
	cmdAddHardDrive->setId("cmdAddHardDrive");
	cmdAddHardDrive->addActionListener(hdAddActionListener);

	cmdAddCDDrive = new gcn::Button("Add CD Drive");
	cmdAddCDDrive->setBaseColor(gui_base_color);
	cmdAddCDDrive->setForegroundColor(gui_foreground_color);
	cmdAddCDDrive->setSize(cmdAddDirectory->getWidth(), BUTTON_HEIGHT);
	cmdAddCDDrive->setId("cmdAddCDDrive");
	cmdAddCDDrive->addActionListener(hdAddActionListener);
	cmdAddCDDrive->setEnabled(true);

	cmdAddTapeDrive = new gcn::Button("Add Tape Drive");
	cmdAddTapeDrive->setBaseColor(gui_base_color);
	cmdAddTapeDrive->setForegroundColor(gui_foreground_color);
	cmdAddTapeDrive->setSize(cmdAddDirectory->getWidth(), BUTTON_HEIGHT);
	cmdAddTapeDrive->setId("cmdAddTapeDrive");
	cmdAddTapeDrive->addActionListener(hdAddActionListener);

	cmdCreateHardfile = new gcn::Button("Create Hardfile");
	cmdCreateHardfile->setBaseColor(gui_base_color);
	cmdCreateHardfile->setForegroundColor(gui_foreground_color);
	cmdCreateHardfile->setSize(cmdAddDirectory->getWidth(), BUTTON_HEIGHT);
	cmdCreateHardfile->setId("cmdCreateHDF");
	cmdCreateHardfile->addActionListener(hdAddActionListener);

	cdCheckActionListener = new CDCheckActionListener();
	cdButtonActionListener = new CDButtonActionListener();
	cdFileActionListener = new CDFileActionListener();

	chkCDFSAutomount = new gcn::CheckBox("CDFS automount CD/DVD drives");
	chkCDFSAutomount->setId("chkCDFSAutomount");
	chkCDFSAutomount->setBaseColor(gui_base_color);
	chkCDFSAutomount->setBackgroundColor(gui_background_color);
	chkCDFSAutomount->setForegroundColor(gui_foreground_color);
	chkCDFSAutomount->addActionListener(cdCheckActionListener);

	chkCD = new gcn::CheckBox("CD drive/image");
	chkCD->setId("chkCD");
	chkCD->setBaseColor(gui_base_color);
	chkCD->setBackgroundColor(gui_background_color);
	chkCD->setForegroundColor(gui_foreground_color);
	chkCD->addActionListener(cdCheckActionListener);

	chkCDTurbo = new gcn::CheckBox("CDTV/CDTV-CR/CD32 turbo CD read speed");
	chkCDTurbo->setId("chkCDTurbo");
	chkCDTurbo->setBaseColor(gui_base_color);
	chkCDTurbo->setBackgroundColor(gui_background_color);
	chkCDTurbo->setForegroundColor(gui_foreground_color);
	chkCDTurbo->addActionListener(cdCheckActionListener);

	cmdCDEject = new gcn::Button("Eject");
	cmdCDEject->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
	cmdCDEject->setBaseColor(gui_base_color);
	cmdCDEject->setForegroundColor(gui_foreground_color);
	cmdCDEject->setId("cmdCDEject");
	cmdCDEject->addActionListener(cdButtonActionListener);

	cmdCDSelectFile = new gcn::Button("Select image file");
	cmdCDSelectFile->setSize(SMALL_BUTTON_WIDTH * 6, SMALL_BUTTON_HEIGHT);
	cmdCDSelectFile->setBaseColor(gui_base_color);
	cmdCDSelectFile->setForegroundColor(gui_foreground_color);
	cmdCDSelectFile->setId("cmdCDSelectFile");
	cmdCDSelectFile->addActionListener(cdButtonActionListener);

	cboCDFile = new gcn::DropDown(&cdfileList);
	cboCDFile->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboCDFile->getHeight());
	cboCDFile->setBaseColor(gui_base_color);
	cboCDFile->setBackgroundColor(gui_background_color);
	cboCDFile->setForegroundColor(gui_foreground_color);
	cboCDFile->setSelectionColor(gui_selection_color);
	cboCDFile->setId("cboCD");
	cboCDFile->addActionListener(cdFileActionListener);

	int posX = DISTANCE_BORDER + 2 + SMALL_BUTTON_WIDTH + 34;
	for (col = 0; col < COL_COUNT; ++col)
	{
		category.panel->add(lblList[col], posX, posY);
		posX += column_size[col];
	}
	posY += lblList[0]->getHeight() + 2;

	for (row = 0; row < MAX_HD_DEVICES; ++row)
	{
		posX = 1;
		listEntry[row]->add(listCmdProps[row], posX, 2);
		posX += listCmdProps[row]->getWidth() + 4;
		listEntry[row]->add(listCmdDelete[row], posX, 2);
		posX += listCmdDelete[row]->getWidth() + 8;
		for (col = 0; col < COL_COUNT; ++col)
		{
			listEntry[row]->add(listCells[row][col], posX, 2);
			posX += column_size[col];
		}
		category.panel->add(listEntry[row], DISTANCE_BORDER, posY);
		posY += listEntry[row]->getHeight() + DISTANCE_NEXT_Y / 2;
	}

	posY += DISTANCE_NEXT_Y / 2;
	category.panel->add(cmdAddDirectory, DISTANCE_BORDER, posY);
	category.panel->add(cmdAddHardfile, cmdAddDirectory->getX() + cmdAddDirectory->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cmdAddHardDrive, cmdAddHardfile->getX() + cmdAddHardfile->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cmdAddDirectory->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cmdAddCDDrive, cmdAddDirectory->getX(), posY);
	category.panel->add(cmdAddTapeDrive, cmdAddCDDrive->getX() + cmdAddCDDrive->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cmdCreateHardfile, cmdAddTapeDrive->getX() + cmdAddTapeDrive->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cmdCreateHardfile->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(chkCDFSAutomount, DISTANCE_BORDER, posY + 2);
	posY = chkCDFSAutomount->getY() + chkCDFSAutomount->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(chkCD, DISTANCE_BORDER, posY);
	category.panel->add(cmdCDEject, category.panel->getWidth() - cmdCDEject->getWidth() - DISTANCE_BORDER, posY);
	category.panel->add(cmdCDSelectFile, cmdCDEject->getX() - DISTANCE_NEXT_X - cmdCDSelectFile->getWidth(), posY);
	posY += cmdCDSelectFile->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboCDFile, DISTANCE_BORDER, posY);
	posY += cboCDFile->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkCDTurbo, DISTANCE_BORDER, posY);

	cboCDFile->clearSelected();
	RefreshPanelHD();
}

void ExitPanelHD()
{
	int col;

	for (col = 0; col < COL_COUNT; ++col)
		delete lblList[col];

	for (auto row = 0; row < MAX_HD_DEVICES; ++row)
	{
		delete listCmdProps[row];
		delete listCmdDelete[row];
		for (col = 0; col < COL_COUNT; ++col)
			delete listCells[row][col];
		delete listEntry[row];
	}

	delete cmdAddDirectory;
	delete cmdAddHardfile;
	delete cmdAddHardDrive;
	delete cmdAddCDDrive;
	delete cmdAddTapeDrive;
	delete cmdCreateHardfile;

	delete chkCDFSAutomount;
	delete chkCD;
	delete cmdCDEject;
	delete cmdCDSelectFile;
	delete cboCDFile;
	delete chkCDTurbo;

	delete cdCheckActionListener;
	delete cdButtonActionListener;
	delete cdFileActionListener;

	delete hdRemoveActionListener;
	delete hdEditActionListener;
	delete hdAddActionListener;
}

static void AdjustDropDownControls()
{
	bIgnoreListChange = true;

	if (changed_prefs.cdslots[0].inuse
		&& strlen(changed_prefs.cdslots[0].name) > 0
		&& changed_prefs.cdslots[0].type == SCSI_UNIT_DEFAULT)
	{
		cboCDFile->clearSelected();
		if (changed_prefs.cdslots[0].inuse && strlen(changed_prefs.cdslots[0].name) > 0)
		{
			for (auto i = 0; i < static_cast<int>(lstMRUCDList.size()); ++i)
			{
				if (strcmp(lstMRUCDList[i].c_str(), changed_prefs.cdslots[0].name) == 0)
				{
					cboCDFile->setSelected(i);
					break;
				}
			}
		}
	}
	bIgnoreListChange = false;
}

void RefreshPanelHD()
{
	AdjustDropDownControls();

	for (auto row = 0; row < MAX_HD_DEVICES; ++row)
	{
		if (row < changed_prefs.mountitems)
		{
			TCHAR bootpri_str[32];
			TCHAR volname_str[256];
			TCHAR devname_str[256];
			TCHAR size_str[32];
			TCHAR blocksize_str[32];
			auto* uci = &changed_prefs.mountconfig[row];
			auto* const ci = &uci->ci;
			int nosize = 0;
			struct mountedinfo mi{};

			int type = get_filesys_unitconfig(&changed_prefs, row, &mi);
			if (type < 0)
			{
				type = ci->type == UAEDEV_HDF || ci->type == UAEDEV_CD || ci->type == UAEDEV_TAPE ? FILESYS_HARDFILE : FILESYS_VIRTUAL;
				nosize = 1;
			}
			if (mi.size < 0)
				nosize = 1;
			TCHAR* rootdir = my_strdup(mi.rootdir);
			TCHAR* rootdirp = rootdir;
			if (!_tcsncmp(rootdirp, _T("HD_"), 3))
				rootdirp += 3;
			if (rootdirp[0] == ':') {
				rootdirp++;
				TCHAR* p = _tcschr(rootdirp, ':');
				if (p)
					*p = 0;
			}

			if (nosize)
				_tcscpy(size_str, _T("n/a"));
			else if (mi.size >= 1024 * 1024 * 1024)
				_sntprintf(size_str, sizeof size_str, _T("%.1fG"), static_cast<double>(static_cast<uae_u32>(mi.size / (1024 * 1024))) / 1024.0);
			else if (mi.size < 10 * 1024 * 1024)
				_sntprintf(size_str, sizeof size_str, _T("%lldK"), mi.size / 1024);
			else
				_sntprintf(size_str, sizeof size_str, _T("%.1fM"), static_cast<double>(static_cast<uae_u32>(mi.size / (1024))) / 1024.0);

			int ctype = ci->controller_type;
			if (ctype >= HD_CONTROLLER_TYPE_IDE_FIRST && ctype <= HD_CONTROLLER_TYPE_IDE_LAST) {
				const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
				const TCHAR* idedevs[] = {
					_T("IDE:%d"),
					_T("A600/A1200/A4000:%d"),
				};
				_sntprintf(blocksize_str, sizeof blocksize_str, _T("%d"), ci->blocksize);
				if (ert) {
					if (ci->controller_type_unit == 0)
						_sntprintf(devname_str, sizeof devname_str, _T("%s:%d"), ert->friendlyname, ci->controller_unit);
					else
						_sntprintf(devname_str, sizeof devname_str, _T("%s:%d/%d"), ert->friendlyname, ci->controller_unit, ci->controller_type_unit + 1);
				}
				else {
					_sntprintf(devname_str, sizeof devname_str, idedevs[ctype - HD_CONTROLLER_TYPE_IDE_FIRST], ci->controller_unit);
				}
				harddisktype(volname_str, ci);
				_tcscpy(bootpri_str, _T("n/a"));
			}
			else if (ctype >= HD_CONTROLLER_TYPE_SCSI_FIRST && ctype <= HD_CONTROLLER_TYPE_SCSI_LAST) {
				TCHAR sid[8];
				const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
				const TCHAR* scsidevs[] = {
					_T("SCSI:%s"),
					_T("A3000:%s"),
					_T("A4000T:%s"),
					_T("CDTV:%s"),
				};
				if (ci->controller_unit == 8 && ert && !_tcscmp(ert->name, _T("a2091")))
					_tcscpy(sid, _T("XT"));
				else if (ci->controller_unit == 8 && ert && !_tcscmp(ert->name, _T("a2090a")))
					_tcscpy(sid, _T("ST-506"));
				else
					_sntprintf(sid, sizeof sid, _T("%d"), ci->controller_unit);
				_sntprintf(blocksize_str, sizeof blocksize_str, _T("%d"), ci->blocksize);
				if (ert) {
					if (ci->controller_type_unit == 0)
						_sntprintf(devname_str, sizeof devname_str, _T("%s:%s"), ert->friendlyname, sid);
					else
						_sntprintf(devname_str, sizeof devname_str, _T("%s:%s/%d"), ert->friendlyname, sid, ci->controller_type_unit + 1);
				}
				else {
					_sntprintf(devname_str, sizeof devname_str, scsidevs[ctype - HD_CONTROLLER_TYPE_SCSI_FIRST], sid);
				}
				harddisktype(volname_str, ci);
				_tcscpy(bootpri_str, _T("n/a"));
			}
			else if (ctype >= HD_CONTROLLER_TYPE_CUSTOM_FIRST && ctype <= HD_CONTROLLER_TYPE_CUSTOM_LAST) {
				TCHAR sid[8];
				const struct expansionromtype* ert = get_unit_expansion_rom(ctype);
				_sntprintf(sid, sizeof sid, _T("%d"), ci->controller_unit);
				if (ert) {
					if (ci->controller_type_unit == 0)
						_sntprintf(devname_str, sizeof devname_str, _T("%s:%s"), ert->friendlyname, sid);
					else
						_sntprintf(devname_str, sizeof devname_str, _T("%s:%s/%d"), ert->friendlyname, sid, ci->controller_type_unit + 1);
				}
				else {
					_sntprintf(devname_str, sizeof devname_str, _T("PCMCIA"));
				}
				harddisktype(volname_str, ci);
				_tcscpy(bootpri_str, _T("n/a"));
			}
			else if (type == FILESYS_HARDFILE) {
				_sntprintf(blocksize_str, sizeof blocksize_str, _T("%d"), ci->blocksize);
				_tcscpy(devname_str, ci->devname);
				_tcscpy(volname_str, _T("n/a"));
				_sntprintf(bootpri_str, sizeof bootpri_str, _T("%d"), ci->bootpri);
			}
			else if (type == FILESYS_HARDFILE_RDB || type == FILESYS_HARDDRIVE || ci->controller_type != HD_CONTROLLER_TYPE_UAE) {
				_sntprintf(blocksize_str, sizeof blocksize_str, _T("%d"), ci->blocksize);
				_sntprintf(devname_str, sizeof devname_str, _T("UAE:%d"), ci->controller_unit);
				_tcscpy(volname_str, _T("n/a"));
				_tcscpy(bootpri_str, _T("n/a"));
			}
			else if (type == FILESYS_TAPE) {
				_sntprintf(blocksize_str, sizeof blocksize_str, _T("%d"), ci->blocksize);
				_tcscpy(devname_str, _T("UAE"));
				harddisktype(volname_str, ci);
				_tcscpy(bootpri_str, _T("n/a"));
			}
			else {
				_tcscpy(blocksize_str, _T("n/a"));
				_tcscpy(devname_str, ci->devname);
				_tcscpy(volname_str, ci->volname);
				_tcscpy(size_str, _T("n/a"));
				_sntprintf(bootpri_str, sizeof bootpri_str, _T("%d"), ci->bootpri);
			}
			if (!mi.ismedia) {
				_tcscpy(blocksize_str, _T("n/a"));
				_tcscpy(size_str, _T("n/a"));
			}
			if (rootdirp[0] == 0) {
				xfree(rootdir);
				rootdir = my_strdup(_T("-"));
				rootdirp = rootdir;
			}

			listCells[row][COL_DEVICE]->setText(devname_str);
			listCells[row][COL_VOLUME]->setText(volname_str);
			listCells[row][COL_PATH]->setText(rootdirp);
			listCells[row][COL_READWRITE]->setText(ci->readonly ? _T("no") : _T("yes"));
			listCells[row][COL_SIZE]->setText(size_str);
			listCells[row][COL_BOOTPRI]->setText(bootpri_str);

			listCmdProps[row]->setEnabled(true);
			listCmdDelete[row]->setEnabled(true);
		}
		else
		{
			// Empty slot
			for (auto col = 0; col < COL_COUNT; ++col)
				listCells[row][col]->setText("");
			listCmdProps[row]->setEnabled(false);
			listCmdDelete[row]->setEnabled(false);
		}
	}

	chkCDFSAutomount->setSelected(changed_prefs.automount_cddrives);
	chkCD->setSelected(changed_prefs.cdslots[0].inuse);
	cmdCDEject->setEnabled(changed_prefs.cdslots[0].inuse);
	cmdCDSelectFile->setEnabled(changed_prefs.cdslots[0].inuse);
	cboCDFile->setEnabled(changed_prefs.cdslots[0].inuse);
	chkCDTurbo->setSelected(changed_prefs.cd_speed == 0);

	if (changed_prefs.cdslots[0].inuse && changed_prefs.cdslots[0].name[0])
	{
		struct device_info di = {};
		blkdev_get_info(&changed_prefs, 0, &di);
	}
}

int count_HDs(const uae_prefs* p)
{
	return p->mountitems;
}

bool HelpPanelHD(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Hard Drives and CD/DVD drives");
	helptext.emplace_back(" ");
	helptext.emplace_back("This panel allows you to add and configure virtual hard drives and CD/DVD drives.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(Use "Add Directory/Archive" to add a folder or Archive as a virtual Amiga drive.)");
	helptext.emplace_back(R"(Use "Add Hardfile to add an HDD image (hdf) as a hard disk.)");
	helptext.emplace_back(R"(Use "Add Hard Drive" to add a physical hard drive as a hard disk.)");
	helptext.emplace_back(R"(Use "Add Tape Drive" to add a tape drive (directory or file only).)");
	helptext.emplace_back(" ");
	helptext.emplace_back("With \"Create Hardfile\", you can create a new formatted HDF file up to 2 GB.");
	helptext.emplace_back("For large files, it will take some time to create the new hard disk. You have to");
	helptext.emplace_back("format the new HDD in the Amiga via the Workbench.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can use the \"...\" button to edit the selected drive. You can change the");
	helptext.emplace_back("name, volume label, path, read/write mode, size and boot priority.");
	helptext.emplace_back(" ");
	helptext.emplace_back("Use the \"Delete\" button to remove the selected drive.");
	helptext.emplace_back(" ");
	helptext.emplace_back("CD/DVD drives:");
	helptext.emplace_back(" ");
	helptext.emplace_back("Use \"CD drive/image\" to connect a CD drive. You can select an iso/cue file");
	helptext.emplace_back("or use the \"Eject\" button to remove the CD from the drive.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can enable the \"CDTV/CDTV-CR/CD32 turbo CD read speed\" option to speed up");
	helptext.emplace_back("the CD read speed.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can enable the \"CDFS automount CD/DVD drives\" option to automatically mount");
	helptext.emplace_back("CD/DVD drives on Workbench, when a CD/DVD is inserted.");
	helptext.emplace_back(" ");
	return true;
}
