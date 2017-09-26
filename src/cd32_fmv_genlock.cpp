/*
* UAE - The Un*x Amiga Emulator
*
* CD32 FMV video genlock
*
* Copyright 2014 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "cd32_fmv.h"
#include "custom.h"
#include "xwin.h"

static uae_u8 *mpeg_out_buffer;
static int mpeg_width, mpeg_height, mpeg_depth;
static uae_u32 fmv_border_color;
static uae_u16 fmv_border_color_16;
int cd32_fmv_active;

// According to schematics there is at least 3 (possible more)
// "genlock modes" but they seem to be unused, at least ROM
// mpeg player software never sets any genlock bits.

// this is probably not how it actually works.
#define GENLOCK_VAL_32 0x20
#define GENLOCK_VAL_16 0x03

#define MPEG_PIXBYTES_32 4
#define MPEG_PIXBYTES_16 2
#define MAX_MPEG_WIDTH 352
#define MAX_MPEG_HEIGHT 288

void cd32_fmv_new_border_color(uae_u32 bordercolor)
{
	fmv_border_color = bordercolor;
	fmv_border_color_16  = (((bordercolor >> (16 + 3)) & 31) << 11);
	fmv_border_color_16 |= (((bordercolor >> ( 8 + 2)) & 63) << 5);
	fmv_border_color_16 |= (((bordercolor >> ( 0 + 3)) & 31) << 0);
}

void cd32_fmv_new_image(int w, int h, int d, uae_u8 *s)
{
	if (!mpeg_out_buffer)
		mpeg_out_buffer = xmalloc(uae_u8, MAX_MPEG_WIDTH * MAX_MPEG_HEIGHT * MPEG_PIXBYTES_32);
	if (s == NULL || w > MAX_MPEG_WIDTH || h > MAX_MPEG_HEIGHT) {
		memset(mpeg_out_buffer, 0, MAX_MPEG_WIDTH * MAX_MPEG_HEIGHT * MPEG_PIXBYTES_32);
		mpeg_width = MAX_MPEG_WIDTH;
		mpeg_height = MAX_MPEG_HEIGHT;
		return;
	}
	memcpy(mpeg_out_buffer, s, w * h * d);
	mpeg_width = w;
	mpeg_height = h;
	mpeg_depth = d;
}

void cd32_fmv_state(int state)
{
	cd32_fmv_active = state;
}

// slow software method but who cares.


static void genlock_32(struct vidbuffer *vbin, struct vidbuffer *vbout, int w, int h, int d, int hoffset, int voffset, int mult)
{
	for (int hh = 0, sh = -voffset; hh < h; sh++, hh += mult) {
		for (int h2 = 0; h2 < mult; h2++) {
			uae_u8 *d8 = vbout->bufmem + vbout->rowbytes * (hh + h2 + voffset);
			uae_u32 *d32 = (uae_u32*)d8;
			uae_u8 *s8 = vbin->bufmem + vbin->rowbytes * (hh + h2 + voffset) ;
			uae_u32 *srcp = NULL;
			if (sh >= 0 && sh < mpeg_height)
				srcp = (uae_u32*)(mpeg_out_buffer + sh * mpeg_width * MPEG_PIXBYTES_32);
			for (int ww = 0, sw = -hoffset; ww < w; sw++, ww += mult) {
				uae_u32 sv = fmv_border_color;
				if (sw >= 0 && sw < mpeg_width && srcp)
					sv = srcp[sw];
				for (int w2 = 0; w2 < mult; w2++) {
					uae_u32 v;
					if (s8[0] >= GENLOCK_VAL_32) {
						v = *((uae_u32*)s8);
					} else {
						v = sv;
					}
					*d32++ = v;
					s8 += d;
				}
			}
		}
	}
}

static void genlock_16(struct vidbuffer *vbin, struct vidbuffer *vbout, int w, int h, int d, int hoffset, int voffset, int mult)
{
	for (int hh = 0, sh = -voffset; hh < h; sh++, hh += mult) {
		for (int h2 = 0; h2 < mult; h2++) {
			uae_u8 *d8 = vbout->bufmem + vbout->rowbytes * (hh + h2 + voffset);
			uae_u16 *d16 = (uae_u16*)d8;
			uae_u8 *s8 = vbin->bufmem + vbin->rowbytes * (hh + h2 + voffset) ;
			uae_u16 *srcp = NULL;
			if (sh >= 0 && sh < mpeg_height)
				srcp = (uae_u16*)(mpeg_out_buffer + sh * mpeg_width * MPEG_PIXBYTES_16);
			for (int ww = 0, sw = -hoffset; ww < w; sw++, ww += mult) {
				uae_u16 sv = fmv_border_color_16;
				if (sw >= 0 && sw < mpeg_width && srcp)
					sv = srcp[sw];
				for (int w2 = 0; w2 < mult; w2++) {
					uae_u32 v;
					if ((((uae_u16*)s8)[0] >> 11) >= GENLOCK_VAL_16) {
						v = *((uae_u16*)s8);
					} else {
						v = sv;
					}
					*d16++ = v;
					s8 += d;
				}
			}
		}
	}
}

static void genlock_16_nomult(struct vidbuffer *vbin, struct vidbuffer *vbout, int w, int h, int d, int hoffset, int voffset)
{
	for (int hh = 0, sh = -voffset; hh < h; sh++, hh++) {
		uae_u8 *d8 = vbout->bufmem + vbout->rowbytes * (hh + voffset);
		uae_u16 *d16 = (uae_u16*)d8;
		uae_u8 *s8 = vbin->bufmem + vbin->rowbytes * (hh + voffset) ;
		uae_u16 *srcp = NULL;
		if (sh >= 0 && sh < mpeg_height)
			srcp = (uae_u16*)(mpeg_out_buffer + sh * mpeg_width * MPEG_PIXBYTES_16);
		for (int ww = 0, sw = -hoffset; ww < w; sw++, ww++) {
			uae_u16 sv = fmv_border_color_16;
			if (sw >= 0 && sw < mpeg_width && srcp)
				sv = srcp[sw];
			uae_u32 v;
			if ((((uae_u16*)s8)[0] >> 11) >= GENLOCK_VAL_16) {
				v = *((uae_u16*)s8);
			} else {
				v = sv;
			}
			*d16++ = v;
			s8 += d;
		}
	}
}

void cd32_fmv_genlock(struct vidbuffer *vbin, struct vidbuffer *vbout)
{
	int hoffset, voffset, mult;
	int w = vbin->outwidth;
	int h = vbin->outheight;
	int d = vbin->pixbytes;

	if (!mpeg_out_buffer)
		return;

	mult = 1;
	for (;;) {
		if (mult < 4 && mpeg_width * (mult << 1) <= w + 8 && mpeg_height * (mult << 1) <= h + 8) {
			mult <<= 1;
		}
		else {
			break;
		}
	}

	hoffset = (w / mult - mpeg_width) / 2;
	voffset = (h / mult - mpeg_height) / 2;

	if (hoffset < 0)
		hoffset = 0;
	if (voffset < 0)
		voffset = 0;

	if (mpeg_depth == 2) {
		if(mult == 1)
		  genlock_16_nomult(vbin, vbout, w, h, d, hoffset, voffset);
		else
  		genlock_16(vbin, vbout, w, h, d, hoffset, voffset, mult);
	}
	else
		genlock_32(vbin, vbout, w, h, d, hoffset, voffset, mult);
}
