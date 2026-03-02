/*
 * gfx_window.cpp - Window/context management: creation, destruction, and lifecycle
 *
 * Extracted from amiberry_gfx.cpp for single-responsibility decomposition.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#include <algorithm>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <cstdio>
#include <cmath>
#include <iostream>

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "events.h"
#include "xwin.h"
#include "keyboard.h"
#include "drawing.h"
#include "statusline.h"
#include "picasso96.h"
#include "gui.h"
#include "amiberry_gfx.h"
#include "gfx_state.h"
#include "gfx_window.h"
#include "gl_overlays.h"
#include "gl_shader_dispatch.h"
#include "gfx_colors.h"
#include "sounddep/sound.h"
#include "inputdevice.h"

#include "threaddep/thread.h"
#include "vkbd/vkbd.h"
#include "on_screen_joystick.h"
#include "fsdb_host.h"
#include "uae/types.h"

#ifdef USE_OPENGL
#include "gl_platform.h"
#endif

#include "dpi_handler.hpp"
#include "registry.h"
#include "target.h"
#include "display_modes.h"

#ifdef AMIBERRY

#ifndef USE_OPENGL
extern SDL_Texture* p96_cursor_overlay_texture;
#endif

// Shared state defined in amiberry_gfx.cpp
extern int wasfs[2];
extern const TCHAR* wasfsname[2];
extern int window_led_drives, window_led_drives_end;
extern int window_led_hd, window_led_hd_end;
extern int window_led_joys, window_led_joys_end, window_led_joy_start;
extern int window_led_msg, window_led_msg_end, window_led_msg_start;

#include "gfx_platform_internal.h"

static int display_width;
static int display_height;

static void movecursor(int x, int y);
static void getextramonitorpos(const struct AmigaMonitor* mon, SDL_Rect* r);
static int create_windows(struct AmigaMonitor* mon);
static void allocsoftbuffer(int monid, const TCHAR* name, struct vidbuffer* buf, int flags, int width, int height);

static void gfxmode_reset(int monid)
{
}

void close_hwnds(struct AmigaMonitor* mon)
{
	if (mon->screen_is_initialized)
		releasecapture(mon);
	mon->screen_is_initialized = 0;
	if (!mon->monitor_id) {
#ifdef AVIOUTPUT
		AVIOutput_Restart(true);
#endif
#ifdef RETROPLATFORM
		rp_set_hwnd(NULL);
#endif
	}
	if (mon->monitor_id > 0 && mon->amiga_window)
		setmouseactive(mon->monitor_id, 0);

#ifdef USE_OPENGL
	destroy_shaders();
	if (gl_context != nullptr)
	{
		if (g_overlay.cursor_texture) {
			glDeleteTextures(1, &g_overlay.cursor_texture);
			g_overlay.cursor_texture = 0;
		}
		if (g_overlay.cursor_vao) {
			glDeleteVertexArrays(1, &g_overlay.cursor_vao);
			g_overlay.cursor_vao = 0;
		}
		if (g_overlay.cursor_vbo) {
			glDeleteBuffers(1, &g_overlay.cursor_vbo);
			g_overlay.cursor_vbo = 0;
		}
	}
#else
	if (amiga_texture)
	{
		SDL_DestroyTexture(amiga_texture);
		amiga_texture = nullptr;
	}
	if (p96_cursor_overlay_texture)
	{
		SDL_DestroyTexture(p96_cursor_overlay_texture);
		p96_cursor_overlay_texture = nullptr;
	}
#endif

#ifdef USE_OPENGL
	if (gl_context != nullptr)
	{
		SDL_GL_DeleteContext(gl_context);
		gl_context = nullptr;
		g_vsync.current_interval = -1; // Reset VSync state
		g_vsync.cached_refresh_rate = 0.0f;
		g_vsync.gl_initialized = false; // Reset GL state flag for next context
	}
#else
	if (mon->amiga_renderer && !kmsdrm_detected)
	{
#if defined(__ANDROID__)
		// Don't destroy the renderer on Android, as we reuse it
#elif defined(_WIN32)
		if (mon->gui_renderer == mon->amiga_renderer) {
			// GUI is sharing this renderer — don't destroy
		} else {
			SDL_DestroyRenderer(mon->amiga_renderer);
			mon->amiga_renderer = nullptr;
		}
#else
		SDL_DestroyRenderer(mon->amiga_renderer);
		mon->amiga_renderer = nullptr;
#endif
	}
#endif
	if (mon->amiga_window && !kmsdrm_detected)
	{
#if defined(__ANDROID__)
		// Reuse existing window
#elif defined(_WIN32)
		if (mon->gui_window == mon->amiga_window) {
			// GUI is sharing this window — don't destroy
		} else {
			SDL_DestroyWindow(mon->amiga_window);
			mon->amiga_window = nullptr;
		}
#else
		SDL_DestroyWindow(mon->amiga_window);
		mon->amiga_window = nullptr;
#endif
	}

	if (currprefs.vkbd_enabled)
		vkbd_quit();

	gfx_hdr = false;
}

void updatemodes(struct AmigaMonitor* mon)
{
	Uint32 flags = 0;

	mon->currentmode.fullfill = 0;
	if (isfullscreen() > 0)
		flags |= SDL_WINDOW_FULLSCREEN;
	else if (isfullscreen() < 0)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	mon->currentmode.flags = flags;
	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		const SDL_Rect rc = getdisplay(&currprefs, mon->monitor_id)->rect;
		mon->currentmode.native_width = rc.w;
		mon->currentmode.native_height = rc.h;
		mon->currentmode.current_width = mon->currentmode.native_width;
		mon->currentmode.current_height = mon->currentmode.native_height;
	} else {
		mon->currentmode.native_width = mon->currentmode.current_width;
		mon->currentmode.native_height = mon->currentmode.current_height;
	}
}

void update_gfxparams(struct AmigaMonitor* mon)
{
	const struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];

	updatewinfsmode(mon->monitor_id, &currprefs);
#ifdef PICASSO96
	if (mon->screen_is_picasso) {
		float mx = 1.0;
		float my = 1.0;
		if (currprefs.gf[GF_RTG].gfx_filter_horiz_zoom_mult > 0) {
			mx *= currprefs.gf[GF_RTG].gfx_filter_horiz_zoom_mult;
		}
		if (currprefs.gf[GF_RTG].gfx_filter_vert_zoom_mult > 0) {
			my *= currprefs.gf[GF_RTG].gfx_filter_vert_zoom_mult;
		}
		mon->currentmode.current_width = static_cast<int>(state->Width * currprefs.rtg_horiz_zoom_mult * mx);
		mon->currentmode.current_height = static_cast<int>(state->Height * currprefs.rtg_vert_zoom_mult * my);
		currprefs.gfx_apmode[APMODE_RTG].gfx_interlaced = false;
		if (currprefs.rtgvblankrate == 0) {
			currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate = currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate;
			if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced) {
				currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate *= 2;
			}
		} else if (currprefs.rtgvblankrate < 0) {
			currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate = 0;
		} else {
			currprefs.gfx_apmode[APMODE_RTG].gfx_refreshrate = currprefs.rtgvblankrate;
		}
	} else {
#endif
		mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
		mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
#ifdef PICASSO96
	}
#endif
	mon->currentmode.amiga_width = mon->currentmode.current_width;
	mon->currentmode.amiga_height = mon->currentmode.current_height;

	mon->scalepicasso = 0;
	if (mon->screen_is_picasso) {
		bool diff = state->Width != mon->currentmode.native_width || state->Height != mon->currentmode.native_height;
		if (isfullscreen() < 0) {
			if ((currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER || currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_SCALE || currprefs.rtgallowscaling) && diff) {
				mon->scalepicasso = RTG_MODE_SCALE;
			}
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE && diff) {
				mon->scalepicasso = RTG_MODE_INTEGER_SCALE;
			}
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER && diff) {
				mon->scalepicasso = currprefs.gf[GF_RTG].gfx_filter_autoscale;
			}
			if (!mon->scalepicasso && currprefs.rtgscaleaspectratio) {
				mon->scalepicasso = -1;
			}
		} else if (isfullscreen() > 0) {
			if (mon->currentmode.native_width > state->Width && mon->currentmode.native_height > state->Height) {
				if (currprefs.gf[GF_RTG].gfx_filter_autoscale)
					mon->scalepicasso = RTG_MODE_SCALE;
				if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE) {
					mon->scalepicasso = RTG_MODE_INTEGER_SCALE;
				}
			}
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER)
				mon->scalepicasso = currprefs.gf[GF_RTG].gfx_filter_autoscale;
			if (!mon->scalepicasso && currprefs.rtgscaleaspectratio)
				mon->scalepicasso = -1;
		} else if (isfullscreen() == 0) {
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_INTEGER_SCALE) {
				mon->scalepicasso = RTG_MODE_INTEGER_SCALE;
				mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
				mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
			} else if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_CENTER) {
				if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width < state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height < state->Height) {
					if (!currprefs.rtgallowscaling) {
						;
					} else if (currprefs.rtgscaleaspectratio) {
						mon->scalepicasso = -1;
						mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
						mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
					}
				} else {
					mon->scalepicasso = RTG_MODE_CENTER;
					mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
					mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
				}
			} else if (currprefs.gf[GF_RTG].gfx_filter_autoscale == RTG_MODE_SCALE) {
				if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width > state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height > state->Height)
					mon->scalepicasso = RTG_MODE_SCALE;
				if ((currprefs.gfx_monitor[mon->monitor_id].gfx_size.width != state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height != state->Height) && currprefs.rtgallowscaling) {
					mon->scalepicasso = RTG_MODE_SCALE;
				} else if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width < state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height < state->Height) {
					// no always scaling and smaller? Back to normal size and set new configured max size
					mon->currentmode.current_width = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.width = state->Width;
					mon->currentmode.current_height = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.height = state->Height;
				} else if (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width == state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height == state->Height) {
					;
				} else if (!mon->scalepicasso && currprefs.rtgscaleaspectratio) {
					mon->scalepicasso = -1;
				}
			} else {
				if ((currprefs.gfx_monitor[mon->monitor_id].gfx_size.width != state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height != state->Height) && currprefs.rtgallowscaling)
					mon->scalepicasso = RTG_MODE_SCALE;
				if (!mon->scalepicasso && currprefs.rtgscaleaspectratio)
					mon->scalepicasso = -1;
			}
		}

		if (mon->scalepicasso > 0 && (currprefs.gfx_monitor[mon->monitor_id].gfx_size.width != state->Width || currprefs.gfx_monitor[mon->monitor_id].gfx_size.height != state->Height)) {
			mon->currentmode.current_width = currprefs.gfx_monitor[mon->monitor_id].gfx_size.width;
			mon->currentmode.current_height = currprefs.gfx_monitor[mon->monitor_id].gfx_size.height;
		}
	}

}

int open_windows(AmigaMonitor* mon, bool mousecapture, bool started)
{
	// Skip window creation entirely if headless mode
	if (currprefs.headless) {
		write_log("Headless mode: Skipping window creation for monitor %d.\n", mon->monitor_id);
		mon->screen_is_initialized = 1;
		mon->amiga_window = nullptr;
		mon->amiga_renderer = nullptr;
		return 1;
	}

	bool recapture = false;
	int ret;

	mon->screen_is_initialized = 0;

	if (mon->monitor_id && mouseactive)
		recapture = true;

	inputdevice_unacquire();
	reset_sound();
	if (mon->amiga_window == nullptr)
		wait_keyrelease();

	mon->in_sizemove = 0;

	updatewinfsmode(mon->monitor_id, &currprefs);

	int init_round = 0;
	ret = -2;
	do {
		if (ret < -1) {
			updatemodes(mon);
			update_gfxparams(mon);
		}
		ret = doInit(mon);
		init_round++;
		if (ret < -9) {
			return 0;
		}
	} while (ret < 0);

	if (!ret) {
		return ret;
	}
	if (gfx_platform_skip_window_activation())
		return ret;

	bool startactive = (started && mouseactive) || (!started && !currprefs.start_uncaptured && !currprefs.start_minimized);
	bool startpaused = !started && ((currprefs.start_minimized && currprefs.minimized_pause) || (currprefs.start_uncaptured && currprefs.inactive_pause && isfullscreen() <= 0));
	bool startminimized = !started && currprefs.start_minimized && isfullscreen() <= 0;
	int input = 0;

	if ((mousecapture && startactive) || recapture)
		setmouseactive(mon->monitor_id, -1);

	int upd = 0;
	if (startactive) {
		setpriority(currprefs.active_capture_priority);
		upd = 2;
	} else if (startminimized) {
		setpriority(currprefs.minimized_priority);
		setminimized(mon->monitor_id);
		input = currprefs.inactive_input;
		upd = 1;
	} else {
		setpriority(currprefs.inactive_priority);
		input = currprefs.inactive_input;
		upd = 2;
	}
	if (upd > 1) {
		for (int i = 0; i < NUM_LEDS; i++)
			gui_flicker_led(i, -1, -1);
		gui_led(LED_POWER, gui_data.powerled, gui_data.powerled_brightness);
		gui_fps(0, 0, 0, 0, 0);
		if (gui_data.md >= 0)
			gui_led(LED_MD, 0, -1);
		for (int i = 0; i < 4; i++) {
			if (currprefs.floppyslots[i].dfxtype >= 0)
				gui_led(LED_DF0 + i, 0, -1);
		}
	}
	if (upd > 0) {
		inputdevice_acquire(TRUE);
		if (!isfocus())
			inputdevice_unacquire(input);
	}

	if (startpaused)
		setpaused(1);

	statusline_updated(mon->monitor_id);
	refreshtitle();

	return ret;
}

void reopen_gfx(struct AmigaMonitor* mon)
{
	open_windows(mon, false, true);
	render_screen(mon->monitor_id, 1, true);
}

void open_screen(struct AmigaMonitor* mon)
{
	close_windows(mon);
	open_windows(mon, true, true);
}

int reopen(struct AmigaMonitor* mon, int full, bool unacquire)
{
	const struct amigadisplay* ad = &adisplays[mon->monitor_id];
	int quick = 0;
	int idx = mon->screen_is_picasso ? 1 : 0;
	struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];

	updatewinfsmode(mon->monitor_id, &changed_prefs);

	if (changed_prefs.gfx_apmode[0].gfx_fullscreen != currprefs.gfx_apmode[0].gfx_fullscreen && !mon->screen_is_picasso)
		full = 1;
	if (changed_prefs.gfx_apmode[1].gfx_fullscreen != currprefs.gfx_apmode[1].gfx_fullscreen && mon->screen_is_picasso)
		full = 1;

	/* fullscreen to fullscreen */
	if (isfullscreen() > 0 && currprefs.gfx_apmode[0].gfx_fullscreen == changed_prefs.gfx_apmode[0].gfx_fullscreen &&
		currprefs.gfx_apmode[1].gfx_fullscreen == changed_prefs.gfx_apmode[1].gfx_fullscreen && currprefs.gfx_apmode[0].gfx_fullscreen == GFX_FULLSCREEN) {
		quick = 1;
	}
	/* windowed to windowed */
	if (isfullscreen() <= 0 && currprefs.gfx_apmode[0].gfx_fullscreen == changed_prefs.gfx_apmode[0].gfx_fullscreen &&
		currprefs.gfx_apmode[1].gfx_fullscreen == changed_prefs.gfx_apmode[1].gfx_fullscreen) {
		quick = 1;
	}

	currprefs.gfx_monitor[mon->monitor_id].gfx_size_fs.width = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.width;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.width = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.width;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.height = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.height;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.x = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.x;
	currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.y = changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_win.y;

	currprefs.gfx_apmode[0].gfx_fullscreen = changed_prefs.gfx_apmode[0].gfx_fullscreen;
	currprefs.gfx_apmode[1].gfx_fullscreen = changed_prefs.gfx_apmode[1].gfx_fullscreen;
	currprefs.gfx_apmode[0].gfx_vsync = changed_prefs.gfx_apmode[0].gfx_vsync;
	currprefs.gfx_apmode[1].gfx_vsync = changed_prefs.gfx_apmode[1].gfx_vsync;
	currprefs.gfx_apmode[0].gfx_vsyncmode = changed_prefs.gfx_apmode[0].gfx_vsyncmode;
	currprefs.gfx_apmode[1].gfx_vsyncmode = changed_prefs.gfx_apmode[1].gfx_vsyncmode;
	currprefs.gfx_apmode[0].gfx_refreshrate = changed_prefs.gfx_apmode[0].gfx_refreshrate;

	currprefs.rtg_horiz_zoom_mult = changed_prefs.rtg_horiz_zoom_mult;
	currprefs.rtg_vert_zoom_mult = changed_prefs.rtg_vert_zoom_mult;

	set_config_changed();

	if (!quick)
		return 1;

	if (unacquire) {
		inputdevice_unacquire();
	}

	reopen_gfx(mon);

	return 0;
}

