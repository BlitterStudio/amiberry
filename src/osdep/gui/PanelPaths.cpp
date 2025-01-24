#include <guisan.hpp>
#include <guisan/sdl.hpp>

#include "fsdb_host.h"
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "gui_handling.h"
#include "tinyxml2.h"

extern int console_logging;

static gcn::ScrollArea* scrlPaths;
static gcn::Window* grpPaths;

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

static gcn::Label* lblPluginFiles;
static gcn::TextField* txtPluginFiles;
static gcn::Button* cmdPluginFiles;

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

static gcn::Label* lblFloppyPath;
static gcn::TextField* txtFloppyPath;
static gcn::Button* cmdFloppyPath;

static gcn::Label* lblCDPath;
static gcn::TextField* txtCDPath;
static gcn::Button* cmdCDPath;

static gcn::Label* lblHardDrivesPath;
static gcn::TextField* txtHardDrivesPath;
static gcn::Button* cmdHardDrivesPath;

static gcn::CheckBox* chkEnableLogging;
static gcn::CheckBox* chkLogToConsole;
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
		const auto source = actionEvent.getSource();
		if (source == cmdSystemROMs)
		{
			get_rom_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for System ROMs", std::string(tmp));
			if (!path.empty())
			{
				set_rom_path(path);
			}
			cmdSystemROMs->requestFocus();
		}
		else if (source == cmdConfigPath)
		{
			get_configuration_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for configuration files", std::string(tmp));
			if (!path.empty())
			{
				set_configuration_path(path);
			}
			cmdConfigPath->requestFocus();
		}
		else if (source == cmdNvramFiles)
		{
			get_nvram_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for NVRAM files", std::string(tmp));
			if (!path.empty())
			{
				set_nvram_path(path);
			}
			cmdNvramFiles->requestFocus();
		}
		else if (source == cmdPluginFiles)
		{
			path = SelectFolder("Folder for Plugins", get_plugins_path());
			if (!path.empty())
			{
				set_plugins_path(path);
			}
			cmdPluginFiles->requestFocus();
		}
		else if (source == cmdScreenshotFiles)
		{
			path = SelectFolder("Folder for Screenshot files", get_screenshot_path());
			if (!path.empty())
			{
				set_screenshot_path(path);
			}
			cmdScreenshotFiles->requestFocus();
		}
		else if (source == cmdStateFiles)
		{
			get_savestate_path(tmp, MAX_DPATH);
			path = SelectFolder("Folder for Save state files", std::string(tmp));
			if (!path.empty())
			{
				set_savestate_path(path);
			}
			cmdStateFiles->requestFocus();
		}
		else if (source == cmdControllersPath)
		{
			path = SelectFolder("Folder for controller files", get_controllers_path());
			if (!path.empty())
			{
				set_controllers_path(path);
			}
			cmdControllersPath->requestFocus();
		}
		else if (source == cmdRetroArchFile)
		{
			const char* filter[] = {"retroarch.cfg", "\0"};
			path = SelectFile("Select RetroArch Config File", get_retroarch_file(), filter);
			if (!path.empty())
			{
				set_retroarch_file(path);
			}
			cmdRetroArchFile->requestFocus();
		}
		else if (source == cmdWHDBootPath)
		{
			path = SelectFolder("Folder for WHDBoot files", get_whdbootpath());
			if (!path.empty())
			{
				set_whdbootpath(path);
			}
			cmdWHDBootPath->requestFocus();
		}
		else if (source == cmdWHDLoadArchPath)
		{
			path = SelectFolder("Folder for WHDLoad Archives", get_whdload_arch_path());
			if (!path.empty())
			{
				set_whdload_arch_path(path);
			}
			cmdWHDLoadArchPath->requestFocus();
		}
		else if (source == cmdFloppyPath)
		{
			path = SelectFolder("Folder for Floppies", get_floppy_path());
			if (!path.empty())
			{
				set_floppy_path(path);
			}
			cmdFloppyPath->requestFocus();
		}
		else if (source == cmdCDPath)
		{
			path = SelectFolder("Folder for CD-ROMs", get_cdrom_path());
			if (!path.empty())
			{
				set_cdrom_path(path);
			}
			cmdCDPath->requestFocus();
		}
		else if (source == cmdHardDrivesPath)
		{
			path = SelectFolder("Folder for Hard Drives", get_harddrive_path());
			if (!path.empty())
			{
				set_harddrive_path(path);
			}
			cmdHardDrivesPath->requestFocus();
		}
		else if (source == cmdLogfilePath)
		{
			const char* filter[] = { ".log", "\0" };
			path = SelectFile("Select Amiberry Log file", get_logfile_path(), filter, true);
			if (!path.empty())
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
		if (actionEvent.getSource() == chkEnableLogging)
		{
			set_logfile_enabled(chkEnableLogging->isSelected());
			logging_init();
		}
		else if (actionEvent.getSource() == chkLogToConsole)
		{
			console_logging = chkLogToConsole->isSelected() ? 1 : 0;
		}
		
		RefreshPanelPaths();
	}
};

