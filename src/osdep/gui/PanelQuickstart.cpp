#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL_ttf.h>
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
#include "include/memory.h"
#include "disk.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "blkdev.h"
#include "gui.h"
#include "gui_handling.h"


static gcn::Label* lblModel;
static gcn::UaeDropDown* cboModel;
static gcn::Label* lblConfig;
static gcn::UaeDropDown* cboConfig;
static gcn::UaeCheckBox* chkNTSC;

static gcn::UaeCheckBox* chkDFx[2];
static gcn::UaeCheckBox* chkDFxWriteProtect[2];
static gcn::Button* cmdDFxInfo[2];
static gcn::Button* cmdDFxEject[2];
static gcn::Button* cmdDFxSelect[2];
static gcn::UaeDropDown* cboDFxFile[2];

static gcn::UaeCheckBox* chkCD;
static gcn::Button* cmdCDEject;
static gcn::Button* cmdCDSelect;
static gcn::UaeDropDown* cboCDFile;

static gcn::UaeCheckBox* chkQuickstartMode;
static gcn::Button* cmdSetConfiguration;

struct amigamodels
{
	int compalevels;
	char name[32];
	char configs[8][128];
};

static struct amigamodels amodels[] = {
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
			"\0"
		}
	},
	{
		4, "Amiga 1000", {
			"Basic non-expanded configuration",
			"\0"
		}
	},
	{
		4, "Amiga 1200", {
			"Basic non-expanded configuration",
			"4 MB Fast RAM expanded configuration",
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
		3, "CD32", {
			"CD32",
			"CD32 with Full Motion Video cartridge",
			"\0"
		}
	},
	{
		3, "CDTV", {
			"CDTV",
			"\0"
		}
	},
	{-1}
};

static const int numModels = 10;
static int numModelConfigs = 0;
static bool bIgnoreListChange = true;


static const char* diskfile_filter[] = {".adf", ".adz", ".fdi", ".ipf", ".zip", ".dms", ".gz", ".xz", "\0"};
static const char* cdfile_filter[] = {".cue", ".ccd", ".iso", "\0"};

static void AdjustDropDownControls(void);

static void CountModelConfigs(void)
{
	numModelConfigs = 0;
	if (quickstart_model >= 0 && quickstart_model < numModels)
	{
		for (auto& config : amodels[quickstart_model].configs)
		{
			if (config[0] == '\0')
				break;
			++numModelConfigs;
		}
	}
}


static void SetControlState(const int model)
{
	auto df1Visible = true;
	auto cdVisible = false;
	auto df0Editable = true;

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
		df0Editable = false;
		df1Visible = false;
		cdVisible = true;
		break;
	default:
		break;
	}

	chkDFxWriteProtect[0]->setEnabled(df0Editable && !changed_prefs.floppy_read_only);
	cmdDFxInfo[0]->setEnabled(df0Editable);
	cmdDFxEject[0]->setEnabled(df0Editable);
	cmdDFxSelect[0]->setEnabled(df0Editable);
	cboDFxFile[0]->setEnabled(df0Editable);

	chkDFx[1]->setVisible(df1Visible);
	chkDFxWriteProtect[1]->setVisible(df1Visible);
	cmdDFxInfo[1]->setVisible(df1Visible);
	cmdDFxEject[1]->setVisible(df1Visible);
	cmdDFxSelect[1]->setVisible(df1Visible);
	cboDFxFile[1]->setVisible(df1Visible);

	chkCD->setVisible(cdVisible);
	cmdCDEject->setVisible(cdVisible);
	cmdCDSelect->setVisible(cdVisible);
	cboCDFile->setVisible(cdVisible);
}


static void AdjustPrefs(void)
{
	const auto old_cs = changed_prefs.cs_compatible;

	built_in_prefs(&changed_prefs, quickstart_model, quickstart_conf, 0, 0);
	switch (quickstart_model)
	{
	case 0: // A500
	case 1: // A500+
	case 2: // A600
	case 3: // A1000
	case 4: // A1200
	case 5: // A3000
	case 6: // A4000
	case 7: // A4000T
		// df0 always active
		changed_prefs.floppyslots[0].dfxtype = DRV_35_DD;

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
		changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
		changed_prefs.gfx_size.width = 768;
		changed_prefs.gfx_size.height = 270;
		break;
	default:
		break;
	}
}


