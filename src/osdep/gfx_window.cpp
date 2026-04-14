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
#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "statusline.h"
#include "picasso96.h"
#include "gui.h"
#include "amiberry_gfx.h"
#include "gfx_window.h"
#include "gfx_colors.h"
#include "sounddep/sound.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#include "threaddep/thread.h"
#include "imgui_overlay.h"
#include "imgui_osk.h"
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
#include "irenderer.h"
#include "renderer_factory.h"
#ifdef USE_OPENGL
#include "opengl_renderer.h"
#endif
#ifdef USE_VULKAN
#include "vulkan_renderer.h"
#endif

#ifdef AMIBERRY

// Config name for window title
extern char last_active_config[];

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

static void movecursor(const AmigaMonitor* mon, int x, int y);
static void getextramonitorpos(const struct AmigaMonitor* mon, SDL_Rect* r);
static int create_windows(struct AmigaMonitor* mon);
static void allocsoftbuffer(int monid, const TCHAR* name, struct vidbuffer* buf, int flags, int width, int height);

static void gfxmode_reset(int monid)
{
}

void close_hwnds(struct AmigaMonitor* mon, const bool force_destroy_fullwindow)
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

	IRenderer* renderer_to_use = nullptr;
	if (mon->monitor_id > 0 && mon->renderer) {
		renderer_to_use = mon->renderer.get();
	} else if (mon->monitor_id == 0 && g_renderer) {
		renderer_to_use = g_renderer.get();
	}

	// Shut down the ImGui overlay BEFORE the renderer destroys its context/device.
	// The overlay's Vulkan backend needs the device alive for cleanup.
	imgui_osk_shutdown();
	imgui_overlay_shutdown();

	const bool preserve_fullwindow_window = isfullscreen() < 0 && !force_destroy_fullwindow;
	if (renderer_to_use) {
		renderer_to_use->close_hwnds_cleanup(mon);
		renderer_to_use->destroy_context();
		// no_wm_detected covers both KMSDRM and x11-without-WM: in both
		// cases the renderer is shared with the GUI and must not be torn
		// down here.
		if (!no_wm_detected && !preserve_fullwindow_window) {
			renderer_to_use->destroy_platform_renderer(mon);
		}
	}
	if (mon->monitor_id > 0 && mon->renderer) {
		mon->renderer.reset();
	}
	if (mon->amiga_window && !no_wm_detected && !preserve_fullwindow_window)
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

	gfx_hdr = false;
}

