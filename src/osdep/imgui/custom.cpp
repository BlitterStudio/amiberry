#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "gui/gui_handling.h"
#include "amiberry_input.h"
#include "settings/settings_definitions.h"
#include <cstring>
#include <string>

#include "imgui_panels.h"

static char set_hotkey_buf[64] = { 0 };

// The panel edits changed_prefs' ports, applying each change as it is made.
static amiberry_settings_custom_controls_stage* custom_stage()
{
	static amiberry_settings_custom_controls_stage* stage = amiberry_settings::custom_controls_stage_new();
	return stage;
}

static void commit_custom_mapping_change(const int portnum, const didata* did)
{
	const auto& port = changed_prefs.jports[portnum];
	amiberry_set_port_joystick_custom_mapping(
		portnum, port.idc.name, port.idc.configname, did->mapping);
	inputdevice_updateconfig(nullptr, &changed_prefs);
}

static const char* use_label(const char* use)
{
	if (!std::strcmp(use, "hotkey")) return "Hotkey";
	if (!std::strcmp(use, "quit")) return "Quit";
	if (!std::strcmp(use, "menu")) return "Menu";
	if (!std::strcmp(use, "reset")) return "Reset";
	return "VKBD";
}

// One control's row: its event, "Default" (what the port's mode gives it)
// or "None"; disabled while the device lacks it or a hotkey function holds
// it.
static void render_custom_control(const size_t index)
{
	const amiberry_settings_custom_control* c = amiberry_settings::custom_control_at(index);
	if (!c)
		return;
	const bool hotkey = SelectedFunction == 1;
	char value[AMIBERRY_SETTINGS_VALUE_MAX] = "";
	amiberry_settings::custom_control_get(nullptr, changed_prefs, SelectedPort, hotkey, c->id, value, sizeof value);
	const char* use = amiberry_settings::custom_control_use(changed_prefs, SelectedPort, hotkey, c->id);
	const bool available = amiberry_settings::custom_control_available(changed_prefs, SelectedPort, hotkey, c->id);

	ImGui::AlignTextToFramePadding();
	ImGui::Text("%s:", c->label); ImGui::SameLine();
	ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
	ImGui::PushID(static_cast<int>(index));
	if (!available) ImGui::BeginDisabled();
	std::string shown = use ? std::string("In-Use (") + use_label(use) + ")"
		: !value[0] ? std::string("Default") : !std::strcmp(value, "none") ? std::string("None") : std::string(value);
	if (ImGui::BeginCombo("##control", shown.c_str())) {
		auto option = [&](const char* label, const char* event) {
			const bool selected = !std::strcmp(value, event);
			if (ImGui::Selectable(label, selected)) {
				auto* stage = custom_stage();
				if (amiberry_settings::custom_control_set(stage, changed_prefs, SelectedPort, hotkey, c->id, event)
					== AMIBERRY_SETTINGS_OK)
					amiberry_settings::custom_controls_commit(stage, changed_prefs);
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		};
		option("Default", "");
		option("None", "none");
		for (size_t i = 0; i < amiberry_settings::input_event_count(); i++) {
			const amiberry_setting_choice* event = amiberry_settings::input_event_at(i);
			ImGui::PushID(static_cast<int>(i));
			option(event->label, event->value);
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	if (!available) ImGui::EndDisabled();
	ImGui::PopID();
}

void render_panel_custom()
{
	ImGui::Indent(4.0f);

	// Ensure SelectedPort stays within valid range
	if (SelectedPort < 0) SelectedPort = 0;
	if (SelectedPort > 3) SelectedPort = 3;

	// ---------------------------------------------------------
	// Joystick Port Selection
	// ---------------------------------------------------------
	BeginGroupBox("Joystick Port");

	AmigaRadioButton("0: Mouse", &SelectedPort, 0); ImGui::SameLine();
	AmigaRadioButton("1: Joystick", &SelectedPort, 1); ImGui::SameLine();
	AmigaRadioButton("2: Parallel 1", &SelectedPort, 2); ImGui::SameLine();
	AmigaRadioButton("3: Parallel 2", &SelectedPort, 3);

	EndGroupBox("Joystick Port");

	// ---------------------------------------------------------
	// Device & Logic Setup
	// ---------------------------------------------------------
	const int host_joy_id = changed_prefs.jports[SelectedPort].id - JSEM_JOYS;
	if (host_joy_id < 0 || host_joy_id >= MAX_INPUT_DEVICES) {
		ImGui::Text("No valid joystick device selected on this port.");
		return;
	}
	didata* did = &di_joystick[host_joy_id];

	// ---------------------------------------------------------
	// Function Key / Hotkey
	// ---------------------------------------------------------
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Function Key:"); ImGui::SameLine();
	AmigaRadioButton("None", &SelectedFunction, 0); ImGui::SameLine();
	AmigaRadioButton("HotKey", &SelectedFunction, 1); ImGui::SameLine();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Set Hotkey:"); ImGui::SameLine();
	
	// Hotkey Display/Set
	if (did->mapping.hotkey_button) {
		// Basic display logic (simplified vs Guisan for now)
		const char* keyname = SDL_GetGamepadStringForButton((SDL_GamepadButton)did->mapping.hotkey_button);
		if (keyname) snprintf(set_hotkey_buf, sizeof(set_hotkey_buf), "%s", keyname);
		else snprintf(set_hotkey_buf, sizeof(set_hotkey_buf), "%d", did->mapping.hotkey_button);
	} else {
		set_hotkey_buf[0] = 0;
	}
	
	ImGui::BeginDisabled(); // Read-only text
	ImGui::SetNextItemWidth(BUTTON_WIDTH);
	ImGui::InputText("##HotkeyText", set_hotkey_buf, sizeof(set_hotkey_buf), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	ImGui::SameLine();

	if (did->mapping.is_retroarch) ImGui::BeginDisabled();
	if (AmigaButton("...")) {
		ImGui::OpenPopup("Set Hotkey");
	}
	ImGui::SameLine();
	if (AmigaButton("X")) {
		did->mapping.hotkey_button = SDL_GAMEPAD_BUTTON_INVALID;
		commit_custom_mapping_change(SelectedPort, did);
	}
	if (did->mapping.is_retroarch) ImGui::EndDisabled();
	
	if (ImGui::BeginPopupModal("Set Hotkey", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Press a button on your controller,\nor Esc to cancel...");
		
		// Check for cancellation
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}
		
		// Check for Controller Button Input
		if (did->controller) {
			for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i) {
				if (SDL_GetGamepadButton(did->controller, static_cast<SDL_GamepadButton>(i))) {
					did->mapping.hotkey_button = i;
					commit_custom_mapping_change(SelectedPort, did);
					ImGui::CloseCurrentPopup();
					break;
				}
			}
		} else if (did->joystick) {
			const int num_buttons = SDL_GetNumJoystickButtons(did->joystick);
			for (int i = 0; i < num_buttons; ++i) {
				if (SDL_GetJoystickButton(did->joystick, i)) {
					did->mapping.hotkey_button = i;
					commit_custom_mapping_change(SelectedPort, did);
					ImGui::CloseCurrentPopup();
					break;
				}
			}
		}

		// Also Allow clearing via button in popup?
		if (AmigaButton(ICON_FA_XMARK " Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		
		ImGui::EndPopup();
	}

	// ---------------------------------------------------------
	// Input Device Info
	// ---------------------------------------------------------
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Input Device:"); ImGui::SameLine();
	
	char device_info[256];
	if (did->mapping.is_retroarch) {
		snprintf(device_info, sizeof(device_info), "%s [R]", did->joystick_name.c_str());
	} else {
		snprintf(device_info, sizeof(device_info), "%s [N]", did->name.c_str());
	}
	
	ImGui::BeginDisabled();
	ImGui::SetNextItemWidth(-ImGui::GetStyle().ItemSpacing.x * 2);
	ImGui::InputText("##DeviceInfo", device_info, sizeof(device_info), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();

	ImGui::BeginChild("RemapArea", ImVec2(0, -40), true);

	// Buttons, then axes: the first half of each in the left column.
	const size_t buttons = SDL_GAMEPAD_BUTTON_COUNT;
	const size_t half_btns = SDL_GAMEPAD_BUTTON_COUNT / 2;
	const size_t half_axes = SDL_GAMEPAD_AXIS_COUNT / 2 + 1;

	if (ImGui::BeginTable("RemapTable", 2, ImGuiTableFlags_None)) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		for (size_t i = 0; i < half_btns; i++)
			render_custom_control(i);
		for (size_t i = 0; i < half_axes; i++)
			render_custom_control(buttons + i);

		ImGui::TableNextColumn();
		for (size_t i = half_btns; i < buttons; i++)
			render_custom_control(i);
		for (size_t i = half_axes; i < SDL_GAMEPAD_AXIS_COUNT; i++)
			render_custom_control(buttons + i);

		ImGui::EndTable();
	}
	ImGui::EndChild();

	// ---------------------------------------------------------
	// Save Button
	// ---------------------------------------------------------
	if (!did->mapping.is_retroarch) {
		if (AmigaButton(ICON_FA_FLOPPY_DISK " Save as default mapping", ImVec2(-1, 0))) {
			std::string controller_path = get_controllers_path();
			std::string controller_file = controller_path + did->name + ".controller";
			save_controller_mapping_to_file(did->mapping, controller_file);
		}
	} else {
		ImGui::TextDisabled("Save as default mapping (Disabled for RetroArch)");
	}
}

