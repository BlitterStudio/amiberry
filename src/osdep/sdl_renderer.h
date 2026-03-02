#pragma once

/*
 * sdl_renderer.h - SDL2 software renderer backend implementation of IRenderer
 *
 * Wraps all non-OpenGL SDL2 rendering operations.
 *
 * Copyright 2026 Dimitris Panokostas
 */

#ifndef USE_OPENGL

#include "irenderer.h"
#include <SDL.h>

class SDLRenderer : public IRenderer {
public:
	SDLRenderer() = default;
	~SDLRenderer() override = default;

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

	// Shader management: uses IRenderer defaults (no-op)
	// Bezel overlay: uses IRenderer defaults (no-op)

	// OSD texture synchronization
	void sync_osd_texture(int monid, int led_width, int led_height) override;

	// Overlay rendering
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

	// SDL-specific accessors
	SDL_Texture*& amiga_texture() { return m_amiga_texture; }
	SDL_Texture*& cursor_overlay_texture() { return m_cursor_overlay_texture; }

private:
	SDL_Texture* m_amiga_texture = nullptr;
	SDL_Texture* m_cursor_overlay_texture = nullptr;
};

SDLRenderer* get_sdl_renderer();

#endif // !USE_OPENGL
