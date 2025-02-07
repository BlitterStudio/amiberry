/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */

#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
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
#include "clipboard.h"
#include "fsdb.h"
#include "scsidev.h"
#ifdef FLOPPYBRIDGE
#include "floppybridge_lib.h"
#endif
#include "registry.h"
#include "threaddep/thread.h"
#include "uae/uae.h"
#include "sana2.h"

#ifdef __MACH__
#include <string>
#include <mach-o/dyld.h>
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
const char* chipname = "gpiochip0";
struct gpiod_chip* chip;
struct gpiod_line* lineRed;    // Red LED
struct gpiod_line* lineGreen;  // Green LED
struct gpiod_line* lineYellow; // Yellow LED
#endif

#ifdef USE_DBUS
#include "amiberry_dbus.h"
#endif

SDL_threadID mainthreadid;
static int logging_started;
int log_scsi;
int log_net;
int log_vsync, debug_vsync_min_delay, debug_vsync_forced_delay;
int uaelib_debug;
int pissoff_value = 15000 * CYCLE_UNIT;

static TCHAR* inipath = nullptr;
extern FILE* debugfile;
static int forceroms;
static void* tablet;
SDL_Cursor* normalcursor;

int paraport_mask;

int pause_emulation;

static int sound_closed;
static int recapture;
static int focus;
static int mouseinside;
int mouseactive;
int minimized;
int monitor_off;

static SDL_cond* cpu_wakeup_event;
static SDL_mutex* cpu_wakeup_mutex;
static volatile bool cpu_wakeup_event_triggered;

int quickstart_model = 0;
int quickstart_conf = 0;
bool host_poweroff = false;
int relativepaths = 0;
int saveimageoriginalpath = 0;

whdload_options whdload_prefs = {};
struct amiberry_options amiberry_options = {};
amiberry_gui_theme gui_theme = {};
amiberry_hotkey enter_gui_key;
SDL_GameControllerButton enter_gui_button;
amiberry_hotkey quit_key;
amiberry_hotkey action_replay_key;
amiberry_hotkey fullscreen_key;
amiberry_hotkey minimize_key;
SDL_GameControllerButton vkbd_button;

bool lctrl_pressed, rctrl_pressed, lalt_pressed, ralt_pressed, lshift_pressed, rshift_pressed, lgui_pressed, rgui_pressed;
bool hotkey_pressed = false;
bool mouse_grabbed = false;

void cap_fps(Uint64 start)
{
	const auto end = SDL_GetPerformanceCounter();
	const auto elapsed_ms = static_cast<float>(end - start) / static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.0f;

	const int refresh_rate = std::clamp(sdl_mode.refresh_rate, 50, 60);
	const float frame_time = 1000.0f / static_cast<float>(refresh_rate);
	const float delay_time = frame_time - elapsed_ms;

	if (delay_time > 0.0f)
		SDL_Delay(static_cast<Uint32>(delay_time));
}

std::string get_version_string()
{
	const auto pre_release_string = std::string(AMIBERRY_VERSION_PRE_RELEASE);
	if (pre_release_string.empty())
		return "Amiberry " + std::string(AMIBERRY_VERSION);
	return "Amiberry " + std::string(AMIBERRY_VERSION) + "-" + std::string(AMIBERRY_VERSION_PRE_RELEASE);
}

std::string get_copyright_notice()
{
	return COPYRIGHT;
}

std::string get_sdl2_version_string()
{
	SDL_version compiled;
	SDL_version linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	std::ostringstream sdl_compiled;
	sdl_compiled << "Compiled against SDL2 v" << static_cast<int>(compiled.major) << "." << static_cast<int>(compiled.minor) << "." << static_cast<int>(compiled.patch);
	sdl_compiled << ", Linked against SDL2 v" << static_cast<int>(linked.major) << "." << static_cast<int>(linked.minor) << "." << static_cast<int>(linked.patch);
	return sdl_compiled.str();
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

static amiberry_hotkey get_hotkey_from_config(std::string config_option)
{
	amiberry_hotkey hotkey = {};
	std::string delimiter = "+";
	std::string lctrl = "LCtrl";
	std::string rctrl = "RCtrl";
	std::string lalt = "LAlt";
	std::string ralt = "RAlt";
	std::string lshift = "LShift";
	std::string rshift = "RShift";
	std::string lgui = "LGUI";
	std::string rgui = "RGUI";
	
	if (config_option.empty()) return hotkey;

	size_t pos = 0;
	std::string token;
	while ((pos = config_option.find(delimiter)) != std::string::npos)
	{
		token = config_option.substr(0, pos);
		if (token == lctrl)
			hotkey.modifiers.lctrl = true;
		if (token == rctrl)
			hotkey.modifiers.rctrl = true;
		if (token == lalt)
			hotkey.modifiers.lalt = true;
		if (token == ralt)
			hotkey.modifiers.ralt = true;
		if (token == lshift)
			hotkey.modifiers.lshift = true;
		if (token == rshift)
			hotkey.modifiers.rshift = true;
		if (token == lgui)
			hotkey.modifiers.lgui = true;
		if (token == rgui)
			hotkey.modifiers.rgui = true;

		config_option.erase(0, pos + delimiter.length());
	}
	hotkey.key_name = config_option;
	hotkey.scancode = SDL_GetScancodeFromName(hotkey.key_name.c_str());
	hotkey.button = SDL_GameControllerGetButtonFromString(hotkey.key_name.c_str());
	
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

	enter_gui_button = SDL_GameControllerGetButtonFromString(p->open_gui);
	if (enter_gui_button == SDL_CONTROLLER_BUTTON_INVALID)
		enter_gui_button = SDL_GameControllerGetButtonFromString(amiberry_options.default_open_gui_key);
	if (enter_gui_button != SDL_CONTROLLER_BUTTON_INVALID)
	{
		for (int port = 0; port < 2; port++)
		{
			const auto host_joy_id = p->jports[port].id - JSEM_JOYS;
			didata* did = &di_joystick[host_joy_id];
			did->mapping.menu_button = enter_gui_button;
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

	if (strncmp(p->minimize, "", 1) != 0)
		minimize_key = get_hotkey_from_config(p->minimize);

	vkbd_button = SDL_GameControllerGetButtonFromString(p->vkbd_toggle);
	if (vkbd_button == SDL_CONTROLLER_BUTTON_INVALID)
		vkbd_button = SDL_GameControllerGetButtonFromString(amiberry_options.default_vkbd_toggle);
	if (vkbd_button != SDL_CONTROLLER_BUTTON_INVALID)
	{
		for (int port = 0; port < 2; port++)
		{
			const auto host_joy_id = p->jports[port].id - JSEM_JOYS;
			didata* did = &di_joystick[host_joy_id];
			did->mapping.vkbd_button = vkbd_button;
		}
	}
}

extern void signal_segv(int signum, siginfo_t* info, void* ptr);
extern void signal_buserror(int signum, siginfo_t* info, void* ptr);
extern void signal_term(int signum, siginfo_t* info, void* ptr);

extern void set_last_active_config(const char* filename);

std::string home_dir;
std::string current_dir;

#ifndef __MACH__
#include <linux/kd.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <sys/ioctl.h>
unsigned char kbd_led_status;
char kbd_flags;

std::string config_path;
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
std::string amiberry_conf_file;
std::string amiberry_ini_file;

char last_loaded_config[MAX_DPATH] = {};
char last_active_config[MAX_DPATH] = {};

int max_uae_width;
int max_uae_height;

extern "C" int main(int argc, char* argv[]);

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

int getdpiforwindow(SDL_Window* hwnd)
{
	float diagDPI = -1;
	float horiDPI = -1;
	float vertDPI = -1;

	SDL_GetDisplayDPI(0, &diagDPI, &horiDPI, &vertDPI);
	return static_cast<int>(vertDPI);
}

uae_s64 spincount;
extern bool calculated_scanline;

void target_spin(int total)
{
	if (!spincount || calculated_scanline)
		return;
	total = std::min(total, 10);
	while (total-- >= 0) {
		uae_s64 v1 = read_processor_time();
		v1 += spincount;
		while (v1 > read_processor_time());
	}
}

extern int vsync_activeheight;

void target_calibrate_spin()
{
	spincount = 0;
}

static int init_mmtimer()
{
	cpu_wakeup_event = SDL_CreateCond();
	cpu_wakeup_mutex = SDL_CreateMutex();
	if (!cpu_wakeup_event || !cpu_wakeup_mutex) {
		write_log(_T("Failed to create CPU wakeup event/mutex\n"));
		return 0;
	}
	return 1;
}

void sleep_cpu_wakeup()
{
	if (!cpu_wakeup_event_triggered) {
		cpu_wakeup_event_triggered = true;
		SDL_CondSignal(cpu_wakeup_event);
	}
}

int get_sound_event();

static int sleep_millis2(int ms, const bool main)
{
	frame_time_t start = 0;

	if (ms < 0)
		ms = -ms;
	if (main) {
		if (SDL_CondWaitTimeout(cpu_wakeup_event, cpu_wakeup_mutex, 0) == 0) {
			return 0;
		}
		start = read_processor_time();

		SDL_Delay(ms);
		//SDL_CondWaitTimeout(cpu_wakeup_event, cpu_wakeup_mutex, ms);
		cpu_wakeup_event_triggered = false;
	}
	else {
		SDL_Delay(ms);
	}

	if (main)
		idletime += read_processor_time() - start;
	return 0;
}

void sleep_micros (const int ms)
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
	set_inhibit_frame(monid, IHF_WINDOWHIDDEN);
}

void unsetminimized(const int monid)
{
	if (minimized > 0)
		full_redraw_all();
	minimized = 0;
	clear_inhibit_frame(monid, IHF_WINDOWHIDDEN);
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
			SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
			break;
		case 1:
			SDL_SetThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
			break;
		case 2:
			SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
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
		if (mon->screen_is_picasso && currprefs.rtg_hardwaresprite)
			SDL_ShowCursor(SDL_ENABLE);
		else
			SDL_ShowCursor(SDL_DISABLE);
	}
	else if (!picasso_setwincursor(monid)) {
		SDL_SetCursor(normalcursor);
		SDL_ShowCursor(SDL_ENABLE);
	}
}

void set_showcursor(const BOOL v)
{
	if (v) {
		const int vv = SDL_ShowCursor(SDL_ENABLE);
		if (vv > 1) {
			SDL_ShowCursor(SDL_DISABLE);
		}
	}
	else {
		int max = 10;
		while (max-- > 0) {
			int vv = SDL_ShowCursor(SDL_DISABLE);
			if (vv < 0) {
				while (vv < -1) {
					vv = SDL_ShowCursor(SDL_ENABLE);
				}
				break;
			}
		}
	}
}

void releasecapture(const AmigaMonitor* mon)
{
	SDL_SetWindowGrab(mon->amiga_window, SDL_FALSE);
	SDL_SetRelativeMouseMode(SDL_FALSE);
	set_showcursor(TRUE);
	mon_cursorclipped = 0;
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
		if (mon_cursorclipped == mon->monitor_id + 1) {
#if MOUSECLIP_LOG
			write_log(_T("CLIP mon=%d %dx%d %dx%d %d\n"), mon->monitor_id, mon->amigawin_rect.left, mon->amigawin_rect.top, mon->amigawin_rect.right, mon->amigawin_rect.bottom, isfullscreen());
#endif
			//SDL_SetWindowGrab(mon->amiga_window, SDL_TRUE);
			//SDL_WarpMouseInWindow(mon->amiga_window, mon->amigawinclip_rect.w / 2, mon->amigawinclip_rect.h / 2);
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
	const Uint32 flags = SDL_GetWindowFlags(mon->amiga_window);

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
	//close_tablet(tablet);
	//tablet = NULL;
	if (full) {
		//rawinput_release();
		SDL_SetWindowGrab(mon->amiga_window, SDL_FALSE);
	}
}
void target_inputdevice_acquire()
{
	const AmigaMonitor* mon = &AMonitors[0];
	target_inputdevice_unacquire(false);
	//tablet = open_tablet(mon->amiga_window);
	//rawinput_alloc();
	SDL_SetWindowGrab(mon->amiga_window, SDL_TRUE);
}

static void setmouseactive2(AmigaMonitor* mon, int active, const bool allowpause)
{
#ifdef RETROPLATFORM
	bool isrp = rp_isactive() != 0;
#else
	bool isrp = false;
#endif
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
		if (!isrp && !(SDL_GetWindowFlags(mon->amiga_window) & SDL_WINDOW_SHOWN))
			return;
	}
	
	if (active < 0)
		active = 1;

	mouseactive = active ? mon->monitor_id + 1 : 0;

	mon->mouseposx = mon->mouseposy = 0;
	releasecapture(mon);
	recapture = 0;

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

	if (mouseactive > 0)
		focus = mon->monitor_id + 1;

	if (mouseactive) {
		if (focus) {
#if MOUSECLIP_HIDE
			set_showcursor(FALSE);
#endif
			SDL_SetWindowGrab(mon->amiga_window, SDL_TRUE);
			// SDL2 hides the cursor when Relative mode is enabled
			// This means that the RTG hardware sprite will no longer be shown,
			// unless it's configured to use Virtual Mouse (absolute movement).
			if (!currprefs.input_tablet)
				SDL_SetRelativeMouseMode(SDL_TRUE);
			
			updatewinrect(mon, false);
			mon_cursorclipped = mon->monitor_id + 1;
			updatemouseclip(mon);
			setcursor(mon, -30000, -30000);
		}
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
}

void setmouseactive(const int monid, const int active)
{
	AmigaMonitor* mon = &AMonitors[monid];
	monitor_off = 0;
	if (active > 1)
		SDL_RaiseWindow(mon->amiga_window);
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
		}
		else
		{
			if (currprefs.active_nocapture_pause)
			{
				if (mouseactive)
					resumepaused(2);
			}
			else if (currprefs.minimized_pause && !currprefs.inactive_pause)
				resumepaused(1);
			else if (currprefs.inactive_pause)
				resumepaused(2);
		}
		sound_closed = 0;
	}
	getcapslock();
	wait_keyrelease();
	inputdevice_acquire(TRUE);
	if (isfullscreen() > 0 || currprefs.capture_always)
		setmouseactive(mon->monitor_id, 1);
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

	if (mouseactive) {
		disablecapture();
		SDL_WarpMouseGlobal(x, y);
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
		}
		else 
		{
			setmouseactive(monid, 0);
		}
	}
	else {
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

//Uint32 timer_callbackfunc(Uint32 interval, void* param)
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
	// No lightpen support (yet?)
	tablet_touch(id, pressrel, x, y, rcontrol);
}

