 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Optimized blitter minterm function generator
  *
  * Copyright 1995,1996 Bernd Schmidt
  * Copyright 1996 Alessandro Bissacco
  */

#ifndef UAE_GENBLITTER_H
#define UAE_GENBLITTER_H

struct blitop {
    const char *s;
    int used;
};

extern struct blitop blitops[256];

#endif /* UAE_GENBLITTER_H */
