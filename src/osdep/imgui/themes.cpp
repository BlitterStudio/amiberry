#include "imgui.h"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#endif

extern std::string get_system_fonts_path();

// Local UI state
static std::vector<std::string> s_theme_files;
static int s_selected_theme = -1;
static bool s_themes_initialized = false;
static char s_save_as_name[128] = "";
static bool s_open_save_as_popup = false;
static bool s_font_changed = false;

// Font input buffer (sized for absolute paths)
static char s_font_name_buf[512] = "";

static void color_to_float3(const amiberry_gui_color& c, float out[3])
{
	out[0] = static_cast<float>(c.r) / 255.0f;
	out[1] = static_cast<float>(c.g) / 255.0f;
	out[2] = static_cast<float>(c.b) / 255.0f;
}

static void float3_to_color(const float in[3], amiberry_gui_color& c)
{
	c.r = static_cast<uint8_t>(std::clamp(in[0], 0.0f, 1.0f) * 255.0f + 0.5f);
	c.g = static_cast<uint8_t>(std::clamp(in[1], 0.0f, 1.0f) * 255.0f + 0.5f);
	c.b = static_cast<uint8_t>(std::clamp(in[2], 0.0f, 1.0f) * 255.0f + 0.5f);
}

static bool is_protected_preset(const std::string& name)
{
	return name == "Default.theme" || name == "Dark.theme";
}

static void scan_theme_files()
{
	s_theme_files.clear();

	// Protected presets always first
	s_theme_files.emplace_back("Default.theme");
	s_theme_files.emplace_back("Dark.theme");

	const std::string themes_dir = get_themes_path();
	if (!themes_dir.empty() && std::filesystem::exists(themes_dir))
	{
		std::vector<std::string> custom;
		for (const auto& entry : std::filesystem::directory_iterator(themes_dir))
		{
			if (!entry.is_regular_file()) continue;
			const auto ext = entry.path().extension().string();
			if (ext != ".theme") continue;
			const auto fname = entry.path().filename().string();
			if (is_protected_preset(fname)) continue;
			custom.push_back(fname);
		}
		std::sort(custom.begin(), custom.end());
		for (auto& f : custom)
			s_theme_files.push_back(std::move(f));
	}
}

static void select_theme_by_name(const std::string& name)
{
	for (int i = 0; i < static_cast<int>(s_theme_files.size()); i++)
	{
		if (s_theme_files[i] == name)
		{
			s_selected_theme = i;
			return;
		}
	}
	s_selected_theme = 0;
}

static void load_selected_theme()
{
	if (s_selected_theme < 0 || s_selected_theme >= static_cast<int>(s_theme_files.size()))
		return;

	const auto& name = s_theme_files[s_selected_theme];
	if (name == "Default.theme")
	{
		const std::string path = get_themes_path() + name;
		if (std::filesystem::exists(path))
			load_theme(name);
		else
			load_default_theme();
	}
	else if (name == "Dark.theme")
	{
		const std::string path = get_themes_path() + name;
		if (std::filesystem::exists(path))
			load_theme(name);
		else
			load_default_dark_theme();
	}
	else
	{
		load_theme(name);
	}

	apply_imgui_theme();
	rebuild_gui_fonts();

	// Sync font input buffer
	strncpy(s_font_name_buf, gui_theme.font_name.c_str(), sizeof(s_font_name_buf) - 1);
	s_font_name_buf[sizeof(s_font_name_buf) - 1] = '\0';
	s_font_changed = false;
}

struct FontEntry {
	std::string full_path;
	std::string display_name; // cached basename
};

