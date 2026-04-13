#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "target.h"
#include <string>
#include <vector>
#include <SDL3/SDL.h>

#include "imgui_panels.h"
#include "gui/gui_handling.h"

// Map SDL gamepad button enum to its config string name
static const char* sdl_button_to_config_name(SDL_GamepadButton btn)
{
	switch (btn) {
	case SDL_GAMEPAD_BUTTON_SOUTH:          return "a";
	case SDL_GAMEPAD_BUTTON_EAST:           return "b";
	case SDL_GAMEPAD_BUTTON_WEST:           return "x";
	case SDL_GAMEPAD_BUTTON_NORTH:          return "y";
	case SDL_GAMEPAD_BUTTON_BACK:           return "back";
	case SDL_GAMEPAD_BUTTON_GUIDE:          return "guide";
	case SDL_GAMEPAD_BUTTON_START:          return "start";
	case SDL_GAMEPAD_BUTTON_LEFT_STICK:     return "leftstick";
	case SDL_GAMEPAD_BUTTON_RIGHT_STICK:    return "rightstick";
	case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:  return "leftshoulder";
	case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "rightshoulder";
	case SDL_GAMEPAD_BUTTON_DPAD_UP:        return "dpup";
	case SDL_GAMEPAD_BUTTON_DPAD_DOWN:      return "dpdown";
	case SDL_GAMEPAD_BUTTON_DPAD_LEFT:      return "dpleft";
	case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:     return "dpright";
	default: return nullptr;
	}
}

// Get a display name for a config string (SDL button name or keyboard key name)
static const char* display_name_for_toggle(const char* cfg)
{
	if (!cfg || !cfg[0]) return "(None)";
	// Check if it's an SDL button name
	if (strcmp(cfg, "a") == 0)              return "Gamepad A (South)";
	if (strcmp(cfg, "b") == 0)              return "Gamepad B (East)";
	if (strcmp(cfg, "x") == 0)              return "Gamepad X (West)";
	if (strcmp(cfg, "y") == 0)              return "Gamepad Y (North)";
	if (strcmp(cfg, "back") == 0)           return "Gamepad Back/Select";
	if (strcmp(cfg, "guide") == 0)          return "Gamepad Guide/Home";
	if (strcmp(cfg, "start") == 0)          return "Gamepad Start/Menu";
	if (strcmp(cfg, "leftstick") == 0)      return "Gamepad Left Stick";
	if (strcmp(cfg, "rightstick") == 0)      return "Gamepad Right Stick";
	if (strcmp(cfg, "leftshoulder") == 0)   return "Gamepad Left Shoulder";
	if (strcmp(cfg, "rightshoulder") == 0)   return "Gamepad Right Shoulder";
	if (strcmp(cfg, "dpup") == 0)           return "Gamepad D-Pad Up";
	if (strcmp(cfg, "dpdown") == 0)         return "Gamepad D-Pad Down";
	if (strcmp(cfg, "dpleft") == 0)         return "Gamepad D-Pad Left";
	if (strcmp(cfg, "dpright") == 0)        return "Gamepad D-Pad Right";
	return cfg; // keyboard key name or unknown — show as-is
}

// Hotkey Capture Popup
static bool open_hotkey_popup = false;

// Track which gamepad buttons were already held when the popup opened,
// so we don't immediately capture a button that was pressed to open it.
static bool popup_just_opened = false;
static bool gamepad_initial_state[SDL_GAMEPAD_BUTTON_COUNT] = {};

