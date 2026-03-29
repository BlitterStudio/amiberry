#include "imgui.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "uae.h"
#include "imgui_panels.h"
#include "fsdb.h"
#include <string>

extern int console_logging;
extern std::string get_xml_timestamp(const std::string& xml_filename);
extern std::string get_json_timestamp(const std::string& json_filename);

// WHDBooter download state
static std::atomic<bool> s_whdboot_downloading{false};
static std::atomic<bool> s_whdboot_complete{false};
static std::atomic<bool> s_whdboot_failed{false};
static std::atomic<bool> s_whdboot_cancel{false};
static std::atomic<int64_t> s_whdboot_dl_now{0};
static std::atomic<int64_t> s_whdboot_dl_total{0};
static std::atomic<int> s_whdboot_step{0};
static std::atomic<int> s_whdboot_step_count{0};
static std::mutex s_whdboot_mutex;
static std::string s_whdboot_status;
static std::string s_whdboot_result_msg;
static bool s_whdboot_result_is_error = false;

// Controllers DB download state
static std::atomic<bool> s_controllers_downloading{false};
static std::atomic<bool> s_controllers_complete{false};
static std::atomic<bool> s_controllers_failed{false};
static std::atomic<bool> s_controllers_cancel{false};
static std::atomic<int64_t> s_controllers_dl_now{0};
static std::atomic<int64_t> s_controllers_dl_total{0};
static std::mutex s_controllers_mutex;
static std::string s_controllers_result_msg;
static bool s_controllers_result_is_error = false;

static std::string s_base_content_path_input;
static std::string s_base_content_path_apply_value;
static std::vector<std::string> s_base_content_missing_dirs;
static bool s_base_content_path_dirty = false;
static bool s_open_base_content_apply_popup = false;

static std::string normalize_ui_path_string(const std::string& input)
{
	if (input.empty())
		return {};

	const auto normalized = std::filesystem::path(input).lexically_normal().string();
	return normalized.empty() ? input : normalized;
}

static bool path_inputs_match(const std::string& lhs, const std::string& rhs)
{
	return normalize_ui_path_string(lhs) == normalize_ui_path_string(rhs);
}

static std::string format_download_bytes(int64_t bytes)
{
	if (bytes < 1024) return std::to_string(bytes) + " B";
	if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
	char buf[32];
	snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
	return buf;
}

