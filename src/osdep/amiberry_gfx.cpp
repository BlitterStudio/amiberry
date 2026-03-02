/*
* Amiberry Amiga Emulator
*
* SDL2 interface
* 
* Copyright 2025 Dimitris Panokostas
* 
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
#include "picasso96.h"
#include "gui.h"
#include "amiberry_gfx.h"
#include "sounddep/sound.h"
#include "inputdevice.h"
#ifdef WITH_MIDI
#include "midi.h"
#endif
#include "serial.h"
#include "parallel.h"
#include "sampler.h"
#include "gfxboard.h"
#include "statusline.h"
#include "devices.h"

#include "threaddep/thread.h"
#include "vkbd/vkbd.h"
#include "on_screen_joystick.h"
#include "fsdb_host.h"
#include "savestate.h"
#include "uae/types.h"

#ifdef USE_OPENGL
#include "gl_platform.h"

#ifdef LIBRETRO
#include "libretro_shared.h"
#endif

#ifndef GL_BGRA
#ifdef GL_BGRA_EXT
#define GL_BGRA GL_BGRA_EXT
#else
#define GL_BGRA 0x80E1
#endif
#endif

#include "crtemu.h"
#include "crt_frame.h"
#include "external_shader.h"
#include "shader_preset.h"

#endif // USE_OPENGL

#ifdef WITH_MIDIEMU
#include "midiemu.h"
#endif

#include "dpi_handler.hpp"
#include "registry.h"
#include "target.h"
#include "gfx_state.h"
#include "gfx_window.h"
#include "gl_shader_dispatch.h"
#include "gl_overlays.h"
#include "gfx_colors.h"
#include "gfx_prefs_check.h"
#include "display_modes.h"

#ifdef USE_OPENGL
#include <SDL_image.h>
#endif

#ifdef AMIBERRY
bool force_auto_crop = false;
SDL_DisplayMode sdl_mode;
SDL_Surface* amiga_surface = nullptr;
#ifndef USE_OPENGL
SDL_Texture* p96_cursor_overlay_texture = nullptr;  // Software cursor overlay for RTG
#endif

#ifdef USE_OPENGL
SDL_GLContext gl_context;
ShaderState g_shader;

// Load or unload the CRT bezel frame overlay based on amiberry_options.use_bezel
void update_crtemu_bezel()
{
	if (g_shader.crtemu == nullptr)
		return;
	if (amiberry_options.use_bezel) {
		auto* frame_pixels = new CRTEMU_U32[CRT_FRAME_WIDTH * CRT_FRAME_HEIGHT];
		crt_frame(frame_pixels);
		crtemu_frame(g_shader.crtemu, frame_pixels, CRT_FRAME_WIDTH, CRT_FRAME_HEIGHT);
		delete[] frame_pixels;
	} else {
		crtemu_frame(g_shader.crtemu, nullptr, 0, 0);
	}
}

#else
SDL_Texture* amiga_texture;
#endif

SDL_Rect render_quad;
static int dx = 0, dy = 0;
SDL_Rect crop_rect;
const char* sdl_video_driver;
bool kmsdrm_detected = false;

Uint32 pixel_format = SDL_PIXELFORMAT_ABGR8888;

static frame_time_t last_synctime;

VSyncState g_vsync;

static SDL_Surface* current_screenshot = nullptr;
std::string screenshot_filename;
FILE* screenshot_file = nullptr;
int delay_savestate_frame = 0;
#endif

struct MultiDisplay Displays[MAX_DISPLAYS];
struct AmigaMonitor AMonitors[MAX_AMIGAMONITORS];

void updatepicasso96(struct AmigaMonitor* mon);

#include "gfx_platform_internal.h"

int window_led_drives, window_led_drives_end;
int window_led_hd, window_led_hd_end;
int window_led_joys, window_led_joys_end, window_led_joy_start;
int window_led_msg, window_led_msg_end, window_led_msg_start;

int wasfs[2];
const TCHAR* wasfsname[2] = { _T("FullScreenMode"), _T("FullScreenModeRTG") };

int default_freq = 60;

int vsync_activeheight, vsync_totalheight;
float vsync_vblank, vsync_hblank;
bool beamracer_debug;
bool gfx_hdr;

void gfx_lock()
{
}
void gfx_unlock()
{
}

#ifdef AMIBERRY
float SDL2_getrefreshrate(const int monid)
{
	SDL_DisplayMode mode;
	if (SDL_GetCurrentDisplayMode(monid, &mode) != 0)
	{
		write_log("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
		return 0;
	}
	return static_cast<float>(mode.refresh_rate);
}

#ifdef USE_OPENGL
// current_vsync_interval, cached_refresh_rate, gl_state_initialized
// are now in the VSyncState g_vsync struct declared above

void update_vsync(const int monid)
{
	if (!AMonitors[monid].amiga_window) return;
	
	const AmigaMonitor* mon = &AMonitors[monid];
	const auto idx = mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE;
	const int vsync_mode = currprefs.gfx_apmode[idx].gfx_vsync;
	int interval = 0;

	if (vsync_mode > 0) {
		if (vsync_mode > 1) {
			// VSync 50/60Hz: Adapt for High Refresh Rate Monitors
			// We cache the refresh rate check to avoid expensive calls every frame
			if (g_vsync.cached_refresh_rate <= 0.0f) {
				g_vsync.cached_refresh_rate = SDL2_getrefreshrate(monid);
				write_log("VSync: Detected refresh rate: %.2f Hz\n", g_vsync.cached_refresh_rate);
			}

			float target_fps = (float)vblank_hz;
			if (target_fps < 45 || target_fps > 125) target_fps = 50.0f; // Sanity check

			if (g_vsync.cached_refresh_rate > 0) {
				interval = (int)std::round(g_vsync.cached_refresh_rate / target_fps);
				if (interval < 1) interval = 1;
			}
			else {
				interval = 1;
			}
		}
		else {
			// Standard VSync
			interval = 1;
		}
	}
	
	if (g_vsync.current_interval != interval) {
		if (SDL_GL_SetSwapInterval(interval) == 0) {
			write_log("OpenGL VSync: Mode %d, Interval set to %d\n", vsync_mode, interval);
		}
		else {
			write_log("OpenGL VSync: Failed to set interval %d: %s (will not retry)\n", interval, SDL_GetError());
		}
		// Update interval even on failure to prevent repeated attempts
		g_vsync.current_interval = interval;
	}
}
#endif

void GetWindowRect(SDL_Window* window, SDL_Rect* rect)
{
	SDL_GetWindowPosition(window, &rect->x, &rect->y);
	SDL_GetWindowSize(window, &rect->w, &rect->h);
}

bool MonitorFromPoint(const SDL_Point pt)
{
	SDL_Rect display_bounds;
	bool point_in_display = false;
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
		if (SDL_GetDisplayBounds(i, &display_bounds) == 0) {
			if (SDL_PointInRect(&pt, &display_bounds)) {
				point_in_display = true;
				break;
			}
		}
	}
	return point_in_display;
}

#endif

#ifdef AMIBERRY
bool vkbd_allowed(const int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	return currprefs.vkbd_enabled && !mon->screen_is_picasso;
}

static bool SDL2_renderframe(const int monid, int mode, int immediate)
{
	// Skip all rendering if in headless mode
	if (currprefs.headless) {
		return amiga_surface != nullptr;
	}
	if (gfx_platform_skip_renderframe(monid, mode, immediate))
		return amiga_surface != nullptr;

	const amigadisplay* ad = &adisplays[monid];
	// Unified OSD update: handle both native (CHIPSET) and RTG modes
	if (((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on) ||
		((currprefs.leds_on_screen & STATUSLINE_RTG) && ad->picasso_on))
	{
		update_leds(monid);
	}
#ifdef WITH_THREADED_CPU
	if (ad->picasso_zero_copy_update_needed) {
		const_cast<amigadisplay*>(ad)->picasso_zero_copy_update_needed = false;
		if (currprefs.rtg_zerocopy) {
			target_graphics_buffer_update(monid, true);
		}
	}
#endif
#ifdef USE_OPENGL
	return amiga_surface != nullptr;
#else
	if (amiga_texture && amiga_surface)
	{
		AmigaMonitor* mon = &AMonitors[monid];

		// Ensure the draw color is black for clearing
		SDL_SetRenderDrawColor(mon->amiga_renderer, 0, 0, 0, 255);
		SDL_RenderClear(mon->amiga_renderer);

		// If a full render is needed or there are no specific dirty rects, update the whole texture.
		if (mon->full_render_needed || mon->dirty_rects.empty()) {
			if (amiga_surface) {
				SDL_UpdateTexture(amiga_texture, NULL, amiga_surface->pixels, amiga_surface->pitch);
			}
		} else {
			// Otherwise, update only the collected dirty rectangles.
			for (const auto& rect : mon->dirty_rects) {
				SDL_UpdateTexture(amiga_texture, &rect, static_cast<const uae_u8*>(amiga_surface->pixels) + rect.y * amiga_surface->pitch + rect.x * amiga_surface->format->BytesPerPixel, amiga_surface->pitch);
			}
		}

		// Clear the dirty rects list for the next frame.
		mon->dirty_rects.clear();
		mon->full_render_needed = false;

		const SDL_Rect* p_crop = &crop_rect;
		const SDL_Rect* p_quad = &render_quad;
		SDL_Rect rtg_rect;

		if (ad->picasso_on) {
			rtg_rect = { 0, 0, amiga_surface->w, amiga_surface->h };
			p_crop = &rtg_rect;
			p_quad = &rtg_rect;

			int lw, lh;
			SDL_RenderGetLogicalSize(mon->amiga_renderer, &lw, &lh);
			if (lw != rtg_rect.w || lh != rtg_rect.h) {
				SDL_RenderSetLogicalSize(mon->amiga_renderer, rtg_rect.w, rtg_rect.h);
			}
		}

		SDL_RenderCopyEx(mon->amiga_renderer, amiga_texture, p_crop, p_quad, 0, nullptr, SDL_FLIP_NONE);

#ifndef USE_OPENGL
		// Render Software Cursor Overlay for RTG (when using relative mouse mode)
		if (ad->picasso_on && p96_uses_software_cursor()) {
			if (p96_cursor_needs_update() || !p96_cursor_overlay_texture) {
				SDL_Surface* s = p96_get_cursor_overlay_surface();
				if (s) {
					if (p96_cursor_overlay_texture)
						SDL_DestroyTexture(p96_cursor_overlay_texture);
					p96_cursor_overlay_texture = SDL_CreateTextureFromSurface(mon->amiga_renderer, s);
					if (p96_cursor_overlay_texture)
						SDL_SetTextureBlendMode(p96_cursor_overlay_texture, SDL_BLENDMODE_BLEND);
				}
			}

			if (p96_cursor_overlay_texture) {
				int cx, cy, cw, ch;
				p96_get_cursor_position(&cx, &cy);
				p96_get_cursor_dimensions(&cw, &ch);
				
				// Renderer logical size matches RTG resolution, so 1:1 mapping
				SDL_Rect dst_cursor = { cx, cy, cw, ch };
				SDL_RenderCopy(mon->amiga_renderer, p96_cursor_overlay_texture, nullptr, &dst_cursor);
			}
		}
#endif

		// GPU-composited Status Line (OSD) for both native and RTG
		if ((((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on) ||
			 ((currprefs.leds_on_screen & STATUSLINE_RTG) && ad->picasso_on)) && mon->statusline_texture)
		{
			int slx, sly, dst_w, dst_h;
			SDL_RenderGetLogicalSize(mon->amiga_renderer, &dst_w, &dst_h);
			if (dst_w == 0 || dst_h == 0) {
				SDL_GetRendererOutputSize(mon->amiga_renderer, &dst_w, &dst_h);
			}
			statusline_getpos(monid, &slx, &sly, dst_w, dst_h);
			SDL_Rect dst_osd = { slx, sly, mon->statusline_surface->w, mon->statusline_surface->h };
			SDL_RenderCopy(mon->amiga_renderer, mon->statusline_texture, nullptr, &dst_osd);
		}

		if (vkbd_allowed(monid))
		{
			vkbd_redraw();
		}

		if (on_screen_joystick_is_enabled())
		{
			on_screen_joystick_redraw(mon->amiga_renderer);
		}

		return true;
	}
#endif

	return false;
}

static void SDL2_showframe(const int monid)
{
	if (gfx_platform_present_frame(amiga_surface)) {
		return;
	}

	// Skip presentation if headless mode
	if (currprefs.headless) {
		return;
	}

	const AmigaMonitor* mon = &AMonitors[monid];
	SDL_RenderPresent(mon->amiga_renderer);
	if (g_vsync.waitvblankthread_mode <= 0)
		g_vsync.wait_vblank_timestamp = read_processor_time();
}

void flush_screen(struct vidbuffer* vb, int y_start, int y_end)
{
	AmigaMonitor* mon = &AMonitors[vb->monitor_id];
	// A call to flush_screen implies a full refresh is needed.
	mon->full_render_needed = true;
}

static void SDL2_refresh(const int monid)
{
	SDL2_renderframe(monid, 1, 1);
	SDL2_showframe(monid);
}
#endif //AMIBERRY

void getgfxoffset(const int monid, float* dxp, float* dyp, float* mxp, float* myp)
{
	const AmigaMonitor* mon = &AMonitors[monid];
	const amigadisplay* ad = &adisplays[monid];
	float dx = 0, dy = 0, mx = 1.0, my = 1.0;
	// Determine effective Amiga source dimensions/offset
	float src_w = 0, src_h = 0;
	float src_x = 0, src_y = 0;

#ifdef AMIBERRY
	if (currprefs.gfx_auto_crop && !ad->picasso_on) {
		src_w = static_cast<float>(crop_rect.w);
		src_h = static_cast<float>(crop_rect.h);
		src_x = static_cast<float>(crop_rect.x);
		src_y = static_cast<float>(crop_rect.y);
	}
#endif
	if (src_w <= 0 && amiga_surface) {
		src_w = static_cast<float>(amiga_surface->w);
		src_h = static_cast<float>(amiga_surface->h);
		src_x = 0;
		src_y = 0;
	}

#ifdef USE_OPENGL
	// OpenGL Path (Windowed & Fullscreen)
	// Always use render_quad if available, as it reflects the true viewport
	if (amiga_surface && render_quad.w > 0 && render_quad.h > 0) {
		// Scaling: Viewport / Amiga Source
		if (src_w > 0) mx = static_cast<float>(render_quad.w) / src_w;
		if (src_h > 0) my = static_cast<float>(render_quad.h) / src_h;

		if (ad->picasso_on) {
			// RTG Mode: inputdevice logic x = (x_win - dx) * fmx + fdx * fmx (Wait, verify formula)
			// Actually: x = (x - XOffset) * fmx + fdx * fmx
			// We want: x_amiga = (x_win - render_quad.x) * fmx + crop_x
			// InputDevice: x_win * fmx + (fdx * fmx)
			// => fdx * fmx = -render_quad.x * fmx + crop_x
			// => fdx = -render_quad.x + crop_x / fmx
			// => fdx = -render_quad.x + crop_x * mx
			
			dx = -static_cast<float>(render_quad.x) + src_x * mx;
			dy = -static_cast<float>(render_quad.y) + src_y * my;
		} else {
			// Native Mode: x = x * fmx - fdx (where x is already window coords)
			// Wait, inputdevice Native: x = (x * fmx) - fdx
			// We want: x_amiga = (x_win - render_quad.x) * fmx + crop_x
			// x_win * fmx - (render_quad.x * fmx - crop_x)
			// => fdx = render_quad.x * fmx - crop_x
			// => fdx = render_quad.x / mx - crop_x
			
			dx = static_cast<float>(render_quad.x) / mx - src_x;
			dy = static_cast<float>(render_quad.y) / my - src_y;
		}
	} else 
#endif
	// Fallback / Software Renderer Path
	if (mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
#ifdef AMIBERRY
		// Legacy AutoCrop for software renderer
		if (currprefs.gfx_auto_crop) {
			dx -= src_x;
			dy -= src_y;
		}
#endif
		if (!(mon->scalepicasso && mon->screen_is_picasso) &&
			!(mon->currentmode.fullfill && (mon->currentmode.current_width > mon->currentmode.native_width || 
			                                 mon->currentmode.current_height > mon->currentmode.native_height))) {
			dx += (mon->currentmode.native_width - mon->currentmode.current_width) / 2;
			dy += (mon->currentmode.native_height - mon->currentmode.current_height) / 2;
		}
	}

	*dxp = dx;
	*dyp = dy;
	*mxp = 1.0f / mx;
	*myp = 1.0f / my;
}


bool render_screen(const int monid, const int mode, const bool immediate)
{
	// Skip if headless mode
	if (currprefs.headless) {
		return true;
	}

	struct AmigaMonitor* mon = &AMonitors[monid];
	const struct amigadisplay* ad = &adisplays[monid];
	int cnt;

	mon->render_ok = false;
	if (ad->picasso_on || monitor_off) {
		return mon->render_ok;
	}
	cnt = 0;
	while (mon->wait_render) {
		sleep_millis(1);
		cnt++;
		if (cnt > 500) {
			return mon->render_ok;
		}
	}
	mon->render_ok = SDL2_renderframe(monid, mode, immediate);
	return mon->render_ok;
}

bool show_screen_maybe(const int monid, const bool show)
{
	const struct amigadisplay* ad = &adisplays[monid];
	const struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	if (!ap->gfx_vflip || ap->gfx_vsyncmode == 0 || ap->gfx_vsync <= 0) {
		if (show)
			show_screen(monid, 0);
		return false;
	}
	return false;
}

float target_adjust_vblank_hz(const int monid, float hz)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	int maxrate;
	if (!currprefs.lightboost_strobo)
		return hz;
	if (isfullscreen() > 0) {
		maxrate = mon->currentmode.freq;
	} else {
		maxrate = deskhz;
	}
	double nhz = hz * 2.0;
	if (nhz >= maxrate - 1 && nhz < maxrate + 1)
		hz -= 0.5f;
	return hz;
}

static float calculate_desired_aspect(const AmigaMonitor* mon)
{
	if (mon->screen_is_picasso && amiga_surface) {
		return (float)amiga_surface->w / (float)amiga_surface->h;
	}

	if (currprefs.gfx_correct_aspect) {
		return 4.0f / 3.0f;
	}

	if (amiga_surface) {
		return (float)amiga_surface->w / (float)amiga_surface->h;
	}

	return 4.0f / 3.0f;
}

void show_screen(const int monid, int mode)
{
	// Skip all rendering if headless mode
	if (currprefs.headless) {
		return;
	}

	AmigaMonitor* mon = &AMonitors[monid];
	if (gfx_platform_requires_window() && !mon->amiga_window) {
		return;
	}

	if (mode >= 0 && !mon->render_ok) {
		return;
	}
#ifdef USE_OPENGL
	// Safety check: if no shader is available, skip rendering
	if (!g_shader.crtemu && !g_shader.external && !g_shader.preset) {
		return;
	}

	const auto time = SDL_GetTicks();

	// Handle VSync options
	update_vsync(monid);

	int drawableWidth, drawableHeight;
	SDL_GL_GetDrawableSize(mon->amiga_window, &drawableWidth, &drawableHeight);

	// Ensure GL context is current for this window
	if (gl_context && mon->amiga_window) {
		if (SDL_GL_MakeCurrent(mon->amiga_window, gl_context) != 0) {
			write_log("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
		}
	}

	// Reset GL state to a known baseline - only on first frame after context creation
	if (!g_vsync.gl_initialized) {
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		g_vsync.gl_initialized = true;
	}

	float desired_aspect = calculate_desired_aspect(mon);
	if (desired_aspect <= 0.0f) desired_aspect = 4.0f / 3.0f;

	// When a custom bezel is active, constrain the emulator output to the
	// bezel's transparent screen hole. We define an "effective" render area
	// that all shader paths use instead of the full drawable.
	int renderAreaX = 0, renderAreaY = 0;
	int renderAreaW = drawableWidth, renderAreaH = drawableHeight;

	if (amiberry_options.use_custom_bezel && g_overlay.bezel_texture != 0 && g_overlay.bezel_hole_w > 0.0f && g_overlay.bezel_hole_h > 0.0f) {
		renderAreaX = static_cast<int>(g_overlay.bezel_hole_x * drawableWidth);
		renderAreaY = static_cast<int>(g_overlay.bezel_hole_y * drawableHeight);
		renderAreaW = static_cast<int>(g_overlay.bezel_hole_w * drawableWidth);
		renderAreaH = static_cast<int>(g_overlay.bezel_hole_h * drawableHeight);
	}

	int destW, destH, destX, destY;

	if (renderAreaX != 0 || renderAreaY != 0 ||
		renderAreaW != drawableWidth || renderAreaH != drawableHeight) {
		// Custom bezel active: fill the screen hole completely.
		// The bezel artist designed the transparent area as the screen shape.
		destW = renderAreaW;
		destH = renderAreaH;
		destX = renderAreaX;
		destY = renderAreaY;
	} else {
		// Normal mode: aspect-ratio-corrected letterboxing
		destW = renderAreaW;
		destH = static_cast<int>(renderAreaW / desired_aspect);

		if (destH > renderAreaH) {
			destH = renderAreaH;
			destW = static_cast<int>(renderAreaH * desired_aspect);
		}

		if (destW <= 0) destW = 1;
		if (destH <= 0) destH = 1;

		destX = (renderAreaW - destW) / 2;
		destY = (renderAreaH - destH) / 2;
	}

	// Only clear if letterboxing is active (frame doesn't cover entire window)
	if (destW < drawableWidth || destH < drawableHeight) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glViewport(destX, destY, destW, destH);

	// Update render_quad to reflect the actual drawn area
	render_quad.x = destX;
	render_quad.y = destY;
	render_quad.w = destW;
	render_quad.h = destH;

	// Check if cropping is active and valid
	bool is_cropped = (crop_rect.x != 0 || crop_rect.y != 0 ||
					   crop_rect.w != (amiga_surface ? amiga_surface->w : 0) ||
					   crop_rect.h != (amiga_surface ? amiga_surface->h : 0)) &&
					   (crop_rect.w > 0 && crop_rect.h > 0);

	// Validate that the crop rectangle produces usable dimensions within the surface
	if (is_cropped && amiga_surface) {
		int cx = std::max(0, crop_rect.x);
		int cy = std::max(0, crop_rect.y);
		int cw = std::min(crop_rect.w, amiga_surface->w - cx);
		int ch = std::min(crop_rect.h, amiga_surface->h - cy);
		if (cw <= 0 || ch <= 0) {
			is_cropped = false;
		}
	}

	// Handle shader preset rendering (multi-pass .glslp)
	if (g_shader.preset && g_shader.preset->is_valid()) {
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		int src_w = amiga_surface ? amiga_surface->w : 0;
		int src_h = amiga_surface ? amiga_surface->h : 0;
		if (is_cropped && amiga_surface) {
			src_w = std::min(crop_rect.w, amiga_surface->w - std::max(0, crop_rect.x));
			src_h = std::min(crop_rect.h, amiga_surface->h - std::max(0, crop_rect.y));
		}

		int viewport_w = std::max(destW, src_w);
		int viewport_h = std::max(destH, src_h);
		int viewport_x = renderAreaX + (renderAreaW - viewport_w) / 2;
		int viewport_y = renderAreaY + (renderAreaH - viewport_h) / 2;

		static int preset_frame_count = 0;

		if (is_cropped && amiga_surface) {
			const int bpp = 4;
			int x = std::max(0, crop_rect.x);
			int y = std::max(0, crop_rect.y);
			int w = std::min(crop_rect.w, amiga_surface->w - x);
			int h = std::min(crop_rect.h, amiga_surface->h - y);
			uae_u8* crop_ptr = static_cast<uae_u8*>(amiga_surface->pixels) + (y * amiga_surface->pitch) + (x * bpp);

			g_shader.preset->render(crop_ptr, w, h, amiga_surface->pitch,
				viewport_x, viewport_y, viewport_w, viewport_h, preset_frame_count++);
		} else if (amiga_surface) {
			g_shader.preset->render(static_cast<const unsigned char*>(amiga_surface->pixels),
				amiga_surface->w, amiga_surface->h, amiga_surface->pitch,
				viewport_x, viewport_y, viewport_w, viewport_h, preset_frame_count++);
		}

	}
	// Handle external shader rendering (simplified single-pass)
	else if (g_shader.external && g_shader.external->is_valid()) {
		// Explicitly disable attributes to avoid leakage from previous passes
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		
		// Get source dimensions for this frame
		int src_w = amiga_surface ? amiga_surface->w : 0;
		int src_h = amiga_surface ? amiga_surface->h : 0;
		if (is_cropped && amiga_surface) {
			src_w = std::min(crop_rect.w, amiga_surface->w - std::max(0, crop_rect.x));
			src_h = std::min(crop_rect.h, amiga_surface->h - std::max(0, crop_rect.y));
		}
		
		// Use aspect-corrected viewport, but ensure it's at least as large as the source
		// Many CRT shaders calculate scale = floor(OutputSize / InputSize) which fails if OutputSize < InputSize
		int viewport_w = std::max(destW, src_w);
		int viewport_h = std::max(destH, src_h);
		int viewport_x = renderAreaX + (renderAreaW - viewport_w) / 2;
		int viewport_y = renderAreaY + (renderAreaH - viewport_h) / 2;
		
		// Set viewport for shader rendering
		glViewport(viewport_x, viewport_y, viewport_w, viewport_h);

		if (is_cropped && amiga_surface) {
			// Fast path for cropping using GL_UNPACK_ROW_LENGTH
			const int bpp = 4;
			int x = std::max(0, crop_rect.x);
			int y = std::max(0, crop_rect.y);
			int w = std::min(crop_rect.w, amiga_surface->w - x);
			int h = std::min(crop_rect.h, amiga_surface->h - y);
			uae_u8* crop_ptr = static_cast<uae_u8*>(amiga_surface->pixels) + (y * amiga_surface->pitch) + (x * bpp);

			render_with_external_shader(g_shader.external, monid, crop_ptr,
				w, h, amiga_surface->pitch, viewport_x, viewport_y, viewport_w, viewport_h);
		} else if (amiga_surface) {
			// FAST PATH: No cropping
			render_with_external_shader(g_shader.external, monid, 
				static_cast<const uae_u8*>(amiga_surface->pixels),
				amiga_surface->w, amiga_surface->h, amiga_surface->pitch, viewport_x, viewport_y, viewport_w, viewport_h);
		}

	} else if (g_shader.crtemu) {
		// crtemu_present expects attribute 0 to be enabled.
		glEnableVertexAttribArray(0);
		// Disable other attributes that might have been enabled by OSD or other passes
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// When a custom bezel defines the viewport, skip crtemu's internal 4:3 letterboxing
		g_shader.crtemu->skip_aspect_correction = (renderAreaW != drawableWidth || renderAreaH != drawableHeight);

		if (g_shader.crtemu->type != CRTEMU_TYPE_NONE) {
			glViewport(renderAreaX, renderAreaY, renderAreaW, renderAreaH);
		}

		// Determine correct OpenGL format
		unsigned int gl_fmt, gl_type;
		int bpp = 4;
		if (pixel_format == SDL_PIXELFORMAT_ARGB8888) {
			gl_fmt = GL_BGRA;
			gl_type = GL_UNSIGNED_BYTE;
		}
		else if (pixel_format == SDL_PIXELFORMAT_RGB565) {
			gl_fmt = GL_RGB;
			gl_type = GL_UNSIGNED_SHORT_5_6_5;
			bpp = 2;
		}
		else if (pixel_format == SDL_PIXELFORMAT_RGB555) {
			gl_fmt = GL_RGBA;
			gl_type = GL_UNSIGNED_SHORT_5_5_5_1;
			bpp = 2;
		}
		else {
			gl_fmt = GL_RGBA;
			gl_type = GL_UNSIGNED_BYTE;
		}

		if (is_cropped && amiga_surface) {
			// Fast path for cropping using GL_UNPACK_ROW_LENGTH
			int x = std::max(0, crop_rect.x);
			int y = std::max(0, crop_rect.y);
			int w = std::min(crop_rect.w, amiga_surface->w - x);
			int h = std::min(crop_rect.h, amiga_surface->h - y);
			uae_u8* crop_ptr = static_cast<uae_u8*>(amiga_surface->pixels) + (y * amiga_surface->pitch) + (x * bpp);
			
			crtemu_present(g_shader.crtemu, time * 1000, reinterpret_cast<const CRTEMU_U32*>(crop_ptr),
				w, h, amiga_surface->pitch, 0xffffffff, 0x000000, gl_fmt, gl_type, bpp);
		} else if (amiga_surface) {
			// FAST PATH: No cropping.
			crtemu_present(g_shader.crtemu, time * 1000, (CRTEMU_U32 const*)amiga_surface->pixels,
			amiga_surface->w, amiga_surface->h, amiga_surface->pitch, 0xffffffff, 0x000000, gl_fmt, gl_type, bpp);
		}
	}

	render_software_cursor_gl(monid, destX, destY, destW, destH);
	render_bezel_overlay(drawableWidth, drawableHeight);
	render_osd(monid, destX, destY, destW, destH);

	if (vkbd_allowed(monid))
	{
		vkbd_redraw_gl(drawableWidth, drawableHeight);
	}

	if (on_screen_joystick_is_enabled())
	{
		on_screen_joystick_redraw_gl(drawableWidth, drawableHeight, render_quad);
	}

	SDL_GL_SwapWindow(mon->amiga_window);
#else
	SDL2_showframe(monid);
#endif
	mon->render_ok = false;
}

int lockscr(struct vidbuffer* vb, bool fullupdate, bool skip)
{
	const struct AmigaMonitor* mon = &AMonitors[vb->monitor_id];
	int ret = 0;

	if ((gfx_platform_requires_window() && !mon->amiga_window) || !amiga_surface)
		return ret;

	// Ensure blanking limits are open and synchronized at the start of frame locking
	set_custom_limits(-1, -1, -1, -1, false);

	if (vb->vram_buffer) {
		// Benchmarks have shown that Locking and Unlocking the Texture is slower than just calling UpdateTexture
		// Therefore, this is disabled in Amiberry.
		//
		//SDL_LockTexture(amiga_texture, nullptr, reinterpret_cast<void**>(&vb->bufmem), &vb->rowbytes);

		vb->bufmem = static_cast<uae_u8*>(amiga_surface->pixels);
		vb->rowbytes = amiga_surface->pitch;
		vb->width_allocated = amiga_surface->w;
		vb->height_allocated = amiga_surface->h;
		if (vb->bufmem) {
			ret = 1;
		}
	} else {
		ret = 1;
	}
	if (ret) {
		vb->locked = true;
	}
	return ret;
}

static void add_dirty_rect(struct AmigaMonitor* mon, const SDL_Rect& new_rect)
{
	// If the whole screen is already marked for update, do nothing.
	if (mon->full_render_needed) {
		return;
	}

	// Attempt to merge with the last rectangle if they are contiguous.
	if (!mon->dirty_rects.empty()) {
		SDL_Rect& last_rect = mon->dirty_rects.back();
		// Check if new_rect starts exactly where last_rect ends and they have the same width.
		if (last_rect.x == new_rect.x && last_rect.w == new_rect.w && (last_rect.y + last_rect.h) == new_rect.y) {
			// Merge by extending the height of the last rectangle.
			last_rect.h += new_rect.h;
			return;
		}
	}

	// If we have too many separate rectangles, it's more efficient to just do a full update.
	if (mon->dirty_rects.size() > 32) {
		mon->full_render_needed = true;
		mon->dirty_rects.clear();
		return;
	}

	mon->dirty_rects.push_back(new_rect);
}

void unlockscr(struct vidbuffer* vb, int y_start, int y_end)
{
	vb->locked = false;
	if (vb->vram_buffer) {
		vb->bufmem = nullptr;
		// Not using SDL2_UnlockTexture due to performance reasons, see lockscr for details
		//SDL_UnlockTexture(amiga_texture);

		// Record the dirty rectangle if y_start and y_end are valid.
		if (y_start >= 0 && y_end >= y_start) {
			AmigaMonitor* mon = &AMonitors[vb->monitor_id];
			
			// Clamp y_end to surface bounds to prevent out-of-bounds dirty rects
			// The drawing code may report y_end beyond the actual surface height
			int clamped_y_end = y_end;
			if (amiga_surface && clamped_y_end >= amiga_surface->h) {
				clamped_y_end = amiga_surface->h - 1;
			}
			
			// Only add dirty rect if we still have valid bounds after clamping
			if (clamped_y_end >= y_start) {
				SDL_Rect dirty_rect;
				dirty_rect.x = 0;
				dirty_rect.y = y_start;
				dirty_rect.w = vb->width_allocated;
				dirty_rect.h = clamped_y_end - y_start + 1;

				add_dirty_rect(mon, dirty_rect);
			}
		}
	}
}

void flush_clear_screen(struct vidbuffer* vb)
{
	if (!vb)
		return;

	// Early exit if buffer is not properly initialized
	if (!vb->bufmem || vb->width_allocated <= 0 || vb->height_allocated <= 0)
		return;

	if (lockscr(vb, true, false)) {
		// Use memset for better performance on aligned memory
		const size_t bytes_per_row = vb->width_allocated * vb->pixbytes;
		const size_t total_bytes = bytes_per_row * vb->height_allocated;

		if (vb->rowbytes == bytes_per_row) {
			// Buffer is contiguous, clear it all at once.
			memset(vb->bufmem, 0, total_bytes);
		} else {
			// Buffer is not contiguous, clear row by row.
			for (int y = 0; y < vb->height_allocated; y++)
				memset(static_cast<uae_u8*>(vb->bufmem) + y * vb->rowbytes, 0, bytes_per_row);
		}
		unlockscr(vb, -1, -1);
	}
}

float filterrectmult(int v1, float v2, int dmode)
{
	const float v = v1 / v2;
	const int vv = static_cast<int>(v + 0.5f);
	if (v > 1.5f && vv * v2 <= v1 && vv * (v2 + vv - 1) >= v1) {
		return static_cast<float>(vv);
	}
	if (!dmode) {
		return v;
	}
	if (v > 0.2f && v < 0.3f) {
		return 0.25f;
	}
	if (v > 0.4f && v < 0.6f) {
		return 0.5f;
	}
	return static_cast<float>(static_cast<int>(v + 0.5f));
}

static uae_u8* gfx_lock_picasso2(int monid, bool fullupdate)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[monid];
	uae_u8* p;

	if (amiga_surface == nullptr) {
		return nullptr;
	}

	// When zero-copy is active, amiga_surface->pixels IS the VRAM.
	// Detect this by comparing the surface pixels pointer with the RTG VRAM pointer.
	// Returning nullptr here prevents picasso_flushpixels from copying
	// VRAM to itself, which would cause display corruption.
	if (currprefs.rtg_zerocopy) {
		uae_u8* rtg_vram = p96_get_render_buffer_pointer(monid);
		if (rtg_vram != nullptr && amiga_surface->pixels == rtg_vram) {
			return nullptr;
		}
	}


	//SDL_LockTexture(amiga_texture, nullptr, reinterpret_cast<void**>(&p), &vidinfo->rowbytes);

	vidinfo->pixbytes = amiga_surface->format->BytesPerPixel;
	vidinfo->rowbytes = amiga_surface->pitch;
	vidinfo->maxwidth = amiga_surface->w;
	vidinfo->maxheight = amiga_surface->h;
	p = static_cast<uae_u8*>(amiga_surface->pixels);
	return p;
}
uae_u8* gfx_lock_picasso(const int monid, bool fullupdate)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	static uae_u8* p;
	if (mon->rtg_locked) {
		return p;
	}
	p = gfx_lock_picasso2(monid, fullupdate);
	if (p) {
		mon->rtg_locked = true;
	}
	return p;
}

void gfx_unlock_picasso(const int monid, const bool dorender)
{
	struct AmigaMonitor* mon = &AMonitors[monid];

	mon->rtg_locked = false;
	if (dorender) {
		if (mon->p96_double_buffer_needs_flushing) {
			//D3D_flushtexture(monid, mon->p96_double_buffer_first, mon->p96_double_buffer_last);
			mon->p96_double_buffer_needs_flushing = 0;
		}
	}
	//SDL_UnlockTexture(amiga_texture);
	if (dorender) {
		if (SDL2_renderframe(monid, true, false)) {
			mon->render_ok = true;
			show_screen_maybe(monid, true);
		}
	}
}

#ifdef PICASSO96

void DX_Invalidate(struct AmigaMonitor* mon, int x, int y, int width, int height)
{
	const struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	int last, lastx;

	if (width == 0 || height == 0)
		return;
	if (y < 0 || height < 0) {
		y = 0;
		height = vidinfo->height;
	}
	if (x < 0 || width < 0) {
		x = 0;
		width = vidinfo->width;
	}

	// Performance Optimization: Bridge Picasso96 invalidation to the SDL dirty rect system.
	// This allows the renderer to only upload the modified portion of the RTG screen.
	SDL_Rect dirty_rect;
	dirty_rect.x = x;
	dirty_rect.y = y;
	dirty_rect.w = width;
	dirty_rect.h = height;
	add_dirty_rect(mon, dirty_rect);

	last = y + height - 1;
	lastx = x + width - 1;
	mon->p96_double_buffer_first = y;
	mon->p96_double_buffer_last = last;
	mon->p96_double_buffer_firstx = x;
	mon->p96_double_buffer_lastx = lastx;
	mon->p96_double_buffer_needs_flushing = 1;
}

#endif

bool vsync_switchmode(const int monid, int hz)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	
	// In Full-Window mode or if using Adaptive VSync options, 
	// we do not need to strictly match/switch resolution refresh rates.
	// We accept the current state as valid.
	if (isfullscreen() < 0 || currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync > 1) {
		return true;
	}

	static struct PicassoResolution* oldmode;
	static int oldhz;
	int w = mon->currentmode.native_width;
	int h = mon->currentmode.native_height;
	struct MultiDisplay* md = getdisplay(&currprefs, monid);
	struct PicassoResolution* found;
	int newh, cnt;
	bool preferdouble = false, preferlace = false;
	bool lace = false;

	if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate > 85) {
		preferdouble = true;
	} else if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced) {
		preferlace = true;
	}

	if (hz >= 55)
		hz = 60;
	else
		hz = 50;

	newh = h * (currprefs.ntscmode ? 60 : 50) / hz;

	found = nullptr;
	for (cnt = 0; cnt <= abs(newh - h) + 1 && !found; cnt++) {
		for (int dbl = 0; dbl < 2 && !found; dbl++) {
			bool doublecheck = false;
			bool lacecheck = false;
			if (preferdouble && dbl == 0)
				doublecheck = true;
			else if (preferlace && dbl == 0)
				lacecheck = true;

			for (int extra = 1; extra >= -1 && !found; extra--) {
				for (int i = 0; md->DisplayModes[i].inuse && !found; i++) {
					struct PicassoResolution* r = &md->DisplayModes[i];
					if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt)) {
						for (int j = 0; r->refresh[j] > 0; j++) {
							if (doublecheck) {
								if (r->refreshtype[j] & REFRESH_RATE_LACE)
									continue;
								if (r->refresh[j] == hz * 2 + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							} else if (lacecheck) {
								if (!(r->refreshtype[j] & REFRESH_RATE_LACE))
									continue;
								if (r->refresh[j] * 2 == hz + extra) {
									found = r;
									lace = true;
									hz = r->refresh[j];
									break;
								}
							} else {
								if (r->refresh[j] == hz + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	if (found == oldmode && hz == oldhz)
		return true;
	oldmode = found;
	oldhz = hz;
	if (!found) {
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync = 0;
		if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync) {
			set_config_changed();
		}
		write_log(_T("refresh rate changed to %d%s but no matching screenmode found, vsync disabled\n"), hz, lace ? _T("i") : _T("p"));
		return false;
	} else {
		newh = static_cast<int>(found->res.height);
		changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height = newh;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = hz;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = lace;
		if (changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height != currprefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height ||
			changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate != currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate) {
			write_log(_T("refresh rate changed to %d%s, new screenmode %dx%d\n"), hz, lace ? _T("i") : _T("p"), w, newh);
			set_config_changed();
		}
		return true;
	}
}

void vsync_clear()
{
	g_vsync.active = false;
}

int vsync_isdone(frame_time_t* dt)
{
	if (isvsync() == 0)
		return -1;
	if (g_vsync.waitvblankthread_mode <= 0)
		return -2;
	if (dt)
		*dt = g_vsync.wait_vblank_timestamp;
	return g_vsync.active ? 1 : 0;
}

#ifdef PICASSO96

static int modeswitchneeded(const struct AmigaMonitor* mon, const struct winuae_currentmode* wc)
{
	struct vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];

	if (isfullscreen() > 0) {
		/* fullscreen to fullscreen */
		if (mon->screen_is_picasso) {
			if (state->Width < wc->current_width && state->Height < wc->current_height) {
				if (currprefs.gf[GF_RTG].gfx_filter_autoscale == 1 || (currprefs.gf[GF_RTG].gfx_filter_autoscale == 2 && currprefs.rtgallowscaling))
					return 0;
			}
			if (state->Width != wc->current_width ||
				state->Height != wc->current_height)
				return 1;
			if (state->Width == wc->current_width &&
				state->Height == wc->current_height) {
			}
			return 1;
		} else {
			if (mon->currentmode.current_width != wc->current_width ||
				mon->currentmode.current_height != wc->current_height)
				return -1;
		}
	} else if (isfullscreen () == 0) {
		/* windowed to windowed */
		return -2;
	} else {
		/* fullwindow to fullwindow */
		if (mon->screen_is_picasso) {
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale && ((wc->native_width > state->Width && wc->native_height >= state->Height) || (wc->native_height > state->Height && wc->native_width >= state->Width)))
				return -1;
			if (currprefs.rtgallowscaling && (state->Width != wc->native_width || state->Height != wc->native_height))
				return -1;
		}
		return -1;
	}
	return 0;
}

