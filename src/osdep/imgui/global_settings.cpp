#include "imgui.h"
#include "sysdeps.h"

#include <algorithm>
#include <climits>
#include <functional>
#include <string>

#include "imgui_panels.h"
#include "options.h"
#include "inputdevice.h"
#include "registry.h"
#include "target.h"
#include "gui/gui_handling.h"
#include "shader_catalog.h"

namespace {

struct ComboOption
{
	int value;
	const char* label;
};

struct StringComboOption
{
	const char* value;
	const char* label;
};

static bool begin_settings_table(const char* id)
{
	if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp))
		return false;

	ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, BUTTON_WIDTH * 2.1f);
	ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
	return true;
}

static void next_setting_row(const char* label)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::TableSetColumnIndex(1);
	ImGui::PushID(label);
}

static void finish_setting_row(const char* help)
{
	if (help)
		ShowHelpMarker(help);
	ImGui::PopID();
}

static bool render_bool_row(const char* label, bool* value, const char* help)
{
	next_setting_row(label);
	const bool changed = AmigaCheckbox("##value", value);
	finish_setting_row(help);
	return changed;
}

static bool render_int_row(const char* label, int* value, const char* help,
	const int step = 1, const int min_value = INT_MIN, const int max_value = INT_MAX)
{
	next_setting_row(label);
	ImGui::SetNextItemWidth(-1.0f);
	const bool changed = ImGui::InputInt("##value", value, step, step * 10);
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	if (changed)
		*value = std::clamp(*value, min_value, max_value);
	finish_setting_row(help);
	return changed;
}

static bool render_string_row(const char* label, char* value, const size_t value_size, const char* help)
{
	next_setting_row(label);
	ImGui::SetNextItemWidth(-1.0f);
	const bool changed = AmigaInputText("##value", value, value_size);
	finish_setting_row(help);
	return changed;
}

static bool render_hotkey_row(const char* label, char* value, const size_t value_size, const char* help)
{
	next_setting_row(label);
	const bool changed = HotkeyPicker("##value", value, value_size);
	finish_setting_row(help);
	return changed;
}

static bool render_input_device_row(const char* label, char* value, const size_t value_size,
	const char* help)
{
	next_setting_row(label);
	const auto& options = get_input_device_options();
	int current_index = -1;
	for (int i = 0; i < static_cast<int>(options.size()); ++i) {
		if ((!value[0] && options[i].id == JPORT_NONE)
			|| stricmp(options[i].config_value.c_str(), value) == 0) {
			current_index = i;
			break;
		}
	}

	const char* preview = value[0] ? value : "<none>";
	if (current_index >= 0)
		preview = options[current_index].label.c_str();

	ImGui::SetNextItemWidth(-1.0f);
	int selected_index = current_index;
	const bool changed = InputDeviceCombo("##value", current_index, preview, &selected_index);
	if (changed) {
		const char* config_value = options[selected_index].id == JPORT_NONE
			? ""
			: options[selected_index].config_value.c_str();
		strncpy(value, config_value, value_size - 1);
		value[value_size - 1] = '\0';
	}
	finish_setting_row(help);
	return changed;
}

