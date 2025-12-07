#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "filesys.h"
#include "gui/gui_handling.h"
#include "uae.h"
#include "imgui_panels.h"
#include <string>

extern int console_logging;
extern std::string get_xml_timestamp(const std::string& xml_filename);

void render_panel_paths()
{
	char tmp[MAX_DPATH];

	// Estimate reserved height for bottom controls (logfile widgets + 1 line spacing + 1 line for buttons)
	const float line_h = ImGui::GetFrameHeightWithSpacing();
	const float reserved_height = line_h * 3.5f; // 1.5 lines for logfile widgets, 1 for spacing, 1 for buttons

	// Begin scrollable area for path entries, only fills available space above bottom controls
	ImGui::BeginChild("PathsScroll", ImVec2(0, -reserved_height), true, ImGuiChildFlags_AutoResizeY);
	get_rom_path(tmp, sizeof tmp);
	ImGui::Text("System ROMs:");
	ImGui::InputText("##SystemROMs", tmp, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##SystemROMs"))
	{
		OpenDirDialog(std::string(tmp));
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_rom_path(filePath);
	}

	ImGui::Spacing();
	get_configuration_path(tmp, sizeof tmp);
	ImGui::Text("Configuration files:");
	ImGui::InputText("##ConfigPath", tmp, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##ConfigPath"))
	{
		OpenDirDialog(std::string(tmp));
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_configuration_path(filePath);
	}

	ImGui::Spacing();
	get_nvram_path(tmp, sizeof tmp);
	ImGui::Text("NVRAM files:");
	ImGui::InputText("##NVRAMPath", tmp, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##NVRAMPath"))
	{
		OpenDirDialog(std::string(tmp));
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_nvram_path(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("Plugin files:");
	ImGui::InputText("##PluginsPath", get_plugins_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##PluginsPath"))
	{
		OpenDirDialog(get_plugins_path());
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_plugins_path(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("Screenshots:");
	ImGui::InputText("##ScreenshotsPath", get_screenshot_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##ScreenshotsPath"))
	{
		OpenDirDialog(get_screenshot_path()); // fixed: was get_plugins_path()
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_screenshot_path(filePath);
	}

	ImGui::Spacing();
	get_savestate_path(tmp, MAX_DPATH);
	ImGui::Text("State files:");
	ImGui::InputText("##SaveStatesPath", tmp, MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##SaveStatesPath"))
	{
		OpenDirDialog(std::string(tmp));
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_savestate_path(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("Controller files:");
	ImGui::InputText("##ControllersPath", get_controllers_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##ControllersPath"))
	{
		OpenDirDialog(get_controllers_path());
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_controllers_path(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("RetroArch config file:");
	ImGui::InputText("##RetroArchConfigPath", get_retroarch_file().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##RetroArchConfigPath"))
	{
		OpenFileDialog("Choose Retroarch .cfg file", ".cfg", get_retroarch_file());
	}
	{
		std::string filePath;
		if (ConsumeFileDialogResult(filePath))
			set_retroarch_file(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("WHDBoot files:");
	ImGui::InputText("##WHDBootPath", get_whdbootpath().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##WHDBootPath"))
	{
		OpenDirDialog(get_whdbootpath());
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_whdbootpath(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("WHDLoad Archives (LHA):");
	ImGui::InputText("##WHDLoadPath", get_whdload_arch_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##WHDLoadPath"))
	{
		OpenDirDialog(get_whdload_arch_path());
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_whdload_arch_path(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("Floppies path:");
	ImGui::InputText("##FloppiesPath", get_floppy_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##FloppiesPath"))
	{
		OpenDirDialog(get_floppy_path());
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_floppy_path(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("CD-ROMs path:");
	ImGui::InputText("##CDROMPath", get_cdrom_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##CDROMPath"))
	{
		OpenDirDialog(get_cdrom_path());
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_cdrom_path(filePath);
	}

	ImGui::Spacing();
	ImGui::Text("Hard drives path:");
	ImGui::InputText("##HDDPath", get_harddrive_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##HDDPath"))
	{
		OpenDirDialog(get_harddrive_path());
	}
	{
		std::string filePath;
		if (ConsumeDirDialogResult(filePath))
			set_harddrive_path(filePath);
	}
	ImGui::EndChild();

	auto logging_enabled = get_logfile_enabled();
	if (ImGui::Checkbox("Enable logging", &logging_enabled))
	{
		set_logfile_enabled(logging_enabled);
		logging_init();
	}
	ImGui::SameLine();
	auto log_to_console = console_logging > 0;
	if (ImGui::Checkbox("Log to console", &log_to_console))
	{
		console_logging = log_to_console ? 1 : 0;
	}
	ImGui::Text("Logfile path:");
	ImGui::SameLine();
	ImGui::InputText("##LogFilePath", get_logfile_path().data(), MAX_DPATH);
	ImGui::SameLine();
	if (ImGui::Button("...##LogFilePath"))
	{
		OpenFileDialog("Choose File", ".log", get_logfile_path());
	}
	{
		std::string filePath;
		if (ConsumeFileDialogResult(filePath))
			set_logfile_path(filePath);
	}

	ImGui::Spacing();
	if (ImGui::Button("Rescan Paths", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
	{
		scan_roms(true);
		symlink_roms(&changed_prefs);
		import_joysticks();

		ShowMessageBox("Rescan Paths", "Scan complete:\n\n- ROMs list updated\n- Joysticks (re)initialized\n- Symlinks recreated.");
	}
	ImGui::SameLine();
	if (ImGui::Button("Update WHDBooter files", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
	{
		std::string destination;
		//  download WHDLoad executable
		destination = get_whdbootpath().append("WHDLoad");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/WHDLoad?raw=true", destination, false);

		//  download JST executable
		destination = get_whdbootpath().append("JST");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/JST?raw=true", destination, false);

		//  download AmiQuit executable
		destination = get_whdbootpath().append("AmiQuit");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/AmiQuit?raw=true", destination, false);

		//  download boot-data.zip
		destination = get_whdbootpath().append("boot-data.zip");
		write_log("Downloading %s ...\n", destination.c_str());
		download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/boot-data.zip?raw=true", destination, false);

		// download kickstart RTB files for maximum compatibility
		download_rtb("kick33180.A500.RTB");
		download_rtb("kick34005.A500.RTB");
		download_rtb("kick40063.A600.RTB");
		download_rtb("kick40068.A1200.RTB");
		download_rtb("kick40068.A4000.RTB");

		destination = get_whdbootpath().append("game-data/whdload_db.xml");
		const auto old_timestamp = get_xml_timestamp(destination);
		write_log("Downloading %s ...\n", destination.c_str());
		const auto result = download_file("https://github.com/HoraceAndTheSpider/Amiberry-XML-Builder/blob/master/whdload_db.xml?raw=true", destination, true);

		if (result)
		{
			const auto new_timestamp = get_xml_timestamp(destination);
			std::string message = "Updated XML downloaded.\n\nPrevious timestamp: " + old_timestamp + "\nNew timestamp: " + new_timestamp;
			ShowMessageBox("XML Downloader", message.c_str());
		}
		else
			ShowMessageBox("XML Downloader", "Failed to download files!\n\nPlease check the log for more details.");
	}
	ImGui::SameLine();
	if (ImGui::Button("Update Controllers DB", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
	{
		std::string destination = get_controllers_path();
		destination += "gamecontrollerdb.txt";
		write_log("Downloading % ...\n", destination.c_str());
		const auto* const url = "https://raw.githubusercontent.com/gabomdq/SDL_GameControllerDB/master/gamecontrollerdb.txt";
		const auto result = download_file(url, destination, true);

		if (result)
		{
			import_joysticks();
			ShowMessageBox("Game Controllers DB", "Latest version of Game Controllers DB downloaded.");
		}
		else
			ShowMessageBox("Game Controllers DB", "Failed to download file!\n\nPlease check the log for more information");
	}
}
