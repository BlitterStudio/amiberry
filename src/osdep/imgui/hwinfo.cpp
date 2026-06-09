#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "gui/gui_handling.h"
#include "autoconf.h"
#include <string>
#include <vector>

#include "imgui_panels.h"

// Define MAX_INFOS if not present, or just use a safe loop limit
#ifndef MAX_INFOS
#define MAX_INFOS 100
#endif

void render_panel_hwinfo()
{
    ImGui::Indent(4.0f);

    // Populate the card list from current prefs only when this panel is (re)entered or
    // after an in-panel change - NOT every frame. Re-scanning every frame re-ran the
    // expansion board init (re-opening flash ROM files, etc.) and, for a board with a
    // missing ROM (e.g. Picasso IV), re-fired gui_message() every frame, which made the
    // resulting message box impossible to dismiss (it reopened the instant it was closed).
    static int last_render_frame = -100;
    static bool needs_scan = true;
    const int this_frame = ImGui::GetFrameCount();
    if (this_frame != last_render_frame + 1)
        needs_scan = true; // not rendered last frame => panel was just (re)entered
    last_render_frame = this_frame;

    if (needs_scan) {
        expansion_scan_autoconfig(&changed_prefs, false);
        needs_scan = false;
    }

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
            
            ImGui::PushID(i);
            // Selectable Row Logic
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
            ImGui::PopID();
            
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
    ImGui::Spacing();
    
    if (AmigaCheckbox("Custom board order", &changed_prefs.autoconfig_custom_sort)) {
        if (changed_prefs.autoconfig_custom_sort) {
            expansion_set_autoconfig_sort(&changed_prefs);
        }
        needs_scan = true; // refresh the displayed order on the next frame
    }
    
    ImGui::SameLine();
    ImGui::BeginDisabled(!changed_prefs.autoconfig_custom_sort || selected_row < 0);
    if (AmigaButton(ICON_FA_ARROW_UP " Move up", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
        int new_idx = expansion_autoconfig_move(&changed_prefs, selected_row, -1, false);
        if (new_idx >= 0) selected_row = new_idx;
    }
    ImGui::SameLine();
    if (AmigaButton(ICON_FA_ARROW_DOWN " Move down", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
        int new_idx = expansion_autoconfig_move(&changed_prefs, selected_row, 1, false);
        if (new_idx >= 0) selected_row = new_idx;
    }
    ImGui::EndDisabled();
}
