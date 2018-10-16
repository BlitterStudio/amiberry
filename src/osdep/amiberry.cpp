/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */

#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdarg>
#include <asm/sigcontext.h>
#include <csignal>
#include <dlfcn.h>
#ifndef ANDROID
#include <execinfo.h>
#endif
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "custom.h"
#include "inputdevice.h"
#include "disk.h"
#include "savestate.h"
#include "rommgr.h"
#include "zfile.h"
#include <SDL.h>
#include "amiberry_rp9.h"
#include "machdep/rpt.h"

#include "threaddep/thread.h"
#include "include/memory.h"
#include "keyboard.h"
#include "rtgmodes.h"
#include "gfxboard.h"
#include "amiberry_gfx.h"
#ifdef USE_SDL2
#include <map>
#endif

#ifdef WITH_LOGGING
extern FILE *debugfile;
#endif

int quickstart_start = 1;
int quickstart_model = 0;
int quickstart_conf = 0;
bool host_poweroff = false;
bool read_config_descriptions = true;

extern void signal_segv(int signum, siginfo_t* info, void* ptr);
extern void signal_buserror(int signum, siginfo_t* info, void* ptr);
extern void signal_term(int signum, siginfo_t* info, void* ptr);

extern void SetLastActiveConfig(const char* filename);

char start_path_data[MAX_DPATH];
char currentDir[MAX_DPATH];

#include <linux/kd.h>
#include <sys/ioctl.h>
unsigned char kbd_led_status;
char kbd_flags;

static char config_path[MAX_DPATH];
static char rom_path[MAX_DPATH];
static char rp9_path[MAX_DPATH];
static char controllers_path[MAX_DPATH];
static char retroarch_file[MAX_DPATH];

char last_loaded_config[MAX_DPATH] = {'\0'};

int max_uae_width;
int max_uae_height;

extern "C" int main(int argc, char* argv[]);

void sleep_millis(int ms)
{
	usleep(ms * 1000);
}

int sleep_millis_main(int ms)
{
	unsigned long start = read_processor_time();
	usleep(ms * 1000);
	idletime += read_processor_time() - start;
	return 0;
}

void logging_init(void)
{
#ifdef WITH_LOGGING
  static int started;
  static int first;
  char debugfilename[MAX_DPATH];

  if (first > 1) {
  	write_log ("***** RESTART *****\n");
	  return;
  }
  if (first == 1) {
  	if (debugfile)
	    fclose (debugfile);
    debugfile = 0;
  }

	sprintf(debugfilename, "%s/amiberry_log.txt", start_path_data);
	if (!debugfile)
		debugfile = fopen(debugfilename, "wt");

	first++;
	write_log("AMIBERRY Logfile\n\n");
#endif
}

void logging_cleanup(void)
{
#ifdef WITH_LOGGING
	if (debugfile)
		fclose(debugfile);
	debugfile = 0;
#endif
}


void stripslashes(TCHAR *p)
{
	while (_tcslen(p) > 0 && (p[_tcslen(p) - 1] == '\\' || p[_tcslen(p) - 1] == '/'))
		p[_tcslen(p) - 1] = 0;
}

void fixtrailing(TCHAR *p)
{
	if (_tcslen(p) == 0)
		return;
	if (p[_tcslen(p) - 1] == '/' || p[_tcslen(p) - 1] == '\\')
		return;
	_tcscat(p, "/");
}

void getpathpart(TCHAR *outpath, int size, const TCHAR *inpath)
{
	_tcscpy(outpath, inpath);
	const auto p = _tcsrchr(outpath, '/');
	if (p)
		p[0] = 0;
	fixtrailing(outpath);
}

void getfilepart(TCHAR *out, int size, const TCHAR *path)
{
	out[0] = 0;
	const auto p = _tcsrchr(path, '/');
	if (p)
		_tcscpy(out, p + 1);
	else
		_tcscpy(out, path);
}

