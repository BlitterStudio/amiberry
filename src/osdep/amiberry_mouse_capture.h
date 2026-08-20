#pragma once

// Decides whether mouse capture should put the SDL window into relative mouse
// mode.
//
// Desktop platforms want it: capture there means "confine the pointer and
// deliver unbounded relative motion", which SDL honors end to end.
//
// Android must not request it. Android delivers physical-mouse motion as
// absolute hover events (ACTION_HOVER_MOVE), and SDL silently drops absolute
// motion while a window is flagged for relative mode (SDL_PrivateSendMouseMotion
// in SDL 3.4.x). Pointer capture, SDL's relative-mode backend there, is not
// guaranteed to engage (API 24-25 has none; API 26+ can deny or lose it on
// DeX, ChromeOS, TV boxes and focus transitions). Requesting relative mode
// therefore leaves a physical mouse with working buttons and a frozen emulated
// pointer. Keeping the window absolute lets SDL compute xrel/yrel deltas from
// the hover position, which handle_mouse_motion_event() feeds to the emulated
// mouse. Touch gestures are unaffected either way: they bypass SDL mouse
// events through the android_touch_mouse recognizer.
//
// input_tablet carries the uae_prefs tablet-mode value (TABLET_OFF == 0,
// TABLET_MOUSEHACK, TABLET_REAL). Any tablet/mousehack mode keeps the window
// absolute because it needs pointer coordinates.
inline bool amiberry_capture_uses_relative_mouse_mode(const bool platform_is_android,
	const int input_tablet)
{
	if (input_tablet != 0)
		return false;
	return !platform_is_android;
}
