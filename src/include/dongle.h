#ifndef UAE_DONGLE_H
#define UAE_DONGLE_H

#include "uae/types.h"

extern void dongle_reset (void);
extern uae_u8 dongle_cia_read (int, int, uae_u8, uae_u8);
extern void dongle_cia_write (int, int, uae_u8, uae_u8);
extern void dongle_joytest (uae_u16);
extern uae_u16 dongle_joydat (int, uae_u16);
extern void dongle_potgo (uae_u16);
extern uae_u16 dongle_potgor (uae_u16);
extern int dongle_analogjoy (int, int);

#endif /* UAE_DONGLE_H */
