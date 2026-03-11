/*
 * sdl_renderer.cpp - SDL2 software renderer backend implementation of IRenderer
 *
 * Owns SDL2 texture state and implements frame rendering/presentation
 * using SDL2_Renderer (SDL_RenderCopy, SDL_RenderPresent).
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "sysdeps.h"

#ifndef USE_OPENGL

#include "options.h"
#include "xwin.h"
#include "picasso96.h"
#include "statusline.h"

#include "sdl_renderer.h"
#include "amiberry_gfx.h"
#include "gfx_platform_internal.h"
#include "vkbd/vkbd.h"
#include "on_screen_joystick.h"

#ifdef AMIBERRY

// External globals (defined in amiberry_gfx.cpp)
extern SDL_Surface* amiga_surface;

SDLRenderer* get_sdl_renderer()
{
	return dynamic_cast<SDLRenderer*>(g_renderer.get());
}

// --- Context lifecycle ---

bool SDLRenderer::init_context(SDL_Window* /*window*/)
{
	// SDL2 software path: no separate context needed
	return true;
}

void SDLRenderer::destroy_context()
{
	// SDL2 software path: nothing to do
}

bool SDLRenderer::has_context() const
{
	// SDL2 software path: always considered "valid" when a renderer exists
	return true;
}

// --- Window creation support ---

Uint32 SDLRenderer::get_window_flags() const
{
	return 0; // No special flags needed for SDL2 software rendering
}

bool SDLRenderer::set_context_attributes(int /*mode*/)
{
	// SDL2 software path: no GL attributes to set
	return true;
}

bool SDLRenderer::create_platform_renderer(AmigaMonitor* mon)
{
	if (mon->amiga_renderer == nullptr)
	{
		Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
		mon->amiga_renderer = SDL_CreateRenderer(mon->amiga_window, -1, renderer_flags);
		if (mon->amiga_renderer == nullptr) {
			write_log("Unable to create a renderer: %s\n", SDL_GetError());
			return false;
		}
	}
	return true;
}

void SDLRenderer::destroy_platform_renderer(AmigaMonitor* mon)
{
	if (mon->amiga_renderer)
	{
#if defined(__ANDROID__)
		// Don't destroy the renderer on Android, as we reuse it
#elif defined(_WIN32)
		if (mon->gui_renderer == mon->amiga_renderer) {
			// GUI is sharing this renderer - don't destroy
		} else {
			SDL_DestroyRenderer(mon->amiga_renderer);
			mon->amiga_renderer = nullptr;
		}
#else
		SDL_DestroyRenderer(mon->amiga_renderer);
		mon->amiga_renderer = nullptr;
#endif
	}
}

// --- Texture / shader allocation ---

bool SDLRenderer::alloc_texture(int monid, int w, int h)
{
	if (currprefs.headless) return true;
	if (w == 0 || h == 0) return false;
	if (gfx_platform_skip_alloctexture(monid, w, h)) return true;

	if (w < 0 || h < 0)
	{
		if (m_amiga_texture)
		{
			int width, height;
			Uint32 format;
			SDL_QueryTexture(m_amiga_texture, &format, nullptr, &width, &height);
			if (width == -w && height == -h && format == pixel_format)
			{
				set_scaling(monid, &currprefs, width, height);
				return true;
			}
		}
		return false;
	}

	if (m_amiga_texture != nullptr)
		SDL_DestroyTexture(m_amiga_texture);

	AmigaMonitor* mon = &AMonitors[monid];
	m_amiga_texture = SDL_CreateTexture(mon->amiga_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, amiga_surface->w, amiga_surface->h);
	if (m_amiga_texture != nullptr)
		SDL_SetTextureBlendMode(m_amiga_texture, SDL_BLENDMODE_NONE);
	return m_amiga_texture != nullptr;
}

// Helper to decide between Linear (smooth) and Nearest Neighbor (pixelated) scaling
static bool ar_is_exact(const SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}

