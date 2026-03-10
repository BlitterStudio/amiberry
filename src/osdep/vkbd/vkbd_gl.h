#pragma once
#ifdef USE_OPENGL

#include "gl_platform.h"
#include <SDL3/SDL.h>

// Initialize the VKBD GL shader program and VAO/VBO.
// Returns true on success, false on shader compilation/link error.
bool vkbd_init_gl_shader();

// Upload an SDL_Surface to a GL texture. Returns the texture ID (0 on failure).
GLuint vkbd_upload_surface_to_gl(SDL_Surface* surface);

// Render a textured quad with alpha blending.
// rect is in screen-space pixels; drawable_w/h is the full GL drawable size.
void vkbd_render_gl_quad(GLuint texture, const SDL_Rect& rect,
	int drawable_w, int drawable_h, float alpha_val);

// Render a solid-color filled rectangle.
void vkbd_render_gl_filled_rect(const SDL_Rect& rect,
	int drawable_w, int drawable_h, float r, float g, float b, float a);

// Clean up GL shader program, VAO, and VBO resources.
void vkbd_cleanup_gl_shader();

// Accessors for GL handles needed by vkbd_redraw_gl() setup
GLuint vkbd_get_gl_program();
GLuint vkbd_get_gl_vao();

#endif // USE_OPENGL