void updatemodes(struct AmigaMonitor* mon)
{
	SDL_WindowFlags flags = 0;

	mon->currentmode.fullfill = 0;
	if (isfullscreen() != 0)
		flags |= SDL_WINDOW_FULLSCREEN;

	mon->currentmode.flags = flags;
	if (isfullscreen() < 0) {
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
	// Only reset sound when creating a new window, not on quick reopen
	// (e.g. auto-resolution changes). close_windows() already resets sound
	// when doing a full teardown. Resetting here on quick reopen wipes the
	// audio buffer and causes audible dropouts during mode switches.
	if (mon->amiga_window == nullptr) {
		reset_sound();
		wait_keyrelease();
	}

	mon->in_sizemove = 0;
	mon->focus_transitioning = false;
	mon->pre_focus_x = 0;
	mon->pre_focus_y = 0;

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
	close_windows(mon, false);
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

void close_windows(struct AmigaMonitor* mon, const bool force_destroy_fullwindow)
{
	if (currprefs.headless) {
		write_log("Headless mode: Skipping SDL resource cleanup for monitor %d.\n", mon->monitor_id);
		if (amiga_surface) {
			SDL_DestroySurface(amiga_surface);
			amiga_surface = nullptr;
		}
		return;
	}

	vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;

	if (mon->monitor_id == 0)
		reset_sound();

#ifdef AMIBERRY
	if (mon->monitor_id == 0) {
		SDL_DestroySurface(amiga_surface);
		amiga_surface = nullptr;
	} else {
		if (mon->amiga_surface) {
			SDL_DestroySurface(mon->amiga_surface);
			mon->amiga_surface = nullptr;
		}
	}
#endif
	if (mon->statusline_surface) {
		SDL_DestroySurface(mon->statusline_surface);
		mon->statusline_surface = nullptr;
	}
	if (mon->statusline_texture) {
		SDL_DestroyTexture(mon->statusline_texture);
		mon->statusline_texture = nullptr;
	}

	freevidbuffer(mon->monitor_id, &avidinfo->drawbuffer);
	freevidbuffer(mon->monitor_id, &avidinfo->tempbuffer);
	close_hwnds(mon, force_destroy_fullwindow);
	mon->active = false;
}

void close_all_windows(const bool force_destroy_fullwindow)
{
	for (int i = MAX_AMIGAMONITORS - 1; i >= 0; i--) {
		if (AMonitors[i].active)
			close_windows(&AMonitors[i], force_destroy_fullwindow);
	}
}

static void movecursor(const AmigaMonitor* mon, const int x, const int y)
{
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
	if (currprefs.headless) {
		write_log("Headless mode: Skipping window creation for monitor %d.\n", mon->monitor_id);
		mon->amiga_window = nullptr;
		mon->amiga_renderer = nullptr;
		mon->screen_is_initialized = 1;
		return 1;
	}

	if (mon->monitor_id > 0) {
#ifdef __ANDROID__
		write_log("Android: Secondary monitors not supported, skipping monitor %d.\n", mon->monitor_id);
		return 0;
#endif
		if (kmsdrm_detected) {
			write_log("KMSDRM: Secondary monitors not supported, skipping monitor %d.\n", mon->monitor_id);
			return 0;
		}
	}

	const SDL_WindowFlags fullscreen = (isfullscreen() > 0) ? SDL_WINDOW_FULLSCREEN : 0;
	uint32_t fullwindow = (isfullscreen() < 0) ? 1 : 0;
	SDL_WindowFlags flags = 0;
	const int borderless = currprefs.borderless;
	int x, y, w, h;
	struct MultiDisplay* md;

#ifdef AMIBERRY
	write_log("Getting Current Video Driver...\n");
	sdl_video_driver = SDL_GetCurrentVideoDriver();
	// Detect WM-less environments (KMSDRM, or x11 without a WM like Batocera).
	// Both require window sharing: without a WM, separate windows can't be
	// focus/stacking-managed, leaving the GUI stuck foreground on resume
	// (see #1962). detect_no_wm() also sets kmsdrm_detected when applicable.
	detect_no_wm();
	if (no_wm_detected)
	{
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

		if (fullwindow) {
			SDL_Rect rc = md->rect;
			nx = rc.x;
			ny = rc.y;
			nw = rc.w;
			nh = rc.h;
		} else if (md && md->display_id) {
			SDL_Rect db;
			SDL_GetDisplayBounds(md->display_id, &db);
			bool on_target = (nx >= db.x && nx < db.x + db.w
				&& ny >= db.y && ny < db.y + db.h);
			if (!on_target) {
				nx = db.x + (db.w - nw) / 2;
				ny = db.y + (db.h - nh) / 2;
			}
		}

		// For fullwindow mode, skip the resize/reposition cycle if the window
		// is already in desktop fullscreen on the correct display. On macOS,
		// SDL_GetWindowPosition on a fullscreen window may return coordinates
		// that don't match SDL_GetDisplayBounds (e.g. Space-relative vs global),
		// causing a false mismatch that triggers SDL_SetWindowFullscreen toggling
		// — which produces a visible Spaces switching animation.
		//
		// However, if the window is in exclusive fullscreen and we want desktop
		// fullscreen (full-window), we must force the mode switch even when
		// sizes match (both modes cover the full display).
		// SDL_GetWindowFullscreenMode returns NULL for desktop fullscreen,
		// non-NULL for exclusive fullscreen.
		bool needs_resize = (w != nw || h != nh || x != nx || y != ny);
		if (fullwindow) {
			SDL_WindowFlags wflags = SDL_GetWindowFlags(mon->amiga_window);
			if (wflags & SDL_WINDOW_FULLSCREEN) {
				SDL_DisplayID current_display = SDL_GetDisplayForWindow(mon->amiga_window);
				SDL_DisplayID target_display = md ? md->display_id : current_display;
				const SDL_DisplayMode* current_fs_mode = SDL_GetWindowFullscreenMode(mon->amiga_window);
				if (current_display == target_display && current_fs_mode == nullptr) {
					// Already in desktop fullscreen (full-window) on the correct display, skip
					needs_resize = false;
					write_log(_T("fullwindow: skipping resize — already in desktop fullscreen on target display\n"));
				} else if (current_fs_mode != nullptr) {
					// Window is in exclusive fullscreen but we want desktop fullscreen — force switch
					needs_resize = true;
					write_log(_T("fullwindow: forcing mode switch from exclusive to desktop fullscreen\n"));
				}
			}
		}

		if (needs_resize) {
			w = nw;
			h = nh;
			x = nx;
			y = ny;
			mon->in_sizemove++;

			// SDL3: position/size changes are deferred on fullscreen windows,
			// so exit fullscreen first, reposition, then re-enter
			if (fullwindow || fullscreen) {
				SDL_SetWindowFullscreen(mon->amiga_window, false);
			}

			SDL_SetWindowPosition(mon->amiga_window, x, y);
			SDL_SetWindowSize(mon->amiga_window, w, h);
			SDL_SyncWindow(mon->amiga_window);

			if (fullwindow) {
				SDL_SetWindowFullscreenMode(mon->amiga_window, nullptr);
				SDL_SetWindowFullscreen(mon->amiga_window, true);
				SDL_SyncWindow(mon->amiga_window);
			} else if (fullscreen) {
				SDL_DisplayID display_id = md ? md->display_id : SDL_GetDisplayForWindow(mon->amiga_window);
				bool mode_set = false;
				if (display_id) {
					SDL_DisplayMode closest;
					if (SDL_GetClosestFullscreenDisplayMode(
						display_id, w, h, 0.0f, true, &closest)) {
						SDL_SetWindowFullscreenMode(mon->amiga_window, &closest);
						mode_set = true;
					}
				}
				if (!mode_set) {
					write_log(_T("Fullscreen: no matching display mode for %dx%d, falling back to desktop mode (full-window)\n"), w, h);
					SDL_SetWindowFullscreenMode(mon->amiga_window, nullptr);
				}
				SDL_SetWindowFullscreen(mon->amiga_window, true);
				SDL_SyncWindow(mon->amiga_window);
			}
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
	SDL_DisplayID bounds_display = (md && md->display_id) ? md->display_id : SDL_GetPrimaryDisplay();
	SDL_GetDisplayBounds(bounds_display, &displayBounds);

	bool pos_on_target = (stored_x >= displayBounds.x
		&& stored_x < displayBounds.x + displayBounds.w
		&& stored_y >= displayBounds.y
		&& stored_y < displayBounds.y + displayBounds.h);

	if (pos_on_target) {
		rc.x = stored_x;
		rc.y = stored_y;
	} else {
		rc.x = displayBounds.x + (displayBounds.w - mon->currentmode.current_width) / 2;
		rc.y = displayBounds.y + (displayBounds.h - mon->currentmode.current_height) / 2;
	}

	rc.w = mon->currentmode.current_width;
	rc.h = mon->currentmode.current_height;

	oldx = rc.x;
	oldy = rc.y;

	mon->win_x_diff = rc.x - oldx;
	mon->win_y_diff = rc.y - oldy;

	if (mon->monitor_id > 0) {
		getextramonitorpos(mon, &rc);
	}

	if (fullwindow) {
		rc = md->rect;
#ifdef __ANDROID__
		flags |= SDL_WINDOW_RESIZABLE;
#endif
		mon->currentmode.native_width = rc.w;
		mon->currentmode.native_height = rc.h;
	} else if (fullscreen) {
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
		flags |= SDL_WINDOW_RESIZABLE;
	}
	if (currprefs.main_alwaysontop) {
		flags |= SDL_WINDOW_ALWAYS_ON_TOP;
	}
	if (borderless)
		flags |= SDL_WINDOW_BORDERLESS;
	if (currprefs.start_minimized || currprefs.headless)
		flags |= SDL_WINDOW_HIDDEN;

	// For secondary monitors, create a dedicated renderer BEFORE window creation
	// so that window flags (e.g., SDL_WINDOW_OPENGL) are set correctly
	IRenderer* renderer_to_use = nullptr;
	if (mon->monitor_id > 0) {
		if (!mon->renderer) {
			mon->renderer = create_renderer();
		}
		renderer_to_use = mon->renderer.get();
	} else {
		renderer_to_use = g_renderer.get();
	}

	if (renderer_to_use) {
		flags |= renderer_to_use->get_window_flags();
	}
	TCHAR wintitle[256];
	if (mon->monitor_id > 0)
		_stprintf(wintitle, _T("Amiberry [%d]"), mon->monitor_id + 1);
	else if (last_active_config[0])
		_stprintf(wintitle, _T("Amiberry - [%s]"), last_active_config);
	else
		_tcscpy(wintitle, _T("Amiberry"));
	mon->amiga_window = SDL_CreateWindow(wintitle,
		rc.w, rc.h,
		flags);
	if (!mon->amiga_window) {
		write_log(_T("main window creation failed\n"));
		write_log(SDL_GetError());
		write_log("\n");
		return 0;
	}
	SDL_SetWindowPosition(mon->amiga_window, rc.x, rc.y);
	if (fullwindow || fullscreen) {
		SDL_SyncWindow(mon->amiga_window);
	}

	if (fullwindow) {
		SDL_SetWindowFullscreenMode(mon->amiga_window, nullptr);
		SDL_SetWindowFullscreen(mon->amiga_window, true);
		SDL_SyncWindow(mon->amiga_window);
	} else if (fullscreen) {
		SDL_DisplayID display_id = md ? md->display_id : SDL_GetDisplayForWindow(mon->amiga_window);
		bool mode_set = false;
		if (display_id) {
			SDL_DisplayMode closest;
			if (SDL_GetClosestFullscreenDisplayMode(
				display_id, mon->currentmode.native_width, mon->currentmode.native_height, 0.0f, true, &closest)) {
				SDL_SetWindowFullscreenMode(mon->amiga_window, &closest);
				mode_set = true;
			}
		}
		if (!mode_set) {
			// No matching fullscreen display mode found (e.g. custom EDID with limited modes).
			// Fall back to borderless fullscreen desktop mode to avoid a windowed-size window.
			write_log(_T("Fullscreen: no matching display mode for %dx%d, falling back to borderless fullscreen desktop mode\n"),
				mon->currentmode.native_width, mon->currentmode.native_height);
			SDL_SetWindowFullscreenMode(mon->amiga_window, nullptr);
		}
		SDL_SetWindowFullscreen(mon->amiga_window, true);
		SDL_SyncWindow(mon->amiga_window);
	}

	SDL_Rect rc2;
	GetWindowRect(mon->amiga_window, &rc2);
	int expected_client_width = mon->currentmode.current_width;
	int expected_client_height = mon->currentmode.current_height;
	mon->window_extra_width = rc2.w - expected_client_width;
	mon->window_extra_height = rc2.h - expected_client_height;

	w = mon->currentmode.native_width;
	h = mon->currentmode.native_height;

	if (!mon->amiga_window) {
		write_log(_T("creation of amiga window failed\n"));
		close_hwnds(mon, false);
		return 0;
	}

	gfx_platform_set_window_icon(mon->amiga_window);

	if (renderer_to_use) {
		if (!renderer_to_use->create_platform_renderer(mon)) {
			write_log(_T("creation of platform renderer failed\n"));
			close_hwnds(mon, false);
			return 0;
		}
		if (mon->amiga_renderer) {
			DPIHandler::set_render_scale(mon->amiga_renderer);
		}
	}

    // Cache current display mode for scaling heuristics
    {
        SDL_DisplayID disp_id = SDL_GetDisplayForWindow(mon->amiga_window);
        const SDL_DisplayMode* dm = disp_id ? SDL_GetDesktopDisplayMode(disp_id) : nullptr;
        if (dm) {
            sdl_mode = *dm;
        }
    }
	updatewinrect(mon, true);
	GetWindowRect(mon->amiga_window, &mon->mainwin_rect);
	if (fullscreen || fullwindow)
		movecursor(mon, x + w / 2, y + h / 2);

	mon->window_extra_height_bar = 0;
	//mon->dpi = getdpiforwindow(mon->monitor_id);

	// SDL3: SDL_HINT_GRAB_KEYBOARD removed, keyboard grab is controlled via SDL_SetWindowKeyboardGrab()

	if (SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0"))
		write_log("SDL3: Set window not to minimize on focus loss\n");

	// Initialize HiDPI scale cache for mouse input
	mon->hidpi_needs_scaling = false;
	mon->hidpi_scale_x = 1.0f;
	mon->hidpi_scale_y = 1.0f;
	if (renderer_to_use && mon->amiga_window) {
		int win_w, win_h, draw_w, draw_h;
		SDL_GetWindowSize(mon->amiga_window, &win_w, &win_h);
		renderer_to_use->get_drawable_size(mon->amiga_window, &draw_w, &draw_h);
		if (win_w > 0 && draw_w > 0 && win_w != draw_w) {
			mon->hidpi_scale_x = (float)draw_w / (float)win_w;
			mon->hidpi_needs_scaling = true;
		}
		if (win_h > 0 && draw_h > 0 && win_h != draw_h) {
			mon->hidpi_scale_y = (float)draw_h / (float)win_h;
			mon->hidpi_needs_scaling = true;
		}
	}

	mon->active = true;
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
			amiga_surface = SDL_CreateSurface(display_width, display_height, pixel_format);
			if (!amiga_surface) {
				write_log("Failed to create amiga_surface: %s\n", SDL_GetError());
				return false;
			}
			update_system_pixel_format();
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

		if (isfullscreen() < 0) {
			const SDL_Rect rc = getdisplay(&currprefs, mon->monitor_id)->rect;
			mon->currentmode.native_width = rc.w;
			mon->currentmode.native_height = rc.h;
		}

	IRenderer* renderer = get_renderer(mon->monitor_id);
		if (renderer) {
			// Quick reopen path: if the window already exists and the renderer
			// context is still valid, skip the expensive destroy/recreate cycle.
			// This avoids tearing down the GL context or Vulkan surface/swapchain
			// on resolution-only changes, which on macOS triggers the Spaces
			// switching animation in fullwindow mode.
			if (mon->amiga_window && renderer->has_context()) {
				if (!create_windows(mon))
				{
					close_hwnds(mon, false);
					return false;
				}
			} else {
				int ctx_attempts = 0;
				bool ctx_success = false;

				while (ctx_attempts < 2 && !ctx_success) {
					if (!renderer->set_context_attributes(ctx_attempts))
					{
						write_log("Failed to set context attributes for mode %d\n", ctx_attempts);
						ctx_attempts++;
						continue;
					}

					if (!create_windows(mon))
					{
						close_hwnds(mon, false);
						return false;
					}

					renderer = get_renderer(mon->monitor_id);

					renderer->destroy_shaders();
					renderer->destroy_context();

					if (renderer->init_context(mon->amiga_window))
					{
						ctx_success = true;
					}
					else
					{
						write_log("Renderer context init failed for mode %d on monitor %d. Retrying...\n", ctx_attempts, mon->monitor_id);
						close_windows(mon, false);
						ctx_attempts++;
					}
				}

				if (!ctx_success) {
					write_log("All renderer context attempts failed for monitor %d. Aborting doInit.\n", mon->monitor_id);
					return false;
				}
			}
		} else {
			if (!create_windows(mon))
			{
				close_hwnds(mon, false);
				return false;
			}
		}
#ifdef PICASSO96
		if (mon->screen_is_picasso) {
			struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];
			display_width = state->Width ? state->Width : 640;
			display_height = state->Height ? state->Height : 480;
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

	// For secondary monitors, use per-monitor surface instead of global
	SDL_Surface*& surface_ref = (mon->monitor_id > 0) ? mon->amiga_surface : amiga_surface;
	if (!surface_ref || surface_ref->w != display_width || surface_ref->h != display_height || surface_ref->format != pixel_format)
	{
		if (surface_ref)
		{
			SDL_DestroySurface(surface_ref);
			surface_ref = nullptr;
		}
		surface_ref = SDL_CreateSurface(display_width, display_height, pixel_format);
		if (!surface_ref) {
			write_log("Failed to create amiga_surface for monitor %d: %s\n", mon->monitor_id, SDL_GetError());
			return false;
		}
		update_system_pixel_format();
	}

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

	// Initialize the ImGui overlay context for the on-screen keyboard.
	// Always init so toggle works; vkbd_allowed() gates rendering.
	{
		bool overlay_inited = false;
#ifdef USE_VULKAN
		auto* vk = get_vulkan_renderer();
		if (vk && vk->get_vk_device()) {
			ImGui_ImplVulkan_InitInfo vk_info{};
			vk_info.Instance = vk->get_vk_instance();
			vk_info.PhysicalDevice = vk->get_vk_physical_device();
			vk_info.Device = vk->get_vk_device();
			vk_info.QueueFamily = vk->get_vk_graphics_queue_family();
			vk_info.Queue = vk->get_vk_graphics_queue();
			// DescriptorPool is created internally by imgui_overlay_init_vulkan
			vk_info.DescriptorPool = VK_NULL_HANDLE;
			vk_info.MinImageCount = vk->get_vk_min_image_count();
			vk_info.ImageCount = vk->get_vk_image_count();
			vk_info.PipelineInfoMain.RenderPass = vk->get_vk_render_pass();
			vk_info.PipelineInfoMain.Subpass = 0;
			vk_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			imgui_overlay_init_vulkan(mon->amiga_window, &vk_info);
			overlay_inited = true;
		}
#endif
		if (!overlay_inited) {
			void* gl_ctx = nullptr;
#ifdef USE_OPENGL
			auto* gl_renderer = get_opengl_renderer();
			if (gl_renderer)
				gl_ctx = gl_renderer->get_gl_context();
#endif
			imgui_overlay_init(mon->amiga_window, mon->amiga_renderer, gl_ctx);
		}
	}
	imgui_osk_init();
	imgui_osk_set_transparency(
		(currprefs.vkbd_transparency > 0)
			? static_cast<float>(currprefs.vkbd_transparency) / 100.0f
			: 0.85f);
	imgui_osk_set_language(currprefs.vkbd_language);

	// Initialize on-screen joystick if enabled
	if (currprefs.onscreen_joystick)
	{
		// Ensure the on-screen joystick device is registered without
		// re-enumerating physical devices (which would wipe custom
		// controller mappings loaded from config files).
		ensure_onscreen_joystick_registered();

		on_screen_joystick_init(mon->amiga_renderer);
		int sw = 0, sh = 0;
		IRenderer* renderer = get_renderer(mon->monitor_id);
		if (renderer) {
			renderer->get_drawable_size(mon->amiga_window, &sw, &sh);
			on_screen_joystick_update_layout(sw, sh, renderer->render_quad);
		} else if (mon->amiga_renderer) {
			SDL_GetCurrentRenderOutputSize(mon->amiga_renderer, &sw, &sh);
			on_screen_joystick_update_layout(sw, sh, {});
		}
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
			success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		} else {
			// Desktop OpenGL (x86, x86_64, ARM desktops, macOS): Try Core Profile 3.3
			write_log(_T("Requesting OpenGL 3.3 Core context...\n"));
			success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#ifdef AMIBERRY_MACOS
			// macOS requires the forward-compatible flag for OpenGL 3.2+ Core Profile
			success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
		}
	} else {
		// Fallback: Legacy OpenGL 2.1 Compatibility
		write_log(_T("Requesting OpenGL 2.1 Compatibility context...\n"));
		success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		success &= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	}

	// Sensible defaults.
	success &= SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	success &= SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	success &= SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

	// Optional: request RGBA8
	success &= SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	success &= SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	success &= SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	success &= SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	return success;
}

#endif // AMIBERRY
