#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "imgui_panels.h"
#include "options.h"

static bool CheckboxInt04(const char *label, int *value) {
    // Guisan logic: isSelected() ? 0 : 4
    // If value is 0, checkbox should be checked.
    // If value is 4, checkbox should be unchecked.
    bool v = (*value == 0);
    bool pressed = AmigaCheckbox(label, &v);
    if (pressed) {
        *value = v ? 0 : 4;
    }
    return pressed;
}

void render_panel_prio() {
    ImGui::Indent(4.0f);

    const char *prio_items[] = {"Low", "Normal", "High"};

    if (ImGui::BeginTable("PrioPanelTable", 3, ImGuiTableFlags_SizingStretchProp)) {
        // --- When Active ---
        ImGui::TableNextColumn();
        BeginGroupBox("When Active");
        ImGui::Text("Run at priority:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Combo("##ActivePrio", &changed_prefs.active_capture_priority, prio_items, IM_ARRAYSIZE(prio_items));
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        ImGui::Spacing();
        ImGui::Text("Mouse uncaptured:");
        AmigaCheckbox("Pause emulation", &changed_prefs.active_nocapture_pause);
        AmigaCheckbox("Disable sound", &changed_prefs.active_nocapture_nosound);
        EndGroupBox("When Active");

        // --- When Inactive ---
        ImGui::TableNextColumn();
        BeginGroupBox("When Inactive");

        ImGui::Text("Run at priority:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Combo("##InactivePrio", &changed_prefs.inactive_priority, prio_items, IM_ARRAYSIZE(prio_items));
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        ImGui::Spacing();
        AmigaCheckbox("Pause emulation", &changed_prefs.inactive_pause);
        AmigaCheckbox("Disable sound", &changed_prefs.inactive_nosound);
        CheckboxInt04("Disable input", &changed_prefs.inactive_input);
        EndGroupBox("When Inactive");

        // --- When Minimized ---
        ImGui::TableNextColumn();
        BeginGroupBox("When Minimized");

        ImGui::Text("Run at priority:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Combo("##MinimizedPrio", &changed_prefs.minimized_priority, prio_items, IM_ARRAYSIZE(prio_items));
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        ImGui::Spacing();
        AmigaCheckbox("Pause emulation", &changed_prefs.minimized_pause);
        AmigaCheckbox("Disable sound", &changed_prefs.minimized_nosound);
        CheckboxInt04("Disable input", &changed_prefs.minimized_input);
        EndGroupBox("When Minimized");

        ImGui::EndTable();
    }
}
