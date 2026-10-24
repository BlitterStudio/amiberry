// Miscellaneous and priority options: the GUI's Miscellaneous and Priority
// panels. Behaviour switches, the keyboard shortcuts the emulator listens
// for, which status LEDs the host keyboard's lock lights show, and what the
// emulation does while its window is inactive or minimized.

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "gui.h"
#include "statusline.h"

#include "settings_definitions.h"

#include <cstdio>
#include <cstring>

namespace amiberry_settings {

namespace {

#define CHOICES(array) array, sizeof(array) / sizeof((array)[0])

#define BOOL_FIELD(field) \
	void get_##field(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_bool(out, size, p.field); \
	} \
	void set_##field(uae_prefs& p, const value& v) \
	{ \
		p.field = v.number != 0; \
	}

BOOL_FIELD(main_alwaysontop)
BOOL_FIELD(gui_alwaysontop)
BOOL_FIELD(tod_hack)
BOOL_FIELD(reset_delay)
BOOL_FIELD(clipboard_sharing)
BOOL_FIELD(native_code)
BOOL_FIELD(illegal_mem)
BOOL_FIELD(blankmonitors)
BOOL_FIELD(start_uncaptured)
BOOL_FIELD(start_minimized)
BOOL_FIELD(minimize_inactive)
BOOL_FIELD(floppy_read_only)
BOOL_FIELD(harddrive_read_only)
BOOL_FIELD(uae_hide_autoconfig)
BOOL_FIELD(right_control_is_right_win_key)
BOOL_FIELD(capture_always)
BOOL_FIELD(debug_mem)
BOOL_FIELD(crash_auto_reset)
BOOL_FIELD(scsidevicedisable)
BOOL_FIELD(turbo_boot)
BOOL_FIELD(alt_tab_release)
BOOL_FIELD(ctrl_alt_release)
BOOL_FIELD(use_retroarch_quit)
BOOL_FIELD(use_retroarch_menu)
BOOL_FIELD(use_retroarch_reset)
BOOL_FIELD(active_nocapture_nosound)
BOOL_FIELD(inactive_pause)
BOOL_FIELD(inactive_nosound)
BOOL_FIELD(minimized_pause)
BOOL_FIELD(minimized_nosound)

#undef BOOL_FIELD

// use_gui: written as "no" or "yes" (a configuration may also say
// "nowait", read as yes).
const amiberry_setting_choice use_gui_choices[] = {
	{"no", "No"},
	{"yes", "Yes"},
};

void get_start_gui(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.start_gui ? "yes" : "no");
}

void set_start_gui(uae_prefs& p, const value& v)
{
	p.start_gui = v.text == "yes";
}

// The status line, one bit per display kind.
#define LEDS_BIT(name, bit) \
	void get_##name(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_bool(out, size, (p.leds_on_screen & (bit)) != 0); \
	} \
	void set_##name(uae_prefs& p, const value& v) \
	{ \
		if (v.number != 0) \
			p.leds_on_screen |= (bit); \
		else \
			p.leds_on_screen &= ~(bit); \
	}

LEDS_BIT(show_leds, STATUSLINE_CHIPSET)
LEDS_BIT(show_leds_rtg, STATUSLINE_RTG)

#undef LEDS_BIT

// The power LED's brightness while the audio filter is off, as the
// configuration writes it: 128 dims it to half, 0 keeps it full. Another
// level from a configuration reads as dimmed.
const amiberry_setting_choice power_led_dim_choices[] = {
	{"0", "Off"},
	{"128", "Dimmed"},
};

void get_power_led_dim(const uae_prefs& p, char* out, size_t size)
{
	write_int(out, size, (p.power_led_dim & 255) != 0 ? 128 : 0);
}

void set_power_led_dim(uae_prefs& p, const value& v)
{
	p.power_led_dim = static_cast<int>(v.number);
}

// target_fixup_options(): pausing while active but not captured also
// pauses, mutes and ignores input while inactive. Set here too, as a live
// apply runs no fixup.
void get_active_nocapture_pause(const uae_prefs& p, char* out, size_t size)
{
	write_bool(out, size, p.active_nocapture_pause);
}

void set_active_nocapture_pause(uae_prefs& p, const value& v)
{
	p.active_nocapture_pause = v.number != 0;
	if (p.active_nocapture_pause) {
		p.inactive_pause = true;
		p.inactive_nosound = true;
		p.inactive_input = 0;
	}
}