class AmigaModelListModel : public gcn::ListModel
{
public:
	AmigaModelListModel()
	= default;

	int getNumberOfElements() override
	{
		return numModels;
	}

	string getElementAt(int i) override
	{
		if (i < 0 || i >= numModels)
			return "---";
		return amodels[i].name;
	}
};

static AmigaModelListModel amigaModelList;


class AmigaConfigListModel : public gcn::ListModel
{
public:
	AmigaConfigListModel()
	= default;

	int getNumberOfElements() override
	{
		return numModelConfigs;
	}

	string getElementAt(const int i) override
	{
		if (quickstart_model < 0 || i < 0 || i >= numModelConfigs)
			return "---";
		return amodels[quickstart_model].configs[i];
	}
};

static AmigaConfigListModel amigaConfigList;


class QSDiskfileListModel : public gcn::ListModel
{
public:
	QSDiskfileListModel()
	= default;

	int getNumberOfElements() override
	{
		return lstMRUDiskList.size();
	}

	string getElementAt(const int i) override
	{
		if (i < 0 || i >= lstMRUDiskList.size())
			return "---";
		return lstMRUDiskList[i];
	}
};

static QSDiskfileListModel diskfileList;


class QSCDfileListModel : public gcn::ListModel
{
public:
	QSCDfileListModel()
	= default;

	int getNumberOfElements() override
	{
		return lstMRUCDList.size();
	}

	string getElementAt(const int i) override
	{
		if (i < 0 || i >= lstMRUCDList.size())
			return "---";
		return lstMRUCDList[i];
	}
};

static QSCDfileListModel cdfileList;


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
			char tmp[MAX_DPATH];

			if (strlen(changed_prefs.cdslots[0].name) > 0)
				strncpy(tmp, changed_prefs.cdslots[0].name, MAX_DPATH);
			else
				strncpy(tmp, currentDir, MAX_DPATH);

			if (SelectFile("Select CD image file", tmp, cdfile_filter))
			{
				if (strncmp(changed_prefs.cdslots[0].name, tmp, MAX_DPATH) != 0)
				{
					strncpy(changed_prefs.cdslots[0].name, tmp, MAX_DPATH);
					changed_prefs.cdslots[0].inuse = true;
					changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
					AddFileToCDList(tmp, 1);
					extractPath(tmp, currentDir);

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
					if (cdfileList.getElementAt(idx) != changed_prefs.cdslots[0].name)
					{
						strncpy(changed_prefs.cdslots[0].name, cdfileList.getElementAt(idx).c_str(), MAX_DPATH);
						changed_prefs.cdslots[0].inuse = true;
						changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
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
					DisableResume();
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
			RefreshAllPanels();
		}

		if (actionEvent.getSource() == cmdSetConfiguration)
		{
			AdjustPrefs();
			RefreshAllPanels();
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
					ShowMessage("Set/Clear write protect", "Failed to change write permission.",
					            "Maybe underlying filesystem doesn't support this.", "Ok", "");
					chkDFxWriteProtect[i]->requestFocus();
				}
				DISK_reinsert(i);
			}
		}

		RefreshPanelFloppy();
		RefreshPanelQuickstart();
	}
};

static QSDFxCheckActionListener* dfxCheckActionListener;


class QSDFxButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < 2; ++i)
		{
			if (actionEvent.getSource() == cmdDFxInfo[i])
			{
				//---------------------------------------
				// Show info about current disk-image
				//---------------------------------------
				//if (changed_prefs.floppyslots[i].dfxtype != DRV_NONE && strlen(changed_prefs.floppyslots[i].df) > 0);
				// ToDo: Show info dialog
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
					strncpy(tmp, currentDir, MAX_DPATH);
				if (SelectFile("Select disk image file", tmp, diskfile_filter))
				{
					if (strncmp(changed_prefs.floppyslots[i].df, tmp, MAX_DPATH) != 0)
					{
						strncpy(changed_prefs.floppyslots[i].df, tmp, MAX_DPATH);
						disk_insert(i, tmp);
						AddFileToDiskList(tmp, 1);
						extractPath(tmp, currentDir);

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

static QSDFxButtonActionListener* dfxButtonActionListener;


class QSDiskFileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		for (auto i = 0; i < 2; ++i)
		{
			if (actionEvent.getSource() == cboDFxFile[i])
			{
				//---------------------------------------
				// Diskimage from list selected
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
		quickstart_start = chkQuickstartMode->isSelected();
	}
};

static QuickstartModeActionListener* quickstartModeActionListener;


void InitPanelQuickstart(const struct _ConfigCategory& category)
{
	auto posY = DISTANCE_BORDER;

	amigaModelActionListener = new AmigaModelActionListener();
	ntscButtonActionListener = new QSNTSCButtonActionListener();
	dfxCheckActionListener = new QSDFxCheckActionListener();
	dfxButtonActionListener = new QSDFxButtonActionListener();
	diskFileActionListener = new QSDiskFileActionListener();
	cdButtonActionListener = new QSCDButtonActionListener();
	cdFileActionListener = new QSCDFileActionListener();
	quickstartModeActionListener = new QuickstartModeActionListener();

	lblModel = new gcn::Label("Amiga model:");
	lblModel->setAlignment(gcn::Graphics::RIGHT);
	cboModel = new gcn::UaeDropDown(&amigaModelList);
	cboModel->setSize(160, cboModel->getHeight());
	cboModel->setBaseColor(gui_baseCol);
	cboModel->setBackgroundColor(colTextboxBackground);
	cboModel->setId("qscboAModel");
	cboModel->addActionListener(amigaModelActionListener);

	lblConfig = new gcn::Label("Config:");
	lblConfig->setAlignment(gcn::Graphics::RIGHT);
	cboConfig = new gcn::UaeDropDown(&amigaConfigList);
	cboConfig->setSize(category.panel->getWidth() - lblConfig->getWidth() - 8 - 2 * DISTANCE_BORDER,
	                   cboConfig->getHeight());
	cboConfig->setBaseColor(gui_baseCol);
	cboConfig->setBackgroundColor(colTextboxBackground);
	cboConfig->setId("qscboAConfig");
	cboConfig->addActionListener(amigaModelActionListener);

	chkNTSC = new gcn::UaeCheckBox("NTSC");
	chkNTSC->setId("qsNTSC");
	chkNTSC->addActionListener(ntscButtonActionListener);

	for (auto i = 0; i < 2; ++i)
	{
		char tmp[20];
		snprintf(tmp, 20, "DF%d:", i);
		chkDFx[i] = new gcn::UaeCheckBox(tmp);
		chkDFx[i]->addActionListener(dfxCheckActionListener);
		snprintf(tmp, 20, "qsDF%d", i);
		chkDFx[i]->setId(tmp);

		chkDFxWriteProtect[i] = new gcn::UaeCheckBox("Write-protected");
		chkDFxWriteProtect[i]->addActionListener(dfxCheckActionListener);
		snprintf(tmp, 20, "qsWP%d", i);
		chkDFxWriteProtect[i]->setId(tmp);

		cmdDFxInfo[i] = new gcn::Button("?");
		cmdDFxInfo[i]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		cmdDFxInfo[i]->setBaseColor(gui_baseCol);
		cmdDFxInfo[i]->addActionListener(dfxButtonActionListener);
		snprintf(tmp, 20, "qsInfo%d", i);
		cmdDFxInfo[i]->setId(tmp);

		cmdDFxEject[i] = new gcn::Button("Eject");
		cmdDFxEject[i]->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
		cmdDFxEject[i]->setBaseColor(gui_baseCol);
		snprintf(tmp, 20, "qscmdEject%d", i);
		cmdDFxEject[i]->setId(tmp);
		cmdDFxEject[i]->addActionListener(dfxButtonActionListener);

		cmdDFxSelect[i] = new gcn::Button("Select file");
		cmdDFxSelect[i]->setSize(BUTTON_WIDTH + 8, SMALL_BUTTON_HEIGHT);
		cmdDFxSelect[i]->setBaseColor(gui_baseCol);
		snprintf(tmp, 20, "qscmdSel%d", i);
		cmdDFxSelect[i]->setId(tmp);
		cmdDFxSelect[i]->addActionListener(dfxButtonActionListener);

		cboDFxFile[i] = new gcn::UaeDropDown(&diskfileList);
		cboDFxFile[i]->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboDFxFile[i]->getHeight());
		cboDFxFile[i]->setBaseColor(gui_baseCol);
		cboDFxFile[i]->setBackgroundColor(colTextboxBackground);
		snprintf(tmp, 20, "qscboDisk%d", i);
		cboDFxFile[i]->setId(tmp);
		cboDFxFile[i]->addActionListener(diskFileActionListener);
	}

	chkCD = new gcn::UaeCheckBox("CD drive");
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

	cboCDFile = new gcn::UaeDropDown(&cdfileList);
	cboCDFile->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboCDFile->getHeight());
	cboCDFile->setBaseColor(gui_baseCol);
	cboCDFile->setBackgroundColor(colTextboxBackground);
	cboCDFile->setId("qscboCD");
	cboCDFile->addActionListener(cdFileActionListener);

	chkQuickstartMode = new gcn::UaeCheckBox("Start in Quickstart mode");
	chkQuickstartMode->setId("qsMode");
	chkQuickstartMode->addActionListener(quickstartModeActionListener);

	cmdSetConfiguration = new gcn::Button("Set configuration");
	cmdSetConfiguration->setSize(BUTTON_WIDTH * 2, BUTTON_HEIGHT);
	cmdSetConfiguration->setBaseColor(gui_baseCol);
	cmdSetConfiguration->setId("cmdSetConfig");
	cmdSetConfiguration->addActionListener(amigaModelActionListener);

	category.panel->add(lblModel, DISTANCE_BORDER, posY);
	category.panel->add(cboModel, DISTANCE_BORDER + lblModel->getWidth() + 8, posY);
	category.panel->add(chkNTSC, cboModel->getX() + cboModel->getWidth() + 8, posY);
	posY += cboModel->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblConfig, DISTANCE_BORDER, posY);
	category.panel->add(cboConfig, DISTANCE_BORDER + lblConfig->getWidth() + 8, posY);
	posY += cboConfig->getHeight() + DISTANCE_NEXT_Y * 2;

	for (auto i = 0; i < 2; ++i)
	{
		auto posX = DISTANCE_BORDER;
		category.panel->add(chkDFx[i], posX, posY);
		posX += 180;
		category.panel->add(chkDFxWriteProtect[i], posX, posY);
		posX += chkDFxWriteProtect[i]->getWidth() + 4 * DISTANCE_NEXT_X;
		//	  category.panel->add(cmdDFxInfo[i], posX, posY);
		//posX += cmdDFxInfo[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cmdDFxEject[i], posX, posY);
		posX += cmdDFxEject[i]->getWidth() + DISTANCE_NEXT_X;
		category.panel->add(cmdDFxSelect[i], posX, posY);
		posY += cmdDFxEject[i]->getHeight() + 8;

		category.panel->add(cboDFxFile[i], DISTANCE_BORDER, posY);
		posY += cboDFxFile[i]->getHeight() + DISTANCE_NEXT_Y + 4;
	}

	category.panel->add(chkCD, chkDFx[1]->getX(), chkDFx[1]->getY());
	category.panel->add(cmdCDEject, cmdDFxEject[1]->getX(), cmdDFxEject[1]->getY());
	category.panel->add(cmdCDSelect, cmdDFxSelect[1]->getX(), cmdDFxSelect[1]->getY());
	category.panel->add(cboCDFile, cboDFxFile[1]->getX(), cboDFxFile[1]->getY());

	category.panel->add(chkQuickstartMode, category.panel->getWidth() - chkQuickstartMode->getWidth() - DISTANCE_BORDER,
	                    posY);
	posY += chkQuickstartMode->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cmdSetConfiguration, DISTANCE_BORDER, posY);

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
	if (!emulating)
		AdjustPrefs();

	RefreshPanelQuickstart();
}


