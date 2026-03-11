/*
 * gfx_colors.cpp - Pixel format, color tables, and palette management
 *
 * Extracted from amiberry_gfx.cpp for single-responsibility decomposition.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "picasso96.h"
#include "gfxboard.h"
#include "amiberry_gfx.h"
#include "fsdb_host.h"
#include "gfx_colors.h"

#include "gfx_platform_internal.h"

#ifdef AMIBERRY

void update_pixel_format()
{
	// TODO LIBRETRO support picasso96 ABGR
	if (gfx_platform_override_pixel_format(&pixel_format))
		return;

	const bool is_packed_16bit_rtg =
		picasso96_state[0].RGBFormat == RGBFB_R5G6B5 ||
		picasso96_state[0].RGBFormat == RGBFB_R5G6B5PC ||
		picasso96_state[0].RGBFormat == RGBFB_R5G5B5 ||
		picasso96_state[0].RGBFormat == RGBFB_R5G5B5PC ||
		picasso96_state[0].RGBFormat == RGBFB_B5G6R5PC ||
		picasso96_state[0].RGBFormat == RGBFB_B5G5R5PC ||
		picasso96_state[0].RGBFormat == RGBFB_Y4U2V2;

	if (is_packed_16bit_rtg) {
		// KMSDRM's SDL renderer presents 32-bit RTG correctly but the 16-bit
		// streaming texture path can go black. Keep the emulated RTG mode
		// unchanged and only widen the host presentation format on KMSDRM.
		if (kmsdrm_detected) {
			pixel_format = currprefs.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE
				? SDL_PIXELFORMAT_ARGB8888
				: SDL_PIXELFORMAT_ABGR8888;
			return;
		}

		pixel_format = SDL_PIXELFORMAT_RGB565;
		return;
	}

	if (currprefs.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE)
	{
		pixel_format = SDL_PIXELFORMAT_ARGB8888; // Custom boards (e.g. PicassoII) output BGRA
	}
	else {
		pixel_format = SDL_PIXELFORMAT_ABGR8888; // Default for native output, UAE RTG (32-bit)
	}
}

/* Color management */

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

void init_colors(const int monid)
{
	update_pixel_format();
	AmigaMonitor* mon = &AMonitors[monid];
	/* init colors */

	red_bits = green_bits = blue_bits = 8;
	red_bits = green_bits = blue_bits = 8;

	SDL_PixelFormat *pf = SDL_AllocFormat(pixel_format);
	if (pf) {
		red_shift = pf->Rshift;
		green_shift = pf->Gshift;
		blue_shift = pf->Bshift;
		alpha_shift = pf->Ashift;
		SDL_FreeFormat(pf);
	} else {
		// Fallback defaults if allocation fails
		if (pixel_format == SDL_PIXELFORMAT_ARGB8888) {
			// BGRA
			red_shift = 16; green_shift = 8; blue_shift = 0; alpha_shift = 24;
		} else {
			// RGBA
			red_shift = 0; green_shift = 8; blue_shift = 16; alpha_shift = 24;
		}
	}

	alloc_colors64k(monid, red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, alpha_bits, alpha_shift, alpha, 0);
	notice_new_xcolors();
#ifdef AVIOUTPUT
	AVIOutput_RGBinfo(red_bits, green_bits, blue_bits, alpha_bits, red_shift, green_shift, blue_shift, alpha_shift);
#endif
	//Screenshot_RGBinfo (red_bits, green_bits, blue_bits, alpha_bits, red_shift, green_shift, blue_shift, alpha_shift);
}

#ifdef PICASSO96

int picasso_palette(struct MyCLUTEntry *CLUT, uae_u32 *clut)
{
	int changed = 0;

	for (int i = 0; i < 256 * 2; i++) {
		const int r = CLUT[i].Red;
		const int g = CLUT[i].Green;
		const int b = CLUT[i].Blue;
		const uae_u32 v = (doMask256 (r, red_bits, red_shift)
			| doMask256(g, green_bits, green_shift)
			| doMask256(b, blue_bits, blue_shift))
			| doMask256 ((1 << alpha_bits) - 1, alpha_bits, alpha_shift);
		if (v != clut[i]) {
			//write_log (_T("%d:%08x\n"), i, v);
			clut[i] = v;
			changed = 1;
		}
	}
	return changed;
}

void gfx_set_picasso_colors(int monid, const RGBFTYPE rgbfmt)
{
	alloc_colors_picasso(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, rgbfmt, p96_rgbx16);
}

#endif // PICASSO96

#endif // AMIBERRY
