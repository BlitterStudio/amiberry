#ifndef UAE_TABLETLIBRARY_H
#define UAE_TABLETLIBRARY_H

#include "uae/types.h"

uaecptr tabletlib_startup(TrapContext*, uaecptr resaddr);
void tabletlib_install(void);

extern void tabletlib_tablet(int x, int y, int z,
	      int pressure, int maxpressure, uae_u32 buttonbits, int inproximity,
	      int ax, int ay, int az);
extern void tabletlib_tablet_info(int maxx, int maxy, int maxz, int maxax, int maxay, int maxaz, int xres, int yres);

#endif /* UAE_TABLETLIBRARY_H */
