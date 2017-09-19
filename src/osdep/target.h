 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff, Amiberry version
  *
  * Copyright 1997 Bernd Schmidt
  */

#pragma once
#include <SDL.h>

#define TARGET_NAME "amiberry"

#define NO_MAIN_IN_MAIN_C

#define OPTIONSFILENAME "uaeconfig"

STATIC_INLINE FILE *uae_tfopen(const char *path, const char *mode)
{
	return fopen(path, mode);
}

extern int emulating;

extern uae_u8* natmem_offset;
extern int z3base_adr;

extern int currVSyncRate;
extern unsigned long time_per_frame;

void run_gui(void);
void InGameMessage(const char *msg);
void init_max_signals(void);
void wait_for_vsync(void);
unsigned long target_lastsynctime(void);
extern int screen_is_picasso;

void saveAdfDir(void);
void update_display(struct uae_prefs *);
void black_screen_now(void);
void graphics_subshutdown(void);

void amiberry_stop_sound();

void keyboard_settrans();
void translate_amiberry_keys(int symbol, int newstate);
void SimulateMouseOrJoy(int code, int keypressed);

#define REMAP_MOUSEBUTTON_LEFT    -1
#define REMAP_MOUSEBUTTON_RIGHT   -2
#define REMAP_JOYBUTTON_ONE       -3
#define REMAP_JOYBUTTON_TWO       -4
#define REMAP_JOY_UP              -5
#define REMAP_JOY_DOWN            -6
#define REMAP_JOY_LEFT            -7
#define REMAP_JOY_RIGHT           -8
#define REMAP_CD32_GREEN          -9
#define REMAP_CD32_YELLOW         -10
#define REMAP_CD32_PLAY           -11
#define REMAP_CD32_FFW            -12
#define REMAP_CD32_RWD            -13

extern void free_AmigaMem();
extern void alloc_AmigaMem();

extern void fetch_configurationpath(char *out, int size);
extern void set_configurationpath(char *newpath);
extern void set_rompath(char *newpath);
extern void fetch_rp9path(char *out, int size);
extern void fetch_savestatepath(char *out, int size);
extern void fetch_screenshotpath(char *out, int size);

extern void extractFileName(const char * str, char *buffer);
extern void extractPath(char *str, char *buffer);
extern void removeFileExtension(char *filename);
extern void ReadConfigFileList(void);
extern void RescanROMs(void);
extern void ClearAvailableROMList(void);

#include <vector>
#include <string>
typedef struct {
	char Name[MAX_PATH];
	char Path[MAX_PATH];
	int ROMType;
} AvailableROM;
extern std::vector<AvailableROM*> lstAvailableROMs;

#define MAX_MRU_DISKLIST 40
extern std::vector<std::string> lstMRUDiskList;
extern void AddFileToDiskList(const char *file, int moveToTop);

#define MAX_MRU_CDLIST 10
extern std::vector<std::string> lstMRUCDList;
extern void AddFileToCDList(const char *file, int moveToTop);

#define AMIGAWIDTH_COUNT 6
#define AMIGAHEIGHT_COUNT 6
extern const int amigawidth_values[AMIGAWIDTH_COUNT];
extern const int amigaheight_values[AMIGAHEIGHT_COUNT];

int count_HDs(struct uae_prefs *p);
extern void gui_force_rtarea_hdchange(void);
extern void gui_restart(void);
extern bool hardfile_testrdb(const char *filename);

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

STATIC_INLINE void atomic_and(volatile uae_atomic *p, uae_u32 v)
{
	__sync_and_and_fetch(p, v);
}
STATIC_INLINE void atomic_or(volatile uae_atomic *p, uae_u32 v)
{
	__sync_or_and_fetch(p, v);
}
STATIC_INLINE uae_atomic atomic_inc(volatile uae_atomic *p)
{
	return __sync_add_and_fetch(p, 1);
}
STATIC_INLINE uae_atomic atomic_dec(volatile uae_atomic *p)
{
	return __sync_sub_and_fetch(p, 1);
}
STATIC_INLINE uae_u32 atomic_bit_test_and_reset(volatile uae_atomic *p, uae_u32 v)
{
	long mask = (1 << v);
	uae_u32 res = __sync_fetch_and_and(p, ~mask);
	return (res && mask);
}
