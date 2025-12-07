#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "inputdevice.h"
#include "gui/gui_handling.h"
#include <vector>

void render_panel_custom()
{
    // Ensure SelectedPort stays within valid range to avoid OOB on jports
    if (SelectedPort < 0) SelectedPort = 0;
    if (SelectedPort > 3) SelectedPort = 3;

    // Persistent temporary buffers for safe text input (avoid using string literals)
    static char set_hotkey_buf[64] = {0};
    static char input_device_buf[256] = {0};

    ImGui::RadioButton("Port 0: Mouse", &SelectedPort, 0);
    ImGui::RadioButton("Port 1: Joystick", &SelectedPort, 1);
    ImGui::RadioButton("Port 2: Parallel 1", &SelectedPort, 2);
    ImGui::RadioButton("Port 3: Parallel 2", &SelectedPort, 3);

    ImGui::RadioButton("None", &SelectedFunction, 0);
    ImGui::RadioButton("HotKey", &SelectedFunction, 1);

    // Use writable buffers instead of string literals
    ImGui::InputText("##SetHotkey", set_hotkey_buf, sizeof(set_hotkey_buf));
    ImGui::SameLine();
    ImGui::Button("...");
    ImGui::SameLine();
    ImGui::Button("X");

    ImGui::InputText("Input Device", input_device_buf, sizeof(input_device_buf));

    const char* items[] = { "None" };

    // Buttons mapping list
    int bindex = 0;
    for (const auto& label : label_button_list) {
        ImGui::PushID(bindex);
        ImGui::Text("%s", label.c_str());
        ImGui::SameLine();
        ImGui::Combo("##btnmap", &changed_prefs.jports[SelectedPort].autofire, items, 1);
        ImGui::PopID();
        ++bindex;
    }

    // Axes mapping list
    int aindex = 0;
    for (const auto& label : label_axis_list) {
        ImGui::PushID(1000 + aindex);
        ImGui::Text("%s", label.c_str());
        ImGui::SameLine();
        ImGui::Combo("##axmap", &changed_prefs.jports[SelectedPort].autofire, items, 1);
        ImGui::PopID();
        ++aindex;
    }

    if (ImGui::Button("Save as default mapping")) {
        // TODO: Save mapping defaults
    }
}
