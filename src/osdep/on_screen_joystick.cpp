/*
 * Amiberry - On-screen touch joystick controls
 *
 * Provides a virtual arcade-style joystick and fire buttons overlay for
 * touchscreen devices. The joystick has a movable ball-top knob that follows
 * the player's finger, providing intuitive slide-to-move input.
 *
 * Registered as a proper joystick input device so it appears in the Input
 * configuration panel and respects the selected port mode (joystick/mouse).
 *
 * Supports both SDL2 renderer and OpenGL rendering paths.
 *
 * Copyright 2025-2026 Amiberry team
 */

#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

#include <SDL.h>

#ifdef USE_OPENGL
#include "gl_platform.h"
#endif

#include "on_screen_joystick.h"
#include "sysdeps.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "options.h"
#include "amiberry_gfx.h"

// ---------------------------------------------------------------------------
// Configuration constants
// ---------------------------------------------------------------------------

// Joystick base size as fraction of the shorter screen dimension
static constexpr float DPAD_SIZE_FRACTION = 0.38f;
// Button size as fraction of the shorter screen dimension
static constexpr float BUTTON_SIZE_FRACTION = 0.22f;
// Vertical spacing between the two buttons as fraction of shorter dimension
static constexpr float BUTTON_GAP_FRACTION = 0.05f;
// Margin from screen edge as fraction of shorter dimension
static constexpr float EDGE_MARGIN_FRACTION = 0.02f;
// Texture resolution for procedurally-generated graphics
static constexpr int STICK_BASE_TEX_SIZE = 256;
static constexpr int STICK_KNOB_TEX_SIZE = 128;
static constexpr int BUTTON_TEX_SIZE = 128;
// Alpha values (0-255)
static constexpr Uint8 ALPHA_NORMAL = 160;
static constexpr Uint8 ALPHA_PRESSED = 220;
// Dead zone in the center of the joystick (fraction of radius)
static constexpr float DPAD_DEADZONE = 0.15f;
// Knob size as fraction of the base plate diameter
static constexpr float KNOB_SIZE_FRACTION = 0.40f;
// Maximum travel distance of knob center from base center (fraction of base radius)
static constexpr float KNOB_MAX_TRAVEL = 0.55f;
// Diagonal offset for button 2 relative to button 1 (fraction of button size)
static constexpr float BTN2_DIAG_X_OFFSET = 0.30f;
static constexpr float BTN2_DIAG_Y_OFFSET = 0.50f;

// ---------------------------------------------------------------------------
// Retro color palette
// ---------------------------------------------------------------------------

struct Color {
	Uint8 r, g, b, a;
};

// Joystick base plate colors - dark gunmetal concave well
static constexpr Color STICK_BASE_OUTER    = { 25,  25,  30, 255 };
static constexpr Color STICK_BASE_BEVEL    = { 50,  50,  56, 255 };
static constexpr Color STICK_BASE_WELL_IN  = { 30,  30,  35, 255 };
static constexpr Color STICK_BASE_WELL_OUT = { 45,  45,  52, 255 };
static constexpr Color STICK_DIR_DOT       = { 70,  70,  78, 180 };
static constexpr Color STICK_CENTER_DOT    = { 55,  55,  62, 255 };

// Joystick knob (ball-top) colors - classic red arcade
static constexpr Color KNOB_OUTER    = {140,  15,  15, 255 };
static constexpr Color KNOB_INNER    = {230,  55,  55, 255 };
static constexpr Color KNOB_GLINT    = {255, 180, 180, 255 };

// Button colors
static constexpr Color BTN1_OUTER     = {160,  20,  20, 255 };
static constexpr Color BTN1_INNER     = {220,  50,  50, 255 };
static constexpr Color BTN1_HIGHLIGHT = {255, 100, 100, 255 };
static constexpr Color BTN2_OUTER     = { 20,  40, 160, 255 };
static constexpr Color BTN2_INNER     = { 50,  70, 220, 255 };
static constexpr Color BTN2_HIGHLIGHT = {100, 130, 255, 255 };
// Keyboard button (green)
static constexpr Color BTNKB_OUTER     = { 20, 130,  40, 255 };
static constexpr Color BTNKB_INNER     = { 50, 180,  70, 255 };
static constexpr Color BTNKB_HIGHLIGHT = {100, 230, 130, 255 };
// Keyboard button size as fraction of shorter dimension (smaller than fire buttons)
static constexpr float KB_BUTTON_SIZE_FRACTION = 0.14f;

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

static bool osj_enabled = false;
static bool osj_initialized = false;

// SDL Textures (used for non-OpenGL rendering)
static SDL_Texture* stick_base_tex = nullptr;
static SDL_Texture* knob_tex = nullptr;
static SDL_Texture* btn1_tex = nullptr;
static SDL_Texture* btn2_tex = nullptr;
static SDL_Texture* btnkb_tex = nullptr;

// SDL Surfaces (kept around for GL texture upload)
static SDL_Surface* stick_base_surface = nullptr;
static SDL_Surface* knob_surface = nullptr;
static SDL_Surface* btn1_surface = nullptr;
static SDL_Surface* btn2_surface = nullptr;
static SDL_Surface* btnkb_surface = nullptr;

#ifdef USE_OPENGL
// OpenGL textures
static GLuint gl_stick_base_tex = 0;
static GLuint gl_knob_tex = 0;
static GLuint gl_btn1_tex = 0;
static GLuint gl_btn2_tex = 0;
static GLuint gl_btnkb_tex = 0;
static GLuint osj_gl_program = 0;
static GLint  osj_gl_tex_loc = -1;
static GLuint osj_gl_vao = 0;
static GLuint osj_gl_vbo = 0;
static bool   osj_gl_initialized = false;
#endif

// Layout rectangles on screen
static SDL_Rect dpad_rect = {};
static SDL_Rect btn1_rect = {};
static SDL_Rect btn2_rect = {};
static SDL_Rect btnkb_rect = {};

// Center and radius for hit-testing (in screen coords)
static int dpad_cx = 0, dpad_cy = 0, dpad_hit_radius = 0;
static int btn1_cx = 0, btn1_cy = 0, btn1_hit_radius = 0;
static int btn2_cx = 0, btn2_cy = 0, btn2_hit_radius = 0;
static int btnkb_cx = 0, btnkb_cy = 0, btnkb_hit_radius = 0;

// Joystick state
static bool joy_up = false, joy_down = false, joy_left = false, joy_right = false;
static bool joy_fire1 = false, joy_fire2 = false;
// Previous state for change detection
static bool prev_up = false, prev_down = false, prev_left = false, prev_right = false;
static bool prev_fire1 = false, prev_fire2 = false;
// Keyboard button tapped flag (consumed on read)
static bool kb_tapped = false;
static bool joy_kb_pressed = false;

