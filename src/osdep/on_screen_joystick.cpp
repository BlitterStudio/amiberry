/*
 * Amiberry - On-screen touch joystick controls
 *
 * Provides a virtual D-pad and fire buttons overlay for touchscreen devices.
 * Renders retro arcade-style graphics and injects joystick events into
 * the emulated Amiga joystick port 2.
 *
 * Copyright 2025 Amiberry team
 */

#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

#include <SDL.h>

#include "on_screen_joystick.h"
#include "sysdeps.h"
#include "inputdevice.h"
#include "options.h"
#include "amiberry_gfx.h"

// ---------------------------------------------------------------------------
// Configuration constants
// ---------------------------------------------------------------------------

// D-pad size as fraction of the shorter screen dimension
static constexpr float DPAD_SIZE_FRACTION = 0.32f;
// Button size as fraction of the shorter screen dimension
static constexpr float BUTTON_SIZE_FRACTION = 0.14f;
// Vertical spacing between the two buttons as fraction of shorter dimension
static constexpr float BUTTON_GAP_FRACTION = 0.04f;
// Margin from screen edge as fraction of shorter dimension
static constexpr float EDGE_MARGIN_FRACTION = 0.02f;
// Texture resolution for procedurally-generated graphics
static constexpr int DPAD_TEX_SIZE = 256;
static constexpr int BUTTON_TEX_SIZE = 128;
// Alpha values (0-255)
static constexpr Uint8 ALPHA_NORMAL = 100;
static constexpr Uint8 ALPHA_PRESSED = 180;
// Dead zone in the center of the D-pad (fraction of radius)
static constexpr float DPAD_DEADZONE = 0.20f;

// ---------------------------------------------------------------------------
// Retro color palette
// ---------------------------------------------------------------------------

struct Color {
	Uint8 r, g, b, a;
	Uint32 to_u32() const {
		return (r << 24) | (g << 16) | (b << 8) | a;
	}
};

// D-pad colors - dark gunmetal with steel cross
static constexpr Color DPAD_BASE      = { 30,  30,  35, 255 };  // dark base circle
static constexpr Color DPAD_CROSS     = { 60,  60,  68, 255 };  // cross surface
static constexpr Color DPAD_CROSS_LIT = { 45,  45,  52, 255 };  // cross border/bevel
static constexpr Color DPAD_HIGHLIGHT = {200, 180,  50, 200 };  // pressed direction
static constexpr Color DPAD_ARROW     = {180, 180, 190, 255 };  // arrow indicators

// Button colors
static constexpr Color BTN1_OUTER     = {160,  20,  20, 255 };  // red button outer
static constexpr Color BTN1_INNER     = {220,  50,  50, 255 };  // red button inner
static constexpr Color BTN1_HIGHLIGHT = {255, 100, 100, 255 };  // red button glint
static constexpr Color BTN2_OUTER     = { 20,  40, 160, 255 };  // blue button outer
static constexpr Color BTN2_INNER     = { 50,  70, 220, 255 };  // blue button inner
static constexpr Color BTN2_HIGHLIGHT = {100, 130, 255, 255 };  // blue button glint

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

static bool osj_enabled = false;
static bool osj_initialized = false;

// Textures
static SDL_Texture* dpad_tex = nullptr;
static SDL_Texture* btn1_tex = nullptr;
static SDL_Texture* btn2_tex = nullptr;

// Layout rectangles on screen
static SDL_Rect dpad_rect = {};
static SDL_Rect btn1_rect = {};
static SDL_Rect btn2_rect = {};

// Center and radius for hit-testing (in screen coords)
static int dpad_cx = 0, dpad_cy = 0, dpad_hit_radius = 0;
static int btn1_cx = 0, btn1_cy = 0, btn1_hit_radius = 0;
static int btn2_cx = 0, btn2_cy = 0, btn2_hit_radius = 0;

// Joystick state
static bool joy_up = false, joy_down = false, joy_left = false, joy_right = false;
static bool joy_fire1 = false, joy_fire2 = false;
// Previous state for change detection
static bool prev_up = false, prev_down = false, prev_left = false, prev_right = false;
static bool prev_fire1 = false, prev_fire2 = false;

