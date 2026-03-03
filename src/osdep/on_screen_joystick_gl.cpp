#ifdef USE_OPENGL

#include "on_screen_joystick_gl.h"

#include <cstdlib>
#include <cstring>

#include "sysdeps.h"
#include "uae.h"

// GL shader resources
static GLuint osj_gl_program = 0;
static GLint osj_gl_tex_loc = -1;
static GLint osj_gl_alpha_loc = -1;
static GLuint osj_gl_vao = 0;
static GLuint osj_gl_vbo = 0;

static const char* osj_vs_source =
	"attribute vec4 pos;\n"
	"varying vec2 uv;\n"
	"void main() {\n"
	"  gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
	"  uv = pos.zw;\n"
	"}\n";

static const char* osj_fs_source =
	"varying vec2 uv;\n"
	"uniform sampler2D tex0;\n"
	"uniform float alpha;\n"
	"void main() {\n"
	"  vec4 c = texture2D(tex0, uv);\n"
	"  gl_FragColor = vec4(c.rgb, c.a * alpha);\n"
	"}\n";

bool osj_init_gl_shader()
{
	if (osj_gl_program != 0 && glIsProgram(osj_gl_program)) return true;

	osj_gl_program = 0;

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
		fs_preamble = "#version 300 es\nprecision mediump float;\n#define varying in\n#define texture2D texture\n#define gl_FragColor outFragColor\nout vec4 outFragColor;\n";
	}
	else if (!is_gles && (major > 3 || (major == 3 && minor >= 2))) {
		vs_preamble = "#version 330 core\n#define attribute in\n#define varying out\n";
		fs_preamble = "#version 330 core\n#define varying in\n#define texture2D texture\n#define gl_FragColor outFragColor\nout vec4 outFragColor;\n";
	}

	GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
	const char* vs_sources[] = { vs_preamble, osj_vs_source };
	glShaderSource(vsh, 2, vs_sources, nullptr);
	glCompileShader(vsh);

	GLint compiled;
	glGetShaderiv(vsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char log[512];
		glGetShaderInfoLog(vsh, 512, nullptr, log);
		write_log("OSJ GL vertex shader error: %s\n", log);
		glDeleteShader(vsh);
		return false;
	}

	GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fs_sources[] = { fs_preamble, osj_fs_source };
	glShaderSource(fsh, 2, fs_sources, nullptr);
	glCompileShader(fsh);
	glGetShaderiv(fsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char log[512];
		glGetShaderInfoLog(fsh, 512, nullptr, log);
		write_log("OSJ GL fragment shader error: %s\n", log);
		glDeleteShader(vsh);
		glDeleteShader(fsh);
		return false;
	}

	osj_gl_program = glCreateProgram();
	glAttachShader(osj_gl_program, vsh);
	glAttachShader(osj_gl_program, fsh);
	glBindAttribLocation(osj_gl_program, 0, "pos");
	glLinkProgram(osj_gl_program);

	GLint linked;
	glGetProgramiv(osj_gl_program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char log[512];
		glGetProgramInfoLog(osj_gl_program, 512, nullptr, log);
		write_log("OSJ GL program link error: %s\n", log);
		glDeleteProgram(osj_gl_program);
		osj_gl_program = 0;
		glDeleteShader(vsh);
		glDeleteShader(fsh);
		return false;
	}

	glDeleteShader(vsh);
	glDeleteShader(fsh);

	osj_gl_tex_loc = glGetUniformLocation(osj_gl_program, "tex0");
	osj_gl_alpha_loc = glGetUniformLocation(osj_gl_program, "alpha");

	if (osj_gl_vao == 0) glGenVertexArrays(1, &osj_gl_vao);
	if (osj_gl_vbo == 0) glGenBuffers(1, &osj_gl_vbo);

	return true;
}

GLuint osj_upload_surface_to_gl(SDL_Surface* surface)
{
	if (!surface) return 0;

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// SDL_PIXELFORMAT_ABGR8888 = R in byte 0, G in byte 1, B in byte 2, A in byte 3
	// On OpenGL this maps to GL_RGBA + GL_UNSIGNED_BYTE
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	return tex;
}

void osj_render_gl_quad(GLuint texture, const SDL_Rect& rect,
	int drawable_w, int drawable_h, float alpha_val)
{
	if (!texture || drawable_w <= 0 || drawable_h <= 0) return;

	// Convert screen rect to NDC. Screen origin is top-left, GL origin is bottom-left.
	float x0 = (2.0f * rect.x / drawable_w) - 1.0f;
	float x1 = (2.0f * (rect.x + rect.w) / drawable_w) - 1.0f;
	// Flip Y: screen Y=0 is top, GL Y=0 is bottom
	float y0 = 1.0f - (2.0f * (rect.y + rect.h) / drawable_h); // bottom in GL
	float y1 = 1.0f - (2.0f * rect.y / drawable_h);             // top in GL

	GLfloat vertices[] = {
		// x,  y,  u,  v
		x0, y0, 0.0f, 1.0f,  // bottom-left  (tex bottom-left)
		x1, y0, 1.0f, 1.0f,  // bottom-right (tex bottom-right)
		x1, y1, 1.0f, 0.0f,  // top-right    (tex top-right)
		x0, y1, 0.0f, 0.0f,  // top-left     (tex top-left)
	};

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	if (osj_gl_tex_loc != -1) glUniform1i(osj_gl_tex_loc, 0);
	if (osj_gl_alpha_loc != -1) glUniform1f(osj_gl_alpha_loc, alpha_val);

	glBindBuffer(GL_ARRAY_BUFFER, osj_gl_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void osj_cleanup_gl_shader()
{
	if (osj_gl_vbo) { glDeleteBuffers(1, &osj_gl_vbo); osj_gl_vbo = 0; }
	if (osj_gl_vao) { glDeleteVertexArrays(1, &osj_gl_vao); osj_gl_vao = 0; }
	if (osj_gl_program) { glDeleteProgram(osj_gl_program); osj_gl_program = 0; }
}

GLuint osj_get_gl_program() { return osj_gl_program; }
GLuint osj_get_gl_vao() { return osj_gl_vao; }

#endif // USE_OPENGL
