#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#endif
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "autoconf.h"
#include "filesys.h"
#include "blkdev.h"
#include "gui_handling.h"

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
static const int COLUMN_SIZE[] =
{
	55, // Device
	65, // Volume
	220, // Path
	40, // R/W
	50, // Size
	40 // Bootpri
};

static const char* cdfile_filter[] = {".cue", ".ccd", ".iso", "\0"};
static void AdjustDropDownControls();

static gcn::Label* lblList[COL_COUNT];
static gcn::Container* listEntry[MAX_HD_DEVICES];
static gcn::TextField* listCells[MAX_HD_DEVICES][COL_COUNT];
static gcn::Button* listCmdProps[MAX_HD_DEVICES];
static gcn::ImageButton* listCmdDelete[MAX_HD_DEVICES];
static gcn::Button* cmdAddDirectory;
static gcn::Button* cmdAddHardfile;
static gcn::Button* cmdCreateHardfile;
static gcn::UaeCheckBox* chkHDReadOnly;
static gcn::UaeCheckBox* chkCD;
static gcn::UaeDropDown* cboCDFile;
static gcn::Button* cmdCDEject;
static gcn::Button* cmdCDSelect;
static gcn::Label* lblCDVol;
static gcn::Label* lblCDVolInfo;
static gcn::Slider* sldCDVol;


static int GetHDType(const int index)
{
	struct mountedinfo mi{};

	auto type = get_filesys_unitconfig(&changed_prefs, index, &mi);
	if (type < 0)
	{
		const auto uci = &changed_prefs.mountconfig[index];
		type = uci->ci.type == UAEDEV_DIR ? FILESYS_VIRTUAL : FILESYS_HARDFILE;
	}
	return type;
}


class CDfileListModel : public gcn::ListModel
{
public:
	CDfileListModel()
	= default;

	int getNumberOfElements() override
	{
		return lstMRUCDList.size();
	}

	string getElementAt(int i) override
	{
		if (i < 0 || i >= lstMRUCDList.size())
			return "---";
		return lstMRUCDList[i];
	}
};

static CDfileListModel cdfileList;


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
		for (auto i = 0; i < MAX_HD_DEVICES; ++i)
		{
			if (actionEvent.getSource() == listCmdProps[i])
			{
				if (GetHDType(i) == FILESYS_VIRTUAL)
				{
					if (EditFilesysVirtual(i))
						gui_force_rtarea_hdchange();
				}
				else
				{
					if (EditFilesysHardfile(i))
						gui_force_rtarea_hdchange();
				}
				listCmdProps[i]->requestFocus();
				break;
			}
		}
		RefreshPanelHD();
	}
};

static HDEditActionListener* hdEditActionListener;


class AddVirtualHDActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdAddDirectory)
		{
			if (EditFilesysVirtual(-1))
				gui_force_rtarea_hdchange();
			cmdAddDirectory->requestFocus();
			RefreshPanelHD();
		}
	}
};

AddVirtualHDActionListener* addVirtualHDActionListener;


class AddHardfileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdAddHardfile)
		{
			if (EditFilesysHardfile(-1))
				gui_force_rtarea_hdchange();
			cmdAddHardfile->requestFocus();
			RefreshPanelHD();
		}
	}
};

AddHardfileActionListener* addHardfileActionListener;


class CreateHardfileActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdCreateHardfile)
		{
			if (CreateFilesysHardfile())
				gui_force_rtarea_hdchange();
			cmdCreateHardfile->requestFocus();
			RefreshPanelHD();
		}
	}
};

CreateHardfileActionListener* createHardfileActionListener;


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
			}
			else
			{
				changed_prefs.cdslots[0].inuse = true;
				changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
			}
			RefreshPanelHD();
			RefreshPanelQuickstart();
		}
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
			strncpy(changed_prefs.cdslots[0].name, "", MAX_DPATH);
			AdjustDropDownControls();
		}
		else if (actionEvent.getSource() == cmdCDSelect)
		{
			char tmp[MAX_DPATH];

			if (strlen(changed_prefs.cdslots[0].name) > 0)
				strncpy(tmp, changed_prefs.cdslots[0].name, MAX_DPATH);
			else
				strcpy(tmp, currentDir);

			if (SelectFile("Select CD image file", tmp, cdfile_filter))
			{
				if(strncmp(changed_prefs.cdslots[0].name, tmp, MAX_DPATH) != 0)
				{
					strncpy(changed_prefs.cdslots[0].name, tmp, sizeof(changed_prefs.cdslots[0].name));
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

CDButtonActionListener* cdButtonActionListener;


class GenericActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == sldCDVol)
		{
			const auto newvol = 100 - int(sldCDVol->getValue());
			if (changed_prefs.sound_volume_cd != newvol)
			{
				changed_prefs.sound_volume_cd = newvol;
				RefreshPanelHD();
			}
		}
		else if (actionEvent.getSource() == chkHDReadOnly) {
			changed_prefs.harddrive_read_only = chkHDReadOnly->isSelected();
		}
	}
};

