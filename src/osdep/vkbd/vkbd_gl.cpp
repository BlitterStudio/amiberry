#ifdef USE_OPENGL

#include "vkbd_gl.h"

#include <cstdlib>
#include <cstring>

#include "sysdeps.h"
#include "uae.h"

// GL shader resources
static GLuint vkbd_gl_program = 0;
static GLint vkbd_gl_tex_loc = -1;
static GLint vkbd_gl_alpha_loc = -1;
static GLint vkbd_gl_color_loc = -1;
static GLint vkbd_gl_mode_loc = -1;
static GLuint vkbd_gl_vao = 0;
static GLuint vkbd_gl_vbo = 0;

// Vertex shader: passes position and texture coordinates through
static const char* vkbd_vs_source =
	"attribute vec4 pos;\n"
	"varying vec2 uv;\n"
	"void main() {\n"
	"  gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
	"  uv = pos.zw;\n"
	"}\n";

// Fragment shader: mode 0 = textured quad with alpha, mode 1 = solid color
static const char* vkbd_fs_source =
	"varying vec2 uv;\n"
	"uniform sampler2D tex0;\n"
	"uniform float alpha;\n"
	"uniform vec4 fillColor;\n"
	"uniform int mode;\n"
	"void main() {\n"
	"  if (mode == 1) {\n"
	"    gl_FragColor = fillColor;\n"
	"  } else {\n"
	"    vec4 c = texture2D(tex0, uv);\n"
	"    gl_FragColor = vec4(c.rgb, c.a * alpha);\n"
	"  }\n"
	"}\n";

bool vkbd_init_gl_shader()
{
	if (vkbd_gl_program != 0 && glIsProgram(vkbd_gl_program)) return true;

	vkbd_gl_program = 0;

	const char* gl_ver_str = (const char*)glGetString(GL_VERSION);
	bool is_gles = gl_ver_str && (strstr(gl_ver_str, "OpenGL ES") != nullptr);
	int major = 0, minor = 0;
	if (gl_ver_str) {
		const char* v = gl_ver_str;
		while (*v && (*v < '0' || *v > '9')) v++;
		if (*v) {
			major = atoi(v);
			while (*v && *v != '.') v++;
			if (*v == '.') { v++; minor = atoi(v); }
		}
	}

	const char* vs_preamble = "#version 120\n";
	const char* fs_preamble = "#version 120\n";

	if (is_gles && major >= 3) {
		vs_preamble = "#version 300 es\nprecision mediump float;\n#define attribute in\n#define varying out\n";
		fs_preamble = "#version 300 es\nprecision mediump float;\nprecision mediump int;\n#define varying in\n#define texture2D texture\n#define gl_FragColor outFragColor\nout vec4 outFragColor;\n";
	}
	else if (!is_gles && (major > 3 || (major == 3 && minor >= 2))) {
		vs_preamble = "#version 330 core\n#define attribute in\n#define varying out\n";
		fs_preamble = "#version 330 core\nprecision mediump int;\n#define varying in\n#define texture2D texture\n#define gl_FragColor outFragColor\nout vec4 outFragColor;\n";
	}

	GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
	const char* vs_sources[] = { vs_preamble, vkbd_vs_source };
	glShaderSource(vsh, 2, vs_sources, nullptr);
	glCompileShader(vsh);

	GLint compiled;
	glGetShaderiv(vsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char log[512];
		glGetShaderInfoLog(vsh, 512, nullptr, log);
		write_log("VKBD GL vertex shader error: %s\n", log);
		glDeleteShader(vsh);
		return false;
	}

	GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fs_sources[] = { fs_preamble, vkbd_fs_source };
	glShaderSource(fsh, 2, fs_sources, nullptr);
	glCompileShader(fsh);
	glGetShaderiv(fsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char log[512];
		glGetShaderInfoLog(fsh, 512, nullptr, log);
		write_log("VKBD GL fragment shader error: %s\n", log);
		glDeleteShader(vsh);
		glDeleteShader(fsh);
		return false;
	}

	vkbd_gl_program = glCreateProgram();
	glAttachShader(vkbd_gl_program, vsh);
	glAttachShader(vkbd_gl_program, fsh);
	glBindAttribLocation(vkbd_gl_program, 0, "pos");
	glLinkProgram(vkbd_gl_program);

	GLint linked;
	glGetProgramiv(vkbd_gl_program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char log[512];
		glGetProgramInfoLog(vkbd_gl_program, 512, nullptr, log);
		write_log("VKBD GL program link error: %s\n", log);
		glDeleteProgram(vkbd_gl_program);
		vkbd_gl_program = 0;
		glDeleteShader(vsh);
		glDeleteShader(fsh);
		return false;
	}

	glDeleteShader(vsh);
	glDeleteShader(fsh);

	vkbd_gl_tex_loc = glGetUniformLocation(vkbd_gl_program, "tex0");
	vkbd_gl_alpha_loc = glGetUniformLocation(vkbd_gl_program, "alpha");
	vkbd_gl_color_loc = glGetUniformLocation(vkbd_gl_program, "fillColor");
	vkbd_gl_mode_loc = glGetUniformLocation(vkbd_gl_program, "mode");

	if (vkbd_gl_vao == 0) glGenVertexArrays(1, &vkbd_gl_vao);
	if (vkbd_gl_vbo == 0) glGenBuffers(1, &vkbd_gl_vbo);

	return true;
}

