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
		ImGui::RadioButton("OCS + OCS Denise", (int*)&changed_prefs.chipset_mask, 0);
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
		
		bool cycle_exact = changed_prefs.cpu_cycle_exact;
		if (ImGui::Checkbox("Cycle Exact (Full)", &cycle_exact))
		{
			changed_prefs.cpu_cycle_exact = cycle_exact;
			if (cycle_exact)
			{
				changed_prefs.cpu_memory_cycle_exact = true;
				changed_prefs.blitter_cycle_exact = true;
			}
			else
			{
				if (changed_prefs.cpu_model < 68020)
				{
					changed_prefs.cpu_memory_cycle_exact = false;
					changed_prefs.blitter_cycle_exact = false;
				}
			}
		}

		bool memory_exact = changed_prefs.cpu_memory_cycle_exact;
		// Disabled if CPU < 68020 (WinUAE logic)
		ImGui::BeginDisabled(changed_prefs.cpu_model < 68020);
		if (ImGui::Checkbox("Cycle Exact (DMA/Memory)", &memory_exact))
		{
			changed_prefs.cpu_memory_cycle_exact = memory_exact;
			changed_prefs.blitter_cycle_exact = memory_exact;
			if (memory_exact)
			{
				if (changed_prefs.cpu_model < 68020) {
					changed_prefs.cpu_cycle_exact = true;
				}
				
				if (changed_prefs.cpu_model <= 68030) {
					changed_prefs.m68k_speed = 0;
					changed_prefs.cpu_compatible = 1;
				}
				changed_prefs.immediate_blits = false;
				changed_prefs.cachesize = 0;
				changed_prefs.gfx_framerate = 1;
			}
			else
			{
				changed_prefs.cpu_cycle_exact = false;
			}
		}
		ImGui::EndDisabled();
	}
	EndGroupBox("Chipset");

	BeginGroupBox("Keyboard");
	{
		const char* keyboard_items[] = { "Keyboard disconnected", "UAE High level emulation", "A500 / A500 + (6500 - 1 MCU)", "A600 (6570 - 036 MCU)", "A1000 (6500 - 1 MCU. ROM not yet dumped)", "A1000 (6570 - 036 MCU)", "A1200 (68HC05C MCU)", "A2000 (Cherry, 8039 MCU)", "A2000/A3000/A4000 (6570-036 MCU)" };
		ImGui::SetNextItemWidth(300.0f);
		int kb_mode = changed_prefs.keyboard_mode + 1;
		if (ImGui::Combo("##Keyboard Layout", &kb_mode, keyboard_items, IM_ARRAYSIZE(keyboard_items)))
		{
			changed_prefs.keyboard_mode = kb_mode - 1;
		}
		
		// Logic: if mode is UAE (0) or A2000_8039 (7 implies index 7 in array which is item 8 in 0-indexed CPP but WinUAE uses constants), 
		// WinUAE constant KB_UAE = 0, KB_A2000_8039 = 6 (based on header view or standard defines).
		// Wait, looking at options.h: KB_UAE=0, KB_A2000_8039=6.
		// items array index matches these values? 
		// Items: 0=Disc, 1=UAE, 2=A500, 3=A600, 4=A1000, 5=A1000_6570, 6=A1200, 7=A2000_8039, 8=A2000_6570
		// So KB_UAE maps to index 1 in combo (value 0). KB_A2000_8039 maps to index 7 in combo (value 6).
		bool disable_nkro = (changed_prefs.keyboard_mode == KB_UAE || changed_prefs.keyboard_mode == KB_A2000_8039);
		if (disable_nkro) changed_prefs.keyboard_nkro = true;
		
		ImGui::BeginDisabled(disable_nkro);
		ImGui::Checkbox("Keyboard N-key rollover", &changed_prefs.keyboard_nkro);
		ImGui::EndDisabled();
	}
	EndGroupBox("Keyboard");

	ImGui::NextColumn();

	// RIGHT COLUMN
	BeginGroupBox("Options");
	{
		ImGui::BeginDisabled(changed_prefs.cpu_cycle_exact);
		if (ImGui::Checkbox("Immediate Blitter", &changed_prefs.immediate_blits)) {
			if (changed_prefs.immediate_blits) changed_prefs.waiting_blits = false;
		}
		ImGui::EndDisabled();
		
		bool disable_wait_blits = (changed_prefs.immediate_blits || (changed_prefs.cpu_memory_cycle_exact && changed_prefs.cpu_model <= 68010));
		if (disable_wait_blits) changed_prefs.waiting_blits = 0;
		ImGui::BeginDisabled(disable_wait_blits);
		bool wait_blits = changed_prefs.waiting_blits;
		if (ImGui::Checkbox("Wait for Blitter", &wait_blits)) {
			changed_prefs.waiting_blits = wait_blits ? 1 : 0;
			if (changed_prefs.waiting_blits) changed_prefs.immediate_blits = false;
		}
		ImGui::EndDisabled();
		
		ImGui::Checkbox("Multithreaded Denise", (bool*)&multithread_enabled);
		
		ImGui::Spacing();
		const char* optimization_items[] = { "Full", "Partial", "None" };
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
		ImGui::Combo("Optimizations", &changed_prefs.cs_optimizations, optimization_items, IM_ARRAYSIZE(optimization_items));

		const char* chipset_items[] = { "Generic", "CDTV", "CDTV-CR", "CD32", "A500", "A500+", "A600", "A1000", "A1200", "A2000", "A3000", "A3000T", "A4000", "A4000T", "Velvet", "Casablanca", "DraCo" };
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
		if (ImGui::Combo("Chipset Extra", &changed_prefs.cs_compatible, chipset_items, IM_ARRAYSIZE(chipset_items)))
		{
			built_in_chipset_prefs(&changed_prefs);
		}

		ImGui::Text("Monitor sync source:");
		const char* sync_items[] = { "Combined", "CSync", "H/VSync" };
		ImGui::Combo("##SyncSource", &changed_prefs.cs_hvcsync, sync_items, IM_ARRAYSIZE(sync_items));

		const char* monitor_items[] = { "-", "Autodetect" };
		ImGui::Text("Video port display hardware");
		ImGui::Combo("##VideoPort", &changed_prefs.monitoremu, monitor_items, IM_ARRAYSIZE(monitor_items));
	}
	EndGroupBox("Options");

	ImGui::Columns(1);

	BeginGroupBox("Collision Level");
	{
		ImGui::BeginGroup();
		ImGui::RadioButton("None##Collision", &changed_prefs.collision_level, 0);
		ImGui::RadioButton("Sprites only", &changed_prefs.collision_level, 1);
		ImGui::EndGroup();
		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::RadioButton("Sprites and Sprites vs. Playfield", &changed_prefs.collision_level, 2);
		ImGui::RadioButton("Full (rarely needed)", &changed_prefs.collision_level, 3);
		ImGui::EndGroup();
	}
	EndGroupBox("Collision Level");

	// Genlock section placeholder if needed, matching WinUAE
	BeginGroupBox("Genlock");
	{
		ImGui::Checkbox("Genlock connected", &changed_prefs.genlock);
		ImGui::SameLine();
		
		bool genlock_enable = changed_prefs.genlock || changed_prefs.genlock_effects;
		ImGui::BeginDisabled(!genlock_enable);
		
		const char* genlock_items[] = { 
			"-", 
			"Noise (built-in)", 
			"Test card (built-in)", 
			"Image file (png)", 
			"Video file", 
			"Capture device", 
			"American Laser Games/Picmatic LaserDisc Player", 
			"Sony LaserDisc Player", 
			"Pioneer LaserDisc Player" 
		};
		ImGui::SetNextItemWidth(300.0f);
		ImGui::Combo("##Genlock", &changed_prefs.genlock_image, genlock_items, IM_ARRAYSIZE(genlock_items));
		ImGui::SameLine();
		
		const char* genlock_mix_items[] = { "100%", "90%", "80%", "70%", "60%", "50%", "40%", "30%", "20%", "10%", "0%" };
		int genlock_mix_index = changed_prefs.genlock_mix / 25;
		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::Combo("##GenlockMix", &genlock_mix_index, genlock_mix_items, IM_ARRAYSIZE(genlock_mix_items)))
		{
			changed_prefs.genlock_mix = genlock_mix_index * 25;
			if (changed_prefs.genlock_mix > 255) changed_prefs.genlock_mix = 255;
		}
		
		ImGui::Checkbox("Include alpha channels in screenshots and video captures.", &changed_prefs.genlock_alpha);
		bool genlock_aspect = changed_prefs.genlock_aspect > 0;
		if (ImGui::Checkbox("Keep aspect ratio", &genlock_aspect)) {
			changed_prefs.genlock_aspect = genlock_aspect ? 1 : 0;
		}
		
		ImGui::EndDisabled();
	}
	EndGroupBox("Genlock");
	ImGui::Unindent(5.0f);
}
