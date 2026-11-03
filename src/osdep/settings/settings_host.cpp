// Host settings: the Global Settings panel. These live in amiberry_options
// and the path variables, saved to amiberry.conf, not in a configuration.
// They are process-wide, so there is no session: a set changes the live
// value, and amiberry_host_settings_save() writes the file.
//
// An ID is the amiberry.conf key. A choice's value is the integer the file
// stores, or the token for the default devices, shaders and bezel. A bool
// is "true" or "false" (the file spells them yes and no).

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "target.h"
#include "gui_layout_scale.h"
#include "amiberry_gfx_mode.h"
#include "macos_bookmarks.h"

#include "settings_definitions.h"

#include <cerrno>
#include <climits>
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
	{"-1", "Automatic"}, {"0", "Nearest"}, {"1", "Linear"}, {"2", "Integer"}, {"3", "Stretch"},
};
const choice enabled_choices[] = {{"0", "Disabled"}, {"1", "Enabled"}};
const choice autoresolution_choices[] = {{"0", "Disabled"}, {"1", "Always on"}};
const choice fullscreen_choices[] = {{"0", "Windowed"}, {"2", "Full-window"}};
const choice stereo_choices[] = {
	{"10", "100%"}, {"9", "90%"}, {"8", "80%"}, {"7", "70%"}, {"6", "60%"},
	{"5", "50%"}, {"4", "40%"}, {"3", "30%"}, {"2", "20%"}, {"1", "10%"}, {"0", "0%"},
};
const choice frequency_choices[] = {
	{"11025", "11,025 Hz"}, {"22050", "22,050 Hz"}, {"32000", "32,000 Hz"}, {"44100", "44,100 Hz"},
	{"48000", "48,000 Hz"},
};
const choice buffer_choices[] = {
	{"0", "Minimum"}, {"1024", "1024"}, {"2048", "2048"}, {"3072", "3072"}, {"4096", "4096"},
	{"6144", "6144"}, {"8192", "8192"}, {"12288", "12288"}, {"16384", "16384"},
	{"32768", "32768"}, {"65536", "65536"},
};
const choice language_choices[] = {{"US", "US"}, {"UK", "UK"}, {"DE", "German"}, {"FR", "French"}};
const choice channel_choices[] = {{"0", "Stable"}, {"1", "Preview"}};
#if defined(AMIBERRY_NATIVE_APPLE_UI) && defined(AMIBERRY_MACOS)
const choice left_command_q_choices[] = {{"1", "Asks to quit"}, {"2", "Quits"}, {"0", "Goes to the Amiga"}};
#endif

enum class kind { boolean, integer, choice_list, text, hotkey, real, path };

// Where a choice setting's list comes from: its own static list, or what
// the host offers now, enumerated by the configuration options that pick
// the same things (the joyport devices, the audio device, the shader and
// bezel catalogs), so both editors list the same entries.
enum class listing { fixed, port_device, sound_device, shader, bezel };

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
	listing list = listing::fixed;
	// A port device's empty value: what the emulator uses when none is set.
	const char* empty_label = nullptr;
	// The empty value already means no device, so the list leaves out the
	// joyports' "none", which means the same.
	bool empty_is_none = false;
	// The descriptor's unit and level (amiberry_settings.h).
	const char* unit = nullptr;
	bool level = false;
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

