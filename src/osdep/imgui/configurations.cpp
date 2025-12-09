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
	static bool init_done = false;

	if (!init_done)
	{
		ReadConfigFileList();
		if (last_active_config[0])
		{
			for (int i = 0; i < ConfigFilesList.size(); ++i)
			{
				if (strcmp(ConfigFilesList[i]->Name, last_active_config) == 0)
				{
					selected = i;
					strncpy(name, ConfigFilesList[i]->Name, MAX_DPATH);
					strncpy(desc, ConfigFilesList[i]->Description, MAX_DPATH);
					break;
				}
			}
		}
		init_done = true;
	}

	ImGui::BeginChild("ConfigList", ImVec2(0, -120), true);
	for (int i = 0; i < ConfigFilesList.size(); ++i)
	{
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
				uae_reset(1, 1);
				gui_running = false;
			}
		}
	}
	ImGui::EndChild();

	ImGui::InputText("Name", name, MAX_DPATH);
	ImGui::InputText("Description", desc, MAX_DPATH);

	if (ImGui::Button("Load", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
		{
			target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0);
			strncpy(last_active_config, ConfigFilesList[selected]->Name, MAX_DPATH);
		}
	}
	ImGui::SameLine();
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
	ImGui::SameLine();
	if (ImGui::Button("Delete", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
			ImGui::OpenPopup("Delete Configuration");
	}

	if (ImGui::BeginPopupModal("Delete Configuration", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Do you want to delete '%s'?", ConfigFilesList[selected]->Name);
		ImGui::Separator();

		if (ImGui::Button("Yes", ImVec2(120, 0)))
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
		if (ImGui::Button("No", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
