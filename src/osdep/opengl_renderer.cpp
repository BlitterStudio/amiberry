/*
 * opengl_renderer.cpp - OpenGL backend implementation of IRenderer
 *
 * Owns the GL context, shader state, overlay resources, and all
 * GL-specific rendering logic (shader dispatch, overlays, frame
 * presentation).
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include "sysdeps.h"

#ifdef USE_OPENGL

#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "picasso96.h"

#include "opengl_renderer.h"
#include "amiberry_gfx.h"
#include "gfx_window.h"
#include "drawing.h"
#include "fsdb_host.h"
#include "gfx_platform_internal.h"
#include "gl_platform.h"
#include "crtemu.h"
#include "crt_frame.h"
#include "external_shader.h"
#include "shader_preset.h"
#include "target.h"
#include "statusline.h"
#include "vkbd/vkbd.h"
#include "on_screen_joystick.h"
#include <SDL_image.h>

#ifndef GL_BGRA
#ifdef GL_BGRA_EXT
#define GL_BGRA GL_BGRA_EXT
#else
#define GL_BGRA 0x80E1
#endif
#endif

#ifdef AMIBERRY

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
		m_vsync.current_interval = -1;
		m_vsync.cached_refresh_rate = 0.0f;
		m_vsync.gl_initialized = false;
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

// Check if shader name refers to an external .glsl file
static bool is_external_shader(const char* shader)
{
	if (!shader) return false;
	const char* ext = strrchr(shader, '.');
	return ext && !strcasecmp(ext, ".glsl");
}

// Check if shader name refers to a .glslp shader preset
static bool is_shader_preset(const char* shader)
{
	if (!shader) return false;
	const char* ext = strrchr(shader, '.');
	return ext && !strcasecmp(ext, ".glslp");
}

static int get_crtemu_type(const char* shader, ShaderState& state)
{
	if (!shader) return CRTEMU_TYPE_TV;

	if (is_external_shader(shader) || is_shader_preset(shader)) {
		state.external_name = shader;
		return CRTEMU_TYPE_NONE;
	}

	if (!std::strcmp(shader, "tv") || !std::strcmp(shader, "TV"))       return CRTEMU_TYPE_TV;
	if (!std::strcmp(shader, "pc") || !std::strcmp(shader, "PC"))       return CRTEMU_TYPE_PC;
	if (!std::strcmp(shader, "lite") || !std::strcmp(shader, "LITE"))   return CRTEMU_TYPE_LITE;
	if (!std::strcmp(shader, "1084"))                                   return CRTEMU_TYPE_1084;
	if (!std::strcmp(shader, "none") || !std::strcmp(shader, "NONE"))   return CRTEMU_TYPE_NONE;
	return CRTEMU_TYPE_TV;
}

bool OpenGLRenderer::alloc_texture(int monid, int w, int h)
{
	if (currprefs.headless) return true;
	if (w == 0 || h == 0) return false;
	if (gfx_platform_skip_alloctexture(monid, w, h)) return true;

	auto mon = &AMonitors[monid];
	const char* shader_name;
	if (mon->screen_is_picasso)
		shader_name = amiberry_options.shader_rtg;
	else
		shader_name = amiberry_options.shader;

	// Skip shader recreation if already loaded with the same name.
	// This preserves runtime parameter changes made by the user.
	bool shader_exists = (m_shader.crtemu != nullptr || m_shader.external != nullptr || m_shader.preset != nullptr);
	if (shader_exists && m_shader.loaded_name == shader_name) {
		return true;
	}

	// Clean up existing shaders (name changed or no shader loaded yet)
	destroy_shaders();
	m_shader.loaded_name = shader_name;

	// Force full render on next frame after shader switch
	mon->full_render_needed = true;

	// Ensure GL context is current before creating shaders
	if (m_gl_context && mon->amiga_window) {
		SDL_GL_MakeCurrent(mon->amiga_window, m_gl_context);
	}

	// Check if we should use a shader preset (.glslp)
	if (is_shader_preset(shader_name)) {
		write_log("Loading shader preset: %s\n", shader_name);
		m_shader.preset = create_shader_preset(shader_name);

		if (!m_shader.preset) {
			write_log("Failed to load shader preset, falling back to built-in shaders\n");
			shader_name = "none";
		} else {
			write_log("Shader preset loaded successfully (%d passes)\n", m_shader.preset->get_pass_count());
			return true;
		}
	}
	// Check if we should use an external shader (.glsl)
	else if (is_external_shader(shader_name)) {
		write_log("Loading external shader: %s\n", shader_name);
		m_shader.external = create_external_shader(shader_name);

		if (!m_shader.external) {
			write_log("Failed to load external shader, falling back to built-in shaders\n");
			shader_name = "none";
		} else {
			write_log("External shader loaded successfully\n");
			return true;
		}
	}

	// Use built-in crtemu shaders
	if (m_shader.crtemu == nullptr) {
		const int crt_type = get_crtemu_type(shader_name, m_shader);
		m_shader.crtemu = crtemu_create(static_cast<crtemu_type_t>(crt_type), nullptr,
			amiberry_options.force_mobile_shaders);

		if (m_shader.crtemu != nullptr && amiberry_options.force_mobile_shaders) {
			m_shader.crtemu->is_mobile_gpu = true;
		}

		// Fallback to NONE if shader creation failed
		if (m_shader.crtemu == nullptr && crt_type != CRTEMU_TYPE_NONE) {
			write_log("WARNING: Failed to create CRT shader type %d, falling back to NONE\n", crt_type);
			m_shader.crtemu = crtemu_create(CRTEMU_TYPE_NONE, nullptr);
		}

		update_crtemu_bezel();
	}
	return m_shader.crtemu != nullptr || m_shader.external != nullptr || m_shader.preset != nullptr;
}

// Helper to decide between Linear (smooth) and Nearest Neighbor (pixelated) scaling
static bool ar_is_exact(const SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}

void OpenGLRenderer::set_scaling(int monid, const uae_prefs* p, int w, int h)
{
	if (currprefs.headless) return;

	auto scale_quality = "nearest";

	switch (p->scaling_method) {
	case -1: // Auto
		if (!ar_is_exact(&sdl_mode, w, h))
			scale_quality = "linear";
		break;
	case 0: scale_quality = "nearest"; break;
	case 1: scale_quality = "linear"; break;
	case 2: scale_quality = "nearest"; break;
	default: scale_quality = "linear"; break;
	}

	m_shader.texture_filter_mode = (strcmp(scale_quality, "linear") == 0) ? GL_LINEAR : GL_NEAREST;

	// Only apply filter mode when no shader is active (NONE mode without external shader)
	bool no_shader_active = (m_shader.crtemu != nullptr
		&& m_shader.crtemu->type == CRTEMU_TYPE_NONE
		&& m_shader.external == nullptr);
	if (no_shader_active) {
		if (m_shader.crtemu->backbuffer != 0 && glIsTexture(m_shader.crtemu->backbuffer)) {
			m_shader.crtemu->texture_filter = m_shader.texture_filter_mode;
			glBindTexture(GL_TEXTURE_2D, m_shader.crtemu->backbuffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_shader.texture_filter_mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_shader.texture_filter_mode);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
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
			if (m_vsync.cached_refresh_rate <= 0.0f) {
				m_vsync.cached_refresh_rate = SDL2_getrefreshrate(monid);
				write_log("VSync: Detected refresh rate: %.2f Hz\n", m_vsync.cached_refresh_rate);
			}

			float target_fps = (float)vblank_hz;
			if (target_fps < 45 || target_fps > 125) target_fps = 50.0f;

			if (m_vsync.cached_refresh_rate > 0) {
				interval = (int)std::round(m_vsync.cached_refresh_rate / target_fps);
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

	if (m_vsync.current_interval != interval) {
		if (SDL_GL_SetSwapInterval(interval) == 0) {
			write_log("OpenGL VSync: Mode %d, Interval set to %d\n", vsync_mode, interval);
		}
		else {
			write_log("OpenGL VSync: Failed to set interval %d: %s (will not retry)\n", interval, SDL_GetError());
		}
		m_vsync.current_interval = interval;
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
	extern SDL_Surface* amiga_surface;

	AmigaMonitor* mon = &AMonitors[monid];

	const auto time = SDL_GetTicks();

	// Handle VSync options
	update_vsync(monid);

	int drawableWidth, drawableHeight;
	get_drawable_size(mon->amiga_window, &drawableWidth, &drawableHeight);

	// Ensure GL context is current for this window
	if (m_gl_context && mon->amiga_window) {
		if (SDL_GL_MakeCurrent(mon->amiga_window, m_gl_context) != 0) {
			write_log("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
		}
	}

	// Reset GL state to a known baseline - only on first frame after context creation
	if (!m_vsync.gl_initialized) {
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		m_vsync.gl_initialized = true;
	}

	float desired_aspect = calculate_desired_aspect(mon);
	if (desired_aspect <= 0.0f) desired_aspect = 4.0f / 3.0f;

	// When a custom bezel is active, constrain the emulator output to the
	// bezel's transparent screen hole. We define an "effective" render area
	// that all shader paths use instead of the full drawable.
	int renderAreaX = 0, renderAreaY = 0;
	int renderAreaW = drawableWidth, renderAreaH = drawableHeight;

	if (amiberry_options.use_custom_bezel && m_overlay.bezel_texture != 0 && m_overlay.bezel_hole_w > 0.0f && m_overlay.bezel_hole_h > 0.0f) {
		renderAreaX = static_cast<int>(m_overlay.bezel_hole_x * drawableWidth);
		renderAreaY = static_cast<int>(m_overlay.bezel_hole_y * drawableHeight);
		renderAreaW = static_cast<int>(m_overlay.bezel_hole_w * drawableWidth);
		renderAreaH = static_cast<int>(m_overlay.bezel_hole_h * drawableHeight);
	}

	int destW, destH, destX, destY;

	if (renderAreaX != 0 || renderAreaY != 0 ||
		renderAreaW != drawableWidth || renderAreaH != drawableHeight) {
		// Custom bezel active: fill the screen hole completely.
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
	if (m_shader.preset && m_shader.preset->is_valid()) {
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

			m_shader.preset->render(crop_ptr, w, h, amiga_surface->pitch,
				viewport_x, viewport_y, viewport_w, viewport_h, preset_frame_count++);
		} else if (amiga_surface) {
			m_shader.preset->render(static_cast<const unsigned char*>(amiga_surface->pixels),
				amiga_surface->w, amiga_surface->h, amiga_surface->pitch,
				viewport_x, viewport_y, viewport_w, viewport_h, preset_frame_count++);
		}

	}
	// Handle external shader rendering (simplified single-pass)
	else if (m_shader.external && m_shader.external->is_valid()) {
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

			render_external_shader(m_shader.external, monid, crop_ptr,
				w, h, amiga_surface->pitch, viewport_x, viewport_y, viewport_w, viewport_h);
		} else if (amiga_surface) {
			// FAST PATH: No cropping
			render_external_shader(m_shader.external, monid,
				static_cast<const uae_u8*>(amiga_surface->pixels),
				amiga_surface->w, amiga_surface->h, amiga_surface->pitch, viewport_x, viewport_y, viewport_w, viewport_h);
		}

	} else if (m_shader.crtemu) {
		// crtemu_present expects attribute 0 to be enabled.
		glEnableVertexAttribArray(0);
		// Disable other attributes that might have been enabled by OSD or other passes
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// When a custom bezel defines the viewport, skip crtemu's internal 4:3 letterboxing
		m_shader.crtemu->skip_aspect_correction = (renderAreaW != drawableWidth || renderAreaH != drawableHeight);

		if (m_shader.crtemu->type != CRTEMU_TYPE_NONE) {
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

			crtemu_present(m_shader.crtemu, time * 1000, reinterpret_cast<const CRTEMU_U32*>(crop_ptr),
				w, h, amiga_surface->pitch, 0xffffffff, 0x000000, gl_fmt, gl_type, bpp);
		} else if (amiga_surface) {
			// FAST PATH: No cropping.
			crtemu_present(m_shader.crtemu, time * 1000, (CRTEMU_U32 const*)amiga_surface->pixels,
			amiga_surface->w, amiga_surface->h, amiga_surface->pitch, 0xffffffff, 0x000000, gl_fmt, gl_type, bpp);
		}
	}

	render_software_cursor(monid, destX, destY, destW, destH);
	render_bezel(drawableWidth, drawableHeight);
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
}

// --- External shader rendering ---

void OpenGLRenderer::render_external_shader(ExternalShader* shader, const int monid,
	const uae_u8* pixels, int width, int height, int pitch,
	int viewport_x, int viewport_y, int viewport_width, int viewport_height)
{
	if (!shader || !shader->is_valid()) {
		write_log("render_external_shader: shader is null or invalid\n");
		return;
	}

	if (!pixels) {
		write_log("render_external_shader: pixels is NULL!\n");
		return;
	}

	// Clear any existing GL errors
	(void)glGetError();

	if (!shader->is_valid() || !glIsProgram(shader->get_program())) {
		write_log("render_external_shader: shader program is invalid or lost. Attempting to reload...\n");
		return;
	}

	GLuint texture = shader->get_input_texture();
	GLuint vbo = shader->get_input_vbo();
	GLuint vao = shader->get_input_vao();
	static int frame_count = 0;

	// Verify resources are still valid for this context
	if (texture != 0 && !glIsTexture(texture)) {
		write_log("render_external_shader: texture lost, resetting\n");
		texture = 0;
		shader->set_input_texture(0);
	}
	if (vbo != 0 && !glIsBuffer(vbo)) {
		write_log("render_external_shader: VBO lost, resetting\n");
		vbo = 0;
		shader->set_input_vbo(0);
	}
	if (vao != 0 && !glIsVertexArray(vao)) {
		write_log("render_external_shader: VAO lost, resetting\n");
		vao = 0;
		shader->set_input_vao(0);
	}

	// Create texture if needed
	if (texture == 0) {
		glGenTextures(1, &texture);
		if (texture == 0) {
			write_log("ERROR: Failed to create texture!\n");
			return;
		}
		shader->set_input_texture(texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	// Create VAO if needed
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		if (vao == 0) {
			write_log("ERROR: Failed to create VAO!\n");
			return;
		}
		shader->set_input_vao(vao);
	}
	glBindVertexArray(vao);

	// Create VBO if needed
	if (vbo == 0) {
		glGenBuffers(1, &vbo);
		if (vbo == 0) {
			write_log("ERROR: Failed to create VBO!\n");
			return;
		}
		shader->set_input_vbo(vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	// Ensure we're rendering to the default framebuffer (screen)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set viewport explicitly
	glViewport(viewport_x, viewport_y, viewport_width, viewport_height);

	// Set up GL state for 2D rendering
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);

	// Determine correct OpenGL format based on global pixel_format
	GLenum gl_fmt = GL_RGBA;
	GLenum gl_type = GL_UNSIGNED_BYTE;
	int bpp = 4;

	if (pixel_format == SDL_PIXELFORMAT_ARGB8888) {
		gl_fmt = GL_BGRA;
		gl_type = GL_UNSIGNED_BYTE;
		bpp = 4;
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

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / bpp);

	// Track texture changes per-shader to handle shader switches properly
	GLuint current_program = shader->get_program();
	static int last_w = 0, last_h = 0;
	static GLuint last_texture = 0;
	static GLuint last_shader_program = 0;

	// Reset cache if shader changed
	if (current_program != last_shader_program) {
		last_w = 0;
		last_h = 0;
		last_texture = 0;
		last_shader_program = current_program;
	}

	if (width != last_w || height != last_h || texture != last_texture) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, gl_fmt, gl_type, pixels);
		last_w = width;
		last_h = height;
		last_texture = texture;
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, gl_fmt, gl_type, pixels);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	// Use the shader
	shader->use();

	// Set uniforms
	shader->set_texture_size(static_cast<float>(width), static_cast<float>(height));
	shader->set_input_size(static_cast<float>(width), static_cast<float>(height));

	int safe_output_w = (viewport_width >= width) ? viewport_width : width;
	int safe_output_h = (viewport_height >= height) ? viewport_height : height;
	shader->set_output_size(static_cast<float>(safe_output_w), static_cast<float>(safe_output_h));
	shader->set_frame_count(frame_count++);

	// Set MVP matrix (orthographic projection for fullscreen quad)
	float mvp[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	shader->set_mvp_matrix(mvp);

	// Apply parameter uniforms
	shader->apply_parameter_uniforms();

	// Bind texture
	shader->bind_texture(texture, 0);

	// Set up vertex data for fullscreen quad
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	float vertices[] = {
		// VertexCoord (x,y,z,w), TexCoord (s,t,0,0)
		-1.0f, -1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 0.0f,  // Bottom-left
		 1.0f, -1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 0.0f,  // Bottom-right
		 1.0f,  1.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 0.0f,  // Top-right
		-1.0f,  1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 0.0f   // Top-left
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	const GLsizei stride = 8 * sizeof(float);

	// Attribute 0: VertexCoord (vec4: x, y, z, w)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, nullptr);

	// Attribute 1: Color (vec4) - Set to white by default
	glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);

	// Attribute 2: TexCoord (vec2: s, t)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));

	// Draw fullscreen quad
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// Cleanup
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

// --- Shader management ---

void OpenGLRenderer::destroy_shaders()
{
	// Clear tracked name so next alloc_texture call will recreate
	m_shader.loaded_name.clear();

	// Early exit if no GL context exists (e.g., quitting before emulation started)
	if (!has_context())
	{
		// Reset non-GL state
		m_shader.crtemu = nullptr;
		m_shader.external = nullptr;
		m_shader.preset = nullptr;
		m_vsync.gl_initialized = false;
		return;
	}

	if (m_shader.crtemu != nullptr)
	{
		crtemu_destroy(m_shader.crtemu);
		m_shader.crtemu = nullptr;
	}
	if (m_shader.external != nullptr)
	{
		destroy_external_shader(m_shader.external);
		m_shader.external = nullptr;
	}
	if (m_shader.preset != nullptr)
	{
		destroy_shader_preset(m_shader.preset);
		m_shader.preset = nullptr;
	}
	if (m_overlay.osd_program != 0 && glIsProgram(m_overlay.osd_program))
	{
		glDeleteProgram(m_overlay.osd_program);
		m_overlay.osd_program = 0;
	}
	if (m_overlay.osd_vbo != 0 && glIsBuffer(m_overlay.osd_vbo))
	{
		glDeleteBuffers(1, &m_overlay.osd_vbo);
		m_overlay.osd_vbo = 0;
	}
	if (m_overlay.osd_vao != 0 && glIsVertexArray(m_overlay.osd_vao))
	{
		glDeleteVertexArrays(1, &m_overlay.osd_vao);
		m_overlay.osd_vao = 0;
	}
	if (m_overlay.osd_texture != 0 && glIsTexture(m_overlay.osd_texture))
	{
		glDeleteTextures(1, &m_overlay.osd_texture);
		m_overlay.osd_texture = 0;
	}

	// Clean up custom bezel overlay
	destroy_bezel();

	// Reset GL state flag to ensure clean slate for next shader
	m_vsync.gl_initialized = false;

	// Reset GL state to ensure clean slate for next shader
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Reset pixel store settings
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// Disable all vertex attributes that might have been enabled
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

void OpenGLRenderer::clear_shader_cache()
{
	m_shader.loaded_name.clear();
}

void OpenGLRenderer::reset_state()
{
	m_vsync.gl_initialized = false;
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

void OpenGLRenderer::get_gfx_offset(int monid, float src_w, float src_h, float src_x, float src_y,
	float* dx, float* dy, float* mx, float* my)
{
	extern SDL_Surface* amiga_surface;
	const amigadisplay* ad = &adisplays[monid];

	*dx = 0; *dy = 0; *mx = 1.0f; *my = 1.0f;

	if (amiga_surface && render_quad.w > 0 && render_quad.h > 0) {
		if (src_w > 0) *mx = static_cast<float>(render_quad.w) / src_w;
		if (src_h > 0) *my = static_cast<float>(render_quad.h) / src_h;

		if (ad->picasso_on) {
			*dx = -static_cast<float>(render_quad.x) + src_x * (*mx);
			*dy = -static_cast<float>(render_quad.y) + src_y * (*my);
		} else {
			*dx = static_cast<float>(render_quad.x) / (*mx) - src_x;
			*dy = static_cast<float>(render_quad.y) / (*my) - src_y;
		}
	}
}

void OpenGLRenderer::get_drawable_size(SDL_Window* w, int* width, int* height)
{
	SDL_GL_GetDrawableSize(w, width, height);
}

// get_vblank_timestamp() uses base class default (returns m_vsync.wait_vblank_timestamp)

// --- Cleanup ---

void OpenGLRenderer::close_hwnds_cleanup(AmigaMonitor* /*mon*/)
{
	destroy_shaders();
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

// --- OSD shader source ---

static const char* osd_vs_source =
	""
	"attribute vec4 pos;\n"
	"varying vec2 uv;\n"
	"void main() {\n"
	"  gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
	"  uv = pos.zw;\n"
	"}\n";

static const char* osd_fs_source =
	""
	"varying vec2 uv;\n"
	"uniform sampler2D tex0;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(tex0, uv);\n"
	"}\n";

bool OpenGLRenderer::init_osd_shader()
{
	if (m_overlay.osd_program != 0 && glIsProgram(m_overlay.osd_program)) return true;

	m_overlay.osd_program = 0;
	m_overlay.osd_vbo = 0;

	// Detect GL version and profile
	const char* gl_ver_str = (const char*)glGetString(GL_VERSION);
	bool is_gles = gl_ver_str && (strstr(gl_ver_str, "OpenGL ES") != nullptr);
	int major = 0, minor = 0;
	if (gl_ver_str) {
		const char* v = gl_ver_str;
		while (*v && (*v < '0' || *v > '9')) v++;
		if (*v) {
			major = atoi(v);
			while (*v && *v != '.') v++;
			if (*v == '.') {
				v++;
				minor = atoi(v);
			}
		}
	}

	const char* vs_preamble = "#version 120\n";
	const char* fs_preamble = "#version 120\n";

	if (is_gles && major >= 3) {
		vs_preamble = "#version 300 es\nprecision mediump float;\n#define attribute in\n#define varying out\n";
		fs_preamble = "#version 300 es\nprecision mediump float;\n#define varying in\n#define texture2D texture\n#define gl_FragColor outFragColor\nout vec4 outFragColor;\n";
	}
	else if (!is_gles && (major > 3 || (major == 3 && minor >= 2))) {
		vs_preamble = "#version 330 core\n#define attribute in\n#define varying out\n";
		fs_preamble = "#version 330 core\n#define varying in\n#define texture2D texture\n#define gl_FragColor outFragColor\nout vec4 outFragColor;\n";
	}

	GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
	const char* vs_sources[] = { vs_preamble, osd_vs_source };
	glShaderSource(vsh, 2, vs_sources, nullptr);
	glCompileShader(vsh);

	GLint compiled;
	glGetShaderiv(vsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char infoLog[512];
		glGetShaderInfoLog(vsh, 512, nullptr, infoLog);
		write_log("OSD Vertex Shader compile error: %s\n", infoLog);
		return false;
	}

	GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fs_sources[] = { fs_preamble, osd_fs_source };
	glShaderSource(fsh, 2, fs_sources, nullptr);
	glCompileShader(fsh);
	glGetShaderiv(fsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char infoLog[512];
		glGetShaderInfoLog(fsh, 512, nullptr, infoLog);
		write_log("OSD Fragment Shader compile error: %s\n", infoLog);
		glDeleteShader(vsh);
		return false;
	}

	m_overlay.osd_program = glCreateProgram();
	glAttachShader(m_overlay.osd_program, vsh);
	glAttachShader(m_overlay.osd_program, fsh);

	// Bind attribute locations explicitly for modern GL
	glBindAttribLocation(m_overlay.osd_program, 0, "pos");

	glLinkProgram(m_overlay.osd_program);

	GLint linked;
	glGetProgramiv(m_overlay.osd_program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char infoLog[512];
		glGetProgramInfoLog(m_overlay.osd_program, 512, nullptr, infoLog);
		write_log("OSD Shader link error: %s\n", infoLog);
		glDeleteShader(vsh);
		glDeleteShader(fsh);
		glDeleteProgram(m_overlay.osd_program);
		m_overlay.osd_program = 0;
		return false;
	}

	// Flag for deletion (they stay attached until program is deleted)
	glDeleteShader(vsh);
	glDeleteShader(fsh);

	m_overlay.osd_tex_loc = glGetUniformLocation(m_overlay.osd_program, "tex0");

	glGenVertexArrays(1, &m_overlay.osd_vao);
	glBindVertexArray(m_overlay.osd_vao);

	glGenBuffers(1, &m_overlay.osd_vbo);
	glBindVertexArray(0);

	return true;
}

// --- Overlay rendering ---

void OpenGLRenderer::render_osd(const int monid, int x, int y, int w, int h)
{
	const AmigaMonitor* mon = &AMonitors[monid];
	const amigadisplay* ad = &adisplays[monid];
	static int last_osd_w = 0;
	static int last_osd_h = 0;

	if (((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on) ||
		((currprefs.leds_on_screen & STATUSLINE_RTG) && ad->picasso_on))
	{
		if (mon->statusline_surface) {
			if (m_overlay.osd_texture != 0 && !glIsTexture(m_overlay.osd_texture)) {
				m_overlay.osd_texture = 0;
			}
			if (m_overlay.osd_texture == 0) {
				glGenTextures(1, &m_overlay.osd_texture);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_overlay.osd_texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				last_osd_w = 0;
				last_osd_h = 0;
			}

			if (!init_osd_shader()) return;

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_overlay.osd_texture);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, mon->statusline_surface->pitch / 4);
			if (mon->statusline_surface->w != last_osd_w || mon->statusline_surface->h != last_osd_h) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mon->statusline_surface->w, mon->statusline_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, mon->statusline_surface->pixels);
				last_osd_w = mon->statusline_surface->w;
				last_osd_h = mon->statusline_surface->h;
			}
			else {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mon->statusline_surface->w, mon->statusline_surface->h, GL_RGBA, GL_UNSIGNED_BYTE, mon->statusline_surface->pixels);
			}
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_SCISSOR_TEST);
			glViewport(x, y, w, h);

			glUseProgram(m_overlay.osd_program);
			if (m_overlay.osd_tex_loc != -1) glUniform1i(m_overlay.osd_tex_loc, 0);

			// Bind VAO
			glBindVertexArray(m_overlay.osd_vao);

			// Ensure only attribute 0 is enabled for OSD
			glEnableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);

			float osd_w = (float)mon->statusline_surface->w;
			float osd_h = (float)mon->statusline_surface->h;
			float win_w = (float)w;
			float win_h = (float)h;

			// Force full width (stretch to fit window)
			float scale_x = win_w / osd_w;

			// Scale height to match width scaling (preserve aspect of LEDs)
			float scaled_h = osd_h * scale_x;

			// Convert to NDC dimensions
			float ndc_h = (scaled_h / win_h) * 2.0f;

			float x0 = -1.0f;
			float x1 = 1.0f;
			float y0 = -1.0f;
			float y1 = y0 + ndc_h;

			GLfloat vertices[] = {
				x0, y0, 0.0f, 1.0f,
				x1, y0, 1.0f, 1.0f,
				x1, y1, 1.0f, 0.0f,
				x0, y1, 0.0f, 0.0f,
			};

			glBindBuffer(GL_ARRAY_BUFFER, m_overlay.osd_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			glDisable(GL_BLEND);
			glUseProgram(0);
		}
	}
}