// An integer setting shown with its unit; level: set by feel, so a front
// end may show a slider.
host_setting measured(const char* id, const char* group, const char* label, const char* help,
	const char* def, int min, int max, int amiberry_options::* field, const char* unit, bool level)
{
	host_setting s = integer(id, group, label, help, def, min, max, field);
	s.unit = unit;
	s.level = level;
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

// A key combination or controller button (the hotkey syntax).
host_setting hotkey(const char* id, const char* group, const char* label, const char* help, const char* def)
{
	host_setting s = text(id, group, label, help, def);
	s.k = kind::hotkey;
	return s;
}

// A default port device: a joyport token, or empty for the fallback that
// empty_label names.
host_setting port_device(const char* id, const char* label, const char* help, const char* def,
	const char* empty_label)
{
	host_setting s = text(id, "global_input", label, help, def);
	s.k = kind::choice_list;
	s.list = listing::port_device;
	s.empty_label = empty_label;
	return s;
}

// A parallel port's default device, where empty means no device.
host_setting parallel_port_device(const char* id, const char* label, const char* help)
{
	host_setting s = port_device(id, label, help, "", "None");
	s.empty_is_none = true;
	return s;
}

// A shader or bezel from the catalogs, or "none".
host_setting catalog(const char* id, const char* label, const char* help, listing list)
{
	host_setting s = text(id, "global_visual", label, help, "none");
	s.k = kind::choice_list;
	s.list = list;
	return s;
}

// The default audio device: the index of a host device, 0 for the system
// default. Any index the file can hold is kept, as the device it names may
// only be unplugged now.
host_setting sound_device(const char* id, const char* label, const char* help)
{
	host_setting s{};
	s.id = id; s.group = "global_sound"; s.label = label; s.help = help; s.k = kind::choice_list;
	s.default_value = "0"; s.min = 0; s.max = 255; s.integer = &amiberry_options::default_soundcard;
	s.list = listing::sound_device;
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
		// The ImGui GUI's own settings: the native Apple apps have no ImGui
		// (AMIBERRY_NO_IMGUI), so there they would change nothing.
#ifndef AMIBERRY_NO_IMGUI
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
#endif

		boolean("rctrl_as_ramiga", "global_input", "Right Ctrl as Right Amiga",
			"Map Right Control to the Right Amiga key by default.",
			"false", &amiberry_options::rctrl_as_ramiga),
#if defined(AMIBERRY_NATIVE_APPLE_UI) && defined(AMIBERRY_MACOS)
		// Read by the macOS menu bar (apple/macOS/MainMenu.swift).
		choices_of("macos_left_command_q", "global_input", "Left Command-Q",
			"What Left Command-Q does while an emulation window is active: ask whether to quit, quit at once, "
			"or reach the Amiga as Left Amiga-Q. Right Command-Q always goes to the Amiga.",
			"1", left_command_q_choices, sizeof left_command_q_choices / sizeof left_command_q_choices[0],
			&amiberry_options::macos_left_command_q),
#endif
		measured("input_default_mouse_speed", "global_input", "Default mouse speed",
			"Mouse speed applied to new configurations (100% is normal).",
			"100", 1, 1000, &amiberry_options::input_default_mouse_speed, "%", false),
		boolean("input_keyboard_as_joystick_stop_keypresses", "global_input", "Keyboard joystick stops keypresses",
			"Stop keyboard-as-joystick mappings from also generating Amiga key presses.",
			"false", &amiberry_options::input_keyboard_as_joystick_stop_keypresses),
		measured("default_joystick_deadzone", "global_input", "Joystick dead zone",
			"Default analog stick dead zone for new configurations, as a percentage of full deflection.",
			"33", 0, 100, &amiberry_options::default_joystick_deadzone, "%", true),
		hotkey("default_open_gui_key", "global_input", "Open GUI key",
			"Default key or controller button that opens the GUI.", "F12"),
		hotkey("default_quit_key", "global_input", "Quit key",
			"Default key or controller button that quits Amiberry.", ""),
		hotkey("default_ar_key", "global_input", "Action Replay key",
			"Default key or controller button that activates Action Replay or HRTmon.", "Pause"),
		hotkey("default_fullscreen_toggle_key", "global_input", "Full-window toggle key",
			"Default key or controller button that switches between windowed and full-window modes.", ""),
		boolean("default_retroarch_quit", "global_input", "RetroArch quit",
			"Enable the RetroArch-style quit button by default when a mapping exists.",
			"true", &amiberry_options::default_retroarch_quit),
		boolean("default_retroarch_menu", "global_input", "RetroArch menu",
			"Enable the RetroArch-style menu button by default when a mapping exists.",
			"true", &amiberry_options::default_retroarch_menu),
		boolean("default_retroarch_reset", "global_input", "RetroArch reset",
			"Enable the RetroArch-style reset button by default when a mapping exists.",
			"false", &amiberry_options::default_retroarch_reset),
		// An empty device makes the emulator fall back, as amiberry.cpp's
		// default options and the WHDLoad booter do; each list names the
		// fallback.
		port_device("default_controller1", "Controller 1",
			"Default device for port 2, the joystick port, in new configurations and autobooted content. "
			"Automatic uses the first joystick, or the second one for WHDLoad.", "joy1", "Automatic"),
		port_device("default_controller2", "Controller 2",
			"Second controller, used on port 1, the mouse port, by WHDLoad and CD32 autoboot. "
			"Automatic keeps the mouse there.", "joy2", "Automatic"),
		parallel_port_device("default_controller3", "Controller 3",
			"Default device for parallel joystick port 3 in new configurations and WHDLoad autoboot."),
		parallel_port_device("default_controller4", "Controller 4",
			"Default device for parallel joystick port 4 in new configurations and WHDLoad autoboot."),
		port_device("default_mouse1", "Mouse 1",
			"Default device for port 1, the mouse port, in new configurations and mouse-controlled "
			"autobooted content. Automatic uses the mouse.", "mouse", "Automatic"),
		port_device("default_mouse2", "Mouse 2",
			"Second mouse device, used on port 2, the joystick port, for mouse-controlled WHDLoad content. "
			"Automatic uses the Controller 1 device.", "joy0", "Automatic"),

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
			"0", autoresolution_choices, sizeof autoresolution_choices / sizeof autoresolution_choices[0],
			&amiberry_options::default_gfx_autoresolution),
		boolean("default_correct_aspect_ratio", "global_display", "Correct aspect ratio",
			"Preserve the Amiga display aspect ratio by default.",
			"true", &amiberry_options::default_correct_aspect_ratio),
		boolean("default_auto_crop", "global_display", "Auto crop",
			"Enable automatic border crop by default.",
			"false", &amiberry_options::default_auto_crop),
		measured("default_width", "global_display", "Default width",
			"Default native display window width.", "720", 1, 16384, &amiberry_options::default_width,
			"px", false),
		measured("default_height", "global_display", "Default height",
			"Default native display window height.", "568", 1, 16384, &amiberry_options::default_height,
			"px", false),
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
			AMIBERRY_SETTINGS_TEXT_OF(ANDROID_DEFAULT_SOUND_FREQUENCY),
#else
			"44100",
#endif
			frequency_choices, sizeof frequency_choices / sizeof frequency_choices[0],
			&amiberry_options::default_sound_frequency),
		choices_of("default_sound_buffer", "global_sound", "Sound buffer",
			"Default audio buffer size. Smaller values reduce latency.",
#ifdef __ANDROID__
			AMIBERRY_SETTINGS_TEXT_OF(ANDROID_DEFAULT_SOUND_BUFFER),
#else
			"3072",
#endif
			buffer_choices, sizeof buffer_choices / sizeof buffer_choices[0],
			&amiberry_options::default_sound_buffer),
		sound_device("default_soundcard", "Audio device",
			"Default sound output device. System default follows the operating system's choice."),

		boolean("default_whd_buttonwait", "global_whdload", "Button wait",
			"Wait for a button press before starting WHDLoad games by default.",
			"false", &amiberry_options::default_whd_buttonwait),
		boolean("default_whd_showsplash", "global_whdload", "Show splash",
			"Show the WHDLoad splash screen by default.",
			"true", &amiberry_options::default_whd_showsplash),
		measured("default_whd_configdelay", "global_whdload", "Config delay",
			"Default delay before starting a WHDLoad title.",
			"0", 0, INT_MAX, &amiberry_options::default_whd_configdelay, "s", false),
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
		measured("default_vkbd_transparency", "global_touch", "On-screen keyboard opacity",
			"Opacity of the on-screen keyboard in new configurations (100% is opaque); 0 uses the "
			"default of 85%.",
			"0", 0, 100, &amiberry_options::default_vkbd_transparency, "%", false),
		language("default_vkbd_language", "global_touch", "On-screen keyboard layout",
			"Default on-screen keyboard layout.", "US"),
		hotkey("default_vkbd_toggle", "global_touch", "On-screen keyboard toggle",
			"Default key or controller button that shows and hides the on-screen keyboard.",
#if defined(__ANDROID__) || defined(LIBRETRO)
			"leftstick"),
