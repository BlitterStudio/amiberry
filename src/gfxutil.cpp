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
#include "machdep/maccess.h"

#include <cmath>

#include <algorithm>

float getvsyncrate(int monid, float hz, int *mult)
{
	struct amigadisplay *ad = &adisplays[monid];
	struct apmode *ap = ad->picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];

	if (hz < 0)
		return 0;
	if (hz > 85) {
		*mult = -1;
		return hz / 2;
	}
	if (hz < 35 && hz > 0) {
		if (ap->gfx_interlaced)
			*mult = 0;
		else
			*mult = 1;
		return hz * 2;
	}
	*mult = 0;
	return hz;
}

#define RED	0
#define GRN	1
#define BLU	2

uae_u32 doMask(uae_u32 p, int bits, int shift)
{
	/* scale to 0..255, shift to align msb with mask, and apply mask */
	uae_u32 val;

	if (flashscreen)
		p ^= 0xff;
	val = p << 24;
	if (!bits)
		return 0;
	val >>= (32 - bits);
	val <<= shift;

	return val;
}

int bits_in_mask (unsigned long mask)
{
	int n = 0;
	while (mask) {
		n += mask & 1;
		mask >>= 1;
	}
	return n;
}

int mask_shift (unsigned long mask)
{
	int n = 0;
	if (!mask)
		return 0;
	while (!(mask & 1)) {
		n++;
		mask >>= 1;
	}
	return n;
}

uae_u32 doMask256 (int p, int bits, int shift)
{
	/* p is a value from 0 to 255 (Amiga color value)
	* shift to align msb with mask, and apply mask */

	uae_u32 val = p;
	if (bits == 0)
		return 0;
	val <<= 24;
	val >>= (32 - bits);
	val <<= shift;

	return val;
}

static uae_u32 doColor (int i, int bits, int shift)
{
	int shift2;

	if (flashscreen)
		i ^= 0xffffffff;
	if (bits >= 8)
		shift2 = 0;
	else
		shift2 = 8 - bits;
	return (i >> shift2) << shift;
}

static uae_u32 doAlpha (int alpha, int bits, int shift)
{
	return (alpha & ((1 << bits) - 1)) << shift;
}

static float video_gamma (float value, float gamma, float bri, float con)
{
	float factor;
	float ret;

	value += bri;
	value *= con;

	if (value <= 0.0f)
		return 0.0f;

	factor = (float)pow(255.0f, 1.0f - gamma);
	ret = (float)(factor * pow(value, gamma));

	ret = std::max(ret, 0.0f);

	return ret;
}

static uae_u32 gamma_table[256 * 3][3];
static int lf, hf;

static void video_calc_gammatable(int monid)
{
	struct amigadisplay *ad = &adisplays[monid];
	struct gfx_filterdata *fd = &currprefs.gf[ad->gf_index];
	float bri, con, gam, gams[3];
	float max = 255;

	if (gfx_hdr) {
		bri = 0.0;
		con = 1.0;
		gam = 0.0;
	} else {
		bri = ((float)(currprefs.gfx_luminance)) * (128.0f / 1000.0f);
		con = ((float)(currprefs.gfx_contrast + 1000)) / 1000.0f;
		gam = ((float)(1000 - currprefs.gfx_gamma)) / 1000.0f - 1.0f;
	}

	gams[0] = gam + ((float)(1000 - currprefs.gfx_gamma_ch[0])) / 1000.0f;
	gams[1] = gam + ((float)(1000 - currprefs.gfx_gamma_ch[1])) / 1000.0f;
	gams[2] = gam + ((float)(1000 - currprefs.gfx_gamma_ch[2])) / 1000.0f;

	lf = 64 * currprefs.gf[ad->gf_index].gfx_filter_blur / 1000;
	hf = 256 - lf * 2;

	for (int i = 0; i < (256 * 3); i++) {
		for (int j = 0; j < 3; j++) {
			float val = i - 256.0f;
			float v;

			if (currprefs.gfx_threebitcolors == 2) {
				val *= 2;
			} else if (currprefs.gfx_threebitcolors == 3) {
				val = (float)((val * 252.0) / 119.0);
			} else if (currprefs.gfx_threebitcolors == 1) {
				val = (float)((val * 252.0) / 238.0);
			}

			if (gfx_hdr || (currprefs.gfx_luminance == 0 && currprefs.gfx_contrast == 0 && currprefs.gfx_gamma == 0 &&
				currprefs.gfx_gamma_ch[0] == 0 && currprefs.gfx_gamma_ch[1] == 0 && currprefs.gfx_gamma_ch[2] == 0)) {
				v = val;
			} else {
				v = video_gamma(val, gams[j], bri, con);
			}

			v = std::max<double>(v, 0.0);
			v = std::min(v, max);

			gamma_table[i][j] = (uae_u32)(v + 0.5);
		}
	}
}

static uae_u32 lowbits (int v, int shift, int lsize)
{
	v >>= shift;
	v &= (1 << lsize) - 1;
	return v;
}

