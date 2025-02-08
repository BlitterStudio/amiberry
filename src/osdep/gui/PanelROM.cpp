#include <cstring>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "rommgr.h"
#include "gui_handling.h"
#include "memory.h"
#include "registry.h"
#include "uae.h"

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
static gcn::CheckBox* chkMapRom;
static gcn::CheckBox* chkKickShifter;

static gcn::StringListModel mainROMList;
static gcn::StringListModel extROMList;
static gcn::StringListModel cartROMList;
static gcn::StringListModel uaeboard_type_list;

// Helper function to reduce code duplication
void handleROMSelection(const gcn::ActionEvent& actionEvent, char* prefs)
{
	std::string tmp;
	const char* filter[] = { ".rom", ".bin", "\0" };
	tmp = SelectFile("Select ROM", get_rom_path(), filter);
	if (!tmp.empty())
	{
		strncpy(prefs, tmp.c_str(), MAX_DPATH);
		fullpath(prefs, MAX_DPATH);
		RefreshPanelROM();
	}
	actionEvent.getSource()->requestFocus();
}

static void getromfile(gcn::DropDown* d, TCHAR* path, int size)
{
	auto val = d->getSelected();
	if (val == -1) {
		auto listmodel = d->getListModel();
		listmodel->add(path);
		val = listmodel->getNumberOfElements() - 1;
		d->setSelected(val);
	}
	else {
		auto listmodel = d->getListModel();
		std::string tmp1 = listmodel->getElementAt(val);
		path[0] = 0;
		struct romdata* rd = getromdatabyname(tmp1.c_str());
		if (rd) {
			struct romlist* rl = getromlistbyromdata(rd);
			if (rd->configname)
				_sntprintf(path, sizeof path, _T(":%s"), rd->configname);
			else if (rl)
				_tcsncpy(path, rl->path, size);
		}
	}
}

class ROMActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		auto source = actionEvent.getSource();
		if (source == cboMainROM)
		{
			getromfile(cboMainROM, changed_prefs.romfile, sizeof(changed_prefs.romfile) / sizeof(TCHAR));
		}
		else if (source == cboExtROM)
		{
			getromfile(cboExtROM, changed_prefs.romextfile, sizeof(changed_prefs.romextfile) / sizeof(TCHAR));
		}
		else if (source == cboCartROM)
		{
			getromfile(cboCartROM, changed_prefs.cartfile, sizeof(changed_prefs.cartfile) / sizeof(TCHAR));
		}
		else if (source == cmdMainROM)
		{
			handleROMSelection(actionEvent, changed_prefs.romfile);
		}
		else if (source == cmdExtROM)
		{
			handleROMSelection(actionEvent, changed_prefs.romextfile);
		}
		else if (source == cmdCartROM)
		{
			handleROMSelection(actionEvent, changed_prefs.cartfile);
		}
		else if (source == cboUAEROM)
		{
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
		else if (source == chkMapRom)
		{
			changed_prefs.maprom = chkMapRom->isSelected() ? 0x0f000000 : 0;
		}
		else if (source == chkKickShifter)
		{
			changed_prefs.kickshifter = chkKickShifter->isSelected();
		}
		read_kickstart_version(&changed_prefs);
	}
};

static ROMActionListener* romActionListener;

