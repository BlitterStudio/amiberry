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
//  - Android touch chrome (built with AMIBERRY_OSK_TOUCH_CHROME=1): the Hide
//    key hides the keyboard by touch and by D-pad without touching the guest,
//    and the portrait dock inset follows the slide animation; without the
//    chrome the inset is always 0

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

// Finish the slide animation (300 ms) so touch input is accepted and the
// animation progress is final.
static void settle_animation()
{
	advance(400);
	imgui_osk_render();
}

#if defined(AMIBERRY_OSK_TOUCH_CHROME) && AMIBERRY_OSK_TOUCH_CHROME
// Key-height fraction of the window the plate covers on this (non-Android)
// test build; mirrors KB_HEIGHT_FRAC.
static constexpr float TEST_KB_HEIGHT_FRAC = 0.42f;

// Right from Help reaches Hide; South press reports and sends nothing, the
// release hides the keyboard like Back, and reopening focuses Esc again.
static void test_hide_key_by_dpad()
{
	reset_engine();
	for (int i = 0; i < 13; i++) { // Esc -> F1..F10, Del, Help, Hide
		p(OSK_RIGHT);
		p(0);
	}
	expect(p(OSK_BUTTON) && g_keycode == 0 && g_pressed == 1,
		"South on Hide must report no guest key");
	expect(g_guest_down.empty(), "pressing Hide must not touch the guest keyboard");
	expect(imgui_osk_is_active(), "Hide acts on release, not on press");
	expect(p(0) && g_keycode == 0 && g_pressed == 0, "South release on Hide");
	expect(!imgui_osk_is_active(), "releasing South on Hide must hide the keyboard");
	expect(g_guest_down.empty(), "hiding via Hide must leave the guest clean");

	imgui_osk_toggle();
	expect(p(OSK_BUTTON) && g_keycode == AK_ESC && g_pressed == 1,
		"after Hide the reopened keyboard must focus Esc, not hide again");
	expect(p(0) && g_keycode == AK_ESC && g_pressed == 0, "release Esc");
}

// A tap on Hide hides the keyboard on finger-up and sends nothing.
static void test_hide_key_by_touch()
{
	reset_engine();
	settle_animation();
	// Hide key centre for the 1280x720 test display: compact layout (17
	// units), 2% padding, key at x=15..16, function row of height 0.7.
	const float pad = 1280.0f * 0.02f;
	const float unit_w = (1280.0f - 2.0f * pad) / 17.0f;
	const float kb_h = 720.0f * TEST_KB_HEIGHT_FRAC;
	const float unit_h = (kb_h - 2.0f * pad) / 6.1f;
	const float x = pad + 15.5f * unit_w;
	const float y = 720.0f - kb_h + pad + 0.35f * unit_h;

	expect(imgui_osk_handle_finger_down(x, y, 7), "finger down on Hide must be consumed");
	expect(g_guest_down.empty(), "touching Hide must not press a guest key");
	expect(imgui_osk_is_active(), "Hide acts on finger-up");
	expect(imgui_osk_handle_finger_up(x, y, 7), "finger up on Hide must be consumed");
	expect(!imgui_osk_is_active(), "finger-up on Hide must hide the keyboard");
	expect(g_guest_down.empty(), "guest must be clean after hiding by touch");
}

// The portrait dock inset is the plate height while shown, 0 in landscape and
// back to 0 once the keyboard has slid out.
static void test_portrait_dock_inset()
{
	reset_engine();
	settle_animation();
	const float shown = imgui_osk_bottom_inset_fraction(1080, 2400);
	expect(shown > TEST_KB_HEIGHT_FRAC - 0.001f && shown < TEST_KB_HEIGHT_FRAC + 0.001f,
		"portrait inset must equal the docked plate height");
	expect(imgui_osk_bottom_inset_fraction(2400, 1080) == 0.0f,
		"landscape must keep the picture where it is");
	imgui_osk_hide();
	settle_animation();
	expect(imgui_osk_bottom_inset_fraction(1080, 2400) == 0.0f,
		"inset must return to 0 once the keyboard has slid out");
}
#else
// Without the Android chrome the picture never moves for the keyboard.
static void test_desktop_keeps_picture_placement()
{
	reset_engine();
	settle_animation();
	expect(imgui_osk_bottom_inset_fraction(1080, 2400) == 0.0f,
		"desktop keyboard must not move the picture, even in portrait");
}
#endif

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
#if defined(AMIBERRY_OSK_TOUCH_CHROME) && AMIBERRY_OSK_TOUCH_CHROME
	test_hide_key_by_dpad();
	test_hide_key_by_touch();
	test_portrait_dock_inset();
#else
	test_desktop_keeps_picture_placement();
#endif

	if (failures > 0) {
		std::cerr << failures << " imgui_osk_engine_test failure(s)\n";
		return 1;
	}
	std::cout << "imgui_osk_engine_test: all checks passed\n";
	return 0;
}
