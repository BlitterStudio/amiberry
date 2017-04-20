/*
 * UAE - The Un*x Amiga Emulator
 *
 * Amiberry interface
 *
 */

#include <algorithm>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <asm/sigcontext.h>
#include <signal.h>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "autoconf.h"
#include "uae.h"
#include "options.h"
#include "threaddep/thread.h"
#include "gui.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "keyboard.h"
#include "disk.h"
#include "savestate.h"
#include "traps.h"
#include "bsdsocket.h"
#include "blkdev.h"
#include "native2amiga.h"
#include "rtgmodes.h"
#include "uaeresource.h"
#include "rommgr.h"
#include "akiko.h"
#include "SDL.h"
#include "amiberry_rp9.h"
#include "scsidev.h"

extern void signal_segv(int signum, siginfo_t* info, void* ptr);
extern void gui_force_rtarea_hdchange();

extern int loadconfig_old(struct uae_prefs* p, const char* orgpath);
extern void SetLastActiveConfig(const char* filename);

int pause_emulation;

/* Keyboard */
map<int, int> customControlMap; // No SDLK_LAST. SDL2 migration guide suggests std::map

char start_path_data[MAX_DPATH];
char currentDir[MAX_DPATH];

#include <linux/kd.h>
#include <sys/ioctl.h>
unsigned char kbd_led_status;
char kbd_flags;

static char config_path[MAX_DPATH];
static char rom_path[MAX_DPATH];
static char rp9_path[MAX_DPATH];
char last_loaded_config[MAX_DPATH] = {'\0'};

int max_uae_width;
int max_uae_height;

extern "C" int main(int argc, char* argv[]);

void reinit_amiga()
{
	write_log("reinit_amiga() called\n");
	DISK_free();
#ifdef CD32
	akiko_free();
#endif
#ifdef FILESYS
	filesys_cleanup();
	hardfile_reset();
#endif
#ifdef AUTOCONFIG
#if defined (BSDSOCKET)
	bsdlib_reset();
#endif
	expansion_cleanup();
#endif
	device_func_reset();
	memory_cleanup();

	currprefs = changed_prefs;
	/* force sound settings change */
	currprefs.produce_sound = 0;

	framecnt = 1;
#ifdef AUTOCONFIG
	rtarea_setup();
#endif
#ifdef FILESYS
	rtarea_init();
	uaeres_install();
	hardfile_install();
#endif
	keybuf_init();

#ifdef AUTOCONFIG
	expansion_init();
#endif
#ifdef FILESYS
	filesys_install();
#endif
	memory_init();
	memory_reset();

#ifdef AUTOCONFIG
#if defined (BSDSOCKET)
	bsdlib_install();
#endif
	emulib_install();
	native2amiga_install();
#endif

	custom_init(); /* Must come after memory_init */
	DISK_init();

	reset_frame_rate_hack();
	init_m68k();
}

void sleep_millis_main(int ms)
{
	usleep(ms * 1000);
}

void sleep_millis(int ms)
{
	usleep(ms * 1000);
}

void logging_init()
{
#ifdef WITH_LOGGING
    static int started;
    static int first;
    char debugfilename[MAX_DPATH];

    if (first > 1)
    {
        write_log ("***** RESTART *****\n");
        return;
    }
    if (first == 1)
    {
        if (debugfile)
            fclose (debugfile);
        debugfile = 0;
    }

    sprintf(debugfilename, "%s/amiberry_log.txt", start_path_data);
    if(!debugfile)
        debugfile = fopen(debugfilename, "wt");

    first++;
    write_log ( "Amiberry Logfile\n\n");
#endif
}

void logging_cleanup()
{
#ifdef WITH_LOGGING
    if(debugfile)
        fclose(debugfile);
    debugfile = 0;
#endif
}


void stripslashes(TCHAR* p)
{
	while (_tcslen (p) > 0 && (p[_tcslen (p) - 1] == '\\' || p[_tcslen (p) - 1] == '/'))
		p[_tcslen (p) - 1] = 0;
}

void fixtrailing(TCHAR* p)
{
	if (_tcslen(p) == 0)
		return;
	if (p[_tcslen(p) - 1] == '/' || p[_tcslen(p) - 1] == '\\')
		return;
	_tcscat(p, "/");
}