void handle_focus_gained_event(const AmigaMonitor* mon)
{
	amiberry_active(mon, minimized);
	unsetminimized(mon->monitor_id);
}

void handle_minimized_event(const AmigaMonitor* mon)
{
	if (!minimized)
	{
		write_log(_T("SIZE_MINIMIZED\n"));
		setminimized(mon->monitor_id);
		amiberry_inactive(mon, minimized);
	}
}

void handle_restored_event(const AmigaMonitor* mon)
{
	amiberry_active(mon, minimized);
	unsetminimized(mon->monitor_id);
}

void handle_moved_event(AmigaMonitor* mon)
{
	if (isfullscreen() <= 0)
	{
		updatewinrect(mon, false);
		updatemouseclip(mon);
	}
}

void handle_enter_event()
{
	mouseinside = true;
	if (currprefs.input_tablet > 0 && currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC && isfullscreen() <= 0)
	{
		if (mousehack_alive())
			setcursorshape(0);
	}
}

void handle_leave_event()
{
	mouseinside = false;
}

void handle_focus_lost_event(const AmigaMonitor* mon)
{
	amiberry_inactive(mon, minimized);
	if (isfullscreen() <= 0 && currprefs.minimize_inactive)
		minimizewindow(mon->monitor_id);
}

void handle_close_event()
{
	wait_keyrelease();
	inputdevice_unacquire();
	uae_quit();
}

void handle_window_event(const SDL_Event& event, AmigaMonitor* mon)
{
	switch (event.window.event)
	{
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		handle_focus_gained_event(mon);
		break;
	case SDL_WINDOWEVENT_MINIMIZED:
		handle_minimized_event(mon);
		break;
	case SDL_WINDOWEVENT_RESTORED:
		handle_restored_event(mon);
		break;
	case SDL_WINDOWEVENT_MOVED:
		handle_moved_event(mon);
		break;
	case SDL_WINDOWEVENT_ENTER:
		handle_enter_event();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		handle_leave_event();
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		handle_focus_lost_event(mon);
		break;
	case SDL_WINDOWEVENT_CLOSE:
		handle_close_event();
		break;
	default:
		break;
	}
}

void handle_quit_event()
{
	uae_quit();
}

void handle_clipboard_update_event()
{
	if (currprefs.clipboard_sharing && SDL_HasClipboardText() == SDL_TRUE)
	{
		auto* clipboard_host = SDL_GetClipboardText();
		uae_clipboard_put_text(clipboard_host);
		SDL_free(clipboard_host);
	}
}

void handle_joy_device_event(const int which, const bool removed)
{
	const didata* existing_did = &di_joystick[which];
	if (existing_did->guid.empty() || removed)
	{
		write_log("SDL2 Controller/Joystick added or removed, re-running import joysticks...\n");
		if (inputdevice_devicechange(&currprefs))
		{
			import_joysticks();
			joystick_refresh_needed = true;
		}
	}
}

void handle_controller_button_event(const SDL_Event& event)
{
	const auto button = event.cbutton.button;
	const auto state = event.cbutton.state == SDL_PRESSED;

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
	else if (button == vkbd_button) {
		inputdevice_add_inputcode(AKS_OSK, state, nullptr);
	}
	else {
		for (auto id = 0; id < MAX_INPUT_DEVICES; id++) {
			const didata* did = &di_joystick[id];
			if (did->name.empty() || did->joystick_id != event.caxis.which || did->mapping.is_retroarch || !did->is_controller) continue;

			read_controller_button(id, button, state);
			break;
		}
	}
}

void handle_joy_button_event(const SDL_Event& event)
{
	const auto button = event.jbutton.button;
	const auto state = event.jbutton.state == SDL_PRESSED;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		const didata* did = &di_joystick[id];
		if (did->joystick_id != event.jbutton.which || did->name.empty() || did->joystick_id != id || (!did->mapping.is_retroarch && did->is_controller)) continue;

		if (button == did->mapping.hotkey_button)
		{
			hotkey_pressed = state;
			break;
		}
		if (button == did->mapping.menu_button && hotkey_pressed && state)
		{
			hotkey_pressed = false;
			inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
			break;
		}
		if (button == did->mapping.vkbd_button && hotkey_pressed && state)
		{
			hotkey_pressed = false;
			inputdevice_add_inputcode(AKS_OSK, 1, nullptr);
			break;
		}

		read_joystick_buttons(id);
		return;
	}
}

