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

static bool p96_monitor_uses_hardware_rtg(const int monid)
{
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		const rtgboardconfig& rbc = currprefs.rtgboards[i];
		if (rbc.monitor_id == monid && rbc.rtgmem_type >= GFXBOARD_HARDWARE) {
			return true;
		}
	}
	return false;
}

SDL_PixelFormat p96_get_host_pixel_format(const RGBFTYPE rgbfmt, const bool hardware_rtg)
{
	if (hardware_rtg || rgbfmt == RGBFB_B8G8R8A8) {
		return SDL_PIXELFORMAT_ARGB8888;
	}
	return SDL_PIXELFORMAT_ABGR8888;
}

SDL_PixelFormat p96_get_host_pixel_format_for_monitor(const int monid)
{
	const int safe_monid = monid >= 0 && monid < MAX_AMIGAMONITORS ? monid : 0;
	return p96_get_host_pixel_format(picasso96_state[safe_monid].RGBFormat, p96_monitor_uses_hardware_rtg(safe_monid));
}

bool p96_rgbformat_matches_host_pixel_format(const RGBFTYPE rgbfmt, const SDL_PixelFormat format)
{
	return (rgbfmt == RGBFB_R8G8B8A8 && format == SDL_PIXELFORMAT_ABGR8888) ||
		(rgbfmt == RGBFB_B8G8R8A8 && format == SDL_PIXELFORMAT_ARGB8888);
}

RGBFTYPE p96_get_host_rgb_format(const SDL_PixelFormat format, const int pixbytes)
{
	if (pixbytes == 4) {
		return format == SDL_PIXELFORMAT_ARGB8888 ? RGBFB_B8G8R8A8 : RGBFB_R8G8B8A8;
	}
	return RGBFB_B5G6R5PC;
}

RGBFTYPE p96_get_host_rgb_format_for_monitor(const int monid, const int pixbytes)
{
	return p96_get_host_rgb_format(p96_get_host_pixel_format_for_monitor(monid), pixbytes);
}

void update_pixel_format(const int monid)
{
	// TODO LIBRETRO support picasso96 ABGR
	if (gfx_platform_override_pixel_format(&pixel_format))
		return;

	// Keep RTG presentation on the mature 32-bit host path. The native
	// 16-bit host path is incomplete across backends, so 15/16-bit RTG modes
	// are converted into one of these directly presentable 32-bit layouts.
	pixel_format = p96_get_host_pixel_format_for_monitor(monid);
}

/* Color management */

static int red_bits, green_bits, blue_bits, alpha_bits;
static int red_shift, green_shift, blue_shift, alpha_shift;
static int alpha;

void init_colors(const int monid)
{
	update_pixel_format(monid);
	AmigaMonitor* mon = &AMonitors[monid];
	/* init colors */

	red_bits = green_bits = blue_bits = 8;

	const SDL_PixelFormatDetails *pf = SDL_GetPixelFormatDetails(pixel_format);
	if (pf) {
		red_shift = pf->Rshift;
		green_shift = pf->Gshift;
		blue_shift = pf->Bshift;
		alpha_shift = pf->Ashift;
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
