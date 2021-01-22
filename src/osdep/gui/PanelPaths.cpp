#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "gui_handling.h"

static gcn::Label* lblSystemROMs;
static gcn::TextField* txtSystemROMs;
static gcn::Button* cmdSystemROMs;

static gcn::Label* lblControllersPath;
static gcn::TextField* txtControllersPath;
static gcn::Button* cmdControllersPath;

static gcn::Label* lblConfigPath;
static gcn::TextField* txtConfigPath;
static gcn::Button* cmdConfigPath;

static gcn::Label* lblRetroArchFile;
static gcn::TextField* txtRetroArchFile;
static gcn::Button* cmdRetroArchFile;

static gcn::CheckBox* chkEnableLogging;
static gcn::Label* lblLogfilePath;
static gcn::TextField* txtLogfilePath;
static gcn::Button* cmdLogfilePath;

static gcn::Button* cmdRescanROMs;
static gcn::Button* cmdDownloadXML;
static gcn::Button* cmdDownloadCtrlDb;

class FolderButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char tmp[MAX_DPATH];

		if (actionEvent.getSource() == cmdSystemROMs)
		{
			get_rom_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for System ROMs", tmp))
			{
				set_rom_path(tmp);
				save_amiberry_settings();
				RefreshPanelPaths();
			}
			cmdSystemROMs->requestFocus();
		}
		else if (actionEvent.getSource() == cmdConfigPath)
		{
			get_configuration_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for configuration files", tmp))
			{
				set_configuration_path(tmp);
				save_amiberry_settings();
				RefreshPanelPaths();
				RefreshPanelConfig();
			}
			cmdConfigPath->requestFocus();
		}
		else if (actionEvent.getSource() == cmdControllersPath)
		{
			get_controllers_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for controller files", tmp))
			{
				set_controllers_path(tmp);
				save_amiberry_settings();
				RefreshPanelPaths();
			}
			cmdControllersPath->requestFocus();
		}

		else if (actionEvent.getSource() == cmdRetroArchFile)
		{
			const char* filter[] = {"retroarch.cfg", "\0"};
			get_retroarch_file(tmp, MAX_DPATH);
			if (SelectFile("Select RetroArch Config File", tmp, filter))
			{
				set_retroarch_file(tmp);
				save_amiberry_settings();
				RefreshPanelPaths();
			}
			cmdRetroArchFile->requestFocus();
		}

		else if (actionEvent.getSource() == cmdLogfilePath)
		{
			const char* filter[] = { "amiberry.log", "\0" };
			get_logfile_path(tmp, MAX_DPATH);
			if (SelectFile("Select Amiberry Log file", tmp, filter, true))
			{
				set_logfile_path(tmp);
				save_amiberry_settings();
				RefreshPanelPaths();
			}
			cmdLogfilePath->requestFocus();
		}
	}
};

static FolderButtonActionListener* folderButtonActionListener;

class EnableLoggingActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		set_logfile_enabled(chkEnableLogging->isSelected());
		logging_init();
		RefreshPanelPaths();
	}
};

static EnableLoggingActionListener* enableLoggingActionListener;

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

		ShowMessage("Rescan Paths", "Scan complete,", "ROMs updated, Joysticks (re)initialized, Symlinks recreated.", "Ok", "");
		cmdRescanROMs->requestFocus();
	}
};

static RescanROMsButtonActionListener* rescanROMsButtonActionListener;

int date_cmp(const char* d1, const char* d2)
{
	// compare years
	auto rc = strncmp(d1 + 0, d2 + 0, 4);
	if (rc != 0)
		return rc;

	auto n = 0;

	for (auto m = 0; m < 5; ++m)
	{
		switch (m)
		{
		case 0: n = 5;
			break; // compare months
		case 1: n = 8;
			break; // compare days
		case 2: n = 14;
			break; // compare hours
		case 3: n = 17;
			break; // compare minutes
		case 4: n = 20;
			break; // compare seconds
		}

		rc = strncmp(d1 + n, d2 + n, 2);
		if (rc != 0)
			return rc;
	}

	return strncmp(d1, d2, 2);
}

class DownloadXMLButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char destination[MAX_DPATH];

		//  download WHDLoad executable
		snprintf(destination, MAX_DPATH, "%s/whdboot/WHDLoad", start_path_data);
		write_log("Downloading %s ...\n", destination);
		download_file("https://github.com/midwan/amiberry/blob/master/whdboot/WHDLoad?raw=true", destination);

		//  download boot-data.zip
		snprintf(destination, MAX_DPATH, "%s/whdboot/boot-data.zip", start_path_data);
		write_log("Downloading %s ...\n", destination);
		download_file("https://github.com/midwan/amiberry/blob/master/whdboot/boot-data.zip?raw=true", destination);

		// download kickstart RTB files for maximum compatibility
		download_rtb("kick33180.A500.RTB");
		download_rtb("kick34005.A500.RTB");
		download_rtb("kick40063.A600.RTB");
		download_rtb("kick40068.A1200.RTB");
		download_rtb("kick40068.A4000.RTB");

		snprintf(destination, MAX_DPATH, "%s/whdboot/game-data/whdload_db.xml", start_path_data);
		write_log("Downloading %s ...\n", destination);
		const auto result = download_file("https://github.com/HoraceAndTheSpider/Amiberry-XML-Builder/blob/master/whdload_db.xml?raw=true", destination);

		if (result)
			ShowMessage("XML Downloader", "Updated XML downloaded.", "", "Ok", "");
		else
			ShowMessage("XML Downloader", "Failed to download files!", "Please check the log for more details.", "Ok", "");
		
		cmdDownloadXML->requestFocus();
	}
};

