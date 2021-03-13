/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */

#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <ctime>
#include <csignal>

#include <algorithm>
#ifndef ANDROID
#include <execinfo.h>
#endif

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
#include <parser.h>
#include "amiberry_input.h"
#include "clipboard.h"
#include "fsdb.h"
#include "scsidev.h"
#include "threaddep/thread.h"
#include "uae/uae.h"

#ifdef AHI
#include "ahi_v1.h"
#ifdef AHI_v2
#include "ahi_v2.h"
#endif
#endif

int log_scsi;
int uaelib_debug;
int pissoff_value = 15000 * CYCLE_UNIT;

extern FILE* debugfile;
SDL_Cursor* normalcursor;

int pause_emulation;

static int sound_closed;
static int recapture;
static int focus;
static int mouseinside;
int mouseactive;
int minimized;
int monitor_off;

int quickstart_model = 0;
int quickstart_conf = 0;
bool host_poweroff = false;
int saveimageoriginalpath = 0;

struct amiberry_options amiberry_options = {};
amiberry_hotkey enter_gui_key;
SDL_GameControllerButton enter_gui_button;
amiberry_hotkey quit_key;
amiberry_hotkey action_replay_key;
amiberry_hotkey fullscreen_key;
amiberry_hotkey minimize_key;

bool lctrl, rctrl, lalt, ralt, lshift, rshift, lgui, rgui;

bool mouse_grabbed = false;

std::string get_version_string()
{
	std::string label_text = AMIBERRYVERSION;
	return label_text;
}

amiberry_hotkey get_hotkey_from_config(std::string config_option)
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
		if (token.compare(lctrl) == 0)
			hotkey.modifiers.lctrl = true;
		if (token.compare(rctrl) == 0)
			hotkey.modifiers.rctrl = true;
		if (token.compare(lalt) == 0)
			hotkey.modifiers.lalt = true;
		if (token.compare(ralt) == 0)
			hotkey.modifiers.ralt = true;
		if (token.compare(lshift) == 0)
			hotkey.modifiers.lshift = true;
		if (token.compare(rshift) == 0)
			hotkey.modifiers.rshift = true;
		if (token.compare(lgui) == 0)
			hotkey.modifiers.lgui = true;
		if (token.compare(rgui) == 0)
			hotkey.modifiers.rgui = true;

		config_option.erase(0, pos + delimiter.length());
	}
	hotkey.key_name = config_option;
	hotkey.scancode = SDL_GetScancodeFromName(hotkey.key_name.c_str());
	
	return hotkey;
}


void set_key_configs(struct uae_prefs* p)
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
	if (enter_gui_button == SDL_CONTROLLER_BUTTON_INVALID)
		enter_gui_button = SDL_CONTROLLER_BUTTON_GUIDE;
	
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
}

extern void signal_segv(int signum, siginfo_t* info, void* ptr);
extern void signal_buserror(int signum, siginfo_t* info, void* ptr);
extern void signal_term(int signum, siginfo_t* info, void* ptr);

extern void SetLastActiveConfig(const char* filename);

char start_path_data[MAX_DPATH];
char current_dir[MAX_DPATH];

#include <linux/kd.h>
#include <sys/ioctl.h>
unsigned char kbd_led_status;
char kbd_flags;

static char config_path[MAX_DPATH];
static char rom_path[MAX_DPATH];
static char rp9_path[MAX_DPATH];
static char controllers_path[MAX_DPATH];
static char retroarch_file[MAX_DPATH];
static char logfile_path[MAX_DPATH];
static char floppy_sounds_dir[MAX_DPATH];

char last_loaded_config[MAX_DPATH] = {'\0'};

int max_uae_width;
int max_uae_height;

extern "C" int main(int argc, char* argv[]);

void sleep_micros (int ms)
{
	usleep(ms);
}

void sleep_millis(int ms)
{
	usleep(ms * 1000);
}

int sleep_millis_main(int ms)
{
	const auto start = read_processor_time();
	usleep(ms * 1000);
	idletime += read_processor_time() - start;
	return 0;
}

static void setcursor(struct AmigaMonitor* mon, int oldx, int oldy)
{
	int dx = (mon->amigawinclip_rect.x - mon->amigawin_rect.x) + (mon->amigawinclip_rect.w - mon->amigawinclip_rect.x) / 2;
	int dy = (mon->amigawinclip_rect.y - mon->amigawin_rect.y) + (mon->amigawinclip_rect.h - mon->amigawinclip_rect.y) / 2;
	mon->mouseposx = oldx - dx;
	mon->mouseposy = oldy - dy;

	mon->windowmouse_max_w = (mon->amigawinclip_rect.w - mon->amigawinclip_rect.x) / 2 - 50;
	mon->windowmouse_max_h = (mon->amigawinclip_rect.h - mon->amigawinclip_rect.y) / 2 - 50;
	if (mon->windowmouse_max_w < 10)
		mon->windowmouse_max_w = 10;
	if (mon->windowmouse_max_h < 10)
		mon->windowmouse_max_h = 10;

	if ((currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) && currprefs.input_tablet > 0 && mousehack_alive() && isfullscreen() <= 0) {
		mon->mouseposx = mon->mouseposy = 0;
		return;
	}

	if (oldx >= 30000 || oldy >= 30000 || oldx <= -30000 || oldy <= -30000) {
		oldx = oldy = 0;
	}
	else {
		if (abs(mon->mouseposx) < mon->windowmouse_max_w && abs(mon->mouseposy) < mon->windowmouse_max_h)
			return;
	}
	mon->mouseposx = mon->mouseposy = 0;
	if (oldx < 0 || oldy < 0 || oldx > mon->amigawin_rect.w - mon->amigawin_rect.x || oldy > mon->amigawin_rect.h - mon->amigawin_rect.y) {
		write_log(_T("Mouse out of range: mon=%d %dx%d (%dx%d %dx%d)\n"), mon->monitor_id, oldx, oldy,
			mon->amigawin_rect.x, mon->amigawin_rect.y, mon->amigawin_rect.w, mon->amigawin_rect.h);
		return;
	}
	int cx = (mon->amigawinclip_rect.w - mon->amigawinclip_rect.x) / 2 + mon->amigawin_rect.x + (mon->amigawinclip_rect.x - mon->amigawin_rect.x);
	int cy = (mon->amigawinclip_rect.h - mon->amigawinclip_rect.y) / 2 + mon->amigawin_rect.y + (mon->amigawinclip_rect.y - mon->amigawin_rect.y);

	SDL_WarpMouseInWindow(mon->sdl_window, cx, cy);
}

static int mon_cursorclipped;
static int pausemouseactive;
void resumesoundpaused(void)
{
	resume_sound();
#ifdef AHI
	ahi_open_sound();
#ifdef AHI_v2
	ahi2_pause_sound(0);
#endif
#endif
}

void setsoundpaused(void)
{
	pause_sound();
#ifdef AHI
	ahi_close_sound();
#ifdef AHI_v2
	ahi2_pause_sound(1);
#endif
#endif
}

bool resumepaused(int priority)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	if (pause_emulation > priority)
		return false;
	if (!pause_emulation)
		return false;
	devices_unpause();
	resumesoundpaused();
	if (pausemouseactive)
	{
		pausemouseactive = 0;
		setmouseactive(mon->monitor_id, isfullscreen() > 0 ? 1 : -1);
	}
	pause_emulation = 0;
	setsystime();
	wait_keyrelease();
	return true;
}

bool setpaused(int priority)
{
	struct AmigaMonitor* mon = &AMonitors[0];
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

void setminimized(int monid)
{
	if (!minimized)
		minimized = 1;
	set_inhibit_frame(monid, IHF_WINDOWHIDDEN);
}

void unsetminimized(int monid)
{
	if (minimized > 0)
		full_redraw_all();
	minimized = 0;
	clear_inhibit_frame(monid, IHF_WINDOWHIDDEN);
}

void setpriority(int prio)
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

static void setcursorshape(int monid)
{
	if (currprefs.input_tablet && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) && currprefs.input_magic_mouse_cursor == MAGICMOUSE_NATIVE_ONLY) {
		if (SDL_GetCursor() != NULL)
			SDL_ShowCursor(SDL_DISABLE);
	}
	else if (!picasso_setwincursor(monid)) {
		if (SDL_GetCursor() != normalcursor)
		{
			SDL_SetCursor(normalcursor);
			SDL_ShowCursor(SDL_ENABLE);
		}
	}
}

void releasecapture(struct AmigaMonitor* mon)
{
	if (!mon_cursorclipped)
		return;
	SDL_SetWindowGrab(mon->sdl_window, SDL_FALSE);
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);
	mon_cursorclipped = 0;
}