void close_windows(struct AmigaMonitor* mon)
{
	// Skip SDL resource cleanup if headless mode
	if (currprefs.headless) {
		write_log("Headless mode: Skipping SDL resource cleanup for monitor %d.\n", mon->monitor_id);
		if (amiga_surface) {
			SDL_FreeSurface(amiga_surface);
			amiga_surface = nullptr;
		}
		return;
	}

	vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;

	reset_sound();

#ifdef AMIBERRY
	SDL_FreeSurface(amiga_surface);
	amiga_surface = nullptr;
#endif
	if (mon->statusline_surface) {
		SDL_FreeSurface(mon->statusline_surface);
		mon->statusline_surface = nullptr;
	}
	if (mon->statusline_texture) {
		SDL_DestroyTexture(mon->statusline_texture);
		mon->statusline_texture = nullptr;
	}

	freevidbuffer(mon->monitor_id, &avidinfo->drawbuffer);
	freevidbuffer(mon->monitor_id, &avidinfo->tempbuffer);
	close_hwnds(mon);
}

static void movecursor(const int x, const int y)
{
	const AmigaMonitor* mon = &AMonitors[0];
	if (mon->amiga_window) {
		SDL_WarpMouseInWindow(mon->amiga_window, x, y);
	}
}

static void getextramonitorpos(const struct AmigaMonitor* mon, SDL_Rect* r)
{
	TCHAR buf[100];
	SDL_Rect r1, r2;
	int x, y;
	bool got = true;

	_sntprintf(buf, sizeof buf, _T("MainPosX_%d"), mon->monitor_id);
	if (!regqueryint(nullptr, buf, &x)) {
		got = false;
	}
	_sntprintf(buf, sizeof buf, _T("MainPosY_%d"), mon->monitor_id);
	if (!regqueryint(nullptr, buf, &y)) {
		got = false;
	}
	if (got) {
		SDL_Point pt;
		pt.x = x;
		pt.y = y;
		if (!MonitorFromPoint(pt)) {
			got = false;
		}
	}
	// find rightmost window edge
	int monid = MAX_AMIGAMONITORS - 1;
	int rightmon = -1;
	int rightedge = 0;
	SDL_Window* hwnd = NULL;
	while (monid >= 1) {
		monid--;
		hwnd = AMonitors[monid].amiga_window;
		if (!hwnd)
			continue;
		GetWindowRect(hwnd, &r1);
		if (r1.w > rightedge) {
			rightedge = r1.w;
			rightmon = monid;
		}
	}
	if (rightmon < 0 && !got)
		return;
	hwnd = AMonitors[rightmon].amiga_window;
	GetWindowRect(hwnd, &r1);
	r2 = r1;

	//getextendedframebounds(hwnd, &r2);
	const int width = r->w;
	const int height = r->h;

	if (got) {
		r->x = x;
		r->y = y;
	} else {
		r->x = r1.w - ((r2.x - r1.x) + (r1.w - r2.w));
		r->y = r1.y;
	}
	r->h = height;
	r->w = width;
}

