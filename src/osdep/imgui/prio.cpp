#include "imgui.h"
#include "sysdeps.h"
#include "imgui_panels.h"
#include "options.h"
#include "settings/settings_definitions.h"

#include <cstring>

namespace settings = amiberry_settings;

// An option of the settings layer as a checkbox, checked for `on`. The
// layer owns its availability (the inactive options follow the
// active-but-not-captured pause, and are disabled while it is on) and how
// the value is stored ("Disable input" is the input option's "0").
static void LayerCheckbox(const char* label, const char* id, const char* on = "true", const char* off = "false")
{
    const settings::definition& d = settings::require(id);
    const settings::context ctx{emulating != 0};
    char value[8];
    settings::get(d, changed_prefs, value, sizeof value);
    bool checked = std::strcmp(value, on) == 0;
    const bool available = settings::is_available(d, changed_prefs, ctx);
    ImGui::BeginDisabled(!available);
    if (AmigaCheckbox(label, &checked))
        settings::set(d, changed_prefs, ctx, checked ? on : off);
    ImGui::EndDisabled();
}

void render_panel_prio() {
    ImGui::Indent(4.0f);

    const char *prio_items[] = {"Low", "Normal", "High"};

    if (ImGui::BeginTable("PrioPanelTable", 3, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("c1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c2", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("c3", ImGuiTableColumnFlags_WidthStretch, 1.0f);
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
        LayerCheckbox("Pause emulation", "amiberry.active_not_captured_pause");
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
        LayerCheckbox("Pause emulation", "amiberry.inactive_pause");
        ShowHelpMarker("Pause emulation when window is inactive");
        LayerCheckbox("Disable sound", "amiberry.inactive_nosound");
        ShowHelpMarker("Stop sound processing when window is inactive to reduce CPU usage");
        LayerCheckbox("Disable input", "amiberry.inactive_input", "0", "4");
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
        LayerCheckbox("Disable input", "amiberry.minimized_input", "0", "4");
        ShowHelpMarker("Stop processing input devices when window is minimized");
        EndGroupBox("When Minimized");

        ImGui::EndTable();
    }
}