void updatewinrect(struct AmigaMonitor* mon, bool allowfullscreen)
{
	int f = isfullscreen();
	if (!allowfullscreen && f > 0)
		return;
	SDL_GetWindowPosition(mon->sdl_window, &mon->amigawin_rect.x, &mon->amigawin_rect.y);
	SDL_GetWindowSize(mon->sdl_window, &mon->amigawin_rect.w, &mon->amigawin_rect.h);
	SDL_GetWindowPosition(mon->sdl_window, &mon->amigawinclip_rect.x, &mon->amigawinclip_rect.y);
	SDL_GetWindowSize(mon->sdl_window, &mon->amigawinclip_rect.w, &mon->amigawinclip_rect.h);
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

//TODO: Tablet only
void target_inputdevice_unacquire(void)
{
//	close_tablet(tablet);
//	tablet = NULL;
}
void target_inputdevice_acquire(void)
{
//	struct AmigaMonitor* mon = &AMonitors[0];
//	target_inputdevice_unacquire();
//	tablet = open_tablet(mon->hAmigaWnd);
}

static void setmouseactive2(struct AmigaMonitor* mon, int active, bool allowpause)
{
	if (active == 0)
		releasecapture(mon);
	if (mouseactive == active && active >= 0)
		return;

	if (active == 1 && !(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		auto* c = SDL_GetCursor();
		if (c != normalcursor)
			return;
	}
	
	if (active < 0)
		active = 1;

	mouseactive = active ? mon->monitor_id + 1 : 0;

	mon->mouseposx = mon->mouseposy = 0;
	releasecapture(mon);
	recapture = 0;

	if (isfullscreen() <= 0 && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) && currprefs.input_tablet > 0) {
		if (mousehack_alive())
		{
			if (active) setcursorshape(mon->monitor_id);
			return;
		}
		SDL_SetCursor(normalcursor);
	}

	if (mouseactive > 0)
		focus = mon->monitor_id + 1;

	if (mouseactive) {
		if (focus) {
			SDL_RaiseWindow(mon->sdl_window);
			if (!mon_cursorclipped) {
				SDL_ShowCursor(SDL_DISABLE);
				SDL_SetWindowGrab(mon->sdl_window, SDL_TRUE);
				updatewinrect(mon, false);
				SDL_SetRelativeMouseMode(SDL_TRUE);
				mon_cursorclipped = mon->monitor_id + 1;
			}
			setcursor(mon, -30000, -30000);
		}
		wait_keyrelease();
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

void setmouseactive(int monid, int active)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	monitor_off = 0;
	if (active > 1)
		SDL_RaiseWindow(mon->sdl_window);
	setmouseactive2(mon, active, true);
}

static void amiberry_active(struct AmigaMonitor* mon, int minimized)
{
	monitor_off = 0;
	
	focus = 1;
	auto pri = currprefs.inactive_priority;
	if (!minimized)
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

static void amiberry_inactive(struct AmigaMonitor* mon, int minimized)
{
	focus = 0;
	recapture = 0;
	wait_keyrelease();
	setmouseactive(mon->monitor_id, 0);
	clipboard_active(1, 0);
	auto pri = currprefs.inactive_priority;
	if (!quit_program) {
		if (minimized) {
			pri = currprefs.minimized_priority;
			if (currprefs.minimized_pause) {
				inputdevice_unacquire();
				setpaused(1);
				sound_closed = 1;
			}
			else if (currprefs.minimized_nosound) {
				inputdevice_unacquire(true, currprefs.minimized_input);
				setsoundpaused();
				sound_closed = -1;
			}
			else {
				inputdevice_unacquire(true, currprefs.minimized_input);
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
				inputdevice_unacquire(true, currprefs.inactive_input);
				setsoundpaused();
				sound_closed = -1;
			}
			else {
				inputdevice_unacquire(true, currprefs.inactive_input);
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

void minimizewindow(int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	if (mon->sdl_window)
		SDL_MinimizeWindow(mon->sdl_window);
}

void enablecapture(int monid)
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
	if (currprefs.active_nocapture_pause && sound_closed == 0) {
		setpaused(2);
		sound_closed = 1;
	}
	else if (currprefs.active_nocapture_nosound && sound_closed == 0) {
		setsoundpaused();
		sound_closed = -1;
	}
}

void setmouseactivexy(int monid, int x, int y, int dir)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	int diff = 8;

	if (isfullscreen() > 0)
		return;
	x += mon->amigawin_rect.x;
	y += mon->amigawin_rect.y;
	if (dir & 1)
		x = mon->amigawin_rect.x - diff;
	if (dir & 2)
		x = mon->amigawin_rect.w + diff;
	if (dir & 4)
		y = mon->amigawin_rect.y - diff;
	if (dir & 8)
		y = mon->amigawin_rect.h + diff;
	if (!dir) {
		x += (mon->amigawin_rect.w - mon->amigawin_rect.x) / 2;
		y += (mon->amigawin_rect.h - mon->amigawin_rect.y) / 2;
	}
	//if (isfullscreen() < 0) {
	//	POINT pt;
	//	pt.x = x;
	//	pt.y = y;
	//	if (MonitorFromPoint(pt, MONITOR_DEFAULTTONULL) == NULL)
	//		return;
	//}
	if (mouseactive) {
		disablecapture();
		SDL_WarpMouseInWindow(mon->sdl_window, x, y);
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

static void activationtoggle(int monid, bool inactiveonly)
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
		if (media_insert_queue[i] != NULL) {
			if (!_tcsicmp(drvname, media_insert_queue[i]))
				return i;
		}
	}
	return -1;
}

static void start_media_insert_timer();

Uint32 timer_callbackfunc(Uint32 interval, void* param)
{
	if ((*(int*)&param) == 2) {
		bool restart = false;
		SDL_RemoveTimer(media_change_timer);
		media_change_timer = 0;

		for (int i = 0; i < MEDIA_INSERT_QUEUE_SIZE; i++) {
			if (media_insert_queue[i]) {
				TCHAR* drvname = media_insert_queue[i];
				int r = my_getvolumeinfo(drvname);
				if (r < 0) {
					if (media_insert_queue_type[i] > 0) {
						write_log(_T("Mounting %s but drive is not ready, %d.. retrying %d..\n"), drvname, r, media_insert_queue_type[i]);
						media_insert_queue_type[i]--;
						restart = true;
						continue;
					}
					else {
						write_log(_T("Mounting %s but drive is not ready, %d.. aborting..\n"), drvname, r);
					}
				}
				else {
					int inserted = 1;
					//DWORD type = GetDriveType(drvname);
					//if (type == DRIVE_CDROM)
					//	inserted = -1;
					r = filesys_media_change(drvname, inserted, NULL);
					if (r < 0) {
						write_log(_T("Mounting %s but previous media change is still in progress..\n"), drvname);
						restart = true;
						break;
					}
					else if (r > 0) {
						write_log(_T("%s mounted\n"), drvname);
					}
					else {
						write_log(_T("%s mount failed\n"), drvname);
					}
				}
				xfree(media_insert_queue[i]);
				media_insert_queue[i] = NULL;
			}
		}

		if (restart)
			start_media_insert_timer();
	}
	else if ((*(int*)&param) == 4) {
		SDL_RemoveTimer(device_change_timer);
		device_change_timer = 0;
		inputdevice_devicechange(&changed_prefs);
		inputdevice_copyjports(&changed_prefs, &currprefs);
	}
	else if ((*(int*)&param) == 1) {
#ifdef PARALLEL_PORT
		finishjob();
#endif
	}
	return 0;
}

static void start_media_insert_timer()
{
	if (!media_change_timer) {
		media_change_timer = SDL_AddTimer(1000, timer_callbackfunc, (void*)2);
	}
}

static void add_media_insert_queue(const TCHAR* drvname, int retrycnt)
{
	int idx = is_in_media_queue(drvname);
	if (idx >= 0) {
		if (retrycnt > media_insert_queue_type[idx])
			media_insert_queue_type[idx] = retrycnt;
		write_log(_T("%s already queued for insertion, cnt=%d.\n"), drvname, retrycnt);
		start_media_insert_timer();
		return;
	}
	for (int i = 0; i < MEDIA_INSERT_QUEUE_SIZE; i++) {
		if (media_insert_queue[i] == NULL) {
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
static struct touch_store touches[MAX_TOUCHES];

static void touch_release(struct touch_store* ts, const SDL_Rect* rcontrol)
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

static void tablet_touch(unsigned long id, int pressrel, int x, int y, const SDL_Rect* rcontrol)
{
	struct touch_store* ts = NULL;
	int buttony = rcontrol->h - (rcontrol->h - rcontrol->y) / 4;

	int new_slot = -1;
	for (int i = 0; i < MAX_TOUCHES; i++) {
		struct touch_store* tts = &touches[i];
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
						int button = x > r->x + (r->w - r->x) / 2 ? 1 : 0;
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

static void touch_event(unsigned long id, int pressrel, int x, int y, const SDL_Rect* rcontrol)
{
	// No lightpen support (yet?)
	tablet_touch(id, pressrel, x, y, rcontrol);
}

void process_event(SDL_Event event)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	
	if (event.type == SDL_WINDOWEVENT)
	{
		switch (event.window.event)
		{
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			focus = 1;
			amiberry_active(mon, minimized);
			unsetminimized(mon->monitor_id);
			return;
		case SDL_WINDOWEVENT_MINIMIZED:
			if (!minimized)
			{
				write_log(_T("SIZE_MINIMIZED\n"));
				setminimized(mon->monitor_id);
				amiberry_inactive(mon, minimized);
			}
			return;
		case SDL_WINDOWEVENT_RESTORED:
			amiberry_active(mon, minimized);
			unsetminimized(mon->monitor_id);
			return;
		case SDL_WINDOWEVENT_MOVED:
			if (isfullscreen() <= 0)
			{
				updatewinrect(mon, false);
				//updatemouseclip(0);
			}
			break;
		case SDL_WINDOWEVENT_ENTER:
			mouseinside = true;
			if (currprefs.input_tablet > 0 && currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC && isfullscreen() <= 0)
			{
				if (mousehack_alive())
					setcursorshape(mon->monitor_id);
			}
			return;
		case SDL_WINDOWEVENT_LEAVE:
			mouseinside = false;
			return;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			focus = 0;
			amiberry_inactive(mon, minimized);
			if (isfullscreen() <= 0 && currprefs.minimize_inactive)
				minimizewindow(mon->monitor_id);
			return;
		case SDL_WINDOWEVENT_CLOSE:
			if (device_change_timer)
				SDL_RemoveTimer(device_change_timer);
			device_change_timer = 0;
			wait_keyrelease();
			inputdevice_unacquire();
			uae_quit();
			return;
		default:
			break;
		}
	}
	else
	switch (event.type)
	{
	case SDL_QUIT:
		uae_quit();
		return;

	case SDL_CLIPBOARDUPDATE:
		if (currprefs.clipboard_sharing && SDL_HasClipboardText() == SDL_TRUE)
		{
			auto* clipboard_host = SDL_GetClipboardText();
			uae_clipboard_put_text(clipboard_host);
			SDL_free(clipboard_host);	
		}
		return;
		
	case SDL_JOYDEVICEADDED:
	case SDL_CONTROLLERDEVICEADDED:
	case SDL_JOYDEVICEREMOVED:
	case SDL_CONTROLLERDEVICEREMOVED:
		write_log("SDL Controller/Joystick device added or removed! Re-running import joysticks...\n");
		import_joysticks();
		if (device_change_timer)
			SDL_RemoveTimer(device_change_timer);
		device_change_timer = SDL_AddTimer(2000, timer_callbackfunc, (void*)4);	
		return;

	case SDL_CONTROLLERBUTTONDOWN:
		if (event.cbutton.button == enter_gui_button)
		{
			inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
			break;
		}
		return;

	case SDL_KEYDOWN:
		// if the key belongs to a "retro arch joystick" ignore it
		// ONLY when in game though, we need to remove the joysticks really 
		// if we want to use the KB
		// i've added this so when using the joysticks it doesn't hit the 'r' key for some games
		// which starts a replay!!!
		// 
		//const auto ok_to_use = !key_used_by_retroarch_joy(event.key.keysym.scancode);
		//if (ok_to_use && event.key.repeat == 0)
		if (event.key.repeat == 0)
		{
			if (!isfocus())
				break;
			if (isfocus() < 2 && currprefs.input_tablet >= TABLET_MOUSEHACK && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC))
				break;
			
			int scancode = event.key.keysym.scancode;
			int pressed = 1;

			if ((amiberry_options.rctrl_as_ramiga || currprefs.right_control_is_right_win_key)
				&& scancode == SDL_SCANCODE_RCTRL)
			{
				scancode = SDL_SCANCODE_RGUI;
			}
			scancode = keyhack(scancode, pressed, 0);
			my_kbd_handler(0, scancode, pressed, false);
		}
		return;

	case SDL_KEYUP:
		//const auto ok_to_use = !key_used_by_retroarch_joy(event.key.keysym.scancode);
		//if (ok_to_use && event.key.repeat == 0)
		if (event.key.repeat == 0)
		{
			if (!isfocus())
				break;
			if (isfocus() < 2 && currprefs.input_tablet >= TABLET_MOUSEHACK && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC))
				break;
			
			int scancode = event.key.keysym.scancode;
			int pressed = 0;

			if ((amiberry_options.rctrl_as_ramiga || currprefs.right_control_is_right_win_key)
				&& scancode == SDL_SCANCODE_RCTRL)
			{
				scancode = SDL_SCANCODE_RGUI;
			}
			scancode = keyhack(scancode, pressed, 0);
			my_kbd_handler(0, scancode, pressed, true);
		}
		break;

	case SDL_FINGERDOWN:
		if (isfocus())
		{
			if (event.button.clicks == 2)
				setmousebuttonstate(0, 0, 1);
		}
		return;

	case SDL_MOUSEBUTTONDOWN:
		if (event.button.button == SDL_BUTTON_LEFT
			&& !mouseactive
			&& (!mousehack_alive()
				|| currprefs.input_tablet != TABLET_MOUSEHACK
				|| (currprefs.input_tablet == TABLET_MOUSEHACK
					&& !(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC))
				|| isfullscreen() > 0))
		{
			if (!pause_emulation || currprefs.active_nocapture_pause)
				setmouseactive(mon->monitor_id, (event.button.clicks == 1 || isfullscreen() > 0) ? 2 : 1);
		}
		else if (isfocus())
		{
			switch (event.button.button)
			{
			case SDL_BUTTON_LEFT:
				setmousebuttonstate(0, 0, 1);
				break;
			case SDL_BUTTON_RIGHT:
				setmousebuttonstate(0, 1, 1);
				break;
			case SDL_BUTTON_MIDDLE:
				if (currprefs.input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON)
					activationtoggle(0, true);
				else
					setmousebuttonstate(0, 2, 1);
				break;
			case SDL_BUTTON_X1:
				setmousebuttonstate(0, 3, 1);
				break;
			case SDL_BUTTON_X2:
				setmousebuttonstate(0, 4, 1);
				break;
			default: break;
			}
		}
		return;

	case SDL_FINGERUP:
		if (isfocus())
		{
			setmousebuttonstate(0, 0, 0);
		}
		return;

	case SDL_MOUSEBUTTONUP:
		if (isfocus())
		{
			switch (event.button.button)
			{
			case SDL_BUTTON_LEFT:
				setmousebuttonstate(0, 0, 0);
				break;
			case SDL_BUTTON_RIGHT:
				setmousebuttonstate(0, 1, 0);
				break;
			case SDL_BUTTON_MIDDLE:
				if (!(currprefs.input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON))
					setmousebuttonstate(0, 2, 0);
				break;
			case SDL_BUTTON_X1:
				setmousebuttonstate(0, 3, 0);
				break;
			case SDL_BUTTON_X2:
				setmousebuttonstate(0, 4, 0);
				break;
			default: break;
			}
		}
		return;

	case SDL_FINGERMOTION:
		if (isfocus())
		{
			setmousestate(0, 0, event.motion.x, 1);
			setmousestate(0, 1, event.motion.y, 1);
		}
		return;

	case SDL_MOUSEMOTION:
	{
		int wm = event.motion.which;

		monitor_off = 0;
		if (!mouseinside)
			mouseinside = true;

#if 0
		write_log(_T("WM_MOUSEMOVE MON=%d NUM=%d ACT=%d FOCUS=%d CLIP=%d FS=%d %dx%d %dx%d\n"),
				mon->monitor_id, 0, mouseactive, focus, mon_cursorclipped, isfullscreen(), mx, my, mon->mouseposx, mon->mouseposy);
#endif
			
		if (recapture && isfullscreen() <= 0) {
			enablecapture(mon->monitor_id);
			return;
		}

		if (wm < 0 && currprefs.input_tablet >= TABLET_MOUSEHACK)
		{
			/* absolute */
			setmousestate(0, 0, event.motion.x, 1);
			setmousestate(0, 1, event.motion.y, 1);
			return;
		}

		if (wm >= 0)
		{
			if (currprefs.input_tablet >= TABLET_MOUSEHACK)
			{
				/* absolute */
				setmousestate(event.motion.which, 0, event.motion.x, 1);
				setmousestate(event.motion.which, 1, event.motion.y, 1);
				return;
			}
			if (!focus || !mouseactive)
				return;
			/* relative */
			setmousestate(event.motion.which, 0, event.motion.xrel, 0);
			setmousestate(event.motion.which, 1, event.motion.yrel, 0);
		}
		else if (isfocus() < 0 && currprefs.input_tablet >= TABLET_MOUSEHACK) {
			setmousestate(0, 0, event.motion.x, 1);
			setmousestate(0, 1, event.motion.y, 1);
		}
	}
	break;

	case SDL_MOUSEWHEEL:
		if (isfocus() > 0)
		{
			const auto val_y = event.wheel.y;
			setmousestate(0, 2, val_y, 0);
			if (val_y < 0)
				setmousebuttonstate(0, 3 + 0, -1);
			else if (val_y > 0)
				setmousebuttonstate(0, 3 + 1, -1);

			const auto val_x = event.wheel.x;
			setmousestate(0, 3, val_x, 0);
			if (val_x < 0)
				setmousebuttonstate(0, 3 + 2, -1);
			else if (val_x > 0)
				setmousebuttonstate(0, 3 + 3, -1);
		}
		return;

	default:
		break;
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

static int canstretch(struct AmigaMonitor* mon)
{
	if (isfullscreen() != 0)
		return 0;
	if (!mon->screen_is_picasso) {
		if (!currprefs.gfx_windowed_resize)
			return 0;
		if (currprefs.gf[APMODE_NATIVE].gfx_filter_autoscale == AUTOSCALE_RESIZE)
			return 0;
		return 1;
	}
	else {
		if (currprefs.rtgallowscaling || currprefs.gf[APMODE_RTG].gfx_filter_autoscale)
			return 1;
	}
	return 0;
}

int handle_msgpump()
{
	lctrl = rctrl = lalt = ralt = lshift = rshift = lgui = rgui = false;
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
	struct AmigaMonitor* mon = &AMonitors[0];
	static auto was_paused = 0;
	static int cnt1, cnt2;

	if (pause_emulation)
	{
		if (was_paused == 0)
		{
			setpaused(pause_emulation);
			was_paused = pause_emulation;
			gui_fps(0, 0, 0);
			gui_led(LED_SND, 0, -1);
			// we got just paused, report it to caller.
			return true;
		}
		lctrl = rctrl = lalt = ralt = lshift = rshift = lgui = rgui = false;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			process_event(event);
		}

		inputdevicefunc_keyboard.read();
		inputdevicefunc_mouse.read();
		inputdevicefunc_joystick.read();
		inputdevice_handle_inputcode();
		cnt1 = 0;
		cnt2--;
		if (cnt2 <= 0) {
			cnt2 = 10;
		}
	}
	if (was_paused && (!pause_emulation || quit_program))
	{
		updatedisplayarea(mon->monitor_id);
		pause_emulation = was_paused;
		resumepaused(was_paused);
		sound_closed = 0;
		was_paused = 0;
	}
	cnt1--;
	if (cnt1 <= 0) {
		uae_time_calibrate();
		//flush_log();
		cnt1 = 50 * 5;
		cnt2--;
		if (cnt2 <= 0) {
			cnt2 = 5;
		}
	}
	return pause_emulation != 0;
}

void logging_init()
{
	if (amiberry_options.write_logfile)
	{
		static int first = 0;
		char debug_filename[MAX_DPATH];

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

		sprintf(debug_filename, "%s", logfile_path);
		if (!debugfile)
			debugfile = fopen(debug_filename, "wt");

		first++;
		write_log("%s Logfile\n\n", get_version_string().c_str());
	}
}

void logging_cleanup(void)
{
	if (debugfile)
		fclose(debugfile);
	debugfile = nullptr;
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

int relativepaths = 0;

// convert path to absolute or relative
void fullpath(TCHAR* path, int size, bool userelative)
{
	// FIXME: forward/backslash fix needed
	if (path[0] == 0 || (path[0] == '\\' && path[1] == '\\') ||
		path[0] == ':') {
		return;
	}
	/* <drive letter>: is supposed to mean same as <drive letter>:\ */
#if 0
	if (path[0] == 0 || (path[0] == '\\' && path[1] == '\\') || path[0] == ':')
		return;
	// has one or more environment variables? do nothing.
	if (_tcschr(path, '%'))
		return;
	if (_tcslen(path) >= 2 && path[_tcslen(path) - 1] == '.')
		return;
	/* <drive letter>: is supposed to mean same as <drive letter>:\ */
	if (_istalpha(path[0]) && path[1] == ':' && path[2] == 0)
		_tcscat(path, _T("\\"));
	if (userelative) {
		TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
		tmp1[0] = 0;
		GetCurrentDirectory(sizeof tmp1 / sizeof(TCHAR), tmp1);
		fixtrailing(tmp1);
		tmp2[0] = 0;
		int ret = GetFullPathName(path, sizeof tmp2 / sizeof(TCHAR), tmp2, NULL);
		if (ret == 0 || ret >= sizeof tmp2 / sizeof(TCHAR))
			return;
		if (_tcslen(tmp1) > 2 && _tcsnicmp(tmp1, tmp2, 3) == 0 && tmp1[1] == ':' && tmp1[2] == '\\') {
			// same drive letter
			path[0] = 0;
			if (PathRelativePathTo(path, tmp1, FILE_ATTRIBUTE_DIRECTORY, tmp2, tmp2[_tcslen(tmp2) - 1] == '\\' ? FILE_ATTRIBUTE_DIRECTORY : 0)) {
				if (path[0]) {
					if (path[0] == '.' && path[1] == 0) {
						_tcscpy(path, _T(".\\"));
					}
					else if (path[0] == '\\') {
						_tcscpy(tmp1, path + 1);
						_stprintf(path, _T(".\\%s"), tmp1);
					}
					else if (path[0] != '.') {
						_tcscpy(tmp1, path);
						_stprintf(path, _T(".\\%s"), tmp1);
					}
				}
				else {
					_tcscpy(path, tmp2);
				}
				goto done;
			}
		}
		if (_tcsnicmp(tmp1, tmp2, _tcslen(tmp1)) == 0) {
			// tmp2 is inside tmp1
			_tcscpy(path, _T(".\\"));
			_tcscat(path, tmp2 + _tcslen(tmp1));
		}
		else {
			_tcscpy(path, tmp2);
		}
	done:;
	}
	else {
		TCHAR tmp[MAX_DPATH];
		_tcscpy(tmp, path);
		DWORD err = GetFullPathName(tmp, size, path, NULL);
	}
#endif
}

// convert path to absolute or relative
void fullpath(TCHAR* path, int size)
{
	fullpath(path, size, relativepaths);
}

bool target_isrelativemode(void)
{
	return relativepaths != 0;
}

bool same_path(const TCHAR* p1, const TCHAR* p2)
{
	if (!_tcsicmp(p1, p2))
		return true;
	return false;
}

void get_path_part(TCHAR* outpath, int size, const TCHAR* inpath)
{
	_tcscpy(outpath, inpath);
	auto* const p = _tcsrchr(outpath, '/');
	if (p)
		p[0] = 0;
	fix_trailing(outpath);
}

void get_file_part(TCHAR* out, int size, const TCHAR* path)
{
	out[0] = 0;
	const auto* const p = _tcsrchr(path, '/');
	if (p)
		_tcscpy(out, p + 1);
	else
		_tcscpy(out, path);
}

uae_u8* target_load_keyfile(struct uae_prefs* p, const char* path, int* sizep, char* name)
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
	struct AmigaMonitor* mon = &AMonitors[0];
	releasecapture(mon);

	write_log("Target_execute received: %s\n", command);
	const std::string cmd_string = command;
	auto cmd_split = split_command(cmd_string);

	for (unsigned int i = 0; i < cmd_split.size(); ++i)
	{
		if (i > 0)
		{
			const auto found = cmd_split[i].find(':');
			if (found != std::string::npos)
			{
				auto chunk = cmd_split[i].substr(0, found);
				for (auto mount = 0; mount < currprefs.mountitems; mount++)
				{
					if (currprefs.mountconfig[mount].ci.type == UAEDEV_DIR)
					{
						if (_tcsicmp(currprefs.mountconfig[mount].ci.volname, chunk.c_str()) == 0
							|| _tcsicmp(currprefs.mountconfig[mount].ci.devname, chunk.c_str()) == 0)
						{
							std::string tmp = currprefs.mountconfig[mount].ci.rootdir;
							chunk += ':';
							replace(cmd_split[i], chunk, tmp);
						}
					}
				}
			}
		}
	}

	std::string cmd_result;
	for (const auto& i : cmd_split)
	{
		cmd_result.append(i);
		cmd_result.append(" ");
	}
	// Ensure this runs in the background, otherwise we'll block the emulator until it returns
	cmd_result.append(" &");
	
	try
	{
		write_log("Executing: %s\n", cmd_result.c_str());
		system(cmd_result.c_str());
	}
	catch (...)
	{
		write_log("Exception thrown when trying to execute: %s", command);
	}
}

void target_run(void)
{
	// Reset counter for access violations
	init_max_signals();
}

void target_quit(void)
{
}

void target_fixup_options(struct uae_prefs* p)
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

	if (p->rtg_hardwaresprite && !p->gfx_api) {
		error_log(_T("Not RTG hardware sprite compatible."));
		p->rtg_hardwaresprite = false;
	}
	
#ifdef AMIBERRY
	p->rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;
	if (z3_base_adr == Z3BASE_REAL)
	{
		// map Z3 memory at real address (0x40000000)
		p->z3_mapping_mode = Z3MAPPING_REAL;
		p->z3autoconfig_start = z3_base_adr;
	}
	else
	{
		// map Z3 memory at UAE address (0x10000000)
		p->z3_mapping_mode = Z3MAPPING_UAE;
		p->z3autoconfig_start = z3_base_adr;
	}
#endif
	
	if (p->rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE) {
		p->rtg_hardwareinterrupt = false;
		p->rtg_hardwaresprite = false;
		p->rtgmatchdepth = false;
		p->color_mode = 5;
		//if (p->ppc_model && !p->gfx_api) {
		//	error_log(_T("Graphics board and PPC: Direct3D enabled."));
		//	p->gfx_api = os_win7 ? 2 : 1;
		//}
	}

	struct MultiDisplay* md = getdisplay(p, 0);
	for (int j = 0; j < MAX_AMIGADISPLAYS; j++) {
		if (p->gfx_monitor[j].gfx_size_fs.special == WH_NATIVE) {
			int i;
			for (i = 0; md->DisplayModes[i].depth >= 0; i++) {
				if (md->DisplayModes[i].res.width == md->rect.w - md->rect.x &&
					md->DisplayModes[i].res.height == md->rect.h - md->rect.y) {
					p->gfx_monitor[j].gfx_size_fs.width = md->DisplayModes[i].res.width;
					p->gfx_monitor[j].gfx_size_fs.height = md->DisplayModes[i].res.height;
					write_log(_T("Native resolution: %dx%d\n"), p->gfx_monitor[j].gfx_size_fs.width, p->gfx_monitor[j].gfx_size_fs.height);
					break;
				}
			}
			if (md->DisplayModes[i].depth < 0) {
				p->gfx_monitor[j].gfx_size_fs.special = 0;
				write_log(_T("Native resolution not found.\n"));
			}
		}
	}
	/* switch from 32 to 16 or vice versa if mode does not exist */
	if (1 || isfullscreen() > 0) {
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
	}

	set_key_configs(p);
}

void target_default_options(struct uae_prefs* p, int type)
{
	//TCHAR buf[MAX_DPATH];
	if (type == 2 || type == 0 || type == 3) {
		//p->logfile = 0;
		p->active_nocapture_pause = 0;
		p->active_nocapture_nosound = 0;
		p->minimized_nosound = 1;
		p->minimized_pause = 1;
		p->minimized_input = 0;
		p->inactive_nosound = 0;
		p->inactive_pause = 0;
		p->inactive_input = 0;
		//p->ctrl_F11_is_quit = 0;
		//p->soundcard = 0;
		//p->samplersoundcard = -1;
		p->minimize_inactive = 0;
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
		p->automount_removable = 0;
		//p->automount_drives = 0;
		//p->automount_removabledrives = 0;
		p->automount_cddrives = 0;
		//p->automount_netdrives = 0;
		//p->kbledmode = 1;
		p->uaescsimode = UAESCSI_CDEMU;
		p->borderless = false;
		p->blankmonitors = false;
		//p->powersavedisabled = true;
		p->sana2 = false;
		p->rtgmatchdepth = true;
		p->gf[APMODE_RTG].gfx_filter_autoscale = RTG_MODE_SCALE;
		p->rtgallowscaling = false;
		p->rtgscaleaspectratio = -1;
		p->rtgvblankrate = 0;
		p->rtg_hardwaresprite = true;
		//p->commandpathstart[0] = 0;
		//p->commandpathend[0] = 0;
		//p->statusbar = 1;
		p->gfx_api = 2;
		if (p->gfx_api > 1)
			p->color_mode = 5;
		if (p->gf[APMODE_NATIVE].gfx_filter == 0 && p->gfx_api)
			p->gf[APMODE_NATIVE].gfx_filter = 1;
		if (p->gf[APMODE_RTG].gfx_filter == 0 && p->gfx_api)
			p->gf[APMODE_RTG].gfx_filter = 1;
		//WIN32GUI_LoadUIString(IDS_INPUT_CUSTOM, buf, sizeof buf / sizeof(TCHAR));
		//for (int i = 0; i < GAMEPORT_INPUT_SETTINGS; i++)
		//	_stprintf(p->input_config_name[i], buf, i + 1);
		//p->aviout_xoffset = -1;
		//p->aviout_yoffset = -1;
	}
	if (type == 1 || type == 0 || type == 3) {
		p->uaescsimode = UAESCSI_CDEMU;
		//p->midioutdev = -2;
		//p->midiindev = 0;
		//p->midirouter = false;
		p->automount_removable = 0;
		//p->automount_drives = 0;
		//p->automount_removabledrives = 0;
		p->automount_cddrives = true;
		//p->automount_netdrives = 0;
		p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_B8G8R8A8;
		//p->filesystem_mangle_reserved_names = true;
	}
	
	p->fast_copper = 0;
	p->kbd_led_num = -1; // No status on numlock
	p->kbd_led_scr = -1; // No status on scrollock

	p->gfx_monitor[0].gfx_size_win.width = amiberry_options.default_width;
	p->gfx_monitor[0].gfx_size_win.height = amiberry_options.default_height;
	
	p->gfx_auto_height = amiberry_options.default_auto_height;
	p->gfx_correct_aspect = amiberry_options.default_correct_aspect_ratio;

	if (amiberry_options.default_fullscreen)
	{
		p->gfx_apmode[0].gfx_fullscreen = GFX_FULLSCREEN;
		p->gfx_apmode[1].gfx_fullscreen = GFX_FULLSCREEN;
	}
	else
	{
		p->gfx_apmode[0].gfx_fullscreen = GFX_WINDOW;
		p->gfx_apmode[1].gfx_fullscreen = GFX_WINDOW;
	}
	
	p->scaling_method = -1; //Default is Auto
	if (amiberry_options.default_scaling_method != -1)
	{
		// only valid values are -1 (Auto), 0 (Nearest) and 1 (Linear)
		if (amiberry_options.default_scaling_method == 0 || amiberry_options.default_scaling_method == 1)
			p->scaling_method = amiberry_options.default_scaling_method;
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
		// Single line mode (default)
		p->gfx_vresolution = VRES_NONDOUBLE;
		p->gfx_pscanlines = 0;
	}

	if (amiberry_options.default_horizontal_centering)
		p->gfx_xcenter = 2;
	
	if (amiberry_options.default_vertical_centering)
		p->gfx_ycenter = 2;

	if (amiberry_options.default_frameskip)
		p->gfx_framerate = 2;
	
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

	p->alt_tab_release = false;
	p->sound_pullmode = 0;
	p->input_analog_remap = false;

	p->use_retroarch_quit = amiberry_options.default_retroarch_quit;
	p->use_retroarch_menu = amiberry_options.default_retroarch_menu;
	p->use_retroarch_reset = amiberry_options.default_retroarch_reset;

	p->whdbootprefs.buttonwait = amiberry_options.default_whd_buttonwait;
	p->whdbootprefs.showsplash = amiberry_options.default_whd_showsplash;
	p->whdbootprefs.configdelay = amiberry_options.default_whd_configdelay;

	// Disable Cycle-Exact modes that are not yet implemented
	if (changed_prefs.cpu_cycle_exact || changed_prefs.cpu_memory_cycle_exact)
	{
		if (changed_prefs.cpu_model > 68010)
		{
			changed_prefs.cpu_cycle_exact = changed_prefs.cpu_memory_cycle_exact = false;
		}
	}
}

static const TCHAR* scsimode[] = { _T("SCSIEMU"), _T("SPTI"), _T("SPTI+SCSISCAN"), NULL };
static const TCHAR* statusbarmode[] = { _T("none"), _T("normal"), _T("extended"), NULL };
static const TCHAR* configmult[] = { _T("1x"), _T("2x"), _T("3x"), _T("4x"), _T("5x"), _T("6x"), _T("7x"), _T("8x"), NULL };

void target_save_options(struct zfile* f, struct uae_prefs* p)
{
	cfgfile_target_write_bool(f, _T("middle_mouse"), (p->input_mouse_untrap & MOUSEUNTRAP_MIDDLEBUTTON) != 0);
	cfgfile_target_dwrite_bool(f, _T("map_drives_auto"), p->automount_removable);
	cfgfile_target_dwrite_bool(f, _T("map_cd_drives"), p->automount_cddrives);
	cfgfile_target_dwrite_str(f, _T("serial_port"), p->sername[0] ? p->sername : _T("none"));

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
	cfgfile_target_dwrite_bool(f, _T("start_iconified"), p->start_minimized);
	cfgfile_target_dwrite_bool(f, _T("start_not_captured"), p->start_uncaptured);

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
	
	cfgfile_target_dwrite(f, _T("cpu_idle"), _T("%d"), p->cpu_idle);
	cfgfile_target_dwrite_bool(f, _T("always_on_top"), p->main_alwaysontop);
	cfgfile_target_dwrite_bool(f, _T("gui_always_on_top"), p->gui_alwaysontop);
	cfgfile_target_dwrite_bool(f, _T("right_control_is_right_win"), p->right_control_is_right_win_key);
	
	cfgfile_target_dwrite_bool(f, _T("gfx_auto_height"), p->gfx_auto_height);
	cfgfile_target_dwrite(f, _T("gfx_correct_aspect"), _T("%d"), p->gfx_correct_aspect);
	cfgfile_target_dwrite(f, _T("kbd_led_num"), _T("%d"), p->kbd_led_num);
	cfgfile_target_dwrite(f, _T("kbd_led_scr"), _T("%d"), p->kbd_led_scr);
	cfgfile_target_dwrite(f, _T("scaling_method"), _T("%d"), p->scaling_method);

	cfgfile_target_dwrite_str(f, _T("open_gui"), p->open_gui);
	cfgfile_target_dwrite_str(f, _T("quit_amiberry"), p->quit_amiberry);
	cfgfile_target_dwrite_str(f, _T("action_replay"), p->action_replay);
	cfgfile_target_dwrite_str(f, _T("fullscreen_toggle"), p->fullscreen_toggle);
	cfgfile_target_dwrite_str(f, _T("minimize"), p->minimize);

	cfgfile_target_dwrite_bool(f, _T("alt_tab_release"), p->alt_tab_release);
	cfgfile_target_dwrite(f, _T("sound_pullmode"), _T("%d"), p->sound_pullmode);
	cfgfile_target_dwrite_bool(f, _T("use_analogue_remap"), p->input_analog_remap);

	cfgfile_target_dwrite_bool(f, _T("use_retroarch_quit"), p->use_retroarch_quit);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_menu"), p->use_retroarch_menu);
	cfgfile_target_dwrite_bool(f, _T("use_retroarch_reset"), p->use_retroarch_reset);
}

void target_restart(void)
{
	emulating = 0;
	gui_restart();
}

TCHAR* target_expand_environment(const TCHAR* path, TCHAR* out, int maxlen)
{
	if (out == nullptr)
		return strdup(path);

	_tcscpy(out, path);
	return out;
}

int target_parse_option(struct uae_prefs* p, const char* option, const char* value)
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
	if (cfgfile_yesno(option, value, _T("map_drives_auto"), &p->automount_removable))
		return 1;
	if (cfgfile_yesno(option, value, _T("map_cd_drives"), &p->automount_cddrives))
		return 1;
	if (cfgfile_yesno(option, value, _T("alt_tab_release"), &p->alt_tab_release))
		return 1;
	if (cfgfile_yesno(option, value, _T("use_retroarch_quit"), &p->use_retroarch_quit))
		return 1;
	if (cfgfile_yesno(option, value, _T("use_retroarch_menu"), &p->use_retroarch_menu))
		return 1;
	if (cfgfile_yesno(option, value, _T("use_retroarch_reset"), &p->use_retroarch_reset))
		return 1;
	if (cfgfile_intval(option, value, _T("sound_pullmode"), &p->sound_pullmode, 1))
		return 1;
	if (cfgfile_yesno(option, value, _T("use_analogue_remap"), &p->input_analog_remap))
		return 1;
	if (cfgfile_intval(option, value, "kbd_led_num", &p->kbd_led_num, 1))
		return 1;
	if (cfgfile_intval(option, value, "kbd_led_scr", &p->kbd_led_scr, 1))
		return 1;
	if (cfgfile_yesno(option, value, _T("gfx_auto_height"), &p->gfx_auto_height))
		return 1;
	if (cfgfile_intval(option, value, "gfx_correct_aspect", &p->gfx_correct_aspect, 1))
		return 1;
	if (cfgfile_intval(option, value, "scaling_method", &p->scaling_method, 1))
		return 1;
	if (cfgfile_string(option, value, "open_gui", p->open_gui, sizeof p->open_gui))
		return 1;
	if (cfgfile_string(option, value, "quit_amiberry", p->quit_amiberry, sizeof p->quit_amiberry))
		return 1;
	if (cfgfile_string(option, value, "action_replay", p->action_replay, sizeof p->action_replay))
		return 1;
	if (cfgfile_string(option, value, "fullscreen_toggle", p->fullscreen_toggle, sizeof p->fullscreen_toggle))
		return 1;
	if (cfgfile_string(option, value, "minimize", p->minimize, sizeof p->minimize))
		return 1;
	if (cfgfile_intval(option, value, _T("cpu_idle"), &p->cpu_idle, 1))
		return 1;
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
	if (cfgfile_yesno(option, value, _T("borderless"), &p->borderless))
		return 1;
	if (cfgfile_yesno(option, value, _T("blank_monitors"), &p->blankmonitors))
		return 1;
	if (cfgfile_yesno(option, value, _T("active_nocapture_pause"), &p->active_nocapture_pause))
		return 1;
	if (cfgfile_yesno(option, value, _T("active_nocapture_nosound"), &p->active_nocapture_nosound))
		return 1;
	if (cfgfile_intval(option, value, _T("inactive_priority"), &p->inactive_priority, 1))
		return 1;
	if (cfgfile_yesno(option, value, _T("inactive_pause"), &p->inactive_pause))
		return 1;
	if (cfgfile_yesno(option, value, _T("inactive_nosound"), &p->inactive_nosound))
		return 1;
	if (cfgfile_intval(option, value, _T("inactive_input"), &p->inactive_input, 1))
		return 1;
	if (cfgfile_intval(option, value, _T("minimized_priority"), &p->minimized_priority, 1))
		return 1;
	if (cfgfile_yesno(option, value, _T("minimized_pause"), &p->minimized_pause))
		return 1;
	if (cfgfile_yesno(option, value, _T("minimized_nosound"), &p->minimized_nosound))
		return 1;
	if (cfgfile_intval(option, value, _T("minimized_input"), &p->minimized_input, 1))
		return 1;
	if (cfgfile_yesno(option, value, _T("active_capture_automatically"), &p->capture_always))
		return 1;
	if (cfgfile_yesno(option, value, _T("inactive_minimize"), &p->minimize_inactive))
		return 1;
	if (cfgfile_yesno(option, value, _T("start_iconified"), &p->start_minimized))
		return 1;
	if (cfgfile_yesno(option, value, _T("start_not_captured"), &p->start_uncaptured))
		return 1;
	if (cfgfile_yesno(option, value, _T("right_control_is_right_win"), &p->right_control_is_right_win_key))
		return 1;
	if (cfgfile_yesno(option, value, _T("always_on_top"), &p->main_alwaysontop))
		return 1;
	if (cfgfile_yesno(option, value, _T("gui_always_on_top"), &p->gui_alwaysontop))
		return 1;
	if (cfgfile_string(option, value, _T("serial_port"), &p->sername[0], 256)) {
		if (p->sername[0])
			p->use_serial = true;
		else
			p->use_serial = false;
		return 1;
	}
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

	if (cfgfile_yesno(option, value, _T("rtg_match_depth"), &p->rtgmatchdepth))
		return 1;

	if (cfgfile_yesno(option, value, _T("rtg_scale_allow"), &p->rtgallowscaling))
		return 1;
	
	if (cfgfile_string(option, value, _T("rtg_scale_aspect_ratio"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		int v1, v2;
		TCHAR* s;

		p->rtgscaleaspectratio = -1;
		v1 = _tstol(tmpbuf);
		s = _tcschr(tmpbuf, ':');
		if (s) {
			v2 = _tstol(s + 1);
			if (v1 < 0 || v2 < 0)
				p->rtgscaleaspectratio = -1;
			else if (v1 == 0 || v2 == 0)
				p->rtgscaleaspectratio = 0;
			else
				p->rtgscaleaspectratio = v1 * ASPECTMULT + v2;
		}
		return 1;
	}
	
	return 0;
}

void get_data_path(char* out, int size)
{
	strncpy(out, start_path_data, size - 1);
	strncat(out, "/", size - 1);
}

void get_saveimage_path(char* out, int size, int dir)
{
	strncpy(out, start_path_data, size - 1);
	strncat(out, "/savestates/", size - 1);
}

void get_configuration_path(char* out, int size)
{
	fix_trailing(config_path);
	strncpy(out, config_path, size - 1);
}

void set_configuration_path(char* newpath)
{
	strncpy(config_path, newpath, MAX_DPATH - 1);
}

void get_controllers_path(char* out, int size)
{
	fix_trailing(controllers_path);
	strncpy(out, controllers_path, size - 1);
}

void set_controllers_path(char* newpath)
{
	strncpy(controllers_path, newpath, MAX_DPATH - 1);
}

void get_retroarch_file(char* out, int size)
{
	strncpy(out, retroarch_file, size - 1);
}

void set_retroarch_file(char* newpath)
{
	strncpy(retroarch_file, newpath, MAX_DPATH - 1);
}

bool get_logfile_enabled()
{
	return amiberry_options.write_logfile;
}

void set_logfile_enabled(bool enabled)
{
	amiberry_options.write_logfile = enabled;
}

void get_logfile_path(char* out, int size)
{
	strncpy(out, logfile_path, size - 1);
}

void set_logfile_path(char* newpath)
{
	strncpy(logfile_path, newpath, MAX_DPATH - 1);
}

void get_rom_path(char* out, int size)
{
	fix_trailing(rom_path);
	strncpy(out, rom_path, size - 1);
}

void set_rom_path(char* newpath)
{
	strncpy(rom_path, newpath, MAX_DPATH - 1);
}

void get_rp9_path(char* out, int size)
{
	fix_trailing(rp9_path);
	strncpy(out, rp9_path, size - 1);
}

void get_savestate_path(char* out, int size)
{
	strncpy(out, start_path_data, size - 1);
	strncat(out, "/savestates/", size - 1);
}

void get_screenshot_path(char* out, int size)
{
	strncpy(out, start_path_data, size - 1);
	strncat(out, "/screenshots/", size - 1);
}

int target_cfgfile_load(struct uae_prefs* p, const char* filename, int type, int isdefault)
{
	auto result = 0;

	write_log(_T("target_cfgfile_load(): load file %s\n"), filename);

	discard_prefs(p, type);
	default_prefs(p, true, 0);

	const char* ptr = strstr(const_cast<char*>(filename), ".uae");
	if (ptr)
	{
		auto config_type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
		result = cfgfile_load(p, filename, &config_type, 0, 1);
	}
	if (result)
		extract_filename(filename, last_loaded_config);

	if (result)
	{
		for (auto i = 0; i < p->nr_floppies; ++i)
		{
			if (!DISK_validate_filename(p, p->floppyslots[i].df, nullptr, 0, nullptr, nullptr, nullptr))
				p->floppyslots[i].df[0] = 0;
			disk_insert(i, p->floppyslots[i].df);
			if (strlen(p->floppyslots[i].df) > 0)
				AddFileToDiskList(p->floppyslots[i].df, 1);
		}

		if (!isdefault)
			inputdevice_updateconfig(nullptr, p);

		SetLastActiveConfig(filename);
	}

	return result;
}

int check_configfile(char* file)
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

void extract_path(char* str, char* buffer)
{
	strncpy(buffer, str, MAX_DPATH - 1);
	auto* p = buffer + strlen(buffer) - 1;
	while (*p != '/' && p >= buffer)
		p--;
	p[1] = '\0';
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

void read_directory(const char* path, std::vector<std::string>* dirs, std::vector<std::string>* files)
{
	struct dirent* dent;

	if (dirs != nullptr)
		dirs->clear();
	if (files != nullptr)
		files->clear();

	auto* const dir = opendir(path);
	if (dir != nullptr)
	{
		while ((dent = readdir(dir)) != nullptr)
		{
			if (dent->d_type == DT_DIR)
			{
				if (dirs != nullptr)
					dirs->push_back(dent->d_name);
			}
			else if (dent->d_type == DT_LNK)
			{
				struct stat stbuf{};
				stat(dent->d_name, &stbuf);
				if (S_ISDIR(stbuf.st_mode))
				{
					if (dirs != nullptr)
						dirs->push_back(dent->d_name);
				}
				else
				{
					if (files != nullptr)
						files->push_back(dent->d_name);
				}
			}
			else if (files != nullptr)
				files->push_back(dent->d_name);
		}
		if (dirs != nullptr && !dirs->empty() && (*dirs)[0] == ".")
			dirs->erase(dirs->begin());
		closedir(dir);
	}

	if (dirs != nullptr)
		sort(dirs->begin(), dirs->end());
	if (files != nullptr)
		sort(files->begin(), files->end());
}

void save_amiberry_settings(void)
{
	char path[MAX_DPATH];

	snprintf(path, MAX_DPATH, "%s/conf/amiberry.conf", start_path_data);
	auto* const f = fopen(path, "we");
	if (!f)
		return;

	char buffer[MAX_DPATH];

	// Should the Quickstart Panel be the default when opening the GUI?
	snprintf(buffer, MAX_DPATH, "Quickstart=%d\n", amiberry_options.quickstart_start);
	fputs(buffer, f);

	// Open each config file and read the Description field? 
	// This will slow down scanning the config list if it's very large
	snprintf(buffer, MAX_DPATH, "read_config_descriptions=%s\n", amiberry_options.read_config_descriptions ? "yes" : "no");
	fputs(buffer, f);

	// Write to logfile? 
	// If enabled, a file named "amiberry_log.txt" will be generated in the startup folder
	snprintf(buffer, MAX_DPATH, "write_logfile=%s\n", amiberry_options.write_logfile ? "yes" : "no");
	fputs(buffer, f);

	// Scanlines ON by default?
	// This will only be enabled if the vertical height is enough, as we need Line Doubling set to ON also
	// Beware this comes with a performance hit, as double the amount of lines need to be drawn on-screen
	snprintf(buffer, MAX_DPATH, "default_line_mode=%d\n", amiberry_options.default_line_mode);
	fputs(buffer, f);

	// Map RCtrl key to RAmiga key?
	// This helps with keyboards that may not have 2 Win keys and no Menu key either
	snprintf(buffer, MAX_DPATH, "rctrl_as_ramiga=%s\n", amiberry_options.rctrl_as_ramiga ? "yes" : "no");
	fputs(buffer, f);

	// Disable controller in the GUI?
	// If you want to disable the default behavior for some reason
	snprintf(buffer, MAX_DPATH, "gui_joystick_control=%s\n", amiberry_options.gui_joystick_control ? "yes" : "no");
	fputs(buffer, f);

	// Use a separate render thread under SDL2?
	// This might give a performance boost, but it's not supported on all SDL2 back-ends
	snprintf(buffer, MAX_DPATH, "use_sdl2_render_thread=%s\n", amiberry_options.use_sdl2_render_thread ? "yes" : "no");
	fputs(buffer, f);

	// Default mouse input speed
	snprintf(buffer, MAX_DPATH, "input_default_mouse_speed=%d\n", amiberry_options.input_default_mouse_speed);
	fputs(buffer, f);

	// When using Keyboard as Joystick, stop any double keypresses
	snprintf(buffer, MAX_DPATH, "input_keyboard_as_joystick_stop_keypresses=%s\n", amiberry_options.input_keyboard_as_joystick_stop_keypresses ? "yes" : "no");
	fputs(buffer, f);
	
	// Default key for opening the GUI (e.g. "F12")
	snprintf(buffer, MAX_DPATH, "default_open_gui_key=%s\n", amiberry_options.default_open_gui_key);
	fputs(buffer, f);

	// Default key for Quitting the emulator
	snprintf(buffer, MAX_DPATH, "default_quit_key=%s\n", amiberry_options.default_quit_key);
	fputs(buffer, f);

	// Default key for opening Action Replay
	snprintf(buffer, MAX_DPATH, "default_ar_key=%s\n", amiberry_options.default_ar_key);
	fputs(buffer, f);

	// Default key for Fullscreen Toggle
	snprintf(buffer, MAX_DPATH, "default_fullscreen_toggle_key=%s\n", amiberry_options.default_fullscreen_toggle_key);
	fputs(buffer, f);

	// Rotation angle of the output display (useful for screens with portrait orientation, like the Go Advance)
	snprintf(buffer, MAX_DPATH, "rotation_angle=%d\n", amiberry_options.rotation_angle);
	fputs(buffer, f);

	// Enable Horizontal Centering by default?
	snprintf(buffer, MAX_DPATH, "default_horizontal_centering=%s\n", amiberry_options.default_horizontal_centering ? "yes" : "no");
	fputs(buffer, f);

	// Enable Vertical Centering by default?
	snprintf(buffer, MAX_DPATH, "default_vertical_centering=%s\n", amiberry_options.default_vertical_centering ? "yes" : "no");
	fputs(buffer, f);

	// Scaling method to use by default?
	// Valid options are: -1 Auto, 0 Nearest Neighbor, 1 Linear
	snprintf(buffer, MAX_DPATH, "default_scaling_method=%d\n", amiberry_options.default_scaling_method);
	fputs(buffer, f);

	// Enable frameskip by default?
	snprintf(buffer, MAX_DPATH, "default_frameskip=%s\n", amiberry_options.default_frameskip ? "yes" : "no");
	fputs(buffer, f);

	// Correct Aspect Ratio by default?
	snprintf(buffer, MAX_DPATH, "default_correct_aspect_ratio=%s\n", amiberry_options.default_correct_aspect_ratio ? "yes" : "no");
	fputs(buffer, f);

	// Enable Auto-Height by default?
	snprintf(buffer, MAX_DPATH, "default_auto_height=%s\n", amiberry_options.default_auto_height ? "yes" : "no");
	fputs(buffer, f);

	// Default Screen Width
	snprintf(buffer, MAX_DPATH, "default_width=%d\n", amiberry_options.default_width);
	fputs(buffer, f);
	
	// Default Screen Height
	snprintf(buffer, MAX_DPATH, "default_height=%d\n", amiberry_options.default_height);
	fputs(buffer, f);

	// Full screen by default?
	snprintf(buffer, MAX_DPATH, "default_fullscreen=%s\n", amiberry_options.default_fullscreen ? "yes" : "no");
	fputs(buffer, f);
	
	// Default Stereo Separation
	snprintf(buffer, MAX_DPATH, "default_stereo_separation=%d\n", amiberry_options.default_stereo_separation);
	fputs(buffer, f);

	// Default Sound buffer size
	snprintf(buffer, MAX_DPATH, "default_sound_buffer=%d\n", amiberry_options.default_sound_buffer);
	fputs(buffer, f);

	// Default Joystick Deadzone
	snprintf(buffer, MAX_DPATH, "default_joystick_deadzone=%d\n", amiberry_options.default_joystick_deadzone);
	fputs(buffer, f);

	// Enable RetroArch Quit by default?
	snprintf(buffer, MAX_DPATH, "default_retroarch_quit=%s\n", amiberry_options.default_retroarch_quit ? "yes" : "no");
	fputs(buffer, f);

	// Enable RetroArch Menu by default?
	snprintf(buffer, MAX_DPATH, "default_retroarch_menu=%s\n", amiberry_options.default_retroarch_menu ? "yes" : "no");
	fputs(buffer, f);

	// Enable RetroArch Reset by default?
	snprintf(buffer, MAX_DPATH, "default_retroarch_reset=%s\n", amiberry_options.default_retroarch_reset ? "yes" : "no");
	fputs(buffer, f);

	// Controller1
	snprintf(buffer, MAX_DPATH, "default_controller1=%s\n", amiberry_options.default_controller1);
	fputs(buffer, f);

	// Controller2
	snprintf(buffer, MAX_DPATH, "default_controller2=%s\n", amiberry_options.default_controller2);
	fputs(buffer, f);

	// Controller3
	snprintf(buffer, MAX_DPATH, "default_controller3=%s\n", amiberry_options.default_controller3);
	fputs(buffer, f);

	// Controller4
	snprintf(buffer, MAX_DPATH, "default_controller4=%s\n", amiberry_options.default_controller4);
	fputs(buffer, f);

	// Mouse1
	snprintf(buffer, MAX_DPATH, "default_mouse1=%s\n", amiberry_options.default_mouse1);
	fputs(buffer, f);

	// Mouse2
	snprintf(buffer, MAX_DPATH, "default_mouse2=%s\n", amiberry_options.default_mouse2);
	fputs(buffer, f);

	// WHDLoad ButtonWait
	snprintf(buffer, MAX_DPATH, "default_whd_buttonwait=%s\n", amiberry_options.default_whd_buttonwait ? "yes" : "no");
	fputs(buffer, f);

	// WHDLoad Show Splash screen
	snprintf(buffer, MAX_DPATH, "default_whd_showsplash=%s\n", amiberry_options.default_whd_showsplash ? "yes" : "no");
	fputs(buffer, f);

	// WHDLoad Config Delay
	snprintf(buffer, MAX_DPATH, "default_whd_configdelay=%d\n", amiberry_options.default_whd_configdelay);
	fputs(buffer, f);
	
	// Paths
	snprintf(buffer, MAX_DPATH, "path=%s\n", current_dir);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "config_path=%s\n", config_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "controllers_path=%s\n", controllers_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "retroarch_config=%s\n", retroarch_file);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "logfile_path=%s\n", logfile_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "rom_path=%s\n", rom_path);
	fputs(buffer, f);

	// The number of ROMs in the last scan
	snprintf(buffer, MAX_DPATH, "ROMs=%zu\n", lstAvailableROMs.size());
	fputs(buffer, f);

	// The ROMs found in the last scan
	for (auto& lstAvailableROM : lstAvailableROMs)
	{
		snprintf(buffer, MAX_DPATH, "ROMName=%s\n", lstAvailableROM->Name);
		fputs(buffer, f);
		snprintf(buffer, MAX_DPATH, "ROMPath=%s\n", lstAvailableROM->Path);
		fputs(buffer, f);
		snprintf(buffer, MAX_DPATH, "ROMType=%d\n", lstAvailableROM->ROMType);
		fputs(buffer, f);
	}

	// Recent disk entries (these are used in the dropdown controls)
	snprintf(buffer, MAX_DPATH, "MRUDiskList=%zu\n", lstMRUDiskList.size());
	fputs(buffer, f);
	for (auto& i : lstMRUDiskList)
	{
		snprintf(buffer, MAX_DPATH, "Diskfile=%s\n", i.c_str());
		fputs(buffer, f);
	}

	// Recent CD entries (these are used in the dropdown controls)
	snprintf(buffer, MAX_DPATH, "MRUCDList=%zu\n", lstMRUCDList.size());
	fputs(buffer, f);
	for (auto& i : lstMRUCDList)
	{
		snprintf(buffer, MAX_DPATH, "CDfile=%s\n", i.c_str());
		fputs(buffer, f);
	}

	// Recent WHDLoad entries (these are used in the dropdown controls)
	// lstMRUWhdloadList
	snprintf(buffer, MAX_DPATH, "MRUWHDLoadList=%zu\n", lstMRUWhdloadList.size());
	fputs(buffer, f);
	for (auto& i : lstMRUWhdloadList)
	{
		snprintf(buffer, MAX_DPATH, "WHDLoadfile=%s\n", i.c_str());
		fputs(buffer, f);
	}
	
	fclose(f);
}

void get_string(FILE* f, char* dst, int size)
{
	char buffer[MAX_DPATH];
	fgets(buffer, MAX_DPATH, f);
	int i = strlen(buffer);
	while (i > 0 && (buffer[i - 1] == '\t' || buffer[i - 1] == ' '
		|| buffer[i - 1] == '\r' || buffer[i - 1] == '\n'))
		buffer[--i] = '\0';
	strncpy(dst, buffer, size);
}

static void trim_wsa(char* s)
{
	/* Delete trailing whitespace.  */
	int len = strlen(s);
	while (len > 0 && strcspn(s + len - 1, "\t \r\n") == 0)
		s[--len] = '\0';
}

void load_amiberry_settings(void)
{
	char path[MAX_DPATH];
	strncpy(current_dir, start_path_data, MAX_DPATH - 1);
	snprintf(config_path, MAX_DPATH, "%s/conf/", start_path_data);
	snprintf(controllers_path, MAX_DPATH, "%s/controllers/", start_path_data);
	snprintf(retroarch_file, MAX_DPATH, "%s/conf/retroarch.cfg", start_path_data);
	snprintf(logfile_path, MAX_DPATH, "%s/amiberry.log", start_path_data);

#ifdef ANDROID
	char afepath[MAX_DPATH];
	snprintf(afepath, MAX_DPATH, "%s/Android/data/com.cloanto.amigaforever.essentials/files/rom/", getenv("SDCARD"));
	DIR* afedir = opendir(afepath);
	if (afedir) {
		snprintf(rom_path, MAX_DPATH, "%s", afepath);
		closedir(afedir);
	}
	else
		snprintf(rom_path, MAX_DPATH, "%s/kickstarts/", start_path_data);
#else
	snprintf(rom_path, MAX_DPATH, "%s/kickstarts/", start_path_data);
#endif
	snprintf(rp9_path, MAX_DPATH, "%s/rp9/", start_path_data);
	snprintf(path, MAX_DPATH, "%s/conf/amiberry.conf", start_path_data);
	snprintf(floppy_sounds_dir, MAX_DPATH, "%s/data/floppy_sounds/", start_path_data);

	auto* const fh = zfile_fopen(path, _T("r"), ZFD_NORMAL);
	if (fh)
	{
		char linea[CONFIG_BLEN];
		TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
		int numROMs, numDisks, numCDs;
		auto romType = -1;
		char romName[MAX_DPATH] = {'\0'};
		char romPath[MAX_DPATH] = {'\0'};
		char tmpFile[MAX_DPATH];

		while (zfile_fgetsa(linea, sizeof linea, fh) != nullptr)
		{
			trim_wsa(linea);
			if (strlen(linea) > 0)
			{
				if (!cfgfile_separate_linea(path, linea, option, value))
					continue;

				if (cfgfile_string(option, value, "ROMName", romName, sizeof romName)
					|| cfgfile_string(option, value, "ROMPath", romPath, sizeof romPath)
					|| cfgfile_intval(option, value, "ROMType", &romType, 1))
				{
					if (strlen(romName) > 0 && strlen(romPath) > 0 && romType != -1)
					{
						auto* tmp = new AvailableROM();
						strncpy(tmp->Name, romName, sizeof tmp->Name - 1);
						strncpy(tmp->Path, romPath, sizeof tmp->Path - 1);
						tmp->ROMType = romType;
						lstAvailableROMs.emplace_back(tmp);
						strncpy(romName, "", sizeof romName);
						strncpy(romPath, "", sizeof romPath);
						romType = -1;
					}
				}
				else if (cfgfile_string(option, value, "Diskfile", tmpFile, sizeof tmpFile))
				{
					auto* const f = fopen(tmpFile, "rbe");
					if (f != nullptr)
					{
						fclose(f);
						lstMRUDiskList.emplace_back(tmpFile);
					}
				}
				else if (cfgfile_string(option, value, "CDfile", tmpFile, sizeof tmpFile))
				{
					auto* const f = fopen(tmpFile, "rbe");
					if (f != nullptr)
					{
						fclose(f);
						lstMRUCDList.emplace_back(tmpFile);
					}
				}
				else if (cfgfile_string(option, value, "WHDLoadfile", tmpFile, sizeof tmpFile))
				{
					auto* const f = fopen(tmpFile, "rbe");
					if (f != nullptr)
					{
						fclose(f);
						lstMRUWhdloadList.emplace_back(tmpFile);
					}
				}
				else
				{
					cfgfile_string(option, value, "path", current_dir, sizeof current_dir);
					cfgfile_string(option, value, "config_path", config_path, sizeof config_path);
					cfgfile_string(option, value, "controllers_path", controllers_path, sizeof controllers_path);
					cfgfile_string(option, value, "retroarch_config", retroarch_file, sizeof retroarch_file);
					cfgfile_string(option, value, "logfile_path", logfile_path, sizeof logfile_path);
					cfgfile_string(option, value, "rom_path", rom_path, sizeof rom_path);
					cfgfile_intval(option, value, "ROMs", &numROMs, 1);
					cfgfile_intval(option, value, "MRUDiskList", &numDisks, 1);
					cfgfile_intval(option, value, "MRUCDList", &numCDs, 1);
					cfgfile_yesno(option, value, "Quickstart", &amiberry_options.quickstart_start);
					cfgfile_yesno(option, value, "read_config_descriptions", &amiberry_options.read_config_descriptions);
					cfgfile_yesno(option, value, "write_logfile", &amiberry_options.write_logfile);
					cfgfile_intval(option, value, "default_line_mode", &amiberry_options.default_line_mode, 1);
					cfgfile_yesno(option, value, "rctrl_as_ramiga", &amiberry_options.rctrl_as_ramiga);
					cfgfile_yesno(option, value, "gui_joystick_control", &amiberry_options.gui_joystick_control);
					cfgfile_yesno(option, value, "use_sdl2_render_thread", &amiberry_options.use_sdl2_render_thread);
					cfgfile_intval(option, value, "input_default_mouse_speed", &amiberry_options.input_default_mouse_speed, 1);
					cfgfile_yesno(option, value, "input_keyboard_as_joystick_stop_keypresses", &amiberry_options.input_keyboard_as_joystick_stop_keypresses);
					cfgfile_string(option, value, "default_open_gui_key", amiberry_options.default_open_gui_key, sizeof amiberry_options.default_open_gui_key);
					cfgfile_string(option, value, "default_quit_key", amiberry_options.default_quit_key, sizeof amiberry_options.default_quit_key);
					cfgfile_string(option, value, "default_ar_key", amiberry_options.default_ar_key, sizeof amiberry_options.default_ar_key);
					cfgfile_string(option, value, "default_fullscreen_toggle_key", amiberry_options.default_fullscreen_toggle_key, sizeof amiberry_options.default_fullscreen_toggle_key);
					cfgfile_intval(option, value, "rotation_angle", &amiberry_options.rotation_angle, 1);
					cfgfile_yesno(option, value, "default_horizontal_centering", &amiberry_options.default_horizontal_centering);
					cfgfile_yesno(option, value, "default_vertical_centering", &amiberry_options.default_vertical_centering);
					cfgfile_intval(option, value, "default_scaling_method", &amiberry_options.default_scaling_method, 1);
					cfgfile_yesno(option, value, "default_frameskip", &amiberry_options.default_frameskip);
					cfgfile_yesno(option, value, "default_correct_aspect_ratio", &amiberry_options.default_correct_aspect_ratio);
					cfgfile_yesno(option, value, "default_auto_height", &amiberry_options.default_auto_height);
					cfgfile_intval(option, value, "default_width", &amiberry_options.default_width, 1);
					cfgfile_intval(option, value, "default_height", &amiberry_options.default_height, 1);
					cfgfile_yesno(option, value, "default_fullscreen", &amiberry_options.default_fullscreen);
					cfgfile_intval(option, value, "default_stereo_separation", &amiberry_options.default_stereo_separation, 1);
					cfgfile_intval(option, value, "default_sound_buffer", &amiberry_options.default_sound_buffer, 1);
					cfgfile_intval(option, value, "default_joystick_deadzone", &amiberry_options.default_joystick_deadzone, 1);
					cfgfile_yesno(option, value, "default_retroarch_quit", &amiberry_options.default_retroarch_quit);
					cfgfile_yesno(option, value, "default_retroarch_menu", &amiberry_options.default_retroarch_menu);
					cfgfile_yesno(option, value, "default_retroarch_reset", &amiberry_options.default_retroarch_reset);
					cfgfile_string(option, value, "default_controller1", amiberry_options.default_controller1, sizeof amiberry_options.default_controller1);
					cfgfile_string(option, value, "default_controller2", amiberry_options.default_controller2, sizeof amiberry_options.default_controller2);
					cfgfile_string(option, value, "default_controller3", amiberry_options.default_controller3, sizeof amiberry_options.default_controller3);
					cfgfile_string(option, value, "default_controller4", amiberry_options.default_controller4, sizeof amiberry_options.default_controller4);
					cfgfile_string(option, value, "default_mouse1", amiberry_options.default_mouse1, sizeof amiberry_options.default_mouse1);
					cfgfile_string(option, value, "default_mouse2", amiberry_options.default_mouse2, sizeof amiberry_options.default_mouse2);
					cfgfile_yesno(option, value, "default_whd_buttonwait", &amiberry_options.default_whd_buttonwait);
					cfgfile_yesno(option, value, "default_whd_showsplash", &amiberry_options.default_whd_showsplash);
					cfgfile_intval(option, value, "default_whd_configdelay", &amiberry_options.default_whd_configdelay, 1);
				}
			}
		}
		zfile_fclose(fh);
	}
}

void rename_old_adfdir()
{
	char old_path[MAX_DPATH];
	char new_path[MAX_DPATH];
	snprintf(old_path, MAX_DPATH, "%s/conf/adfdir.conf", start_path_data);
	snprintf(new_path, MAX_DPATH, "%s/conf/amiberry.conf", start_path_data);

	const auto result = rename(old_path, new_path);
	if (result == 0)
		write_log("Old adfdir.conf file successfully renamed to amiberry.conf");
	else
		write_log("Error while trying to rename old adfdir.conf file to amiberry.conf!");
}

void target_getdate(int* y, int* m, int* d)
{
	*y = GETBDY(AMIBERRYDATE);
	*m = GETBDM(AMIBERRYDATE);
	*d = GETBDD(AMIBERRYDATE);
}

void target_addtorecent(const TCHAR* name, int t)
{
}

void target_reset()
{
	clipboard_reset();
}

bool target_can_autoswitchdevice(void)
{
	if (mouseactive <= 0)
		return false;
	return true;
}

uae_u32 emulib_target_getcpurate(uae_u32 v, uae_u32* low)
{
	*low = 0;
	if (v == 1)
	{
		*low = 1e+9; /* We have nano seconds */
		return 0;
	}
	if (v == 2)
	{
		struct timespec ts{};
		clock_gettime(CLOCK_MONOTONIC, &ts);
		const auto time = int64_t(ts.tv_sec) * 1000000000 + ts.tv_nsec;
		*low = uae_u32(time & 0xffffffff);
		return uae_u32(time >> 32);
	}
	return 0;
}

void target_shutdown(void)
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

void* uaenative_get_uaevar(void)
{
	static struct winuae uaevar;
#ifdef _WIN32
	uaevar.amigawnd = mon->hAmigaWnd;
#endif
	//uaevar.z3offset = uae_u32(get_real_address(z3fastmem_bank[0].start)) - z3fastmem_bank[0].start;
	return &uaevar;
}

const TCHAR** uaenative_get_library_dirs(void)
{
	static const TCHAR** nats;
	static TCHAR* path;

	if (nats == NULL)
		nats = xcalloc(const TCHAR*, 3);
	if (path == NULL) {
		path = xcalloc(TCHAR, MAX_DPATH);
		_tcscpy(path, start_path_data);
		_tcscat(path, _T("plugins"));
	}
	nats[0] = start_path_data;
	nats[1] = path;
	return nats;
}

int main(int argc, char* argv[])
{
	struct sigaction action{};

	max_uae_width = 8192;
	max_uae_height = 8192;

	// Get startup path
#ifdef ANDROID
	strncpy(start_path_data, getenv("EXTERNAL_FILES_DIR"), MAX_DPATH - 1);
#else
	getcwd(start_path_data, MAX_DPATH);
#endif
	rename_old_adfdir();
	load_amiberry_settings();

	snprintf(savestate_fname, sizeof savestate_fname, "%s/savestates/default.ads", start_path_data);
	logging_init();

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

	alloc_AmigaMem();
	RescanROMs();
	uae_time_calibrate();
	
	if (
#ifdef USE_DISPMANX
		SDL_Init(SDL_INIT_TIMER
			| SDL_INIT_AUDIO
			| SDL_INIT_JOYSTICK
			| SDL_INIT_HAPTIC
			| SDL_INIT_GAMECONTROLLER
			| SDL_INIT_EVENTS) != 0
#else
		SDL_Init(SDL_INIT_EVERYTHING) != 0
#endif
		)
	{
		write_log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		abort();
	}
	atexit(SDL_Quit);
	write_log(_T("Sorting devices and modes...\n"));
	sortdisplays();
	
	normalcursor = SDL_GetDefaultCursor();
	clipboard_init();
	
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

	real_main(argc, argv);

	// restore keyboard LEDs to normal state
	ioctl(0, KDSETLED, 0xFF);

	ClearAvailableROMList();
	romlist_clear();
	free_keyring();
	free_AmigaMem();
	lstMRUDiskList.clear();
	lstMRUCDList.clear();
	lstMRUWhdloadList.clear();

	logging_cleanup();

	if (host_poweroff)
		target_shutdown();
	return 0;
}

void toggle_mousegrab()
{
	activationtoggle(0, false);
}

bool get_plugin_path(TCHAR* out, int len, const TCHAR* path)
{
	if (strcmp(path, "floppysounds") == 0) {
		if (floppy_sounds_dir) {
			strncpy(out, floppy_sounds_dir, len);
		}
		else {
			strncpy(out, "floppy_sounds", len);
		}
		// make sure out is null-terminated in any case
		out[len - 1] = '\0';
	}
	else {
		strncpy(out, start_path_data, len - 1);
		strncat(out, "/", len - 1);
		strncat(out, path, len - 1);
		strncat(out, "/", len - 1);
		return my_existsfile(out);
	}
	return TRUE;
}
