// Runtime actions and status of the shared settings layer
// (settings/amiberry_actions.h). Front ends post actions from their own
// threads; the emulator main thread drains them in order between frames and
// while the GUI is open, which is where amiberry_actions_run_pending() is
// called. The executors wrap exactly what the ImGui GUI and the IPC
// transport did until now, so behaviour does not depend on the caller.

#include "sysconfig.h"
#include "sysdeps.h"

#include "settings/amiberry_actions.h"
#include "settings/settings_actions_queue.h"

#include "options.h"
#include "memory.h"
#include "disk.h"
#include "savestate.h"
#include "custom.h"
#include "gui.h"
#include "uae.h"
#include "target.h"
#include "inputdevice.h"
#ifdef DEBUGGER
#include "debug.h"
#include "gdb_server.h"
#endif

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>

using amiberry_actions::kind;
using amiberry_actions::request;

namespace {

amiberry_actions::action_queue& queue()
{
	static amiberry_actions::action_queue instance;
	return instance;
}

// The strings are the ones the IPC transport replies with; it forwards them
// unchanged.
bool execute(const request& r, void*, std::string* error)
{
	switch (r.k) {
	case kind::insert_floppy:
		_tcsncpy(changed_prefs.floppyslots[r.drive].df, r.path.c_str(), MAX_DPATH);
		changed_prefs.floppyslots[r.drive].df[MAX_DPATH - 1] = 0;
		set_last_active_config_from_media(r.path.c_str());
		set_config_changed();
		return true;

	case kind::eject_floppy:
		disk_eject(r.drive);
		return true;

	case kind::set_swapper_entry:
		// Same as the disk swapper panel's file dialog: the pending list
		// only; no config change until something inserts from it.
		_tcsncpy(changed_prefs.dfxlist[r.drive], r.path.c_str(), MAX_DPATH - 1);
		changed_prefs.dfxlist[r.drive][MAX_DPATH - 1] = 0;
		return true;

	case kind::insert_swapper_entry:
		// Eject the entry from any drive that holds it, then insert it into
		// the requested drive. Both the list and the drives read are the
		// pending ones, so swapper actions posted back to back compose in
		// posting order, and an entry just stored through
		// amiberry_actions_set_swapper_entry (or the GUI's swapper panel)
		// is insertable immediately; until this API, the IPC DISKSWAP read
		// the effective list and drives, which lag those changes until the
		// next configuration change.
		for (int i = 0; i < AMIBERRY_ACTIONS_FLOPPY_DRIVES; i++) {
			if (!_tcscmp(changed_prefs.floppyslots[i].df, changed_prefs.dfxlist[r.drive]))
				changed_prefs.floppyslots[i].df[0] = 0;
		}
		_tcscpy(changed_prefs.floppyslots[r.number].df, changed_prefs.dfxlist[r.drive]);
		set_config_changed();
		return true;

	case kind::insert_cd:
		_tcsncpy(changed_prefs.cdslots[0].name, r.path.c_str(), MAX_DPATH);
		changed_prefs.cdslots[0].name[MAX_DPATH - 1] = 0;
		changed_prefs.cdslots[0].inuse = true;
		set_last_active_config_from_media(r.path.c_str());
		set_config_changed();
		return true;

	case kind::eject_cd:
		changed_prefs.cdslots[0].name[0] = 0;
		changed_prefs.cdslots[0].inuse = false;
		set_config_changed();
		return true;

	case kind::reset:
		uae_reset(r.flag_a, r.flag_b);
		return true;

	case kind::pause:
		setpaused(3);
		activationtoggle(0, true);
		return true;

	case kind::resume:
		// The settings GUI (and other higher-priority holders) pause at a
		// higher priority than this action may lift; emulation would stay
		// paused, so the action fails instead of reporting success.
		if (pause_emulation > 3) {
			*error = "Emulation is paused by the settings GUI; close it to resume";
			return false;
		}
		resumepaused(3);
		activationtoggle(0, false);
		return true;

	case kind::save_state:
	case kind::save_state_slot:
#ifdef DEBUGGER
		if (debugger_is_stopped() || gdb_server_halted()) {
			*error = "Cannot save state while the debugger has the CPU stopped; resume or PAUSE first";
			return false;
		}
#endif
		if (r.k == kind::save_state) {
			savestate_initsave(r.path.c_str(), 1, true, true);
			// save_state() returns 0 when the file cannot be written (or -1
			// while a filesystem is active); the transports must not report
			// success, and SAVESTATE must not write its companion config,
			// when no usable state was saved.
			if (save_state(r.path.c_str(), "...") <= 0) {
				*error = "Failed to save state to: " + r.path;
				return false;
			}
		} else if (savestate_quick(r.number, 1) <= 0) {
			*error = "Failed to save quick-save slot " + std::to_string(r.number);
			return false;
		}
		return true;

	case kind::load_state:
		savestate_state = STATE_DORESTORE;
		_tcscpy(savestate_fname, r.path.c_str());
		return true;

	case kind::load_state_slot:
		// 0 means there was nothing to load (no usable state file name, or
		// the slot's file does not exist); the restore was not scheduled.
		if (savestate_quick(r.number, 0) == 0) {
			*error = "No state file for quick-save slot " + std::to_string(r.number);
			return false;
		}
		return true;

	case kind::screenshot:
		if (!create_screenshot()) {
			*error = "Failed to create screenshot";
			return false;
		}
		if (save_thumb(r.path) == 0) {
			*error = "Failed to save screenshot to: " + r.path;
			return false;
		}
		return true;

	case kind::quit:
		uae_quit();
		return true;
	}
	*error = "unknown action";
	return false;
}

// The status snapshot. Written by the main thread in publish_status(),
// copied by any thread in amiberry_actions_get_status().
std::mutex status_mutex;
amiberry_actions_status status_snapshot;
unsigned status_revision = 0;

// The pending disk swapper list, same locking as the snapshot. Only copied
// when it changed, so the per-frame publish stays cheap.
std::mutex swapper_mutex;
TCHAR swapper_entries[MAX_SPARE_DRIVES][MAX_DPATH];
bool swapper_valid = false;
static_assert(MAX_SPARE_DRIVES == AMIBERRY_ACTIONS_SWAPPER_ENTRIES,
	"the swapper mirror must have exactly the core's number of entries");
static_assert(MAX_DPATH <= AMIBERRY_ACTIONS_PATH_MAX,
	"the public path buffer must fit the core's MAX_DPATH");

void copy_text(char* out, size_t size, const TCHAR* text)
{
	std::snprintf(out, size, "%s", text ? text : "");
}

void publish_status()
{
	amiberry_actions_status s{};
	s.run_state = !emulating ? AMIBERRY_RUN_STATE_STOPPED
		: (pause_emulation ? AMIBERRY_RUN_STATE_PAUSED : AMIBERRY_RUN_STATE_RUNNING);
	s.paused = pause_emulation != 0;
	s.power_led = gui_data.powerled;
	s.caps_lock = gui_data.capslock;
	s.hd_activity = gui_data.hd;
	s.cd_activity = gui_data.cd;
	for (int i = 0; i < AMIBERRY_ACTIONS_FLOPPY_DRIVES; i++) {
		s.floppy[i].motor_on = gui_data.drives[i].drive_motor;
		s.floppy[i].writing = gui_data.drives[i].drive_writing;
		s.floppy[i].disabled = gui_data.drives[i].drive_disabled;
		s.floppy[i].media_requested = changed_prefs.floppyslots[i].df[0] != 0;
		copy_text(s.floppy[i].media_path, sizeof s.floppy[i].media_path,
			changed_prefs.floppyslots[i].df);
	}
	s.cd_requested = changed_prefs.cdslots[0].inuse && changed_prefs.cdslots[0].name[0] != 0;
	copy_text(s.cd_path, sizeof s.cd_path, changed_prefs.cdslots[0].name);
	copy_text(s.config_description, sizeof s.config_description,
		changed_prefs.description);

	{
		std::lock_guard<std::mutex> lock(status_mutex);
		status_snapshot = s;
		status_snapshot.revision = ++status_revision;
	}

	if (!swapper_valid
		|| std::memcmp(swapper_entries, changed_prefs.dfxlist,
			sizeof changed_prefs.dfxlist) != 0) {
		std::lock_guard<std::mutex> lock(swapper_mutex);
		std::memcpy(swapper_entries, changed_prefs.dfxlist,
			sizeof changed_prefs.dfxlist);
		swapper_valid = true;
	}
}

bool valid_path(const char* path)
{
	// The prefs fields the executors write are MAX_DPATH-sized; accept
	// exactly what they can store, like the IPC handlers did. An empty path
	// is allowed: it is how the transports clear a media slot, exactly what
	// writing an empty path to changed_prefs always did.
	return path && std::strlen(path) < MAX_DPATH;
}

amiberry_action_id post(kind k, int drive = 0, int number = 0,
	bool flag_a = false, bool flag_b = false, const char* path = nullptr)
{
	return queue().post(k, drive, number, flag_a, flag_b, path);
}

} // namespace

