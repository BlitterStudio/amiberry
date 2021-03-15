#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

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
static gcn::Window* grpBlitter;
static gcn::RadioButton* optBlitNormal;
static gcn::RadioButton* optBlitImmed;
static gcn::RadioButton* optBlitWait;
static gcn::Window* grpCopper;
static gcn::CheckBox* chkFastCopper;
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

static struct chipset chipsets[] = {
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

static const int numChipsets = 17;

class ChipsetListModel : public gcn::ListModel
{
public:
	ChipsetListModel()
	= default;

	int getNumberOfElements() override
	{
		return numChipsets;
	}

	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= numChipsets)
			return "---";
		return chipsets[i].name;
	}
};

static ChipsetListModel chipsetList;
static bool bIgnoreListChange = true;

class ChipsetActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (!bIgnoreListChange)
		{
			if (actionEvent.getSource() == cboChipset)
			{
				//---------------------------------------
				// Chipset selected
				//---------------------------------------
				const auto cs = chipsets[cboChipset->getSelected()].compatible;
				if (changed_prefs.cs_compatible != cs)
				{
					changed_prefs.cs_compatible = cs;
					built_in_chipset_prefs(&changed_prefs);
					RefreshPanelChipset();
				}
			}
		}
	}
};
static ChipsetActionListener* chipsetActionListener;

class ChipsetButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optOCS)
			changed_prefs.chipset_mask = 0;
		else if (actionEvent.getSource() == optECSAgnus)
			changed_prefs.chipset_mask = CSMASK_ECS_AGNUS;
		else if (actionEvent.getSource() == optECSDenise)
			changed_prefs.chipset_mask = CSMASK_ECS_DENISE;
		else if (actionEvent.getSource() == optECS)
			changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
		else
			changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	}
};
static ChipsetButtonActionListener* chipsetButtonActionListener;

class CycleExactActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
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
			else if (actionEvent.getSource() == chkMemoryCycleExact)
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
		RefreshPanelCPU();
		RefreshPanelQuickstart();
		RefreshPanelChipset();
	}
};
static CycleExactActionListener* cycleExactActionListener;

class NTSCButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (chkNTSC->isSelected())
		{
			changed_prefs.ntscmode = true;
			changed_prefs.chipset_refreshrate = 60.0;
		}
		else
		{
			changed_prefs.ntscmode = false;
			changed_prefs.chipset_refreshrate = 50.0;
		}
		RefreshPanelQuickstart();
	}
};
static NTSCButtonActionListener* ntscButtonActionListener;

class FastCopperActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.fast_copper = chkFastCopper->isSelected();
	}
};
static FastCopperActionListener* fastCopperActionListener;

class BlitterButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		changed_prefs.immediate_blits = optBlitImmed->isSelected();
		changed_prefs.waiting_blits = optBlitWait->isSelected();
	}
};
static BlitterButtonActionListener* blitterButtonActionListener;

class CollisionButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == optCollNone)
			changed_prefs.collision_level = 0;
		else if (actionEvent.getSource() == optCollSprites)
			changed_prefs.collision_level = 1;
		else if (actionEvent.getSource() == optCollPlayfield)
			changed_prefs.collision_level = 2;
		else
			changed_prefs.collision_level = 3;
	}
};
static CollisionButtonActionListener* collisionButtonActionListener;

