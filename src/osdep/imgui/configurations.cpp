#include "sysdeps.h"
#include "imgui.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "uae.h"

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

	ImGui::BeginChild("ConfigList", ImVec2(0, -150), true);
	for (int i = 0; i < ConfigFilesList.size(); ++i)
	{
		if (search_text[0] != '\0' && strcasestr(ConfigFilesList[i]->Name, search_text) == nullptr)
			continue;

		char label[MAX_DPATH * 2];
		if (strlen(ConfigFilesList[i]->Description) > 0)
			snprintf(label, sizeof(label), "%s (%s)", ConfigFilesList[i]->Name, ConfigFilesList[i]->Description);
		else
			snprintf(label, sizeof(label), "%s", ConfigFilesList[i]->Name);

		if (ImGui::Selectable(label, selected == i, ImGuiSelectableFlags_AllowDoubleClick))
		{
			selected = i;
			strncpy(name, ConfigFilesList[i]->Name, MAX_DPATH);
			strncpy(desc, ConfigFilesList[i]->Description, MAX_DPATH);

			if (ImGui::IsMouseDoubleClicked(0))
			{
				target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0);
				strncpy(last_active_config, ConfigFilesList[selected]->Name, MAX_DPATH);
				uae_restart(NULL, 0, ConfigFilesList[selected]->FullPath);
				gui_running = false;
			}
		}
	}
	ImGui::EndChild();

	// Define a fixed width for labels to align input fields
	const float label_width = 100.0f;

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Search:");
	ImGui::SameLine(label_width);
	ImGui::InputText("##Search", search_text, sizeof(search_text));
	ImGui::SameLine();
	if (search_text[0] == '\0') ImGui::BeginDisabled();
	if (ImGui::Button("X"))
		search_text[0] = '\0';
	if (search_text[0] == '\0') ImGui::EndDisabled();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Name:");
	ImGui::SameLine(label_width);
	ImGui::InputText("##Name", name, MAX_DPATH);

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Description:");
	ImGui::SameLine(label_width);
	ImGui::InputText("##Description", desc, MAX_DPATH);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Load", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
		{
			target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0);
			strncpy(last_active_config, ConfigFilesList[selected]->Name, MAX_DPATH);
		}
	}
	ImGui::SameLine();
	if (strlen(name) == 0) ImGui::BeginDisabled();
	if (ImGui::Button("Save", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
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
	if (ImGui::Button("Delete", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
			ImGui::OpenPopup("Delete Configuration");
	}
	if (selected == -1) ImGui::EndDisabled();

	if (ImGui::BeginPopupModal("Delete Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Do you want to delete '%s'?", ConfigFilesList[selected]->Name);
		ImGui::Separator();

		if (ImGui::Button("Yes", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
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
		if (ImGui::Button("No", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
