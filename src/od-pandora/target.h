 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff, Pandora version
  *
  * Copyright 1997 Bernd Schmidt
  */

#define TARGET_NAME "pandora"

#define NO_MAIN_IN_MAIN_C

#define OPTIONSFILENAME "uaeconfig"

extern int emulating;

extern int z3_start_adr;
extern int rtg_start_adr;

void run_gui(void);
void InGameMessage(const char *msg);

void saveAdfDir(void);
void setCpuSpeed(void);
void resetCpuSpeed(void);
void update_display(struct uae_prefs *);
void black_screen_now(void);
void graphics_subshutdown (void);

void pandora_stop_sound(void);

void keyboard_settrans (void);
int translate_pandora_keys(int symbol, int *modifier);
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

void reinit_amiga(void);
int count_HDs(struct uae_prefs *p);
extern void gui_force_rtarea_hdchange(void);
extern bool hardfile_testrdb (const TCHAR *filename);
