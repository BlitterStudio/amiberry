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

#include <stdio.h>

#include <libxml/tree.h>
#include <fstream>

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "gui_handling.h"
#include "zfile.h"

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
static gcn::Button* cmdDownloadXML;


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
				save_amiberry_settings();
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
				save_amiberry_settings();
				RefreshPanelPaths();
				RefreshPanelConfig();
			}
			cmdConfigPath->requestFocus();
		}
		else if (actionEvent.getSource() == cmdControllersPath)
		{
			fetch_controllerspath(tmp, MAX_DPATH);
			if (SelectFolder("Folder for controller files", tmp))
			{
				set_controllerspath(tmp);
				save_amiberry_settings();
				RefreshPanelPaths();
			}
			cmdControllersPath->requestFocus();
		}

		else if (actionEvent.getSource() == cmdRetroArchFile)
		{
			const char *filter[] = { "retroarch.cfg", "\0" };
			fetch_retroarchfile(tmp, MAX_DPATH);
			if (SelectFile("Select RetroArch Config File", tmp, filter))
			{
				set_retroarchfile(tmp);
				save_amiberry_settings();
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
		SymlinkROMs();

		import_joysticks();
		RefreshPanelInput();
		RefreshPanelCustom();
		RefreshPanelROM();

		ShowMessage("Rescan Paths", "Scan complete", "", "Ok", "");
		cmdRescanROMs->requestFocus();
	}
};

static RescanROMsButtonActionListener* rescanROMsButtonActionListener;

int date_cmp(const char *d1, const char *d2)
{
	// compare years
	auto rc = strncmp(d1 + 0, d2 + 0, 4);
    if (rc != 0)
        return rc;

    auto n = 0;
    
    for (auto m = 0; m < 5; ++m)
    {
        switch(m) {
        case 0: n = 5 ; break;   // compare months
        case 1: n = 8 ; break;   // compare days
        case 2: n = 14; break;   // compare hours
        case 3: n = 17; break;   // compare minutes
        case 4: n = 20; break;   // compare seconds
        }
    
        rc = strncmp(d1 + n, d2 + n, 2);
        if (rc != 0)
            return rc;
    } 
            
    return strncmp(d1, d2, 2);
}

static xmlNode* get_node(xmlNode* node, const char* name)
{
	for (auto curr_node = node; curr_node; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE && strcmp(reinterpret_cast<const char *>(curr_node->name), name) == 0)
			return curr_node->children;
	}
	return nullptr;
}

void copy_file( const char* srce_file, const char* dest_file )
{
    std::ifstream srce( srce_file, std::ios::binary ) ;
    std::ofstream dest( dest_file, std::ios::binary ) ;
    dest << srce.rdbuf() ;
}

class DownloadXMLButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char original_date[MAX_DPATH] = "2000-01-01 at 00:00:01\n";
		char updated_date[MAX_DPATH] = "2000-01-01 at 00:00:01\n";

		char xml_path[MAX_DPATH];
		char xml2_path[MAX_DPATH];

		snprintf(xml_path, MAX_DPATH, "%s/whdboot/game-data/whdload_db.xml", start_path_data);
		snprintf(xml2_path, MAX_DPATH, "/tmp/whdload_db.xml");

		write_log("Checking original %s for date...\n", xml_path);

		if (zfile_exists(xml_path)) // use local XML 
		{
			const auto doc = xmlParseFile(xml_path);
			const auto root_element = xmlDocGetRootElement(doc);
			_stprintf(original_date, "%s", reinterpret_cast<const char*>(xmlGetProp(root_element, reinterpret_cast<const xmlChar *>("timestamp"))));
			write_log(" ... Date from original ...  %s\n", original_date);
		}
		else
			write_log("\n");

		// download the updated XML to /tmp/               
		auto afile = popen("wget -np -nv -O /tmp/whdload_db.xml https://raw.githubusercontent.com/HoraceAndTheSpider/Amiberry-XML-Builder/master/whdload_db.xml", "r");
		pclose(afile);

		// do i need to pause here??

		// check the downloaded file's 
		const auto doc = xmlParseFile(xml2_path);
		const auto root_element = xmlDocGetRootElement(doc);

		if (zfile_exists(xml2_path)) // use downloaded XML
		{
			_stprintf(updated_date, "%s", reinterpret_cast<const char*>(xmlGetProp(root_element, reinterpret_cast<const xmlChar *>("timestamp"))));
		}

		//do_download     
		write_log("Checking downloaded whdload_db.xml for date... \n");
		write_log(" ...Date from download ...  %s\n", updated_date);

		// do the compare
		if (date_cmp(original_date, updated_date) < 0)
		{
			remove(xml_path);
			copy_file("/tmp/whdload_db.xml", xml_path);
			ShowMessage("XML Downloader", "Updated XML downloaded.", "", "Ok", "");
		}
		else if (date_cmp(original_date, updated_date) > 0)
		{
			ShowMessage("XML Downloader", "Local XML does not require update.", "", "Ok", "");
		}
		else
		{
			ShowMessage("XML Downloader", "Local XML does not require update.", "", "Ok", "");
		}

		// show message depending on what was done		
		cmdDownloadXML->requestFocus();
	}
};

