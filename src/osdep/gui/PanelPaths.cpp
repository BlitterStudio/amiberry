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

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "gui.h"
#include "gui_handling.h"

static gcn::Label* lblSystemROMs;
static gcn::TextField* txtSystemROMs;
static gcn::Button* cmdSystemROMs;

static gcn::Label *lblControllersPath;
static gcn::TextField *txtControllersPath;
static gcn::Button *cmdControllersPath;

static gcn::Label* lblConfigPath;
static gcn::TextField* txtConfigPath;
static gcn::Button* cmdConfigPath;

static gcn::Label *lblRetroArchFile;
static gcn::TextField *txtRetroArchFile;
static gcn::Button *cmdRetroArchFile;

static gcn::Button* cmdRescanROMs;

class FolderButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char tmp[MAX_DPATH];

		if (actionEvent.getSource() == cmdSystemROMs)
		{
			fetch_rompath(tmp, MAX_DPATH);
			if (SelectFolder("Folder for System ROMs", tmp))
			{
				set_rompath(tmp);
				saveAdfDir();
				RefreshPanelPaths();
			}
			cmdSystemROMs->requestFocus();
		}
		else if (actionEvent.getSource() == cmdConfigPath)
		{
			fetch_configurationpath(tmp, MAX_DPATH);
			if (SelectFolder("Folder for configuration files", tmp))
			{
				set_configurationpath(tmp);
				saveAdfDir();
				RefreshPanelPaths();
				RefreshPanelConfig();
			}
			cmdConfigPath->requestFocus();
		}
		else if(actionEvent.getSource() == cmdControllersPath)
      {
        fetch_controllerspath(tmp, MAX_DPATH);
        if(SelectFolder("Folder for controller files", tmp))
        {
          set_controllerspath(tmp);
          saveAdfDir();
          RefreshPanelPaths();
        }
        cmdControllersPath->requestFocus();
      } 
      
      else if(actionEvent.getSource() == cmdRetroArchFile)
      {
        const char *filter[] = { "retroarch.cfg", "\0" };
        fetch_retroarchfile(tmp, MAX_DPATH);
        if(SelectFile("Select RetroArch Config File", tmp,filter))
        {
          set_retroarchfile(tmp);
          saveAdfDir();
          RefreshPanelPaths();
        }
        cmdRetroArchFile->requestFocus();
      }
	}
};

static FolderButtonActionListener* folderButtonActionListener;


class RescanROMsButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		RescanROMs();
		import_joysticks();
		RefreshPanelROM();
	}
};

static RescanROMsButtonActionListener* rescanROMsButtonActionListener;