static void start_whdboot_download()
{
	if (s_whdboot_downloading.load())
		return;

	s_whdboot_downloading.store(true);
	s_whdboot_complete.store(false);
	s_whdboot_failed.store(false);
	s_whdboot_cancel.store(false);
	s_whdboot_dl_now.store(0);
	s_whdboot_dl_total.store(0);
	s_whdboot_step.store(0);
	s_whdboot_step_count.store(10);
	{
		std::lock_guard<std::mutex> lock(s_whdboot_mutex);
		s_whdboot_status = "Starting...";
		s_whdboot_result_msg.clear();
		s_whdboot_result_is_error = false;
	}

	std::thread([]() {
		auto progress_cb = [](int64_t dlnow, int64_t dltotal) -> bool {
			s_whdboot_dl_now.store(dlnow);
			s_whdboot_dl_total.store(dltotal);
			return s_whdboot_cancel.load();
		};

		auto set_status = [](const std::string& status) {
			std::lock_guard<std::mutex> lock(s_whdboot_mutex);
			s_whdboot_status = status;
		};

		auto do_download = [&](int step, const std::string& label, const std::string& url,
			const std::string& dest, bool backup) -> bool {
			if (s_whdboot_cancel.load()) return false;
			s_whdboot_step.store(step);
			s_whdboot_dl_now.store(0);
			s_whdboot_dl_total.store(0);
			set_status(label);
			write_log("Downloading %s ...\n", dest.c_str());
			return download_file(url, dest, backup, progress_cb, &s_whdboot_cancel);
		};

		int step = 0;
		std::string dest;
		bool all_ok = true;

		// 1. WHDLoad
		dest = get_whdbootpath().append("WHDLoad");
		if (!do_download(++step, "WHDLoad", "https://github.com/BlitterStudio/amiberry/blob/master/whdboot/WHDLoad?raw=true", dest, false))
			all_ok = false;

		// 2. JST
		dest = get_whdbootpath().append("JST");
		if (!do_download(++step, "JST", "https://github.com/BlitterStudio/amiberry/blob/master/whdboot/JST?raw=true", dest, false))
			all_ok = false;

		// 3. AmiQuit
		dest = get_whdbootpath().append("AmiQuit");
		if (!do_download(++step, "AmiQuit", "https://github.com/BlitterStudio/amiberry/blob/master/whdboot/AmiQuit?raw=true", dest, false))
			all_ok = false;

		// 4. boot-data.zip
		dest = get_whdbootpath().append("boot-data.zip");
		if (!do_download(++step, "boot-data.zip", "https://github.com/BlitterStudio/amiberry/blob/master/whdboot/boot-data.zip?raw=true", dest, false))
			all_ok = false;

		// 5-9. RTB files (skip if already present)
		const char* rtb_files[] = {
			"kick33180.A500.RTB",
			"kick34005.A500.RTB",
			"kick40063.A600.RTB",
			"kick40068.A1200.RTB",
			"kick40068.A4000.RTB"
		};
		for (const auto& rtb : rtb_files)
		{
			++step;
			if (s_whdboot_cancel.load()) break;
			const std::string rtb_dest_name = std::string("save-data/Kickstarts/") + rtb;
			const std::string rtb_dest = get_whdbootpath().append(rtb_dest_name);
			if (std::filesystem::exists(rtb_dest))
			{
				s_whdboot_step.store(step);
				set_status(std::string(rtb) + " (already exists)");
				continue;
			}
			const std::string rtb_url = std::string("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/save-data/Kickstarts/") + rtb + "?raw=true";
			if (!do_download(step, rtb, rtb_url, rtb_dest, false))
				all_ok = false;
		}

		// 10. whdload_db.json (primary) with XML fallback
		const auto json_dest = get_whdbootpath().append("game-data/whdload_db.json");
		const auto xml_dest = get_whdbootpath().append("game-data/whdload_db.xml");
		const auto old_json_timestamp = get_json_timestamp(json_dest);
		const auto old_xml_timestamp = get_xml_timestamp(xml_dest);
		const auto old_timestamp = !old_json_timestamp.empty() ? old_json_timestamp : old_xml_timestamp;

		bool db_ok = do_download(++step, "whdload_db.json",
			"https://raw.githubusercontent.com/BlitterStudio/amiberry-game-db/main/whdload_db.json", json_dest, true);

		if (!db_ok && !s_whdboot_cancel.load())
		{
			write_log("WHDBooter - JSON download failed, falling back to XML\n");
			dest = xml_dest;
			db_ok = do_download(step, "whdload_db.xml",
				"https://github.com/HoraceAndTheSpider/Amiberry-XML-Builder/blob/master/whdload_db.xml?raw=true", dest, true);
		}

		if (s_whdboot_cancel.load())
		{
			std::lock_guard<std::mutex> lock(s_whdboot_mutex);
			s_whdboot_result_msg = "Download cancelled.";
			s_whdboot_result_is_error = true;
			s_whdboot_failed.store(true);
			s_whdboot_downloading.store(false);
			return;
		}

		{
			std::lock_guard<std::mutex> lock(s_whdboot_mutex);
			if (db_ok)
			{
				const auto new_json_timestamp = get_json_timestamp(json_dest);
				const auto new_xml_timestamp = get_xml_timestamp(xml_dest);
				const auto new_timestamp = !new_json_timestamp.empty() ? new_json_timestamp : new_xml_timestamp;
				s_whdboot_result_msg = "WHDBooter files updated.\n\nDB previous: " + old_timestamp + "\nDB new: " + new_timestamp;
			}
			else if (!all_ok)
			{
				s_whdboot_result_msg = "Some files failed to download.\nPlease check the log for details.";
				s_whdboot_result_is_error = true;
			}
			else
			{
				s_whdboot_result_msg = "Failed to download game database.\nPlease check the log for details.";
				s_whdboot_result_is_error = true;
			}
		}

		if (s_whdboot_result_is_error)
			s_whdboot_failed.store(true);
		else
			s_whdboot_complete.store(true);
		s_whdboot_downloading.store(false);
	}).detach();
}

