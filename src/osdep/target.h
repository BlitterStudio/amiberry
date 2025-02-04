/*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff, Amiberry version
  */

#pragma once
#include <SDL.h>

#include "options.h"

#define TARGET_NAME _T("amiberry")

#define NO_MAIN_IN_MAIN_C

#define OPTIONSFILENAME _T("default")

#define MAKEBD(x,y,z) ((((x) - 2000) * 10000 + (y)) * 100 + (z))
#define GETBDY(x) ((x) / 1000000 + 2000)
#define GETBDM(x) (((x) - (((x) / 10000) * 10000)) / 100)
#define GETBDD(x) ((x) % 100)

#define AMIBERRYDATE MAKEBD(2025, 1, 8)
#define COPYRIGHT _T("Copyright (C) 2016-2025 Dimitris Panokostas")

#define IHF_WINDOWHIDDEN 6

extern std::string get_version_string();
extern std::string get_copyright_notice();
extern std::string get_sdl2_version_string();

STATIC_INLINE FILE* uae_tfopen(const TCHAR* path, const TCHAR* mode)
{
	return fopen(path, mode);
}

// Expose prefix_with_application_directory from amiberry_filesys,
// so we can use it to open the main configuration file on OS X
extern int mouseactive;
extern int minimized;
extern int monitor_off;
extern bool joystick_refresh_needed;
extern std::string screenshot_filename;

extern void logging_init();

extern bool my_kbd_handler(int, int, int, bool);
extern void clearallkeys();
extern int getcapslock();

extern void releasecapture(const struct AmigaMonitor*);
extern void enablecapture(int monid);
extern void disablecapture();
extern void activationtoggle(int monid, bool inactiveonly);
extern bool create_screenshot();
extern int save_thumb(const std::string& path);

extern amiberry_hotkey enter_gui_key;
extern SDL_GameControllerButton enter_gui_button;
extern amiberry_hotkey quit_key;
extern amiberry_hotkey action_replay_key;
extern amiberry_hotkey fullscreen_key;
extern amiberry_hotkey minimize_key;

extern int emulating;
extern bool config_loaded;

extern int z3_base_adr;
extern bool volatile flip_in_progress;

extern void setmouseactive(int monid, int active);
extern void minimizewindow(int monid);
extern void updatemouseclip(struct AmigaMonitor*);
extern void updatewinrect(struct AmigaMonitor*, bool);
int getdpiforwindow(SDL_Window* hwnd);
void amiberry_gui_init();
void gui_widgets_init();
void run_gui();
void gui_widgets_halt();
void amiberry_gui_halt();
void init_max_signals();
void wait_for_vsync();
unsigned long target_lastsynctime();

void save_amiberry_settings();
void update_display(struct uae_prefs*);
void clearscreen();
void graphics_subshutdown();

extern void wait_keyrelease();
extern void keyboard_settrans();

extern void* open_tablet(SDL_Window* window);
extern int close_tablet(void*);
extern void send_tablet(int x, int y, int z, int pres, uae_u32 buttons, int flags, int ax, int ay, int az, int rx, int ry, int rz, SDL_Rect* r);
extern void send_tablet_proximity(int);

extern bool can_have_1gb();

#ifdef __MACH__
// from amifilesys.cpp
string prefix_with_application_directory_path(string currentpath);
#endif

extern void get_configuration_path(char* out, int size);
extern void set_configuration_path(const std::string& newpath);
extern void set_nvram_path(const std::string& newpath);
extern void set_plugins_path(const std::string& newpath);
extern void set_screenshot_path(const std::string& newpath);
extern void set_savestate_path(const std::string& newpath);
extern void set_themes_path(const std::string& newpath);
extern std::string get_controllers_path();
extern void set_controllers_path(const std::string& newpath);

extern std::string get_retroarch_file();
extern void set_retroarch_file(const std::string& newpath);

extern std::string get_savedatapath(bool force_internal);
extern std::string get_whdbootpath();
extern void set_whdbootpath(const std::string& newpath);
extern std::string get_whdload_arch_path();
extern void set_whdload_arch_path(const std::string& newpath);
extern std::string get_floppy_path();
extern void set_floppy_path(const std::string& newpath);
extern std::string get_harddrive_path();
extern void set_harddrive_path(const std::string& newpath);
extern std::string get_cdrom_path();
extern void set_cdrom_path(const std::string& newpath);
extern std::string get_themes_path();

extern bool get_logfile_enabled();
extern void set_logfile_enabled(bool enabled);
extern std::string get_logfile_path();
extern void set_logfile_path(const std::string& newpath);

extern void set_rom_path(const std::string& newpath);
extern void get_rp9_path(char* out, int size);
extern std::string get_screenshot_path();

extern void extract_filename(const char* str, char* buffer);
extern std::string extract_filename(const std::string& path);
extern void extract_path(const char* str, char* buffer);
extern std::string extract_path(const std::string& filename);
extern void remove_file_extension(char* filename);
extern std::string remove_file_extension(const std::string& filename);
extern void ReadConfigFileList();
extern void read_rom_list(bool);
extern int scan_roms(int show);

extern bool resumepaused(int priority);
extern bool setpaused(int priority);
extern void unsetminimized(int monid);
extern void setminimized(int monid);

extern void setpriority(int prio);
void init_colors(int monid);

#include <vector>
#include <string>

#define MAX_MRU_LIST 40

extern std::vector<std::string> lstMRUDiskList;
extern std::vector<std::string> lstMRUCDList;
extern std::vector<std::string> lstMRUWhdloadList;

extern void add_file_to_mru_list(std::vector<std::string>& vec, const std::string& file);

int count_HDs(const struct uae_prefs* p);
extern void gui_force_rtarea_hdchange();
extern int isfocus();
extern void gui_restart();
extern bool hardfile_testrdb(const char* filename);

extern bool host_poweroff;

#ifdef __cplusplus
extern "C" {
#endif
void trace_begin(void);
void trace_end(void);
#ifdef __cplusplus
}
#endif

STATIC_INLINE int max(int x, int y)
{
	return x > y ? x : y;
}

#ifdef USE_JIT_FPU
#ifdef __cplusplus
extern "C" {
#endif
void save_host_fp_regs(void* buf);
void restore_host_fp_regs(void* buf);
#ifdef __cplusplus
}
#endif
#endif

#ifndef _WIN32
// Dummy types so this header file can be included on other platforms (for
// a few declarations).
typedef void* HINSTANCE;
typedef void* HMODULE;
typedef SDL_Window* HWND;
typedef void* HKEY;
typedef void* OSVERSIONINFO;
typedef bool BOOL;
typedef int LPARAM;
typedef int WPARAM;
typedef int WORD;
typedef unsigned int UINT;
typedef long LONG;
#define WINAPI
typedef long GUID;
typedef wchar_t* LPCWSTR;
#endif

#define MAX_SOUND_DEVICES 100
#define SOUND_DEVICE_DS 1
#define SOUND_DEVICE_AL 2
#define SOUND_DEVICE_PA 3
#define SOUND_DEVICE_WASAPI 4
#define SOUND_DEVICE_WASAPI_EXCLUSIVE 5
#define SOUND_DEVICE_XAUDIO2 6
#define SOUND_DEVICE_SDL2 7

struct sound_device
{
	GUID guid;
#ifdef AMIBERRY
	int id;
#endif
	TCHAR* name;
	TCHAR* alname;
	TCHAR* cfgname;
	TCHAR* prefix;
	int panum;
	int type;
};
extern struct sound_device* sound_devices[MAX_SOUND_DEVICES];
extern struct sound_device* record_devices[MAX_SOUND_DEVICES];
