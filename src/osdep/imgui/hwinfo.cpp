#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "autoconf.h"
#include <string>
#include <vector>

// Define MAX_INFOS if not present, or just use a safe loop limit
#ifndef MAX_INFOS
#define MAX_INFOS 100
#endif

void render_panel_hwinfo()
{
    // Ensure the card list is populated based on current prefs
    expansion_scan_autoconfig(&changed_prefs, false);

    static int selected_row = -1;

    ImGui::Separator();

    if (ImGui::BeginTable("HWInfoTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, -50))) // Reserve space for bottom controls
    {
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Start");
        ImGui::TableSetupColumn("End");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("ID");
        ImGui::TableHeadersRow();

        for (int i = 0; i < MAX_INFOS; ++i)
        {
            struct autoconfig_info* aci = expansion_get_autoconfig_data(&changed_prefs, i);
            if (!aci) break; // Stop if no more cards

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            
            // Selectable Row Logic
            char label[32];
            sprintf(label, "##row%d", i);
            bool is_selected = (selected_row == i);
            
            // Type
            const char* type_str = "-";
            if (aci->zorro == 1) type_str = "Z1";
            else if (aci->zorro == 2) type_str = "Z2";
            else if (aci->zorro == 3) type_str = "Z3";
            
            // Selectable covers the Type column, but we want row selection behavior ideally.
            // Using SpanAllColumns flag on the first item.
            if (ImGui::Selectable(type_str, is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selected_row = i;
            }
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", aci->name);
            
            ImGui::TableNextColumn();
            ImGui::Text("0x%08X", aci->start);
            
            ImGui::TableNextColumn();
            ImGui::Text("0x%08X", aci->start + aci->size - 1);
            
            ImGui::TableNextColumn();
            // Format size
            if (aci->size >= 1024 * 1024)
                ImGui::Text("%d MB", aci->size / (1024 * 1024));
            else if (aci->size >= 1024)
                ImGui::Text("%d KB", aci->size / 1024);
            else
                ImGui::Text("%d B", aci->size);

            ImGui::TableNextColumn();
            // ManID / ProdID
            ImGui::Text("0x%04X / 0x%02X", (aci->autoconfig_bytes[4] << 8) | aci->autoconfig_bytes[5], aci->autoconfig_bytes[1]);
        }
        ImGui::EndTable();
    }

    // Bottom Controls
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
    
    if (ImGui::Checkbox("Custom board order", &changed_prefs.autoconfig_custom_sort)) {
        if (changed_prefs.autoconfig_custom_sort) {
            expansion_set_autoconfig_sort(&changed_prefs);
        }
    }
    
    ImGui::SameLine();
    ImGui::BeginDisabled(!changed_prefs.autoconfig_custom_sort || selected_row < 0);
    if (ImGui::Button("Move up")) {
        int new_idx = expansion_autoconfig_move(&changed_prefs, selected_row, -1, false);
        if (new_idx >= 0) selected_row = new_idx;
    }
    ImGui::SameLine();
    if (ImGui::Button("Move down")) {
        int new_idx = expansion_autoconfig_move(&changed_prefs, selected_row, 1, false);
        if (new_idx >= 0) selected_row = new_idx;
    }
    ImGui::EndDisabled();

}