void getpathpart(TCHAR* outpath, int size, const TCHAR* inpath)
{
	_tcscpy (outpath, inpath);
	TCHAR* p = _tcsrchr (outpath, '/');
	if (p)
		p[0] = 0;
	fixtrailing(outpath);
}

void getfilepart(TCHAR* out, int size, const TCHAR* path)
{
	out[0] = 0;
	const TCHAR* p = _tcsrchr (path, '/');
	if (p)
	_tcscpy (out, p + 1);
	else
	_tcscpy (out, path);
}

uae_u8* target_load_keyfile(struct uae_prefs* p, const char* path, int* sizep, char* name)
{
	return nullptr;
}

void target_run()
{
}

void target_quit()
{
}

void target_fixup_options(struct uae_prefs* p)
{
	p->rtgmem_type = 1;
	if (p->z3fastmem_start != z3_start_adr)
		p->z3fastmem_start = z3_start_adr;

	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5 | RGBFF_R8G8B8A8;
	if (p->gfx_size.width == 0)
		p->gfx_size.width = 640;
	if (p->gfx_size.height == 0)
		p->gfx_size.height == 256;
	p->gfx_resolution = p->gfx_size.width > 600 ? 1 : 0;
}

void target_default_options(struct uae_prefs* p, int type)
{
	p->customControls = false;
	p->custom_up = 0;
	p->custom_down = 0;
	p->custom_left = 0;
	p->custom_right = 0;
	p->custom_a = 0;
	p->custom_b = 0;
	p->custom_x = 0;
	p->custom_y = 0;
	p->custom_l = 0;
	p->custom_r = 0;
	p->custom_play = 0;

	p->picasso96_modeflags = RGBFF_CLUT | RGBFF_R5G6B5 | RGBFF_R8G8B8A8;

	p->kbd_led_num = -1; // No status on numlock
	p->kbd_led_scr = -1; // No status on scrollock
	p->scaling_method = -1; //Default is Auto
	p->key_for_menu = SDLK_F12;
	p->key_for_quit = 0;
	p->button_for_menu = -1;
	p->button_for_quit = -1;
}

void target_save_options(struct zfile* f, struct uae_prefs* p)
{
	cfgfile_write(f, _T("gfx_correct_aspect"), _T("%d"), p->gfx_correct_aspect);
	cfgfile_write(f, _T("kbd_led_num"), _T("%d"), p->kbd_led_num);
	cfgfile_write(f, _T("kbd_led_scr"), _T("%d"), p->kbd_led_scr);
	cfgfile_write(f, _T("scaling_method"), _T("%d"), p->scaling_method);
	cfgfile_write(f, _T("key_for_menu"), _T("%d"), p->key_for_menu);
	cfgfile_write(f, _T("key_for_quit"), _T("%d"), p->key_for_quit);
	cfgfile_write(f, _T("button_for_menu"), _T("%d"), p->button_for_menu);
	cfgfile_write(f, _T("button_for_quit"), _T("%d"), p->button_for_quit);

	cfgfile_write_bool(f, "amiberry.custom_controls", p->customControls);
	cfgfile_write(f, "amiberry.custom_up", "%d", p->custom_up);
	cfgfile_write(f, "amiberry.custom_down", "%d", p->custom_down);
	cfgfile_write(f, "amiberry.custom_left", "%d", p->custom_left);
	cfgfile_write(f, "amiberry.custom_right", "%d", p->custom_right);
	cfgfile_write(f, "amiberry.custom_a", "%d", p->custom_a);
	cfgfile_write(f, "amiberry.custom_b", "%d", p->custom_b);
	cfgfile_write(f, "amiberry.custom_x", "%d", p->custom_x);
	cfgfile_write(f, "amiberry.custom_y", "%d", p->custom_y);
	cfgfile_write(f, "amiberry.custom_l", "%d", p->custom_l);
	cfgfile_write(f, "amiberry.custom_r", "%d", p->custom_r);
	cfgfile_write(f, "amiberry.custom_play", "%d", p->custom_play);
}

void target_restart()
{
}

TCHAR* target_expand_environment(const TCHAR* path)
{
	if (!path)
		return nullptr;
	return strdup(path);
}

