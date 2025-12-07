#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "autoconf.h"
#include <string>
#include <vector>

void render_panel_hwinfo()
{
	ImGui::BeginChild("HWInfoList", ImVec2(0, -50), true);
	ImGui::Columns(6, "HWInfoColumns");
	ImGui::Separator();
	ImGui::Text("Type"); ImGui::NextColumn();
	ImGui::Text("Name"); ImGui::NextColumn();
	ImGui::Text("Start"); ImGui::NextColumn();
	ImGui::Text("End"); ImGui::NextColumn();
	ImGui::Text("Size"); ImGui::NextColumn();
	ImGui::Text("ID"); ImGui::NextColumn();
	ImGui::Separator();
	for (int i = 0; i < MAX_INFOS; ++i)
	{
		struct autoconfig_info* aci = expansion_get_autoconfig_data(&changed_prefs, i);
		if (aci)
		{
			ImGui::Text("%s", aci->zorro >= 1 && aci->zorro <= 3 ? std::to_string(aci->zorro).c_str() : "-"); ImGui::NextColumn();
			ImGui::Text("%s", aci->name); ImGui::NextColumn();
			ImGui::Text("0x%08x", aci->start); ImGui::NextColumn();
			ImGui::Text("0x%08x", aci->start + aci->size - 1); ImGui::NextColumn();
			ImGui::Text("0x%08x", aci->size); ImGui::NextColumn();
			ImGui::Text("0x%04x/0x%02x", (aci->autoconfig_bytes[4] << 8) | aci->autoconfig_bytes[5], aci->autoconfig_bytes[1]); ImGui::NextColumn();
		}
	}
	ImGui::Columns(1);
	ImGui::EndChild();

	ImGui::Checkbox("Custom board order", &changed_prefs.autoconfig_custom_sort);
	if (ImGui::Button("Move up"))
	{
		// Move up
	}
	ImGui::SameLine();
	if (ImGui::Button("Move down"))
	{
		// Move down
	}
}
