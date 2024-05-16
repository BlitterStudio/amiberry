#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "gui_handling.h"

static gcn::Window* grpChipset;
static gcn::RadioButton* optOCS;
static gcn::RadioButton* optECSAgnus;
static gcn::RadioButton* optECSDenise;
static gcn::RadioButton* optECS;
static gcn::RadioButton* optAGA;
static gcn::CheckBox* chkNTSC;
static gcn::CheckBox* chkCycleExact;
static gcn::CheckBox* chkMemoryCycleExact;
static gcn::Label* lblChipset;
static gcn::DropDown* cboChipset;
static gcn::Window* grpOptions;
static gcn::CheckBox* chkKeyboardConnected;
static gcn::CheckBox* chkSubpixelEmu;
static gcn::CheckBox* chkBlitImmed;
static gcn::CheckBox* chkBlitWait;
static gcn::CheckBox* chkMultithreadedDrawing;
static gcn::Window* grpCollisionLevel;
static gcn::RadioButton* optCollNone;
static gcn::RadioButton* optCollSprites;
static gcn::RadioButton* optCollPlayfield;
static gcn::RadioButton* optCollFull;

struct chipset
{
	int compatible;
	char name[32];
};

static chipset chipsets[] = {
	{CP_GENERIC, "Generic"},
	{CP_CDTV, "CDTV"},
	{CP_CDTVCR, "CDTV-CR"},
	{CP_CD32, "CD32"},
	{CP_A500, "A500"},
	{CP_A500P, "A500+"},
	{CP_A600, "A600"},
	{CP_A1000, "A1000"},
	{CP_A1200, "A1200"},
	{CP_A2000, "A2000"},
	{CP_A3000, "A3000"},
	{CP_A3000T, "A3000T"},
	{CP_A4000, "A4000"},
	{CP_A4000T, "A4000T"},
	{CP_VELVET, "Velvet"},
	{CP_CASABLANCA, "Casablanca"},
	{CP_DRACO, "DraCo"},
	{-1, ""}
};

static gcn::StringListModel chipsetList;

class ChipsetActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.keyboard_connected = chkKeyboardConnected->isSelected();
		changed_prefs.chipset_hr = chkSubpixelEmu->isSelected();
		changed_prefs.immediate_blits = chkBlitImmed->isSelected();
		changed_prefs.waiting_blits = chkBlitWait->isSelected();

		auto n2 = chkMemoryCycleExact->isSelected();
		auto n1 = chkCycleExact->isSelected();
		if (changed_prefs.cpu_cycle_exact != n1 || changed_prefs.cpu_memory_cycle_exact != n2)
		{
			if (actionEvent.getSource() == chkMemoryCycleExact)
			{
				if (n2)
				{
					if (changed_prefs.cpu_model < 68020)
					{
						n1 = true;
						chkCycleExact->setSelected(n1);
					}
				}
				else
				{
					n1 = false;
					chkCycleExact->setSelected(n1);
				}
			}
			else if (actionEvent.getSource() == chkCycleExact)
			{
				if (n1)
				{
					n2 = true;
					chkMemoryCycleExact->setSelected(n2);
				}
				else
				{
					if (changed_prefs.cpu_model < 68020)
					{
						n2 = false;
						chkMemoryCycleExact->setSelected(n2);
					}
				}
			}
			changed_prefs.cpu_cycle_exact = n1;
			changed_prefs.cpu_memory_cycle_exact = changed_prefs.blitter_cycle_exact = n2;
			if (n2)
			{
				if (changed_prefs.cpu_model <= 68030)
				{
					changed_prefs.m68k_speed = 0;
					changed_prefs.cpu_compatible = true;
				}
				if (changed_prefs.immediate_blits)
				{
					changed_prefs.immediate_blits = false;
					chkBlitImmed->setSelected(false);
				}
				changed_prefs.gfx_framerate = 1;
				changed_prefs.cachesize = 0;
			}
		}

		changed_prefs.collision_level = optCollNone->isSelected() ? 0
			: optCollSprites->isSelected() ? 1
			: optCollPlayfield->isSelected() ? 2 : 3;

		changed_prefs.chipset_mask = optOCS->isSelected() ? 0
			: optECSAgnus->isSelected() ? CSMASK_ECS_AGNUS
			: optECSDenise->isSelected() ? CSMASK_ECS_DENISE
			: optECS->isSelected() ? CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE
			: CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;

		n1 = chkNTSC->isSelected();
		if (changed_prefs.ntscmode != n1)
		{
			changed_prefs.ntscmode = n1;
		}

		const auto cs = chipsets[cboChipset->getSelected()].compatible;
		if (changed_prefs.cs_compatible != cs)
		{
			changed_prefs.cs_compatible = cs;
			built_in_chipset_prefs(&changed_prefs);
		}

		changed_prefs.multithreaded_drawing = chkMultithreadedDrawing->isSelected();

		RefreshPanelCPU();
		RefreshPanelQuickstart();
		RefreshPanelChipset();
	}
};
static ChipsetActionListener* chipsetActionListener;