#else
			"guide"),
#endif
		boolean("default_retroarch_vkbd", "global_touch", "RetroArch on-screen keyboard button",
			"Use the RetroArch on-screen keyboard button by default when a mapping exists.",
			"false", &amiberry_options::default_retroarch_vkbd),

#ifndef AMIBERRY_NO_IMGUI
		text("gui_theme", "global_visual", "GUI theme",
			"Theme file loaded by the ImGui interface.", "Default.theme"),
#endif
		catalog("shader", "Native shader",
			"Default shader for native chipset modes: a built-in CRT shader or a preset from the shaders folder.",
			listing::shader),
		catalog("shader_rtg", "RTG shader",
			"Default shader for RTG modes: a built-in CRT shader or a preset from the shaders folder.",
			listing::shader),
		boolean("use_bezel", "global_visual", "Use bezel",
			"Show the built-in CRT bezel overlay.", "false", &amiberry_options::use_bezel),
		boolean("use_custom_bezel", "global_visual", "Use custom bezel",
			"Show a bezel from the bezels folder instead of the built-in one.",
			"false", &amiberry_options::use_custom_bezel),
		catalog("custom_bezel", "Custom bezel",
			"Bezel image from the bezels folder. Picking one turns Use custom bezel on and the built-in "
			"bezel off; None turns Use custom bezel off.", listing::bezel),

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
			get_savestate_folder, set_savestate_path),
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

		// Store builds are updated by the App Store: no update settings.
