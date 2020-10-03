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
#include "amiberry_rp9.h"
#include "rtgmodes.h"
#include "gfxboard.h"
#include "devices.h"
#include <map>


#include "amiberry_input.h"
#include "clipboard.h"
#include "uae/uae.h"

extern FILE* debugfile;

int pause_emulation;

static int sound_closed;
static int recapture;
static int focus;
static int mouseinside;
int mouseactive;
int minimized;

int quickstart_model = 0;
int quickstart_conf = 0;
bool host_poweroff = false;

struct amiberry_options amiberry_options = {};

// Default Enter GUI key is F12
int enter_gui_key = 0;
SDL_GameControllerButton enter_gui_button;
// We don't set a default value for Quitting
int quit_key = 0;
// The default value for Action Replay is Pause/Break
int action_replay_button = 0;
// No default value for Full Screen toggle
int fullscreen_key = 0;

bool mouse_grabbed = false;

std::string get_version_string()
{
	std::string label_text = AMIBERRYVERSION;
	return label_text;
}

void set_key_configs(struct uae_prefs* p)
{
	if (strncmp(p->open_gui, "", 1) != 0)
		// If we have a value in the config, we use that instead
		enter_gui_key = SDL_GetKeyFromName(p->open_gui);
	else
		// Otherwise we go for the default found in amiberry.conf
		enter_gui_key = SDL_GetKeyFromName(amiberry_options.default_open_gui_key);
	// if nothing was found in amiberry.conf either, we default back to F12
	if (enter_gui_key == 0)
		enter_gui_key = SDLK_F12;

	enter_gui_button = SDL_GameControllerGetButtonFromString(p->open_gui);
	if (enter_gui_button == SDL_CONTROLLER_BUTTON_INVALID)
		enter_gui_button = SDL_GameControllerGetButtonFromString(amiberry_options.default_open_gui_key);
	if (enter_gui_button == SDL_CONTROLLER_BUTTON_INVALID)
		enter_gui_button = SDL_CONTROLLER_BUTTON_GUIDE;
	
	if (strncmp(p->quit_amiberry, "", 1) != 0)
		quit_key = SDL_GetKeyFromName(p->quit_amiberry);
	else
		quit_key = SDL_GetKeyFromName(amiberry_options.default_quit_key);

	if (strncmp(p->action_replay, "", 1) != 0)
		action_replay_button = SDL_GetKeyFromName(p->action_replay);
	else
		action_replay_button = SDL_GetKeyFromName(amiberry_options.default_ar_key);
	if (action_replay_button == 0)
		action_replay_button = SDLK_PAUSE;

	if (strncmp(p->fullscreen_toggle, "", 1) != 0)
		fullscreen_key = SDL_GetKeyFromName(p->fullscreen_toggle);
	else
		fullscreen_key = SDL_GetKeyFromName(amiberry_options.default_fullscreen_toggle_key);
}

int pissoff_value = 15000 * CYCLE_UNIT;

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

static int pausemouseactive;

void resumesoundpaused(void)
{
	resume_sound();
#ifdef AHI
	ahi_open_sound();
	ahi2_pause_sound(0);
#endif
}

void setsoundpaused(void)
{
	pause_sound();
#ifdef AHI
	ahi_close_sound();
	ahi2_pause_sound(1);
#endif
}

bool resumepaused(int priority)
{
	//write_log (_T("resume %d (%d)\n"), priority, pause_emulation);
	if (pause_emulation > priority)
		return false;
	if (!pause_emulation)
		return false;
	devices_unpause();
	resumesoundpaused();
	if (pausemouseactive)
	{
		pausemouseactive = 0;
		setmouseactive(isfullscreen() > 0 ? 1 : -1);
	}
	pause_emulation = 0;
	setsystime();
	return true;
}

bool setpaused(int priority)
{
	//write_log (_T("pause %d (%d)\n"), priority, pause_emulation);
	if (pause_emulation > priority)
		return false;
	pause_emulation = priority;
	devices_pause();
	setsoundpaused();
	pausemouseactive = 1;
	if (isfullscreen() <= 0) {
		pausemouseactive = mouseactive;
		setmouseactive(0);
	}
	return true;
}

