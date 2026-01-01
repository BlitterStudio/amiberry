#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "gui/gui_handling.h"

static bool CheckboxInt04(const char* label, int* value)
{
    // Guisan logic: isSelected() ? 0 : 4
    // If value is 0, checkbox should be checked.
    // If value is 4, checkbox should be unchecked.
    bool v = (*value == 0);
    bool pressed = ImGui::Checkbox(label, &v);
    if (pressed)
    {
        *value = v ? 0 : 4;
    }
    return pressed;
}

// Helper to center text in the current content region (e.g., Table Column)
static void TextCentered(const char* text) {
    float win_width = ImGui::GetContentRegionAvail().x;
    float text_width = ImGui::CalcTextSize(text).x;
    float start_x = ImGui::GetCursorPosX() + (win_width - text_width) * 0.5f;
    if (start_x > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(start_x);
    ImGui::Text("%s", text);
}

void render_panel_prio()
{
    const char* prio_items[] = { "Low", "Normal", "High" };
    const float group_height = 220.0f;

    if (ImGui::BeginTable("PrioPanelTable", 3, ImGuiTableFlags_SizingStretchProp))
    {
        // --- When Active ---
        ImGui::TableNextColumn();
        TextCentered("When Active");
        if (ImGui::BeginChild("##ActiveGroup", ImVec2(0, group_height), true))
        {
            ImGui::Text("Run at priority:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::Combo("##ActivePrio", &changed_prefs.active_capture_priority, prio_items, IM_ARRAYSIZE(prio_items));
            
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Text("Mouse uncaptured:");
            ImGui::Checkbox("Pause emulation", &changed_prefs.active_nocapture_pause);
            ImGui::Checkbox("Disable sound", &changed_prefs.active_nocapture_nosound);
        }
        ImGui::EndChild();

        // --- When Inactive ---
        ImGui::TableNextColumn();
        TextCentered("When Inactive");
        if (ImGui::BeginChild("##InactiveGroup", ImVec2(0, group_height), true))
        {
            ImGui::Text("Run at priority:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::Combo("##InactivePrio", &changed_prefs.inactive_priority, prio_items, IM_ARRAYSIZE(prio_items));
            
            ImGui::Dummy(ImVec2(0.0f, 18.0f)); // Spacing to align with Active group's "Mouse uncaptured" gap if needed, or just standard list
            ImGui::Checkbox("Pause emulation", &changed_prefs.inactive_pause);
            ImGui::Checkbox("Disable sound", &changed_prefs.inactive_nosound);
            CheckboxInt04("Disable input", &changed_prefs.inactive_input);
        }
        ImGui::EndChild();

        // --- When Minimized ---
        ImGui::TableNextColumn();
        TextCentered("When Minimized");
        if (ImGui::BeginChild("##MinimizedGroup", ImVec2(0, group_height), true))
        {
            ImGui::Text("Run at priority:");
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::Combo("##MinimizedPrio", &changed_prefs.minimized_priority, prio_items, IM_ARRAYSIZE(prio_items));
            
            ImGui::Dummy(ImVec2(0.0f, 18.0f));
            ImGui::Checkbox("Pause emulation", &changed_prefs.minimized_pause);
            ImGui::Checkbox("Disable sound", &changed_prefs.minimized_nosound);
            CheckboxInt04("Disable input", &changed_prefs.minimized_input);
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }
}