void gfx_set_picasso_state(const int monid, const int on)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct winuae_currentmode wc;
	struct apmode* newmode, * oldmode;
	struct gfx_filterdata* newf, * oldf;
	int mode;

	if (mon->screen_is_picasso == on)
		return;
	mon->screen_is_picasso = on;
#ifdef RETROPLATFORM
	rp_rtg_switch();
#endif
	memcpy(&wc, &mon->currentmode, sizeof(wc));

	newmode = &currprefs.gfx_apmode[on ? 1 : 0];
	oldmode = &currprefs.gfx_apmode[on ? 0 : 1];

	newf = &currprefs.gf[on ? 1 : 0];
	oldf = &currprefs.gf[on ? 0 : 1];

	updatemodes(mon);
	update_gfxparams(mon);

	// if filter changes, need to reset
	mode = 0;
	if (newf->gfx_filter != oldf->gfx_filter)
		mode = -1;
	for (int i = 0; i <= 2 * MAX_FILTERSHADERS; i++) {
		if (_tcscmp(newf->gfx_filtershader[i], oldf->gfx_filtershader[i]))
			mode = -1;
		if (_tcscmp(newf->gfx_filtermask[i], oldf->gfx_filtermask[i]))
			mode = -1;
	}
	const bool differentmonitor = getdisplay(&currprefs, newmode->gfx_display) != getdisplay(&currprefs, oldmode->gfx_display);
	// if screen parameter changes, need to reopen window
	if (newmode->gfx_fullscreen != oldmode->gfx_fullscreen ||
		(newmode->gfx_fullscreen && (
			newmode->gfx_backbuffers != oldmode->gfx_backbuffers ||
			differentmonitor ||
			newmode->gfx_refreshrate != oldmode->gfx_refreshrate ||
			newmode->gfx_strobo != oldmode->gfx_strobo ||
			newmode->gfx_vflip != oldmode->gfx_vflip ||
			newmode->gfx_vsync != oldmode->gfx_vsync))) {
		mode = 1;
	}
	if (mode <= 0) {
		const int m = modeswitchneeded(mon, &wc);
		if (m > 0)
			mode = m;
		if (m < 0 && !mode)
			mode = m;
		if (!mode)
			return;
	}
	if (mode < 0) {
		open_windows(mon, true, true);
	} else {
		open_screen(mon); // reopen everything
	}

