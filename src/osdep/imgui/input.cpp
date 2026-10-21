#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "imgui_panels.h"
#include "gui/gui_handling.h"
#include "settings/settings_definitions.h"
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include <cstring>

// Local constants matching inputdevice.h if needed
#ifndef TABLET_OFF
#define TABLET_OFF 0
#define TABLET_MOUSEHACK 1
#define TABLET_REAL 2
#endif

namespace settings = amiberry_settings;

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules, the device lists and the dependent changes a new
// value implies.
struct input_options {
    const settings::definition* joyport[4] = {
        &settings::require("joyport0"), &settings::require("joyport1"),
        &settings::require("joyport2"), &settings::require("joyport3"),
    };
    const settings::definition* joyport_autofire[4] = {
        &settings::require("joyport0autofire"), &settings::require("joyport1autofire"),
        &settings::require("joyport2autofire"), &settings::require("joyport3autofire"),
    };
    // Ports 1 and 2 only: the configuration stores no mode or mouse map
    // for the parallel-port adapter ports.
    const settings::definition* joyport_mode[2] = {
        &settings::require("joyport0mode"), &settings::require("joyport1mode"),
    };
    const settings::definition* joyport_mousemap[2] = {
        &settings::require("joyport0mousemap"), &settings::require("joyport1mousemap"),
    };
    const settings::definition& input_autoswitch = settings::require("input.autoswitch");
    const settings::definition& input_multi_mouse = settings::require("input.multi_mouse");
    const settings::definition& input_joystick_up_button = settings::require("input.joystick_up_button");
    const settings::definition& input_joystick_deadzone = settings::require("input.joystick_deadzone");
    const settings::definition& input_autofire_speed = settings::require("input.autofire_speed");
    const settings::definition& joymouse_speed_digital = settings::require("input.joymouse_speed_digital");
    const settings::definition& joymouse_speed_analog = settings::require("input.joymouse_speed_analog");
    const settings::definition& input_mouse_speed = settings::require("input.mouse_speed");
    const settings::definition& magic_mouse = settings::require("magic_mouse");
    const settings::definition& middle_mouse = settings::require("amiberry.middle_mouse");
    const settings::definition& magic_mousecursor = settings::require("magic_mousecursor");
    const settings::definition& tablet_library = settings::require("tablet_library");
    const settings::definition& absolute_mouse = settings::require("absolute_mouse");
};

const input_options& options()
{
    static const input_options o;
    return o;
}

bool available(const settings::definition& d, const settings::context& ctx)
{
    return settings::is_available(d, changed_prefs, ctx);
}

void option_checkbox(const char* label, const settings::definition& d, const settings::context& ctx)
{
    bool checked = settings::get_bool(d, changed_prefs);
    if (AmigaCheckbox(label, &checked))
        settings::set_bool(d, changed_prefs, ctx, checked);
}

// The device id a joyport token names, for the host-side calls the panel
// still makes itself (the custom mapping and the controller mapper).
int input_device_id(const char* token)
{
    if (!strcmp(token, "none"))
        return JPORT_NONE;
    int index = 0;
    bool valid = true;
    auto number = [&](const char* p) {
        int value = 0;
        bool seen = false;
        for (; *p; p++) {
            if (*p < '0' || *p > '9')
                return -1;
            value = value * 10 + (*p - '0');
            seen = true;
        }
        return seen ? value : -1;
    };
    if (!strncmp(token, "kbd", 3)) {
        index = number(token + 3);
        if (index < 1 || index > JSEM_LASTKBD)
            valid = false;
        return valid ? JSEM_KBDLAYOUT + index - 1 : JPORT_NONE;
    }
    if (!strncmp(token, "joy", 3)) {
        index = number(token + 3);
        return index >= 0 ? JSEM_JOYS + index : JPORT_NONE;
    }
    if (!strcmp(token, "mouse"))
        return JSEM_MICE;
    if (!strncmp(token, "mouse", 5)) {
        index = number(token + 5);
        return index > 0 ? JSEM_MICE + index : JPORT_NONE;
    }
    return JPORT_NONE;
}

