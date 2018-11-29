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

#include "fsdb.h"
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

//  DIMITRIS I AM REALLY SORRY - THIS XML READING CODE IS A REPEAT FROM AMIBERRY_WHDBOOTER.CPP
//    I AM SURE WE CAN OPTIMISE BY SHARING IT IN A HEADER BUT I AM TOO LAZY - DOM

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

static gcn::Button* cmdReadDB;



static xmlNode* get_node(xmlNode* node, const char* name)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), name) == 0)
			return curr_node->children;
	}
	return nullptr;
}

static bool get_value(xmlNode* node, const char* key, char* value, int max_size)
{
	auto result = false;

	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), key) == 0)
		{
			const auto content = xmlNodeGetContent(curr_node);
			if (content != nullptr)
			{
				strncpy(value, reinterpret_cast<char *>(content), max_size);
				xmlFree(content);
				result = true;
			}
			break;
		}
	}
	return result;
}



class BooterActionListener : public gcn::ActionListener
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
				 //strncpy(pick_file, tmp, MAX_DPATH); 
                                 
                            
                            //	const auto pick_file = my_getfilepart(tmp);
                            //    TCHAR game_name[MAX_DPATH];

                                
                               // extractFileName(filepath, last_loaded_config);
                               // extractFileName(filepath, game_name);
                           //     removeFileExtension(game_name);
        
        
                                 //const auto pick_file = my_getfilepart(tmp);
                                 auto pick_file = my_getfilepart(tmp);

                                 txtGameFile->setText(pick_file);
                                          
                                 RefreshPanelBooter();
			}
			cmdSetGameFile->requestFocus();
		}
                else if (actionEvent.getSource() == cmdReadDB)
		{ 
                    auto fish = ShowMessage("Some information goes here","","Other information goes here ... ok?? ", "Ok", "");  
                    cmdReadDB->requestFocus();
                    
                    // just some test stuff
                    // scan the XML for this game
                    // find the subpath

                    // find all the game options
                    
                    // find all the available slaves (put this in dropdown)
                        // set the default slave
                        // get the data path option for this slave
                }
	}
};

static BooterActionListener* booterActionListener;




void InitPanelBooter(const struct _ConfigCategory& category)
{
	auto yPos = DISTANCE_BORDER;

	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - (SMALL_BUTTON_WIDTH * 5) - 100 - DISTANCE_NEXT_X;
	//auto yPos = DISTANCE_BORDER;
	//folderButtonActionListener = new FolderButtonActionListener();
        
        
        booterActionListener = new BooterActionListener();
        
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
	cmdSetGameFile->addActionListener(booterActionListener);
        
        lblSlaveFile = new gcn::Label("Slave File:");
        lblSlaveFile->setAlignment(gcn::Graphics::RIGHT);
	txtSlaveFile = new gcn::TextField();
 	txtSlaveFile->setId("SlaveFile");
	txtSlaveFile->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtSlaveFile->setBackgroundColor(colTextboxBackground);
        
        lblDataPath = new gcn::Label("Data Path:");
        lblDataPath->setAlignment(gcn::Graphics::RIGHT);    
        
	txtDataPath = new gcn::TextField();
 	txtDataPath->setId("DataPath");
	txtDataPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtDataPath->setBackgroundColor(colTextboxBackground);
        
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
        
        
        cmdReadDB = new gcn::Button("Read XML");
	cmdReadDB->setId("ReadXML");
	cmdReadDB->setSize(SMALL_BUTTON_WIDTH * 4, SMALL_BUTTON_HEIGHT);
	cmdReadDB->setBaseColor(gui_baseCol);
        cmdReadDB->addActionListener(booterActionListener);      
        
        
        
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

        
	category.panel->add(cmdReadDB, button_x, yPos);
        


        
	RefreshPanelBooter();
}


void ExitPanelBooter(void)
{
        delete booterActionListener;
        
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
