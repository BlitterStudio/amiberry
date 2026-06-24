/*
* UAE - The Un*x Amiga Emulator
*
* Vortex ATonce Plus emulation (286 board in the A500 68000 socket)
*/

#ifndef UAE_ATONCE_H
#define UAE_ATONCE_H

bool atonce_init(struct autoconfig_info *aci);
bool atonce_available(void);
// hook called from cia.cpp; returns 1 if the access hit a magic port
int atonce_cia_access(uaecptr addr, uae_u32 *value, int size, int iswrite);

extern int atonce_intr_vector;

#endif /* UAE_ATONCE_H */
