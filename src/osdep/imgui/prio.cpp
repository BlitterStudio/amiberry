#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_prio()
{
	ImGui::Text("When Active");
	const char* prio_items[] = { "Low", "Normal", "High" };
	ImGui::Combo("Run at priority##Active", &changed_prefs.active_capture_priority, prio_items, IM_ARRAYSIZE(prio_items));
	ImGui::Checkbox("Pause emulation##Active", &changed_prefs.active_nocapture_pause);
	ImGui::Checkbox("Disable sound##Active", &changed_prefs.active_nocapture_nosound);

	ImGui::Separator();
	ImGui::Text("When Inactive");
	ImGui::Combo("Run at priority##Inactive", &changed_prefs.inactive_priority, prio_items, IM_ARRAYSIZE(prio_items));
	ImGui::Checkbox("Pause emulation##Inactive", &changed_prefs.inactive_pause);
	ImGui::Checkbox("Disable sound##Inactive", &changed_prefs.inactive_nosound);
	ImGui::Checkbox("Disable input##Inactive", (bool*)&changed_prefs.inactive_input);

	ImGui::Separator();
	ImGui::Text("When Minimized");
	ImGui::Combo("Run at priority##Minimized", &changed_prefs.minimized_priority, prio_items, IM_ARRAYSIZE(prio_items));
	ImGui::Checkbox("Pause emulation##Minimized", &changed_prefs.minimized_pause);
	ImGui::Checkbox("Disable sound##Minimized", &changed_prefs.minimized_nosound);
	ImGui::Checkbox("Disable input##Minimized", (bool*)&changed_prefs.minimized_input);
}
