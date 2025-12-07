#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_configurations()
{
	static int selected = -1;
	ImGui::BeginChild("ConfigList", ImVec2(0, -100), true);
	for (int i = 0; i < ConfigFilesList.size(); ++i)
	{
		if (ImGui::Selectable(ConfigFilesList[i]->Name, selected == i))
			selected = i;
	}
	ImGui::EndChild();

	static char name[MAX_DPATH] = "";
	static char desc[MAX_DPATH] = "";
	if (selected != -1)
	{
		strncpy(name, ConfigFilesList[selected]->Name, MAX_DPATH);
		strncpy(desc, ConfigFilesList[selected]->Description, MAX_DPATH);
	}
	ImGui::InputText("Name", name, MAX_DPATH);
	ImGui::InputText("Description", desc, MAX_DPATH);

	if (ImGui::Button("Load", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
			target_cfgfile_load(&changed_prefs, ConfigFilesList[selected]->FullPath, CONFIG_TYPE_DEFAULT, 0);
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
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)))
	{
		if (selected != -1)
		{
			remove(ConfigFilesList[selected]->FullPath);
			ReadConfigFileList();
			selected = -1;
		}
	}
}