// Joystick knob position tracking
// Normalized offset of the knob from the base center, in range [-1.0, +1.0]
static float knob_offset_x = 0.0f;
static float knob_offset_y = 0.0f;
static bool knob_active = false;

// Multi-touch tracking
enum ControlType { CTL_NONE, CTL_DPAD, CTL_BUTTON1, CTL_BUTTON2, CTL_KEYBOARD };

struct FingerTrack {
	SDL_FingerID id;
	ControlType control;
};
static std::vector<FingerTrack> active_fingers;

// Current screen dimensions (cached from last layout update)
static int screen_w = 0, screen_h = 0;
// Cached game rect for change detection
static SDL_Rect cached_game_rect = {};

// ---------------------------------------------------------------------------
// Helper: draw a filled circle on an SDL_Surface
// ---------------------------------------------------------------------------

static void fill_circle(SDL_Surface* s, int cx, int cy, int radius, Color col)
{
	Uint32 color = SDL_MapRGBA(s->format, col.r, col.g, col.b, col.a);
	auto* pixels = static_cast<Uint32*>(s->pixels);
	int pitch = s->pitch / 4;
	int r2 = radius * radius;

	int y0 = std::max(0, cy - radius);
	int y1 = std::min(s->h - 1, cy + radius);
	for (int y = y0; y <= y1; y++) {
		int dy = y - cy;
		int dx_max = static_cast<int>(std::sqrt(static_cast<float>(r2 - dy * dy)));
		int x0 = std::max(0, cx - dx_max);
		int x1 = std::min(s->w - 1, cx + dx_max);
		for (int x = x0; x <= x1; x++) {
			pixels[y * pitch + x] = color;
		}
	}
}

// ---------------------------------------------------------------------------
// Helper: draw a filled circle with a radial gradient (outer -> inner)
// ---------------------------------------------------------------------------

static void fill_circle_gradient(SDL_Surface* s, int cx, int cy, int radius,
	Color outer, Color inner, Color glint)
{
	auto* pixels = static_cast<Uint32*>(s->pixels);
	int pitch = s->pitch / 4;
	float r = static_cast<float>(radius);

	int y0 = std::max(0, cy - radius);
	int y1 = std::min(s->h - 1, cy + radius);
	for (int y = y0; y <= y1; y++) {
		int dy = y - cy;
		int dx_max = static_cast<int>(std::sqrt(r * r - static_cast<float>(dy * dy)));
		int x0 = std::max(0, cx - dx_max);
		int x1 = std::min(s->w - 1, cx + dx_max);
		for (int x = x0; x <= x1; x++) {
			int dx = x - cx;
			float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
			float t = dist / r;

			Uint8 cr = static_cast<Uint8>(inner.r + (outer.r - inner.r) * t);
			Uint8 cg = static_cast<Uint8>(inner.g + (outer.g - inner.g) * t);
			Uint8 cb = static_cast<Uint8>(inner.b + (outer.b - inner.b) * t);
			Uint8 ca = static_cast<Uint8>(inner.a + (outer.a - inner.a) * t);

			float highlight = std::max(0.0f, 1.0f - dist / (r * 0.5f));
			float angle = std::atan2(static_cast<float>(dy), static_cast<float>(dx));
			float dir_factor = std::max(0.0f, -std::cos(angle + 0.7f)) * highlight * 0.4f;
			cr = static_cast<Uint8>(std::min(255.0f, cr + (glint.r - cr) * dir_factor));
			cg = static_cast<Uint8>(std::min(255.0f, cg + (glint.g - cg) * dir_factor));
			cb = static_cast<Uint8>(std::min(255.0f, cb + (glint.b - cb) * dir_factor));

			pixels[y * pitch + x] = SDL_MapRGBA(s->format, cr, cg, cb, ca);
		}
	}
}

// ---------------------------------------------------------------------------
// Helper: draw a filled rectangle on an SDL_Surface
// ---------------------------------------------------------------------------

static void fill_rect(SDL_Surface* s, int x0, int y0, int w, int h, Color col)
{
	SDL_Rect r = { x0, y0, w, h };
	Uint32 color = SDL_MapRGBA(s->format, col.r, col.g, col.b, col.a);
	SDL_FillRect(s, &r, color);
}

// ---------------------------------------------------------------------------
// Surface generation: joystick base plate (concave well)
// ---------------------------------------------------------------------------

static SDL_Surface* create_stick_base_surface()
{
	const int sz = STICK_BASE_TEX_SIZE;
	// ABGR8888 stores bytes as [R,G,B,A] in memory on little-endian,
	// which matches GL_RGBA + GL_UNSIGNED_BYTE for OpenGL upload.
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_ABGR8888);
	if (!surface) return nullptr;

	SDL_FillRect(surface, nullptr, 0);

	int cx = sz / 2;
	int cy = sz / 2;
	int base_r = sz / 2 - 4;

	// Dark outer ring
	fill_circle(surface, cx, cy, base_r, STICK_BASE_OUTER);

	// Bevel ring (lighter just inside the outer edge)
	fill_circle(surface, cx, cy, base_r - 2, STICK_BASE_BEVEL);

	// Inner well with radial gradient (concave look: darker center, lighter edges)
	{
		int well_r = base_r - 5;
		auto* pixels = static_cast<Uint32*>(surface->pixels);
		int pitch = surface->pitch / 4;
		float fr = static_cast<float>(well_r);

		int y0 = std::max(0, cy - well_r);
		int y1 = std::min(sz - 1, cy + well_r);
		for (int y = y0; y <= y1; y++) {
			int dy = y - cy;
			int dx_max = static_cast<int>(std::sqrt(fr * fr - static_cast<float>(dy * dy)));
			int x0 = std::max(0, cx - dx_max);
			int x1 = std::min(sz - 1, cx + dx_max);
			for (int x = x0; x <= x1; x++) {
				int dx = x - cx;
				float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
				float t = dist / fr; // 0 at center, 1 at edge

				// Gradient: center is darker (well_in), edge is lighter (well_out)
				Uint8 cr = static_cast<Uint8>(STICK_BASE_WELL_IN.r + (STICK_BASE_WELL_OUT.r - STICK_BASE_WELL_IN.r) * t);
				Uint8 cg = static_cast<Uint8>(STICK_BASE_WELL_IN.g + (STICK_BASE_WELL_OUT.g - STICK_BASE_WELL_IN.g) * t);
				Uint8 cb = static_cast<Uint8>(STICK_BASE_WELL_IN.b + (STICK_BASE_WELL_OUT.b - STICK_BASE_WELL_IN.b) * t);

				pixels[y * pitch + x] = SDL_MapRGBA(surface->format, cr, cg, cb, 255);
			}
		}
	}

	// 8 direction indicator dots at the cardinal and diagonal positions
	{
		static constexpr float PI = 3.14159265f;
		int dot_r = sz / 64;
		if (dot_r < 2) dot_r = 2;
		float dot_dist = (base_r - 5) * 0.70f;

		for (int i = 0; i < 8; i++) {
			float angle = i * PI / 4.0f;
			int dx = cx + static_cast<int>(std::cos(angle) * dot_dist);
			int dy = cy + static_cast<int>(std::sin(angle) * dot_dist);
			fill_circle(surface, dx, dy, dot_r, STICK_DIR_DOT);
		}
	}

	// Center position marker
	fill_circle(surface, cx, cy, sz / 16, STICK_CENTER_DOT);

	return surface;
}

