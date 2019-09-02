#include <strings.h>
#include <string.h>

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
#include "UaeDropDown.hpp"

#include "sysdeps.h"
#include "options.h"
#include "rommgr.h"
#include "gui_handling.h"

static gcn::Label* lblMainROM;
static gcn::UaeDropDown* cboMainROM;
static gcn::Button* cmdMainROM;
static gcn::Label* lblExtROM;
static gcn::UaeDropDown* cboExtROM;
static gcn::Button* cmdExtROM;
static gcn::Label *lblCartROM;
static gcn::UaeDropDown* cboCartROM;
static gcn::Button *cmdCartROM;

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

		auto currIdx = -1;
		if (ROMType & (ROMTYPE_ALL_EXT | ROMTYPE_ALL_CART))
		{
			roms.push_back(" ");
			idxToAvailableROMs.push_back(-1);
			currIdx = 0;
		}
		
		for (auto i = 0; i < lstAvailableROMs.size(); ++i)
		{
			if (lstAvailableROMs[i]->ROMType & ROMType)
			{
				if (!stricmp(lstAvailableROMs[i]->Path, current))
					currIdx = roms.size();
				roms.emplace_back(lstAvailableROMs[i]->Name);
				idxToAvailableROMs.push_back(i);
			}
		}
		return currIdx;
	}
};

static ROMListModel* mainROMList;
static ROMListModel* extROMList;
static ROMListModel *cartROMList;

class MainROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto rom = mainROMList->getROMat(cboMainROM->getSelected());
		if (rom != nullptr)
			strncpy(workprefs.romfile, rom->Path, sizeof(workprefs.romfile));
	}
};

static MainROMActionListener* mainROMActionListener;


class ExtROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto rom = extROMList->getROMat(cboExtROM->getSelected());
		if (rom != nullptr)
			strncpy(workprefs.romextfile, rom->Path, sizeof(workprefs.romextfile));
		else
			strncpy(workprefs.romextfile, " ", sizeof(workprefs.romextfile));
	}
};

static ExtROMActionListener* extROMActionListener;

class CartROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		const auto rom = cartROMList->getROMat(cboCartROM->getSelected());
		if (rom != nullptr)
			strncpy(workprefs.cartfile, rom->Path, sizeof workprefs.cartfile);
		else
			strncpy(workprefs.cartfile, "", sizeof workprefs.cartfile);
	}
};
static CartROMActionListener* cartROMActionListener;

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
				const auto newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH);
				newrom->ROMType = ROMTYPE_KICK;
				lstAvailableROMs.push_back(newrom);
				strncpy(workprefs.romfile, tmp, sizeof(workprefs.romfile));
				RefreshPanelROM();
			}
			cmdMainROM->requestFocus();
		}
		else if (actionEvent.getSource() == cmdExtROM)
		{
			strncpy(tmp, currentDir, MAX_DPATH);
			if (SelectFile("Select Extended ROM", tmp, filter))
			{
				const auto newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH);
				newrom->ROMType = ROMTYPE_EXTCDTV;
				lstAvailableROMs.push_back(newrom);
				strncpy(workprefs.romextfile, tmp, sizeof(workprefs.romextfile));
				RefreshPanelROM();
			}
			cmdExtROM->requestFocus();
		}
		else if (actionEvent.getSource() == cmdCartROM)
		{
			strncpy(tmp, currentDir, MAX_DPATH);
			if (SelectFile("Select Cartridge ROM", tmp, filter))
			{
				const auto newrom = new AvailableROM();
				extractFileName(tmp, newrom->Name);
				removeFileExtension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH);
				newrom->ROMType = ROMTYPE_CD32CART;
				lstAvailableROMs.push_back(newrom);
				strncpy(workprefs.romextfile, tmp, sizeof(workprefs.romextfile));
				RefreshPanelROM();
			}
			cmdCartROM->requestFocus();
		}
	}
};

static ROMButtonActionListener* romButtonActionListener;


void InitPanelROM(const struct _ConfigCategory& category)
{
	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;

	mainROMActionListener = new MainROMActionListener();
	extROMActionListener = new ExtROMActionListener();
	cartROMActionListener = new CartROMActionListener();
	romButtonActionListener = new ROMButtonActionListener();
	mainROMList = new ROMListModel(ROMTYPE_ALL_KICK);
	extROMList = new ROMListModel(ROMTYPE_ALL_EXT);
	cartROMList = new ROMListModel(ROMTYPE_ALL_CART);

	lblMainROM = new gcn::Label("Main ROM File:");
	cboMainROM = new gcn::UaeDropDown(mainROMList);
	cboMainROM->setSize(textFieldWidth, cboMainROM->getHeight());
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
	cboExtROM->setSize(textFieldWidth, cboExtROM->getHeight());
	cboExtROM->setBaseColor(gui_baseCol);
	cboExtROM->setBackgroundColor(colTextboxBackground);
	cboExtROM->setId("cboExtROM");
	cboExtROM->addActionListener(extROMActionListener);
	cmdExtROM = new gcn::Button("...");
	cmdExtROM->setId("ExtROM");
	cmdExtROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdExtROM->setBaseColor(gui_baseCol);
	cmdExtROM->addActionListener(romButtonActionListener);

	lblCartROM = new gcn::Label("Cartridge ROM File:");
	cboCartROM = new gcn::UaeDropDown(cartROMList);
	cboCartROM->setSize(textFieldWidth, cboCartROM->getHeight());
	cboCartROM->setBaseColor(gui_baseCol);
	cboCartROM->setBackgroundColor(colTextboxBackground);
	cboCartROM->setId("cboCartROM");
	cboCartROM->addActionListener(cartROMActionListener);
	cmdCartROM = new gcn::Button("...");
	cmdCartROM->setId("CartROM");
	cmdCartROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdCartROM->setBaseColor(gui_baseCol);
	cmdCartROM->addActionListener(romButtonActionListener);

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

	category.panel->add(lblCartROM, DISTANCE_BORDER, posY);
	posY += lblCartROM->getHeight() + 4;
	category.panel->add(cboCartROM, DISTANCE_BORDER, posY);
	category.panel->add(cmdCartROM, DISTANCE_BORDER + cboCartROM->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboCartROM->getHeight() + DISTANCE_NEXT_Y;

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

	delete lblCartROM;
	delete cboCartROM;
	delete cmdCartROM;
	delete cartROMList;
	delete cartROMActionListener;

	delete romButtonActionListener;
}


void RefreshPanelROM()
{
	auto idx = mainROMList->InitROMList(workprefs.romfile);
	cboMainROM->setSelected(idx);

	idx = extROMList->InitROMList(workprefs.romextfile);
	cboExtROM->setSelected(idx);

	idx = cartROMList->InitROMList(workprefs.cartfile);
	cboCartROM->setSelected(idx);
}

bool HelpPanelROM(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required kickstart ROM for the Amiga you want to emulate in \"Main ROM File\".");
	helptext.emplace_back(" ");
	helptext.emplace_back("In \"Extended ROM File\", you can only select the required ROM for CD32 emulation.");
	helptext.emplace_back(" ");
	helptext.emplace_back("In \"Cartridge ROM File\", you can select the CD32 FMV module to activate video");
	helptext.emplace_back("playback in CD32. There are also some Action Replay and Freezer cards and the built-in");
	helptext.emplace_back("HRTMon available.");
	return true;
}
