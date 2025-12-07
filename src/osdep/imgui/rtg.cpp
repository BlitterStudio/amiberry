#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_rtg()
{
	const char* rtg_boards[] = { "-", "UAE Zorro II", "UAE Zorro III", "PCI bridgeboard" };
	ImGui::Combo("RTG Graphics Board", &changed_prefs.rtgboards[0].rtgmem_type, rtg_boards, IM_ARRAYSIZE(rtg_boards));

    int vram = static_cast<int>(changed_prefs.rtgboards[0].rtgmem_size);
	if (ImGui::SliderInt("VRAM size", &vram, 0, 0x10000000))
        changed_prefs.rtgboards[0].rtgmem_size = static_cast<uae_u32>(vram);
	ImGui::Checkbox("Scale if smaller than display size setting", (bool*)&changed_prefs.gf[1].gfx_filter_autoscale);
	ImGui::Checkbox("Always scale in windowed mode", &changed_prefs.rtgallowscaling);
	ImGui::Checkbox("Always center", (bool*)&changed_prefs.gf[1].gfx_filter_autoscale);
	ImGui::Checkbox("Hardware vertical blank interrupt", &changed_prefs.rtg_hardwareinterrupt);
	ImGui::Checkbox("Hardware sprite emulation", &changed_prefs.rtg_hardwaresprite);
	ImGui::Checkbox("Multithreaded", &changed_prefs.rtg_multithread);
	const char* rtg_refreshrates[] = { "Chipset", "Default", "50", "60", "70", "75" };
	ImGui::Combo("Refresh rate", &changed_prefs.rtgvblankrate, rtg_refreshrates, IM_ARRAYSIZE(rtg_refreshrates));
	const char* rtg_buffermodes[] = { "Double buffering", "Triple buffering" };
	ImGui::Combo("Buffer mode", &changed_prefs.gfx_apmode[1].gfx_backbuffers, rtg_buffermodes, IM_ARRAYSIZE(rtg_buffermodes));
	const char* rtg_aspectratios[] = { "Disabled", "Automatic" };
	ImGui::Combo("Aspect Ratio", &changed_prefs.rtgscaleaspectratio, rtg_aspectratios, IM_ARRAYSIZE(rtg_aspectratios));
	const char* rtg_16bit_modes[] = { "(15/16bit)", "All", "R5G6B5PC (*)", "R5G5B5PC", "R5G6B5", "R5G5B5", "B5G6R5PC", "B5G5R5PC" };
	ImGui::Combo("16-bit modes", (int*)&changed_prefs.picasso96_modeflags, rtg_16bit_modes, IM_ARRAYSIZE(rtg_16bit_modes));
	const char* rtg_32bit_modes[] = { "(32bit)", "All", "A8R8G8B8", "A8B8G8R8", "R8G8B8A8 (*)", "B8G8R8A8" };
	ImGui::Combo("32-bit modes", (int*)&changed_prefs.picasso96_modeflags, rtg_32bit_modes, IM_ARRAYSIZE(rtg_32bit_modes));
}