static int create_windows(struct AmigaMonitor* mon)
{
	// Skip window creation entirely if headless mode
	if (currprefs.headless) {
		write_log("Headless mode: Skipping window creation for monitor %d.\n", mon->monitor_id);
		mon->amiga_window = nullptr;
		mon->amiga_renderer = nullptr;
		mon->screen_is_initialized = 1;
		return 1;
	}

	const Uint32 fullscreen = mon->currentmode.flags & SDL_WINDOW_FULLSCREEN;
	Uint32 fullwindow = mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
	Uint32 flags = 0;
	const int borderless = currprefs.borderless;
	int x, y, w, h;
	struct MultiDisplay* md;

#ifdef AMIBERRY
	// Detect KMSDRM driver
	write_log("Getting Current Video Driver...\n");
	sdl_video_driver = SDL_GetCurrentVideoDriver();
	if (sdl_video_driver != nullptr && strcmpi(sdl_video_driver, "KMSDRM") == 0)
	{
		kmsdrm_detected = true;
		if (!mon->amiga_window && mon->gui_window)
		{
			mon->amiga_window = mon->gui_window;
		}
		if (!mon->amiga_renderer && mon->gui_renderer)
		{
			mon->amiga_renderer = mon->gui_renderer;
		}
	}
#ifdef __ANDROID__
	if (!mon->amiga_window && mon->gui_window)
	{
		mon->amiga_window = mon->gui_window;
	}
	if (!mon->amiga_renderer && mon->gui_renderer)
	{
		mon->amiga_renderer = mon->gui_renderer;
	}
#endif
	// If KMSDRM is detected, force Full-Window mode
	if (kmsdrm_detected)
	{
		currprefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen = GFX_FULLWINDOW;
		currprefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen = GFX_FULLWINDOW;
		fullwindow = 1;
	}
#endif

	md = getdisplay(&currprefs, mon->monitor_id);
	if (mon->monitor_id && fullwindow) {
		struct MultiDisplay* md2 = nullptr;
		int idx = 0;
		for (;;) {
			md2 = getdisplay2(&currprefs, idx);
			if (md2 == md)
				break;
			if (!md2)
				break;
			idx++;
		}
		for (int i = 0; i <= mon->monitor_id; i++) {
			md2 = getdisplay2(&currprefs, idx);
			if (!md2)
				idx = 0;
			else
				idx++;
		}
		if (md2)
			md = md2;
	}
	mon->md = md;

	if (mon->amiga_window) {
		SDL_Rect r;
		int nw, nh, nx, ny;

		if (minimized) {
			minimized = -1;
			return 1;
		}

		GetWindowRect(mon->amiga_window, &r);

		x = r.x;
		y = r.y;
		w = r.w;
		h = r.h;

		// Default new position to current position; fullwindow overrides below
		nx = x;
		ny = y;

		if (mon->screen_is_picasso) {
			nw = mon->currentmode.current_width;
			nh = mon->currentmode.current_height;
		} else {
			nw = currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.width;
			nh = currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.height;
		}

		if (!fullscreen && !fullwindow) {
			nw = DPIHandler::scale_window_dimension(nw);
			nh = DPIHandler::scale_window_dimension(nh);
		}

		if (fullwindow) {
			SDL_Rect rc = md->rect;
			nx = rc.x;
			ny = rc.y;
			nw = rc.w;
			nh = rc.h;
		}

		if (w != nw || h != nh || x != nx || y != ny) {
			w = nw;
			h = nh;
			x = nx;
			y = ny;
			mon->in_sizemove++;
			SDL_SetWindowPosition(mon->amiga_window, x, y);
			SDL_SetWindowSize(mon->amiga_window, w, h);
		} else {
			w = nw;
			h = nh;
			x = nx;
			y = ny;
		}

		updatewinrect(mon, false);
		write_log(_T("window already open (%dx%d %dx%d)\n"),
			mon->amigawin_rect.x, mon->amigawin_rect.y, mon->amigawin_rect.w, mon->amigawin_rect.h);
		updatemouseclip(mon);
#ifdef RETROPLATFORM
		rp_screenmode_changed();
#endif
		mon->window_extra_height_bar = 0;
		return 1;
	}

	window_led_drives = 0;
	window_led_drives_end = 0;
	x = 0; y = 0;

	SDL_Rect rc;
	int stored_x = 1, stored_y = 0;
	int oldx, oldy;

	regqueryint(nullptr, _T("MainPosX"), &stored_x);
	regqueryint(nullptr, _T("MainPosY"), &stored_y);

	if (borderless) {
		stored_x = currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.x;
		stored_y = currprefs.gfx_monitor[mon->monitor_id].gfx_size_win.y;
	}

	stored_x = std::max(stored_x, 0);
	stored_y = std::max(stored_y, 0);

	SDL_Rect displayBounds;
	SDL_GetDisplayBounds(0, &displayBounds);

	if (stored_x > displayBounds.w)
		rc.x = 1;
	else
		rc.x = stored_x;

	if (stored_y > displayBounds.h)
		rc.y = 1;
	else
		rc.y = stored_y;

	rc.w = mon->currentmode.current_width;
	rc.h = mon->currentmode.current_height;

	if (!fullscreen && !fullwindow) {
		rc.w = DPIHandler::scale_window_dimension(rc.w);
		rc.h = DPIHandler::scale_window_dimension(rc.h);
	}

	oldx = rc.x;
	oldy = rc.y;

	mon->win_x_diff = rc.x - oldx;
	mon->win_y_diff = rc.y - oldy;

	if (mon->monitor_id > 0) {
		getextramonitorpos(mon, &rc);
	}

	if (fullwindow) {
		rc = md->rect;
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef __ANDROID__
		flags |= SDL_WINDOW_RESIZABLE;
#endif
		mon->currentmode.native_width = rc.w;
		mon->currentmode.native_height = rc.h;
	} else if (fullscreen) {
		flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef __ANDROID__
		flags |= SDL_WINDOW_RESIZABLE;
#endif
		getbestmode(mon, 0);
		w = mon->currentmode.native_width;
		h = mon->currentmode.native_height;
		rc = md->rect;
		if (rc.x >= 0)
			x = rc.x;
		else
			x = rc.x + (rc.w - w);
		if (rc.y >= 0)
			y = rc.y;
		else
			y = rc.y + (rc.h - h);
	} else {
		flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	}
	if (currprefs.main_alwaysontop) {
		flags |= SDL_WINDOW_ALWAYS_ON_TOP;
	}
	if (borderless)
		flags |= SDL_WINDOW_BORDERLESS;
	if (currprefs.start_minimized || currprefs.headless)
		flags |= SDL_WINDOW_HIDDEN;
#ifdef USE_OPENGL
	flags |= SDL_WINDOW_OPENGL;
#endif
	mon->amiga_window = SDL_CreateWindow(_T("Amiberry"),
		rc.x, rc.y,
		rc.w, rc.h,
		flags);
	if (!mon->amiga_window) {
		write_log(_T("main window creation failed\n"));
		write_log(SDL_GetError());
		write_log("\n");
		return 0;
	}

	SDL_Rect rc2;
	GetWindowRect(mon->amiga_window, &rc2);
	int expected_client_width = mon->currentmode.current_width;
	int expected_client_height = mon->currentmode.current_height;
	if (!fullscreen && !fullwindow) {
		expected_client_width = DPIHandler::scale_window_dimension(expected_client_width);
		expected_client_height = DPIHandler::scale_window_dimension(expected_client_height);
	}
	mon->window_extra_width = rc2.w - expected_client_width;
	mon->window_extra_height = rc2.h - expected_client_height;

	w = mon->currentmode.native_width;
	h = mon->currentmode.native_height;

	if (!mon->amiga_window) {
		write_log(_T("creation of amiga window failed\n"));
		close_hwnds(mon);
		return 0;
	}

	gfx_platform_set_window_icon(mon->amiga_window);

#ifndef USE_OPENGL
	if (mon->amiga_renderer == nullptr)
	{
		Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
		const auto* ad = &adisplays[mon->monitor_id];
		const auto* ap = ad->picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];

		mon->amiga_renderer = SDL_CreateRenderer(mon->amiga_window, -1, renderer_flags);
		check_error_sdl(mon->amiga_renderer == nullptr, "Unable to create a renderer:");
	}
	DPIHandler::set_render_scale(mon->amiga_renderer);
