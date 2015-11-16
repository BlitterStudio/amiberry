 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Common code needed by all the various graphics systems.
  *
  * (c) 1996 Bernd Schmidt, Ed Hanway, Samuel Devulder
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "config.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "rtgmodes.h"
#include "keyboard.h"
#include "xwin.h"
#include "keybuf.h"

#define	RED 	0
#define	GRN	1
#define	BLU	2


unsigned int doMask (int p, int bits, int shift)
{
    /* p is a value from 0 to 15 (Amiga color value)
     * scale to 0..255, shift to align msb with mask, and apply mask */

    unsigned long val = p * 0x11111111UL;
    if (!bits) return 0;
    val >>= (32 - bits);
    val <<= shift;

    return val;
}


unsigned int doMask256 (int p, int bits, int shift)
{
    /* p is a value from 0 to 255 (Amiga color value)
     * shift to align msb with mask, and apply mask */

    unsigned long val = p * 0x01010101UL;
    val >>= (32 - bits);
    val <<= shift;

    return val;
}

static unsigned int doColor(int i, int bits, int shift)
{
    int shift2;
    if(bits >= 8) 
      shift2 = 0; 
    else 
      shift2 = 8 - bits;
    return (i >> shift2) << shift;
}

static uae_u32 lowbits (int v, int shift, int lsize)
{
    v >>= shift;
    v &= (1 << lsize) - 1;
    return v;
}

#ifdef PICASSO96
void alloc_colors_picasso (int rw, int gw, int bw, int rs, int gs, int bs, int rgbfmt)
{
    int byte_swap = 0;
    int i;
    int red_bits = 0, green_bits, blue_bits;
    int red_shift, green_shift, blue_shift;

    int bpp = rw + gw + bw;

    switch (rgbfmt)
    {
	case RGBFB_R5G6B5PC:
	red_bits = 5;
	green_bits = 6;
	blue_bits = 5;
	red_shift = 11;
	green_shift = 5;
	blue_shift = 0;
	break;
	case RGBFB_R5G5B5PC:
	red_bits = green_bits = blue_bits = 5;
	red_shift = 10;
	green_shift = 5;
	blue_shift = 0;
	break;
	case RGBFB_R5G6B5:
	red_bits = 5;
	green_bits = 6;
	blue_bits = 5;
	red_shift = 11;
	green_shift = 5;
	blue_shift = 0;
	byte_swap = 1;
	break;
	case RGBFB_R5G5B5:
	red_bits = green_bits = blue_bits = 5;
	red_shift = 10;
	green_shift = 5;
	blue_shift = 0;
	byte_swap = 1;
	break;
	case RGBFB_B5G6R5PC:
	red_bits = 5;
	green_bits = 6;
	blue_bits = 5;
	red_shift = 0;
	green_shift = 5;
	blue_shift = 11;
	break;
	case RGBFB_B5G5R5PC:
	red_bits = 5;
	green_bits = 5;
	blue_bits = 5;
	red_shift = 0;
	green_shift = 5;
	blue_shift = 10;
	break;
	default:
	red_bits = rw;
	green_bits = gw;
	blue_bits = bw;
	red_shift = rs;
	green_shift = gs;
	blue_shift = bs;
	break;
    }

    memset (p96_rgbx16, 0, sizeof p96_rgbx16);

    if (red_bits) {
	int lrbits = 8 - red_bits;
	int lgbits = 8 - green_bits;
	int lbbits = 8 - blue_bits;
	int lrmask = (1 << red_bits) - 1;
	int lgmask = (1 << green_bits) - 1;
	int lbmask = (1 << blue_bits) - 1;
	for (i = 65535; i >= 0; i--) {
	    uae_u32 r, g, b, c;
	    uae_u32 j = byte_swap ? bswap_16 (i) : i;
	    r = (((j >>   red_shift) & lrmask) << lrbits) | lowbits (j,   red_shift, lrbits);
	    g = (((j >> green_shift) & lgmask) << lgbits) | lowbits (j, green_shift, lgbits);
	    b = (((j >>  blue_shift) & lbmask) << lbbits) | lowbits (j,  blue_shift, lbbits);
	    c = doMask(r, rw, rs) | doMask(g, gw, gs) | doMask(b, bw, bs);
	    if (bpp <= 16)
		c *= 0x00010001;
	    p96_rgbx16[i] = c;
	}
    }
}
#endif

void alloc_colors64k (int rw, int gw, int bw, int rs, int gs, int bs, int byte_swap)
{
	int i;
	for (i = 0; i < 4096; i++) {
		int r = i >> 8;
		int g = (i >> 4) & 0xF;
		int b = i & 0xF;
		xcolors[i] = doMask(r, rw, rs) | doMask(g, gw, gs) | doMask(b, bw, bs);
	}
	/* create AGA color tables */
	for(i=0; i<256; i++) {
		xredcolors[i] = doColor(i, rw, rs);
		xgreencolors[i] = doColor(i, gw, gs);
		xbluecolors[i] = doColor(i, bw, bs);
	}
}
