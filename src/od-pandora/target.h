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
extern int stylusAdjustX;
extern int stylusAdjustY;

void run_gui(void);

void saveAdfDir(void);
void setCpuSpeed(void);
void resetCpuSpeed(void);
void update_display(struct uae_prefs *);
void graphics_subshutdown (void);

void pandora_stop_sound(void);

void keyboard_init(void);

void reinit_amiga(void);
int count_HDs(struct uae_prefs *p);