#if !defined(AMIBERRY_IOS) && !defined(MACOS_APP_STORE)
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
#endif

		boolean("perf_log", "global_advanced", "Performance log",
			"Enable performance logging for diagnostics.", "false", &amiberry_options::perf_log),
		boolean("slow_host_warning", "global_advanced", "Slow host warning",
			"Show warnings when Amiberry detects a slow host configuration.",
			"true", &amiberry_options::slow_host_warning),
#ifdef __ANDROID__
		boolean("use_adpf", "global_advanced", "Android ADPF",
			"Use Android Dynamic Performance Framework hints when available.",
			"true", &amiberry_options::use_adpf),
#endif
		boolean("default_disable_cycle_exact", "global_advanced", "Disable cycle exact by default",
			"Disable cycle-exact defaults for quickstart-style configurations.",
			"false", &amiberry_options::default_disable_cycle_exact),
	};
	return list;
}

#ifndef AMIBERRY_NO_IMGUI
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
	s.unit = "%";
	return s;
}
#endif

const host_setting* find_setting(std::string_view id)
{
	for (const host_setting& s : settings()) {
		if (id == s.id)
			return &s;
	}
#ifndef AMIBERRY_NO_IMGUI
	if (id == "gui_layout_scale") {
		static const host_setting scale = scale_setting();
		return &scale;
	}
#endif
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
	case kind::hotkey: return AMIBERRY_SETTING_HOTKEY;
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
			// A choice's range is its list (the audio device keeps its
			// index range for the setter only).
			d.min = s.k == kind::choice_list ? 0 : s.min;
			d.max = s.k == kind::choice_list ? 0 : s.max;
			d.step = s.k == kind::integer || s.k == kind::real ? 1 : 0;
			d.default_value = s.default_value;
			d.choices = reinterpret_cast<const amiberry_setting_choice*>(s.choices);
			d.choice_count = s.choice_count;
			d.unit = s.unit;
			d.level = s.level;
			out.push_back(d);
		};
		for (const host_setting& s : settings())
			add(s);
#ifndef AMIBERRY_NO_IMGUI
		// The GUI scale (ImGui's) sits after the other display defaults.
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
		d.unit = scale.unit;
		out.insert(at, d);
#endif
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

// The list a listed setting offers now. Entries point into the
// configuration options' lists, this vector and the stored value's
// strings, valid until the next main-thread call of the layer.
std::vector<amiberry_setting_choice> listed_choices;
std::string stored_value;
std::string stored_label;

