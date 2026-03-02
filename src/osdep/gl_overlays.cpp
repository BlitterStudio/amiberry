/*
 * gl_overlays.cpp - OpenGL overlay rendering: OSD, bezel, software cursor
 *
 * Extracted from amiberry_gfx.cpp.
 *
 * Copyright 2025 Dimitris Panokostas
 */

#include <cstring>

#include "sysdeps.h"
#include "options.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "picasso96.h"
#include "statusline.h"

#include "amiberry_gfx.h"
#include "gfx_state.h"
#include "gl_overlays.h"
#include "target.h"
#include "fsdb_host.h"
#include "gfx_platform_internal.h"

#ifdef USE_OPENGL
#include "gl_platform.h"
#include <SDL_image.h>
#include "opengl_renderer.h"
#endif

#ifdef AMIBERRY

#ifdef USE_OPENGL

static const char* osd_vs_source =
	""
	"attribute vec4 pos;\n"
	"varying vec2 uv;\n"
	"void main() {\n"
	"  gl_Position = vec4(pos.xy, 0.0, 1.0);\n"
	"  uv = pos.zw;\n"
	"}\n";

static const char* osd_fs_source =
	""
	"varying vec2 uv;\n"
	"uniform sampler2D tex0;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(tex0, uv);\n"
	"}\n";