uae_u8 *target_load_keyfile(struct uae_prefs *p, const char *path, int *sizep, char *name)
{
	return nullptr;
}

void target_run(void)
{
	// Reset counter for access violations
	init_max_signals();
}

void target_quit(void)
{
}

void fix_apmodes(struct uae_prefs *p)
{
	if (p->ntscmode)
	{
		p->gfx_apmode[0].gfx_refreshrate = 60;
		p->gfx_apmode[1].gfx_refreshrate = 60;
	}
	else
	{
		p->gfx_apmode[0].gfx_refreshrate = 50;
		p->gfx_apmode[1].gfx_refreshrate = 50;
	}

	fixup_prefs_dimensions(p);
}

void target_fixup_options(struct uae_prefs* p)
{
	p->rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;

	if (z3_base_adr == Z3BASE_REAL) {
		// map Z3 memory at real address (0x40000000)
		p->z3_mapping_mode = Z3MAPPING_REAL;
		p->z3autoconfig_start = z3_base_adr;
	}
	else {
		// map Z3 memory at UAE address (0x10000000)
		p->z3_mapping_mode = Z3MAPPING_UAE;
		p->z3autoconfig_start = z3_base_adr;
	}

	if (p->cs_cd32cd && p->cs_cd32nvram && (p->cs_compatible == CP_GENERIC || p->cs_compatible == 0)) {
		// Old config without cs_compatible, but other cd32-flags
		p->cs_compatible = CP_CD32;
		built_in_chipset_prefs(p);
	}

	if (p->cs_cd32cd && p->cartfile[0]) {
		p->cs_cd32fmv = true;
	}

	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5 | RGBFF_R8G8B8A8;
	if (p->gfx_size.width == 0)
		p->gfx_size.width = 640;
	if (p->gfx_size.height == 0)
		p->gfx_size.height = 256;
	p->gfx_resolution = p->gfx_size.width > 600 ? 1 : 0;

	if (p->gfx_vresolution && !can_have_linedouble)
		p->gfx_vresolution = 0;

	if (p->cachesize > 0)
		p->fpu_no_unimplemented = false;
	else
		p->fpu_no_unimplemented = true;

	if (!p->cachesize > 0)
		p->compfpu = false;

	fix_apmodes(p);
}

void target_default_options(struct uae_prefs* p, int type)
{
	p->fast_copper = 0;
	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5 | RGBFF_R8G8B8A8;

	p->kbd_led_num = -1; // No status on numlock
	p->kbd_led_scr = -1; // No status on scrollock

	p->vertical_offset = OFFSET_Y_ADJUST;
	p->gfx_correct_aspect = 1; // Default is Enabled
	p->scaling_method = -1; //Default is Auto
	p->gfx_vresolution = 0; // Disabled by default due to big performance hit under SDL2

	_tcscpy(p->open_gui, "F12");
	_tcscpy(p->quit_amiberry, "");
	_tcscpy(p->action_replay, "Pause");
	_tcscpy(p->fullscreen_toggle, "");

	p->input_analog_remap = false;

	p->use_retroarch_quit = true;
	p->use_retroarch_menu = true;
	p->use_retroarch_reset = false;

#ifdef ANDROIDSDL
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

	p->cr[CHIPSET_REFRESH_PAL].locked = true;
	p->cr[CHIPSET_REFRESH_PAL].vsync = 1;

	p->cr[CHIPSET_REFRESH_NTSC].locked = true;
	p->cr[CHIPSET_REFRESH_NTSC].vsync = 1;

	p->cr[0].index = 0;
	p->cr[0].horiz = -1;
	p->cr[0].vert = -1;
	p->cr[0].lace = -1;
	p->cr[0].resolution = 0;
	p->cr[0].vsync = -1;
	p->cr[0].rate = 60.0;
	p->cr[0].ntsc = 1;
	p->cr[0].locked = true;
	p->cr[0].rtg = true;
	_tcscpy(p->cr[0].label, _T("RTG"));
}