choice_list enumerated(std::string_view option)
{
	return amiberry_settings::choices_of(require(option), changed_prefs);
}

// The stored value, read the way get reports it.
std::string stored_text(const host_setting& s)
{
	if (s.integer)
		return std::to_string(amiberry_options.*(s.integer));
	size_t size = 0;
	const char* text = text_of(s.id, size);
	return text ? text : "";
}

void list_choices(const host_setting& s)
{
	listed_choices.clear();
	switch (s.list) {
	case listing::fixed:
		break;
	case listing::port_device: {
		// The fallback first, then the devices the joyports offer. Where the
		// fallback is no device, one entry stands for both spellings of
		// it: the stored "none" while that is the value, else "".
		size_t size = 0;
		const bool none_stored = s.empty_is_none && !std::strcmp(text_of(s.id, size), "none");
		listed_choices.push_back({none_stored ? "none" : "", s.empty_label});
		const choice_list devices = connected_port_devices();
		for (size_t i = 0; i < devices.count; i++) {
			if (!s.empty_is_none || std::strcmp(devices.items[i].value, "none") != 0)
				listed_choices.push_back(devices.items[i]);
		}
		break;
	}
	case listing::sound_device: {
		// Index 0 stands for the system default here, as in the Global
		// Settings panel, so the first host device is not listed by name.
		listed_choices.push_back({"0", "System default"});
		const choice_list devices = enumerated("amiberry.soundcard");
		for (size_t i = 1; i < devices.count; i++)
			listed_choices.push_back(devices.items[i]);
		break;
	}
	case listing::shader:
	case listing::bezel: {
		listed_choices.push_back({"none", "None"});
		const choice_list names = enumerated(s.list == listing::shader ? "amiberry.shader" : "amiberry.custom_bezel");
		for (size_t i = 0; i < names.count; i++) {
			if (std::strcmp(names.items[i].value, "none") != 0)
				listed_choices.push_back(names.items[i]);
		}
		break;
	}
	}
	// A stored value the list lacks comes first, named for what it is, so
	// a front end shows the list as it is.
	stored_value = stored_text(s);
	for (const amiberry_setting_choice& c : listed_choices) {
		if (stored_value == c.value)
			return;
	}
	switch (s.list) {
	case listing::port_device:
		stored_label = port_device_label(stored_value);
		break;
	case listing::sound_device:
		stored_label = "Audio device " + stored_value + " (not connected)";
		break;
	case listing::shader:
	case listing::bezel:
		stored_label = stored_value + " (not found)";
		break;
	case listing::fixed:
		stored_label = stored_value;
		break;
	}
	listed_choices.insert(listed_choices.begin(), {stored_value.c_str(), stored_label.c_str()});
}

// Text no setting of the file keeps as it was set: the loader strips
// whitespace at both ends (a shader name ending in a space, for one).
bool trimmed(std::string_view value)
{
	return value.empty() || (value.front() != ' ' && value.front() != '\t'
		&& value.back() != ' ' && value.back() != '\t');
}

// The port device tokens the core's joyport parser (inputdevice_joyport_config)
// reads besides the settings layer's: custom input mappings ("custom",
// "customN") and the first joystick or mouse ("joydefault", "mousedefault").
bool core_port_device_token(std::string_view value)
{
	if (value == "joydefault" || value == "mousedefault" || value == "custom")
		return true;
	if (value.substr(0, 6) != "custom" || value.size() == 6)
		return false;
	return value.find_first_not_of("0123456789", 6) == std::string_view::npos;
}

