#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "imgui_panels.h"
#include <vector>
#include <string>

#include "gui/gui_handling.h"

// Local constants matching inputdevice.h if needed
#ifndef TABLET_OFF
#define TABLET_OFF 0
#define TABLET_MOUSEHACK 1
#endif
#ifndef MOUSEUNTRAP_MIDDLEBUTTON
#define MOUSEUNTRAP_MIDDLEBUTTON 1
#endif
#ifndef MOUSEUNTRAP_MAGIC
#define MOUSEUNTRAP_MAGIC 2
#endif

static std::vector<std::string> input_device_names;
static std::vector<const char *> input_device_items;
static std::vector<int> input_device_ids;

static void poll_input_devices() {
    input_device_names.clear();
    input_device_items.clear();
    input_device_ids.clear();

    // Static Items
    auto add = [](const char *name, int id) {
        input_device_names.emplace_back(name);
        input_device_ids.push_back(id);
    };

    add("<none>", JPORT_NONE);
    add("Keyboard Layout A (Numpad, 0/5=Fire, Decimal/DEL=2nd Fire)", JSEM_KBDLAYOUT);
    add("Keyboard Layout B (Cursor, RCtrl/RAlt=Fire, RShift=2nd Fire)", JSEM_KBDLAYOUT + 1);
    add("Keyboard Layout C (WSAD, LAlt=Fire, LShift=2nd Fire)", JSEM_KBDLAYOUT + 2);
    add("Keyrah Layout (Cursor, Space/RAlt=Fire, RShift=2nd Fire)", JSEM_KBDLAYOUT + 3);

    for (int j = 0; j < 4; j++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Retroarch KBD as Joystick Player%d", j + 1);
        add(buf, JSEM_KBDLAYOUT + j + 4);
    }

    // Joysticks
    int num_joys = inputdevice_get_device_total(IDTYPE_JOYSTICK);
    for (int j = 0; j < num_joys; j++) {
        add(inputdevice_get_device_name(IDTYPE_JOYSTICK, j), JSEM_JOYS + j);
    }

    // Mice
    int num_mice = inputdevice_get_device_total(IDTYPE_MOUSE);
    for (int j = 0; j < num_mice; j++) {
        add(inputdevice_get_device_name(IDTYPE_MOUSE, j), JSEM_MICE + j);
    }

    for (const auto &name: input_device_names) input_device_items.push_back(name.c_str());
}

static int get_device_index(int id) {
    for (size_t i = 0; i < input_device_ids.size(); ++i) {
        if (input_device_ids[i] == id) return (int) i;
    }
    return 0; // Default to <none>
}