// Keyboard shortcuts: a key with optional modifiers ("LCtrl+F12") or an SDL
// gamepad button name, as the GUI's capture records it; empty unbinds it.
#define HOTKEY_FIELD(field) \
	void get_##field(const uae_prefs& p, char* out, size_t size) \
	{ \
		std::snprintf(out, size, "%s", p.field); \
	} \
	void set_##field(uae_prefs& p, const value& v) \
	{ \
		std::snprintf(p.field, sizeof p.field, "%.*s", static_cast<int>(v.text.size()), v.text.data()); \
	} \
	bool fits_##field(const uae_prefs& p, const value& v) \
	{ \
		return v.text.size() < sizeof p.field; \
	}

HOTKEY_FIELD(open_gui)
HOTKEY_FIELD(quit_amiberry)
HOTKEY_FIELD(action_replay)
HOTKEY_FIELD(fullscreen_toggle)
HOTKEY_FIELD(minimize)
HOTKEY_FIELD(left_amiga)
HOTKEY_FIELD(right_amiga)
HOTKEY_FIELD(screenshot)
HOTKEY_FIELD(debugger_trigger)

#undef HOTKEY_FIELD

// The configuration's description and category, as the Configurations panel
// saves them: one line each (the writer does not escape them, so a line
// break would split the entry), the category without surrounding spaces.
bool fits_config_text(size_t size, const value& v)
{
	return v.text.size() < size && v.text.find_first_of("\r\n") == std::string_view::npos;
}

void get_description(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.description);
}

void set_description(uae_prefs& p, const value& v)
{
	std::snprintf(p.description, sizeof p.description, "%.*s", static_cast<int>(v.text.size()), v.text.data());
}

bool fits_description(const uae_prefs& p, const value& v)
{
	return fits_config_text(sizeof p.description, v);
}

void get_category(const uae_prefs& p, char* out, size_t size)
{
	std::snprintf(out, size, "%s", p.category);
}

void set_category(uae_prefs& p, const value& v)
{
	std::string_view text = v.text;
	const auto first = text.find_first_not_of(" \t");
	text = first == std::string_view::npos ? std::string_view() : text.substr(first);
	const auto last = text.find_last_not_of(" \t");
	text = last == std::string_view::npos ? std::string_view() : text.substr(0, last + 1);
	std::snprintf(p.category, sizeof p.category, "%.*s", static_cast<int>(text.size()), text.data());
}

bool fits_category(const uae_prefs& p, const value& v)
{
	return fits_config_text(sizeof p.category, v);
}

// Which emulated LED a host keyboard lock light shows.
const amiberry_setting_choice led_choices[] = {
	{"-1", "None"},
	{"0", "Power"},
	{"1", "DF0:"},
	{"2", "DF1:"},
	{"3", "DF2:"},
	{"4", "DF3:"},
	{"5", "Hard drive"},
	{"6", "CD"},
};
static_assert(LED_POWER == 0 && LED_DF0 == 1 && LED_DF3 == 4 && LED_HD == 5 && LED_CD == 6,
	"led_choices spell the LED_* numbers");

#define LED_FIELD(field) \
	void get_##field(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_int(out, size, p.field); \
	} \
	void set_##field(uae_prefs& p, const value& v) \
	{ \
		p.field = static_cast<int>(v.number); \
	}

LED_FIELD(kbd_led_num)
LED_FIELD(kbd_led_scr)
LED_FIELD(kbd_led_cap)

#undef LED_FIELD

// The host process priority while the window is active, inactive or
// minimized.
const amiberry_setting_choice priority_choices[] = {
	{"0", "Low"},
	{"1", "Normal"},
	{"2", "High"},
};

#define PRIORITY_FIELD(field) \
	void get_##field(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_int(out, size, p.field); \
	} \
	void set_##field(uae_prefs& p, const value& v) \
	{ \
		p.field = static_cast<int>(v.number); \
	}

PRIORITY_FIELD(active_capture_priority)
PRIORITY_FIELD(inactive_priority)
PRIORITY_FIELD(minimized_priority)

#undef PRIORITY_FIELD

// Input while inactive or minimized, as the configuration writes it: 4
// (its bit 4) keeps the host's input, 0 ignores it (the GUI's "Disable
// input"). Other bits a configuration sets are kept.
const amiberry_setting_choice input_choices[] = {
	{"0", "Ignored"},
	{"4", "Used"},
};

