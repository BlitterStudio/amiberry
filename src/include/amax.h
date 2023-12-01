#ifndef UAE_AMAX_H
#define UAE_AMAX_H

#include "uae/types.h"

void amax_diskwrite (uae_u16 w);
void amax_bfe001_write (uae_u8 pra, uae_u8 dra);
uae_u8 amax_disk_status (uae_u8);
void amax_disk_select (uae_u8 v, uae_u8 ov, int);
void amax_reset (void);
void amax_init (void);
bool amax_active(void);

#endif /* UAE_AMAX_H */
