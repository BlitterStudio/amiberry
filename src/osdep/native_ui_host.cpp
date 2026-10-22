/*
 * C++ side of the native macOS front end (see native_ui_host.h); the
 * windows and menus are Swift (apple/macOS).
 */
#include "sysconfig.h"
#include "sysdeps.h"

#include "native_ui_host.h"

#include "options.h"
#include "uae.h"
#include "target.h"
#include "perf_monitor.h"
#include "gui/gui_handling.h"
#include "amiberry_actions_host.h"
#include "settings/amiberry_actions.h"
#include "settings/amiberry_settings.h"
#ifdef USE_IPC_SOCKET
#include "amiberry_ipc.h"
#endif

#include <SDL3/SDL.h>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

extern int emulating;

// Swift (apple/macOS/NativeHost.swift).
extern "C" {
void amiberry_native_ui_settings_show(bool emulating);
bool amiberry_native_ui_settings_visible(void);
void amiberry_native_ui_settings_dismiss(void);
void amiberry_native_ui_settings_commit(void);
void amiberry_native_ui_settings_devices_changed(void);
void amiberry_native_ui_install_menus(void);
bool amiberry_native_ui_choose_file(int kind, const char* title, const char* start_path,
	char* chosen, size_t size);
void amiberry_native_ui_message(const char* title, const char* message);
int amiberry_native_ui_alert(const char* title, const char* message, const char* const* buttons,
	int button_count, const char* suppression, bool* suppressed);
}

namespace {

int alert(const std::string& title, const std::string& message, const std::vector<const char*>& buttons,
	const char* suppression = nullptr, bool* suppressed = nullptr)
{
	return amiberry_native_ui_alert(title.c_str(), message.c_str(), buttons.data(),
		static_cast<int>(buttons.size()), suppression, suppressed);
}

// The notices the built-in GUI shows when it opens (run_gui), as alerts
// with the same choices.
void startup_notices()
{
	native_host_show_deferred_messages();
	std::string title, message;
	if (consume_startup_migration_notice(title, message))
		native_host_message(title.c_str(), message.c_str());

	if (should_show_legacy_cleanup_prompt()) {
		const bool trash = legacy_cleanup_uses_trash();
		std::string error;
		for (;;) {
			std::string text = "Amiberry found leftover files and folders from the previous layout.\n\n"
				"Your active paths already use the new locations, so cleaning these up will not change "
				"the settings and visuals Amiberry is using now.\n\n"
				"If everything looks right, Amiberry can move the leftovers ";
			text += trash ? "to Trash" : "aside";
			text += " for safekeeping. You can also keep them for now and decide later.";
			if (!error.empty())
				text += "\n\n" + error;
			text += "\n\nItems to clean up:";
			for (const auto& item : get_legacy_cleanup_prompt_items())
				text += "\n- " + item;
			const int choice = alert("Clean Up Old Files", text,
				{trash ? "Move to Trash" : "Archive Old Items", "Ask Later", "Don't Ask Again"});
			if (choice == 1) {
				postpone_legacy_cleanup_prompt();
				break;
			}
			if (choice == 2) {
				dismiss_legacy_cleanup_prompt();
				break;
			}
			std::vector<std::string> failed;
			if (cleanup_legacy_items(failed))
				break;
			error = trash ? "Some legacy items could not be moved to Trash:"
				: "Some legacy items could not be moved aside:";
			for (const auto& item : failed)
				error += "\n- " + item;
			error += "\n\nThey were left in place.";
		}
	}

	if (perf_monitor_warning_pending()) {
		bool dont_ask = false;
		const int choice = alert("Performance Warning",
			"This system cannot keep up with the current settings - the emulated Amiga is running "
			"below full speed, which also affects audio.\n\n"
			"Enabling resolution autoswitch lets Amiberry drop the output to low resolution when the "
			"displayed screen allows it, which significantly reduces the rendering cost. Emulation "
			"accuracy is unchanged - only horizontal sharpness is softened on high-resolution screens.",
			{"Enable resolution autoswitch", "Keep settings"}, "Don't warn me again", &dont_ask);
		if (choice == 0)
			changed_prefs.gfx_autoresolution = 1;
		perf_monitor_clear_warning();
		if (dont_ask) {
			amiberry_options.slow_host_warning = false;
			save_amiberry_settings();
		}
	}
}

} // namespace

