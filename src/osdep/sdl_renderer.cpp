/*
 * sdl_renderer.cpp - SDL2 software renderer backend implementation of IRenderer
 *
 * Phase 1: Thin forwarding layer delegating to existing free functions
 * and accessing existing globals. No code is moved yet.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "sysdeps.h"

#ifndef USE_OPENGL

#include "options.h"
#include "xwin.h"

#include "sdl_renderer.h"
#include "amiberry_gfx.h"
#include "gl_shader_dispatch.h"

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
	return SDL2_alloctexture(monid, w, h);
}

void SDLRenderer::set_scaling(int monid, const uae_prefs* p, int w, int h)
{
	AmigaMonitor* mon = &AMonitors[monid];
	if (mon->amiga_renderer)
		SDL_RenderSetLogicalSize(mon->amiga_renderer, w, h);
	set_scaling_option(monid, p, w, h);
}

// --- VSync ---

void SDLRenderer::update_vsync(int /*monid*/)
{
	// SDL2 software path: VSync handled by renderer flags at creation time
}

// --- Frame rendering ---

bool SDLRenderer::render_frame(int monid, int mode, int immediate)
{
	// The SDL2 software render_frame logic is embedded in SDL2_renderframe()
	// in amiberry_gfx.cpp. This will be wired up when SDL2_renderframe
	// delegates to g_renderer.
	return m_amiga_texture && amiga_surface;
}

void SDLRenderer::present_frame(int monid, int mode)
{
	// The SDL2 present logic is in SDL2_showframe() in amiberry_gfx.cpp.
	// This will be wired up when show_screen delegates to g_renderer.
}

// Shader management and bezel overlay: uses IRenderer defaults (no-op)

// --- Drawable size query ---

void SDLRenderer::get_drawable_size(SDL_Window* /*w*/, int* width, int* height)
{
	// SDL2 software path: use renderer output size
	AmigaMonitor* mon = &AMonitors[0];
	if (mon->amiga_renderer) {
		SDL_GetRendererOutputSize(mon->amiga_renderer, width, height);
	}
}

// get_vblank_timestamp() uses base class default (returns m_vsync.wait_vblank_timestamp)

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