// Multi-touch tracking
enum ControlType { CTL_NONE, CTL_DPAD, CTL_BUTTON1, CTL_BUTTON2 };

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
// Helper: draw a filled circle with a radial gradient (outer → inner)
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
			float t = dist / r; // 0 at center, 1 at edge

			// Base gradient: inner → outer
			Uint8 cr = static_cast<Uint8>(inner.r + (outer.r - inner.r) * t);
			Uint8 cg = static_cast<Uint8>(inner.g + (outer.g - inner.g) * t);
			Uint8 cb = static_cast<Uint8>(inner.b + (outer.b - inner.b) * t);
			Uint8 ca = static_cast<Uint8>(inner.a + (outer.a - inner.a) * t);

			// Add a specular highlight toward the top-left
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
// Helper: draw a small triangle arrow on an SDL_Surface
// ---------------------------------------------------------------------------

static void draw_arrow_up(SDL_Surface* s, int cx, int cy, int size, Color col)
{
	Uint32 color = SDL_MapRGBA(s->format, col.r, col.g, col.b, col.a);
	auto* pixels = static_cast<Uint32*>(s->pixels);
	int pitch = s->pitch / 4;
	for (int row = 0; row < size; row++) {
		int half_w = row;
		int y = cy - size / 2 + row;
		if (y < 0 || y >= s->h) continue;
		for (int dx = -half_w; dx <= half_w; dx++) {
			int x = cx + dx;
			if (x >= 0 && x < s->w)
				pixels[y * pitch + x] = color;
		}
	}
}

static void draw_arrow_down(SDL_Surface* s, int cx, int cy, int size, Color col)
{
	Uint32 color = SDL_MapRGBA(s->format, col.r, col.g, col.b, col.a);
	auto* pixels = static_cast<Uint32*>(s->pixels);
	int pitch = s->pitch / 4;
	for (int row = 0; row < size; row++) {
		int half_w = size - 1 - row;
		int y = cy - size / 2 + row;
		if (y < 0 || y >= s->h) continue;
		for (int dx = -half_w; dx <= half_w; dx++) {
			int x = cx + dx;
			if (x >= 0 && x < s->w)
				pixels[y * pitch + x] = color;
		}
	}
}

static void draw_arrow_left(SDL_Surface* s, int cx, int cy, int size, Color col)
{
	Uint32 color = SDL_MapRGBA(s->format, col.r, col.g, col.b, col.a);
	auto* pixels = static_cast<Uint32*>(s->pixels);
	int pitch = s->pitch / 4;
	for (int col_idx = 0; col_idx < size; col_idx++) {
		int half_h = col_idx;
		int x = cx - size / 2 + col_idx;
		if (x < 0 || x >= s->w) continue;
		for (int dy = -half_h; dy <= half_h; dy++) {
			int y = cy + dy;
			if (y >= 0 && y < s->h)
				pixels[y * pitch + x] = color;
		}
	}
}

static void draw_arrow_right(SDL_Surface* s, int cx, int cy, int size, Color col)
{
	Uint32 color = SDL_MapRGBA(s->format, col.r, col.g, col.b, col.a);
	auto* pixels = static_cast<Uint32*>(s->pixels);
	int pitch = s->pitch / 4;
	for (int col_idx = 0; col_idx < size; col_idx++) {
		int half_h = size - 1 - col_idx;
		int x = cx - size / 2 + col_idx;
		if (x < 0 || x >= s->w) continue;
		for (int dy = -half_h; dy <= half_h; dy++) {
			int y = cy + dy;
			if (y >= 0 && y < s->h)
				pixels[y * pitch + x] = color;
		}
	}
}

// ---------------------------------------------------------------------------
// Texture generation: D-pad
// ---------------------------------------------------------------------------