void logging_init(void)
{
	if (amiberry_options.write_logfile)
	{
		static int started;
		static int first;
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
		write_log("AMIBERRY Logfile\n\n");
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

void target_execute(const char* command)
{
	set_mouse_grab(false);
	try
	{
		system(command);
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

void fix_apmodes(struct uae_prefs* p)
{
	if (p->ntscmode)
	{
		p->gfx_apmode[APMODE_NATIVE].gfx_refreshrate = 60;
		p->gfx_apmode[APMODE_RTG].gfx_refreshrate = 60;
	}
	else
	{
		p->gfx_apmode[APMODE_NATIVE].gfx_refreshrate = 50;
		p->gfx_apmode[APMODE_RTG].gfx_refreshrate = 50;
	}

	fixup_prefs_dimensions(p);
}

void target_fixup_options(struct uae_prefs* p)
{
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

	if (p->cs_cd32cd && p->cs_cd32nvram && (p->cs_compatible == CP_GENERIC || p->cs_compatible == 0))
	{
		// Old config without cs_compatible, but other cd32-flags
		p->cs_compatible = CP_CD32;
		built_in_chipset_prefs(p);
	}

	if (p->cs_cd32cd && p->cartfile[0])
	{
		p->cs_cd32fmv = true;
	}

	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_R8G8B8A8;

	if (p->cachesize <= 0)
		p->compfpu = false;

	// Fix old height values, which were 50% of the real height
	if (p->gfx_monitor.gfx_size_win.height < AMIGA_HEIGHT_MAX && p->gfx_resolution > 0)
		p->gfx_monitor.gfx_size_win.height = p->gfx_monitor.gfx_size_win.height * 2;

	fix_apmodes(p);
	set_key_configs(p);
}

void target_default_options(struct uae_prefs* p, int type)
{
	p->fast_copper = 0;
	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5PC | RGBFF_R8G8B8A8;

	p->kbd_led_num = -1; // No status on numlock
	p->kbd_led_scr = -1; // No status on scrollock

	p->gfx_monitor.gfx_size_win.width = amiberry_options.default_width;
	p->gfx_monitor.gfx_size_win.height = amiberry_options.default_height;
	
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

	p->allow_host_run = false;
	p->active_capture_priority = 1;
	p->active_nocapture_pause = false;
	p->active_nocapture_nosound = false;
	p->inactive_priority = 0;
	p->inactive_nosound = false;
	p->inactive_pause = false;
	p->inactive_input = 0;
	p->minimized_priority = 0;
	p->minimized_pause = true;
	p->minimized_nosound = true;
	p->minimized_input = 0;
	
	p->input_analog_remap = false;

	p->use_retroarch_quit = amiberry_options.default_retroarch_quit;
	p->use_retroarch_menu = amiberry_options.default_retroarch_menu;
	p->use_retroarch_reset = amiberry_options.default_retroarch_reset;

#ifdef ANDROID
	p->onScreen = 1;
	p->onScreen_textinput = 1;
	p->onScreen_dpad = 1;
	p->onScreen_button1 = 1;
	p->onScreen_button2 = 1;
	p->onScreen_button3 = 1;
	p->onScreen_button4 = 1;
	p->onScreen_button5 = 0;
	p->onScreen_button6 = 0;
	p->custom_position = 0;
	p->pos_x_textinput = 0;
	p->pos_y_textinput = 0;
	p->pos_x_dpad = 4;
	p->pos_y_dpad = 215;
	p->pos_x_button1 = 430;
	p->pos_y_button1 = 286;
	p->pos_x_button2 = 378;
	p->pos_y_button2 = 286;
	p->pos_x_button3 = 430;
	p->pos_y_button3 = 214;
	p->pos_x_button4 = 378;
	p->pos_y_button4 = 214;
	p->pos_x_button5 = 430;
	p->pos_y_button5 = 142;
	p->pos_x_button6 = 378;
	p->pos_y_button6 = 142;
	p->extfilter = 1;
	p->quickSwitch = 0;
	p->floatingJoystick = 0;
	p->disableMenuVKeyb = 0;
#endif
}

void target_save_options(struct zfile* f, struct uae_prefs* p)
{
	cfgfile_write_bool(f, _T("amiberry.gfx_auto_height"), p->gfx_auto_height);
	cfgfile_write(f, _T("amiberry.gfx_correct_aspect"), _T("%d"), p->gfx_correct_aspect);
	cfgfile_write(f, _T("amiberry.kbd_led_num"), _T("%d"), p->kbd_led_num);
	cfgfile_write(f, _T("amiberry.kbd_led_scr"), _T("%d"), p->kbd_led_scr);
	cfgfile_write(f, _T("amiberry.scaling_method"), _T("%d"), p->scaling_method);

	cfgfile_dwrite_str(f, _T("amiberry.open_gui"), p->open_gui);
	cfgfile_dwrite_str(f, _T("amiberry.quit_amiberry"), p->quit_amiberry);
	cfgfile_dwrite_str(f, _T("amiberry.action_replay"), p->action_replay);
	cfgfile_dwrite_str(f, _T("amiberry.fullscreen_toggle"), p->fullscreen_toggle);
	cfgfile_write_bool(f, _T("amiberry.allow_host_run"), p->allow_host_run);
	cfgfile_write_bool(f, _T("amiberry.use_analogue_remap"), p->input_analog_remap);

	cfgfile_write_bool(f, _T("amiberry.use_retroarch_quit"), p->use_retroarch_quit);
	cfgfile_write_bool(f, _T("amiberry.use_retroarch_menu"), p->use_retroarch_menu);
	cfgfile_write_bool(f, _T("amiberry.use_retroarch_reset"), p->use_retroarch_reset);

	cfgfile_target_dwrite(f, _T("cpu_idle"), _T("%d"), p->cpu_idle);
	cfgfile_write(f, _T("amiberry.active_priority"), _T("%d"), p->active_capture_priority);
	cfgfile_target_dwrite_bool(f, _T("active_not_captured_nosound"), p->active_nocapture_nosound);
	cfgfile_target_dwrite_bool(f, _T("active_not_captured_pause"), p->active_nocapture_pause);
	cfgfile_write(f, _T("amiberry.inactive_priority"), _T("%d"), p->inactive_priority);
	cfgfile_target_dwrite_bool(f, _T("inactive_nosound"), p->inactive_nosound);
	cfgfile_target_dwrite_bool(f, _T("inactive_pause"), p->inactive_pause);
	cfgfile_target_dwrite(f, _T("inactive_input"), _T("%d"), p->inactive_input);
	cfgfile_write(f, _T("amiberry.minimized_priority"), _T("%d"), p->minimized_priority);
	cfgfile_target_dwrite_bool(f, _T("minimized_nosound"), p->minimized_nosound);
	cfgfile_target_dwrite_bool(f, _T("minimized_pause"), p->minimized_pause);
	cfgfile_write(f, _T("amiberry.minimized_input"), _T("%d"), p->minimized_input);
	
#ifdef ANDROID
	cfgfile_write(f, "amiberry.onscreen", "%d", p->onScreen);
	cfgfile_write(f, "amiberry.onscreen_textinput", "%d", p->onScreen_textinput);
	cfgfile_write(f, "amiberry.onscreen_dpad", "%d", p->onScreen_dpad);
	cfgfile_write(f, "amiberry.onscreen_button1", "%d", p->onScreen_button1);
	cfgfile_write(f, "amiberry.onscreen_button2", "%d", p->onScreen_button2);
	cfgfile_write(f, "amiberry.onscreen_button3", "%d", p->onScreen_button3);
	cfgfile_write(f, "amiberry.onscreen_button4", "%d", p->onScreen_button4);
	cfgfile_write(f, "amiberry.onscreen_button5", "%d", p->onScreen_button5);
	cfgfile_write(f, "amiberry.onscreen_button6", "%d", p->onScreen_button6);
	cfgfile_write(f, "amiberry.custom_position", "%d", p->custom_position);
	cfgfile_write(f, "amiberry.pos_x_textinput", "%d", p->pos_x_textinput);
	cfgfile_write(f, "amiberry.pos_y_textinput", "%d", p->pos_y_textinput);
	cfgfile_write(f, "amiberry.pos_x_dpad", "%d", p->pos_x_dpad);
	cfgfile_write(f, "amiberry.pos_y_dpad", "%d", p->pos_y_dpad);
	cfgfile_write(f, "amiberry.pos_x_button1", "%d", p->pos_x_button1);
	cfgfile_write(f, "amiberry.pos_y_button1", "%d", p->pos_y_button1);
	cfgfile_write(f, "amiberry.pos_x_button2", "%d", p->pos_x_button2);
	cfgfile_write(f, "amiberry.pos_y_button2", "%d", p->pos_y_button2);
	cfgfile_write(f, "amiberry.pos_x_button3", "%d", p->pos_x_button3);
	cfgfile_write(f, "amiberry.pos_y_button3", "%d", p->pos_y_button3);
	cfgfile_write(f, "amiberry.pos_x_button4", "%d", p->pos_x_button4);
	cfgfile_write(f, "amiberry.pos_y_button4", "%d", p->pos_y_button4);
	cfgfile_write(f, "amiberry.pos_x_button5", "%d", p->pos_x_button5);
	cfgfile_write(f, "amiberry.pos_y_button5", "%d", p->pos_y_button5);
	cfgfile_write(f, "amiberry.pos_x_button6", "%d", p->pos_x_button6);
	cfgfile_write(f, "amiberry.pos_y_button6", "%d", p->pos_y_button6);
	cfgfile_write(f, "amiberry.floating_joystick", "%d", p->floatingJoystick);
	cfgfile_write(f, "amiberry.disable_menu_vkeyb", "%d", p->disableMenuVKeyb);
#endif
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
#ifdef ANDROID
	int result = (cfgfile_intval(option, value, "onscreen", &p->onScreen, 1)
		|| cfgfile_intval(option, value, "onscreen_textinput", &p->onScreen_textinput, 1)
		|| cfgfile_intval(option, value, "onscreen_dpad", &p->onScreen_dpad, 1)
		|| cfgfile_intval(option, value, "onscreen_button1", &p->onScreen_button1, 1)
		|| cfgfile_intval(option, value, "onscreen_button2", &p->onScreen_button2, 1)
		|| cfgfile_intval(option, value, "onscreen_button3", &p->onScreen_button3, 1)
		|| cfgfile_intval(option, value, "onscreen_button4", &p->onScreen_button4, 1)
		|| cfgfile_intval(option, value, "onscreen_button5", &p->onScreen_button5, 1)
		|| cfgfile_intval(option, value, "onscreen_button6", &p->onScreen_button6, 1)
		|| cfgfile_intval(option, value, "custom_position", &p->custom_position, 1)
		|| cfgfile_intval(option, value, "pos_x_textinput", &p->pos_x_textinput, 1)
		|| cfgfile_intval(option, value, "pos_y_textinput", &p->pos_y_textinput, 1)
		|| cfgfile_intval(option, value, "pos_x_dpad", &p->pos_x_dpad, 1)
		|| cfgfile_intval(option, value, "pos_y_dpad", &p->pos_y_dpad, 1)
		|| cfgfile_intval(option, value, "pos_x_button1", &p->pos_x_button1, 1)
		|| cfgfile_intval(option, value, "pos_y_button1", &p->pos_y_button1, 1)
		|| cfgfile_intval(option, value, "pos_x_button2", &p->pos_x_button2, 1)
		|| cfgfile_intval(option, value, "pos_y_button2", &p->pos_y_button2, 1)
		|| cfgfile_intval(option, value, "pos_x_button3", &p->pos_x_button3, 1)
		|| cfgfile_intval(option, value, "pos_y_button3", &p->pos_y_button3, 1)
		|| cfgfile_intval(option, value, "pos_x_button4", &p->pos_x_button4, 1)
		|| cfgfile_intval(option, value, "pos_y_button4", &p->pos_y_button4, 1)
		|| cfgfile_intval(option, value, "pos_x_button5", &p->pos_x_button5, 1)
		|| cfgfile_intval(option, value, "pos_y_button5", &p->pos_y_button5, 1)
		|| cfgfile_intval(option, value, "pos_x_button6", &p->pos_x_button6, 1)
		|| cfgfile_intval(option, value, "pos_y_button6", &p->pos_y_button6, 1)
		|| cfgfile_intval(option, value, "floating_joystick", &p->floatingJoystick, 1)
		|| cfgfile_intval(option, value, "disable_menu_vkeyb", &p->disableMenuVKeyb, 1)
		);
	if (result)
		return 1;
#endif
	if (cfgfile_yesno(option, value, _T("allow_host_run"), &p->allow_host_run))
		return 1;
	if (cfgfile_yesno(option, value, _T("use_retroarch_quit"), &p->use_retroarch_quit))
		return 1;
	if (cfgfile_yesno(option, value, _T("use_retroarch_menu"), &p->use_retroarch_menu))
		return 1;
	if (cfgfile_yesno(option, value, _T("use_retroarch_reset"), &p->use_retroarch_reset))
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
	if (cfgfile_intval(option, value, _T("cpu_idle"), &p->cpu_idle, 1))
		return 1;
	if (cfgfile_intval(option, value, _T("active_priority"), &p->active_capture_priority, 1))
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

	const char* ptr = strstr(const_cast<char*>(filename), ".rp9");
	if (ptr)
	{
		// Load rp9 config
		result = rp9_parse_file(p, filename);
		if (result)
			extract_filename(filename, last_loaded_config);
	}
	else
	{
		ptr = strstr(const_cast<char*>(filename), ".uae");
		if (ptr)
		{
			auto config_type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
			result = cfgfile_load(p, filename, &config_type, 0, 1);
		}
		if (result)
			extract_filename(filename, last_loaded_config);
	}

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

	// Swap Win keys with Alt keys?
	// This helps with keyboards that may not have 2 Win keys and no Menu key either
	snprintf(buffer, MAX_DPATH, "swap_win_alt_keys=%s\n", amiberry_options.swap_win_alt_keys ? "yes" : "no");
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
	int i;
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
					cfgfile_yesno(option, value, "swap_win_alt_keys", &amiberry_options.swap_win_alt_keys);
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

	auto* const ad = &adisplays;
	ad->picasso_requested_on = false;
	ad->picasso_on = false;
}

bool target_can_autoswitchdevice(void)
{
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

bool target_isrelativemode(void)
{
	return true;
}

int main(int argc, char* argv[])
{
	struct sigaction action{};

	max_uae_width = 1920;
	max_uae_height = 1080;

	// Get startup path
#ifdef ANDROID
    strncpy(start_path_data, getenv("EXTERNAL_FILES_DIR"), MAX_DPATH - 1);
#else
	getcwd(start_path_data, MAX_DPATH);
#endif
	rename_old_adfdir();
	load_amiberry_settings();
	rp9_init();

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
	rp9_cleanup();

	logging_cleanup();

	if (host_poweroff)
		target_shutdown();
	return 0;
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

void toggle_mousegrab()
{
	// Release mouse
	if (mouse_grabbed)
	{
		mouse_grabbed = false;
		SDL_ShowCursor(SDL_ENABLE);
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
	else
	{
		mouse_grabbed = true;
		SDL_ShowCursor(SDL_DISABLE);
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
}

void set_mouse_grab(const bool grab)
{
#ifdef USE_DISPMANX
	if (currprefs.allow_host_run)
	{
		if (grab)
			change_layer_number(0);
		else
			change_layer_number(-128);
	}
#endif
	if (grab != mouse_grabbed)
		toggle_mousegrab();	
}

void setminimized()
{
	if (!minimized)
		minimized = 1;
	set_inhibit_frame(IHF_WINDOWHIDDEN);
}

void unsetminimized()
{
	if (minimized > 0)
		full_redraw_all();
	minimized = 0;
	clear_inhibit_frame(IHF_WINDOWHIDDEN);
}

static void amiberry_inactive(int minimized)
{
	focus = 0;
	recapture = 0;
	setmouseactive(0);
	clipboard_active(1, 0);

	if (!quit_program) {
		if (minimized) {
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
	} else {
		inputdevice_unacquire();
	}
	setpriority(currprefs.inactive_priority);
}

static void amiberry_active(int minimized)
{
	setpriority(currprefs.active_capture_priority);
	
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
			else if (currprefs.inactive_pause)
				resumepaused(2);
		}
		sound_closed = 0;
	}
	//getcapslock();
	inputdevice_acquire(TRUE);
	if (isfullscreen() > 0)
		setmouseactive(1);
	clipboard_active(1, 1);
}

static void setmouseactive2(int active, bool allowpause)
{
	if (active == 0)
		set_mouse_grab(false);
	if (mouseactive == active && active >= 0)
		return;

	if (active == 1 && !(currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC)) {
		set_mouse_grab(true);
	}
	
	if (active < 0)
		active = 1;

	mouseactive = active ? 1 : 0;

	recapture = 0;
	
	if (isfullscreen() <= 0 && (currprefs.input_mouse_untrap & MOUSEUNTRAP_MAGIC) && currprefs.input_tablet > 0) {
		if (mousehack_alive())
			return;
	}
	
	if (active) {
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
			setsoundpaused();
			sound_closed = -1;
	}
}

void setmouseactive(int active)
{
	if (active > 1)
		SDL_RaiseWindow(sdl_window);
	setmouseactive2(active, true);
}

void enablecapture()
{
	if (pause_emulation > 2)
		return;
	setmouseactive(1);
	if (sound_closed < 0) {
		resumesoundpaused();
		sound_closed = 0;
	}
	if (currprefs.inactive_pause || currprefs.active_nocapture_pause)
		resumepaused(2);
}

void disablecapture()
{
	setmouseactive(0);
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

void process_event(SDL_Event event)
{
	const auto* keystate = SDL_GetKeyboardState(nullptr);
	
	if (event.type == SDL_WINDOWEVENT)
	{
		switch (event.window.event)
		{
		case SDL_WINDOWEVENT_MINIMIZED:
			setminimized();
			amiberry_inactive(minimized);
			break;
		case SDL_WINDOWEVENT_RESTORED:
			amiberry_active(minimized);
			unsetminimized();
			break;
		case SDL_WINDOWEVENT_ENTER:
			mouseinside = true;
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			mouseinside = true;
			set_mouse_grab(true);
			amiberry_active(minimized);
			break;
		case SDL_WINDOWEVENT_LEAVE:
			mouseinside = false;
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			mouseinside = false;
			set_mouse_grab(false);
			amiberry_inactive(minimized);
			break;
		case SDL_WINDOWEVENT_CLOSE:
			uae_quit();
			break;

		default:
			break;
		}
	}

	switch (event.type)
	{
	case SDL_QUIT:
		uae_quit();
		break;

	case SDL_JOYDEVICEADDED:
	case SDL_CONTROLLERDEVICEADDED:
		write_log("SDL Controller/Joystick device added! Re-running import joysticks...\n");
		import_joysticks();
		break;

	case SDL_JOYDEVICEREMOVED:
	case SDL_CONTROLLERDEVICEREMOVED:
		write_log("SDL Controller/Joystick device removed!\n");
		break;

	case SDL_CONTROLLERBUTTONDOWN:
		if (event.cbutton.button == enter_gui_button)
		{
			inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
			break;
		}
		break;

	case SDL_KEYDOWN:
	{
		// if the key belongs to a "retro arch joystick" ignore it
		// ONLY when in game though, we need to remove the joysticks really 
		// if we want to use the KB
		// i've added this so when using the joysticks it doesn't hit the 'r' key for some games
		// which starts a replay!!!
		const auto ok_to_use = !key_used_by_retroarch_joy(event.key.keysym.scancode);
		if (ok_to_use)
		{
			if (event.key.repeat == 0)
			{
				// If the Enter GUI key was pressed, handle it
				if (enter_gui_key && event.key.keysym.sym == enter_gui_key)
				{
					inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
					break;
				}

				// If the Quit emulator key was pressed, handle it
				if (quit_key && event.key.keysym.sym == quit_key)
				{
					inputdevice_add_inputcode(AKS_QUIT, 1, nullptr);
					break;
				}

				if (action_replay_button && event.key.keysym.sym == action_replay_button)
				{
					inputdevice_add_inputcode(AKS_FREEZEBUTTON, 1, nullptr);
					break;
				}

				if (fullscreen_key && event.key.keysym.sym == fullscreen_key)
				{
					inputdevice_add_inputcode(AKS_TOGGLEWINDOWEDFULLSCREEN, 1, nullptr);
					break;
				}
			}
			// If the reset combination was pressed, handle it
			if (amiberry_options.swap_win_alt_keys)
			{
				if (keystate[SDL_SCANCODE_LCTRL] && keystate[SDL_SCANCODE_LALT] && (keystate[SDL_SCANCODE_RALT] || keystate[SDL_SCANCODE_APPLICATION]))
				{
					uae_reset(0, 1);
					break;
				}
			}
			else if (keystate[SDL_SCANCODE_LCTRL] && keystate[SDL_SCANCODE_LGUI] && (keystate[SDL_SCANCODE_RGUI] || keystate[SDL_SCANCODE_APPLICATION]))
			{
				uae_reset(0, 1);
				break;
			}

			if (event.key.repeat == 0)
			{
				if (event.key.keysym.sym == SDLK_CAPSLOCK)
				{
					// Treat CAPSLOCK as a toggle. If on, set off and vice/versa
					ioctl(0, KDGKBLED, &kbd_flags);
					ioctl(0, KDGETLED, &kbd_led_status);
					if (kbd_flags & 07 & LED_CAP)
					{
						// On, so turn off
						kbd_led_status &= ~LED_CAP;
						kbd_flags &= ~LED_CAP;
						inputdevice_do_keyboard(AK_CAPSLOCK, 0);
					}
					else
					{
						// Off, so turn on
						kbd_led_status |= LED_CAP;
						kbd_flags |= LED_CAP;
						inputdevice_do_keyboard(AK_CAPSLOCK, 1);
					}
					ioctl(0, KDSETLED, kbd_led_status);
					ioctl(0, KDSKBLED, kbd_flags);
					break;
				}

				if (event.key.keysym.sym == SDLK_SYSREQ)
					clipboard_disable(true);
				
				// Handle all other keys
				if (amiberry_options.swap_win_alt_keys)
				{
					if (event.key.keysym.scancode == SDL_SCANCODE_LALT)
						event.key.keysym.scancode = SDL_SCANCODE_LGUI;
					else if (event.key.keysym.scancode == SDL_SCANCODE_RALT)
						event.key.keysym.scancode = SDL_SCANCODE_RGUI;
				}
				inputdevice_translatekeycode(0, event.key.keysym.scancode, 1, false);
			}
		}
	}
	break;
	case SDL_KEYUP:
	{
		const auto ok_to_use = !key_used_by_retroarch_joy(event.key.keysym.scancode);
		if (ok_to_use)
		{
			if (event.key.repeat == 0)
			{
				if (amiberry_options.swap_win_alt_keys)
				{
					if (event.key.keysym.scancode == SDL_SCANCODE_LALT)
						event.key.keysym.scancode = SDL_SCANCODE_LGUI;
					else if (event.key.keysym.scancode == SDL_SCANCODE_RALT)
						event.key.keysym.scancode = SDL_SCANCODE_RGUI;
				}
				inputdevice_translatekeycode(0, event.key.keysym.scancode, 0, true);
			}
		}
	}
	break;

	case SDL_FINGERDOWN:
		setmousebuttonstate(0, 0, 1);
		break;

	case SDL_MOUSEBUTTONDOWN:
		if (event.button.button == SDL_BUTTON_LEFT && mouseinside && !currprefs.input_mouse_untrap)
			set_mouse_grab(true);
		if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
				setmousebuttonstate(0, 0, 1);
			if (event.button.button == SDL_BUTTON_RIGHT)
				setmousebuttonstate(0, 1, 1);
			if (event.button.button == SDL_BUTTON_MIDDLE)
				setmousebuttonstate(0, 2, 1);
		}
		break;

	case SDL_FINGERUP:
		setmousebuttonstate(0, 0, 0);
		break;

	case SDL_MOUSEBUTTONUP:
		if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
		{
			if (event.button.button == SDL_BUTTON_LEFT)
				setmousebuttonstate(0, 0, 0);
			if (event.button.button == SDL_BUTTON_RIGHT)
				setmousebuttonstate(0, 1, 0);
			if (event.button.button == SDL_BUTTON_MIDDLE)
			{
				if (currprefs.input_mouse_untrap)
					toggle_mousegrab();
				else
					setmousebuttonstate(0, 2, 0);
			}
		}
		break;

	case SDL_FINGERMOTION:
		//TODO this doesn't work yet
		setmousestate(0, 0, event.motion.xrel, 0);
		setmousestate(0, 1, event.motion.yrel, 0);
		break;

	case SDL_MOUSEMOTION:
		if (recapture && isfullscreen() <= 0) {
			enablecapture();
			break;
		}

		if (currprefs.input_tablet >= TABLET_MOUSEHACK)
		{
			/* absolute */
			setmousestate(0, 0, event.motion.x, 1);
			setmousestate(0, 1, event.motion.y, 1);
			break;
		}

		if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
		{
			/* relative */
#if defined (ANDROID)
			if (event.motion.x == 0 && x > -4)
				x = -4;
			if (event.motion.y == 0 && y > -4)
				y = -4;
			if (event.motion.x == currprefs.gfx_monitor.gfx_size.width - 1 && x < 4)
				x = 4;
			if (event.motion.y == currprefs.gfx_monitor.gfx_size.height - 1 && y < 4)
				y = 4;
#endif //ANDROID
			setmousestate(0, 0, event.motion.xrel, 0);
			setmousestate(0, 1, event.motion.yrel, 0);
			break;
		}
		break;

	case SDL_MOUSEWHEEL:
		if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
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
		break;

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
	else {
		// FIXME: Ideally, we would want to avoid this alloc/free
		// when the clipboard hasn't changed.
		if (SDL_HasClipboardText() == SDL_TRUE)
		{
			auto* clipboard_host = SDL_GetClipboardText();
			uae_clipboard_put_text(clipboard_host);
			SDL_free(clipboard_host);
		}
	}
}

int handle_msgpump()
{
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
	static auto was_paused = 0;

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
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			process_event(event);
		}
		
		inputdevicefunc_keyboard.read();
		inputdevicefunc_mouse.read();
		inputdevicefunc_joystick.read();
		inputdevice_handle_inputcode();
	}
	if (was_paused && (!pause_emulation || quit_program))
	{
		//updatedisplayarea();
		pause_emulation = was_paused;
		resumepaused(was_paused);
		sound_closed = 0;
		was_paused = 0;
	}

	return pause_emulation != 0;
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
		write_log("\n-----------------> STUB: get_plugin_path, "
			"size: %d, path: %s\n", len, path);
		out[0] = '\0';
	}
	return TRUE;
}