bool OpenGLRenderer::load_bezel_texture(const char* bezel_name)
{
	if (!bezel_name || !strcmp(bezel_name, "none") || !strlen(bezel_name))
		return false;

	// Already loaded with same name
	if (m_overlay.bezel_texture != 0 && m_overlay.loaded_bezel_name == bezel_name)
		return true;

	// Unload existing
	if (m_overlay.bezel_texture != 0 && glIsTexture(m_overlay.bezel_texture)) {
		glDeleteTextures(1, &m_overlay.bezel_texture);
		m_overlay.bezel_texture = 0;
	}
	m_overlay.loaded_bezel_name.clear();
	m_overlay.bezel_tex_w = 0;
	m_overlay.bezel_tex_h = 0;

	std::string full_path = get_bezels_path() + bezel_name;
	SDL_Surface* surface = IMG_Load(full_path.c_str());
	if (!surface) {
		write_log("Custom bezel: failed to load %s: %s\n", full_path.c_str(), IMG_GetError());
		return false;
	}

	SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
	SDL_FreeSurface(surface);
	if (!rgba) {
		write_log("Custom bezel: failed to convert %s to RGBA\n", full_path.c_str());
		return false;
	}

	// Scan alpha channel to find the transparent "screen hole" bounding box
	{
		int min_x = rgba->w, min_y = rgba->h, max_x = 0, max_y = 0;
		const int pitch = rgba->pitch;
		const Uint8* pixels = static_cast<const Uint8*>(rgba->pixels);

		for (int y = 0; y < rgba->h; y++) {
			const Uint8* row = pixels + y * pitch;
			for (int x = 0; x < rgba->w; x++) {
				// ABGR8888 layout: R, G, B, A (alpha is byte 3)
				Uint8 alpha = row[x * 4 + 3];
				if (alpha < 128) {  // Transparent or semi-transparent
					if (x < min_x) min_x = x;
					if (x > max_x) max_x = x;
					if (y < min_y) min_y = y;
					if (y > max_y) max_y = y;
				}
			}
		}

		if (max_x >= min_x && max_y >= min_y) {
			m_overlay.bezel_hole_x = static_cast<float>(min_x) / rgba->w;
			m_overlay.bezel_hole_y = static_cast<float>(min_y) / rgba->h;
			m_overlay.bezel_hole_w = static_cast<float>(max_x - min_x + 1) / rgba->w;
			m_overlay.bezel_hole_h = static_cast<float>(max_y - min_y + 1) / rgba->h;
			write_log("Custom bezel: screen hole at %.1f%%,%.1f%% size %.1f%%x%.1f%%\n",
				m_overlay.bezel_hole_x * 100.0f, m_overlay.bezel_hole_y * 100.0f,
				m_overlay.bezel_hole_w * 100.0f, m_overlay.bezel_hole_h * 100.0f);
		} else {
			// No transparent region found - default to full image
			m_overlay.bezel_hole_x = 0.0f;
			m_overlay.bezel_hole_y = 0.0f;
			m_overlay.bezel_hole_w = 1.0f;
			m_overlay.bezel_hole_h = 1.0f;
			write_log("Custom bezel: no transparent region found, using full area\n");
		}
	}

	glGenTextures(1, &m_overlay.bezel_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_overlay.bezel_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	m_overlay.bezel_tex_w = rgba->w;
	m_overlay.bezel_tex_h = rgba->h;
	m_overlay.loaded_bezel_name = bezel_name;

	SDL_FreeSurface(rgba);
	write_log("Custom bezel: loaded %s (%dx%d)\n", bezel_name, m_overlay.bezel_tex_w, m_overlay.bezel_tex_h);
	return true;
}

void OpenGLRenderer::destroy_bezel()
{
	if (m_overlay.bezel_texture != 0 && glIsTexture(m_overlay.bezel_texture)) {
		glDeleteTextures(1, &m_overlay.bezel_texture);
		m_overlay.bezel_texture = 0;
	}
	if (m_overlay.bezel_vbo != 0) {
		glDeleteBuffers(1, &m_overlay.bezel_vbo);
		m_overlay.bezel_vbo = 0;
	}
	if (m_overlay.bezel_vao != 0) {
		glDeleteVertexArrays(1, &m_overlay.bezel_vao);
		m_overlay.bezel_vao = 0;
	}
	m_overlay.loaded_bezel_name.clear();
	m_overlay.bezel_tex_w = 0;
	m_overlay.bezel_tex_h = 0;
	m_overlay.bezel_hole_x = 0.0f;
	m_overlay.bezel_hole_y = 0.0f;
	m_overlay.bezel_hole_w = 1.0f;
	m_overlay.bezel_hole_h = 1.0f;
}

void OpenGLRenderer::render_bezel(int drawableWidth, int drawableHeight)
{
	if (!amiberry_options.use_custom_bezel ||
		strcmp(amiberry_options.custom_bezel, "none") == 0) {
		// Clean up if we had a texture loaded
		if (m_overlay.bezel_texture != 0) {
			destroy_bezel();
		}
		return;
	}

	// Check if we need to (re)load the texture
	if (m_overlay.loaded_bezel_name != amiberry_options.custom_bezel) {
		// Name changed or was cleared by update_custom_bezel() - reload
		if (m_overlay.bezel_texture != 0) {
			destroy_bezel();
		}
		if (!load_bezel_texture(amiberry_options.custom_bezel)) return;
	}
	if (m_overlay.bezel_texture == 0 || !glIsTexture(m_overlay.bezel_texture)) return;

	// Reuse OSD shader program
	if (!init_osd_shader()) return;

	// Create VAO/VBO and upload vertex data on first use
	if (m_overlay.bezel_vao == 0) {
		// Full-screen quad in NDC (static - never changes)
		static const GLfloat vertices[] = {
			-1.0f, -1.0f, 0.0f, 1.0f,  // bottom-left
			 1.0f, -1.0f, 1.0f, 1.0f,  // bottom-right
			 1.0f,  1.0f, 1.0f, 0.0f,  // top-right
			-1.0f,  1.0f, 0.0f, 0.0f,  // top-left
		};

		glGenVertexArrays(1, &m_overlay.bezel_vao);
		glBindVertexArray(m_overlay.bezel_vao);
		glGenBuffers(1, &m_overlay.bezel_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, m_overlay.bezel_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
		glBindVertexArray(0);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_SCISSOR_TEST);

	// Full window viewport
	glViewport(0, 0, drawableWidth, drawableHeight);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_overlay.bezel_texture);

	glUseProgram(m_overlay.osd_program);
	if (m_overlay.osd_tex_loc != -1) glUniform1i(m_overlay.osd_tex_loc, 0);

	glBindVertexArray(m_overlay.bezel_vao);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, m_overlay.bezel_vbo);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRenderer::render_software_cursor(const int monid, int x, int y, int w, int h)
{
	const amigadisplay* ad = &adisplays[monid];
	if (ad->picasso_on && p96_uses_software_cursor()) {
		if (p96_cursor_needs_update() || !m_overlay.cursor_texture) {
			SDL_Surface* s = p96_get_cursor_overlay_surface();
			if (s) {
				if (m_overlay.cursor_texture == 0) {
					glGenTextures(1, &m_overlay.cursor_texture);
				}
				glBindTexture(GL_TEXTURE_2D, m_overlay.cursor_texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
			}
		}

		if (m_overlay.cursor_texture) {
			if (m_overlay.cursor_vao == 0) glGenVertexArrays(1, &m_overlay.cursor_vao);
			if (m_overlay.cursor_vbo == 0) glGenBuffers(1, &m_overlay.cursor_vbo);

			if (!init_osd_shader()) return; // Re-use OSD shader (simple texture shader)

			int cx, cy, cw, ch;
			p96_get_cursor_position(&cx, &cy);
			p96_get_cursor_dimensions(&cw, &ch);

			extern SDL_Surface* amiga_surface;
			if (amiga_surface) {
				float surf_w = (float)amiga_surface->w;
				float surf_h = (float)amiga_surface->h;

				// Percentage of surface
				float px = (float)cx / surf_w;
				float py = (float)cy / surf_h;
				float pw = (float)cw / surf_w;
				float ph = (float)ch / surf_h;

				float x0 = px * 2.0f - 1.0f;
				float y0 = 1.0f - (py * 2.0f + ph * 2.0f); // Origin top-left, GL bottom-left.

				float y_top = 1.0f - py * 2.0f;
				float y_bottom = 1.0f - (py + ph) * 2.0f;

				float x_left = x0;
				float x_right = px * 2.0f - 1.0f + pw * 2.0f;

				GLfloat vertices[] = {
					x_left,  y_bottom, 0.0f, 1.0f,
					x_right, y_bottom, 1.0f, 1.0f,
					x_right, y_top,    1.0f, 0.0f,
					x_left,  y_top,    0.0f, 0.0f,
				};

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_SCISSOR_TEST);
				glViewport(x, y, w, h);

				glUseProgram(m_overlay.osd_program);
				if (m_overlay.osd_tex_loc != -1) glUniform1i(m_overlay.osd_tex_loc, 0);

				glBindVertexArray(m_overlay.cursor_vao);

				glEnableVertexAttribArray(0);
				glDisableVertexAttribArray(1);
				glDisableVertexAttribArray(2);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m_overlay.cursor_texture);

				glBindBuffer(GL_ARRAY_BUFFER, m_overlay.cursor_vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
				glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

				glDisableVertexAttribArray(0);
				glBindVertexArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glDisable(GL_BLEND);
				glUseProgram(0);
			}
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
