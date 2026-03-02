/*
 * gl_shader_dispatch.cpp - Shader creation, dispatch, and teardown
 *
 * Extracted from amiberry_gfx.cpp.
 *
 * Copyright 2025 Dimitris Panokostas
 */

#include "sysdeps.h"

#include "options.h"
#include "xwin.h"
#include "custom.h"
#include "drawing.h"
#include "picasso96.h"
#include "amiberry_gfx.h"
#include "gfx_state.h"
#include "gl_shader_dispatch.h"
#include "gl_overlays.h"
#include "target.h"
#include "fsdb_host.h"
#include "gfx_platform_internal.h"

#ifdef USE_OPENGL
#include "gl_platform.h"
#include "crtemu.h"
#include "external_shader.h"
#include "shader_preset.h"
#include "opengl_renderer.h"
#endif

#ifdef AMIBERRY

extern VSyncState g_vsync;

// Based on this we make the decision to use Linear (smooth) or Nearest Neighbor (pixelated) scaling
static bool ar_is_exact(const SDL_DisplayMode* mode, const int width, const int height)
{
	return mode->w % width == 0 && mode->h % height == 0;
}

// Set the scaling method based on the user's preferences
// Scaling methods available:
// -1: Auto select between Nearest Neighbor and Linear (default)
// 0: Nearest Neighbor
// 1: Linear
// 2: Integer Scaling (this uses Nearest Neighbor)
void set_scaling_option(const int monid, const uae_prefs* p, const int width, const int height)
{
	// Skip scaling setup if headless
	if (currprefs.headless) {
		return;
	}

	auto scale_quality = "nearest";
	SDL_bool integer_scale = SDL_FALSE;

	switch (p->scaling_method) {
	case -1: // Auto
		if (!ar_is_exact(&sdl_mode, width, height)) {
			scale_quality = "linear";
		}
		break;
	case 0: // Nearest
		scale_quality = "nearest";
		break;
	case 1: // Linear
		scale_quality = "linear";
		break;
	case 2: // Integer
		scale_quality = "nearest";
		integer_scale = SDL_TRUE;
		break;
	default:
		scale_quality = "linear";
		break;
	}

#ifdef USE_OPENGL
	auto& shader = get_opengl_renderer()->shader_state();
	// Store the texture filter mode for use when creating/updating textures
	shader.texture_filter_mode = (strcmp(scale_quality, "linear") == 0) ? GL_LINEAR : GL_NEAREST;
	// Note: integer_scale is not directly applicable to OpenGL - it would need custom
	// viewport calculations which are handled elsewhere in the rendering pipeline

	// Only apply filter mode when no shader is active (NONE mode without external shader)
	// CRT shaders and external shaders handle their own texture filtering
	bool no_shader_active = (shader.crtemu != nullptr && shader.crtemu->type == CRTEMU_TYPE_NONE && shader.external == nullptr);
	if (no_shader_active) {
		// For NONE mode without external shader, we need to apply filter to the backbuffer texture
		if (shader.crtemu->backbuffer != 0 && glIsTexture(shader.crtemu->backbuffer)) {
			// Update the persistent filter state in crtemu so it's used every frame
			shader.crtemu->texture_filter = shader.texture_filter_mode;

			glBindTexture(GL_TEXTURE_2D, shader.crtemu->backbuffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, shader.texture_filter_mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, shader.texture_filter_mode);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
#else
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale_quality);
	SDL_RenderSetIntegerScale(AMonitors[monid].amiga_renderer, integer_scale);
#endif
}

bool SDL2_alloctexture(int monid, int w, int h)
{
	// Skip texture allocation if headless
	if (currprefs.headless) {
		return true;
	}

	if (w == 0 || h == 0)
		return false;
	if (gfx_platform_skip_alloctexture(monid, w, h))
		return true;
#ifdef USE_OPENGL
	auto* r = get_opengl_renderer();
	auto& shader = r->shader_state();
	auto mon = &AMonitors[monid];
	const char* shader_name;
	if (mon->screen_is_picasso)
		shader_name = amiberry_options.shader_rtg;
	else
		shader_name = amiberry_options.shader;

	// Skip shader recreation if already loaded with the same name.
	// This preserves runtime parameter changes made by the user.
	bool shader_exists = (shader.crtemu != nullptr || shader.external != nullptr || shader.preset != nullptr);
	if (shader_exists && shader.loaded_name == shader_name) {
		return true;
	}

	// Clean up existing shaders (name changed or no shader loaded yet)
	destroy_shaders();
	shader.loaded_name = shader_name;

	// Force full render on next frame after shader switch
	mon->full_render_needed = true;

	// Ensure GL context is current before creating shaders
	auto gl_ctx = r->get_gl_context();
	if (gl_ctx && mon->amiga_window) {
		SDL_GL_MakeCurrent(mon->amiga_window, gl_ctx);
	}

	// Check if we should use a shader preset (.glslp)
	if (is_shader_preset(shader_name)) {
		write_log("Loading shader preset: %s\n", shader_name);
		shader.preset = create_shader_preset(shader_name);

		if (!shader.preset) {
			write_log("Failed to load shader preset, falling back to built-in shaders\n");
			shader_name = "none";
		} else {
			write_log("Shader preset loaded successfully (%d passes)\n", shader.preset->get_pass_count());
			return true;
		}
	}
	// Check if we should use an external shader (.glsl)
	else if (is_external_shader(shader_name)) {
		write_log("Loading external shader: %s\n", shader_name);
		shader.external = create_external_shader(shader_name);

		if (!shader.external) {
			write_log("Failed to load external shader, falling back to built-in shaders\n");
			// Fall back to built-in shaders
			shader_name = "none";
		} else {
			write_log("External shader loaded successfully\n");
			return true;
		}
	}

	// Use built-in crtemu shaders
	if (shader.crtemu == nullptr) {
		const int crt_type = get_crtemu_type(shader_name);
		shader.crtemu = crtemu_create(static_cast<crtemu_type_t>(crt_type), nullptr,
			amiberry_options.force_mobile_shaders);

		// Apply force_mobile_shaders override if enabled
		if (shader.crtemu != nullptr && amiberry_options.force_mobile_shaders) {
			shader.crtemu->is_mobile_gpu = true;
		}

		// Fallback to NONE if shader creation failed (e.g., shader compilation error)
		if (shader.crtemu == nullptr && crt_type != CRTEMU_TYPE_NONE) {
			write_log("WARNING: Failed to create CRT shader type %d, falling back to NONE\n", crt_type);
			shader.crtemu = crtemu_create(CRTEMU_TYPE_NONE, nullptr);
		}

		// Load bezel frame overlay if enabled
		r->update_crtemu_bezel();
	}
	return shader.crtemu != nullptr || shader.external != nullptr || shader.preset != nullptr;
#else
	if (w < 0 || h < 0)
	{
		if (amiga_texture)
		{
			int width, height;
			Uint32 format;
			SDL_QueryTexture(amiga_texture, &format, nullptr, &width, &height);
			if (width == -w && height == -h && format == pixel_format)
			{
				set_scaling_option(monid, &currprefs, width, height);
				return true;
			}
		}
		return false;
	}

	if (amiga_texture != nullptr)
		SDL_DestroyTexture(amiga_texture);

	AmigaMonitor* mon = &AMonitors[monid];
	amiga_texture = SDL_CreateTexture(mon->amiga_renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, amiga_surface->w, amiga_surface->h);
	return amiga_texture != nullptr;
#endif
}

#ifdef USE_OPENGL

#ifndef GL_BGRA
#ifdef GL_BGRA_EXT
#define GL_BGRA GL_BGRA_EXT
#else
#define GL_BGRA 0x80E1
#endif
#endif

// Check if shader name refers to an external .glsl file
bool is_external_shader(const char* shader)
{
	if (!shader) return false;
	const char* ext = strrchr(shader, '.');
	return ext && !strcasecmp(ext, ".glsl");
}

// Check if shader name refers to a .glslp shader preset
bool is_shader_preset(const char* shader)
{
	if (!shader) return false;
	const char* ext = strrchr(shader, '.');
	return ext && !strcasecmp(ext, ".glslp");
}

int get_crtemu_type(const char* shader)
{
	if (!shader) return CRTEMU_TYPE_TV;

	// Check if it's an external shader file or preset
	if (is_external_shader(shader) || is_shader_preset(shader)) {
		get_opengl_renderer()->shader_state().external_name = shader;
		return CRTEMU_TYPE_NONE; // Use NONE to skip crtemu initialization
	}

	// Built-in shaders
	if (!std::strcmp(shader, "tv") || !std::strcmp(shader, "TV"))       return CRTEMU_TYPE_TV;
	if (!std::strcmp(shader, "pc") || !std::strcmp(shader, "PC"))       return CRTEMU_TYPE_PC;
	if (!std::strcmp(shader, "lite") || !std::strcmp(shader, "LITE"))   return CRTEMU_TYPE_LITE;
	if (!std::strcmp(shader, "1084"))                                   return CRTEMU_TYPE_1084;
	if (!std::strcmp(shader, "none") || !std::strcmp(shader, "NONE"))   return CRTEMU_TYPE_NONE;
	return CRTEMU_TYPE_TV;
}

// Render using external shader (single-pass)
void render_with_external_shader(ExternalShader* shader, const int monid,
	const uae_u8* pixels, int width, int height, int pitch,
	int viewport_x, int viewport_y, int viewport_width, int viewport_height)
{
	if (!shader || !shader->is_valid()) {
		write_log("render_with_external_shader: shader is null or invalid\n");
		return;
	}

	if (!pixels) {
		write_log("render_with_external_shader: pixels is NULL!\n");
		return;
	}

	// Clear any existing GL errors
	(void)glGetError();

	if (!shader->is_valid() || !glIsProgram(shader->get_program())) {
		write_log("render_with_external_shader: shader program is invalid or lost. Attempting to reload...\n");
		// We can't easily reload here without the filepath, but we can at least detect it.
		// In a real scenario, we should probably signal a re-init.
		return;
	}

	GLuint texture = shader->get_input_texture();
	GLuint vbo = shader->get_input_vbo();
	GLuint vao = shader->get_input_vao();
	static int frame_count = 0;

	// Verify resources are still valid for this context
	if (texture != 0 && !glIsTexture(texture)) {
		write_log("render_with_external_shader: texture lost, resetting\n");
		texture = 0;
		shader->set_input_texture(0);
	}
	if (vbo != 0 && !glIsBuffer(vbo)) {
		write_log("render_with_external_shader: VBO lost, resetting\n");
		vbo = 0;
		shader->set_input_vbo(0);
	}
	if (vao != 0 && !glIsVertexArray(vao)) {
		write_log("render_with_external_shader: VAO lost, resetting\n");
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
		// External shaders handle their own filtering, use linear as base input
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
		// Bind the VBO immediately after creation
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	// Ensure we're rendering to the default framebuffer (screen)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set viewport explicitly - critical fix for external shaders
	glViewport(viewport_x, viewport_y, viewport_width, viewport_height);

	// Set up GL state for 2D rendering
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND); // Disable blending by default for performance and to avoid issues with zero alpha
	glDisable(GL_SCISSOR_TEST);

	// Determine correct OpenGL format based on global pixel_format
	// SDL_PIXELFORMAT_ARGB8888 -> BGRA in memory -> GL_BGRA
	// SDL_PIXELFORMAT_ABGR8888 -> RGBA in memory -> GL_RGBA
	// SDL_PIXELFORMAT_RGB565   -> RGB in memory  -> GL_RGB
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
	// Using shader program ID as part of cache key to detect shader changes
	GLuint current_program = shader->get_program();
	static int last_w = 0, last_h = 0;
	static GLuint last_texture = 0;
	static GLuint last_shader_program = 0;

	// Reset cache if shader changed (handles shader switching)
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

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Reset for other textures

	// Use the shader
	shader->use();

	// Set uniforms
	shader->set_texture_size(static_cast<float>(width), static_cast<float>(height));
	shader->set_input_size(static_cast<float>(width), static_cast<float>(height));

	// Many CRT shaders calculate scale = floor(OutputSize / InputSize)
	// If OutputSize < InputSize, scale becomes 0, causing division by zero.
	// Ensure OutputSize is always at least InputSize to prevent shader errors.
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

	// Apply parameter uniforms from the emulator's GL context.
	// set_parameter() only stores values; the actual glUniform1f calls
	// must happen here in the render loop where the correct GL context is active.
	shader->apply_parameter_uniforms();

	// Bind texture
	shader->bind_texture(texture, 0);

	// Set up vertex data for fullscreen quad
	// The shader expects: attribute vec4 VertexCoord (position as x,y,z,w)
	//                     attribute vec2/vec4 TexCoord (s,t,...)
	// We'll use interleaved format: x, y, z, w, s, t, 0, 0 (8 floats per vertex)
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Always upload vertex data - it's small (128 bytes) and ensures correct state
	// after shader switches or window resize
	float vertices[] = {
		// VertexCoord (x,y,z,w), TexCoord (s,t,0,0)
		-1.0f, -1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 0.0f,  // Bottom-left
		 1.0f, -1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 0.0f,  // Bottom-right
		 1.0f,  1.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 0.0f,  // Top-right
		-1.0f,  1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f, 0.0f   // Top-left
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	// Set up vertex attributes to match shader bindings
	// Explicitly disable any previously enabled arrays to avoid state leakage
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	// Stride: 8 floats per vertex (4 for VertexCoord + 4 for TexCoord)
	const GLsizei stride = 8 * sizeof(float);

	// Attribute 0: VertexCoord (vec4: x, y, z, w)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, nullptr);

	// Attribute 1: Color (vec4) - Set to white by default
	// Many RetroArch shaders multiply the sampled color by this attribute.
	glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);


	// Attribute 2: TexCoord (vec2: s, t)
	// Note: Some shaders use vec2, some use vec4. Using 2 components is more compatible.
	// The shader will only read the first 2 components anyway.
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

void destroy_shaders()
{
	auto* r = get_opengl_renderer();
	auto& shader = r->shader_state();
	auto& overlay = r->overlay_state();

	// Clear tracked name so next SDL2_alloctexture call will recreate
	shader.loaded_name.clear();

	// Early exit if no GL context exists (e.g., quitting before emulation started)
	if (!r->has_context())
	{
		// Reset non-GL state
		shader.crtemu = nullptr;
		shader.external = nullptr;
		shader.preset = nullptr;
		g_vsync.gl_initialized = false;
		return;
	}

	if (shader.crtemu != nullptr)
	{
		crtemu_destroy(shader.crtemu);
		shader.crtemu = nullptr;
	}
	if (shader.external != nullptr)
	{
		destroy_external_shader(shader.external);
		shader.external = nullptr;
	}
	if (shader.preset != nullptr)
	{
		destroy_shader_preset(shader.preset);
		shader.preset = nullptr;
	}
	if (overlay.osd_program != 0 && glIsProgram(overlay.osd_program))
	{
		glDeleteProgram(overlay.osd_program);
		overlay.osd_program = 0;
	}
	if (overlay.osd_vbo != 0 && glIsBuffer(overlay.osd_vbo))
	{
		glDeleteBuffers(1, &overlay.osd_vbo);
		overlay.osd_vbo = 0;
	}
	if (overlay.osd_vao != 0 && glIsVertexArray(overlay.osd_vao))
	{
		glDeleteVertexArrays(1, &overlay.osd_vao);
		overlay.osd_vao = 0;
	}
	if (overlay.osd_texture != 0 && glIsTexture(overlay.osd_texture))
	{
		glDeleteTextures(1, &overlay.osd_texture);
		overlay.osd_texture = 0;
	}

	// Clean up custom bezel overlay
	destroy_bezel_overlay();

	// Reset GL state flag to ensure clean slate for next shader
	g_vsync.gl_initialized = false;

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

void clear_loaded_shader_name()
{
	get_opengl_renderer()->shader_state().loaded_name.clear();
}

void reset_gl_state()
{
	g_vsync.gl_initialized = false;
}
#endif // USE_OPENGL

#endif // AMIBERRY