// The port device list, mirrored from the settings layer's dynamic
// choices so this panel and the global settings rows share one list.
std::vector<InputDeviceOption> input_device_options;

} // namespace

const std::vector<InputDeviceOption>& get_input_device_options()
{
    const settings::definition& d = *options().joyport[0];
    const settings::choice_list choices = settings::choices_of(d, changed_prefs);
    input_device_options.clear();
    for (size_t i = 0; i < choices.count; i++) {
        input_device_options.push_back({choices.items[i].label, choices.items[i].value,
            input_device_id(choices.items[i].value)});
    }
    return input_device_options;
}

static void set_port_input_device(const int port_idx, const int selected_idx)
{
    const auto& device_options = get_input_device_options();
    if (selected_idx < 0 || selected_idx >= static_cast<int>(device_options.size()))
        return;

    const settings::context ctx{emulating != 0};
    settings::set(*options().joyport[port_idx], changed_prefs, ctx,
        device_options[selected_idx].config_value.c_str());
    // Host-side bookkeeping that belongs to the device, not the
    // configuration, in the order the panel always ran it: drop the old
    // controller's mapping snapshot and reconnect target, validate again
    // with the usage history (the layer's setter validates the
    // configuration only, since sessions must not touch device state), and
    // snapshot the newly chosen controller's mapping.
    amiberry_clear_port_joystick_custom_mapping(port_idx);
    inputdevice_forget_unplugged_device(port_idx);
    inputdevice_validate_jports(&changed_prefs, port_idx, nullptr);
    const auto& selected_port = changed_prefs.jports[port_idx];
    if (selected_port.id >= JSEM_JOYS && selected_port.id < JSEM_MICE) {
        amiberry_preserve_port_joystick_custom_mapping(
            port_idx, selected_port.idc.name, selected_port.idc.configname);
    }
}

static int get_device_index(const int id)
{
    const auto& options = get_input_device_options();
    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i].id == id)
            return static_cast<int>(i);
    }
    return 0; // Default to <none>
}