void native_host_run_settings()
{
	startup_notices();
	amiberry_native_ui_settings_show(emulating != 0);
	while (amiberry_native_ui_settings_visible()) {
		// As the built-in GUI's loop: a reset, quit or state load executed
		// here (IPC runs its commands' actions at once) is carried out by the
		// emulation loop, so the window closes.
		const unsigned long requests_before = amiberry_actions_emulation_requests();
#ifdef USE_IPC_SOCKET
		Amiberry::IPC::IPCHandle();
#endif
		amiberry_actions_run_pending();
		if (amiberry_actions_emulation_requests() != requests_before) {
			// A reset (or quit, or state load) from the menu or IPC closes
			// the window as its own buttons do: with the edits stored, so a
			// reset starts with them.
			amiberry_native_ui_settings_commit();
			break;
		}
		// Messages from worker threads while the window is up.
		native_host_show_deferred_messages();
		// Waiting for an SDL event also runs the application's event loop,
		// which drives the settings window.
		SDL_Event event;
		if (SDL_WaitEventTimeout(&event, 16)) {
			do {
				if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
					// Closing the emulation window quits, as it does while
					// the emulation runs (handle_close_event).
					uae_quit();
					amiberry_native_ui_settings_dismiss();
				} else if (event.type == SDL_EVENT_JOYSTICK_ADDED || event.type == SDL_EVENT_JOYSTICK_REMOVED) {
					// As run_gui: controllers plugged in meanwhile are picked up.
					handle_joy_device_event(event.jdevice.which, event.type == SDL_EVENT_JOYSTICK_REMOVED);
					// The input choices list the host's devices.
					amiberry_settings_refresh_choices();
					amiberry_native_ui_settings_devices_changed();
				}
			} while (SDL_PollEvent(&event));
		}
	}
	// What the window's buttons queued as they closed it (Quit, Reset,
	// Restart) runs now, so the emulation loop sees it before it starts or
	// resumes, as after the built-in GUI's buttons.
	amiberry_actions_run_pending();
	amiberry_native_ui_settings_dismiss();
}

bool native_host_settings_open()
{
	return amiberry_native_ui_settings_visible();
}

void native_host_install_menus()
{
	amiberry_native_ui_install_menus();
}

bool native_host_choose_file(native_host_file kind, const std::string& title,
	const std::string& start_path, std::string& chosen)
{
	std::vector<char> buffer(MAX_DPATH);
	if (!amiberry_native_ui_choose_file(static_cast<int>(kind), title.c_str(), start_path.c_str(),
			buffer.data(), buffer.size()))
		return false;
	chosen = buffer.data();
	return !chosen.empty();
}

void native_host_message(const char* title, const char* message)
{
	amiberry_native_ui_message(title ? title : "", message ? message : "");
}

namespace {
std::mutex deferred_mutex;
std::vector<std::string> deferred_messages;
}

void native_host_message_deferred(const char* message)
{
	// A message repeated while the window is closed (a failing write
	// reports every attempt) shows once; at most a few distinct ones wait.
	constexpr size_t limit = 8;
	const std::string text = message ? message : "";
	std::lock_guard<std::mutex> lock(deferred_mutex);
	for (const auto& waiting : deferred_messages) {
		if (waiting == text)
			return;
	}
	if (deferred_messages.size() < limit)
		deferred_messages.push_back(text);
}

void native_host_show_deferred_messages()
{
	std::vector<std::string> messages;
	{
		std::lock_guard<std::mutex> lock(deferred_mutex);
		messages.swap(deferred_messages);
	}
	for (const auto& message : messages)
		native_host_message("Message", message.c_str());
}
