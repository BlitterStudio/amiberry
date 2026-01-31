#include <vector>
#include <string>
#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"

#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "autoconf.h"
#include "rommgr.h"

// Categories for Expansion Boards (Matches WinUAE scsiromselectedmask)
static const char* ExpansionCategories[] = {
    "Built-in / Internal",
    "SCSI Controllers (AutoConfig)",
    "IDE Controllers",
    "SASI Controllers",
    "Custom / Other",
    "PCI Bridge Boards",
    "x86 Bridge Boards",
    "RTG Graphics Cards",
    "Sound Cards",
    "Network Cards",
    "Floppy Controllers",
    "x86 Expansion Boards"
};
static const int ExpansionCategoriesMask[] = {
    EXPANSIONTYPE_INTERNAL,
    EXPANSIONTYPE_SCSI,
    EXPANSIONTYPE_IDE,
    EXPANSIONTYPE_SASI,
    EXPANSIONTYPE_CUSTOM,
    EXPANSIONTYPE_PCI_BRIDGE,
    EXPANSIONTYPE_X86_BRIDGE,
    EXPANSIONTYPE_RTG,
    EXPANSIONTYPE_SOUND,
    EXPANSIONTYPE_NET,
    EXPANSIONTYPE_FLOPPY,
    EXPANSIONTYPE_X86_EXPANSION
};


static std::vector<int> displayed_rom_indices;

static void RefreshExpansionList() {
    displayed_rom_indices.clear();
    int first_match = -1;
    bool matched = false;

    for (int i = 0; expansionroms[i].name; i++) {
        if (expansionroms[i].romtype & ROMTYPE_CPUBOARD) continue;
        
        int mask = ExpansionCategoriesMask[scsiromselectedcatnum];
        if (!(expansionroms[i].deviceflags & mask)) continue;
        
        if (scsiromselectedcatnum == 0 && (expansionroms[i].deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM))) continue;
        if ((expansionroms[i].deviceflags & EXPANSIONTYPE_X86_EXPANSION) && mask != EXPANSIONTYPE_X86_EXPANSION)
            continue;

        // Check for duplicates/enabled instances
        int cnt = 0;
        for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
            if (is_board_enabled(&changed_prefs, expansionroms[i].romtype, j)) {
                cnt++;
            }
        }
        
        if (i == scsiromselected) matched = true;
        if (cnt > 0 && first_match < 0) first_match = i;

        displayed_rom_indices.push_back(i);
    }
    
    // Auto-select if current choice is invalid
    bool current_valid = false;
    for(int idx : displayed_rom_indices) {
        if(idx == scsiromselected) { current_valid = true; break; }
    }
    if(!current_valid) {
        if (first_match >= 0) scsiromselected = first_match;
        else if (!displayed_rom_indices.empty()) scsiromselected = displayed_rom_indices[0];
        else scsiromselected = 0;
    }
}


static void InitializeExpansionSelection();


struct ROMOption {
    std::string name;
    std::string path;
    bool is_disabled_opt;
};

static std::vector<ROMOption> GetAvailableROMs(int romtype, int romtype_extra)
{
    std::vector<ROMOption> options;
    
    // WinUAE adds "ROM Disabled" effectively by allowing NULL path or specific selection
    options.push_back({ "ROM Disabled", "", true });

    struct romlist *rl = romlist_getit();
    int count = romlist_count();
    
    for (int i = 0; i < count; i++) {
        struct romdata *rd = rl[i].rd;
        if (!rd) continue;
        
        bool match = false;
        if ((rd->type & ROMTYPE_MASK) == (romtype & ROMTYPE_MASK)) match = true;
        if (romtype_extra && (rd->type & ROMTYPE_MASK) == (romtype_extra & ROMTYPE_MASK)) match = true;
        
        if (match) {
            options.push_back({ rd->name, rl[i].path, false });
        }
    }
    return options;
}