#define INPUT_FIELD(field) \
	void get_##field(const uae_prefs& p, char* out, size_t size) \
	{ \
		write_int(out, size, p.field & 4); \
	} \
	void set_##field(uae_prefs& p, const value& v) \
	{ \
		if (v.number != 0) \
			p.field |= 4; \
		else \
			p.field &= ~4; \
	}

INPUT_FIELD(inactive_input)
INPUT_FIELD(minimized_input)

#undef INPUT_FIELD

// target_fixup_options(): an emulation that pauses while active but not
// captured also pauses, mutes and ignores input while inactive.
bool inactive_options_available(const uae_prefs& p, const context&)
{
	return !p.active_nocapture_pause;
}

constexpr const char* inactive_constraint =
	"Always on while the emulation pauses when active but not captured.";

// ---------------------------------------------------------------- table

#define BOOL_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_BOOL, apply, 0, 0, 0, def, nullptr, 0, constraint}
#define CHOICE_OPTION(id, group, label, help, apply, def, choices, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_CHOICE, apply, 0, 0, 0, def, CHOICES(choices), constraint}
#define STRING_OPTION(id, group, label, help, apply, def, constraint) \
	{id, group, label, help, AMIBERRY_SETTING_STRING, apply, 0, 0, 0, def, nullptr, 0, constraint}

#define BOOL_DEF(id, group, label, help, apply, def, field) \
	{BOOL_OPTION(id, group, label, help, apply, def, nullptr), get_##field, set_##field, nullptr, nullptr, \
		nullptr, nullptr}