int target_parse_option(struct uae_prefs* p, const char* option, const char* value)
{

	if (cfgfile_intval(option, value, "gfx_correct_aspect", &p->gfx_correct_aspect, 1))
		return 1;
	if (cfgfile_intval(option, value, "kbd_led_num", &p->kbd_led_num, 1))
		return 1;
	if (cfgfile_intval(option, value, "kbd_led_scr", &p->kbd_led_scr, 1))
		return 1;
	if (cfgfile_intval(option, value, "scaling_method", &p->scaling_method, 1))
		return 1;
	if (cfgfile_intval(option, value, "key_for_menu", &p->key_for_menu, 1))
		return 1;
	if (cfgfile_intval(option, value, "key_for_quit", &p->key_for_quit, 1))
		return 1;
	if (cfgfile_intval(option, value, "button_for_menu", &p->button_for_menu, 1))
		return 1;
	if (cfgfile_intval(option, value, "button_for_quit", &p->button_for_quit, 1))
		return 1;

	int result = cfgfile_yesno(option, value, "amiberry.custom_controls", &p->customControls)
		|| cfgfile_intval(option, value, "amiberry.custom_up", &p->custom_up, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_down", &p->custom_down, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_left", &p->custom_left, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_right", &p->custom_right, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_a", &p->custom_a, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_b", &p->custom_b, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_x", &p->custom_x, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_y", &p->custom_y, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_l", &p->custom_l, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_r", &p->custom_r, 1)
		|| cfgfile_intval(option, value, "amiberry.custom_play", &p->custom_play, 1);
	return result;
}

void fetch_datapath(char* out, int size)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/", size);
}

void fetch_saveimagepath(char* out, int size, int dir)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/savestates/", size);
}

void fetch_configurationpath(char* out, int size)
{
	strncpy(out, config_path, size);
}

void set_configurationpath(char* newpath)
{
	strncpy(config_path, newpath, MAX_DPATH);
}

void fetch_rompath(char* out, int size)
{
	strncpy(out, rom_path, size);
}

void set_rompath(char* newpath)
{
	strncpy(rom_path, newpath, MAX_DPATH);
}

void fetch_rp9path(char* out, int size)
{
	strncpy(out, rp9_path, size);
}

void fetch_statefilepath(char* out, int size)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/savestates/", size);
}

void fetch_screenshotpath(char* out, int size)
{
	strncpy(out, start_path_data, size);
	strncat(out, "/screenshots/", size);
}

int target_cfgfile_load(struct uae_prefs* p, const char* filename, int type, int isdefault)
{
	int i;
	int result = 0;

	if (emulating && changed_prefs.cdslots[0].inuse)
		gui_force_rtarea_hdchange();

	discard_prefs(p, type);
	default_prefs(p, 0);

	const char* ptr = strstr(filename, ".rp9");
	if (ptr > nullptr)
	{
		// Load rp9 config
		result = rp9_parse_file(p, filename);
		if (result)
			extractFileName(filename, last_loaded_config);
	}
	else
	{
		ptr = strstr(filename, ".uae");
		if (ptr > nullptr)
		{
			int type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
			result = cfgfile_load(p, filename, &type, 0, 1);
		}
		if (result)
			extractFileName(filename, last_loaded_config);
		else
			result = loadconfig_old(p, filename);
	}

	if (result)
	{
		for (i = 0; i < p->nr_floppies; ++i)
		{
			if (!DISK_validate_filename(p, p->floppyslots[i].df, 0, nullptr, nullptr, nullptr))
				p->floppyslots[i].df[0] = 0;
			disk_insert(i, p->floppyslots[i].df);
			if (strlen(p->floppyslots[i].df) > 0)
				AddFileToDiskList(p->floppyslots[i].df, 1);
		}

		if (!isdefault)
			inputdevice_updateconfig(nullptr, p);

		SetLastActiveConfig(filename);

		if (count_HDs(p) > 0) // When loading a config with HDs, always do a hardreset
			gui_force_rtarea_hdchange();
	}

	return result;
}