void target_save_options(struct zfile* f, struct uae_prefs* p)
{
	cfgfile_write(f, "amiberry.vertical_offset", "%d", p->vertical_offset - OFFSET_Y_ADJUST);
	cfgfile_write(f, "amiberry.hide_idle_led", "%d", p->hide_idle_led);
	cfgfile_write(f, _T("amiberry.gfx_correct_aspect"), _T("%d"), p->gfx_correct_aspect);
	cfgfile_write(f, _T("amiberry.kbd_led_num"), _T("%d"), p->kbd_led_num);
	cfgfile_write(f, _T("amiberry.kbd_led_scr"), _T("%d"), p->kbd_led_scr);
	cfgfile_write(f, _T("amiberry.scaling_method"), _T("%d"), p->scaling_method);

	cfgfile_dwrite_str(f, _T("amiberry.open_gui"), p->open_gui);
	cfgfile_dwrite_str(f, _T("amiberry.quit_amiberry"), p->quit_amiberry);
	cfgfile_dwrite_str(f, _T("amiberry.action_replay"), p->action_replay);
	cfgfile_dwrite_str(f, _T("amiberry.fullscreen_toggle"), p->fullscreen_toggle);
	cfgfile_write_bool(f, _T("amiberry.use_analogue_remap"), p->input_analog_remap);        

	cfgfile_write_bool(f, _T("amiberry.use_retroarch_quit"), p->use_retroarch_quit);
	cfgfile_write_bool(f, _T("amiberry.use_retroarch_menu"), p->use_retroarch_menu);
	cfgfile_write_bool(f, _T("amiberry.use_retroarch_reset"), p->use_retroarch_reset);

#ifdef ANDROIDSDL
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

TCHAR *target_expand_environment(const TCHAR *path, TCHAR *out, int maxlen)
{
	if (out == nullptr)
		return strdup(path);

	_tcscpy(out, path);
	return out;
}

int target_parse_option(struct uae_prefs* p, const char* option, const char* value)
{
#ifdef ANDROIDSDL
		|| cfgfile_intval(option, value, "onscreen", &p->onScreen, 1)
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
#endif

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

	if (cfgfile_intval(option, value, "vertical_offset", &p->vertical_offset, 1))
	{
		p->vertical_offset += OFFSET_Y_ADJUST;
		return 1;
	}
	if (cfgfile_intval(option, value, "hide_idle_led", &p->hide_idle_led, 1))
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
	return 0;
}

void fetch_datapath(char *out, int size)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/", size);
}

void fetch_saveimagepath(char *out, int size, int dir)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/savestates/", size);
}

void fetch_configurationpath(char *out, int size)
{
	fixtrailing(config_path);
	strncpy(out, config_path, size);
}

void set_configurationpath(char *newpath)
{
	strncpy(config_path, newpath, MAX_DPATH);
}

void fetch_controllerspath(char* out, int size)
{
	fixtrailing(controllers_path);
	strncpy(out, controllers_path, size);
}

void set_controllerspath(char* newpath)
{
	strncpy(controllers_path, newpath, MAX_DPATH);
}

void fetch_retroarchfile(char* out, int size)
{
	strncpy(out, retroarch_file, size);
}

void set_retroarchfile(char* newpath)
{
	strncpy(retroarch_file, newpath, MAX_DPATH);
}


void fetch_rompath(char* out, int size)
{
	fixtrailing(rom_path);
	strncpy(out, rom_path, size);
}

void set_rompath(char *newpath)
{
	strncpy(rom_path, newpath, MAX_DPATH);
}


void fetch_rp9path(char *out, int size)
{
	fixtrailing(rp9_path);
	strncpy(out, rp9_path, size);
}

void fetch_savestatepath(char *out, int size)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/savestates/", size);
}

void fetch_screenshotpath(char *out, int size)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/screenshots/", size);
}

