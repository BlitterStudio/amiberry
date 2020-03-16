#include <stdbool.h>

#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeCheckBox.hpp"
#include "UaeDropDown.hpp"

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "gui_handling.h"

static gcn::Window* grpChipset;
static gcn::UaeRadioButton* optOCS;
static gcn::UaeRadioButton* optECSAgnus;
static gcn::UaeRadioButton* optECS;
static gcn::UaeRadioButton* optAGA;
static gcn::UaeCheckBox* chkNTSC;
static gcn::Label* lblChipset;
static gcn::UaeDropDown* cboChipset;
static gcn::Window* grpBlitter;
static gcn::UaeRadioButton* optBlitNormal;
static gcn::UaeRadioButton* optBlitImmed;
static gcn::UaeRadioButton* optBlitWait;
static gcn::Window* grpCopper;
static gcn::UaeCheckBox* chkFastCopper;
static gcn::Window* grpCollisionLevel;
static gcn::UaeRadioButton* optCollNone;
static gcn::UaeRadioButton* optCollSprites;
static gcn::UaeRadioButton* optCollPlayfield;
static gcn::UaeRadioButton* optCollFull;

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
		else if (actionEvent.getSource() == optECS)
			changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
		else
			changed_prefs.chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	}
};

static ChipsetButtonActionListener* chipsetButtonActionListener;


