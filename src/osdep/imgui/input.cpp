#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "inputdevice.h"
#include "gui/gui_handling.h"
#include <vector>
#include <string>

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
static std::vector<const char*> input_device_items;
static std::vector<int> input_device_ids;

static void poll_input_devices()
{
	input_device_names.clear();
	input_device_items.clear();
	input_device_ids.clear();

	// Static Items
	auto add = [](const char* name, int id) {
		input_device_names.push_back(name);
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

	for (const auto& name : input_device_names) input_device_items.push_back(name.c_str());
}

static int get_device_index(int id)
{
	for (size_t i = 0; i < input_device_ids.size(); ++i) {
		if (input_device_ids[i] == id) return (int)i;
	}
	return 0; // Default to <none>
}

void render_panel_input()
{
	if (input_device_names.empty()) poll_input_devices();

	const char* autofire_items = "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0";
	const char* mode_items = "Default\0Wheel Mouse\0Mouse\0Joystick\0Gamepad\0Analog Joystick\0CDTV remote mouse\0CD32 pad\0";

	// ---------------------------------------------------------
	// Mouse and Joystick settings
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("GamePorts", ImVec2(0, 230), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Mouse and Joystick settings"); ImGui::EndMenuBar(); }

	ImGui::Columns(2, "PortsCols", false); 
	ImGui::SetColumnWidth(0, 60);

	auto render_port = [&](int port_idx, const char* label) {
		ImGui::Text("%s", label); ImGui::NextColumn();
		
		int dev_idx = get_device_index(changed_prefs.jports[port_idx].id);
		ImGui::SetNextItemWidth(-1);
		if (ImGui::Combo(std::string("##Dev").append(std::to_string(port_idx)).c_str(), &dev_idx, input_device_items.data(), input_device_items.size())) {
			changed_prefs.jports[port_idx].id = input_device_ids[dev_idx];
			inputdevice_validate_jports(&changed_prefs, port_idx, nullptr); // Validate change
		}
		
		float avail_w = ImGui::GetContentRegionAvail().x;
		float item_w = (avail_w - 120) / 2;
		ImGui::SetNextItemWidth(item_w);
		ImGui::Combo(std::string("##Autofire").append(std::to_string(port_idx)).c_str(), &changed_prefs.jports[port_idx].autofire, autofire_items);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(item_w);
		ImGui::Combo(std::string("##Mode").append(std::to_string(port_idx)).c_str(), &changed_prefs.jports[port_idx].mode, mode_items);
		ImGui::SameLine();
		ImGui::Button(std::string("Remap / Test##").append(std::to_string(port_idx)).c_str());
		
		// Row 3: Mouse Map
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Mouse map:"); ImGui::SameLine();
		ImGui::SetNextItemWidth(item_w);
		
		ImGui::BeginDisabled(changed_prefs.jports[port_idx].mode == 0);
		ImGui::Combo(std::string("##MouseMap").append(std::to_string(port_idx)).c_str(), &changed_prefs.jports[port_idx].mousemap, "None\0LStick\0");
		ImGui::EndDisabled();

		ImGui::NextColumn();
	};

	render_port(0, "Port 1:");
	ImGui::Separator();
	render_port(1, "Port 2:");
	
	// Swap & Autoswitch in Column 2
	ImGui::NextColumn(); // Skip Col 1
	ImGui::Dummy(ImVec2(0, 8.0f));
	ImGui::Button("Swap ports");
	ImGui::SameLine();
	ImGui::Checkbox("Mouse/Joystick autoswitching", &changed_prefs.input_autoswitch);
	ImGui::NextColumn(); // Finish Row
	
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::EndGroup();

	// ---------------------------------------------------------
	// Emulated Parallel Port
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("ParallelPorts", ImVec2(0, 160), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Emulated parallel port joystick adapter"); ImGui::EndMenuBar(); }
	
	ImGui::Columns(2, "ParCols", false); 
	ImGui::SetColumnWidth(0, 60);

	auto render_par_port = [&](int port_idx, const char* label) {
		ImGui::Text("%s", label); ImGui::NextColumn();
		
		int dev_idx = get_device_index(changed_prefs.jports[port_idx].id);
		ImGui::SetNextItemWidth(-1);
		if (ImGui::Combo(std::string("##ParDev").append(std::to_string(port_idx)).c_str(), &dev_idx, input_device_items.data(), input_device_items.size())) {
			changed_prefs.jports[port_idx].id = input_device_ids[dev_idx];
		}
		
		// Row 2: Autofire | Remap
		// Calculate width to match GamePorts: (avail - 120) / 2
		float avail_w = ImGui::GetContentRegionAvail().x;
		float item_w = (avail_w - 120) / 2; 
		
		ImGui::SetNextItemWidth(item_w);
		ImGui::Combo(std::string("##ParAutofire").append(std::to_string(port_idx)).c_str(), &changed_prefs.jports[port_idx].autofire, autofire_items);
		ImGui::SameLine();
		ImGui::Button(std::string("Remap / Test##Par").append(std::to_string(port_idx)).c_str());
		
		ImGui::NextColumn();
	};

	render_par_port(2, "Port 3:");
	ImGui::Separator();
	render_par_port(3, "Port 4:");

	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::EndGroup();

	// ---------------------------------------------------------
	// Game Controller Settings
	// ---------------------------------------------------------
	static const int digital_joymousespeed_values[] = { 2, 5, 10, 15, 20 };
	static const int analog_joymousespeed_values[] = { 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 75, 100, 125, 150 };

	ImGui::BeginGroup();
	ImGui::BeginChild("ControllerSettings", ImVec2(0, 110), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Game Controller Settings"); ImGui::EndMenuBar(); }

	ImGui::Columns(2, "ControllerCols", false);

	// Joystick Dead Zone
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Joystick dead zone:"); ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetColumnOffset() + 190);
	ImGui::SetNextItemWidth(120);
	ImGui::SliderInt("##DeadZone", &changed_prefs.input_joystick_deadzone, 0, 100, "%d%%");
	ImGui::NextColumn();

	// Autofire Rate
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Autofire Rate:"); ImGui::SameLine();
	int af_rate_idx = 0;
	if (changed_prefs.input_autofire_linecnt == 0) af_rate_idx = 0;
	else if (changed_prefs.input_autofire_linecnt > 10 * 312) af_rate_idx = 1;
	else if (changed_prefs.input_autofire_linecnt > 6 * 312) af_rate_idx = 2;
	else af_rate_idx = 3;
	
	ImGui::SetCursorPosX(ImGui::GetColumnOffset() + 190);
	ImGui::SetNextItemWidth(100);
	if (ImGui::Combo("##AutofireRate", &af_rate_idx, "Off\0Slow\0Medium\0Fast\0")) {
		if (af_rate_idx == 0) changed_prefs.input_autofire_linecnt = 0;
		else if (af_rate_idx == 1) changed_prefs.input_autofire_linecnt = 12 * 312;
		else if (af_rate_idx == 2) changed_prefs.input_autofire_linecnt = 8 * 312;
		else changed_prefs.input_autofire_linecnt = 4 * 312;
	}
	ImGui::NextColumn();

	// Digital Joy-Mouse Speed
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Digital joy-mouse speed:"); ImGui::SameLine();
	int dig_speed_idx = 2; // Default 10
	for (int i = 0; i < IM_ARRAYSIZE(digital_joymousespeed_values); ++i) {
		if (changed_prefs.input_joymouse_speed == digital_joymousespeed_values[i]) {
			dig_speed_idx = i; break;
		}
	}
	ImGui::SetCursorPosX(ImGui::GetColumnOffset() + 190);
	ImGui::SetNextItemWidth(120);
	if (ImGui::SliderInt("##DigSpeed", &dig_speed_idx, 0, IM_ARRAYSIZE(digital_joymousespeed_values) - 1, "")) {
		changed_prefs.input_joymouse_speed = digital_joymousespeed_values[dig_speed_idx];
	}
	ImGui::SameLine(); ImGui::Text("%d", changed_prefs.input_joymouse_speed);
	ImGui::NextColumn();
	
	// Analog Joy-Mouse Speed
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Analog joy-mouse speed:"); ImGui::SameLine();
	int ana_speed_idx = 1; // Default 10
	for (int i = 0; i < IM_ARRAYSIZE(analog_joymousespeed_values); ++i) {
		if (changed_prefs.input_joymouse_multiplier == analog_joymousespeed_values[i]) {
			ana_speed_idx = i; break;
		}
	}
	ImGui::SetCursorPosX(ImGui::GetColumnOffset() + 190);
	ImGui::SetNextItemWidth(120);
	if (ImGui::SliderInt("##AnaSpeed", &ana_speed_idx, 0, IM_ARRAYSIZE(analog_joymousespeed_values) - 1, "")) {
		changed_prefs.input_joymouse_multiplier = analog_joymousespeed_values[ana_speed_idx];
	}
	ImGui::SameLine(); ImGui::Text("%d", changed_prefs.input_joymouse_multiplier);
	ImGui::NextColumn();

	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::EndGroup();

	// ---------------------------------------------------------
	// Mouse Extra Settings
	// ---------------------------------------------------------
	ImGui::BeginGroup();
	ImGui::BeginChild("MouseSettings", ImVec2(0, 130), true, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar()) { ImGui::Text("Mouse extra settings"); ImGui::EndMenuBar(); }

	ImGui::Columns(2, "MouseExtras", false);
	
	// Row 1
	// Left: Mouse Speed
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Mouse speed:"); ImGui::SameLine(); 
	ImGui::SetNextItemWidth(100);
	ImGui::InputInt("##MouseSpd", &changed_prefs.input_mouse_speed, 1, 10);
	ImGui::NextColumn();

	// Right: Mouse Untrap (Inline Label)
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Mouse untrap:"); ImGui::SameLine();
	
	const char* untrap_items[] = { "None", "Middle button", "Magic mouse", "Both" };
	int untrap_val = changed_prefs.input_mouse_untrap;
	int untrap_idx = 0;
	if ((untrap_val & MOUSEUNTRAP_MIDDLEBUTTON) && (untrap_val & MOUSEUNTRAP_MAGIC)) untrap_idx = 3;
	else if (untrap_val & MOUSEUNTRAP_MAGIC) untrap_idx = 2;
	else if (untrap_val & MOUSEUNTRAP_MIDDLEBUTTON) untrap_idx = 1;
	
	ImGui::SetNextItemWidth(-1);
	if (ImGui::Combo("##UntrapMode", &untrap_idx, untrap_items, IM_ARRAYSIZE(untrap_items))) {
		int new_val = 0;
		if (untrap_idx == 1) new_val = MOUSEUNTRAP_MIDDLEBUTTON;
		else if (untrap_idx == 2) new_val = MOUSEUNTRAP_MAGIC;
		else if (untrap_idx == 3) new_val = MOUSEUNTRAP_MIDDLEBUTTON | MOUSEUNTRAP_MAGIC;
		changed_prefs.input_mouse_untrap = new_val;
	}
	ImGui::NextColumn();

	// Row 2
	// Left: Virtual Mouse
	bool virt_mouse = changed_prefs.input_tablet > 0;
	if (ImGui::Checkbox("Install virtual mouse driver", &virt_mouse)) {
		changed_prefs.input_tablet = virt_mouse ? TABLET_MOUSEHACK : TABLET_OFF; 
	}
	ImGui::NextColumn();

	// Right: Cursor Mode
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Magic Mouse cursor:"); ImGui::SameLine();
	const char* magic_items[] = { "Both", "Native only", "Host only" };
	ImGui::SetNextItemWidth(-1);
	
	ImGui::BeginDisabled(changed_prefs.input_tablet == 0);
	ImGui::Combo("##MagicMouse", &changed_prefs.input_magic_mouse_cursor, magic_items, IM_ARRAYSIZE(magic_items));
	ImGui::EndDisabled();
	ImGui::NextColumn();

	// Row 3
	// Left: Tablet Library emul
	ImGui::Checkbox("Tablet.library emulation", &changed_prefs.tablet_library);
	ImGui::NextColumn();

	// Right: Tablet Mode (Aligned with Row 3)
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Tablet mode:"); ImGui::SameLine();
	
	int tablet_mode = changed_prefs.input_tablet == TABLET_MOUSEHACK ? 1 : 0;
	ImGui::SetNextItemWidth(-1);
	if (ImGui::Combo("##TabletMode", &tablet_mode, "Disabled\0MouseHack\0")) {
		changed_prefs.input_tablet = tablet_mode == 1 ? TABLET_MOUSEHACK : TABLET_OFF;
	}
	ImGui::NextColumn();
	
	ImGui::Columns(1);
	ImGui::EndChild();
	ImGui::EndGroup();
}


