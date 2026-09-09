// Real-engine regression tests for the on-screen keyboard input lifecycle.
//
// The production translation unit src/osdep/imgui_osk.cpp is compiled
// unmodified by tests/test_imgui_osk_engine.sh (copied into a scratch
// directory whose sysconfig.h/sysdeps.h/SDL3 shims replace only the platform
// boundaries), linked against the real external/imgui and the real
// src/include headers. This harness replaces the remaining sink boundaries:
// the SDL time source (deterministic fake clock), the overlay font
// accessors, the guest keyboard, and the input-layer osk_control() /
// osk_clear_controller_holds() entry points.
//
// Covered regressions:
//  - held-direction key repeat advances only via imgui_osk_update(), never
//    from imgui_osk_render() (review #3: render thread must stay read-only
//    with respect to focus/repeat state)
//  - no repeat while South is held; releasing South frees the originally
//    pressed key, and directions pressed during the hold replay one step on
//    release, then repeat re-arms for the still-held mask
//  - imgui_osk_shutdown() releases every guest key the keyboard owns,
//    including sticky modifiers (review #2, pre-existing)
//  - imgui_osk_hide() preserves intentionally engaged sticky modifiers and
//    drops accumulated input state via osk_control(..., OskInputSource::Gamepad)
//  - rendering draws geometry without advancing navigation state

#include "sysconfig.h" // scratch platform shim: SIZEOF_* for uae/types.h

#include "imgui_osk.h"
#include "imgui_overlay.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "on_screen_joystick.h"

#include "imgui.h"

#include <iostream>
#include <set>
#include <string>

static int failures = 0;

static void expect(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		failures++;
	}
}

// ---- deterministic time source (replaces SDL_GetTicks) -------------------

static Uint64 g_ticks = 0;

Uint64 SDL_GetTicks()
{
	return g_ticks;
}

static void advance(Uint64 ms)
{
	g_ticks += ms;
}

// ---- guest keyboard sink --------------------------------------------------

static std::set<int> g_guest_down;

void inputdevice_do_keyboard(int key, int state)
{
	if (state)
		g_guest_down.insert(key);
	else
		g_guest_down.erase(key);
}

// Input-layer boundaries are covered by the separate native routing harness.
void osk_control(int, int, int, int, OskInputSource) {}
void osk_clear_controller_holds() {}

// ---- overlay sinks ----------------------------------------------------------

static ImFont* g_font = nullptr;

bool imgui_overlay_is_initialized()
{
	return g_font != nullptr;
}

ImFont* imgui_overlay_get_font()
{
	return g_font;
}

ImFont* imgui_overlay_get_font_small()
{
	return g_font;
}

void on_screen_joystick_release_all()
{
}

// ---- engine driver ----------------------------------------------------------

static int g_keycode = -1;
static int g_pressed = -1;

// Feed one joystick state snapshot; returns whether a key event was reported.
static bool p(int state)
{
	return imgui_osk_process(state, &g_keycode, &g_pressed);
}

// Fresh keyboard session: focus on Esc, sticky/held state cleared, clock at
// t=1000. Asserting a clean guest keyboard here means every test also
// re-verifies that shutdown released everything it owned.
static void reset_engine()
{
	imgui_osk_shutdown();
	expect(g_guest_down.empty(), "shutdown must leave no guest key asserted");
	g_guest_down.clear();
	imgui_osk_init();
	g_ticks = 1000;
	imgui_osk_toggle();
}


// ---- tests --------------------------------------------------------------------