void InitPanelChipset(const struct _ConfigCategory& category)
{
	chipsetActionListener = new ChipsetActionListener();
	chipsetButtonActionListener = new ChipsetButtonActionListener();
	ntscButtonActionListener = new NTSCButtonActionListener();
	cycleExactActionListener = new CycleExactActionListener();

	optOCS = new gcn::RadioButton("OCS", "radiochipsetgroup");
	optOCS->setId("optOCS");
	optOCS->addActionListener(chipsetButtonActionListener);

	optECSAgnus = new gcn::RadioButton("ECS Agnus", "radiochipsetgroup");
	optECSAgnus->setId("optECSAgnus");
	optECSAgnus->addActionListener(chipsetButtonActionListener);

	optECSDenise = new gcn::RadioButton("ECS Denise", "radiochipsetgroup");
	optECSDenise->setId("optECSDenise");
	optECSDenise->addActionListener(chipsetButtonActionListener);
	
	optECS = new gcn::RadioButton("Full ECS", "radiochipsetgroup");
	optECS->setId("optFullECS");
	optECS->addActionListener(chipsetButtonActionListener);

	optAGA = new gcn::RadioButton("AGA", "radiochipsetgroup");
	optAGA->setId("optAGA");
	optAGA->addActionListener(chipsetButtonActionListener);

	chkNTSC = new gcn::CheckBox("NTSC");
	chkNTSC->setId("chkNTSC");
	chkNTSC->addActionListener(ntscButtonActionListener);

	chkCycleExact = new gcn::CheckBox("Cycle Exact (Full)");
	chkCycleExact->setId("chkCycleExact");
	chkCycleExact->addActionListener(cycleExactActionListener);

	chkMemoryCycleExact = new gcn::CheckBox("Cycle Exact (DMA/Memory)");
	chkMemoryCycleExact->setId("chkMemoryCycleExact");
	chkMemoryCycleExact->addActionListener(cycleExactActionListener);

	lblChipset = new gcn::Label("Chipset Extra:");
	lblChipset->setAlignment(gcn::Graphics::RIGHT);
	cboChipset = new gcn::DropDown(&chipsetList);
	cboChipset->setSize(120, cboChipset->getHeight());
	cboChipset->setBaseColor(gui_baseCol);
	cboChipset->setBackgroundColor(colTextboxBackground);
	cboChipset->setId("ChipsetExtra");
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
	grpChipset->add(lblChipset, 80, 180);
	grpChipset->add(cboChipset, 80 + lblChipset->getWidth() + 10, 180);

	grpChipset->setMovable(false);
	grpChipset->setSize(350, 300);
	grpChipset->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpChipset->setBaseColor(gui_baseCol);

	category.panel->add(grpChipset);

	blitterButtonActionListener = new BlitterButtonActionListener();

	optBlitNormal = new gcn::RadioButton("Normal", "radiocblittergroup");
	optBlitNormal->setId("BlitNormal");
	optBlitNormal->addActionListener(blitterButtonActionListener);

	optBlitImmed = new gcn::RadioButton("Immediate", "radiocblittergroup");
	optBlitImmed->setId("Immediate");
	optBlitImmed->addActionListener(blitterButtonActionListener);

	optBlitWait = new gcn::RadioButton("Wait for blitter", "radiocblittergroup");
	optBlitWait->setId("BlitWait");
	optBlitWait->addActionListener(blitterButtonActionListener);

	grpBlitter = new gcn::Window("Blitter");
	grpBlitter->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpBlitter->add(optBlitNormal, 10, 10);
	grpBlitter->add(optBlitImmed, 10, 40);
	grpBlitter->add(optBlitWait, 10, 70);
	grpBlitter->setMovable(false);
	grpBlitter->setSize(optBlitWait->getWidth() + DISTANCE_BORDER + 10, 125);
	grpBlitter->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpBlitter->setBaseColor(gui_baseCol);

	category.panel->add(grpBlitter);

	fastCopperActionListener = new FastCopperActionListener();

	chkFastCopper = new gcn::CheckBox("Fast copper");
	chkFastCopper->setId("Fast copper");
	chkFastCopper->addActionListener(fastCopperActionListener);

	grpCopper = new gcn::Window("Copper");
	grpCopper->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_NEXT_X,
	                       grpBlitter->getY() + grpBlitter->getHeight() + DISTANCE_NEXT_Y);
	grpCopper->add(chkFastCopper, 10, 10);
	grpCopper->setMovable(false);
	grpCopper->setSize(grpBlitter->getWidth(), 65);
	grpCopper->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCopper->setBaseColor(gui_baseCol);

	category.panel->add(grpCopper);

	collisionButtonActionListener = new CollisionButtonActionListener();

	optCollNone = new gcn::RadioButton("None", "radioccollisiongroup");
	optCollNone->setId("CollNone");
	optCollNone->addActionListener(collisionButtonActionListener);

	optCollSprites = new gcn::RadioButton("Sprites only", "radioccollisiongroup");
	optCollSprites->setId("Sprites only");
	optCollSprites->addActionListener(collisionButtonActionListener);

	optCollPlayfield = new gcn::RadioButton("Sprites and Sprites vs. Playfield", "radioccollisiongroup");
	optCollPlayfield->setId("CollPlay");
	optCollPlayfield->addActionListener(collisionButtonActionListener);

	optCollFull = new gcn::RadioButton("Full (rarely needed)", "radioccollisiongroup");
	optCollFull->setId("CollFull");
	optCollFull->addActionListener(collisionButtonActionListener);

	grpCollisionLevel = new gcn::Window("Collision Level");
	grpCollisionLevel->setPosition(DISTANCE_BORDER, DISTANCE_BORDER + grpChipset->getHeight() + DISTANCE_NEXT_Y);
	grpCollisionLevel->add(optCollNone, 10, 10);
	grpCollisionLevel->add(optCollSprites, 10, 40);
	grpCollisionLevel->add(optCollPlayfield, 10, 70);
	grpCollisionLevel->add(optCollFull, 10, 100);
	grpCollisionLevel->setMovable(false);
	grpCollisionLevel->setSize(grpChipset->getWidth(), 165);
	grpCollisionLevel->setTitleBarHeight(TITLEBAR_HEIGHT);
	grpCollisionLevel->setBaseColor(gui_baseCol);

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
	delete chipsetButtonActionListener;
	delete ntscButtonActionListener;
	delete cycleExactActionListener;
	delete chipsetActionListener;

	delete optBlitNormal;
	delete optBlitImmed;
	delete optBlitWait;
	delete grpBlitter;
	delete blitterButtonActionListener;

	delete chkFastCopper;
	delete grpCopper;
	delete fastCopperActionListener;
	delete optCollNone;
	delete optCollSprites;
	delete optCollPlayfield;
	delete optCollFull;
	delete grpCollisionLevel;
	delete collisionButtonActionListener;
}


void RefreshPanelChipset()
{
	bIgnoreListChange = true;
	auto idx = 0;
	for (auto i = 0; i < numChipsets; ++i)
	{
		if (chipsets[i].compatible == changed_prefs.cs_compatible)
		{
			idx = i;
			break;
		}
	}
	cboChipset->setSelected(idx);
	bIgnoreListChange = false;

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
	chkCycleExact->setSelected(changed_prefs.cpu_cycle_exact);
	chkMemoryCycleExact->setSelected(changed_prefs.cpu_memory_cycle_exact);
	chkCycleExact->setEnabled(changed_prefs.cpu_model <= 68010);
	chkMemoryCycleExact->setEnabled(changed_prefs.cpu_model <= 68010);

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

	if (changed_prefs.collision_level == 0)
		optCollNone->setSelected(true);
	else if (changed_prefs.collision_level == 1)
		optCollSprites->setSelected(true);
	else if (changed_prefs.collision_level == 2)
		optCollPlayfield->setSelected(true);
	else
		optCollFull->setSelected(true);
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