void ExitPanelQuickstart(void)
{
	delete lblModel;
	delete cboModel;
	delete lblConfig;
	delete cboConfig;
	delete chkNTSC;
	for (auto i = 0; i < 2; ++i)
	{
		delete chkDFx[i];
		delete chkDFxWriteProtect[i];
		delete cmdDFxInfo[i];
		delete cmdDFxEject[i];
		delete cmdDFxSelect[i];
		delete cboDFxFile[i];
	}
	delete chkCD;
	delete cmdCDEject;
	delete cmdCDSelect;
	delete cboCDFile;
	delete chkQuickstartMode;
	delete cmdSetConfiguration;

	delete amigaModelActionListener;
	delete ntscButtonActionListener;
	delete dfxCheckActionListener;
	delete dfxButtonActionListener;
	delete diskFileActionListener;
	delete cdButtonActionListener;
	delete cdFileActionListener;
	delete quickstartModeActionListener;
}


static void AdjustDropDownControls(void)
{
	bIgnoreListChange = true;

	for (auto i = 0; i < 2; ++i)
	{
		cboDFxFile[i]->clearSelected();

		if (changed_prefs.floppyslots[i].dfxtype != DRV_NONE && strlen(changed_prefs.floppyslots[i].df) > 0)
		{
			for (unsigned int j = 0; j < lstMRUDiskList.size(); ++j)
			{
				if (strcmp(lstMRUDiskList[j].c_str(), changed_prefs.floppyslots[i].df) == 0)
				{
					cboDFxFile[i]->setSelected(j);
					break;
				}
			}
		}
	}

	cboCDFile->clearSelected();
	if (changed_prefs.cdslots[0].inuse && strlen(changed_prefs.cdslots[0].name) > 0)
	{
		for (unsigned int i = 0; i < lstMRUCDList.size(); ++i)
		{
			if (lstMRUCDList[i].c_str() != changed_prefs.cdslots[0].name)
			{
				cboCDFile->setSelected(i);
				break;
			}
		}
	}

	bIgnoreListChange = false;
}