void SDLRenderer::set_scaling(int monid, const uae_prefs* p, int w, int h)
{
	AmigaMonitor* mon = &AMonitors[monid];
	if (mon->amiga_renderer)
		SDL_RenderSetLogicalSize(mon->amiga_renderer, w, h);

	if (currprefs.headless) return;

	auto scale_quality = "nearest";
	SDL_bool integer_scale = SDL_FALSE;

	switch (p->scaling_method) {
	case -1: // Auto
		if (!ar_is_exact(&sdl_mode, w, h))
			scale_quality = "linear";
		break;
	case 0: scale_quality = "nearest"; break;
	case 1: scale_quality = "linear"; break;
	case 2: scale_quality = "nearest"; integer_scale = SDL_TRUE; break;
	default: scale_quality = "linear"; break;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale_quality);
	SDL_RenderSetIntegerScale(mon->amiga_renderer, integer_scale);
}

// --- OSD texture synchronization ---

void SDLRenderer::sync_osd_texture(int monid, int led_width, int led_height)
{
	AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->amiga_renderer)
		return;

	// Destroy and recreate texture if dimensions changed
	if (mon->statusline_texture) {
		int tw, th;
		SDL_QueryTexture(mon->statusline_texture, nullptr, nullptr, &tw, &th);
		if (tw != led_width || th != led_height) {
			SDL_DestroyTexture(mon->statusline_texture);
			mon->statusline_texture = nullptr;
		}
	}

	// Create texture if needed
	if (!mon->statusline_texture) {
		mon->statusline_texture = SDL_CreateTexture(mon->amiga_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, led_width, led_height);
		if (mon->statusline_texture)
			SDL_SetTextureBlendMode(mon->statusline_texture, SDL_BLENDMODE_BLEND);
	}

	// Upload surface pixels to texture
	if (mon->statusline_texture && mon->statusline_surface)
		SDL_UpdateTexture(mon->statusline_texture, nullptr, mon->statusline_surface->pixels, mon->statusline_surface->pitch);
}

// --- VSync ---

void SDLRenderer::update_vsync(int /*monid*/)
{
	// SDL2 software path: VSync handled by renderer flags at creation time
}

// --- Frame rendering ---