bool InputDeviceCombo(const char* id, const int current_index, const char* fallback_preview,
	int* selected_index)
{
	const auto& options = get_input_device_options();
	const bool current_valid = current_index >= 0 && current_index < static_cast<int>(options.size());
	const char* preview = current_valid
		? options[current_index].label.c_str()
		: (fallback_preview ? fallback_preview : "<none>");

	bool changed = false;
	if (ImGui::BeginCombo(id, preview)) {
		for (int n = 0; n < static_cast<int>(options.size()); ++n) {
			const bool is_selected = current_index == n;
			if (is_selected)
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
			ImGui::PushID(n);
			if (ImGui::Selectable(options[n].label.c_str(), is_selected)) {
				*selected_index = n;
				changed = true;
			}
			ImGui::PopID();
			if (is_selected) {
				ImGui::PopStyleColor();
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
	return changed;
}

void render_panel_input() {
    const input_options& o = options();
    const settings::context ctx{emulating != 0};

    std::string mapping;
    if (ControllerMap_ConsumeResult(mapping)) {
        SDL_AddGamepadMapping(mapping.c_str());
        save_mapping_to_file(mapping);
    }

	get_input_device_options();

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

            char current[AMIBERRY_SETTINGS_VALUE_MAX];
            settings::get(*o.joyport[port_idx], changed_prefs, current, sizeof current);
            int dev_idx = get_device_index(changed_prefs.jports[port_idx].id);
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
			int selected_idx = dev_idx;
			const std::string combo_id = "##Dev" + std::to_string(port_idx);
			if (InputDeviceCombo(combo_id.c_str(), dev_idx, current, &selected_idx))
				set_port_input_device(port_idx, selected_idx);
            ShowHelpMarker("Select the input device to use for this Amiga port. Port 0 is typically for mouse, Port 1 for joystick");

            float avail_w = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2;
            float item_w = (avail_w - BUTTON_WIDTH * 1.5f) / 2;
            ImGui::SetNextItemWidth(item_w);

            const amiberry_setting_descriptor& af_desc = o.joyport_autofire[port_idx]->descriptor;
            {
                char af_current[AMIBERRY_SETTINGS_VALUE_MAX];
                settings::get(*o.joyport_autofire[port_idx], changed_prefs, af_current,
                    sizeof af_current);
                const char* preview = af_current;
                int selected = -1;
                for (size_t n = 0; n < af_desc.choice_count; n++) {
                    if (!strcmp(af_current, af_desc.choices[n].value)) {
                        selected = static_cast<int>(n);
                        preview = af_desc.choices[n].label;
                    }
                }
                const std::string af_id = "##Autofire" + std::to_string(port_idx);
                if (ImGui::BeginCombo(af_id.c_str(), preview)) {
                    for (size_t n = 0; n < af_desc.choice_count; n++) {
                        const bool is_selected = selected == static_cast<int>(n);
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                        ImGui::PushID(static_cast<int>(n));
                        if (ImGui::Selectable(af_desc.choices[n].label, is_selected))
                            settings::set(*o.joyport_autofire[port_idx], changed_prefs, ctx,
                                af_desc.choices[n].value);
                        ImGui::PopID();
                        if (is_selected) {
                            ImGui::PopStyleColor();
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ShowHelpMarker("Automatically repeat fire button presses. Toggle mode allows switching on/off with a button");

            ImGui::SameLine();
            ImGui::SetNextItemWidth(item_w);

            // The mode list comes from the layer, so a lightpen mode from
            // a configuration no longer indexes past the old list.
            const amiberry_setting_descriptor& mode_desc = o.joyport_mode[port_idx]->descriptor;
            {
                char mode_current[AMIBERRY_SETTINGS_VALUE_MAX];
                settings::get(*o.joyport_mode[port_idx], changed_prefs, mode_current,
                    sizeof mode_current);
                const char* preview = mode_current[0] ? mode_current : mode_desc.choices[0].label;
                int selected = -1;
                for (size_t n = 0; n < mode_desc.choice_count; n++) {
                    if (!strcmp(mode_current, mode_desc.choices[n].value)) {
                        selected = static_cast<int>(n);
                        preview = mode_desc.choices[n].label;
                    }
                }
                const std::string mode_id = "##Mode" + std::to_string(port_idx);
                if (ImGui::BeginCombo(mode_id.c_str(), preview)) {
                    for (size_t n = 0; n < mode_desc.choice_count; n++) {
                        const bool is_selected = selected == static_cast<int>(n);
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                        ImGui::PushID(static_cast<int>(n));
                        if (ImGui::Selectable(mode_desc.choices[n].label, is_selected))
                            settings::set(*o.joyport_mode[port_idx], changed_prefs, ctx,
                                mode_desc.choices[n].value);
                        ImGui::PopID();
                        if (is_selected) {
                            ImGui::PopStyleColor();
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ShowHelpMarker("Select the emulation mode for this device (default, mouse, joystick, gamepad, CD32 pad, etc.)");

            ImGui::SameLine();
            const bool remap_enabled = changed_prefs.jports[port_idx].id >= JSEM_JOYS
                                       && changed_prefs.jports[port_idx].id < JSEM_MICE - 1;
            ImGui::BeginDisabled(!remap_enabled);
            if (AmigaButton(std::string("Remap / Test##").append(std::to_string(port_idx)).c_str())) {
                const auto host_joy_id = changed_prefs.jports[port_idx].id - JSEM_JOYS;
                ControllerMap_Open(host_joy_id, false);
            }
            ImGui::EndDisabled();

            // Row 3: Mouse Map
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Mouse map:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(item_w);

            ImGui::BeginDisabled(!available(*o.joyport_mousemap[port_idx], ctx));
            // The value is a bitmask the hotkeys can toggle per stick;
            // show any nonzero value as LStick so the combo never indexes
            // past its list, the way the old panel did.
            {
                const int mouse_map_value = changed_prefs.jports[port_idx].mousemap > 0 ? 1 : 0;
                const amiberry_setting_descriptor& mm_desc = o.joyport_mousemap[port_idx]->descriptor;
                const std::string mm_id = "##MouseMap" + std::to_string(port_idx);
                if (ImGui::BeginCombo(mm_id.c_str(), mm_desc.choices[mouse_map_value].label)) {
                    for (size_t n = 0; n < mm_desc.choice_count; n++) {
                        const bool is_selected = mouse_map_value == static_cast<int>(n);
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                        ImGui::PushID(static_cast<int>(n));
                        if (ImGui::Selectable(mm_desc.choices[n].label, is_selected))
                            settings::set(*o.joyport_mousemap[port_idx], changed_prefs, ctx,
                                mm_desc.choices[n].value);
                        ImGui::PopID();
                        if (is_selected) {
                            ImGui::PopStyleColor();
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ShowHelpMarker("Map mouse movement to analog stick (LStick). Only available when mode is not default");
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
        if (AmigaButton(ICON_FA_SHUFFLE " Swap ports")) {
            inputdevice_swap_compa_ports(&changed_prefs, 0);
        }
        ImGui::SameLine();
        option_checkbox("Mouse/Joystick autoswitching", o.input_autoswitch, ctx);
        ShowHelpMarker("Automatically switch Port 0 and Port 1 devices based on input activity");

        // Multi-mouse toggle — live-applies via a targeted mouse re-init that
        // preserves port bindings by index. We deliberately avoid
        // inputdevice_devicechange() here: that routine rebinds by friendly
        // name, and the "System mouse" ↔ SDL-device-name swap during a mode
        // toggle would make the match fail and shuffle the port assignment.
        ImGui::SameLine();
        const bool prev_multi_mouse = changed_prefs.input_multi_mouse;
        option_checkbox("Multi-mouse support", o.input_multi_mouse, ctx);
        ShowHelpMarker("Treat each physical mouse as a separate device for 2-player mouse games (e.g., Lemmings). "
                       "Default: off — all mouse input is merged into a single System mouse, which is the most reliable option.");
        if (prev_multi_mouse != changed_prefs.input_multi_mouse) {
            inputdevice_mouse_reinit(&changed_prefs);
            // Repopulate the dropdown labels immediately so this frame
            // reflects the new device list.
            joystick_refresh_needed = true;
            get_input_device_options();
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); // Skip Col 1
        ImGui::TableNextColumn();
        option_checkbox("Map left face button (Xbox X / PS Square) as extra Joy Up",
            o.input_joystick_up_button, ctx);
        ShowHelpMarker("Binds the left face button (Xbox X / PlayStation Square) as a duplicate of "
                       "Joystick Up on Joystick and Default port modes, giving a free dedicated jump "
                       "button on modern gamepads. In Default mode this overrides the rarely-used "
                       "3rd fire button. Does not apply to CD32 or Gamepad modes.");

        ImGui::EndTable();
    }
    ImGui::Spacing();
    EndGroupBox("Mouse and Joystick settings");

    // ---------------------------------------------------------
    // Emulated Parallel Port
    // ---------------------------------------------------------
    if (BeginGroupBox("Emulated parallel port joystick adapter", true)) {

    if (ImGui::BeginTable("ParTable", 2, ImGuiTableFlags_None)) {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH);
        ImGui::TableSetupColumn("controls", ImGuiTableColumnFlags_WidthStretch);

        auto render_par_port = [&](int port_idx, const char *label) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::TableNextColumn();

            char current[AMIBERRY_SETTINGS_VALUE_MAX];
            settings::get(*o.joyport[port_idx], changed_prefs, current, sizeof current);
            int dev_idx = get_device_index(changed_prefs.jports[port_idx].id);
            ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
			int selected_idx = dev_idx;
			const std::string combo_id = "##ParDev" + std::to_string(port_idx);
			if (InputDeviceCombo(combo_id.c_str(), dev_idx, current, &selected_idx))
				set_port_input_device(port_idx, selected_idx);

            // Row 2: Autofire | Remap
            float avail_w = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2;
            float item_w = (avail_w - BUTTON_WIDTH * 1.5f) / 2;

            ImGui::SetNextItemWidth(item_w);
            const amiberry_setting_descriptor& af_desc = o.joyport_autofire[port_idx]->descriptor;
            {
                char af_current[AMIBERRY_SETTINGS_VALUE_MAX];
                settings::get(*o.joyport_autofire[port_idx], changed_prefs, af_current,
                    sizeof af_current);
                const char* preview = af_current;
                int selected = -1;
                for (size_t n = 0; n < af_desc.choice_count; n++) {
                    if (!strcmp(af_current, af_desc.choices[n].value)) {
                        selected = static_cast<int>(n);
                        preview = af_desc.choices[n].label;
                    }
                }
                const std::string af_id = "##ParAutofire" + std::to_string(port_idx);
                if (ImGui::BeginCombo(af_id.c_str(), preview)) {
                    for (size_t n = 0; n < af_desc.choice_count; n++) {
                        const bool is_selected = selected == static_cast<int>(n);
                        if (is_selected)
                            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                        ImGui::PushID(static_cast<int>(n));
                        if (ImGui::Selectable(af_desc.choices[n].label, is_selected))
                            settings::set(*o.joyport_autofire[port_idx], changed_prefs, ctx,
                                af_desc.choices[n].value);
                        ImGui::PopID();
                        if (is_selected) {
                            ImGui::PopStyleColor();
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
            ShowHelpMarker("Automatically repeat fire button presses on this parallel port device");

            ImGui::SameLine();
            const bool remap_enabled = changed_prefs.jports[port_idx].id >= JSEM_JOYS
                                       && changed_prefs.jports[port_idx].id < JSEM_MICE - 1;
            ImGui::BeginDisabled(!remap_enabled);
            if (AmigaButton(std::string("Remap / Test##Par").append(std::to_string(port_idx)).c_str())) {
                const auto host_joy_id = changed_prefs.jports[port_idx].id - JSEM_JOYS;
                ControllerMap_Open(host_joy_id, false);
            }
            ImGui::EndDisabled();
        };

        render_par_port(2, "Port 3:");
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        render_par_port(3, "Port 4:");

        ImGui::EndTable();
    }
    ImGui::Spacing();
    }
    EndGroupBox("Emulated parallel port joystick adapter");

    if (BeginGroupBox("Game Controller Settings", true, false)) {
    // Joystick Dead Zone
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Joystick dead zone:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    {
        double min, max;
        settings::get_range(o.input_joystick_deadzone, changed_prefs, min, max);
        int deadzone = static_cast<int>(settings::get_number(o.input_joystick_deadzone,
            changed_prefs));
        if (ImGui::SliderInt("##DeadZone", &deadzone, static_cast<int>(min), static_cast<int>(max),
                "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
            settings::set_number(o.input_joystick_deadzone, changed_prefs, ctx, deadzone);
        }
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
    ShowHelpMarker("Analog stick dead zone - area of no response around center position (0-100%)");

    // Autofire Rate
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Autofire Rate:");
    ImGui::SameLine();
    int af_rate_idx = 0;
    const int linecnt = static_cast<int>(settings::get_number(o.input_autofire_speed,
        changed_prefs));
    if (linecnt == 0) af_rate_idx = 0;
    else if (linecnt > 10 * 312) af_rate_idx = 1;
    else if (linecnt > 6 * 312) af_rate_idx = 2;
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
                static const int rates[] = {0, 12 * 312, 8 * 312, 4 * 312};
                settings::set_number(o.input_autofire_speed, changed_prefs, ctx, rates[n]);
            }
            if (is_selected) {
                ImGui::PopStyleColor();
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
    ShowHelpMarker("How fast autofire repeats (lower values = faster repeat rate)");

    // Digital Joy-Mouse Speed
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Digital joy-mouse speed:");
    ImGui::SameLine();
    const amiberry_setting_descriptor& dig_desc = o.joymouse_speed_digital.descriptor;
    int dig_speed_idx = 0;
    for (size_t i = 0; i < dig_desc.choice_count; i++) {
        if (changed_prefs.input_joymouse_speed == atoi(dig_desc.choices[i].value)) {
            dig_speed_idx = static_cast<int>(i);
            break;
        }
    }
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    if (ImGui::SliderInt("##DigSpeed", &dig_speed_idx, 0,
            static_cast<int>(dig_desc.choice_count) - 1, "", ImGuiSliderFlags_AlwaysClamp)) {
        settings::set(o.joymouse_speed_digital, changed_prefs, ctx,
            dig_desc.choices[dig_speed_idx].value);
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
    ShowHelpMarker("Mouse cursor speed when controlled by digital joystick directions");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%d", changed_prefs.input_joymouse_speed);

    // Analog Joy-Mouse Speed
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Analog joy-mouse speed:");
    ImGui::SameLine();
    // Nearest listed value, so a hand-edited config still shows a sensible
    // position instead of snapping the display to the default.
    const amiberry_setting_descriptor& ana_desc = o.joymouse_speed_analog.descriptor;
    int ana_speed_idx = 0;
    for (size_t i = 1; i < ana_desc.choice_count; i++) {
        if (abs(atoi(ana_desc.choices[i].value) - changed_prefs.input_joymouse_multiplier)
            < abs(atoi(ana_desc.choices[ana_speed_idx].value) - changed_prefs.input_joymouse_multiplier))
            ana_speed_idx = static_cast<int>(i);
    }
    ImGui::SetNextItemWidth(BUTTON_WIDTH);
    if (ImGui::SliderInt("##AnaSpeed", &ana_speed_idx, 0,
            static_cast<int>(ana_desc.choice_count) - 1, "", ImGuiSliderFlags_AlwaysClamp)) {
        settings::set(o.joymouse_speed_analog, changed_prefs, ctx,
            ana_desc.choices[ana_speed_idx].value);
    }
    AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
    ShowHelpMarker("Mouse pointer speed when an analog stick drives the mouse (Mouse Map, or a controller in Mouse mode). Lower is slower");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%d", changed_prefs.input_joymouse_multiplier);
    ImGui::Spacing();
    }
    EndGroupBox("Game Controller Settings");

    if (BeginGroupBox("Mouse extra settings", true, false)) {

    const float mouse_extras_width = std::min(BUTTON_WIDTH * 6.0f, ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginTable("MouseExtrasTable", 2, ImGuiTableFlags_None,
        ImVec2(mouse_extras_width, 0.0f))) {
        // Row 1
        ImGui::TableNextRow();

        // Left: Mouse Speed
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mouse speed:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(BUTTON_WIDTH);
        {
            double min, max;
            settings::get_range(o.input_mouse_speed, changed_prefs, min, max);
            int speed = static_cast<int>(settings::get_number(o.input_mouse_speed, changed_prefs));
            if (ImGui::InputInt("##MouseSpd", &speed, 1, 10)) {
                speed = std::max(static_cast<int>(min), std::min(static_cast<int>(max), speed));
                settings::set_number(o.input_mouse_speed, changed_prefs, ctx, speed);
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true);
        ShowHelpMarker("Speed of real mouse and touch pointer movement (100 = normal speed, higher = faster). Analog stick mouse movement uses Analog joy-mouse speed instead");

        // Right: Mouse Untrap (Inline Label)
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mouse untrap:");
        ImGui::SameLine();

        // One bitmask behind two keys: the combo writes both the WinUAE
        // magic_mouse key and Amiberry's middle_mouse key.
        const char *untrap_items[] = {"None", "Middle button", "Magic mouse", "Both"};
        const bool untrap_middle = settings::get_bool(o.middle_mouse, changed_prefs);
        const bool untrap_magic = settings::get_bool(o.magic_mouse, changed_prefs);
        int untrap_idx = untrap_middle && untrap_magic ? 3 : untrap_magic ? 2
            : untrap_middle ? 1 : 0;

        ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
        if (ImGui::BeginCombo("##UntrapMode", untrap_items[untrap_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(untrap_items); n++) {
                const bool is_selected = (untrap_idx == n);
                if (is_selected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                if (ImGui::Selectable(untrap_items[n], is_selected)) {
                    untrap_idx = n;
                    settings::set_bool(o.middle_mouse, changed_prefs, ctx,
                        untrap_idx == 1 || untrap_idx == 3);
                    settings::set_bool(o.magic_mouse, changed_prefs, ctx,
                        untrap_idx == 2 || untrap_idx == 3);
                }
                if (is_selected) {
                    ImGui::PopStyleColor();
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("How to release the mouse from the emulator window (middle button, magic mouse, or both)");

        // Row 2
        ImGui::TableNextRow();

        // Left: Virtual Mouse
        ImGui::TableNextColumn();
		bool virt_mouse = changed_prefs.input_tablet > 0;
		if (AmigaCheckbox("Install virtual mouse driver", &virt_mouse)) {
			if (virt_mouse) {
				if (changed_prefs.input_tablet == TABLET_OFF)
                    settings::set(o.absolute_mouse, changed_prefs, ctx, "mousehack");
			} else {
                settings::set(o.absolute_mouse, changed_prefs, ctx, "none");
			}
		}
        ShowHelpMarker("Install a virtual mouse driver for tablet/touchscreen absolute positioning");

        // Right: Cursor Mode
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Magic Mouse cursor:");
        ImGui::SameLine();

        ImGui::BeginDisabled(!available(o.magic_mousecursor, ctx));
        {
            // The values are words (both, native, host): the current one is
            // matched as text.
            const amiberry_setting_descriptor& mm_desc = o.magic_mousecursor.descriptor;
            char current[AMIBERRY_SETTINGS_VALUE_MAX];
            settings::get(o.magic_mousecursor, changed_prefs, current, sizeof current);
            int cursor = -1;
            for (size_t n = 0; n < mm_desc.choice_count; n++) {
                if (!std::strcmp(current, mm_desc.choices[n].value))
                    cursor = static_cast<int>(n);
            }
            const char* preview = cursor >= 0 ? mm_desc.choices[cursor].label : current;
            if (ImGui::BeginCombo("##MagicMouse", preview)) {
                for (size_t n = 0; n < mm_desc.choice_count; n++) {
                    const bool is_selected = cursor == static_cast<int>(n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    ImGui::PushID(static_cast<int>(n));
                    if (ImGui::Selectable(mm_desc.choices[n].label, is_selected))
                        settings::set(o.magic_mousecursor, changed_prefs, ctx,
                            mm_desc.choices[n].value);
                    ImGui::PopID();
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
        ShowHelpMarker("Display Amiga cursor, host cursor, or both when using magic mouse mode");
        ImGui::EndDisabled();

        // Row 3
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::BeginDisabled(!available(o.tablet_library, ctx));
        option_checkbox("Tablet.library emulation", o.tablet_library, ctx);
        ShowHelpMarker("Emulate Amiga tablet.library for graphics tablet support");
        ImGui::EndDisabled();

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Tablet mode:");
        ImGui::SameLine();

		ImGui::BeginDisabled(changed_prefs.input_tablet == 0);
		{
            const amiberry_setting_descriptor& tb_desc = o.absolute_mouse.descriptor;
            char tb_current[AMIBERRY_SETTINGS_VALUE_MAX];
            settings::get(o.absolute_mouse, changed_prefs, tb_current, sizeof tb_current);
            const char* preview = tb_current;
            int selected = -1;
            for (size_t n = 0; n < tb_desc.choice_count; n++) {
                if (!strcmp(tb_current, tb_desc.choices[n].value)) {
                    selected = static_cast<int>(n);
                    preview = tb_desc.choices[n].label;
                }
            }
            if (ImGui::BeginCombo("##TabletMode", preview)) {
                for (size_t n = 0; n < tb_desc.choice_count; n++) {
                    const bool is_selected = selected == static_cast<int>(n);
                    if (is_selected)
                        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
                    ImGui::PushID(static_cast<int>(n));
                    if (ImGui::Selectable(tb_desc.choices[n].label, is_selected))
                        settings::set(o.absolute_mouse, changed_prefs, ctx, tb_desc.choices[n].value);
                    ImGui::PopID();
                    if (is_selected) {
                        ImGui::PopStyleColor();
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
		ShowHelpMarker("Tablet mode: MouseHack for mouse absolute positioning, Real Tablet for pressure-sensitive pen/tablet input via SDL3");
        ImGui::EndDisabled();

        ImGui::EndTable();
    }
    ImGui::Spacing();
    }
    EndGroupBox("Mouse extra settings");
}
