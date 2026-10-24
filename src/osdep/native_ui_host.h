/*
 * The native Apple front ends (apple/Shared/UI with apple/macOS or
 * apple/iOS, -DAMIBERRY_NATIVE_APPLE_UI=ON): where the built-in GUI would
 * show, the emulator calls these instead, so the Apple apps show no ImGui.
 * Other builds keep the built-in GUI.
 */
#pragma once

#if defined(AMIBERRY_NATIVE_APPLE_UI) && (defined(AMIBERRY_MACOS) || defined(AMIBERRY_IOS))
#define AMIBERRY_NATIVE_HOST 1

#include <string>

// Shows the native settings window and runs until it closes, in place of
// run_gui(): queued actions and IPC keep running, and a reset, quit or
// state load among them closes it as the built-in GUI's buttons do.
void native_host_run_settings();
// Installs the front end's commands once SDL has created the application:
// the menu bar on macOS, the emulation overlay (settings and keyboard
// buttons) on iOS.
void native_host_install_menus();
// SDL has created the emulation window (macOS: its toolbar goes on;
// nothing on iOS, where the overlay is installed once). Main thread only.
struct SDL_Window;
void native_host_emulation_window_created(SDL_Window* window);
// The native on-screen Amiga keyboard (iOS; nothing on macOS, which keeps
// the built-in one until its GUI is compiled out): shown or hidden by its
// hotkey and the on-screen joystick's keyboard button, hidden when input
// leaves the emulation. Main thread only.
void native_host_toggle_keyboard();
void native_host_hide_keyboard();
// The autosave state (iOS; nothing on macOS): the running emulation is
// saved as the app goes to the background, where the system may end it, and
// the settings UI offers to resume it. Requested from the lifecycle event
// (main thread, which can be mid-frame), written by the service call at the
// next point between frames: the emulation loop's handle_events() and the
// settings loop.
void native_host_request_autosave();
void native_host_service_autosave();
// Marks a native front end call made between frames (a hotkey's
// requester), where its modal waits may write a requested autosave.
struct native_host_between_frames {
	native_host_between_frames();
	~native_host_between_frames();
	native_host_between_frames(const native_host_between_frames&) = delete;
	native_host_between_frames& operator=(const native_host_between_frames&) = delete;
};
// The autosave state's path.
std::string native_host_autosave_path();

enum class native_host_file {
	floppy,     // a disk image to insert
	cd,         // a CD image to insert
	load_state, // a state file to load
	save_state  // where to save a state
};
// Asks the user for a file; false when cancelled.
bool native_host_choose_file(native_host_file kind, const std::string& title,
	const std::string& start_path, std::string& chosen);
// Shows a message and waits until it is dismissed. Main thread only.
void native_host_message(const char* title, const char* message);
// Keeps a message for the next time the settings window opens, as the
// built-in GUI did; any thread.
void native_host_message_deferred(const char* message);
void native_host_show_deferred_messages();
// Whether the settings window is up. Main thread only.
bool native_host_settings_open();

#endif
