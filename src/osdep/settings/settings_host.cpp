// Host settings: the Global Settings panel. These live in amiberry_options
// and the path variables, saved to amiberry.conf, not in a configuration.
// They are process-wide, so there is no session: a set changes the live
// value, and amiberry_host_settings_save() writes the file.
//
// An ID is the amiberry.conf key. A choice's value is the integer the file
// stores. A bool is "true" or "false" (the file spells them yes and no).

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "target.h"
#include "gui_layout_scale.h"
#include "amiberry_gfx_mode.h"

#include "settings_definitions.h"

#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace amiberry_settings {

namespace {

struct choice { const char* value; const char* label; };

const choice compatibility_choices[] = {
	{"0", "Best compatibility"}, {"1", "Good compatibility"}, {"2", "Less compatible"},
	{"3", "Low compatibility"}, {"4", "Low compatibility (JIT)"},
};
const choice line_mode_choices[] = {{"0", "Single"}, {"1", "Double"}, {"2", "Scanlines"}};
const choice scaling_choices[] = {
	{"-1", "Auto"}, {"0", "Nearest"}, {"1", "Linear"}, {"2", "Integer"}, {"3", "Stretch"},
};
const choice enabled_choices[] = {{"0", "Disabled"}, {"1", "Enabled"}};
const choice fullscreen_choices[] = {{"0", "Windowed"}, {"2", "Full-window"}};
const choice stereo_choices[] = {
	{"10", "100%"}, {"9", "90%"}, {"8", "80%"}, {"7", "70%"}, {"6", "60%"},
	{"5", "50%"}, {"4", "40%"}, {"3", "30%"}, {"2", "20%"}, {"1", "10%"}, {"0", "0%"},
};
const choice frequency_choices[] = {
	{"11025", "11025"}, {"22050", "22050"}, {"32000", "32000"}, {"44100", "44100"}, {"48000", "48000"},
};
const choice buffer_choices[] = {
	{"0", "Min"}, {"1024", "1024"}, {"2048", "2048"}, {"3072", "3072"}, {"4096", "4096"},
	{"6144", "6144"}, {"8192", "8192"}, {"12288", "12288"}, {"16384", "16384"},
	{"32768", "32768"}, {"65536", "65536"},
};
const choice language_choices[] = {{"US", "US"}, {"UK", "UK"}, {"DE", "DE"}, {"FR", "FR"}};
const choice channel_choices[] = {{"0", "Stable"}, {"1", "Preview"}};

enum class kind { boolean, integer, choice_list, text, real, path };

struct host_setting {
	const char* id;
	const char* group;
	const char* label;
	const char* help;
	kind k;
	const char* default_value;
	double min = 0, max = 0;
	const choice* choices = nullptr;
	size_t choice_count = 0;
	bool amiberry_options::* boolean = nullptr;
	int amiberry_options::* integer = nullptr;
	float amiberry_options::* real = nullptr;
	char* text = nullptr;
	size_t text_size = 0;
	std::string (*path_get)() = nullptr;
	void (*path_set)(const std::string&) = nullptr;
	bool normalize_fullscreen = false;
	bool clamp_scale = false;
};

// The text fields, by address, so a pointer-to-member of differing array
// sizes is not needed.
char* text_of(const char* id, size_t& size)
{
	struct field { const char* id; char* data; size_t size; };
	const field fields[] = {
		{"default_open_gui_key", amiberry_options.default_open_gui_key, sizeof amiberry_options.default_open_gui_key},
		{"default_quit_key", amiberry_options.default_quit_key, sizeof amiberry_options.default_quit_key},
		{"default_ar_key", amiberry_options.default_ar_key, sizeof amiberry_options.default_ar_key},
		{"default_fullscreen_toggle_key", amiberry_options.default_fullscreen_toggle_key, sizeof amiberry_options.default_fullscreen_toggle_key},
		{"default_controller1", amiberry_options.default_controller1, sizeof amiberry_options.default_controller1},
		{"default_controller2", amiberry_options.default_controller2, sizeof amiberry_options.default_controller2},
		{"default_controller3", amiberry_options.default_controller3, sizeof amiberry_options.default_controller3},
		{"default_controller4", amiberry_options.default_controller4, sizeof amiberry_options.default_controller4},
		{"default_mouse1", amiberry_options.default_mouse1, sizeof amiberry_options.default_mouse1},
		{"default_mouse2", amiberry_options.default_mouse2, sizeof amiberry_options.default_mouse2},
		{"default_vkbd_language", amiberry_options.default_vkbd_language, sizeof amiberry_options.default_vkbd_language},
		{"default_vkbd_toggle", amiberry_options.default_vkbd_toggle, sizeof amiberry_options.default_vkbd_toggle},
		{"gui_theme", amiberry_options.gui_theme, sizeof amiberry_options.gui_theme},
		{"shader", amiberry_options.shader, sizeof amiberry_options.shader},
		{"shader_rtg", amiberry_options.shader_rtg, sizeof amiberry_options.shader_rtg},
		{"custom_bezel", amiberry_options.custom_bezel, sizeof amiberry_options.custom_bezel},
		{"skipped_version", amiberry_options.skipped_version, sizeof amiberry_options.skipped_version},
	};
	for (const field& f : fields) {
		if (std::strcmp(f.id, id) == 0) {
			size = f.size;
			return f.data;
		}
	}
	size = 0;
	return nullptr;
}

host_setting boolean(const char* id, const char* group, const char* label, const char* help,
	const char* def, bool amiberry_options::* field)
{
	host_setting s{};
	s.id = id; s.group = group; s.label = label; s.help = help; s.k = kind::boolean;
	s.default_value = def; s.boolean = field;
	return s;
}

host_setting integer(const char* id, const char* group, const char* label, const char* help,
	const char* def, int min, int max, int amiberry_options::* field)
{
	host_setting s{};
	s.id = id; s.group = group; s.label = label; s.help = help; s.k = kind::integer;
	s.default_value = def; s.min = min; s.max = max; s.integer = field;
	return s;
}

host_setting choices_of(const char* id, const char* group, const char* label, const char* help,
	const char* def, const choice* items, size_t count, int amiberry_options::* field)
{
	host_setting s{};
	s.id = id; s.group = group; s.label = label; s.help = help; s.k = kind::choice_list;
	s.default_value = def; s.choices = items; s.choice_count = count; s.integer = field;
	return s;
}

host_setting text(const char* id, const char* group, const char* label, const char* help, const char* def)
{
	host_setting s{};
	s.id = id; s.group = group; s.label = label; s.help = help; s.k = kind::text;
	s.default_value = def;
	return s;
}

host_setting language(const char* id, const char* group, const char* label, const char* help, const char* def)
{
	host_setting s = text(id, group, label, help, def);
	s.k = kind::choice_list;
	s.choices = language_choices;
	s.choice_count = sizeof language_choices / sizeof language_choices[0];
	return s;
}

host_setting path(const char* id, const char* label, const char* help,
	std::string (*get)(), void (*set)(const std::string&))
{
	host_setting s{};
	s.id = id; s.group = "global_paths"; s.label = label; s.help = help; s.k = kind::path;
	s.default_value = ""; s.path_get = get; s.path_set = set;
	return s;
}

const std::vector<host_setting>& settings()
{
	static const std::vector<host_setting> list = {
		boolean("Quickstart", "global_startup", "Quickstart on startup",
			"Open the Quickstart panel by default when the GUI starts without a loaded configuration.",
			"true", &amiberry_options::quickstart_start),
		boolean("read_config_descriptions", "global_startup", "Read config metadata",
			"Read descriptions and categories while building the configurations list.",
			"true", &amiberry_options::read_config_descriptions),
		boolean("gui_joystick_control", "global_startup", "GUI joystick control",
			"Allow joystick and gamepad navigation in the GUI.",
			"true", &amiberry_options::gui_joystick_control),
		boolean("disable_shutdown_button", "global_startup", "Disable shutdown button",
			"Hide the GUI shutdown button.",
			"true", &amiberry_options::disable_shutdown_button),
		choices_of("default_quickstart_compatibility", "global_startup", "Quickstart compatibility",
			"Default Quickstart compatibility level for built-in model presets.",
			"0", compatibility_choices, sizeof compatibility_choices / sizeof compatibility_choices[0],
			&amiberry_options::default_quickstart_compatibility),

		boolean("rctrl_as_ramiga", "global_input", "RCtrl as RAmiga",
			"Map Right Control to the Right Amiga key by default.",
			"false", &amiberry_options::rctrl_as_ramiga),
		integer("input_default_mouse_speed", "global_input", "Default mouse speed",
			"Mouse speed applied to new configurations (100 = normal).",
			"100", 1, 1000, &amiberry_options::input_default_mouse_speed),
		boolean("input_keyboard_as_joystick_stop_keypresses", "global_input", "Keyboard joystick stops keypresses",
			"Stop keyboard-as-joystick mappings from also generating Amiga key presses.",
			"false", &amiberry_options::input_keyboard_as_joystick_stop_keypresses),
		integer("default_joystick_deadzone", "global_input", "Joystick deadzone",
			"Default joystick and joystick-mouse deadzone percentage.",
			"33", 0, 100, &amiberry_options::default_joystick_deadzone),
		text("default_open_gui_key", "global_input", "Open GUI key",
			"Default keyboard shortcut for opening the GUI.", "F12"),
		text("default_quit_key", "global_input", "Quit key",
			"Default keyboard shortcut for quitting Amiberry.", ""),
		text("default_ar_key", "global_input", "Action Replay key",
			"Default keyboard shortcut for activating Action Replay or HRTmon.", "Pause"),
		text("default_fullscreen_toggle_key", "global_input", "Full-window toggle key",
			"Default keyboard shortcut for toggling windowed and full-window modes.", ""),
		boolean("default_retroarch_quit", "global_input", "RetroArch quit",
			"Enable the RetroArch-style quit button by default when a mapping exists.",
			"true", &amiberry_options::default_retroarch_quit),
		boolean("default_retroarch_menu", "global_input", "RetroArch menu",
			"Enable the RetroArch-style menu button by default when a mapping exists.",
			"true", &amiberry_options::default_retroarch_menu),
		boolean("default_retroarch_reset", "global_input", "RetroArch reset",
			"Enable the RetroArch-style reset button by default when a mapping exists.",
			"false", &amiberry_options::default_retroarch_reset),
		text("default_controller1", "global_input", "Controller 1",
			"Default device for joystick port 2 in new configurations.", "joy1"),
		text("default_controller2", "global_input", "Controller 2",
			"Second controller, used on the mouse port by WHDLoad and CD32 autoboot.", "joy2"),
		text("default_controller3", "global_input", "Controller 3",
			"Default device for parallel joystick port 3 in new configurations.", ""),
		text("default_controller4", "global_input", "Controller 4",
			"Default device for parallel joystick port 4 in new configurations.", ""),
		text("default_mouse1", "global_input", "Mouse 1",
			"Default device for the mouse port in new configurations.", "mouse"),
		text("default_mouse2", "global_input", "Mouse 2",
			"Second mouse device, used on the joystick port for mouse-controlled WHDLoad content.", "joy0"),

		choices_of("default_line_mode", "global_display", "Line mode",
			"Default native display line mode for new configurations.",
			"1", line_mode_choices, sizeof line_mode_choices / sizeof line_mode_choices[0],
			&amiberry_options::default_line_mode),
		boolean("default_horizontal_centering", "global_display", "Horizontal centering",
			"Center the native display horizontally by default.",
			"false", &amiberry_options::default_horizontal_centering),
		boolean("default_vertical_centering", "global_display", "Vertical centering",
			"Center the native display vertically by default.",
			"false", &amiberry_options::default_vertical_centering),
		choices_of("default_scaling_method", "global_display", "Scaling method",
			"Default image scaling method.",
			"-1", scaling_choices, sizeof scaling_choices / sizeof scaling_choices[0],
			&amiberry_options::default_scaling_method),
		choices_of("default_gfx_autoresolution", "global_display", "Auto resolution",
			"Enable native display auto-resolution switching by default.",
			"0", enabled_choices, sizeof enabled_choices / sizeof enabled_choices[0],
			&amiberry_options::default_gfx_autoresolution),
		boolean("default_correct_aspect_ratio", "global_display", "Correct aspect ratio",
			"Preserve the Amiga display aspect ratio by default.",
			"true", &amiberry_options::default_correct_aspect_ratio),
		boolean("default_auto_crop", "global_display", "Auto crop",
			"Enable automatic border crop by default.",
			"false", &amiberry_options::default_auto_crop),
		integer("default_width", "global_display", "Default width",
			"Default native display window width.", "720", 1, 16384, &amiberry_options::default_width),
		integer("default_height", "global_display", "Default height",
			"Default native display window height.", "568", 1, 16384, &amiberry_options::default_height),
		choices_of("default_fullscreen_mode", "global_display", "Screen mode",
			"Default native and RTG screen mode. The legacy exclusive full-screen value is full-window.",
			"0", fullscreen_choices, sizeof fullscreen_choices / sizeof fullscreen_choices[0],
			&amiberry_options::default_fullscreen_mode),

		choices_of("default_stereo_separation", "global_sound", "Stereo separation",
			"Default stereo separation between the left and right channels.",
			"7", stereo_choices, sizeof stereo_choices / sizeof stereo_choices[0],
			&amiberry_options::default_stereo_separation),
		choices_of("default_sound_frequency", "global_sound", "Sound frequency",
			"Default audio output sample rate in Hz.",
#ifdef __ANDROID__
			"48000",
#else
			"44100",
#endif
			frequency_choices, sizeof frequency_choices / sizeof frequency_choices[0],
			&amiberry_options::default_sound_frequency),
		choices_of("default_sound_buffer", "global_sound", "Sound buffer",
			"Default audio buffer size. Smaller values reduce latency.",
#ifdef __ANDROID__
			"8192",
#else
			"3072",
#endif
			buffer_choices, sizeof buffer_choices / sizeof buffer_choices[0],
			&amiberry_options::default_sound_buffer),
		integer("default_soundcard", "global_sound", "Audio device",
			"Default sound output device index. Zero uses the operating system default.",
			"0", 0, 255, &amiberry_options::default_soundcard),

		boolean("default_whd_buttonwait", "global_whdload", "Button wait",
			"Wait for a button press before starting WHDLoad games by default.",
			"false", &amiberry_options::default_whd_buttonwait),
		boolean("default_whd_showsplash", "global_whdload", "Show splash",
			"Show the WHDLoad splash screen by default.",
			"true", &amiberry_options::default_whd_showsplash),
		integer("default_whd_configdelay", "global_whdload", "Config delay",
			"Default delay in seconds before starting a WHDLoad title.",
			"0", 0, INT_MAX, &amiberry_options::default_whd_configdelay),
		boolean("default_whd_writecache", "global_whdload", "Write cache",
			"Enable the WHDLoad write cache by default.",
			"false", &amiberry_options::default_whd_writecache),
		boolean("default_whd_quit_on_exit", "global_whdload", "Quit on exit",
			"Quit Amiberry when the WHDLoad title exits.",
			"false", &amiberry_options::default_whd_quit_on_exit),
		boolean("use_jst_instead_of_whd", "global_whdload", "Use JST instead of WHDLoad",
			"Use JST instead of WHDLoad for auto-booted titles.",
			"false", &amiberry_options::use_jst_instead_of_whd),
		boolean("allow_display_settings_from_json", "global_whdload", "JSON display overrides",
			"Allow the WHDLoad JSON booter database to override display defaults.",
			"true", &amiberry_options::allow_display_settings_from_json),

		boolean("default_onscreen_joystick", "global_touch", "On-screen joystick",
			"Show virtual joystick controls by default on touchscreen devices.",
#ifdef __ANDROID__
			"true",
#else
			"false",
#endif
			&amiberry_options::default_onscreen_joystick),
		boolean("default_vkbd_enabled", "global_touch", "On-screen keyboard",
			"Enable the on-screen Amiga keyboard by default.",
#ifdef __ANDROID__
			"true",
#else
			"false",
#endif
			&amiberry_options::default_vkbd_enabled),
		integer("default_vkbd_transparency", "global_touch", "On-screen keyboard transparency",
			"Default on-screen keyboard transparency percentage.",
			"0", 0, 100, &amiberry_options::default_vkbd_transparency),
		language("default_vkbd_language", "global_touch", "On-screen keyboard layout",
			"Default on-screen keyboard layout.", "US"),
		text("default_vkbd_toggle", "global_touch", "On-screen keyboard toggle",
			"Default controller button or key that toggles the on-screen keyboard.",
#if defined(__ANDROID__) || defined(LIBRETRO)
			"leftstick"),
#else
			"guide"),
#endif
		boolean("default_retroarch_vkbd", "global_touch", "Use RetroArch vkbd button",
			"Use the RetroArch virtual keyboard button by default when a mapping exists.",
			"false", &amiberry_options::default_retroarch_vkbd),

		text("gui_theme", "global_visual", "GUI theme",
			"Theme file loaded by the ImGui interface.", "Default.theme"),
		text("shader", "global_visual", "Native shader",
			"Default shader preset for native chipset modes. none disables it.", "none"),
		text("shader_rtg", "global_visual", "RTG shader",
			"Default shader preset for RTG modes. none disables it.", "none"),
		boolean("use_bezel", "global_visual", "Use bezel",
			"Show the built-in CRT bezel overlay.", "false", &amiberry_options::use_bezel),
		boolean("use_custom_bezel", "global_visual", "Use custom bezel",
			"Show a bezel from the bezels folder instead of the built-in one.",
			"false", &amiberry_options::use_custom_bezel),
		text("custom_bezel", "global_visual", "Custom bezel",
			"Bezel overlay from the bezels folder. none disables it.", "none"),

		boolean("write_logfile", "global_paths", "Write logfile",
			"Write emulator log output to the configured logfile.",
			"false", &amiberry_options::write_logfile),
		path("base_content_path", "Base content",
			"Optional root used to derive most standard content folders.",
			get_base_content_path, set_base_content_path),
		path("config_path", "Configurations", "Folder containing user .uae configurations.",
			get_configuration_path, set_configuration_path),
		path("controllers_path", "Controllers", "Folder containing SDL controller database files.",
			get_controllers_path, set_controllers_path),
		path("retroarch_config", "RetroArch config",
			"RetroArch configuration file used for compatible controller mappings.",
			get_retroarch_file, set_retroarch_file),
		path("whdboot_path", "WHDBoot", "Folder containing WHDBooter support files.",
			get_whdbootpath, set_whdbootpath),
		path("whdload_arch_path", "WHDLoad archives", "Default folder for WHDLoad archives.",
			get_whdload_arch_path, set_whdload_arch_path),
		path("floppy_path", "Floppies", "Default folder for floppy disk images.",
			get_floppy_path, set_floppy_path),
		path("harddrive_path", "Hard drives", "Default folder for hard drive files and directories.",
			get_harddrive_path, set_harddrive_path),
		path("cdrom_path", "CD-ROMs", "Default folder for CD images.",
			get_cdrom_path, set_cdrom_path),
		path("logfile_path", "Logfile", "Log file path used when logfile output is enabled.",
			get_logfile_path, set_logfile_path),
		path("rom_path", "System ROMs", "Folder containing Kickstart and other system ROM files.",
			get_rom_path, set_rom_path),
		path("rp9_path", "RP9", "Default folder for RP9 packages.", get_rp9_path, set_rp9_path),
		path("floppy_sounds_dir", "Floppy sounds", "Folder containing floppy drive sound samples.",
			get_floppy_sounds_path, set_floppy_sounds_path),
		path("saveimage_dir", "Save data", "Default folder for save-data images.",
			get_saveimage_path, set_saveimage_path),
		path("savestate_dir", "Save states", "Default folder for emulator save states.",
			get_savestate_path, set_savestate_path),
		path("ripper_path", "Ripper", "Default output folder for ripper files.",
			get_ripper_path, set_ripper_path),
		path("inputrecordings_dir", "Input recordings", "Default folder for input recordings.",
			get_inputrecordings_path, set_inputrecordings_path),
		path("screenshot_dir", "Screenshots", "Default folder for screenshots.",
			get_screenshot_path, set_screenshot_path),
		path("nvram_dir", "NVRAM", "Default folder for NVRAM files.", get_nvram_path, set_nvram_path),
		path("plugins_dir", "Plugins override",
			"Optional folder searched after AMIBERRY_PLUGINS_DIR and before the user plugins folder.",
			get_plugins_override_path, set_plugins_path),
		path("video_dir", "Videos", "Default folder for video output files.",
			get_video_path, set_video_path),
		path("themes_path", "Themes", "Folder containing GUI theme files.",
			get_themes_path, set_themes_path),
		path("shaders_path", "Shaders", "Folder containing shader presets.",
			get_shaders_path, set_shaders_path),
		path("bezels_path", "Bezels", "Folder containing bezel overlay assets.",
			get_bezels_path, set_bezels_path),

		choices_of("update_check", "global_updates", "Startup update check",
			"Check for Amiberry updates on startup when the installation type supports it.",
			"1", enabled_choices, sizeof enabled_choices / sizeof enabled_choices[0],
			&amiberry_options::update_check),
		choices_of("update_channel", "global_updates", "Update channel",
			"Stable checks only releases. Preview can include pre-release versions.",
			"0", channel_choices, sizeof channel_choices / sizeof channel_choices[0],
			&amiberry_options::update_channel),
		text("skipped_version", "global_updates", "Skipped version",
			"Version string to skip during update checks. Empty stops skipping a version.", ""),

		boolean("perf_log", "global_advanced", "Performance log",
			"Enable performance logging for diagnostics.", "false", &amiberry_options::perf_log),
		boolean("slow_host_warning", "global_advanced", "Slow host warning",
			"Show warnings when Amiberry detects a slow host configuration.",
			"true", &amiberry_options::slow_host_warning),
		boolean("use_adpf", "global_advanced", "Android ADPF",
			"Use Android Dynamic Performance Framework hints when available.",
			"true", &amiberry_options::use_adpf),
		boolean("default_disable_cycle_exact", "global_advanced", "Disable cycle exact by default",
			"Disable cycle-exact defaults for quickstart-style configurations.",
			"false", &amiberry_options::default_disable_cycle_exact),
	};
	return list;
}