// ---------------------------------------------------------------------------
// Surface generation: joystick knob (red ball-top)
// ---------------------------------------------------------------------------

static SDL_Surface* create_stick_knob_surface()
{
	const int sz = STICK_KNOB_TEX_SIZE;
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_ABGR8888);
	if (!surface) return nullptr;

	SDL_FillRect(surface, nullptr, 0);

	int cx = sz / 2;
	int cy = sz / 2;
	int r = sz / 2 - 4;

	// Main ball with radial gradient (dark red outer -> bright red inner)
	// and a directional glint for 3D appearance
	fill_circle_gradient(surface, cx, cy, r, KNOB_OUTER, KNOB_INNER, KNOB_GLINT);

	// Specular highlight dot (upper-left quadrant for 3D sphere look)
	{
		int spec_cx = cx - static_cast<int>(r * 0.25f);
		int spec_cy = cy - static_cast<int>(r * 0.25f);
		int spec_r = static_cast<int>(r * 0.15f);
		if (spec_r < 2) spec_r = 2;

		auto* pixels = static_cast<Uint32*>(surface->pixels);
		int pitch = surface->pitch / 4;
		float fspec_r = static_cast<float>(spec_r);

		int y0 = std::max(0, spec_cy - spec_r);
		int y1 = std::min(sz - 1, spec_cy + spec_r);
		for (int y = y0; y <= y1; y++) {
			int dy = y - spec_cy;
			int dx_max = static_cast<int>(std::sqrt(fspec_r * fspec_r - static_cast<float>(dy * dy)));
			int x0 = std::max(0, spec_cx - dx_max);
			int x1 = std::min(sz - 1, spec_cx + dx_max);
			for (int x = x0; x <= x1; x++) {
				int dx = x - spec_cx;
				float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
				float t = dist / fspec_r;
				// Fade from bright center to transparent edge
				Uint8 alpha = static_cast<Uint8>(200.0f * (1.0f - t));
				// Blend specular onto existing pixel
				Uint32 existing = pixels[y * pitch + x];
				Uint8 er, eg, eb, ea;
				SDL_GetRGBA(existing, surface->format, &er, &eg, &eb, &ea);
				if (ea > 0) {
					float a = alpha / 255.0f;
					er = static_cast<Uint8>(std::min(255.0f, er + (255 - er) * a));
					eg = static_cast<Uint8>(std::min(255.0f, eg + (220 - eg) * a));
					eb = static_cast<Uint8>(std::min(255.0f, eb + (220 - eb) * a));
					pixels[y * pitch + x] = SDL_MapRGBA(surface->format, er, eg, eb, ea);
				}
			}
		}
	}

	// Subtle bottom shadow crescent for depth
	{
		auto* pixels = static_cast<Uint32*>(surface->pixels);
		int pitch = surface->pitch / 4;
		float fr = static_cast<float>(r);

		int y0 = cy + static_cast<int>(r * 0.7f);
		int y1 = std::min(sz - 1, cy + r);
		for (int y = y0; y <= y1; y++) {
			int dy = y - cy;
			int dx_max = static_cast<int>(std::sqrt(fr * fr - static_cast<float>(dy * dy)));
			int x0 = std::max(0, cx - dx_max);
			int x1 = std::min(sz - 1, cx + dx_max);
			for (int x = x0; x <= x1; x++) {
				float shadow_t = static_cast<float>(y - y0) / static_cast<float>(y1 - y0 + 1);
				Uint32 existing = pixels[y * pitch + x];
				Uint8 er, eg, eb, ea;
				SDL_GetRGBA(existing, surface->format, &er, &eg, &eb, &ea);
				if (ea > 0) {
					float darken = shadow_t * 0.35f;
					er = static_cast<Uint8>(er * (1.0f - darken));
					eg = static_cast<Uint8>(eg * (1.0f - darken));
					eb = static_cast<Uint8>(eb * (1.0f - darken));
					pixels[y * pitch + x] = SDL_MapRGBA(surface->format, er, eg, eb, ea);
				}
			}
		}
	}

	return surface;
}

// ---------------------------------------------------------------------------
// Helper: draw a simple pixel-art character on an SDL_Surface
// Each character is defined as a 5x7 bitmap.
// ---------------------------------------------------------------------------

static const uint8_t font_K[7] = {
	0b10010,
	0b10100,
	0b11000,
	0b10000,
	0b11000,
	0b10100,
	0b10010,
};

static const uint8_t font_B[7] = {
	0b11100,
	0b10010,
	0b10010,
	0b11100,
	0b10010,
	0b10010,
	0b11100,
};

