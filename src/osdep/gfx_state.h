#pragma once

/*
 * gfx_state.h - Grouped graphics state structs
 *
 * Groups the scattered global/static variables from amiberry_gfx.cpp into named
 * structs to make state ownership visible and enable extraction of functions into
 * separate translation units (each taking a struct reference parameter).
 *
 * Copyright 2026 Dimitris Panokostas
 */

#include <string>
#include <SDL.h>
#include "uae/types.h"
#include "uae/time.h"

#ifdef USE_OPENGL
#include "gl_platform.h"
#endif

// Forward declarations
struct crtemu_t;
class ExternalShader;
class ShaderPreset;
struct MultiDisplay;

#ifdef USE_OPENGL

// Shader lifecycle and caching state
struct ShaderState {
	crtemu_t* crtemu = nullptr;
	ExternalShader* external = nullptr;
	ShaderPreset* preset = nullptr;
	std::string external_name;
	std::string loaded_name;      // cache key to avoid recreation
	GLenum texture_filter_mode = GL_LINEAR;
};

// OpenGL overlay resources: OSD, bezel, software cursor
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

#endif // USE_OPENGL

// VSync and vblank state
struct VSyncState {
	int current_interval = -1;
	float cached_refresh_rate = 0.0f;
	bool gl_initialized = false;
	volatile int waitvblankthread_mode = 0;
	frame_time_t wait_vblank_timestamp = 0;
	MultiDisplay* wait_vblank_display = nullptr;
	volatile bool active = false;
	bool scanlinecalibrating = false;
};

// Render geometry: viewport, crop, and scroll offsets
struct RenderGeometry {
	SDL_Rect render_quad = {};
	SDL_Rect crop_rect = {};
	int dx = 0;
	int dy = 0;
};
