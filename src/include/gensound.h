 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Prototypes for general sound related functions
  * This use to be called sound.h, but that causes confusion
  *
  * Copyright 1997 Bernd Schmidt
  */

#ifndef UAE_GENSOUND_H
#define UAE_GENSOUND_H

extern int sound_available;

extern void (*sample_handler) (void);
/* sample_evtime is in normal Amiga cycles; scaled_sample_evtime is in our
   event cycles. */
extern unsigned long scaled_sample_evtime;

/* Determine if we can produce any sound at all.  This can be only a guess;
 * if unsure, say yes.  Any call to init_sound may change the value.  */
extern int setup_sound (void);

extern int init_sound (void);
extern void close_sound (void);

extern void sample16_handler (void);
extern void sample16s_handler (void);

#endif /* UAE_GENSOUND_H */
