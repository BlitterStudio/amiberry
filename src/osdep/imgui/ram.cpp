#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "imgui_panels.h"
#include "memory.h"
#include "uae.h"

// Helper to find the index for a given size in an msi array
static int get_mem_index(uae_u32 size, const int* msi_array, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (size == memsizes[msi_array[i]])
			return i;
	}
	return 0; // Default to 0 if not found
}

// Helper to render label + slider + value text
// Layout: [Label (Fixed Width)] [Slider (Flexible)] [Value Box (Fixed)]
static bool RamControl(const char* label, int* current_index, const int* msi_array, int count)
{
    bool changed = false;
    ImGui::PushID(label);

    // Label on Left
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(110); 

    // Slider Middle
    ImGui::SetNextItemWidth(120); 
    if (ImGui::SliderInt("##slider", current_index, 0, count - 1, "")) {
        changed = true;
    }
    
    // Value on Right
    ImGui::SameLine();
    ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(60); 
    char val_buf[32];
    strncpy(val_buf, memsize_names[msi_array[*current_index]], sizeof(val_buf));
    ImGui::InputText("##val", val_buf, sizeof(val_buf), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
    ImGui::EndDisabled();

    ImGui::PopID();
    return changed;
}

static const int MAX_RAM_SELECT = 10;
static int current_advanced_ram_idx = 0;

static struct ramboard* get_selected_ramboard(int idx)
{
    // Need to handle missing indices gracefully
    switch(idx) {
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

static const char* get_ramboard_name(int idx)
{
    switch(idx) {
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

void render_panel_ram()
{
	ImGui::Indent(5.0f);
	BeginGroupBox("Memory Settings");

    if (ImGui::BeginTable("RamLayout", 2, ImGuiTableFlags_None))
    {
        ImGui::TableNextColumn();
        
        // LEFT COLUMN
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

        ImGui::TableNextColumn();
        
        // RIGHT COLUMN
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
        ImGui::EndTable();
    }
    
    int mblow_idx = get_mem_index(changed_prefs.mbresmem_low.size, msi_mb, 8);
    if (RamControl("Motherboard Fast:", &mblow_idx, msi_mb, 8)) {
     	changed_prefs.mbresmem_low.size = memsizes[msi_mb[mblow_idx]];
     	if (currprefs.mbresmem_low.size != changed_prefs.mbresmem_low.size)
     		disable_resume();
    }

    ImGui::Dummy(ImVec2(0, 5));
    ImGui::Separator();
    ImGui::Text("Configured 32-bit address space: Z3 available: %dM", max_z3fastmem / 1024 / 1024);

	EndGroupBox("Memory Settings");

    BeginGroupBox("Advanced Memory Settings");
    
    struct ramboard* rb = get_selected_ramboard(current_advanced_ram_idx);
    
    // Top Row: Selector ...... Slider [ Size ]
    ImGui::SetNextItemWidth(250);
    if (ImGui::BeginCombo("##RamSelector", get_ramboard_name(current_advanced_ram_idx))) {
        for (int i = 0; i < 7; ++i) {
            bool is_selected = (current_advanced_ram_idx == i);
            if (ImGui::Selectable(get_ramboard_name(i), is_selected))
                current_advanced_ram_idx = i;
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    // Simulate Slider to the right + Size Box
    // In WinUAE, there IS a slider here to control the size of the selected board. 
    // We didn't implement logic to map this slider to `rb->size` yet, but visually we can add it.
    // If user changes it, we can try to find nearest size?
    // For now, let's just make it a fixed visual or disabled slider + text to match screenshot logic requested.
    // Actually, WinUAE *does* let you change size here.
    if (rb) {
        ImGui::SameLine(300); // Fixed offset to push to right side
        ImGui::SetNextItemWidth(120);
        int dummy_slider = 0; // Just visual for now or we map it?
        // Let's leave it as text box only if logic is complex, but user wants visual match.
        // WinUAE: [ Slider ] [ Box ]
        ImGui::BeginDisabled();
        ImGui::SliderInt("##AdvSlider", &dummy_slider, 0, 10, "");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        char size_buf[32];
        if (rb->size >= 1024 * 1024)
            sprintf(size_buf, "%d MB", rb->size / (1024 * 1024));
        else
            sprintf(size_buf, "%d KB", rb->size / 1024);
        ImGui::InputText("##AdvSize", size_buf, sizeof(size_buf), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);
        ImGui::EndDisabled();
    }

    ImGui::Dummy(ImVec2(0, 5));

    if (rb) {
        if (ImGui::BeginTable("AdvRamDetails", 2)) {
             ImGui::TableNextColumn();
             
             // Left Column: Inputs
             
             // Manufacturer & Product on same line
             ImGui::AlignTextToFramePadding();
             ImGui::Text("Manufacturer");
             ImGui::SameLine(130);
             char manuf_buf[8];
             sprintf(manuf_buf, "%04X", rb->manufacturer);
             ImGui::SetNextItemWidth(50); 
             if (ImGui::InputText("##Manuf", manuf_buf, 8, ImGuiInputTextFlags_CharsHexadecimal)) {
                 rb->manufacturer = (uae_u16)strtoul(manuf_buf, nullptr, 16);
             }

             ImGui::SameLine();
             ImGui::Text("Product");
             ImGui::SameLine();
             char prod_buf[8];
             sprintf(prod_buf, "%02X", rb->product);
             ImGui::SetNextItemWidth(35); 
             if (ImGui::InputText("##Product", prod_buf, 8, ImGuiInputTextFlags_CharsHexadecimal)) {
                 rb->product = (uae_u8)strtoul(prod_buf, nullptr, 16);
             }
             
             // Autoconfig Data (Full width label?)
             ImGui::AlignTextToFramePadding();
             ImGui::Text("Autoconfig data");
             ImGui::SameLine(130);
             ImGui::SetNextItemWidth(250); // Wider to match WinUAE visual
             ImGui::InputText("##Autoconfig", (char*)"00.00.00...", 64, ImGuiInputTextFlags_ReadOnly);

             // Memory Board Dropdown (Placeholder)
             ImGui::AlignTextToFramePadding();
             ImGui::Text("Memory board");
             ImGui::SameLine(130);
             ImGui::SetNextItemWidth(250);
             ImGui::BeginDisabled();
             if (ImGui::BeginCombo("##MemBoardPlaceholder", "")) ImGui::EndCombo();
             ImGui::EndDisabled();

             // Address Range 
             // Always show label to keep layout stable? Or only when manual?
             // WinUAE shows it disabled if not Manual.
             bool manual_active = rb->manual_config;
             ImGui::BeginDisabled(!manual_active);
             ImGui::AlignTextToFramePadding();
             ImGui::Text("Address range");
             ImGui::SameLine(130);
             char addr_buf[16];
             sprintf(addr_buf, "%08X", rb->start_address);
             ImGui::SetNextItemWidth(80);
             if (ImGui::InputText("##Address", addr_buf, 16, ImGuiInputTextFlags_CharsHexadecimal)) {
                 rb->start_address = strtoul(addr_buf, nullptr, 16);
             }
             // Second box for end address? WinUAE has two boxes.
             ImGui::SameLine();
             ImGui::SetNextItemWidth(80);
             ImGui::InputText("##AddressEnd", (char*)"00000000", 16, ImGuiInputTextFlags_ReadOnly);
             ImGui::EndDisabled();

             ImGui::TableNextColumn();
             
             // Right Column: Flags
             // Order: Edit Autoconfig, Manual, DMA, 16-bit, Slow
             
             bool edit_ac = false; // logic not implemented
             AmigaCheckbox("Edit Autoconfig data", &edit_ac);
             
             AmigaCheckbox("Manual configuration", &rb->manual_config);
             
             bool dma_capable = !rb->nodma;
             if (AmigaCheckbox("DMA Capable", &dma_capable)) rb->nodma = !dma_capable;
             
             AmigaCheckbox("Force 16-bit", &rb->force16bit);
             AmigaCheckbox("Slow RAM", &rb->chipramtiming);
             
             // Z3 Mapping Mode - Bottom Right
             ImGui::Dummy(ImVec2(0, 10));
             ImGui::Text("Z3 mapping mode:");
             const char* z3_mapping_items[] = { "Automatic (*)", "UAE", "Real" };
             ImGui::SetNextItemWidth(200); 
             ImGui::Combo("##Z3Mapping", &changed_prefs.z3_mapping_mode, z3_mapping_items, IM_ARRAYSIZE(z3_mapping_items));
             
             ImGui::EndTable();
        }
    }
    
	EndGroupBox("Advanced Memory Settings");
	ImGui::Unindent(5.0f);
}