static void start_controllers_download()
{
	if (s_controllers_downloading.load())
		return;

	s_controllers_downloading.store(true);
	s_controllers_complete.store(false);
	s_controllers_failed.store(false);
	s_controllers_cancel.store(false);
	s_controllers_dl_now.store(0);
	s_controllers_dl_total.store(0);
	{
		std::lock_guard<std::mutex> lock(s_controllers_mutex);
		s_controllers_result_msg.clear();
		s_controllers_result_is_error = false;
	}

	std::thread([]() {
		auto progress_cb = [](int64_t dlnow, int64_t dltotal) -> bool {
			s_controllers_dl_now.store(dlnow);
			s_controllers_dl_total.store(dltotal);
			return s_controllers_cancel.load();
		};

		std::string destination = get_controllers_path();
		destination += "gamecontrollerdb.txt";
		write_log("Downloading %s ...\n", destination.c_str());
		const auto result = download_file(
			"https://raw.githubusercontent.com/mdqinc/SDL_GameControllerDB/master/gamecontrollerdb.txt",
			destination, true, progress_cb, &s_controllers_cancel);

		if (s_controllers_cancel.load())
		{
			std::lock_guard<std::mutex> lock(s_controllers_mutex);
			s_controllers_result_msg = "Download cancelled.";
			s_controllers_result_is_error = true;
			s_controllers_failed.store(true);
			s_controllers_downloading.store(false);
			return;
		}

		{
			std::lock_guard<std::mutex> lock(s_controllers_mutex);
			if (result)
			{
				s_controllers_result_msg = "Latest version of Game Controllers DB downloaded.";
			}
			else
			{
				s_controllers_result_msg = "Failed to download file!\nPlease check the log for details.";
				s_controllers_result_is_error = true;
			}
		}

		if (s_controllers_result_is_error)
			s_controllers_failed.store(true);
		else
			s_controllers_complete.store(true);
		s_controllers_downloading.store(false);
	}).detach();
}

