#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"
#include "StringListModel.h"

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "gui_handling.h"
#include "specialmonitors.h"

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
static gcn::RadioButton* optBlitNormal;
static gcn::RadioButton* optBlitImmed;
static gcn::RadioButton* optBlitWait;
static gcn::Window* grpCopper;
static gcn::CheckBox* chkFastCopper;
static gcn::CheckBox* chkMultithreadedDrawing;
static gcn::Label* lblSpecialMonitors;
static gcn::DropDown* cboSpecialMonitors;
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
static gcn::StringListModel specialMonitorsList;

class ChipsetActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.immediate_blits = optBlitImmed->isSelected();
		changed_prefs.waiting_blits = optBlitWait->isSelected();
		changed_prefs.monitoremu = cboSpecialMonitors->getSelected();

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
				//else
				//{
				//	if (changed_prefs.cpu_model < 68020)
				//	{
				//		n2 = false;
				//		chkMemoryCycleExact->setSelected(n2);
				//	}
				//}
			}
			changed_prefs.cpu_cycle_exact = n1;
			changed_prefs.cpu_memory_cycle_exact = changed_prefs.blitter_cycle_exact = n2;
			if (n2)
			{
				if (changed_prefs.cpu_model == 68000)
					changed_prefs.cpu_compatible = true;
				if (changed_prefs.cpu_model <= 68030)
					changed_prefs.m68k_speed = 0;
				if (changed_prefs.immediate_blits)
				{
					changed_prefs.immediate_blits = false;
					optBlitImmed->setSelected(false);
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

		changed_prefs.fast_copper = chkFastCopper->isSelected();
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

	specialMonitorsList.clear();
	specialMonitorsList.add("-");
	specialMonitorsList.add("Autodetect");
	for (int i = 0; specialmonitorfriendlynames[i]; ++i)
		specialMonitorsList.add(specialmonitorfriendlynames[i]);

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
	lblChipset->setAlignment(gcn::Graphics::Right);
	cboChipset = new gcn::DropDown(&chipsetList);
	cboChipset->setSize(120, cboChipset->getHeight());
	cboChipset->setBaseColor(gui_base_color);
	cboChipset->setBackgroundColor(gui_textbox_background_color);
	cboChipset->setForegroundColor(gui_foreground_color);
	cboChipset->setSelectionColor(gui_selection_color);
	cboChipset->setId("cboChipset");
	cboChipset->addActionListener(chipsetActionListener);

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

	grpChipset->setMovable(false);
	grpChipset->setSize(cboChipset->getX() + cboChipset->getWidth() + DISTANCE_BORDER, TITLEBAR_HEIGHT + 250 + DISTANCE_NEXT_Y * 2);
	grpChipset->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpChipset->setBaseColor(gui_base_color);
	grpChipset->setForegroundColor(gui_foreground_color);

	category.panel->add(grpChipset);

	optBlitNormal = new gcn::RadioButton("Normal", "radiocblittergroup");
	optBlitNormal->setId("optBlitNormal");
	optBlitNormal->setBaseColor(gui_base_color);
	optBlitNormal->setBackgroundColor(gui_textbox_background_color);
	optBlitNormal->setForegroundColor(gui_foreground_color);
	optBlitNormal->addActionListener(chipsetActionListener);

	optBlitImmed = new gcn::RadioButton("Immediate", "radiocblittergroup");
	optBlitImmed->setId("optBlitImmed");
	optBlitImmed->setBaseColor(gui_base_color);
	optBlitImmed->setBackgroundColor(gui_textbox_background_color);
	optBlitImmed->setForegroundColor(gui_foreground_color);
	optBlitImmed->addActionListener(chipsetActionListener);

	optBlitWait = new gcn::RadioButton("Wait for blitter", "radiocblittergroup");
	optBlitWait->setId("optBlitWait");
	optBlitWait->setBaseColor(gui_base_color);
	optBlitWait->setBackgroundColor(gui_textbox_background_color);
	optBlitWait->setForegroundColor(gui_foreground_color);
	optBlitWait->addActionListener(chipsetActionListener);

	chkMultithreadedDrawing = new gcn::CheckBox("Multithreaded Drawing");
	chkMultithreadedDrawing->setId("chkMultithreadedDrawing");
	chkMultithreadedDrawing->setBaseColor(gui_base_color);
	chkMultithreadedDrawing->setBackgroundColor(gui_textbox_background_color);
	chkMultithreadedDrawing->setForegroundColor(gui_foreground_color);
	chkMultithreadedDrawing->addActionListener(chipsetActionListener);

	lblSpecialMonitors = new gcn::Label("Video port display hardware:");
	lblSpecialMonitors->setAlignment(gcn::Graphics::Right);
	cboSpecialMonitors = new gcn::DropDown(&specialMonitorsList);
	cboSpecialMonitors->setSize(lblSpecialMonitors->getWidth(), cboSpecialMonitors->getHeight());
	cboSpecialMonitors->setBaseColor(gui_base_color);
	cboSpecialMonitors->setBackgroundColor(gui_textbox_background_color);
	cboSpecialMonitors->setForegroundColor(gui_foreground_color);
	cboSpecialMonitors->setSelectionColor(gui_selection_color);
	cboSpecialMonitors->setId("cboSpecialMonitors");
	cboSpecialMonitors->addActionListener(chipsetActionListener);

	grpOptions = new gcn::Window("Options");
	grpOptions->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_BORDER, DISTANCE_BORDER);
	grpOptions->add(optBlitNormal, 10, 10);
	grpOptions->add(optBlitImmed, 10, 40);
	grpOptions->add(optBlitWait, 10, 70);
	grpOptions->add(chkMultithreadedDrawing, 10, 110);
	grpOptions->add(lblSpecialMonitors, 10, 150);
	grpOptions->add(cboSpecialMonitors, 10, 180);

	grpOptions->setMovable(false);
	grpOptions->setSize(lblSpecialMonitors->getWidth() + DISTANCE_BORDER + DISTANCE_NEXT_X, TITLEBAR_HEIGHT + cboSpecialMonitors->getY() + cboSpecialMonitors->getHeight() + DISTANCE_NEXT_Y * 6);
	grpOptions->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpOptions->setBaseColor(gui_base_color);
	grpOptions->setForegroundColor(gui_foreground_color);

	category.panel->add(grpOptions);

	chkFastCopper = new gcn::CheckBox("Fast copper");
	chkFastCopper->setId("chkFastCopper");
	chkFastCopper->setBaseColor(gui_base_color);
	chkFastCopper->setBackgroundColor(gui_textbox_background_color);
	chkFastCopper->setForegroundColor(gui_foreground_color);
	chkFastCopper->addActionListener(chipsetActionListener);

	grpCopper = new gcn::Window("Copper");
	grpCopper->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_NEXT_X,
		grpOptions->getY() + grpOptions->getHeight() + DISTANCE_NEXT_Y);
	grpCopper->add(chkFastCopper, 10, 10);
	grpCopper->setMovable(false);
	grpCopper->setSize(grpOptions->getWidth(), 65);
	grpCopper->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCopper->setBaseColor(gui_base_color);

	category.panel->add(grpCopper);

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

	delete optBlitNormal;
	delete optBlitImmed;
	delete optBlitWait;
	delete chkMultithreadedDrawing;
	delete lblSpecialMonitors;
	delete cboSpecialMonitors;
	delete grpOptions;

	delete chkFastCopper;
	delete grpCopper;

	delete optCollNone;
	delete optCollSprites;
	delete optCollPlayfield;
	delete optCollFull;
	delete grpCollisionLevel;
}

