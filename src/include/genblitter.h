 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Optimized blitter minterm function generator
  *
  * Copyright 1995,1996 Bernd Schmidt
  * Copyright 1996 Alessandro Bissacco
  */

struct blitop {
    const char *s;
    int used;
};

extern struct blitop blitops[256];
