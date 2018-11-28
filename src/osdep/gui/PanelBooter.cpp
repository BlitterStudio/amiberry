#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#endif
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"
#include "UaeCheckBox.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "include/memory.h"
#include "disk.h"
#include "uae.h"
#include "autoconf.h"
#include "filesys.h"
#include "blkdev.h"
#include "gui.h"
#include "gui_handling.h"

TCHAR pick_file[MAX_DPATH];

// 
static gcn::Label* lblGameFile;
static gcn::TextField* txtGameFile;
static gcn::Button* cmdSetGameFile;

static gcn::Label* lblSlaveFile;
static gcn::TextField* txtSlaveFile;

static gcn::Label* lblDataPath;
static gcn::TextField* txtDataPath;

static gcn::Label* lblCustomOptions;
static gcn::TextField* txtCustomOptions;
static gcn::Button* cmdSetCustomOptions;


class GameButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char tmp[MAX_DPATH];
		const char* filter[] = {".lha", "\0"};

		if (actionEvent.getSource() == cmdSetGameFile)
		{
			strncpy(tmp, currentDir, MAX_DPATH);
			if (SelectFile("Select WHDLoad Game LHA Archive", tmp, filter))
			{
				//const auto newrom = new AvailableROM();
				//extractFileName(tmp, newrom->Name);
				//removeFileExtension(newrom->Name);
				  strncpy(pick_file, tmp, MAX_DPATH);
				//newrom->ROMType = ROMTYPE_KICK;
				//lstAvailableROMs.push_back(newrom);
                                
                                
				//strncpy(changed_prefs.whdload_file, tmp, sizeof(changed_prefs.whdload_file));
                                
                                //whdload_auto_prefs(pick_file);
				RefreshPanelBooter();
			}
			cmdSetGameFile->requestFocus();
		}

	}
};


static GameButtonActionListener* gameButtonActionListener;



void InitPanelBooter(const struct _ConfigCategory& category)
{
	auto yPos = DISTANCE_BORDER;

	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - (SMALL_BUTTON_WIDTH * 5) - 100 - DISTANCE_NEXT_X;
	//auto yPos = DISTANCE_BORDER;
	//folderButtonActionListener = new FolderButtonActionListener();
        
        
        gameButtonActionListener = new GameButtonActionListener();
        
        
        lblGameFile = new gcn::Label("Name:");
        lblGameFile->setAlignment(gcn::Graphics::RIGHT);
	txtGameFile = new gcn::TextField();
	txtGameFile->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtGameFile->setBackgroundColor(colTextboxBackground);
 	txtGameFile->setId("GameFile");
        
        
        cmdSetGameFile = new gcn::Button("Mount LHA");
	cmdSetGameFile->setId("SetGameFile");
	cmdSetGameFile->setSize(SMALL_BUTTON_WIDTH * 4, SMALL_BUTTON_HEIGHT);
	cmdSetGameFile->setBaseColor(gui_baseCol);
	cmdSetGameFile->addActionListener(gameButtonActionListener);
        
        lblSlaveFile = new gcn::Label("Slave File:");
        lblSlaveFile->setAlignment(gcn::Graphics::RIGHT);
	txtSlaveFile = new gcn::TextField();
	txtSlaveFile->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtSlaveFile->setBackgroundColor(colTextboxBackground);
 	txtSlaveFile->setId("SlaveFile");
        
        lblDataPath = new gcn::Label("Data Path:");
        lblDataPath->setAlignment(gcn::Graphics::RIGHT);    
        
	txtDataPath = new gcn::TextField();
	txtDataPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtDataPath->setBackgroundColor(colTextboxBackground);
 	txtDataPath->setId("DataPath");
        
        lblCustomOptions = new gcn::Label("Custom Options:");
        lblCustomOptions->setAlignment(gcn::Graphics::RIGHT);  
        
	txtCustomOptions = new gcn::TextField();
	txtCustomOptions->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtCustomOptions->setBackgroundColor(colTextboxBackground);
	txtDataPath->setBackgroundColor(colTextboxBackground);
 	txtDataPath->setId("CustomOptions");
        
        cmdSetCustomOptions = new gcn::Button("Set Options");
	cmdSetCustomOptions->setId("SetCustomOptions");
	cmdSetCustomOptions->setSize(SMALL_BUTTON_WIDTH * 4, SMALL_BUTTON_HEIGHT);
	cmdSetCustomOptions->setBaseColor(gui_baseCol);
	//cmdSetCustomOptions->addActionListener(folderButtonActionListener);
        
        
        auto button_x = category.panel->getWidth() - 2 * DISTANCE_BORDER - (SMALL_BUTTON_WIDTH * 4);
        
        
        category.panel->add(lblGameFile, DISTANCE_BORDER, yPos);
	category.panel->add(txtGameFile, DISTANCE_BORDER + 120, yPos);
	category.panel->add(cmdSetGameFile, button_x, yPos);
	yPos += txtGameFile->getHeight() + DISTANCE_NEXT_Y;
        
        category.panel->add(lblSlaveFile, DISTANCE_BORDER, yPos);
	category.panel->add(txtSlaveFile, DISTANCE_BORDER + 120, yPos);
	yPos += lblGameFile->getHeight() + DISTANCE_NEXT_Y;
        
        category.panel->add(lblDataPath, DISTANCE_BORDER, yPos);
	category.panel->add(txtDataPath, DISTANCE_BORDER + 120, yPos);        
	yPos += lblSlaveFile->getHeight() + DISTANCE_NEXT_Y;

        category.panel->add(lblCustomOptions, DISTANCE_BORDER, yPos);
	category.panel->add(txtCustomOptions, DISTANCE_BORDER + 120, yPos);
	category.panel->add(cmdSetCustomOptions, button_x, yPos);
	yPos += txtCustomOptions->getHeight() + DISTANCE_NEXT_Y;

        
	RefreshPanelBooter();
}


void ExitPanelBooter(void)
{
        delete gameButtonActionListener;
        
}


static void AdjustDropDownControls(void)
{

}


void RefreshPanelBooter(void)
{
	char tmp[MAX_DPATH];

	//fetch_rompath(tmp, MAX_DPATH);
	//txtSystemROMs->setText(tmp)
        
}


bool HelpPanelBooter(vector<string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("");
	helptext.emplace_back("");
	helptext.emplace_back("");
	helptext.emplace_back("");
	helptext.emplace_back("");
	helptext.emplace_back("");
	helptext.emplace_back("");
	helptext.emplace_back("");
	return true;
}