void render_panel_paths()
{
	char tmp[MAX_DPATH];
	const auto applied_base_content_path = get_base_content_path();
	if (!s_base_content_path_dirty && !path_inputs_match(s_base_content_path_input, applied_base_content_path))
		s_base_content_path_input = applied_base_content_path;

	auto sync_base_content_path_input = [&]() {
		s_base_content_path_input = get_base_content_path();
		s_base_content_path_dirty = false;
		s_base_content_path_apply_value.clear();
		s_base_content_missing_dirs.clear();
	};

	auto finish_base_content_apply = [&](const bool create_missing_directories) {
		set_base_content_path(s_base_content_path_apply_value);
		if (create_missing_directories)
			create_missing_directories_for_base_content_path(s_base_content_path_apply_value);
		save_amiberry_settings();
		sync_base_content_path_input();

		if (s_base_content_path_apply_value.empty())
		{
			ShowMessageBox("Base Content Folder",
				create_missing_directories
					? "Base content folder cleared.\n\nMissing default folders were created."
					: "Base content folder cleared.");
		}
		else
		{
			ShowMessageBox("Base Content Folder",
				create_missing_directories
					? "Base content folder updated.\n\nMissing folders were created."
					: "Base content folder updated.");
		}
	};

	// Helper lambda for rendering path rows
	auto RenderPathRow = [&](const char* label, const char* id, const std::string& path, const std::function<void(const std::string&)>& setter, const bool is_file = false, const char* filter_name = nullptr, const char* filter_ext = nullptr) {
		ImGui::PushID(id);
		ImGui::Text("%s", label);

		const float button_width = SMALL_BUTTON_WIDTH;
		const float spacing = ImGui::GetStyle().ItemSpacing.x * 2;
		const float input_width = ImGui::GetContentRegionAvail().x - button_width - spacing;

		ImGui::SetNextItemWidth(input_width);

		// Validation check
		bool exists = false;
		if (path.empty()) {
			exists = true; // Treat an empty path as valid (or at least not error) to avoid clutter
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
		
		if (AmigaInputText("##Input", buffer, MAX_DPATH)) {
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

        std::string dialog_key = std::string("PATHS_") + id + (is_file ? "_FILE" : "_DIR");
        if (AmigaButton("...", ImVec2(button_width, 0)))
		{
			if (is_file) {
				OpenFileDialogKey(dialog_key.c_str(), filter_name ? filter_name : "Choose File", filter_ext ? filter_ext : ".*", path);
			} else {
				OpenDirDialogKey(dialog_key.c_str(), path);
			}
		}

		std::string filePath;
		if (is_file) {
			if (ConsumeFileDialogResultKey(dialog_key.c_str(), filePath)) {
				setter(filePath);
			}
		} else {
			if (ConsumeDirDialogResultKey(dialog_key.c_str(), filePath)) {
				setter(filePath);
			}
		}
		ImGui::PopID();
		ImGui::Spacing();
	};

	// Portable mode toggle (functional on Windows/Linux, shown disabled on macOS, hidden on Android)
#if !defined(__ANDROID__)
	{
		bool portable = get_portable_mode();
#if defined(__MACH__)
		ImGui::BeginDisabled();
#endif
		if (AmigaCheckbox("Portable mode (use exe directory for all paths)", &portable))
		{
#if !defined(__MACH__)
			if (set_portable_mode(portable))
			{
				ShowMessageBox("Portable Mode",
					portable
						? "Portable mode enabled.\n\nPlease restart Amiberry for this change to take full effect."
						: "Portable mode disabled.\n\nPlease restart Amiberry for this change to take full effect.");
			}
			else
			{
				ShowMessageBox("Portable Mode", "Failed to change portable mode.\n\nCheck file permissions in the application directory.");
			}
#endif
		}
#if defined(__MACH__)
		ImGui::EndDisabled();
#endif
			ShowHelpMarker("When enabled, all data directories are resolved relative to the executable location.\n"
				"Creates/removes the 'amiberry.portable' marker file.\n"
				"A restart is required for the change to take effect.\n\n"
				"Supported on Windows and Linux startup-directory installs.\n"
				"Not available on macOS or Android.");
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
#endif

	// Estimate reserved height for bottom controls (logfile widgets + 1 line spacing + 1 line for buttons)
	const float line_h = ImGui::GetFrameHeightWithSpacing();
	float reserved_height = line_h * 6.0f; // logfile widgets + logfile path + spacing + 2 button rows
	if (s_whdboot_downloading.load() || s_whdboot_complete.load() || s_whdboot_failed.load())
		reserved_height += line_h * 3.0f;
	if (s_controllers_downloading.load() || s_controllers_complete.load() || s_controllers_failed.load())
		reserved_height += line_h * 3.0f;

	// Begin scrollable area for path entries
	// Draw a recessed frame around the scroll area
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.06f));
	ImGui::BeginChild("PathsScroll", ImVec2(0, -reserved_height), ImGuiChildFlags_Borders);

	ImGui::PushID("BaseContentPath");
	ImGui::Text("Base content folder:");

	const float browse_button_width = SMALL_BUTTON_WIDTH;
	const float apply_button_width = BUTTON_WIDTH * 0.95f;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	float input_width = ImGui::GetContentRegionAvail().x - browse_button_width - apply_button_width - spacing * 2.0f;
	if (input_width < BUTTON_WIDTH)
		input_width = BUTTON_WIDTH;

	ImGui::SetNextItemWidth(input_width);
	const bool base_content_exists = s_base_content_path_input.empty() || my_existsdir(s_base_content_path_input.c_str());
	if (!base_content_exists)
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));

	char base_content_buffer[MAX_DPATH];
	strncpy(base_content_buffer, s_base_content_path_input.c_str(), MAX_DPATH);
	base_content_buffer[MAX_DPATH - 1] = '\0';
	if (AmigaInputText("##Input", base_content_buffer, MAX_DPATH))
	{
		s_base_content_path_input = base_content_buffer;
		s_base_content_path_dirty = true;
	}

	if (!base_content_exists)
	{
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Directory not found");
	}

	ImGui::SameLine();
	if (AmigaButton("...", ImVec2(browse_button_width, 0)))
	{
		OpenDirDialogKey("PATHS_BaseContentPath_DIR", s_base_content_path_input);
	}

	std::string base_content_dialog_path;
	if (ConsumeDirDialogResultKey("PATHS_BaseContentPath_DIR", base_content_dialog_path))
	{
		s_base_content_path_input = base_content_dialog_path;
		s_base_content_path_dirty = true;
	}

	ImGui::SameLine();
	const auto normalized_pending_base_content_path = normalize_ui_path_string(s_base_content_path_input);
	const bool can_apply_base_content = !path_inputs_match(normalized_pending_base_content_path, applied_base_content_path);
	if (!can_apply_base_content)
		ImGui::BeginDisabled();
	if (AmigaButton("Apply", ImVec2(apply_button_width, 0)))
	{
		s_base_content_path_apply_value = normalized_pending_base_content_path;
		s_base_content_missing_dirs = get_base_content_missing_directories(s_base_content_path_apply_value);
		if (s_base_content_missing_dirs.empty())
		{
			finish_base_content_apply(false);
		}
		else
		{
			s_open_base_content_apply_popup = true;
		}
	}
	if (!can_apply_base_content)
		ImGui::EndDisabled();
	ImGui::PopID();

		ImGui::TextWrapped("Optional. When set, Amiberry derives the standard folders below from this root. Visual assets are stored in a sibling Visuals folder, not inside Configuration files. Editing a path below makes that entry an explicit override. Click Apply to use the new root. The bootstrap settings files stay in Amiberry's platform settings location.");
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	RenderPathRow("System ROMs:", "SystemROMs", get_rom_path(), [](const std::string& p) { set_rom_path(p); });

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
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar();

	if (s_open_base_content_apply_popup)
	{
		ImGui::OpenPopup("Apply Base Content Folder");
		s_open_base_content_apply_popup = false;
	}
	if (ImGui::BeginPopupModal("Apply Base Content Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Some managed folders for this path layout do not exist yet.");
		if (!s_base_content_path_apply_value.empty())
		{
			ImGui::Spacing();
			ImGui::TextWrapped("Base folder: %s", s_base_content_path_apply_value.c_str());
		}
		ImGui::Spacing();
		ImGui::Text("Missing folders:");
		ImGui::BeginChild("MissingBaseContentDirs", ImVec2(BUTTON_WIDTH * 4.5f, BUTTON_HEIGHT * 6.0f), ImGuiChildFlags_Borders);
		for (const auto& directory : s_base_content_missing_dirs)
			ImGui::BulletText("%s", directory.c_str());
		ImGui::EndChild();
		ImGui::Spacing();
		if (AmigaButton("Create", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			finish_base_content_apply(true);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (AmigaButton("Use As-Is", ImVec2(BUTTON_WIDTH * 1.3f, BUTTON_HEIGHT)))
		{
			finish_base_content_apply(false);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_XMARK " Cancel", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			s_base_content_path_apply_value.clear();
			s_base_content_missing_dirs.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Logging Options
	auto logging_enabled = get_logfile_enabled();
	if (AmigaCheckbox("Enable logging", &logging_enabled))
	{
		set_logfile_enabled(logging_enabled);
		logging_init();
	}
	ShowHelpMarker("Write debug information to log file");
	ImGui::SameLine();
	auto log_to_console = console_logging > 0;
	if (AmigaCheckbox("Log to console", &log_to_console))
	{
		console_logging = log_to_console ? 1 : 0;
	}
	ShowHelpMarker("Also output log messages to terminal");

	RenderPathRow("Logfile path:", "LogFilePath", get_logfile_path(), [](const std::string& p) { set_logfile_path(p); }, true, "Choose File", ".log");

	ImGui::Spacing();
	if (AmigaButton(ICON_FA_ARROW_ROTATE_LEFT " Reset to Defaults", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
	{
		reset_default_paths();
		save_amiberry_settings();
		sync_base_content_path_input();
		ShowMessageBox("Reset Paths", "All paths have been reset to their default values.");
	}
	ImGui::SameLine();
	if (AmigaButton(ICON_FA_ARROWS_ROTATE " Rescan Paths", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
	{
		create_missing_amiberry_folders();
		scan_roms(true);
		symlink_roms(&changed_prefs);
		import_joysticks();

		ShowMessageBox("Rescan Paths", "Scan complete:\n\n- Missing folders created\n- ROMs list updated\n- Joysticks (re)initialized\n- Symlinks recreated.");
	}
	{
		const bool any_downloading = s_whdboot_downloading.load() || s_controllers_downloading.load();
		if (any_downloading)
			ImGui::BeginDisabled();

		if (AmigaButton(ICON_FA_DOWNLOAD " Update WHDBooter files", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
			start_whdboot_download();
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_DOWNLOAD " Update Controllers DB", ImVec2(BUTTON_WIDTH * 2, BUTTON_HEIGHT)))
			start_controllers_download();

		if (any_downloading)
			ImGui::EndDisabled();
	}

	// WHDBooter download progress / results
	if (s_whdboot_downloading.load())
	{
		ImGui::Spacing();
		const int step = s_whdboot_step.load();
		const int total_steps = s_whdboot_step_count.load();
		std::string status;
		{
			std::lock_guard<std::mutex> lock(s_whdboot_mutex);
			status = s_whdboot_status;
		}
		ImGui::Text("Downloading: %s (%d/%d)", status.c_str(), step, total_steps);

		const int64_t dl_now = s_whdboot_dl_now.load();
		const int64_t dl_total = s_whdboot_dl_total.load();
		float progress = 0.0f;
		if (dl_total > 0)
			progress = static_cast<float>(dl_now) / static_cast<float>(dl_total);
		if (progress > 1.0f) progress = 1.0f;

		ImGui::ProgressBar(progress, ImVec2(-SMALL_BUTTON_WIDTH - ImGui::GetStyle().ItemSpacing.x, 0.0f));
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_XMARK "##CancelWHD", ImVec2(SMALL_BUTTON_WIDTH, 0.0f)))
			s_whdboot_cancel.store(true);

		if (dl_total > 0)
			ImGui::Text("%s / %s", format_download_bytes(dl_now).c_str(), format_download_bytes(dl_total).c_str());
	}
	else if (s_whdboot_complete.load() || s_whdboot_failed.load())
	{
		ImGui::Spacing();
		{
			std::lock_guard<std::mutex> lock(s_whdboot_mutex);
			if (s_whdboot_result_is_error)
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
			ImGui::TextWrapped("%s", s_whdboot_result_msg.c_str());
			if (s_whdboot_result_is_error)
				ImGui::PopStyleColor();
		}
		if (AmigaButton(ICON_FA_CHECK " OK##WHDResult", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			s_whdboot_complete.store(false);
			s_whdboot_failed.store(false);
		}
	}

	// Controllers DB download progress / results
	if (s_controllers_downloading.load())
	{
		ImGui::Spacing();
		ImGui::Text("Downloading: gamecontrollerdb.txt");

		const int64_t dl_now = s_controllers_dl_now.load();
		const int64_t dl_total = s_controllers_dl_total.load();
		float progress = 0.0f;
		if (dl_total > 0)
			progress = static_cast<float>(dl_now) / static_cast<float>(dl_total);
		if (progress > 1.0f) progress = 1.0f;

		ImGui::ProgressBar(progress, ImVec2(-SMALL_BUTTON_WIDTH - ImGui::GetStyle().ItemSpacing.x, 0.0f));
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_XMARK "##CancelCtrl", ImVec2(SMALL_BUTTON_WIDTH, 0.0f)))
			s_controllers_cancel.store(true);

		if (dl_total > 0)
			ImGui::Text("%s / %s", format_download_bytes(dl_now).c_str(), format_download_bytes(dl_total).c_str());
	}
	else if (s_controllers_complete.load() || s_controllers_failed.load())
	{
		// Re-import joysticks on main thread after successful download
		static bool s_controllers_joysticks_reimported = false;
		if (s_controllers_complete.load() && !s_controllers_joysticks_reimported)
		{
			import_joysticks();
			s_controllers_joysticks_reimported = true;
		}

		ImGui::Spacing();
		{
			std::lock_guard<std::mutex> lock(s_controllers_mutex);
			if (s_controllers_result_is_error)
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
			ImGui::TextWrapped("%s", s_controllers_result_msg.c_str());
			if (s_controllers_result_is_error)
				ImGui::PopStyleColor();
		}
		if (AmigaButton(ICON_FA_CHECK " OK##CtrlResult", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			s_controllers_complete.store(false);
			s_controllers_failed.store(false);
			s_controllers_joysticks_reimported = false;
		}
	}
}