int target_cfgfile_load(struct uae_prefs* p, const char* filename, int type, int isdefault)
{
	auto result = 0;

	write_log(_T("target_cfgfile_load(): load file %s\n"), filename);

	discard_prefs(p, type);
	default_prefs(p, true, 0);

	const char* ptr = strstr(const_cast<char *>(filename), ".rp9");
	if (ptr > nullptr)
	{
		// Load rp9 config
		result = rp9_parse_file(p, filename);
		if (result)
			extractFileName(filename, last_loaded_config);
	}
	else
	{
		ptr = strstr(const_cast<char *>(filename), ".uae");
		if (ptr > nullptr)
		{
			auto config_type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
			result = cfgfile_load(p, filename, &config_type, 0, 1);
		}
		if (result)
			extractFileName(filename, last_loaded_config);
	}

	if (result)
	{
		for (auto i = 0; i < p->nr_floppies; ++i)
		{
			if (!DISK_validate_filename(p, p->floppyslots[i].df, 0, nullptr, nullptr, nullptr))
				p->floppyslots[i].df[0] = 0;
			disk_insert(i, p->floppyslots[i].df);
			if (strlen(p->floppyslots[i].df) > 0)
				AddFileToDiskList(p->floppyslots[i].df, 1);
		}

		if (!isdefault)
			inputdevice_updateconfig(nullptr, p);
#ifdef WITH_LOGGING
		p->leds_on_screen = true;
#endif
		SetLastActiveConfig(filename);
	}

	return result;
}

int check_configfile(char *file)
{
	char tmp[MAX_DPATH];

	auto f = fopen(file, "rte");
	if (f)
	{
		fclose(f);
		return 1;
	}

	strncpy(tmp, file, MAX_DPATH);
	const auto ptr = strstr(tmp, ".uae");
	if (ptr > nullptr)
	{
		*(ptr + 1) = '\0';
		strncat(tmp, "conf", MAX_DPATH);
		f = fopen(tmp, "rte");
		if (f)
		{
			fclose(f);
			return 2;
		}
	}
	return 0;
}

void extractFileName(const char * str, char *buffer)
{
	auto p = str + strlen(str) - 1;
	while (*p != '/' && p > str)
		p--;
	p++;
	strncpy(buffer, p, MAX_DPATH);
}

void extractPath(char *str, char *buffer)
{
	strncpy(buffer, str, MAX_DPATH);
	auto p = buffer + strlen(buffer) - 1;
	while (*p != '/' && p > buffer)
		p--;
	p[1] = '\0';
}

void removeFileExtension(char *filename)
{
	auto p = filename + strlen(filename) - 1;
	while (p > filename && *p != '.')
	{
		*p = '\0';
		--p;
	}
	*p = '\0';
}

