#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"

void render_panel_cpu()
{
    const float slider_width = 150.0f; 
    
    // Global padding for the whole panel
    ImGui::Dummy(ImVec2(0.0f, 5.0f)); 
    ImGui::Indent(5.0f);

	ImGui::BeginGroup(); // Left Column
	{
		BeginGroupBox("CPU");
		ImGui::RadioButton("68000", &changed_prefs.cpu_model, 68000);
		ImGui::RadioButton("68010", &changed_prefs.cpu_model, 68010);
		ImGui::RadioButton("68020", &changed_prefs.cpu_model, 68020);
		ImGui::RadioButton("68030", &changed_prefs.cpu_model, 68030);
		ImGui::RadioButton("68040", &changed_prefs.cpu_model, 68040);
		ImGui::RadioButton("68060", &changed_prefs.cpu_model, 68060);
		ImGui::Checkbox("24-bit addressing", &changed_prefs.address_space_24);
		ImGui::Checkbox("More compatible", &changed_prefs.cpu_compatible);
		ImGui::Checkbox("Data cache emulation", &changed_prefs.cpu_data_cache);
		ImGui::Checkbox("JIT", (bool*)&changed_prefs.cachesize);
		EndGroupBox("CPU");

		BeginGroupBox("MMU");
		ImGui::RadioButton("None##MMU", &changed_prefs.mmu_model, 0);
		ImGui::RadioButton("MMU", &changed_prefs.mmu_model, changed_prefs.cpu_model);
		ImGui::Checkbox("EC", &changed_prefs.mmu_ec);
		EndGroupBox("MMU");

		BeginGroupBox("FPU");
		ImGui::RadioButton("None##FPU", &changed_prefs.fpu_model, 0);
		ImGui::RadioButton("68881", &changed_prefs.fpu_model, 68881);
		ImGui::RadioButton("68882", &changed_prefs.fpu_model, 68882);
		ImGui::RadioButton("CPU internal", &changed_prefs.fpu_model, changed_prefs.cpu_model);
		ImGui::Checkbox("More compatible##FPU", &changed_prefs.fpu_strict);
		EndGroupBox("FPU");
	}
	ImGui::EndGroup();

	ImGui::SameLine();
    ImGui::Dummy(ImVec2(10.0f, 0.0f)); // Spacing between columns
    ImGui::SameLine();

	ImGui::BeginGroup(); // Right Column
	{
		BeginGroupBox("CPU Emulation Speed");
		ImGui::RadioButton("Fastest Possible", &changed_prefs.m68k_speed, -1);
		ImGui::RadioButton("A500/A1200 or cycle-exact", &changed_prefs.m68k_speed, 0);
		
		ImGui::AlignTextToFramePadding();
		ImGui::Text("CPU Speed"); 
		ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
		ImGui::SliderInt("##CPU Speed", (int*)&changed_prefs.m68k_speed_throttle, 0, 5000);
		
		ImGui::AlignTextToFramePadding();
		ImGui::Text("CPU Idle ");
		ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
		ImGui::SliderInt("##CPU Idle", &changed_prefs.cpu_idle, 0, 120);
		EndGroupBox("CPU Emulation Speed");

		BeginGroupBox("Cycle-exact CPU Emulation Speed");
		const char* freq_items[] = { "1x", "2x (A500)", "4x (A1200)", "8x", "16x" }; 
		ImGui::AlignTextToFramePadding();
		ImGui::Text("CPU Frequency");
		ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
		ImGui::Combo("##CPU Frequency", &changed_prefs.cpu_clock_multiplier, freq_items, IM_ARRAYSIZE(freq_items));
		ImGui::Checkbox("Multi-threaded CPU", &changed_prefs.cpu_thread); 
		EndGroupBox("Cycle-exact CPU Emulation Speed");

		BeginGroupBox("PPC CPU Options");
		ImGui::Checkbox("PPC emulation", (bool*)&changed_prefs.ppc_mode);
		
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Stopped M68K CPU idle mode");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
		ImGui::SliderInt("##PPC Idle", &changed_prefs.ppc_cpu_idle, 0, 10);
		EndGroupBox("PPC CPU Options");

		BeginGroupBox("Advanced JIT Settings");
		
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Cache size");
		ImGui::SameLine();
        ImGui::SetNextItemWidth(slider_width);
		ImGui::SliderInt("##Cache size", &changed_prefs.cachesize, 0, 8192);
		
		ImGui::Checkbox("FPU Support##JIT", &changed_prefs.compfpu);
		ImGui::SameLine();
		ImGui::Checkbox("Constant jump", &changed_prefs.comp_constjump);
		ImGui::SameLine();
		ImGui::Checkbox("Hard flush", &changed_prefs.comp_hardflush);
		
		ImGui::RadioButton("Direct##memaccess", &changed_prefs.comptrustbyte, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Indirect##memaccess", &changed_prefs.comptrustbyte, 1);
		ImGui::SameLine();
		ImGui::Checkbox("No flags", &changed_prefs.compnf);
		
		ImGui::Checkbox("Catch unexpected exceptions", &changed_prefs.comp_catchfault);
		EndGroupBox("Advanced JIT Settings");
	}
	ImGui::EndGroup();
    
    ImGui::Unindent(5.0f);
}
