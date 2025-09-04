#ifndef UAE_GFXFILTER_H
#define UAE_GFXFILTER_H

#include "uae/types.h"

#ifdef GFXFILTER

struct displayscale
{
	// max output size
	int dstwidth, dstheight;
	// input bitmap size
	int srcwidth, srcheight;
	// output size
	int outwidth, outheight;

	int mx, my;
	float mmx, mmy;
	int xoffset, yoffset;
	int mode;
	int scale;
};

void getfilterdata(int monid, struct displayscale *ds);
void getfilteroffset(int monid, float *dx, float *dy, float *mx, float *my);

uae_u8 *getfilterbuffer(int monid, int *widthp, int *heightp, int *pitch, int *depth, bool *locked);
void freefilterbuffer(int monid, uae_u8*, bool unlock);

uae_u8 *uaegfx_getrtgbuffer(int monid, int *widthp, int *heightp, int *pitch, int *depth, uae_u8 *palette);
void uaegfx_freertgbuffer(int monid, uae_u8 *dst);

extern void getrtgfilterdata(int monid, struct displayscale *ds);
extern float filterrectmult(int v1, float v2, int dmode);

#endif /* GFXFILTER */

#endif /* UAE_GFXFILTER_H */
