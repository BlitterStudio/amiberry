#pragma once

/*
 * opengl_renderer.h - OpenGL backend implementation of IRenderer
 *
 * Owns the GL context, shader state, and overlay resources as private
 * members. External code accesses GL-specific state via the typed
 * get_opengl_renderer() helper.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#ifdef USE_OPENGL

#include "irenderer.h"
#include "gl_platform.h"
#include "external_shader.h"
#include <SDL3/SDL.h>
#include <string>
#include <vector>

// Forward declarations
struct crtemu_t;
class ShaderPreset;

struct ShaderParameterCache {
	std::string shader_name;
	std::vector<ShaderParameter> parameters;
};

// Shader lifecycle and caching state (owned by OpenGLRenderer)
struct ShaderState {
	crtemu_t* crtemu = nullptr;
	ExternalShader* external = nullptr;
	ShaderPreset* preset = nullptr;
	std::string external_name;
	std::string loaded_name;      // cache key to avoid recreation
	bool loaded_for_rtg = false;
	ShaderParameterCache parameter_template;
	ShaderParameterCache native_parameter_cache;
	ShaderParameterCache rtg_parameter_cache;
	GLenum texture_filter_mode = GL_LINEAR;
	bool bezel_enabled = false;
	GLuint resolve_framebuffer = 0;
	GLuint resolve_texture = 0;
	int resolve_width = 0;
	int resolve_height = 0;
};

// OpenGL overlay resources: OSD, bezel, software cursor (owned by OpenGLRenderer)
struct GLOverlayState {
	GLuint osd_texture = 0;
	GLuint osd_program = 0;
	GLint osd_tex_loc = -1;
	GLuint osd_vbo = 0;
	GLuint osd_vao = 0;
	GLuint vbo_uploaded = 0;

	GLuint bezel_texture = 0;
	GLuint bezel_vao = 0;
	GLuint bezel_vbo = 0;
	std::string loaded_bezel_name;
	bool bezel_dirty = true;  // Set true to trigger reload check; false = skip string comparisons
	int bezel_tex_w = 0;
	int bezel_tex_h = 0;
	float bezel_hole_x = 0.0f;
	float bezel_hole_y = 0.0f;
	float bezel_hole_w = 1.0f;
	float bezel_hole_h = 1.0f;

	GLuint cursor_texture = 0;
	GLuint cursor_vao = 0;
	GLuint cursor_vbo = 0;
};

class OpenGLRenderer : public IRenderer {
public:
	OpenGLRenderer() = default;
	~OpenGLRenderer() override = default;

	// Context lifecycle
	bool init_context(SDL_Window* window) override;
	void destroy_context() override;
	bool has_context() const override;

	// Window creation support
	SDL_WindowFlags get_window_flags() const override;
	bool set_context_attributes(int mode) override;
	bool create_platform_renderer(AmigaMonitor* mon) override;
	void destroy_platform_renderer(AmigaMonitor* mon) override;

	// Texture / shader allocation
	bool alloc_texture(int monid, int w, int h) override;
	void set_scaling(int monid, const uae_prefs* p, int w, int h) override;

	// VSync
	void update_vsync(int monid) override;

	// Frame rendering
	bool render_frame(int monid, int mode, int immediate) override;
	void present_frame(int monid, int mode) override;

	// Shader management
	void destroy_shaders() override;
	void clear_shader_cache() override;
	void reset_state() override;
	bool has_valid_shader() const override;
	bool has_shader_parameters() const override;

	// Bezel overlay
	void update_custom_bezel() override;
	void update_crtemu_bezel() override;
	BezelHoleInfo get_bezel_hole_info() const override;

	// Overlay rendering
	void render_osd(int monid, int x, int y, int w, int h) override;
	void render_bezel(int x, int y, int w, int h) override;
	void render_software_cursor(int monid, int x, int y, int w, int h) override;
	void destroy_bezel() override;
	void render_vkbd(int monid) override;
	void render_onscreen_joystick(int monid) override;

	// Input coordinate translation
	void get_gfx_offset(int monid, float src_w, float src_h, float src_x, float src_y,
		float* dx, float* dy, float* mx, float* my) override;

	// Drawable size query
	void get_drawable_size(SDL_Window* w, int* width, int* height) override;

	// Cleanup of window-associated resources
	void close_hwnds_cleanup(AmigaMonitor* mon) override;

	// GUI context transitions
	void prepare_gui_sharing(AmigaMonitor* mon) override;
	void restore_emulation_context(SDL_Window* window) override;

	// --- OpenGL-specific accessors (not part of IRenderer) ---

	// Access the GL context for ImGui GUI integration
	SDL_GLContext get_gl_context() const;

	// Access shader state for filter.cpp parameter editing
	ShaderState& shader_state();
	const ShaderState& shader_state() const;
	std::vector<ShaderParameter>* shader_parameters(const char* shader_name, bool rtg);
	const std::vector<ShaderParameter>* shader_parameters(const char* shader_name, bool rtg) const;
	bool has_shader_parameters(const char* shader_name, bool rtg) const;
	bool set_shader_parameter(const char* shader_name, bool rtg, const std::string& name, float value);
	void save_shader_parameters(const char* shader_name, bool rtg);

	// Access overlay state for overlay rendering functions
	GLOverlayState& overlay_state();
	const GLOverlayState& overlay_state() const;

private:
	SDL_GLContext m_gl_context = nullptr;
	ShaderState m_shader;
	GLOverlayState m_overlay;
	bool m_integer_scaling = false; // Integer scaling for native Amiga modes (scaling_method == 2)

	// Cached GL pixel format info (resolved once from pixel_format, reused per frame)
	struct GLFormatInfo {
		GLenum fmt = GL_RGBA;
		GLenum type = GL_UNSIGNED_BYTE;
		int bpp = 4;
	} m_gl_format;

	// Private helpers for overlay rendering
	bool init_osd_shader();
	bool load_bezel_texture(const char* bezel_name);
	void cache_shader_parameter_template();
	void cache_shader_parameters();
	void restore_shader_parameters(const char* shader_name, bool rtg);
	bool ensure_shader_resolve_target(int width, int height);
	void destroy_shader_resolve_target();
	void render_shader_resolve(int x, int y, int width, int height);

	// Private helper for external shader rendering
	void render_external_shader(ExternalShader* shader, int monid,
		const uae_u8* pixels, int width, int height, int pitch,
		int viewport_x, int viewport_y, int viewport_width, int viewport_height,
		GLuint target_framebuffer);
};

// Helper to get the OpenGL renderer from the global g_renderer.
// Returns nullptr if g_renderer is null or not an OpenGLRenderer.
OpenGLRenderer* get_opengl_renderer();

#endif // USE_OPENGL