static SDL_Texture* create_dpad_texture(SDL_Renderer* renderer)
{
	const int sz = DPAD_TEX_SIZE;
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_RGBA8888);
	if (!surface) return nullptr;

	// Clear to transparent
	SDL_FillRect(surface, nullptr, 0);

	int cx = sz / 2;
	int cy = sz / 2;
	int base_r = sz / 2 - 4;
	int arm_half = sz / 7;    // half-width of each arm
	int arm_len = base_r - 4; // length from center to arm tip

	// 1. Dark circular base plate
	fill_circle(surface, cx, cy, base_r, DPAD_BASE);

	// 2. Slightly smaller inner ring for depth effect
	Color inner_ring = { 40, 40, 46, 255 };
	fill_circle(surface, cx, cy, base_r - 3, inner_ring);

	// 3. Cross shape - vertical arm
	fill_rect(surface, cx - arm_half, cy - arm_len, arm_half * 2, arm_len * 2, DPAD_CROSS);
	// Cross shape - horizontal arm
	fill_rect(surface, cx - arm_len, cy - arm_half, arm_len * 2, arm_half * 2, DPAD_CROSS);

	// 4. Bevel effect: lighter top/left edges of cross
	Color bevel_light = { 80, 80, 90, 255 };
	// Top edge of vertical arm
	fill_rect(surface, cx - arm_half, cy - arm_len, arm_half * 2, 2, bevel_light);
	// Left edge of horizontal arm
	fill_rect(surface, cx - arm_len, cy - arm_half, 2, arm_half * 2, bevel_light);
	// Left edge of vertical arm
	fill_rect(surface, cx - arm_half, cy - arm_len, 2, arm_len * 2, bevel_light);
	// Top edge of horizontal arm
	fill_rect(surface, cx - arm_len, cy - arm_half, arm_len * 2, 2, bevel_light);

	// 5. Arrow indicators on each arm
	int arrow_size = sz / 14;
	int arrow_offset = arm_len * 2 / 3;
	draw_arrow_up(surface, cx, cy - arrow_offset, arrow_size, DPAD_ARROW);
	draw_arrow_down(surface, cx, cy + arrow_offset, arrow_size, DPAD_ARROW);
	draw_arrow_left(surface, cx - arrow_offset, cy, arrow_size, DPAD_ARROW);
	draw_arrow_right(surface, cx + arrow_offset, cy, arrow_size, DPAD_ARROW);

	// 6. Center circle (joystick ball cosmetic)
	Color center_dot = { 50, 50, 56, 255 };
	fill_circle(surface, cx, cy, sz / 12, center_dot);
	Color center_highlight = { 70, 70, 78, 255 };
	fill_circle(surface, cx - 1, cy - 1, sz / 16, center_highlight);

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	if (tex) {
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	}
	SDL_FreeSurface(surface);
	return tex;
}

// ---------------------------------------------------------------------------
// Texture generation: arcade button
// ---------------------------------------------------------------------------

static SDL_Texture* create_button_texture(SDL_Renderer* renderer,
	Color outer, Color inner, Color glint)
{
	const int sz = BUTTON_TEX_SIZE;
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_RGBA8888);
	if (!surface) return nullptr;

	SDL_FillRect(surface, nullptr, 0);

	int cx = sz / 2;
	int cy = sz / 2;
	int r = sz / 2 - 4;

	// Outer ring (bezel)
	Color bezel = { static_cast<Uint8>(outer.r / 2), static_cast<Uint8>(outer.g / 2),
		static_cast<Uint8>(outer.b / 2), 255 };
	fill_circle(surface, cx, cy, r, bezel);

	// Main button surface with gradient
	fill_circle_gradient(surface, cx, cy, r - 4, outer, inner, glint);

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
	if (tex) {
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
	}
	SDL_FreeSurface(surface);
	return tex;
}

// ---------------------------------------------------------------------------
// Input injection helpers
// ---------------------------------------------------------------------------