// A held D-pad direction produces no further SDL events; the repeat must be
// driven solely by imgui_osk_update(), honoring the 400 ms delay and the
// 100 ms rate.
static void test_repeat_advances_only_via_update()
{
	reset_engine();
	p(OSK_RIGHT);       // Esc -> F1, repeat armed at t=1000
	advance(399);
	imgui_osk_update(); // below the initial delay: no move
	advance(2);
	imgui_osk_update(); // delay elapsed, >=100 ms since arming: F1 -> F2
	advance(49);
	imgui_osk_update(); // 49 ms since the last repeat: rate-limited, no move
	advance(51);
	imgui_osk_update(); // 100 ms elapsed: F2 -> F3

	expect(p(OSK_RIGHT | OSK_BUTTON) && g_keycode == AK_F3 && g_pressed == 1,
		"after two update-driven repeats South must press F3");
	expect(g_guest_down.count(AK_F3) == 1, "F3 must be held in the guest while South is down");
	expect(p(OSK_RIGHT) && g_keycode == AK_F3 && g_pressed == 0,
		"South release must free F3, the key that was pressed");
	expect(g_guest_down.empty(), "no guest key may stay asserted after South release");
}

// Rendering must draw but never advance focus/repeat, even when the repeat
// delay and many rate intervals have elapsed since the direction was held.
static void test_render_does_not_advance_navigation()
{
	reset_engine();
	p(OSK_RIGHT); // focus F1, armed at t=1000
	advance(2000);

	ImDrawList* fg = ImGui::GetForegroundDrawList();
	const int vtx_before = fg->VtxBuffer.Size;
	imgui_osk_render();
	imgui_osk_render();
	imgui_osk_render();
	expect(fg->VtxBuffer.Size > vtx_before, "render must draw keyboard geometry");

	expect(p(OSK_RIGHT | OSK_BUTTON) && g_keycode == AK_F1 && g_pressed == 1,
		"rendering must not advance focus: South must still press F1");
	expect(p(OSK_RIGHT) && g_keycode == AK_F1 && g_pressed == 0,
		"South release after render frames must free F1");

	// The input-side pump still advances after rendering.
	advance(401);
	imgui_osk_update();
	expect(p(OSK_RIGHT | OSK_BUTTON) && g_keycode == AK_F2 && g_pressed == 1,
		"imgui_osk_update() after render frames must advance focus to F2");
	expect(p(OSK_RIGHT) && g_keycode == AK_F2 && g_pressed == 0,
		"South release must free F2");
}

// While South is held the repeat stays frozen, so the eventual release frees
// the originally pressed key rather than a key the focus moved onto.
static void test_south_hold_freezes_repeat_and_releases_original_key()
{
	reset_engine();
	p(OSK_RIGHT); // F1
	expect(p(OSK_RIGHT | OSK_BUTTON) && g_keycode == AK_F1 && g_pressed == 1,
		"South must press F1");
	advance(1000); // deep past the delay and many rate intervals
	imgui_osk_update();
	imgui_osk_update();
	expect(p(OSK_RIGHT) && g_keycode == AK_F1 && g_pressed == 0,
		"South release must free the originally pressed F1 while RIGHT is still held");
	expect(g_guest_down.empty(), "guest must be clean after the release");
	expect(p(OSK_RIGHT | OSK_BUTTON) && g_keycode == AK_F1 && g_pressed == 1,
		"focus must not have moved while South was held");
	expect(p(OSK_RIGHT) && g_keycode == AK_F1 && g_pressed == 0,
		"release F1 again");
}

// Directions pressed while South is held are suppressed; the release replays
// them one step and re-arms repeat for the still-held mask.
static void test_suppressed_direction_replays_on_release()
{
	reset_engine();
	p(OSK_RIGHT); // F1, armed
	expect(p(OSK_RIGHT | OSK_BUTTON) && g_keycode == AK_F1 && g_pressed == 1,
		"South must press F1");
	p(OSK_RIGHT | OSK_BUTTON | OSK_DOWN); // DOWN rising suppressed during the hold
	advance(500);
	imgui_osk_update(); // frozen while South is held

	expect(p(OSK_RIGHT | OSK_DOWN) && g_keycode == AK_F1 && g_pressed == 0,
		"release must free the original F1, not a repeat-moved key");
	expect(p(OSK_RIGHT | OSK_DOWN | OSK_BUTTON) && g_keycode == AK_1 && g_pressed == 1,
		"suppressed DOWN must replay one step on release: key below F1 is 1");
	expect(p(OSK_RIGHT | OSK_DOWN) && g_keycode == AK_1 && g_pressed == 0,
		"release 1");

	advance(401);     // repeat re-armed at release time for RIGHT|DOWN
	imgui_osk_update(); // DOWN then RIGHT from 1 lands on W
	expect(p(OSK_RIGHT | OSK_DOWN | OSK_BUTTON) && g_keycode == AK_W && g_pressed == 1,
		"repeat after replay must advance DOWN then RIGHT from 1 to W");
	expect(p(OSK_RIGHT | OSK_DOWN) && g_keycode == AK_W && g_pressed == 0,
		"release W");
}

