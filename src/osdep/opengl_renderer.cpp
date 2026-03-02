/*
 * opengl_renderer.cpp - OpenGL backend implementation of IRenderer
 *
 * Phase 1: Thin forwarding layer delegating to existing free functions
 * and accessing existing globals. No code is moved yet.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "sysdeps.h"

#ifdef USE_OPENGL

#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "picasso96.h"

#include "opengl_renderer.h"
#include "amiberry_gfx.h"
#include "gfx_state.h"
#include "gfx_window.h"
#include "gl_shader_dispatch.h"
#include "gl_overlays.h"
#include "gl_platform.h"

#ifdef AMIBERRY

// External globals (defined in amiberry_gfx.cpp and gfx_window.cpp)
extern SDL_GLContext gl_context;
extern ShaderState g_shader;
extern VSyncState g_vsync;

// Forward declarations for existing free functions
extern void update_vsync(int monid);
extern void update_crtemu_bezel();

// --- Context lifecycle ---

bool OpenGLRenderer::init_context(SDL_Window* window)
{
	return init_opengl_context(window);
}

void OpenGLRenderer::destroy_context()
{
	if (gl_context != nullptr)
	{
		SDL_GL_DeleteContext(gl_context);
		gl_context = nullptr;
		g_vsync.current_interval = -1;
		g_vsync.cached_refresh_rate = 0.0f;
		g_vsync.gl_initialized = false;
	}
}

bool OpenGLRenderer::has_context() const
{
	return gl_context != nullptr;
}

// --- Window creation support ---

Uint32 OpenGLRenderer::get_window_flags() const
{
	return SDL_WINDOW_OPENGL;
}

bool OpenGLRenderer::set_context_attributes(int mode)
{
	return set_opengl_attributes(mode);
}

bool OpenGLRenderer::create_platform_renderer(AmigaMonitor* /*mon*/)
{
	// OpenGL path: no SDL_Renderer needed
	return true;
}

void OpenGLRenderer::destroy_platform_renderer(AmigaMonitor* /*mon*/)
{
	// OpenGL path: no SDL_Renderer to destroy
}

// --- Texture / shader allocation ---

bool OpenGLRenderer::alloc_texture(int monid, int w, int h)
{
	return SDL2_alloctexture(monid, w, h);
}

void OpenGLRenderer::set_scaling(int monid, const uae_prefs* p, int w, int h)
{
	set_scaling_option(monid, p, w, h);
}

// --- VSync ---

void OpenGLRenderer::update_vsync(int monid)
{
	::update_vsync(monid);
}

// --- Frame rendering ---

bool OpenGLRenderer::render_frame(int monid, int mode, int immediate)
{
	// OpenGL path in SDL2_renderframe: just checks surface exists.
	// The actual GL rendering happens in present_frame (show_screen).
	extern SDL_Surface* amiga_surface;
	return amiga_surface != nullptr;
}

void OpenGLRenderer::present_frame(int monid, int mode)
{
	// The OpenGL present logic is embedded in show_screen() in amiberry_gfx.cpp.
	// This will be wired up in Step 4 when show_screen delegates to g_renderer.
	// For now this is a placeholder - show_screen calls GL code directly.
}

// --- Shader management ---

void OpenGLRenderer::destroy_shaders()
{
	::destroy_shaders();
}

void OpenGLRenderer::clear_shader_cache()
{
	::clear_loaded_shader_name();
}

void OpenGLRenderer::reset_state()
{
	::reset_gl_state();
}

bool OpenGLRenderer::has_valid_shader() const
{
	return g_shader.crtemu != nullptr ||
		   g_shader.external != nullptr ||
		   g_shader.preset != nullptr;
}

// --- Bezel overlay ---

void OpenGLRenderer::update_custom_bezel()
{
	::update_custom_bezel();
}

void OpenGLRenderer::update_crtemu_bezel()
{
	::update_crtemu_bezel();
}

BezelHoleInfo OpenGLRenderer::get_bezel_hole_info() const
{
	BezelHoleInfo info;
	info.x = g_overlay.bezel_hole_x;
	info.y = g_overlay.bezel_hole_y;
	info.w = g_overlay.bezel_hole_w;
	info.h = g_overlay.bezel_hole_h;
	info.active = (g_overlay.bezel_texture != 0 &&
				   g_overlay.bezel_hole_w > 0.0f &&
				   g_overlay.bezel_hole_h > 0.0f);
	return info;
}

// --- Drawable size query ---

void OpenGLRenderer::get_drawable_size(SDL_Window* w, int* width, int* height)
{
	SDL_GL_GetDrawableSize(w, width, height);
}

// --- VSync timestamp ---

frame_time_t OpenGLRenderer::get_vblank_timestamp() const
{
	return g_vsync.wait_vblank_timestamp;
}

// --- Cleanup ---

void OpenGLRenderer::close_hwnds_cleanup(AmigaMonitor* /*mon*/)
{
	::destroy_shaders();
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
}

// --- OpenGL-specific accessors ---

SDL_GLContext OpenGLRenderer::get_gl_context() const
{
	return gl_context;
}

ShaderState& OpenGLRenderer::shader_state()
{
	return g_shader;
}

const ShaderState& OpenGLRenderer::shader_state() const
{
	return g_shader;
}

#endif // AMIBERRY
#endif // USE_OPENGL
