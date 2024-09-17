 /*
  * UAE - The Un*x Amiga Emulator
  *
  * A "replacement" for a missing Kickstart
  *
  * (c) 1995 Bernd Schmidt
  */

#ifndef UAE_ERSATZ_H
#define UAE_ERSATZ_H

#include "uae/types.h"

extern void init_ersatz_rom (uae_u8 *data);
extern void ersatz_chipcopy (void);
extern void ersatz_perform (uae_u16);
extern void DISK_ersatz_read (int,int, uaecptr);

#endif /* UAE_ERSATZ_H */
