#ifndef __SHADER_STUFF_H
#define __SHADER_STUFF_H

#include <EGL/egl.h>

extern int shader_stuff_init();
extern int shader_stuff_shader_needs_reload();
extern int shader_stuff_reload_shaders();
extern int shader_stuff_set_data(GLfloat *vertex_coords_3f, GLfloat *texture_coords_2f, GLuint texture_name);
extern int shader_stuff_frame(int framecount, int emu_width, int emu_height, int out_width, int out_height);

#endif
