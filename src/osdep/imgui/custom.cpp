#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "inputdevice.h"
#include "gui/gui_handling.h"
#include "amiberry_input.h"
#include <vector>
#include <string>

#include "imgui_panels.h"

static std::vector<std::string> custom_event_items;
static bool initialized = false;
static char set_hotkey_buf[64] = { 0 };

static bool VectorCombo(const char* label, int* current_item, const std::vector<std::string>& items)
{
	bool changed = false;
	if (ImGui::BeginCombo(label, items[*current_item].c_str()))
	{
		for (int n = 0; n < items.size(); n++)
		{
			const bool is_selected = (*current_item == n);
			if (ImGui::Selectable(items[n].c_str(), is_selected))
			{
				*current_item = n;
				changed = true;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return changed;
}

static int get_mapped_event_index(int event_id)
{
	if (event_id <= 0) return 0;
	for (int i = 0; i < remap_event_list_size; ++i) {
		if (remap_event_list[i] == event_id) return i + 1; // +1 for "None"
	}
	return 0;
}

void render_panel_custom()
{
	if (!initialized) {
		custom_event_items.clear();
		custom_event_items.push_back("None");
		for (int idx = 0; idx < remap_event_list_size; idx++) {
			const auto* ie = inputdevice_get_eventinfo(remap_event_list[idx]);
			if (ie) custom_event_items.push_back(ie->name);
		}
		initialized = true;
	}

	// Ensure SelectedPort stays within valid range
	if (SelectedPort < 0) SelectedPort = 0;
	if (SelectedPort > 3) SelectedPort = 3;

	// ---------------------------------------------------------
	// Joystick Port Selection
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("JoystickPort", ImVec2(0, 60), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Joystick Port"); ImGui::EndMenuBar(); }

	ImGui::RadioButton("0: Mouse", &SelectedPort, 0); ImGui::SameLine();
	ImGui::RadioButton("1: Joystick", &SelectedPort, 1); ImGui::SameLine();
	ImGui::RadioButton("2: Parallel 1", &SelectedPort, 2); ImGui::SameLine();
	ImGui::RadioButton("3: Parallel 2", &SelectedPort, 3);

	ImGui::EndChild();
	ImGui::EndGroup();

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
	ImGui::RadioButton("None", &SelectedFunction, 0); ImGui::SameLine();
	ImGui::RadioButton("HotKey", &SelectedFunction, 1); ImGui::SameLine();

	ImGui::Text("Set Hotkey:"); ImGui::SameLine();
	
	// Hotkey Display/Set
	if (did->mapping.hotkey_button) {
		// Basic display logic (simplified vs Guisan for now)
		const char* keyname = SDL_GameControllerGetStringForButton((SDL_GameControllerButton)did->mapping.hotkey_button);
		if (keyname) snprintf(set_hotkey_buf, sizeof(set_hotkey_buf), "%s", keyname);
		else snprintf(set_hotkey_buf, sizeof(set_hotkey_buf), "%d", did->mapping.hotkey_button);
	} else {
		set_hotkey_buf[0] = 0;
	}
	
	ImGui::BeginDisabled(); // Read-only text
	ImGui::SetNextItemWidth(100);
	ImGui::InputText("##HotkeyText", set_hotkey_buf, sizeof(set_hotkey_buf), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	ImGui::SameLine();

	if (AmigaButton("...")) {
		ImGui::OpenPopup("Set Hotkey");
	}
	ImGui::SameLine();
	if (AmigaButton("X")) {
		did->mapping.hotkey_button = SDL_CONTROLLER_BUTTON_INVALID;
	}
	
	if (ImGui::BeginPopupModal("Set Hotkey", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Press a button on your controller,\nor Esc to cancel...");
		
		// Check for cancellation
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}
		
		// Check for Controller Button Input
		if (did->controller) {
			for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
				if (SDL_GameControllerGetButton(did->controller, (SDL_GameControllerButton)i)) {
					// Debounce: Wait for release? Or just assign. 
					// Ideally we assign and close.
					did->mapping.hotkey_button = i;
					ImGui::CloseCurrentPopup();
					break;
				}
			}
		} else if (did->joystick) {
			// Fallback for joystick API if controller not available? 
			// Guisan code uses SDL events. 
			// For now, let's assume Controller API is primary for mappings.
		}

		// Also Allow clearing via button in popup?
		if (AmigaButton("Cancel")) {
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
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##DeviceInfo", device_info, sizeof(device_info), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();

	// Helper for checking if a button is "In-Use"
	auto CheckInUse = [&](int button_idx, std::string& type_out) -> bool {
		if (did->mapping.button[button_idx] == did->mapping.hotkey_button) {
			type_out = "Hotkey"; return true;
		}
		if (SelectedFunction == 1) { // Only check these if custom function is selected? Guisan logic seems to imply SelectedFunction==1 for some.
		    // Actually Guisan checks: if button == quit_button && SelectedFunction == 1 && use_retroarch_quit
			if (did->mapping.button[button_idx] == did->mapping.quit_button && changed_prefs.use_retroarch_quit) {
				type_out = "Quit"; return true;
			}
			if (did->mapping.button[button_idx] == did->mapping.menu_button && changed_prefs.use_retroarch_menu) {
				type_out = "Menu"; return true;
			}
			if (did->mapping.button[button_idx] == did->mapping.reset_button && changed_prefs.use_retroarch_reset) {
				type_out = "Reset"; return true;
			}
		}
		// VKBD: if SelectedFunction == 1 && retroarch_vkbd OR SelectedFunction == 0 && !is_retroarch
		if (did->mapping.button[button_idx] == did->mapping.vkbd_button) {
			if ((SelectedFunction == 1 && changed_prefs.use_retroarch_vkbd) || (SelectedFunction == 0 && !did->mapping.is_retroarch)) {
				type_out = "VKBD"; return true;
			}
		}
		return false;
	};

	ImGui::BeginChild("RemapArea", ImVec2(0, -40), true);
	ImGui::Columns(2, "RemapCols", false);

	int half_btns = SDL_CONTROLLER_BUTTON_MAX / 2;
	int half_axes = SDL_CONTROLLER_AXIS_MAX / 2 + 1;

	// Loop for Buttons (First Half)
	for (int i = 0; i < half_btns; i++) {
		int* store_ptr = (SelectedFunction == 0) ? &did->mapping.amiberry_custom_none[i] : &did->mapping.amiberry_custom_hotkey[i];
		int display_val = *store_ptr;
		if (display_val <= 0 && !did->mapping.is_retroarch) display_val = default_button_mapping[i];
		
		int idx = get_mapped_event_index(display_val);
		
		// Unmapped Check
		bool is_mapped = did->mapping.button[i] > -1;
		
		// D-Pad override
		if (i >= SDL_CONTROLLER_BUTTON_DPAD_UP && i <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT && did->mapping.number_of_hats > 0)
			is_mapped = true;

		std::string in_use_type;
		bool in_use = CheckInUse(i, in_use_type);

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label_button_list[i].c_str()); ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		
		ImGui::PushID(i);
		if (!is_mapped || in_use) ImGui::BeginDisabled();
		
		if (in_use) {
			std::string label = "In-Use (" + in_use_type + ")";
			if (ImGui::BeginCombo("##btn", label.c_str())) ImGui::EndCombo(); // Just show label
		}
		else if (VectorCombo("##btn", &idx, custom_event_items)) {
			int new_evt = (idx == 0) ? -1 : remap_event_list[idx - 1];
			*store_ptr = new_evt;
			inputdevice_updateconfig(nullptr, &changed_prefs);
		}
		
		if (!is_mapped || in_use) ImGui::EndDisabled();
		ImGui::PopID();
	}
	
	// Loop for Axes (First Half)
	for (int i = 0; i < half_axes; i++) {
		int* store_ptr = (SelectedFunction == 0) ? &did->mapping.amiberry_custom_axis_none[i] : &did->mapping.amiberry_custom_axis_hotkey[i];
		int display_val = *store_ptr;
		if (display_val <= 0 && !did->mapping.is_retroarch) display_val = default_axis_mapping[i];

		int idx = get_mapped_event_index(display_val);
		bool is_mapped = did->mapping.axis[i] > -1;

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label_axis_list[i].c_str()); ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		
		ImGui::PushID(100 + i);
		if (!is_mapped) ImGui::BeginDisabled();
		if (VectorCombo("##axis", &idx, custom_event_items)) {
			int new_evt = (idx == 0) ? -1 : remap_event_list[idx - 1];
			*store_ptr = new_evt;
			inputdevice_updateconfig(nullptr, &changed_prefs);
		}
		if (!is_mapped) ImGui::EndDisabled();
		ImGui::PopID();
	}

	ImGui::NextColumn();

	// Loop for Buttons (Second Half)
	for (int i = half_btns; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
		int* store_ptr = (SelectedFunction == 0) ? &did->mapping.amiberry_custom_none[i] : &did->mapping.amiberry_custom_hotkey[i];
		int display_val = *store_ptr;
		if (display_val <= 0 && !did->mapping.is_retroarch) display_val = default_button_mapping[i];
		
		int idx = get_mapped_event_index(display_val);
		
		bool is_mapped = did->mapping.button[i] > -1;
		if (i >= SDL_CONTROLLER_BUTTON_DPAD_UP && i <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT && did->mapping.number_of_hats > 0)
			is_mapped = true;

		std::string in_use_type;
		bool in_use = CheckInUse(i, in_use_type);
		
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label_button_list[i].c_str()); ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		
		ImGui::PushID(i);
		if (!is_mapped || in_use) ImGui::BeginDisabled();

		if (in_use) {
			std::string label = "In-Use (" + in_use_type + ")";
			if (ImGui::BeginCombo("##btn", label.c_str())) ImGui::EndCombo();
		}
		else if (VectorCombo("##btn", &idx, custom_event_items)) {
			int new_evt = (idx == 0) ? -1 : remap_event_list[idx - 1];
			*store_ptr = new_evt;
			inputdevice_updateconfig(nullptr, &changed_prefs);
		}

		if (!is_mapped || in_use) ImGui::EndDisabled();
		ImGui::PopID();
	}
	
	// Loop for Axes (Second Half)
	for (int i = half_axes; i < SDL_CONTROLLER_AXIS_MAX; i++) {
		int* store_ptr = (SelectedFunction == 0) ? &did->mapping.amiberry_custom_axis_none[i] : &did->mapping.amiberry_custom_axis_hotkey[i];
		int display_val = *store_ptr;
		if (display_val <= 0 && !did->mapping.is_retroarch) display_val = default_axis_mapping[i];

		int idx = get_mapped_event_index(display_val);
		bool is_mapped = did->mapping.axis[i] > -1;

		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", label_axis_list[i].c_str()); ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		
		ImGui::PushID(100 + i);
		if (!is_mapped) ImGui::BeginDisabled();
		if (VectorCombo("##axis", &idx, custom_event_items)) {
			int new_evt = (idx == 0) ? -1 : remap_event_list[idx - 1];
			*store_ptr = new_evt;
			inputdevice_updateconfig(nullptr, &changed_prefs);
		}
		if (!is_mapped) ImGui::EndDisabled();
		ImGui::PopID();
	}
	
	ImGui::Columns(1);
	ImGui::EndChild();

	// ---------------------------------------------------------
	// Save Button
	// ---------------------------------------------------------
	if (!did->mapping.is_retroarch) {
		if (AmigaButton("Save as default mapping", ImVec2(-1, 0))) {
			std::string controller_path = get_controllers_path();
			std::string controller_file = controller_path + did->name + ".controller";
			save_controller_mapping_to_file(did->mapping, controller_file);
		}
	} else {
		ImGui::TextDisabled("Save as default mapping (Disabled for RetroArch)");
	}
}

