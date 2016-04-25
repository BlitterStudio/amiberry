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
extern int uae4all_keystate[256];

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

void keyboard_init(void);

void reinit_amiga(void);
int count_HDs(struct uae_prefs *p);
extern void gui_force_rtarea_hdchange(void);
extern bool hardfile_testrdb (const TCHAR *filename);
