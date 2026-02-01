#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "memory.h"
#include "autoconf.h" // Required for memoryboards and expansion_get_autoconfig_info
#include "rommgr.h"

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

static int current_advanced_ram_idx = 0;

static ramboard *get_selected_ramboard(int idx) {
    // Need to handle missing indices gracefully
    switch (idx) {
        case 0: return &changed_prefs.chipmem;
        case 1: return &changed_prefs.bogomem;
        case 2: return &changed_prefs.fastmem[0]; // Z2 Fast
        case 3: return &changed_prefs.z3fastmem[0];
        case 4: return &changed_prefs.z3chipmem; // 32-bit Chip
        case 5: return &changed_prefs.mbresmem_low; // MB Fast
        case 6: return &changed_prefs.mbresmem_high; // Slot Fast
        default: return nullptr;
    }
}

static const char *get_ramboard_name(int idx) {
    switch (idx) {
        case 0: return "Chip RAM";
        case 1: return "Slow RAM";
        case 2: return "Z2 Fast RAM";
        case 3: return "Z3 Fast RAM";
        case 4: return "32-bit Chip RAM";
        case 5: return "Motherboard Fast RAM";
        case 6: return "Processor Slot Fast RAM";
        default: return "Unknown";
    }
}

void render_panel_ram() {
    ImGui::Indent(4.0f);
    BeginGroupBox("Memory Settings");

    int chip_idx = get_mem_index(changed_prefs.chipmem.size, msi_chip, 7);
    if (RamControl("Chip:", &chip_idx, msi_chip, 7)) {
        changed_prefs.chipmem.size = memsizes[msi_chip[chip_idx]];
        if (changed_prefs.chipmem.size > 0x200000 && changed_prefs.fastmem[0].size > 0)
            changed_prefs.fastmem[0].size = 0;
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
    ImGui::Text("Configured 32-bit address space: Z3 available: %dM", max_z3fastmem / 1024 / 1024);
    ImGui::Spacing();

    EndGroupBox("Memory Settings");

    BeginGroupBox("Advanced Memory Settings");

    ramboard *rb = get_selected_ramboard(current_advanced_ram_idx);

    // Top Row: Selector ...... Slider [ Size ]
    ImGui::SetNextItemWidth(BUTTON_WIDTH * 3);
    if (ImGui::BeginCombo("##RamSelector", get_ramboard_name(current_advanced_ram_idx))) {
        for (int i = 0; i < 7; ++i) {
            // Filter out Z3/32-bit boards if in 24-bit mode
            if (!z3_enabled) {
                if (i == 3 || i == 4 || i == 5 || i == 6) continue;
            }
            
            bool is_selected = (current_advanced_ram_idx == i);
            if (ImGui::Selectable(get_ramboard_name(i), is_selected))
                current_advanced_ram_idx = i;
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());

    if (rb) {
        const int *current_msi = nullptr;
        int msi_count = 0;

        // Determine MSI array and count based on section
        switch (current_advanced_ram_idx) {
        case 0: // Chip
            current_msi = msi_chip;
            msi_count = 7;
            break;
        case 1: // Slow
            current_msi = msi_bogo;
            msi_count = 5;
            break;
        case 2: // Z2 Fast
            current_msi = msi_fast;
            msi_count = 9;
            break;
        case 3: // Z3 Fast
            current_msi = msi_z3fast;
            msi_count = can_have_1gb() ? 12 : 11;
            break;
        case 4: // 32-bit Chip
            current_msi = msi_z3chip;
            msi_count = can_have_1gb() ? 10 : 9;
            break;
        case 5: // MB Fast
        case 6: // Slot Fast
            current_msi = msi_mb;
            msi_count = 9; // Assuming 9 from MB logic in WinUAE/render_panel_ram
            break;
        }

        if (current_msi) {
             ImGui::SameLine();
             ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
             
             int current_size_idx = get_mem_index(rb->size, current_msi, msi_count);

             if (ImGui::SliderInt("##AdvSlider", &current_size_idx, 0, msi_count - 1, "")) {
                 rb->size = memsizes[current_msi[current_size_idx]];
                 // specific logic ref checks
                 if (current_advanced_ram_idx == 0) { // Chip
                     if (rb->size > 0x200000 && changed_prefs.fastmem[0].size > 0)
                         changed_prefs.fastmem[0].size = 0;
                 } else if (current_advanced_ram_idx == 2) { // Z2 Fast
                     if (rb->size > 0 && changed_prefs.chipmem.size > 0x200000)
                         changed_prefs.chipmem.size = 0x200000;
                 }
             }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

             ImGui::SameLine();
             ImGui::SetNextItemWidth(BUTTON_WIDTH);
             char size_buf[32];
             // Display Logic mimicking WinUAE/Amiberry RamControl
             strncpy(size_buf, memsize_names[current_msi[current_size_idx]], sizeof(size_buf));
             ImGui::InputText("##AdvSize", size_buf, sizeof(size_buf),
                              ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        }
    }

    ImGui::Spacing();

    if (rb) {
        if (ImGui::BeginTable("AdvRamDetails", 2)) {
            // Adjust column widths: Left column needs more space for wider controls
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            // Right column (flags) can be smaller
            ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            
            ImGui::TableNextColumn();

            // Left Column: Inputs

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

            // Memory Board Dropdown
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Memory board");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(BUTTON_WIDTH * 3);
            
            bool enable_board_select = (current_advanced_ram_idx == 2 || current_advanced_ram_idx == 3);
            if (!enable_board_select) ImGui::BeginDisabled();
            
            const char *current_board_name = "-";
            // Try to find current board name based on manuf/prod
             for (int i = 0; memoryboards[i].name; i++) {
                if (memoryboards[i].manufacturer == rb->manufacturer && 
                    memoryboards[i].product == rb->product) {
                    current_board_name = memoryboards[i].name;
                    break;
                }
            }

            if (ImGui::BeginCombo("##MemBoardSelect", current_board_name)) {
                // Populate List
                int z_filter = 0;
                if (current_advanced_ram_idx == 2) z_filter = 2; // Z2
                else if (current_advanced_ram_idx == 3) z_filter = 3; // Z3
                
                if (ImGui::Selectable("-", rb->manufacturer == 0 && rb->product == 0)) {
                    rb->manufacturer = 0;
                    rb->product = 0;
                    memset(rb->autoconfig, 0, sizeof(rb->autoconfig));
                    // Re-fetch defaults?
                    struct autoconfig_info *aci = expansion_get_autoconfig_info(&changed_prefs, z_filter == 2 ? ROMTYPE_RAMZ2 : ROMTYPE_RAMZ3, 0); 
                    if (aci) {
                         memcpy(rb->autoconfig, aci->autoconfig_bytes, sizeof(rb->autoconfig));
                    }
                }

                for (int i = 0; memoryboards[i].name; i++) {
                    if (memoryboards[i].z == z_filter) {
                        bool is_selected = (memoryboards[i].manufacturer == rb->manufacturer && 
                                          memoryboards[i].product == rb->product);
                        if (ImGui::Selectable(memoryboards[i].name, is_selected)) {
                            rb->manufacturer = memoryboards[i].manufacturer;
                            rb->product = memoryboards[i].product;
                            // Set Autoconfig from board definition
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
            ImGui::EndDisabled();

            ImGui::TableNextColumn();

            // Right Column: Flags
            // Order: Edit Autoconfig, Manual, DMA, 16-bit, Slow

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
}