static void inject_directions()
{
	// Only send events when state changes
	if (joy_up != prev_up) {
		send_input_event(INPUTEVENT_JOY2_UP, joy_up ? 1 : 0, 1, 0);
		prev_up = joy_up;
	}
	if (joy_down != prev_down) {
		send_input_event(INPUTEVENT_JOY2_DOWN, joy_down ? 1 : 0, 1, 0);
		prev_down = joy_down;
	}
	if (joy_left != prev_left) {
		send_input_event(INPUTEVENT_JOY2_LEFT, joy_left ? 1 : 0, 1, 0);
		prev_left = joy_left;
	}
	if (joy_right != prev_right) {
		send_input_event(INPUTEVENT_JOY2_RIGHT, joy_right ? 1 : 0, 1, 0);
		prev_right = joy_right;
	}
}

static void inject_buttons()
{
	if (joy_fire1 != prev_fire1) {
		send_input_event(INPUTEVENT_JOY2_FIRE_BUTTON, joy_fire1 ? 1 : 0, 1, 0);
		prev_fire1 = joy_fire1;
	}
	if (joy_fire2 != prev_fire2) {
		send_input_event(INPUTEVENT_JOY2_2ND_BUTTON, joy_fire2 ? 1 : 0, 1, 0);
		prev_fire2 = joy_fire2;
	}
}

// ---------------------------------------------------------------------------
// Determine D-pad direction from touch position relative to center
// ---------------------------------------------------------------------------

static void update_dpad_from_position(int touch_x, int touch_y)
{
	int dx = touch_x - dpad_cx;
	int dy = touch_y - dpad_cy;
	float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

	// Inside dead zone → no direction
	float deadzone_px = dpad_hit_radius * DPAD_DEADZONE;
	if (dist < deadzone_px) {
		joy_up = joy_down = joy_left = joy_right = false;
		inject_directions();
		return;
	}

	// Calculate angle: 0 = right, PI/2 = down, etc.
	float angle = std::atan2(static_cast<float>(dy), static_cast<float>(dx));

	// 8-way direction with 45-degree sectors
	// Each cardinal direction covers ±22.5° around its axis,
	// diagonals cover the space between.
	// Use PI/8 (22.5°) boundaries:
	//   Right:     -PI/8 to  PI/8
	//   Down-Right: PI/8 to 3PI/8
	//   Down:     3PI/8 to 5PI/8
	//   Down-Left:5PI/8 to 7PI/8
	//   Left:     7PI/8 or -7PI/8 to -5PI/8  (wraps around ±PI)
	//   Up-Left: -7PI/8 to -5PI/8... etc.

	// Simpler approach: check which 45° octant the angle falls in
	static constexpr float PI = 3.14159265f;
	static constexpr float SECTOR = PI / 4.0f; // 45 degrees
	static constexpr float HALF_SECTOR = PI / 8.0f; // 22.5 degrees

	joy_right = (angle > -SECTOR + HALF_SECTOR && angle < SECTOR - HALF_SECTOR) ||
	            (angle >= -HALF_SECTOR && angle <= HALF_SECTOR);
	joy_left  = (angle > PI - HALF_SECTOR || angle < -PI + HALF_SECTOR);
	joy_down  = (angle > SECTOR - HALF_SECTOR && angle < PI - SECTOR + HALF_SECTOR) ||
	            (angle >= PI / 2.0f - HALF_SECTOR && angle <= PI / 2.0f + HALF_SECTOR);
	joy_up    = (angle < -SECTOR + HALF_SECTOR && angle > -PI + SECTOR - HALF_SECTOR) ||
	            (angle >= -PI / 2.0f - HALF_SECTOR && angle <= -PI / 2.0f + HALF_SECTOR);

	// Cleaner 8-way: normalize angle to [0, 2*PI)
	float a = angle;
	if (a < 0) a += 2.0f * PI;
	// Sector index: 0=right, 1=down-right, 2=down, 3=down-left, 4=left, 5=up-left, 6=up, 7=up-right
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
	inject_directions();
}