#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif
}

void updatepicasso96(struct AmigaMonitor* mon)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	vidinfo->rowbytes = 0;
	vidinfo->pixbytes = 32 / 8;
	vidinfo->rgbformat = 0;
	vidinfo->extra_mem = 1;
	vidinfo->height = mon->currentmode.current_height;
	vidinfo->width = mon->currentmode.current_width;
	vidinfo->offset = 0;
	vidinfo->splitypos = -1;
}

void gfx_set_picasso_modeinfo(const int monid, const RGBFTYPE rgbfmt)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];
	if (!mon->screen_is_picasso)
		return;
	gfx_set_picasso_colors(monid, rgbfmt);
	const int need = modeswitchneeded(mon, &mon->currentmode);
	update_gfxparams(mon);
	updatemodes(mon);
	if (need > 0) {
		open_screen(mon);
	} else if (need < 0) {
		open_windows(mon, true, true);
	}
	state->ModeChanged = false;
#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif
	target_graphics_buffer_update(monid, false);
}


static void gfxmode_reset(int monid)
{
}
#endif

int machdep_init()
{
	for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
		struct AmigaMonitor* mon = &AMonitors[i];
		struct amigadisplay* ad = &adisplays[i];
		mon->monitor_id = i;
		ad->picasso_requested_on = false;
		ad->picasso_on = false;
		mon->screen_is_picasso = 0;
		memset(&mon->currentmode, 0, sizeof(mon->currentmode));
	}
