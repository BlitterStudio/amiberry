#include "imgui.h"
#include <functional>
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "filesys.h"
#include "gui/gui_handling.h"
#include "uae.h"
#include "imgui_panels.h"
#include "fsdb.h"
#include <string>

extern int console_logging;
extern std::string get_xml_timestamp(const std::string& xml_filename);

void render_panel_paths()
{
	char tmp[MAX_DPATH];

	// Helper lambda for rendering path rows
	auto RenderPathRow = [&](const char* label, const char* id, std::string path, const std::function<void(const std::string&)>& setter, bool is_file = false, const char* filter_name = nullptr, const char* filter_ext = nullptr) {
		ImGui::PushID(id);
		ImGui::Text("%s", label);

		const float button_width = 40.0f; // Fixed width for "..." button
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float input_width = ImGui::GetContentRegionAvail().x - button_width - spacing;

		ImGui::SetNextItemWidth(input_width);

		// Validation check
		bool exists = false;
		if (path.empty()) {
			exists = true; // Treat empty path as valid (or at least not error) to avoid clutter
		} else {
			if (is_file) {
				exists = my_existsfile(path.c_str());
			} else {
				exists = my_existsdir(path.c_str());
			}
		}

		if (!exists) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
		}

		// Use a local buffer for InputText to avoid constness/temporary issues
		char buffer[MAX_DPATH];
		strncpy(buffer, path.c_str(), MAX_DPATH);
		buffer[MAX_DPATH - 1] = '\0';
		
		if (ImGui::InputText("##Input", buffer, MAX_DPATH)) {
			// User edited text, update immediately
			setter(buffer);
		}
		
		if (!exists) {
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip(is_file ? "File not found" : "Directory not found");
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("...", ImVec2(button_width, 0)))
		{
			if (is_file) {
				OpenFileDialog(filter_name ? filter_name : "Choose File", filter_ext ? filter_ext : ".*", path);
			} else {
				OpenDirDialog(path);
			}
		}

		std::string filePath;
		if (is_file) {
			if (ConsumeFileDialogResult(filePath)) {
				setter(filePath);
			}
		} else {
			if (ConsumeDirDialogResult(filePath)) {
				setter(filePath);
			}
		}
		ImGui::PopID();
		// ImGui::Spacing(); // Removed extra spacing to match compact look
	};

	// Estimate reserved height for bottom controls (logfile widgets + 1 line spacing + 1 line for buttons)
	const float line_h = ImGui::GetFrameHeightWithSpacing();
	const float reserved_height = line_h * 4.5f; // 2.5 lines for logfile widgets, 1 for spacing, 1 for buttons

	// Begin scrollable area for path entries
	ImGui::BeginChild("PathsScroll", ImVec2(0, -reserved_height), true, ImGuiChildFlags_AutoResizeY);

	get_rom_path(tmp, sizeof tmp);
	RenderPathRow("System ROMs:", "SystemROMs", tmp, [](const std::string& p) { set_rom_path(p); });

	get_configuration_path(tmp, sizeof tmp);
	RenderPathRow("Configuration files:", "ConfigPath", tmp, [](const std::string& p) { set_configuration_path(p); });

	get_nvram_path(tmp, sizeof tmp);
	RenderPathRow("NVRAM files:", "NVRAMPath", tmp, [](const std::string& p) { set_nvram_path(p); });

	RenderPathRow("Plugin files:", "PluginsPath", get_plugins_path(), [](const std::string& p) { set_plugins_path(p); });

	RenderPathRow("Screenshots:", "ScreenshotsPath", get_screenshot_path(), [](const std::string& p) { set_screenshot_path(p); });

	get_savestate_path(tmp, MAX_DPATH);
	RenderPathRow("State files:", "SaveStatesPath", tmp, [](const std::string& p) { set_savestate_path(p); });

	RenderPathRow("Controller files:", "ControllersPath", get_controllers_path(), [](const std::string& p) { set_controllers_path(p); });

	RenderPathRow("RetroArch config file:", "RetroArchConfigPath", get_retroarch_file(), [](const std::string& p) { set_retroarch_file(p); }, true, "Choose Retroarch .cfg file", ".cfg");

	RenderPathRow("WHDBoot files:", "WHDBootPath", get_whdbootpath(), [](const std::string& p) { set_whdbootpath(p); });

	RenderPathRow("WHDLoad Archives (LHA):", "WHDLoadPath", get_whdload_arch_path(), [](const std::string& p) { set_whdload_arch_path(p); });

	RenderPathRow("Floppies path:", "FloppiesPath", get_floppy_path(), [](const std::string& p) { set_floppy_path(p); });

	RenderPathRow("CD-ROMs path:", "CDROMPath", get_cdrom_path(), [](const std::string& p) { set_cdrom_path(p); });

	RenderPathRow("Hard drives path:", "HDDPath", get_harddrive_path(), [](const std::string& p) { set_harddrive_path(p); });

	ImGui::EndChild();

	// Logging Options
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

	RenderPathRow("Logfile path:", "LogFilePath", get_logfile_path(), [](const std::string& p) { set_logfile_path(p); }, true, "Choose File", ".log");

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
