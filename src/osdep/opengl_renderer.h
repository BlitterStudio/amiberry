#pragma once

/*
 * opengl_renderer.h - OpenGL backend implementation of IRenderer
 *
 * Wraps all OpenGL-specific rendering operations. During the initial
 * integration phase this delegates to existing free functions and globals.
 * Subsequent phases will internalize the state.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#ifdef USE_OPENGL

#include "irenderer.h"
#include <SDL.h>

// Forward declarations
struct ShaderState;
struct GLOverlayState;
struct VSyncState;

class OpenGLRenderer : public IRenderer {
public:
	OpenGLRenderer() = default;
	~OpenGLRenderer() override = default;

	// Context lifecycle
	bool init_context(SDL_Window* window) override;
	void destroy_context() override;
	bool has_context() const override;

	// Window creation support
	Uint32 get_window_flags() const override;
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

	// Bezel overlay
	void update_custom_bezel() override;
	void update_crtemu_bezel() override;
	BezelHoleInfo get_bezel_hole_info() const override;

	// Drawable size query
	void get_drawable_size(SDL_Window* w, int* width, int* height) override;

	// VSync timestamp
	frame_time_t get_vblank_timestamp() const override;

	// Cleanup of window-associated resources
	void close_hwnds_cleanup(AmigaMonitor* mon) override;

	// --- OpenGL-specific accessors (not part of IRenderer) ---

	// Access the GL context for ImGui GUI integration
	SDL_GLContext get_gl_context() const;

	// Access shader state for filter.cpp parameter editing
	ShaderState& shader_state();
	const ShaderState& shader_state() const;
};

#endif // USE_OPENGL