// The GUI scale is a real, clamped the way the configuration parser clamps it.
host_setting scale_setting()
{
	host_setting s{};
	s.id = "gui_layout_scale";
	s.group = "global_display";
	s.label = "GUI scale";
	s.help = "Size of the GUI as a percentage of automatic DPI scaling. 100 is the default.";
	s.k = kind::real;
#ifdef __ANDROID__
	s.default_value = "70";
#else
	s.default_value = "100";
#endif
	s.min = gui_layout_scale_min_percent;
	s.max = gui_layout_scale_max_percent;
	s.real = &amiberry_options::gui_layout_scale_percent;
	s.clamp_scale = true;
	return s;
}

const host_setting* find_setting(std::string_view id)
{
	for (const host_setting& s : settings()) {
		if (id == s.id)
			return &s;
	}
	if (id == "gui_layout_scale") {
		static const host_setting scale = scale_setting();
		return &scale;
	}
	return nullptr;
}

bool one_line(std::string_view text)
{
	return text.find_first_of("\r\n") == std::string_view::npos;
}

amiberry_setting_type setting_type(kind k)
{
	switch (k) {
	case kind::boolean: return AMIBERRY_SETTING_BOOL;
	case kind::integer: return AMIBERRY_SETTING_INT;
	case kind::choice_list: return AMIBERRY_SETTING_CHOICE;
	case kind::real: return AMIBERRY_SETTING_REAL;
	case kind::path: return AMIBERRY_SETTING_PATH;
	case kind::text: return AMIBERRY_SETTING_STRING;
	}
	return AMIBERRY_SETTING_STRING;
}

