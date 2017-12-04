 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Common code needed by all the various graphics systems.
  *
  * (c) 1996 Bernd Schmidt, Ed Hanway, Samuel Devulder
  */

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "rtgmodes.h"
#include "xwin.h"

#define	RED 	0
#define	GRN	1
#define	BLU	2

unsigned int doMask (int p, int bits, int shift)
{
  /* scale to 0..255, shift to align msb with mask, and apply mask */
  uae_u32 val;

	val = p << 24;
  if (!bits) 
    return 0;
  val >>= (32 - bits);
  val <<= shift;

  return val;
}


unsigned int doMask256 (int p, int bits, int shift)
{
  /* p is a value from 0 to 255 (Amiga color value)
   * shift to align msb with mask, and apply mask */

  unsigned long val = p * 0x01010101UL;
	if (bits == 0)
		return 0;
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

#ifndef ARMV6_ASSEMBLY
void alloc_colors_rgb (int rw, int gw, int bw, int rs, int gs, int bs, int byte_swap,
	uae_u32 *rc, uae_u32 *gc, uae_u32 *bc)
{
	int bpp = rw + gw + bw;
	int i;
	for(i = 0; i < 256; i++) {
		rc[i] = doColor (i, rw, rs);
		gc[i] = doColor (i, gw, gs);
		bc[i] = doColor (i, bw, bs);
		if (byte_swap) {
			if (bpp <= 16) {
				rc[i] = bswap_16 (rc[i]);
				gc[i] = bswap_16 (gc[i]);
				bc[i] = bswap_16 (bc[i]);
			} else {
				rc[i] = bswap_32 (rc[i]);
				gc[i] = bswap_32 (gc[i]);
				bc[i] = bswap_32 (bc[i]);
			}
		}
		if (bpp <= 16) {
			/* Fill upper 16 bits of each colour value with
			* a copy of the colour. */
			rc[i] = rc[i] * 0x00010001;
			gc[i] = gc[i] * 0x00010001;
			bc[i] = bc[i] * 0x00010001;
		}
	}
}
#endif

void alloc_colors64k (int rw, int gw, int bw, int rs, int gs, int bs, int byte_swap)
{
	int i;
	for (i = 0; i < 4096; i++) {
		int r = ((i >> 8) << 4) | (i >> 8);
		int g = (((i >> 4) & 0xf) << 4) | ((i >> 4) & 0x0f);
		int b = ((i & 0xf) << 4) | (i & 0x0f);
		xcolors[i] = doMask(r, rw, rs) | doMask(g, gw, gs) | doMask(b, bw, bs);
		/* Fill upper 16 bits of each colour value
		* with a copy of the colour. */
		xcolors[i] = xcolors[i] * 0x00010001;
	}
#ifndef ARMV6_ASSEMBLY
	alloc_colors_rgb (rw, gw, bw, rs, gs, bs, byte_swap, xredcolors, xgreencolors, xbluecolors);
#endif
}
