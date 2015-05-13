 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Joystick emulation prototypes
  *
  * Copyright 1995 Bernd Schmidt
  */

extern void read_joystick (int nr, unsigned int *dir, int *button);
extern void handle_joymouse(void);
extern void init_joystick (void);
extern void close_joystick (void);