void ReadDirectory(const char *path, std::vector<std::string> *dirs, std::vector<std::string> *files)
{
	struct dirent *dent;

	if (dirs != nullptr)
		dirs->clear();
	if (files != nullptr)
		files->clear();

	const auto dir = opendir(path);
	if (dir != nullptr)
	{
		while ((dent = readdir(dir)) != nullptr)
		{
			if (dent->d_type == DT_DIR)
			{
				if (dirs != nullptr)
					dirs->push_back(dent->d_name);
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

void saveAdfDir(void)
{
	char path[MAX_DPATH];

	snprintf(path, MAX_DPATH, "%s/conf/adfdir.conf", start_path_data);
	const auto f = fopen(path, "we");
	if (!f)
		return;

	char buffer[MAX_DPATH];
	snprintf(buffer, MAX_DPATH, "path=%s\n", currentDir);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "config_path=%s\n", config_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "controllers_path=%s\n", controllers_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "retroarch_config=%s\n", retroarch_file);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "rom_path=%s\n", rom_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "ROMs=%d\n", lstAvailableROMs.size());
	fputs(buffer, f);
	for (auto & lstAvailableROM : lstAvailableROMs)
	{
		snprintf(buffer, MAX_DPATH, "ROMName=%s\n", lstAvailableROM->Name);
		fputs(buffer, f);
		snprintf(buffer, MAX_DPATH, "ROMPath=%s\n", lstAvailableROM->Path);
		fputs(buffer, f);
		snprintf(buffer, MAX_DPATH, "ROMType=%d\n", lstAvailableROM->ROMType);
		fputs(buffer, f);
	}

	snprintf(buffer, MAX_DPATH, "MRUDiskList=%d\n", lstMRUDiskList.size());
	fputs(buffer, f);
	for (auto & i : lstMRUDiskList)
	{
		snprintf(buffer, MAX_DPATH, "Diskfile=%s\n", i.c_str());
		fputs(buffer, f);
	}

	snprintf(buffer, MAX_DPATH, "MRUCDList=%d\n", lstMRUCDList.size());
	fputs(buffer, f);
	for (auto & i : lstMRUCDList)
	{
		snprintf(buffer, MAX_DPATH, "CDfile=%s\n", i.c_str());
		fputs(buffer, f);
	}

	snprintf(buffer, MAX_DPATH, "Quickstart=%d\n", quickstart_start);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "read_config_descriptions=%s\n", read_config_descriptions ? "yes" : "no");
	fputs(buffer, f);

	fclose(f);
}

void get_string(FILE *f, char *dst, int size)
{
	char buffer[MAX_DPATH];
	fgets(buffer, MAX_DPATH, f);
	int i = strlen(buffer);
	while (i > 0 && (buffer[i - 1] == '\t' || buffer[i - 1] == ' '
		|| buffer[i - 1] == '\r' || buffer[i - 1] == '\n'))
		buffer[--i] = '\0';
	strncpy(dst, buffer, size);
}

static void trimwsa(char *s)
{
	/* Delete trailing whitespace.  */
	int len = strlen(s);
	while (len > 0 && strcspn(s + len - 1, "\t \r\n") == 0)
		s[--len] = '\0';
}

void loadAdfDir(void)
{
	char path[MAX_DPATH];
	int i;
#ifdef ANDROID
	strncpy(currentDir, getenv("SDCARD"), MAX_DPATH);
#else
	strncpy(currentDir, start_path_data, MAX_DPATH);
#endif
	snprintf(config_path, MAX_DPATH, "%s/conf/", start_path_data);
	snprintf(controllers_path, MAX_DPATH, "%s/controllers/", start_path_data);
	snprintf(retroarch_file, MAX_DPATH, "%s/conf/retroarch.cfg", start_path_data);

#ifdef ANDROID
    char afepath[MAX_DPATH];
    snprintf(afepath, MAX_DPATH, "%s/Android/data/com.cloanto.amigaforever.essentials/files/rom/", getenv("SDCARD"));
    DIR *afedir = opendir(afepath);
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

	snprintf(path, MAX_DPATH, "%s/conf/adfdir.conf", start_path_data);

	const auto fh = zfile_fopen(path, _T("r"), ZFD_NORMAL);
	if (fh) {
		char linea[CONFIG_BLEN];
		TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
		int numROMs, numDisks, numCDs;
		auto romType = -1;
		char romName[MAX_DPATH] = { '\0' };
		char romPath[MAX_DPATH] = { '\0' };
		char tmpFile[MAX_DPATH];

		while (zfile_fgetsa(linea, sizeof linea, fh) != nullptr)
		{
			trimwsa(linea);
			if (strlen(linea) > 0) {
				if (!cfgfile_separate_linea(path, linea, option, value))
					continue;

				if (cfgfile_string(option, value, "ROMName", romName, sizeof romName)
					|| cfgfile_string(option, value, "ROMPath", romPath, sizeof romPath)
					|| cfgfile_intval(option, value, "ROMType", &romType, 1)) {
					if (strlen(romName) > 0 && strlen(romPath) > 0 && romType != -1) {
						auto tmp = new AvailableROM();
						strncpy(tmp->Name, romName, sizeof tmp->Name);
						strncpy(tmp->Path, romPath, sizeof tmp->Path);
						tmp->ROMType = romType;
						lstAvailableROMs.emplace_back(tmp);
						strncpy(romName, "", sizeof romName);
						strncpy(romPath, "", sizeof romPath);
						romType = -1;
					}
				}
				else if (cfgfile_string(option, value, "Diskfile", tmpFile, sizeof tmpFile))
				{
					auto f = fopen(tmpFile, "rbe");
					if (f != nullptr)
					{
						fclose(f);
						lstMRUDiskList.emplace_back(tmpFile);
					}
				}
				else if (cfgfile_string(option, value, "CDfile", tmpFile, sizeof tmpFile))
				{
					FILE* f = fopen(tmpFile, "rbe");
					if (f != nullptr)
					{
						fclose(f);
						lstMRUCDList.emplace_back(tmpFile);
					}
				}
				else {
					cfgfile_string(option, value, "path", currentDir, sizeof currentDir);
					cfgfile_string(option, value, "config_path", config_path, sizeof config_path);
					cfgfile_string(option, value, "controllers_path", controllers_path, sizeof controllers_path);
					cfgfile_string(option, value, "retroarch_config", retroarch_file, sizeof retroarch_file);
					cfgfile_string(option, value, "rom_path", rom_path, sizeof rom_path);
					cfgfile_intval(option, value, "ROMs", &numROMs, 1);
					cfgfile_intval(option, value, "MRUDiskList", &numDisks, 1);
					cfgfile_intval(option, value, "MRUCDList", &numCDs, 1);
					cfgfile_intval(option, value, "Quickstart", &quickstart_start, 1);
					cfgfile_yesno(option, value, "read_config_descriptions", &read_config_descriptions);
				}
			}
		}
		zfile_fclose(fh);
	}
}

void target_addtorecent(const TCHAR *name, int t)
{
}


void target_reset(void)
{
}

bool target_can_autoswitchdevice(void)
{
	return true;
}

uae_u32 emulib_target_getcpurate(uae_u32 v, uae_u32 *low)
{
	*low = 0;
	if (v == 1) {
		*low = 1e+9; /* We have nano seconds */
		return 0;
	}
	if (v == 2) {
		struct timespec ts{};
		clock_gettime(CLOCK_MONOTONIC, &ts);
		const int64_t time = int64_t(ts.tv_sec) * 1000000000 + ts.tv_nsec;
		*low = uae_u32(time & 0xffffffff);
		return uae_u32(time >> 32);
	}
	return 0;
}

void target_shutdown(void)
{
	system("sudo poweroff");
}

int main(int argc, char* argv[])
{
	struct sigaction action{};

	max_uae_width = 1920;
	max_uae_height = 1080;

	// Get startup path
	getcwd(start_path_data, MAX_DPATH);
	loadAdfDir();
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
	rp9_cleanup();

	logging_cleanup();

	if(host_poweroff)
	  target_shutdown();
	return 0;
}

#ifdef USE_SDL1
SDLKey GetKeyFromName(const char *name)
{
	if (!name || !*name) {
		return SDLK_UNKNOWN;
	}

	for (int key = SDLK_FIRST; key < SDLK_LAST; key++)
	{
		if (!SDL_GetKeyName(SDLKey(key)))
			continue;
		if (SDL_strcasecmp(name, SDL_GetKeyName(SDLKey(key))) == 0)
		{
			return SDLKey(key);
		}
	}

	return SDLK_UNKNOWN;
}
#endif

int handle_msgpump()
{
	auto got = 0;
	SDL_Event rEvent;

	// Default Enter GUI key is F12
	int enter_gui_key = SDLK_F12;
	if (strncmp(currprefs.open_gui, "", 1) != 0)
	{
		// If we have a value in the config, we use that instead
		// SDL2-only for now
#ifdef USE_SDL1
		enter_gui_key = GetKeyFromName(currprefs.open_gui);
#elif USE_SDL2
		enter_gui_key = SDL_GetKeyFromName(currprefs.open_gui);
#endif
	}

	// We don't set a default value for Quitting
	int quit_key = 0;
	if (strncmp(currprefs.quit_amiberry, "", 1) != 0)
	{
		// If we have a value in the config, we use that instead
#ifdef USE_SDL1
		quit_key = GetKeyFromName(currprefs.quit_amiberry);
#elif USE_SDL2
		quit_key = SDL_GetKeyFromName(currprefs.quit_amiberry);
#endif
	}

	// The default value for Action Replay is Pause/Break
	int action_replay_button = SDLK_PAUSE;
	if (strncmp(currprefs.action_replay, "", 1) != 0)
	{
#ifdef USE_SDL1
		action_replay_button = GetKeyFromName(currprefs.action_replay);
#elif USE_SDL2
		action_replay_button = SDL_GetKeyFromName(currprefs.action_replay);
#endif
	}

	// No default value for Full Screen toggle
	int fullscreen_key = 0;
	if (strncmp(currprefs.fullscreen_toggle, "", 1) != 0)
	{
#ifdef USE_SDL1
		fullscreen_key = GetKeyFromName(currprefs.fullscreen_toggle);
#elif USE_SDL2
		fullscreen_key = SDL_GetKeyFromName(currprefs.fullscreen_toggle);
#endif
	}

	while (SDL_PollEvent(&rEvent))
	{
		got = 1;
#ifdef USE_SDL1
		Uint8* keystate = SDL_GetKeyState(nullptr);
#elif USE_SDL2
		const Uint8* keystate = SDL_GetKeyboardState(nullptr);
#endif
		switch (rEvent.type)
		{
		case SDL_QUIT:
			uae_quit();
			break;

		case SDL_KEYDOWN:
#ifdef USE_SDL2
			if (rEvent.key.repeat == 0)
			{
#endif
				// If the Enter GUI key was pressed, handle it
				if (enter_gui_key && rEvent.key.keysym.sym == enter_gui_key)
				{
					inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
					break;
				}

				// If the Quit emulator key was pressed, handle it
				if (quit_key && rEvent.key.keysym.sym == quit_key)
				{
					inputdevice_add_inputcode(AKS_QUIT, 1, nullptr);
					break;
				}

				if (action_replay_button && rEvent.key.keysym.sym == action_replay_button)
				{
					inputdevice_add_inputcode(AKS_FREEZEBUTTON, 1, nullptr);
					break;
				}

				if (fullscreen_key && rEvent.key.keysym.sym == fullscreen_key)
				{
					inputdevice_add_inputcode(AKS_TOGGLEWINDOWEDFULLSCREEN, 1, nullptr);
					break;
				}
#ifdef USE_SDL2
			}
#endif
			// If the reset combination was pressed, handle it
#ifdef USE_SDL1
			// Strangely in FBCON left window is seen as left alt ??
			if (keyboard_type == 2) // KEYCODE_FBCON
			{
				if (keystate[SDLK_LCTRL] && (keystate[SDLK_LSUPER] || keystate[SDLK_LALT]) && (keystate[SDLK_RSUPER] || keystate[SDLK_MENU]))
				{
					uae_reset(0, 1);
					break;
				}
			}
			else if (keystate[SDLK_LCTRL] && keystate[SDLK_LSUPER] && (keystate[SDLK_RSUPER] || keystate[SDLK_MENU]))
#elif USE_SDL2			
			if (keystate[SDL_SCANCODE_LCTRL] && keystate[SDL_SCANCODE_LGUI] && (keystate[SDL_SCANCODE_RGUI] || keystate[SDL_SCANCODE_APPLICATION]))
#endif
			{
				uae_reset(0, 1);
				break;
			}

#ifdef USE_SDL1
			// fix Caps Lock keypress shown as SDLK_UNKNOWN (scancode = 58)
			if (rEvent.key.keysym.scancode == 58 && rEvent.key.keysym.sym == SDLK_UNKNOWN)
				rEvent.key.keysym.sym = SDLK_CAPSLOCK;
#endif
#ifdef USE_SDL2
			if (rEvent.key.repeat == 0)
			{
#endif
				if (rEvent.key.keysym.sym == SDLK_CAPSLOCK)
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

				// Handle all other keys
#ifdef USE_SDL1
				if (keyboard_type == KEYCODE_UNK)
					inputdevice_translatekeycode(0, rEvent.key.keysym.sym, 1);
				else
					inputdevice_translatekeycode(0, rEvent.key.keysym.scancode, 1);
#elif USE_SDL2
				inputdevice_translatekeycode(0, rEvent.key.keysym.scancode, 1);
			}
#endif
			break;
		case SDL_KEYUP:
#ifdef USE_SDL2
			if (rEvent.key.repeat == 0)
			{
#endif
#ifdef USE_SDL1
				if (keyboard_type == KEYCODE_UNK)
					inputdevice_translatekeycode(0, rEvent.key.keysym.sym, 0);
				else
					inputdevice_translatekeycode(0, rEvent.key.keysym.scancode, 0);
#elif USE_SDL2
				inputdevice_translatekeycode(0, rEvent.key.keysym.scancode, 0);
			}
#endif
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
			{
				if (rEvent.button.button == SDL_BUTTON_LEFT)
					setmousebuttonstate(0, 0, 1);
				if (rEvent.button.button == SDL_BUTTON_RIGHT)
					setmousebuttonstate(0, 1, 1);
				if (rEvent.button.button == SDL_BUTTON_MIDDLE)
					setmousebuttonstate(0, 2, 1);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
			{
				if (rEvent.button.button == SDL_BUTTON_LEFT)
					setmousebuttonstate(0, 0, 0);
				if (rEvent.button.button == SDL_BUTTON_RIGHT)
					setmousebuttonstate(0, 1, 0);
				if (rEvent.button.button == SDL_BUTTON_MIDDLE)
					setmousebuttonstate(0, 2, 0);
			}
			break;

		case SDL_MOUSEMOTION:
			if (currprefs.input_tablet == TABLET_OFF)
			{
				if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
				{
					const auto mouseScale = currprefs.input_joymouse_multiplier / 2;
					auto x = rEvent.motion.xrel;
					auto y = rEvent.motion.yrel;
#if defined (ANDROIDSDL)
					if (rEvent.motion.x == 0 && x > -4)
						x = -4;
					if (rEvent.motion.y == 0 && y > -4)
						y = -4;
					if (rEvent.motion.x == currprefs.gfx_size.width - 1 && x < 4)
						x = 4;
					if (rEvent.motion.y == currprefs.gfx_size.height - 1 && y < 4)
						y = 4;
#endif //ANDROIDSDL
					setmousestate(0, 0, x * mouseScale, 0);
					setmousestate(0, 1, y * mouseScale, 0);
				}
			}
			break;

#ifdef USE_SDL2
		case SDL_MOUSEWHEEL:
			if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
			{
				const int valY = rEvent.wheel.y;
				const int valX = rEvent.wheel.x;
				setmousestate(0, 2, valY, 0);
				setmousestate(0, 3, valX, 0);
			}
			break;
#endif

		default:
			break;
		}
	}
	return got;
}

static uaecptr clipboard_data;

void amiga_clipboard_die()
{
}

void amiga_clipboard_init()
{
}

void amiga_clipboard_task_start(uaecptr data)
{
	clipboard_data = data;
}

uae_u32 amiga_clipboard_proc_start()
{
	return clipboard_data;
}

void amiga_clipboard_got_data(uaecptr data, uae_u32 size, uae_u32 actual)
{
}

int amiga_clipboard_want_data()
{
	return 0;
}

void clipboard_vsync()
{
}