static EnableLoggingActionListener* enableLoggingActionListener;

class RescanROMsButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		scan_roms(true);
		symlink_roms(&changed_prefs);

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
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/WHDLoad?raw=true", destination, false);

		//  download JST executable
		destination = prefix_with_whdboot_path("JST");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/JST?raw=true", destination, false);

		//  download AmiQuit executable
		destination = prefix_with_whdboot_path("AmiQuit");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/AmiQuit?raw=true", destination, false);

		//  download boot-data.zip
		destination = prefix_with_whdboot_path("boot-data.zip");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/boot-data.zip?raw=true", destination, false);

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
		std::string destination = get_controllers_path();
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
	const int textFieldWidth = category.panel->getWidth() - 2 * DISTANCE_BORDER - SMALL_BUTTON_WIDTH * 2 - DISTANCE_NEXT_X;
	folderButtonActionListener = new FolderButtonActionListener();

	grpPaths = new gcn::Window();
	grpPaths->setId("grpPaths");

	lblSystemROMs = new gcn::Label("System ROMs:");
	txtSystemROMs = new gcn::TextField();
	txtSystemROMs->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtSystemROMs->setBaseColor(gui_base_color);
	txtSystemROMs->setBackgroundColor(gui_background_color);
	txtSystemROMs->setForegroundColor(gui_foreground_color);

	cmdSystemROMs = new gcn::Button("...");
	cmdSystemROMs->setId("cmdSystemROMs");
	cmdSystemROMs->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdSystemROMs->setBaseColor(gui_base_color);
	cmdSystemROMs->setForegroundColor(gui_foreground_color);
	cmdSystemROMs->addActionListener(folderButtonActionListener);

	lblConfigPath = new gcn::Label("Configuration files:");
	txtConfigPath = new gcn::TextField();
	txtConfigPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtConfigPath->setBaseColor(gui_base_color);
	txtConfigPath->setBackgroundColor(gui_background_color);
	txtConfigPath->setForegroundColor(gui_foreground_color);

	cmdConfigPath = new gcn::Button("...");
	cmdConfigPath->setId("cmdConfigPath");
	cmdConfigPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdConfigPath->setBaseColor(gui_base_color);
	cmdConfigPath->setForegroundColor(gui_foreground_color);
	cmdConfigPath->addActionListener(folderButtonActionListener);

	lblNvramFiles = new gcn::Label("NVRAM files:");
	txtNvramFiles = new gcn::TextField();
	txtNvramFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtNvramFiles->setBaseColor(gui_base_color);
	txtNvramFiles->setForegroundColor(gui_foreground_color);
	txtNvramFiles->setBackgroundColor(gui_background_color);

	cmdNvramFiles = new gcn::Button("...");
	cmdNvramFiles->setId("cmdNvramFiles");
	cmdNvramFiles->setBaseColor(gui_base_color);
	cmdNvramFiles->setForegroundColor(gui_foreground_color);
	cmdNvramFiles->addActionListener(folderButtonActionListener);

	lblPluginFiles = new gcn::Label("Plugins path:");
	txtPluginFiles = new gcn::TextField();
	txtPluginFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtPluginFiles->setBaseColor(gui_base_color);
	txtPluginFiles->setForegroundColor(gui_foreground_color);
	txtPluginFiles->setBackgroundColor(gui_background_color);

	cmdPluginFiles = new gcn::Button("...");
	cmdPluginFiles->setId("cmdPluginFiles");
	cmdPluginFiles->setBaseColor(gui_base_color);
	cmdPluginFiles->setForegroundColor(gui_foreground_color);
	cmdPluginFiles->addActionListener(folderButtonActionListener);

	lblScreenshotFiles = new gcn::Label("Screenshots:");
	txtScreenshotFiles = new gcn::TextField();
	txtScreenshotFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtScreenshotFiles->setBaseColor(gui_base_color);
	txtScreenshotFiles->setForegroundColor(gui_foreground_color);
	txtScreenshotFiles->setBackgroundColor(gui_background_color);

	cmdScreenshotFiles = new gcn::Button("...");
	cmdScreenshotFiles->setId("cmdScreenshotFiles");
	cmdScreenshotFiles->setBaseColor(gui_base_color);
	cmdScreenshotFiles->setForegroundColor(gui_foreground_color);
	cmdScreenshotFiles->addActionListener(folderButtonActionListener);

	lblStateFiles = new gcn::Label("Save state files:");
	txtStateFiles = new gcn::TextField();
	txtStateFiles->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtStateFiles->setBaseColor(gui_base_color);
	txtStateFiles->setForegroundColor(gui_foreground_color);
	txtStateFiles->setBackgroundColor(gui_background_color);

	cmdStateFiles = new gcn::Button("...");
	cmdStateFiles->setId("cmdStateFiles");
	cmdStateFiles->setBaseColor(gui_base_color);
	cmdStateFiles->setForegroundColor(gui_foreground_color);
	cmdStateFiles->addActionListener(folderButtonActionListener);

	lblControllersPath = new gcn::Label("Controller files:");
	txtControllersPath = new gcn::TextField();
	txtControllersPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtControllersPath->setBaseColor(gui_base_color);
	txtControllersPath->setForegroundColor(gui_foreground_color);
	txtControllersPath->setBackgroundColor(gui_background_color);

	cmdControllersPath = new gcn::Button("...");
	cmdControllersPath->setId("cmdControllersPath");
	cmdControllersPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdControllersPath->setBaseColor(gui_base_color);
	cmdControllersPath->setForegroundColor(gui_foreground_color);
	cmdControllersPath->addActionListener(folderButtonActionListener);

	lblRetroArchFile = new gcn::Label("RetroArch configuration file (retroarch.cfg):");
	txtRetroArchFile = new gcn::TextField();
	txtRetroArchFile->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtRetroArchFile->setBaseColor(gui_base_color);
	txtRetroArchFile->setForegroundColor(gui_foreground_color);
	txtRetroArchFile->setBackgroundColor(gui_background_color);

	cmdRetroArchFile = new gcn::Button("...");
	cmdRetroArchFile->setId("cmdRetroArchFile");
	cmdRetroArchFile->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdRetroArchFile->setBaseColor(gui_base_color);
	cmdRetroArchFile->setForegroundColor(gui_foreground_color);
	cmdRetroArchFile->addActionListener(folderButtonActionListener);

	lblWHDBootPath = new gcn::Label("WHDBoot files:");
	txtWHDBootPath = new gcn::TextField();
	txtWHDBootPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtWHDBootPath->setBaseColor(gui_base_color);
	txtWHDBootPath->setForegroundColor(gui_foreground_color);
	txtWHDBootPath->setBackgroundColor(gui_background_color);

	cmdWHDBootPath = new gcn::Button("...");
	cmdWHDBootPath->setId("cmdWHDBootPath");
	cmdWHDBootPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdWHDBootPath->setBaseColor(gui_base_color);
	cmdWHDBootPath->setForegroundColor(gui_foreground_color);
	cmdWHDBootPath->addActionListener(folderButtonActionListener);

	lblWHDLoadArchPath = new gcn::Label("WHDLoad Archives (LHA):");
	txtWHDLoadArchPath = new gcn::TextField();
	txtWHDLoadArchPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtWHDLoadArchPath->setBaseColor(gui_base_color);
	txtWHDLoadArchPath->setForegroundColor(gui_foreground_color);
	txtWHDLoadArchPath->setBackgroundColor(gui_background_color);

	cmdWHDLoadArchPath = new gcn::Button("...");
	cmdWHDLoadArchPath->setId("cmdWHDLoadArchPath");
	cmdWHDLoadArchPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdWHDLoadArchPath->setBaseColor(gui_base_color);
	cmdWHDLoadArchPath->setForegroundColor(gui_foreground_color);
	cmdWHDLoadArchPath->addActionListener(folderButtonActionListener);

	lblFloppyPath = new gcn::Label("Floppies path:");
	txtFloppyPath = new gcn::TextField();
	txtFloppyPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtFloppyPath->setBaseColor(gui_base_color);
	txtFloppyPath->setForegroundColor(gui_foreground_color);
	txtFloppyPath->setBackgroundColor(gui_background_color);

	cmdFloppyPath = new gcn::Button("...");
	cmdFloppyPath->setId("cmdFloppyPath");
	cmdFloppyPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdFloppyPath->setBaseColor(gui_base_color);
	cmdFloppyPath->setForegroundColor(gui_foreground_color);
	cmdFloppyPath->addActionListener(folderButtonActionListener);

	lblCDPath = new gcn::Label("CD-ROMs path:");
	txtCDPath = new gcn::TextField();
	txtCDPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtCDPath->setBaseColor(gui_base_color);
	txtCDPath->setForegroundColor(gui_foreground_color);
	txtCDPath->setBackgroundColor(gui_background_color);

	cmdCDPath = new gcn::Button("...");
	cmdCDPath->setId("cmdCDPath");
	cmdCDPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdCDPath->setBaseColor(gui_base_color);
	cmdCDPath->setForegroundColor(gui_foreground_color);
	cmdCDPath->addActionListener(folderButtonActionListener);

	lblHardDrivesPath = new gcn::Label("Hard drives path:");
	txtHardDrivesPath = new gcn::TextField();
	txtHardDrivesPath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtHardDrivesPath->setBaseColor(gui_base_color);
	txtHardDrivesPath->setBackgroundColor(gui_background_color);
	txtHardDrivesPath->setForegroundColor(gui_foreground_color);

	cmdHardDrivesPath = new gcn::Button("...");
	cmdHardDrivesPath->setId("cmdHardDrivesPath");
	cmdHardDrivesPath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdHardDrivesPath->setBaseColor(gui_base_color);
	cmdHardDrivesPath->setForegroundColor(gui_foreground_color);
	cmdHardDrivesPath->addActionListener(folderButtonActionListener);

	enableLoggingActionListener = new EnableLoggingActionListener();
	chkEnableLogging = new gcn::CheckBox("Enable logging", true);
	chkEnableLogging->setId("chkEnableLogging");
	chkEnableLogging->setBaseColor(gui_base_color);
	chkEnableLogging->setBackgroundColor(gui_background_color);
	chkEnableLogging->setForegroundColor(gui_foreground_color);
	chkEnableLogging->addActionListener(enableLoggingActionListener);
	chkLogToConsole = new gcn::CheckBox("Log to console", false);
	chkLogToConsole->setId("chkLogToConsole");
	chkLogToConsole->setBaseColor(gui_base_color);
	chkLogToConsole->setBackgroundColor(gui_background_color);
	chkLogToConsole->setForegroundColor(gui_foreground_color);
	chkLogToConsole->addActionListener(enableLoggingActionListener);
	
	lblLogfilePath = new gcn::Label("Amiberry logfile path:");
	txtLogfilePath = new gcn::TextField();
	txtLogfilePath->setSize(textFieldWidth, TEXTFIELD_HEIGHT);
	txtLogfilePath->setBaseColor(gui_base_color);
	txtLogfilePath->setBackgroundColor(gui_background_color);
	txtLogfilePath->setForegroundColor(gui_foreground_color);

	cmdLogfilePath = new gcn::Button("...");
	cmdLogfilePath->setId("cmdLogfilePath");
	cmdLogfilePath->setSize(SMALL_BUTTON_WIDTH, SMALL_BUTTON_HEIGHT);
	cmdLogfilePath->setBaseColor(gui_base_color);
	cmdLogfilePath->setForegroundColor(gui_foreground_color);
	cmdLogfilePath->addActionListener(folderButtonActionListener);

	int yPos = DISTANCE_BORDER;
	grpPaths->setPosition(DISTANCE_BORDER, DISTANCE_BORDER);
	grpPaths->setBaseColor(gui_base_color);
	grpPaths->setBackgroundColor(gui_background_color);

	grpPaths->add(lblSystemROMs, DISTANCE_BORDER, yPos);
	yPos += lblSystemROMs->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtSystemROMs, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdSystemROMs, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtSystemROMs->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblConfigPath, DISTANCE_BORDER, yPos);
	yPos += lblConfigPath->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtConfigPath, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdConfigPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtConfigPath->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblNvramFiles, DISTANCE_BORDER, yPos);
	yPos += lblNvramFiles->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtNvramFiles, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdNvramFiles, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtNvramFiles->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblPluginFiles, DISTANCE_BORDER, yPos);
	yPos += lblPluginFiles->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtPluginFiles, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdPluginFiles, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtPluginFiles->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblScreenshotFiles, DISTANCE_BORDER, yPos);
	yPos += lblScreenshotFiles->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtScreenshotFiles, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdScreenshotFiles, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtScreenshotFiles->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblStateFiles, DISTANCE_BORDER, yPos);
	yPos += lblStateFiles->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtStateFiles, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdStateFiles, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtStateFiles->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblControllersPath, DISTANCE_BORDER, yPos);
	yPos += lblControllersPath->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtControllersPath, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdControllersPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtControllersPath->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblRetroArchFile, DISTANCE_BORDER, yPos);
	yPos += lblRetroArchFile->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtRetroArchFile, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdRetroArchFile, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtRetroArchFile->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblWHDBootPath, DISTANCE_BORDER, yPos);
	yPos += lblWHDBootPath->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtWHDBootPath, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdWHDBootPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtWHDBootPath->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblWHDLoadArchPath, DISTANCE_BORDER, yPos);
	yPos += lblWHDLoadArchPath->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtWHDLoadArchPath, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdWHDLoadArchPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtWHDLoadArchPath->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblFloppyPath, DISTANCE_BORDER, yPos);
	yPos += lblFloppyPath->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtFloppyPath, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdFloppyPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtFloppyPath->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblCDPath, DISTANCE_BORDER, yPos);
	yPos += lblCDPath->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtCDPath, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdCDPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);
	yPos += txtCDPath->getHeight() + DISTANCE_NEXT_Y;

	grpPaths->add(lblHardDrivesPath, DISTANCE_BORDER, yPos);
	yPos += lblHardDrivesPath->getHeight() + DISTANCE_NEXT_Y / 2;
	grpPaths->add(txtHardDrivesPath, DISTANCE_BORDER, yPos);
	grpPaths->add(cmdHardDrivesPath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X / 2, yPos);

	grpPaths->setSize(category.panel->getWidth() - DISTANCE_BORDER * 2 - 14, txtHardDrivesPath->getY() + txtHardDrivesPath->getHeight() + DISTANCE_NEXT_Y);
	grpPaths->setTitleBarHeight(1);

	scrlPaths = new gcn::ScrollArea(grpPaths);
	scrlPaths->setId("scrlPaths");
	scrlPaths->setBaseColor(gui_base_color);
	scrlPaths->setBackgroundColor(gui_background_color);
	scrlPaths->setForegroundColor(gui_foreground_color);
	scrlPaths->setWidth(category.panel->getWidth() - DISTANCE_BORDER * 2);
	scrlPaths->setHeight(category.panel->getHeight() - TEXTFIELD_HEIGHT * 6);
	scrlPaths->setFrameSize(1);
	scrlPaths->setFocusable(true);
	category.panel->add(scrlPaths, DISTANCE_BORDER, DISTANCE_BORDER);

	yPos = scrlPaths->getY() + scrlPaths->getHeight() + DISTANCE_NEXT_Y;

	category.panel->add(lblLogfilePath, DISTANCE_BORDER, yPos);
	category.panel->add(chkEnableLogging, lblLogfilePath->getX() + lblLogfilePath->getWidth() + DISTANCE_NEXT_X * 3, yPos);
	category.panel->add(chkLogToConsole, chkEnableLogging->getX() + chkEnableLogging->getWidth() + DISTANCE_NEXT_X / 2, yPos);
	yPos += lblLogfilePath->getHeight() + DISTANCE_NEXT_Y / 2;
	category.panel->add(txtLogfilePath, DISTANCE_BORDER, yPos);
	category.panel->add(cmdLogfilePath, DISTANCE_BORDER + textFieldWidth + DISTANCE_NEXT_X, yPos);

	rescanROMsButtonActionListener = new RescanROMsButtonActionListener();
	cmdRescanROMs = new gcn::Button("Rescan Paths");
	cmdRescanROMs->setSize(cmdRescanROMs->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdRescanROMs->setBaseColor(gui_base_color);
	cmdRescanROMs->setForegroundColor(gui_foreground_color);
	cmdRescanROMs->setId("cmdRescanROMs");
	cmdRescanROMs->addActionListener(rescanROMsButtonActionListener);

	downloadXMLButtonActionListener = new DownloadXMLButtonActionListener();
	cmdDownloadXML = new gcn::Button("Update WHDBooter files");
	cmdDownloadXML->setSize(cmdDownloadXML->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdDownloadXML->setBaseColor(gui_base_color);
	cmdDownloadXML->setForegroundColor(gui_foreground_color);
	cmdDownloadXML->setId("cmdDownloadXML");
	cmdDownloadXML->addActionListener(downloadXMLButtonActionListener);

	downloadControllerDbActionListener = new DownloadControllerDbActionListener();
	cmdDownloadCtrlDb = new gcn::Button("Update Controllers DB");
	cmdDownloadCtrlDb->setSize(cmdDownloadCtrlDb->getWidth() + DISTANCE_BORDER, BUTTON_HEIGHT);
	cmdDownloadCtrlDb->setBaseColor(gui_base_color);
	cmdDownloadCtrlDb->setForegroundColor(gui_foreground_color);
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

	delete lblPluginFiles;
	delete txtPluginFiles;
	delete cmdPluginFiles;

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

	delete lblFloppyPath;
	delete txtFloppyPath;
	delete cmdFloppyPath;

	delete lblCDPath;
	delete txtCDPath;
	delete cmdCDPath;

	delete lblHardDrivesPath;
	delete txtHardDrivesPath;
	delete cmdHardDrivesPath;

	delete chkEnableLogging;
	delete chkLogToConsole;
	delete lblLogfilePath;
	delete txtLogfilePath;
	delete cmdLogfilePath;

	delete grpPaths;
	delete scrlPaths;

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

	txtPluginFiles->setText(get_plugins_path());
	txtScreenshotFiles->setText(get_screenshot_path());

	get_savestate_path(tmp, MAX_DPATH);
	txtStateFiles->setText(tmp);

	txtControllersPath->setText(get_controllers_path());
	txtRetroArchFile->setText(get_retroarch_file());
	txtWHDBootPath->setText(get_whdbootpath());
	txtWHDLoadArchPath->setText(get_whdload_arch_path());
	txtFloppyPath->setText(get_floppy_path());
	txtCDPath->setText(get_cdrom_path());
	txtHardDrivesPath->setText(get_harddrive_path());

	chkEnableLogging->setSelected(get_logfile_enabled());
	chkLogToConsole->setSelected(console_logging > 0);
	txtLogfilePath->setText(get_logfile_path());

	txtSystemROMs->setEnabled(false);
	txtConfigPath->setEnabled(false);
	txtCDPath->setEnabled(false);
	txtControllersPath->setEnabled(false);
	txtFloppyPath->setEnabled(false);
	txtHardDrivesPath->setEnabled(false);
	txtNvramFiles->setEnabled(false);
	txtPluginFiles->setEnabled(false);
	txtRetroArchFile->setEnabled(false);
	txtScreenshotFiles->setEnabled(false);
	txtStateFiles->setEnabled(false);
	txtWHDBootPath->setEnabled(false);
	txtWHDLoadArchPath->setEnabled(false);

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
		helptext.emplace_back("You can also redirect the log output to console, by enabling that logging option.");
        helptext.emplace_back(" ");
        helptext.emplace_back("The \"Rescan Paths\" button will rescan the paths specified above and refresh the");
        helptext.emplace_back("local cache. This should be done if you added kickstart ROMs for example, in order");
        helptext.emplace_back("for Amiberry to pick them up. This button will regenerate the amiberry.conf file");
        helptext.emplace_back("if it's missing, and will be populated with the default values.");
        helptext.emplace_back(" ");
        helptext.emplace_back("The \"Update WHDBooter files\" button will attempt to download the latest XML used for");
        helptext.emplace_back("the WHDLoad-booter functionality of Amiberry, along with all related files in the");
		helptext.emplace_back("\"whdboot\" directory. It requires an internet connection and write permissions in the");
        helptext.emplace_back("destination directory. The downloaded XML file will be stored in the default location");
        helptext.emplace_back("(whdboot/game-data/whdload_db.xml). Once the file is successfully downloaded, you");
        helptext.emplace_back("will also get a dialog box informing you about the details. A backup copy of the");
        helptext.emplace_back("existing whdload_db.xml is made (whdboot/game-data/whdload_db.bak), to preserve any");
        helptext.emplace_back("custom edits that may have been made. The rest of the files will be updated with the");
        helptext.emplace_back("latest version from the repository.");
		helptext.emplace_back(" ");
        helptext.emplace_back("The \"Update Controllers DB\" button will attempt to download the latest version of");
        helptext.emplace_back("the bundled gamecontrollerdb.txt file, to be stored in the Controllers files path.");
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
		helptext.emplace_back("- System ROMs: The Amiga Kickstart files are by default located under 'roms'.");
		helptext.emplace_back("  After changing the location of the Kickstart ROMs, or adding any additional ROMs, ");
		helptext.emplace_back("  click on the \"Rescan\" button to refresh the list of the available ROMs. Please");
		helptext.emplace_back("  note that MT-32 ROM files may also reside here, or in a \"mt32-roms\" directory");
		helptext.emplace_back("  at this location, if you wish to use the MT-32 MIDI emulation feature in Amiberry.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Configuration files: These are located under \"conf\" by default. This is where your");
        helptext.emplace_back("  configurations will be stored, but also where Amiberry keeps the special amiberry.conf");
        helptext.emplace_back("  file, which contains the default settings the emulator uses when it starts up. This");
        helptext.emplace_back("  is also where the bundled gamecontrollersdb.txt file is located, which contains the");
        helptext.emplace_back("  community-maintained mappings for various controllers that SDL2 recognizes.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- NVRAM files: the location where CDTV/CD32 modes will store their NVRAM files.");
        helptext.emplace_back(" ");
		helptext.emplace_back("- Plugins path: the location where external plugins (such as the CAPSimg or the");
		helptext.emplace_back("  floppybridge plugin) are stored.");
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
        helptext.emplace_back("- WHDboot files: This directory contains the files required by the whd-booter process");
        helptext.emplace_back("  to launch WHDLoad game archives. In normal usage you should not need to change this.");
        helptext.emplace_back(" ");
        helptext.emplace_back("- Below that are 4 additional paths, that can be used to better organize your various");
        helptext.emplace_back("  Amiga files, and streamline GUI operations when it comes to selecting the different");
        helptext.emplace_back("  types of Amiga media. The file selector buttons in Amiberry associated with each of");
        helptext.emplace_back("  the media types, will open these path locations. The defaults are shown, but these");
        helptext.emplace_back("  can be changed to better suit your requirements.");
        helptext.emplace_back(" ");
        helptext.emplace_back("These settings are saved automatically when you click Rescan, or exit the emulator.");
        helptext.emplace_back(" ");
        return true;
}
