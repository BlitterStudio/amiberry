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
        ShowHelpMarker("CPU scheduling priority when emulator window is active and mouse is captured");

        ImGui::Spacing();
        ImGui::Text("Mouse uncaptured:");
        AmigaCheckbox("Pause emulation", &changed_prefs.active_nocapture_pause);
        ShowHelpMarker("Pause emulation when mouse is not captured (e.g., Alt+Tab)");
        AmigaCheckbox("Disable sound", &changed_prefs.active_nocapture_nosound);
        ShowHelpMarker("Stop sound processing when mouse is not captured to reduce CPU usage");
        EndGroupBox("When Active");

        // --- When Inactive ---
        ImGui::TableNextColumn();
        BeginGroupBox("When Inactive");

        ImGui::Text("Run at priority:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Combo("##InactivePrio", &changed_prefs.inactive_priority, prio_items, IM_ARRAYSIZE(prio_items));
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("CPU scheduling priority when emulator window loses focus");

        ImGui::Spacing();
        AmigaCheckbox("Pause emulation", &changed_prefs.inactive_pause);
        ShowHelpMarker("Pause emulation when window is inactive");
        AmigaCheckbox("Disable sound", &changed_prefs.inactive_nosound);
        ShowHelpMarker("Stop sound processing when window is inactive to reduce CPU usage");
        CheckboxInt04("Disable input", &changed_prefs.inactive_input);
        ShowHelpMarker("Stop processing input devices when window is inactive");
        EndGroupBox("When Inactive");

        // --- When Minimized ---
        ImGui::TableNextColumn();
        BeginGroupBox("When Minimized");

        ImGui::Text("Run at priority:");
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Combo("##MinimizedPrio", &changed_prefs.minimized_priority, prio_items, IM_ARRAYSIZE(prio_items));
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("CPU scheduling priority when emulator window is minimized");

        ImGui::Spacing();
        AmigaCheckbox("Pause emulation", &changed_prefs.minimized_pause);
        ShowHelpMarker("Pause emulation when window is minimized");
        AmigaCheckbox("Disable sound", &changed_prefs.minimized_nosound);
        ShowHelpMarker("Stop sound processing when window is minimized to reduce CPU usage");
        CheckboxInt04("Disable input", &changed_prefs.minimized_input);
        ShowHelpMarker("Stop processing input devices when window is minimized");
        EndGroupBox("When Minimized");

        ImGui::EndTable();
    }
}
