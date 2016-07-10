 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Joystick emulation prototypes
  *
  * Copyright 1995 Bernd Schmidt
  */

extern void read_joystick (int nr, int *dir, int *button);
extern void init_joystick (void);
extern void close_joystick (void);

extern void joystick_setting_changed (void);