void handle_controller_axis_motion_event(const SDL_Event& event)
{
	const auto axis = event.caxis.axis;
	const auto value = event.caxis.value;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		const didata* did = &di_joystick[id];
		if (did->name.empty() || did->joystick_id != event.caxis.which || did->mapping.is_retroarch || !did->is_controller) continue;

		read_controller_axis(id, axis, value);
	}
}

void handle_joy_axis_motion_event(const SDL_Event& event)
{
	const auto axis = event.jaxis.axis;
	const auto value = event.jaxis.value;
	const auto which = event.jaxis.which;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		const didata* did = &di_joystick[id];
		if (did->name.empty() || did->joystick_id != which || did->joystick_id != id || (!did->mapping.is_retroarch && did->is_controller)) continue;

		read_joystick_axis(which, axis, value);
	}
}

void handle_joy_hat_motion_event(const SDL_Event& event)
{
	const auto hat = event.jhat.hat;
	const auto value = event.jhat.value;
	const auto which = event.jhat.which;

	for (auto id = 0; id < MAX_INPUT_DEVICES; id++)
	{
		const didata* did = &di_joystick[id];
		if (did->name.empty() || did->joystick_id != which || did->joystick_id != id || (!did->mapping.is_retroarch && did->is_controller)) continue;

		read_joystick_hat(which, hat, value);
		break;
	}
}

void handle_key_event(const SDL_Event& event)
{
	if (event.key.repeat != 0 || !isfocus() || (isfocus() < 2 && currprefs.input_tablet >= TABLET_MOUSEHACK && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)))
		return;

	int scancode = event.key.keysym.scancode;
	const auto pressed = event.key.state;

	if ((amiberry_options.rctrl_as_ramiga || currprefs.right_control_is_right_win_key) && scancode == SDL_SCANCODE_RCTRL)
	{
		scancode = SDL_SCANCODE_RGUI;
	}

	scancode = keyhack(scancode, pressed, 0);
	if (scancode >= 0)
	{
		my_kbd_handler(0, scancode, pressed, true);
	}
}

void handle_finger_event(const SDL_Event& event)
{
	if (!isfocus() || event.tfinger.fingerId != 0)
		return;

	if (event.tfinger.type == SDL_FINGERDOWN && event.button.clicks == 2)
	{
		setmousebuttonstate(0, 0, 1);
	}
	else if (event.tfinger.type == SDL_FINGERUP)
	{
		setmousebuttonstate(0, 0, 0);
	}
}

void handle_mouse_button_event(const SDL_Event& event, const AmigaMonitor* mon)
{
	const auto button = event.button.button;
	const auto state = event.button.state == SDL_PRESSED;
	const auto clicks = event.button.clicks;

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
			setmousebuttonstate(0, 0, state);
			break;
		case SDL_BUTTON_RIGHT:
			setmousebuttonstate(0, 1, state);
			break;
		case SDL_BUTTON_MIDDLE:
			if (currprefs.input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON)
				activationtoggle(0, true);
			else
				setmousebuttonstate(0, 2, state);
			break;
		case SDL_BUTTON_X1:
			setmousebuttonstate(0, 3, state);
			break;
		case SDL_BUTTON_X2:
			setmousebuttonstate(0, 4, state);
			break;
		default: break;
		}
	}
}

void handle_finger_motion_event(const SDL_Event& event)
{
	if (isfocus() && event.tfinger.fingerId == 0)
	{
		setmousestate(0, 0, event.tfinger.x, 1);
		setmousestate(0, 1, event.tfinger.y, 1);
	}
}

void handle_mouse_motion_event(const SDL_Event& event, const AmigaMonitor* mon)
{
	monitor_off = 0;

	if (mouseinside && recapture && isfullscreen() <= 0) {
		enablecapture(mon->monitor_id);
		return;
	}

	if (isfocus() <= 0) return;

	const auto is_picasso = mon->screen_is_picasso;
	Sint32 x, y, xrel, yrel;
	if (amiberry_options.rotation_angle == 0)
	{
		x = event.motion.x;
		y = event.motion.y;
		xrel = event.motion.xrel;
		yrel = event.motion.yrel;
	}
	else if (amiberry_options.rotation_angle == 180)
	{
		x = -event.motion.x;
		y = -event.motion.y;
		xrel = -event.motion.xrel;
		yrel = -event.motion.yrel;
	}
	else if (amiberry_options.rotation_angle == 90)
	{
		x = event.motion.y;
		y = -event.motion.x;
		xrel = event.motion.yrel;
		yrel = -event.motion.xrel;
	}
	else // -90
	{
		x = -event.motion.y;
		y = event.motion.x;
		xrel = -event.motion.yrel;
		yrel = event.motion.xrel;
	}

	if (currprefs.input_tablet >= TABLET_MOUSEHACK)
	{
		/* absolute */
		setmousestate(0, 0, is_picasso ? x : (x / 2) << currprefs.gfx_resolution, 1);
		setmousestate(0, 1, is_picasso ? y : (y / 2) << currprefs.gfx_vresolution, 1);
	}
	else
	{
		/* relative */
		setmousestate(0, 0, xrel, 0);
		setmousestate(0, 1, yrel, 0);
	}
}

void handle_mouse_wheel_event(const SDL_Event& event)
{
	if (isfocus() <= 0) return;
	
	const auto val_y = event.wheel.y;
	const auto val_x = event.wheel.x;

	setmousestate(0, 2, val_y, 0);
	setmousestate(0, 3, val_x, 0);

	if (val_y < 0)
		setmousebuttonstate(0, 3, -1);
	else if (val_y > 0)
		setmousebuttonstate(0, 4, -1);

	if (val_x < 0)
		setmousebuttonstate(0, 5, -1);
	else if (val_x > 0)
		setmousebuttonstate(0, 6, -1);
}

void handle_pen_event(const SDL_Event& event)
{
	//TODO Implement with SDL3 for Tablet support
	//if (inputdevice_is_tablet() <= 0 && !currprefs.tablet_library && !is_touch_lightpen()) {
	//	close_tablet(tablet);
	//	tablet = NULL;
	//	return;
	//}
	//if (pWTPacket((HCTX)lParam, (UINT)wParam, &pkt)) {
	//	AmigaMonitor* mon = &AMonitors[0];
	//	int x, y, z, pres, proxi;
	//	DWORD buttons;
	//	ORIENTATION ori;
	//	ROTATION rot;

	//	x = pkt.pkX;
	//	y = pkt.pkY;
	//	z = pkt.pkZ;
	//	pres = pkt.pkNormalPressure;
	//	ori = pkt.pkOrientation;
	//	rot = pkt.pkRotation;
	//	buttons = pkt.pkButtons;
	//	proxi = pkt.pkStatus;
	//	send_tablet(x, y, z, pres, buttons, proxi, ori.orAzimuth, ori.orAltitude, ori.orTwist, rot.roPitch, rot.roRoll, rot.roYaw, &mon->amigawin_rect);

	//}
}

void process_event(const SDL_Event& event)
{
	AmigaMonitor* mon = &AMonitors[0];

	// Handle window events
	if (event.type == SDL_WINDOWEVENT && SDL_GetWindowFromID(event.window.windowID) == mon->amiga_window)
	{
		handle_window_event(event, mon);
	}
	else
	{
		// Handle other types of events
		switch (event.type)
		{
		case SDL_QUIT:
			handle_quit_event();
			break;

		case SDL_CLIPBOARDUPDATE:
			handle_clipboard_update_event();
			break;

		case SDL_JOYDEVICEADDED:
			handle_joy_device_event(event.jdevice.which, false);
			break;
		case SDL_JOYDEVICEREMOVED:
			handle_joy_device_event(event.jdevice.which, true);
			break;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			handle_controller_button_event(event);
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			handle_joy_button_event(event);
			break;

		case SDL_CONTROLLERAXISMOTION:
			handle_controller_axis_motion_event(event);
			break;

		case SDL_JOYAXISMOTION:
			handle_joy_axis_motion_event(event);
			break;

		case SDL_JOYHATMOTION:
			handle_joy_hat_motion_event(event);
			break;

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			handle_key_event(event);
			break;

		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
			handle_finger_event(event);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			handle_mouse_button_event(event, mon);
			break;

		case SDL_FINGERMOTION:
			handle_finger_motion_event(event);
			break;

		case SDL_MOUSEMOTION:
			handle_mouse_motion_event(event, mon);
			break;

		case SDL_MOUSEWHEEL:
			handle_mouse_wheel_event(event);
			break;

		//TODO Implement with SDL3 for Tablet support
			/* Pressure-sensitive pen events */
			//  SDL_EVENT_PEN_PROXIMITY_IN = 0x1300,  /**< Pressure-sensitive pen has become available */
			//	SDL_EVENT_PEN_PROXIMITY_OUT,          /**< Pressure-sensitive pen has become unavailable */
			//	SDL_EVENT_PEN_DOWN,                   /**< Pressure-sensitive pen touched drawing surface */
			//	SDL_EVENT_PEN_UP,                     /**< Pressure-sensitive pen stopped touching drawing surface */
			//	SDL_EVENT_PEN_BUTTON_DOWN,            /**< Pressure-sensitive pen button pressed */
			//	SDL_EVENT_PEN_BUTTON_UP,              /**< Pressure-sensitive pen button released */
			//	SDL_EVENT_PEN_MOTION,                 /**< Pressure-sensitive pen is moving on the tablet */
			//	SDL_EVENT_PEN_AXIS,                   /**< Pressure-sensitive pen angle/pressure/etc changed */
			//
			// handle_pen_event(event);
			// break;
		default:
			break;
		}
	}
}

