 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Drive Click emulation stuff
  *
  * Copyright 2004 James Bagg, Toni Wilen
  */


struct drvsample {
    int len;
    int pos;
    uae_s16 *p;
};

#define DS_CLICK 0
#define DS_SPIN 1
#define DS_SPINND 2
#define DS_START 3
#define DS_SNATCH 4
#define DS_END 5

extern void driveclick_click(int drive, int startOffset);
extern void driveclick_motor(int drive, int running);
extern void driveclick_insert(int drive, int eject);
extern void driveclick_init(void);
extern void driveclick_free(void);
extern void driveclick_reset(void);
extern void driveclick_mix(uae_s16*, int);
extern int driveclick_loadresource(struct drvsample*, int);
extern void driveclick_check_prefs (void);
extern uae_s16 *decodewav (uae_u8 *s, int *len);

#define DS_BUILD_IN_SOUNDS 1
#define DS_NAME_CLICK "drive_click_"
#define DS_NAME_SPIN "drive_spin_"
#define DS_NAME_SPIN_ND "drive_spinnd_"
#define DS_NAME_START "drive_start_"
#define DS_NAME_SNATCH "drive_snatch_"