static std::vector<FontEntry> enumerate_system_fonts()
{
	std::vector<std::string> dirs;

	const std::string sys_path = get_system_fonts_path();
	if (!sys_path.empty())
		dirs.push_back(sys_path);

#ifdef AMIBERRY_IOS
	dirs.emplace_back("/System/Library/Fonts");
#elif defined(AMIBERRY_MACOS)
	dirs.emplace_back("/System/Library/Fonts");
	dirs.emplace_back("/Library/Fonts");
	const char* home = getenv("HOME");
	if (home)
		dirs.push_back(std::string(home) + "/Library/Fonts");
#elif defined(__ANDROID__)
	dirs.emplace_back("/system/fonts");
#elif !defined(_WIN32)
	// Linux / FreeBSD
	dirs.emplace_back("/usr/share/fonts");
	dirs.emplace_back("/usr/local/share/fonts");
	const char* home = getenv("HOME");
	if (home)
	{
		dirs.push_back(std::string(home) + "/.local/share/fonts");
		dirs.push_back(std::string(home) + "/.fonts");
	}
#endif

	std::vector<std::string> paths;
	for (const auto& dir : dirs)
	{
		if (!std::filesystem::exists(dir)) continue;
		try
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(
				dir, std::filesystem::directory_options::skip_permission_denied))
			{
				if (!entry.is_regular_file()) continue;
				const auto ext = entry.path().extension().string();
				if (ext == ".ttf" || ext == ".otf" || ext == ".ttc" ||
					ext == ".TTF" || ext == ".OTF" || ext == ".TTC")
				{
					paths.push_back(entry.path().string());
				}
			}
		}
		catch (const std::filesystem::filesystem_error&) {}
	}

	// Deduplicate and build display names
	std::sort(paths.begin(), paths.end());
	paths.erase(std::unique(paths.begin(), paths.end()), paths.end());

	std::vector<FontEntry> result;
	result.reserve(paths.size());
	for (auto& p : paths)
	{
		std::string name = std::filesystem::path(p).filename().string();
		result.push_back({std::move(p), std::move(name)});
	}
	return result;
}

static bool case_insensitive_contains(const std::string& haystack, const char* needle)
{
	const size_t needle_len = strlen(needle);
	if (needle_len == 0) return true;
	if (needle_len > haystack.size()) return false;
	for (size_t i = 0; i + needle_len <= haystack.size(); i++)
	{
		if (strncasecmp(haystack.c_str() + i, needle, needle_len) == 0)
			return true;
	}
	return false;
}

// Background font enumeration state
static std::atomic<bool> s_font_enum_running{false};
static std::atomic<bool> s_font_enum_done{false};
static std::mutex s_font_enum_mutex;
static std::vector<FontEntry> s_font_enum_result;

static void start_font_enumeration()
{
	if (s_font_enum_running.load())
		return;
	s_font_enum_running.store(true);
	s_font_enum_done.store(false);

	std::thread([]() {
		auto result = enumerate_system_fonts();
		{
			std::lock_guard<std::mutex> lock(s_font_enum_mutex);
			s_font_enum_result = std::move(result);
		}
		s_font_enum_done.store(true);
		s_font_enum_running.store(false);
	}).detach();
}

