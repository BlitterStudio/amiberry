#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "gui.h"
#include "target.h"
#include "gui_handling.h"

static gcn::Label *lblSystemROMs;
static gcn::TextField *txtSystemROMs;
static gcn::Button *cmdSystemROMs;
static gcn::Label *lblConfigPath;
static gcn::TextField *txtConfigPath;
static gcn::Button *cmdConfigPath;
static gcn::Button *cmdRescanROMs;


class FolderButtonActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      char tmp[MAX_PATH];
      
      if (actionEvent.getSource() == cmdSystemROMs)
      {
        fetch_rompath(tmp, MAX_PATH);
        if(SelectFolder("Folder for System ROMs", tmp))
        {
          set_rompath(tmp);
          RefreshPanelPaths();
        }
        cmdSystemROMs->requestFocus();
      }
      else if(actionEvent.getSource() == cmdConfigPath)
      {
        fetch_configurationpath(tmp, MAX_PATH);
        if(SelectFolder("Folder for configuration files", tmp))
        {
          set_configurationpath(tmp);
          RefreshPanelPaths();
          RefreshPanelConfig();
        }
        cmdConfigPath->requestFocus();
      }
    }
};
static FolderButtonActionListener* folderButtonActionListener;


class RescanROMsButtonActionListener : public gcn::ActionListener
{
  public:
    void action(const gcn::ActionEvent& actionEvent)
    {
      RescanROMs();
      RefreshPanelROM();
    }
};
static RescanROMsButtonActionListener* rescanROMsButtonActionListener;


void InitPanelPaths(const struct _ConfigCategory& category)
{
  int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;
  int yPos = DISTANCE_BORDER;
  folderButtonActionListener = new FolderButtonActionListener();
  
  lblSystemROMs = new gcn::Label("System ROMs:");
  lblSystemROMs->setSize(120, LABEL_HEIGHT);
  txtSystemROMs = new gcn::TextField();
  txtSystemROMs->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
  txtSystemROMs->setEnabled(false);
  cmdSystemROMs = new gcn::Button("...");
  cmdSystemROMs->setId("SystemROMs");
  cmdSystemROMs->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
  cmdSystemROMs->setBaseColor(gui_baseCol);
  cmdSystemROMs->addActionListener(folderButtonActionListener);

  lblConfigPath = new gcn::Label("Configuration files:");
  lblConfigPath->setSize(120, LABEL_HEIGHT);
  txtConfigPath = new gcn::TextField();
  txtConfigPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
  txtConfigPath->setEnabled(false);
  cmdConfigPath = new gcn::Button("...");
  cmdConfigPath->setId("ConfigPath");
  cmdConfigPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
  cmdConfigPath->setBaseColor(gui_baseCol);
  cmdConfigPath->addActionListener(folderButtonActionListener);
  
  category.panel->add(lblSystemROMs, DISTANCE_BORDER, yPos);
  yPos += lblSystemROMs->getHeight();
  category.panel->add(txtSystemROMs, DISTANCE_BORDER, yPos);
  category.panel->add(cmdSystemROMs, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
  yPos += txtSystemROMs->getHeight() + DISTANCE_NEXT_Y;
  
  category.panel->add(lblConfigPath, DISTANCE_BORDER, yPos);
  yPos += lblConfigPath->getHeight();
  category.panel->add(txtConfigPath, DISTANCE_BORDER, yPos);
  category.panel->add(cmdConfigPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
  yPos += txtConfigPath->getHeight() + DISTANCE_NEXT_Y;

  rescanROMsButtonActionListener = new RescanROMsButtonActionListener();

  cmdRescanROMs = new gcn::Button("Rescan ROMs");
  cmdRescanROMs->setSize(120, BUTTON_HEIGHT);
  cmdRescanROMs->setBaseColor(gui_baseCol);
  cmdRescanROMs->setId("RescanROMs");
  cmdRescanROMs->addActionListener(rescanROMsButtonActionListener);
  
  category.panel->add(cmdRescanROMs, DISTANCE_BORDER, category.panel->getHeight() - BUTTON_HEIGHT - DISTANCE_BORDER);

  RefreshPanelPaths();
}


void ExitPanelPaths(void)
{
  delete lblSystemROMs;
  delete txtSystemROMs;
  delete cmdSystemROMs;

  delete lblConfigPath;
  delete txtConfigPath;
  delete cmdConfigPath;
  
  delete folderButtonActionListener;

  delete cmdRescanROMs;
  delete rescanROMsButtonActionListener;
}


void RefreshPanelPaths(void)
{
  char tmp[MAX_PATH];
  
  fetch_rompath(tmp, MAX_PATH);
  txtSystemROMs->setText(tmp);
  
  fetch_configurationpath(tmp, MAX_PATH);
  txtConfigPath->setText(tmp);
}
