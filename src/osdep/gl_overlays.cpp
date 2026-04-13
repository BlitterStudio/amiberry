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
#include "gui.h"
#include "inputdevice.h"
#include "picasso96.h"
#include "statusline.h"

#include "amiberry_gfx.h"
#include "irenderer.h"
#include "gl_overlays.h"
#include "target.h"
#include "fsdb_host.h"
#include "gfx_platform_internal.h"

#ifdef AMIBERRY

struct led_state_snapshot {
	uae_u8 powerled_brightness;
	bool capslock;
	uae_s8 drive_side;
	uae_s8 hd;
	uae_s8 cd;
	uae_s8 md;
	uae_s8 net;
	int cpu_halted;
	int cpu_stopped;
	int fps;
	int fps_color;
	int idle;
	int sndbuf;
	int sndbuf_status;
	bool sndbuf_avail;
	bool pause;
	bool picasso_on;
#ifdef AMIBERRY
	int temperature;
#endif
	struct {
		bool drive_motor;
		uae_u8 drive_track;
		bool drive_writing;
		bool drive_disabled;
		bool floppy_protected;
		bool floppy_inserted;
	} drives[4];
};

static bool capture_led_state(int monid, led_state_snapshot& snap)
{
	const amigadisplay* ad = &adisplays[monid];
	snap.powerled_brightness = gui_data.powerled_brightness;
	snap.capslock = gui_data.capslock;
	snap.drive_side = gui_data.drive_side;
	snap.hd = gui_data.hd;
	snap.cd = gui_data.cd;
	snap.md = gui_data.md;
	snap.net = gui_data.net;
	snap.cpu_halted = gui_data.cpu_halted;
	snap.cpu_stopped = gui_data.cpu_stopped;
	snap.fps = gui_data.fps;
	snap.fps_color = gui_data.fps_color;
	snap.idle = gui_data.idle;
	snap.sndbuf = gui_data.sndbuf;
	snap.sndbuf_status = gui_data.sndbuf_status;
	snap.sndbuf_avail = gui_data.sndbuf_avail;
	snap.pause = pause_emulation;
	snap.picasso_on = ad->picasso_on;
#ifdef AMIBERRY
	snap.temperature = gui_data.temperature;
#endif
	for (int i = 0; i < 4; i++) {
		snap.drives[i].drive_motor = gui_data.drives[i].drive_motor;
		snap.drives[i].drive_track = gui_data.drives[i].drive_track;
		snap.drives[i].drive_writing = gui_data.drives[i].drive_writing;
		snap.drives[i].drive_disabled = gui_data.drives[i].drive_disabled;
		snap.drives[i].floppy_protected = gui_data.drives[i].floppy_protected;
		snap.drives[i].floppy_inserted = gui_data.drives[i].floppy_inserted;
	}
	return true;
}

void update_leds(const int monid)
{
	AmigaMonitor* mon = &AMonitors[monid];

	if (currprefs.headless) {
		return;
	}
	if (!gfx_platform_render_leds())
		return;

	static uae_u32 rc[256], gc[256], bc[256], a[256];
	static bool color_tables_initialized = false;

	if (!color_tables_initialized) {
		for (int i = 0; i < 256; i++) {
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

	// Calculate message area height (use at least 2x scale for readability)
	const TCHAR* msg_text = statusline_fetch();
	const int msg_m = m < 2 ? 2 : m;
	const int msg_line_height = (LDP_CHAR_HEIGHT + 4) * msg_m;
	const int msg_height = msg_text ? msg_line_height : 0;
	const int total_height = led_height + msg_height;

	bool surface_exists = mon->statusline_surface
		&& mon->statusline_surface->w == led_width
		&& mon->statusline_surface->h == total_height;

	if (surface_exists) {
		static led_state_snapshot prev_snap = {};
		static bool prev_valid = false;
		static const TCHAR* prev_msg = nullptr;
		led_state_snapshot cur_snap = {};
		capture_led_state(monid, cur_snap);
		if (prev_valid && memcmp(&cur_snap, &prev_snap, sizeof(cur_snap)) == 0
			&& prev_msg == msg_text) {
			return;
		}
		prev_snap = cur_snap;
		prev_valid = true;
		prev_msg = msg_text;
	}

	if (!surface_exists) {
		if (mon->statusline_surface) SDL_DestroySurface(mon->statusline_surface);
		mon->statusline_surface = SDL_CreateSurface(led_width, total_height, SDL_PIXELFORMAT_RGBA32);
	}

	if (mon->statusline_surface) {
		SDL_FillSurfaceRect(mon->statusline_surface, nullptr, 0x00000000);

		// Render statusline message at the top (if any)
		if (msg_text && msg_height > 0) {
			statusline_render(monid,
				static_cast<uae_u8*>(mon->statusline_surface->pixels),
				mon->statusline_surface->pitch, led_width, msg_height,
				rc, gc, bc, a);
		}

		// Render LED bar below the message area
		for (int y = 0; y < led_height; y++) {
			uae_u8* buf = static_cast<uae_u8*>(mon->statusline_surface->pixels)
				+ (y + msg_height) * mon->statusline_surface->pitch;
			draw_status_line_single(monid, buf, y, led_width, rc, gc, bc, a);
		}
	}

	if (g_renderer)
		g_renderer->sync_osd_texture(monid, led_width, total_height);
}

#endif // AMIBERRY