bool init_osd_shader()
{
	auto& overlay = get_opengl_renderer()->overlay_state();

	if (overlay.osd_program != 0 && glIsProgram(overlay.osd_program)) return true;

	overlay.osd_program = 0;
	overlay.osd_vbo = 0;

	// Detect GL version and profile
	const char* gl_ver_str = (const char*)glGetString(GL_VERSION);
	bool is_gles = gl_ver_str && (strstr(gl_ver_str, "OpenGL ES") != nullptr);
	int major = 0, minor = 0;
	if (gl_ver_str) {
		const char* v = gl_ver_str;
		while (*v && (*v < '0' || *v > '9')) v++;
		if (*v) {
			major = atoi(v);
			while (*v && *v != '.') v++;
			if (*v == '.') {
				v++;
				minor = atoi(v);
			}
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
	const char* vs_sources[] = { vs_preamble, osd_vs_source };
	glShaderSource(vsh, 2, vs_sources, nullptr);
	glCompileShader(vsh);

	GLint compiled;
	glGetShaderiv(vsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char infoLog[512];
		glGetShaderInfoLog(vsh, 512, nullptr, infoLog);
		write_log("OSD Vertex Shader compile error: %s\n", infoLog);
		return false;
	}

	GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fs_sources[] = { fs_preamble, osd_fs_source };
	glShaderSource(fsh, 2, fs_sources, nullptr);
	glCompileShader(fsh);
	glGetShaderiv(fsh, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		char infoLog[512];
		glGetShaderInfoLog(fsh, 512, nullptr, infoLog);
		write_log("OSD Fragment Shader compile error: %s\n", infoLog);
		glDeleteShader(vsh);
		return false;
	}

	overlay.osd_program = glCreateProgram();
	glAttachShader(overlay.osd_program, vsh);
	glAttachShader(overlay.osd_program, fsh);

	// Bind attribute locations explicitly for modern GL
	glBindAttribLocation(overlay.osd_program, 0, "pos");

	glLinkProgram(overlay.osd_program);

	GLint linked;
	glGetProgramiv(overlay.osd_program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char infoLog[512];
		glGetProgramInfoLog(overlay.osd_program, 512, nullptr, infoLog);
		write_log("OSD Shader link error: %s\n", infoLog);
		glDeleteShader(vsh);
		glDeleteShader(fsh);
		glDeleteProgram(overlay.osd_program);
		overlay.osd_program = 0;
		return false;
	}

	// Flag for deletion (they stay attached until program is deleted)
	glDeleteShader(vsh);
	glDeleteShader(fsh);

	overlay.osd_tex_loc = glGetUniformLocation(overlay.osd_program, "tex0");

    glGenVertexArrays(1, &overlay.osd_vao);
    glBindVertexArray(overlay.osd_vao);

	glGenBuffers(1, &overlay.osd_vbo);
    glBindVertexArray(0);

	return true;
}
#endif

void update_leds(const int monid)
{
	AmigaMonitor* mon = &AMonitors[monid];

	// Skip LED rendering if headless
	if (currprefs.headless) {
		return;
	}
	if (!gfx_platform_render_leds())
		return;

#ifndef USE_OPENGL
	if (!mon->amiga_renderer)
		return;
#endif

	// Use static variables to avoid recalculating color tables every frame
	static uae_u32 rc[256], gc[256], bc[256], a[256];
	static bool color_tables_initialized = false;

	// Only initialize color tables once for better performance
	if (!color_tables_initialized) {
		for (int i = 0; i < 256; i++) {
			// Using RGBA32 for the internal OSD surface
			rc[i] = i << 0;
			gc[i] = i << 8;
			bc[i] = i << 16;
			a[i] = i << 24;
		}
		color_tables_initialized = true;
	}

	const amigadisplay* ad = &adisplays[monid];
	const int m = statusline_get_multiplier(monid) / 100;
	const int led_height = TD_TOTAL_HEIGHT * m;
	int led_width = ad->picasso_on ? (amiga_surface ? amiga_surface->w : mon->currentmode.native_width) : 640;
	if (led_width <= 0)
		led_width = 640;

	// (Re)allocate OSD surface and texture if dimensions changed
	if (!mon->statusline_surface || mon->statusline_surface->w != led_width || mon->statusline_surface->h != led_height) {
		if (mon->statusline_surface) SDL_FreeSurface(mon->statusline_surface);
		mon->statusline_surface = SDL_CreateRGBSurfaceWithFormat(0, led_width, led_height, 32, SDL_PIXELFORMAT_RGBA32);

#ifndef USE_OPENGL
		if (mon->statusline_texture) {
			SDL_DestroyTexture(mon->statusline_texture);
			mon->statusline_texture = nullptr;
		}
#endif
	}

#ifndef USE_OPENGL
	if (!mon->statusline_texture) {
		mon->statusline_texture = SDL_CreateTexture(mon->amiga_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, led_width, led_height);
		SDL_SetTextureBlendMode(mon->statusline_texture, SDL_BLENDMODE_BLEND);
	}
#endif

	if (mon->statusline_surface) {
		// Clear with transparent color
		SDL_FillRect(mon->statusline_surface, nullptr, 0x00000000);

		// Draw the LEDs into the off-screen surface
		for (int y = 0; y < led_height; y++) {
			uae_u8* buf = static_cast<uae_u8*>(mon->statusline_surface->pixels) + y * mon->statusline_surface->pitch;
			draw_status_line_single(monid, buf, y, led_width, rc, gc, bc, a);
		}

#ifndef USE_OPENGL
		// Map the surface to the texture
		if (mon->statusline_texture)
			SDL_UpdateTexture(mon->statusline_texture, nullptr, mon->statusline_surface->pixels, mon->statusline_surface->pitch);
#endif
	}
}

#ifdef USE_OPENGL
void render_osd(const int monid, int x, int y, int w, int h)
{
	auto& overlay = get_opengl_renderer()->overlay_state();
	const AmigaMonitor* mon = &AMonitors[monid];
	const amigadisplay* ad = &adisplays[monid];
	static int last_osd_w = 0;
	static int last_osd_h = 0;

	if (((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on) ||
		((currprefs.leds_on_screen & STATUSLINE_RTG) && ad->picasso_on))
	{
		if (mon->statusline_surface) {
			if (overlay.osd_texture != 0 && !glIsTexture(overlay.osd_texture)) {
				overlay.osd_texture = 0;
			}
			if (overlay.osd_texture == 0) {
				glGenTextures(1, &overlay.osd_texture);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, overlay.osd_texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				last_osd_w = 0;
				last_osd_h = 0;
			}

			if (!init_osd_shader()) return;

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, overlay.osd_texture);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, mon->statusline_surface->pitch / 4);
			if (mon->statusline_surface->w != last_osd_w || mon->statusline_surface->h != last_osd_h) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mon->statusline_surface->w, mon->statusline_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, mon->statusline_surface->pixels);
				last_osd_w = mon->statusline_surface->w;
				last_osd_h = mon->statusline_surface->h;
			}
			else {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mon->statusline_surface->w, mon->statusline_surface->h, GL_RGBA, GL_UNSIGNED_BYTE, mon->statusline_surface->pixels);
			}
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_SCISSOR_TEST);
			glViewport(x, y, w, h);

			glUseProgram(overlay.osd_program);
			if (overlay.osd_tex_loc != -1) glUniform1i(overlay.osd_tex_loc, 0);

            // Bind VAO
            glBindVertexArray(overlay.osd_vao);

			// Ensure only attribute 0 is enabled for OSD
			glEnableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);

			float osd_w = (float)mon->statusline_surface->w;
			float osd_h = (float)mon->statusline_surface->h;
			float win_w = (float)w;
			float win_h = (float)h;

			// Force full width (stretch to fit window)
			float scale_x = win_w / osd_w;

			// Scale height to match width scaling (preserve aspect of LEDs)
			float scaled_h = osd_h * scale_x;

			// Convert to NDC dimensions
			float ndc_h = (scaled_h / win_h) * 2.0f;

			float x0 = -1.0f;
			float x1 = 1.0f;
			float y0 = -1.0f;
			float y1 = y0 + ndc_h;

			GLfloat vertices[] = {
				x0, y0, 0.0f, 1.0f,
				x1, y0, 1.0f, 1.0f,
				x1, y1, 1.0f, 0.0f,
				x0, y1, 0.0f, 0.0f,
			};

			glBindBuffer(GL_ARRAY_BUFFER, overlay.osd_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
			glDisable(GL_BLEND);
			glUseProgram(0);
		}
	}
}