void InitPanelChipset(const struct config_category& category)
{
	chipsetList.clear();
	for (int i = 0; chipsets[i].compatible >= 0; ++i)
		chipsetList.add(chipsets[i].name);

	chipsetActionListener = new ChipsetActionListener();

	optOCS = new gcn::RadioButton("OCS", "radiochipsetgroup");
	optOCS->setId("optOCS");
	optOCS->setBaseColor(gui_base_color);
	optOCS->setBackgroundColor(gui_textbox_background_color);
	optOCS->setForegroundColor(gui_foreground_color);
	optOCS->addActionListener(chipsetActionListener);

	optECSAgnus = new gcn::RadioButton("ECS Agnus", "radiochipsetgroup");
	optECSAgnus->setId("optECSAgnus");
	optECSAgnus->setBaseColor(gui_base_color);
	optECSAgnus->setBackgroundColor(gui_textbox_background_color);
	optECSAgnus->setForegroundColor(gui_foreground_color);
	optECSAgnus->addActionListener(chipsetActionListener);

	optECSDenise = new gcn::RadioButton("ECS Denise", "radiochipsetgroup");
	optECSDenise->setId("optECSDenise");
	optECSDenise->setBaseColor(gui_base_color);
	optECSDenise->setBackgroundColor(gui_textbox_background_color);
	optECSDenise->setForegroundColor(gui_foreground_color);
	optECSDenise->addActionListener(chipsetActionListener);
	
	optECS = new gcn::RadioButton("Full ECS", "radiochipsetgroup");
	optECS->setId("optFullECS");
	optECS->setBaseColor(gui_base_color);
	optECS->setBackgroundColor(gui_textbox_background_color);
	optECS->setForegroundColor(gui_foreground_color);
	optECS->addActionListener(chipsetActionListener);

	optAGA = new gcn::RadioButton("AGA", "radiochipsetgroup");
	optAGA->setId("optAGA");
	optAGA->setBaseColor(gui_base_color);
	optAGA->setBackgroundColor(gui_textbox_background_color);
	optAGA->setForegroundColor(gui_foreground_color);
	optAGA->addActionListener(chipsetActionListener);

	chkNTSC = new gcn::CheckBox("NTSC");
	chkNTSC->setId("chkNTSC");
	chkNTSC->setBaseColor(gui_base_color);
	chkNTSC->setBackgroundColor(gui_textbox_background_color);
	chkNTSC->setForegroundColor(gui_foreground_color);
	chkNTSC->addActionListener(chipsetActionListener);

	chkCycleExact = new gcn::CheckBox("Cycle Exact (Full)");
	chkCycleExact->setId("chkCycleExact");
	chkCycleExact->setBaseColor(gui_base_color);
	chkCycleExact->setBackgroundColor(gui_textbox_background_color);
	chkCycleExact->setForegroundColor(gui_foreground_color);
	chkCycleExact->addActionListener(chipsetActionListener);

	chkMemoryCycleExact = new gcn::CheckBox("Cycle Exact (DMA/Memory)");
	chkMemoryCycleExact->setId("chkMemoryCycleExact");
	chkMemoryCycleExact->setBaseColor(gui_base_color);
	chkMemoryCycleExact->setBackgroundColor(gui_textbox_background_color);
	chkMemoryCycleExact->setForegroundColor(gui_foreground_color);
	chkMemoryCycleExact->addActionListener(chipsetActionListener);

	lblChipset = new gcn::Label("Chipset Extra:");
	lblChipset->setAlignment(gcn::Graphics::RIGHT);
	cboChipset = new gcn::DropDown(&chipsetList);
	cboChipset->setSize(120, cboChipset->getHeight());
	cboChipset->setBaseColor(gui_base_color);
	cboChipset->setBackgroundColor(gui_textbox_background_color);
	cboChipset->setForegroundColor(gui_foreground_color);
	cboChipset->setSelectionColor(gui_selection_color);
	cboChipset->setId("cboChipset");
	cboChipset->addActionListener(chipsetActionListener);

	chkMultithreadedDrawing = new gcn::CheckBox("Multithreaded Drawing");
	chkMultithreadedDrawing->setId("chkMultithreadedDrawing");
	chkMultithreadedDrawing->setBaseColor(gui_base_color);
	chkMultithreadedDrawing->setBackgroundColor(gui_textbox_background_color);
	chkMultithreadedDrawing->setForegroundColor(gui_foreground_color);
	chkMultithreadedDrawing->addActionListener(chipsetActionListener);

	grpChipset = new gcn::Window("Chipset");
	grpChipset->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpChipset->add(optOCS, 10, 10);
	grpChipset->add(optECSAgnus, 10, 40);
	grpChipset->add(optECS, 10, 70);
	grpChipset->add(optAGA, 165, 10);
	grpChipset->add(optECSDenise, 165, 40);
	grpChipset->add(chkNTSC, 165, 70);
	grpChipset->add(chkCycleExact, 10, 120);
	grpChipset->add(chkMemoryCycleExact, 10, 150);
	grpChipset->add(lblChipset, 60, 180);
	grpChipset->add(cboChipset, 60 + lblChipset->getWidth() + 10, 180);
	grpChipset->add(chkMultithreadedDrawing, 10, 250);

	grpChipset->setMovable(false);
	grpChipset->setSize(cboChipset->getX() + cboChipset->getWidth() + DISTANCE_BORDER, TITLEBAR_HEIGHT + 250 + chkMultithreadedDrawing->getHeight() + DISTANCE_NEXT_Y * 2);
	grpChipset->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpChipset->setBaseColor(gui_base_color);
	grpChipset->setForegroundColor(gui_foreground_color);

	category.panel->add(grpChipset);

	chkKeyboardConnected = new gcn::CheckBox("Keyboard connected");
	chkKeyboardConnected->setId("chkKeyboardConnected");
	chkKeyboardConnected->setBaseColor(gui_base_color);
	chkKeyboardConnected->setBackgroundColor(gui_textbox_background_color);
	chkKeyboardConnected->setForegroundColor(gui_foreground_color);
	chkKeyboardConnected->addActionListener(chipsetActionListener);

	chkSubpixelEmu = new gcn::CheckBox("Subpixel Display emulation");
	chkSubpixelEmu->setId("chkSubpixelEmu");
	chkSubpixelEmu->setBaseColor(gui_base_color);
	chkSubpixelEmu->setBackgroundColor(gui_textbox_background_color);
	chkSubpixelEmu->setForegroundColor(gui_foreground_color);
	chkSubpixelEmu->addActionListener(chipsetActionListener);

	chkBlitImmed = new gcn::CheckBox("Immediate Blitter");
	chkBlitImmed->setId("chkBlitImmed");
	chkBlitImmed->setBaseColor(gui_base_color);
	chkBlitImmed->setBackgroundColor(gui_textbox_background_color);
	chkBlitImmed->setForegroundColor(gui_foreground_color);
	chkBlitImmed->addActionListener(chipsetActionListener);

	chkBlitWait = new gcn::CheckBox("Wait for Blitter");
	chkBlitWait->setId("chkBlitWait");
	chkBlitWait->setBaseColor(gui_base_color);
	chkBlitWait->setBackgroundColor(gui_textbox_background_color);
	chkBlitWait->setForegroundColor(gui_foreground_color);
	chkBlitWait->addActionListener(chipsetActionListener);

	grpOptions = new gcn::Window("Options");
	grpOptions->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_BORDER, DISTANCE_BORDER);
	grpOptions->add(chkKeyboardConnected, 10, 10);
	grpOptions->add(chkSubpixelEmu, 10, 40);
	grpOptions->add(chkBlitImmed, 10, 70);
	grpOptions->add(chkBlitWait, 10, 100);
	grpOptions->setMovable(false);
	grpOptions->setSize(chkSubpixelEmu->getWidth() + DISTANCE_BORDER + DISTANCE_NEXT_X, TITLEBAR_HEIGHT + 100 + chkBlitWait->getHeight() + DISTANCE_NEXT_Y);
	grpOptions->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpOptions->setBaseColor(gui_base_color);
	grpOptions->setForegroundColor(gui_foreground_color);

	category.panel->add(grpOptions);

	optCollNone = new gcn::RadioButton("None", "radioccollisiongroup");
	optCollNone->setId("optCollNone");
	optCollNone->setBaseColor(gui_base_color);
	optCollNone->setBackgroundColor(gui_textbox_background_color);
	optCollNone->setForegroundColor(gui_foreground_color);
	optCollNone->addActionListener(chipsetActionListener);

	optCollSprites = new gcn::RadioButton("Sprites only", "radioccollisiongroup");
	optCollSprites->setId("optCollSprites");
	optCollSprites->setBaseColor(gui_base_color);
	optCollSprites->setBackgroundColor(gui_textbox_background_color);
	optCollSprites->setForegroundColor(gui_foreground_color);
	optCollSprites->addActionListener(chipsetActionListener);

	optCollPlayfield = new gcn::RadioButton("Sprites and Sprites vs. Playfield", "radioccollisiongroup");
	optCollPlayfield->setId("optCollPlayfield");
	optCollPlayfield->setBaseColor(gui_base_color);
	optCollPlayfield->setBackgroundColor(gui_textbox_background_color);
	optCollPlayfield->setForegroundColor(gui_foreground_color);
	optCollPlayfield->addActionListener(chipsetActionListener);

	optCollFull = new gcn::RadioButton("Full (rarely needed)", "radioccollisiongroup");
	optCollFull->setId("optCollFull");
	optCollFull->setBaseColor(gui_base_color);
	optCollFull->setBackgroundColor(gui_textbox_background_color);
	optCollFull->setForegroundColor(gui_foreground_color);
	optCollFull->addActionListener(chipsetActionListener);

	grpCollisionLevel = new gcn::Window("Collision Level");
	grpCollisionLevel->setPosition(DISTANCE_BORDER, DISTANCE_BORDER + grpChipset->getHeight() + DISTANCE_NEXT_Y);
	grpCollisionLevel->add(optCollNone, 10, 10);
	grpCollisionLevel->add(optCollSprites, 10, 40);
	grpCollisionLevel->add(optCollPlayfield, 10, 70);
	grpCollisionLevel->add(optCollFull, 10, 100);
	grpCollisionLevel->setMovable(false);
	grpCollisionLevel->setSize(grpChipset->getWidth(), TITLEBAR_HEIGHT + 100 + optCollFull->getHeight() + DISTANCE_NEXT_Y);
	grpCollisionLevel->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCollisionLevel->setBaseColor(gui_base_color);
	grpCollisionLevel->setForegroundColor(gui_foreground_color);

	category.panel->add(grpCollisionLevel);

	RefreshPanelChipset();
}

