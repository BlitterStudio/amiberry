#include <cstring>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "rommgr.h"
#include "gui_handling.h"

static gcn::Label* lblMainROM;
static gcn::DropDown* cboMainROM;
static gcn::Button* cmdMainROM;
static gcn::Label* lblExtROM;
static gcn::DropDown* cboExtROM;
static gcn::Button* cmdExtROM;
static gcn::Label* lblCartROM;
static gcn::DropDown* cboCartROM;
static gcn::Button* cmdCartROM;
static gcn::Label* lblUAEROM;
static gcn::DropDown* cboUAEROM;
static gcn::CheckBox* chkShapeShifter;

class ROMListModel : public gcn::ListModel
{
	std::vector<std::string> roms;
	std::vector<int> idxToAvailableROMs;
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
		if (i < 0 || i >= static_cast<int>(roms.size()))
			return "---";
		return roms[i];
	}

	AvailableROM* get_rom_at(const int i)
	{
		if (i >= 0 && i < static_cast<int>(idxToAvailableROMs.size()))
			return idxToAvailableROMs[i] < 0 ? nullptr : lstAvailableROMs[idxToAvailableROMs[i]];
		return nullptr;
	}

	int init_rom_list(char* current)
	{
		roms.clear();
		idxToAvailableROMs.clear();

		auto currIdx = -1;
		if (ROMType & (ROMTYPE_ALL_EXT | ROMTYPE_ALL_CART))
		{
			roms.emplace_back(" ");
			idxToAvailableROMs.push_back(-1);
			currIdx = 0;
		}

		for (auto i = 0; i < static_cast<int>(lstAvailableROMs.size()); ++i)
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
static ROMListModel* cartROMList;

class StringListModel : public gcn::ListModel
{
private:
	std::vector<std::string> values;
public:
	StringListModel(const char* entries[], const int count)
	{
		for (auto i = 0; i < count; ++i)
			values.emplace_back(entries[i]);
	}

	int getNumberOfElements() override
	{
		return values.size();
	}

	int add_element(const char* Elem)
	{
		values.emplace_back(Elem);
		return 0;
	}

	std::string getElementAt(const int i) override
	{
		if (i < 0 || i >= static_cast<int>(values.size()))
			return "---";
		return values[i];
	}
};

const char* uaeValues[] = { "ROM disabled", "Original UAE (FS + F0 ROM)", "New UAE (64k + F0 ROM)" };
StringListModel uaeList(uaeValues, 3);

class MainROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		auto* const rom = mainROMList->get_rom_at(cboMainROM->getSelected());
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
		auto* const rom = extROMList->get_rom_at(cboExtROM->getSelected());
		if (rom != nullptr)
			strncpy(changed_prefs.romextfile, rom->Path, sizeof(changed_prefs.romextfile));
		else
			strncpy(changed_prefs.romextfile, " ", sizeof(changed_prefs.romextfile));
	}
};

static ExtROMActionListener* extROMActionListener;

class CartROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		auto* const rom = cartROMList->get_rom_at(cboCartROM->getSelected());
		if (rom != nullptr)
			strncpy(changed_prefs.cartfile, rom->Path, sizeof changed_prefs.cartfile);
		else
			strncpy(changed_prefs.cartfile, "", sizeof changed_prefs.cartfile);
	}
};

static CartROMActionListener* cartROMActionListener;

class ROMButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char tmp[MAX_DPATH];
		const char* filter[] = {".rom", ".bin", "\0"};

		if (actionEvent.getSource() == cmdMainROM)
		{
			strncpy(tmp, current_dir, MAX_DPATH - 1);
			if (SelectFile("Select System ROM", tmp, filter))
			{
				auto* const newrom = new AvailableROM();
				extract_filename(tmp, newrom->Name);
				remove_file_extension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH - 1);
				newrom->ROMType = ROMTYPE_KICK;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romfile, tmp, sizeof changed_prefs.romfile);
				RefreshPanelROM();
			}
			cmdMainROM->requestFocus();
		}
		else if (actionEvent.getSource() == cmdExtROM)
		{
			strncpy(tmp, current_dir, MAX_DPATH - 1);
			if (SelectFile("Select Extended ROM", tmp, filter))
			{
				auto* const newrom = new AvailableROM();
				extract_filename(tmp, newrom->Name);
				remove_file_extension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH - 1);
				newrom->ROMType = ROMTYPE_EXTCDTV;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romextfile, tmp, sizeof changed_prefs.romextfile);
				RefreshPanelROM();
			}
			cmdExtROM->requestFocus();
		}
		else if (actionEvent.getSource() == cmdCartROM)
		{
			strncpy(tmp, current_dir, MAX_DPATH - 1);
			if (SelectFile("Select Cartridge ROM", tmp, filter))
			{
				auto* const newrom = new AvailableROM();
				extract_filename(tmp, newrom->Name);
				remove_file_extension(newrom->Name);
				strncpy(newrom->Path, tmp, MAX_DPATH - 1);
				newrom->ROMType = ROMTYPE_CD32CART;
				lstAvailableROMs.push_back(newrom);
				strncpy(changed_prefs.romextfile, tmp, sizeof changed_prefs.romextfile);
				RefreshPanelROM();
			}
			cmdCartROM->requestFocus();
		}
		else if (actionEvent.getSource() == cboUAEROM) {
			const auto v = cboUAEROM->getSelected();
			if (v > 0) {
				changed_prefs.uaeboard = v - 1;
				changed_prefs.boot_rom = 0;
			}
			else {
				changed_prefs.uaeboard = 0;
				changed_prefs.boot_rom = 1; // disabled
			}
		}
		else if (actionEvent.getSource() == chkShapeShifter)
		{
			changed_prefs.kickshifter = chkShapeShifter->isSelected();
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
	cboMainROM = new gcn::DropDown(mainROMList);
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
	cboExtROM = new gcn::DropDown(extROMList);
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
	cboCartROM = new gcn::DropDown(cartROMList);
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

	lblUAEROM = new gcn::Label("Advanced UAE expansion board/Boot ROM:");
	cboUAEROM = new gcn::DropDown(&uaeList);
	cboUAEROM->setSize(textFieldWidth, cboUAEROM->getHeight());
	cboUAEROM->setBaseColor(gui_baseCol);
	cboUAEROM->setBackgroundColor(colTextboxBackground);
	cboUAEROM->setId("cboUAEROM");
	cboUAEROM->addActionListener(romButtonActionListener);

	chkShapeShifter = new gcn::CheckBox("ShapeShifter support");
	chkShapeShifter->setId("chkShapeShifter");
	chkShapeShifter->addActionListener(romButtonActionListener);

	auto posY = DISTANCE_BORDER;
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

	category.panel->add(lblUAEROM, DISTANCE_BORDER, posY);
	posY += lblUAEROM->getHeight() + 4;
	category.panel->add(cboUAEROM, DISTANCE_BORDER, posY);
	posY += cboUAEROM->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(chkShapeShifter, DISTANCE_BORDER, posY);

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

	delete lblUAEROM;
	delete cboUAEROM;
	delete chkShapeShifter;
	delete romButtonActionListener;
}

void RefreshPanelROM()
{
	auto idx = mainROMList->init_rom_list(changed_prefs.romfile);
	if (idx != -1)
		cboMainROM->setSelected(idx);
	else
	{
		// ROM file not in the list of known ROMs -- let's add it
		auto* newrom = new AvailableROM();
		extract_filename(changed_prefs.romfile, newrom->Name);
		remove_file_extension(newrom->Name);
		strncpy(newrom->Path, changed_prefs.romfile, MAX_DPATH - 1);
		newrom->ROMType = ROMTYPE_KICK;
		lstAvailableROMs.push_back(newrom);
		RefreshPanelROM();
	}

	idx = extROMList->init_rom_list(changed_prefs.romextfile);
	// ROM was found in the list
	if (idx != 0 && strlen(changed_prefs.romextfile) == 0)
		cboExtROM->setSelected(idx);
	// A ROM filename was specified, but it wasn't found in the list - let's add it
	else if (idx == 0 && strlen(changed_prefs.romextfile) > 0)
	{
		// ROM file not in the list of known ROMs
		auto* newrom = new AvailableROM();
		extract_filename(changed_prefs.romextfile, newrom->Name);
		remove_file_extension(newrom->Name);
		strncpy(newrom->Path, changed_prefs.romextfile, MAX_DPATH - 1);
		newrom->ROMType = ROMTYPE_EXTCDTV;
		lstAvailableROMs.push_back(newrom);
		RefreshPanelROM();
	}
	// No ROM specified
	else
	{
		cboExtROM->setSelected(idx);
	}

	idx = cartROMList->init_rom_list(changed_prefs.cartfile);
	cboCartROM->setSelected(idx);

	if (changed_prefs.boot_rom == 1) {
		cboUAEROM->setSelected(0);
	}
	else {
		cboUAEROM->setSelected(changed_prefs.uaeboard + 1);
	}
	cboUAEROM->setEnabled(!emulating);

	chkShapeShifter->setSelected(changed_prefs.kickshifter);
}

bool HelpPanelROM(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Select the required Kickstart ROM for the Amiga you want to emulate in \"Main ROM File\".");
	helptext.emplace_back(" ");
	helptext.emplace_back("In \"Extended ROM File\", you can only select the required ROM for CD32 emulation.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can use the ShapeShifter support checkbox to patch the system ROM for ShapeShifter");
	helptext.emplace_back("compatibility. You do not need to run PrepareEmul on startup with this enabled.");
	helptext.emplace_back(" ");
	helptext.emplace_back("In \"Cartridge ROM File\", you can select the CD32 FMV module to activate video");
	helptext.emplace_back("playback in CD32. There are also some Action Replay and Freezer cards and the built-in");
	helptext.emplace_back("HRTMon available.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The Advanced UAE Expansion/Boot ROM option allows you to set the following:");
	helptext.emplace_back("Rom Disabled: All UAE expansions are disabled. Only needed if you want to force it.");
	helptext.emplace_back("Original UAE: Autoconfig board + F0 ROM.");
	helptext.emplace_back("New UAE: 64k + F0 ROM - not very useful (per Toni Wilen).");
	return true;
}