// Whether a text-backed choice setting takes value. A listed setting also
// takes what its list cannot show now: a device that is not connected, a
// shader or bezel no longer in its folder.
bool text_choice_accepted(const host_setting& s, std::string_view value)
{
	switch (s.list) {
	case listing::fixed:
		return choice_named(s, value) != nullptr;
	case listing::port_device:
		return value.empty() || is_port_device_token(value) || core_port_device_token(value);
	case listing::shader:
	case listing::bezel:
		// "none" is how no shader or bezel is spelled; empty names no file.
		return !value.empty() && trimmed(value);
	case listing::sound_device:
		break;
	}
	return false;
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
	case kind::text:
	case kind::hotkey: {
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
			// The audio device takes any index in range, listed or not.
			const bool ranged = s->k == kind::integer || s->list == listing::sound_device;
			const choice* chosen = ranged ? nullptr : choice_named(*s, value);
			if (!ranged && !chosen && !(s->id == std::string_view("default_fullscreen_mode") && value == "1"))
				return AMIBERRY_SETTINGS_INVALID_VALUE;
			int number = 0;
			if (ranged) {
				char* end = nullptr;
				const std::string text(value);
				errno = 0;
				const long wide = std::strtol(text.c_str(), &end, 10);
				// Narrowing before the check wraps a too-big value into range
				// (4294967336 becomes 40), so the long is checked first.
				if (!end || *end || end == text.c_str() || errno == ERANGE
					|| wide < s->min || wide > s->max)
					return AMIBERRY_SETTINGS_INVALID_VALUE;
				number = static_cast<int>(wide);
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
			if (!field || value.size() >= text_size || !text_choice_accepted(*s, value))
				return AMIBERRY_SETTINGS_INVALID_VALUE;
			std::snprintf(field, text_size, "%.*s", static_cast<int>(value.size()), value.data());
			// Picking a bezel shows it instead of the built-in one, and none
			// shows none, as the Global Settings panel does.
			if (s->list == listing::bezel) {
				amiberry_options.use_custom_bezel = _tcsicmp(field, "none") != 0;
				if (amiberry_options.use_custom_bezel)
					amiberry_options.use_bezel = false;
			}
			return AMIBERRY_SETTINGS_OK;
		}
	case kind::text:
	case kind::hotkey: {
		// The loader strips whitespace at both ends, so a value with either
		// comes back different.
		if (!trimmed(value))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
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
		// The loader strips whitespace at both ends of a value, so a path
		// with either does not come back as it was set.
		if (!trimmed(value))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		if (value.size() + std::strlen(s->id) + 2 >= static_cast<size_t>(CONFIG_BLEN))
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		// The folder getters add a trailing separator, and the search path
		// copy keeps PATH_MAX - 1, so the stored path must leave both.
		if (value.size() >= static_cast<size_t>(PATH_MAX) - 1)
			return AMIBERRY_SETTINGS_INVALID_VALUE;
		const std::string previous = s->path_get();
		s->path_set(std::string(value));
		// An App Store build needs a security-scoped bookmark for the folder
		// (the setter asks for one and ignores the result). An empty path
		// clears the folder and needs none. A failure puts the old folder back.
		if (!value.empty() && !macos_bookmark_store(std::string(value))) {
			s->path_set(previous);
			return AMIBERRY_SETTINGS_UNAVAILABLE;
		}
		// Another ROMs folder holds other ROMs (as the GUI's Paths panel).
		if (std::strcmp(s->id, "rom_path") == 0 && previous != s->path_get())
			rescan_roms();
		return AMIBERRY_SETTINGS_OK;
	}
	return AMIBERRY_SETTINGS_UNKNOWN_ID;
}

size_t host_settings_choices(std::string_view id, const amiberry_setting_choice** out)
{
	*out = nullptr;
	const host_setting* s = find_setting(id);
	if (!s || s->k != kind::choice_list)
		return 0;
	if (s->list == listing::fixed) {
		*out = reinterpret_cast<const amiberry_setting_choice*>(s->choices);
		return s->choice_count;
	}
	list_choices(*s);
	*out = listed_choices.data();
	return listed_choices.size();
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

size_t amiberry_host_settings_choices(const char* id, const amiberry_setting_choice** choices)
{
	if (!choices)
		return 0;
	*choices = nullptr;
	if (!id)
		return 0;
	return amiberry_settings::no_throw<size_t>(0, [&] {
		return amiberry_settings::host_settings_choices(id, choices);
	});
}

void amiberry_host_settings_refresh_choices(void)
{
	amiberry_settings::no_throw(0, [] {
		amiberry_settings::refresh_dynamic_choices();
		return 0;
	});
}

bool amiberry_host_settings_save(void)
{
	return amiberry_settings::host_settings_save();
}