#ifdef LOGITECHLCD
	lcd_open();
#endif
	return 1;
}

void machdep_free()
{
#ifdef LOGITECHLCD
	lcd_close();
#endif
}

int graphics_init(bool mousecapture)
{
	// Skip all graphics initialization if running in headless mode
	if (currprefs.headless) {
		write_log("Headless mode: Skipping graphics initialization.\n");
		g_vsync.wait_vblank_timestamp = read_processor_time();
		update_pixel_format();
		if (amiga_surface == nullptr) {
			amiga_surface = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, pixel_format);
		}
		return amiga_surface != nullptr;
	}

	g_vsync.wait_vblank_timestamp = read_processor_time();
	update_pixel_format();
	gfxmode_reset(0);
	if (open_windows(&AMonitors[0], mousecapture, false)) {
		if (currprefs.monitoremu_mon > 0 && currprefs.monitoremu) {
			gfxmode_reset(currprefs.monitoremu_mon);
			open_windows(&AMonitors[currprefs.monitoremu_mon], mousecapture, false);
		}
		return true;
	}
	return false;
}

int graphics_setup()
{
#ifdef PICASSO96
	InitPicasso96(0);
#endif
	return 1;
}

void graphics_leave()
{
	for (int i = 0; i < MAX_AMIGAMONITORS; i++) 
	{
		close_windows(&AMonitors[i]);
	}
}



