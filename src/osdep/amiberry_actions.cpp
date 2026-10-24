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
#include "amiberry_actions_host.h"

#include "options.h"
#include "memory.h"
#include "disk.h"
#include "savestate.h"
#include "custom.h"
#include "gui.h"
#include "uae.h"
#include "target.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "zfile.h"
#include "xwin.h"
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

#ifndef LIBRETRO
extern int gui_active; // amiberry_gui.cpp: the settings UI is open
#endif

namespace {

amiberry_actions::action_queue& queue()
{
	static amiberry_actions::action_queue instance;
	return instance;
}

// Main thread only (the executor and the GUI loop).
unsigned long emulation_requests = 0; // resets, quits and state loads executed
bool pause_after_gui = false;         // a PAUSE executed while the GUI held its pause

// savestate_state and savestate_fname describe a single state operation:
// a restore (or rewind) that the next vsync starts, or a save the guest's
// statefile_save option or the debugger queued for savestate_check(). State
// actions would replace or redirect it (a save rewrites savestate_fname and
// clears savestate_state, a load overwrites both), so they fail until it
// has run.
bool state_operation_pending(std::string* error)
{
	if (!(savestate_state & (STATE_SAVE | STATE_DOSAVE | STATE_DORESTORE | STATE_RESTORE
			| STATE_DOREWIND | STATE_REWIND))) {
		return false;
	}
	*error = "A state save or restore is already pending";
	return true;
}

// Whether the native debugger or a GDB client has the CPU stopped; state
// actions need the CPU running (a save needs a consistent frame, a restore
// needs the reset the stop loops do not process).
bool debugger_holds_cpu(std::string* error, const char* message)
{
#ifdef DEBUGGER
	if (debugger_is_stopped() || gdb_server_halted()) {
		*error = message;
		return true;
	}
#else
	(void)error;
	(void)message;
#endif
	return false;
}

// Why a state load cannot start now, if it cannot.
bool restore_blocked(std::string* error)
{
	if (debugger_holds_cpu(error, "Cannot load state while the debugger has the CPU stopped; resume first")) {
		return true;
	}
	// A quit discards the reset the restore runs in.
	if (quit_program == UAE_QUIT || quit_program == -UAE_QUIT) {
		*error = "The emulator is quitting";
		return true;
	}
	// savestate_state/savestate_fname hold one operation; a second load in
	// the same drain would replace the first after both reported success.
	return state_operation_pending(error);
}

// A scheduled restore (STATE_DORESTORE) runs inside a reset: the emulation
// loop's reset path restores the file. savestate_check() requests that
// reset at the next vsync, but neither the settings GUI nor a paused
// emulation runs one, so the restore would wait for the user. The action
// requests the reset itself, as savestate_check()'s caller does; that also
// ends a pause and closes the GUI (run_gui leaves on a new reset request).
// A reset that is already pending carries the restore with it.
void begin_restore()
{
	uae_reset(0, 0);
	emulation_requests++;
}

} // namespace

// The GUI's "filesys hack": the core attaches drives at a reset from the
// active mount list, which only this copy brings up to the pending one.
void adopt_pending_mount_list()
{
	currprefs.mountitems = changed_prefs.mountitems;
	std::memcpy(static_cast<void*>(currprefs.mountconfig), static_cast<const void*>(changed_prefs.mountconfig),
		sizeof currprefs.mountconfig);
}