std::vector<amiberry_setting_descriptor>& descriptors()
{
	static std::vector<amiberry_setting_descriptor> list = [] {
		std::vector<amiberry_setting_descriptor> out;
		auto add = [&out](const host_setting& s) {
			amiberry_setting_descriptor d{};
			d.id = s.id;
			d.group = s.group;
			d.label = s.label;
			d.help = s.help;
			d.type = setting_type(s.k);
			d.apply = AMIBERRY_APPLY_LIVE;
			d.min = s.min;
			d.max = s.max;
			d.step = s.k == kind::integer || s.k == kind::real ? 1 : 0;
			d.default_value = s.default_value;
			d.choices = reinterpret_cast<const amiberry_setting_choice*>(s.choices);
			d.choice_count = s.choice_count;
			out.push_back(d);
		};
		for (const host_setting& s : settings())
			add(s);
		// The GUI scale sits after the other display defaults.
		const host_setting scale = scale_setting();
		auto at = out.begin();
		for (auto it = out.begin(); it != out.end(); ++it) {
			if (std::strcmp(it->group, "global_display") == 0)
				at = it + 1;
		}
		amiberry_setting_descriptor d{};
		d.id = scale.id; d.group = scale.group; d.label = scale.label; d.help = scale.help;
		d.type = AMIBERRY_SETTING_REAL; d.apply = AMIBERRY_APPLY_LIVE;
		d.min = scale.min; d.max = scale.max; d.step = 1; d.default_value = scale.default_value;
		out.insert(at, d);
		return out;
	}();
	return list;
}