GenericActionListener* genericActionListener;


static bool bIgnoreListChange = false;

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
				strncpy(changed_prefs.cdslots[0].name, "", MAX_DPATH);
				AdjustDropDownControls();
			}
			else
			{
				if (cdfileList.getElementAt(idx) != changed_prefs.cdslots[0].name)
				{
					strncpy(changed_prefs.cdslots[0].name, cdfileList.getElementAt(idx).c_str(), sizeof changed_prefs.cdslots[0].name);
					changed_prefs.cdslots[0].inuse = true;
					changed_prefs.cdslots[0].type = SCSI_UNIT_IMAGE;
					lstMRUCDList.erase(lstMRUCDList.begin() + idx);
					lstMRUCDList.insert(lstMRUCDList.begin(), changed_prefs.cdslots[0].name);
					bIgnoreListChange = true;
					cboCDFile->setSelected(0);
					bIgnoreListChange = false;
				}
			}
		}
		RefreshPanelHD();
	}
};

static CDFileActionListener* cdFileActionListener;


void InitPanelHD(const struct _ConfigCategory& category)
{
	int row, col;
	auto posY = DISTANCE_BORDER;
	char tmp[20];

	hdRemoveActionListener = new HDRemoveActionListener();
	hdEditActionListener = new HDEditActionListener();
	addVirtualHDActionListener = new AddVirtualHDActionListener();
	addHardfileActionListener = new AddHardfileActionListener();
	createHardfileActionListener = new CreateHardfileActionListener();

	auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;

	for (col = 0; col < COL_COUNT; ++col)
		lblList[col] = new gcn::Label(column_caption[col]);

	for (row = 0; row < MAX_HD_DEVICES; ++row)
	{
		listEntry[row] = new gcn::Container();
		listEntry[row]->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, SMALL_BUTTON_HEIGHT + 4);
		listEntry[row]->setBaseColor(gui_baseCol);
#ifdef USE_SDL1
		listEntry[row]->setFrameSize(0);
#elif USE_SDL2
		listEntry[row]->setBorderSize(0);
#endif

		listCmdProps[row] = new gcn::Button("...");
		listCmdProps[row]->setBaseColor(gui_baseCol);
		listCmdProps[row]->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
		snprintf(tmp, 20, "cmdProp%d", row);
		listCmdProps[row]->setId(tmp);
		listCmdProps[row]->addActionListener(hdEditActionListener);

		listCmdDelete[row] = new gcn::ImageButton("data/delete.png");
		listCmdDelete[row]->setBaseColor(gui_baseCol);
		listCmdDelete[row]->setSize(SMALL_BUTTON_HEIGHT, SMALL_BUTTON_HEIGHT);
		snprintf(tmp, 20, "cmdDel%d", row);
		listCmdDelete[row]->setId(tmp);
		listCmdDelete[row]->addActionListener(hdRemoveActionListener);

		for (col = 0; col < COL_COUNT; ++col)
		{
			listCells[row][col] = new gcn::TextField();
			listCells[row][col]->setSize(COLUMN_SIZE[col], SMALL_BUTTON_HEIGHT);
			listCells[row][col]->setEnabled(false);
			listCells[row][col]->setBackgroundColor(colTextboxBackground);
		}
	}

	cmdAddDirectory = new gcn::Button("Add Directory");
	cmdAddDirectory->setBaseColor(gui_baseCol);
	cmdAddDirectory->setSize(cmdAddDirectory->getWidth(), BUTTON_HEIGHT);
	cmdAddDirectory->setId("cmdAddDir");
	cmdAddDirectory->addActionListener(addVirtualHDActionListener);

	cmdAddHardfile = new gcn::Button("Add Hardfile");
	cmdAddHardfile->setBaseColor(gui_baseCol);
	cmdAddHardfile->setSize(cmdAddHardfile->getWidth(), BUTTON_HEIGHT);
	cmdAddHardfile->setId("cmdAddHDF");
	cmdAddHardfile->addActionListener(addHardfileActionListener);

	cmdCreateHardfile = new gcn::Button("Create Hardfile");
	cmdCreateHardfile->setBaseColor(gui_baseCol);
	cmdCreateHardfile->setSize(cmdCreateHardfile->getWidth(), BUTTON_HEIGHT);
	cmdCreateHardfile->setId("cmdCreateHDF");
	cmdCreateHardfile->addActionListener(createHardfileActionListener);

	cdCheckActionListener = new CDCheckActionListener();
	cdButtonActionListener = new CDButtonActionListener();
	cdFileActionListener = new CDFileActionListener();
	genericActionListener = new GenericActionListener();

	chkHDReadOnly = new gcn::UaeCheckBox("Master harddrive write protection");
	chkHDReadOnly->setId("chkHDRO");
	chkHDReadOnly->addActionListener(genericActionListener);

	chkCD = new gcn::UaeCheckBox("CD drive");
	chkCD->addActionListener(cdCheckActionListener);

	cmdCDEject = new gcn::Button("Eject");
	cmdCDEject->setSize(SMALL_BUTTON_WIDTH * 2, SMALL_BUTTON_HEIGHT);
	cmdCDEject->setBaseColor(gui_baseCol);
	cmdCDEject->setId("cdEject");
	cmdCDEject->addActionListener(cdButtonActionListener);

	cmdCDSelect = new gcn::Button("...");
	cmdCDSelect->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdCDSelect->setBaseColor(gui_baseCol);
	cmdCDSelect->setId("CDSelect");
	cmdCDSelect->addActionListener(cdButtonActionListener);

	cboCDFile = new gcn::UaeDropDown(&cdfileList);
	cboCDFile->setSize(category.panel->getWidth() - 2 * DISTANCE_BORDER, cboCDFile->getHeight());
	cboCDFile->setBaseColor(gui_baseCol);
	cboCDFile->setBackgroundColor(colTextboxBackground);
	cboCDFile->setId("cboCD");
	cboCDFile->addActionListener(cdFileActionListener);

	lblCDVol = new gcn::Label("CD Volume:");
	lblCDVol->setAlignment(gcn::Graphics::RIGHT);
	sldCDVol = new gcn::Slider(0, 100);
	sldCDVol->setSize(200, SLIDER_HEIGHT);
	sldCDVol->setBaseColor(gui_baseCol);
	sldCDVol->setMarkerLength(20);
	sldCDVol->setStepLength(10);
	sldCDVol->setId("CDVol");
	sldCDVol->addActionListener(genericActionListener);
	lblCDVolInfo = new gcn::Label("80 %");

	int posX = DISTANCE_BORDER + 2 + SMALL_BUTTON_WIDTH + 34;
	for (col = 0; col < COL_COUNT; ++col)
	{
		category.panel->add(lblList[col], posX, posY);
		posX += COLUMN_SIZE[col];
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
			posX += COLUMN_SIZE[col];
		}
		category.panel->add(listEntry[row], DISTANCE_BORDER, posY);
		posY += listEntry[row]->getHeight() + DISTANCE_NEXT_Y/2;
	}

	posY += DISTANCE_NEXT_Y;
	category.panel->add(cmdAddDirectory, DISTANCE_BORDER, posY);
	category.panel->add(cmdAddHardfile, DISTANCE_BORDER + cmdAddDirectory->getWidth() + DISTANCE_NEXT_X, posY);
	category.panel->add(cmdCreateHardfile, cmdAddHardfile->getX() + cmdAddHardfile->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cmdAddDirectory->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkHDReadOnly, DISTANCE_BORDER, posY);
	posY += chkHDReadOnly->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(chkCD, DISTANCE_BORDER, posY + 2);
	category.panel->add(cmdCDEject, category.panel->getWidth() - cmdCDEject->getWidth() - DISTANCE_NEXT_X - cmdCDSelect->getWidth() - DISTANCE_BORDER, posY);
	category.panel->add(cmdCDSelect, category.panel->getWidth() - cmdCDSelect->getWidth() - DISTANCE_BORDER, posY);
	posY += cmdCDSelect->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(cboCDFile, DISTANCE_BORDER, posY);
	posY += cboCDFile->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblCDVol, DISTANCE_BORDER, posY);
	category.panel->add(sldCDVol, DISTANCE_BORDER + lblCDVol->getWidth() + 8, posY);
	category.panel->add(lblCDVolInfo, sldCDVol->getX() + sldCDVol->getWidth() + 12, posY);

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
	delete cmdCreateHardfile;
	delete chkHDReadOnly;

	delete chkCD;
	delete cmdCDEject;
	delete cmdCDSelect;
	delete cboCDFile;
	delete lblCDVol;
	delete lblCDVolInfo;
	delete sldCDVol;

	delete cdCheckActionListener;
	delete cdButtonActionListener;
	delete cdFileActionListener;
	delete genericActionListener;

	delete hdRemoveActionListener;
	delete hdEditActionListener;
	delete addVirtualHDActionListener;
	delete addHardfileActionListener;
	delete createHardfileActionListener;
}


