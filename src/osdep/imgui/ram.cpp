#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "memory.h"
#include "autoconf.h" // Required for memoryboards and expansion_get_autoconfig_info
#include "rommgr.h"
#include "custom.h" // Required for CSMASK_ECS_AGNUS

// WinUAE sync: fix_values_memorydlg()
// When Chip RAM > 2MB, fast RAM must be limited to max 256KB total across all boards
static void fix_values_memorydlg()
{
    if (changed_prefs.chipmem.size <= 0x200000)
        return;
    int total = 0;
    for (int i = 0; i < MAX_RAM_BOARDS; i++) {
        if (changed_prefs.fastmem[i].size > 262144)
            changed_prefs.fastmem[i].size = 262144;
        total += changed_prefs.fastmem[i].size;
        if (total > 262144) {
            changed_prefs.fastmem[i].size = 0;
        }
    }
}

// Helper to find the index for a given size in an msi array
static int get_mem_index(uae_u32 size, const int *msi_array, int count) {
    for (int i = 0; i < count; ++i) {
        if (size == memsizes[msi_array[i]])
            return i;
    }
    return 0; // Default to 0 if not found
}

static int getmemsize(uae_u32 size, const int *msi)
{
    int mem_size = 0;
    if (msi == msi_fast) {
        switch (size) {
            case 0x00000000: mem_size = 0; break;
            case 0x00010000: mem_size = 1; break;
            case 0x00020000: mem_size = 2; break;
            case 0x00040000: mem_size = 3; break;
            case 0x00080000: mem_size = 4; break;
            case 0x00100000: mem_size = 5; break;
            case 0x00200000: mem_size = 6; break;
            case 0x00400000: mem_size = 7; break;
            case 0x00800000: mem_size = 8; break;
            case 0x01000000: mem_size = 9; break;
        }
    } else if (msi == msi_chip) {
        switch (size) {
            case 0x00040000: mem_size = 0; break;
            case 0x00080000: mem_size = 1; break;
            case 0x00100000: mem_size = 2; break;
            case 0x00180000: mem_size = 3; break;
            case 0x00200000: mem_size = 4; break;
            case 0x00400000: mem_size = 5; break;
            case 0x00800000: mem_size = 6; break;
        }
    } else if (msi == msi_bogo) {
        switch (size) {
            case 0x00000000: mem_size = 0; break;
            case 0x00080000: mem_size = 1; break;
            case 0x00100000: mem_size = 2; break;
            case 0x00180000: mem_size = 3; break;
            case 0x001C0000: mem_size = 4; break;
        }
    } else if (msi == msi_z3chip) {
        switch (size) {
            case 0x00000000: mem_size = 0; break;
            case 0x01000000: mem_size = 1; break;
            case 0x02000000: mem_size = 2; break;
            case 0x04000000: mem_size = 3; break;
            case 0x08000000: mem_size = 4; break;
            case 0x10000000: mem_size = 5; break;
            case 0x18000000: mem_size = 6; break;
            case 0x20000000: mem_size = 7; break;
            case 0x30000000: mem_size = 8; break;
            case 0x40000000: mem_size = 9; break;
        }
    } else {
        if (size < 0x00100000) mem_size = 0;
        else if (size < 0x00200000) mem_size = 1;
        else if (size < 0x00400000) mem_size = 2;
        else if (size < 0x00800000) mem_size = 3;
        else if (size < 0x01000000) mem_size = 4;
        else if (size < 0x02000000) mem_size = 5;
        else if (size < 0x04000000) mem_size = 6;
        else if (size < 0x08000000) mem_size = 7;
        else if (size < 0x10000000) mem_size = 8;
        else if (size < 0x20000000) mem_size = 9;
        else if (size < 0x40000000) mem_size = 10;
        else mem_size = 11; // 1GB
    }
    return mem_size;
}