int check_configfile(char* file)
{
	char tmp[MAX_PATH];

	FILE* f = fopen(file, "rt");
	if (f)
	{
		fclose(f);
		return 1;
	}

	strcpy(tmp, file);
	char* ptr = strstr(tmp, ".uae");
	if (ptr > nullptr)
	{
		*(ptr + 1) = '\0';
		strcat(tmp, "conf");
		f = fopen(tmp, "rt");
		if (f)
		{
			fclose(f);
			return 2;
		}
	}

	return 0;
}

void extractFileName(const char* str, char* buffer)
{
	const char* p = str + strlen(str) - 1;
	while (*p != '/' && p > str)
		p--;
	p++;
	strcpy(buffer, p);
}

void extractPath(char* str, char* buffer)
{
	strcpy(buffer, str);
	char* p = buffer + strlen(buffer) - 1;
	while (*p != '/' && p > buffer)
		p--;
	p[1] = '\0';
}

void removeFileExtension(char* filename)
{
	char* p = filename + strlen(filename) - 1;
	while (p > filename && *p != '.')
	{
		*p = '\0';
		--p;
	}
	*p = '\0';
}

void ReadDirectory(const char* path, vector<string>* dirs, vector<string>* files)
{
	DIR* dir;
	struct dirent* dent;

	if (dirs != nullptr)
		dirs->clear();
	if (files != nullptr)
		files->clear();

	if ((dir = opendir(path)) != nullptr)
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
		if (dirs != nullptr && dirs->size() > 0 && (*dirs)[0] == ".")
			dirs->erase(dirs->begin());
		closedir(dir);
	}

	if (dirs != nullptr)
		sort(dirs->begin(), dirs->end());
	if (files != nullptr)
		sort(files->begin(), files->end());
}

void saveAdfDir()
{
	char path[MAX_DPATH];
	int i;

	snprintf(path, MAX_DPATH, "%s/conf/adfdir.conf", start_path_data);
	FILE* f = fopen(path, "w");
	if (!f)
		return;

	char buffer[MAX_DPATH];
	snprintf(buffer, MAX_DPATH, "path=%s\n", currentDir);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "config_path=%s\n", config_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "rom_path=%s\n", rom_path);
	fputs(buffer, f);

	snprintf(buffer, MAX_DPATH, "ROMs=%d\n", lstAvailableROMs.size());
	fputs(buffer, f);
	for (i = 0; i < lstAvailableROMs.size(); ++i)
	{
		snprintf(buffer, MAX_DPATH, "ROMName=%s\n", lstAvailableROMs[i]->Name);
		fputs(buffer, f);
		snprintf(buffer, MAX_DPATH, "ROMPath=%s\n", lstAvailableROMs[i]->Path);
		fputs(buffer, f);
		snprintf(buffer, MAX_DPATH, "ROMType=%d\n", lstAvailableROMs[i]->ROMType);
		fputs(buffer, f);
	}

	snprintf(buffer, MAX_DPATH, "MRUDiskList=%d\n", lstMRUDiskList.size());
	fputs(buffer, f);
	for (i = 0; i < lstMRUDiskList.size(); ++i)
	{
		snprintf(buffer, MAX_DPATH, "Diskfile=%s\n", lstMRUDiskList[i].c_str());
		fputs(buffer, f);
	}

	snprintf(buffer, MAX_DPATH, "MRUCDList=%d\n", lstMRUCDList.size());
	fputs(buffer, f);
	for (i = 0; i < lstMRUCDList.size(); ++i)
	{
		snprintf(buffer, MAX_DPATH, "CDfile=%s\n", lstMRUCDList[i].c_str());
		fputs(buffer, f);
	}

	fclose(f);
}

void get_string(FILE* f, char* dst, int size)
{
	char buffer[MAX_PATH];
	fgets(buffer, MAX_PATH, f);
	int i = strlen(buffer);
	while (i > 0 && (buffer[i - 1] == '\t' || buffer[i - 1] == ' '
		|| buffer[i - 1] == '\r' || buffer[i - 1] == '\n'))
		buffer[--i] = '\0';
	strncpy(dst, buffer, size);
}