void ExitPanelChipset()
{
	delete optOCS;
	delete optECSAgnus;
	delete optECSDenise;
	delete optECS;
	delete optAGA;
	delete chkNTSC;
	delete chkCycleExact;
	delete chkMemoryCycleExact;
	delete lblChipset;
	delete cboChipset;
	delete grpChipset;
	delete chipsetActionListener;

	delete chkKeyboardConnected;
	delete chkSubpixelEmu;
	delete chkBlitImmed;
	delete chkBlitWait;
	delete grpOptions;

	delete chkMultithreadedDrawing;
	delete optCollNone;
	delete optCollSprites;
	delete optCollPlayfield;
	delete optCollFull;
	delete grpCollisionLevel;
}

void RefreshPanelChipset()
{
	// Set Enabled status
#if !defined (CPUEMU_13)
	chkMemoryCycleExact->setEnabled(false);
#else
	chkMemoryCycleExact->setEnabled(changed_prefs.cpu_model >= 68020);
#endif
	if (changed_prefs.immediate_blits || (changed_prefs.cpu_memory_cycle_exact && changed_prefs.cpu_model <= 68010)) {
		changed_prefs.waiting_blits = 0;
		chkBlitWait->setSelected(false);
		chkBlitWait->setEnabled(false);
	}
	else
	{
		chkBlitWait->setEnabled(true);
	}
	chkBlitImmed->setEnabled(!changed_prefs.cpu_cycle_exact);
	chkMultithreadedDrawing->setEnabled(!emulating);

	// Set Values
	if (changed_prefs.chipset_mask == 0)
		optOCS->setSelected(true);
	else if (changed_prefs.chipset_mask == CSMASK_ECS_AGNUS)
		optECSAgnus->setSelected(true);
	else if (changed_prefs.chipset_mask == CSMASK_ECS_DENISE)
		optECSDenise->setSelected(true);
	else if (changed_prefs.chipset_mask == (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE))
		optECS->setSelected(true);
	else if (changed_prefs.chipset_mask == (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA))
		optAGA->setSelected(true);

	chkNTSC->setSelected(changed_prefs.ntscmode);
	chkKeyboardConnected->setSelected(changed_prefs.keyboard_connected);
	chkSubpixelEmu->setSelected(changed_prefs.chipset_hr);
	chkBlitImmed->setSelected(changed_prefs.immediate_blits);
	chkBlitWait->setSelected(changed_prefs.waiting_blits);

	if (changed_prefs.collision_level == 0)
		optCollNone->setSelected(true);
	else if (changed_prefs.collision_level == 1)
		optCollSprites->setSelected(true);
	else if (changed_prefs.collision_level == 2)
		optCollPlayfield->setSelected(true);
	else
		optCollFull->setSelected(true);

	chkCycleExact->setSelected(changed_prefs.cpu_cycle_exact);
	chkMemoryCycleExact->setSelected(changed_prefs.cpu_memory_cycle_exact);

	auto idx = 0;
	for (auto i = 0; i < chipsetList.getNumberOfElements(); ++i)
	{
		if (chipsets[i].compatible == changed_prefs.cs_compatible)
		{
			idx = i;
			break;
		}
	}
	cboChipset->setSelected(idx);
	chkMultithreadedDrawing->setSelected(changed_prefs.multithreaded_drawing);
}