static DownloadXMLButtonActionListener* downloadXMLButtonActionListener;

void InitPanelPaths(const struct _ConfigCategory& category)
{
	const auto textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;
	auto yPos = DISTANCE_BORDER;
	folderButtonActionListener = new FolderButtonActionListener();

	lblSystemROMs = new gcn::Label("System ROMs:");
	txtSystemROMs = new gcn::TextField();
	txtSystemROMs->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtSystemROMs->setBackgroundColor(colTextboxBackground);

	cmdSystemROMs = new gcn::Button("...");
	cmdSystemROMs->setId("SystemROMs");
	cmdSystemROMs->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSystemROMs->setBaseColor(gui_baseCol);
	cmdSystemROMs->addActionListener(folderButtonActionListener);

	lblConfigPath = new gcn::Label("Configuration files:");
	txtConfigPath = new gcn::TextField();
	txtConfigPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtConfigPath->setBackgroundColor(colTextboxBackground);

	cmdConfigPath = new gcn::Button("...");
	cmdConfigPath->setId("ConfigPath");
	cmdConfigPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdConfigPath->setBaseColor(gui_baseCol);
	cmdConfigPath->addActionListener(folderButtonActionListener);

	lblControllersPath = new gcn::Label("Controller files:");
	txtControllersPath = new gcn::TextField();
	txtControllersPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtControllersPath->setBackgroundColor(colTextboxBackground);

	cmdControllersPath = new gcn::Button("...");
	cmdControllersPath->setId("ControllersPath");
	cmdControllersPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdControllersPath->setBaseColor(gui_baseCol);
	cmdControllersPath->addActionListener(folderButtonActionListener);

	lblRetroArchFile = new gcn::Label("RetroArch configuration file (retroarch.cfg):");
	txtRetroArchFile = new gcn::TextField();
	txtRetroArchFile->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
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

	downloadXMLButtonActionListener = new DownloadXMLButtonActionListener();
	cmdDownloadXML = new gcn::Button("Update WHDLoad Database/XML");
	cmdDownloadXML->setSize(cmdDownloadXML->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdDownloadXML->setBaseColor(gui_baseCol);
	cmdDownloadXML->setId("DownloadXML");
	cmdDownloadXML->addActionListener(downloadXMLButtonActionListener);

	category.panel->add(cmdRescanROMs, DISTANCE_BORDER, category.panel->getHeight() - BUTTON_HEIGHT - DISTANCE_BORDER);
	category.panel->add(cmdDownloadXML, DISTANCE_BORDER + cmdRescanROMs->getWidth() + 20, category.panel->getHeight() - BUTTON_HEIGHT - DISTANCE_BORDER);

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
	delete cmdDownloadXML;
	delete rescanROMsButtonActionListener;
	delete downloadXMLButtonActionListener;
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
	helptext.emplace_back("Specify the location of your kickstart roms and the folders where the configurations");
	helptext.emplace_back("and controller files should be stored. With the button \"...\" you can open a dialog");
	helptext.emplace_back("to choose the folder.");
	helptext.emplace_back("");
	helptext.emplace_back("After changing the location of the kickstart roms, click on \"Rescan\" to refresh");
	helptext.emplace_back("the list of the available ROMs.");
	return true;
}