void InitPanelROM(const config_category& category)
{
	uaeboard_type_list.clear();
	uaeboard_type_list.add("ROM disabled");
	uaeboard_type_list.add("Original UAE (FS + F0 ROM)");
	uaeboard_type_list.add("New UAE (64k + F0 ROM)");
	uaeboard_type_list.add("New UAE (128k, ROM, Direct)");
	uaeboard_type_list.add("New UAE (128k, ROM, Indirect)");

	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;

	romActionListener = new ROMActionListener();

	lblMainROM = new gcn::Label("Main ROM File:");
	cboMainROM = new gcn::DropDown(&mainROMList);
	cboMainROM->setSize(textFieldWidth, cboMainROM->getHeight());
	cboMainROM->setBaseColor(gui_base_color);
	cboMainROM->setBackgroundColor(gui_background_color);
	cboMainROM->setForegroundColor(gui_foreground_color);
	cboMainROM->setSelectionColor(gui_selection_color);
	cboMainROM->setId("cboMainROM");
	cboMainROM->addActionListener(romActionListener);
	cmdMainROM = new gcn::Button("...");
	cmdMainROM->setId("MainROM");
	cmdMainROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdMainROM->setBaseColor(gui_base_color);
	cmdMainROM->setForegroundColor(gui_foreground_color);
	cmdMainROM->addActionListener(romActionListener);

	lblExtROM = new gcn::Label("Extended ROM File:");
	cboExtROM = new gcn::DropDown(&extROMList);
	cboExtROM->setSize(textFieldWidth, cboExtROM->getHeight());
	cboExtROM->setBaseColor(gui_base_color);
	cboExtROM->setBackgroundColor(gui_background_color);
	cboExtROM->setForegroundColor(gui_foreground_color);
	cboExtROM->setSelectionColor(gui_selection_color);
	cboExtROM->setId("cboExtROM");
	cboExtROM->addActionListener(romActionListener);
	cmdExtROM = new gcn::Button("...");
	cmdExtROM->setId("ExtROM");
	cmdExtROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdExtROM->setBaseColor(gui_base_color);
	cmdExtROM->setForegroundColor(gui_foreground_color);
	cmdExtROM->addActionListener(romActionListener);

	chkMapRom = new gcn::CheckBox("MapROM emulation");
	chkMapRom->setId("chkMapRom");
	chkMapRom->setBaseColor(gui_base_color);
	chkMapRom->setBackgroundColor(gui_background_color);
	chkMapRom->setForegroundColor(gui_foreground_color);
	chkMapRom->addActionListener(romActionListener);

	chkKickShifter = new gcn::CheckBox("ShapeShifter support");
	chkKickShifter->setId("chkKickShifter");
	chkKickShifter->setBaseColor(gui_base_color);
	chkKickShifter->setBackgroundColor(gui_background_color);
	chkKickShifter->setForegroundColor(gui_foreground_color);
	chkKickShifter->addActionListener(romActionListener);

	lblCartROM = new gcn::Label("Cartridge ROM File:");
	cboCartROM = new gcn::DropDown(&cartROMList);
	cboCartROM->setSize(textFieldWidth, cboCartROM->getHeight());
	cboCartROM->setBaseColor(gui_base_color);
	cboCartROM->setBackgroundColor(gui_background_color);
	cboCartROM->setForegroundColor(gui_foreground_color);
	cboCartROM->setSelectionColor(gui_selection_color);
	cboCartROM->setId("cboCartROM");
	cboCartROM->addActionListener(romActionListener);
	cmdCartROM = new gcn::Button("...");
	cmdCartROM->setId("CartROM");
	cmdCartROM->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdCartROM->setBaseColor(gui_base_color);
	cmdCartROM->setForegroundColor(gui_foreground_color);
	cmdCartROM->addActionListener(romActionListener);

	lblUAEROM = new gcn::Label("Advanced UAE expansion board/Boot ROM:");
	cboUAEROM = new gcn::DropDown(&uaeboard_type_list);
	cboUAEROM->setSize(textFieldWidth, cboUAEROM->getHeight());
	cboUAEROM->setBaseColor(gui_base_color);
	cboUAEROM->setBackgroundColor(gui_background_color);\
	cboUAEROM->setForegroundColor(gui_foreground_color);
	cboUAEROM->setSelectionColor(gui_selection_color);
	cboUAEROM->setId("cboUAEROM");
	cboUAEROM->addActionListener(romActionListener);

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

	category.panel->add(chkMapRom, DISTANCE_BORDER, posY);
	category.panel->add(chkKickShifter, chkMapRom->getX() + chkMapRom->getWidth() + DISTANCE_NEXT_X * 2, posY);
	posY += chkMapRom->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(lblCartROM, DISTANCE_BORDER, posY);
	posY += lblCartROM->getHeight() + 4;
	category.panel->add(cboCartROM, DISTANCE_BORDER, posY);
	category.panel->add(cmdCartROM, DISTANCE_BORDER + cboCartROM->getWidth() + DISTANCE_NEXT_X, posY);
	posY += cboCartROM->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblUAEROM, DISTANCE_BORDER, posY);
	posY += lblUAEROM->getHeight() + 4;
	category.panel->add(cboUAEROM, DISTANCE_BORDER, posY);

	RefreshPanelROM();
}

void ExitPanelROM()
{
	delete lblMainROM;
	delete cboMainROM;
	delete cmdMainROM;
	delete romActionListener;

	delete lblExtROM;
	delete cboExtROM;
	delete cmdExtROM;

	delete lblCartROM;
	delete cboCartROM;
	delete cmdCartROM;

	delete lblUAEROM;
	delete cboUAEROM;
	delete chkMapRom;
	delete chkKickShifter;
}

void RefreshPanelROM()
{
	// Load the ROMs
	UAEREG* fkey = regcreatetree(nullptr, _T("DetectedROMs"));
	load_keyring(&changed_prefs, nullptr);
	addromfiles(fkey, cboMainROM, changed_prefs.romfile,
		ROMTYPE_KICK | ROMTYPE_KICKCD32, 0);
	addromfiles(fkey, cboExtROM, changed_prefs.romextfile,
		ROMTYPE_EXTCD32 | ROMTYPE_EXTCDTV | ROMTYPE_ARCADIABIOS | ROMTYPE_ALG, 0);
	addromfiles(fkey, cboCartROM, changed_prefs.cartfile,
		ROMTYPE_FREEZER | ROMTYPE_ARCADIAGAME | ROMTYPE_CD32CART, 0);
	regclosetree(fkey);

	//TODO add flashfile and rtcfile options

	chkKickShifter->setSelected(changed_prefs.kickshifter);
	chkMapRom->setSelected(changed_prefs.maprom);

	if (changed_prefs.boot_rom == 1) {
		cboUAEROM->setSelected(0);
	}
	else {
		cboUAEROM->setSelected(changed_prefs.uaeboard + 1);
	}
	cboUAEROM->setEnabled(!emulating);
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