// Helper: Compute scaled dimensions for aspect ratio correction
// RTG modes use 1:1 scaling, Native modes scale based on resolution settings
static void compute_scaled_dimensions(const int w, const int h, const bool is_rtg, int& scaled_w, int& scaled_h)
{
	if (is_rtg) {
		scaled_w = w;
		scaled_h = h;
		return;
	}

	scaled_w = w;
	scaled_h = h;

	if (currprefs.gfx_vresolution == VRES_NONDOUBLE) {
		if (currprefs.gfx_resolution == RES_HIRES || currprefs.gfx_resolution == RES_SUPERHIRES)
			scaled_h *= 2;
	} else {
		if (currprefs.gfx_resolution == RES_LORES)
			scaled_w *= 2;
	}

	if (currprefs.ntscmode)
		scaled_h = scaled_h * 6 / 5;
}

// Helper: Configure render_quad and crop_rect based on mode and crop settings
// For RTG: direct 1:1 mapping
// For Native: respects manual crop settings, auto_crop is handled elsewhere
static void configure_render_rects(const int w, const int h, const int scaled_w, const int scaled_h, const bool is_rtg)
{
	if (is_rtg) {
		render_quad = { dx, dy, w, h };
		crop_rect = { dx, dy, w, h };
		return;
	}

	// Native mode with manual crop
	if (currprefs.gfx_manual_crop) {
		render_quad = { dx, dy, scaled_w, scaled_h };
		crop_rect = {
			currprefs.gfx_horizontal_offset,
			currprefs.gfx_vertical_offset,
			(currprefs.gfx_manual_crop_width > 0) ? currprefs.gfx_manual_crop_width : w,
			(currprefs.gfx_manual_crop_height > 0) ? currprefs.gfx_manual_crop_height : h
		};
	}
	// Native mode without auto_crop (auto_crop is handled in auto_crop_image())
	else if (!currprefs.gfx_auto_crop) {
		render_quad = { dx, dy, scaled_w, scaled_h };
		crop_rect = { dx, dy, w, h };
	}
}

