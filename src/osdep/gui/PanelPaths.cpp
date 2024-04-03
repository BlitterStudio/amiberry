#include <guisan.hpp>
#include <guisan/sdl.hpp>

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

static gcn::Label* lblWHDBootPath;
static gcn::TextField* txtWHDBootPath;
static gcn::Button* cmdWHDBootPath;

static gcn::Label* lblWHDLoadArchPath;
static gcn::TextField* txtWHDLoadArchPath;
static gcn::Button* cmdWHDLoadArchPath;

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
		std::string path;

		if (actionEvent.getSource() == cmdSystemROMs)
		{
			get_rom_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for System ROMs", std::string(tmp));
			{
				set_rom_path(path);
			}
			cmdSystemROMs->requestFocus();
		}
		else if (actionEvent.getSource() == cmdConfigPath)
		{
			get_configuration_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for configuration files", std::string(tmp));
			{
				set_configuration_path(path);
			}
			cmdConfigPath->requestFocus();
		}
		else if (actionEvent.getSource() == cmdNvramFiles)
		{
			get_nvram_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for NVRAM files", std::string(tmp));
			{
				set_nvram_path(path);
			}
			cmdNvramFiles->requestFocus();
		}
		else if (actionEvent.getSource() == cmdScreenshotFiles)
		{
			path = SelectFolder("Folder for Screenshot files", get_screenshot_path());
			{
				set_screenshot_path(path);
			}
			cmdScreenshotFiles->requestFocus();
		}
		else if (actionEvent.getSource() == cmdStateFiles)
		{
			get_savestate_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for Save state files", std::string(tmp));
			{
				set_savestate_path(path);
			}
			cmdStateFiles->requestFocus();
		}
		else if (actionEvent.getSource() == cmdControllersPath)
		{
			path = SelectFolder("Folder for controller files", get_controllers_path());
			{
				set_controllers_path(path);
			}
			cmdControllersPath->requestFocus();
		}

		else if (actionEvent.getSource() == cmdRetroArchFile)
		{
			const char* filter[] = {"retroarch.cfg", "\0"};
			path = SelectFile("Select RetroArch Config File", get_retroarch_file(), filter);
			{
				set_retroarch_file(path);
			}
			cmdRetroArchFile->requestFocus();
		}

		else if (actionEvent.getSource() == cmdWHDBootPath)
		{
			path = SelectFolder("Folder for WHDBoot files", get_whdbootpath());
			{
				set_whdbootpath(path);
			}
			cmdWHDBootPath->requestFocus();
		}

		else if (actionEvent.getSource() == cmdWHDLoadArchPath)
		{
			path = SelectFolder("Folder for WHDLoad Archives", get_whdload_arch_path());
			{
				set_whdload_arch_path(path);
			}
			cmdWHDLoadArchPath->requestFocus();
		}

		else if (actionEvent.getSource() == cmdLogfilePath)
		{
			const char* filter[] = { "amiberry.log", "\0" };
			path = SelectFile("Select Amiberry Log file", get_logfile_path(), filter, true);
			{
				set_logfile_path(path);
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
	const int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH - DISTANCE_NEXT_X;
	int yPos = DISTANCE_BORDER;
	folderButtonActionListener = new FolderButtonActionListener();

	lblSystemROMs = new gcn::Label("System ROMs:");
	txtSystemROMs = new gcn::TextField();
	txtSystemROMs->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtSystemROMs->setBaseColor(gui_baseCol);
	txtSystemROMs->setBackgroundColor(colTextboxBackground);

	cmdSystemROMs = new gcn::Button("...");
	cmdSystemROMs->setId("cmdSystemROMs");
	cmdSystemROMs->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSystemROMs->setBaseColor(gui_baseCol);
	cmdSystemROMs->addActionListener(folderButtonActionListener);

	lblConfigPath = new gcn::Label("Configuration files:");
	txtConfigPath = new gcn::TextField();
	txtConfigPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtConfigPath->setBaseColor(gui_baseCol);
	txtConfigPath->setBackgroundColor(colTextboxBackground);

	cmdConfigPath = new gcn::Button("...");
	cmdConfigPath->setId("cmdConfigPath");
	cmdConfigPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdConfigPath->setBaseColor(gui_baseCol);
	cmdConfigPath->addActionListener(folderButtonActionListener);

	lblNvramFiles = new gcn::Label("NVRAM files:");
	txtNvramFiles = new gcn::TextField();
	txtNvramFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtNvramFiles->setBaseColor(gui_baseCol);
	txtNvramFiles->setBackgroundColor(colTextboxBackground);

	cmdNvramFiles = new gcn::Button("...");
	cmdNvramFiles->setId("cmdNvramFiles");
	cmdNvramFiles->setBaseColor(gui_baseCol);
	cmdNvramFiles->addActionListener(folderButtonActionListener);

	lblScreenshotFiles = new gcn::Label("Screenshots:");
	txtScreenshotFiles = new gcn::TextField();
	txtScreenshotFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtScreenshotFiles->setBaseColor(gui_baseCol);
	txtScreenshotFiles->setBackgroundColor(colTextboxBackground);

	cmdScreenshotFiles = new gcn::Button("...");
	cmdScreenshotFiles->setId("cmdScreenshotFiles");
	cmdScreenshotFiles->setBaseColor(gui_baseCol);
	cmdScreenshotFiles->addActionListener(folderButtonActionListener);

	lblStateFiles = new gcn::Label("Save state files:");
	txtStateFiles = new gcn::TextField();
	txtStateFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtStateFiles->setBaseColor(gui_baseCol);
	txtStateFiles->setBackgroundColor(colTextboxBackground);

	cmdStateFiles = new gcn::Button("...");
	cmdStateFiles->setId("cmdStateFiles");
	cmdStateFiles->setBaseColor(gui_baseCol);
	cmdStateFiles->addActionListener(folderButtonActionListener);

	lblControllersPath = new gcn::Label("Controller files:");
	txtControllersPath = new gcn::TextField();
	txtControllersPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtControllersPath->setBaseColor(gui_baseCol);
	txtControllersPath->setBackgroundColor(colTextboxBackground);

	cmdControllersPath = new gcn::Button("...");
	cmdControllersPath->setId("cmdControllersPath");
	cmdControllersPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdControllersPath->setBaseColor(gui_baseCol);
	cmdControllersPath->addActionListener(folderButtonActionListener);

	lblRetroArchFile = new gcn::Label("RetroArch configuration file (retroarch.cfg):");
	txtRetroArchFile = new gcn::TextField();
	txtRetroArchFile->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtRetroArchFile->setBaseColor(gui_baseCol);
	txtRetroArchFile->setBackgroundColor(colTextboxBackground);

	cmdRetroArchFile = new gcn::Button("...");
	cmdRetroArchFile->setId("cmdRetroArchFile");
	cmdRetroArchFile->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdRetroArchFile->setBaseColor(gui_baseCol);
	cmdRetroArchFile->addActionListener(folderButtonActionListener);

	lblWHDBootPath = new gcn::Label("WHDBoot files:");
	txtWHDBootPath = new gcn::TextField();
	txtWHDBootPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtWHDBootPath->setBaseColor(gui_baseCol);
	txtWHDBootPath->setBackgroundColor(colTextboxBackground);

	cmdWHDBootPath = new gcn::Button("...");
	cmdWHDBootPath->setId("cmdWHDBootPath");
	cmdWHDBootPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdWHDBootPath->setBaseColor(gui_baseCol);
	cmdWHDBootPath->addActionListener(folderButtonActionListener);

	lblWHDLoadArchPath = new gcn::Label("WHDLoad Archives (LHA):");
	txtWHDLoadArchPath = new gcn::TextField();
	txtWHDLoadArchPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtWHDLoadArchPath->setBaseColor(gui_baseCol);
	txtWHDLoadArchPath->setBackgroundColor(colTextboxBackground);

	cmdWHDLoadArchPath = new gcn::Button("...");
	cmdWHDLoadArchPath->setId("cmdWHDLoadArchPath");
	cmdWHDLoadArchPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdWHDLoadArchPath->setBaseColor(gui_baseCol);
	cmdWHDLoadArchPath->addActionListener(folderButtonActionListener);

	enableLoggingActionListener = new EnableLoggingActionListener();
	chkEnableLogging = new gcn::CheckBox("Enable logging", true);
	chkEnableLogging->setId("chkEnableLogging");
	chkEnableLogging->addActionListener(enableLoggingActionListener);
	
	lblLogfilePath = new gcn::Label("Amiberry logfile path:");
	txtLogfilePath = new gcn::TextField();
	txtLogfilePath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtLogfilePath->setBaseColor(gui_baseCol);
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

	category.panel->add(lblWHDBootPath, DISTANCE_BORDER, yPos);
	yPos += lblWHDBootPath->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtWHDBootPath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdWHDBootPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);
	yPos += txtWHDBootPath->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblWHDLoadArchPath, DISTANCE_BORDER, yPos);
	yPos += lblWHDLoadArchPath->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtWHDLoadArchPath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdWHDLoadArchPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);

	yPos += txtWHDLoadArchPath->getHeight() + DISTANCE_NEXT_Y * 2;

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

	delete lblWHDBootPath;
	delete txtWHDBootPath;
	delete cmdWHDBootPath;

	delete lblWHDLoadArchPath;
	delete txtWHDLoadArchPath;
	delete cmdWHDLoadArchPath;

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

	txtScreenshotFiles->setText(get_screenshot_path());

	get_savestate_path(tmp, MAX_DPATH);
	txtStateFiles->setText(tmp);

	txtControllersPath->setText(get_controllers_path());
	txtRetroArchFile->setText(get_retroarch_file());
	txtWHDBootPath->setText(get_whdbootpath());
	txtWHDLoadArchPath->setText(get_whdload_arch_path());

	chkEnableLogging->setSelected(get_logfile_enabled());
	txtLogfilePath->setText(get_logfile_path());
	
	lblLogfilePath->setEnabled(chkEnableLogging->isSelected());
	txtLogfilePath->setEnabled(chkEnableLogging->isSelected());
	cmdLogfilePath->setEnabled(chkEnableLogging->isSelected());
}

