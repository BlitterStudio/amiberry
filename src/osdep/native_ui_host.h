/*
 * The native macOS front end (apple/macOS, -DAMIBERRY_NATIVE_APPLE_UI=ON):
 * where the built-in GUI would show, the emulator calls these instead, so
 * the macOS app shows no ImGui. Other builds keep the built-in GUI.
 */
#pragma once

#if defined(AMIBERRY_NATIVE_APPLE_UI) && defined(AMIBERRY_MACOS)
#define AMIBERRY_NATIVE_HOST 1

#include <string>

// Shows the native settings window and runs until it closes, in place of
// run_gui(): queued actions and IPC keep running, and a reset, quit or
// state load among them closes it as the built-in GUI's buttons do.
void native_host_run_settings();
// Installs the menu bar commands; once SDL has created the application.
void native_host_install_menus();

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
