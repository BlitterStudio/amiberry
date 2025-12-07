#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "custom.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_chipset()
{
	ImGui::RadioButton("OCS", (int*)&changed_prefs.chipset_mask, 0);
	ImGui::RadioButton("ECS Agnus", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS);
	ImGui::RadioButton("ECS Denise", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_DENISE);
	ImGui::RadioButton("Full ECS", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE);
	ImGui::RadioButton("AGA", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA);
	ImGui::Checkbox("NTSC", &changed_prefs.ntscmode);
	ImGui::Checkbox("Cycle Exact (Full)", &changed_prefs.cpu_cycle_exact);
	ImGui::Checkbox("Cycle Exact (DMA/Memory)", &changed_prefs.cpu_memory_cycle_exact);
	const char* chipset_items[] = { "Generic", "CDTV", "CDTV-CR", "CD32", "A500", "A500+", "A600", "A1000", "A1200", "A2000", "A3000", "A3000T", "A4000", "A4000T", "Velvet", "Casablanca", "DraCo" };
	ImGui::Combo("Chipset Extra", &changed_prefs.cs_compatible, chipset_items, IM_ARRAYSIZE(chipset_items));

	ImGui::Separator();
	ImGui::Text("Options");
	ImGui::Checkbox("Immediate Blitter", &changed_prefs.immediate_blits);
	ImGui::Checkbox("Wait for Blitter", (bool*)&changed_prefs.waiting_blits);
	ImGui::Checkbox("Multithreaded Drawing", (bool*)&multithread_enabled);
	const char* monitor_items[] = { "-", "Autodetect" };
	ImGui::Combo("Video port display hardware", &changed_prefs.monitoremu, monitor_items, IM_ARRAYSIZE(monitor_items));

	ImGui::Separator();
	ImGui::Text("Keyboard");
	const char* keyboard_items[] = { "Keyboard disconnected", "UAE High level emulation", "A500 / A500 + (6500 - 1 MCU)", "A600 (6570 - 036 MCU)", "A1000 (6500 - 1 MCU. ROM not yet dumped)", "A1000 (6570 - 036 MCU)", "A1200 (68HC05C MCU)", "A2000 (Cherry, 8039 MCU)", "A2000/A3000/A4000 (6570-036 MCU)" };
	ImGui::Combo("Keyboard Layout", &changed_prefs.keyboard_mode, keyboard_items, IM_ARRAYSIZE(keyboard_items));
	ImGui::Checkbox("Keyboard N-key rollover", &changed_prefs.keyboard_nkro);

	ImGui::Separator();
	ImGui::Text("Collision Level");
	ImGui::RadioButton("None##Collision", &changed_prefs.collision_level, 0);
	ImGui::RadioButton("Sprites only", &changed_prefs.collision_level, 1);
	ImGui::RadioButton("Sprites and Sprites vs. Playfield", &changed_prefs.collision_level, 2);
	ImGui::RadioButton("Full (rarely needed)", &changed_prefs.collision_level, 3);
}
