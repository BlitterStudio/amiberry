 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Keyboard buffer. Not really needed for X, but for SVGAlib and possibly
  * Mac and DOS ports.
  *
  * (c) 1996 Bernd Schmidt
  */

#pragma once
extern int get_next_key(void);
extern int keys_available(void);
extern int record_key(int);
extern int record_key_direct(int);
extern void keybuf_init(void);
extern int getcapslockstate(void);
extern void setcapslockstate(int);
extern void keybuf_inject(const uae_char*);