static bool render_string_combo_row(const char* label, char* value, const size_t value_size,
	const StringComboOption* options, const int option_count, const char* help)
{
	next_setting_row(label);
	int current_index = 0;
	for (int i = 0; i < option_count; ++i) {
		if (stricmp(options[i].value, value) == 0) {
			current_index = i;
			break;
		}
	}

	bool changed = false;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##value", options[current_index].label)) {
		for (int i = 0; i < option_count; ++i) {
			const bool selected = current_index == i;
			if (ImGui::Selectable(options[i].label, selected)) {
				strncpy(value, options[i].value, value_size - 1);
				value[value_size - 1] = '\0';
				current_index = i;
				changed = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	finish_setting_row(help);
	return changed;
}

static bool render_shader_combo_row(const char* label, char* value, const size_t value_size,
	const char* help)
{
	next_setting_row(label);
	const auto& shader_names = get_available_shader_names();
	const char* preview = value[0] ? value : "none";

	bool changed = false;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##value", preview)) {
		for (const auto& shader_name : shader_names) {
			const bool selected = strcmp(shader_name.c_str(), value) == 0;
			if (ImGui::Selectable(shader_name.c_str(), selected)) {
				strncpy(value, shader_name.c_str(), value_size - 1);
				value[value_size - 1] = '\0';
				changed = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	finish_setting_row(help);
	return changed;
}

static bool render_catalog_combo_row(const char* label, char* value, const size_t value_size,
	const std::vector<std::string>& options, const char* help)
{
	next_setting_row(label);
	const char* preview = value[0] ? value : (options.empty() ? "none" : options.front().c_str());

	bool changed = false;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##value", preview)) {
		for (const auto& option : options) {
			const bool selected = stricmp(option.c_str(), value) == 0;
			if (ImGui::Selectable(option.c_str(), selected)) {
				strncpy(value, option.c_str(), value_size - 1);
				value[value_size - 1] = '\0';
				changed = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	finish_setting_row(help);
	return changed;
}

static bool render_sound_device_row(const char* label, int* value, const char* help)
{
	next_setting_row(label);
	const auto& device_names = get_sound_device_names();
	const char* preview = "System default";
	if (*value > 0 && *value < static_cast<int>(device_names.size()))
		preview = device_names[*value].c_str();
	else if (*value > 0)
		preview = "Unknown device";

	bool changed = false;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##value", preview)) {
		if (ImGui::Selectable("System default", *value == 0)) {
			*value = 0;
			changed = true;
		}
		if (*value == 0)
			ImGui::SetItemDefaultFocus();

		// Index zero is represented by the system-default choice in amiberry.conf.
		for (int i = 1; i < static_cast<int>(device_names.size()); ++i) {
			const bool selected = *value == i;
			if (ImGui::Selectable(device_names[i].c_str(), selected)) {
				*value = i;
				changed = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	finish_setting_row(help);
	return changed;
}

static bool render_sound_buffer_row(const char* label, int* value, const char* help)
{
	next_setting_row(label);
	int buffer_index = get_sound_buffer_size_index(*value);
	const float value_width = BUTTON_WIDTH * 1.6f;
	ImGui::SetNextItemWidth(std::max(BUTTON_WIDTH, ImGui::GetContentRegionAvail().x - value_width));
	const bool changed = ImGui::SliderInt("##value", &buffer_index, 0,
		IM_ARRAYSIZE(SOUND_BUFFER_SIZES), "");
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
	if (changed)
		*value = buffer_index == 0 ? 0 : SOUND_BUFFER_SIZES[buffer_index - 1];
	ImGui::SameLine();
	ImGui::TextUnformatted(buffer_index == 0 ? "Min" : std::to_string(*value).c_str());
	finish_setting_row(help);
	return changed;
}

static bool render_combo_row(const char* label, int* value, const ComboOption* options,
	const int option_count, const char* help)
{
	next_setting_row(label);
	int current_index = 0;
	for (int i = 0; i < option_count; ++i) {
		if (options[i].value == *value) {
			current_index = i;
			break;
		}
	}

	bool changed = false;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##value", options[current_index].label)) {
		for (int i = 0; i < option_count; ++i) {
			const bool selected = current_index == i;
			if (ImGui::Selectable(options[i].label, selected)) {
				*value = options[i].value;
				current_index = i;
				changed = true;
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::IsItemActive());
	finish_setting_row(help);
	return changed;
}

static bool render_path_row(const char* label, const char* id, const std::string& value,
	const std::function<void(const std::string&)>& setter, const char* help,
	const bool select_file = false, const char* filter_name = "Choose File", const char* filter_ext = ".*")
{
	next_setting_row(label);

	const float browse_button_width = SMALL_BUTTON_WIDTH;
	const float input_width = std::max(BUTTON_WIDTH,
		ImGui::GetContentRegionAvail().x - browse_button_width - ImGui::GetStyle().ItemSpacing.x);

	char buffer[MAX_DPATH];
	strncpy(buffer, value.c_str(), sizeof buffer);
	buffer[sizeof buffer - 1] = '\0';

	bool changed = false;
	ImGui::SetNextItemWidth(input_width);
	if (AmigaInputText("##value", buffer, sizeof buffer)) {
		setter(buffer);
		changed = true;
	}
	if (help)
		ShowHelpMarker(help);

	ImGui::SameLine();
	std::string dialog_key = std::string("GLOBAL_SETTINGS_") + id + (select_file ? "_FILE" : "_DIR");
	if (AmigaButton("...", ImVec2(browse_button_width, 0.0f))) {
		if (select_file)
			OpenFileDialogKey(dialog_key.c_str(), filter_name, filter_ext, value);
		else
			OpenDirDialogKey(dialog_key.c_str(), value);
	}

	std::string selected_path;
	if (select_file) {
		if (ConsumeFileDialogResultKey(dialog_key.c_str(), selected_path)) {
			setter(selected_path);
			changed = true;
		}
	} else {
		if (ConsumeDirDialogResultKey(dialog_key.c_str(), selected_path)) {
			setter(selected_path);
			changed = true;
		}
	}

	ImGui::PopID();
	return changed;
}

static void render_group(const char* name, const char* table_id, const std::function<void()>& render_contents)
{
	if (BeginGroupBox(name, true, false)) {
		if (begin_settings_table(table_id)) {
			render_contents();
			ImGui::EndTable();
		}
	}
	EndGroupBox(name);
}

static void save_global_settings()
{
	const bool ini_ok =
		regsetint(nullptr, _T("KeySwapBackslashF11"), key_swap_hack) != 0 &&
		regsetint(nullptr, _T("KeyEndPageUp"), key_swap_end_pgup) != 0;
	const bool conf_ok = save_amiberry_settings_with_result();
	regclosetree(nullptr);

	if (conf_ok && ini_ok) {
		ShowMessageBox("Global Settings", "Global settings saved.");
	} else if (!conf_ok && !ini_ok) {
		ShowMessageBox("Global Settings",
			"Failed to save amiberry.conf and amiberry.ini.\n\nCheck file permissions in the settings directory.");
	} else if (!conf_ok) {
		ShowMessageBox("Global Settings",
			"Failed to save amiberry.conf.\n\nCheck file permissions in the settings directory.");
	} else {
		ShowMessageBox("Global Settings",
			"Failed to save amiberry.ini.\n\nCheck file permissions in the settings directory.");
	}
}

} // namespace

void render_panel_global_settings()
{
	ImGui::Indent(4.0f);

	if (AmigaButton(ICON_FA_FLOPPY_DISK " Save Global Settings", ImVec2(BUTTON_WIDTH * 2.0f, BUTTON_HEIGHT)))
		save_global_settings();
	ShowHelpMarker("Write these global defaults to amiberry.conf and the INI-backed settings to amiberry.ini.");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	static const ComboOption line_mode_options[] = {
		{0, "Single"},
		{1, "Double"},
		{2, "Scanlines"},
	};
	static const ComboOption scaling_options[] = {
		{-1, "Auto"},
		{0, "Nearest"},
		{1, "Linear"},
		{2, "Integer"},
	};
	static const ComboOption fullscreen_options[] = {
		{0, "Windowed"},
		{2, "Full-window"},
	};
	static const ComboOption enabled_options[] = {
		{0, "Disabled"},
		{1, "Enabled"},
	};
	static const ComboOption update_channel_options[] = {
		{0, "Stable"},
		{1, "Preview"},
	};
	static const ComboOption compatibility_options[] = {
		{0, "Best compatibility"},
		{1, "Good compatibility"},
		{2, "Less compatible"},
		{3, "Low compatibility"},
		{4, "Low compatibility (JIT)"},
	};
	static const ComboOption stereo_separation_options[] = {
		{10, "100%"},
		{9, "90%"},
		{8, "80%"},
		{7, "70%"},
		{6, "60%"},
		{5, "50%"},
		{4, "40%"},
		{3, "30%"},
		{2, "20%"},
		{1, "10%"},
		{0, "0%"},
	};
	static const ComboOption sound_frequency_options[] = {
		{11025, "11025"},
		{22050, "22050"},
		{32000, "32000"},
		{44100, "44100"},
		{48000, "48000"},
	};
	static const StringComboOption vkbd_language_options[] = {
		{"US", "US"},
		{"UK", "UK"},
		{"DE", "DE"},
		{"FR", "FR"},
	};

	render_group("Startup and GUI", "GlobalStartupGui", [&]() {
		render_bool_row("Quickstart on startup", &amiberry_options.quickstart_start,
			"Open the Quickstart panel by default when the GUI starts without a loaded configuration.");
		render_bool_row("Read config descriptions", &amiberry_options.read_config_descriptions,
			"Read descriptions from configuration files while building the Configurations panel list.");
		render_bool_row("GUI joystick control", &amiberry_options.gui_joystick_control,
			"Allow joystick/gamepad navigation in the GUI.");
		render_bool_row("Disable shutdown button", &amiberry_options.disable_shutdown_button,
			"Hide the GUI Shutdown button. Useful on systems where users should not power off the host from Amiberry.");
		render_combo_row("Quickstart compatibility", &amiberry_options.default_quickstart_compatibility,
			compatibility_options, IM_ARRAYSIZE(compatibility_options),
			"Default Quickstart compatibility level used when applying built-in model presets.");
	});

	render_group("Input defaults and hotkeys", "GlobalInputDefaults", [&]() {
		render_bool_row("RCtrl as RAmiga", &amiberry_options.rctrl_as_ramiga,
			"Map Right Control to the Right Amiga key by default.");
		render_int_row("Mouse speed", &amiberry_options.input_default_mouse_speed,
			"Default mouse input speed applied to new configurations.", 1, 1, 1000);
		render_bool_row("Keyboard joystick stops keypresses",
			&amiberry_options.input_keyboard_as_joystick_stop_keypresses,
			"Stop keyboard-as-joystick mappings from also generating normal Amiga key presses.");
		next_setting_row("Joystick deadzone");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderInt("##value", &amiberry_options.default_joystick_deadzone, 0, 100, "%d%%");
		AmigaBevel(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
		finish_setting_row("Default joystick and joystick-mouse deadzone percentage.");
		render_hotkey_row("Open GUI key", amiberry_options.default_open_gui_key,
			sizeof amiberry_options.default_open_gui_key,
			"Default keyboard shortcut for opening the GUI.");
		render_hotkey_row("Quit key", amiberry_options.default_quit_key,
			sizeof amiberry_options.default_quit_key,
			"Default keyboard shortcut for quitting Amiberry.");
		render_hotkey_row("Action Replay key", amiberry_options.default_ar_key,
			sizeof amiberry_options.default_ar_key,
			"Default keyboard shortcut for activating Action Replay/HRTmon.");
		render_hotkey_row("Full-window toggle key", amiberry_options.default_fullscreen_toggle_key,
			sizeof amiberry_options.default_fullscreen_toggle_key,
			"Default keyboard shortcut for toggling Windowed and Full-window modes.");
		render_bool_row("RetroArch quit", &amiberry_options.default_retroarch_quit,
			"Enable RetroArch-style quit button mapping by default when a mapping exists.");
		render_bool_row("RetroArch menu", &amiberry_options.default_retroarch_menu,
			"Enable RetroArch-style menu button mapping by default when a mapping exists.");
		render_bool_row("RetroArch reset", &amiberry_options.default_retroarch_reset,
			"Enable RetroArch-style reset button mapping by default when a mapping exists.");
		render_input_device_row("Controller 1", amiberry_options.default_controller1,
			sizeof amiberry_options.default_controller1,
			"Default controller mapping for port 1.");
		render_input_device_row("Controller 2", amiberry_options.default_controller2,
			sizeof amiberry_options.default_controller2,
			"Default controller mapping for port 2.");
		render_input_device_row("Controller 3", amiberry_options.default_controller3,
			sizeof amiberry_options.default_controller3,
			"Default controller mapping for port 3.");
		render_input_device_row("Controller 4", amiberry_options.default_controller4,
			sizeof amiberry_options.default_controller4,
			"Default controller mapping for port 4.");
		render_input_device_row("Mouse 1", amiberry_options.default_mouse1,
			sizeof amiberry_options.default_mouse1,
			"Default mouse mapping for mouse port 1.");
		render_input_device_row("Mouse 2", amiberry_options.default_mouse2,
			sizeof amiberry_options.default_mouse2,
			"Default mouse mapping for mouse port 2.");
	});

	render_group("Display defaults", "GlobalDisplayDefaults", [&]() {
		render_combo_row("Line mode", &amiberry_options.default_line_mode,
			line_mode_options, IM_ARRAYSIZE(line_mode_options),
			"Default native display line mode for new configurations.");
		render_bool_row("Horizontal centering", &amiberry_options.default_horizontal_centering,
			"Center the native display horizontally by default.");
		render_bool_row("Vertical centering", &amiberry_options.default_vertical_centering,
			"Center the native display vertically by default.");
		render_combo_row("Scaling method", &amiberry_options.default_scaling_method,
			scaling_options, IM_ARRAYSIZE(scaling_options),
			"Default image scaling method.");
		render_combo_row("Auto resolution", &amiberry_options.default_gfx_autoresolution,
			enabled_options, IM_ARRAYSIZE(enabled_options),
			"Enable native display auto-resolution switching by default.");
		render_bool_row("Frameskip", &amiberry_options.default_frameskip,
			"Enable default frameskip for slower hosts.");
		render_bool_row("Correct aspect ratio", &amiberry_options.default_correct_aspect_ratio,
			"Preserve the Amiga display aspect ratio by default.");
		render_bool_row("Auto crop", &amiberry_options.default_auto_crop,
			"Enable automatic border crop by default.");
		render_int_row("Default width", &amiberry_options.default_width,
			"Default native display window width.", 1, 1, 16384);
		render_int_row("Default height", &amiberry_options.default_height,
			"Default native display window height.", 1, 1, 16384);
		amiberry_options.default_fullscreen_mode = amiberry_normalize_gfx_fullscreen_mode(
			amiberry_options.default_fullscreen_mode);
		render_combo_row("Screen mode", &amiberry_options.default_fullscreen_mode,
			fullscreen_options, IM_ARRAYSIZE(fullscreen_options),
			"Default native and RTG screen mode.");
	});

	render_group("Sound defaults", "GlobalSoundDefaults", [&]() {
		render_combo_row("Stereo separation", &amiberry_options.default_stereo_separation,
			stereo_separation_options, IM_ARRAYSIZE(stereo_separation_options),
			"Default stereo separation between the left and right channels.");
		render_combo_row("Sound frequency", &amiberry_options.default_sound_frequency,
			sound_frequency_options, IM_ARRAYSIZE(sound_frequency_options),
			"Default audio output sample rate in Hz.");
		render_sound_buffer_row("Sound buffer", &amiberry_options.default_sound_buffer,
			"Default emulator audio buffer size. Smaller values reduce latency but may cause glitches on slower systems.");
		render_sound_device_row("Audio device", &amiberry_options.default_soundcard,
			"Default sound output device. Index zero uses the operating system default.");
	});

	render_group("WHDLoad defaults", "GlobalWHDLoadDefaults", [&]() {
		render_bool_row("Button wait", &amiberry_options.default_whd_buttonwait,
			"Wait for a button press before starting WHDLoad games by default.");
		render_bool_row("Show splash", &amiberry_options.default_whd_showsplash,
			"Show the WHDLoad splash screen by default.");
		render_int_row("Config delay", &amiberry_options.default_whd_configdelay,
			"Default delay in seconds before starting a WHDLoad title.", 1, 0, INT_MAX);
		render_bool_row("Write cache", &amiberry_options.default_whd_writecache,
			"Enable WHDLoad write cache by default.");
		render_bool_row("Quit on exit", &amiberry_options.default_whd_quit_on_exit,
			"Quit Amiberry when the WHDLoad title exits.");
		render_bool_row("Use JST instead of WHDLoad", &amiberry_options.use_jst_instead_of_whd,
			"Use JST instead of WHDLoad for auto-booted titles.");
		render_bool_row("Allow display settings from XML", &amiberry_options.allow_display_settings_from_xml,
			"Allow WHDLoad XML display settings to override global display defaults.");
	});

	render_group("Touch and on-screen controls", "GlobalTouchDefaults", [&]() {
		render_bool_row("On-screen joystick", &amiberry_options.default_onscreen_joystick,
			"Show virtual joystick controls by default on touchscreen devices.");
		render_bool_row("On-screen keyboard", &amiberry_options.default_vkbd_enabled,
			"Enable the on-screen Amiga keyboard by default.");
		render_int_row("On-screen keyboard transparency", &amiberry_options.default_vkbd_transparency,
			"Default on-screen keyboard transparency percentage.", 1, 0, 100);
		render_string_combo_row("On-screen keyboard layout", amiberry_options.default_vkbd_language,
			sizeof amiberry_options.default_vkbd_language, vkbd_language_options, IM_ARRAYSIZE(vkbd_language_options),
			"Default on-screen keyboard layout.");
		render_string_row("On-screen keyboard toggle", amiberry_options.default_vkbd_toggle,
			sizeof amiberry_options.default_vkbd_toggle,
			"Default controller button or key name used to toggle the on-screen keyboard.");
		render_bool_row("Use RetroArch vkbd button", &amiberry_options.default_retroarch_vkbd,
			"Use the RetroArch virtual keyboard button mapping by default when a mapping exists.");
	});

	render_group("Visual defaults", "GlobalVisualDefaults", [&]() {
		render_catalog_combo_row("GUI theme", amiberry_options.gui_theme,
			sizeof amiberry_options.gui_theme, get_available_theme_names(),
			"Theme file loaded by the ImGui interface.");
		render_shader_combo_row("Native shader", amiberry_options.shader,
			sizeof amiberry_options.shader,
			"Default shader preset for native chipset modes. Use none to disable.");
		render_shader_combo_row("RTG shader", amiberry_options.shader_rtg,
			sizeof amiberry_options.shader_rtg,
			"Default shader preset for RTG modes. Use none to disable.");
		const bool custom_bezel_active = amiberry_options.use_custom_bezel
			&& stricmp(amiberry_options.custom_bezel, "none") != 0;
		if (custom_bezel_active)
			amiberry_options.use_bezel = false;
		ImGui::BeginDisabled(custom_bezel_active);
		if (render_bool_row("Use bezel", &amiberry_options.use_bezel,
			"Show the built-in CRT bezel overlay.") && amiberry_options.use_bezel)
			amiberry_options.use_custom_bezel = false;
		ImGui::EndDisabled();

		ImGui::BeginDisabled(amiberry_options.use_bezel);
		if (render_catalog_combo_row("Custom bezel", amiberry_options.custom_bezel,
			sizeof amiberry_options.custom_bezel, get_available_bezel_names(),
			"Select a custom bezel overlay from the Bezels directory, or none to disable it.")) {
			amiberry_options.use_custom_bezel = stricmp(amiberry_options.custom_bezel, "none") != 0;
			if (amiberry_options.use_custom_bezel)
				amiberry_options.use_bezel = false;
		}
		ImGui::EndDisabled();
	});

	render_group("Paths and logging", "GlobalPathsLogging", [&]() {
		render_bool_row("Write logfile", &amiberry_options.write_logfile,
			"Write emulator log output to the configured logfile.");
		render_path_row("Base content", "BaseContent", get_base_content_path(),
			[](const std::string& path) { set_base_content_path(path); },
			"Optional root used to derive most standard content folders.");
		render_path_row("Configurations", "Configurations", get_configuration_path(),
			[](const std::string& path) { set_configuration_path(path); },
			"Folder containing user .uae configurations.");
		render_path_row("Controllers", "Controllers", get_controllers_path(),
			[](const std::string& path) { set_controllers_path(path); },
			"Folder containing SDL controller database files.");
		render_path_row("RetroArch config", "RetroArchConfig", get_retroarch_file(),
			[](const std::string& path) { set_retroarch_file(path); },
			"RetroArch configuration file used for compatible controller mappings.", true,
			"Choose RetroArch .cfg file", ".cfg");
		render_path_row("WHDBoot", "WHDBoot", get_whdbootpath(),
			[](const std::string& path) { set_whdbootpath(path); },
			"Folder containing WHDBooter support files.");
		render_path_row("WHDLoad archives", "WHDLoadArchives", get_whdload_arch_path(),
			[](const std::string& path) { set_whdload_arch_path(path); },
			"Default folder for WHDLoad archives.");
		render_path_row("Floppies", "Floppies", get_floppy_path(),
			[](const std::string& path) { set_floppy_path(path); },
			"Default folder for floppy disk images.");
		render_path_row("Hard drives", "HardDrives", get_harddrive_path(),
			[](const std::string& path) { set_harddrive_path(path); },
			"Default folder for hard drive files and directories.");
		render_path_row("CD-ROMs", "CDROMs", get_cdrom_path(),
			[](const std::string& path) { set_cdrom_path(path); },
			"Default folder for CD images.");
		render_path_row("Logfile", "Logfile", get_logfile_path(),
			[](const std::string& path) { set_logfile_path(path); },
			"Log file path used when logfile output is enabled.", true, "Choose Log File", ".log");
		render_path_row("System ROMs", "SystemROMs", get_rom_path(),
			[](const std::string& path) { set_rom_path(path); },
			"Folder containing Kickstart and other system ROM files.");
		render_path_row("RP9", "RP9", get_rp9_path(),
			[](const std::string& path) { set_rp9_path(path); },
			"Default folder for RP9 packages.");
		render_path_row("Floppy sounds", "FloppySounds", get_floppy_sounds_path(),
			[](const std::string& path) { set_floppy_sounds_path(path); },
			"Folder containing floppy drive sound samples.");
		render_path_row("Save data", "SaveData", get_saveimage_path(),
			[](const std::string& path) { set_saveimage_path(path); },
			"Default folder for save-data images.");
		render_path_row("Save states", "SaveStates", get_savestate_path(),
			[](const std::string& path) { set_savestate_path(path); },
			"Default folder for emulator save states.");
		render_path_row("Ripper", "Ripper", get_ripper_path(),
			[](const std::string& path) { set_ripper_path(path); },
			"Default output folder for ripper files.");
		render_path_row("Input recordings", "InputRecordings", get_inputrecordings_path(),
			[](const std::string& path) { set_inputrecordings_path(path); },
			"Default folder for input recordings.");
		render_path_row("Screenshots", "Screenshots", get_screenshot_path(),
			[](const std::string& path) { set_screenshot_path(path); },
			"Default folder for screenshots.");
		render_path_row("NVRAM", "NVRAM", get_nvram_path(),
			[](const std::string& path) { set_nvram_path(path); },
			"Default folder for NVRAM files.");
		render_path_row("Plugins", "Plugins", get_plugins_path(),
			[](const std::string& path) { set_plugins_path(path); },
			"Default folder for runtime plugins.");
		render_path_row("Videos", "Videos", get_video_path(),
			[](const std::string& path) { set_video_path(path); },
			"Default folder for video output files.");
		render_path_row("Themes", "Themes", get_themes_path(),
			[](const std::string& path) { set_themes_path(path); },
			"Folder containing GUI theme files.");
		render_path_row("Shaders", "Shaders", get_shaders_path(),
			[](const std::string& path) { set_shaders_path(path); },
			"Folder containing shader presets.");
		render_path_row("Bezels", "Bezels", get_bezels_path(),
			[](const std::string& path) { set_bezels_path(path); },
			"Folder containing bezel overlay assets.");
	});

	render_group("Updates", "GlobalUpdates", [&]() {
		render_combo_row("Startup update check", &amiberry_options.update_check,
			enabled_options, IM_ARRAYSIZE(enabled_options),
			"Check for Amiberry updates on startup when the installation type supports it.");
		render_combo_row("Update channel", &amiberry_options.update_channel,
			update_channel_options, IM_ARRAYSIZE(update_channel_options),
			"Stable checks only releases. Preview can include pre-release versions.");
		render_string_row("Skipped version", amiberry_options.skipped_version,
			sizeof amiberry_options.skipped_version,
			"Version string to skip during update checks. Clear this to stop skipping a version.");
	});

	render_group("Advanced / compatibility", "GlobalAdvancedCompatibility", [&]() {
		render_bool_row("Performance log", &amiberry_options.perf_log,
			"Enable performance logging for diagnostics.");
		render_bool_row("Slow host warning", &amiberry_options.slow_host_warning,
			"Show warnings when Amiberry detects a slow host configuration.");
		render_bool_row("Android ADPF", &amiberry_options.use_adpf,
			"Use Android Dynamic Performance Framework hints when available.");
		render_bool_row("Disable cycle exact by default", &amiberry_options.default_disable_cycle_exact,
			"Disable cycle-exact defaults for quickstart-style configurations.");
	});

	render_group("INI-backed UI/input behavior", "GlobalIniBacked", [&]() {
		bool swap_backslash_f11 = key_swap_hack != 0;
		if (render_bool_row("Swap Backslash and F11", &swap_backslash_f11,
			"Swap host F11 and backslash before Amiga-side mapping. Saved in amiberry.ini."))
			key_swap_hack = swap_backslash_f11 ? 1 : 0;

		bool swap_end_pageup = key_swap_end_pgup != 0;
		if (render_bool_row("Swap End and Page Up", &swap_end_pageup,
			"Swap host End and Page Up before they reach the Amiga. Saved in amiberry.ini."))
			key_swap_end_pgup = swap_end_pageup ? 1 : 0;
	});

	HotkeyCapture_RenderPopup();
}
