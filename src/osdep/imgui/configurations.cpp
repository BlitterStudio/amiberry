#include "sysdeps.h"
#include "imgui.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "uae.h"
#include "imgui_panels.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
static const char* strcasestr(const char* haystack, const char* needle)
{
	if (!needle[0]) return haystack;
	for (; *haystack; ++haystack) {
		const char* h = haystack;
		const char* n = needle;
		while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
			++h; ++n;
		}
		if (!*n) return haystack;
	}
	return nullptr;
}
#endif

static ImVec4 rgb_to_vec4(int r, int g, int b, float a = 1.0f) { return ImVec4{ static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, a }; }
static ImVec4 lighten(const ImVec4& c, float f) { return ImVec4{ std::min(c.x + f, 1.0f), std::min(c.y + f, 1.0f), std::min(c.z + f, 1.0f), c.w }; }

static bool s_configs_initialized = false;

struct ConfigGroup
{
	std::string name;
	std::vector<int> entries;
};

static bool is_rp9_config(const ConfigFileInfo* config)
{
	const size_t path_length = strlen(config->FullPath);
	return path_length >= 4 && strcasecmp(config->FullPath + path_length - 4, ".rp9") == 0;
}

static int find_config_by_path(const char* path)
{
	if (!path || !path[0])
		return -1;

	for (int i = 0; i < static_cast<int>(ConfigFilesList.size()); ++i)
	{
#ifdef _WIN32
		if (strcasecmp(ConfigFilesList[i]->FullPath, path) == 0)
#else
		if (strcmp(ConfigFilesList[i]->FullPath, path) == 0)
#endif
			return i;
	}
	return -1;
}

static void trim_category(char* value)
{
	char* first = value;
	while (*first && std::isspace(static_cast<unsigned char>(*first)))
		++first;

	char* last = first + strlen(first);
	while (last > first && std::isspace(static_cast<unsigned char>(last[-1])))
		--last;

	const size_t length = static_cast<size_t>(last - first);
	if (first != value)
		memmove(value, first, length);
	value[length] = '\0';
}

void configurations_panel_reset()
{
	s_configs_initialized = false;
}