#define HOTKEY_DEF(id, label, help, def, field) \
	{STRING_OPTION(id, "misc", label, help, AMIBERRY_APPLY_LIVE, def, nullptr), get_##field, set_##field, \
		nullptr, nullptr, nullptr, fits_##field}

const definition definitions[] = {
	{CHOICE_OPTION("use_gui", "misc", "Show GUI on startup",
			"Show the settings when Amiberry starts, before the emulation runs.",
			AMIBERRY_APPLY_RESTART, "yes", use_gui_choices, nullptr),
		get_start_gui, set_start_gui, nullptr, nullptr, nullptr, nullptr},
	BOOL_DEF("amiberry.always_on_top", "misc", "Always on top",
		"Keep the emulator window above other windows.",
		AMIBERRY_APPLY_LIVE, "false", main_alwaysontop),
	BOOL_DEF("amiberry.gui_always_on_top", "misc", "GUI always on top",
		"Keep the settings window above other windows.",
		AMIBERRY_APPLY_LIVE, "false", gui_alwaysontop),
	BOOL_DEF("synchronize_clock", "misc", "Synchronize clock",
		"Keep the Amiga's clock in step with the host's time.",
		AMIBERRY_APPLY_RESTART, "false", tod_hack),
	BOOL_DEF("cpu_reset_pause", "misc", "One second reboot pause",
		"Pause for one second when the Amiga resets.",
		AMIBERRY_APPLY_LIVE, "false", reset_delay),
	BOOL_DEF("clipboard_sharing", "misc", "Clipboard sharing",
		"Share the clipboard between the Amiga and the host.",
		AMIBERRY_APPLY_RESTART, "false", clipboard_sharing),
	BOOL_DEF("native_code", "misc", "Allow native code",
		"Let Amiga programs run host code through the emulator's native code interface.",
		AMIBERRY_APPLY_LIVE, "false", native_code),
	BOOL_DEF("show_leds", "misc", "Native on-screen display",
		"Show the status line with the drive and power LEDs on the native Amiga display.",
		AMIBERRY_APPLY_LIVE, "false", show_leds),
	BOOL_DEF("show_leds_rtg", "misc", "RTG on-screen display",
		"Show the status line with the drive and power LEDs on the RTG display.",
		AMIBERRY_APPLY_LIVE, "false", show_leds_rtg),
	BOOL_DEF("log_illegal_mem", "misc", "Log illegal memory accesses",
		"Log reads and writes to addresses where no memory is.",
		AMIBERRY_APPLY_RESTART, "false", illegal_mem),
	BOOL_DEF("amiberry.blank_monitors", "misc", "Blank unused displays",
		"Blank the other displays while the emulation runs full-window.",
		AMIBERRY_APPLY_LIVE, "false", blankmonitors),
	BOOL_DEF("amiberry.start_not_captured", "misc", "Start mouse uncaptured",
		"Start the emulation without capturing the mouse.",
		AMIBERRY_APPLY_RESTART, "false", start_uncaptured),
	BOOL_DEF("amiberry.start_minimized", "misc", "Start minimized",
		"Start the emulation with its window minimized.",
		AMIBERRY_APPLY_RESTART, "false", start_minimized),
	BOOL_DEF("amiberry.inactive_minimize", "misc", "Minimize when focus is lost",
		"Minimize the emulator window when another window takes the focus.",
		AMIBERRY_APPLY_LIVE, "false", minimize_inactive),
	BOOL_DEF("floppy_write_protect", "misc", "Master floppy write protection",
		"Write-protect every floppy disk image.",
		AMIBERRY_APPLY_LIVE, "false", floppy_read_only),
	BOOL_DEF("harddrive_write_protect", "misc", "Master hard drive write protection",
		"Write-protect every hard drive.",
		AMIBERRY_APPLY_LIVE, "false", harddrive_read_only),
	BOOL_DEF("uae_hide_autoconfig", "misc", "Hide all UAE autoconfig boards",
		"Hide the emulator's own expansion boards from the Amiga's autoconfig.",
		AMIBERRY_APPLY_RESTART, "false", uae_hide_autoconfig),
	BOOL_DEF("amiberry.right_control_is_right_win", "misc", "RCtrl = RAmiga",
		"Use the right Control key as the right Amiga key.",
		AMIBERRY_APPLY_LIVE, "false", right_control_is_right_win_key),
	{CHOICE_OPTION("power_led_dim", "misc", "Power LED when the audio filter is off",
			"Show the power LED at half brightness while the audio filter is off, as on an A500.",
			AMIBERRY_APPLY_LIVE, "0", power_led_dim_choices, nullptr),
		get_power_led_dim, set_power_led_dim, nullptr, nullptr, nullptr, nullptr},
	BOOL_DEF("amiberry.active_capture_automatically", "misc", "Capture mouse when the window is activated",
		"Capture the mouse when the emulator window is clicked.",
		AMIBERRY_APPLY_LIVE, "true", capture_always),
	BOOL_DEF("debug_mem", "misc", "Debug memory space",
		"Add a memory space for debugging tools.",
		AMIBERRY_APPLY_RESTART, "false", debug_mem),
	BOOL_DEF("cpu_halt_auto_reset", "misc", "Force hard reset if CPU halted",
		"Hard-reset the Amiga when its CPU halts.",
		AMIBERRY_APPLY_RESTART, "false", crash_auto_reset),
	BOOL_DEF("scsidevice_disable", "misc", "A600/A1200/A4000 IDE scsi.device disable",
		"Disable the scsi.device of the built-in IDE controller on these models.",
		AMIBERRY_APPLY_RESTART, "false", scsidevicedisable),
	BOOL_DEF("warpboot", "misc", "Warp mode reset",
		"Run at warp speed while the Amiga boots after a reset.",
		AMIBERRY_APPLY_LIVE, "false", turbo_boot),
	BOOL_DEF("amiberry.alt_tab_release", "misc", "Alt-Tab releases control",
		"Leave Alt-Tab to the host and release the mouse and keyboard when the focus moves.",
		AMIBERRY_APPLY_LIVE, "false", alt_tab_release),
	BOOL_DEF("amiberry.ctrl_alt_release", "misc", "Ctrl+Alt releases capture",
		"Release the mouse with Ctrl+Alt (for laptops without a middle button).",
		AMIBERRY_APPLY_LIVE, "true", ctrl_alt_release),
	BOOL_DEF("amiberry.use_retroarch_quit", "misc", "Use RetroArch quit button",
		"Quit with the controller's RetroArch quit button combination.",
		AMIBERRY_APPLY_LIVE, "true", use_retroarch_quit),
	BOOL_DEF("amiberry.use_retroarch_menu", "misc", "Use RetroArch menu button",
		"Open the settings with the controller's RetroArch menu button combination.",
		AMIBERRY_APPLY_LIVE, "true", use_retroarch_menu),
	BOOL_DEF("amiberry.use_retroarch_reset", "misc", "Use RetroArch reset button",
		"Reset with the controller's RetroArch reset button combination.",
		AMIBERRY_APPLY_LIVE, "false", use_retroarch_reset),
	HOTKEY_DEF("amiberry.open_gui", "Open settings",
		"Key or controller button that opens the settings; empty uses F12.", "F12", open_gui),
	HOTKEY_DEF("amiberry.quit_amiberry", "Quit",
		"Key or controller button that quits the emulator; empty unbinds it.", "", quit_amiberry),
	HOTKEY_DEF("amiberry.action_replay", "Action Replay",
		"Key that activates the Action Replay cartridge; empty uses Pause.", "Pause", action_replay),
	HOTKEY_DEF("amiberry.fullscreen_toggle", "Full-window",
		"Key that switches between window and full-window; empty unbinds it.", "", fullscreen_toggle),
	HOTKEY_DEF("amiberry.minimize", "Minimize",
		"Key that minimizes the emulator window; empty unbinds it.", "", minimize),
	HOTKEY_DEF("amiberry.left_amiga", "Left Amiga",
		"Key that acts as the left Amiga key; empty unbinds it.", "", left_amiga),
	HOTKEY_DEF("amiberry.right_amiga", "Right Amiga",
		"Key that acts as the right Amiga key; empty unbinds it.", "", right_amiga),
	HOTKEY_DEF("amiberry.screenshot", "Screenshot",
		"Key or controller button that saves a screenshot; empty unbinds it.", "", screenshot),
	HOTKEY_DEF("amiberry.debugger_trigger", "Debugger",
		"Key or controller button that opens the debugger; empty unbinds it.", "", debugger_trigger),
	{CHOICE_OPTION("amiberry.kbd_led_num", "misc", "NumLock LED",
			"The emulated LED the host keyboard's NumLock light shows.",
			AMIBERRY_APPLY_LIVE, "-1", led_choices, nullptr),
		get_kbd_led_num, set_kbd_led_num, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.kbd_led_scr", "misc", "ScrollLock LED",
			"The emulated LED the host keyboard's ScrollLock light shows.",
			AMIBERRY_APPLY_LIVE, "-1", led_choices, nullptr),
		get_kbd_led_scr, set_kbd_led_scr, nullptr, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.kbd_led_cap", "misc", "CapsLock LED",
			"The emulated LED the host keyboard's CapsLock light shows.",
			AMIBERRY_APPLY_LIVE, "-1", led_choices, nullptr),
		get_kbd_led_cap, set_kbd_led_cap, nullptr, nullptr, nullptr, nullptr},

	{CHOICE_OPTION("amiberry.active_priority", "priority", "Priority when active",
			"The emulator's process priority while its window is active.",
			AMIBERRY_APPLY_LIVE, "1", priority_choices, nullptr),
		get_active_capture_priority, set_active_capture_priority, nullptr, nullptr, nullptr, nullptr},
	BOOL_DEF("amiberry.active_not_captured_pause", "priority", "Pause when active but not captured",
		"Pause the emulation while its window is active but the mouse is not captured.",
		AMIBERRY_APPLY_LIVE, "false", active_nocapture_pause),
	BOOL_DEF("amiberry.active_not_captured_nosound", "priority", "No sound when active but not captured",
		"Mute the emulation while its window is active but the mouse is not captured.",
		AMIBERRY_APPLY_LIVE, "false", active_nocapture_nosound),
	{CHOICE_OPTION("amiberry.inactive_priority", "priority", "Priority when inactive",
			"The emulator's process priority while another window is active.",
			AMIBERRY_APPLY_LIVE, "0", priority_choices, nullptr),
		get_inactive_priority, set_inactive_priority, nullptr, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.inactive_pause", "priority", "Pause when inactive",
			"Pause the emulation while another window is active.",
			AMIBERRY_APPLY_LIVE, "false", inactive_constraint),
		get_inactive_pause, set_inactive_pause, inactive_options_available, nullptr, nullptr, nullptr},
	{BOOL_OPTION("amiberry.inactive_nosound", "priority", "No sound when inactive",
			"Mute the emulation while another window is active.",
			AMIBERRY_APPLY_LIVE, "false", inactive_constraint),
		get_inactive_nosound, set_inactive_nosound, inactive_options_available, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.inactive_input", "priority", "Input when inactive",
			"Whether controllers reach the emulation while another window is active.",
			AMIBERRY_APPLY_LIVE, "0", input_choices, inactive_constraint),
		get_inactive_input, set_inactive_input, inactive_options_available, nullptr, nullptr, nullptr},
	{CHOICE_OPTION("amiberry.minimized_priority", "priority", "Priority when minimized",
			"The emulator's process priority while its window is minimized.",
			AMIBERRY_APPLY_LIVE, "0", priority_choices, nullptr),
		get_minimized_priority, set_minimized_priority, nullptr, nullptr, nullptr, nullptr},
	BOOL_DEF("amiberry.minimized_pause", "priority", "Pause when minimized",
		"Pause the emulation while its window is minimized.",
		AMIBERRY_APPLY_LIVE, "true", minimized_pause),
	BOOL_DEF("amiberry.minimized_nosound", "priority", "No sound when minimized",
		"Mute the emulation while its window is minimized.",
		AMIBERRY_APPLY_LIVE, "true", minimized_nosound),
	{CHOICE_OPTION("amiberry.minimized_input", "priority", "Input when minimized",
			"Whether controllers reach the emulation while the window is minimized.",
			AMIBERRY_APPLY_LIVE, "0", input_choices, nullptr),
		get_minimized_input, set_minimized_input, nullptr, nullptr, nullptr, nullptr},
	{STRING_OPTION("config_description", "config", "Description",
			"A description of the configuration, shown with it in the configuration list.",
			AMIBERRY_APPLY_LIVE, "UAE default configuration", nullptr),
		get_description, set_description, nullptr, nullptr, nullptr, fits_description},
	{STRING_OPTION("config_category", "config", "Category",
			"The group the configuration list shows the configuration in; empty for none.",
			AMIBERRY_APPLY_LIVE, "", nullptr),
		get_category, set_category, nullptr, nullptr, nullptr, fits_category},
};

