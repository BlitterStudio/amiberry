#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "gui.h"
#include "custom.h"
#include "target.h"
#include "gui_handling.h"


static gcn::Window *grpChipset;
static gcn::UaeRadioButton* optOCS;
static gcn::UaeRadioButton* optECSAgnus;
static gcn::UaeRadioButton* optECS;
static gcn::UaeRadioButton* optAGA;
static gcn::UaeCheckBox* chkNTSC;
static gcn::Window *grpBlitter;
static gcn::UaeRadioButton* optBlitNormal;
static gcn::UaeRadioButton* optBlitImmed;
static gcn::Window *grpCopper;
static gcn::UaeCheckBox* chkFastCopper;
static gcn::Window *grpCollisionLevel;
static gcn::UaeRadioButton* optCollNone;
static gcn::UaeRadioButton* optCollSprites;
static gcn::UaeRadioButton* optCollPlayfield;
static gcn::UaeRadioButton* optCollFull;


class ChipsetButtonActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
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
    void action(const gcn::ActionEvent& actionEvent)
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
    }
};
static NTSCButtonActionListener* ntscButtonActionListener;


class FastCopperActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
	    changed_prefs.fast_copper = chkFastCopper->isSelected();
    }
};
static FastCopperActionListener* fastCopperActionListener;


class BlitterButtonActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      changed_prefs.immediate_blits = optBlitImmed->isSelected();
    }
};
static BlitterButtonActionListener* blitterButtonActionListener;


class CollisionButtonActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
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

	grpChipset = new gcn::Window("Chipset");
	grpChipset->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpChipset->add(optOCS, 5, 10);
	grpChipset->add(optECSAgnus, 5, 40);
	grpChipset->add(optECS, 5, 70);
	grpChipset->add(optAGA, 5, 100);
	grpChipset->add(chkNTSC, 5, 140);
	grpChipset->setMovable(false);
	grpChipset->setSize(120, 185);
  grpChipset->setBaseColor(gui_baseCol);
  
  category.panel->add(grpChipset);

  blitterButtonActionListener = new BlitterButtonActionListener();

	optBlitNormal = new gcn::UaeRadioButton("Normal", "radiocblittergroup");
	optBlitNormal->setId("BlitNormal");
	optBlitNormal->addActionListener(blitterButtonActionListener);

	optBlitImmed = new gcn::UaeRadioButton("Immediate", "radiocblittergroup");
	optBlitImmed->addActionListener(blitterButtonActionListener);

	grpBlitter = new gcn::Window("Blitter");
	grpBlitter->setPosition(DISTANCE_BORDER + grpChipset->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpBlitter->add(optBlitNormal, 5, 10);
	grpBlitter->add(optBlitImmed, 5, 40);
	grpBlitter->setMovable(false);
	grpBlitter->setSize(120, 85);
  grpBlitter->setBaseColor(gui_baseCol);
  
  category.panel->add(grpBlitter);
  
  fastCopperActionListener = new FastCopperActionListener();

	chkFastCopper = new gcn::UaeCheckBox("Fast copper");
  chkFastCopper->addActionListener(fastCopperActionListener);

	grpCopper = new gcn::Window("Copper");
	grpCopper->setPosition(grpBlitter->getX() + grpBlitter->getWidth() + DISTANCE_NEXT_X, DISTANCE_BORDER);
	grpCopper->add(chkFastCopper, 5, 10);
	grpCopper->setMovable(false);
	grpCopper->setSize(120, 55);
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
	grpCollisionLevel->setSize(250, 145);
  grpCollisionLevel->setBaseColor(gui_baseCol);
  
  category.panel->add(grpCollisionLevel);

  RefreshPanelChipset();
}


void ExitPanelChipset(void)
{
  delete optOCS;
  delete optECSAgnus;
  delete optECS;
  delete optAGA;
  delete chkNTSC;
  delete grpChipset;
  delete chipsetButtonActionListener;
  delete ntscButtonActionListener;

  delete optBlitNormal;
  delete optBlitImmed;
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


void RefreshPanelChipset(void)
{
	if (changed_prefs.chipset_mask == 0)
    optOCS->setSelected(true);
	else if (changed_prefs.chipset_mask == CSMASK_ECS_AGNUS)
    optECSAgnus->setSelected(true);
  else if (changed_prefs.chipset_mask == (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE))
    optECS->setSelected(true);
  else if (changed_prefs.chipset_mask == (CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA))
    optAGA->setSelected(true);
  
  chkNTSC->setSelected(changed_prefs.ntscmode);
  
  if(changed_prefs.immediate_blits)
    optBlitImmed->setSelected(true);
  else
    optBlitNormal->setSelected(true);
  
  chkFastCopper->setSelected(changed_prefs.fast_copper);
  
  if(changed_prefs.collision_level == 0)
    optCollNone->setSelected(true);
  else if(changed_prefs.collision_level == 1)
    optCollSprites->setSelected(true);
  else if(changed_prefs.collision_level == 2)
    optCollPlayfield->setSelected(true);
  else
    optCollFull->setSelected(true);
}
