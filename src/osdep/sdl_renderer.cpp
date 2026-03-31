/*
 * sdl_renderer.cpp - SDL3 software renderer backend implementation of IRenderer
 *
 * Owns SDL3 texture state and implements frame rendering/presentation
 * using SDL3_Renderer (SDL_RenderTexture, SDL_RenderPresent).
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
#include "display_modes.h"
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
	// SDL software path: no separate context needed
	return true;
}

void SDLRenderer::destroy_context()
{
	// SDL software path: nothing to do
}

bool SDLRenderer::has_context() const
{
	// SDL software path: always considered "valid" when a renderer exists
	return true;
}

// --- Window creation support ---

SDL_WindowFlags SDLRenderer::get_window_flags() const
{
#if !defined(__ANDROID__)
	return SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
	return 0;
#endif
}

bool SDLRenderer::set_context_attributes(int /*mode*/)
{
	// SDL software path: no GL attributes to set
	return true;
}

bool SDLRenderer::create_platform_renderer(AmigaMonitor* mon)
{
	if (mon->amiga_renderer == nullptr)
	{
		mon->amiga_renderer = SDL_CreateRenderer(mon->amiga_window, NULL);
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
			float tex_fw, tex_fh;
			SDL_GetTextureSize(m_amiga_texture, &tex_fw, &tex_fh);
			int width = static_cast<int>(tex_fw);
			int height = static_cast<int>(tex_fh);
			if (width == -w && height == -h && m_texture_format == pixel_format)
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
	m_amiga_texture = SDL_CreateTexture(mon->amiga_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (m_amiga_texture != nullptr)
	{
		SDL_SetTextureBlendMode(m_amiga_texture, SDL_BLENDMODE_NONE);
		m_texture_format = pixel_format;
	}
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
		SDL_SetRenderLogicalPresentation(mon->amiga_renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	if (currprefs.headless) return;

	SDL_ScaleMode scale_mode = SDL_SCALEMODE_NEAREST;
	bool integer_scale = false;

	switch (p->scaling_method) {
	case -1: // Auto
		if (!ar_is_exact(&sdl_mode, w, h))
			scale_mode = SDL_SCALEMODE_LINEAR;
		break;
	case 0: scale_mode = SDL_SCALEMODE_NEAREST; break;
	case 1: scale_mode = SDL_SCALEMODE_LINEAR; break;
	case 2: scale_mode = SDL_SCALEMODE_NEAREST; integer_scale = true; break;
	default: scale_mode = SDL_SCALEMODE_LINEAR; break;
	}

	// SDL3: scale mode is per-texture, set on the amiga texture
	if (m_amiga_texture)
		SDL_SetTextureScaleMode(m_amiga_texture, scale_mode);

	// SDL3: integer scaling via logical presentation mode
	if (integer_scale)
		SDL_SetRenderLogicalPresentation(mon->amiga_renderer, w, h, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
	else
		SDL_SetRenderLogicalPresentation(mon->amiga_renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

// --- OSD texture synchronization ---

void SDLRenderer::sync_osd_texture(int monid, int led_width, int led_height)
{
	AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->amiga_renderer)
		return;

	// Destroy and recreate texture if dimensions changed
	if (mon->statusline_texture) {
		float tw_f, th_f;
		SDL_GetTextureSize(mon->statusline_texture, &tw_f, &th_f);
		int tw = static_cast<int>(tw_f);
		int th = static_cast<int>(th_f);
		if (tw != led_width || th != led_height) {
			SDL_DestroyTexture(mon->statusline_texture);
			mon->statusline_texture = nullptr;
		}
	}

	// Create texture if needed
	if (!mon->statusline_texture) {
		mon->statusline_texture = SDL_CreateTexture(mon->amiga_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, led_width, led_height);
		if (mon->statusline_texture) {
			SDL_SetTextureBlendMode(mon->statusline_texture, SDL_BLENDMODE_BLEND);
			SDL_SetTextureScaleMode(mon->statusline_texture, SDL_SCALEMODE_NEAREST);
		}
	}

	// Upload surface pixels to texture
	if (mon->statusline_texture && mon->statusline_surface)
		SDL_UpdateTexture(mon->statusline_texture, nullptr, mon->statusline_surface->pixels, mon->statusline_surface->pitch);
}

// --- VSync ---

void SDLRenderer::update_vsync(int /*monid*/)
{
	// SDL software path: VSync handled by renderer flags at creation time
}

// --- Frame rendering ---

bool SDLRenderer::render_frame(int monid, int mode, int immediate)
 {
	SDL_Surface* surface = get_amiga_surface(monid);
	if (m_amiga_texture && surface) {
	 const amigadisplay* ad = &adisplays[monid];
        AmigaMonitor* mon = &AMonitors[monid];

        // Ensure the draw color is black for clearing
        SDL_SetRenderDrawColor(mon->amiga_renderer, 0, 0, 0, 255);
        SDL_RenderClear(mon->amiga_renderer);

        // If a full render is needed or there are no specific dirty rects, update the whole texture.
        if (mon->full_render_needed || mon->dirty_rects.empty()) {
            if (surface) {
                SDL_UpdateTexture(m_amiga_texture, NULL, surface->pixels, surface->pitch);
            }
        } else {
            // Otherwise, update only the collected dirty rectangles.
            const auto* fmt = SDL_GetPixelFormatDetails(surface->format);
            const int bpp = fmt ? fmt->bytes_per_pixel : 4;
            const auto* base = static_cast<const uae_u8*>(surface->pixels);
            const int pitch = surface->pitch;
            for (const auto& rect : mon->dirty_rects) {
                SDL_UpdateTexture(m_amiga_texture, &rect, base + rect.y * pitch + rect.x * bpp, pitch);
            }
        }

        // Clear the dirty rects list for the next frame.
        mon->dirty_rects.clear();
        mon->full_render_needed = false;

        SDL_FRect f_crop = { static_cast<float>(crop_rect.x), static_cast<float>(crop_rect.y),
            static_cast<float>(crop_rect.w), static_cast<float>(crop_rect.h) };
        SDL_FRect f_quad = { static_cast<float>(render_quad.x), static_cast<float>(render_quad.y),
            static_cast<float>(render_quad.w), static_cast<float>(render_quad.h) };

        if (ad->picasso_on) {
            f_crop = { 0.0f, 0.0f, static_cast<float>(surface->w), static_cast<float>(surface->h) };
            f_quad = f_crop;

            int lw, lh;
            SDL_GetRenderLogicalPresentation(mon->amiga_renderer, &lw, &lh, nullptr);
            if (lw != surface->w || lh != surface->h) {
			SDL_SetRenderLogicalPresentation(mon->amiga_renderer, surface->w, surface->h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
            }
        }

        SDL_RenderTextureRotated(mon->amiga_renderer, m_amiga_texture, &f_crop, &f_quad, 0, nullptr, SDL_FLIP_NONE);


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
				SDL_FRect dst_cursor = { static_cast<float>(cx), static_cast<float>(cy),
					static_cast<float>(cw), static_cast<float>(ch) };
				SDL_RenderTexture(mon->amiga_renderer, m_cursor_overlay_texture, nullptr, &dst_cursor);
			}
		}

		// GPU-composited Status Line (OSD) for both native and RTG
		if ((((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on) ||
			 ((currprefs.leds_on_screen & STATUSLINE_RTG) && ad->picasso_on)) && mon->statusline_texture)
		{
			int slx, sly, dst_w, dst_h;
			SDL_GetRenderLogicalPresentation(mon->amiga_renderer, &dst_w, &dst_h, nullptr);
			if (dst_w == 0 || dst_h == 0) {
				SDL_GetCurrentRenderOutputSize(mon->amiga_renderer, &dst_w, &dst_h);
			}
			statusline_getpos(monid, &slx, &sly, dst_w, dst_h);
			SDL_FRect dst_osd = { static_cast<float>(slx), static_cast<float>(sly),
				static_cast<float>(mon->statusline_surface->w), static_cast<float>(mon->statusline_surface->h) };
			SDL_RenderTexture(mon->amiga_renderer, mon->statusline_texture, nullptr, &dst_osd);
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
	if (gfx_platform_present_frame(get_amiga_surface(monid))) {
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

	if (isfullscreen() < 0) {
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

void SDLRenderer::get_drawable_size(SDL_Window* w, int* width, int* height)
{
	// SDL3 software path: try to find the monitor owning this window
	for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
		if (AMonitors[i].amiga_window == w && AMonitors[i].amiga_renderer) {
			SDL_GetCurrentRenderOutputSize(AMonitors[i].amiga_renderer, width, height);
			return;
		}
	}
	// Fallback: use monitor 0
	if (AMonitors[0].amiga_renderer) {
		SDL_GetCurrentRenderOutputSize(AMonitors[0].amiga_renderer, width, height);
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