#undef BOOL_DEF
#undef HOTKEY_DEF

bool commit_misc(const uae_prefs& base, const uae_prefs& from, uae_prefs& to, const commit_info&)
{
	bool wrote = false;
#define FIELD(f) wrote |= commit_field(base.f, from.f, to.f)
	FIELD(start_gui);
	FIELD(main_alwaysontop);
	FIELD(gui_alwaysontop);
	FIELD(tod_hack);
	FIELD(reset_delay);
	FIELD(clipboard_sharing);
	FIELD(native_code);
	FIELD(illegal_mem);
	FIELD(blankmonitors);
	FIELD(start_uncaptured);
	FIELD(start_minimized);
	FIELD(minimize_inactive);
	FIELD(floppy_read_only);
	FIELD(harddrive_read_only);
	FIELD(uae_hide_autoconfig);
	FIELD(right_control_is_right_win_key);
	FIELD(power_led_dim);
	FIELD(capture_always);
	FIELD(debug_mem);
	FIELD(crash_auto_reset);
	FIELD(scsidevicedisable);
	FIELD(turbo_boot);
	FIELD(alt_tab_release);
	FIELD(ctrl_alt_release);
	FIELD(use_retroarch_quit);
	FIELD(use_retroarch_menu);
	FIELD(use_retroarch_reset);
	FIELD(description);
	FIELD(category);
	FIELD(open_gui);
	FIELD(quit_amiberry);
	FIELD(action_replay);
	FIELD(fullscreen_toggle);
	FIELD(minimize);
	FIELD(left_amiga);
	FIELD(right_amiga);
	FIELD(screenshot);
	FIELD(debugger_trigger);
	FIELD(kbd_led_num);
	FIELD(kbd_led_scr);
	FIELD(kbd_led_cap);
	FIELD(active_capture_priority);
	FIELD(active_nocapture_pause);
	FIELD(active_nocapture_nosound);
	FIELD(inactive_priority);
	FIELD(inactive_pause);
	FIELD(inactive_nosound);
	FIELD(minimized_priority);
	FIELD(minimized_pause);
	FIELD(minimized_nosound);
#undef FIELD
	// The status line bits are shared with other fields of leds_on_screen
	// only through these two options; inactive_input and minimized_input
	// keep bits other than 4 from a configuration.
	if (const int changed = (base.leds_on_screen ^ from.leds_on_screen) & (STATUSLINE_CHIPSET | STATUSLINE_RTG)) {
		const int merged = (to.leds_on_screen & ~changed) | (from.leds_on_screen & changed);
		wrote |= merged != to.leds_on_screen;
		to.leds_on_screen = merged;
	}
	wrote |= commit_field(base.inactive_input, from.inactive_input, to.inactive_input);
	wrote |= commit_field(base.minimized_input, from.minimized_input, to.minimized_input);
	return wrote;
}

const domain misc = {
	definitions, sizeof definitions / sizeof definitions[0], commit_misc, nullptr,
};

} // namespace

const domain& misc_domain()
{
	return misc;
}

} // namespace amiberry_settings