bool SDLRenderer::render_frame(int monid, int mode, int immediate)
{
	if (m_amiga_texture && amiga_surface)
	{
		const amigadisplay* ad = &adisplays[monid];
		AmigaMonitor* mon = &AMonitors[monid];

		// Ensure the draw color is black for clearing
		SDL_SetRenderDrawColor(mon->amiga_renderer, 0, 0, 0, 255);
		SDL_RenderClear(mon->amiga_renderer);

		// If a full render is needed or there are no specific dirty rects, update the whole texture.
		if (mon->full_render_needed || mon->dirty_rects.empty()) {
			if (amiga_surface) {
				SDL_UpdateTexture(m_amiga_texture, NULL, amiga_surface->pixels, amiga_surface->pitch);
			}
		} else {
			// Otherwise, update only the collected dirty rectangles.
			const int bpp = amiga_surface->format->BytesPerPixel;
			const auto* base = static_cast<const uae_u8*>(amiga_surface->pixels);
			const int pitch = amiga_surface->pitch;
			for (const auto& rect : mon->dirty_rects) {
				SDL_UpdateTexture(m_amiga_texture, &rect, base + rect.y * pitch + rect.x * bpp, pitch);
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

		SDL_RenderCopyEx(mon->amiga_renderer, m_amiga_texture, p_crop, p_quad, 0, nullptr, SDL_FLIP_NONE);

		// Render Software Cursor Overlay for RTG (when using relative mouse mode)
		if (ad->picasso_on && p96_uses_software_cursor()) {
			if (p96_cursor_needs_update() || !m_cursor_overlay_texture) {
				SDL_Surface* s = p96_get_cursor_overlay_surface();
				if (s) {
					if (m_cursor_overlay_texture)
						SDL_DestroyTexture(m_cursor_overlay_texture);
					m_cursor_overlay_texture = SDL_CreateTextureFromSurface(mon->amiga_renderer, s);
					if (m_cursor_overlay_texture)
						SDL_SetTextureBlendMode(m_cursor_overlay_texture, SDL_BLENDMODE_BLEND);
				}
			}

			if (m_cursor_overlay_texture) {
				int cx, cy, cw, ch;
				p96_get_cursor_position(&cx, &cy);
				p96_get_cursor_dimensions(&cw, &ch);

				// Renderer logical size matches RTG resolution, so 1:1 mapping
				SDL_Rect dst_cursor = { cx, cy, cw, ch };
				SDL_RenderCopy(mon->amiga_renderer, m_cursor_overlay_texture, nullptr, &dst_cursor);
			}
		}

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

		render_vkbd(monid);
		render_onscreen_joystick(monid);

		return true;
	}

	return false;
}

// --- Virtual keyboard / on-screen joystick wrappers ---

void SDLRenderer::render_vkbd(int monid)
{
	if (vkbd_allowed(monid))
	{
		vkbd_redraw();
	}
}

void SDLRenderer::render_onscreen_joystick(int monid)
{
	if (on_screen_joystick_is_enabled())
	{
		AmigaMonitor* mon = &AMonitors[monid];
		on_screen_joystick_redraw(mon->amiga_renderer);
	}
}

void SDLRenderer::present_frame(int monid, int mode)
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
	if (m_vsync.waitvblankthread_mode <= 0)
		m_vsync.wait_vblank_timestamp = read_processor_time();
}

// Shader management and bezel overlay: uses IRenderer defaults (no-op)

// --- Drawable size query ---

void SDLRenderer::get_gfx_offset(int monid, float src_w, float src_h, float src_x, float src_y,
	float* dx, float* dy, float* mx, float* my)
{
	const AmigaMonitor* mon = &AMonitors[monid];

	*dx = 0; *dy = 0; *mx = 1.0f; *my = 1.0f;

	if (mon->currentmode.flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		if (currprefs.gfx_auto_crop) {
			*dx -= src_x;
			*dy -= src_y;
		}
		if (!(mon->scalepicasso && mon->screen_is_picasso) &&
			!(mon->currentmode.fullfill && (mon->currentmode.current_width > mon->currentmode.native_width ||
			                                 mon->currentmode.current_height > mon->currentmode.native_height))) {
			*dx += (mon->currentmode.native_width - mon->currentmode.current_width) / 2;
			*dy += (mon->currentmode.native_height - mon->currentmode.current_height) / 2;
		}
	}
}

void SDLRenderer::get_drawable_size(SDL_Window* /*w*/, int* width, int* height)
{
	// SDL2 software path: use renderer output size
	AmigaMonitor* mon = &AMonitors[0];
	if (mon->amiga_renderer) {
		SDL_GetRendererOutputSize(mon->amiga_renderer, width, height);
	}
}

// get_vblank_timestamp() uses base class default (returns m_vsync.wait_vblank_timestamp)

// --- GUI context transitions ---

void SDLRenderer::prepare_gui_sharing(AmigaMonitor* mon)
{
	if (mon->amiga_renderer && !mon->gui_renderer)
		mon->gui_renderer = mon->amiga_renderer;
}

// --- Cleanup ---

void SDLRenderer::close_hwnds_cleanup(AmigaMonitor* mon)
{
	if (m_amiga_texture)
	{
		SDL_DestroyTexture(m_amiga_texture);
		m_amiga_texture = nullptr;
	}
	if (m_cursor_overlay_texture)
	{
		SDL_DestroyTexture(m_cursor_overlay_texture);
		m_cursor_overlay_texture = nullptr;
	}
}

#endif // AMIBERRY
#endif // !USE_OPENGL
