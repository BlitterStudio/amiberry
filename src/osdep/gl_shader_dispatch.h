#pragma once

/*
 * gl_shader_dispatch.h - Shader creation, dispatch, and teardown
 *
 * Extracted from amiberry_gfx.cpp.
 *
 * Copyright 2025 Dimitris Panokostas
 */

#include "uae/types.h"

struct uae_prefs;

// Used in both OpenGL and non-OpenGL paths
void set_scaling_option(int monid, const uae_prefs* p, int width, int height);
bool SDL2_alloctexture(int monid, int w, int h);

#ifdef USE_OPENGL

class ExternalShader;

bool is_external_shader(const char* shader);
bool is_shader_preset(const char* shader);
int get_crtemu_type(const char* shader);

void render_with_external_shader(ExternalShader* shader, int monid,
	const uae_u8* pixels, int width, int height, int pitch,
	int viewport_x, int viewport_y, int viewport_width, int viewport_height);

void destroy_shaders();
void clear_loaded_shader_name();
void reset_gl_state();

#endif // USE_OPENGL
