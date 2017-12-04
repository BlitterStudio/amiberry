 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Keyboard buffer. Not really needed for X, but for SVGAlib and possibly
  * Mac and DOS ports.
  *
  * (c) 1996 Bernd Schmidt
  */

#ifndef UAE_KEYBUF_H
#define UAE_KEYBUF_H

extern int get_next_key (void);
extern int keys_available (void);
extern int record_key (int);
extern void keybuf_init (void);

#endif /* UAE_KEYBUF_H */