void update_clipboard()
{
	auto* clipboard_uae = uae_clipboard_get_text();
	if (clipboard_uae) {
		SDL_SetClipboardText(clipboard_uae);
		uae_clipboard_free_text(clipboard_uae);
	}
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

int handle_msgpump(bool vblank)
{
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

	if (pause_emulation)
	{
		if (was_paused == 0)
		{
			setpaused(pause_emulation);
			was_paused = pause_emulation;
			gui_fps(0, 0, false, 0, 0);
			gui_led(LED_SND, 0, -1);
			// we got just paused, report it to caller.
			return true;
		}
		lctrl_pressed = rctrl_pressed = lalt_pressed = ralt_pressed = lshift_pressed = rshift_pressed = lgui_pressed = rgui_pressed = false;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			process_event(event);
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
	if (pause_emulation)
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
		write_log("%s Logfile\n\n", get_version_string().c_str());
		write_log("%s\n", get_sdl2_version_string().c_str());
		regstatus();
	}
}

void logging_cleanup()
{
	if (debugfile)
		fclose(debugfile);
	debugfile = nullptr;
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
	_tcscat(p, "/");
}

std::string fix_trailing(std::string& input)
{
	if (!input.empty() && input.back() != '/')
	{
		return input + "/";
	}
	return input;
}

// convert path to absolute
void fullpath(TCHAR* path, int size, bool userelative)
{
	// Resolve absolute path
	TCHAR tmp1[MAX_DPATH];
	tmp1[0] = 0;
	if (realpath(path, tmp1) != nullptr)
	{
		if (_tcsnicmp(path, tmp1, _tcslen(tmp1)) != 0)
			_tcscpy(path, tmp1);
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
	_tcscpy(outpath, inpath);
	auto* const p = _tcsrchr(outpath, '/');
	if (p)
		p[0] = 0;
	fix_trailing(outpath);
}

void getfilepart(TCHAR* out, int size, const TCHAR* path)
{
	out[0] = 0;
	const auto* const p = _tcsrchr(path, '/');
	if (p)
		_tcscpy(out, p + 1);
	else
		_tcscpy(out, path);
}

uae_u8* target_load_keyfile(uae_prefs* p, const char* path, int* sizep, char* name)
{
	return nullptr;
}

std::vector<std::string> split_command(const std::string& command)
{
	std::vector<std::string> word_vector;
	std::size_t prev = 0, pos;
	while ((pos = command.find_first_of(' ', prev)) != std::string::npos)
	{
		if (pos > prev)
			word_vector.push_back(command.substr(prev, pos - prev));
		prev = pos + 1;
	}
	if (prev < command.length())
		word_vector.push_back(command.substr(prev, std::string::npos));

	return word_vector;
}

void replace(std::string& str, const std::string& from, const std::string& to)
{
	const auto start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return;
	str.replace(start_pos, from.length(), to);
}

void target_execute(const char* command)
{
	AmigaMonitor* mon = &AMonitors[0];
	releasecapture(mon);
	mouseactive = 0;

	write_log("Target_execute received: %s\n", command);
	const std::string command_string = command;
	auto command_parts = split_command(command_string);

	for (size_t i = 0; i < command_parts.size(); ++i)
	{
		if (i > 0)
		{
			const auto delimiter_position = command_parts[i].find(':');
			if (delimiter_position != std::string::npos)
			{
				auto volume_or_device_name = command_parts[i].substr(0, delimiter_position);
				for (auto mount = 0; mount < currprefs.mountitems; mount++)
				{
					if (currprefs.mountconfig[mount].ci.type == UAEDEV_DIR)
					{
						if (_tcsicmp(currprefs.mountconfig[mount].ci.volname, volume_or_device_name.c_str()) == 0
							|| _tcsicmp(currprefs.mountconfig[mount].ci.devname, volume_or_device_name.c_str()) == 0)
						{
							std::string root_directory = currprefs.mountconfig[mount].ci.rootdir;
							volume_or_device_name += ':';
							replace(command_parts[i], volume_or_device_name, root_directory);
						}
					}
				}
			}
		}
	}

	std::ostringstream command_stream;
	for (const auto& part : command_parts)
	{
		command_stream << part << " ";
	}
	// Ensure this runs in the background, otherwise we'll block the emulator until it returns
	command_stream << "&";

	try
	{
		std::string final_command = command_stream.str();
		write_log("Executing: %s\n", final_command.c_str());
		system(final_command.c_str());
	}
	catch (const std::exception& e)
	{
		write_log("Exception thrown when trying to execute: %s. Exception: %s", command, e.what());
	}
}

void target_run()
{
	// Reset counter for access violations
	init_max_signals();
}

void target_quit()
{
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
#endif
	
	if (p->rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE) {
		p->rtg_hardwareinterrupt = false;
		p->rtg_hardwaresprite = false;
		p->rtgmatchdepth = false;
		p->color_mode = 5;
	}

#ifdef AMIBERRY
	// Disable hardware sprite if not using Virtual mouse mode,
	// otherwise we'll have no cursor showing (due to SDL2 Relative mouse)
	if (p->rtg_hardwaresprite && p->input_tablet == 0)
		p->rtg_hardwaresprite = false;
#endif

	const MultiDisplay* md = getdisplay(p, 0);
	for (auto & j : p->gfx_monitor) {
		if (j.gfx_size_fs.special == WH_NATIVE) {
			int i;
			for (i = 0; md->DisplayModes[i].depth >= 0; i++) {
				if (md->DisplayModes[i].res.width == md->rect.w - md->rect.x &&
					md->DisplayModes[i].res.height == md->rect.h - md->rect.y) {
					j.gfx_size_fs.width = md->DisplayModes[i].res.width;
					j.gfx_size_fs.height = md->DisplayModes[i].res.height;
					write_log(_T("Native resolution: %dx%d\n"), j.gfx_size_fs.width, j.gfx_size_fs.height);
					break;
				}
			}
			if (md->DisplayModes[i].depth < 0) {
				j.gfx_size_fs.special = 0;
				write_log(_T("Native resolution not found.\n"));
			}
		}
	}
	/* switch from 32 to 16 or vice versa if mode does not exist */
	//if (1 || isfullscreen() > 0) {
		int depth = p->color_mode == 5 ? 4 : 2;
		for (int i = 0; md->DisplayModes[i].depth >= 0; i++) {
			if (md->DisplayModes[i].depth == depth) {
				depth = 0;
				break;
			}
		}
		if (depth) {
			p->color_mode = p->color_mode == 5 ? 2 : 5;
		}
	//}

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
		p->rtgmatchdepth = true;
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
		p->gfx_api = 4;
		p->color_mode = 5;
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
		p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_B8G8R8A8;
		//p->filesystem_mangle_reserved_names = true;
	}

	p->multithreaded_drawing = amiberry_options.default_multithreaded_drawing;

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
	p->drawbridge_connected_drive_b = false;
	p->drawbridge_driver = 0;

	drawbridge_update_profiles(p);

	p->alt_tab_release = false;
	p->sound_pullmode = amiberry_options.default_sound_pull;

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

	// Virtual keyboard default options
	p->vkbd_enabled = amiberry_options.default_vkbd_enabled;
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

extern int scsiromselected;

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

	cfgfile_target_dwrite_bool(f, _T("rtg_match_depth"), p->rtgmatchdepth);
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

	cfgfile_target_dwrite_str(f, _T("open_gui"), p->open_gui);
	cfgfile_target_dwrite_str(f, _T("quit_amiberry"), p->quit_amiberry);
	cfgfile_target_dwrite_str(f, _T("action_replay"), p->action_replay);
	cfgfile_target_dwrite_str(f, _T("fullscreen_toggle"), p->fullscreen_toggle);
	cfgfile_target_dwrite_str(f, _T("minimize"), p->minimize);

	if (p->drawbridge_driver > 0)
	{
		cfgfile_target_dwrite(f, _T("drawbridge_driver"), _T("%d"), p->drawbridge_driver);
		cfgfile_target_dwrite_bool(f, _T("drawbridge_serial_autodetect"), p->drawbridge_serial_auto);
		cfgfile_target_write_str(f, _T("drawbridge_serial_port"), p->drawbridge_serial_port);
		cfgfile_target_dwrite_bool(f, _T("drawbridge_smartspeed"), p->drawbridge_smartspeed);
		cfgfile_target_dwrite_bool(f, _T("drawbridge_autocache"), p->drawbridge_autocache);
		cfgfile_target_dwrite_bool(f, _T("drawbridge_connected_drive_b"), p->drawbridge_connected_drive_b);
	}

	cfgfile_target_dwrite_bool(f, _T("alt_tab_release"), p->alt_tab_release);
	cfgfile_target_dwrite(f, _T("sound_pullmode"), _T("%d"), p->sound_pullmode);

	cfgfile_target_dwrite_bool(f, _T("use_retroarch_quit"), p->use_retroarch_quit);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_menu"), p->use_retroarch_menu);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_reset"), p->use_retroarch_reset);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_vkbd"), p->use_retroarch_vkbd);

	if (scsiromselected > 0)
		cfgfile_target_write(f, _T("expansion_gui_page"), expansionroms[scsiromselected].name);

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
		|| cfgfile_yesno(option, value, _T("drawbridge_connected_drive_b"), &p->drawbridge_connected_drive_b)
		|| cfgfile_yesno(option, value, _T("alt_tab_release"), &p->alt_tab_release)
		|| cfgfile_yesno(option, value, _T("use_retroarch_quit"), &p->use_retroarch_quit)
		|| cfgfile_yesno(option, value, _T("use_retroarch_menu"), &p->use_retroarch_menu)
		|| cfgfile_yesno(option, value, _T("use_retroarch_reset"), &p->use_retroarch_reset)
		|| cfgfile_yesno(option, value, _T("use_retroarch_vkbd"), &p->use_retroarch_vkbd)
		|| cfgfile_intval(option, value, _T("sound_pullmode"), &p->sound_pullmode, 1)
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
		|| cfgfile_intval(option, value, _T("cpu_idle"), &p->cpu_idle, 1))
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
	
	if (cfgfile_yesno(option, value, _T("rtg_match_depth"), &p->rtgmatchdepth))
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