// Shutdown must release every guest key the keyboard still owns: the held
// key under South plus all sticky modifiers. (Pre-existing leak, review #2.)
static void test_shutdown_releases_owned_guest_keys()
{
	reset_engine();
	for (int i = 0; i < 4; i++) {
		p(OSK_DOWN);
		p(0); // Esc -> ` -> Tab -> Ctrl -> left Shift
	}
	expect(p(OSK_BUTTON) && g_keycode == AK_LSH && g_pressed == 1,
		"navigation must reach left Shift and South must engage it");
	expect(p(0) && g_keycode == AK_LSH && g_pressed == 0,
		"South release over a modifier frees the button");
	expect(g_guest_down.count(AK_LSH) == 1,
		"left Shift must stay engaged as a sticky modifier after the press");

	p(OSK_DOWN);
	p(0); // -> left Alt
	expect(p(OSK_BUTTON) && g_keycode == AK_LALT && g_pressed == 1,
		"South must press left Alt");
	advance(600);
	imgui_osk_update(); // frozen while South is held

	imgui_osk_shutdown();
	expect(g_guest_down.empty(),
		"shutdown must release held keys and sticky modifiers owned by the OSK");
	expect(!imgui_osk_is_active(), "shutdown must hide the keyboard");
}

// A normal hide preserves intentionally engaged sticky modifiers but still
// drops accumulated input state; a later shutdown releases them.
static void test_hide_preserves_sticky_and_clears_accumulators()
{
	reset_engine();
	for (int i = 0; i < 4; i++) {
		p(OSK_DOWN);
		p(0);
	}
	expect(p(OSK_BUTTON) && g_keycode == AK_LSH && g_pressed == 1,
		"South must engage left Shift");
	expect(p(0) && g_pressed == 0, "release South");

	imgui_osk_hide();
	expect(g_guest_down.count(AK_LSH) == 1,
		"hide must preserve intentionally engaged sticky modifiers");
	expect(!imgui_osk_is_active(), "hide must hide the keyboard");

	advance(2000);
	imgui_osk_update(); // inactive: harmless no-op
	expect(!imgui_osk_is_active(), "update while inactive must not revive the keyboard");

	imgui_osk_toggle(); // reopen with the sticky still engaged
	expect(imgui_osk_is_active(), "toggle must reopen the keyboard");
	imgui_osk_shutdown();
	expect(g_guest_down.empty(), "shutdown after reopen must release the sticky Shift");
}

int main()
{
	imgui_osk_update(); // before init: harmless no-op

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(1280.0f, 720.0f);
	g_font = io.Fonts->AddFontDefault();
	io.Fonts->Build();
	ImGui::NewFrame();

	test_repeat_advances_only_via_update();
	test_render_does_not_advance_navigation();
	test_south_hold_freezes_repeat_and_releases_original_key();
	test_suppressed_direction_replays_on_release();
	test_shutdown_releases_owned_guest_keys();
	test_hide_preserves_sticky_and_clears_accumulators();

	if (failures > 0) {
		std::cerr << failures << " imgui_osk_engine_test failure(s)\n";
		return 1;
	}
	std::cout << "imgui_osk_engine_test: all checks passed\n";
	return 0;
}