// Custom bezel overlay functions
static bool load_bezel_texture(const char* bezel_name)
{
	auto& overlay = get_opengl_renderer()->overlay_state();

	if (!bezel_name || !strcmp(bezel_name, "none") || !strlen(bezel_name))
		return false;

	// Already loaded with same name
	if (overlay.bezel_texture != 0 && overlay.loaded_bezel_name == bezel_name)
		return true;

	// Unload existing
	if (overlay.bezel_texture != 0 && glIsTexture(overlay.bezel_texture)) {
		glDeleteTextures(1, &overlay.bezel_texture);
		overlay.bezel_texture = 0;
	}
	overlay.loaded_bezel_name.clear();
	overlay.bezel_tex_w = 0;
	overlay.bezel_tex_h = 0;

	std::string full_path = get_bezels_path() + bezel_name;
	SDL_Surface* surface = IMG_Load(full_path.c_str());
	if (!surface) {
		write_log("Custom bezel: failed to load %s: %s\n", full_path.c_str(), IMG_GetError());
		return false;
	}

	SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
	SDL_FreeSurface(surface);
	if (!rgba) {
		write_log("Custom bezel: failed to convert %s to RGBA\n", full_path.c_str());
		return false;
	}

	// Scan alpha channel to find the transparent "screen hole" bounding box
	{
		int min_x = rgba->w, min_y = rgba->h, max_x = 0, max_y = 0;
		const int pitch = rgba->pitch;
		const Uint8* pixels = static_cast<const Uint8*>(rgba->pixels);

		for (int y = 0; y < rgba->h; y++) {
			const Uint8* row = pixels + y * pitch;
			for (int x = 0; x < rgba->w; x++) {
				// ABGR8888 layout: R, G, B, A (alpha is byte 3)
				Uint8 alpha = row[x * 4 + 3];
				if (alpha < 128) {  // Transparent or semi-transparent
					if (x < min_x) min_x = x;
					if (x > max_x) max_x = x;
					if (y < min_y) min_y = y;
					if (y > max_y) max_y = y;
				}
			}
		}

		if (max_x >= min_x && max_y >= min_y) {
			overlay.bezel_hole_x = static_cast<float>(min_x) / rgba->w;
			overlay.bezel_hole_y = static_cast<float>(min_y) / rgba->h;
			overlay.bezel_hole_w = static_cast<float>(max_x - min_x + 1) / rgba->w;
			overlay.bezel_hole_h = static_cast<float>(max_y - min_y + 1) / rgba->h;
			write_log("Custom bezel: screen hole at %.1f%%,%.1f%% size %.1f%%x%.1f%%\n",
				overlay.bezel_hole_x * 100.0f, overlay.bezel_hole_y * 100.0f,
				overlay.bezel_hole_w * 100.0f, overlay.bezel_hole_h * 100.0f);
		} else {
			// No transparent region found - default to full image
			overlay.bezel_hole_x = 0.0f;
			overlay.bezel_hole_y = 0.0f;
			overlay.bezel_hole_w = 1.0f;
			overlay.bezel_hole_h = 1.0f;
			write_log("Custom bezel: no transparent region found, using full area\n");
		}
	}

	glGenTextures(1, &overlay.bezel_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, overlay.bezel_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	overlay.bezel_tex_w = rgba->w;
	overlay.bezel_tex_h = rgba->h;
	overlay.loaded_bezel_name = bezel_name;

	SDL_FreeSurface(rgba);
	write_log("Custom bezel: loaded %s (%dx%d)\n", bezel_name, overlay.bezel_tex_w, overlay.bezel_tex_h);
	return true;
}

void destroy_bezel_overlay()
{
	auto& overlay = get_opengl_renderer()->overlay_state();

	if (overlay.bezel_texture != 0 && glIsTexture(overlay.bezel_texture)) {
		glDeleteTextures(1, &overlay.bezel_texture);
		overlay.bezel_texture = 0;
	}
	if (overlay.bezel_vbo != 0) {
		glDeleteBuffers(1, &overlay.bezel_vbo);
		overlay.bezel_vbo = 0;
	}
	if (overlay.bezel_vao != 0) {
		glDeleteVertexArrays(1, &overlay.bezel_vao);
		overlay.bezel_vao = 0;
	}
	overlay.loaded_bezel_name.clear();
	overlay.bezel_tex_w = 0;
	overlay.bezel_tex_h = 0;
	overlay.bezel_hole_x = 0.0f;
	overlay.bezel_hole_y = 0.0f;
	overlay.bezel_hole_w = 1.0f;
	overlay.bezel_hole_h = 1.0f;
}

void render_bezel_overlay(int drawableWidth, int drawableHeight)
{
	auto& overlay = get_opengl_renderer()->overlay_state();

	if (!amiberry_options.use_custom_bezel ||
		strcmp(amiberry_options.custom_bezel, "none") == 0) {
		// Clean up if we had a texture loaded
		if (overlay.bezel_texture != 0) {
			destroy_bezel_overlay();
		}
		return;
	}

	// Check if we need to (re)load the texture
	if (overlay.loaded_bezel_name != amiberry_options.custom_bezel) {
		// Name changed or was cleared by update_custom_bezel() - reload
		if (overlay.bezel_texture != 0) {
			destroy_bezel_overlay();
		}
		if (!load_bezel_texture(amiberry_options.custom_bezel)) return;
	}
	if (overlay.bezel_texture == 0 || !glIsTexture(overlay.bezel_texture)) return;

	// Reuse OSD shader program
	if (!init_osd_shader()) return;

	// Create VAO/VBO and upload vertex data on first use
	if (overlay.bezel_vao == 0) {
		// Full-screen quad in NDC (static - never changes)
		static const GLfloat vertices[] = {
			-1.0f, -1.0f, 0.0f, 1.0f,  // bottom-left
			 1.0f, -1.0f, 1.0f, 1.0f,  // bottom-right
			 1.0f,  1.0f, 1.0f, 0.0f,  // top-right
			-1.0f,  1.0f, 0.0f, 0.0f,  // top-left
		};

		glGenVertexArrays(1, &overlay.bezel_vao);
		glBindVertexArray(overlay.bezel_vao);
		glGenBuffers(1, &overlay.bezel_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, overlay.bezel_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
		glBindVertexArray(0);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_SCISSOR_TEST);

	// Full window viewport
	glViewport(0, 0, drawableWidth, drawableHeight);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, overlay.bezel_texture);

	glUseProgram(overlay.osd_program);
	if (overlay.osd_tex_loc != -1) glUniform1i(overlay.osd_tex_loc, 0);

	glBindVertexArray(overlay.bezel_vao);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, overlay.bezel_vbo);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glDisable(GL_BLEND);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void render_software_cursor_gl(const int monid, int x, int y, int w, int h)
{
	auto& overlay = get_opengl_renderer()->overlay_state();
	const amigadisplay* ad = &adisplays[monid];
	if (ad->picasso_on && p96_uses_software_cursor()) {
		if (p96_cursor_needs_update() || !overlay.cursor_texture) {
			SDL_Surface* s = p96_get_cursor_overlay_surface();
			if (s) {
				if (overlay.cursor_texture == 0) {
					glGenTextures(1, &overlay.cursor_texture);
				}
				glBindTexture(GL_TEXTURE_2D, overlay.cursor_texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
			}
		}

		if (overlay.cursor_texture) {
			if (overlay.cursor_vao == 0) glGenVertexArrays(1, &overlay.cursor_vao);
			if (overlay.cursor_vbo == 0) glGenBuffers(1, &overlay.cursor_vbo);

			if (!init_osd_shader()) return; // Re-use OSD shader (simple texture shader)

			int cx, cy, cw, ch;
			p96_get_cursor_position(&cx, &cy);
			p96_get_cursor_dimensions(&cw, &ch);

			if (amiga_surface) {
				float surf_w = (float)amiga_surface->w;
				float surf_h = (float)amiga_surface->h;

				// Percentage of surface
				float px = (float)cx / surf_w;
				float py = (float)cy / surf_h;
				float pw = (float)cw / surf_w;
				float ph = (float)ch / surf_h;

				float x0 = px * 2.0f - 1.0f;
				float y0 = 1.0f - (py * 2.0f + ph * 2.0f); // Origin top-left, GL bottom-left.

				float y_top = 1.0f - py * 2.0f;
				float y_bottom = 1.0f - (py + ph) * 2.0f;

				float x_left = x0;
				float x_right = px * 2.0f - 1.0f + pw * 2.0f;

				GLfloat vertices[] = {
					x_left,  y_bottom, 0.0f, 1.0f,
					x_right, y_bottom, 1.0f, 1.0f,
					x_right, y_top,    1.0f, 0.0f,
					x_left,  y_top,    0.0f, 0.0f,
				};

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_SCISSOR_TEST);
				glViewport(x, y, w, h);

				glUseProgram(overlay.osd_program);
				if (overlay.osd_tex_loc != -1) glUniform1i(overlay.osd_tex_loc, 0);

				glBindVertexArray(overlay.cursor_vao);

				glEnableVertexAttribArray(0);
				glDisableVertexAttribArray(1);
				glDisableVertexAttribArray(2);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, overlay.cursor_texture);

				glBindBuffer(GL_ARRAY_BUFFER, overlay.cursor_vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
				glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

				glDisableVertexAttribArray(0);
				glBindVertexArray(0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glDisable(GL_BLEND);
				glUseProgram(0);
			}
		}
	}
}
#endif // USE_OPENGL

#endif // AMIBERRY
