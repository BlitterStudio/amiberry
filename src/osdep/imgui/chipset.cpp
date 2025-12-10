#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "custom.h"
#include "options.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"

void render_panel_chipset()
{
	ImGui::Indent(5.0f);

	ImGui::Columns(2, "chipset_columns", false); // 2 columns, no border
	ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.60f);

	// LEFt COLUMN
	BeginGroupBox("Chipset");
	{
		// Chipset radios
		ImGui::BeginGroup();
		ImGui::RadioButton("A1000 (No EHB)", (int*)&changed_prefs.chipset_mask, CSMASK_A1000_NOEHB);
		ImGui::RadioButton("OCS", (int*)&changed_prefs.chipset_mask, 0);
		ImGui::RadioButton("ECS + OCS Denise", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS);
		ImGui::RadioButton("AGA", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA);
		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::RadioButton("A1000", (int*)&changed_prefs.chipset_mask, CSMASK_A1000);
		ImGui::RadioButton("OCS + ECS Denise", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_DENISE);
		ImGui::RadioButton("Full ECS", (int*)&changed_prefs.chipset_mask, CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE);
		ImGui::Checkbox("NTSC", &changed_prefs.ntscmode);
		ImGui::EndGroup();

		ImGui::Spacing();
		ImGui::Checkbox("Cycle Exact (Full)", &changed_prefs.cpu_cycle_exact);
		ImGui::Checkbox("Cycle Exact (DMA/Memory)", &changed_prefs.cpu_memory_cycle_exact);
	}
	EndGroupBox("Chipset");

	BeginGroupBox("Keyboard");
	{
		const char* keyboard_items[] = { "Keyboard disconnected", "UAE High level emulation", "A500 / A500 + (6500 - 1 MCU)", "A600 (6570 - 036 MCU)", "A1000 (6500 - 1 MCU. ROM not yet dumped)", "A1000 (6570 - 036 MCU)", "A1200 (68HC05C MCU)", "A2000 (Cherry, 8039 MCU)", "A2000/A3000/A4000 (6570-036 MCU)" };
		ImGui::SetNextItemWidth(300.0f);
		ImGui::Combo("##Keyboard Layout", &changed_prefs.keyboard_mode, keyboard_items, IM_ARRAYSIZE(keyboard_items));
		ImGui::Checkbox("Keyboard N-key rollover", &changed_prefs.keyboard_nkro);
	}
	EndGroupBox("Keyboard");

	ImGui::NextColumn();

	// RIGHT COLUMN
	BeginGroupBox("Options");
	{
		ImGui::Checkbox("Immediate Blitter", &changed_prefs.immediate_blits);
		ImGui::Checkbox("Wait for Blitter", (bool*)&changed_prefs.waiting_blits);
		ImGui::Checkbox("Multithreaded Denise", (bool*)&multithread_enabled);
		
		ImGui::Spacing();
		const char* optimization_items[] = {"Full", "Partial", "None"};
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
		ImGui::Combo("Optimizations", &changed_prefs.cs_optimizations, optimization_items, IM_ARRAYSIZE(optimization_items));

		const char* chipset_items[] = { "Generic", "CDTV", "CDTV-CR", "CD32", "A500", "A500+", "A600", "A1000", "A1200", "A2000", "A3000", "A3000T", "A4000", "A4000T", "Velvet", "Casablanca", "DraCo" };
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
		ImGui::Combo("Chipset Extra", &changed_prefs.cs_compatible, chipset_items, IM_ARRAYSIZE(chipset_items));

		const char* monitor_items[] = { "-", "Autodetect" };
		ImGui::Text("Video port display hardware");
		ImGui::Combo("##VideoPort", &changed_prefs.monitoremu, monitor_items, IM_ARRAYSIZE(monitor_items));
	}
	EndGroupBox("Options");

	ImGui::Columns(1);

	BeginGroupBox("Collision Level");
	{
		ImGui::RadioButton("None##Collision", &changed_prefs.collision_level, 0);
		ImGui::RadioButton("Sprites only", &changed_prefs.collision_level, 1);
		ImGui::RadioButton("Sprites and Sprites vs. Playfield", &changed_prefs.collision_level, 2);
		ImGui::RadioButton("Full (rarely needed)", &changed_prefs.collision_level, 3);
	}
	EndGroupBox("Collision Level");

	// Genlock section placeholder if needed, matching WinUAE
	BeginGroupBox("Genlock");
	{
		ImGui::Checkbox("Genlock connected", &changed_prefs.genlock);
		ImGui::SameLine();
		const char* genlock_items[] = { "None" };
		ImGui::SetNextItemWidth(300.0f);
		ImGui::Combo("##Genlock", &changed_prefs.genlock_image, genlock_items, IM_ARRAYSIZE(genlock_items));
		ImGui::SameLine();
		const char* genlock_scale[] = { "100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%"};
		ImGui::SetNextItemWidth(100.0f);
		ImGui::Combo("##GenlockScale", &changed_prefs.genlock_scale, genlock_scale, IM_ARRAYSIZE(genlock_scale));
		ImGui::Checkbox("Include alpha channels in screenshots and video captures.", &changed_prefs.genlock_alpha);
		bool genlock_aspect = changed_prefs.genlock_aspect > 0;
		ImGui::Checkbox("Keep aspect ratio", &genlock_aspect);
	}
	EndGroupBox("Genlock");
	ImGui::Unindent(5.0f);
}