bool HelpPanelChipset(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("If you want to emulate an Amiga 1200, select AGA. For most Amiga 500 games,");
	helptext.emplace_back(R"(select "Full ECS" instead. Some older Amiga games require "OCS" or "ECS Agnus".)");
	helptext.emplace_back("You have to play with these options if a game won't work as expected. By selecting");
	helptext.emplace_back("an entry in \"Extra\", all internal Chipset settings will change to the required values");
	helptext.emplace_back("for the specified Amiga model. For some games, you have to switch to \"NTSC\"");
	helptext.emplace_back("(60 Hz instead of 50 Hz) for correct timing.");
	helptext.emplace_back(" ");
	helptext.emplace_back("The \"Multithreaded drawing\" option, will enable some Amiberry optimizations");
	helptext.emplace_back("that will help the performance when drawing lines on native screen modes.");
	helptext.emplace_back("In some cases, this might cause screen tearing artifacts, so you can choose to");
	helptext.emplace_back("disable this option when needed. Note that it cannot be changed once emulation has");
	helptext.emplace_back("started.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(If you see graphic issues in a game, try the "Immediate" or "Wait for blitter")");
	helptext.emplace_back("Blitter options.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(For "Collision Level", select "Sprites and Sprites vs. Playfield" which is fine)");
	helptext.emplace_back("for nearly all games.");
	return true;
}