#endif

    // Cache current display mode for scaling heuristics
    if (SDL_GetWindowDisplayMode(mon->amiga_window, &sdl_mode) != 0) {
        // Fallback to desktop mode if window query fails
        SDL_GetDesktopDisplayMode(0, &sdl_mode);
    }
	updatewinrect(mon, true);
	GetWindowRect(mon->amiga_window, &mon->mainwin_rect);
	if (fullscreen || fullwindow)
		movecursor(x + w / 2, y + h / 2);

	mon->window_extra_height_bar = 0;
	//mon->dpi = getdpiforwindow(mon->monitor_id);

	if (SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1") != SDL_TRUE)
		write_log("SDL2: could not grab the keyboard!\n");

	if (SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0") == SDL_TRUE)
		write_log("SDL2: Set window not to minimize on focus loss\n");

	return 1;
}

static void allocsoftbuffer(int monid, const TCHAR* name, struct vidbuffer* buf, int flags, int width, int height)
{
	struct vidbuf_description *vidinfo = &adisplays[monid].gfxvidinfo;
	int depth = 32;

	if (buf->initialized && buf->vram_buffer) {
		return;
	}

	buf->monitor_id = monid;
	buf->pixbytes = (depth + 7) / 8;
	buf->width_allocated = (width + 7) & ~7;
	buf->height_allocated = height;
	buf->initialized = true;
	buf->hardwiredpositioning = false;

	if (buf == &vidinfo->drawbuffer) {

		buf->bufmem = NULL;
		buf->bufmemend = NULL;
		buf->realbufmem = NULL;
		buf->bufmem_allocated = NULL;
		buf->vram_buffer = true;

	} else {

		xfree(buf->realbufmem);
		const int w = buf->width_allocated;
		const int h = buf->height_allocated;
		const int size = (w * 2) * (h * 2) * buf->pixbytes;
		buf->rowbytes = w * 2 * buf->pixbytes;
		buf->realbufmem = xcalloc(uae_u8, size);
		buf->bufmem_allocated = buf->bufmem = buf->realbufmem + (h / 2) * buf->rowbytes + (w / 2) * buf->pixbytes;
		buf->bufmemend = buf->realbufmem + size - buf->rowbytes;

	}
}

int oldtex_w[MAX_AMIGAMONITORS], oldtex_h[MAX_AMIGAMONITORS], oldtex_rtg[MAX_AMIGAMONITORS];

// updatepicasso96 remains in amiberry_gfx.cpp; declared here for doInit to call
extern void updatepicasso96(struct AmigaMonitor* mon);

bool doInit(AmigaMonitor* mon)
{
	bool modechanged;

	struct vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;
	struct amigadisplay* ad = &adisplays[mon->monitor_id];
	avidinfo->gfx_resolution_reserved = RES_MAX;
	avidinfo->gfx_vresolution_reserved = VRES_MAX;
	if (gfx_platform_do_init(mon))
		return true;

	// If headless mode, skip all window/renderer setup
	if (currprefs.headless) {
		write_log("Headless mode: Skipping doInit window setup for monitor %d.\n", mon->monitor_id);

		// Still need to set up display dimensions for emulation
		display_width = 1920;
		display_height = 1080;

		if (!mon->screen_is_picasso) {

			allocsoftbuffer(mon->monitor_id, _T("draw"), &avidinfo->drawbuffer, mon->currentmode.flags,
				1920, 1280);

			allocsoftbuffer(mon->monitor_id, _T("monemu"), &avidinfo->tempbuffer, mon->currentmode.flags,
				mon->currentmode.amiga_width > 2048 ? mon->currentmode.amiga_width : 2048,
				mon->currentmode.amiga_height > 2048 ? mon->currentmode.amiga_height : 2048);
		}

		avidinfo->outbuffer = &avidinfo->drawbuffer;
		avidinfo->inbuffer = &avidinfo->tempbuffer;

		// Create internal surface if needed
		if (amiga_surface == nullptr) {
			amiga_surface = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, 32, pixel_format);
		}

		mon->screen_is_initialized = 1;
		init_colors(mon->monitor_id);
		target_graphics_buffer_update(mon->monitor_id, false);
		return true;
	}

	modechanged = true;
	if (wasfs[0] == 0)
		regqueryint(NULL, wasfsname[0], &wasfs[0]);
	if (wasfs[1] == 0)
		regqueryint(NULL, wasfsname[1], &wasfs[1]);

	gfxmode_reset(mon->monitor_id);

	for (;;) {
		updatemodes(mon);

		if (mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
			const SDL_Rect rc = getdisplay(&currprefs, mon->monitor_id)->rect;
			mon->currentmode.native_width = rc.w;
			mon->currentmode.native_height = rc.h;
		}

#ifdef USE_OPENGL
		int gl_attempts = 0;
		bool gl_success = false;

		while (gl_attempts < 2 && !gl_success) {
			if (!set_opengl_attributes(gl_attempts))
			{
				write_log("Failed to set OpenGL attributes for mode %d\n", gl_attempts);
				gl_attempts++;
				continue;
			}

			if (!create_windows(mon))
			{
				close_hwnds(mon);
				return false;
			}

			// Destroy existing shaders and GL context before creating a new one.
			// When the window is reused (e.g. auto-resolution change), create_windows
			// returns without calling close_hwnds, so the old GL context and shaders
			// survive. The shaders hold GL resources tied to the old context, so they
			// must be destroyed while the old context is still current. Without this,
			// init_opengl_context creates a new context, leaks the old one, and leaves
			// stale shader GL objects that produce a black screen after a few frames.
			destroy_shaders();
			if (gl_context != nullptr) {
				SDL_GL_DeleteContext(gl_context);
				gl_context = nullptr;
			}

			if (init_opengl_context(mon->amiga_window))
			{
				gl_success = true;
			}
			else
			{
				write_log("OpenGL context init failed for mode %d. Retrying...\n", gl_attempts);
				// Close window to force recreation with new attributes
				close_windows(mon);
				gl_attempts++;
			}
		}

		if (!gl_success) {
			write_log("All OpenGL context attempts failed. Aborting doInit.\n");
			return false;
		}
#else
		if (!create_windows(mon))
		{
			close_hwnds(mon);
			return false;
		}
#endif
#ifdef PICASSO96
		if (mon->screen_is_picasso) {
			display_width = picasso96_state[0].Width ? picasso96_state[0].Width : 640;
			display_height = picasso96_state[0].Height ? picasso96_state[0].Height : 480;
			break;
		} else {
#endif
			avidinfo->gfx_resolution_reserved = std::max(currprefs.gfx_resolution, avidinfo->gfx_resolution_reserved);
			avidinfo->gfx_vresolution_reserved = std::max(currprefs.gfx_vresolution, avidinfo->gfx_vresolution_reserved);

			if (!currprefs.gfx_autoresolution) {
				mon->currentmode.amiga_width = AMIGA_WIDTH_MAX << currprefs.gfx_resolution;
				mon->currentmode.amiga_height = AMIGA_HEIGHT_MAX << currprefs.gfx_vresolution;
			} else {
				mon->currentmode.amiga_width = AMIGA_WIDTH_MAX << avidinfo->gfx_resolution_reserved;
				mon->currentmode.amiga_height = AMIGA_HEIGHT_MAX << avidinfo->gfx_vresolution_reserved;
			}
			if (avidinfo->gfx_resolution_reserved == RES_SUPERHIRES)
				mon->currentmode.amiga_height *= 2;
			mon->currentmode.amiga_height = std::min(mon->currentmode.amiga_height, 1280);

			display_width = mon->currentmode.amiga_width;
			display_height = mon->currentmode.amiga_height;
			break;
#ifdef PICASSO96
		}
#endif
	}

	updatepicasso96(mon);

	if (!mon->screen_is_picasso) {

		allocsoftbuffer(mon->monitor_id, _T("draw"), &avidinfo->drawbuffer, mon->currentmode.flags,
			1920, 1280);

		allocsoftbuffer(mon->monitor_id, _T("monemu"), &avidinfo->tempbuffer, mon->currentmode.flags,
			mon->currentmode.amiga_width > 2048 ? mon->currentmode.amiga_width : 2048,
			mon->currentmode.amiga_height > 2048 ? mon->currentmode.amiga_height : 2048);
	}

	avidinfo->outbuffer = &avidinfo->drawbuffer;
	avidinfo->inbuffer = &avidinfo->tempbuffer;

	if (amiga_surface)
	{
		SDL_FreeSurface(amiga_surface);
		amiga_surface = nullptr;
	}
	amiga_surface = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, 32, pixel_format);

	updatewinrect(mon, true);
	mon->screen_is_initialized = 1;

	if (modechanged) {
		init_colors(mon->monitor_id);
		display_param_init(mon);
		//createstatusline(mon->hAmigaWnd, mon->monitor_id);
	}
	target_graphics_buffer_update(mon->monitor_id, false);

	picasso_refresh(mon->monitor_id);
