 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff, Amiberry version
  */

#pragma once
#include <SDL.h>

#define TARGET_NAME "amiberry"

#define NO_MAIN_IN_MAIN_C

#define OPTIONSFILENAME "uaeconfig"

STATIC_INLINE FILE *uae_tfopen(const TCHAR *path, const TCHAR *mode)
{
	return fopen(path, mode);
}

extern void fix_apmodes(struct uae_prefs *p);
extern int generic_main (int argc, char *argv[]);

#define OFFSET_Y_ADJUST 18

extern int emulating;

extern int z3_base_adr;

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
void graphics_subshutdown (void);

void stop_sound();

void keyboard_settrans();

extern void free_AmigaMem();
extern void alloc_AmigaMem();

extern void fetch_configurationpath(char *out, int size);
extern void set_configurationpath(char *newpath);

extern void fetch_controllerspath (char *out, int size);
extern void set_controllerspath(char *newpath);

extern void fetch_retroarchfile (char *out, int size);
extern void set_retroarchfile(char *newpath);


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
	char Name[MAX_DPATH];
	char Path[MAX_DPATH];
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
STATIC_INLINE void atomic_set(volatile uae_atomic *p, uae_u32 v)
{
	__sync_lock_test_and_set(p, v);
}