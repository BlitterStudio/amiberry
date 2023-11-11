#include <guisan.hpp>
#include <SDL_ttf.h>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>

#include "fsdb_host.h"
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "gui_handling.h"
#include "tinyxml2.h"

static gcn::Label* lblSystemROMs;
static gcn::TextField* txtSystemROMs;
static gcn::Button* cmdSystemROMs;

static gcn::Label* lblControllersPath;
static gcn::TextField* txtControllersPath;
static gcn::Button* cmdControllersPath;

static gcn::Label* lblConfigPath;
static gcn::TextField* txtConfigPath;
static gcn::Button* cmdConfigPath;

static gcn::Label* lblNvramFiles;
static gcn::TextField* txtNvramFiles;
static gcn::Button* cmdNvramFiles;

static gcn::Label* lblScreenshotFiles;
static gcn::TextField* txtScreenshotFiles;
static gcn::Button* cmdScreenshotFiles;

static gcn::Label* lblStateFiles;
static gcn::TextField* txtStateFiles;
static gcn::Button* cmdStateFiles;

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
			}
			cmdSystemROMs->requestFocus();
		}
		else if (actionEvent.getSource() == cmdConfigPath)
		{
			get_configuration_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for configuration files", tmp))
			{
				set_configuration_path(tmp);
			}
			cmdConfigPath->requestFocus();
		}
		else if (actionEvent.getSource() == cmdNvramFiles)
		{
			get_nvram_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for NVRAM files", tmp))
			{
				set_nvram_path(tmp);
			}
			cmdNvramFiles->requestFocus();
		}
		else if (actionEvent.getSource() == cmdScreenshotFiles)
		{
			get_screenshot_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for Screenshot files", tmp))
			{
				set_screenshot_path(tmp);
			}
			cmdScreenshotFiles->requestFocus();
		}
		else if (actionEvent.getSource() == cmdStateFiles)
		{
			get_savestate_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for Save state files", tmp))
			{
				set_savestate_path(tmp);
			}
			cmdStateFiles->requestFocus();
		}
		else if (actionEvent.getSource() == cmdControllersPath)
		{
			get_controllers_path(tmp, MAX_DPATH);
			if (SelectFolder("Folder for controller files", tmp))
			{
				set_controllers_path(tmp);
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
			}
			cmdLogfilePath->requestFocus();
		}

		save_amiberry_settings();
		RefreshPanelPaths();
		RefreshPanelConfig();
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

		ShowMessage("Rescan Paths", "Scan complete,", "ROMs updated, Joysticks (re)initialized, Symlinks recreated.", "", "Ok", "");
		cmdRescanROMs->requestFocus();
	}
};

static RescanROMsButtonActionListener* rescanROMsButtonActionListener;

std::string get_xml_timestamp(const std::string& xml_filename)
{
	std::string result;
	tinyxml2::XMLDocument doc;
	auto error = false;

	auto* f = fopen(xml_filename.c_str(), _T("rb"));
	if (f)
	{
		auto err = doc.LoadFile(f);
		if (err != tinyxml2::XML_SUCCESS)
		{
			write_log(_T("Failed to parse '%s':  %d\n"), xml_filename.c_str(), err);
			error = true;
		}
		fclose(f);
	}
	else
	{
		error = true;
	}
	if (!error)
	{
		auto* whdbooter = doc.FirstChildElement("whdbooter");
		result = whdbooter->Attribute("timestamp");
	}
	if (!result.empty()) 
		return result;
	return "";
}

class DownloadXMLButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		std::string destination;
		//  download WHDLoad executable
		destination = prefix_with_whdboot_path("WHDLoad");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/midwan/amiberry/blob/master/whdboot/WHDLoad?raw=true", destination, false);

		//  download boot-data.zip
		destination = prefix_with_whdboot_path("boot-data.zip");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/midwan/amiberry/blob/master/whdboot/boot-data.zip?raw=true", destination, false);

		// download kickstart RTB files for maximum compatibility
		download_rtb("kick33180.A500.RTB");
		download_rtb("kick34005.A500.RTB");
		download_rtb("kick40063.A600.RTB");
		download_rtb("kick40068.A1200.RTB");
		download_rtb("kick40068.A4000.RTB");

		destination = prefix_with_whdboot_path("game-data/whdload_db.xml");
		const auto old_timestamp = get_xml_timestamp(destination);
		write_log("Downloading %s ...\n", destination.c_str());
		const auto result = download_file("https://github.com/HoraceAndTheSpider/Amiberry-XML-Builder/blob/master/whdload_db.xml?raw=true", destination, true);

		if (result)
		{
			const auto new_timestamp = get_xml_timestamp(destination);
			ShowMessage("XML Downloader", "Updated XML downloaded.", "Previous timestamp: " + old_timestamp, "New timestamp: " + new_timestamp, "Ok", "");
		}
		else
			ShowMessage("XML Downloader", "Failed to download files!", "Please check the log for more details.", "", "Ok", "");
		
		cmdDownloadXML->requestFocus();
	}
};

