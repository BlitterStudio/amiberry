#include "sysdeps.h"
#include "imgui.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "uae.h"
#include "imgui_panels.h"

static ImVec4 rgb_to_vec4(int r, int g, int b, float a = 1.0f) { return ImVec4{ static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, a }; }
static ImVec4 lighten(const ImVec4& c, float f) { return ImVec4{ std::min(c.x + f, 1.0f), std::min(c.y + f, 1.0f), std::min(c.z + f, 1.0f), c.w }; }

void render_panel_configurations()
{
	static int selected = -1;
	static char name[MAX_DPATH] = "";
	static char desc[MAX_DPATH] = "";
	static char search_text[256] = "";
	static char last_seen_config[MAX_DPATH] = "";

	static bool initialized = false;
	// Check if the current config has changed (e.g. via Quickstart or loading a file)
	// If so, update the fields to match.
	if (!initialized || strncmp(last_active_config, last_seen_config, MAX_DPATH) != 0)
	{
		ReadConfigFileList();
		initialized = true;
		bool found = false;
		if (last_active_config[0])
		{
			for (int i = 0; i < ConfigFilesList.size(); ++i)
			{
				if (strcmp(ConfigFilesList[i]->Name, last_active_config) == 0)
				{
					selected = i;
					strncpy(name, ConfigFilesList[i]->Name, MAX_DPATH);
					strncpy(desc, ConfigFilesList[i]->Description, MAX_DPATH);
					found = true;
					break;
				}
			}
			
			if (!found)
			{
				// Not in the list (e.g. from Quickstart autofill), use the values directly
				strncpy(name, last_active_config, MAX_DPATH);
				strncpy(desc, changed_prefs.description, MAX_DPATH);
				selected = -1; // Ensure nothing is selected
			}
		}
		else
		{
			// Reset fields if no active config
			name[0] = '\0';
			desc[0] = '\0';
			selected = -1;
		}
		
		// Update tracker
		strncpy(last_seen_config, last_active_config, MAX_DPATH);
	}

	// Calculate footer height dynamically
	const ImGuiStyle& style = ImGui::GetStyle();
	const float input_row_h = std::max(TEXTFIELD_HEIGHT, ImGui::GetTextLineHeight()) + style.ItemSpacing.y;
	// 3 input rows + 2 Spacings + Separator + Buttons, plus extra padding to avoid scrollbar
	const float footer_h = (input_row_h * 3) + (style.ItemSpacing.y * 2) + 1.0f + BUTTON_HEIGHT + style.WindowPadding.y + 10.0f;

	ImGui::Indent(4.0f);
	ImGui::Spacing();
	ImGui::BeginChild("ConfigList", ImVec2(ImGui::GetContentRegionAvail().x - 2.0f, -footer_h));
	ImGui::Spacing();
	ImGui::Indent(4.0f);

	for (int i = 0; i < ConfigFilesList.size(); ++i)
	{
		if (search_text[0] != '\0' && strcasestr(ConfigFilesList[i]->Name, search_text) == nullptr)
			continue;

		char label[MAX_DPATH * 2];
		if (strlen(ConfigFilesList[i]->Description) > 0)
			snprintf(label, sizeof(label), "%s (%s)", ConfigFilesList[i]->Name, ConfigFilesList[i]->Description);
		else
			snprintf(label, sizeof(label), "%s", ConfigFilesList[i]->Name);

		const bool is_selected = (selected == i);
		if (is_selected)
		{
			const ImVec4 col_act = rgb_to_vec4(gui_theme.selector_active.r, gui_theme.selector_active.g, gui_theme.selector_active.b);
			ImGui::PushStyleColor(ImGuiCol_Header, col_act);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, lighten(col_act, 0.05f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, lighten(col_act, 0.10f));
		}

		if (ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_AllowDoubleClick))
		{
			selected = i;
			strncpy(name, ConfigFilesList[i]->Name, MAX_DPATH);
			strncpy(desc, ConfigFilesList[i]->Description, MAX_DPATH);

			if (ImGui::IsMouseDoubleClicked(0))
			{
				target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0);
				strncpy(last_active_config, ConfigFilesList[selected]->Name, MAX_DPATH);
				uae_restart(nullptr, 0, ConfigFilesList[selected]->FullPath);
				gui_running = false;
			}
		}

		if (is_selected)
		{
			ImGui::PopStyleColor(3);
		}
	}
	ImGui::Unindent(4.0f);
	ImGui::EndChild();
	// Draw bevel outside child
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	AmigaBevel(ImVec2(min.x - 1, min.y - 1), ImVec2(max.x + 1, max.y + 1), false);

	// Define a fixed width for labels to align input fields
	const float label_width = BUTTON_WIDTH;

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

	ImGui::Spacing();

	if (AmigaButton("Load", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
		{
			target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0);
			strncpy(last_active_config, ConfigFilesList[selected]->Name, MAX_DPATH);
		}
	}
	ImGui::SameLine();
	if (strlen(name) == 0) ImGui::BeginDisabled();
	if (AmigaButton("Save", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		char filename[MAX_DPATH];
		get_configuration_path(filename, MAX_DPATH);
		strncat(filename, name, MAX_DPATH - 1);
		strncat(filename, ".uae", MAX_DPATH - 10);
		strncpy(changed_prefs.description, desc, 256);
		if (cfgfile_save(&changed_prefs, filename, 0))
		{
			strncpy(last_active_config, name, MAX_DPATH);
			ReadConfigFileList();
			// Re-select the saved file
			for (int i = 0; i < ConfigFilesList.size(); ++i) {
				if (strcmp(ConfigFilesList[i]->Name, name) == 0) {
					selected = i;
					break;
				}
			}
		}
	}
	if (strlen(name) == 0) ImGui::EndDisabled();
	ImGui::SameLine();
	if (selected == -1) ImGui::BeginDisabled();
	if (AmigaButton("Delete", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
			ImGui::OpenPopup("Delete Configuration");
	}
	if (selected == -1) ImGui::EndDisabled();

	if (ImGui::BeginPopupModal("Delete Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Do you want to delete '%s'?", ConfigFilesList[selected]->Name);
		ImGui::Separator();

		if (AmigaButton("Yes", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			remove(ConfigFilesList[selected]->FullPath);
			ReadConfigFileList();
			selected = -1;
			name[0] = '\0';
			desc[0] = '\0';
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (AmigaButton("No", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