void get_configuration_path(char* out, const int size)
{
	_tcsncpy(out, fix_trailing(config_path).c_str(), size - 1);
}

void set_configuration_path(const std::string& newpath)
{
	config_path = newpath;
}

void set_nvram_path(const std::string& newpath)
{
	nvram_dir = newpath;
}

void set_plugins_path(const std::string& newpath)
{
	plugins_dir = newpath;
}

void set_video_path(const std::string& newpath)
{
	video_dir = newpath;
}

void set_themes_path(const std::string& newpath)
{
	themes_path = newpath;
}

void set_screenshot_path(const std::string& newpath)
{
	screenshot_dir = newpath;
}

void set_savestate_path(const std::string& newpath)
{
	savestate_dir = newpath;
}

std::string get_controllers_path()
{
	return fix_trailing(controllers_path);
}

void set_controllers_path(const std::string& newpath)
{
	controllers_path = newpath;
}

std::string get_retroarch_file()
{
	return retroarch_file;
}

void set_retroarch_file(const std::string& newpath)
{
	retroarch_file = newpath;
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
}

std::string get_whdload_arch_path()
{
	return fix_trailing(whdload_arch_path);
}

void set_whdload_arch_path(const std::string& newpath)
{
	whdload_arch_path = newpath;
}

std::string get_floppy_path()
{
	return fix_trailing(floppy_path);
}

void set_floppy_path(const std::string& newpath)
{
	floppy_path = newpath;
}

std::string get_harddrive_path()
{
	return fix_trailing(harddrive_path);
}

void set_harddrive_path(const std::string& newpath)
{
	harddrive_path = newpath;
}

std::string get_cdrom_path()
{
	return fix_trailing(cdrom_path);
}

void set_cdrom_path(const std::string& newpath)
{
	cdrom_path = newpath;
}

std::string get_logfile_path()
{
	return logfile_path;
}

void set_logfile_path(const std::string& newpath)
{
	logfile_path = newpath;
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

	auto* f = fopen(file, "rte");
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
		f = fopen(tmp, "rte");
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

	if (dirs != nullptr && !dirs->empty() && (*dirs)[0] == ".")
		dirs->erase(dirs->begin());

	if (dirs != nullptr)
		sort(dirs->begin(), dirs->end());
	if (files != nullptr)
		sort(files->begin(), files->end());
}