bool target_graphics_buffer_update(const int monid, const bool force)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct vidbuf_description* avidinfo = &adisplays[monid].gfxvidinfo;
	const struct picasso96_state_struct* state = &picasso96_state[monid];
	struct vidbuffer *vb = NULL, *vbout = NULL;

	int w, h;

	if (mon->screen_is_picasso) {
		w = state->Width;
		h = state->Height;
		update_pixel_format();
	} else {
		update_pixel_format();
		vb = avidinfo->inbuffer;
		vbout = avidinfo->outbuffer;
		if (!vb) {
			return false;
		}
		w = vb->outwidth;
		h = vb->outheight;
	}

	if (!force && oldtex_w[monid] == w && oldtex_h[monid] == h && oldtex_rtg[monid] == mon->screen_is_picasso && amiga_surface && amiga_surface->format->format == pixel_format) {
		bool skip_update = true;
		if (mon->screen_is_picasso) {
			uae_u8* rtg_ptr = p96_get_render_buffer_pointer(mon->monitor_id);
			if (rtg_ptr && amiga_surface->pixels != rtg_ptr) {
				skip_update = false;
			} else {
				// write_log("GFX: Skipping update. Pointers match: %p\n", rtg_ptr);
			}
		}

		if (skip_update) {
			if (SDL2_alloctexture(mon->monitor_id, -w, -h))
			{
				//osk_setup(monid, -2);
				if (vbout) {
					vbout->width_allocated = w;
					vbout->height_allocated = h;
				}
				if (force) {
					mon->full_render_needed = true;
				}
				return false;
			}
		}
	}

	if (!w || !h) {
		oldtex_w[monid] = w;
		oldtex_h[monid] = h;
		oldtex_rtg[monid] = mon->screen_is_picasso;
		
		// Even if buffer dimensions aren't ready yet, we need to ensure the shader is created
		// for native mode. Use the amiga_surface dimensions that doInit already set up.
		// This is critical for RTG→Native switches where the shader must be recreated for native mode.
#ifdef USE_OPENGL
		if (!mon->screen_is_picasso && amiga_surface) {
			SDL2_alloctexture(mon->monitor_id, amiga_surface->w, amiga_surface->h);
		}
#endif
		return false;
	}

	// Ensure amiga_surface is in sync with the texture size and format
	// Zero-Copy Eligibility Check
	uae_u8* rtg_render_ptr = p96_get_render_buffer_pointer(mon->monitor_id);
	bool is_zero_copy_eligible = false;
	
	if (mon->screen_is_picasso && currprefs.rtgboards[0].rtgmem_type < GFXBOARD_HARDWARE && currprefs.rtg_zerocopy) {
		int p96_bpp = state->BytesPerPixel;

		int host_bpp = SDL_BYTESPERPIXEL(pixel_format);

		if (rtg_render_ptr != nullptr && p96_bpp == host_bpp && (pixel_format == SDL_PIXELFORMAT_ABGR8888 || pixel_format == SDL_PIXELFORMAT_RGB565)) {
			is_zero_copy_eligible = true;
		}
	}

	// Ensure amiga_surface is in sync with the texture size, format, and memory pointer (if zero-copy)
	bool recreate_surface = (amiga_surface == nullptr || amiga_surface->w != w || amiga_surface->h != h || amiga_surface->format->format != pixel_format);

	// For zero-copy, also check if pitch matches the RTG state
	if (!recreate_surface && is_zero_copy_eligible && amiga_surface->pitch != state->BytesPerRow) {
		recreate_surface = true;
	}
	if (amiga_surface && is_zero_copy_eligible && amiga_surface->pixels != (void*)rtg_render_ptr) {
		recreate_surface = true;
	}
	// If Zero-Copy is disabled, but we are still pointing to VRAM, we must recreate the surface
	if (amiga_surface && !is_zero_copy_eligible && rtg_render_ptr && amiga_surface->pixels == rtg_render_ptr) {
		recreate_surface = true;
	}

	if (recreate_surface) {
		if (amiga_surface) {
			SDL_FreeSurface(amiga_surface);
			amiga_surface = nullptr;
		}
		const int surface_depth = (pixel_format == SDL_PIXELFORMAT_RGB565 || pixel_format == SDL_PIXELFORMAT_RGB555) ? 16 : 32;

		if (is_zero_copy_eligible) {
			// Zero-Copy: Create surface from existing memory (rtg_render_ptr guaranteed non-null)
			amiga_surface = SDL_CreateRGBSurfaceWithFormatFrom(rtg_render_ptr, w, h, surface_depth, state->BytesPerRow, pixel_format);
		} else {
			// Normal copy: Create fresh surface
			amiga_surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, surface_depth, pixel_format);
		}

		if (amiga_surface) {
			SDL_SetSurfaceBlendMode(amiga_surface, SDL_BLENDMODE_NONE);
		} else {
			write_log("!!! Failed to create amiga_surface.\n");
			return false;
		}
		mon->full_render_needed = true;
		struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
		vidinfo->pixbytes = amiga_surface->format->BytesPerPixel;