namespace {

// Brings the running emulation up to the pending configuration for an
// apply class (amiberry_actions_apply_settings).
bool apply_settings(int apply, std::string* error)
{
	if (apply != AMIBERRY_APPLY_LIVE && apply != AMIBERRY_APPLY_RESET && apply != AMIBERRY_APPLY_RESTART) {
		*error = "Unknown apply class";
		return false;
	}
	// First: a restart in progress also clears emulating, and what is
	// committed now would not reach the emulation it starts.
	const bool reset_pending = quit_program <= -UAE_RESET && quit_program >= -UAE_RESET_HARD;
	if (quit_program != 0 && !reset_pending) {
		*error = "The emulator is quitting";
		return false;
	}
	// Nothing runs yet: the emulation starts with the pending configuration.
	if (!emulating)
		return true;
#ifdef LIBRETRO
	// Refused before anything is signalled, so the running configuration
	// stays as it is.
	if (apply == AMIBERRY_APPLY_RESTART) {
		*error = "Restarting is up to the libretro front end";
		return false;
	}
#endif
	// What the running emulation picks up between frames.
	set_config_changed();
	if (apply == AMIBERRY_APPLY_LIVE)
		return true;
	if (apply == AMIBERRY_APPLY_RESET) {
		// As the GUI does before its Reboot (gui_to_prefs): drives attach
		// from the active mount list, and memory is rebuilt.
		adopt_pending_mount_list();
		memory_hardreset(2);
		if (quit_program == 0)
			uae_reset(1, 1);
		else
			quit_program = -UAE_RESET_HARD;
		emulation_requests++;
		return true;
	}
#ifndef LIBRETRO
	uae_restart_with_pending_prefs();
	emulation_requests++;
#endif
	return true;
}

// The strings are the ones the IPC transport replies with; it forwards them
// unchanged.
bool execute(const request& r, void*, std::string* error)
{
	// Once a restart is scheduled the running emulation is being replaced:
	// what an action changed now would not reach the one that starts. A
	// quit still wins over the restart.
	if (uae_restart_pending()) {
		if (r.k == kind::quit) {
			uae_cancel_restart();
			uae_quit();
			emulation_requests++;
			return true;
		}
		// A key release always goes through (amiberry_actions_key), so no
		// key stays down in the emulation being replaced.
#ifndef LIBRETRO
		if (r.k == kind::key && !r.flag_a) {
			inputdevice_do_keyboard(r.number, 0);
			return true;
		}
#endif
		*error = "The emulator is restarting";
		return false;
	}
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

	case kind::reset: {
		// uae_reset() ignores a request while another reset or a quit is
		// pending. Two resets in one drain combine into the stronger one
		// (hard over keyboard over soft), which covers both requests; a
		// reset behind a pending quit cannot happen and fails.
		const int requested = r.flag_a ? -UAE_RESET_HARD
			: r.flag_b ? -UAE_RESET_KEYBOARD : -UAE_RESET;
		if (quit_program == 0) {
			uae_reset(r.flag_a, r.flag_b);
			emulation_requests++;
			return true;
		}
		if (quit_program <= -UAE_RESET && quit_program >= -UAE_RESET_HARD) {
			if (requested < quit_program)
				quit_program = requested;
			emulation_requests++;
			return true;
		}
		*error = "The emulator is quitting";
		return false;
	}

	case kind::pause:
		// The settings GUI pauses at a higher priority (7) and releases its
		// pause when it closes, which would resume emulation behind this
		// action's back. Emulation is paused now; the request is kept and
		// re-applied when the GUI closes (amiberry_actions_gui_closed).
		if (pause_emulation == 7) {
			pause_after_gui = true;
			return true;
		}
		setpaused(3);
		activationtoggle(0, true);
		return true;

	case kind::resume:
		// A pause requested while the GUI was open is simply dropped.
		if (pause_emulation == 7 && pause_after_gui) {
			pause_after_gui = false;
			return true;
		}
		// The settings GUI and the pause key pause at a higher priority
		// than this action may lift; emulation would stay paused, so the
		// action fails instead of reporting success.
		if (pause_emulation > 3) {
			*error = pause_emulation == 7
				? "Emulation is paused by the settings GUI; close it to resume"
				: "Emulation is paused by the pause key; press it to resume";
			return false;
		}
		resumepaused(3);
		activationtoggle(0, false);
		return true;

	case kind::save_state:
	case kind::save_state_slot:
		if (debugger_holds_cpu(error, "Cannot save state while the debugger has the CPU stopped; resume or PAUSE first")) {
			return false;
		}
		// Saving rewrites savestate_fname, which would redirect a pending
		// restore or save to the file just saved.
		if (state_operation_pending(error)) {
			return false;
		}
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
		if (restore_blocked(error)) {
			return false;
		}
		// A restore starts with a reset, which happens even when
		// restore_state() then rejects the file; check what it checks (the
		// file opens and has the AmigaStateFile header) first, so a bad
		// path fails without touching the guest.
		if (r.path.empty() || !zfile_exists(r.path.c_str())) {
			*error = "State file not found: " + r.path;
			return false;
		}
		if (!savestate_is_state_file(r.path.c_str())) {
			*error = "Not a state file: " + r.path;
			return false;
		}
		savestate_state = STATE_DORESTORE;
		_tcscpy(savestate_fname, r.path.c_str());
		begin_restore();
		return true;

	case kind::load_state_slot:
		// savestate_quick() writes savestate_fname even when the slot is
		// empty, so a pending operation is checked before calling it.
		if (restore_blocked(error)) {
			return false;
		}
		// 0 means there was nothing to load (no usable state file name, or
		// the slot's file is missing or not a state file); the restore was
		// not scheduled.
		if (savestate_quick(r.number, 0) == 0) {
			*error = "No state file for quick-save slot " + std::to_string(r.number);
			return false;
		}
		begin_restore();
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

	case kind::load_config: {
		// target_cfgfile_load() resets the configuration to the defaults
		// before it reads the file, so a file it cannot read would leave
		// the defaults behind; check it opens first.
		const std::string extension = r.path.size() > 4 ? r.path.substr(r.path.size() - 4) : std::string();
		const bool uae = _tcsicmp(extension.c_str(), ".uae") == 0;
		if (!uae && _tcsicmp(extension.c_str(), ".rp9") != 0) {
			*error = "Not a configuration file: " + r.path;
			return false;
		}
		// cfgfile_load() retries a name it cannot open under the
		// configuration directory (LOAD_CONFIG Games.uae). Resolve that
		// here, so the checks below see the file the loader will read.
		std::string path = r.path;
		if (!zfile_exists(path.c_str())) {
			TCHAR directory[MAX_DPATH];
			get_configuration_path(directory, sizeof directory / sizeof(TCHAR));
			const std::string fallback = std::string(directory) + path;
			if (zfile_exists(fallback.c_str()))
				path = fallback;
		}
		if (!zfile_exists(path.c_str())) {
			*error = "Configuration file not found: " + r.path;
			return false;
		}
		// target_cfgfile_load() resets the configuration before it opens a
		// .uae, so a file that exists but cannot be opened (no permission)
		// would still wipe it. Open it the way the loader does, first.
		if (uae) {
			zfile* file = zfile_fopen(path.c_str(), "r", ZFD_NORMAL);
			if (!file) {
				*error = "Failed to load configuration: " + r.path;
				return false;
			}
			zfile_fclose(file);
		}
		// As the GUI's Load: the file becomes the pending configuration
		// (the floppies it names are inserted at once).
		if (!target_cfgfile_load(&changed_prefs, path.c_str(), CONFIG_TYPE_DEFAULT, 0)) {
			*error = "Failed to load configuration: " + r.path;
			return false;
		}
		set_config_changed();
		return true;
	}

	case kind::save_config:
		if (!cfgfile_save(&changed_prefs, r.path.c_str(), 0)) {
			*error = "Failed to save configuration to: " + r.path;
			return false;
		}
		set_last_active_config(r.path.c_str());
		return true;

	case kind::quit:
		uae_quit();
		emulation_requests++;
		return true;

	case kind::apply_settings:
		return apply_settings(r.number, error);

	case kind::toggle_fullscreen:
#ifdef LIBRETRO
		*error = "The display is up to the libretro front end";
		return false;
#else
		toggle_fullscreen(0, -1);
		return true;
#endif

	case kind::open_settings:
#ifdef LIBRETRO
		*error = "The libretro core has no settings UI";
		return false;
#else
		// The GUI hotkey's request; the emulation loop opens the settings
		// UI at its next input check. Nothing to open while it is open
		// already, or before an emulation runs (it is what shows then).
		if (emulating && gui_active <= 0)
			inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
		return true;
#endif

	case kind::choose_media:
#ifdef LIBRETRO
		*error = "The libretro core has no requesters";
		return false;
#else
		// The hotkeys' requests: gui_display() pauses, releases input and
		// shows the requester at the next input check. The settings UI has
		// the media itself while it is open.
		if (gui_active > 0) {
			*error = "The settings UI is open";
			return false;
		}
		if (emulating) {
			static const int codes[] = {AKS_FLOPPY0, AKS_FLOPPY1, AKS_FLOPPY2, AKS_FLOPPY3,
				AKS_STATERESTOREDIALOG, AKS_STATESAVEDIALOG, AKS_CD0};
			inputdevice_add_inputcode(codes[r.number], 1, nullptr);
		}
		return true;
#endif

	case kind::key:
#ifdef LIBRETRO
		*error = "The keyboard is up to the libretro front end";
		return false;
#else
		// A release always goes through, so a key pressed before the
		// emulation paused does not stay down.
		if (r.flag_a && (!emulating || gui_active > 0)) {
			*error = emulating ? "The settings UI is open" : "No emulation is running";
			return false;
		}
		inputdevice_do_keyboard(r.number, r.flag_a ? 1 : 0);
		return true;
#endif
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

amiberry_action_id amiberry_actions_load_config(const char* path)
{
	if (!valid_path(path))
		return 0;
	return post(kind::load_config, 0, 0, false, false, path);
}

amiberry_action_id amiberry_actions_save_config(const char* path)
{
	if (!valid_path(path))
		return 0;
	return post(kind::save_config, 0, 0, false, false, path);
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

amiberry_action_id amiberry_actions_apply_settings(amiberry_apply_class apply)
{
	return post(kind::apply_settings, 0, static_cast<int>(apply));
}

amiberry_action_id amiberry_actions_toggle_fullscreen(void)
{
	return post(kind::toggle_fullscreen);
}

amiberry_action_id amiberry_actions_open_settings(void)
{
	return post(kind::open_settings);
}

amiberry_action_id amiberry_actions_choose_media(amiberry_media_request request)
{
	if (request < AMIBERRY_REQUEST_FLOPPY0 || request > AMIBERRY_REQUEST_CD)
		return 0;
	return post(kind::choose_media, 0, static_cast<int>(request));
}

amiberry_action_id amiberry_actions_key(int amiga_key, bool pressed)
{
	if (amiga_key < 0 || amiga_key > AMIBERRY_ACTIONS_MAX_AMIGA_KEY)
		return 0;
	return post(kind::key, 0, amiga_key, pressed);
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

unsigned long amiberry_actions_emulation_requests()
{
	return emulation_requests;
}

void amiberry_actions_gui_closed()
{
	if (!pause_after_gui)
		return;
	pause_after_gui = false;
	setpaused(3);
	activationtoggle(0, true);
}