#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif

	if (isfullscreen() != 0) {
		setmouseactive(mon->monitor_id, -1);
	}

	//osk_setup(mon->monitor_id, -2);
	if (vkbd_allowed(mon->monitor_id))
	{
		vkbd_set_transparency(static_cast<double>(currprefs.vkbd_transparency) / 100.0);
		vkbd_set_hires(currprefs.vkbd_hires);
		vkbd_set_keyboard_has_exit_button(currprefs.vkbd_exit);
		vkbd_set_language(string(currprefs.vkbd_language));
		vkbd_set_style(string(currprefs.vkbd_style));
		vkbd_init();
	}

	// Initialize on-screen joystick if enabled
	if (currprefs.onscreen_joystick)
	{
		on_screen_joystick_init(mon->amiga_renderer);
		int sw = 0, sh = 0;
#ifdef USE_OPENGL
		SDL_GL_GetDrawableSize(mon->amiga_window, &sw, &sh);
#else
		SDL_GetRendererOutputSize(mon->amiga_renderer, &sw, &sh);
#endif
		on_screen_joystick_update_layout(sw, sh, render_quad);
		on_screen_joystick_set_enabled(true);
	}

	return true;
}

[[nodiscard]] bool set_opengl_attributes(int mode)
{
	bool success = true;

	// Reset attributes to default before setting them (optional but good practice)
	SDL_GL_ResetAttributes();

	// Get SDL video driver to determine OpenGL vs GLES
	const char* drv = SDL_GetCurrentVideoDriver();
	write_log(_T("SDL video driver: %hs\n"), drv ? drv : "unknown");

	// Detect GLES-only drivers (e.g., KMSDRM on Raspberry Pi)
#ifdef __ANDROID__
	const bool likely_gles_only = true;
#else
	const bool likely_gles_only = (drv && (strcmp(drv, "KMSDRM") == 0));
#endif

	if (mode == 0) {
		if (likely_gles_only) {
			// GLES-only systems (e.g., Raspberry Pi with KMSDRM): Try GLES 3.0
			write_log(_T("Requesting OpenGL ES 3.0 context (GLES-only driver detected)...\n"));
			success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) == 0);
			success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) == 0);
			success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) == 0);
		} else {
			// Desktop OpenGL (x86, x86_64, ARM desktops, macOS): Try Core Profile 3.3
			write_log(_T("Requesting OpenGL 3.3 Core context...\n"));
			success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) == 0);
			success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) == 0);
			success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) == 0);