void render_panel_themes()
{
	// Track the persisted theme name so we can detect external changes
	// (e.g. the dark-theme toggle in the Misc panel).
	static char s_synced_theme[128] = "";

	if (!s_themes_initialized)
	{
		scan_theme_files();
		select_theme_by_name(amiberry_options.gui_theme);
		strncpy(s_font_name_buf, gui_theme.font_name.c_str(), sizeof(s_font_name_buf) - 1);
		s_font_name_buf[sizeof(s_font_name_buf) - 1] = '\0';
		s_font_changed = false;
		strncpy(s_synced_theme, amiberry_options.gui_theme, sizeof(s_synced_theme) - 1); s_synced_theme[sizeof(s_synced_theme) - 1] = '\0';
		s_themes_initialized = true;
	}

	// Re-sync if another panel changed the active theme
	if (strcmp(s_synced_theme, amiberry_options.gui_theme) != 0)
	{
		strncpy(s_synced_theme, amiberry_options.gui_theme, sizeof(s_synced_theme) - 1); s_synced_theme[sizeof(s_synced_theme) - 1] = '\0';
		// Only rescan the directory if the new theme isn't already in the list
		bool found = false;
		for (int i = 0; i < static_cast<int>(s_theme_files.size()); i++)
		{
			if (s_theme_files[i] == amiberry_options.gui_theme)
			{ found = true; break; }
		}
		if (!found)
			scan_theme_files();
		select_theme_by_name(amiberry_options.gui_theme);
		strncpy(s_font_name_buf, gui_theme.font_name.c_str(), sizeof(s_font_name_buf) - 1);
		s_font_name_buf[sizeof(s_font_name_buf) - 1] = '\0';
		s_font_changed = false;
	}

	ImGui::Indent(4.0f);

	// ========== Theme Selection ==========
	BeginGroupBox("Theme selection");

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Theme:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2);

	const char* current_label = (s_selected_theme >= 0 && s_selected_theme < static_cast<int>(s_theme_files.size()))
		? s_theme_files[s_selected_theme].c_str() : "Default.theme";

	if (ImGui::BeginCombo("##ThemeCombo", current_label))
	{
		for (int i = 0; i < static_cast<int>(s_theme_files.size()); i++)
		{
			ImGui::PushID(i);
			const bool is_selected = (s_selected_theme == i);
			if (ImGui::Selectable(s_theme_files[i].c_str(), is_selected))
			{
				s_selected_theme = i;
				load_selected_theme();
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

	EndGroupBox("Theme selection");

	ImGui::Spacing();

	// ========== Font ==========
	BeginGroupBox("Font");

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Font:");
	ImGui::SameLine();

	const float browse_btn_w = SMALL_BUTTON_WIDTH;
	const float font_input_w = ImGui::GetContentRegionAvail().x - browse_btn_w - ImGui::GetStyle().ItemSpacing.x;
	ImGui::SetNextItemWidth(font_input_w);
	if (AmigaInputText("##FontName", s_font_name_buf, sizeof(s_font_name_buf)))
	{
		gui_theme.font_name = s_font_name_buf;
		s_font_changed = true;
	}
	ImGui::SameLine();
	if (AmigaButton("...##FontBrowse", ImVec2(browse_btn_w, 0)))
	{
		std::string start_dir = get_system_fonts_path();
		if (start_dir.empty() || !std::filesystem::exists(start_dir))
			start_dir = ".";
		OpenFileDialogKey("THEMES_FONT_FILE", "Select Font",
			"Font files (*.ttf *.otf *.ttc){.ttf,.otf,.ttc}", start_dir);
	}
	{
		std::string font_result;
		if (ConsumeFileDialogResultKey("THEMES_FONT_FILE", font_result))
		{
			if (!font_result.empty())
			{
				strncpy(s_font_name_buf, font_result.c_str(), sizeof(s_font_name_buf) - 1);
				s_font_name_buf[sizeof(s_font_name_buf) - 1] = '\0';
				gui_theme.font_name = font_result;
				s_font_changed = true;
			}
		}
	}

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Size:");
	ImGui::SameLine();
	{
		const int old_size = gui_theme.font_size;
		if (AmigaButton("-##FontDec") && gui_theme.font_size > 8)
			gui_theme.font_size--;
		ImGui::SameLine();
		ImGui::Text("%d", gui_theme.font_size);
		ImGui::SameLine();
		if (AmigaButton("+##FontInc") && gui_theme.font_size < 32)
			gui_theme.font_size++;
		if (gui_theme.font_size != old_size)
			rebuild_gui_fonts();
	}

	if (s_font_changed)
	{
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_CHECK " Apply font"))
		{
			rebuild_gui_fonts();
			s_font_changed = false;
		}
	}



	// System fonts (collapsible, with search and recessed scroll area)
	static std::vector<FontEntry> s_system_fonts;
	static bool s_fonts_ready = false;
	static char s_font_search[128] = "";
	if (ImGui::TreeNode("System fonts"))
	{
		// Kick off background enumeration on first expand
		if (!s_fonts_ready && !s_font_enum_running.load() && !s_font_enum_done.load())
			start_font_enumeration();

		// Collect results when background thread finishes
		if (!s_fonts_ready && s_font_enum_done.load())
		{
			std::lock_guard<std::mutex> lock(s_font_enum_mutex);
			s_system_fonts = std::move(s_font_enum_result);
			s_fonts_ready = true;
		}

		if (s_font_enum_running.load())
		{
			ImGui::TextDisabled("Scanning system fonts...");
		}
		else if (s_system_fonts.empty())
		{
			ImGui::TextDisabled("No system fonts found");
		}
		else
		{
			// Search row
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Search:");
			ImGui::SameLine();
			const float clear_btn_w = SMALL_BUTTON_WIDTH;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - clear_btn_w - ImGui::GetStyle().ItemSpacing.x);
			AmigaInputText("##FontSearch", s_font_search, sizeof(s_font_search));
			ImGui::SameLine();
			const bool search_empty = (s_font_search[0] == '\0');
			if (search_empty) ImGui::BeginDisabled();
			if (AmigaButton(ICON_FA_XMARK "##ClearFontSearch", ImVec2(clear_btn_w, 0)))
				s_font_search[0] = '\0';
			if (search_empty) ImGui::EndDisabled();

			const bool has_filter = (s_font_search[0] != '\0');

			// Count visible fonts for the label (single pass)
			int visible_count = 0;
			if (!has_filter)
				visible_count = static_cast<int>(s_system_fonts.size());
			else
				for (const auto& fe : s_system_fonts)
					if (case_insensitive_contains(fe.display_name, s_font_search))
						visible_count++;
			ImGui::Text("%d / %d fonts", visible_count, static_cast<int>(s_system_fonts.size()));

			// Recessed scroll area
			const float list_height = std::min(200.0f, std::max(80.0f, ImGui::GetContentRegionAvail().y * 0.35f));
			ImGui::BeginChild("##SystemFontsList", ImVec2(0, list_height));
			ImGui::Indent(4.0f);

			for (const auto& fe : s_system_fonts)
			{
				if (has_filter && !case_insensitive_contains(fe.display_name, s_font_search))
					continue;

				ImGui::PushID(fe.full_path.c_str());
				if (ImGui::Selectable(fe.display_name.c_str(), false))
				{
					strncpy(s_font_name_buf, fe.full_path.c_str(), sizeof(s_font_name_buf) - 1);
					s_font_name_buf[sizeof(s_font_name_buf) - 1] = '\0';
					gui_theme.font_name = fe.full_path;
					s_font_changed = true;
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", fe.full_path.c_str());
				ImGui::PopID();
			}

			ImGui::Unindent(4.0f);
			ImGui::EndChild();
			// Draw recessed bevel around the scroll area
			const ImVec2 list_min = ImGui::GetItemRectMin();
			const ImVec2 list_max = ImGui::GetItemRectMax();
			AmigaBevel(ImVec2(list_min.x - 1, list_min.y - 1),
				ImVec2(list_max.x + 1, list_max.y + 1), true);
		}
		ImGui::TreePop();
	}

	EndGroupBox("Font");

	ImGui::Spacing();

	// ========== Colors ==========
	BeginGroupBox("Colors");

	const ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha;
	bool color_changed = false;

	auto ColorRow = [&](const char* label, amiberry_gui_color& color) {
		float col[3];
		color_to_float3(color, col);
		ImGui::AlignTextToFramePadding();
		if (ImGui::ColorEdit3(label, col, color_flags))
		{
			float3_to_color(col, color);
			color_changed = true;
		}
	};

	if (ImGui::BeginTable("##ColorGroups", 2, ImGuiTableFlags_None))
	{
		ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 1.0f);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SeparatorText("Window");
		ColorRow("Window background", gui_theme.base_color);
		ColorRow("Input fields", gui_theme.frame_color);

		ImGui::TableNextColumn();
		ImGui::SeparatorText("Text");
		ColorRow("Text", gui_theme.font_color);
		ColorRow("Text disabled", gui_theme.text_disabled_color);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SeparatorText("Buttons");
		ColorRow("Button", gui_theme.button_color);
		ColorRow("Button hover", gui_theme.button_hover_color);

		ImGui::TableNextColumn();
		ImGui::SeparatorText("Accent");
		ColorRow("Active / accent", gui_theme.selector_active);
		ColorRow("Selection highlight", gui_theme.selection_color);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SeparatorText("Borders");
		ColorRow("Border / separator", gui_theme.border_color);

		ImGui::TableNextColumn();
		ImGui::SeparatorText("Tables");
		ColorRow("Table header", gui_theme.table_header_color);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::SeparatorText("Overlay");
		ColorRow("Modal dim", gui_theme.modal_dim_color);

		ImGui::EndTable();
	}

	if (color_changed)
		apply_imgui_theme();

	EndGroupBox("Colors");

	ImGui::Spacing();

	// ========== Actions ==========
	const auto& current_theme = (s_selected_theme >= 0 && s_selected_theme < static_cast<int>(s_theme_files.size()))
		? s_theme_files[s_selected_theme] : s_theme_files[0];
	const bool is_protected = is_protected_preset(current_theme);

	if (AmigaButton(ICON_FA_CHECK " Use", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		strncpy(amiberry_options.gui_theme, current_theme.c_str(),
			sizeof(amiberry_options.gui_theme) - 1);
		amiberry_options.gui_theme[sizeof(amiberry_options.gui_theme) - 1] = '\0';
		strncpy(s_synced_theme, amiberry_options.gui_theme, sizeof(s_synced_theme) - 1); s_synced_theme[sizeof(s_synced_theme) - 1] = '\0';
		save_amiberry_settings();
	}
	ShowHelpMarker("Apply the current theme as the active theme");

	ImGui::SameLine();
	if (is_protected) ImGui::BeginDisabled();
	if (AmigaButton(ICON_FA_FLOPPY_DISK " Save", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		save_theme(current_theme);
	}
	if (is_protected) ImGui::EndDisabled();
	ShowHelpMarker("Overwrite the current custom theme file");

	ImGui::SameLine();
	if (AmigaButton(ICON_FA_FLOPPY_DISK " Save as...", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		s_save_as_name[0] = '\0';
		s_open_save_as_popup = true;
	}
	ShowHelpMarker("Save the current colors and font as a new theme file");

	ImGui::SameLine();
	if (AmigaButton(ICON_FA_ARROW_ROTATE_LEFT " Reset", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		load_selected_theme(); // already calls rebuild_gui_fonts() internally
	}
	ShowHelpMarker("Reload the current theme from disk, discarding any unsaved changes");

	// Save As popup
	if (s_open_save_as_popup)
	{
		ImGui::OpenPopup("Save Theme As");
		s_open_save_as_popup = false;
	}
	if (ImGui::BeginPopupModal("Save Theme As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Theme name:");
		AmigaInputText("##SaveAsName", s_save_as_name, sizeof(s_save_as_name));

		ImGui::Spacing();
		const bool name_empty = (s_save_as_name[0] == '\0');
		std::string save_filename = s_save_as_name;
		// Ensure .theme suffix
		if (save_filename.size() < 7 || save_filename.substr(save_filename.size() - 6) != ".theme")
			save_filename += ".theme";
		// Reject names that are just the extension or have no stem
		const auto stem = std::filesystem::path(save_filename).stem().string();
		const bool name_no_stem = stem.empty() || stem == ".theme";

		const bool name_protected = is_protected_preset(save_filename);
		const bool name_has_path_sep = save_filename.find('/') != std::string::npos
			|| save_filename.find('\\') != std::string::npos
			|| save_filename.find("..") != std::string::npos;
		const bool name_invalid = name_empty || name_no_stem || name_protected || name_has_path_sep;

		if (name_invalid) ImGui::BeginDisabled();
		if (AmigaButton(ICON_FA_FLOPPY_DISK " Save", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			save_theme(save_filename);

			// Only persist if the file was actually written
			const std::string saved_path = get_themes_path() + save_filename;
			if (std::filesystem::exists(saved_path))
			{
				strncpy(amiberry_options.gui_theme, save_filename.c_str(),
					sizeof(amiberry_options.gui_theme) - 1);
				amiberry_options.gui_theme[sizeof(amiberry_options.gui_theme) - 1] = '\0';
				strncpy(s_synced_theme, amiberry_options.gui_theme, sizeof(s_synced_theme) - 1); s_synced_theme[sizeof(s_synced_theme) - 1] = '\0';
				save_amiberry_settings();

				scan_theme_files();
				select_theme_by_name(save_filename);
			}

			ImGui::CloseCurrentPopup();
		}
		if (name_invalid) ImGui::EndDisabled();

		if (name_protected)
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Cannot overwrite built-in presets");
		}
		if (name_has_path_sep)
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Name must not contain path separators");
		}

		ImGui::SameLine();
		if (AmigaButton(ICON_FA_XMARK " Cancel", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
			ImGui::CloseCurrentPopup();

		ImGui::EndPopup();
	}
}