#ifndef PICASSO_STATE_SETDAC
#define PICASSO_STATE_SETDAC 8
#endif
		atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETDAC);
	}

	// Allocate/update texture (single call regardless of surface recreation)
	if (!SDL2_alloctexture(mon->monitor_id, w, h)) {
		return false;
	}

	// Update video buffer dimensions (consolidated)
	if (vbout) {
		vbout->width_allocated = w;
		vbout->height_allocated = h;
	}

	if (avidinfo->inbuffer != avidinfo->outbuffer) {
		if (avidinfo->outbuffer) {
			avidinfo->outbuffer->inwidth = w;
			avidinfo->outbuffer->inheight = h;
			avidinfo->outbuffer->width_allocated = w;
			avidinfo->outbuffer->height_allocated = h;
		}
	}

	oldtex_w[monid] = w;
	oldtex_h[monid] = h;
	oldtex_rtg[monid] = mon->screen_is_picasso;
	//osk_setup(monid, -2);

	write_log(_T("Buffer %d size (%d*%d) %s\n"), monid, w, h, mon->screen_is_picasso ? _T("RTG") : _T("Native"));

	// Compute scaled dimensions for aspect ratio correction
	int scaled_width, scaled_height;
	compute_scaled_dimensions(w, h, mon->screen_is_picasso, scaled_width, scaled_height);

#ifdef USE_OPENGL
	configure_render_rects(w, h, scaled_width, scaled_height, mon->screen_is_picasso);
	set_scaling_option(monid, &currprefs, scaled_width, scaled_height);
#else
	if (!mon->amiga_renderer)
		return false;

	SDL_RenderSetLogicalSize(mon->amiga_renderer, scaled_width, scaled_height);
	configure_render_rects(w, h, scaled_width, scaled_height, mon->screen_is_picasso);
	set_scaling_option(monid, &currprefs, scaled_width, scaled_height);
#endif

	return true;
}

static void updatedisplayarea2(const int monid)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->screen_is_initialized)
		return;
	SDL2_refresh(monid);
}

void updatedisplayarea(const int monid)
{
	set_custom_limits(-1, -1, -1, -1, false);
	if (monid >= 0) {
		updatedisplayarea2(monid);
	} else {
		for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
			updatedisplayarea2(i);
		}
	}
}

void updatewinfsmode(const int monid, struct uae_prefs* p)
{
	struct MultiDisplay* md;
	struct amigadisplay* ad = &adisplays[monid];

	fixup_prefs_dimensions(p);
	int fs = isfullscreen_2(p);
	if (fs != 0) {
		p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_fs;
	} else {
		p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_win;
	}

	int* wfw = &wasfs[ad->picasso_on ? 1 : 0];
	const TCHAR* wfwname = wasfsname[ad->picasso_on ? 1 : 0];
	if (fs != *wfw && fs != 0) {
		*wfw = fs;
		regsetint(NULL, wfwname, *wfw);
	}

	md = getdisplay(p, monid);
	set_config_changed();

#ifdef AMIBERRY
	const AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->screen_is_picasso)
		force_auto_crop = true;
#endif
}

bool toggle_3d_debug(void)
{
	if (isvsync_chipset() < 0) {
		beamracer_debug = !beamracer_debug;
		//if (D3D_debug) {
		//	D3D_debug(0, beamracer_debug);
		//}
		reset_drawing();
		return true;
	}
	return false;
}

int rtg_index = -1;