// Helper to render label + slider + value text
// Layout: [Label (Fixed aligned)] [Slider (Flexible)] [Value Box (Fixed)]
static bool RamControl(const char *label, int *current_index, const int *msi_array, int count) {
    bool changed = false;
    ImGui::PushID(label);

    // Calculate Layout Dimensions
    // Use scalable units based on BUTTON_WIDTH
    float label_column_width = BUTTON_WIDTH * 1.5f; 
    float value_column_width = BUTTON_WIDTH;
    float padding = ImGui::GetStyle().ItemSpacing.x;
    
    // Available width for the whole line
    float avail_width = ImGui::GetContentRegionAvail().x;
    // Slider takes remaining space
    float slider_width = avail_width - label_column_width - value_column_width - (padding * 2);
    // Ensure slider has a sane minimum width
    if (slider_width < BUTTON_WIDTH) slider_width = BUTTON_WIDTH;

    // 1. Label (Right Aligned in column)
    ImGui::AlignTextToFramePadding();
    float current_x = ImGui::GetCursorPosX();
    float text_width = ImGui::CalcTextSize(label).x;
    
    // Position cursor to right-align text within the label_column_width
    ImGui::SetCursorPosX(current_x + label_column_width - text_width - padding);
    ImGui::Text("%s", label);
    
    // Move to start of Slider column
    ImGui::SameLine(current_x + label_column_width);

    // 2. Slider (Flexible Width)
    ImGui::SetNextItemWidth(slider_width);
    if (ImGui::SliderInt("##slider", current_index, 0, count - 1, "")) {
        changed = true;
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

    // 3. Value (Fixed Width)
    ImGui::SameLine();
    ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(value_column_width);
    char val_buf[32];
    strncpy(val_buf, memsize_names[msi_array[*current_index]], sizeof(val_buf));
    ImGui::InputText("##val", val_buf, sizeof(val_buf),
                     ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
    ImGui::EndDisabled();

    ImGui::PopID();
    return changed;
}

// WinUAE sync: Advanced RAM panel supports multiple RAM boards
// Index layout matches WinUAE's setfastram_selectmenu():
// 0 = Chip RAM
// 1 = Slow RAM
// 2..2+MAX_RAM_BOARDS-1 = Z2 Fast RAM boards
// 2+MAX_RAM_BOARDS..2+2*MAX_RAM_BOARDS-1 = Z3 Fast RAM boards
// 2+2*MAX_RAM_BOARDS = Processor Slot Fast RAM
// 2+2*MAX_RAM_BOARDS+1 = Motherboard Fast RAM
// 2+2*MAX_RAM_BOARDS+2 = 32-bit Chip RAM
#define ADVANCED_RAM_CHIP 0
#define ADVANCED_RAM_SLOW 1
#define ADVANCED_RAM_Z2_BASE 2
#define ADVANCED_RAM_Z3_BASE (ADVANCED_RAM_Z2_BASE + MAX_RAM_BOARDS)
#define ADVANCED_RAM_CPUSLOT (ADVANCED_RAM_Z2_BASE + 2 * MAX_RAM_BOARDS)
#define ADVANCED_RAM_MBLOW (ADVANCED_RAM_CPUSLOT + 1)
#define ADVANCED_RAM_Z3CHIP (ADVANCED_RAM_MBLOW + 1)
#define ADVANCED_RAM_COUNT (ADVANCED_RAM_Z3CHIP + 1)

static int current_advanced_ram_idx = 0;

static ramboard *get_selected_ramboard(int idx) {
    if (idx == ADVANCED_RAM_CHIP)
        return &changed_prefs.chipmem;
    if (idx == ADVANCED_RAM_SLOW)
        return &changed_prefs.bogomem;
    if (idx >= ADVANCED_RAM_Z2_BASE && idx < ADVANCED_RAM_Z3_BASE)
        return &changed_prefs.fastmem[idx - ADVANCED_RAM_Z2_BASE];
    if (idx >= ADVANCED_RAM_Z3_BASE && idx < ADVANCED_RAM_CPUSLOT)
        return &changed_prefs.z3fastmem[idx - ADVANCED_RAM_Z3_BASE];
    if (idx == ADVANCED_RAM_CPUSLOT)
        return &changed_prefs.mbresmem_high;
    if (idx == ADVANCED_RAM_MBLOW)
        return &changed_prefs.mbresmem_low;
    if (idx == ADVANCED_RAM_Z3CHIP)
        return &changed_prefs.z3chipmem;
    return nullptr;
}

// WinUAE sync: get_ramboard_name with size info like addadvancedram()
static void get_ramboard_name(int idx, char *buf, size_t bufsize, bool include_size = false) {
    ramboard *rb = get_selected_ramboard(idx);
    const char *name = "Unknown";
    const int *msi = nullptr;

    if (idx == ADVANCED_RAM_CHIP) {
        name = "Chip RAM";
        msi = msi_chip;
    } else if (idx == ADVANCED_RAM_SLOW) {
        name = "Slow RAM";
        msi = msi_bogo;
    } else if (idx >= ADVANCED_RAM_Z2_BASE && idx < ADVANCED_RAM_Z3_BASE) {
        int board_num = idx - ADVANCED_RAM_Z2_BASE;
        snprintf(buf, bufsize, "Z2 Fast RAM #%d", board_num + 1);
        if (include_size && rb && rb->size) {
            size_t len = strlen(buf);
            snprintf(buf + len, bufsize - len, " [%dk]", rb->size / 1024);
        }
        // WinUAE sync: Show autoconfig board name if available
        struct autoconfig_info *aci = expansion_get_autoconfig_info(&changed_prefs, ROMTYPE_RAMZ2, board_num);
        if (aci && aci->ert) {
            size_t len = strlen(buf);
            snprintf(buf + len, bufsize - len, " (%s)", aci->ert->friendlyname);
        }
        return;
    } else if (idx >= ADVANCED_RAM_Z3_BASE && idx < ADVANCED_RAM_CPUSLOT) {
        int board_num = idx - ADVANCED_RAM_Z3_BASE;
        snprintf(buf, bufsize, "Z3 Fast RAM #%d", board_num + 1);
        if (include_size && rb && rb->size) {
            size_t len = strlen(buf);
            snprintf(buf + len, bufsize - len, " [%dM]", rb->size / (1024 * 1024));
        }
        // WinUAE sync: Show autoconfig board name if available
        struct autoconfig_info *aci = expansion_get_autoconfig_info(&changed_prefs, ROMTYPE_RAMZ3, board_num);
        if (aci && aci->ert) {
            size_t len = strlen(buf);
            snprintf(buf + len, bufsize - len, " (%s)", aci->ert->friendlyname);
        }
        return;
    } else if (idx == ADVANCED_RAM_CPUSLOT) {
        name = "Processor Slot Fast RAM";
        msi = msi_mb;
    } else if (idx == ADVANCED_RAM_MBLOW) {
        name = "Motherboard Fast RAM";
        msi = msi_mb;
    } else if (idx == ADVANCED_RAM_Z3CHIP) {
        name = "32-bit Chip RAM";
        msi = msi_z3chip;
    }

    if (include_size && rb && rb->size && msi) {
        int mem_idx = getmemsize(rb->size, msi);
        snprintf(buf, bufsize, "%s (%s)", name, memsize_names[msi[mem_idx]]);
    } else {
        strncpy(buf, name, bufsize);
        buf[bufsize - 1] = '\0';
    }
}

void render_panel_ram() {
    ImGui::Indent(4.0f);
    BeginGroupBox("Memory Settings");

    int chip_idx = get_mem_index(changed_prefs.chipmem.size, msi_chip, 7);
    if (RamControl("Chip:", &chip_idx, msi_chip, 7)) {
        changed_prefs.chipmem.size = memsizes[msi_chip[chip_idx]];
        // WinUAE sync: Auto-enable ECS Agnus when Chip RAM > 512KB (OCS can't address more)
        if (!(changed_prefs.chipset_mask & CSMASK_ECS_AGNUS) && changed_prefs.chipmem.size > 0x80000) {
            changed_prefs.chipset_mask |= CSMASK_ECS_AGNUS;
        }
        // WinUAE sync: Chip RAM > 2MB conflicts with Z2 Fast RAM
        if (changed_prefs.chipmem.size > 0x200000) {
            fix_values_memorydlg();
        }
    }

    int fast_idx = get_mem_index(changed_prefs.fastmem[0].size, msi_fast, 9);
    if (RamControl("Z2 Fast:", &fast_idx, msi_fast, 9)) {
        changed_prefs.fastmem[0].size = memsizes[msi_fast[fast_idx]];
        if (changed_prefs.fastmem[0].size > 0 && changed_prefs.chipmem.size > 0x200000)
            changed_prefs.chipmem.size = 0x200000;
    }

    int mbhigh_idx = get_mem_index(changed_prefs.mbresmem_high.size, msi_mb, 9);
    if (RamControl("Processor slot:", &mbhigh_idx, msi_mb, 9)) {
        changed_prefs.mbresmem_high.size = memsizes[msi_mb[mbhigh_idx]];
        if (currprefs.mbresmem_high.size != changed_prefs.mbresmem_high.size)
            disable_resume();
    }

    int mblow_idx = get_mem_index(changed_prefs.mbresmem_low.size, msi_mb, 8);
    if (RamControl("Motherboard Fast:", &mblow_idx, msi_mb, 8)) {
        changed_prefs.mbresmem_low.size = memsizes[msi_mb[mblow_idx]];
        if (currprefs.mbresmem_low.size != changed_prefs.mbresmem_low.size)
            disable_resume();
    }

    int slow_idx = get_mem_index(changed_prefs.bogomem.size, msi_bogo, 5);
    if (RamControl("Slow:", &slow_idx, msi_bogo, 5)) {
        changed_prefs.bogomem.size = memsizes[msi_bogo[slow_idx]];
    }

    bool z3_enabled = !changed_prefs.address_space_24;
    ImGui::BeginDisabled(!z3_enabled);

    int z3fast_count = can_have_1gb() ? 12 : 11;
    int z3_idx = get_mem_index(changed_prefs.z3fastmem[0].size, msi_z3fast, z3fast_count);
    if (RamControl("Z3 Fast:", &z3_idx, msi_z3fast, z3fast_count)) {
        changed_prefs.z3fastmem[0].size = memsizes[msi_z3fast[z3_idx]];
        if (changed_prefs.z3fastmem[0].size > max_z3fastmem)
            changed_prefs.z3fastmem[0].size = max_z3fastmem;
    }

    int z3chip_count = can_have_1gb() ? 10 : 9;
    int z3chip_idx = get_mem_index(changed_prefs.z3chipmem.size, msi_z3chip, z3chip_count);
    if (RamControl("32-bit Chip:", &z3chip_idx, msi_z3chip, z3chip_count)) {
        changed_prefs.z3chipmem.size = memsizes[msi_z3chip[z3chip_idx]];
        if (changed_prefs.z3chipmem.size > max_z3fastmem)
            changed_prefs.z3chipmem.size = max_z3fastmem;
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();

    // WinUAE sync: Calculate and display 32-bit memory info like setmax32bitram()
    {
        uae_u32 size32 = 0;
        uae_u32 first = 0;
        int idx = 0;
        for (;;) {
            struct autoconfig_info *aci = expansion_get_autoconfig_data(&changed_prefs, idx);
            if (!aci)
                break;
            if (aci->start >= 0x10000000 && !first)
                first = aci->start;
            if (aci->start >= 0x10000000 && aci->start + aci->size > size32)
                size32 = aci->start + aci->size;
            idx++;
        }
        if (size32 >= first)
            size32 -= first;

        uae_u32 z3size_uae = natmem_reserved_size >= expamem_z3_pointer_uae ?
            natmem_reserved_size - expamem_z3_pointer_uae : 0;
        uae_u32 z3size_real = natmem_reserved_size >= expamem_z3_pointer_real ?
            natmem_reserved_size - expamem_z3_pointer_real : 0;

        ImGui::Text("Configured: %dM, Total: %dM, Z3(UAE): %dM, Z3(Real): %dM",
            size32 / (1024 * 1024),
            (natmem_reserved_size - 256 * 1024 * 1024) / (1024 * 1024),
            z3size_uae / (1024 * 1024),
            z3size_real / (1024 * 1024));
    }
    ImGui::Spacing();

    EndGroupBox("Memory Settings");

    BeginGroupBox("Advanced Memory Settings");

    // WinUAE sync: Validate current selection is still valid (e.g., after switching to 24-bit mode)
    if (!z3_enabled && current_advanced_ram_idx >= ADVANCED_RAM_Z3_BASE) {
        current_advanced_ram_idx = ADVANCED_RAM_CHIP;
    }

    // Top Row: Selector ...... Slider [ Size ]
    char current_name_buf[128];
    get_ramboard_name(current_advanced_ram_idx, current_name_buf, sizeof(current_name_buf), true);

    ImGui::SetNextItemWidth(BUTTON_WIDTH * 3);
    if (ImGui::BeginCombo("##RamSelector", current_name_buf)) {
        // WinUAE sync: Build dropdown like setfastram_selectmenu()
        char item_buf[128];

        // Chip RAM
        get_ramboard_name(ADVANCED_RAM_CHIP, item_buf, sizeof(item_buf), true);
        if (ImGui::Selectable(item_buf, current_advanced_ram_idx == ADVANCED_RAM_CHIP))
            current_advanced_ram_idx = ADVANCED_RAM_CHIP;
        if (current_advanced_ram_idx == ADVANCED_RAM_CHIP) ImGui::SetItemDefaultFocus();

        // Slow RAM
        get_ramboard_name(ADVANCED_RAM_SLOW, item_buf, sizeof(item_buf), true);
        if (ImGui::Selectable(item_buf, current_advanced_ram_idx == ADVANCED_RAM_SLOW))
            current_advanced_ram_idx = ADVANCED_RAM_SLOW;
        if (current_advanced_ram_idx == ADVANCED_RAM_SLOW) ImGui::SetItemDefaultFocus();

        // Z2 Fast RAM boards (multiple)
        for (int i = 0; i < MAX_RAM_BOARDS; i++) {
            int idx = ADVANCED_RAM_Z2_BASE + i;
            get_ramboard_name(idx, item_buf, sizeof(item_buf), true);
            if (ImGui::Selectable(item_buf, current_advanced_ram_idx == idx))
                current_advanced_ram_idx = idx;
            if (current_advanced_ram_idx == idx) ImGui::SetItemDefaultFocus();
        }

        // Z3 boards only available in 32-bit mode
        if (z3_enabled) {
            // Z3 Fast RAM boards (multiple)
            for (int i = 0; i < MAX_RAM_BOARDS; i++) {
                int idx = ADVANCED_RAM_Z3_BASE + i;
                get_ramboard_name(idx, item_buf, sizeof(item_buf), true);
                if (ImGui::Selectable(item_buf, current_advanced_ram_idx == idx))
                    current_advanced_ram_idx = idx;
                if (current_advanced_ram_idx == idx) ImGui::SetItemDefaultFocus();
            }

            // Processor Slot Fast RAM
            get_ramboard_name(ADVANCED_RAM_CPUSLOT, item_buf, sizeof(item_buf), true);
            if (ImGui::Selectable(item_buf, current_advanced_ram_idx == ADVANCED_RAM_CPUSLOT))
                current_advanced_ram_idx = ADVANCED_RAM_CPUSLOT;
            if (current_advanced_ram_idx == ADVANCED_RAM_CPUSLOT) ImGui::SetItemDefaultFocus();

            // Motherboard Fast RAM
            get_ramboard_name(ADVANCED_RAM_MBLOW, item_buf, sizeof(item_buf), true);
            if (ImGui::Selectable(item_buf, current_advanced_ram_idx == ADVANCED_RAM_MBLOW))
                current_advanced_ram_idx = ADVANCED_RAM_MBLOW;
            if (current_advanced_ram_idx == ADVANCED_RAM_MBLOW) ImGui::SetItemDefaultFocus();

            // 32-bit Chip RAM
            get_ramboard_name(ADVANCED_RAM_Z3CHIP, item_buf, sizeof(item_buf), true);
            if (ImGui::Selectable(item_buf, current_advanced_ram_idx == ADVANCED_RAM_Z3CHIP))
                current_advanced_ram_idx = ADVANCED_RAM_Z3CHIP;
            if (current_advanced_ram_idx == ADVANCED_RAM_Z3CHIP) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

    // Update rb pointer after potential index change
    ramboard *rb = get_selected_ramboard(current_advanced_ram_idx);

    if (rb) {
        const int *current_msi = nullptr;
        int msi_count = 0;
        int zram = 0; // 0=none, 2=Z2, 3=Z3 (for memory board dropdown)

        // WinUAE sync: Determine MSI array and count based on selection
        if (current_advanced_ram_idx == ADVANCED_RAM_CHIP) {
            current_msi = msi_chip;
            msi_count = 7;
        } else if (current_advanced_ram_idx == ADVANCED_RAM_SLOW) {
            current_msi = msi_bogo;
            msi_count = 5;
        } else if (current_advanced_ram_idx >= ADVANCED_RAM_Z2_BASE && current_advanced_ram_idx < ADVANCED_RAM_Z3_BASE) {
            current_msi = msi_fast;
            msi_count = 9;
            zram = 2;
        } else if (current_advanced_ram_idx >= ADVANCED_RAM_Z3_BASE && current_advanced_ram_idx < ADVANCED_RAM_CPUSLOT) {
            current_msi = msi_z3fast;
            msi_count = can_have_1gb() ? 12 : 11;
            zram = 3;
        } else if (current_advanced_ram_idx == ADVANCED_RAM_CPUSLOT) {
            current_msi = msi_mb;
            msi_count = MAX_MBH_MEM + 1;
        } else if (current_advanced_ram_idx == ADVANCED_RAM_MBLOW) {
            current_msi = msi_mb;
            msi_count = MAX_MBL_MEM + 1;
        } else if (current_advanced_ram_idx == ADVANCED_RAM_Z3CHIP) {
            current_msi = msi_z3chip;
            msi_count = can_have_1gb() ? 10 : 9;
        }

        if (current_msi) {
             ImGui::SameLine();
             ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);

             int current_size_idx = get_mem_index(rb->size, current_msi, msi_count);

             if (ImGui::SliderInt("##AdvSlider", &current_size_idx, 0, msi_count - 1, "")) {
                 rb->size = memsizes[current_msi[current_size_idx]];
                 // WinUAE sync: specific logic ref checks
                 if (current_advanced_ram_idx == ADVANCED_RAM_CHIP) {
                     // Auto-enable ECS Agnus when Chip RAM > 512KB
                     if (!(changed_prefs.chipset_mask & CSMASK_ECS_AGNUS) && rb->size > 0x80000) {
                         changed_prefs.chipset_mask |= CSMASK_ECS_AGNUS;
                     }
                     // Chip RAM > 2MB conflicts with Z2 Fast RAM
                     if (rb->size > 0x200000) {
                         fix_values_memorydlg();
                     }
                 } else if (current_advanced_ram_idx >= ADVANCED_RAM_Z2_BASE && current_advanced_ram_idx < ADVANCED_RAM_Z3_BASE) {
                     // Z2 Fast: if enabled and Chip > 2MB, reduce Chip
                     if (rb->size > 0 && changed_prefs.chipmem.size > 0x200000)
                         changed_prefs.chipmem.size = 0x200000;
                 } else if (current_advanced_ram_idx >= ADVANCED_RAM_Z3_BASE && current_advanced_ram_idx < ADVANCED_RAM_CPUSLOT) {
                     // Z3 Fast: enforce max limit
                     if (rb->size > max_z3fastmem)
                         rb->size = max_z3fastmem;
                 } else if (current_advanced_ram_idx == ADVANCED_RAM_Z3CHIP) {
                     // 32-bit Chip: enforce max limit
                     if (rb->size > max_z3fastmem)
                         rb->size = max_z3fastmem;
                 }
             }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

             ImGui::SameLine();
             ImGui::SetNextItemWidth(BUTTON_WIDTH);
             char size_buf[32];
             strncpy(size_buf, memsize_names[current_msi[current_size_idx]], sizeof(size_buf));
             ImGui::InputText("##AdvSize", size_buf, sizeof(size_buf),
                              ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
        }

        ImGui::Spacing();

        if (ImGui::BeginTable("AdvRamDetails", 2)) {
            // Adjust column widths: Left column needs more space for wider controls
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            // Right column (flags) can be smaller
            ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            
            ImGui::TableNextColumn();

            // Left Column: Inputs
            bool edit_ac = rb->autoconfig_inuse;

            // Manufacturer & Product on same line
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Manufacturer");
            ImGui::SameLine();
            
            ImGui::BeginDisabled(!edit_ac);
            char manuf_buf[8];
            snprintf(manuf_buf, sizeof(manuf_buf), "%04X", rb->manufacturer);
            ImGui::SetNextItemWidth(BUTTON_WIDTH);
            if (ImGui::InputText("##Manuf", manuf_buf, 8, ImGuiInputTextFlags_CharsHexadecimal)) {
                rb->manufacturer = (uae_u16) strtoul(manuf_buf, nullptr, 16);
                // Update Autoconfig bytes
                rb->autoconfig[4] = (rb->manufacturer >> 8) & 0xff;
                rb->autoconfig[5] = rb->manufacturer & 0xff;
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);

            ImGui::SameLine();
            ImGui::Text("Product");
            ImGui::SameLine();
            char prod_buf[8];
            snprintf(prod_buf, sizeof(prod_buf), "%02X", rb->product);
            ImGui::SetNextItemWidth(BUTTON_WIDTH / 2);
            if (ImGui::InputText("##Product", prod_buf, 8, ImGuiInputTextFlags_CharsHexadecimal)) {
                rb->product = (uae_u8) strtoul(prod_buf, nullptr, 16);
                // Update Autoconfig bytes
                rb->autoconfig[1] = rb->product;
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
            ImGui::EndDisabled();

            // Autoconfig Data
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Autoconfig data");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(BUTTON_WIDTH * 3); // Wider to match WinUAE visual
            
            // Format autoconfig bytes
            char autoconfig_buf[64] = "";
            // Logic to fetch valid autoconfig data if rb->autoconfig is empty or we want to show default
            // In WinUAE, it shows rb->autoconfig bytes.
            // If they are all zero, it might try to fetch defaults.
            // Let's format rb->autoconfig.
            char *p = autoconfig_buf;
            for (int i = 0; i < 16; i++) {
                 if (i > 0) {
                     *p++ = '.';
                 }
                 if (i < 12) // WinUAE usually shows first 12 bytes or so
                    snprintf(p, 3, "%02X", rb->autoconfig[i]);
                 p += 2;
            }
            ImGui::InputText("##Autoconfig", autoconfig_buf, 64, ImGuiInputTextFlags_ReadOnly);
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);

            // WinUAE sync: Memory Board Dropdown - only enabled for Z2/Z3 Fast RAM
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Memory board");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(BUTTON_WIDTH * 3);

            // zram is set above: 0=none, 2=Z2, 3=Z3
            bool enable_board_select = (zram == 2 || zram == 3);
            if (!enable_board_select) ImGui::BeginDisabled();

            // WinUAE sync: Find current board name with manufacturer prefix
            char current_board_label[128] = "-";
            for (int i = 0; memoryboards[i].name; i++) {
                if (memoryboards[i].z == zram &&
                    memoryboards[i].manufacturer == rb->manufacturer &&
                    memoryboards[i].product == rb->product) {
                    // WinUAE format: "Manufacturer Name"
                    snprintf(current_board_label, sizeof(current_board_label), "%s %s",
                             memoryboards[i].man, memoryboards[i].name);
                    break;
                }
            }

            if (ImGui::BeginCombo("##MemBoardSelect", current_board_label)) {
                // Get the board index for autoconfig info
                int board_idx = 0;
                if (zram == 2)
                    board_idx = current_advanced_ram_idx - ADVANCED_RAM_Z2_BASE;
                else if (zram == 3)
                    board_idx = current_advanced_ram_idx - ADVANCED_RAM_Z3_BASE;

                // "-" option to clear selection
                if (ImGui::Selectable("-", rb->manufacturer == 0 && rb->product == 0)) {
                    rb->manufacturer = 0;
                    rb->product = 0;
                    rb->manual_config = false;
                    memset(rb->autoconfig, 0, sizeof(rb->autoconfig));
                    // WinUAE sync: Re-fetch default autoconfig
                    struct autoconfig_info *aci = expansion_get_autoconfig_info(&changed_prefs,
                        zram == 2 ? ROMTYPE_RAMZ2 : ROMTYPE_RAMZ3, board_idx);
                    if (aci) {
                         memcpy(rb->autoconfig, aci->autoconfig_bytes, sizeof(rb->autoconfig));
                    }
                }

                // WinUAE sync: List all matching boards with manufacturer prefix
                char board_label[128];
                for (int i = 0; memoryboards[i].name; i++) {
                    if (memoryboards[i].z == zram) {
                        bool is_selected = (memoryboards[i].manufacturer == rb->manufacturer &&
                                          memoryboards[i].product == rb->product);
                        // WinUAE format: "Manufacturer Name"
                        snprintf(board_label, sizeof(board_label), "%s %s",
                                 memoryboards[i].man, memoryboards[i].name);
                        if (ImGui::Selectable(board_label, is_selected)) {
                            // WinUAE sync: Handle autoconfig vs manual boards
                            if (memoryboards[i].manufacturer != 0xffff) {
                                // Normal autoconfig board
                                rb->manual_config = false;
                                if (memoryboards[i].manufacturer) {
                                    rb->manufacturer = memoryboards[i].manufacturer;
                                    rb->product = memoryboards[i].product;
                                }
                            } else {
                                // Manual configuration board (manufacturer 0xffff)
                                rb->autoconfig_inuse = false;
                                rb->manual_config = true;
                            }
                            // Set address if manual and board has address
                            if (rb->manual_config && memoryboards[i].address) {
                                rb->start_address = memoryboards[i].address;
                                rb->end_address = memoryboards[i].address + rb->size - 1;
                            }
                            // Copy autoconfig data
                            memcpy(rb->autoconfig, memoryboards[i].autoconfig, sizeof(rb->autoconfig));
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

            if (!enable_board_select) ImGui::EndDisabled();

            // Address Range
            // Always show label to keep layout stable? Or only when manual?
            // WinUAE shows it disabled if not Manual.
            bool manual_active = rb->manual_config;
            ImGui::BeginDisabled(!manual_active);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Address range");
            ImGui::SameLine();
            char addr_buf[16];
            snprintf(addr_buf, sizeof(addr_buf), "%08X", rb->start_address);
            ImGui::SetNextItemWidth(BUTTON_WIDTH);
            if (ImGui::InputText("##Address", addr_buf, 16, ImGuiInputTextFlags_CharsHexadecimal)) {
                rb->start_address = strtoul(addr_buf, nullptr, 16);
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
            // Second box for end address
            ImGui::SameLine();
            ImGui::SetNextItemWidth(BUTTON_WIDTH);
            char end_addr_buf[16];
            uae_u32 end_addr = rb->start_address + rb->size;
            if (end_addr > 0) end_addr -= 1; // Inclusive range usually
            if (rb->manual_config) {
                 if (rb->end_address > rb->start_address)
                     end_addr = rb->end_address;
                 else
                     end_addr = rb->start_address + rb->size - 1; // Default
            }
            snprintf(end_addr_buf, sizeof(end_addr_buf), "%08X", end_addr);
            ImGui::InputText("##AddressEnd", end_addr_buf, 16, ImGuiInputTextFlags_ReadOnly);
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
            ImGui::EndDisabled();

            ImGui::TableNextColumn();

            // Right Column: Flags
            // Order: Edit Autoconfig, Manual, DMA, 16-bit, Slow

            if (AmigaCheckbox("Edit Autoconfig data", &edit_ac)) {
                 rb->autoconfig_inuse = edit_ac;
                 // If enabling, ensure autoconfig bytes are synced from current Manuf/Product if currently zero?
                 // Or just trust the current state.
            }
            AmigaCheckbox("Manual configuration", &rb->manual_config);

            bool dma_capable = !rb->nodma;
            if (AmigaCheckbox("DMA Capable", &dma_capable)) rb->nodma = !dma_capable;

            AmigaCheckbox("Force 16-bit", &rb->force16bit);
            AmigaCheckbox("Slow RAM", &rb->chipramtiming);

            // Z3 Mapping Mode - Bottom Right
            ImGui::Spacing();
            ImGui::Text("Z3 mapping mode:");
            const char *z3_mapping_items[] = {"Automatic (*)", "UAE", "Real"};
            ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
            ImGui::BeginDisabled(!z3_enabled);
            ImGui::Combo("##Z3Mapping", &changed_prefs.z3_mapping_mode, z3_mapping_items,
                         IM_ARRAYSIZE(z3_mapping_items));
            ImGui::EndDisabled();
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
            ImGui::EndTable();
        }
    }

    EndGroupBox("Advanced Memory Settings");

    copycpuboardmem(true);
}