class NTSCButtonActionListener : public gcn::ActionListener
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

	optOCS = new gcn::UaeRadioButton("OCS", "radiochipsetgroup");
	optOCS->addActionListener(chipsetButtonActionListener);

	optECSAgnus = new gcn::UaeRadioButton("ECS Agnus", "radiochipsetgroup");
	optECSAgnus->addActionListener(chipsetButtonActionListener);

	optECS = new gcn::UaeRadioButton("Full ECS", "radiochipsetgroup");
	optECS->addActionListener(chipsetButtonActionListener);

	optAGA = new gcn::UaeRadioButton("AGA", "radiochipsetgroup");
	optAGA->addActionListener(chipsetButtonActionListener);

	chkNTSC = new gcn::UaeCheckBox("NTSC");
	chkNTSC->addActionListener(ntscButtonActionListener);

	lblChipset = new gcn::Label("Extra:");
	lblChipset->setAlignment(gcn::Graphics::RIGHT);
	cboChipset = new gcn::UaeDropDown(&chipsetList);
	cboChipset->setSize(75, cboChipset->getHeight());
	cboChipset->setBaseColor(gui_baseCol);
	cboChipset->setBackgroundColor(colTextboxBackground);
	cboChipset->setId("ChipsetExtra");
	cboChipset->addActionListener(chipsetActionListener);

	grpChipset = new gcn::Window("Chipset");
	grpChipset->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpChipset->add(optOCS, 5, 10);
	grpChipset->add(optECSAgnus, 5, 40);
	grpChipset->add(optECS, 5, 70);
	grpChipset->add(optAGA, 5, 100);
	grpChipset->add(chkNTSC, 5, 140);
	grpChipset->add(lblChipset, 145, 10);
	grpChipset->add(cboChipset, 145 + lblChipset->getWidth() + 8, 10);

	grpChipset->setMovable(false);
	grpChipset->setSize(optOCS->getWidth() + 125 + lblChipset->getWidth() + cboChipset->getWidth(), 185);
	grpChipset->setBaseColor(gui_baseCol);

	category.panel->add(grpChipset);

	blitterButtonActionListener = new BlitterButtonActionListener();

	optBlitNormal = new gcn::UaeRadioButton("Normal", "radiocblittergroup");
	optBlitNormal->setId("BlitNormal");
	optBlitNormal->addActionListener(blitterButtonActionListener);

	optBlitImmed = new gcn::UaeRadioButton("Immediate", "radiocblittergroup");
	optBlitImmed->addActionListener(blitterButtonActionListener);

	optBlitWait = new gcn::UaeRadioButton("Wait for blit.", "radiocblittergroup");
	optBlitWait->setId("BlitWait");
	optBlitWait->addActionListener(blitterButtonActionListener);

	grpBlitter = new gcn::Window("Blitter");
	grpBlitter->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpBlitter->add(optBlitNormal, 5, 10);
	grpBlitter->add(optBlitImmed, 5, 40);
	grpBlitter->add(optBlitWait, 5, 70);
	grpBlitter->setMovable(false);
	grpBlitter->setSize(optBlitWait->getWidth() + DISTANCE_BORDER, 115);
	grpBlitter->setBaseColor(gui_baseCol);

	category.panel->add(grpBlitter);

	fastCopperActionListener = new FastCopperActionListener();

	chkFastCopper = new gcn::UaeCheckBox("Fast copper");
	chkFastCopper->addActionListener(fastCopperActionListener);

	grpCopper = new gcn::Window("Copper");
	grpCopper->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_NEXT_X,
	                       grpBlitter->getY() + grpBlitter->getHeight() + DISTANCE_NEXT_Y);
	grpCopper->add(chkFastCopper, 5, 10);
	grpCopper->setMovable(false);
	grpCopper->setSize(chkFastCopper->getWidth() + DISTANCE_BORDER, 55);
	grpCopper->setBaseColor(gui_baseCol);

	category.panel->add(grpCopper);

	collisionButtonActionListener = new CollisionButtonActionListener();

	optCollNone = new gcn::UaeRadioButton("None", "radioccollisiongroup");
	optCollNone->setId("CollNone");
	optCollNone->addActionListener(collisionButtonActionListener);

	optCollSprites = new gcn::UaeRadioButton("Sprites only", "radioccollisiongroup");
	optCollSprites->addActionListener(collisionButtonActionListener);

	optCollPlayfield = new gcn::UaeRadioButton("Sprites and Sprites vs. Playfield", "radioccollisiongroup");
	optCollPlayfield->setId("CollPlay");
	optCollPlayfield->addActionListener(collisionButtonActionListener);

	optCollFull = new gcn::UaeRadioButton("Full (rarely needed)", "radioccollisiongroup");
	optCollFull->setId("CollFull");
	optCollFull->addActionListener(collisionButtonActionListener);

	grpCollisionLevel = new gcn::Window("Collision Level");
	grpCollisionLevel->setPosition(DISTANCE_BORDER, DISTANCE_BORDER + grpChipset->getHeight() + DISTANCE_NEXT_Y);
	grpCollisionLevel->add(optCollNone, 5, 10);
	grpCollisionLevel->add(optCollSprites, 5, 40);
	grpCollisionLevel->add(optCollPlayfield, 5, 70);
	grpCollisionLevel->add(optCollFull, 5, 100);
	grpCollisionLevel->setMovable(false);
	grpCollisionLevel->setSize(optCollPlayfield->getWidth() + DISTANCE_BORDER, 145);
	grpCollisionLevel->setBaseColor(gui_baseCol);

	category.panel->add(grpCollisionLevel);

	RefreshPanelChipset();
}


void ExitPanelChipset()
{
	delete optOCS;
	delete optECSAgnus;
	delete optECS;
	delete optAGA;
	delete chkNTSC;
	delete lblChipset;
	delete cboChipset;
	delete grpChipset;
	delete chipsetButtonActionListener;
	delete ntscButtonActionListener;
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
	else if (changed_prefs.chipset_mask == (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE))
		optECS->setSelected(true);
	else if (changed_prefs.chipset_mask == (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA))
		optAGA->setSelected(true);

	chkNTSC->setSelected(changed_prefs.ntscmode);

	if (changed_prefs.immediate_blits)
		optBlitImmed->setSelected(true);
	else if (changed_prefs.waiting_blits)
		optBlitWait->setSelected(true);
	else
		optBlitNormal->setSelected(true);

	chkFastCopper->setSelected(changed_prefs.fast_copper);

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