void alloc_colors_picasso (int rw, int gw, int bw, int rs, int gs, int bs, int rgbfmt, uae_u32 *rgbx16)
{
#ifdef PICASSO96
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
	case RGBFB_Y4U2V2:
	case RGBFB_Y4U1V1:
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
		red_bits = green_bits = blue_bits = 5;
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

#ifdef WORDS_BIGENDIAN
	byte_swap = !byte_swap;
#endif

	memset (rgbx16, 0, 65536 * sizeof(uae_u32));

	if (red_bits) {
		int lrbits = 8 - red_bits;
		int lgbits = 8 - green_bits;
		int lbbits = 8 - blue_bits;
		int lrmask = (1 << red_bits) - 1;
		int lgmask = (1 << green_bits) - 1;
		int lbmask = (1 << blue_bits) - 1;
		for (i = 65535; i >= 0; i--) {
			uae_u32 r, g, b, c;
			uae_u32 j = byte_swap ? do_byteswap_16(i) : i;
			r = (((j >>   red_shift) & lrmask) << lrbits) | lowbits (j,   red_shift, lrbits);
			g = (((j >> green_shift) & lgmask) << lgbits) | lowbits (j, green_shift, lgbits);
			b = (((j >>  blue_shift) & lbmask) << lbbits) | lowbits (j,  blue_shift, lbbits);
			c = doMask(r, rw, rs) | doMask(g, gw, gs) | doMask(b, bw, bs);
			if (bpp <= 16)
				c *= 0x00010001;
			rgbx16[i] = c;
		}
	}
#endif
}

#define BLACKERTHANBLACKADJ 4

void alloc_colors_rgb (int rw, int gw, int bw, int rs, int gs, int bs, int aw, int as, int alpha, int byte_swap,
	uae_u32 *rc, uae_u32 *gc, uae_u32 *bc)
{
	int bpp = rw + gw + bw + aw;
	int i;
	for(i = 0; i < 256; i++) {
		int j;

		if (!gfx_hdr && currprefs.gfx_blackerthanblack) {
			j = i * (255 - BLACKERTHANBLACKADJ) / 255  + BLACKERTHANBLACKADJ;
		} else {
			j = i;
		}
		j += 256;

		rc[i] = doColor(gamma_table[j][0], rw, rs) | doAlpha(alpha, aw, as);
		gc[i] = doColor(gamma_table[j][1], gw, gs) | doAlpha(alpha, aw, as);
		bc[i] = doColor(gamma_table[j][2], bw, bs) | doAlpha(alpha, aw, as);

		if (byte_swap) {
			if (bpp <= 16) {
				rc[i] = do_byteswap_16(rc[i]);
				gc[i] = do_byteswap_16(gc[i]);
				bc[i] = do_byteswap_16(bc[i]);
			} else {
				rc[i] = do_byteswap_32(rc[i]);
				gc[i] = do_byteswap_32(gc[i]);
				bc[i] = do_byteswap_32(bc[i]);
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

void alloc_colors64k(int monid, int rw, int gw, int bw, int rs, int gs, int bs, int aw, int as, int alpha, int byte_swap)
{
	int bpp = rw + gw + bw + aw;
	int i, j;

	video_calc_gammatable(monid);
	j = 256;
	for (i = 0; i < 4096; i++) {
		int r = ((i >> 8) << 4) | (i >> 8);
		int g = (((i >> 4) & 0xf) << 4) | ((i >> 4) & 0x0f);
		int b = ((i & 0xf) << 4) | (i & 0x0f);

		if (!gfx_hdr && currprefs.gfx_blackerthanblack) {
			r = (r * (255 - BLACKERTHANBLACKADJ) / 255) + BLACKERTHANBLACKADJ;
			g = (g * (255 - BLACKERTHANBLACKADJ) / 255) + BLACKERTHANBLACKADJ;
			b = (b * (255 - BLACKERTHANBLACKADJ) / 255) + BLACKERTHANBLACKADJ;
		}
		r = gamma_table[r + j][0];
		g = gamma_table[g + j][1];
		b = gamma_table[b + j][2];
		xcolors[i] = doMask(r, rw, rs) | doMask(g, gw, gs) | doMask(b, bw, bs) | doAlpha(alpha, aw, as);
		if (byte_swap) {
			if (bpp <= 16) {
				xcolors[i] = do_byteswap_16(xcolors[i]);
			} else {
				xcolors[i] = do_byteswap_32(xcolors[i]);
			}
		}
		if (bpp <= 16) {
			/* Fill upper 16 bits of each colour value
			* with a copy of the colour. */
			xcolors[i] |= xcolors[i] * 0x00010001;
		}
	}

	fullblack = 0;
	if (gfx_hdr) {
		fullblack = doAlpha(1, aw, as);
	}

#if defined(AGA) || defined(GFXFILTER)
	alloc_colors_rgb (rw, gw, bw, rs, gs, bs, aw, as, alpha, byte_swap, xredcolors, xgreencolors, xbluecolors);
#endif

	xredcolor_b = rw;
	xgreencolor_b = gw;
	xbluecolor_b = bw;
	xredcolor_s = rs;
	xgreencolor_s = gs;
	xbluecolor_s = bs;
	xredcolor_m = (1 << rw) - 1;
	xgreencolor_m = (1 << gw) - 1;
	xbluecolor_m = (1 << bw) - 1;
}
