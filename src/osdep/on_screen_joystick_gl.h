#pragma once
#ifdef USE_OPENGL

#include "gl_platform.h"
#include <SDL3/SDL.h>

// Initialize the on-screen joystick GL shader program and VAO/VBO.
// Returns true on success, false on shader compilation/link error.
bool osj_init_gl_shader();

// Upload an SDL_Surface to a GL texture. Returns the texture ID (0 on failure).
// Expects SDL_PIXELFORMAT_ABGR8888 surfaces (GL_RGBA + GL_UNSIGNED_BYTE).
GLuint osj_upload_surface_to_gl(SDL_Surface* surface);

// Render a textured quad with alpha blending.
// rect is in screen-space pixels; drawable_w/h is the full GL drawable size.
void osj_render_gl_quad(GLuint texture, const SDL_Rect& rect,
	int drawable_w, int drawable_h, float alpha);

// Clean up GL shader program, VAO, and VBO resources.
void osj_cleanup_gl_shader();

// Accessors for GL handles needed by on_screen_joystick_redraw_gl() setup
GLuint osj_get_gl_program();
GLuint osj_get_gl_vao();

#endif // USE_OPENGL
