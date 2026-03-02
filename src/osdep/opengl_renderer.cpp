/*
 * opengl_renderer.cpp - OpenGL backend implementation of IRenderer
 *
 * Owns the GL context, shader state, and overlay resources. Free
 * functions in gl_shader_dispatch.cpp and gl_overlays.cpp access
 * the state via get_opengl_renderer() rather than extern globals.
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
#include "crtemu.h"
#include "crt_frame.h"
#include "target.h"

#ifdef AMIBERRY

// g_vsync stays global (shared between GL and SDL paths)
extern VSyncState g_vsync;

// Declared in amiberry_gfx.cpp (no header)
extern float SDL2_getrefreshrate(int monid);

// --- get_opengl_renderer() helper ---

OpenGLRenderer* get_opengl_renderer()
{
	return dynamic_cast<OpenGLRenderer*>(g_renderer.get());
}

// --- Context lifecycle ---

bool OpenGLRenderer::init_context(SDL_Window* window)
{
	write_log("DEBUG: Initializing OpenGL Context...\n");

	m_gl_context = SDL_GL_CreateContext(window);
	if (!m_gl_context) {
		write_log(_T("!!! SDL_GL_CreateContext failed: %hs\n"), SDL_GetError());
		return false;
	}

	if (SDL_GL_MakeCurrent(window, m_gl_context) != 0) {
		write_log(_T("!!! SDL_GL_MakeCurrent failed: %hs\n"), SDL_GetError());
		SDL_GL_DeleteContext(m_gl_context);
		m_gl_context = nullptr;
		return false;
	}

	// Load OpenGL extension functions (does nothing on Android/GLES3)
	if (!gl_platform_init()) {
		const GLubyte* ver = glGetString(GL_VERSION);
		if (!ver) {
			write_log(_T("!!! glGetString(GL_VERSION) is null; failing OpenGL init.\n"));
			SDL_GL_DeleteContext(m_gl_context);
			m_gl_context = nullptr;
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

void OpenGLRenderer::destroy_context()
{
	if (m_gl_context != nullptr)
	{
		SDL_GL_DeleteContext(m_gl_context);
		m_gl_context = nullptr;
		g_vsync.current_interval = -1;
		g_vsync.cached_refresh_rate = 0.0f;
		g_vsync.gl_initialized = false;
	}
}

bool OpenGLRenderer::has_context() const
{
	return m_gl_context != nullptr;
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
	if (!AMonitors[monid].amiga_window) return;

	const AmigaMonitor* mon = &AMonitors[monid];
	const auto idx = mon->screen_is_picasso ? APMODE_RTG : APMODE_NATIVE;
	const int vsync_mode = currprefs.gfx_apmode[idx].gfx_vsync;
	int interval = 0;

	if (vsync_mode > 0) {
		if (vsync_mode > 1) {
			// VSync 50/60Hz: Adapt for High Refresh Rate Monitors
			if (g_vsync.cached_refresh_rate <= 0.0f) {
				g_vsync.cached_refresh_rate = SDL2_getrefreshrate(monid);
				write_log("VSync: Detected refresh rate: %.2f Hz\n", g_vsync.cached_refresh_rate);
			}

			float target_fps = (float)vblank_hz;
			if (target_fps < 45 || target_fps > 125) target_fps = 50.0f;

			if (g_vsync.cached_refresh_rate > 0) {
				interval = (int)std::round(g_vsync.cached_refresh_rate / target_fps);
				if (interval < 1) interval = 1;
			}
			else {
				interval = 1;
			}
		}
		else {
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
		g_vsync.current_interval = interval;
	}
}

// --- Frame rendering ---

bool OpenGLRenderer::render_frame(int monid, int mode, int immediate)
{
	// OpenGL path: just checks surface exists.
	// The actual GL rendering happens in present_frame (show_screen).
	extern SDL_Surface* amiga_surface;
	return amiga_surface != nullptr;
}

void OpenGLRenderer::present_frame(int monid, int mode)
{
	// The OpenGL present logic is embedded in show_screen() in amiberry_gfx.cpp.
	// This is a placeholder — show_screen calls GL code directly.
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
	return m_shader.crtemu != nullptr ||
		   m_shader.external != nullptr ||
		   m_shader.preset != nullptr;
}

// --- Bezel overlay ---

void OpenGLRenderer::update_custom_bezel()
{
	// Don't do GL calls here - this may be called from the GUI
	// where the GL context isn't current. Just invalidate the loaded name
	// and reset hole coordinates so render_bezel_overlay() will reload
	// on the next frame (where the GL context IS current).
	m_overlay.loaded_bezel_name.clear();
	m_overlay.bezel_hole_x = 0.0f;
	m_overlay.bezel_hole_y = 0.0f;
	m_overlay.bezel_hole_w = 1.0f;
	m_overlay.bezel_hole_h = 1.0f;
}

void OpenGLRenderer::update_crtemu_bezel()
{
	if (m_shader.crtemu == nullptr)
		return;
	if (amiberry_options.use_bezel) {
		auto* frame_pixels = new CRTEMU_U32[CRT_FRAME_WIDTH * CRT_FRAME_HEIGHT];
		crt_frame(frame_pixels);
		crtemu_frame(m_shader.crtemu, frame_pixels, CRT_FRAME_WIDTH, CRT_FRAME_HEIGHT);
		delete[] frame_pixels;
	} else {
		crtemu_frame(m_shader.crtemu, nullptr, 0, 0);
	}
}

BezelHoleInfo OpenGLRenderer::get_bezel_hole_info() const
{
	BezelHoleInfo info;
	info.x = m_overlay.bezel_hole_x;
	info.y = m_overlay.bezel_hole_y;
	info.w = m_overlay.bezel_hole_w;
	info.h = m_overlay.bezel_hole_h;
	info.active = (m_overlay.bezel_texture != 0 &&
				   m_overlay.bezel_hole_w > 0.0f &&
				   m_overlay.bezel_hole_h > 0.0f);
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
	if (m_gl_context != nullptr)
	{
		if (m_overlay.cursor_texture) {
			glDeleteTextures(1, &m_overlay.cursor_texture);
			m_overlay.cursor_texture = 0;
		}
		if (m_overlay.cursor_vao) {
			glDeleteVertexArrays(1, &m_overlay.cursor_vao);
			m_overlay.cursor_vao = 0;
		}
		if (m_overlay.cursor_vbo) {
			glDeleteBuffers(1, &m_overlay.cursor_vbo);
			m_overlay.cursor_vbo = 0;
		}
	}
}

// --- OpenGL-specific accessors ---

SDL_GLContext OpenGLRenderer::get_gl_context() const
{
	return m_gl_context;
}

ShaderState& OpenGLRenderer::shader_state()
{
	return m_shader;
}

const ShaderState& OpenGLRenderer::shader_state() const
{
	return m_shader;
}

GLOverlayState& OpenGLRenderer::overlay_state()
{
	return m_overlay;
}

const GLOverlayState& OpenGLRenderer::overlay_state() const
{
	return m_overlay;
}

#endif // AMIBERRY
#endif // USE_OPENGL