#ifdef __APPLE__
			// macOS requires the forward-compatible flag for OpenGL 3.2+ Core Profile
			success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG) == 0);
#endif
		}
	} else {
		// Fallback: Legacy OpenGL 2.1 Compatibility
		write_log(_T("Requesting OpenGL 2.1 Compatibility context...\n"));
		success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0) == 0);
		success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) == 0);
		success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) == 0);
		success &= (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) == 0);
	}

	// Sensible defaults.
	success &= (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) == 0);
	success &= (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16) == 0);
	success &= (SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0) == 0);

	// Optional: request RGBA8
	success &= (SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8) == 0);
	success &= (SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8) == 0);
	success &= (SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8) == 0);
	success &= (SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8) == 0);

	return success;
}

#ifdef USE_OPENGL

/**
 * @brief Creates the OpenGL context and initializes extension function pointers.
 *
 * This function must be called after an SDL window has been successfully
 * created. It performs the final steps required to prepare for OpenGL
 * rendering:
 * 1. Creates an OpenGL context and associates it with the given window.
 * 2. Binds the newly created context to the current thread.
 * 3. Loads OpenGL extension function pointers via SDL_GL_GetProcAddress.
 *
 * The function includes error checking after each step and will log detailed
 * error messages if any part of the process fails. It also logs the vendor,
 * renderer, and version strings for debugging purposes.
 *
 * @param window A pointer to the SDL_Window that the OpenGL context will be
 *               created for.
 * @return true if the context was created and extension functions were loaded
 *         successfully, false otherwise.
 */