static void release_button(ControlType ctl)
{
	if (ctl == CTL_BUTTON1) {
		joy_fire1 = false;
	} else if (ctl == CTL_BUTTON2) {
		joy_fire2 = false;
	}
	inject_buttons();
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

// Hit-test: which control does this screen position fall on?
static ControlType hit_test(int px, int py)
{
	// D-pad: circular hit area
	{
		int dx = px - dpad_cx;
		int dy = py - dpad_cy;
		if (dx * dx + dy * dy <= dpad_hit_radius * dpad_hit_radius)
			return CTL_DPAD;
	}
	// Button 1
	{
		int dx = px - btn1_cx;
		int dy = py - btn1_cy;
		if (dx * dx + dy * dy <= btn1_hit_radius * btn1_hit_radius)
			return CTL_BUTTON1;
	}
	// Button 2
	{
		int dx = px - btn2_cx;
		int dy = py - btn2_cy;
		if (dx * dx + dy * dy <= btn2_hit_radius * btn2_hit_radius)
			return CTL_BUTTON2;
	}
	return CTL_NONE;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void on_screen_joystick_init(SDL_Renderer* renderer)
{
	if (osj_initialized) {
		on_screen_joystick_quit();
	}

	dpad_tex = create_dpad_texture(renderer);
	btn1_tex = create_button_texture(renderer, BTN1_OUTER, BTN1_INNER, BTN1_HIGHLIGHT);
	btn2_tex = create_button_texture(renderer, BTN2_OUTER, BTN2_INNER, BTN2_HIGHLIGHT);

	// Reset state
	joy_up = joy_down = joy_left = joy_right = false;
	joy_fire1 = joy_fire2 = false;
	prev_up = prev_down = prev_left = prev_right = false;
	prev_fire1 = prev_fire2 = false;
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

	if (dpad_tex) { SDL_DestroyTexture(dpad_tex); dpad_tex = nullptr; }
	if (btn1_tex) { SDL_DestroyTexture(btn1_tex); btn1_tex = nullptr; }
	if (btn2_tex) { SDL_DestroyTexture(btn2_tex); btn2_tex = nullptr; }
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
	if (!enabled && osj_initialized) {
		// Release any held directions/buttons
		joy_up = joy_down = joy_left = joy_right = false;
		joy_fire1 = joy_fire2 = false;
		inject_directions();
		inject_buttons();
		active_fingers.clear();
	}
}

void on_screen_joystick_update_layout(int sw, int sh, const SDL_Rect& game_rect)
{
	screen_w = sw;
	screen_h = sh;

	int shorter = std::min(sw, sh);

	// D-pad dimensions
	int dpad_size = static_cast<int>(shorter * DPAD_SIZE_FRACTION);
	int margin = static_cast<int>(shorter * EDGE_MARGIN_FRACTION);

	// Place D-pad on the left side, vertically centered
	int left_space = game_rect.x; // pixels available left of game screen
	if (left_space > dpad_size + margin * 2) {
		// Enough room in the letterbox - center in left margin
		dpad_rect.x = (left_space - dpad_size) / 2;
	} else {
		// Overlap with game screen if necessary
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

	// Place buttons on the right side
	int right_start = game_rect.x + game_rect.w;
	int right_space = sw - right_start;
	int btn_x;
	if (right_space > btn_size + margin * 2) {
		btn_x = right_start + (right_space - btn_size) / 2;
	} else {
		btn_x = sw - btn_size - margin;
	}

	// Two buttons stacked vertically, centered on screen height
	// Position them with button 1 higher, button 2 lower
	// Offset them diagonally for ergonomic thumb reach:
	// Button 1 (top-right) is slightly higher and to the right
	// Button 2 (bottom-left) is slightly lower and to the left
	int total_btn_height = btn_size * 2 + btn_gap;
	int btn_y_start = (sh - total_btn_height) / 2;

	btn1_rect.x = btn_x + btn_size / 4; // Offset right slightly
	btn1_rect.y = btn_y_start - btn_size / 6;
	btn1_rect.w = btn_size;
	btn1_rect.h = btn_size;

	btn2_rect.x = btn_x - btn_size / 4; // Offset left slightly
	btn2_rect.y = btn_y_start + btn_size + btn_gap + btn_size / 6;
	btn2_rect.w = btn_size;
	btn2_rect.h = btn_size;

	btn1_cx = btn1_rect.x + btn_size / 2;
	btn1_cy = btn1_rect.y + btn_size / 2;
	btn1_hit_radius = btn_size / 2 + btn_size / 8; // Slightly larger hit area

	btn2_cx = btn2_rect.x + btn_size / 2;
	btn2_cy = btn2_rect.y + btn_size / 2;
	btn2_hit_radius = btn_size / 2 + btn_size / 8;
}

void on_screen_joystick_redraw(SDL_Renderer* renderer)
{
	if (!osj_enabled || !osj_initialized) return;
	if (!dpad_tex || !btn1_tex || !btn2_tex) return;

	// Auto-recalculate layout when screen geometry changes.
	// render_quad is updated by the core renderer whenever the Amiga display
	// size or scaling changes, so we detect that here and re-layout.
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

	// D-pad
	{
		bool any_pressed = joy_up || joy_down || joy_left || joy_right;
		Uint8 alpha = any_pressed ? ALPHA_PRESSED : ALPHA_NORMAL;
		SDL_SetTextureAlphaMod(dpad_tex, alpha);
		SDL_RenderCopy(renderer, dpad_tex, nullptr, &dpad_rect);

		// Draw highlight overlays for pressed directions
		if (any_pressed) {
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			int arm_half = dpad_rect.w / 7;
			int arm_len = dpad_rect.w / 2 - 8;

			if (joy_up) {
				SDL_Rect r = { dpad_cx - arm_half, dpad_rect.y + 8, arm_half * 2, arm_len };
				SDL_SetRenderDrawColor(renderer, DPAD_HIGHLIGHT.r, DPAD_HIGHLIGHT.g,
					DPAD_HIGHLIGHT.b, DPAD_HIGHLIGHT.a / 2);
				SDL_RenderFillRect(renderer, &r);
			}
			if (joy_down) {
				SDL_Rect r = { dpad_cx - arm_half, dpad_cy, arm_half * 2, arm_len };
				SDL_SetRenderDrawColor(renderer, DPAD_HIGHLIGHT.r, DPAD_HIGHLIGHT.g,
					DPAD_HIGHLIGHT.b, DPAD_HIGHLIGHT.a / 2);
				SDL_RenderFillRect(renderer, &r);
			}
			if (joy_left) {
				SDL_Rect r = { dpad_rect.x + 8, dpad_cy - arm_half, arm_len, arm_half * 2 };
				SDL_SetRenderDrawColor(renderer, DPAD_HIGHLIGHT.r, DPAD_HIGHLIGHT.g,
					DPAD_HIGHLIGHT.b, DPAD_HIGHLIGHT.a / 2);
				SDL_RenderFillRect(renderer, &r);
			}
			if (joy_right) {
				SDL_Rect r = { dpad_cx, dpad_cy - arm_half, arm_len, arm_half * 2 };
				SDL_SetRenderDrawColor(renderer, DPAD_HIGHLIGHT.r, DPAD_HIGHLIGHT.g,
					DPAD_HIGHLIGHT.b, DPAD_HIGHLIGHT.a / 2);
				SDL_RenderFillRect(renderer, &r);
			}
		}
	}

	// Button 1 (red / fire)
	{
		Uint8 alpha = joy_fire1 ? ALPHA_PRESSED : ALPHA_NORMAL;
		SDL_SetTextureAlphaMod(btn1_tex, alpha);
		if (joy_fire1) {
			SDL_SetTextureColorMod(btn1_tex, 255, 200, 200); // Brighten when pressed
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
}

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

	// Track this finger
	FingerTrack ft;
	ft.id = event.tfinger.fingerId;
	ft.control = ctl;
	active_fingers.push_back(ft);

	switch (ctl) {
	case CTL_DPAD:
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
	default:
		break;
	}

	return true; // consumed
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

	return true; // consumed
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
	// Buttons don't need motion tracking (press/release only)

	return true; // consumed
}
