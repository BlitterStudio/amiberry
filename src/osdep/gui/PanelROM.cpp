#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include "guisan/sdl/sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "rommgr.h"
#include "gui.h"
#include "gui_handling.h"

static gcn::Label* lblMainROM;
static gcn::UaeDropDown* cboMainROM;
static gcn::Button* cmdMainROM;
static gcn::Label* lblExtROM;
static gcn::UaeDropDown* cboExtROM;
static gcn::Button* cmdExtROM;

class ROMListModel : public gcn::ListModel
{
	vector<string> roms;
	vector<int> idxToAvailableROMs;
	int ROMType;

public:
	ROMListModel(int romtype)
	{
		ROMType = romtype;
	}

	int getNumberOfElements() override
	{
		return roms.size();
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= roms.size())
			return "---";
		return roms[i];
	}

	AvailableROM* getROMat(int i)
	{
		if (i >= 0 && i < idxToAvailableROMs.size())
			return idxToAvailableROMs[i] < 0 ? NULL : lstAvailableROMs[idxToAvailableROMs[i]];
		return nullptr;
	}

	int InitROMList(char* current)
	{
		roms.clear();
		idxToAvailableROMs.clear();

		if (ROMType & (ROMTYPE_EXTCDTV | ROMTYPE_EXTCD32))
		{
			roms.push_back("");
			idxToAvailableROMs.push_back(-1);
		}
		int currIdx = -1;
		for (int i = 0; i < lstAvailableROMs.size(); ++i)
		{
			if (lstAvailableROMs[i]->ROMType & ROMType)
			{
				if (!strcasecmp(lstAvailableROMs[i]->Path, current))
					currIdx = roms.size();
				roms.push_back(lstAvailableROMs[i]->Name);
				idxToAvailableROMs.push_back(i);
			}
		}
		return currIdx;
	}
};

static ROMListModel* mainROMList;
static ROMListModel* extROMList;


class MainROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		AvailableROM* rom = mainROMList->getROMat(cboMainROM->getSelected());
		if (rom != nullptr)
			strncpy(changed_prefs.romfile, rom->Path, sizeof(changed_prefs.romfile));
	}
};

static MainROMActionListener* mainROMActionListener;


class ExtROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		AvailableROM* rom = extROMList->getROMat(cboExtROM->getSelected());
		if (rom != nullptr)
			strncpy(changed_prefs.romextfile, rom->Path, sizeof(changed_prefs.romextfile));
		else
			strncpy(changed_prefs.romextfile, "", sizeof(changed_prefs.romextfile));
	}
};

static ExtROMActionListener* extROMActionListener;


class ROMButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char tmp[MAX_PATH];
		const char* filter[] = {".rom", "\0"};

		if (actionEvent.getSource() == cmdMainROM)
		{
			strncpy(tmp, currentDir, sizeof tmp);
			if (SelectFile("Select System ROM", tmp, filter))
			{
				AvailableROM* newrom;
				newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, sizeof newrom->Path);
				newrom->ROMType = ROMTYPE_KICK;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romfile, tmp, sizeof(changed_prefs.romfile));
				RefreshPanelROM();
			}
			cmdMainROM->requestFocus();
		}
		else if (actionEvent.getSource() == cmdExtROM)
		{
			strncpy(tmp, currentDir, sizeof tmp);
			if (SelectFile("Select Extended ROM", tmp, filter))
			{
				AvailableROM* newrom;
				newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, sizeof newrom->Path);
				newrom->ROMType = ROMTYPE_EXTCDTV;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romextfile, tmp, sizeof(changed_prefs.romextfile));
				RefreshPanelROM();
			}
			cmdExtROM->requestFocus();
		}
	}
};

static ROMButtonActionListener* romButtonActionListener;


void InitPanelROM(const struct _ConfigCategory& category)
{
	int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;

	mainROMActionListener = new MainROMActionListener();
	extROMActionListener = new ExtROMActionListener();
	romButtonActionListener = new ROMButtonActionListener();
	mainROMList = new ROMListModel(ROMTYPE_KICK | ROMTYPE_KICKCD32);
	extROMList = new ROMListModel(ROMTYPE_EXTCDTV | ROMTYPE_EXTCD32);

	lblMainROM = new gcn::Label("Main ROM File:");
	lblMainROM->setSize(120, LABEL_HEIGHT);
	cboMainROM = new gcn::UaeDropDown(mainROMList);
	cboMainROM->setSize(textFieldWidth, DROPDOWN_HEIGHT);
	cboMainROM->setBaseColor(gui_baseCol);
	cboMainROM->setBackgroundColor(colTextboxBackground);
	cboMainROM->setId("cboMainROM");
	cboMainROM->addActionListener(mainROMActionListener);
	cmdMainROM = new gcn::Button("...");
	cmdMainROM->setId("MainROM");
	cmdMainROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdMainROM->setBaseColor(gui_baseCol);
	cmdMainROM->addActionListener(romButtonActionListener);

	lblExtROM = new gcn::Label("Extended ROM File:");
	lblExtROM->setSize(120, LABEL_HEIGHT);
	cboExtROM = new gcn::UaeDropDown(extROMList);
	cboExtROM->setSize(textFieldWidth, DROPDOWN_HEIGHT);
	cboExtROM->setBaseColor(gui_baseCol);
	cboExtROM->setBackgroundColor(colTextboxBackground);
	cboExtROM->setId("cboExtROM");
	cboExtROM->addActionListener(extROMActionListener);
	cmdExtROM = new gcn::Button("...");
	cmdExtROM->setId("ExtROM");
	cmdExtROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdExtROM->setBaseColor(gui_baseCol);
	cmdExtROM->addActionListener(romButtonActionListener);

	int posY = DISTANCE_BORDER;
	category.panel->add(lblMainROM, DISTANCE_BORDER, posY);
	posY += lblMainROM->getHeight() + 4;
	category.panel->add(cboMainROM, DISTANCE_BORDER, posY);
	category.panel->add(cmdMainROM, DISTANCE_BORDER + cboMainROM->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboMainROM->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblExtROM, DISTANCE_BORDER, posY);
	posY += lblExtROM->getHeight() + 4;
	category.panel->add(cboExtROM, DISTANCE_BORDER, posY);
	category.panel->add(cmdExtROM, DISTANCE_BORDER + cboExtROM->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboExtROM->getHeight() + DISTANCE_NEXT_Y;

	RefreshPanelROM();
}


void ExitPanelROM()
{
	delete lblMainROM;
	delete cboMainROM;
	delete cmdMainROM;
	delete mainROMList;
	delete mainROMActionListener;

	delete lblExtROM;
	delete cboExtROM;
	delete cmdExtROM;
	delete extROMList;
	delete extROMActionListener;

	delete romButtonActionListener;
	//	delete chkMapROM;
}


void RefreshPanelROM()
{
	int idx = mainROMList->InitROMList(changed_prefs.romfile);
	cboMainROM->setSelected(idx);

	idx = extROMList->InitROMList(changed_prefs.romextfile);
	cboExtROM->setSelected(idx);
}