static DownloadXMLButtonActionListener* downloadXMLButtonActionListener;

class DownloadControllerDbActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		char config_path[MAX_DPATH];
		get_configuration_path(config_path, MAX_DPATH);
		auto destination = std::string(config_path);
		destination += "gamecontrollerdb.txt";
		write_log("Downloading % ...\n", destination.c_str());
		const auto* const url = "https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt";
		const auto result = download_file(url, destination, true);

		if (result)
		{
			import_joysticks();
			ShowMessage("Game Controllers DB", "Latest version of Game Controllers DB downloaded.", "", "", "Ok", "");
		}
		else
			ShowMessage("Game Controllers DB", "Failed to download file!", "Please check the log for more information", "", "Ok", "");

		cmdDownloadCtrlDb->requestFocus();
	}
};
static DownloadControllerDbActionListener* downloadControllerDbActionListener;

void InitPanelPaths(const config_category& category)
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

	lblNvramFiles = new gcn::Label("NVRAM files:");
	txtNvramFiles = new gcn::TextField();
	txtNvramFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtNvramFiles->setBackgroundColor(colTextboxBackground);

	cmdNvramFiles = new gcn::Button("...");
	cmdNvramFiles->setId("cmdNvramFiles");
	cmdNvramFiles->setBaseColor(gui_baseCol);
	cmdNvramFiles->addActionListener(folderButtonActionListener);

	lblScreenshotFiles = new gcn::Label("Screenshots:");
	txtScreenshotFiles = new gcn::TextField();
	txtScreenshotFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtScreenshotFiles->setBackgroundColor(colTextboxBackground);

	cmdScreenshotFiles = new gcn::Button("...");
	cmdScreenshotFiles->setId("cmdScreenshotFiles");
	cmdScreenshotFiles->setBaseColor(gui_baseCol);
	cmdScreenshotFiles->addActionListener(folderButtonActionListener);

	lblStateFiles = new gcn::Label("Save state files:");
	txtStateFiles = new gcn::TextField();
	txtStateFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtStateFiles->setBackgroundColor(colTextboxBackground);

	cmdStateFiles = new gcn::Button("...");
	cmdStateFiles->setId("cmdStateFiles");
	cmdStateFiles->setBaseColor(gui_baseCol);
	cmdStateFiles->addActionListener(folderButtonActionListener);

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

	category.panel->add(lblNvramFiles, DISTANCE_BORDER, yPos);
	yPos += lblNvramFiles->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtNvramFiles, DISTANCE_BORDER, yPos);
	category.panel->add(cmdNvramFiles, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtNvramFiles->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblScreenshotFiles, DISTANCE_BORDER, yPos);
	yPos += lblScreenshotFiles->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtScreenshotFiles, DISTANCE_BORDER, yPos);
	category.panel->add(cmdScreenshotFiles, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtScreenshotFiles->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblStateFiles, DISTANCE_BORDER, yPos);
	yPos += lblStateFiles->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtStateFiles, DISTANCE_BORDER, yPos);
	category.panel->add(cmdStateFiles, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtStateFiles->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblControllersPath, DISTANCE_BORDER, yPos);
	yPos += lblControllersPath->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtControllersPath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdControllersPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtControllersPath->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblRetroArchFile, DISTANCE_BORDER, yPos);
	yPos += lblRetroArchFile->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtRetroArchFile, DISTANCE_BORDER, yPos);
	category.panel->add(cmdRetroArchFile, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtRetroArchFile->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblLogfilePath, DISTANCE_BORDER, yPos);
	category.panel->add(chkEnableLogging, lblLogfilePath->getX() + lblLogfilePath->getWidth() + DISTANCE_NEXT_X * 3, yPos);
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

	delete lblNvramFiles;
	delete txtNvramFiles;
	delete cmdNvramFiles;

	delete lblScreenshotFiles;
	delete txtScreenshotFiles;
	delete cmdScreenshotFiles;

	delete lblStateFiles;
	delete txtStateFiles;
	delete cmdStateFiles;

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

	get_nvram_path(tmp, MAX_DPATH);
	txtNvramFiles->setText(tmp);

	get_screenshot_path(tmp, MAX_DPATH);
	txtScreenshotFiles->setText(tmp);

	get_savestate_path(tmp, MAX_DPATH);
	txtStateFiles->setText(tmp);

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