static DownloadXMLButtonActionListener* downloadXMLButtonActionListener;

class DownloadControllerDbActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char destination[MAX_DPATH];
		get_configuration_path(destination, MAX_DPATH);
		strcat(destination, "gamecontrollerdb.txt");
		write_log("Downloading % ...\n", destination);
		const auto* const url = "https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt";
		const auto result = download_file(url, destination);

		if (result)
		{
			import_joysticks();
			ShowMessage("Game Controllers DB", "Latest version of Game Controllers DB downloaded.", "", "Ok", "");
		}
		else
			ShowMessage("Game Controllers DB", "Failed to download file!", "Please check the log for more information", "Ok", "");

		cmdDownloadCtrlDb->requestFocus();
	}
};
static DownloadControllerDbActionListener* downloadControllerDbActionListener;

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
	cmdSystemROMs->setId("cmdSystemROMs");
	cmdSystemROMs->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSystemROMs->setBaseColor(gui_baseCol);
	cmdSystemROMs->addActionListener(folderButtonActionListener);

	lblConfigPath = new gcn::Label("Configuration files:");
	txtConfigPath = new gcn::TextField();
	txtConfigPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtConfigPath->setBackgroundColor(colTextboxBackground);

	cmdConfigPath = new gcn::Button("...");
	cmdConfigPath->setId("cmdConfigPath");
	cmdConfigPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdConfigPath->setBaseColor(gui_baseCol);
	cmdConfigPath->addActionListener(folderButtonActionListener);

	lblControllersPath = new gcn::Label("Controller files:");
	txtControllersPath = new gcn::TextField();
	txtControllersPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtControllersPath->setBackgroundColor(colTextboxBackground);

	cmdControllersPath = new gcn::Button("...");
	cmdControllersPath->setId("cmdControllersPath");
	cmdControllersPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdControllersPath->setBaseColor(gui_baseCol);
	cmdControllersPath->addActionListener(folderButtonActionListener);

	lblRetroArchFile = new gcn::Label("RetroArch configuration file (retroarch.cfg):");
	txtRetroArchFile = new gcn::TextField();
	txtRetroArchFile->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtRetroArchFile->setBackgroundColor(colTextboxBackground);

	cmdRetroArchFile = new gcn::Button("...");
	cmdRetroArchFile->setId("cmdRetroArchFile");
	cmdRetroArchFile->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdRetroArchFile->setBaseColor(gui_baseCol);
	cmdRetroArchFile->addActionListener(folderButtonActionListener);

	enableLoggingActionListener = new EnableLoggingActionListener();
	chkEnableLogging = new gcn::CheckBox("Enable logging", true);
	chkEnableLogging->setId("chkEnableLogging");
	chkEnableLogging->addActionListener(enableLoggingActionListener);
	
	lblLogfilePath = new gcn::Label("Amiberry logfile path:");
	txtLogfilePath = new gcn::TextField();
	txtLogfilePath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtLogfilePath->setBackgroundColor(colTextboxBackground);

	cmdLogfilePath = new gcn::Button("...");
	cmdLogfilePath->setId("cmdLogfilePath");
	cmdLogfilePath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdLogfilePath->setBaseColor(gui_baseCol);
	cmdLogfilePath->addActionListener(folderButtonActionListener);
	
	category.panel->add(lblSystemROMs, DISTANCE_BORDER, yPos);
	yPos += lblSystemROMs->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtSystemROMs, DISTANCE_BORDER, yPos);
	category.panel->add(cmdSystemROMs, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtSystemROMs->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblConfigPath, DISTANCE_BORDER, yPos);
	yPos += lblConfigPath->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtConfigPath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdConfigPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtConfigPath->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblControllersPath, DISTANCE_BORDER, yPos);
	yPos += lblControllersPath->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtControllersPath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdControllersPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtControllersPath->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblRetroArchFile, DISTANCE_BORDER, yPos);
	yPos += lblRetroArchFile->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtRetroArchFile, DISTANCE_BORDER, yPos);
	category.panel->add(cmdRetroArchFile, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);

	yPos += txtRetroArchFile->getHeight() + DISTANCE_NEXT_Y * 2;

	category.panel->add(chkEnableLogging, DISTANCE_BORDER, yPos);
	yPos += chkEnableLogging->getHeight() + DISTANCE_NEXT_Y;
	category.panel->add(lblLogfilePath, DISTANCE_BORDER, yPos);
	yPos += lblLogfilePath->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtLogfilePath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdLogfilePath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);

	rescanROMsButtonActionListener = new RescanROMsButtonActionListener();
	cmdRescanROMs = new gcn::Button("Rescan Paths");
	cmdRescanROMs->setSize(cmdRescanROMs->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdRescanROMs->setBaseColor(gui_baseCol);
	cmdRescanROMs->setId("cmdRescanROMs");
	cmdRescanROMs->addActionListener(rescanROMsButtonActionListener);

	downloadXMLButtonActionListener = new DownloadXMLButtonActionListener();
	cmdDownloadXML = new gcn::Button("Update WHDLoad XML");
	cmdDownloadXML->setSize(cmdDownloadXML->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdDownloadXML->setBaseColor(gui_baseCol);
	cmdDownloadXML->setId("cmdDownloadXML");
	cmdDownloadXML->addActionListener(downloadXMLButtonActionListener);

	downloadControllerDbActionListener = new DownloadControllerDbActionListener();
	cmdDownloadCtrlDb = new gcn::Button("Update Controllers DB");
	cmdDownloadCtrlDb->setSize(cmdDownloadCtrlDb->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdDownloadCtrlDb->setBaseColor(gui_baseCol);
	cmdDownloadCtrlDb->setId("cmdDownloadCtrlDb");
	cmdDownloadCtrlDb->addActionListener(downloadControllerDbActionListener);
	
	category.panel->add(cmdRescanROMs, DISTANCE_BORDER, category.panel->getHeight() - BUTTON_HEIGHT - DISTANCE_BORDER);
	category.panel->add(cmdDownloadXML, cmdRescanROMs->getX() + cmdRescanROMs->getWidth() + DISTANCE_NEXT_X,
		category.panel->getHeight() - BUTTON_HEIGHT - DISTANCE_BORDER);
	category.panel->add(cmdDownloadCtrlDb, cmdDownloadXML->getX() + cmdDownloadXML->getWidth() + DISTANCE_NEXT_X,
		category.panel->getHeight() - BUTTON_HEIGHT - DISTANCE_BORDER);

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

	delete chkEnableLogging;
	delete lblLogfilePath;
	delete txtLogfilePath;
	delete cmdLogfilePath;
	
	delete cmdRescanROMs;
	delete cmdDownloadXML;
	delete cmdDownloadCtrlDb;
	delete folderButtonActionListener;
	delete rescanROMsButtonActionListener;
	delete downloadXMLButtonActionListener;
	delete downloadControllerDbActionListener;
	delete enableLoggingActionListener;
}


void RefreshPanelPaths()
{
	char tmp[MAX_DPATH];

	get_rom_path(tmp, MAX_DPATH);
	txtSystemROMs->setText(tmp);

	get_configuration_path(tmp, MAX_DPATH);
	txtConfigPath->setText(tmp);

	get_controllers_path(tmp, MAX_DPATH);
	txtControllersPath->setText(tmp);

	get_retroarch_file(tmp, MAX_DPATH);
	txtRetroArchFile->setText(tmp);

	chkEnableLogging->setSelected(get_logfile_enabled());
	get_logfile_path(tmp, MAX_DPATH);
	txtLogfilePath->setText(tmp);
	
	lblLogfilePath->setEnabled(chkEnableLogging->isSelected());
	txtLogfilePath->setEnabled(chkEnableLogging->isSelected());
	cmdLogfilePath->setEnabled(chkEnableLogging->isSelected());
}

bool HelpPanelPaths(std::vector<std::string>& helptext)
{
	helptext.clear();
	helptext.emplace_back("Specify the location of your Kickstart ROMs and the folders where the configurations");
	helptext.emplace_back("and controller files should be stored. With the \"...\" button you can open a dialog");
	helptext.emplace_back("to choose the folder.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can enable/disable logging and specify the location of the logfile with the relevant");
	helptext.emplace_back("options.");
	helptext.emplace_back(" ");
	helptext.emplace_back("After changing the location of the Kickstart ROMs, click on \"Rescan\" to refresh");
	helptext.emplace_back("the list of the available ROMs.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can download the latest version of the WHDLoad Booter XML file, using the");
	helptext.emplace_back("relevant button. You will need an internet connection for this to work.");
	helptext.emplace_back(" ");
	helptext.emplace_back("You can also download the latest Game Controller DB file, using the relevant button.");
	helptext.emplace_back("BEWARE: this will overwrite any changes you may have had in your local file!");
	helptext.emplace_back(" ");
	helptext.emplace_back("These settings are saved automatically when you click Rescan, or exit the emulator.");
	return true;
}