const choice* choice_named(const host_setting& s, std::string_view value)
{
	for (size_t i = 0; i < s.choice_count; i++) {
		if (value == s.choices[i].value)
			return &s.choices[i];
	}
	return nullptr;
}

} // namespace

size_t host_settings_count()
{
	return descriptors().size();
}

const amiberry_setting_descriptor* host_settings_at(size_t index)
{
	std::vector<amiberry_setting_descriptor>& list = descriptors();
	return index < list.size() ? &list[index] : nullptr;
}

const amiberry_setting_descriptor* host_settings_find(std::string_view id)
{
	for (const amiberry_setting_descriptor& d : descriptors()) {
		if (id == d.id)
			return &d;
	}
	return nullptr;
}

amiberry_settings_status host_settings_get(std::string_view id, char* out, size_t size)
{
	const host_setting* s = find_setting(id);
	if (!s)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	std::string value;
	switch (s->k) {
	case kind::boolean:
		value = amiberry_options.*(s->boolean) ? "true" : "false";
		break;
	case kind::integer:
		value = std::to_string(amiberry_options.*(s->integer));
		break;
	case kind::choice_list:
		if (s->integer) {
			value = std::to_string(amiberry_options.*(s->integer));
		} else {
			size_t text_size = 0;
			value = text_of(s->id, text_size);
		}
		break;
	case kind::text: {
		size_t text_size = 0;
		value = text_of(s->id, text_size);
		break;
	}
	case kind::real: {
		char buffer[64];
		std::snprintf(buffer, sizeof buffer, "%.9g", amiberry_options.*(s->real));
		value = buffer;
		break;
	}
	case kind::path:
		value = s->path_get();
		break;
	}
	if (value.size() >= size)
		return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
	std::snprintf(out, size, "%s", value.c_str());
	return AMIBERRY_SETTINGS_OK;
}