// -2 = default
// -1 = prev
// 0 = chipset
// 1..4 = rtg
// 5 = next
bool toggle_rtg(const int monid, const int mode)
{
	struct amigadisplay* ad = &adisplays[monid];

	int old_index = rtg_index;

	if (monid > 0) {
		return true;
	}

	if (mode < -1 && rtg_index >= 0)
		return true;

	for (;;) {
		if (mode == -1) {
			rtg_index--;
		} else if (mode >= 0 && mode <= MAX_RTG_BOARDS) {
			rtg_index = mode - 1;
		} else {
			rtg_index++;
		}
		if (rtg_index >= MAX_RTG_BOARDS) {
			rtg_index = -1;
		} else if (rtg_index < -1) {
			rtg_index = MAX_RTG_BOARDS - 1;
		}
		if (rtg_index < 0) {
			if (ad->picasso_on) {
				gfxboard_rtg_disable(monid, old_index);
				ad->picasso_requested_on = false;
				statusline_add_message(STATUSTYPE_DISPLAY, _T("Chipset display"));
				set_config_changed();
				return false;
			}
			return false;
		}
		struct rtgboardconfig* r = &currprefs.rtgboards[rtg_index];
		if (r->rtgmem_size > 0 && r->monitor_id == monid) {
			if (r->rtgmem_type >= GFXBOARD_HARDWARE) {
				int idx = gfxboard_toggle(r->monitor_id, rtg_index, mode >= -1);
				if (idx >= 0) {
					rtg_index = idx;
					return true;
				}
				if (idx < -1) {
					rtg_index = -1;
					return false;
				}
			} else {
				gfxboard_toggle(r->monitor_id, -1, -1);
				if (mode < -1)
					return true;
				devices_unsafeperiod();
				gfxboard_rtg_disable(monid, old_index);
				// can always switch from RTG to custom
				if (ad->picasso_requested_on && ad->picasso_on) {
					ad->picasso_requested_on = false;
					rtg_index = -1;
					set_config_changed();
					return true;
				}
				if (ad->picasso_on)
					return false;
				// can only switch from custom to RTG if there is some mode active
				if (picasso_is_active(r->monitor_id)) {
					picasso_enablescreen(r->monitor_id, 1);
					ad->picasso_requested_on = true;
					statusline_add_message(STATUSTYPE_DISPLAY, _T("RTG %d: %s"), rtg_index + 1, _T("UAEGFX"));
					set_config_changed();
					return true;
				}
			}
		}
		if (mode >= 0 && mode <= MAX_RTG_BOARDS) {
			rtg_index = old_index;
			return false;
		}
	}
	//return false;
}

void close_rtg(const int monid, const bool reset)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	close_windows(mon);
	if (reset) {
		struct amigadisplay* ad = &adisplays[monid];
		mon->screen_is_picasso = false;
		ad->picasso_on = false;
		ad->picasso_requested_on = false;
		ad->picasso_requested_forced_on = false;
	}
}

void toggle_fullscreen(const int monid, const int mode)
{
	const amigadisplay* ad = &adisplays[monid];
	auto* p = ad->picasso_on ? &changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen : &changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen;
	int* wfw = &wasfs[ad->picasso_on ? 1 : 0];
	auto v = *p;
	//static int prevmode = -1;

	if (mode < 0) {
		// fullwindow->window->fullwindow.
		// fullscreen->window->fullscreen.
		// window->fullscreen->window.
		if (v == GFX_FULLWINDOW) {
			//prevmode = v;
			*wfw = -1;
			v = GFX_WINDOW;
		} else if (v == GFX_WINDOW) {
			if (*wfw >= 0) {
				v = GFX_FULLSCREEN;
			} else {
				v = GFX_FULLWINDOW;
			}
		} else if (v == GFX_FULLSCREEN) {
			//prevmode = v;
			*wfw = 1;
			v = GFX_WINDOW;
		}
	} else if (mode == 0) {
		//prevmode = v;
		// fullscreen <> window
		if (v == GFX_FULLSCREEN)
			v = GFX_WINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 1) {
		//prevmode = v;
		// fullscreen <> fullwindow
		if (v == GFX_FULLSCREEN)
			v = GFX_FULLWINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 2) {
		//prevmode = v;
		// window <> fullwindow
		if (v == GFX_FULLWINDOW)
			v = GFX_WINDOW;
		else
			v = GFX_FULLWINDOW;
	} else if (mode == 10) {
		v = GFX_WINDOW;
	}
	*p = v;
	devices_unsafeperiod();
	updatewinfsmode(monid, &changed_prefs);
}


#ifdef AMIBERRY
void auto_crop_image()
{
	const AmigaMonitor* mon = &AMonitors[0];
	static bool last_autocrop;

	if (currprefs.gfx_auto_crop)
	{
		static int last_cw = 0, last_ch = 0, last_cx = 0, last_cy = 0;
		int cw, ch, cx, cy, crealh = 0;
		int hres = currprefs.gfx_resolution;
		int vres = currprefs.gfx_vresolution;
		get_custom_limits(&cw, &ch, &cx, &cy, &crealh, &hres, &vres);

		if (!force_auto_crop && last_autocrop == currprefs.gfx_auto_crop && last_cw == cw && last_ch == ch && last_cx == cx && last_cy == cy)
		{
			return;
		}

		last_cw = cw;
		last_ch = ch;
		last_cx = cx;
		last_cy = cy;
		force_auto_crop = false;

		int width = cw;
		int height = ch;
		if (currprefs.gfx_vresolution == VRES_NONDOUBLE)
		{
			if (currprefs.gfx_resolution == RES_HIRES || currprefs.gfx_resolution == RES_SUPERHIRES)
				height *= 2;
		}
		else
		{
			// Add missing LORES width doubling to match compute_scaled_dimensions()
			if (currprefs.gfx_resolution == RES_LORES)
				width *= 2;
		}

		if (currprefs.ntscmode)
			height = height * 6 / 5;

		if (currprefs.gfx_correct_aspect == 0)
		{
			width = sdl_mode.w;
			height = sdl_mode.h;
		}
#ifdef USE_OPENGL
		render_quad = { dx, dy, width, height };
		crop_rect = { cx, cy, cw, ch };
		if (amiga_surface) {
			if (crop_rect.x < 0) crop_rect.x = 0;
			if (crop_rect.y < 0) crop_rect.y = 0;
			if (crop_rect.x >= amiga_surface->w) crop_rect.x = 0;
			if (crop_rect.y >= amiga_surface->h) crop_rect.y = 0;
			if (crop_rect.w <= 0 || crop_rect.x + crop_rect.w > amiga_surface->w)
				crop_rect.w = amiga_surface->w - crop_rect.x;
			if (crop_rect.h <= 0 || crop_rect.y + crop_rect.h > amiga_surface->h)
				crop_rect.h = amiga_surface->h - crop_rect.y;
		}

		if (vkbd_allowed(0))
		{
			vkbd_update_position_from_texture();
		}
#else
		SDL_RenderSetLogicalSize(mon->amiga_renderer, width, height);
		render_quad = { dx, dy, width, height };
		crop_rect = { cx, cy, cw, ch };
		if (amiga_surface) {
			if (crop_rect.x < 0) crop_rect.x = 0;
			if (crop_rect.y < 0) crop_rect.y = 0;
			if (crop_rect.x >= amiga_surface->w) crop_rect.x = 0;
			if (crop_rect.y >= amiga_surface->h) crop_rect.y = 0;
			if (crop_rect.w <= 0 || crop_rect.x + crop_rect.w > amiga_surface->w)
				crop_rect.w = amiga_surface->w - crop_rect.x;
			if (crop_rect.h <= 0 || crop_rect.y + crop_rect.h > amiga_surface->h)
				crop_rect.h = amiga_surface->h - crop_rect.y;
		}

		if (vkbd_allowed(0))
		{
			vkbd_update_position_from_texture();
		}
#endif
	}

	last_autocrop = currprefs.gfx_auto_crop;
}
#endif

unsigned long target_lastsynctime()
{
	return last_synctime;
}

static int save_png(const SDL_Surface* surface, const std::string& path)
{
	return gfx_platform_save_png(surface, path);
}

bool create_screenshot()
{
	if (current_screenshot != nullptr)
	{
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}
	return gfx_platform_create_screenshot(amiga_surface, &current_screenshot);
}

int save_thumb(const std::string& path)
{
	auto ret = 0;
	if (current_screenshot != nullptr)
	{
		ret = save_png(current_screenshot, path);
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}
	return ret;
}

void screenshot(int monid, int mode, int doprepare)
{
	std::string filename;

	if (!create_screenshot())
	{
		write_log(_T("Failed to create screenshot\n"));
		return;
	}

	if (strlen(currprefs.floppyslots[0].df) > 0)
		filename = extract_filename(currprefs.floppyslots[0].df);
	else if (currprefs.cdslots[0].inuse && strlen(currprefs.cdslots[0].name) > 0)
		filename = extract_filename(currprefs.cdslots[0].name);
	else
		filename = "default.uae";

	screenshot_filename = get_screenshot_path();
	screenshot_filename += filename;
	screenshot_filename = remove_file_extension(screenshot_filename);
	screenshot_filename += ".png";

	save_thumb(screenshot_filename);
}

