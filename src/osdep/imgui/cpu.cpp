#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_cpu()
{
	ImGui::RadioButton("68000", &changed_prefs.cpu_model, 68000);
	ImGui::RadioButton("68010", &changed_prefs.cpu_model, 68010);
	ImGui::RadioButton("68020", &changed_prefs.cpu_model, 68020);
	ImGui::RadioButton("68030", &changed_prefs.cpu_model, 68030);
	ImGui::RadioButton("68040", &changed_prefs.cpu_model, 68040);
	ImGui::RadioButton("68060", &changed_prefs.cpu_model, 68060);
	ImGui::Checkbox("24-bit addressing", &changed_prefs.address_space_24);
	ImGui::Checkbox("More compatible", &changed_prefs.cpu_compatible);
	ImGui::Checkbox("Data cache", &changed_prefs.cpu_data_cache);
	ImGui::Checkbox("JIT", (bool*)&changed_prefs.cachesize);

	ImGui::Separator();
	ImGui::Text("MMU");
	ImGui::RadioButton("None##MMU", &changed_prefs.mmu_model, 0);
	ImGui::RadioButton("MMU", &changed_prefs.mmu_model, changed_prefs.cpu_model);
	ImGui::Checkbox("EC", &changed_prefs.mmu_ec);

	ImGui::Separator();
	ImGui::Text("FPU");
	ImGui::RadioButton("None##FPU", &changed_prefs.fpu_model, 0);
	ImGui::RadioButton("68881", &changed_prefs.fpu_model, 68881);
	ImGui::RadioButton("68882", &changed_prefs.fpu_model, 68882);
	ImGui::RadioButton("CPU internal", &changed_prefs.fpu_model, changed_prefs.cpu_model);
	ImGui::Checkbox("More compatible##FPU", &changed_prefs.fpu_strict);

	ImGui::Separator();
	ImGui::Text("CPU Speed");
	ImGui::RadioButton("Fastest Possible", &changed_prefs.m68k_speed, -1);
	ImGui::RadioButton("A500/A1200 or cycle exact", &changed_prefs.m68k_speed, 0);
	ImGui::SliderInt("CPU Speed", (int*)&changed_prefs.m68k_speed_throttle, 0, 5000);
	ImGui::SliderInt("CPU Idle", &changed_prefs.cpu_idle, 0, 120);

	ImGui::Separator();
	ImGui::Text("Cycle-Exact CPU Emulation Speed");
	const char* freq_items[] = { "1x", "2x (A500)", "4x (A1200)", "8x", "16x" };
	ImGui::Combo("CPU Frequency", &changed_prefs.cpu_clock_multiplier, freq_items, IM_ARRAYSIZE(freq_items));
	ImGui::Checkbox("Multi-threaded CPU", &changed_prefs.cpu_thread);

	ImGui::Separator();
	ImGui::Text("PowerPC CPU Options");
	ImGui::Checkbox("PPC emulation", (bool*)&changed_prefs.ppc_mode);
	ImGui::SliderInt("Stopped M68K CPU Idle", &changed_prefs.ppc_cpu_idle, 0, 10);

	ImGui::Separator();
	ImGui::Text("Advanced JIT Settings");
	ImGui::SliderInt("Cache size", &changed_prefs.cachesize, 0, 8192);
	ImGui::Checkbox("FPU Support##JIT", &changed_prefs.compfpu);
	ImGui::Checkbox("Constant jump", &changed_prefs.comp_constjump);
	ImGui::Checkbox("Hard flush", &changed_prefs.comp_hardflush);
	ImGui::RadioButton("Direct##memaccess", &changed_prefs.comptrustbyte, 0);
	ImGui::RadioButton("Indirect##memaccess", &changed_prefs.comptrustbyte, 1);
	ImGui::Checkbox("No flags", &changed_prefs.compnf);
	ImGui::Checkbox("Catch unexpected exceptions", &changed_prefs.comp_catchfault);
}