void render_panel_expansions()
{
    // Initialize if needed
    if (displayed_rom_indices.empty()) {
        InitializeExpansionSelection();
    }
    
    // Enable/Disable based on emulation state (enable_for_expansion2dlg logic)
    bool gui_enabled = !emulating; 

    BeginGroupBox("Expansion Board Settings");
    
    // Category Selector
    ImGui::PushItemWidth(-1);
    if (ImGui::BeginCombo("##Category", ExpansionCategories[scsiromselectedcatnum])) {
        for (int i = 0; i < IM_ARRAYSIZE(ExpansionCategories); i++) {
            const bool is_selected = (scsiromselectedcatnum == i);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(ExpansionCategories[i], is_selected)) {
                scsiromselectedcatnum = i;
                scsiromselected = 0; // Reset board selection
                RefreshExpansionList();
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    
    // Board Selection Logic
    int current_combo_idx = -1;
    for(size_t i=0; i<displayed_rom_indices.size(); ++i) {
        if (displayed_rom_indices[i] == scsiromselected) {
             current_combo_idx = i;
             break;
        }
    }
    const char* preview_val = (current_combo_idx >= 0) ? expansionroms[displayed_rom_indices[current_combo_idx]].friendlyname : "Select Board...";

    float avail_w = ImGui::GetContentRegionAvail().x;
    float enable_w = 120.0f;
    float unit_w = 80.0f;
    float board_w = avail_w - enable_w - unit_w - 20.0f;
    
    ImGui::PushItemWidth(board_w);
    if (ImGui::BeginCombo("##ExpansionBoard", preview_val)) {
        for (size_t i = 0; i < displayed_rom_indices.size(); i++) {
            int global_idx = displayed_rom_indices[i];
            const bool is_selected = (current_combo_idx == i);
            
            // Format name with count if multiple
            int cnt = 0;
             for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
                if (is_board_enabled(&changed_prefs, expansionroms[global_idx].romtype, j)) cnt++;
            }
            std::string name_label = expansionroms[global_idx].friendlyname;
            if (cnt > 0) name_label = (cnt > 1 ? "[" + std::to_string(cnt) + "] " : "* ") + name_label;

            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(name_label.c_str(), is_selected)) {
                 scsiromselected = global_idx;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    
    const expansionromtype* ert = (scsiromselected >= 0) ? &expansionroms[scsiromselected] : nullptr;
    // Verify ert is valid (expansionroms is null-terminated)
    if (ert && !ert->name) ert = nullptr;

    int index = 0;
    boardromconfig* brc = ert ? get_device_rom(&changed_prefs, ert->romtype, scsiromselectednum, &index) : nullptr;
    bool enabled = (brc != nullptr);

    // Flag Aggregation (Board + Subtype)
    int deviceflags = ert ? ert->deviceflags : 0;
    if (ert && ert->subtypes) {
        int subtype_idx = brc ? brc->roms[index].subtype : 0;
        deviceflags |= ert->subtypes[subtype_idx].deviceflags;
    }

    // Unit Selector (WinUAE logic: values_to_expansion2dlg_sub)
    // Enabled only if Zorro >= 2 (and not singleonly) OR Clockport
    bool unit_selector_enabled = false;
    if (ert) {
        if ((ert->zorro >= 2 && !ert->singleonly) || (deviceflags & EXPANSIONTYPE_CLOCKPORT)) {
            unit_selector_enabled = true;
        } else {
             // Force Unit 0 if selector is disabled (WinUAE behavior)
             scsiromselectednum = 0;
        }
    }
    
    ImGui::PushItemWidth(unit_w);
    ImGui::BeginDisabled(!unit_selector_enabled);
    if (ImGui::BeginCombo("##Unit", std::to_string(scsiromselectednum + 1).c_str())) {
        int max_units = MAX_AVAILABLE_DUPLICATE_EXPANSION_BOARDS; 
        
        // WinUAE adds "-" entry for Clockport, though effectively it maps to unit 0 often or is just visual
        if (deviceflags & EXPANSIONTYPE_CLOCKPORT) {
             const bool is_selected = (scsiromselectednum == 0);
             if (is_selected)
                 ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
             if (ImGui::Selectable("-", is_selected)) {
                 scsiromselectednum = 0;
             }
             if (is_selected) {
                 ImGui::PopStyleColor();
                 ImGui::SetItemDefaultFocus();
             }
        }
        
        for (int i = 0; i < max_units; i++) {
             const bool is_selected = (scsiromselectednum == i);
             if (is_selected)
                 ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
             if (ImGui::Selectable(std::to_string(i + 1).c_str(), is_selected)) {
                 scsiromselectednum = i;
             }
             if (is_selected) {
                 ImGui::PopStyleColor();
                 ImGui::SetItemDefaultFocus();
             }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    ImGui::PopItemWidth();

    ImGui::SameLine();
    
    // Enable Checkbox
    if (ert && AmigaCheckbox("Enable Board", &enabled)) {
        if (enabled) {
            if (!brc) {
                brc = get_device_rom_new(&changed_prefs, ert->romtype, scsiromselectednum, &index);
            }
            // WinUAE logic: If ROMTYPE_NOT, set romfile to ":ENABLED" to mark it active without a file
            if (ert->romtype & ROMTYPE_NOT) {
                if (brc) strncpy(brc->roms[index].romfile, ":ENABLED", MAX_DPATH);
            }
        } else {
            clear_device_rom(&changed_prefs, ert->romtype, scsiromselectednum, true);
            brc = nullptr;
        }
    }
    
    // Detailed Settings
    if (ert && scsiromselected > 0) {
         ImGui::BeginDisabled(!enabled || !brc);
         ImGui::Spacing();
         
         // Subtype Selector
        if (ert->subtypes) {
             const expansionsubromtype* current_subtype = &ert->subtypes[brc ? brc->roms[index].subtype : 0]; 
             ImGui::Text("Subtype:");
             ImGui::SameLine();
             ImGui::PushItemWidth(-1);
             if (ImGui::BeginCombo("##Subtype", current_subtype->name)) {
                int st_idx = 0;
                const expansionsubromtype* st = ert->subtypes;
                while (st && st->name) {
                    const bool is_selected = (brc && brc->roms[index].subtype == st_idx);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(st->name, is_selected)) {
                         if (brc) brc->roms[index].subtype = st_idx;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                    st++;
                    st_idx++;
                }
                ImGui::EndCombo();
             }
             ImGui::PopItemWidth();
        }
        
        // SCSI ID Jumper (visible if id_jumper is true)
        if (ert->id_jumper) {
             int current_id = brc ? brc->roms[index].device_id : 0;
             ImGui::Text("SCSI ID:");
             ImGui::SameLine();
             ImGui::PushItemWidth(80);
             if (ImGui::BeginCombo("##ScsiId", std::to_string(current_id).c_str())) {
                  for (int i=0; i<8; ++i) {
                       const bool is_selected = (current_id == i);
                       if (is_selected)
                           ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                       if (ImGui::Selectable(std::to_string(i).c_str(), is_selected)) {
                            if (brc) brc->roms[index].device_id = i;
                       }
                       if (is_selected) {
                           ImGui::PopStyleColor();
                           ImGui::SetItemDefaultFocus();
                       }
                  }
                  ImGui::EndCombo();
             }
             ImGui::PopItemWidth();
        }


        bool show_rom_file = !(ert->romtype & ROMTYPE_NOT);
        if (show_rom_file) {
            char rom_path[MAX_DPATH] = "";
            if (brc && brc->roms[index].romfile[0]) {
                strncpy(rom_path, brc->roms[index].romfile, MAX_DPATH);
            }
            
            // Check for available ROMs for this type
            std::vector<ROMOption> rom_options = GetAvailableROMs(ert->romtype, ert->romtype_extra);
            
            ImGui::Text("ROM Image:");
            ImGui::SameLine();
            ImGui::PushItemWidth(-40);
            
            if (rom_options.size() > 1) { // More than just "ROM Disabled"
                std::string current_preview = "Select ROM...";
                // Logic to set preview based on current path match
                for (const auto& opt : rom_options) {
                    if (opt.is_disabled_opt) {
                       if (strcmp(rom_path, ":ENABLED") == 0 || rom_path[0] == 0) { // Or some other disabled marker? WinUAE uses empty for disabled usually?
                           if (rom_path[0] == 0) current_preview = opt.name; 
                       }
                    } else if (strcmp(opt.path.c_str(), rom_path) == 0) {
                        current_preview = opt.name;
                    }
                }
                // Fallback if path is custom/unknown
                if (current_preview == "Select ROM..." && rom_path[0] != 0) {
                     current_preview = rom_path; // Show path if custom
                }

                if (ImGui::BeginCombo("##ROMSelector", current_preview.c_str())) {
                    for (const auto& opt : rom_options) {
                        bool is_selected = false;
                        if (opt.is_disabled_opt) {
                             is_selected = (rom_path[0] == 0);
                        } else {
                             is_selected = (strcmp(opt.path.c_str(), rom_path) == 0);
                        }
                        
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                        if (ImGui::Selectable(opt.name.c_str(), is_selected)) {
                            if (brc) {
                                if (opt.is_disabled_opt) {
                                    brc->roms[index].romfile[0] = 0;
                                } else {
                                    strncpy(brc->roms[index].romfile, opt.path.c_str(), MAX_DPATH);
                                }
                            }
                        }
                        if (is_selected) {
                            ImGui::PopStyleColor();
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            } else {
                // Fallback to text input
                ImGui::InputText("##ROMPath", rom_path, MAX_DPATH, ImGuiInputTextFlags_ReadOnly);
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (AmigaButton("...")) {
                 OpenFileDialog("Select ROM Image", "ROM Files (*.rom,*.bin){.rom,.bin}", rom_path);
            }
            
            std::string result_path;
            if (ConsumeFileDialogResult(result_path)) {
                if (brc && !result_path.empty()) {
                    strncpy(brc->roms[index].romfile, result_path.c_str(), MAX_DPATH);
                }
            }
        }
        
         // Flags
        bool autoboot_disabled = brc ? brc->roms[index].autoboot_disabled : false;
        if (ert->autoboot_jumper) {
             if (AmigaCheckbox("Disable Autoboot", &autoboot_disabled)) {
                 if (brc) brc->roms[index].autoboot_disabled = autoboot_disabled;
             }
        }

        if (deviceflags & EXPANSIONTYPE_PCMCIA) {
            bool inserted = brc ? brc->roms[index].inserted : false;
            // WinUAE uses "PCMCIA inserted"
            if (AmigaCheckbox("PCMCIA inserted", &inserted)) {
                if (brc) brc->roms[index].inserted = inserted;
            }
        }

        if (deviceflags & EXPANSIONTYPE_DMA24) {
            bool dma24 = brc ? brc->roms[index].dma24bit : false;
            if (AmigaCheckbox("24-bit DMA", &dma24)) {
                if (brc) brc->roms[index].dma24bit = dma24;
            }
        }
        
         // Custom Settings
        if (enabled && brc && ert->settings) {
            ImGui::Separator();
            ImGui::Text("Board Settings:");
            const expansionboardsettings* ebs = ert->settings;
            int settings_val = brc->roms[index].device_settings;
            
            int item_idx = 0;
            while (ebs[item_idx].name) {
                const expansionboardsettings* s = &ebs[item_idx];
                int bitcnt = 0;
                int current_bit_shift = 0;
                 for(int k=0; k<item_idx; ++k) {
                     const expansionboardsettings* prev = &ebs[k];
                     if (prev->type == EXPANSIONBOARD_MULTI) {
                         int items = 0;
                         const char* pp = (const char*)prev->configname;
                         while (*pp) { items++; pp += strlen(pp) + 1; }
                         int bits = 1;
                         for (int b=0; b<8; b++) { if ((1<<b) >= items) { bits=b; break; } }
                         current_bit_shift += bits;
                     } else if (prev->type == EXPANSIONBOARD_CHECKBOX) {
                         current_bit_shift++;
                     }
                     current_bit_shift += prev->bitshift;
                 }
                 current_bit_shift += s->bitshift;

                if (s->type == EXPANSIONBOARD_CHECKBOX) {
                    bool checked = (settings_val & (1 << current_bit_shift)) != 0;
                    if (s->invert) checked = !checked;
                    if (AmigaCheckbox(s->name, &checked)) {
                         if (s->invert) checked = !checked;
                         if (checked) settings_val |= (1 << current_bit_shift);
                         else settings_val &= ~(1 << current_bit_shift);
                         brc->roms[index].device_settings = settings_val;
                    }
                } else if (s->type == EXPANSIONBOARD_MULTI) {
                     // s->name format: "Label\0Option1\0Option2\0\0"
                     // s->configname format: "id\0id1\0id2\0\0"
                     
                     std::vector<std::string> options;
                     std::string label = "Settings";
                     
                     if (s->name) {
                         const char* pp = (const char*)s->name;
                         if (*pp) {
                             label = pp;
                             pp += strlen(pp) + 1;
                             while (*pp) {
                                 options.push_back(pp);
                                 pp += strlen(pp) + 1;
                             }
                         }
                     }
                     
                     // If no options found in name, fall back to configname (legacy/safety)
                     if (options.empty() && s->configname) {
                         const char* pp = (const char*)s->configname;
                         // If configname matches pattern (ID\0Val1\0Val2)
                         if (*pp) {
                             // Use first item as Label/ID if name failed? 
                             // Or just assume it's a list. 
                             // Current buggy implementation treated it as list only.
                             // But for multi, we need options.
                             // Let's assume options are in name. If not, maybe it's malformed.
                             // Fallback: use configname as options list (WinUAE compatibility check?)
                             while (*pp) { options.push_back(pp); pp += strlen(pp) + 1; }
                             
                             // If we fell back to configname, the first item is likely the ID/Label, 
                             // so maybe we should remove it from options or use it as label?
                             // But let's trust s->name is populated correctly for Multi.
                             
                             // Actually, for the Keyboard example: 
                             // name: "Options\0-\0RAM fault\0..."
                             // configname: "kbmcuopt\0-\0ramf\0..."
                             // So if we used configname, we got "kbmcuopt", "-", "ramf".
                             // We want "-", "RAM fault" from name.
                         }
                     }

                     int items_count = (int)options.size();
                     int bits = 1;
                     for (int b=0; b<8; b++) { if ((1<<b) >= items_count) { bits=b; break; } }
                     int mask = (1 << bits) - 1;
                     int val = (settings_val >> current_bit_shift) & mask;
                     if (s->invert) val ^= (1 << bits) - 1; // Invert read

                     // Ensure val is within bounds
                     if (val >= items_count) val = 0; // Default to 0 if out of range

                     if (ImGui::BeginCombo(label.c_str(), (items_count > val) ? options[val].c_str() : "Unknown")) { 
                         for (int opt_i = 0; opt_i < items_count; opt_i++) {
                             const bool is_selected = (opt_i == val);
                             if (is_selected)
                                 ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                             if (ImGui::Selectable(options[opt_i].c_str(), is_selected)) {
                                 val = opt_i;
                                 if (s->invert) val ^= (1 << bits) - 1; // Invert write
                                 
                                 settings_val &= ~(mask << current_bit_shift);
                                 settings_val |= (val << current_bit_shift);
                                 brc->roms[index].device_settings = settings_val;
                             }
                             if (is_selected) {
                                 ImGui::PopStyleColor();
                                 ImGui::SetItemDefaultFocus();
                             }
                         }
                         ImGui::EndCombo();
                     }
                } else if (s->type == EXPANSIONBOARD_STRING) {
                     AmigaInputText(s->name, brc->roms[index].configtext, sizeof(brc->roms[index].configtext));
                }
                item_idx++;
            }
        }
        ImGui::EndDisabled();
    }
    EndGroupBox("Expansion Board Settings");

    // Accelerator Settings
    BeginGroupBox("Accelerator Board Settings");
    const int type = changed_prefs.cpuboard_type;
    const cpuboardsubtype* st = &cpuboards[type].subtypes[changed_prefs.cpuboard_subtype];

    if (ImGui::BeginTable("AccelTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Left Column: Type & Subtype
        ImGui::Text("Accelerator Board:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##AccelType", cpuboards[changed_prefs.cpuboard_type].name)) {
        for (int i = 0; cpuboards[i].name; i++) {
            const bool is_selected = (changed_prefs.cpuboard_type == i);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(cpuboards[i].name, is_selected)) {
                changed_prefs.cpuboard_type = i;
                changed_prefs.cpuboard_subtype = cpuboards[i].defaultsubtype;
                changed_prefs.cpuboard_settings = 0;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (cpuboards[type].subtypes) {
         ImGui::Text("Subtype:");
         ImGui::SetNextItemWidth(-1);
         const cpuboardsubtype* subtypes = cpuboards[type].subtypes;
         if (ImGui::BeginCombo("##AccelSub", subtypes[changed_prefs.cpuboard_subtype].name)) {
             int i = 0;
             while (subtypes[i].name) {
                 const bool is_selected = (changed_prefs.cpuboard_subtype == i);
                 if (is_selected)
                     ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                 if (ImGui::Selectable(subtypes[i].name, is_selected)) {
                     changed_prefs.cpuboard_subtype = i;
                 }
                 if (is_selected) {
                     ImGui::PopStyleColor();
                     ImGui::SetItemDefaultFocus();
                 }
                 i++;
             }
             ImGui::EndCombo();
         }
    }
    
        ImGui::TableNextColumn();

        // Right Column: ROM & Memory
        if (changed_prefs.cpuboard_type != 0) {
         int idx = 0;
         boardromconfig* brc = get_device_rom(&changed_prefs, ROMTYPE_CPUBOARD, 0, &idx);
         
         ImGui::Text("Boot ROM:");
         char rom_path[MAX_DPATH] = "";
         if (brc) strncpy(rom_path, brc->roms[idx].romfile, MAX_DPATH);
         
         ImGui::PushItemWidth(-40);
         ImGui::BeginDisabled(!gui_enabled); // Only change ROM if not running/safe
         ImGui::InputText("##AccelROM", rom_path, MAX_DPATH, ImGuiInputTextFlags_ReadOnly);
         ImGui::PopItemWidth();
         ImGui::SameLine();
         if (AmigaButton("...##Accel")) {
              OpenFileDialog("Select Boot ROM", "ROM Files (*.rom,*.bin){.rom,.bin}", rom_path);
         }
         ImGui::EndDisabled();

         std::string result_path;
         if (ConsumeFileDialogResult(result_path)) {
            if (brc && !result_path.empty()) {
                strncpy(brc->roms[idx].romfile, result_path.c_str(), MAX_DPATH);
            }
         }
    } else {
        ImGui::Text("Boot ROM: Not Required");
    }
    
    // Memory Slider
    static const int mem_sizes[] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 256 };
    static const char* mem_labels[] = { "0 MB", "1 MB", "2 MB", "4 MB", "8 MB", "16 MB", "32 MB", "64 MB", "128 MB", "256 MB" };
    int current_size = changed_prefs.cpuboardmem1.size >> 20;
    int current_idx = 0;
    for (int i=0; i < IM_ARRAYSIZE(mem_sizes); ++i) {
        if (mem_sizes[i] == current_size) { current_idx = i; break; }
    }

    int max_mb = st->maxmemory >> 20;
    int max_idx = 0;
     for (int i=0; i < IM_ARRAYSIZE(mem_sizes); ++i) {
        if (mem_sizes[i] <= max_mb) max_idx = i;
    }
    
    if (max_idx > 0) {
        ImGui::Text("Board Memory:");
        ImGui::SetNextItemWidth(-1);
        ImGui::BeginDisabled(!gui_enabled);
        if (ImGui::SliderInt("##BoardMem", &current_idx, 0, max_idx, mem_labels[current_idx])) {
            changed_prefs.cpuboardmem1.size = mem_sizes[current_idx] << 20;
        }
        ImGui::EndDisabled();
    } else {
        ImGui::Text("Board Memory: None / Fixed");
    }

        ImGui::EndTable();
    }

    // Accelerator Settings (Jumpers) - Full width below columns
     if (st->settings) {
          ImGui::Separator();
          ImGui::Text("Jumper Settings:");
          const expansionboardsettings* ebs = st->settings;
          int settings_val = changed_prefs.cpuboard_settings;
          int item_idx = 0;
            while (ebs[item_idx].name) {
                const expansionboardsettings* s = &ebs[item_idx];
                int current_bit_shift = 0;
                 for(int k=0; k<item_idx; ++k) {
                     const expansionboardsettings* prev = &ebs[k];
                     if (prev->type == EXPANSIONBOARD_MULTI) {
                         int items = 0;
                         const char* pp = (const char*)prev->configname;
                         while (*pp) { items++; pp += strlen(pp) + 1; }
                         int bits = 1;
                         for (int b=0; b<8; b++) { if ((1<<b) >= items) { bits=b; break; } }
                         current_bit_shift += bits;
                     } else if (prev->type == EXPANSIONBOARD_CHECKBOX) {
                         current_bit_shift++;
                     }
                     current_bit_shift += prev->bitshift;
                 }
                 current_bit_shift += s->bitshift;

                if (s->type == EXPANSIONBOARD_CHECKBOX) {
                    bool checked = (settings_val & (1 << current_bit_shift)) != 0;
                    if (s->invert) checked = !checked;
                    if (AmigaCheckbox(s->name, &checked)) {
                         if (s->invert) checked = !checked;
                         if (checked) settings_val |= (1 << current_bit_shift);
                         else settings_val &= ~(1 << current_bit_shift);
                         changed_prefs.cpuboard_settings = settings_val;
                    }
                } else if (s->type == EXPANSIONBOARD_MULTI) {
                     int items = 0;
                     const char* pp = (const char*)s->configname; 
                     std::vector<std::string> options;
                     while (*pp) { options.push_back(pp); pp += strlen(pp) + 1; }
                     int bits = 1;
                     for (int b=0; b<8; b++) { if ((1<<b) >= (int)options.size()) { bits=b; break; } }
                     int mask = (1 << bits) - 1;
                     int val = (settings_val >> current_bit_shift) & mask;
                     
                     if (ImGui::BeginCombo(s->configname ? s->configname : "Settings", options.size() > val ? options[val].c_str() : "Unknown")) { 
                         for (size_t opt_i = 0; opt_i < options.size(); opt_i++) {
                             if (ImGui::Selectable(options[opt_i].c_str(), (int)opt_i == val)) {
                                 val = (int)opt_i;
                                 settings_val &= ~(mask << current_bit_shift);
                                 settings_val |= (val << current_bit_shift);
                                 changed_prefs.cpuboard_settings = settings_val;
                             }
                         }
                         ImGui::EndCombo();
                     }
                }
                item_idx++;
            }
     }
    EndGroupBox("Accelerator Board Settings");

	BeginGroupBox("Miscellaneous Expansions");
    if (ImGui::BeginTable("MiscTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::BeginDisabled(!gui_enabled); // WinUAE disables misc expansions when running? logic says "ew(hDlg, IDC_SOCKETS, workprefs.socket_emu)" - check enable_for_expansion2dlg

        AmigaCheckbox("bsdsocket.library", &changed_prefs.socket_emu);

        bool scsi_enabled = changed_prefs.scsi != 0;
        if (AmigaCheckbox("uaescsi.device", &scsi_enabled)) {
            changed_prefs.scsi = scsi_enabled ? 1 : 0;
        }

        ImGui::TableNextColumn();
        // CD32 FMV Card removed for WinUAE parity
        AmigaCheckbox("uaenet.device", &changed_prefs.sana2);

        ImGui::EndDisabled();
        ImGui::EndTable();
    }
    EndGroupBox("Miscellaneous Expansions");
}

static void InitializeExpansionSelection()
{
    // Logic from WinUAE init_expansion2: Find the first enabled board to select on open
    scsiromselected = 0;
    scsiromselectedcatnum = 0;
    scsiromselectednum = 0;
    
    // Check all categories
    for (int cat = 0; cat < IM_ARRAYSIZE(ExpansionCategoriesMask); cat++) {
        int mask = ExpansionCategoriesMask[cat];
        for (int i = 0; expansionroms[i].name; i++) {
             if (expansionroms[i].romtype & ROMTYPE_CPUBOARD) continue;
             if (!(expansionroms[i].deviceflags & mask)) continue;
             if (cat == 0 && (expansionroms[i].deviceflags & (EXPANSIONTYPE_SASI | EXPANSIONTYPE_CUSTOM))) continue;
             if ((expansionroms[i].deviceflags & EXPANSIONTYPE_X86_EXPANSION) && mask != EXPANSIONTYPE_X86_EXPANSION) continue;
             
             // Is this board enabled?
             if (is_board_enabled(&changed_prefs, expansionroms[i].romtype, 0)) {
                 scsiromselectedcatnum = cat;
                 scsiromselected = i;
                 goto found_active;
             }
        }
    }
found_active:
    RefreshExpansionList(); 
}


