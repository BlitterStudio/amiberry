/*
 * gl_overlays.cpp - LED/OSD status line rendering (shared between GL and non-GL paths)
 *
 * OpenGL overlay rendering (OSD, bezel, cursor) has moved to OpenGLRenderer.
 *
 * Copyright 2025 Dimitris Panokostas
 */

#include <cstring>

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "picasso96.h"
#include "statusline.h"

#include "amiberry_gfx.h"
#include "gl_overlays.h"
#include "target.h"
#include "fsdb_host.h"
#include "gfx_platform_internal.h"

#ifdef AMIBERRY

void update_leds(const int monid)
{
	AmigaMonitor* mon = &AMonitors[monid];

	// Skip LED rendering if headless
	if (currprefs.headless) {
		return;
	}
	if (!gfx_platform_render_leds())
		return;

#ifndef USE_OPENGL
	if (!mon->amiga_renderer)
		return;
#endif

	// Use static variables to avoid recalculating color tables every frame
	static uae_u32 rc[256], gc[256], bc[256], a[256];
	static bool color_tables_initialized = false;

	// Only initialize color tables once for better performance
	if (!color_tables_initialized) {
		for (int i = 0; i < 256; i++) {
			// Using RGBA32 for the internal OSD surface
			rc[i] = i << 0;
			gc[i] = i << 8;
			bc[i] = i << 16;
			a[i] = i << 24;
		}
		color_tables_initialized = true;
	}

	const amigadisplay* ad = &adisplays[monid];
	const int m = statusline_get_multiplier(monid) / 100;
	const int led_height = TD_TOTAL_HEIGHT * m;
	int led_width = ad->picasso_on ? (amiga_surface ? amiga_surface->w : mon->currentmode.native_width) : 640;
	if (led_width <= 0)
		led_width = 640;

	// (Re)allocate OSD surface and texture if dimensions changed
	if (!mon->statusline_surface || mon->statusline_surface->w != led_width || mon->statusline_surface->h != led_height) {
		if (mon->statusline_surface) SDL_FreeSurface(mon->statusline_surface);
		mon->statusline_surface = SDL_CreateRGBSurfaceWithFormat(0, led_width, led_height, 32, SDL_PIXELFORMAT_RGBA32);

#ifndef USE_OPENGL
		if (mon->statusline_texture) {
			SDL_DestroyTexture(mon->statusline_texture);
			mon->statusline_texture = nullptr;
		}
#endif
	}

#ifndef USE_OPENGL
	if (!mon->statusline_texture) {
		mon->statusline_texture = SDL_CreateTexture(mon->amiga_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, led_width, led_height);
		SDL_SetTextureBlendMode(mon->statusline_texture, SDL_BLENDMODE_BLEND);
	}
#endif

	if (mon->statusline_surface) {
		// Clear with transparent color
		SDL_FillRect(mon->statusline_surface, nullptr, 0x00000000);

		// Draw the LEDs into the off-screen surface
		for (int y = 0; y < led_height; y++) {
			uae_u8* buf = static_cast<uae_u8*>(mon->statusline_surface->pixels) + y * mon->statusline_surface->pitch;
			draw_status_line_single(monid, buf, y, led_width, rc, gc, bc, a);
		}

#ifndef USE_OPENGL
		// Map the surface to the texture
		if (mon->statusline_texture)
			SDL_UpdateTexture(mon->statusline_texture, nullptr, mon->statusline_surface->pixels, mon->statusline_surface->pitch);
#endif
	}
}

#endif // AMIBERRY
