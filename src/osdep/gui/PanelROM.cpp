#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
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
#include "rommgr.h"
#include "uae.h"
#include "gui.h"
#include "gui_handling.h"

static gcn::Label* lblMainROM;
static gcn::UaeDropDown* cboMainROM;
static gcn::Button* cmdMainROM;
static gcn::Label* lblExtROM;
static gcn::UaeDropDown* cboExtROM;
static gcn::Button* cmdExtROM;
#ifdef ACTION_REPLAY
static gcn::Label *lblCartROM;
static gcn::UaeDropDown* cboCartROM;
static gcn::Button *cmdCartROM;
#endif

class ROMListModel : public gcn::ListModel
{
	vector<string> roms;
	vector<int> idxToAvailableROMs;
	int ROMType;

public:
	ROMListModel(const int romtype)
	{
		ROMType = romtype;
	}

	int getNumberOfElements() override
	{
		return roms.size();
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= roms.size())
			return "---";
		return roms[i];
	}

	AvailableROM* getROMat(const int i)
	{
		if (i >= 0 && i < idxToAvailableROMs.size())
			return idxToAvailableROMs[i] < 0 ? NULL : lstAvailableROMs[idxToAvailableROMs[i]];
		return nullptr;
	}

	int InitROMList(char* current)
	{
		roms.clear();
		idxToAvailableROMs.clear();

		int currIdx = -1;
		if (ROMType & (ROMTYPE_ALL_EXT | ROMTYPE_ALL_CART))
		{
			roms.push_back("");
			idxToAvailableROMs.push_back(-1);
			currIdx = 0;
		}
		
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
#ifdef ACTION_REPLAY
static ROMListModel *cartROMList;
#endif

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

#ifdef ACTION_REPLAY
class CartROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		AvailableROM* rom = cartROMList->getROMat(cboCartROM->getSelected());
		if (rom != nullptr)
			strncpy(changed_prefs.cartfile, rom->Path, sizeof(changed_prefs.cartfile));
		else
			strncpy(changed_prefs.cartfile, "", sizeof(changed_prefs.cartfile));
	}
};
static CartROMActionListener* cartROMActionListener;
#endif

class ROMButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char tmp[MAX_DPATH];
		const char* filter[] = {".rom", "\0"};

		if (actionEvent.getSource() == cmdMainROM)
		{
			strncpy(tmp, currentDir, MAX_DPATH);
			if (SelectFile("Select System ROM", tmp, filter))
			{
				AvailableROM * newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH);
				newrom->ROMType = ROMTYPE_KICK;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romfile, tmp, sizeof(changed_prefs.romfile));
				RefreshPanelROM();
			}
			cmdMainROM->requestFocus();
		}
		else if (actionEvent.getSource() == cmdExtROM)
		{
			strncpy(tmp, currentDir, MAX_DPATH);
			if (SelectFile("Select Extended ROM", tmp, filter))
			{
				AvailableROM * newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH);
				newrom->ROMType = ROMTYPE_EXTCDTV;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romextfile, tmp, sizeof(changed_prefs.romextfile));
				RefreshPanelROM();
			}
			cmdExtROM->requestFocus();
		}
#ifdef ACTION_REPLAY
		else if (actionEvent.getSource() == cmdCartROM)
		{
			strncpy(tmp, currentDir, MAX_DPATH);
			if (SelectFile("Select Cartridge ROM", tmp, filter))
			{
				AvailableROM *newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH);
				newrom->ROMType = ROMTYPE_CD32CART;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romextfile, tmp, sizeof(changed_prefs.romextfile));
				RefreshPanelROM();
			}
			cmdCartROM->requestFocus();
		}
#endif
	}
};

static ROMButtonActionListener* romButtonActionListener;


void InitPanelROM(const struct _ConfigCategory& category)
{
	const int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;

	mainROMActionListener = new MainROMActionListener();
	extROMActionListener = new ExtROMActionListener();
#ifdef ACTION_REPLAY
	cartROMActionListener = new CartROMActionListener();
#endif
	romButtonActionListener = new ROMButtonActionListener();
	mainROMList = new ROMListModel(ROMTYPE_ALL_KICK);
	extROMList = new ROMListModel(ROMTYPE_ALL_EXT);
#ifdef ACTION_REPLAY
	cartROMList = new ROMListModel(ROMTYPE_ALL_CART);
#endif

	lblMainROM = new gcn::Label("Main ROM File:");
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

#ifdef ACTION_REPLAY
	lblCartROM = new gcn::Label("Cartridge ROM File:");
	cboCartROM = new gcn::UaeDropDown(cartROMList);
	cboCartROM->setSize(textFieldWidth, DROPDOWN_HEIGHT);
	cboCartROM->setBaseColor(gui_baseCol);
	cboCartROM->setBackgroundColor(colTextboxBackground);
	cboCartROM->setId("cboCartROM");
	cboCartROM->addActionListener(cartROMActionListener);
	cmdCartROM = new gcn::Button("...");
	cmdCartROM->setId("CartROM");
	cmdCartROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdCartROM->setBaseColor(gui_baseCol);
	cmdCartROM->addActionListener(romButtonActionListener);
#endif

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

#ifdef ACTION_REPLAY
	category.panel->add(lblCartROM, DISTANCE_BORDER, posY);
	posY += lblCartROM->getHeight() + 4;
	category.panel->add(cboCartROM, DISTANCE_BORDER, posY);
	category.panel->add(cmdCartROM, DISTANCE_BORDER + cboCartROM->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboCartROM->getHeight() + DISTANCE_NEXT_Y;
#endif

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

#ifdef ACTION_REPLAY
	delete lblCartROM;
	delete cboCartROM;
	delete cmdCartROM;
	delete cartROMList;
	delete cartROMActionListener;
#endif

	delete romButtonActionListener;
}


void RefreshPanelROM()
{
	int idx = mainROMList->InitROMList(changed_prefs.romfile);
	cboMainROM->setSelected(idx);

	idx = extROMList->InitROMList(changed_prefs.romextfile);
	cboExtROM->setSelected(idx);

#ifdef ACTION_REPLAY
	idx = cartROMList->InitROMList(changed_prefs.cartfile);
	cboCartROM->setSelected(idx);
#endif
}

bool HelpPanelROM(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.push_back("Select the required kickstart ROM for the Amiga you want to emulate in \"Main ROM File\".");
	helptext.push_back("");
	helptext.push_back("In \"Extended ROM File\", you can only select the required ROM for CD32 emulation.");
	helptext.push_back("");
	helptext.push_back("In \"Cartridge ROM File\", you can select the CD32 FMV module to activate video playback in CD32.");
	helptext.push_back("There are also some Action Replay and Freezer cards and the built in HRTMon available.");
	return true;
}
