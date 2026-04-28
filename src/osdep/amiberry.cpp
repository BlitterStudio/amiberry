/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "sysdeps.h"
#include "options.h"
#include "audio.h"
#include "sounddep/sound.h"
#include "uae.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "newcpu.h"
#include "traps.h"
#include "xwin.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "drawing.h"
#include "amiberry_gfx.h"
#include "irenderer.h"
#include "autoconf.h"
#include "gui.h"
#include "disk.h"
#include "savestate.h"
#include "zfile.h"
#include "rtgmodes.h"
#include "gfxboard.h"
#include "devices.h"
#include <map>
#include "ioport.h"
#include <parser.h>
#include <sstream>

#include "amiberry_input.h"
#include "amiberry_update.h"
#include "clipboard.h"
#include "dpi_handler.hpp"
#include "fsdb.h"
#include "scsidev.h"
#ifdef FLOPPYBRIDGE
#include "floppybridge_lib.h"
#endif
#include "registry.h"
#include "threaddep/thread.h"
#include "uae/uae.h"
#include "sana2.h"
#include "gui_handling_platform.h"
#include "gui/gui_handling.h"
#include "on_screen_joystick.h"
#include "imgui_osk.h"
#include "macos_bookmarks.h"
#include <mutex>
#if !defined(LIBRETRO) && defined(AMIBERRY_HAS_CURL)
#include <curl/curl.h>
#endif

#ifdef __MACH__
#include <string>
#include <mach-o/dyld.h>
#include <CoreFoundation/CoreFoundation.h>
#ifdef AMIBERRY_MACOS
#include <DiskArbitration/DiskArbitration.h>
#endif
#endif

#ifdef AHI
#include "ahi_v1.h"
#include "sana2.h"
#include "ethernet.h"

#ifdef AHI_v2
#include "ahi_v2.h"
#endif
#endif

#ifdef USE_GPIOD
#include <gpiod.h>
struct gpiod_chip* chip;
#if defined(GPIOD_VERSION_MAJOR) && GPIOD_VERSION_MAJOR >= 2
struct gpiod_line_request* gpio_request;
#else
const char* chipname = "gpiochip0";
struct gpiod_line* lineRed;    // Red LED
struct gpiod_line* lineGreen;  // Green LED
struct gpiod_line* lineYellow; // Yellow LED
#endif
#endif

#ifdef USE_DBUS
#include "amiberry_dbus.h"
#endif
#ifdef USE_IPC_SOCKET
#include "amiberry_ipc.h"
#endif

extern int run_jit_selftest_cli(void);
extern int console_logging;

static SDL_ThreadID mainthreadid;
static int logging_started;
int log_scsi;
int log_net;
int log_vsync, debug_vsync_min_delay, debug_vsync_forced_delay;
int uaelib_debug;
int pissoff_value = 15000 * CYCLE_UNIT;
int pissoff_nojit_value = 160 * CYCLE_UNIT;
int multithread_enabled = 1;

static TCHAR* inipath = nullptr;
extern FILE* debugfile;
static int forceroms;
static void* tablet;
SDL_Cursor* normalcursor;

TCHAR VersionStr[256];
TCHAR BetaStr[64];

int paraport_mask;

int pause_emulation;

static int sound_closed;
static int recapture;
static int focus;
static int mouseinside;
static int pending_mousecapture_monid = -1;
static int pending_mousecapture_active;
int mouseactive;
int mouse_monid;
int minimized;
int monitor_off;

static SDL_Condition* cpu_wakeup_event;
static SDL_Mutex* cpu_wakeup_mutex;
static volatile bool cpu_wakeup_event_triggered;

int quickstart_model = 0;
int quickstart_conf = 0;
bool host_poweroff = false;
int relativepaths = 0;
int saveimageoriginalpath = 0;
int net_enumerated;
struct netdriverdata* ndd[MAX_TOTAL_NET_DEVICES + 1];

whdload_options whdload_prefs = {};
struct amiberry_options amiberry_options = {};
amiberry_gui_theme gui_theme = {};
amiberry_hotkey enter_gui_key;
SDL_GamepadButton enter_gui_button;
amiberry_hotkey quit_key;
amiberry_hotkey action_replay_key;
amiberry_hotkey fullscreen_key;
amiberry_hotkey minimize_key;
amiberry_hotkey left_amiga_key;
amiberry_hotkey right_amiga_key;
amiberry_hotkey vkbd_key;
SDL_GamepadButton vkbd_button;
amiberry_hotkey screenshot_key;
amiberry_hotkey debugger_key;

bool lctrl_pressed, rctrl_pressed, lalt_pressed, ralt_pressed, lshift_pressed, rshift_pressed, lgui_pressed, rgui_pressed;
bool mouse_grabbed = false;

void cap_fps(uint64_t start)
{
	const auto end = SDL_GetPerformanceCounter();
	const auto elapsed_ms = static_cast<float>(end - start) / static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.0f;

	const int refresh_rate = std::clamp(static_cast<int>(sdl_mode.refresh_rate), 50, 60);
	const float frame_time = 1000.0f / static_cast<float>(refresh_rate);
	const float delay_time = frame_time - elapsed_ms;

	if (delay_time > 0.0f)
		SDL_Delay(static_cast<uint32_t>(delay_time));
}

std::string get_version_string()
{
	return VersionStr;
}

std::string get_copyright_notice()
{
	return COPYRIGHT;
}

std::string get_sdl_version_string()
{
	const int version = SDL_GetVersion();
	const int major = SDL_VERSIONNUM_MAJOR(version);
	const int minor = SDL_VERSIONNUM_MINOR(version);
	const int micro = SDL_VERSIONNUM_MICRO(version);
	std::ostringstream sdl_version;
	sdl_version << "SDL " << major << "." << minor << "." << micro;
	return sdl_version.str();
}

std::vector<int> parse_color_string(const std::string& input)
{
	std::vector<int> result;
	std::stringstream ss(input);
	std::string token;
	while (std::getline(ss, token, ',')) {
		result.push_back(std::stoi(token));
	}
	return result;
}

amiberry_hotkey get_hotkey_from_config(const std::string& config_option)
{
	amiberry_hotkey hotkey = {};

	if (config_option.empty()) return hotkey;

	// Use static lookup tables to avoid repeated string allocations and comparisons
	static const std::unordered_map<std::string, std::function<void(hotkey_modifiers&)>> modifier_map = {
		{"LCtrl", [](hotkey_modifiers& m) { m.lctrl = true; }},
		{"RCtrl", [](hotkey_modifiers& m) { m.rctrl = true; }},
		{"LAlt", [](hotkey_modifiers& m) { m.lalt = true; }},
		{"RAlt", [](hotkey_modifiers& m) { m.ralt = true; }},
		{"LShift", [](hotkey_modifiers& m) { m.lshift = true; }},
		{"RShift", [](hotkey_modifiers& m) { m.rshift = true; }},
		{"LGUI", [](hotkey_modifiers& m) { m.lgui = true; }},
		{"RGUI", [](hotkey_modifiers& m) { m.rgui = true; }}
	};

	static const std::unordered_map<std::string, const char*> sdl_key_name_map = {
		// Modifier keys (ImGui short names → SDL names)
		{"LCtrl", "Left Ctrl"},
		{"RCtrl", "Right Ctrl"},
		{"LAlt", "Left Alt"},
		{"RAlt", "Right Alt"},
		{"LShift", "Left Shift"},
		{"RShift", "Right Shift"},
		{"LGUI", "Left GUI"},
		{"RGUI", "Right GUI"},
		// ImGui full names → SDL names (from ImGui::GetKeyName)
		{"LeftCtrl", "Left Ctrl"},
		{"RightCtrl", "Right Ctrl"},
		{"LeftAlt", "Left Alt"},
		{"RightAlt", "Right Alt"},
		{"LeftShift", "Left Shift"},
		{"RightShift", "Right Shift"},
		{"LeftSuper", "Left GUI"},
		{"RightSuper", "Right GUI"},
		// Arrow keys
		{"LeftArrow", "Left"},
		{"RightArrow", "Right"},
		{"UpArrow", "Up"},
		{"DownArrow", "Down"},
		// Punctuation / symbol keys
		{"Equal", "="},
		{"Minus", "-"},
		{"Comma", ","},
		{"Period", "."},
		{"Slash", "/"},
		{"Backslash", "\\"},
		{"Semicolon", ";"},
		{"Apostrophe", "'"},
		{"GraveAccent", "`"},
		{"LeftBracket", "["},
		{"RightBracket", "]"},
		// Misc keys
		{"Enter", "Return"},
		{"NumLock", "Numlock"},
		{"ScrollLock", "ScrollLock"},
		{"PrintScreen", "PrintScreen"},
	};

	// Parse tokens more efficiently without modifying the original string
	size_t start = 0;
	size_t pos = 0;
	constexpr char delimiter = '+';

	while ((pos = config_option.find(delimiter, start)) != std::string::npos)
	{
		const std::string token = config_option.substr(start, pos - start);

		// Use lookup table for O(1) modifier detection
		const auto it = modifier_map.find(token);
		if (it != modifier_map.end()) {
			it->second(hotkey.modifiers);
		}

		start = pos + 1;
	}

	// The remaining part is the key name
	hotkey.key_name = config_option.substr(start);

	// Use lookup table for SDL name mapping
	const auto sdl_it = sdl_key_name_map.find(hotkey.key_name);
	const char* sdl_key_name = (sdl_it != sdl_key_name_map.end()) ? sdl_it->second : hotkey.key_name.c_str();

	hotkey.scancode = SDL_GetScancodeFromName(sdl_key_name);
	hotkey.button = SDL_GetGamepadButtonFromString(hotkey.key_name.c_str());
	
	return hotkey;
}

static void set_key_configs(const uae_prefs* p)
{
	if (strncmp(p->open_gui, "", 1) != 0)
		// If we have a value in the config, we use that instead
		enter_gui_key = get_hotkey_from_config(p->open_gui);
	else
		// Otherwise we go for the default found in amiberry.conf
		enter_gui_key = get_hotkey_from_config(amiberry_options.default_open_gui_key);
	// if nothing was found in amiberry.conf either, we default back to F12
	if (enter_gui_key.scancode == 0)
		enter_gui_key.scancode = SDL_SCANCODE_F12;

	enter_gui_button = SDL_GetGamepadButtonFromString(p->open_gui);
	if (enter_gui_button == SDL_GAMEPAD_BUTTON_INVALID)
		enter_gui_button = SDL_GetGamepadButtonFromString(amiberry_options.default_open_gui_key);
#ifdef __ANDROID__
	// Android: default Start button as pause/GUI toggle for hardware controllers
	// (no F12 key available, and software back button isn't accessible from gamepads)
	if (enter_gui_button == SDL_GAMEPAD_BUTTON_INVALID)
		enter_gui_button = SDL_GAMEPAD_BUTTON_START;
#endif
	if (enter_gui_button != SDL_GAMEPAD_BUTTON_INVALID)
	{
		for (int port = 0; port < 2; port++)
		{
			const auto host_joy_id = p->jports[port].id - JSEM_JOYS;
			if (host_joy_id >= 0 && host_joy_id < MAX_INPUT_DEVICES)
			{
				didata* did = &di_joystick[host_joy_id];
				did->mapping.menu_button = enter_gui_button;
			}
		}
	}
	
	if (strncmp(p->quit_amiberry, "", 1) != 0)
		quit_key = get_hotkey_from_config(p->quit_amiberry);
	else
		quit_key = get_hotkey_from_config(amiberry_options.default_quit_key);

	if (strncmp(p->action_replay, "", 1) != 0)
		action_replay_key = get_hotkey_from_config(p->action_replay);
	else
		action_replay_key = get_hotkey_from_config(amiberry_options.default_ar_key);
	if (action_replay_key.scancode == 0)
		action_replay_key.scancode = SDL_SCANCODE_PAUSE;

	if (strncmp(p->fullscreen_toggle, "", 1) != 0)
		fullscreen_key = get_hotkey_from_config(p->fullscreen_toggle);
	else
		fullscreen_key = get_hotkey_from_config(amiberry_options.default_fullscreen_toggle_key);

	minimize_key = get_hotkey_from_config(p->minimize);
	left_amiga_key = get_hotkey_from_config(p->left_amiga);
	right_amiga_key = get_hotkey_from_config(p->right_amiga);
	screenshot_key = get_hotkey_from_config(p->screenshot);

	debugger_key = get_hotkey_from_config(p->debugger_trigger);

	if (strncmp(p->vkbd_toggle, "", 1) != 0)
		vkbd_key = get_hotkey_from_config(p->vkbd_toggle);
	else
		vkbd_key = get_hotkey_from_config(amiberry_options.default_vkbd_toggle);

	vkbd_button = SDL_GetGamepadButtonFromString(p->vkbd_toggle);
	if (vkbd_button == SDL_GAMEPAD_BUTTON_INVALID)
		vkbd_button = SDL_GetGamepadButtonFromString(amiberry_options.default_vkbd_toggle);
	if (vkbd_button != SDL_GAMEPAD_BUTTON_INVALID)
	{
		for (int port = 0; port < 2; port++)
		{
			const auto host_joy_id = p->jports[port].id - JSEM_JOYS;
			if (host_joy_id >= 0 && host_joy_id < MAX_INPUT_DEVICES)
			{
				didata* did = &di_joystick[host_joy_id];
				did->mapping.vkbd_button = vkbd_button;
			}
		}
	}
}

#ifndef _WIN32
extern void signal_segv(int signum, siginfo_t* info, void* ptr);
extern void signal_buserror(int signum, siginfo_t* info, void* ptr);
extern void signal_term(int signum, siginfo_t* info, void* ptr);
#endif

extern void set_last_active_config(const char* filename);

std::string home_dir;
std::string current_dir;
static bool g_portable_mode = false;

#if defined(__linux__)
#include <linux/kd.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif
#ifndef _WIN32
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#endif
unsigned char kbd_led_status;
char kbd_flags;
int led_console_fd = -1;

#if defined(__linux__) && !defined(__ANDROID__) && !defined(LIBRETRO)
static void open_led_console(void)
{
	if (led_console_fd >= 0)
		return;

	static const char* const candidates[] = { "/dev/tty", "/dev/tty0", "/dev/console" };
	int last_errno = 0;
	for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
		const int fd = open(candidates[i], O_RDWR | O_NOCTTY | O_CLOEXEC);
		if (fd < 0) {
			last_errno = errno;
			continue;
		}
		unsigned char probe = 0;
		if (ioctl(fd, KDGETLED, &probe) == 0) {
			led_console_fd = fd;
			write_log(_T("Keyboard LEDs: using %s for KD ioctls\n"), candidates[i]);
			return;
		}
		last_errno = errno;
		close(fd);
	}
	write_log(_T("Keyboard LEDs: no usable console TTY (errno=%d), LED indicators disabled\n"), last_errno);
}

static void close_led_console(void)
{
	if (led_console_fd >= 0) {
		ioctl(led_console_fd, KDSETLED, 0xFF);
		close(led_console_fd);
		led_console_fd = -1;
	}
}
#else
static inline void open_led_console(void) {}
static inline void close_led_console(void) {}
#endif

#include "amiberry_platform_internal.h"

std::string config_path;
std::string base_content_path;
std::string rom_path;
std::string rp9_path;
std::string controllers_path;
std::string retroarch_file;
std::string whdboot_path;
std::string whdload_arch_path;
std::string floppy_path;
std::string harddrive_path;
std::string cdrom_path;
std::string logfile_path;
std::string floppy_sounds_dir;
std::string data_dir;
std::string saveimage_dir;
std::string savestate_dir;
std::string ripper_path;
std::string input_dir;
std::string screenshot_dir;
std::string nvram_dir;
std::string plugins_dir;
std::string video_dir;
std::string themes_path;
std::string shaders_path;
std::string bezels_path;
std::string settings_dir;
std::string amiberry_conf_file;
std::string amiberry_ini_file;
static std::string resolved_settings_file;
static bool amiberry_conf_file_overridden_from_cli = false;
static bool suppress_runtime_path_side_effects = false;
struct legacy_migration_state_summary
{
	bool config_migrated{};
	bool config_failed{};
	bool state_migrated{};
	bool state_failed{};
	bool visuals_migrated{};
	bool visuals_failed{};
	bool visuals_conflicts{};
	bool settings_rewrite_needed{};

	bool any_migrated() const
	{
		return config_migrated || state_migrated || visuals_migrated;
	}

	bool any_failures() const
	{
		return config_failed || state_failed || visuals_failed || visuals_conflicts;
	}

	bool any_bootstrap_files_migrated() const
	{
		return config_migrated || state_migrated;
	}

	bool any_bootstrap_failures() const
	{
		return config_failed || state_failed;
	}
};

enum class settings_resolution_source
{
	default_paths_only,
	settings_dir,
	legacy_settings,
	cli_override,
};

static legacy_migration_state_summary legacy_migration_state;
static bool startup_migration_notice_pending = false;
static settings_resolution_source resolved_settings_source = settings_resolution_source::default_paths_only;

struct legacy_cleanup_item
{
	std::string label;
	std::string path;
	bool is_directory{};
};

static std::vector<legacy_cleanup_item> legacy_cleanup_items;
static bool legacy_cleanup_prompt_enabled = false;

char last_loaded_config[MAX_DPATH] = {};
char last_active_config[MAX_DPATH] = {};

int max_uae_width;
int max_uae_height;

extern "C" int main(int argc, char* argv[]);
static void init_amiberry_dirs(bool portable_mode, bool materialize_host_roots = true);
void save_amiberry_settings();

static const char* get_settings_resolution_source_name(const settings_resolution_source source)
{
	switch (source)
	{
	case settings_resolution_source::settings_dir:
		return "settings_dir";
	case settings_resolution_source::legacy_settings:
		return "legacy_settings";
	case settings_resolution_source::cli_override:
		return "cli_override";
	case settings_resolution_source::default_paths_only:
	default:
		return "default_paths_only";
	}
}

static std::string describe_bootstrap_migration_subjects(const bool config, const bool state)
{
	if (config && state)
		return "existing settings and state data";
	if (config)
		return "existing settings";
	if (state)
		return "existing state data";
	return {};
}

static std::string describe_layout_migration_subjects(const legacy_migration_state_summary& state)
{
	const auto bootstrap_subjects =
		describe_bootstrap_migration_subjects(state.config_migrated || state.config_failed,
			state.state_migrated || state.state_failed);
	const bool include_visuals = state.visuals_migrated || state.visuals_failed || state.visuals_conflicts;

	if (!bootstrap_subjects.empty() && include_visuals)
		return bootstrap_subjects + " and compatible visual assets";
	if (!bootstrap_subjects.empty())
		return bootstrap_subjects;
	if (include_visuals)
		return "compatible visual assets";
	return "legacy files";
}

static std::string get_bootstrap_destination_label(const legacy_migration_state_summary& state)
{
	if (state.config_migrated || state.config_failed)
	{
		if (state.state_migrated || state.state_failed)
			return "Bootstrap settings and state now live in:\n\n  ";
		return "Bootstrap settings now live in:\n\n  ";
	}
	if (state.state_migrated || state.state_failed)
		return "Bootstrap state now lives in:\n\n  ";
	return {};
}

void set_last_loaded_config(const char* filename)
{
	extract_filename(filename, last_loaded_config);
	remove_file_extension(last_loaded_config);
}

void set_last_active_config(const char* filename)
{
	extract_filename(filename, last_active_config);
	remove_file_extension(last_active_config);
}

int getdpiformonitor(SDL_DisplayID displayID)
{
	float scale = SDL_GetDisplayContentScale(displayID);
	if (scale > 0.0f) {
		return static_cast<int>(96.0f * scale);
	}
	return 96; // Common default DPI value
}

int getdpiforwindow(const int monid)
{
	SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
	if (AMonitors[monid].amiga_window) {
		SDL_DisplayID winDisplay = SDL_GetDisplayForWindow(AMonitors[monid].amiga_window);
		if (winDisplay) displayID = winDisplay;
	}
	return getdpiformonitor(displayID);
}

uae_s64 spincount;
extern bool calculated_scanline;

void target_spin(int total)
{
	if (!spincount || calculated_scanline)
		return;
	total = std::min(total, 10);
	while (total-- >= 0) {
		uae_s64 v1 = read_processor_time_rdtsc();
		v1 += spincount;
		while (v1 > read_processor_time_rdtsc());
	}
}

extern int vsync_activeheight;

void target_calibrate_spin()
{
	struct amigadisplay* ad = &adisplays[0];
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	int vp;
	const int cntlines = 1;
	uae_u64 sc;

	spincount = 0;
	if (!ap->gfx_vsyncmode)
		return;
	extern bool calculated_scanline;
	if (calculated_scanline) {
		write_log(_T("target_calibrate_spin() skipped (%d)\n"), calculated_scanline);
		return;
	}
	write_log(_T("target_calibrate_spin() start (%d)\n"), calculated_scanline);
	sc = 0x800000000000LL;
	for (int i = 0; i < 50; i++) {
		for (;;) {
			vp = target_get_display_scanline(-1);
			if (vp <= -10)
				goto fail;
			if (vp >= 1 && vp < vsync_activeheight - 10)
				break;
		}
		uae_u64 v1;
		int vp2;
		for (;;) {
			v1 = read_processor_time_rdtsc();
			vp2 = target_get_display_scanline(-1);
			if (vp2 <= -10)
				goto fail;
			if (vp2 == vp + cntlines)
				break;
			if (vp2 < vp || vp2 > vp + cntlines)
				goto trynext;
		}
		for (;;) {
			int vp2 = target_get_display_scanline(-1);
			if (vp2 <= -10)
				goto fail;
			if (vp2 == vp + cntlines * 2) {
				uae_u64 scd = (read_processor_time_rdtsc() - v1) / cntlines;
				if (sc > scd)
					sc = scd;
			}
			if (vp2 < vp)
				break;
			if (vp2 > vp + cntlines * 2)
				break;
		}
trynext:;
	}
	if (sc == 0x800000000000LL) {
		write_log(_T("Spincount calculation error, spinloop not used.\n"));
		spincount = 0;
	} else {
		spincount = sc;
		write_log(_T("Spincount = %llu\n"), sc);
	}
	return;
fail:
	write_log(_T("Scanline read failed: %d!\n"), vp);
	spincount = 0;
}

static int init_mmtimer()
{
	cpu_wakeup_event = SDL_CreateCondition();
	cpu_wakeup_mutex = SDL_CreateMutex();
	if (!cpu_wakeup_event || !cpu_wakeup_mutex) {
		write_log(_T("Failed to create CPU wakeup event/mutex\n"));
		return 0;
	}
	return 1;
}

void sleep_cpu_wakeup()
{
	SDL_LockMutex(cpu_wakeup_mutex);
	if (!cpu_wakeup_event_triggered) {
		cpu_wakeup_event_triggered = true;
		SDL_SignalCondition(cpu_wakeup_event);
	}
	SDL_UnlockMutex(cpu_wakeup_mutex);
}

int get_sound_event();

static int sleep_millis2(int ms, const bool main)
{
	frame_time_t start = 0;

	if (ms < 0)
		ms = -ms;
	if (main) {
		SDL_LockMutex(cpu_wakeup_mutex);
		if (cpu_wakeup_event_triggered) {
			cpu_wakeup_event_triggered = false;
			SDL_UnlockMutex(cpu_wakeup_mutex);
			return 0;
		}
		SDL_UnlockMutex(cpu_wakeup_mutex);
		start = read_processor_time();

		SDL_Delay(ms);
		SDL_LockMutex(cpu_wakeup_mutex);
		cpu_wakeup_event_triggered = false;
		SDL_UnlockMutex(cpu_wakeup_mutex);
	}
	else {
		SDL_Delay(ms);
	}

	if (main)
		idletime += read_processor_time() - start;
	return 0;
}

int busywait;

int target_sleep_nanos(int us)
{
	if (us < 0)
		return 800; // Microseconds threshold

	struct timespec req;
	req.tv_sec = us / 1000000;
	req.tv_nsec = (us % 1000000) * 1000;
	nanosleep(&req, nullptr);
	return 0;
}

void sleep_micros(const int ms)
{
	usleep(ms);
}

int sleep_millis_main(const int ms)
{
	return sleep_millis2(ms, true);
}
int sleep_millis(const int ms)
{
	return sleep_millis2(ms, false);
}

static void setcursor(AmigaMonitor* mon, int oldx, int oldy)
{
	const int dx = (mon->amigawinclip_rect.x - mon->amigawin_rect.x) + ((mon->amigawinclip_rect.x + mon->amigawinclip_rect.w) - mon->amigawinclip_rect.x) / 2;
	const int dy = (mon->amigawinclip_rect.y - mon->amigawin_rect.y) + ((mon->amigawinclip_rect.y + mon->amigawinclip_rect.h) - mon->amigawinclip_rect.y) / 2;
	mon->mouseposx = oldx - dx;
	mon->mouseposy = oldy - dy;

	mon->windowmouse_max_w = mon->amigawinclip_rect.w / 2 - 50;
	mon->windowmouse_max_h = mon->amigawinclip_rect.h / 2 - 50;
	mon->windowmouse_max_w = std::max(mon->windowmouse_max_w, 10);
	mon->windowmouse_max_h = std::max(mon->windowmouse_max_h, 10);

	if ((currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) && currprefs.input_tablet > 0 && mousehack_alive() && isfullscreen() <= 0) {
		mon->mouseposx = mon->mouseposy = 0;
		return;
	}

	if (oldx >= 30000 || oldy >= 30000 || oldx <= -30000 || oldy <= -30000) {
		oldx = oldy = 0;
	} else {
		if (abs(mon->mouseposx) < mon->windowmouse_max_w && abs(mon->mouseposy) < mon->windowmouse_max_h)
			return;
	}
	mon->mouseposx = mon->mouseposy = 0;
	if (oldx < 0 || oldy < 0 || oldx > mon->amigawin_rect.w || oldy > mon->amigawin_rect.h) {
		write_log(_T("Mouse out of range: mon=%d %dx%d (%dx%d %dx%d)\n"), mon->monitor_id, oldx, oldy,
			mon->amigawin_rect.x, mon->amigawin_rect.y, mon->amigawin_rect.w, mon->amigawin_rect.h);
		return;
	}
	const int cx = mon->amigawinclip_rect.w / 2 + mon->amigawin_rect.x + (mon->amigawinclip_rect.x - mon->amigawin_rect.x);
	const int cy = mon->amigawinclip_rect.h / 2 + mon->amigawin_rect.y + (mon->amigawinclip_rect.y - mon->amigawin_rect.y);

	SDL_WarpMouseGlobal(cx, cy);
}

static int mon_cursorclipped;
static int pausemouseactive;
void resumesoundpaused()
{
	resume_sound();
#ifdef AHI
	ahi_open_sound();
#ifdef AHI_v2
	ahi2_pause_sound(0);
#endif
#endif
}

void setsoundpaused()
{
	pause_sound();
#ifdef AHI
	ahi_close_sound();
#ifdef AHI_v2
	ahi2_pause_sound(1);
#endif
#endif
}

bool resumepaused(const int priority)
{
	const AmigaMonitor* mon = &AMonitors[0];
	if (pause_emulation > priority)
		return false;
	if (!pause_emulation)
		return false;
	devices_unpause();
	resumesoundpaused();
	if (pausemouseactive)
	{
		pausemouseactive = 0;
		// In Amiberry, we'll do this for Full Window and Fullscreen both.
		// Otherwise, KMSDRM did not get the focus after resuming from the GUI
		setmouseactive(mon->monitor_id, isfullscreen() != 0 ? 1 : -1);
	}
	pause_emulation = 0;
	setsystime();
	wait_keyrelease();
	return true;
}

bool setpaused(const int priority)
{
	const AmigaMonitor* mon = &AMonitors[0];
	if (pause_emulation > priority)
		return false;
	if (!pause_emulation) {
		wait_keyrelease();
	}
	pause_emulation = priority;
	devices_pause();
	setsoundpaused();
	pausemouseactive = 1;
	if (isfullscreen() <= 0) {
		pausemouseactive = mouseactive;
		setmouseactive(mon->monitor_id, 0);
	}
	return true;
}

void setminimized(const int monid)
{
	if (!minimized)
		minimized = 1;
}

void unsetminimized(const int monid)
{
	if (minimized < 0)
		gfx_DisplayChangeRequested(2);
	else if (minimized > 0)
		full_redraw_all();
	minimized = 0;
}

void refreshtitle()
{
	//for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
	//	struct AmigaMonitor* mon = &AMonitors[i];
	//	if (mon->sdl_window && isfullscreen() == 0) {
	//		setmaintitle(mon->monitor_id);
	//	}
	//}
}

void setpriority(const int prio)
{
	if (prio >= 0 && prio <= 2)
	{
		switch (prio)
		{
		case 0:
			SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_LOW);
			break;
		case 1:
			SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
			break;
		case 2:
			SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
			break;
		default:
			break;
		}
	}
}

static void setcursorshape(const int monid)
{
	const AmigaMonitor* mon = &AMonitors[monid];
	if (currprefs.input_tablet && currprefs.input_magic_mouse_cursor == MAGICMOUSE_NATIVE_ONLY) {
		SDL_HideCursor();
	}
	else if (!picasso_setwincursor(monid)) {
		SDL_SetCursor(normalcursor);
		// When the mouse is captured in relative mode the OS pointer must stay
		// hidden; SDL hides it internally, but on Windows the explicit
		// HideCursor call keeps SDL's cursor-visibility intent in sync so the
		// Windows pointer does not remain drawn alongside the Amiga pointer
		// after re-grabbing (issue #1969).
		if (mouseactive && !currprefs.input_tablet) {
			SDL_HideCursor();
		} else {
			SDL_ShowCursor();
		}
	}
}

void set_showcursor(const BOOL v)
{
	if (v) {
		SDL_ShowCursor();
	}
	else {
		SDL_HideCursor();
	}
}

void updatemouseclip(AmigaMonitor* mon);
void updatewinrect(AmigaMonitor* mon, const bool allowfullscreen);
void releasecapture(const AmigaMonitor* mon);

static void clear_pending_mouse_capture()
{
	pending_mousecapture_monid = -1;
	pending_mousecapture_active = 0;
}

static void queue_pending_mouse_capture(const int monid, const int active)
{
	if (pending_mousecapture_monid != monid || pending_mousecapture_active != active) {
		write_log("Deferring mouse capture until focus is gained on monitor %d (active=%d)\n", monid, active);
	}
	pending_mousecapture_monid = monid;
	pending_mousecapture_active = active;
}

static bool consume_pending_mouse_capture(const int monid, int* active)
{
	if (pending_mousecapture_monid != monid)
		return false;
	if (active)
		*active = pending_mousecapture_active;
	pending_mousecapture_monid = -1;
	pending_mousecapture_active = 0;
	return true;
}

#ifndef LIBRETRO
static bool apply_mouse_capture_grabs(AmigaMonitor* mon)
{
	const bool mouse_grab_ok = SDL_SetWindowMouseGrab(mon->amiga_window, true);
	if (!mouse_grab_ok) {
		write_log("SDL_SetWindowMouseGrab(true) failed on monitor %d: %s\n", mon->monitor_id, SDL_GetError());
	}
	// Skip keyboard grab when alt_tab_release is on so the
	// compositor handles Alt-Tab; focus-loss tears down capture
	// safely after pointer focus has already moved (see #1829).
	if (!SDL_SetWindowKeyboardGrab(mon->amiga_window, !currprefs.alt_tab_release)) {
		write_log("SDL_SetWindowKeyboardGrab(%d) failed on monitor %d: %s\n",
			currprefs.alt_tab_release ? 0 : 1, mon->monitor_id, SDL_GetError());
	}
	// SDL hides the cursor when Relative mode is enabled.
	// This means that the RTG hardware sprite will no longer be shown,
	// unless it's configured to use Virtual Mouse (absolute movement).
	bool relative_ok = true;
	if (!currprefs.input_tablet) {
		relative_ok = SDL_SetWindowRelativeMouseMode(mon->amiga_window, true);
		if (!relative_ok) {
			write_log("SDL_SetWindowRelativeMouseMode(true) failed on monitor %d: %s\n",
				mon->monitor_id, SDL_GetError());
		}
		// Keep the cursor-visibility intent synced with relative mode so a
		// previous SDL_ShowCursor() call (e.g. from releasecapture) doesn't
		// leave the OS pointer visible on top of the Amiga pointer after a
		// re-grab on Windows (issue #1969).
		SDL_HideCursor();
	}

	if (!mouse_grab_ok || !relative_ok) {
		releasecapture(mon);
		return false;
	}

	updatewinrect(mon, false);
	mon_cursorclipped = mon->monitor_id + 1;
	updatemouseclip(mon);
	setcursor(mon, -30000, -30000);
	return true;
}
#else
static bool apply_mouse_capture_grabs(AmigaMonitor*) { return true; }
#endif

void releasecapture(const AmigaMonitor* mon)
{
	SDL_SetWindowMouseGrab(mon->amiga_window, false);
	SDL_SetWindowKeyboardGrab(mon->amiga_window, false);
	SDL_SetWindowRelativeMouseMode(mon->amiga_window, false);
	set_showcursor(TRUE);
	mon_cursorclipped = 0;
}

static void restore_active_mouse_capture(AmigaMonitor* mon)
{
#ifndef LIBRETRO
	if (!mon || !mon->amiga_window)
		return;
	if (mouseactive != mon->monitor_id + 1 || !focus || currprefs.input_tablet)
		return;
	if (SDL_GetWindowRelativeMouseMode(mon->amiga_window))
		return;

	write_log("Restoring relative mouse capture on monitor %d\n", mon->monitor_id);
	if (!apply_mouse_capture_grabs(mon)) {
		write_log("Mouse capture restore failed on monitor %d, keeping window uncaptured\n", mon->monitor_id);
		focus = 0;
		mouseactive = 0;
		recapture = 0;
	}
#endif
}

void updatemouseclip(AmigaMonitor* mon)
{
	if (mon_cursorclipped) {
		mon->amigawinclip_rect = mon->amigawin_rect;
		if (!isfullscreen()) {
			GetWindowRect(mon->amiga_window, &mon->amigawinclip_rect);
			// Too small or invalid?
			if (mon->amigawinclip_rect.w <= mon->amigawinclip_rect.x + 7 || mon->amigawinclip_rect.h <= mon->amigawinclip_rect.y + 7)
				mon->amigawinclip_rect = mon->amigawin_rect;
		}
	}
}

void updatewinrect(AmigaMonitor* mon, const bool allowfullscreen)
{
	int f = isfullscreen();
	if (!allowfullscreen && f > 0)
		return;
	GetWindowRect(mon->amiga_window, &mon->amigawin_rect);
	GetWindowRect(mon->amiga_window, &mon->amigawinclip_rect);
#if MOUSECLIP_LOG
	write_log(_T("GetWindowRect mon=%d %dx%d %dx%d %d\n"), mon->monitor_id, mon->amigawin_rect.left, mon->amigawin_rect.top, mon->amigawin_rect.right, mon->amigawin_rect.bottom, f);
#endif
	if (f == 0 && mon->monitor_id == 0) {
		changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.x = mon->amigawin_rect.x;
		changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.y = mon->amigawin_rect.y;
		currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.x = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.x;
		currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.y = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.y;
	}
}

static bool iswindowfocus(const AmigaMonitor* mon)
{
	bool donotfocus = false;
	const SDL_WindowFlags flags = SDL_GetWindowFlags(mon->amiga_window);

	if (!(flags & SDL_WINDOW_INPUT_FOCUS)) {
		donotfocus = true;
	}

	if (isfullscreen() > 0)
		donotfocus = false;
	return donotfocus == false;
}

bool ismouseactive ()
{
	return mouseactive > 0;
}

void target_inputdevice_unacquire(const bool full)
{
	const AmigaMonitor* mon = &AMonitors[0];
	close_tablet(tablet);
	tablet = NULL;
	if (full) {
		SDL_SetWindowMouseGrab(mon->amiga_window, false);
		SDL_SetWindowKeyboardGrab(mon->amiga_window, false);
	}
}
void target_inputdevice_acquire()
{
	const AmigaMonitor* mon = &AMonitors[0];
	target_inputdevice_unacquire(false);
	tablet = open_tablet(mon->amiga_window);
	SDL_SetWindowMouseGrab(mon->amiga_window, true);
	SDL_SetWindowKeyboardGrab(mon->amiga_window, !currprefs.alt_tab_release);
}

static void setmouseactive2(AmigaMonitor* mon, int active, const bool allowpause)
{
#ifdef RETROPLATFORM
	bool isrp = rp_isactive() != 0;
#else
	bool isrp = false;
#endif
	if (osdep_platform_require_window_for_mouse() && !mon->amiga_window)
		return;
	const int lastmouseactive = mouseactive;

	if (active == 0)
		releasecapture(mon);
	if (mouseactive == active && active >= 0)
		return;

	if (!isrp && active == 1 && !(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		if (SDL_GetCursor() != normalcursor)
			return;
	}
	if (active) {
		if (!isrp && (SDL_GetWindowFlags(mon->amiga_window) & SDL_WINDOW_HIDDEN))
			return;
	}
	
	if (active < 0)
		active = 1;

	mouseactive = active ? mon->monitor_id + 1 : 0;

	mon->mouseposx = mon->mouseposy = 0;

	if (isfullscreen() <= 0 && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		if (currprefs.input_tablet > 0) {
			if (mousehack_alive()) {
				releasecapture(mon);
				recapture = 0;
				return;
			}
			SDL_SetCursor(normalcursor);
		} else {
			recapture = 0;
		}
	}

	if (mouseactive) {
		bool gotfocus = false;
		for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
			if (AMonitors[i].amiga_window && iswindowfocus(&AMonitors[i])) {
				gotfocus = true;
				mon = &AMonitors[i];
				break;
			}
		}
		if (!gotfocus) {
			write_log(_T("Tried to capture mouse but window didn't have focus! F=%d A=%d\n"), focus, mouseactive);
			focus = 0;
			mouseactive = 0;
			active = 0;
			releasecapture(mon);
			recapture = 0;
		}
	}

	if (mouseactive > 0)
		focus = mon->monitor_id + 1;

	if (mouseactive && focus) {
#if MOUSECLIP_HIDE
		set_showcursor(FALSE);
#endif
		if (!apply_mouse_capture_grabs(mon)) {
			write_log("Mouse capture activation failed on monitor %d, keeping window uncaptured\n", mon->monitor_id);
			focus = 0;
			mouseactive = 0;
			active = 0;
			recapture = 0;
		}
	}

	if (mouseactive) {
		if (lastmouseactive != mouseactive) {
			wait_keyrelease();
		}
		inputdevice_acquire(TRUE);
		setpriority(currprefs.active_capture_priority);
		if (currprefs.active_nocapture_pause)
			resumepaused(2);
		else if (currprefs.active_nocapture_nosound && sound_closed < 0)
			resumesoundpaused();
	}
	else {
		inputdevice_acquire(FALSE);
		inputdevice_releasebuttons();
	}
	if (!active && allowpause)
	{
		if (currprefs.active_nocapture_pause)
			setpaused(2);
		else if (currprefs.active_nocapture_nosound)
		{
			setsoundpaused();
			sound_closed = -1;
		}
	}
#ifdef RETROPLATFORM
	rp_mouse_capture(active);
	rp_mouse_magic(magicmouse_alive());
#endif
}

void setmouseactive(const int monid, const int active)
{
	AmigaMonitor* mon = &AMonitors[monid];
	monitor_off = 0;
	if (active <= 0)
		clear_pending_mouse_capture();
	if (osdep_platform_require_window_for_mouse() && !mon->amiga_window)
		return;
	if (active > 1)
		SDL_RaiseWindow(mon->amiga_window);
	if (active != 0 && !iswindowfocus(mon)) {
		queue_pending_mouse_capture(monid, active);
		return;
	}
	if (active != 0 && pending_mousecapture_monid == monid)
		clear_pending_mouse_capture();
	setmouseactive2(mon, active, true);
	setcursorshape(monid);
}

static void amiberry_active(const AmigaMonitor* mon, const int is_minimized)
{
	monitor_off = 0;
	
	focus = mon->monitor_id + 1;
	auto pri = currprefs.inactive_priority;
	if (!is_minimized)
		pri = currprefs.active_capture_priority;
	setpriority(pri);

	if (sound_closed != 0) {
		if (sound_closed < 0) {
			resumesoundpaused();
		} else {
			if (currprefs.active_nocapture_pause)
			{
				if (mouseactive)
					resumepaused(2);
			} else if (currprefs.minimized_pause && !currprefs.inactive_pause)
				resumepaused(1);
			else if (currprefs.inactive_pause)
				resumepaused(2);
		}
		sound_closed = 0;
	}
	getcapslock();
	wait_keyrelease();
	if (isfullscreen() > 0 || currprefs.capture_always) {
		setmouseactive(mon->monitor_id, 1);
		inputdevice_acquire(TRUE);
	}
	clipboard_active(1, 1);
}

static void amiberry_inactive(const AmigaMonitor* mon, const int is_minimized)
{
	focus = 0;
	recapture = 0;
	wait_keyrelease();
	setmouseactive(mon->monitor_id, 0);
	clipboard_active(1, 0);
	auto pri = currprefs.inactive_priority;
	if (!quit_program) {
		if (is_minimized) {
			pri = currprefs.minimized_priority;
			if (currprefs.minimized_pause) {
				inputdevice_unacquire();
				setpaused(1);
				sound_closed = 1;
			}
			else if (currprefs.minimized_nosound) {
				inputdevice_unacquire(currprefs.minimized_input);
				setsoundpaused();
				sound_closed = -1;
			}
			else {
				inputdevice_unacquire(currprefs.minimized_input);
			}
		}
		else if (mouseactive) {
			inputdevice_unacquire();
			if (currprefs.active_nocapture_pause)
			{
				setpaused(2);
				sound_closed = 1;
			}
			else if (currprefs.active_nocapture_nosound)
			{
				setsoundpaused();
				sound_closed = -1;
			}
		}
		else {
			if (currprefs.inactive_pause)
			{
				inputdevice_unacquire();
				setpaused(2);
				sound_closed = 1;
			}
			else if (currprefs.inactive_nosound)
			{
				inputdevice_unacquire(currprefs.inactive_input);
				setsoundpaused();
				sound_closed = -1;
			}
			else {
				inputdevice_unacquire(currprefs.inactive_input);
			}
		}
	}
	else {
		inputdevice_unacquire();
	}
	setpriority(pri);
#ifdef FILESYS
	filesys_flush_cache();
#endif
#ifdef LOGITECHLCD
	lcd_priority(0);
#endif
}

void minimizewindow(const int monid)
{
	const AmigaMonitor* mon = &AMonitors[monid];
	if (mon->amiga_window)
		SDL_MinimizeWindow(mon->amiga_window);
}

void enablecapture(const int monid)
{
	if (pause_emulation > 2)
		return;
	setmouseactive(monid, 1);
	if (sound_closed < 0) {
		resumesoundpaused();
		sound_closed = 0;
	}
	if (currprefs.inactive_pause || currprefs.active_nocapture_pause)
		resumepaused(2);
}

void disablecapture()
{
	setmouseactive(0, 0);
	focus = 0;
	mouseinside = false;
	if (currprefs.active_nocapture_pause && sound_closed == 0) {
		setpaused(2);
		sound_closed = 1;
	}
	else if (currprefs.active_nocapture_nosound && sound_closed == 0) {
		setsoundpaused();
		sound_closed = -1;
	}
}

void setmouseactivexy(const int monid, int x, int y, const int dir)
{
	const AmigaMonitor* mon = &AMonitors[monid];
	constexpr int diff = 8;

	if (isfullscreen() > 0)
		return;
	x += mon->amigawin_rect.x;
	y += mon->amigawin_rect.y;
	if (dir & 1)
		x = mon->amigawin_rect.x - diff;
	if (dir & 2)
		x = mon->amigawin_rect.x + mon->amigawin_rect.w + diff;
	if (dir & 4)
		y = mon->amigawin_rect.y - diff;
	if (dir & 8)
		y = mon->amigawin_rect.y + mon->amigawin_rect.h + diff;
	if (!dir) {
		x += mon->amigawin_rect.w / 2;
		y += mon->amigawin_rect.h / 2;
	}
	if (isfullscreen() < 0) {
		SDL_Point pt;
		pt.x = x;
		pt.y = y;
		if (!MonitorFromPoint(pt))
			return;
	}
	if (mouseactive) {
		disablecapture();
		// Magic mouse edge release needs the host cursor to leave the window.
		// Window-relative warps cannot do that; use global coordinates instead.
		if (!SDL_WarpMouseGlobal((float)x, (float)y)) {
			write_log("SDL_WarpMouseGlobal(%d, %d) failed on monitor %d: %s\n",
				x, y, mon->monitor_id, SDL_GetError());
		}
		if (dir) {
			recapture = 1;
		}
	}
}

int isfocus()
{
	if (isfullscreen() > 0) {
		if (!minimized)
			return 2;
		return 0;
	}
	if (currprefs.input_tablet >= TABLET_MOUSEHACK && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		if (mouseinside)
			return 2;
		if (focus)
			return 1;
		return 0;
	}
	if (focus && mouseactive > 0)
		return 2;
	if (focus)
		return -1;
	return 0;
}

void activationtoggle(const int monid, const bool inactiveonly)
{
	if (mouseactive) {
		if ((isfullscreen() > 0) || (isfullscreen() < 0 && currprefs.minimize_inactive)) {
			disablecapture();
			minimizewindow(monid);
		} else {
			setmouseactive(monid, 0);
		}
	} else {
		if (!inactiveonly)
			setmouseactive(monid, 1);
	}
}

#define MEDIA_INSERT_QUEUE_SIZE 10
static TCHAR* media_insert_queue[MEDIA_INSERT_QUEUE_SIZE];
static int media_insert_queue_type[MEDIA_INSERT_QUEUE_SIZE];
static SDL_TimerID media_change_timer;
static SDL_TimerID device_change_timer;

static int is_in_media_queue(const TCHAR* drvname)
{
	for (int i = 0; i < MEDIA_INSERT_QUEUE_SIZE; i++) {
		if (media_insert_queue[i] != nullptr) {
			if (!_tcsicmp(drvname, media_insert_queue[i]))
				return i;
		}
	}
	return -1;
}

static void start_media_insert_timer();

//uint32_t timer_callbackfunc(uint32_t interval, void* param)
//{
//	if ((*(int*)&param) == 2) {
//		bool restart = false;
//		SDL_RemoveTimer(media_change_timer);
//		media_change_timer = 0;
//
//		for (int i = 0; i < MEDIA_INSERT_QUEUE_SIZE; i++) {
//			if (media_insert_queue[i]) {
//				TCHAR* drvname = media_insert_queue[i];
//				int r = my_getvolumeinfo(drvname);
//				if (r < 0) {
//					if (media_insert_queue_type[i] > 0) {
//						write_log(_T("Mounting %s but drive is not ready, %d.. retrying %d..\n"), drvname, r, media_insert_queue_type[i]);
//						media_insert_queue_type[i]--;
//						restart = true;
//						continue;
//					}
//					else {
//						write_log(_T("Mounting %s but drive is not ready, %d.. aborting..\n"), drvname, r);
//					}
//				}
//				else {
//					int inserted = 1;
//					//DWORD type = GetDriveType(drvname);
//					//if (type == DRIVE_CDROM)
//					//	inserted = -1;
//					r = filesys_media_change(drvname, inserted, NULL);
//					if (r < 0) {
//						write_log(_T("Mounting %s but previous media change is still in progress..\n"), drvname);
//						restart = true;
//						break;
//					}
//					else if (r > 0) {
//						write_log(_T("%s mounted\n"), drvname);
//					}
//					else {
//						write_log(_T("%s mount failed\n"), drvname);
//					}
//				}
//				xfree(media_insert_queue[i]);
//				media_insert_queue[i] = NULL;
//			}
//		}
//
//		if (restart)
//			start_media_insert_timer();
//	}
//	else if ((*(int*)&param) == 4) {
//		SDL_RemoveTimer(device_change_timer);
//		device_change_timer = 0;
//		inputdevice_devicechange(&changed_prefs);
//		inputdevice_copyjports(&changed_prefs, &currprefs);
//	}
//	else if ((*(int*)&param) == 1) {
//#ifdef PARALLEL_PORT
//		finishjob();
//#endif
//	}
//	return 0;
//}

static void start_media_insert_timer()
{
	//if (!media_change_timer) {
	//	media_change_timer = SDL_AddTimer(1000, timer_callbackfunc, (void*)2);
	//}
}

static void add_media_insert_queue(const TCHAR* drvname, const int retrycnt)
{
	const int idx = is_in_media_queue(drvname);
	if (idx >= 0) {
		media_insert_queue_type[idx] = std::max(retrycnt, media_insert_queue_type[idx]);
		write_log(_T("%s already queued for insertion, cnt=%d.\n"), drvname, retrycnt);
		start_media_insert_timer();
		return;
	}
	for (int i = 0; i < MEDIA_INSERT_QUEUE_SIZE; i++) {
		if (media_insert_queue[i] == nullptr) {
			media_insert_queue[i] = my_strdup(drvname);
			media_insert_queue_type[i] = retrycnt;
			start_media_insert_timer();
			return;
		}
	}
}

static int touch_touched;
static unsigned long touch_time;

#define MAX_TOUCHES 10
struct touch_store
{
	bool inuse;
	unsigned long id;
	int port;
	int button;
	int axis;
};
static touch_store touches[MAX_TOUCHES];

static void touch_release(touch_store* ts, const SDL_Rect* rcontrol)
{
	if (ts->port == 0) {
		if (ts->button == 0)
			inputdevice_uaelib(_T("JOY1_FIRE_BUTTON"), 0, 1, false);
		if (ts->button == 1)
			inputdevice_uaelib(_T("JOY1_2ND_BUTTON"), 0, 1, false);
	}
	else if (ts->port == 1) {
		if (ts->button == 0)
			inputdevice_uaelib(_T("JOY2_FIRE_BUTTON"), 0, 1, false);
		if (ts->button == 1)
			inputdevice_uaelib(_T("JOY2_2ND_BUTTON"), 0, 1, false);
	}
	if (ts->axis >= 0) {
		if (ts->port == 0) {
			inputdevice_uaelib(_T("MOUSE1_HORIZ"), 0, -1, false);
			inputdevice_uaelib(_T("MOUSE1_VERT"), 0, -1, false);
		}
		else {
			inputdevice_uaelib(_T("JOY2_HORIZ"), 0, 1, false);
			inputdevice_uaelib(_T("JOY2_VERT"), 0, 1, false);
		}
	}
	ts->button = -1;
	ts->axis = -1;
}

static void tablet_touch(unsigned long id, int pressrel, const int x, const int y, const SDL_Rect* rcontrol)
{
	touch_store* ts = nullptr;
	const int buttony = rcontrol->h - (rcontrol->h - rcontrol->y) / 4;

	int new_slot = -1;
	for (int i = 0; i < MAX_TOUCHES; i++) {
		touch_store* tts = &touches[i];
		if (!tts->inuse && new_slot < 0)
			new_slot = i;
		if (tts->inuse && tts->id == id) {
			ts = tts;
#if TOUCH_DEBUG > 1
			write_log(_T("touch_event: old touch event %d\n"), id);
#endif
			break;
		}
	}
	if (!ts) {
		// do not allocate new if release
		if (pressrel == 0)
			return;
		if (new_slot < 0)
			return;
#if TOUCH_DEBUG > 1
		write_log(_T("touch_event: new touch event %d\n"), id);
#endif
		ts = &touches[new_slot];
		ts->inuse = true;
		ts->axis = -1;
		ts->button = -1;
		ts->id = id;

	}

	// Touch release? Release buttons, center stick/mouse.
	if (ts->inuse && ts->id == id && pressrel < 0) {
		touch_release(ts, rcontrol);
		ts->inuse = false;
		return;
	}

	// Check hit boxes if new touch.
	for (int i = 0; i < 2; i++) {
		const SDL_Rect* r = &rcontrol[i];
		if (x >= r->x && x < r->w && y >= r->y && y < r->h) {

#if TOUCH_DEBUG > 1
			write_log(_T("touch_event: press=%d rect=%d wm=%d\n"), pressrel, i, dinput_winmouse());
#endif
			if (pressrel == 0) {
				// move? port can't change, axis<>button not allowed
				if (ts->port == i) {
					if (y >= buttony && ts->button >= 0) {
						const int button = x > r->x + (r->w - r->x) / 2 ? 1 : 0;
						if (button != ts->button) {
							// button change, release old button
							touch_release(ts, rcontrol);
							ts->button = button;
							pressrel = 1;
						}
					}
				}
			}
			else if (pressrel > 0) {
				// new touch
				ts->port = i;
				if (ts->button < 0 && y >= buttony) {
					ts->button = x > r->x + (r->w - r->x) / 2 ? 1 : 0;
				}
				else if (ts->axis < 0 && y < buttony) {
					ts->axis = 1;
				}
			}
		}
	}

	// Directions hit?
	if (ts->inuse && ts->id == id && ts->axis >= 0) {
		const SDL_Rect* r = &rcontrol[ts->port];
		int xdiff = (r->x + (r->w - r->x) / 2) - x;
		int ydiff = (r->y + (buttony - r->y) / 2) - y;

#if TOUCH_DEBUG > 1
		write_log(_T("touch_event: rect=%d xdiff %03d ydiff %03d\n"), ts->port, xdiff, ydiff);
#endif
		xdiff = -xdiff;
		ydiff = -ydiff;

		if (ts->port == 0) {

			int div = (r->h - r->y) / (2 * 5);
			if (div <= 0)
				div = 1;
			int vx = xdiff / div;
			int vy = ydiff / div;

#if TOUCH_DEBUG > 1
			write_log(_T("touch_event: xdiff %03d ydiff %03d div %03d vx %03d vy %03d\n"), xdiff, ydiff, div, vx, vy);
#endif
			inputdevice_uaelib(_T("MOUSE1_HORIZ"), vx, -1, false);
			inputdevice_uaelib(_T("MOUSE1_VERT"), vy, -1, false);

		}
		else {

			int div = (r->h - r->y) / (2 * 3);
			if (div <= 0)
				div = 1;
			if (xdiff <= -div)
				inputdevice_uaelib(_T("JOY2_HORIZ"), -1, 1, false);
			else if (xdiff >= div)
				inputdevice_uaelib(_T("JOY2_HORIZ"), 1, 1, false);
			else
				inputdevice_uaelib(_T("JOY2_HORIZ"), 0, 1, false);
			if (ydiff <= -div)
				inputdevice_uaelib(_T("JOY2_VERT"), -1, 1, false);
			else if (ydiff >= div)
				inputdevice_uaelib(_T("JOY2_VERT"), 1, 1, false);
			else
				inputdevice_uaelib(_T("JOY2_VERT"), 0, 1, false);
		}
	}

	// Buttons hit?
	if (ts->inuse && ts->id == id && pressrel > 0) {
		if (ts->port == 0) {
			if (ts->button == 0)
				inputdevice_uaelib(_T("JOY1_FIRE_BUTTON"), 1, 1, false);
			if (ts->button == 1)
				inputdevice_uaelib(_T("JOY1_2ND_BUTTON"), 1, 1, false);
		}
		else if (ts->port == 1) {
			if (ts->button == 0)
				inputdevice_uaelib(_T("JOY2_FIRE_BUTTON"), 1, 1, false);
			if (ts->button == 1)
				inputdevice_uaelib(_T("JOY2_2ND_BUTTON"), 1, 1, false);
		}
	}
}

static void touch_event(const unsigned long id, const int pressrel, const int x, const int y, const SDL_Rect* rcontrol)
{
	if (is_touch_lightpen()) {
		//tablet_lightpen(x, y, -1, -1, pressrel, pressrel > 0, true, dinput_lightpen(), -1);
	}
	else {
		tablet_touch(id, pressrel, x, y, rcontrol);
	}
}

static bool hasresizelimit(struct AmigaMonitor* mon)
{
	if (!mon->ratio_sizing || !mon->ratio_width || !mon->ratio_height)
		return false;
	return true;
}

static int canstretch(const AmigaMonitor* mon)
{
	if (isfullscreen() != 0)
		return 0;
	if (!mon->screen_is_picasso) {
		if (!currprefs.gfx_windowed_resize)
			return 0;
		if (currprefs.gf[GF_NORMAL].gfx_filter_autoscale == AUTOSCALE_RESIZE)
			return 0;
		return 1;
	}
	else {
		if (currprefs.rtgallowscaling || currprefs.gf[GF_RTG].gfx_filter_autoscale)
			return 1;
	}
	return 0;
}

static void getsizemove(AmigaMonitor* mon)
{
	mon->ratio_width = mon->amigawin_rect.w;
	mon->ratio_height = mon->amigawin_rect.h;
	mon->ratio_adjust_x = mon->ratio_width - mon->mainwin_rect.w;
	mon->ratio_adjust_y = mon->ratio_height - mon->mainwin_rect.h;
	const bool* state = SDL_GetKeyboardState(nullptr);
	mon->ratio_sizing = state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL];
}

static int setsizemove(AmigaMonitor* mon, SDL_Window* hWnd)
{
	if (isfullscreen() > 0)
		return 0;
	if (mon->in_sizemove > 0)
		return 0;
	int iconic = (SDL_GetWindowFlags(hWnd) & SDL_WINDOW_MINIMIZED) != 0;
	if (mon->amiga_window && !iconic) {
		//write_log (_T("WM_WINDOWPOSCHANGED MAIN\n"));
		GetWindowRect(mon->amiga_window, &mon->mainwin_rect);
		updatewinrect(mon, false);
		updatemouseclip(mon);
		if (minimized) {
			unsetminimized(mon->monitor_id);
			amiberry_active(mon, minimized);
		}
		if (isfullscreen() == 0) {
			static int store_xy;
			SDL_Rect rc2;
			GetWindowRect(mon->amiga_window, &rc2);
			int left = rc2.x - mon->win_x_diff;
			int top = rc2.y - mon->win_y_diff;
			int width = rc2.w;
			int height = rc2.h;
			if (store_xy++) {
				if (!mon->monitor_id) {
					regsetint(nullptr, _T("MainPosX"), left);
					regsetint(nullptr, _T("MainPosY"), top);
				}
				else {
					TCHAR buf[100];
					_sntprintf(buf, sizeof buf, _T("MainPosX_%d"), mon->monitor_id);
					regsetint(nullptr, buf, left);
					_sntprintf(buf, sizeof buf, _T("MainPosY_%d"), mon->monitor_id);
					regsetint(nullptr, buf, top);
				}
			}
			changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.x = left;
			changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.y = top;
			if (canstretch(mon)) {
				int w = mon->mainwin_rect.w;
				int h = mon->mainwin_rect.h;
				const int content_w = w - mon->window_extra_width;
				const int content_h = h - mon->window_extra_height;
				if (content_w != changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.width ||
					content_h != changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.height) {
					changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.width = content_w;
					changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.height = content_h;
					set_config_changed();
				}
				//if (mon->hStatusWnd)
				//	SendMessage(mon->hStatusWnd, WM_SIZE, SIZE_RESTORED, MAKELONG(w, h));
			}

			return 0;
		}
	}
	return 1;
}

static void handle_focus_gained_event(AmigaMonitor* mon)
{
	amiberry_active(mon, minimized);
	unsetminimized(mon->monitor_id);

	if (mon->focus_transitioning && isfullscreen() <= 0 && mon->amiga_window) {
		if (mon->moved_during_focus_transition) {
			// User intentionally moved the window while focus was lost
			// (e.g. dragging by the title bar on Windows). Keep the new
			// position and persist it to prefs, since handle_moved_event
			// skipped setsizemove during the transition.
			setsizemove(mon, mon->amiga_window);
		} else {
			int cur_x, cur_y;
			SDL_GetWindowPosition(mon->amiga_window, &cur_x, &cur_y);
			if (cur_x != mon->pre_focus_x || cur_y != mon->pre_focus_y) {
				mon->in_sizemove++;
				SDL_SetWindowPosition(mon->amiga_window, mon->pre_focus_x, mon->pre_focus_y);
			}
		}
	}
	mon->focus_transitioning = false;
	mon->moved_during_focus_transition = false;

	int pending_active = 0;
	if (consume_pending_mouse_capture(mon->monitor_id, &pending_active) && !mouseactive) {
		write_log("Focus gained on monitor %d, completing deferred mouse capture (active=%d)\n",
			mon->monitor_id, pending_active);
		setmouseactive(mon->monitor_id, pending_active);
	}
	restore_active_mouse_capture(mon);
}

static void handle_minimized_event(const AmigaMonitor* mon)
{
	if (!minimized)
	{
		write_log(_T("SIZE_MINIMIZED\n"));
		setminimized(mon->monitor_id);
		amiberry_inactive(mon, minimized);
	}
}

static void handle_restored_event(const AmigaMonitor* mon)
{
	amiberry_active(mon, minimized);
	unsetminimized(mon->monitor_id);
}

static void handle_moved_event(AmigaMonitor* mon)
{
	if (mon->focus_transitioning) {
		mon->moved_during_focus_transition = true;
		return;
	}
	setsizemove(mon, mon->amiga_window);
}

static void update_hidpi_scale(AmigaMonitor* mon)
{
	mon->hidpi_needs_scaling = false;
	mon->hidpi_scale_x = 1.0f;
	mon->hidpi_scale_y = 1.0f;
	IRenderer* renderer = get_renderer(mon->monitor_id);
	if (renderer && mon->amiga_window) {
		int win_w, win_h, draw_w, draw_h;
		SDL_GetWindowSize(mon->amiga_window, &win_w, &win_h);
		renderer->get_drawable_size(mon->amiga_window, &draw_w, &draw_h);
		if (win_w > 0 && draw_w > 0 && win_w != draw_w) {
			mon->hidpi_scale_x = (float)draw_w / (float)win_w;
			mon->hidpi_needs_scaling = true;
		}
		if (win_h > 0 && draw_h > 0 && win_h != draw_h) {
			mon->hidpi_scale_y = (float)draw_h / (float)win_h;
			mon->hidpi_needs_scaling = true;
		}
	}
}

static void handle_resized_event(AmigaMonitor* mon, int width, int height)
{
	write_log("Window resized to: %dx%d\n", width, height);
	setsizemove(mon, mon->amiga_window);
	update_hidpi_scale(mon);
}

static void handle_enter_event(AmigaMonitor* mon)
{
	mouseinside = true;
	if (currprefs.input_tablet > 0 && currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC && isfullscreen() <= 0)
	{
		if (mousehack_alive())
			setcursorshape(0);
	}
	restore_active_mouse_capture(mon);
}

static void handle_leave_event(AmigaMonitor* mon)
{
	mouseinside = false;
#ifndef LIBRETRO
	// SDL3 < 3.4.2 Wayland crash workaround: turn off relative mouse mode
	// when the pointer leaves, so subsequent pump cycles don't hit a NULL
	// window in pointer_dispatch_relative_motion. See SDL fab42a14, #1829.
	const char* video_driver = SDL_GetCurrentVideoDriver();
	if (video_driver && strcmpi(video_driver, "wayland") == 0
		&& mon->amiga_window && SDL_GetWindowRelativeMouseMode(mon->amiga_window)) {
		SDL_SetWindowRelativeMouseMode(mon->amiga_window, false);
	}
#endif
}

static void handle_focus_lost_event(AmigaMonitor* mon)
{
	if (isfullscreen() <= 0 && mon->amiga_window) {
		SDL_GetWindowPosition(mon->amiga_window, &mon->pre_focus_x, &mon->pre_focus_y);
		mon->focus_transitioning = true;
		mon->moved_during_focus_transition = false;
	}
	amiberry_inactive(mon, minimized);
	if (isfullscreen() <= 0 && currprefs.minimize_inactive)
		minimizewindow(mon->monitor_id);
}

static void handle_close_event()
{
	wait_keyrelease();
	inputdevice_unacquire();
	uae_quit();
}

static void handle_window_event(const SDL_Event& event, AmigaMonitor* mon)
{
	switch (event.type)
	{
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		handle_focus_gained_event(mon);
		break;
	case SDL_EVENT_WINDOW_MINIMIZED:
		handle_minimized_event(mon);
		break;
	case SDL_EVENT_WINDOW_RESTORED:
		handle_restored_event(mon);
		break;
	case SDL_EVENT_WINDOW_MOVED:
		handle_moved_event(mon);
		break;
#ifndef LIBRETRO
	case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
		// Window migrated to a different display — force the hw VSync pacing
		// decision to re-probe against the new display's refresh rate.
		amiberry_hw_vsync_pacing_invalidate();
		break;
#endif
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	case SDL_EVENT_WINDOW_RESIZED:
		handle_resized_event(mon, event.window.data1, event.window.data2);
		break;
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
		handle_enter_event(mon);
		break;
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		handle_leave_event(mon);
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		handle_focus_lost_event(mon);
		break;
	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
	{
		// Sync internal state when entering fullscreen via OS mechanism (e.g. macOS green button)
		const auto* ad = &adisplays[mon->monitor_id];
		auto* p = ad->picasso_on
			? &changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen
			: &changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen;
		if (*p == GFX_WINDOW) {
			*p = GFX_FULLWINDOW;
			set_config_changed();
		}
		break;
	}
	case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
	{
		// Sync internal state when leaving fullscreen via OS mechanism (e.g. macOS green button)
		const auto* ad = &adisplays[mon->monitor_id];
		auto* p = ad->picasso_on
			? &changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen
			: &changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen;
		if (*p == GFX_FULLWINDOW) {
			*p = GFX_WINDOW;
			set_config_changed();
		}
		break;
	}
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		handle_close_event();
		break;
	default:
		break;
	}
}

static void handle_quit_event()
{
	uae_quit();
}

static void handle_clipboard_update_event()
{
	if (currprefs.clipboard_sharing && SDL_HasClipboardText() == true)
	{
		const auto* clipboard_host = SDL_GetClipboardText();
		uae_clipboard_put_text(clipboard_host);
	}
}

void handle_joy_device_event(const SDL_JoystickID which, const bool removed, struct uae_prefs* prefs)
{
	bool known_device = false;
	for (int id = 0; id < MAX_INPUT_DEVICES; ++id)
	{
		const didata* did = &di_joystick[id];
		if (!did->guid.empty() && did->joystick_id == which)
		{
			known_device = true;
			break;
		}
	}
	if (!known_device || removed)
	{
		write_log("SDL Gamepad/Joystick added or removed, re-running import joysticks...\n");
		if (inputdevice_devicechange(prefs))
		{
			import_joysticks();
			joystick_refresh_needed = true;
		}
	}
}

static void handle_controller_button_event(const SDL_Event& event)
{
	const auto button = event.gbutton.button;
	const auto state = event.gbutton.down;
	const auto which = event.gbutton.which;

	if (button == enter_gui_button) {
		inputdevice_add_inputcode(AKS_ENTERGUI, state, nullptr);
	}
	else if (quit_key.button && button == quit_key.button) {
		uae_quit();
	}
	else if (action_replay_key.button && button == action_replay_key.button) {
		inputdevice_add_inputcode(AKS_FREEZEBUTTON, state, nullptr);
	}
	else if (fullscreen_key.button && button == fullscreen_key.button) {
		inputdevice_add_inputcode(AKS_TOGGLEWINDOWFULLWINDOW, state, nullptr);
	}
	else if (minimize_key.button && button == minimize_key.button) {
		minimizewindow(0);
	}
	else if (vkbd_button != SDL_GAMEPAD_BUTTON_INVALID && button == vkbd_button) {
		inputdevice_add_inputcode(AKS_OSK, state, nullptr);
	}
	else if (imgui_osk_is_active()) {
		// When OSK is visible, intercept D-pad and face buttons at the SDL level
		// before they reach UAE's input system. This ensures immediate response.
		// Track per-button state so releasing one direction doesn't lose the other.
		static bool dpad_up = false, dpad_down = false, dpad_left = false, dpad_right = false;
		bool is_dir = true;
		switch (button) {
		case SDL_GAMEPAD_BUTTON_DPAD_UP:    dpad_up    = state; break;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:  dpad_down  = state; break;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:  dpad_left  = state; break;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: dpad_right = state; break;
		default: is_dir = false; break;
		}
		if (is_dir) {
			int dx = 0, dy = 0;
			if (dpad_left)  dx = -1;
			else if (dpad_right) dx = 1;
			if (dpad_up)    dy = -1;
			else if (dpad_down)  dy = 1;
			osk_control(dx, dy, 0, 0);
			return; // consume — don't pass to UAE input system
		}
		// Fire button (A/South) = press key
		if (button == SDL_GAMEPAD_BUTTON_SOUTH) {
			osk_control(0, 0, 1, state);
			return;
		}
		// B/East = close keyboard
		if (button == SDL_GAMEPAD_BUTTON_EAST && state) {
			imgui_osk_toggle();
			return;
		}
	}
	else if (screenshot_key.button && button == screenshot_key.button) {
		inputdevice_add_inputcode(AKS_SCREENSHOT_FILE, state, nullptr);
	}
	else if (debugger_key.button && button == debugger_key.button) {
		inputdevice_add_inputcode(AKS_ENTERDEBUGGER, state, nullptr);
	}
	else {
		for (auto id = 0; id < MAX_INPUT_DEVICES; id++) {
			didata* did = &di_joystick[id];
			if (did->name.empty() || did->joystick_id != which || did->mapping.is_retroarch || !did->is_controller) continue;

			// Update per-device hotkey state in event order
			if (button == did->mapping.hotkey_button)
			{
				did->hotkey_held = state;
				break;
			}

			read_controller_button(id, button, state);
			break;
		}
	}

}

static void handle_joy_button_event(const SDL_Event& event)
{
	const auto button = event.jbutton.button;
	const auto state = event.jbutton.down;
	const auto which = event.jbutton.which;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		didata* did = &di_joystick[id];
		if (did->name.empty() || did->joystick_id != which || (!did->mapping.is_retroarch && did->is_controller)) continue;

		// Update per-device hotkey state in event order (not polled)
		if (button == did->mapping.hotkey_button)
		{
			did->hotkey_held = state;
			break;
		}
		if (button == did->mapping.menu_button && did->hotkey_held && state)
		{
			did->hotkey_held = false;
			inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
			break;
		}
		if (button == did->mapping.vkbd_button && did->hotkey_held && state)
		{
			did->hotkey_held = false;
			inputdevice_add_inputcode(AKS_OSK, 1, nullptr);
			break;
		}

		read_joystick_button_single(id, button, state);
		break;
	}

}

static void handle_controller_axis_motion_event(const SDL_Event& event)
{
	const auto axis = event.gaxis.axis;
	const auto value = event.gaxis.value;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		const didata* did = &di_joystick[id];
		if (did->name.empty() || did->joystick_id != event.gaxis.which || did->mapping.is_retroarch || !did->is_controller) continue;

		read_controller_axis(id, axis, value);
		break;
	}

}

static void handle_joy_axis_motion_event(const SDL_Event& event)
{
	const auto axis = event.jaxis.axis;
	const auto value = event.jaxis.value;
	const auto which = event.jaxis.which;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		const didata* did = &di_joystick[id];
		if (did->name.empty() || did->joystick_id != which || (!did->mapping.is_retroarch && did->is_controller)) continue;

		read_joystick_axis(id, axis, value);
		break;
	}

}

static void handle_joy_hat_motion_event(const SDL_Event& event)
{
	const auto hat = event.jhat.hat;
	const auto value = event.jhat.value;
	const auto which = event.jhat.which;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		const didata* did = &di_joystick[id];
		if (did->name.empty() || did->joystick_id != which || (!did->mapping.is_retroarch && did->is_controller)) continue;

		read_joystick_hat(id, hat, value);
		break;
	}
}

static void handle_key_event(const SDL_Event& event)
{
#ifdef __ANDROID__
	// Android back button must be handled before the focus check.
	// When the native pause-menu AlertDialog is visible the SDL window
	// loses focus, so isfocus() returns 0 and all key events would be
	// dropped.  The pause menu's "Advanced Settings" option injects
	// KEYCODE_BACK via JNI to open the ImGui GUI — that must always
	// get through regardless of focus state.
	if (event.key.scancode == SDL_SCANCODE_AC_BACK) {
		if (event.key.down && !event.key.repeat) {
			inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
		}
		return;
	}
#endif

	// Allow keyboard input if we have any focus level
	const int focus_level = isfocus();
	if (event.key.repeat || !focus_level)
		return;
	
	// Only apply the virtual mouse focus restriction in windowed mode.
	// In fullscreen/full-window modes (including KMSDRM console), keyboard should work
	// because console mode never receives SDL_EVENT_WINDOW_MOUSE_ENTER, leaving mouseinside false.
	if (isfullscreen() == 0 && focus_level < 2 && 
		currprefs.input_tablet >= TABLET_MOUSEHACK && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC))
		return;

	int scancode = event.key.scancode;
	const auto pressed = event.key.down;

	// Ctrl+Alt releases mouse capture (like QEMU). Works on trackpads
	// where there is no middle mouse button available.
	if (currprefs.ctrl_alt_release && pressed && mouseactive)
	{
		const bool* kbstate = SDL_GetKeyboardState(nullptr);
		const bool ctrl_held = kbstate[SDL_SCANCODE_LCTRL] || kbstate[SDL_SCANCODE_RCTRL];
		const bool alt_held = kbstate[SDL_SCANCODE_LALT] || kbstate[SDL_SCANCODE_RALT];
		const bool is_ctrl = (scancode == SDL_SCANCODE_LCTRL || scancode == SDL_SCANCODE_RCTRL);
		const bool is_alt = (scancode == SDL_SCANCODE_LALT || scancode == SDL_SCANCODE_RALT);

		if ((is_ctrl && alt_held) || (is_alt && ctrl_held))
		{
			activationtoggle(0, true);
			// Release both modifiers in the emulator so they don't stick
			my_kbd_handler(0, SDL_SCANCODE_LCTRL, 0, true);
			my_kbd_handler(0, SDL_SCANCODE_RCTRL, 0, true);
			my_kbd_handler(0, SDL_SCANCODE_LALT, 0, true);
			my_kbd_handler(0, SDL_SCANCODE_RALT, 0, true);
			return;
		}
	}

	if (key_swap_hack == 1) {
		if (scancode == SDL_SCANCODE_F11) {
			scancode = SDL_SCANCODE_BACKSLASH;
		}
		else if (scancode == SDL_SCANCODE_BACKSLASH) {
			scancode = SDL_SCANCODE_F11;
		}
	}
	if (key_swap_end_pgup) {
		if (scancode == SDL_SCANCODE_END) {
			scancode = SDL_SCANCODE_PAGEUP;
		}
		else if (scancode == SDL_SCANCODE_PAGEUP) {
			scancode = SDL_SCANCODE_END;
		}
	}
	if (left_amiga_key.scancode && scancode == left_amiga_key.scancode) {
		scancode = SDL_SCANCODE_LGUI;
	}
	if ((amiberry_options.rctrl_as_ramiga || currprefs.right_control_is_right_win_key) && scancode == SDL_SCANCODE_RCTRL)
	{
		scancode = SDL_SCANCODE_RGUI;
	}
	if (right_amiga_key.scancode && scancode == right_amiga_key.scancode)
	{
		scancode = SDL_SCANCODE_RGUI;
	}
	scancode = keyhack(scancode, pressed, 0);
	if (scancode >= 0)
	{
		my_kbd_handler(0, scancode, pressed, true);
	}

}

#ifndef LIBRETRO
static int pen_in_proximity;
#endif

static void handle_finger_event(const SDL_Event& event)
{
	if (!isfocus())
		return;
#ifndef LIBRETRO
	if (pen_in_proximity && currprefs.input_tablet > 0)
		return;
#endif

    // Simple single-finger tap for Left Click
	if (event.tfinger.fingerID == 0)
	{
        if (event.tfinger.type == SDL_EVENT_FINGER_DOWN)
        {
            setmousebuttonstate(0, 0, 1);
        }
        else if (event.tfinger.type == SDL_EVENT_FINGER_UP)
        {
            setmousebuttonstate(0, 0, 0);
        }
	}
    // 2nd finger tap for Right Click
    else if (event.tfinger.fingerID == 1)
    {
        if (event.tfinger.type == SDL_EVENT_FINGER_DOWN)
        {
            setmousebuttonstate(0, 1, 1);
        }
        else if (event.tfinger.type == SDL_EVENT_FINGER_UP)
        {
            setmousebuttonstate(0, 1, 0);
        }
    }
}

static void handle_mouse_button_event(const SDL_Event& event, const AmigaMonitor* mon)
{
#ifndef LIBRETRO
	if (pen_in_proximity && currprefs.input_tablet > 0)
		return;
#endif

	const auto button = event.button.button;
	const auto state = event.button.down;
	const auto clicks = event.button.clicks;
	const int midx = get_mouse_index_from_sdl_id(event.button.which);

	if (button == SDL_BUTTON_LEFT && !mouseactive && (!mousehack_alive() || currprefs.input_tablet != TABLET_MOUSEHACK ||
		(currprefs.input_tablet == TABLET_MOUSEHACK && !(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC))))
	{
		mouseinside = true;
		if (!pause_emulation || currprefs.active_nocapture_pause)
			setmouseactive(mon->monitor_id, (clicks == 1 || isfullscreen() > 0) ? 2 : 1);
	}
	else if (isfocus())
	{
		switch (button)
		{
		case SDL_BUTTON_LEFT:
			setmousebuttonstate(midx, 0, state);
			break;
		case SDL_BUTTON_RIGHT:
			setmousebuttonstate(midx, 1, state);
			break;
		case SDL_BUTTON_MIDDLE:
			if (currprefs.input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON)
				activationtoggle(0, true);
			else
				setmousebuttonstate(midx, 2, state);
			break;
		case SDL_BUTTON_X1:
			setmousebuttonstate(midx, 3, state);
			break;
		case SDL_BUTTON_X2:
			setmousebuttonstate(midx, 4, state);
			break;
		default: break;
		}
	}

}

static void handle_finger_motion_event(const SDL_Event& event)
{
#ifndef LIBRETRO
	if (pen_in_proximity && currprefs.input_tablet > 0)
		return;
#endif
	if (isfocus() && event.tfinger.fingerID == 0)
	{
		// Use relative movement for better control (Laptop touchpad style)
		// Scale normalized coords (0..1) to window pixels
		int w = 0, h = 0;
		const AmigaMonitor* mon = &AMonitors[mouse_monid];
		if (mon->amiga_window) {
			SDL_GetWindowSize(mon->amiga_window, &w, &h);
		} else {
			// Fallback if window not ready, though unlikely if getting events
			w = 640; h = 480;
		}

		int relX = (int)(event.tfinger.dx * w);
		int relY = (int)(event.tfinger.dy * h);

		setmousestate(0, 0, relX, 0); // 0 = relative
		setmousestate(0, 1, relY, 0);
	}
}

static void handle_mouse_motion_event(const SDL_Event& event, const AmigaMonitor* mon)
{
	monitor_off = 0;

#ifndef LIBRETRO
	if (pen_in_proximity && currprefs.input_tablet > 0)
		return;
#endif

	if (mouseinside && recapture && isfullscreen() <= 0) {
		enablecapture(mon->monitor_id);
		return;
	}

	if (isfocus() <= 0) return;

	const int midx = get_mouse_index_from_sdl_id(event.motion.which);

	int32_t x = event.motion.x;
	int32_t y = event.motion.y;
	int32_t xrel = event.motion.xrel;
	int32_t yrel = event.motion.yrel;

	// HiDPI / Retina: scale from screen coordinates (points) to drawable pixels.
	// Only needed for OpenGL path — SDL_RenderCoordinatesFromWindow handles this
	// internally when the SDL renderer is active.
	// Scale factors cached per-monitor in update_hidpi_scale(), updated on window resize.
	if (!mon->amiga_renderer && mon->hidpi_needs_scaling) {
		x = (int32_t)(x * mon->hidpi_scale_x);
		xrel = (int32_t)(xrel * mon->hidpi_scale_x);
		y = (int32_t)(y * mon->hidpi_scale_y);
		yrel = (int32_t)(yrel * mon->hidpi_scale_y);
	}

#ifndef LIBRETRO
	// SDL3 logical presentation: convert window coordinates to render coordinates
	if (mon->amiga_renderer) {
		float rx, ry, rx0, ry0;
		if (SDL_RenderCoordinatesFromWindow(mon->amiga_renderer, (float)x, (float)y, &rx, &ry)) {
			SDL_RenderCoordinatesFromWindow(mon->amiga_renderer, (float)(x - xrel), (float)(y - yrel), &rx0, &ry0);
			xrel = (int32_t)(rx - rx0);
			yrel = (int32_t)(ry - ry0);
			x = (int32_t)rx;
			y = (int32_t)ry;
		}
	}
#endif

	if (currprefs.input_tablet >= TABLET_MOUSEHACK)
	{
		setmousestate(midx, 0, x, 1);
		setmousestate(midx, 1, y, 1);
	}
	else
	{
		setmousestate(midx, 0, xrel, 0);
		setmousestate(midx, 1, yrel, 0);
	}

}

static void handle_mouse_wheel_event(const SDL_Event& event)
{
	if (isfocus() <= 0) return;

	const int midx = get_mouse_index_from_sdl_id(event.wheel.which);
	const auto val_y = event.wheel.y;
	const auto val_x = event.wheel.x;

	setmousestate(midx, 2, val_y, 0);
	setmousestate(midx, 3, val_x, 0);

	if (val_y < 0)
		setmousebuttonstate(midx, 3, -1);
	else if (val_y > 0)
		setmousebuttonstate(midx, 4, -1);

	if (val_x < 0)
		setmousebuttonstate(midx, 5, -1);
	else if (val_x > 0)
		setmousebuttonstate(midx, 6, -1);
}

static AmigaMonitor* monitor_from_window_id(SDL_WindowID window_id);

#ifndef LIBRETRO
static float pen_pressure;
static float pen_xtilt;
static float pen_ytilt;
static float pen_rotation;
static uae_u32 pen_buttons;
static int pen_eraser;

static void pen_coords_to_tablet(const AmigaMonitor* mon, float x, float y, int* tx, int* ty)
{
	IRenderer* renderer = get_renderer(mon->monitor_id);
	if (renderer) {
		// Transform window coordinates to match render_quad's coordinate space.
		// SDL renderer: logical presentation space (via SDL_RenderCoordinatesFromWindow).
		// OpenGL: drawable pixel space (via cached HiDPI scale factors).
		if (mon->amiga_renderer) {
			float rx, ry;
			if (SDL_RenderCoordinatesFromWindow(mon->amiga_renderer, x, y, &rx, &ry)) {
				x = rx;
				y = ry;
			}
		} else if (mon->hidpi_needs_scaling) {
			x *= mon->hidpi_scale_x;
			y *= mon->hidpi_scale_y;
		}

		// Map relative to the Amiga display area within the render target
		const SDL_Rect& rq = renderer->render_quad;
		if (rq.w > 0 && rq.h > 0) {
			float rx = (x - static_cast<float>(rq.x)) * 4095.0f / static_cast<float>(rq.w);
			float ry = (y - static_cast<float>(rq.y)) * 4095.0f / static_cast<float>(rq.h);
			*tx = static_cast<int>(std::clamp(rx, 0.0f, 4095.0f));
			*ty = static_cast<int>(std::clamp(ry, 0.0f, 4095.0f));
			return;
		}
	}

	// Fallback: map to full window
	int win_w, win_h;
	SDL_GetWindowSize(mon->amiga_window, &win_w, &win_h);
	*tx = (win_w > 0) ? static_cast<int>(std::clamp(x * 4095.0f / static_cast<float>(win_w), 0.0f, 4095.0f)) : 0;
	*ty = (win_h > 0) ? static_cast<int>(std::clamp(y * 4095.0f / static_cast<float>(win_h), 0.0f, 4095.0f)) : 0;
}

static void pen_send_current(const AmigaMonitor* mon, float x, float y)
{
	int tx, ty;
	pen_coords_to_tablet(mon, x, y, &tx, &ty);
	int pres = static_cast<int>(pen_pressure * 255.0f);
	int flags = pen_eraser ? 1 : 0;
	int ax = static_cast<int>((pen_xtilt + 90.0f) * 255.0f / 180.0f);
	int ay = static_cast<int>((pen_ytilt + 90.0f) * 255.0f / 180.0f);
	int az = static_cast<int>((pen_rotation + 180.0f) * 255.0f / 360.0f);
	send_tablet(tx, ty, 0, pres, pen_buttons, flags, ax, ay, az, 0, 0, 0, nullptr);
}

static void pen_position_via_mouse(const AmigaMonitor* mon, float x, float y)
{
	int32_t px = static_cast<int32_t>(x);
	int32_t py = static_cast<int32_t>(y);

	// Match mouse handler coordinate paths:
	// SDL renderer handles HiDPI + logical presentation in one call.
	// OpenGL uses cached per-monitor HiDPI scale factors.
	if (mon->amiga_renderer) {
		float rx, ry;
		if (SDL_RenderCoordinatesFromWindow(mon->amiga_renderer, x, y, &rx, &ry)) {
			px = static_cast<int32_t>(rx);
			py = static_cast<int32_t>(ry);
		}
	} else if (mon->hidpi_needs_scaling) {
		px = static_cast<int32_t>(x * mon->hidpi_scale_x);
		py = static_cast<int32_t>(y * mon->hidpi_scale_y);
	}

	setmousestate(0, 0, px, 1);
	setmousestate(0, 1, py, 1);
}

static void handle_pen_event(const SDL_Event& event)
{
	if (currprefs.input_tablet <= 0)
		return;

	const bool tablet_real = inputdevice_is_tablet() > 0;
	AmigaMonitor* mon = monitor_from_window_id(event.pproximity.windowID);

	switch (event.type) {
	case SDL_EVENT_PEN_PROXIMITY_IN:
		pen_in_proximity = 1;
		if (tablet_real)
			send_tablet_proximity(1);
		break;

	case SDL_EVENT_PEN_PROXIMITY_OUT:
		pen_in_proximity = 0;
		pen_pressure = 0;
		if (tablet_real)
			send_tablet_proximity(0);
		break;

	case SDL_EVENT_PEN_DOWN:
		pen_eraser = event.ptouch.eraser ? 1 : 0;
		pen_buttons |= 1;
		if (tablet_real) {
			pen_send_current(mon, event.ptouch.x, event.ptouch.y);
		} else {
			pen_position_via_mouse(mon, event.ptouch.x, event.ptouch.y);
			setmousebuttonstate(0, 0, 1);
		}
		break;

	case SDL_EVENT_PEN_UP:
		pen_pressure = 0;
		pen_buttons &= ~1u;
		if (tablet_real) {
			pen_send_current(mon, event.ptouch.x, event.ptouch.y);
		} else {
			pen_position_via_mouse(mon, event.ptouch.x, event.ptouch.y);
			setmousebuttonstate(0, 0, 0);
		}
		break;

	case SDL_EVENT_PEN_MOTION:
		if (tablet_real)
			pen_send_current(mon, event.pmotion.x, event.pmotion.y);
		else
			pen_position_via_mouse(mon, event.pmotion.x, event.pmotion.y);
		break;

	case SDL_EVENT_PEN_AXIS:
		switch (event.paxis.axis) {
		case SDL_PEN_AXIS_PRESSURE:
			pen_pressure = event.paxis.value;
			break;
		case SDL_PEN_AXIS_XTILT:
			pen_xtilt = event.paxis.value;
			break;
		case SDL_PEN_AXIS_YTILT:
			pen_ytilt = event.paxis.value;
			break;
		case SDL_PEN_AXIS_ROTATION:
			pen_rotation = event.paxis.value;
			break;
		default:
			break;
		}
		if (tablet_real)
			pen_send_current(mon, event.paxis.x, event.paxis.y);
		break;

	case SDL_EVENT_PEN_BUTTON_DOWN:
	{
		Uint8 btn = event.pbutton.button;
		if (tablet_real) {
			if (btn >= 1 && btn <= 5)
				pen_buttons |= (1u << (btn - 1));
			pen_send_current(mon, event.pbutton.x, event.pbutton.y);
		} else {
			pen_position_via_mouse(mon, event.pbutton.x, event.pbutton.y);
			if (btn == 1)
				setmousebuttonstate(0, 2, 1);
			else if (btn == 2)
				setmousebuttonstate(0, 1, 1);
		}
		break;
	}

	case SDL_EVENT_PEN_BUTTON_UP:
	{
		Uint8 btn = event.pbutton.button;
		if (tablet_real) {
			if (btn >= 1 && btn <= 5)
				pen_buttons &= ~(1u << (btn - 1));
			pen_send_current(mon, event.pbutton.x, event.pbutton.y);
		} else {
			pen_position_via_mouse(mon, event.pbutton.x, event.pbutton.y);
			if (btn == 1)
				setmousebuttonstate(0, 2, 0);
			else if (btn == 2)
				setmousebuttonstate(0, 1, 0);
		}
		break;
	}

	default:
		break;
	}
}
#endif

std::string get_filename_extension(const TCHAR* filename);

static void handle_drop_file_event(const SDL_Event& event)
{
	const char* dropped_file = event.drop.data;
	const auto ext = get_filename_extension(dropped_file);

	if (strcasecmp(ext.c_str(), ".uae") == 0)
	{
		// Load configuration file
		uae_restart(&currprefs, 1, dropped_file);
		gui_running = false;
	}
	else if (strcasecmp(ext.c_str(), ".adf") == 0 || strcasecmp(ext.c_str(), ".adz") == 0 || strcasecmp(ext.c_str(), ".dms") == 0 || strcasecmp(ext.c_str(), ".ipf") == 0 || strcasecmp(ext.c_str(), ".zip") == 0)
	{
		// Insert floppy image
		disk_insert(0, dropped_file);
	}
	else if (strcasecmp(ext.c_str(), ".lha") == 0)
	{
		// WHDLoad archive
		whdload_auto_prefs(&currprefs, dropped_file);
		uae_restart(&currprefs, 0, nullptr);
		gui_running = false;
	}
	else if (strcasecmp(ext.c_str(), ".cue") == 0 || strcasecmp(ext.c_str(), ".iso") == 0 || strcasecmp(ext.c_str(), ".chd") == 0)
	{
		// CD image
		cd_auto_prefs(&currprefs, const_cast<char*>(dropped_file));
		uae_restart(&currprefs, 0, nullptr);
		gui_running = false;
	}
}

static AmigaMonitor* monitor_from_window_id(SDL_WindowID window_id)
{
	SDL_Window* win = SDL_GetWindowFromID(window_id);
	if (win) {
		for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
			if (AMonitors[i].active && AMonitors[i].amiga_window == win)
				return &AMonitors[i];
		}
	}
	return &AMonitors[0];
}

static void process_event(const SDL_Event& event)
{
	AmigaMonitor* mon = &AMonitors[0];

	if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
		mon = monitor_from_window_id(event.window.windowID);
		if (mon->amiga_window && SDL_GetWindowFromID(event.window.windowID) == mon->amiga_window) {
			handle_window_event(event, mon);
			return;
		}
	}
	else
	{
		// Handle other types of events
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			handle_quit_event();
			break;

		case SDL_EVENT_WILL_ENTER_BACKGROUND:
		case SDL_EVENT_DID_ENTER_BACKGROUND:
			pause_sound();
			pause_emulation = 1;
			break;

		case SDL_EVENT_WILL_ENTER_FOREGROUND:
		case SDL_EVENT_DID_ENTER_FOREGROUND:
			pause_emulation = 0;
			resume_sound();
			break;

		case SDL_EVENT_CLIPBOARD_UPDATE:
			handle_clipboard_update_event();
			break;

#ifndef LIBRETRO
		case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
			// Display refresh-mode changed (user switched Hz in OS display
			// settings, or HDMI re-linked at a new mode). Force the hw VSync
			// pacing decision to re-probe without waiting for the window to
			// move or the Amiga target refresh to change.
			amiberry_hw_vsync_pacing_invalidate();
			break;
#endif

		case SDL_EVENT_JOYSTICK_ADDED:
			handle_joy_device_event(event.jdevice.which, false, &currprefs);
			break;
		case SDL_EVENT_JOYSTICK_REMOVED:
			handle_joy_device_event(event.jdevice.which, true, &currprefs);
			break;

		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			handle_controller_button_event(event);
			break;

		case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
		case SDL_EVENT_JOYSTICK_BUTTON_UP:
			handle_joy_button_event(event);
			break;

		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			handle_controller_axis_motion_event(event);
			break;

		case SDL_EVENT_JOYSTICK_AXIS_MOTION:
			handle_joy_axis_motion_event(event);
			break;

		case SDL_EVENT_JOYSTICK_HAT_MOTION:
			handle_joy_hat_motion_event(event);
			break;

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			handle_key_event(event);
			break;

		case SDL_EVENT_FINGER_DOWN:
		case SDL_EVENT_FINGER_UP:
		{
			mon = monitor_from_window_id(event.tfinger.windowID);
			mouse_monid = mon->monitor_id;
			int ww = 0, wh = 0;
			if (mon->amiga_window)
				SDL_GetWindowSize(mon->amiga_window, &ww, &wh);
			bool consumed = false;

			// Let ImGui on-screen keyboard consume the event first when visible.
			// OSK geometry is in drawable-pixel space, so convert touch coords
			// using pixel size (not window size) for HiDPI correctness.
			if (imgui_osk_is_active() && mon->amiga_window) {
				int pw = 0, ph = 0;
				SDL_GetWindowSizeInPixels(mon->amiga_window, &pw, &ph);
				float sx = event.tfinger.x * pw;
				float sy = event.tfinger.y * ph;
				int fid = static_cast<int>(event.tfinger.fingerID);
				if (event.type == SDL_EVENT_FINGER_DOWN)
					consumed = imgui_osk_handle_finger_down(sx, sy, fid);
				else
					consumed = imgui_osk_handle_finger_up(sx, sy, fid);
			}

			// Then let on-screen joystick try
			if (!consumed && on_screen_joystick_is_enabled() && ww > 0 && wh > 0) {
				if (event.type == SDL_EVENT_FINGER_DOWN)
					consumed = on_screen_joystick_handle_finger_down(event, ww, wh);
				else
					consumed = on_screen_joystick_handle_finger_up(event, ww, wh);
			}
			// Check if the on-screen keyboard button was tapped
			if (on_screen_joystick_keyboard_tapped()) {
				if (vkbd_allowed(0))
					imgui_osk_toggle();
			}
			if (!consumed)
				handle_finger_event(event);
			break;
		}

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			// Skip touch-synthesized mouse events when the on-screen keyboard or joystick is active,
			// otherwise touches also inject unwanted mouse input into Amiga port 1
			if ((imgui_osk_is_active() || on_screen_joystick_is_enabled()) && event.button.which == SDL_TOUCH_MOUSEID)
				break;
			mon = monitor_from_window_id(event.button.windowID);
			mouse_monid = mon->monitor_id;
			handle_mouse_button_event(event, mon);
			break;

		case SDL_EVENT_FINGER_MOTION:
		{
			mon = monitor_from_window_id(event.tfinger.windowID);
			mouse_monid = mon->monitor_id;
			int ww = 0, wh = 0;
			if (mon->amiga_window)
				SDL_GetWindowSize(mon->amiga_window, &ww, &wh);
			bool consumed = false;
			// Let ImGui on-screen keyboard consume motion first (drawable-pixel space)
			if (imgui_osk_is_active() && mon->amiga_window) {
				int pw = 0, ph = 0;
				SDL_GetWindowSizeInPixels(mon->amiga_window, &pw, &ph);
				float sx = event.tfinger.x * pw;
				float sy = event.tfinger.y * ph;
				int fid = static_cast<int>(event.tfinger.fingerID);
				consumed = imgui_osk_handle_finger_motion(sx, sy, fid);
			}
			if (!consumed && on_screen_joystick_is_enabled() && ww > 0 && wh > 0)
				consumed = on_screen_joystick_handle_finger_motion(event, ww, wh);
			if (!consumed)
				handle_finger_motion_event(event);
			break;
		}

		case SDL_EVENT_MOUSE_MOTION:
			// Skip touch-synthesized mouse events when the on-screen joystick is active,
			// otherwise D-pad touches also inject unwanted mouse input into Amiga port 1
			if (on_screen_joystick_is_enabled() && event.motion.which == SDL_TOUCH_MOUSEID)
				break;
			mon = monitor_from_window_id(event.motion.windowID);
			mouse_monid = mon->monitor_id;
			handle_mouse_motion_event(event, mon);
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			handle_mouse_wheel_event(event);
			break;

#ifndef LIBRETRO
		case SDL_EVENT_MOUSE_ADDED:
			handle_sdl_mouse_added(event.mdevice.which);
			break;
		case SDL_EVENT_MOUSE_REMOVED:
			handle_sdl_mouse_removed(event.mdevice.which);
			break;
#endif

		case SDL_EVENT_DROP_FILE:
			handle_drop_file_event(event);
			break;

#ifndef LIBRETRO
		case SDL_EVENT_PEN_PROXIMITY_IN:
		case SDL_EVENT_PEN_PROXIMITY_OUT:
		case SDL_EVENT_PEN_DOWN:
		case SDL_EVENT_PEN_UP:
		case SDL_EVENT_PEN_BUTTON_DOWN:
		case SDL_EVENT_PEN_BUTTON_UP:
		case SDL_EVENT_PEN_MOTION:
		case SDL_EVENT_PEN_AXIS:
			handle_pen_event(event);
			break;
#endif

		default:
			break;
		}
	}
}

void update_clipboard()
{
	osdep_platform_update_clipboard();
}

int handle_msgpump(bool vblank)
{
	if (!osdep_platform_use_event_pump())
		return 0;
	lctrl_pressed = rctrl_pressed = lalt_pressed = ralt_pressed = lshift_pressed = rshift_pressed = lgui_pressed = rgui_pressed = false;
	auto got_event = 0;
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		got_event = 1;
		process_event(event);
		if (currprefs.clipboard_sharing)
			update_clipboard();
	}
	return got_event;
}

bool handle_events()
{
	const AmigaMonitor* mon = &AMonitors[0];
	static auto was_paused = 0;

#ifdef USE_DBUS
	DBusHandle();
#endif
#ifdef USE_IPC_SOCKET
	Amiberry::IPC::IPCHandle();
#endif

	if (pause_emulation)
	{
		if (was_paused == 0)
		{
			setpaused(pause_emulation);
			was_paused = pause_emulation;
			gui_fps(0, gui_data.lines, gui_data.lace, 0, 0);
			gui_led(LED_SND, 0, -1);
			// we got just paused, report it to caller.
			return true;
		}
		lctrl_pressed = rctrl_pressed = lalt_pressed = ralt_pressed = lshift_pressed = rshift_pressed = lgui_pressed = rgui_pressed = false;
		if (osdep_platform_use_event_pump()) {
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				process_event(event);
			}
		}

		// Keyboard, mouse and joystick read events are handled in process_event in Amiberry
		//inputdevicefunc_keyboard.read();
		//inputdevicefunc_mouse.read();
		//inputdevicefunc_joystick.read();
		inputdevice_handle_inputcode();
	}
	if (was_paused && (!pause_emulation || quit_program))
	{
		updatedisplayarea(mon->monitor_id);
		pause_emulation = was_paused;
		resumepaused(was_paused);
		sound_closed = 0;
		was_paused = 0;
	}
	// Insert a 10ms delay to prevent 100% CPU usage
	if (pause_emulation && osdep_platform_should_delay_on_pause())
		SDL_Delay(10);
	return pause_emulation != 0;
}

void logging_init()
{
	if (amiberry_options.write_logfile)
	{
		static int first = 0;
		if (first > 1)
		{
			write_log("***** RESTART *****\n");
			return;
		}
		if (first == 1)
		{
			if (debugfile)
				fclose(debugfile);
			debugfile = nullptr;
		}

		if (!debugfile)
			debugfile = fopen(logfile_path.c_str(), "wt");

		logging_started = 1;
		first++;
		write_log("%s Logfile\n\n", VersionStr);
		write_log("%s\n", get_sdl_version_string().c_str());
		regstatus();
	}
}

void logging_cleanup()
{
	if (debugfile)
		fclose(debugfile);
	debugfile = nullptr;
}

static void reopen_active_logfile_if_needed()
{
	if (!amiberry_options.write_logfile || logfile_path.empty())
		return;

	if (debugfile)
		fclose(debugfile);
	debugfile = fopen(logfile_path.c_str(), "at");
	if (debugfile)
	{
		logging_started = 1;
		write_log("***** LOGFILE PATH UPDATED *****\n");
	}
}

uae_u8* save_log(int bootlog, size_t* len)
{
	uae_u8* dst = nullptr;

	if (!logging_started)
		return nullptr;
	FILE* f = fopen(logfile_path.c_str(), _T("rb"));
	if (!f)
		return nullptr;
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (*len > 0 && size > *len)
		size = *len;
	if (size > 0) {
		dst = xcalloc(uae_u8, size + 1);
		if (dst)
			fread(dst, 1, size, f);
		fclose(f);
		*len = size + 1;
	}
	return dst;
}

void strip_slashes(TCHAR* p)
{
	while (_tcslen(p) > 0 && (p[_tcslen(p) - 1] == '\\' || p[_tcslen(p) - 1] == '/'))
		p[_tcslen(p) - 1] = 0;
}

void fix_trailing(TCHAR* p)
{
	if (_tcslen(p) == 0)
		return;
	if (p[_tcslen(p) - 1] == '/' || p[_tcslen(p) - 1] == '\\')
		return;
#ifdef _WIN32
	_tcscat(p, "\\");
#else
	_tcscat(p, "/");
#endif
}

std::string fix_trailing(std::string& input)
{
	if (!input.empty() && input.back() != '/' && input.back() != '\\')
	{
#ifdef _WIN32
		return input + "\\";
#else
		return input + "/";
#endif
	}
	return input;
}

static std::string normalize_path_string(const std::string& input)
{
	if (input.empty())
		return {};

	const auto normalized = std::filesystem::path(input).lexically_normal().string();
	return normalized.empty() ? input : normalized;
}

static std::string join_path(const std::string& base, const std::string& leaf)
{
	if (base.empty())
		return {};

	return (std::filesystem::path(base) / leaf).string();
}

static void ensure_directory_exists(const std::string& directory_path)
{
	if (directory_path.empty())
		return;

	std::error_code ec;
	std::filesystem::create_directories(directory_path, ec);
}

static void ensure_parent_directory_exists(const std::string& file_path)
{
	if (file_path.empty())
		return;

	const auto parent_path = std::filesystem::path(file_path).parent_path();
	if (parent_path.empty())
		return;

	ensure_directory_exists(parent_path.string());
}

struct base_content_path_set
{
	std::string config_path;
	std::string controllers_path;
	std::string whdboot_path;
	std::string whdload_arch_path;
	std::string floppy_path;
	std::string harddrive_path;
	std::string cdrom_path;
	std::string logfile_path;
	std::string rom_path;
	std::string rp9_path;
	std::string saveimage_dir;
	std::string savestate_dir;
	std::string ripper_path;
	std::string input_dir;
	std::string screenshot_dir;
	std::string nvram_dir;
	std::string video_dir;
	std::string themes_path;
	std::string shaders_path;
	std::string bezels_path;
};

struct visual_asset_path_set
{
	std::string themes_path;
	std::string shaders_path;
	std::string bezels_path;
};

static const char* get_visual_assets_directory_name()
{
	return "Visuals";
}

static const char* get_configurations_directory_name()
{
	return "Configurations";
}

static const char* get_controllers_directory_name()
{
	return "Controllers";
}

static const char* get_whdboot_directory_name()
{
	return "WHDBoot";
}

static const char* get_whdload_archives_directory_name()
{
	return "LHA";
}

static const char* get_floppies_directory_name()
{
	return "Floppies";
}

static const char* get_harddrives_directory_name()
{
	return "HardDrives";
}

static const char* get_cdroms_directory_name()
{
	return "CDROMs";
}

static const char* get_roms_directory_name()
{
	return "ROMs";
}

static const char* get_rp9_directory_name()
{
	return "RP9";
}

static const char* get_savestates_directory_name()
{
	return "SaveStates";
}

static const char* get_ripper_directory_name()
{
	return "Ripper";
}

static const char* get_inputrecordings_directory_name()
{
	return "InputRecordings";
}

static const char* get_screenshots_directory_name()
{
	return "Screenshots";
}

static const char* get_nvram_directory_name()
{
	return "NVRAM";
}

static const char* get_videos_directory_name()
{
	return "Videos";
}

static const char* get_logfile_name()
{
	return "Amiberry.log";
}

static const char* get_themes_directory_name()
{
	return "Themes";
}

static const char* get_shaders_directory_name()
{
	return "Shaders";
}

static const char* get_bezels_directory_name()
{
	return "Bezels";
}

static visual_asset_path_set get_visual_asset_paths_from_content_root(const std::string& content_root)
{
	visual_asset_path_set paths;
	const auto normalized_root = normalize_path_string(content_root);
	if (normalized_root.empty())
		return paths;

	const auto visuals_root = join_path(normalized_root, get_visual_assets_directory_name());
	paths.themes_path = join_path(visuals_root, get_themes_directory_name());
	paths.shaders_path = join_path(visuals_root, get_shaders_directory_name());
	paths.bezels_path = join_path(visuals_root, get_bezels_directory_name());
	return paths;
}

static visual_asset_path_set get_legacy_visual_asset_paths_from_root(const std::string& legacy_root)
{
	visual_asset_path_set paths;
	const auto normalized_root = normalize_path_string(legacy_root);
	if (normalized_root.empty())
		return paths;

	paths.themes_path = join_path(normalized_root, get_themes_directory_name());
	paths.shaders_path = join_path(normalized_root, get_shaders_directory_name());
	paths.bezels_path = join_path(normalized_root, get_bezels_directory_name());
	return paths;
}

static void apply_visual_asset_paths(base_content_path_set& paths, const visual_asset_path_set& visual_paths)
{
	paths.themes_path = visual_paths.themes_path;
	paths.shaders_path = visual_paths.shaders_path;
	paths.bezels_path = visual_paths.bezels_path;
}

struct managed_path_option_descriptor
{
	const char* name;
	std::string base_content_path_set::*member;
};

static constexpr managed_path_option_descriptor base_content_managed_path_options[] = {
	{"config_path", &base_content_path_set::config_path},
	{"controllers_path", &base_content_path_set::controllers_path},
	{"whdboot_path", &base_content_path_set::whdboot_path},
	{"whdload_arch_path", &base_content_path_set::whdload_arch_path},
	{"floppy_path", &base_content_path_set::floppy_path},
	{"harddrive_path", &base_content_path_set::harddrive_path},
	{"cdrom_path", &base_content_path_set::cdrom_path},
	{"logfile_path", &base_content_path_set::logfile_path},
	{"rom_path", &base_content_path_set::rom_path},
	{"rp9_path", &base_content_path_set::rp9_path},
	{"saveimage_dir", &base_content_path_set::saveimage_dir},
	{"savestate_dir", &base_content_path_set::savestate_dir},
	{"ripper_path", &base_content_path_set::ripper_path},
	{"inputrecordings_dir", &base_content_path_set::input_dir},
	{"screenshot_dir", &base_content_path_set::screenshot_dir},
	{"nvram_dir", &base_content_path_set::nvram_dir},
	{"video_dir", &base_content_path_set::video_dir},
	{"themes_path", &base_content_path_set::themes_path},
	{"shaders_path", &base_content_path_set::shaders_path},
	{"bezels_path", &base_content_path_set::bezels_path},
};

static base_content_path_set default_base_content_paths;

static base_content_path_set get_base_content_path_set(const std::string& base_path)
{
	base_content_path_set paths;
	if (base_path.empty())
		return paths;

	const auto normalized_base = normalize_path_string(base_path);
	if (normalized_base.empty())
		return paths;

	paths.config_path = join_path(normalized_base, get_configurations_directory_name());
	paths.controllers_path = join_path(normalized_base, get_controllers_directory_name());
	paths.whdboot_path = join_path(normalized_base, get_whdboot_directory_name());
	paths.whdload_arch_path = join_path(normalized_base, get_whdload_archives_directory_name());
	paths.floppy_path = join_path(normalized_base, get_floppies_directory_name());
	paths.harddrive_path = join_path(normalized_base, get_harddrives_directory_name());
	paths.cdrom_path = join_path(normalized_base, get_cdroms_directory_name());
	paths.logfile_path = join_path(normalized_base, get_logfile_name());
	paths.rom_path = join_path(normalized_base, get_roms_directory_name());
	paths.rp9_path = join_path(normalized_base, get_rp9_directory_name());
	paths.saveimage_dir = normalized_base;
	paths.savestate_dir = join_path(normalized_base, get_savestates_directory_name());
	paths.ripper_path = join_path(normalized_base, get_ripper_directory_name());
	paths.input_dir = join_path(normalized_base, get_inputrecordings_directory_name());
	paths.screenshot_dir = join_path(normalized_base, get_screenshots_directory_name());
	paths.nvram_dir = join_path(normalized_base, get_nvram_directory_name());
	paths.video_dir = join_path(normalized_base, get_videos_directory_name());
	apply_visual_asset_paths(paths, get_visual_asset_paths_from_content_root(normalized_base));
	return paths;
}

static base_content_path_set get_legacy_base_content_path_set(const std::string& base_path)
{
	auto paths = get_base_content_path_set(base_path);
	if (base_path.empty())
		return paths;

	apply_visual_asset_paths(paths, get_legacy_visual_asset_paths_from_root(paths.config_path));
	return paths;
}

static visual_asset_path_set get_current_visual_asset_path_set()
{
	visual_asset_path_set paths;
	paths.themes_path = themes_path;
	paths.shaders_path = shaders_path;
	paths.bezels_path = bezels_path;
	return paths;
}

static visual_asset_path_set get_visual_asset_path_set(const base_content_path_set& paths)
{
	visual_asset_path_set visual_paths;
	visual_paths.themes_path = paths.themes_path;
	visual_paths.shaders_path = paths.shaders_path;
	visual_paths.bezels_path = paths.bezels_path;
	return visual_paths;
}

static void apply_current_visual_asset_paths(const visual_asset_path_set& paths)
{
	themes_path = paths.themes_path;
	shaders_path = paths.shaders_path;
	bezels_path = paths.bezels_path;
}

std::string get_config_directory(bool portable_mode);
static std::string get_xdg_config_home();

static const managed_path_option_descriptor* const themes_path_descriptor = &base_content_managed_path_options[17];
static const managed_path_option_descriptor* const shaders_path_descriptor = &base_content_managed_path_options[18];
static const managed_path_option_descriptor* const bezels_path_descriptor = &base_content_managed_path_options[19];

static std::string get_current_visual_assets_root_path()
{
	if (!themes_path.empty())
	{
		const auto visuals_root = normalize_path_string(std::filesystem::path(themes_path).parent_path().string());
		if (!visuals_root.empty())
			return visuals_root;
	}

	if (!home_dir.empty())
		return join_path(home_dir, get_visual_assets_directory_name());

	return {};
}

static bool is_visual_managed_path_option(const managed_path_option_descriptor* descriptor)
{
	return descriptor == themes_path_descriptor
		|| descriptor == shaders_path_descriptor
		|| descriptor == bezels_path_descriptor;
}

static std::string get_visual_asset_path_for_descriptor(const visual_asset_path_set& paths,
	const managed_path_option_descriptor* descriptor)
{
	if (descriptor == themes_path_descriptor)
		return paths.themes_path;
	if (descriptor == shaders_path_descriptor)
		return paths.shaders_path;
	if (descriptor == bezels_path_descriptor)
		return paths.bezels_path;
	return {};
}

static visual_asset_path_set get_legacy_default_visual_asset_paths(const bool portable_mode)
{
#if defined(__MACH__) || defined(_WIN32) || defined(__ANDROID__)
	return get_legacy_visual_asset_paths_from_root(get_config_directory(portable_mode));
#else
	if (portable_mode)
		return get_legacy_visual_asset_paths_from_root(get_config_directory(true));
	return get_legacy_visual_asset_paths_from_root(get_xdg_config_home() + "/amiberry");
#endif
}

static base_content_path_set get_current_base_content_path_set()
{
	base_content_path_set paths;
	paths.config_path = config_path;
	paths.controllers_path = controllers_path;
	paths.whdboot_path = whdboot_path;
	paths.whdload_arch_path = whdload_arch_path;
	paths.floppy_path = floppy_path;
	paths.harddrive_path = harddrive_path;
	paths.cdrom_path = cdrom_path;
	paths.logfile_path = logfile_path;
	paths.rom_path = rom_path;
	paths.rp9_path = rp9_path;
	paths.saveimage_dir = saveimage_dir;
	paths.savestate_dir = savestate_dir;
	paths.ripper_path = ripper_path;
	paths.input_dir = input_dir;
	paths.screenshot_dir = screenshot_dir;
	paths.nvram_dir = nvram_dir;
	paths.video_dir = video_dir;
	paths.themes_path = themes_path;
	paths.shaders_path = shaders_path;
	paths.bezels_path = bezels_path;
	return paths;
}

static std::string normalize_path_for_compare(const std::string& input)
{
	if (input.empty())
		return {};

	auto normalized = std::filesystem::path(input).lexically_normal().generic_string();
	if (normalized.empty())
		normalized = input;

	while (normalized.size() > 1 && normalized.back() == '/')
	{
#ifdef _WIN32
		if (normalized.size() == 3
			&& std::isalpha(static_cast<unsigned char>(normalized[0]))
			&& normalized[1] == ':'
			&& normalized[2] == '/')
		{
			break;
		}
#endif
		normalized.pop_back();
	}

	return normalized;
}

static bool path_strings_match(const std::string& lhs, const std::string& rhs)
{
	return normalize_path_for_compare(lhs) == normalize_path_for_compare(rhs);
}

static std::string lowercase_path_for_compare(const std::string& path)
{
	auto normalized = normalize_path_for_compare(path);
	std::transform(normalized.begin(), normalized.end(), normalized.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return normalized;
}

static bool path_strings_match_case_insensitive(const std::string& lhs, const std::string& rhs)
{
	return lowercase_path_for_compare(lhs) == lowercase_path_for_compare(rhs);
}

static base_content_path_set get_base_content_override_baseline()
{
	if (!base_content_path.empty())
		return get_base_content_path_set(base_content_path);
	return default_base_content_paths;
}

static base_content_path_set merge_base_content_path_overrides(const base_content_path_set& target_paths,
	const base_content_path_set& previous_paths,
	const base_content_path_set& baseline_paths)
{
	auto merged_paths = target_paths;
	for (const auto& descriptor : base_content_managed_path_options)
	{
		const auto& previous_value = previous_paths.*(descriptor.member);
		const auto& baseline_value = baseline_paths.*(descriptor.member);
		if (!path_strings_match(previous_value, baseline_value))
			merged_paths.*(descriptor.member) = previous_value;
	}
	return merged_paths;
}

static std::string try_extract_base_content_root(const std::string& value, const std::string& suffix)
{
	const auto normalized_value = normalize_path_for_compare(value);
	if (normalized_value.empty())
		return {};

	const auto normalized_suffix = normalize_path_for_compare(suffix);
	if (normalized_suffix.empty())
		return normalize_path_string(normalized_value);

	const auto value_for_compare = lowercase_path_for_compare(normalized_value);
	const auto suffix_with_separator = "/" + lowercase_path_for_compare(normalized_suffix);
	if (value_for_compare.size() <= suffix_with_separator.size())
		return {};
	if (value_for_compare.compare(value_for_compare.size() - suffix_with_separator.size(),
		suffix_with_separator.size(), suffix_with_separator) != 0)
		return {};

	return normalize_path_string(normalized_value.substr(0,
		normalized_value.size() - suffix_with_separator.size()));
}

static std::string get_base_content_path_suffix(const std::string& placeholder_path, const std::string& placeholder_base)
{
	const auto normalized_path = normalize_path_for_compare(placeholder_path);
	if (normalized_path.empty())
		return {};

	const auto normalized_base = normalize_path_for_compare(placeholder_base);
	if (normalized_base.empty())
		return normalized_path;

	const auto path_for_compare = lowercase_path_for_compare(normalized_path);
	const auto base_for_compare = lowercase_path_for_compare(normalized_base);
	if (path_for_compare == base_for_compare)
		return {};

	const auto base_with_separator = base_for_compare + "/";
	if (path_for_compare.rfind(base_with_separator, 0) == 0)
		return normalize_path_string(normalized_path.substr(normalized_base.size() + 1));

	return normalized_path;
}

static const managed_path_option_descriptor* find_base_content_managed_path_option(const TCHAR* option)
{
	for (const auto& descriptor : base_content_managed_path_options)
	{
		if (!_tcscmp(option, descriptor.name))
			return &descriptor;
	}
	return nullptr;
}

struct managed_path_option_line
{
	const managed_path_option_descriptor* descriptor{};
	std::string value;
};

static std::string infer_serialized_base_content_path(const std::vector<managed_path_option_line>& managed_path_lines)
{
	if (managed_path_lines.empty())
		return {};

	constexpr auto placeholder_base = "__amiberry_base__";
	const auto suffix_paths = get_base_content_path_set(placeholder_base);
	std::map<std::string, int> candidate_counts;

	for (const auto& line : managed_path_lines)
	{
		if (line.descriptor == nullptr)
			continue;

		const auto suffix = get_base_content_path_suffix(suffix_paths.*(line.descriptor->member), placeholder_base);
		const auto candidate_root = try_extract_base_content_root(line.value, suffix);
		if (!candidate_root.empty())
			++candidate_counts[candidate_root];
	}

	std::string best_candidate;
	int best_count = 0;
	for (const auto& [candidate, count] : candidate_counts)
	{
		if (count > best_count)
		{
			best_candidate = candidate;
			best_count = count;
		}
	}

	return best_count >= 3 ? best_candidate : std::string{};
}

static bool managed_path_line_matches_visual_paths(const managed_path_option_line& line,
	const visual_asset_path_set& visual_paths)
{
	if (!is_visual_managed_path_option(line.descriptor))
		return false;

	return path_strings_match(line.value,
		get_visual_asset_path_for_descriptor(visual_paths, line.descriptor));
}

static bool any_managed_path_line_matches_visual_paths(const std::vector<managed_path_option_line>& managed_path_lines,
	const visual_asset_path_set& visual_paths)
{
	for (const auto& line : managed_path_lines)
	{
		if (managed_path_line_matches_visual_paths(line, visual_paths))
			return true;
	}
	return false;
}

static std::string infer_content_root_from_configuration_path(const std::string& configuration_path)
{
	constexpr auto placeholder_base = "__amiberry_base__";
	const auto placeholder_paths = get_base_content_path_set(placeholder_base);
	const auto suffix = get_base_content_path_suffix(placeholder_paths.config_path, placeholder_base);
	return try_extract_base_content_root(configuration_path, suffix);
}

static std::vector<std::string> get_base_content_directories(const base_content_path_set& paths)
{
	std::vector<std::string> directories;
	const std::string candidates[] = {
		paths.saveimage_dir,
		paths.config_path,
		paths.controllers_path,
		paths.whdboot_path,
		paths.whdload_arch_path,
		paths.floppy_path,
		paths.harddrive_path,
		paths.cdrom_path,
		paths.rom_path,
		paths.rp9_path,
		paths.savestate_dir,
		paths.ripper_path,
		paths.input_dir,
		paths.screenshot_dir,
		paths.nvram_dir,
		paths.video_dir,
		paths.themes_path,
		paths.shaders_path,
		paths.bezels_path,
	};

	for (const auto& candidate : candidates)
	{
		if (candidate.empty())
			continue;

		bool already_added = false;
		for (const auto& existing : directories)
		{
			if (path_strings_match(existing, candidate))
			{
				already_added = true;
				break;
			}
		}
		if (!already_added)
			directories.emplace_back(candidate);
	}
	return directories;
}

static base_content_path_set preview_base_content_path_set(const std::string& base_path)
{
	const auto previous_paths = get_current_base_content_path_set();
	const auto previous_baseline_paths = get_base_content_override_baseline();

	if (base_path.empty())
		return merge_base_content_path_overrides(default_base_content_paths,
			previous_paths, previous_baseline_paths);

	const auto normalized_base = normalize_path_string(base_path);
	if (normalized_base.empty())
		return previous_paths;

	return merge_base_content_path_overrides(get_base_content_path_set(normalized_base),
		previous_paths, previous_baseline_paths);
}

static void create_base_content_root_directory(const std::string& base_path)
{
	const auto normalized_base = normalize_path_string(base_path);
	if (normalized_base.empty())
		return;

	std::error_code ec;
	std::filesystem::create_directories(normalized_base, ec);
}

static void apply_base_content_path_set(const base_content_path_set& paths)
{
	config_path = paths.config_path;
	controllers_path = paths.controllers_path;
	whdboot_path = paths.whdboot_path;
	whdload_arch_path = paths.whdload_arch_path;
	floppy_path = paths.floppy_path;
	harddrive_path = paths.harddrive_path;
	cdrom_path = paths.cdrom_path;
	logfile_path = paths.logfile_path;
	rom_path = paths.rom_path;
	rp9_path = paths.rp9_path;
	saveimage_dir = paths.saveimage_dir;
	savestate_dir = paths.savestate_dir;
	ripper_path = paths.ripper_path;
	input_dir = paths.input_dir;
	screenshot_dir = paths.screenshot_dir;
	nvram_dir = paths.nvram_dir;
	video_dir = paths.video_dir;
	themes_path = paths.themes_path;
	shaders_path = paths.shaders_path;
	bezels_path = paths.bezels_path;
}

static void apply_base_content_path_defaults(const std::string& base_path)
{
	if (base_path.empty())
		return;

	const auto normalized_base = normalize_path_string(base_path);
	if (normalized_base.empty())
		return;

	base_content_path = normalized_base;
	current_dir = home_dir = normalized_base;
	apply_base_content_path_set(get_base_content_path_set(normalized_base));
}

static void apply_base_content_path_preserving_overrides(const std::string& base_path)
{
	if (base_path.empty())
		return;

	const auto normalized_base = normalize_path_string(base_path);
	if (normalized_base.empty())
		return;

	const auto previous_paths = get_current_base_content_path_set();
	const auto previous_baseline_paths = get_base_content_override_baseline();

	apply_base_content_path_defaults(normalized_base);
	const auto updated_paths = get_current_base_content_path_set();
	apply_base_content_path_set(merge_base_content_path_overrides(updated_paths,
		previous_paths, previous_baseline_paths));
}

static void clear_base_content_path_preserving_overrides()
{
	if (base_content_path.empty())
		return;

	const auto previous_paths = get_current_base_content_path_set();
	const auto previous_baseline_paths = get_base_content_override_baseline();

	const auto previous_retroarch_file = retroarch_file;
	const auto previous_floppy_sounds_dir = floppy_sounds_dir;
	const auto previous_plugins_dir = plugins_dir;

	base_content_path.clear();
	init_amiberry_dirs(g_portable_mode);

	retroarch_file = previous_retroarch_file;
	floppy_sounds_dir = previous_floppy_sounds_dir;
	plugins_dir = previous_plugins_dir;
	apply_base_content_path_set(merge_base_content_path_overrides(get_current_base_content_path_set(),
		previous_paths, previous_baseline_paths));
}

// convert path to absolute
void fullpath(TCHAR* path, int size, bool userelative)
{
	// Resolve absolute path
	TCHAR tmp1[MAX_DPATH];
	tmp1[0] = 0;
#ifdef _WIN32
	if (_fullpath(tmp1, path, MAX_DPATH) != nullptr)
#else
	if (realpath(path, tmp1) != nullptr)
#endif
	{
		if (_tcsnicmp(path, tmp1, _tcslen(tmp1)) != 0)
			_tcscpy(path, tmp1);
	}
	else if (path[0] != 0)
	{
		write_log("fullpath: realpath() failed for '%s' (errno=%d), using path as-is.\n", path, errno);
	}
}

// convert path to absolute
void fullpath(TCHAR* path, const int size)
{
	fullpath(path, size, relativepaths);
}

bool target_isrelativemode()
{
	return relativepaths != 0;
}

bool samepath(const TCHAR* p1, const TCHAR* p2)
{
	if (!_tcsicmp(p1, p2))
		return true;
	return false;
}

void getpathpart(TCHAR* outpath, int size, const TCHAR* inpath)
{
	if (!outpath || size <= 0)
		return;
	outpath[0] = 0;
	if (!inpath)
		return;
	_tcsncpy(outpath, inpath, size - 1);
	outpath[size - 1] = 0;
	auto* const p = _tcsrchr(outpath, '/');
	if (p)
		p[0] = 0;
	fix_trailing(outpath);
}

void getfilepart(TCHAR* out, int size, const TCHAR* path)
{
	if (!out || size <= 0)
		return;
	out[0] = 0;
	if (!path)
		return;
	const auto* const p = _tcsrchr(path, '/');
	if (p)
		_tcsncpy(out, p + 1, size - 1);
	else
		_tcsncpy(out, path, size - 1);
	out[size - 1] = 0;
}

uae_u8* target_load_keyfile(uae_prefs* p, const char* path, int* sizep, char* name)
{
	return nullptr;
}

void replace(std::string& str, const std::string& from, const std::string& to)
{
	const auto start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return;
	str.replace(start_pos, from.length(), to);
}

bool target_execute(const char* command)
{
	if (!command)
		return false;

	AmigaMonitor* mon = &AMonitors[0];
	releasecapture(mon);
	mouseactive = 0;

	write_log("Target_execute received: %s\n", command);
	
	std::string final_command = command;
	// Ensure this runs in the background
	final_command += " &";

	try
	{
		write_log("Executing: %s\n", final_command.c_str());
	#ifndef AMIBERRY_IOS
		const int result = system(final_command.c_str());
		if (result != 0) {
			write_log("system() failed when trying to execute: %s. Result: %d\n", command, result);
			return false;
		}
	#else
		write_log("system() not available on iOS\n");
		return false;
	#endif
	}
	catch (const std::exception& e)
	{
		write_log("Exception thrown when trying to execute: %s. Exception: %s", command, e.what());
		return false;
	}
	return true;
}

void target_run()
{
	// Reset counter for access violations
	init_max_signals();
}

void target_quit()
{
	cancel_async_update_check();

#ifdef __ANDROID__
	// Write a clean-exit marker so the Kotlin launcher knows this was
	// an intentional quit, not a crash.  SDL3's SDLActivity calls
	// System.exit(0) after native main() returns, which kills the
	// process without running onDestroy() — so marker cleanup from
	// Java never executes.  Write from native code instead.
	const char* ext = SDL_GetAndroidExternalStoragePath();
	if (ext) {
		std::string marker = std::string(ext) + "/.clean_exit";
		FILE* f = fopen(marker.c_str(), "w");
		if (f) {
			fputs("1", f);
			fclose(f);
		}
		std::string session = std::string(ext) + "/.emulator_session";
		remove(session.c_str());
	}
#endif
}

void target_fixup_options(uae_prefs* p)
{
	if (p->automount_cddrives && !p->scsi)
		p->scsi = 1;
	if (p->uaescsimode > UAESCSI_LAST)
		p->uaescsimode = UAESCSI_SPTI;
	bool paused = false;
	bool nosound = false;
	bool nojoy = true;
	if (!paused) {
		paused = p->active_nocapture_pause;
		nosound = p->active_nocapture_nosound;
	}
	else {
		p->active_nocapture_pause = p->active_nocapture_nosound = true;
		nosound = true;
		nojoy = false;
	}
	if (!paused) {
		paused = p->inactive_pause;
		nosound = p->inactive_nosound;
		nojoy = (p->inactive_input & 4) == 0;
	}
	else {
		p->inactive_pause = p->inactive_nosound = true;
		p->inactive_input = 0;
		nosound = true;
		nojoy = true;
	}
	
#ifdef AMIBERRY
	// Some old configs might have lower values there. Ensure they are updated
	p->gfx_api = std::max(p->gfx_api, 2);

	if (p->gfx_auto_crop)
	{
		// Make sure that Auto-Center is disabled
		p->gfx_xcenter = p->gfx_ycenter = 0;
	}
#ifdef WITH_THREADED_CPU
	// JIT and CPU Thread can now work together.
	// cpu_thread_run_jit() handles JIT execution on a separate thread,
	// while run_cpu_thread() manages cycle timing on the main thread.
#endif
#endif
	
	if (p->rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE) {
		p->rtg_hardwareinterrupt = false;
		p->rtg_hardwaresprite = false;
	}

	const MultiDisplay* md = getdisplay(p, 0);
	for (auto & j : p->gfx_monitor) {
		if (j.gfx_size_fs.special == WH_NATIVE) {
			int i;
			for (i = 0; md->DisplayModes[i].inuse; i++) {
				if (md->DisplayModes[i].res.width == md->rect.w - md->rect.x &&
					md->DisplayModes[i].res.height == md->rect.h - md->rect.y) {
					j.gfx_size_fs.width = md->DisplayModes[i].res.width;
					j.gfx_size_fs.height = md->DisplayModes[i].res.height;
					write_log(_T("Native resolution: %dx%d\n"), j.gfx_size_fs.width, j.gfx_size_fs.height);
					break;
				}
			}
			if (!md->DisplayModes[i].inuse) {
				j.gfx_size_fs.special = 0;
				write_log(_T("Native resolution not found.\n"));
			}
		}
	}

	if ((p->gfx_apmode[0].gfx_vsyncmode || p->gfx_apmode[1].gfx_vsyncmode)) {
		if (p->produce_sound && sound_devices[p->soundcard]->type == SOUND_DEVICE_SDL2) {
			p->soundcard = 0;
		}
	}
#ifdef AMIBERRY
	// Initialize Drawbridge if needed
	for (const auto& floppyslot : p->floppyslots)
	{
		if (floppyslot.dfxtype == DRV_FB)
			drawbridge_update_profiles(p);
	}

	set_key_configs(p);
#endif
}

void target_default_options(uae_prefs* p, const int type)
{
	//TCHAR buf[MAX_DPATH];
	if (type == 2 || type == 0 || type == 3) {
		//p->logfile = 0;
		p->active_nocapture_pause = false;
		p->active_nocapture_nosound = false;
		p->minimized_nosound = true;
		p->minimized_pause = true;
		p->minimized_input = 0;
		p->inactive_nosound = false;
		p->inactive_pause = false;
		p->inactive_input = 0;
		//p->ctrl_F11_is_quit = 0;
		p->soundcard = 0;
		p->samplersoundcard = -1;
		p->minimize_inactive = false;
		p->capture_always = true;
		p->start_minimized = false;
		p->start_uncaptured = false;
		p->active_capture_priority = 1;
		p->inactive_priority = 0;
		p->minimized_priority = 0;
		//p->notaskbarbutton = false;
		//p->nonotificationicon = false;
		p->main_alwaysontop = false;
		p->gui_alwaysontop = false;
		//p->guikey = -1;
		p->automount_removable = false;
		//p->automount_drives = 0;
		//p->automount_removabledrives = 0;
		p->automount_cddrives = false;
		//p->automount_netdrives = 0;
		//p->kbledmode = 1;
		p->uaescsimode = UAESCSI_CDEMU;
		p->borderless = false;
		p->blankmonitors = false;
		//p->powersavedisabled = true;
		p->sana2 = false;
		p->gf[GF_RTG].gfx_filter_autoscale = RTG_MODE_SCALE;
		p->rtgallowscaling = false;
		p->rtgscaleaspectratio = -1;
		p->rtgvblankrate = 0;
		p->rtg_hardwaresprite = false;
		p->rtg_overlay = true;
		p->rtg_vgascreensplit = true;
		p->rtg_paletteswitch = true;
		p->rtg_dacswitch = true;
		//p->commandpathstart[0] = 0;
		//p->commandpathend[0] = 0;
		//p->statusbar = 1;
	#ifdef USE_VULKAN
	p->gfx_api = 5;
#else
	p->gfx_api = 4;
#endif
		if (p->gf[GF_NORMAL].gfx_filter == 0)
			p->gf[GF_NORMAL].gfx_filter = 1;
		if (p->gf[GF_RTG].gfx_filter == 0)
			p->gf[GF_RTG].gfx_filter = 1;
		//WIN32GUI_LoadUIString(IDS_INPUT_CUSTOM, buf, sizeof buf / sizeof(TCHAR));
		//for (int i = 0; i < GAMEPORT_INPUT_SETTINGS; i++)
		//	_stprintf(p->input_config_name[i], buf, i + 1);
		//p->aviout_xoffset = -1;
		//p->aviout_yoffset = -1;
	}
	if (type == 1 || type == 0 || type == 3) {
		p->uaescsimode = UAESCSI_CDEMU;
		//p->midirouter = false;
		p->automount_removable = false;
		//p->automount_drives = 0;
		//p->automount_removabledrives = 0;
		p->automount_cddrives = false;
		//p->automount_netdrives = 0;
		p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_R8G8B8A8;
		//p->filesystem_mangle_reserved_names = true;
	}

	multithread_enabled = true;

	p->kbd_led_num = -1; // No status on numlock
	p->kbd_led_scr = -1; // No status on scrollock
	p->kbd_led_cap = -1;

	p->gfx_monitor[0].gfx_size_win.width = amiberry_options.default_width;
	p->gfx_monitor[0].gfx_size_win.height = amiberry_options.default_height;

	p->gfx_manual_crop = false;
	p->gfx_manual_crop_width = AMIGA_WIDTH_MAX << p->gfx_resolution;
	p->gfx_manual_crop_height = AMIGA_HEIGHT_MAX << p->gfx_vresolution;
	p->gfx_horizontal_offset = 0;
	p->gfx_vertical_offset = 0;
	if (amiberry_options.default_auto_crop)
	{
		p->gfx_auto_crop = amiberry_options.default_auto_crop;
	}
	
	p->gfx_correct_aspect = amiberry_options.default_correct_aspect_ratio;

	// GFX_WINDOW = 0
	// GFX_FULLSCREEN = 1
	// GFX_FULLWINDOW = 2
	if (amiberry_options.default_fullscreen_mode >= 0 && amiberry_options.default_fullscreen_mode <= 2)
	{
		p->gfx_apmode[0].gfx_fullscreen = amiberry_options.default_fullscreen_mode;
		p->gfx_apmode[1].gfx_fullscreen = amiberry_options.default_fullscreen_mode;
	}
	else
	{
		p->gfx_apmode[0].gfx_fullscreen = GFX_WINDOW;
		p->gfx_apmode[1].gfx_fullscreen = GFX_WINDOW;
	}

	//Default is Auto
	p->scaling_method = -1; 
	if (amiberry_options.default_scaling_method != -1)
	{
		// only valid values are -1 (Auto), 0 (Nearest), 1 (Linear), 2 (Integer)
		if (amiberry_options.default_scaling_method >= 0 && amiberry_options.default_scaling_method <= 2)
			p->scaling_method = amiberry_options.default_scaling_method;
	}

	if (amiberry_options.default_gfx_autoresolution)
	{
		p->gfx_autoresolution = amiberry_options.default_gfx_autoresolution;
	}

	_tcscpy(p->shader, amiberry_options.shader[0] ? amiberry_options.shader : _T("none"));
	_tcscpy(p->shader_rtg, amiberry_options.shader_rtg[0] ? amiberry_options.shader_rtg : _T("none"));
	_tcscpy(p->custom_bezel, amiberry_options.custom_bezel[0] ? amiberry_options.custom_bezel : _T("none"));
	p->use_custom_bezel = amiberry_options.use_custom_bezel
		&& _tcsicmp(p->custom_bezel, _T("none")) != 0;
	p->use_bezel = p->use_custom_bezel ? false : amiberry_options.use_bezel;

	if (amiberry_options.default_line_mode == 1)
	{
		// Double line mode
		p->gfx_vresolution = VRES_DOUBLE;
		p->gfx_pscanlines = 0;
	}
	else if (amiberry_options.default_line_mode == 2)
	{
		// Scanlines line mode
		p->gfx_vresolution = VRES_DOUBLE;
		p->gfx_pscanlines = 1;
	}
	else
	{
		// Single line mode
		p->gfx_vresolution = VRES_NONDOUBLE;
		p->gfx_pscanlines = 0;
	}

	if (amiberry_options.default_horizontal_centering)
		p->gfx_xcenter = 2;

	if (amiberry_options.default_vertical_centering)
		p->gfx_ycenter = 2;

	if (amiberry_options.default_frameskip)
		p->gfx_framerate = 2;

	p->soundcard_default = true;

	if (amiberry_options.default_stereo_separation >= 0 && amiberry_options.default_stereo_separation <= 10)
		p->sound_stereo_separation = amiberry_options.default_stereo_separation;

	if (amiberry_options.default_sound_buffer > 0 && amiberry_options.default_sound_buffer <= 65536)
		p->sound_maxbsiz = amiberry_options.default_sound_buffer;

	if (amiberry_options.default_sound_frequency > 0 && amiberry_options.default_sound_frequency <= 48000)
		p->sound_freq = amiberry_options.default_sound_frequency;

	if (amiberry_options.default_joystick_deadzone >= 0
		&& amiberry_options.default_joystick_deadzone <= 100
		&& amiberry_options.default_joystick_deadzone != 33)
	{
		p->input_joymouse_deadzone = amiberry_options.default_joystick_deadzone;
		p->input_joystick_deadzone = amiberry_options.default_joystick_deadzone;
	}
	
	_tcscpy(p->open_gui, amiberry_options.default_open_gui_key);
	_tcscpy(p->quit_amiberry, amiberry_options.default_quit_key);
	_tcscpy(p->action_replay, amiberry_options.default_ar_key);
	_tcscpy(p->fullscreen_toggle, amiberry_options.default_fullscreen_toggle_key);

	p->drawbridge_serial_auto = true;
	p->drawbridge_smartspeed = false;
	p->drawbridge_autocache = false;
	p->drawbridge_drive_cable = 0;
	p->drawbridge_driver = 0;

	drawbridge_update_profiles(p);

	p->alt_tab_release = false;
	p->ctrl_alt_release = true;
	p->sound_pullmode = 0;

	p->use_retroarch_quit = amiberry_options.default_retroarch_quit;
	p->use_retroarch_menu = amiberry_options.default_retroarch_menu;
	p->use_retroarch_reset = amiberry_options.default_retroarch_reset;
	p->use_retroarch_vkbd = amiberry_options.default_retroarch_vkbd;

	// Default IDs for ports 0 and 1: Mouse and first joystick
	p->jports[0].id = JSEM_MICE;
	p->jports[1].id = JSEM_JOYS;

	whdload_prefs.button_wait = amiberry_options.default_whd_buttonwait;
	whdload_prefs.show_splash = amiberry_options.default_whd_showsplash;
	whdload_prefs.config_delay = amiberry_options.default_whd_configdelay;
	whdload_prefs.write_cache = amiberry_options.default_whd_writecache;
	whdload_prefs.quit_on_exit = amiberry_options.default_whd_quit_on_exit;

	if (amiberry_options.default_soundcard > 0) p->soundcard = amiberry_options.default_soundcard;

	// On-screen joystick
	p->onscreen_joystick = amiberry_options.default_onscreen_joystick;

	// Virtual keyboard default options
	p->vkbd_enabled = amiberry_options.default_vkbd_enabled;

#ifdef __ANDROID__
	// Touch-only Android: enable on-screen controls by default. If a physical
	// gamepad is already connected at startup, disable them so they don't
	// obscure the emulation for users who have real hardware. User overrides
	// from a loaded .uae config still take precedence (applied after defaults).
	if (SDL_HasJoystick()) {
		p->onscreen_joystick = false;
		p->vkbd_enabled = false;
	}
#endif
	p->vkbd_exit = amiberry_options.default_vkbd_exit;
	p->vkbd_hires = amiberry_options.default_vkbd_hires;
	if (amiberry_options.default_vkbd_language[0])
		_tcscpy(p->vkbd_language, amiberry_options.default_vkbd_language);
	else
		_tcscpy(p->vkbd_language, ""); // This will use the default language.
	if (amiberry_options.default_vkbd_style[0])
		_tcscpy(p->vkbd_style, amiberry_options.default_vkbd_style);
	else
		_tcscpy(p->vkbd_style, ""); // This will use the default theme.
	p->vkbd_transparency = amiberry_options.default_vkbd_transparency;
	_tcscpy(p->vkbd_toggle, amiberry_options.default_vkbd_toggle);

	// Initialize multipath search directories from Amiberry's configured paths.
	// This allows configs to use bare filenames (e.g. "game.hdf") which get
	// resolved against the default directories automatically.
	auto rom = get_rom_path();
	if (!rom.empty())
		_tcsncpy(p->path_rom.path[0], rom.c_str(), PATH_MAX - 1);
	auto floppy = get_floppy_path();
	if (!floppy.empty())
		_tcsncpy(p->path_floppy.path[0], floppy.c_str(), PATH_MAX - 1);
	auto hd = get_harddrive_path();
	if (!hd.empty())
		_tcsncpy(p->path_hardfile.path[0], hd.c_str(), PATH_MAX - 1);
	auto cd = get_cdrom_path();
	if (!cd.empty())
		_tcsncpy(p->path_cd.path[0], cd.c_str(), PATH_MAX - 1);
}

static const TCHAR* scsimode[] = { _T("SCSIEMU"), _T("SPTI"), _T("SPTI+SCSISCAN"), nullptr};
//static const TCHAR* statusbarmode[] = { _T("none"), _T("normal"), _T("extended"), NULL };
//static const TCHAR* configmult[] = { _T("1x"), _T("2x"), _T("3x"), _T("4x"), _T("5x"), _T("6x"), _T("7x"), _T("8x"), NULL };

//static struct midiportinfo *getmidiport (struct midiportinfo **mi, int devid)
//{
//	for (int i = 0; i < MAX_MIDI_PORTS; i++) {
//		if (mi[i] != NULL && mi[i]->devid == devid)
//			return mi[i];
//	}
//	return NULL;
//}

int scsiromselected = 0;
int scsiromselectednum = 0;
int scsiromselectedcatnum = 0;

void target_save_options(zfile* f, uae_prefs* p)
{
	//struct midiportinfo *midp;

	cfgfile_target_write_bool(f, _T("middle_mouse"), (p->input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON) != 0);
	cfgfile_target_dwrite_bool(f, _T("map_drives_auto"), p->automount_removable);
	cfgfile_target_dwrite_bool(f, _T("map_cd_drives"), p->automount_cddrives);

	cfgfile_target_dwrite(f, _T("active_priority"), _T("%d"), p->active_capture_priority);
	cfgfile_target_dwrite_bool(f, _T("active_not_captured_nosound"), p->active_nocapture_nosound);
	cfgfile_target_dwrite_bool(f, _T("active_not_captured_pause"), p->active_nocapture_pause);
	cfgfile_target_dwrite(f, _T("inactive_priority"), _T("%d"), p->inactive_priority);
	cfgfile_target_dwrite_bool(f, _T("inactive_nosound"), p->inactive_nosound);
	cfgfile_target_dwrite_bool(f, _T("inactive_pause"), p->inactive_pause);
	cfgfile_target_dwrite(f, _T("inactive_input"), _T("%d"), p->inactive_input);
	cfgfile_target_dwrite(f, _T("minimized_priority"), _T("%d"), p->minimized_priority);
	cfgfile_target_dwrite_bool(f, _T("minimized_nosound"), p->minimized_nosound);
	cfgfile_target_dwrite_bool(f, _T("minimized_pause"), p->minimized_pause);
	cfgfile_target_dwrite(f, _T("minimized_input"), _T("%d"), p->minimized_input);
	cfgfile_target_dwrite_bool(f, _T("inactive_minimize"), p->minimize_inactive);
	cfgfile_target_dwrite_bool(f, _T("active_capture_automatically"), p->capture_always);
	cfgfile_target_dwrite_bool(f, _T("start_minimized"), p->start_minimized);
	cfgfile_target_dwrite_bool(f, _T("start_not_captured"), p->start_uncaptured);

#ifdef AMIBERRY
	cfgfile_target_dwrite_str_escape(f, _T("midiout_device_name"), p->midioutdev[0] ? p->midioutdev : _T("none"));
	cfgfile_target_dwrite_str_escape(f, _T("midiin_device_name"), p->midiindev[0] ? p->midiindev : _T("none"));
#else
	cfgfile_target_dwrite (f, _T("midiout_device"), _T("%d"), p->midioutdev);
	cfgfile_target_dwrite (f, _T("midiin_device"), _T("%d"), p->midiindev);

	midp = getmidiport (midioutportinfo, p->midioutdev);
	if (p->midioutdev < -1)
		cfgfile_target_dwrite_str_escape(f, _T("midiout_device_name"), _T("none"));
	else if (p->midioutdev == -1 || midp == NULL)
		cfgfile_target_dwrite_str_escape(f, _T("midiout_device_name"), _T("default"));
	else
		cfgfile_target_dwrite_str_escape(f, _T("midiout_device_name"), midp->name);

	midp = getmidiport (midiinportinfo, p->midiindev);
	if (p->midiindev < 0 || midp == NULL)
		cfgfile_target_dwrite_str_escape(f, _T("midiin_device_name"), _T("none"));
	else
		cfgfile_target_dwrite_str_escape(f, _T("midiin_device_name"), midp->name);
#endif

	cfgfile_target_dwrite_bool (f, _T("midirouter"), p->midirouter);

	cfgfile_target_dwrite_bool(f, _T("rtg_scale_allow"), p->rtgallowscaling);
	cfgfile_target_dwrite(f, _T("rtg_scale_aspect_ratio"), _T("%d:%d"),
		p->rtgscaleaspectratio >= 0 ? (p->rtgscaleaspectratio / ASPECTMULT) : -1,
		p->rtgscaleaspectratio >= 0 ? (p->rtgscaleaspectratio & (ASPECTMULT - 1)) : -1);
	if (p->rtgvblankrate <= 0)
		cfgfile_target_dwrite_str(f, _T("rtg_vblank"), p->rtgvblankrate == -1 ? _T("real") : (p->rtgvblankrate == -2 ? _T("disabled") : _T("chipset")));
	else
		cfgfile_target_dwrite(f, _T("rtg_vblank"), _T("%d"), p->rtgvblankrate);
	cfgfile_target_dwrite_bool(f, _T("borderless"), p->borderless);
	cfgfile_target_dwrite_bool(f, _T("blank_monitors"), p->blankmonitors);
	cfgfile_target_dwrite_str(f, _T("uaescsimode"), scsimode[p->uaescsimode]);
	cfgfile_target_dwrite_bool(f, _T("soundcard_default"), p->soundcard_default);
	cfgfile_target_write(f, _T("soundcard"), _T("%d"), p->soundcard);
	if (p->soundcard >= 0 && p->soundcard < MAX_SOUND_DEVICES && sound_devices[p->soundcard])
		cfgfile_target_write_str(f, _T("soundcardname"), sound_devices[p->soundcard]->cfgname);
	if (p->samplersoundcard >= 0 && p->samplersoundcard < MAX_SOUND_DEVICES) {
		cfgfile_target_write(f, _T("samplersoundcard"), _T("%d"), p->samplersoundcard);
		if (record_devices[p->samplersoundcard])
			cfgfile_target_write_str(f, _T("samplersoundcardname"), record_devices[p->samplersoundcard]->cfgname);
	}

	cfgfile_target_dwrite(f, _T("cpu_idle"), _T("%d"), p->cpu_idle);
	cfgfile_target_dwrite_bool(f, _T("always_on_top"), p->main_alwaysontop);
	cfgfile_target_dwrite_bool(f, _T("gui_always_on_top"), p->gui_alwaysontop);
	cfgfile_target_dwrite_bool(f, _T("right_control_is_right_win"), p->right_control_is_right_win_key);

	cfgfile_target_dwrite(f, _T("gfx_horizontal_offset"), _T("%d"), p->gfx_horizontal_offset);
	cfgfile_target_dwrite(f, _T("gfx_vertical_offset"), _T("%d"), p->gfx_vertical_offset);
	if (p->gfx_auto_crop)
	{
		cfgfile_target_dwrite_bool(f, _T("gfx_auto_crop"), p->gfx_auto_crop);
	}
	else if (p->gfx_manual_crop)
	{
		cfgfile_target_dwrite_bool(f, _T("gfx_manual_crop"), p->gfx_manual_crop);
		cfgfile_target_dwrite(f, _T("gfx_manual_crop_width"), _T("%d"), p->gfx_manual_crop_width);
		cfgfile_target_dwrite(f, _T("gfx_manual_crop_height"), _T("%d"), p->gfx_manual_crop_height);
	}
	cfgfile_target_dwrite(f, _T("gfx_correct_aspect"), _T("%d"), p->gfx_correct_aspect);
	cfgfile_target_dwrite(f, _T("kbd_led_num"), _T("%d"), p->kbd_led_num);
	cfgfile_target_dwrite(f, _T("kbd_led_scr"), _T("%d"), p->kbd_led_scr);
	cfgfile_target_dwrite(f, _T("kbd_led_cap"), _T("%d"), p->kbd_led_cap);
	cfgfile_target_dwrite(f, _T("scaling_method"), _T("%d"), p->scaling_method);
	cfgfile_target_dwrite_str(f, _T("shader"), p->shader);
	cfgfile_target_dwrite_str(f, _T("shader_rtg"), p->shader_rtg);
	cfgfile_target_dwrite_bool(f, _T("use_bezel"), p->use_bezel);
	cfgfile_target_dwrite_bool(f, _T("use_custom_bezel"), p->use_custom_bezel);
	cfgfile_target_dwrite_str(f, _T("custom_bezel"), p->custom_bezel);

	cfgfile_target_dwrite_str(f, _T("open_gui"), p->open_gui);
	cfgfile_target_dwrite_str(f, _T("quit_amiberry"), p->quit_amiberry);
	cfgfile_target_dwrite_str(f, _T("action_replay"), p->action_replay);
	cfgfile_target_dwrite_str(f, _T("fullscreen_toggle"), p->fullscreen_toggle);
	cfgfile_target_dwrite_str(f, _T("minimize"), p->minimize);
	cfgfile_target_dwrite_str(f, _T("left_amiga"), p->left_amiga);
	cfgfile_target_dwrite_str(f, _T("right_amiga"), p->right_amiga);
	cfgfile_target_dwrite_str(f, _T("screenshot"), p->screenshot);
	cfgfile_target_dwrite_str(f, _T("debugger_trigger"), p->debugger_trigger);

	if (p->drawbridge_driver > 0)
	{
		cfgfile_target_dwrite(f, _T("drawbridge_driver"), _T("%d"), p->drawbridge_driver);
		cfgfile_target_dwrite_bool(f, _T("drawbridge_serial_autodetect"), p->drawbridge_serial_auto);
		cfgfile_target_write_str(f, _T("drawbridge_serial_port"), p->drawbridge_serial_port);
		cfgfile_target_dwrite_bool(f, _T("drawbridge_smartspeed"), p->drawbridge_smartspeed);
		cfgfile_target_dwrite_bool(f, _T("drawbridge_autocache"), p->drawbridge_autocache);
		cfgfile_target_dwrite(f, _T("drawbridge_drive_cable"), _T("%d"), p->drawbridge_drive_cable);
	}

	cfgfile_target_dwrite_bool(f, _T("alt_tab_release"), p->alt_tab_release);
	cfgfile_target_dwrite_bool(f, _T("ctrl_alt_release"), p->ctrl_alt_release);

	cfgfile_target_dwrite_bool(f, _T("use_retroarch_quit"), p->use_retroarch_quit);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_menu"), p->use_retroarch_menu);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_reset"), p->use_retroarch_reset);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_vkbd"), p->use_retroarch_vkbd);

	if (scsiromselected > 0)
		cfgfile_target_write(f, _T("expansion_gui_page"), expansionroms[scsiromselected].name);

	cfgfile_target_dwrite_bool(f, _T("onscreen_joystick"), p->onscreen_joystick);
	cfgfile_target_dwrite_bool(f, _T("vkbd_enabled"), p->vkbd_enabled);
	cfgfile_target_dwrite_bool(f, _T("vkbd_hires"), p->vkbd_hires);
	cfgfile_target_dwrite_bool(f, _T("vkbd_exit"), p->vkbd_exit);
	cfgfile_target_dwrite(f, _T("vkbd_transparency"), "%d", p->vkbd_transparency);
	cfgfile_target_dwrite_str(f, _T("vkbd_language"), p->vkbd_language);
	cfgfile_target_dwrite_str(f, _T("vkbd_style"), p->vkbd_style);
	cfgfile_target_dwrite_str(f, _T("vkbd_toggle"), p->vkbd_toggle);
}

void target_restart()
{
	emulating = 0;
	gui_restart();
}

TCHAR* target_expand_environment(const TCHAR* path, TCHAR* out, int maxlen)
{
	if (!path)
		return nullptr;
	if (out == nullptr)
		return strdup(path);

	_tcscpy(out, path);
	return out;
}

static const TCHAR *obsolete[] = {
	_T("killwinkeys"), _T("sound_force_primary"), _T("iconified_highpriority"),
	_T("sound_sync"), _T("sound_tweak"), _T("directx6"), _T("sound_style"),
	_T("file_path"), _T("iconified_nospeed"), _T("activepriority"), _T("magic_mouse"),
	_T("filesystem_codepage"), _T("aspi"), _T("no_overlay"), _T("soundcard_exclusive"),
	_T("specialkey"), _T("sound_speed_tweak"), _T("sound_lag"),
	nullptr
};

static int target_parse_option_hardware(uae_prefs *p, const TCHAR *option, const TCHAR *value)
{
	TCHAR tmpbuf[CONFIG_BLEN];
	if (cfgfile_string(option, value, _T("rtg_vblank"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		if (!_tcscmp(tmpbuf, _T("real"))) {
			p->rtgvblankrate = -1;
			return 1;
		}
		if (!_tcscmp(tmpbuf, _T("disabled"))) {
			p->rtgvblankrate = -2;
			return 1;
		}
		if (!_tcscmp(tmpbuf, _T("chipset"))) {
			p->rtgvblankrate = 0;
			return 1;
		}
		p->rtgvblankrate = _tstol(tmpbuf);
		return 1;
	}

	return 0;
}

static int target_parse_option_host(uae_prefs *p, const TCHAR *option, const TCHAR *value)
{
	TCHAR tmpbuf[CONFIG_BLEN];
	bool tbool;
	
	if (cfgfile_yesno(option, value, _T("middle_mouse"), &tbool)) {
		if (tbool)
			p->input_mouse_untrap |= MOUSEUNTRAP_MIDDLEBUTTON;
		else
			p->input_mouse_untrap &= ~MOUSEUNTRAP_MIDDLEBUTTON;
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("map_drives_auto"), &p->automount_removable)
		|| cfgfile_yesno(option, value, _T("map_cd_drives"), &p->automount_cddrives)
		|| cfgfile_yesno(option, value, _T("borderless"), &p->borderless)
		|| cfgfile_yesno(option, value, _T("blank_monitors"), &p->blankmonitors)
		|| cfgfile_yesno(option, value, _T("active_nocapture_pause"), &p->active_nocapture_pause)
		|| cfgfile_yesno(option, value, _T("active_nocapture_nosound"), &p->active_nocapture_nosound)
		|| cfgfile_yesno(option, value, _T("inactive_pause"), &p->inactive_pause)
		|| cfgfile_yesno(option, value, _T("inactive_nosound"), &p->inactive_nosound)
		|| cfgfile_intval(option, value, _T("inactive_input"), &p->inactive_input, 1)
		|| cfgfile_yesno(option, value, _T("minimized_pause"), &p->minimized_pause)
		|| cfgfile_yesno(option, value, _T("minimized_nosound"), &p->minimized_nosound)
		|| cfgfile_intval(option, value, _T("minimized_input"), &p->minimized_input, 1)
		|| cfgfile_string(option, value, _T("midi_device"), p->midioutdev, sizeof p->midioutdev)
		|| cfgfile_string(option, value, _T("midiout_device_name"), p->midioutdev, sizeof p->midioutdev)
		|| cfgfile_string(option, value, _T("midiin_device_name"), p->midiindev, sizeof p->midiindev)
		|| cfgfile_yesno(option, value, _T("midirouter"), &p->midirouter)
		|| cfgfile_yesno(option, value, _T("right_control_is_right_win"), &p->right_control_is_right_win_key)
		|| cfgfile_yesno(option, value, _T("always_on_top"), &p->main_alwaysontop)
		|| cfgfile_yesno(option, value, _T("gui_always_on_top"), &p->gui_alwaysontop)
		|| cfgfile_intval(option, value, _T("drawbridge_driver"), &p->drawbridge_driver, 1)
		|| cfgfile_yesno(option, value, _T("drawbridge_serial_autodetect"), &p->drawbridge_serial_auto)
		|| cfgfile_string(option, value, _T("drawbridge_serial_port"), p->drawbridge_serial_port, sizeof p->drawbridge_serial_port)
		|| cfgfile_yesno(option, value, _T("drawbridge_smartspeed"), &p->drawbridge_smartspeed)
		|| cfgfile_yesno(option, value, _T("drawbridge_autocache"), &p->drawbridge_autocache)
		|| cfgfile_intval(option, value, _T("drawbridge_drive_cable"), &p->drawbridge_drive_cable, 1)
		|| cfgfile_yesno(option, value, _T("alt_tab_release"), &p->alt_tab_release)
		|| cfgfile_yesno(option, value, _T("ctrl_alt_release"), &p->ctrl_alt_release)
		|| cfgfile_yesno(option, value, _T("use_retroarch_quit"), &p->use_retroarch_quit)
		|| cfgfile_yesno(option, value, _T("use_retroarch_menu"), &p->use_retroarch_menu)
		|| cfgfile_yesno(option, value, _T("use_retroarch_reset"), &p->use_retroarch_reset)
		|| cfgfile_yesno(option, value, _T("use_retroarch_vkbd"), &p->use_retroarch_vkbd)
		|| cfgfile_intval(option, value, _T("samplersoundcard"), &p->samplersoundcard, 1)
		|| cfgfile_intval(option, value, "kbd_led_num", &p->kbd_led_num, 1)
		|| cfgfile_intval(option, value, "kbd_led_scr", &p->kbd_led_scr, 1)
		|| cfgfile_intval(option, value, "kbd_led_cap", &p->kbd_led_cap, 1)
		|| cfgfile_intval(option, value, "gfx_horizontal_offset", &p->gfx_horizontal_offset, 1)
		|| cfgfile_intval(option, value, "gfx_vertical_offset", &p->gfx_vertical_offset, 1)
		|| cfgfile_intval(option, value, "gfx_manual_crop_width", &p->gfx_manual_crop_width, 1)
		|| cfgfile_intval(option, value, "gfx_manual_crop_height", &p->gfx_manual_crop_height, 1)
		|| cfgfile_yesno(option, value, _T("gfx_auto_height"), &p->gfx_auto_crop)
		|| cfgfile_yesno(option, value, _T("gfx_auto_crop"), &p->gfx_auto_crop)
		|| cfgfile_yesno(option, value, _T("gfx_manual_crop"), &p->gfx_manual_crop)
		|| cfgfile_intval(option, value, "gfx_correct_aspect", &p->gfx_correct_aspect, 1)
		|| cfgfile_intval(option, value, "scaling_method", &p->scaling_method, 1)
		|| cfgfile_string(option, value, "open_gui", p->open_gui, sizeof p->open_gui)
		|| cfgfile_string(option, value, "quit_amiberry", p->quit_amiberry, sizeof p->quit_amiberry)
		|| cfgfile_string(option, value, "action_replay", p->action_replay, sizeof p->action_replay)
		|| cfgfile_string(option, value, "fullscreen_toggle", p->fullscreen_toggle, sizeof p->fullscreen_toggle)
		|| cfgfile_string(option, value, "minimize", p->minimize, sizeof p->minimize)
		|| cfgfile_string(option, value, "left_amiga", p->left_amiga, sizeof p->left_amiga)
		|| cfgfile_string(option, value, "right_amiga", p->right_amiga, sizeof p->right_amiga)
		|| cfgfile_string(option, value, "screenshot", p->screenshot, sizeof p->screenshot)
		|| cfgfile_string(option, value, "debugger_trigger", p->debugger_trigger, sizeof p->debugger_trigger)
		|| cfgfile_intval(option, value, _T("cpu_idle"), &p->cpu_idle, 1))
		return 1;

	if (cfgfile_string(option, value, _T("shader"), p->shader, sizeof p->shader)) {
		if (!p->shader[0])
			_tcscpy(p->shader, _T("none"));
		return 1;
	}

	if (cfgfile_string(option, value, _T("shader_rtg"), p->shader_rtg, sizeof p->shader_rtg)) {
		if (!p->shader_rtg[0])
			_tcscpy(p->shader_rtg, _T("none"));
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("use_bezel"), &p->use_bezel)) {
		if (p->use_bezel)
			p->use_custom_bezel = false;
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("use_custom_bezel"), &p->use_custom_bezel)) {
		if (p->use_custom_bezel)
			p->use_bezel = false;
		return 1;
	}

	if (cfgfile_string(option, value, _T("custom_bezel"), p->custom_bezel, sizeof p->custom_bezel)) {
		if (!p->custom_bezel[0] || _tcsicmp(p->custom_bezel, _T("none")) == 0) {
			_tcscpy(p->custom_bezel, _T("none"));
			p->use_custom_bezel = false;
		}
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("onscreen_joystick"), &p->onscreen_joystick))
		return 1;

	if (cfgfile_yesno(option, value, _T("vkbd_enabled"), &p->vkbd_enabled)
		|| cfgfile_yesno(option, value, _T("vkbd_hires"), &p->vkbd_hires)
		|| cfgfile_yesno(option, value, _T("vkbd_exit"), &p->vkbd_exit)
		|| cfgfile_intval(option, value, _T("vkbd_transparency"), &p->vkbd_transparency, 1)
		|| cfgfile_string(option, value, _T("vkbd_language"), p->vkbd_language, sizeof p->vkbd_language)
		|| cfgfile_string(option, value, _T("vkbd_style"), p->vkbd_style, sizeof p->vkbd_style)
		|| cfgfile_string(option, value, _T("vkbd_toggle"), p->vkbd_toggle, sizeof p->vkbd_toggle))
		return 1;

	if (cfgfile_string(option, value, _T("expansion_gui_page"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		TCHAR* p = _tcschr(tmpbuf, ',');
		if (p != nullptr)
			*p = 0;
		for (int i = 0; expansionroms[i].name; i++) {
			if (!_tcsicmp(tmpbuf, expansionroms[i].name)) {
				scsiromselected = i;
				break;
			}
		}
		return 1;
	}
	
	if (cfgfile_yesno(option, value, _T("rtg_match_depth"), &tbool))
		return 1;
	if (cfgfile_yesno(option, value, _T("rtg_scale_allow"), &p->rtgallowscaling))
		return 1;

	if (cfgfile_yesno(option, value, _T("soundcard_default"), &p->soundcard_default))
		return 1;
	if (cfgfile_intval(option, value, _T("soundcard"), &p->soundcard, 1)) {
		if (p->soundcard < 0 || p->soundcard >= MAX_SOUND_DEVICES || sound_devices[p->soundcard] == nullptr)
			p->soundcard = 0;
		return 1;
	}

	if (cfgfile_string(option, value, _T("soundcardname"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		const int num = p->soundcard;
		p->soundcard = -1;
		for (int i = 0; i < MAX_SOUND_DEVICES && sound_devices[i]; i++) {
			if (i < num)
				continue;
			if (!_tcscmp(sound_devices[i]->cfgname, tmpbuf)) {
				p->soundcard = i;
				break;
			}
		}
		if (p->soundcard < 0) {
			for (int i = 0; i < MAX_SOUND_DEVICES && sound_devices[i]; i++) {
				if (!_tcscmp(sound_devices[i]->cfgname, tmpbuf)) {
					p->soundcard = i;
					break;
				}
			}
		}
		if (p->soundcard < 0) {
			for (int i = 0; i < MAX_SOUND_DEVICES && sound_devices[i]; i++) {
				if (!sound_devices[i]->prefix)
					continue;
				const int prefixlen = _tcslen(sound_devices[i]->prefix);
				const int tmplen = _tcslen(tmpbuf);
				if (prefixlen > 0 && tmplen >= prefixlen &&
					!_tcsncmp(sound_devices[i]->prefix, tmpbuf, prefixlen) &&
					((tmplen > prefixlen && tmpbuf[prefixlen] == ':')
						|| tmplen == prefixlen)) {
					p->soundcard = i;
					break;
				}
			}

		}
		if (p->soundcard < 0)
			p->soundcard = num;
		return 1;
	}
	if (cfgfile_string(option, value, _T("samplersoundcardname"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		const int num = p->samplersoundcard;
		p->samplersoundcard = -1;
		for (int i = 0; i < MAX_SOUND_DEVICES && record_devices[i]; i++) {
			if (i < num)
				continue;
			if (!_tcscmp(record_devices[i]->cfgname, tmpbuf)) {
				p->samplersoundcard = i;
				break;
			}
		}
		if (p->samplersoundcard < 0) {
			for (int i = 0; i < MAX_SOUND_DEVICES && record_devices[i]; i++) {
				if (!_tcscmp(record_devices[i]->cfgname, tmpbuf)) {
					p->samplersoundcard = i;
					break;
				}
			}
		}
		return 1;
	}

	if (cfgfile_string(option, value, _T("rtg_scale_aspect_ratio"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		p->rtgscaleaspectratio = -1;
		const int v1 = _tstol(tmpbuf);
		TCHAR* s = _tcschr(tmpbuf, ':');
		if (s) {
			const int v2 = _tstol(s + 1);
			if (v1 < 0 || v2 < 0)
				p->rtgscaleaspectratio = -1;
			else if (v1 == 0 || v2 == 0)
				p->rtgscaleaspectratio = 0;
			else
				p->rtgscaleaspectratio = v1 * ASPECTMULT + v2;
		}
		return 1;
	}

	if (cfgfile_strval(option, value, _T("uaescsimode"), &p->uaescsimode, scsimode, 0)) {
		// force SCSIEMU if pre 2.3 configuration
		if (p->config_version < ((2 << 16) | (3 << 8)))
			p->uaescsimode = UAESCSI_CDEMU;
		return 1;
	}

	if (cfgfile_intval(option, value, _T("active_priority"), &p->active_capture_priority, 1))
	{
		p->active_nocapture_pause = false;
		p->active_nocapture_nosound = false;
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("active_capture_automatically"), &p->capture_always))
		return 1;

	if (cfgfile_intval(option, value, _T("inactive_priority"), &p->inactive_priority, 1)) {
		return 1;
	}
	if (cfgfile_intval(option, value, _T("minimized_priority"), &p->minimized_priority, 1)) {
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("minimize_inactive"), &p->minimize_inactive))
		return 1;

	if (cfgfile_yesno(option, value, _T("start_minimized"), &p->start_minimized))
		return 1;

	if (cfgfile_yesno(option, value, _T("start_not_captured"), &p->start_uncaptured))
		return 1;
	if (cfgfile_string(option, value, _T("serial_port"), &p->sername[0], 256)) {
		if (p->sername[0])
			p->use_serial = true;
		else
			p->use_serial = false;
		return 1;
	}

	if (cfgfile_string_escape(option, value, _T("parallel_port"), &p->prtname[0], 256)) {
		if (!_tcscmp(p->prtname, _T("none")))
			p->prtname[0] = 0;
		if (!_tcscmp(p->prtname, _T("default"))) {
			p->prtname[0] = 0;
			//unsigned long size = 256;
			//GetDefaultPrinter(p->prtname, &size);
		}
		return 1;
	}

	return 0;
}

int target_parse_option(uae_prefs *p, const TCHAR *option, const TCHAR *value, const int type)
{
	int v = 0;
	if (type & CONFIG_TYPE_HARDWARE) {
		v = target_parse_option_hardware(p, option, value);
	} else if (type & CONFIG_TYPE_HOST) {
		v = target_parse_option_host(p, option, value);
	}
	if (v) {
		return v;
	}
	
	int i = 0;
	while (obsolete[i]) {
		if (!strcasecmp(obsolete[i], option)) {
			write_log(_T("obsolete config entry '%s'\n"), option);
			return 1;
		}
		i++;
	}
	return 0;
}

std::string get_data_path()
{
	return fix_trailing(data_dir);
}

void get_saveimage_path(char* out, const int size, int dir)
{
	_tcsncpy(out, fix_trailing(saveimage_dir).c_str(), size - 1);
}

std::string get_configuration_path()
{
	return fix_trailing(config_path);
}

std::string get_base_content_path()
{
	return base_content_path;
}

std::vector<std::string> get_base_content_missing_directories(const std::string& newpath)
{
	const auto preview_paths = preview_base_content_path_set(newpath);
	std::vector<std::string> missing_directories;
	for (const auto& directory : get_base_content_directories(preview_paths))
	{
		if (!my_existsdir(directory.c_str()))
			missing_directories.emplace_back(directory);
	}
	return missing_directories;
}

void get_configuration_path(char* out, const int size)
{
	_tcsncpy(out, fix_trailing(config_path).c_str(), size - 1);
}

void set_base_content_path(const std::string& newpath)
{
	const auto previous_logfile_path = logfile_path;

	if (newpath.empty())
	{
		clear_base_content_path_preserving_overrides();
	}
	else
	{
		apply_base_content_path_preserving_overrides(newpath);
		if (!suppress_runtime_path_side_effects)
			macos_bookmark_store(base_content_path);
	}

	if (!suppress_runtime_path_side_effects && !path_strings_match(previous_logfile_path, logfile_path))
		reopen_active_logfile_if_needed();
}

void create_missing_directories_for_base_content_path(const std::string& newpath)
{
	if (!newpath.empty())
		create_base_content_root_directory(newpath);
	create_missing_amiberry_folders();
}

void set_configuration_path(const std::string& newpath)
{
	config_path = newpath;
	macos_bookmark_store(newpath);
}

void set_nvram_path(const std::string& newpath)
{
	nvram_dir = newpath;
	macos_bookmark_store(newpath);
}

void set_plugins_path(const std::string& newpath)
{
	plugins_dir = newpath;
	macos_bookmark_store(newpath);
}

void set_video_path(const std::string& newpath)
{
	video_dir = newpath;
	macos_bookmark_store(newpath);
}

void set_themes_path(const std::string& newpath)
{
	themes_path = newpath;
	macos_bookmark_store(newpath);
}

void set_shaders_path(const std::string& newpath)
{
	shaders_path = newpath;
	macos_bookmark_store(newpath);
}

void set_bezels_path(const std::string& newpath)
{
	bezels_path = newpath;
	macos_bookmark_store(newpath);
}

void set_screenshot_path(const std::string& newpath)
{
	screenshot_dir = newpath;
	macos_bookmark_store(newpath);
}

void set_savestate_path(const std::string& newpath)
{
	savestate_dir = newpath;
	macos_bookmark_store(newpath);
}

std::string get_controllers_path()
{
	return fix_trailing(controllers_path);
}

void set_controllers_path(const std::string& newpath)
{
	controllers_path = newpath;
	macos_bookmark_store(newpath);
}

std::string get_retroarch_file()
{
	return retroarch_file;
}

void set_retroarch_file(const std::string& newpath)
{
	retroarch_file = newpath;
	macos_bookmark_store(newpath);
}

bool get_logfile_enabled()
{
	return amiberry_options.write_logfile;
}

void set_logfile_enabled(const bool enabled)
{
	amiberry_options.write_logfile = enabled;
}

// if force_internal == true, the non-overridden whdbootpath based save-data path will be returned
std::string get_savedatapath(const bool force_internal)
{
	std::string result;
	bool isOverridden = false;

	if (!force_internal && std::getenv("WHDBOOT_SAVE_DATA")) {
		result = std::getenv("WHDBOOT_SAVE_DATA");
		isOverridden = true;
	}
	else {
		result = get_whdbootpath();
		result.append("save-data");
	}
	write_log("%s savedatapath [%s]\n", isOverridden ? "external" : "internal", result.c_str());
	return result;
}

std::string get_whdbootpath()
{
	return fix_trailing(whdboot_path);
}

void set_whdbootpath(const std::string& newpath)
{
	whdboot_path = newpath;
	macos_bookmark_store(newpath);
}

std::string get_whdload_arch_path()
{
	return fix_trailing(whdload_arch_path);
}

void set_whdload_arch_path(const std::string& newpath)
{
	whdload_arch_path = newpath;
	macos_bookmark_store(newpath);
}

std::string get_floppy_path()
{
	return fix_trailing(floppy_path);
}

void set_floppy_path(const std::string& newpath)
{
	floppy_path = newpath;
	macos_bookmark_store(newpath);
}

std::string get_harddrive_path()
{
	return fix_trailing(harddrive_path);
}

void set_harddrive_path(const std::string& newpath)
{
	harddrive_path = newpath;
	macos_bookmark_store(newpath);
}

std::string get_cdrom_path()
{
	return fix_trailing(cdrom_path);
}

static std::string get_first_valid_multipath_entry(const multipath& paths)
{
	for (const auto& path : paths.path)
	{
		if (path[0] != '\0' && _tcscmp(path, _T("./")) != 0 && _tcscmp(path, _T(".\\")) != 0)
			return path;
	}

	return {};
}

std::string get_cdrom_browse_path()
{
	auto path = get_first_valid_multipath_entry(changed_prefs.path_cd);
	if (path.empty())
		path = get_first_valid_multipath_entry(currprefs.path_cd);
	if (!path.empty())
		return path;

	return get_cdrom_path();
}

static std::string resolve_cdrom_relative_path(const multipath& paths, const std::filesystem::path& relativePath)
{
	for (const auto& basePath : paths.path)
	{
		if (basePath[0] == '\0' || _tcscmp(basePath, _T("./")) == 0 || _tcscmp(basePath, _T(".\\")) == 0)
			continue;

		std::error_code ec;
		const auto candidate = std::filesystem::path(basePath) / relativePath;
		if (std::filesystem::exists(candidate, ec))
			return candidate.string();

		const auto parent = candidate.parent_path();
		ec.clear();
		if (!parent.empty() && std::filesystem::is_directory(parent, ec))
			return candidate.string();
	}

	return {};
}

std::string get_cdrom_browse_path(const std::string& currentPath)
{
	if (currentPath.empty())
		return get_cdrom_browse_path();

	const std::filesystem::path path(currentPath);
	if (path.is_absolute())
		return currentPath;

	auto resolved = resolve_cdrom_relative_path(changed_prefs.path_cd, path);
	if (resolved.empty())
		resolved = resolve_cdrom_relative_path(currprefs.path_cd, path);
	if (!resolved.empty())
		return resolved;

	return get_cdrom_browse_path();
}

void set_cdrom_path(const std::string& newpath)
{
	cdrom_path = newpath;
	macos_bookmark_store(newpath);
}

std::string get_logfile_path()
{
	return logfile_path;
}

void set_logfile_path(const std::string& newpath)
{
	logfile_path = newpath;
	macos_bookmark_store(newpath);
}

std::string get_rom_path()
{
	return fix_trailing(rom_path);
}

void get_rom_path(char* out, const int size)
{
	_tcsncpy(out, fix_trailing(rom_path).c_str(), size - 1);
}

void set_rom_path(const std::string& newpath)
{
	rom_path = newpath;
	macos_bookmark_store(newpath);
}

void get_rp9_path(char* out, const int size)
{
	_tcsncpy(out, fix_trailing(rp9_path).c_str(), size - 1);
}

void get_savestate_path(char* out, const int size)
{
	if (path_statefile[0]) {
		_tcsncpy(out, path_statefile, size);
		return;
	}
	_tcsncpy(out, fix_trailing(savestate_dir).c_str(), size - 1);
}

void fetch_ripperpath(TCHAR* out, const int size)
{
	_tcsncpy(out, fix_trailing(ripper_path).c_str(), size - 1);
}

void fetch_inputfilepath(TCHAR* out, const int size)
{
	_tcsncpy(out, fix_trailing(input_dir).c_str(), size - 1);
}

void get_nvram_path(TCHAR* out, const int size)
{
	_tcsncpy(out, fix_trailing(nvram_dir).c_str(), size - 1);
}

std::string get_plugins_path()
{
	return fix_trailing(plugins_dir);
}

std::string get_screenshot_path()
{
	return fix_trailing(screenshot_dir);
}

std::string get_ini_file_path()
{
	return amiberry_ini_file;
}

void get_video_path(char* out, const int size)
{
	_tcsncpy(out, fix_trailing(video_dir).c_str(), size - 1);
}

std::string get_themes_path()
{
	return fix_trailing(themes_path);
}

std::string get_shaders_path()
{
	return fix_trailing(shaders_path);
}

std::string get_bezels_path()
{
	return fix_trailing(bezels_path);
}

void get_floppy_sounds_path(char* out, const int size)
{
	_tcsncpy(out, fix_trailing(floppy_sounds_dir).c_str(), size - 1);
}

int target_cfgfile_load(uae_prefs* p, const char* filename, int type, const int isdefault)
{
	int type2;
	auto result = 0;

	if (isdefault) {
		path_statefile[0] = 0;
	}

	type = std::max(type, 0);

	if (type == 0 || type == 1) {
		discard_prefs(p, 0);
	}
	type2 = type;
	if (type == 0 || type == 3) {
		default_prefs(p, true, type);
		write_log(_T("config reset\n"));
	}

	const char* ptr = strstr(const_cast<char*>(filename), ".uae");
	if (ptr)
	{
		write_log(_T("target_cfgfile_load: loading file %s\n"), filename);
		result = cfgfile_load(p, filename, &type2, 0, isdefault ? 0 : 1);
	}
	if (!result)
	{
		write_log(_T("target_cfgfile_load: loading file %s failed\n"), filename);
		return result;
	}
	if (type > 0)
		return result;
	if (result)
		extract_filename(filename, last_loaded_config);

	for (auto i = 0; i < p->nr_floppies; ++i)
	{
		if (!DISK_validate_filename(p, p->floppyslots[i].df, i, nullptr, 0, nullptr, nullptr, nullptr))
			p->floppyslots[i].df[0] = 0;
		disk_insert(i, p->floppyslots[i].df);
		if (strlen(p->floppyslots[i].df) > 0)
			add_file_to_mru_list(lstMRUDiskList, std::string(p->floppyslots[i].df));
	}
	set_last_loaded_config(filename);
	set_last_active_config(filename);
	return result;
}

int check_configfile(const char* file)
{
	char tmp[MAX_DPATH];

	auto* f = uae_fopen(file, "rte");
	if (f)
	{
		fclose(f);
		return 1;
	}

	strncpy(tmp, file, MAX_DPATH - 1);
	auto* const ptr = strstr(tmp, ".uae");
	if (ptr)
	{
		*(ptr + 1) = '\0';
		strncat(tmp, "conf", MAX_DPATH - 1);
		f = uae_fopen(tmp, "rte");
		if (f)
		{
			fclose(f);
			return 2;
		}
	}
	return 0;
}

void extract_filename(const char* str, char* buffer)
{
	const auto* p = str + strlen(str) - 1;
	while (*p != '/' && p >= str)
		p--;
	p++;
	strncpy(buffer, p, MAX_DPATH - 1);
}

std::string extract_filename(const std::string& path)
{
	const std::filesystem::path file_path(path);
	return file_path.filename().string();
}

void extract_path(const char* str, char* buffer)
{
	strncpy(buffer, str, MAX_DPATH - 1);
	auto* p = buffer + strlen(buffer) - 1;
	while (*p != '/' && p >= buffer)
		p--;
	p[1] = '\0';
}

std::string extract_path(const std::string& filename)
{
	const std::filesystem::path file_path(filename);
	return file_path.parent_path().string();
}

void remove_file_extension(char* filename)
{
	auto* p = filename + strlen(filename) - 1;
	while (p >= filename && *p != '.')
	{
		*p = '\0';
		--p;
	}
	*p = '\0';
}

std::string remove_file_extension(const std::string& filename)
{
	const size_t last_dot = filename.find_last_of('.');
	if (last_dot == std::string::npos) {
		// No extension found, return the original filename
		return filename;
	}
	return filename.substr(0, last_dot);
}

void read_directory(const std::string& path, std::vector<std::string>* dirs, std::vector<std::string>* files)
{
	if (dirs != nullptr)
		dirs->clear();
	if (files != nullptr)
		files->clear();

	// check if the path exists first
	if (!std::filesystem::exists(path))
		return;

	try {
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				if (dirs != nullptr && (entry.path().filename().string()[0] != '.' || (entry.path().filename().string()[0] == '.' && entry.path().filename().string()[1] == '.')))
					dirs->emplace_back(entry.path().filename().string());
			}
			else if (entry.is_symlink())
			{
				if (std::filesystem::is_directory(entry))
				{
					if (dirs != nullptr && (entry.path().filename().string()[0] != '.' || (entry.path().filename().string()[0] == '.' && entry.path().filename().string()[1] == '.')))
						dirs->emplace_back(entry.path().filename().string());
				}
				else
				{
					if (files != nullptr && entry.path().filename().string()[0] != '.')
						files->emplace_back(entry.path().filename().string());
				}
			}
			else if (files != nullptr && entry.path().filename().string()[0] != '.')
				files->emplace_back(entry.path().filename().string());
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		write_log("read_directory error: %s\n", e.what());
	}

	if (dirs != nullptr && !dirs->empty() && (*dirs)[0] == ".")
		dirs->erase(dirs->begin());

	if (dirs != nullptr)
		sort(dirs->begin(), dirs->end());
	if (files != nullptr)
		sort(files->begin(), files->end());
}

void save_amiberry_settings()
{
	ensure_parent_directory_exists(amiberry_conf_file);
	auto* const f = uae_fopen(amiberry_conf_file.c_str(), "we");
	if (!f)
		return;

	char buffer[MAX_DPATH];

	auto write_bool_option = [&](const char* name, const bool value) {
		_sntprintf(buffer, MAX_DPATH, "%s=%s\n", name, value ? "yes" : "no");
		fputs(buffer, f);
		};

	auto write_int_option = [&](const char* name, const int value) {
		_sntprintf(buffer, MAX_DPATH, "%s=%d\n", name, value);
		fputs(buffer, f);
		};

	auto write_string_option = [&](const char* name, const std::string& value) {
		_sntprintf(buffer, MAX_DPATH, "%s=%s\n", name, value.c_str());
		fputs(buffer, f);
		};

	auto write_float_option = [&](const char* name, const float value) {
		_sntprintf(buffer, MAX_DPATH, "%s=%f\n", name, value);
		fputs(buffer, f);
		};

	// Should the Quickstart Panel be the default when opening the GUI?
	write_int_option("Quickstart", amiberry_options.quickstart_start);

	// Open each config file and read the Description field? 
	// This will slow down scanning the config list if it's very large
	write_bool_option("read_config_descriptions", amiberry_options.read_config_descriptions);

	// Write to logfile? 
	// If enabled, a file named "amiberry_log.txt" will be generated in the startup folder
	write_bool_option("write_logfile", amiberry_options.write_logfile);

	// Scanlines ON by default?
	// This will only be enabled if the vertical height is enough, as we need Line Doubling set to ON also
	// Beware this comes with a performance hit, as double the amount of lines need to be drawn on-screen
	write_int_option("default_line_mode", amiberry_options.default_line_mode);

	// Map RCtrl key to RAmiga key?
	// This helps with keyboards that may not have 2 Win keys and no Menu key either
	write_bool_option("rctrl_as_ramiga", amiberry_options.rctrl_as_ramiga);

	// Disable controller in the GUI?
	// If you want to disable the default behavior for some reason
	write_bool_option("gui_joystick_control", amiberry_options.gui_joystick_control);

	// Default mouse input speed
	write_int_option("input_default_mouse_speed", amiberry_options.input_default_mouse_speed);

	// When using Keyboard as Joystick, stop any double keypresses
	write_bool_option("input_keyboard_as_joystick_stop_keypresses", amiberry_options.input_keyboard_as_joystick_stop_keypresses);
	
	// Default key for opening the GUI (e.g. "F12")
	write_string_option("default_open_gui_key", amiberry_options.default_open_gui_key);

	// Default key for Quitting the emulator
	write_string_option("default_quit_key", amiberry_options.default_quit_key);

	// Default key for opening Action Replay
	write_string_option("default_ar_key", amiberry_options.default_ar_key);

	// Default key for Fullscreen Toggle
	write_string_option("default_fullscreen_toggle_key", amiberry_options.default_fullscreen_toggle_key);

	// Enable Horizontal Centering by default?
	write_bool_option("default_horizontal_centering", amiberry_options.default_horizontal_centering);

	// Enable Vertical Centering by default?
	write_bool_option("default_vertical_centering", amiberry_options.default_vertical_centering);

	// Scaling method to use by default?
	// Valid options are: -1 Auto, 0 Nearest Neighbor, 1 Linear
	write_int_option("default_scaling_method", amiberry_options.default_scaling_method);

	// Enable Auto Resolution Switching by default?
	write_int_option("default_gfx_autoresolution", amiberry_options.default_gfx_autoresolution);

	// Enable frameskip by default?
	write_bool_option("default_frameskip", amiberry_options.default_frameskip);

	// Correct Aspect Ratio by default?
	write_bool_option("default_correct_aspect_ratio", amiberry_options.default_correct_aspect_ratio);

	// Enable Auto-Crop by default?
	write_bool_option("default_auto_crop", amiberry_options.default_auto_crop);

	// Default Screen Width
	write_int_option("default_width", amiberry_options.default_width);
	
	// Default Screen Height
	write_int_option("default_height", amiberry_options.default_height);

	// Full screen mode (0, 1, 2)
	write_int_option("default_fullscreen_mode", amiberry_options.default_fullscreen_mode);
	
	// Default Stereo Separation
	write_int_option("default_stereo_separation", amiberry_options.default_stereo_separation);

	// Default Sound Frequency
	write_int_option("default_sound_frequency", amiberry_options.default_sound_frequency);

	// Default Sound buffer size
	write_int_option("default_sound_buffer", amiberry_options.default_sound_buffer);

	// Default Joystick Deadzone
	write_int_option("default_joystick_deadzone", amiberry_options.default_joystick_deadzone);

	// Enable RetroArch Quit by default?
	write_bool_option("default_retroarch_quit", amiberry_options.default_retroarch_quit);

	// Enable RetroArch Menu by default?
	write_bool_option("default_retroarch_menu", amiberry_options.default_retroarch_menu);

	// Enable RetroArch Reset by default?
	write_bool_option("default_retroarch_reset", amiberry_options.default_retroarch_reset);

	// Enable RetroArch VKBD by default?
	write_bool_option("default_retroarch_vkbd", amiberry_options.default_retroarch_vkbd);

	// Controller1
	write_string_option("default_controller1", amiberry_options.default_controller1);

	// Controller2
	write_string_option("default_controller2", amiberry_options.default_controller2);

	// Controller3
	write_string_option("default_controller3", amiberry_options.default_controller3);

	// Controller4
	write_string_option("default_controller4", amiberry_options.default_controller4);

	// Mouse1
	write_string_option("default_mouse1", amiberry_options.default_mouse1);

	// Mouse2
	write_string_option("default_mouse2", amiberry_options.default_mouse2);

	// WHDLoad ButtonWait
	write_bool_option("default_whd_buttonwait", amiberry_options.default_whd_buttonwait);

	// WHDLoad Show Splash screen
	write_bool_option("default_whd_showsplash", amiberry_options.default_whd_showsplash);

	// WHDLoad Config Delay
	write_int_option("default_whd_configdelay", amiberry_options.default_whd_configdelay);

	// WHDLoad WriteCache
	write_bool_option("default_whd_writecache", amiberry_options.default_whd_writecache);

	// WHDLoad Quit emulator after game exits
	write_bool_option("default_whd_quit_on_exit", amiberry_options.default_whd_quit_on_exit);

	// Use JST instead of WHDLoad
	write_bool_option("use_jst_instead_of_whd", amiberry_options.use_jst_instead_of_whd);

	// Disable Shutdown button in GUI
	write_bool_option("disable_shutdown_button", amiberry_options.disable_shutdown_button);

	// Allow Display settings to be used from the WHDLoad XML (override amiberry.conf defaults)
	write_bool_option("allow_display_settings_from_xml", amiberry_options.allow_display_settings_from_xml);

	// Default Sound Card (0=default, first one available in the system)
	write_int_option("default_soundcard", amiberry_options.default_soundcard);

	// Enable On-screen Joystick by default (for touchscreen devices)
	write_bool_option("default_onscreen_joystick", amiberry_options.default_onscreen_joystick);

	// Enable Virtual Keyboard by default
	write_bool_option("default_vkbd_enabled", amiberry_options.default_vkbd_enabled);

	// Show the High-res version of the Virtual Keyboard by default
	write_bool_option("default_vkbd_hires", amiberry_options.default_vkbd_hires);

	// Enable Quit functionality through Virtual Keyboard by default
	write_bool_option("default_vkbd_exit", amiberry_options.default_vkbd_exit);

	// Default Language for the Virtual Keyboard
	write_string_option("default_vkbd_language", amiberry_options.default_vkbd_language);

	// Default Style for the Virtual Keyboard
	write_string_option("default_vkbd_style", amiberry_options.default_vkbd_style);

	// Default transparency for the Virtual Keyboard
	write_int_option("default_vkbd_transparency", amiberry_options.default_vkbd_transparency);

	// Default controller button for toggling the Virtual Keyboard
	write_string_option("default_vkbd_toggle", amiberry_options.default_vkbd_toggle);

	// GUI Theme
	write_string_option("gui_theme", amiberry_options.gui_theme);

	// Shader to use for Native modes (if any)
	write_string_option("shader", amiberry_options.shader);

	// Shader to use for RTG modes (if any)
	write_string_option("shader_rtg", amiberry_options.shader_rtg);

	// Show CRT bezel frame overlay
	write_bool_option("use_bezel", amiberry_options.use_bezel);
	write_bool_option("use_custom_bezel", amiberry_options.use_custom_bezel);
	write_string_option("custom_bezel", amiberry_options.custom_bezel);

	// Auto-update settings
	write_int_option("update_check", amiberry_options.update_check);
	write_int_option("update_channel", amiberry_options.update_channel);
	write_string_option("skipped_version", amiberry_options.skipped_version);
	write_string_option("last_update_check", amiberry_options.last_update_check);

	// Paths
	const auto derived_base_paths = base_content_path.empty()
		? base_content_path_set{}
		: get_base_content_path_set(base_content_path);
	auto write_managed_path_option = [&](const managed_path_option_descriptor& descriptor, const std::string& value) {
		if (base_content_path.empty() || !path_strings_match(value, derived_base_paths.*(descriptor.member)))
			write_string_option(descriptor.name, value);
	};

	write_string_option("base_content_path", base_content_path);
	write_string_option("retroarch_config", retroarch_file);
	write_string_option("floppy_sounds_dir", floppy_sounds_dir);
	write_string_option("plugins_dir", plugins_dir);
	write_managed_path_option(base_content_managed_path_options[0], config_path);
	write_managed_path_option(base_content_managed_path_options[1], controllers_path);
	write_managed_path_option(base_content_managed_path_options[2], whdboot_path);
	write_managed_path_option(base_content_managed_path_options[3], whdload_arch_path);
	write_managed_path_option(base_content_managed_path_options[4], floppy_path);
	write_managed_path_option(base_content_managed_path_options[5], harddrive_path);
	write_managed_path_option(base_content_managed_path_options[6], cdrom_path);
	write_managed_path_option(base_content_managed_path_options[7], logfile_path);
	write_managed_path_option(base_content_managed_path_options[8], rom_path);
	write_managed_path_option(base_content_managed_path_options[9], rp9_path);
	write_managed_path_option(base_content_managed_path_options[10], saveimage_dir);
	write_managed_path_option(base_content_managed_path_options[11], savestate_dir);
	write_managed_path_option(base_content_managed_path_options[12], ripper_path);
	write_managed_path_option(base_content_managed_path_options[13], input_dir);
	write_managed_path_option(base_content_managed_path_options[14], screenshot_dir);
	write_managed_path_option(base_content_managed_path_options[15], nvram_dir);
	write_managed_path_option(base_content_managed_path_options[16], video_dir);
	write_managed_path_option(base_content_managed_path_options[17], themes_path);
	write_managed_path_option(base_content_managed_path_options[18], shaders_path);
	write_managed_path_option(base_content_managed_path_options[19], bezels_path);

	// Recent disk entries (these are used in the dropdown controls)
	_sntprintf(buffer, MAX_DPATH, "MRUDiskList=%zu\n", lstMRUDiskList.size());
	fputs(buffer, f);
	for (auto& i : lstMRUDiskList)
	{
		write_string_option("Diskfile", i);
	}

	// Recent CD entries (these are used in the dropdown controls)
	_sntprintf(buffer, MAX_DPATH, "MRUCDList=%zu\n", lstMRUCDList.size());
	fputs(buffer, f);
	for (auto& i : lstMRUCDList)
	{
		write_string_option("CDfile", i);
	}

	// Recent WHDLoad entries (these are used in the dropdown controls)
	// lstMRUWhdloadList
	_sntprintf(buffer, MAX_DPATH, "MRUWHDLoadList=%zu\n", lstMRUWhdloadList.size());
	fputs(buffer, f);
	for (auto& i : lstMRUWhdloadList)
	{
		write_string_option("WHDLoadfile", i);
	}
	
	fclose(f);
}

void get_string(FILE* f, char* dst, const int size)
{
	char buffer[MAX_DPATH];
	fgets(buffer, MAX_DPATH, f);
	auto i = strlen(buffer);
	while (i > 0 && (buffer[i - 1] == '\t' || buffer[i - 1] == ' '
		|| buffer[i - 1] == '\r' || buffer[i - 1] == '\n'))
		buffer[--i] = '\0';
	strncpy(dst, buffer, size);
}

static void trim_wsa(char* s)
{
	/* Delete trailing whitespace.  */
	auto len = strlen(s);
	while (len > 0 && strcspn(s + len - 1, "\t \r\n") == 0)
		s[--len] = '\0';
}

static bool parse_base_content_path_line(const char* path, char* linea, std::string& value_out)
{
	char line_copy[CONFIG_BLEN];
	TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
	char configured_base_path[MAX_DPATH];

	strncpy(line_copy, linea, CONFIG_BLEN - 1);
	line_copy[CONFIG_BLEN - 1] = '\0';

	if (!cfgfile_separate_linea(path, line_copy, option, value))
		return false;

	configured_base_path[0] = '\0';
	if (!cfgfile_string(option, value, "base_content_path", configured_base_path, sizeof configured_base_path))
		return false;

	value_out = normalize_path_string(configured_base_path);
	return true;
}

static bool parse_managed_path_option_line(const char* path, char* linea, managed_path_option_line& line_out)
{
	char line_copy[CONFIG_BLEN];
	TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
	char configured_path[MAX_DPATH];

	strncpy(line_copy, linea, CONFIG_BLEN - 1);
	line_copy[CONFIG_BLEN - 1] = '\0';

	if (!cfgfile_separate_linea(path, line_copy, option, value))
		return false;

	const auto* descriptor = find_base_content_managed_path_option(option);
	if (descriptor == nullptr)
		return false;

	configured_path[0] = '\0';
	if (!cfgfile_string(option, value, descriptor->name, configured_path, sizeof configured_path))
		return false;

	line_out.descriptor = descriptor;
	line_out.value = normalize_path_string(configured_path);
	return true;
}

static int parse_amiberry_settings_line(const char *path, char *linea)
{
	TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
	int numROMs, numDisks, numCDs;
	char tmpFile[MAX_DPATH];
	TCHAR legacy_cd_path[MAX_DPATH];
	TCHAR legacy_cd_path_option[CONFIG_BLEN];
	int ret = 0;
	std::string configured_base_path;

	if (parse_base_content_path_line(path, linea, configured_base_path))
	{
		set_base_content_path(configured_base_path);
		return 1;
	}

	if (!cfgfile_separate_linea(path, linea, option, value))
		return 0;

	if (cfgfile_string(option, value, "Diskfile", tmpFile, sizeof tmpFile))
	{
		auto* const f = uae_fopen(tmpFile, "rbe");
		if (f != nullptr)
		{
			fclose(f);
			lstMRUDiskList.emplace_back(tmpFile);
			ret = 1;
		}
	}
	else if (cfgfile_string(option, value, "CDfile", tmpFile, sizeof tmpFile))
	{
		auto* const f = uae_fopen(tmpFile, "rbe");
		if (f != nullptr)
		{
			fclose(f);
			lstMRUCDList.emplace_back(tmpFile);
			ret = 1;
		}
	}
	else if (cfgfile_string(option, value, "WHDLoadfile", tmpFile, sizeof tmpFile))
	{
		auto* const f = uae_fopen(tmpFile, "rbe");
		if (f != nullptr)
		{
			fclose(f);
			lstMRUWhdloadList.emplace_back(tmpFile);
			ret = 1;
		}
	}
	else
	{
		_sntprintf(legacy_cd_path_option, sizeof legacy_cd_path_option / sizeof(TCHAR), _T("%s.cd_path"), TARGET_NAME);
		ret |= cfgfile_string(option, value, "config_path", config_path);
		ret |= cfgfile_string(option, value, "controllers_path", controllers_path);
		ret |= cfgfile_string(option, value, "retroarch_config", retroarch_file);
		ret |= cfgfile_string(option, value, "whdboot_path", whdboot_path);
		ret |= cfgfile_string(option, value, "whdload_arch_path", whdload_arch_path);
		ret |= cfgfile_string(option, value, "floppy_path", floppy_path);
		ret |= cfgfile_string(option, value, "harddrive_path", harddrive_path);
		ret |= cfgfile_string(option, value, "cdrom_path", cdrom_path);
		if (cfgfile_string(option, value, _T("cd_path"), legacy_cd_path, sizeof legacy_cd_path)
			|| cfgfile_string(option, value, legacy_cd_path_option, legacy_cd_path, sizeof legacy_cd_path))
		{
			cdrom_path = legacy_cd_path;
			ret = 1;
		}
		ret |= cfgfile_string(option, value, "logfile_path", logfile_path);
		ret |= cfgfile_string(option, value, "rom_path", rom_path);
		ret |= cfgfile_string(option, value, "rp9_path", rp9_path);
		ret |= cfgfile_string(option, value, "floppy_sounds_dir", floppy_sounds_dir);
		ret |= cfgfile_string(option, value, "saveimage_dir", saveimage_dir);
		ret |= cfgfile_string(option, value, "savestate_dir", savestate_dir);
		ret |= cfgfile_string(option, value, "ripper_path", ripper_path);
		ret |= cfgfile_string(option, value, "inputrecordings_dir", input_dir);
		ret |= cfgfile_string(option, value, "screenshot_dir", screenshot_dir);
		ret |= cfgfile_string(option, value, "nvram_dir", nvram_dir);
		ret |= cfgfile_string(option, value, "plugins_dir", plugins_dir);
		ret |= cfgfile_string(option, value, "video_dir", video_dir);
		ret |= cfgfile_string(option, value, "themes_path", themes_path);
		ret |= cfgfile_string(option, value, "shaders_path", shaders_path);
		ret |= cfgfile_string(option, value, "bezels_path", bezels_path);
		// NOTE: amiberry_config is a "read only", i.e. it's not written in
		// save_amiberry_settings(). It's purpose is to provide -o amiberry_config=path
		// command line option.
		ret |= cfgfile_string(option, value, "amiberry_config", amiberry_conf_file);
		ret |= cfgfile_intval(option, value, "ROMs", &numROMs, 1);
		ret |= cfgfile_intval(option, value, "MRUDiskList", &numDisks, 1);
		ret |= cfgfile_intval(option, value, "MRUCDList", &numCDs, 1);
		ret |= cfgfile_yesno(option, value, "Quickstart", &amiberry_options.quickstart_start);
		ret |= cfgfile_yesno(option, value, "read_config_descriptions", &amiberry_options.read_config_descriptions);
		ret |= cfgfile_yesno(option, value, "write_logfile", &amiberry_options.write_logfile);
		ret |= cfgfile_intval(option, value, "default_line_mode", &amiberry_options.default_line_mode, 1);
		ret |= cfgfile_yesno(option, value, "rctrl_as_ramiga", &amiberry_options.rctrl_as_ramiga);
		ret |= cfgfile_yesno(option, value, "gui_joystick_control", &amiberry_options.gui_joystick_control);
		ret |= cfgfile_intval(option, value, "input_default_mouse_speed", &amiberry_options.input_default_mouse_speed, 1);
		ret |= cfgfile_yesno(option, value, "input_keyboard_as_joystick_stop_keypresses", &amiberry_options.input_keyboard_as_joystick_stop_keypresses);
		ret |= cfgfile_string(option, value, "default_open_gui_key", amiberry_options.default_open_gui_key, sizeof amiberry_options.default_open_gui_key);
		ret |= cfgfile_string(option, value, "default_quit_key", amiberry_options.default_quit_key, sizeof amiberry_options.default_quit_key);
		ret |= cfgfile_string(option, value, "default_ar_key", amiberry_options.default_ar_key, sizeof amiberry_options.default_ar_key);
		ret |= cfgfile_string(option, value, "default_fullscreen_toggle_key", amiberry_options.default_fullscreen_toggle_key, sizeof amiberry_options.default_fullscreen_toggle_key);
		ret |= cfgfile_yesno(option, value, "default_horizontal_centering", &amiberry_options.default_horizontal_centering);
		ret |= cfgfile_yesno(option, value, "default_vertical_centering", &amiberry_options.default_vertical_centering);
		ret |= cfgfile_intval(option, value, "default_scaling_method", &amiberry_options.default_scaling_method, 1);
		ret |= cfgfile_intval(option, value, "default_gfx_autoresolution", &amiberry_options.default_gfx_autoresolution, 1);
		ret |= cfgfile_yesno(option, value, "default_frameskip", &amiberry_options.default_frameskip);
		ret |= cfgfile_yesno(option, value, "default_correct_aspect_ratio", &amiberry_options.default_correct_aspect_ratio);
		ret |= cfgfile_yesno(option, value, "default_auto_height", &amiberry_options.default_auto_crop);
		ret |= cfgfile_yesno(option, value, "default_auto_crop", &amiberry_options.default_auto_crop);
		ret |= cfgfile_intval(option, value, "default_width", &amiberry_options.default_width, 1);
		ret |= cfgfile_intval(option, value, "default_height", &amiberry_options.default_height, 1);
		ret |= cfgfile_intval(option, value, "default_fullscreen_mode", &amiberry_options.default_fullscreen_mode, 1);
		ret |= cfgfile_intval(option, value, "default_stereo_separation", &amiberry_options.default_stereo_separation, 1);
		ret |= cfgfile_intval(option, value, "default_sound_buffer", &amiberry_options.default_sound_buffer, 1);
		ret |= cfgfile_intval(option, value, "default_sound_frequency", &amiberry_options.default_sound_frequency, 1);
		ret |= cfgfile_intval(option, value, "default_joystick_deadzone", &amiberry_options.default_joystick_deadzone, 1);
		ret |= cfgfile_yesno(option, value, "default_retroarch_quit", &amiberry_options.default_retroarch_quit);
		ret |= cfgfile_yesno(option, value, "default_retroarch_menu", &amiberry_options.default_retroarch_menu);
		ret |= cfgfile_yesno(option, value, "default_retroarch_reset", &amiberry_options.default_retroarch_reset);
		ret |= cfgfile_yesno(option, value, "default_retroarch_vkbd", &amiberry_options.default_retroarch_vkbd);
		ret |= cfgfile_string(option, value, "default_controller1", amiberry_options.default_controller1, sizeof amiberry_options.default_controller1);
		ret |= cfgfile_string(option, value, "default_controller2", amiberry_options.default_controller2, sizeof amiberry_options.default_controller2);
		ret |= cfgfile_string(option, value, "default_controller3", amiberry_options.default_controller3, sizeof amiberry_options.default_controller3);
		ret |= cfgfile_string(option, value, "default_controller4", amiberry_options.default_controller4, sizeof amiberry_options.default_controller4);
		ret |= cfgfile_string(option, value, "default_mouse1", amiberry_options.default_mouse1, sizeof amiberry_options.default_mouse1);
		ret |= cfgfile_string(option, value, "default_mouse2", amiberry_options.default_mouse2, sizeof amiberry_options.default_mouse2);
		ret |= cfgfile_yesno(option, value, "default_whd_buttonwait", &amiberry_options.default_whd_buttonwait);
		ret |= cfgfile_yesno(option, value, "default_whd_showsplash", &amiberry_options.default_whd_showsplash);
		ret |= cfgfile_intval(option, value, "default_whd_configdelay", &amiberry_options.default_whd_configdelay, 1);
		ret |= cfgfile_yesno(option, value, "default_whd_writecache", &amiberry_options.default_whd_writecache);
		ret |= cfgfile_yesno(option, value, "default_whd_quit_on_exit", &amiberry_options.default_whd_quit_on_exit);
		ret |= cfgfile_yesno(option, value, "use_jst_instead_of_whd", &amiberry_options.use_jst_instead_of_whd);
		ret |= cfgfile_yesno(option, value, "disable_shutdown_button", &amiberry_options.disable_shutdown_button);
		ret |= cfgfile_yesno(option, value, "allow_display_settings_from_xml", &amiberry_options.allow_display_settings_from_xml);
		ret |= cfgfile_intval(option, value, "default_soundcard", &amiberry_options.default_soundcard, 1);
		ret |= cfgfile_yesno(option, value, "default_onscreen_joystick", &amiberry_options.default_onscreen_joystick);
		ret |= cfgfile_yesno(option, value, "default_vkbd_enabled", &amiberry_options.default_vkbd_enabled);
		ret |= cfgfile_yesno(option, value, "default_vkbd_hires", &amiberry_options.default_vkbd_hires);
		ret |= cfgfile_yesno(option, value, "default_vkbd_exit", &amiberry_options.default_vkbd_exit);
		ret |= cfgfile_string(option, value, "default_vkbd_language", amiberry_options.default_vkbd_language, sizeof amiberry_options.default_vkbd_language);
		ret |= cfgfile_string(option, value, "default_vkbd_style", amiberry_options.default_vkbd_style, sizeof amiberry_options.default_vkbd_style);
		ret |= cfgfile_intval(option, value, "default_vkbd_transparency", &amiberry_options.default_vkbd_transparency, 1);
		ret |= cfgfile_string(option, value, "default_vkbd_toggle", amiberry_options.default_vkbd_toggle, sizeof amiberry_options.default_vkbd_toggle);
		ret |= cfgfile_string(option, value, "gui_theme", amiberry_options.gui_theme, sizeof amiberry_options.gui_theme);
		ret |= cfgfile_string(option, value, "shader", amiberry_options.shader, sizeof amiberry_options.shader);
		ret |= cfgfile_string(option, value, "shader_rtg", amiberry_options.shader_rtg, sizeof amiberry_options.shader_rtg);
		ret |= cfgfile_yesno(option, value, "use_bezel", &amiberry_options.use_bezel);
		ret |= cfgfile_yesno(option, value, "use_custom_bezel", &amiberry_options.use_custom_bezel);
		ret |= cfgfile_string(option, value, "custom_bezel", amiberry_options.custom_bezel, sizeof amiberry_options.custom_bezel);
		ret |= cfgfile_intval(option, value, "update_check", &amiberry_options.update_check, 1);
		ret |= cfgfile_intval(option, value, "update_channel", &amiberry_options.update_channel, 1);
		ret |= cfgfile_string(option, value, "skipped_version", amiberry_options.skipped_version, sizeof amiberry_options.skipped_version);
		ret |= cfgfile_string(option, value, "last_update_check", amiberry_options.last_update_check, sizeof amiberry_options.last_update_check);
	}
	return ret;
}

static int parse_amiberry_cmd_line(int *argc, char* argv[], const bool remove_used_args)
{
	char arg_copy[CONFIG_BLEN];

	for (int i = 0; i < *argc; i++)
	{
		if (strncmp(argv[i], "-o", 3) == 0)
		{
			if (i >= *argc - 1)
			{
				// fail because option arg is missing
				return 0;
			}
			// Keep a copy to restore after parsing because settings parsing is destructive.
			strncpy(arg_copy, argv[i + 1], CONFIG_BLEN);
			arg_copy[CONFIG_BLEN - 1] = '\0';
			const bool is_amiberry_config_override = strncmp(arg_copy, "amiberry_config=", 16) == 0;
			if (!parse_amiberry_settings_line("<command line>", argv[i + 1]))
			{
				// fail because on cmd line we require correctly formatted setting in option arg
				return 0;
			}
			if (is_amiberry_config_override)
				amiberry_conf_file_overridden_from_cli = true;
			strcpy(argv[i + 1], arg_copy);
			if (!remove_used_args)
				continue;
			// shift all args after the found one by 2
			for (int j = i + 2; j < *argc; j++)
			{
				argv[j - 2] = argv[j];
			}
			// argc is now 2 items shorter ...
			*argc -= 2;
			// ... and we must read this index again because of the shifting we did
			i--;
		}
	}

	return 1;
}

static int get_env_dir( char * path, const char *path_template, const char *envname )
{
	int ret = 0;
	char *ep = getenv(envname);
	if( ep != nullptr) {
		_sntprintf(path, MAX_DPATH, path_template, ep );
		DIR* tdir = opendir(path);
		if (tdir) {
			closedir(tdir);
			ret = 1;
		}
	}
	return ret;
}

// Get the value of one of the XDG_*_HOME variables, as per the spec:
// https://specifications.freedesktop.org/basedir-spec/latest/
static std::string get_xdg_home(const char *var, const std::string& suffix)
{
	// If the variable is set explicitly, return its value
	const auto env_value = getenv(var);
	if (env_value != nullptr)
	{
		return { env_value };
	}

	// If HOME is set, construct the default value relative to it
	const auto home = getenv("HOME");
	if (home != nullptr)
	{
		return std::string(home) + suffix;
	}

	// Return "" so directory_exists will fail
	return { "" };
}

static std::string get_xdg_data_home()
{
	return get_xdg_home("XDG_DATA_HOME", "/.local/share");
}

static std::string get_xdg_config_home()
{
	return get_xdg_home("XDG_CONFIG_HOME", "/.config");
}

bool directory_exists(std::string directory, const std::string& sub_dir)
{
	if (directory.empty() || sub_dir.empty()) return false;
	directory += sub_dir;
	return my_existsdir(directory.c_str());
}

bool file_exists(const std::string& file)
{
#ifdef USE_OLDGCC
	namespace fs = std::experimental::filesystem;
#else
	namespace fs = std::filesystem;
#endif
	fs::path f{ file };
	return (fs::exists(f));
}

#ifdef _WIN32
static std::string get_windows_executable_directory()
{
	char exepath[MAX_DPATH];
	GetModuleFileNameA(NULL, exepath, MAX_DPATH);
	std::string dir(exepath);
	const auto last_sep = dir.find_last_of("\\/");
	if (last_sep != std::string::npos)
		dir = dir.substr(0, last_sep);
	return dir;
}
#endif

static std::string strip_filename_from_path(const std::string& path)
{
	const auto last_sep = path.find_last_of("\\/");
	if (last_sep == std::string::npos)
		return {};
	return normalize_path_string(path.substr(0, last_sep));
}

static std::string get_desktop_executable_directory()
{
#ifdef _WIN32
	return get_windows_executable_directory();
#elif defined(__MACH__)
	char exepath[MAX_DPATH];
	uint32_t size = sizeof exepath;
	if (_NSGetExecutablePath(exepath, &size) == 0)
		return strip_filename_from_path(exepath);
	return {};
#else
	if (const char* base_path = SDL_GetBasePath())
	{
		const auto executable_directory = normalize_path_string(base_path);
		if (!executable_directory.empty())
			return executable_directory;
	}

	char exepath[MAX_DPATH];
	const auto len = readlink("/proc/self/exe", exepath, sizeof exepath - 1);
	if (len > 0)
	{
		exepath[len] = '\0';
		return strip_filename_from_path(exepath);
	}

	return {};
#endif
}

static std::string get_portable_root_directory()
{
	const auto executable_directory = get_desktop_executable_directory();
	if (!executable_directory.empty())
		return executable_directory;

	return {};
}

static std::string get_portable_mode_marker_path()
{
#if defined(__MACH__) || defined(__ANDROID__)
	return {};
#else
	return join_path(get_portable_root_directory(), "amiberry.portable");
#endif
}

static bool is_portable_mode_enabled()
{
	const auto portable_marker = get_portable_mode_marker_path();
	if (portable_marker.empty())
		return false;

	return my_existsfile2(portable_marker.c_str());
}

bool get_portable_mode()
{
	return g_portable_mode;
}

#if !defined(__MACH__) && !defined(__ANDROID__)
bool can_toggle_portable_mode(std::string* reason_out)
{
	const auto portable_root = get_portable_root_directory();
	if (portable_root.empty())
	{
		if (reason_out != nullptr)
			*reason_out = "Amiberry could not resolve the executable directory.";
		return false;
	}

	const auto marker = get_portable_mode_marker_path();
	if (marker.empty())
	{
		if (reason_out != nullptr)
			*reason_out = "Amiberry could not resolve the portable marker path.";
		return false;
	}

#ifdef _WIN32
	const bool can_write_portable_root = _access(portable_root.c_str(), 2) == 0;
#else
	const bool can_write_portable_root = access(portable_root.c_str(), W_OK) == 0;
#endif
	if (!can_write_portable_root)
	{
		if (reason_out != nullptr)
			*reason_out = "Amiberry cannot write to the application directory:\n\n" + portable_root;
		return false;
	}

	return true;
}

bool set_portable_mode(bool enable)
{
	if (!can_toggle_portable_mode(nullptr))
		return false;

	const auto marker = get_portable_mode_marker_path();
	if (marker.empty())
		return false;

	if (enable)
	{
		FILE* f = fopen(marker.c_str(), "w");
		if (!f)
			return false;
		fclose(f);
	}
	else
	{
		std::remove(marker.c_str());
	}
	g_portable_mode = enable;
	return true;
}
#endif

#ifndef LIBRETRO
#ifdef AMIBERRY_HAS_CURL
static size_t curl_write_file_cb(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	auto* fp = static_cast<FILE*>(userdata);
	return fwrite(ptr, size, nmemb, fp);
}

static void ensure_curl_initialized()
{
	static std::once_flag s_curl_once;
	std::call_once(s_curl_once, []() {
		curl_global_init(CURL_GLOBAL_DEFAULT);
	});
}

bool download_file(const std::string& source, const std::string& destination, bool keep_backup)
{
	ensure_curl_initialized();

	auto tmp = destination;
	tmp = tmp.append(".tmp");

	if (file_exists(tmp))
	{
		write_log("Existing file found, removing %s\n", tmp.c_str());
		if (std::remove(tmp.c_str()) < 0)
		{
			write_log(strerror(errno));
			write_log("\n");
		}
	}

	FILE* fp = fopen(tmp.c_str(), "wb");
	if (!fp)
	{
		write_log("Failed to open temporary file for writing: %s\n", tmp.c_str());
		return false;
	}

	CURL* curl = curl_easy_init();
	if (!curl)
	{
		write_log("Failed to initialize libcurl\n");
		fclose(fp);
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, source.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Amiberry");
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

	const CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(fp);

	if (res != CURLE_OK)
	{
		write_log("Download failed: %s (URL: %s)\n", curl_easy_strerror(res), source.c_str());
		std::remove(tmp.c_str());
		return false;
	}

	if (file_exists(tmp))
	{
		if (file_exists(destination) && keep_backup)
		{
			write_log("Backup requested, renaming destination file %s to .bak\n", destination.c_str());
			const std::string new_filename = destination.substr(0, destination.find_last_of('.')).append(".bak");
			if (std::rename(destination.c_str(), new_filename.c_str()) < 0)
			{
				write_log(strerror(errno));
				write_log("\n");
			}
		}

		write_log("Renaming downloaded temporary file %s to final destination\n", tmp.c_str());
		if (std::rename(tmp.c_str(), destination.c_str()) < 0)
		{
			write_log(strerror(errno));
			write_log("\n");
		}
		return true;
	}

	return false;
}

struct download_progress_ctx
{
	std::function<bool(int64_t, int64_t)> progress_cb;
	std::atomic<bool>* cancel_flag = nullptr;
};

static int download_progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
{
	auto* ctx = static_cast<download_progress_ctx*>(clientp);
	if (!ctx)
		return 0;
	if (ctx->cancel_flag && ctx->cancel_flag->load())
		return 1;
	if (ctx->progress_cb)
	{
		const bool should_cancel = ctx->progress_cb(static_cast<int64_t>(dlnow), static_cast<int64_t>(dltotal));
		if (should_cancel)
			return 1;
	}
	return 0;
}

bool download_file(const std::string& source, const std::string& destination, bool keep_backup,
	const std::function<bool(int64_t, int64_t)>& progress_cb, std::atomic<bool>* cancel_flag)
{
	ensure_curl_initialized();

	auto tmp = destination;
	tmp = tmp.append(".tmp");

	if (file_exists(tmp))
	{
		write_log("Existing file found, removing %s\n", tmp.c_str());
		if (std::remove(tmp.c_str()) < 0)
		{
			write_log(strerror(errno));
			write_log("\n");
		}
	}

	FILE* fp = fopen(tmp.c_str(), "wb");
	if (!fp)
	{
		write_log("Failed to open temporary file for writing: %s\n", tmp.c_str());
		return false;
	}

	CURL* curl = curl_easy_init();
	if (!curl)
	{
		write_log("Failed to initialize libcurl\n");
		fclose(fp);
		return false;
	}

	download_progress_ctx pctx;
	pctx.progress_cb = progress_cb;
	pctx.cancel_flag = cancel_flag;

	curl_easy_setopt(curl, CURLOPT_URL, source.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Amiberry");
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, download_progress_callback);
	curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &pctx);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

	const CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(fp);

	if (res != CURLE_OK)
	{
		write_log("Download failed: %s (URL: %s)\n", curl_easy_strerror(res), source.c_str());
		std::remove(tmp.c_str());
		return false;
	}

	if (file_exists(tmp))
	{
		if (file_exists(destination) && keep_backup)
		{
			write_log("Backup requested, renaming destination file %s to .bak\n", destination.c_str());
			const std::string new_filename = destination.substr(0, destination.find_last_of('.')).append(".bak");
			if (std::rename(destination.c_str(), new_filename.c_str()) < 0)
			{
				write_log(strerror(errno));
				write_log("\n");
			}
		}

		write_log("Renaming downloaded temporary file %s to final destination\n", tmp.c_str());
		if (std::rename(tmp.c_str(), destination.c_str()) < 0)
		{
			write_log(strerror(errno));
			write_log("\n");
		}
		return true;
	}

	return false;
}

void download_rtb(const std::string& filename)
{
	const std::string destination_filename = "save-data/Kickstarts/" + filename;
	const std::string destination = get_whdbootpath().append(destination_filename);
	if (!file_exists(destination))
	{
		write_log("Downloading %s ...\n", destination.c_str());
		const std::string url = "https://github.com/BlitterStudio/amiberry/blob/master/whdboot/save-data/Kickstarts/" + filename + "?raw=true";
		download_file(url, destination, false);
	}
}
#else // !AMIBERRY_HAS_CURL (no curl on this platform, e.g. iOS)
bool download_file(const std::string& source, const std::string& destination, bool keep_backup)
{
	write_log("download_file: not available (no CURL)\n");
	return false;
}

bool download_file(const std::string& source, const std::string& destination, bool keep_backup,
	const std::function<bool(int64_t, int64_t)>& progress_cb, std::atomic<bool>* cancel_flag)
{
	write_log("download_file: not available (no CURL)\n");
	return false;
}

void download_rtb(const std::string& filename)
{
}
#endif // AMIBERRY_HAS_CURL
#else // LIBRETRO
bool download_file(const std::string& source, const std::string& destination, bool keep_backup)
{
	write_log("download_file: not available in libretro build\n");
	return false;
}

bool download_file(const std::string& source, const std::string& destination, bool keep_backup,
	const std::function<bool(int64_t, int64_t)>& progress_cb, std::atomic<bool>* cancel_flag)
{
	write_log("download_file: not available in libretro build\n");
	return false;
}

void download_rtb(const std::string& filename)
{
}
#endif // LIBRETRO

#ifdef AMIBERRY_IOS
// iOS .app bundles are flat: the executable sits at the bundle root.
// Returns the bundle root directory (e.g. /path/to/MyApp.app).
static std::string get_ios_app_bundle_directory()
{
	char exepath[MAX_DPATH];
	uint32_t size = sizeof exepath;
	if (_NSGetExecutablePath(exepath, &size) == 0)
	{
		const std::string path(exepath);
		const size_t last_slash = path.rfind('/');
		if (last_slash != std::string::npos)
			return path.substr(0, last_slash);
	}
	return {};
}
#endif

// this is where the required assets are stored, like fonts, icons, etc.
std::string get_data_directory(bool portable_mode)
{
#ifdef AMIBERRY_IOS
	return get_ios_app_bundle_directory() + "/data/";
#elif defined(AMIBERRY_MACOS)
	char exepath[MAX_DPATH];
	uint32_t size = sizeof exepath;
	std::string directory;
	if (_NSGetExecutablePath(exepath, &size) == 0)
	{
		size_t last_slash_idx = string(exepath).rfind('/');
		if (std::string::npos != last_slash_idx)
		{
			directory = string(exepath).substr(0, last_slash_idx);
		}
		last_slash_idx = directory.rfind('/');
		if (std::string::npos != last_slash_idx)
		{
			directory = directory.substr(0, last_slash_idx);
		}
	}
	return directory + "/Resources/data/";
#elif defined(__ANDROID__)
    return prefix_with_application_directory_path("data/");
#elif defined(_WIN32)
	{
		// Use the executable's directory, not getcwd(), because file associations
		// and shortcuts can launch the app with an arbitrary working directory.
		return get_windows_executable_directory() + "\\data\\";
	}
#else
	if (portable_mode)
	{
		const auto portable_root = get_portable_root_directory();
		write_log("Portable mode: Setting data directory relative to executable path: %s\n", portable_root.c_str());
		return join_path(portable_root, "data");
	}

	const auto env_data_dir = getenv("AMIBERRY_DATA_DIR");
	const auto xdg_data_home = get_xdg_data_home();

	// 1: Check if the $AMIBERRY_DATA_DIR ENV variable is set
	if (env_data_dir != nullptr && my_existsdir(env_data_dir))
	{
		// If the ENV variable is set, and it actually contains our data dir, use it
		write_log("Using data directory from AMIBERRY_DATA_DIR: %s\n", env_data_dir);
		return {env_data_dir};
	}
	// 2: Check for system-wide installation (e.g. from a .deb package)
	if (directory_exists(AMIBERRY_DATADIR, "/data"))
	{
		// If the data directory exists in AMIBERRY_DATADIR, use it
		// This would be the default location after an installation with the .deb package
		write_log("Using data directory from " AMIBERRY_DATADIR "\n");
		return std::string(AMIBERRY_DATADIR) + "/data/";
	}

	// 3: Check for a local executable-relative data dir (e.g. local build or unpacked tree)
	const auto executable_directory = get_desktop_executable_directory();
	if (directory_exists(executable_directory, "/data"))
	{
		write_log("Using data directory from executable path: %s\n", executable_directory.c_str());
		return join_path(executable_directory, "data");
	}

	write_log("Unable to resolve a usable data directory.\n");
	return {};
#endif
}

// This path wil be used to create most of the user-specific files and directories
// Kickstart ROMs, HDD images, Floppy images will live under this directory
#ifdef AMIBERRY_MACOS
static std::string get_default_macos_content_root()
{
	const auto user_home_dir = getenv("HOME");
	if (user_home_dir == nullptr || user_home_dir[0] == '\0')
		return {};

	return normalize_path_string(std::string(user_home_dir) + "/Documents/Amiberry");
}
#endif

#ifdef _WIN32
static std::string get_default_windows_content_root()
{
	const auto user_home_dir = getenv("USERPROFILE");
	if (user_home_dir == nullptr || user_home_dir[0] == '\0')
		return {};

	return normalize_path_string(std::string(user_home_dir) + "\\Amiberry");
}
#endif

static std::string get_default_posix_content_root()
{
	const auto user_home_dir = getenv("HOME");
	if (user_home_dir == nullptr || user_home_dir[0] == '\0')
		return {};

	return normalize_path_string(std::string(user_home_dir) + "/Amiberry");
}

std::string get_home_directory(const bool portable_mode)
{
#if defined(AMIBERRY_IOS)
	// iOS: sandboxed Documents directory, similar to Android
	const auto user_home_dir = getenv("HOME");
	if (user_home_dir != nullptr && user_home_dir[0] != '\0')
	{
		write_log("iOS: Using home directory %s/Documents/Amiberry\n", user_home_dir);
		return normalize_path_string(std::string(user_home_dir) + "/Documents/Amiberry");
	}
	return {};
#elif defined(__ANDROID__)
	const char* path = SDL_GetAndroidExternalStoragePath();
	if (path) {
		std::string home(path);
		home += "/";
		return home;
	}
	return prefix_with_application_directory_path("");
#elif defined(_WIN32)
	if (portable_mode)
	{
		write_log("Portable mode: Setting home directory to executable path\n");
		return get_windows_executable_directory();
	}
	{
		const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
		if (env_home_dir != nullptr && env_home_dir[0] != '\0')
		{
			write_log("Using home directory from AMIBERRY_HOME_DIR: %s\n", env_home_dir);
			return { env_home_dir };
		}
		const auto default_home_dir = get_default_windows_content_root();
		if (!default_home_dir.empty())
		{
			// Keep path discovery side-effect free so migrated/customized setups do not recreate default folders.
			write_log("Using home directory from %%USERPROFILE%%\\Amiberry\n");
			return default_home_dir;
		}
		write_log("Fallback: Setting home directory to executable path\n");
		return get_windows_executable_directory();
	}
#endif
#ifdef AMIBERRY_MACOS
	// macOS: check AMIBERRY_HOME_DIR first, then default to ~/Documents/Amiberry.
	// Keep path discovery side-effect free so migrated setups do not recreate legacy folders.
	{
		const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
		if (env_home_dir != nullptr && env_home_dir[0] != '\0')
		{
			write_log("macOS: Using home directory from AMIBERRY_HOME_DIR: %s\n", env_home_dir);
			return { env_home_dir };
		}
		const auto default_home_dir = get_default_macos_content_root();
		if (!default_home_dir.empty())
		{
			write_log("macOS: Using home directory %s\n", default_home_dir.c_str());
			return default_home_dir;
		}
	}
#endif
	if (portable_mode)
	{
		write_log("Portable mode: Setting home directory to executable path\n");
		return get_portable_root_directory();
	}

	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");

	// 1: Check if the $AMIBERRY_HOME_DIR ENV variable is set
	if (env_home_dir != nullptr && env_home_dir[0] != '\0')
	{
		// If the ENV variable is set, use it
		write_log("Using home directory from AMIBERRY_HOME_DIR: %s\n", env_home_dir);
		return { env_home_dir };
	}
	// 2: Check $HOME/Amiberry
	const auto default_home_dir = get_default_posix_content_root();
	if (!default_home_dir.empty())
	{
		// Keep path discovery side-effect free so migrated/customized setups do not recreate default folders.
		write_log("Using home directory from $HOME/Amiberry\n");
		return default_home_dir;
	}

	// 3: Fallback to the executable directory when no user home is available.
	const auto portable_root = get_portable_root_directory();
	write_log("Fallback: Setting home directory to executable path\n");
	return portable_root;
}

// The location of .uae configurations
std::string get_config_directory(bool portable_mode)
{
#ifdef AMIBERRY_IOS
	return join_path(get_home_directory(false), "Configurations");
#elif defined(AMIBERRY_MACOS)
	{
		const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
		if (env_home_dir != nullptr && env_home_dir[0] != '\0')
			return normalize_path_string(std::string(env_home_dir) + "/Configurations");

		const auto default_home_dir = get_default_macos_content_root();
		if (!default_home_dir.empty())
			return join_path(default_home_dir, "Configurations");

		return {};
	}
#elif defined(_WIN32)
	if (portable_mode)
	{
		write_log("Portable mode: Setting config directory to executable path\n");
		return normalize_path_string(get_windows_executable_directory() + "\\" + get_configurations_directory_name());
	}
	{
		const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
		if (env_home_dir != nullptr && env_home_dir[0] != '\0')
			return normalize_path_string(std::string(env_home_dir) + "\\Configurations");

		const auto default_home_dir = get_default_windows_content_root();
		if (!default_home_dir.empty())
			return normalize_path_string(default_home_dir + "\\Configurations");
		return normalize_path_string(get_windows_executable_directory() + "\\" + get_configurations_directory_name());
	}
#elif defined(__ANDROID__)
	const char* path = SDL_GetAndroidExternalStoragePath();
	if (path) {
		auto config_dir = join_path(path, get_configurations_directory_name());
		return fix_trailing(config_dir);
	}
	return prefix_with_application_directory_path(std::string(get_configurations_directory_name()) + "/");
#else
	if (portable_mode)
	{
		write_log("Portable mode: Setting config directory to executable path\n");
		return join_path(get_portable_root_directory(), get_configurations_directory_name());
	}

	const auto env_conf_dir = getenv("AMIBERRY_CONFIG_DIR");
	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");

	// 1: Check if the $AMIBERRY_CONFIG_DIR ENV variable is set
	if (env_conf_dir != nullptr && env_conf_dir[0] != '\0')
	{
		// If the ENV variable is set, use it
		write_log("Using config directory from AMIBERRY_CONFIG_DIR: %s\n", env_conf_dir);
		return { env_conf_dir };
	}
	// 2: Check if the $AMIBERRY_HOME_DIR ENV variable is set
	if (env_home_dir != nullptr && env_home_dir[0] != '\0')
		return join_path(env_home_dir, get_configurations_directory_name());

	// 2: Check $HOME/Amiberry/Configurations
	const auto default_home_dir = get_default_posix_content_root();
	if (!default_home_dir.empty())
		return join_path(default_home_dir, get_configurations_directory_name());

	// 3: Fallback to the executable directory when $HOME is unavailable.
	write_log("Using config directory from executable path\n");
	return join_path(get_portable_root_directory(), get_configurations_directory_name());
#endif
}

// Plugins that Amiberry can use, usually in the form of shared libraries
std::string get_plugins_directory(bool portable_mode)
{
#ifdef AMIBERRY_IOS
	// iOS: no plugins directory (no dynamic loading on iOS)
	return {};
#elif defined(AMIBERRY_MACOS)
	char exepath[MAX_DPATH];
	uint32_t size = sizeof exepath;
	std::string directory;
	if (_NSGetExecutablePath(exepath, &size) == 0)
	{
		size_t last_slash_idx = string(exepath).rfind('/');
		if (std::string::npos != last_slash_idx)
		{
			directory = string(exepath).substr(0, last_slash_idx);
		}
		last_slash_idx = directory.rfind('/');
		if (std::string::npos != last_slash_idx)
		{

			directory = directory.substr(0, last_slash_idx);
		}
	}
	return directory + "/Resources/plugins/";
#elif defined(__ANDROID__)
	return prefix_with_application_directory_path("plugins/");
#elif defined(_WIN32)
	{
		return get_windows_executable_directory() + "\\plugins";
	}
#else
	if (portable_mode)
	{
		write_log("Portable mode: Setting plugins directory to executable path\n");
		return join_path(get_portable_root_directory(), "plugins");
	}

	// 1: Check if the $AMIBERRY_PLUGINS_DIR ENV variable is set
	const auto env_plugins_dir = getenv("AMIBERRY_PLUGINS_DIR");
	if (env_plugins_dir != nullptr && env_plugins_dir[0] != '\0')
	{
		// If the ENV variable is set, use it
		write_log("Using config directory from AMIBERRY_PLUGINS_DIR: %s\n", env_plugins_dir);
		return { env_plugins_dir };
	}
	// 2: Check system-wide location (e.g. installed from a .deb package)
	if (my_existsdir(AMIBERRY_LIBDIR))
	{
		write_log("Using plugins directory from " AMIBERRY_LIBDIR "\n");
		return AMIBERRY_LIBDIR;
	}
	// 3: Check for $AMIBERRY_HOME_DIR/plugins
	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
	if (env_home_dir != nullptr && env_home_dir[0] != '\0')
	{
		write_log("Using plugins directory from AMIBERRY_HOME_DIR/plugins\n");
		return { std::string(env_home_dir) + "/plugins" };
	}
	// 4: Check for ~/Amiberry/plugins.
	// Keep path discovery side-effect free so migrated/customized setups do not recreate default folders.
	const auto default_home_dir = get_default_posix_content_root();
	if (!default_home_dir.empty())
	{
		write_log("Using plugins directory from $HOME/Amiberry/plugins\n");
		return join_path(default_home_dir, "plugins");
	}

	// 5: Fallback to the executable directory when no other plugin path is available.
	write_log("Using plugins directory from executable path\n");
	return join_path(get_portable_root_directory(), "plugins");
#endif
}

static void append_unique_path_candidate(std::vector<std::string>& candidates, const std::string& candidate)
{
	if (candidate.empty())
		return;

	const auto normalized_candidate = normalize_path_string(candidate);
	for (const auto& existing : candidates)
	{
		if (path_strings_match(existing, normalized_candidate))
			return;
	}
	candidates.emplace_back(normalized_candidate);
}

static void append_settings_candidate(std::vector<std::string>& candidates, const std::string& candidate)
{
	append_unique_path_candidate(candidates, candidate);
}

static std::string get_settings_directory(const bool portable_mode)
{
	if (portable_mode)
		return join_path(get_portable_root_directory(), "Settings");

#ifdef AMIBERRY_IOS
	// iOS: settings in the Documents sandbox
	return join_path(get_home_directory(false), "Settings");
#elif defined(AMIBERRY_MACOS)
	const auto user_home_dir = getenv("HOME");
	if (user_home_dir != nullptr)
		return normalize_path_string(std::string(user_home_dir) + "/Library/Application Support/Amiberry");
	return join_path(get_portable_root_directory(), "Settings");
#elif defined(__ANDROID__)
	if (char* pref_path = SDL_GetPrefPath("BlitterStudio", "Amiberry"))
	{
		const auto pref_dir = normalize_path_string(pref_path);
		SDL_free(pref_path);
		if (!pref_dir.empty())
			return pref_dir;
	}
	return normalize_path_string(get_home_directory(false));
#elif defined(_WIN32)
	const auto local_app_data = getenv("LOCALAPPDATA");
	if (local_app_data != nullptr && local_app_data[0] != '\0')
		return normalize_path_string(std::string(local_app_data) + "\\Amiberry");
	const auto roaming_app_data = getenv("APPDATA");
	if (roaming_app_data != nullptr && roaming_app_data[0] != '\0')
		return normalize_path_string(std::string(roaming_app_data) + "\\Amiberry");
	const auto user_home_dir = getenv("USERPROFILE");
	if (user_home_dir != nullptr && user_home_dir[0] != '\0')
		return normalize_path_string(std::string(user_home_dir) + "\\AppData\\Local\\Amiberry");
	return join_path(get_portable_root_directory(), "Settings");
#else
	return normalize_path_string(get_xdg_config_home() + "/amiberry");
#endif
}

static std::vector<std::string> get_legacy_settings_candidate_directories(const bool portable_mode)
{
	std::vector<std::string> candidates;

	if (portable_mode)
	{
		const auto portable_root = get_portable_root_directory();
		append_settings_candidate(candidates, join_path(portable_root, "conf"));
		append_settings_candidate(candidates, join_path(portable_root, "Configurations"));
		return candidates;
	}

#ifdef AMIBERRY_IOS
	append_settings_candidate(candidates, join_path(get_home_directory(false), "Configurations"));
#elif defined(AMIBERRY_MACOS)
	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
	if (env_home_dir != nullptr && my_existsdir(env_home_dir))
		append_settings_candidate(candidates, join_path(env_home_dir, "Configurations"));

	const auto user_home_dir = getenv("HOME");
	if (user_home_dir != nullptr)
	{
		append_settings_candidate(candidates,
			join_path(std::string(user_home_dir) + "/Documents/Amiberry", "Configurations"));
		append_settings_candidate(candidates,
			join_path(std::string(user_home_dir) + "/Library/Application Support/Amiberry", "Configurations"));
	}
#elif defined(_WIN32)
	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
	if (env_home_dir != nullptr && env_home_dir[0] != '\0' && my_existsdir(env_home_dir))
		append_settings_candidate(candidates, join_path(env_home_dir, "Configurations"));

	const auto user_home_dir = getenv("USERPROFILE");
	if (user_home_dir != nullptr && user_home_dir[0] != '\0')
		append_settings_candidate(candidates, std::string(user_home_dir) + "\\Amiberry\\Configurations");
#elif defined(__ANDROID__)
	append_settings_candidate(candidates, get_home_directory(false));
#else
	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
	if (env_home_dir != nullptr && my_existsdir(env_home_dir))
		append_settings_candidate(candidates, join_path(env_home_dir, "conf"));

	const auto default_home_dir = get_default_posix_content_root();
	if (!default_home_dir.empty())
		append_settings_candidate(candidates, join_path(default_home_dir, "conf"));
#endif

	return candidates;
}

static std::vector<std::string> get_legacy_configuration_candidate_directories(const bool portable_mode)
{
	std::vector<std::string> candidates;

	if (portable_mode)
	{
		append_settings_candidate(candidates, join_path(get_portable_root_directory(), "conf"));
		return candidates;
	}

#if defined(_WIN32)
	append_settings_candidate(candidates, get_windows_executable_directory() + "\\conf");
#elif !defined(__ANDROID__) && !defined(AMIBERRY_IOS) && !defined(AMIBERRY_MACOS)
	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
	if (env_home_dir != nullptr && env_home_dir[0] != '\0' && my_existsdir(env_home_dir))
		append_settings_candidate(candidates, join_path(env_home_dir, "conf"));

	const auto default_home_dir = get_default_posix_content_root();
	if (!default_home_dir.empty())
		append_settings_candidate(candidates, join_path(default_home_dir, "conf"));
#endif

	return candidates;
}

static std::vector<std::string> get_legacy_bookmark_candidate_directories(const bool portable_mode)
{
	std::vector<std::string> candidates;

#if defined(MACOS_APP_STORE)
	append_settings_candidate(candidates, home_dir);
	append_settings_candidate(candidates, get_home_directory(portable_mode));
#endif

	return candidates;
}

static std::string get_existing_settings_file_for_resolution(const bool portable_mode)
{
	if (!amiberry_conf_file.empty() && my_existsfile2(amiberry_conf_file.c_str()))
		return amiberry_conf_file;

	if (amiberry_conf_file_overridden_from_cli)
		return {};

	for (const auto& candidate_directory : get_legacy_settings_candidate_directories(portable_mode))
	{
		if (candidate_directory.empty())
			continue;

		const auto candidate_file = join_path(candidate_directory, "amiberry.conf");
		if (my_existsfile2(candidate_file.c_str()))
			return candidate_file;
	}

	return {};
}

static bool copy_file_if_missing(const std::string& source_file, const std::string& destination_file, bool& failed)
{
	failed = false;
	if (source_file.empty() || destination_file.empty())
		return false;
	if (!my_existsfile2(source_file.c_str()))
		return false;
	if (path_strings_match(source_file, destination_file))
		return false;
	if (my_existsfile2(destination_file.c_str()))
		return false;

	ensure_parent_directory_exists(destination_file);

	std::error_code ec;
	std::filesystem::copy_file(source_file, destination_file, std::filesystem::copy_options::none, ec);
	if (ec)
	{
		write_log("Settings migration: failed to copy %s to %s: %s\n",
			source_file.c_str(), destination_file.c_str(), ec.message().c_str());
		failed = true;
		return false;
	}

	write_log("Settings migration: imported %s from %s\n",
		destination_file.c_str(), source_file.c_str());
	return true;
}

static bool import_legacy_settings_file_if_needed(const std::vector<std::string>& candidate_directories,
	const char* filename, bool& failed)
{
	const auto destination_file = join_path(settings_dir, filename);
	if (destination_file.empty() || my_existsfile2(destination_file.c_str()))
	{
		failed = false;
		return false;
	}

	for (const auto& candidate_directory : candidate_directories)
	{
		if (candidate_directory.empty())
			continue;

		const auto source_file = join_path(candidate_directory, filename);
		if (copy_file_if_missing(source_file, destination_file, failed))
			return true;
		if (failed)
			return false;
	}

	failed = false;
	return false;
}

static void migrate_legacy_settings_files(const bool portable_mode)
{
	const auto candidate_directories = get_legacy_settings_candidate_directories(portable_mode);
	bool conf_copy_failed = false;
	bool ini_copy_failed = false;
	const bool conf_copied = import_legacy_settings_file_if_needed(candidate_directories, "amiberry.conf", conf_copy_failed);
	const bool ini_copied = import_legacy_settings_file_if_needed(candidate_directories, "amiberry.ini", ini_copy_failed);
	if (conf_copied)
		legacy_migration_state.config_migrated = true;
	if (conf_copy_failed)
		legacy_migration_state.config_failed = true;
	if (ini_copied)
		legacy_migration_state.state_migrated = true;
	if (ini_copy_failed)
		legacy_migration_state.state_failed = true;
}

static void append_visual_asset_candidate(std::vector<visual_asset_path_set>& candidates,
	const visual_asset_path_set& candidate)
{
	if (candidate.themes_path.empty() && candidate.shaders_path.empty() && candidate.bezels_path.empty())
		return;

	for (const auto& existing : candidates)
	{
		if (path_strings_match(existing.themes_path, candidate.themes_path)
			&& path_strings_match(existing.shaders_path, candidate.shaders_path)
			&& path_strings_match(existing.bezels_path, candidate.bezels_path))
		{
			return;
		}
	}

	candidates.emplace_back(candidate);
}

static bool merge_directory_contents_if_needed(const std::string& source_dir, const std::string& destination_dir,
	bool& failed, bool& conflicts, const char* migration_label)
{
	failed = false;
	conflicts = false;
	if (source_dir.empty() || destination_dir.empty())
		return false;
	if (!my_existsdir(source_dir.c_str()))
		return false;
	if (path_strings_match(source_dir, destination_dir))
		return false;

	ensure_directory_exists(destination_dir);

	bool copied_any = false;
	std::error_code ec;
	std::filesystem::recursive_directory_iterator iterator(source_dir,
		std::filesystem::directory_options::skip_permission_denied, ec);
	if (ec)
	{
		write_log("%s migration: failed to scan %s: %s\n",
			migration_label, source_dir.c_str(), ec.message().c_str());
		failed = true;
		return false;
	}

	const std::filesystem::recursive_directory_iterator end;
	for (; iterator != end; iterator.increment(ec))
	{
		if (ec)
		{
			write_log("%s migration: failed while reading %s: %s\n",
				migration_label, source_dir.c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		const auto relative_path = std::filesystem::relative(iterator->path(), source_dir, ec);
		if (ec)
		{
			write_log("%s migration: failed to relativize %s against %s: %s\n",
				migration_label, iterator->path().string().c_str(), source_dir.c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		const auto destination_path = std::filesystem::path(destination_dir) / relative_path;
		if (iterator->is_directory())
		{
			std::filesystem::create_directories(destination_path, ec);
			if (ec)
			{
				write_log("%s migration: failed to create %s: %s\n",
					migration_label, destination_path.string().c_str(), ec.message().c_str());
				failed = true;
				return copied_any;
			}
			continue;
		}

		if (!iterator->is_regular_file())
			continue;

		std::filesystem::create_directories(destination_path.parent_path(), ec);
		if (ec)
		{
			write_log("%s migration: failed to create %s: %s\n",
				migration_label, destination_path.parent_path().string().c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		if (std::filesystem::exists(destination_path))
		{
			conflicts = true;
			write_log("%s migration: keeping existing %s, skipping %s\n",
				migration_label, destination_path.string().c_str(), iterator->path().string().c_str());
			continue;
		}

		std::filesystem::copy_file(iterator->path(), destination_path, std::filesystem::copy_options::none, ec);
		if (ec)
		{
			write_log("%s migration: failed to copy %s to %s: %s\n",
				migration_label, iterator->path().string().c_str(), destination_path.string().c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		write_log("%s migration: imported %s from %s\n",
			migration_label, destination_path.string().c_str(), iterator->path().string().c_str());
		copied_any = true;
	}

	return copied_any;
}

static bool is_legacy_bootstrap_settings_file(const std::filesystem::path& relative_path)
{
	if (relative_path.has_parent_path())
		return false;

	auto filename = relative_path.filename().string();
	std::transform(filename.begin(), filename.end(), filename.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return filename == "amiberry.conf" || filename == "amiberry.ini";
}

static bool merge_legacy_configuration_directory_if_needed(const std::string& source_dir,
	const std::string& destination_dir, bool& failed, bool& conflicts)
{
	failed = false;
	conflicts = false;
	if (source_dir.empty() || destination_dir.empty())
		return false;
	if (!my_existsdir(source_dir.c_str()))
		return false;
	if (path_strings_match(source_dir, destination_dir))
		return false;

	ensure_directory_exists(destination_dir);

	bool copied_any = false;
	std::error_code ec;
	std::filesystem::recursive_directory_iterator iterator(source_dir,
		std::filesystem::directory_options::skip_permission_denied, ec);
	if (ec)
	{
		write_log("Configuration migration: failed to scan %s: %s\n", source_dir.c_str(), ec.message().c_str());
		failed = true;
		return false;
	}

	const std::filesystem::recursive_directory_iterator end;
	for (; iterator != end; iterator.increment(ec))
	{
		if (ec)
		{
			write_log("Configuration migration: failed while reading %s: %s\n", source_dir.c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		const auto relative_path = std::filesystem::relative(iterator->path(), source_dir, ec);
		if (ec)
		{
			write_log("Configuration migration: failed to relativize %s against %s: %s\n",
				iterator->path().string().c_str(), source_dir.c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		if (is_legacy_bootstrap_settings_file(relative_path))
			continue;

		const auto destination_path = std::filesystem::path(destination_dir) / relative_path;
		if (iterator->is_directory())
		{
			std::filesystem::create_directories(destination_path, ec);
			if (ec)
			{
				write_log("Configuration migration: failed to create %s: %s\n",
					destination_path.string().c_str(), ec.message().c_str());
				failed = true;
				return copied_any;
			}
			continue;
		}

		if (!iterator->is_regular_file())
			continue;

		std::filesystem::create_directories(destination_path.parent_path(), ec);
		if (ec)
		{
			write_log("Configuration migration: failed to create %s: %s\n",
				destination_path.parent_path().string().c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		if (std::filesystem::exists(destination_path))
		{
			conflicts = true;
			write_log("Configuration migration: keeping existing %s, skipping %s\n",
				destination_path.string().c_str(), iterator->path().string().c_str());
			continue;
		}

		std::filesystem::copy_file(iterator->path(), destination_path, std::filesystem::copy_options::none, ec);
		if (ec)
		{
			write_log("Configuration migration: failed to copy %s to %s: %s\n",
				iterator->path().string().c_str(), destination_path.string().c_str(), ec.message().c_str());
			failed = true;
			return copied_any;
		}

		write_log("Configuration migration: imported %s from %s\n",
			destination_path.string().c_str(), iterator->path().string().c_str());
		copied_any = true;
	}

	return copied_any;
}

static void migrate_legacy_configuration_directories(const bool portable_mode)
{
	const auto baseline_config_path = get_base_content_override_baseline().config_path;
	if (baseline_config_path.empty())
		return;

	bool config_path_is_legacy_default = path_strings_match(config_path, baseline_config_path);
	for (const auto& candidate : get_legacy_configuration_candidate_directories(portable_mode))
	{
		if (path_strings_match(config_path, candidate))
			config_path_is_legacy_default = true;
	}
	if (!config_path_is_legacy_default)
		return;

	bool failed = false;
	bool conflicts = false;
	bool migrated_any = false;
	bool source_exists = false;
	for (const auto& candidate : get_legacy_configuration_candidate_directories(portable_mode))
	{
		if (candidate.empty() || path_strings_match(candidate, baseline_config_path))
			continue;
		if (my_existsdir(candidate.c_str()))
			source_exists = true;

		bool copy_failed = false;
		bool copy_conflicts = false;
		if (merge_legacy_configuration_directory_if_needed(candidate, baseline_config_path, copy_failed, copy_conflicts))
			migrated_any = true;
		if (copy_failed)
			failed = true;
		if (copy_conflicts)
			conflicts = true;
	}

	if (!failed && !path_strings_match(config_path, baseline_config_path))
	{
		config_path = baseline_config_path;
		legacy_migration_state.settings_rewrite_needed = true;
	}

	if (failed)
		write_log("Configuration migration: completed with errors (see log above)\n");
	else if (migrated_any)
		write_log("Configuration migration: imported legacy files into %s\n", baseline_config_path.c_str());
	else if (conflicts || source_exists)
		write_log("Configuration migration: legacy directory already reconciled with %s\n", baseline_config_path.c_str());
}

static void migrate_legacy_visual_asset_directories()
{
	const auto baseline_visual_paths = get_visual_asset_path_set(get_base_content_override_baseline());
	const auto current_visual_paths = get_current_visual_asset_path_set();
	std::vector<visual_asset_path_set> legacy_candidates;
	append_visual_asset_candidate(legacy_candidates, get_legacy_default_visual_asset_paths(g_portable_mode));

	if (!base_content_path.empty())
		append_visual_asset_candidate(legacy_candidates,
			get_visual_asset_path_set(get_legacy_base_content_path_set(base_content_path)));
	else
	{
		const auto inferred_content_root = infer_content_root_from_configuration_path(config_path);
		if (!inferred_content_root.empty())
			append_visual_asset_candidate(legacy_candidates,
				get_visual_asset_path_set(get_legacy_base_content_path_set(inferred_content_root)));
	}

	bool migrated_any = false;
	bool migration_failed = false;
	bool migration_conflicts = false;

	const auto migrate_visual_directory = [&](const managed_path_option_descriptor* descriptor,
		const std::string& current_path, const std::string& baseline_path)
	{
		if (!path_strings_match(current_path, baseline_path))
			return;

		for (const auto& legacy_paths : legacy_candidates)
		{
			bool copy_failed = false;
			bool copy_conflicts = false;
			const auto source_dir = get_visual_asset_path_for_descriptor(legacy_paths, descriptor);
			if (merge_directory_contents_if_needed(source_dir, current_path, copy_failed, copy_conflicts, "Visuals"))
				migrated_any = true;
			if (copy_failed)
				migration_failed = true;
			if (copy_conflicts)
				migration_conflicts = true;
		}
	};

	migrate_visual_directory(themes_path_descriptor, current_visual_paths.themes_path, baseline_visual_paths.themes_path);
	migrate_visual_directory(shaders_path_descriptor, current_visual_paths.shaders_path, baseline_visual_paths.shaders_path);
	migrate_visual_directory(bezels_path_descriptor, current_visual_paths.bezels_path, baseline_visual_paths.bezels_path);
	if (migrated_any)
		legacy_migration_state.visuals_migrated = true;
	if (migration_failed)
		legacy_migration_state.visuals_failed = true;
	// Duplicate filenames in an already-migrated layout should not suppress the later cleanup prompt.
	// We only keep the "needs manual review" state when this run also imported new files.
	if (migration_conflicts && migrated_any)
		legacy_migration_state.visuals_conflicts = true;
}

static bool rename_path_to_canonical_case(const std::filesystem::path& source,
	const std::filesystem::path& target, std::string& error_message)
{
	std::error_code ec;
	std::filesystem::rename(source, target, ec);
	if (!ec)
		return true;

	error_message = ec.message();

	const auto parent = target.parent_path();
	const auto filename = target.filename().string();
	for (int suffix = 0; suffix < 1000; ++suffix)
	{
		std::filesystem::path temporary = parent / (filename + ".amiberry-case-migration-" + std::to_string(suffix));
		ec.clear();
		if (std::filesystem::exists(temporary, ec))
			continue;

		ec.clear();
		std::filesystem::rename(source, temporary, ec);
		if (ec)
		{
			error_message += "; temporary rename failed: ";
			error_message += ec.message();
			return false;
		}

		ec.clear();
		std::filesystem::rename(temporary, target, ec);
		if (!ec)
			return true;

		error_message += "; final rename failed: ";
		error_message += ec.message();

		std::error_code rollback_ec;
		std::filesystem::rename(temporary, source, rollback_ec);
		if (rollback_ec)
		{
			error_message += "; rollback failed: ";
			error_message += rollback_ec.message();
		}
		return false;
	}

	error_message += "; no temporary migration name available";
	return false;
}

enum class path_case_migration_result
{
	no_change,
	migrated,
	failed,
};

// Legacy content layouts used a mix of lowercase and partially capitalized names.
// Rename case-only variants to the current canonical names so existing installs and
// cross-platform content packs resolve to the same paths on case-sensitive filesystems.
static path_case_migration_result migrate_path_case_if_needed(const std::string& target_path,
	bool& migrated_any, bool& failed)
{
	if (target_path.empty())
		return path_case_migration_result::no_change;

	std::filesystem::path target(target_path);
	while (!target.empty() && target.filename().empty())
		target = target.parent_path();
	if (target.empty())
		return path_case_migration_result::no_change;

	const auto parent = target.parent_path();
	const auto basename = target.filename().string();
	if (parent.empty() || basename.empty())
		return path_case_migration_result::no_change;
	if (!my_existsdir(parent.string().c_str()))
		return path_case_migration_result::no_change;

	std::string basename_lower = basename;
	std::transform(basename_lower.begin(), basename_lower.end(), basename_lower.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	std::error_code ec;
	std::filesystem::directory_iterator it(parent, ec);
	if (ec)
	{
		write_log("Directory case migration: failed to scan %s: %s\n",
			parent.string().c_str(), ec.message().c_str());
		failed = true;
		return path_case_migration_result::failed;
	}

	const std::filesystem::directory_iterator end;
	std::filesystem::path matched_source;
	std::vector<std::filesystem::path> all_matches;
	bool exact_match_exists = false;
	for (; it != end; it.increment(ec))
	{
		if (ec)
		{
			write_log("Directory case migration: failed while reading %s: %s\n",
				parent.string().c_str(), ec.message().c_str());
			failed = true;
			return path_case_migration_result::failed;
		}

		auto entry_name = it->path().filename().string();
		if (entry_name == basename)
		{
			exact_match_exists = true;
			continue;
		}

		std::string entry_lower = entry_name;
		std::transform(entry_lower.begin(), entry_lower.end(), entry_lower.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (entry_lower != basename_lower)
			continue;

		all_matches.push_back(it->path());
		if (matched_source.empty())
			matched_source = it->path();
	}

	if (matched_source.empty())
		return path_case_migration_result::no_change;

	if (exact_match_exists)
	{
		std::error_code type_ec;
		if (std::filesystem::is_directory(matched_source, type_ec)
			&& !type_ec
			&& std::filesystem::is_directory(target, type_ec)
			&& !type_ec)
		{
			bool copy_failed = false;
			bool copy_conflicts = false;
			if (merge_directory_contents_if_needed(matched_source.string(), target.string(),
				copy_failed, copy_conflicts, "Directory case"))
			{
				migrated_any = true;
				return path_case_migration_result::migrated;
			}
			if (copy_failed)
			{
				failed = true;
				return path_case_migration_result::failed;
			}
		}
		write_log("Directory case migration: canonical path %s already exists; leaving %s in place\n",
			target.string().c_str(), matched_source.string().c_str());
		return path_case_migration_result::no_change;
	}

	if (all_matches.size() > 1)
	{
		std::string others;
		for (std::size_t i = 0; i < all_matches.size(); ++i) {
			if (i > 0) others += ", ";
			others += all_matches[i].filename().string();
		}
		write_log("Directory case migration: multiple case-variant matches for %s in %s [%s]; renaming %s and leaving the rest in place — please reconcile manually\n",
			basename.c_str(), parent.string().c_str(), others.c_str(),
			matched_source.filename().string().c_str());
	}

	std::string error_message;
	if (!rename_path_to_canonical_case(matched_source, target, error_message))
	{
		write_log("Directory case migration: failed to rename %s -> %s: %s\n",
			matched_source.string().c_str(), target.string().c_str(), error_message.c_str());
		failed = true;
		return path_case_migration_result::failed;
	}

	write_log("Directory case migration: renamed %s -> %s\n",
		matched_source.string().c_str(), target.string().c_str());
	migrated_any = true;
	return path_case_migration_result::migrated;
}

static void migrate_legacy_lowercase_content_directories()
{
	const auto baseline = get_base_content_override_baseline();
	auto current = get_current_base_content_path_set();

	bool migrated_any = false;
	bool failed = false;

	const auto migrate_if_default = [&](std::string& current_path,
		const std::string& baseline_path)
	{
		if (current_path.empty() || baseline_path.empty())
			return;
		if (!path_strings_match_case_insensitive(current_path, baseline_path))
			return;
		const auto migration_result = migrate_path_case_if_needed(baseline_path, migrated_any, failed);
		if (migration_result != path_case_migration_result::failed)
		{
			if (!path_strings_match(current_path, baseline_path))
				legacy_migration_state.settings_rewrite_needed = true;
			current_path = baseline_path;
		}
	};

	// Visual assets live one level deeper (Visuals/Themes etc.). Rename the parent
	// "visuals" -> "Visuals" first so the child renames find the new parent.
	if (path_strings_match_case_insensitive(current.themes_path, baseline.themes_path)
		&& !current.themes_path.empty())
	{
		std::filesystem::path themes(baseline.themes_path);
		while (!themes.empty() && themes.filename().empty())
			themes = themes.parent_path();
		const auto visuals_dir = themes.parent_path();
		if (!visuals_dir.empty())
			migrate_path_case_if_needed(visuals_dir.string(), migrated_any, failed);
	}

	migrate_if_default(current.config_path,       baseline.config_path);
	migrate_if_default(current.controllers_path,  baseline.controllers_path);
	migrate_if_default(current.whdboot_path,      baseline.whdboot_path);
	migrate_if_default(current.whdload_arch_path, baseline.whdload_arch_path);
	migrate_if_default(current.floppy_path,       baseline.floppy_path);
	migrate_if_default(current.harddrive_path,    baseline.harddrive_path);
	migrate_if_default(current.cdrom_path,        baseline.cdrom_path);
	migrate_if_default(current.rom_path,          baseline.rom_path);
	migrate_if_default(current.rp9_path,          baseline.rp9_path);
	migrate_if_default(current.savestate_dir,     baseline.savestate_dir);
	migrate_if_default(current.ripper_path,       baseline.ripper_path);
	migrate_if_default(current.input_dir,         baseline.input_dir);
	migrate_if_default(current.screenshot_dir,    baseline.screenshot_dir);
	migrate_if_default(current.nvram_dir,         baseline.nvram_dir);
	migrate_if_default(current.video_dir,         baseline.video_dir);
	migrate_if_default(current.themes_path,       baseline.themes_path);
	migrate_if_default(current.shaders_path,      baseline.shaders_path);
	migrate_if_default(current.bezels_path,       baseline.bezels_path);
	// Logfile is a regular file, but the same rename logic applies.
	migrate_if_default(current.logfile_path,      baseline.logfile_path);

	apply_base_content_path_set(current);

	if (failed)
		write_log("Directory case migration: completed with errors (see log above)\n");
	else if (migrated_any)
		write_log("Directory case migration: completed (legacy names renamed to canonical names)\n");
}

static constexpr auto LEGACY_CLEANUP_DISMISSED_KEY = _T("LegacyCleanupDismissed");

static void append_legacy_cleanup_item(const std::string& label, const std::string& path, const bool is_directory)
{
	if (path.empty())
		return;

	for (const auto& item : legacy_cleanup_items)
	{
		if (path_strings_match(item.path, path))
			return;
	}

	legacy_cleanup_items.push_back({label, normalize_path_string(path), is_directory});
}

static void collect_legacy_settings_cleanup_items()
{
	if (amiberry_conf_file_overridden_from_cli)
		return;

	for (const auto& candidate_directory : get_legacy_settings_candidate_directories(g_portable_mode))
	{
		if (candidate_directory.empty())
			continue;

		const auto legacy_conf = join_path(candidate_directory, "amiberry.conf");
		if (my_existsfile2(legacy_conf.c_str()) && !path_strings_match(legacy_conf, amiberry_conf_file))
			append_legacy_cleanup_item("Legacy settings file", legacy_conf, false);

		const auto legacy_ini = join_path(candidate_directory, "amiberry.ini");
		if (my_existsfile2(legacy_ini.c_str()) && !path_strings_match(legacy_ini, amiberry_ini_file))
			append_legacy_cleanup_item("Legacy state file", legacy_ini, false);
	}
}

static void collect_legacy_bookmark_cleanup_items()
{
#if defined(MACOS_APP_STORE)
	const auto current_bookmarks_file = join_path(settings_dir, "bookmarks.plist");
	for (const auto& candidate_directory : get_legacy_bookmark_candidate_directories(g_portable_mode))
	{
		const auto legacy_bookmarks_file = join_path(candidate_directory, "bookmarks.plist");
		if (my_existsfile2(legacy_bookmarks_file.c_str())
			&& !path_strings_match(legacy_bookmarks_file, current_bookmarks_file))
		{
			append_legacy_cleanup_item("Legacy security bookmarks", legacy_bookmarks_file, false);
		}
	}
#endif
}

static std::vector<visual_asset_path_set> get_legacy_visual_asset_candidates_for_current_state()
{
	std::vector<visual_asset_path_set> legacy_candidates;
	append_visual_asset_candidate(legacy_candidates, get_legacy_default_visual_asset_paths(g_portable_mode));

	if (!base_content_path.empty())
	{
		append_visual_asset_candidate(legacy_candidates,
			get_visual_asset_path_set(get_legacy_base_content_path_set(base_content_path)));
	}
	else
	{
		const auto inferred_content_root = infer_content_root_from_configuration_path(config_path);
		if (!inferred_content_root.empty())
		{
			append_visual_asset_candidate(legacy_candidates,
				get_visual_asset_path_set(get_legacy_base_content_path_set(inferred_content_root)));
		}
	}

	return legacy_candidates;
}

static void collect_legacy_visual_cleanup_items()
{
	const auto current_visual_paths = get_current_visual_asset_path_set();
	for (const auto& legacy_paths : get_legacy_visual_asset_candidates_for_current_state())
	{
		if (!legacy_paths.themes_path.empty()
			&& my_existsdir(legacy_paths.themes_path.c_str())
			&& !path_strings_match(legacy_paths.themes_path, current_visual_paths.themes_path))
		{
			append_legacy_cleanup_item("Legacy Themes folder", legacy_paths.themes_path, true);
		}

		if (!legacy_paths.shaders_path.empty()
			&& my_existsdir(legacy_paths.shaders_path.c_str())
			&& !path_strings_match(legacy_paths.shaders_path, current_visual_paths.shaders_path))
		{
			append_legacy_cleanup_item("Legacy Shaders folder", legacy_paths.shaders_path, true);
		}

		if (!legacy_paths.bezels_path.empty()
			&& my_existsdir(legacy_paths.bezels_path.c_str())
			&& !path_strings_match(legacy_paths.bezels_path, current_visual_paths.bezels_path))
		{
			append_legacy_cleanup_item("Legacy Bezels folder", legacy_paths.bezels_path, true);
		}
	}
}

static std::string get_legacy_cleanup_destination_root()
{
#ifdef AMIBERRY_MACOS
	const auto home_dir_env = getenv("HOME");
	if (home_dir_env != nullptr && home_dir_env[0] != '\0')
		return normalize_path_string(std::string(home_dir_env) + "/.Trash");
#endif
	return join_path(settings_dir, "Legacy Cleanup");
}

static std::filesystem::path get_unique_cleanup_destination(const std::string& destination_root,
	const std::filesystem::path& source_path)
{
	std::filesystem::path candidate = std::filesystem::path(destination_root) / source_path.filename();
	if (!std::filesystem::exists(candidate))
		return candidate;

	const auto stem = source_path.stem().string();
	const auto extension = source_path.extension().string();
	for (int suffix = 1; suffix < 1000; ++suffix)
	{
		std::filesystem::path unique_name;
		if (source_path.has_extension())
			unique_name = stem + " (" + std::to_string(suffix) + ")" + extension;
		else
			unique_name = source_path.filename().string() + " (" + std::to_string(suffix) + ")";

		candidate = std::filesystem::path(destination_root) / unique_name;
		if (!std::filesystem::exists(candidate))
			return candidate;
	}

	return std::filesystem::path(destination_root) / (source_path.filename().string() + ".migrated");
}

static bool move_path_to_cleanup_destination(const std::string& source_path, const std::string& destination_path)
{
	std::error_code ec;
	std::filesystem::rename(source_path, destination_path, ec);
	if (!ec)
		return true;

	ec.clear();
	if (my_existsdir(source_path.c_str()))
	{
		std::filesystem::copy(source_path, destination_path,
			std::filesystem::copy_options::recursive, ec);
		if (ec)
			return false;
		std::filesystem::remove_all(source_path, ec);
		return !ec;
	}

	std::filesystem::copy_file(source_path, destination_path, std::filesystem::copy_options::none, ec);
	if (ec)
		return false;
	std::filesystem::remove(source_path, ec);
	return !ec;
}

static void refresh_legacy_cleanup_items()
{
	legacy_cleanup_items.clear();
	collect_legacy_settings_cleanup_items();
	collect_legacy_bookmark_cleanup_items();
	collect_legacy_visual_cleanup_items();
}

static void initialize_legacy_cleanup_prompt_state()
{
	legacy_cleanup_prompt_enabled = false;
	refresh_legacy_cleanup_items();

	if (legacy_cleanup_items.empty())
	{
		regdelete(nullptr, LEGACY_CLEANUP_DISMISSED_KEY);
		return;
	}

	if (legacy_migration_state.any_failures())
		return;

	const bool migration_happened_this_run = legacy_migration_state.any_migrated();
	const int cleanup_dismissed = regexists(nullptr, LEGACY_CLEANUP_DISMISSED_KEY);

	if (migration_happened_this_run)
	{
		regdelete(nullptr, LEGACY_CLEANUP_DISMISSED_KEY);
		return;
	}

	if (cleanup_dismissed)
		return;

	legacy_cleanup_prompt_enabled = true;
}

bool should_show_legacy_cleanup_prompt()
{
	return legacy_cleanup_prompt_enabled && !legacy_cleanup_items.empty();
}

std::vector<std::string> get_legacy_cleanup_prompt_items()
{
	std::vector<std::string> items;
	items.reserve(legacy_cleanup_items.size());
	for (const auto& item : legacy_cleanup_items)
		items.emplace_back(item.label + ": " + item.path);
	return items;
}

bool legacy_cleanup_uses_trash()
{
#if defined(AMIBERRY_MACOS)
	return true;
#else
	return false;
#endif
}

bool cleanup_legacy_items(std::vector<std::string>& failed_items)
{
	failed_items.clear();

	const auto destination_root = get_legacy_cleanup_destination_root();
	ensure_directory_exists(destination_root);

	bool moved_any = false;
	for (const auto& item : legacy_cleanup_items)
	{
		if ((!item.is_directory && !my_existsfile2(item.path.c_str()))
			|| (item.is_directory && !my_existsdir(item.path.c_str())))
		{
			continue;
		}

		const auto destination_path = get_unique_cleanup_destination(destination_root,
			std::filesystem::path(item.path));
		if (!move_path_to_cleanup_destination(item.path, destination_path.string()))
		{
			failed_items.emplace_back(item.label + ": " + item.path);
			write_log("Legacy cleanup: failed to move %s to %s\n",
				item.path.c_str(), destination_path.string().c_str());
		}
		else
		{
			moved_any = true;
			write_log("Legacy cleanup: moved %s to %s\n",
				item.path.c_str(), destination_path.string().c_str());
		}
	}

	refresh_legacy_cleanup_items();
	legacy_cleanup_prompt_enabled = !legacy_cleanup_items.empty() && !failed_items.empty();

	if (legacy_cleanup_items.empty())
	{
		regdelete(nullptr, LEGACY_CLEANUP_DISMISSED_KEY);
	}

	return moved_any && failed_items.empty();
}

void postpone_legacy_cleanup_prompt()
{
	legacy_cleanup_prompt_enabled = false;
}

void dismiss_legacy_cleanup_prompt()
{
	legacy_cleanup_prompt_enabled = false;
	regsetint(nullptr, LEGACY_CLEANUP_DISMISSED_KEY, 1);
}

static void update_startup_migration_notice_state()
{
	startup_migration_notice_pending = legacy_migration_state.any_migrated() || legacy_migration_state.any_failures();
}

static void persist_bootstrap_settings_after_migration_if_needed()
{
	if ((!legacy_migration_state.config_migrated && !legacy_migration_state.settings_rewrite_needed)
		|| amiberry_conf_file_overridden_from_cli)
	{
		return;
	}

	save_amiberry_settings();
	write_log("Settings migration: rewrote bootstrap settings file at %s\n", amiberry_conf_file.c_str());
}

bool consume_startup_migration_notice(std::string& title, std::string& message)
{
	if (!startup_migration_notice_pending)
		return false;

	startup_migration_notice_pending = false;

	const auto settings_root = settings_dir.empty() ? get_settings_directory(g_portable_mode) : settings_dir;
	const auto visuals_root = get_current_visual_assets_root_path();
	const auto subject_description = describe_layout_migration_subjects(legacy_migration_state);
	const auto bootstrap_destination_label = get_bootstrap_destination_label(legacy_migration_state);

	if (legacy_migration_state.any_failures())
	{
		title = "Amiberry Migration Review Needed";
		message = "Amiberry could not fully migrate your " + subject_description + " into the new layout.\n\n";

		if (!bootstrap_destination_label.empty())
			message += bootstrap_destination_label + settings_root + "\n\n";
		if (legacy_migration_state.visuals_migrated
			|| legacy_migration_state.visuals_failed
			|| legacy_migration_state.visuals_conflicts)
		{
			message += "Visual asset folders now default to:\n\n  " + visuals_root + "\n\n";
		}
		message += "Existing files were left in place whenever possible.\nPlease check the log file for details.";
		return true;
	}

	title = "Amiberry Migration";
	message = "Amiberry imported your " + subject_description + " into the new layout.\n\n";

	if (!bootstrap_destination_label.empty())
		message += bootstrap_destination_label + settings_root + "\n\n";
	if (legacy_migration_state.visuals_migrated)
		message += "Visual asset folders now default to:\n\n  " + visuals_root + "\n\n";
	message += "Your remaining content folders stay in their existing location.";
	return true;
}

static void resolve_bootstrap_settings_paths(const bool portable_mode, const bool ensure_settings_root)
{
	settings_dir = get_settings_directory(portable_mode);
	if (ensure_settings_root)
		ensure_directory_exists(settings_dir);
	amiberry_ini_file = join_path(settings_dir, "amiberry.ini");
	if (!amiberry_conf_file_overridden_from_cli)
		amiberry_conf_file = join_path(settings_dir, "amiberry.conf");
}

static bool ensure_directory_exists_if_missing(const std::string& directory_path)
{
	if (directory_path.empty() || my_existsdir(directory_path.c_str()))
		return false;

	ensure_directory_exists(directory_path);
	return true;
}

#ifdef AMIBERRY_MACOS
static std::string get_macos_app_resources_directory()
{
	char exepath[MAX_DPATH];
	uint32_t size = sizeof exepath;
	std::string app_directory;
	if (_NSGetExecutablePath(exepath, &size) == 0)
	{
		size_t last_slash_idx = string(exepath).rfind('/');
		if (std::string::npos != last_slash_idx)
		{
			app_directory = string(exepath).substr(0, last_slash_idx);
		}
		last_slash_idx = app_directory.rfind('/');
		if (std::string::npos != last_slash_idx)
		{
			app_directory = app_directory.substr(0, last_slash_idx);
		}
	}
	if (app_directory.empty())
		return {};
	return join_path(app_directory, "Resources");
}
#endif

static std::vector<std::string> get_seed_source_candidates(const std::string& subdirectory,
	const bool include_usr_local_share)
{
	std::vector<std::string> candidates;
#ifdef AMIBERRY_IOS
	const auto app_bundle_dir = get_ios_app_bundle_directory();
	if (!app_bundle_dir.empty())
		append_unique_path_candidate(candidates, join_path(app_bundle_dir, subdirectory));
#elif defined(AMIBERRY_MACOS)
	const auto app_resources_dir = get_macos_app_resources_directory();
	if (!app_resources_dir.empty())
		append_unique_path_candidate(candidates, join_path(app_resources_dir, subdirectory));
#else
	append_unique_path_candidate(candidates, join_path(AMIBERRY_DATADIR, subdirectory));
#if !defined(_WIN32)
	append_unique_path_candidate(candidates, join_path("/usr/share/amiberry", subdirectory));
	if (include_usr_local_share)
		append_unique_path_candidate(candidates, join_path("/usr/local/share/amiberry", subdirectory));
#endif
#endif
	return candidates;
}

static bool copy_missing_directory_contents_if_exists(const std::string& source_dir, const std::string& destination_dir)
{
	if (source_dir.empty() || destination_dir.empty() || !my_existsdir(source_dir.c_str()))
		return false;

	ensure_directory_exists(destination_dir);

	try
	{
		std::filesystem::copy(source_dir, destination_dir,
			std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing);
		return true;
	}
	catch (const std::exception& e)
	{
		write_log("Failed to seed missing files from %s to %s: %s\n",
			source_dir.c_str(), destination_dir.c_str(), e.what());
		return false;
	}
}

static bool seed_missing_directory_contents_from_candidates(const std::string& destination_dir,
	const std::vector<std::string>& source_candidates)
{
	for (const auto& candidate : source_candidates)
	{
		if (copy_missing_directory_contents_if_exists(candidate, destination_dir))
			return true;
	}

	return false;
}

static void ensure_amiberry_user_directories()
{
	ensure_directory_exists_if_missing(config_path);
	if (!plugins_dir.empty())
		ensure_directory_exists_if_missing(plugins_dir);

	ensure_directory_exists_if_missing(controllers_path);
	ensure_directory_exists_if_missing(whdboot_path);
	ensure_directory_exists_if_missing(whdload_arch_path);
	ensure_directory_exists_if_missing(floppy_path);
	ensure_directory_exists_if_missing(harddrive_path);
	ensure_directory_exists_if_missing(cdrom_path);
	ensure_directory_exists_if_missing(rom_path);
	ensure_directory_exists_if_missing(saveimage_dir);
	ensure_directory_exists_if_missing(savestate_dir);
	ensure_directory_exists_if_missing(ripper_path);
	ensure_directory_exists_if_missing(input_dir);
	ensure_directory_exists_if_missing(screenshot_dir);
	ensure_directory_exists_if_missing(nvram_dir);
	ensure_directory_exists_if_missing(video_dir);
	ensure_directory_exists_if_missing(themes_path);
	ensure_directory_exists_if_missing(shaders_path);
	ensure_directory_exists_if_missing(bezels_path);
}

static bool controller_seed_files_missing()
{
	return !file_exists(join_path(controllers_path, "gamecontrollerdb.txt"));
}

static void seed_default_controller_files_if_needed()
{
	if (!controller_seed_files_missing())
		return;

	seed_missing_directory_contents_from_candidates(controllers_path,
		get_seed_source_candidates("controllers", true));
}

static void ensure_whdboot_runtime_directories()
{
	const char* subdirectories[] = {
		"save-data",
		"save-data/Autoboots",
		"save-data/Debugs",
		"save-data/Kickstarts",
		"save-data/Savegames",
		"game-data",
	};

	for (const auto* subdirectory : subdirectories)
		ensure_directory_exists(join_path(whdboot_path, subdirectory));
}

extern std::string get_xml_timestamp(const std::string& xml_filename);
extern std::string get_json_timestamp(const std::string& json_filename);

static bool should_cancel_whdboot_download(std::atomic<bool>* cancel_flag)
{
	return cancel_flag != nullptr && cancel_flag->load();
}

static bool report_whdboot_download_progress(
	const std::function<bool(const whdboot_download_progress&)>& progress_cb,
	std::atomic<bool>* cancel_flag,
	const int step,
	const int total_steps,
	const std::string& label,
	const int64_t bytes_downloaded,
	const int64_t bytes_total)
{
	if (should_cancel_whdboot_download(cancel_flag))
		return true;

	if (!progress_cb)
		return false;

	whdboot_download_progress progress;
	progress.step = step;
	progress.total_steps = total_steps;
	progress.label = label;
	progress.bytes_downloaded = bytes_downloaded;
	progress.bytes_total = bytes_total;
	return progress_cb(progress);
}

whdboot_download_result download_whdboot_assets(
	const std::function<bool(const whdboot_download_progress&)>& progress_cb,
	std::atomic<bool>* cancel_flag)
{
	constexpr int total_steps = 10;
	whdboot_download_result result;
	bool any_success = false;
	bool any_failure = false;

	write_log("Downloading WHDBoot runtime assets into %s\n", whdboot_path.c_str());

	ensure_whdboot_runtime_directories();

	const auto do_download = [&](const int step, const std::string& label, const std::string& url,
		const std::string& destination, const bool keep_backup, const bool record_failure = true) -> bool
	{
		if (report_whdboot_download_progress(progress_cb, cancel_flag, step, total_steps, label, 0, 0))
			return false;

		write_log("Downloading %s ...\n", destination.c_str());
		const auto ok = download_file(url, destination, keep_backup,
			[&](const int64_t dlnow, const int64_t dltotal)
			{
				return report_whdboot_download_progress(progress_cb, cancel_flag, step, total_steps,
					label, dlnow, dltotal);
			},
			cancel_flag);
		if (ok)
			any_success = true;
		else if (record_failure && !should_cancel_whdboot_download(cancel_flag))
			any_failure = true;
		return ok;
	};

	int step = 0;
	auto destination = join_path(whdboot_path, "WHDLoad");
	do_download(++step, "WHDLoad",
		"https://github.com/BlitterStudio/amiberry/blob/master/whdboot/WHDLoad?raw=true",
		destination, false);
	if (should_cancel_whdboot_download(cancel_flag))
	{
		result.outcome = whdboot_download_outcome::cancelled;
		return result;
	}

	destination = join_path(whdboot_path, "JST");
	do_download(++step, "JST",
		"https://github.com/BlitterStudio/amiberry/blob/master/whdboot/JST?raw=true",
		destination, false);
	if (should_cancel_whdboot_download(cancel_flag))
	{
		result.outcome = whdboot_download_outcome::cancelled;
		return result;
	}

	destination = join_path(whdboot_path, "AmiQuit");
	do_download(++step, "AmiQuit",
		"https://github.com/BlitterStudio/amiberry/blob/master/whdboot/AmiQuit?raw=true",
		destination, false);
	if (should_cancel_whdboot_download(cancel_flag))
	{
		result.outcome = whdboot_download_outcome::cancelled;
		return result;
	}

	destination = join_path(whdboot_path, "boot-data.zip");
	do_download(++step, "boot-data.zip",
		"https://github.com/BlitterStudio/amiberry/blob/master/whdboot/boot-data.zip?raw=true",
		destination, false);
	if (should_cancel_whdboot_download(cancel_flag))
	{
		result.outcome = whdboot_download_outcome::cancelled;
		return result;
	}

	const char* rtb_files[] = {
		"kick33180.A500.RTB",
		"kick34005.A500.RTB",
		"kick40063.A600.RTB",
		"kick40068.A1200.RTB",
		"kick40068.A4000.RTB"
	};
	for (const auto& rtb : rtb_files)
	{
		++step;
		const auto rtb_dest_name = std::string("save-data/Kickstarts/") + rtb;
		const auto rtb_destination = join_path(whdboot_path, rtb_dest_name);
		if (file_exists(rtb_destination))
		{
			report_whdboot_download_progress(progress_cb, cancel_flag, step, total_steps,
				std::string(rtb) + " (already exists)", 0, 0);
			if (should_cancel_whdboot_download(cancel_flag))
			{
				result.outcome = whdboot_download_outcome::cancelled;
				return result;
			}
			continue;
		}

		const auto rtb_url = std::string("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/save-data/Kickstarts/")
			+ rtb + "?raw=true";
		do_download(step, rtb, rtb_url, rtb_destination, false);
		if (should_cancel_whdboot_download(cancel_flag))
		{
			result.outcome = whdboot_download_outcome::cancelled;
			return result;
		}
	}

	const auto json_destination = join_path(whdboot_path, "game-data/whdload_db.json");
	const auto xml_destination = join_path(whdboot_path, "game-data/whdload_db.xml");
	const auto old_json_timestamp = get_json_timestamp(json_destination);
	const auto old_xml_timestamp = get_xml_timestamp(xml_destination);
	result.previous_db_timestamp = !old_json_timestamp.empty() ? old_json_timestamp : old_xml_timestamp;

	bool db_ok = do_download(++step, "whdload_db.json",
		"https://raw.githubusercontent.com/BlitterStudio/amiberry-game-db/main/whdload_db.json",
		json_destination, true, false);
	if (!db_ok && !should_cancel_whdboot_download(cancel_flag))
	{
		write_log("WHDBooter - JSON download failed, falling back to XML\n");
		db_ok = do_download(step, "whdload_db.xml",
			"https://github.com/HoraceAndTheSpider/Amiberry-XML-Builder/blob/master/whdload_db.xml?raw=true",
			xml_destination, true, true);
	}

	if (should_cancel_whdboot_download(cancel_flag))
	{
		result.outcome = whdboot_download_outcome::cancelled;
		return result;
	}

	const auto new_json_timestamp = get_json_timestamp(json_destination);
	const auto new_xml_timestamp = get_xml_timestamp(xml_destination);
	result.current_db_timestamp = !new_json_timestamp.empty() ? new_json_timestamp : new_xml_timestamp;

	if (!any_failure)
		result.outcome = whdboot_download_outcome::success;
	else if (any_success)
		result.outcome = whdboot_download_outcome::partial_failure;
	else
		result.outcome = whdboot_download_outcome::failed;

	return result;
}

static bool whdboot_seed_files_missing()
{
	if (!file_exists(join_path(whdboot_path, "WHDLoad")))
		return true;
	if (!file_exists(join_path(whdboot_path, "JST")))
		return true;
	if (!file_exists(join_path(whdboot_path, "AmiQuit")))
		return true;

	const auto boot_data_zip = join_path(whdboot_path, "boot-data.zip");
	const auto boot_data_dir = join_path(whdboot_path, "boot-data");
	if (!file_exists(boot_data_zip) && !my_existsdir(boot_data_dir.c_str()))
		return true;

	const auto json_destination = join_path(whdboot_path, "game-data/whdload_db.json");
	const auto xml_destination = join_path(whdboot_path, "game-data/whdload_db.xml");
	if (!file_exists(json_destination) && !file_exists(xml_destination))
		return true;

	const char* required_rtb_files[] = {
		"kick33180.A500.RTB",
		"kick34005.A500.RTB",
		"kick40063.A600.RTB",
		"kick40068.A1200.RTB",
		"kick40068.A4000.RTB"
	};
	for (const auto* filename : required_rtb_files)
	{
		const auto destination = join_path(whdboot_path, std::string("save-data/Kickstarts/") + filename);
		if (!file_exists(destination))
			return true;
	}

	return false;
}

static void seed_default_whdboot_files_if_needed()
{
	if (!whdboot_seed_files_missing())
		return;

	const auto source_candidates = get_seed_source_candidates("whdboot", true);
	if (!seed_missing_directory_contents_from_candidates(whdboot_path, source_candidates))
	{
		write_log("No WHDLoad boot files found in bundled or system data locations\n");
		write_log("Skipping automatic download during startup. Use the Paths panel or --download-whdboot to fetch them explicitly.\n");
	}
}

static bool rom_seed_files_missing()
{
	if (!file_exists(join_path(rom_path, "aros-ext.bin")))
		return true;
	if (!file_exists(join_path(rom_path, "aros-rom.bin")))
		return true;
	if (!file_exists(join_path(rom_path, "mt32-roms/dir.txt")))
		return true;
	return false;
}

static void seed_default_rom_files_if_needed()
{
	if (!rom_seed_files_missing())
		return;

	seed_missing_directory_contents_from_candidates(rom_path,
		get_seed_source_candidates("roms", true));
}

static void refresh_builtin_theme_presets()
{
	// Always regenerate built-in theme presets so they include any new fields.
	load_default_theme();
	save_theme("Default.theme");
	load_default_dark_theme();
	save_theme("Dark.theme");
}

void create_missing_amiberry_folders()
{
	ensure_amiberry_user_directories();
	seed_default_controller_files_if_needed();
	ensure_whdboot_runtime_directories();
	seed_default_whdboot_files_if_needed();
	seed_default_rom_files_if_needed();
	refresh_builtin_theme_presets();
}

static void append_path_component(std::string& path, const char* component, const bool trailing_separator = true)
{
	if (path.empty() || component == nullptr)
		return;

#ifdef _WIN32
	constexpr char separator = '\\';
#else
	constexpr char separator = '/';
#endif
	if (path.back() != '/' && path.back() != '\\')
		path += separator;
	if (component[0] == '\0')
		return;
	path += component;
	if (trailing_separator)
		path += separator;
}

static void init_amiberry_dirs(const bool portable_mode, const bool materialize_host_roots)
{
#ifdef __MACH__
	const std::string amiberry_dir = "Amiberry";
#else
	const std::string amiberry_dir = "amiberry";
#endif
	config_path = get_config_directory(portable_mode);
	current_dir = home_dir = get_home_directory(portable_mode);
	data_dir = get_data_directory(portable_mode);
	plugins_dir = get_plugins_directory(portable_mode);
	const auto visual_content_root = home_dir;

#ifdef __MACH__
	if constexpr (true)
#elif defined(__ANDROID__)
    if constexpr (true)
#elif defined(_WIN32)
	if constexpr (true)
#else
	if (portable_mode)
#endif
	{
		// These paths are relative to the XDG_DATA_HOME directory
		controllers_path = whdboot_path = saveimage_dir = savestate_dir =
		ripper_path = input_dir = screenshot_dir = nvram_dir = video_dir =
		home_dir;

		whdload_arch_path = floppy_path = harddrive_path =
		cdrom_path = logfile_path = rom_path = rp9_path =
		home_dir;
	}
		else
		{
			std::string xdg_data_home = get_xdg_data_home();
			if (materialize_host_roots && !my_existsdir(xdg_data_home.c_str()))
			{
				// Create the XDG_DATA_HOME directory if it doesn't exist
				const auto user_home_dir = getenv("HOME");
			if (user_home_dir != nullptr)
			{
				std::string destination = std::string(user_home_dir) + "/.local";
				my_mkdir(destination.c_str());
				destination += "/share";
				my_mkdir(destination.c_str());
				}
			}
			xdg_data_home += "/" + amiberry_dir;
			if (materialize_host_roots && !my_existsdir(xdg_data_home.c_str()))
				my_mkdir(xdg_data_home.c_str());

			std::string xdg_config_home = get_xdg_config_home();
			if (materialize_host_roots && !my_existsdir(xdg_config_home.c_str()))
				my_mkdir(xdg_config_home.c_str());
			xdg_config_home += "/" + amiberry_dir;
			if (materialize_host_roots && !my_existsdir(xdg_config_home.c_str()))
				my_mkdir(xdg_config_home.c_str());

		// These paths are relative to the XDG_DATA_HOME directory
		controllers_path = whdboot_path = saveimage_dir = 
		ripper_path = input_dir = nvram_dir = video_dir =
		xdg_data_home;

		// These go in $HOME/Amiberry by default
		whdload_arch_path = floppy_path = harddrive_path = screenshot_dir =
		savestate_dir = cdrom_path = logfile_path = rom_path = rp9_path =
			home_dir;
	}

	append_path_component(controllers_path, get_controllers_directory_name());
	append_path_component(whdboot_path, get_whdboot_directory_name());
	append_path_component(whdload_arch_path, get_whdload_archives_directory_name());
	append_path_component(floppy_path, get_floppies_directory_name());
	append_path_component(harddrive_path, get_harddrives_directory_name());
	append_path_component(cdrom_path, get_cdroms_directory_name());
	append_path_component(logfile_path, get_logfile_name(), false);
	append_path_component(rom_path, get_roms_directory_name());
	append_path_component(rp9_path, get_rp9_directory_name());
	append_path_component(saveimage_dir, "");
	append_path_component(savestate_dir, get_savestates_directory_name());
	append_path_component(ripper_path, get_ripper_directory_name());
	append_path_component(input_dir, get_inputrecordings_directory_name());
	append_path_component(screenshot_dir, get_screenshots_directory_name());
	append_path_component(nvram_dir, get_nvram_directory_name());
	append_path_component(video_dir, get_videos_directory_name());

	apply_current_visual_asset_paths(get_visual_asset_paths_from_content_root(visual_content_root));

	retroarch_file = config_path;
#ifdef _WIN32
	retroarch_file.append("\\retroarch.cfg");
#elif defined(__ANDROID__)
	retroarch_file.append("retroarch.cfg");
#else
	retroarch_file.append("/retroarch.cfg");
#endif

	floppy_sounds_dir = data_dir;
#ifdef _WIN32
	floppy_sounds_dir.append("floppy_sounds\\");
#else
	floppy_sounds_dir.append("floppy_sounds/");
#endif

	default_base_content_paths = get_current_base_content_path_set();
}

static void dump_resolved_paths(const bool write_dump_file)
{
	std::string output;
	const auto append_line = [&](const char* key, const std::string& value)
	{
		output.append(key).append("=").append(value).append("\n");
	};

	output.append("portable_mode=").append(g_portable_mode ? "1" : "0").append("\n");
	append_line("portable_root", get_portable_root_directory());
	append_line("portable_marker_file", get_portable_mode_marker_path());
	append_line("settings_resolution_source", get_settings_resolution_source_name(resolved_settings_source));
	append_line("settings_resolution_file", resolved_settings_file);
	append_line("settings_dir", settings_dir);
	append_line("amiberry_conf_file", amiberry_conf_file);
	append_line("amiberry_ini_file", amiberry_ini_file);
	append_line("base_content_path", base_content_path);
	append_line("home_dir", home_dir);
	append_line("config_path", config_path);
	append_line("data_dir", data_dir);
	append_line("plugins_dir", plugins_dir);
	append_line("controllers_path", controllers_path);
	append_line("whdboot_path", whdboot_path);
	append_line("whdload_arch_path", whdload_arch_path);
	append_line("floppy_path", floppy_path);
	append_line("harddrive_path", harddrive_path);
	append_line("cdrom_path", cdrom_path);
	append_line("logfile_path", logfile_path);
	append_line("rom_path", rom_path);
	append_line("rp9_path", rp9_path);
	append_line("saveimage_dir", saveimage_dir);
	append_line("savestate_dir", savestate_dir);
	append_line("ripper_path", ripper_path);
	append_line("input_dir", input_dir);
	append_line("screenshot_dir", screenshot_dir);
	append_line("nvram_dir", nvram_dir);
	append_line("video_dir", video_dir);
	append_line("themes_path", themes_path);
	append_line("shaders_path", shaders_path);
	append_line("bezels_path", bezels_path);

	fputs(output.c_str(), stdout);
	fflush(stdout);

	if (write_dump_file)
	{
		const auto dump_file = join_path(settings_dir, "resolved-paths.txt");
		if (!dump_file.empty())
		{
			if (auto* dump_handle = fopen(dump_file.c_str(), "w"))
			{
				fwrite(output.data(), 1, output.size(), dump_handle);
				fclose(dump_handle);
			}
		}
	}
}

void reset_default_paths()
{
	const auto previous_logfile_path = logfile_path;
	base_content_path.clear();
	init_amiberry_dirs(g_portable_mode);
	create_missing_amiberry_folders();
	if (!path_strings_match(previous_logfile_path, logfile_path))
		reopen_active_logfile_if_needed();
}

static void load_amiberry_settings_from_file(const std::string& settings_file)
{
	auto* const fh = zfile_fopen(settings_file.c_str(), _T("r"), ZFD_NORMAL);
	if (fh)
	{
		char linea[CONFIG_BLEN];
		std::vector<std::string> settings_lines;
		std::vector<managed_path_option_line> managed_path_lines;
		std::string configured_base_path;
		std::string serialized_base_path_to_skip;
		bool has_base_content_path = false;

		while (zfile_fgetsa(linea, sizeof linea, fh) != nullptr)
		{
			trim_wsa(linea);
			if (strlen(linea) > 0)
				settings_lines.emplace_back(linea);
		}
		zfile_fclose(fh);

		for (const auto& line : settings_lines)
		{
			char line_copy[CONFIG_BLEN];
			managed_path_option_line managed_path_line;
			strncpy(line_copy, line.c_str(), CONFIG_BLEN - 1);
			line_copy[CONFIG_BLEN - 1] = '\0';

			if (parse_base_content_path_line(settings_file.c_str(), line_copy, configured_base_path))
				has_base_content_path = true;

			strncpy(line_copy, line.c_str(), CONFIG_BLEN - 1);
			line_copy[CONFIG_BLEN - 1] = '\0';
			if (parse_managed_path_option_line(settings_file.c_str(), line_copy, managed_path_line))
				managed_path_lines.emplace_back(managed_path_line);
		}

		const auto inferred_serialized_base_path = infer_serialized_base_content_path(managed_path_lines);
		std::vector<visual_asset_path_set> legacy_visual_candidate_paths;
		legacy_visual_candidate_paths.emplace_back(get_legacy_default_visual_asset_paths(g_portable_mode));
		if (has_base_content_path && !configured_base_path.empty())
			legacy_visual_candidate_paths.emplace_back(
				get_visual_asset_path_set(get_legacy_base_content_path_set(configured_base_path)));
		if (!inferred_serialized_base_path.empty())
			legacy_visual_candidate_paths.emplace_back(
				get_visual_asset_path_set(get_legacy_base_content_path_set(inferred_serialized_base_path)));

		if (has_base_content_path
			&& !configured_base_path.empty()
			&& !inferred_serialized_base_path.empty()
			&& !path_strings_match(inferred_serialized_base_path, configured_base_path))
		{
			serialized_base_path_to_skip = inferred_serialized_base_path;
		}

		if (has_base_content_path)
			set_base_content_path(configured_base_path);
		else if (!inferred_serialized_base_path.empty())
		{
			const auto legacy_inferred_visual_paths =
				get_visual_asset_path_set(get_legacy_base_content_path_set(inferred_serialized_base_path));
			if (any_managed_path_line_matches_visual_paths(managed_path_lines, legacy_inferred_visual_paths))
				apply_current_visual_asset_paths(
					get_visual_asset_paths_from_content_root(inferred_serialized_base_path));
		}

		for (const auto& line : settings_lines)
		{
			char line_copy[CONFIG_BLEN];
			char managed_line_copy[CONFIG_BLEN];
			managed_path_option_line managed_path_line;
			std::string ignored;

			strncpy(line_copy, line.c_str(), CONFIG_BLEN - 1);
			line_copy[CONFIG_BLEN - 1] = '\0';

			if (parse_base_content_path_line(settings_file.c_str(), line_copy, ignored))
				continue;

			if (!serialized_base_path_to_skip.empty())
			{
				strncpy(managed_line_copy, line.c_str(), CONFIG_BLEN - 1);
				managed_line_copy[CONFIG_BLEN - 1] = '\0';
				if (parse_managed_path_option_line(settings_file.c_str(), managed_line_copy, managed_path_line))
				{
					const auto serialized_base_paths = get_base_content_path_set(serialized_base_path_to_skip);
					if (path_strings_match(managed_path_line.value,
						serialized_base_paths.*(managed_path_line.descriptor->member)))
					{
						continue;
					}
				}
			}

			strncpy(managed_line_copy, line.c_str(), CONFIG_BLEN - 1);
			managed_line_copy[CONFIG_BLEN - 1] = '\0';
			if (parse_managed_path_option_line(settings_file.c_str(), managed_line_copy, managed_path_line)
				&& is_visual_managed_path_option(managed_path_line.descriptor))
			{
				bool skip_legacy_visual_line = false;
				for (const auto& legacy_visual_paths : legacy_visual_candidate_paths)
				{
					if (managed_path_line_matches_visual_paths(managed_path_line, legacy_visual_paths))
					{
						skip_legacy_visual_line = true;
						break;
					}
				}
				if (skip_legacy_visual_line)
					continue;
			}

			parse_amiberry_settings_line(settings_file.c_str(), line_copy);
		}
	}
}

void load_amiberry_settings()
{
	load_amiberry_settings_from_file(amiberry_conf_file);
}

static void romlist_add2(const TCHAR* path, romdata* rd)
{
	if (getregmode()) {
		int ok = 0;
		if (path[0] == '/' || path[0] == '\\')
			ok = 1;
		if (_tcslen(path) > 1 && path[1] == ':')
			ok = 1;
		if (!ok) {
			TCHAR tmp[MAX_DPATH];
			_tcscpy(tmp, get_rom_path().c_str());
			_tcscat(tmp, path);
			romlist_add(tmp, rd);
			return;
		}
	}
	romlist_add(path, rd);
}

void read_rom_list(bool initial)
{
	int size, size2;

	romlist_clear();
	const int exists = regexiststree(nullptr, _T("DetectedROMs"));
	UAEREG* fkey = regcreatetree(nullptr, _T("DetectedROMs"));
	if (fkey == nullptr)
		return;
	if (!exists || forceroms) {
		//if (initial) {
		//	scaleresource_init(NULL, 0);
		//}
		load_keyring(nullptr, nullptr);
		scan_roms(forceroms ? 0 : 1);
	}
	forceroms = 0;
	int idx = 0;
	for (;;) {
		TCHAR tmp[1000];
		TCHAR tmp2[1000];
		size = sizeof(tmp) / sizeof(TCHAR);
		size2 = sizeof(tmp2) / sizeof(TCHAR);
		if (!regenumstr(fkey, idx, tmp, &size, tmp2, &size2))
			break;
		if (_tcslen(tmp) == 7 || _tcslen(tmp) == 13) {
			int group = 0;
			int subitem = 0;
			const int idx2 = _tstol(tmp + 4);
			if (_tcslen(tmp) == 13) {
				group = _tstol(tmp + 8);
				subitem = _tstol(tmp + 11);
			}
			if (idx2 >= 0 && _tcslen(tmp2) > 0) {
				romdata* rd = getromdatabyidgroup(idx2, group, subitem);
				if (rd) {
					TCHAR* s = _tcschr(tmp2, '\"');
					if (s && _tcslen(s) > 1) {
						TCHAR* s2 = my_strdup(s + 1);
						s = _tcschr(s2, '\"');
						if (s)
							*s = 0;
						romlist_add2(s2, rd);
						xfree(s2);
					}
					else {
						romlist_add2(tmp2, rd);
					}
				}
			}
		}
		idx++;
	}
	romlist_add(nullptr, nullptr);
	regclosetree(fkey);
}

void target_getdate(int* y, int* m, int* d)
{
	*y = GETBDY(AMIBERRYDATE);
	*m = GETBDM(AMIBERRYDATE);
	*d = GETBDD(AMIBERRYDATE);
}

void target_addtorecent(const TCHAR* name, int t)
{
	add_file_to_mru_list(lstMRUDiskList, std::string(name));
}

void target_reset()
{
	clipboard_reset();
}

bool target_can_autoswitchdevice()
{
	if (mouseactive <= 0)
		return false;
	return true;
}

uae_u32 emulib_target_getcpurate(const uae_u32 v, uae_u32* low)
{
	*low = 0;
	if (v == 1)
	{
		*low = 1e+9; /* We have nanoseconds */
		return 0;
	}
	if (v == 2)
	{
		timespec ts{};
		clock_gettime(CLOCK_MONOTONIC, &ts);
		const auto time = static_cast<int64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
		*low = static_cast<uae_u32>(time & 0xffffffff);
		return static_cast<uae_u32>(time >> 32);
	}
	return 0;
}

void target_shutdown()
{
#ifndef AMIBERRY_IOS
	system("sudo poweroff");
#endif
}

struct winuae	//this struct is put in a6 if you call
	//execute native function
{
	HWND amigawnd;    //address of amiga Window Windows Handle
	unsigned int changenum;   //number to detect screen close/open
	unsigned int z3offset;    //the offset to add to access Z3 mem from Dll side
};

void* uaenative_get_uaevar()
{
	static winuae uaevar;
#if defined(_WIN32) && !defined(AMIBERRY)
	uaevar.amigawnd = mon->hAmigaWnd;
#endif
	// WARNING: not 64-bit safe!
	uaevar.z3offset = static_cast<uae_u32>(reinterpret_cast<uae_u64>(get_real_address(z3fastmem_bank[0].start))) - z3fastmem_bank[0].start;
	return &uaevar;
}

const TCHAR** uaenative_get_library_dirs()
{
	static const TCHAR** nats;
	static TCHAR* path;
	static TCHAR* libpath;
	
	if (nats == nullptr)
		nats = xcalloc(const TCHAR*, 4);
	if (path == nullptr) {
		path = xcalloc(TCHAR, MAX_DPATH);
		_tcscpy(path, plugins_dir.c_str());
	}
	if (libpath == nullptr)
	{
		libpath = strdup(_T(AMIBERRY_LIBDIR));
	}
	nats[0] = home_dir.c_str();
	nats[1] = path;
	nats[2] = libpath; 
	return nats;
}

static int parseversion(TCHAR** vs)
{
	TCHAR tmp[10];
	int i;

	i = 0;
	while (**vs >= '0' && **vs <= '9') {
		if (i >= sizeof(tmp) / sizeof(TCHAR))
			return 0;
		tmp[i++] = **vs;
		(*vs)++;
	}
	if (**vs == '.')
		(*vs)++;
	tmp[i] = 0;
	return _tstol(tmp);
}

static int checkversion(TCHAR* vs, int* verp)
{
	int ver;
	if (_tcslen(vs) < 10)
		return 0;
	if (_tcsncmp(vs, _T("Amiberry "), 256))
		return 0;
	vs += 7;
	ver = parseversion(&vs) << 16;
	ver |= parseversion(&vs) << 8;
	ver |= parseversion(&vs);
	if (verp)
		*verp = ver;
	if (ver >= ((UAEMAJOR << 16) | (UAEMINOR << 8) | UAESUBREV))
		return 0;
	return 1;
}

static void initialize_ini()
{
	regsetstr(nullptr, _T("Version"), VersionStr);

	if (!regexists(nullptr, _T("MainPosX")) || !regexists(nullptr, _T("GUIPosX"))) {
		const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
		int x = dm ? dm->w : 1920;
		int y = dm ? dm->h : 1080;
		const int dpi = getdpiformonitor(SDL_GetPrimaryDisplay());
		x = (x - (GUI_WIDTH * dpi / 96)) / 2;
		y = (y - (GUI_HEIGHT * dpi / 96)) / 2;
		x = std::max(x, 10);
		y = std::max(y, 10);
		/* Create and initialize all our sub-keys to the default values */
		regsetint(nullptr, _T("MainPosX"), x);
		regsetint(nullptr, _T("MainPosY"), y);
		regsetint(nullptr, _T("GUIPosX"), x);
		regsetint(nullptr, _T("GUIPosY"), y);
	}

	regqueryint(NULL, _T("KeySwapBackslashF11"), &key_swap_hack);
	regqueryint(NULL, _T("KeyEndPageUp"), &key_swap_end_pgup);

	read_rom_list(true);
	load_keyring(nullptr, nullptr);
}

static void makeverstr(TCHAR* s)
{
	if (_tcslen(AMIBERRYBETA) > 0) {
		if (AMIBERRYPUBLICBETA == 2) {
			_sntprintf(BetaStr, sizeof BetaStr, _T(" (DevAlpha %s, %d.%02d.%02d)"), AMIBERRYBETA,
				GETBDY(AMIBERRYDATE), GETBDM(AMIBERRYDATE), GETBDD(AMIBERRYDATE));
		}
		else {
			_sntprintf(BetaStr, sizeof BetaStr, _T(" (%sBeta %s, %d.%02d.%02d)"), AMIBERRYPUBLICBETA > 0 ? _T("Public ") : _T(""), AMIBERRYBETA,
				GETBDY(AMIBERRYDATE), GETBDM(AMIBERRYDATE), GETBDD(AMIBERRYDATE));
		}
#ifdef _WIN64
		_tcscat(BetaStr, _T(" 64-bit"));
#endif
		_sntprintf(s, sizeof VersionStr, _T("Amiberry %d.%d.%d%s%s"),
			UAEMAJOR, UAEMINOR, UAESUBREV, AMIBERRYREV, BetaStr);
	}
	else {
		_sntprintf(s, sizeof VersionStr, _T("Amiberry %d.%d.%d%s (%d.%02d.%02d)"),
			UAEMAJOR, UAEMINOR, UAESUBREV, AMIBERRYREV, GETBDY(AMIBERRYDATE), GETBDM(AMIBERRYDATE), GETBDD(AMIBERRYDATE));
#ifdef _WIN64
		_tcscat(s, _T(" 64-bit"));
#endif
	}
	if (_tcslen(AMIBERRYEXTRA) > 0) {
		_tcscat(s, _T(" "));
		_tcscat(s, AMIBERRYEXTRA);
	}
}

int amiberry_main(int argc, char* argv[])
{
#ifdef __ANDROID__
	if (!SDL_Init(0)) {
		write_log("SDL_Init(0) failed: %s\n", SDL_GetError());
	}
#endif
	settings_dir.clear();
	amiberry_conf_file.clear();
	amiberry_ini_file.clear();
	resolved_settings_file.clear();
	amiberry_conf_file_overridden_from_cli = false;
	suppress_runtime_path_side_effects = false;
	legacy_migration_state = {};
	startup_migration_notice_pending = false;
	resolved_settings_source = settings_resolution_source::default_paths_only;
	max_uae_width = 8192;
	max_uae_height = 8192;

	makeverstr(VersionStr);

	bool run_jit_selftest = false;
	bool dump_paths = false;
	bool download_whdboot = false;
	for (auto i = 1; i < argc; i++) {
		if (_tcscmp(argv[i], _T("-h")) == 0 || _tcscmp(argv[i], _T("--help")) == 0)
			usage();
		if (_tcscmp(argv[i], _T("--version")) == 0)
			print_version();
		if (_tcscmp(argv[i], _T("--log")) == 0)
			console_logging = 1;
		if (_tcscmp(argv[i], _T("--jit-selftest")) == 0)
			run_jit_selftest = true;
		if (_tcscmp(argv[i], _T("--dump-paths")) == 0)
			dump_paths = true;
		if (_tcscmp(argv[i], _T("--download-whdboot")) == 0)
			download_whdboot = true;
		if (_tcscmp(argv[i], _T("--rescan-roms")) == 0)
			forceroms = 1;
	}

	if (run_jit_selftest)
		return run_jit_selftest_cli();

#ifndef _WIN32
	struct sigaction action{};
#endif
	mainthreadid = uae_thread_get_id(nullptr);



#ifdef USE_DBUS
	if (!dump_paths)
		DBusSetup();
#endif
#ifdef USE_IPC_SOCKET
	if (!dump_paths)
		Amiberry::IPC::IPCSetup();
#endif

	suppress_runtime_path_side_effects = dump_paths;

	// Parse the command line to possibly set amiberry_config.
	// Do not remove used args yet.
	if (!parse_amiberry_cmd_line(&argc, argv, false))
	{
		printf("Error in Amiberry command line option parsing.\n");
		usage();
		abort();
	}

	// Portable mode is enabled by an amiberry.portable marker next to the executable.
	// This keeps portable installs stable even when shortcuts or shell launches use
	// a different working directory.
	g_portable_mode = is_portable_mode_enabled();
#if defined(__MACH__) || defined(__ANDROID__)
	if (g_portable_mode)
		write_log("Portable mode marker ignored on this platform.\n");
	g_portable_mode = false;
	#endif
	const bool portable_mode = g_portable_mode;
	resolve_bootstrap_settings_paths(portable_mode, !dump_paths);
	if (dump_paths)
	{
		init_amiberry_dirs(portable_mode, false);
		resolved_settings_source = amiberry_conf_file_overridden_from_cli
			? settings_resolution_source::cli_override
			: settings_resolution_source::default_paths_only;
		const auto settings_file_for_resolution = get_existing_settings_file_for_resolution(portable_mode);
		if (!settings_file_for_resolution.empty())
		{
			resolved_settings_file = normalize_path_string(settings_file_for_resolution);
			if (!amiberry_conf_file_overridden_from_cli)
			{
				resolved_settings_source = path_strings_match(settings_file_for_resolution, amiberry_conf_file)
					? settings_resolution_source::settings_dir
					: settings_resolution_source::legacy_settings;
			}
			load_amiberry_settings_from_file(settings_file_for_resolution);
		}
		dump_resolved_paths(false);
		return 0;
	}

	if (!amiberry_conf_file_overridden_from_cli)
		migrate_legacy_settings_files(portable_mode);
	const bool config_found = my_existsfile2(amiberry_conf_file.c_str());

	init_amiberry_dirs(portable_mode);
	if (config_found)
		load_amiberry_settings();
	migrate_legacy_configuration_directories(portable_mode);
	migrate_legacy_visual_asset_directories();
	migrate_legacy_lowercase_content_directories();
	macos_bookmarks_init(settings_dir, get_legacy_bookmark_candidate_directories(portable_mode));
	create_missing_amiberry_folders();
	int whdboot_download_exit_code = 0;
	if (download_whdboot)
	{
		const auto download_result = download_whdboot_assets({}, nullptr);
		switch (download_result.outcome)
		{
		case whdboot_download_outcome::success:
			whdboot_download_exit_code = 0;
			break;
		case whdboot_download_outcome::partial_failure:
			whdboot_download_exit_code = 2;
			break;
		case whdboot_download_outcome::cancelled:
			whdboot_download_exit_code = 130;
			break;
		case whdboot_download_outcome::failed:
		default:
			whdboot_download_exit_code = 1;
			break;
		}
	}
	persist_bootstrap_settings_after_migration_if_needed();
	update_startup_migration_notice_state();
	if (download_whdboot)
		return whdboot_download_exit_code;

	uae_time_init();

	// Parse the command line, remove used amiberry specific args
	// and modify both argc & argv accordingly
	if (!parse_amiberry_cmd_line(&argc, argv, true))
	{
		printf("Error in Amiberry command line option parsing.\n");
		usage();
		abort();
	}

	ensure_parent_directory_exists(amiberry_ini_file);
	reginitializeinit(&inipath);
	if (getregmode() == nullptr)
	{
		const std::string ini_file_path = get_ini_file_path();
		TCHAR* path = my_strdup(ini_file_path.c_str());;
		auto f = fopen(path, _T("r"));
		if (!f)
			f = fopen(path, _T("w"));
		if (f)
		{
			fclose(f);
			reginitializeinit(&path);
		}
		xfree(path);
	}

	logging_init();
#if defined (CPU_arm) && !defined (_WIN32)
	memset(&action, 0, sizeof action);
	action.sa_sigaction = signal_segv;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGSEGV, &action, nullptr) < 0)
	{
		write_log("Failed to set signal handler (SIGSEGV).\n");
#ifndef __ANDROID__
		abort();
#endif
	}
	if (sigaction(SIGILL, &action, nullptr) < 0)
	{
		write_log("Failed to set signal handler (SIGILL).\n");
#ifndef __ANDROID__
		abort();
#endif
	}

	memset(&action, 0, sizeof action);
	action.sa_sigaction = signal_buserror;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGBUS, &action, nullptr) < 0)
	{
		write_log("Failed to set signal handler (SIGBUS).\n");
#ifndef __ANDROID__
		abort();
#endif
	}

	memset(&action, 0, sizeof action);
	action.sa_sigaction = signal_term;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGTERM, &action, nullptr) < 0)
	{
		write_log("Failed to set signal handler (SIGTERM).\n");
#ifndef __ANDROID__
		abort();
#endif
	}
#endif

	preinit_shm ();

	if (!init_mmtimer())
		return 0;
	uae_time_calibrate();
	
	if (!osdep_platform_init_sdl())
		abort();

	initialize_ini();
	initialize_legacy_cleanup_prompt_state();
	write_log(_T("Enumerating display devices.. \n"));
	enumeratedisplays();
	write_log(_T("Sorting devices and modes...\n"));
	sortdisplays();
	enumerate_sound_devices();
	for (int i = 0; i < MAX_SOUND_DEVICES && sound_devices[i]; i++) {
		const int type = sound_devices[i]->type;
		write_log(_T("%d:%s: %s\n"), i, type == SOUND_DEVICE_SDL2 ? _T("SDL") : (type == SOUND_DEVICE_DS ? _T("DS") : (type == SOUND_DEVICE_AL ? _T("AL") : (type == SOUND_DEVICE_WASAPI ? _T("WA") : (type == SOUND_DEVICE_WASAPI_EXCLUSIVE ? _T("WX") : _T("PA"))))), sound_devices[i]->name);
	}
	write_log(_T("Enumerating recording devices:\n"));
	for (int i = 0; i < MAX_SOUND_DEVICES && record_devices[i]; i++) {
		const int type = record_devices[i]->type;
		write_log(_T("%d:%s: %s\n"), i, type == SOUND_DEVICE_SDL2 ? _T("SDL") : (type == SOUND_DEVICE_DS ? _T("DS") : (type == SOUND_DEVICE_AL ? _T("AL") : (type == SOUND_DEVICE_WASAPI ? _T("WA") : (type == SOUND_DEVICE_WASAPI_EXCLUSIVE ? _T("WX") : _T("PA"))))), record_devices[i]->name);
	}
	write_log(_T("Enumeration done\n"));

	osdep_platform_init_ui();
	keyboard_settrans();

	open_led_console();
	osdep_platform_sync_keyboard_leds();

#ifdef USE_GPIOD
#if defined(GPIOD_VERSION_MAJOR) && GPIOD_VERSION_MAJOR >= 2
	// Open GPIO chip and request output lines (v2 API)
	chip = gpiod_chip_open("/dev/gpiochip0");
	if (chip) {
		struct gpiod_line_settings* settings = gpiod_line_settings_new();
		gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
		gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

		struct gpiod_line_config* line_cfg = gpiod_line_config_new();
		unsigned int offsets[] = {GPIO_LINE_RED, GPIO_LINE_YELLOW, GPIO_LINE_GREEN};
		gpiod_line_config_add_line_settings(line_cfg, offsets, 3, settings);

		struct gpiod_request_config* req_cfg = gpiod_request_config_new();
		gpiod_request_config_set_consumer(req_cfg, "amiberry");

		gpio_request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

		gpiod_request_config_free(req_cfg);
		gpiod_line_config_free(line_cfg);
		gpiod_line_settings_free(settings);
	}
#else
	// Open GPIO chip (v1 API)
	chip = gpiod_chip_open_by_name(chipname);

	// Open GPIO lines
	lineRed = gpiod_chip_get_line(chip, 18);
	lineGreen = gpiod_chip_get_line(chip, 24);
	lineYellow = gpiod_chip_get_line(chip, 23);

	// Open LED lines for output
	gpiod_line_request_output(lineRed, "amiberry", 0);
	gpiod_line_request_output(lineGreen, "amiberry", 0);
	gpiod_line_request_output(lineYellow, "amiberry", 0);
#endif
#endif
#ifdef CATWEASEL
	catweasel_init ();
#endif
#ifdef PARALLEL_DIRECT
	paraport_mask = paraport_init ();
#endif
#ifdef SERIAL_PORT
	shmem_serial_create();
	enumserialports();
#endif
	enummidiports();

	osdep_platform_call_real_main(argc, argv);

#ifdef USE_GPIOD
#if defined(GPIOD_VERSION_MAJOR) && GPIOD_VERSION_MAJOR >= 2
	if (gpio_request)
		gpiod_line_request_release(gpio_request);
#else
	gpiod_line_release(lineRed);
	gpiod_line_release(lineGreen);
	gpiod_line_release(lineYellow);
#endif
	if (chip)
		gpiod_chip_close(chip);
#endif
	// restore keyboard LEDs to normal state and release console fd
	close_led_console();

#ifdef SERIAL_PORT
	shmem_serial_delete();
#endif
#ifdef AHI
	ahi_close_sound();
#endif
#ifdef PARALLEL_DIRECT
	paraport_free();
	closeprinter();
#endif
#ifdef RETROPLATFORM
	rp_free();
#endif
	close_console();
	regclosetree(nullptr);

	romlist_clear();
	free_keyring();

	logging_cleanup();

	osdep_platform_shutdown_sdl();

	if (host_poweroff)
		target_shutdown();
	return 0;
}

void toggle_mousegrab()
{
	activationtoggle(0, false);
}

bool get_plugin_path(TCHAR* out, const int len, const TCHAR* path)
{
	if (strcmp(path, "floppysounds") == 0) {
		if (floppy_sounds_dir[0]) {
			strncpy(out, floppy_sounds_dir.c_str(), len);
		}
		else {
			strncpy(out, "floppy_sounds", len);
		}
		// make sure out is null-terminated in any case
		out[len - 1] = '\0';
	}
	else if (strcmp(path, "abr") == 0)
	{
		strncpy(out, data_dir.c_str(), len - 1);
		strncat(out, "abr/", len - 1);
		out[len - 1] = '\0';
	}
	else {
		strncpy(out, plugins_dir.c_str(), len - 1);
		strncat(out, "/", len - 1);
		strncat(out, path, len - 1);
		strncat(out, "/", len - 1);
		return my_existsfile2(out);
	}
	return TRUE;
}

// The serialization logic here is taken from FloppyBridge.cpp -> void BridgeConfig::toString(char** serialisedOptions)
void drawbridge_update_profiles(uae_prefs* p)
{
#ifdef FLOPPYBRIDGE
	// sanity check
	if (p->drawbridge_drive_cable < 0 || p->drawbridge_drive_cable > 5)
		p->drawbridge_drive_cable = 0;

	const unsigned int flags = (p->drawbridge_autocache ? 1 : 0) | (p->drawbridge_drive_cable & 1) << 1 | (p->drawbridge_drive_cable & 6) << 3 | (p->drawbridge_serial_auto ? 4 : 0) | (p->drawbridge_smartspeed ? 8 : 0);

	const std::string profile_name_normal = "Normal";
	const std::string profile_name_comp = "Compatible";
	const std::string profile_name_turbo = "Turbo";
	const std::string profile_name_stalling = "Stalling";

	const std::string bridge_mode_normal = "0";
	const std::string bridge_mode_comp = "1";
	const std::string bridge_mode_turbo = "2";
	const std::string bridge_mode_stalling = "3";

	const std::string bridge_density_auto = "0";

	std::string serial_port = p->drawbridge_serial_port;
	if (serial_port.empty())
		serial_port = "/dev/ttyUSB0";

	// profileID | profileName [ drawbridge_driver | flags | serial_port | bridge_mode | bridge_density ]
	std::string tmp;
	// Normal
	tmp = std::string("1") + "|" + profile_name_normal + "[" + std::to_string(p->drawbridge_driver) + "|" + std::to_string(flags) + "|" + serial_port + "|" + bridge_mode_normal + "|" + bridge_density_auto + "]";
	// Compatible
	tmp += std::string("2") + "|" + profile_name_comp + "[" + std::to_string(p->drawbridge_driver) + "|" + std::to_string(flags) + "|" + serial_port + "|" + bridge_mode_comp + "|" + bridge_density_auto + "]";
	// Turbo
	tmp += std::string("3") + "|" + profile_name_turbo + "[" + std::to_string(p->drawbridge_driver) + "|" + std::to_string(flags) + "|" + serial_port + "|" + bridge_mode_turbo + "|" + bridge_density_auto + "]";
	// Stalling
	tmp += std::string("4") + "|" + profile_name_stalling + "[" + std::to_string(p->drawbridge_driver) + "|" + std::to_string(flags) + "|" + serial_port + "|" + bridge_mode_stalling + "|" + bridge_density_auto + "]";

	floppybridge_set_config(tmp.c_str());
	if (quit_program != UAE_QUIT)
		floppybridge_init(p);
#endif
}

bool is_mainthread()
{
	return uae_thread_get_id(nullptr) == mainthreadid;
}

void clear_whdload_prefs()
{
	whdload_prefs.filename.clear();
	whdload_prefs.game_name.clear();
	whdload_prefs.sub_path.clear();
	whdload_prefs.variant_uuid.clear();
	whdload_prefs.slave_count = 0;
	whdload_prefs.slave_default.clear();
	whdload_prefs.slave_libraries = false;
	whdload_prefs.slaves = {};
	whdload_prefs.selected_slave = {};
	whdload_prefs.custom.clear();
}

void save_controller_mapping_to_file(const controller_mapping& input, const std::string& filename)
{
	FILE* out_file = fopen(filename.c_str(), "wb");
	if (!out_file) {
		std::cerr << "Unable to open file " << filename << '\n';
		return;
	}

	fputs("button=", out_file);
	for (const auto& b : input.button) {
		fprintf(out_file, "%d ", b);
	}
	fputs("\naxis=", out_file);
	for (const auto& a : input.axis) {
		fprintf(out_file, "%d ", a);
	}
	fprintf(out_file, "\nlstick_axis_y_invert=%d", input.lstick_axis_y_invert);
	fprintf(out_file, "\nlstick_axis_x_invert=%d", input.lstick_axis_x_invert);
	fprintf(out_file, "\nrstick_axis_y_invert=%d", input.rstick_axis_y_invert);
	fprintf(out_file, "\nrstick_axis_x_invert=%d", input.rstick_axis_x_invert);
	fprintf(out_file, "\nhotkey_button=%d", input.hotkey_button);
	fprintf(out_file, "\nquit_button=%d", input.quit_button);
	fprintf(out_file, "\nreset_button=%d", input.reset_button);
	fprintf(out_file, "\nmenu_button=%d", input.menu_button);
	fprintf(out_file, "\nload_state_button=%d", input.load_state_button);
	fprintf(out_file, "\nsave_state_button=%d", input.save_state_button);
	fprintf(out_file, "\nvkbd_button=%d", input.vkbd_button);
	fprintf(out_file, "\nnumber_of_hats=%d", input.number_of_hats);
	fprintf(out_file, "\nnumber_of_axis=%d", input.number_of_axis);
	fprintf(out_file, "\nis_retroarch=%d", input.is_retroarch);
	fputs("\namiberry_custom_none=", out_file);
	for (const auto& b : input.amiberry_custom_none) {
		fprintf(out_file, "%d ", b);
	}
	fputs("\namiberry_custom_hotkey=", out_file);
	for (const auto& b : input.amiberry_custom_hotkey) {
		fprintf(out_file, "%d ", b);
	}
	fputs("\namiberry_custom_axis_none=", out_file);
	for (const auto& a : input.amiberry_custom_axis_none) {
		fprintf(out_file, "%d ", a);
	}
	fputs("\namiberry_custom_axis_hotkey=", out_file);
	for (const auto& a : input.amiberry_custom_axis_hotkey) {
		fprintf(out_file, "%d ", a);
	}

	fclose(out_file);
}

void read_controller_mapping_from_file(controller_mapping& input, const std::string& filename)
{
	FILE* in_file = fopen(filename.c_str(), "rb");
	if (!in_file) {
		std::cerr << "Unable to open file " << filename << '\n';
		return;
	}

	char buffer[1024];
	std::string line;
	while (fgets(buffer, sizeof(buffer), in_file)) {
		line = buffer;
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
			line.pop_back();

		std::istringstream iss(line);
		std::string key;
		if (std::getline(iss, key, '=')) {
			if (key == "button") {
				for (auto& b : input.button) {
					iss >> b;
				}
			}
			else if (key == "axis") {
				for (auto& a : input.axis) {
					iss >> a;
				}
			}
			else if (key == "lstick_axis_y_invert") {
				iss >> input.lstick_axis_y_invert;
			}
			else if (key == "lstick_axis_x_invert") {
				iss >> input.lstick_axis_x_invert;
			}
			else if (key == "rstick_axis_y_invert") {
				iss >> input.rstick_axis_y_invert;
			}
			else if (key == "rstick_axis_x_invert") {
				iss >> input.rstick_axis_x_invert;
			}
			else if (key == "hotkey_button") {
				iss >> input.hotkey_button;
			}
			else if (key == "quit_button") {
				iss >> input.quit_button;
			}
			else if (key == "reset_button") {
				iss >> input.reset_button;
			}
			else if (key == "menu_button") {
				iss >> input.menu_button;
			}
			else if (key == "load_state_button") {
				iss >> input.load_state_button;
			}
			else if (key == "save_state_button") {
				iss >> input.save_state_button;
			}
			else if (key == "vkbd_button") {
				iss >> input.vkbd_button;
			}
			else if (key == "number_of_hats") {
				iss >> input.number_of_hats;
			}
			else if (key == "number_of_axis") {
				iss >> input.number_of_axis;
			}
			else if (key == "is_retroarch") {
				iss >> input.is_retroarch;
			}
			else if (key == "amiberry_custom_none") {
				for (auto& b : input.amiberry_custom_none) {
					iss >> b;
				}
			}
			else if (key == "amiberry_custom_hotkey") {
				for (auto& b : input.amiberry_custom_hotkey) {
					iss >> b;
				}
			}
			else if (key == "amiberry_custom_axis_none") {
				for (auto& a : input.amiberry_custom_axis_none) {
					iss >> a;
				}
			}
			else if (key == "amiberry_custom_axis_hotkey") {
				for (auto& a : input.amiberry_custom_axis_hotkey) {
					iss >> a;
				}
			}
		}
	}

	fclose(in_file);
}

std::vector<std::string> get_cd_drives()
{
	// Cache results to avoid expensive system calls on every GUI frame.
	// CD drives rarely change, so a 5-second refresh interval is sufficient.
	static std::vector<std::string> cached_results;
	static uint64_t last_query_time = 0;
	constexpr uint64_t cache_interval_ms = 5000;

	uint64_t now = SDL_GetTicks();
	if (last_query_time != 0 && (now - last_query_time) < cache_interval_ms) {
		return cached_results;
	}
	last_query_time = now;

	std::vector<std::string> results{};
#ifdef AMIBERRY_MACOS
	DASessionRef session = DASessionCreate(kCFAllocatorDefault);
	if (!session) {
		write_log("Failed to create DiskArbitration session, cannot auto-detect CD drives in system\n");
		return results;
	}

	DIR* devdir = opendir("/dev");
	if (!devdir) {
		write_log("Failed to open /dev, cannot auto-detect CD drives in system\n");
		CFRelease(session);
		return results;
	}

	auto is_whole_disk_name = [](const char* name) -> bool {
		if (!name) return false;
		if (strncmp(name, "disk", 4) != 0) return false;
		const char* p = name + 4;
		if (!*p) return false;
		for (; *p; ++p) {
			if (*p < '0' || *p > '9') return false;
		}
		return true;
	};

	auto cfstring_contains_ci = [](CFStringRef str, const char* needle) -> bool {
		if (!str || !needle) return false;
		char buf[256];
		if (!CFStringGetCString(str, buf, sizeof(buf), kCFStringEncodingUTF8))
			return false;
		for (char* p = buf; *p; ++p) *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
		std::string hay(buf);
		std::string ndl(needle);
		for (char& c : ndl) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		return hay.find(ndl) != std::string::npos;
	};

	struct dirent* entry = nullptr;
	while ((entry = readdir(devdir)) != nullptr) {
		if (!is_whole_disk_name(entry->d_name))
			continue;

		DADiskRef disk = DADiskCreateFromBSDName(kCFAllocatorDefault, session, entry->d_name);
		if (!disk)
			continue;

		CFDictionaryRef desc = DADiskCopyDescription(disk);
		if (!desc) {
			CFRelease(disk);
			continue;
		}

		CFBooleanRef whole = (CFBooleanRef)CFDictionaryGetValue(desc, kDADiskDescriptionMediaWholeKey);
		if (whole != kCFBooleanTrue) {
			CFRelease(desc);
			CFRelease(disk);
			continue;
		}

		CFBooleanRef ejectable = (CFBooleanRef)CFDictionaryGetValue(desc, kDADiskDescriptionMediaEjectableKey);
		CFBooleanRef removable = (CFBooleanRef)CFDictionaryGetValue(desc, kDADiskDescriptionMediaRemovableKey);
		if (ejectable != kCFBooleanTrue && removable != kCFBooleanTrue) {
			CFRelease(desc);
			CFRelease(disk);
			continue;
		}

		CFStringRef kind = (CFStringRef)CFDictionaryGetValue(desc, kDADiskDescriptionMediaKindKey);
		bool is_optical = cfstring_contains_ci(kind, "cd") || cfstring_contains_ci(kind, "dvd");
		if (is_optical) {
			std::string devpath = "/dev/";
			devpath.append(entry->d_name);
			results.emplace_back(devpath);
		}

		CFRelease(desc);
		CFRelease(disk);
	}

	closedir(devdir);
	CFRelease(session);

	if (results.empty())
		write_log("DiskArbitration did not find any CD drives on this system\n");
#elif defined(_WIN32)
	DWORD drive_mask = GetLogicalDrives();
	for (char letter = 'A'; letter <= 'Z'; letter++, drive_mask >>= 1) {
		if (!(drive_mask & 1))
			continue;
		char root[] = { letter, ':', '\\', '\0' };
		if (GetDriveTypeA(root) == DRIVE_CDROM) {
			results.emplace_back(root);
		}
	}
#elif defined(__HAIKU__)
	write_log("CD drive auto-detection not supported on Haiku\n");
#else
	char path[MAX_DPATH];
	FILE* fp = popen("lsblk -o NAME,TYPE | awk '$2==\"rom\"{print \"/dev/\"$1}'", "r");
	if (fp == nullptr) {
		write_log("Failed to run 'lsblk' command, cannot auto-detect CD drives in system\n");
		return results;
	}

	while (fgets(path, sizeof(path), fp) != nullptr) {
		path[strcspn(path, "\n")] = 0;
		results.emplace_back(path);
	}
	pclose(fp);
#endif
	cached_results = results;
	return cached_results;
}

void target_setdefaultstatefilename(const TCHAR* name)
{
	TCHAR path[MAX_DPATH];
	get_savestate_path(path, sizeof(path) / sizeof(TCHAR));
	if (!name || !name[0]) {
		_tcscat(path, _T("default.uss"));
	}
	else {
		const TCHAR* p2 = _tcsrchr(name, '\\');
		const TCHAR* p3 = _tcsrchr(name, '/');
		const TCHAR* p1 = nullptr;
		if (p2 >= p3) {
			p1 = p2;
		}
		else if (p3 >= p2) {
			p1 = p3;
		}
		if (p1) {
			_tcscat(path, p1 + 1);
		}
		else {
			_tcscat(path, name);
		}
		const TCHAR* p = _tcsrchr(path, '.');
		if (p) {
			path[_tcslen(path) - ((path + _tcslen(path)) - p)] = 0;
			_tcscat(path, _T(".uss"));
		}
	}
	_tcscpy(savestate_fname, path);
}



struct netdriverdata** target_ethernet_enumerate()
{
	if (net_enumerated)
		return ndd;
	ethernet_enumerate(ndd, 0);
	net_enumerated = 1;
	return ndd;
}
