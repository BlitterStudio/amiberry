#include "imgui.h"
#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "target.h"
#include "settings/settings_definitions.h"
#include <SDL3/SDL.h>

#include "imgui_panels.h"
#include "gui/gui_handling.h"

namespace settings = amiberry_settings;

namespace {

// The options this panel edits. The shared settings layer owns their
// availability rules and the dependent changes a new value implies.
struct onscreen_options {
    const settings::definition& onscreen_joystick = settings::require("amiberry.onscreen_joystick");
    const settings::definition& vkbd_enabled = settings::require("amiberry.vkbd_enabled");
    const settings::definition& vkbd_numpad = settings::require("amiberry.vkbd_numpad");
    const settings::definition& vkbd_touch_button = settings::require("amiberry.vkbd_touch_button");
    const settings::definition& vkbd_transparency = settings::require("amiberry.vkbd_transparency");
    const settings::definition& vkbd_language = settings::require("amiberry.vkbd_language");
    const settings::definition& vkbd_toggle = settings::require("amiberry.vkbd_toggle");
    const settings::definition& use_retroarch_vkbd = settings::require("amiberry.use_retroarch_vkbd");
};

const onscreen_options& options()
{
    static const onscreen_options o;
    return o;
}

bool available(const settings::definition& d, const settings::context& ctx)
{
    return settings::is_available(d, changed_prefs, ctx);
}

// Map SDL gamepad button enum to its config string name
static const char* sdl_button_to_config_name(SDL_GamepadButton btn)
{
	switch (btn) {
	case SDL_GAMEPAD_BUTTON_SOUTH: return "a";
	case SDL_GAMEPAD_BUTTON_EAST: return "b";
	case SDL_GAMEPAD_BUTTON_WEST: return "x";
	case SDL_GAMEPAD_BUTTON_NORTH: return "y";
	case SDL_GAMEPAD_BUTTON_BACK: return "back";
	case SDL_GAMEPAD_BUTTON_GUIDE: return "guide";
	case SDL_GAMEPAD_BUTTON_START: return "start";
	case SDL_GAMEPAD_BUTTON_LEFT_STICK: return "leftstick";
	case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return "rightstick";
	case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return "leftshoulder";
	case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return "rightshoulder";
	case SDL_GAMEPAD_BUTTON_DPAD_UP: return "dpup";
	case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return "dpdown";
	case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return "dpleft";
	case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return "dpright";
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
	if (strcmp(cfg, "rightstick") == 0)     return "Gamepad Right Stick";
	if (strcmp(cfg, "leftshoulder") == 0)   return "Gamepad Left Shoulder";
	if (strcmp(cfg, "rightshoulder") == 0)  return "Gamepad Right Shoulder";
	if (strcmp(cfg, "dpup") == 0)           return "Gamepad D-Pad Up";
	if (strcmp(cfg, "dpdown") == 0)         return "Gamepad D-Pad Down";
	if (strcmp(cfg, "dpleft") == 0)         return "Gamepad D-Pad Left";
	if (strcmp(cfg, "dpright") == 0)        return "Gamepad D-Pad Right";
	return cfg; // keyboard key name or unknown — show as-is
}

// Store a captured toggle key or button through the settings layer.
void set_toggle(const settings::definition& d, const settings::context& ctx, const char* value)
{
    settings::set(d, changed_prefs, ctx, value);
}

} // namespace

// Hotkey Capture Popup
static bool open_hotkey_popup = false;

// Track which gamepad buttons were already held when the popup opened,
// so we don't immediately capture a button that was pressed to open it.
static bool popup_just_opened = false;
static bool gamepad_initial_state[SDL_GAMEPAD_BUTTON_COUNT] = {};

static void ShowHotkeyPopup()
{
	const settings::context ctx{emulating != 0};

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
					set_toggle(options().vkbd_toggle, ctx, key_name);
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
								set_toggle(options().vkbd_toggle, ctx, name);
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

void render_panel_onscreen()
{
	ImGui::Indent(4.0f);

	const onscreen_options& o = options();
	const settings::context ctx{emulating != 0};

	// On-screen touch joystick (D-pad + fire buttons overlay). The overlay
	// registers its own input device from currprefs, so the change applies
	// at once, as the panel always did: the device can then be picked in
	// the Input panel without emulating a frame first.
	ImGui::BeginDisabled(!available(o.onscreen_joystick, ctx));
	bool osj = settings::get_bool(o.onscreen_joystick, changed_prefs);
	if (AmigaCheckbox("On-screen Joystick", &osj)
		&& settings::set_bool(o.onscreen_joystick, changed_prefs, ctx, osj) == AMIBERRY_SETTINGS_OK) {
		currprefs.onscreen_joystick = changed_prefs.onscreen_joystick;
		import_joysticks();
		inputdevice_config_change();
		joystick_refresh_needed = true;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show virtual D-pad and fire buttons for touchscreen control");
	}

	ImGui::Separator();

	bool vkbd = settings::get_bool(o.vkbd_enabled, changed_prefs);
	if (AmigaCheckbox("On-screen Keyboard enabled", &vkbd)) {
		settings::set_bool(o.vkbd_enabled, changed_prefs, ctx, vkbd);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show Amiga-style on-screen keyboard overlay during emulation");
	}

	ImGui::BeginDisabled(!available(o.vkbd_numpad, ctx));

	bool numpad = settings::get_bool(o.vkbd_numpad, changed_prefs);
	if (AmigaCheckbox("Include numeric keypad", &numpad)) {
		settings::set_bool(o.vkbd_numpad, changed_prefs, ctx, numpad);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Include the Amiga numeric keypad; keys will be smaller to fit the full keyboard");
	}

	ImGui::BeginDisabled(!available(o.vkbd_touch_button, ctx));
	bool touch_button = settings::get_bool(o.vkbd_touch_button, changed_prefs);
	if (AmigaCheckbox("Keyboard button on On-screen Joystick", &touch_button)) {
		settings::set_bool(o.vkbd_touch_button, changed_prefs, ctx, touch_button);
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Show the KB touch button next to the fire buttons; the toggle button below works either way");
	}

	// Transparency slider
	ImGui::BeginDisabled(!available(o.vkbd_transparency, ctx));
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Transparency:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
	int transparency = static_cast<int>(settings::get_number(o.vkbd_transparency, changed_prefs));
	if (ImGui::SliderInt("##Transparency", &transparency, 0, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
		settings::set_number(o.vkbd_transparency, changed_prefs, ctx, transparency);
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
	ImGui::EndDisabled();

	// Keyboard layout
	{
		ImGui::BeginDisabled(!available(o.vkbd_language, ctx));
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Layout:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
		const amiberry_setting_descriptor& desc = o.vkbd_language.descriptor;
		// An unknown language from a configuration stays visible; the
		// combo then shows it as-is until the user picks a known one.
		char current[AMIBERRY_SETTINGS_VALUE_MAX];
		settings::get(o.vkbd_language, changed_prefs, current, sizeof current);
		const char* preview = current;
		int current_lang = -1;
		for (size_t i = 0; i < desc.choice_count; i++) {
			if (!stricmp(current, desc.choices[i].value)) {
				current_lang = static_cast<int>(i);
				preview = desc.choices[i].label;
				break;
			}
		}
		if (ImGui::BeginCombo("##Layout", preview)) {
			for (size_t n = 0; n < desc.choice_count; n++) {
				const bool is_selected = current_lang == static_cast<int>(n);
				if (is_selected)
					ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]);
				ImGui::PushID(static_cast<int>(n));
				if (ImGui::Selectable(desc.choices[n].label, is_selected))
					settings::set(o.vkbd_language, changed_prefs, ctx, desc.choices[n].value);
				ImGui::PopID();
				if (is_selected) {
					ImGui::PopStyleColor();
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActivated());
		ImGui::EndDisabled();
	}

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Toggle button:");
	ImGui::SameLine();

	ImGui::BeginDisabled(!available(o.vkbd_toggle, ctx));

	ImGui::SetNextItemWidth(BUTTON_WIDTH * 2);
	char toggle[AMIBERRY_SETTINGS_VALUE_MAX];
	settings::get(o.vkbd_toggle, changed_prefs, toggle, sizeof toggle);
	const char* display = display_name_for_toggle(toggle);
	char buf[256];
	snprintf(buf, sizeof(buf), "%s", display);
	ImGui::InputText("##ToggleDisplay", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

	ImGui::SameLine();
	if (AmigaButton("Set")) {
		open_hotkey_popup = true;
	}
	ImGui::SameLine();
	if (AmigaButton("X")) {
		set_toggle(o.vkbd_toggle, ctx, "");
	}

	ImGui::EndDisabled(); // RetroArch disable

	bool retroarch = settings::get_bool(o.use_retroarch_vkbd, changed_prefs);
	if (AmigaCheckbox("Use RetroArch Vkbd button", &retroarch)) {
		settings::set_bool(o.use_retroarch_vkbd, changed_prefs, ctx, retroarch);
	}

	ImGui::EndDisabled(); // Enabled disable

	ShowHotkeyPopup();
}