amiberry_settings_status host_settings_set(std::string_view id, std::string_view value)
{
	const host_setting* s = find_setting(id);
	if (!s)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (!one_line(value))
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	switch (s->k) {
	case kind::boolean:
		if (value != "true" && value != "false")
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		amiberry_options.*(s->boolean) = value == "true";
		return AMIBERRY_SETTINGS_OK;
	case kind::integer:
	case kind::choice_list:
		if (s->integer) {
			const choice* chosen = s->k == kind::choice_list ? choice_named(*s, value) : nullptr;
			if (s->k == kind::choice_list && !chosen && !(s->id == std::string_view("default_fullscreen_mode") && value == "1"))
				return AMIBERRY_SETTINGS_INVALID_VALUE;
			int number = 0;
			if (s->k == kind::integer) {
				char* end = nullptr;
				const std::string text(value);
				number = static_cast<int>(std::strtol(text.c_str(), &end, 10));
				if (!end || *end || end == text.c_str() || number < s->min || number > s->max)
					return AMIBERRY_SETTINGS_INVALID_VALUE;
			} else {
				number = std::atoi(std::string(value).c_str());
			}
			if (std::strcmp(s->id, "default_fullscreen_mode") == 0)
				number = amiberry_normalize_gfx_fullscreen_mode(number);
			amiberry_options.*(s->integer) = number;
			return AMIBERRY_SETTINGS_OK;
		}
		{
			size_t text_size = 0;
			char* field = text_of(s->id, text_size);
			if (!field || value.size() >= text_size || !choice_named(*s, value))
				return AMIBERRY_SETTINGS_INVALID_VALUE;
			std::snprintf(field, text_size, "%.*s", static_cast<int>(value.size()), value.data());
			return AMIBERRY_SETTINGS_OK;
		}
	case kind::text: {
		size_t text_size = 0;
		char* field = text_of(s->id, text_size);
		if (!field || value.size() >= text_size)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		std::snprintf(field, text_size, "%.*s", static_cast<int>(value.size()), value.data());
		return AMIBERRY_SETTINGS_OK;
	}
	case kind::real: {
		char* end = nullptr;
		const std::string text(value);
		const float number = std::strtof(text.c_str(), &end);
		if (!end || *end || end == text.c_str())
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		amiberry_options.*(s->real) = s->clamp_scale ? clamp_gui_layout_scale_percent(number) : number;
		return AMIBERRY_SETTINGS_OK;
	}
	case kind::path:
		// The loader reads amiberry.conf in CONFIG_BLEN chunks and trims
		// trailing whitespace, so a longer line or a trailing space is not
		// what the next start loads. "key=value\n" must fit the chunk.
		if (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		if (value.size() + std::strlen(s->id) + 2 >= static_cast<size_t>(CONFIG_BLEN))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		s->path_set(std::string(value));
		return AMIBERRY_SETTINGS_OK;
	}
	return AMIBERRY_SETTINGS_UNKNOWN_ID;
}

bool host_settings_save()
{
	return save_amiberry_settings_with_result();
}

} // namespace amiberry_settings

size_t amiberry_host_settings_count(void)
{
	return amiberry_settings::host_settings_count();
}

const amiberry_setting_descriptor* amiberry_host_settings_at(size_t index)
{
	return amiberry_settings::host_settings_at(index);
}

const amiberry_setting_descriptor* amiberry_host_settings_find(const char* id)
{
	return id ? amiberry_settings::host_settings_find(id) : nullptr;
}

amiberry_settings_status amiberry_host_settings_get(const char* id, char* buffer, size_t size)
{
	if (!id)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (!buffer || size == 0)
		return AMIBERRY_SETTINGS_BUFFER_TOO_SMALL;
	return amiberry_settings::host_settings_get(id, buffer, size);
}

amiberry_settings_status amiberry_host_settings_set(const char* id, const char* value)
{
	if (!id)
		return AMIBERRY_SETTINGS_UNKNOWN_ID;
	if (!value)
		return AMIBERRY_SETTINGS_INVALID_VALUE;
	return amiberry_settings::host_settings_set(id, value);
}

bool amiberry_host_settings_save(void)
{
	return amiberry_settings::host_settings_save();
}
