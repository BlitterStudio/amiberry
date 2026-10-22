// A file requester rendered over the built-in GUI, independently of the
// active panel, for the media and state hotkeys and the Savestates panel's
// file buttons (request_disk_selection in amiberry_gui.cpp).
#include "sysdeps.h"
#include "imgui_panels.h"
#include "file_dialog.h"
#include "gui/gui_handling.h"

#include <string>
#include <utility>

namespace {

enum class state { idle, requested, active };

constexpr const char* dialog_key = "MEDIA_REQUEST";

state request_state = state::idle;
std::string request_title;
std::string request_filters;
std::string request_initial;
bool request_save = false;
bool request_close_gui = false;
MediaRequestDone request_done;

// Cancelled: nothing to use, so a hotkey's GUI closes.
void cancel()
{
	request_state = state::idle;
	request_done = nullptr;
	if (request_close_gui)
		gui_running = false;
}

} // namespace

void MediaRequestDialog_Open(const char* title, const char* filters, const std::string& initial_path,
	bool save, bool close_gui, MediaRequestDone done)
{
	request_title = title ? title : "";
	request_filters = filters ? filters : "All Files (*){.*}";
	request_initial = initial_path;
	request_save = save;
	request_close_gui = close_gui;
	request_done = std::move(done);
	request_state = state::requested;
}

void MediaRequestDialog_Reset()
{
	if (request_state == state::active)
		CloseFileDialogKey(dialog_key);
	request_state = state::idle;
	request_done = nullptr;
}

void MediaRequestDialog_Render()
{
	// Not in the frame the GUI closes in: Reset drops the request then.
	if (!gui_running)
		return;
	if (request_state == state::requested) {
		OpenFileDialogKey(dialog_key, request_title.c_str(), request_filters.c_str(), request_initial, request_save);
		request_state = state::active;
	}
	if (request_state != state::active)
		return;

	std::string path;
	if (ConsumeFileDialogResultKey(dialog_key, path)) {
		const MediaRequestDone done = std::move(request_done);
		const bool close_gui = request_close_gui;
		request_state = state::idle;
		request_done = nullptr;
		const bool used = !done || done(path);
		if (close_gui && used)
			gui_running = false;
	} else if (!IsFileDialogOpenKey(dialog_key)) {
		cancel();
	}
}
