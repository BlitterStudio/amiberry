/*
 * The Android app's in-emulation UI (android/app/src/main/java/com/
 * blitterstudio/amiberry/overlay): a Compose overlay over the SDL surface.
 * Where the built-in GUI would open over a running emulation (Back, the
 * gamepad's Start or Guide, the two-finger swipe, F12, the RetroArch combo,
 * the open_settings action), the emulator shows this menu instead, paused as
 * under the GUI, and the media and state hotkeys open it at their picker or
 * the states list. gui_message's message boxes and gui_ask_disk's recovery
 * requester are native dialogs of the same view. The built-in GUI stays for
 * startup without -G, as the menu's Classic settings window, and for people
 * who turn the menu off in the launcher.
 */
#pragma once

#include <string>

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
// it as the built-in GUI's buttons do. The page is the media or state
// hotkey's shortcut as gui_display received it: 0-3 the DF0-3 picker, 4 and
// 5 the states list, 6 the CD picker; -1 (or anything else) the menu's
// Home. Main thread, inside gui_display().
android_overlay_result android_overlay_run(int page = -1);

// gui_message's message box: queued, and shown by the overlay's Compose
// view over whatever runs, without stopping the emulation. Any thread.
void android_overlay_message(const char* text);

// gui_ask_disk's requester: the recovery dialog shows over the last frame
// while this waits, as the built-in GUI's did (the emulator thread,
// mid-instruction, keeps SDL alive). True with the replacement image in
// chosen (empty: eject the drive), false when the dialog was skipped or
// cancelled. Main thread.
bool android_overlay_ask_disk(int drive, const char* missing_path, std::string& chosen);

// The native on-screen Amiga keyboard (the Compose overlay's
// OverlayKeyboard, in place of the ImGui one): shown over the running
// emulation, it pauses nothing. The emulator thread toggles it (every way
// in arrives there: the AKS_OSK hotkey, a controller button, the on-screen
// joystick's KB button, the menu's Keyboard entry); the view owns the keys
// it has down (they go through amiberry_actions_key) and hides with them
// released, so a hide from here only has to tell it. Emulator thread for
// the toggle; any thread for the rest.
void android_osk_toggle();
void android_osk_hide();
bool android_osk_is_active();

// Between frames (handle_events): shows the on-screen keyboard the menu's
// Show keyboard asked for, once the emulation runs with the keyboard on.
void android_overlay_service();

// Once per frame (handle_msgpump): moves the port 2 joystick as the TV
// remote's D-pad and OK are held. The activity turns this on for a TV with
// no gamepad and hands those keys over (OverlayNative.remoteKey) while the
// emulation runs; a key tapped between two frames is held for one frame,
// so the Amiga sees it. Main thread.
void android_remote_joystick_pump();

#endif // __ANDROID__
