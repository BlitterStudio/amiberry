#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

void render_panel_ram()
{
    // Use temporary ints to avoid aliasing issues with ImGui writing into uae_u32 fields
    int chip = static_cast<int>(changed_prefs.chipmem.size);
    int slow = static_cast<int>(changed_prefs.bogomem.size);
    int z2fast0 = static_cast<int>(changed_prefs.fastmem[0].size);
    int z3fast0 = static_cast<int>(changed_prefs.z3fastmem[0].size);
    int z3chip = static_cast<int>(changed_prefs.z3chipmem.size);
    int mb_low = static_cast<int>(changed_prefs.mbresmem_low.size);
    int mb_high = static_cast<int>(changed_prefs.mbresmem_high.size);

    if (ImGui::SliderInt("Chip", &chip, 0, 0x800000))
        changed_prefs.chipmem.size = static_cast<uae_u32>(chip);
    if (ImGui::SliderInt("Slow", &slow, 0, 0x180000))
        changed_prefs.bogomem.size = static_cast<uae_u32>(slow);
    if (ImGui::SliderInt("Z2 Fast", &z2fast0, 0, 0x800000))
        changed_prefs.fastmem[0].size = static_cast<uae_u32>(z2fast0);
    if (ImGui::SliderInt("Z3 Fast", &z3fast0, 0, 0x40000000))
        changed_prefs.z3fastmem[0].size = static_cast<uae_u32>(z3fast0);
    if (ImGui::SliderInt("32-bit Chip RAM", &z3chip, 0, 0x40000000))
        changed_prefs.z3chipmem.size = static_cast<uae_u32>(z3chip);
    if (ImGui::SliderInt("Motherboard Fast RAM", &mb_low, 0, 0x8000000))
        changed_prefs.mbresmem_low.size = static_cast<uae_u32>(mb_low);
    if (ImGui::SliderInt("Processor slot Fast RAM", &mb_high, 0, 0x8000000))
        changed_prefs.mbresmem_high.size = static_cast<uae_u32>(mb_high);

    const char* z3_mapping_items[] = { "Automatic (*)", "UAE (0x10000000)", "Real (0x40000000)" };
    ImGui::Combo("Z3 Mapping Mode", &changed_prefs.z3_mapping_mode, z3_mapping_items, IM_ARRAYSIZE(z3_mapping_items));
}