void InitPanelPaths(const struct _ConfigCategory& category)
{
	const int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;
	int yPos = DISTANCE_BORDER;
	folderButtonActionListener = new FolderButtonActionListener();

	lblSystemROMs = new gcn::Label("System ROMs:");
	txtSystemROMs = new gcn::TextField();
	txtSystemROMs->setSize(textFieldWidth, txtSystemROMs->getHeight());
	txtSystemROMs->setBackgroundColor(colTextboxBackground);

	cmdSystemROMs = new gcn::Button("...");
	cmdSystemROMs->setId("SystemROMs");
	cmdSystemROMs->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSystemROMs->setBaseColor(gui_baseCol);
	cmdSystemROMs->addActionListener(folderButtonActionListener);

	lblConfigPath = new gcn::Label("Configuration files:");
	txtConfigPath = new gcn::TextField();
	txtConfigPath->setSize(textFieldWidth, txtConfigPath->getHeight());
	txtConfigPath->setBackgroundColor(colTextboxBackground);

	cmdConfigPath = new gcn::Button("...");
	cmdConfigPath->setId("ConfigPath");
	cmdConfigPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdConfigPath->setBaseColor(gui_baseCol);
	cmdConfigPath->addActionListener(folderButtonActionListener);

	lblControllersPath = new gcn::Label("Controller files:");
	txtControllersPath = new gcn::TextField();
	txtControllersPath->setSize(textFieldWidth, txtControllersPath->getHeight());
	txtControllersPath->setBackgroundColor(colTextboxBackground);

	cmdControllersPath = new gcn::Button("...");
	cmdControllersPath->setId("ControllersPath");
	cmdControllersPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdControllersPath->setBaseColor(gui_baseCol);
	cmdControllersPath->addActionListener(folderButtonActionListener);

	lblRetroArchFile = new gcn::Label("RetroArch configuration file (retroarch.cfg):");
	txtRetroArchFile = new gcn::TextField();
	txtRetroArchFile->setSize(textFieldWidth, txtRetroArchFile->getHeight());
	txtRetroArchFile->setBackgroundColor(colTextboxBackground);

	cmdRetroArchFile = new gcn::Button("...");
	cmdRetroArchFile->setId("RetroArchFile");
	cmdRetroArchFile->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdRetroArchFile->setBaseColor(gui_baseCol);
	cmdRetroArchFile->addActionListener(folderButtonActionListener);

	category.panel->add(lblSystemROMs, DISTANCE_BORDER, yPos);
	yPos += lblSystemROMs->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(txtSystemROMs, DISTANCE_BORDER, yPos);
	category.panel->add(cmdSystemROMs, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtSystemROMs->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblConfigPath, DISTANCE_BORDER, yPos);
	yPos += lblConfigPath->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(txtConfigPath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdConfigPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtConfigPath->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblControllersPath, DISTANCE_BORDER, yPos);
	yPos += lblControllersPath->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(txtControllersPath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdControllersPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtControllersPath->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblRetroArchFile, DISTANCE_BORDER, yPos);
	yPos += lblRetroArchFile->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(txtRetroArchFile, DISTANCE_BORDER, yPos);
	category.panel->add(cmdRetroArchFile, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtRetroArchFile->getHeight() + DISTANCE_NEXT_Y;
	rescanROMsButtonActionListener = new RescanROMsButtonActionListener();

	cmdRescanROMs = new gcn::Button("Rescan Paths");
	cmdRescanROMs->setSize(cmdRescanROMs->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdRescanROMs->setBaseColor(gui_baseCol);
	cmdRescanROMs->setId("RescanROMs");
	cmdRescanROMs->addActionListener(rescanROMsButtonActionListener);

	category.panel->add(cmdRescanROMs, DISTANCE_BORDER, category.panel->getHeight() - BUTTON_HEIGHT - DISTANCE_BORDER);

	RefreshPanelPaths();
}


void ExitPanelPaths()
{
	delete lblSystemROMs;
	delete txtSystemROMs;
	delete cmdSystemROMs;

	delete lblConfigPath;
	delete txtConfigPath;
	delete cmdConfigPath;

	delete lblControllersPath;
	delete txtControllersPath;
	delete cmdControllersPath;

	delete lblRetroArchFile;
	delete txtRetroArchFile;
	delete cmdRetroArchFile;
	delete folderButtonActionListener;

	delete cmdRescanROMs;
	delete rescanROMsButtonActionListener;
}


void RefreshPanelPaths()
{
	char tmp[MAX_DPATH];

	fetch_rompath(tmp, MAX_DPATH);
	txtSystemROMs->setText(tmp);

	fetch_configurationpath(tmp, MAX_DPATH);
	txtConfigPath->setText(tmp);

	fetch_controllerspath(tmp, MAX_DPATH);
	txtControllersPath->setText(tmp);

	fetch_retroarchfile(tmp, MAX_DPATH);
	txtRetroArchFile->setText(tmp); 
}

bool HelpPanelPaths(std::vector<std::string> &helptext)
{
	helptext.clear();
	helptext.push_back("Specify the location of your kickstart roms and the folders where the configurations");
	helptext.push_back("and controller files should be stored. With the button \"...\" you can open a dialog");
	helptext.push_back("to choose the folder.");
	helptext.push_back("");
	helptext.push_back("After changing the location of the kickstart roms, click on \"Rescan\" to refresh");
	helptext.push_back("the list of the available ROMs.");
	return true;
}