void RefreshPanelQuickstart(void)
{
	auto prevAvailable = true;

	chkNTSC->setSelected(changed_prefs.ntscmode);

	AdjustDropDownControls();

	changed_prefs.nr_floppies = 0;
	for (auto i = 0; i < 4; ++i)
	{
		const auto driveEnabled = changed_prefs.floppyslots[i].dfxtype != DRV_NONE;
		if (i < 2)
		{
			chkDFx[i]->setSelected(driveEnabled);
			chkDFxWriteProtect[i]->setSelected(disk_getwriteprotect(&changed_prefs, changed_prefs.floppyslots[i].df));
			if (i == 0)
				chkDFx[i]->setEnabled(false);
			else
				chkDFx[i]->setEnabled(prevAvailable);

			cmdDFxInfo[i]->setEnabled(driveEnabled);
			chkDFxWriteProtect[i]->setEnabled(driveEnabled && !changed_prefs.floppy_read_only);
			cmdDFxEject[i]->setEnabled(driveEnabled);
			cmdDFxSelect[i]->setEnabled(driveEnabled);
			cboDFxFile[i]->setEnabled(driveEnabled);
		}
		prevAvailable = driveEnabled;
		if (driveEnabled)
			changed_prefs.nr_floppies = i + 1;
	}

	chkCD->setSelected(changed_prefs.cdslots[0].inuse);
	cmdCDEject->setEnabled(changed_prefs.cdslots[0].inuse);
	cmdCDSelect->setEnabled(changed_prefs.cdslots[0].inuse);
	cboCDFile->setEnabled(changed_prefs.cdslots[0].inuse);

	chkQuickstartMode->setSelected(quickstart_start);
}


bool HelpPanelQuickstart(vector<string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Simplified start of emulation by just selecting the Amiga model and the disk/CD");
	helptext.emplace_back("you want to use.");
	helptext.emplace_back("");
	helptext.emplace_back("After selecting the Amiga model, you can choose from a small list of standard");
	helptext.emplace_back("configurations for this model to start with.");
	helptext.emplace_back("");
	helptext.emplace_back("When you activate \"Start in Quickstart mode\", the next time you run the emulator,");
	helptext.emplace_back("it  will start with the QuickStart panel. Otherwise you start in configurations panel.");
	return true;
}
