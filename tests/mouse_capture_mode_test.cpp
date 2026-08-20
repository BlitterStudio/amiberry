#include <iostream>

#include "amiberry_mouse_capture.h"

// Mirror of the uae_prefs tablet-mode values (src/include/options.h); kept
// local so this test compiles standalone like the other osdep unit tests.
constexpr int k_tablet_off = 0;
constexpr int k_tablet_mousehack = 1;
constexpr int k_tablet_real = 2;

static int failures;

static void expect_eq(const bool actual, const bool expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

int main()
{
	// Regression: Android must never request relative mouse mode while the
	// mouse is captured. Android delivers physical-mouse motion as absolute
	// hover events, and SDL drops absolute motion while a window is in
	// relative mode, leaving the buttons working but the emulated pointer
	// frozen. Touch gestures are unaffected either way: they bypass SDL mouse
	// events through the android_touch_mouse recognizer.
	expect_eq(amiberry_capture_uses_relative_mouse_mode(true, k_tablet_off), false,
		"Android capture must keep the window in absolute mode");
	expect_eq(amiberry_capture_uses_relative_mouse_mode(true, k_tablet_mousehack), false,
		"Android capture must stay absolute with mousehack mode");
	expect_eq(amiberry_capture_uses_relative_mouse_mode(true, k_tablet_real), false,
		"Android capture must stay absolute with real-tablet mode");

	// Desktop platforms honor the captured-relative contract (all motion
	// events arrive relative), so capture keeps requesting relative mode.
	expect_eq(amiberry_capture_uses_relative_mouse_mode(false, k_tablet_off), true,
		"Desktop capture without tablet mode must request relative mode");
	expect_eq(amiberry_capture_uses_relative_mouse_mode(false, k_tablet_mousehack), false,
		"Mousehack mode must keep the window absolute on desktop");
	expect_eq(amiberry_capture_uses_relative_mouse_mode(false, k_tablet_real), false,
		"Real-tablet mode must keep the window absolute on desktop");

	return failures == 0 ? 0 : 1;
}