void save_amiberry_settings()
{
	auto* const f = fopen(amiberry_conf_file.c_str(), "we");
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

	// Use a separate thread for drawing native chipset output
	// This helps with performance, but may cause glitches in some cases
	write_bool_option("default_multithreaded_drawing", amiberry_options.default_multithreaded_drawing);

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

	// Rotation angle of the output display (useful for screens with portrait orientation, like the Go Advance)
	write_int_option("rotation_angle", amiberry_options.rotation_angle);
	
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

	// Default Sound buffer size
	write_int_option("default_sound_buffer", amiberry_options.default_sound_buffer);

	// Default Sound Mode (Pull/Push)
	write_bool_option("default_sound_pull", amiberry_options.default_sound_pull);

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

	// Paths
	write_string_option("config_path", config_path);
	write_string_option("controllers_path", controllers_path);
	write_string_option("retroarch_config", retroarch_file);
	write_string_option("whdboot_path", whdboot_path);
	write_string_option("whdload_arch_path", whdload_arch_path);
	write_string_option("floppy_path", floppy_path);
	write_string_option("harddrive_path", harddrive_path);
	write_string_option("cdrom_path", cdrom_path);
	write_string_option("logfile_path", logfile_path);
	write_string_option("rom_path", rom_path);
	write_string_option("rp9_path", rp9_path);
	write_string_option("floppy_sounds_dir", floppy_sounds_dir);
	write_string_option("saveimage_dir", saveimage_dir);
	write_string_option("savestate_dir", savestate_dir);
	write_string_option("screenshot_dir", screenshot_dir);
	write_string_option("ripper_path", ripper_path);
	write_string_option("inputrecordings_dir", input_dir);
	write_string_option("nvram_dir", nvram_dir);
	write_string_option("plugins_dir", plugins_dir);
	write_string_option("video_dir", video_dir);
	write_string_option("themes_path", themes_path);

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

static int parse_amiberry_settings_line(const char *path, char *linea)
{
	TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
	int numROMs, numDisks, numCDs;
	auto romType = -1;
	char romName[MAX_DPATH] = {'\0'};
	char romPath[MAX_DPATH] = {'\0'};
	char tmpFile[MAX_DPATH];
	int ret = 0;

	if (!cfgfile_separate_linea(path, linea, option, value))
		return 0;

	if (cfgfile_string(option, value, "Diskfile", tmpFile, sizeof tmpFile))
	{
		auto* const f = fopen(tmpFile, "rbe");
		if (f != nullptr)
		{
			fclose(f);
			lstMRUDiskList.emplace_back(tmpFile);
			ret = 1;
		}
	}
	else if (cfgfile_string(option, value, "CDfile", tmpFile, sizeof tmpFile))
	{
		auto* const f = fopen(tmpFile, "rbe");
		if (f != nullptr)
		{
			fclose(f);
			lstMRUCDList.emplace_back(tmpFile);
			ret = 1;
		}
	}
	else if (cfgfile_string(option, value, "WHDLoadfile", tmpFile, sizeof tmpFile))
	{
		auto* const f = fopen(tmpFile, "rbe");
		if (f != nullptr)
		{
			fclose(f);
			lstMRUWhdloadList.emplace_back(tmpFile);
			ret = 1;
		}
	}
	else
	{
		ret |= cfgfile_string(option, value, "config_path", config_path);
		ret |= cfgfile_string(option, value, "controllers_path", controllers_path);
		ret |= cfgfile_string(option, value, "retroarch_config", retroarch_file);
		ret |= cfgfile_string(option, value, "whdboot_path", whdboot_path);
		ret |= cfgfile_string(option, value, "whdload_arch_path", whdload_arch_path);
		ret |= cfgfile_string(option, value, "floppy_path", floppy_path);
		ret |= cfgfile_string(option, value, "harddrive_path", harddrive_path);
		ret |= cfgfile_string(option, value, "cdrom_path", cdrom_path);
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
		ret |= cfgfile_yesno(option, value, "default_multithreaded_drawing", &amiberry_options.default_multithreaded_drawing);
		ret |= cfgfile_intval(option, value, "input_default_mouse_speed", &amiberry_options.input_default_mouse_speed, 1);
		ret |= cfgfile_yesno(option, value, "input_keyboard_as_joystick_stop_keypresses", &amiberry_options.input_keyboard_as_joystick_stop_keypresses);
		ret |= cfgfile_string(option, value, "default_open_gui_key", amiberry_options.default_open_gui_key, sizeof amiberry_options.default_open_gui_key);
		ret |= cfgfile_string(option, value, "default_quit_key", amiberry_options.default_quit_key, sizeof amiberry_options.default_quit_key);
		ret |= cfgfile_string(option, value, "default_ar_key", amiberry_options.default_ar_key, sizeof amiberry_options.default_ar_key);
		ret |= cfgfile_string(option, value, "default_fullscreen_toggle_key", amiberry_options.default_fullscreen_toggle_key, sizeof amiberry_options.default_fullscreen_toggle_key);
		ret |= cfgfile_intval(option, value, "rotation_angle", &amiberry_options.rotation_angle, 1);
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
		ret |= cfgfile_yesno(option, value, "default_sound_pull", &amiberry_options.default_sound_pull);
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
		ret |= cfgfile_yesno(option, value, "default_vkbd_enabled", &amiberry_options.default_vkbd_enabled);
		ret |= cfgfile_yesno(option, value, "default_vkbd_hires", &amiberry_options.default_vkbd_hires);
		ret |= cfgfile_yesno(option, value, "default_vkbd_exit", &amiberry_options.default_vkbd_exit);
		ret |= cfgfile_string(option, value, "default_vkbd_language", amiberry_options.default_vkbd_language, sizeof amiberry_options.default_vkbd_language);
		ret |= cfgfile_string(option, value, "default_vkbd_style", amiberry_options.default_vkbd_style, sizeof amiberry_options.default_vkbd_style);
		ret |= cfgfile_intval(option, value, "default_vkbd_transparency", &amiberry_options.default_vkbd_transparency, 1);
		ret |= cfgfile_string(option, value, "default_vkbd_toggle", amiberry_options.default_vkbd_toggle, sizeof amiberry_options.default_vkbd_toggle);
		ret |= cfgfile_string(option, value, "gui_theme", amiberry_options.gui_theme, sizeof amiberry_options.gui_theme);
	}
	return ret;
}

static int parse_amiberry_cmd_line(int *argc, char* argv[], const int remove_used_args)
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
			if (!parse_amiberry_settings_line("<command line>", argv[i + 1]))
			{
				// fail because on cmd line we require correctly formatted setting in option arg
				return 0;
			}
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

bool download_file(const std::string& source, const std::string& destination, bool keep_backup)
{
	// homebrew installs in different locations on OSX Intel vs OSX Apple Silicon
#if defined (__MACH__) && defined (__arm64__)	
	std::string wget_path = "/opt/homebrew/bin/wget";
	if (!file_exists(wget_path))
	{
		write_log("Could not locate wget in /opt/homebrew/ - Please use homebrew to install it!\n");
		return false;
	}
#elif defined(__MACH__)
	std::string wget_path = "/usr/local/bin/wget";
	if (!file_exists(wget_path))
	{
		write_log("Could not locate wget in /usr/local/bin/ - Please use homebrew to install it!\n");
		return false;
	}
#else
	std::string wget_path = "wget";
#endif
	std::string download_command = wget_path + " -np -nv -O ";
	auto tmp = destination;
	tmp = tmp.append(".tmp");

	download_command.append(tmp);
	download_command.append(" ");
	download_command.append(source);
	download_command.append(" 2>&1");

	// Cleanup if the tmp destination already exists
	if (file_exists(tmp))
	{
		write_log("Existing file found, removing %s\n", tmp.c_str());
		if (std::remove(tmp.c_str()) < 0)
		{
			write_log(strerror(errno));
			write_log("\n");
		}
	}

	try
	{
		char buffer[MAX_DPATH];
		const auto output = popen(download_command.c_str(), "r");
		if (!output)
		{
			write_log("Failed while trying to run wget! Make sure it exists in your system...\n");
			return false;
		}

		while (fgets(buffer, sizeof buffer, output))
		{
			write_log(buffer);
			write_log("\n");
		}
		pclose(output);
	}
	catch (...)
	{
		write_log("An exception was thrown while trying to execute wget!\n");
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

// this is where the required assets are stored, like fonts, icons, etc.
std::string get_data_directory(bool portable_mode)
{
#ifdef __MACH__
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
#else
	if (portable_mode)
	{
		char tmp[MAX_DPATH];
		getcwd(tmp, MAX_DPATH);
		write_log("Portable mode: Setting data directory to startup path: %s\n", tmp);
		return  std::string(tmp) + "/data/";
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

	// 3: Fallback Portable mode, all in the startup path
	char tmp[MAX_DPATH];
	getcwd(tmp, MAX_DPATH);
	write_log("Using data directory from startup path: %s\n", tmp);
	return  std::string(tmp) + "/data/";
#endif
}

// This path wil be used to create most of the user-specific files and directories
// Kickstart ROMs, HDD images, Floppy images will live under this directory
std::string get_home_directory(const bool portable_mode)
{
	if (portable_mode)
	{
		// Portable mode, all in startup path
		write_log("Portable mode: Setting home directory to startup path\n");
		char tmp[MAX_DPATH];
		getcwd(tmp, MAX_DPATH);
		return {tmp};
	}
	const auto env_home_dir = getenv("AMIBERRY_HOME_DIR");
	const auto user_home_dir = getenv("HOME");

	// 1: Check if the $AMIBERRY_HOME_DIR ENV variable is set
	if (env_home_dir != nullptr && my_existsdir(env_home_dir))
	{
		// If the ENV variable is set, use it
		write_log("Using home directory from AMIBERRY_HOME_DIR: %s\n", env_home_dir);
		return { env_home_dir };
	}
	// 2: Check $HOME/Amiberry
	if (user_home_dir != nullptr)
	{
        if (!directory_exists(user_home_dir, "/Amiberry"))
        {
            // If $HOME exists, but not the Amiberry subdirectory, create it
            my_mkdir((std::string(user_home_dir) + "/Amiberry").c_str());
        }
		// $HOME/Amiberry exists, use it
		write_log("Using home directory from $HOME/Amiberry\n");
        auto result = std::string(user_home_dir);
		return result.append("/Amiberry");
	}

	// 3: Fallback Portable mode, all in startup path
	write_log("Fallback Portable mode: Setting home directory to startup path\n");
	char tmp[MAX_DPATH];
	getcwd(tmp, MAX_DPATH);
	return {tmp};
}

// The location of .uae configurations
std::string get_config_directory(bool portable_mode)
{
#ifdef __MACH__
    const auto user_home_dir = getenv("HOME");
    if (!directory_exists(user_home_dir, "/Amiberry"))
    {
        my_mkdir((std::string(user_home_dir) + "/Amiberry").c_str());
    }
    if (!directory_exists(user_home_dir, "/Amiberry/Configurations"))
    {
        my_mkdir((std::string(user_home_dir) + "/Amiberry/Configurations").c_str());
    }
    auto result = std::string(user_home_dir);
    return result.append("/Amiberry/Configurations");
#else
	if (portable_mode)
	{
		write_log("Portable mode: Setting config directory to startup path\n");
		char tmp[MAX_DPATH];
		getcwd(tmp, MAX_DPATH);
		return { std::string(tmp) + "/conf" };
	}

	const auto env_conf_dir = getenv("AMIBERRY_CONFIG_DIR");
	const auto user_home_dir = getenv("HOME");

	// 1: Check if the $AMIBERRY_CONFIG_DIR ENV variable is set
	if (env_conf_dir != nullptr)
	{
		// If the ENV variable is set, use it
		write_log("Using config directory from AMIBERRY_CONFIG_DIR: %s\n", env_conf_dir);
		return { env_conf_dir };
	}
	// 2: Check $HOME/Amiberry/conf
	if (user_home_dir != nullptr)
	{
		auto result = std::string(user_home_dir);
		return result.append("/Amiberry/conf");
	}

	// 3: Fallback Portable mode, all in startup path
    // Should never really end up here, unless $HOME is not defined
	write_log("Using config directory from startup path\n");
	char tmp[MAX_DPATH];
	getcwd(tmp, MAX_DPATH);
	return { std::string(tmp) + "/conf" };
#endif
}

// Plugins that Amiberry can use, usually in the form of shared libraries
std::string get_plugins_directory(bool portable_mode)
{
#ifdef __MACH__
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
#else
	if (portable_mode)
	{
		write_log("Portable mode: Setting plugins directory to startup path\n");
		char tmp[MAX_DPATH];
		getcwd(tmp, MAX_DPATH);
		return { std::string(tmp) + "/plugins" };
	}

	// 1: Check if the $AMIBERRY_PLUGINS_DIR ENV variable is set
	const auto env_plugins_dir = getenv("AMIBERRY_PLUGINS_DIR");
	if (env_plugins_dir != nullptr && my_existsdir(env_plugins_dir))
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
	// 3: Check for ~/Amiberry/plugins
    const auto user_home_dir = getenv("HOME");
    if (user_home_dir != nullptr)
    {
        if (!directory_exists(user_home_dir, "/Amiberry"))
        {
            my_mkdir((std::string(user_home_dir) + "/Amiberry").c_str());
        }
        // $HOME/Amiberry exists, use it
        if (!directory_exists(user_home_dir, "/Amiberry/plugins"))
        {
            my_mkdir((std::string(user_home_dir) + "/Amiberry/plugins").c_str());
        }
        write_log("Using plugins directory from $HOME/Amiberry/plugins\n");
        return { std::string(user_home_dir) + "/Amiberry/plugins" };
    }

    // 4: Fallback Portable mode, all in the startup path
    write_log("Using plugins directory from startup path\n");
    char tmp[MAX_DPATH];
    getcwd(tmp, MAX_DPATH);
    return { std::string(tmp) + "/plugins" };
#endif
}

extern void save_theme(const std::string& theme_filename);
extern void load_theme(const std::string& theme_filename);
extern void load_default_theme();

void create_missing_amiberry_folders()
{
#ifdef __MACH__
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
#endif
	if (!my_existsdir(config_path.c_str()))
		my_mkdir(config_path.c_str());
	if (!my_existsdir(controllers_path.c_str()))
	{
		my_mkdir(controllers_path.c_str());
#ifdef __MACH__
		const std::string default_controller_path = app_directory + "/Resources/controllers/";
#else
		const std::string default_controller_path = AMIBERRY_DATADIR "/controllers/";
#endif
		// copy default controller files, if they exist in AMIBERRY_DATADIR/controllers
		if (my_existsdir(default_controller_path.c_str()))
		{
			const std::string command = "cp -R " + default_controller_path + "* " + controllers_path;
			system(command.c_str());
		}
		else if (my_existsdir("/usr/share/amiberry/controllers/"))
		{
			const std::string command = "cp -R /usr/share/amiberry/controllers/* " + controllers_path;
			system(command.c_str());
		}
	}
	if (!my_existsdir(whdboot_path.c_str()))
	{
		my_mkdir(whdboot_path.c_str());
#ifdef __MACH__
		const std::string default_whdboot_path = app_directory + "/Resources/whdboot/";
#else
		const std::string default_whdboot_path = AMIBERRY_DATADIR "/whdboot/";
#endif
		// copy default whdboot files, if they exist in AMIBERRY_DATADIR/whdboot
		if (my_existsdir(default_whdboot_path.c_str()))
		{
			const std::string command = "cp -R " + default_whdboot_path + "* " + whdboot_path;
			system(command.c_str());
		}
		else if (my_existsdir("/usr/share/amiberry/whdboot/"))
		{
			const std::string command = "cp -R /usr/share/amiberry/whdboot/* " + whdboot_path;
			system(command.c_str());
		}
		else if (my_existsdir("/usr/local/share/amiberry/whdboot/"))
		{
			const std::string command = "cp -R /usr/local/share/amiberry/whdboot/* " + whdboot_path;
			system(command.c_str());
		}
		else
		{
			write_log("No WHDLoad boot files found in %s, %s, or %s\n", default_whdboot_path.c_str(), "/usr/share/amiberry/whdboot/", "/usr/local/share/amiberry/whdboot/");
			write_log("Attempting to download them from the internet...\n");

			std::string directory_name = whdboot_path + "save-data";
			if (!my_existsdir(directory_name.c_str()))
				my_mkdir(directory_name.c_str());

			directory_name = whdboot_path + "save-data/Autoboots";
			if (!my_existsdir(directory_name.c_str()))
				my_mkdir(directory_name.c_str());

			directory_name = whdboot_path + "save-data/Debugs";
			if (!my_existsdir(directory_name.c_str()))
				my_mkdir(directory_name.c_str());

			directory_name = whdboot_path + "save-data/Kickstarts";
			if (!my_existsdir(directory_name.c_str()))
				my_mkdir(directory_name.c_str());

			directory_name = whdboot_path + "save-data/Savegames";
			if (!my_existsdir(directory_name.c_str()))
				my_mkdir(directory_name.c_str());

			directory_name = whdboot_path + "game-data";
			if (!my_existsdir(directory_name.c_str()))
				my_mkdir(directory_name.c_str());

			//  download WHDLoad executable
			std:: string destination = get_whdbootpath().append("WHDLoad");
			write_log("Downloading %s ...\n", destination.c_str());
			download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/WHDLoad?raw=true", destination, false);

			//  download JST executable
			destination = get_whdbootpath().append("JST");
			write_log("Downloading %s ...\n", destination.c_str());
			download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/JST?raw=true", destination, false);

			//  download AmiQuit executable
			destination = get_whdbootpath().append("AmiQuit");
			write_log("Downloading %s ...\n", destination.c_str());
			download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/AmiQuit?raw=true", destination, false);

			//  download boot-data.zip
			destination = get_whdbootpath().append("boot-data.zip");
			write_log("Downloading %s ...\n", destination.c_str());
			download_file("https://github.com/BlitterStudio/amiberry/blob/master/whdboot/boot-data.zip?raw=true", destination, false);

			// download kickstart RTB files for maximum compatibility
			download_rtb("kick33180.A500.RTB");
			download_rtb("kick34005.A500.RTB");
			download_rtb("kick40063.A600.RTB");
			download_rtb("kick40068.A1200.RTB");
			download_rtb("kick40068.A4000.RTB");

			destination = get_whdbootpath().append("game-data/whdload_db.xml");
			download_file("https://github.com/HoraceAndTheSpider/Amiberry-XML-Builder/blob/master/whdload_db.xml?raw=true", destination, true);
		}
	}
    if (!my_existsdir(whdload_arch_path.c_str()))
        my_mkdir(whdload_arch_path.c_str());
    if (!my_existsdir(floppy_path.c_str()))
        my_mkdir(floppy_path.c_str());
    if (!my_existsdir(harddrive_path.c_str()))
        my_mkdir(harddrive_path.c_str());
    if (!my_existsdir(cdrom_path.c_str()))
        my_mkdir(cdrom_path.c_str());
	if (!my_existsdir(rom_path.c_str()))
	{
		my_mkdir(rom_path.c_str());
#ifdef __MACH__
		const std::string default_roms_path = app_directory + "/Resources/roms/";
#else
		const std::string default_roms_path = AMIBERRY_DATADIR "/roms/";
#endif
		// copy default kickstart files, if they exist in AMIBERRY_DATADIR/roms
		if (my_existsdir(default_roms_path.c_str()))
		{
			const std::string command = "cp -R " + default_roms_path + "* " + rom_path;
			system(command.c_str());
		}
		else if (my_existsdir("/usr/share/amiberry/roms/"))
		{
			const std::string command = "cp -R /usr/share/amiberry/roms/* " + rom_path;
			system(command.c_str());
		}
		else if (my_existsdir("/usr/local/share/amiberry/roms/"))
		{
			const std::string command = "cp -R /usr/local/share/amiberry/roms/* " + rom_path;
			system(command.c_str());
		}
	}
	if (!my_existsdir(rp9_path.c_str()))
		my_mkdir(rp9_path.c_str());
	if (!my_existsdir(saveimage_dir.c_str()))
		my_mkdir(saveimage_dir.c_str());
	if (!my_existsdir(savestate_dir.c_str()))
		my_mkdir(savestate_dir.c_str());
	if (!my_existsdir(ripper_path.c_str()))
		my_mkdir(ripper_path.c_str());
	if (!my_existsdir(input_dir.c_str()))
		my_mkdir(input_dir.c_str());
	if (!my_existsdir(screenshot_dir.c_str()))
		my_mkdir(screenshot_dir.c_str());
	if (!my_existsdir(nvram_dir.c_str()))
		my_mkdir(nvram_dir.c_str());
	if (!my_existsdir(video_dir.c_str()))
		my_mkdir(video_dir.c_str());
	if (!my_existsdir(themes_path.c_str()))
		my_mkdir(themes_path.c_str());
    std::string default_theme_file = themes_path + "Default.theme";
	if (!my_existsfile2(default_theme_file.c_str()))
	{
		load_default_theme();
		save_theme("Default.theme");
	}
}

static bool locate_amiberry_conf(const bool portable_mode)
{
	config_path = get_config_directory(portable_mode);
#ifdef __MACH__
	if constexpr (true)
#else
	if (portable_mode)
#endif
	{
		amiberry_conf_file = config_path + "/amiberry.conf";
		amiberry_ini_file = config_path + "/amiberry.ini";
	}
	else
	{
#ifdef __MACH__
		const std::string amiberry_dir = "Amiberry";
#else
		const std::string amiberry_dir = "amiberry";
#endif
		std::string xdg_config_home = get_xdg_config_home();
		if (!my_existsdir(xdg_config_home.c_str()))
			my_mkdir(xdg_config_home.c_str());
		xdg_config_home += "/" + amiberry_dir;
		if (!my_existsdir(xdg_config_home.c_str()))
			my_mkdir(xdg_config_home.c_str());

		amiberry_conf_file = xdg_config_home + "/amiberry.conf";
		amiberry_ini_file = xdg_config_home + "/amiberry.ini";
	}
	return my_existsfile2(amiberry_conf_file.c_str());
}

static void init_amiberry_dirs(const bool portable_mode)
{
#ifdef __MACH__
	const std::string amiberry_dir = "Amiberry";
#else
	const std::string amiberry_dir = "amiberry";
#endif
	current_dir = home_dir = get_home_directory(portable_mode);
    data_dir = get_data_directory(portable_mode);
    plugins_dir = get_plugins_directory(portable_mode);

#ifdef __MACH__
	if constexpr (true)
#else
	if (portable_mode)
#endif
	{
		themes_path = config_path;

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
		if (!my_existsdir(xdg_data_home.c_str()))
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
		if (!my_existsdir(xdg_data_home.c_str()))
			my_mkdir(xdg_data_home.c_str());

		std::string xdg_config_home = get_xdg_config_home();
		if (!my_existsdir(xdg_config_home.c_str()))
			my_mkdir(xdg_config_home.c_str());
		xdg_config_home += "/" + amiberry_dir;
		if (!my_existsdir(xdg_config_home.c_str()))
			my_mkdir(xdg_config_home.c_str());
		themes_path = xdg_config_home;

		// These paths are relative to the XDG_DATA_HOME directory
		controllers_path = whdboot_path = saveimage_dir = 
		ripper_path = input_dir = nvram_dir = video_dir =
		xdg_data_home;

		// These go in $HOME/Amiberry by default
		whdload_arch_path = floppy_path = harddrive_path = screenshot_dir =
		savestate_dir = cdrom_path = logfile_path = rom_path = rp9_path =
		home_dir;
	}

#ifdef __MACH__
    controllers_path.append("/Controllers/");
    whdboot_path.append("/Whdboot/");
    whdload_arch_path.append("/Lha/");
    floppy_path.append("/Floppies/");
    harddrive_path.append("/Harddrives/");
    cdrom_path.append("/CDROMs/");
    logfile_path.append("/Amiberry.log");
    rom_path.append("/Roms/");
    rp9_path.append("/RP9/");
    saveimage_dir.append("/");
    savestate_dir.append("/Savestates/");
    ripper_path.append("/Ripper/");
    input_dir.append("/Inputrecordings/");
    screenshot_dir.append("/Screenshots/");
    nvram_dir.append("/Nvram/");
    video_dir.append("/Videos/");
	themes_path.append("/Themes/");
#else
	controllers_path.append("/controllers/");
	whdboot_path.append("/whdboot/");
	whdload_arch_path.append("/lha/");
	floppy_path.append("/floppies/");
	harddrive_path.append("/harddrives/");
	cdrom_path.append("/cdroms/");
	logfile_path.append("/amiberry.log");
	rom_path.append("/roms/");
	rp9_path.append("/rp9/");
	saveimage_dir.append("/");
	savestate_dir.append("/savestates/");
	ripper_path.append("/ripper/");
	input_dir.append("/inputrecordings/");
	screenshot_dir.append("/screenshots/");
	nvram_dir.append("/nvram/");
	video_dir.append("/videos/");
	themes_path.append("/themes/");
#endif

	retroarch_file = config_path;
	retroarch_file.append("/retroarch.cfg");

	floppy_sounds_dir = data_dir;
	floppy_sounds_dir.append("floppy_sounds/");
}

void load_amiberry_settings()
{
	auto* const fh = zfile_fopen(amiberry_conf_file.c_str(), _T("r"), ZFD_NORMAL);
	if (fh)
	{
		char linea[CONFIG_BLEN];

		while (zfile_fgetsa(linea, sizeof linea, fh) != nullptr)
		{
			trim_wsa(linea);
			if (strlen(linea) > 0)
				parse_amiberry_settings_line(amiberry_conf_file.c_str(), linea);
		}
		zfile_fclose(fh);
	}
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
	system("sudo poweroff");
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
#ifdef _WIN32
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

int main(int argc, char* argv[])
{
	for (auto i = 1; i < argc; i++) {
		if (_tcscmp(argv[i], _T("-h")) == 0 || _tcscmp(argv[i], _T("--help")) == 0)
			usage();
	}

	struct sigaction action{};
	mainthreadid = uae_thread_get_id(nullptr);

	if(argc == 2)
	{
		const std::string two(argv[1]);
		if(two == "-v" || two == "--version")
		{
			print_version();
		}
	}

#ifdef USE_DBUS
	DBusSetup();
#endif

	max_uae_width = 8192;
	max_uae_height = 8192;

	// Parse command line to possibly set amiberry_config.
	// Do not remove used args yet.
	if (!parse_amiberry_cmd_line(&argc, argv, 0))
	{
		printf("Error in Amiberry command line option parsing.\n");
		usage();
		abort();
	}
	// Check if a file with the name "amiberry.portable" exists in the current directory
	// If it does, we will set portable_mode to true
	const bool portable_mode = my_existsfile2("amiberry.portable");
	const bool config_found = locate_amiberry_conf(portable_mode);

	init_amiberry_dirs(portable_mode);
	if (config_found)
	{
		load_amiberry_settings();
	}
	create_missing_amiberry_folders();

	// Parse command line and remove used amiberry specific args
	// and modify both argc & argv accordingly
	if (!parse_amiberry_cmd_line(&argc, argv, 1))
	{
		printf("Error in Amiberry command line option parsing.\n");
		usage();
		abort();
	}

	reginitializeinit(&inipath);
	if (getregmode() == nullptr)
	{
		const std::string ini_file_path = get_ini_file_path();
		TCHAR* path = my_strdup(ini_file_path.c_str());;
		auto f = fopen(path, _T("r"));
		if (!f)
			f = fopen(path, _T("w"));
		if (f) {
			fclose(f);
			reginitializeinit(&path);
		}
		xfree(path);
	}

	logging_init();
#if defined (CPU_arm)
	memset(&action, 0, sizeof action);
	action.sa_sigaction = signal_segv;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGSEGV, &action, nullptr) < 0)
	{
		printf("Failed to set signal handler (SIGSEGV).\n");
		abort();
	}
	if (sigaction(SIGILL, &action, nullptr) < 0)
	{
		printf("Failed to set signal handler (SIGILL).\n");
		abort();
	}

	memset(&action, 0, sizeof action);
	action.sa_sigaction = signal_buserror;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGBUS, &action, nullptr) < 0)
	{
		printf("Failed to set signal handler (SIGBUS).\n");
		abort();
	}

	memset(&action, 0, sizeof action);
	action.sa_sigaction = signal_term;
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIGTERM, &action, nullptr) < 0)
	{
		printf("Failed to set signal handler (SIGTERM).\n");
		abort();
	}