void render_panel_input() {
    if (input_device_names.empty()) poll_input_devices();

    ImGui::Indent(4.0f);

    // ---------------------------------------------------------
    // Mouse and Joystick settings
    // ---------------------------------------------------------
    BeginGroupBox("Mouse and Joystick settings");

    if (ImGui::BeginTable("PortsTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        ImGui::TableSetupColumn("controls", ImGuiTableColumnFlags_WidthStretch);

        auto render_port = [&](int port_idx, const char *label) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::TableNextColumn();

            int dev_idx = get_device_index(changed_prefs.jports[port_idx].id);
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            if (ImGui::BeginCombo(std::string("##Dev").append(std::to_string(port_idx)).c_str(),
                                  input_device_items[dev_idx])) {
                for (int n = 0; n < static_cast<int>(input_device_items.size()); n++) {
                    const bool is_selected = (dev_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(input_device_items[n], is_selected)) {
                        dev_idx = n;
                        changed_prefs.jports[port_idx].id = input_device_ids[dev_idx];
                        inputdevice_validate_jports(&changed_prefs, port_idx, nullptr); // Validate change
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

            float avail_w = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2;
            float item_w = (avail_w - BUTTON_WIDTH * 1.5f) / 2;
            ImGui::SetNextItemWidth(item_w);

            const char *autofire_names[] = {
                "No autofire (normal)", "Autofire", "Autofire (toggle)", "Autofire (always)", "No autofire (toggle)"
            };
            if (ImGui::BeginCombo(std::string("##Autofire").append(std::to_string(port_idx)).c_str(),
                                  autofire_names[changed_prefs.jports[port_idx].autofire])) {
                for (int n = 0; n < IM_ARRAYSIZE(autofire_names); n++) {
                    const bool is_selected = (changed_prefs.jports[port_idx].autofire == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(autofire_names[n], is_selected)) {
                        changed_prefs.jports[port_idx].autofire = n;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

            ImGui::SameLine();
            ImGui::SetNextItemWidth(item_w);

            const char *mode_names[] = {
                "Default", "Wheel Mouse", "Mouse", "Joystick", "Gamepad", "Analog Joystick", "CDTV remote mouse",
                "CD32 pad"
            };
            if (ImGui::BeginCombo(std::string("##Mode").append(std::to_string(port_idx)).c_str(),
                                  mode_names[changed_prefs.jports[port_idx].mode])) {
                for (int n = 0; n < IM_ARRAYSIZE(mode_names); n++) {
                    const bool is_selected = (changed_prefs.jports[port_idx].mode == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(mode_names[n], is_selected)) {
                        changed_prefs.jports[port_idx].mode = n;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

            ImGui::SameLine();
            AmigaButton(std::string("Remap / Test##").append(std::to_string(port_idx)).c_str());

            // Row 3: Mouse Map
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Mouse map:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(item_w);

            ImGui::BeginDisabled(changed_prefs.jports[port_idx].mode == 0);
            const char *mouse_map_names[] = {"None", "LStick"};
            if (ImGui::BeginCombo(std::string("##MouseMap").append(std::to_string(port_idx)).c_str(),
                                  mouse_map_names[changed_prefs.jports[port_idx].mousemap])) {
                for (int n = 0; n < IM_ARRAYSIZE(mouse_map_names); n++) {
                    const bool is_selected = (changed_prefs.jports[port_idx].mousemap == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(mouse_map_names[n], is_selected)) {
                        changed_prefs.jports[port_idx].mousemap = n;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ImGui::EndDisabled();
        };

        render_port(0, "Port 1:");
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); // separator row spans both columns
        ImGui::TableNextColumn();
        ImGui::Separator();
        render_port(1, "Port 2:");

        // Swap & Autoswitch in Column 2
        ImGui::TableNextRow();
        ImGui::TableNextColumn(); // Skip Col 1
        ImGui::TableNextColumn();
        ImGui::Dummy(ImVec2(0, 8.0f));
        AmigaButton("Swap ports");
        ImGui::SameLine();
        AmigaCheckbox("Mouse/Joystick autoswitching", &changed_prefs.input_autoswitch);

        ImGui::EndTable();
    }
    ImGui::Spacing();
    EndGroupBox("Mouse and Joystick settings");

    // ---------------------------------------------------------
    // Emulated Parallel Port
    // ---------------------------------------------------------
    BeginGroupBox("Emulated parallel port joystick adapter");

    if (ImGui::BeginTable("ParTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        ImGui::TableSetupColumn("controls", ImGuiTableColumnFlags_WidthStretch);

        auto render_par_port = [&](int port_idx, const char *label) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::TableNextColumn();

            int dev_idx = get_device_index(changed_prefs.jports[port_idx].id);
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
            if (ImGui::BeginCombo(std::string("##ParDev").append(std::to_string(port_idx)).c_str(),
                                  input_device_items[dev_idx])) {
                for (int n = 0; n < static_cast<int>(input_device_items.size()); n++) {
                    const bool is_selected = (dev_idx == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(input_device_items[n], is_selected)) {
                        dev_idx = n;
                        changed_prefs.jports[port_idx].id = input_device_ids[dev_idx];
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

            // Row 2: Autofire | Remap
            float avail_w = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2;
            float item_w = (avail_w - BUTTON_WIDTH * 1.5f) / 2;

            ImGui::SetNextItemWidth(item_w);
            const char *autofire_names[] = {
                "No autofire (normal)", "Autofire", "Autofire (toggle)", "Autofire (always)", "No autofire (toggle)"
            };
            if (ImGui::BeginCombo(std::string("##ParAutofire").append(std::to_string(port_idx)).c_str(),
                                  autofire_names[changed_prefs.jports[port_idx].autofire])) {
                for (int n = 0; n < IM_ARRAYSIZE(autofire_names); n++) {
                    const bool is_selected = (changed_prefs.jports[port_idx].autofire == n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    if (ImGui::Selectable(autofire_names[n], is_selected)) {
                        changed_prefs.jports[port_idx].autofire = n;
                    }
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

            ImGui::SameLine();
            AmigaButton(std::string("Remap / Test##Par").append(std::to_string(port_idx)).c_str());
        };

        render_par_port(2, "Port 3:");
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        render_par_port(3, "Port 4:");

        ImGui::EndTable();
    }
    ImGui::Spacing();
    EndGroupBox("Emulated parallel port joystick adapter");

    // ---------------------------------------------------------
    // Game Controller Settings
    // ---------------------------------------------------------
    static const int digital_joymousespeed_values[] = {2, 5, 10, 15, 20};
    static const int analog_joymousespeed_values[] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 75, 100, 125, 150};

    BeginGroupBox("Game Controller Settings");
    // Joystick Dead Zone
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Joystick dead zone:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    ImGui::SliderInt("##DeadZone", &changed_prefs.input_joystick_deadzone, 0, 100, "%d%%");
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

    // Autofire Rate
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Autofire Rate:");
    ImGui::SameLine();
    int af_rate_idx = 0;
    if (changed_prefs.input_autofire_linecnt == 0) af_rate_idx = 0;
    else if (changed_prefs.input_autofire_linecnt > 10 * 312) af_rate_idx = 1;
    else if (changed_prefs.input_autofire_linecnt > 6 * 312) af_rate_idx = 2;
    else af_rate_idx = 3;

    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    const char *af_rate_names[] = {"Off", "Slow", "Medium", "Fast"};
    if (ImGui::BeginCombo("##AutofireRate", af_rate_names[af_rate_idx])) {
        for (int n = 0; n < IM_ARRAYSIZE(af_rate_names); n++) {
            const bool is_selected = (af_rate_idx == n);
            if (is_selected)
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
            if (ImGui::Selectable(af_rate_names[n], is_selected)) {
                af_rate_idx = n;
                if (af_rate_idx == 0) changed_prefs.input_autofire_linecnt = 0;
                else if (af_rate_idx == 1) changed_prefs.input_autofire_linecnt = 12 * 312;
                else if (af_rate_idx == 2) changed_prefs.input_autofire_linecnt = 8 * 312;
                else changed_prefs.input_autofire_linecnt = 4 * 312;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

    // Digital Joy-Mouse Speed
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Digital joy-mouse speed:");
    ImGui::SameLine();
    int dig_speed_idx = 2; // Default 10
    for (int i = 0; i < IM_ARRAYSIZE(digital_joymousespeed_values); ++i) {
        if (changed_prefs.input_joymouse_speed == digital_joymousespeed_values[i]) {
            dig_speed_idx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    if (ImGui::SliderInt("##DigSpeed", &dig_speed_idx, 0, IM_ARRAYSIZE(digital_joymousespeed_values) - 1, "")) {
        changed_prefs.input_joymouse_speed = digital_joymousespeed_values[dig_speed_idx];
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%d", changed_prefs.input_joymouse_speed);

    // Analog Joy-Mouse Speed
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Analog joy-mouse speed:");
    ImGui::SameLine();
    int ana_speed_idx = 1; // Default 10
    for (int i = 0; i < IM_ARRAYSIZE(analog_joymousespeed_values); ++i) {
        if (changed_prefs.input_joymouse_multiplier == analog_joymousespeed_values[i]) {
            ana_speed_idx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    if (ImGui::SliderInt("##AnaSpeed", &ana_speed_idx, 0, IM_ARRAYSIZE(analog_joymousespeed_values) - 1, "")) {
        changed_prefs.input_joymouse_multiplier = analog_joymousespeed_values[ana_speed_idx];
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%d", changed_prefs.input_joymouse_multiplier);
    ImGui::Spacing();
    EndGroupBox("Game Controller Settings");

    // ---------------------------------------------------------
    // Mouse Extra Settings
    // ---------------------------------------------------------
    BeginGroupBox("Mouse extra settings");

    if (ImGui::BeginTable("MouseExtrasTable", 2, ImGuiTableFlags_None)) {
        // Row 1
        ImGui::TableNextRow();

        // Left: Mouse Speed
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mouse speed:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        ImGui::InputInt("##MouseSpd", &changed_prefs.input_mouse_speed, 1, 10);
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);

        // Right: Mouse Untrap (Inline Label)
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mouse untrap:");
        ImGui::SameLine();

        const char *untrap_items[] = {"None", "Middle button", "Magic mouse", "Both"};
        int untrap_val = changed_prefs.input_mouse_untrap;
        int untrap_idx = 0;
        if ((untrap_val & MOUSEUNTRAP_MIDDLEBUTTON) && (untrap_val & MOUSEUNTRAP_MAGIC)) untrap_idx = 3;
        else if (untrap_val & MOUSEUNTRAP_MAGIC) untrap_idx = 2;
        else if (untrap_val & MOUSEUNTRAP_MIDDLEBUTTON) untrap_idx = 1;

        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##UntrapMode", untrap_items[untrap_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(untrap_items); n++) {
                const bool is_selected = (untrap_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(untrap_items[n], is_selected)) {
                    untrap_idx = n;
                    int new_val = 0;
                    if (untrap_idx == 1) new_val = MOUSEUNTRAP_MIDDLEBUTTON;
                    else if (untrap_idx == 2) new_val = MOUSEUNTRAP_MAGIC;
                    else if (untrap_idx == 3) new_val = MOUSEUNTRAP_MIDDLEBUTTON | MOUSEUNTRAP_MAGIC;
                    changed_prefs.input_mouse_untrap = new_val;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        // Row 2
        ImGui::TableNextRow();

        // Left: Virtual Mouse
        ImGui::TableNextColumn();
        bool virt_mouse = changed_prefs.input_tablet > 0;
        if (AmigaCheckbox("Install virtual mouse driver", &virt_mouse)) {
            changed_prefs.input_tablet = virt_mouse ? TABLET_MOUSEHACK : TABLET_OFF;
        }

        // Right: Cursor Mode
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Magic Mouse cursor:");
        ImGui::SameLine();
        const char *magic_items[] = {"Both", "Native only", "Host only"};
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);

        ImGui::BeginDisabled(changed_prefs.input_tablet == 0);
        if (ImGui::BeginCombo("##MagicMouse", magic_items[changed_prefs.input_magic_mouse_cursor])) {
            for (int n = 0; n < IM_ARRAYSIZE(magic_items); n++) {
                const bool is_selected = (changed_prefs.input_magic_mouse_cursor == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(magic_items[n], is_selected)) {
                    changed_prefs.input_magic_mouse_cursor = n;
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ImGui::EndDisabled();

        // Row 3
        ImGui::TableNextRow();

        // Left: Tablet Library emul
        ImGui::TableNextColumn();
        AmigaCheckbox("Tablet.library emulation", &changed_prefs.tablet_library);

        // Right: Tablet Mode (Aligned with Row 3)
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Tablet mode:");
        ImGui::SameLine();

        int tablet_mode = changed_prefs.input_tablet == TABLET_MOUSEHACK ? 1 : 0;
        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::Combo("##TabletMode", &tablet_mode, "Disabled\0MouseHack\0")) {
            changed_prefs.input_tablet = tablet_mode == 1 ? TABLET_MOUSEHACK : TABLET_OFF;
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());

        ImGui::EndTable();
    }
    ImGui::Spacing();
    EndGroupBox("Mouse extra settings");
}