[[nodiscard]] bool init_opengl_context(SDL_Window* window)
{
	write_log("DEBUG: Initializing OpenGL Context...\n");

	gl_context = SDL_GL_CreateContext(window);
	if (!gl_context) {
		write_log(_T("!!! SDL_GL_CreateContext failed: %hs\n"), SDL_GetError());
		return false;
	}

	if (SDL_GL_MakeCurrent(window, gl_context) != 0) {
		write_log(_T("!!! SDL_GL_MakeCurrent failed: %hs\n"), SDL_GetError());
		SDL_GL_DeleteContext(gl_context);
		gl_context = nullptr;
		return false;
	}

	// Load OpenGL extension functions (does nothing on Android/GLES3)
	if (!gl_platform_init()) {
		const GLubyte* ver = glGetString(GL_VERSION);
		if (!ver) {
			write_log(_T("!!! glGetString(GL_VERSION) is null; failing OpenGL init.\n"));
			SDL_GL_DeleteContext(gl_context);
			gl_context = nullptr;
			return false;
		}
		write_log(_T("!!! OpenGL version: %hs\n"), ver);
	}

	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* version  = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	const char* sl_ver   = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	write_log(_T("OpenGL Renderer: %hs\n"), renderer ? renderer : "unknown");
	write_log(_T("OpenGL Version:  %hs\n"), version ? version : "unknown");
	write_log(_T("GLSL Version:    %hs\n"), sl_ver ? sl_ver : "unknown");

	return true;
}

#endif

#endif // AMIBERRY