GLuint vkbd_upload_surface_to_gl(SDL_Surface* surface)
{
	if (!surface) return 0;

	// Convert to RGBA32 if needed
	SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
	if (!rgba) return 0;

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);

	SDL_FreeSurface(rgba);
	return tex;
}

void vkbd_render_gl_quad(GLuint texture, const SDL_Rect& rect,
	int drawable_w, int drawable_h, float alpha_val)
{
	if (!texture || drawable_w <= 0 || drawable_h <= 0) return;

	// Convert screen rect to NDC. Screen Y=0 is top, GL Y=0 is bottom.
	float x0 = (2.0f * rect.x / drawable_w) - 1.0f;
	float x1 = (2.0f * (rect.x + rect.w) / drawable_w) - 1.0f;
	float y0 = 1.0f - (2.0f * (rect.y + rect.h) / drawable_h);
	float y1 = 1.0f - (2.0f * rect.y / drawable_h);

	GLfloat vertices[] = {
		x0, y0, 0.0f, 1.0f,
		x1, y0, 1.0f, 1.0f,
		x1, y1, 1.0f, 0.0f,
		x0, y1, 0.0f, 0.0f,
	};

	if (vkbd_gl_mode_loc != -1) glUniform1i(vkbd_gl_mode_loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	if (vkbd_gl_tex_loc != -1) glUniform1i(vkbd_gl_tex_loc, 0);
	if (vkbd_gl_alpha_loc != -1) glUniform1f(vkbd_gl_alpha_loc, alpha_val);

	glBindBuffer(GL_ARRAY_BUFFER, vkbd_gl_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void vkbd_render_gl_filled_rect(const SDL_Rect& rect,
	int drawable_w, int drawable_h, float r, float g, float b, float a)
{
	if (drawable_w <= 0 || drawable_h <= 0) return;

	float x0 = (2.0f * rect.x / drawable_w) - 1.0f;
	float x1 = (2.0f * (rect.x + rect.w) / drawable_w) - 1.0f;
	float y0 = 1.0f - (2.0f * (rect.y + rect.h) / drawable_h);
	float y1 = 1.0f - (2.0f * rect.y / drawable_h);

	GLfloat vertices[] = {
		x0, y0, 0.0f, 0.0f,
		x1, y0, 0.0f, 0.0f,
		x1, y1, 0.0f, 0.0f,
		x0, y1, 0.0f, 0.0f,
	};

	if (vkbd_gl_mode_loc != -1) glUniform1i(vkbd_gl_mode_loc, 1);
	if (vkbd_gl_color_loc != -1) glUniform4f(vkbd_gl_color_loc, r, g, b, a);

	glBindBuffer(GL_ARRAY_BUFFER, vkbd_gl_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void vkbd_cleanup_gl_shader()
{
	if (vkbd_gl_vbo) { glDeleteBuffers(1, &vkbd_gl_vbo); vkbd_gl_vbo = 0; }
	if (vkbd_gl_vao) { glDeleteVertexArrays(1, &vkbd_gl_vao); vkbd_gl_vao = 0; }
	if (vkbd_gl_program) { glDeleteProgram(vkbd_gl_program); vkbd_gl_program = 0; }
}

// Accessor for the GL program handle (needed for vkbd_redraw_gl setup)
GLuint vkbd_get_gl_program() { return vkbd_gl_program; }
GLuint vkbd_get_gl_vao() { return vkbd_gl_vao; }

#endif // USE_OPENGL