static void ShowHotkeyPopup()
{
	if (open_hotkey_popup) {
		ImGui::OpenPopup("Set On-Screen Keyboard Toggle");
		open_hotkey_popup = false;
		popup_just_opened = true;

		// Snapshot current gamepad button state across ALL gamepads to ignore held buttons
		memset(gamepad_initial_state, 0, sizeof(gamepad_initial_state));
		int gamepad_count = 0;
		SDL_JoystickID* ids = SDL_GetGamepads(&gamepad_count);
		if (ids) {
			for (int gi = 0; gi < gamepad_count; gi++) {
				SDL_Gamepad* gp = SDL_GetGamepadFromID(ids[gi]);
				if (!gp) continue;
				for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
					if (SDL_GetGamepadButton(gp, static_cast<SDL_GamepadButton>(b)))
						gamepad_initial_state[b] = true;
				}
				// Don't close — handle is owned by the input system
			}
			SDL_free(ids);
		}
	}

	if (ImGui::BeginPopupModal("Set On-Screen Keyboard Toggle", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Press a key on your keyboard or a button on your gamepad...");
		ImGui::Separator();

		bool captured = false;

		// Check keyboard keys via ImGui (gamepad nav won't interfere with keyboard)
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_GamepadStart; key++) {
			if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key))) {
				if (key == ImGuiKey_Escape) {
					ImGui::CloseCurrentPopup();
					captured = true;
					break;
				}
				const char* key_name = ImGui::GetKeyName(static_cast<ImGuiKey>(key));
				if (key_name && key_name[0]) {
					snprintf(changed_prefs.vkbd_toggle, sizeof(changed_prefs.vkbd_toggle), "%s", key_name);
					ImGui::CloseCurrentPopup();
					captured = true;
					break;
				}
			}
		}

		// Poll SDL gamepads directly (bypasses ImGui nav consumption).
		// Uses SDL_GetGamepadFromID to get already-opened handles — no open/close churn.
		if (!captured) {
			int gamepad_count = 0;
			SDL_JoystickID* ids = SDL_GetGamepads(&gamepad_count);
			if (ids) {
				for (int gi = 0; gi < gamepad_count && !captured; gi++) {
					SDL_Gamepad* gp = SDL_GetGamepadFromID(ids[gi]);
					if (!gp) continue;
					for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT && !captured; b++) {
						if (SDL_GetGamepadButton(gp, static_cast<SDL_GamepadButton>(b))) {
							if (popup_just_opened && gamepad_initial_state[b])
								continue;
							const char* name = sdl_button_to_config_name(static_cast<SDL_GamepadButton>(b));
							if (name) {
								snprintf(changed_prefs.vkbd_toggle, sizeof(changed_prefs.vkbd_toggle), "%s", name);
								ImGui::CloseCurrentPopup();
								captured = true;
							}
						}
					}
				}
				SDL_free(ids);
			}
		}

		// After the first frame, clear the "just opened" flag
		if (popup_just_opened && !captured)
			popup_just_opened = false;

		if (AmigaButton(ICON_FA_XMARK " Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void render_panel_virtual_keyboard()
{
	ImGui::Indent(4.0f);

	// On-screen touch joystick (D-pad + fire buttons overlay)
	bool osj_before = changed_prefs.onscreen_joystick;
	AmigaCheckbox("On-screen Joystick", &changed_prefs.onscreen_joystick);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show virtual D-pad and fire buttons for touchscreen control");
	}
	if (changed_prefs.onscreen_joystick != osj_before) {
		currprefs.onscreen_joystick = changed_prefs.onscreen_joystick;
		import_joysticks();
		inputdevice_config_change();
		joystick_refresh_needed = true;
	}

	ImGui::Separator();

	AmigaCheckbox("On-screen Keyboard enabled", &changed_prefs.vkbd_enabled);
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show Amiga-style on-screen keyboard overlay during emulation");
	}

	ImGui::BeginDisabled(!changed_prefs.vkbd_enabled);

	// Transparency slider
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Transparency:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
	int val = changed_prefs.vkbd_transparency;
	if (ImGui::SliderInt("##Transparency", &val, 0, 100, "%d%%")) {
		changed_prefs.vkbd_transparency = val;
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

	// Keyboard layout
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Layout:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
		int current_lang = 0;
		if (stricmp(changed_prefs.vkbd_language, "UK") == 0) current_lang = 1;
		else if (stricmp(changed_prefs.vkbd_language, "DE") == 0) current_lang = 2;
		else if (stricmp(changed_prefs.vkbd_language, "FR") == 0) current_lang = 3;
		const char* lang_names[] = { "US", "UK", "DE", "FR" };
		if (ImGui::Combo("##Layout", &current_lang, lang_names, 4)) {
			strncpy(changed_prefs.vkbd_language, lang_names[current_lang], sizeof(changed_prefs.vkbd_language) - 1);
		}
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
	}
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Toggle button:");
	ImGui::SameLine();

	ImGui::BeginDisabled(changed_prefs.use_retroarch_vkbd);

	ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
	const char* display = display_name_for_toggle(changed_prefs.vkbd_toggle);
	char buf[256];
	snprintf(buf, sizeof(buf), "%s", display);
	ImGui::InputText("##ToggleDisplay", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

	ImGui::SameLine();
	if (AmigaButton("Set")) {
		open_hotkey_popup = true;
	}
	ImGui::SameLine();
	if (AmigaButton("X")) {
		changed_prefs.vkbd_toggle[0] = '\0';
	}

	ImGui::EndDisabled(); // RetroArch disable

	AmigaCheckbox("Use RetroArch Vkbd button", &changed_prefs.use_retroarch_vkbd);

	ImGui::EndDisabled(); // Enabled disable

	ShowHotkeyPopup();
}
