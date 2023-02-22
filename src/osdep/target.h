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

#if !defined(ARMV6T2) && !defined(CPU_AARCH64)
#undef USE_JIT_FPU
#endif

#define MAKEBD(x,y,z) ((((x) - 2000) * 10000 + (y)) * 100 + (z))
#define GETBDY(x) ((x) / 1000000 + 2000)
#define GETBDM(x) (((x) - (((x) / 10000) * 10000)) / 100)
#define GETBDD(x) ((x) % 100)

#define AMIBERRYVERSION _T("Amiberry v5.7 DEV (2023-02-19)")
#define AMIBERRYDATE MAKEBD(2023, 2, 19)

#define IHF_WINDOWHIDDEN 6

extern std::string get_version_string();
extern std::string get_sdl2_version_string();

STATIC_INLINE FILE* uae_tfopen(const TCHAR* path, const TCHAR* mode)
{
	return fopen(path, mode);
}

// Expose prefix_with_application_directory from amiberry_filesys so we can use it to open the main confguration file on OS X
extern int mouseactive;
extern int minimized;
extern int monitor_off;
extern bool joystick_refresh_needed;

extern void logging_init();

extern bool my_kbd_handler(int, int, int, bool);
extern void clearallkeys();
extern int getcapslock();

void releasecapture(struct AmigaMonitor*);
extern void disablecapture();

extern amiberry_hotkey enter_gui_key;
extern SDL_GameControllerButton enter_gui_button;
extern amiberry_hotkey quit_key;
extern amiberry_hotkey action_replay_key;
extern amiberry_hotkey fullscreen_key;
extern amiberry_hotkey minimize_key;

extern int emulating;
extern bool config_loaded;

extern int z3_base_adr;
#ifdef USE_DISPMANX
extern unsigned long time_per_frame;
#endif
extern bool volatile flip_in_progress;

void amiberry_gui_init();
void gui_widgets_init();
void run_gui(void);
void gui_widgets_halt();
void amiberry_gui_halt();
void init_max_signals(void);
void wait_for_vsync(void);
unsigned long target_lastsynctime(void);

void save_amiberry_settings(void);
void update_display(struct uae_prefs*);
void clearscreen(void);
void graphics_subshutdown(void);

extern void wait_keyrelease(void);
extern void keyboard_settrans(void);

extern void free_AmigaMem();
extern void alloc_AmigaMem();
extern bool can_have_1gb();

#ifdef __MACH__
// from amifilesys.cpp
string prefix_with_application_directory_path(string currentpath);
#endif

extern void get_configuration_path(char* out, int size);
extern void set_configuration_path(char* newpath);
extern void set_nvram_path(char* newpath);
extern void set_screenshot_path(char* newpath);
extern void set_savestate_path(char* newpath);
extern void get_controllers_path(char* out, int size);
extern void set_controllers_path(char* newpath);

extern void get_retroarch_file(char* out, int size);
extern void set_retroarch_file(char* newpath);

extern int get_savedatapath(char* out, int size, const int force_internal);
extern void get_whdbootpath(char* out, int size);
extern void set_whdbootpath(char* newpath);

extern bool get_logfile_enabled();
extern void set_logfile_enabled(bool enabled);
extern void get_logfile_path(char* out, int size);
extern void set_logfile_path(char* newpath);

extern void set_rom_path(char* newpath);
extern void get_rp9_path(char* out, int size);
extern void get_savestate_path(char* out, int size);
extern void get_screenshot_path(char* out, int size);

extern void extract_filename(const char* str, char* buffer);
extern void extract_path(char* str, char* buffer);
extern void remove_file_extension(char* filename);
extern void ReadConfigFileList(void);
extern void RescanROMs(void);
extern void SymlinkROMs(void);
extern void ClearAvailableROMList(void);

extern void minimizewindow(int monid);
extern void updatewinrect(struct AmigaMonitor*, bool);

extern bool resumepaused(int priority);
extern bool setpaused(int priority);
extern void unsetminimized(int monid);
extern void setminimized(int monid);

extern void setpriority(int prio);
void init_colors(int monid);

#include <vector>
#include <string>

typedef struct
{
	char Name[MAX_DPATH];
	char Path[MAX_DPATH];
	int ROMType;
} AvailableROM;

extern std::vector<AvailableROM*> lstAvailableROMs;

#define MAX_MRU_DISKLIST 40
extern std::vector<std::string> lstMRUDiskList;
extern void AddFileToDiskList(const char* file, int moveToTop);

#define MAX_MRU_CDLIST 10
extern std::vector<std::string> lstMRUCDList;
extern void AddFileToCDList(const char* file, int moveToTop);

#define MAX_MRU_WHDLOADLIST 10
extern std::vector<std::string> lstMRUWhdloadList;
extern void AddFileToWHDLoadList(const char* file, int moveToTop);

int count_HDs(struct uae_prefs* p);
extern void gui_force_rtarea_hdchange(void);
extern int isfocus(void);
extern void gui_restart(void);
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
typedef void* HWND;
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

static inline int uae_deterministic_mode()
{
	// Only returns 1 if using netplay mode (not implemented yet)
	return 0;
}