void RefreshPanelChipset()
{
	if (changed_prefs.immediate_blits)
		optBlitImmed->setSelected(true);
	else if (changed_prefs.waiting_blits)
		optBlitWait->setSelected(true);
	else
		optBlitNormal->setSelected(true);

	if (changed_prefs.cpu_memory_cycle_exact)
	{
		chkFastCopper->setEnabled(false);
		chkFastCopper->setSelected(false);
	}
	else
	{
		chkFastCopper->setEnabled(true);
		chkFastCopper->setSelected(changed_prefs.fast_copper);
	}

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

	chkMultithreadedDrawing->setSelected(changed_prefs.multithreaded_drawing);
	cboSpecialMonitors->setSelected(changed_prefs.monitoremu);

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
	chkCycleExact->setEnabled(changed_prefs.cpu_model <= 68010);
	chkMemoryCycleExact->setEnabled(changed_prefs.cpu_model <= 68010);

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
	helptext.emplace_back("Blitter options and/or disable \"Fast copper\".");
	helptext.emplace_back(" ");
	helptext.emplace_back("\"Fast copper\" uses a prediction algorithm instead of checking the copper state");
	helptext.emplace_back("on a more regular basis. This may cause issues but brings a big performance improvement.");
	helptext.emplace_back("The option was removed in WinUAE in an early state, but for most games, it works fine and");
	helptext.emplace_back("the improved performance is helpful for low powered devices.");
	helptext.emplace_back(" ");
	helptext.emplace_back(R"(For "Collision Level", select "Sprites and Sprites vs. Playfield" which is fine)");
	helptext.emplace_back("for nearly all games.");
	return true;
}