void loadAdfDir()
{
	char path[MAX_DPATH];
	int i;

	strcpy(currentDir, start_path_data);
	snprintf(config_path, MAX_DPATH, "%s/conf/", start_path_data);
	snprintf(rom_path, MAX_DPATH, "%s/kickstarts/", start_path_data);
	snprintf(rp9_path, MAX_DPATH, "%s/rp9/", start_path_data);

	snprintf(path, MAX_DPATH, "%s/conf/adfdir.conf", start_path_data);
	FILE* f1 = fopen(path, "rt");
	if (f1)
	{
		fscanf(f1, "path=");
		get_string(f1, currentDir, sizeof(currentDir));
		if (!feof(f1))
		{
			fscanf(f1, "config_path=");
			get_string(f1, config_path, sizeof(config_path));
			fscanf(f1, "rom_path=");
			get_string(f1, rom_path, sizeof(rom_path));

			int numROMs;
			fscanf(f1, "ROMs=%d\n", &numROMs);
			for (i = 0; i < numROMs; ++i)
			{
				AvailableROM* tmp;
				tmp = new AvailableROM();
				fscanf(f1, "ROMName=");
				get_string(f1, tmp->Name, sizeof(tmp->Name));
				fscanf(f1, "ROMPath=");
				get_string(f1, tmp->Path, sizeof(tmp->Path));
				fscanf(f1, "ROMType=%d\n", &(tmp->ROMType));
				lstAvailableROMs.push_back(tmp);
			}

			lstMRUDiskList.clear();
			int numDisks = 0;
			char disk[MAX_PATH];
			fscanf(f1, "MRUDiskList=%d\n", &numDisks);
			for (i = 0; i < numDisks; ++i)
			{
				fscanf(f1, "Diskfile=");
				get_string(f1, disk, sizeof(disk));
				FILE * f = fopen(disk, "rb");
				if (f != NULL)
				{
					fclose(f);
					lstMRUDiskList.push_back(disk);
				}
			}

			lstMRUCDList.clear();
			int numCD = 0;
			char cd[MAX_PATH];
			fscanf(f1, "MRUCDList=%d\n", &numCD);
			for (i = 0; i < numCD; ++i)
			{
				fscanf(f1, "CDfile=");
				get_string(f1, cd, sizeof(cd));
				FILE * f = fopen(cd, "rb");
				if (f != NULL)
				{
					fclose(f);
					lstMRUCDList.push_back(cd);
				}
			}
		}
		fclose(f1);
	}
}

void target_reset()
{
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
		int64_t time;
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		time = (int64_t(ts.tv_sec) * 1000000000) + ts.tv_nsec;
		*low = uae_u32(time & 0xffffffff);
		return uae_u32(time >> 32);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	struct sigaction action;
	max_uae_width = 1920;
	max_uae_height = 1080;

	// Get startup path
	getcwd(start_path_data, MAX_DPATH);
	loadAdfDir();
	rp9_init();

	snprintf(savestate_fname, MAX_PATH, "%s/saves/default.ads", start_path_data);
	logging_init();

	memset(&action, 0, sizeof(action));
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

	alloc_AmigaMem();
	RescanROMs();
	keyboard_settrans();

#ifdef CAPSLOCK_DEBIAN_WORKAROUND
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
#endif

	real_main(argc, argv);

#ifdef CAPSLOCK_DEBIAN_WORKAROUND
	// restore keyboard LEDs to normal state
	ioctl(0, KDSETLED, 0xFF);
#endif

	ClearAvailableROMList();
	romlist_clear();
	free_keyring();
	free_AmigaMem();
	lstMRUDiskList.clear();
	lstMRUCDList.clear();
	rp9_cleanup();

	logging_cleanup();

	return 0;
}

void PopulateCustomControlMap()
{
	customControlMap[VK_UP] = currprefs.custom_up;
	customControlMap[VK_DOWN] = currprefs.custom_down;
	customControlMap[VK_LEFT] = currprefs.custom_left;
	customControlMap[VK_RIGHT] = currprefs.custom_right;
	customControlMap[VK_Green] = currprefs.custom_a;
	customControlMap[VK_Blue] = currprefs.custom_b;
	customControlMap[VK_Red] = currprefs.custom_x;
	customControlMap[VK_Yellow] = currprefs.custom_y;
	customControlMap[VK_LShoulder] = currprefs.custom_l;
	customControlMap[VK_RShoulder] = currprefs.custom_r;
	customControlMap[VK_Play] = currprefs.custom_play;
}

