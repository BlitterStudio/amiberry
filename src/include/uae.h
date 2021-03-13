 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Prototypes for main.c
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef UAE_UAE_H
#define UAE_UAE_H

#include "uae/types.h"

extern void real_main (int, TCHAR **);
extern void usage (void);
extern void sleep_micros (int ms);
extern void sleep_millis (int ms);
extern int sleep_millis_main(int ms);
extern int sleep_millis_amiga(int ms);
extern void sleep_cpu_wakeup(void);
extern int sleep_resolution;

#define UAE_QUIT 1
#define UAE_RESET 2
#define UAE_RESET_KEYBOARD 3
#define UAE_RESET_HARD 4

extern void uae_reset (int, int);
extern void uae_quit (void);
extern void target_shutdown(void);
extern void uae_restart (int, const TCHAR*);
extern void target_execute(const char* command);
extern void target_reset (void);
extern void target_addtorecent (const TCHAR*, int);
extern void target_run (void);
extern void target_quit (void);
extern void target_restart (void);
extern void target_getdate(int *y, int *m, int *d);
extern void target_startup_msg(const TCHAR *title, const TCHAR *msg);
extern void target_cpu_speed(void);
extern int target_sleep_nanos(int);
extern bool get_plugin_path (TCHAR *out, int size, const TCHAR *path);
extern void strip_slashes (TCHAR *p);
extern void fix_trailing (TCHAR *p);
extern void fullpath(TCHAR *path, int size);
extern void fullpath(TCHAR *path, int size, bool userelative);
extern void get_path_part (TCHAR *outpath, int size, const TCHAR *inpath);
extern void get_file_part (TCHAR *out, int size, const TCHAR *path);
extern bool same_path(const TCHAR *p1, const TCHAR *p2);
extern bool target_isrelativemode(void);
extern uae_u32 getlocaltime (void);
extern bool isguiactive(void);
extern bool is_mainthread(void);
extern void replace(std::string& str, const std::string& from, const std::string& to);

extern int quit_program;
extern bool console_emulation;

extern TCHAR warning_buffer[256];
extern TCHAR start_path_data[];
extern TCHAR start_path_data_exe[];
extern TCHAR start_path_plugins[];

/* This structure is used to define menus. The val field can hold key
 * shortcuts, or one of these special codes:
 *   -4: deleted entry, not displayed, not selectable, but does count in
 *       select value
 *   -3: end of table
 *   -2: line that is displayed, but not selectable
 *   -1: line that is selectable, but has no keyboard shortcut
 *    0: Menu title
 */
struct bstring {
    const TCHAR *data;
    int val;
};

extern TCHAR *colormodes[];
extern int saveimageoriginalpath;
extern void get_saveimage_path (TCHAR*, int, int);
extern void get_configuration_path (TCHAR *out, int size);
extern void fetch_luapath (TCHAR *out, int size);
extern void get_screenshot_path (TCHAR *out, int size);
extern void fetch_ripperpath (TCHAR *out, int size);
extern void get_savestate_path(TCHAR *out, int size);
extern void fetch_inputfilepath (TCHAR *out, int size);
extern void get_data_path (TCHAR *out, int size);
extern void get_rom_path (TCHAR *out, int size);
//extern uae_u32 uaerand (void);
STATIC_INLINE uae_u32 uaerand(void)
{
  return rand();
}
extern uae_u32 uaesrand (uae_u32 seed);
extern uae_u32 uaerandgetseed (void);

/* the following prototypes should probably be moved somewhere else */

int get_guid_target (uae_u8 *out);
void filesys_addexternals (void);

#endif /* UAE_UAE_H */