#endif

	if (!init_mmtimer())
		return 0;
	uae_time_calibrate();
	
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		write_log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		abort();
	}
#ifdef USE_OPENGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
#endif
	(void)atexit(SDL_Quit);

	read_rom_list(true);
	load_keyring(nullptr, nullptr);
	write_log(_T("Enumerating display devices.. \n"));
	enumeratedisplays();
	write_log(_T("Sorting devices and modes...\n"));
	sortdisplays();
	enumerate_sound_devices();
	for (int i = 0; i < MAX_SOUND_DEVICES && sound_devices[i]; i++) {
		const int type = sound_devices[i]->type;
		write_log(_T("%d:%s: %s\n"), i, type == SOUND_DEVICE_SDL2 ? _T("SDL2") : (type == SOUND_DEVICE_DS ? _T("DS") : (type == SOUND_DEVICE_AL ? _T("AL") : (type == SOUND_DEVICE_WASAPI ? _T("WA") : (type == SOUND_DEVICE_WASAPI_EXCLUSIVE ? _T("WX") : _T("PA"))))), sound_devices[i]->name);
	}
	write_log(_T("Enumerating recording devices:\n"));
	for (int i = 0; i < MAX_SOUND_DEVICES && record_devices[i]; i++) {
		const int type = record_devices[i]->type;
		write_log(_T("%d:%s: %s\n"), i, type == SOUND_DEVICE_SDL2 ? _T("SDL2") : (type == SOUND_DEVICE_DS ? _T("DS") : (type == SOUND_DEVICE_AL ? _T("AL") : (type == SOUND_DEVICE_WASAPI ? _T("WA") : (type == SOUND_DEVICE_WASAPI_EXCLUSIVE ? _T("WX") : _T("PA"))))), record_devices[i]->name);
	}
	write_log(_T("done\n"));

	normalcursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	clipboard_init();

