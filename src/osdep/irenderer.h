#pragma once

/*
 * irenderer.h - Abstract renderer interface
 *
 * Defines the IRenderer base class that encapsulates all backend-specific
 * rendering operations. OpenGLRenderer and SDLRenderer implement it for
 * the two current paths.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include <SDL.h>
#include "uae/types.h"
#include "uae/time.h"
#include "gfx_state.h"

struct AmigaMonitor;
struct uae_prefs;

// Bezel hole geometry in normalized coordinates (0.0-1.0)
struct BezelHoleInfo {
	float x = 0.0f;
	float y = 0.0f;
	float w = 1.0f;
	float h = 1.0f;
	bool active = false;
};

class IRenderer {
public:
	virtual ~IRenderer() = default;

	// --- Context lifecycle ---
	virtual bool init_context(SDL_Window* window) = 0;
	virtual void destroy_context() = 0;
	virtual bool has_context() const = 0;

	// --- Window creation support ---
	virtual Uint32 get_window_flags() const = 0;        // SDL_WINDOW_OPENGL or 0
	virtual bool set_context_attributes(int mode) = 0;  // GL attributes or no-op
	virtual bool create_platform_renderer(AmigaMonitor* mon) = 0;  // SDL_CreateRenderer or no-op
	virtual void destroy_platform_renderer(AmigaMonitor* mon) = 0;

	// --- Texture / shader allocation ---
	virtual bool alloc_texture(int monid, int w, int h) = 0;
	virtual void set_scaling(int monid, const uae_prefs* p, int w, int h) = 0;

	// --- VSync ---
	virtual void update_vsync(int monid) = 0;

	// --- Frame rendering ---
	virtual bool render_frame(int monid, int mode, int immediate) = 0;
	virtual void present_frame(int monid, int mode) = 0;

	// --- Shader management (default: no-op for non-GL backends) ---
	virtual void destroy_shaders() {}
	virtual void clear_shader_cache() {}
	virtual void reset_state() {}
	virtual bool has_valid_shader() const { return true; }

	// --- Bezel overlay (default: no-op for non-GL backends) ---
	virtual void update_custom_bezel() {}
	virtual void update_crtemu_bezel() {}
	virtual BezelHoleInfo get_bezel_hole_info() const { return BezelHoleInfo{}; }

	// --- Overlay rendering (default: no-op for non-GL backends) ---
	virtual void render_osd(int monid, int x, int y, int w, int h) {}
	virtual void render_bezel(int drawableWidth, int drawableHeight) {}
	virtual void render_software_cursor(int monid, int x, int y, int w, int h) {}
	virtual void destroy_bezel() {}

	// --- Drawable size query ---
	virtual void get_drawable_size(SDL_Window* w, int* width, int* height) = 0;

	// --- VSync timestamp (for frame timing) ---
	virtual frame_time_t get_vblank_timestamp() const { return m_vsync.wait_vblank_timestamp; }

	// --- Cleanup of window-associated resources ---
	virtual void close_hwnds_cleanup(AmigaMonitor* mon) = 0;

	// --- VSync state accessor ---
	VSyncState& vsync_state() { return m_vsync; }
	const VSyncState& vsync_state() const { return m_vsync; }

protected:
	VSyncState m_vsync;
};