extern "C" {

amiberry_action_id amiberry_actions_insert_floppy(int drive, const char* path)
{
	if (drive < 0 || drive >= AMIBERRY_ACTIONS_FLOPPY_DRIVES || !valid_path(path))
		return 0;
	return post(kind::insert_floppy, drive, 0, false, false, path);
}

amiberry_action_id amiberry_actions_eject_floppy(int drive)
{
	if (drive < 0 || drive >= AMIBERRY_ACTIONS_FLOPPY_DRIVES)
		return 0;
	return post(kind::eject_floppy, drive);
}

amiberry_action_id amiberry_actions_set_swapper_entry(int index, const char* path)
{
	if (index < 0 || index >= AMIBERRY_ACTIONS_SWAPPER_ENTRIES || !valid_path(path))
		return 0;
	return post(kind::set_swapper_entry, index, 0, false, false, path);
}

amiberry_action_id amiberry_actions_insert_swapper_entry(int index, int drive)
{
	if (index < 0 || index >= AMIBERRY_ACTIONS_SWAPPER_ENTRIES
		|| drive < 0 || drive >= AMIBERRY_ACTIONS_FLOPPY_DRIVES)
		return 0;
	return post(kind::insert_swapper_entry, index, drive);
}

amiberry_action_id amiberry_actions_insert_cd(const char* path)
{
	if (!valid_path(path))
		return 0;
	return post(kind::insert_cd, 0, 0, false, false, path);
}

amiberry_action_id amiberry_actions_eject_cd(void)
{
	return post(kind::eject_cd);
}

amiberry_action_id amiberry_actions_reset(bool hard, bool keyboard)
{
	return post(kind::reset, 0, 0, hard, keyboard);
}

amiberry_action_id amiberry_actions_pause(void)
{
	return post(kind::pause);
}

amiberry_action_id amiberry_actions_resume(void)
{
	return post(kind::resume);
}

amiberry_action_id amiberry_actions_save_state(const char* path)
{
	if (!valid_path(path))
		return 0;
	return post(kind::save_state, 0, 0, false, false, path);
}

amiberry_action_id amiberry_actions_load_state(const char* path)
{
	if (!valid_path(path))
		return 0;
	return post(kind::load_state, 0, 0, false, false, path);
}

amiberry_action_id amiberry_actions_save_state_slot(int slot)
{
	if (slot < 0 || slot > 9)
		return 0;
	return post(kind::save_state_slot, 0, slot);
}

amiberry_action_id amiberry_actions_load_state_slot(int slot)
{
	if (slot < 0 || slot > 9)
		return 0;
	return post(kind::load_state_slot, 0, slot);
}

amiberry_action_id amiberry_actions_screenshot(const char* path)
{
	if (!valid_path(path))
		return 0;
	return post(kind::screenshot, 0, 0, false, false, path);
}

amiberry_action_id amiberry_actions_quit(void)
{
	return post(kind::quit);
}

amiberry_action_result amiberry_actions_poll(amiberry_action_id id,
	char* error_buffer, size_t size)
{
	if (error_buffer && size)
		error_buffer[0] = 0;
	if (id == 0)
		return AMIBERRY_ACTION_UNKNOWN;
	// No C++ exception may cross into C callers.
	try {
		std::string error;
		const int state = queue().poll(id, error_buffer ? &error : nullptr);
		if (error_buffer && size)
			std::snprintf(error_buffer, size, "%s", error.c_str());
		switch (state) {
		case 1: return AMIBERRY_ACTION_DONE;
		case 2: return AMIBERRY_ACTION_FAILED;
		case 3: return AMIBERRY_ACTION_UNKNOWN;
		default: return AMIBERRY_ACTION_PENDING;
		}
	} catch (...) {
		return AMIBERRY_ACTION_UNKNOWN;
	}
}

size_t amiberry_actions_run_pending(void)
{
	const size_t executed = queue().run_pending(execute, nullptr);
	publish_status();
	return executed;
}

void amiberry_actions_get_status(amiberry_actions_status* out)
{
	if (!out)
		return;
	std::lock_guard<std::mutex> lock(status_mutex);
	*out = status_snapshot;
}

void amiberry_actions_get_swapper_entry(int index, char* buffer, size_t size)
{
	if (!buffer || !size)
		return;
	buffer[0] = 0;
	if (index < 0 || index >= AMIBERRY_ACTIONS_SWAPPER_ENTRIES)
		return;
	std::lock_guard<std::mutex> lock(swapper_mutex);
	std::snprintf(buffer, size, "%s", swapper_entries[index]);
}

} // extern "C"