bool HelpPanelPaths(std::vector<std::string>& helptext)
{
        helptext.clear();
        helptext.emplace_back("Here you can configure the various paths for Amiberry resources. In normal usage,");
        helptext.emplace_back("the default paths should work fine, however if you wish to change any path, you");
        helptext.emplace_back("can use the \"...\" button, to select the folder/path of your choosing. Details");
        helptext.emplace_back("for each path resource appear below.");
        helptext.emplace_back(" ");
        helptext.emplace_back("You can enable/disable logging and specify the location of the logfile by using");
        helptext.emplace_back("the relevant options. A logfile is useful when trying to troubleshoot something,");
        helptext.emplace_back("but otherwise this option should be off, as it will incur some extra overhead.");
        helptext.emplace_back(" ");
        helptext.emplace_back("The \"Rescan Paths\" button will rescan the paths specified above and refresh the");
        helptext.emplace_back("local cache. This should be done if you added kickstart ROMs for example, in order");
        helptext.emplace_back("for Amiberry to pick them up. This button will regenerate the amiberry.conf file");
        helptext.emplace_back("if it's missing, and will be populated with the default values.");
        helptext.emplace_back(" ");
        helptext.emplace_back("The \"Update WHDLoad XML\" button will attempt to download the latest XML used for");
        helptext.emplace_back("the WHDLoad-booter functionality of Amiberry. It requires an internet connection to");
        helptext.emplace_back("work, obviously. The downloaded file will be stored in the default location of");
        helptext.emplace_back("(whdboot/game-data/whdload_db.xml). Once the file is successfully downloaded, you");
        helptext.emplace_back("will also get a dialog box informing you about the details. A backup copy of the");
        helptext.emplace_back("existing whdload_db.xml is made (whdboot/game-data/whdload_db.bak), to preserve any");
        helptext.emplace_back("custom edits that may have been made.");
        helptext.emplace_back(" ");
        helptext.emplace_back("The \"Update Controllers DB\" button will attempt to download the latest version of");
        helptext.emplace_back("the bundled gamecontrollerdb.txt file, to be stored in the Configuration files path.");
        helptext.emplace_back("The file contains the \"official\" mappings for recognized controllers by SDL2 itself.");
        helptext.emplace_back("Please note that this is separate from the user-configurable gamecontrollerdb_user.txt");
        helptext.emplace_back("file, which is contained in the Controllers path. That file is never overwritten, and");
        helptext.emplace_back("it will be loaded after the official one, so any entries contained there will take a ");
        helptext.emplace_back("higher priority. Once the file is successfully downloaded, you will also get a dialog");
        helptext.emplace_back("box informing you about the details. A backup copy of the existing gamecontrollerdb.txt");
        helptext.emplace_back("(conf/gamecontrollerdb.bak) is created, to preserve any custom edits it may contain.");
        helptext.emplace_back(" ");
        helptext.emplace_back("The paths for Amiberry resources include;");
        helptext.emplace_back(" ");
        helptext.emplace_back("- System ROMs: The Amiga Kickstart files are by default located under 'kickstarts'.");
        helptext.emplace_back("  After changing the location of the Kickstart ROMs, or adding additional ROMs, ");
        helptext.emplace_back("  click on the \"Rescan\" button to refresh the list of the available ROMs.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Configuration files: These are located under \"conf\" by default. This is where your");
        helptext.emplace_back("  configurations will be stored, but also where Amiberry keeps the special amiberry.conf");
        helptext.emplace_back("  file, which contains the default settings the emulator uses when it starts up. This");
        helptext.emplace_back("  is also where the bundled gamecontrollersdb.txt file is located, which contains the");
        helptext.emplace_back("  community-maintained mappings for various controllers that SDL2 recognizes.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- NVRAM files: the location where CDTV/CD32 modes will store their NVRAM files.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Screenshots: any screenshots you take will be saved by default in this location.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Save state files: if you use them, they will be saved in the specified location.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Controller files: any custom (user-generated) controller mapping files will be saved");
        helptext.emplace_back("  in this location. This location is also used in RetroArch environments (ie; such as");
        helptext.emplace_back("  RetroPie) to point to the directory containing the controller mappings.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- RetroArch configuration file (retroarch.cfg): only useful if you are using RetroArch");
        helptext.emplace_back("  (ie; in RetroPie). Amiberry can pick-up the configuration file from the path specified");
        helptext.emplace_back("  here, and load it automatically, applying any mappings it contains. You can ignore this");
        helptext.emplace_back("  path if you're not using RetroArch.");
        helptext.emplace_back(" ");
        helptext.emplace_back("These settings are saved automatically when you click Rescan, or exit the emulator.");
        helptext.emplace_back(" ");
        return true;
}