void render_panel_configurations()
{
	static int selected = -1;
	static char name[MAX_DPATH] = "";
	static char desc[MAX_DPATH] = "";
	static char category[256] = "";
	static char search_text[256] = "";
	static char last_seen_config[MAX_DPATH] = "";
	static bool last_read_config_descriptions = amiberry_options.read_config_descriptions;
	static bool reveal_selected_group = false;

	const auto select_config = [&](const int index)
	{
		selected = index;
		if (index < 0 || index >= static_cast<int>(ConfigFilesList.size()))
		{
			name[0] = '\0';
			desc[0] = '\0';
			category[0] = '\0';
			return;
		}

		const auto* config = ConfigFilesList[index];
		snprintf(name, sizeof(name), "%s", config->Name);
		snprintf(desc, sizeof(desc), "%s", config->Description);
		if (amiberry_options.read_config_descriptions && !is_rp9_config(config))
			snprintf(category, sizeof(category), "%s", config->Category);
		else
			category[0] = '\0';
	};

	const auto select_config_by_path = [&](const char* path)
	{
		const int index = find_config_by_path(path);
		select_config(index);
		reveal_selected_group = index >= 0;
	};

	const bool metadata_setting_changed =
		s_configs_initialized && last_read_config_descriptions != amiberry_options.read_config_descriptions;

	// Check if the current config has changed (e.g. via Quickstart or loading a file)
	// or if configuration metadata scanning was toggled. If so, update the fields to match.
	if (!s_configs_initialized || metadata_setting_changed ||
		strncmp(last_active_config, last_seen_config, MAX_DPATH) != 0)
	{
		char previously_selected_path[MAX_DPATH] = "";
		if (metadata_setting_changed && selected >= 0 && selected < static_cast<int>(ConfigFilesList.size()))
			snprintf(previously_selected_path, sizeof(previously_selected_path), "%s", ConfigFilesList[selected]->FullPath);

		ReadConfigFileList();
		s_configs_initialized = true;
		bool found = false;
		if (previously_selected_path[0])
		{
			selected = find_config_by_path(previously_selected_path);
			if (selected >= 0)
			{
				select_config(selected);
				reveal_selected_group = true;
				found = true;
			}
		}
		if (!found && last_active_config[0])
		{
			for (int i = 0; i < static_cast<int>(ConfigFilesList.size()); ++i)
			{
				if (strcmp(ConfigFilesList[i]->Name, last_active_config) == 0)
				{
					select_config(i);
					reveal_selected_group = true;
					found = true;
					break;
				}
			}
			
			if (!found)
			{
				// Not in the list (e.g. from Quickstart autofill), use the values directly
				snprintf(name, sizeof(name), "%s", last_active_config);
				snprintf(desc, sizeof(desc), "%s", changed_prefs.description);
				if (amiberry_options.read_config_descriptions)
					snprintf(category, sizeof(category), "%s", changed_prefs.category);
				else
					category[0] = '\0';
				selected = -1; // Ensure nothing is selected
			}
		}
		else if (!found)
		{
			// Reset fields if no active config
			select_config(-1);
		}
		
		// Update tracker
		snprintf(last_seen_config, sizeof(last_seen_config), "%s", last_active_config);
		last_read_config_descriptions = amiberry_options.read_config_descriptions;
	}

	// Calculate footer height dynamically
	const ImGuiStyle& style = ImGui::GetStyle();
	const float input_row_h = std::max(TEXTFIELD_HEIGHT, ImGui::GetTextLineHeight()) + style.ItemSpacing.y;
	// 4 input rows + 2 Spacings + Separator + Buttons, plus extra padding to avoid scrollbar
	const float footer_h = (input_row_h * 4) + (style.ItemSpacing.y * 2) + 1.0f + BUTTON_HEIGHT + style.WindowPadding.y + 10.0f;

	ImGui::Indent(4.0f);
	ImGui::Spacing();
	ImGui::BeginChild("ConfigList", ImVec2(ImGui::GetContentRegionAvail().x - 2.0f, -footer_h));
	ImGui::Spacing();
	ImGui::Indent(4.0f);

	const auto render_config_entry = [&](const int index)
	{
		const auto* config = ConfigFilesList[index];

		char label[MAX_DPATH * 2];
		if (strlen(config->Description) > 0)
			snprintf(label, sizeof(label), "%s (%s)", config->Name, config->Description);
		else
			snprintf(label, sizeof(label), "%s", config->Name);

		const bool is_selected = selected == index;
		if (is_selected)
		{
			const ImVec4 col_act = rgb_to_vec4(gui_theme.selector_active.r, gui_theme.selector_active.g, gui_theme.selector_active.b);
			ImGui::PushStyleColor(ImGuiCol_Header, col_act);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, lighten(col_act, 0.05f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, lighten(col_act, 0.10f));
		}

		ImGui::PushID(config->FullPath);
		if (ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_AllowDoubleClick))
		{
			select_config(index);

			if (ImGui::IsMouseDoubleClicked(0))
			{
				if (target_cfgfile_load(&changed_prefs, config->FullPath, CONFIG_TYPE_DEFAULT, 0))
				{
					play_clear_content_selection();
					snprintf(last_active_config, MAX_DPATH, "%s", config->Name);
					uae_reset(1, 1);
					gui_running = false;
				}
			}
		}
		ImGui::PopID();

		if (is_selected)
			ImGui::PopStyleColor(3);
	};

	bool grouped_view = false;
	if (amiberry_options.read_config_descriptions)
	{
		for (const auto* config : ConfigFilesList)
		{
			if (config->Category[0])
			{
				grouped_view = true;
				break;
			}
		}
	}

	if (!grouped_view)
	{
		for (int i = 0; i < static_cast<int>(ConfigFilesList.size()); ++i)
		{
			if (search_text[0] != '\0' && strcasestr(ConfigFilesList[i]->Name, search_text) == nullptr)
				continue;
			render_config_entry(i);
		}
	}
	else
	{
		std::vector<ConfigGroup> groups;
		std::unordered_map<std::string, size_t> group_indices;
		for (int i = 0; i < static_cast<int>(ConfigFilesList.size()); ++i)
		{
			const char* group_name = ConfigFilesList[i]->Category;
			if (!group_name[0] || strcmp(group_name, "Ungrouped") == 0)
				group_name = "Ungrouped";

			const auto [group_index, inserted] = group_indices.emplace(group_name, groups.size());
			if (inserted)
			{
				groups.push_back({group_name, {}});
			}
			groups[group_index->second].entries.push_back(i);
		}

		for (const auto& group : groups)
		{
			const bool category_matches =
				search_text[0] != '\0' && strcasestr(group.name.c_str(), search_text) != nullptr;
			bool has_visible_entry = search_text[0] == '\0' || category_matches;
			bool contains_selection = false;
			if (!has_visible_entry)
			{
				for (const int index : group.entries)
				{
					if (strcasestr(ConfigFilesList[index]->Name, search_text) != nullptr)
					{
						has_visible_entry = true;
						break;
					}
				}
			}
			for (const int index : group.entries)
			{
				if (index == selected)
				{
					contains_selection = true;
					break;
				}
			}
			if (!has_visible_entry)
				continue;

			if ((search_text[0] != '\0' && has_visible_entry) ||
				(reveal_selected_group && contains_selection))
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);

			ImGui::PushID(group.name.c_str());
			if (ImGui::CollapsingHeader(group.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();
				for (const int index : group.entries)
				{
					if (search_text[0] != '\0' && !category_matches &&
						strcasestr(ConfigFilesList[index]->Name, search_text) == nullptr)
						continue;
					render_config_entry(index);
				}
				ImGui::Unindent();
			}
			ImGui::PopID();
		}
	}
	reveal_selected_group = false;

	ImGui::Unindent(4.0f);
	ImGui::EndChild();
	// Draw bevel outside child
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	AmigaBevel(ImVec2(min.x - 1, min.y - 1), ImVec2(max.x + 1, max.y + 1), false);

	// Define a fixed width for labels to align input fields
	const float label_width = BUTTON_WIDTH * 1.2f;

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Search:");
	ImGui::SameLine(label_width);
	AmigaInputText("##Search", search_text, sizeof(search_text));
	ImGui::SameLine();
	if (search_text[0] == '\0') ImGui::BeginDisabled();
	if (AmigaButton("X"))
		search_text[0] = '\0';
	if (search_text[0] == '\0') ImGui::EndDisabled();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Name:");
	ImGui::SameLine(label_width);
	AmigaInputText("##Name", name, MAX_DPATH);

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Description:");
	ImGui::SameLine(label_width);
	AmigaInputText("##Description", desc, MAX_DPATH);

	const bool category_editable = amiberry_options.read_config_descriptions &&
		(selected < 0 || (selected < static_cast<int>(ConfigFilesList.size()) &&
			!is_rp9_config(ConfigFilesList[selected])));
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Category:");
	ImGui::SameLine(label_width);
	if (!category_editable)
		ImGui::BeginDisabled();
	AmigaInputText("##Category", category, sizeof(category));
	if (!category_editable)
		ImGui::EndDisabled();

	ImGui::Spacing();

	if (AmigaButton(ICON_FA_UPLOAD " Load", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
		{
			if (target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0))
			{
				play_clear_content_selection();
				strncpy(last_active_config, ConfigFilesList[selected]->Name, MAX_DPATH);
				gui_config_mark_clean();
			}
		}
	}
	ImGui::SameLine();
	if (strlen(name) == 0) ImGui::BeginDisabled();
	if (AmigaButton(ICON_FA_FLOPPY_DISK " Save", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		char filename[MAX_DPATH];
		char config_path[MAX_DPATH];
		get_configuration_path(config_path, MAX_DPATH);
		snprintf(filename, MAX_DPATH, "%s%s.uae", config_path, name);
		snprintf(changed_prefs.description, 256, "%s", desc);
		if (category_editable)
		{
			trim_category(category);
			snprintf(changed_prefs.category, sizeof(changed_prefs.category), "%s", category);
		}
		if (cfgfile_save(&changed_prefs, filename, 0))
		{
			write_log("Config save: SUCCESS\n");
			snprintf(last_active_config, MAX_DPATH, "%s", name);
			snprintf(last_seen_config, MAX_DPATH, "%s", last_active_config);
			gui_config_mark_clean();
			ReadConfigFileList();
			select_config_by_path(filename);
		}
		else
		{
			write_log("Config save: FAILED for '%s'\n", filename);
		}
	}
	if (strlen(name) == 0) ImGui::EndDisabled();
	ImGui::SameLine();
	if (AmigaButton(ICON_FA_FLOPPY_DISK " Save As...", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		std::string config_dir = get_configuration_path();
		OpenFileDialogKey("CONFIG_SAVE_AS", "Save Configuration As", "UAE Config (*.uae){.uae}", config_dir, true);
	}
	{
		std::string save_as_path;
		if (ConsumeFileDialogResultKey("CONFIG_SAVE_AS", save_as_path))
		{
			if (!save_as_path.empty())
			{
				if (save_as_path.size() < 4 ||
					strcasecmp(save_as_path.c_str() + save_as_path.size() - 4, ".uae") != 0)
				{
					save_as_path += ".uae";
				}
				snprintf(changed_prefs.description, 256, "%s", desc);
				if (category_editable)
				{
					trim_category(category);
					snprintf(changed_prefs.category, sizeof(changed_prefs.category), "%s", category);
				}
				if (cfgfile_save(&changed_prefs, save_as_path.c_str(), 0))
				{
					write_log("Config save as: SUCCESS '%s'\n", save_as_path.c_str());
					ReadConfigFileList();
					char saved_name[MAX_DPATH];
					extract_filename(save_as_path.c_str(), saved_name);
					remove_file_extension(saved_name);
					snprintf(last_active_config, MAX_DPATH, "%s", saved_name);
					snprintf(last_seen_config, MAX_DPATH, "%s", last_active_config);
					gui_config_mark_clean();
					select_config_by_path(save_as_path.c_str());
				}
				else
				{
					write_log("Config save as: FAILED for '%s'\n", save_as_path.c_str());
				}
			}
		}
	}
	ImGui::SameLine();
	if (selected == -1) ImGui::BeginDisabled();
	if (AmigaButton(ICON_FA_TRASH_CAN " Delete", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
			ImGui::OpenPopup("Delete Configuration");
	}
	if (selected == -1) ImGui::EndDisabled();

	if (ImGui::BeginPopupModal("Delete Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Do you want to delete '%s'?", ConfigFilesList[selected]->Name);
		ImGui::Separator();

		if (AmigaButton(ICON_FA_CHECK " Yes", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			remove(ConfigFilesList[selected]->FullPath);
			ReadConfigFileList();
			selected = -1;
			name[0] = '\0';
			desc[0] = '\0';
			category[0] = '\0';
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (AmigaButton(ICON_FA_XMARK " No", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
