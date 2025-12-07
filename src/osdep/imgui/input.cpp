#include "imgui.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "inputdevice.h"
#include "gui/gui_handling.h"

void render_panel_input()
{
	ImGui::Text("Port 0:");
	ImGui::Combo("##Port0", &changed_prefs.jports[0].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port0Mode", &changed_prefs.jports[0].mode, "Default\0Wheel Mouse\0Mouse\0Joystick\0Gamepad\0Analog Joystick\0CDTV remote mouse\0CD32 pad\0");
	ImGui::Combo("##Port0Autofire", &changed_prefs.jports[0].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");
	ImGui::Combo("##Port0MouseMode", &changed_prefs.jports[0].mousemap, "None\0LStick\0");
	ImGui::Button("Remap");

	ImGui::Text("Port 1:");
	ImGui::Combo("##Port1", &changed_prefs.jports[1].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port1Mode", &changed_prefs.jports[1].mode, "Default\0Wheel Mouse\0Mouse\0Joystick\0Gamepad\0Analog Joystick\0CDTV remote mouse\0CD32 pad\0");
	ImGui::Combo("##Port1Autofire", &changed_prefs.jports[1].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");
	ImGui::Combo("##Port1MouseMode", &changed_prefs.jports[1].mousemap, "None\0LStick\0");
	ImGui::Button("Remap");

	ImGui::Button("Swap ports");
	ImGui::Checkbox("Mouse/Joystick autoswitching", &changed_prefs.input_autoswitch);

	ImGui::Text("Emulated Parallel Port joystick adapter");
	ImGui::Text("Port 2:");
	ImGui::Combo("##Port2", &changed_prefs.jports[2].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port2Autofire", &changed_prefs.jports[2].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");

	ImGui::Text("Port 3:");
	ImGui::Combo("##Port3", &changed_prefs.jports[3].id, "<none>\0Keyboard Layout A\0Keyboard Layout B\0Keyboard Layout C\0Keyrah Layout\0Retroarch KBD as Joystick Player1\0Retroarch KBD as Joystick Player2\0Retroarch KBD as Joystick Player3\0Retroarch KBD as Joystick Player4\0");
	ImGui::Combo("##Port3Autofire", &changed_prefs.jports[3].autofire, "No autofire (normal)\0Autofire\0Autofire (toggle)\0Autofire (always)\0No autofire (toggle)\0");

	const char* autofire_rate_items[] = { "Off", "Slow", "Medium", "Fast" };
	ImGui::Combo("Autofire Rate", &changed_prefs.input_autofire_linecnt, autofire_rate_items, IM_ARRAYSIZE(autofire_rate_items));

	ImGui::SliderInt("Digital joy-mouse speed", &changed_prefs.input_joymouse_speed, 2, 20);
	ImGui::SliderInt("Analog joy-mouse speed", &changed_prefs.input_joymouse_multiplier, 5, 150);
	ImGui::SliderInt("Mouse speed", &changed_prefs.input_mouse_speed, 5, 150);

	ImGui::Checkbox("Virtual mouse driver", (bool*)&changed_prefs.input_tablet);
	ImGui::Checkbox("Magic Mouse untrap", (bool*)&changed_prefs.input_mouse_untrap);

	ImGui::RadioButton("Both", &changed_prefs.input_magic_mouse_cursor, 0);
	ImGui::RadioButton("Native only", &changed_prefs.input_magic_mouse_cursor, 1);
	ImGui::RadioButton("Host only", &changed_prefs.input_magic_mouse_cursor, 2);

	ImGui::Checkbox("Swap Backslash/F11", (bool*)&key_swap_hack);
	ImGui::Checkbox("Page Up = End", (bool*)&key_swap_end_pgup);
}
