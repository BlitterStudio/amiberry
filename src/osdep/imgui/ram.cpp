#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

// ImGui RAM panel
// NOTE: Large RAM sizes are expressed in MiB on the sliders to keep the
// integer ranges small and within ImGui's SliderInt S32 constraints.
// Values are converted to/from bytes when writing to changed_prefs.

static inline int bytes_to_mib(uae_u32 bytes)
{
    return static_cast<int>(bytes / (1024 * 1024));
}

static inline uae_u32 mib_to_bytes(int mib)
{
    if (mib <= 0)
        return 0;
    // 32-bit safe conversion: (up to a few GB, but our UI caps well below)
    return static_cast<uae_u32>(mib) * 1024u * 1024u;
}

void render_panel_ram()
{
    // Small ranges: keep byte units directly
    int chip = static_cast<int>(changed_prefs.chipmem.size);
    int slow = static_cast<int>(changed_prefs.bogomem.size);
    int z2fast0 = static_cast<int>(changed_prefs.fastmem[0].size);

    if (ImGui::SliderInt("Chip", &chip, 0, 0x800000))
        changed_prefs.chipmem.size = static_cast<uae_u32>(chip);
    if (ImGui::SliderInt("Slow", &slow, 0, 0x180000))
        changed_prefs.bogomem.size = static_cast<uae_u32>(slow);
    if (ImGui::SliderInt("Z2 Fast", &z2fast0, 0, 0x800000))
        changed_prefs.fastmem[0].size = static_cast<uae_u32>(z2fast0);

    // Large ranges: use MiB units in UI, bytes in prefs

    // Z3 Fast
    int z3fast_mib = bytes_to_mib(changed_prefs.z3fastmem[0].size);
    // Match classic GUI options based on memsizes/msi_z3fast (0..1GB)
    int z3fast_mib_max = 1024; // 1 GiB max, well within S32/2
    if (ImGui::SliderInt("Z3 Fast (MiB)", &z3fast_mib, 0, z3fast_mib_max))
    {
        uae_u32 new_bytes = mib_to_bytes(z3fast_mib);
        if (max_z3fastmem != 0 && new_bytes > max_z3fastmem)
            new_bytes = max_z3fastmem;
        changed_prefs.z3fastmem[0].size = new_bytes;
    }

    // 32-bit Chip RAM
    int z3chip_mib = bytes_to_mib(changed_prefs.z3chipmem.size);
    int z3chip_mib_max = 1024; // mirror Z3 Fast upper bound for UI
    if (ImGui::SliderInt("32-bit Chip RAM (MiB)", &z3chip_mib, 0, z3chip_mib_max))
    {
        uae_u32 new_bytes = mib_to_bytes(z3chip_mib);
        if (max_z3fastmem != 0 && new_bytes > max_z3fastmem)
            new_bytes = max_z3fastmem;
        changed_prefs.z3chipmem.size = new_bytes;
    }

    // Motherboard / Processor slot Fast RAM
    int mb_low_mib = bytes_to_mib(changed_prefs.mbresmem_low.size);
    int mb_high_mib = bytes_to_mib(changed_prefs.mbresmem_high.size);
    // Classic GUI uses msi_mb[] that goes up to 128 MiB; use a sensible cap
    int mb_mib_max = 128;

    if (ImGui::SliderInt("Motherboard Fast RAM (MiB)", &mb_low_mib, 0, mb_mib_max))
    {
        uae_u32 new_bytes = mib_to_bytes(mb_low_mib);
        changed_prefs.mbresmem_low.size = new_bytes;
        if (currprefs.mbresmem_low.size != changed_prefs.mbresmem_low.size)
            disable_resume();
    }

    if (ImGui::SliderInt("Processor slot Fast RAM (MiB)", &mb_high_mib, 0, mb_mib_max))
    {
        uae_u32 new_bytes = mib_to_bytes(mb_high_mib);
        changed_prefs.mbresmem_high.size = new_bytes;
        if (currprefs.mbresmem_high.size != changed_prefs.mbresmem_high.size)
            disable_resume();
    }

    const char* z3_mapping_items[] = { "Automatic (*)", "UAE (0x10000000)", "Real (0x40000000)" };
    ImGui::Combo("Z3 Mapping Mode", &changed_prefs.z3_mapping_mode, z3_mapping_items, IM_ARRAYSIZE(z3_mapping_items));
}
