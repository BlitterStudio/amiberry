 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Target specific stuff
  *
  * Copyright 1997 Bernd Schmidt
  */

#define TARGET_NAME "pandora"

#define OPTIONSFILENAME "uaeconfig"

/* Just 0x0 and not 680x0, so that constants can fit in ARM instructions */
#define M68000 0
#define M68020 2

#define SIMULATE_SHIFT 0x200
#define SIMULATE_RELEASED_SHIFT 0x400

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
