/*
 * The Android app's in-emulation menu: a Compose overlay over the SDL
 * surface (android/app/src/main/java/com/blitterstudio/amiberry/overlay).
 * Where the built-in GUI would open over a running emulation (Back, the
 * gamepad's Start or Guide, the two-finger swipe, F12, the RetroArch combo,
 * the open_settings action), the emulator shows this menu instead, paused as
 * under the GUI. The built-in GUI stays for startup without -G, for the
 * media hotkeys' file requesters, as the menu's Advanced settings, and for
 * people who turn the menu off in the launcher.
 */
#pragma once

#ifdef __ANDROID__

// How the menu was closed.
enum class android_overlay_result {
	resume,   // back to the emulation (or to a reset, quit or state load it ran)
	advanced  // open the built-in GUI within the same pause
};

// Whether the open-GUI path shows the menu: the launcher's setting, which
// the activity passes on when it starts. Any thread.
bool android_overlay_enabled();

// Shows the menu and runs until it closes, in place of run_gui(): queued
// actions keep running, and a reset, quit or state load among them closes
// it as the built-in GUI's buttons do. Main thread, inside gui_display().
android_overlay_result android_overlay_run();

// Between frames (handle_events): shows the on-screen keyboard the menu's
// Show keyboard asked for, once the emulation runs with the keyboard on.
void android_overlay_service();

// Once per frame (handle_msgpump): moves the port 2 joystick as the TV
// remote's D-pad and OK are held. The activity turns this on for a TV with
// no gamepad and hands those keys over (OverlayNative.remoteKey) while the
// emulation runs; a key tapped between two frames is held for one frame,
// so the Amiga sees it. Main thread.
void android_remote_joystick_pump();

#endif