static void AdjustDropDownControls()
{
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
}

void RefreshPanelHD()
{
	char tmp[32];
	struct mountedinfo mi{};
	auto nosize = 0;

	AdjustDropDownControls();

	for (auto row = 0; row < MAX_HD_DEVICES; ++row)
	{
		if (row < changed_prefs.mountitems)
		{
			auto uci = &changed_prefs.mountconfig[row];
			const auto ci = &uci->ci;
			auto type = get_filesys_unitconfig(&changed_prefs, row, &mi);
			if (type < 0)
			{
				type = uci->ci.type == UAEDEV_DIR ? FILESYS_VIRTUAL : FILESYS_HARDFILE;
				nosize = 1;
			}
			if (mi.size < 0)
				nosize = 1;

			if (type == FILESYS_VIRTUAL)
			{
				listCells[row][COL_DEVICE]->setText(ci->devname);
				listCells[row][COL_VOLUME]->setText(ci->volname);
				listCells[row][COL_PATH]->setText(ci->rootdir);
				if (ci->readonly)
					listCells[row][COL_READWRITE]->setText("no");
				else
					listCells[row][COL_READWRITE]->setText("yes");
				listCells[row][COL_SIZE]->setText("n/a");
				snprintf(tmp, 32, "%d", ci->bootpri);
				listCells[row][COL_BOOTPRI]->setText(tmp);
			}
			else
			{
				listCells[row][COL_DEVICE]->setText(ci->devname);
				listCells[row][COL_VOLUME]->setText("n/a");
				listCells[row][COL_PATH]->setText(ci->rootdir);
				if (ci->readonly)
					listCells[row][COL_READWRITE]->setText("no");
				else
					listCells[row][COL_READWRITE]->setText("yes");
				if (nosize)
					snprintf(tmp, 32, "n/a");
				else if (mi.size >= 1024 * 1024 * 1024)
					snprintf(tmp, 32, "%.1fG", double(uae_u32(mi.size / (1024 * 1024))) / 1024.0);
				else
					snprintf(tmp, 32, "%.1fM", double(uae_u32(mi.size / 1024)) / 1024.0);
				listCells[row][COL_SIZE]->setText(tmp);
				snprintf(tmp, 32, "%d", ci->bootpri);
				listCells[row][COL_BOOTPRI]->setText(tmp);
			}
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

	chkHDReadOnly->setSelected(changed_prefs.harddrive_read_only);

	chkCD->setSelected(changed_prefs.cdslots[0].inuse);
	cmdCDEject->setEnabled(changed_prefs.cdslots[0].inuse);
	cmdCDSelect->setEnabled(changed_prefs.cdslots[0].inuse);
	cboCDFile->setEnabled(changed_prefs.cdslots[0].inuse);
	sldCDVol->setEnabled(changed_prefs.cdslots[0].inuse);

	sldCDVol->setValue(100 - changed_prefs.sound_volume_cd);
	snprintf(tmp, 32, "%d %%", 100 - changed_prefs.sound_volume_cd);
	lblCDVolInfo->setCaption(tmp);
}


int count_HDs(struct uae_prefs* p)
{
	return p->mountitems;
}

bool HelpPanelHD(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.emplace_back(R"(Use "Add Directory" to add a folder or "Add Hardfile" to add a HDF file as)");
	helptext.emplace_back("a hard disk. To edit the settings of a HDD, click on \"...\" left to the entry in");
	helptext.emplace_back("the list. With the red cross, you can delete an entry.");
	helptext.emplace_back("");
	helptext.emplace_back("With \"Create Hardfile\", you can create a new formatted HDF file up to 2 GB.");
	helptext.emplace_back("For large files, it will take some time to create the new hard disk. You have to");
	helptext.emplace_back("format the new HDD in the Amiga via the Workbench.");
	helptext.emplace_back("");
	helptext.emplace_back("If \"Master harddrive write protection\" is activated, you can't write to any HD.");
	helptext.emplace_back("");
	helptext.emplace_back(R"(Activate "CD drive" to emulate CD for CD32. Use "Eject" to remove current CD)");
	helptext.emplace_back("and click on \"...\" to open a dialog to select the iso/cue file for CD emulation.");
	helptext.emplace_back("");
	helptext.emplace_back("In current version, WAV, MP3 and FLAC is supported for audio tracks.");
	return true;
}