int handle_msgpump()
{
	int got = 0;
	SDL_Event rEvent;
	int keycode;

	if (currprefs.customControls)
		PopulateCustomControlMap();

	while (SDL_PollEvent(&rEvent))
	{
		got = 1;
		const Uint8* keystate = SDL_GetKeyboardState(nullptr);

		switch (rEvent.type)
		{
		case SDL_QUIT:
			uae_quit();
			break;

		case SDL_JOYBUTTONDOWN:
			if (currprefs.button_for_menu != -1 && rEvent.jbutton.button == currprefs.button_for_menu)
				inputdevice_add_inputcode(AKS_ENTERGUI, 1);
			if (currprefs.button_for_quit != -1 && rEvent.jbutton.button == currprefs.button_for_quit)
				inputdevice_add_inputcode(AKS_QUIT, 1);
			break;

		case SDL_KEYDOWN:
		  printf("Key pressed %d %d\n", rEvent.key.keysym.scancode, rEvent.key.keysym.scancode_new);
			if (keystate[SDL_SCANCODE_LCTRL] && keystate[SDL_SCANCODE_LGUI] && (keystate[SDL_SCANCODE_RGUI] || keystate[SDL_SCANCODE_APPLICATION]))
			{
				uae_reset(0, 1);
				break;
			}

			switch (rEvent.key.keysym.sym)
			{
			case SDLK_NUMLOCKCLEAR:
				if (currprefs.keyboard_leds[KBLED_NUMLOCKB] > 0)
				{
					//oldleds ^= KBLED_NUMLOCKM;
					//ch = true;
				}
				break;
			case SDLK_CAPSLOCK: // capslock
				if (currprefs.keyboard_leds[KBLED_CAPSLOCKB] > 0)
				{
					//oldleds ^= KBLED_CAPSLOCKM;
					//ch = true;
				}
				// Treat CAPSLOCK as a toggle. If on, set off and vice/versa
				ioctl(0, KDGKBLED, &kbd_flags);
				ioctl(0, KDGETLED, &kbd_led_status);
				if ((kbd_flags & 07) & LED_CAP)
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

			case SDLK_SCROLLLOCK:
				if (currprefs.keyboard_leds[KBLED_SCROLLLOCKB] > 0)
				{
					//oldleds ^= KBLED_SCROLLLOCKM;
					//ch = true;
				}
				break;

			default:
				if (currprefs.customControls)
				{
					keycode = customControlMap[rEvent.key.keysym.sym];
					if (keycode < 0)
					{
						// Simulate mouse or joystick
						SimulateMouseOrJoy(keycode, 1);
						break;
					}
					if (keycode > 0)
					{
						// Send mapped key press
						inputdevice_do_keyboard(keycode, 1);
						break;
					}
				}
				translate_amiberry_keys(rEvent.key.keysym.sym, 1);
				break;
			}
			break;

		case SDL_KEYUP:
				if (currprefs.customControls)
				{
					keycode = customControlMap[rEvent.key.keysym.sym];
					if (keycode < 0)
					{
						// Simulate mouse or joystick
						SimulateMouseOrJoy(keycode, 0);
						break;
					}
					if (keycode > 0)
					{
						// Send mapped key release
						inputdevice_do_keyboard(keycode, 0);
						break;
					}
				}

				translate_amiberry_keys(rEvent.key.keysym.sym, 0);
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
					int x, y;
					int mouseScale = currprefs.input_joymouse_multiplier / 2;
					x = rEvent.motion.xrel;
					y = rEvent.motion.yrel;

					setmousestate(0, 0, x * mouseScale, 0);
					setmousestate(0, 1, y * mouseScale, 0);
				}
			}
			break;

		case SDL_MOUSEWHEEL:
			if (currprefs.jports[0].id == JSEM_MICE || currprefs.jports[1].id == JSEM_MICE)
			{
				int valY = rEvent.wheel.y;
				int valX = rEvent.wheel.x;
				setmousestate(0, 2, valY, 0);
				setmousestate(0, 3, valX, 0);
			}
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