static void draw_char(SDL_Surface* s, const uint8_t bitmap[7], int ox, int oy, int scale, Color col)
{
	Uint32 color = SDL_MapRGBA(s->format, col.r, col.g, col.b, col.a);
	auto* pixels = static_cast<Uint32*>(s->pixels);
	int pitch = s->pitch / 4;
	for (int row = 0; row < 7; row++) {
		for (int col_idx = 0; col_idx < 5; col_idx++) {
			if (bitmap[row] & (1 << (4 - col_idx))) {
				for (int sy = 0; sy < scale; sy++) {
					for (int sx = 0; sx < scale; sx++) {
						int x = ox + col_idx * scale + sx;
						int y = oy + row * scale + sy;
						if (x >= 0 && x < s->w && y >= 0 && y < s->h)
							pixels[y * pitch + x] = color;
					}
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Surface generation: arcade dome button
// ---------------------------------------------------------------------------

static SDL_Surface* create_button_surface(Color outer, Color inner, Color glint)
{
	const int sz = BUTTON_TEX_SIZE;
	// ABGR8888 stores bytes as [R,G,B,A] in memory on little-endian,
	// which matches GL_RGBA + GL_UNSIGNED_BYTE for OpenGL upload.
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_ABGR8888);
	if (!surface) return nullptr;

	SDL_FillRect(surface, nullptr, 0);

	int cx = sz / 2;
	int cy = sz / 2;
	int r = sz / 2 - 4;

	// Dark housing/collar ring (arcade button surround)
	Color housing = { 20, 20, 22, 255 };
	fill_circle(surface, cx, cy, r, housing);

	// Outer ring (bezel) - wider than before for more prominent dome look
	Color bezel = { static_cast<Uint8>(outer.r / 2), static_cast<Uint8>(outer.g / 2),
		static_cast<Uint8>(outer.b / 2), 255 };
	fill_circle(surface, cx, cy, r - 3, bezel);

	// Main button surface with gradient (dome)
	fill_circle_gradient(surface, cx, cy, r - 6, outer, inner, glint);

	// Second specular highlight dot for more pronounced dome reflection
	{
		int spec_cx = cx - static_cast<int>((r - 6) * 0.22f);
		int spec_cy = cy - static_cast<int>((r - 6) * 0.22f);
		int spec_r = static_cast<int>((r - 6) * 0.12f);
		if (spec_r < 2) spec_r = 2;

		auto* pixels = static_cast<Uint32*>(surface->pixels);
		int pitch = surface->pitch / 4;
		float fspec_r = static_cast<float>(spec_r);

		int y0 = std::max(0, spec_cy - spec_r);
		int y1 = std::min(sz - 1, spec_cy + spec_r);
		for (int y = y0; y <= y1; y++) {
			int dy = y - spec_cy;
			int dx_max = static_cast<int>(std::sqrt(fspec_r * fspec_r - static_cast<float>(dy * dy)));
			int x0 = std::max(0, spec_cx - dx_max);
			int x1 = std::min(sz - 1, spec_cx + dx_max);
			for (int x = x0; x <= x1; x++) {
				int dx = x - spec_cx;
				float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
				float t = dist / fspec_r;
				Uint8 alpha = static_cast<Uint8>(150.0f * (1.0f - t));
				Uint32 existing = pixels[y * pitch + x];
				Uint8 er, eg, eb, ea;
				SDL_GetRGBA(existing, surface->format, &er, &eg, &eb, &ea);
				if (ea > 0) {
					float a = alpha / 255.0f;
					er = static_cast<Uint8>(std::min(255.0f, er + (255 - er) * a));
					eg = static_cast<Uint8>(std::min(255.0f, eg + (255 - eg) * a));
					eb = static_cast<Uint8>(std::min(255.0f, eb + (255 - eb) * a));
					pixels[y * pitch + x] = SDL_MapRGBA(surface->format, er, eg, eb, ea);
				}
			}
		}
	}

	// Subtle bottom shadow arc for depth
	{
		auto* pixels = static_cast<Uint32*>(surface->pixels);
		int pitch = surface->pitch / 4;
		int inner_r = r - 6;
		float fr = static_cast<float>(inner_r);

		int y0 = cy + static_cast<int>(inner_r * 0.65f);
		int y1 = std::min(sz - 1, cy + inner_r);
		for (int y = y0; y <= y1; y++) {
			int dy = y - cy;
			int dx_max = static_cast<int>(std::sqrt(fr * fr - static_cast<float>(dy * dy)));
			int x0 = std::max(0, cx - dx_max);
			int x1 = std::min(sz - 1, cx + dx_max);
			for (int x = x0; x <= x1; x++) {
				float shadow_t = static_cast<float>(y - y0) / static_cast<float>(y1 - y0 + 1);
				Uint32 existing = pixels[y * pitch + x];
				Uint8 er, eg, eb, ea;
				SDL_GetRGBA(existing, surface->format, &er, &eg, &eb, &ea);
				if (ea > 0) {
					float darken = shadow_t * 0.25f;
					er = static_cast<Uint8>(er * (1.0f - darken));
					eg = static_cast<Uint8>(eg * (1.0f - darken));
					eb = static_cast<Uint8>(eb * (1.0f - darken));
					pixels[y * pitch + x] = SDL_MapRGBA(surface->format, er, eg, eb, ea);
				}
			}
		}
	}

	return surface;
}

// ---------------------------------------------------------------------------
// Surface generation: keyboard button (green with "KB" label)
// ---------------------------------------------------------------------------

static SDL_Surface* create_kb_button_surface()
{
	const int sz = BUTTON_TEX_SIZE;
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_ABGR8888);
	if (!surface) return nullptr;

	SDL_FillRect(surface, nullptr, 0);

	int cx = sz / 2;
	int cy = sz / 2;
	int r = sz / 2 - 4;

	// Dark housing/collar ring
	Color housing = { 20, 20, 22, 255 };
	fill_circle(surface, cx, cy, r, housing);

	// Outer ring (bezel)
	Color bezel = { static_cast<Uint8>(BTNKB_OUTER.r / 2), static_cast<Uint8>(BTNKB_OUTER.g / 2),
		static_cast<Uint8>(BTNKB_OUTER.b / 2), 255 };
	fill_circle(surface, cx, cy, r - 3, bezel);

	// Main button surface with gradient
	fill_circle_gradient(surface, cx, cy, r - 6, BTNKB_OUTER, BTNKB_INNER, BTNKB_HIGHLIGHT);

	// Draw "KB" text centered on the button
	Color text_col = { 255, 255, 255, 255 };
	int char_scale = sz / 32;  // Scale factor for the 5x7 font
	if (char_scale < 1) char_scale = 1;
	int char_w = 5 * char_scale;
	int char_h = 7 * char_scale;
	int gap = char_scale;  // Gap between K and B
	int total_w = char_w * 2 + gap;
	int text_x = cx - total_w / 2;
	int text_y = cy - char_h / 2;
	draw_char(surface, font_K, text_x, text_y, char_scale, text_col);
	draw_char(surface, font_B, text_x + char_w + gap, text_y, char_scale, text_col);

	return surface;
}

// ---------------------------------------------------------------------------
// Input injection helpers
// ---------------------------------------------------------------------------

// On-screen joystick button indices (within the registered device).
// Must match the layout set up in init_joystick() in amiberry_input.cpp.
// Buttons are at widget offset FIRST_JOY_BUTTON (== MAX_JOY_AXES == 8).
static constexpr int OSJ_BTN_FIRE1 = 0;
static constexpr int OSJ_BTN_FIRE2 = 1;

static void inject_directions()
{
	int dev = get_onscreen_joystick_device_index();
	if (dev < 0) return;

	// Compute axis values from directional booleans.
	// X axis: left = -32767, right = +32767, neither = 0
	// Y axis: up   = -32767, down  = +32767, neither = 0
	constexpr int AXIS_MAX = 32767;

	bool x_changed = (joy_left != prev_left) || (joy_right != prev_right);
	bool y_changed = (joy_up != prev_up) || (joy_down != prev_down);

	if (x_changed) {
		int x = 0;
		if (joy_left)  x = -AXIS_MAX;
		if (joy_right) x =  AXIS_MAX;
		setjoystickstate(dev, 0, x, AXIS_MAX);
		prev_left = joy_left;
		prev_right = joy_right;
	}
	if (y_changed) {
		int y = 0;
		if (joy_up)   y = -AXIS_MAX;
		if (joy_down) y =  AXIS_MAX;
		setjoystickstate(dev, 1, y, AXIS_MAX);
		prev_up = joy_up;
		prev_down = joy_down;
	}
}

static void inject_buttons()
{
	int dev = get_onscreen_joystick_device_index();
	if (dev < 0) return;

	if (joy_fire1 != prev_fire1) {
		setjoybuttonstate(dev, OSJ_BTN_FIRE1, joy_fire1 ? 1 : 0);
		prev_fire1 = joy_fire1;
	}
	if (joy_fire2 != prev_fire2) {
		setjoybuttonstate(dev, OSJ_BTN_FIRE2, joy_fire2 ? 1 : 0);
		prev_fire2 = joy_fire2;
	}
}

// ---------------------------------------------------------------------------
// Joystick direction from touch position (with knob tracking)
// ---------------------------------------------------------------------------

static void update_dpad_from_position(int touch_x, int touch_y)
{
	int dx = touch_x - dpad_cx;
	int dy = touch_y - dpad_cy;
	float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
	float max_travel = dpad_hit_radius * KNOB_MAX_TRAVEL;

	// Update knob visual position (clamped to max travel radius)
	if (dist > max_travel) {
		float scale = max_travel / dist;
		knob_offset_x = (dx * scale) / max_travel;
		knob_offset_y = (dy * scale) / max_travel;
	} else if (dist > 0.001f) {
		knob_offset_x = static_cast<float>(dx) / max_travel;
		knob_offset_y = static_cast<float>(dy) / max_travel;
	} else {
		knob_offset_x = 0.0f;
		knob_offset_y = 0.0f;
	}

	// Deadzone for digital direction output
	float deadzone_px = dpad_hit_radius * DPAD_DEADZONE;
	if (dist < deadzone_px) {
		joy_up = joy_down = joy_left = joy_right = false;
		inject_directions();
		return;
	}

	float angle = std::atan2(static_cast<float>(dy), static_cast<float>(dx));

	static constexpr float PI = 3.14159265f;
	static constexpr float SECTOR = PI / 4.0f;
	static constexpr float HALF_SECTOR = PI / 8.0f;

	float a = angle;
	if (a < 0) a += 2.0f * PI;
	int sector = static_cast<int>((a + HALF_SECTOR) / SECTOR) % 8;

	joy_right = (sector == 0 || sector == 1 || sector == 7);
	joy_left  = (sector == 3 || sector == 4 || sector == 5);
	joy_down  = (sector == 1 || sector == 2 || sector == 3);
	joy_up    = (sector == 5 || sector == 6 || sector == 7);

	inject_directions();
}

static void release_dpad()
{
	joy_up = joy_down = joy_left = joy_right = false;
	knob_offset_x = 0.0f;
	knob_offset_y = 0.0f;
	knob_active = false;
	inject_directions();
}

static void release_button(ControlType ctl)
{
	if (ctl == CTL_BUTTON1) {
		joy_fire1 = false;
	} else if (ctl == CTL_BUTTON2) {
		joy_fire2 = false;
	} else if (ctl == CTL_KEYBOARD) {
		joy_kb_pressed = false;
		return;  // No Amiga input to inject
	}
	inject_buttons();
}

// ---------------------------------------------------------------------------
// Knob position helper for rendering
// ---------------------------------------------------------------------------

static SDL_Rect compute_knob_rect()
{
	int knob_size = static_cast<int>(dpad_rect.w * KNOB_SIZE_FRACTION);
	float max_travel_px = dpad_hit_radius * KNOB_MAX_TRAVEL;

	int knob_cx = dpad_cx + static_cast<int>(knob_offset_x * max_travel_px);
	int knob_cy = dpad_cy + static_cast<int>(knob_offset_y * max_travel_px);

	SDL_Rect r;
	r.x = knob_cx - knob_size / 2;
	r.y = knob_cy - knob_size / 2;
	r.w = knob_size;
	r.h = knob_size;
	return r;
}

// ---------------------------------------------------------------------------
// Finger tracking helpers
// ---------------------------------------------------------------------------

static FingerTrack* find_finger(SDL_FingerID id)
{
	for (auto& f : active_fingers) {
		if (f.id == id) return &f;
	}
	return nullptr;
}

static void remove_finger(SDL_FingerID id)
{
	active_fingers.erase(
		std::remove_if(active_fingers.begin(), active_fingers.end(),
			[id](const FingerTrack& f) { return f.id == id; }),
		active_fingers.end());
}

static ControlType hit_test(int px, int py)
{
	{
		int dx = px - dpad_cx;
		int dy = py - dpad_cy;
		if (dx * dx + dy * dy <= dpad_hit_radius * dpad_hit_radius)
			return CTL_DPAD;
	}
	{
		int dx = px - btn1_cx;
		int dy = py - btn1_cy;
		if (dx * dx + dy * dy <= btn1_hit_radius * btn1_hit_radius)
			return CTL_BUTTON1;
	}
	{
		int dx = px - btn2_cx;
		int dy = py - btn2_cy;
		if (dx * dx + dy * dy <= btn2_hit_radius * btn2_hit_radius)
			return CTL_BUTTON2;
	}
	{
		int dx = px - btnkb_cx;
		int dy = py - btnkb_cy;
		if (dx * dx + dy * dy <= btnkb_hit_radius * btnkb_hit_radius)
			return CTL_KEYBOARD;
	}
	return CTL_NONE;
}

// ---------------------------------------------------------------------------
// OpenGL shader setup (simple textured quad, same as OSD shader)
// ---------------------------------------------------------------------------

#ifdef USE_OPENGL
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

static GLint osj_gl_alpha_loc = -1;

static bool init_osj_gl_shader()
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

static GLuint upload_surface_to_gl(SDL_Surface* surface)
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

// Render a single textured quad using OpenGL.
// rect = screen-space rectangle, drawable_w/h = full drawable dimensions.
// OpenGL has Y=0 at bottom, so we flip.
static void render_gl_quad(GLuint texture, const SDL_Rect& rect, int drawable_w, int drawable_h, float alpha_val)
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

static void cleanup_osj_gl()
{
	if (gl_stick_base_tex) { glDeleteTextures(1, &gl_stick_base_tex); gl_stick_base_tex = 0; }
	if (gl_knob_tex) { glDeleteTextures(1, &gl_knob_tex); gl_knob_tex = 0; }
	if (gl_btn1_tex) { glDeleteTextures(1, &gl_btn1_tex); gl_btn1_tex = 0; }
	if (gl_btn2_tex) { glDeleteTextures(1, &gl_btn2_tex); gl_btn2_tex = 0; }
	if (gl_btnkb_tex) { glDeleteTextures(1, &gl_btnkb_tex); gl_btnkb_tex = 0; }
	if (osj_gl_vbo) { glDeleteBuffers(1, &osj_gl_vbo); osj_gl_vbo = 0; }
	if (osj_gl_vao) { glDeleteVertexArrays(1, &osj_gl_vao); osj_gl_vao = 0; }
	if (osj_gl_program) { glDeleteProgram(osj_gl_program); osj_gl_program = 0; }
	osj_gl_initialized = false;
}
#endif // USE_OPENGL

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void on_screen_joystick_init(SDL_Renderer* renderer)
{
	if (osj_initialized) {
		on_screen_joystick_quit();
	}

	// Generate surfaces
	stick_base_surface = create_stick_base_surface();
	knob_surface = create_stick_knob_surface();
	btn1_surface = create_button_surface(BTN1_OUTER, BTN1_INNER, BTN1_HIGHLIGHT);
	btn2_surface = create_button_surface(BTN2_OUTER, BTN2_INNER, BTN2_HIGHLIGHT);
	btnkb_surface = create_kb_button_surface();

#ifndef USE_OPENGL
	// Create SDL textures from surfaces (only when using SDL2 renderer)
	if (renderer) {
		if (stick_base_surface) {
			stick_base_tex = SDL_CreateTextureFromSurface(renderer, stick_base_surface);
			if (stick_base_tex) SDL_SetTextureBlendMode(stick_base_tex, SDL_BLENDMODE_BLEND);
		}
		if (knob_surface) {
			knob_tex = SDL_CreateTextureFromSurface(renderer, knob_surface);
			if (knob_tex) SDL_SetTextureBlendMode(knob_tex, SDL_BLENDMODE_BLEND);
		}
		if (btn1_surface) {
			btn1_tex = SDL_CreateTextureFromSurface(renderer, btn1_surface);
			if (btn1_tex) SDL_SetTextureBlendMode(btn1_tex, SDL_BLENDMODE_BLEND);
		}
		if (btn2_surface) {
			btn2_tex = SDL_CreateTextureFromSurface(renderer, btn2_surface);
			if (btn2_tex) SDL_SetTextureBlendMode(btn2_tex, SDL_BLENDMODE_BLEND);
		}
		if (btnkb_surface) {
			btnkb_tex = SDL_CreateTextureFromSurface(renderer, btnkb_surface);
			if (btnkb_tex) SDL_SetTextureBlendMode(btnkb_tex, SDL_BLENDMODE_BLEND);
		}
	}
#endif

	// Reset state
	joy_up = joy_down = joy_left = joy_right = false;
	joy_fire1 = joy_fire2 = false;
	prev_up = prev_down = prev_left = prev_right = false;
	prev_fire1 = prev_fire2 = false;
	knob_offset_x = 0.0f;
	knob_offset_y = 0.0f;
	knob_active = false;
	active_fingers.clear();

	osj_initialized = true;
}

void on_screen_joystick_quit()
{
	// Release any held inputs
	if (joy_up || joy_down || joy_left || joy_right) {
		joy_up = joy_down = joy_left = joy_right = false;
		inject_directions();
	}
	if (joy_fire1 || joy_fire2) {
		joy_fire1 = joy_fire2 = false;
		inject_buttons();
	}

	if (stick_base_tex) { SDL_DestroyTexture(stick_base_tex); stick_base_tex = nullptr; }
	if (knob_tex) { SDL_DestroyTexture(knob_tex); knob_tex = nullptr; }
	if (btn1_tex) { SDL_DestroyTexture(btn1_tex); btn1_tex = nullptr; }
	if (btn2_tex) { SDL_DestroyTexture(btn2_tex); btn2_tex = nullptr; }
	if (btnkb_tex) { SDL_DestroyTexture(btnkb_tex); btnkb_tex = nullptr; }

	if (stick_base_surface) { SDL_FreeSurface(stick_base_surface); stick_base_surface = nullptr; }
	if (knob_surface) { SDL_FreeSurface(knob_surface); knob_surface = nullptr; }
	if (btn1_surface) { SDL_FreeSurface(btn1_surface); btn1_surface = nullptr; }
	if (btn2_surface) { SDL_FreeSurface(btn2_surface); btn2_surface = nullptr; }
	if (btnkb_surface) { SDL_FreeSurface(btnkb_surface); btnkb_surface = nullptr; }

	knob_offset_x = 0.0f;
	knob_offset_y = 0.0f;
	knob_active = false;

#ifdef USE_OPENGL
	cleanup_osj_gl();
#endif

	active_fingers.clear();
	osj_initialized = false;
}

bool on_screen_joystick_is_enabled()
{
	return osj_enabled;
}

void on_screen_joystick_set_enabled(bool enabled)
{
	osj_enabled = enabled;
	if (enabled) {
		// Auto-assign the on-screen joystick to Port 1 (Amiga joystick port)
		// so it is recognized by the input system as a proper joystick device.
		int dev = get_onscreen_joystick_device_index();
		if (dev >= 0) {
			int target_id = JSEM_JOYS + dev;
			if (changed_prefs.jports[1].id != target_id) {
				changed_prefs.jports[1].id = target_id;
				changed_prefs.jports[1].mode = JSEM_MODE_JOYSTICK;
				// Also update currprefs immediately so input routing works
				// right away, without waiting for the next config sync cycle.
				currprefs.jports[1].id = target_id;
				currprefs.jports[1].mode = JSEM_MODE_JOYSTICK;
				inputdevice_config_change();
			}
		}
	}
	else if (osj_initialized) {
		joy_up = joy_down = joy_left = joy_right = false;
		joy_fire1 = joy_fire2 = false;
		joy_kb_pressed = false;
		kb_tapped = false;
		knob_offset_x = 0.0f;
		knob_offset_y = 0.0f;
		knob_active = false;
		inject_directions();
		inject_buttons();
		active_fingers.clear();
	}
}

bool on_screen_joystick_keyboard_tapped()
{
	if (kb_tapped) {
		kb_tapped = false;
		return true;
	}
	return false;
}

void on_screen_joystick_update_layout(int sw, int sh, const SDL_Rect& game_rect)
{
	screen_w = sw;
	screen_h = sh;

	int shorter = std::min(sw, sh);

	int dpad_size = static_cast<int>(shorter * DPAD_SIZE_FRACTION);
	int margin = static_cast<int>(shorter * EDGE_MARGIN_FRACTION);

	// Place joystick base on the left side, vertically centered
	int left_space = game_rect.x;
	if (left_space > dpad_size + margin * 2) {
		dpad_rect.x = (left_space - dpad_size) / 2;
	} else {
		dpad_rect.x = margin;
	}
	dpad_rect.y = (sh - dpad_size) / 2;
	dpad_rect.w = dpad_size;
	dpad_rect.h = dpad_size;

	dpad_cx = dpad_rect.x + dpad_size / 2;
	dpad_cy = dpad_rect.y + dpad_size / 2;
	dpad_hit_radius = dpad_size / 2;

	// Button dimensions
	int btn_size = static_cast<int>(shorter * BUTTON_SIZE_FRACTION);
	int btn_gap = static_cast<int>(shorter * BUTTON_GAP_FRACTION);

	// Place buttons on the right side with diagonal offset
	int right_start = game_rect.x + game_rect.w;
	int right_space = sw - right_start;

	// Calculate diagonal offset
	int diag_x = static_cast<int>(btn_size * BTN2_DIAG_X_OFFSET);
	int diag_y = static_cast<int>(btn_size * BTN2_DIAG_Y_OFFSET);

	// Total width needed for diagonal layout
	int total_btn_w = btn_size + diag_x;
	int btn_x;
	if (right_space > total_btn_w + margin * 2) {
		btn_x = right_start + (right_space - total_btn_w) / 2;
	} else {
		btn_x = sw - total_btn_w - margin;
	}

	int total_btn_height = btn_size + diag_y + btn_gap;
	int btn_y_start = (sh - total_btn_height) / 2;

	// Button 1 (upper-left of the pair)
	btn1_rect.x = btn_x;
	btn1_rect.y = btn_y_start;
	btn1_rect.w = btn_size;
	btn1_rect.h = btn_size;

	// Button 2 (lower-right, diagonal offset)
	btn2_rect.x = btn_x + diag_x;
	btn2_rect.y = btn_y_start + diag_y + btn_gap;
	btn2_rect.w = btn_size;
	btn2_rect.h = btn_size;

	btn1_cx = btn1_rect.x + btn_size / 2;
	btn1_cy = btn1_rect.y + btn_size / 2;
	btn1_hit_radius = btn_size / 2 + btn_size / 8;

	btn2_cx = btn2_rect.x + btn_size / 2;
	btn2_cy = btn2_rect.y + btn_size / 2;
	btn2_hit_radius = btn_size / 2 + btn_size / 8;

	// Keyboard button: smaller, placed below the button pair
	int kb_size = static_cast<int>(shorter * KB_BUTTON_SIZE_FRACTION);
	// Center KB button horizontally between the two fire buttons
	int kb_x = (btn1_rect.x + btn2_rect.x + btn_size) / 2 - kb_size / 2;
	int kb_y = btn2_rect.y + btn_size + btn_gap;

	btnkb_rect.x = kb_x;
	btnkb_rect.y = kb_y;
	btnkb_rect.w = kb_size;
	btnkb_rect.h = kb_size;

	btnkb_cx = btnkb_rect.x + kb_size / 2;
	btnkb_cy = btnkb_rect.y + kb_size / 2;
	btnkb_hit_radius = kb_size / 2 + kb_size / 8;
}

void on_screen_joystick_redraw(SDL_Renderer* renderer)
{
	if (!osj_enabled || !osj_initialized) return;
	if (!stick_base_tex || !knob_tex || !btn1_tex || !btn2_tex) return;

	// Auto-recalculate layout when screen geometry changes
	{
		int sw = 0, sh = 0;
		SDL_GetRendererOutputSize(renderer, &sw, &sh);
		if (sw > 0 && sh > 0 && render_quad.w > 0 && render_quad.h > 0) {
			if (sw != screen_w || sh != screen_h ||
				render_quad.x != cached_game_rect.x || render_quad.y != cached_game_rect.y ||
				render_quad.w != cached_game_rect.w || render_quad.h != cached_game_rect.h)
			{
				on_screen_joystick_update_layout(sw, sh, render_quad);
				cached_game_rect = render_quad;
			}
		}
	}

	// Joystick base plate
	{
		Uint8 alpha = knob_active ? ALPHA_PRESSED : ALPHA_NORMAL;
		SDL_SetTextureAlphaMod(stick_base_tex, alpha);
		SDL_SetTextureColorMod(stick_base_tex, 255, 255, 255);
		SDL_RenderCopy(renderer, stick_base_tex, nullptr, &dpad_rect);
	}

	// Joystick knob (ball-top) - rendered at offset position
	{
		SDL_Rect knob_rect = compute_knob_rect();

		// Drop shadow when active
		if (knob_active) {
			SDL_Rect shadow_rect = knob_rect;
			shadow_rect.x += 2;
			shadow_rect.y += 3;
			shadow_rect.w += 2;
			shadow_rect.h += 2;
			SDL_SetTextureAlphaMod(knob_tex, 60);
			SDL_SetTextureColorMod(knob_tex, 0, 0, 0);
			SDL_RenderCopy(renderer, knob_tex, nullptr, &shadow_rect);
		}

		// The knob itself
		Uint8 knob_alpha = knob_active ? 240 : ALPHA_NORMAL;
		SDL_SetTextureAlphaMod(knob_tex, knob_alpha);
		SDL_SetTextureColorMod(knob_tex, 255, 255, 255);
		SDL_RenderCopy(renderer, knob_tex, nullptr, &knob_rect);
	}

	// Button 1 (red / fire)
	{
		Uint8 alpha = joy_fire1 ? ALPHA_PRESSED : ALPHA_NORMAL;
		SDL_SetTextureAlphaMod(btn1_tex, alpha);
		if (joy_fire1) {
			SDL_SetTextureColorMod(btn1_tex, 255, 200, 200);
		} else {
			SDL_SetTextureColorMod(btn1_tex, 255, 255, 255);
		}
		SDL_RenderCopy(renderer, btn1_tex, nullptr, &btn1_rect);
	}

	// Button 2 (blue / secondary fire)
	{
		Uint8 alpha = joy_fire2 ? ALPHA_PRESSED : ALPHA_NORMAL;
		SDL_SetTextureAlphaMod(btn2_tex, alpha);
		if (joy_fire2) {
			SDL_SetTextureColorMod(btn2_tex, 200, 200, 255);
		} else {
			SDL_SetTextureColorMod(btn2_tex, 255, 255, 255);
		}
		SDL_RenderCopy(renderer, btn2_tex, nullptr, &btn2_rect);
	}

	// Keyboard button (green)
	if (btnkb_tex) {
		Uint8 alpha = joy_kb_pressed ? ALPHA_PRESSED : ALPHA_NORMAL;
		SDL_SetTextureAlphaMod(btnkb_tex, alpha);
		if (joy_kb_pressed) {
			SDL_SetTextureColorMod(btnkb_tex, 200, 255, 200);
		} else {
			SDL_SetTextureColorMod(btnkb_tex, 255, 255, 255);
		}
		SDL_RenderCopy(renderer, btnkb_tex, nullptr, &btnkb_rect);
	}
}

#ifdef USE_OPENGL
void on_screen_joystick_redraw_gl(int drawable_w, int drawable_h, const SDL_Rect& game_rect)
{
	if (!osj_enabled || !osj_initialized) return;
	if (!stick_base_surface || !knob_surface || !btn1_surface || !btn2_surface) return;

	// Update layout if geometry changed
	if (drawable_w > 0 && drawable_h > 0 && game_rect.w > 0 && game_rect.h > 0) {
		if (drawable_w != screen_w || drawable_h != screen_h ||
			game_rect.x != cached_game_rect.x || game_rect.y != cached_game_rect.y ||
			game_rect.w != cached_game_rect.w || game_rect.h != cached_game_rect.h)
		{
			on_screen_joystick_update_layout(drawable_w, drawable_h, game_rect);
			cached_game_rect = game_rect;
		}
	}

	// Lazy-init GL resources
	if (!init_osj_gl_shader()) return;

	// Upload surfaces to GL textures on first use
	if (!gl_stick_base_tex) gl_stick_base_tex = upload_surface_to_gl(stick_base_surface);
	if (!gl_knob_tex) gl_knob_tex = upload_surface_to_gl(knob_surface);
	if (!gl_btn1_tex) gl_btn1_tex = upload_surface_to_gl(btn1_surface);
	if (!gl_btn2_tex) gl_btn2_tex = upload_surface_to_gl(btn2_surface);
	if (!gl_btnkb_tex) gl_btnkb_tex = upload_surface_to_gl(btnkb_surface);

	if (!gl_stick_base_tex || !gl_knob_tex || !gl_btn1_tex || !gl_btn2_tex) return;

	// Set up GL state for overlay rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_SCISSOR_TEST);

	// Use full drawable as viewport so our NDC calculations cover the whole screen
	glViewport(0, 0, drawable_w, drawable_h);

	glUseProgram(osj_gl_program);
	glBindVertexArray(osj_gl_vao);
	glEnableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	// Joystick base plate
	{
		float alpha = knob_active ? (ALPHA_PRESSED / 255.0f) : (ALPHA_NORMAL / 255.0f);
		render_gl_quad(gl_stick_base_tex, dpad_rect, drawable_w, drawable_h, alpha);
	}

	// Joystick knob (ball-top)
	{
		SDL_Rect knob_rect = compute_knob_rect();

		// Drop shadow when active
		if (knob_active) {
			SDL_Rect shadow_rect = knob_rect;
			shadow_rect.x += 2;
			shadow_rect.y += 3;
			shadow_rect.w += 2;
			shadow_rect.h += 2;
			render_gl_quad(gl_knob_tex, shadow_rect, drawable_w, drawable_h, 0.23f);
		}

		// The knob itself
		float knob_alpha = knob_active ? 0.94f : (ALPHA_NORMAL / 255.0f);
		render_gl_quad(gl_knob_tex, knob_rect, drawable_w, drawable_h, knob_alpha);
	}

	// Button 1 (red / fire)
	{
		float alpha = joy_fire1 ? (ALPHA_PRESSED / 255.0f) : (ALPHA_NORMAL / 255.0f);
		render_gl_quad(gl_btn1_tex, btn1_rect, drawable_w, drawable_h, alpha);
	}

	// Button 2 (blue / secondary fire)
	{
		float alpha = joy_fire2 ? (ALPHA_PRESSED / 255.0f) : (ALPHA_NORMAL / 255.0f);
		render_gl_quad(gl_btn2_tex, btn2_rect, drawable_w, drawable_h, alpha);
	}

	// Keyboard button (green)
	if (gl_btnkb_tex) {
		float alpha = joy_kb_pressed ? (ALPHA_PRESSED / 255.0f) : (ALPHA_NORMAL / 255.0f);
		render_gl_quad(gl_btnkb_tex, btnkb_rect, drawable_w, drawable_h, alpha);
	}

	// Restore GL state
	glDisableVertexAttribArray(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_BLEND);
	glUseProgram(0);
}
#endif // USE_OPENGL

// ---------------------------------------------------------------------------
// Touch event handlers
// ---------------------------------------------------------------------------

bool on_screen_joystick_handle_finger_down(const SDL_Event& event, int window_w, int window_h)
{
	if (!osj_enabled || !osj_initialized) return false;

	int px = static_cast<int>(event.tfinger.x * window_w);
	int py = static_cast<int>(event.tfinger.y * window_h);

	ControlType ctl = hit_test(px, py);
	if (ctl == CTL_NONE) return false;

	FingerTrack ft;
	ft.id = event.tfinger.fingerId;
	ft.control = ctl;
	active_fingers.push_back(ft);

	switch (ctl) {
	case CTL_DPAD:
		knob_active = true;
		update_dpad_from_position(px, py);
		break;
	case CTL_BUTTON1:
		joy_fire1 = true;
		inject_buttons();
		break;
	case CTL_BUTTON2:
		joy_fire2 = true;
		inject_buttons();
		break;
	case CTL_KEYBOARD:
		joy_kb_pressed = true;
		kb_tapped = true;
		break;
	default:
		break;
	}

	return true;
}

bool on_screen_joystick_handle_finger_up(const SDL_Event& event, int /*window_w*/, int /*window_h*/)
{
	if (!osj_enabled || !osj_initialized) return false;

	FingerTrack* ft = find_finger(event.tfinger.fingerId);
	if (!ft) return false;

	ControlType ctl = ft->control;
	remove_finger(event.tfinger.fingerId);

	switch (ctl) {
	case CTL_DPAD:
		release_dpad();
		break;
	case CTL_BUTTON1:
	case CTL_BUTTON2:
		release_button(ctl);
		break;
	default:
		break;
	}

	return true;
}

bool on_screen_joystick_handle_finger_motion(const SDL_Event& event, int window_w, int window_h)
{
	if (!osj_enabled || !osj_initialized) return false;

	FingerTrack* ft = find_finger(event.tfinger.fingerId);
	if (!ft) return false;

	if (ft->control == CTL_DPAD) {
		int px = static_cast<int>(event.tfinger.x * window_w);
		int py = static_cast<int>(event.tfinger.y * window_h);
		update_dpad_from_position(px, py);
	}

	return true;
}