#ifndef __MACH__
	// set capslock state based upon current "real" state
	ioctl(0, KDGKBLED, &kbd_flags);
	ioctl(0, KDGETLED, &kbd_led_status);
	if (kbd_flags & 07 & LED_CAP)
	{
		// record capslock pressed
		kbd_led_status |= LED_CAP;
		inputdevice_do_keyboard(AK_CAPSLOCK, 1);
	}
	else
	{
		// record capslock as not pressed
		kbd_led_status &= ~LED_CAP;
		inputdevice_do_keyboard(AK_CAPSLOCK, 0);
	}
	ioctl(0, KDSETLED, kbd_led_status);
#else
	// I tried to use Apple IO KIT to poll the status of the various leds, but it requires a special input reading permission (that the user needs to explicitly allow
	// for the app on launch) and in addition to that it doesn't consistently work well enough, more often than not we'll get a permission denied from the IOKIT framework 
	// which causes the app to silently fail anyway.  I can't find recent examples for macOS that work well enough and I feel it's not good to rely on a framework that
	// doesn't work all the time

	// We'll just call SDL and do a rudimentary state check instead
	int caps = SDL_GetModState();
		caps = caps & KMOD_CAPS;
	if(caps == KMOD_CAPS)
		// 0x04 is LED_CAP in the Linux file which is used here to trigger it
		kbd_led_status |= ~0x04;
	else
		kbd_led_status &= ~0x04;
#endif

#ifdef USE_GPIOD
	// Open GPIO chip
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
	real_main(argc, argv);

#ifdef USE_GPIOD
	// Release lines and chip
	gpiod_line_release(lineRed);
	gpiod_line_release(lineGreen);
	gpiod_line_release(lineYellow);

	gpiod_chip_close(chip);
#endif
#ifndef __MACH__
	// restore keyboard LEDs to normal state
	ioctl(0, KDSETLED, 0xFF);
#else
	// Unsolved for OS X
#endif

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

	SDL_Quit();

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

void drawbridge_update_profiles(uae_prefs* p)
{
#ifdef FLOPPYBRIDGE
	const unsigned int flags = (p->drawbridge_autocache ? 1 : 0) | (p->drawbridge_connected_drive_b & 1) << 1 | (p->drawbridge_serial_auto ? 4 : 0) | (p->drawbridge_smartspeed ? 8 : 0);

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
	std::ofstream out_file(filename);
	if (!out_file) {
		std::cerr << "Unable to open file " << filename << '\n';
		return;
	}

	out_file << "button=";
	for (const auto& b : input.button) {
		out_file << b << " ";
	}
	out_file << "\naxis=";
	for (const auto& a : input.axis) {
		out_file << a << " ";
	}
	out_file << "\nlstick_axis_y_invert=" << input.lstick_axis_y_invert;
	out_file << "\nlstick_axis_x_invert=" << input.lstick_axis_x_invert;
	out_file << "\nrstick_axis_y_invert=" << input.rstick_axis_y_invert;
	out_file << "\nrstick_axis_x_invert=" << input.rstick_axis_x_invert;
	out_file << "\nhotkey_button=" << input.hotkey_button;
	out_file << "\nquit_button=" << input.quit_button;
	out_file << "\nreset_button=" << input.reset_button;
	out_file << "\nmenu_button=" << input.menu_button;
	out_file << "\nload_state_button=" << input.load_state_button;
	out_file << "\nsave_state_button=" << input.save_state_button;
	out_file << "\nvkbd_button=" << input.vkbd_button;
	out_file << "\nnumber_of_hats=" << input.number_of_hats;
	out_file << "\nnumber_of_axis=" << input.number_of_axis;
	out_file << "\nis_retroarch=" << input.is_retroarch;
	out_file << "\namiberry_custom_none=";
	for (const auto& b : input.amiberry_custom_none) {
		out_file << b << " ";
	}
	out_file << "\namiberry_custom_hotkey=";
	for (const auto& b : input.amiberry_custom_hotkey) {
		out_file << b << " ";
	}
	out_file << "\namiberry_custom_axis_none=";
	for (const auto& a : input.amiberry_custom_axis_none) {
		out_file << a << " ";
	}
	out_file << "\namiberry_custom_axis_hotkey=";
	for (const auto& a : input.amiberry_custom_axis_hotkey) {
		out_file << a << " ";
	}

	out_file.close();
}

void read_controller_mapping_from_file(controller_mapping& input, const std::string& filename)
{
	std::ifstream in_file(filename);
	if (!in_file) {
		std::cerr << "Unable to open file " << filename << '\n';
		return;
	}

	std::string line;
	while (std::getline(in_file, line)) {
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

	in_file.close();
}

std::vector<std::string> get_cd_drives()
{
	char path[MAX_DPATH];
	std::vector<std::string> results{};
#ifndef __MACH__
	FILE* fp = popen("lsblk -o NAME,TYPE | grep 'rom' | awk '{print \"/dev/\" $1}'", "r");
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
	return results;
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
		const TCHAR* p1 = NULL;
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
